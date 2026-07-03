#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint64_t align_up(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

uint64_t align_down(uint64_t value, uint64_t alignment) {
    return value & ~(alignment - 1);
}

void write_u64_le(uint8_t* buf, uint64_t val) {
    buf[0] = (val >> 0)  & 0xFF;
    buf[1] = (val >> 8)  & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
    buf[4] = (val >> 32) & 0xFF;
    buf[5] = (val >> 40) & 0xFF;
    buf[6] = (val >> 48) & 0xFF;
    buf[7] = (val >> 56) & 0xFF;
}

void write_u32_le(uint8_t* buf, uint32_t val) {
    buf[0] = (val >> 0)  & 0xFF;
    buf[1] = (val >> 8)  & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

uint64_t read_u64_le(const uint8_t* buf) {
    return ((uint64_t)buf[0] << 0)  |
           ((uint64_t)buf[1] << 8)  |
           ((uint64_t)buf[2] << 16) |
           ((uint64_t)buf[3] << 24) |
           ((uint64_t)buf[4] << 32) |
           ((uint64_t)buf[5] << 40) |
           ((uint64_t)buf[6] << 48) |
           ((uint64_t)buf[7] << 56);
}

uint32_t read_u32_le(const uint8_t* buf) {
    return ((uint32_t)buf[0] << 0)  |
           ((uint32_t)buf[1] << 8)  |
           ((uint32_t)buf[2] << 16) |
           ((uint32_t)buf[3] << 24);
}

int file_copy(const char* src, const char* dst) {
    FILE* in = fopen(src, "rb");
    if (!in) return -1;
    
    FILE* out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }
    
    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        fwrite(buffer, 1, n, out);
    }
    
    fclose(in);
    fclose(out);
    return 0;
}
