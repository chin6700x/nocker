/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: OCI Image Spec & APFS Copy-on-Write Interfaces
 */

#ifndef OCI_SPEC_H
#define OCI_SPEC_H

#include "nocker.h"

#define MAX_LAYERS 32
#define MAX_DIGEST_LEN 128

typedef struct {
    char registry[256];     /* e.g., registry-1.docker.io */
    char repository[256];   /* e.g., library/alpine */
    char tag[64];           /* e.g., latest */
    char full_name[512];    /* e.g., alpine:latest */
} oci_image_ref_t;

typedef struct {
    char digest[MAX_DIGEST_LEN];
    size_t size_bytes;
} oci_layer_info_t;

typedef struct {
    char id[64];            /* sha256 short id (12 hex chars) */
    char full_digest[MAX_DIGEST_LEN];
    char repository[256];
    char tag[64];
    size_t size_bytes;
    int64_t created_at;
    char rootfs_dir[1024];
} oci_image_metadata_t;

/* Registry & OCI Operations */
int oci_parse_image_ref(const char *raw_ref, oci_image_ref_t *ref);
int oci_pull_image(const oci_image_ref_t *ref, const char *storage_root, oci_image_metadata_t *meta);
int oci_get_image_metadata(const char *storage_root, const char *image_name, oci_image_metadata_t *meta);
int oci_list_images_json(const char *storage_root, char *out_json, size_t max_len);

/* APFS Copy-on-Write Snapshotter Operations */
int apfs_cow_clone_directory(const char *src_dir, const char *dst_dir);
int apfs_create_container_scratch(const char *storage_root,
                                 const char *image_name,
                                 const char *container_id,
                                 char *out_path,
                                 size_t max_len);

#endif /* OCI_SPEC_H */
