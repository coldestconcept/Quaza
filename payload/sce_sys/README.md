# sce_sys/

This folder becomes the `sce_sys/` directory inside the Quaza fPKG.
It tells the PS5 game library what the app is called, what icon to show, and what firmware it needs.

## Files

| File | Status | Description |
|------|--------|-------------|
| `param.sfo` | **generated** | Binary metadata — auto-generated from `param_sfo_config.json` at build time. Do **not** commit the binary; edit the JSON instead. |
| `param_sfo_config.json` | source | Human-readable config for `param.sfo`. Edit here. |
| `icon0.png` | **you provide** | 512 × 512 px PNG shown in the PS5 game library. Not committed to the repo — create your own. |

## Creating icon0.png

The icon must be exactly **512 × 512 pixels**, PNG format.

Quick option using ImageMagick (creates a solid ice-blue placeholder):
```bash
convert -size 512x512 xc:'#5ecfee' \
    -font DejaVu-Sans-Bold -pointsize 80 \
    -fill '#1a3a52' -gravity center \
    -annotate 0 "Quaza\nPKG Tool" \
    payload/sce_sys/icon0.png
```

Replace it with your own artwork before distributing.

## Editing app metadata

Open `param_sfo_config.json` and change the values you want, then the build system regenerates `param.sfo` automatically. Key fields:

| Field | Current value | Notes |
|-------|--------------|-------|
| `TITLE` | `Quaza PKG Tool` | Shown under the icon |
| `TITLE_ID` | `QUAZ00001` | Must be exactly 9 chars (4 letters + 5 digits) |
| `CONTENT_ID` | `EP0000-QUAZ00001_00-QUAZAPKGTOOL0001` | Must be 36 chars |
| `APP_VER` | `01.00` | Shown in the PS5 app info panel |
| `SYSTEM_VER` | `851968` | Minimum PS5 FW (0xD0000 = 4.00) |
