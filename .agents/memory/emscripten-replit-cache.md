---
name: Emscripten build cache on Replit/Nix
description: How to get emcc to actually build on Replit when its default cache dir is inside the read-only Nix store
---

Emscripten (installed via Nix on Replit) keeps its default cache — the pre-built
sysroot headers, system libs, and symbol_lists — inside the read-only Nix store.
Any real build needs to write at least one missing system-lib variant or a
symbol_lists json, which fails there. Copying/mirroring the whole cache tree
to a writable dir also fails: many source dirs/files inside the Nix store cache
are `dr-xr-xr-x`, so `cp`/`rsync` recreate them read-only in the destination too,
and re-chmodding piecemeal turns into an unwinnable whack-a-mole (each retry
just reveals the next read-only file).

**Why:** wasted a long debugging loop trying to `rm -rf` / `cp --no-preserve=mode`
/ chmod-loop a full mirror of the Nix cache before finding the actual fix.

**How to apply:** build a small writable *overlay* cache instead of a full copy:
- Point `EM_CACHE` at a fresh writable dir (e.g. under `/tmp`).
- Symlink `$EM_CACHE/sysroot` straight at the Nix store's prebuilt
  `share/emscripten/cache/sysroot` (read-only access is all compiling needs).
- Copy just `sysroot_install.stamp` and the small `symbol_lists/*.json` files
  into the writable overlay (so `ensure_sysroot()` doesn't try to reinstall headers).
- Create a real writable `build/` dir alongside it.
- Compile with `-g -O0` so emcc selects the `*-debug.a` prebuilt system-lib
  variants (Nix ships debug variants but not always the release ones), which
  avoids needing to compile+write any new system lib into the cache at all.
- See `payload/libprosperopkg/wasm/build_wasm.sh` in the Quaza project for the
  working implementation of this pattern.
