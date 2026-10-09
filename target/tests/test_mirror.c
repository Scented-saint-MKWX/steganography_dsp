#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../core/mirror.h"

int test_mirror_hermitian_symmetry(void) {
    uint32_t H = 4;
    uint32_t N = 8;
    float col[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float spectrum[16]; /* 8 complex = 16 floats */
    float recovered[4];

    make_pair_column(col, spectrum, H, N);

    /* DC: index 0 */
    if (spectrum[0] != 1.0f || spectrum[1] != 0.0f) {
        printf("FAIL: DC bin mismatch (%f, %f)\n", spectrum[0], spectrum[1]);
        return 1;
    }

    /* Nyquist: index H = 4 -> 2*4 = 8 */
    if (spectrum[8] != 0.0f || spectrum[9] != 0.0f) {
        printf("FAIL: Nyquist bin mismatch (%f, %f)\n", spectrum[8], spectrum[9]);
        return 1;
    }

    /* Check mirror symmetry: spectrum[N-k] == spectrum[k] */
    for (uint32_t k = 1; k < H; k++) {
        if (spectrum[2 * (N - k)] != spectrum[2 * k]) {
            printf("FAIL: Mirror mismatch at k=%u: %f != %f\n", k, spectrum[2 * (N - k)], spectrum[2 * k]);
            return 1;
        }
    }

    unmake_pair_column(spectrum, recovered, H, N);
    for (uint32_t k = 0; k < H; k++) {
        if (recovered[k] != col[k]) {
            printf("FAIL: unmake_pair_column mismatch at %u: %f != %f\n", k, recovered[k], col[k]);
            return 1;
        }
    }

    printf("PASS: test_mirror_hermitian_symmetry\n");
    return 0;
}
