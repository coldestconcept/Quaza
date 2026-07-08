# Quaza — PS5 PKG Tool

Browser-based game dump → PKG converter for jailbroken PS5.

## Project overview

| Layer | What it is |
|-------|-----------|
| `www/` | Web UI — plain HTML + JS + pre-compiled WASM (`prosperopkg.wasm`). Served via `server.js`. |
| `payload/` | C ELF payload that runs **on the PS5** as an HTTP server (port 4242). Requires the PS5 Payload SDK to build — not buildable on Replit. |
| `python/scraper.py` | Scrapes backport data from prosperopatches.com. |
| `docs/` | Build, install, and PKG documentation. |

## Running on Replit

The **web UI** is served as a static site:

```bash
node server.js
```

This starts a Node HTTP server on port 5000 that serves `www/`. The UI enters demo mode automatically when the PS5 payload (port 4242) is unreachable.

## Building the PS5 payload (not on Replit)

The C payload requires:
- PS5 Payload SDK (`/opt/ps5-payload-sdk`)
- CMake 3.20+
- Linux build environment

See `docs/BUILD.md` and `docs/INSTALL.md` for full instructions.

## User preferences

- Keep existing project structure; do not restructure or migrate.
- Push changes to GitHub (origin/main) after every set of changes.
