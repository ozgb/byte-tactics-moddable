#include "win_types.h"
#include "../state/block.h"
#include <string.h>
#include <stdio.h>

#include "../util/tdf.h"
#include "../../interfaces/ref.h"

void __cdecl GameFreeThunk(void* p);

void __cdecl operator delete(void* p);

struct MovementClass;
unsigned int __stdcall GetPassMapFootprintValueWithEdgeStrips(MovementClass* obj, int x, int y);

struct Source_00440340 {
    char unknown_0[4];
    TdfRecord* tdf;
};

#include "../util/vec3.h"

#pragma pack(push, 2)
struct Unit_00440af0 {
    char unknown_0[0x26];
    unsigned int lastTick;
};

struct Object_00440af0 {
    Ref<Unit_00440af0> unit;
    char unknown_4[0x76 - 0x4];
    Point16 a;
    char unknown_7a[0x7e - 0x7a];
    Point16 b;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Record_00440af0 {
    Ref<Unit_00440af0> unit;
    char unknown_4[0x76 - 0x4];
    Point16 a;
    char unknown_7a[0x7e - 0x7a];
    Point16 b;
    char unknown_82[0x110 - 0x82];
    unsigned int flags;
    char unknown_114[0x118 - 0x114];
};

#include "../map/cell.h"
#pragma pack(pop)

#include "movement_class.h"

class Class_00440500 {
public:
    int* name;
    short footprintX;
    short footprintZ;
    short maxWaterDepth;
    short minWaterDepth;
    unsigned char maxSlope;
    unsigned char badSlope;
    unsigned char maxWaterSlope;
    unsigned char badWaterSlope;
    unsigned int width;
    unsigned int height;
    Ref<unsigned int> cells;
    int lastTick;

    void BuildPassMap();
};

int __stdcall GetPassMapCellValue(Class_00440500* obj, Cell* cell);

#pragma pack(push, 2)
struct Struct_00440a70 {
    Ref<Unit_00440af0> unit;
    char unknown_4[0x76 - 0x4];
    Point16 a;
    char unknown_7a[0x7e - 0x7a];
    Point16 b;
};
#pragma pack(pop)

#include "../network/player.h"
#include "../../interfaces/seams.h"

void AbortProgram(void);

#include "../map/mission.h"

#pragma pack(push, 1)
#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

struct Rect_00440ca0 {
    int a;
    int b;
    int c;
    int d;
};

#pragma pack(push, 1)
struct Dst_00440ca0 {
    char unknown_0[0x99];
    Rect_00440ca0 rect;
};
#pragma pack(pop)

struct Src_00440ca0 {
    int unknown_0;
    Rect_00440ca0 rect;
};

extern Game* g_game;

// FUNCTION: 0x4402e0
MovementClass::MovementClass()
{
    name = 0;
    footprintX = 0;
    footprintZ = 0;
    maxWaterDepth = 10000;
    minWaterDepth = -10000;
    maxSlope = 0xff;
    maxWaterSlope = 0xff;
    badSlope = 0xff;
    badWaterSlope = 0xff;
    width = 0;
    height = 0;
    cells = 0;
    lastTick = 0;
}

// FUNCTION: 0x440320
MovementClass::~MovementClass()
{
    GameFreeThunk(name);
    operator delete(cells);
}

// FUNCTION: 0x440340
void MovementClass::ReadMoveInfo(Source_00440340* src)
{
    footprintX = src->tdf->GetFieldInt("FootPrintX", 0);
    footprintZ = src->tdf->GetFieldInt("FootPrintZ", 0);
    maxWaterDepth = src->tdf->GetFieldInt("maxwaterdepth", maxWaterDepth);
    minWaterDepth = src->tdf->GetFieldInt("minwaterdepth", minWaterDepth);
    maxSlope = src->tdf->GetFieldInt("maxslope", maxSlope);
    badSlope = src->tdf->GetFieldInt("badslope", maxSlope / 2);
    maxWaterSlope = src->tdf->GetFieldInt("maxwaterslope", maxWaterSlope);
    badWaterSlope = src->tdf->GetFieldInt("badwaterslope", maxWaterSlope / 2);
    if (maxSlope > maxWaterSlope)
        maxSlope = maxWaterSlope;
    if (badSlope > maxSlope)
        badSlope = maxSlope;
    if (badWaterSlope > maxWaterSlope)
        badWaterSlope = maxWaterSlope;
}

// FUNCTION: 0x440420
MovementClass* __stdcall FindMovementClass(char* name)
{
    for (int i = 0; i < 32; i++) {
        if (g_movementClasses.entries[i].name != 0
            && _strcmpi((char*)g_movementClasses.entries[i].name, name) == 0) {
            return &g_movementClasses.entries[i];
        }
    }
    return 0;
}

// FUNCTION: 0x440470
void MovementClass::ResizePassMap(unsigned int w, unsigned int h)
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
void MovementClass::SetPassMapCell(int x, int y, int value)
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
void Class_00440500::BuildPassMap()
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
void MovementClass::RefreshPassMap(Point16 a, Point16 b)
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

// FUNCTION: 0x440930
void NopAfterFeatureEnum(void)
{
}

// FUNCTION: 0x440940
void BuildAllPassMaps(void)
{
    int count = 0;
    unsigned int n;
    int p = (int)&g_movementClasses;
    do {
        if (*(int*)p != 0) {
            count++;
        }
        p += 0x20;
    } while (p < (int)&g_movementClasses + (int)sizeof(MovementClassTable));

    int progress = 100;
    unsigned int* q = (unsigned int*)DAT_00512370;
    do {
        if (q[-6] != 0) {
            unsigned int h = (unsigned int)g_game->height;
            unsigned int w = (unsigned int)g_game->width;
            q[-2] = w;
            q[-1] = h;
            n = ((unsigned)(q[-1] + 0xf) >> 4) * q[-2];
            Ref<unsigned int>* slot = (Ref<unsigned int>*)q;
            operator delete((void*)slot->get());
            if (n != 0) {
                *slot = (unsigned int*)SimAlloc(n * 4);
            } else {
                *slot = (unsigned int*)0;
            }
            ((Class_00440500*)(q - 6))->BuildPassMap();
            progress += 100;
            g_game->progress_38d73 = progress / count;
        }
        q += 8;
    } while ((int)q < (int)DAT_00512370 + (int)sizeof(MovementClassTable));

    g_game->progress_38d73 = 100;
}

static inline void FreeA(int p)
{
    GameFreeThunk(*(void**)(p - 0x18));
    *(void**)(p - 0x18) = 0;
}

static inline void FreeC(int p)
{
    void* c = ((Ref<unsigned int>*)p)->get();
    *(void**)(p - 8) = 0;
    *(void**)(p - 4) = 0;
    operator delete(c);
    *(void**)p = 0;
}

// FUNCTION: 0x440a00
void FreeMovementClasses(void)
{
    int p = (int)DAT_00512370;
    do {
        FreeA(p);
        FreeC(p);
        p += 0x20;
    } while (p < (int)DAT_00512370 + (int)sizeof(MovementClassTable));
}

// FUNCTION: 0x440a40
void __stdcall RefreshAllPassMaps(Point16 a, Point16 b)
{
    for (int i = 0; i < 32; i++) {
        if (g_movementClasses.entries[i].name != 0) {
            ((MovementClass*)&g_movementClasses.entries[i])->RefreshPassMap(a, b);
        }
    }
}

static inline void UpdateAll(Point16 a, Point16 b)
{
    for (int i = 0; i < 32; i++) {
        if (g_movementClasses.entries[i].name != 0) {
            ((MovementClass*)&g_movementClasses.entries[i])->RefreshPassMap(a, b);
        }
    }
}

// FUNCTION: 0x440a70
void __stdcall RefreshPassMapsForUnit(Struct_00440a70* p)
{
    if (p->unit != 0) {
        unsigned int last = p->unit->lastTick;
        p->unit->lastTick = (unsigned int)g_game->ticks;
        for (int i = 0; i < 32; i++) {
            if (last < g_movementClasses.entries[i].lastTick) {
                ((MovementClass*)&g_movementClasses.entries[i])->RefreshPassMap(p->a, p->b);
            }
        }
        return;
    }
    UpdateAll(p->a, p->b);
}

// FUNCTION: 0x440af0
void MovementClass::RefreshMovedUnits(Object_00440af0* p)
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
        g_seams.map->RefreshPassMap(this, 0, p->a, p->b);
    }
    if (start != old) {
        for (Record_00440af0* r = &((Record_00440af0 *)g_game->units.get())[1]; r <= (Record_00440af0 *)g_game->unitsEnd.get(); r++) {
            if ((r->flags & 0x10000000) != 0 && r->unit != 0) {
                unsigned int t = r->unit->lastTick;
                if (t >= old && t < start) {
                    g_seams.map->RefreshPassMap(this, 0, r->a, r->b);
                }
            }
        }
    }
    p->unit->lastTick = prev;
}

// FUNCTION: 0x440be0
void MovementClass::RefreshUnitIfStale(Object_00440af0* p)
{
    if (p->unit->lastTick < lastTick) {
        g_seams.map->RefreshPassMap(this, 0, p->a, p->b);
    }
}

#include "../network/player_info.h"

// FUNCTION: 0x440c10
int FindUnusedLogo()
{
    int used[10];
    memset(used, 0, sizeof(used));
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active && (p->type == 1 || p->type == 2 || p->type == 3) && p->index != 10)
            used[p->info->color < 9 ? p->info->color : 9] = 1;
    }
    int result = 0;
    for (int j = 0; j < 10; j++) {
        if (!used[j]) {
            result = j;
            break;
        }
    }
    return result;
}

// FUNCTION: 0x440ca0
void __stdcall CopyPathLockFields(Dst_00440ca0* dst, Src_00440ca0* src)
{
    dst->rect = src->rect;
}

unsigned char FindHostSlot();

// FUNCTION: 0x440cd0
int CheckMapCrc()
{
    if (!g_seams.map->GetTerrainLength(g_game->net)) {
        return 0;
    }
    unsigned char me = FindHostSlot();
    PlayerInfo* data = 0;
    int check = 0;
    if (me != 10) {
        data = g_game->players[me].info;
        if (data->versionMajor >= 2 || (data->versionMajor == 1 && data->versionMinor >= 2)) {
            check = 1;
        }
    }
    if (!check) {
        return 1;
    }
    return g_seams.map->ComputeMapChecksum(g_game->net) == data->mapCrc;
}
