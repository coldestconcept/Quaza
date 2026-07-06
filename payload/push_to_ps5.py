#!/usr/bin/env python3
"""
push_to_ps5.py — send the freshly built quaza_payload.elf straight to a
                 PS5 running ps5-payload-elfldr, over the network.

elfldr listens on TCP port 9021 (by default) and expects:
    1. The ELF size as an 8-byte little-endian unsigned integer.
    2. The raw ELF bytes.
It loads the ELF into memory and executes it immediately — no PKG install,
no FTP, no reboot required. Requires elfldr already running on the PS5
(load it once via GoldHEN / another loader).

Usage:
    python3 payload/push_to_ps5.py <ps5-ip> [elf-path] [port]

Examples:
    python3 payload/push_to_ps5.py 192.168.1.50
    python3 payload/push_to_ps5.py 192.168.1.50 payload/quaza_payload.elf
    python3 payload/push_to_ps5.py 192.168.1.50 payload/quaza_payload.elf 9021
"""

import os
import socket
import struct
import sys

DEFAULT_PORT = 9021
DEFAULT_ELF = os.path.join(os.path.dirname(__file__), "quaza_payload.elf")


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__.strip())
        return 1

    ps5_ip = sys.argv[1]
    elf_path = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_ELF
    port = int(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_PORT

    if not os.path.isfile(elf_path):
        print(f"ERROR: ELF not found at {elf_path} — run `make` in payload/ first", file=sys.stderr)
        return 1

    with open(elf_path, "rb") as f:
        elf_bytes = f.read()

    size = len(elf_bytes)
    print(f"==> Connecting to {ps5_ip}:{port} (elfldr) ...")

    try:
        with socket.create_connection((ps5_ip, port), timeout=5) as sock:
            print(f"==> Sending {elf_path} ({size:,} bytes) ...")
            sock.sendall(struct.pack("<Q", size))
            sock.sendall(elf_bytes)
    except (ConnectionRefusedError, OSError) as e:
        print(f"ERROR: could not reach elfldr at {ps5_ip}:{port}: {e}", file=sys.stderr)
        print("Make sure elfldr is running on the PS5 and both devices are on the same network.", file=sys.stderr)
        return 1

    print("Done. quaza_payload.elf sent — check the PS5 for it launching (port 4242).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
