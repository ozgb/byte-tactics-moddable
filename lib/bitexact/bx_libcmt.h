#ifndef BX_LIBCMT_H
#define BX_LIBCMT_H

#include "bitexact.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MSVC 5's _hypot: big = max(|x|, |y|), then sqrt((x / big)^2 + (y / big)^2) * big with each step rounded to a 64-bit mantissa as the x87 does at control word 0x133f and then stored as a double; finite arguments, results within double range. */
double bx_hypot(double x, double y);

/* The result of fpatan(sqrt((1 + x) * (1 - x)), x), the x87 code of MSVC 5's acos under 53-bit precision, stored as a double; x in [-1, 1], an x outside giving the x87 indefinite NaN; agrees with the CPU's fpatan except where its 64-bit result is not the correctly rounded one. */
double bx_acos(double x);

/* The float MSVC 5's sscanf("%f") stores for text: LIBCMT's strgtold12 (24 digits, 80-bit powers of ten), then the rounding of ld12cvt to 24 bits, which is not always the correctly rounded float; *end gets the first character not used, text when there is no number. */
float bx_strtof_msvc5(const char *text, const char **end);

/* The double MSVC 5's atof returns for text: leading isspace characters skipped, then the strgtold12 scan of bx_strtof_msvc5 rounded to 53 bits by ld12cvt, with LIBCMT's overflow to infinity and underflow to zero or a denormal. */
double bx_atof_msvc5(const char *text);

/* LIBCMT qsort: sorts num elements of width bytes with the library's own partition and swap order, so equal elements end in the same order and cmp sees the same calls; not stable. */
void bx_qsort(void *base, size_t num, size_t width, int (*cmp)(const void *, const void *));

#ifdef __cplusplus
}
#endif

#endif
