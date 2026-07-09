/* Copyright (C) 2024 John Törnblom

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */

#include "payload.h"
#include "syscall.h"

#ifndef O_WRONLY
#define O_WRONLY  0x0001
#define O_CREAT   0x0200
#define O_TRUNC   0x0400
#endif


/**
 * Dependencies provided by the ELF linker.
 **/
extern void (*__init_array_start[])(payload_args_t*, int, char**, char**) __attribute__((weak));
extern void (*__init_array_end[])(payload_args_t*, int, char**, char**) __attribute__((weak));

extern void (*__fini_array_start[])(void) __attribute__((weak));
extern void (*__fini_array_end[])(void) __attribute__((weak));

extern unsigned char __bss_start[] __attribute__((weak));
extern unsigned char __bss_end[] __attribute__((weak));


/**
 * Entry point to the main program.
 **/
extern int main(int argc, char* argv[], char *envp[]);


int __klog_init(payload_args_t *args);
int __kernel_init(payload_args_t* args);
int __rtld_init(payload_args_t* args);
extern int rtld_null_syms; /* unresolved GOT slots after rtld_load */


/**
 * The PS5 does not allow syscalls from .text sections that are allocated in
 * shared memory. Instead, we simply assign the approriate registers, and
 * jump directly to a syscall instruction in libkernel (which is not in shared
 * memory).
 **/
static __attribute__ ((used)) long ptr_syscall = 0;
asm(".intel_syntax noprefix\n"
    ".global syscall\n"
    ".type syscall @function\n"
    "syscall:\n"
    "  mov rax, rdi\n"                      // sysno
    "  mov rdi, rsi\n"                      // arg1
    "  mov rsi, rdx\n"                      // arg2
    "  mov rdx, rcx\n"                      // arg3
    "  mov r10, r8\n"                       // arg4
    "  mov r8,  r9\n"                       // arg5
    "  mov r9,  qword ptr [rsp + 8]\n"      // arg6
    "  jmp qword ptr [rip + ptr_syscall]\n" // syscall
    "  ret\n"
    );


static payload_args_t* payload_args = 0;


payload_args_t*
payload_get_args(void) {
  return payload_args;
}


/* ── raw_udp_probe ───────────────────────────────────────────────────────────
 * Send a UDP diagnostic datagram using ONLY raw FreeBSD syscalls — no GOT,
 * no libc — so it works at any point after ptr_syscall is initialised, even
 * before rtld has resolved a single library symbol.
 *
 * Destination strategy (both attempted every call):
 *
 *   1. UNICAST to the pusher's IP  — obtained via getpeername(stdin), which
 *      works when elfldr has dup2'd its accepted TCP socket onto fd 0.
 *      This bypasses WiFi AP/client isolation that silently drops broadcasts.
 *
 *   2. BROADCAST to 255.255.255.255 — fallback for non-AP-isolated networks.
 *
 * push_to_ps5.py listens on 0.0.0.0:9020 and receives both variants.
 *
 * Message format: "PROBE sN\n"  (N = step digit 0-9)
 */
static void
raw_udp_probe(int step)
{
  /* FreeBSD constants (PS5 is FreeBSD-derived) */
  enum {
    _AF_INET      = 2,
    _SOCK_DGRAM   = 2,
    _IPPROTO_UDP  = 17,
    _SOL_SOCKET   = 0xffff,
    _SO_BROADCAST = 0x0020,
    _PORT         = 9020,
  };

  /* FreeBSD sockaddr_in: sin_len(1) sin_family(1) sin_port(2) sin_addr(4) pad(8) */
  struct _sa {
    unsigned char  sin_len;
    unsigned char  sin_family;
    unsigned short sin_port;
    unsigned int   sin_addr;
    char           sin_zero[8];
  };

  /* htons(9020): 0x233C → 0x3C23 */
  unsigned short net_port = (unsigned short)((_PORT >> 8) | ((_PORT & 0xff) << 8));

  /* ── 1. Try unicast: getpeername(stdin) to get pusher's IP ───────── */
  struct _sa peer = { 0 };
  unsigned int peerlen = 16;
  int has_unicast = 0;
  if (syscall(SYS_getpeername, (long)0, &peer, &peerlen) == 0 &&
      peer.sin_family == _AF_INET &&
      peer.sin_addr   != 0u &&
      peer.sin_addr   != 0xffffffffu) {
    peer.sin_len  = 16;
    peer.sin_port = net_port;
    has_unicast   = 1;
  }

  /* ── 2. Broadcast destination (always attempted as fallback) ─────── */
  struct _sa sa_bc = { 0 };
  sa_bc.sin_len    = 16;
  sa_bc.sin_family = _AF_INET;
  sa_bc.sin_port   = net_port;
  sa_bc.sin_addr   = 0xffffffffu;  /* 255.255.255.255 */

  long fd = syscall(SYS_socket, _AF_INET, _SOCK_DGRAM, _IPPROTO_UDP);
  if (fd < 0) return;

  int one = 1;
  syscall(SYS_setsockopt, fd, _SOL_SOCKET, _SO_BROADCAST, &one, (long)sizeof(one));

  /* Build message on the stack: "PROBE sN\n" */
  char msg[12];
  msg[0]='P'; msg[1]='R'; msg[2]='O'; msg[3]='B'; msg[4]='E';
  msg[5]=' '; msg[6]='s';
  msg[7] = '0' + (char)(step % 10);
  msg[8] = '\n'; msg[9] = '\0';

  /* Send unicast first (succeeds through AP isolation), then broadcast */
  if (has_unicast)
    syscall(SYS_sendto, fd, msg, (long)9, (long)0, &peer, (long)16);
  syscall(SYS_sendto, fd, msg, (long)9, (long)0, &sa_bc, (long)16);

  syscall(SYS_close, fd);
}


/* ── early_notify ────────────────────────────────────────────────────────────
 * Fire a PS5 on-screen toast at any point in startup — even before rtld runs.
 *
 * Uses ONLY:
 *   - args->sceKernelDlsym (a function pointer from elfldr, always valid)
 *   - Stack-allocated data (no globals, no string literals needing RELATIVE
 *     relocations — safe for both ET_EXEC and ET_DYN before rtld_load)
 *
 * step codes:
 *   0 = _start entered (CRT alive)
 *   1 = pre_init: getpid dlsym failed (both handles)
 *   2 = pre_init: __isthreaded dlsym failed
 *   3 = pre_init: __klog_init returned
 *   4 = pre_init: __kernel_init returned
 *   5 = pre_init: __rtld_init returned  (rc=0 → OK, rc!=0 → failed)
 *   6 = _start: reached main()
 */
static void
early_notify(payload_args_t *args, int step, int rc)
{
  /* SceNotificationRequest — matches browser_launch.c definition exactly.
   * Defined inline so this file needs no extra headers. */
  typedef struct {
    int  type;
    int  req_id;
    int  priority;
    int  msg_id;
    int  target_id;
    int  user_id;
    int  unk1;
    int  unk2;
    char icon_uri[1024];
    char message[1024];
  } SceNotifReq;

  int (*p_notify)(int, SceNotifReq*, long, int) = 0;

  /* Try handle 0x1 (libkernel), fall back to 0x2001. */
  if (args->sceKernelDlsym(0x1, "sceKernelSendNotificationRequest", &p_notify))
    args->sceKernelDlsym(0x2001, "sceKernelSendNotificationRequest", &p_notify);
  if (!p_notify)
    return;

  /* Zero the struct manually — memset not available before rtld. */
  SceNotifReq req;
  {
    char *p = (char *)&req;
    long  n = (long)sizeof(req);
    while (n--) *p++ = 0;
  }

  req.type      = 0;
  req.target_id = -1;

  /* Build message on the stack: "Quaza sN rc=0xXXXXXXXX"
   * No string literals — every character is a direct assignment. */
  char *m = req.message;
  /* "Quaza s" */
  m[0]='Q'; m[1]='u'; m[2]='a'; m[3]='z'; m[4]='a';
  m[5]=' '; m[6]='s';
  /* step digit(s) */
  m[7] = '0' + (char)(step % 10);
  /* " rc=0x" */
  m[8]=' '; m[9]='r'; m[10]='c'; m[11]='='; m[12]='0'; m[13]='x';
  /* 8 hex digits of rc */
  int j = 14;
  for (int shift = 28; shift >= 0; shift -= 4) {
    int nib = ((unsigned int)rc >> shift) & 0xf;
    m[j++] = nib < 10 ? '0' + nib : 'a' + nib - 10;
  }
  m[j] = '\0';

  p_notify(0, &req, (long)sizeof(req), 0);
}


static int
pre_init(payload_args_t *args) {
  int *__isthreaded;
  int error = 0;

  payload_args = args;

  /* ── step 1: get syscall ptr from libkernel ─────────────────────── */
  if(args->sceKernelDlsym(0x1, "getpid", &ptr_syscall)) {
    if((error=args->sceKernelDlsym(0x2001, "getpid", &ptr_syscall))) {
      early_notify(args, 1, error);
      return error;
    }
  }
  ptr_syscall += 0xa;   /* jump to the actual syscall insn inside getpid */
  raw_udp_probe(1);     /* syscall ptr ready — first probe that can fire */

  /* ── step 2: thread flag ────────────────────────────────────────── */
  if((error=args->sceKernelDlsym(0x2, "__isthreaded", &__isthreaded))) {
    early_notify(args, 2, error);
    return error;
  }
  *__isthreaded = 1;

  /* ── step 3: klog ───────────────────────────────────────────────── */
  if((error=__klog_init(args))) {
    early_notify(args, 3, error);
    return error;
  }
  early_notify(args, 3, 0);   /* klog OK */
  raw_udp_probe(3);

  /* ── step 4: kernel patches (non-fatal on unknown firmware) ─────── */
  if((error=__kernel_init(args))) {
    early_notify(args, 4, error);
    raw_udp_probe(4);
    return error;
  }
  early_notify(args, 4, 0);   /* kernel OK */

  /* ── step 5: dynamic linker (loads DT_NEEDED .sprx files) ──────── */
  if((error=__rtld_init(args))) {
    early_notify(args, 5, error);
    raw_udp_probe(5);
    return error;
  }
  early_notify(args, 5, rtld_null_syms);   /* rc = unresolved symbol count */
  raw_udp_probe(5);

  return 0;
}


/**
 * Terminate the payload.
 **/
static void
terminate(payload_args_t *args) {
  void (*exit)(int) = 0;
  long dummy;

  // we are running inside a hijacked process, just return
  if(args->sceKernelDlsym(0x1, "sceKernelDlsym", &dummy)) {
    return;
  }

  if(!args->sceKernelDlsym(0x2, "exit", &exit)) {
    exit(*args->payloadout);
  }
}


/**
 * Entry-point invoked by the ELF loader.
 **/
void
_start(payload_args_t *args, int argc, char* argv[], char* envp[]) {
  unsigned long count = 0;

  /* Probe 0: very first thing — before BSS clear.
   * If the ELF is ET_DYN loaded at non-zero base, __bss_start/__bss_end
   * hold wrong link-time VMAs and the clear loop corrupts memory.
   * Firing the probe here tells us whether _start is even being called. */
  early_notify(args, 0, 0);

  /* Clear .bss section. */
  for(unsigned char* bss=__bss_start; bss<__bss_end; bss++) {
    *bss = 0;
  }

  *args->payloadout = 0;
  if((*args->payloadout=pre_init(args))) {
    terminate(args);
    return;
  }

  /* Probe 6: main() about to run. */
  early_notify(args, 6, 0);
  raw_udp_probe(6);

  /* Run .init functions. */
  count = __init_array_end - __init_array_start;
  for(int i=0; i<count; i++) {
    __init_array_start[i](args, argc, argv, envp);
  }

  /* Run the actual payload. */
  *args->payloadout = main(argc, argv, envp);

  /* Run .fini functions. */
  count = __fini_array_end - __fini_array_start;
  for(int i=0; i<count; i++) {
    __fini_array_start[count-i-1]();
  }

  terminate(args);
}
