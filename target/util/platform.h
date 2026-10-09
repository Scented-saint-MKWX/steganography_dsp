/*
 * platform.h - Platform abstraction layer
 *
 * Isolates TI C6000-specific code (pragmas, intrinsics, TSC, DSPLIB)
 * behind macros so the core algorithms compile on both the DSP target
 * (cl6x) and a host compiler (gcc/clang) for testing.
 *
 * See EXPLANATION.md Section 9 for C6748 architecture notes.
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

/* ─── Compiler detection ─────────────────────────────────────────────── */

#ifdef __TI_COMPILER_VERSION__
  /* ── TI C6000 compiler (cl6x) ── */
  #define PLATFORM_TI   1
  #define PLATFORM_HOST 0

  #include <c6x.h>  /* TSCL, TSCH, _nassert, _itoll, etc. */

  /* Cycle counter (Time Stamp Counter): 64-bit, counts CPU cycles */
  static inline uint64_t platform_get_cycles(void) {
      TSCL;  /* Reading TSCL latches TSCH; must read TSCL first */
      return _itoll(TSCH, TSCL);
  }

  /* Enable TSC (must be done once at startup) */
  static inline void platform_init_tsc(void) {
      TSCL = 0;  /* Writing to TSCL enables the counter */
  }

  /* Alignment assertion hint for optimizer */
  #define PLATFORM_NASSERT(cond) _nassert(cond)

  /* Data section placement */
  #define PLATFORM_DATA_SECTION(buf, section) \
      _Pragma("DATA_SECTION(" #buf ", \"" section "\")")

  /* Must-iterate pragma for loop optimization */
  #define PLATFORM_MUST_ITERATE(min, max, mult) \
      _Pragma("MUST_ITERATE(" #min ", " #max ", " #mult ")")

  /* Memory alignment */
  #define PLATFORM_ALIGN(n) __attribute__((aligned(n)))

  /* Restrict qualifier */
  #define PLATFORM_RESTRICT restrict

  /* DSPLIB toggle: define USE_DSPLIB=1 in the build to enable.
   * When enabled, include the appropriate DSPLIB header. */
  #ifdef USE_DSPLIB
    #include "dsplib.h"  /* DSPF_sp_fftSPxSP, etc. */
  #endif

#else
  /* ── Host compiler (gcc / clang / MSVC) ── */
  #define PLATFORM_TI   0
  #define PLATFORM_HOST 1

  /* Cycle counter: use a simple fallback (not cycle-accurate) */
  #if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    #include <x86intrin.h>
    static inline uint64_t platform_get_cycles(void) {
        return __rdtsc();
    }
  #else
    #include <time.h>
    static inline uint64_t platform_get_cycles(void) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
  #endif

  static inline void platform_init_tsc(void) { /* no-op on host */ }

  /* No-op stubs for TI-specific features */
  #define PLATFORM_NASSERT(cond) ((void)0)
  #define PLATFORM_DATA_SECTION(buf, section) /* no-op */
  #define PLATFORM_MUST_ITERATE(min, max, mult) /* no-op */

  #ifdef _MSC_VER
    #define PLATFORM_ALIGN(n) __declspec(align(n))
  #else
    #define PLATFORM_ALIGN(n) __attribute__((aligned(n)))
  #endif

  #define PLATFORM_RESTRICT restrict

#endif /* __TI_COMPILER_VERSION__ */

/* ─── Common utilities ───────────────────────────────────────────────── */

/* Minimum / maximum (avoid double-evaluation with GCC extension if available) */
#if defined(__GNUC__)
  #define PLATFORM_MIN(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
  #define PLATFORM_MAX(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })
#else
  #define PLATFORM_MIN(a, b) ((a) < (b) ? (a) : (b))
  #define PLATFORM_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Static assert that works in C99 */
#ifndef STATIC_ASSERT
  #define STATIC_ASSERT(cond, msg) \
      typedef char static_assert_##msg[(cond) ? 1 : -1]
#endif

#endif /* PLATFORM_H */

