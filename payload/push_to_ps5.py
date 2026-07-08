#!/usr/bin/env python3
"""
push_to_ps5.py — send the freshly built quaza_payload.elf to a PS5 running
                 ps5-payload-elfldr, then stream back all stdout/stderr output
                 printed by the running payload.

elfldr listens on TCP port 9021 (by default) and expects:
    1. The ELF size as an 8-byte little-endian unsigned integer.
    2. The raw ELF bytes.
It loads the ELF into memory, executes it, and pipes its stdout/stderr back
over the same TCP connection until the payload exits.

Usage:
    python3 payload/push_to_ps5.py <ps5-ip> [elf-path] [port]

Examples:
    python3 payload/push_to_ps5.py 192.168.1.50
    python3 payload/push_to_ps5.py 192.168.1.50 payload/quaza_payload.elf
    python3 payload/push_to_ps5.py 192.168.1.50 payload/quaza_payload.elf 9021

While the connection is open you will see every printf/fprintf call from the
running payload in real time.  Hit Ctrl-C to disconnect (the payload keeps
running on the PS5).

UDP netlog (secondary channel)
───────────────────────────────
The payload also broadcasts each NL_LOG() line as a UDP datagram to port 9020
on 255.255.255.255.  To receive those in a second terminal:

    nc -ul 9020
"""

import os
import select
import socket
import struct
import sys
import threading

DEFAULT_PORT    = 9021
DEFAULT_ELF     = os.path.join(os.path.dirname(__file__), "quaza_payload.elf")
NETLOG_UDP_PORT = 9020


# ── UDP netlog listener (optional second channel) ─────────────────────────────

def _udp_listener(stop_event: threading.Event) -> None:
    """Listen for UDP broadcast datagrams from the payload and print them."""
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("", NETLOG_UDP_PORT))
        sock.settimeout(0.5)
        print(f"[netlog] Listening for UDP datagrams on :{NETLOG_UDP_PORT} …",
              flush=True)
        while not stop_event.is_set():
            try:
                data, addr = sock.recvfrom(4096)
                msg = data.decode("utf-8", errors="replace").rstrip("\n")
                print(f"[UDP {addr[0]}] {msg}", flush=True)
            except socket.timeout:
                pass
    except OSError as e:
        print(f"[netlog] Could not open UDP :{NETLOG_UDP_PORT}: {e}", flush=True)
    finally:
        if sock is not None:
            sock.close()


# ── Main ──────────────────────────────────────────────────────────────────────

def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__.strip())
        return 1

    ps5_ip   = sys.argv[1]
    elf_path = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_ELF
    port     = int(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_PORT

    if not os.path.isfile(elf_path):
        print(f"ERROR: ELF not found at {elf_path} — run `make` in payload/ first",
              file=sys.stderr)
        return 1

    with open(elf_path, "rb") as f:
        elf_bytes = f.read()
    size = len(elf_bytes)

    # Start UDP listener thread so we capture netlog output even before the
    # TCP connection is fully established.
    stop_evt = threading.Event()
    udp_thread = threading.Thread(target=_udp_listener, args=(stop_evt,),
                                  daemon=True)
    udp_thread.start()

    print(f"==> Connecting to {ps5_ip}:{port} (elfldr) …", flush=True)
    try:
        sock = socket.create_connection((ps5_ip, port), timeout=10)
    except (ConnectionRefusedError, OSError) as e:
        print(f"ERROR: could not reach elfldr at {ps5_ip}:{port}: {e}",
              file=sys.stderr)
        print("Make sure elfldr is running on the PS5 and both devices are "
              "on the same network.", file=sys.stderr)
        stop_evt.set()
        return 1

    try:
        # Send ELF
        print(f"==> Sending {elf_path} ({size:,} bytes) …", flush=True)
        sock.sendall(struct.pack("<Q", size))
        sock.sendall(elf_bytes)

        # Signal end-of-send so elfldr knows we are done uploading.
        # Half-close the write side; elfldr will then start executing the ELF
        # and piping its stdout/stderr back.
        sock.shutdown(socket.SHUT_WR)

        print("==> ELF sent — streaming payload output (Ctrl-C to stop) …\n",
              flush=True)

        # Stream back whatever elfldr sends (payload stdout + stderr)
        sock.setblocking(False)
        buf = b""
        while True:
            try:
                ready, _, _ = select.select([sock], [], [], 0.2)
            except KeyboardInterrupt:
                print("\n[push] Disconnected (payload still running on PS5).",
                      flush=True)
                break

            if ready:
                try:
                    chunk = sock.recv(4096)
                except OSError:
                    chunk = b""

                if not chunk:
                    # Flush any partial line that didn't end with \n
                    if buf:
                        print(buf.decode("utf-8", errors="replace"), flush=True)
                        buf = b""
                    print("\n[push] Connection closed by PS5 (payload exited or "
                          "elfldr disconnected).", flush=True)
                    break

                buf += chunk
                # Print complete lines immediately; hold partial lines
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    print(line.decode("utf-8", errors="replace"), flush=True)

    except KeyboardInterrupt:
        print("\n[push] Disconnected.", flush=True)
    finally:
        sock.close()
        stop_evt.set()

    return 0


if __name__ == "__main__":
    sys.exit(main())
