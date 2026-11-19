#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <vector>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/ai.h"
#include "../src/state/sim_globals.h"
#include "../src/units/unit_def.h"
#include "../src/units/unit_type_set.h"
#include "../src/orders/mission_type.h"
#include "../src/util/vec3.h"
#pragma pack(push, 1)
#include "../src/map/cell.h"
#include "../src/map/spatial_bucket.h"
#include "../src/game/unit_resources.h"
#include "../src/network/player.h"
#include "../src/units/unit.h"
#include "../src/game/game.h"
#include "../src/ai/squad_manager.h"
#pragma pack(pop)
#include "../src/ai/ai_player.h"
#include "../src/orders/ai_effects.h"
#include "../src/ai/ai_idle.h"

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
int __stdcall RandomInt(int n)
{
    return n > 0 ? n - 1 : 0;
}
void __stdcall RecordSimEvent(SimEventKind, unsigned int, const unsigned int*, unsigned int) {}
void __stdcall HarnessAiEvent(const AiEvent*, int) {}
Unit* __stdcall ScheduleTargetingTargetUnit(Unit*, int)
{
    return 0;
}

enum {
    kTicks = 16,
    kDropTick = 4,
    kRedirectTick = 10,
    kUnits = 6,
    kAi0 = 1,
    kAi1 = 2,
    kEnemy0 = 3,
    kEnemy1 = 4,
    kHitter = 5,
    kPlayers = 2
};

struct OrderRecord {
    int stops;
};

static Unit* s_units;
static UnitDef* s_defs;
static UnitTypeSet* s_noTypes;
static SquadManager* s_managers[kPlayers];
static OrderRecord s_records[kUnits + 1];
static int s_tick;
static int s_dropKind;
static int s_dropTick;
static int s_dropped;
static int s_redirectTick;
static int s_redirected;
static int s_applied;
static unsigned long long s_effects;
static int s_effectCount;

static unsigned long long Fnv(unsigned long long h, const void* p, unsigned int n)
{
    const unsigned char* b = (const unsigned char*)p;
    for (unsigned int i = 0; i < n; i++)
        h = (h ^ b[i]) * 0x100000001b3ull;
    return h;
}

static const unsigned long long kFnvBasis = 0xcbf29ce484222325ull;

static int OtherAi(unsigned int offset)
{
    return offset == Ref<Unit>(&s_units[kAi0]).offset ? kAi1 : kAi0;
}

void __stdcall SchedPublish(int outbox, const void* data, unsigned int size)
{
    AiEvent event;
    memcpy(&event, data, size < sizeof event ? size : sizeof event);
    if (outbox == SCHED_AI && event.kind == AI_EFFECT_STOP && s_tick == s_redirectTick && s_redirected < 0) {
        s_redirected = s_tick;
        event.unit = Ref<Unit>(&s_units[OtherAi(event.unit)]).offset;
    }
    s_effects = Fnv(s_effects, &outbox, sizeof outbox);
    s_effects = Fnv(s_effects, &event, sizeof event);
    s_effectCount++;
    if (outbox != SCHED_AI)
        return;
    if (event.kind == AI_EFFECT_STOP)
        OrdersOnAiStop(&event);
}

static int IndexOf(Unit* unit)
{
    return (int)(unit - s_units);
}

static void __stdcall FakeDeleteOrders(Unit* unit, int)
{
    s_records[IndexOf(unit)].stops++;
}

static void SetUnit(int i, int player, int defIndex, unsigned int flags2)
{
    Unit* u = &s_units[i];
    s_defs[i].flags1 = 0x10000000;
    s_defs[i].flags2 = flags2;
    s_defs[i].categories = s_noTypes;
    for (int w = 0; w < 3; w++)
        s_defs[i].weaponCategories[w] = s_noTypes;
    u->def = &s_defs[i];
    u->unitDefIndex = (unsigned short)defIndex;
    u->player = &g_game->players[player];
    u->playerIndex = (unsigned char)player;
    u->id = (unsigned short)i;
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
    for (int p = 0; p < kPlayers; p++) {
        g_game->players[p].active = 1;
        g_game->players[p].type = 2;
        g_game->players[p].index = (unsigned char)p;
        s_managers[p] = (SquadManager*)SimAlloc(sizeof(SquadManager));
        memset((void*)s_managers[p], 0, sizeof(SquadManager));
        g_game->players[p].ai = s_managers[p];
    }
    SetUnit(kAi0, 0, 10, 0x1000);
    SetUnit(kAi1, 0, 11, 0);
    SetUnit(kEnemy0, 1, 12, 0);
    SetUnit(kEnemy1, 1, 13, 0);
    SetUnit(kHitter, 1, 14, 0);

    static OrdersSeam orders;
    memset(&orders, 0, sizeof orders);
    orders.DeleteOrders = FakeDeleteOrders;
    g_seams.orders = &orders;
}

static void Restart()
{
    for (int i = kAi0; i <= kHitter; i++) {
        s_units[i].flags = 0x10000000 | 0x40000 | 0x100000;
        s_units[i].netDirtyFlags = 0;
    }
    for (int p = 0; p < kPlayers; p++)
        s_managers[p]->nextAction = 0;
    memset(s_records, 0, sizeof s_records);
    g_game->ticks = 0;
}

static int __stdcall TamperedApply(const AiEvent* e)
{
    s_applied++;
    if (s_dropKind != 0 && s_tick == s_dropTick && e->kind == s_dropKind && s_dropped < 0) {
        s_dropped = s_tick;
        return 0;
    }
    return AiApply(e);
}

struct Run {
    unsigned long long state[kTicks];
    unsigned long long effects[kTicks];
    int dropped;
    int redirected;
    int applied;
    int effectCount;
    OrderRecord records[kUnits + 1];
    int nextAction[kPlayers];
};

static unsigned long long HashState()
{
    unsigned long long h = Fnv(kFnvBasis, s_records, sizeof s_records);
    for (int p = 0; p < kPlayers; p++)
        h = Fnv(h, &s_managers[p]->nextAction, sizeof s_managers[p]->nextAction);
    for (int i = kAi0; i <= kHitter; i++) {
        h = Fnv(h, &s_units[i].netDirtyFlags, sizeof s_units[i].netDirtyFlags);
        h = Fnv(h, &s_units[i].flags, sizeof s_units[i].flags);
    }
    h = Fnv(h, &g_aiCommandsEnabled, sizeof g_aiCommandsEnabled);
    unsigned long long parts[2] = { h, s_effects };
    return Fnv(kFnvBasis, parts, sizeof parts);
}

static AiEvent Event(int kind, int unit)
{
    AiEvent e;
    memset(&e, 0, sizeof e);
    e.kind = kind;
    e.unit = Ref<Unit>(&s_units[unit]).offset;
    return e;
}

static AiEvent Attacked(int unit, int attacker)
{
    AiEvent e = Event(AI_REACT_TO_ATTACK, unit);
    e.target = Ref<Unit>(&s_units[attacker]).offset;
    e.value = 10;
    return e;
}

static int Offer(const AiEvent& e, bool direct)
{
    return direct ? AiApply(&e) : g_seams.ai->Apply(&e);
}

static void Play(Run* run, int dropKind, int dropTick, int redirectTick, bool direct)
{
    Restart();
    memset(run, 0, sizeof *run);
    s_dropKind = dropKind;
    s_dropTick = dropTick;
    s_redirectTick = redirectTick;
    s_dropped = -1;
    s_redirected = -1;
    s_applied = 0;
    s_effects = kFnvBasis;
    s_effectCount = 0;
    for (s_tick = 0; s_tick < kTicks; s_tick++) {
        g_game->ticks = s_tick;
        if (s_tick == 0) {
            AiEvent plan = Event(AI_PLAN, kAi0);
            plan.unit = 0;
            plan.value = 8;
            Check(Offer(plan, direct) == 1, "a plan for any difficulty has outcome 1");
        }
        int victim = s_tick % 2 == 0 ? kAi0 : kAi1;
        int attacker = s_tick % 3 == 0 ? kEnemy0 : kEnemy1;
        int expected = (!direct && dropKind == AI_REACT_TO_ATTACK && s_tick == dropTick) ? 0 : 1;
        Check(Offer(Attacked(victim, attacker), direct) == expected, "an attack on a unit has outcome 1 unless the tampered module drops it");
        run->effects[s_tick] = s_effects;
        run->state[s_tick] = HashState();
    }
    run->dropped = s_dropped;
    run->redirected = s_redirected;
    run->applied = s_applied;
    run->effectCount = s_effectCount;
    memcpy(run->records, s_records, sizeof s_records);
    for (int p = 0; p < kPlayers; p++)
        run->nextAction[p] = s_managers[p]->nextAction;
}

static int FirstDiffering(const unsigned long long* a, const unsigned long long* b, int* differing)
{
    int first = -1;
    *differing = 0;
    for (int t = 0; t < kTicks; t++) {
        if (a[t] != b[t]) {
            if (first < 0)
                first = t;
            (*differing)++;
        }
    }
    return first;
}

static void CheckRejected()
{
    unsigned long long before = HashState();
    int effects = s_effectCount;
    AiEvent noUnit = Event(AI_REACT_TO_ATTACK, kAi0);
    noUnit.unit = 0;
    AiEvent badSquad = Event(AI_SET_SQUAD, kAi0);
    badSquad.value = 10;
    AiEvent badPlan = Event(AI_PLAN, kAi0);
    badPlan.value = 16;
    AiEvent badKind = Event(255, kAi0);
    AiEvent squadBelow = Event(AI_SET_SQUAD, kAi0);
    squadBelow.value = -2;
    AiEvent noSquads = Event(AI_SET_SQUAD, kAi0);
    noSquads.value = 0;
    Check(g_seams.ai->Apply(&noUnit) == 0, "a reaction with a null unit has outcome 0");
    Check(g_seams.ai->Apply(&badSquad) == 0, "a squad above 9 has outcome 0");
    Check(g_seams.ai->Apply(&badPlan) == 0, "a plan with a bit above 3 has outcome 0");
    Check(g_seams.ai->Apply(&badKind) == 0, "an event of an unknown kind has outcome 0");
    Check(g_seams.ai->Apply(&squadBelow) == 0, "a squad below -1 has outcome 0");
    Check(g_seams.ai->Apply(&noSquads) == 0, "a squad for a unit whose player has no squads has outcome 0");
    Check(HashState() == before, "a rejected event changes no hashed state");
    Check(s_effectCount == effects, "a rejected event publishes no effect");
}

static int s_standardApplies;
static int s_listRefreshes;
static int s_squadRuns;

static int __stdcall CountApply(const AiEvent*)
{
    s_standardApplies++;
    return 1;
}

static void __stdcall CountListRefresh(int)
{
    s_listRefreshes++;
}

static void __stdcall CountSquadRun(int)
{
    s_squadRuns++;
}

static void CheckIdleSeam()
{
    AiSeam standard;
    memset(&standard, 0, sizeof standard);
    standard.Apply = CountApply;
    standard.UpdatePlayerAI = CountListRefresh;
    standard.UpdateSquads = CountSquadRun;
    AiSeam* idle = AiIdleSeam(&standard);
    for (int p = 0; p < kPlayers; p++) {
        idle->UpdatePlayerAI(p);
        idle->UpdateSquads(p);
    }
    Check(s_listRefreshes == 0, "the idle module's UpdatePlayerAI runs nothing");
    Check(s_squadRuns == 0, "the idle module runs no squads");

    AiEvent computer = Attacked(kAi0, kEnemy0);
    Check(idle->Apply(&computer) == 0 && s_standardApplies == 0, "the idle module rejects a reaction of a computer player's unit");
    g_game->players[0].type = 1;
    Check(idle->Apply(&computer) == 1 && s_standardApplies == 1, "the idle module passes a reaction of a human player's unit on");
    g_game->players[0].type = 2;
    AiEvent plan = Event(AI_PLAN, kAi0);
    Check(idle->Apply(&plan) == 1 && s_standardApplies == 2, "the idle module passes other events on");
    AiEvent noUnit = Event(AI_REACT_TO_ATTACK, kAi0);
    noUnit.unit = 0;
    Check(idle->Apply(&noUnit) == 1 && s_standardApplies == 3, "the idle module passes a reaction with no unit on");
}

int main()
{
    Setup();
    AiSeam seam;
    memset(&seam, 0, sizeof seam);
    seam.Apply = TamperedApply;
    g_seams.ai = &seam;

    static Run plain, again, direct, noReact, redirected;
    Play(&plain, 0, -1, -1, false);
    Play(&again, 0, -1, -1, false);
    Play(&direct, 0, -1, -1, true);
    Play(&noReact, AI_REACT_TO_ATTACK, kDropTick, -1, false);
    Play(&redirected, 0, -1, kRedirectTick, false);

    Check(plain.dropped == -1 && plain.redirected == -1, "the default module drops and redirects nothing");
        Check(plain.records[kAi0].stops > 0 && plain.records[kAi1].stops == 0, "only the unit that flags the stop is stopped");
    Check(plain.nextAction[0] != 0, "a reaction sets the next action of the player's AI");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same hashes at every tick");
    Check(memcmp(plain.state, direct.state, sizeof plain.state) == 0, "a call through the seam hashes as a direct call of AiApply at every tick");
    Check(memcmp(plain.records, direct.records, sizeof plain.records) == 0, "a call through the seam leaves the records a direct call of AiApply leaves");
    Check(noReact.dropped == kDropTick, "the tampered module drops the reaction at the tampered tick");
    Check(redirected.redirected == kRedirectTick, "the tampered module redirects the stop at the tampered tick");
    Check(noReact.applied == plain.applied && redirected.applied == plain.applied, "the tampered modules are offered the same events as the default");
    Check(noReact.effectCount < plain.effectCount, "the dropped reaction publishes fewer effects");
    Check(redirected.effectCount == plain.effectCount, "the redirected stop publishes the same number of effects");

    int dropDiffering = 0, redirectDiffering = 0, redirectEffects = 0;
    int firstDrop = FirstDiffering(plain.state, noReact.state, &dropDiffering);
    int firstRedirect = FirstDiffering(plain.state, redirected.state, &redirectDiffering);
    FirstDiffering(plain.effects, redirected.effects, &redirectEffects);
    Check(firstDrop == kDropTick, "dropping a reaction differs first at the tampered tick");
    Check(dropDiffering == kTicks - kDropTick, "dropping a reaction differs at every tick from the tampered tick on");
    Check(firstRedirect == kRedirectTick, "redirecting a stop differs first at the tampered tick");
    Check(redirectDiffering == kTicks - kRedirectTick, "redirecting a stop differs at every tick from the tampered tick on");
    Check(redirectEffects == kTicks - kRedirectTick, "redirecting a stop changes the published effects from the tampered tick on");
    printf("tampered ticks drop %d redirect %d, first differing tick drop %d redirect %d, differing ticks drop %d redirect %d (redirect effects %d)\n",
        noReact.dropped, redirected.redirected, firstDrop, firstRedirect, dropDiffering, redirectDiffering, redirectEffects);

    CheckRejected();
    s_dropKind = 0;
    s_redirectTick = -1;
    CheckIdleSeam();

    printf(g_failures ? "FAILED\n" : "ai_writer_test: all passed\n");
    return g_failures != 0;
}
