#include "bx_game.h"

#include "internal.h"

#include "bx_tables.h"

static bx_i32 bx_shrd13(bx_i64 product)
{
    return (bx_i32)(bx_u32)(bx_u32)((bx_u64)product >> 13);
}

static bx_i32 bx_mul13(bx_i32 a, bx_i32 b)
{
    return bx_shrd13((bx_i64)a * (bx_i64)b);
}

static bx_i32 bx_add(bx_i32 a, bx_i32 b)
{
    return (bx_i32)((bx_u32)a + (bx_u32)b);
}

static bx_i32 bx_sub(bx_i32 a, bx_i32 b)
{
    return (bx_i32)((bx_u32)a - (bx_u32)b);
}

static bx_i32 bx_neg(bx_i32 a)
{
    return (bx_i32)((bx_u32)0 - (bx_u32)a);
}

/* FUNCTION: 0x4b70a0 */
bx_i32 bx_FixedSine(bx_u16 angle)
{
    return bx_sin_table[(angle >> 7) & 0x1ff];
}

/* FUNCTION: 0x4b70c0 */
bx_i32 bx_fixed_cos(bx_u16 angle)
{
    return bx_sin_table[128 + ((angle >> 7) & 0x1ff)];
}

/* FUNCTION: 0x4b70e0 */
bx_i32 bx_mul_hi32(bx_i32 a, bx_i32 b)
{
    return (bx_i32)((bx_i64)a * (bx_i64)b >> 32);
}

static bx_i32 bx_scaled(bx_i32 entry, bx_i32 scale)
{
    return bx_shrd13((bx_i64)entry * (bx_i64)scale + 0x1000);
}

/* FUNCTION: 0x4b70ef */
bx_i32 bx_ScaledSin(bx_i16 angle, bx_i32 scale)
{
    bx_u32 index = (((bx_u32)(bx_i32)angle + 0x20) >> 7) & 0x1ff;
    return bx_scaled(bx_sin_table[index], scale);
}

/* FUNCTION: 0x4b7123 */
bx_i32 bx_ScaledCos(bx_i16 angle, bx_i32 scale)
{
    bx_u32 index = (((bx_u32)(bx_u16)angle + 0x4020) >> 7) & 0x1ff;
    return bx_scaled(bx_sin_table[index], scale);
}

/* FUNCTION: 0x4b715a */
bx_i32 bx_AngleOfVector(bx_i32 x, bx_i32 z)
{
    double n, d, num, den, u1, u2, w, p, pe, rem, poly, tail;
    double ch, cl, ah, ae, al, qh, ql, th, te, tl, k_rad, v;
    double c_hi, c_lo, sign;
    bx_u32 ax, bz;
    int k, swap, z_neg;

    if (x == 0 && z == 0)
        return 0;
    ax = x < 0 ? (bx_u32)0 - (bx_u32)x : (bx_u32)x;
    bz = z < 0 ? (bx_u32)0 - (bx_u32)z : (bx_u32)z;
    z_neg = z < 0;
    swap = ax > bz;
    n = (double)(swap ? bz : ax);
    d = (double)(swap ? ax : bz);

    k = (int)(n * 16.0 / d + 0.5);
    num = 16.0 * n - (double)k * d;
    den = 16.0 * d + (double)k * n;
    u1 = num / den;
    bx_two_prod(u1, den, &p, &pe);
    rem = (num - p) - pe;
    u2 = rem / den;
    w = u1 * u1;
    poly = 1.0 / 17.0;
    poly = w * poly - 1.0 / 15.0;
    poly = w * poly + 1.0 / 13.0;
    poly = w * poly - 1.0 / 11.0;
    poly = w * poly + 1.0 / 9.0;
    poly = w * poly - 1.0 / 7.0;
    poly = w * poly + 1.0 / 5.0;
    poly = w * poly - 1.0 / 3.0;
    tail = u1 * w * poly + u2 / (1.0 + w);
    bx_dd_const(bx_atan_sixteenths + 4 * k, &ch, &cl);
    bx_two_sum(ch, u1, &ah, &ae);
    al = cl + ae + tail;

    if (!swap && !z_neg) {
        c_hi = 0.0;
        c_lo = 0.0;
        sign = 1.0;
    } else if (!swap) {
        bx_dd_const(bx_pi_pi_half, &c_hi, &c_lo);
        sign = -1.0;
    } else {
        bx_dd_const(bx_pi_pi_half + 4, &c_hi, &c_lo);
        sign = z_neg ? 1.0 : -1.0;
    }
    qh = sign * ah;
    ql = sign * al;
    bx_two_sum(c_hi, qh, &th, &te);
    tl = te + c_lo + ql;

    k_rad = bx_double_from_words(bx_rad_to_angle16_bits[0], bx_rad_to_angle16_bits[1]);
    bx_two_prod(th, k_rad, &p, &pe);
    v = p + (pe + tl * k_rad);
    k = bx_round_to_int(v);
    return x < 0 ? bx_neg(k) : k;
}

/* FUNCTION: 0x4b7173 */
void bx_RotatePoint2D(bx_i16 angle, bx_i32 *xz)
{
    const bx_u32 *e;
    double xc, yc, xs, ys;
    bx_i32 x, y;

    if (angle == 0)
        return;
    e = bx_sincos_table + 5 * (bx_u32)(bx_u16)angle;
    x = xz[0];
    y = xz[1];
    xc = bx_mul_ext(x, e[2], e[3], e[4] & 0xffff);
    yc = bx_mul_ext(y, e[2], e[3], e[4] & 0xffff);
    xs = bx_mul_ext(x, e[0], e[1], e[4] >> 16);
    ys = bx_mul_ext(y, e[0], e[1], e[4] >> 16);
    xz[0] = bx_round_to_int(xc - ys);
    xz[1] = bx_round_to_int(yc + xs);
}

/* FUNCTION: 0x4b71a7 */
void bx_build_rotation_matrix(const bx_i16 *angles, bx_i32 m[9])
{
    bx_i32 c0 = bx_fixed_cos((bx_u16)angles[0]);
    bx_i32 s0 = bx_FixedSine((bx_u16)angles[0]);
    bx_i32 c1 = bx_fixed_cos((bx_u16)angles[1]);
    bx_i32 s1 = bx_FixedSine((bx_u16)angles[1]);
    bx_i32 c2 = bx_fixed_cos((bx_u16)angles[2]);
    bx_i32 s2 = bx_FixedSine((bx_u16)angles[2]);

    m[0] = bx_mul13(c1, c2);
    m[1] = bx_add(bx_mul13(s2, c0), bx_mul13(bx_mul13(s1, s0), c2));
    m[2] = bx_sub(bx_mul13(s0, s2), bx_mul13(bx_mul13(c0, s1), c2));
    m[3] = bx_neg(bx_mul13(c1, s2));
    m[4] = bx_sub(bx_mul13(c0, c2), bx_mul13(bx_mul13(s0, s1), s2));
    m[5] = bx_add(bx_mul13(bx_mul13(c0, s1), s2), bx_mul13(s0, c2));
    m[6] = s1;
    m[7] = bx_neg(bx_mul13(s0, c1));
    m[8] = bx_mul13(c0, c1);
}

/* FUNCTION: 0x4b72e2 */
void bx_apply_rotation_matrix(const bx_i32 m[9], const bx_i32 *in, bx_i32 *out)
{
    out[0] = bx_add(bx_add(bx_mul13(in[0], m[0]), bx_mul13(in[1], m[3])), bx_mul13(in[2], m[6]));
    out[1] = bx_add(bx_add(bx_mul13(in[0], m[1]), bx_mul13(in[1], m[4])), bx_mul13(in[2], m[7]));
    out[2] = bx_add(bx_add(bx_mul13(in[0], m[2]), bx_mul13(in[1], m[5])), bx_mul13(in[2], m[8]));
}

/* FUNCTION: 0x4b7381 */
bx_i32 bx_MulDivTrunc(bx_i32 a, bx_i32 b, bx_i32 c)
{
    bx_i64 q;

    if (c == 0)
        return 0;
    q = (bx_i64)a * (bx_i64)b / (bx_i64)c;
    if (q > 2147483647 || q < -2147483647 - 1)
        return BX_INT_MIN;
    return (bx_i32)q;
}
