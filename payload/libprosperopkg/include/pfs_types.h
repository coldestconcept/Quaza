#ifndef PFS_TYPES_H
#define PFS_TYPES_H

#include <stdint.h>

#define PFS_MAGIC 0x20534650
#define PFS_VERSION 0x0001
#define PFS_BLOCK_SIZE 65536

// PFS Inode types
#define PFS_INODE_TYPE_DIR  0x01
#define PFS_INODE_TYPE_FILE 0x02

typedef struct __attribute__((packed)) {
    uint64_t version;
    uint64_t magic;
    uint64_t id;
    uint64_t fmode;
    uint64_t unknown_0x20;
    uint64_t unknown_0x28;
    uint64_t unknown_0x30;
    uint64_t unknown_0x38;
    uint64_t unknown_0x40;
    uint64_t unknown_0x48;
    uint64_t unknown_0x50;
    uint64_t unknown_0x58;
    uint64_t unknown_0x60;
    uint64_t unknown_0x68;
    uint64_t unknown_0x70;
    uint64_t unknown_0x78;
    uint64_t unknown_0x80;
    uint64_t unknown_0x88;
    uint64_t unknown_0x90;
    uint64_t unknown_0x98;
    uint64_t unknown_0xA0;
    uint64_t unknown_0xA8;
    uint64_t unknown_0xB0;
    uint64_t unknown_0xB8;
    uint64_t unknown_0xC0;
    uint64_t unknown_0xC8;
    uint64_t unknown_0xD0;
    uint64_t unknown_0xD8;
    uint64_t unknown_0xE0;
    uint64_t unknown_0xE8;
    uint64_t unknown_0xF0;
    uint64_t unknown_0xF8;
    uint64_t unknown_0x100;
    uint64_t unknown_0x108;
    uint64_t unknown_0x110;
    uint64_t unknown_0x118;
    uint64_t unknown_0x120;
    uint64_t unknown_0x128;
    uint64_t unknown_0x130;
    uint64_t unknown_0x138;
    uint64_t unknown_0x140;
    uint64_t unknown_0x148;
    uint64_t unknown_0x150;
    uint64_t unknown_0x158;
    uint64_t unknown_0x160;
    uint64_t unknown_0x168;
    uint64_t unknown_0x170;
    uint64_t unknown_0x178;
    uint64_t unknown_0x180;
    uint64_t unknown_0x188;
    uint64_t unknown_0x190;
    uint64_t unknown_0x198;
    uint64_t unknown_0x1A0;
    uint64_t unknown_0x1A8;
    uint64_t unknown_0x1B0;
    uint64_t unknown_0x1B8;
    uint64_t unknown_0x1C0;
    uint64_t unknown_0x1C8;
    uint64_t unknown_0x1D0;
    uint64_t unknown_0x1D8;
    uint64_t unknown_0x1E0;
    uint64_t unknown_0x1E8;
    uint64_t unknown_0x1F0;
    uint64_t unknown_0x1F8;
    uint64_t unknown_0x200;
    uint64_t unknown_0x208;
    uint64_t unknown_0x210;
    uint64_t unknown_0x218;
    uint64_t unknown_0x220;
    uint64_t unknown_0x228;
    uint64_t unknown_0x230;
    uint64_t unknown_0x238;
    uint64_t unknown_0x240;
    uint64_t unknown_0x248;
    uint64_t unknown_0x250;
    uint64_t unknown_0x258;
    uint64_t unknown_0x260;
    uint64_t unknown_0x268;
    uint64_t unknown_0x270;
    uint64_t unknown_0x278;
    uint64_t unknown_0x280;
    uint64_t unknown_0x288;
    uint64_t unknown_0x290;
    uint64_t unknown_0x298;
    uint64_t unknown_0x2A0;
    uint64_t unknown_0x2A8;
    uint64_t unknown_0x2B0;
    uint64_t unknown_0x2B8;
    uint64_t unknown_0x2C0;
    uint64_t unknown_0x2C8;
    uint64_t unknown_0x2D0;
    uint64_t unknown_0x2D8;
    uint64_t unknown_0x2E0;
    uint64_t unknown_0x2E8;
    uint64_t unknown_0x2F0;
    uint64_t unknown_0x2F8;
    uint64_t unknown_0x300;
    uint64_t unknown_0x308;
    uint64_t unknown_0x310;
    uint64_t unknown_0x318;
    uint64_t unknown_0x320;
    uint64_t unknown_0x328;
    uint64_t unknown_0x330;
    uint64_t unknown_0x338;
    uint64_t unknown_0x340;
    uint64_t unknown_0x348;
    uint64_t unknown_0x350;
    uint64_t unknown_0x358;
    uint64_t unknown_0x360;
    uint64_t unknown_0x368;
    uint64_t unknown_0x370;
    uint64_t unknown_0x378;
    uint64_t unknown_0x380;
    uint64_t unknown_0x388;
    uint64_t unknown_0x390;
    uint64_t unknown_0x398;
    uint64_t unknown_0x3A0;
    uint64_t unknown_0x3A8;
    uint64_t unknown_0x3B0;
    uint64_t unknown_0x3B8;
    uint64_t unknown_0x3C0;
    uint64_t unknown_0x3C8;
    uint64_t unknown_0x3D0;
    uint64_t unknown_0x3D8;
    uint64_t unknown_0x3E0;
    uint64_t unknown_0x3E8;
    uint64_t unknown_0x3F0;
    uint64_t unknown_0x3F8;
} pfs_header_t;

typedef struct pfs_inode {
    uint32_t mode;
    uint32_t nlink;
    uint32_t flags;
    uint64_t size;
    uint64_t size_compressed;
    uint64_t offset;
    uint64_t ino;
    char* name;
    void* data;
    struct pfs_inode* parent;
    struct pfs_inode** children;
    int child_count;
    int type;
} pfs_inode_t;

typedef struct {
    pfs_header_t header;
    pfs_inode_t* root;
    uint64_t block_size;
    uint64_t inode_count;
    uint64_t block_count;
    uint64_t data_offset;
    uint8_t* inode_table;
    uint8_t* block_bitmap;
    uint8_t* data_blocks;
} pfs_image_t;

#endif
