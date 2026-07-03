#ifndef PARAM_SFO_H
#define PARAM_SFO_H

#include <stdint.h>
#include <stddef.h>

/*
 * param.sfo binary format
 * Used by PS4/PS5 to store game metadata (title, content ID, version, etc.)
 */

#define SFO_MAGIC         0x46535000u   /* "\x00PSF" little-endian */
#define SFO_VERSION       0x00000101u
#define SFO_FMT_UTF8_S    0x0004        /* special UTF-8 (null-padded fixed length) */
#define SFO_FMT_UTF8      0x0204        /* standard UTF-8 string */
#define SFO_FMT_INT32     0x0404        /* 32-bit integer */

typedef struct __attribute__((packed)) {
    uint32_t magic;               /* SFO_MAGIC */
    uint32_t version;             /* SFO_VERSION */
    uint32_t key_table_offset;    /* byte offset to key table */
    uint32_t data_table_offset;   /* byte offset to data table */
    uint32_t entry_count;         /* number of index entries */
} sfo_header_t;

typedef struct __attribute__((packed)) {
    uint16_t key_offset;          /* offset into key table (from key_table_offset) */
    uint16_t data_fmt;            /* SFO_FMT_* */
    uint32_t data_len;            /* actual byte length of value */
    uint32_t data_max_len;        /* allocated byte length in data table */
    uint32_t data_offset;         /* offset into data table (from data_table_offset) */
} sfo_index_entry_t;

/*
 * sfo_read_field
 * Read a named field from a param.sfo file.
 * Returns 0 on success, -1 on error.
 * out_buf must be at least out_len bytes.
 */
int sfo_read_field(const char *sfo_path, const char *key,
                   char *out_buf, size_t out_len);

/*
 * sfo_get_content_id
 * Shorthand: reads the CONTENT_ID field from sfo_path.
 * out_buf should be >= 64 bytes.
 */
int sfo_get_content_id(const char *sfo_path,
                       char *out_buf, size_t out_len);

/*
 * sfo_get_content_id_from_dump
 * Given a game dump root directory, locates sce_sys/param.sfo automatically
 * and extracts CONTENT_ID.
 * Returns 0 on success, -1 on error.
 */
int sfo_get_content_id_from_dump(const char *dump_dir,
                                 char *out_buf, size_t out_len);

#endif /* PARAM_SFO_H */
