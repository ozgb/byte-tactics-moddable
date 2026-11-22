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
#include "../src/seams/units_world.h"
#include "../src/weapons/damage.h"
#include "../src/weapons/weapons_apply.h"
#include "../src/weapons/projectile.h"
#include "../interfaces/state_writer.h"

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

void __stdcall FatalError(const char* text)
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
void __stdcall HarnessUnitsEvent(const UnitsEventHeader*, unsigned int, int) {}
void __stdcall HarnessScriptsEvent(const ScriptsEventHeader*, unsigned int, int) {}

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

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

static int __stdcall FakeUnitsApply(const UnitsEventHeader* e, unsigned int)
{
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return 0;
    switch (e->kind) {
    case UNITS_MARK_DAMAGED:
        s_marked++;
        break;
    case UNITS_DAMAGE_RECORD: {
        const UnitsDamageRecord* p = reinterpret_cast<const UnitsDamageRecord*>(e);
        unit->lastDamageType = (unsigned char)p->damageKind;
        Unit* attacker = UnitOfOffset(p->attacker);
        if (attacker) {
            unit->lastAttackerSlot = attacker->playerIndex;
            unit->attacker = attacker;
        }
        break;
    }
    case UNITS_SET_HEALTH: {
        const UnitsSetHealth* p = reinterpret_cast<const UnitsSetHealth*>(e);
        unit->health = (short)p->health;
        if (p->dying == 1)
            unit->flags |= 0x4000;
        break;
    }
    }
    return 1;
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
    units.Apply = FakeUnitsApply;
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

void CountExplosionTables(unsigned int* explosions, unsigned int* debris, unsigned int* pieces)
{
    *explosions = 0;
    *debris = 0;
    *pieces = 0;
}

struct Line {
    char record[64];
    char field[64];
    int index;
    int size;
    unsigned int value;
};
static Line s_lines[2048];
static int s_lineCount;
static char s_record[64];

static void AddLine(const char* field, int index, int size, unsigned int value)
{
    if (s_lineCount >= 2048)
        return;
    Line* l = &s_lines[s_lineCount++];
    strcpy(l->record, s_record);
    strcpy(l->field, field);
    l->index = index;
    l->size = size;
    l->value = value;
}

static void __stdcall RecBegin(int, const char* name)
{
    strcpy(s_record, name);
}

static void __stdcall RecInt(const char* field, int index, int size, unsigned int value)
{
    AddLine(field, index, size, value);
}

static void __stdcall RecBytes(const char* field, int index, const void*, int size)
{
    AddLine(field, index, -size, 0);
}

static void __stdcall RecHandle(const char* field, int index, int, unsigned int value)
{
    AddLine(field, index, 0, value);
}

static void __stdcall RecRun(int) {}

static const Line* Find(const char* record, const char* field, int index)
{
    for (int i = 0; i < s_lineCount; i++)
        if (strcmp(s_lines[i].record, record) == 0 && strcmp(s_lines[i].field, field) == 0 && s_lines[i].index == index)
            return &s_lines[i];
    return nullptr;
}

static void Dump(void (__stdcall* writeState)(const StateWriter*))
{
    StateWriter writer = { RecBegin, RecInt, RecBytes, RecHandle, RecRun };
    s_lineCount = 0;
    s_record[0] = 0;
    writeState(&writer);
}

static void CheckStateRecords()
{
    g_game->poolCount = kUnits + 1;
    UnitWeaponSlot* slot = &s_units[kVictim].weapons[1];
    slot->reloadTimer = 7;
    slot->fireHeading = -2;
    slot->aimCob.nAimCobDone = 3;
    Projectile* shots = (Projectile*)SimAlloc(2 * sizeof(Projectile));
    memset(shots, 0, 2 * sizeof(Projectile));
    shots[0].speed = 0x1234;
    shots[1].speed = 0x4321;
    shots[1].flags = 2;
    g_game->projectiles = shots;
    g_game->projectileCount = 2;
    Dump(WeaponsWriteState);
    const Line* reload = Find("weaponsState.units[1]", "weapons[%u].reloadTimer", 1);
    Check(reload != nullptr && reload->size == 2 && reload->value == 7, "the unit's record holds the reload timer of its slot");
    const Line* heading = Find("weaponsState.units[1]", "weapons[%u].fireHeading", 1);
    Check(heading != nullptr && heading->value == 0xfffe, "the unit's record holds the fire heading of its slot as two bytes");
    const Line* done = Find("weaponsState.units[1]", "weapons[%u].aimCob.done", 1);
    Check(done != nullptr && done->size == 4 && done->value == 3, "the unit's record holds the aim callback's done count");
    Check(Find("weaponsState.units[1]", "weapons[%u].firePitch", 2) != nullptr && Find("weaponsState.units[0]", "weapons[%u].firePitch", 0) == nullptr,
          "only the live unit has a record, with all three slots");
    const Line* speed = Find("weaponsState.projectiles[0]", "speed", 0);
    Check(speed != nullptr && speed->value == 0x1234, "a projectile's record holds its speed");
    Check(Find("weaponsState.projectiles[1]", "speed", 0) == nullptr, "a projectile marked dead has no record");
    slot->reloadTimer = 9;
    shots[0].speed = 0x2222;
    Dump(WeaponsWriteState);
    reload = Find("weaponsState.units[1]", "weapons[%u].reloadTimer", 1);
    speed = Find("weaponsState.projectiles[0]", "speed", 0);
    Check(reload != nullptr && reload->value == 9, "tampering with a slot's reload timer changes its record");
    Check(speed != nullptr && speed->value == 0x2222, "tampering with a projectile's speed changes its record");
    g_game->projectileCount = 0;
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
    CheckStateRecords();
    printf("tampered tick %d, first differing tick %d, %d differing ticks, death at tick %d against tick %d\n", tampered.tampered, first, differing, plainDeath, tamperedDeath);

    printf(g_failures ? "FAILED\n" : "weapons_writer_test: all passed\n");
    return g_failures != 0;
}
