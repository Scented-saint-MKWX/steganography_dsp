# Image-in-Audio Steganography on TMS320C6748 Floating-Point DSP

An end-to-end DSP final project implementing high-capacity image steganography inside carrier audio, based on spectral synthesis (ISTFT), keyed pseudo-random permutation, and Discrete Wavelet Transform (DWT) high-pass sub-band embedding.

Designed for Texas Instruments **TMS320C6748** (C674x VLIW DSP) with host-verifiable C99 core and Python 3 golden reference models.

---

## Quickstart (Host Verification)

### 1. Compile & Run All C Unit Tests
```bash
cd target
make clean && make test
```
*Builds the C99 test suite using `gcc` and executes 14 unit tests (PRNG, Permutation, Kaiser, FFT, Mirror, DWT, Embed/Extract round-trip).*

### 2. Run Host Stego Application
```bash
make host_stego
./build_host/stego_host
```

### 3. Run Python Reference Pipeline & Baseline Benchmarks
```bash
# Setup virtual environment
python3 -m venv .venv
source .venv/bin/activate
pip install -r host/requirements.txt

# Run Python reference self-test
python3 host/ref_pipeline.py --selftest

# Run Classical Steganography Baselines (LSB, Spread Spectrum, Phase Coding)
python3 host/compare_methods.py

# Run Kaiser Window Sweep (Reproducing Paper Table 2)
python3 host/sweep_window.py
```

---

## Repository Structure

```
dsp_stego/
├── README.md                     # This quickstart document
├── EXPLANATION.md                # Comprehensive technical and mathematical manual
├── AGENTS.md                     # Guidelines for AI agents working in repo
├── host/                         # Python reference model, tools, DSS automation
│   ├── ref_pipeline.py           # Golden embed/extract pipeline in NumPy/PyWavelets
│   ├── prng.py                   # Bit-exact xorshift32 PRNG + Fisher-Yates shuffle
│   ├── compare_methods.py        # Baseline methods (LSB, DSSS, Phase Coding)
│   ├── sweep_window.py           # Window length sweep (Table 2 analysis)
│   ├── run_target.js             # DSS automation script for target loading/polling
│   └── run_all.py                # End-to-end multi-backend orchestrator
├── target/                       # C99 Firmware & C6748 Target Implementation
│   ├── config.h                  # Central tunables, DDR2 address layout
│   ├── memmap.cmd                # C6748 linker script (L2 SRAM + DDR2 mappings)
│   ├── main.c                    # Application entry (EMBED / EXTRACT / SELFTEST)
│   ├── Makefile                  # Host GCC and TI cl6x build targets
│   ├── core/                     # Math & Algorithm kernels (C99 portable)
│   ├── io/                       # Header serialization, CRC-32, file I/O
│   ├── util/                     # Platform abstractions, timing, metrics
│   └── tests/                    # Unit tests and test_runner.c
├── docs/
│   ├── memory_map.md             # Detailed L2/DDR2 memory budget
│   ├── ccs_import.md             # Importing, debugging & graphing in CCS
│   └── results/                  # Generated benchmark tables and metrics
└── scripts/
    ├── build_host_tests.sh/.bat  # One-step host test build script
    └── bootstrap_ccs.md          # CCS project creation guide
```

---

## Status and Verification Summary

### Verified on Host:
- [x] Bit-exact PRNG and Fisher-Yates permutation between C and Python (`test_prng`, `test_permute`).
- [x] Kaiser window generator ($I_0(x)$ power series) verified against SciPy (`test_kaiser`).
- [x] Radix-2 Cooley-Tukey FFT/IFFT verified against NumPy complex FFT (`test_fft`).
- [x] Hermitian symmetry ("make pair") real-signal IFFT construction (`test_mirror`, `test_stft_img`).
- [x] Discrete Wavelet Transform (Haar & db2) cascade decomposition and perfect reconstruction (`test_dwt`).
- [x] Full embed and extract round-trip in C and Python with wrong-key noise validation (`test_embed_extract`).
- [x] Comparative evaluation against LSB, Spread Spectrum, and Phase Coding baselines.
- [x] Window length sweep ($N \in \{16..1024\}$) reproducing Table 2 trends.

### Target Hardware (TMS320C6748) Notes:
- **Linker Configuration**: Linker script (`memmap.cmd`) and compile-time overlap assertions (`config.h`) validated.
- **On-Board Execution**: Automated through `host/run_target.js` via Debug Server Scripting (DSS). Physical board validation requires target probe connection (XDS100v3) via CCS.
