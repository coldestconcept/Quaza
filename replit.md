# Quaza — PS5 PKG Tool

Browser-based game dump-to-PKG converter for jailbroken PS5.

## How to run

The web interface is served as a static site:

```
python3 -m http.server 5000 --directory www
```

This is configured as the "Start application" workflow and runs on port 5000.

## Project structure

- `www/` — Static HTML/JS web frontend (edit this to work on the UI)
- `payload/` — PS5 ELF payload (C, HTTP server that runs *on the PS5*; requires PS5 Payload SDK to build)
- `python/scraper.py` — Scraper for prosperopatches.com
- `docs/` — Build instructions and API docs

## How it works

The PS5 browser loads `www/index.html` from a web server. The frontend sends API calls to the PS5 payload's HTTP server (port 8080) to trigger PKG creation and poll progress.

**Note:** The payload cannot be built on Replit — it requires the PS5 Payload SDK on a Linux machine. See `docs/BUILD.md`.

## User preferences

_None recorded yet._
