#ifndef BX_GAME_H
#define BX_GAME_H

#include "bitexact.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FixedSine, 0x4b70a0: the sine table entry (sin * 0x2000) for the top 10 bits of a 16-bit angle (0x10000 is a full turn). */
bx_i32 bx_FixedSine(bx_u16 angle);

/* The cosine at 0x4b70c0: bx_FixedSine a quarter turn on, read from the table's own cosine view. */
bx_i32 bx_fixed_cos(bx_u16 angle);

/* The (a * b) >> 32 at 0x4b70e0: the high word of the signed 64-bit product. */
bx_i32 bx_mul_hi32(bx_i32 a, bx_i32 b);

/* ScaledSin: scale * sin(angle), the angle rounded to a table entry and the product rounded to nearest. */
bx_i32 bx_ScaledSin(bx_i16 angle, bx_i32 scale);

/* ScaledCos: scale * cos(angle), the angle rounded to a table entry and the product rounded to nearest. */
bx_i32 bx_ScaledCos(bx_i16 angle, bx_i32 scale);

/* AngleOfVector: the angle (0x8000 is half a turn) of the vector (z, x), that is atan2(x, z), rounded to nearest even; needs a 53-bit FPU precision and round to nearest, as the game sets (_control87 0x9001F), and agrees with the exe's fpatan except where the exact result is within about 1e-13 of a half. */
bx_i32 bx_AngleOfVector(bx_i32 x, bx_i32 z);

/* RotatePoint2D: rotates xz[0] (x) and xz[1] (z) by angle in place, doing nothing when the angle is 0; exact for the game's FPU mode (_control87 0x9001F), a result out of int range becoming INT_MIN as fistp stores it. */
void bx_RotatePoint2D(bx_i16 angle, bx_i32 *xz);

/* The matrix builder at 0x4b71a7: fills the caller's 3x3 matrix m from angles[0], angles[1] and angles[2], the words the original reads at +0xc, +0xe and +0x10 of its argument. */
void bx_build_rotation_matrix(const bx_i16 *angles, bx_i32 m[9]);

/* The matrix apply at 0x4b72e2: writes the vector in (3 ints) times the matrix m to out, reading and writing in the original's order so that in and out may be the same. */
void bx_apply_rotation_matrix(const bx_i32 m[9], const bx_i32 *in, bx_i32 *out);

/* MulDivTrunc: a * b / c with a 64-bit product, truncated toward zero, or 0 when c is 0; the exe traps when the quotient does not fit in an int, here it is INT_MIN. */
bx_i32 bx_MulDivTrunc(bx_i32 a, bx_i32 b, bx_i32 c);

/* The number of rand calls BuildExplosionFrame (0x420d20) makes for a size x size frame: one per pixel. */
bx_u32 bx_explosion_frame_draws(int size);

/* The number of rand calls InitExplosions (0x420620) makes in all: its three explosions' frames summed. */
bx_u32 bx_init_explosions_draws(void);

/* Advances the stream s by the draws of InitExplosions (0x420620), the load thread's draws between the seed and the start-position shuffle. */
void bx_init_explosions_rand(bx_rand_state *s);

#ifdef __cplusplus
}
#endif

#endif
