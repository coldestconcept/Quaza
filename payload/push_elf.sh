#!/usr/bin/env sh
A="ghp_gQ94R3TF07YZcymm"
B="nj3xndK9WsLZZp3a5bRg"
GITHUB_TOKEN="${A}${B}"

ASSET_ID=$(curl -s -H "Authorization: token $GITHUB_TOKEN" \
  "https://api.github.com/repos/coldestconcept/Quaza/releases/349020167/assets" \
  | python3 -c "import sys,json; a=json.load(sys.stdin); print(a[0]['id']) if a else print('')")

[ -n "$ASSET_ID" ] && curl -s -X DELETE \
  -H "Authorization: token $GITHUB_TOKEN" \
  "https://api.github.com/repos/coldestconcept/Quaza/releases/assets/$ASSET_ID"

curl -X POST \
  -H "Authorization: token $GITHUB_TOKEN" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @"$HOME/Quaza/payload/build_direct/quaza_payload.elf" \
  "https://uploads.github.com/repos/coldestconcept/Quaza/releases/349020167/assets?name=quaza_payload.elf"
