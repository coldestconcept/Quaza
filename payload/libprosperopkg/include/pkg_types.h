#ifndef PKG_TYPES_H
#define PKG_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define PKG_MAGIC 0x7F434E54
#define PKG_VERSION 2
#define PKG_HEADER_SIZE 0x4000

// PKG Types
typedef enum {
    PKG_TYPE_PS4_GD = 0x1,
    PKG_TYPE_PS4_AC = 0x2,
    PKG_TYPE_PS5_GD = 0x10,
    PKG_TYPE_PS5_AC = 0x11,
    PKG_TYPE_PS5_PATCH = 0x12,
} pkg_type_t;

// Entry IDs
#define PKG_ENTRY_ID_DIGESTS         0x0001
#define PKG_ENTRY_ID_ENTRY_KEYS      0x0010
#define PKG_ENTRY_ID_IMAGE_KEY       0x0020
#define PKG_ENTRY_ID_GENERAL_DIGESTS 0x0080
#define PKG_ENTRY_ID_METAS           0x0100
#define PKG_ENTRY_ID_ENTRIES         0x0200
#define PKG_ENTRY_ID_ENTRY_NAMES     0x0400
#define PKG_ENTRY_ID_NPDRM           0x4000
#define PKG_ENTRY_ID_PFS_IMAGE       0x4200
#define PKG_ENTRY_ID_PFS_SIGNED      0x4201
#define PKG_ENTRY_ID_PFS_OFFSET      0x4202
#define PKG_ENTRY_ID_PFS_IMAGE_SIZE  0x4203
#define PKG_ENTRY_ID_PFS_FLAGS       0x4204
#define PKG_ENTRY_ID_PFS_HDR_DIGEST  0x4210
#define PKG_ENTRY_ID_PFS_HDR_SIZE    0x4211
#define PKG_ENTRY_ID_PFS_BLOCK_SIZE  0x4220
#define PKG_ENTRY_ID_PFS_DATA_OFFSET 0x4230
#define PKG_ENTRY_ID_PFS_DATA_SIZE   0x4231

typedef struct {
    uint32_t id;
    uint32_t offset;
    uint32_t size;
    uint32_t flags;
} pkg_entry_t;

typedef struct {
    uint32_t entry_count;
    pkg_entry_t* entries;
} pkg_entry_table_t;

typedef struct {
    char content_id[37];
    char passcode[33];
    uint32_t type;
    uint64_t package_size;
    
    // Paths
    char app_path[1024];
    char playgo_path[1024];
    char sce_sys_path[1024];
    
    // PFS settings
    uint64_t pfs_image_size;
    uint32_t pfs_block_size;
    
    // Flags
    bool use_encryption;
    bool is_patch;
} pkg_project_t;

// Result codes
typedef enum {
    PKG_SUCCESS = 0,
    PKG_ERROR_INVALID_PARAM = -1,
    PKG_ERROR_IO = -2,
    PKG_ERROR_MEMORY = -3,
    PKG_ERROR_INVALID_FORMAT = -4,
    PKG_ERROR_NOT_FOUND = -5,
} pkg_result_t;

#endif
