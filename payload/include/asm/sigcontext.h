/* asm/sigcontext.h — x86_64 shim for cross-compilation on Termux ARM64. */
#ifndef _ASM_SIGCONTEXT_H_STUB
#define _ASM_SIGCONTEXT_H_STUB

#include <asm/types.h>

/* Minimal x86_64 signal context — only the fields signal.h needs */
struct sigcontext {
    __u64 r8, r9, r10, r11, r12, r13, r14, r15;
    __u64 rdi, rsi, rbp, rbx, rdx, rax, rcx, rsp;
    __u64 rip;
    __u64 eflags;
    __u16 cs, gs, fs, ss;
    __u64 err, trapno, oldmask, cr2;
    /* FPU state pointer */
    __u64 fpstate;
    __u64 reserved[8];
};

#endif /* _ASM_SIGCONTEXT_H_STUB */
