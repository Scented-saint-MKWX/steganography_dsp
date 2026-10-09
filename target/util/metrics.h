#ifndef METRICS_H
#define METRICS_H
#include <stdint.h>

float compute_mse(const float *a, const float *b, uint32_t len);
float compute_psnr(const float *orig, const float *recon, uint32_t len, float max_val);
float compute_snr(const float *orig, const float *noisy, uint32_t len);

#endif
