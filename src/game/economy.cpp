#include <float.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../files/files_api.h"
#include "../../interfaces/seams.h"

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

struct ResourceAccount {
    float produced;
    float used;
    float demand;
    float backlog;
    float lastProduced;
    float lastUsed;
};

static_assert(sizeof(ResourceAccount) * 2 == offsetof(UnitResources, player), "ResourceAccount");

static inline ResourceAccount* EnergyAccount(UnitResources* r)
{
    return reinterpret_cast<ResourceAccount*>(&r->energyMake);
}

static inline ResourceAccount* MetalAccount(UnitResources* r)
{
    return reinterpret_cast<ResourceAccount*>(&r->metalMake);
}

void __stdcall SettleResourceAccount(ResourceAccount* r, float ratioBacklog, float ratioDemand)
{
    r->lastUsed = r->used;
    r->lastProduced = r->produced;
    r->used = 0;
    r->produced = 0;
#ifdef __clang__
    r->backlog = (double)r->backlog - (double)ratioBacklog * r->backlog + ((double)r->demand - (double)ratioDemand * r->demand);
#else
    r->backlog = r->backlog - ratioBacklog * r->backlog;
    r->backlog += r->demand - ratioDemand * r->demand;
#endif
    r->demand = 0;
}

static inline void AddIncome(Unit* u, float* dst, float v)
{
    Player* o = u->resourceSlot.player;
    if (o->active && o->type == 2) {
        switch (g_game->difficulty) {
        case 0:
            *dst += v * 0.5;
            return;
        case 1:
            *dst += v * 0.7;
            return;
        default:
            *dst += v;
            return;
        }
    }
    *dst += v;
}

static inline void AddIncomeD(Unit* u, float* dst, double v)
{
    Player* o = u->resourceSlot.player;
    if (o->active && o->type == 2) {
        switch (g_game->difficulty) {
        case 0:
            *dst += v * 0.5;
            return;
        case 1:
            *dst += v * 0.7;
            return;
        default:
            *dst += v;
            return;
        }
    }
    *dst += v;
}

static inline void AddIncomeDB(Unit* u, float* dst, double v)
{
    Player* o = u->resourceSlot.player;
    if (o->active && o->type == 2) {
        switch (g_game->difficulty) {
        case 0:
            *dst += v * 0.5;
            break;
        case 1:
            *dst += v * 0.7;
            break;
        default:
            *dst += v;
            break;
        }
    } else
        *dst += v;
}

#ifdef __clang__
#define WIND_ADD(v) (v)
#else
#define WIND_ADD(v) ((float)(v))
#endif
static inline void AddIncomeW(Unit* u, float* dst, double v)
{
    Player* o = u->resourceSlot.player;
    if (o->active && o->type == 2) {
        switch (g_game->difficulty) {
        case 0:
            *dst += v * 0.5;
            return;
        case 1:
            *dst += v * 0.7;
            return;
        default:
            *dst += WIND_ADD(v);
            return;
        }
    }
    *dst += WIND_ADD(v);
}

#undef WIND_ADD

static int UseEnergy(Unit* u, float v)
{
    if (v >= 0)
        return u->resourceSlot.RequestEnergy(&u->resourceSlot, v);
    AddIncome(u, &u->resourceSlot.energyMake, -v);
    return 0;
}

static int UseEnergyD(Unit* u, float v)
{
    if (v >= 0) {
        double a = v;
        float* used = &u->resourceSlot.energyUse;
        *used += v;
        if (u->resourceSlot.energyStall > 0.0f)
            return 0;
        u->resourceSlot.energyUsePaid += (float)a;
        return 1;
    }
    AddIncomeDB(u, &u->resourceSlot.energyMake, -v);
    return 0;
}

// FUNCTION: 0x401360
void __stdcall UpdatePlayerEconomy(Player* p)
{
    float usedA[2];
    float backlogA[2];
    float demandA[2];
    float producedA[2];
    float ratioA[2];
    float avail[2];
    float demandRatio[2];
    Unit* u;
    int i;

    p->metalCapacity = 0;
    p->energyCapacity = 0;
    producedA[0] = 0;
    producedA[1] = 0;
    usedA[0] = 0;
    usedA[1] = 0;
    demandA[0] = 0;
    demandA[1] = 0;
    backlogA[0] = 0;
    backlogA[1] = 0;
    for (u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if (!(u->flags & 0x10000000))
            continue;
        if (u->flags & 0x20000000) {
            if (u->activateFlags & 1) {
                int ok = UseEnergy(u, u->def->energyuse);
                if (u->def->extractsMetal > 0) {
                    if (ok)
                        AddIncome(u, &u->resourceSlot.metalMake, *(float*)&u->extraction);
                } else if (u->def->makesMetal) {
                    if (ok)
                        AddIncome(u, &u->resourceSlot.metalMake, u->def->makesMetal);
                } else if (u->def->windgenerator > 0) {
                    AddIncomeW(u, &u->resourceSlot.energyMake, (double)g_game->windStrength * u->def->windgenerator);
                } else if (u->def->tidalgenerator > 0) {
                    AddIncomeDB(u, &u->resourceSlot.energyMake, g_game->tidal * u->def->tidalgenerator);
                }
            }
        } else if ((u->activateFlags & 1) || (u->flags & 0xc) > 0) {
            UseEnergyD(u, u->def->energyuse);
        }
        if (u->buildLeft == 0) {
            AddIncomeD(u, &u->resourceSlot.energyMake, u->def->energymake);
            AddIncome(u, &u->resourceSlot.metalMake, u->def->metalmake);
            p->metalCapacity += u->def->metalstorage;
            p->energyCapacity += u->def->energystorage;
        }
        if (!(p->active && p->type == 3)) {
            if (u->cloakOn) {
                if (!(u->flags & 0x1000) && u->workTime <= ((unsigned int)g_game->ticks)) {
                    int cost = (int)((u->flags & 0xc) > 0 ? u->def->cloakcostmoving : u->def->cloakcost);
                    g_seams.units->SetStateBits((Unit*)u, 4, u->resourceSlot.SpendEnergy(cost));
                } else
                    g_seams.units->SetStateBits((Unit*)u, 4, 0);
            } else
                g_seams.units->SetStateBits((Unit*)u, 4, 0);
        }
        producedA[0] += u->resourceSlot.energyMake;
        usedA[0] += u->resourceSlot.energyUse;
        demandA[0] += u->resourceSlot.energyUsePaid;
        backlogA[0] += u->resourceSlot.energyStall;
        producedA[1] += u->resourceSlot.metalMake;
        usedA[1] += u->resourceSlot.metalUse;
        demandA[1] += u->resourceSlot.metalUsePaid;
        backlogA[1] += u->resourceSlot.metalStall;
#ifdef __clang__
        {
            volatile float roundedSums[8] = {producedA[0], producedA[1], usedA[0], usedA[1],
                                             demandA[0], demandA[1], backlogA[0], backlogA[1]};
            producedA[0] = roundedSums[0];
            producedA[1] = roundedSums[1];
            usedA[0] = roundedSums[2];
            usedA[1] = roundedSums[3];
            demandA[0] = roundedSums[4];
            demandA[1] = roundedSums[5];
            backlogA[0] = roundedSums[6];
            backlogA[1] = roundedSums[7];
        }
#endif
    }
    UnitResources* e = p->econ.get();
#ifdef __clang__
    double energyProduced = (double)producedA[0] + e->energyMake;
    producedA[0] = energyProduced;
#else
    producedA[0] += e->energyMake;
#endif
    usedA[0] += e->energyUse;
    demandA[0] += e->energyUsePaid;
    backlogA[0] += e->energyStall;
#ifdef __clang__
    double metalProduced = (double)producedA[1] + e->metalMake;
    producedA[1] = metalProduced;
#else
    producedA[1] += e->metalMake;
#endif
    usedA[1] += e->metalUse;
    demandA[1] += e->metalUsePaid;
    backlogA[1] += e->metalStall;
#ifdef __clang__
    {
        volatile float roundedUsed[2] = {usedA[0], usedA[1]};
        volatile float roundedDemand[2] = {demandA[0], demandA[1]};
        volatile float roundedBacklog[2] = {backlogA[0], backlogA[1]};
        for (i = 0; i < 2; i++) {
            usedA[i] = roundedUsed[i];
            demandA[i] = roundedDemand[i];
            backlogA[i] = roundedBacklog[i];
        }
    }
#endif
    if (p->flags & 1) {
        p->energyCapacity += p->energyStorageBonus;
        p->metalCapacity += p->metalStorageBonus;
    }
    p->energyIncome = producedA[0];
    p->energyUsage = usedA[0];
#ifdef __clang__
    p->totalEnergyProduced += energyProduced;
#else
    p->totalEnergyProduced += producedA[0];
#endif
    p->totalEnergyConsumed += usedA[0];
    p->metalIncome = producedA[1];
    p->metalUsage = usedA[1];
    p->totalMetalProduced += producedA[1];
    p->totalMetalConsumed += usedA[1];
#ifdef __clang__
    avail[0] = energyProduced + p->energy;
#else
    avail[0] = producedA[0] + p->energy;
#endif
#ifdef __clang__
    avail[1] = metalProduced + p->metal;
#else
    avail[1] = producedA[1] + p->metal;
#endif
#ifdef __clang__
    for (i = 0; i < 2; i++) {
        double take;
        if (backlogA[i] <= avail[i]) {
            take = backlogA[i];
            ratioA[i] = 1.0f;
        } else {
            take = avail[i];
            ratioA[i] = (double)avail[i] / backlogA[i];
        }
        double left = (double)avail[i] - take;
        if (demandA[i] <= left) {
            take = demandA[i];
            demandRatio[i] = 1.0f;
        } else {
            take = left;
            demandRatio[i] = left / demandA[i];
        }
        avail[i] = left - take;
    }
#else
    for (i = 0; i < 2; i++) {
        float take;
        if (backlogA[i] <= avail[i]) {
            take = backlogA[i];
            ratioA[i] = 1.0f;
        } else {
            take = avail[i];
            ratioA[i] = avail[i] / backlogA[i];
        }
        avail[i] -= take;
        float left = avail[i];
        if (demandA[i] <= left) {
            take = demandA[i];
            demandRatio[i] = 1.0f;
        } else {
            take = left;
            demandRatio[i] = left / demandA[i];
        }
        avail[i] = left - take;
    }
#endif
#ifdef __clang__
    volatile float roundedAvail[2] = {avail[0], avail[1]};
    avail[0] = roundedAvail[0];
    avail[1] = roundedAvail[1];
    volatile float roundedRatio[2] = {ratioA[0], ratioA[1]};
    volatile float roundedDemandRatio[2] = {demandRatio[0], demandRatio[1]};
    for (i = 0; i < 2; i++) {
        ratioA[i] = roundedRatio[i];
        demandRatio[i] = roundedDemandRatio[i];
    }
#endif
    p->energy = avail[0];
    if (avail[0] > p->energyCapacity) {
        p->energy = p->energyCapacity;
        p->energyWasted += (double)avail[0] - p->energyCapacity;
    }
    p->metal = avail[1];
    if (avail[1] > p->metalCapacity) {
        p->metal = p->metalCapacity;
        p->metalWasted += (double)avail[1] - p->metalCapacity;
    }
    for (u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if (u->flags & 0x10000000) {
            SettleResourceAccount(EnergyAccount(&u->resourceSlot), ratioA[0], demandRatio[0]);
            SettleResourceAccount(MetalAccount(&u->resourceSlot), ratioA[1], demandRatio[1]);
        }
    }
    SettleResourceAccount(EnergyAccount(p->econ.get()), ratioA[0], demandRatio[0]);
    SettleResourceAccount(MetalAccount(p->econ.get()), ratioA[1], demandRatio[1]);
}
