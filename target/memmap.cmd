/*
 * memmap.cmd - TMS320C6748 Linker Command File
 *
 * Memory layout for Image-in-Audio Steganography project.
 * See docs/memory_map.md and config.h for the full address map.
 *
 * Assumptions:
 *   - DDR2: 128 MB starting at 0xC0000000
 *   - L2 SRAM: 256 KB at 0x00800000 (default for C6748)
 *   - L1D: 32 KB at 0x00F00000
 *   - L1P: 32 KB at 0x00E00000
 *   - GEL file handles PLL, DDR2, and EMIFA initialization.
 *
 * NOTE: This file is used by cl6x (TI linker). It is not used for host builds.
 */

MEMORY
{
    /* ── On-chip memory ─────────────────────────────────── */
    L2_SRAM     : o = 0x00800000  l = 0x00040000  /* 256 KB L2 SRAM */
    L1D_SRAM    : o = 0x00F00000  l = 0x00008000  /* 32 KB L1D cache/SRAM */
    L1P_SRAM    : o = 0x00E00000  l = 0x00008000  /* 32 KB L1P cache/SRAM */

    /* ── DDR2 external memory ───────────────────────────── */
    /* General code and data */
    DDR2_CODE   : o = 0xC0000000  l = 0x00100000  /* 1 MB for code */

    /* Application-specific data regions (non-overlapping, see config.h) */
    DDR2_IMG_IN : o = 0xC0100000  l = 0x00100000  /* 1 MB input image */
    DDR2_TARGET : o = 0xC0200000  l = 0x00200000  /* 2 MB target signal */
    DDR2_CARRIER: o = 0xC0400000  l = 0x01000000  /* 16 MB carrier audio */
    DDR2_STEGO  : o = 0xC1400000  l = 0x01000000  /* 16 MB stego output */
    DDR2_IMGOUT : o = 0xC2400000  l = 0x00200000  /* 2 MB extracted image */
    DDR2_PERM   : o = 0xC2600000  l = 0x00200000  /* 2 MB permutation buf */
    DDR2_HDR    : o = 0xC2800000  l = 0x00000100  /* 256 B header */
    DDR2_RESULT : o = 0xC2800100  l = 0x00000100  /* 256 B results struct */
    DDR2_FLAG   : o = 0xC2800200  l = 0x00000004  /* 4 B done flag */
    DDR2_DWTW   : o = 0xC2800400  l = 0x01800000  /* 24 MB DWT work */
    DDR2_HEAP   : o = 0xC4000000  l = 0x04000000  /* 64 MB heap (if needed) */
}

SECTIONS
{
    /* ── Code sections ─────────────────────────────────── */
    .text       >  DDR2_CODE
    .switch     >  DDR2_CODE
    .cinit      >  DDR2_CODE
    .pinit      >  DDR2_CODE
    .const      >  DDR2_CODE

    /* ── Standard data sections ────────────────────────── */
    .bss        >  L2_SRAM
    .neardata   >  L2_SRAM
    .rodata     >  L2_SRAM
    .far        >  DDR2_HEAP
    .fardata    >  DDR2_HEAP
    .stack      >  L2_SRAM  (size = 0x4000)   /* 16 KB stack */
    .sysmem     >  DDR2_HEAP (size = 0x10000)  /* 64 KB heap  */

    /* ── Performance-critical data in L2 SRAM ──────────── */
    .l2_twiddles  >  L2_SRAM   /* FFT twiddle factors (8 KB) */
    .l2_window    >  L2_SRAM   /* Kaiser window (4 KB) */
    .l2_fftwork   >  L2_SRAM   /* Per-frame FFT working buffer (8 KB) */
    .l2_dwt_filt  >  L2_SRAM   /* DWT filter coefficients (<1 KB) */

    /* ── Application data in DDR2 ──────────────────────── */
    .ddr_image_in  >  DDR2_IMG_IN
    .ddr_target    >  DDR2_TARGET
    .ddr_carrier   >  DDR2_CARRIER
    .ddr_stego     >  DDR2_STEGO
    .ddr_image_out >  DDR2_IMGOUT
    .ddr_perm      >  DDR2_PERM
    .ddr_header    >  DDR2_HDR
    .ddr_results   >  DDR2_RESULT
    .ddr_done_flag >  DDR2_FLAG
    .ddr_dwt_work  >  DDR2_DWTW

    /* ── C I/O library ─────────────────────────────────── */
    .cio        >  DDR2_HEAP
}

