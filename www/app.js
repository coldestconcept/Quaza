document.getElementById('convertBtn').addEventListener('click', () => {
    const dumpPath = document.getElementById('dumpPath').value.trim();
    const enableBackport = document.getElementById('enableBackport').checked;
    const statusBox = document.getElementById('status-box');
    const statusTxt = document.getElementById('statusTxt');
    const progressBar = document.getElementById('progressBar');

    // Reset interface elements
    if (!dumpPath) {
        alert('Please enter a target path directory.');
        return;
    }

    statusBox.style.display = 'block';
    statusTxt.innerText = 'Sending parameters to PS5 server runtime...';
    progressBar.style.width = '0%';

    // Detect target host dynamically relative to where the site is served
    const host = window.location.hostname || '127.0.0.1';
    
    // Construct request configuration payload
    const requestData = {
        path: dumpPath,
        backport: enableBackport
    };

    fetch(`http://${host}:8080/api/convert`, {
        method: 'POST',
        headers: { 
            'Content-Type': 'application/json' 
        },
        body: JSON.stringify(requestData)
    })
    .then(response => {
        if (!response.ok) {
            throw new Error(`Server returned HTTP status ${response.status}`);
        }
        return response.json();
    })
    .then(data => {
        statusTxt.innerText = data.message || 'PKG generation task spawned successfully.';
        progressBar.style.width = '5%';
        // Handover execution context to the tracking loop
        pollConversionStatus(host);
    })
    .catch(err => {
        statusTxt.innerText = 'Error: Failed to reach Quaza payload. Ensure the background ELF server is active.';
        progressBar.style.backgroundColor = '#ff4444';
        console.error('API Handshake Failure:', err);
    });
});

function pollConversionStatus(host) {
    const statusTxt = document.getElementById('statusTxt');
    const progressBar = document.getElementById('progressBar');
    let failureCount = 0;
    
    // Establish interval frequency matching task complexity
    const taskMonitor = setInterval(() => {
        fetch(`http://${host}:8080/api/status`)
        .then(res => {
            if (!res.ok) throw new Error('Unhealthy status endpoint response.');
            return res.json();
        })
        .then(data => {
            failureCount = 0; // Reset network safety drops counter
            progressBar.style.backgroundColor = 'var(--primary)'; // Restore state color variables

            if (data.status === 'processing') {
                const progressVal = data.progress || 0;
                statusTxt.innerText = `Compiling PFS/PKG structures... (${progressVal}%)`;
                progressBar.style.width = `${progressVal}%`;
            } 
            else if (data.status === 'completed') {
                statusTxt.innerText = 'Success! Retail PKG package has been compiled natively.';
                progressBar.style.width = '100%';
                clearInterval(taskMonitor);
            } 
            else if (data.status === 'failed') {
                statusTxt.innerText = `Build Failure: ${data.error || 'Unknown runtime exception.'}`;
                progressBar.style.backgroundColor = '#ff4444';
                clearInterval(taskMonitor);
            }
        })
        .catch(err => {
            failureCount++;
            statusTxt.innerText = `Reconnecting to runtime daemon... (Attempt ${failureCount}/5)`;
            
            if (failureCount >= 5) {
                statusTxt.innerText = 'Task abandoned. Connection with PS5 payload server lost.';
                progressBar.style.backgroundColor = '#ff4444';
                clearInterval(taskMonitor);
                console.error('Polling connection terminated:', err);
            }
        });
    }, 2000);
}
