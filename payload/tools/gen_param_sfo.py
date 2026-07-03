#!/usr/bin/env python3
"""
gen_param_sfo.py — Generate binary param.sfo from a JSON config.

Usage:
    python3 payload/tools/gen_param_sfo.py \
        payload/sce_sys/param_sfo_config.json \
        payload/sce_sys/param.sfo

The binary format is the standard PS4/PS5 SFO layout:
  [header][index table][key table][data table]
"""

import json
import struct
import sys
import os

SFO_MAGIC   = b"\x00PSF"
SFO_VERSION = 0x00000101

FMT_UTF8_S  = 0x0004   # fixed-length UTF-8 (null-padded), used for CONTENT_ID etc.
FMT_UTF8    = 0x0204   # variable UTF-8 string
FMT_INT32   = 0x0404   # 32-bit integer

# Field definitions: (json_key, sfo_key, fmt, max_len)
# max_len is bytes allocated in the data table (including null terminator).
FIELDS = [
    ("APP_TYPE",    "APP_TYPE",    FMT_INT32,  4),
    ("APP_VER",     "APP_VER",     FMT_UTF8_S, 8),
    ("CATEGORY",    "CATEGORY",    FMT_UTF8_S, 4),
    ("CONTENT_ID",  "CONTENT_ID",  FMT_UTF8_S, 48),
    ("PUBTOOLINFO", "PUBTOOLINFO", FMT_UTF8,   512),
    ("SYSTEM_VER",  "SYSTEM_VER",  FMT_INT32,  4),
    ("TITLE",       "TITLE",       FMT_UTF8,   128),
    ("TITLE_ID",    "TITLE_ID",    FMT_UTF8_S, 12),
    ("VERSION",     "VERSION",     FMT_UTF8_S, 8),
]

def align(value, boundary):
    return (value + boundary - 1) & ~(boundary - 1)

def build_sfo(config):
    # Build key table and data table
    entries    = []  # (key_bytes, data_bytes, fmt, data_max)
    key_table  = b""
    data_table = b""

    key_offsets  = []
    data_offsets = []

    for json_key, sfo_key, fmt, max_len in FIELDS:
        if json_key not in config:
            continue
        value = config[json_key]

        # Encode key
        key_bytes = sfo_key.encode("utf-8") + b"\x00"
        key_offsets.append(len(key_table))
        key_table += key_bytes

        # Encode value
        if fmt == FMT_INT32:
            data_bytes = struct.pack("<I", int(value))
            data_len   = 4
            data_max   = 4
        else:
            enc = value.encode("utf-8") + b"\x00"
            data_len = len(enc)
            data_max = max_len
            # pad to max_len
            data_bytes = enc + b"\x00" * (max_len - len(enc))

        data_offsets.append(len(data_table))
        data_table += data_bytes
        entries.append((sfo_key, fmt, data_len, data_max))

    # Align key table to 4 bytes
    key_table_aligned = key_table + b"\x00" * (align(len(key_table), 4) - len(key_table))

    n = len(entries)
    key_table_offset  = 20 + n * 16           # header(20) + index(n*16)
    data_table_offset = key_table_offset + len(key_table_aligned)

    # Pack header
    hdr = struct.pack("<4sIIII",
        SFO_MAGIC, SFO_VERSION,
        key_table_offset, data_table_offset, n)

    # Pack index table
    idx = b""
    for i, (_, fmt, data_len, data_max) in enumerate(entries):
        idx += struct.pack("<HHIII",
            key_offsets[i], fmt, data_len, data_max, data_offsets[i])

    return hdr + idx + key_table_aligned + data_table


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <config.json> <output.sfo>", file=sys.stderr)
        sys.exit(1)

    config_path = sys.argv[1]
    out_path    = sys.argv[2]

    with open(config_path) as f:
        config = json.load(f)

    # Remove comment keys
    config = {k: v for k, v in config.items() if not k.startswith("_")}

    sfo_data = build_sfo(config)

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(sfo_data)

    print(f"[gen_param_sfo] Written {len(sfo_data)} bytes → {out_path}")

if __name__ == "__main__":
    main()
