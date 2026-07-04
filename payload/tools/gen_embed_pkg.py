#!/usr/bin/env python3
"""
gen_embed_pkg.py — Convert a built tile PKG into a C byte-array header.

Usage:
    python3 payload/tools/gen_embed_pkg.py <input.pkg> <output_header.h>

The header exposes:
    extern const unsigned char g_tile_pkg_data[];
    extern const size_t        g_tile_pkg_size;
"""

import sys
import os

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.pkg> <output_header.h>",
              file=sys.stderr)
        sys.exit(1)

    pkg_path    = sys.argv[1]
    header_path = sys.argv[2]

    with open(pkg_path, "rb") as f:
        data = f.read()

    size = len(data)

    # Format as C hex array, 16 bytes per line
    lines = []
    for i in range(0, size, 16):
        chunk = data[i:i+16]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")

    hex_body = "\n".join(lines)

    header = f"""\
/* AUTO-GENERATED — do not edit.
 * Run payload/tools/gen_embed_pkg.py to regenerate.
 * Source: {os.path.basename(pkg_path)} ({size:,} bytes)
 */
#ifndef TILE_PKG_EMBED_H
#define TILE_PKG_EMBED_H
#include <stddef.h>

/* Quaza tile PKG — install with sceAppInstUtilAppInstallPkg() */
static const unsigned char g_tile_pkg_data[] = {{
{hex_body}
}};
static const size_t g_tile_pkg_size = {size}UL;

#endif /* TILE_PKG_EMBED_H */
"""

    os.makedirs(os.path.dirname(os.path.abspath(header_path)), exist_ok=True)
    with open(header_path, "w") as f:
        f.write(header)

    print(f"[gen_embed_pkg] {size:,} bytes → {header_path}")

if __name__ == "__main__":
    main()
