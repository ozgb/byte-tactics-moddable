#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/los_radar.h"
#include "../src/state/sim_globals.h"
#include "../src/units/unit_def.h"
#include "../src/util/vec3.h"
#include "../src/map/los_tables.h"
#include "../src/game/mission_conditions.h"
#pragma pack(push, 1)
#include "../src/graphics/gaf_frame.h"
#include "../src/map/frame_table.h"
#include "../src/map/eye.h"
#include "../src/game/unit_resources.h"
#include "../src/network/player.h"
#include "../src/units/unit.h"
#include "../src/game/game.h"
#pragma pack(pop)
#include "../src/game/los_radar_events.h"
#include "../src/map/line_of_sight.h"
#include "../src/game/players.h"

struct Surface;
struct Rect;

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

static void Unexpected(const char* what)
{
    printf("FAIL: unexpected call to %s\n", what);
    g_failures++;
}

SimGlobals& __stdcall SimCreate()
{
    static char storage[sizeof(SimGlobals)];
    return *(SimGlobals*)storage;
}
void __stdcall SchedPublish(int, const void*, unsigned int) {}
void __stdcall RecordSimEvent(SimEventKind, unsigned int, const unsigned int*, unsigned int) {}
int __stdcall RandomInt(int n)
{
    return n > 0 ? n - 1 : 0;
}
char* __stdcall Translate(const char* key)
{
    return (char*)key;
}
char* __stdcall LoadFileWithProgress(char*)
{
    Unexpected("LoadFileWithProgress");
    return 0;
}
void* __cdecl GameAllocIgnoreTag(const char*, unsigned int)
{
    Unexpected("GameAllocIgnoreTag");
    return 0;
}
void __cdecl GameFreeThunk(void*) {}
void __cdecl operator delete[](void* p)
{
    free(p);
}
unsigned char FindHostSlot()
{
    return 10;
}
int __stdcall ScheduleEconomySpend(const EconomyEvent*)
{
    return 0;
}
int __stdcall CmdShareMapping(int)
{
    return 0;
}
int __stdcall CmdShareRadar(int)
{
    return 0;
}
void __stdcall SetStartingStorageBonus(Player*, int, int) {}
CommandArgs* CommandArgs::InitArgs()
{
    count = 0;
    return this;
}
int __stdcall ExecuteCommandText(char*, int, CommandArgs*, int)
{
    return 0;
}
int MissionConditions::CheckVictory()
{
    return 0;
}
int MissionConditions::CheckDefeat(RandStream)
{
    return 0;
}
void __stdcall BuildTedRadarFrame(unsigned short*) {}
int __stdcall FillRectangle(Surface*, Rect*, int)
{
    return 0;
}
int __stdcall GrayRectangle(Surface*, Rect*)
{
    return 0;
}
int __stdcall DitherRectangle(Surface*, Rect*, int)
{
    return 0;
}
void __stdcall DrawFrame(void*, void*, int, int) {}
void __stdcall DrawTile(void*, int, int, unsigned char*) {}
void __stdcall DrawFrameOpaque(Surface*, GafFrame*, int, int) {}
void __stdcall DrawFrameGray(Surface*, GafFrame*, int, int) {}
void __stdcall EraseFrameDithered(Surface*, GafFrame*, int, int, int) {}
void LosTables::FreeTables() {}
void LosTables::LoadLosTables() {}
LosTable* LosTables::GetLosTable(int)
{
    Unexpected("LosTables::GetLosTable");
    return 0;
}
int LosTables::GetLosTableCount()
{
    return 0;
}
int LosTable::GetLosLineCount()
{
    return 0;
}
LosLine* LosTable::GetLosLine(short)
{
    Unexpected("LosTable::GetLosLine");
    return 0;
}
int LosLine::GetLosLineStepCount()
{
    return 0;
}
void LosLine::GetLosLineStep(short, unsigned short*, unsigned short*) {}

enum {
    kMapCells = 32,
    kLos = kMapCells / 2,
    kLosCells = kLos * kLos,
    kLosCellWorld = 32,
    kFrames = 2,
    kMaxRadius = 2 + kFrames - 1,
    kFrameSide = 2 * kMaxRadius + 1,
    kSightBase = 5 * kLosCellWorld,
    kTicks = 60,
    kTamperTick = 25,
    kEyeTick = 10,
    kEyeLifetime = 8,
    kPlayers = 2,
    kUnits = 7,
    kScout = 1,
    kRover = 2,
    kBase = 3,
    kPeakEnemy = 4,
    kPatroller = 5,
    kWatched = 6,
    kScoutRow = 3,
    kScoutHome = 1,
    kScoutPeak = 6,
    kMaxEyes = 0x14
};

static GafFrame s_frames[kFrames];
static unsigned char s_framePixels[kFrames][kFrameSide * kFrameSide];

GafFrame* __stdcall GetGafFrame(FrameTable*, int index)
{
    if (index < 0 || index >= kFrames) {
        Unexpected("GetGafFrame outside the table");
        index = 0;
    }
    return &s_frames[index];
}

GafFrame* __stdcall GetGafFrame(void* table, int index)
{
    return GetGafFrame((FrameTable*)table, index);
}

static Unit* s_units;
static Player* s_players;
static int s_tick;
static bool s_tamper;
static int s_tampered;
static unsigned long long s_calls;

static unsigned long long Fnv(unsigned long long h, const void* p, unsigned int n)
{
    const unsigned char* b = (const unsigned char*)p;
    for (unsigned int i = 0; i < n; i++)
        h = (h ^ b[i]) * 0x100000001b3ull;
    return h;
}

static const unsigned long long kFnvBasis = 0xcbf29ce484222325ull;

static void __stdcall TamperedUpdateUnitLineOfSight(Unit* unit)
{
    if (s_tamper && s_tick == kTamperTick && unit == &s_units[kScout]) {
        s_tampered = s_tick;
        return;
    }
    s_calls = Fnv(s_calls, &unit->id, sizeof unit->id);
    UpdateUnitLineOfSight(unit);
}

static int Triangle(int t, int period, int span)
{
    int phase = t % period;
    return phase < span ? phase : 2 * span - phase;
}

static void LosCellAt(int t, int unit, int* x, int* z)
{
    switch (unit) {
    case kScout: {
        int reach = kScoutPeak - abs(t - kTamperTick);
        *x = reach > kScoutHome ? reach : kScoutHome;
        *z = kScoutRow;
        return;
    }
    case kRover:
        *x = 1 + Triangle(t / 2, 26, 13);
        *z = 12;
        return;
    case kBase:
        *x = 2;
        *z = 8;
        return;
    case kPeakEnemy:
        *x = kScoutPeak + 2;
        *z = kScoutRow;
        return;
    case kPatroller:
        *x = 14 - Triangle(t / 3, 26, 13);
        *z = 13;
        return;
    case kWatched:
        *x = 12;
        *z = 7;
        return;
    }
    *x = 0;
    *z = 0;
}

static int WorldOfLosCell(int c)
{
    return (c * kLosCellWorld + kLosCellWorld / 2) << 16;
}

static void Move(int t)
{
    for (int i = 1; i < kUnits; i++) {
        int x, z;
        LosCellAt(t, i, &x, &z);
        s_units[i].pos.x = WorldOfLosCell(x);
        s_units[i].pos.y = 0;
        s_units[i].pos.z = WorldOfLosCell(z);
    }
}

static void BuildFrames()
{
    for (int k = 0; k < kFrames; k++) {
        int r = 2 + k;
        int side = 2 * r + 1;
        for (int y = 0; y < side; y++)
            for (int x = 0; x < side; x++)
                s_framePixels[k][y * side + x] = (x - r) * (x - r) + (y - r) * (y - r) <= r * r + 1 ? 1 : 0;
        memset(&s_frames[k], 0, sizeof s_frames[k]);
        s_frames[k].width = (unsigned short)side;
        s_frames[k].height = (unsigned short)side;
        s_frames[k].xOffset = (short)r;
        s_frames[k].yOffset = (short)r;
        s_frames[k].transparency = 0;
        s_frames[k].pixelsOrLayers = s_framePixels[k];
    }
}

static void Setup()
{
    SimBlockInit();
    BuildFrames();
    g_game = (Game*)SimAlloc(sizeof(Game));
    memset(g_game, 0, sizeof(Game));
    g_game->width = kMapCells;
    g_game->height = kMapCells;
    g_game->playerIndex = 0;
    g_game->seaLevel = 0;

    FrameTable* table = (FrameTable*)SimAlloc(sizeof(FrameTable));
    memset(table, 0, sizeof(FrameTable));
    table->count = kFrames;
    g_game->losTable = table;

    unsigned short* mask = (unsigned short*)SimAlloc(kLosCells * sizeof(unsigned short));
    g_game->visibilityMask = mask;
    Eye* eyes = (Eye*)SimAlloc(kMaxEyes * sizeof(Eye));
    g_game->eyes = eyes;

    s_units = (Unit*)SimAlloc(kUnits * sizeof(Unit));
    memset((void*)s_units, 0, kUnits * sizeof(Unit));
    g_game->units = s_units;
    g_game->unitsEnd = &s_units[kUnits - 1];

    UnitDef* scoutDef = (UnitDef*)SimAlloc(sizeof(UnitDef));
    memset((void*)scoutDef, 0, sizeof(UnitDef));
    scoutDef->range = kSightBase;
    UnitDef* towerDef = (UnitDef*)SimAlloc(sizeof(UnitDef));
    memset((void*)towerDef, 0, sizeof(UnitDef));
    towerDef->range = kSightBase + kLosCellWorld;

    s_players = g_game->players;
    for (int p = 0; p < kPlayers; p++) {
        Player* player = &s_players[p];
        player->active = 1;
        player->type = 1;
        player->index = (unsigned char)p;
        player->explored = (unsigned char*)SimAlloc(kLosCells);
        player->exploredWidth = kLos;
        player->exploredHeight = kLos;
        player->exploredSize = kLosCells;
        int first = 1 + p * 3;
        player->unitsBegin = &s_units[first];
        player->unitsEnd = &s_units[first + 2];
    }
    for (int i = 1; i < kUnits; i++) {
        Unit* u = &s_units[i];
        int p = (i - 1) / 3;
        u->id = (unsigned short)i;
        u->unitDefIndex = (unsigned short)(i == kBase ? 2 : 1);
        u->flags = 0x10000000;
        u->def = i == kBase ? towerDef : scoutDef;
        u->player = &s_players[p];
        u->playerIndex = (unsigned char)p;
    }
}

static void Restart()
{
    g_game->mapFlags = 3;
    g_game->ticks = 0;
    g_game->count_14277 = 0;
    memset(g_game->visibilityMask.get(), 0, kLosCells * sizeof(unsigned short));
    memset((void*)g_game->eyes.get(), 0, kMaxEyes * sizeof(Eye));
    for (int p = 0; p < kPlayers; p++)
        memset(s_players[p].explored.get(), 0, kLosCells);
    for (int i = 1; i < kUnits; i++) {
        s_units[i].losCacheCell = 0;
        s_units[i].losSightFrameIdx = 0;
    }
    Move(0);
}

struct Run {
    unsigned long long state[kTicks];
    unsigned long long sight[kTicks];
    unsigned long long mapped[kTicks];
    unsigned long long reader[kTicks];
    unsigned long long calls[kTicks];
    unsigned int visible[kTicks][kPlayers];
    int tampered;
    int eyeballs;
};

static unsigned long long HashSight()
{
    unsigned long long h = kFnvBasis;
    for (int i = 1; i < kUnits; i++) {
        Unit* u = &s_units[i];
        h = Fnv(h, &u->flags, sizeof u->flags);
        h = Fnv(h, &u->losCacheCell, sizeof u->losCacheCell);
        h = Fnv(h, &u->losSightFrameIdx, sizeof u->losSightFrameIdx);
    }
    for (int p = 0; p < kPlayers; p++)
        h = Fnv(h, s_players[p].explored.get(), kLosCells);
    h = Fnv(h, &g_game->count_14277, sizeof g_game->count_14277);
    h = Fnv(h, g_game->eyes.get(), g_game->count_14277 * sizeof(Eye));
    return h;
}

static unsigned long long HashMapped()
{
    return Fnv(kFnvBasis, g_game->visibilityMask.get(), kLosCells * sizeof(unsigned short));
}

static unsigned int ReadVisible(const LosRadarSeam& seam, Player* owner, unsigned long long* reader)
{
    unsigned int mask = 0;
    for (Unit* u = g_game->units + 1; u <= g_game->unitsEnd; ++u) {
        if (u->unitDefIndex == 0 || u->player.get() == owner)
            continue;
        if (seam.IsUnitVisibleToPlayer(owner, u) && !(u->flags & 0x8000)) {
            *reader = Fnv(*reader, &u->id, sizeof u->id);
            mask |= 1u << u->id;
        }
    }
    *reader = Fnv(*reader, &owner->index, sizeof owner->index);
    return mask;
}

static void Play(Run* run, bool tamper)
{
    Restart();
    memset(run, 0, sizeof *run);
    s_tamper = tamper;
    s_tampered = -1;
    unsigned long long reader = kFnvBasis;
    const LosRadarSeam& seam = *g_seams.los_radar;
    for (s_tick = 0; s_tick < kTicks; s_tick++) {
        g_game->ticks = s_tick;
        s_calls = kFnvBasis;
        Move(s_tick);
        if (s_tick == 0) {
            for (int i = 1; i < kUnits; i++) {
                LosRadarEvent reveal = LosRadarEventOf(LOS_RADAR_REVEAL_UNIT, &s_units[i]);
                Check(seam.Apply(&reveal) == 1, "REVEAL_UNIT of a unit has outcome 1");
            }
        }
        if (s_tick == kEyeTick) {
            int x, z;
            LosCellAt(s_tick, kWatched, &x, &z);
            Vec3 at;
            at.x = WorldOfLosCell(x);
            at.y = 0;
            at.z = WorldOfLosCell(z - 1);
            LosRadarEvent eye = LosRadarEventOf(LOS_RADAR_ADD_EYEBALL);
            eye.pos[0] = at.x;
            eye.pos[1] = at.y;
            eye.pos[2] = at.z;
            eye.sightDistance = kSightBase;
            eye.lifetime = kEyeLifetime;
            run->eyeballs += seam.Apply(&eye);
        }
        for (int p = 0; p < kPlayers; p++) {
            Player* owner = &s_players[p];
            for (Unit* u = owner->unitsBegin; u <= owner->unitsEnd; ++u)
                if (u->flags & 0x10000000)
                    seam.UpdateUnitLineOfSight(u);
        }
        seam.ExpireEyeballs();
        for (int p = 0; p < kPlayers; p++)
            run->visible[s_tick][p] = ReadVisible(seam, &s_players[p], &reader);
        run->reader[s_tick] = reader;
        run->sight[s_tick] = HashSight();
        run->mapped[s_tick] = HashMapped();
        unsigned long long state = Fnv(kFnvBasis, &run->sight[s_tick], sizeof run->sight[s_tick]);
        run->state[s_tick] = Fnv(state, &run->mapped[s_tick], sizeof run->mapped[s_tick]);
        run->calls[s_tick] = s_calls;
    }
    run->tampered = s_tampered;
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
    std::vector<char> before((char*)s_units, (char*)(s_units + kUnits));
    LosRadarEvent noUnit = LosRadarEventOf(LOS_RADAR_UPDATE_UNIT);
    LosRadarEvent badPlayer = LosRadarEventOf(LOS_RADAR_SHARE_MAP);
    badPlayer.player = 10;
    LosRadarEvent unknown = LosRadarEventOf(127);
    const LosRadarEvent* rejected[] = { &noUnit, &badPlayer, &unknown };
    for (const LosRadarEvent* e : rejected)
        Check(g_seams.los_radar->Apply(e) == 0, "an event with a null unit, a player outside 0 to 9 or an unknown kind has outcome 0");
    Check(memcmp(&before[0], s_units, before.size()) == 0, "a rejected event changes no unit");
}

int main()
{
    Setup();
    LosRadarSeam seam;
    memset(&seam, 0, sizeof seam);
    seam.Apply = ApplyLosRadarEvent;
    seam.UpdateUnitLineOfSight = TamperedUpdateUnitLineOfSight;
    seam.ExpireEyeballs = ExpireEyeballs;
    seam.IsUnitVisibleToPlayer = IsUnitVisibleToPlayer;
    g_seams.los_radar = &seam;

    static Run plain, again, tampered;
    Play(&plain, false);
    Play(&again, false);
    Play(&tampered, true);

    const unsigned int peak = 1u << kPeakEnemy;
    const unsigned int watched = 1u << kWatched;
    int peakSeen = 0, watchedSeen = 0, changes = 0;
    for (int t = 0; t < kTicks; t++) {
        if (plain.visible[t][0] & peak)
            peakSeen++;
        if (plain.visible[t][0] & watched)
            watchedSeen++;
        if (t > 0 && memcmp(plain.visible[t], plain.visible[t - 1], sizeof plain.visible[t]) != 0)
            changes++;
    }
    Check(plain.eyeballs == 1, "ADD_EYEBALL adds the eyeball");
    Check(watchedSeen == kEyeLifetime + 1 && (plain.visible[kEyeTick][0] & watched) && !(plain.visible[kEyeTick - 1][0] & watched),
          "the eyeball shows its enemy from the tick it is added until it expires");
    Check(peakSeen == 1 && (plain.visible[kTamperTick][0] & peak), "the scout sees the far enemy at the tampered tick only");
    Check(changes >= 4, "what the players see changes as the units move");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same hashes at every tick");

    Check(tampered.tampered == kTamperTick, "the tampered module skips the scout's line of sight at the tampered tick");
    int callsDiffering = 0;
    int firstCalls = FirstDiffering(plain.calls, tampered.calls, &callsDiffering);
    Check(firstCalls == kTamperTick && callsDiffering == 1, "the tampered module's calls differ at the tampered tick only");
    Check(!(tampered.visible[kTamperTick][0] & peak), "without the scout's line of sight the far enemy is unseen");

    int stateDiffering = 0, readerDiffering = 0, sightDiffering = 0, mappedDiffering = 0;
    int firstState = FirstDiffering(plain.state, tampered.state, &stateDiffering);
    int firstReader = FirstDiffering(plain.reader, tampered.reader, &readerDiffering);
    int firstSight = FirstDiffering(plain.sight, tampered.sight, &sightDiffering);
    int firstMapped = FirstDiffering(plain.mapped, tampered.mapped, &mappedDiffering);
    Check(firstState == kTamperTick, "the hashed state differs first at the tampered tick");
    Check(stateDiffering == kTicks - kTamperTick, "the hashed state differs at every tick from the tampered tick on");
    Check(firstReader == kTamperTick, "the reader's visible list differs first at the tampered tick");
    Check(readerDiffering == kTicks - kTamperTick, "the reader's visible list differs at every tick from the tampered tick on");
    Check(firstSight == kTamperTick && sightDiffering == 1, "the line of sight differs at the tampered tick and heals at the next");
    Check(firstMapped == kTamperTick && mappedDiffering == kTicks - kTamperTick, "the mapped cells differ at every tick from the tampered tick on");
    printf("tampered tick %d, first differing tick %d, %d differing ticks\n", tampered.tampered, firstState, stateDiffering);

    CheckRejected();

    printf(g_failures ? "FAILED\n" : "los_radar_writer_test: all passed\n");
    return g_failures != 0;
}
