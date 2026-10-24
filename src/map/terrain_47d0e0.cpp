#include "win_types.h"
#include "../../interfaces/ref.h"

#pragma pack(push, 1)

#include "../util/vec3.h"

struct Cell {
    short unit;
    short unit2;
    char unknown_4[0xc - 0x4];
    unsigned char flags;
};

struct Unit_0047db20 {
    char unknown_0[0x14e];
    Ref<unsigned char> mask;
};

union Flags_0047db20 {
    struct {
        unsigned int unknown_0 : 26;
        unsigned int flag26 : 1;
        unsigned int unknown_1 : 5;
    } bits;
    int all;
};

struct Obj_0047db20 {
    char unknown_0[0x76];
    Point16 pos;
    char unknown_7a[4];
    Point16 footprint;
    Ref<char> spatialBucket;
    char unknown_86[0x92 - 0x86];
    Ref<Unit_0047db20> def;
    char unknown_96[0xa8 - 0x96];
    short id;
    char unknown_aa[0x110 - 0xaa];
    Flags_0047db20 flags;
};

#include "../game/game.h"

class ClaimFootprintVisitor {
public:
    virtual void ClaimFootprint();
};
#pragma pack(pop)

#include "../../link/globals.h"
extern ClaimFootprintVisitor g_claimFootprintVtable[];

#include "../../interfaces/seams.h"
#include "terrain.h"
void __stdcall RefreshPassMapsForUnit(Obj_0047db20* obj);

static inline Cell* CellAt(const Point16& p)
{
    return &g_game->cells[p.y * g_game->width + p.x];
}

// FUNCTION: 0x47d0e0
void __stdcall RemoveUnitFromMap(Obj_0047db20* obj)
{
    if ((int)obj->spatialBucket.get() != (int)g_game->overflowBucket.get()) {
        Point16 size = obj->footprint;
        Cell* cell = CellAt(obj->pos);
        int index = 0;
        if (obj->flags.all & 0x20000000) {
            for (int j = size.y; j > 0; j--) {
                for (int i = size.x; i > 0; i--) {
                    unsigned char m = obj->def->mask[index];
                    index++;
                    if (cell->unit == obj->id) cell->unit = 0;
                    if (m & 1) cell->flags &= 0xfd;
                    cell++;
                }
                cell += g_game->width - size.x;
            }
            Point16 grown;
            grown.x = size.x + 2;
            grown.y = size.y + 2;
            Point16 pad;
            pad.x = obj->pos.x - 1;
            pad.y = obj->pos.y - 1;
            g_seams.los_radar->UpdateCellHeightRange(pad, grown);
        } else if ((obj->flags.all & 3) == 1) {
            for (int j = size.y; j > 0; j--) {
                for (int i = size.x; i > 0; i--) {
                    if (cell->unit == obj->id) cell->unit = 0;
                    cell++;
                }
                cell += g_game->width - size.x;
            }
        } else if ((obj->flags.all & 3) == 2) {
            for (int j = size.y; j > 0; j--) {
                for (int i = size.x; i > 0; i--) {
                    if (cell->unit2 == obj->id) cell->unit2 = 0;
                    cell++;
                }
                cell += g_game->width - size.x;
            }
        }
    }
    obj->flags.all &= ~0x08000000;
    if (obj->flags.bits.flag26) {
        obj->flags.all &= ~0x04000000;
        ClaimFootprintVisitor visitor;
        VisitObjectsInArea(obj->pos, obj->footprint, &visitor);
    }
    RefreshPassMapsForUnit(obj);
}
