/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · Apple APFS Copy-on-Write Snapshotter Implementation
 */

#include "oci_spec.h"
#include <copyfile.h>
#include <sys/stat.h>
#include <dirent.h>

int apfs_cow_clone_directory(const char *src_dir, const char *dst_dir) {
    if (!src_dir || !dst_dir) return -1;

    struct stat st;
    if (stat(src_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "APFS CoW error: source directory '%s' does not exist\n", src_dir);
        return -1;
    }

    /* Ensure parent of dst_dir exists */
    char parent[1024];
    snprintf(parent, sizeof(parent), "%s", dst_dir);
    char *slash = strrchr(parent, '/');
    if (slash) {
        *slash = '\0';
        char cmd[1100];
        snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", parent);
        (void)system(cmd);
    }

    /* Remove destination if it already exists */
    char rm_cmd[1100];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", dst_dir);
    (void)system(rm_cmd);

    /* Apple Silicon APFS Instant Copy-on-Write Clone */
    int rc = copyfile(src_dir, dst_dir, NULL, COPYFILE_ALL | COPYFILE_RECURSIVE | COPYFILE_CLONE);
    if (rc != 0) {
        perror("copyfile (APFS CoW Clone) failed");
        return -1;
    }

    return 0;
}

int apfs_create_container_scratch(const char *storage_root,
                                 const char *image_name,
                                 const char *container_id,
                                 char *out_path,
                                 size_t max_len) {
    if (!storage_root || !image_name || !container_id || !out_path) return -1;

    oci_image_metadata_t meta;
    if (oci_get_image_metadata(storage_root, image_name, &meta) != 0) {
        fprintf(stderr, "Image '%s' not found in local storage\n", image_name);
        return -1;
    }

    char container_rootfs[1024];
    snprintf(container_rootfs, sizeof(container_rootfs),
             "%s/containers/%s/rootfs", storage_root, container_id);

    if (access(container_rootfs, F_OK) == 0) {
        snprintf(out_path, max_len, "%s", container_rootfs);
        return 0;
    }

    double t0 = engine_get_time_ms();
    if (apfs_cow_clone_directory(meta.rootfs_dir, container_rootfs) != 0) {
        return -1;
    }
    double t1 = engine_get_time_ms();

    printf("⚡ [apfs-cow] Cloned rootfs for container '%s' in %.3f ms (Zero-Copy)!\n",
           container_id, t1 - t0);

    snprintf(out_path, max_len, "%s", container_rootfs);
    return 0;
}
