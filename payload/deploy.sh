#!/usr/bin/env bash
# deploy.sh — rebuild quaza_payload.elf and push it straight to a PS5
#             running ps5-payload-elfldr, in one step.
#
# Usage:
#   sh payload/deploy.sh <ps5-ip> [port]
#
# Examples:
#   sh payload/deploy.sh 192.168.1.50
#   sh payload/deploy.sh 192.168.1.50 9021

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [ -z "$1" ]; then
    echo "Usage: $0 <ps5-ip> [port]" >&2
    exit 1
fi

PS5_IP="$1"
PORT="${2:-9021}"

echo "==> Building quaza_payload.elf ..."
make -C "$SCRIPT_DIR"

echo "==> Deploying to $PS5_IP:$PORT ..."
python3 "$SCRIPT_DIR/push_to_ps5.py" "$PS5_IP" "$SCRIPT_DIR/quaza_payload.elf" "$PORT"
