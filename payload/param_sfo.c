#include "param_sfo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Read entire file into a heap buffer; caller must free(). Returns NULL on error. */
static uint8_t *read_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz <= 0)                     { fclose(f); return NULL; }
    rewind(f);

    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf)                        { fclose(f); return NULL; }

    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    *out_size = (size_t)sz;
    return buf;
}

int sfo_read_field(const char *sfo_path, const char *key,
                   char *out_buf, size_t out_len) {
    size_t file_size = 0;
    uint8_t *data = read_file(sfo_path, &file_size);
    if (!data) {
        fprintf(stderr, "[sfo] Cannot open %s: %s\n", sfo_path, strerror(errno));
        return -1;
    }

    int ret = -1;

    /* Validate header */
    if (file_size < sizeof(sfo_header_t)) goto done;
    const sfo_header_t *hdr = (const sfo_header_t *)data;
    if (hdr->magic != SFO_MAGIC) {
        fprintf(stderr, "[sfo] Bad magic in %s (got 0x%08x)\n", sfo_path, hdr->magic);
        goto done;
    }

    const uint32_t n         = hdr->entry_count;
    const uint32_t key_base  = hdr->key_table_offset;
    const uint32_t data_base = hdr->data_table_offset;
    const sfo_index_entry_t *idx =
        (const sfo_index_entry_t *)(data + sizeof(sfo_header_t));

    for (uint32_t i = 0; i < n; i++) {
        /* bounds check index entry */
        size_t idx_off = sizeof(sfo_header_t) + i * sizeof(sfo_index_entry_t);
        if (idx_off + sizeof(sfo_index_entry_t) > file_size) break;

        const sfo_index_entry_t *e = &idx[i];

        /* read key (null-terminated in key table) */
        uint32_t koff = key_base + e->key_offset;
        if (koff >= file_size) continue;
        const char *k = (const char *)(data + koff);

        if (strcmp(k, key) != 0) continue;

        /* key matched — read value */
        uint32_t doff = data_base + e->data_offset;
        if (doff + e->data_len > file_size) break;

        size_t copy = e->data_len < out_len ? e->data_len : out_len - 1;
        memcpy(out_buf, data + doff, copy);
        out_buf[copy] = '\0';
        ret = 0;
        break;
    }

    if (ret != 0)
        fprintf(stderr, "[sfo] Key '%s' not found in %s\n", key, sfo_path);

done:
    free(data);
    return ret;
}

int sfo_get_content_id(const char *sfo_path, char *out_buf, size_t out_len) {
    return sfo_read_field(sfo_path, "CONTENT_ID", out_buf, out_len);
}

int sfo_get_content_id_from_dump(const char *dump_dir,
                                 char *out_buf, size_t out_len) {
    char sfo_path[1024];
    snprintf(sfo_path, sizeof(sfo_path), "%s/sce_sys/param.sfo", dump_dir);
    return sfo_get_content_id(sfo_path, out_buf, out_len);
}
