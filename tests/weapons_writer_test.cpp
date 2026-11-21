#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/units.h"
#include "../interfaces/weapons.h"
#include "../src/state/sim_globals.h"
#include "../src/units/unit_def.h"
#include "../src/units/unit_type_set.h"
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
#pragma pack(pop)
#include "../src/game/scheduler.h"
#include "../src/game/weapons_events.h"
#include "../src/seams/weapons_world.h"
#include "../src/units/weapons_effects.h"
#include "../src/weapons/damage.h"
#include "../src/weapons/weapons_apply.h"

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

enum {
    kTicks = 90,
    kTamperTick = 20,
    kVictim = 1,
    kUnits = 1,
    kHealth = 1000
};

static Unit* s_units;
static UnitDef* s_defs;
static UnitTypeSet* s_noTypes;
static int s_effects;
static int s_marked;

int __stdcall RandomInt(int n)
{
    return n > 0 ? n - 1 : 0;
}

void __stdcall RecordSimEvent(SimEventKind, unsigned int, const unsigned int*, unsigned int) {}
void __stdcall HarnessWeaponsEvent(const WeaponsEventHeader*, unsigned int, int) {}
void __stdcall HarnessOrdersEvent(const OrdersEventHeader*, unsigned int, int) {}
void __stdcall HarnessMovementEvent(const MovementEventHeader*, unsigned int, int) {}

static bool IsHandled(SchedHandler handler)
{
    return handler == WeaponsInbox || handler == UnitsOnWeaponsMarkDamaged || handler == UnitsOnWeaponsDamageRecord
        || handler == UnitsOnWeaponsHealth;
}

void __stdcall SchedPublish(int outbox, const void* data, unsigned int size)
{
    int kind = *(const int*)data;
    int count = 0;
    const SchedRoute* routes = SchedRoutes(&count);
    for (int i = 0; i < count; i++)
        if (routes[i].outbox == outbox && routes[i].kind == kind && routes[i].synchronous && IsHandled(routes[i].handler)) {
            s_effects++;
            routes[i].handler(data, size);
        }
}

static void __stdcall FakeMarkRecentlyDamaged(Unit*)
{
    s_marked++;
}

static int s_tamperAt = -1;
static int s_tick;
static int s_dropped = -1;

static int s_applied;

static int __stdcall PlainApply(const WeaponsEventHeader* e, unsigned int size)
{
    s_applied++;
    return WeaponsApply(e, size);
}

static int __stdcall TamperedApply(const WeaponsEventHeader* e, unsigned int size)
{
    if (e->kind == WEAPONS_DAMAGE && s_tick == s_tamperAt && s_dropped < 0) {
        s_dropped = s_tick;
        return 0;
    }
    return PlainApply(e, size);
}

static int Amount(int tick)
{
    return 20 + (tick * 7) % 13;
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
    g_game->units = s_units;
    g_game->unitsEnd = &s_units[kUnits];
    g_game->players[0].index = 0;
    g_game->players[0].active = 1;
    g_game->players[0].type = 1;
    s_defs[kVictim].flags1 = 0x10000000;
    s_defs[kVictim].maxHealth = kHealth;
    s_defs[kVictim].categories = s_noTypes;
    s_units[kVictim].def = &s_defs[kVictim];
    s_units[kVictim].unitDefIndex = 11;
    s_units[kVictim].player = &g_game->players[0];
    s_units[kVictim].playerIndex = 0;
    s_units[kVictim].id = kVictim;

    static UnitsSeam units;
    memset(&units, 0, sizeof units);
    units.MarkRecentlyDamaged = FakeMarkRecentlyDamaged;
    g_seams.units = &units;
}

static void Restart()
{
    Unit* u = &s_units[kVictim];
    u->flags = 0x10000000;
    u->health = kHealth;
    u->killCount = 0;
    u->activateFlags = 0;
    s_effects = 0;
    s_marked = 0;
    s_dropped = -1;
    s_applied = 0;
}

struct Run {
    short health[kTicks];
    unsigned char dying[kTicks];
    int tampered;
    int applied;
};

static void Play(Run* run, bool tamper)
{
    Restart();
    s_tamperAt = tamper ? kTamperTick : -1;
    for (int t = 0; t < kTicks; t++) {
        s_tick = t;
        g_game->ticks = t;
        WeaponsDamage e;
        memset(&e, 0, sizeof e);
        e.h = WeaponsHeaderOf(WEAPONS_DAMAGE, &s_units[kVictim]);
        e.amount = Amount(t);
        e.damageKind = 0;
        SchedPublish(SCHED_UNITS, &e, sizeof e);
        run->health[t] = s_units[kVictim].health;
        run->dying[t] = (s_units[kVictim].flags & 0x4000) != 0;
    }
    run->tampered = s_dropped;
    run->applied = s_applied;
}

int main()
{
    Setup();
    static WeaponsSeam plainSeam, tamperedSeam;
    memset(&plainSeam, 0, sizeof plainSeam);
    plainSeam.Apply = PlainApply;
    plainSeam.WriteState = WeaponsWriteState;
    tamperedSeam = plainSeam;
    tamperedSeam.Apply = TamperedApply;

    static Run plain, again, tampered;
    g_seams.weapons = &plainSeam;
    Play(&plain, false);
    Play(&again, false);
    g_seams.weapons = &tamperedSeam;
    Play(&tampered, true);

    Check(plain.tampered < 0, "the default drops no event");
    Check(plain.health[0] == kHealth - Amount(0), "a damage event takes its damage off the unit's health");
    Check(plain.health[0] > plain.health[1], "each damage event lowers the unit's health");
    Check(s_marked > 0, "a damage event marks the unit as recently damaged");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same health and death at every tick");
    int plainDeath = -1;
    int tamperedDeath = -1;
    for (int t = 0; t < kTicks; t++) {
        if (plainDeath < 0 && plain.dying[t])
            plainDeath = t;
        if (tamperedDeath < 0 && tampered.dying[t])
            tamperedDeath = t;
    }
    Check(plainDeath > kTamperTick, "the default's unit dies after the tampered tick");
    Check(plainDeath >= 0 && plain.health[plainDeath] <= 0, "the killing hit leaves no health and marks the unit dying");
    Check(tampered.tampered == kTamperTick, "the tampered module drops a damage event at the tampered tick");
    Check(tampered.applied == plain.applied - 1, "the tampered module drops exactly one event");
    Check(tamperedDeath > plainDeath, "the tampered module's unit dies later");
    int first = -1;
    int differing = 0;
    int dyingFirst = -1;
    for (int t = 0; t < kTicks; t++) {
        if (plain.health[t] != tampered.health[t]) {
            if (first < 0)
                first = t;
            differing++;
        }
        if (dyingFirst < 0 && plain.dying[t] != tampered.dying[t])
            dyingFirst = t;
    }
    Check(first == kTamperTick, "the unit's health differs first at the tampered tick");
    Check(differing == kTicks - kTamperTick, "the unit's health differs at every tick from the tampered tick on");
    Check(dyingFirst == plainDeath, "the default's unit is marked dying at a tick where the tampered one is not yet");
    printf("tampered tick %d, first differing tick %d, %d differing ticks, death at tick %d against tick %d\n", tampered.tampered, first, differing, plainDeath, tamperedDeath);

    printf(g_failures ? "FAILED\n" : "weapons_writer_test: all passed\n");
    return g_failures != 0;
}
