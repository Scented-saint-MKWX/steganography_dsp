# TMS320C6748 Memory Map & Budget Specification

This document details the memory layout, section mappings, and address budgets designed for the **TMS320C6748 DSP** (128 MB DDR2 at base address `0xC0000000`).

## 1. On-Chip SRAM (L2 RAM) Budget (256 KB Total)

| Section Name   | Destination | Start Addr   | Size / Capacity | Purpose |
|----------------|-------------|--------------|-----------------|---------|
| `.text`/stack  | L2_SRAM     | `0x00800000` | ~64 KB          | Stack (16 KB), near data, critical routines |
| `.l2_twiddles` | L2_SRAM     | Dynamic      | 8 KB            | Precomputed Radix-2 FFT twiddle factors (1024 complex float) |
| `.l2_window`   | L2_SRAM     | Dynamic      | 4 KB            | Precomputed Kaiser window coefficients (1024 float) |
| `.l2_fftwork`  | L2_SRAM     | Dynamic      | 8 KB            | High-speed per-frame FFT complex scratchpad |
| **Total L2**   |             |              | **~85 KB / 256 KB** | Leaves ample margin (>170 KB) for cache and runtime |

## 2. External DDR2 RAM Memory Map (128 MB Total: 0xC0000000 - 0xC7FFFFFF)

The external memory layout is partitioned into contiguous, non-overlapping regions checked at compile-time via `STEGO_STATIC_ASSERT` in [config.h](file:///home/moham/projects/steganography_image_dsp/dsp_stego/target/config.h) and linked via [memmap.cmd](file:///home/moham/projects/steganography_image_dsp/dsp_stego/target/memmap.cmd):

| Region Name       | Start Address | Size / Alloc | Max Capacity / Usage | Section Pragma |
|-------------------|---------------|--------------|----------------------|----------------|
| **Input Image**   | `0xC0000000`  | 2 MB         | 512×512 float32 (1 MB used) | `.ddr_image_in` |
| **Target Signal** | `0xC0200000`  | 2 MB         | 524,288 float32 (2 MB used) | `.ddr_target` |
| **Carrier Audio** | `0xC0400000`  | 16 MB        | Up to 4.2M samples (16.8 MB max) | `.ddr_carrier` |
| **Stego Output**  | `0xC1400000`  | 16 MB        | 4.2M samples stego audio | `.ddr_stego` |
| **Output Image**  | `0xC2400000`  | 2 MB         | 512×512 reconstructed image | `.ddr_image_out` |
| **Permutation Buf**| `0xC2600000` | 2 MB         | 524,288 uint32 permutation indices | `.ddr_perm` |
| **Header Block**  | `0xC2800000`  | 256 B        | 64-byte packed sidecar header | `.ddr_header` |
| **Results Struct**| `0xC2800100`  | 256 B        | Status, cycle counts, checksums | `.ddr_results` |
| **Done Flag**     | `0xC2800200`  | 4 B          | Polling flag (0 = busy, 1 = done) | `.ddr_done_flag` |
| **DWT Work Area** | `0xC2800204`  | 24 MB        | Multi-level decomposition working bands | `.ddr_dwt_work` |
| **Heap / Reserve**| `0xC4000000`  | 64 MB        | Free DDR2 margin | `.far`, `.sysmem` |

- **Total Allocated Active Space**: ~63 MB of 128 MB (49% DDR2 utilization).
- **Compile-time Bounds Verification**: [config.h](file:///home/moham/projects/steganography_image_dsp/dsp_stego/target/config.h) verifies all boundary offsets to prevent overlap or buffer overrun.
