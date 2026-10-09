#include "util/metrics.h"
#include <math.h>

float compute_mse(const float *a, const float *b, uint32_t len) {
    if (!a || !b || len == 0) return 0.0f;
    float sum_sq = 0.0f;
    for (uint32_t i = 0; i < len; i++) {
        float diff = a[i] - b[i];
        sum_sq += diff * diff;
    }
    return sum_sq / (float)len;
}

float compute_psnr(const float *orig, const float *recon, uint32_t len, float max_val) {
    if (!orig || !recon || len == 0) return 0.0f;
    float mse = compute_mse(orig, recon, len);
    if (mse <= 1e-10f) return 100.0f; // Perfect reconstruction
    return 10.0f * log10f((max_val * max_val) / mse);
}

float compute_snr(const float *orig, const float *noisy, uint32_t len) {
    if (!orig || !noisy || len == 0) return 0.0f;
    float signal_power = 0.0f;
    float noise_power = 0.0f;
    for (uint32_t i = 0; i < len; i++) {
        float s = orig[i];
        float n = noisy[i] - orig[i];
        signal_power += s * s;
        noise_power += n * n;
    }
    if (noise_power <= 1e-10f) return 100.0f;
    return 10.0f * log10f(signal_power / noise_power);
}
