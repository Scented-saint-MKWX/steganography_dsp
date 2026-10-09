# Instructions & Rules for AI Agents

Welcome to the **Image-in-Audio Steganography on TMS320C6748** project. All autonomous and interactive agents working in this repository must strictly adhere to the following constraints and architectural principles:

## 1. Golden Development Rules
- **Host Portability**: All core algorithmic files in `target/core/`, `target/io/`, and `target/util/` must remain portable C99. They MUST compile with host compilers (`gcc`, `clang`) as well as TI `cl6x`.
- **Target Specifics in `platform.h`**: Any TI-specific intrinsics, pragmas (`MUST_ITERATE`, `DATA_SECTION`), or cycle counters (TSC) MUST be isolated behind macros in [platform.h](file:///home/moham/projects/steganography_image_dsp/dsp_stego/target/util/platform.h).
- **Zero Heap Allocations in Core**: Never call `malloc()`, `calloc()`, or `free()` inside `target/core/`. All buffers are statically sized and passed explicitly by the caller.
- **Strictly No Standard Random**: NEVER use `rand()`, `srand()`, `drand48()`, or NumPy/MATLAB pseudo-random generators for permutation or key logic. Always use the custom bit-exact `xorshift32` PRNG defined in `target/core/prng.c` and mirrored in `host/prng.py`.
- **DDR2 Section Placement**: On the target, all large data structures (images, audio, permutation indices, working memory) reside in DDR2 at `0xC0000000` via `#pragma DATA_SECTION`. Small per-frame buffers (FFT twiddles, Kaiser window) reside in L2 SRAM.
- **Never Fabricate Measurements**: Do not claim hardware execution results unless actual board measurements were gathered and logged via DSS / JTAG. Clearly distinguish host verification results from board profiling.

## 2. Verification Protocol
Before modifying or submitting any algorithm changes:
1. Run `./scripts/build_host_tests.sh` (or `make host && make host_stego`). Ensure zero failures.
2. Run `.venv/bin/python3 host/ref_pipeline.py --selftest` to ensure Python golden model parity.
3. Update [EXPLANATION.md](file:///home/moham/projects/steganography_image_dsp/dsp_stego/EXPLANATION.md) if mathematical formulations or data structures change.
