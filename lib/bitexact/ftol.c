#include "bitexact.h"

bx_i32 bx_ftol(double x)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    bx_u32 hi, lo, mhi, low;
    int e, shift;

    b.d = x;
    lo = b.w[0];
    hi = b.w[1];
    e = (int)((hi >> 20) & 0x7ffu);
    if (e < 1023 || e >= 1023 + 63)
        return 0;
    mhi = (hi & 0xfffffu) | 0x100000u;
    shift = e - 1075;
    if (shift >= 32)
        low = 0;
    else if (shift >= 0)
        low = lo << shift;
    else if (shift > -32)
        low = (lo >> -shift) | (mhi << (32 + shift));
    else
        low = mhi >> (-shift - 32);
    return (bx_i32)(hi >> 31 ? 0u - low : low);
}
