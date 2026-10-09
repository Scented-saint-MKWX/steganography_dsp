#include <stdio.h>

/* Forward declarations */
int test_prng_deterministic(void);
int test_bounded_random_range(void);
int test_key_check_hash(void);

int test_permute_roundtrip(void);
int test_permute_wrong_key(void);

int test_bessel_i0_known_values(void);
int test_kaiser_symmetry_and_endpoints(void);

int test_fft_roundtrip(void);
int test_fft_impulse_response(void);

int test_mirror_hermitian_symmetry(void);
int test_stft_img_roundtrip(void);

int test_dwt_haar_roundtrip(void);
int test_dwt_cascade_roundtrip(void);

int test_embed_extract_full_roundtrip(void);

int main(void) {
    int failures = 0;

    printf("========================================\n");
    printf("   C6748 DSP Steganography Host Tests   \n");
    printf("========================================\n\n");

    /* PRNG */
    failures += test_prng_deterministic();
    failures += test_bounded_random_range();
    failures += test_key_check_hash();

    /* Permutation */
    failures += test_permute_roundtrip();
    failures += test_permute_wrong_key();

    /* Kaiser Window */
    failures += test_bessel_i0_known_values();
    failures += test_kaiser_symmetry_and_endpoints();

    /* FFT */
    failures += test_fft_roundtrip();
    failures += test_fft_impulse_response();

    /* Mirror / STFT */
    failures += test_mirror_hermitian_symmetry();
    failures += test_stft_img_roundtrip();

    /* DWT */
    failures += test_dwt_haar_roundtrip();
    failures += test_dwt_cascade_roundtrip();

    /* Embed & Extract */
    failures += test_embed_extract_full_roundtrip();

    printf("\n========================================\n");
    if (failures == 0) {
        printf("  ALL HOST TESTS PASSED! (0 failures)   \n");
    } else {
        printf("  FAILED: %d tests failed!               \n", failures);
    }
    printf("========================================\n");

    return failures;
}
