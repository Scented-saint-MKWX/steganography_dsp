# ============================================================================
# Makefile for DSP Steganography Project
#
# Two build targets:
#   1. Host build (gcc/clang) for testing and verification
#   2. Target build (TI cl6x) for TMS320C6748
#
# Usage:
#   make host          - Build host test binary
#   make test          - Build and run all host tests
#   make target        - Build for C6748 (requires cl6x)
#   make clean         - Remove build artifacts
#   make host_stego    - Build host stego binary (full embed/extract)
#
# Variables (set on command line or environment):
#   CGT_ROOT     - TI Code Generation Tools root (e.g. /opt/ti/cgt-c6000_8.3.x)
#   DSPLIB_ROOT  - TI DSPLIB root (optional, for USE_DSPLIB builds)
#   CC_HOST      - Host C compiler (default: gcc)
# ============================================================================

# ── Host compiler ──────────────────────────────────────────────────────────
CC_HOST    ?= gcc
CFLAGS_HOST = -std=c99 -Wall -Wextra -Wpedantic -O2 -g \
              -I. -Icore -Iio -Iutil \
              -DPLATFORM_HOST_BUILD=1 \
              -D_POSIX_C_SOURCE=199309L
LDFLAGS_HOST = -lm

# ── TI cl6x compiler ──────────────────────────────────────────────────────
CGT_ROOT   ?= /opt/ti/cgt-c6000_8.3.13
CL6X        = $(CGT_ROOT)/bin/cl6x
CFLAGS_TI   = -mv6740 --float_support=fpu32 -O2 --opt_for_speed=5 \
              --include_path=. \
              --include_path=core \
              --include_path=io \
              --include_path=util \
              --include_path="$(CGT_ROOT)/include" \
              --display_error_number --diag_warning=225 \
              --abi=eabi --mem_model:data=far
LDFLAGS_TI  = -z --reread_libs --warn_sections \
              --rom_model \
              -i"$(CGT_ROOT)/lib" \
              -l rts6740_elf.lib \
              memmap.cmd

ifdef DSPLIB_ROOT
  CFLAGS_TI += --include_path="$(DSPLIB_ROOT)/packages/ti/dsplib/src" \
               -DUSE_DSPLIB=1
  LDFLAGS_TI += -i"$(DSPLIB_ROOT)/packages/ti/dsplib/lib" \
                -l dsplib.ae674
endif

# ── Source files ───────────────────────────────────────────────────────────
CORE_SRCS  = core/prng.c core/permute.c core/kaiser.c core/fft.c \
             core/mirror.c core/stft_img.c core/dwt.c \
             core/embed.c core/extract.c
IO_SRCS    = io/crc32.c io/dataio.c
UTIL_SRCS  = util/timing.c util/metrics.c
ALL_SRCS   = $(CORE_SRCS) $(IO_SRCS) $(UTIL_SRCS)

TEST_SRCS  = tests/test_prng.c tests/test_permute.c tests/test_kaiser.c \
             tests/test_fft.c tests/test_mirror.c tests/test_stft_img.c \
             tests/test_dwt.c tests/test_embed_extract.c \
             tests/test_runner.c

# ── Build directories ─────────────────────────────────────────────────────
HOST_OBJ_DIR    = build_host
TARGET_OBJ_DIR  = build_target

# ── Object files ──────────────────────────────────────────────────────────
HOST_OBJS       = $(patsubst %.c,$(HOST_OBJ_DIR)/%.o,$(ALL_SRCS))
HOST_TEST_OBJS  = $(patsubst %.c,$(HOST_OBJ_DIR)/%.o,$(TEST_SRCS))
TARGET_OBJS     = $(patsubst %.c,$(TARGET_OBJ_DIR)/%.o,$(ALL_SRCS) main.c)

# ── Targets ────────────────────────────────────────────────────────────────

.PHONY: all host test target clean host_stego

all: host

# ── Host test runner ───────────────────────────────────────────────────────
host: $(HOST_OBJ_DIR)/test_runner

$(HOST_OBJ_DIR)/test_runner: $(HOST_OBJS) $(HOST_TEST_OBJS)
	@echo "  LINK  $@"
	@$(CC_HOST) $(CFLAGS_HOST) -o $@ $^ $(LDFLAGS_HOST)

# ── Host stego binary (full pipeline) ──────────────────────────────────────
host_stego: $(HOST_OBJ_DIR)/stego_host

$(HOST_OBJ_DIR)/stego_host: $(HOST_OBJS) $(HOST_OBJ_DIR)/main.o
	@echo "  LINK  $@"
	@$(CC_HOST) $(CFLAGS_HOST) -o $@ $^ $(LDFLAGS_HOST)

$(HOST_OBJ_DIR)/main.o: main.c
	@mkdir -p $(dir $@)
	@echo "  CC    $<"
	@$(CC_HOST) $(CFLAGS_HOST) -c -o $@ $<

# ── Pattern rules: host ───────────────────────────────────────────────────
$(HOST_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC    $<"
	@$(CC_HOST) $(CFLAGS_HOST) -c -o $@ $<

# ── Test execution ─────────────────────────────────────────────────────────
test: $(HOST_OBJ_DIR)/test_runner
	@echo ""
	@echo "════════════════════════════════════════════"
	@echo "  Running host tests..."
	@echo "════════════════════════════════════════════"
	@echo ""
	@cd .. && ./target/$(HOST_OBJ_DIR)/test_runner

# ── TI target build ───────────────────────────────────────────────────────
target: $(TARGET_OBJ_DIR)/stego.out

$(TARGET_OBJ_DIR)/stego.out: $(TARGET_OBJS)
	@echo "  LINK  $@"
	@$(CL6X) $(CFLAGS_TI) -z -o $@ $^ $(LDFLAGS_TI)

$(TARGET_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CL6X  $<"
	@$(CL6X) $(CFLAGS_TI) -c -o $@ $<

# ── Clean ──────────────────────────────────────────────────────────────────
clean:
	rm -rf $(HOST_OBJ_DIR) $(TARGET_OBJ_DIR)
	rm -f stego_host test_runner

