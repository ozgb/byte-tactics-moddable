#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../files/files_api.h"

#pragma pack(push, 1)

struct Obj_00401320 {
    float x0;
    float x1;
    float x2;
    float x3;
    float prev0;
    float prev1;
};


struct UnitInfo {
    char unknown_0[0xa8];
    unsigned short id;
};

#include "unit_resources.h"

#include "../network/player.h"

#include "../units/unit_def.h"

#include "../units/unit.h"

#include "game.h"
#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;

class Obj {};
typedef void (Obj::*ThisFn)();

// FUNCTION: 0x401000 ??_H@YGXPAXIHP6EX0@Z@Z
struct Elem_00401000 {
    Elem_00401000();
    ~Elem_00401000();
    int field_0;
};

struct Holder_00401000 {
    Elem_00401000 items[4];
};

Holder_00401000 g_holder_00401000;

// FUNCTION: 0x401030
void __stdcall CallOnEachReverse(Obj* base, int stride, int count, ThisFn func)
{
    base = (Obj*)((char*)base + count * stride);
    while (--count >= 0) {
        base = (Obj*)((char*)base - stride);
        (base->*func)();
    }
}

// FUNCTION: 0x401070
void UnitResources::Reset(unsigned char playerIndex)
{
    memset(this, 0, sizeof(*this));
    player = &g_game->players[playerIndex];
}

// FUNCTION: 0x4010b0
void UnitResources::SaveUnitAccounts(UnitInfo* info, HapiBank* file)
{
    char name[32];
    sprintf(name, "u%04xacc", info->id);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&energyMake, 0x18);
    file->WriteBox(&metalMake, 0x18);
}

// FUNCTION: 0x401110
void UnitResources::LoadUnitAccounts(UnitInfo* info, HapiBank* file)
{
    char name[32];
    sprintf(name, "u%04xacc", info->id);
    if (file->OpenNamedBox(name)) {
        file->SeekBox(0);
        file->ReadBox(&energyMake, 0x18);
        file->ReadBox(&metalMake, 0x18);
    }
}

// FUNCTION: 0x401180
int UnitResources::RequestEnergy(UnitResources* r, float amount)
{
    r->energyUse += amount;
    if (r->energyStall > 0.0f)
        return 0;
    r->energyUsePaid += amount;
    return 1;
}

// FUNCTION: 0x4011c0
int UnitResources::RequestEnergyAndMetal(float dx, float dy)
{
    energyUse += dx;
    metalUse += dy;
    if (energyStall <= 0.0f && metalStall <= 0.0f) {
        energyUsePaid += dx;
        metalUsePaid += dy;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x401220
int UnitResources::SpendEnergy(float amount)
{
    if (player->energy >= amount) {
        player->energy -= amount;
        energyUse += amount;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x401260
int UnitResources::SpendMetal(float amount)
{
    if (player->metal >= amount) {
        player->metal -= amount;
        metalUse += amount;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4012a0
int UnitResources::SpendEnergyAndMetal(float energy, float metal)
{
    if (player->energy >= energy && player->metal >= metal) {
        player->energy -= energy;
        energyUse += energy;
        if (player->metal >= metal) {
            player->metal -= metal;
            metalUse += metal;
        }
        return 1;
    }
    return 0;
}

// FUNCTION: 0x401320
void __stdcall SettleResourceAccount(Obj_00401320* p, float a, float b)
{
    p->prev1 = p->x1;
    p->prev0 = p->x0;
    p->x3 = p->x3 - a * p->x3 + (p->x2 - b * p->x2);
    p->x1 = 0;
    p->x0 = 0;
    p->x2 = 0;
}
