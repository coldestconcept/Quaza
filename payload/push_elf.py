#!/usr/bin/env python3
"""
push_elf.py — delete the old quaza_payload.elf from the v1.0.0 release
              and upload the freshly built one.

Run on Termux after build_direct.sh:
    export GITHUB_TOKEN=ghp_xxxxxxxxxxxx
    python3 payload/push_elf.py
"""

import os, sys, json, urllib.request, urllib.error

REPO       = "coldestconcept/Quaza"
TAG        = "v1.0.0"
ASSET_NAME = "quaza_payload.elf"
ELF        = os.path.join(os.path.dirname(__file__), "build_direct", "quaza_payload.elf")
BASE       = "https://api.github.com"

TOKEN = os.environ.get("GITHUB_TOKEN", "")
if not TOKEN:
    sys.exit("ERROR: set GITHUB_TOKEN before running this script")
if not os.path.isfile(ELF):
    sys.exit(f"ERROR: ELF not found at {ELF} — run build_direct.sh first")

def api(method, path, body=None, base=BASE):
    url = f"{base}{path}"
    data = json.dumps(body).encode() if body else None
    req = urllib.request.Request(url, data=data, method=method)
    req.add_header("Authorization", f"token {TOKEN}")
    req.add_header("Accept", "application/vnd.github+json")
    if body:
        req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req) as r:
            raw = r.read()
            return json.loads(raw) if raw else {}
    except urllib.error.HTTPError as e:
        sys.exit(f"HTTP {e.code} {method} {url}: {e.read().decode()}")

# 1. Get release
print(f"==> Fetching release {TAG} ...")
rel = api("GET", f"/repos/{REPO}/releases/tags/{TAG}")
release_id  = rel["id"]
upload_base = rel["upload_url"].split("{")[0]   # strip {?name,label}
print(f"    id={release_id}  upload={upload_base}")

# 2. Find and delete existing asset
print(f"==> Checking for existing {ASSET_NAME} ...")
assets = api("GET", f"/repos/{REPO}/releases/{release_id}/assets")
for asset in assets:
    if asset["name"] == ASSET_NAME:
        print(f"    deleting old asset id={asset['id']} ({asset['size']} bytes)")
        api("DELETE", f"/repos/{REPO}/releases/assets/{asset['id']}")
        print("    deleted.")
        break
else:
    print("    none found, skipping delete.")

# 3. Upload new ELF
size = os.path.getsize(ELF)
print(f"==> Uploading {ELF} ({size:,} bytes) ...")
url = f"{upload_base}?name={ASSET_NAME}&label={ASSET_NAME}"
with open(ELF, "rb") as f:
    data = f.read()
req = urllib.request.Request(url, data=data, method="POST")
req.add_header("Authorization", f"token {TOKEN}")
req.add_header("Accept", "application/vnd.github+json")
req.add_header("Content-Type", "application/octet-stream")
try:
    with urllib.request.urlopen(req) as r:
        result = json.loads(r.read())
except urllib.error.HTTPError as e:
    sys.exit(f"Upload failed HTTP {e.code}: {e.read().decode()}")

print(f"\nDone: {result['browser_download_url']}")
