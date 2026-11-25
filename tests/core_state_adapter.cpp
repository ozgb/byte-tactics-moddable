#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "../src/state/sim_globals.h"
#include "../src/state/sim_events.h"
#include "../src/state/game_rand.h"
#include "../interfaces/host.h"
#include "../src/state/block.h"
#include "../src/state/alloc_hooks.h"
#include "../src/util/vec3.h"
#include "../src/harness/replay.h"
#include "../src/game/game.h"
#include "../src/util/math.h"
#include "../link/globals.h"

SimGlobals* g_simGlobals = 0;
Game* g_game = 0;

const AllocHooks* g_allocHooks = 0;
const char g_errorCaption[20] = "Error";

static unsigned int s_streams[2];
static FILE* s_out;
static const char* s_fault = "";
static int s_testMode = 1;
static const char* s_call = "record";

static bool Fault(const char* name)
{
    return strcmp(s_fault, name) == 0;
}

ReplayGate& ReplayGateRef()
{
    static ReplayGate gate;
    return gate;
}

SimGlobals& __stdcall SimCreate()
{
    static char storage[sizeof(SimGlobals)];
    return *(SimGlobals*)storage;
}

unsigned int* __cdecl SimRandStream(int stream)
{
    return &s_streams[stream];
}

void* __cdecl SimAlloc(unsigned int size)
{
    return calloc(size ? size : 1, 1);
}

void __cdecl SimBlockTrack(void**) {}

void HOST_CALL host_message(void*, const char*, const char*, unsigned int) {}

void __cdecl RotatePoint2D(short, int*) {}

void __cdecl RotatePoint2D(int, int*) {}

int HarnessTestMode()
{
    return s_testMode;
}

void __stdcall HarnessSimEventSlot(unsigned) {}

void __stdcall FatalError(const char*)
{
    fprintf(s_out, "%s\t\"full\"\n", s_call);
    fclose(s_out);
    exit(0);
}

static unsigned long Num(const char* s)
{
    return strtoul(s, 0, 10);
}

static RandStream StreamOf(const char* s)
{
    return strcmp(s, "load") == 0 ? RAND_STREAM_LOAD : RAND_STREAM_MAIN;
}

static unsigned int Word()
{
    return g_randomSeed;
}

static int s_randCalls = 0;

static bool RunRand(const char* op, char** a, int n)
{
    if (strcmp(op, "srand") == 0 && n >= 2) {
        RandStream s = StreamOf(a[0]);
        GameSrand(s, (unsigned int)Num(a[1]));
        fprintf(s_out, "%s\t%u\n", op, GameHoldrand(s));
    } else if (strcmp(op, "rand") == 0 && n >= 1) {
        RandStream s = StreamOf(a[0]);
        if (++s_randCalls == 3 && Fault("rand_skip"))
            GameRand(s);
        int v = GameRand(s);
        fprintf(s_out, "%s\t[%d,%u]\n", op, v, GameHoldrand(s));
    } else if (strcmp(op, "shuffle") == 0 && n >= 2) {
        RandStream s = StreamOf(a[0]);
        int v = GameShuffleDraw(s)((int)Num(a[1]));
        fprintf(s_out, "%s\t[%d,%u]\n", op, v, GameHoldrand(s));
    } else if (strcmp(op, "seed") == 0 && n >= 1) {
        SeedRandom((unsigned int)Num(a[0]));
        fprintf(s_out, "%s\t%u\n", op, Word());
    } else if (strcmp(op, "int") == 0 && n >= 1) {
        int v = RandomInt((int)strtol(a[0], 0, 10));
        fprintf(s_out, "%s\t[%d,%u]\n", op, v, Word());
    } else if (strcmp(op, "state") == 0) {
        fprintf(s_out, "%s\t[%u,%u,%u]\n", op, GameHoldrand(RAND_STREAM_MAIN), GameHoldrand(RAND_STREAM_LOAD), Word());
    } else {
        return false;
    }
    return true;
}

static void PutRecord(const SimEvent& e)
{
    fprintf(s_out, "[%u,%u,%u,[%u,%u,%u,%u,%u,%u]]", (unsigned)e.kind, (unsigned)e.flags, e.tick, e.args[0], e.args[1], e.args[2], e.args[3], e.args[4], e.args[5]);
}

static void PutCounters(const SimEventQueue& q, bool reserved)
{
    fprintf(s_out, "[%u,%u,%u", q.head, q.tail, q.dropped);
    if (reserved)
        fprintf(s_out, ",%u", q.reserved);
    fprintf(s_out, "]");
}

static bool RunEvents(const char* op, char** a, int n)
{
    SimEventQueue& q = SimEvents();
    if (strcmp(op, "mode") == 0 && n >= 1) {
        s_testMode = strcmp(a[0], "test") == 0;
        fprintf(s_out, "%s\tnull\n", op);
    } else if (strcmp(op, "tick") == 0 && n >= 1) {
        g_game->ticks = (int)Num(a[0]);
        fprintf(s_out, "%s\tnull\n", op);
    } else if ((strcmp(op, "record") == 0 && n >= 2 && n <= 14) || (strcmp(op, "repeat") == 0 && n >= 3 && n <= 15)) {
        s_call = op;
        bool rep = op[1] == 'e' && op[2] == 'p';
        const char** b = (const char**)a + (rep ? 1 : 0);
        unsigned int times = rep ? (unsigned int)Num(a[0]) : 1;
        unsigned int args[12] = {};
        unsigned int count = (unsigned int)(n - 2 - (rep ? 1 : 0));
        for (unsigned int i = 0; i < count; i++)
            args[i] = (unsigned int)Num(b[2 + i]);
        for (unsigned int t = 0; t < times; t++) {
            unsigned int before = q.tail;
            RecordSimEvent((SimEventKind)Num(b[0]), (unsigned int)Num(b[1]), args, count);
            if (count > 6 && q.tail == before + 2 && Fault("events_continuation"))
                q.records[(before + 1) % SIM_EVENT_CAPACITY].flags &= (unsigned short)~SIM_EVENT_CONTINUED;
        }
        fprintf(s_out, "%s\t", op);
        PutCounters(q, false);
        fprintf(s_out, "\n");
    } else if (strcmp(op, "counters") == 0) {
        fprintf(s_out, "%s\t", op);
        PutCounters(q, true);
        fprintf(s_out, "\n");
    } else if (strcmp(op, "ring") == 0 && n >= 1) {
        fprintf(s_out, "%s\t", op);
        PutRecord(q.records[Num(a[0]) % SIM_EVENT_CAPACITY]);
        fprintf(s_out, "\n");
    } else if (strcmp(op, "pending") == 0 || strcmp(op, "drain") == 0) {
        fprintf(s_out, "%s\t[", op);
        for (unsigned int i = q.head; i != q.tail; i++) {
            if (i != q.head)
                fprintf(s_out, ",");
            PutRecord(q.records[i % SIM_EVENT_CAPACITY]);
        }
        fprintf(s_out, "]\n");
        if (op[0] == 'd')
            q.head = q.tail;
    } else {
        return false;
    }
    return true;
}

int main(int argc, char** argv)
{
    if (argc < 4)
        return 2;
    bool rand = strcmp(argv[1], "rand") == 0;
    if (!rand && strcmp(argv[1], "events") != 0)
        return 2;
    if (getenv("BT_CORE_FAULT"))
        s_fault = getenv("BT_CORE_FAULT");
    FILE* calls = fopen(argv[2], "rb");
    if (!calls)
        return 2;
    s_out = fopen(argv[3], "wb");
    if (!s_out)
        return 2;
    static char simStorage[sizeof(SimGlobals)];
    g_simGlobals = (SimGlobals*)simStorage;
    static char gameStorage[sizeof(Game)];
    g_game = (Game*)gameStorage;
    SimEventsCreate();
    SimEventsClear();
    char line[0x1000];
    while (fgets(line, sizeof(line), calls)) {
        char* args[16];
        int n = 0;
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0])
            continue;
        char* op = line;
        char* p = strchr(line, '\t');
        while (p && n < 16) {
            *p++ = 0;
            args[n++] = p;
            p = strchr(p, '\t');
        }
        if (!(rand ? RunRand(op, args, n) : RunEvents(op, args, n))) {
            fclose(s_out);
            return 2;
        }
        fflush(s_out);
    }
    fclose(calls);
    fclose(s_out);
    return 0;
}
