# PKG Converter

The Quaza PKG Converter allows you to convert decrypted game dumps into installable PKG files directly on your PS5.

## Features

- Native PS5 PKG creation (no PC required)
- PFS image generation
- NPDRM header creation
- Progress tracking via web interface
- Direct download of created PKG files

## Usage

1. **Load the payload**: Send `quaza_payload.elf` to your PS5
2. **Open the web interface**: Navigate to `http://<ps5-ip>:8080`
3. **Go to PKG Converter**: Click the PKG Converter card
4. **Enter details**:
   - Dump Path: Full path to decrypted game dump (e.g., `/mnt/usb0/CUSA00000-app`)
   - Content ID: The game's content ID from param.sfo
5. **Start conversion**: Click "Start Conversion"
6. **Wait**: Monitor progress in real-time
7. **Download**: Once complete, download the PKG file

## API Endpoints

- `GET /api/status` - Server status
- `GET /api/pkg/create?path=<path>&content_id=<id>` - Start PKG creation
- `GET /api/pkg/progress` - Get creation progress
- `GET /api/pkg/download?path=<path>` - Download created PKG

## Technical Details

The PKG converter uses a native implementation of the PS5 PKG format:

- **PKG Header**: Standard PS5 PKG header with entry table
- **PFS Image**: Proprietary File System image containing game files
- **NPDRM**: License header (fake for homebrew)

## Building

See [BUILD.md](BUILD.md) for compilation instructions.
