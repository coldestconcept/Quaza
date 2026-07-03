document.getElementById('convertBtn').addEventListener('click', () => {
    const dumpPath = document.getElementById('dumpPath').value;
    const statusBox = document.getElementById('status-box');
    const statusTxt = document.getElementById('statusTxt');

    if (!dumpPath) {
        alert('Please enter a target path directory.');
        return;
    }

    statusBox.style.display = 'block';
    statusTxt.innerText = 'Sending task payload to PS5 server...';

    // Base targeting relative host setup
    const host = window.location.hostname;
    
    fetch(`http://${host}:8080/api/convert`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ path: dumpPath })
    })
    .then(response => response.json())
    .then(data => {
        statusTxt.innerText = data.message;
        // Start polling execution progress loop
        pollStatus(host);
    })
    .catch(err => {
        statusTxt.innerText = 'Failed to communicate with runtime ELF. Ensure payload is running.';
        console.error(err);
    });
});

function pollStatus(host) {
    const statusTxt = document.getElementById('statusTxt');
    
    const interval = setInterval(() => {
        fetch(`http://${host}:8080/api/status`)
        .then(res => res.json())
        .then(data => {
            if(data.status === 'completed') {
                statusTxt.innerText = 'Success! PKG has been compiled and saved.';
                clearInterval(interval);
            } else if (data.status === 'processing') {
                statusTxt.innerText = `Compiling PFS/PKG structures... Progress: ${data.progress}%`;
            }
        })
        .catch(() => clearInterval(interval));
    }, 2000);
}
