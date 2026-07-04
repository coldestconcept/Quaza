// Folder picker — a small file-manager-style modal for choosing the
// decrypted title directory, instead of typing the path by hand.
// Talks to the payload's /api/browse endpoint (or demo.js's mock in
// demo mode) to list real PS5 mount points and directories.
(function (window) {
    'use strict';

    const API_HOST = window.location.hostname || '127.0.0.1';
    const API_BASE = `http://${API_HOST}:4242`;

    const overlay        = document.getElementById('browserOverlay');
    const listEl          = document.getElementById('browserList');
    const breadcrumbEl    = document.getElementById('browserBreadcrumb');
    const upBtn            = document.getElementById('browserUpBtn');
    const closeBtn         = document.getElementById('browserCloseBtn');
    const cancelBtn         = document.getElementById('browserCancelBtn');
    const selectBtn          = document.getElementById('browserSelectBtn');
    const selectedLabelEl  = document.getElementById('browserSelectedLabel');
    const browseBtn         = document.getElementById('browseBtn');
    const dumpPathInput    = document.getElementById('dumpPath');

    if (!overlay || !browseBtn) return; // markup not present on this page

    const FOLDER_ICON =
        '<svg viewBox="0 0 24 24" width="18" height="18" fill="none" xmlns="http://www.w3.org/2000/svg">' +
        '<path d="M3 6.5C3 5.67 3.67 5 4.5 5H9.17C9.59 5 10.03 5.18 10.33 5.5L11.5 7H19.5C20.33 7 21 7.67 21 8.5V17.5C21 18.33 20.33 19 19.5 19H4.5C3.67 19 3 18.33 3 17.5V6.5Z" ' +
        'fill="currentColor" fill-opacity="0.18" stroke="currentColor" stroke-width="1.6" stroke-linejoin="round"/></svg>';

    const FILE_ICON =
        '<svg viewBox="0 0 24 24" width="18" height="18" fill="none" xmlns="http://www.w3.org/2000/svg">' +
        '<path d="M6 3.5C6 3.22 6.22 3 6.5 3H13L18 8V20.5C18 20.78 17.78 21 17.5 21H6.5C6.22 21 6 20.78 6 20.5V3.5Z" ' +
        'stroke="currentColor" stroke-width="1.5" stroke-linejoin="round"/><path d="M13 3V8H18" stroke="currentColor" stroke-width="1.5" stroke-linejoin="round"/></svg>';

    let currentPath   = null;   // path currently displayed (null = root shortcuts)
    let currentParent = null;   // parent of currentPath, or null if at root shortcuts
    let selectedPath  = null;   // the highlighted folder's full path (or null)

    function joinPath(base, name) {
        if (base === null || base === '') return name; // root shortcuts already give absolute paths
        return base.endsWith('/') ? base + name : `${base}/${name}`;
    }

    function setSelected(path) {
        selectedPath = path;
        selectedLabelEl.textContent = path || '';
        selectBtn.disabled = !path;
    }

    function renderBreadcrumb() {
        breadcrumbEl.textContent = currentPath === null ? 'Storage Devices' : currentPath;
        upBtn.disabled = currentPath === null;
    }

    function renderEntries(entries) {
        listEl.innerHTML = '';

        if (!entries || entries.length === 0) {
            listEl.innerHTML = '<div class="browser-empty">This folder is empty.</div>';
            return;
        }

        // Directories first, then files, alphabetically within each group.
        const dirs  = entries.filter(e => e.type === 'dir').sort((a, b) => a.name.localeCompare(b.name));
        const files = entries.filter(e => e.type !== 'dir').sort((a, b) => a.name.localeCompare(b.name));

        [...dirs, ...files].forEach(entry => {
            const row = document.createElement('div');
            const isDir = entry.type === 'dir';
            const fullPath = joinPath(currentPath, entry.name);
            const displayName = currentPath === null ? entry.name : entry.name;

            row.className = 'browser-row' + (isDir ? '' : ' disabled');
            row.innerHTML =
                `<span class="browser-row-icon${isDir ? '' : ' file'}">${isDir ? FOLDER_ICON : FILE_ICON}</span>` +
                `<span class="browser-row-name">${escapeHtml(displayName)}</span>` +
                (isDir ? '<span class="browser-row-chevron">▸</span>' : '');

            if (isDir) {
                row.addEventListener('click', () => {
                    // Single click: highlight/select this folder as the candidate.
                    listEl.querySelectorAll('.browser-row.selected').forEach(r => r.classList.remove('selected'));
                    row.classList.add('selected');
                    setSelected(fullPath);
                });
                row.addEventListener('dblclick', () => {
                    loadPath(fullPath);
                });
            }

            listEl.appendChild(row);
        });
    }

    function escapeHtml(s) {
        const div = document.createElement('div');
        div.textContent = s;
        return div.innerHTML;
    }

    function fetchBrowse(path) {
        const demo = window.QuazaDemo;
        if (demo && demo.isDemoMode()) {
            return demo.browse(path || '');
        }
        const q = path ? `?path=${encodeURIComponent(path)}` : '';
        return fetch(`${API_BASE}/api/browse${q}`).then(res => {
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            return res.json();
        });
    }

    function loadPath(path) {
        listEl.innerHTML = '<div class="browser-loading">Loading…</div>';
        setSelected(null);

        fetchBrowse(path).then(data => {
            currentPath   = data.path || null;
            currentParent = (data.parent === null || data.parent === undefined) ? null : data.parent;
            renderBreadcrumb();
            renderEntries(data.entries || []);
        }).catch(err => {
            listEl.innerHTML = `<div class="browser-empty">Could not read this folder.<br>${escapeHtml(err.message || '')}</div>`;
            console.error('[Quaza] browse failed:', err);
        });
    }

    function openBrowser() {
        overlay.classList.add('open');
        loadPath(dumpPathInput && dumpPathInput.value.trim() ? null : null);
    }

    function closeBrowser() {
        overlay.classList.remove('open');
    }

    browseBtn.addEventListener('click', openBrowser);
    closeBtn.addEventListener('click', closeBrowser);
    cancelBtn.addEventListener('click', closeBrowser);

    upBtn.addEventListener('click', () => {
        if (currentPath === null) return;
        loadPath(currentParent || null);
    });

    selectBtn.addEventListener('click', () => {
        if (!selectedPath) return;
        dumpPathInput.value = selectedPath;
        closeBrowser();
        // Reuse the existing auto-detect flow (blur handler) for the Content ID.
        dumpPathInput.dispatchEvent(new Event('blur'));
    });

    overlay.addEventListener('click', (e) => {
        if (e.target === overlay) closeBrowser();
    });
})(window);
