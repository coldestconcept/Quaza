# Installing Quaza on PS5

Quaza installs as a game library icon — exactly like ELF Arsenal, Kuro for PS5, and PS5 Payload Manager. Once installed you click the icon, the payload starts, the PS5 browser opens automatically at `http://127.0.0.1:4242`, and you're straight into the tool.

## Prerequisites

- PS5 on a supported jailbreak firmware with GoldHEN (or equivalent)
- A Linux build machine with the PS5 Payload SDK installed
- `python3` on your build machine
- `orbis-pub-cmd` **or** `fp-pkg` on PATH (for PKG generation)

---

## Step 1 — Build

```bash
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
mkdir build && cd build
cmake ..
make
```

This produces two things:
- `build/quaza_payload.elf` — the raw ELF (can be injected directly via a loader)
- `build/Quaza.pkg`         — installable fPKG with icon, for game library integration

---

## Step 2 — Create the app icon

The icon file `payload/sce_sys/icon0.png` must be **512 × 512 px PNG** before building.
Create one with any image editor and drop it at that path.

The `param.sfo` (app metadata) is generated automatically from
`payload/sce_sys/param_sfo_config.json` during the build.
Edit that JSON to change the title or title ID — then rebuild.

---

## Step 3 — Install the PKG

Transfer `Quaza.pkg` to your PS5 and install it using one of:

| Tool | How |
|------|-----|
| **GoldHEN Package Installer** | Settings → GoldHEN → Package Installer → select Quaza.pkg |
| **ItemZz** | Drop PKG via FTP, install from ItemZz |
| **Direct FTP** | Copy to `/user/app/` directory structure manually |

After installation Quaza appears in your PS5 game library with the icon.

---

## Step 4 — Launch

Click the Quaza icon in your game library.

**What happens:**
1. The ELF payload starts.
2. The HTTP server binds to port **4242**.
3. The PS5 browser opens automatically at `http://127.0.0.1:4242`.
4. The Quaza UI loads — enter your dump path and the Content ID auto-detects.

---

## Offline mode

All web assets (HTML/JS) are **compiled directly into the ELF** via `payload/www_embed.h`.
The server serves them from memory, so Quaza works with zero files on disk.

If you change `www/` files, regenerate the embed header before rebuilding:

```bash
python3 payload/tools/gen_embed.py
```

---

## Raw ELF injection (no PKG)

If you just want to test without installing the PKG, inject the raw ELF through your loader (e.g. ps5-payload-elfldr) as usual. The browser won't auto-open in this mode — navigate manually to `http://<ps5-ip>:4242` from any device on the same network.

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Quaza icon missing after install | Check `param.sfo` TITLE_ID is exactly 9 chars |
| Browser doesn't open automatically | `sceLncUtilLaunchWebBrowser2` may need GoldHEN ≥ 2.4 |
| `Content ID detected` shows wrong value | The dump's `sce_sys/param.sfo` may be corrupt — enter manually |
| Port 4242 already in use | Another payload is running; kill it or reboot the PS5 |
