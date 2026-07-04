#ifndef PKG_HEADER_H
#define PKG_HEADER_H

#include "pkg_types.h"

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t type;
    uint32_t header_size;
    uint32_t unknown_0x0C;
    uint64_t header_size2;
    uint64_t data_size;
    uint64_t pkg_size;
    uint64_t unknown_0x28;
    uint64_t unknown_0x30;
    uint16_t unknown_0x38;
    uint16_t unknown_0x3A;
    uint32_t unknown_0x3C;
    uint32_t unknown_0x40;
    uint32_t entry_count;
    uint16_t entry_count2;
    uint16_t entry_table_size;
    uint32_t unknown_0x4C;
    uint64_t entry_table_offset;
    uint64_t body_offset;
    uint8_t  reserved[0x3F00];
} pkg_header_t;

typedef struct {
    pkg_header_t header;
    pkg_entry_table_t entry_table;
    uint8_t* entry_data;
    size_t entry_data_size;
} pkg_context_t;

int pkg_header_init(pkg_context_t* ctx, pkg_project_t* project);
int pkg_header_finalize(pkg_context_t* ctx);
int pkg_write_header(FILE* fp, pkg_context_t* ctx);
void pkg_context_free(pkg_context_t* ctx);

/* Top-level entry point implemented in pkg_builder.c — declared here (rather
 * than left as an implicit/undeclared call) so callers like pkg_server.c
 * get a real prototype and the compiler can catch signature mismatches. */
int pkg_build(pkg_project_t* project, const char* input_dir, const char* output_path);

#endif
