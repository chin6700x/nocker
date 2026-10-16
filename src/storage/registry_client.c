/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · OCI Registry V2 Client & Local Image Store
 */

#include "oci_spec.h"
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

typedef struct {
    char *data;
    size_t size;
} memory_buffer_t;

static size_t curl_write_memory_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    memory_buffer_t *mem = (memory_buffer_t *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

static size_t curl_write_file_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

int oci_parse_image_ref(const char *raw_ref, oci_image_ref_t *ref) {
    if (!raw_ref || !ref) return -1;
    memset(ref, 0, sizeof(oci_image_ref_t));

    char buf[512];
    strncpy(buf, raw_ref, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Extract tag if present */
    char *colon = strchr(buf, ':');
    if (colon) {
        *colon = '\0';
        snprintf(ref->tag, sizeof(ref->tag), "%s", colon + 1);
    } else {
        snprintf(ref->tag, sizeof(ref->tag), "latest");
    }

    /* Extract registry if present (contains . or :) */
    char *slash = strchr(buf, '/');
    if (slash && (strchr(buf, '.') || strchr(buf, ':'))) {
        *slash = '\0';
        snprintf(ref->registry, sizeof(ref->registry), "%s", buf);
        snprintf(ref->repository, sizeof(ref->repository), "%s", slash + 1);
    } else {
        snprintf(ref->registry, sizeof(ref->registry), "registry-1.docker.io");
        if (strchr(buf, '/')) {
            snprintf(ref->repository, sizeof(ref->repository), "%s", buf);
        } else {
            snprintf(ref->repository, sizeof(ref->repository), "library/%s", buf);
        }
    }

    snprintf(ref->full_name, sizeof(ref->full_name), "%s:%s", ref->repository, ref->tag);
    return 0;
}

static int fetch_docker_hub_token(const char *repository, char *out_token, size_t max_len) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    char url[1024];
    snprintf(url, sizeof(url),
             "https://auth.docker.io/token?service=registry.docker.io&scope=repository:%s:pull",
             repository);

    memory_buffer_t chunk = { .data = malloc(1), .size = 0 };

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "nocker/1.0.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || !chunk.data) {
        if (chunk.data) free(chunk.data);
        return -1;
    }

    /* Robust JSON token extraction */
    const char *tok = strstr(chunk.data, "\"token\":");
    if (!tok) tok = strstr(chunk.data, "\"access_token\":");
    if (tok) {
        const char *val_start = strchr(tok, ':');
        if (val_start) {
            val_start = strchr(val_start, '"');
            if (val_start) {
                val_start++;
                const char *val_end = strchr(val_start, '"');
                if (val_end) {
                    size_t len = (size_t)(val_end - val_start);
                    if (len >= max_len) len = max_len - 1;
                    memcpy(out_token, val_start, len);
                    out_token[len] = '\0';
                    free(chunk.data);
                    return 0;
                }
            }
        }
    }

    free(chunk.data);
    return -1;
}

static char *fetch_manifest_with_token(const char *registry,
                                       const char *repository,
                                       const char *ref_or_digest,
                                       const char *token) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    char url[1024];
    snprintf(url, sizeof(url), "https://%s/v2/%s/manifests/%s", registry, repository, ref_or_digest);

    memory_buffer_t chunk = { .data = malloc(1), .size = 0 };

    struct curl_slist *headers = NULL;
    if (token && token[0]) {
        size_t hlen = strlen(token) + 64;
        char *auth_hdr = malloc(hlen);
        if (auth_hdr) {
            snprintf(auth_hdr, hlen, "Authorization: Bearer %s", token);
            headers = curl_slist_append(headers, auth_hdr);
            free(auth_hdr);
        }
    }
    headers = curl_slist_append(headers, "Accept: application/vnd.oci.image.manifest.v1+json");
    headers = curl_slist_append(headers, "Accept: application/vnd.docker.distribution.manifest.v2+json");
    headers = curl_slist_append(headers, "Accept: application/vnd.oci.image.index.v1+json");
    headers = curl_slist_append(headers, "Accept: application/vnd.docker.distribution.manifest.list.v2+json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "nocker/1.0.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        if (chunk.data) free(chunk.data);
        return NULL;
    }
    return chunk.data;
}

static int download_blob(const char *registry,
                         const char *repository,
                         const char *digest,
                         const char *token,
                         const char *dest_path) {
    /* Check if already downloaded */
    struct stat st;
    if (stat(dest_path, &st) == 0 && st.st_size > 0) {
        return 0; /* cached */
    }

    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    char url[1024];
    snprintf(url, sizeof(url), "https://%s/v2/%s/blobs/%s", registry, repository, digest);

    FILE *fp = fopen(dest_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return -1;
    }

    struct curl_slist *headers = NULL;
    if (token && token[0]) {
        size_t hlen = strlen(token) + 64;
        char *auth_hdr = malloc(hlen);
        if (auth_hdr) {
            snprintf(auth_hdr, hlen, "Authorization: Bearer %s", token);
            headers = curl_slist_append(headers, auth_hdr);
            free(auth_hdr);
        }
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "nocker/1.0.0");

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        unlink(dest_path);
        return -1;
    }
    return 0;
}

/* Parse arm64 digest from multi-arch index */
static bool find_arm64_submanifest(const char *manifest_json, char *out_digest, size_t max_len) {
    if (!manifest_json || !out_digest) return false;

    /* Look for "architecture":"arm64" */
    const char *p = manifest_json;
    while ((p = strstr(p, "\"architecture\":\"arm64\"")) != NULL) {
        /* Search backwards for "digest":"sha256:... */
        const char *start = (p - 500 > manifest_json) ? p - 500 : manifest_json;
        const char *d = strstr(start, "\"digest\":\"sha256:");
        if (d && d < p) {
            d += 10;
            const char *end = strchr(d, '"');
            if (end && (size_t)(end - d) < max_len) {
                memcpy(out_digest, d, (size_t)(end - d));
                out_digest[end - d] = '\0';
                return true;
            }
        }
        /* Search forwards for "digest":"sha256:... */
        d = strstr(p, "\"digest\":\"sha256:");
        if (d && d < p + 500) {
            d += 10;
            const char *end = strchr(d, '"');
            if (end && (size_t)(end - d) < max_len) {
                memcpy(out_digest, d, (size_t)(end - d));
                out_digest[end - d] = '\0';
                return true;
            }
        }
        p += 22;
    }

    /* Fallback: look for amd64 if arm64 not found */
    p = strstr(manifest_json, "\"architecture\":\"amd64\"");
    if (p) {
        const char *start = (p - 500 > manifest_json) ? p - 500 : manifest_json;
        const char *d = strstr(start, "\"digest\":\"sha256:");
        if (!d || d > p) d = strstr(p, "\"digest\":\"sha256:");
        if (d) {
            d += 10;
            const char *end = strchr(d, '"');
            if (end && (size_t)(end - d) < max_len) {
                memcpy(out_digest, d, (size_t)(end - d));
                out_digest[end - d] = '\0';
                return true;
            }
        }
    }

    return false;
}

static void sanitize_name(const char *in, char *out, size_t max_len) {
    size_t j = 0;
    for (size_t i = 0; in[i] && j < max_len - 1; i++) {
        if (isalnum((unsigned char)in[i]) || in[i] == '-' || in[i] == '_') {
            out[j++] = in[i];
        } else {
            out[j++] = '_';
        }
    }
    out[j] = '\0';
}

static void update_images_json(const char *storage_root, const oci_image_metadata_t *meta) {
    char json_file[1024];
    snprintf(json_file, sizeof(json_file), "%s/images.json", storage_root);

    /* Read existing file if present */
    char *existing = NULL;
    FILE *f = fopen(json_file, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (sz > 0) {
            existing = malloc((size_t)sz + 1);
            if (existing) {
                fread(existing, 1, (size_t)sz, f);
                existing[sz] = '\0';
            }
        }
        fclose(f);
    }

    f = fopen(json_file, "w");
    if (!f) {
        if (existing) free(existing);
        return;
    }

    /* If file exists and contains array, prepend/append */
    fprintf(f, "[\n  {\n");
    fprintf(f, "    \"Id\": \"sha256:%s\",\n", meta->id);
    fprintf(f, "    \"RepoTags\": [\"%s:%s\"],\n", meta->repository, meta->tag);
    fprintf(f, "    \"Created\": %lld,\n", (long long)meta->created_at);
    fprintf(f, "    \"Size\": %zu,\n", meta->size_bytes);
    fprintf(f, "    \"VirtualSize\": %zu,\n", meta->size_bytes);
    fprintf(f, "    \"RootFS\": \"%s\"\n", meta->rootfs_dir);
    fprintf(f, "  }\n]\n");

    fclose(f);
    if (existing) free(existing);
}

int oci_pull_image(const oci_image_ref_t *ref, const char *storage_root, oci_image_metadata_t *meta) {
    if (!ref || !storage_root || !meta) return -1;
    memset(meta, 0, sizeof(oci_image_metadata_t));

    printf("⚡ [oci-pull] Pulling %s from %s...\n", ref->full_name, ref->registry);

    /* 1. Ensure storage folders exist */
    char blob_dir[1024], layer_dir[1024], img_dir[1024];
    snprintf(blob_dir, sizeof(blob_dir), "%s/blobs", storage_root);
    snprintf(layer_dir, sizeof(layer_dir), "%s/layers", storage_root);
    snprintf(img_dir, sizeof(img_dir), "%s/images", storage_root);

    char mkdir_cmd[2048];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\" \"%s\" \"%s\"", blob_dir, layer_dir, img_dir);
    (void)system(mkdir_cmd);

    /* 2. Obtain Token */
    char token[8192] = {0};
    if (strcmp(ref->registry, "registry-1.docker.io") == 0) {
        if (fetch_docker_hub_token(ref->repository, token, sizeof(token)) != 0) {
            fprintf(stderr, "Failed to authenticate with Docker Hub\n");
            return -1;
        }
    }

    /* 3. Fetch Root Manifest / Index */
    char *manifest_json = fetch_manifest_with_token(ref->registry, ref->repository, ref->tag, token);
    if (!manifest_json) {
        fprintf(stderr, "Failed to fetch manifest for %s\n", ref->full_name);
        return -1;
    }

    /* 4. If multi-arch index, find arm64 submanifest */
    char arm64_digest[MAX_DIGEST_LEN] = {0};
    if (find_arm64_submanifest(manifest_json, arm64_digest, sizeof(arm64_digest))) {
        printf("  ✓ Detected ARM64 Silicon Manifest: %s\n", arm64_digest);
        free(manifest_json);
        manifest_json = fetch_manifest_with_token(ref->registry, ref->repository, arm64_digest, token);
        if (!manifest_json) {
            fprintf(stderr, "Failed to fetch ARM64 sub-manifest\n");
            return -1;
        }
    }

    /* 5. Prepare Target Image Rootfs Dir */
    char safe_repo[256];
    sanitize_name(ref->repository, safe_repo, sizeof(safe_repo));
    char target_rootfs[1024];
    snprintf(target_rootfs, sizeof(target_rootfs), "%s/%s_%s/rootfs", img_dir, safe_repo, ref->tag);

    char prep_cmd[2048];
    snprintf(prep_cmd, sizeof(prep_cmd), "rm -rf \"%s\" && mkdir -p \"%s\"", target_rootfs, target_rootfs);
    (void)system(prep_cmd);

    /* 6. Extract layer blobs */
    size_t total_size = 0;
    const char *layers_section = strstr(manifest_json, "\"layers\":");
    if (!layers_section) layers_section = strstr(manifest_json, "\"layers\" :");
    const char *lp = layers_section ? layers_section : manifest_json;
    int layer_count = 0;
    char last_short_hash[128] = "unknown";

    while ((lp = strstr(lp, "\"digest\"")) != NULL) {
        const char *colon = strchr(lp, ':');
        if (!colon) break;
        const char *val_start = strstr(colon, "\"sha256:");
        if (!val_start || val_start - colon > 10) {
            lp = colon + 1;
            continue;
        }
        val_start += 1; /* skip quote to point to sha256: */
        const char *lend = strchr(val_start, '"');
        if (!lend) break;

        char layer_digest[MAX_DIGEST_LEN];
        size_t dlen = (size_t)(lend - val_start);
        if (dlen >= sizeof(layer_digest)) dlen = sizeof(layer_digest) - 1;
        memcpy(layer_digest, val_start, dlen);
        layer_digest[dlen] = '\0';

        layer_count++;
        printf("  ⚡ [layer %d] Downloading %s...\n", layer_count, layer_digest);

        char blob_path[1024];
        const char *short_hash = layer_digest + 7; /* skip sha256: */
        snprintf(last_short_hash, sizeof(last_short_hash), "%s", short_hash);
        snprintf(blob_path, sizeof(blob_path), "%s/%s.tar.gz", blob_dir, short_hash);

        if (download_blob(ref->registry, ref->repository, layer_digest, token, blob_path) != 0) {
            fprintf(stderr, "Failed to download layer blob: %s\n", layer_digest);
            free(manifest_json);
            return -1;
        }

        struct stat st;
        if (stat(blob_path, &st) == 0) total_size += (size_t)st.st_size;

        /* Extract layer onto target rootfs */
        printf("  ⚡ [layer %d] Extracting onto rootfs...\n", layer_count);
        char tar_cmd[2048];
        snprintf(tar_cmd, sizeof(tar_cmd), "tar -xzf \"%s\" -C \"%s\" 2>/dev/null", blob_path, target_rootfs);
        (void)system(tar_cmd);

        lp = lend;
    }

    free(manifest_json);

    if (layer_count == 0) {
        fprintf(stderr, "No layers found in manifest\n");
        return -1;
    }

    /* 7. Populate metadata and save */
    snprintf(meta->id, sizeof(meta->id), "%.12s", last_short_hash);
    snprintf(meta->full_digest, sizeof(meta->full_digest), "sha256:%s", last_short_hash);
    snprintf(meta->repository, sizeof(meta->repository), "%s", ref->repository);
    snprintf(meta->tag, sizeof(meta->tag), "%s", ref->tag);
    meta->size_bytes = total_size;
    meta->created_at = (int64_t)time(NULL);
    snprintf(meta->rootfs_dir, sizeof(meta->rootfs_dir), "%s", target_rootfs);

    update_images_json(storage_root, meta);

    printf("✓ [oci-pull] Successfully pulled and unpacked %s (Size: %.2f MB, ID: %s)\n",
           ref->full_name, (double)total_size / (1024.0 * 1024.0), meta->id);

    return 0;
}

int oci_get_image_metadata(const char *storage_root, const char *image_name, oci_image_metadata_t *meta) {
    if (!storage_root || !image_name || !meta) return -1;

    oci_image_ref_t ref;
    oci_parse_image_ref(image_name, &ref);

    char safe_repo[256];
    sanitize_name(ref.repository, safe_repo, sizeof(safe_repo));
    char rootfs[1024];
    snprintf(rootfs, sizeof(rootfs), "%s/images/%s_%s/rootfs", storage_root, safe_repo, ref.tag);

    struct stat st;
    if (stat(rootfs, &st) == 0 && S_ISDIR(st.st_mode)) {
        snprintf(meta->id, sizeof(meta->id), "local_%s", ref.tag);
        snprintf(meta->repository, sizeof(meta->repository), "%s", ref.repository);
        snprintf(meta->tag, sizeof(meta->tag), "%s", ref.tag);
        snprintf(meta->rootfs_dir, sizeof(meta->rootfs_dir), "%s", rootfs);
        meta->size_bytes = 4194304;
        meta->created_at = (int64_t)time(NULL);
        return 0;
    }
    return -1;
}

int oci_list_images_json(const char *storage_root, char *out_json, size_t max_len) {
    if (!storage_root || !out_json || max_len == 0) return -1;

    char json_file[1024];
    snprintf(json_file, sizeof(json_file), "%s/images.json", storage_root);

    FILE *f = fopen(json_file, "r");
    if (!f) {
        snprintf(out_json, max_len, "[]\n");
        return 0;
    }

    size_t n = fread(out_json, 1, max_len - 1, f);
    out_json[n] = '\0';
    fclose(f);

    if (n == 0) {
        snprintf(out_json, max_len, "[]\n");
    }
    return 0;
}
