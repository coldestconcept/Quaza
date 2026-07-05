/* asm-generic/int-ll64.h — shim for x86_64 cross-compilation on Termux ARM64.
 * Provides the __intN_t typedefs that linux/types.h may need.            */
#ifndef _ASM_GENERIC_INT_LL64_H
#define _ASM_GENERIC_INT_LL64_H

#ifndef __ASSEMBLY__

typedef signed   char      __s8;
typedef unsigned char      __u8;
typedef signed   short     __s16;
typedef unsigned short     __u16;
typedef signed   int       __s32;
typedef unsigned int       __u32;
typedef signed   long long __s64;
typedef unsigned long long __u64;

#endif /* __ASSEMBLY__ */

#endif /* _ASM_GENERIC_INT_LL64_H */
