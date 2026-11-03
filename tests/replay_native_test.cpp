#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "../src/state/game_rand.h"

#pragma pack(push, 1)
#include "../src/game/game.h"
#pragma pack(pop)

#include "../src/weapons/weapon_def.h"
#include "../src/util/vec3.h"
#include "../src/files/files_bank.h"
#include "../src/util/commands.h"
#include "../interfaces/services.h"
#include "../interfaces/seams.h"
#include "../interfaces/wind_meteors.h"
#include "../interfaces/weapons.h"
#include "../interfaces/files.h"
#include "../interfaces/rand_stream.h"
#include "../src/state/sim_globals.h"
#include "../src/state/block.h"
#include "../src/map/meteors.h"
#include "../src/game/game_state.h"
#include "../src/game/console_commands.h"
#include "../src/harness/replay_text.h"
#include "../src/harness/replay_schema.h"
#include "replay_wasm.h"

extern "C" int __cdecl bx_rand(void* state);
extern "C" int __cdecl bx_ScaledSin(short angle, int scale);
extern "C" int __cdecl bx_ScaledCos(short angle, int scale);


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

Game* g_game;
SimGlobals* g_simGlobals;
char g_extTdf[4];
Seams g_seams;

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

namespace {

enum { MAX_LINES = 8192, MAX_OUTS = 8, MAX_RECORDS = 64, TEXT = 512 };
enum ObjectKind { OBJ_FILE, OBJ_RECORD, OBJ_BANK };

enum ImportId {
    IMP_GAME_RAND,
    IMP_RANDOM_INT,
    IMP_FATAL_ERROR,
    IMP_BUILD_DATA_PATH,
    IMP_TDF_CONSTRUCT,
    IMP_TDF_DESTRUCT,
    IMP_TDF_LOAD_FILE,
    IMP_TDF_SELECT_RECORD,
    IMP_TDF_GET_INT,
    IMP_TDF_GET_DOUBLE,
    IMP_TDF_GET_STRING,
    IMP_GET_INT_ARG,
    IMP_BANK_OPEN_ACCOUNT,
    IMP_BANK_SET_INTEGER,
    IMP_BANK_GET_INTEGER,
    IMP_SPAWN_PROJECTILE,
    IMP_FIND_WEAPON
};

const char* const kImportNames[] = {
    "services.GameRand", "services.RandomInt", "services.FatalError", "services.BuildDataPath",
    "services.TdfFile_Construct", "services.TdfFile_Destruct", "services.TdfFile_LoadFile",
    "services.TdfFile_SelectRecord", "services.TdfRecord_GetFieldInt", "services.TdfRecord_GetFieldDouble",
    "services.TdfRecord_GetFieldString", "services.CommandArgs_GetIntArg", "services.HapiBank_OpenAccount",
    "services.HapiBank_SetIntegerItem", "services.HapiBank_GetIntegerItem", "weapons.SpawnProjectile",
    "weapons.FindWeaponByName",
};

int g_failures;

void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

void Fatal(const char* file, int call, const char* what, const char* text)
{
    printf("FAIL: %s: call %d: %s: %s\n", file, call, what, text);
    exit(2);
}

struct Recorded {
    char name[48];
    char value[192];
};

Recorded g_recorded[64];
int g_recordedCount;

Recorded* FindRecorded(const char* name)
{
    for (int i = 0; i < g_recordedCount; i++) {
        if (strcmp(g_recorded[i].name, name) == 0)
            return &g_recorded[i];
    }
    return nullptr;
}

void SetRecorded(const char* name, const char* value)
{
    Recorded* r = FindRecorded(name);
    if (r == nullptr) {
        if (g_recordedCount == 64 || strlen(name) >= sizeof r->name)
            abort();
        r = &g_recorded[g_recordedCount++];
        strcpy(r->name, name);
    }
    if (strlen(value) >= sizeof r->value)
        abort();
    strcpy(r->value, value);
}

char* g_text;
char* g_lines[MAX_LINES];
int g_lineCount;

int g_callIndex;
int g_callFirst;
int g_callEnd;
int g_cursor;
int g_importIndex;
int g_diffs;
char g_firstKey[96];
ReplayText g_detail;
ReplayOrdinals g_ordinals;
const char* g_outNames[MAX_OUTS];
const char* g_outValues[MAX_OUTS];
int g_outCount;
unsigned g_hash;
int g_importsTotal;

char g_fakeRecords[MAX_RECORDS][16];
HapiBank g_bank;

void Diff(const char* key, const char* recorded, const char* replayed)
{
    if (g_diffs++ == 0)
        strncpy(g_firstKey, key, sizeof g_firstKey - 1);
    g_detail.Add("    ");
    g_detail.Add(key);
    g_detail.Add(": recorded ");
    g_detail.Add(recorded);
    g_detail.Add(", replayed ");
    g_detail.Add(replayed);
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

void AddFloatBits(ReplayText& t, float value)
{
    unsigned bits;
    memcpy(&bits, &value, 4);
    t.AddF32(bits);
}

void AddDoubleBits(ReplayText& t, double value)
{
    unsigned bits[2];
    memcpy(bits, &value, 8);
    t.AddF64(bits[0], bits[1]);
}

void AddVec3(ReplayText& t, const Vec3* v)
{
    t.AddChar('(');
    t.AddInt(v->x);
    t.AddChar(',');
    t.AddInt(v->y);
    t.AddChar(',');
    t.AddInt(v->z);
    t.AddChar(')');
}

void AddStream(ReplayText& t, int stream)
{
    if (stream == RAND_STREAM_MAIN)
        t.Add("main");
    else if (stream == RAND_STREAM_LOAD)
        t.Add("load");
    else
        t.AddInt(stream);
}

void AddWeapon(ReplayText& t, const void* p)
{
    if (p == nullptr) {
        t.Add("null");
        return;
    }
    const unsigned char* base = (const unsigned char*)g_game->weapons;
    const unsigned char* q = (const unsigned char*)p;
    unsigned span = sizeof(WeaponDef);
    if (q >= base && q < base + 256 * span && (unsigned)(q - base) % span == 0) {
        t.Add("weapondef#");
        t.AddUnsigned((unsigned)(q - base) / span);
    } else {
        t.Add("weapondef#?");
    }
}

WeaponDef* ParseWeapon(const char* value)
{
    const char* p = Skip(value, "weapondef#");
    if (p == nullptr)
        return nullptr;
    unsigned index = (unsigned)atoi(p);
    return index < 256 ? &g_game->weapons[index] : nullptr;
}

void AddObject(ReplayText& t, unsigned kind, const void* p)
{
    static const char* const names[] = { "file#", "rec#", "bank#" };
    unsigned ordinal;
    if (kind == OBJ_RECORD) {
        ordinal = 0;
        for (int i = 0; i < MAX_RECORDS; i++) {
            if (p == g_fakeRecords[i])
                ordinal = (unsigned)i + 1;
        }
    } else {
        ordinal = g_ordinals.Of(kind, p);
    }
    if (ordinal == 0) {
        t.Add("null");
        return;
    }
    t.Add(names[kind]);
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

const char* Block(const ReplayField& f)
{
    if (f.base == RB_GAME)
        return (const char*)g_game + f.offset;
    return (const char*)g_simGlobals + f.offset;
}

#pragma push_macro("g_randomSeed")
#undef g_randomSeed

unsigned* SeedWord()
{
    return &g_simGlobals->g_randomSeed;
}

#pragma pop_macro("g_randomSeed")

void FormatField(const ReplayField& f, ReplayText& out)
{
    out.Clear();
    if (f.base == RB_RNG) {
        out.AddHex32(f.offset == 2 ? *SeedWord() : *SimRandStream((int)f.offset));
        return;
    }
    const unsigned char* at = (const unsigned char*)Block(f);
    switch (f.kind) {
    case RK_I32:
        out.AddInt(*(const int*)at);
        break;
    case RK_U32:
        out.AddUnsigned(*(const unsigned*)at);
        break;
    case RK_U16:
        out.AddUnsigned(*(const unsigned short*)at);
        break;
    case RK_F32:
        out.AddF32(*(const unsigned*)at);
        break;
    case RK_POINT16:
        out.AddChar('(');
        out.AddInt(((const short*)at)[0]);
        out.AddChar(',');
        out.AddInt(((const short*)at)[1]);
        out.AddChar(')');
        break;
    case RK_STRING: {
        char text[64];
        unsigned size = f.size < sizeof text - 1 ? f.size : (unsigned)sizeof text - 1;
        memcpy(text, at, size);
        text[size] = '\0';
        out.AddString(text);
        break;
    }
    default: {
        unsigned stored = *(const unsigned*)at;
        AddWeapon(out, stored == 0 ? nullptr : (const unsigned char*)SimBlockBase() + stored);
        break;
    }
    }
}

bool RestoreField(const ReplayField& f, const char* value)
{
    if (f.base == RB_RNG) {
        unsigned bits = ParseHex(value + 2, 8);
        if (f.offset == 2)
            *SeedWord() = bits;
        else
            *SimRandStream((int)f.offset) = bits;
        return true;
    }
    unsigned char* at = (unsigned char*)Block(f);
    switch (f.kind) {
    case RK_I32:
        *(int*)at = atoi(value);
        break;
    case RK_U32:
        *(unsigned*)at = strtoul(value, nullptr, 10);
        break;
    case RK_U16:
        *(unsigned short*)at = (unsigned short)strtoul(value, nullptr, 10);
        break;
    case RK_F32:
        *(unsigned*)at = ParseF32(value);
        break;
    case RK_POINT16: {
        int x = 0;
        int y = 0;
        sscanf(value, "(%d,%d)", &x, &y);
        ((short*)at)[0] = (short)x;
        ((short*)at)[1] = (short)y;
        break;
    }
    case RK_STRING:
        memset(at, 0, f.size);
        ParseQuoted(value, (char*)at, f.size);
        break;
    default: {
        WeaponDef* weapon = ParseWeapon(value);
        if (weapon == nullptr && strcmp(value, "null") != 0)
            return false;
        *(unsigned*)at = weapon == nullptr ? 0u : (unsigned)((const char*)weapon - (const char*)SimBlockBase());
        break;
    }
    }
    return true;
}

const ReplayField* FindField(const char* name)
{
    for (unsigned i = 0; i < kReplayFieldCount; i++) {
        if (strcmp(kReplayFields[i].name, name) == 0)
            return &kReplayFields[i];
    }
    return nullptr;
}

bool InSet(const unsigned char* set, unsigned count, const ReplayField* f)
{
    for (unsigned i = 0; i < count; i++) {
        if (&kReplayFields[set[i]] == f)
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

const char* Enter(int id, const ReplayText& actual)
{
    int index = g_importIndex++;
    g_importsTotal++;
    g_outCount = 0;
    char key[96];
    sprintf(key, "import %d %s", index, kImportNames[id]);
    int next = g_cursor;
    while (next < g_callEnd && strncmp(g_lines[next], "  i ", 4) != 0 && strncmp(g_lines[next], "  o ", 4) != 0 && strncmp(g_lines[next], "  w ", 4) != 0)
        next++;
    if (next == g_callEnd || strncmp(g_lines[next], "  i ", 4) != 0) {
        Diff(key, "no such import call", actual.Data());
        return "";
    }
    g_cursor = next + 1;
    const char* recorded = g_lines[next] + 4;
    while (g_cursor < g_callEnd && strncmp(g_lines[g_cursor], "  o ", 4) == 0 && strncmp(g_lines[g_cursor], "  o params.", 11) != 0) {
        if (g_outCount < MAX_OUTS) {
            char* name = g_lines[g_cursor] + 4;
            char* space = strchr(name, ' ');
            if (space != nullptr) {
                *space = '\0';
                g_outNames[g_outCount] = name;
                g_outValues[g_outCount] = space + 1;
                g_outCount++;
            }
        }
        g_cursor++;
    }
    unsigned n = (unsigned)strlen(actual.Data());
    if (strncmp(recorded, actual.Data(), n) == 0 && (recorded[n] == '\0' || strncmp(recorded + n, " = ", 3) == 0))
        return recorded[n] == '\0' ? "" : recorded + n + 3;
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

void StartImport(ReplayText& t, int id)
{
    t.Clear();
    t.Add(kImportNames[id]);
}

void Space(ReplayText& t)
{
    t.AddChar(' ');
}

}

int __cdecl GameRand(RandStream stream)
{
    ReplayText t;
    StartImport(t, IMP_GAME_RAND);
    Space(t);
    AddStream(t, stream);
    const char* result = Enter(IMP_GAME_RAND, t);
    bx_rand(SimRandStream(stream));
    return atoi(result);
}

int __stdcall RandomInt(int range)
{
    ReplayText t;
    StartImport(t, IMP_RANDOM_INT);
    Space(t);
    t.AddInt(range);
    const char* result = Enter(IMP_RANDOM_INT, t);
    if (range >= 2) {
        unsigned int seed = *SeedWord();
        unsigned int q = seed / 127773;
        unsigned int correction = (q << 31) - seed / 127773;
        seed = seed * 16807 - correction;
        if ((int)seed <= 0)
            seed += 2147483647;
        *SeedWord() = seed;
    }
    return atoi(result);
}

void __stdcall FatalError(char* message)
{
    ReplayText t;
    StartImport(t, IMP_FATAL_ERROR);
    Space(t);
    t.AddString(message);
    Enter(IMP_FATAL_ERROR, t);
}

char* __stdcall BuildDataPath(char* buf, const char* dir, const char* name, const char* ext)
{
    ReplayText t;
    StartImport(t, IMP_BUILD_DATA_PATH);
    Space(t);
    t.AddString(dir);
    Space(t);
    t.AddString(name);
    Space(t);
    t.AddString(ext);
    const char* result = Enter(IMP_BUILD_DATA_PATH, t);
    ParseQuoted(result, buf, 256);
    return buf;
}

TdfFile::TdfFile()
{
    root = nullptr;
    current = nullptr;
    ReplayText t;
    StartImport(t, IMP_TDF_CONSTRUCT);
    Space(t);
    AddObject(t, OBJ_FILE, this);
    Enter(IMP_TDF_CONSTRUCT, t);
}

TdfFile::~TdfFile()
{
    ReplayText t;
    StartImport(t, IMP_TDF_DESTRUCT);
    Space(t);
    AddObject(t, OBJ_FILE, this);
    Enter(IMP_TDF_DESTRUCT, t);
}

int TdfFile::LoadFile(char* path)
{
    ReplayText t;
    StartImport(t, IMP_TDF_LOAD_FILE);
    Space(t);
    AddObject(t, OBJ_FILE, this);
    Space(t);
    t.AddString(path);
    int result = atoi(Enter(IMP_TDF_LOAD_FILE, t));
    const char* newRoot = OutValue(".root");
    const char* newCurrent = OutValue(".current");
    root = newRoot == nullptr ? nullptr : ParseRecord(newRoot);
    current = newCurrent == nullptr ? nullptr : ParseRecord(newCurrent);
    return result;
}

int TdfFile::SelectRecord(char* name)
{
    ReplayText t;
    StartImport(t, IMP_TDF_SELECT_RECORD);
    Space(t);
    AddObject(t, OBJ_FILE, this);
    Space(t);
    t.AddString(name);
    int result = atoi(Enter(IMP_TDF_SELECT_RECORD, t));
    const char* newCurrent = OutValue(".current");
    if (newCurrent != nullptr)
        current = ParseRecord(newCurrent);
    return result;
}

int TdfRecord::GetFieldInt(const char* name, int def)
{
    ReplayText t;
    StartImport(t, IMP_TDF_GET_INT);
    Space(t);
    AddObject(t, OBJ_RECORD, this);
    Space(t);
    t.AddString(name);
    Space(t);
    t.AddInt(def);
    return atoi(Enter(IMP_TDF_GET_INT, t));
}

double TdfRecord::GetFieldDouble(const char* name, double def)
{
    ReplayText t;
    StartImport(t, IMP_TDF_GET_DOUBLE);
    Space(t);
    AddObject(t, OBJ_RECORD, this);
    Space(t);
    t.AddString(name);
    Space(t);
    AddDoubleBits(t, def);
    return ParseF64(Enter(IMP_TDF_GET_DOUBLE, t));
}

int TdfRecord::GetFieldString(char* dst, char* key, size_t size, char* def)
{
    ReplayText t;
    StartImport(t, IMP_TDF_GET_STRING);
    Space(t);
    AddObject(t, OBJ_RECORD, this);
    Space(t);
    t.AddString(key);
    Space(t);
    t.AddInt((int)size);
    Space(t);
    t.AddString(def);
    int result = atoi(Enter(IMP_TDF_GET_STRING, t));
    const char* text = OutValue("dst");
    if (text != nullptr)
        ParseQuoted(text, dst, (unsigned)size);
    return result;
}

int CommandArgs::GetIntArg(int index, int def)
{
    ReplayText t;
    StartImport(t, IMP_GET_INT_ARG);
    Space(t);
    t.AddInt(index);
    Space(t);
    t.AddInt(def);
    return atoi(Enter(IMP_GET_INT_ARG, t));
}

namespace {

int __stdcall BankOpenAccount(HapiBank* self, char* name)
{
    ReplayText t;
    StartImport(t, IMP_BANK_OPEN_ACCOUNT);
    Space(t);
    AddObject(t, OBJ_BANK, self);
    Space(t);
    t.AddString(name);
    return atoi(Enter(IMP_BANK_OPEN_ACCOUNT, t));
}

int __stdcall BankSetInteger(HapiBank* self, const char* name, int value)
{
    ReplayText t;
    StartImport(t, IMP_BANK_SET_INTEGER);
    Space(t);
    AddObject(t, OBJ_BANK, self);
    Space(t);
    t.AddString(name);
    Space(t);
    t.AddInt(value);
    return atoi(Enter(IMP_BANK_SET_INTEGER, t));
}

int __stdcall BankGetInteger(HapiBank* self, char* name, int def)
{
    ReplayText t;
    StartImport(t, IMP_BANK_GET_INTEGER);
    Space(t);
    AddObject(t, OBJ_BANK, self);
    Space(t);
    t.AddString(name);
    Space(t);
    t.AddInt(def);
    return atoi(Enter(IMP_BANK_GET_INTEGER, t));
}

int __stdcall SpawnProjectile(WeaponDef* weapon, Vec3* pos, Vec3* vel, int flag)
{
    ReplayText t;
    StartImport(t, IMP_SPAWN_PROJECTILE);
    Space(t);
    AddWeapon(t, weapon);
    t.Add(" pos=");
    AddVec3(t, pos);
    t.Add(" vel=");
    AddVec3(t, vel);
    Space(t);
    t.AddInt(flag);
    return atoi(Enter(IMP_SPAWN_PROJECTILE, t));
}

WeaponDef* __stdcall FindWeaponByName(char* name)
{
    ReplayText t;
    StartImport(t, IMP_FIND_WEAPON);
    Space(t);
    t.AddString(name);
    const char* result = Enter(IMP_FIND_WEAPON, t);
    WeaponDef* weapon = ParseWeapon(result);
    const char* flags = OutValue(".flags");
    if (weapon != nullptr && flags != nullptr)
        weapon->flags.value = ParseHex(flags + 2, 8);
    return weapon;
}

void __fastcall LoadMeteorDefaultsSlot(MeteorParams* params)
{
    params->LoadMeteorDefaults();
}

WindMeteorsSeam g_windMeteors = {
    UpdateWind, UpdateMeteors, InitMeteors, EmptyShutdownPreCleanup, EnableMeteors, DisableMeteors,
    SetMeteorParams, LoadMeteorDefaultsSlot, SaveMeteors, LoadMeteors, CmdMeteor, UpdateWindOnStream,
};
WeaponsSeam g_weaponsSeam;
FilesSeam g_filesSeam;

struct Slot {
    const ReplaySlotSets* sets;
    MeteorParams params;
    CommandArgs args;
    char argText[0x14][128];
    int stream;
    bool hasBank;
};

void ParseParam(MeteorParams& p, const char* name, const char* value)
{
    if (strcmp(name, "params.name") == 0) {
        memset(p.name, 0, sizeof p.name);
        char text[64];
        ParseQuoted(value, text, sizeof text);
        strncpy(p.name, text, sizeof p.name);
    } else if (strcmp(name, "params.radius") == 0) {
        p.radius = atoi(value);
    } else {
        unsigned bits = ParseF32(value);
        float f;
        memcpy(&f, &bits, 4);
        if (strcmp(name, "params.density") == 0)
            p.density = f;
        else if (strcmp(name, "params.duration") == 0)
            p.duration = f;
        else
            p.interval = f;
    }
}

void ParseArgument(Slot& slot, const char* line, const char* file)
{
    char name[64];
    const char* space = strchr(line + 4, ' ');
    if (space == nullptr || (unsigned)(space - (line + 4)) >= sizeof name)
        Fatal(file, g_callIndex, "bad argument line", line);
    memcpy(name, line + 4, (unsigned)(space - (line + 4)));
    name[space - (line + 4)] = '\0';
    const char* value = space + 1;
    if (strncmp(name, "params.", 7) == 0) {
        ParseParam(slot.params, name, value);
    } else if (strcmp(name, "args.count") == 0) {
        slot.args.count = atoi(value);
    } else if (strncmp(name, "args[", 5) == 0) {
        int i = atoi(name + 5);
        if (i < 0 || i >= 0x14)
            Fatal(file, g_callIndex, "bad argument index", line);
        ParseQuoted(value, slot.argText[i], sizeof slot.argText[i]);
        slot.args.args[i] = slot.argText[i];
    } else if (strcmp(name, "stream") == 0) {
        slot.stream = strcmp(value, "load") == 0 ? RAND_STREAM_LOAD : strcmp(value, "main") == 0 ? RAND_STREAM_MAIN : atoi(value);
    } else if (strcmp(name, "bank") == 0) {
        slot.hasBank = true;
    } else {
        Fatal(file, g_callIndex, "unknown argument", line);
    }
}

void RunSlot(Slot& slot, const char* file)
{
    const char* name = slot.sets->name;
    WindMeteorsSeam* s = g_seams.wind_meteors;
    if (strcmp(name, "UpdateWind") == 0)
        s->UpdateWind();
    else if (strcmp(name, "UpdateMeteors") == 0)
        s->UpdateMeteors();
    else if (strcmp(name, "InitMeteors") == 0)
        s->InitMeteors();
    else if (strcmp(name, "EmptyShutdownPreCleanup") == 0)
        s->EmptyShutdownPreCleanup();
    else if (strcmp(name, "EnableMeteors") == 0)
        s->EnableMeteors();
    else if (strcmp(name, "DisableMeteors") == 0)
        s->DisableMeteors();
    else if (strcmp(name, "SetMeteorParams") == 0)
        s->SetMeteorParams(&slot.params);
    else if (strcmp(name, "LoadMeteorDefaults") == 0)
        s->LoadMeteorDefaults(&slot.params);
    else if (strcmp(name, "SaveMeteors") == 0)
        s->SaveMeteors(&g_bank);
    else if (strcmp(name, "LoadMeteors") == 0)
        s->LoadMeteors(&g_bank);
    else if (strcmp(name, "CmdMeteor") == 0)
        s->CmdMeteor(&slot.args);
    else if (strcmp(name, "UpdateWindOnStream") == 0)
        s->UpdateWindOnStream((RandStream)slot.stream);
    else
        Fatal(file, g_callIndex, "unknown slot", name);
}

void ResetBlock()
{
    if (ReplayWasmActive())
        ReplayWasmReload();
    else
        SimBlockReset();
    g_simGlobals = (SimGlobals*)SimAlloc(sizeof(SimGlobals));
    g_game = (Game*)SimAlloc(sizeof(Game));
    g_seams.wind_meteors = &g_windMeteors;
    g_seams.weapons = &g_weaponsSeam;
    g_seams.files = &g_filesSeam;
    g_weaponsSeam.SpawnProjectile = SpawnProjectile;
    g_weaponsSeam.FindWeaponByName = FindWeaponByName;
    g_filesSeam.BankOpenAccount = BankOpenAccount;
    g_filesSeam.BankSetInteger = BankSetInteger;
    g_filesSeam.BankGetInteger = BankGetInteger;
    if (ReplayWasmActive())
        ReplayWasmUseSeam();
    strcpy(g_extTdf, "tdf");
    g_recordedCount = 0;
}

const ReplaySlotSets* FindSlot(const char* name)
{
    for (unsigned i = 0; i < kReplaySlotCount; i++) {
        if (strcmp(kReplaySlots[i].name, name) == 0)
            return &kReplaySlots[i];
    }
    return nullptr;
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
    return true;
}

struct Outcome {
    int calls;
    int imports;
    bool diverged;
    int call;
    char key[96];
    ReplayText log;
};

void CompareWrites(const char* file, const ReplaySlotSets* sets)
{
    ReplayText value;
    for (unsigned i = 0; i < sets->writeCount; i++) {
        const ReplayField& f = kReplayFields[sets->writes[i]];
        FormatField(f, value);
        HashText(value.Data());
        const char* recorded = nullptr;
        for (int k = g_callFirst + 1; k < g_callEnd; k++) {
            const char* line = g_lines[k];
            if (strncmp(line, "  w ", 4) == 0 && strncmp(line + 4, f.name, strlen(f.name)) == 0 && line[4 + strlen(f.name)] == ' ')
                recorded = line + 5 + strlen(f.name);
        }
        if (recorded != nullptr)
            SetRecorded(f.name, recorded);
        Recorded* known = FindRecorded(f.name);
        if (known == nullptr)
            Diff(f.name, "no value", value.Data());
        else if (strcmp(known->value, value.Data()) != 0)
            Diff(f.name, known->value, value.Data());
    }
    for (int k = g_callFirst + 1; k < g_callEnd; k++) {
        const char* line = g_lines[k];
        if (strncmp(line, "  w ", 4) != 0)
            continue;
        const char* space = strchr(line + 4, ' ');
        char name[64];
        if (space == nullptr || (unsigned)(space - (line + 4)) >= sizeof name)
            Fatal(file, g_callIndex, "bad write line", line);
        memcpy(name, line + 4, (unsigned)(space - (line + 4)));
        name[space - (line + 4)] = '\0';
        const ReplayField* f = FindField(name);
        if (f == nullptr || !InSet(sets->writes, sets->writeCount, f))
            Fatal(file, g_callIndex, "field is not in the write set of the slot", line);
    }
}

void CompareClosing(const ReplaySlotSets* sets, const Slot& slot)
{
    if (strcmp(sets->name, "LoadMeteorDefaults") != 0)
        return;
    ReplayText t;
    char name[0x21];
    memcpy(name, slot.params.name, 0x20);
    name[0x20] = '\0';
    struct Item {
        const char* name;
        ReplayText text;
    } items[5];
    items[0].name = "params.name";
    items[0].text.AddString(name);
    items[1].name = "params.radius";
    items[1].text.AddInt(slot.params.radius);
    items[2].name = "params.density";
    AddFloatBits(items[2].text, slot.params.density);
    items[3].name = "params.duration";
    AddFloatBits(items[3].text, slot.params.duration);
    items[4].name = "params.interval";
    AddFloatBits(items[4].text, slot.params.interval);
    for (int i = 0; i < 5; i++) {
        const char* recorded = "no such line";
        for (int k = g_callFirst + 1; k < g_callEnd; k++) {
            const char* line = g_lines[k];
            size_t n = strlen(items[i].name);
            if (strncmp(line, "  o ", 4) == 0 && strncmp(line + 4, items[i].name, n) == 0 && line[4 + n] == ' ')
                recorded = line + 5 + n;
        }
        HashText(items[i].text.Data());
        if (strcmp(recorded, items[i].text.Data()) != 0)
            Diff(items[i].name, recorded, items[i].text.Data());
    }
}

void ReplayCall(const char* file, Outcome& out)
{
    char* header = g_lines[g_callFirst];
    char* tickText = header + 5;
    char* slotText = strchr(tickText, ' ');
    if (slotText == nullptr)
        Fatal(file, g_callIndex, "bad call line", header);
    *slotText++ = '\0';
    const ReplaySlotSets* sets = FindSlot(slotText);
    if (sets == nullptr)
        Fatal(file, g_callIndex, "unknown slot", slotText);
    g_diffs = 0;
    g_firstKey[0] = '\0';
    g_detail.Clear();
    g_ordinals.Reset();
    g_importIndex = 0;
    g_outCount = 0;
    if (strcmp(tickText, "-") != 0)
        g_game->ticks = atoi(tickText);

    static Slot slot;
    memset(&slot, 0, sizeof slot);
    slot.sets = sets;
    slot.stream = RAND_STREAM_MAIN;
    int first = g_callFirst + 1;
    for (int k = first; k < g_callEnd; k++) {
        char* line = g_lines[k];
        if (strncmp(line, "  r ", 4) == 0) {
            char* space = strchr(line + 4, ' ');
            if (space == nullptr)
                Fatal(file, g_callIndex, "bad read line", line);
            *space = '\0';
            const ReplayField* f = FindField(line + 4);
            if (f == nullptr || !InSet(sets->reads, sets->readCount, f))
                Fatal(file, g_callIndex, "field is not in the read set of the slot", line + 4);
            if (!RestoreField(*f, space + 1))
                Fatal(file, g_callIndex, "a value that cannot be restored", space + 1);
            SetRecorded(f->name, space + 1);
            *space = ' ';
        } else if (strncmp(line, "  a ", 4) == 0) {
            ParseArgument(slot, line, file);
        }
    }
    g_cursor = first;
    g_hash = 2166136261u;
    RunSlot(slot, file);
    for (int k = g_cursor; k < g_callEnd; k++) {
        if (strncmp(g_lines[k], "  i ", 4) == 0) {
            char key[96];
            const char* nameEnd = strchr(g_lines[k] + 4, ' ');
            int nameLength = nameEnd == nullptr ? (int)strlen(g_lines[k] + 4) : (int)(nameEnd - (g_lines[k] + 4));
            sprintf(key, "import %d %.*s", g_importIndex, nameLength, g_lines[k] + 4);
            Diff(key, g_lines[k] + 4, "no call (missing)");
            break;
        }
    }
    CompareWrites(file, sets);
    CompareClosing(sets, slot);

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
}

enum Mutation { MUT_NONE, MUT_ARGUMENT, MUT_EXTRA, MUT_MISSING };

char g_mutated[TEXT];
int g_mutationCall;
char g_mutationKey[96];

bool Mutate(int mutation)
{
    int call = -1;
    int imports = 0;
    for (int k = 2; k < g_lineCount; k++) {
        if (strncmp(g_lines[k], "call ", 5) == 0) {
            call++;
            imports = 0;
        } else if (strncmp(g_lines[k], "  i ", 4) == 0) {
            if (mutation == MUT_ARGUMENT && strncmp(g_lines[k], "  i weapons.SpawnProjectile weapondef#", 38) == 0) {
                sprintf(g_mutated, "%s", g_lines[k]);
                char* digit = strstr(g_mutated, "weapondef#") + 10;
                *digit = *digit == '9' ? '8' : '9';
                g_lines[k] = g_mutated;
                g_mutationCall = call;
                sprintf(g_mutationKey, "import %d weapons.SpawnProjectile", imports);
                return true;
            }
            if (mutation == MUT_MISSING && strncmp(g_lines[k], "  i weapons.SpawnProjectile", 27) == 0) {
                for (int j = k; j + 1 < g_lineCount; j++)
                    g_lines[j] = g_lines[j + 1];
                g_lineCount--;
                g_mutationCall = call;
                sprintf(g_mutationKey, "import %d weapons.SpawnProjectile", imports);
                return true;
            }
            if (mutation == MUT_EXTRA && strncmp(g_lines[k], "  i weapons.SpawnProjectile", 27) == 0) {
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

void ReplayFile(const char* file, Outcome& out, int mutation = MUT_NONE)
{
    if (!LoadFile(file)) {
        printf("FAIL: cannot read %s\n", file);
        exit(2);
    }
    if (g_lineCount < 2 || strcmp(g_lines[0], "bt-replay 1") != 0 || strncmp(g_lines[1], "subsystem wind_meteors ", 22) != 0)
        Fatal(file, 0, "not a bt-replay 1 recording of wind_meteors", g_lines[0]);
    if (mutation != MUT_NONE && !Mutate(mutation))
        Fatal(file, 0, "nothing to mutate", "no SpawnProjectile import");
    ResetBlock();
    out.calls = 0;
    out.imports = 0;
    out.diverged = false;
    out.call = -1;
    out.key[0] = '\0';
    out.log.Clear();
    int at = 2;
    g_callIndex = 0;
    while (at < g_lineCount && !out.diverged) {
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

}

int SelfTest(const char* file)
{
    static const char* const names[] = { "", "an import with other arguments", "an extra import", "a missing import" };
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

int main(int argc, char** argv)
{
    int expectCall = -1;
    const char* expectField = nullptr;
    int arg = 1;
    if (arg + 1 < argc && strcmp(argv[arg], "--wasm") == 0) {
        ReplayWasmSelect(argv[arg + 1]);
        printf("replaying against the WASM module %s\n", argv[arg + 1]);
        arg += 2;
    }
    if (arg + 1 < argc && strcmp(argv[arg], "--self-test") == 0) {
        SelfTest(argv[arg + 1]);
        arg = argc;
    } else if (arg + 2 < argc && strcmp(argv[arg], "--expect") == 0) {
        expectCall = atoi(argv[arg + 1]);
        expectField = argv[arg + 2];
        arg += 3;
    } else if (arg >= argc) {
        printf("usage: replay_native_test [--wasm MODULE.wasm] [--self-test RECORDING | --expect CALL FIELD] RECORDING...\n");
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
