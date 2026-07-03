// Derive the PS5 payload host from the page URL so this works from any
// device (PC, phone, etc.) — never hardcode localhost.
const API_BASE = `http://${window.location.hostname || '127.0.0.1'}:4242`;

let progressInterval = null;

// ── Status helpers ───────────────────────────────────────────────────────
function showStatus(message, type) {
    const status = document.getElementById('status');
    status.className = 'status-msg ' + type + ' active';
    status.textContent = message;
}

function setDownloadLink(outputPath) {
    const status = document.getElementById('status');
    status.className = 'status-msg success active';
    status.textContent = 'Conversion complete! ';
    const link = document.createElement('a');
    link.href = `${API_BASE}/api/pkg/download?path=${encodeURIComponent(outputPath)}`;
    link.className = 'download-link';
    link.download = '';
    link.textContent = 'Download PKG';
    status.appendChild(link);
}

function hideStatus() {
    const el = document.getElementById('status');
    if (el) el.className = 'status-msg';
}

function updateProgress(percent, statusText, currentFile) {
    document.getElementById('progressFill').style.width   = percent + '%';
    document.getElementById('progressPercent').textContent = percent + '%';
    document.getElementById('progressStatus').textContent  = statusText;
    document.getElementById('currentFile').textContent     = currentFile || '';
}

// ── Auto-detect Content ID ───────────────────────────────────────────────
const dumpPathInput  = document.getElementById('dumpPath');
const contentIdInput = document.getElementById('contentId');
const autoHint       = document.getElementById('autoDetectHint');

function autoDetectContentId(path) {
    if (!path) return;
    if (autoHint) { autoHint.textContent = 'Detecting…'; autoHint.style.color = '#4a8aaa'; }

    fetch(`${API_BASE}/api/sfo/parse?path=${encodeURIComponent(path)}`)
        .then(res => res.json())
        .then(data => {
            if (data.content_id) {
                contentIdInput.value = data.content_id;
                if (autoHint) {
                    autoHint.textContent = '✓ Content ID detected from param.sfo';
                    autoHint.style.color = '#1a5c30';
                }
            } else {
                if (autoHint) {
                    autoHint.textContent = 'Could not detect — enter Content ID manually.';
                    autoHint.style.color = '#8a5500';
                }
            }
        })
        .catch(() => {
            if (autoHint) {
                autoHint.textContent = 'Found in the game\'s param.sfo file.';
                autoHint.style.color = '#4a8aaa';
            }
        });
}

if (dumpPathInput) {
    dumpPathInput.addEventListener('blur', () => {
        autoDetectContentId(dumpPathInput.value.trim());
    });
    dumpPathInput.addEventListener('keydown', e => {
        if (e.key === 'Enter') autoDetectContentId(dumpPathInput.value.trim());
    });
}

// ── Conversion ───────────────────────────────────────────────────────────
function startConversion() {
    const dumpPath  = dumpPathInput.value.trim();
    const contentId = contentIdInput.value.trim();
    const btn = document.getElementById('convertBtn');

    if (!dumpPath || !contentId) {
        showStatus('Please fill in all fields.', 'error');
        return;
    }

    hideStatus();
    document.getElementById('progressContainer').classList.add('active');
    btn.disabled = true;
    updateProgress(0, 'Starting...', '');

    const params = new URLSearchParams({ path: dumpPath, content_id: contentId });

    fetch(`${API_BASE}/api/pkg/create?${params}`)
        .then(res => {
            if (!res.ok) throw new Error(`Server returned HTTP ${res.status}`);
            return res.json();
        })
        .then(data => {
            if (data.error) throw new Error(data.error);
            pollProgress();
        })
        .catch(error => {
            showStatus('Error: ' + error.message, 'error');
            btn.disabled = false;
            document.getElementById('progressContainer').classList.remove('active');
        });
}

function pollProgress() {
    let failureCount = 0;

    const tick = () => {
        fetch(`${API_BASE}/api/pkg/progress`)
            .then(res => {
                if (!res.ok) throw new Error(`HTTP ${res.status}`);
                return res.json();
            })
            .then(data => {
                failureCount = 0;
                updateProgress(
                    data.progress_percent || 0,
                    data.status_text || '',
                    data.current_file
                );

                if (data.status === 4) { // PKG_STATUS_COMPLETE
                    clearInterval(progressInterval);
                    document.getElementById('convertBtn').disabled = false;
                    setDownloadLink(data.output_path);
                } else if (data.status === 5) { // PKG_STATUS_ERROR
                    clearInterval(progressInterval);
                    document.getElementById('convertBtn').disabled = false;
                    showStatus('Error: ' + (data.error || 'Unknown error'), 'error');
                }
            })
            .catch(error => {
                failureCount++;
                console.error(`Progress poll failed (attempt ${failureCount}):`, error);
                if (failureCount >= 5) {
                    clearInterval(progressInterval);
                    document.getElementById('convertBtn').disabled = false;
                    showStatus('Connection lost. Check the payload is still running on your PS5.', 'error');
                }
            });
    };

    progressInterval = setInterval(tick, 1000);
}

// ── Payload connectivity check on load ───────────────────────────────────
fetch(`${API_BASE}/api/status`)
    .then(res => res.json())
    .then(data => console.log('Payload status:', data))
    .catch(() => {
        showStatus('Cannot connect to PKG server. Make sure the payload ELF is loaded on your PS5.', 'info');
    });
