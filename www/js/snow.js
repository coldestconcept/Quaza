// Snowfall canvas effect — white snowballs drifting down
(function () {
    const canvas = document.createElement('canvas');
    canvas.id = 'snow-canvas';
    canvas.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;pointer-events:none;z-index:0;';
    document.body.prepend(canvas);

    const ctx = canvas.getContext('2d');
    let W, H, flakes;

    function resize() {
        W = canvas.width  = window.innerWidth;
        H = canvas.height = window.innerHeight;
    }

    function makeFlake() {
        const r = Math.random() * 7 + 3; // radius 3–10px
        return {
            x: Math.random() * W,
            y: Math.random() * -H,       // start above viewport
            r,
            speed: Math.random() * 1.2 + 0.4,
            drift: (Math.random() - 0.5) * 0.5,
            opacity: Math.random() * 0.55 + 0.35,
            wobble: Math.random() * Math.PI * 2,
            wobbleSpeed: (Math.random() - 0.5) * 0.02,
        };
    }

    function init() {
        resize();
        const count = Math.floor((W * H) / 12000);
        flakes = Array.from({ length: count }, () => {
            const f = makeFlake();
            f.y = Math.random() * H; // seed already on-screen at start
            return f;
        });
    }

    function draw() {
        ctx.clearRect(0, 0, W, H);
        for (const f of flakes) {
            ctx.beginPath();
            ctx.arc(f.x, f.y, f.r, 0, Math.PI * 2);
            ctx.fillStyle = `rgba(255,255,255,${f.opacity})`;
            ctx.fill();
        }
    }

    function update() {
        for (const f of flakes) {
            f.wobble += f.wobbleSpeed;
            f.x += f.drift + Math.sin(f.wobble) * 0.4;
            f.y += f.speed;

            if (f.y - f.r > H) {
                // reset to top
                const fresh = makeFlake();
                f.x = fresh.x;
                f.y = -f.r;
            }
            // wrap horizontally
            if (f.x < -f.r)  f.x = W + f.r;
            if (f.x > W + f.r) f.x = -f.r;
        }
    }

    function loop() {
        update();
        draw();
        requestAnimationFrame(loop);
    }

    window.addEventListener('resize', () => { resize(); init(); });
    init();
    loop();
})();
