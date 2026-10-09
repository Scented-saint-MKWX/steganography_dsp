#include "kaiser.h"
#include <math.h>
#include <stddef.h>

float bessel_i0(float x) {
    float sum = 1.0f;
    float term = 1.0f;
    float x2_4 = (x * x) * 0.25f;

    for (int k = 1; k <= 20; k++) {
        term *= x2_4 / (k * k);
        sum += term;
    }
    return sum;
}

void kaiser_window(float *win, uint32_t N, float beta) {
    if (N == 0 || win == NULL) return;

    if (N == 1) {
        win[0] = 1.0f;
        return;
    }

    if (beta == 0.0f) {
        for (uint32_t n = 0; n < N; n++) {
            win[n] = 1.0f;
        }
        return;
    }

    float i0_beta_inv = 1.0f / bessel_i0(beta);
    float N_minus_1 = (float)(N - 1);

    for (uint32_t n = 0; n < N; n++) {
        float val = ((2.0f * n) / N_minus_1) - 1.0f;
        float val2 = 1.0f - val * val;
        val2 = val2 > 0.0f ? val2 : 0.0f; /* max(0, 1 - val^2) */
        
        float arg = beta * sqrtf(val2);
        win[n] = bessel_i0(arg) * i0_beta_inv;
    }
}
