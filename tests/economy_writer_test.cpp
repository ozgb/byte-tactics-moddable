#include <stdio.h>
#include <string.h>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../src/game/economy.h"
#include "../src/harness/economy_results.h"
#pragma pack(push, 1)
#include "../src/game/unit_resources.h"
#include "../src/network/player.h"
#include "../src/game/game.h"
#pragma pack(pop)

void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}

void (*g_outOfMemoryHandler)() = 0;

int HarnessTestMode()
{
    return 0;
}

extern "C" void HOST_CALL host_fatal(void*, const char* title, const char* text)
{
    printf("FAIL: host_fatal: %s: %s\n", title, text);
}

struct ConstructedElement {
    ConstructedElement();
    ~ConstructedElement();
    int field_0;
};
ConstructedElement::ConstructedElement() {}
ConstructedElement::~ConstructedElement() {}

Game* g_game;
Seams g_seams;

static int g_failures;

static void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

enum { kUnits = 4, kPlayers = 2, kTicks = 90, kUnitTag = 2 };

static unsigned int s_tick;
static EconomyResults s_results;

static unsigned int __stdcall WTick() { return s_tick; }
static int __stdcall WDifficulty() { return 1; }
static int __stdcall WSharingEnabled() { return 0; }
static float __stdcall WWind() { return 7.0f; }
static float __stdcall WTide() { return 3.0f; }
static void __stdcall WPlayerInfo(int player, EconPlayerInfo* out)
{
    memset(out, 0, sizeof *out);
    out->active = player < kPlayers;
    out->controller = player == 0 ? 1 : 2;
    out->index = player;
    out->inGame = 1;
    out->storage = 1;
}
static int __stdcall WAllied(int from, int to) { return from == to; }
static EconUnit __stdcall WFirstUnit(int player) { return player == 0 ? 1 : player == 1 ? 3 : 1; }
static EconUnit __stdcall WLastUnit(int player) { return player == 0 ? 2 : player == 1 ? 4 : 0; }
static EconUnit __stdcall WNextUnit(EconUnit unit) { return unit + 1; }
static int __stdcall WUnitSlot(EconUnit unit) { return (int)unit - 1; }
static int __stdcall WUnitInfo(EconUnit unit, EconUnitInfo* out)
{
    if (unit < 1 || unit > kUnits)
        return 0;
    memset(out, 0, sizeof *out);
    out->player = unit <= 2 ? 0 : 1;
    out->type = unit % 2;
    out->structure = 1;
    out->active = 1;
    out->extraction = 0.25f;
    out->cellX = (int)unit;
    out->cellY = 1;
    out->footprintX = 2;
    out->footprintY = 2;
    return 1;
}
static void __stdcall WTypeInfo(EconType type, EconTypeInfo* out)
{
    memset(out, 0, sizeof *out);
    if (type == 0) {
        out->energyMake = 20.0f;
        out->energyStorage = 100.0f;
        out->energyUse = 1.5f;
    } else {
        out->windGenerator = 0.7f;
        out->metalMake = 0.1f;
        out->metalStorage = 50.0f;
        out->energyUse = 0.3f;
    }
}
static int __stdcall WCellMetal(int, int) { return 10; }
static void __stdcall WEmit(const EconomyEvent* effect)
{
    EconomyResultsAdd(&s_results, effect, 1);
}

static EconomyWorldView s_world = { WTick, WDifficulty, WSharingEnabled, WWind, WTide, WPlayerInfo, WAllied, WFirstUnit, WLastUnit,
                                    WNextUnit, WUnitSlot, WUnitInfo, WTypeInfo, WCellMetal, WEmit };

static unsigned long long Fnv(unsigned long long h, const void* p, unsigned int n)
{
    const unsigned char* b = (const unsigned char*)p;
    for (unsigned int i = 0; i < n; i++)
        h = (h ^ b[i]) * 0x100000001b3ull;
    return h;
}

static unsigned long long FnvU32(unsigned long long h, unsigned int v)
{
    return Fnv(h, &v, 4);
}

static unsigned long long s_hash;
static int s_lines;

static void __stdcall WriterBegin(int section, const char*)
{
    s_hash = FnvU32(s_hash, 0xffff0000u + section);
}

static void __stdcall WriterInt(const char*, int, int size, unsigned int value)
{
    s_hash = Fnv(s_hash, &value, size);
    s_lines++;
}

static void __stdcall WriterBytes(const char*, int, const void* data, int size)
{
    s_hash = Fnv(s_hash, data, size);
    s_lines++;
}

static void __stdcall WriterHandle(const char*, int, int kind, unsigned int value)
{
    s_hash = FnvU32(FnvU32(s_hash, kind), value);
    s_lines++;
}

static void __stdcall WriterRun(int) {}

static const StateWriter s_writer = { WriterBegin, WriterInt, WriterBytes, WriterHandle, WriterRun };

static unsigned long long HashWritten()
{
    s_hash = 0xcbf29ce484222325ull;
    s_lines = 0;
    EconomyWriteState(&s_writer);
    return s_hash;
}

static void* s_region;
static unsigned int s_regionSize;

static unsigned long long HashRegion()
{
    return Fnv(0xcbf29ce484222325ull, s_region, s_regionSize);
}

static EconMapInfo Map()
{
    EconMapInfo map = { kUnits, kPlayers };
    return map;
}

static void Restart()
{
    if (s_region != 0)
        SimFree(EconomyShutdown());
    for (int p = 0; p < kPlayers; p++) {
        memset(g_game->players[p].econ.get(), 0, sizeof(UnitResources));
        g_game->players[p].active = 1;
        g_game->players[p].energy = 500.0f;
        g_game->players[p].metal = 300.0f;
    }
    EconMapInfo map = Map();
    s_regionSize = EconomyRegionSize(&map);
    s_region = SimAlloc(s_regionSize);
    memset(s_region, 0xa5, s_regionSize);
    EconomyInit(&map, &s_world, s_region, s_regionSize);
    s_tick = 0;
    EconomyResultsClear(&s_results);
}

static void Setup()
{
    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    for (int p = 0; p < kPlayers; p++)
        g_game->players[p].econ = (UnitResources*)SimAlloc(sizeof(UnitResources));
    s_region = 0;
    Restart();
}

static EconomyEvent Event(int kind, int player, EconUnit unit, unsigned int resources, float energy, float metal)
{
    EconomyEvent e;
    memset(&e, 0, sizeof e);
    e.kind = kind;
    e.player = player;
    e.unit = unit;
    e.resources = resources;
    e.energy = energy;
    e.metal = metal;
    return e;
}

struct Run {
    unsigned long long written[kTicks];
    unsigned long long results[kTicks];
    unsigned long long region[kTicks];
    int tampered;
    int granted;
    int spent;
};

static int Scripted(EconomyEvent* out, int index, int tick)
{
    EconUnit unit = (EconUnit)(tick % kUnits + 1);
    int player = unit <= 2 ? 0 : 1;
    switch (index) {
    case 0:
        *out = Event(ECONOMY_REQUEST, player, unit, ECON_ENERGY | ECON_METAL, 1.25f + (float)(tick % 5), 0.5f + (float)(tick % 3));
        return 1;
    case 1:
        *out = Event(ECONOMY_SPEND, player, unit, ECON_ENERGY | ECON_METAL, 20.0f + (float)(tick % 7), 6.0f + (float)(tick % 4));
        return 1;
    case 2:
        *out = Event(ECONOMY_INCOME, player, unit, ECON_METAL, 0.0f, 2.5f);
        return 1;
    case 3:
        *out = Event(ECONOMY_INCOME, player, 0, ECON_ENERGY, 3.0f, 0.0f);
        return 1;
    }
    return 0;
}

static void Play(Run* run, bool tamper)
{
    Restart();
    run->tampered = -1;
    run->granted = 0;
    run->spent = 0;
    for (int p = 0; p < kPlayers; p++) {
        EconomyEvent reset = Event(ECONOMY_RESET, p, 0, 0, 0, 0);
        EconomyApply(&reset);
    }
    for (EconUnit u = 1; u <= kUnits; u++) {
        EconomyEvent reset = Event(ECONOMY_RESET, u <= 2 ? 0 : 1, u, 0, 0, 0);
        EconomyApply(&reset);
    }
    for (int t = 0; t < kTicks; t++) {
        s_tick++;
        EconomyResultsClear(&s_results);
        EconomyEvent e;
        for (int i = 0; Scripted(&e, i, t); i++) {
            if (e.kind == ECONOMY_REQUEST) {
                int granted = EconomyRequestGranted(e.unit, e.resources);
                int outcome = EconomyApply(&e);
                Check(outcome == granted, "RequestGranted says what the REQUEST then returns");
                run->granted += outcome;
                EconomyResultsAdd(&s_results, &e, outcome);
                continue;
            }
            int outcome = EconomyApply(&e);
            if (e.kind == ECONOMY_SPEND)
                run->spent += outcome;
            EconomyEvent seen = e;
            if (tamper && run->tampered < 0 && e.kind == ECONOMY_SPEND && outcome == 1 && t >= 40) {
                seen.energy += 1.0f;
                run->tampered = t;
            }
            EconomyResultsAdd(&s_results, &seen, outcome);
        }
        EconomyUpdatePlayer(0);
        EconomyUpdatePlayer(1);
        run->results[t] = EconomyResultsHash(&s_results, 0xcbf29ce484222325ull);
        run->written[t] = HashWritten();
        run->region[t] = HashRegion();
    }
}

static void CheckResultsSection()
{
    static Run plain, again, tampered;
    Play(&plain, false);
    Play(&again, false);
    Play(&tampered, true);
    Check(plain.granted > 0 && plain.spent > 0, "requests are granted and spends succeed");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same results, written state and region");
    Check(tampered.tampered >= 0, "the tampered pilot changes an event");
    Check(memcmp(plain.written, tampered.written, sizeof plain.written) == 0,
          "the tampered pilot reports the default's state every tick");
    int first = -1;
    for (int t = 0; t < kTicks && first < 0; t++) {
        if (plain.results[t] != tampered.results[t])
            first = t;
    }
    Check(first == tampered.tampered, "the economy events section differs first at the tick of the changed event");
    printf("tampered tick %d, first differing tick %d\n", tampered.tampered, first);
    Check(plain.region[0] != plain.region[kTicks - 1], "the events change the region");
}

int main()
{
    Setup();
    static Run warm;
    Play(&warm, false);
    unsigned long long region = HashRegion();
    unsigned long long block = Fnv(0, SimBlockBase(), SimBlockSize());
    unsigned long long first = HashWritten();
    Check(HashRegion() == region, "WriteState writes nothing into the region");
    Check(Fnv(0, SimBlockBase(), SimBlockSize()) == block, "WriteState writes nothing into the state block");
    Check(HashWritten() == first, "WriteState writes the same state twice");
    Check(s_lines > 4 * 2, "WriteState writes the accounts of the live units");

    EconomyEvent e = Event(ECONOMY_INCOME, 0, 1, ECON_METAL, 0.0f, 1.0f);
    unsigned long long before = HashWritten();
    EconomyApply(&e);
    Check(HashWritten() != before, "an event to a unit's account changes the written state");

    CheckResultsSection();

    printf(g_failures ? "FAILED\n" : "economy_writer_test: all passed\n");
    return g_failures != 0;
}
