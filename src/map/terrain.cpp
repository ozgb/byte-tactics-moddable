#include "win_types.h"
#include "../state/block.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

struct Unit;

#include "../../interfaces/ref.h"
#pragma pack(push, 1)

#include "../util/vec3.h"
#include "../../interfaces/ref.h"

struct Point;
#include "../map/cell.h"

union Flags {
    struct {
        unsigned int unknown_0 : 26;
        unsigned int flag26 : 1;
        unsigned int flag27 : 1;
        unsigned int unknown_1 : 4;
    } bits;
    int all;
};

#include "../network/player.h"

struct UnitBucket {
    char unknown_0[6];
    Unit* first;
};

#include "../units/unit_def.h"

struct Feature {
    char unknown_0[0xec];
    float weightA;
    float weightB;
    char unknown_f4[0xfe - 0xf4];
    union {
        unsigned char flags;
        unsigned short flags16;
        struct {
            unsigned char unknown_6 : 6;
            unsigned char steep : 1;
        };
    };
};

#include "../units/unit.h"

struct BucketGrid {
    Ref<UnitBucket> cells;
    unsigned int width;
    unsigned int height;
};

#include "../game/game.h"
#include "../quirks/quirks.h"

#include "../units/movement_class.h"

union Fixed {
    int value;
    int v;
    struct { short lo; short hi; } p;
};

static inline int InsideExplored(Player* los, unsigned int tx, unsigned int ty)
{
    return tx < (unsigned int)los->exploredWidth && ty < (unsigned int)los->exploredHeight;
}

struct Node_0047cb00 {
    char unknown_0[0x8e];
    Ref<Node_0047cb00> next;
};

class SpatialBucket {
public:
    char unknown_0[6];
    union {
        Node_0047cb00* head;
        int field_6;
    };
    void UnlinkUnit(Node_0047cb00* node);
    void PrependUnit(int node);
};

class ClaimFootprintVisitor {
public:
    virtual void ClaimFootprint(Unit* obj);
};

class Visitor_0047e750 {
public:
    virtual void Visit(Unit* obj) = 0;
};

class DamagedAllyCollector {
public:
    virtual void CollectDamagedAlly(Unit* unit);
};

struct Entry_0047ea40 {
    Vec3 pos;
    float val;
};

#pragma pack(pop)

#include "../../link/globals.h"
extern ClaimFootprintVisitor g_claimFootprintVtable[];
#include "../../interfaces/seams.h"
#include "../state/sim_globals.h"

// FUNCTION: 0x47c790
void __stdcall ClaimFootprintCells(Unit* obj)
{
    unsigned int f = obj->flags;
    unsigned char b = (unsigned char)((f & 0x8000000) >> 27);
    if (!(b & 1))
        return;
    f &= ~0x8000000;
    obj->flags = f;
    Point16 size = obj->footprint;
    if (f & 0x20000000) {
        Cell* cell = g_seams.los_radar->GetMapCell(obj->cell.x, obj->cell.y);
        int index = 0;
        for (int j = size.y; j > 0; j--) {
            for (int i = size.x; i > 0; i--) {
                if (((unsigned char*)obj->def->yardmap.get())[index] & (obj->bit2 ? 2 : 4)) {
                    Unit* rec;
                    unsigned short id = cell->unit;
                    if (id == 0)
                        goto a_write;
                    rec = &g_game->units[id];
                    if (rec->player->active == 0)
                        goto a_b;
                    if (rec->player->type != 3)
                        goto a_b;
                    rec->flags |= 0x8000000;
                    obj->flags |= 0x4000000;
                a_write:
                    cell->unit = obj->id;
                    goto a_next;
                a_b:
                    rec->flags |= 0x4000000;
                    obj->flags |= 0x8000000;
                a_next: ;
                } else if (cell->unit == obj->id) {
                    cell->unit = 0;
                }
                index++;
                cell++;
            }
            cell += g_game->width - size.x;
        }
    } else if ((obj->flags & 3) == 1) {
        Cell* cell = g_seams.los_radar->GetMapCell(obj->cell.x, obj->cell.y);
        for (int j = size.y; j > 0; j--) {
            for (int i = size.x; i > 0; i--) {
                Unit* rec;
                unsigned short id = cell->unit;
                if (id == 0)
                    goto b_write;
                rec = &g_game->units[id];
                if (rec->player->active == 0)
                    goto b_b;
                if (rec->player->type != 3)
                    goto b_b;
                rec->flags |= 0x8000000;
                obj->flags |= 0x4000000;
            b_write:
                cell->unit = obj->id;
                goto b_next;
            b_b:
                rec->flags |= 0x4000000;
                obj->flags |= 0x8000000;
            b_next: ;
                cell++;
            }
            cell += g_game->width - size.x;
        }
    } else {
        Cell* cell = g_seams.los_radar->GetMapCell(obj->cell.x, obj->cell.y);
        for (int j = size.y; j > 0; j--) {
            for (int i = size.x; i > 0; i--) {
                Unit* rec;
                unsigned short id = cell->unit2;
                if (id == 0)
                    goto c_write;
                rec = &g_game->units[id];
                if (rec->player->active == 0)
                    goto c_b;
                if (rec->player->type != 3)
                    goto c_b;
                rec->flags |= 0x8000000;
                obj->flags |= 0x4000000;
            c_write:
                cell->unit2 = obj->id;
                goto c_next;
            c_b:
                rec->flags |= 0x4000000;
                obj->flags |= 0x8000000;
            c_next: ;
                cell++;
            }
            cell += g_game->width - size.x;
        }
    }
}

#include "terrain.h"

// FUNCTION: 0x47cac0
void __stdcall ClaimFootprintsInArea(Point16 pos, Point16 size)
{
    ClaimFootprintVisitor visitor;
    VisitObjectsInArea(pos, size, &visitor);
}

// FUNCTION: 0x47caf0
void FUN_0047caf0(void)
{
}

// FUNCTION: 0x47cb00
void SpatialBucket::UnlinkUnit(Node_0047cb00* node)
{
    if (head == node) {
        head = node->next;
        node->next = 0;
        return;
    }
    Ref<Node_0047cb00>* pp = &head->next;
    while (*pp != node)
        pp = &(*pp)->next;
    *pp = node->next;
    node->next = 0;
}

// FUNCTION: 0x47cb40
void SpatialBucket::PrependUnit(int node)
{
    int temp = field_6;
    ((Node_0047cb00*)node)->next = (Node_0047cb00*)temp;
    field_6 = node;
}

static void UnlinkBucketUnit(UnitBucket* bucket, Unit* unit)
{
    if (bucket->first == unit) {
        bucket->first = unit->cargoNext;
        return;
    }
    Ref<Unit>* pp = &bucket->first->cargoNext;
    while (*pp != unit)
        pp = &(*pp)->cargoNext;
    *pp = unit->cargoNext;
}

// FUNCTION: 0x47cb60
void __stdcall SetOwner(Unit* unit, UnitBucket* bucket)
{
    UnitBucket* cur = ((UnitBucket *)unit->spatialBucket.get());
    if (bucket != cur) {
        if (unit->carrier == 0) {
            if (cur != 0) {
                UnlinkBucketUnit(cur, unit);
                unit->cargoNext = 0;
            }
            unit->cargoNext = bucket->first;
            bucket->first = unit;
        }
        unit->spatialBucket = (Class_0047cb00*)(bucket);
    }
}

// FUNCTION: 0x47cbd0
void __stdcall ClearFootprintAndUnlink(Unit* unit)
{
    RemoveUnitFromMap(unit);
    if (unit->carrier == 0) {
        UnitBucket* cur = ((UnitBucket *)unit->spatialBucket.get());
        if (cur != 0) {
            UnlinkBucketUnit(cur, unit);
            unit->cargoNext = 0;
        }
    }
    unit->spatialBucket = 0;
}

// FUNCTION: 0x47cc30
void __stdcall AddUnitToMap(Unit* obj)
{
    Point16 size = obj->footprint;
    if (obj->motion.get() != nullptr)
        *(int*)((unsigned char*)obj->motion.get() + 0x26) = g_game->ticks;
    {
        short px = obj->cell.x;
        if (px < 0)
            goto remove;
        short py = obj->cell.y;
        if (py < 0 || px + size.x >= g_game->width || py + size.y >= g_game->height)
            goto remove;
    }
    {
        Vec3 p = obj->pos;
        SetOwner(obj,
            &((UnitBucket*)g_game->cells_1429f.get())[(p.x >> 23) + (p.z >> 23) * (int)g_game->width_142a3]);
    }
    {
        Cell* cell = &g_game->cells[g_game->width * obj->cell.y + obj->cell.x];
        unsigned int f = obj->flags;
        int index = 0;

        if (f & 0x20000000) {
            for (int y = size.y; y > 0; y--) {
                for (int x = size.x; x > 0; x--) {
                    unsigned char m = ((unsigned char*)obj->def->yardmap.get())[index++];
                    if (m & (obj->bit2 ? 2 : 4)) {
                        unsigned short id = cell->unit;
                        if (id != 0) {
                            Unit* rec = &g_game->units[id];
                            if (rec->player->active == 0) {
                                goto a_bad;
                            } else if (rec->player->type != 3) {
                                goto a_bad;
                            }
                            rec->flags |= 0x8000000;
                            obj->flags |= 0x4000000;
                            goto a_write;
                        a_bad:
                            rec->flags |= 0x4000000;
                            obj->flags |= 0x8000000;
                            goto a_next;
                        a_write: ;
                        }
                        cell->unit = obj->id;
                    }
                a_next:
                    if (m & 1)
                        cell->flags |= 2;
                    cell++;
                }
                cell += g_game->width - size.x;
            }
            Point16 grown;
            grown.x = size.x + 2;
            grown.y = size.y + 2;
            Point16 pad;
            pad.x = obj->cell.x - 1;
            pad.y = obj->cell.y - 1;
            g_seams.los_radar->UpdateCellHeightRange(pad, grown);
            RefreshAllPassMaps(obj->cell, obj->footprint);
            return;
        }
        if ((f & 3) == 1) {
            for (int y = size.y; y > 0; y--) {
                for (int x = size.x; x > 0; x--) {
                    unsigned short id = cell->unit;
                    if (id != 0) {
                        Unit* rec = &g_game->units[id];
                        if (rec->player->active == 0) {
                            goto b_bad;
                        } else if (rec->player->type != 3) {
                            goto b_bad;
                        }
                        rec->flags |= 0x8000000;
                        obj->flags |= 0x4000000;
                        goto b_write;
                    b_bad:
                        rec->flags |= 0x4000000;
                        obj->flags |= 0x8000000;
                        goto b_next;
                    b_write: ;
                    }
                    cell->unit = obj->id;
                b_next:
                    cell++;
                }
                cell += g_game->width - size.x;
            }
            return;
        }
        if ((f & 3) == 2) {
            for (int y = size.y; y > 0; y--) {
                for (int x = size.x; x > 0; x--) {
                    unsigned short id = cell->unit2;
                    if (id != 0) {
                        Unit* rec = &g_game->units[id];
                        if (rec->player->active == 0) {
                            goto c_bad;
                        } else if (rec->player->type != 3) {
                            goto c_bad;
                        }
                        rec->flags |= 0x8000000;
                        obj->flags |= 0x4000000;
                        goto c_write;
                    c_bad:
                        rec->flags |= 0x4000000;
                        obj->flags |= 0x8000000;
                        goto c_next;
                    c_write: ;
                    }
                    cell->unit2 = obj->id;
                c_next:
                    cell++;
                }
                cell += g_game->width - size.x;
            }
        }
    }
    return;

remove:
    SetOwner(obj, (UnitBucket*)g_game->overflowBucket.get());
}

void __stdcall RefreshPassMapsForUnit(Unit* obj);

// FUNCTION: 0x47d0e0
void __stdcall RemoveUnitFromMap(Unit* obj)
{
    if ((int)obj->spatialBucket.get() != (int)g_game->overflowBucket.get()) {
        Point16 size = obj->footprint;
        Cell* cell = &g_game->cells[obj->cell.y * g_game->width + obj->cell.x];
        int index = 0;
        if (obj->flags & 0x20000000) {
            for (int j = size.y; j > 0; j--) {
                for (int i = size.x; i > 0; i--) {
                    unsigned char m = ((unsigned char*)obj->def->yardmap.get())[index];
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
            pad.x = obj->cell.x - 1;
            pad.y = obj->cell.y - 1;
            g_seams.los_radar->UpdateCellHeightRange(pad, grown);
        } else if ((obj->flags & 3) == 1) {
            for (int j = size.y; j > 0; j--) {
                for (int i = size.x; i > 0; i--) {
                    if (cell->unit == obj->id) cell->unit = 0;
                    cell++;
                }
                cell += g_game->width - size.x;
            }
        } else if ((obj->flags & 3) == 2) {
            for (int j = size.y; j > 0; j--) {
                for (int i = size.x; i > 0; i--) {
                    if (cell->unit2 == obj->id) cell->unit2 = 0;
                    cell++;
                }
                cell += g_game->width - size.x;
            }
        }
    }
    obj->flags &= ~0x08000000;
    if (obj->flags & 0x4000000) {
        obj->flags &= ~0x04000000;
        ClaimFootprintVisitor visitor;
        VisitObjectsInArea(obj->cell, obj->footprint, &visitor);
    }
    RefreshPassMapsForUnit(obj);
}

struct Pos_0047d2e0 {
    Fixed x, y, z;
};

static inline int IsExploredAtHeight(Player* los, Vec3Words* pos,
    Fixed* hgt)
{
    int tx = pos->xWhole >> 5;
    int ty = (pos->zWhole - (hgt->p.hi >> 1)) >> 5;
    if (InsideExplored(los, tx, ty) && los->explored[(unsigned int)los->exploredWidth * ty + tx] != 0)
        return 1;
    return 0;
}

static inline int IsSeenAtHeight(Player* los, Vec3Words* pos,
    Fixed* hgt, unsigned int bit)
{
    int tx = pos->xWhole >> 5;
    int ty = (pos->zWhole - (hgt->p.hi >> 1)) >> 5;
    unsigned int w = los->exploredWidth;
    int r;
    if (!InsideExplored(los, tx, ty))
        r = 0;
    else
        r = (g_game->visibilityMask[w * ty + tx] & bit) != 0;
    return r;
}

static inline unsigned short VisibilityWord(Player* los, int tx,
    Vec3Words* pos, Fixed* hgt)
{
    int ty = (pos->zWhole - (hgt->p.hi >> 1)) >> 5;
    unsigned int w = los->exploredWidth;
    return g_game->visibilityMask[w * ty + tx];
}

static int Blocked_0047d2e0(Cell* c)
{
    unsigned short v = c->feature;
    if (v == 0xffff)
        return 0;
    if (v < 0xfffb) {
        if ((int)v >= g_game->featureCount)
            return 1;
        return (((unsigned char *)g_game->features)[v * 0x100 + 0xfe] >> 6) & 1;
    }
    if (v != 0xfffe)
        return 1;
    Cell* ref = c - (c->offsetY * g_game->width + c->offsetX);
    unsigned short v2 = ref->feature;
    if (v2 >= 0xfffb)
        return 0;
    return (((unsigned char *)g_game->features)[v2 * 0x100 + 0xfe] >> 6) & 1;
}

static unsigned char* Terrain_0047d2e0(
Cell* c)
{
    if (c == 0)
        return 0;
    unsigned short v = c->feature;
    if (v < 0xfffb) {
        if ((int)v >= g_game->featureCount)
            return 0;
        return (unsigned char *)g_game->features + v * 0x100;
    }
    if (v != 0xfffe)
        return 0;
    Cell* ref = c - (c->offsetY * g_game->width + c->offsetX);
    unsigned short v2 = ref->feature;
    if (v2 >= 0xfffb)
        return 0;
    return (unsigned char *)g_game->features + v2 * 0x100;
}

// FUNCTION: 0x47d2e0
int __stdcall CanBuildAt(UnitDef* unit, Point16 cell, short type, Player* los)
{
    int ok;
    g_lastPlaceHeight = 0;
    g_lastPlaceMetalSum = 0;
    Point16 origin = *(Point16*)&unit->footprintx;
    if (cell.x < 1 || cell.y < 1 || cell.x + origin.x >= g_game->width ||
        cell.y + origin.y >= g_game->height)
        return 0;
    int cols = origin.x;
    int x;
    int y;
    ok = 1;
    if (los != 0) {
        Pos_0047d2e0 pos;
        Fixed hgt;
        pos.x.v = (origin.x + cell.x * 2) << 19;
        pos.z.v = (origin.y + cell.y * 2) << 19;
        hgt.v = GetCellHeight(&cell) << 16;
        x = pos.x.p.hi >> 5;
        y = (pos.z.p.hi - (hgt.p.hi >> 1)) >> 5;
        if (!InsideExplored(los, x, y))
            return 0;
        unsigned int bit = 1 << g_game->playerIndex;
        if ((VisibilityWord(los, x, (Vec3Words*)&pos, &hgt) & bit) == 0)
            return 0;
        if ((g_game->mapFlags & 2) == 2)
            ok = IsExploredAtHeight(los, (Vec3Words*)&pos, &hgt);
        else
            ok = IsSeenAtHeight(los, (Vec3Words*)&pos, &hgt, bit);
    }
    Cell* c = &g_game->cells[cell.y * g_game->width + cell.x];
    unsigned char min6 = 0xff;
    unsigned char max5 = 0;
    unsigned char max5b = 0;
    int found80 = 0;
    int foundFE20 = 0;
    int index = 0;
    int row;
    int col;
    row = 0;
    if (origin.y > row) {
        do {
            for (col = 0; col < cols; col++) {
                g_lastPlaceMetalSum += c->metal;
                int m = ((unsigned char*)unit->yardmap.get())[index++];
                if (m & 8) {
                    if (c->low < min6)
                        min6 = c->low;
                    if (c->high > max5)
                        max5 = c->high;
                }
                if ((m & 0x10) && c->high > max5b)
                    max5b = c->high;
                if ((m & 1) && (c->flags & 2) && ok)
                    return 0;
                if ((m & 6) && (short)c->unit != 0 && (short)c->unit != type && ok)
                    return 0;
                if (m & 0x20) {
                    if (Blocked_0047d2e0(c) != 0)
                        return 0;
                }
                if (m & 0x40) {
                    unsigned char* e = Terrain_0047d2e0(c);
                    if (e != 0 && (e[0xff] & 2))
                        return 0;
                }
                if (m & 0x80) {
                    found80 = 1;
                    unsigned char* e = Terrain_0047d2e0(c);
                    if (e != 0 && (e[0xfe] & 0x20))
                        foundFE20 = 1;
                }
                ++c;
            }
            row++;
            c = (g_game->width - ((int)cols)) + c;
        } while (row < origin.y);
    }
    if (found80 && !foundFE20)
        return 0;
    unsigned char r;
    if (max5 < min6) {
        r = g_game->seaLevel - unit->draft;
    } else {
        if (max5 - min6 > unit->maxslope)
            return 0;
        r = min6;
    }
    if (max5b > r)
        return 0;
    if (min6 < g_game->seaLevel - unit->maxwaterdepth)
        return 0;
    if ((max5 > max5b ? max5 : max5b) > g_game->seaLevel - unit->minwaterdepth)
        return 0;
    g_lastPlaceHeight = r;
    return 1;
}

// FUNCTION: 0x47d820
int __stdcall GetFootprintHeight(UnitDef* unit, Point16 cell)
{
    short y = cell.y;
    Point16 fp = *(Point16*)&unit->footprintx;
    short x = cell.x;
    if (x < 1 || y < 1 || x + fp.x >= g_game->width || y + fp.y >= g_game->height)
        return 0;
    int width = g_game->width;
    Cell* c = &g_game->cells[y * width + x];
    unsigned char low = 0xff, high = 0, high2 = 0;
    int i = 0;
    for (int row = fp.y; row > 0; row--) {
        for (int col = fp.x; col > 0; col--) {
            int f = ((unsigned char*)unit->yardmap.get())[i++];
            if (f & 8) {
                if (c->low < low)
                    low = c->low;
                if (c->high > high)
                    high = c->high;
            }
            if (f & 0x10) {
                if (c->high > high2)
                    high2 = c->high;
            }
            c++;
        }
        c += width - fp.x;
    }
    unsigned char r;
    if (high < low)
        r = g_game->seaLevel - unit->draft;
    else
        r = low;
    return r;
}

// FUNCTION: 0x47d970
int __stdcall IsFootprintClear(Unit* obj, int flag)
{
    short* pp = &obj->cell.x;
    Point16* q = &obj->cell;
    Point16* r = &obj->footprint;
    short xend = pp[0] + obj->footprint.x;
    short yend = q->y + r->y;
    Point16 p = obj->cell;
    if (p.x < 1 || p.y < 1 || xend >= g_game->width || yend >= g_game->height)
        return 0;
    int width = g_game->width;
    unsigned char bit = flag ? 2 : 4;
    int n = 0;
    for (int y = p.y; y < yend; y++) {
        Cell* c = g_game->cells + y * width;
        for (int x = p.x; x < xend; x++) {
            if ((((unsigned char*)obj->def->yardmap.get())[n++] & bit) && c[x].unit != 0 && c[x].unit != obj->id)
                return 0;
        }
    }
    return 1;
}


// FUNCTION: 0x47dac0
void __stdcall SetYardOpen(Unit* obj, int flag)
{
    if (g_quirks.skipEmptyYardmap && obj->def->yardmap == 0)
        return;
    if (IsFootprintClear(obj, flag)) {
        obj->bit2 = flag;
        obj->flags |= 0x8000000;
        ClaimFootprintCells(obj);
        RefreshAllPassMaps(obj->cell, obj->footprint);
    }
}


// FUNCTION: 0x47db20
void __stdcall ForceNeighborFootprintReclaim(Unit* obj)
{
    obj->flags |= 0x8000000;
    ClaimFootprintVisitor visitor;
    Point16 size = obj->footprint;
    Point16 pos = obj->cell;
    VisitObjectsInArea(pos, size, &visitor);
    RefreshAllPassMaps(obj->cell, obj->footprint);
}

int __stdcall CanBuildAt(UnitDef* unit, Point16 cell, short type, Player* los);

static inline int SteepCell(Cell* c)
{
    unsigned short f = c->feature;
    if (f == 0xffff)
        return 0;
    if (f < 0xfffb) {
        if (f >= g_game->featureCount)
            return 1;
        return g_game->features[f].steep;
    }
    if (f != 0xfffe)
        return 1;
    c = c - (c->offsetY * g_game->width + c->offsetX);
    f = c->feature;
    if (f >= 0xfffb)
        return 0;
    return g_game->features[f].steep;
}

// FUNCTION: 0x47db70
int __stdcall CanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, Point16 cell, int flags)
{
    Point16 fp = *(Point16*)&unit->footprintx;
    if (cell.x < 0 || cell.y < 0)
        return flags == 2;
    int fx = fp.x;
    if (fx + cell.x >= g_game->width)
        return flags == 2;
    if (fp.y + cell.y >= g_game->height)
        return flags == 2;
    if (!unit->mobile)
        return CanBuildAt(unit, cell, 0, 0);
    Cell* c = &g_game->cells[cell.y * g_game->width + cell.x];
    int minHeight, maxHeight, stride;
    stride = g_game->width - fx;
    unsigned char tolerance = unit->maxslope;
    minHeight = g_game->seaLevel - unit->maxwaterdepth;
    maxHeight = g_game->seaLevel - unit->minwaterdepth;
    if (flags != 1)
        return 1;
    for (int row = 0; row < fp.y; row++, c += stride) {
        for (int col = 0; col < fx; col++, c++) {
            if (SteepCell(c))
                return 0;
            if (c->unit != 0 && c->unit != otherTypeId)
                return 0;
            if (c->low < minHeight)
                return 0;
            if (c->high > maxHeight)
                return 0;
            if (c->high - c->low > tolerance) {
                if (c->low >= g_game->seaLevel)
                    return 0;
                if (c->high - c->low > unit->maxwaterslope)
                    return 0;
            }
        }
    }
    return 1;
}

static inline Point16 WorldToCell(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}

static inline void CellToWorld(Point16 origin, Point16 c, Vec3* v)
{
    v->x = (origin.x + c.x * 2) << 19;
    v->z = (origin.y + c.y * 2) << 19;
}

// FUNCTION: 0x47ddc0
void __stdcall SnapWorldPosToFootprint(UnitDef* unit, Vec3* pos)
{
    if (unit->mobile)
        return;
    Point16 cell = WorldToCell(*pos, *(Point16*)&unit->footprintx);
    CellToWorld(*(Point16*)&unit->footprintx, cell, pos);
    pos->y = GetFootprintHeight(unit, cell) << 16;
}

// FUNCTION: 0x47de60
int __stdcall GetPassMapCellValue(MovementClass* obj, Cell* cell)
{
    int blocked;
    unsigned short feature = cell->feature;
    if (feature == 0xffff) {
        blocked = 0;
    } else if (feature < 0xfffb) {
        if (feature >= g_game->featureCount) {
            blocked = 1;
        } else {
            blocked = (g_game->features[feature].flags >> 6) & 1;
        }
    } else if (feature != 0xfffe) {
        blocked = 1;
    } else {
        int* width = &g_game->width;
        unsigned short f2 =
            (cell - (cell->offsetY * *width + cell->offsetX))->feature;
        if (0xfffb <= (cell - (cell->offsetY * *width + cell->offsetX))->feature) {
            blocked = 0;
        } else {
            blocked = (g_game->features[f2].flags >> 6) & 1;
        }
    }
    if (blocked)
        return 0;
    if (cell->unit != 0) {
        Ref<Unit>& unit = (*(Ref<Unit>*)&g_game->units[cell->unit].motion);
        if (!unit || ((unsigned int)unit->lastTick) < obj->lastTick)
            return 0;
    }
    int minHeight = g_game->seaLevel - obj->maxWaterDepth;
    if ((int)cell->low < minHeight)
        return 0;
    int maxHeight = g_game->seaLevel - obj->minWaterDepth;
    if ((int)cell->high > maxHeight)
        return 0;
    unsigned char diff = cell->high - cell->low;
    if (cell->low < g_game->seaLevel) {
        if (diff > obj->badWaterSlope)
            return !(obj->maxWaterSlope < diff);
    } else {
        if (diff > obj->badSlope)
            return !(obj->maxSlope < diff);
    }
    return 3;
}

// FUNCTION: 0x47dfc0
int __stdcall GetPassMapFootprintValue(MovementClass* obj, int x, int y, int w, int h)
{
    if (x < 0 || y < 0)
        return 0;
    if (x + w >= g_game->width)
        return 0;
    if (y + h >= g_game->height)
        return 0;
    int index = y * g_game->width + x;
    unsigned int result = 3;
    int row;
    Cell* cell = &g_game->cells[index];
    int rowStep = g_game->width - w;
    int minHeight = g_game->seaLevel - obj->maxWaterDepth;
    int maxHeight = g_game->seaLevel - obj->minWaterDepth;
    for (row = 0; row < h; row++, cell += rowStep) {
        for (int col = 0; col < w; col++, cell++) {
            unsigned short feature = cell->feature;
            int blocked;
            if (feature == 0xffff) {
                blocked = 0;
            } else if (feature < 0xfffb) {
                if (feature >= g_game->featureCount)
                    blocked = 1;
                else
                    blocked = (g_game->features[feature].flags >> 6) & 1;
            } else if (feature != 0xfffe) {
                blocked = 1;
            } else {
                Cell* other =
                    cell - (cell->offsetY * g_game->width + cell->offsetX);
                unsigned short f2 = other->feature;
                if (f2 >= 0xfffb)
                    blocked = 0;
                else
                    blocked = (g_game->features[f2].flags >> 6) & 1;
            }

            if (blocked)
                return 0;
            if (cell->unit != 0) {
                Ref<Unit>& unit = (*(Ref<Unit>*)&g_game->units[cell->unit].motion);
                if (!unit || ((unsigned int)unit->lastTick) < obj->lastTick)
                    return 0;
            }
            if ((int)cell->low < minHeight)
                return 0;
            if ((int)cell->high > maxHeight)
                return 0;
            unsigned char diff = cell->high - cell->low;
            if (cell->low < g_game->seaLevel) {
                if (diff > obj->badWaterSlope) {
                    if (diff > obj->maxWaterSlope)
                        return 0;
                    if (result > 1)
                        result = 1;
                }
            } else {
                if (diff > obj->badSlope) {
                    if (diff > obj->maxSlope)
                        return 0;
                    if (result > 1)
                        result = 1;
                }
            }
        }
    }
    return result;
}

// FUNCTION: 0x47e1f0
unsigned int __stdcall GetPassMapFootprintValueWithEdgeStrips(MovementClass* obj, int x, int y)
{
    int h = obj->footprintZ;
    int w = obj->footprintX;
    unsigned int r = GetPassMapFootprintValue(obj, x, y, w, h);
    if (r <= 1)
        return r;
    if (GetPassMapFootprintValue(obj, x - 1, y - 1, w + 1, 1) != 3)
        return 1;
    if (GetPassMapFootprintValue(obj, x + w, y - 1, 1, h + 1) != 3)
        return 1;
    if (GetPassMapFootprintValue(obj, x, y + h, w + 1, 1) != 3)
        return 1;
    return GetPassMapFootprintValue(obj, x - 1, y, 1, h + 1) == 3 ? 3 : 1;
}

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

// FUNCTION: 0x47e570
int __stdcall IsPadSlotFree(Unit* pad, int id)
{
    if (pad->carrier != 0) {
        return 0;
    }
    for (Unit* n = pad->cargo; n != 0; n = n->cargoNext) {
        if (((char)n->transportPiece) == id) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: 0x47e5c0
void __stdcall VisitObjectsInArea(Point16 pos, Point16 size, ClaimFootprintVisitor* visitor)
{
    int sumy;
    BucketGrid* grid = &(*(BucketGrid*)((char*)g_game + 0x1429f));
    int xstart = -1 + (pos.x >> 3);
    int ystart = -1 + (pos.y >> 3);
    int sumx = pos.x + size.x;
    sumy = pos.y + size.y;
    int xend = (sumx >> 3) + 1;
    int ye = ((pos.y + size.y) >> 3) + 1;
    for (int x = xstart; x <= xend; x++) {
        for (int y = ystart; y <= ye; y++) {
            if (x >= grid->width) {
                continue;
            }
            if (y >= grid->height) {
                continue;
            }
            for (Unit* o = grid->cells[grid->width * y + x].first; o != 0; o = o->cargoNext) {
                Point16 op = o->cell;
                Point16 os = o->footprint;
                if (pos.x < os.x + op.x && sumx > op.x && pos.y < os.y + op.y && sumy > op.y) {
                    visitor->ClaimFootprint(o);
                }
                for (Unit* c = o->cargo; c != 0; c = c->cargoNext) {
                    Point16 cs = c->footprint;
                    Point16 cp = c->cell;
                    if (pos.x < cs.x + cp.x && sumx > cp.x && pos.y < cp.y + cs.y && sumy > cp.y) {
                        visitor->ClaimFootprint(c);
                    }
                }
            }
        }
    }
}

static inline int Clamp_0047e750(int v, unsigned int size)
{
    if (v < size) {
        return v;
    }
    if (v < 0) {
        return 0;
    }
    return size - 1;
}

// FUNCTION: 0x47e750
void __stdcall VisitObjectsInRect(int x1, int y1, int x2, int y2, Visitor_0047e750* visitor)
{
    int cx1 = Clamp_0047e750(x1 >> 23, (*(BucketGrid*)((char*)g_game + 0x1429f)).width);
    int cy1 = Clamp_0047e750(y1 >> 23, (*(BucketGrid*)((char*)g_game + 0x1429f)).height);
    int cx2 = Clamp_0047e750(x2 >> 23, (*(BucketGrid*)((char*)g_game + 0x1429f)).width);
    int cy2 = Clamp_0047e750(y2 >> 23, (*(BucketGrid*)((char*)g_game + 0x1429f)).height);
    for (int y = cy1; y <= cy2; y++) {
        for (int x = cx1; x <= cx2; x++) {
            for (Unit* o = (*(BucketGrid*)((char*)g_game + 0x1429f)).cells[(*(BucketGrid*)((char*)g_game + 0x1429f)).width * y + x].first; o != 0; o = o->cargoNext) {
                if (o->pos.x >= x1 && o->pos.x <= x2 && o->pos.z >= y1 && o->pos.z <= y2) {
                    visitor->Visit(o);
                }
            }
        }
    }
}

static inline int Clamp_0047e890(int v, unsigned int size)
{
    if (v < size) {
        return v;
    }
    if (v < 0) {
        return 0;
    }
    return size - 1;
}

static inline int ClampX_0047e890(Vec3* p, int range, unsigned int size)
{
    return Clamp_0047e890((p->x - range) >> 23, size);
}

static inline int ClampZ_0047e890(Vec3* p, int range, unsigned int size)
{
    return Clamp_0047e890((p->z - range) >> 23, size);
}

static inline int ClampX2_0047e890(Vec3* p, int range, unsigned int size)
{
    return Clamp_0047e890((p->x + range) >> 23, size);
}

static inline int ClampZ2_0047e890(Vec3* p, int range, unsigned int size)
{
    return Clamp_0047e890((p->z + range) >> 23, size);
}

static inline int Dist2_0047e890(Vec3* b, Vec3* a)
{
    int dz = a->z - b->z;
    int dx = a->x - b->x;
    return (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
}

// FUNCTION: 0x47e890
void __stdcall VisitObjectsInRange(Vec3* pos, int range, DamagedAllyCollector& visitor)
{
    int cx1 = ClampX_0047e890(pos, range, (*(BucketGrid*)((char*)g_game + 0x1429f)).width);
    int cy1 = ClampZ_0047e890(pos, range, (*(BucketGrid*)((char*)g_game + 0x1429f)).height);
    int cx2 = ClampX2_0047e890(pos, range, (*(BucketGrid*)((char*)g_game + 0x1429f)).width);
    int cy2 = ClampZ2_0047e890(pos, range, (*(BucketGrid*)((char*)g_game + 0x1429f)).height);
    int range2 = (int)(((__int64)range * range) >> 32);
    for (int y = cy1; y <= cy2; y++) {
        for (int x = cx1; x <= cx2; x++) {
            for (Unit* o = (*(BucketGrid*)((char*)g_game + 0x1429f)).cells[(*(BucketGrid*)((char*)g_game + 0x1429f)).width * y + x].first; o != 0; o = o->cargoNext) {
                if (Dist2_0047e890(pos, &o->pos) <= range2) {
                    visitor.CollectDamagedAlly(o);
                }
            }
        }
    }
}

#include "../../interfaces/seams.h"

void __cdecl operator delete(void* p);

#include "../util/math.h"

// FUNCTION: 0x47ea40
int __stdcall PickRandomReclaimableResourcesInRadius(Vec3* center, Fixed radius, Vec3** out1, float* val1, Vec3** out2, float* val2)
{
    int countB = 0;
    int countA = 0;
    int max = radius.p.hi * radius.p.hi / 256;
    Entry_0047ea40* a = (Entry_0047ea40*)SimAlloc(max * sizeof(Entry_0047ea40));
    Entry_0047ea40* b = (Entry_0047ea40*)SimAlloc(max * sizeof(Entry_0047ea40));
    int half = radius.value / 2;
    Vec3 pos;
    for (pos.z = center->z - half; pos.z <= center->z + half; pos.z += 0x300000) {
        for (pos.x = center->x - half; pos.x <= center->x + half; pos.x += 0x300000) {
            void* cell = g_seams.los_radar->GetMapCellAtPosition(&pos);
            if (!cell)
                continue;
            unsigned short index = g_seams.features->GetCellFeature((Cell*)cell);
            if (index >= 0xfffb)
                continue;
            unsigned short fl = g_game->features[index].flags16;
            if (!(fl & 0x80))
                continue;
            if (!(fl & 0x100))
                continue;
            if (g_game->features[index].weightA != 0.0f) {
                b[countB].pos = pos;
                b[countB].val = g_game->features[index].weightA;
                countB++;
            }
            if (g_game->features[index].weightB != 0.0f) {
                a[countA].pos = pos;
                a[countA].val = g_game->features[index].weightB;
                countA++;
            }
        }
    }
    int found = 0;
    if (countB != 0) {
        int best = 0;
        float bestVal = 0.0f;
        for (int k = 0; k < 3; k++) {
            int i = RandomInt(countB);
            if (bestVal < b[i].val) {
                bestVal = b[i].val;
                best = i;
            }
        }
        **out1 = b[best].pos;
        *val1 = b[best].val;
        found = 1;
    } else {
        *out1 = 0;
        *val1 = 0;
    }
    if (countA != 0) {
        float bestVal = 0.0f;
        int best = 0;
        for (int k = 0; k < 3; k++) {
            int i = RandomInt(countA);
            if (bestVal < a[i].val) {
                bestVal = a[i].val;
                best = i;
            }
        }
        **out2 = a[best].pos;
        *val2 = a[best].val;
        found = 1;
    } else {
        *out2 = 0;
        *val2 = 0;
    }
    operator delete(b);
    operator delete(a);
    return found;
}

// FUNCTION: 0x47ed30
void ClaimFootprintVisitor::ClaimFootprint(Unit* obj)
{
    ClaimFootprintCells(obj);
}

static inline Cell* CellAt(int x, int y)
{
    if (x >= 0 && x < g_game->width && y >= 0 && y < g_game->height)
        return &g_game->cells[y * g_game->width + x];
    return nullptr;
}

static inline int WholePart(int fixed)
{
    return (short)(fixed >> 16);
}

static inline void SetWholePart(int* fixed, int whole)
{
    ((short*)fixed)[1] = (short)whole;
}

// FUNCTION: 0x485010
int __stdcall GetCellHeight(Point16* p)
{
    Cell* cell = CellAt(p->x, p->y);
    if (cell)
        return cell->height;
    return 0;
}

static inline Cell* GetCellCachedWidth(int x, int y)
{
    if (x >= 0) {
        int w = g_game->width;
        if (x < w && y >= 0 && y < g_game->height)
            return &g_game->cells[w * y + x];
    }
    return nullptr;
}

// FUNCTION: 0x485140
int __stdcall GetCellMeanHeight(Vec3* p)
{
    int x = WholePart(p->x) / 16, y = WholePart(p->z) / 16;
    Cell* cell = GetCellCachedWidth(x, y);
    if (cell)
        return (cell->low + cell->high) >> 1;
    return -1;
}

// FUNCTION: 0x4851c0
int __stdcall FindHighestPointOnLine(Vec3 a, Vec3 b)
{
    int dz = b.z - a.z;
    b.z = dz;
    b.y -= a.y;
    int dx = b.x - a.x;
    b.x = dx;
    Vec3 d = b;
    int n;
    if (abs(dx) < abs(dz))
        n = abs(dz) / 0x100000;
    else
        n = abs(dx) / 0x100000;
    n++;
    d.x /= n;
    d.z /= n;
    short best = 0;
    for (int i = 0; i <= n; i++) {
        Cell* c = CellAt(a.x / 0x100000, a.z / 0x100000);
        if (c) {
            short v = ((unsigned char*)g_game->features)[c->feature * 256 + 0xfa] + c->height;
            if (best < v) best = v;
            if (c->unit) {
                Unit* o = &g_game->units[c->unit];
                short w = (o->def.get()->modelMaxY + o->pos.y) >> 16;
                if (best < w) best = w;
            }
        }
        a.x += d.x; a.y += d.y; a.z += d.z;
    }
    return best;
}

// FUNCTION: 0x485330
void __stdcall GetCellPosition(int x, int y, Vec3* out)
{
    Cell* cell = CellAt(x, y);
    if (cell) {
        memset(out, 0, sizeof(*out));
        SetWholePart(&out->x, x << 4);
        SetWholePart(&out->y, cell->height);
        SetWholePart(&out->z, y << 4);
    }
}
