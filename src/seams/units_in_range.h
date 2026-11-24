#ifndef UNITS_IN_RANGE_H
#define UNITS_IN_RANGE_H

#include "../../interfaces/seams.h"
#include "../../interfaces/map.h"
#include "../../link/globals.h"
#include "../game/game.h"
#include "../units/unit.h"

template <class Visit>
inline void ForEachUnitInRange(const Vec3* pos, int range, Visit visit)
{
    unsigned short ids[256];
    int skip = 0;
    for (;;) {
        int count = g_seams.map->UnitsInRange(pos, range, skip, ids, 256);
        for (int i = 0; i < count; i++)
            visit(&g_game->units[ids[i]]);
        if (count < 256)
            return;
        skip += count;
    }
}

#endif
