#ifndef KAISER_H
#define KAISER_H
#include <stdint.h>

/* Kaiser window generation using I0 Bessel function.
 * See EXPLANATION.md Section 3.3. */

/* Compute modified Bessel function of first kind, order 0.
 * Uses convergent power series: I0(x) = sum_{k=0}^{inf} ((x/2)^k / k!)^2 */
float bessel_i0(float x);

/* Generate Kaiser window of length N with parameter beta.
 * Output written to win[0..N-1]. */
void kaiser_window(float *win, uint32_t N, float beta);

#endif
