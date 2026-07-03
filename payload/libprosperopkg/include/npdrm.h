#ifndef NPDRM_H
#define NPDRM_H

#include <stdint.h>

#define NPDRM_MAGIC 0x4E504400

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t type;
    uint32_t flags;
    uint8_t  content_id[36];
    uint8_t  digest[16];
    uint8_t  inv_digest[16];
    uint8_t  xor_digest[16];
    uint8_t  unknown[16];
} npdrm_header_t;

int npdrm_generate_header(const char* content_id, const char* passcode, npdrm_header_t* header);
int npdrm_encrypt_data(void* data, size_t size, const uint8_t* key);

#endif
