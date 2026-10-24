#include "../util/vec3.h"

struct UnitDef;

#pragma pack(push, 1)
#include "../units/unit.h"

#include "../units/unit_def.h"

#include "feature.h"

#include "cell.h"

#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

// FUNCTION: 0x47e2d0
int __stdcall CanPlaceFootprintAt(Unit* unit, Vec3* pos)
{
    Point16 footprint = unit->footprint;
    Vec3 v = *pos;
    UnitDef* def = unit->def;
    short a = (v.x - (footprint.x << 19) + 0x80000) >> 20;
    short c = (v.z - (footprint.y << 19) + 0x80000) >> 20;
    if (a < 0 || c < 0)
        return 0;
    Game* g = g_game;
    if (a + footprint.x >= g->width)
        return 0;
    if (c + footprint.y >= g->height)
        return 0;
    int w = g->width;
    int x = footprint.x;
    int idx = ((c >> 1) + (footprint.x >> 2)) * (w >> 1)
            + (a >> 1) + (footprint.x >> 2);
    if (!(g->visibilityMask[idx] & (1 << unit->playerIndex)))
        return 1;
    unsigned char field228 = def->maxslope;
    short bd0 = def->maxwaterdepth;
    int sl = g->seaLevel;
    int lowBound = sl - bd0;
    int upperBound = sl - def->minwaterdepth;
    if (lowBound < sl && (def->flags1 & 0x800) && !(def->flags1 & 0x200000))
        lowBound = sl;
    Cell* cell = &g->cells[c * w + a];
    int rowStep = w - x;
    for (int i = 0; i < footprint.y; i++, cell += rowStep) {
        for (int j = 0; j < x; j++, cell++) {
            unsigned short f = cell->feature;
            unsigned int occ;
            if (f == 0xffff) {
                occ = 0;
            } else if (f < 0xfffb) {
                if (f < g->featureCount)
                    occ = (g->features[f].flags >> 6) & 1;
                else
                    occ = 1;
            } else {
                if (f != 0xfffe) {
                    occ = 1;
                } else {
                    unsigned short f2 = cell[-(cell->offsetY * w + cell->offsetX)].feature;
                    if (f2 >= 0xfffb)
                        occ = 0;
                    else
                        occ = (g->features[f2].flags >> 6) & 1;
                }
            }
            if (occ != 0)
                return 0;
            if (cell->flags & 2)
                return 0;
            if (cell->unit != 0 && cell->unit != unit->id)
                return 0;
            if (cell->unit2 != 0 && cell->unit2 != unit->id)
                return 0;
            int lo = cell->low;
            if (lo < lowBound)
                return 0;
            int hi = cell->high;
            if (hi > upperBound)
                return 0;
            if (hi - lo > field228)
                return 0;
        }
    }
    return 1;
}
