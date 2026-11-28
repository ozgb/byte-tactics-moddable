#define BT_REPLAY_FIXTURES 1
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>
#include <vector>
#include <algorithm>

#include "../src/harness/replay_schema.h"
#include "../src/state/load_data.h"
#include "../src/files/files_bank.h"
#include "../src/util/commands.h"
#include "../src/util/engine_services.h"
#include "../interfaces/seams.h"
#include "../src/game/game_state.h"
#include "../src/map/meteors.h"
#include "../src/map/feature_spot.h"
#include "../src/map/features.h"
#include "../src/game/console_commands.h"
#include "../src/units/unit_targets.h"
#include "../src/units/factory_index.h"
#include "../src/units/unit_ref.h"
#include "../src/units/path_order_attach.h"
#include "../src/units/units.h"

Unit* __stdcall CreateUnit(unsigned char player, unsigned short typeId, Vec3 pos, int finished, int mode, int id);
#include "../src/units/units_seam.h"
#include "../src/units/unit_position.h"
#include "../src/units/unit_commands.h"
#include "../src/ingame/control_panel.h"
#include "../src/map/terrain.h"
#include "../src/mission/mission.h"
#include "../src/map/spatial_bucket.h"
#include "../src/map/plot_map.h"
#include "../src/mission/map_list.h"
#include "../src/units/movement_class.h"
#include "../src/map/pass_map.h"
#include "../src/weapons/weapon_aim_cob_cb.h"
#include "../src/scripts/cob_script.h"
#include "../src/scripts/cob_types.h"
#include "../src/scripts/unit_script.h"
#include "../src/scripts/object_state.h"
#include "../src/scripts/piece_queries.h"
#include "../src/scripts/scripts_seam.h"
#include "../src/weapons/weapons.h"
#include "../src/weapons/weapons_apply.h"
#include "../src/weapons/weapon_types.h"
#include "../src/weapons/explosions.h"
#include "../src/weapons/explosion_types.h"
#include "../src/graphics/piece_frame.h"
#include "../src/graphics/memory_cache.h"
#define g_turnCosts g_turnCostsLinkTable
#include "../link/globals.h"
#undef g_turnCosts
#include "../src/game/economy.h"
#include "../src/ai/pathfinder.h"
#include "../src/game/players.h"
#include "../src/game/players_events.h"
#include "../src/ai/ai_player.h"
#include "../src/units/squads.h"
#include "../src/orders/orders_seam.h"
#include "../src/movement/movement_seam.h"
#include "../src/map/line_of_sight.h"
#include "../src/map/line_of_sight_index.h"
#include "../src/map/radar_capture.h"
#include "../src/ingame/radar_picture.h"
#include "../src/ingame/radar.h"
#include "../src/ingame/info_panel.h"
#include "../src/harness/replay_services.h"
#include "../src/harness/display_game.h"
#include "../src/harness/replay.h"
#include "../src/harness/replay_text.h"
#include "../src/harness/replay_values.h"
#include "../src/harness/replay_objects.h"
#include "../src/harness/replay_callbacks.h"
#include "replay_wasm.h"

struct InterceptorUnitView;
struct ReachPosVec3;
Projectile* __stdcall FindTargetableProjectile(InterceptorUnitView* table, int index);
int __stdcall WeaponCanReachPos(Unit* a1, ReachPosVec3* a2, ReachPosVec3* a3, int a4);
int __stdcall GetWeaponRange(int param1, unsigned int param2);

int __stdcall PickRandomReclaimableResourcesInRadius(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount);

int __stdcall CanBuildAt(UnitDef* unit, Point16 cell, short type, Player* player);
int __stdcall CanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, Point16 cell, int flags);

extern "C" int __cdecl bx_rand(void* state);
extern "C" int __cdecl bx_ScaledSin(short angle, int scale);
extern "C" int __cdecl bx_ScaledCos(short angle, int scale);

void __stdcall DisplayLog3do(const char*, Object3do*)
{
}

void* __cdecl operator new(unsigned int size);
void __cdecl operator delete(void* p);


void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}

void (*g_outOfMemoryHandler)() = 0;

extern "C" void HOST_CALL host_fatal(void*, const char* title, const char* text)
{
    printf("FAIL: host_fatal: %s: %s\n", title, text);
    exit(2);
}

int HarnessTestMode()
{
    return 1;
}

void __stdcall Emit(const void* effect, unsigned int size);

void __stdcall EmitFrom(int, const void* effect, unsigned int size)
{
    Emit(effect, size);
}

void __stdcall HarnessLosRadarEvent(const LosRadarEventHeader*, unsigned int, int)
{
}

void __stdcall HarnessMapEvent(const MapEventHeader*, unsigned int, int)
{
}

void __stdcall HarnessPlayersEvent(const PlayersEventHeader*, unsigned int, int)
{
}

void __stdcall HarnessWeaponsEvent(const WeaponsEventHeader*, unsigned int, int)
{
}

void __stdcall HarnessFeaturesEvent(const FeaturesEventHeader*, unsigned int, int)
{
}

void __stdcall HarnessAiEvent(const AiEventHeader*, unsigned int, int)
{
}

void __stdcall HarnessOrdersEvent(const OrdersEventHeader*, unsigned int, int)
{
}

void __stdcall HarnessMovementEvent(const MovementEventHeader*, unsigned int, int)
{
}

void __stdcall HarnessUnitsEvent(const UnitsEventHeader*, unsigned int, int)
{
}

void __stdcall HarnessScriptsEvent(const ScriptsEventHeader*, unsigned int, int)
{
}

Game* g_game;
SimGlobals* g_simGlobals;
char g_extTdf[4];
Seams g_seams;
const TurnCostTable g_turnCosts = { { 0, 40, 60, 80, 100, 80, 60, 40 } };
signed char g_dirDeltaX[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
signed char g_dirDeltaZ[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
signed char g_rangeByPitchTable[11] __asm__("_g_rangeByPitchTable") = { 25, 55, 70, 85, 100, 100, 75, 50, 25, 20, 15 };
__asm__(".globl \"?g_slopeSpeedFactor@@3PACA\"\n\t.set \"?g_slopeSpeedFactor@@3PACA\", _g_rangeByPitchTable+5");

SimGlobals& SimCreate()
{
    printf("FAIL: the module reached the sim globals before the replayer made them\n");
    exit(2);
}

int __cdecl ScaledSin(int angle, int scale)
{
    return bx_ScaledSin((short)angle, scale);
}

int __cdecl ScaledCos(int angle, int scale)
{
    return bx_ScaledCos((short)angle, scale);
}

char* g_fireScriptNames[4] = {(char*)"FirePrimary", (char*)"FireSecondary", (char*)"FireTertiary"};
char* g_aimScriptNames[3] = {(char*)"AimPrimary", (char*)"AimSecondary", (char*)"AimTertiary"};
char g_takeDamageScriptName[12] = "TakeDamage";
char g_hitByWeaponScriptName[12] = "HitByWeapon";
char g_paralyzeMissionName[12] = "paralyze";

UnitDef* __stdcall FindUnitType(const char* name);

extern "C" int __cdecl bx_AngleOfVector(int x, int z);

short __cdecl AngleOfVector(int x, int z)
{
    return (short)bx_AngleOfVector(x, z);
}

extern "C" void __cdecl bx_RotatePoint2D(short angle, int* xz);

void __cdecl RotatePoint2D(short angle, int* xz)
{
    bx_RotatePoint2D(angle, xz);
}

#pragma comment(linker, "/alternatename:?AngleOfVector@@YAHHH@Z=?AngleOfVector@@YAFHH@Z")
#pragma comment(linker, "/alternatename:?RotatePoint2D@@YAXGPAUXzPoint@@@Z=?RotatePoint2D@@YAXFPAH@Z")
#pragma comment(linker, "/alternatename:?AddExplosionEffect@@YGXPAUVec3@@PAXHH@Z=?AddExplosionEffect@@YGXPAUVec3@@PAUGafSequence@@HH@Z")
#pragma comment(linker, "/alternatename:?AddExplosionEffect@@YGXPAX0HH@Z=?AddExplosionEffect@@YGXPAUVec3@@PAUGafSequence@@HH@Z")
#pragma comment(linker, "/alternatename:?GetGafFrame@@YGPAXPAUGafSequence@@H@Z=?GetGafFrame@@YGPAUGafFrame@@PAUFrameTable@@H@Z")
#pragma comment(linker, "/alternatename:?CalcAimAngles@@YGHPAUUnit@@PAUWeaponDef@@PAF2EPAUVec3@@@Z=?CalcAimAngles@@YGHPAUUnit@@0PAF1HPAUVec3@@@Z")
#pragma comment(linker, "/alternatename:?CalcAimAngles@@YGHPAUUnit@@PAUWeaponDef@@PAG2EPAUVec3@@@Z=?CalcAimAngles@@YGHPAUUnit@@0PAF1HPAUVec3@@@Z")
#pragma comment(linker, "/alternatename:?FindTargetableProjectile@@YGPAHPAUUnit@@E@Z=?FindTargetableProjectile@@YGPAUProjectile@@PAUInterceptorUnitView@@H@Z")
#pragma comment(linker, "/alternatename:?FireBallisticProjectile@@YGHPAUUnitWeaponSlot@@PAUUnit@@PAUVec3@@21@Z=?FireBallisticProjectile@@YGHPAUUnitWeaponSlot@@PAUUnit@@PAUVec3@@2H@Z")
#pragma comment(linker, "/alternatename:?FireLineOfSightProjectile@@YGHPAUUnitWeaponSlot@@PAUUnit@@PAUVec3@@21@Z=?FireLineOfSightProjectile@@YGHPAUUnitWeaponSlot@@PAUUnit@@PAUVec3@@2H@Z")
#pragma comment(linker, "/alternatename:?FireVLaunchProjectile@@YGHPAUUnitWeaponSlot@@PAUUnit@@PAUVec3@@21PAH@Z=?FireVLaunchProjectile@@YGHPAUUnitWeaponSlot@@PAUUnit@@PAUVec3@@2HH@Z")
#pragma comment(linker, "/alternatename:?GetWeaponPiecePosition@@YGXPAUUnit@@PAUVec3@@EH@Z=?GetWeaponPiecePosition@@YGXPAUObject@@PAUVec3@@EH@Z")
#pragma comment(linker, "/alternatename:?InitGafSequence@@YGXPAUHandle@@PAUAnimSrc@@H@Z=?InitGafSequence@@YGXPAUHandle@@PAUGafEntry@@H@Z")
#pragma comment(linker, "/alternatename:?FindNearestEnemyUnit@SquadManager@@QAEPAUUnit@@UVec3@@@Z=?FindNearestEnemyUnit@SquadManager@@QAEPAUUnit@@HHH@Z")
#pragma comment(linker, "/alternatename:?IsCountBelowAiLimit@@YGHHGH@Z=?IsCountBelowAiLimit@@YGHEGH@Z")
#pragma comment(linker, "/alternatename:?ScaleUnitWeights@@YGXHPAVUnitTypeSet@@MH@Z=?ScaleUnitWeights@@YGXHPAIMH@Z")
#pragma comment(linker, "/alternatename:?SetUnitLimits@@YGXHPAVUnitTypeSet@@HH@Z=?SetUnitLimits@@YGXHPAIHH@Z")
#pragma comment(linker, "/alternatename:?OrderSquad@@YGXPAUPlayer@@HEHPAUUnit@@PAUVec3@@HH@Z=?OrderSquad@@YGXPAUPlayer@@HEHPAUUnit@@HHH@Z")
#pragma comment(linker, "/alternatename:?InitProjectile@@YGXPAUProjectile@@PAUWeaponDef@@PAUVec3@@HHPAUUnit@@@Z=?InitProjectile@@YGXPAUProjectile@@PAUWeaponDef@@PAUVec3@@2HPAUUnit@@@Z")
#pragma comment(linker, "/alternatename:?FindUnitType@@YGPAVUnitDef@@PBD@Z=?FindUnitType@@YGPAUUnitDefNameKeyView@@PBD@Z")
#pragma comment(linker, "/alternatename:?InitGafSequence@@YGXPAUGafRef@@PAUGafSequence@@H@Z=?InitGafSequence@@YGXPAUHandle@@PAUGafEntry@@H@Z")
#pragma comment(linker, "/alternatename:?StepGafSequence@@YGHPAUGafRef@@@Z=?StepGafSequence@@YGHPAUHandle@@@Z")
#pragma comment(linker, "/alternatename:?SolveLaunchAngle@@YGFHHHHH@Z=?SolveLaunchAngle@@YGFHHHHM@Z")
#pragma comment(linker, "/alternatename:?SubtractVec3@@YGXPAUVec3@@0PAH11@Z=?SubtractVec3@@YGXPAH0000@Z")
#pragma comment(linker, "/alternatename:?WeaponCanReachPos@@YGHPAUUnit@@PAUVec3@@1E@Z=?WeaponCanReachPos@@YGHPAUUnit@@PAUReachPosVec3@@1H@Z")

struct ConstructedElement {
    ConstructedElement();
    ~ConstructedElement();
    int value;
};

ConstructedElement::ConstructedElement()
{
}

ConstructedElement::~ConstructedElement()
{
}

int __stdcall FixtureOtherNotify(Unit*, Order*, int)
{
    return 4;
}

extern void (* const g_fixtureTimerVtable[2])() = { nullptr, nullptr };
extern void (* const g_fixtureAssaultVtable[2])() = { nullptr, nullptr };
extern void (* const g_fixtureCbVtable[2])() = { nullptr, nullptr };
extern void (* const g_fixtureAttachVtable[2])() = { nullptr, nullptr };

struct PieceOffsetObjectView;
struct PieceAnglesObjectView;
Vec3 __stdcall GetPieceOffset(PieceOffsetObjectView* obj, int index);
Angles16 __stdcall GetPieceAngles(PieceAnglesObjectView* obj, int index);

namespace {

enum { MAX_LINES = 1 << 22, MAX_OUTS = 16384, MAX_RECORDS = 64, TEXT = 512 };
enum HandleKind { OBJ_FILE, OBJ_RECORD, OBJ_BANK, OBJ_CALLBACK };

int g_failures;

void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

jmp_buf* g_fatalTrap;
char g_fatalWhat[200];

void Fatal(const char* file, int call, const char* what, const char* text)
{
    if (g_fatalTrap != nullptr) {
        _snprintf(g_fatalWhat, sizeof g_fatalWhat, "%s: %s", what, text);
        g_fatalWhat[sizeof g_fatalWhat - 1] = '\0';
        longjmp(*g_fatalTrap, 1);
    }
    printf("FAIL: %s: call %d: %s: %s\n", file, call, what, text);
    exit(2);
}

ReplayKnown g_known;
bool g_core;
bool g_loading;
bool g_coreFault;
bool g_coreDropped;
ReplayText g_coreImports;
ReplayText g_coreWrites;
ReplayText g_coreClosings;

void CoreJson(ReplayText& list, const char* kind, const char* name, const char* value)
{
    if (list.Size() != 0)
        list.AddChar(',');
    list.AddChar('"');
    ReplayText line;
    line.Add(kind);
    line.AddChar(' ');
    line.Add(name);
    line.AddChar(' ');
    line.Add(value);
    for (const unsigned char* p = (const unsigned char*)line.Data(); *p != 0; p++) {
        if (*p == '"' || *p == '\\') {
            list.AddChar('\\');
            list.AddChar((char)*p);
        } else if (*p < 0x20 || *p >= 0x7f) {
            char escape[8];
            sprintf(escape, "\\u%04x", *p);
            list.Add(escape);
        } else {
            list.AddChar((char)*p);
        }
    }
    list.AddChar('"');
    line.Free();
}

void CoreClosing(const char* name, const char* value)
{
    if (g_core)
        CoreJson(g_coreClosings, "o", name, value);
}

ReplayObjectTable g_objects;
ReplayReach g_reach;

void* AllocObject(unsigned size)
{
    bool& busy = ReplayAnswerBusy();
    bool was = busy;
    busy = true;
    void* p = SimAlloc(size);
    busy = was;
    return p;
}

struct LoadObject {
    unsigned char* native;
    unsigned size;
    unsigned kind;
    unsigned char* pixels;
    unsigned pixelSize;
};

std::vector<LoadObject> g_loadObjects;

void MapLoadObject(void* object, unsigned kind)
{
    if (!ReplayWasmActive() || object == nullptr || (kind != ROK_FRAMETABLE && kind != ROK_GAFFRAME))
        return;
    for (size_t i = 0; i < g_loadObjects.size(); i++) {
        if (g_loadObjects[i].native == object)
            return;
    }
    LoadObject o = { (unsigned char*)object, kReplayObjectKinds[kind].size, kind, nullptr, 0 };
    LoadDataRegisterFrame(object, o.size);
    g_loadObjects.push_back(o);
}

void MapLoadTarget(const ReplayObjectField& f, const char* value)
{
    if (ReplayWasmActive() && f.kind == RK_PTR && (f.target == ROK_FRAMETABLE || f.target == ROK_GAFFRAME) && strcmp(value, "null") != 0)
        MapLoadObject(ReplayResolveObject(value, nullptr, g_objects, AllocObject), f.target);
}

void SyncLoadObjects()
{
    for (size_t i = 0; i < g_loadObjects.size(); i++) {
        LoadObject& o = g_loadObjects[i];
        unsigned char* copy = ReplayWasmLinear(LoadAddress(o.native));
        memcpy(copy, o.native, o.size);
        if (o.kind == ROK_FRAMETABLE)
            ((FrameTable*)copy)->entries = nullptr;
        if (o.kind != ROK_GAFFRAME)
            continue;
        const GafFrame* frame = (const GafFrame*)o.native;
        unsigned pixelSize = frame->pixelsOrLayers == nullptr ? 0u : (unsigned)frame->width * frame->height;
        if (pixelSize > o.pixelSize) {
            if (o.pixels != nullptr) {
                LoadDataUnregister(o.pixels);
                free(o.pixels);
            }
            o.pixels = (unsigned char*)calloc(pixelSize, 1);
            o.pixelSize = pixelSize;
            LoadDataRegisterFrame(o.pixels, pixelSize);
        }
        unsigned pixels = 0;
        if (pixelSize != 0) {
            pixels = LoadAddress(o.pixels);
            memcpy(ReplayWasmLinear(pixels), frame->pixelsOrLayers, pixelSize);
        }
        ((GafFrame*)copy)->pixelsOrLayers = (unsigned char*)pixels;
        ((GafFrame*)copy)->scratch = nullptr;
    }
}

void ForgetLoadObjects()
{
    for (size_t i = 0; i < g_loadObjects.size(); i++) {
        LoadDataUnregister(g_loadObjects[i].native);
        if (g_loadObjects[i].pixels != nullptr) {
            LoadDataUnregister(g_loadObjects[i].pixels);
            free(g_loadObjects[i].pixels);
        }
    }
    g_loadObjects.clear();
}

char* g_text;
char* g_lines[MAX_LINES];
int g_lineCount;
char g_subsystem[48];
bool g_importChanges;
bool g_forgetAllocations;
const char* g_file = "";

int g_callIndex;
int g_callFirst;
int g_callEnd;
int g_cursor;
int g_importIndex;
int g_diffs;
char g_firstKey[96];
ReplayText g_detail;
const char* g_outNames[MAX_OUTS];
const char* g_outValues[MAX_OUTS];
const char* g_beforeNames[MAX_OUTS];
const char* g_beforeValues[MAX_OUTS];
int g_beforeCount;
int g_outCount;
unsigned g_hash;
int g_importsTotal;
const char* g_closing[8];
unsigned short g_rootKinds[REPLAY_MAX_ROOTS];
void* g_rootObjects[REPLAY_MAX_ROOTS];
unsigned g_rootCount;
const ReplaySlotSets* g_sets;

enum { MAX_CALLBACK_LINES = 1024 };
const char* g_cbLines[MAX_CALLBACK_LINES];
int g_cbCount;
int g_cbImportIndex;
int g_nestedFrom;
int g_nestedAfter;

char g_fakeRecords[MAX_RECORDS][16];
HapiBank g_bank;

void Diff(const char* key, const char* recorded, const char* replayed)
{
    if (g_diffs++ == 0)
        strncpy(g_firstKey, key, sizeof g_firstKey - 1);
    g_detail.Add("    ");
    g_detail.Add(key);
    g_detail.Add(": ");
    if (ReplayIsArrayText(recorded) || ReplayIsArrayText(replayed)) {
        ReplayArrayDiff(recorded, replayed, g_detail);
    } else {
        g_detail.Add("recorded ");
        g_detail.Add(recorded);
        g_detail.Add(", replayed ");
        g_detail.Add(replayed);
    }
    g_detail.AddChar('\n');
}

void HashText(const char* text)
{
    for (const unsigned char* p = (const unsigned char*)text; *p != 0; p++)
        g_hash = (g_hash ^ *p) * 16777619u;
    g_hash = (g_hash ^ 0xff) * 16777619u;
}

const char* Skip(const char* p, const char* prefix)
{
    unsigned n = (unsigned)strlen(prefix);
    return strncmp(p, prefix, n) == 0 ? p + n : nullptr;
}

unsigned ParseHex(const char* p, int digits)
{
    unsigned v = 0;
    for (int i = 0; i < digits; i++) {
        char c = p[i];
        v = v << 4 | (unsigned)(c <= '9' ? c - '0' : c - 'a' + 10);
    }
    return v;
}

unsigned ParseF32(const char* value)
{
    const char* p = Skip(value, "f32:0x");
    return p == nullptr ? 0 : ParseHex(p, 8);
}

double ParseF64(const char* value)
{
    const char* p = Skip(value, "f64:0x");
    unsigned bits[2] = { 0, 0 };
    if (p != nullptr) {
        bits[1] = ParseHex(p, 8);
        bits[0] = ParseHex(p + 8, 8);
    }
    double d;
    memcpy(&d, bits, 8);
    return d;
}

const char* ParseQuoted(const char* p, char* out, unsigned size)
{
    unsigned n = 0;
    if (*p == '"')
        p++;
    for (; *p != '\0' && *p != '"'; p++) {
        char c = *p;
        if (c == '\\') {
            p++;
            if (*p == 'n')
                c = '\n';
            else if (*p == 'r')
                c = '\r';
            else if (*p == 't')
                c = '\t';
            else if (*p == 'x') {
                c = (char)ParseHex(p + 1, 2);
                p += 2;
            } else
                c = *p;
        }
        if (n + 1 < size)
            out[n++] = c;
    }
    out[n] = '\0';
    return *p == '"' ? p + 1 : p;
}

void AddDoubleBits(ReplayText& t, double value)
{
    ReplayValue(t, value);
}

void AddVec3(ReplayText& t, const Vec3* v)
{
    ReplayValue(t, *v);
}

void AddVec3Xz(ReplayText& t, const Vec3* v)
{
    t.AddChar('(');
    t.AddInt(v->x);
    t.AddChar(',');
    t.AddInt(v->z);
    t.AddChar(')');
}

void AddObject(ReplayText& t, unsigned kind, const void* p)
{
    static const char* const names[] = { "file", "rec", "bank" };
    unsigned ordinal;
    if (kind == OBJ_RECORD) {
        ordinal = 0;
        for (int i = 0; i < MAX_RECORDS; i++) {
            if (p == g_fakeRecords[i])
                ordinal = (unsigned)i + 1;
        }
    } else {
        ordinal = ReplayHandles().Of(kind, p);
    }
    if (ordinal == 0) {
        t.Add("null");
        return;
    }
    t.Add(kind >= OBJ_CALLBACK ? kReplayCallbacks[kind - OBJ_CALLBACK].handle : names[kind]);
    t.AddChar('#');
    t.AddUnsigned(ordinal);
}

TdfRecord* ParseRecord(const char* value)
{
    const char* p = Skip(value, "rec#");
    if (p == nullptr)
        return nullptr;
    int ordinal = atoi(p);
    return ordinal >= 1 && ordinal <= MAX_RECORDS ? (TdfRecord*)g_fakeRecords[ordinal - 1] : nullptr;
}

#pragma push_macro("g_randomSeed")
#undef g_randomSeed

unsigned* SeedWord()
{
    return &g_simGlobals->g_randomSeed;
}

#pragma pop_macro("g_randomSeed")

#pragma push_macro("g_movementClasses")
#undef g_movementClasses

MovementClass* MovementEntries()
{
    return g_simGlobals->g_movementClasses.entries;
}

#pragma pop_macro("g_movementClasses")

#pragma push_macro("g_passMaps")
#undef g_passMaps

PassMap* PassMapEntries()
{
    return g_simGlobals->g_passMaps.entries;
}

#pragma pop_macro("g_passMaps")

struct RegionBinding {
    void* ptr;
    unsigned size;
};

RegionBinding g_region;
ReplayRegionView g_regionView;

struct RegionClash {
    unsigned first;
    unsigned end;
    bool found;
};

const unsigned kRegionChunkHeader = 8;
const unsigned kRegionFirstChunk = 64;

void __cdecl FindRegionClash(void* payload, unsigned int, void* context)
{
    RegionClash& clash = *(RegionClash*)context;
    unsigned first = (unsigned)((unsigned char*)payload - (unsigned char*)SimBlockBase()) - kRegionChunkHeader;
    unsigned chunk = *(const unsigned*)((const unsigned char*)payload - kRegionChunkHeader) & ~1u;
    if (first < clash.end && clash.first < first + chunk)
        clash.found = true;
}

void* PlaceRegion(unsigned offset, unsigned size, char* why)
{
    void* p = SimAllocAt(offset, size);
    if (p != nullptr)
        return p;
    unsigned block = SimBlockSize();
    unsigned need = size > block ? block + kMinChunk : ((size + 7) & ~7u) + kRegionChunkHeader;
    if (need < kMinChunk)
        need = kMinChunk;
    if (offset < kRegionFirstChunk + kRegionChunkHeader) {
        sprintf(why, "block offset %u (%u bytes) puts its chunk header inside the block's own header of %u bytes", offset, size, kRegionFirstChunk);
        return nullptr;
    }
    if ((offset & 7) != 0) {
        sprintf(why, "block offset %u (%u bytes) is not 8-byte aligned", offset, size);
        return nullptr;
    }
    if (need > block || offset - kRegionChunkHeader > block - need) {
        sprintf(why, "block offset %u (%u bytes) is past the block's end at %u, with its %u-byte chunk", offset, size, block, need);
        return nullptr;
    }
    RegionClash clash = { offset - kRegionChunkHeader, offset - kRegionChunkHeader + need, false };
    SimBlockForEach(FindRegionClash, &clash);
    if (clash.found)
        sprintf(why, "block offset %u (%u bytes) is not free: it clashes with another allocation", offset, size);
    else
        sprintf(why, "block offset %u (%u bytes) lies in a free range that would leave a leading remainder under %u bytes", offset, size, kMinChunk);
    return nullptr;
}

void* AllocRegion(unsigned offset, unsigned size)
{
    char why[160];
    void* p = PlaceRegion(offset, size, why);
    if (p == nullptr)
        Fatal(g_file, g_callIndex, "a region that cannot be placed at its recorded offset", why);
    return p;
}

void RegionBind(void* ptr, unsigned size)
{
    g_region.ptr = ptr;
    g_region.size = size;
    SimBlockTrack(&g_region.ptr);
}

unsigned char* BlockBase(const ReplayField& f)
{
    if (f.base == RB_REGION) {
        g_regionView.size = g_region.size;
        g_regionView.data = g_region.ptr == nullptr ? 0u : (unsigned)((unsigned char*)g_region.ptr - (unsigned char*)SimBlockBase());
        return (unsigned char*)&g_regionView;
    }
    return f.base == RB_GAME ? (unsigned char*)g_game : (unsigned char*)g_simGlobals;
}

void FormatField(const ReplayField& f, ReplayText& out, bool assign = false)
{
    out.Clear();
    if (f.base == RB_RNG) {
        out.AddHex32(f.offset == 2 ? *SeedWord() : *SimRandStream((int)f.offset));
        return;
    }
    ReplayFormatObjectField(ReplayFieldView(f), BlockBase(f), g_objects, assign, out);
}

bool RestoreField(const ReplayField& f, const char* value)
{
    if (f.base == RB_RNG) {
        unsigned bits = ParseHex(value + 2, 8);
        if (f.offset == 2)
            *SeedWord() = bits;
        else
            *SimRandStream((int)f.offset) = bits;
        g_known.Set(f.name, value);
        return true;
    }
    if (f.kind == RK_ARRAY)
        return ReplayApplyArray(ReplayFieldView(f), BlockBase(f), f.name, value, g_known, true, AllocObject);
    g_known.Set(f.name, value);
    MapLoadTarget(ReplayFieldView(f), value);
    return ReplayRestoreObjectField(ReplayFieldView(f), BlockBase(f), value, g_objects, AllocObject);
}

const ReplayField* FindField(const char* name)
{
    for (unsigned i = 0; i < kReplayFieldCount; i++) {
        if (strcmp(kReplayFields[i].name, name) == 0)
            return &kReplayFields[i];
    }
    return nullptr;
}

const ReplayField* FindBlockField(const char* name)
{
    const char* open = strchr(name, '[');
    if (open == nullptr)
        return FindField(name);
    if (const ReplayField* whole = FindField(name))
        return whole;
    char base[96];
    size_t length = (size_t)(open - name);
    if (length >= sizeof base)
        return nullptr;
    memcpy(base, name, length);
    base[length] = '\0';
    const ReplayField* f = FindField(base);
    return f != nullptr && f->kind == RK_VECTOR ? f : nullptr;
}

bool ApplyBlockVector(const ReplayField& f, const char* name, const char* value, bool memory)
{
    ReplayObjectField view = ReplayFieldView(f);
    unsigned char* object = memory ? BlockBase(f) : nullptr;
    if (memory && object == nullptr)
        return false;
    const char* open = strchr(name, '[');
    if (open == nullptr)
        return ReplayApplyVector(view, object, name, value, g_known, memory, g_objects, memory ? AllocObject : nullptr);
    char* end;
    unsigned long at = strtoul(open + 1, &end, 10);
    if (end == open + 1 || *end != ']')
        return false;
    if (end[1] == '\0')
        return ReplayApplyVectorString(view, object, (unsigned)at, name, value, g_known, memory, memory ? AllocObject : nullptr);
    if (end[1] != '.')
        return false;
    return ReplayApplyVectorElement(view, object, (unsigned)at, end + 2, name, value, g_known, memory, g_objects, memory ? AllocObject : nullptr);
}

void CompareBlockVector(const ReplayField& f)
{
    ReplayCompareVector(ReplayFieldView(f), BlockBase(f), f.name, g_known, g_objects, Diff, HashText);
}

bool InSet(const unsigned char* set, unsigned count, const ReplayField* f)
{
    for (unsigned i = 0; i < count; i++) {
        if (&kReplayFields[set[i]] == f)
            return true;
    }
    return false;
}

bool InObjectSet(const unsigned short* set, unsigned count, const ReplayObjectField* f)
{
    for (unsigned i = 0; i < count; i++) {
        const ReplayObjectField& g = kReplayObjectFields[set[i]];
        if (g.object == f->object && strcmp(g.name, f->name) == 0)
            return true;
    }
    return false;
}

const char* LastResult(const char* line)
{
    const char* found = nullptr;
    for (const char* p = line; (p = strstr(p, " = ")) != nullptr; p += 3)
        found = p + 3;
    return found == nullptr ? "" : found;
}

bool ClosingLine(const char* line)
{
    if (strncmp(line + 4, "snapshot#", 9) == 0)
        return true;
    for (int i = 0; i < 8; i++) {
        if (g_closing[i] != nullptr && strncmp(line + 4, g_closing[i], strlen(g_closing[i])) == 0)
            return true;
    }
    return false;
}

const ReplayField* RngField(const char* name)
{
    for (unsigned i = 0; i < kReplayFieldCount; i++) {
        if (kReplayFields[i].base == RB_RNG && strcmp(kReplayFields[i].name, name) == 0)
            return &kReplayFields[i];
    }
    return nullptr;
}

void RestoreImportObject(const char* name, const char* value);

const char* const kQueryImports[] = { "services.GetGafFrame" };

struct UnreadArgument {
    const char* import;
    const char* vector;
    int component;
};

const UnreadArgument kUnreadArguments[] = { { "los_radar.GetMapCellAtPosition", "pos=(", 1 } };

bool IsImportNamed(const char* text, const char* name)
{
    size_t length = strcspn(text, " ");
    return strlen(name) == length && strncmp(text, name, length) == 0;
}

bool IsQueryName(const char* text)
{
    for (const char* name : kQueryImports) {
        if (IsImportNamed(text, name))
            return true;
    }
    return false;
}

bool IsQueryLine(const char* line)
{
    return strncmp(line, "  i ", 4) == 0 && IsQueryName(line + 4);
}

int SkipQueryLines(int k)
{
    while (k < g_callEnd && IsQueryLine(g_lines[k])) {
        k++;
        while (k < g_callEnd && (strncmp(g_lines[k], "    c ", 6) == 0 || strncmp(g_lines[k], "  b ", 4) == 0 || (strncmp(g_lines[k], "  o ", 4) == 0 && !ClosingLine(g_lines[k]))))
            k++;
    }
    return k;
}

int NextImportLine(int k)
{
    for (;;) {
        while (k < g_callEnd && strncmp(g_lines[k], "  i ", 4) != 0 && strncmp(g_lines[k], "  o ", 4) != 0 && strncmp(g_lines[k], "  w ", 4) != 0)
            k++;
        if (k == g_callEnd || !IsQueryLine(g_lines[k]))
            return k;
        k = SkipQueryLines(k);
    }
}

bool MaskUnread(const char* text, const UnreadArgument& u, char* out, size_t size)
{
    const char* result = strstr(text, " = ");
    size_t length = result == nullptr ? strlen(text) : (size_t)(result - text);
    if (length >= size)
        return false;
    memcpy(out, text, length);
    out[length] = '\0';
    char* p = strstr(out, u.vector);
    if (p == nullptr)
        return false;
    p += strlen(u.vector);
    for (int i = 0; i < u.component; i++) {
        p = strpbrk(p, ",)");
        if (p == nullptr || *p == ')')
            return false;
        p++;
    }
    char* end = strpbrk(p, ",)");
    if (end == nullptr)
        return false;
    memmove(p + 1, end, strlen(end) + 1);
    *p = '*';
    return true;
}

bool EqualButUnread(const char* recorded, const char* actual)
{
    for (const UnreadArgument& u : kUnreadArguments) {
        if (!IsImportNamed(actual, u.import) || !IsImportNamed(recorded, u.import))
            continue;
        char a[512];
        char b[512];
        return MaskUnread(recorded, u, a, sizeof a) && MaskUnread(actual, u, b, sizeof b) && strcmp(a, b) == 0;
    }
    return false;
}

int FindQueryLine(const char* actual, int from, int to)
{
    unsigned n = (unsigned)strlen(actual);
    for (int k = from; k < to; k++) {
        const char* line = g_lines[k];
        if (strncmp(line, "  i ", 4) == 0 && strncmp(line + 4, actual, n) == 0 && (line[4 + n] == '\0' || strncmp(line + 4 + n, " = ", 3) == 0))
            return k;
    }
    return -1;
}

char g_queryNames[MAX_OUTS][160];
int g_queryAnswers;

const char* EnterQuery(const ReplayText& actual)
{
    g_outCount = 0;
    g_beforeCount = 0;
    g_cbCount = 0;
    int at = FindQueryLine(actual.Data(), g_callFirst + 1, g_callEnd);
    if (at < 0)
        at = FindQueryLine(actual.Data(), 0, g_lineCount);
    if (at < 0) {
        char key[96];
        sprintf(key, "query %.*s", (int)strcspn(actual.Data(), " "), actual.Data());
        Diff(key, "no answer to these arguments", actual.Data());
        return "";
    }
    const char* recorded = g_lines[at] + 4;
    unsigned n = (unsigned)strlen(actual.Data());
    const char* result = recorded[n] == '\0' ? "" : recorded + n + 3;
    char prefix[96];
    _snprintf(prefix, sizeof prefix, "%s.", result);
    prefix[sizeof prefix - 1] = '\0';
    size_t prefixLength = strlen(prefix);
    for (int k = at + 1; k < g_lineCount && strncmp(g_lines[k], "  o ", 4) == 0 && strncmp(g_lines[k] + 4, prefix, prefixLength) == 0; k++) {
        const char* name = g_lines[k] + 4;
        const char* space = strchr(name, ' ');
        size_t length = space == nullptr ? 0 : (size_t)(space - name);
        if (space == nullptr || length >= sizeof g_queryNames[0] || g_outCount == MAX_OUTS)
            Fatal(g_file, g_callIndex, "a line of a query's answer that cannot be read", g_lines[k]);
        memcpy(g_queryNames[g_outCount], name, length);
        g_queryNames[g_outCount][length] = '\0';
        g_outNames[g_outCount] = g_queryNames[g_outCount];
        g_outValues[g_outCount] = space + 1;
        g_outCount++;
    }
    for (int i = 0; i < g_outCount; i++) {
        const ReplayField* f = RngField(g_outNames[i]);
        if (f != nullptr)
            RestoreField(*f, g_outValues[i]);
        else
            RestoreImportObject(g_outNames[i], g_outValues[i]);
    }
    g_queryAnswers++;
    return result;
}

const char* Enter(const ReplayText& actual)
{
    if (g_loading)
        return "";
    if (IsQueryName(actual.Data()))
        return EnterQuery(actual);
    int index = g_importIndex++;
    g_importsTotal++;
    g_outCount = 0;
    g_beforeCount = 0;
    g_cbCount = 0;
    g_cbImportIndex = index;
    char key[96];
    const char* space = strchr(actual.Data(), ' ');
    int nameLength = space == nullptr ? (int)strlen(actual.Data()) : (int)(space - actual.Data());
    sprintf(key, "import %d %.*s", index, nameLength, actual.Data());
    int next = NextImportLine(g_cursor);
    if (next == g_callEnd || strncmp(g_lines[next], "  i ", 4) != 0) {
        Diff(key, "no such import call", actual.Data());
        if (g_core) {
            char name[96];
            sprintf(name, "%.*s", nameLength < 95 ? nameLength : 95, actual.Data());
            CoreJson(g_coreImports, "i", name, space == nullptr ? "" : space + 1);
        }
        return "";
    }
    g_cursor = next + 1;
    const char* recorded = g_lines[next] + 4;
    while (g_cursor < g_callEnd && strncmp(g_lines[g_cursor], "    c ", 6) == 0) {
        if (g_cbCount == MAX_CALLBACK_LINES)
            abort();
        g_cbLines[g_cbCount++] = g_lines[g_cursor] + 6;
        g_cursor++;
    }
    while (g_cursor < g_callEnd && ((strncmp(g_lines[g_cursor], "  o ", 4) == 0 && !ClosingLine(g_lines[g_cursor])) || strncmp(g_lines[g_cursor], "  b ", 4) == 0)) {
        if (g_lines[g_cursor][2] == 'b') {
            char* name = g_lines[g_cursor] + 4;
            char* sp = strchr(name, ' ');
            if (sp != nullptr && g_beforeCount < MAX_OUTS) {
                *sp = '\0';
                g_beforeNames[g_beforeCount] = name;
                g_beforeValues[g_beforeCount] = sp + 1;
                g_beforeCount++;
            }
            g_cursor++;
            continue;
        }
        if (g_outCount < MAX_OUTS) {
            char* name = g_lines[g_cursor] + 4;
            char* sp = strchr(name, ' ');
            if (sp != nullptr) {
                *sp = '\0';
                g_outNames[g_outCount] = name;
                g_outValues[g_outCount] = sp + 1;
                g_outCount++;
            }
        }
        g_cursor++;
    }
    for (int i = 0; i < g_outCount; i++) {
        const ReplayField* f = RngField(g_outNames[i]);
        if (f != nullptr)
            RestoreField(*f, g_outValues[i]);
        else
            RestoreImportObject(g_outNames[i], g_outValues[i]);
    }
    unsigned n = (unsigned)strlen(actual.Data());
    if (strncmp(recorded, actual.Data(), n) == 0 && (recorded[n] == '\0' || strncmp(recorded + n, " = ", 3) == 0))
        return recorded[n] == '\0' ? "" : recorded + n + 3;
    if (!EqualButUnread(recorded, actual.Data()))
        Diff(key, recorded, actual.Data());
    return LastResult(recorded);
}

const char* OutValue(const char* suffix)
{
    for (int i = 0; i < g_outCount; i++) {
        unsigned n = (unsigned)strlen(g_outNames[i]);
        unsigned m = (unsigned)strlen(suffix);
        if (n >= m && strcmp(g_outNames[i] + n - m, suffix) == 0)
            return g_outValues[i];
    }
    return nullptr;
}

const char* OutExact(const char* name)
{
    for (int i = 0; i < g_outCount; i++) {
        if (strcmp(g_outNames[i], name) == 0)
            return g_outValues[i];
    }
    return nullptr;
}

void ApplyVec3(Vec3* dst, const char* name)
{
    const char* value = OutExact(name);
    if (value != nullptr && dst != nullptr)
        sscanf(value, "(%d,%d,%d)", &dst->x, &dst->y, &dst->z);
}

void StartImport(ReplayText& t, const char* name)
{
    t.Clear();
    t.Add(name);
}

void StartImport(ReplayText& t, int id)
{
    StartImport(t, kReplayImportNames[id]);
}

void Space(ReplayText& t)
{
    t.AddChar(' ');
}

template <class T> T ParseResult(const char* text)
{
    return (T)atoi(text);
}

template <> float ParseResult<float>(const char* text)
{
    unsigned bits = ParseF32(text);
    float f;
    memcpy(&f, &bits, 4);
    return f;
}

template <> double ParseResult<double>(const char* text)
{
    return ParseF64(text);
}

void AddBytes(ReplayText& t, const void* p, int length)
{
    ReplayBytes(t, p, length);
}

void ApplyBytes(void* dst, int length, const char* name)
{
    const char* value = OutValue(name);
    if (value != nullptr && dst != nullptr)
        ReplayParseBytes(value, dst, length);
}

void AddInOut(ReplayText& t, unsigned kind, const void* p)
{
    ReplayAddInOut(t, kind, p, g_objects);
}

void ApplyInOut(void* dst, const char* name, unsigned kind)
{
    const char* value = OutExact(name);
    if (value != nullptr && dst != nullptr)
        ReplayParseValue(kind, value, dst, g_objects, nullptr);
}

template <class T> void AddInOutScalar(ReplayText& t, const T* p)
{
    ReplayAddInOutScalar(t, p);
}

template <class T> void ApplyValues(std::vector<T, std::allocator<T> >* out, const char* name, unsigned kind)
{
    const char* list = OutExact(name);
    if (list == nullptr || out == nullptr || list[0] != '[')
        return;
    char piece[128];
    for (const char* cursor = list; ReplayNextListValue(&cursor, piece, sizeof piece);) {
        T value = T();
        if (ReplayParseValue(kind, piece, &value, g_objects, nullptr))
            out->push_back(value);
    }
}

bool ApplyReachField(void* object, unsigned kind, const char* suffix)
{
    const char* value = OutValue(suffix);
    const ReplayObjectField* f = ReplayFindObjectField(kind, suffix + 1);
    if (object != nullptr && value != nullptr && f != nullptr && f->kind == RK_BLOB)
        return ReplayApplyBlob(*f, object, suffix, value, g_known, true, g_objects, AllocObject);
    if (object == nullptr || value == nullptr || f == nullptr)
        return true;
    MapLoadTarget(*f, value);
    return ReplayRestoreObjectField(*f, object, value, g_objects, AllocObject);
}

void ApplyReach(void* object, unsigned kind, const char* suffix)
{
    if (!ApplyReachField(object, kind, suffix))
        Fatal(g_file, g_callIndex, "a value that cannot be restored", suffix);
    MapLoadObject(object, kind);
    SyncLoadObjects();
}

const char* ArgLine(const char* name)
{
    size_t n = strlen(name);
    for (int k = g_callFirst + 1; k < g_callEnd; k++) {
        const char* line = g_lines[k];
        if (strncmp(line, "  a ", 4) == 0 && strncmp(line + 4, name, n) == 0 && line[4 + n] == ' ')
            return line + 5 + n;
    }
    return nullptr;
}

const char* OutLine(const char* name)
{
    size_t n = strlen(name);
    const char* found = nullptr;
    for (int k = g_callFirst + 1; k < g_callEnd; k++) {
        const char* line = g_lines[k];
        if (strncmp(line, "  o ", 4) == 0 && strncmp(line + 4, name, n) == 0 && line[4 + n] == ' ')
            found = line + 5 + n;
    }
    return found;
}

void ParseScalar(const char* v, int& out) { out = atoi(v); }
void ParseScalar(const char* v, long& out) { out = atol(v); }
void ParseScalar(const char* v, short& out) { out = (short)atoi(v); }
void ParseScalar(const char* v, char& out) { out = (char)atoi(v); }
void ParseScalar(const char* v, bool& out) { out = atoi(v) != 0; }
void ParseScalar(const char* v, unsigned& out) { out = strtoul(v, nullptr, 10); }
void ParseScalar(const char* v, unsigned short& out) { out = (unsigned short)strtoul(v, nullptr, 10); }
void ParseScalar(const char* v, unsigned char& out) { out = (unsigned char)strtoul(v, nullptr, 10); }
void ParseScalar(const char* v, float& out)
{
    unsigned bits = ParseF32(v);
    memcpy(&out, &bits, 4);
}
void ParseScalar(const char* v, double& out) { out = ParseF64(v); }
void ParseScalar(const char* v, Point16& out)
{
    int x = 0;
    int y = 0;
    sscanf(v, "(%d,%d)", &x, &y);
    out.x = (short)x;
    out.y = (short)y;
}
void ParseScalar(const char* v, Vec3& out)
{
    if (sscanf(v, "(%d,%d,%d)", &out.x, &out.y, &out.z) == 2) {
        out.z = out.y;
        out.y = 0;
    }
}

template <class T> T ArgScalar(const char* name)
{
    T value = T();
    const char* text = ArgLine(name);
    if (text != nullptr)
        ParseScalar(text, value);
    return value;
}

RandStream ArgStream(const char* name)
{
    const char* value = ArgLine(name);
    if (value == nullptr || strcmp(value, "main") == 0)
        return RAND_STREAM_MAIN;
    return strcmp(value, "load") == 0 ? RAND_STREAM_LOAD : (RandStream)atoi(value);
}

void ArgText(const char* name, char* out, unsigned size)
{
    const char* value = ArgLine(name);
    out[0] = '\0';
    if (value != nullptr)
        ParseQuoted(value, out, size);
}

void* ArgHandle(const char*)
{
    return &g_bank;
}

void ArgPending()
{
}

void* ArgObject(const char* name, unsigned kind)
{
    const char* value = ArgLine(name);
    unsigned found = 0;
    void* p = value == nullptr ? nullptr : ReplayResolveObject(value, &found, g_objects, AllocObject);
    if (p != nullptr && found == kind && g_rootCount < REPLAY_MAX_ROOTS) {
        g_rootKinds[g_rootCount] = (unsigned short)kind;
        g_rootObjects[g_rootCount++] = p;
    }
    return p;
}

void* ArgWeapon(const char* name)
{
    const char* value = ArgLine(name);
    void* p = value == nullptr ? nullptr : ReplayParseWeapon(value);
    if (p != nullptr && g_rootCount < REPLAY_MAX_ROOTS) {
        g_rootKinds[g_rootCount] = ROK_WEAPONDEF;
        g_rootObjects[g_rootCount++] = p;
    }
    return p;
}

void* ArgLoad(const char* name, unsigned load)
{
    const char* value = ArgLine(name);
    if (value == nullptr)
        return nullptr;
    bool found = false;
    const void* p = ReplayParseLoad(load, value, &found);
    if (!found)
        Fatal(g_file, g_callIndex, "a load that no load line names", value);
    return (void*)p;
}

const void* ParseLoadResult(unsigned load, const char* text)
{
    bool found = false;
    const void* p = ReplayParseLoad(load, text, &found);
    if (!found)
        Fatal(g_file, g_callIndex, "a load that no load line names", text);
    return p;
}

char g_refusedKey[96];

bool ParseStructFields(const char* name, unsigned kind, void* object)
{
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[i];
        if (f.object != kind)
            continue;
        char key[96];
        sprintf(key, "%s.%s", name, f.name);
        const char* value = ArgLine(key);
        if (value == nullptr)
            continue;
        if (!ReplayRestoreObjectField(f, object, value, g_objects, AllocObject)) {
            strcpy(g_refusedKey, key);
            return false;
        }
        void* target = nullptr;
        if (f.kind == RK_REF) {
            unsigned offset = *(unsigned*)((unsigned char*)object + f.offset);
            target = offset == 0 ? nullptr : (char*)SimBlockBase() + offset;
        }
        else if (f.kind == RK_PTR)
            target = *(void**)((unsigned char*)object + f.offset);
        if (target != nullptr && g_rootCount < REPLAY_MAX_ROOTS) {
            g_rootKinds[g_rootCount] = f.target;
            g_rootObjects[g_rootCount++] = target;
        }
    }
    return true;
}

void ParseStruct(const char* name, unsigned kind, void* object)
{
    if (!ParseStructFields(name, kind, object))
        Fatal(g_file, g_callIndex, "a value that cannot be restored", g_refusedKey);
}

bool ParseEventFields(const char* name, unsigned table, void* buffer)
{
    char key[96];
    sprintf(key, "%s.kind", name);
    const char* value = ArgLine(key);
    for (unsigned i = 0; value != nullptr && i < kReplayObjectFieldCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[i];
        if (f.object == kReplayEventTables[table].header && strcmp(f.name, "kind") == 0 && !ReplayRestoreObjectField(f, buffer, value, g_objects, AllocObject)) {
            strcpy(g_refusedKey, key);
            return false;
        }
    }
    return ParseStructFields(name, ReplayEventObject(table, *(int*)buffer), buffer);
}

void ParseEvent(const char* name, unsigned table, void* buffer)
{
    if (!ParseEventFields(name, table, buffer))
        Fatal(g_file, g_callIndex, "a value that cannot be restored", g_refusedKey);
}

void* ParseStructOpt(const char* name, unsigned kind, void* storage)
{
    const char* value = ArgLine(name);
    if (value != nullptr && strcmp(value, "null") == 0)
        return nullptr;
    ParseStruct(name, kind, storage);
    return storage;
}

bool ParseNullOut(const char* name)
{
    const char* value = ArgLine(name);
    return value != nullptr && strcmp(value, "null") == 0;
}

struct StructsHolder {
    StructsHolder() : data(nullptr) {}
    ~StructsHolder() { free(data); }
    void Parse(const char* name, unsigned kind, int count)
    {
        const char* value = ArgLine(name);
        if (value != nullptr && strcmp(value, "null") == 0)
            return;
        unsigned size = kReplayObjectKinds[kind].size;
        if (count < 0 || count > (1 << 20))
            Fatal(g_file, g_callIndex, "an array of structs that is too long", name);
        data = (unsigned char*)calloc((unsigned)count + 1, size);
        for (int i = 0; i < count; i++) {
            char element[96];
            sprintf(element, "%.80s[%d]", name, i);
            ParseStruct(element, kind, data + (unsigned)i * size);
        }
    }
    unsigned char* data;

private:
    StructsHolder(const StructsHolder&);
    StructsHolder& operator=(const StructsHolder&);
};

void ParseValueArg(const char* name, unsigned kind, void* object)
{
    const char* value = ArgLine(name);
    if (value != nullptr)
        ReplayParseValue(kind, value, object, g_objects, nullptr);
}

void AddValueStruct(ReplayText& t, unsigned kind, const void* object)
{
    ReplayAddValue(t, kind, object, g_objects);
}

void AddStructIn(ReplayText& t, unsigned kind, const void* p)
{
    if (p == nullptr)
        t.Add("null");
    else
        AddValueStruct(t, kind, p);
}

template <class T> T ParseValueText(unsigned kind, const char* text)
{
    T value = T();
    ReplayParseValue(kind, text, &value, g_objects, nullptr);
    return value;
}

void CompareValueResult(unsigned kind, const void* object)
{
    ReplayText value;
    AddValueStruct(value, kind, object);
    HashText(value.Data());
    CoreClosing("result", value.Data());
    const char* recorded = OutLine("result");
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff("result", recorded, value.Data());
    value.Free();
}

void CompareStruct(const char* name, unsigned kind, const void* object)
{
    ReplayText value;
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[i];
        if (f.object != kind)
            continue;
        char key[96];
        sprintf(key, "%s.%s", name, f.name);
        ReplayFormatObjectField(f, object, g_objects, false, value);
        HashText(value.Data());
        CoreClosing(key, value.Data());
        const char* recorded = OutLine(key);
        if (recorded == nullptr)
            recorded = "no such line";
        if (strcmp(recorded, value.Data()) != 0)
            Diff(key, recorded, value.Data());
    }
    value.Free();
}

void* ParseInOut(const char* name, unsigned kind, void* storage)
{
    const char* value = ArgLine(name);
    if (value == nullptr || strcmp(value, "null") == 0)
        return nullptr;
    ReplayParseValue(kind, value, storage, g_objects, nullptr);
    return storage;
}

void CompareInOut(const char* name, unsigned kind, const void* object)
{
    const char* recorded = OutLine(name);
    if (object == nullptr) {
        if (recorded != nullptr)
            Diff(name, recorded, "no line");
        return;
    }
    ReplayText value;
    ReplayAddValue(value, kind, object, g_objects);
    HashText(value.Data());
    CoreClosing(name, value.Data());
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff(name, recorded, value.Data());
    value.Free();
}

bool InnerOfParentheses(const char* text, char* inner, unsigned size)
{
    size_t n = strlen(text);
    if (n < 2 || text[0] != '(' || text[n - 1] != ')' || n - 1 > size)
        return false;
    memcpy(inner, text + 1, n - 2);
    inner[n - 2] = '\0';
    return true;
}

template <class T> void ApplyOutScalar(T* dst, const char* name)
{
    const char* value = OutExact(name);
    if (value != nullptr && dst != nullptr)
        ParseScalar(value, *dst);
}

void CompareBytes(const char* name, const unsigned char* bytes, int count, int length)
{
    if (count > length || bytes == nullptr)
        count = bytes == nullptr ? 0 : length;
    const char* recorded = OutLine(name);
    if (count <= 0) {
        if (recorded != nullptr)
            Diff(name, recorded, "no line");
        return;
    }
    ReplayText value;
    ReplayBytes(value, bytes, count);
    HashText(value.Data());
    CoreClosing(name, value.Data());
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff(name, recorded, value.Data());
    value.Free();
}

struct ScratchHolder {
    void* data;
    ScratchHolder() : data(nullptr) {}
    ~ScratchHolder() { free(data); }
    void Reserve(size_t bytes) { data = calloc(bytes > 0 ? bytes : 1, 1); }
};

template <class T> void ApplyInOutScalar(T* dst, const char* name)
{
    const char* value = OutExact(name);
    char inner[64];
    if (value != nullptr && dst != nullptr && InnerOfParentheses(value, inner, sizeof inner))
        ParseScalar(inner, *dst);
}

template <class T> T* ParseInOutScalar(const char* name, T* storage)
{
    const char* value = ArgLine(name);
    char inner[64];
    if (value == nullptr || !InnerOfParentheses(value, inner, sizeof inner))
        return nullptr;
    ParseScalar(inner, *storage);
    return storage;
}

template <class T> void CompareInOutScalar(const char* name, const T* object)
{
    const char* recorded = OutLine(name);
    if (object == nullptr) {
        if (recorded != nullptr)
            Diff(name, recorded, "no line");
        return;
    }
    ReplayText value;
    ReplayAddInOutScalar(value, object);
    HashText(value.Data());
    CoreClosing(name, value.Data());
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff(name, recorded, value.Data());
    value.Free();
}

void BindObjectResult(unsigned kind, void* object)
{
    if (object == nullptr)
        return;
    unsigned actual = ReplayConcreteKind(kind, object);
    if (actual == kReplayObjectKindCount)
        Fatal(g_file, g_callIndex, "an object result whose kind its vtable does not tell", ReplayFailure() != nullptr ? ReplayFailure() : kReplayObjectKinds[kind].name);
    const char* recorded = OutLine("result");
    const char* hash = recorded == nullptr ? nullptr : strchr(recorded, '#');
    size_t length = strlen(kReplayObjectKinds[actual].name);
    if (g_objects.Find(actual, object) == 0 && hash != nullptr && (size_t)(hash - recorded) == length && strncmp(recorded, kReplayObjectKinds[actual].name, length) == 0
        && kReplayObjectKinds[actual].table == RT_ORDER && hash[1] >= '1' && hash[1] <= '9') {
        unsigned ordinal = (unsigned)atoi(hash + 1);
        if (ordinal <= REPLAY_MAX_OBJECTS && g_objects.Pointer(actual, ordinal) == nullptr)
            g_objects.Bind(actual, ordinal, object);
    }
    if (g_rootCount < REPLAY_MAX_ROOTS) {
        g_rootKinds[g_rootCount] = (unsigned short)actual;
        g_rootObjects[g_rootCount++] = object;
    }
}

void CompareObjectResult(unsigned kind, const void* object)
{
    ReplayText value;
    ReplayAddObjectName(value, g_objects, kind, object, false);
    HashText(value.Data());
    CoreClosing("result", value.Data());
    const char* recorded = OutLine("result");
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff("result", recorded, value.Data());
    value.Free();
}

void CompareUnitBuffer(const char* name, const Ref<Unit>* units, int count, int capacity)
{
    if (count > capacity)
        count = capacity;
    ReplayText value;
    value.AddChar('[');
    for (int i = 0; i < count; i++) {
        if (i != 0)
            value.AddChar(',');
        ReplayAddObjectName(value, g_objects, ROK_UNIT, units[i].get(), false);
    }
    value.AddChar(']');
    HashText(value.Data());
    CoreClosing(name, value.Data());
    const char* recorded = OutLine(name);
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff(name, recorded, value.Data());
    value.Free();
}

void CompareRegionResult(const void* result)
{
    const char* value = result != nullptr && result == g_region.ptr ? "region" : "null";
    HashText(value);
    const char* recorded = OutLine("result");
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value) != 0)
        Diff("result", recorded, value);
}

template <class T> void CompareResult(T result)
{
    ReplayText value;
    ReplayValue(value, result);
    HashText(value.Data());
    CoreClosing("result", value.Data());
    const char* recorded = OutLine("result");
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff("result", recorded, value.Data());
    value.Free();
}

void CompareWeaponResult(const void* weapon)
{
    ReplayText value;
    ReplayAddWeapon(value, weapon);
    HashText(value.Data());
    const char* recorded = OutLine("result");
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff("result", recorded, value.Data());
    value.Free();
}

void CompareLoadResult(unsigned load, const void* p)
{
    ReplayText value;
    ReplayAddLoadName(value, g_objects, load, p, false);
    HashText(value.Data());
    const char* recorded = OutLine("result");
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff("result", recorded, value.Data());
    value.Free();
}

struct ArgvHolder {
    CommandArgs args;
    char text[0x14][128];
};

void ParseArgv(ArgvHolder& holder)
{
    memset(&holder, 0, sizeof holder);
    const char* count = ArgLine("args.count");
    holder.args.count = count == nullptr ? 0 : atoi(count);
    for (int i = 0; i < 0x14; i++) {
        char key[32];
        sprintf(key, "args[%d]", i);
        const char* value = ArgLine(key);
        if (value != nullptr) {
            ParseQuoted(value, holder.text[i], sizeof holder.text[i]);
            holder.args.args[i] = holder.text[i];
        }
    }
}

void SetClosing(const char* a, const char* b = nullptr, const char* c = nullptr, const char* d = nullptr, const char* e = nullptr, const char* f = nullptr, const char* g = nullptr, const char* h = nullptr)
{
    g_closing[0] = a;
    g_closing[1] = b;
    g_closing[2] = c;
    g_closing[3] = d;
    g_closing[4] = e;
    g_closing[5] = f;
    g_closing[6] = g;
    g_closing[7] = h;
}

}

namespace {

void AddAllocName(ReplayText& t, const void* p)
{
    if (p == nullptr) {
        t.Add("null");
        return;
    }
    if (g_objects.Find(ROK_ALLOC, p) != 0) {
        ReplayAddObjectName(t, g_objects, ROK_ALLOC, p, false);
        return;
    }
    for (unsigned i = 0; i < kReplayAllocTagCount; i++) {
        if (g_objects.Find(kReplayAllocTags[i].kind, p) != 0) {
            ReplayAddObjectName(t, g_objects, kReplayAllocTags[i].kind, p, false);
            return;
        }
    }
    t.Add("alloc#?");
}

void* AllocAnswer(const char* result, unsigned size)
{
    if (strcmp(result, "null") == 0)
        return nullptr;
    void* p = SimAlloc(size);
    if (g_forgetAllocations)
        g_objects.Forget(p);
    const char* hash = strchr(result, '#');
    int kind = hash == nullptr ? -1 : ReplayFindObjectKind(result, (unsigned)(hash - result));
    if (kind >= 0 && hash[1] != '?')
        g_objects.Bind((unsigned)kind, (unsigned)atoi(hash + 1), p);
    return p;
}

void* ParseHandle(unsigned kind, const char* text)
{
    if (kind == OBJ_RECORD)
        return ParseRecord(text);
    return strcmp(text, "null") == 0 || kind == OBJ_FILE ? nullptr : &g_bank;
}

enum { MAX_SNAPSHOTS = 64, CAPTURE_ARENA_BYTES = 1 << 20 };
unsigned char g_captureArena[CAPTURE_ARENA_BYTES];
unsigned g_captureUsed;
unsigned char* g_snapshotData[MAX_SNAPSHOTS];
unsigned g_snapshotBytes[MAX_SNAPSHOTS];

void SnapshotRegister(const char* result, void* buffer, unsigned bytes)
{
    if (strncmp(result, "snapshot#", 9) != 0)
        return;
    unsigned ordinal = (unsigned)atoi(result + 9);
    if (ordinal == 0 || ordinal > MAX_SNAPSHOTS)
        Fatal(g_file, g_callIndex, "a snapshot that the replayer cannot register", result);
    g_snapshotData[ordinal - 1] = (unsigned char*)buffer;
    g_snapshotBytes[ordinal - 1] = bytes;
}

void* SnapshotAnswer(const char* result, unsigned bytes, unsigned* offset)
{
    if (strncmp(result, "snapshot#", 9) != 0)
        return nullptr;
    unsigned ordinal = (unsigned)atoi(result + 9);
    const char* plus = strchr(result, '+');
    if (plus != nullptr)
        g_captureUsed = (unsigned)atoi(plus + 1);
    unsigned size = (bytes + 7u) & ~7u;
    if (ordinal == 0 || ordinal > MAX_SNAPSHOTS || g_captureUsed + size > CAPTURE_ARENA_BYTES)
        Fatal(g_file, g_callIndex, "a snapshot that the replayer cannot allocate", result);
    *offset = g_captureUsed;
    unsigned char* at = g_captureArena + g_captureUsed;
    memset(at, 0, size);
    g_captureUsed += size;
    g_snapshotData[ordinal - 1] = at;
    g_snapshotBytes[ordinal - 1] = bytes;
    return at;
}

}

namespace {

void PlayNestedCalls(const char* import);


const char* EnterNested(const ReplayText& actual)
{
    const char* space = strchr(actual.Data(), ' ');
    size_t n = space == nullptr ? strlen(actual.Data()) : (size_t)(space - actual.Data());
    int at = g_cursor;
    while (at < g_callEnd && !(strncmp(g_lines[at], "  i ", 4) == 0 && strncmp(g_lines[at] + 4, actual.Data(), n) == 0 && (g_lines[at][4 + n] == ' ' || g_lines[at][4 + n] == '\0')))
        at++;
    g_nestedFrom = g_cursor;
    if (at < g_callEnd)
        g_cursor = at;
    const char* result = Enter(actual);
    g_nestedAfter = g_cursor;
    return result;
}

}

#include "replay_services.h"

int CommandArgs::Count() { return count; }

namespace {

struct NestedScope {
    NestedScope() : count(g_cbCount), outer(g_cbImportIndex), after(g_nestedAfter)
    {
        memcpy(line, g_cbLines, (unsigned)g_cbCount * sizeof g_cbLines[0]);
        g_cursor = g_nestedFrom;
    }
    ~NestedScope() { g_cursor = after; }
    void Restore() const { g_cbImportIndex = outer; }
    const char* line[MAX_CALLBACK_LINES];
    int count;
    int outer;
    int after;

private:
    NestedScope(const NestedScope&);
    NestedScope& operator=(const NestedScope&);
};

const char* NestedName(const char* line, const char** args)
{
    const char* end = strchr(line, ' ');
    *args = end == nullptr ? line + strlen(line) : end;
    return line;
}

void ParseArgsList(const char** args, ArgvHolder& holder)
{
    memset(&holder, 0, sizeof holder);
    while (**args == ' ')
        (*args)++;
    if (**args != '[') {
        while (**args != '\0' && **args != ' ')
            (*args)++;
        return;
    }
    (*args)++;
    while (**args != ']' && **args != '\0') {
        if (**args == ',') {
            (*args)++;
            continue;
        }
        if (holder.args.count < 0x14) {
            *args = ParseQuoted(*args, holder.text[holder.args.count], sizeof holder.text[0]);
            holder.args.args[holder.args.count] = holder.text[holder.args.count];
            holder.args.count++;
        } else {
            *args = ParseQuoted(*args, holder.text[0], sizeof holder.text[0]);
        }
    }
    if (**args == ']')
        (*args)++;
}

void CallbackKey(char* key, const char* import, int k)
{
    sprintf(key, "import %d %s.c[%d]", g_cbImportIndex, import, k);
}

const char* CallbackMethod(int k, unsigned handle, const void* object, const char* label, const char* import, const char** args)
{
    const char* line = g_cbLines[k];
    ReplayText want;
    AddObject(want, handle, object);
    unsigned n = (unsigned)strlen(want.Data());
    bool same = strncmp(line, want.Data(), n) == 0 && line[n] == '.';
    if (!same) {
        char key[96];
        char recorded[48];
        const char* dot = strchr(line, '.');
        unsigned length = dot == nullptr ? (unsigned)strlen(line) : (unsigned)(dot - line);
        if (length >= sizeof recorded)
            length = (unsigned)sizeof recorded - 1;
        memcpy(recorded, line, length);
        recorded[length] = '\0';
        CallbackKey(key, import, k);
        Diff(key, recorded, want.Data());
        want.Free();
        return nullptr;
    }
    want.Free();
    (void)label;
    const char* method = line + n + 1;
    const char* end = strchr(method, ' ');
    *args = end == nullptr ? method + strlen(method) : end;
    return method;
}

bool CallbackIs(const char* method, const char* name)
{
    size_t n = strlen(name);
    return strncmp(method, name, n) == 0 && (method[n] == ' ' || method[n] == '\0');
}

void CallbackToken(const char** p, char* out, unsigned size)
{
    const char* q = *p;
    while (*q == ' ')
        q++;
    unsigned n = 0;
    int depth = 0;
    bool quoted = false;
    for (; *q != '\0'; q++) {
        if (quoted) {
            if (*q == '\\' && q[1] != '\0') {
                if (n + 2 < size) {
                    out[n++] = *q;
                    out[n++] = q[1];
                }
                q++;
                continue;
            }
            quoted = *q != '"';
        } else if (*q == '"') {
            quoted = true;
        } else if (*q == '(') {
            depth++;
        } else if (*q == ')') {
            depth--;
        } else if (*q == ' ' && depth <= 0) {
            break;
        }
        if (n + 1 < size)
            out[n++] = *q;
    }
    out[n] = '\0';
    *p = q;
}

template <class T> T CallbackScalar(const char** args)
{
    char token[96];
    CallbackToken(args, token, sizeof token);
    T value = T();
    ParseScalar(token, value);
    return value;
}

void CallbackText(const char** args, char* out, unsigned size)
{
    while (**args == ' ')
        (*args)++;
    *args = ParseQuoted(*args, out, size);
}

void* CallbackObject(const char** args, unsigned kind)
{
    char token[96];
    CallbackToken(args, token, sizeof token);
    if (strcmp(token, "null") == 0)
        return nullptr;
    unsigned found = 0;
    void* p = ReplayResolveObject(token, &found, g_objects, AllocObject);
    if (p == nullptr || !ReplayKindFits(kind, found))
        Fatal(g_file, g_callIndex, "a callback argument that is no object of its kind", token);
    return p;
}

template <class T> T CallbackValue(const char** args, unsigned kind)
{
    char token[256];
    CallbackToken(args, token, sizeof token);
    return ParseValueText<T>(kind, token);
}

void CallbackDone(const char* import, int k, const char* rest, const char* have)
{
    const char* recorded = strncmp(rest, " = ", 3) == 0 ? rest + 3 : "";
    const char* replayed = have == nullptr ? "" : have;
    HashText(replayed);
    if (strcmp(recorded, replayed) != 0) {
        char key[96];
        CallbackKey(key, import, k);
        Diff(key, recorded[0] == '\0' ? "no result" : recorded, replayed[0] == '\0' ? "no result" : replayed);
    }
}

void CallbackUnknown(const char*, int, const char* method)
{
    Fatal(g_file, g_callIndex, "a callback method that its interface does not have", method);
}

enum { MAX_HELD = 16, MAX_HELD_CALLS = 64 };

struct HeldCallback {
    unsigned callback;
    unsigned ordinal;
    const void* object;
    int count;
    char* call[MAX_HELD_CALLS];
};

HeldCallback g_held[MAX_HELD];
int g_heldCount;

void ResetHeld()
{
    for (int i = 0; i < g_heldCount; i++) {
        for (int k = 0; k < g_held[i].count; k++)
            free(g_held[i].call[k]);
    }
    g_heldCount = 0;
}

void BeginHeld()
{
    ResetHeld();
    ReplayHandles().Reset();
}

HeldCallback* FindHeld(const void* object)
{
    for (int i = 0; i < g_heldCount; i++) {
        if (g_held[i].object == object)
            return &g_held[i];
    }
    return nullptr;
}

void HeldPrefix(const HeldCallback& h, char* out)
{
    sprintf(out, "%s#%u.", kReplayCallbacks[h.callback].handle, h.ordinal);
}

const char* RecordedHeldCall(const HeldCallback& h, int k)
{
    char prefix[64];
    HeldPrefix(h, prefix);
    size_t n = strlen(prefix);
    int seen = 0;
    for (int i = g_callFirst + 1; i < g_callEnd; i++) {
        if (strncmp(g_lines[i], "  c ", 4) == 0 && strncmp(g_lines[i] + 4, prefix, n) == 0 && seen++ == k)
            return g_lines[i] + 4;
    }
    return nullptr;
}

bool HeldMatches(const char* recorded, const char* made)
{
    size_t n = strlen(made);
    return strncmp(recorded, made, n) == 0 && (recorded[n] == '\0' || strncmp(recorded + n, " = ", 3) == 0);
}

class HeldCall {
public:
    HeldCall(unsigned callback, const void* object, const char* method) : entry(FindHeld(object))
    {
        if (entry == nullptr)
            Fatal(g_file, g_callIndex, "a call on a callback that no r or a line of the call names", method);
        char prefix[64];
        HeldPrefix(*entry, prefix);
        (void)callback;
        text.Add(prefix);
        text.Add(method);
    }
    ~HeldCall() { text.Free(); }
    void Space() { text.AddChar(' '); }
    ReplayText& Text() { return text; }
    const char* Finish()
    {
        if (entry->count == MAX_HELD_CALLS)
            Fatal(g_file, g_callIndex, "more calls on one callback than the replayer keeps", text.Data());
        char* made = (char*)malloc(text.Size() + 1);
        strcpy(made, text.Data());
        int k = entry->count++;
        entry->call[k] = made;
        HashText(made);
        const char* recorded = RecordedHeldCall(*entry, k);
        if (recorded == nullptr || !HeldMatches(recorded, made))
            return "";
        const char* result = recorded + strlen(made);
        return result[0] == '\0' ? result : result + 3;
    }

private:
    HeldCall(const HeldCall&);
    HeldCall& operator=(const HeldCall&);
    HeldCallback* entry;
    ReplayText text;
};

}

#include "replay_callbacks.h"

namespace {

void* BindHeld(unsigned callback, unsigned ordinal, ReplayAllocFn alloc)
{
    for (int i = 0; i < g_heldCount; i++) {
        if (g_held[i].callback == callback && g_held[i].ordinal == ordinal)
            return (void*)g_held[i].object;
    }
    if (g_heldCount == MAX_HELD)
        Fatal(g_file, g_callIndex, "more callbacks held in one call than the replayer keeps", kReplayCallbacks[callback].name);
    void* object = NewHeldCallback(callback, alloc(HeldCallbackSize(callback)));
    HeldCallback& h = g_held[g_heldCount++];
    h.callback = callback;
    h.ordinal = ordinal;
    h.object = object;
    h.count = 0;
    ReplayHandles().Bind(REPLAY_HANDLE_CALLBACK + callback, ordinal, object);
    return object;
}

void* ArgCallback(const char* name, unsigned callback)
{
    const char* value = ArgLine(name);
    if (value == nullptr || strcmp(value, "null") == 0)
        return nullptr;
    const char* handle = kReplayCallbacks[callback].handle;
    size_t n = strlen(handle);
    unsigned ordinal = strncmp(value, handle, n) == 0 && value[n] == '#' ? (unsigned)atoi(value + n + 1) : 0;
    if (ordinal == 0)
        Fatal(g_file, g_callIndex, "an argument that is no callback of its interface", value);
    return BindHeld(callback, ordinal, AllocObject);
}

void CompareHeld()
{
    for (int k = g_callFirst + 1; k < g_callEnd; k++) {
        if (strncmp(g_lines[k], "  c ", 4) != 0)
            continue;
        bool named = false;
        for (int i = 0; i < g_heldCount; i++) {
            char prefix[64];
            HeldPrefix(g_held[i], prefix);
            named = named || strncmp(g_lines[k] + 4, prefix, strlen(prefix)) == 0;
        }
        if (!named)
            Fatal(g_file, g_callIndex, "a c line for a callback that no r or a line of the call names", g_lines[k]);
    }
    for (int i = 0; i < g_heldCount; i++) {
        const HeldCallback& h = g_held[i];
        char prefix[64];
        HeldPrefix(h, prefix);
        size_t n = strlen(prefix);
        for (int k = 0;; k++) {
            const char* recorded = RecordedHeldCall(h, k);
            const char* made = k < h.count ? h.call[k] : nullptr;
            if (recorded == nullptr && made == nullptr)
                break;
            if (recorded != nullptr && made != nullptr && HeldMatches(recorded, made))
                continue;
            const char* named = recorded != nullptr ? recorded : made;
            const char* space = strchr(named + n, ' ');
            char key[96];
            sprintf(key, "%.*s", (int)(space == nullptr ? strlen(named) : (size_t)(space - named)), named);
            Diff(key, recorded == nullptr ? "no call (extra)" : recorded + n, made == nullptr ? "no call (missing)" : made + n);
        }
    }
}

bool RestoreObjectLine(const char* name, const char* value, const char* file, const ReplaySlotSets* sets, const char* line);

void ApplyUnits(std::vector<Unit*, std::allocator<Unit*> >* out, const char* name)
{
    for (int k = 0; k < g_outCount; k++) {
        const char* key = g_outNames[k];
        if (strchr(key, '#') == nullptr || strchr(key, '.') == nullptr)
            continue;
        RestoreObjectLine(key, g_outValues[k], g_file, nullptr, key);
    }
    const char* list = OutExact(name);
    if (list == nullptr || out == nullptr || list[0] != '[')
        return;
    for (const char* p = list + 1; *p != '\0' && *p != ']';) {
        char token[48];
        unsigned n = 0;
        while (*p != '\0' && *p != ',' && *p != ']' && n + 1 < sizeof token)
            token[n++] = *p++;
        token[n] = '\0';
        if (*p == ',')
            p++;
        out->push_back((Unit*)ReplayResolveObject(token, nullptr, g_objects, AllocObject));
    }
}

struct ListAnswer {
    bool valid;
    bool over;
    int capacity;
    char key[192];
    std::vector<Unit*, std::allocator<Unit*> > units;
} g_list;

int EnterList(ReplayText& t, Ref<Unit>* out, int capacity)
{
    bool repeat = g_list.valid && g_list.over && capacity > g_list.capacity && strcmp(g_list.key, t.Data()) == 0;
    if (!repeat) {
        int before = g_diffs;
        g_list.valid = false;
        g_list.units.clear();
        Enter(t);
        ApplyUnits(&g_list.units, "out");
        g_list.valid = g_diffs == before && strlen(t.Data()) < sizeof g_list.key;
        if (g_list.valid)
            strcpy(g_list.key, t.Data());
    }
    int count = (int)g_list.units.size();
    for (int k = 0; k < count && k < capacity; k++)
        out[k] = Ref<Unit>(g_list.units[(unsigned)k]);
    g_list.over = count > capacity;
    g_list.capacity = capacity;
    return count;
}

unsigned char __stdcall ChooseWeaponSlot(Unit* unit)
{
    return unit->ChooseWeapon();
}

int __stdcall Native_fixture_Query(FixtureAmount* first, FixtureAmount* second)
{
    if (first != nullptr)
        first->amount += 3;
    if (second != nullptr)
        second->amount = 0;
    return first != nullptr ? first->amount : -1;
}

int __stdcall Native_fixture_Locate(FixtureAmount* first, FixtureAmount* second)
{
    if (first == nullptr)
        return 0;
    first->amount = 4;
    if (second != nullptr)
        second->amount = 9;
    return 1;
}

bool g_makeGoalB;

unsigned int __stdcall Native_fixture_MakeGoal()
{
    unsigned size = g_makeGoalB ? sizeof(FixtureGoalB) : sizeof(FixtureGoalA);
    void* at = SimAlloc(size);
    memset(at, 0, size);
    FixtureGoalBase* goal = g_makeGoalB ? (FixtureGoalBase*)new (at) FixtureGoalB() : new (at) FixtureGoalA();
    goal->ticks = 4;
    return ReplayRefOffset(goal);
}

int __stdcall Native_fixture_HandGoal(unsigned int goal)
{
    return (int)(goal != 0);
}

WeaponDef* __stdcall Native_fixture_Pick()
{
    return &g_game->weapons[7];
}

int __stdcall Native_fixture_Strike(WeaponDef* weapon)
{
    if (weapon == nullptr)
        return -1;
    weapon->speed += 1;
    return weapon->speed;
}

void* g_attachedScript;
void* g_attachedShape;

int __stdcall Native_fixture_Attach(void* script, void* shape)
{
    g_attachedScript = script;
    g_attachedShape = shape;
    return script != nullptr ? (int)((unsigned char*)script)[0] : -1;
}

void* __stdcall Native_fixture_Lookup(int lane)
{
    FixtureLane* lanes = (FixtureLane*)g_game->p1cd5.get();
    return lanes == nullptr ? nullptr : lanes[lane].script;
}

int __stdcall Native_fixture_Notify(FixtureNotify* holder, Callback* done, int amount)
{
    if (holder != nullptr && holder->hook != nullptr)
        holder->hook->Complete(amount);
    if (done != nullptr)
        done->Complete(amount * 2);
    if (holder != nullptr && holder->link.get() != nullptr)
        holder->link->Complete(amount * 3);
    return amount;
}

int __stdcall Native_fixture_Measure(int* p2, short* p3, unsigned char* p4)
{
    if (p2 != nullptr)
        *p2 *= 2;
    if (p3 != nullptr)
        *p3 = (short)(*p3 - 1);
    if (p4 != nullptr)
        *p4 = (unsigned char)(*p4 + 1);
    return 1;
}

int __stdcall Native_fixture_Drain(void* out, unsigned int size)
{
    unsigned char* bytes = (unsigned char*)out;
    unsigned int n = size < 3 ? size : 3;
    for (unsigned int k = 0; k < n; k++)
        bytes[k] = (unsigned char)(0x10 + k);
    return 3;
}

int __stdcall Native_fixture_Gather(int* points, int capacity)
{
    for (int k = 0; k < capacity; k++)
        points[k] = 100 + k;
    return capacity;
}

int __stdcall GetWeaponRangeSlot(Unit* unit, unsigned int weapon)
{
    return GetWeaponRange((int)unit, weapon);
}

Projectile* __stdcall FindTargetableProjectileSlot(Unit* unit, unsigned int weapon)
{
    return FindTargetableProjectile((InterceptorUnitView*)unit, (int)weapon);
}

int __stdcall CanBuildAtSlot(UnitDef* unit, const Point16* cell, short type, Player* player)
{
    return CanBuildAt(unit, *cell, type, player);
}

int __stdcall CanPlaceUnitFootprintSlot(UnitDef* unit, unsigned short otherTypeId, const Point16* cell, int flags)
{
    return CanPlaceUnitFootprint(unit, otherTypeId, *cell, flags);
}

int __stdcall WeaponCanReachPosSlot(Unit* unit, Vec3* from, Vec3* to, int flags)
{
    return WeaponCanReachPos(unit, (ReachPosVec3*)from, (ReachPosVec3*)to, flags);
}

void __stdcall GetFootprintCentreSlot(const Point16* cell, Feature* def, Vec3* out)
{
    Point16 at = *cell;
    *out = GetFootprintCentre(&at, def);
}

Mission* TheMission()
{
    return g_game->net.get();
}

int __stdcall GetGameTypeSlot()
{
    return TheMission()->GetGameType();
}

const char* __stdcall GetNameSlotSlot(int index)
{
    return TheMission()->GetNameSlot(index);
}

int __stdcall AdvanceMissionSlot()
{
    return TheMission()->AdvanceMission();
}

const char* __stdcall GetCampaignNameSlot()
{
    return TheMission()->GetCampaignName();
}

int __stdcall GetMissionIndexSlot()
{
    return TheMission()->GetMissionIndex();
}

const char* __stdcall GetMissionNameSlot()
{
    return TheMission()->GetMissionName();
}

int __stdcall GetStartPositionSlot(Vec3* out, int id)
{
    return TheMission()->GetStartPosition(out, id);
}

void __stdcall LoadCampaignSlot(const char* file)
{
    TheMission()->LoadCampaign(file);
}

int __stdcall LoadMissionByNameSlot(const char* name)
{
    return TheMission()->LoadMissionByName(name);
}

int __stdcall MissionExistsSlot(int index)
{
    return TheMission()->MissionExists(index);
}

void __stdcall RefreshMapListSlot(int multiplayer)
{
    TheMission()->RefreshMapList(multiplayer);
}

int __stdcall SelectMissionSlot(int index)
{
    return TheMission()->SelectMission(index);
}

int __stdcall ComputeMapChecksumSlot()
{
    return TheMission()->ComputeMapChecksum();
}

int __stdcall GetTerrainLengthSlot()
{
    return TheMission()->GetTerrainLength();
}

int __stdcall CountMissionsSlot()
{
    return TheMission()->CountMissions();
}

int __stdcall GetCampaignMissionNameSlot(int index, char* out, int size)
{
    return TheMission()->GetCampaignMissionName(index, out, size);
}

int __stdcall CountStartPositionsSlot()
{
    return TheMission()->CountStartPositions();
}

const char* __stdcall GetBriefingSlot()
{
    return TheMission()->GetBriefing();
}

const char* __stdcall GetDescriptionSlot()
{
    return TheMission()->GetDescription();
}

char* __stdcall GetPlanetSlot()
{
    return TheMission()->GetPlanet();
}

int __stdcall GetTerrainSizeTierSlot()
{
    return TheMission()->GetTerrainSizeTier();
}

const char* __stdcall GetTranslatedNameSlot()
{
    return TheMission()->GetTranslatedName();
}

int __stdcall HasMissionNameSlot()
{
    return TheMission()->HasMissionName() ? 1 : 0;
}

int __stdcall SelectSchemaSlot(int type, TdfFile* parser, char* schema)
{
    return TheMission()->SelectSchema(type, parser, schema);
}

void __stdcall GetMissionParamsSlot(MissionParams* out)
{
    TheMission()->GetParams(out);
}

int __stdcall CountMissionUnitsSlot()
{
    return TheMission()->CountUnits();
}

int __stdcall GetMissionUnitSlot(int index, MissionUnitInfo* out)
{
    return TheMission()->GetUnit(index, out);
}

void __stdcall SetMissionUnitPositionSlot(int index, Vec3* pos)
{
    TheMission()->SetUnitPosition(index, pos);
}

int __stdcall CountMissionFeaturesSlot()
{
    return TheMission()->CountFeatures();
}

int __stdcall GetMissionFeatureSlot(int index, MissionFeatureInfo* out)
{
    return TheMission()->GetFeature(index, out);
}

const char* __stdcall GetMemorySlot()
{
    return TheMission()->GetMemory();
}

const char* __stdcall GetNumPlayersSlot()
{
    return TheMission()->GetNumPlayers();
}

void __stdcall UpdateFeaturesSlot(void)
{
    abort();
}

void __cdecl UpdateWindSlot(RandStream stream)
{
    UpdateWindOnStream(stream);
}

void __stdcall ApplySlot(const WindMeteorsEventHeader* event, unsigned int size)
{
    WindMeteorsApply(event, size);
}

int __stdcall ApplyTargetingSlot(const TargetingEventHeader* event, unsigned int size)
{
    return ApplyTargetingEvent(event, size);
}

int __stdcall ApplyMapEventSlot(const MapEventHeader* event, unsigned int size)
{
    return ApplyMapEvent(event, size);
}

void __stdcall AllocateBucketsSlot()
{
    MapAllocateBuckets();
}

void __stdcall FreeBucketsSlot()
{
    MapFreeBuckets();
}

void __stdcall WriteStateSlot(const StateWriter* writer)
{
    MapWriteState(writer);
}

void __stdcall UpdateSquadsSlot(int player)
{
    SquadManager* manager = g_game->players[player].ai.get();
    if (manager == nullptr)
        return;
    manager->TickIfActive();
}

void __stdcall CreateSquadManagerSlot(Player* player)
{
    player->ai = new (SimAlloc(64)) SquadManager(player);
}

void __stdcall FreeSquadManagerSlot(Player* player)
{
    SquadManager* manager = player->ai.get();
    if (manager == nullptr)
        return;
    manager->DeleteTimers();
    delete manager;
    player->ai = nullptr;
}

void __stdcall WriteAiStateSlot(const StateWriter* writer)
{
    AiWriteState(writer);
}

WeaponDef* __stdcall FindWeaponByNameSlot(const char* name)
{
    return (WeaponDef*)FindWeaponByName(name);
}

int __stdcall ApplyWeaponsSlot(const WeaponsEventHeader* event, unsigned int size)
{
    return WeaponsApply(event, size);
}

void __stdcall WriteWeaponsStateSlot(const StateWriter* writer)
{
    WeaponsWriteState(writer);
}

void __stdcall LoadWeaponsStateSlot(Unit* unit, int slot, const WeaponsSlotState* state)
{
    WeaponsLoadState(unit, slot, state);
}

void __stdcall SaveWeaponsStateSlot(Unit* unit, int slot, WeaponsSlotState* state)
{
    WeaponsSaveState(unit, slot, state);
}

void __fastcall SpatialBucketPrependUnitSlot(SpatialBucket* self, int unusedEdx, Unit* unit)
{
    self->PrependUnit(unit);
}

void __fastcall SpatialBucketUnlinkUnitSlot(SpatialBucket* self, int unusedEdx, Unit* unit)
{
    self->UnlinkUnit(unit);
}

int __stdcall PassMapGetSlot(int moveClass, int x, int y)
{
    return PassMapEntries()[moveClass].Get(x, y);
}

int __stdcall PassMapInBoundsSlot(int moveClass, unsigned int x, unsigned int y)
{
    return PassMapEntries()[moveClass].InBounds(x, y);
}

void __stdcall RefreshMovedUnitsSlot(int moveClass, Unit* unit)
{
    PassMapEntries()[moveClass].RefreshMovedUnits((UnitPassMapView*)unit);
}

void __stdcall RefreshUnitIfStaleSlot(int moveClass, Unit* unit)
{
    PassMapEntries()[moveClass].RefreshUnitIfStale((UnitPassMapView*)unit);
}

void __stdcall UpdatePlayersSlot()
{
}

void __stdcall UpdatePlayersOnStreamSlot(RandStream)
{
}


int __stdcall ScriptsNative_Apply(const ScriptsEventHeader* event, unsigned int size)
{
    return ScriptsApply(event, size);
}

int __stdcall ScriptsNative_StartScriptWithArgsByIndex(CobScript* script, int index, Callback* callback, int update, int count, int a, int b, int c, int d)
{
    return 0;
}

int __stdcall ScriptsNative_QueryScript(CobScript* script, const char* name, int* arg0, int* arg1, int* arg2, int* arg3)
{
    return script->QueryScript(name, arg0, arg1, arg2, arg3);
}

int __stdcall ScriptsNative_FindScript(CobScript* script, const char* name)
{
    return script->FindScript(name);
}

void __stdcall ScriptsNative_GetPieceOffset(Unit* unit, int piece, Vec3* out)
{
    *out = GetPieceOffset((PieceOffsetObjectView*)unit, piece);
}

void __stdcall ScriptsNative_GetPiecePosition(Unit* unit, int piece, Vec3* out)
{
    *out = GetPiecePosition((Object*)unit, piece);
}

void __stdcall ScriptsNative_GetPieceAngles(Unit* unit, int piece, Angles16* out)
{
    *out = GetPieceAngles((PieceAnglesObjectView*)unit, piece);
}

void __stdcall ScriptsNative_GetNanoPiecePosition(Unit* unit, Vec3* out)
{
    GetNanoPiecePosition((Object*)unit, out);
}

void __stdcall ScriptsNative_GetAimFromPosition(Unit* unit, Vec3* out, unsigned char weapon)
{
    GetAimFromPosition((Object*)unit, out, weapon);
}

void __stdcall ScriptsNative_GetSweetSpot(Unit* unit, Vec3* out)
{
    GetSweetSpot((SweetSpotObjectView*)unit, (int)out);
}

void __stdcall ScriptsNative_PoseUnitModel(Unit* unit)
{
}

ModelState* __stdcall ScriptsNative_CreateObjectState(Object3do* model)
{
    return CreateObjectState(model);
}

void __stdcall ScriptsNative_FreeObjectState(ModelState* state)
{
    FreeObjectState(state);
}

void __stdcall ScriptsNative_UpdateObjectState(UnitDrawView* unit)
{
}

unsigned int __stdcall ScriptsNative_ObjectStateSize(Object3do* model)
{
    return 0;
}

void __stdcall ScriptsNative_NewUnitScript(Unit* unit)
{
    NewUnitScript(unit);
}

void __stdcall ScriptsNative_SetCob(CobScript* script, ScriptTable* data)
{
    script->SetCob(data);
}

ModelState* __stdcall ScriptsNative_CreatePlayerObjectState(Object3do* model, ScriptTable* data, Unit* owner)
{
    return CreatePlayerObjectState(model, (ModelBuildList*)data, owner);
}

void __stdcall ScriptsNative_SetObjectState(UnitScript* script, ModelState* state)
{
    script->SetObjectState((ObjectState*)state);
}

void __stdcall ScriptsNative_RunScripts(CobScript* script, int elapsed)
{
    script->RunScripts(elapsed);
}

void __stdcall ScriptsNative_SaveScriptState(CobScript* script, HapiBank* file)
{
}

int __stdcall ScriptsNative_LoadScriptState(CobScript* script, HapiBank* file)
{
    return 0;
}

void __stdcall ScriptsNative_DeleteScript(CobScript* script)
{
    delete script;
}


int __stdcall UnitsNative_Apply(const UnitsEventHeader* event, unsigned int size)
{
    return UnitsApply(event, size);
}

void __stdcall UnitsNative_UpdateAllUnits()
{
}

void __stdcall UnitsNative_UnitTypeSetAddTypeOrCategory(UnitTypeSet* self, const char* text, int* out)
{
}

void __stdcall UnitsNative_AllocateUnitMemory()
{
}

void __stdcall UnitsNative_FreeUnitMemory()
{
}

void __stdcall UnitsNative_FreeUnitInfo()
{
}

void __stdcall UnitsNative_RefreshUnitInfo()
{
}

void __stdcall UnitsNative_LoadUnitTypes()
{
}

void __stdcall UnitsNative_FreeUnitTypes()
{
}

void __stdcall UnitsNative_LoadDownloadMenus()
{
}

void __stdcall UnitsNative_FreeDownloadMenus()
{
}

void __stdcall UnitsNative_FreeMovementClasses()
{
}

void __stdcall UnitsNative_InitUnitCategories()
{
}

void __stdcall UnitsNative_FreeUnitCategories()
{
}

void __stdcall UnitsNative_KillAllUnits()
{
}

void __stdcall UnitsNative_KillUnitsOfType(short id)
{
}

void __stdcall UnitsNative_ReloadUnitType(unsigned short id)
{
}

void __stdcall UnitsNative_KillUnit(Unit* unit, int cause)
{
    KillUnit(unit, cause);
}

void __stdcall UnitsNative_LoadUnits(HapiBank* file)
{
}

void __stdcall UnitsNative_SaveUnits(HapiBank* file)
{
}

Unit* __stdcall UnitsNative_LoadUnit(unsigned short id, HapiBank* file)
{
    return nullptr;
}

Unit* __stdcall UnitsNative_CreateUnit(unsigned char player, unsigned short typeId, const Vec3* pos, int finished, int mode, int id)
{
    return CreateUnit(player, typeId, *pos, finished, mode, id);
}

UnitDef* __stdcall UnitsNative_FindUnitType(const char* name)
{
    return FindUnitType(name);
}

unsigned short __stdcall UnitsNative_FindUnitTypeId(const char* name)
{
    return FindUnitTypeId(name);
}

Unit* __stdcall UnitsNative_FindMissionUnit(const char* name, const Ref<Unit>* units, int count, Unit* after)
{
    return nullptr;
}

int __stdcall UnitsNative_AddBuildProgress(Unit* builder, Unit* unit, float amount)
{
    return AddBuildProgress(builder, unit, amount);
}

int __stdcall UnitsNative_AddRepairProgress(Unit* builder, Unit* unit, float amount)
{
    return AddRepairProgress(builder, unit, amount);
}

int __stdcall UnitsNative_CanLoad(Unit* unit, Unit* target)
{
    return unit->CanLoad(target);
}

int __stdcall UnitsNative_CanReclaim(Unit* unit, Unit* target)
{
    return unit->CanReclaim(target);
}

int __stdcall UnitsNative_CanRepair(Unit* unit, Unit* target)
{
    return unit->CanRepair(target);
}

void __stdcall UnitsNative_UnitRefUnlink(UnitRef* ref)
{
    ref->Unlink();
}

void __stdcall UnitsNative_UpdateUnitHead(Unit* unit)
{
    UpdateUnitHead(unit);
}

int __stdcall UnitsNative_UpdateUnitBody(Unit* unit)
{
    return UpdateUnitBody(unit);
}

void __stdcall UnitsNative_UpdateUnitTail(Unit* unit, int moved)
{
    UpdateUnitTail(unit, moved);
}

int __stdcall UnitsNative_GetFactoriesInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
{
    return GetFactoriesInRadius(player, pos, radius, out, capacity);
}

void __stdcall UnitsNative_CreateFactoryIndex(int player)
{
    CreateFactoryIndex(player);
}

void __stdcall UnitsNative_FreeFactoryIndex(int player)
{
    FreeFactoryIndex(player);
}

void __stdcall UnitsNative_RefreshFactoryIndex(int player)
{
    RefreshFactoryIndex(player);
}

void __stdcall UnitsNative_UnitRefInit(UnitRef* ref, Unit* unit, OrderBase* value)
{
    ((PathOrderAttach*)ref)->PathOrderAttach::PathOrderAttach(unit, value);
}

void __stdcall UnitsNative_UnitRefSetUnit(UnitRef* ref, Unit* unit)
{
    ((PathOrderAttach*)ref)->SetUnit(unit);
}

Unit* __stdcall UnitsNative_UnitRefGet(UnitRef* ref)
{
    return UnitRefGet(ref);
}

void __stdcall UnitsNative_UnitRefSetValue(UnitRef* ref, OrderBase* value)
{
    UnitRefSetValue(ref, value);
}

void __stdcall UnitsNative_WriteState(const StateWriter* writer)
{
}

int __stdcall UnitsNative_GetMovementLimits(MapMoveLimits* out, int capacity)
{
    return 0;
}

#include "replay_slots.h"
#include "replay_shapes.h"

struct LoadFileBytes {
    char name[260];
    std::vector<unsigned char> bytes;
};

std::vector<LoadFileBytes*> g_loadFiles;

void ForgetLoads()
{
    for (size_t i = 0; i < g_loadFiles.size(); i++)
        delete g_loadFiles[i];
    g_loadFiles.clear();
    ReplayLoadsReset();
}

const LoadFileBytes* FindLoadFile(const char* name)
{
    for (size_t i = 0; i < g_loadFiles.size(); i++) {
        if (strcmp(g_loadFiles[i]->name, name) == 0)
            return g_loadFiles[i];
    }
    return nullptr;
}

int __stdcall LoadFileLength(const char* name)
{
    const LoadFileBytes* f = FindLoadFile(name);
    return f == nullptr ? 0 : (int)f->bytes.size();
}

void* __stdcall LoadFileBuffer(const char* name, int* size)
{
    const LoadFileBytes* f = FindLoadFile(name);
    if (size != nullptr)
        *size = f == nullptr ? 0 : (int)f->bytes.size();
    if (f == nullptr || f->bytes.empty())
        return nullptr;
    void* copy = malloc(f->bytes.size());
    memcpy(copy, &f->bytes[0], f->bytes.size());
    return copy;
}

void* LoadByKind(unsigned load, char* name)
{
    if (load == RL_COB)
        return LoadCobScript(name);
    if (load == RL_MODEL) {
        void* model = Load3do(name);
        if (model != nullptr)
            MirrorObject((Object3do*)model);
        return model;
    }
    return g_seams.files->Load(name, nullptr);
}

unsigned ParseHexByte(const char* p)
{
    return ParseHex(p, 2);
}

void BindLoadFiles()
{
    g_import_files.Load = LoadFileBuffer;
    g_import_files.LengthByName = LoadFileLength;
}

void ClearFaceBits(PieceFrame* face, unsigned, void*)
{
    *(unsigned*)&face->flags &= ~7u;
}

bool DecimalWord(const char* text, unsigned* value)
{
    if (text[0] == '\0' || strlen(text) > 10)
        return false;
    for (const char* p = text; *p != '\0'; p++) {
        if (*p < '0' || *p > '9')
            return false;
    }
    *value = (unsigned)strtoul(text, nullptr, 10);
    return true;
}

void BindModelFaces(const char* file, void* base, unsigned size, const std::vector<ReplayLoadBind>& binds)
{
    ReplayEachFace(base, size, ClearFaceBits, nullptr);
    for (size_t i = 0; i < binds.size(); i++) {
        PieceFrame* face = ReplayFaceAt(base, size, binds[i].offset);
        if (face == nullptr)
            Fatal(file, 0, "a bind line names no face of its model", "bind");
        *(unsigned*)&face->flags = binds[i].flags;
        face->fillColor = (int)binds[i].fillColor;
        face->texKind = (int)binds[i].texKind;
        face->ptr_10 = 0;
        if (strcmp(binds[i].src, "null") != 0) {
            unsigned kind = 0;
            void* target = ReplayResolveObject(binds[i].src, &kind, g_objects, AllocObject);
            if (target == nullptr)
                Fatal(file, 0, "a bind line names an object it cannot make", binds[i].src);
            face->color = (unsigned short*)target;
        }
        if (binds[i].tex[0] == '=') {
            unsigned word = 0;
            if (!DecimalWord(binds[i].tex + 1, &word))
                Fatal(file, 0, "a bind line's word is decimal", binds[i].tex);
            face->ptr_10 = (int)word;
        } else if (binds[i].tex[0] == '+') {
            unsigned at = 0;
            if (!DecimalWord(binds[i].tex + 1, &at) || at >= size)
                Fatal(file, 0, "a bind line names a place outside its model", binds[i].tex);
            face->ptr_10 = (int)(size_t)((char*)base + at);
        } else if (strcmp(binds[i].tex, "null") != 0) {
            unsigned kind = 0;
            void* target = ReplayResolveObject(binds[i].tex, &kind, g_objects, AllocObject);
            if (target == nullptr || kind != ROK_GAFFRAME)
                Fatal(file, 0, "a bind line names a picture it cannot make", binds[i].tex);
            face->ptr_10 = (int)(size_t)target;
        }
    }
}

bool HexBytes(const char* text, unsigned char* out, unsigned long* count)
{
    unsigned long length = (unsigned long)strlen(text);
    if (length % 2 != 0)
        return false;
    for (unsigned long i = 0; i < length; i++) {
        char c = text[i];
        if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f'))
            return false;
        if (out != nullptr && i % 2 == 0)
            out[i / 2] = (unsigned char)ParseHexByte(text + i);
    }
    *count = length / 2;
    return true;
}

ReplayCacheStart g_cacheStart;
bool g_cacheNamed;

int ReadCacheLines(const char* file, int at, int end)
{
    const char* line = g_lines[at];
    char name[24];
    unsigned cap = 0;
    unsigned cur = 0;
    char extra;
    if (g_cacheNamed || sscanf(line, "cache %23s %u %u%c", name, &cap, &cur, &extra) != 3 || strcmp(name, "debris") != 0)
        Fatal(file, 0, "a cache line is `cache debris <size> <cursor>`, once", line);
    g_cacheNamed = true;
    g_cacheStart.taken = true;
    g_cacheStart.cap = cap;
    g_cacheStart.cur = cur;
    unsigned next = 0;
    bool curAtBlock = false;
    for (at++; at < end && strncmp(g_lines[at], "  block ", 8) == 0; at++) {
        ReplayCacheBlock b;
        unsigned offset;
        unsigned size;
        char owner[40] = "";
        char tail[16] = "";
        int fields = sscanf(g_lines[at], "  block %u %u %39s %15s%c", &offset, &size, owner, tail, &extra);
        bool slot = strncmp(owner, "ExplodedSlot#", 13) == 0;
        bool live = strcmp(tail, "live") == 0;
        if (fields < 3 || fields > 4 || offset != next || size < 8 || size > cap - offset || (!slot && strcmp(owner, "null") != 0) || (fields == 4 && !(live && slot)))
            Fatal(file, 0, "a block line is `  block <offset> <size> <ExplodedSlot#N|null>[ live]`, tiling the cache", g_lines[at]);
        b.offset = offset;
        b.size = size;
        b.owner = -1;
        b.live = live;
        if (slot) {
            char* digits;
            unsigned long index = strtoul(owner + 13, &digits, 10);
            if (*digits != '\0' || digits == owner + 13 || index >= 100)
                Fatal(file, 0, "a block owned by a slot that g_explodedPieces does not have", g_lines[at]);
            b.owner = (int)index;
        }
        curAtBlock = curAtBlock || offset == cur;
        g_cacheStart.blocks.push_back(b);
        next = offset + size;
    }
    if (next != cap || !curAtBlock)
        Fatal(file, 0, "the blocks of a cache line do not tile its size, or its cursor is in none", line);
    return at;
}

void StoreGameHolder(const char* file, const char* line, unsigned load, unsigned kind, unsigned index, void* base)
{
    const ReplayLoadKind& l = kReplayLoads[load];
    for (unsigned n = 0; n < l.holderCount; n++) {
        const ReplayLoadHolder& h = kReplayLoadHolders[l.firstHolder + n];
        if (h.how != RLH_GAME || h.kind != kind)
            continue;
        unsigned room = kReplayTables[kReplayObjectKinds[kind].table - 1].alloc;
        if (index >= room)
            Fatal(file, 0, "a load line whose holder is past the room of its table", line);
        unsigned* ref = (unsigned*)((unsigned char*)g_game + h.offset);
        if (*ref == 0) {
            void* array = AllocObject(room * sizeof(void*));
            memset(array, 0, room * sizeof(void*));
            *ref = (unsigned)((unsigned char*)array - (unsigned char*)SimBlockBase());
        }
        ((void**)((unsigned char*)SimBlockBase() + *ref))[index] = base;
    }
}

void LoadInputs(const char* file, int end)
{
    BindLoadFiles();
    g_cacheStart = ReplayCacheStart();
    g_cacheNamed = false;
    std::vector<unsigned> held;
    int at = 2;
    while (at < end) {
        const char* line = g_lines[at];
        if (line[0] == '\0') {
            at++;
            continue;
        }
        if (strncmp(line, "cache ", 6) == 0) {
            at = ReadCacheLines(file, at, end);
            continue;
        }
        char loadName[48];
        char kindName[48];
        unsigned index = 0;
        const char* quote = strchr(line, '"');
        const char* open = strrchr(line, '[');
        if (strncmp(line, "load ", 5) != 0 || sscanf(line, "load %47s %47[^#]#%u", loadName, kindName, &index) != 3 || quote == nullptr || open == nullptr)
            Fatal(file, 0, "expected a load line: load <load> <Kind>#<N> \"<file>\" [<size>]", line);
        unsigned load = 0;
        while (load < kReplayLoadCount && strcmp(kReplayLoads[load].name, loadName) != 0)
            load++;
        int kind = ReplayFindObjectKind(kindName, (unsigned)strlen(kindName));
        if (load == kReplayLoadCount || kind < 0)
            Fatal(file, 0, "a load line of an unknown load kind or holder kind", line);
        unsigned key = (load << 24) ^ ((unsigned)kind << 16) ^ index;
        for (size_t i = 0; i < held.size(); i++) {
            if (held[i] == key)
                Fatal(file, 0, "a second load line for one holder", line);
        }
        held.push_back(key);
        LoadFileBytes* f = new LoadFileBytes;
        ParseQuoted(quote, f->name, sizeof f->name);
        unsigned size = (unsigned)strtoul(open + 1, nullptr, 10);
        f->bytes.assign(size, 0);
        for (at++; at < end && strncmp(g_lines[at], "  x ", 4) == 0; at++) {
            char* colon;
            unsigned long first = strtoul(g_lines[at] + 4, &colon, 10);
            if (*colon != ':')
                Fatal(file, 0, "a load byte line is `  x <offset>:<hex>`", g_lines[at]);
            unsigned long n = 0;
            if (!HexBytes(colon + 1, nullptr, &n))
                Fatal(file, 0, "a load byte line holds bytes as an even number of lower case hex digits", g_lines[at]);
            if (first + n > size)
                Fatal(file, 0, "a load byte line past the size of its file", g_lines[at]);
            HexBytes(colon + 1, n == 0 ? nullptr : &f->bytes[first], &n);
        }
        std::vector<ReplayLoadBind> binds;
        for (; at < end && strncmp(g_lines[at], "  bind ", 7) == 0; at++) {
            if (load != RL_MODEL)
                Fatal(file, 0, "a bind line on a load that is not a model", g_lines[at]);
            ReplayLoadBind b;
            memset(&b, 0, sizeof b);
            char flagsText[24];
            char fillColorText[24];
            char texKindText[24];
            char src[40];
            char tex[40];
            char extra;
            if (sscanf(g_lines[at], "  bind %u %23s %39s %23s %39s %23s%c", &b.offset, flagsText, src, fillColorText, tex, texKindText, &extra) != 6
                || !DecimalWord(flagsText, &b.flags) || !DecimalWord(fillColorText, &b.fillColor) || !DecimalWord(texKindText, &b.texKind))
                Fatal(file, 0, "a bind line is `  bind <offset> <flags> <FrameTable#N|null> <fillColor> <GafFrame#N|null|=index> <texKind>`", g_lines[at]);
            strcpy(b.src, src);
            strcpy(b.tex, tex);
            binds.push_back(b);
        }
        g_loadFiles.push_back(f);
        g_loading = true;
        void* base = LoadByKind(load, f->name);
        g_loading = false;
        if (base == nullptr)
            Fatal(file, 0, "the loader refused the file of a load line", line);
        if (load == RL_MODEL)
            BindModelFaces(file, base, size, binds);
        ReplayLoadAdopt(load, (unsigned)kind, index, base, size);
        StoreGameHolder(file, line, load, (unsigned)kind, index, base);
    }
}

void ResetBlock()
{
    ForgetLoadObjects();
    if (ReplayWasmActive())
        ReplayWasmReload();
    else
        SimBlockReset();
    g_simGlobals = (SimGlobals*)SimAlloc(sizeof(SimGlobals));
    g_game = (Game*)SimAlloc(sizeof(Game));
    BindSeams();
    if (ReplayWasmActive())
        ReplayWasmUseSeam();
    strcpy(g_extTdf, "tdf");
    g_known.Reset();
    g_objects.Reset();
    g_captureUsed = 0;
    g_region.ptr = nullptr;
    g_region.size = 0;
    ForgetLoads();
}

const ReplaySlotSets* FindSlot(const char* subsystem, const char* name)
{
    for (unsigned s = 0; s < kReplaySubsystemCount; s++) {
        if (strcmp(kReplaySubsystems[s].name, subsystem) != 0)
            continue;
        for (unsigned i = 0; i < kReplaySubsystems[s].slotCount; i++) {
            const ReplaySlotSets& slot = kReplaySlots[kReplaySubsystems[s].firstSlot + i];
            if (strcmp(slot.name, name) == 0)
                return &slot;
        }
    }
    return nullptr;
}

const ReplaySlotSets* SetsOfKind(const ReplaySlotSets* sets)
{
    unsigned slot = (unsigned)(sets - kReplaySlots);
    for (unsigned i = 0; i < kReplayKindSetCount; i++) {
        if (kReplayKindSets[i].slot != slot)
            continue;
        for (int k = g_callFirst + 1; k < g_callEnd; k++) {
            int kind;
            if (sscanf(g_lines[k], "  a event.kind %d", &kind) == 1 && kind == kReplayKindSets[i].kind)
                return kReplayKindSets[i].sets;
        }
    }
    return sets;
}

void SplitLines()
{
    g_lineCount = 0;
    for (char* p = g_text; *p != '\0';) {
        if (g_lineCount == MAX_LINES)
            abort();
        g_lines[g_lineCount++] = p;
        char* end = strchr(p, '\n');
        if (end == nullptr)
            break;
        *end = '\0';
        p = end + 1;
    }
}

bool LoadFile(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (f == nullptr)
        return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    free(g_text);
    g_text = (char*)malloc((unsigned)size + 1);
    unsigned got = (unsigned)fread(g_text, 1, (unsigned)size, f);
    fclose(f);
    g_text[got] = '\0';
    SplitLines();
    return true;
}

struct Outcome {
    int calls;
    int imports;
    bool diverged;
    int call;
    char key[96];
    ReplayText log;
    ReplayText core;
};

struct LineKey {
    unsigned kind;
    unsigned ordinal;
    const char* fieldName;
    const ReplayObjectField* field;
    bool vectorElement;
    bool refsElement;
    unsigned element;
    const char* sub;
    char prefix[96];
    char object[96];
};

bool ParseLineKey(const char* name, LineKey& out)
{
    out.field = nullptr;
    out.vectorElement = false;
    out.refsElement = false;
    out.element = 0;
    out.sub = nullptr;
    const char* base = name;
    if (ReplayParseKey(name, &out.kind, &out.ordinal, &out.fieldName))
        out.field = ReplayFindObjectField(out.kind, out.fieldName);
    if (out.field == nullptr) {
        if (ReplayParseElementKey(name, out.prefix, sizeof out.prefix, &out.element, &out.sub))
            out.vectorElement = true;
        else if (ReplayParseRefsKey(name, out.prefix, sizeof out.prefix, &out.element))
            out.refsElement = true;
        else
            return false;
        base = out.prefix;
        if (!ReplayParseKey(base, &out.kind, &out.ordinal, &out.fieldName))
            return false;
        out.field = ReplayFindObjectField(out.kind, out.fieldName);
    }
    const char* dot = strrchr(base, '.');
    memcpy(out.object, base, (unsigned)(dot - base));
    out.object[dot - base] = '\0';
    return true;
}

void AdoptCachedPiece(const ReplayObjectField& f, void* slot, const char* value)
{
    unsigned held = *(unsigned*)((unsigned char*)slot + f.offset);
    if (held == 0 || strncmp(value, "ExplodedPiece#", 14) != 0)
        return;
    char* end;
    unsigned long ordinal = strtoul(value + 14, &end, 10);
    void* piece = ReplayRefPtr(held);
    if (*end == '\0' && end != value + 14 && ordinal != 0 && g_objects.Find(ROK_EXPLODEDPIECE, piece) == 0 && g_objects.Pointer(ROK_EXPLODEDPIECE, (unsigned)ordinal) == nullptr)
        g_objects.Bind(ROK_EXPLODEDPIECE, (unsigned)ordinal, piece);
}

bool RestoreObjectLine(const char* name, const char* value, const char* file, const ReplaySlotSets* sets, const char* line)
{
    LineKey k;
    if (!ParseLineKey(name, k))
        return false;
    void* p = ReplayResolveObject(k.object, nullptr, g_objects, AllocObject);
    const ReplayObjectField* f = k.field;
    if (p == nullptr || f == nullptr)
        Fatal(file, g_callIndex, "an object line that names no object or field", line);
    if (sets != nullptr && !InObjectSet(sets->objReads, sets->objReadCount, f) && !InObjectSet(sets->objWrites, sets->objWriteCount, f))
        Fatal(file, g_callIndex, "field is not in the object sets of the slot", name);
    if (!ReplayFieldLive(*f, p))
        return true;
    if (f->kind == RK_ARRAY) {
        if (!ReplayApplyArray(*f, p, name, value, g_known, true, AllocObject))
            Fatal(file, g_callIndex, "an array line that cannot be applied", value);
        return true;
    }
    if (f->kind == RK_BLOB) {
        if (!ReplayApplyBlob(*f, p, name, value, g_known, true, g_objects, AllocObject))
            Fatal(file, g_callIndex, "a blob line that cannot be applied", value);
        return true;
    }
    if (f->kind == RK_VECTOR) {
        bool applied = k.vectorElement ? ReplayApplyVectorElement(*f, p, k.element, k.sub, name, value, g_known, true, g_objects, AllocObject)
                       : k.refsElement ? ReplayApplyVectorString(*f, p, k.element, name, value, g_known, true, AllocObject)
                                       : ReplayApplyVector(*f, p, name, value, g_known, true, g_objects, AllocObject);
        if (!applied)
            Fatal(file, g_callIndex, "a vector line that cannot be applied", value);
        return true;
    }
    if (f->kind == RK_CSTRINGS) {
        bool applied = k.refsElement ? ReplayApplyCStringsElement(*f, p, k.prefix, k.element, name, value, g_known, true, AllocObject)
                                     : ReplayApplyCStrings(*f, p, name, value, g_known, true, AllocObject);
        if (!applied)
            Fatal(file, g_callIndex, "a string array line that cannot be applied", value);
        return true;
    }
    if (f->kind == RK_REFS) {
        if (!k.refsElement || !ReplayRestoreRefsElement(*f, p, k.element, value, g_objects, AllocObject))
            Fatal(file, g_callIndex, "a ref line that cannot be applied", value);
        g_known.Set(name, value);
        return true;
    }
    if (f->object == ROK_EXPLODEDSLOT && f->kind == RK_REF)
        AdoptCachedPiece(*f, p, value);
    MapLoadTarget(*f, value);
    if (!ReplayRestoreObjectField(*f, p, value, g_objects, AllocObject))
        Fatal(file, g_callIndex, "a value that cannot be restored", value);
    g_known.Set(name, value);
    return true;
}

const char* ImportObjectProblem(const char* name, const ReplaySlotSets& sets)
{
    LineKey k;
    if (!ParseLineKey(name, k) || k.field == nullptr)
        return "an object line after an import that names no object field";
    if (!InObjectSet(sets.objReads, sets.objReadCount, k.field) && !InObjectSet(sets.objWrites, sets.objWriteCount, k.field))
        return "an object line after an import whose field the slot neither reads nor writes";
    return nullptr;
}

const char* BeforeValue(const char* name)
{
    for (int i = 0; i < g_beforeCount; i++) {
        if (strcmp(g_beforeNames[i], name) == 0)
            return g_beforeValues[i];
    }
    return g_known.Get(name);
}

void CompareBeforeImport(const char* name)
{
    LineKey k;
    if (!ParseLineKey(name, k) || k.vectorElement || k.refsElement || k.field == nullptr || !InObjectSet(g_sets->objWrites, g_sets->objWriteCount, k.field))
        return;
    const ReplayObjectField& f = *k.field;
    if (f.kind == RK_ARRAY || f.kind == RK_BLOB || f.kind == RK_VECTOR || f.kind == RK_REFS || f.kind == RK_CSTRINGS)
        return;
    const char* expected = BeforeValue(name);
    void* p = ReplayResolveObject(k.object, nullptr, g_objects, AllocObject);
    if (expected == nullptr || p == nullptr || !ReplayFieldLive(f, p))
        return;
    ReplayText value;
    ReplayFormatObjectField(f, p, g_objects, false, value);
    if (strcmp(expected, value.Data()) != 0)
        Diff(name, expected, value.Data());
    value.Free();
}

void RestoreImportObject(const char* name, const char* value)
{
    if (!g_importChanges || g_sets == nullptr || strchr(name, '#') == nullptr)
        return;
    const char* problem = ImportObjectProblem(name, *g_sets);
    if (problem != nullptr)
        Fatal(g_file, g_callIndex, problem, name);
    CompareBeforeImport(name);
    RestoreObjectLine(name, value, g_file, g_sets, name);
}

bool ObjectLineIsCondition(const char* name)
{
    LineKey k;
    if (!ParseLineKey(name, k) || k.vectorElement || k.refsElement)
        return false;
    return k.field != nullptr && ReplayFieldIsCondition(*k.field);
}

bool RestoreRunLine(const char* name, const char* values, const char* file, const ReplaySlotSets* sets, const char* line, int pass)
{
    const char* plus = strchr(name, '+');
    const char* hash = strchr(name, '#');
    if (plus == nullptr || hash == nullptr || hash > plus)
        return false;
    char* end;
    unsigned first = (unsigned)strtoul(hash + 1, &end, 10);
    if (end != plus)
        Fatal(file, g_callIndex, "a run line that names no first element", line);
    unsigned count = (unsigned)strtoul(plus + 1, &end, 10);
    if (*end != '.' || count < 2)
        Fatal(file, g_callIndex, "a run line that names no count of elements", line);
    const char* field = end;
    char key[96];
    unsigned prefix = (unsigned)(hash + 1 - name);
    if (prefix + 10 + strlen(field) >= sizeof key)
        Fatal(file, g_callIndex, "a run line whose name is too long", line);
    memcpy(key, name, prefix);
    sprintf(key + prefix, "%u%s", first, field);
    if (ObjectLineIsCondition(key) != (pass == 0))
        return true;
    unsigned done = 0;
    for (const char* p = values; *p != '\0';) {
        char token[48];
        unsigned n = 0;
        while (*p != '\0' && *p != ' ' && *p != '*' && n + 1 < sizeof token)
            token[n++] = *p++;
        token[n] = '\0';
        unsigned repeat = 1;
        if (*p == '*') {
            repeat = (unsigned)strtoul(p + 1, &end, 10);
            p = end;
        }
        if (*p != '\0' && *p != ' ')
            Fatal(file, g_callIndex, "a run line with a bad value", line);
        while (*p == ' ')
            p++;
        if (n == 0 || repeat == 0 || done + repeat > count)
            Fatal(file, g_callIndex, "a run line with more values than its count", line);
        for (unsigned r = 0; r < repeat; r++, done++) {
            sprintf(key + prefix, "%u%s", first + done, field);
            RestoreObjectLine(key, token, file, sets, line);
        }
    }
    if (done != count)
        Fatal(file, g_callIndex, "a run line with fewer values than its count", line);
    return true;
}

std::vector<void*> g_blobSeen;

void CompareObject(const ReplayReach* reach, unsigned at, unsigned kind, void* object, const unsigned short* set, unsigned count, ReplayText& key, ReplayText& value)
{
    for (unsigned i = 0; i < count; i++) {
        const ReplayObjectField& f = kReplayObjectFields[set[i]];
        if (f.object != kind || f.kind == RK_EMBED || f.kind == RK_EMBEDS || f.kind == RK_UNITID || f.kind == RK_SELF || (reach != nullptr && !ReplayInReach(*reach, at, set[i])))
            continue;
        if (!ReplayFieldLive(f, object))
            continue;
        ReplayObjectFieldKey(key, g_objects, false, kind, object, f);
        if (f.kind == RK_VECTOR) {
            ReplayCompareVector(f, object, key.Data(), g_known, g_objects, Diff, HashText);
            continue;
        }
        if (f.kind == RK_REFS) {
            ReplayCompareRefs(f, object, key.Data(), g_known, g_objects, Diff, HashText);
            continue;
        }
        if (f.kind == RK_CSTRINGS) {
            ReplayCompareCStrings(f, object, key.Data(), g_known, Diff, HashText);
            continue;
        }
        ReplayText bytes;
        const char* lookup = key.Data();
        if (f.kind == RK_BLOB && ReplayBlobKey(f, object, g_objects, bytes)) {
            void* target = *(void**)((unsigned char*)object + f.offset);
            if (std::find(g_blobSeen.begin(), g_blobSeen.end(), target) != g_blobSeen.end()) {
                bytes.Free();
                continue;
            }
            g_blobSeen.push_back(target);
            lookup = bytes.Data();
        }
        ReplayFormatObjectField(f, object, g_objects, false, value);
        HashText(value.Data());
        const char* known = g_known.Get(lookup);
        if (known == nullptr)
            Diff(key.Data(), "no value", value.Data());
        else if (strcmp(known, value.Data()) != 0)
            Diff(key.Data(), known, value.Data());
        bytes.Free();
    }
}

void CompareObjects(const ReplaySlotSets* sets)
{
    g_blobSeen.clear();
    ReplayWalk(g_reach, g_objects, true, *sets, g_rootKinds, g_rootObjects, g_rootCount);
    ReplayText key;
    ReplayText value;
    for (unsigned at = 0; at < g_reach.count; at++) {
        if (!ReplayInDeltaTable(g_reach, g_reach.kind[at], g_reach.object[at]))
            CompareObject(&g_reach, at, g_reach.kind[at], g_reach.object[at], sets->objWrites, sets->objWriteCount, key, value);
    }
    for (unsigned d = 0; d < ReplayDeltaCount(g_reach); d++) {
        unsigned kind;
        unsigned char* base;
        unsigned elements;
        unsigned stride;
        if (!ReplayDeltaTable(g_reach, d, &kind, &base, &elements, &stride))
            continue;
        for (unsigned e = 0; e < elements; e++)
            CompareObject(nullptr, 0, kind, base + e * stride, sets->objWrites, sets->objWriteCount, key, value);
    }
    key.Free();
    value.Free();
}

void CoreObjectWrites(const char* file, const ReplayReach* reach, unsigned at, unsigned kind, void* object, const ReplaySlotSets* sets,
                      ReplayText& key, ReplayText& value)
{
    for (unsigned i = 0; i < sets->objWriteCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[sets->objWrites[i]];
        if (f.object != kind || f.kind == RK_EMBED || f.kind == RK_EMBEDS || f.kind == RK_UNITID || f.kind == RK_SELF
            || (reach != nullptr && !ReplayInReach(*reach, at, sets->objWrites[i])))
            continue;
        if (!ReplayFieldLive(f, object))
            continue;
        ReplayObjectFieldKey(key, g_objects, false, kind, object, f);
        if (f.kind == RK_VECTOR || f.kind == RK_REFS || f.kind == RK_CSTRINGS || f.kind == RK_BLOB)
            Fatal(file, g_callIndex, "core mode cannot write vector fields", key.Data());
        ReplayFormatObjectField(f, object, g_objects, false, value);
        const char* known = g_known.Get(key.Data());
        if (known != nullptr && strcmp(known, value.Data()) == 0)
            continue;
        g_known.Set(key.Data(), value.Data());
        CoreJson(g_coreWrites, "w", key.Data(), value.Data());
    }
}

void CoreObjectsWrites(const char* file, const ReplaySlotSets* sets)
{
    if (sets->objWriteCount == 0)
        return;
    ReplayWalk(g_reach, g_objects, true, *sets, g_rootKinds, g_rootObjects, g_rootCount);
    ReplayText key;
    ReplayText value;
    for (unsigned at = 0; at < g_reach.count; at++) {
        if (!ReplayInDeltaTable(g_reach, g_reach.kind[at], g_reach.object[at]))
            CoreObjectWrites(file, &g_reach, at, g_reach.kind[at], g_reach.object[at], sets, key, value);
    }
    for (unsigned d = 0; d < ReplayDeltaCount(g_reach); d++) {
        unsigned kind;
        unsigned char* base;
        unsigned elements;
        unsigned stride;
        if (!ReplayDeltaTable(g_reach, d, &kind, &base, &elements, &stride))
            continue;
        for (unsigned e = 0; e < elements; e++)
            CoreObjectWrites(file, nullptr, 0, kind, base + e * stride, sets, key, value);
    }
    key.Free();
    value.Free();
}

void CoreWrites(const char* file, const ReplaySlotSets* sets)
{
    ReplayText value;
    for (unsigned i = 0; i < sets->writeCount; i++) {
        const ReplayField& f = kReplayFields[sets->writes[i]];
        if (f.kind == RK_VECTOR)
            Fatal(file, g_callIndex, "core mode cannot write vector fields", f.name);
        FormatField(f, value);
        const char* known = g_known.Get(f.name);
        if (known != nullptr && strcmp(known, value.Data()) == 0)
            continue;
        g_known.Set(f.name, value.Data());
        if (g_coreFault && g_callIndex >= 1 && !g_coreDropped) {
            g_coreDropped = true;
            continue;
        }
        CoreJson(g_coreWrites, "w", f.name, value.Data());
    }
    value.Free();
    CoreObjectsWrites(file, sets);
}

void CompareWrites(const char* file, const ReplaySlotSets* sets)
{
    if (g_core) {
        CoreWrites(file, sets);
        return;
    }
    ReplayText value;
    for (int k = g_callFirst + 1; k < g_callEnd; k++) {
        const char* line = g_lines[k];
        if (strncmp(line, "  w ", 4) != 0)
            continue;
        const char* space = strchr(line + 4, ' ');
        char name[96];
        if (space == nullptr || (unsigned)(space - (line + 4)) >= sizeof name)
            Fatal(file, g_callIndex, "bad write line", line);
        memcpy(name, line + 4, (unsigned)(space - (line + 4)));
        name[space - (line + 4)] = '\0';
        bool array = false;
        const ReplayObjectField* arrayField = nullptr;
        ReplayObjectField blockView = {};
        if (strchr(name, '#') != nullptr) {
            LineKey k;
            const ReplayObjectField* f = ParseLineKey(name, k) ? k.field : nullptr;
            if (f == nullptr || !InObjectSet(sets->objWrites, sets->objWriteCount, f))
                Fatal(file, g_callIndex, "field is not in the write set of the slot", line);
            if (f->kind == RK_VECTOR) {
                bool applied = k.vectorElement ? ReplayApplyVectorElement(*f, nullptr, k.element, k.sub, name, space + 1, g_known, false, g_objects, nullptr)
                               : k.refsElement ? ReplayApplyVectorString(*f, nullptr, k.element, name, space + 1, g_known, false, nullptr)
                                               : ReplayApplyVector(*f, nullptr, name, space + 1, g_known, false, g_objects, nullptr);
                if (!applied)
                    Fatal(file, g_callIndex, "a vector line that cannot be applied", line);
                continue;
            }
            if (f->kind == RK_REFS || f->kind == RK_CSTRINGS) {
                g_known.Set(name, space + 1);
                continue;
            }
            if (f->kind == RK_BLOB) {
                if (!ReplayApplyBlob(*f, nullptr, name, space + 1, g_known, false, g_objects, nullptr))
                    Fatal(file, g_callIndex, "a blob line that cannot be applied", line);
                continue;
            }
            array = f->kind == RK_ARRAY;
            arrayField = f;
        } else {
            const ReplayField* f = FindBlockField(name);
            if (f == nullptr || !InSet(sets->writes, sets->writeCount, f))
                Fatal(file, g_callIndex, "field is not in the write set of the slot", line);
            if (f->kind == RK_VECTOR) {
                if (!ApplyBlockVector(*f, name, space + 1, false))
                    Fatal(file, g_callIndex, "a vector line that cannot be applied", line);
                continue;
            }
            blockView = ReplayFieldView(*f);
            array = f->kind == RK_ARRAY;
            arrayField = &blockView;
        }
        if (array) {
            if (!ReplayApplyArray(*arrayField, nullptr, name, space + 1, g_known, false, nullptr))
                Fatal(file, g_callIndex, "an array line that cannot be applied", line);
        } else {
            g_known.Set(name, space + 1);
        }
    }
    for (unsigned i = 0; i < sets->writeCount; i++) {
        const ReplayField& f = kReplayFields[sets->writes[i]];
        if (f.kind == RK_VECTOR) {
            CompareBlockVector(f);
            continue;
        }
        FormatField(f, value, true);
        HashText(value.Data());
        const char* known = g_known.Get(f.name);
        if (known == nullptr)
            Diff(f.name, "no value", value.Data());
        else if (strcmp(known, value.Data()) != 0)
            Diff(f.name, known, value.Data());
    }
    value.Free();
    CompareObjects(sets);
}

void CompareSnapshots()
{
    ReplayText hex;
    for (unsigned n = 0; n < MAX_SNAPSHOTS; n++) {
        char prefix[32];
        sprintf(prefix, "  o snapshot#%u x:", n + 1);
        size_t length = strlen(prefix);
        bool recorded = false;
        hex.Clear();
        for (int k = g_callFirst + 1; k < g_callEnd; k++) {
            if (strncmp(g_lines[k], prefix, length) == 0) {
                recorded = true;
                hex.Add(g_lines[k] + length);
            }
        }
        char key[48];
        sprintf(key, "snapshot#%u", n + 1);
        if (g_snapshotData[n] == nullptr) {
            if (recorded)
                Diff(key, "a snapshot", "no allocation");
            continue;
        }
        ReplayText actual;
        ReplayBytes(actual, g_snapshotData[n], g_snapshotBytes[n]);
        HashText(actual.Data());
        actual.Free();
        if (!recorded) {
            Diff(key, "no snapshot", "bytes");
            continue;
        }
        if (hex.Size() / 2 != g_snapshotBytes[n]) {
            char a[24];
            char b[24];
            sprintf(key, "snapshot#%u.length", n + 1);
            sprintf(a, "%u", hex.Size() / 2);
            sprintf(b, "%u", g_snapshotBytes[n]);
            Diff(key, a, b);
            continue;
        }
        unsigned at = ReplayFirstSnapshotDifference(hex.Data(), g_snapshotData[n], g_snapshotBytes[n]);
        if (at != ~0u) {
            char a[8];
            char b[8];
            sprintf(key, "snapshot#%u[%u]", n + 1, at);
            memcpy(a, hex.Data() + at * 2, 2);
            a[2] = '\0';
            sprintf(b, "%02x", g_snapshotData[n][at]);
            Diff(key, a, b);
        }
    }
    hex.Free();
}

bool CachedSubsystem()
{
    return !g_core && strcmp(g_subsystem, "weapons") == 0;
}

void CacheLinesText(const char* lines, ReplayText& out)
{
    if (*lines == '\0')
        out.Add("no change");
    for (const char* p = lines; *p != '\0';) {
        const char* end = strchr(p, '\n');
        if (p != lines)
            out.Add("; ");
        for (const char* c = p + 4; c < end; c++)
            out.AddChar(*c);
        p = end + 1;
    }
}

void CompareCache(const ReplayCacheStart& before)
{
    ReplayCacheStart after;
    const char* failure = ReplayReadDebrisCache(after);
    if (failure != nullptr) {
        Diff("cache debris", "blocks that tile the arena", failure);
        return;
    }
    ReplayText replayed;
    ReplayAddCacheChanges(before, after, replayed);
    ReplayText recorded;
    for (int k = g_callFirst + 1; k < g_callEnd; k++) {
        if (strncmp(g_lines[k], "  d ", 4) == 0) {
            recorded.Add(g_lines[k]);
            recorded.AddChar('\n');
        }
    }
    HashText(replayed.Data());
    if (strcmp(recorded.Data(), replayed.Data()) != 0) {
        ReplayText a;
        ReplayText b;
        CacheLinesText(recorded.Data(), a);
        CacheLinesText(replayed.Data(), b);
        Diff("cache debris", a.Data(), b.Data());
        a.Free();
        b.Free();
    }
    recorded.Free();
    replayed.Free();
}

void (*g_aroundSlot)(bool after);

void ReplayCall(const char* file, Outcome& out)
{
    char* header = g_lines[g_callFirst];
    char* tickText = header + 5;
    char* slotText = strchr(tickText, ' ');
    if (slotText == nullptr)
        Fatal(file, g_callIndex, "bad call line", header);
    *slotText++ = '\0';
    const ReplaySlotSets* sets = FindSlot(g_subsystem, slotText);
    if (sets == nullptr)
        Fatal(file, g_callIndex, "unknown slot", slotText);
    sets = SetsOfKind(sets);
    g_coreImports.Clear();
    g_coreWrites.Clear();
    g_coreClosings.Clear();
    g_diffs = 0;
    g_firstKey[0] = '\0';
    g_detail.Clear();
    BeginHeld();
    g_importIndex = 0;
    g_list.valid = false;
    g_outCount = 0;
    g_rootCount = 0;
    memset(g_snapshotData, 0, sizeof g_snapshotData);
    g_captureUsed = 0;
    if (strcmp(tickText, "-") != 0)
        g_game->ticks = atoi(tickText);

    int first = g_callFirst + 1;
    for (int pass = 0; pass < 2; pass++) {
        for (int k = first; k < g_callEnd; k++) {
            char* line = g_lines[k];
            if (strncmp(line, "  r ", 4) != 0)
                continue;
            char* space = strchr(line + 4, ' ');
            if (space == nullptr)
                Fatal(file, g_callIndex, "bad read line", line);
            *space = '\0';
            bool object = strchr(line + 4, '#') != nullptr;
            if (object && !RestoreRunLine(line + 4, space + 1, file, sets, line, pass) && ObjectLineIsCondition(line + 4) == (pass == 0)) {
                RestoreObjectLine(line + 4, space + 1, file, sets, line);
            } else if (!object && pass == 0) {
                const ReplayField* f = FindBlockField(line + 4);
                if (f == nullptr || !InSet(sets->reads, sets->readCount, f))
                    Fatal(file, g_callIndex, "field is not in the read set of the slot", line + 4);
                if (f->kind == RK_VECTOR) {
                    if (!ApplyBlockVector(*f, line + 4, space + 1, true))
                        Fatal(file, g_callIndex, "a vector line that cannot be applied", space + 1);
                } else if (!RestoreField(*f, space + 1)) {
                    Fatal(file, g_callIndex, "a value that cannot be restored", space + 1);
                }
            }
            *space = ' ';
        }
    }
    g_cursor = first;
    g_hash = 2166136261u;
    g_sets = sets;
    ReplayAnswerHook() = ReplayAnswerImport;
    ReplayEnsureAllOfTables(sets->allOf, sets->allOfCount, AllocObject);
    if (g_game->unitsEnd.get() == nullptr && g_game->units.get() != nullptr && g_game->poolCount != 0)
        g_game->unitsEnd = (Unit*)((unsigned char*)g_game->units.get() + g_game->poolCount * sizeof(Unit) - sizeof(Unit));
    ReplayCacheStart cacheBefore;
    if (CachedSubsystem() && ReplayReadDebrisCache(cacheBefore) != nullptr)
        Fatal(file, g_callIndex, "the debris cache does not tile its arena before the call", slotText);
    SyncLoadObjects();
    if (g_aroundSlot != nullptr)
        g_aroundSlot(false);
    bool ran = RunSlot(g_subsystem, slotText);
    if (g_aroundSlot != nullptr)
        g_aroundSlot(true);
    ReplayAnswerHook() = nullptr;
    if (!ran)
        Fatal(file, g_callIndex, "a slot with no runner", slotText);
    if (ReplayWasmActive() && !ReplayWasmClassWordsIntact())
        Fatal(file, g_callIndex, "the module wrote the class word table", slotText);
    for (int k = g_cursor; k < g_callEnd; k++) {
        if (strncmp(g_lines[k], "  i ", 4) == 0 && !IsQueryLine(g_lines[k])) {
            char key[96];
            const char* nameEnd = strchr(g_lines[k] + 4, ' ');
            int nameLength = nameEnd == nullptr ? (int)strlen(g_lines[k] + 4) : (int)(nameEnd - (g_lines[k] + 4));
            sprintf(key, "import %d %.*s", g_importIndex, nameLength, g_lines[k] + 4);
            Diff(key, g_lines[k] + 4, "no call (missing)");
            break;
        }
    }
    CompareWrites(file, sets);
    if (CachedSubsystem())
        CompareCache(cacheBefore);
    CompareSnapshots();
    RunAfter(g_subsystem, slotText);
    CompareHeld();

    char head[160];
    sprintf(head, "call %d %s %s", g_callIndex, tickText, sets->name);
    out.log.Add(head);
    if (g_diffs == 0) {
        sprintf(head, " ok imports=%d hash=%08x\n", g_importIndex, g_hash);
        out.log.Add(head);
    } else {
        out.log.Add(" DIFFERS\n");
        out.log.AddText(g_detail);
        out.diverged = true;
        out.call = g_callIndex;
        strcpy(out.key, g_firstKey);
    }
    out.imports += g_importIndex;
    out.calls++;
    if (g_core) {
        if (g_callIndex != 0)
            out.core.AddChar(',');
        out.core.AddChar('[');
        out.core.AddText(g_coreImports);
        if (g_coreImports.Size() != 0 && g_coreWrites.Size() + g_coreClosings.Size() != 0)
            out.core.AddChar(',');
        out.core.AddText(g_coreWrites);
        if (g_coreWrites.Size() != 0 && g_coreClosings.Size() != 0)
            out.core.AddChar(',');
        out.core.AddText(g_coreClosings);
        out.core.AddChar(']');
    }
}

enum Mutation { MUT_NONE, MUT_ARGUMENT, MUT_EXTRA, MUT_MISSING };

char g_mutated[TEXT];
int g_mutationCall;
char g_mutationKey[96];

bool Mutate(int mutation)
{
    const char* anchor = "  i weapons.SpawnProjectile";
    const char* anchorName = "weapons.SpawnProjectile";
    bool spawnImport = false;
    for (int k = 2; k < g_lineCount; k++)
        spawnImport = spawnImport || strncmp(g_lines[k], anchor, 27) == 0;
    if (!spawnImport) {
        anchor = "  i services.Emit";
        anchorName = "services.Emit";
    }
    size_t anchorLength = strlen(anchor);
    int call = -1;
    int imports = 0;
    for (int k = 2; k < g_lineCount; k++) {
        if (strncmp(g_lines[k], "call ", 5) == 0) {
            call++;
            imports = 0;
        } else if (strncmp(g_lines[k], "  i ", 4) == 0 && !IsQueryLine(g_lines[k])) {
            if (mutation == MUT_ARGUMENT && strncmp(g_lines[k], anchor, anchorLength) == 0) {
                sprintf(g_mutated, "%s", g_lines[k]);
                char* digit = spawnImport ? strstr(g_mutated, "weapondef#") + 10 : strchr(g_mutated, ',') + 1;
                *digit = *digit == '9' ? '8' : '9';
                g_lines[k] = g_mutated;
                g_mutationCall = call;
                sprintf(g_mutationKey, "import %d %s", imports, anchorName);
                return true;
            }
            if (mutation == MUT_MISSING && strncmp(g_lines[k], anchor, anchorLength) == 0) {
                for (int j = k; j + 1 < g_lineCount; j++)
                    g_lines[j] = g_lines[j + 1];
                g_lineCount--;
                g_mutationCall = call;
                sprintf(g_mutationKey, "import %d %s", imports, anchorName);
                return true;
            }
            if (mutation == MUT_EXTRA && strncmp(g_lines[k], anchor, anchorLength) == 0) {
                int last = k;
                while (last + 1 < g_lineCount && strncmp(g_lines[last + 1], "  ", 2) == 0 && strncmp(g_lines[last + 1], "  w ", 4) != 0)
                    last++;
                sprintf(g_mutated, "  i services.RandomInt 7 = 3");
                for (int j = g_lineCount; j > last + 1; j--)
                    g_lines[j] = g_lines[j - 1];
                g_lines[last + 1] = g_mutated;
                g_lineCount++;
                g_mutationCall = call;
                int total = 0;
                for (int j = k; j <= last; j++)
                    total += strncmp(g_lines[j], "  i ", 4) == 0;
                sprintf(g_mutationKey, "import %d services.RandomInt", imports + total);
                return true;
            }
            imports++;
        }
    }
    return false;
}

void InitDebrisForReplay(const char* file)
{
    InitDebrisPieces();
    if (!g_cacheStart.taken)
        return;
    if (g_cacheStart.cap != (unsigned)g_debrisMemCache.cap)
        Fatal(file, 0, "the recorded debris cache has another size than the game's", "cache");
    unsigned char* base = (unsigned char*)g_debrisMemCache.base.get();
    for (size_t i = 0; i < g_cacheStart.blocks.size(); i++) {
        const ReplayCacheBlock& b = g_cacheStart.blocks[i];
        DebrisCacheChunk* chunk = (DebrisCacheChunk*)(base + b.offset);
        chunk->size = (int)b.size;
        chunk->owner = b.owner < 0 ? nullptr : (Ref<char>*)&g_explodedPieces[b.owner];
        if (b.live)
            g_explodedPieces[b.owner] = (ExplodedPiece*)(base + b.offset + 8);
    }
    g_debrisMemCache.cur = (DebrisCacheChunk*)(base + g_cacheStart.cur);
}

void ReplayFile(const char* file, Outcome& out, int mutation = MUT_NONE)
{
    if (!LoadFile(file)) {
        printf("FAIL: cannot read %s\n", file);
        exit(2);
    }
    g_file = file;
    if (g_lineCount < 2 || strcmp(g_lines[0], "bt-replay 2") != 0 || strncmp(g_lines[1], "subsystem ", 10) != 0)
        Fatal(file, 0, "not a bt-replay 2 recording", g_lines[0]);
    char version[32] = "";
    char tail[32] = "";
    if (sscanf(g_lines[1], "subsystem %47s %31s %31s", g_subsystem, version, tail) < 2)
        Fatal(file, 0, "bad subsystem line", g_lines[1]);
    const ReplaySubsystem* known = nullptr;
    for (unsigned s = 0; s < kReplaySubsystemCount; s++) {
        if (strcmp(kReplaySubsystems[s].name, g_subsystem) == 0)
            known = &kReplaySubsystems[s];
    }
    if (known == nullptr)
        Fatal(file, 0, "not a simulation subsystem", g_subsystem);
    g_importChanges = known->recordsImportChanges != 0;
    g_forgetAllocations = known->forgetsAllocations != 0;
    if (tail[0] != '\0' || !known->complete)
        Fatal(file, 0, "the annotations of this subsystem are not complete, so its recordings cannot be replayed", g_subsystem);
    char want[32];
    sprintf(want, "%u.%u", known->major, known->minor);
    if (strcmp(version, want) != 0)
        Fatal(file, 0, "the interface version differs from the schema's", version);
    if (ReplayWasmActive() && strcmp(ReplayWasmSubsystem(), g_subsystem) != 0)
        Fatal(file, 0, "the WASM module is of another subsystem", ReplayWasmSubsystem());
    if (mutation != MUT_NONE && !Mutate(mutation))
        Fatal(file, 0, "nothing to mutate", "no spawn import");
    ResetBlock();
    out.calls = 0;
    out.imports = 0;
    out.diverged = false;
    out.call = -1;
    out.key[0] = '\0';
    out.log.Clear();
    out.core.Clear();
    g_coreDropped = false;
    int at = 2;
    while (at < g_lineCount && strncmp(g_lines[at], "call ", 5) != 0)
        at++;
    g_callIndex = 0;
    LoadInputs(file, at);
    if (strcmp(g_subsystem, "weapons") == 0)
        InitDebrisForReplay(file);
    while (at < g_lineCount && (g_core || !out.diverged)) {
        if (g_lines[at][0] == '\0') {
            at++;
            continue;
        }
        if (strncmp(g_lines[at], "call ", 5) != 0)
            Fatal(file, g_callIndex, "expected a call line", g_lines[at]);
        int end = at + 1;
        while (end < g_lineCount && strncmp(g_lines[end], "  ", 2) == 0)
            end++;
        g_callFirst = at;
        g_callEnd = end;
        ReplayCall(file, out);
        g_callIndex++;
        at = end;
    }
}

int Replay(const char* file, int expectCall, const char* expectField)
{
    Outcome first;
    Outcome second;
    ReplayFile(file, first);
    ReplayFile(file, second);
    Check(strcmp(first.log.Data(), second.log.Data()) == 0, "replaying twice gives the same result");
    if (expectField == nullptr) {
        if (first.diverged) {
            printf("DIVERGED %s: first at call %d, %s\n%s", file, first.call, first.key, first.log.Data() + 0);
            g_failures++;
        } else {
            printf("%s: %d calls, %d imports, identical on a second run\n", file, first.calls, first.imports);
        }
        return 0;
    }
    if (!first.diverged) {
        printf("FAIL: %s: the injected change was not reported\n", file);
        g_failures++;
    } else {
        const char* tail = strstr(first.log.Data(), "DIFFERS");
        printf("%s: reported at call %d, %s\n%s", file, first.call, first.key, tail == nullptr ? "" : tail);
        Check(first.call == expectCall, "the injected change is reported at the expected call");
        Check(strcmp(first.key, expectField) == 0, "the injected change is reported at the expected field");
    }
    return 0;
}

const unsigned short* AllStateFields(unsigned* count)
{
    static unsigned short all[512];
    unsigned n = 0;
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        if (kReplayObjectKinds[kReplayObjectFields[i].object].scope == RS_STATE)
            all[n++] = (unsigned short)i;
    }
    *count = n;
    return all;
}

void RecordGraph(ReplayText& text, ReplayKnown& known, void* player, const ReplaySlotSets& sets)
{
    ReplayObjectTable* table = new ReplayObjectTable;
    ReplayReach* reach = new ReplayReach;
    unsigned short kind = ROK_PLAYER;
    ReplayWalk(*reach, *table, true, sets, &kind, &player, 1);
    ReplayCaptureObjects(text, known, *table, *reach, sets.objReads, sets.objReadCount, 'r');
    delete reach;
    delete table;
}

void ObjectSelfTest()
{
    printf("object round trip\n");
    ResetBlock();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };

    Unit* pool = (Unit*)SimAlloc(8 * (unsigned)sizeof(Unit));
    g_game->units = pool;
    g_game->poolCount = 8;
    Player* player = &g_game->players[3];
    Unit* a = &pool[2];
    Unit* m = &pool[3];
    Unit* b = &pool[5];
    UnitDef* defs = (UnitDef*)SimAlloc(2 * (unsigned)sizeof(UnitDef));
    g_game->defs = defs;
    g_game->unitDefCount = 2;
    UnitDef* def = &defs[1];
    player->active = 1;
    player->id = 7;
    player->energy = 1500.5f;
    player->metal = 20.25f;
    player->unitsBegin = a;
    player->unitsEnd = b;
    a->health = 100;
    a->buildLeft = 0.5f;
    a->def = def;
    a->player = player;
    m->health = 3;
    m->def = def;
    b->health = -5;
    b->def = def;
    b->player = player;
    def->energymake = 25.0f;
    def->windgenerator = 0.75f;
    player->totalEnergyProduced = 123.5;
    player->allied[3] = 1;
    player->type = 2;
    a->cell.x = 5;
    a->cell.y = 6;
    a->flags = 0x10000000u;
    UnitMotion* motion = (UnitMotion*)SimAlloc(sizeof(UnitMotion));
    Unit* aimed = &pool[7];
    a->motion = motion;
    a->weapons[0].aimTargetZOrUnitSentinel = (short)0x8000;
    a->weapons[0].aimTargetXOrUnitId = 7;
    a->weapons[1].aimTargetXOrUnitId = 6;
    a->weapons[1].aimTargetZOrUnitSentinel = 5;
    aimed->unitDefIndex = 9;
    aimed->health = 77;
    aimed->motion = motion;
    motion->velocity.x = 11;
    UnitTypeSet* mask = (UnitTypeSet*)SimAlloc(sizeof(UnitTypeSet));
    mask->bits[0] = 0x12345678;
    mask->bits[15] = (int)0x80000001u;
    def->categories = mask;

    ReplayText text;
    ReplayKnown recorded;
    RecordGraph(text, recorded, player, sets);
    Check(strstr(text.Data(), "  r Player#3.unitsBegin Unit#2\n") != nullptr, "the player's first unit is named by its slot");
    Check(strstr(text.Data(), "  r Player#3.unitsEnd Unit#5\n") != nullptr, "the end of the range is the last unit, not one past it");
    Check(strstr(text.Data(), "  r Unit#3.health 3\n") != nullptr, "a unit inside the range is reached");
    Check(strstr(text.Data(), "  r Unit#4.health 0\n") != nullptr, "an empty slot of the range is reached");
    Check(strstr(text.Data(), "  r Unit#6.") == nullptr && strstr(text.Data(), "  r Unit#1.") == nullptr, "no slot outside the range is reached");
    Check(strstr(text.Data(), "  r Unit#5.def UnitDef#1\n") != nullptr, "the units share UnitDef#1");
    Check(strstr(text.Data(), "UnitDef#2") == nullptr, "no other definition is reached");
    Check(strstr(text.Data(), "  r Player#3.totalEnergyProduced f64:0x405ee00000000000\n") != nullptr, "a double is written as its bits");
    Check(strstr(text.Data(), "  r Player#3.allied x:0000000100000000000000\n") != nullptr, "an array of bytes is written in hex");
    Check(strstr(text.Data(), "  r Unit#2.cell (5,6)\n") != nullptr, "a cell is written as a point");
    Check(strstr(text.Data(), "  r Unit#7.unitDefIndex 9\n") != nullptr, "a unit named by the id of a weapon slot is reached");
    Check(strstr(text.Data(), "  r Unit#7.health") == nullptr, "and only the fields of its reach are recorded");
    Check(strstr(text.Data(), "  r Unit#6.") == nullptr, "a slot that holds a position, not an id, is not followed");
    Check(strstr(text.Data(), "  r UnitMotion#1.velocityX 11\n") != nullptr, "the reach of the unit leads on to its motion");
    Check(strstr(text.Data(), "weapon0TargetUnit") == nullptr, "a unit-id field is never written as a line");
    Check(strstr(text.Data(), "  r UnitDef#1.categories UnitTypeSet#1\n") != nullptr, "a category mask is an object of its own");
    Check(strstr(text.Data(), "  r UnitTypeSet#1.bits x:78563412") != nullptr && strstr(text.Data(), "01000080\n") != nullptr, "the words of a mask are written in hex");
    Check(strstr(text.Data(), "  r UnitDef#1.weaponCategories0 null\n") != nullptr, "a null mask is null");

    ResetBlock();
    g_callIndex = 0;
    char* copy = (char*)malloc(text.Size() + 1);
    memcpy(copy, text.Data(), text.Size() + 1);
    int lines = 0;
    for (char* p = copy; *p != '\0'; lines++) {
        char* end = strchr(p, '\n');
        *end = '\0';
        char* space = strchr(p + 4, ' ');
        *space = '\0';
        Check(RestoreObjectLine(p + 4, space + 1, "object self-test", &sets, p), "a recorded line restores");
        p = end + 1;
    }
    Check(lines > 0, "lines were restored");
    free(copy);
    Player* rebuilt = (Player*)ReplayResolveObject("Player#3", nullptr, g_objects, AllocObject);
    Unit* ra = (Unit*)ReplayResolveObject("Unit#2", nullptr, g_objects, AllocObject);
    Unit* rm = (Unit*)ReplayResolveObject("Unit#3", nullptr, g_objects, AllocObject);
    Unit* rb = (Unit*)ReplayResolveObject("Unit#5", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && ra != nullptr && rb != nullptr, "the objects are rebuilt in the block");
    Check(rebuilt == &g_game->players[3], "a player is a slot of the block's player array");
    Check(rebuilt->unitsBegin.get() == ra && rebuilt->unitsEnd.get() == rb, "the player's refs point at the rebuilt units");
    Check(rm == ra + 1 && rb == ra + 3, "the slice of the unit table is contiguous");
    Check(rm->health == 3, "a unit inside the range is restored");
    Check(ra->def.get() != nullptr && ra->def.get() == rb->def.get(), "both units share one rebuilt UnitDef");
    Check(ra->player.get() == rebuilt && rb->player.get() == rebuilt, "the units point back at the player");
    Unit* raimed = (Unit*)ReplayResolveObject("Unit#7", nullptr, g_objects, AllocObject);
    Check(raimed != nullptr && raimed->unitDefIndex == 9 && raimed->motion.get() != nullptr && raimed->motion.get()->velocity.x == 11, "the unit named by an id is rebuilt in its slot, with its motion");
    Check(raimed != nullptr && raimed->health == 0, "and nothing else of it");
    Check(ra->def.get()->categories.get() != nullptr && ra->def.get()->categories->bits[0] == 0x12345678 && ra->def.get()->categories->bits[15] == (int)0x80000001u && ra->def.get()->weaponCategories[0].get() == nullptr, "a mask is rebuilt in the block");
    Check(rebuilt->totalEnergyProduced == 123.5 && rebuilt->allied[3] == 1 && rebuilt->allied[2] == 0, "a double and an array of bytes are restored");

    ReplayText again;
    ReplayKnown fresh;
    RecordGraph(again, fresh, rebuilt, sets);
    Check(strcmp(text.Data(), again.Data()) == 0, "the rebuilt graph records the same text");

    g_rootKinds[0] = ROK_PLAYER;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    g_diffs = 0;
    g_detail.Clear();
    g_firstKey[0] = '\0';
    g_hash = 2166136261u;
    CompareObjects(&sets);
    Check(g_diffs == 0, "the unchanged graph compares equal");
    rb->health = 99;
    g_diffs = 0;
    g_detail.Clear();
    CompareObjects(&sets);
    printf("a changed field: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "Unit#5.health") == 0, "a changed field is reported at its Kind#N.field");
    rb->health = -5;
    ra->def.get()->windgenerator = 1.5f;
    g_diffs = 0;
    g_detail.Clear();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "UnitDef#1.windgenerator") == 0, "a field of the shared definition is reported once");
    text.Free();
    again.Free();
}


void LoadCall(const char* body)
{
    free(g_text);
    g_text = (char*)malloc(strlen(body) + 16);
    strcpy(g_text, "call 1 Fake\n");
    strcat(g_text, body);
    SplitLines();
    g_callFirst = 0;
    g_callEnd = g_lineCount;
    g_callIndex = 0;
}

struct Recorder {
    Recorder() : table(new ReplayObjectTable), reach(new ReplayReach) {}
    ~Recorder()
    {
        delete reach;
        delete table;
        text.Free();
    }
    void Record(const unsigned short* kinds, void* const* roots, unsigned count, const ReplaySlotSets& sets)
    {
        text.Clear();
        ReplayWalk(*reach, *table, true, sets, kinds, roots, count);
        ReplayCaptureObjects(text, known, *table, *reach, sets.objReads, sets.objReadCount, 'r');
    }
    ReplayObjectTable* table;
    ReplayReach* reach;
    ReplayKnown known;
    ReplayText text;

private:
    Recorder(const Recorder&);
    Recorder& operator=(const Recorder&);
};

int RestoreText(const char* text, const ReplaySlotSets& sets)
{
    char* copy = (char*)malloc(strlen(text) + 1);
    int lines = 0;
    for (int pass = 0; pass < 2; pass++) {
        strcpy(copy, text);
        for (char* p = copy; *p != '\0'; p = strchr(p, '\0') + 1) {
            char* end = strchr(p, '\n');
            *end = '\0';
            char* space = strchr(p + 4, ' ');
            *space = '\0';
            if (!RestoreRunLine(p + 4, space + 1, "self-test", &sets, p, pass) && ObjectLineIsCondition(p + 4) == (pass == 0))
                Check(RestoreObjectLine(p + 4, space + 1, "self-test", &sets, p), "a recorded line restores");
            lines += pass == 0 ? 1 : 0;
            p = end;
        }
    }
    free(copy);
    return lines;
}

int CountLines(const char* text, const char* prefix)
{
    int n = 0;
    unsigned length = (unsigned)strlen(prefix);
    for (const char* p = text; p != nullptr && *p != '\0'; p = strchr(p, '\n') == nullptr ? nullptr : strchr(p, '\n') + 1) {
        if (strncmp(p, prefix, length) == 0)
            n++;
    }
    return n;
}

unsigned LongestLine(const char* text)
{
    unsigned longest = 0;
    for (const char* p = text; *p != '\0';) {
        const char* end = strchr(p, '\n');
        unsigned length = end == nullptr ? (unsigned)strlen(p) : (unsigned)(end - p);
        if (length > longest)
            longest = length;
        p = end == nullptr ? p + length : end + 1;
    }
    return longest;
}

void BeginCompare()
{
    g_diffs = 0;
    g_detail.Clear();
    g_firstKey[0] = '\0';
    g_hash = 2166136261u;
}

void TableSelfTest()
{
    printf("indexed tables\n");
    ResetBlock();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    Unit* pool = (Unit*)SimAlloc(8 * (unsigned)sizeof(Unit));
    g_game->units = pool;
    g_game->poolCount = 8;
    Projectile* shots = (Projectile*)SimAlloc(8 * (unsigned)sizeof(Projectile));
    memset(shots, 0, 8 * sizeof(Projectile));
    g_game->projectiles = shots;
    g_game->projectileCount = 8;
    Projectile* shot = &shots[3];
    shot->speed = 77;
    shot->spawnTick = 900;
    shot->attacker = &pool[2];
    shot->active = 1;
    shot->owner = 4;
    pool[2].health = 12;
    MovementClass* movement = &MovementEntries()[5];
    movement->footprintX = 3;
    movement->maxSlope = 9;

    unsigned short kinds[2] = { ROK_PROJECTILE, ROK_MOVEMENTCLASS };
    void* roots[2] = { shot, movement };
    Recorder recorder;
    recorder.Record(kinds, roots, 2, sets);
    const char* text = recorder.text.Data();
    Check(strstr(text, "  r Projectile#3.speed 77\n") != nullptr, "a projectile is named by its index in the pool");
    Check(strstr(text, "  r Projectile#3.attacker Unit#2\n") != nullptr, "a pointer from one table to another names the index");
    Check(strstr(text, "  r MovementClass#5.maxSlope 9\n") != nullptr, "an entry of a table in the sim globals is named by its index");
    Check(strstr(text, "  r Unit#2.health 12\n") != nullptr, "the unit reached through the projectile is recorded");
    Check(strstr(text, "#?") == nullptr, "every object is in a table");

    ResetBlock();
    RestoreText(text, sets);
    Projectile* rebuilt = (Projectile*)ReplayResolveObject("Projectile#3", nullptr, g_objects, AllocObject);
    MovementClass* rebuiltClass = (MovementClass*)ReplayResolveObject("MovementClass#5", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt == &g_game->projectiles.get()[3], "a projectile is a slot of the rebuilt pool");
    Check(ReplayResolveObject("Projectile#299", nullptr, g_objects, AllocObject) == &g_game->projectiles.get()[299], "the last slot of the pool resolves");
    Check(ReplayResolveObject("Projectile#300", nullptr, g_objects, AllocObject) == nullptr, "the pool holds the 300 slots the schema says");
    Check(rebuilt != nullptr && rebuilt->speed == 77 && rebuilt->spawnTick == 900 && rebuilt->owner == 4, "its fields are restored");
    Check(rebuilt != nullptr && rebuilt->attacker.get() == &g_game->units.get()[2], "its attacker is a slot of the unit pool");
    Check(rebuiltClass == &MovementEntries()[5] && rebuiltClass->maxSlope == 9, "an entry of the sim table is restored in place");

    Recorder again;
    unsigned short againKinds[2] = { ROK_PROJECTILE, ROK_MOVEMENTCLASS };
    void* againRoots[2] = { rebuilt, rebuiltClass };
    again.Record(againKinds, againRoots, 2, sets);
    Check(strcmp(text, again.text.Data()) == 0, "the rebuilt tables record the same text");

    memcpy(g_rootKinds, againKinds, sizeof againKinds);
    g_rootObjects[0] = rebuilt;
    g_rootObjects[1] = rebuiltClass;
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the unchanged tables compare equal");
    rebuilt->speed = 78;
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed field: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "Projectile#3.speed") == 0, "a changed projectile is reported at its index and field");
    rebuilt->speed = 77;
    rebuiltClass->maxSlope = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "MovementClass#5.maxSlope") == 0, "a changed entry of the sim table is reported at its index and field");
}

void ArraySelfTest()
{
    printf("sized arrays\n");
    ResetBlock();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    Player* player = &g_game->players[3];
    player->exploredSize = 1500;
    unsigned char* explored = (unsigned char*)SimAlloc(1500);
    for (int i = 0; i < 1500; i++)
        explored[i] = (unsigned char)(i * 7 + 1);
    player->explored = explored;
    PassMap* movement = &PassMapEntries()[2];
    movement->width = 4;
    movement->height = 32;
    unsigned* cells = (unsigned*)SimAlloc(8 * 4);
    for (int i = 0; i < 8; i++)
        cells[i] = 0x01020304u * (unsigned)(i + 1);
    movement->cells = cells;
    unsigned short kinds[2] = { ROK_PLAYER, ROK_PASSMAP };
    void* roots[2] = { player, movement };

    Recorder recorder;
    recorder.Record(kinds, roots, 2, sets);
    ReplayText calls;
    calls.AddText(recorder.text);
    const char* first = recorder.text.Data();
    Check(strstr(first, "  r Player#3.explored [1500] 0:") != nullptr, "the first use writes the whole array, with its length");
    Check(CountLines(first, "  r Player#3.explored ") == 3, "a long array is chunked into lines");
    Check(strstr(first, "  r Player#3.explored 512:") != nullptr && strstr(first, "  r Player#3.explored 1024:") != nullptr, "each chunk says where it starts");
    Check(LongestLine(first) < 1100, "the lines are bounded");
    Check(strstr(first, "  r PassMap#2.cells [8] 0:04030201") != nullptr, "the elements are written as the bytes of memory");

    explored[5] = 0xaa;
    explored[6] = 0xbb;
    explored[1000] = 0xcc;
    cells[3] = 0xdeadbeefu;
    recorder.Record(kinds, roots, 2, sets);
    calls.AddText(recorder.text);
    const char* second = recorder.text.Data();
    Check(strstr(second, "  r Player#3.explored 5:aabb 1000:cc\n") != nullptr, "a later call writes the changed runs only");
    Check(strstr(second, "  r PassMap#2.cells 3:efbeadde\n") != nullptr, "a changed element of a wider array is one run");
    Check(CountLines(second, "  r ") == 2, "and nothing else");

    explored[1] = 0;
    recorder.Record(kinds, roots, 2, sets);
    Check(strstr(recorder.text.Data(), "  r Player#3.explored 1:00\n") != nullptr, "a single element is a run of one");
    explored[1] = (unsigned char)(1 * 7 + 1);
    player->explored = nullptr;
    recorder.Record(kinds, roots, 2, sets);
    calls.AddText(recorder.text);
    Check(strstr(recorder.text.Data(), "  r Player#3.explored null\n") != nullptr, "a null array is null");
    unsigned char* smaller = (unsigned char*)SimAlloc(40);
    memset(smaller, 9, 40);
    player->explored = smaller;
    player->exploredSize = 40;
    recorder.Record(kinds, roots, 2, sets);
    calls.AddText(recorder.text);
    Check(strstr(recorder.text.Data(), "  r Player#3.explored [40] 0:09090909") != nullptr, "an array that was null or changed length is written whole again");

    ResetBlock();
    int lines = RestoreText(calls.Data(), sets);
    Check(lines > 8, "the lines of every call restore");
    Player* rebuilt = (Player*)ReplayResolveObject("Player#3", nullptr, g_objects, AllocObject);
    PassMap* rebuiltClass = (PassMap*)ReplayResolveObject("PassMap#2", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->exploredSize == 40 && rebuilt->explored.get() != nullptr && rebuilt->explored.get()[39] == 9 && rebuilt->explored.get()[40 - 1] == 9, "the last whole write wins");
    Check(rebuiltClass != nullptr && rebuiltClass->cells.get() != nullptr && rebuiltClass->cells.get()[3] == 0xdeadbeefu && rebuiltClass->cells.get()[2] == 0x01020304u * 3, "the delta is applied to the array the replayer kept");

    unsigned short kind = ROK_PLAYER;
    void* root = rebuilt;
    Recorder replayed;
    replayed.Record(&kind, &root, 1, sets);
    Check(strstr(replayed.text.Data(), "  r Player#3.explored [40] 0:09090909") != nullptr, "the replayed array records as it was");

    g_rootKinds[0] = ROK_PLAYER;
    g_rootObjects[0] = rebuilt;
    g_rootKinds[1] = ROK_PASSMAP;
    g_rootObjects[1] = rebuiltClass;
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the arrays compare equal after the deltas");
    rebuiltClass->cells.get()[6] ^= 0x100u;
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed element: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "PassMap#2.cells") == 0, "a changed element is reported at its array");
    Check(strstr(g_detail.Data(), "byte 25 of 32") != nullptr, "and at its byte");
    rebuiltClass->cells.get()[6] ^= 0x100u;

    printf("a pass map whose height is not a multiple of 16\n");
    ResetBlock();
    PassMap* partial = &PassMapEntries()[3];
    partial->width = 4;
    partial->height = 20;
    unsigned* rows = (unsigned*)SimAlloc(8 * 4);
    for (int i = 0; i < 8; i++)
        rows[i] = 0x01010101u * (unsigned)(i + 1);
    partial->cells = rows;
    unsigned short partialKind = ROK_PASSMAP;
    void* partialRoot = partial;
    Recorder partialRecorder;
    partialRecorder.Record(&partialKind, &partialRoot, 1, sets);
    ReplayText partialCalls;
    partialCalls.AddText(partialRecorder.text);
    Check(strstr(partialRecorder.text.Data(), "  r PassMap#3.cells [8] 0:01010101") != nullptr, "the whole allocation is recorded, the last row included");
    ResetBlock();
    RestoreText(partialCalls.Data(), sets);
    PassMap* partialRebuilt = (PassMap*)ReplayResolveObject("PassMap#3", nullptr, g_objects, AllocObject);
    g_rootKinds[0] = ROK_PASSMAP;
    g_rootObjects[0] = partialRebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the last row compares equal after the restore");
    partialRebuilt->cells.get()[7] ^= 1u;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "PassMap#3.cells") == 0, "a one-word change in the last row is reported");
    Check(strstr(g_detail.Data(), "byte 28 of 32") != nullptr, "at its byte");

    printf("long deltas\n");
    ResetBlock();
    Player* wide = &g_game->players[4];
    wide->exploredSize = 1500;
    unsigned char* field = (unsigned char*)SimAlloc(1500);
    for (int i = 0; i < 1500; i++)
        field[i] = (unsigned char)i;
    wide->explored = field;
    unsigned short wideKind = ROK_PLAYER;
    void* wideRoot = wide;
    Recorder wideRecorder;
    ReplayText wideCalls;
    wideRecorder.Record(&wideKind, &wideRoot, 1, sets);
    wideCalls.AddText(wideRecorder.text);
    for (int i = 0; i < 1500; i += 3)
        field[i] = (unsigned char)(field[i] + 77);
    wideRecorder.Record(&wideKind, &wideRoot, 1, sets);
    wideCalls.AddText(wideRecorder.text);
    Check(CountLines(wideRecorder.text.Data(), "  r Player#4.explored ") >= 3, "a delta of many runs is several lines");
    Check(LongestLine(wideCalls.Data()) < 1300, "and every line of it is bounded");
    unsigned char expected[1500];
    memcpy(expected, field, sizeof expected);
    ResetBlock();
    RestoreText(wideCalls.Data(), sets);
    Player* wideBack = (Player*)ReplayResolveObject("Player#4", nullptr, g_objects, AllocObject);
    bool same = wideBack != nullptr && wideBack->explored.get() != nullptr;
    for (int i = 0; same && i < 1500; i++)
        same = wideBack->explored.get()[i] == expected[i];
    Check(same, "the runs of every line are applied");
    wideCalls.Free();

    printf("block arrays\n");
    ResetBlock();
    const ReplayField* mask = FindField("Game.visibilityMask");
    Check(mask != nullptr && mask->kind == RK_ARRAY, "a block field can be an array");
    g_game->width = 8;
    g_game->height = 8;
    unsigned short* bits = (unsigned short*)SimAlloc(16 * 2);
    for (int i = 0; i < 16; i++)
        bits[i] = (unsigned short)(i * 0x101);
    g_game->visibilityMask = bits;
    ReplayKnown known;
    ReplayText out;
    ReplayText scratch;
    ReplayObjectTable* table = new ReplayObjectTable;
    ReplayEmitField(out, known, *table, 'r', mask->name, ReplayFieldView(*mask), BlockBase(*mask), scratch);
    Check(strstr(out.Data(), "  r Game.visibilityMask [16] 0:0000010102020303") != nullptr, "the whole mask is written at first use");
    bits[9] = 0xffff;
    bits[10] = 0;
    ReplayText second_call;
    ReplayEmitField(second_call, known, *table, 'r', mask->name, ReplayFieldView(*mask), BlockBase(*mask), scratch);
    Check(strcmp(second_call.Data(), "  r Game.visibilityMask 9:ffff0000\n") == 0, "and the changed elements after it");
    ResetBlock();
    g_game->width = 8;
    g_game->height = 8;
    char* line = (char*)malloc(out.Size() + second_call.Size() + 1);
    strcpy(line, out.Data());
    strcat(line, second_call.Data());
    for (char* p = line; *p != '\0';) {
        char* end = strchr(p, '\n');
        *end = '\0';
        char* space = strchr(p + 4, ' ');
        *space = '\0';
        Check(RestoreField(*FindField(p + 4), space + 1), "a mask line restores");
        p = end + 1;
    }
    free(line);
    unsigned short* replayedBits = g_game->visibilityMask.get();
    Check(replayedBits != nullptr && replayedBits[9] == 0xffff && replayedBits[10] == 0 && replayedBits[15] == 0x0f0f, "the mask is rebuilt in the block");
    ReplayText now;
    FormatField(*mask, now);
    Check(strcmp(now.Data(), g_known.Get("Game.visibilityMask")) == 0, "the replayed mask is the recorded one");
    replayedBits[4] ^= 1;
    FormatField(*mask, now);
    BeginCompare();
    Diff("Game.visibilityMask", g_known.Get("Game.visibilityMask"), now.Data());
    printf("a changed element: %s\n%s", g_firstKey, g_detail.Data());
    Check(strcmp(g_firstKey, "Game.visibilityMask") == 0 && strstr(g_detail.Data(), "byte 8 of 32: recorded 04, replayed 05") != nullptr, "a changed element of the mask is reported at the field");
    out.Free();
    second_call.Free();
    scratch.Free();
    now.Free();
    calls.Free();
    delete table;
}

void FormSelfTest()
{
    printf("struct forms\n");
    ResetBlock();
    ReplayText t;
    Vec3 pos = { 1, -2, 3 };
    ReplayAddStruct(t, "a", "pos", ROK_VEC3, &pos, g_objects);
    Check(strcmp(t.Data(), "  a pos.x 1\n  a pos.y -2\n  a pos.z 3\n") == 0, "a struct in is a line per field");
    LoadCall(t.Data());
    Vec3 got = { 0, 0, 0 };
    Vec3* p = (Vec3*)ParseStructOpt("pos", ROK_VEC3, &got);
    Check(p == &got && got.x == 1 && got.y == -2 && got.z == 3, "and replays to the same struct");
    t.Clear();
    ReplayAddStruct(t, "a", "pos", ROK_VEC3, nullptr, g_objects);
    Check(strcmp(t.Data(), "  a pos null\n") == 0, "a null struct in is one line");
    LoadCall(t.Data());
    Vec3 untouched = { 5, 5, 5 };
    Check(ParseStructOpt("pos", ROK_VEC3, &untouched) == nullptr && untouched.x == 5, "and replays to a null pointer");
    t.Clear();
    ReplayAddStruct(t, "a", "pos", ROK_VEC3, &pos, g_objects);
    LoadCall("  a pos.x 1\n  a pos.y -2\n  a pos.z 9\n");
    got.x = got.y = got.z = 0;
    ParseStructOpt("pos", ROK_VEC3, &got);
    BeginCompare();
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[i];
        if (f.object != ROK_VEC3)
            continue;
        ReplayText want;
        ReplayText have;
        ReplayFormatObjectField(f, &pos, g_objects, false, want);
        ReplayFormatObjectField(f, &got, g_objects, false, have);
        if (strcmp(want.Data(), have.Data()) != 0) {
            char key[32];
            sprintf(key, "pos.%s", f.name);
            Diff(key, want.Data(), have.Data());
        }
        want.Free();
        have.Free();
    }
    printf("a changed field: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "pos.z") == 0, "a changed field of a struct in is reported at its name");

    printf("events\n");
    TargetingSetPos aim;
    memset(&aim, 0, sizeof aim);
    aim.h.kind = TARGETING_SET_POS;
    aim.h.weapon = 2;
    aim.pos[0] = 10;
    aim.pos[1] = -20;
    aim.pos[2] = 30;
    t.Clear();
    ReplayAddEvent(t, "event", RET_TARGETING, &aim, sizeof aim, g_objects);
    Check(strstr(t.Data(), "  a event.pos2 30\n") != nullptr && strstr(t.Data(), "  a event.target ") == nullptr, "an event in is the lines of its kind's payload");
    LoadCall(t.Data());
    unsigned char buffer[REPLAY_EVENT_MAX];
    memset(buffer, 0, sizeof buffer);
    ParseEvent("event", RET_TARGETING, buffer);
    TargetingSetPos* eventBack = (TargetingSetPos*)buffer;
    Check(eventBack->h.kind == TARGETING_SET_POS && eventBack->h.weapon == 2 && eventBack->pos[0] == 10 && eventBack->pos[1] == -20 && eventBack->pos[2] == 30, "and replays to the same payload");
    TargetingEventHeader clear;
    memset(&clear, 0, sizeof clear);
    clear.kind = TARGETING_CLEAR;
    t.Clear();
    ReplayAddEvent(t, "event", RET_TARGETING, &clear, sizeof clear, g_objects);
    Check(strstr(t.Data(), "event.pos") == nullptr && strstr(t.Data(), "event.weapon") != nullptr, "an event of the header alone is the lines of the header");
    t.Clear();
    ReplayAddEvent(t, "event", RET_TARGETING, &aim, sizeof(TargetingEventHeader), g_objects);
    Check(strstr(t.Data(), "  a event.pos0 0\n") != nullptr, "an event copies exactly its size: the payload beyond it reads as zero");

    printf("an event that holds the offset of a polymorphic object\n");
    LoadCall("  a event.kind 326\n  a event.unit null\n  a event.target ApproachRadius#1\n");
    memset(buffer, 0, sizeof buffer);
    ParseEvent("event", RET_MOVEMENT, buffer);
    MovementTarget* targetBack = (MovementTarget*)buffer;
    void* radius = ReplayRefPtr(targetBack->target);
    Check(targetBack->h.kind == MOVEMENT_SET_TARGET && radius != nullptr && *(const void**)radius == ReplayVtable(ROK_APPROACHRADIUS), "the target is the offset of an object of the named kind, with its vtable");
    t.Clear();
    ReplayAddEvent(t, "event", RET_MOVEMENT, buffer, sizeof(MovementTarget), g_objects);
    Check(strstr(t.Data(), "  a event.target ApproachRadius#1\n") != nullptr, "and is written as the kind its vtable says");
    LoadCall("  a event.kind 326\n  a event.unit null\n  a event.target null\n");
    memset(buffer, 0, sizeof buffer);
    ParseEvent("event", RET_MOVEMENT, buffer);
    Check(targetBack->target == 0, "and null is 0");

    printf("refused values\n");
    LoadCall("  a event.kind 1\n  a event.unit FrameTable#?\n");
    memset(buffer, 0, sizeof buffer);
    Check(!ParseEventFields("event", RET_TARGETING, buffer) && strcmp(g_refusedKey, "event.unit") == 0, "an event field the restore refuses stops the replay at its name");
    LoadCall("  a pos.x 1\n");
    Vec3 refusedPos = Vec3();
    Check(ParseStructFields("pos", ROK_VEC3, &refusedPos) && refusedPos.x == 1, "and a struct of fields it takes is restored");
    TargetingSetUnit refusedUnit;
    memset(&refusedUnit, 0, sizeof refusedUnit);
    LoadCall("  a aim.target FrameTable#?\n");
    Check(!ParseStructFields("aim", ROK_TARGETINGSETUNIT, &refusedUnit) && strcmp(g_refusedKey, "aim.target") == 0, "a struct field the restore refuses stops the replay");
    g_outNames[0] = "out.target";
    g_outValues[0] = "FrameTable#?";
    g_outCount = 1;
    Check(!ApplyReachField(&refusedUnit, ROK_TARGETINGSETUNIT, ".target"), "and so does a reached field it refuses");
    g_outValues[0] = "null";
    Check(ApplyReachField(&refusedUnit, ROK_TARGETINGSETUNIT, ".target"), "while null is restored");
    g_outCount = 0;

    printf("arrays of structs\n");
    Point16 points[3] = { { 1, 2 }, { 3, 4 }, { -5, 6 } };
    t.Clear();
    ReplayAddStructs(t, "a", "points", ROK_POINT16, points, 3, g_objects);
    Check(strstr(t.Data(), "  a points[0].x 1\n  a points[0].y 2\n  a points[1].x 3\n") == t.Data() && strstr(t.Data(), "  a points[2].y 6\n") != nullptr, "an array of structs is a line per field of each element");
    LoadCall(t.Data());
    StructsHolder holder;
    holder.Parse("points", ROK_POINT16, 3);
    Point16* back = (Point16*)holder.data;
    Check(back != nullptr && back[0].x == 1 && back[1].y == 4 && back[2].x == -5 && back[2].y == 6, "and replays to the same elements");
    t.Clear();
    ReplayAddStructs(t, "a", "points", ROK_POINT16, nullptr, 0, g_objects);
    Check(strcmp(t.Data(), "  a points null\n") == 0, "a null array is null");
    LoadCall(t.Data());
    StructsHolder none;
    none.Parse("points", ROK_POINT16, 0);
    Check(none.data == nullptr, "and replays to a null pointer");
    t.Clear();
    ReplayAddStructs(t, "a", "points", ROK_POINT16, points, 0, g_objects);
    Check(strcmp(t.Data(), "  a points []\n") == 0, "an empty array is []");
    LoadCall(t.Data());
    StructsHolder empty;
    empty.Parse("points", ROK_POINT16, 0);
    Check(empty.data != nullptr, "and replays to a pointer to nothing");
    LoadCall("  a points[0].x 1\n  a points[0].y 2\n  a points[1].x 3\n  a points[1].y 5\n  a points[2].x -5\n  a points[2].y 6\n");
    StructsHolder changed;
    changed.Parse("points", ROK_POINT16, 3);
    BeginCompare();
    for (int e = 0; e < 3; e++) {
        for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
            const ReplayObjectField& f = kReplayObjectFields[i];
            if (f.object != ROK_POINT16)
                continue;
            ReplayText want;
            ReplayText have;
            ReplayFormatObjectField(f, &points[e], g_objects, false, want);
            ReplayFormatObjectField(f, changed.data + e * sizeof(Point16), g_objects, false, have);
            if (strcmp(want.Data(), have.Data()) != 0) {
                char key[48];
                sprintf(key, "points[%d].%s", e, f.name);
                Diff(key, want.Data(), have.Data());
            }
            want.Free();
            have.Free();
        }
    }
    printf("a changed element: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "points[1].y") == 0, "a changed element is reported at its index and field");

    printf("by-value structs\n");
    Angles16 angles = { -3, 40000, 7 };
    t.Clear();
    AddValueStruct(t, ROK_ANGLES16, &angles);
    Check(strcmp(t.Data(), "(-3,40000,7)") == 0, "a struct by value is its fields in order");
    Angles16 parsed = ParseValueText<Angles16>(ROK_ANGLES16, t.Data());
    Check(parsed.bank == -3 && parsed.heading == 40000 && parsed.pitch == 7, "and parses back");
    MissionType mission(5);
    t.Clear();
    AddValueStruct(t, ROK_MISSIONTYPE, &mission);
    Check(strcmp(t.Data(), "(5)") == 0, "a one-field struct keeps its parentheses");
    MissionType again = ParseValueText<MissionType>(ROK_MISSIONTYPE, "(5)");
    Check(again.index == 5, "and parses back");
    Check(!ReplayParseValue(ROK_ANGLES16, "(1,2)", &parsed, g_objects, nullptr) && !ReplayParseValue(ROK_ANGLES16, "1,2,3", &parsed, g_objects, nullptr), "a malformed value is refused");
    LoadCall("  a kind (5)\n  o result (-3,40000,7)\n");
    MissionType arg;
    ParseValueArg("kind", ROK_MISSIONTYPE, &arg);
    Check(arg.index == 5, "an argument by value replays");
    BeginCompare();
    CompareValueResult(ROK_ANGLES16, &angles);
    Check(g_diffs == 0, "a result by value that is the same compares equal");
    angles.heading = 40001;
    CompareValueResult(ROK_ANGLES16, &angles);
    printf("a changed result: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "result") == 0, "a changed result by value is reported at the result");
    t.Free();
}

class FixtureScan : public UnitScanVisitor {
public:
    FixtureScan() : seen(0), sum(0), invert(false) {}
    virtual int VisitUnit(Unit* unit)
    {
        seen++;
        sum += unit->health;
        return (unit->health > 50) != invert ? 1 : 0;
    }
    int seen;
    int sum;
    bool invert;
};

class FixtureNested : public UnitScanVisitor {
public:
    FixtureNested() : inner(0) {}
    virtual int VisitUnit(Unit* unit)
    {
        if (inner++ == 0)
            VisitUnit(unit);
        return 0;
    }
    int inner;
};

class FixtureComplete : public Callback {
public:
    FixtureComplete() : got(-1) {}
    virtual void Complete(int result) { got = result; }
    int got;
};

void FakeScan(UnitScanVisitor* visitor, Unit* a, Unit* b)
{
    visitor->VisitUnit(a);
    visitor->VisitUnit(b);
}

void BeginImport()
{
    g_cursor = g_callFirst + 1;
    g_importIndex = 0;
    g_list.valid = false;
    ReplayHandles().Reset();
    BeginCompare();
}

void EnemyListSelfTest()
{
    printf("a unit list answered from the recorded vector form\n");
    const char* call = "  i los_radar.GetVisibleEnemiesInRadius 1 pos=(100,0,200) 300 0\n  o out [Unit#3,Unit#9,Unit#5]\n";
    Vec3 pos;
    pos.x = 100;
    pos.y = 0;
    pos.z = 200;
    Ref<Unit> wide[5];
    Ref<Unit> narrow[2];
    ResetBlock();
    LoadCall(call);
    BeginImport();
    memset(wide, 0, sizeof wide);
    int count = Imp_los_radar_GetVisibleEnemiesInRadius(1, &pos, 300, 0, wide, 5);
    Check(g_diffs == 0 && count == 3 && wide[0].get() != nullptr && wide[1].get() != nullptr && wide[2].get() != nullptr && wide[3].get() == nullptr, "a capacity at or above the count gets every unit in one call");
    Check(wide[0].get() != wide[1].get() && wide[1].get() != wide[2].get(), "in the recorded order, one unit each");

    ResetBlock();
    LoadCall(call);
    BeginImport();
    memset(narrow, 0, sizeof narrow);
    memset(wide, 0, sizeof wide);
    count = Imp_los_radar_GetVisibleEnemiesInRadius(1, &pos, 300, 0, narrow, 2);
    Check(g_diffs == 0 && count == 3 && narrow[0].get() != nullptr && narrow[1].get() != nullptr, "a capacity below the count gets the first units and the full count");
    int again = Imp_los_radar_GetVisibleEnemiesInRadius(1, &pos, 300, 0, wide, 5);
    Check(g_diffs == 0 && again == 3 && g_importIndex == 1, "a repeat with a larger buffer is answered from the same line");
    Check(wide[0].get() == narrow[0].get() && wide[1].get() == narrow[1].get() && wide[2].get() != nullptr && wide[3].get() == nullptr, "with the same units in the same order");
    again = Imp_los_radar_GetVisibleEnemiesInRadius(1, &pos, 300, 0, wide, 5);
    printf("a third call: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 1 los_radar.GetVisibleEnemiesInRadius") == 0, "a third call is reported as an extra import");

    ResetBlock();
    LoadCall(call);
    BeginImport();
    Imp_los_radar_GetVisibleEnemiesInRadius(1, &pos, 300, 0, narrow, 2);
    Imp_los_radar_GetVisibleEnemiesInRadius(1, &pos, 301, 0, wide, 5);
    printf("a repeat with another radius: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 1 los_radar.GetVisibleEnemiesInRadius") == 0, "a repeat with other arguments is reported as an extra import");

    printf("a recorded slot whose count is above its capacity\n");
    ResetBlock();
    LoadCall(call);
    BeginImport();
    memset(narrow, 0, sizeof narrow);
    Imp_los_radar_GetVisibleEnemiesInRadius(1, &pos, 300, 0, narrow, 2);
    LoadCall("  a capacity 2\n  o out [Unit#3,Unit#9]\n  o result 3\n");
    BeginCompare();
    CompareUnitBuffer("out", narrow, 3, 2);
    Check(g_diffs == 0, "the buffer holds the first capacity units of a larger count, as the line has them");
    BeginCompare();
    CompareUnitBuffer("out", narrow, 1, 2);
    Check(g_diffs == 1, "a count below the units of the line is a difference");
    LoadCall("  a capacity 2\n  o out [Unit#3,Unit#9,Unit#5]\n  o result 3\n");
    BeginCompare();
    CompareUnitBuffer("out", narrow, 3, 2);
    Check(g_diffs == 1, "a line with more units than the capacity is a difference");
}

void ShapeSelfTest()
{
    printf("an out vector of value structs\n");
    ResetBlock();
    ReplayText t;
    std::vector<FixtureSpan, std::allocator<FixtureSpan> > spans;
    FixtureSpan first = { 3, 4 };
    FixtureSpan second = { -5, 6 };
    spans.push_back(first);
    spans.push_back(second);
    ReplayAddValueList(t, ROK_FIXTURESPAN, &spans[0], (unsigned)spans.size(), g_objects);
    Check(strcmp(t.Data(), "[(3,4),(-5,6)]") == 0, "a vector of values is a list of values in parentheses");
    t.Clear();
    ReplayAddValueList(t, ROK_FIXTURESPAN, nullptr, 0, g_objects);
    Check(strcmp(t.Data(), "[]") == 0, "an empty vector is []");
    LoadCall("  i fixture.FillGoals 7\n  o goals [(3,4),(-5,6)]\n");
    BeginImport();
    std::vector<FixtureSpan, std::allocator<FixtureSpan> > goals;
    Imp_fixture_FillGoals(7, &goals);
    Check(g_diffs == 0 && goals.size() == 2 && goals[0].from == 3 && goals[0].to == 4 && goals[1].from == -5 && goals[1].to == 6, "the stub pushes the recorded elements");
    LoadCall("  i fixture.FillGoals 7\n  o goals []\n");
    BeginImport();
    std::vector<FixtureSpan, std::allocator<FixtureSpan> > none;
    Imp_fixture_FillGoals(7, &none);
    Check(g_diffs == 0 && none.empty(), "and none for an empty list");
    LoadCall("  i fixture.FillGoals 7\n  o goals [(3,4),(-5,6)]\n");
    BeginImport();
    std::vector<FixtureSpan, std::allocator<FixtureSpan> > other;
    Imp_fixture_FillGoals(8, &other);
    printf("a changed argument: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 fixture.FillGoals") == 0, "an import with another argument is reported");

    printf("a void* argument with a cast\n");
    LoadCall("  i fixture.PlaceAt pos=(1,2,3) = 5\n");
    BeginImport();
    Vec3 place = { 1, 2, 3 };
    int placed = Imp_fixture_PlaceAt(&place);
    Check(g_diffs == 0 && placed == 5, "the stub reads the Vec3 behind the void* and returns the recorded result");
    LoadCall("  i fixture.PlaceAt pos=(1,2,3) = 5\n");
    BeginImport();
    Vec3 moved = { 1, 2, 4 };
    Imp_fixture_PlaceAt(&moved);
    printf("a changed vector: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 fixture.PlaceAt") == 0, "a different Vec3 behind the pointer is reported");

    printf("an array sized by a state field\n");
    ResetBlock();
    Player* view = (Player*)SimAlloc(sizeof(Player));
    memset((void*)view, 0, sizeof(Player));
    view->unitCount = 3;
    g_game->slowest = view;
    int ids[4] = { 1, 2, 3, 4 };
    LoadCall("  i fixture.ReadIds x:010000000200000003000000 = 1\n");
    BeginImport();
    int read = Imp_fixture_ReadIds(ids);
    Check(g_diffs == 0 && read == 1, "the stub writes as many elements as the field says");
    view->unitCount = 2;
    LoadCall("  i fixture.ReadIds x:010000000200000003000000 = 1\n");
    BeginImport();
    Imp_fixture_ReadIds(ids);
    printf("a changed length: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 fixture.ReadIds") == 0, "a length that follows the field is reported when the field differs");
    g_game->slowest = (Player*)nullptr;
    LoadCall("  i fixture.ReadIds x: = 1\n");
    BeginImport();
    Imp_fixture_ReadIds(ids);
    Check(g_diffs == 0, "a null ref is a length of 0");
    g_game->slowest = view;
    LoadCall("  i fixture.FillIds ? = 8\n  o ids x:0a0000000b000000\n");
    BeginImport();
    int filled[4] = { 0, 0, 0, 0 };
    Imp_fixture_FillIds(filled);
    Check(g_diffs == 0 && filled[0] == 10 && filled[1] == 11 && filled[2] == 0, "the stub fills the buffer by the same length");

    printf("a nullable in/out value\n");
    FixtureAmount amount = { 5 };
    t.Clear();
    AddInOut(t, ROK_FIXTUREAMOUNT, &amount);
    Check(strcmp(t.Data(), "(5)") == 0, "a value going in is its fields in parentheses");
    t.Clear();
    AddInOut(t, ROK_FIXTUREAMOUNT, nullptr);
    Check(strcmp(t.Data(), "null") == 0, "and a null pointer is null");
    LoadCall("  i fixture.Query (5) null = 9\n  o first (8)\n");
    BeginImport();
    FixtureAmount in = { 5 };
    int queried = Imp_fixture_Query(&in, nullptr);
    Check(g_diffs == 0 && queried == 9 && in.amount == 8, "the stub passes the value in and writes the o line back");
    LoadCall("  i fixture.Query (5) (6) = 0\n  o first (8)\n  o second (9)\n");
    BeginImport();
    FixtureAmount left = { 5 };
    FixtureAmount right = { 6 };
    Imp_fixture_Query(&left, &right);
    Check(g_diffs == 0 && left.amount == 8 && right.amount == 9, "each pointer takes its own o line");
    LoadCall("  i fixture.Query (5) null = 9\n  o first (8)\n");
    BeginImport();
    FixtureAmount stray = { 5 };
    FixtureAmount extra = { 1 };
    Imp_fixture_Query(&stray, &extra);
    printf("a null that was not recorded: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 fixture.Query") == 0, "a pointer where null was recorded is reported");
    LoadCall("  i fixture.Query (5) null = 9\n  o first (8)\n  o second (3)\n");
    SetClosing("second ");
    BeginImport();
    FixtureAmount shut = { 5 };
    Imp_fixture_Query(&shut, nullptr);
    Check(g_outCount == 1, "an o line named by the closing list is the call's, not the import's");
    SetClosing(nullptr);

    printf("a nullable out value\n");
    LoadCall("  i fixture.Locate = 1\n  o first (4)\n  o second (9)\n");
    BeginImport();
    FixtureAmount found = { 0 };
    FixtureAmount foundSize = { 0 };
    int located = Imp_fixture_Locate(&found, &foundSize);
    Check(g_diffs == 0 && located == 1 && found.amount == 4 && foundSize.amount == 9, "the stub writes each o line back and the line holds no value going in");
    LoadCall("  i fixture.Locate = 1\n  o first (4)\n  o second (9)\n");
    BeginImport();
    FixtureAmount partial = { 0 };
    Imp_fixture_Locate(&partial, nullptr);
    Check(g_diffs == 0 && partial.amount == 4, "a null pointer is skipped");
    LoadCall("  i fixture.Locate = 0\n");
    BeginImport();
    FixtureAmount alone = { 7 };
    Imp_fixture_Locate(&alone, nullptr);
    Check(g_diffs == 0 && alone.amount == 7, "and a pointer with no o line is left alone");

    printf("in/out of the slot under replay\n");
    LoadCall("  a p2 (5)\n  o p2 (8)\n");
    FixtureAmount storage = { 0 };
    FixtureAmount* passed = (FixtureAmount*)ParseInOut("p2", ROK_FIXTUREAMOUNT, &storage);
    Check(passed == &storage && storage.amount == 5, "the value goes to the slot by address");
    storage.amount = 8;
    BeginCompare();
    CompareInOut("p2", ROK_FIXTUREAMOUNT, passed);
    Check(g_diffs == 0, "and the value it leaves is compared with the o line");
    storage.amount = 9;
    CompareInOut("p2", ROK_FIXTUREAMOUNT, passed);
    printf("a changed value: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "p2") == 0, "a different value is reported at the argument");
    LoadCall("  a p2 null\n");
    Check(ParseInOut("p2", ROK_FIXTUREAMOUNT, &storage) == nullptr, "null goes to the slot as a null pointer");
    BeginCompare();
    CompareInOut("p2", ROK_FIXTUREAMOUNT, nullptr);
    Check(g_diffs == 0, "and has no o line to compare");
    LoadCall("  a p2 null\n  o p2 (1)\n");
    BeginCompare();
    CompareInOut("p2", ROK_FIXTUREAMOUNT, nullptr);
    Check(g_diffs == 1, "an o line for a null pointer is a difference");

    printf("a scalar in/out value\n");
    int count = 5;
    short level = -3;
    unsigned char flag = 7;
    t.Clear();
    AddInOutScalar(t, &count);
    Check(strcmp(t.Data(), "(5)") == 0, "an int going in is its value in parentheses");
    t.Clear();
    AddInOutScalar(t, &level);
    Check(strcmp(t.Data(), "(-3)") == 0, "and a short is signed");
    t.Clear();
    AddInOutScalar(t, &flag);
    Check(strcmp(t.Data(), "(7)") == 0, "and an unsigned char is not a character");
    t.Clear();
    AddInOutScalar(t, (int*)nullptr);
    Check(strcmp(t.Data(), "null") == 0, "and a null pointer is null");
    LoadCall("  i fixture.Measure (5) null (7) = 1\n  o p2 (9)\n  o p4 (250)\n");
    BeginImport();
    int measuredCount = 5;
    unsigned char measuredFlag = 7;
    int measured = Imp_fixture_Measure(&measuredCount, nullptr, &measuredFlag);
    Check(g_diffs == 0 && measured == 1 && measuredCount == 9 && measuredFlag == 250, "the stub passes the values in and writes each o line back through its pointer");
    LoadCall("  i fixture.Measure (5) null (7) = 1\n  o p2 (9)\n");
    BeginImport();
    int untouchedCount = 5;
    short strayLevel = -3;
    unsigned char untouchedFlag = 7;
    Imp_fixture_Measure(&untouchedCount, &strayLevel, &untouchedFlag);
    printf("a pointer where null was recorded: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 fixture.Measure") == 0 && untouchedFlag == 7, "is reported, and a pointer with no o line is left alone");
    LoadCall("  a p2 (5)\n  a p3 null\n  a p4 (7)\n  o p2 (9)\n  o p4 (8)\n");
    int slotCount = 0;
    int* slotCountPtr = ParseInOutScalar("p2", &slotCount);
    short slotLevel = 0;
    short* slotLevelPtr = ParseInOutScalar("p3", &slotLevel);
    unsigned char slotFlag = 0;
    unsigned char* slotFlagPtr = ParseInOutScalar("p4", &slotFlag);
    Check(slotCountPtr == &slotCount && slotCount == 5 && slotLevelPtr == nullptr && slotFlagPtr == &slotFlag && slotFlag == 7, "the replayer passes each value by address, and null as a null pointer");
    slotCount = 9;
    slotFlag = 8;
    BeginCompare();
    CompareInOutScalar("p2", slotCountPtr);
    CompareInOutScalar("p3", slotLevelPtr);
    CompareInOutScalar("p4", slotFlagPtr);
    Check(g_diffs == 0, "and the values the slot leaves are compared with the o lines");
    slotFlag = 9;
    CompareInOutScalar("p4", slotFlagPtr);
    printf("a changed scalar: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "p4") == 0, "a different value is reported at the argument");
    t.Free();
}

void InOutRunnerSelfTest()
{
    printf("in/out arguments through the runner\n");
    ResetBlock();
    LoadCall("  a first (5)\n  a second (6)\n  o first (8)\n  o second (0)\n  o result 8\n");
    BeginCompare();
    Run_fixture_Query();
    After_fixture_Query();
    Check(g_diffs == 0, "a struct in/out recording replays equal");
    LoadCall("  a first (5)\n  a second null\n  o first (8)\n  o result 8\n");
    BeginCompare();
    Run_fixture_Query();
    After_fixture_Query();
    Check(g_diffs == 0, "and so does one with a null pointer, which has no o line");
    LoadCall("  a first (5)\n  a second null\n  o first (9)\n  o result 8\n");
    BeginCompare();
    Run_fixture_Query();
    After_fixture_Query();
    printf("a struct that came out differently: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "first") == 0, "a replayed value that differs is reported at the argument");
    LoadCall("  a first (5)\n  a second null\n  o first (8)\n  o second (0)\n  o result 8\n");
    BeginCompare();
    Run_fixture_Query();
    After_fixture_Query();
    Check(g_diffs == 1 && strcmp(g_firstKey, "second") == 0, "an o line for a pointer that was null is a difference");
    LoadCall("  a first (5)\n  a second null\n  o first (8)\n  o result 7\n");
    BeginCompare();
    Run_fixture_Query();
    After_fixture_Query();
    Check(g_diffs == 1 && strcmp(g_firstKey, "result") == 0, "and the result is compared after the values");

    LoadCall("  a p2 (5)\n  a p3 null\n  a p4 (7)\n  o p2 (10)\n  o p4 (8)\n  o result 1\n");
    BeginCompare();
    Run_fixture_Measure();
    After_fixture_Measure();
    Check(g_diffs == 0, "a scalar in/out recording replays equal");
    LoadCall("  a p2 (5)\n  a p3 (-3)\n  a p4 (7)\n  o p2 (10)\n  o p3 (-4)\n  o p4 (8)\n  o result 1\n");
    BeginCompare();
    Run_fixture_Measure();
    After_fixture_Measure();
    Check(g_diffs == 0, "with every pointer set too");
    LoadCall("  a p2 (5)\n  a p3 null\n  a p4 (7)\n  o p2 (10)\n  o p4 (9)\n  o result 1\n");
    BeginCompare();
    Run_fixture_Measure();
    After_fixture_Measure();
    printf("a scalar that came out differently: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "p4") == 0, "a replayed scalar that differs is reported at the argument");

    printf("a weapon as the result of a slot\n");
    LoadCall("  o result weapondef#7\n  o weapondef#7.flags 0\n");
    BeginCompare();
    Run_fixture_Pick();
    After_fixture_Pick();
    Check(g_diffs == 0, "a weapon result is compared by its name");
    LoadCall("  o result weapondef#8\n");
    BeginCompare();
    Run_fixture_Pick();
    After_fixture_Pick();
    printf("another weapon: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "result") == 0, "another weapon is reported at the result");

    printf("a nullable out value of the slot under replay\n");
    LoadCall("  a second null\n  o first.amount 4\n  o result 1\n");
    BeginCompare();
    Run_fixture_Locate();
    After_fixture_Locate();
    Check(g_diffs == 0, "a null out pointer is passed as null, and has no o line");
    LoadCall("  o first.amount 4\n  o second.amount 9\n  o result 1\n");
    BeginCompare();
    Run_fixture_Locate();
    After_fixture_Locate();
    Check(g_diffs == 0, "and a pointer that is not null is compared with its o lines");
    LoadCall("  a first null\n  a second null\n  o result 0\n");
    BeginCompare();
    Run_fixture_Locate();
    After_fixture_Locate();
    Check(g_diffs == 0, "a call whose result leaves the out values alone records none");
    LoadCall("  o first.amount 5\n  o second.amount 9\n  o result 1\n");
    BeginCompare();
    Run_fixture_Locate();
    After_fixture_Locate();
    printf("an out struct that came out differently: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "first.amount") == 0, "a replayed value that differs is reported at its field");
}

void WeaponArgumentSelfTest()
{
    printf("a weapon as an argument\n");
    unsigned short field[1] = { 0 };
    bool found = false;
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        if (kReplayObjectFields[i].object == ROK_WEAPONDEF && strcmp(kReplayObjectFields[i].name, "speed") == 0) {
            field[0] = (unsigned short)i;
            found = true;
        }
    }
    Check(found, "the weapon definition has a speed field");
    if (!found)
        return;
    ReplaySlotSets sets = { "Strike", 0, nullptr, 0, nullptr, 0, field, 1, field, 1 };
    const char* calls[3] = { "  r weapondef#3.speed 7\n  a weapon weapondef#3\n  w weapondef#3.speed 8\n  o result 8\n",
                             "  r weapondef#3.speed 7\n  a weapon weapondef#3\n  w weapondef#3.speed 9\n  o result 8\n",
                             "  a weapon null\n  o result -1\n" };
    for (int n = 0; n < 3; n++) {
        ResetBlock();
        LoadCall(calls[n]);
        for (int k = 1; k < g_lineCount; k++) {
            char* space = strchr(g_lines[k] + 4, ' ');
            if (strncmp(g_lines[k], "  r ", 4) == 0 && space != nullptr) {
                *space = '\0';
                RestoreObjectLine(g_lines[k] + 4, space + 1, "self-test", &sets, g_lines[k]);
                *space = ' ';
            }
        }
        g_rootCount = 0;
        BeginCompare();
        Run_fixture_Strike();
        CompareWrites("self-test", &sets);
        After_fixture_Strike();
        if (n == 0)
            Check(g_diffs == 0 && g_rootCount == 1, "a weapon with its recorded field replays equal, as a root of the slot");
        else if (n == 1)
            Check(g_diffs == 1 && strcmp(g_firstKey, "weapondef#3.speed") == 0, "a weapon field that came out differently is reported at that field");
        else
            Check(g_diffs == 0 && g_rootCount == 0, "a null weapon stays null");
    }
}

void CallbackSelfTest()
{
    printf("callbacks\n");
    ResetBlock();
    ReplayClearFailure();
    Unit* pool = (Unit*)SimAlloc(16 * (unsigned)sizeof(Unit));
    memset(pool, 0, 16 * sizeof(Unit));
    g_game->units = pool;
    g_game->poolCount = 16;
    pool[3].health = 77;
    pool[9].health = 5;

    FixtureScan module;
    const void* before = *(const void* const*)&module;
    ReplayObjectTable* table = new ReplayObjectTable;
    ReplayCallbackScope scope;
    ReplayOrdinals ordinals;
    ordinals.Reset();
    ReplayText line;
    line.Add("  i fixture.Scan");
    unsigned kind = OBJ_CALLBACK + RCB_UNITSCANVISITOR;
    line.Add(" scanner#");
    line.AddUnsigned(ordinals.Of(kind, &module));
    scope.Start(RCB_UNITSCANVISITOR, &module, 1, *table, ReplayThunkVtable(RCB_UNITSCANVISITOR));
    Check(*(const void* const*)&module != before, "while the import runs, the object has the vtable of the recorder");
    FakeScan(&module, &pool[3], &pool[9]);
    scope.Stop();
    Check(*(const void* const*)&module == before, "and its own vtable after it");
    Check(module.seen == 2 && module.sum == 82, "the object's own method ran for each call");
    line.Add(" = 0");
    line.AddText(scope.Lines());
    line.AddChar('\n');
    printf("%s", line.Data());
    Check(strcmp(line.Data(), "  i fixture.Scan scanner#1 = 0\n    c scanner#1.VisitUnit Unit#3 = 1\n    c scanner#1.VisitUnit Unit#9 = 0\n") == 0, "each call is a c line under the i line, in call order, with its arguments and result");
    Check(ReplayFailure() == nullptr, "and none is a failure");

    FixtureScan after;
    FakeScan(&after, &pool[3], &pool[9]);
    Check(scope.Lines().Size() > 0 && after.seen == 2, "a call after the import is not recorded");

    printf("replayed callbacks\n");
    char body[512];
    sprintf(body, "%s", line.Data());
    LoadCall(body);
    BeginImport();
    FixtureScan player;
    ReplayText t;
    StartImport(t, "fixture.Scan");
    Space(t);
    AddObject(t, kind, &player);
    const char* r = Enter(t);
    Check(strcmp(r, "0") == 0 && g_cbCount == 2, "the stub finds the recorded calls under the import");
    PlayCallbacks_UnitScanVisitor(&player, kind, "fixture.Scan");
    Check(g_diffs == 0, "the recorded calls replay equal");
    Check(player.seen == 2 && player.sum == 82, "on the module's own object, with the recorded arguments");

    LoadCall(body);
    BeginImport();
    FixtureScan inverted;
    inverted.invert = true;
    StartImport(t, "fixture.Scan");
    Space(t);
    AddObject(t, kind, &inverted);
    Enter(t);
    PlayCallbacks_UnitScanVisitor(&inverted, kind, "fixture.Scan");
    printf("a changed result: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 2 && strcmp(g_firstKey, "import 0 fixture.Scan.c[0]") == 0, "a result that differs is reported at the call under the import");
    Check(strstr(g_detail.Data(), "c[1]: recorded 0, replayed 1") != nullptr, "with both values, for each call");

    LoadCall("  i fixture.Scan scanner#1 = 0\n    c scanner#2.VisitUnit Unit#3 = 1\n");
    BeginImport();
    FixtureScan other;
    StartImport(t, "fixture.Scan");
    Space(t);
    AddObject(t, kind, &other);
    Enter(t);
    PlayCallbacks_UnitScanVisitor(&other, kind, "fixture.Scan");
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 fixture.Scan.c[0]") == 0 && other.seen == 0, "a call on another object than the recorded one is reported and not made");

    LoadCall("  i fixture.Scan scanner#1 = 0\n    c scanner#1.VisitUnit Unit#3\n  o x 1\n");
    BeginImport();
    FixtureScan missing;
    StartImport(t, "fixture.Scan");
    Space(t);
    AddObject(t, kind, &missing);
    Enter(t);
    Check(g_cbCount == 1 && g_outCount == 1, "the c lines come before the o lines of the import");
    PlayCallbacks_UnitScanVisitor(&missing, kind, "fixture.Scan");
    Check(g_diffs == 1 && strstr(g_detail.Data(), "recorded no result, replayed 1") != nullptr, "a call recorded without a result differs from one that returns one");

    printf("void callbacks with a scalar\n");
    FixtureComplete done;
    ReplayCallbackScope doneScope;
    ReplayText doneLine;
    doneScope.Start(RCB_CALLBACK, &done, 1, *table, ReplayThunkVtable(RCB_CALLBACK));
    ((Callback*)&done)->Complete(7);
    doneScope.Stop();
    doneLine.AddText(doneScope.Lines());
    Check(strcmp(doneLine.Data(), "\n    c callback#1.Complete 7") == 0, "a call with a scalar argument and no result");
    Check(done.got == 7, "reaches the object");
    LoadCall("  i fixture.Complete callback#1\n    c callback#1.Complete 9\n");
    BeginImport();
    FixtureComplete played;
    StartImport(t, "fixture.Complete");
    Space(t);
    AddObject(t, OBJ_CALLBACK + RCB_CALLBACK, &played);
    Enter(t);
    PlayCallbacks_Callback(&played, OBJ_CALLBACK + RCB_CALLBACK, "fixture.Complete");
    Check(g_diffs == 0 && played.got == 9, "and replays with the recorded scalar");

    printf("a callback inside a callback\n");
    FixtureNested nested;
    ReplayCallbackScope nestedScope;
    nestedScope.Start(RCB_UNITSCANVISITOR, &nested, 1, *table, ReplayThunkVtable(RCB_UNITSCANVISITOR));
    ((UnitScanVisitor*)&nested)->VisitUnit(&pool[3]);
    nestedScope.Stop();
    printf("%s\n", ReplayFailure() == nullptr ? "no failure" : ReplayFailure());
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "inside a callback") != nullptr, "is a failure, because its line could not be in call order");
    ReplayClearFailure();
    line.Free();
    doneLine.Free();
    t.Free();
    delete table;
}

void RestoreCallLines()
{
    for (int k = g_callFirst + 1; k < g_callEnd; k++) {
        char* line = g_lines[k];
        if (strncmp(line, "  r ", 4) != 0)
            continue;
        char* space = strchr(line + 4, ' ');
        *space = '\0';
        Check(RestoreObjectLine(line + 4, space + 1, "self-test", nullptr, line), "a recorded line restores");
        *space = ' ';
    }
}

void RunNotify(const char* body)
{
    LoadCall(body);
    BeginCompare();
    BeginHeld();
    RestoreCallLines();
    Run_fixture_Notify();
    After_fixture_Notify();
    CompareHeld();
}

const char* const kNotifyHeld = "  r FixtureNotify#1.id 7\n  r FixtureNotify#1.hook callback#1\n  r FixtureNotify#1.link callback#2\n  a holder FixtureNotify#1\n  a done callback#3\n  a amount 5\n";

void HeldCallbackSelfTest()
{
    printf("callbacks held in state\n");
    ResetBlock();
    ReplayClearFailure();
    char body[1024];
    sprintf(body, "%s  c callback#1.Complete 5\n  c callback#3.Complete 10\n  c callback#2.Complete 15\n  o result 5\n", kNotifyHeld);
    RunNotify(body);
    Check(g_diffs == 0, "the calls a module makes on a held ptr, a held ref and an argument replay equal");
    Check(g_heldCount == 3 && g_held[0].count == 1 && g_held[1].count == 1 && g_held[2].count == 1, "each callback object was bound at its line and got its call");
    FixtureNotify* holder = (FixtureNotify*)ReplayResolveObject("FixtureNotify#1", nullptr, g_objects, AllocObject);
    Check(holder != nullptr && holder->hook == g_held[0].object && holder->link.get() == g_held[1].object, "the fields hold the objects of the lines");
    Check(ReplayHandles().Find(REPLAY_HANDLE_CALLBACK + RCB_CALLBACK, g_held[2].object) == 3, "and the argument has the number the recording gave it");
    ReplayText value;
    ReplayFormatObjectField(*ReplayFindObjectField(ROK_FIXTURENOTIFY, "hook"), holder, g_objects, false, value);
    Check(strcmp(value.Data(), "callback#1") == 0, "a field that holds a callback is written as its handle");
    ReplayFormatObjectField(*ReplayFindObjectField(ROK_FIXTURENOTIFY, "link"), holder, g_objects, false, value);
    Check(strcmp(value.Data(), "callback#2") == 0, "a ref as well");
    value.Free();

    printf("a different call\n");
    sprintf(body, "%s  c callback#1.Complete 5\n  c callback#3.Complete 11\n  c callback#2.Complete 15\n  o result 5\n", kNotifyHeld);
    RunNotify(body);
    printf("%s: %s\n%s", "a different argument", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "callback#3.Complete") == 0, "is reported at the handle and the method");
    Check(strstr(g_detail.Data(), "recorded Complete 11, replayed Complete 10") != nullptr, "with both calls");

    printf("a missing call\n");
    sprintf(body, "%s  c callback#1.Complete 5\n  c callback#1.Complete 5\n  c callback#3.Complete 10\n  c callback#2.Complete 15\n  o result 5\n", kNotifyHeld);
    RunNotify(body);
    printf("%s: %s\n%s", "a missing call", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "callback#1.Complete") == 0 && strstr(g_detail.Data(), "replayed no call (missing)") != nullptr, "is reported at the handle, as a call that the module did not make");

    printf("an extra call\n");
    sprintf(body, "%s  c callback#1.Complete 5\n  c callback#2.Complete 15\n  o result 5\n", kNotifyHeld);
    RunNotify(body);
    printf("%s: %s\n%s", "an extra call", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "callback#3.Complete") == 0 && strstr(g_detail.Data(), "recorded no call (extra), replayed Complete 10") != nullptr, "is reported at the handle, with the call the recording does not have");

    printf("a call on another handle\n");
    sprintf(body, "%s  c callback#2.Complete 5\n  c callback#3.Complete 10\n  c callback#2.Complete 15\n  o result 5\n", kNotifyHeld);
    RunNotify(body);
    printf("%s: %s\n%s", "another handle", g_firstKey, g_detail.Data());
    Check(g_diffs == 3 && strcmp(g_firstKey, "callback#1.Complete") == 0, "is reported on the handles it concerns, the first in the order they were bound");

    printf("null fields and a null argument\n");
    RunNotify("  r FixtureNotify#1.hook null\n  r FixtureNotify#1.link null\n  a holder FixtureNotify#1\n  a done null\n  a amount 5\n  o result 5\n");
    Check(g_diffs == 0 && g_heldCount == 0, "bind nothing and replay equal");

    printf("a result of a held callback\n");
    Unit* pool = (Unit*)SimAlloc(16 * (unsigned)sizeof(Unit));
    memset(pool, 0, 16 * sizeof(Unit));
    g_game->units = pool;
    g_game->poolCount = 16;
    LoadCall("  c scanner#1.VisitUnit Unit#3 = 1\n  c scanner#1.VisitUnit Unit#9 = 0\n");
    BeginCompare();
    BeginHeld();
    Check(ReplayResolveObject("Unit#3", nullptr, g_objects, AllocObject) == &pool[3], "the units are named as in a recording");
    UnitScanVisitor* scanner = (UnitScanVisitor*)BindHeld(RCB_UNITSCANVISITOR, 1, AllocObject);
    int first = scanner->VisitUnit(&pool[3]);
    int second = scanner->VisitUnit(&pool[9]);
    CompareHeld();
    Check(g_diffs == 0 && first == 1 && second == 0, "is the recorded result, and the calls replay equal");
    scanner->VisitUnit(&pool[3]);
    CompareHeld();
    Check(g_diffs == 1 && strcmp(g_firstKey, "scanner#1.VisitUnit") == 0, "and a call past the recorded ones is extra");

    printf("the recorder's side\n");
    ReplayObjectTable* table = new ReplayObjectTable;
    FixtureComplete module;
    ReplayCallbackScope scope;
    scope.Start(RCB_CALLBACK, &module, 1, *table, ReplayThunkVtable(RCB_CALLBACK), 2);
    ((Callback*)&module)->Complete(7);
    scope.Stop();
    Check(strcmp(scope.Lines().Data(), "\n  c callback#1.Complete 7") == 0 && module.got == 7, "a call made during a slot is a c line indented as the slot's lines");
    BeginHeld();
    FixtureNotify recorded;
    memset(&recorded, 0, sizeof recorded);
    recorded.hook = &module;
    ReplayText field;
    ReplayFormatObjectField(*ReplayFindObjectField(ROK_FIXTURENOTIFY, "hook"), &recorded, *table, false, field);
    Check(strcmp(field.Data(), "callback#?") == 0, "an object no line named has no number, as any object");
    ReplayFormatObjectField(*ReplayFindObjectField(ROK_FIXTURENOTIFY, "hook"), &recorded, *table, true, field);
    Check(strcmp(field.Data(), "callback#1") == 0, "and the first line that names it numbers it");
    field.Free();
    delete table;
    ResetHeld();
}

void VectorSelfTest()
{
    printf("block vectors\n");
    ResetBlock();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    Unit* pool = (Unit*)SimAlloc(16 * (unsigned)sizeof(Unit));
    memset((void*)pool, 0, 16 * sizeof(Unit));
    g_game->units = pool;
    g_game->poolCount = 16;
    pool[3].health = 30;
    pool[9].health = 90;
    PlayerAI* ai = (PlayerAI*)SimAlloc(sizeof(PlayerAI));
    memset((void*)ai, 0, sizeof(PlayerAI));
    ai->index = 4;
    Ref<Unit> first;
    first = &pool[3];
    Ref<Unit> second;
    second = &pool[9];
    ai->cells.PushBack(KeyedCell(3, 4, 1.5f));
    ai->cells.PushBack(KeyedCell(5, 6, 2.5f));
    ai->counts.PushBack(1);
    ai->counts.PushBack(2);
    ai->counts.PushBack(3);
    Check(ai->counts.Size() == 3 && ai->counts.Capacity() == 4, "the block vector grows as the module's does");

    FixtureSeen* seen = (FixtureSeen*)SimAlloc(sizeof(FixtureSeen));
    memset((void*)seen, 0, sizeof(FixtureSeen));
    seen->index = 4;
    seen->units.PushBack(first);
    seen->units.PushBack(second);
    unsigned short kind[2] = { ROK_PLAYERAI, ROK_FIXTURESEEN };
    void* root[2] = { ai, seen };
    Recorder recorder;
    recorder.Record(kind, root, 2, sets);
    ReplayText calls;
    const char* text = recorder.text.Data();
    printf("%s", text);
    Check(strstr(text, "  r FixtureSeen#1.units [2/2] Unit#3,Unit#9\n") != nullptr, "pointer elements are the length, the capacity and the handles");
    Check(strstr(text, "  r Unit#9.health 90\n") != nullptr, "and the walker reaches the units held only by the vector");
    Check(strstr(text, "  r PlayerAI#1.counts [3/4] 0:010002000300\n") != nullptr, "scalar elements are runs, as in an array");
    Check(strstr(text, "  r PlayerAI#1.cells [2/2]\n  r PlayerAI#1.cells[0].pos (3,4)\n  r PlayerAI#1.cells[0].key f32:0x3fc00000\n  r PlayerAI#1.cells[1].pos (5,6)\n") != nullptr, "struct elements are the bracket and a line per field of each element");
    calls.AddText(recorder.text);
    ReplayText firstCall;
    firstCall.AddText(recorder.text);

    ai->counts[1] = 9;
    recorder.Record(kind, root, 2, sets);
    Check(strcmp(recorder.text.Data(), "  r PlayerAI#1.counts 1:0900\n") == 0, "a changed scalar element is a delta run, with no length or capacity");
    calls.AddText(recorder.text);
    ai->counts.PushBack(4);
    recorder.Record(kind, root, 2, sets);
    Check(strcmp(recorder.text.Data(), "  r PlayerAI#1.counts [4/4] 0:01000900030004000\n") != 0 && strstr(recorder.text.Data(), "  r PlayerAI#1.counts [4/4] 0:0100090003000400\n") != nullptr, "a push that grows the length writes the vector again");
    calls.AddText(recorder.text);
    ai->counts.PushBack(5);
    ai->cells.PushBack(KeyedCell(7, 8, 3.5f));
    seen->units.PushBack(first);
    recorder.Record(kind, root, 2, sets);
    Check(strstr(recorder.text.Data(), "  r PlayerAI#1.counts [5/8] 0:01000900030004000500\n") != nullptr, "a push that grows the capacity writes both");
    Check(strstr(recorder.text.Data(), "  r PlayerAI#1.cells [3/4]\n  r PlayerAI#1.cells[2].pos (7,8)\n") != nullptr && strstr(recorder.text.Data(), "cells[0]") == nullptr, "a struct vector that moved writes only the element that is new");
    Check(strstr(recorder.text.Data(), "  r FixtureSeen#1.units [3/4] Unit#3,Unit#9,Unit#3\n") != nullptr, "and a pointer vector is written whole");
    calls.AddText(recorder.text);
    seen->units.Clear();
    ai->counts.Clear();
    recorder.Record(kind, root, 2, sets);
    Check(strstr(recorder.text.Data(), "  r FixtureSeen#1.units [0/4]\n") != nullptr && strstr(recorder.text.Data(), "  r PlayerAI#1.counts [0/8]\n") != nullptr, "a vector with no elements keeps its capacity");
    calls.AddText(recorder.text);

    PlayerAI* bare = (PlayerAI*)SimAlloc(sizeof(PlayerAI));
    memset((void*)bare, 0, sizeof(PlayerAI));
    FixtureSeen* bareSeen = (FixtureSeen*)SimAlloc(sizeof(FixtureSeen));
    memset((void*)bareSeen, 0, sizeof(FixtureSeen));
    void* bareRoot[2] = { bare, bareSeen };
    Recorder empty;
    empty.Record(kind, bareRoot, 2, sets);
    Check(strstr(empty.text.Data(), "  r PlayerAI#1.counts [0/0]\n") != nullptr && strstr(empty.text.Data(), "  r FixtureSeen#1.units [0/0]\n") != nullptr && strstr(empty.text.Data(), "  r PlayerAI#1.cells [0/0]\n") != nullptr, "an empty vector is [0/0]");

    ResetBlock();
    int lines = RestoreText(calls.Data(), sets);
    Check(lines > 10, "the lines of every call restore");
    PlayerAI* rebuilt = (PlayerAI*)ReplayResolveObject("PlayerAI#1", nullptr, g_objects, AllocObject);
    FixtureSeen* rebuiltSeen = (FixtureSeen*)ReplayResolveObject("FixtureSeen#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->index == 4, "the object is rebuilt");
    Check(rebuilt != nullptr && rebuilt->counts.Size() == 0 && rebuilt->counts.Capacity() == 8, "a vector has the length and capacity of the last write");
    Check(rebuilt != nullptr && rebuilt->cells.Size() == 3 && rebuilt->cells.Capacity() == 4, "a struct vector too");
    Check(rebuilt != nullptr && rebuilt->cells[0].pos.x == 3 && rebuilt->cells[0].key == 1.5f && rebuilt->cells[2].pos.y == 8 && rebuilt->cells[2].key == 3.5f, "and the elements of every call, the old ones carried over the move");
    Check(rebuilt != nullptr && rebuiltSeen->units.Size() == 0 && rebuiltSeen->units.Capacity() == 4, "a pointer vector too");

    ResetBlock();
    RestoreText(calls.Data(), sets);
    rebuilt = (PlayerAI*)ReplayResolveObject("PlayerAI#1", nullptr, g_objects, AllocObject);
    Recorder replayed;
    rebuiltSeen = (FixtureSeen*)ReplayResolveObject("FixtureSeen#1", nullptr, g_objects, AllocObject);
    void* rebuiltRoot[2] = { rebuilt, rebuiltSeen };
    replayed.Record(kind, rebuiltRoot, 2, sets);
    Check(rebuilt != nullptr && strstr(replayed.text.Data(), "  r PlayerAI#1.cells [3/4]\n") != nullptr && strstr(replayed.text.Data(), "  r PlayerAI#1.counts [0/8]\n") != nullptr, "the rebuilt vectors record as they were");

    ResetBlock();
    RestoreText(calls.Data(), sets);
    rebuilt = (PlayerAI*)ReplayResolveObject("PlayerAI#1", nullptr, g_objects, AllocObject);
    rebuiltSeen = (FixtureSeen*)ReplayResolveObject("FixtureSeen#1", nullptr, g_objects, AllocObject);
    g_rootKinds[0] = ROK_PLAYERAI;
    g_rootObjects[0] = rebuilt;
    g_rootKinds[1] = ROK_FIXTURESEEN;
    g_rootObjects[1] = rebuiltSeen;
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the unchanged vectors compare equal");
    rebuilt->counts.PushBack(7);
    rebuilt->counts.PushBack(8);
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed length: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "PlayerAI#1.counts.length") == 0 && strstr(g_detail.Data(), "recorded 0, replayed 2") != nullptr, "a longer vector is reported at its length");
    rebuilt->counts.Clear();
    rebuilt->counts.Reserve(16);
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "PlayerAI#1.counts.capacity") == 0, "a vector of another capacity is reported at its capacity");
    rebuilt->cells[1].key = 9.5f;
    BeginCompare();
    CompareObjects(&sets);
    Check(strstr(g_detail.Data(), "PlayerAI#1.cells[1].key") != nullptr, "a changed field of a struct element is reported at its element and field");
    rebuiltSeen->units.PushBack(first);
    BeginCompare();
    CompareObjects(&sets);
    Check(strstr(g_detail.Data(), "FixtureSeen#1.units.length") != nullptr, "a pointer vector that grew is reported");

    ResetBlock();
    RestoreText(firstCall.Data(), sets);
    rebuilt = (PlayerAI*)ReplayResolveObject("PlayerAI#1", nullptr, g_objects, AllocObject);
    rebuiltSeen = (FixtureSeen*)ReplayResolveObject("FixtureSeen#1", nullptr, g_objects, AllocObject);
    g_rootObjects[0] = rebuilt;
    g_rootObjects[1] = rebuiltSeen;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "a vector restored from its first write compares equal");
    rebuilt->counts[1] = 5;
    rebuiltSeen->units[0] = rebuiltSeen->units[1];
    BeginCompare();
    CompareObjects(&sets);
    printf("changed elements: %s\n%s", g_firstKey, g_detail.Data());
    Check(strstr(g_detail.Data(), "PlayerAI#1.counts[1]: recorded 0200, replayed 0500") != nullptr, "a changed scalar element is reported at its index");
    Check(strstr(g_detail.Data(), "FixtureSeen#1.units[0]: recorded Unit#3, replayed Unit#9") != nullptr, "and a changed pointer element");
    firstCall.Free();
    calls.Free();
}

unsigned char g_fakeArena[2048];
unsigned g_fakeUsed;

void* FakeCaptureAlloc(unsigned int bytes, unsigned int* offset)
{
    *offset = g_fakeUsed;
    unsigned char* at = g_fakeArena + g_fakeUsed;
    g_fakeUsed += (bytes + 7u) & ~7u;
    return at;
}

void SnapshotSelfTest()
{
    printf("snapshots\n");
    g_fakeUsed = 0;
    ReplaySnapshots snapshots;
    ReplayText recorded;
    unsigned offset = 0;
    unsigned char* first = (unsigned char*)FakeCaptureAlloc(20, &offset);
    recorded.Add("  i services.SimCaptureAlloc 20 = snapshot#");
    recorded.AddUnsigned(snapshots.Add(first, 20));
    recorded.AddChar('\n');
    unsigned char* second = (unsigned char*)FakeCaptureAlloc(700, &offset);
    recorded.Add("  i services.SimCaptureAlloc 700 = snapshot#");
    recorded.AddUnsigned(snapshots.Add(second, 700));
    recorded.AddChar('\n');
    for (int i = 0; i < 20; i++)
        first[i] = (unsigned char)(i + 1);
    for (int i = 0; i < 700; i++)
        second[i] = (unsigned char)(i * 3);
    snapshots.Emit(recorded);
    printf("%.300s\n", recorded.Data());
    Check(strstr(recorded.Data(), "  i services.SimCaptureAlloc 20 = snapshot#1\n  i services.SimCaptureAlloc 700 = snapshot#2\n  o snapshot#1 x:0102030405060708090a0b0c0d0e0f1011121314\n  o snapshot#2 x:000306") != nullptr, "an import line carries the size and the handle, and each buffer follows as bytes");
    Check(CountLines(recorded.Data(), "  o snapshot#2 ") == 2, "a long buffer is several lines of the same name, in order");
    Check(LongestLine(recorded.Data()) < 1100, "and the lines are bounded");
    Check(strstr(recorded.Data(), "offset") == nullptr, "no line is written for the offset");

    char* body = (char*)malloc(recorded.Size() + 1);
    strcpy(body, recorded.Data());
    LoadCall(body);
    g_cursor = g_callFirst + 1;
    g_importIndex = 0;
    g_list.valid = false;
    ReplayHandles().Reset();
    memset(g_snapshotData, 0, sizeof g_snapshotData);
    g_captureUsed = 0;
    BeginCompare();
    ReplayText t;
    unsigned char* got[2];
    unsigned sizes[2] = { 20, 700 };
    for (int n = 0; n < 2; n++) {
        StartImport(t, "services.SimCaptureAlloc");
        Space(t);
        t.AddUnsigned(sizes[n]);
        const char* r = Enter(t);
        got[n] = (unsigned char*)SnapshotAnswer(r, sizes[n], &offset);
        Check(got[n] != nullptr && g_outCount == 0, "the stub allocates from its own arena and the snapshot lines are not the outputs of the import");
    }
    Check(got[0] != nullptr && got[1] != nullptr && got[1] >= got[0] + 24, "two snapshots are two buffers");
    for (int i = 0; i < 20; i++)
        got[0][i] = (unsigned char)(i + 1);
    for (int i = 0; i < 700; i++)
        got[1][i] = (unsigned char)(i * 3);
    CompareSnapshots();
    Check(g_diffs == 0, "the snapshots the slot filled compare equal");
    got[1][600] ^= 4;
    BeginCompare();
    CompareSnapshots();
    printf("a changed byte: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "snapshot#2[600]") == 0, "a changed byte is reported at its snapshot and byte");
    got[1][600] ^= 4;
    memset(g_snapshotData + 1, 0, sizeof g_snapshotData[0]);
    BeginCompare();
    CompareSnapshots();
    Check(g_diffs == 1 && strcmp(g_firstKey, "snapshot#2") == 0, "a snapshot the stub never allocated is reported");
    g_snapshotData[1] = got[1];
    g_snapshotBytes[1] = 699;
    BeginCompare();
    CompareSnapshots();
    Check(g_diffs == 1 && strcmp(g_firstKey, "snapshot#2.length") == 0, "and a buffer of another size");
    g_snapshotBytes[1] = 700;
    unsigned later = 0;
    unsigned char* placed = (unsigned char*)SnapshotAnswer("snapshot#1+5784", 56, &later);
    Check(placed == g_captureArena + 5784 && later == 5784, "an offset after the ordinal is where the buffer is placed and the offset it answers");
    g_captureUsed = 0;
    placed = (unsigned char*)SnapshotAnswer("snapshot#2", 56, &later);
    Check(placed == g_captureArena && later == 0, "a line with no offset follows the buffers of the call, from the start of the arena");
    free(body);
    t.Free();
    recorded.Free();
}

void BeginServices(const char* body)
{
    LoadCall(body);
    BeginImport();
    memset(g_snapshotData, 0, sizeof g_snapshotData);
}

void StringSelfTest()
{
    printf("a string field\n");
    const ReplayObjectField* f = ReplayFindObjectField(ROK_WINDMETEORSPARAMS, "weapon");
    WindMeteorsParams params;
    memset(&params, 0, sizeof params);
    Check(f != nullptr && ReplayRestoreObjectField(*f, &params, "\"METEOR\"", g_objects, AllocObject) && strcmp(params.params.weapon, "METEOR") == 0, "a quoted string is restored");
    Check(f != nullptr && !ReplayRestoreObjectField(*f, &params, "x:4d455445", g_objects, AllocObject), "and bytes in its place are refused");
}

void FilesSelfTest()
{
    printf("services: files and data\n");
    const char* body =
        "  i services.TdfFile_SelectRecordAt file#1 0 = 1\n"
        "  o file#1.current rec#1\n"
        "  i services.TdfRecord_FindSubRecord rec#1 \"units\" = rec#2\n"
        "  i services.TdfRecord_GetSubRecordCount rec#2 = 3\n"
        "  i services.TdfRecord_GetSubRecord rec#2 1 = rec#3\n"
        "  i services.TdfRecord_GetFieldName rec#3 0 = \"Name\"\n"
        "  i services.TdfRecord_CopyRecordName rec#3 16\n"
        "  o dest \"Armada\"\n"
        "  i services.TdfRecord_GetFieldFixed rec#3 \"scale\" 5\n"
        "  o dst 98304\n"
        "  i services.TdfRecord_FindFieldValue rec#3 \"none\" = null\n"
        "  i services.FilePrintf \"%d units\" \"3 units\" = 7\n";
    BeginServices(body);
    TdfFile* file = (TdfFile*)calloc(1, sizeof(TdfFile));
    Check(file->SelectRecordAt(0) == 1 && file->current == ParseRecord("rec#1"), "a call that moves the current record gives its result and its out");
    TdfRecord* units = file->current->FindSubRecord("units");
    Check(units == ParseRecord("rec#2"), "a record found is the one recorded");
    Check(units->GetSubRecordCount() == 3, "a count replays");
    TdfRecord* second = units->GetSubRecord(1);
    Check(second == ParseRecord("rec#3"), "a sub-record by index replays");
    char* name = second->GetFieldName(0);
    Check(name != nullptr && strcmp(name, "Name") == 0, "a text result replays");
    char copy[16] = "";
    second->CopyRecordName(copy, sizeof copy);
    Check(strcmp(copy, "Armada") == 0, "a text written through a pointer replays");
    int fixed = 0;
    int* back = second->GetFieldFixed(&fixed, "scale", 5);
    Check(back == &fixed && fixed == 98304, "a number written through a pointer replays and the pointer is returned");
    Check(second->FindFieldValue("none") == 0, "a null text result is zero");
    Check(FilePrintf(nullptr, "%d units", 3) == 7, "a formatted text is compared and the result replays");
    Check(g_diffs == 0 && g_importIndex == 9, "and nothing differs");
    BeginServices(body);
    file->SelectRecordAt(0);
    file->current->FindSubRecord("units");
    ParseRecord("rec#2")->GetSubRecordCount();
    ParseRecord("rec#2")->GetSubRecord(2);
    printf("a changed argument: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 3 services.TdfRecord_GetSubRecord") == 0, "a changed argument is reported at its import");
    BeginServices("  i services.FilePrintf \"%d units\" \"3 units\" = 7\n");
    FilePrintf(nullptr, "%d units", 4);
    Check(g_diffs == 1 && strstr(g_detail.Data(), "\"4 units\"") != nullptr, "and so is a changed formatted text");
    free(file);
}

void CommandsSelfTest()
{
    printf("services: commands\n");
    const char* body =
        "  i services.Translate \"key\" = \"Teksti\"\n"
        "  i services.Translate \"gone\" = null\n"
        "  i services.CommandArgs_InitArgs\n"
        "  i services.ComputeChecksum x:0102ff 3 = 1234567\n";
    BeginServices(body);
    char* text = Translate("key");
    Check(text != nullptr && strcmp(text, "Teksti") == 0, "a translation replays as the recorded text");
    Check(Translate("gone") == nullptr, "and a null one as null");
    CommandArgs* args = (CommandArgs*)calloc(1, sizeof(CommandArgs));
    args->count = 4;
    Check(args->InitArgs() == args && args->count == 0, "InitArgs clears the list and returns it");
    unsigned char data[3] = { 1, 2, 0xff };
    Check(ComputeChecksum(data, 3) == 1234567, "a checksum is compared on its bytes and replays");
    Check(g_diffs == 0 && g_importIndex == 4, "and nothing differs");
    BeginServices(body);
    Translate("key");
    Translate("gone");
    args->InitArgs();
    data[2] = 0xfe;
    ComputeChecksum(data, 3);
    printf("a changed argument: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 3 services.ComputeChecksum") == 0 && strstr(g_detail.Data(), "x:0102fe") != nullptr, "a changed byte is reported at the import");
    BeginServices(body);
    Translate("other");
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 services.Translate") == 0, "and so is a changed key");
    free(args);
}

void SimPlumbingSelfTest()
{
    printf("services: sim plumbing\n");
    ResetBlock();
    const char* body =
        "  i services.SimAlloc 64 = alloc#1\n"
        "  i services.SimAllocTagged \"Player Info\" 185 = PlayerInfo#1\n"
        "  i services.SimAllocTagged \"Other\" 16 = alloc#2\n"
        "  i services.SimFree alloc#1\n"
        "  i services.RecordSimEvent 2 1 [3,4] 2\n"
        "  i services.FindSlotByDpid 77 = 2\n"
        "  i services.GetSlotDpid 2 = 77\n"
        "  i services.SimCaptureAlloc 20 = snapshot#1\n"
        "  o snapshot#1 x:0102030405060708090a0b0c0d0e0f1011121314\n";
    BeginServices(body);
    ReplayAnswerHook() = ReplayAnswerImport;
    void* first = SimAlloc(64);
    void* info = SimAllocTagged("Player Info", 185);
    void* other = SimAllocTagged("Other", 16);
    Check(first != nullptr && g_objects.Pointer(ROK_ALLOC, 1) == first, "an allocation is the replayer's own memory, named as the recording names it");
    Check(info != nullptr && g_objects.Pointer(ROK_PLAYERINFO, 1) == info, "an allocation of a tag the annotation names is an object of its kind");
    Check(other != nullptr && g_objects.Pointer(ROK_ALLOC, 2) == other && other != first, "and another is another block");
    SimFree(first);
    ReplayAnswerHook() = nullptr;
    unsigned args[2] = { 3, 4 };
    RecordSimEvent(SIM_EVENT_SOUND, 1, args, 2);
    Check(FindSlotByDpid(77) == 2 && GetSlotDpid(2) == 77, "a slot lookup replays");
    unsigned offset = 0;
    unsigned char* snapshot = (unsigned char*)SimCaptureAlloc(20, &offset);
    for (int i = 0; i < 20; i++)
        snapshot[i] = (unsigned char)(i + 1);
    CompareSnapshots();
    Check(g_diffs == 0 && g_importIndex == 8, "nothing differs, the free and the event included");

    BeginServices(body);
    ReplayAnswerHook() = ReplayAnswerImport;
    SimAlloc(65);
    ReplayAnswerHook() = nullptr;
    printf("a changed size: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 services.SimAlloc") == 0, "a changed size is reported at the import");
    BeginServices(body);
    ReplayAnswerHook() = ReplayAnswerImport;
    SimAlloc(64);
    SimAllocTagged("Player Info", 184);
    ReplayAnswerHook() = nullptr;
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 1 services.SimAllocTagged") == 0, "so is a changed size of a tagged one");
    BeginServices("  i services.RecordSimEvent 2 1 [3,4] 2\n");
    args[1] = 5;
    RecordSimEvent(SIM_EVENT_SOUND, 1, args, 2);
    Check(g_diffs == 1 && strstr(g_detail.Data(), "[3,5]") != nullptr, "and so are the arguments of an event");
}

void LoadReadsSelfTest()
{
    printf("services: load reads\n");
    ResetBlock();
    const char* body =
        "  i services.LoadAnimGaf \"logos\" = Gaf#1\n"
        "  i services.FindGafEntry Gaf#1 \"radar\" = FrameTable#1\n"
        "  o FrameTable#1.count 3\n"
        "  i services.GetGafFrame FrameTable#1 2 = GafFrame#1\n"
        "  o GafFrame#1.width 64\n"
        "  o GafFrame#1.height 32\n"
        "  o GafFrame#1.xOffset -3\n"
        "  o GafFrame#1.yOffset 4\n"
        "  o GafFrame#1.transparency 1\n"
        "  i services.GetGafFrame FrameTable#1 9 = null\n"
        "  i services.Load3do \"objects3d\\\\arm.3do\" = Object3do#1\n"
        "  o Object3do#1.vertexCount 12\n"
        "  o Object3do#1.box_x -50\n"
        "  o Object3do#1.box_y 7\n"
        "  o Object3do#1.box_z 8\n"
        "  i services.GetObjectHeight Object3do#1 = 88\n";
    BeginServices(body);
    void* gaf = LoadAnimGaf("logos");
    Check(gaf != nullptr && g_objects.Pointer(ROK_GAF, 1) == gaf, "a GAF file is an object the replayer names as the recording does");
    GafEntry* entry = FindGafEntry((GafArchive*)gaf, "radar");
    Check(entry != nullptr && ((FrameTable*)entry)->count == 3, "an entry found has the fields the subsystems read");
    GafFrame* frame = GetGafFrame((FrameTable*)entry, 2);
    Check(frame != nullptr && frame->width == 64 && frame->height == 32 && frame->xOffset == -3 && frame->yOffset == 4 && frame->transparency == 1, "a frame has its recorded size, offsets and transparency");
    Check(GetGafFrame((FrameTable*)entry, 9) == nullptr, "a missing frame is null");
    ReplayAnswerHook() = ReplayAnswerImport;
    Object3do* model = (Object3do*)Load3do((char*)"objects3d\\arm.3do");
    Check(model != nullptr && model->vertexCount == 12 && model->box_x == -50 && model->box_y == 7 && model->box_z == 8, "a 3DO model has its recorded box and vertex count");
    Check(GetObjectHeight(model) == 88, "its height replays");
    ReplayAnswerHook() = nullptr;
    Check(g_diffs == 0 && g_importIndex == 4, "and nothing differs, the frames answered as queries outside the import order");
    BeginServices(body);
    LoadAnimGaf("logos");
    FindGafEntry((GafArchive*)gaf, "radar");
    GetGafFrame((FrameTable*)entry, 3);
    printf("a changed argument: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "query services.GetGafFrame") == 0, "a frame index with no recorded answer is reported at the query");
    BeginServices(body);
    LoadAnimGaf("other");
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 services.LoadAnimGaf") == 0, "and so is a changed name");
}

void GafCursorSelfTest()
{
    printf("services: GAF cursors\n");
    ResetBlock();
    const char* body =
        "  i services.LoadAnimGaf \"logos\" = Gaf#1\n"
        "  i services.FindGafEntry Gaf#1 \"radar\" = FrameTable#1\n"
        "  o FrameTable#1.count 3\n"
        "  i services.InitGafSequence Handle#1 FrameTable#1 0\n"
        "  o Handle#1.index 0\n"
        "  o Handle#1.value 4\n"
        "  o Handle#1.kind 1\n"
        "  o Handle#1.src FrameTable#1\n"
        "  i services.StepGafSequence Handle#1 = 1\n"
        "  o Handle#1.index 1\n"
        "  o Handle#1.value 7\n"
        "  o Handle#1.kind 1\n"
        "  o Handle#1.src FrameTable#1\n"
        "  i services.StepGafSequence Handle#1 = 1\n"
        "  o Handle#1.index 1\n"
        "  o Handle#1.value 7\n"
        "  o Handle#1.kind 0\n"
        "  o Handle#1.src null\n";
    BeginServices(body);
    void* gaf = LoadAnimGaf("logos");
    GafEntry* entry = FindGafEntry((GafArchive*)gaf, "radar");
    Handle* cursor = (Handle*)ReplayResolveObject("Handle#1", nullptr, g_objects, AllocObject);
    InitGafSequence(cursor, entry, 0);
    Check(cursor->index == 0 && cursor->value == 4 && cursor->kind == 1, "a cursor started has the recorded index, value and kind");
    Check(cursor->src == (GafEntry*)LoadAddressIfLoadData(entry) && entry != nullptr, "and its src is the entry's address, not a block offset");
    Check(StepGafSequence(cursor) == 1 && cursor->index == 1 && cursor->value == 7 && cursor->src == (GafEntry*)LoadAddressIfLoadData(entry), "a step replays its result and its fields");
    Check(StepGafSequence(cursor) == 1 && cursor->kind == 0 && cursor->src == nullptr, "a cursor that ran out has a null src");
    Check(g_diffs == 0 && g_importIndex == 5, "and nothing differs");

    BeginServices(body);
    LoadAnimGaf("logos");
    FindGafEntry((GafArchive*)gaf, "radar");
    InitGafSequence(cursor, nullptr, 0);
    printf("a changed entry: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 2 services.InitGafSequence") == 0, "an entry that is not the recorded one is reported at the import");
}

void RawPointerSelfTest()
{
    printf("ptr fields\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    Handle* cursor = (Handle*)SimAlloc(sizeof(Handle));
    FrameTable* table = (FrameTable*)SimAlloc(sizeof(FrameTable));
    memset((void*)cursor, 0, sizeof(Handle));
    memset((void*)table, 0, sizeof(FrameTable));
    cursor->index = 2;
    cursor->value = 9;
    cursor->kind = 1;
    cursor->src = (GafEntry*)table;
    unsigned short kinds[1] = { ROK_HANDLE };
    void* roots[1] = { cursor };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    const char* text = recorder.text.Data();
    printf("%s", text);
    Check(strstr(text, "  r Handle#1.src FrameTable#1\n") != nullptr, "a ptr is written as the object it points at");
    Check(strstr(text, "  r Handle#1.index 2\n") != nullptr && strstr(text, "  r Handle#1.value 9\n") != nullptr && strstr(text, "  r Handle#1.kind 1\n") != nullptr, "beside the fields of its object");
    Check(strstr(text, "0x") == nullptr, "and no address is written");

    ReplayText value;
    const ReplayObjectField* src = ReplayFindObjectField(ROK_HANDLE, "src");
    ReplayObjectTable* stranger = new ReplayObjectTable;
    ReplayFormatObjectField(*src, cursor, *stranger, false, value);
    Check(strcmp(value.Data(), "FrameTable#?") == 0, "a ptr to an object no line named is Kind#?");
    cursor->src = nullptr;
    ReplayFormatObjectField(*src, cursor, *stranger, false, value);
    Check(strcmp(value.Data(), "null") == 0, "and a null one is null");
    cursor->src = (GafEntry*)table;
    value.Free();
    delete stranger;

    ResetBlock();
    RestoreText(text, sets);
    Handle* rebuilt = (Handle*)ReplayResolveObject("Handle#1", nullptr, g_objects, AllocObject);
    FrameTable* rebuiltTable = (FrameTable*)ReplayResolveObject("FrameTable#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuiltTable != nullptr && rebuilt->index == 2 && rebuilt->value == 9 && rebuilt->kind == 1, "the object is rebuilt");
    Check(rebuilt->src == (GafEntry*)LoadAddressIfLoadData(rebuiltTable), "its ptr is the address of the replayer's own copy");
    Recorder again;
    unsigned short againKinds[1] = { ROK_HANDLE };
    void* againRoots[1] = { rebuilt };
    again.Record(againKinds, againRoots, 1, sets);
    Check(strcmp(text, again.text.Data()) == 0, "the rebuilt object records the same text");

    g_rootKinds[0] = ROK_HANDLE;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the unchanged object compares equal");
    rebuilt->src = (GafEntry*)SimAlloc(sizeof(FrameTable));
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed src: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "Handle#1.src") == 0, "a ptr to another object is reported at its Kind#N.field");
    rebuilt->src = nullptr;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "Handle#1.src") == 0, "and so is a null one");
    rebuilt->src = (GafEntry*)rebuiltTable;

    Check(!ReplayRestoreObjectField(*src, rebuilt, "FrameTable#?", g_objects, nullptr), "a ptr to an unnamed object is refused");
    Check(!ReplayRestoreObjectField(*src, rebuilt, "Gaf#1", g_objects, AllocObject), "and so is one to another kind");
    Check(ReplayRestoreObjectField(*src, rebuilt, "null", g_objects, nullptr) && rebuilt->src == nullptr, "null restores as null");
}

void MaskedViewSelfTest()
{
    printf("masked views\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    FixtureView* view = (FixtureView*)SimAlloc(sizeof(FixtureView));
    FrameTable* frames = (FrameTable*)SimAlloc(sizeof(FrameTable));
    UnitDef* defs = (UnitDef*)SimAlloc(2 * (unsigned)sizeof(UnitDef));
    g_game->defs = defs;
    g_game->unitDefCount = 2;
    UnitDef* def = &defs[1];
    memset((void*)view, 0, sizeof(FixtureView));
    memset((void*)frames, 0, sizeof(FrameTable));
    memset((void*)defs, 0, 2 * sizeof(UnitDef));
    frames->count = 3;
    view->flags = 0x0100;
    view->plain.count = 7;
    view->plain.total = 9;
    view->linked.def = Ref<UnitDef>(def);
    unsigned short kinds[1] = { ROK_FIXTUREVIEW };
    void* roots[1] = { view };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    char* plain = strdup(recorder.text.Data());
    printf("%s", plain);
    Check(strstr(plain, "  r FixtureView#1.flags 256\n") != nullptr, "a bit outside the mask does not choose the view");
    Check(strstr(plain, "  r FixtureView#1.count 7\n") != nullptr && strstr(plain, "  r FixtureView#1.total 9\n") != nullptr, "the live view is written");
    Check(strstr(plain, "FixtureView#1.scale") == nullptr && strstr(plain, "FixtureView#1.table") == nullptr && strstr(plain, "FixtureView#1.def") == nullptr, "and the dead one is not");
    Check(strstr(plain, "UnitDef#") == nullptr, "a ref of the dead view is not followed");

    view->flags = 0x0101;
    view->linked.scale = 1.5f;
    view->linked.table = frames;
    view->linked.def = Ref<UnitDef>(def);
    recorder.Record(kinds, roots, 1, sets);
    char* linked = strdup(recorder.text.Data());
    printf("%s", linked);
    Check(strstr(linked, "  r FixtureView#1.flags 257\n") != nullptr && strstr(linked, "  r FixtureView#1.scale f32:0x3fc00000\n") != nullptr, "the other view is written when the bit is set");
    Check(strstr(linked, "  r FixtureView#1.table FrameTable#1\n") != nullptr && strstr(linked, "  r FixtureView#1.def UnitDef#1\n") != nullptr, "its ptr and ref too");
    Check(strstr(linked, "FixtureView#1.count") == nullptr && strstr(linked, "FixtureView#1.total") == nullptr, "and the first view is not");

    view->flags = 0x0100;
    view->plain.count = 7;
    view->plain.total = 9;
    recorder.Record(kinds, roots, 1, sets);
    char* again = strdup(recorder.text.Data());
    printf("%s", again);
    Check(strstr(again, "  r FixtureView#1.count 7\n") != nullptr && strstr(again, "  r FixtureView#1.total 9\n") != nullptr, "a view that comes back is written again, though its values are those it had before");

    ResetBlock();
    RestoreText(plain, sets);
    FixtureView* rebuilt = (FixtureView*)ReplayResolveObject("FixtureView#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->flags == 0x0100 && rebuilt->plain.count == 7 && rebuilt->plain.total == 9, "the live view is restored");
    const ReplayObjectField* scale = ReplayFindObjectField(ROK_FIXTUREVIEW, "scale");
    Check(ReplayRestoreObjectField(*scale, rebuilt, "f32:0x40000000", g_objects, AllocObject) && rebuilt->plain.count == 7, "a value for a dead field is not restored");
    Check(RestoreObjectLine("FixtureView#1.scale", "f32:0x40000000", "self-test", &sets, "r FixtureView#1.scale f32:0x40000000") && rebuilt->plain.count == 7, "nor is a line for one");
    g_rootKinds[0] = ROK_FIXTUREVIEW;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the dead view is not compared, so the object compares equal");

    RestoreText(linked, sets);
    FrameTable* rebuiltFrames = (FrameTable*)ReplayResolveObject("FrameTable#1", nullptr, g_objects, AllocObject);
    UnitDef* rebuiltDef = (UnitDef*)ReplayResolveObject("UnitDef#1", nullptr, g_objects, AllocObject);
    Check(rebuilt->flags == 0x0101 && rebuilt->linked.scale == 1.5f, "the other view is restored");
    Check(rebuilt->linked.table == (FrameTable*)LoadAddressIfLoadData(rebuiltFrames) && rebuilt->linked.def.get() == rebuiltDef, "with its ptr and its ref");
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "and compares equal");

    RestoreText(again, sets);
    Check(rebuilt->flags == 0x0100 && rebuilt->plain.count == 7 && rebuilt->plain.total == 9, "the first view is restored again over the bytes of the other");
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "and compares equal");
    free(plain);
    free(linked);
    free(again);

    ResetBlock();
    printf("masked view through another object\n");
    FixtureSlave* slave = (FixtureSlave*)SimAlloc(sizeof(FixtureSlave));
    FixtureView* master = (FixtureView*)SimAlloc(sizeof(FixtureView));
    memset((void*)slave, 0, sizeof(FixtureSlave));
    memset((void*)master, 0, sizeof(FixtureView));
    Player* player = &g_game->players[3];
    player->flags = 0x0014;
    slave->player = 3;
    slave->master = Ref<FixtureView>(master);
    slave->ground.moved = 5;
    slave->narrow.low = 11;
    unsigned short slaveKinds[2] = { ROK_FIXTURESLAVE, ROK_PLAYER };
    void* slaveRoots[2] = { slave, player };
    Recorder through;
    through.Record(slaveKinds, slaveRoots, 2, sets);
    char* first = strdup(through.text.Data());
    Check(strstr(first, "  r FixtureSlave#1.moved 5\n") != nullptr && strstr(first, "  r FixtureSlave#1.low 11\n") != nullptr, "a field is written when the object it reads says so");
    Check(strstr(first, "FixtureSlave#1.speed") == nullptr && strstr(first, "FixtureSlave#1.high") == nullptr, "and the other views are not");
    Check(strstr(first, "FixtureSlave#1.moved") < strstr(first, "  r Player#3.flags 20\n"), "the line of a guarded field comes before the line it reads");

    player->flags = 0;
    master->flags = 2;
    slave->air.speed = 2.5f;
    slave->wide.high = 0.5f;
    through.Record(slaveKinds, slaveRoots, 2, sets);
    char* second = strdup(through.text.Data());
    Check(strstr(second, "  r FixtureSlave#1.speed f32:0x40200000\n") != nullptr && strstr(second, "  r FixtureSlave#1.high f32:0x3f000000\n") != nullptr, "the views switch with the bits of the objects it reads");
    Check(strstr(second, "FixtureSlave#1.moved") == nullptr && strstr(second, "FixtureSlave#1.low") == nullptr, "and the old ones are not written");

    ResetBlock();
    RestoreText(first, sets);
    FixtureSlave* rebuiltSlave = (FixtureSlave*)ReplayResolveObject("FixtureSlave#1", nullptr, g_objects, AllocObject);
    Check(rebuiltSlave != nullptr && rebuiltSlave->player == 3 && rebuiltSlave->ground.moved == 5 && rebuiltSlave->narrow.low == 11, "the fields that read another object are restored after the fields they read");
    g_rootKinds[0] = ROK_FIXTURESLAVE;
    g_rootObjects[0] = rebuiltSlave;
    g_rootKinds[1] = ROK_PLAYER;
    g_rootObjects[1] = &g_game->players[3];
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "and compare equal");
    RestoreText(second, sets);
    Check(rebuiltSlave->air.speed == 2.5f && rebuiltSlave->wide.high == 0.5f, "the other views are restored");
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "and compare equal");
    free(first);
    free(second);
}

void FeatureTableSelfTest()
{
    printf("the feature table\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    Feature* table = (Feature*)SimAlloc(4 * (unsigned)sizeof(Feature));
    memset((void*)table, 0, 4 * sizeof(Feature));
    g_game->features = table;
    g_game->featureCount = 3;
    table[3].anim.index = 2;
    unsigned short kinds[1] = { ROK_HANDLE };
    void* roots[1] = { &table[3].anim };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    Check(strstr(recorder.text.Data(), "Handle#") == nullptr, "a handle of the element one past the count is not named by order");
    Check(strstr(recorder.text.Data(), "  r Feature#3.anim.index 2\n") != nullptr, "it is named by its site");
    Check(ReplayResolveObject("Feature#3.anim", nullptr, g_objects, AllocObject) == &table[3].anim, "and the name resolves to it");
    Check(ReplayResolveObject("Feature#4.anim", nullptr, g_objects, AllocObject) == nullptr, "an element past that is none");

    ResetBlock();
    g_game->featureCount = 3;
    Check(ReplayResolveObject("Feature#2", nullptr, g_objects, AllocObject) != nullptr, "an element of a table the replayer allocates resolves");
    Check(g_game->featureCount == 3, "and the count a recording restored is kept");

    ResetBlock();
    Feature* defs = (Feature*)SimAlloc(2 * (unsigned)sizeof(Feature));
    memset((void*)defs, 0, 2 * sizeof(Feature));
    g_game->features = defs;
    g_game->featureCount = 2;
    FeatureSpot* pool = (FeatureSpot*)SimAlloc(2048 * (unsigned)sizeof(FeatureSpot));
    memset((void*)pool, 0, 2048 * sizeof(FeatureSpot));
    g_game->spots = pool;
    Spot* spots = (Spot*)pool;
    spots[0].feature = 1;
    unsigned short spotKinds[1] = { ROK_SPOT };
    void* spotRoots[1] = { &spots[0] };
    Recorder spot;
    defs[1].noobject = 1;
    spots[0].anim.index = 5;
    spot.Record(spotKinds, spotRoots, 1, sets);
    Check(strstr(spot.text.Data(), "  r Spot#0.anim.index 5\n") != nullptr, "a spot of a sprite feature records its handle");
    defs[1].noobject = 0;
    spot.Record(spotKinds, spotRoots, 1, sets);
    Check(strstr(spot.text.Data(), "Spot#0.anim") == nullptr, "a spot of a model feature has none");
    defs[1].noobject = 1;
    spots[0].anim.index = 5;
    spot.Record(spotKinds, spotRoots, 1, sets);
    Check(strstr(spot.text.Data(), "  r Spot#0.anim.index 5\n") != nullptr, "a handle that came back after a model phase is recorded again");
}

void EmbeddedSiteSelfTest()
{
    printf("an object embedded at two sites\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    FixtureHolder* holder = (FixtureHolder*)SimAlloc(sizeof(FixtureHolder));
    FrameTable* frames = (FrameTable*)SimAlloc(sizeof(FrameTable));
    Handle* loose = (Handle*)SimAlloc(sizeof(Handle));
    memset((void*)holder, 0, sizeof(FixtureHolder));
    memset((void*)frames, 0, sizeof(FrameTable));
    memset((void*)loose, 0, sizeof(Handle));
    holder->id = 4;
    holder->anim.index = 2;
    holder->anim.value = 9;
    holder->anim.kind = 1;
    holder->anim.src = (GafEntry*)frames;
    holder->shadow.index = 5;
    holder->shadow.value = 6;
    holder->shadow.kind = 1;
    holder->shadow.src = (GafEntry*)frames;
    unsigned short kinds[1] = { ROK_FIXTUREHOLDER };
    void* roots[1] = { holder };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(strstr(text, "  r FixtureHolder#1.anim.index 2\n") != nullptr && strstr(text, "  r FixtureHolder#1.anim.value 9\n") != nullptr, "the walk names an embedded object by its site");
    Check(strstr(text, "  r FixtureHolder#1.shadow.index 5\n") != nullptr && strstr(text, "  r FixtureHolder#1.shadow.src FrameTable#1\n") != nullptr, "and the other site by its own");
    Check(strstr(text, "Handle#") == nullptr, "and no embedded object is named by order");
    ReplayText name;
    ReplayAddObjectName(name, *recorder.table, ROK_HANDLE, &holder->anim, true);
    Check(strcmp(name.Data(), "FixtureHolder#1.anim") == 0, "an argument at the first site is named by it");
    name.Clear();
    ReplayAddObjectName(name, *recorder.table, ROK_HANDLE, &holder->shadow, true);
    Check(strcmp(name.Data(), "FixtureHolder#1.shadow") == 0, "an argument at the second site is named by it");
    name.Clear();
    ReplayAddObjectName(name, *recorder.table, ROK_HANDLE, loose, true);
    Check(strcmp(name.Data(), "Handle#1") == 0, "an object of the kind that is embedded nowhere is named by order");
    name.Free();

    ResetBlock();
    RestoreText(text, sets);
    FixtureHolder* rebuilt = (FixtureHolder*)ReplayResolveObject("FixtureHolder#1", nullptr, g_objects, AllocObject);
    unsigned found = 0;
    Check(ReplayResolveObject("FixtureHolder#1.shadow", &found, g_objects, AllocObject) == &rebuilt->shadow && found == ROK_HANDLE, "a name of a site resolves to the bytes inside its object");
    Check(rebuilt->id == 4 && rebuilt->anim.index == 2 && rebuilt->anim.value == 9 && rebuilt->shadow.index == 5 && rebuilt->shadow.value == 6, "and the fields are restored there");
    Check(g_objects.Pointer(ROK_HANDLE, 1) == nullptr, "no separate object of the kind is allocated");
    g_rootKinds[0] = ROK_FIXTUREHOLDER;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the restored object compares equal");
    free(text);

    const char* body =
        "  i services.LoadAnimGaf \"logos\" = Gaf#1\n"
        "  i services.FindGafEntry Gaf#1 \"radar\" = FrameTable#1\n"
        "  o FrameTable#1.count 3\n"
        "  i services.InitGafSequence FixtureHolder#1.anim FrameTable#1 0\n"
        "  o FixtureHolder#1.anim.index 0\n"
        "  o FixtureHolder#1.anim.value 4\n"
        "  o FixtureHolder#1.anim.kind 1\n"
        "  o FixtureHolder#1.anim.src FrameTable#1\n"
        "  i services.InitGafSequence FixtureHolder#1.shadow FrameTable#1 0\n"
        "  o FixtureHolder#1.shadow.index 1\n"
        "  o FixtureHolder#1.shadow.value 8\n"
        "  o FixtureHolder#1.shadow.kind 1\n"
        "  o FixtureHolder#1.shadow.src FrameTable#1\n";
    ResetBlock();
    BeginServices(body);
    void* gaf = LoadAnimGaf("logos");
    GafEntry* entry = FindGafEntry((GafArchive*)gaf, "radar");
    FixtureHolder* served = (FixtureHolder*)ReplayResolveObject("FixtureHolder#1", nullptr, g_objects, AllocObject);
    InitGafSequence(&served->anim, entry, 0);
    InitGafSequence(&served->shadow, entry, 0);
    Check(served->anim.value == 4 && served->shadow.value == 8 && served->shadow.index == 1, "a service line names each site, and writes the object inside");
    Check(g_diffs == 0 && g_importIndex == 4, "the replayed call names them the same, so nothing differs");
    Check(g_objects.Pointer(ROK_HANDLE, 1) == nullptr, "and no separate object of the kind is allocated");

    BeginServices(body);
    LoadAnimGaf("logos");
    FindGafEntry((GafArchive*)gaf, "radar");
    InitGafSequence(&served->shadow, entry, 0);
    printf("the other site: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 2 services.InitGafSequence") == 0, "a call at the other site is reported at the import");
}

void UnnamedPtrSelfTest()
{
    printf("a ptr to an object no line named\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    FixtureView* view = (FixtureView*)SimAlloc(sizeof(FixtureView));
    Handle* cursor = (Handle*)SimAlloc(sizeof(Handle));
    FrameTable* frames = (FrameTable*)SimAlloc(sizeof(FrameTable));
    memset((void*)view, 0, sizeof(FixtureView));
    memset((void*)cursor, 0, sizeof(Handle));
    memset((void*)frames, 0, sizeof(FrameTable));
    frames->count = 3;
    view->flags = 1;
    view->linked.table = frames;
    cursor->src = (GafEntry*)frames;
    unsigned short kinds[2] = { ROK_FIXTUREVIEW, ROK_HANDLE };
    void* roots[2] = { view, cursor };
    Recorder recorder;
    recorder.Record(kinds, roots, 2, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(strstr(text, "  r FixtureView#1.table FrameTable#1\n") != nullptr, "a ptr is named when it is first seen, and the walk does not reach its object");
    Check(strstr(text, "  r Handle#1.src FrameTable#1\n") != nullptr, "another ptr to it has the same name");
    Check(strstr(text, "FrameTable#1.count") == nullptr && strstr(text, "#?") == nullptr, "the object is not recorded, and no pointer is unnamed");

    ResetBlock();
    RestoreText(text, sets);
    FixtureView* rebuilt = (FixtureView*)ReplayResolveObject("FixtureView#1", nullptr, g_objects, AllocObject);
    Handle* rebuiltCursor = (Handle*)ReplayResolveObject("Handle#1", nullptr, g_objects, AllocObject);
    FrameTable* empty = (FrameTable*)ReplayResolveObject("FrameTable#1", nullptr, g_objects, AllocObject);
    Check(empty != nullptr && empty->count == 0, "the replayer makes an empty object of the kind");
    Check(rebuilt->linked.table == (FrameTable*)LoadAddressIfLoadData(empty) && (FrameTable*)rebuiltCursor->src == (FrameTable*)LoadAddressIfLoadData(empty), "and both ptrs hold its address");
    g_rootKinds[0] = ROK_FIXTUREVIEW;
    g_rootObjects[0] = rebuilt;
    g_rootKinds[1] = ROK_HANDLE;
    g_rootObjects[1] = rebuiltCursor;
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the restored objects compare equal");
    const ReplayObjectField* table = ReplayFindObjectField(ROK_FIXTUREVIEW, "table");
    Check(!ReplayRestoreObjectField(*table, rebuilt, "FrameTable#?", g_objects, AllocObject), "a ptr that is still unnamed is refused");
    free(text);
}

unsigned short FieldIndex(unsigned kind, const char* name)
{
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        if (kReplayObjectFields[i].object == kind && strcmp(kReplayObjectFields[i].name, name) == 0)
            return (unsigned short)i;
    }
    abort();
}

void AllOfListSelfTest()
{
    printf("a slot with two all-of kinds\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureLane* lanes = (FixtureLane*)SimAlloc(4 * (unsigned)sizeof(FixtureLane));
    memset((void*)lanes, 0, 4 * sizeof(FixtureLane));
    g_game->p1cd5 = (SideData*)(void*)lanes;
    g_game->players[1].id = 11;
    g_game->players[2].id = 12;
    lanes[2].level = 9;
    unsigned short fields[2] = { FieldIndex(ROK_PLAYER, "id"), FieldIndex(ROK_FIXTURELANE, "level") };
    unsigned short both[2] = { ROK_PLAYER, ROK_FIXTURELANE };
    ReplaySlotSets sets = { "two", 0, nullptr, 0, nullptr, 0, fields, 2, fields, 2, both, 2 };
    Recorder recorder;
    recorder.Record(nullptr, nullptr, 0, sets);
    char* text = strdup(recorder.text.Data());
    Check(ReplayFailure() == nullptr, "the walk reaches both tables without a failure");
    Check(strstr(text, "  r Player#2.id 12\n") != nullptr, "every element of the first all-of kind is recorded");
    Check(strstr(text, "  r FixtureLane#2.level 9\n") != nullptr && CountLines(text, "  r FixtureLane#") == 4, "and every element of the second");
    ReplaySlotSets first = sets;
    first.allOfCount = 1;
    Recorder one;
    one.Record(nullptr, nullptr, 0, first);
    Check(strstr(one.text.Data(), "FixtureLane") == nullptr && CountLines(one.text.Data(), "  r Player#") == 10, "a slot with one all-of kind records only that kind");

    ResetBlock();
    RestoreText(text, sets);
    FixtureLane* rebuilt = (FixtureLane*)g_game->p1cd5.get();
    Check(rebuilt != nullptr && rebuilt[2].level == 9 && g_game->players[2].id == 12, "both tables are restored");
    if (rebuilt == nullptr) {
        free(text);
        return;
    }
    g_rootCount = 0;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the unchanged tables compare equal");
    rebuilt[2].level = 8;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureLane#2.level") == 0, "a change in the second kind is reported");
    rebuilt[2].level = 9;
    g_game->players[1].id = 3;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "Player#1.id") == 0, "and one in the first");
    free(text);
}

void DeltaTableSelfTest()
{
    printf("a table of structs recorded as changed elements\n");
    ResetBlock();
    ReplayClearFailure();
    const unsigned cells = 9000;
    static_assert(sizeof(FixtureCell) == 14, "the fixture cell is 14 bytes");
    FixtureCell* table = (FixtureCell*)SimAlloc(cells * (unsigned)sizeof(FixtureCell));
    memset((void*)table, 0, cells * sizeof(FixtureCell));
    g_game->p1b8a = (SideData*)(void*)table;
    Unit* pool = (Unit*)SimAlloc(8 * (unsigned)sizeof(Unit));
    g_game->units = pool;
    g_game->poolCount = 8;
    pool[3].health = 12;
    table[40].unit = 3;
    table[500].metal = 9;
    unsigned short fields[5] = { FieldIndex(ROK_FIXTURECELL, "unit"), FieldIndex(ROK_FIXTURECELL, "metal"), FieldIndex(ROK_FIXTURECELL, "flags"), FieldIndex(ROK_FIXTURECELL, "occupant"),
                                 FieldIndex(ROK_UNIT, "health") };
    unsigned short every[1] = { ROK_FIXTURECELL };
    ReplaySlotSets sets = { "delta", 0, nullptr, 0, nullptr, 0, fields, 5, fields, 5, every, 1 };
    Recorder recorder;
    recorder.Record(nullptr, nullptr, 0, sets);
    char* first = strdup(recorder.text.Data());
    Check(ReplayFailure() == nullptr, "a table of more elements than the walk holds is recorded");
    Check(CountLines(first, "  r FixtureCell#") == 3, "the first record writes every element of a field as one run");
    Check(strstr(first, "  r FixtureCell#0+9000.unit 0*40 3 0*8959\n") != nullptr && strstr(first, "  r FixtureCell#0+9000.metal 0*500 9 0*8499\n") != nullptr,
          "a run counts its repeated values");
    Check(strstr(first, "  r FixtureCell#0+9000.flags 0*9000\n") != nullptr, "a run of one value is one token");
    Check(strstr(first, "  r Unit#3.health 12\n") != nullptr && strstr(first, "Unit#0") == nullptr, "the unit that a cell names is reached, and id 0 is not");

    table[12].flags = 258;
    table[7000].metal = 5;
    recorder.Record(nullptr, nullptr, 0, sets);
    char* second = strdup(recorder.text.Data());
    printf("%s", second);
    Check(strcmp(second, "  r FixtureCell#7000.metal 5\n  r FixtureCell#12.flags 258\n") == 0, "the next record writes only the two changed fields");
    recorder.Record(nullptr, nullptr, 0, sets);
    Check(recorder.text.Size() == 0, "and the one after it nothing");

    table[100].flags = 1;
    table[101].flags = 1;
    table[102].flags = 2;
    table[104].flags = 3;
    for (unsigned e = 0; e < cells; e++)
        table[e].metal = (unsigned char)((e * 7 + 1) % 255 + 1);
    recorder.Record(nullptr, nullptr, 0, sets);
    char* third = strdup(recorder.text.Data());
    Check(strstr(third, "  r FixtureCell#100+3.flags 1*2 2\n  r FixtureCell#104.flags 3\n") != nullptr, "changed neighbours are one run, and a lone change keeps its own line");
    Check(CountLines(third, "  r FixtureCell#") >= 12 && strstr(third, "  r FixtureCell#0+") != nullptr && strstr(third, ".metal 2 9 16 ") != nullptr, "a long run is several lines");
    for (const char* p = third; *p != '\0'; p = strchr(p, '\n') + 1)
        Check(strchr(p, '\n') - p < 1100, "and none of them is much longer than a thousand characters");

    ResetBlock();
    RestoreText(first, sets);
    RestoreText(second, sets);
    RestoreText(third, sets);
    FixtureCell* rebuilt = (FixtureCell*)g_game->p1b8a.get();
    bool metal = rebuilt != nullptr;
    for (unsigned e = 0; metal && e < cells; e++)
        metal = rebuilt[e].metal == (unsigned char)((e * 7 + 1) % 255 + 1);
    Check(rebuilt != nullptr && metal && rebuilt[12].flags == 258 && rebuilt[101].flags == 1 && rebuilt[102].flags == 2 && rebuilt[103].flags == 0 && rebuilt[104].flags == 3 && rebuilt[40].unit == 3,
          "the table is restored from the first record and the deltas");
    if (rebuilt == nullptr) {
        free(first);
        free(second);
        free(third);
        return;
    }
    g_rootCount = 0;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored table compares equal");
    rebuilt[8999].metal = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureCell#8999.metal") == 0, "a changed element is reported at its index and field");
    free(first);
    free(second);
    free(third);
}

void DeltaLimitSelfTest()
{
    printf("an element of a delta table that a limited reach also names\n");
    ResetBlock();
    ReplayClearFailure();
    const unsigned cells = 9000;
    FixtureCell* table = (FixtureCell*)SimAlloc(cells * (unsigned)sizeof(FixtureCell));
    memset((void*)table, 0, cells * sizeof(FixtureCell));
    g_game->p1b8a = (SideData*)(void*)table;
    FixtureLane* lanes = (FixtureLane*)SimAlloc(4 * (unsigned)sizeof(FixtureLane));
    memset((void*)lanes, 0, 4 * sizeof(FixtureLane));
    g_game->p1cd5 = (SideData*)(void*)lanes;
    lanes[1].unit = 40;
    table[40].metal = 9;
    table[40].flags = 7;
    table[41].flags = 3;
    unsigned short fields[4] = { FieldIndex(ROK_FIXTURELANE, "unit"), FieldIndex(ROK_FIXTURELANE, "cell"), FieldIndex(ROK_FIXTURECELL, "metal"), FieldIndex(ROK_FIXTURECELL, "flags") };
    unsigned short every[2] = { ROK_FIXTURECELL, ROK_FIXTURELANE };
    ReplaySlotSets sets = { "limit", 0, nullptr, 0, nullptr, 0, fields, 4, fields, 4, every, 2 };
    Recorder recorder;
    recorder.Record(nullptr, nullptr, 0, sets);
    char* text = strdup(recorder.text.Data());
    Check(ReplayFailure() == nullptr, "the walk reaches the table through the lane ids without a failure");
    Check(strstr(text, "  r FixtureCell#0+9000.metal 0*40 9 0*8959\n") != nullptr, "the limited field of the element a lane names is recorded");
    Check(strstr(text, "  r FixtureCell#0+9000.flags 0*40 7 3 0*8958\n") != nullptr, "and so is its other field, which the limit does not list");
    Check(CountLines(text, "  r FixtureCell#") == 2, "each field of each element is written once");

    ResetBlock();
    RestoreText(text, sets);
    FixtureCell* rebuilt = (FixtureCell*)g_game->p1b8a.get();
    Check(rebuilt != nullptr && g_game->p1cd5.get() != nullptr && rebuilt[40].flags == 7 && rebuilt[41].flags == 3, "the table is restored");
    if (rebuilt == nullptr || g_game->p1cd5.get() == nullptr) {
        free(text);
        return;
    }
    g_rootCount = 0;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored table compares equal");
    rebuilt[40].flags = 8;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureCell#40.flags") == 0, "a change in the field the limit does not list is reported");
    rebuilt[40].flags = 7;
    rebuilt[40].metal = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureCell#40.metal") == 0, "and one in the limited field");
    free(text);
}

void UnconditionedUnitIdSelfTest()
{
    printf("a unit id with no condition\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureLane* lanes = (FixtureLane*)SimAlloc(4 * (unsigned)sizeof(FixtureLane));
    memset((void*)lanes, 0, 4 * sizeof(FixtureLane));
    g_game->p1cd5 = (SideData*)(void*)lanes;
    Unit* pool = (Unit*)SimAlloc(8 * (unsigned)sizeof(Unit));
    g_game->units = pool;
    g_game->poolCount = 8;
    pool[5].health = 4;
    lanes[1].unit = 5;
    lanes[3].unit = 9;
    unsigned short fields[4] = { FieldIndex(ROK_FIXTURELANE, "unit"), FieldIndex(ROK_FIXTURELANE, "level"), FieldIndex(ROK_FIXTURELANE, "occupant"), FieldIndex(ROK_UNIT, "health") };
    unsigned short every[1] = { ROK_FIXTURELANE };
    ReplaySlotSets sets = { "unit", 0, nullptr, 0, nullptr, 0, fields, 4, fields, 4, every, 1 };
    Recorder recorder;
    recorder.Record(nullptr, nullptr, 0, sets);
    char* text = strdup(recorder.text.Data());
    Check(strstr(text, "  r FixtureLane#1.unit 5\n") != nullptr && strstr(text, "  r Unit#5.health 4\n") != nullptr, "a non-zero id is followed to its unit");
    Check(strstr(text, "  r FixtureLane#2.unit 0\n") != nullptr && strstr(text, "Unit#0") == nullptr, "an id of 0 is no unit");
    Check(strstr(text, "Unit#9") == nullptr && ReplayFailure() == nullptr, "an id past the pool is skipped");
    Check(strstr(text, "occupant") == nullptr, "the id field is never a line");

    ResetBlock();
    RestoreText(text, sets);
    Unit* rebuilt = (Unit*)ReplayResolveObject("Unit#5", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->health == 4 && ((FixtureLane*)g_game->p1cd5.get())[1].unit == 5, "the unit and the id are rebuilt");
    if (rebuilt == nullptr || g_game->p1cd5.get() == nullptr) {
        free(text);
        return;
    }
    g_game->poolCount = 8;
    g_rootCount = 0;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the restored lanes compare equal");
    rebuilt->health = 3;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "Unit#5.health") == 0, "the unit behind an id is compared");
    free(text);
}

void IdReachSelfTest()
{
    printf("a slot's own reach list for a unit id\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureLane* lanes = (FixtureLane*)SimAlloc(4 * (unsigned)sizeof(FixtureLane));
    memset((void*)lanes, 0, 4 * sizeof(FixtureLane));
    g_game->p1cd5 = (SideData*)(void*)lanes;
    Unit* pool = (Unit*)SimAlloc(8 * (unsigned)sizeof(Unit));
    g_game->units = pool;
    g_game->poolCount = 8;
    pool[5].health = 4;
    pool[5].flags = 0x21;
    lanes[1].unit = 5;
    unsigned short fields[5] = { FieldIndex(ROK_FIXTURELANE, "unit"), FieldIndex(ROK_FIXTURELANE, "level"), FieldIndex(ROK_FIXTURELANE, "occupant"), FieldIndex(ROK_UNIT, "health"),
                                 FieldIndex(ROK_UNIT, "flags") };
    unsigned short every[1] = { ROK_FIXTURELANE };
    ReplaySlotSets plain = { "plain", 0, nullptr, 0, nullptr, 0, fields, 5, fields, 5, every, 1 };
    Recorder recorder;
    recorder.Record(nullptr, nullptr, 0, plain);
    char* text = strdup(recorder.text.Data());
    Check(strstr(text, "  r Unit#5.health 4\n") != nullptr && strstr(text, "Unit#5.flags") == nullptr, "without a slot reach the field's own reach applies");
    free(text);

    unsigned short own[1] = { FieldIndex(ROK_UNIT, "flags") };
    ReplayIdReach byId[1] = { { FieldIndex(ROK_FIXTURELANE, "occupant"), own, 1 } };
    ReplaySlotSets sets = { "by id", 0, nullptr, 0, nullptr, 0, fields, 5, fields, 5, every, 1, byId, 1 };
    Recorder slot;
    slot.Record(nullptr, nullptr, 0, sets);
    text = strdup(slot.text.Data());
    Check(strstr(text, "  r Unit#5.flags 0x00000021\n") != nullptr && strstr(text, "Unit#5.health") == nullptr, "the slot's reach list replaces the field's own");
    Check(ReplayFailure() == nullptr, "and the walk does not fail");

    ResetBlock();
    RestoreText(text, sets);
    Unit* rebuilt = (Unit*)ReplayResolveObject("Unit#5", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->flags == 0x21 && rebuilt->health == 0, "the replayer restores the fields of the slot's reach only");
    if (rebuilt == nullptr || g_game->p1cd5.get() == nullptr) {
        free(text);
        return;
    }
    g_game->poolCount = 8;
    g_rootCount = 0;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the restored unit compares equal");
    rebuilt->health = 3;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "a field outside the slot's reach is not compared");
    rebuilt->flags = 0x20;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "Unit#5.flags") == 0, "a field of the slot's reach is compared");
    free(text);
}

void RecordGameFields(Recorder& recorder, const ReplaySlotSets& sets)
{
    ReplayText value;
    recorder.text.Clear();
    for (unsigned i = 0; i < sets.readCount; i++) {
        const ReplayField& f = kReplayFields[sets.reads[i]];
        ReplayEmitField(recorder.text, recorder.known, *recorder.table, 'r', f.name, ReplayFieldView(f), g_game, value);
    }
    value.Free();
    ReplayWalk(*recorder.reach, *recorder.table, true, sets, nullptr, nullptr, 0);
    ReplayCaptureObjects(recorder.text, recorder.known, *recorder.table, *recorder.reach, sets.objReads, sets.objReadCount, 'r');
}

void GameRefSelfTest()
{
    printf("a ref field of Game\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureView* view = (FixtureView*)SimAlloc(sizeof(FixtureView));
    memset((void*)view, 0, sizeof(FixtureView));
    view->flags = 4;
    g_game->slowest = (Player*)(void*)view;
    unsigned char reads[1] = { RF_GAME_SLOWEST };
    unsigned short objects[1] = { FieldIndex(ROK_FIXTUREVIEW, "flags") };
    ReplaySlotSets sets = { "ref", 0, reads, 1, reads, 1, objects, 1, objects, 1 };
    Recorder recorder;
    RecordGameFields(recorder, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(strcmp(text, "  r Game.slowest FixtureView#1\n  r FixtureView#1.flags 4\n") == 0, "the target is named at first sight, and the walk reaches it");

    view->flags = 6;
    g_game->slowest = (Player*)nullptr;
    RecordGameFields(recorder, sets);
    Check(strcmp(recorder.text.Data(), "  r Game.slowest null\n") == 0, "a null ref is written null, and no object follows");

    ResetBlock();
    char* line = strchr(text, '\n');
    *line = '\0';
    Check(RestoreField(kReplayFields[RF_GAME_SLOWEST], strchr(text + 4, ' ') + 1), "the ref line restores");
    *line = '\n';
    RestoreText(line + 1, sets);
    FixtureView* rebuilt = (FixtureView*)g_game->slowest.get();
    Check(rebuilt != nullptr && rebuilt->flags == 4, "the replayer allocates the target and restores its fields");
    if (rebuilt == nullptr) {
        free(text);
        return;
    }
    ReplayText now;
    FormatField(kReplayFields[RF_GAME_SLOWEST], now);
    Check(strcmp(now.Data(), "FixtureView#1") == 0, "the restored ref is written as it was");
    g_rootCount = 0;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the target compares equal through the Game field");
    rebuilt->flags = 5;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureView#1.flags") == 0, "a changed field of the target is reported");
    Check(RestoreField(kReplayFields[RF_GAME_SLOWEST], "null") && g_game->slowest.get() == nullptr, "null restores as null");
    FormatField(kReplayFields[RF_GAME_SLOWEST], now);
    Check(strcmp(now.Data(), "null") == 0, "and is written null");
    now.Free();
    free(text);
}

void GamePtrSelfTest()
{
    printf("a raw pointer field of Game\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureView* view = (FixtureView*)SimAlloc(sizeof(FixtureView));
    memset((void*)view, 0, sizeof(FixtureView));
    view->flags = 4;
    g_game->gaf_1480f = (void*)view;
    unsigned char reads[1] = { RF_GAME_GAF_1480F };
    unsigned short objects[1] = { FieldIndex(ROK_FIXTUREVIEW, "flags") };
    ReplaySlotSets sets = { "ptr", 0, reads, 1, reads, 1, objects, 1, objects, 1 };
    Recorder recorder;
    RecordGameFields(recorder, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(strcmp(text, "  r Game.gaf_1480f FixtureView#1\n  r FixtureView#1.flags 4\n") == 0, "the target is named at first sight, and the walk reaches it");

    view->flags = 6;
    g_game->gaf_1480f = nullptr;
    RecordGameFields(recorder, sets);
    Check(strcmp(recorder.text.Data(), "  r Game.gaf_1480f null\n") == 0, "a null pointer is written null, and no object follows");

    ResetBlock();
    char* line = strchr(text, '\n');
    *line = '\0';
    Check(RestoreField(kReplayFields[RF_GAME_GAF_1480F], strchr(text + 4, ' ') + 1), "the pointer line restores");
    *line = '\n';
    RestoreText(line + 1, sets);
    FixtureView* rebuilt = (FixtureView*)g_game->gaf_1480f;
    Check(rebuilt != nullptr && rebuilt->flags == 4, "the replayer allocates the target and restores its fields");
    if (rebuilt == nullptr) {
        free(text);
        return;
    }
    ReplayText now;
    FormatField(kReplayFields[RF_GAME_GAF_1480F], now);
    Check(strcmp(now.Data(), "FixtureView#1") == 0, "the restored pointer is written as it was");
    g_rootCount = 0;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the target compares equal through the Game field");
    rebuilt->flags = 5;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureView#1.flags") == 0, "a changed field of the target is reported");
    Check(RestoreField(kReplayFields[RF_GAME_GAF_1480F], "null") && g_game->gaf_1480f == nullptr, "null restores as null");
    FormatField(kReplayFields[RF_GAME_GAF_1480F], now);
    Check(strcmp(now.Data(), "null") == 0, "and is written null");
    now.Free();
    free(text);
}

void ExecuteCommandSelfTest()
{
    printf("services: ExecuteCommandText\n");
    ResetBlock();
    const char* body =
        "  i services.ExecuteCommandText \"meteor 1\\nmeteor 0\" 17 [\"x\"] -1 = 1\n"
        "  i services.RandomInt 5 = 3\n";
    BeginServices(body);
    CommandArgs vars;
    memset(&vars, 0, sizeof vars);
    char first[2] = "x";
    vars.args[0] = first;
    vars.count = 1;
    char text[] = "meteor 1\nmeteor 0";
    int ran = ExecuteCommandText(text, 17, &vars, -1);
    Check(ran == 1, "the result of the command text replays");
    Check(RandomInt(5) == 3, "the imports after the commands replay from where the commands left the lines");
    Check(g_diffs == 0 && g_importIndex == 2, "nothing differs, the import included");

    BeginServices(body);
    ExecuteCommandText(text, 17, &vars, 0);
    printf("a changed mask: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 services.ExecuteCommandText") == 0, "a changed argument is reported at the import");
}

typedef int(__stdcall* NotifyFn)(Unit*, Order*, int);

unsigned CodeIndex(const char* table, const void* function)
{
    for (unsigned t = 0; t < kReplayCodeTableCount; t++) {
        if (strcmp(kReplayCodeTables[t].name, table) != 0)
            continue;
        for (unsigned i = 0; i < kReplayCodeTables[t].count; i++) {
            if (kReplayCodeTables[t].entries[i] == function)
                return i;
        }
    }
    return ~0u;
}

void SetVtable(void* object, const void* vtable)
{
    *(const void**)object = vtable;
}

void PolymorphicSelfTest()
{
    printf("polymorphic objects\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    Order* first = (Order*)SimAlloc(sizeof(Order));
    Order* second = (Order*)SimAlloc(sizeof(Order));
    memset((void*)first, 0, sizeof(Order));
    memset((void*)second, 0, sizeof(Order));
    SetVtable(first, ReplayVtable(ROK_ORDER));
    SetVtable(second, ReplayVtable(ROK_ORDER));
    SetVtable(&first->link, ReplayVtable(ROK_PATHORDERATTACH));
    SetVtable(&second->link, ReplayVtable(ROK_PATHORDERATTACH));
    ApproachRadius* area = (ApproachRadius*)SimAlloc(sizeof(ApproachRadius));
    RingApproach* ring = (RingApproach*)SimAlloc(sizeof(RingApproach));
    memset((void*)area, 0, sizeof(ApproachRadius));
    memset((void*)ring, 0, sizeof(RingApproach));
    SetVtable(area, ReplayVtable(ROK_APPROACHRADIUS));
    SetVtable(ring, ReplayVtable(ROK_RINGAPPROACH));
    area->source = first;
    area->pos.x = 3;
    area->pos.y = 4;
    area->radius = 40;
    area->radiusSq = 1600;
    ring->source = second;
    ring->inner = 10;
    ring->outer = 20;
    first->state = 2;
    first->attached = area;
    second->attached = ring;
    second->next = first;

    unsigned short kinds[2] = { ROK_ORDER, ROK_ORDER };
    void* roots[2] = { first, second };
    Recorder recorder;
    recorder.Record(kinds, roots, 2, sets);
    const char* text = recorder.text.Data();
    Check(ReplayFailure() == nullptr, "every vtable is one of the kinds");
    Check(strstr(text, "  r Order#1.attached ApproachRadius#1\n") != nullptr, "a pointer to the base is written as the kind its vtable says");
    Check(strstr(text, "  r Order#2.attached RingApproach#1\n") != nullptr, "another object of the base is another kind");
    Check(strstr(text, "  r ApproachRadius#1.radius 40\n") != nullptr && strstr(text, "  r ApproachRadius#1.pos (3,4)\n") != nullptr, "the fields are those of the kind");
    Check(strstr(text, "  r RingApproach#1.inner 10\n") != nullptr && strstr(text, "  r RingApproach#1.outer 20\n") != nullptr, "and of the other kind");
    Check(strstr(text, "  r RingApproach#1.source Order#2\n") != nullptr, "a field of the base part is a field of the kind");
    Check(strstr(text, "OrderFx#") == nullptr && strstr(text, "vtable") == nullptr && strstr(text, "vptr") == nullptr, "neither the base nor the vtable is written");

    ResetBlock();
    RestoreText(text, sets);
    Order* rebuiltFirst = (Order*)ReplayResolveObject("Order#1", nullptr, g_objects, AllocObject);
    Order* rebuiltSecond = (Order*)ReplayResolveObject("Order#2", nullptr, g_objects, AllocObject);
    ApproachRadius* rebuiltArea = (ApproachRadius*)ReplayResolveObject("ApproachRadius#1", nullptr, g_objects, AllocObject);
    RingApproach* rebuiltRing = (RingApproach*)ReplayResolveObject("RingApproach#1", nullptr, g_objects, AllocObject);
    Check(rebuiltFirst != nullptr && rebuiltSecond != nullptr && rebuiltArea != nullptr && rebuiltRing != nullptr, "the objects are rebuilt");
    Check(rebuiltFirst->attached.get() == (OrderFx*)rebuiltArea && rebuiltSecond->attached.get() == (OrderFx*)rebuiltRing, "the base pointer leads to the rebuilt kind");
    Check(*(const void**)rebuiltArea == ReplayVtable(ROK_APPROACHRADIUS) && *(const void**)rebuiltRing == ReplayVtable(ROK_RINGAPPROACH), "each is built with the vtable of the replayer");
    Check(rebuiltArea->radius == 40 && rebuiltArea->pos.x == 3 && rebuiltRing->outer == 20 && rebuiltFirst->state == 2, "the fields are restored");
    Check(rebuiltArea->source.get() == rebuiltFirst && rebuiltSecond->next.get() == rebuiltFirst, "and the pointers between the kinds");

    Recorder again;
    unsigned short againKinds[2] = { ROK_ORDER, ROK_ORDER };
    void* againRoots[2] = { rebuiltFirst, rebuiltSecond };
    again.Record(againKinds, againRoots, 2, sets);
    Check(strcmp(text, again.text.Data()) == 0, "the rebuilt objects record the same text");

    memcpy(g_rootKinds, againKinds, sizeof againKinds);
    g_rootObjects[0] = rebuiltFirst;
    g_rootObjects[1] = rebuiltSecond;
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the unchanged objects compare equal");
    rebuiltArea->radius = 41;
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed field: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "ApproachRadius#1.radius") == 0, "a changed field of a kind is reported at its kind and field");
    rebuiltArea->radius = 40;
    SetVtable(rebuiltArea, ReplayVtable(ROK_RINGAPPROACH));
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed kind: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs > 0 && strncmp(g_firstKey, "Order#1.attached", 16) == 0, "a pointer that leads to another kind is reported at the pointer");
    SetVtable(rebuiltArea, ReplayVtable(ROK_APPROACHRADIUS));

    printf("a vtable of no kind\n");
    static void* stray[2];
    ApproachRadius* odd = (ApproachRadius*)SimAlloc(sizeof(ApproachRadius));
    memset((void*)odd, 0, sizeof(ApproachRadius));
    *(const void**)odd = (const void*)stray;
    rebuiltFirst->attached = odd;
    ReplayClearFailure();
    Recorder unnamed;
    unnamed.Record(againKinds, againRoots, 2, sets);
    Check(strstr(unnamed.text.Data(), "attached OrderFx#?\n") != nullptr, "the object is not named");
    const char* failure = ReplayFailure();
    printf("%s\n", failure == nullptr ? "no failure" : failure);
    Check(failure != nullptr && strstr(failure, "OrderFx") != nullptr, "the failure names the base");
    Check(failure != nullptr && strstr(failure, "0x") == nullptr, "and writes no address");
    rebuiltFirst->attached = rebuiltArea;
    ReplayClearFailure();

    printf("code pointers\n");
    ResetBlock();
    OrderTableEntry* entry = (OrderTableEntry*)SimAlloc(sizeof(OrderTableEntry));
    memset((void*)entry, 0, sizeof(OrderTableEntry));
    entry->notify = (NotifyFn)MakeSelectableOrder;
    entry->markerAnim = 2;
    entry->flags = 0x413;
    unsigned stop = CodeIndex("orderNotify", (const void*)StopOrder);
    unsigned selectable = CodeIndex("orderNotify", (const void*)MakeSelectableOrder);
    unsigned wait = CodeIndex("orderNotify", (const void*)WaitOrder);
    Check(stop != ~0u && selectable != ~0u && wait != ~0u && stop != selectable && selectable != wait, "the table lists the functions by their order of registration");
    unsigned short entryKind = ROK_ORDERTABLEENTRY;
    void* entryRoot = entry;
    Recorder code;
    code.Record(&entryKind, &entryRoot, 1, sets);
    char want[96];
    sprintf(want, "  r OrderTableEntry#1.notify orderNotify[%u]\n", selectable);
    Check(strstr(code.text.Data(), want) != nullptr, "a function is written as its index in the table");
    Check(ReplayFailure() == nullptr, "and a listed function is no failure");
    entry->notify = nullptr;
    code.Record(&entryKind, &entryRoot, 1, sets);
    Check(strstr(code.text.Data(), "  r OrderTableEntry#1.notify null\n") != nullptr, "a null pointer is null");
    entry->notify = (NotifyFn)MakeSelectableOrder;
    code.Record(&entryKind, &entryRoot, 1, sets);
    Check(strstr(code.text.Data(), want) != nullptr, "a pointer that changed is written again");
    code.Record(&entryKind, &entryRoot, 1, sets);
    Check(code.text.Size() == 0, "an unchanged pointer is not written again");

    Recorder source;
    entry->notify = (NotifyFn)WaitOrder;
    source.Record(&entryKind, &entryRoot, 1, sets);
    ResetBlock();
    RestoreText(source.text.Data(), sets);
    OrderTableEntry* rebuiltEntry = (OrderTableEntry*)ReplayResolveObject("OrderTableEntry#1", nullptr, g_objects, AllocObject);
    Check(rebuiltEntry != nullptr && rebuiltEntry->notify == (NotifyFn)WaitOrder, "the replayer stores its own function of that index");
    Check(rebuiltEntry != nullptr && rebuiltEntry->markerAnim == 2 && rebuiltEntry->flags == 0x413, "and the other fields");
    Recorder replayedCode;
    void* rebuiltRoot = rebuiltEntry;
    replayedCode.Record(&entryKind, &rebuiltRoot, 1, sets);
    Check(strcmp(source.text.Data(), replayedCode.text.Data()) == 0, "the rebuilt entry records the same text");
    g_rootKinds[0] = ROK_ORDERTABLEENTRY;
    g_rootObjects[0] = rebuiltEntry;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the same function compares equal");
    rebuiltEntry->notify = (NotifyFn)StopOrder;
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed function: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "OrderTableEntry#1.notify") == 0, "another function is reported at its field");
    Check(strstr(g_detail.Data(), "recorded orderNotify[") != nullptr && strstr(g_detail.Data(), "replayed orderNotify[") != nullptr, "by the indexes of both");
    ReplayClearFailure();
    rebuiltEntry->notify = (NotifyFn)FixtureOtherNotify;
    replayedCode.Record(&entryKind, &rebuiltRoot, 1, sets);
    printf("%s\n", ReplayFailure() == nullptr ? "no failure" : ReplayFailure());
    Check(strstr(replayedCode.text.Data(), "notify orderNotify[?]\n") != nullptr && ReplayFailure() != nullptr && strstr(ReplayFailure(), "orderNotify") != nullptr, "a function of no entry is a failure that names the table");
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "0x") == nullptr, "and writes no address");
    ReplayClearFailure();
    Check(!ReplayRestoreObjectField(*ReplayFindObjectField(ROK_ORDERTABLEENTRY, "notify"), rebuiltEntry, "orderNotify[9999]", g_objects, nullptr), "an index past the table is refused");
    Check(!ReplayRestoreObjectField(*ReplayFindObjectField(ROK_ORDERTABLEENTRY, "notify"), rebuiltEntry, "other[1]", g_objects, nullptr), "and so is another table");
}
}

void* Fresh(unsigned size)
{
    void* p = SimAlloc(size);
    memset(p, 0, size);
    return p;
}

void GoalSelfTest()
{
    printf("a kind whose vtable is the compiler's\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    FixtureGoalHolder* holder = (FixtureGoalHolder*)Fresh(sizeof(FixtureGoalHolder));
    FixtureGoalA* a = new (Fresh(sizeof(FixtureGoalA))) FixtureGoalA();
    FixtureGoalB* b = new (Fresh(sizeof(FixtureGoalB))) FixtureGoalB();
    a->ticks = 4;
    a->heading = -9;
    b->ticks = 6;
    b->count = 3;
    holder->id = 2;
    holder->goal = a;
    Check(*(const void**)a == ReplayVtable(ROK_FIXTUREGOALA) && *(const void**)b == ReplayVtable(ROK_FIXTUREGOALB), "the vtable of the kind is the one the compiler gave the class");
    Check(ReplayVtable(ROK_FIXTUREGOALA) != ReplayVtable(ROK_FIXTUREGOALB), "and each kind has its own");

    unsigned short kinds[1] = { ROK_FIXTUREGOALHOLDER };
    void* roots[1] = { holder };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    char* first = strdup(recorder.text.Data());
    printf("%s", first);
    Check(ReplayFailure() == nullptr, "an object of such a kind is recorded");
    Check(strstr(first, "  r FixtureGoalHolder#1.goal FixtureGoalA#1\n") != nullptr && strstr(first, "  r FixtureGoalA#1.heading -9\n") != nullptr, "and named as the kind its vptr says");
    holder->goal = b;
    recorder.Record(kinds, roots, 1, sets);
    char* second = strdup(recorder.text.Data());
    Check(strstr(second, "  r FixtureGoalHolder#1.goal FixtureGoalB#1\n") != nullptr, "another kind of the base is named as its own");

    ResetBlock();
    RestoreText(first, sets);
    FixtureGoalHolder* rebuilt = (FixtureGoalHolder*)ReplayResolveObject("FixtureGoalHolder#1", nullptr, g_objects, AllocObject);
    FixtureGoalA* rebuiltA = (FixtureGoalA*)ReplayResolveObject("FixtureGoalA#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuiltA != nullptr && rebuilt->goal.get() == (FixtureGoalBase*)rebuiltA, "the object is rebuilt and linked");
    if (rebuilt == nullptr || rebuiltA == nullptr) {
        free(first);
        free(second);
        return;
    }
    Check(*(const void**)rebuiltA == ReplayVtable(ROK_FIXTUREGOALA) && rebuiltA->Kind() == 1, "it has the class's vtable, so a virtual call on it runs the class's method");
    g_rootKinds[0] = ROK_FIXTUREGOALHOLDER;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored object compares equal");
    *(const void**)rebuiltA = ReplayVtable(ROK_FIXTUREGOALB);
    BeginCompare();
    CompareObjects(&sets);
    printf("another vptr: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs > 0, "an object whose vptr is another kind's is a difference");
    free(first);
    free(second);

    ResetBlock();
    ReplayClearFailure();
    FixtureGoalHolder* twinHolder = (FixtureGoalHolder*)Fresh(sizeof(FixtureGoalHolder));
    FixtureTwin* twin = new (Fresh(sizeof(FixtureTwin))) FixtureTwin();
    twin->ticks = 9;
    twin->count = 5;
    twinHolder->id = 3;
    twinHolder->goal = twin;
    roots[0] = twinHolder;
    Recorder twinRecorder;
    twinRecorder.Record(kinds, roots, 1, sets);
    char* third = strdup(twinRecorder.text.Data());
    printf("%s", third);
    Check(ReplayFailure() == nullptr && strstr(third, "  r FixtureGoalHolder#1.goal FixtureTwin#1\n") != nullptr, "a class with a second vtable is named as its kind by its first");
    ResetBlock();
    RestoreText(third, sets);
    FixtureTwin* rebuiltTwin = (FixtureTwin*)ReplayResolveObject("FixtureTwin#1", nullptr, g_objects, AllocObject);
    Check(rebuiltTwin != nullptr && rebuiltTwin->Kind() == 3 && rebuiltTwin->count == 5, "it is rebuilt");
    if (rebuiltTwin != nullptr)
        Check(static_cast<FixtureScanBase*>(rebuiltTwin)->Scan() == 7, "with the vtable of its second base too, so a call through that base runs the class's method");
    free(third);

    printf("an object that a slot creates and returns as its offset\n");
    ResetBlock();
    g_makeGoalB = false;
    g_rootCount = 0;
    LoadCall("  o result FixtureGoalA#1\n");
    BeginCompare();
    Run_fixture_MakeGoal();
    After_fixture_MakeGoal();
    Check(g_diffs == 0 && g_rootCount == 1 && g_rootKinds[0] == ROK_FIXTUREGOALA, "it takes the recorded name and is walked as a root");
    ResetBlock();
    g_makeGoalB = true;
    g_rootCount = 0;
    LoadCall("  o result FixtureGoalA#1\n");
    BeginCompare();
    Run_fixture_MakeGoal();
    After_fixture_MakeGoal();
    printf("another kind: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "result") == 0, "an object of another kind is reported at the result");
    g_makeGoalB = false;

    printf("an object that an import creates and returns as its offset\n");
    ResetBlock();
    LoadCall("  i fixture.MakeGoal = FixtureGoalB#2\n");
    BeginImport();
    unsigned made = Imp_fixture_MakeGoal();
    FixtureGoalB* madeB = (FixtureGoalB*)ReplayRefPtr(made);
    Check(g_diffs == 0 && madeB != nullptr && *(const void**)madeB == ReplayVtable(ROK_FIXTUREGOALB), "the stub allocates the recorded kind in the block, with its vtable");
    Check(madeB != nullptr && madeB->Kind() == 2, "so a virtual call on it runs the class's method");
    LoadCall("  i fixture.MakeGoal = FixtureGoalB#2\n");
    BeginImport();
    Check(Imp_fixture_MakeGoal() == made, "a name it has made returns the same offset");
    LoadCall("  i fixture.MakeGoal = null\n");
    BeginImport();
    Check(Imp_fixture_MakeGoal() == 0, "and null returns 0");

    printf("a handle the slot takes as a scalar and the import names as an object\n");
    LoadCall("  a goal 777\n  o result 1\n");
    BeginCompare();
    Run_fixture_HandGoal();
    After_fixture_HandGoal();
    Check(g_diffs == 0, "the slot side records and compares the number");
    LoadCall("  i fixture.HandGoal FixtureGoalB#2 = 1\n");
    BeginImport();
    Check(Imp_fixture_HandGoal(made) == 1 && g_diffs == 0, "the import side passes the name of the object it was made as");
    LoadCall("  i fixture.HandGoal FixtureGoalB#3 = 1\n");
    BeginImport();
    Imp_fixture_HandGoal(made);
    Check(g_diffs != 0, "and a call with another object is a difference");

    printf("an object field that an import changes\n");
    ResetBlock();
    unsigned short goalReads[2] = { FieldIndex(ROK_FIXTUREGOALA, "ticks"), FieldIndex(ROK_FIXTUREGOALA, "heading") };
    unsigned short goalWrites[1] = { FieldIndex(ROK_FIXTUREGOALA, "heading") };
    ReplaySlotSets goalSets = { "goal", 0, nullptr, 0, nullptr, 0, goalReads, 2, goalWrites, 1 };
    const ReplaySlotSets* outerSets = g_sets;
    g_sets = &goalSets;
    bool outerImportChanges = g_importChanges;
    g_importChanges = true;
    LoadCall("  i fixture.MakeGoal = FixtureGoalA#1\n  o FixtureGoalA#1.ticks 9\n");
    BeginImport();
    FixtureGoalA* changed = (FixtureGoalA*)ReplayRefPtr(Imp_fixture_MakeGoal());
    Check(g_diffs == 0 && changed != nullptr && changed->ticks == 9, "the stub sets a field the slot reads from its line after the import");
    Check(ImportObjectProblem("FixtureGoalA#1.ticks", goalSets) == nullptr, "and the line is one it can place");
    const char* unparsed = ImportObjectProblem("FixtureGoalA#1.nosuchfield", goalSets);
    Check(unparsed != nullptr && strstr(unparsed, "names no object field") != nullptr, "a line that names no field of the kind is fatal");
    ReplaySlotSets ticksOnly = { "ticks", 0, nullptr, 0, nullptr, 0, goalReads, 1, nullptr, 0 };
    const char* unread = ImportObjectProblem("FixtureGoalA#1.heading", ticksOnly);
    Check(unread != nullptr && strstr(unread, "neither reads nor writes") != nullptr, "so is a line for a field the slot neither reads nor writes");
    Check(ImportObjectProblem("FixtureGoalA#1.heading", goalSets) == nullptr, "a line for a field the slot writes is one it can place too: the import changed it, and the w lines compare what the slot writes after");
    Check(changed != nullptr && changed->heading == 0, "none of which changes the object");

    printf("a written field that an import changes\n");
    g_known.Set("FixtureGoalA#1.heading", "5");
    changed->heading = 5;
    LoadCall("  i fixture.MakeGoal = FixtureGoalA#1\n  o FixtureGoalA#1.heading 7\n");
    BeginImport();
    Imp_fixture_MakeGoal();
    Check(g_diffs == 0 && changed->heading == 7, "the module's value before the import is the last known one, and the line sets the import's");
    changed->heading = 4;
    LoadCall("  i fixture.MakeGoal = FixtureGoalA#1\n  o FixtureGoalA#1.heading 9\n");
    BeginImport();
    Imp_fixture_MakeGoal();
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureGoalA#1.heading") == 0 && changed->heading == 9, "a wrong write that the import overwrites is a difference");
    changed->heading = 6;
    LoadCall("  i fixture.MakeGoal = FixtureGoalA#1\n  b FixtureGoalA#1.heading 6\n  o FixtureGoalA#1.heading 8\n");
    BeginImport();
    Imp_fixture_MakeGoal();
    Check(g_diffs == 0 && changed->heading == 8, "a b line gives the value the slot wrote in the call before the import");
    changed->heading = 3;
    LoadCall("  i fixture.MakeGoal = FixtureGoalA#1\n  b FixtureGoalA#1.heading 6\n  o FixtureGoalA#1.heading 8\n");
    BeginImport();
    Imp_fixture_MakeGoal();
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureGoalA#1.heading") == 0, "and a module that wrote another value differs from it");
    g_importChanges = outerImportChanges;
    g_sets = outerSets;
}

void RefsSelfTest()
{
    printf("an array of refs\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    FixtureManager* manager = (FixtureManager*)Fresh(sizeof(FixtureManager));
    FixtureTimer* timer = (FixtureTimer*)Fresh(sizeof(FixtureTimer));
    FixtureAssault* assault = (FixtureAssault*)Fresh(sizeof(FixtureAssault));
    SetVtable(timer, ReplayVtable(ROK_FIXTURETIMER));
    SetVtable(assault, ReplayVtable(ROK_FIXTUREASSAULT));
    timer->ticks = 5;
    timer->period = 9;
    assault->ticks = 7;
    assault->squad = 2;
    manager->id = 3;
    manager->timers[0] = timer;
    manager->timers[2] = assault;

    unsigned short kinds[1] = { ROK_FIXTUREMANAGER };
    void* roots[1] = { manager };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    char* first = strdup(recorder.text.Data());
    printf("%s", first);
    Check(ReplayFailure() == nullptr, "every element is a ref, null or an object of a kind of the base");
    Check(strstr(first, "  r FixtureManager#1.timers[0] FixtureTimer#1\n") != nullptr, "an element that is the base kind is written as that kind");
    Check(strstr(first, "  r FixtureManager#1.timers[2] FixtureAssault#1\n") != nullptr, "and one that is the derived kind as its own");
    Check(strstr(first, "  r FixtureManager#1.timers[1] null\n") != nullptr && strstr(first, "  r FixtureManager#1.timers[3] null\n") != nullptr, "a null element is written null");
    Check(CountLines(first, "  r FixtureManager#1.timers[") == 4 && strstr(first, "FixtureTimerBase#") == nullptr, "each element is a line, and the base is never written");
    Check(strstr(first, "  r FixtureTimer#1.period 9\n") != nullptr && strstr(first, "  r FixtureAssault#1.squad 2\n") != nullptr, "the walk reaches the objects the elements name");

    manager->timers[3] = timer;
    recorder.Record(kinds, roots, 1, sets);
    char* second = strdup(recorder.text.Data());
    Check(strcmp(second, "  r FixtureManager#1.timers[3] FixtureTimer#1\n") == 0, "an element that changed between calls is one line");

    ResetBlock();
    RestoreText(first, sets);
    RestoreText(second, sets);
    FixtureManager* rebuilt = (FixtureManager*)ReplayResolveObject("FixtureManager#1", nullptr, g_objects, AllocObject);
    FixtureTimer* rebuiltTimer = (FixtureTimer*)ReplayResolveObject("FixtureTimer#1", nullptr, g_objects, AllocObject);
    FixtureAssault* rebuiltAssault = (FixtureAssault*)ReplayResolveObject("FixtureAssault#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuiltTimer != nullptr && rebuiltAssault != nullptr, "the objects are rebuilt");
    if (rebuilt == nullptr || rebuiltTimer == nullptr || rebuiltAssault == nullptr) {
        free(first);
        free(second);
        return;
    }
    Check(rebuilt->timers[0].get() == (FixtureTimerBase*)rebuiltTimer && rebuilt->timers[2].get() == (FixtureTimerBase*)rebuiltAssault, "each element leads to its object");
    Check(rebuilt->timers[1].get() == nullptr && rebuilt->timers[3].get() == (FixtureTimerBase*)rebuiltTimer, "a null stays null, and the changed element is applied");
    Check(*(const void**)rebuiltTimer == ReplayVtable(ROK_FIXTURETIMER) && *(const void**)rebuiltAssault == ReplayVtable(ROK_FIXTUREASSAULT), "and each has the vtable of its kind");
    Check(rebuiltTimer->period == 9 && rebuiltAssault->squad == 2 && rebuilt->id == 3, "the fields are restored");
    g_rootKinds[0] = ROK_FIXTUREMANAGER;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored array compares equal");
    rebuilt->timers[1] = (FixtureTimerBase*)rebuiltAssault;
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed element: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureManager#1.timers[1]") == 0, "a changed element is reported at its index");
    rebuilt->timers[1] = (FixtureTimerBase*)nullptr;
    rebuiltAssault->squad = 4;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureAssault#1.squad") == 0, "and a field of an element's object at the object");
    free(first);
    free(second);
}

void BasedTableSelfTest()
{
    printf("a table behind a ref field\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    FixtureOwner* owner = (FixtureOwner*)Fresh(sizeof(FixtureOwner));
    FixtureOwner* other = (FixtureOwner*)Fresh(sizeof(FixtureOwner));
    FixtureGroup* groups = (FixtureGroup*)Fresh(4 * (unsigned)sizeof(FixtureGroup));
    FixtureGroup* otherGroups = (FixtureGroup*)Fresh(4 * (unsigned)sizeof(FixtureGroup));
    FixtureUser* user = (FixtureUser*)Fresh(sizeof(FixtureUser));
    FixtureUser* otherUser = (FixtureUser*)Fresh(sizeof(FixtureUser));
    owner->id = 1;
    owner->groups = groups;
    other->id = 2;
    other->groups = otherGroups;
    groups[2].level = 7;
    groups[0].mode = 3;
    otherGroups[1].level = 8;
    user->tag = 10;
    user->group = &groups[2];
    otherUser->tag = 11;
    otherUser->group = &otherGroups[1];

    unsigned short kinds[4] = { ROK_FIXTUREOWNER, ROK_FIXTUREOWNER, ROK_FIXTUREUSER, ROK_FIXTUREUSER };
    void* roots[4] = { owner, other, user, otherUser };
    Recorder recorder;
    recorder.Record(kinds, roots, 4, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(ReplayFailure() == nullptr, "the walk reaches the elements without a failure");
    Check(strstr(text, "  r FixtureUser#1.group FixtureOwner#1.groups[2]\n") != nullptr, "a ref into the table names the element by its owner's field and index");
    Check(strstr(text, "  r FixtureUser#2.group FixtureOwner#2.groups[1]\n") != nullptr, "and an element of another owner by that owner");
    Check(strstr(text, "  r FixtureOwner#1.groups[2].level 7\n") != nullptr && strstr(text, "  r FixtureOwner#2.groups[1].level 8\n") != nullptr, "the fields of an element are written under that name");
    Check(strstr(text, "  r FixtureOwner#1.groups FixtureOwner#1.groups[0]\n") != nullptr, "the owner's ref field is the first element");
    Check(strstr(text, "FixtureGroup#") == nullptr, "no element is named by order");

    ResetBlock();
    RestoreText(text, sets);
    FixtureOwner* rebuilt = (FixtureOwner*)ReplayResolveObject("FixtureOwner#1", nullptr, g_objects, AllocObject);
    FixtureOwner* rebuiltOther = (FixtureOwner*)ReplayResolveObject("FixtureOwner#2", nullptr, g_objects, AllocObject);
    FixtureUser* rebuiltUser = (FixtureUser*)ReplayResolveObject("FixtureUser#1", nullptr, g_objects, AllocObject);
    FixtureUser* rebuiltOtherUser = (FixtureUser*)ReplayResolveObject("FixtureUser#2", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuiltOther != nullptr && rebuiltUser != nullptr && rebuiltOtherUser != nullptr, "the objects are rebuilt");
    if (rebuilt == nullptr || rebuiltOther == nullptr || rebuiltUser == nullptr || rebuiltOtherUser == nullptr || rebuilt->groups.get() == nullptr || rebuiltOther->groups.get() == nullptr) {
        free(text);
        return;
    }
    FixtureGroup* array = rebuilt->groups.get();
    Check(rebuiltUser->group.get() == array + 2 && rebuiltOtherUser->group.get() == rebuiltOther->groups.get() + 1, "a ref into the table is restored to the right element of the right owner");
    Check(array != rebuiltOther->groups.get() && array[2].level == 7 && array[0].mode == 3 && rebuiltOther->groups.get()[1].level == 8, "each owner has an array of its own, with the fields in place");
    Check(ReplayResolveObject("FixtureOwner#1.groups[3]", nullptr, g_objects, AllocObject) == array + 3, "the whole array is allocated at first sight");
    Check(ReplayResolveObject("FixtureOwner#1.groups[4]", nullptr, g_objects, AllocObject) == nullptr, "and an index past it names nothing");
    unsigned found = 0;
    Check(ReplayResolveObject("FixtureOwner#1.groups[1]", &found, g_objects, AllocObject) == array + 1 && found == ROK_FIXTUREGROUP, "a name resolves to the element and its kind");
    g_rootKinds[0] = ROK_FIXTUREOWNER;
    g_rootKinds[1] = ROK_FIXTUREOWNER;
    g_rootKinds[2] = ROK_FIXTUREUSER;
    g_rootKinds[3] = ROK_FIXTUREUSER;
    g_rootObjects[0] = rebuilt;
    g_rootObjects[1] = rebuiltOther;
    g_rootObjects[2] = rebuiltUser;
    g_rootObjects[3] = rebuiltOtherUser;
    g_rootCount = 4;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored table compares equal");
    array[2].level = 6;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureOwner#1.groups[2].level") == 0, "a changed field is reported under the element's name");
    array[2].level = 7;
    rebuiltUser->group = &array[1];
    BeginCompare();
    CompareObjects(&sets);
    printf("a ref to another element: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs > 0 && strcmp(g_firstKey, "FixtureUser#1.group") == 0, "a ref that leads to another element is reported at the ref");
    free(text);
}

void ElementSiteSelfTest()
{
    printf("an object at an array element's site, with a vptr set by hand\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    FixtureRig* rig = (FixtureRig*)Fresh(sizeof(FixtureRig));
    rig->id = 5;
    for (int i = 0; i < 3; i++)
        rig->slots[i].cb.ppVtbl = (void**)ReplayVtable(ROK_FIXTURECB);
    rig->slots[1].cb.done = 6;
    rig->slots[2].cb.done = 1;
    unsigned short kinds[1] = { ROK_FIXTURERIG };
    void* roots[1] = { rig };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(ReplayFailure() == nullptr, "the vptr of each element is the vtable of the kind");
    Check(strstr(text, "  r FixtureRig#1.slots[0].cb.done 0\n") != nullptr && strstr(text, "  r FixtureRig#1.slots[1].cb.done 6\n") != nullptr && strstr(text, "  r FixtureRig#1.slots[2].cb.done 1\n") != nullptr, "the walk names each element's object by its site and index");
    Check(strstr(text, "FixtureCb#") == nullptr && strstr(text, "ppVtbl") == nullptr && strstr(text, "vptr") == nullptr, "no object is named by order, and no vptr is written");

    ReplayText name;
    ReplayAddObjectName(name, *recorder.table, ROK_FIXTURECB, &rig->slots[1].cb, true);
    Check(strcmp(name.Data(), "FixtureRig#1.slots[1].cb") == 0, "an argument at an element's site is named by it");
    name.Clear();
    ReplayAddObjectName(name, *recorder.table, ROK_FIXTURECB, &rig->slots[2].cb, true);
    Check(strcmp(name.Data(), "FixtureRig#1.slots[2].cb") == 0, "and another element by its own index");
    FixtureCb* loose = (FixtureCb*)Fresh(sizeof(FixtureCb));
    loose->ppVtbl = (void**)ReplayVtable(ROK_FIXTURECB);
    name.Clear();
    ReplayAddObjectName(name, *recorder.table, ROK_FIXTURECB, loose, true);
    Check(strcmp(name.Data(), "FixtureCb#1") == 0, "an object of the kind that is at no site is named by order");
    name.Clear();

    ResetBlock();
    RestoreText(text, sets);
    FixtureRig* rebuilt = (FixtureRig*)ReplayResolveObject("FixtureRig#1", nullptr, g_objects, AllocObject);
    unsigned kind = 0;
    FixtureCb* cb = (FixtureCb*)ReplayResolveObject("FixtureRig#1.slots[1].cb", &kind, g_objects, AllocObject);
    Check(rebuilt != nullptr && cb == &rebuilt->slots[1].cb && kind == ROK_FIXTURECB, "a name of an element's site resolves to the bytes inside the element");
    if (rebuilt == nullptr || cb == nullptr) {
        free(text);
        name.Free();
        return;
    }
    Check(rebuilt->slots[1].cb.done == 6 && rebuilt->slots[2].cb.done == 1 && rebuilt->id == 5, "the fields are restored in place");
    Check((const void*)rebuilt->slots[0].cb.ppVtbl == ReplayVtable(ROK_FIXTURECB) && (const void*)rebuilt->slots[2].cb.ppVtbl == ReplayVtable(ROK_FIXTURECB), "each element gets the replayer's copy of the vtable");
    Check(ReplayResolveObject("FixtureRig#1.slots[3].cb", nullptr, g_objects, AllocObject) == nullptr, "an index past the array names nothing");
    Check(g_objects.Pointer(ROK_FIXTURECB, 1) == nullptr, "no separate object of the kind is allocated");
    g_rootKinds[0] = ROK_FIXTURERIG;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    printf("%s", g_detail.Data());
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored rig compares equal");
    rebuilt->slots[2].cb.done = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureRig#1.slots[2].cb.done") == 0, "a changed field is reported at its element");

    static void* stray[2];
    rebuilt->slots[1].cb.ppVtbl = (void**)stray;
    ReplayClearFailure();
    Recorder wrong;
    wrong.Record(kinds, (void* const*)&rebuilt, 1, sets);
    printf("a vptr that is not the global: %s\n", ReplayFailure() == nullptr ? "(none)" : ReplayFailure());
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "FixtureCb has a vptr that is not g_fixtureCbVtable") != nullptr, "a word that is not the global's address is a failure that names the global");
    ReplayClearFailure();

    rebuilt->slots[1].cb.ppVtbl = nullptr;
    Recorder unset;
    unset.Record(kinds, (void* const*)&rebuilt, 1, sets);
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "FixtureCb has a vptr that is not g_fixtureCbVtable") != nullptr, "a word of 0 is a failure for a kind without vptr_unset");
    ReplayClearFailure();
    WeaponAimCobCb notYetSet;
    memset(&notYetSet, 0, sizeof notYetSet);
    Check(ReplayConcreteKind(ROK_WEAPONAIMCOBCB, &notYetSet) == ROK_WEAPONAIMCOBCB && ReplayFailure() == nullptr, "and an object not set up yet for a kind with vptr_unset");
    static void* other[2];
    notYetSet.ppVtbl = (void**)other;
    Check(ReplayConcreteKind(ROK_WEAPONAIMCOBCB, &notYetSet) == kReplayObjectKindCount && ReplayFailure() != nullptr, "whose other wrong words still fail");
    ReplayClearFailure();
    free(text);
}

void EmbedsSelfTest()
{
    printf("an array of embedded structs\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    FixtureScript* script = (FixtureScript*)Fresh(sizeof(FixtureScript));
    script->id = 9;
    script->channels[0].pc = 3;
    script->channels[2].pc = 7;
    script->channels[2].state = -2;
    script->channels[3].stack = 99;
    unsigned short kinds[1] = { ROK_FIXTURESCRIPT };
    void* roots[1] = { script };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    char* first = strdup(recorder.text.Data());
    Check(ReplayFailure() == nullptr, "the elements are embedded objects of the kind");
    Check(strstr(first, "  r FixtureScript#1.channels[2].pc 7\n") != nullptr && strstr(first, "  r FixtureScript#1.channels[2].state -2\n") != nullptr, "a field of an element is written under the element's index");
    Check(CountLines(first, "  r FixtureScript#1.channels[") == 12 && strstr(first, "FixtureChannel#") == nullptr, "every field of every element is written once, and none is named by order");

    script->channels[1].pc = 4;
    recorder.Record(kinds, roots, 1, sets);
    char* second = strdup(recorder.text.Data());
    Check(strcmp(second, "  r FixtureScript#1.channels[1].pc 4\n") == 0, "a changed element is written field by field, and only that field");

    ResetBlock();
    RestoreText(first, sets);
    RestoreText(second, sets);
    FixtureScript* rebuilt = (FixtureScript*)ReplayResolveObject("FixtureScript#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->id == 9 && rebuilt->channels[0].pc == 3 && rebuilt->channels[1].pc == 4 && rebuilt->channels[2].state == -2 && rebuilt->channels[3].stack == 99, "the elements are restored in place");
    Check(rebuilt != nullptr && ReplayResolveObject("FixtureScript#1.channels[3]", nullptr, g_objects, AllocObject) == &rebuilt->channels[3], "an element's name resolves to its bytes");
    if (rebuilt == nullptr) {
        free(first);
        free(second);
        return;
    }
    g_rootKinds[0] = ROK_FIXTURESCRIPT;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    printf("%s", g_detail.Data());
    Check(g_diffs == 0, "the restored script compares equal");
    rebuilt->channels[2].pc = 8;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureScript#1.channels[2].pc") == 0, "a changed field is reported at its element");
    free(first);
    free(second);
}

void CountedEmbedsSelfTest()
{
    printf("an array of embedded structs counted by a member, and an array measured by a load\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned short fields[5] = { FieldIndex(ROK_FIXTURETRACK, "count"), FieldIndex(ROK_FIXTURETRACK, "marks"), FieldIndex(ROK_FIXTURETRACK, "laps"),
                                 FieldIndex(ROK_FIXTURELAP, "time"), FieldIndex(ROK_FIXTURELAP, "mark") };
    ReplaySlotSets sets = { "track", 0, nullptr, 0, nullptr, 0, fields, 5, fields, 5 };
    FixtureFile* file = (FixtureFile*)Fresh(sizeof(FixtureFile));
    file->lapCount = 3;
    FixtureTrack* track = (FixtureTrack*)Fresh(sizeof(FixtureTrack) + 4 * sizeof(FixtureLap));
    track->count = 3;
    track->table = file;
    unsigned char* marks = (unsigned char*)Fresh(6);
    for (int i = 0; i < 6; i++)
        marks[i] = (unsigned char)(10 + i);
    track->marks = marks;
    track->laps[0].time = 5;
    track->laps[1].time = 6;
    track->laps[2].time = 7;
    track->laps[2].mark = -4;
    track->laps[3].time = 99;
    unsigned short kinds[1] = { ROK_FIXTURETRACK };
    void* roots[1] = { track };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(ReplayFailure() == nullptr, "a count within the capacity is recorded");
    Check(strstr(text, "  r FixtureTrack#1.laps[2].time 7\n") != nullptr && strstr(text, "  r FixtureTrack#1.laps[2].mark -4\n") != nullptr, "the elements up to the count are written under their index");
    Check(strstr(text, "laps[3]") == nullptr && strstr(text, "FixtureLap#") == nullptr, "an element past the count is not, and none is named by order");
    Check(strstr(text, "  r FixtureTrack#1.marks [6] 0:0a0b0c0d0e0f\n") != nullptr, "an array whose length is a member of the file its load field leads to has that many bytes");

    ResetBlock();
    RestoreText(text, sets);
    FixtureTrack* rebuilt = (FixtureTrack*)ReplayResolveObject("FixtureTrack#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->count == 3 && rebuilt->laps[2].time == 7 && rebuilt->laps[2].mark == -4 && rebuilt->laps[0].time == 5, "the elements are restored in place");
    Check(rebuilt != nullptr && ReplayResolveObject("FixtureTrack#1.laps[4]", nullptr, g_objects, AllocObject) == &rebuilt->laps[4], "the owner has room for the capacity, so the last element has an address");
    Check(ReplayResolveObject("FixtureTrack#1.laps[5]", nullptr, g_objects, AllocObject) == nullptr, "and a name past the capacity names nothing");
    if (rebuilt == nullptr) {
        free(text);
        return;
    }
    FixtureFile* again = (FixtureFile*)Fresh(sizeof(FixtureFile));
    again->lapCount = 3;
    rebuilt->table = again;
    g_rootKinds[0] = ROK_FIXTURETRACK;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    printf("%s", g_detail.Data());
    Check(g_diffs == 0, "the restored track compares equal");
    rebuilt->laps[1].time = 8;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureTrack#1.laps[1].time") == 0, "a changed field is reported at its element");
    rebuilt->laps[1].time = 6;
    rebuilt->count = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureTrack#1.count") == 0, "a count that changed is reported, and the elements past it are not compared");
    rebuilt->count = 3;
    again->lapCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs > 0 && strncmp(g_firstKey, "FixtureTrack#1.marks", 20) == 0, "an array is as long as the file the replayer loaded says");
    free(text);

    ResetBlock();
    ReplayClearFailure();
    FixtureTrack* full = (FixtureTrack*)Fresh(sizeof(FixtureTrack) + 4 * sizeof(FixtureLap));
    full->count = 6;
    roots[0] = full;
    Recorder over;
    over.Record(kinds, roots, 1, sets);
    printf("%s\n", ReplayFailure() == nullptr ? "no failure" : ReplayFailure());
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "FixtureTrack.laps has 6 elements, past the capacity of 5 that the schema gives") != nullptr, "a count above the capacity fails the recording");
    ReplayClearFailure();
}

void EmbeddedPolymorphicSelfTest()
{
    printf("an embedded polymorphic object\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned count = 0;
    const unsigned short* all = AllStateFields(&count);
    ReplaySlotSets sets = { "all", 0, nullptr, 0, nullptr, 0, all, count, all, count };
    FixtureOrder* first = (FixtureOrder*)Fresh(sizeof(FixtureOrder));
    FixtureOrder* second = (FixtureOrder*)Fresh(sizeof(FixtureOrder));
    SetVtable(&first->link, ReplayVtable(ROK_FIXTUREATTACH));
    SetVtable(&second->link, ReplayVtable(ROK_FIXTUREATTACH));
    first->id = 1;
    second->id = 2;
    first->link.owner = first;
    first->link.next = &second->link;
    first->link.value = 3;
    second->link.owner = second;
    second->link.value = 4;
    unsigned short kinds[2] = { ROK_FIXTUREORDER, ROK_FIXTUREORDER };
    void* roots[2] = { first, second };
    Recorder recorder;
    recorder.Record(kinds, roots, 2, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(ReplayFailure() == nullptr, "each vptr is the vtable of the kind");
    Check(strstr(text, "  r FixtureOrder#1.link.owner FixtureOrder#1\n") != nullptr && strstr(text, "  r FixtureOrder#1.link.value 3\n") != nullptr, "the walk names the embedded object by its site");
    Check(strstr(text, "  r FixtureOrder#1.link.next FixtureOrder#2.link\n") != nullptr, "and a pointer to another one by that one's site");
    Check(strstr(text, "FixtureAttach#") == nullptr && strstr(text, "vtable") == nullptr, "no object is named by order and no vtable is written");
    ReplayText name;
    ReplayAddObjectName(name, *recorder.table, ROK_FIXTUREATTACH, &second->link, true);
    Check(strcmp(name.Data(), "FixtureOrder#2.link") == 0, "an argument that is the embedded object is named by its site");
    name.Free();

    ResetBlock();
    RestoreText(text, sets);
    FixtureOrder* rebuilt = (FixtureOrder*)ReplayResolveObject("FixtureOrder#1", nullptr, g_objects, AllocObject);
    FixtureOrder* rebuiltSecond = (FixtureOrder*)ReplayResolveObject("FixtureOrder#2", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuiltSecond != nullptr, "the orders are rebuilt");
    if (rebuilt == nullptr || rebuiltSecond == nullptr) {
        free(text);
        return;
    }
    Check(rebuilt->link.next.get() == &rebuiltSecond->link && rebuilt->link.owner.get() == rebuilt && rebuilt->link.value == 3, "the fields and the pointers between the sites are restored");
    Check(*(const void**)&rebuilt->link == ReplayVtable(ROK_FIXTUREATTACH), "the embedded object gets the vtable of the replayer");
    g_rootKinds[0] = ROK_FIXTUREORDER;
    g_rootKinds[1] = ROK_FIXTUREORDER;
    g_rootObjects[0] = rebuilt;
    g_rootObjects[1] = rebuiltSecond;
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored orders compare equal");
    rebuiltSecond->link.value = 5;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureOrder#2.link.value") == 0, "a changed field is reported at the site");
    rebuiltSecond->link.value = 4;
    static void* stray[2];
    SetVtable(&rebuiltSecond->link, stray);
    ReplayClearFailure();
    BeginCompare();
    CompareObjects(&sets);
    printf("a vtable of another kind: %s\n", ReplayFailure() == nullptr ? "(none)" : ReplayFailure());
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "FixtureAttach has a vptr that is not g_fixtureAttachVtable") != nullptr, "a vptr that is not the kind's vtable is a failure");
    ReplayClearFailure();
    free(text);
}

unsigned OffsetOf(const void* p)
{
    return (unsigned)((const char*)p - (const char*)SimBlockBase());
}

FixtureEye* MakeEyes(unsigned live)
{
    FixtureEye* eyes = (FixtureEye*)operator new(4 * (unsigned)sizeof(FixtureEye));
    memset((void*)eyes, 0, 4 * sizeof(FixtureEye));
    for (unsigned i = 0; i < 4; i++) {
        eyes[i].screen.offset = OffsetOf(&eyes[i].pos);
        eyes[i].flagPtr.offset = OffsetOf(&eyes[i].flag);
    }
    g_game->hotRadar.offset = OffsetOf(eyes);
    g_game->hotRadarCount = (int)live;
    return eyes;
}

void CapacityTableSelfTest()
{
    printf("a table with a live count and a capacity\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureView* view = (FixtureView*)Fresh(sizeof(FixtureView));
    view->flags = 4;
    FixtureEye* eyes = MakeEyes(2);
    eyes[0].owner.offset = OffsetOf(view);
    eyes[0].x = 1;
    eyes[1].x = 2;
    eyes[3].x = 9;
    eyes[3].pos.x = 5;
    eyes[3].pos.y = 6;
    unsigned short fields[7] = { FieldIndex(ROK_FIXTUREEYE, "owner"), FieldIndex(ROK_FIXTUREEYE, "screen"), FieldIndex(ROK_FIXTUREEYE, "x"), FieldIndex(ROK_FIXTUREEYE, "flag"),
                                 FieldIndex(ROK_FIXTUREEYE, "flagPtr"), FieldIndex(ROK_FIXTUREEYE, "pos"), FieldIndex(ROK_FIXTUREVIEW, "flags") };
    unsigned short every[1] = { ROK_FIXTUREEYE };
    ReplaySlotSets sets = { "capacity", 0, nullptr, 0, nullptr, 0, fields, 7, fields, 7, every, 1 };
    unsigned short kinds[1] = { ROK_FIXTUREEYE };
    void* roots[1] = { &eyes[3] };
    Recorder recorder;
    recorder.Record(kinds, roots, 1, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(ReplayFailure() == nullptr, "the walk reaches the elements without a failure");
    Check(strstr(text, "  r FixtureEye#3.x 9\n") != nullptr && strstr(text, "  r FixtureEye#3.pos (5,6)\n") != nullptr, "an element past the live count is named by its index");
    Check(strstr(text, "FixtureEye#?") == nullptr && strstr(text, "FixtureEye#2") == nullptr, "and no pointer into the table is unnamed");
    Check(CountLines(text, "  r FixtureEye#") == 12, "the all-of walk records the live elements, and the root the one past them");
    Check(strstr(text, "  r FixtureEye#0.owner FixtureView#1\n") != nullptr && strstr(text, "  r FixtureView#1.flags 4\n") != nullptr, "a ref with a ctype is written as any ref is");
    Check(strstr(text, "screen") == nullptr && strstr(text, "flagPtr") == nullptr, "a self pointer is never a line");

    ResetBlock();
    Check(RestoreField(kReplayFields[RF_GAME_HOTRADARCOUNT], "2"), "the live count restores as a recorded field");
    RestoreText(text, sets);
    FixtureEye* rebuilt = (FixtureEye*)g_game->hotRadar.get();
    Check(rebuilt != nullptr, "the table is allocated when a line first names an element");
    if (rebuilt == nullptr) {
        free(text);
        return;
    }
    Check(g_game->hotRadarCount == 2, "the replayer stores nothing into the live count");
    Check(!SimOwns(rebuilt), "the table is the module's own memory, from its operator new");
    Check(rebuilt[3].x == 9 && rebuilt[3].pos.x == 5 && rebuilt[3].pos.y == 6 && rebuilt[0].x == 1 && rebuilt[1].x == 2, "the elements are restored, past the live count too");
    Check(rebuilt[0].owner.get() == (char*)ReplayResolveObject("FixtureView#1", nullptr, g_objects, AllocObject), "the ref with a ctype leads to its object");
    unsigned found = 0;
    Check(ReplayResolveObject("FixtureEye#3", &found, g_objects, AllocObject) == &rebuilt[3] && found == ROK_FIXTUREEYE, "an index below the capacity names its element");
    Check(ReplayResolveObject("FixtureEye#4", nullptr, g_objects, AllocObject) == nullptr, "and one past the capacity names nothing");
    g_rootKinds[0] = ROK_FIXTUREEYE;
    g_rootObjects[0] = &rebuilt[3];
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored table compares equal");
    rebuilt[3].x = 8;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureEye#3.x") == 0, "a change past the live count is reported at its element");
    rebuilt[3].x = 9;
    if (!SimOwns(rebuilt)) {
        operator delete(rebuilt);
        g_game->hotRadar.offset = 0;
    }
    free(text);
}

void TrackedCountSelfTest()
{
    printf("a table with a tracked count and a recorded alloc count\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureTally* tally = (FixtureTally*)Fresh(6 * sizeof(FixtureTally));
    for (int i = 0; i < 6; i++)
        tally[i].value = 10 * (i + 1);
    g_game->list_1435f.offset = OffsetOf(tally);
    g_game->count_14367 = 3;
    unsigned short fields[1] = { FieldIndex(ROK_FIXTURETALLY, "value") };
    unsigned char reads[1] = { RF_GAME_COUNT_14367 };
    unsigned short every[1] = { ROK_FIXTURETALLY };
    ReplaySlotSets tracked = { "tracked", 0, reads, 1, nullptr, 0, fields, 1, fields, 1, every, 1 };
    Recorder recorder;
    recorder.Record(nullptr, nullptr, 0, tracked);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(CountLines(text, "  r FixtureTally#") == 3, "the all-of walk records the live elements");

    ResetBlock();
    Check(RestoreField(kReplayFields[RF_GAME_COUNT_14367], "3"), "the tracked count restores as recorded data");
    RestoreText(text, tracked);
    FixtureTally* rebuilt = (FixtureTally*)g_game->list_1435f.get();
    Check(rebuilt != nullptr && g_game->count_14367 == 3, "the table is allocated and the replayer leaves the restored count");
    Check(rebuilt != nullptr && rebuilt[2].value == 30, "the elements are restored");
    unsigned found = 0;
    Check(ReplayResolveObject("FixtureTally#3", &found, g_objects, AllocObject) == (rebuilt == nullptr ? nullptr : &rebuilt[3]), "the element one past the tracked count is named");
    Check(ReplayResolveObject("FixtureTally#4", nullptr, g_objects, AllocObject) == nullptr, "and one past that names nothing");

    ResetBlock();
    ReplaySlotSets plain = { "plain", 0, nullptr, 0, nullptr, 0, fields, 1, fields, 1, every, 1 };
    RestoreText("  r FixtureTally#5.value 60\n", plain);
    rebuilt = (FixtureTally*)g_game->list_1435f.get();
    Check(rebuilt != nullptr && rebuilt[5].value == 60 && g_game->count_14367 == 6 && ReplayResolveObject("FixtureTally#5", nullptr, g_objects, AllocObject) == &rebuilt[5], "a recording that does not track the count gets the alloc count, which sizes the elements it names");
    free(text);
}

void SelfPointerSelfTest()
{
    printf("pointers into an object's own members\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureEye* eyes = MakeEyes(2);
    eyes[0].x = 1;
    eyes[1].x = 2;
    eyes[1].flag = 7;
    eyes[1].pos.x = 3;
    unsigned short fields[5] = { FieldIndex(ROK_FIXTUREEYE, "screen"), FieldIndex(ROK_FIXTUREEYE, "x"), FieldIndex(ROK_FIXTUREEYE, "flag"), FieldIndex(ROK_FIXTUREEYE, "flagPtr"),
                                 FieldIndex(ROK_FIXTUREEYE, "pos") };
    unsigned short every[1] = { ROK_FIXTUREEYE };
    ReplaySlotSets sets = { "self", 0, nullptr, 0, nullptr, 0, fields, 5, fields, 5, every, 1 };
    Recorder recorder;
    recorder.Record(nullptr, nullptr, 0, sets);
    char* first = strdup(recorder.text.Data());
    Check(CountLines(first, "  r FixtureEye#") == 6 && strstr(first, "screen") == nullptr && strstr(first, "flagPtr") == nullptr, "a self pointer to a member is no line");

    eyes[0] = eyes[1];
    Check(eyes[0].screen.get() == &eyes[1].pos && eyes[0].flagPtr.get() == &eyes[1].flag, "an assignment copies the pointers, which now lead into the other element");
    recorder.Record(nullptr, nullptr, 0, sets);
    char* second = strdup(recorder.text.Data());
    printf("%s", second);
    Check(strstr(second, "  r FixtureEye#0.x 2\n") != nullptr && strstr(second, "  r FixtureEye#0.flag 7\n") != nullptr && CountLines(second, "  r ") == 3, "only the data that moved is recorded");

    ResetBlock();
    Check(RestoreField(kReplayFields[RF_GAME_HOTRADARCOUNT], "2"), "the count restores");
    RestoreText(first, sets);
    FixtureEye* rebuilt = (FixtureEye*)g_game->hotRadar.get();
    Check(rebuilt != nullptr, "the table is rebuilt");
    if (rebuilt == nullptr) {
        free(first);
        free(second);
        return;
    }
    Check(rebuilt[0].screen.get() == &rebuilt[0].pos && rebuilt[0].flagPtr.get() == &rebuilt[0].flag, "the replayer sets each pointer to the member of its own object");
    Check(rebuilt[1].screen.get() == &rebuilt[1].pos && rebuilt[1].flagPtr.get() == &rebuilt[1].flag, "in every element it names");
    rebuilt[0] = rebuilt[1];
    Check(rebuilt[0].screen.get() == &rebuilt[1].pos, "the replayed module moves an element by assignment, and the pointer leads into the other one");
    RestoreText(second, sets);
    Check(rebuilt[0].screen.get() == &rebuilt[0].pos && rebuilt[0].flagPtr.get() == &rebuilt[0].flag && rebuilt[0].x == 2 && rebuilt[0].flag == 7, "the next call's lines name the element again, and set its pointers");
    g_rootCount = 0;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the self pointers are neither compared nor a difference");
    if (!SimOwns(rebuilt)) {
        operator delete(rebuilt);
        g_game->hotRadar.offset = 0;
    }
    free(first);
    free(second);

    printf("an index into the object's own array\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureHeap* heap = (FixtureHeap*)Fresh(sizeof(FixtureHeap));
    FixtureNode* nodes = (FixtureNode*)Fresh(4 * (unsigned)sizeof(FixtureNode));
    for (int i = 0; i < 4; i++) {
        nodes[i].key = 10 * i;
        nodes[i].pri = (short)i;
    }
    heap->capacity = 4;
    heap->used = 3;
    heap->pool = nodes;
    heap->top = &nodes[2];
    unsigned short heapFields[6] = { FieldIndex(ROK_FIXTUREHEAP, "capacity"), FieldIndex(ROK_FIXTUREHEAP, "used"), FieldIndex(ROK_FIXTUREHEAP, "pool"), FieldIndex(ROK_FIXTUREHEAP, "top"),
                                     FieldIndex(ROK_FIXTURENODE, "key"), FieldIndex(ROK_FIXTURENODE, "pri") };
    ReplaySlotSets heapSets = { "heap", 0, nullptr, 0, nullptr, 0, heapFields, 6, heapFields, 6 };
    unsigned short kinds[2] = { ROK_FIXTUREHEAP, ROK_FIXTURENODE };
    void* roots[2] = { heap, &nodes[2] };
    Recorder heapRecorder;
    heapRecorder.Record(kinds, roots, 2, heapSets);
    char* one = strdup(heapRecorder.text.Data());
    printf("%s", one);
    Check(strstr(one, "  r FixtureHeap#1.top 2\n") != nullptr, "a pointer into the array is written as its index");
    Check(strstr(one, "  r FixtureHeap#1.pool[2].key 20\n") != nullptr, "and the element it leads to under its own name");
    heap->top = &nodes[3];
    heapRecorder.Record(kinds, roots, 2, heapSets);
    char* two = strdup(heapRecorder.text.Data());
    Check(strcmp(two, "  r FixtureHeap#1.top 3\n") == 0, "a change of the pointer is one line");
    FixtureNode* grown = (FixtureNode*)Fresh(8 * (unsigned)sizeof(FixtureNode));
    memcpy((void*)grown, nodes, 4 * sizeof(FixtureNode));
    heap->pool = grown;
    heap->capacity = 8;
    heap->top = &grown[3];
    roots[1] = &grown[2];
    heapRecorder.Record(kinds, roots, 2, heapSets);
    char* three = strdup(heapRecorder.text.Data());
    Check(strcmp(three, "  r FixtureHeap#1.capacity 8\n") == 0, "an array that moves with its pointer into it records no line for the pointer");

    ResetBlock();
    RestoreText(one, heapSets);
    FixtureHeap* back = (FixtureHeap*)ReplayResolveObject("FixtureHeap#1", nullptr, g_objects, AllocObject);
    Check(back != nullptr && back->pool.get() != nullptr, "the heap is rebuilt");
    if (back == nullptr || back->pool.get() == nullptr) {
        free(one);
        free(two);
        free(three);
        return;
    }
    Check(!SimOwns(back->pool.get()) && back->top.get() == back->pool.get() + 2 && back->pool.get()[2].key == 20, "the index is the element of the array the replayer made");
    RestoreText(two, heapSets);
    RestoreText(three, heapSets);
    Check(back->top.get() == back->pool.get() + 3 && back->capacity == 8, "a later index replaces it");
    g_rootKinds[0] = ROK_FIXTUREHEAP;
    g_rootObjects[0] = back;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&heapSets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored heap compares equal");
    back->top.offset = OffsetOf(back->pool.get() + 1);
    BeginCompare();
    CompareObjects(&heapSets);
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureHeap#1.top") == 0 && strstr(g_detail.Data(), "recorded 3, replayed 1") != nullptr, "a pointer to another element is reported at the field, as indexes");
    back->top.offset = OffsetOf(back->pool.get() + 3);
    Check(!ReplayRestoreObjectField(*ReplayFindObjectField(ROK_FIXTUREHEAP, "top"), back, "?", g_objects, nullptr), "a pointer that no index names is refused");
    if (!SimOwns(back->pool.get())) {
        operator delete(back->pool.get());
        back->pool.offset = 0;
    }
    free(one);
    free(two);
    free(three);
}

char* BlockText(const char* text)
{
    char* p = (char*)SimAlloc((unsigned)strlen(text) + 1);
    strcpy(p, text);
    return p;
}

void CStringSelfTest()
{
    printf("strings behind pointers\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureNamed* named = (FixtureNamed*)Fresh(sizeof(FixtureNamed));
    new (&named->list) BlockVector<char*>();
    new (&named->idents) BlockVector<Ref<char> >();
    named->count = 2;
    named->name = BlockText("alpha");
    named->ident = BlockText("tag \"1\"\n");
    char** words = (char**)Fresh(2 * (unsigned)sizeof(char*));
    words[0] = BlockText("one");
    words[1] = BlockText("two");
    named->words = words;
    Ref<char>* refWords = (Ref<char>*)Fresh(2 * (unsigned)sizeof(Ref<char>));
    refWords[0] = BlockText("r0");
    refWords[1] = BlockText("r1");
    named->refWords = refWords;
    named->fixed[0] = BlockText("f0");
    named->fixed[1] = BlockText("f1");
    named->list.PushBack(BlockText("l0"));
    named->list.PushBack(BlockText("l1"));
    named->idents.PushBack(Ref<char>(BlockText("i0")));
    static const char* const fieldNames[8] = { "count", "name", "ident", "words", "refWords", "fixed", "list", "idents" };
    unsigned short fields[8];
    for (int i = 0; i < 8; i++)
        fields[i] = FieldIndex(ROK_FIXTURENAMED, fieldNames[i]);
    ReplaySlotSets sets = { "named", 0, nullptr, 0, nullptr, 0, fields, 8, fields, 8 };
    unsigned short kind = ROK_FIXTURENAMED;
    void* root = named;
    Recorder recorder;
    recorder.Record(&kind, &root, 1, sets);
    char* one = strdup(recorder.text.Data());
    printf("%s", one);
    Check(strstr(one, "  r FixtureNamed#1.name \"alpha\"\n") != nullptr, "a char* is its text");
    Check(strstr(one, "  r FixtureNamed#1.ident \"tag \\\"1\\\"\\n\"\n") != nullptr, "a Ref<char> is its text, with C escapes");
    Check(strstr(one, "  r FixtureNamed#1.words [2]\n  r FixtureNamed#1.words[0] \"one\"\n  r FixtureNamed#1.words[1] \"two\"\n") != nullptr, "an array of strings is its length and a line per element");
    Check(strstr(one, "  r FixtureNamed#1.refWords [2]\n  r FixtureNamed#1.refWords[0] \"r0\"\n  r FixtureNamed#1.refWords[1] \"r1\"\n") != nullptr && strstr(one, "  r FixtureNamed#1.fixed [2]\n  r FixtureNamed#1.fixed[0] \"f0\"\n") != nullptr, "whether it is held by a Ref, a pointer or in the object");
    Check(strstr(one, "  r FixtureNamed#1.list [2/2]\n  r FixtureNamed#1.list[0] \"l0\"\n  r FixtureNamed#1.list[1] \"l1\"\n") != nullptr, "a vector of strings is the bracket and a line per element");
    Check(strstr(one, "  r FixtureNamed#1.idents [1/1]\n  r FixtureNamed#1.idents[0] \"i0\"\n") != nullptr, "and so is one of Ref<char>");

    named->name = BlockText("beta");
    words[1] = BlockText("TWO");
    named->list.PushBack(BlockText("l2"));
    recorder.Record(&kind, &root, 1, sets);
    char* two = strdup(recorder.text.Data());
    printf("%s", two);
    Check(strcmp(two, "  r FixtureNamed#1.name \"beta\"\n  r FixtureNamed#1.words[1] \"TWO\"\n  r FixtureNamed#1.list [3/4]\n  r FixtureNamed#1.list[2] \"l2\"\n") == 0, "a string that changed between calls is one line, and a vector that grew writes only its new element");
    recorder.Record(&kind, &root, 1, sets);
    Check(recorder.text.Size() == 0, "strings that did not change are not written again");
    named->name = nullptr;
    named->ident = nullptr;
    named->words = nullptr;
    recorder.Record(&kind, &root, 1, sets);
    char* three = strdup(recorder.text.Data());
    Check(strcmp(three, "  r FixtureNamed#1.name null\n  r FixtureNamed#1.ident null\n  r FixtureNamed#1.words null\n") == 0, "a null pointer is null");

    ResetBlock();
    RestoreText(one, sets);
    RestoreText(two, sets);
    FixtureNamed* rebuilt = (FixtureNamed*)ReplayResolveObject("FixtureNamed#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->name != nullptr && strcmp(rebuilt->name, "beta") == 0, "the replayer allocates the string, with the text of the last line");
    Check(rebuilt != nullptr && strcmp(rebuilt->ident.get(), "tag \"1\"\n") == 0, "and the string behind a Ref");
    Check(rebuilt != nullptr && rebuilt->words != nullptr && strcmp(rebuilt->words[0], "one") == 0 && strcmp(rebuilt->words[1], "TWO") == 0, "the array of strings");
    Check(rebuilt != nullptr && strcmp(rebuilt->refWords.get()[1].get(), "r1") == 0 && strcmp(rebuilt->fixed[1], "f1") == 0 && rebuilt->fixed[2] == nullptr, "by a Ref and in the object");
    Check(rebuilt != nullptr && rebuilt->list.Size() == 3 && rebuilt->list.Capacity() == 4 && strcmp(rebuilt->list[0], "l0") == 0 && strcmp(rebuilt->list[2], "l2") == 0, "and the vector, its old elements carried over its growth");
    Check(rebuilt != nullptr && rebuilt->idents.Size() == 1 && strcmp(rebuilt->idents[0].get(), "i0") == 0, "and the vector of Ref<char>");

    Recorder replayed;
    void* rebuiltRoot = rebuilt;
    replayed.Record(&kind, &rebuiltRoot, 1, sets);
    Check(strcmp(replayed.text.Data(), one) != 0 && strstr(replayed.text.Data(), "  r FixtureNamed#1.name \"beta\"\n") != nullptr && strstr(replayed.text.Data(), "  r FixtureNamed#1.list [3/4]\n") != nullptr, "the rebuilt object records as it was");

    g_rootKinds[0] = ROK_FIXTURENAMED;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored strings compare equal");
    rebuilt->name[0] = 'B';
    rebuilt->words[1][0] = 'W';
    rebuilt->list[1] = rebuilt->list[0];
    rebuilt->idents[0] = Ref<char>(rebuilt->name);
    BeginCompare();
    CompareObjects(&sets);
    printf("changed strings: %s\n%s", g_firstKey, g_detail.Data());
    Check(strstr(g_detail.Data(), "FixtureNamed#1.name: recorded \"beta\", replayed \"Beta\"") != nullptr, "a changed string is reported at the field");
    Check(strstr(g_detail.Data(), "FixtureNamed#1.words[1]: recorded \"TWO\", replayed \"WWO\"") != nullptr, "an element of an array at its index");
    Check(strstr(g_detail.Data(), "FixtureNamed#1.list[1]: recorded \"l1\", replayed \"l0\"") != nullptr && strstr(g_detail.Data(), "FixtureNamed#1.idents[0]: recorded \"i0\", replayed \"Beta\"") != nullptr, "and of a vector");
    Check(g_diffs == 4, "and nothing else");
    rebuilt->name = nullptr;
    BeginCompare();
    CompareObjects(&sets);
    Check(strstr(g_detail.Data(), "FixtureNamed#1.name: recorded \"beta\", replayed null") != nullptr, "a null pointer where a string was recorded is a difference");
    Check(!ReplayRestoreObjectField(*ReplayFindObjectField(ROK_FIXTURENAMED, "name"), rebuilt, "alpha", g_objects, AllocObject), "a value that is not quoted is refused");
    Check(!ReplayRestoreObjectField(*ReplayFindObjectField(ROK_FIXTURENAMED, "name"), rebuilt, "\"x\"", g_objects, nullptr), "and so is a string with no allocator");

    ResetBlock();
    RestoreText(one, sets);
    RestoreText(three, sets);
    rebuilt = (FixtureNamed*)ReplayResolveObject("FixtureNamed#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->name == nullptr && rebuilt->ident.get() == nullptr && rebuilt->words == nullptr, "a null line restores a null pointer, and a null array");
    free(one);
    free(two);
    free(three);
}

void BlobSelfTest()
{
    printf("bytes behind a pointer\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureProgram* program = (FixtureProgram*)Fresh(sizeof(FixtureProgram));
    program->count = 5;
    int* entries = (int*)Fresh(5 * (unsigned)sizeof(int));
    for (int i = 0; i < 5; i++)
        entries[i] = 10 * (i + 1);
    unsigned char* bits = (unsigned char*)Fresh(1);
    bits[0] = 0xa5;
    program->entries = entries;
    program->bits = bits;
    program->plain = (int*)Fresh(8);
    static const char* const fieldNames[6] = { "count", "entries", "entries[]", "bits", "bits[]", "plain" };
    unsigned short fields[6];
    for (int i = 0; i < 6; i++)
        fields[i] = FieldIndex(ROK_FIXTUREPROGRAM, fieldNames[i]);
    ReplaySlotSets sets = { "program", 0, nullptr, 0, nullptr, 0, fields, 6, fields, 6 };
    unsigned short kind = ROK_FIXTUREPROGRAM;
    void* root = program;
    Recorder recorder;
    ReplayText calls;
    recorder.Record(&kind, &root, 1, sets);
    printf("%s", recorder.text.Data());
    Check(strstr(recorder.text.Data(), "  r FixtureProgram#1.entries alloc#1\n  r FixtureProgram#1.entries[] [20] 0:0a000000140000001e00000028000000320000") != nullptr,
          "a blob is its pointer, as any ptr is, and its bytes as an array's are, with the length the other fields give");
    Check(strstr(recorder.text.Data(), "  r FixtureProgram#1.bits alloc#2\n  r FixtureProgram#1.bits[] [1] 0:a5\n") != nullptr, "a length may add before it shifts");
    Check(strstr(recorder.text.Data(), "  r FixtureProgram#1.plain alloc#3\n") != nullptr && strstr(recorder.text.Data(), "plain[]") == nullptr, "a ptr with no blob is its name only");
    calls.AddText(recorder.text);

    recorder.Record(&kind, &root, 1, sets);
    Check(recorder.text.Size() == 0, "bytes that did not change are not written again");
    entries[2] = 77;
    recorder.Record(&kind, &root, 1, sets);
    Check(strcmp(recorder.text.Data(), "  r FixtureProgram#1.entries[] 8:4d\n") == 0, "a changed byte is a delta run of one byte, as in an array");
    calls.AddText(recorder.text);

    int* moved = (int*)Fresh(5 * (unsigned)sizeof(int));
    memcpy(moved, entries, 5 * sizeof(int));
    program->entries = moved;
    recorder.Record(&kind, &root, 1, sets);
    Check(strstr(recorder.text.Data(), "  r FixtureProgram#1.entries alloc#4\n  r FixtureProgram#1.entries[] [20] 0:0a000000140000004d000000") != nullptr, "a pointer to another block writes all its bytes, though they are the same");
    calls.AddText(recorder.text);

    program->count = 6;
    int* longer = (int*)Fresh(6 * (unsigned)sizeof(int));
    memcpy(longer, moved, 5 * sizeof(int));
    longer[5] = 99;
    program->entries = longer;
    recorder.Record(&kind, &root, 1, sets);
    Check(strstr(recorder.text.Data(), "  r FixtureProgram#1.count 6\n  r FixtureProgram#1.entries alloc#5\n  r FixtureProgram#1.entries[] [24] 0:") != nullptr && strstr(recorder.text.Data(), "bits[]") == nullptr, "a longer length is a new bracket");
    calls.AddText(recorder.text);
    program->entries = nullptr;
    recorder.Record(&kind, &root, 1, sets);
    Check(strcmp(recorder.text.Data(), "  r FixtureProgram#1.entries null\n  r FixtureProgram#1.entries[] null\n") == 0, "a null pointer is null, twice");
    program->entries = longer;

    ResetBlock();
    RestoreText(calls.Data(), sets);
    FixtureProgram* rebuilt = (FixtureProgram*)ReplayResolveObject("FixtureProgram#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->count == 6 && rebuilt->entries != nullptr && rebuilt->entries != longer, "the replayer allocates the bytes");
    Check(rebuilt != nullptr && rebuilt->entries != nullptr && rebuilt->entries[0] == 10 && rebuilt->entries[2] == 77 && rebuilt->entries[5] == 99, "fills them, and the module reads them through an offset");
    Check(rebuilt != nullptr && rebuilt->bits != nullptr && rebuilt->bits[0] == 0xa5, "of every blob");
    Check(rebuilt != nullptr && rebuilt->plain != nullptr, "a ptr with no blob is an empty copy");
    g_rootKinds[0] = ROK_FIXTUREPROGRAM;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored bytes compare equal, and the pointer is still the object it was named");
    rebuilt->entries[3] = 1;
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed byte: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureProgram#1.entries[]") == 0 && strstr(g_detail.Data(), "byte 12 of 24") != nullptr, "a changed byte is reported at the blob, with its offset");
    rebuilt->entries[3] = 40;
    rebuilt->count = 5;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs >= 1 && strstr(g_detail.Data(), "FixtureProgram#1.entries[]") != nullptr && strstr(g_detail.Data(), "recorded 24 elements, replayed 20") != nullptr, "a blob of another length is reported");

    ResetBlock();
    RestoreText(calls.Data(), sets);
    Recorder replayed;
    rebuilt = (FixtureProgram*)ReplayResolveObject("FixtureProgram#1", nullptr, g_objects, AllocObject);
    void* rebuiltRoot = rebuilt;
    replayed.Record(&kind, &rebuiltRoot, 1, sets);
    Check(strstr(replayed.text.Data(), "  r FixtureProgram#1.entries[] [24] 0:0a00000014000000") != nullptr, "the rebuilt object records its bytes again");
    calls.Free();
}

void SharedBlobSelfTest()
{
    printf("two holders of one blob\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureProgram* first = (FixtureProgram*)Fresh(sizeof(FixtureProgram));
    FixtureProgram* second = (FixtureProgram*)Fresh(sizeof(FixtureProgram));
    int* entries = (int*)Fresh(5 * (unsigned)sizeof(int));
    for (int i = 0; i < 5; i++)
        entries[i] = 10 * (i + 1);
    first->count = second->count = 5;
    first->entries = second->entries = entries;
    static const char* const fieldNames[3] = { "count", "entries", "entries[]" };
    unsigned short fields[3];
    for (int i = 0; i < 3; i++)
        fields[i] = FieldIndex(ROK_FIXTUREPROGRAM, fieldNames[i]);
    ReplaySlotSets sets = { "program", 0, nullptr, 0, nullptr, 0, fields, 3, fields, 3 };
    unsigned short kinds[2] = { ROK_FIXTUREPROGRAM, ROK_FIXTUREPROGRAM };
    void* roots[2] = { first, second };
    Recorder recorder;
    recorder.Record(kinds, roots, 2, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    const char* bytes = strstr(text, "FixtureProgram#1.entries[] [20]");
    Check(bytes != nullptr && strstr(bytes + 31, "[20]") == nullptr, "the bytes of a target are written once");
    Check(strstr(text, "  r FixtureProgram#2.entries alloc#1\n") != nullptr && strstr(text, "FixtureProgram#2.entries[]") == nullptr, "the second holder names the target and writes no bytes");
    entries[1] = 77;
    recorder.Record(kinds, roots, 2, sets);
    Check(strcmp(recorder.text.Data(), "  r FixtureProgram#1.entries[] 4:4d\n") == 0, "a changed byte is one delta, whoever holds the target");
    char* changed = strdup(recorder.text.Data());

    ResetBlock();
    RestoreText(text, sets);
    RestoreText(changed, sets);
    FixtureProgram* rebuiltFirst = (FixtureProgram*)ReplayResolveObject("FixtureProgram#1", nullptr, g_objects, AllocObject);
    FixtureProgram* rebuiltSecond = (FixtureProgram*)ReplayResolveObject("FixtureProgram#2", nullptr, g_objects, AllocObject);
    Check(rebuiltFirst != nullptr && rebuiltSecond != nullptr && rebuiltFirst->entries != nullptr && rebuiltFirst->entries == rebuiltSecond->entries, "the holders replay to the same block");
    Check(rebuiltFirst != nullptr && rebuiltFirst->entries != nullptr && rebuiltFirst->entries[1] == 77 && rebuiltFirst->entries[4] == 50, "with its bytes");
    Check(rebuiltFirst != nullptr && g_objects.Pointer(ROK_ALLOC, 1) == rebuiltFirst->entries, "and the name still leads to it");
    g_rootKinds[0] = ROK_FIXTUREPROGRAM;
    g_rootKinds[1] = ROK_FIXTUREPROGRAM;
    g_rootObjects[0] = rebuiltFirst;
    g_rootObjects[1] = rebuiltSecond;
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the shared bytes compare equal for both");
    if (rebuiltFirst != nullptr && rebuiltFirst->entries != nullptr) {
        rebuiltFirst->entries[3] = 1;
        BeginCompare();
        CompareObjects(&sets);
        printf("a changed shared byte: %s\n%s", g_firstKey, g_detail.Data());
        Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureProgram#1.entries[]") == 0, "a changed byte is reported once");
    }
    free(text);
    free(changed);
}

void EmptyTargetSelfTest()
{
    printf("empty targets that are not null\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureProgram* program = (FixtureProgram*)Fresh(sizeof(FixtureProgram));
    FixtureNamed* named = (FixtureNamed*)Fresh(sizeof(FixtureNamed));
    new (&named->list) BlockVector<char*>();
    new (&named->idents) BlockVector<Ref<char> >();
    program->count = 0;
    program->entries = (int*)Fresh(1);
    named->count = 0;
    named->words = (char**)Fresh(1);
    static const char* const programNames[3] = { "count", "entries", "entries[]" };
    unsigned short programFields[3];
    for (int i = 0; i < 3; i++)
        programFields[i] = FieldIndex(ROK_FIXTUREPROGRAM, programNames[i]);
    unsigned short namedFields[2] = { FieldIndex(ROK_FIXTURENAMED, "count"), FieldIndex(ROK_FIXTURENAMED, "words") };
    unsigned short fields[5];
    memcpy(fields, programFields, sizeof programFields);
    memcpy(fields + 3, namedFields, sizeof namedFields);
    ReplaySlotSets sets = { "empty", 0, nullptr, 0, nullptr, 0, fields, 5, fields, 5 };
    unsigned short kinds[2] = { ROK_FIXTUREPROGRAM, ROK_FIXTURENAMED };
    void* roots[2] = { program, named };
    Recorder recorder;
    recorder.Record(kinds, roots, 2, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(strstr(text, "  r FixtureProgram#1.entries alloc#1\n  r FixtureProgram#1.entries[] [0]") != nullptr, "an empty blob is named, with no bytes");
    Check(strstr(text, "  r FixtureNamed#1.words [0]\n") != nullptr, "an empty array of strings is a length of nothing");

    ResetBlock();
    RestoreText(text, sets);
    FixtureProgram* rebuiltProgram = (FixtureProgram*)ReplayResolveObject("FixtureProgram#1", nullptr, g_objects, AllocObject);
    FixtureNamed* rebuiltNamed = (FixtureNamed*)ReplayResolveObject("FixtureNamed#1", nullptr, g_objects, AllocObject);
    Check(rebuiltProgram != nullptr && rebuiltProgram->entries != nullptr, "a non-null empty blob is not null when replayed");
    Check(rebuiltProgram != nullptr && rebuiltProgram->entries != nullptr && g_objects.Pointer(ROK_ALLOC, 1) == rebuiltProgram->entries, "and the name leads to it");
    Check(rebuiltNamed != nullptr && rebuiltNamed->words != nullptr, "nor is a non-null empty array of strings");
    g_rootKinds[0] = ROK_FIXTUREPROGRAM;
    g_rootKinds[1] = ROK_FIXTURENAMED;
    g_rootObjects[0] = rebuiltProgram;
    g_rootObjects[1] = rebuiltNamed;
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "both compare equal");
    free(text);
}

void RebindSelfTest()
{
    printf("a name bound again\n");
    ResetBlock();
    static char cells[64];
    for (unsigned i = 0; i < 20000; i++)
        g_objects.Bind(ROK_ALLOC, 1, cells + i % 64);
    Check(g_objects.Find(ROK_ALLOC, cells + 19999 % 64) == 1, "the last pointer bound to a name is found by it");
    Check(g_objects.Find(ROK_ALLOC, cells + 19998 % 64) == 0, "and the one it replaced is not");
    Check(g_objects.Pointer(ROK_ALLOC, 1) == cells + 19999 % 64, "the name leads to the last one, after more rebinds than the table has slots");
    g_objects.Bind(ROK_ALLOC, 2, cells);
    Check(g_objects.Find(ROK_ALLOC, cells) == 2 && g_objects.Find(ROK_ALLOC, cells + 19999 % 64) == 1, "another name is still told apart");
}

void RebindToFreshAddressesSelfTest()
{
    printf("a name bound to fresh addresses\n");
    ResetBlock();
    static char cells[20000];
    for (unsigned i = 0; i < 20000; i++)
        g_objects.Bind(ROK_ALLOC, 1, cells + i);
    Check(g_objects.Find(ROK_ALLOC, cells + 19999) == 1, "the last address bound is found");
    Check(g_objects.Find(ROK_ALLOC, cells + 19998) == 0, "the one it replaced is not");
    static char never;
    Check(g_objects.Find(ROK_ALLOC, &never) == 0, "an address never bound is not found");
}

void NestedVectorSelfTest()
{
    printf("vectors of vectors\n");
    ResetBlock();
    ReplayClearFailure();
    FixtureNest* nest = (FixtureNest*)Fresh(sizeof(FixtureNest));
    new (&nest->mids) BlockVector<FixtureMid>();
    new (&nest->rows) BlockVector<FixtureRow>();
    nest->id = 7;
    for (int m = 0; m < 2; m++) {
        FixtureMid mid;
        new (&mid) FixtureMid();
        mid.tag = 10 + m;
        for (int l = 0; l < 2; l++) {
            FixtureLeaf leaf;
            new (&leaf) FixtureLeaf();
            leaf.kind = 100 * m + l;
            leaf.steps.PushBack((short)(m + l));
            leaf.steps.PushBack((short)(m + l + 1));
            leaf.names.PushBack(BlockText(l == 0 ? "a" : "b"));
            mid.leaves.PushBack(leaf);
        }
        nest->mids.PushBack(mid);
    }
    FixtureRow row;
    new (&row) FixtureRow();
    row.PushBack(4);
    row.PushBack(5);
    nest->rows.PushBack(row);

    static const char* const fieldNames[3] = { "id", "mids", "rows" };
    unsigned short fields[3];
    for (int i = 0; i < 3; i++)
        fields[i] = FieldIndex(ROK_FIXTURENEST, fieldNames[i]);
    ReplaySlotSets sets = { "nest", 0, nullptr, 0, nullptr, 0, fields, 3, fields, 3 };
    unsigned short kind = ROK_FIXTURENEST;
    void* root = nest;
    Recorder recorder;
    ReplayText calls;
    recorder.Record(&kind, &root, 1, sets);
    printf("%s", recorder.text.Data());
    const char* first = recorder.text.Data();
    Check(strstr(first, "  r FixtureNest#1.mids [2/2]\n  r FixtureNest#1.mids[0].tag 10\n  r FixtureNest#1.mids[0].leaves [2/2]\n  r FixtureNest#1.mids[0].leaves[0].kind 0\n") != nullptr,
          "a vector inside an element is a bracket under the element's path, and its elements are lines under it");
    Check(strstr(first, "  r FixtureNest#1.mids[1].leaves[1].kind 101\n  r FixtureNest#1.mids[1].leaves[1].steps [2/2] 0:02000300\n  r FixtureNest#1.mids[1].leaves[1].names [1/1]\n  r FixtureNest#1.mids[1].leaves[1].names[0] \"b\"\n") != nullptr,
          "to any depth, a scalar vector as a run and a vector of strings as lines");
    Check(strstr(first, "  r FixtureNest#1.rows [1/1]\n  r FixtureNest#1.rows[0].cells [2/2] 0:04000500\n") != nullptr, "an element that is itself a vector is its field named for its base");
    calls.AddText(recorder.text);

    nest->mids[1].leaves[0].steps[1] = 9;
    recorder.Record(&kind, &root, 1, sets);
    Check(strcmp(recorder.text.Data(), "  r FixtureNest#1.mids[1].leaves[0].steps 1:0900\n") == 0, "one inner element that changed is one line, at its path");
    calls.AddText(recorder.text);
    nest->mids[0].leaves[1].names[0] = BlockText("z");
    recorder.Record(&kind, &root, 1, sets);
    Check(strcmp(recorder.text.Data(), "  r FixtureNest#1.mids[0].leaves[1].names[0] \"z\"\n") == 0, "and so is a string");
    calls.AddText(recorder.text);
    FixtureLeaf extra;
    new (&extra) FixtureLeaf();
    extra.kind = 5;
    extra.steps.PushBack(1);
    nest->mids[0].leaves.PushBack(extra);
    recorder.Record(&kind, &root, 1, sets);
    printf("%s", recorder.text.Data());
    Check(strstr(recorder.text.Data(), "  r FixtureNest#1.mids[0].leaves [3/4]\n  r FixtureNest#1.mids[0].leaves[2].kind 5\n  r FixtureNest#1.mids[0].leaves[2].steps [1/1] 0:0100\n") != nullptr && strstr(recorder.text.Data(), "leaves[0]") == nullptr, "a vector that grew writes its new element in full");
    calls.AddText(recorder.text);

    ResetBlock();
    RestoreText(calls.Data(), sets);
    FixtureNest* rebuilt = (FixtureNest*)ReplayResolveObject("FixtureNest#1", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->mids.Size() == 2 && rebuilt->mids[0].leaves.Size() == 3 && rebuilt->mids[0].leaves.Capacity() == 4, "the replayer rebuilds the vectors at every depth");
    Check(rebuilt != nullptr && rebuilt->mids[1].leaves[0].steps.Size() == 2 && rebuilt->mids[1].leaves[0].steps[1] == 9 && rebuilt->mids[1].leaves[1].kind == 101, "with the inner element that changed");
    Check(rebuilt != nullptr && strcmp(rebuilt->mids[0].leaves[1].names[0], "z") == 0 && rebuilt->mids[0].leaves[2].steps[0] == 1 && rebuilt->mids[0].leaves[0].steps[1] == 1, "and the strings, the new element, and the old ones carried over the growth");
    Check(rebuilt != nullptr && rebuilt->rows.Size() == 1 && rebuilt->rows[0].Size() == 2 && rebuilt->rows[0][1] == 5, "a vector that is an element too");

    g_rootKinds[0] = ROK_FIXTURENEST;
    g_rootObjects[0] = rebuilt;
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0 && ReplayFailure() == nullptr, "the restored vectors compare equal");
    rebuilt->mids[1].leaves[0].steps[1] = 8;
    rebuilt->mids[0].leaves[2].kind = 6;
    BeginCompare();
    CompareObjects(&sets);
    printf("changed inner elements: %s\n%s", g_firstKey, g_detail.Data());
    Check(strstr(g_detail.Data(), "FixtureNest#1.mids[1].leaves[0].steps[1]: recorded 0900, replayed 0800") != nullptr, "an inner element is reported at its path");
    Check(strstr(g_detail.Data(), "FixtureNest#1.mids[0].leaves[2].kind: recorded 5, replayed 6") != nullptr && g_diffs == 2, "and so is a field of an inner struct");
    rebuilt->mids[1].leaves[0].steps[1] = 9;
    rebuilt->mids[0].leaves[2].kind = 5;
    rebuilt->mids[0].leaves[1].steps.PushBack(3);
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 2 && strcmp(g_firstKey, "FixtureNest#1.mids[0].leaves[1].steps.length") == 0 && strstr(g_detail.Data(), "FixtureNest#1.mids[0].leaves[1].steps.capacity") != nullptr, "an inner vector of another length is reported at its length, and its capacity");

    ResetBlock();
    RestoreText(calls.Data(), sets);
    Recorder replayed;
    rebuilt = (FixtureNest*)ReplayResolveObject("FixtureNest#1", nullptr, g_objects, AllocObject);
    void* rebuiltRoot = rebuilt;
    replayed.Record(&kind, &rebuiltRoot, 1, sets);
    Check(strstr(replayed.text.Data(), "  r FixtureNest#1.mids[0].leaves [3/4]\n") != nullptr && strstr(replayed.text.Data(), "  r FixtureNest#1.mids[1].leaves[0].steps [2/2] 0:01000900\n") != nullptr, "the rebuilt object records the vectors as they were");
    calls.Free();
}

#pragma push_macro("g_losTables")
#undef g_losTables

LosTables& LosTablesOf()
{
    return g_simGlobals->g_losTables;
}

#pragma pop_macro("g_losTables")

#pragma push_macro("g_missionOrderTableVec")
#undef g_missionOrderTableVec

BlockVector<OrderTableEntry>& OrderEntriesOf()
{
    return g_simGlobals->g_missionOrderTableVec;
}

#pragma pop_macro("g_missionOrderTableVec")

int RestoreBlockText(const char* text)
{
    char* copy = (char*)malloc(strlen(text) + 1);
    strcpy(copy, text);
    int lines = 0;
    for (char* p = copy; *p != '\0'; p = strchr(p, '\0') + 1) {
        char* end = strchr(p, '\n');
        *end = '\0';
        char* space = strchr(p + 4, ' ');
        *space = '\0';
        const ReplayField* f = FindBlockField(p + 4);
        Check(f != nullptr && ApplyBlockVector(*f, p + 4, space + 1, true), "a block line restores");
        lines++;
        p = end;
    }
    free(copy);
    return lines;
}

void SimVectorSelfTest()
{
    printf("a block vector in the sim globals\n");
    ResetBlock();
    ReplayClearFailure();
    for (int t = 0; t < 4; t++) {
        LosTable table;
        new (&table) LosTable();
        if (t == 3) {
            table.Resize(6);
            LosStep first = { 1, 2 };
            LosStep second = { 3, 4 };
            table[5].PushBack(first);
            table[5].PushBack(second);
        }
        LosTablesOf().PushBack(table);
    }
    OrderTableEntry entry;
    memset(&entry, 0, sizeof entry);
    entry.value = BlockText("stop");
    entry.drawMask = 5;
    OrderEntriesOf().PushBack(entry);

    const ReplayField& tables = kReplayFields[RF_SIMGLOBALS_G_LOSTABLES];
    const ReplayField& orders = kReplayFields[RF_SIMGLOBALS_G_MISSIONORDERTABLEVEC];
    Check(tables.kind == RK_VECTOR && orders.kind == RK_VECTOR, "a BlockVector global is a field of kind vector");
    ReplayKnown known;
    ReplayText out;
    ReplayText scratch;
    ReplayObjectTable* table = new ReplayObjectTable;
    ReplayEmitField(out, known, *table, 'r', tables.name, ReplayFieldView(tables), BlockBase(tables), scratch);
    ReplayEmitField(out, known, *table, 'r', orders.name, ReplayFieldView(orders), BlockBase(orders), scratch);
    printf("%s", out.Data());
    Check(strstr(out.Data(), "  r SimGlobals.g_losTables [4/4]\n  r SimGlobals.g_losTables[0].lines [0/0]\n") != nullptr, "it is written as an object's vector is, under the name of the global");
    Check(strstr(out.Data(), "  r SimGlobals.g_losTables[3].lines [6/6]\n") != nullptr && strstr(out.Data(), "  r SimGlobals.g_losTables[3].lines[5].steps [2/2]\n  r SimGlobals.g_losTables[3].lines[5].steps[0].a 1\n"
                                                                                                                   "  r SimGlobals.g_losTables[3].lines[5].steps[0].b 2\n  r SimGlobals.g_losTables[3].lines[5].steps[1].a 3\n") != nullptr,
          "and its elements, three deep, by path");
    Check(strstr(out.Data(), "  r SimGlobals.g_missionOrderTableVec [1/1]\n  r SimGlobals.g_missionOrderTableVec[0].value \"stop\"\n  r SimGlobals.g_missionOrderTableVec[0].name null\n  r SimGlobals.g_missionOrderTableVec[0].drawMask 5\n") != nullptr, "a struct with strings too");
    char* one = strdup(out.Data());
    LosTablesOf()[3][5][1].b = 9;
    out.Clear();
    ReplayEmitField(out, known, *table, 'r', tables.name, ReplayFieldView(tables), BlockBase(tables), scratch);
    Check(strcmp(out.Data(), "  r SimGlobals.g_losTables[3].lines[5].steps[1].b 9\n") == 0, "one element that changed is one line");
    char* two = strdup(out.Data());

    ResetBlock();
    int lines = RestoreBlockText(one);
    RestoreBlockText(two);
    Check(lines >= 20 && LosTablesOf().Size() == 4 && LosTablesOf()[3].Size() == 6 && LosTablesOf()[3][5].Size() == 2, "the replayer rebuilds the vectors in the globals");
    Check(LosTablesOf()[3][5][0].a == 1 && LosTablesOf()[3][5][1].a == 3 && LosTablesOf()[3][5][1].b == 9, "and their elements");
    Check(OrderEntriesOf().Size() == 1 && strcmp(OrderEntriesOf()[0].value, "stop") == 0 && OrderEntriesOf()[0].name == nullptr && OrderEntriesOf()[0].drawMask == 5, "with the strings");
    BeginCompare();
    CompareBlockVector(tables);
    CompareBlockVector(orders);
    Check(g_diffs == 0, "the restored globals compare equal");
    LosTablesOf()[3][5][0].b = 7;
    OrderEntriesOf()[0].value = BlockText("go");
    BeginCompare();
    CompareBlockVector(tables);
    CompareBlockVector(orders);
    printf("%s", g_detail.Data());
    Check(strstr(g_detail.Data(), "SimGlobals.g_losTables[3].lines[5].steps[0].b: recorded 2, replayed 7") != nullptr, "a changed inner element is reported at its path");
    Check(strstr(g_detail.Data(), "SimGlobals.g_missionOrderTableVec[0].value: recorded \"stop\", replayed \"go\"") != nullptr && g_diffs == 2, "and a changed string");
    free(one);
    free(two);
    out.Free();
    scratch.Free();
    delete table;
}

void GameWriteSelfTest()
{
    printf("a write to Game through a pointer\n");
    ResetBlock();
    ReplayClearFailure();
    const ReplayField& f = kReplayFields[RF_GAME_HOTRADARCOUNT];
    g_game->hotRadarCount = 2;
    ReplayKnown known;
    ReplayText entry;
    ReplayText exit;
    ReplayText scratch;
    ReplayObjectTable* table = new ReplayObjectTable;
    ReplayEmitField(entry, known, *table, 'r', f.name, ReplayFieldView(f), BlockBase(f), scratch);
    int* through = &g_game->hotRadarCount;
    *through = 3;
    ReplayEmitField(exit, known, *table, 'w', f.name, ReplayFieldView(f), BlockBase(f), scratch);
    Check(strcmp(entry.Data(), "  r Game.hotRadarCount 2\n") == 0 && strcmp(exit.Data(), "  w Game.hotRadarCount 3\n") == 0, "a write through a pointer is a w line of the field it lands on");

    ResetBlock();
    Check(RestoreField(f, "2"), "the entry value restores");
    through = &g_game->hotRadarCount;
    *through = 4;
    ReplayText now;
    FormatField(f, now);
    BeginCompare();
    if (strcmp("3", now.Data()) != 0)
        Diff("Game.hotRadarCount", "3", now.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "Game.hotRadarCount") == 0 && strstr(g_detail.Data(), "recorded 3, replayed 4") != nullptr, "a different write is reported at the field");
    now.Free();
    entry.Free();
    exit.Free();
    scratch.Free();
    delete table;
}

void OutFormsSelfTest()
{
    printf("a byte buffer a slot fills\n");
    ResetBlock();
    LoadCall("  a size 8\n  o out x:101112\n  o result 3\n");
    BeginCompare();
    Run_fixture_Drain();
    After_fixture_Drain();
    Check(g_diffs == 0, "a slot's out bytes replay equal, as long as the result");
    LoadCall("  a size 2\n  o out x:1011\n  o result 3\n");
    BeginCompare();
    Run_fixture_Drain();
    After_fixture_Drain();
    Check(g_diffs == 0, "and a result larger than the buffer is as long as its size");
    LoadCall("  a size 8\n  o out x:101113\n  o result 3\n");
    BeginCompare();
    Run_fixture_Drain();
    After_fixture_Drain();
    printf("bytes that came out differently: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "out") == 0, "a byte that differs is reported at the argument");
    LoadCall("  a size 8\n  o result 3\n");
    BeginCompare();
    Run_fixture_Drain();
    After_fixture_Drain();
    Check(g_diffs == 1 && strcmp(g_firstKey, "out") == 0, "a missing o line is a difference");
    LoadCall("  a size 0\n  o result 3\n");
    BeginCompare();
    Run_fixture_Drain();
    After_fixture_Drain();
    Check(g_diffs == 0, "a call with no bytes has no o line, whatever its result");

    printf("a scratch buffer\n");
    LoadCall("  a capacity 64\n  o result 64\n");
    BeginCompare();
    Run_fixture_Gather();
    After_fixture_Gather();
    Check(g_diffs == 0, "a scratch buffer of the capacity is passed, and what the call writes to it is not compared");

    printf("a scalar the import writes\n");
    LoadCall("  i fixture.Dims\n  o width 12\n  o height -7\n");
    BeginImport();
    int width = 99;
    int height = 99;
    Imp_fixture_Dims(&width, &height);
    Check(g_diffs == 0 && width == 12 && height == -7, "the stub writes each o line through its pointer");
    LoadCall("  i fixture.Dims\n  o width 12\n");
    BeginImport();
    Imp_fixture_Dims(&width, nullptr);
    Check(g_diffs == 0 && width == 12, "and a null pointer takes none");
}

void LoadDataMapSelfTest()
{
    if (!ReplayWasmActive())
        return;
    printf("load data in the module memory\n");
    ResetBlock();
    BeginServices(
        "  i services.GetGafFrame FrameTable#1 4 = GafFrame#1\n"
        "  o GafFrame#1.width 2\n"
        "  o GafFrame#1.height 2\n"
        "  o GafFrame#1.xOffset -1\n"
        "  o GafFrame#1.yOffset 1\n"
        "  o GafFrame#1.transparency 9\n"
        "  o GafFrame#1.pixelsOrLayers alloc#2\n"
        "  o GafFrame#1.pixelsOrLayers[] [4] 0:0102ff09\n");
    Check(RestoreField(*FindBlockField("Game.losTable"), "FrameTable#1"), "a ptr to a frame table restores");
    Check(RestoreObjectLine("FrameTable#1.count", "5", "self-test", nullptr, "  r FrameTable#1.count 5"), "and so does the table's count");
    SyncLoadObjects();
    FrameTable* table = (FrameTable*)ReplayResolveObject("FrameTable#1", nullptr, g_objects, AllocObject);
    unsigned linear = (unsigned)g_game->losTable;
    Check(table != nullptr && linear != (unsigned)table && LoadNative(linear) == table, "the field holds the table's load address, not the replayer's");
    table->entries = table;
    SyncLoadObjects();
    Check(*(const unsigned short*)ReplayWasmLinear(linear) == 5, "and the module reads the recorded count there");
    Check(((const FrameTable*)ReplayWasmLinear(linear))->entries == nullptr, "but no native address in its entries");
    unsigned classWords[256];
    unsigned classWordCount = ReplayClassWords(classWords, sizeof classWords / sizeof classWords[0]);
    Check(classWordCount != 0 && linear >= ReplayWasmLoadDataBase() + classWordCount * 4 && memcmp(ReplayWasmLinear(ReplayWasmLoadDataBase()), classWords, classWordCount * 4) == 0, "above the class word table, which stays at the region's base");
    ReplayAnswerHook() = ReplayAnswerImport;
    unsigned frame = LoadGetGafFrame(linear, 4);
    ReplayAnswerHook() = nullptr;
    Check(g_diffs == 0 && frame != 0 && LoadNative(frame) == g_objects.Pointer(ROK_GAFFRAME, 1), "GetGafFrame answers with the frame's load address");
    const GafFrame* copy = (const GafFrame*)ReplayWasmLinear(frame);
    Check(copy->width == 2 && copy->height == 2 && copy->xOffset == -1 && copy->yOffset == 1 && copy->transparency == 9, "where the module reads the recorded size, offsets and transparency");
    const unsigned char* pixels = ReplayWasmLinear((unsigned)copy->pixelsOrLayers);
    Check(copy->pixelsOrLayers != nullptr && pixels[0] == 1 && pixels[1] == 2 && pixels[2] == 0xff && pixels[3] == 9, "and whose pixels are the recorded ones, at their load address");
    ResetBlock();
}

void LoadDataReclaimSelfTest()
{
    ForgetLoadObjects();
    const unsigned regionSize = 65536;
    const unsigned base = 0x10100000;
    unsigned char* region = (unsigned char*)calloc(regionSize, 1);
    unsigned char* kept = (unsigned char*)calloc(64, 1);
    LoadDataAttach(region, base, regionSize, nullptr);
    LoadDataRegisterFrame(kept, 64);
    unsigned keptAt = LoadAddress(kept);
    unsigned first = 0;
    for (int battle = 0; battle < 8; battle++) {
        unsigned char* a = (unsigned char*)calloc(20000, 1);
        unsigned char* b = (unsigned char*)calloc(20000, 1);
        a[100] = (unsigned char)battle;
        LoadDataRegisterFrame(a, 20000);
        LoadDataRegisterFrame(b, 20000);
        unsigned at = LoadAddress(a);
        LoadAddress(b);
        if (battle == 0)
            first = at;
        Check(at == first, "a battle's load data reuses the space the last battle freed");
        Check(region[at - base + 100] == (unsigned char)battle, "the reused space holds the new copy");
        Check(LoadNative(at + 100) == a + 100, "a load address of the new copy leads to its buffer");
        LoadDataUnregister(a);
        LoadDataUnregister(b);
        free(a);
        free(b);
    }
    Check(LoadNative(keptAt) == kept, "a buffer mapped before every battle stays mapped");
    LoadDataUnregister(kept);
    LoadDataAttach(nullptr, 0, 0, nullptr);
    free(kept);
    free(region);
}

void LoadDataClassWordsSelfTest()
{
    const unsigned regionSize = 4096;
    const unsigned base = 0x10100000;
    const unsigned words[3] = {0x401000, 0x402000, 0x403000};
    unsigned char* region = (unsigned char*)calloc(regionSize, 1);
    LoadDataAttach(region, base, regionSize, nullptr);
    LoadDataWriteClassWords(words, 3);
    Check(memcmp(region, words, sizeof words) == 0, "the class words start the load_data region, in index order");
    for (int battle = 0; battle < 3; battle++) {
        unsigned char* frame = (unsigned char*)calloc(64, 1);
        LoadDataRegisterFrame(frame, 64);
        Check(LoadAddress(frame) == base + 16, "the bump allocator starts above the class words, at an 8-byte boundary");
        LoadDataCheck(1);
        LoadDataUnregister(frame);
        free(frame);
    }
    Check(memcmp(region, words, sizeof words) == 0, "reclaiming the load data leaves the class words");
    LoadDataAttach(nullptr, 0, 0, nullptr);
    free(region);
}

void RegionSelfTest()
{
    printf("a module's region\n");
    ResetBlock();
    ReplayClearFailure();
    const ReplayField& f = kReplayFields[RF_REGION];
    Check(f.base == RB_REGION && f.kind == RK_ARRAY, "the region is a block field of its own base");
    unsigned char* region = (unsigned char*)AllocObject(2048);
    RegionBind(region, 2048);
    region[600] = 0x11;
    region[601] = 0x22;
    ReplayKnown known;
    ReplayText first;
    ReplayText second;
    ReplayText scratch;
    ReplayObjectTable* table = new ReplayObjectTable;
    ReplayEmitField(first, known, *table, 'w', f.name, ReplayFieldView(f), BlockBase(f), scratch);
    Check(strncmp(first.Data(), "  w region [2048]\n  w region 512:0000", 36) == 0, "a whole write names the length, and leaves out the zero chunks");
    Check(CountLines(first.Data(), "  w region ") == 2 && strstr(first.Data(), " 1024:") == nullptr && strstr(first.Data(), " 1536:") == nullptr, "only the chunk with a non-zero byte follows");
    region[2000] = 0x7f;
    ReplayEmitField(second, known, *table, 'w', f.name, ReplayFieldView(f), BlockBase(f), scratch);
    Check(strcmp(second.Data(), "  w region 2000:7f\n") == 0, "a later write is the changed run");

    ReplayKnown replayed;
    ReplayText lines;
    lines.AddText(first);
    lines.AddText(second);
    for (const char* p = lines.Data(); *p != '\0';) {
        const char* end = strchr(p, '\n');
        char value[1300];
        size_t n = (size_t)(end - (p + 11));
        memcpy(value, p + 11, n);
        value[n] = '\0';
        Check(ReplayApplyArray(ReplayFieldView(f), nullptr, "region", value, replayed, false, nullptr), "a line of the region applies");
        p = end + 1;
    }
    ReplayText now;
    FormatField(f, now);
    Check(strcmp(replayed.Get("region"), now.Data()) == 0, "the lines make the bytes the module left");
    region[600] = 0x12;
    FormatField(f, now);
    BeginCompare();
    if (strcmp(replayed.Get("region"), now.Data()) != 0)
        Diff("region", replayed.Get("region"), now.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "region") == 0 && strstr(g_detail.Data(), "byte 600 of 2048: recorded 11, replayed 12") != nullptr, "a changed byte is reported at the region, with its byte");
    first.Free();
    second.Free();
    lines.Free();
    now.Free();
    scratch.Free();
    delete table;

    printf("a region at its recorded offset\n");
    ResetBlock();
    char why[160];
    unsigned offset = 0x200000;
    unsigned char* placed = (unsigned char*)PlaceRegion(offset, 4096, why);
    Check(placed == (unsigned char*)SimBlockBase() + offset, "the region is placed at the offset the recording names");
    RegionBind(placed, 4096);
    BlockBase(f);
    Check(g_regionView.data == offset && g_regionView.size == 4096, "the region's block offset is the recorded one");
    Check(PlaceRegion(offset + 1024, 64, why) == nullptr && strstr(why, "block offset 2098176 (64 bytes) is not free") != nullptr, "a second region that clashes with it is refused, naming its offset");
    Check(PlaceRegion((unsigned)((unsigned char*)g_game - (unsigned char*)SimBlockBase()), 64, why) == nullptr && strstr(why, "is not free") != nullptr, "a region at the offset of the replayer's own Game is refused");
    Check(PlaceRegion(SimBlockSize() - 32, 64, why) == nullptr && strstr(why, "is past the block's end") != nullptr, "a region that passes the block's end is refused, naming its offset");
    Check(PlaceRegion(offset + 4096 + 16, 64, why) == nullptr && strstr(why, "lies in a free range that would leave a leading remainder under 16 bytes") != nullptr, "a region too close to the start of a free range is refused as such, not as a clash");
    Check(PlaceRegion(offset + 8196, 64, why) == nullptr && strstr(why, "is not 8-byte aligned") != nullptr, "and a region off the 8-byte grid as that");
    Check(PlaceRegion(40, 16, why) == nullptr && strstr(why, "inside the block's own header of 64 bytes") != nullptr, "a region whose chunk header would sit in the block's header is refused as such");
    Check(PlaceRegion(SimBlockSize(), 0, why) == nullptr && strstr(why, "is past the block's end") != nullptr, "and one that passes the block's end only with its smallest chunk is past the end");
    void* after = PlaceRegion(offset + 8192, 64, why);
    void* folded = PlaceRegion(offset + 4096 + 8, 4072, why);
    Check(after != nullptr && folded != nullptr && (*(const unsigned*)((const unsigned char*)folded - 8) & ~1u) == 4088, "a region that leaves a tail under 16 bytes takes the tail into its chunk");
    SimFree(after);
    Check(PlaceRegion(offset + 8184, 64, why) == nullptr && strstr(why, "is not free") != nullptr, "so a region whose header lands in that tail clashes with it, not with a leading remainder");
}


void MakeCobFile(unsigned char* file)
{
    memset(file, 0, 72);
    int* h = (int*)file;
    h[1] = 2;
    h[2] = 1;
    h[5] = 1;
    h[6] = 68;
    h[7] = 44;
    h[8] = 52;
    h[9] = 56;
    h[10] = 60;
    h[11] = 8;
    h[12] = 16;
    h[13] = 24;
    h[15] = 32;
    h[16] = 40;
}

void MakeModelFile(unsigned char* file)
{
    memset(file, 0, 128);
    Object3do* root = (Object3do*)file;
    Object3do* child = (Object3do*)(file + 52);
    root->vertexCount = 1;
    root->firstFace = -1;
    root->vertices = (Vec3*)104;
    root->faces = (PieceFrame*)116;
    root->child = (Object3do*)52;
    child->vertexCount = 1;
    child->firstFace = -1;
    child->vertices = (Vec3*)116;
    child->faces = (PieceFrame*)128;
    Vec3* vertices = (Vec3*)(file + 104);
    vertices[0].x = 10;
    vertices[0].y = 20;
    vertices[0].z = 30;
    vertices[1].x = 1;
    vertices[1].y = 2;
    vertices[1].z = 3;
}

void AddLoadFile(const char* name, const void* bytes, unsigned size)
{
    LoadFileBytes* f = new LoadFileBytes;
    strcpy(f->name, name);
    f->bytes.assign((const unsigned char*)bytes, (const unsigned char*)bytes + size);
    g_loadFiles.push_back(f);
}

void NoteLoadHook(unsigned load, const char* name, const void* data)
{
    ReplayLoadNote(load, name, data, (unsigned)LoadFileLength(name));
}

char* PrepareLoads(const char* loads, const char* body)
{
    free(g_text);
    g_text = (char*)malloc(strlen(loads) + strlen(body) + 64);
    strcpy(g_text, "bt-replay 2\nsubsystem fixture 1.0\n");
    strcat(g_text, loads);
    strcat(g_text, "call 1 Fake\n");
    strcat(g_text, body);
    SplitLines();
    int at = 2;
    while (at < g_lineCount && strncmp(g_lines[at], "call ", 5) != 0)
        at++;
    LoadInputs("self-test", at);
    g_callFirst = at;
    g_callEnd = g_lineCount;
    g_callIndex = 0;
    return g_text;
}

void LoadInputsSelfTest()
{
    printf("load inputs\n");
    ResetBlock();
    ReplayClearFailure();
    BindLoadFiles();
    unsigned char cob[72];
    MakeCobFile(cob);
    unsigned char model[128];
    MakeModelFile(model);
    AddLoadFile("scripts\\ARM.COB", cob, sizeof cob);
    AddLoadFile("scripts\\CORE.COB", cob, sizeof cob);
    AddLoadFile("objects3d\\arm.3do", model, sizeof model);
    ReplayLoadHook() = NoteLoadHook;
    int* armCob = (int*)LoadCobScript((char*)"scripts\\ARM.COB");
    int* coreCob = (int*)LoadCobScript((char*)"scripts\\CORE.COB");
    Object3do* armModel = (Object3do*)Load3do((char*)"objects3d\\arm.3do");
    MirrorObject(armModel);
    ReplayLoadHook() = nullptr;
    Check(armCob != nullptr && coreCob != nullptr && armCob != coreCob && armCob[11] == 8 + (int)armCob, "the real loaders run on the registered files and relocate them in place");
    Check(armModel->child == (Object3do*)((char*)armModel + 52) && armModel->vertices[0].x == -10 && armModel->child->vertices[0].z == -3, "a model is relocated and mirrored");

    UnitDef* defs = (UnitDef*)Fresh(3 * (unsigned)sizeof(UnitDef));
    g_game->defs = defs;
    g_game->unitDefCount = 3;
    void** types = (void**)Fresh(3 * (unsigned)sizeof(void*));
    g_game->types = (Object3do**)types;
    defs[1].data = armCob;
    defs[2].data = coreCob;
    types[1] = armModel;
    ReplayObjectTable* table = new ReplayObjectTable;
    ReplayText name;
    Check(ReplayAddLoadName(name, *table, RL_COB, armCob) && strcmp(name.Data(), "UnitDef#1") == 0, "a pointer to a loaded script is the unit type that holds it");
    name.Clear();
    ReplayAddLoadName(name, *table, RL_COB, (char*)coreCob + 20);
    Check(strcmp(name.Data(), "UnitDef#2+20") == 0, "and the offset into the file when it is not the start");
    name.Clear();
    ReplayAddLoadName(name, *table, RL_MODEL, (char*)armModel + 52);
    Check(strcmp(name.Data(), "UnitDef#1+52") == 0, "a node inside a model is the model and its offset, the holder being Game.types[1]");
    name.Clear();
    ReplayAddLoadName(name, *table, RL_COB, nullptr);
    Check(strcmp(name.Data(), "null") == 0 && ReplayFailure() == nullptr, "a null pointer is null");
    Check(table->LoadUses().size() == 3, "each load that a line named is listed once, in first-use order");

    ReplayText lines;
    ReplayAddLoadLines(*table, lines);
    printf("%s", lines.Data());
    const char* firstLine = "load cob UnitDef#1 \"scripts\\\\ARM.COB\" [72]\n  x 0:";
    Check(strncmp(lines.Data(), firstLine, strlen(firstLine)) == 0, "a load line names the load kind, the holder, the file and its size");
    Check(strstr(lines.Data(), "load cob UnitDef#2 \"scripts\\\\CORE.COB\" [72]\n") != nullptr && strstr(lines.Data(), "load model UnitDef#1 \"objects3d\\\\arm.3do\" [128]\n  x 0:") != nullptr, "one for each, with the bytes of the file as it was read");
    Check(strstr(lines.Data(), "08000000") != nullptr && strstr(lines.Data(), "08000000") != nullptr, "the bytes are the file as it was read, before its pointers were relocated");
    ReplayText expected;
    char relocated[16];
    sprintf(relocated, "%02x%02x%02x%02x", (unsigned)(armCob[11] & 0xff), (unsigned)((armCob[11] >> 8) & 0xff), (unsigned)((armCob[11] >> 16) & 0xff), (unsigned)((armCob[11] >> 24) & 0xff));
    Check(strstr(lines.Data(), relocated) == nullptr, "and not as it is now");
    expected.Free();

    int stray = 5;
    name.Clear();
    ReplayAddLoadName(name, *table, RL_COB, &stray);
    printf("%s\n", ReplayFailure() == nullptr ? "no failure" : ReplayFailure());
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "a pointer into no cob load") != nullptr, "a pointer into no loaded file fails the run");
    ReplayClearFailure();
    name.Clear();
    ReplayAddLoadName(name, *table, RL_COB, (char*)armModel + 4);
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "a pointer into no cob load") != nullptr, "and so does one into a file of another load kind");
    ReplayClearFailure();
    defs[2].data = nullptr;
    ReplayLoadsReset();
    ReplayLoadHook() = NoteLoadHook;
    int* orphan = (int*)LoadCobScript((char*)"scripts\\CORE.COB");
    ReplayLoadHook() = nullptr;
    name.Clear();
    ReplayAddLoadName(name, *table, RL_COB, orphan);
    printf("%s\n", ReplayFailure() == nullptr ? "no failure" : ReplayFailure());
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "no holder holds the cob load of scripts\\CORE.COB") != nullptr, "a loaded file that no unit type holds fails the run, naming the file");
    ReplayClearFailure();
    defs[0].data = orphan;
    ReplayObjectTable* again = new ReplayObjectTable;
    name.Clear();
    ReplayAddLoadName(name, *again, RL_COB, orphan);
    Check(ReplayFailure() == nullptr && strcmp(name.Data(), "UnitDef#0") == 0, "a script that a unit type holds is found by scanning the unit types");
    unsigned char changed[72];
    MakeCobFile(changed);
    changed[0] = 9;
    AddLoadFile("scripts\\CORE2.COB", changed, sizeof changed);
    ReplayLoadHook() = NoteLoadHook;
    int* reloaded = (int*)LoadCobScript((char*)"scripts\\CORE2.COB");
    ReplayLoadHook() = nullptr;
    defs[0].data = reloaded;
    name.Clear();
    ReplayAddLoadName(name, *again, RL_COB, reloaded);
    printf("%s\n", ReplayFailure() == nullptr ? "no failure" : ReplayFailure());
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "the same holder has two different files in the cob load of scripts\\CORE2.COB") != nullptr, "a unit type that holds two different files in one recording fails the run");
    ReplayClearFailure();
    delete again;
    lines.Free();
    name.Free();
    delete table;

    printf("big files\n");
    unsigned char big[1100];
    memset(big, 0, sizeof big);
    MakeCobFile(big);
    AddLoadFile("scripts\\BIG.COB", big, sizeof big);
    ReplayLoadsReset();
    ReplayLoadHook() = NoteLoadHook;
    int* bigCob = (int*)LoadCobScript((char*)"scripts\\BIG.COB");
    ReplayLoadHook() = nullptr;
    defs[0].data = bigCob;
    table = new ReplayObjectTable;
    ReplayText bigName;
    ReplayAddLoadName(bigName, *table, RL_COB, bigCob);
    ReplayText bigLines;
    ReplayAddLoadLines(*table, bigLines);
    Check(strstr(bigLines.Data(), "[1100]\n  x 0:") != nullptr && strstr(bigLines.Data(), "  x 512:") == nullptr && strstr(bigLines.Data(), "  x 1024:") == nullptr, "a chunk of 512 bytes that is all zero is left out");
    bigName.Free();
    bigLines.Free();
    delete table;
}

void LoadBindSelfTest()
{
    printf("load binds\n");
    ResetBlock();
    ReplayClearFailure();
    BindLoadFiles();
    unsigned char file[256];
    memset(file, 0, sizeof file);
    Object3do* root = (Object3do*)file;
    root->firstFace = -1;
    root->faceCount = 3;
    root->vertices = (Vec3*)208;
    root->faces = (PieceFrame*)64;
    for (int i = 0; i < 3; i++) {
        PieceFrame* face = (PieceFrame*)(file + 64 + 32 * i);
        face->count = 1;
        face->indices = (unsigned short*)224;
    }
    AddLoadFile("objects3d\\bind.3do", file, sizeof file);
    ReplayLoadHook() = NoteLoadHook;
    Object3do* model = (Object3do*)Load3do((char*)"objects3d\\bind.3do");
    MirrorObject(model);
    ReplayLoadHook() = nullptr;
    Check(model != nullptr && model->faceCount == 3, "the model loads");
    unsigned short sequence[8] = { 4, 0, 0, 0, 0, 0, 0, 0 };
    *(unsigned*)&model->faces[1].flags = 0x1e;
    model->faces[1].color = sequence;
    model->faces[1].ptr_10 = 0x50003;
    model->faces[1].texKind = 7;
    *(unsigned*)&model->faces[2].flags = 0x1;
    model->faces[2].fillColor = 0xd1;
    model->faces[2].ptr_10 = (int)(size_t)((char*)model + 200);
    UnitDef* defs = (UnitDef*)Fresh(2 * (unsigned)sizeof(UnitDef));
    g_game->defs = defs;
    g_game->unitDefCount = 2;
    void** types = (void**)Fresh(2 * (unsigned)sizeof(void*));
    g_game->types = (Object3do**)types;
    types[1] = model;
    ReplayObjectTable* table = new ReplayObjectTable;
    ReplayText name;
    ReplayAddLoadName(name, *table, RL_MODEL, model);
    ReplayText lines;
    ReplayAddLoadLines(*table, lines);
    printf("%s", lines.Data());
    Check(strstr(lines.Data(), "  bind 96 30 FrameTable#1 0 =327683 7\n  bind 128 1 null 209 +200 0\n") != nullptr && strstr(lines.Data(), "  bind 64 ") == nullptr, "a face a bind set is a bind line with its offset, flags, sequence, fill colour, picture word and texture kind; an unbound face has none");
    name.Free();
    lines.Free();
    delete table;

    ResetBlock();
    BindLoadFiles();
    AddLoadFile("objects3d\\bind.3do", file, sizeof file);
    Object3do* again = (Object3do*)Load3do((char*)"objects3d\\bind.3do");
    MirrorObject(again);
    *(unsigned*)&again->faces[0].flags = 0x7;
    std::vector<ReplayLoadBind> binds;
    ReplayLoadBind one = { 96, 0x1e, "FrameTable#1", 0, "=327683", 7 };
    ReplayLoadBind two = { 128, 0x1, "null", 0xd1, "+200", 0 };
    binds.push_back(one);
    binds.push_back(two);
    BindModelFaces("bind", again, sizeof file, binds);
    void* made = g_objects.Pointer(ROK_FRAMETABLE, 1);
    Check((*(unsigned*)&again->faces[0].flags & 7) == 0 && *(unsigned*)&again->faces[1].flags == 0x1e && *(unsigned*)&again->faces[2].flags == 0x1, "the replayer clears the bits of every face and sets those of the lines");
    Check(made != nullptr && (void*)again->faces[1].color == made && again->faces[2].color == nullptr, "and names the object of a sequence as the services' answers do, FrameTable#1, so a later line resolves to it");
    Check(again->faces[1].ptr_10 == 0x50003 && again->faces[1].texKind == 7 && again->faces[2].fillColor == 0xd1 && again->faces[2].ptr_10 == (int)(size_t)((char*)again + 200), "and the fill colour, the picture word, a name inside the model and the texture kind");
    Check(ReplayFaceAt(again, sizeof file, 77) == nullptr && ReplayFaceAt(again, sizeof file, 96) == again->faces + 1, "a face is found by its offset in the file");
}

void LoadFormSelfTest()
{
    printf("load forms\n");
    ResetBlock();
    ReplayClearFailure();
    unsigned char first[16];
    unsigned char second[16];
    unsigned char shape[8];
    for (int i = 0; i < 16; i++) {
        first[i] = (unsigned char)(11 + i);
        second[i] = (unsigned char)(31 + i);
    }
    for (int i = 0; i < 8; i++)
        shape[i] = (unsigned char)(1 + i);
    ReplayLoadsReset();
    FixtureLane* lanes = (FixtureLane*)Fresh(4 * (unsigned)sizeof(FixtureLane));
    g_game->p1cd5 = (SideData*)(void*)lanes;
    void** types = (void**)Fresh(4 * (unsigned)sizeof(void*));
    g_game->types = (Object3do**)types;
    unsigned char* a = (unsigned char*)malloc(16);
    unsigned char* b = (unsigned char*)malloc(16);
    unsigned char* c = (unsigned char*)malloc(8);
    memcpy(a, first, 16);
    memcpy(b, second, 16);
    memcpy(c, shape, 8);
    ReplayLoadNote(RL_FIXTURE_SCRIPT, "files\\a.bin", a, 16);
    ReplayLoadNote(RL_FIXTURE_SCRIPT, "files\\b.bin", b, 16);
    ReplayLoadNote(RL_FIXTURE_SHAPE, "files\\c.bin", c, 8);
    lanes[2].script = a;
    lanes[3].script = b;
    types[1] = c;
    lanes[0].script = a + 4;
    lanes[0].shape = c + 2;

    unsigned short fields[2] = { FieldIndex(ROK_FIXTURELANE, "script"), FieldIndex(ROK_FIXTURELANE, "shape") };
    ReplaySlotSets sets = { "lanes", 0, nullptr, 0, nullptr, 0, fields, 2, fields, 2 };
    unsigned short kind = ROK_FIXTURELANE;
    void* root = &lanes[0];
    Recorder recorder;
    recorder.Record(&kind, &root, 1, sets);
    char* text = strdup(recorder.text.Data());
    printf("%s", text);
    Check(strstr(text, "  r FixtureLane#0.script FixtureLane#2+4\n") != nullptr && strstr(text, "  r FixtureLane#0.shape FixtureLane#1+2\n") != nullptr, "a field of kind load is the holder and the offset of the pointer it holds");
    root = &lanes[1];
    recorder.Record(&kind, &root, 1, sets);
    Check(strstr(recorder.text.Data(), "  r FixtureLane#1.script null\n") != nullptr, "and null when it holds none");
    lanes[0].script = b + 15;
    root = &lanes[0];
    recorder.Record(&kind, &root, 1, sets);
    Check(strstr(recorder.text.Data(), "  r FixtureLane#0.script FixtureLane#3+15\n") != nullptr, "the last byte of a file is in it");
    int stray = 0;
    lanes[0].script = &stray;
    recorder.Record(&kind, &root, 1, sets);
    printf("%s\n", ReplayFailure() == nullptr ? "no failure" : ReplayFailure());
    Check(ReplayFailure() != nullptr && strstr(ReplayFailure(), "a pointer into no fixture_script load") != nullptr, "a pointer that no loaded file holds fails the run");
    ReplayClearFailure();
    lanes[0].script = b + 15;
    ReplayText loadLines;
    ReplayAddLoadLines(*recorder.table, loadLines);
    char* loads = strdup(loadLines.Data());
    printf("%s", loads);
    Check(strstr(loads, "load fixture_script FixtureLane#2 \"files\\\\a.bin\" [16]\n  x 0:0b0c0d0e0f101112131415161718191a\n") != nullptr && strstr(loads, "load fixture_shape FixtureLane#1 \"files\\\\c.bin\" [8]\n") != nullptr, "the recording lists the loads the lines named");
    Check(strstr(loads, "load fixture_script FixtureLane#3 \"files\\\\b.bin\" [16]\n") != nullptr, "in the order of first use, and each once");
    loadLines.Free();

    ResetBlock();
    ReplayClearFailure();
    char body[1024];
    sprintf(body, "  r FixtureLane#0.script FixtureLane#2+4\n  r FixtureLane#0.shape FixtureLane#1+2\n  a script FixtureLane#2+4\n  a shape FixtureLane#1+2\n  o result 15\n");
    PrepareLoads(loads, body);
    bool found = false;
    const void* scriptBase = ReplayParseLoad(RL_FIXTURE_SCRIPT, "FixtureLane#2", &found);
    Check(found && scriptBase != nullptr && memcmp(scriptBase, first, 16) == 0, "the replayer registers the file of each load line and loads it");
    const void* otherBase = ReplayParseLoad(RL_FIXTURE_SCRIPT, "FixtureLane#3", &found);
    Check(found && otherBase != scriptBase && memcmp(otherBase, second, 16) == 0, "each under the holder the line names");
    ReplayParseLoad(RL_FIXTURE_SCRIPT, "FixtureLane#9", &found);
    Check(!found, "a holder that no load line names is not found");
    ReplayParseLoad(RL_FIXTURE_SCRIPT, "FixtureLane#1", &found);
    Check(!found, "nor a holder of another load kind");
    ReplayParseLoad(RL_FIXTURE_SCRIPT, "FixtureLane#2+16", &found);
    Check(!found, "nor an offset past the end of the file");
    const void* at = ReplayParseLoad(RL_FIXTURE_SCRIPT, "FixtureLane#2+9", &found);
    Check(found && at == (const char*)scriptBase + 9 && ReplayParseLoad(RL_FIXTURE_SCRIPT, "null", &found) == nullptr && found, "an offset is added to the base, and null is null");

    BeginCompare();
    BeginHeld();
    RestoreCallLines();
    Run_fixture_Attach();
    After_fixture_Attach();
    Check(g_diffs == 0, "a slot called with load arguments replays equal");
    Check(g_attachedScript == (const char*)scriptBase + 4 && g_attachedShape == ReplayParseLoad(RL_FIXTURE_SHAPE, "FixtureLane#1+2", &found), "the module got the base of each file and the offset");
    Check(ReplayFailure() == nullptr, "and no failure");
    FixtureLane* rebuilt = (FixtureLane*)g_game->p1cd5.get();
    Check(rebuilt != nullptr && rebuilt[0].script == (const char*)scriptBase + 4, "a load field is restored to the pointer");
    g_rootKinds[0] = ROK_FIXTURELANE;
    g_rootObjects[0] = &rebuilt[0];
    g_rootCount = 1;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "and compares equal");
    rebuilt[0].script = (void*)((const char*)scriptBase + 8);
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed offset: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "FixtureLane#0.script") == 0 && strstr(g_detail.Data(), "recorded FixtureLane#2+4, replayed FixtureLane#2+8") != nullptr, "a changed offset is reported at the field");
    rebuilt[0].script = (void*)((const char*)otherBase + 4);
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed unit type: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strstr(g_detail.Data(), "recorded FixtureLane#2+4, replayed FixtureLane#3+4") != nullptr, "and so is a changed holder");
    rebuilt[0].script = &stray;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strstr(g_detail.Data(), "recorded FixtureLane#2+4, replayed ?") != nullptr, "and a pointer into no load");
    rebuilt[0].script = nullptr;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 1 && strstr(g_detail.Data(), "recorded FixtureLane#2+4, replayed null") != nullptr, "and null");
    const ReplayObjectField* scriptField = ReplayFindObjectField(ROK_FIXTURELANE, "script");
    Check(scriptField != nullptr && !ReplayRestoreObjectField(*scriptField, &rebuilt[1], "FixtureLane#9+4", g_objects, AllocObject), "a field whose load no load line holds is refused");
    Check(!ReplayRestoreObjectField(*scriptField, &rebuilt[1], "?", g_objects, AllocObject) && !ReplayRestoreObjectField(*scriptField, &rebuilt[1], "FixtureLane#2+", g_objects, AllocObject), "and so is a value that is no load name");

    printf("a result\n");
    rebuilt[1].script = (void*)((const char*)otherBase + 6);
    PrepareLoads("", "  a lane 1\n  o result FixtureLane#3+6\n");
    BeginCompare();
    Run_fixture_Lookup();
    After_fixture_Lookup();
    Check(g_diffs == 0, "a load result compares by its name");
    PrepareLoads("", "  a lane 1\n  o result FixtureLane#3+7\n");
    BeginCompare();
    Run_fixture_Lookup();
    After_fixture_Lookup();
    printf("%s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "result") == 0 && strstr(g_detail.Data(), "recorded FixtureLane#3+7, replayed FixtureLane#3+6") != nullptr, "and a different one is reported at the result");

    printf("an import\n");
    BeginServices("  i fixture.Attach FixtureLane#2+4 FixtureLane#1+2 = 15\n");
    int result = Imp_fixture_Attach((void*)((const char*)scriptBase + 4), g_attachedShape);
    Check(g_diffs == 0 && result == 15, "a load argument of an import is compared by its name");
    BeginServices("  i fixture.Attach FixtureLane#2+4 FixtureLane#1+3 = 15\n");
    Imp_fixture_Attach((void*)((const char*)scriptBase + 4), g_attachedShape);
    printf("%s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 fixture.Attach") == 0 && strstr(g_detail.Data(), "FixtureLane#1+3") != nullptr, "and a different one is reported at the import");
    BeginServices("  i fixture.Lookup 1 = FixtureLane#3+6\n");
    Check(Imp_fixture_Lookup(1) == (const char*)otherBase + 6 && g_diffs == 0, "the result of an import is the pointer its name leads to");
    free(text);
    free(loads);
}


void HexOf(const unsigned char* bytes, unsigned count, char* out)
{
    for (unsigned i = 0; i < count; i++)
        sprintf(out + i * 2, "%02x", (unsigned)bytes[i]);
}

bool LoadsRefused(const char* loads, const char* want)
{
    jmp_buf jump;
    g_fatalWhat[0] = '\0';
    ForgetLoads();
    BindLoadFiles();
    if (setjmp(jump) == 0) {
        g_fatalTrap = &jump;
        PrepareLoads(loads, "  a x 1\n");
        g_fatalTrap = nullptr;
        return false;
    }
    g_fatalTrap = nullptr;
    printf("refused: %s\n", g_fatalWhat);
    return strstr(g_fatalWhat, want) != nullptr;
}

void LoadRefusalSelfTest()
{
    printf("load refusals\n");
    unsigned char cob[72];
    MakeCobFile(cob);
    char hex[200];
    HexOf(cob, 72, hex);
    char text[1024];
    sprintf(text, "load cob UnitDef#1 \"scripts\\\\A.COB\" [72]\n  x 0:%s\n", hex);
    Check(!LoadsRefused(text, "never"), "a well formed load line is read");
    sprintf(text, "load cob UnitDef#1 \"scripts\\\\A.COB\" [72]\n  x 0:%.8sZZ\n", hex);
    Check(LoadsRefused(text, "even number of lower case hex digits"), "a byte line with bad hex is refused");
    sprintf(text, "load cob UnitDef#1 \"scripts\\\\A.COB\" [72]\n  x 0:%.9s\n", hex);
    Check(LoadsRefused(text, "even number of lower case hex digits"), "and one of odd length");
    sprintf(text, "load cob UnitDef#1 \"scripts\\\\A.COB\" [72]\n  x 0:0B000000\n");
    Check(LoadsRefused(text, "even number of lower case hex digits"), "and upper case");
    sprintf(text, "load cob UnitDef#1 \"scripts\\\\A.COB\" [72]\n  x 0:%s\n  bind 8 1 null 0 null 0\n", hex);
    Check(LoadsRefused(text, "a bind line on a load that is not a model"), "a bind line on a cob load is refused");
    sprintf(text, "load cob UnitDef#1 \"scripts\\\\A.COB\" [72]\n  x 0:%s\nload cob UnitDef#1 \"scripts\\\\A.COB\" [72]\n  x 0:%s\n", hex, hex);
    Check(LoadsRefused(text, "a second load line for one holder"), "a second load line for one holder is refused");
    sprintf(text, "load cob UnitDef#1 \"scripts\\\\A.COB\" [72]\n  x 64:%s\n", hex);
    Check(LoadsRefused(text, "past the size of its file"), "and a byte line past the size of its file");
    ForgetLoads();
}

void LoadChunkSelfTest()
{
    printf("load chunks\n");
    ResetBlock();
    ReplayClearFailure();
    BindLoadFiles();
    unsigned char big[1100];
    memset(big, 0, sizeof big);
    MakeCobFile(big);
    big[1030] = 7;
    big[1099] = 9;
    AddLoadFile("scripts\\TAIL.COB", big, sizeof big);
    ReplayLoadsReset();
    ReplayLoadHook() = NoteLoadHook;
    int* cob = (int*)LoadCobScript((char*)"scripts\\TAIL.COB");
    ReplayLoadHook() = nullptr;
    UnitDef* defs = (UnitDef*)Fresh(2 * (unsigned)sizeof(UnitDef));
    g_game->defs = defs;
    g_game->unitDefCount = 2;
    defs[1].data = cob;
    ReplayObjectTable* table = new ReplayObjectTable;
    ReplayText name;
    ReplayAddLoadName(name, *table, RL_COB, cob);
    ReplayText lines;
    ReplayAddLoadLines(*table, lines);
    char* loads = strdup(lines.Data());
    Check(strstr(loads, "  x 512:") == nullptr && strstr(loads, "  x 1024:") != nullptr, "a chunk of zeros in the middle is left out, and the short last chunk is written");
    PrepareLoads(loads, "  a x 1\n");
    const LoadFileBytes* back = g_loadFiles.back();
    Check(back->bytes.size() == sizeof big && memcmp(&back->bytes[0], big, sizeof big) == 0, "the file reads back whole, with the zero chunk restored and the short last chunk in place");
    free(loads);
    name.Free();
    lines.Free();
    delete table;
}

void DebrisCacheSelfTest()
{
    printf("debris cache\n");
    ResetBlock();
    ReplayClearFailure();
    InitDebrisPieces();
    Check(g_debrisMemCache.AllocHandle(&g_explodedPieces[3], 200) == 1 && g_debrisMemCache.AllocHandle(&g_explodedPieces[5], 300) == 1, "two blocks are allocated");
    g_explodedPieces[5] = nullptr;
    ReplayCacheStart start;
    ReplayCaptureDebrisCache(start);
    ReplayText text;
    ReplayAddCacheLines(start, text);
    printf("%s", text.Data());
    Check(ReplayFailure() == nullptr && start.blocks.size() == 3 && start.blocks[0].owner == 3 && start.blocks[0].live && start.blocks[1].owner == 5 && !start.blocks[1].live && start.blocks[2].owner == -1, "the blocks of the cache are captured with their owners, and whether the owner still holds them");
    Check(start.cur == start.blocks[2].offset, "and the cursor");
    char* recorded = strdup(text.Data());
    text.Free();

    ResetBlock();
    BindLoadFiles();
    PrepareLoads(recorded, "  a x 1\n");
    InitDebrisForReplay("self-test");
    unsigned char* base = (unsigned char*)g_debrisMemCache.base.get();
    Check((unsigned char*)g_debrisMemCache.cur.get() == base + start.cur, "the replayer starts from the recorded cursor");
    Check((unsigned char*)g_explodedPieces[3].get() == base + 8 && g_explodedPieces[5].get() == nullptr, "a live slot points at its block and the others at none");
    DebrisCacheChunk* second = (DebrisCacheChunk*)(base + start.blocks[1].offset);
    Check((char*)second->owner.get() == (char*)&g_explodedPieces[5] && (unsigned)second->size == start.blocks[1].size, "the headers are the recorded ones, owner and size");
    g_game->ticks = 1;
    AdoptCachedPiece(kReplayObjectFields[FieldIndex(ROK_EXPLODEDSLOT, "piece")], &g_explodedPieces[3], "ExplodedPiece#4");
    Check(g_objects.Pointer(ROK_EXPLODEDPIECE, 4) == base + 8, "the object a line names for a live slot is its block");

    char bad[256];
    sprintf(bad, "cache debris 100000 0\n  block 0 50000 null\n");
    Check(LoadsRefused(bad, "do not tile"), "blocks that do not tile the cache are refused");
    sprintf(bad, "cache debris 100000 5\n  block 0 100000 null\n");
    Check(LoadsRefused(bad, "do not tile its size, or its cursor"), "and a cursor inside a block");
    free(recorded);
}

void DebrisCacheChangeSelfTest()
{
    printf("debris cache changes\n");
    ResetBlock();
    ReplayClearFailure();
    InitDebrisPieces();
    ReplayCacheStart before;
    ReplayCacheStart after;
    ReplayText text;
    Check(ReplayReadDebrisCache(before) == nullptr, "the installed cache is read");
    ReplayAddCacheChanges(before, before, text);
    Check(text.Size() == 0, "a call that leaves the cache alone has no d line");
    Check(g_debrisMemCache.AllocHandle(&g_explodedPieces[3], 200) == 1, "a block is allocated");
    Check(ReplayReadDebrisCache(after) == nullptr, "and the cache read again");
    ReplayAddCacheChanges(before, after, text);
    printf("%s", text.Data());
    Check(strcmp(text.Data(), "  d cache 100000 208\n  d block 0 208 ExplodedSlot#3\n  d block 208 99792 null\n") == 0, "an allocation is the cursor, its block with the 8-byte header, and the rest split off");

    unsigned char* base = (unsigned char*)g_debrisMemCache.base.get();
    DebrisCacheChunk* first = (DebrisCacheChunk*)base;
    first->owner = (Ref<char>*)&g_explodedPieces[5];
    first->size = 99900;
    DebrisCacheChunk* last = (DebrisCacheChunk*)(base + 99900);
    last->owner = nullptr;
    last->size = 100;
    g_explodedPieces[3] = nullptr;
    g_explodedPieces[5] = (ExplodedPiece*)(base + 8);
    g_debrisMemCache.cur = last;
    Check(ReplayReadDebrisCache(before) == nullptr, "a cache whose cursor is near the end of the arena is read");
    Check(g_debrisMemCache.AllocHandle(&g_explodedPieces[3], 200) == 1 && ReplayReadDebrisCache(after) == nullptr, "a block is allocated past its end");
    text.Clear();
    ReplayAddCacheChanges(before, after, text);
    printf("%s", text.Data());
    Check(strcmp(text.Data(), "  d cache 100000 208\n  d block 0 208 ExplodedSlot#3\n  d block 208 99692 null\n") == 0, "the allocation wraps to the start of the arena, and the block at the end is unchanged");
    Check(g_explodedPieces[5].get() == nullptr && (unsigned char*)g_explodedPieces[3].get() == base + 8, "and it evicts the piece whose block it takes");

    first->size = 4;
    Check(ReplayReadDebrisCache(after) != nullptr, "blocks that do not tile the arena are refused");
    text.Free();
}

char* ReadWholeFile(const char* file, unsigned* size)
{
    FILE* f = fopen(file, "rb");
    if (f == nullptr)
        Fatal(file, 0, "cannot read", file);
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* text = (char*)malloc((unsigned)length + 1);
    *size = (unsigned)fread(text, 1, (unsigned)length, f);
    fclose(f);
    text[*size] = '\0';
    return text;
}

void ReplayChangedText(const char* text, unsigned size, Outcome& out)
{
    const char* changed = "replay_changed.tmp";
    FILE* f = fopen(changed, "wb");
    fwrite(text, 1, size, f);
    fclose(f);
    ReplayFile(changed, out);
    remove(changed);
    printf("changed: call %d, %s\n", out.call, out.key);
}

void LoadByteFlipSelfTest(const char* file)
{
    printf("a flipped load byte\n");
    unsigned size = 0;
    char* text = ReadWholeFile(file, &size);
    char* setCob = strstr(text, " SetCob\n");
    char* data = setCob == nullptr ? nullptr : strstr(setCob, "\n  a data ");
    char holder[64] = "";
    if (data != nullptr)
        sscanf(data + 10, "%40s", holder);
    char want[80];
    sprintf(want, "\nload cob %s ", holder);
    char* load = holder[0] == '\0' ? nullptr : strstr(text, want);
    char* chunk = load == nullptr ? nullptr : strstr(load, "\n  x ");
    char* end = chunk == nullptr ? nullptr : strchr(chunk + 1, '\n');
    if (end == nullptr)
        Fatal(file, 0, "a scripts recording with a SetCob call and the load of its script", "load cob");
    Outcome same;
    ReplayFile(file, same);
    Check(!same.diverged, "the recording replays equal");
    end[-1] = end[-1] == '0' ? '1' : '0';
    Outcome changed;
    ReplayChangedText(text, size, changed);
    Check(changed.diverged && strstr(changed.key, "checksum") != nullptr, "a changed byte of a loaded file is reported at the field the script's load computes from it");
    free(text);
}

void SetCobWriteSelfTest(const char* file)
{
    printf("a wrong write of SetCob\n");
    unsigned size = 0;
    char* text = ReadWholeFile(file, &size);
    char* setCob = strstr(text, " SetCob\n");
    char* next = setCob == nullptr ? nullptr : strstr(setCob + 1, "\ncall ");
    char* pieces = setCob == nullptr ? nullptr : strstr(setCob, ".pieces [");
    char* bytes = pieces == nullptr ? nullptr : strstr(pieces, "] 0:");
    if (bytes == nullptr || (next != nullptr && bytes > next))
        Fatal(file, 0, "a scripts recording whose SetCob call writes the pieces", "SetCob");
    int call = -1;
    for (char* at = strstr(text, "\ncall "); at != nullptr && at < setCob; at = strstr(at + 1, "\ncall "))
        call++;
    bytes[4] = bytes[4] == '0' ? '1' : '0';
    Outcome out;
    ReplayChangedText(text, size, out);
    Check(out.diverged && out.call == call && strstr(out.key, ".pieces") != nullptr, "a write of SetCob's script that the call does not make is reported at the call and the field");
    free(text);
}

char* Splice(const char* text, unsigned size, const char* at, unsigned removed, const char* insert, unsigned* spliced)
{
    unsigned head = (unsigned)(at - text);
    unsigned added = (unsigned)strlen(insert);
    *spliced = size - removed + added;
    char* out = (char*)malloc(*spliced + 1);
    memcpy(out, text, head);
    memcpy(out + head, insert, added);
    memcpy(out + head + added, at + removed, size - head - removed);
    out[*spliced] = '\0';
    return out;
}

int CallOfLine(const char* text, const char* at)
{
    int call = -1;
    for (const char* p = strstr(text, "\ncall "); p != nullptr && p < at; p = strstr(p + 1, "\ncall "))
        call++;
    return call;
}

void DebrisBlockSizeSelfTest(const char* file)
{
    printf("a wrong block size of the debris cache\n");
    unsigned size = 0;
    char* text = ReadWholeFile(file, &size);
    char* line = strstr(text, "\n  d block ");
    unsigned offset = 0;
    unsigned blockSize = 0;
    int used = 0;
    if (line == nullptr || sscanf(line, "\n  d block %u %u%n", &offset, &blockSize, &used) != 2)
        Fatal(file, 0, "a weapons recording whose calls change the debris cache", "d block");
    char replaced[48];
    sprintf(replaced, "\n  d block %u %u", offset, blockSize + 8);
    unsigned length = 0;
    char* changed = Splice(text, size, line, (unsigned)used, replaced, &length);
    Outcome out;
    ReplayChangedText(changed, length, out);
    Check(out.diverged && out.call == CallOfLine(text, line) && strcmp(out.key, "cache debris") == 0, "a block of another size than the game's is reported at the call that allocated it");
    free(changed);
    free(text);
}

void DebrisWrapSelfTest(const char* file)
{
    printf("a debris cache that wraps\n");
    unsigned size = 0;
    char* text = ReadWholeFile(file, &size);
    const char* empty = "\ncache debris 100000 0\n  block 0 100000 null\n";
    char* start = strstr(text, empty);
    char* line = strstr(text, "\n  d block 0 ");
    unsigned blockSize = 0;
    char owner[40] = "";
    if (start == nullptr || line == nullptr || sscanf(line, "\n  d block 0 %u %39s", &blockSize, owner) != 2 || blockSize >= 99992)
        Fatal(file, 0, "a weapons recording that starts from an empty debris cache and allocates from it", "cache debris");
    unsigned length = 0;
    char* changed = Splice(text, size, start, (unsigned)strlen(empty), "\ncache debris 100000 99992\n  block 0 99992 null\n  block 99992 8 null\n", &length);
    Outcome out;
    ReplayChangedText(changed, length, out);
    char want[160];
    sprintf(want, "replayed cache 100000 %u; block 0 %u %s; block %u %u null\n", blockSize, blockSize, owner, blockSize, 99992 - blockSize);
    Check(out.diverged && out.call == CallOfLine(text, line) && strcmp(out.key, "cache debris") == 0 && strstr(out.log.Data(), want) != nullptr,
          "a start cursor near the end of the arena makes the first allocation wrap to its start, which leaves a smaller rest than the game's");
    free(changed);
    free(text);
}

void TeamColourSourceSelfTest(const char* file)
{
    printf("a team colour face without its sequence\n");
    unsigned size = 0;
    char* text = ReadWholeFile(file, &size);
    char* line = nullptr;
    char* name = nullptr;
    for (char* at = strstr(text, ".texTeamSrc FrameTable#"); at != nullptr && line == nullptr; at = strstr(at + 1, ".texTeamSrc FrameTable#")) {
        name = at;
        while (name > text && name[-1] != ' ')
            name--;
        if (name - text >= 5 && strncmp(name - 5, "\n  w ", 5) == 0)
            line = at;
    }
    char* end = line == nullptr ? nullptr : strchr(line, '\n');
    if (end == nullptr)
        Fatal(file, 0, "a weapons recording whose debris has a team colour face", "texTeamSrc");
    char key[96];
    sprintf(key, "%.*s", (int)(line - name) + 11, name);
    const char* value = line + 12;
    unsigned length = 0;
    char* changed = Splice(text, size, value, (unsigned)(end - value), "null", &length);
    Outcome out;
    ReplayChangedText(changed, length, out);
    Check(out.diverged && out.call == CallOfLine(text, line) && strcmp(out.key, key) == 0, "a team colour face whose sequence is not copied is reported at the face");
    free(changed);
    free(text);
}

void UnreadArgumentSelfTest()
{
    printf("an argument the callee does not read\n");
    ResetBlock();
    const char* body = "  i los_radar.GetMapCellAtPosition pos=(1048576,77,2097152) = null\n";
    Vec3 pos;
    pos.x = 1048576;
    pos.y = -5;
    pos.z = 2097152;
    BeginServices(body);
    Imp_los_radar_GetMapCellAtPosition(&pos);
    Check(g_diffs == 0, "the y of a cell lookup's position is not compared");
    pos.z = 2097153;
    BeginServices(body);
    Imp_los_radar_GetMapCellAtPosition(&pos);
    printf("a changed z: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 los_radar.GetMapCellAtPosition") == 0, "its x and z are");
    pos.z = 2097152;
    BeginServices("  i los_radar.GetOriginCellAtPosition pos=(1048576,77,2097152) = null\n");
    Imp_los_radar_GetOriginCellAtPosition(&pos);
    printf("a changed y of another lookup: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "import 0 los_radar.GetOriginCellAtPosition") == 0, "the y of any other import is compared");
}

int g_askIndex;

void AskGafFrame(bool after)
{
    if (g_game->losTable == nullptr)
        return;
    GetGafFrame(g_game->losTable, g_askIndex);
    if (after)
        GetGafFrame(g_game->losTable, g_askIndex);
}

void GafQuerySelfTest(const char* file)
{
    printf("GetGafFrame answered as a query\n");
    unsigned size = 0;
    char* text = ReadWholeFile(file, &size);
    const char* first = strstr(text, "\n  i services.GetGafFrame FrameTable#");
    int index = -1;
    if (first != nullptr)
        sscanf(first, "\n  i services.GetGafFrame FrameTable#%*d %d", &index);
    Check(index >= 0, "the recording asks for a frame");
    if (index < 0) {
        free(text);
        return;
    }
    Outcome out;
    g_queryAnswers = 0;
    ReplayFile(file, out);
    int recorded = g_queryAnswers;
    printf("in the recorded order: %d calls, %d frames, %s\n", out.calls, recorded, out.diverged ? out.key : "equal");
    Check(!out.diverged && recorded > 0, "a module that asks for its frames in the recorded order is equal");

    g_aroundSlot = AskGafFrame;
    g_askIndex = index;
    g_queryAnswers = 0;
    ReplayFile(file, out);
    printf("asked before and twice after each slot's imports: %d frames, %s\n", g_queryAnswers, out.diverged ? out.key : "equal");
    Check(!out.diverged && g_queryAnswers > recorded, "a module that asks more often and in another order is equal");
    g_askIndex = 0x7fff;
    ReplayFile(file, out);
    printf("asked for a frame no line answers: call %d, %s\n", out.call, out.key);
    Check(out.diverged && strcmp(out.key, "query services.GetGafFrame") == 0, "a frame the recording has no answer to is reported at the query");
    g_aroundSlot = nullptr;

    const char* end = first + 1;
    do
        end = strchr(end, '\n') + 1;
    while (strncmp(end, "  o GafFrame#", 13) == 0);
    unsigned length = (unsigned)(end - (first + 1));
    char* block = (char*)malloc(length + 1);
    memcpy(block, first + 1, length);
    block[length] = '\0';
    const char* at = strstr(text, "\n  i ");
    while (at != nullptr && IsQueryLine(at + 1))
        at = strstr(at + 1, "\n  i ");
    Check(at != nullptr, "the recording has an import judged in order");
    if (at == nullptr) {
        free(block);
        free(text);
        return;
    }
    at++;
    unsigned changedLength = 0;
    char* changed = Splice(text, size, at, 0, block, &changedLength);
    g_queryAnswers = 0;
    ReplayChangedText(changed, changedLength, out);
    printf("an unasked frame before an import judged in order: %d frames, %s\n", g_queryAnswers, out.diverged ? out.key : "equal");
    Check(!out.diverged && g_queryAnswers == recorded, "a module that asks less often than the recording does is equal");
    free(changed);
    free(block);
    free(text);
}

int SelfTest(const char* file)
{
    static const char* const names[] = { "", "an import with other arguments", "an extra import", "a missing import" };
    ObjectSelfTest();
    TableSelfTest();
    ArraySelfTest();
    FormSelfTest();
    ShapeSelfTest();
    EnemyListSelfTest();
    InOutRunnerSelfTest();
    OutFormsSelfTest();
    WeaponArgumentSelfTest();
    PolymorphicSelfTest();
    RefsSelfTest();
    GoalSelfTest();
    BasedTableSelfTest();
    ElementSiteSelfTest();
    EmbedsSelfTest();
    CountedEmbedsSelfTest();
    EmbeddedPolymorphicSelfTest();
    CallbackSelfTest();
    HeldCallbackSelfTest();
    VectorSelfTest();
    SnapshotSelfTest();
    StringSelfTest();
    FilesSelfTest();
    CommandsSelfTest();
    SimPlumbingSelfTest();
    LoadReadsSelfTest();
    GafCursorSelfTest();
    RawPointerSelfTest();
    MaskedViewSelfTest();
    EmbeddedSiteSelfTest();
    FeatureTableSelfTest();
    UnnamedPtrSelfTest();
    AllOfListSelfTest();
    DeltaTableSelfTest();
    DeltaLimitSelfTest();
    UnconditionedUnitIdSelfTest();
    IdReachSelfTest();
    GameRefSelfTest();
    GamePtrSelfTest();
    ExecuteCommandSelfTest();
    CapacityTableSelfTest();
    TrackedCountSelfTest();
    SelfPointerSelfTest();
    CStringSelfTest();
    BlobSelfTest();
    SharedBlobSelfTest();
    LoadInputsSelfTest();
    LoadBindSelfTest();
    LoadFormSelfTest();
    LoadRefusalSelfTest();
    LoadChunkSelfTest();
    DebrisCacheSelfTest();
    DebrisCacheChangeSelfTest();
    EmptyTargetSelfTest();
    RebindSelfTest();
    RebindToFreshAddressesSelfTest();
    NestedVectorSelfTest();
    SimVectorSelfTest();
    GameWriteSelfTest();
    RegionSelfTest();
    LoadDataMapSelfTest();
    LoadDataReclaimSelfTest();
    LoadDataClassWordsSelfTest();
    UnreadArgumentSelfTest();
    if (file == nullptr)
        return 0;
    if (strstr(file, "los_radar") != nullptr) {
        GafQuerySelfTest(file);
        return 0;
    }
    if (strstr(file, "scripts") != nullptr) {
        LoadByteFlipSelfTest(file);
        SetCobWriteSelfTest(file);
        return 0;
    }
    if (strstr(file, "weapons") != nullptr) {
        DebrisBlockSizeSelfTest(file);
        DebrisWrapSelfTest(file);
        TeamColourSourceSelfTest(file);
        return 0;
    }
    for (int mutation = MUT_ARGUMENT; mutation <= MUT_MISSING; mutation++) {
        Outcome out;
        ReplayFile(file, out, mutation);
        const char* tail = strstr(out.log.Data(), "DIFFERS");
        printf("%s: call %d, %s\n%s", names[mutation], out.call, out.key, tail == nullptr ? "" : tail);
        Check(out.diverged, "the mutated recording is reported");
        Check(out.call == g_mutationCall, "the mutation is reported at its call");
        Check(strcmp(out.key, g_mutationKey) == 0, "the mutation is reported at its import");
    }
    return 0;
}

FILE* g_coreOut;

void CoreError(const char* errorClass, const char* text)
{
    const char* subsystem = ReplayWasmSubsystem();
    if (subsystem == nullptr)
        subsystem = "";
    if (strcmp(errorClass, "import") == 0 || strcmp(errorClass, "glue") == 0 || strcmp(errorClass, "fatal") == 0) {
        if (strcmp(errorClass, "import") == 0)
            text = ReplayWasmRefusal(text);
        fprintf(g_coreOut, "replay\t{\"error\": \"%s\", \"modules\": [\"%s\"], \"message\": \"", errorClass, subsystem);
        for (const char* p = text; *p != '\0' && *p != '\r' && *p != '\n'; p++) {
            unsigned char c = (unsigned char)*p;
            if (c == '"' || c == '\\')
                fprintf(g_coreOut, "\\%c", c);
            else if (c < 0x20 || c >= 0x7f)
                fprintf(g_coreOut, "\\u%04x", c);
            else
                fputc(c, g_coreOut);
        }
        fputs("\"}\n", g_coreOut);
        fclose(g_coreOut);
        exit(0);
    }
    fprintf(g_coreOut, "replay\t{\"error\": \"%s\", \"modules\": [\"%s\"]}\n", errorClass, subsystem);
    fclose(g_coreOut);
    exit(0);
}

const char* CoreSubsystem(const char* recording)
{
    static char subsystem[64];
    char line[160];
    strcpy(subsystem, "wind_meteors");
    FILE* file = fopen(recording, "rb");
    if (file == nullptr)
        return subsystem;
    if (fgets(line, sizeof line, file) != nullptr && fgets(line, sizeof line, file) != nullptr)
        sscanf(line, "subsystem %63s", subsystem);
    fclose(file);
    return subsystem;
}

int CoreHost(const char* callsPath, const char* outPath)
{
    FILE* calls = fopen(callsPath, "rb");
    if (calls == nullptr) {
        printf("FAIL: cannot read %s\n", callsPath);
        return 2;
    }
    g_coreOut = fopen(outPath, "wb");
    if (g_coreOut == nullptr) {
        printf("FAIL: cannot write %s\n", outPath);
        return 2;
    }
    const char* fault = getenv("BT_CORE_FAULT");
    g_core = true;
    g_coreFault = fault != nullptr && strcmp(fault, "host_write") == 0;
    ReplayWasmNoBudgets();
    ReplayWasmSetTrapHook(CoreError);
    char line[1024];
    while (fgets(line, sizeof line, calls) != nullptr) {
        size_t length = strlen(line);
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r'))
            line[--length] = '\0';
        if (length == 0)
            continue;
        char* recording = strchr(line, '\t');
        char* module = recording == nullptr ? nullptr : strchr(recording + 1, '\t');
        if (strncmp(line, "replay\t", 7) != 0 || module == nullptr) {
            printf("FAIL: not a replay call: %s\n", line);
            return 2;
        }
        *module++ = '\0';
        recording++;
        ReplayWasmSelect(module, CoreSubsystem(recording));
        Outcome out;
        ReplayFile(recording, out);
        fprintf(g_coreOut, "replay\t[%s]\n", out.core.Data());
        fflush(g_coreOut);
    }
    fclose(g_coreOut);
    fclose(calls);
    return 0;
}

int main(int argc, char** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    ReplayBindCallbackHook() = BindHeld;
    int expectCall = -1;
    const char* expectField = nullptr;
    int arg = 1;
    if (arg + 3 < argc && strcmp(argv[arg], "--core") == 0) {
        if (strcmp(argv[arg + 1], "host") != 0) {
            printf("FAIL: no core group %s\n", argv[arg + 1]);
            return 2;
        }
        return CoreHost(argv[arg + 2], argv[arg + 3]);
    }
    if (arg + 1 < argc && strcmp(argv[arg], "--wasm") == 0) {
        ReplayWasmSelect(argv[arg + 1]);
        printf("replaying against the WASM module %s\n", argv[arg + 1]);
        arg += 2;
        if (arg < argc && strcmp(argv[arg], "--no-budgets") == 0) {
            ReplayWasmNoBudgets();
            printf("an entry with no fuel_budget runs with the largest, %ld\n", 2147483647L);
            arg++;
        }
    }
    if (arg < argc && strcmp(argv[arg], "--self-test") == 0) {
        SelfTest(arg + 1 < argc ? argv[arg + 1] : nullptr);
        arg = argc;
    } else if (arg + 2 < argc && strcmp(argv[arg], "--expect") == 0) {
        expectCall = atoi(argv[arg + 1]);
        expectField = argv[arg + 2];
        arg += 3;
    } else if (arg >= argc) {
        printf("usage: replay_native_test [--wasm MODULE.wasm [--no-budgets]] [--self-test [RECORDING] | --expect CALL FIELD RECORDING...] RECORDING...\n       replay_native_test --core host CALLS OUT\n");
        return 2;
    }
    for (; arg < argc; arg++)
        Replay(argv[arg], expectCall, expectField);
    if (g_failures == 0) {
        printf("replay_native_test: all passed\n");
        return 0;
    }
    printf("replay_native_test: %d failed\n", g_failures);
    return 1;
}

void __cdecl operator delete(void* p)
{
    if (SimOwns(p)) {
        SimFree(p);
        return;
    }
    free(p);
}

void __cdecl GameFreeThunk(void* p)
{
    SimFree(p);
}

void __stdcall ReleaseUnitPictures(ModelState* state)
{
}

int __stdcall GetCobChecksum(int key);

int __stdcall GetCobChecksum(void* param)
{
    return GetCobChecksum((int)param);
}

Vec3f __stdcall VectorFromTo(Vec3f from, Vec3f to)
{
    Vec3f r;
    r.x = to.x - from.x;
    r.y = to.y - from.y;
    r.z = to.z - from.z;
    return r;
}

Vec3f __stdcall CrossProduct(Vec3f a, Vec3f b)
{
    double yz = (double)a.y * b.z;
    double zy = (double)a.z * b.y;
    double zx = (double)a.z * b.x;
    double xz = (double)a.x * b.z;
    double xy = (double)a.x * b.y;
    double yx = (double)a.y * b.x;
    Vec3f r;
    r.z = (float)(xy - yx);
    r.x = (float)(yz - zy);
    r.y = (float)(zx - xz);
    return r;
}

Vec3f __stdcall NormalizeVector(Vec3f v)
{
    double sx = (double)v.x * v.x;
    double sy = (double)v.y * v.y;
    double sz = (double)v.z * v.z;
    double len = sqrt(sx + sy + sz);
    v.x /= len;
    v.y /= len;
    v.z /= len;
    return v;
}

void __stdcall RotateByAngles(Vec3* in, Vec3* out, short* angles)
{
    int a[2];
    int b[2];
    a[0] = in->x;
    a[1] = in->y;
    RotatePoint2D(angles[0], a);
    b[0] = a[1];
    b[1] = in->z;
    RotatePoint2D(angles[2], b);
    out->y = b[0];
    a[1] = b[1];
    RotatePoint2D(angles[1], a);
    out->x = a[0];
    out->z = a[1];
}
