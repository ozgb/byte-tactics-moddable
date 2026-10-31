#include <memory>
#include "../state/block.h"
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <algorithm>
#include "win_types.h"
#include <stdio.h>
#include <stdlib.h>
#include "los_tables.h"
#include "spatial_bucket.h"

#pragma pack(push, 1)

#include "sight_query.h"

#include "../graphics/rect.h"

#include "../map/cell.h"
#include "../map/tnt_info.h"

#include "grid2.h"

struct FogGrid {
    Ref<FogCell> cells;
    unsigned int width;
    unsigned int height;
    int count;
};

struct MapFogGrid {
    FogCell* cells;
    unsigned int width;
    unsigned int height;
    int count;
};

struct PlayerGridView {
    unsigned char* cells;
    unsigned int width;
    unsigned int height;
    int size;
    unsigned char& at(int x, int y) { return cells[y * width + x]; }
};

struct Sound;
struct Feature;
struct Camera;
struct Palette;
void RegisterUnitOrders();
#include "../network/player.h"

#include "../units/unit_def.h"

#include "../units/unit.h"

#include "../graphics/gaf_frame.h"

#include "frame_table.h"

#include "eye.h"

union Flags_142f1 {
    unsigned short raw;
    unsigned char rawByte;
    struct {
        unsigned short bit0 : 1;
        unsigned short bit1 : 1;
        unsigned short mapChanged : 1;
        unsigned short rest : 13;
    };
};

struct IconSet {
    char unknown_0[4];
    unsigned char* data;
};

#include "line_of_sight.h"

union Slot {
    int n;
    Point16 p;
};

class Mission;

struct GraphicsOptionFlagsView {
    unsigned short damagebars : 1;
    unsigned short antiAlias : 1;
    unsigned short shadows : 1;
    unsigned short vehicleShadows : 1;
    unsigned short featureShadows : 1;
    unsigned short shading : 1;
    unsigned short ditheredFog : 1;
    unsigned short unused7 : 1;
    unsigned short switchAlt : 1;
};

#include "../game/game.h"

#pragma pack(pop)

#include "../../link/globals.h"

#include "los_lines.h"

int RIReport(int, int, int, int, int, int, int, int, int, int);
int DrawWrappedText(char*, char*, int, int, int, int, int);
int ScanDirectory(char*, char*, char*, int, int, int);

#include "../map/mission.h"
#include "../state/sim_globals.h"

#include "../debug/debug_lib.h"
#include "../graphics/object_pool.h"
#include "../graphics/display.h"
#include "../game/data_files.h"
GafFrame* __stdcall GetGafFrame(void* table, int index);
#include "radar.h"
#include "../graphics/gaf.h"
void __stdcall DrawFrame(void* surface, void* header, int x, int y);
void __stdcall DrawTile(void* dst, int x, int y, unsigned char* pix);
#include "../graphics/draw.h"
void* __cdecl operator new(unsigned int size);
void __cdecl operator delete(void* p);

struct Expired {
    int operator()(const Eye& e) const { return e.expires < (unsigned int)g_game->ticks; }
};

static inline int SumX(Point16 a, Point16 b) { return a.x + b.x; }
static inline int SumY(Point16 a, Point16 b) { return a.y + b.y; }

static inline Point16 MakePoint(int x, int y)
{
    Point16 p;
    p.x = x;
    p.y = y;
    return p;
}

static inline Cell* GetCellPixel(int x, int y, int py)
{
    if (x >= 0 && x < g_game->width && py >= 0 && y < g_game->height)
        return &g_game->cells[y * g_game->width + x];
    return 0;
}

static inline void ClearFeature(Cell* c)
{
    unsigned short f = c->feature;
    if (f == 0xffff || f == 0xfffe)
        c->feature = 0xfffd;
}

inline int SightTilesRaw(SightQuery* params)
{
    return params->sightDistance / 32;
}

inline int SightTilesClamped(SightQuery* params)
{
    int v = SightTilesRaw(params);
    return v < 0 ? 0 : v;
}

// FUNCTION: 0x4814f0
void FreeLosTables()
{
    g_losTables.FreeTables();
}

// FUNCTION: 0x481550
Cell* __stdcall GetMapCell(int x, int y)
{
    if (x >= 0 && x < g_game->width && y >= 0 && y < g_game->height)
        return &g_game->cells[y * g_game->width + x];
    return 0;
}

// FUNCTION: 0x4815a0
Cell* __stdcall GetMapCellAtPosition(Vec3* pos)
{
    int x = pos->x >> 20;
    int y = pos->z >> 20;
    if (x >= 0 && x < g_game->width && y >= 0 && y < g_game->height)
        return &g_game->cells[y * g_game->width + x];
    return 0;
}

// FUNCTION: 0x4815f0
Cell* __stdcall GetOriginCellAtPosition(Vec3* pos)
{
    int x = pos->x >> 20;
    int z = pos->z >> 20;
    if (x < 0 || x >= g_game->width || z < 0 || z >= g_game->height)
        return 0;
    Cell* cell = &g_game->cells[z * g_game->width + x];
    if (cell->feature == 0xfffe)
    {
        x -= cell->offsetX;
        z -= cell->offsetY;
        if (x < 0 || x >= g_game->width || z < 0 || z >= g_game->height)
            cell = 0;
        else
            cell = &g_game->cells[z * g_game->width + x];
    }
    return cell;
}

// FUNCTION: 0x482090
void __stdcall RemoveUnitLineOfSight(Unit* unit)
{
    SightQuery p;
    p.player = unit->player;
    p.cacheCell = &unit->losCacheCellX;
    p.sightDistance = unit->def->range;
    p.frameIdx = &unit->losSightFrameIdx;
    p.pos = unit->pos;
    p.eyeHeight = (unsigned char)unit->def->modelMaxYHigh;
    int minY = (g_game->seaLevel + 1) << 16;
    if (p.pos.y < minY) {
        p.pos.y = minY;
    }
    RemoveLineOfSight(&p);
}

// FUNCTION: 0x482110
int __stdcall IsEyeballExpired(Eye* obj)
{
    unsigned int field_val = obj->expires;
    unsigned int cmp_val = (unsigned int)g_game->ticks;
    return field_val < cmp_val ? 1 : 0;
}

// FUNCTION: 0x482130
void ExpireEyeballs()
{
    int changed;
    Eye* p = (Eye *)g_game->eyes;
    for (int i = 0; i < g_game->count_14277; i++, p++) {
        if (p->expires < (unsigned int)g_game->ticks) {
            RemoveLineOfSight((SightQuery*)p);
            changed = 1;
        }
    }
    if (!changed)
        return;
    Eye* end = (Eye *)g_game->eyes + g_game->count_14277;
    Eye* n = std::remove_if((Eye *)g_game->eyes, end, Expired());
    g_game->count_14277 = n - (Eye *)g_game->eyes;
}

#include <io.h>
#include <float.h>

// FUNCTION: 0x4825b0
void __stdcall UpdateLineOfSight(SightQuery* params)
{
    if ((g_game->mapFlags_14281.raw & 4) == 4) {
        int x = ((short*)&params->pos.x)[1] >> 5;
        int v = params->eyeHeight + ((short*)&params->pos.y)[1];
        if (v < 0) {
            v = 0;
        }
        if (v > 0xff) {
            v = 0xff;
        }
        int y = (((short*)&params->pos.z)[1] - (v >> 1)) >> 5;
        int diff = abs((int)*params->frameIdx - v);
        unsigned char c = *params->frameIdx;
        if (params->cacheCell[0] != x || params->cacheCell[1] != y || diff > 5) {
            if (c != 0 && (g_game->mapFlags_14281.raw & 2)) {
                RemoveLineOfSight(params);
            }
            params->cacheCell[0] = (short)x;
            params->cacheCell[1] = (short)y;
            if ((unsigned)x >= g_game->grid1.width || (unsigned)y >= g_game->grid1.height) {
                *params->frameIdx = 0;
                return;
            }
            *params->frameIdx = (unsigned char)v;
            if ((unsigned char)(g_game->mapFlags_14281.raw >> 1) & 1) {
                AddLineOfSight(params);
            }
            if (g_game->mapFlags_14281.raw & 1) {
                RevealAroundUnit(params);
            }
        }
    }
    else {
        int i = (short)params->sightDistance / 32 - 5;
        if (i < 0) {
            i = 0;
        } else if (i >= g_game->losTable->count) {
            i = g_game->losTable->count - 1;
        }
        int cx = params->pos.x / 0x200000;
        int cy = params->pos.z / 0x200000 - ((short*)&params->pos.y)[1] / 64;
        GafFrame* e = GetGafFrame(g_game->losTable, i);
        cx -= e->xOffset;
        cy -= e->yOffset;
        if (params->cacheCell[0] != cx || params->cacheCell[1] != cy || *params->frameIdx != i) {
            if ((g_game->mapFlags_14281.raw & 2) == 2) {
                RemoveLineOfSight(params);
                params->cacheCell[0] = (short)cx;
                params->cacheCell[1] = (short)cy;
                *params->frameIdx = (unsigned char)i;
                AddLineOfSight(params);
            } else {
                params->cacheCell[0] = (short)cx;
                params->cacheCell[1] = (short)cy;
                *params->frameIdx = (unsigned char)i;
            }
            if (g_game->mapFlags_14281.raw & 1) {
                RevealAroundUnit(params);
            }
        }
    }
}

// FUNCTION: 0x4827b0
void __stdcall UpdateUnitLineOfSight(Unit* unit)
{
    SightQuery p;
    p.player = unit->player;
    p.cacheCell = &unit->losCacheCellX;
    p.sightDistance = unit->def->range;
    p.frameIdx = &unit->losSightFrameIdx;
    p.pos = unit->pos;
    p.eyeHeight = (unsigned char)unit->def->modelMaxYHigh;
    int minY = (g_game->seaLevel + 1) << 16;
    if (p.pos.y < minY) {
        p.pos.y = minY;
    }
    UpdateLineOfSight(&p);
}

// FUNCTION: 0x482830
void __stdcall InitUnitSightCircleReveal(SightQuery* params)
{
    if ((g_game->mapFlags_14281.rawByte & 2) != 2) {
        return;
    }
    *params->frameIdx = 0;
    if ((g_game->mapFlags_14281.rawByte & 4) == 4) {
        UpdateLineOfSight(params);
        return;
    }
    int lod = params->sightDistance / 32 - 5;
    if (lod < 0) {
        lod = 0;
    } else {
        if (lod >= g_game->losTable->count) {
            lod = g_game->losTable->count - 1;
        }
    }
    int x = params->pos.x / 0x200000;
    int y = params->pos.z / 0x200000 - ((short*)&params->pos.y)[1] / 64;
    GafFrame* entry = GetGafFrame(g_game->losTable, lod);
    x -= entry->xOffset;
    y -= entry->yOffset;
    params->cacheCell[0] = (short)x;
    params->cacheCell[1] = (short)y;
    *params->frameIdx = (char)lod;
    AddLineOfSight(params);
    RevealAroundUnit(params);
}

// FUNCTION: 0x482c20
void BuildDerivedLayers(void)
{
    Rec* rec = new (kSim) Rec;
    g_game->overflowBucket = (SpatialBucket*)rec;
    ((Rec *)g_game->overflowBucket.get())->flags = 0x1f;

    Grid2* grid2 = &g_game->spatialGrid_1429f;
    int b = g_game->baseY * 0x10000;
    int a = g_game->baseX * 0x10000;
    grid2->heightFixed = b;
    grid2->widthFixed = a;
    int w2;
    int h2;
    h2 = (b + 0x7fffff) >> 0x17;
    w2 = (a + 0x7fffff) >> 0x17;
    grid2->width = w2;
    grid2->height = h2;
    operator delete(grid2->cells);
    int count2 = (h2 * w2 + 7) & 0xfffffff8;
    grid2->count = count2;

    grid2->cells = count2 != 0 ? (FogCell*)new (kSim) Rec[count2] : 0;
    int outer;
    unsigned char* cells2 = (unsigned char*)grid2->cells.get();
    unsigned char* cellp = (unsigned char*)g_game->cells.get();
    int inner;
    int accum;

    for (unsigned int lb1 = 0; lb1 < (unsigned int)grid2->width; lb1++)
        ((Rec*)grid2->cells.get())[lb1].flags |= 1;
    for (unsigned int lb2 = 0; lb2 < (unsigned int)grid2->width; lb2++)
        ((Rec*)grid2->cells.get())[(grid2->height - 1) * grid2->width + lb2].flags |= 2;
    for (unsigned int lb3 = 0; lb3 < (unsigned int)grid2->height; lb3++)
        ((Rec*)grid2->cells.get())[lb3 * grid2->width].flags |= 4;
    for (unsigned int i = 0; i < (unsigned int)grid2->height; i++) {
        int* fp = &((Rec*)grid2->cells.get())[(i + 1) * grid2->width - 1].flags;
        *fp = *fp | 8;
    }

    for (int d = 0; d < grid2->count; d++)
        ((unsigned char*)grid2->cells.get())[d * 10] = g_game->seaLevel;

    {
        for (int y = 0; y < g_game->height; y++) {
            unsigned char* recp = cells2;
            for (int x = 0; x < g_game->width; x++, cellp += 0xd) {
                if (cellp[5] > recp[0])
                    recp[0] = cellp[5];
                if ((x & 7) == 7)
                    recp += 10;
            }
            if ((y & 7) == 7)
                cells2 += grid2->width * 10;
        }
    }

    for (unsigned int r = 0; r < (unsigned int)grid2->height; r++) {
        unsigned char* p = (unsigned char*)grid2->cells.get() + r * grid2->width * 10;
        unsigned char prev = 0;
        for (unsigned int fr = 1; fr < (unsigned int)grid2->width; fr++, p += 10) {
            unsigned char t = p[10];
            unsigned char last = prev;
            prev = p[0];
            if (prev <= t)
                prev = t;
            p[1] = last > prev ? last : prev;
        }
        p[1] = prev;
    }

    for (outer = 0; (unsigned int)outer < (unsigned int)grid2->width; outer++) {
        unsigned char* p = (unsigned char*)grid2->cells.get() + outer * 10;
        unsigned char prev = 0;
        for (unsigned int gc = 1; gc < (unsigned int)grid2->height; gc++, p += grid2->width * 10) {
            unsigned char t = p[grid2->width * 10 + 1];
            unsigned char last = prev;
            prev = p[1];
            if (prev <= t)
                prev = t;
            p[1] = last > prev ? last : prev;

        }
        p[1] = prev;
    }

    FogGrid* grid1 = (FogGrid*)&g_game->grid1;
    int w1, h1;
    h1 = g_game->height / 2;
    w1 = g_game->width / 2;

    grid1->width = w1;
    grid1->height = h1;
    operator delete(grid1->cells);
    int count1 = (h1 * w1 + 7) & 0xfffffff8;
    grid1->count = count1;
    grid1->cells = count1 != 0 ? (FogCell*)SimAlloc(count1 * 2) : 0;
    for (int hf = 0; hf < grid1->count; hf++) {
        ((unsigned char*)grid1->cells.get())[hf * 2] = 0;
        ((unsigned char*)grid1->cells.get())[hf * 2 + 1] = 0xff;
    }

    for (outer = 0; outer < g_game->width; outer++) {
        int t20 = (outer - 1) >> 1;
        int t24 = outer >> 1;
        unsigned char* p1 = 0;
        unsigned char* p2 = 0;
        inner = 0;
        if (inner < g_game->height) {
            accum = 0;
            do {
                int idx = g_game->width * inner + outer;
                int cellval = ((unsigned char*)g_game->cells.get())[idx * 0xd + 4];
                int v = accum - (cellval >> 1);
                int block = v >> 5;
                if (block > -1) {
                    int q = ((block * 32 + 31) * cellval) / (v + 31);
                    if (p1) {
                        p1[0] = max(p1[0], q);
                        p1[1] = min(p1[1], q);
                    }
                    if (p2) {
                        p2[0] = max(p2[0], q);
                        p2[1] = min(p2[1], q);
                    }
                    if ((unsigned int)t20 < (unsigned int)grid1->width
                            && (unsigned int)block < (unsigned int)grid1->height) {
                        p1 = (unsigned char*)grid1->cells.get() + (block * grid1->width + t20) * 2;
                        p1[0] = max(p1[0], q);
                        p1[1] = min(p1[1], q);
                    } else {
                        p1 = 0;
                    }
                    if (t20 != t24
                            && (unsigned int)t24 < (unsigned int)grid1->width
                            && (unsigned int)block < (unsigned int)grid1->height) {
                        p2 = (unsigned char*)grid1->cells.get() + (block * grid1->width + t24) * 2;
                        p2[0] = max(p2[0], q);
                        p2[1] = min(p2[1], q);
                    } else {
                        p2 = 0;
                    }
                }
                if (p1) {
                    p1[0] = max(p1[0], cellval);
                    p1[1] = min(p1[1], cellval);
                }
                if (p2) {
                    p2[0] = max(p2[0], cellval);
                    p2[1] = min(p2[1], cellval);
                }
                inner++;
                accum += 0x10;
            } while (inner < g_game->height);
        }
    }

    int def = g_game->seaLevel;
    for (int jf = 0; jf < grid1->count; jf++) {
        unsigned char* p = (unsigned char*)grid1->cells.get() + jf * 2;
        int aa = p[0];
        int bb = p[1];
        int q1 = (aa * 2 + bb) / 3;
        int q2 = (aa + bb * 2) / 3;
        if (q1 <= def)
            q1 = def;
        p[0] = q1;
        if (q2 <= def)
            q2 = def;
        p[1] = q2;
    }
}

// FUNCTION: 0x483210
void __stdcall UpdateCellHeightRange(Point16 pos, Point16 size)
{
    int width = g_game->width;
    int height = g_game->height;
    int xmax = SumX(pos, size);
    int ymax = SumY(pos, size);
    if (pos.x < 0)
        pos.x = 0;
    if (pos.y < 0)
        pos.y = 0;
    if (xmax >= width)
        xmax = width - 1;
    if (ymax >= height)
        ymax = height - 1;
    if (pos.x >= xmax)
        return;
    if (pos.y >= ymax)
        return;
    int col, row;
    for (row = pos.y; row < ymax; row++) {
        Cell* c = &g_game->cells[row * width + pos.x];
        for (col = pos.x; col < xmax; col++, c++) {
            unsigned char lo = c->height;
            unsigned char hi = c->height;
            if (col < width - 1) {
                unsigned char v = c[1].height;
                if (v < lo)
                    lo = v;
                if (v > hi)
                    hi = v;
            }
            if (row < height - 1) {
                unsigned char v = c[width].height;
                if (v < lo)
                    lo = v;
                if (v > hi)
                    hi = v;
                if (col < width - 1) {
                    unsigned char v2 = c[width + 1].height;
                    if (v2 < lo)
                        lo = v2;
                    if (v2 > hi)
                        hi = v2;
                }
            }
            c->low = lo;
            c->high = hi;
        }
    }
}

// FUNCTION: 0x483370
void UpdateAllCellHeightRanges()
{
    UpdateCellHeightRange(MakePoint(0, 0), MakePoint(g_game->width, g_game->height));
}

// FUNCTION: 0x4833b0
void ClearBorderFeatures()
{
    g_game->mapWidth = g_game->baseX - 0x20;
    g_game->mapHeight = g_game->baseY - 0x80;

    int col = g_game->width - 2;
    for (int row = 0; row < g_game->height; row++) {
        Cell* c = GetMapCell(col, row);
        ClearFeature(c);
        ClearFeature(c + 1);
    }

    for (int x = 0; x < g_game->width; x++) {
        int y = 0;
        int y16 = 0;
        for (;; y++, y16 += 16) {
            Cell* c = GetCellPixel(x, y, y16);
            if (y16 - (c->height >> 1) >= 0)
                break;
            ClearFeature(c);
        }
    }

    for (int x2 = 0; x2 < g_game->width; x2++) {
        int y2 = g_game->height - 1;
        int y16b = y2 * 16;
        for (;; y2--, y16b -= 16) {
            Cell* c = GetMapCell(x2, y2);
            if (y16b - (c->height >> 1) <= g_game->mapHeight)
                break;
            Cell* prev = c - g_game->width;
            ClearFeature(prev);
        }
    }

    if (g_game->net->lavaWorld != 0) {
        Cell* c = g_game->cells;
        Cell* end = c + g_game->width * g_game->height;
        while (c < end) {
            if (c->low <= g_game->seaLevel)
                ClearFeature(c);
            c++;
        }
    }
}

struct MapSettings {
    int* sortUnits;
    int* sortIndices;
    int* sortLineCount;
    char unknown_c[0x24 - 0xc];
    MapFogGrid* grid;
    int baseX;
    int baseY;
    char unknown_30[0x38 - 0x30];
    int width;
    int height;
    int screenTilesX;
    int screenTilesY;
    int blocksX;
    int blocksY;
    int sortRowWidth;
    int sortRowCount;
    char unknown_58[0x5c - 0x58];
    int featureReproduceCursor;
    int windSpeedMin;
    int windSpeedMax;
    int rise;
    int tidal;
    char unknown_70[0x78 - 0x70];
    Ref<unsigned short> visibilityMask;
    char unknown_7c[0x84 - 0x7c];
    unsigned char seaLevel;
    IconSet* iconSet;
    Ref<Cell> cells;
    unsigned short* mapValues;
};

#include "../../interfaces/seams.h"

// FUNCTION: 0x483610
void LoadTntMap()
{
    MapSettings* mapSettings = (MapSettings*)((char*)g_game + 0x141fb);
    TntInfo info;
    GafFrame pic;
    char text[64];
    Slot a;
    Slot b;
    int* tnt;

    tnt = (int*)g_seams.map->GetNameSlot(g_game->net, 1);
    tnt = (int *)LoadFileWithProgress((char *)tnt);
    info.version = *tnt;
    switch (info.version) {
    case 0x1020:
        info.width = tnt[1];
        info.height = tnt[2];
        info.flag = tnt[9];
        info.sea_d = tnt[0xd];
        info.sea_a = tnt[10];
        info.sea_b = tnt[0xb];
        info.featureCount = tnt[7];
        info.featureNames = (TntFeatureName*)(tnt[8] + (int)tnt);
        info.tile_set_count = tnt[6];
        info.tile_set_src = (int*)(tnt[5] + (int)tnt);
        info.tile_map_src = (int*)(tnt[3] + (int)tnt);
        info.attr_b = (unsigned char*)(tnt[4] + (int)tnt);
        info.attr_a = 0;
        info.attr_limit = 0xfc;
        info.feature_flags = info.feature_flags ^ ((tnt[0xf] ^ info.feature_flags) & 1);
        info.feature_data = (unsigned short*)(tnt[0xe] + (int)tnt);
        break;
    case 0x2000:
        info.width = tnt[1];
        info.height = tnt[2];
        info.flag = tnt[9];
        info.sea_d = 0;
        info.sea_a = 100;
        info.sea_b = 2000;
        info.featureCount = tnt[7];
        info.featureNames = (TntFeatureName*)(tnt[8] + (int)tnt);
        info.tile_set_count = tnt[6];
        info.tile_set_src = (int*)(tnt[5] + (int)tnt);
        info.tile_map_src = (int*)(tnt[3] + (int)tnt);
        info.attr_a = (unsigned char*)(tnt[4] + (int)tnt);
        info.attr_b = 0;
        info.attr_limit = 0xfffb;
        info.feature_flags = info.feature_flags ^ ((tnt[0xb] ^ info.feature_flags) & 1);
        info.feature_data = (unsigned short*)(tnt[10] + (int)tnt);
        break;
    default:
        sprintf(text, "Unknown TNT version:  0x%08x", info.version);
        FatalError(text);
        break;
    }

    a.n = g_game->net->minWindSpeed;
    if (a.n >= 0 && info.version >= 0x2000)
        mapSettings->windSpeedMin = a.n;
    else
        mapSettings->windSpeedMin = info.sea_a;
    a.n = g_game->net->maxWindSpeed;
    if (a.n >= 0 && info.version >= 0x2000)
        mapSettings->windSpeedMax = a.n;
    else
        mapSettings->windSpeedMax = info.sea_b;
    a.n = g_game->net->gravity;
    if (a.n >= 0 && info.version >= 0x2000)
        mapSettings->rise = (int)((a.n * 65536.0) * 0.0011111111111111111);
    else if (info.sea_d != 0)
        mapSettings->rise = (int)((info.sea_d * 65536.0) * 0.0011111111111111111);
    else
        mapSettings->rise = 0x1fdb;
    if (g_game->net->tidalStrength >= 0.0f)
        mapSettings->tidal = *(int*)&g_game->net->tidalStrength;
    else
        mapSettings->tidal = 0x3f000000;
    mapSettings->seaLevel = (unsigned char)info.flag;
    mapSettings->width = info.width;
    mapSettings->height = info.height;
    mapSettings->baseX = mapSettings->width << 4;
    mapSettings->baseY = mapSettings->height << 4;
    if (info.feature_flags & 1) {
        pic.width = *info.feature_data;
        pic.height = info.feature_data[2];
        pic.xOffset = 0;
        pic.yOffset = 0;
        pic.transparency = 0;
        pic.compressed = 0;
        pic.layers = 0;
        pic.blend = 0;
        pic.reserved = 0;
        pic.pixelsOrLayers = (unsigned char*)(info.feature_data + 4);
        pic.scratch = 0;
        g_game->radarFrame = AllocFrame("TED GENERATED PIC", *(int*)info.feature_data, *(int*)(info.feature_data + 2));
        SurfaceFromFrame((Surface *)text, (GafFrame *)g_game->radarFrame);
        DrawFrame(text, &pic, 0, 0);
    } else {
        g_game->radarFrame = 0;
    }

    a.n = (mapSettings->baseX / 32) * (mapSettings->baseY / 32);
    int* dst = (int*)GameAllocIgnoreTag("TILE MAP", a.n * 2);
    mapSettings->mapValues = (unsigned short*)dst;
    memcpy(dst, info.tile_map_src, a.n * 2);
    a.n = mapSettings->width * mapSettings->height;
    unsigned char* plot = (unsigned char*)SimAllocTagged("PLOT MEMORY", a.n * 0xd);
    mapSettings->cells = (Cell*)plot;
    int fill = g_game->net->surfaceMetal;
    if (fill < 0 || info.version < 0x2000)
        fill = 0;
    for (int i = a.n; i > 0; i--) {
        plot[0xc] &= 0xfc;
        *(unsigned short*)plot = 0;
        *(unsigned short*)(plot + 2) = 0;
        *(unsigned short*)(plot + 8) = 0xffff;
        plot[7] = (unsigned char)fill;
        plot += 0xd;
    }
    g_seams.features->InitFeatureAnimPool(&info);
    if (info.attr_b != 0) {
        unsigned char* q = (unsigned char*)mapSettings->cells.get();
        if (a.n > 0) {
            unsigned char* src = info.attr_b;
            for (int i = a.n; i > 0; i--) {
                q[4] = *src;
                q[7] = src[6];
                q[0xc] = (q[0xc] & 0xd7) | 0x50;
                q += 0xd;
                src += 8;
            }
        }
        if (g_game->pendingSaveStore == 0) {
            q = (unsigned char*)mapSettings->cells.get();
            if (a.n > 0) {
                unsigned char* src = info.attr_b + 2;
                do {
                    if (*src < info.attr_limit)
                        g_seams.features->PlaceFeature((Cell*)q, *src, 0, 0, 10);
                    q += 0xd;
                    src += 8;
                    a.n--;
                } while (a.n != 0);
            }
        }
    } else {
        if (info.attr_a != 0) {
            unsigned char* q = (unsigned char*)mapSettings->cells.get();
            unsigned char* src = info.attr_a;
            if (a.n > 0) {
                b.n = a.n;
                do {
                    q[4] = *src;
                    q[0xc] = (q[0xc] & 0xd7) | 0x50;
                    if (*(unsigned short*)(src + 1) == 0xfffc)
                        g_seams.features->PlaceFeature((Cell*)q, 0xfffc, 0, 0, 10);
                    q += 0xd;
                    src += 4;
                    b.n--;
                } while (b.n != 0);
            }
            if (g_game->pendingSaveStore == 0) {
                q = (unsigned char*)mapSettings->cells.get();
                if (a.n > 0) {
                    unsigned short* sp = (unsigned short*)(info.attr_a + 1);
                    int n = a.n;
                    do {
                        if ((int)*sp < info.attr_limit)
                            g_seams.features->PlaceFeature((Cell*)q, *sp, 0, 0, 10);
                        q += 0xd;
                        sp += 2;
                        n--;
                    } while (n != 0);
                }
                g_seams.features->PlaceMissionFeatures();
            }
        }
    }

    unsigned int* set = (unsigned int*)GameAllocIgnoreTag("TILE SET", info.tile_set_count * 0x400 + 8);
    mapSettings->iconSet = (IconSet*)set;
    *set = info.tile_set_count;
    mapSettings->iconSet->data = (unsigned char*)mapSettings->iconSet + 8;
    memcpy(mapSettings->iconSet->data, info.tile_set_src, info.tile_set_count * 0x400);
    GameFreeThunk(tnt);
    g_losTables.LoadLosTables();
    int mw = g_game->viewWidth;
    int mh = g_game->viewHeight;
    mapSettings->screenTilesX = mw / 16;
    mapSettings->screenTilesY = mh / 16;
    mapSettings->blocksX = mw / 32;
    mapSettings->blocksY = mh / 32;
    int* list = (int*)operator new(0x10);
    int* obj = 0;
    if (list != 0) {
        list[1] = 0;
        list[2] = 0;
        list[3] = 0;
        list[0] = 0;
        obj = list;
    }
    int rb = 2;
    mapSettings->grid = (MapFogGrid*)obj;
    a.n = 2;
    if (mw % 32 != 0)
        a.n = 3;
    if (mh % 32 != 0)
        rb = 3;
    int cols, rows;
    rows = mapSettings->screenTilesY / 2 + rb;
    cols = mapSettings->screenTilesX / 2 + a.n;
    obj[1] = cols;
    obj[2] = rows;
    operator delete((void*)obj[0]);
    unsigned int total = (rows * cols + 7U) & 0xfffffff8;
    obj[3] = total;
    int buf;
    if (total != 0)
        buf = (int)operator new(total * 2);
    else
        buf = 0;
    obj[0] = buf;
    g_game->mapFlags_14281.raw &= 0xfff7;
    b.p.x = 0;
    b.p.y = 0;
    a.p.x = (short)g_game->width;
    a.p.y = (short)g_game->height;
    UpdateCellHeightRange(b.p, a.p);

    BuildDerivedLayers();
    ClearBorderFeatures();
    unsigned int total2 = (unsigned int)(mapSettings->width * mapSettings->height) * 2;
    unsigned int half = total2 / 4;
    int* mapped = (int*)SimAllocTagged("MAPPED MEMORY", half);
    mapSettings->visibilityMask = (unsigned short*)mapped;
    memset(mapped, 0, half);
    int sx = mapSettings->screenTilesX + 0xc;
    int sy = mapSettings->screenTilesY + 0x20;
    mapSettings->sortRowCount = sy;
    mapSettings->sortRowWidth = sx;
    mapSettings->sortUnits = (int*)GameAllocIgnoreTag("SORT UNIT LIST", sy * sx * 4);
    mapSettings->sortIndices = (int*)GameAllocIgnoreTag("SORT INDICES", mapSettings->sortRowCount << 2);
    mapSettings->sortLineCount = (int*)GameAllocIgnoreTag("SORT LINE COUNT", mapSettings->sortRowCount << 1);
    g_seams.features->StampFeatureMetal();
    g_game->count_14277 = 0;
    (*(Eye **)&g_game->eyes) = (Eye*)SimAllocTagged("EYEBALL MEMORY", 0x2d0);
    mapSettings->featureReproduceCursor = 0;
    g_game->loadProgress = 100;
}

// FUNCTION: 0x483dd0
void FreeMapResources()
{
    GameFreeThunk((Eye *)g_game->eyes);
    g_seams.features->FreeFeaturePool();
    if (g_game->radarFrame) {
        GameFreeThunk(g_game->radarFrame);
        g_game->radarFrame = 0;
    }
    GameFreeThunk((int *)g_game->field_14203);
    GameFreeThunk((int *)g_game->field_141ff);
    GameFreeThunk((int *)g_game->field_141fb);
    GameFreeThunk((IconSet *)g_game->iconSet);
    GameFreeThunk(g_game->visibilityMask);
    GameFreeThunk(g_game->cells);
    GameFreeThunk(g_game->mapValues);
    (*(int **)&g_game->field_14203) = 0;
    (*(int **)&g_game->field_141ff) = 0;
    (*(int **)&g_game->field_141fb) = 0;
    (*(IconSet **)&g_game->iconSet) = 0;
    g_game->visibilityMask = 0;
    g_game->cells = 0;
    g_game->mapValues = 0;

    void** p = (void**)(MapFogGrid *)g_game->fogGrid;
    if (p) {
        operator delete(p[0]);
        operator delete(p);
    }
    (*(MapFogGrid **)&g_game->fogGrid) = 0;

    FogGrid* g1 = (FogGrid*)&g_game->grid1;
    g1->width = 0;
    g1->height = 0;
    operator delete(g1->cells);
    g1->count = 0;
    g1->cells = 0;

    Grid2* g2 = &g_game->spatialGrid_1429f;
    g2->width = 0;
    g2->height = 0;
    operator delete(g2->cells);
    g2->count = 0;
    g2->cells = 0;

    operator delete(g_game->overflowBucket.get());
    g_game->overflowBucket = 0;
}

// FUNCTION: 0x483fa0
void __stdcall DrawMapTiles(void* surface)
{
    int tilesX, tilesY;
    int tileX, tileY;
    int screenX, screenY;
    int offX, offY;
    int edgeX, edgeY;
    int scrollX, scrollY, viewW, viewH;
    GafFrame bmp;
    screenX = g_game->rect.left;
    screenY = g_game->rect.top;
    scrollX = g_game->scrollX;
    scrollY = g_game->scrollY;
    viewW = g_game->viewWidth;
    viewH = g_game->viewHeight;
    tileX = scrollX / 32;
    tileY = scrollY / 32;
    offX = scrollX - tileX * 32;
    offY = scrollY - tileY * 32;
    tilesX = (viewW + offX) / 32;
    tilesY = (viewH + offY) / 32;
    edgeX = (viewW - tilesX * 32) + offX;
    edgeY = (viewH - tilesY * 32) + offY;
    if (edgeX != 0)
        tilesX++;
    if (edgeY != 0)
        tilesY++;
    int stride = g_game->width / 2;
    bmp.width = 32;
    bmp.height = 32;
    bmp.xOffset = 0;
    bmp.yOffset = 0;
    bmp.compressed = 0;
    bmp.layers = 0;

    if (offX != 0 || edgeX != 0) {
        unsigned short* left = g_game->mapValues + tileY * stride + tileX;
        unsigned short* right = g_game->mapValues + tileY * stride + tileX + tilesX - 1;
        for (int j = 0; j < tilesY; j++) {
            if (offX != 0) {
                bmp.pixelsOrLayers = ((IconSet *)g_game->iconSet)->data + *left * 0x400;
                DrawFrameOpaque((Surface *)surface, &bmp, screenX - offX, (screenY + j * 32) - offY);
            }
            if (edgeX != 0) {
                bmp.pixelsOrLayers = ((IconSet *)g_game->iconSet)->data + *right * 0x400;
                DrawFrameOpaque((Surface *)surface, &bmp, tilesX * 32 + screenX - offX - 32, (screenY + j * 32) - offY);
            }
            left += stride;
            right += stride;
        }
    }

    if (offY != 0 || edgeY != 0) {
        unsigned short* top = g_game->mapValues + tileY * stride + tileX;
        unsigned short* bottom = g_game->mapValues + (tileY + tilesY - 1) * stride + tileX;
        for (int i = 0; i < tilesX; i++) {
            if (offY != 0) {
                bmp.pixelsOrLayers = ((IconSet *)g_game->iconSet)->data + *top * 0x400;
                DrawFrameOpaque((Surface *)surface, &bmp, (screenX + i * 32) - offX, screenY - offY);
            }
            if (edgeY != 0) {
                bmp.pixelsOrLayers = ((IconSet *)g_game->iconSet)->data + *bottom * 0x400;
                DrawFrameOpaque((Surface *)surface, &bmp, (screenX + i * 32) - offX, tilesY * 32 + screenY - offY - 32);
            }
            top++;
            bottom++;
        }
    }

    if (offX != 0) {
        tilesX--;
        screenX += 32 - offX;
        tileX++;
    }
    if (offY != 0) {
        tilesY--;
        screenY += 32 - offY;
        tileY++;
    }
    if (edgeX != 0)
        tilesX--;
    if (edgeY != 0)
        tilesY--;

    for (int j = 0; j < tilesY; j++) {
        unsigned short* p = g_game->mapValues + (tileY + j) * (unsigned short)stride + tileX;
        for (int i = 0; i < tilesX; i++, p++) {
            DrawTile(surface, screenX + i * 32, screenY + j * 32, ((IconSet *)g_game->iconSet)->data + *p * 0x400);
        }
    }
}

// FUNCTION: 0x4848e0
void __stdcall DrawFogOfWar(void* surface)
{
    if (!(g_game->mapFlags_14281.raw & 8)) {
        BuildFogTiles();
        g_game->mapFlags_14281.raw |= 8;
    }

    MapFogGrid* grid = (MapFogGrid *)g_game->fogGrid;
    int scrollY = g_game->scrollY;
    int scrollX = g_game->scrollX;
    int q = (scrollY + 16) / 32 + (scrollX + 16) / 32;
    int parity = (g_game->scrollX + scrollY) & 1;
    int rX = scrollX % 32;
    int rY = scrollY % 32;
    int fx;
    if (rX < 16) fx = -16 - rX;
    else fx = 16 - rX;
    int fy;
    if (rY < 16) fy = -16 - rY;
    else fy = 16 - rY;
    Rect r;

    for (unsigned int j = 0; j < (unsigned int)grid->height; j++) {
        for (unsigned int i = 0; i < (unsigned int)grid->width; i++) {
            FogCell* cell = &grid->cells[j * grid->width + i];
            r.left = g_game->rect.left + fx + (i << 5);
            r.top = g_game->rect.top + fy + (j << 5);
            r.right = r.left + 31;
            r.bottom = r.top + 31;
            if (cell->level0 == 0xf) {
                FillRectangle((Surface*)surface, &r, g_game->colors[0]);
            } else {
                if (cell->level1 != 0) {
                    if (cell->level1 != 0xf) {
                        void* bmp = GetGafFrame(g_game->gray[(i + j + q) & 3], cell->level1 - 1);
                        if ((*(GraphicsOptionFlagsView*)&g_game->flags_37f06).ditheredFog) {
                            EraseFrameDithered((Surface *)surface, (GafFrame *)bmp, r.left, r.top, parity);
                        } else {
                            DrawFrameGray((Surface *)surface, (GafFrame *)bmp, r.left, r.top);
                        }
                    } else {
                        if ((*(GraphicsOptionFlagsView*)&g_game->flags_37f06).ditheredFog) {
                            DitherRectangle((Surface *)surface, &r, parity);
                        } else {
                            GrayRectangle((Surface *)surface, &r);
                        }
                    }
                }
                if (cell->level0 > 0) {
                    void* bmp = GetGafFrame(g_game->black[(i + j + q) & 3], cell->level0 - 1);
                    DrawFrame(surface, bmp, r.left, r.top);
                }
            }
        }
    }
}

// FUNCTION: 0x484b50
void __stdcall ClampWorldPosToTerrain(int x, int y, Vec3* out)
{
    if (x < 0)
        x = 0;
    if (x >= g_game->baseX)
        x = g_game->baseX - 1;
    if (y < 0)
        y = 0;
    if (y >= g_game->baseY)
        y = g_game->baseY - 1;

    Vec3 p;
    int s1;
    int s2;
    p.x = x << 16;
    int t = y & ~0xf;
    int i;
    for (i = 0x80; i >= 0; i -= 0x10) {
        p.z = (t + i) << 16;
        p.y = max(g_seams.units->GetGroundHeight(&p), g_game->seaLevel) << 16;
        s1 = ((short*)&p.z)[1] - (((short*)&p.y)[1] >> 1);
        if (s1 <= y)
            goto found;
    }
    goto done;

found:
    {
        Vec3 q = p;
        q.z = p.z + 0x100000;
        q.y = max(g_seams.units->GetGroundHeight(&q), g_game->seaLevel) << 16;
        s2 = ((short*)&q.z)[1] - (((short*)&q.y)[1] >> 1);
        if ((s1 < s2 && y >= s1) || y <= s2) {
            p.z = p.z + ((y - s1) << 20) / (s2 - s1);
            p.y = max(g_seams.units->GetGroundHeight(&p), g_game->seaLevel) << 16;
        }
    }
done:
    *out = p;
}

// FUNCTION: 0x4816a0
void __stdcall RecalculateLineOfSight(int arg)
{
    if (arg != 0) {
        memset(g_game->visibilityMask, (g_game->mapFlags_14281.raw & 1) ? 0 : 0xFFFF,
               g_game->width * g_game->height * sizeof(short) / 4);
    }
    for (unsigned char i = 0; i < 10; i++) {
        if (i >= 10) continue;
        Player* p = &g_game->players[i];
        if (p->active == 0) continue;
        if (p->type != 1 && p->type != 2 && p->type != 3) continue;
        if (p->index == 10) continue;
        memset(p->explored, (unsigned char)~((unsigned char)g_game->mapFlags_14281.raw >> 1) & 1, p->exploredSize);
    }
    for (Unit* u = g_game->units + 1; u <= g_game->unitsEnd; u++) {
        if (u->unitDefIndex == 0)
            continue;
        SightQuery params;
        params.player = u->player;
        params.cacheCell = &u->losCacheCellX;
        params.sightDistance = u->def->range;
        params.frameIdx = &u->losSightFrameIdx;
        params.pos = u->pos;
        params.eyeHeight = (unsigned char)u->def->modelMaxYHigh;
        if (params.pos.y < (int)((g_game->seaLevel + 1) << 16))
            params.pos.y = (g_game->seaLevel + 1) << 16;
        if ((g_game->mapFlags_14281.raw & 2) == 2) {
            *params.frameIdx = 0;
            if ((g_game->mapFlags_14281.raw & 4) == 4) {
                UpdateLineOfSight(&params);
            } else {
                int i = params.sightDistance / 32 - 5;
                if (i < 0)
                    i = 0;
                else if (i >= g_game->losTable->count)
                    i = g_game->losTable->count - 1;
                int y = ((short*)&params.pos.y)[1] / 64;
                int cx = params.pos.x / 0x200000;
                int cy = params.pos.z / 0x200000 - y;
                GafFrame* e = GetGafFrame(g_game->losTable, i);
                int vx = cx - e->xOffset;
                int vz = cy - e->yOffset;
                params.cacheCell[0] = (short)vx;
                params.cacheCell[1] = (short)vz;
                *params.frameIdx = (unsigned char)i;
                AddLineOfSight(&params);
                RevealAroundUnit(&params);
            }
        }
    }
    (*(Flags_142f1*)&g_game->viewDirtyFlags).mapChanged = 1;
    g_game->mapFlags_14281.raw &= 0xfff7;
    UpdateRadarMapped();
    DrawRadarUnits();
}

inline int LodRaw(SightQuery* params)
{
    return params->sightDistance / 32;
}

inline int Lod(SightQuery* params)
{
    int v = LodRaw(params);
    return v < 0 ? 0 : v;
}

// FUNCTION: 0x481930
void __stdcall RevealAroundUnit(SightQuery* params)
{
    int changed = 0;
    int x, y;
    int limitX, limitY, nx, ny;
    int i, stride, off;
    unsigned int bit = 1 << ((Player*)params->player)->index;
    GafFrame* frame;
    int halfW = g_game->width / 2;
    int halfH = g_game->height / 2;
    x = params->cacheCell[0];
    y = params->cacheCell[1];
    if (g_game->mapFlags_14281.bit2 == 1) {
        FogGrid* grid = (FogGrid*)&g_game->grid1;
        if ((unsigned)x < grid->width && (unsigned)y < grid->height) {
            LosTable* table =
                g_losTables
                    .GetLosTable(
                        (params->sightDistance / 32 < 0 ? 0 : params->sightDistance / 32) <
                                (short)g_losTables.GetLosTableCount() - 1
                            ? (params->sightDistance / 32 < 0 ? 0 : params->sightDistance / 32)
                            : (short)g_losTables.GetLosTableCount() - 1);
            short count = table->GetLosLineCount();
            unsigned short* cell = &g_game->visibilityMask[halfW * y + x];
            if ((unsigned short)(bit & *cell) == 0) {
                *cell ^= bit;
                changed = 1;
            }
            int ref = *params->frameIdx;
            for (short i = 0; (short)i < count; i++) {
                LosLine* line = table->GetLosLine(i);
                short num = line->GetLosLineStepCount();
                int bestIdx = 0;
                int j1;
                int bestDiff = -1;
                short j = 0;
                if ((short)num > 0) {
                    j1 = 1;
                    do {
                        unsigned short sx, sy;
                        line->GetLosLineStep((short)j, &sx, &sy);
                        int x2 = sx + x;
                        int y2 = sy + y;
                        if ((unsigned)(short)x2 < grid->width &&
                            (unsigned)(short)y2 < grid->height) {
                            unsigned char* c =
                                (unsigned char*)grid->cells.get() + ((short)y2 * grid->width + (short)x2) * 2;
                            int d1 = c[1] - ref;
                            int d0 = c[0] - ref;
                            if (d0 * bestIdx > bestDiff * j1) {
                                unsigned short* q = &g_game->visibilityMask[
                                    halfW * (short)y2 + (short)x2];
                                if ((unsigned short)(bit & *q) == 0) {
                                    *q ^= bit;
                                    changed = 1;
                                }
                                if (d1 * bestIdx > bestDiff * j1) {
                                    bestIdx = j1;
                                    bestDiff = d1;
                                }
                            }
                        }
                        j++;
                        j1++;
                    } while ((short)j < (short)num);
                }
            }
        }
    } else {
        int lod = LodRaw(params) - 5;
        if (lod < 0)
            lod = 0;
        else if (lod >= g_game->losTable->count)
            lod = g_game->losTable->count - 1;
        frame = GetGafFrame(g_game->losTable, lod);
        limitX = (x + frame->width < halfW) ? frame->width : halfW - x;
        limitY = (y + frame->height >= halfH) ? halfH - y : frame->height;
        nx = x < 0 ? -x : 0;
        ny = y < 0 ? -y : 0;
        changed = 0;
        i = ny;
        if (i < limitY) {
            stride = halfW * 2;
            off = ((y + ny) * halfW + nx + x) * 2;
            do {
                unsigned short* dst =
                    (unsigned short*)((unsigned char*)g_game->visibilityMask.get() + off);
                unsigned char* src = frame->pixelsOrLayers + i * frame->width + nx;
                if (nx < limitX) {
                    int n = limitX - nx;
                    do {
                        if (*src != frame->transparency && (unsigned short)(bit & *dst) == 0) {
                            changed = 1;
                            *dst ^= bit;
                        }
                        dst++;
                        src++;
                    } while (--n);
                }
                i++;
                off += stride;
            } while (i < limitY);
        }
    }
    if (changed && ((Player*)params->player)->index == g_game->playerIndex) {
        g_game->mapFlags_14281.bit3 = 0;
        (*(Flags_142f1*)&g_game->viewDirtyFlags).rawByte |= 4;
    }
}

// FUNCTION: 0x482910
void __stdcall AddEyeball(Vec3* src, int a, int b, int c)
{
    if ((g_game->mapFlags_14281.raw & 2) == 2 && g_game->count_14277 < 0x14) {
        Eye* e = &((Eye *)g_game->eyes)[g_game->count_14277];
        e->player = (char*)&g_game->players[g_game->playerIndex];
        e->screen = &e->screenPos;
        e->x = a;
        e->flagPtr = &e->flagB;
        e->pos = *src;
        e->flagA = b;
        int minY = (g_game->seaLevel + 1) << 16;
        if (e->pos.y < minY) {
            e->pos.y = minY;
        }
        e->expires = (unsigned int)g_game->ticks + c;
        if ((g_game->mapFlags_14281.raw & 2) == 2) {
            *e->flagPtr = 0;
            if ((g_game->mapFlags_14281.raw & 4) == 4) {
                UpdateLineOfSight((SightQuery*)e);
            } else {
                int lod = e->x / 32 - 5;
                if (lod < 0) {
                    lod = 0;
                } else if (lod >= g_game->losTable->count) {
                    lod = g_game->losTable->count - 1;
                }
                int cell_x = e->pos.x / 0x200000;
                int cell_y = e->pos.z / 0x200000 - ((short*)&e->pos.y)[1] / 64;
                GafFrame* ce = GetGafFrame(g_game->losTable, lod);
                cell_x -= ce->xOffset;
                cell_y -= ce->yOffset;
                e->screen->x = (short)cell_x;
                e->screen->y = (short)cell_y;
                *e->flagPtr = (char)lod;
                AddLineOfSight((SightQuery*)e);
                RevealAroundUnit((SightQuery*)e);
            }
        }
        g_game->count_14277++;
    }
}

// FUNCTION: 0x482ac0
void __stdcall RevealNewUnit(Unit* unit)
{
    SightQuery p;
    p.player = unit->player;
    p.cacheCell = &unit->losCacheCellX;
    p.sightDistance = unit->def->range;
    p.frameIdx = &unit->losSightFrameIdx;
    p.pos = unit->pos;
    p.eyeHeight = (unsigned char)unit->def->modelMaxYHigh;
    int min_y = (g_game->seaLevel + 1) << 16;
    if (p.pos.y < min_y) {
        p.pos.y = min_y;
    }
    if ((g_game->mapFlags_14281.raw & 2) == 2) {
        *p.frameIdx = 0;
        if ((g_game->mapFlags_14281.raw & 4) == 4) {
            UpdateLineOfSight(&p);
        } else {
            int i = p.sightDistance / 32 - 5;
            if (i < 0) {
                i = 0;
            } else if (i >= g_game->losTable->count) {
                i = g_game->losTable->count - 1;
            }
            int cell_x = p.pos.x / 0x200000;
            int cell_y = p.pos.z / 0x200000 - ((short*)&p.pos.y)[1] / 64;
            GafFrame* e = GetGafFrame(g_game->losTable, i);
            cell_x -= e->xOffset;
            cell_y -= e->yOffset;
            p.cacheCell[0] = (short)cell_x;
            p.cacheCell[1] = (short)cell_y;
            *p.frameIdx = i;
            AddLineOfSight(&p);
            RevealAroundUnit(&p);
        }
    }
}

static __inline unsigned char* ExploredCell(Player* player, int x, int y)
{
    return &player->explored[(unsigned int)y * (unsigned int)player->exploredWidth + (unsigned int)x];
}

// FUNCTION: 0x481d50
void __stdcall RemoveLineOfSight(SightQuery* params)
{
    Player* player = params->player.get();
    if (player->index == g_game->playerIndex) {
        g_game->bits_14281_1.flag3 = 0;
        g_game->bits_142f1_1.flags_142f1_mapChanged = 1;
    }
    int halfW = g_game->width / 2;
    int halfH = g_game->height / 2;
    int x = params->cacheCell[0];
    int y = params->cacheCell[1];
    if (g_game->bits_14281_1.flag2 == 1) {
        Grid* grid = &g_game->grid1;
        if ((unsigned)x >= grid->width)
            return;
        if ((unsigned)y >= grid->height)
            return;
        LosTable* table = g_losTables.GetLosTable(
            (params->sightDistance / 32 < 0 ? 0 : params->sightDistance / 32) <
                    (short)g_losTables.GetLosTableCount() - 1
                ? (params->sightDistance / 32 < 0 ? 0 : params->sightDistance / 32)
                : (short)g_losTables.GetLosTableCount() - 1);
        short count = table->GetLosLineCount();
        short i = 0;
        (*ExploredCell(player, x, y))--;
        int ref = *params->frameIdx;
        for (i = 0; i < count; i++) {
            LosLine* line = table->GetLosLine(i);
            short num = line->GetLosLineStepCount();
            int bestIdx = 0;
            int j1;
            int bestDiff = -1;
            short j = 0;
            if ((short)num > 0) {
                j1 = 1;
                do {
                    unsigned short sx, sy;
                    line->GetLosLineStep(j, &sx, &sy);
                    int dx = sx + x;
                    int dy = sy + y;
                    if ((unsigned)(short)dx >= grid->width)
                        continue;
                    if ((unsigned)(short)dy >= grid->height)
                        continue;
                    unsigned char* cell =
                        grid->cells + ((short)dy * grid->width + (short)dx) * 2;
                    int d1 = cell[1] - ref;
                    int d0 = cell[0] - ref;
                    if (d0 * bestIdx > bestDiff * j1) {
                        (*ExploredCell(player, (short)dx, (short)dy))--;
                        if (d1 * bestIdx > bestDiff * j1) {
                            bestIdx = j1;
                            bestDiff = d1;
                        }
                    }
                } while (j++, j1++, (short)j < (short)num);
            }
        }
    } else {
        int ref = *params->frameIdx;
        GafFrame* frame = GetGafFrame(g_game->losTable, ref);
        int limitX;
        if (x + frame->width >= halfW)
            limitX = halfW - x;
        else
            limitX = frame->width;
        int limitY;
        if (y + frame->height >= halfH)
            limitY = halfH - y;
        else
            limitY = frame->height;
        int nx = x < 0 ? -x : 0;
        int ny = y < 0 ? -y : 0;
        if (ny >= limitY)
            return;
        if (nx >= limitX)
            return;
        for (int i = ny; i < limitY; i++) {
            unsigned char* dst = player->explored + ((unsigned int)(y + i) * (unsigned int)player->exploredWidth + nx + x);
            unsigned char* src = frame->pixelsOrLayers + i * frame->width + nx;
            for (int j = nx; j < limitX; j++) {
                if (*src != frame->transparency)
                    (*dst)--;
                dst++;
                src++;
            }
        }
    }
}

// FUNCTION: 0x482270
void __stdcall AddLineOfSight(SightQuery* params)
{
    Player* player = params->player.get();
    if (player->index == g_game->playerIndex) {
        g_game->bits_14281_1.flag3 = 0;
        g_game->bits_142f1_2.flagA = 1;
    }
    int halfW = g_game->width / 2;
    int halfH = g_game->height / 2;
    int x = params->cacheCell[0];
    int y = params->cacheCell[1];
    if (g_game->bits_14281_1.flag2 == 1) {
        Grid* grid = &g_game->grid1;
        if ((unsigned)x >= grid->width)
            return;
        if ((unsigned)y >= grid->height)
            return;
        LosTable* table = g_losTables.GetLosTable(
            (params->sightDistance / 32 < 0 ? 0 : params->sightDistance / 32) <
                    (short)g_losTables.GetLosTableCount() - 1
                ? (params->sightDistance / 32 < 0 ? 0 : params->sightDistance / 32)
                : (short)g_losTables.GetLosTableCount() - 1);
        short count = table->GetLosLineCount();
        short i = 0;
        (*ExploredCell(player, x, y))++;
        int ref = *params->frameIdx;
        for (i = 0; i < count; i++) {
            LosLine* line = table->GetLosLine(i);
            short num = line->GetLosLineStepCount();
            int bestIdx = 0;
            int j1;
            int bestDiff = -1;
            short j = 0;
            if ((short)num > 0) {
                j1 = 1;
                do {
                    unsigned short sx, sy;
                    line->GetLosLineStep(j, &sx, &sy);
                    int dx = sx + x;
                    int dy = sy + y;
                    if ((unsigned)(short)dx >= grid->width)
                        continue;
                    if ((unsigned)(short)dy >= grid->height)
                        continue;
                    unsigned char* cell =
                        grid->cells + ((short)dy * grid->width + (short)dx) * 2;
                    int d1 = cell[1] - ref;
                    int d0 = cell[0] - ref;
                    if (d0 * bestIdx > bestDiff * j1) {
                        (*ExploredCell(player, (short)dx, (short)dy))++;
                        if (d1 * bestIdx > bestDiff * j1) {
                            bestIdx = j1;
                            bestDiff = d1;
                        }
                    }
                } while (j++, j1++, (short)j < (short)num);
            }
        }
    } else {
        int ref = *params->frameIdx;
        GafFrame* frame = GetGafFrame(g_game->losTable, ref);
        int limitX;
        if (x + frame->width >= halfW)
            limitX = halfW - x;
        else
            limitX = frame->width;
        int limitY;
        if (y + frame->height >= halfH)
            limitY = halfH - y;
        else
            limitY = frame->height;
        int nx = x < 0 ? -x : 0;
        int ny = y < 0 ? -y : 0;
        if (ny >= limitY)
            return;
        if (nx >= limitX)
            return;
        for (int i = ny; i < limitY; i++) {
            unsigned char* dst = player->explored + ((unsigned int)(y + i) * (unsigned int)player->exploredWidth + nx + x);
            unsigned char* src = frame->pixelsOrLayers + i * frame->width + nx;
            for (int j = nx; j < limitX; j++) {
                if (*src != frame->transparency)
                    (*dst)++;
                dst++;
                src++;
            }
        }
    }
}


// FUNCTION: 0x4843c0
void BuildFogTiles(void)
{
    MapInfo* info = &g_game->info_141fb;
    MapFogGrid* grid = (MapFogGrid*)info->grid;
    PlayerGrid* pg = &g_game->players_1bdf[g_game->playerIndex];
    unsigned int bit = 1 << g_game->playerIndex;

    memset(grid->cells, 0, grid->count * 2);

    int y0 = g_game->scrollY;
    int ry = y0 % 32;
    int x0 = g_game->scrollX;
    int rx = x0 % 32;
    if (rx < 16) x0 = x0 / 32 - 1; else x0 = x0 / 32;
    if (ry < 16) y0 = y0 / 32 - 1; else y0 = y0 / 32;

    int xEnd = x0 + grid->width;
    int yEnd = y0 + grid->height;

    for (int y = y0; y < yEnd; y++) {
        for (int x = x0; x < xEnd; x++) {
            if (x < pg->width && y < pg->height) {
                if (pg->cells[y * pg->width + x] == 0 && (g_game->mapFlags_14281.raw & 2)) {
                    if (x - x0 < grid->width && y - y0 < grid->height)
                        grid->cells[(y - y0) * grid->width + x - x0].hi |= 1;
                    if (x - x0 - 1 < grid->width && y - y0 < grid->height)
                        grid->cells[(y - y0) * grid->width + x - x0 - 1].hi |= 2;
                    if (x - x0 < grid->width && y - y0 - 1 < grid->height)
                        grid->cells[(y - y0 - 1) * grid->width + x - x0].hi |= 4;
                    if (x - x0 - 1 < grid->width && y - y0 - 1 < grid->height)
                        grid->cells[(y - y0 - 1) * grid->width + x - x0 - 1].hi |= 8;
                }
                if ((unsigned short)(bit & info->visibilityMask[info->width * y / 2 + x]) == 0) {
                    if (x - x0 < grid->width && y - y0 < grid->height)
                        grid->cells[(y - y0) * grid->width + x - x0].lo |= 1;
                    if (x - x0 - 1 < grid->width && y - y0 < grid->height)
                        grid->cells[(y - y0) * grid->width + x - x0 - 1].lo |= 2;
                    if (x - x0 < grid->width && y - y0 - 1 < grid->height)
                        grid->cells[(y - y0 - 1) * grid->width + x - x0].lo |= 4;
                    if (x - x0 - 1 < grid->width && y - y0 - 1 < grid->height)
                        grid->cells[(y - y0 - 1) * grid->width + x - x0 - 1].lo |= 8;
                }
            }
        }
    }

    if (y0 < 0) {
        for (unsigned int i = 0; i < grid->width; i++) {
            if (g_game->mapFlags_14281.bit1) {
                if (grid->cells[i].hi & 4) grid->cells[i].hi |= 1;
                if (grid->cells[i].hi & 8) grid->cells[i].hi |= 2;
            }
            if (grid->cells[i].lo & 4) grid->cells[i].lo |= 1;
            if (grid->cells[i].lo & 8) grid->cells[i].lo |= 2;
        }
    }

    if (yEnd > info->height / 2) {
        for (unsigned int i = 0; i < grid->width; i++) {
            if (g_game->mapFlags_14281.bit1) {
                if (grid->cells[(grid->height - 2) * grid->width + i].hi & 1)
                    grid->cells[(grid->height - 2) * grid->width + i].hi |= 4;
                if (grid->cells[(grid->height - 2) * grid->width + i].hi & 2)
                    grid->cells[(grid->height - 2) * grid->width + i].hi |= 8;
            }
            if (grid->cells[(grid->height - 2) * grid->width + i].lo & 1)
                grid->cells[(grid->height - 2) * grid->width + i].lo |= 4;
            if (grid->cells[(grid->height - 2) * grid->width + i].lo & 2)
                grid->cells[(grid->height - 2) * grid->width + i].lo |= 8;
        }
    }

    if (x0 < 0) {
        for (unsigned int i = 0; i < grid->height; i++) {
            if (g_game->mapFlags_14281.bit1) {
                if (grid->cells[i * grid->width].hi & 8)
                    grid->cells[i * grid->width].hi |= 4;
                if (grid->cells[i * grid->width].hi & 2)
                    grid->cells[i * grid->width].hi |= 1;
            }
            if (grid->cells[i * grid->width].lo & 8)
                grid->cells[i * grid->width].lo |= 4;
            if (grid->cells[i * grid->width].lo & 2)
                grid->cells[i * grid->width].lo |= 1;
        }
    }

    if (xEnd > info->width / 2) {
        if (grid->height > 0) {
            unsigned int i = 1;
            do {
                if (g_game->mapFlags_14281.bit1) {
                    if (grid->cells[i * grid->width - 2].hi & 4)
                        grid->cells[i * grid->width - 2].hi |= 8;
                    if (grid->cells[i * grid->width - 2].hi & 1)
                        grid->cells[i * grid->width - 2].hi |= 2;
                }
                if (grid->cells[i * grid->width - 2].lo & 4)
                    grid->cells[i * grid->width - 2].lo |= 8;
                if (grid->cells[i * grid->width - 2].lo & 1)
                    grid->cells[i * grid->width - 2].lo |= 2;
                i++;
            } while (i - 1 < grid->height);
        }
    }
}
