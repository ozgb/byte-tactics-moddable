#include <float.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../files/files_api.h"
#include "../../interfaces/seams.h"
#include "economy.h"
#include "../state/block.h"
#include "../network/resource_share.h"

#pragma pack(push, 1)

#include "unit_resources.h"

#include "../network/player.h"

#include "../units/unit_def.h"

#include "../units/unit.h"

#include "game.h"
#include "../util/x87_compare.h"
#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;

class ArrayElement {};
typedef void (ArrayElement::*ThisFn)();

// FUNCTION: 0x401000 ??_H@YGXPAXIHP6EX0@Z@Z
struct ConstructedElement {
    ConstructedElement();
    ~ConstructedElement();
    int field_0;
};

struct ConstructedElementArray {
    ConstructedElement items[4];
};

ConstructedElementArray g_holder_00401000;

// FUNCTION: 0x401030
void __stdcall CallOnEachReverse(ArrayElement* base, int stride, int count, ThisFn func)
{
    base = (ArrayElement*)((char*)base + count * stride);
    while (--count >= 0) {
        base = (ArrayElement*)((char*)base - stride);
        (base->*func)();
    }
}

static_assert(sizeof(EconLedger) == 24, "EconLedger");
static_assert(sizeof(EconAccount) == 48, "EconAccount");
static_assert(offsetof(UnitResources, player) == sizeof(EconAccount), "EconAccount");
static_assert(offsetof(UnitResources, energyMake) == offsetof(EconAccount, energy), "EconAccount");
static_assert(offsetof(UnitResources, metalMake) == offsetof(EconAccount, metal), "EconAccount");

static const EconomyWorldView* s_world;
static EconAccount* s_region;

enum { HANDICAP_NONE = 0, HANDICAP_HALF = 1, HANDICAP_SEVEN = 2 };

static Unit* UnitOfHandle(EconUnit handle)
{
    Ref<Unit> ref;
    ref.offset = handle;
    return ref.get();
}

static EconAccount* UnitAccountOf(EconUnit unit)
{
    return &s_region[s_world->UnitSlot(unit)];
}

static EconAccount* PlayerAccountOf(int player)
{
    return reinterpret_cast<EconAccount*>(g_game->players[player].econ.get());
}

static EconAccount* AccountOf(int player, EconUnit unit)
{
    return unit != 0 ? UnitAccountOf(unit) : PlayerAccountOf(player);
}

static int HandicapOf(int player)
{
    EconPlayerInfo info;
    s_world->PlayerInfo(player, &info);
    if (info.active != 0 && info.controller == 2) {
        switch (s_world->Difficulty()) {
        case 0:
            return HANDICAP_HALF;
        case 1:
            return HANDICAP_SEVEN;
        }
    }
    return HANDICAP_NONE;
}

template <class V>
static inline void AddIncome(float* dst, int handicap, V v)
{
    switch (handicap) {
    case HANDICAP_HALF:
        *dst += v * 0.5;
        return;
    case HANDICAP_SEVEN:
        *dst += v * 0.7;
        return;
    default:
        *dst += v;
        return;
    }
}

#ifdef __clang__
#define WIND_ADD(v) (v)
#else
#define WIND_ADD(v) ((float)(v))
#endif
static inline void AddIncomeW(float* dst, int handicap, double v)
{
    switch (handicap) {
    case HANDICAP_HALF:
        *dst += v * 0.5;
        return;
    case HANDICAP_SEVEN:
        *dst += v * 0.7;
        return;
    default:
        *dst += WIND_ADD(v);
        return;
    }
}

#undef WIND_ADD

static void EmitEffect(int kind, int player, EconUnit unit, unsigned int resources, float energy, float metal, int value)
{
    EconomyEvent effect;
    memset(&effect, 0, sizeof(effect));
    effect.kind = kind;
    effect.player = player;
    effect.unit = unit;
    effect.resources = resources;
    effect.energy = energy;
    effect.metal = metal;
    effect.value = value;
    s_world->Emit(&effect);
}

unsigned int __stdcall EconomyRegionSize(const EconMapInfo* map)
{
    return (unsigned int)map->unitSlots * sizeof(EconAccount);
}

void __stdcall EconomyInit(const EconMapInfo*, const EconomyWorldView* world, void* region, unsigned int)
{
    s_world = world;
    s_region = static_cast<EconAccount*>(region);
    SimBlockTrack(reinterpret_cast<void**>(&s_region));
}

void* __stdcall EconomyShutdown()
{
    void* region = s_region;
    s_region = 0;
    return region;
}

// FUNCTION: 0x401070
static void ResetAccount(int player, EconUnit unit)
{
    if (unit == 0) {
        UnitResources* r = g_game->players[player].econ.get();
        memset(r, 0, sizeof(*r));
        r->player = &g_game->players[player];
        return;
    }
    memset(UnitAccountOf(unit), 0, sizeof(EconAccount));
}

// FUNCTION: 0x401180
static int RequestEnergyOn(EconAccount* a, float amount)
{
    a->energy.use += amount;
    if (a->energy.stall > 0.0f)
        return 0;
    a->energy.usePaid += amount;
    return 1;
}

static int RequestMetalOn(EconAccount* a, float amount)
{
    a->metal.use += amount;
    if (a->metal.stall > 0.0f)
        return 0;
    a->metal.usePaid += amount;
    return 1;
}

// FUNCTION: 0x4011c0
static int RequestEnergyAndMetalOn(EconAccount* a, float dx, float dy)
{
    a->energy.use += dx;
    a->metal.use += dy;
    if (StallsAllowRequest(a->energy.stall, a->metal.stall)) {
        a->energy.usePaid += dx;
        a->metal.usePaid += dy;
        return 1;
    }
    return 0;
}

static int RequestOn(EconAccount* a, unsigned int resources, float energy, float metal)
{
    if ((resources & (ECON_ENERGY | ECON_METAL)) == (ECON_ENERGY | ECON_METAL))
        return RequestEnergyAndMetalOn(a, energy, metal);
    if (resources & ECON_ENERGY)
        return RequestEnergyOn(a, energy);
    return RequestMetalOn(a, metal);
}

// FUNCTION: 0x401220
static int SpendEnergyOn(EconAccount* a, Player* pl, float amount)
{
    if (pl->energy >= amount) {
        pl->energy -= amount;
        a->energy.use += amount;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x401260
static int SpendMetalOn(EconAccount* a, Player* pl, float amount)
{
    if (pl->metal >= amount) {
        pl->metal -= amount;
        a->metal.use += amount;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4012a0
static int SpendEnergyAndMetalOn(EconAccount* a, Player* pl, float energy, float metal)
{
    if (pl->energy >= energy && pl->metal >= metal) {
        pl->energy -= energy;
        a->energy.use += energy;
        if (pl->metal >= metal) {
            pl->metal -= metal;
            a->metal.use += metal;
        }
        return 1;
    }
    return 0;
}

static int SpendOn(EconAccount* a, Player* pl, unsigned int resources, float energy, float metal)
{
    if ((resources & (ECON_ENERGY | ECON_METAL)) == (ECON_ENERGY | ECON_METAL))
        return SpendEnergyAndMetalOn(a, pl, energy, metal);
    if (resources & ECON_ENERGY)
        return SpendEnergyOn(a, pl, energy);
    return SpendMetalOn(a, pl, metal);
}

int __stdcall EconomyApply(const EconomyEvent* event)
{
    switch (event->kind) {
    case ECONOMY_RESET:
        ResetAccount(event->player, event->unit);
        return 1;
    case ECONOMY_LOAD:
        *UnitAccountOf(event->unit) = *event->account;
        return 1;
    case ECONOMY_REQUEST:
        return RequestOn(AccountOf(event->player, event->unit), event->resources, event->energy, event->metal);
    case ECONOMY_SPEND:
        return SpendOn(AccountOf(event->player, event->unit), &g_game->players[event->player], event->resources, event->energy, event->metal);
    case ECONOMY_INCOME: {
        EconAccount* a = AccountOf(event->player, event->unit);
        int handicap = HandicapOf(event->player);
        if (event->resources & ECON_ENERGY)
            AddIncome(&a->energy.make, handicap, event->energy);
        if (event->resources & ECON_METAL)
            AddIncome(&a->metal.make, handicap, event->metal);
        return 1;
    }
    }
    return 0;
}

int __stdcall EconomyRequestGranted(EconUnit unit, unsigned int resources)
{
    EconAccount* a = UnitAccountOf(unit);
    if ((resources & (ECON_ENERGY | ECON_METAL)) == (ECON_ENERGY | ECON_METAL))
        return StallsAllowRequest(a->energy.stall, a->metal.stall) ? 1 : 0;
    if (resources & ECON_ENERGY)
        return a->energy.stall > 0.0f ? 0 : 1;
    return a->metal.stall > 0.0f ? 0 : 1;
}

void __stdcall EconomyUnitAccount(EconUnit unit, EconAccount* out)
{
    *out = *UnitAccountOf(unit);
}

// FUNCTION: 0x401320
static void __stdcall SettleResourceAccount(EconLedger* r, float ratioBacklog, float ratioDemand)
{
    r->usePrev = r->use;
    r->makePrev = r->make;
    r->use = 0;
    r->make = 0;
#ifdef __clang__
    r->stall = (double)r->stall - (double)ratioBacklog * r->stall + ((double)r->usePaid - (double)ratioDemand * r->usePaid);
#else
    r->stall = r->stall - ratioBacklog * r->stall;
    r->stall += r->usePaid - ratioDemand * r->usePaid;
#endif
    r->usePaid = 0;
}

static int UseEnergy(EconAccount* a, int handicap, float v)
{
    if (v >= 0)
        return RequestEnergyOn(a, v);
    AddIncome(&a->energy.make, handicap, -v);
    return 0;
}

static int UseEnergyD(EconAccount* a, int handicap, float v)
{
    if (v >= 0) {
        double d = v;
        float* used = &a->energy.use;
        *used += v;
        if (a->energy.stall > 0.0f)
            return 0;
        a->energy.usePaid += (float)d;
        return 1;
    }
    AddIncome(&a->energy.make, handicap, (double)-v);
    return 0;
}

// FUNCTION: 0x401360
void __stdcall EconomyUpdatePlayer(int player)
{
    float usedA[2];
    float backlogA[2];
    float demandA[2];
    float producedA[2];
    float ratioA[2];
    float avail[2];
    float demandRatio[2];
    EconUnit u;
    EconUnit last;
    int i;
    Player* p = &g_game->players[player];
    EconPlayerInfo pinfo;
    s_world->PlayerInfo(player, &pinfo);

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
    last = s_world->LastUnit(player);
    for (u = s_world->FirstUnit(player); u <= last; u = s_world->NextUnit(u)) {
        EconUnitInfo ui;
        EconTypeInfo ti;
        if (!s_world->UnitInfo(u, &ui))
            continue;
        s_world->TypeInfo(ui.type, &ti);
        EconAccount* acct = UnitAccountOf(u);
        int handicap = HandicapOf(ui.player);
        if (ui.structure) {
            if (ui.active) {
                int ok = UseEnergy(acct, handicap, ti.energyUse);
                if (ti.extractsMetal > 0) {
                    if (ok)
                        AddIncome(&acct->metal.make, handicap, ui.extraction);
                } else if (ti.makesMetal) {
                    if (ok)
                        AddIncome(&acct->metal.make, handicap, (float)ti.makesMetal);
                } else if (ti.windGenerator > 0) {
                    AddIncomeW(&acct->energy.make, handicap, (double)s_world->Wind() * ti.windGenerator);
                } else if (ti.tidalGenerator > 0) {
                    AddIncome(&acct->energy.make, handicap, (double)s_world->Tide() * ti.tidalGenerator);
                }
            }
        } else if (ui.active || ui.moving) {
            UseEnergyD(acct, handicap, ti.energyUse);
        }
        if (X87EqTest40(ui.buildLeft, 0.0f)) {
            AddIncome(&acct->energy.make, handicap, (double)ti.energyMake);
            AddIncome(&acct->metal.make, handicap, ti.metalMake);
            p->metalCapacity += ti.metalStorage;
            p->energyCapacity += ti.energyStorage;
        }
        if (!(pinfo.active && pinfo.controller == 3)) {
            if (ui.cloakOn) {
                if (!ui.cloakBlocked && ui.workTime <= s_world->Tick()) {
                    int cost = (int)(ui.moving ? ti.cloakCostMoving : ti.cloakCost);
                    Player* owner = &g_game->players[ui.player];
#ifdef __clang__
                    int paid = 0;
                    if (CloakCostPayable((float)cost, owner->energy)) {
                        owner->energy -= (float)cost;
                        acct->energy.use += (float)cost;
                        paid = 1;
                    }
                    EmitEffect(ECONOMY_EFFECT_CLOAK, ui.player, u, 0, 0, 0, paid);
#else
                    EmitEffect(ECONOMY_EFFECT_CLOAK, ui.player, u, 0, 0, 0, SpendOn(acct, owner, ECON_ENERGY, (float)cost, 0));
#endif
                } else
                    EmitEffect(ECONOMY_EFFECT_CLOAK, ui.player, u, 0, 0, 0, 0);
            } else
                EmitEffect(ECONOMY_EFFECT_CLOAK, ui.player, u, 0, 0, 0, 0);
        }
        producedA[0] += acct->energy.make;
        usedA[0] += acct->energy.use;
        demandA[0] += acct->energy.usePaid;
        backlogA[0] += acct->energy.stall;
        producedA[1] += acct->metal.make;
        usedA[1] += acct->metal.use;
        demandA[1] += acct->metal.usePaid;
        backlogA[1] += acct->metal.stall;
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
    EconAccount* e = PlayerAccountOf(player);
#ifdef __clang__
    double energyProduced = (double)producedA[0] + e->energy.make;
    producedA[0] = energyProduced;
#else
    producedA[0] += e->energy.make;
#endif
    usedA[0] += e->energy.use;
    demandA[0] += e->energy.usePaid;
    backlogA[0] += e->energy.stall;
#ifdef __clang__
    double metalProduced = (double)producedA[1] + e->metal.make;
    producedA[1] = metalProduced;
#else
    producedA[1] += e->metal.make;
#endif
    usedA[1] += e->metal.use;
    demandA[1] += e->metal.usePaid;
    backlogA[1] += e->metal.stall;
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
    if (pinfo.storage) {
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
        if (SettlementBacklogCovered(backlogA[i], avail[i])) {
            take = backlogA[i];
            ratioA[i] = 1.0f;
        } else {
            take = avail[i];
            ratioA[i] = (double)avail[i] / backlogA[i];
        }
        double left = (double)avail[i] - take;
        if (SettlementDemandCovered(demandA[i], left)) {
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
        if (SettlementBacklogCovered(backlogA[i], avail[i])) {
            take = backlogA[i];
            ratioA[i] = 1.0f;
        } else {
            take = avail[i];
            ratioA[i] = avail[i] / backlogA[i];
        }
        avail[i] -= take;
        float left = avail[i];
        if (SettlementDemandCovered(demandA[i], left)) {
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
    for (u = s_world->FirstUnit(player); u <= last; u = s_world->NextUnit(u)) {
        EconUnitInfo ui;
        if (s_world->UnitInfo(u, &ui)) {
            EconAccount* acct = UnitAccountOf(u);
            SettleResourceAccount(&acct->energy, ratioA[0], demandRatio[0]);
            SettleResourceAccount(&acct->metal, ratioA[1], demandRatio[1]);
        }
    }
    SettleResourceAccount(&PlayerAccountOf(player)->energy, ratioA[0], demandRatio[0]);
    SettleResourceAccount(&PlayerAccountOf(player)->metal, ratioA[1], demandRatio[1]);
}

union SumFixed {
    int value;
    struct {
        unsigned short fraction;
        short whole;
    } parts;
};

// FUNCTION: 0x437840
void __stdcall EconomyUpdateMetalExtraction(EconUnit unit)
{
    EconUnitInfo ui;
    EconTypeInfo ti;
    s_world->UnitInfo(unit, &ui);
    s_world->TypeInfo(ui.type, &ti);
    if (ti.extractsMetal > 0.0f) {
        SumFixed total;
        total.value = 0;
        for (int y = ui.cellY; y < ui.cellY + ui.footprintY; y++) {
            for (int x = ui.cellX; x < ui.cellX + ui.footprintX; x++) {
                int metal = s_world->CellMetal(x, y);
                if (metal >= 0) {
                    total.parts.whole += metal + 1;
                }
            }
        }
        *(float*)&UnitOfHandle(unit)->extraction = ti.extractsMetal * 1.52587890625e-05 * (float)total.value;
        if (ui.hasScript)
            EmitEffect(ECONOMY_EFFECT_EXTRACTOR, ui.player, unit, 0, 0, 0, total.parts.whole);
    }
}

static int IsRecipient(const EconPlayerInfo& info)
{
    return info.active != 0
        && (info.controller == 1 || info.controller == 2 || info.controller == 3)
        && info.index != 10
        && (info.controller == 1 || info.controller == 2 || info.controller == 3)
        && info.inGame
        && info.controller == 3
        && info.human;
}

// FUNCTION: 0x457d30
void __stdcall EconomyUpdateResourceSharing(int player)
{
    if (!s_world->SharingEnabled())
        return;

    EconPlayerInfo me;
    s_world->PlayerInfo(player, &me);
    Player* self = &g_game->players[player];

    if (s_world->Tick() % 60 == 0) {
        int found = player;
        int foundIndex = me.index;
        if (me.shareMetal && self->metal > self->shareMetal) {
            for (int i = 0; i < 10; i++) {
                EconPlayerInfo info;
                s_world->PlayerInfo(i, &info);
                Player* p = &g_game->players[i];
                if (IsRecipient(info)
                    && s_world->Allied(player, i) != 0
                    && MetalRecipientPoorer(p->metal, self->metal)) {
                    found = i;
                    foundIndex = info.index;
                }
            }
        }
        if (found != player && self->metal > self->shareMetal) {
            Player* other = &g_game->players[found];
            float amount = ShareMetalAmount(self->metal, self->shareMetal, other->metalCapacity, other->metal);
            EmitEffect(ECONOMY_EFFECT_SHARE, me.index, 0, ECON_METAL, 0, amount, foundIndex);
        }

        found = player;
        foundIndex = me.index;
        if (me.shareEnergy && self->energy > self->shareEnergy) {
            for (int i = 0; i < 10; i++) {
                EconPlayerInfo info;
                s_world->PlayerInfo(i, &info);
                Player* p = &g_game->players[i];
                if (IsRecipient(info)
                    && s_world->Allied(player, i) != 0
                    && EnergyRecipientPoorer(p->energy, self->energy)) {
                    found = i;
                    foundIndex = info.index;
                }
            }
        }
        if (found != player && self->energy > self->shareEnergy) {
            Player* other = &g_game->players[found];
            float amount = ShareEnergyAmount(self->energy, self->shareEnergy, other->energyCapacity, other->energy);
            EmitEffect(ECONOMY_EFFECT_SHARE, me.index, 0, ECON_ENERGY, amount, 0, foundIndex);
        }
    }

    if (s_world->Tick() % 450 == 0) {
        if (me.shareMapping) {
            for (int i = 0; i < 10; i++) {
                EconPlayerInfo info;
                s_world->PlayerInfo(i, &info);
                if (IsRecipient(info) && s_world->Allied(player, i) != 0) {
                    unsigned char a = (unsigned char)me.index;
                    unsigned char b = (unsigned char)info.index;
                    if (a != 10 && b != 10)
                        g_seams.network->SendShareMapInfo(a, b);
                }
            }
        }
    }
}

// FUNCTION: 0x488f30
double __stdcall EconomyGetEnergyUse(EconType type)
{
    EconTypeInfo ti;
    s_world->TypeInfo(type, &ti);
    if (ti.energyUse != 0.0) {
        return ti.energyUse;
    }
    if (ti.windGenerator > 0.0f) {
        return -((double)s_world->Wind() * (double)ti.windGenerator);
    }
    if (ti.tidalGenerator > 0.0f) {
        return -((double)s_world->Tide() * (double)ti.tidalGenerator);
    }
    return 0.0f;
}

void __stdcall EconomyWriteState(const StateWriter* w)
{
    unsigned int count = 0;
    EconUnitInfo ui;
    for (int player = 0; player < 10; player++) {
        EconUnit last = s_world->LastUnit(player);
        for (EconUnit u = s_world->FirstUnit(player); u != 0 && u <= last; u = s_world->NextUnit(u))
            if (s_world->UnitInfo(u, &ui))
                count++;
    }
    w->Begin(34, "economy");
    w->Int("count", 0, 4, count);
    unsigned int k = 0;
    for (int player = 0; player < 10; player++) {
        EconUnit last = s_world->LastUnit(player);
        for (EconUnit u = s_world->FirstUnit(player); u != 0 && u <= last; u = s_world->NextUnit(u)) {
            if (s_world->UnitInfo(u, &ui)) {
                w->Handle("unit[%u]", k, STATE_HANDLE_UNIT, u);
                w->Bytes("account[%u]", k, UnitAccountOf(u), sizeof(EconAccount));
                k++;
            }
        }
    }
}
