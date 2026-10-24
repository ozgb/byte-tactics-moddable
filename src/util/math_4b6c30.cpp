#include "../state/sim_globals.h"
// FLAGS: /Gi

// FUNCTION: 0x4b6c30
int __stdcall RandomInt(int range)
{
    if (range < 2)
        return 0;

    unsigned int seed = g_randomSeed;
    unsigned int q = seed / 127773;
    unsigned int correction = (q << 31) - seed / 127773;
    seed = seed * 16807 - correction;
    if ((int)seed <= 0)
        seed += 2147483647;
    g_randomSeed = seed;
    return seed % range;
}
