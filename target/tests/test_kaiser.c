#include <stdio.h>
#include <math.h>
#include "../core/kaiser.h"

int test_bessel_i0_known_values(void) {
    /* I0(0) = 1.0 */
    float val0 = bessel_i0(0.0f);
    if (fabsf(val0 - 1.0f) > 1e-6f) {
        printf("FAIL: bessel_i0(0) = %f, expected 1.0\n", val0);
        return 1;
    }

    /* I0(1) ~= 1.266066 */
    float val1 = bessel_i0(1.0f);
    if (fabsf(val1 - 1.266066f) > 1e-4f) {
        printf("FAIL: bessel_i0(1) = %f, expected 1.266066\n", val1);
        return 1;
    }

    printf("PASS: test_bessel_i0_known_values\n");
    return 0;
}

int test_kaiser_symmetry_and_endpoints(void) {
    uint32_t N = 64;
    float win[64];
    kaiser_window(win, N, 1.0f);

    /* Test symmetry w[n] == w[N-1-n] */
    for (uint32_t i = 0; i < N / 2; i++) {
        if (fabsf(win[i] - win[N - 1 - i]) > 1e-6f) {
            printf("FAIL: Kaiser window asymmetry at %u: %f != %f\n", i, win[i], win[N - 1 - i]);
            return 1;
        }
    }

    /* Middle point should be maximum (~ 1.0) */
    uint32_t mid = N / 2;
    if (win[mid] < win[0]) {
        printf("FAIL: Kaiser window center %f < endpoint %f\n", win[mid], win[0]);
        return 1;
    }

    printf("PASS: test_kaiser_symmetry_and_endpoints\n");
    return 0;
}
