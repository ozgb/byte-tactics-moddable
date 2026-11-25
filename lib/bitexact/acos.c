#include "bx_libcmt.h"

#include "internal.h"

/* FUNCTION: 0x4e67f0 */
double bx_acos(double x)
{
    double s, ax = x < 0 ? -x : x;

    if (x != x)
        return x;
    if (ax > 1.0)
        return bx_sqrt(-1.0);
    s = bx_sqrt((1.0 + x) * (1.0 - x));
    return bx_dd_to_x87_double(bx_dd_atan2_nonneg(s, x));
}
