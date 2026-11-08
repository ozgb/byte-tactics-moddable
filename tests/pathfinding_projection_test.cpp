#include <stdio.h>
#include <string.h>

#include "../src/state/block.h"
#include "../src/ai/pathfind.h"
#include "../src/ai/pathfinder.h"
#pragma pack(push, 1)
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

Game* g_game;
Seams g_seams;
int g_scratchSteps;

extern const TurnCostTable g_turnCosts;
const TurnCostTable g_turnCosts = { { 0, 40, 60, 80, 100, 80, 60, 40 } };
extern "C" signed char bt_dirDeltaX[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
extern "C" signed char bt_dirDeltaZ[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
#pragma comment(linker, "/alternatename:?g_dirDeltaX@@3PACA=_bt_dirDeltaX")
#pragma comment(linker, "/alternatename:?g_dirDeltaZ@@3PACA=_bt_dirDeltaZ")

static int g_failures;

static void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

enum { kW = 32, kH = 32, kUnits = 4 };

static unsigned char s_pass[kH][kW];
static unsigned int s_tick;
static PathPoint s_cell[kUnits + 1];

static unsigned int __stdcall WTick() { return s_tick; }
static unsigned int __stdcall WSearchStepLimit() { return 160; }
static int __stdcall WPlayerCount() { return 1; }
static int __stdcall WIsPlaying(int player) { return player == 0; }
static int __stdcall WUnitsPerPlayer() { return kUnits; }
static PathUnit __stdcall WFirstUnit(int) { return 1; }
static PathUnit __stdcall WLastUnit(int) { return kUnits; }
static PathUnit __stdcall WNextUnit(PathUnit unit) { return unit + 1; }
static int __stdcall WUnitState(PathUnit unit, PathUnitState* out)
{
    if (unit < 1 || unit > kUnits)
        return 0;
    out->player = 0;
    out->cell = s_cell[unit];
    out->heading = 0x4000;
    out->moveClass = 1;
    out->footprintX = 2;
    out->footprintY = 2;
    return 1;
}
static PathClass __stdcall WMoveClass(int index) { return index == 0 ? 1 : 0; }
static int __stdcall WInBounds(PathClass, int x, int y) { return x >= 0 && y >= 0 && x < kW && y < kH; }
static int __stdcall WPass(PathClass, int x, int y) { return s_pass[y][x]; }
static void __stdcall WSeenSize(int* width, int* height)
{
    *width = kW / 2 + 2;
    *height = kH / 2 + 2;
}
static int __stdcall WSeen(int, int, int) { return 1; }
static int __stdcall WLineOfSight(PathClass, int x0, int y0, int x1, int y1)
{
    return s_pass[y0][x0] != 0 && s_pass[y1][x1] != 0;
}
static int __stdcall WTerrainPass(PathClass, int x, int y) { return s_pass[y][x]; }
static void __stdcall WBegin(PathUnit) {}
static void __stdcall WEnd(PathUnit) {}

static PathWorldView s_world = { WTick, WSearchStepLimit, WPlayerCount, WIsPlaying, WUnitsPerPlayer, WFirstUnit, WLastUnit, WNextUnit, WUnitState,
                                 WMoveClass, WInBounds, WPass, WSeenSize, WSeen, WLineOfSight, WTerrainPass, WBegin, WEnd };

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

struct Field {
    unsigned int offset;
    unsigned int size;
};

static const Field kRecord[] = { { 8, 4 }, { 12, 4 }, { 20, 4 }, { 24, 4 }, { 32, 4 }, { 36, 4 }, { 40, 4 }, { 72, 4 }, { 76, 4 },
                                 { 80, 4 }, { 84, 4 }, { 88, 4 }, { 120, 1 }, { 121, 4 }, { 125, 4 }, { 129, 4 }, { 133, 4 },
                                 { 137, 4 }, { 141, 4 }, { 145, 4 }, { 149, 4 }, { 153, 4 }, { 157, 4 }, { 161, 40 } };
static const Field kSearch[] = { { 48, 2 }, { 50, 2 }, { 56, 4 }, { 60, 4 }, { 64, 4 }, { 68, 4 }, { 92, 4 }, { 96, 4 },
                                 { 100, 4 }, { 104, 8 }, { 112, 8 } };
static const unsigned int kObject = 88;

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

static unsigned long long HashProjection()
{
    s_hash = 0xcbf29ce484222325ull;
    s_lines = 0;
    PathfindingWriteState(&s_writer);
    return s_hash;
}

static unsigned long long HashBlock()
{
    return Fnv(0xcbf29ce484222325ull, SimBlockBase(), SimHighWater());
}

static void Setup()
{
    memset(s_pass, 2, sizeof s_pass);
    for (int y = 0; y < 26; y++)
        s_pass[y][16] = 0;
    for (int y = 4; y < 10; y++)
        s_pass[y][8] = 0;
    for (int u = 1; u <= kUnits; u++) {
        s_cell[u].x = 2;
        s_cell[u].y = (short)(3 * u);
    }
    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    g_game->width = kW;
    g_game->height = kH;
    g_seams.pathworld = &s_world;
    PathMapInfo map = { kW, kH, kUnits + 1, 1 };
    unsigned int size = PathfindingRegionSize(&map);
    PathfindingInit(&map, SimAlloc(size), size);
}

static void Request(PathUnit unit, int x, int y)
{
    PathRequest r;
    memset(&r, 0, sizeof r);
    r.unit = unit;
    r.notBefore = 0;
    r.requester = unit * 16;
    r.target = unit;
    r.goal.kind = PATH_GOAL_CIRCLE;
    r.goal.centre.x = (short)x;
    r.goal.centre.y = (short)y;
    r.goal.radius[0] = 18;
    PathfindingSubmit(&r);
}

static void Tick()
{
    s_tick++;
    PathfindingStep();
    PathEvent e;
    PathPoint points[64];
    while (PathfindingPoll(&e, points, 64))
        ;
}

static void FlipByte(unsigned char* at)
{
    *at ^= 0x01;
}

static void Mutate(const char* what, unsigned char* at, unsigned int size)
{
    unsigned long long base = HashProjection();
    unsigned char saved[64];
    memcpy(saved, at, size);
    for (unsigned int i = 0; i < size; i += (size >= 4 ? 4 : size)) {
        FlipByte(at + i);
        char message[160];
        sprintf(message, "flipping %s byte %u changes the projection", what, i);
        Check(HashProjection() != base, message);
        memcpy(at, saved, size);
    }
    char message[160];
    sprintf(message, "restoring %s restores the projection", what);
    Check(HashProjection() == base, message);
}

int main()
{
    Setup();
    Request(1, 28, 4);
    Request(2, 28, 12);
    Request(3, 28, 28);
    unsigned long long idle = HashProjection();
    Pathfinder* pf = GetPathfinder();
    for (int i = 0; i < 40 && !(pf->object != 0 && pf->count > 3 && pf->used > 8); i++)
        Tick();
    Check(HashProjection() != idle, "a step changes the projection");
    Check(pf->object != 0, "a search is running");
    Check(pf->count > 3, "the open heap holds nodes");

    unsigned long long block = HashBlock();
    unsigned long long first = HashProjection();
    Check(HashBlock() == block, "WriteState writes nothing to the state block");
    Check(HashProjection() == first, "WriteState writes the same state twice");
    Check(s_lines > 100, "WriteState writes the record, the nodes and the cells");

    unsigned char* base = (unsigned char*)pf;
    for (unsigned int i = 0; i < sizeof kRecord / sizeof kRecord[0]; i++)
        Mutate("a record field", base + kRecord[i].offset, kRecord[i].size);
    for (unsigned int i = 0; i < sizeof kSearch / sizeof kSearch[0]; i++)
        Mutate("a search field", base + kSearch[i].offset, kSearch[i].size);

    Node* pool = pf->pool.get();
    int node = -1, penalised = -1;
    for (int n = 0; n < pf->used; n++) {
        bool inHeap = false;
        for (int s = 0; s < pf->count; s++)
            inHeap = inHeap || pf->items[s].get() == &pool[n];
        if (!inHeap)
            continue;
        if (node < 0)
            node = n;
        if (pool[n].data.g != 0 && penalised < 0)
            penalised = n;
    }
    Check(node >= 0 && penalised >= 0, "the heap holds a node with a cost");
    if (penalised >= 0) {
        Node* n = &pool[penalised];
        Mutate("a node index", (unsigned char*)&n->index, 4);
        Mutate("a node position x", (unsigned char*)&n->data.pos.x, 2);
        Mutate("a node position y", (unsigned char*)&n->data.pos.y, 2);
        Mutate("a node g", (unsigned char*)&n->data.g, 4);
        Mutate("a node f", (unsigned char*)&n->data.f, 4);
        Mutate("a node steps", (unsigned char*)&n->data.steps, 2);
        Mutate("a node penalty", (unsigned char*)&n->data.penalty, 2);
    }
    if (pf->count >= 2) {
        unsigned long long before = HashProjection();
        Ref<Node> a = pf->items[0];
        pf->items[0] = pf->items[1];
        pf->items[1] = a;
        Check(HashProjection() != before, "swapping two heap entries changes the projection");
        pf->items[1] = pf->items[0];
        pf->items[0] = a;
        Check(HashProjection() == before, "swapping them back restores the projection");
    }

    const unsigned int* dirty = pf->grid.dirty.get();
    unsigned int word = 0;
    while (word < (pf->grid.count + 255u) / 256 && dirty[word] == 0)
        word++;
    Check(word < (pf->grid.count + 255u) / 256, "a dirty word is set");
    Mutate("a dirty word", (unsigned char*)(dirty + word), 4);

    PathCell* cells = pf->grid.cells.get();
    int searched = -1;
    for (int i = 0; i < pf->grid.count && searched < 0; i++) {
        bool marked = (dirty[i >> 8] >> ((i >> 3) & 0x1f)) & 1;
        if (marked && ((cells[i].flags & 3) == 1 || (cells[i].flags & 3) == 2))
            searched = i;
    }
    Check(searched >= 0, "a dirty cell is open or closed");
    if (searched >= 0) {
        Mutate("a cell flags", (unsigned char*)&cells[searched].flags, 1);
        Mutate("a cell dir", (unsigned char*)&cells[searched].dir, 1);
        Mutate("a cell node", (unsigned char*)&cells[searched].node, 2);
    }

    PathExtra* extra = pf->Extra();
    Mutate("a tick load", (unsigned char*)extra->tickLoad, 4);
    Mutate("a budget cap", (unsigned char*)extra->budgetCap, 4);
    Mutate("the refresh counter", (unsigned char*)&extra->refreshCounter, 4);

    unsigned long long before = HashBlock();
    HashProjection();
    Check(HashBlock() == before, "WriteState still writes nothing after the mutations");

    SimFree(PathfindingShutdown());
    unsigned long long empty = HashProjection();
    Check(empty != first, "with no region the state written is empty");
    PathMapInfo map = { kW, kH, kUnits + 1, 1 };
    unsigned int size = PathfindingRegionSize(&map);
    PathfindingInit(&map, SimAlloc(size), size);
    Check(GetPathfinder() != 0, "a new region is built");
    Check(HashProjection() != empty, "a new region is written");

    printf(g_failures ? "FAILED\n" : "pathfinding_projection_test: all passed\n");
    return g_failures != 0;
}
