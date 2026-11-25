#include "bitexact.h"

void bx_srand(bx_rand_state *s, bx_u32 seed)
{
    s->holdrand = seed;
}

int bx_rand(bx_rand_state *s)
{
    s->holdrand = s->holdrand * 214013u + 2531011u;
    return (int)((s->holdrand >> 16) & 0x7fff);
}
