#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <vector>

#include "../src/harness/replay_schema.h"
#include "../src/files/files_bank.h"
#include "../src/util/commands.h"
#include "../interfaces/services.h"
#include "../interfaces/seams.h"
#include "../src/game/game_state.h"
#include "../src/game/console_commands.h"
#include "../src/units/unit_targets.h"
#include "../src/harness/replay_services.h"
#include "../src/harness/replay_text.h"
#include "../src/harness/replay_values.h"
#include "../src/harness/replay_objects.h"
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

void __stdcall UpdateResourceSharing(Player* player);
float __stdcall GetEnergyUse(UnitDef* def);
void __stdcall UpdatePlayerEconomy(Player* player);
void __stdcall UpdateMetalExtraction(Unit* unit);

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

namespace {

enum { MAX_LINES = 1 << 21, MAX_OUTS = 16384, MAX_RECORDS = 64, TEXT = 512 };
enum HandleKind { OBJ_FILE, OBJ_RECORD, OBJ_BANK };

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

ReplayKnown g_known;
ReplayObjectTable g_objects;
ReplayReach g_reach;

void* AllocObject(unsigned size)
{
    return SimAlloc(size);
}

char* g_text;
char* g_lines[MAX_LINES];
int g_lineCount;
char g_subsystem[48];
const char* g_file = "";

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
const char* g_closing[3];
unsigned char g_rootKinds[REPLAY_MAX_ROOTS];
void* g_rootObjects[REPLAY_MAX_ROOTS];
unsigned g_rootCount;
const ReplaySlotSets* g_sets;

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

unsigned char* BlockBase(const ReplayField& f)
{
    return f.base == RB_GAME ? (unsigned char*)g_game : (unsigned char*)g_simGlobals;
}

void FormatField(const ReplayField& f, ReplayText& out)
{
    out.Clear();
    if (f.base == RB_RNG) {
        out.AddHex32(f.offset == 2 ? *SeedWord() : *SimRandStream((int)f.offset));
        return;
    }
    ReplayFormatObjectField(ReplayFieldView(f), BlockBase(f), g_objects, false, out);
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
    return ReplayRestoreObjectField(ReplayFieldView(f), BlockBase(f), value, g_objects, nullptr);
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
    for (int i = 0; i < 3; i++) {
        if (g_closing[i] != nullptr && strncmp(line + 4, g_closing[i], strlen(g_closing[i])) == 0)
            return true;
    }
    return false;
}

const char* Enter(const ReplayText& actual)
{
    int index = g_importIndex++;
    g_importsTotal++;
    g_outCount = 0;
    char key[96];
    const char* space = strchr(actual.Data(), ' ');
    int nameLength = space == nullptr ? (int)strlen(actual.Data()) : (int)(space - actual.Data());
    sprintf(key, "import %d %.*s", index, nameLength, actual.Data());
    int next = g_cursor;
    while (next < g_callEnd && strncmp(g_lines[next], "  i ", 4) != 0 && strncmp(g_lines[next], "  o ", 4) != 0 && strncmp(g_lines[next], "  w ", 4) != 0)
        next++;
    if (next == g_callEnd || strncmp(g_lines[next], "  i ", 4) != 0) {
        Diff(key, "no such import call", actual.Data());
        return "";
    }
    g_cursor = next + 1;
    const char* recorded = g_lines[next] + 4;
    while (g_cursor < g_callEnd && strncmp(g_lines[g_cursor], "  o ", 4) == 0 && !ClosingLine(g_lines[g_cursor])) {
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

void ApplyReach(void* object, unsigned kind, const char* suffix)
{
    const char* value = OutValue(suffix);
    const ReplayObjectField* f = ReplayFindObjectField(kind, suffix + 1);
    if (object != nullptr && value != nullptr && f != nullptr)
        ReplayRestoreObjectField(*f, object, value, g_objects, nullptr);
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
void ParseScalar(const char* v, Vec3& out) { sscanf(v, "(%d,%d,%d)", &out.x, &out.y, &out.z); }

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
        g_rootKinds[g_rootCount] = (unsigned char)kind;
        g_rootObjects[g_rootCount++] = p;
    }
    return p;
}

void ParseStruct(const char* name, unsigned kind, void* object)
{
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[i];
        if (f.object != kind)
            continue;
        char key[96];
        sprintf(key, "%s.%s", name, f.name);
        const char* value = ArgLine(key);
        if (value != nullptr)
            ReplayRestoreObjectField(f, object, value, g_objects, nullptr);
    }
}

void* ParseStructOpt(const char* name, unsigned kind, void* storage)
{
    const char* value = ArgLine(name);
    if (value != nullptr && strcmp(value, "null") == 0)
        return nullptr;
    ParseStruct(name, kind, storage);
    return storage;
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
        const char* recorded = OutLine(key);
        if (recorded == nullptr)
            recorded = "no such line";
        if (strcmp(recorded, value.Data()) != 0)
            Diff(key, recorded, value.Data());
    }
    value.Free();
}

void CompareObjectResult(unsigned kind, const void* object)
{
    ReplayText value;
    ReplayAddObjectName(value, g_objects, kind, object, false);
    HashText(value.Data());
    const char* recorded = OutLine("result");
    if (recorded == nullptr)
        recorded = "no such line";
    if (strcmp(recorded, value.Data()) != 0)
        Diff("result", recorded, value.Data());
    value.Free();
}

template <class T> void CompareResult(T result)
{
    ReplayText value;
    ReplayValue(value, result);
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

void SetClosing(const char* a, const char* b = nullptr, const char* c = nullptr)
{
    g_closing[0] = a;
    g_closing[1] = b;
    g_closing[2] = c;
}

}

#include "replay_services.h"

namespace {

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

void __fastcall LoadMeteorDefaultsSlot(MeteorParams* params)
{
    params->LoadMeteorDefaults();
}

unsigned char __fastcall ChooseWeaponSlot(Unit* unit)
{
    return unit->ChooseWeapon();
}

void __stdcall SetWeaponTargetUnitSlot(Unit* unit, Unit* target, int weapon)
{
    SetWeaponTargetUnit((char*)unit, (char*)target, weapon);
}

void __stdcall SetWeaponTargetPosSlot(Unit* unit, Vec3* pos, int weapon)
{
    SetWeaponTargetPos((char*)unit, &pos->x, weapon);
}

void __stdcall ResetSlot(UnitResources* self, unsigned char playerIndex)
{
    self->Reset(playerIndex);
}

void __stdcall SaveUnitAccountsSlot(UnitResources* self, UnitInfo* info, HapiBank* file)
{
    self->SaveUnitAccounts(info, file);
}

void __stdcall LoadUnitAccountsSlot(UnitResources* self, UnitInfo* info, HapiBank* file)
{
    self->LoadUnitAccounts(info, file);
}

int __stdcall RequestEnergySlot(UnitResources* self, UnitResources* r, float amount)
{
    return self->RequestEnergy(r, amount);
}

int __stdcall RequestEnergyAndMetalSlot(UnitResources* self, float energy, float metal)
{
    return self->RequestEnergyAndMetal(energy, metal);
}

int __stdcall SpendEnergySlot(UnitResources* self, float amount)
{
    return self->SpendEnergy(amount);
}

int __stdcall SpendMetalSlot(UnitResources* self, float amount)
{
    return self->SpendMetal(amount);
}

int __stdcall SpendEnergyAndMetalSlot(UnitResources* self, float energy, float metal)
{
    return self->SpendEnergyAndMetal(energy, metal);
}

#include "replay_slots.h"

void ResetBlock()
{
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
};

bool RestoreObjectLine(const char* name, const char* value, const char* file, const ReplaySlotSets* sets, const char* line)
{
    unsigned kind = 0;
    unsigned ordinal = 0;
    const char* fieldName = nullptr;
    if (!ReplayParseKey(name, &kind, &ordinal, &fieldName))
        return false;
    char object[48];
    const char* dot = strrchr(name, '.');
    memcpy(object, name, (unsigned)(dot - name));
    object[dot - name] = '\0';
    void* p = ReplayResolveObject(object, nullptr, g_objects, AllocObject);
    const ReplayObjectField* f = ReplayFindObjectField(kind, fieldName);
    if (p == nullptr || f == nullptr)
        Fatal(file, g_callIndex, "an object line that names no object or field", line);
    if (sets != nullptr && !InObjectSet(sets->objReads, sets->objReadCount, f) && !InObjectSet(sets->objWrites, sets->objWriteCount, f))
        Fatal(file, g_callIndex, "field is not in the object sets of the slot", name);
    if (f->kind == RK_ARRAY) {
        if (!ReplayApplyArray(*f, p, name, value, g_known, true, AllocObject))
            Fatal(file, g_callIndex, "an array line that cannot be applied", value);
        return true;
    }
    if (!ReplayRestoreObjectField(*f, p, value, g_objects, AllocObject))
        Fatal(file, g_callIndex, "a value that cannot be restored", value);
    g_known.Set(name, value);
    return true;
}

void CompareObjects(const ReplaySlotSets* sets)
{
    ReplayWalk(g_reach, g_objects, false, *sets, g_rootKinds, g_rootObjects, g_rootCount);
    ReplayText key;
    ReplayText value;
    for (unsigned at = 0; at < g_reach.count; at++) {
        for (unsigned i = 0; i < sets->objWriteCount; i++) {
            const ReplayObjectField& f = kReplayObjectFields[sets->objWrites[i]];
            if (f.object != g_reach.kind[at] || f.kind == RK_EMBED || f.kind == RK_UNITID || !ReplayInReach(g_reach, at, sets->objWrites[i]))
                continue;
            ReplayObjectFieldKey(key, g_objects, false, g_reach.kind[at], g_reach.object[at], f);
            ReplayFormatObjectField(f, g_reach.object[at], g_objects, false, value);
            HashText(value.Data());
            const char* known = g_known.Get(key.Data());
            if (known == nullptr)
                Diff(key.Data(), "no value", value.Data());
            else if (strcmp(known, value.Data()) != 0)
                Diff(key.Data(), known, value.Data());
        }
    }
    key.Free();
    value.Free();
}

void CompareWrites(const char* file, const ReplaySlotSets* sets)
{
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
            unsigned kind = 0;
            unsigned ordinal = 0;
            const char* fieldName = nullptr;
            const ReplayObjectField* f = ReplayParseKey(name, &kind, &ordinal, &fieldName) ? ReplayFindObjectField(kind, fieldName) : nullptr;
            if (f == nullptr || !InObjectSet(sets->objWrites, sets->objWriteCount, f))
                Fatal(file, g_callIndex, "field is not in the write set of the slot", line);
            array = f->kind == RK_ARRAY;
            arrayField = f;
        } else {
            const ReplayField* f = FindField(name);
            if (f == nullptr || !InSet(sets->writes, sets->writeCount, f))
                Fatal(file, g_callIndex, "field is not in the write set of the slot", line);
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
        FormatField(f, value);
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
    g_diffs = 0;
    g_firstKey[0] = '\0';
    g_detail.Clear();
    g_ordinals.Reset();
    g_importIndex = 0;
    g_outCount = 0;
    g_rootCount = 0;
    if (strcmp(tickText, "-") != 0)
        g_game->ticks = atoi(tickText);

    int first = g_callFirst + 1;
    for (int k = first; k < g_callEnd; k++) {
        char* line = g_lines[k];
        if (strncmp(line, "  r ", 4) != 0)
            continue;
        char* space = strchr(line + 4, ' ');
        if (space == nullptr)
            Fatal(file, g_callIndex, "bad read line", line);
        *space = '\0';
        if (strchr(line + 4, '#') != nullptr) {
            RestoreObjectLine(line + 4, space + 1, file, sets, line);
        } else {
            const ReplayField* f = FindField(line + 4);
            if (f == nullptr || !InSet(sets->reads, sets->readCount, f))
                Fatal(file, g_callIndex, "field is not in the read set of the slot", line + 4);
            if (!RestoreField(*f, space + 1))
                Fatal(file, g_callIndex, "a value that cannot be restored", space + 1);
        }
        *space = ' ';
    }
    g_cursor = first;
    g_hash = 2166136261u;
    g_sets = sets;
    if (!RunSlot(g_subsystem, slotText))
        Fatal(file, g_callIndex, "a slot with no runner", slotText);
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
    RunAfter(g_subsystem, slotText);

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
    if (tail[0] != '\0' || !known->complete)
        Fatal(file, 0, "the annotations of this subsystem are not complete, so its recordings cannot be replayed", g_subsystem);
    char want[32];
    sprintf(want, "%u.%u", known->major, known->minor);
    if (strcmp(version, want) != 0)
        Fatal(file, 0, "the interface version differs from the schema's", version);
    if (ReplayWasmActive() && strcmp(ReplayWasmSubsystem(), g_subsystem) != 0)
        Fatal(file, 0, "the WASM module is of another subsystem", ReplayWasmSubsystem());
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
    unsigned char kind = ROK_PLAYER;
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
    UnitDef* def = (UnitDef*)SimAlloc(sizeof(UnitDef));
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
    UnitResources* econ = (UnitResources*)SimAlloc(sizeof(UnitResources));
    player->econ = econ;
    econ->energyMake = 7.0f;
    econ->player = player;
    player->totalEnergyProduced = 123.5;
    player->allied[3] = 1;
    player->type = 2;
    a->resourceSlot.energyMake = 2.5f;
    a->resourceSlot.player = player;
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
    Check(strstr(text.Data(), "  r Player#3.econ UnitResources#1\n") != nullptr, "a separate account is named by its order");
    Check(strstr(text.Data(), "  r UnitResources#1.energyMake f32:0x40e00000\n") != nullptr, "the fields of a separate account are recorded");
    Check(strstr(text.Data(), "  r Unit#2.resourceSlot.energyMake f32:0x40200000\n") != nullptr, "the account inside a unit is named by the unit and the member");
    Check(strstr(text.Data(), "  r Unit#2.resourceSlot.player Player#3\n") != nullptr, "the account inside a unit leads back to its player");
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
    Check(ra->resourceSlot.energyMake == 2.5f && ra->resourceSlot.player.get() == rebuilt, "the account inside a unit is restored in the unit");
    Check(rebuilt->econ.get() != nullptr && rebuilt->econ.get()->energyMake == 7.0f, "a separate account is rebuilt");
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
    void Record(const unsigned char* kinds, void* const* roots, unsigned count, const ReplaySlotSets& sets)
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
    strcpy(copy, text);
    int lines = 0;
    for (char* p = copy; *p != '\0'; lines++) {
        char* end = strchr(p, '\n');
        *end = '\0';
        char* space = strchr(p + 4, ' ');
        *space = '\0';
        Check(RestoreObjectLine(p + 4, space + 1, "self-test", &sets, p), "a recorded line restores");
        p = end + 1;
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

    unsigned char kinds[2] = { ROK_PROJECTILE, ROK_MOVEMENTCLASS };
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
    Check(g_game->projectileCount == 1024, "the pool has the size the schema says");
    Check(rebuilt != nullptr && rebuilt->speed == 77 && rebuilt->spawnTick == 900 && rebuilt->owner == 4, "its fields are restored");
    Check(rebuilt != nullptr && rebuilt->attacker.get() == &g_game->units.get()[2], "its attacker is a slot of the unit pool");
    Check(rebuiltClass == &MovementEntries()[5] && rebuiltClass->maxSlope == 9, "an entry of the sim table is restored in place");

    Recorder again;
    unsigned char againKinds[2] = { ROK_PROJECTILE, ROK_MOVEMENTCLASS };
    void* againRoots[2] = { rebuilt, rebuiltClass };
    again.Record(againKinds, againRoots, 2, sets);
    Check(strcmp(text, again.text.Data()) == 0, "the rebuilt tables record the same text");

    memcpy(g_rootKinds, againKinds, 2);
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
    MovementClass* movement = &MovementEntries()[2];
    movement->width = 4;
    movement->height = 32;
    unsigned* cells = (unsigned*)SimAlloc(8 * 4);
    for (int i = 0; i < 8; i++)
        cells[i] = 0x01020304u * (unsigned)(i + 1);
    movement->cells = cells;
    unsigned char kinds[2] = { ROK_PLAYER, ROK_MOVEMENTCLASS };
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
    Check(strstr(first, "  r MovementClass#2.cells [8] 0:04030201") != nullptr, "the elements are written as the bytes of memory");

    explored[5] = 0xaa;
    explored[6] = 0xbb;
    explored[1000] = 0xcc;
    cells[3] = 0xdeadbeefu;
    recorder.Record(kinds, roots, 2, sets);
    calls.AddText(recorder.text);
    const char* second = recorder.text.Data();
    Check(strstr(second, "  r Player#3.explored 5:aabb 1000:cc\n") != nullptr, "a later call writes the changed runs only");
    Check(strstr(second, "  r MovementClass#2.cells 3:efbeadde\n") != nullptr, "a changed element of a wider array is one run");
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
    MovementClass* rebuiltClass = (MovementClass*)ReplayResolveObject("MovementClass#2", nullptr, g_objects, AllocObject);
    Check(rebuilt != nullptr && rebuilt->exploredSize == 40 && rebuilt->explored.get() != nullptr && rebuilt->explored.get()[39] == 9 && rebuilt->explored.get()[40 - 1] == 9, "the last whole write wins");
    Check(rebuiltClass != nullptr && rebuiltClass->cells.get() != nullptr && rebuiltClass->cells.get()[3] == 0xdeadbeefu && rebuiltClass->cells.get()[2] == 0x01020304u * 3, "the delta is applied to the array the replayer kept");

    unsigned char kind = ROK_PLAYER;
    void* root = rebuilt;
    Recorder replayed;
    replayed.Record(&kind, &root, 1, sets);
    Check(strstr(replayed.text.Data(), "  r Player#3.explored [40] 0:09090909") != nullptr, "the replayed array records as it was");

    g_rootKinds[0] = ROK_PLAYER;
    g_rootObjects[0] = rebuilt;
    g_rootKinds[1] = ROK_MOVEMENTCLASS;
    g_rootObjects[1] = rebuiltClass;
    g_rootCount = 2;
    BeginCompare();
    CompareObjects(&sets);
    Check(g_diffs == 0, "the arrays compare equal after the deltas");
    rebuiltClass->cells.get()[6] ^= 0x100u;
    BeginCompare();
    CompareObjects(&sets);
    printf("a changed element: %s\n%s", g_firstKey, g_detail.Data());
    Check(g_diffs == 1 && strcmp(g_firstKey, "MovementClass#2.cells") == 0, "a changed element is reported at its array");
    Check(strstr(g_detail.Data(), "byte 25 of 32") != nullptr, "and at its byte");
    rebuiltClass->cells.get()[6] ^= 0x100u;

    printf("long deltas\n");
    ResetBlock();
    Player* wide = &g_game->players[4];
    wide->exploredSize = 1500;
    unsigned char* field = (unsigned char*)SimAlloc(1500);
    for (int i = 0; i < 1500; i++)
        field[i] = (unsigned char)i;
    wide->explored = field;
    unsigned char wideKind = ROK_PLAYER;
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
}

int SelfTest(const char* file)
{
    static const char* const names[] = { "", "an import with other arguments", "an extra import", "a missing import" };
    ObjectSelfTest();
    TableSelfTest();
    ArraySelfTest();
    FormSelfTest();
    if (file == nullptr)
        return 0;
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
    if (arg < argc && strcmp(argv[arg], "--self-test") == 0) {
        SelfTest(arg + 1 < argc ? argv[arg + 1] : nullptr);
        arg = argc;
    } else if (arg + 2 < argc && strcmp(argv[arg], "--expect") == 0) {
        expectCall = atoi(argv[arg + 1]);
        expectField = argv[arg + 2];
        arg += 3;
    } else if (arg >= argc) {
        printf("usage: replay_native_test [--wasm MODULE.wasm] [--self-test [RECORDING] | --expect CALL FIELD RECORDING...] RECORDING...\n");
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
