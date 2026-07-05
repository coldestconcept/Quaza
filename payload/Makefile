# Quaza PS5 Payload — Makefile
#
# Builds quaza_payload.elf using the PS5 Payload SDK toolchain.
#
# Usage (from the payload/ directory):
#   export PS5_PAYLOAD_SDK=$HOME/target-sdk/ps5-payload-sdk
#   make
#
# The SDK provides:
#   - clang cross-compiler targeting x86_64-ps5-freebsd12
#   - Library stubs in $SDK/target/lib/ (creates DT_NEEDED entries)
#   - Headers in $SDK/target/include/
#
# If the SDK is not installed, see docs/BUILD.md.

PS5_PAYLOAD_SDK ?= $(HOME)/target-sdk/ps5-payload-sdk

SDK_TARGET  := $(PS5_PAYLOAD_SDK)/target
SDK_INCLUDE := $(SDK_TARGET)/include
SDK_LIB     := $(SDK_TARGET)/lib

CC := clang

# Cross-compile target (FreeBSD 12, x86-64, PS5 ABI)
TARGET_TRIPLE := x86_64-sie-ps5

CFLAGS := \
    -target $(TARGET_TRIPLE) \
    -isysroot $(SDK_TARGET) \
    -I$(SDK_INCLUDE) \
    -I$(SDK_INCLUDE)/c++/v1 \
    -I. \
    -Icrt \
    -Ilibprosperopkg/include \
    -fPIC \
    -fpie \
    -fno-stack-protector \
    -O2 \
    -Wall \
    -Wno-unused-parameter \
    -Wno-unused-function

# Use clang as the linker driver (not ld.lld directly) so it handles target
# ABI and relocation model correctly.  Driver flags like -nostartfiles and
# -Wl,... are then valid.  -fuse-ld=lld makes clang call ld.lld internally.
LDFLAGS := \
    -target $(TARGET_TRIPLE) \
    -fuse-ld=lld \
    -nostartfiles \
    -Wl,-T,payload.ld \
    -Wl,--export-dynamic \
    -Wl,--gc-sections \
    -L$(SDK_LIB) \
    -lkernel \
    -lSceLibcInternal \
    -lSceNet \
    -lSceUserService \
    -lSceSystemService \
    -lSceAppInstUtil

# ── Sources ──────────────────────────────────────────────────────────────────

CRT_SRCS := \
    crt/crt.c \
    crt/klog.c \
    crt/kernel.c \
    crt/mdbg.c \
    crt/patch.c \
    crt/rtld.c \
    crt/env.c

LIBPKG_SRCS := \
    libprosperopkg/src/npdrm.c \
    libprosperopkg/src/pfs_builder.c \
    libprosperopkg/src/pfs_directory.c \
    libprosperopkg/src/pfs_inode.c \
    libprosperopkg/src/pkg_builder.c \
    libprosperopkg/src/utils.c

PAYLOAD_SRCS := \
    main.c \
    pkg_server.c \
    browser_launch.c \
    self_install.c \
    param_sfo.c \
    jailbreak_detect.c

ALL_SRCS := $(CRT_SRCS) $(LIBPKG_SRCS) $(PAYLOAD_SRCS)
OBJS     := $(ALL_SRCS:.c=.o)

# ── Rules ────────────────────────────────────────────────────────────────────

.PHONY: all clean

all: quaza_payload.elf
	@echo ""
	@echo "✓ Built quaza_payload.elf"
	@file quaza_payload.elf || true
	@size quaza_payload.elf || true

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

quaza_payload.elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	find . -name '*.o' -delete
	rm -f quaza_payload.elf
