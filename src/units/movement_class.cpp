#include "win_types.h"
#include "../state/block.h"
#include <string.h>
#include <stdio.h>

#include "../util/tdf.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/map.h"

#include "../graphics/object_pool.h"

struct TdfParserView {
    char unknown_0[4];
    TdfRecord* tdf;
};

#include "movement_class.h"

#include "../../interfaces/services.h"

#pragma pack(push, 1)
#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

struct PathLockRect {
    int a;
    int b;
    int c;
    int d;
};

#pragma pack(push, 1)
struct PathLockDstView {
    char unknown_0[0x99];
    PathLockRect rect;
};
#pragma pack(pop)

struct PathLockSrcView {
    int unknown_0;
    PathLockRect rect;
};

#include "../../link/globals.h"

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
}

// FUNCTION: 0x440320
MovementClass::~MovementClass()
{
    GameFreeThunk(name);
}

// FUNCTION: 0x440340
void MovementClass::ReadMoveInfo(TdfParserView* src)
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

// FUNCTION: 0x440930
void NopAfterFeatureEnum(void)
{
}

// FUNCTION: 0x440a00
void FreeMovementClasses(void)
{
    for (int i = 0; i < 32; i++) {
        GameFreeThunk(g_movementClasses.entries[i].name);
        g_movementClasses.entries[i].name = nullptr;
    }
}

int __stdcall GetMovementLimits(MapMoveLimits* out, int capacity)
{
    int count = 0;
    for (int i = 0; i < 32 && count < capacity; i++) {
        const MovementClass* c = &g_movementClasses.entries[i];
        if (c->name == nullptr)
            continue;
        MapMoveLimits* l = &out[count++];
        l->index = i;
        l->footprintX = c->footprintX;
        l->footprintZ = c->footprintZ;
        l->maxWaterDepth = c->maxWaterDepth;
        l->minWaterDepth = c->minWaterDepth;
        l->maxSlope = c->maxSlope;
        l->badSlope = c->badSlope;
        l->maxWaterSlope = c->maxWaterSlope;
        l->badWaterSlope = c->badWaterSlope;
    }
    return count;
}

#include "../network/player_info.h"

// FUNCTION: 0x440ca0
void __stdcall CopyPathLockFields(PathLockDstView* dst, PathLockSrcView* src)
{
    dst->rect = src->rect;
}

