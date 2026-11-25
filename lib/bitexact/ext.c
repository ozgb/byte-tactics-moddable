#include "internal.h"

#define BX_TOP_BIT (BX_ONE << 63)

typedef struct {
    bx_u64 hi;
    bx_u64 lo;
} bx_u128;

bx_u64 bx_bits_of(double d)
{
    bx_double_bits b;
    b.d = d;
    return ((bx_u64)b.w[1] << 32) | b.w[0];
}

double bx_from_bits(bx_u64 u)
{
    bx_double_bits b;
    b.w[1] = (bx_u32)(u >> 32);
    b.w[0] = (bx_u32)u;
    return b.d;
}

static bx_u128 bx_mul_64(bx_u64 a, bx_u64 b)
{
    bx_u64 a0 = a & 0xffffffffu, a1 = a >> 32, b0 = b & 0xffffffffu, b1 = b >> 32;
    bx_u64 p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
    bx_u64 mid = (p00 >> 32) + (p01 & 0xffffffffu) + (p10 & 0xffffffffu);
    bx_u128 r;
    r.lo = (p00 & 0xffffffffu) | (mid << 32);
    r.hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
    return r;
}

static int bx_clz64(bx_u64 v)
{
    int n = 0;
    if (!(v >> 32)) {
        n += 32;
        v <<= 32;
    }
    if (!(v >> 48)) {
        n += 16;
        v <<= 16;
    }
    if (!(v >> 56)) {
        n += 8;
        v <<= 8;
    }
    if (!(v >> 60)) {
        n += 4;
        v <<= 4;
    }
    if (!(v >> 62)) {
        n += 2;
        v <<= 2;
    }
    if (!(v >> 63))
        n += 1;
    return n;
}

static bx_ext bx_ext_zero(void)
{
    bx_ext r;
    r.m = 0;
    r.e = 0;
    return r;
}

static bx_ext bx_ext_from_u128(bx_u128 v, int e, int sticky)
{
    bx_ext r;
    int s = 0, sh;
    bx_u64 g;

    if (v.hi == 0 && v.lo == 0)
        return bx_ext_zero();
    if (v.hi == 0) {
        v.hi = v.lo;
        v.lo = 0;
        s = 64;
    }
    sh = bx_clz64(v.hi);
    if (sh) {
        v.hi = (v.hi << sh) | (v.lo >> (64 - sh));
        v.lo <<= sh;
        s += sh;
    }
    g = v.lo >> 63;
    r.m = v.hi;
    r.e = e - s + 64;
    if (g && (sticky || (v.lo << 1) != 0 || (r.m & 1))) {
        r.m++;
        if (r.m == 0) {
            r.m = BX_TOP_BIT;
            r.e++;
        }
    }
    return r;
}

bx_ext bx_ext_of_double(double d)
{
    bx_u64 bits = bx_bits_of(d) & BX_NO_SIGN;
    bx_u64 m = bits & BX_MANTISSA;
    int e = (int)(bits >> 52), sh;
    bx_ext r;

    if (bits == 0)
        return bx_ext_zero();
    if (e == 0) {
        e = 1;
    } else {
        m |= BX_ONE << 52;
    }
    sh = bx_clz64(m);
    r.m = m << sh;
    r.e = e - 1075 - sh;
    return r;
}

double bx_double_of_ext(bx_ext x)
{
    int drop, lead;
    bx_u64 q, guard, below, bits;

    if (x.m == 0)
        return 0.0;
    lead = x.e + 63;
    drop = lead >= -1022 ? 11 : 11 + (-1022 - lead);
    if (drop > 64)
        return 0.0;
    if (drop == 64) {
        q = 0;
        guard = x.m >> 63;
        below = (x.m << 1) != 0;
    } else {
        q = x.m >> drop;
        guard = (x.m >> (drop - 1)) & 1;
        below = (x.m & ((BX_ONE << (drop - 1)) - 1)) != 0;
    }
    if (guard && (below || (q & 1)))
        q++;
    if (lead >= -1022) {
        if (q == (BX_ONE << 53)) {
            q >>= 1;
            lead++;
        }
        if (lead > 1023)
            return bx_from_bits(BX_U64(0x7FF00000u, 0u));
        bits = ((bx_u64)(lead + 1023) << 52) | (q & BX_MANTISSA);
    } else {
        bits = q;
    }
    return bx_from_bits(bits);
}

bx_ext bx_ext_mul(bx_ext a, bx_ext b)
{
    if (a.m == 0 || b.m == 0)
        return bx_ext_zero();
    return bx_ext_from_u128(bx_mul_64(a.m, b.m), a.e + b.e, 0);
}

bx_ext bx_ext_add(bx_ext a, bx_ext b)
{
    bx_u128 sum;
    bx_ext t;
    int d, sticky = 0, e;
    bx_u64 bh, bl;

    if (a.m == 0)
        return b;
    if (b.m == 0)
        return a;
    if (b.e > a.e) {
        t = a;
        a = b;
        b = t;
    }
    d = a.e - b.e;
    if (d >= 128) {
        bh = 0;
        bl = 0;
        sticky = 1;
    } else if (d >= 64) {
        bh = 0;
        bl = d == 64 ? b.m : b.m >> (d - 64);
        sticky = d > 64 && (b.m & ((BX_ONE << (d - 64)) - 1)) != 0;
    } else if (d == 0) {
        bh = b.m;
        bl = 0;
    } else {
        bh = b.m >> d;
        bl = b.m << (64 - d);
    }
    sum.lo = bl;
    sum.hi = a.m + bh;
    e = a.e - 64;
    if (sum.hi < a.m) {
        sticky |= (int)(sum.lo & 1);
        sum.lo = (sum.lo >> 1) | (sum.hi << 63);
        sum.hi = (sum.hi >> 1) | BX_TOP_BIT;
        e++;
    }
    return bx_ext_from_u128(sum, e, sticky);
}

bx_ext bx_ext_div(bx_ext a, bx_ext b)
{
    bx_u64 ma = a.m >> 11, mb = b.m >> 11, q = 0, rem;
    int xa = a.e + 11, xb = b.e + 11, i, guard, sticky;
    bx_ext r;

    if (a.m == 0)
        return bx_ext_zero();
    if (ma < mb) {
        ma <<= 1;
        xa--;
    }
    rem = ma;
    for (i = 0; i < 64; i++) {
        q <<= 1;
        if (rem >= mb) {
            rem -= mb;
            q |= 1;
        }
        rem <<= 1;
    }
    guard = rem >= mb;
    if (guard)
        rem -= mb;
    sticky = rem != 0;
    r.m = q;
    r.e = xa - xb - 63;
    if (guard && (sticky || (q & 1))) {
        r.m++;
        if (r.m == 0) {
            r.m = BX_TOP_BIT;
            r.e++;
        }
    }
    return r;
}

static int bx_u128_ge(bx_u128 a, bx_u128 b)
{
    return a.hi > b.hi || (a.hi == b.hi && a.lo >= b.lo);
}

static bx_u128 bx_u128_sub(bx_u128 a, bx_u128 b)
{
    bx_u128 r;
    r.lo = a.lo - b.lo;
    r.hi = a.hi - b.hi - (a.lo < b.lo);
    return r;
}

static bx_u128 bx_u128_shl(bx_u128 a, int n)
{
    bx_u128 r;
    r.hi = (a.hi << n) | (a.lo >> (64 - n));
    r.lo = a.lo << n;
    return r;
}

bx_ext bx_ext_sqrt(bx_ext a)
{
    bx_u128 rem, root, cand;
    int odd = a.e & 1, i, pos, guard, sticky;
    int shift = odd ? 65 : 66;
    bx_u64 pair;
    bx_ext r;

    if (a.m == 0)
        return bx_ext_zero();
    rem.hi = rem.lo = 0;
    root.hi = root.lo = 0;
    for (i = 0; i < 65; i++) {
        pos = 128 - 2 * i;
        pair = 0;
        if (pos - shift >= 0 && pos - shift < 64)
            pair = (a.m >> (pos - shift)) & 3;
        else if (pos - shift == -1)
            pair = (a.m << 1) & 3;
        rem = bx_u128_shl(rem, 2);
        rem.lo |= pair;
        cand = bx_u128_shl(root, 2);
        cand.lo |= 1;
        root = bx_u128_shl(root, 1);
        if (bx_u128_ge(rem, cand)) {
            rem = bx_u128_sub(rem, cand);
            root.lo |= 1;
        }
    }
    guard = (int)(root.lo & 1);
    sticky = rem.hi != 0 || rem.lo != 0;
    r.m = (root.hi << 63) | (root.lo >> 1);
    r.e = (a.e - shift) / 2 + 1;
    if (guard && (sticky || (r.m & 1))) {
        r.m++;
        if (r.m == 0) {
            r.m = BX_TOP_BIT;
            r.e++;
        }
    }
    return r;
}

double bx_ext64_to_double(bx_u64 m, int e)
{
    bx_ext x;
    x.m = m;
    x.e = e;
    return bx_double_of_ext(x);
}

bx_i32 bx_round_to_int(double v)
{
    bx_i64 t;
    double f;

    if (!(v > -4e18 && v < 4e18))
        return BX_INT_MIN;
    t = (bx_i64)v;
    f = v - (double)t;
    if (v >= 0) {
        if (f > 0.5 || (f == 0.5 && (t & 1)))
            t++;
    } else {
        if (f < -0.5 || (f == -0.5 && (t & 1)))
            t--;
    }
    if (t > 2147483647 || t < -2147483647 - 1)
        return BX_INT_MIN;
    return (bx_i32)t;
}

static int bx_bit_length(bx_u64 v)
{
    int n = 0;
    while (v) {
        n++;
        v >>= 1;
    }
    return n;
}

double bx_mul_ext(bx_i32 x, bx_u32 mant_hi, bx_u32 mant_lo, bx_u32 sign_exp)
{
    bx_u32 ax;
    bx_u64 phi, plo, carry, top, sticky, r, rem;
    bx_u32 low32;
    int sh, neg, exp;
    bx_double_bits b;

    if (x == 0)
        return 0.0;
    ax = x < 0 ? (bx_u32)0 - (bx_u32)x : (bx_u32)x;
    neg = (x < 0) != ((sign_exp & 0x8000) != 0);
    phi = (bx_u64)ax * (bx_u64)mant_hi;
    plo = (bx_u64)ax * (bx_u64)mant_lo;
    carry = phi + (plo >> 32);
    low32 = (bx_u32)(plo & 0xffffffffu);
    sh = bx_bit_length(carry) - 32;
    top = (carry << (32 - sh)) | ((bx_u64)low32 >> sh);
    sticky = (bx_u64)low32 & (((bx_u64)1 << sh) - 1);
    r = top >> 11;
    rem = top & 0x7ff;
    if (rem > 0x400 || (rem == 0x400 && (sticky || (r & 1))))
        r++;
    exp = (int)(sign_exp & 0x7fff) - 16383 + sh;
    if (r == ((bx_u64)1 << 53)) {
        r >>= 1;
        exp++;
    }
    b.w[1] = ((bx_u32)(exp + 1023) << 20) | (bx_u32)((r >> 32) & 0xfffff);
    b.w[0] = (bx_u32)(r & 0xffffffffu);
    if (neg)
        b.w[1] |= 0x80000000u;
    return b.d;
}
