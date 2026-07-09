# Quaza PS5 Payload — Makefile
#
# Works in two modes, detected automatically:
#
#   SDK mode  — PS5 Payload SDK installed at $(PS5_PAYLOAD_SDK).
#               Uses SDK sysroot headers and stub libs.
#
#   Stub mode — SDK not found (e.g. bare Termux install).
#               Builds minimal x86_64 stub .so files in stubs/ then links
#               against them so the ELF gets correct DT_NEEDED entries.
#               Run once: bash stubs/gen_stubs.sh   (Makefile does this auto)
#
# Usage:
#   export PS5_PAYLOAD_SDK=$HOME/target-sdk/ps5-payload-sdk  # optional
#   make

# Try the most common install locations in order.
# john-tornblom/ps5-payload-sdk uses sce_stubs/ for libs; older forks use target/lib/.
PS5_PAYLOAD_SDK ?= $(or \
    $(if $(wildcard $(HOME)/ps5-payload-sdk/sce_stubs/libkernel.so),$(HOME)/ps5-payload-sdk),\
    $(if $(wildcard $(HOME)/target-sdk/ps5-payload-sdk/target/lib/libkernel.so),$(HOME)/target-sdk/ps5-payload-sdk),\
    $(HOME)/ps5-payload-sdk)

# SDK layout: newer builds use sce_stubs/ for libs and include/ at root;
# older builds used target/lib and target/include.
SDK_SCE_STUBS   := $(PS5_PAYLOAD_SDK)/sce_stubs
SDK_TARGET      := $(PS5_PAYLOAD_SDK)/target
SDK_INCLUDE     := $(or $(wildcard $(PS5_PAYLOAD_SDK)/include),$(SDK_TARGET)/include)
SDK_LIB         := $(or $(wildcard $(SDK_SCE_STUBS)),$(SDK_TARGET)/lib)

CC             := clang
TARGET_TRIPLE  := x86_64-sie-ps5
STUB_DIR       := $(CURDIR)/stubs

# ── SDK detection ─────────────────────────────────────────────────────────────
SDK_PRESENT := $(shell test -f "$(SDK_LIB)/libkernel.so" && echo yes || echo no)

ifeq ($(SDK_PRESENT),yes)
    # ── SDK mode ──────────────────────────────────────────────────────────────
    # john-tornblom/ps5-payload-sdk layout:
    #   sce_stubs/   — pre-built stub .so files (correct PS5 symbol tables)
    #   include/     — PS5-specific headers at SDK root
    # Older forks put libs in target/lib and headers in target/include.
    CC_TARGET     := $(TARGET_TRIPLE)
    # Build up include flags: SDK headers first, then C++ headers if present,
    # then clang's own builtins for stdint.h / stddef.h / etc.
    CLANG_BUILTINS_SDK := $(shell $(CC) -print-resource-dir 2>/dev/null)/include
    SYSROOT_FLAGS := -nostdinc \
                     -I$(SDK_INCLUDE) \
                     $(if $(wildcard $(SDK_INCLUDE)/c++/v1),-I$(SDK_INCLUDE)/c++/v1) \
                     -I$(CURDIR)/include \
                     -isystem $(CLANG_BUILTINS_SDK)
    LIB_FLAGS     := -L$(SDK_LIB)
    STUB_PREREQ   :=
    $(info [SDK mode] Using PS5 Payload SDK at $(PS5_PAYLOAD_SDK))
    $(info [SDK mode] Headers: $(SDK_INCLUDE))
    $(info [SDK mode] Libs:    $(SDK_LIB))
else
    # ── Stub mode ─────────────────────────────────────────────────────────────
    # Use x86_64-linux-gnu so clang finds the host's stdio.h / stdlib.h.
    # The generated x86_64 machine code is identical to x86_64-sie-ps5 for
    # straight C; the PS5-specific ELF layout comes from payload.ld, not the
    # target triple.
    CC_TARGET     := x86_64-linux-gnu
    # -nostdinc stops clang from ever touching Termux's ARM64 bionic headers.
    # -isystem adds clang's own resource-dir headers (stdint.h, stddef.h, etc.).
    # -I adds our minimal hand-written system headers.
    CLANG_BUILTINS := $(shell $(CC) -print-resource-dir 2>/dev/null)/include
    ifeq ($(wildcard $(CLANG_BUILTINS)/stddef.h),)
    $(error Stub mode: clang resource headers not found at $(CLANG_BUILTINS). \
    Install clang on Termux: pkg install clang)
    endif
    SYSROOT_FLAGS := -nostdinc \
                     -isystem $(CLANG_BUILTINS) \
                     -I$(CURDIR)/include
    LIB_FLAGS     := -L$(STUB_DIR)
    STUB_PREREQ   := $(STUB_DIR)/built
    $(info [Stub mode] PS5 SDK not found — using generated stubs in stubs/)
endif

# ── Compile flags ─────────────────────────────────────────────────────────────
CFLAGS := \
    -target $(CC_TARGET) \
    $(SYSROOT_FLAGS) \
    -I. \
    -Icrt \
    -Ilibprosperopkg/include \
    -fPIC \
    -fpie \
    -fno-stack-protector \
    -O2 \
    -Wall \
    -Wno-unused-parameter \
    -Wno-unused-function \
    -MMD -MP

# ── Link flags ────────────────────────────────────────────────────────────────
# The link step always uses x86_64-linux-gnu so that -fuse-ld=lld is accepted
# by the clang driver.  The x86_64-sie-ps5 driver rejects that flag.
# The ELF output format is controlled entirely by payload.ld (OUTPUT_FORMAT /
# OUTPUT_ARCH), so the driver triple at link time does not affect the output.
LDFLAGS := \
    -target x86_64-linux-gnu \
    -fuse-ld=lld \
    -nostartfiles \
    -nostdlib \
    -Wl,-T,payload.ld \
    -Wl,--export-dynamic \
    -Wl,--gc-sections \
    $(LIB_FLAGS) \
    -lkernel \
    -lSceLibcInternal \
    -lSceNet \
    -lSceUserService \
    -lSceSystemService \
    -lSceAppInstUtil

# ── Sources ───────────────────────────────────────────────────────────────────
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
    netlog.c \
    pkg_server.c \
    browser_launch.c \
    self_install.c \
    param_sfo.c \
    jailbreak_detect.c

ALL_SRCS := $(CRT_SRCS) $(LIBPKG_SRCS) $(PAYLOAD_SRCS)
OBJS     := $(ALL_SRCS:.c=.o)
DEPS     := $(ALL_SRCS:.c=.d)

# Auto-generated header dependency files — silently included so changes to
# any .h automatically trigger recompilation of every .c that includes it.
-include $(DEPS)

# ── Rules ─────────────────────────────────────────────────────────────────────
.PHONY: all clean stubs

all: $(STUB_PREREQ) quaza_payload.elf
	@echo ""
	@echo "✓ Built quaza_payload.elf"
	@readelf -d quaza_payload.elf 2>/dev/null | grep NEEDED || true

# Build stubs if they haven't been generated yet, or if gen_stubs.sh changed
# since the last build (avoids linking against stale stub .so files).
$(STUB_DIR)/built: $(STUB_DIR)/gen_stubs.sh
	@echo "Building linker stubs..."
	@bash $(STUB_DIR)/gen_stubs.sh

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

quaza_payload.elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	find . -name '*.o' -not -path './stubs/*' -delete
	find . -name '*.d' -not -path './stubs/*' -delete
	rm -f quaza_payload.elf

stubs:
	bash $(STUB_DIR)/gen_stubs.sh
