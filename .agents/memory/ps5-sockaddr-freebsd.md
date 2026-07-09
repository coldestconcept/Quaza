---
name: PS5 FreeBSD sockaddr layout
description: FreeBSD/PS5 socket structs have sin_len/sa_len as the first byte — missing this causes all sendto/bind to silently fail.
---

## The rule
FreeBSD `sockaddr_in` starts with a 1-byte `sin_len` field (total struct length) BEFORE `sin_family`. Linux doesn't have this field. PS5/Orbis OS is FreeBSD-derived, so the PS5 kernel reads byte 0 as the length. A Linux-style struct puts `AF_INET=2` at byte 0 → kernel sees sin_len=2, sin_family=AF_UNSPEC → every `sendto`/`bind` silently returns EINVAL.

**Why:** Spent two debug cycles on this. The symptom is completely silent failure: socket() succeeds, but sendto/bind return error with no visible output. In SDK mode the SDK headers are correct, but any code that zeroes a sockaddr_in with memset without then setting sin_len is broken on PS5.

**How to apply:**
- In stub headers (`payload/include/netinet/in.h`): struct must use `uint8_t sin_len` first, then `uint8_t sin_family`
- In any C code initializing a `struct sockaddr_in`: always set `addr.sin_len = sizeof(addr)` after memset
- `struct sockaddr` in `sys/socket.h` similarly needs `uint8_t sa_len` first

Files already fixed: `payload/netlog.c`, `payload/pkg_server.c`, `payload/include/netinet/in.h`, `payload/include/sys/socket.h`.

## AP isolation workaround
WiFi routers with client/AP isolation block UDP broadcasts (255.255.255.255) between wireless devices. The payload's `raw_udp_probe` and `netlog` both try `getpeername(stdin)` to detect the pusher's IP (if elfldr dup2'd the TCP socket onto stdin) and send unicast there in addition to broadcast. This is implemented in `payload/crt/crt.c` and `payload/netlog.c`.
