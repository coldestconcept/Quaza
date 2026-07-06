#!/usr/bin/env sh
# push_elf.sh — delete the old quaza_payload.elf release asset and upload
#               the freshly built one.
#
# Run on Termux after build:
#   export GITHUB_TOKEN=ghp_xxxxxxxxxxxx
#   sh payload/push_elf.sh

if [ -z "$GITHUB_TOKEN" ]; then
  echo "ERROR: set GITHUB_TOKEN before running this script" >&2
  exit 1
fi

REPO="coldestconcept/Quaza"
RELEASE_ID="349020167"
ELF="$HOME/Quaza/payload/build_direct/quaza_payload.elf"

if [ ! -f "$ELF" ]; then
  echo "ERROR: ELF not found at $ELF — run build_direct.sh first" >&2
  exit 1
fi

ASSET_ID=$(curl -s -H "Authorization: token $GITHUB_TOKEN" \
  "https://api.github.com/repos/${REPO}/releases/${RELEASE_ID}/assets" \
  | python3 -c "import sys,json; a=json.load(sys.stdin); print(a[0]['id']) if a else print('')")

[ -n "$ASSET_ID" ] && curl -s -X DELETE \
  -H "Authorization: token $GITHUB_TOKEN" \
  "https://api.github.com/repos/${REPO}/releases/assets/$ASSET_ID"

curl -X POST \
  -H "Authorization: token $GITHUB_TOKEN" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @"$ELF" \
  "https://uploads.github.com/repos/${REPO}/releases/${RELEASE_ID}/assets?name=quaza_payload.elf"
