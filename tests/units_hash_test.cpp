#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/units.h"
#include "../interfaces/movement.h"
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
#include "../src/game/movement_events.h"
#include "../src/seams/units_world.h"
#include "../src/units/units_seam.h"

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

void __stdcall HarnessUnitsEvent(const UnitsEventHeader*, unsigned int, int) {}

enum {
    kTicks = 90,
    kTamperTick = 20,
    kUnit = 1,
    kUnits = 1
};

static Unit* s_units;
static int s_tamperAt = -1;
static int s_tick;
static int s_dropped = -1;
static int s_applied;

static int __stdcall PlainApply(const UnitsEventHeader* e, unsigned int size)
{
    s_applied++;
    return UnitsApply(e, size);
}

static int __stdcall TamperedApply(const UnitsEventHeader* e, unsigned int size)
{
    if (e->kind == UNITS_MOVE && s_tick == s_tamperAt && s_dropped < 0) {
        s_dropped = s_tick;
        return 0;
    }
    return PlainApply(e, size);
}

static void Setup()
{
    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    memset(g_game, 0, sizeof(Game));
    s_units = (Unit*)SimAlloc((kUnits + 1) * sizeof(Unit));
    memset((void*)s_units, 0, (kUnits + 1) * sizeof(Unit));
    g_game->units = s_units;
    g_game->unitsEnd = &s_units[kUnits];
    s_units[kUnit].id = kUnit;
}

struct Run {
    int x[kTicks];
    int z[kTicks];
    int tampered;
    int applied;
};

static void Play(Run* run, bool tamper)
{
    s_units[kUnit].pos.x = 0;
    s_units[kUnit].pos.y = 0;
    s_units[kUnit].pos.z = 0;
    s_dropped = -1;
    s_applied = 0;
    s_tamperAt = tamper ? kTamperTick : -1;
    for (int t = 0; t < kTicks; t++) {
        s_tick = t;
        g_game->ticks = t;
        MovementMove e;
        memset(&e, 0, sizeof e);
        e.h = MovementHeaderOf(MOVEMENT_EFFECT_MOVE, &s_units[kUnit]);
        e.pos[0] = s_units[kUnit].pos.x + 65536 + t * 31;
        e.pos[1] = 0;
        e.pos[2] = s_units[kUnit].pos.z + 3 * 65536 - t * 17;
        UnitsOnMovementMove(&e, sizeof e);
        run->x[t] = s_units[kUnit].pos.x;
        run->z[t] = s_units[kUnit].pos.z;
    }
    run->tampered = s_dropped;
    run->applied = s_applied;
}

int main()
{
    Setup();
    static UnitsSeam plainSeam, tamperedSeam;
    memset(&plainSeam, 0, sizeof plainSeam);
    plainSeam.Apply = PlainApply;
    tamperedSeam = plainSeam;
    tamperedSeam.Apply = TamperedApply;

    static Run plain, again, tampered;
    g_seams.units = &plainSeam;
    Play(&plain, false);
    Play(&again, false);
    g_seams.units = &tamperedSeam;
    Play(&tampered, true);

    Check(plain.tampered < 0, "the default drops no event");
    Check(plain.applied == kTicks, "the default applies one event per tick");
    Check(plain.x[0] == 65536 && plain.x[1] == 2 * 65536 + 31, "a move event sets the unit's position");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same position at every tick");
    Check(tampered.tampered == kTamperTick, "the tampered module drops a move event at the tampered tick");
    Check(tampered.applied == plain.applied - 1, "the tampered module drops exactly one event");
    int first = -1;
    int differing = 0;
    for (int t = 0; t < kTicks; t++)
        if (plain.x[t] != tampered.x[t] || plain.z[t] != tampered.z[t]) {
            if (first < 0)
                first = t;
            differing++;
        }
    Check(first == kTamperTick, "the unit's position differs first at the tampered tick");
    Check(differing == kTicks - kTamperTick, "the unit's position differs at every tick from the tampered tick on");
    printf("tampered tick %d, first differing tick %d, %d differing ticks\n", tampered.tampered, first, differing);

    printf(g_failures ? "FAILED\n" : "units_hash_test: all passed\n");
    return g_failures != 0;
}
