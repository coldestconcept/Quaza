/* asm/byteorder.h — x86_64 shim for cross-compilation on Termux ARM64. */
#ifndef _ASM_BYTEORDER_H_STUB
#define _ASM_BYTEORDER_H_STUB

/* x86_64 is little-endian */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef __BYTE_ORDER
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif

#endif /* _ASM_BYTEORDER_H_STUB */
