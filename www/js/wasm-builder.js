// Local ("This Device") PKG conversion — runs the exact same PKG-building C
// engine that powers the PS5 payload, compiled to WebAssembly, so a PC or
// Android browser can convert a selected game folder into a real .pkg file
// with no PS5 or payload connection required at all.
(function (window) {
    'use strict';

    let modulePromise = null;

    function loadScript(src) {
        return new Promise((resolve, reject) => {
            const s = document.createElement('script');
            s.src = src;
            s.onload = resolve;
            s.onerror = () => reject(new Error(`Failed to load ${src}`));
            document.head.appendChild(s);
        });
    }

    // Loads wasm/prosperopkg.js (Emscripten MODULARIZE output) once and caches
    // the initialized Module instance for reuse across multiple conversions.
    function loadModule() {
        if (modulePromise) return modulePromise;
        modulePromise = loadScript('wasm/prosperopkg.js')
            .then(() => {
                if (typeof window.ProsperoPkgModule !== 'function') {
                    throw new Error('prosperopkg.js loaded but ProsperoPkgModule is missing');
                }
                return window.ProsperoPkgModule();
            })
            .catch(err => {
                modulePromise = null; // allow retry on next call
                throw err;
            });
        return modulePromise;
    }

    // Writes every File from a webkitdirectory-selected FileList into the
    // module's MEMFS under `rootPath`, preserving the folder structure but
    // stripping the top-level selected-folder name (so rootPath itself lines
    // up with the game's own root, e.g. rootPath/sce_sys/param.sfo).
    async function writeFilesToFS(Module, files, rootPath, onFile) {
        const madeDirs = new Set([rootPath]);
        Module.FS.mkdirTree(rootPath);

        for (const file of Array.from(files)) {
            const rel = file.webkitRelativePath || file.name;
            const parts = rel.split('/').filter(Boolean);
            parts.shift(); // drop the selected root folder's own name
            if (parts.length === 0) continue;

            const destPath = rootPath + '/' + parts.join('/');
            const dirPath = rootPath + '/' + parts.slice(0, -1).join('/');

            if (dirPath !== rootPath && !madeDirs.has(dirPath)) {
                Module.FS.mkdirTree(dirPath);
                madeDirs.add(dirPath);
            }

            const bytes = new Uint8Array(await file.arrayBuffer());
            Module.FS.writeFile(destPath, bytes);
            if (onFile) onFile(rel, bytes.length);
        }
    }

    // Builds a PKG from a webkitdirectory FileList and returns the raw bytes
    // of the resulting .pkg file (a Uint8Array). Throws on failure.
    async function buildPkgFromFiles(files, contentId, opts) {
        opts = opts || {};
        const onStage = opts.onStage || function () {};

        onStage('Loading conversion engine (WebAssembly)…');
        const Module = await loadModule();

        const inputDir = '/input';
        const outputPath = '/output.pkg';

        // Fresh MEMFS scratch space per build.
        try { Module.FS.unmount('/'); } catch (e) { /* not mounted, fine */ }
        try {
            const st = Module.FS.stat(inputDir);
            if (st) Module.FS.unlink(inputDir);
        } catch (e) { /* doesn't exist yet, fine */ }

        onStage('Reading selected folder into memory…');
        let totalBytes = 0;
        await writeFilesToFS(Module, files, inputDir, (relPath, size) => {
            totalBytes += size;
            onStage(`Loaded ${relPath}`);
        });

        if (totalBytes === 0) {
            throw new Error('No files were read from the selected folder.');
        }

        onStage('Building PKG (this can take a while for large titles)…');
        const ret = Module.ccall(
            'wasm_pkg_build',
            'number',
            ['string', 'string', 'string'],
            [contentId, inputDir, outputPath]
        );

        if (ret !== 0) {
            throw new Error(`PKG build failed (engine error code ${ret}).`);
        }

        onStage('Reading finished PKG…');
        const data = Module.FS.readFile(outputPath);
        return data;
    }

    // Triggers a normal browser download for the given bytes.
    function downloadBytes(bytes, filename) {
        const blob = new Blob([bytes], { type: 'application/octet-stream' });
        const url = URL.createObjectURL(blob);
        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        document.body.appendChild(link);
        link.click();
        link.remove();
        setTimeout(() => URL.revokeObjectURL(url), 10000);
    }

    window.QuazaWasm = { loadModule, buildPkgFromFiles, downloadBytes };
})(window);
