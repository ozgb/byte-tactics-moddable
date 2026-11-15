#include <stdio.h>
#include <string.h>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/features.h"
#include "../src/state/sim_globals.h"
#include "../src/map/cell.h"
#include "../src/map/feature.h"
#include "../src/map/features.h"
#include "../src/map/string_ref_view.h"
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
SimGlobals* g_simGlobals = 0;

int __stdcall HarnessTestMode()
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
    int value;
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

static int s_effects;

SimGlobals& __stdcall SimCreate()
{
    static char storage[sizeof(SimGlobals)];
    return *(SimGlobals*)storage;
}
void __stdcall HarnessFeaturesEvent(const FeaturesEvent*, int) {}
void __stdcall SchedPublish(int, const void*, unsigned int)
{
    s_effects++;
}
void __stdcall RecordSimEvent(SimEventKind, unsigned int, const unsigned int*, unsigned int) {}
void __stdcall FatalError(char* text)
{
    printf("FAIL: FatalError: %s\n", text);
}
int __cdecl GameRand(RandStream)
{
    return 0;
}
int __stdcall RandomInt(int)
{
    return 0;
}
void __cdecl GameFreeThunk(void*) {}
char* __stdcall BuildDataPath(char* out, const char*, const char*, const char*)
{
    return out;
}
void* __stdcall LoadObject3d(const char*)
{
    return 0;
}
Gaf* __stdcall LoadGaf(char*)
{
    return 0;
}
AnimSrc* __stdcall FindGafEntry(void*, const char*)
{
    return 0;
}
void __stdcall InitGafSequence(Handle*, AnimSrc*, int) {}
int __stdcall StepGafSequence(Handle*)
{
    return 0;
}
GafFrame* __stdcall GetGafSequenceFrame(Handle*)
{
    return 0;
}
void StringRefView::ReleaseRef() {}
TdfFile::TdfFile() {}
TdfFile::~TdfFile() {}
int TdfFile::LoadFile(char*)
{
    return 0;
}
int TdfFile::SelectRecord(char*)
{
    return 0;
}
void TdfFile::ResetCurrentRecord() {}
int TdfRecord::GetFieldInt(const char*, int def)
{
    return def;
}
double TdfRecord::GetFieldDouble(const char*, double def)
{
    return def;
}
int TdfRecord::GetFieldString(char*, char*, size_t, char*)
{
    return 0;
}

enum {
    kWidth = 16,
    kHeight = 16,
    kTicks = 90,
    kTamperTick = 40,
    kPlaced = 16,
    kTypes = 2,
    kFirstRemoveTick = 30,
    kRemoveStride = 2,
    kRemoved = 12,
    kNoFeature = 0xffff
};

static Cell* s_cells;

static unsigned long long Fnv(unsigned long long h, const void* p, unsigned int n)
{
    const unsigned char* b = (const unsigned char*)p;
    for (unsigned int i = 0; i < n; i++)
        h = (h ^ b[i]) * 0x100000001b3ull;
    return h;
}

static unsigned long long HashCells()
{
    return Fnv(0xcbf29ce484222325ull, s_cells, kWidth * kHeight * sizeof(Cell));
}

static void Restart()
{
    memset(s_cells, 0, kWidth * kHeight * sizeof(Cell));
    for (int i = 0; i < kWidth * kHeight; i++) {
        s_cells[i].feature = kNoFeature;
        s_cells[i].height = 100;
    }
    s_effects = 0;
}

static void Setup()
{
    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    g_game->width = kWidth;
    g_game->height = kHeight;
    s_cells = (Cell*)SimAlloc(kWidth * kHeight * sizeof(Cell));
    g_game->cells = s_cells;
    Feature* types = (Feature*)SimAlloc(kTypes * sizeof(Feature));
    memset(types, 0, kTypes * sizeof(Feature));
    for (int t = 0; t < kTypes; t++) {
        types[t].footprint.x = (short)(t + 1);
        types[t].footprint.y = (short)(t + 1);
        types[t].noobject = 1;
    }
    g_game->features = types;
    g_game->featureCount = kTypes;
    Restart();
}

static int PlacedX(int i)
{
    return (i % 8) * 2;
}

static int PlacedZ(int i)
{
    return (i / 8) * 2;
}

static FeaturesEvent Event(int kind, int i)
{
    FeaturesEvent e;
    memset(&e, 0, sizeof e);
    e.kind = kind;
    e.x = PlacedX(i);
    e.z = PlacedZ(i);
    e.feature = i % kTypes;
    e.owner = 10;
    return e;
}

static int Scripted(FeaturesEvent* out, int index, int tick)
{
    if (index != 0)
        return 0;
    if (tick < kPlaced) {
        *out = Event(FEATURES_PLACE, tick);
        return 1;
    }
    int removal = tick - kFirstRemoveTick;
    if (removal >= 0 && removal % kRemoveStride == 0 && removal / kRemoveStride < kRemoved) {
        *out = Event(FEATURES_REMOVE, removal / kRemoveStride);
        return 1;
    }
    return 0;
}

struct Run {
    unsigned long long cells[kTicks];
    int tampered;
    int placed;
    int removed;
};

static void Play(Run* run, bool tamper)
{
    Restart();
    run->tampered = -1;
    run->placed = 0;
    run->removed = 0;
    for (int t = 0; t < kTicks; t++) {
        FeaturesEvent e;
        for (int i = 0; Scripted(&e, i, t); i++) {
            if (tamper && t == kTamperTick && e.kind == FEATURES_REMOVE && run->tampered < 0) {
                run->tampered = t;
                continue;
            }
            int outcome = g_seams.features->Apply(&e);
            if (e.kind == FEATURES_PLACE)
                run->placed += outcome;
            else
                run->removed += outcome;
        }
        run->cells[t] = HashCells();
    }
}

int main()
{
    Setup();
    FeaturesSeam seam;
    memset(&seam, 0, sizeof seam);
    seam.Apply = ApplyEvent;
    seam.GetCellFeature = GetCellFeature;
    g_seams.features = &seam;

    static Run plain, again, tampered;
    Play(&plain, false);
    Play(&again, false);
    Play(&tampered, true);
    Check(plain.placed == 0, "features with no object take no spot of the pool");
    Check(plain.removed == kRemoved, "every scripted REMOVE removes a feature");
    Check(plain.cells[0] != plain.cells[kPlaced], "PLACE events change the map's cells");
    Check(plain.cells[kTicks - 1] != plain.cells[kPlaced], "REMOVE events change the map's cells");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same cells at every tick");
    Check(tampered.tampered == kTamperTick, "the tampered pilot drops a REMOVE at the tampered tick");
    Check(tampered.removed == kRemoved - 1, "the tampered pilot removes one feature fewer");
    int first = -1;
    int differing = 0;
    for (int t = 0; t < kTicks; t++) {
        if (plain.cells[t] != tampered.cells[t]) {
            if (first < 0)
                first = t;
            differing++;
        }
    }
    Check(first == kTamperTick, "the map's cells differ first at the tampered tick");
    Check(differing == kTicks - kTamperTick, "the map's cells differ at every tick from the tampered tick on");
    printf("tampered tick %d, first differing tick %d, %d differing ticks\n", tampered.tampered, first, differing);

    printf(g_failures ? "FAILED\n" : "features_writer_test: all passed\n");
    return g_failures != 0;
}
