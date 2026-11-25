#include "bx_game.h"

bx_u32 bx_explosion_frame_draws(int size)
{
    return size > 0 ? (bx_u32)size * (bx_u32)size : 0;
}

static bx_u32 explosion_draws(int count, int start, int end)
{
    bx_u32 total = 0;
    int radius = start;
    int step = (end - start) / count;
    int i;

    for (i = 0; i < count; i++) {
        total += bx_explosion_frame_draws(radius);
        radius += step;
    }
    return total;
}

bx_u32 bx_init_explosions_draws(void)
{
    return explosion_draws(24 / 2, 64, 8) + explosion_draws(30 / 2, 128, 16) + explosion_draws(30 / 2, 200, 32);
}

void bx_init_explosions_rand(bx_rand_state *s)
{
    bx_u32 n = bx_init_explosions_draws();

    while (n--)
        bx_rand(s);
}
