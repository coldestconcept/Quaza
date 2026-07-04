// Minimal client-side PS param.sfo parser — used only in "This Device" (WASM)
// mode, where there's no PS5 payload to ask for the CONTENT_ID server-side.
// SFO format: 20-byte header, then an index table, a key table, a data table.
// We only care about pulling out the CONTENT_ID string.
(function (window) {
    'use strict';

    function parseParamSfo(buffer) {
        const view = new DataView(buffer);
        if (view.byteLength < 20) return null;

        const magic = view.getUint32(0, true);
        if (magic !== 0x46535000) return null; // "\0PSF"

        const keyTableStart  = view.getUint32(8, true);
        const dataTableStart = view.getUint32(12, true);
        const entryCount     = view.getUint32(16, true);

        const bytes = new Uint8Array(buffer);
        const result = {};

        for (let i = 0; i < entryCount; i++) {
            const entryOffset = 20 + i * 16;
            if (entryOffset + 16 > view.byteLength) break;

            const keyOffset = view.getUint16(entryOffset, true);
            const dataFmt   = view.getUint16(entryOffset + 2, true);
            const dataLen   = view.getUint32(entryOffset + 4, true);
            const dataOffset = view.getUint32(entryOffset + 12, true);

            // Read null-terminated key string
            let keyStart = keyTableStart + keyOffset;
            let keyEnd = keyStart;
            while (keyEnd < bytes.length && bytes[keyEnd] !== 0) keyEnd++;
            const key = new TextDecoder('ascii').decode(bytes.slice(keyStart, keyEnd));

            const valStart = dataTableStart + dataOffset;
            const valEnd = Math.min(valStart + dataLen, bytes.length);
            const raw = bytes.slice(valStart, valEnd);

            if (dataFmt === 0x0204 || dataFmt === 0x0004) {
                // UTF-8 string (possibly null-terminated within the field)
                let strEnd = raw.indexOf(0);
                if (strEnd === -1) strEnd = raw.length;
                result[key] = new TextDecoder('utf-8').decode(raw.slice(0, strEnd));
            } else if (dataFmt === 0x0404 && raw.length >= 4) {
                result[key] = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true);
            }
        }

        return result;
    }

    // Given a FileList/array of File objects (from a webkitdirectory input),
    // find sce_sys/param.sfo (relative to the selected root) and return the
    // parsed CONTENT_ID, or null if not found/parseable.
    async function detectContentIdFromFiles(files) {
        const sfoFile = Array.from(files).find(f =>
            /(^|\/)sce_sys\/param\.sfo$/i.test(f.webkitRelativePath || f.name)
        );
        if (!sfoFile) return null;

        try {
            const buf = await sfoFile.arrayBuffer();
            const parsed = parseParamSfo(buf);
            return (parsed && parsed.CONTENT_ID) ? parsed.CONTENT_ID : null;
        } catch (e) {
            console.warn('[Quaza] param.sfo parse failed:', e);
            return null;
        }
    }

    window.QuazaSfo = { parseParamSfo, detectContentIdFromFiles };
})(window);
