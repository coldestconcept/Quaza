#!/usr/bin/env python3
"""
push_to_ps5.py — send quaza_payload.elf to a PS5 running ps5-payload-elfldr,
                 then listen for UDP netlog datagrams broadcast by the payload.

Protocol:
  - Send the raw ELF bytes, then close the write side (SHUT_WR).
  - elfldr reads until EOF, loads and runs the ELF.
  - The payload broadcasts every NL_LOG() line as a UDP datagram to
    255.255.255.255:9020.  This script listens on :9020 in the background
    and prints those lines in real time.

Usage:
    python3 payload/push_to_ps5.py <ps5-ip> [elf-path] [port]

Examples:
    python3 payload/push_to_ps5.py 192.168.1.50
    python3 payload/push_to_ps5.py 192.168.1.50 payload/quaza_payload.elf
    python3 payload/push_to_ps5.py 192.168.1.50 payload/quaza_payload.elf 9021
"""

import os
import select
import socket
import struct
import sys
import threading
import time

DEFAULT_PORT    = 9021
DEFAULT_ELF     = os.path.join(os.path.dirname(__file__), "quaza_payload.elf")
NETLOG_UDP_PORT = 9020


# ── UDP netlog listener ───────────────────────────────────────────────────────

def _udp_listener(stop_event: threading.Event) -> None:
    """Receive UDP broadcast datagrams from the payload and print them."""
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("", NETLOG_UDP_PORT))
        sock.settimeout(0.5)
        print(f"[netlog] Listening on UDP :{NETLOG_UDP_PORT} …", flush=True)
        while not stop_event.is_set():
            try:
                data, addr = sock.recvfrom(4096)
                msg = data.decode("utf-8", errors="replace").rstrip("\n")
                print(f"[UDP {addr[0]}] {msg}", flush=True)
            except socket.timeout:
                pass
    except OSError as e:
        print(f"[netlog] Could not bind UDP :{NETLOG_UDP_PORT}: {e}", flush=True)
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

    # Start UDP listener before connecting so we don't miss early boot messages.
    stop_evt = threading.Event()
    udp_thread = threading.Thread(target=_udp_listener, args=(stop_evt,),
                                  daemon=True)
    udp_thread.start()
    time.sleep(0.1)  # give the thread time to bind

    print(f"==> Connecting to {ps5_ip}:{port} (elfldr) …", flush=True)
    try:
        sock = socket.create_connection((ps5_ip, port), timeout=10)
    except (ConnectionRefusedError, OSError) as e:
        print(f"ERROR: could not reach elfldr at {ps5_ip}:{port}: {e}",
              file=sys.stderr)
        stop_evt.set()
        return 1

    try:
        print(f"==> Sending {elf_path} ({size:,} bytes) …", flush=True)
        sock.sendall(elf_bytes)
        sock.shutdown(socket.SHUT_WR)   # EOF — tells elfldr the ELF is complete

        # Drain any bytes elfldr sends back (e.g. status lines)
        print("==> ELF sent. Waiting for UDP log (Ctrl-C to stop) …\n",
              flush=True)
        sock.setblocking(False)
        buf = b""
        while True:
            try:
                ready, _, _ = select.select([sock], [], [], 0.2)
            except KeyboardInterrupt:
                break
            if ready:
                try:
                    chunk = sock.recv(4096)
                except OSError:
                    chunk = b""
                if not chunk:
                    if buf:
                        print(buf.decode("utf-8", errors="replace"), flush=True)
                    print("[TCP closed by PS5]", flush=True)
                    break
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    print(f"[TCP] {line.decode('utf-8', errors='replace')}",
                          flush=True)

    except KeyboardInterrupt:
        print("\n[push] Disconnected.", flush=True)
    finally:
        sock.close()

    # Keep UDP listener alive after TCP closes — payload keeps running.
    if not stop_evt.is_set():
        print("[push] TCP closed. UDP log still active — press Ctrl-C to exit.",
              flush=True)
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            print("\n[push] Exiting.", flush=True)

    stop_evt.set()
    return 0


if __name__ == "__main__":
    sys.exit(main())
