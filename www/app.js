// Derive the PS5 payload host dynamically so this works from any device
// (PC, phone, another PS5) pointed at the same IP — never hardcode localhost.
// When the payload is unreachable (HTTPS preview, or no ELF loaded), demo.js
// activates and these calls fall back to simulated data so the UI is fully
// interactive on any PC.
const API_HOST = window.location.hostname || '127.0.0.1';
const API_BASE = `http://${API_HOST}:4242`;
const demo = window.QuazaDemo;

// Check payload connection on page load
if (demo && demo.isDemoMode()) {
    demo.status().then(data => console.log('Demo payload connected:', data));
} else {
    fetch(`${API_BASE}/api/status`)
        .then(res => res.json())
        .then(data => console.log('Payload connected:', data))
        .catch(() => console.warn('Payload not reachable — ensure the ELF is loaded on your PS5.'));
}

// ── Auto-detect Content ID from dump path ────────────────────────────────
// When the user finishes typing a dump path, query the payload for the
// CONTENT_ID from that folder's sce_sys/param.sfo automatically.
const dumpPathInput   = document.getElementById('dumpPath');
const contentIdInput  = document.getElementById('contentId');
const autoDetectHint  = document.getElementById('autoDetectHint');

function autoDetectContentId(path) {
    if (!path) return;
    if (autoDetectHint) {
        autoDetectHint.textContent = 'Detecting…';
        autoDetectHint.style.color = '#4a8aaa';
    }

    const detect = (data) => {
        if (data.content_id) {
            contentIdInput.value = data.content_id;
            if (autoDetectHint) {
                autoDetectHint.textContent = '✓ Content ID detected from param.sfo';
                autoDetectHint.style.color = '#1a7a40';
            }
        } else {
            if (autoDetectHint) {
                autoDetectHint.textContent = 'Could not detect — enter Content ID manually.';
                autoDetectHint.style.color = '#8a5500';
            }
        }
    };

    if (demo && demo.isDemoMode()) {
        demo.sfoParse(path).then(detect).catch(() => {
            if (autoDetectHint) {
                autoDetectHint.textContent = 'Found in the game\'s param.sfo file.';
                autoDetectHint.style.color = '#4a8aaa';
            }
        });
    } else {
        fetch(`${API_BASE}/api/sfo/parse?path=${encodeURIComponent(path)}`)
            .then(res => res.json())
            .then(detect)
            .catch(() => {
                if (autoDetectHint) {
                    autoDetectHint.textContent = 'Found in the game\'s param.sfo file.';
                    autoDetectHint.style.color = '#4a8aaa';
                }
            });
    }
}

if (dumpPathInput) {
    dumpPathInput.addEventListener('blur', () => {
        autoDetectContentId(dumpPathInput.value.trim());
    });
    // Also trigger on Enter key
    dumpPathInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') autoDetectContentId(dumpPathInput.value.trim());
    });
}

// ── Mode switching: PS5 Remote vs This Device (local WASM build) ────────
let currentMode = 'remote';
let localFiles = null;

const modeTabRemote  = document.getElementById('modeTabRemote');
const modeTabLocal   = document.getElementById('modeTabLocal');
const modeHint       = document.getElementById('modeHint');
const remoteFields    = document.getElementById('remoteFields');
const localFields     = document.getElementById('localFields');
const backportRow     = document.getElementById('backportRow');
const localFolderInput = document.getElementById('localFolderInput');
const localBrowseBtn  = document.getElementById('localBrowseBtn');
const localFolderHint = document.getElementById('localFolderHint');
const convertBtnEl    = document.getElementById('convertBtn');

function setMode(mode) {
    currentMode = mode;
    modeTabRemote.classList.toggle('active', mode === 'remote');
    modeTabLocal.classList.toggle('active', mode === 'local');
    remoteFields.style.display = mode === 'remote' ? '' : 'none';
    localFields.style.display  = mode === 'local' ? '' : 'none';
    backportRow.style.display  = mode === 'remote' ? '' : 'none';
    convertBtnEl.textContent = mode === 'remote'
        ? 'Compile & Deploy Native PKG'
        : 'Convert on This Device';
}

if (modeTabRemote && modeTabLocal) {
    modeTabRemote.addEventListener('click', () => setMode('remote'));
    modeTabLocal.addEventListener('click', () => setMode('local'));
}

if (localBrowseBtn && localFolderInput) {
    localBrowseBtn.addEventListener('click', () => localFolderInput.click());
    localFolderInput.addEventListener('change', async () => {
        localFiles = localFolderInput.files;
        if (!localFiles || localFiles.length === 0) {
            localFolderHint.textContent = 'No folder selected.';
            return;
        }
        const rootName = (localFiles[0].webkitRelativePath || '').split('/')[0] || 'folder';
        localFolderHint.textContent = `${rootName} — ${localFiles.length} files selected.`;

        if (autoDetectHint) {
            autoDetectHint.textContent = 'Detecting…';
            autoDetectHint.style.color = '#4a8aaa';
        }
        try {
            const detected = window.QuazaSfo && await window.QuazaSfo.detectContentIdFromFiles(localFiles);
            if (detected) {
                contentIdInput.value = detected;
                if (autoDetectHint) {
                    autoDetectHint.textContent = '✓ Content ID detected from param.sfo';
                    autoDetectHint.style.color = '#1a7a40';
                }
            } else if (autoDetectHint) {
                autoDetectHint.textContent = 'Could not detect — enter Content ID manually.';
                autoDetectHint.style.color = '#8a5500';
            }
        } catch (e) {
            console.warn('[Quaza] local content-id detection failed:', e);
        }
    });
}

// If no real PS5 payload is reachable, nudge the user toward local
// conversion automatically (still lets them switch back manually).
if (demo && demo.isDemoMode() && modeTabLocal) {
    setTimeout(() => {
        if (window.QUAZA_DEMO === true) {
            modeHint.style.display = 'block';
            modeHint.textContent = 'No PS5 payload detected — try "This Device" to convert locally in your browser, no PS5 required.';
        }
    }, 400);
}

function runLocalConversion() {
    const statusBox   = document.getElementById('status-box');
    const statusTxt    = document.getElementById('statusTxt');
    const progressBar  = document.getElementById('progressBar');
    const contentId    = contentIdInput.value.trim();

    if (!localFiles || localFiles.length === 0) {
        alert('Please choose a game dump folder first.');
        return;
    }
    if (!contentId) {
        alert('Please enter the Content ID (or select a folder to auto-detect).');
        return;
    }

    statusBox.style.display  = 'block';
    statusBox.style.borderLeftColor = 'var(--primary)';
    progressBar.style.width  = '30%';
    progressBar.style.backgroundColor = 'var(--primary)';
    convertBtnEl.disabled = true;

    window.QuazaWasm.buildPkgFromFiles(localFiles, contentId, {
        onStage: (msg) => { statusTxt.textContent = msg; }
    }).then((bytes) => {
        progressBar.style.width = '100%';
        statusTxt.textContent = 'PKG built successfully on this device!';
        window.QuazaWasm.downloadBytes(bytes, `${contentId}.pkg`);
        convertBtnEl.disabled = false;
    }).catch((err) => {
        console.error('Local build failed:', err);
        statusTxt.textContent = `Local build failed: ${err.message || err}`;
        progressBar.style.backgroundColor = '#ff4444';
        convertBtnEl.disabled = false;
    });
}

// ── Conversion ───────────────────────────────────────────────────────────
document.getElementById('convertBtn').addEventListener('click', () => {
    if (currentMode === 'local') {
        runLocalConversion();
        return;
    }
    const dumpPath  = dumpPathInput.value.trim();
    const contentId = contentIdInput.value.trim();
    const enableBackport = document.getElementById('enableBackport').checked;
    const statusBox  = document.getElementById('status-box');
    const statusTxt  = document.getElementById('statusTxt');
    const progressBar = document.getElementById('progressBar');

    if (!dumpPath) {
        alert('Please enter the game dump path.');
        return;
    }
    if (!contentId) {
        alert('Please enter the Content ID (or tab out of the path field to auto-detect).');
        return;
    }

    // Reset UI
    statusBox.style.display  = 'block';
    statusBox.style.borderLeftColor = 'var(--primary)';
    statusTxt.textContent    = 'Sending parameters to PS5 payload...';
    progressBar.style.width  = '0%';
    progressBar.style.backgroundColor = 'var(--primary)';
    document.getElementById('convertBtn').disabled = true;

    const params = new URLSearchParams({
        path: dumpPath,
        content_id: contentId,
        ...(enableBackport && { backport: 'true' })
    });

    const onStart = () => {
        statusTxt.textContent = 'PKG creation started — monitoring progress...';
        progressBar.style.width = '5%';
        pollProgress();
    };
    const onFail = (err) => {
        statusTxt.textContent = 'Failed to reach Quaza payload. Make sure the ELF server is active on your PS5.';
        progressBar.style.backgroundColor = '#ff4444';
        document.getElementById('convertBtn').disabled = false;
        console.error('Conversion start error:', err);
    };

    if (demo && demo.isDemoMode()) {
        demo.pkgCreate(params).then(onStart).catch(onFail);
    } else {
        fetch(`${API_BASE}/api/pkg/create?${params}`)
            .then(res => {
                if (!res.ok) throw new Error(`HTTP ${res.status}`);
                return res.json();
            })
            .then(data => {
                if (data.error) throw new Error(data.error);
                onStart();
            })
            .catch(onFail);
    }
});

function pollProgress() {
    const statusTxt   = document.getElementById('statusTxt');
    const progressBar = document.getElementById('progressBar');
    let failureCount  = 0;

    const monitor = setInterval(() => {
        const handleData = (data) => {
            failureCount = 0;
            progressBar.style.backgroundColor = 'var(--primary)';

            const pct = data.progress_percent || 0;
            progressBar.style.width = `${pct}%`;

            if (data.status === 4) { // PKG_STATUS_COMPLETE
                clearInterval(monitor);
                document.getElementById('convertBtn').disabled = false;
                progressBar.style.width = '100%';
                statusTxt.textContent = 'PKG compiled successfully!';

                if (data.output_path) {
                    const dlUrl = (demo && demo.isDemoMode())
                        ? demo.downloadUrl()
                        : `${API_BASE}/api/pkg/download?path=${encodeURIComponent(data.output_path)}`;
                    const link  = document.createElement('a');
                    link.href      = dlUrl;
                    link.download  = data.output_path.split('/').pop() || 'output.pkg';
                    link.textContent = '⬇ Download PKG';
                    link.style.cssText = 'display:inline-block;margin-top:10px;padding:8px 16px;background:var(--primary);color:#fff;border-radius:6px;text-decoration:none;font-weight:bold;';
                    statusTxt.appendChild(document.createElement('br'));
                    statusTxt.appendChild(link);
                }
            } else if (data.status === 5) { // PKG_STATUS_ERROR
                clearInterval(monitor);
                document.getElementById('convertBtn').disabled = false;
                statusTxt.textContent = `Build failed: ${data.error || 'Unknown error'}`;
                progressBar.style.backgroundColor = '#ff4444';
            } else {
                const label = data.status_text || statusLabel(data.status);
                const file  = data.current_file ? ` — ${data.current_file}` : '';
                statusTxt.textContent = `${label}${file} (${pct}%)`;
            }
        };
        const handleErr = (err) => {
            failureCount++;
            statusTxt.textContent = `Reconnecting to payload... (attempt ${failureCount}/5)`;
            if (failureCount >= 5) {
                clearInterval(monitor);
                document.getElementById('convertBtn').disabled = false;
                statusTxt.textContent = 'Connection lost. Check the payload is still running on your PS5.';
                progressBar.style.backgroundColor = '#ff4444';
                console.error('Polling failed:', err);
            }
        };

        if (demo && demo.isDemoMode()) {
            demo.pkgProgress().then(handleData).catch(handleErr);
        } else {
            fetch(`${API_BASE}/api/pkg/progress`)
                .then(res => {
                    if (!res.ok) throw new Error(`HTTP ${res.status}`);
                    return res.json();
                })
                .then(handleData)
                .catch(handleErr);
        }
    }, 1000);
}

function statusLabel(status) {
    const labels = { 0:'Idle', 1:'Scanning files...', 2:'Building PFS image...', 3:'Building PKG...', 4:'Complete', 5:'Error' };
    return labels[status] || `Processing (${status})`;
}
