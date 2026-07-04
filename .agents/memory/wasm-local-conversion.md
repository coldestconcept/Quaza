---
name: WASM local PKG conversion architecture
description: How Quaza's browser-only (no PS5 payload) PKG conversion mode is wired, for consistency if extended.
---

The frontend has two conversion modes toggled by tabs: "PS5 Remote" (talks to the real payload over HTTP) and
"This Device" (runs entirely in-browser via WebAssembly, no PS5 needed).

Local mode reuses the exact same portable C PKG-building code as the PS5 payload (compiled to WASM via
`payload/libprosperopkg/wasm/build_wasm.sh`) — never reimplement PKG-building logic in JS. The WASM module exposes
one function, `wasm_pkg_build(content_id, input_dir, output_path)`, operating purely on Emscripten's MEMFS.

**Why:** Single source of truth between native payload and browser build avoids logic drift; MEMFS lets the same
disk-path-based C API work unchanged in a browser sandbox.

**How to apply:** `www/js/wasm-builder.js` loads the module once (cached promise) and writes a user's
`webkitdirectory`-selected FileList into MEMFS, stripping the top-level folder name so paths line up with the game
root. `www/js/sfo-parser.js` does client-side PS param.sfo parsing (no server) to auto-detect CONTENT_ID in local
mode, since there's no payload to ask. Mode auto-suggests "This Device" using the existing `window.QUAZA_DEMO`
detection flag from `demo.js` (already flags when no real payload is reachable) — reuse that flag rather than
adding a second detection mechanism. Known limitation: large game dumps loaded fully into MEMFS can use significant
browser RAM; `webkitdirectory` doesn't work on iOS Safari.
