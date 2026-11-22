#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include "../src/state/block.h"
#include "../src/movement/movement_seam.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/state_writer.h"
#include "../src/state/sim_globals.h"
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
#include "../src/movement/unit_motion.h"
#include "../src/movement/ai_search_goal.h"
#include "../src/movement/order_goals.h"
#include "../src/movement/packed_goal.h"
#pragma pack(pop)

Game* g_game;
Seams g_seams;
SimGlobals* g_simGlobals = 0;
void (*g_outOfMemoryHandler)() = 0;
void* g_patrolGoalVtable[1];
void* g_aiSearchGoalVtable[1];
void* g_packedGoalVtable[1];
void* g_liteGoalVtable[1];
void* g_packedPosGoalVtable[1];

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

static void* NewGoal(void** vtable, unsigned int size)
{
    void** goal = (void**)SimAlloc(size);
    memset(goal, 0, size);
    goal[0] = vtable;
    return goal;
}

int main()
{
    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    memset(g_game, 0, sizeof(Game));
    Unit* units = (Unit*)SimAlloc(3 * sizeof(Unit));
    memset((void*)units, 0, 3 * sizeof(Unit));
    g_game->units = units;
    g_game->poolCount = 3;
    units[1].unitDefIndex = 11;
    units[2].unitDefIndex = 12;
    UnitMotion* motion1 = (UnitMotion*)SimAlloc(sizeof(UnitMotion));
    memset((void*)motion1, 0, sizeof(UnitMotion));
    UnitMotion* motion2 = (UnitMotion*)SimAlloc(sizeof(UnitMotion));
    memset((void*)motion2, 0, sizeof(UnitMotion));
    units[1].motion = motion1;
    units[2].motion = motion2;
    PatrolGoal* patrol = (PatrolGoal*)NewGoal(g_patrolGoalVtable, sizeof(PatrolGoal));
    patrol->count = 2;
    patrol->points[0].x = 5;
    patrol->points[0].y = 6;
    patrol->owner = &units[1];
    motion1->obj = (PathGoalVtableView*)(void*)patrol;
    motion1->p2.x = 100;
    motion1->turn = -3;
    LiteGoal* lite = (LiteGoal*)NewGoal(g_liteGoalVtable, sizeof(LiteGoal));
    lite->netDirtyFlags = 0xff;
    motion2->obj = (PathGoalVtableView*)(void*)lite;

    Dump(MovementWriteState);
    Check(Find("movementState.units[1]", "motion.p2.x", 0) != nullptr && Find("movementState.units[1]", "motion.p2.x", 0)->value == 100, "the unit's record holds the motion's second point");
    Check(Find("movementState.units[1]", "motion.turn", 0)->value == 0xfffd && Find("movementState.units[1]", "motion.turn", 0)->size == 2, "the unit's record holds the motion's turn as two bytes");
    Check(Find("movementState.units[1]", "goal.class", 0)->value == 1, "a patrol goal has class 1");
    Check(Find("movementState.units[1]", "goal.count", 0)->value == 2, "the patrol goal's count is written");
    Check(Find("movementState.units[1]", "goal.points", 0)->size == -8, "the patrol goal's points are written as count times four bytes");
    Check(Find("movementState.units[2]", "goal.class", 0)->value == 4 && Find("movementState.units[2]", "goal.netDirty", 0)->value == 7, "a lite goal has class 4 and its net dirty bits masked");
    Check(Find("movementState.units[0]", "goal.class", 0) == nullptr, "a slot with no unit type has no record");
    motion1->p2.x = 101;
    patrol->count = 3;
    Dump(MovementWriteState);
    Check(Find("movementState.units[1]", "motion.p2.x", 0)->value == 101, "tampering with a motion point changes its record");
    Check(Find("movementState.units[1]", "goal.points", 0)->size == -12, "tampering with the patrol goal's count changes its points");
    motion2->obj = 0;
    Dump(MovementWriteState);
    Check(Find("movementState.units[2]", "goal.class", 0)->value == 0, "a motion without a goal has class 0");

    printf(g_failures ? "FAILED\n" : "movement_writer_test: all passed\n");
    return g_failures != 0;
}
