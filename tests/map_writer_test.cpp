#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/los_radar.h"
#include "../interfaces/map.h"
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
#include "../src/game/map_events.h"
#include "../src/map/terrain.h"

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
void __stdcall SchedPublish(int, const void*, unsigned int) {}
void __stdcall RecordSimEvent(SimEventKind, unsigned int, const unsigned int*, unsigned int) {}
void __stdcall HarnessMapEvent(const MapEvent*, int) {}
void __stdcall HarnessPlayersEvent(const PlayersEvent*, int) {}

enum {
    kMapCells = 32,
    kBucketSide = 4,
    kBuckets = kBucketSide * kBucketSide,
    kTicks = 28,
    kTamperTick = 10,
    kUnits = 7,
    kRunner = 1,
    kDriver = 2,
    kWestbound = 3,
    kEastbound = 4,
    kLayer2 = 5,
    kLeaver = 6
};

static Unit* s_units;
static Player* s_playerStorage;
static int s_tick;
static bool s_tamper;
static int s_dropped;
static int s_removals;

static Cell* __stdcall TestGetMapCell(int x, int y)
{
    return &g_game->cells[y * g_game->width + x];
}

static unsigned long long Fnv(unsigned long long h, const void* p, unsigned int n)
{
    const unsigned char* b = (const unsigned char*)p;
    for (unsigned int i = 0; i < n; i++)
        h = (h ^ b[i]) * 0x100000001b3ull;
    return h;
}

static const unsigned long long kFnvBasis = 0xcbf29ce484222325ull;

static int __stdcall TamperedApply(const MapEvent* e)
{
    if (e->kind == MAP_REMOVE_UNIT) {
        s_removals++;
        if (s_tamper && s_tick == kTamperTick && e->unit == Ref<Unit>(&s_units[kRunner]).offset) {
            s_dropped = s_tick;
            return 0;
        }
    }
    return ApplyMapEvent(e);
}

static int Triangle(int t, int period, int span)
{
    int phase = t % period;
    return phase < span ? phase : 2 * span - phase;
}

static void CellAt(int t, int unit, int* x, int* y)
{
    switch (unit) {
    case kRunner:
        *x = 1 + t;
        *y = 3;
        return;
    case kDriver:
        *x = 26;
        *y = 28 - t;
        return;
    case kWestbound:
        *x = 1 + t;
        *y = 20;
        return;
    case kEastbound:
        *x = 27 - t;
        *y = 20;
        return;
    case kLayer2:
        *x = 1 + Triangle(t, 40, 20);
        *y = 24;
        return;
    case kLeaver:
        *x = t < 14 ? 2 + t : 31;
        *y = 12;
        return;
    }
    *x = 0;
    *y = 0;
}

static void Move(int t, int unit)
{
    int x, y;
    CellAt(t, unit, &x, &y);
    Unit* u = &s_units[unit];
    u->cell.x = (short)x;
    u->cell.y = (short)y;
    u->pos.x = (x * 16 + 8) << 16;
    u->pos.y = 0;
    u->pos.z = (y * 16 + 8) << 16;
}

static void Setup()
{
    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    memset(g_game, 0, sizeof(Game));
    g_game->width = kMapCells;
    g_game->height = kMapCells;

    g_game->cells = (Cell*)SimAlloc(kMapCells * kMapCells * sizeof(Cell));
    g_game->cells_1429f = (SpatialBucket*)SimAlloc(kBuckets * sizeof(SpatialBucket));
    g_game->width_142a3 = kBucketSide;
    g_game->height_142a7 = kBucketSide;
    g_game->overflowBucket = (SpatialBucket*)SimAlloc(sizeof(SpatialBucket));

    s_playerStorage = (Player*)SimAlloc(2 * sizeof(Player));
    s_units = (Unit*)SimAlloc(kUnits * sizeof(Unit));
    memset((void*)s_units, 0, kUnits * sizeof(Unit));
    g_game->units = s_units;
    g_game->unitsEnd = &s_units[kUnits - 1];

    for (int p = 0; p < 2; p++) {
        s_playerStorage[p].active = 1;
        s_playerStorage[p].type = p == 0 ? 1 : 3;
        s_playerStorage[p].index = (unsigned char)p;
    }
    for (int i = 1; i < kUnits; i++) {
        Unit* u = &s_units[i];
        u->id = (unsigned short)i;
        u->unitDefIndex = 1;
        u->footprint.x = 2;
        u->footprint.y = 2;
        int p = i == kEastbound ? 1 : 0;
        u->player = &s_playerStorage[p];
        u->playerIndex = (unsigned char)p;
    }
}

static void Restart()
{
    memset(g_game->cells.get(), 0, kMapCells * kMapCells * sizeof(Cell));
    memset(g_game->cells_1429f.get(), 0, kBuckets * sizeof(SpatialBucket));
    memset(g_game->overflowBucket.get(), 0, sizeof(SpatialBucket));
    for (int i = 1; i < kUnits; i++) {
        Unit* u = &s_units[i];
        u->flags = i == kLayer2 ? 2 : 1;
        u->spatialBucket = 0;
        u->cargoNext = 0;
        u->carrier = 0;
    }
}

struct Run {
    unsigned long long state[kTicks];
    unsigned long long cells[kTicks];
    unsigned long long buckets[kTicks];
    unsigned long long conflicts[kTicks];
    int conflictUnits[kTicks];
    int dropped;
    int removals;
};

static unsigned long long HashCells()
{
    unsigned long long h = kFnvBasis;
    for (int i = 0; i < kMapCells * kMapCells; i++) {
        Cell* c = &g_game->cells[i];
        h = Fnv(h, &c->unit, sizeof c->unit);
        h = Fnv(h, &c->unit2, sizeof c->unit2);
    }
    return h;
}

static unsigned long long HashBucket(unsigned long long h, SpatialBucket* b)
{
    for (Unit* u = b->head.get(); u != 0; u = u->cargoNext.get())
        h = Fnv(h, &u->id, sizeof u->id);
    unsigned int end = 0xffffffffu;
    return Fnv(h, &end, sizeof end);
}

static unsigned long long HashBuckets()
{
    unsigned long long h = kFnvBasis;
    for (int i = 0; i < kBuckets; i++)
        h = HashBucket(h, &g_game->cells_1429f[i]);
    h = HashBucket(h, g_game->overflowBucket.get());
    for (int i = 1; i < kUnits; i++) {
        h = Fnv(h, &s_units[i].spatialBucket.offset, sizeof s_units[i].spatialBucket.offset);
        h = Fnv(h, &s_units[i].cargoNext.offset, sizeof s_units[i].cargoNext.offset);
    }
    return h;
}

static unsigned long long HashConflicts(int* units)
{
    unsigned long long h = kFnvBasis;
    *units = 0;
    for (int i = 1; i < kUnits; i++) {
        unsigned int bits = s_units[i].flags & 0x0c000000u;
        h = Fnv(h, &bits, sizeof bits);
        if (bits)
            (*units)++;
    }
    return h;
}

static unsigned long long HashState()
{
    int units;
    unsigned long long parts[3] = { HashCells(), HashBuckets(), HashConflicts(&units) };
    return Fnv(kFnvBasis, parts, sizeof parts);
}

static void Play(Run* run, bool tamper)
{
    Restart();
    memset(run, 0, sizeof *run);
    s_tamper = tamper;
    s_dropped = -1;
    s_removals = 0;
    for (s_tick = 0; s_tick < kTicks; s_tick++) {
        for (int i = 1; i < kUnits; i++) {
            if (s_tick > 0) {
                MapEvent remove = MapEventOf(MAP_REMOVE_UNIT, &s_units[i]);
                g_seams.map->Apply(&remove);
            }
            Move(s_tick, i);
            MapEvent add = MapEventOf(MAP_ADD_UNIT, &s_units[i]);
            Check(g_seams.map->Apply(&add) == 1, "ADD_UNIT of a unit has outcome 1");
        }
        run->cells[s_tick] = HashCells();
        run->buckets[s_tick] = HashBuckets();
        run->conflicts[s_tick] = HashConflicts(&run->conflictUnits[s_tick]);
        run->state[s_tick] = HashState();
    }
    run->dropped = s_dropped;
    run->removals = s_removals;
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
    std::vector<char> units((char*)s_units, (char*)(s_units + kUnits));
    std::vector<char> cells((char*)g_game->cells.get(), (char*)(g_game->cells.get() + kMapCells * kMapCells));
    MapEvent noUnit[7];
    for (int k = 0; k < 7; k++)
        noUnit[k] = MapEventOf(MAP_ADD_UNIT + k);
    MapEvent badClass = MapEventOf(MAP_REFRESH_PASS_MAP);
    badClass.value = 40;
    MapEvent badKind = MapEventOf(127);
    badKind.unit = Ref<Unit>(&s_units[kRunner]).offset;
    for (int k = 0; k < 7; k++)
        Check(g_seams.map->Apply(&noUnit[k]) == 0, "an event of a unit kind with a null unit has outcome 0");
    Check(g_seams.map->Apply(&badClass) == 0, "a pass map refresh of a class that does not exist has outcome 0");
    Check(g_seams.map->Apply(&badKind) == 0, "an event of an unknown kind has outcome 0");
    Unit* loose = &s_units[0];
    Check(!loose->spatialBucket && s_units[kRunner].spatialBucket, "the unit outside the run has no bucket, and the runner has one");
    MapEvent unlinkNoBucket = MapEventOf(MAP_BUCKET_UNLINK, loose);
    MapEvent prependNoBucket = MapEventOf(MAP_BUCKET_PREPEND, loose);
    MapEvent prependAgain = MapEventOf(MAP_BUCKET_PREPEND, &s_units[kRunner]);
    MapEvent unlinkStray = MapEventOf(MAP_BUCKET_UNLINK, loose);
    MapEvent negativePassMaps = MapEventOf(MAP_PASS_MAPS);
    negativePassMaps.size[0] = -1;
    negativePassMaps.size[1] = 2;
    Check(g_seams.map->Apply(&unlinkNoBucket) == 0, "a bucket unlink of a unit with no bucket has outcome 0");
    Check(g_seams.map->Apply(&prependNoBucket) == 0, "a bucket prepend of a unit with no bucket has outcome 0");
    Check(g_seams.map->Apply(&prependAgain) == 0, "a bucket prepend of a unit already in its bucket has outcome 0");
    loose->spatialBucket = s_units[kRunner].spatialBucket;
    Check(g_seams.map->Apply(&unlinkStray) == 0, "a bucket unlink of a unit not in its bucket's chain has outcome 0");
    loose->spatialBucket = 0;
    Check(g_seams.map->Apply(&negativePassMaps) == 0, "a PASS_MAPS of negative size has outcome 0");
    Check(HashState() == before, "a rejected event changes no hashed state");
    Check(memcmp(&units[0], s_units, units.size()) == 0, "a rejected event changes no unit");
    Check(memcmp(&cells[0], g_game->cells.get(), cells.size()) == 0, "a rejected event changes no cell");
}

int main()
{
    Setup();
    LosRadarSeam los;
    memset(&los, 0, sizeof los);
    los.GetMapCell = TestGetMapCell;
    g_seams.los_radar = &los;
    MapSeam seam;
    memset(&seam, 0, sizeof seam);
    seam.Apply = TamperedApply;
    g_seams.map = &seam;

    static Run plain, again, tampered;
    Play(&plain, false);
    Play(&again, false);
    Play(&tampered, true);

    int conflictTicks = 0, withOverflow = 0;
    for (int t = 0; t < kTicks; t++) {
        if (plain.conflictUnits[t] > 0)
            conflictTicks++;
        if (t > 0 && plain.buckets[t] != plain.buckets[t - 1])
            withOverflow++;
    }
    Check(plain.dropped == -1 && plain.removals == (kTicks - 1) * (kUnits - 1), "the default module applies every removal");
    Check(conflictTicks >= 2, "units that overlap set conflict bits at some ticks");
    Check(withOverflow >= 4, "the spatial buckets change as the units move");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same hashes at every tick");

    Check(tampered.dropped == kTamperTick, "the tampered module drops the runner's removal at the tampered tick");
    Check(tampered.removals == plain.removals, "the tampered module is offered the same removals as the default");

    int stateDiffering = 0, cellsDiffering = 0, bucketsDiffering = 0, conflictsDiffering = 0;
    int firstState = FirstDiffering(plain.state, tampered.state, &stateDiffering);
    int firstCells = FirstDiffering(plain.cells, tampered.cells, &cellsDiffering);
    FirstDiffering(plain.buckets, tampered.buckets, &bucketsDiffering);
    int firstConflicts = FirstDiffering(plain.conflicts, tampered.conflicts, &conflictsDiffering);
    Check(firstState == kTamperTick, "the hashed state differs first at the tampered tick");
    Check(stateDiffering == kTicks - kTamperTick, "the hashed state differs at every tick from the tampered tick on");
    Check(firstCells == kTamperTick && cellsDiffering == kTicks - kTamperTick, "the cells differ at every tick from the tampered tick on");
    Check(bucketsDiffering == 0, "the spatial buckets do not differ, as a removal leaves them alone");
    Check(firstConflicts == kTamperTick, "the conflict bits differ first at the tampered tick");
    printf("tampered tick %d, first differing tick %d, %d differing ticks (cells %d, buckets %d, conflicts %d)\n",
        tampered.dropped, firstState, stateDiffering, cellsDiffering, bucketsDiffering, conflictsDiffering);

    CheckRejected();

    printf(g_failures ? "FAILED\n" : "map_writer_test: all passed\n");
    return g_failures != 0;
}
