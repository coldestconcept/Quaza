#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pkg_types.h"
#include "pkg_header.h"
#include "pfs_builder.h"
#include "npdrm.h"

extern uint64_t align_up(uint64_t value, uint64_t alignment);

int pkg_header_init(pkg_context_t* ctx, pkg_project_t* project) {
    if (!ctx || !project) return PKG_ERROR_INVALID_PARAM;
    
    memset(ctx, 0, sizeof(pkg_context_t));
    
    // Initialize header
    pkg_header_t* hdr = &ctx->header;
    hdr->magic = PKG_MAGIC;
    hdr->type = project->type;
    hdr->header_size = PKG_HEADER_SIZE;
    hdr->header_size2 = PKG_HEADER_SIZE;
    hdr->entry_table_offset = PKG_HEADER_SIZE;
    
    // Calculate entry table size (we'll have several entries)
    ctx->entry_table.entry_count = 10; // Approximate
    ctx->entry_table.entries = calloc(ctx->entry_table.entry_count, 
                                      sizeof(pkg_entry_t));
    
    return PKG_SUCCESS;
}

int pkg_add_entry(pkg_context_t* ctx, uint32_t id, uint32_t size, 
                  const void* data, uint32_t* offset_out) {
    if (!ctx || !ctx->entry_table.entries) return PKG_ERROR_INVALID_PARAM;
    
    // Find free entry slot
    for (uint32_t i = 0; i < ctx->entry_table.entry_count; i++) {
        if (ctx->entry_table.entries[i].id == 0) {
            ctx->entry_table.entries[i].id = id;
            ctx->entry_table.entries[i].size = size;
            ctx->entry_table.entries[i].flags = 0;
            
            // Calculate offset
            if (offset_out) {
                *offset_out = ctx->entry_data_size;
            }
            
            // Store entry data
            if (data && size > 0) {
                ctx->entry_data = realloc(ctx->entry_data, 
                                          ctx->entry_data_size + size);
                memcpy(ctx->entry_data + ctx->entry_data_size, data, size);
                ctx->entry_data_size += size;
            }
            
            ctx->header.entry_count++;
            return PKG_SUCCESS;
        }
    }
    
    return PKG_ERROR_MEMORY;
}

int pkg_header_finalize(pkg_context_t* ctx) {
    if (!ctx) return PKG_ERROR_INVALID_PARAM;
    
    // Calculate body offset
    uint32_t entry_table_size = ctx->header.entry_count * sizeof(pkg_entry_t);
    ctx->header.body_offset = align_up(PKG_HEADER_SIZE + entry_table_size, 16);
    ctx->header.entry_table_size = entry_table_size;
    ctx->header.entry_count2 = ctx->header.entry_count;
    
    return PKG_SUCCESS;
}

int pkg_write_header(FILE* fp, pkg_context_t* ctx) {
    if (!fp || !ctx) return PKG_ERROR_INVALID_PARAM;
    
    // Write header
    fwrite(&ctx->header, sizeof(pkg_header_t), 1, fp);
    
    // Write entry table
    fseek(fp, ctx->header.entry_table_offset, SEEK_SET);
    fwrite(ctx->entry_table.entries, sizeof(pkg_entry_t), 
           ctx->header.entry_count, fp);
    
    return PKG_SUCCESS;
}

void pkg_context_free(pkg_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->entry_table.entries) {
        free(ctx->entry_table.entries);
    }
    if (ctx->entry_data) {
        free(ctx->entry_data);
    }
}

// Main PKG build function
int pkg_build(pkg_project_t* project, const char* input_dir, 
              const char* output_path) {
    if (!project || !input_dir || !output_path) {
        return PKG_ERROR_INVALID_PARAM;
    }
    
    printf("[PKG] Building package...\n");
    printf("[PKG] Content ID: %s\n", project->content_id);
    printf("[PKG] Input: %s\n", input_dir);
    printf("[PKG] Output: %s\n", output_path);
    
    // Create PFS image
    printf("[PKG] Creating PFS image...\n");
    pfs_image_t* pfs = pfs_image_create(project->pfs_block_size);
    if (!pfs) {
        fprintf(stderr, "[PKG] Failed to create PFS image\n");
        return PKG_ERROR_MEMORY;
    }
    
    // Scan input directory
    if (pfs_scan_directory(input_dir, pfs->root) < 0) {
        fprintf(stderr, "[PKG] Failed to scan directory\n");
        pfs_image_free(pfs);
        return PKG_ERROR_IO;
    }
    
    // Build PFS structures
    if (pfs_build_image(pfs) < 0) {
        fprintf(stderr, "[PKG] Failed to build PFS\n");
        pfs_image_free(pfs);
        return PKG_ERROR_INVALID_FORMAT;
    }
    
    uint64_t pfs_size = pfs_calculate_size(pfs);
    printf("[PKG] PFS size: %lu bytes\n", pfs_size);
    
    // Create PKG context
    pkg_context_t ctx;
    if (pkg_header_init(&ctx, project) != PKG_SUCCESS) {
        pfs_image_free(pfs);
        return PKG_ERROR_MEMORY;
    }
    
    // Add NPDRM entry
    npdrm_header_t npdrm;
    npdrm_generate_header(project->content_id, project->passcode, &npdrm);
    pkg_add_entry(&ctx, PKG_ENTRY_ID_NPDRM, sizeof(npdrm), &npdrm, NULL);
    
    // Add PFS entries
    uint32_t pfs_flags = 0;
    pkg_add_entry(&ctx, PKG_ENTRY_ID_PFS_FLAGS, sizeof(pfs_flags), 
                  &pfs_flags, NULL);
    pkg_add_entry(&ctx, PKG_ENTRY_ID_PFS_BLOCK_SIZE, 
                  sizeof(pfs->block_size), &pfs->block_size, NULL);
    pkg_add_entry(&ctx, PKG_ENTRY_ID_PFS_IMAGE_SIZE, sizeof(pfs_size), 
                  &pfs_size, NULL);
    
    // Finalize header
    pkg_header_finalize(&ctx);
    
    // Open output file
    FILE* fp = fopen(output_path, "wb");
    if (!fp) {
        perror("[PKG] fopen");
        pkg_context_free(&ctx);
        pfs_image_free(pfs);
        return PKG_ERROR_IO;
    }
    
    // Write header
    pkg_write_header(fp, &ctx);
    
    // Write body offset padding
    fseek(fp, ctx.header.body_offset, SEEK_SET);
    
    // Write PFS image
    printf("[PKG] Writing PFS image...\n");
    if (pfs_write_image(pfs, fp) < 0) {
        fprintf(stderr, "[PKG] Failed to write PFS\n");
        fclose(fp);
        pkg_context_free(&ctx);
        pfs_image_free(pfs);
        return PKG_ERROR_IO;
    }
    
    // Update and rewrite header with final size
    uint64_t final_size = ftell(fp);
    ctx.header.pkg_size = final_size;
    ctx.header.data_size = final_size - PKG_HEADER_SIZE;
    
    fseek(fp, 0, SEEK_SET);
    pkg_write_header(fp, &ctx);
    
    fclose(fp);
    
    printf("[PKG] Package created: %s (%lu bytes)\n", output_path, final_size);
    
    // Cleanup
    pkg_context_free(&ctx);
    pfs_image_free(pfs);
    
    return PKG_SUCCESS;
}
