#include "internal.h"

double bx_sqrt(double x)
{
    bx_u64 bits = bx_bits_of(x);
    bx_u64 m, root = 0, rem = 0, pair;
    int e, exp2, i;

    if ((bits << 1) == 0)
        return x;
    if (bits >> 63)
        return bx_from_bits(BX_U64(0xFFF80000u, 0u));
    e = (int)((bits >> 52) & 0x7ff);
    if (e == 0x7ff)
        return x;
    m = bits & BX_MANTISSA;
    if (e == 0) {
        exp2 = -1074;
        while (!(m & (BX_ONE << 52))) {
            m <<= 1;
            exp2--;
        }
    } else {
        m |= BX_ONE << 52;
        exp2 = e - 1075;
    }
    if (exp2 & 1) {
        m <<= 1;
        exp2--;
    }
    for (i = 0; i < 56; i++) {
        pair = i <= 26 ? (m >> (52 - 2 * i)) & 3 : 0;
        rem = (rem << 2) | pair;
        if (rem >= ((root << 2) | 1)) {
            rem -= (root << 2) | 1;
            root = (root << 1) | 1;
        } else {
            root <<= 1;
        }
    }
    m = root >> 3;
    pair = root & 7;
    if (pair > 4 || (pair == 4 && (rem != 0 || (m & 1))))
        m++;
    if (m == (BX_ONE << 53)) {
        m >>= 1;
        exp2 += 2;
    }
    return bx_from_bits(((bx_u64)(exp2 / 2 + 1049) << 52) | (m & BX_MANTISSA));
}
