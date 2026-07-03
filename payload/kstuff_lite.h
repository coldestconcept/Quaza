#ifndef KSTUFF_LITE_H
#define KSTUFF_LITE_H

/*
 * kstuff-lite interface
 * Source: https://github.com/sleirsgoevy/ps5-kstuff
 *
 * Provides kernel read/write and privileged syscall execution needed to
 * write to restricted paths (/user/app/) and call internal PS5 APIs
 * that are normally blocked from user-land.
 *
 * Link with -lkstuff (provided by the PS5 Payload SDK kstuff-lite package).
 */

#include <stdint.h>
#include <stddef.h>

/*
 * kstuff_init — initialise kstuff-lite.
 * Must be called before any other kstuff function.
 * Returns 0 on success, negative on failure.
 */
int kstuff_init(void);

/*
 * kread — read `len` bytes from kernel virtual address `addr` into `buf`.
 */
void kread(uint64_t addr, void *buf, size_t len);

/*
 * kwrite — write `len` bytes from `buf` to kernel virtual address `addr`.
 */
void kwrite(uint64_t addr, const void *buf, size_t len);

/*
 * ksys — execute kernel syscall `nr` with up to 6 arguments.
 * Returns the syscall return value.
 */
uint64_t ksys(int nr, uint64_t a1, uint64_t a2, uint64_t a3,
              uint64_t a4, uint64_t a5, uint64_t a6);

/*
 * kexec — execute `func(arg)` in kernel context.
 * Use sparingly; prefer ksys where a syscall number exists.
 */
uint64_t kexec(void *func, void *arg);

#endif /* KSTUFF_LITE_H */
