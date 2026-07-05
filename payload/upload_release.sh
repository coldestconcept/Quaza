#!/usr/bin/env sh
# upload_release.sh — upload quaza_payload.elf to a GitHub release
#
# Usage (on Termux):
#   export GITHUB_TOKEN=ghp_xxxxxxxxxxxx
#   sh payload/upload_release.sh [tag]
#
# If [tag] is omitted, defaults to "latest".
# Creates the release if it doesn't exist yet.

set -e

REPO="coldestconcept/Quaza"
ELF="$(dirname "$0")/build_direct/quaza_payload.elf"
TAG="${1:-latest}"
API="https://api.github.com"

# ── require curl and token ───────────────────────────────────────────────────
if [ -z "$GITHUB_TOKEN" ]; then
    echo "ERROR: set GITHUB_TOKEN before running this script" >&2
    exit 1
fi
if ! command -v curl >/dev/null 2>&1; then
    echo "ERROR: curl not found (pkg install curl)" >&2
    exit 1
fi
if [ ! -f "$ELF" ]; then
    echo "ERROR: ELF not found at $ELF — run build_direct.sh first" >&2
    exit 1
fi

AUTH="-H \"Authorization: token $GITHUB_TOKEN\""
HDR="-H \"Accept: application/vnd.github+json\""

api() {
    method="$1"; path="$2"; shift 2
    curl -sf -X "$method" \
         -H "Authorization: token $GITHUB_TOKEN" \
         -H "Accept: application/vnd.github+json" \
         "$@" \
         "${API}${path}"
}

# ── get or create release ────────────────────────────────────────────────────
echo "==> Looking up release '$TAG' in $REPO ..."
release=$(api GET "/repos/$REPO/releases/tags/$TAG" 2>/dev/null || true)

if echo "$release" | grep -q '"id"'; then
    release_id=$(echo "$release" | python3 -c "import sys,json; print(json.load(sys.stdin)['id'])")
    upload_url=$(echo "$release" | python3 -c "import sys,json; print(json.load(sys.stdin)['upload_url'].split('{')[0])")
    echo "    found release id=$release_id"
else
    echo "==> Release '$TAG' not found — creating it ..."
    release=$(api POST "/repos/$REPO/releases" \
        -H "Content-Type: application/json" \
        -d "{\"tag_name\":\"$TAG\",\"name\":\"$TAG\",\"draft\":false,\"prerelease\":false}")
    release_id=$(echo "$release" | python3 -c "import sys,json; print(json.load(sys.stdin)['id'])")
    upload_url=$(echo "$release" | python3 -c "import sys,json; print(json.load(sys.stdin)['upload_url'].split('{')[0])")
    echo "    created release id=$release_id"
fi

# ── delete existing asset with the same name (allows re-upload) ──────────────
echo "==> Checking for existing quaza_payload.elf asset ..."
assets=$(api GET "/repos/$REPO/releases/$release_id/assets")
existing_id=$(echo "$assets" | python3 -c "
import sys, json
assets = json.load(sys.stdin)
for a in assets:
    if a['name'] == 'quaza_payload.elf':
        print(a['id'])
        break
" 2>/dev/null || true)

if [ -n "$existing_id" ]; then
    echo "    deleting old asset id=$existing_id ..."
    api DELETE "/repos/$REPO/releases/assets/$existing_id" > /dev/null
fi

# ── upload ───────────────────────────────────────────────────────────────────
size=$(wc -c < "$ELF")
echo "==> Uploading $ELF ($size bytes) to release '$TAG' ..."
result=$(curl -sf -X POST \
    -H "Authorization: token $GITHUB_TOKEN" \
    -H "Accept: application/vnd.github+json" \
    -H "Content-Type: application/octet-stream" \
    --data-binary "@$ELF" \
    "${upload_url}?name=quaza_payload.elf&label=quaza_payload.elf")

asset_url=$(echo "$result" | python3 -c "import sys,json; print(json.load(sys.stdin)['browser_download_url'])")
echo ""
echo "Done: $asset_url"
