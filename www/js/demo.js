// Demo mode — lets the full Quaza UI run in any browser without a PS5 payload.
// Activates automatically when the real payload API is unreachable (e.g. HTTPS
// preview, or HTTP page with no ELF loaded). When active, every API call is
// intercepted and answered with simulated data + animated progress so the
// entire flow is clickable and visible.
(function (window) {
    'use strict';

    const DEMO_CONTENT_ID = 'CUSA00000-XXXX00000_00-AAAAAAAAAABBBBBB';
    const DEMO_OUTPUT_PATH = '/mnt/usb0/quaza_output/CUSA00000-XXXX00000_00-AAAAAAAAAABBBBBB.pkg';

    // Build phases: [status, status_text, target_percent, current_file, ms]
    const BUILD_PHASES = [
        [1, 'Scanning files...', 8, 'app/', 700],
        [1, 'Scanning files...', 16, 'app/eboot.bin', 600],
        [1, 'Scanning files...', 24, 'app/sce_sys/', 600],
        [2, 'Building PFS image...', 38, 'image0.pfs', 900],
        [2, 'Building PFS image...', 52, 'image0.pfs (compressing)', 900],
        [3, 'Building PKG...', 66, 'pkg_header', 700],
        [3, 'Building PKG...', 78, 'pkg_body', 800],
        [3, 'Building PKG...', 90, 'pkg_tail', 700],
        [4, 'Complete', 100, '', 400],
    ];

    let buildState = { status: 0, progress_percent: 0, status_text: 'Idle', current_file: '', output_path: '' };
    let buildTimer = null;
    let phaseIndex = 0;

    function isDemoMode() {
        return window.QUAZA_DEMO === true;
    }

    function delay(ms) {
        return new Promise(r => setTimeout(r, ms));
    }

    // Simulate the /api/status endpoint
    async function mockStatus() {
        return {
            payload: 'quaza_payload.elf',
            version: '1.0.0-demo',
            uptime: Math.floor(performance.now() / 1000),
            status: 'ready',
            demo: true,
        };
    }

    // Simulate /api/sfo/parse — derive a plausible content ID from the path
    async function mockSfoParse(path) {
        await delay(500);
        // Try to extract a CUSA-style code from the path
        const match = path.match(/(CUSA\d{5})/i);
        const cusa = match ? match[1].toUpperCase() : 'CUSA00000';
        const contentId = `${cusa}-XXXX00000_00-AAAAAAAAAABBBBBB`;
        return {
            content_id: contentId,
            title: 'Demo Game Title',
            title_id: cusa,
            version: '1.00',
            demo: true,
        };
    }

    // Simulate /api/pkg/create — kick off the build
    async function mockPkgCreate(params) {
        await delay(400);
        buildState = { status: 1, progress_percent: 0, status_text: 'Scanning files...', current_file: '', output_path: '' };
        phaseIndex = 0;
        if (buildTimer) clearInterval(buildTimer);
        runBuildPhases();
        return { ok: true, message: 'PKG creation started (demo mode)', demo: true };
    }

    function runBuildPhases() {
        if (phaseIndex >= BUILD_PHASES.length) return;
        const [status, text, pct, file, ms] = BUILD_PHASES[phaseIndex];
        buildState = {
            status,
            status_text: text,
            progress_percent: pct,
            current_file: file,
            output_path: status === 4 ? DEMO_OUTPUT_PATH : '',
        };
        phaseIndex++;
        buildTimer = setTimeout(runBuildPhases, ms);
    }

    // Simulate /api/pkg/progress
    async function mockPkgProgress() {
        return { ...buildState, demo: true };
    }

    // Simulate /api/pkg/download — return a tiny blob so the link works
    function mockDownloadUrl() {
        const blob = new Blob(['Quaza demo PKG — this is a placeholder file.'], { type: 'application/octet-stream' });
        return URL.createObjectURL(blob);
    }

    // Public API — the page scripts call these instead of fetch() when in demo mode
    window.QuazaDemo = {
        isDemoMode,
        status: mockStatus,
        sfoParse: mockSfoParse,
        pkgCreate: mockPkgCreate,
        pkgProgress: mockPkgProgress,
        downloadUrl: mockDownloadUrl,
        DEMO_OUTPUT_PATH,
    };

    // Auto-detect demo mode: if we're not on plain HTTP to a real host, or the
    // status probe fails, flip the flag on. Page scripts check isDemoMode().
    const proto = window.location.protocol;
    const host = window.location.hostname;
    const likelyRealPayload = proto === 'http:' && host !== '127.0.0.1' && host !== 'localhost' && host !== '';

    if (!likelyRealPayload) {
        window.QUAZA_DEMO = true;
        console.info('%c[Quaza] Demo mode active — UI runs with simulated data. No PS5 payload required.', 'color:#2dc8ef;font-weight:bold;');
    } else {
        // On a real HTTP host, probe the payload; if it's down, fall back to demo.
        fetch(`http://${host}:4242/api/status`)
            .then(r => r.json())
            .then(() => { window.QUAZA_DEMO = false; })
            .catch(() => {
                window.QUAZA_DEMO = true;
                console.info('%c[Quaza] Payload unreachable — demo mode active.', 'color:#2dc8ef;font-weight:bold;');
            });
    }
})(window);
