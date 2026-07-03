#ifndef PFS_BUILDER_H
#define PFS_BUILDER_H

#include "pfs_types.h"

pfs_image_t* pfs_image_create(uint64_t block_size);
void pfs_image_free(pfs_image_t* image);

pfs_inode_t* pfs_inode_create(const char* name, int type, pfs_inode_t* parent);
void pfs_inode_free(pfs_inode_t* inode);
void pfs_inode_add_child(pfs_inode_t* parent, pfs_inode_t* child);

int pfs_scan_directory(const char* path, pfs_inode_t* parent);
int pfs_build_image(pfs_image_t* image);
int pfs_write_image(pfs_image_t* image, FILE* fp);

uint64_t pfs_calculate_size(pfs_image_t* image);

#endif
