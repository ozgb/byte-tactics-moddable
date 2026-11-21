#include "../state/block.h"

#include "../../interfaces/ref.h"
#include "../../interfaces/map.h"
#include "../../interfaces/pathfinding.h"
#include "../../interfaces/seams.h"
#include "../util/vec3.h"

#include "pass_map.h"

void __cdecl operator delete(void* p);

#pragma pack(push, 1)
struct UnitPassMapRecordView {
    Ref<MotionTickView> unit;
    char unknown_4[0x76 - 0x4];
    Point16 a;
    char unknown_7a[0x7e - 0x7a];
    Point16 b;
    char unknown_82[0x110 - 0x82];
    unsigned int flags;
    char unknown_114[0x118 - 0x114];
};

#include "cell.h"
#pragma pack(pop)

#include "terrain.h"
#include "../network/player.h"
#include "../game/map_events.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)
#include "../state/sim_globals.h"

#include "../../link/globals.h"

// FUNCTION: 0x440470
void PassMap::Resize(unsigned int w, unsigned int h)
{
    width = w;
    height = h;
    unsigned int n = ((h + 15) >> 4) * w;
    delete cells;
    if (n != 0) {
        cells = new (kSim) unsigned int[n];
    } else {
        cells = 0;
    }
}

// FUNCTION: 0x4404c0
void PassMap::SetCell(int x, int y, int value)
{
    int shift = (y & 0xf) << 1;
    int row = (y >> 4) * width + x;
    unsigned int* p = cells + row;
    *p = (value << shift) | (~(3 << shift) & *p);
}

static inline void setcell(unsigned int* q, int sh, unsigned int val)
{
    *q = (*q & ~(3 << sh)) | (val << sh);
}

// FUNCTION: 0x440500
void PassMap::Build()
{
    unsigned char b;
    unsigned char* p;
    unsigned char* buf;
    int n;
    unsigned char* v;
    unsigned int size;
    if (width + footprintX > footprintZ + height)
        size = width + footprintX;
    else
        size = footprintZ + height;

    buf = (unsigned char*)SimAlloc(size + 3);
    v = buf + 2;

    {
    for (int j = 0; j < height; j++) {
        Cell* c = &g_game->cells[j * width];
        for (n = 0; n < width; n++, c++)
            v[n] = (unsigned char)GetPassMapCellValue(this, c);
        v[-2] = 0;
        v[-1] = 0;
        for (n = 0; n <= footprintX; n++)
            (v + n)[width] = 0;
        b = 0;
        p = v;
        n = 0;
        for (; n < width; n++, p++) {
            int e = n + footprintX - 1;
            if (v[e] <= b) {
                b = v[e];
            } else if (p[-1] <= b) {
                b = p[0];
                int k;
                for (k = n + 1; k <= e; k++)
                    if (b >= v[k]) b = v[k];
            }
            if (b == 3 && (p[-1] < 3 || v[e + 1] < 3))
                setcell(&cells[(j >> 4) * width + n], (j & 0xf) * 2, 1);
            else
                setcell(&cells[(j >> 4) * width + n], (j & 0xf) * 2, b);
        }
    }
    }

    {
    for (int j = 0; j < width; j++) {
        for (n = 0; n < height; n++)
            v[n] = (unsigned char)((cells[(n >> 4) * width + j]
                                    >> ((n & 0xf) * 2)) & 3);
        v[-2] = 0;
        v[-1] = 0;
        for (n = 0; n <= footprintZ; n++)
            (v + n)[height] = 0;
        b = 0;
        n = 0;
        for (; n < height; n++) {
            int e = n + footprintZ - 1;
            if (v[e] <= b) {
                b = v[e];
            } else if (v[n - 1] <= b) {
                b = v[n];
                int k;
                for (k = n + 1; k <= e; k++)
                    if (b >= v[k]) b = v[k];
            }
            if (b == 3 && (v[n - 1] < 3 || v[e + 1] < 3))
                setcell(&cells[(n >> 4) * width + j], (n & 0xf) * 2, 1);
            else
                setcell(&cells[(n >> 4) * width + j], (n & 0xf) * 2, b);
        }
    }
    }

    operator delete(buf);
}

// FUNCTION: 0x440830
void PassMap::Refresh(Point16 a, Point16 b)
{
    int left = a.x - footprintX;
    int top = a.y - footprintZ;
    int right = a.x + b.x + 1;
    int bottom = a.y + b.y + 1;
    if (left < 0) {
        left = 0;
    }
    if (top < 0) {
        top = 0;
    }
    if (right > width) {
        right = width;
    }
    if (bottom > height) {
        bottom = height;
    }
    if (left < right && top < bottom) {
        for (int y = top; y < bottom; y++) {
            for (int x = left; x < right; x++) {
                unsigned int v = GetPassMapFootprintValueWithEdgeStrips(this, x, y);
                unsigned int m = 3 << ((y & 0xf) * 2);
                unsigned int* p = &cells[(y >> 4) * width + x];
                *p = (*p & ~m) | (v << ((y & 0xf) * 2));
            }
        }
    }
}

// FUNCTION: 0x440940
void __stdcall BuildAllPassMaps(const MapMoveLimits* limits, int count)
{
    for (int i = 0; i < 32; i++)
        g_passMaps.entries[i].defined = 0;
    for (int i = 0; i < count; i++) {
        const MapMoveLimits* l = &limits[i];
        PassMap* m = &g_passMaps.entries[l->index];
        m->footprintX = l->footprintX;
        m->footprintZ = l->footprintZ;
        m->maxWaterDepth = l->maxWaterDepth;
        m->minWaterDepth = l->minWaterDepth;
        m->maxSlope = l->maxSlope;
        m->badSlope = l->badSlope;
        m->maxWaterSlope = l->maxWaterSlope;
        m->badWaterSlope = l->badWaterSlope;
        m->defined = 1;
    }

    int progress = 100;
    for (int i = 0; i < 32; i++) {
        PassMap* m = &g_passMaps.entries[i];
        if (m->defined == 0)
            continue;
        m->width = (unsigned int)g_game->width;
        m->height = (unsigned int)g_game->height;
        unsigned int n = ((m->height + 0xf) >> 4) * m->width;
        operator delete(m->cells.get());
        if (n != 0) {
            m->cells = (unsigned int*)SimAlloc(n * 4);
        } else {
            m->cells = nullptr;
        }
        m->Build();
        progress += 100;
        g_game->progress_38d73 = progress / count;
    }

    g_game->progress_38d73 = 100;
}

void __stdcall FreePassMaps(void)
{
    for (int i = 0; i < 32; i++) {
        PassMap* m = &g_passMaps.entries[i];
        void* c = m->cells.get();
        m->width = 0;
        m->height = 0;
        operator delete(c);
        m->cells = nullptr;
        m->defined = 0;
    }
}

#define PASS_MAP_CHANGE_ARGS(pm, a, b) \
    (a).x - (pm)->footprintX < 0 ? 0 : (a).x - (pm)->footprintX, \
    (a).y - (pm)->footprintZ < 0 ? 0 : (a).y - (pm)->footprintZ, \
    (a).x + (b).x >= (int)(pm)->width ? (int)(pm)->width - 1 : (a).x + (b).x, \
    (a).y + (b).y >= (int)(pm)->height ? (int)(pm)->height - 1 : (a).y + (b).y, \
    1u << (int)((pm) - g_passMaps.entries)

#define PUBLISH_PASS_MAP_CHANGE(pm, a, b, cause) \
    MapPublishCellsChanged(PASS_MAP_CHANGE_ARGS(pm, a, b), cause)

static void RefreshAndReportAllPassMaps(Point16 a, Point16 b, int cause)
{
    for (int i = 0; i < 32; i++) {
        if (g_passMaps.entries[i].defined != 0) {
            g_passMaps.entries[i].Refresh(a, b);
        }
    }
    for (int i = 0; i < 32; i++) {
        if (g_passMaps.entries[i].defined != 0) {
            PUBLISH_PASS_MAP_CHANGE(&g_passMaps.entries[i], a, b, cause);
        }
    }
}

// FUNCTION: 0x440a40
void __stdcall RefreshAllPassMaps(Point16 a, Point16 b)
{
    RefreshAndReportAllPassMaps(a, b, PATH_CHANGE_TERRAIN);
}

void __stdcall RefreshAllPassMapsForUnits(Point16 a, Point16 b)
{
    RefreshAndReportAllPassMaps(a, b, PATH_CHANGE_UNITS);
}

// FUNCTION: 0x440a70
void __stdcall RefreshPassMapsForUnit(UnitPassMapView* p)
{
    if (p->unit != 0) {
        unsigned int last = p->unit->lastTick;
        p->unit->lastTick = (unsigned int)g_game->ticks;
        for (int i = 0; i < 32; i++) {
            if (last < g_passMaps.entries[i].lastTick) {
                g_passMaps.entries[i].Refresh(p->a, p->b);
                PUBLISH_PASS_MAP_CHANGE(&g_passMaps.entries[i], p->a, p->b, PATH_CHANGE_UNITS);
            }
        }
        return;
    }
    RefreshAllPassMapsForUnits(p->a, p->b);
}

// FUNCTION: 0x440af0
void PassMap::RefreshMovedUnits(UnitPassMapView* p)
{
    unsigned int now = (unsigned int)g_game->ticks;
    unsigned int old = lastTick;
    if (now <= 0x1e) {
        now = 0x1e;
    }
    unsigned int start = now - 0x1e;
    lastTick = start;
    unsigned int prev = p->unit->lastTick;
    p->unit->lastTick = (unsigned int)g_game->ticks;
    if (prev < old) {
        Refresh(p->a, p->b);
        PUBLISH_PASS_MAP_CHANGE(this, p->a, p->b, PATH_CHANGE_UNITS);
    }
    if (start != old) {
        for (UnitPassMapRecordView* r = &((UnitPassMapRecordView *)g_game->units.get())[1]; r <= (UnitPassMapRecordView *)g_game->unitsEnd.get(); r++) {
            if ((r->flags & 0x10000000) != 0 && r->unit != 0) {
                unsigned int t = r->unit->lastTick;
                if (t >= old && t < start) {
                    Refresh(r->a, r->b);
                    PUBLISH_PASS_MAP_CHANGE(this, r->a, r->b, PATH_CHANGE_UNITS);
                }
            }
        }
    }
    p->unit->lastTick = prev;
}

int __stdcall PathLockStamp(Unit* unit)
{
    UnitPassMapView* p = (UnitPassMapView*)unit;
    if (p->unit == 0)
        return 0;
    return (int)p->unit->lastTick;
}

void __stdcall LoadPathLockStamp(Unit* unit, int stamp)
{
    ((UnitPassMapView*)unit)->unit->lastTick = (unsigned int)stamp;
}

// FUNCTION: 0x440be0
void PassMap::RefreshUnitIfStale(UnitPassMapView* p)
{
    if (p->unit->lastTick < lastTick) {
        Refresh(p->a, p->b);
        PUBLISH_PASS_MAP_CHANGE(this, p->a, p->b, PATH_CHANGE_UNITS);
    }
}
