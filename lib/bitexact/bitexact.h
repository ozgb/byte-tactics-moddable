#ifndef BITEXACT_H
#define BITEXACT_H

#include <stddef.h>

/* The calling convention of the functions a /Gz game build calls: the library itself is cdecl, so such a build defines this as __cdecl first. */
#ifndef BX_CALL
#define BX_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* A signed 16-bit integer. */
typedef short bx_i16;
/* An unsigned 16-bit integer. */
typedef unsigned short bx_u16;
/* A signed 32-bit integer. */
typedef int bx_i32;
/* An unsigned 32-bit integer. */
typedef unsigned int bx_u32;
/* A signed 64-bit integer. */
#if defined(_MSC_VER)
typedef __int64 bx_i64;
#else
__extension__ typedef long long bx_i64;
#endif
/* An unsigned 64-bit integer. */
#if defined(_MSC_VER)
typedef unsigned __int64 bx_u64;
#else
__extension__ typedef unsigned long long bx_u64;
#endif

/* The correctly rounded square root, equal to x87 fsqrt at 53-bit precision (and to SSE2 sqrtsd and wasm f64.sqrt), done in integer code; finite non-negative x, a negative x giving the x87 indefinite NaN. */
BX_CALL double bx_sqrt(double x);

/* MSVC 5's _ftol: x truncated toward zero to a 64-bit integer, its low 32 bits; 0 for a NaN, an infinity or a magnitude of 2^63 or more. */
BX_CALL bx_i32 bx_ftol(double x);

/* The state of one linear congruential stream: the 32-bit seed, which each step replaces by seed * 214013 + 2531011 (mod 2^32). */
typedef struct {
    bx_u32 holdrand;
} bx_rand_state;

/* Restarts the stream s from seed. */
BX_CALL void bx_srand(bx_rand_state *s, bx_u32 seed);

/* The linear congruential generator: seed = seed * 214013 + 2531011, the value (seed >> 16) & 0x7fff, from 0 to 0x7fff. */
BX_CALL int bx_rand(bx_rand_state *s);

#ifdef __cplusplus
}
#endif

#endif
