#ifndef BX_INTERNAL_H
#define BX_INTERNAL_H

#include "bitexact.h"

#define BX_U64(hi, lo) (((bx_u64)(hi) << 32) | (bx_u64)(lo))
#define BX_ONE ((bx_u64)1)
#define BX_MANTISSA BX_U64(0xFFFFFu, 0xFFFFFFFFu)
#define BX_NO_SIGN BX_U64(0x7FFFFFFFu, 0xFFFFFFFFu)
#define BX_INT_MIN (-2147483647 - 1)

#ifdef __cplusplus
extern "C" {
#endif

/* A double and its two 32-bit words, the low word first. */
typedef union {
    double d;
    bx_u32 w[2];
} bx_double_bits;

/* An 80-bit style value: the 64-bit mantissa m (top bit set unless zero) times 2^e. */
typedef struct {
    bx_u64 m;
    int e;
} bx_ext;

/* A double-double: the unevaluated sum hi + lo of two doubles. */
typedef struct {
    double hi;
    double lo;
} bx_dd;

/* The 64 bits of a double. */
bx_u64 bx_bits_of(double d);

/* The double with the 64 bits u. */
double bx_from_bits(bx_u64 u);

/* The double d as a 64-bit mantissa and an exponent, subnormals normalised, the sign dropped. */
bx_ext bx_ext_of_double(double d);

/* The value x rounded to nearest even at 53 bits, with subnormals and overflow to infinity. */
double bx_double_of_ext(bx_ext x);

/* The product a * b rounded to nearest even at 64 bits. */
bx_ext bx_ext_mul(bx_ext a, bx_ext b);

/* The sum a + b rounded to nearest even at 64 bits. */
bx_ext bx_ext_add(bx_ext a, bx_ext b);

/* The quotient a / b rounded to nearest even at 64 bits. */
bx_ext bx_ext_div(bx_ext a, bx_ext b);

/* The square root of a rounded to nearest even at 64 bits. */
bx_ext bx_ext_sqrt(bx_ext a);

/* The value m * 2^e rounded to a double, m being a 64-bit mantissa. */
double bx_ext64_to_double(bx_u64 m, int e);

/* The double v rounded to the nearest int, ties to even, or INT_MIN when it does not fit. */
bx_i32 bx_round_to_int(double v);

/* The product of the int x and the 64-bit mantissa mant_hi:mant_lo with the sign/exponent word sign_exp, rounded to 53 bits. */
double bx_mul_ext(bx_i32 x, bx_u32 mant_hi, bx_u32 mant_lo, bx_u32 sign_exp);

/* The double whose high word is hi and low word lo. */
double bx_double_from_words(bx_u32 hi, bx_u32 lo);

/* The sum a + b as s and its exact rounding error e. */
void bx_two_sum(double a, double b, double *s, double *e);

/* The product a * b as p and its exact rounding error e. */
void bx_two_prod(double a, double b, double *p, double *e);

/* The double-double whose words are the four 32-bit values at w (hi high, hi low, lo high, lo low). */
void bx_dd_const(const bx_u32 *w, double *hi, double *lo);

/* The double-double atan2(s, x) for s >= 0. */
bx_dd bx_dd_atan2_nonneg(double s, double x);

/* The double-double v rounded to a 64-bit mantissa and then to a double. */
double bx_dd_to_x87_double(bx_dd v);

#ifdef __cplusplus
}
#endif

#endif
