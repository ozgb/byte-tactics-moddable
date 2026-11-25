#include "bx_stl.h"

void bx_random_shuffle_int(bx_rand_state *s, int *first, int *last)
{
    int *x = first;
    int d = 1;

    if (first == last)
        return;
    for (; ++x != last; ++d) {
        unsigned long rm = 0x7fff;
        unsigned long rn = (unsigned long)bx_rand(s) & 0x7fff;
        int t;

        for (; rm < (unsigned long)d && rm != ~0UL; rm = rm << 15 | 0x7fff)
            rn = rn << 15 | 0x7fff;
        t = *x;
        *x = first[(int)(rn % (unsigned long)d)];
        first[(int)(rn % (unsigned long)d)] = t;
    }
}
