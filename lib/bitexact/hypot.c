#include "bx_libcmt.h"

#include "internal.h"

double bx_hypot(double x, double y)
{
    bx_u64 xb = bx_bits_of(x) & BX_NO_SIGN, yb = bx_bits_of(y) & BX_NO_SIGN;
    double a = bx_from_bits(xb), b = bx_from_bits(yb), big, ra, rb, sum, root;
    bx_ext eb, ext_sum;

    if (xb == BX_U64(0x7FF00000u, 0u) || yb == BX_U64(0x7FF00000u, 0u))
        return bx_from_bits(BX_U64(0x7FF00000u, 0u));
    if ((xb >> 52) == 0x7ff)
        return x;
    if ((yb >> 52) == 0x7ff)
        return y;
    big = xb > yb ? a : b;
    if (xb == 0 && yb == 0)
        return 0.0;
    eb = bx_ext_of_double(big);
    ra = bx_double_of_ext(bx_ext_div(bx_ext_of_double(a), eb));
    rb = bx_double_of_ext(bx_ext_div(bx_ext_of_double(b), eb));
    ext_sum = bx_ext_add(bx_ext_mul(bx_ext_of_double(ra), bx_ext_of_double(ra)),
                         bx_ext_mul(bx_ext_of_double(rb), bx_ext_of_double(rb)));
    sum = bx_double_of_ext(ext_sum);
    root = bx_double_of_ext(bx_ext_sqrt(bx_ext_of_double(sum)));
    return bx_double_of_ext(bx_ext_mul(bx_ext_of_double(root), eb));
}
