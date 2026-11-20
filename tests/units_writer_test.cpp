#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <vector>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/ai.h"
#include "../interfaces/los_radar.h"
#include "../interfaces/orders.h"
#include "../interfaces/units.h"
#include "../interfaces/weapons.h"
#include "../interfaces/targeting.h"
#include "../src/state/sim_globals.h"
#include "../src/units/unit_def.h"
#include "../src/units/unit_type_set.h"
#include "../src/orders/mission_type.h"
#include "../src/util/vec3.h"
#pragma pack(push, 1)
#include "../src/weapons/weapon_def.h"
#include "../src/weapons/unit_weapon_slot.h"
#include "../src/orders/order.h"
#include "../src/map/cell.h"
#include "../src/map/spatial_bucket.h"
#include "../src/game/unit_resources.h"
#include "../src/network/player.h"
#include "../src/units/unit.h"
#include "../src/game/game.h"
#include "../src/ai/squad_manager.h"
#pragma pack(pop)
#include "../src/ai/ai_player.h"
#include "../src/ai/ai_idle.h"
#include "../src/game/scheduler.h"
#include "../src/game/weapons_events.h"
#include "../src/seams/ai_world.h"
#include "../src/orders/order_list.h"
#include "../src/orders/orders_seam.h"
#include "../src/seams/orders_world.h"
#include "../src/units/weapons_effects.h"
#include "../src/units/factory_index.h"
#include "../src/map/line_of_sight_index.h"

Game* g_game;
Seams g_seams;
SimGlobals* g_simGlobals = 0;
void (*g_outOfMemoryHandler)() = 0;

static int g_failures;

static void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}

int __stdcall HarnessTestMode()
{
    return 0;
}

extern "C" void HOST_CALL host_fatal(void*, const char* title, const char* text)
{
    printf("FAIL: host_fatal: %s: %s\n", title, text);
    g_failures++;
}

void __stdcall FatalError(char* text)
{
    printf("FAIL: FatalError: %s\n", text);
    g_failures++;
}

SimGlobals& __stdcall SimCreate()
{
    static char storage[sizeof(SimGlobals)];
    return *(SimGlobals*)storage;
}

enum Tag { TAG_NOTICE = 1, TAG_DRAW, TAG_STOP, TAG_ATTACK, TAG_SPEECH, TAG_SET_UNIT };

enum {
    kGuard = 1,
    kPad = 2,
    kPadOff = 3,
    kEnemyA = 4,
    kEnemyB = 5,
    kEnemyFactory = 6,
    kUnits = 6,
    kPlayers = 2,
    kCell = 32
};

static Unit* s_units;
static UnitDef* s_defs;
static UnitTypeSet* s_noTypes;
static SquadManager* s_managers[kPlayers];
static Order* s_order;
static WeaponDef* s_weaponDef;
static std::vector<int> s_log;
static int s_canReach = 1;
static int s_attackResult = 1;
static Unit* s_currentTarget;

int __stdcall RandomInt(int n)
{
    s_log.push_back(TAG_DRAW);
    return n > 0 ? n - 1 : 0;
}

void __stdcall RecordSimEvent(SimEventKind kind, unsigned int, const unsigned int*, unsigned int)
{
    if (kind == SIM_EVENT_UNIT_SPEECH)
        s_log.push_back(TAG_SPEECH);
}

void __stdcall HarnessAiEvent(const AiEventHeader*, unsigned int, int) {}
void __stdcall HarnessWeaponsEvent(const WeaponsEventHeader*, unsigned int, int) {}
void __stdcall HarnessOrdersEvent(const OrdersEventHeader*, unsigned int, int) {}
void __stdcall HarnessMovementEvent(const MovementEventHeader*, unsigned int, int) {}

Unit* __stdcall ScheduleTargetingTargetUnit(Unit*, int)
{
    return s_currentTarget;
}

int __stdcall IsUnitVisibleToPlayer(Player*, Unit*)
{
    return 1;
}

static bool IsHandled(SchedHandler handler)
{
    return handler == UnitsOnWeaponsAttacked || handler == AiOnWeaponsAttacked || handler == OrdersOnWeaponsAttacked
        || handler == UnitsOnWeaponsAttackedSpeech || handler == UnitsOnWeaponsHit || handler == OrdersOnAiStop;
}

void __stdcall SchedPublish(int outbox, const void* data, unsigned int size)
{
    int kind = *(const int*)data;
    if (outbox == SCHED_ORDERS && kind == TARGETING_SET_UNIT)
        s_log.push_back(TAG_SET_UNIT);
    int count = 0;
    const SchedRoute* routes = SchedRoutes(&count);
    for (int i = 0; i < count; i++)
        if (routes[i].outbox == outbox && routes[i].kind == kind && routes[i].synchronous && IsHandled(routes[i].handler))
            routes[i].handler(data, size);
}

static int IndexOf(Unit* unit)
{
    return (int)(unit - s_units);
}

static void __stdcall FakeNotifyUnitRefs(Unit*, int event)
{
    if (event == 16)
        s_log.push_back(TAG_NOTICE);
}

int __stdcall IssueAttackOrder(Unit*, Unit*, int)
{
    s_log.push_back(TAG_ATTACK);
    return s_attackResult;
}

void __stdcall DeleteOrders(Unit*, int)
{
    s_log.push_back(TAG_STOP);
}

static int __stdcall FakeGetOrderFlags(Unit*)
{
    return 0;
}

static int __stdcall FakeWeaponCanReachUnit(Unit*, Unit*, unsigned char)
{
    return s_canReach;
}

static void Place(int unit, int cellX, int cellZ)
{
    s_units[unit].pos.x = (cellX * kCell) << 16;
    s_units[unit].pos.y = 0;
    s_units[unit].pos.z = (cellZ * kCell) << 16;
}

static void SetUnit(int i, int player, unsigned int flags1, unsigned int flags2)
{
    Unit* u = &s_units[i];
    s_defs[i].flags1 = flags1;
    s_defs[i].flags2 = flags2;
    s_defs[i].categories = s_noTypes;
    for (int w = 0; w < 3; w++)
        s_defs[i].weaponCategories[w] = s_noTypes;
    u->def = &s_defs[i];
    u->unitDefIndex = (unsigned short)(10 + i);
    u->player = &g_game->players[player];
    u->playerIndex = (unsigned char)player;
    u->id = (unsigned short)i;
}

static const unsigned int kPadFlags = 0x40 | 0x200;

static UnitTypeSet* s_attackerTypes;

static void Layout()
{
    SetUnit(kGuard, 0, 0x10000000, 0x1000);
    SetUnit(kPad, 0, 0x10000000 | kPadFlags, 0);
    SetUnit(kPadOff, 0, 0x10000000 | kPadFlags, 0);
    SetUnit(kEnemyA, 1, 0x10000000, 0);
    SetUnit(kEnemyB, 1, 0x10000000, 0);
    SetUnit(kEnemyFactory, 1, 0x10000000 | kPadFlags, 0);
    Place(kGuard, 10, 10);
    Place(kPad, 12, 10);
    Place(kPadOff, 14, 10);
    Place(kEnemyA, 11, 10);
    Place(kEnemyB, 30, 10);
    Place(kEnemyFactory, 12, 11);
}

static void Reset(int type0, int type1)
{
    Layout();
    for (int i = 1; i <= kUnits; i++) {
        s_units[i].flags = 0;
        s_units[i].activateFlags = 0;
        s_units[i].buildLeft = 0;
        s_units[i].netDirtyFlags = 0;
        s_units[i].lastAttackerSlot = 1;
        s_units[i].lastDamageType = 0;
        s_units[i].list = (Order*)0;
        for (int w = 0; w < 3; w++)
            s_units[i].weapons[w].flags = 0;
    }
    memset(s_order, 0, sizeof(Order));
    memset((void*)s_weaponDef, 0, sizeof(WeaponDef));
    for (int p = 0; p < kPlayers; p++) {
        Player* player = &g_game->players[p];
        memset(player->allied, 0, sizeof player->allied);
        player->allied[p] = 1;
        player->active = 1;
        s_managers[p]->nextAction = 0;
    }
    g_game->players[0].type = (unsigned char)type0;
    g_game->players[1].type = (unsigned char)type1;
    g_game->ticks = 0;
    s_log.clear();
    s_canReach = 1;
    s_attackResult = 1;
    s_currentTarget = 0;
}

static AiSeam s_standard;
static AiSeam* s_idle;
static int s_squadRuns;
static int s_playerAiRuns;

static void __stdcall CountSquadRun(int)
{
    s_squadRuns++;
}

static void __stdcall CountPlayerAi(int)
{
    s_playerAiRuns++;
}

static void Setup()
{
    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    memset(g_game, 0, sizeof(Game));
    s_units = (Unit*)SimAlloc((kUnits + 1) * sizeof(Unit));
    memset((void*)s_units, 0, (kUnits + 1) * sizeof(Unit));
    s_defs = (UnitDef*)SimAlloc((kUnits + 1) * sizeof(UnitDef));
    memset((void*)s_defs, 0, (kUnits + 1) * sizeof(UnitDef));
    s_noTypes = new (SimAlloc(sizeof(UnitTypeSet))) UnitTypeSet();
    s_attackerTypes = new (SimAlloc(sizeof(UnitTypeSet))) UnitTypeSet();
    s_attackerTypes->bits[(10 + kEnemyA) >> 5] |= 1 << ((10 + kEnemyA) & 31);
    s_order = (Order*)SimAlloc(sizeof(Order));
    s_weaponDef = (WeaponDef*)SimAlloc(sizeof(WeaponDef));
    g_game->units = s_units;
    g_game->unitsEnd = &s_units[kUnits];
    for (int p = 0; p < kPlayers; p++) {
        g_game->players[p].index = (unsigned char)p;
        s_managers[p] = (SquadManager*)SimAlloc(sizeof(SquadManager));
        memset((void*)s_managers[p], 0, sizeof(SquadManager));
        g_game->players[p].ai = s_managers[p];
    }
    Reset(1, 2);

    static OrdersSeam orders;
    memset(&orders, 0, sizeof orders);
    orders.Apply = OrdersApply;
    orders.GetOrderFlags = FakeGetOrderFlags;
    g_seams.orders = &orders;
    static UnitsSeam units;
    memset(&units, 0, sizeof units);
    units.NotifyUnitRefs = FakeNotifyUnitRefs;
    units.CreateFactoryIndex = CreateFactoryIndex;
    units.FreeFactoryIndex = FreeFactoryIndex;
    units.RefreshFactoryIndex = RefreshFactoryIndex;
    units.GetFactoriesInRadius = GetFactoriesInRadius;
    units.WriteState = UnitsWriteState;
    g_seams.units = &units;
    static WeaponsSeam weapons;
    memset(&weapons, 0, sizeof weapons);
    weapons.WeaponCanReachUnit = FakeWeaponCanReachUnit;
    g_seams.weapons = &weapons;
    static LosRadarSeam losRadar;
    memset(&losRadar, 0, sizeof losRadar);
    losRadar.CreateUnitIndex = CreateUnitIndex;
    losRadar.FreeUnitIndex = FreeUnitIndex;
    losRadar.RefreshUnitIndex = RefreshUnitIndex;
    losRadar.GetVisibleEnemiesInRadius = GetVisibleEnemiesInRadius;
    losRadar.GetVisibleEnemiesInRadiusList = GetVisibleEnemiesInRadiusList;
    losRadar.HasReadyUnitInRange = HasReadyUnitInRange;
    losRadar.IsPointVisible = IsPointVisible;
    losRadar.GetSeenUnitsInRadius = GetSeenUnitsInRadius;
    g_seams.los_radar = &losRadar;

    memset(&s_standard, 0, sizeof s_standard);
    s_standard.Apply = AiApply;
    s_standard.UpdateSquads = CountSquadRun;
    s_standard.UpdatePlayerAI = CountPlayerAi;
    s_idle = AiIdleSeam(&s_standard);
    g_seams.ai = &s_standard;
}

static int Count(const BlockVector<Ref<Unit> >& list)
{
    return (int)(list.End() - list.Begin());
}

static void PresentAt(int unit, int tick, int from)
{
    s_units[unit].flags = tick >= from ? 0x10000000 : 0;
}

static void ApplyWorld(int tick)
{
    PresentAt(kGuard, tick, 45);
    PresentAt(kPad, tick, 20);
    PresentAt(kPadOff, tick, 20);
    PresentAt(kEnemyA, tick, 10);
    PresentAt(kEnemyB, tick, 40);
    PresentAt(kEnemyFactory, tick, 25);
    s_units[kPad].activateFlags = 1;
    s_units[kPadOff].activateFlags = tick >= 50 ? 1 : 0;
    s_units[kEnemyFactory].activateFlags = 1;
}

static void PlayerStep(const AiSeam* ai, int player)
{
    ai->UpdateSquads(player);
    g_seams.los_radar->RefreshUnitIndex(player);
    g_seams.units->RefreshFactoryIndex(player);
    ai->UpdatePlayerAI(player);
}

struct Sample {
    int visible[kPlayers];
    int factories[kPlayers];
    unsigned int lastTick[kPlayers];
};

static void RunTicks(const AiSeam* ai, Sample* samples, int ticks)
{
    Reset(2, 2);
    s_squadRuns = 0;
    s_playerAiRuns = 0;
    for (int p = 0; p < kPlayers; p++) {
        g_seams.los_radar->CreateUnitIndex(p);
        g_seams.units->CreateFactoryIndex(p);
    }
    for (int t = 0; t <= ticks; t++) {
        g_game->ticks = t;
        ApplyWorld(t);
        for (int p = 0; p < kPlayers; p++)
            PlayerStep(ai, p);
        for (int p = 0; p < kPlayers; p++) {
            samples[t].visible[p] = Count(LosUnitIndexOf(p)->visible);
            samples[t].factories[p] = Count(*FactoryIndexOf(p));
            samples[t].lastTick[p] = LosUnitIndexOf(p)->lastTick;
        }
    }
    for (int p = 0; p < kPlayers; p++) {
        g_seams.los_radar->FreeUnitIndex(p);
        g_seams.units->FreeFactoryIndex(p);
    }
}

static void CheckIndexCadence(const char* module, const AiSeam* ai, int expectSquadRuns)
{
    static Sample samples[91];
    RunTicks(ai, samples, 90);
    const int at[] = { 29, 30, 59, 60, 89, 90 };
    const int visible0[] = { 0, 2, 2, 3, 3, 3 };
    const int factories0[] = { 0, 1, 1, 2, 2, 2 };
    const int visible1[] = { 0, 2, 2, 3, 3, 3 };
    const int factories1[] = { 0, 1, 1, 1, 1, 1 };
    const unsigned int lastTick[] = { 0, 30, 30, 60, 60, 90 };
    bool ok = true;
    for (int i = 0; i < 6; i++) {
        const Sample& s = samples[at[i]];
        ok = ok && s.visible[0] == visible0[i] && s.factories[0] == factories0[i] && s.visible[1] == visible1[i]
            && s.factories[1] == factories1[i] && s.lastTick[0] == lastTick[i] && s.lastTick[1] == lastTick[i];
    }
    printf("%s: visible lists %d %d %d %d, factory lists %d %d %d %d\n", module, samples[30].visible[0], samples[60].visible[0],
        samples[30].visible[1], samples[60].visible[1], samples[30].factories[0], samples[60].factories[0], samples[30].factories[1], samples[60].factories[1]);
    Check(ok, "every player's visible and factory lists are refilled at ticks 30, 60 and 90 and not between");
    Check(s_squadRuns == expectSquadRuns, "the squads run as often as the module says");
    Check((s_playerAiRuns != 0) == (expectSquadRuns != 0), "UpdatePlayerAI runs only when the module runs squads");
}

static int Idx(Unit* u)
{
    return u == 0 ? 0 : IndexOf(u);
}

struct Recorded {
    int sections[8];
    int count;
    unsigned int size;
    int handles;
};
static Recorded s_recorded;

static void __stdcall RecBegin(int section, const char*)
{
    if (s_recorded.count < 8)
        s_recorded.sections[s_recorded.count] = section;
    s_recorded.count++;
}

static void __stdcall RecInt(const char* field, int, int, unsigned int value)
{
    if (strcmp(field, "factories.size") == 0)
        s_recorded.size = value;
}

static void __stdcall RecBytes(const char*, int, const void*, int) {}

static void __stdcall RecHandle(const char* field, int, int, unsigned int)
{
    if (strcmp(field, "factories[%u]") == 0)
        s_recorded.handles++;
}

static void __stdcall RecRun(int) {}

static void CheckFactoryRecords(const UnitsSeam& units, unsigned int factories)
{
    memset(&s_recorded, 0, sizeof s_recorded);
    StateWriter writer = { RecBegin, RecInt, RecBytes, RecHandle, RecRun };
    units.WriteState(&writer);
    Check(s_recorded.count == 1 && s_recorded.sections[0] == 25, "WriteState writes a section 25 record for each live factory index");
    Check(s_recorded.size == factories && (unsigned int)s_recorded.handles == factories, "the section 25 record lists the factories of the index");
}

static void CheckFactoryIndex()
{
    Reset(1, 2);
    const UnitsSeam& units = *g_seams.units;
    for (int i = 1; i <= kUnits; i++)
        s_units[i].flags = 0x10000000;
    s_units[kPad].activateFlags = 1;
    s_units[kPadOff].activateFlags = 0;
    s_units[kEnemyFactory].activateFlags = 1;
    s_defs[kGuard].flags1 |= 0x40;
    s_units[kGuard].activateFlags = 1;
    units.CreateFactoryIndex(0);
    Check(FactoryIndexOf(0) != 0 && Count(*FactoryIndexOf(0)) == 0, "CreateFactoryIndex makes an empty index");
    g_game->ticks = 29;
    units.RefreshFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 0, "the index is not refilled before tick 30");
    g_game->ticks = 30;
    units.RefreshFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 1 && *FactoryIndexOf(0)->Begin() == &s_units[kPad],
          "the index holds own finished ready units with flags 0x40 and 0x200 that are switched on, and no padless, switched-off or enemy ones");
    CheckFactoryRecords(units, 1);
    s_units[kPadOff].activateFlags = 1;
    s_units[kPadOff].buildLeft = 1.0f;
    g_game->ticks = 60;
    units.RefreshFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 1, "an unfinished factory is not in the index");
    s_units[kPadOff].buildLeft = 0;
    g_game->ticks = 89;
    units.RefreshFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 1, "the index is not refilled at tick 89");
    g_game->ticks = 90;
    units.RefreshFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 2, "a finished switched-on factory joins at the next refill");
    s_units[kPadOff].flags |= 0x4000;
    g_game->ticks = 120;
    units.RefreshFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 1, "a dying factory is not in the index");
    s_units[kPadOff].flags = 0x10000000;
    g_game->ticks = 150;
    units.RefreshFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 2, "the index is refilled again");

    std::vector<Unit*> found;
    Vec3 at;
    at.x = (12 * kCell) << 16;
    at.y = 0;
    at.z = (10 * kCell) << 16;
    units.GetFactoriesInRadius(0, &at, 10, &found);
    Check(found.size() == 1 && found[0] == &s_units[kPad], "GetFactoriesInRadius appends the factory in range");
    found.clear();
    units.GetFactoriesInRadius(0, &at, 40, &found);
    Check(found.size() == 1, "GetFactoriesInRadius leaves out a factory out of range");
    found.clear();
    units.GetFactoriesInRadius(0, &at, 200, &found);
    Check(found.size() == 2 && found[0] == &s_units[kPad] && found[1] == &s_units[kPadOff], "GetFactoriesInRadius appends the factories in the index's order");
    s_units[kPadOff].activateFlags = 0;
    found.clear();
    units.GetFactoriesInRadius(0, &at, 200, &found);
    Check(found.size() == 1, "GetFactoriesInRadius leaves out a factory switched off since the refill");
    s_units[kPadOff].activateFlags = 1;
    s_defs[kPadOff].flags1 &= ~0x200u;
    found.clear();
    units.GetFactoriesInRadius(0, &at, 200, &found);
    Check(found.size() == 1, "GetFactoriesInRadius leaves out a factory whose type has no pad");
    s_defs[kPadOff].flags1 |= 0x200;

    units.FreeFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 0, "FreeFactoryIndex empties the index");
    g_game->ticks = 300;
    units.RefreshFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 0, "a freed index is not refilled");
    units.CreateFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 0, "an index made again starts empty");
    units.RefreshFactoryIndex(0);
    Check(Count(*FactoryIndexOf(0)) == 2, "an index made again is filled by its first refill");
    units.FreeFactoryIndex(0);
}

static void Attacked(int unit, int attacker, int amount = 10)
{
    WeaponsPublishAttacked(&s_units[unit], attacker == 0 ? (Unit*)0 : &s_units[attacker], amount);
}

static std::vector<int> AttackLog(int attacked, int attacker)
{
    s_log.clear();
    Attacked(attacked, attacker);
    return s_log;
}

static bool LogIs(const std::vector<int>& log, int a = 0, int b = 0, int c = 0, int d = 0, int e = 0, int f = 0)
{
    int expected[6] = { a, b, c, d, e, f };
    int n = 0;
    while (n < 6 && expected[n] != 0)
        n++;
    if ((int)log.size() != n)
        return false;
    for (int i = 0; i < n; i++)
        if (log[i] != expected[i])
            return false;
    return true;
}

static void CheckRouteOrder()
{
    int count = 0;
    const SchedRoute* routes = SchedRoutes(&count);
    SchedHandler order[8];
    int n = 0;
    for (int i = 0; i < count; i++)
        if (routes[i].outbox == SCHED_WEAPONS && routes[i].kind == WEAPONS_EFFECT_ATTACKED && n < 8)
            order[n++] = routes[i].handler;
    Check(n == 4 && order[0] == UnitsOnWeaponsAttacked && order[1] == AiOnWeaponsAttacked && order[2] == OrdersOnWeaponsAttacked
              && order[3] == UnitsOnWeaponsAttackedSpeech,
          "ATTACKED is routed to the notice, ai, the return fire and the speech, in that order");
    int hit = 0;
    for (int i = 0; i < count; i++)
        if (routes[i].outbox == SCHED_WEAPONS && routes[i].kind == WEAPONS_EFFECT_HIT && routes[i].handler == UnitsOnWeaponsHit && routes[i].synchronous)
            hit++;
    Check(hit == 1, "HIT is routed synchronously to the units hit handler");
}

static void CheckSplitAttack()
{
    Reset(2, 1);
    s_units[kGuard].flags = 0x10000000;
    s_units[kEnemyA].flags = 0x10000000;
    g_seams.ai = &s_standard;
    Check(LogIs(AttackLog(kGuard, kEnemyA), TAG_NOTICE, TAG_DRAW, TAG_STOP, TAG_ATTACK, TAG_SPEECH),
          "an attack on a computer player's unit gives the notice, the draw, the stop, the return fire and the speech, in that order");
    Check(s_managers[0]->nextAction == 299 + 0 + 30, "the computer player's draw sets its next action");
    Check(LogIs(AttackLog(kGuard, 0), TAG_NOTICE, TAG_DRAW, TAG_STOP, TAG_SPEECH), "an attack by no unit gives no return fire");

    s_defs[kGuard].flags2 = 0;
    Check(LogIs(AttackLog(kGuard, kEnemyA), TAG_NOTICE, TAG_ATTACK, TAG_SPEECH), "a unit without the stop flag draws nothing and is not stopped");
    s_defs[kGuard].flags2 = 0x1000;

    g_game->players[0].type = 1;
    Check(LogIs(AttackLog(kGuard, kEnemyA), TAG_NOTICE, TAG_ATTACK, TAG_SPEECH), "an attack on a human player's unit gives no draw and no stop");
    g_game->players[0].type = 2;

    s_units[kGuard].lastAttackerSlot = 0;
    Check(LogIs(AttackLog(kGuard, kEnemyA), TAG_NOTICE, TAG_DRAW, TAG_STOP, TAG_ATTACK), "an attack by the unit's own slot with another damage type has no speech");
    s_units[kGuard].lastDamageType = 1;
    Check(LogIs(AttackLog(kGuard, kEnemyA), TAG_NOTICE, TAG_DRAW, TAG_STOP, TAG_ATTACK, TAG_SPEECH), "damage type 1 has the speech whatever the slot");
    s_units[kGuard].lastAttackerSlot = 1;
    s_units[kGuard].lastDamageType = 0;

    WeaponsAttacked none;
    memset(&none, 0, sizeof none);
    none.h = WeaponsHeaderOf(WEAPONS_EFFECT_ATTACKED);
    s_log.clear();
    SchedPublish(SCHED_WEAPONS, &none, sizeof none);
    Check(s_log.empty(), "an attack with a null unit does nothing");

    WeaponsPublishHit(&s_units[kGuard], 10, 4);
    Check(s_units[kGuard].netDirtyFlags == 0x4000, "a hit with enemy damage over twice the friendly damage sets 0x4000");
    s_units[kGuard].netDirtyFlags = 0;
    WeaponsPublishHit(&s_units[kGuard], 8, 4);
    Check(s_units[kGuard].netDirtyFlags == 0x2000, "a hit with enemy damage of twice the friendly damage sets 0x2000");
    WeaponsPublishHit(&s_units[kGuard], 10, 4);
    Check(s_units[kGuard].netDirtyFlags == 0x6000, "the hit flags accumulate");
    WeaponsHit noShooter;
    memset(&noShooter, 0, sizeof noShooter);
    noShooter.h = WeaponsHeaderOf(WEAPONS_EFFECT_HIT);
    s_units[kGuard].netDirtyFlags = 0;
    SchedPublish(SCHED_WEAPONS, &noShooter, sizeof noShooter);
    Check(s_units[kGuard].netDirtyFlags == 0, "a hit with a null unit does nothing");
}

static int ReturnFire(int unit, int attacker)
{
    s_log.clear();
    WeaponsAttacked e;
    memset(&e, 0, sizeof e);
    e.h = WeaponsHeaderOf(WEAPONS_EFFECT_ATTACKED, &s_units[unit]);
    e.attacker = attacker == 0 ? 0 : Ref<Unit>(&s_units[attacker]).offset;
    e.amount = 10;
    OrdersOnWeaponsAttacked(&e, sizeof(e));
    int attacks = 0;
    for (size_t i = 0; i < s_log.size(); i++)
        if (s_log[i] == TAG_ATTACK)
            attacks++;
    return attacks;
}

static int SetUnitsPublished()
{
    int n = 0;
    for (size_t i = 0; i < s_log.size(); i++)
        if (s_log[i] == TAG_SET_UNIT)
            n++;
    return n;
}

static void CheckReturnFireGates()
{
    Reset(1, 2);
    s_units[kGuard].flags = 0x10000000;
    s_units[kEnemyA].flags = 0x10000000;
    Check(ReturnFire(kGuard, kEnemyA) == 1, "an idle unit with no orders attacks its attacker");
    Check(ReturnFire(kGuard, kPad) == 0, "a unit attacked by its own player gives no attack order");
    Check(ReturnFire(kGuard, 0) == 0, "a unit attacked by no unit gives no attack order");
    s_units[kEnemyA].unitDefIndex = 0;
    Check(ReturnFire(kGuard, kEnemyA) == 0, "a unit attacked by a dead unit gives no attack order");
    s_units[kEnemyA].unitDefIndex = 14;
    g_game->players[0].type = 2;
    Check(ReturnFire(kGuard, kEnemyA) == 1, "a computer player's unit attacks its attacker");
    g_game->players[0].type = 3;
    Check(ReturnFire(kGuard, kEnemyA) == 0, "a unit of a player that is neither human nor computer gives no attack order");
    g_game->players[0].type = 1;
    g_game->players[0].active = 0;
    Check(ReturnFire(kGuard, kEnemyA) == 0, "a unit of an inactive player gives no attack order");
    g_game->players[0].active = 1;
    s_defs[kGuard].flags1 = 0x1;
    Check(ReturnFire(kGuard, kEnemyA) == 0, "a unit whose type lacks flags 0x10010000 gives no attack order");
    s_defs[kGuard].flags1 = 0x10000;
    Check(ReturnFire(kGuard, kEnemyA) == 1, "a unit whose type has flag 0x10000 attacks its attacker");
    s_defs[kGuard].flags1 = 0x10000000;
    s_units[kGuard].buildLeft = 1.0f;
    Check(ReturnFire(kGuard, kEnemyA) == 0, "an unfinished unit gives no attack order");
    s_units[kGuard].buildLeft = 0;
    g_game->players[0].allied[1] = 1;
    Check(ReturnFire(kGuard, kEnemyA) == 0, "a unit attacked by an ally gives no attack order");
    g_game->players[0].allied[1] = 0;
    s_units[kGuard].list = s_order;
    Check(ReturnFire(kGuard, kEnemyA) == 0, "a unit with orders that are not idle gives no attack order");
    s_order->flags_42 = ORDER_FLAG_IDLE;
    Check(ReturnFire(kGuard, kEnemyA) == 1, "a unit whose orders are idle attacks its attacker");
    s_units[kGuard].list = (Order*)0;
    s_defs[kGuard].categories = s_attackerTypes;
    Check(ReturnFire(kGuard, kEnemyA) == 0, "a unit that does not attack the attacker's category gives no attack order");
    s_defs[kGuard].categories = s_noTypes;
    s_defs[kGuard].weaponCategories[0] = s_attackerTypes;
    Check(ReturnFire(kGuard, kEnemyA) == 0, "a unit whose first weapon does not target the attacker's category gives no attack order");
    s_defs[kGuard].weaponCategories[0] = s_noTypes;
    s_canReach = 0;
    Check(ReturnFire(kGuard, kEnemyA) == 0 && SetUnitsPublished() == 0, "a unit that cannot reach the attacker gives no attack order and no target");
    s_canReach = 1;

    s_attackResult = 0;
    s_units[kGuard].flags |= 0x100000;
    s_units[kGuard].weapons[0].weapon = s_weaponDef;
    s_units[kGuard].weapons[0].flags = 2 | 0x10;
    Check(ReturnFire(kGuard, kEnemyA) == 1 && SetUnitsPublished() == 1, "when the attack order is refused a unit that may fire retargets a ready weapon");
    s_units[kGuard].weapons[0].flags = 2;
    ReturnFire(kGuard, kEnemyA);
    Check(SetUnitsPublished() == 0, "a weapon that is not ready is not retargeted");
    s_units[kGuard].weapons[0].flags = 2 | 0x10;
    s_weaponDef->flags.value = 1u << 26;
    ReturnFire(kGuard, kEnemyA);
    Check(SetUnitsPublished() == 0, "a weapon with flag bit 26 is not retargeted");
    s_weaponDef->flags.value = 0;
    s_units[kGuard].flags &= ~0x100000u;
    ReturnFire(kGuard, kEnemyA);
    Check(SetUnitsPublished() == 0, "a unit holding its fire does not retarget");
    s_units[kGuard].flags |= 0x100000;
    s_currentTarget = &s_units[kEnemyB];
    ReturnFire(kGuard, kEnemyA);
    Check(SetUnitsPublished() == 0, "a weapon that already has a reachable target keeps it");
    s_canReach = 1;
    s_attackResult = 1;
}

struct Snapshot {
    std::vector<int> values;
    std::vector<int> attacked;
};

static void Note(Snapshot* s, const std::vector<Unit*>& list)
{
    s->values.push_back((int)list.size());
    for (size_t i = 0; i < list.size(); i++)
        s->values.push_back(IndexOf(list[i]));
}

static void Observe(const AiSeam* ai, Snapshot* s)
{
    g_seams.ai = const_cast<AiSeam*>(ai);
    Reset(1, 2);
    s_squadRuns = 0;
    s_playerAiRuns = 0;
    for (int p = 0; p < kPlayers; p++) {
        g_seams.los_radar->CreateUnitIndex(p);
        g_seams.units->CreateFactoryIndex(p);
    }
    for (int t = 0; t <= 60; t++) {
        g_game->ticks = t;
        ApplyWorld(t);
        for (int p = 0; p < kPlayers; p++)
            PlayerStep(ai, p);
    }
    Unit* guard = &s_units[kGuard];
    const int radii[] = { 10, 40, 100, 200, 2000 };
    for (int i = 0; i < 5; i++) {
        std::vector<Unit*> found;
        g_seams.los_radar->GetVisibleEnemiesInRadius(0, &guard->pos, radii[i], 0, &found);
        Note(s, found);
        s->values.push_back(g_seams.los_radar->HasReadyUnitInRange(0, &guard->pos, radii[i]) ? 1 : 0);
        Ref<Unit> refs[4];
        s->values.push_back(g_seams.los_radar->GetVisibleEnemiesInRadiusList(0, &guard->pos, radii[i], 0, refs, 4));
        found.clear();
        g_seams.units->GetFactoriesInRadius(0, &guard->pos, radii[i], &found);
        Note(s, found);
    }
    s_log.clear();
    Attacked(kGuard, kEnemyA);
    s->attacked = s_log;
    s_log.clear();
    Attacked(kGuard, kEnemyB);
    s->attacked.insert(s->attacked.end(), s_log.begin(), s_log.end());
    for (int p = 0; p < kPlayers; p++) {
        g_seams.los_radar->FreeUnitIndex(p);
        g_seams.units->FreeFactoryIndex(p);
    }
    g_seams.ai = &s_standard;
}

static void CheckHumanUnderIdle()
{
    static Snapshot standard, idle;
    standard = Snapshot();
    idle = Snapshot();
    Observe(&s_standard, &standard);
    Check(s_squadRuns == 61 * kPlayers, "the standard module runs the squads of every player at every tick");
    Observe(s_idle, &idle);
    Check(s_squadRuns == 0 && s_playerAiRuns == 0, "the idle module runs no squads and no player AI");
    Check(standard.values == idle.values, "a human player's guard, weapon search and pad search see the same units under the idle module");
    Check(standard.values.size() > 20 && standard.values[0] == 0 && standard.values[3] == 0, "the observed searches cover empty and non-empty answers");
    Check(standard.attacked == idle.attacked && LogIs(idle.attacked, TAG_NOTICE, TAG_ATTACK, TAG_SPEECH, TAG_NOTICE, TAG_ATTACK, TAG_SPEECH),
          "a human player's unit returns fire the same under the idle module");

    Reset(2, 1);
    s_units[kGuard].flags = 0x10000000;
    s_units[kEnemyA].flags = 0x10000000;
    g_seams.ai = s_idle;
    Check(LogIs(AttackLog(kGuard, kEnemyA), TAG_NOTICE, TAG_ATTACK, TAG_SPEECH), "under the idle module a computer player's unit is noticed and returns fire but is not stopped and draws nothing");
    Check(s_managers[0]->nextAction == 0, "under the idle module a computer player's next action is not set");
    g_seams.ai = &s_standard;
}

int main()
{
    Setup();
    CheckFactoryIndex();
    CheckRouteOrder();
    CheckSplitAttack();
    CheckReturnFireGates();
    CheckIndexCadence("standard", &s_standard, 91 * kPlayers);
    CheckIndexCadence("idle", s_idle, 0);
    CheckHumanUnderIdle();

    printf(g_failures ? "FAILED\n" : "units_writer_test: all passed\n");
    return g_failures != 0;
}
