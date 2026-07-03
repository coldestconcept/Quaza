#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pfs_types.h"
#include "pfs_builder.h"

extern void write_u64_le(uint8_t* buf, uint64_t val);
extern void write_u32_le(uint8_t* buf, uint32_t val);
extern uint64_t align_up(uint64_t value, uint64_t alignment);

pfs_image_t* pfs_image_create(uint64_t block_size) {
    pfs_image_t* image = calloc(1, sizeof(pfs_image_t));
    if (!image) return NULL;
    
    image->block_size = block_size ? block_size : PFS_BLOCK_SIZE;
    image->root = pfs_inode_create("", PFS_INODE_TYPE_DIR, NULL);
    image->root->ino = 1; // Root is always 1
    
    // Initialize header
    image->header.version = PFS_VERSION;
    image->header.magic = PFS_MAGIC;
    image->header.id = (uint64_t)time(NULL);
    image->header.fmode = 0x0000000000000001ULL;
    
    return image;
}

void pfs_image_free(pfs_image_t* image) {
    if (!image) return;
    
    if (image->root) {
        pfs_inode_free(image->root);
    }
    if (image->inode_table) free(image->inode_table);
    if (image->block_bitmap) free(image->block_bitmap);
    if (image->data_blocks) free(image->data_blocks);
    
    free(image);
}

static uint64_t count_inodes(pfs_inode_t* node) {
    if (!node) return 0;
    
    uint64_t count = 1;
    for (int i = 0; i < node->child_count; i++) {
        count += count_inodes(node->children[i]);
    }
    return count;
}

static uint64_t count_blocks(pfs_inode_t* node, uint64_t block_size) {
    if (!node) return 0;
    
    uint64_t blocks = 0;
    if (node->type == PFS_INODE_TYPE_FILE && node->size > 0) {
        blocks = align_up(node->size, block_size) / block_size;
    }
    
    for (int i = 0; i < node->child_count; i++) {
        blocks += count_blocks(node->children[i], block_size);
    }
    return blocks;
}

static void assign_offsets(pfs_inode_t* node, uint64_t* current_offset, uint64_t block_size) {
    if (!node) return;
    
    if (node->type == PFS_INODE_TYPE_FILE && node->size > 0) {
        node->offset = *current_offset;
        *current_offset += align_up(node->size, block_size);
    }
    
    for (int i = 0; i < node->child_count; i++) {
        assign_offsets(node->children[i], current_offset, block_size);
    }
}

int pfs_build_image(pfs_image_t* image) {
    if (!image || !image->root) return -1;
    
    // Count inodes and blocks
    image->inode_count = count_inodes(image->root);
    image->block_count = count_blocks(image->root, image->block_size);
    
    // Calculate data offset (after header + inode table + bitmap)
    uint64_t inode_table_size = image->inode_count * 256; // Each inode is 256 bytes
    uint64_t bitmap_size = align_up(image->block_count / 8, image->block_size);
    
    image->data_offset = sizeof(pfs_header_t) + inode_table_size + bitmap_size;
    image->data_offset = align_up(image->data_offset, image->block_size);
    
    // Assign data offsets to files
    uint64_t current_offset = image->data_offset;
    assign_offsets(image->root, &current_offset, image->block_size);
    
    // Allocate data blocks
    uint64_t data_size = current_offset - image->data_offset;
    image->data_blocks = calloc(1, data_size);
    if (!image->data_blocks) return -1;
    
    // Copy file data to blocks
    // (This would be done during write)
    
    return 0;
}

uint64_t pfs_calculate_size(pfs_image_t* image) {
    if (!image) return 0;
    
    uint64_t inode_table_size = image->inode_count * 256;
    uint64_t bitmap_size = align_up(image->block_count / 8, image->block_size);
    uint64_t data_size = count_blocks(image->root, image->block_size) * image->block_size;
    
    return sizeof(pfs_header_t) + inode_table_size + bitmap_size + data_size;
}

static void write_inode(FILE* fp, pfs_inode_t* inode, uint64_t block_size) {
    uint8_t buf[256] = {0};
    
    write_u32_le(buf + 0, inode->mode);
    write_u32_le(buf + 4, inode->nlink);
    write_u32_le(buf + 8, inode->flags);
    write_u64_le(buf + 16, inode->size);
    write_u64_le(buf + 24, inode->size_compressed);
    write_u64_le(buf + 32, inode->offset);
    write_u64_le(buf + 40, inode->ino);
    
    // Write name (up to 32 bytes)
    if (inode->name) {
        strncpy((char*)buf + 48, inode->name, 32);
    }
    
    fwrite(buf, 1, 256, fp);
}

static void write_inodes_recursive(FILE* fp, pfs_inode_t* node, uint64_t block_size) {
    if (!node) return;
    
    write_inode(fp, node, block_size);
    
    for (int i = 0; i < node->child_count; i++) {
        write_inodes_recursive(fp, node->children[i], block_size);
    }
}

static void write_file_data(FILE* fp, pfs_inode_t* node, uint64_t block_size) {
    if (!node) return;
    
    if (node->type == PFS_INODE_TYPE_FILE && node->size > 0 && node->data) {
        fseek(fp, node->offset, SEEK_SET);
        fwrite(node->data, 1, node->size, fp);
        
        // Pad to block boundary
        uint64_t padded = align_up(node->size, block_size);
        uint8_t zero = 0;
        for (uint64_t i = node->size; i < padded; i++) {
            fwrite(&zero, 1, 1, fp);
        }
    }
    
    for (int i = 0; i < node->child_count; i++) {
        write_file_data(fp, node->children[i], block_size);
    }
}

int pfs_write_image(pfs_image_t* image, FILE* fp) {
    if (!image || !fp) return -1;
    
    // Write header
    fwrite(&image->header, sizeof(pfs_header_t), 1, fp);
    
    // Write inode table
    write_inodes_recursive(fp, image->root, image->block_size);
    
    // Write block bitmap (simplified - all blocks used)
    uint64_t inode_table_size = image->inode_count * 256;
    uint64_t bitmap_offset = sizeof(pfs_header_t) + inode_table_size;
    uint64_t bitmap_size = align_up(image->block_count / 8, image->block_size);
    
    fseek(fp, bitmap_offset, SEEK_SET);
    uint8_t* bitmap = calloc(1, bitmap_size);
    if (bitmap) {
        // Mark all blocks as used
        memset(bitmap, 0xFF, (image->block_count + 7) / 8);
        fwrite(bitmap, 1, bitmap_size, fp);
        free(bitmap);
    }
    
    // Write file data
    fseek(fp, image->data_offset, SEEK_SET);
    write_file_data(fp, image->root, image->block_size);
    
    return 0;
}
