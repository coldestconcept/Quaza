// Derive the PS5 payload host dynamically so this works from any device
// (PC, phone, another PS5) pointed at the same IP — never hardcode localhost.
// When served over HTTPS (e.g. a web preview), the browser blocks plain-HTTP
// fetches as mixed content; detect that case up front so the UI stays usable
// instead of every request silently failing.
const API_HOST = window.location.hostname || '127.0.0.1';
const API_BASE = `http://${API_HOST}:4242`;
const API_REACHABLE = window.location.protocol === 'http:' || window.location.hostname === '127.0.0.1' || window.location.hostname === 'localhost';

// Check payload connection on page load
if (API_REACHABLE) {
    fetch(`${API_BASE}/api/status`)
        .then(res => res.json())
        .then(data => console.log('Payload connected:', data))
        .catch(() => console.warn('Payload not reachable — ensure the ELF is loaded on your PS5.'));
} else {
    console.warn('Preview context: payload API is not reachable from this origin. UI is view-only.');
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

    fetch(`${API_BASE}/api/sfo/parse?path=${encodeURIComponent(path)}`)
        .then(res => res.json())
        .then(data => {
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
        })
        .catch(() => {
            if (autoDetectHint) {
                autoDetectHint.textContent = 'Found in the game\'s param.sfo file.';
                autoDetectHint.style.color = '#4a8aaa';
            }
        });
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

// ── Conversion ───────────────────────────────────────────────────────────
document.getElementById('convertBtn').addEventListener('click', () => {
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

    fetch(`${API_BASE}/api/pkg/create?${params}`)
        .then(res => {
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            return res.json();
        })
        .then(data => {
            if (data.error) throw new Error(data.error);
            statusTxt.textContent = 'PKG creation started — monitoring progress...';
            progressBar.style.width = '5%';
            pollProgress();
        })
        .catch(err => {
            statusTxt.textContent = 'Failed to reach Quaza payload. Make sure the ELF server is active on your PS5.';
            progressBar.style.backgroundColor = '#ff4444';
            document.getElementById('convertBtn').disabled = false;
            console.error('Conversion start error:', err);
        });
});

function pollProgress() {
    const statusTxt   = document.getElementById('statusTxt');
    const progressBar = document.getElementById('progressBar');
    let failureCount  = 0;

    const monitor = setInterval(() => {
        fetch(`${API_BASE}/api/pkg/progress`)
            .then(res => {
                if (!res.ok) throw new Error(`HTTP ${res.status}`);
                return res.json();
            })
            .then(data => {
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
                        const dlUrl = `${API_BASE}/api/pkg/download?path=${encodeURIComponent(data.output_path)}`;
                        const link  = document.createElement('a');
                        link.href      = dlUrl;
                        link.download  = '';
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
            })
            .catch(err => {
                failureCount++;
                statusTxt.textContent = `Reconnecting to payload... (attempt ${failureCount}/5)`;
                if (failureCount >= 5) {
                    clearInterval(monitor);
                    document.getElementById('convertBtn').disabled = false;
                    statusTxt.textContent = 'Connection lost. Check the payload is still running on your PS5.';
                    progressBar.style.backgroundColor = '#ff4444';
                    console.error('Polling failed:', err);
                }
            });
    }, 1000);
}

function statusLabel(status) {
    const labels = { 0:'Idle', 1:'Scanning files...', 2:'Building PFS image...', 3:'Building PKG...', 4:'Complete', 5:'Error' };
    return labels[status] || `Processing (${status})`;
}
