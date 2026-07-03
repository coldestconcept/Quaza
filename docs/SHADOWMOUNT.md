# Building PS5 PKG Tool

## Prerequisites
- ps5-payload-sdk or sleirsgoevy toolchain
- Python 3.8+ (for scraper)
- PS5 with jailbreak (etaHEN, kstuff, etc.)

## Build Payload
```bash
cd payload
make
```

## Install on PS5
```bash
make install
# Or manually copy files to /data/pkg_tool/
```

## Run
1. Send payload via ELF loader
2. Access http://[PS5_IP]:8080
3. Use web interface
