#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include "../src/state/block.h"
#include "../src/scripts/scripts_seam.h"
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
#include "../src/scripts/cob_script.h"
#include "../src/scripts/cob_types.h"
#pragma pack(pop)

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
    CobScript* script = (CobScript*)SimAlloc(sizeof(CobScript));
    memset((void*)script, 0, sizeof(CobScript));
    ScriptTable* table = (ScriptTable*)SimAlloc(sizeof(ScriptTable));
    memset(table, 0, sizeof(ScriptTable));
    table->staticCount = 2;
    table->pieceCount = 1;
    int* statics = (int*)SimAlloc(8);
    statics[0] = 7;
    statics[1] = 8;
    char* pieces = (char*)SimAlloc(0x4c);
    memset(pieces, 0, 0x4c);
    script->table = table;
    script->statics = statics;
    script->pieces = (Piece*)(void*)pieces;
    script->scale = 3;
    script->channels[2].state = 0x2000000 | 0x400000;
    script->channels[2].pc = 40;
    script->channels[2].sp = 1;
    script->channels[2].delay = 9;
    units[1].script = (UnitScript*)(void*)script;

    Dump(ScriptsWriteState);
    Check(Find("scriptsState.units[1]", "scale", 0) != nullptr && Find("scriptsState.units[1]", "scale", 0)->value == 3, "the unit's record holds the script's scale");
    Check(Find("scriptsState.units[1]", "channels[%u].state", 0)->value == 0, "an idle channel writes only its state");
    Check(Find("scriptsState.units[1]", "channels[%u].pc", 0) == nullptr, "an idle channel writes no program counter");
    Check(Find("scriptsState.units[1]", "channels[%u].pc", 2)->value == 40, "a busy channel writes its program counter");
    Check(Find("scriptsState.units[1]", "channels[%u].stack", 2)->size == -8, "a busy channel writes its live stack words");
    Check(Find("scriptsState.units[1]", "channels[%u].delay", 2)->value == 9, "a channel that sleeps writes its delay");
    Check(Find("scriptsState.units[1]", "statics.count", 0)->value == 2 && Find("scriptsState.units[1]", "statics", 0)->size == -8, "the statics are written");
    Check(Find("scriptsState.units[1]", "pieces[%u].active", 0) != nullptr, "the script's pieces are written");
    Check(Find("scriptsState.units[2]", "scale", 0) == nullptr && Find("scriptsState.units[0]", "scale", 0) == nullptr, "units without a script have no record");
    script->channels[2].pc = 41;
    Dump(ScriptsWriteState);
    Check(Find("scriptsState.units[1]", "channels[%u].pc", 2)->value == 41, "tampering with a channel's program counter changes its record");

    printf(g_failures ? "FAILED\n" : "scripts_writer_test: all passed\n");
    return g_failures != 0;
}
