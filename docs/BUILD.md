# Building Quaza

## Prerequisites

- Linux development environment (Ubuntu/Debian recommended)
- PS5 Payload SDK installed
- CMake 3.20+

## Install PS5 Payload SDK

```bash
git clone https://github.com/ps5-payload-dev/sdk.git
cd sdk
make
sudo make DESTDIR=/opt/ps5-payload-sdk install
