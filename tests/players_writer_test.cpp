#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <vector>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/players.h"
#include "../src/state/sim_globals.h"
#include "../src/units/unit_def.h"
#include "../src/util/vec3.h"
#include "../src/util/tdf.h"
#pragma pack(push, 1)
#include "../src/map/cell.h"
#include "../src/map/spatial_bucket.h"
#include "../src/game/unit_resources.h"
#include "../src/network/player.h"
#include "../src/units/unit.h"
#include "../src/game/game.h"
#pragma pack(pop)
#include "../src/game/mission_conditions.h"
#include "../src/game/victory_conditions.h"
#include "../src/game/players.h"

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
void __cdecl GameFreeThunk(void*) {}
int TdfRecord::GetFieldInt(const char*, int fallback)
{
    return fallback;
}
int __stdcall ScheduleEconomySpend(const EconomyEvent*)
{
    return 1;
}
void __stdcall RecordSimEvent(SimEventKind, unsigned int, const unsigned int*, unsigned int) {}
void __stdcall HarnessPlayersEvent(const PlayersEvent*, int) {}

enum {
    kTicks = 20,
    kGiveTamperTick = 3,
    kDeathTamperTick = 9,
    kUnits = 8,
    kSolar = 1,
    kPyro1 = 2,
    kPyro2 = 3,
    kCommander = 4,
    kFlash1 = 5,
    kFlashes = 3,
    kPlayers = 3,
    kEffectsMax = 64
};

static Unit* s_units;
static UnitDef* s_defs;
static MissionConditions* s_conditions;
static VictoryCaptureUnitType* s_capture;
static VictoryKillUnitType* s_killType;
static DefeatUnitTypeKilled* s_defeatType;
static int s_tick;
static int s_tamperKind;
static int s_tamperTick;
static int s_dropped;
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

void __stdcall SchedPublish(int outbox, const void* data, unsigned int size)
{
    s_effects = Fnv(s_effects, &outbox, sizeof outbox);
    if (size == sizeof(PlayersEvent)) {
        const PlayersEvent* e = (const PlayersEvent*)data;
        s_effects = Fnv(s_effects, &e->kind, sizeof e->kind);
        s_effects = Fnv(s_effects, &e->unit, sizeof e->unit);
        s_effects = Fnv(s_effects, &e->player, sizeof e->player);
        s_effects = Fnv(s_effects, &e->value, sizeof e->value);
        s_effects = Fnv(s_effects, &e->record, sizeof e->record);
    } else {
        s_effects = Fnv(s_effects, data, size);
    }
    s_effectCount++;
}

static void SetUnit(int i, const char* type, int player)
{
    Unit* u = &s_units[i];
    strcpy(s_defs[i].unitname, type);
    u->def = &s_defs[i];
    u->unitDefIndex = (unsigned short)i;
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
    g_game->units = s_units;
    g_game->unitsEnd = &s_units[kUnits];

    for (int p = 0; p < kPlayers; p++) {
        g_game->players[p].active = 1;
        g_game->players[p].type = p == 0 ? 1 : (p == 1 ? 2 : 3);
        g_game->players[p].index = (unsigned char)p;
    }
    g_game->players[0].unitsBegin = &s_units[kFlash1];
    g_game->players[0].unitsEnd = &s_units[kFlash1 + kFlashes - 1];
    g_game->players[1].unitsBegin = &s_units[kSolar];
    g_game->players[1].unitsEnd = &s_units[kCommander];

    SetUnit(kSolar, "ARMSOLAR", 1);
    SetUnit(kPyro1, "CORPYR", 1);
    SetUnit(kPyro2, "CORPYR", 1);
    SetUnit(kCommander, "CORCOM", 1);
    for (int i = 0; i < kFlashes; i++)
        SetUnit(kFlash1 + i, "ARMFLASH", 0);

    s_conditions = new (SimAlloc(sizeof(MissionConditions))) MissionConditions();
    s_capture = (VictoryCaptureUnitType*)SimAlloc(sizeof(VictoryCaptureUnitType));
    s_killType = (VictoryKillUnitType*)SimAlloc(sizeof(VictoryKillUnitType));
    s_defeatType = (DefeatUnitTypeKilled*)SimAlloc(sizeof(DefeatUnitTypeKilled));
    g_game->list_391ed = s_conditions;
}

static void Restart()
{
    for (int i = kSolar; i <= kFlash1 + kFlashes - 1; i++)
        s_units[i].flags = 0x10000000;
    g_game->players[0].unitCount = kFlashes;
    g_game->players[1].unitCount = 4;
    s_conditions = new (s_conditions) MissionConditions();
    s_capture = new (s_capture) VictoryCaptureUnitType("ARMSOLAR");
    s_killType = new (s_killType) VictoryKillUnitType("CORPYR", 100);
    s_defeatType = new (s_defeatType) DefeatUnitTypeKilled("ARMFLASH", 100);
    s_conditions->victory[0] = s_capture;
    s_conditions->victory[1] = s_killType;
    s_conditions->victoryCount = 2;
    s_conditions->defeat[0] = s_defeatType;
    s_conditions->defeatCount = 1;
    g_game->list_391ed = s_conditions;
}

static int __stdcall TamperedApply(const PlayersEvent* e)
{
    s_applied++;
    if (s_tamperKind != 0 && s_tick == s_tamperTick && e->kind == s_tamperKind && s_dropped < 0) {
        s_dropped = s_tick;
        return 0;
    }
    return PlayersApply(e);
}

struct Run {
    unsigned long long state[kTicks];
    unsigned long long conditions[kTicks];
    unsigned long long effects[kTicks];
    int dropped;
    int applied;
    int effectCount;
    int satisfied;
    int leftToKill;
};

static unsigned long long HashConditions()
{
    unsigned long long h = kFnvBasis;
    MissionCondition* all[3] = { s_capture, s_killType, s_defeatType };
    for (int i = 0; i < 3; i++) {
        h = Fnv(h, &all[i]->satisfied, sizeof all[i]->satisfied);
        h = Fnv(h, &all[i]->celebrated, sizeof all[i]->celebrated);
    }
    h = Fnv(h, &s_killType->numLeftToKill, sizeof s_killType->numLeftToKill);
    h = Fnv(h, &s_defeatType->numLeftToKill, sizeof s_defeatType->numLeftToKill);
    h = Fnv(h, &s_conditions->active, sizeof s_conditions->active);
    return h;
}

static unsigned long long HashPlayers()
{
    return Fnv(kFnvBasis, &g_game->players[0], kPlayers * sizeof(Player));
}

static unsigned long long HashState()
{
    unsigned long long parts[3] = { HashConditions(), HashPlayers(), s_effects };
    return Fnv(kFnvBasis, parts, sizeof parts);
}

static int Offer(const PlayersEvent& e)
{
    return g_seams.players->Apply(&e);
}

static PlayersEvent Died(int unit)
{
    PlayersEvent e;
    memset(&e, 0, sizeof e);
    e.kind = PLAYERS_UNIT_DIED;
    e.unit = Ref<Unit>(&s_units[unit]).offset;
    return e;
}

static PlayersEvent Give(int unit, int player)
{
    PlayersEvent e;
    memset(&e, 0, sizeof e);
    e.kind = PLAYERS_GIVE_UNIT;
    e.unit = Ref<Unit>(&s_units[unit]).offset;
    e.player = player;
    return e;
}

static void Play(Run* run, int tamperKind, int tamperTick)
{
    Restart();
    memset(run, 0, sizeof *run);
    s_tamperKind = tamperKind;
    s_tamperTick = tamperTick;
    s_dropped = -1;
    s_applied = 0;
    s_effects = kFnvBasis;
    s_effectCount = 0;
    for (s_tick = 0; s_tick < kTicks; s_tick++) {
        if (s_tick >= 1) {
            Check(Offer(Died(kFlash1 + s_tick % kFlashes)) == (s_tick == kDeathTamperTick && tamperKind == PLAYERS_UNIT_DIED ? 0 : 1),
                "a death of a unit has outcome 1 unless the tampered module drops it");
            Check(Offer(Died(kPyro1 + s_tick % 2)) == 1, "a death of a unit has outcome 1");
        }
        if (s_tick == kGiveTamperTick) {
            Check(Offer(Give(kSolar, 0)) == (tamperKind == PLAYERS_GIVE_UNIT ? 0 : 1), "capturing the solar collector has outcome 1 unless dropped");
        }
        if (s_tick == 7)
            Check(Offer(Give(kPyro1, 2)) == 1, "giving a unit to a player on another machine has outcome 1");
        if (s_tick == 12)
            Check(Offer(Give(kPyro2, 0)) == 1, "giving a unit to a computer player has outcome 1");
        if (s_tick == 15) {
            PlayersEvent kill;
            memset(&kill, 0, sizeof kill);
            kill.kind = PLAYERS_KILL_UNITS;
            kill.player = 1;
            Check(Offer(kill) == 1, "killing a player's units has outcome 1");
        }
        run->conditions[s_tick] = HashConditions();
        run->effects[s_tick] = s_effects;
        run->state[s_tick] = HashState();
    }
    run->dropped = s_dropped;
    run->applied = s_applied;
    run->effectCount = s_effectCount;
    run->satisfied = s_capture->satisfied;
    run->leftToKill = s_defeatType->numLeftToKill;
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
    std::vector<char> units((char*)s_units, (char*)(s_units + kUnits + 1));
    PlayersEvent diedNoUnit = Died(kSolar);
    diedNoUnit.unit = 0;
    PlayersEvent createdNoUnit = diedNoUnit;
    createdNoUnit.kind = PLAYERS_UNIT_CREATED;
    PlayersEvent giveNoUnit = Give(kSolar, 0);
    giveNoUnit.unit = 0;
    PlayersEvent giveBadPlayer = Give(kFlash1, 10);
    PlayersEvent giveOwn = Give(kFlash1, 0);
    PlayersEvent giveDead = Give(kPyro1, 0);
    PlayersEvent killBadPlayer;
    memset(&killBadPlayer, 0, sizeof killBadPlayer);
    killBadPlayer.kind = PLAYERS_KILL_UNITS;
    killBadPlayer.player = 11;
    PlayersEvent transferNone;
    memset(&transferNone, 0, sizeof transferNone);
    transferNone.kind = PLAYERS_TRANSFER;
    transferNone.from = 10;
    PlayersEvent transferZero = transferNone;
    transferZero.from = 0;
    PlayersEvent transferBadKind = transferZero;
    transferBadKind.value = 5;
    transferBadKind.amount = 1.0f;
    PlayersEvent badKind;
    memset(&badKind, 0, sizeof badKind);
    badKind.kind = 127;
    badKind.unit = Ref<Unit>(&s_units[kSolar]).offset;
    s_units[kPyro1].flags = 0x10000000 | 0x4000;
    std::vector<char> withDying((char*)s_units, (char*)(s_units + kUnits + 1));
    Check(Offer(diedNoUnit) == 0, "a death with a null unit has outcome 0");
    Check(Offer(createdNoUnit) == 0, "a creation with a null unit has outcome 0");
    Check(Offer(giveNoUnit) == 0, "a gift of a null unit has outcome 0");
    Check(Offer(giveBadPlayer) == 0, "a gift to a player above 9 has outcome 0");
    Check(Offer(giveOwn) == 0, "a gift of a unit to its own player has outcome 0");
    Check(Offer(giveDead) == 0, "a gift of a dying unit has outcome 0");
    Check(Offer(killBadPlayer) == 0, "killing the units of a player above 10 has outcome 0");
    Check(Offer(transferNone) == 0, "a transfer from player 10 has outcome 0");
    Check(Offer(transferZero) == 0, "a transfer of nothing has outcome 0");
    Check(Offer(transferBadKind) == 0, "a transfer of an unknown resource has outcome 0");
    Check(Offer(badKind) == 0, "an event of an unknown kind has outcome 0");
    Check(HashState() == before, "a rejected event changes no hashed state");
    Check(s_effectCount == effects, "a rejected event publishes no effect");
    Check(memcmp(&withDying[0], s_units, withDying.size()) == 0, "a rejected event changes no unit");
    Check(units.size() == withDying.size(), "the unit snapshots have one size");
}

int main()
{
    Setup();
    PlayersSeam seam;
    memset(&seam, 0, sizeof seam);
    seam.Apply = TamperedApply;
    g_seams.players = &seam;

    static Run plain, again, noDeath, noGive;
    Play(&plain, 0, -1);
    Play(&again, 0, -1);
    Play(&noDeath, PLAYERS_UNIT_DIED, kDeathTamperTick);
    Play(&noGive, PLAYERS_GIVE_UNIT, kGiveTamperTick);

    Check(plain.dropped == -1, "the default module drops nothing");
    Check(plain.satisfied == 1, "capturing the solar collector satisfies the capture condition");
    Check(plain.leftToKill == 100 - (kTicks - 1), "every death reaches the defeat condition");
    Check(plain.effectCount == 7, "the gifts and the kill publish effects");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same hashes at every tick");
    Check(noDeath.dropped == kDeathTamperTick, "the tampered module drops the death at the tampered tick");
    Check(noGive.dropped == kGiveTamperTick, "the tampered module drops the gift at the tampered tick");
    Check(noDeath.applied == plain.applied && noGive.applied == plain.applied, "the tampered module is offered the same events as the default");
    Check(noDeath.satisfied == 1 && noDeath.leftToKill == plain.leftToKill + 1, "the dropped death leaves one more unit to kill");
    Check(noGive.satisfied == 0, "the dropped gift leaves the capture condition unsatisfied");

    int deathDiffering = 0, giveDiffering = 0, giveConditions = 0, giveEffects = 0;
    int firstDeath = FirstDiffering(plain.state, noDeath.state, &deathDiffering);
    int firstGive = FirstDiffering(plain.state, noGive.state, &giveDiffering);
    int firstGiveConditions = FirstDiffering(plain.conditions, noGive.conditions, &giveConditions);
    FirstDiffering(plain.effects, noGive.effects, &giveEffects);
    Check(firstDeath == kDeathTamperTick, "dropping a death differs first at the tampered tick");
    Check(deathDiffering == kTicks - kDeathTamperTick, "dropping a death differs at every tick from the tampered tick on");
    Check(firstGive == kGiveTamperTick, "dropping a gift differs first at the tampered tick");
    Check(giveDiffering == kTicks - kGiveTamperTick, "dropping a gift differs at every tick from the tampered tick on");
    Check(firstGiveConditions == kGiveTamperTick && giveConditions == kTicks - kGiveTamperTick, "dropping a gift changes the conditions from the tampered tick on");
    Check(giveEffects == kTicks - kGiveTamperTick, "dropping a gift changes the published effects from the tampered tick on");
    printf("tampered ticks death %d gift %d, first differing tick death %d gift %d, differing ticks death %d gift %d (gift conditions %d, effects %d)\n",
        noDeath.dropped, noGive.dropped, firstDeath, firstGive, deathDiffering, giveDiffering, giveConditions, giveEffects);

    CheckRejected();

    printf(g_failures ? "FAILED\n" : "players_writer_test: all passed\n");
    return g_failures != 0;
}
