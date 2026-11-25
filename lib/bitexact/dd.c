#include "internal.h"

#include "bx_tables.h"

double bx_double_from_words(bx_u32 hi, bx_u32 lo)
{
    bx_double_bits b;
    b.w[1] = hi;
    b.w[0] = lo;
    return b.d;
}

void bx_two_sum(double a, double b, double *s, double *e)
{
    double t = a + b;
    double bb = t - a;
    *e = (a - (t - bb)) + (b - bb);
    *s = t;
}

void bx_two_prod(double a, double b, double *p, double *e)
{
    double t, ah, al, bh, bl;

    *p = a * b;
    t = 134217729.0 * a;
    ah = t - (t - a);
    al = a - ah;
    t = 134217729.0 * b;
    bh = t - (t - b);
    bl = b - bh;
    *e = ((ah * bh - *p) + ah * bl + al * bh) + al * bl;
}

void bx_dd_const(const bx_u32 *w, double *hi, double *lo)
{
    *hi = bx_double_from_words(w[0], w[1]);
    *lo = bx_double_from_words(w[2], w[3]);
}

static bx_dd bx_dd_make(double hi, double lo)
{
    bx_dd r;
    r.hi = hi + lo;
    r.lo = lo - (r.hi - hi);
    return r;
}

static bx_dd bx_dd_neg(bx_dd a)
{
    a.hi = -a.hi;
    a.lo = -a.lo;
    return a;
}

static bx_dd bx_dd_add(bx_dd a, bx_dd b)
{
    double s, e, t, f;
    bx_dd r;

    bx_two_sum(a.hi, b.hi, &s, &e);
    bx_two_sum(a.lo, b.lo, &t, &f);
    e += t;
    r = bx_dd_make(s, e);
    r.lo += f;
    return bx_dd_make(r.hi, r.lo);
}

static bx_dd bx_dd_mul(bx_dd a, bx_dd b)
{
    double p, e;

    bx_two_prod(a.hi, b.hi, &p, &e);
    e += a.hi * b.lo + a.lo * b.hi;
    return bx_dd_make(p, e);
}

static bx_dd bx_dd_mul_d(bx_dd a, double b)
{
    double p, e;

    bx_two_prod(a.hi, b, &p, &e);
    e += a.lo * b;
    return bx_dd_make(p, e);
}

static bx_dd bx_dd_div(bx_dd a, bx_dd b)
{
    double q1, q2, q3;
    bx_dd r, q;

    q1 = a.hi / b.hi;
    r = bx_dd_add(a, bx_dd_neg(bx_dd_mul_d(b, q1)));
    q2 = r.hi / b.hi;
    r = bx_dd_add(r, bx_dd_neg(bx_dd_mul_d(b, q2)));
    q3 = r.hi / b.hi;
    q = bx_dd_make(q1, q2);
    return bx_dd_add(q, bx_dd_make(q3, 0.0));
}

static bx_dd bx_dd_from_double(double a)
{
    bx_dd r;
    r.hi = a;
    r.lo = 0.0;
    return r;
}

static bx_dd bx_dd_product(double a, double b)
{
    bx_dd r;
    bx_two_prod(a, b, &r.hi, &r.lo);
    return r;
}

static bx_dd bx_dd_atan_ratio(double n, double d)
{
    bx_dd num, den, u, w, p, atan_u, c;
    bx_dd table;
    double th, tl;
    int k, m;

    if (n == 0.0)
        return bx_dd_from_double(0.0);
    k = (int)(n * 16.0 / d + 0.5);
    num = bx_dd_add(bx_dd_from_double(16.0 * n), bx_dd_neg(bx_dd_product((double)k, d)));
    den = bx_dd_add(bx_dd_from_double(16.0 * d), bx_dd_product((double)k, n));
    u = bx_dd_div(num, den);
    w = bx_dd_mul(u, u);
    bx_dd_const(bx_atan_series + 4 * 11, &p.hi, &p.lo);
    for (m = 10; m >= 0; m--) {
        bx_dd_const(bx_atan_series + 4 * m, &c.hi, &c.lo);
        p = bx_dd_add(bx_dd_mul(p, w), c);
    }
    atan_u = bx_dd_mul(u, p);
    bx_dd_const(bx_atan_sixteenths + 4 * k, &th, &tl);
    table.hi = th;
    table.lo = tl;
    return bx_dd_add(table, atan_u);
}

bx_dd bx_dd_atan2_nonneg(double s, double x)
{
    double ax = x < 0 ? -x : x, ch, cl;
    bx_dd a, c;

    if (s == 0.0 && ax == 0.0)
        return bx_dd_from_double(0.0);
    if (s <= ax) {
        a = bx_dd_atan_ratio(s, ax);
    } else {
        bx_dd_const(bx_pi_pi_half + 4, &ch, &cl);
        c.hi = ch;
        c.lo = cl;
        a = bx_dd_add(c, bx_dd_neg(bx_dd_atan_ratio(ax, s)));
    }
    if (x < 0) {
        bx_dd_const(bx_pi_pi_half, &ch, &cl);
        c.hi = ch;
        c.lo = cl;
        a = bx_dd_add(c, bx_dd_neg(a));
    }
    return a;
}

static bx_u64 bx_double_bits64(double d)
{
    bx_double_bits b;
    b.d = d;
    return ((bx_u64)b.w[1] << 32) | b.w[0];
}

static bx_u64 bx_round_shift(bx_u64 v, int shift)
{
    bx_u64 q, rem, half;

    if (shift <= 0)
        return v << (-shift);
    if (shift > 64)
        return 0;
    if (shift == 64) {
        q = 0;
        rem = v;
        half = (bx_u64)1 << 63;
    } else {
        q = v >> shift;
        rem = v & (((bx_u64)1 << shift) - 1);
        half = (bx_u64)1 << (shift - 1);
    }
    if (rem > half || (rem == half && (q & 1)))
        q++;
    return q;
}

double bx_dd_to_x87_double(bx_dd v)
{
    bx_u64 hb = bx_double_bits64(v.hi), lb = bx_double_bits64(v.lo);
    bx_u64 mh, ml, n, r;
    int eh, el, neg, shift;

    if (v.hi == 0.0)
        return 0.0;
    eh = (int)(hb >> 52) & 0x7ff;
    mh = (hb & (((bx_u64)1 << 52) - 1)) | ((bx_u64)1 << 52);
    n = mh << 11;
    if (v.lo == 0.0)
        return bx_ext64_to_double(n, eh - 1075 - 11);
    neg = (int)(lb >> 63);
    el = (int)(lb >> 52) & 0x7ff;
    ml = (lb & (((bx_u64)1 << 52) - 1)) | ((bx_u64)1 << 52);
    shift = (eh - 1075 - 11) - (el - 1075);
    if (neg && mh == ((bx_u64)1 << 52)) {
        r = bx_round_shift(ml, shift + 1);
        if (r == 0)
            return bx_ext64_to_double(n, eh - 1075 - 11);
        return bx_ext64_to_double((bx_u64)0 - r, eh - 1075 - 12);
    }
    r = bx_round_shift(ml, shift);
    return bx_ext64_to_double(neg ? n - r : n + r, eh - 1075 - 11);
}
