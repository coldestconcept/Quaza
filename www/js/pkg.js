const API_BASE = 'http://localhost:8080';
let progressInterval = null;

function showStatus(message, type) {
    const status = document.getElementById('status');
    status.className = 'status ' + type + ' active';
    status.textContent = message;
}

function hideStatus() {
    document.getElementById('status').className = 'status';
}

function updateProgress(percent, status, currentFile) {
    document.getElementById('progressFill').style.width = percent + '%';
    document.getElementById('progressPercent').textContent = percent + '%';
    document.getElementById('progressStatus').textContent = status;
    document.getElementById('currentFile').textContent = currentFile || '';
}

function startConversion() {
    const dumpPath = document.getElementById('dumpPath').value;
    const contentId = document.getElementById('contentId').value;
    const btn = document.getElementById('convertBtn');
    
    if (!dumpPath || !contentId) {
        showStatus('Please fill in all fields', 'error');
        return;
    }
    
    // Reset UI
    hideStatus();
    document.getElementById('progressContainer').classList.add('active');
    btn.disabled = true;
    updateProgress(0, 'Starting...', '');
    
    // Start conversion
    fetch(`${API_BASE}/api/pkg/create?path=${encodeURIComponent(dumpPath)}&content_id=${encodeURIComponent(contentId)}`)
        .then(response => response.json())
        .then(data => {
            if (data.error) {
                throw new Error(data.error);
            }
            
            // Poll for progress
            progressInterval = setInterval(pollProgress, 1000);
        })
        .catch(error => {
            showStatus('Error: ' + error.message, 'error');
            btn.disabled = false;
            document.getElementById('progressContainer').classList.remove('active');
        });
}

function pollProgress() {
    fetch(`${API_BASE}/api/pkg/progress`)
        .then(response => response.json())
        .then(data => {
            updateProgress(
                data.progress_percent,
                data.status_text,
                data.current_file
            );
            
            if (data.status === 4) { // COMPLETE
                clearInterval(progressInterval);
                document.getElementById('convertBtn').disabled = false;
                showStatus(
                    'Conversion complete! ' +
                    `<a href="${API_BASE}/api/pkg/download?path=${encodeURIComponent(data.output_path)}" ` +
                    `class="download-link" download>Download PKG</a>`,
                    'success'
                );
            } else if (data.status === 5) { // ERROR
                clearInterval(progressInterval);
                document.getElementById('convertBtn').disabled = false;
                showStatus('Error: ' + data.error, 'error');
            }
        })
        .catch(error => {
            console.error('Progress poll failed:', error);
        });
}

// Check server status on load
fetch(`${API_BASE}/api/status`)
    .then(response => response.json())
    .then(data => {
        console.log('Server status:', data);
    })
    .catch(error => {
        showStatus('Warning: Cannot connect to PKG server. Make sure the payload is loaded.', 'info');
    });
