#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <new>
#include "../state/game_rand.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../weapons/weapon_def.h"
#include "../util/vec3.h"
#include "../files/files_bank.h"
#include "../../interfaces/services.h"
#include "../../interfaces/seams.h"
#include "../state/sim_globals.h"
#include "../state/block.h"
#include "../game/los_helpers.h"
#include "../map/meteors.h"
#include "replay_text.h"
#include "replay.h"
#include "replay_schema.h"

int g_replayCalls;

static_assert(sizeof(WeaponDef) == 0x115, "WeaponDef is 0x115 bytes");

namespace {

enum Slot {
    SLOT_UPDATE_WIND,
    SLOT_UPDATE_METEORS,
    SLOT_INIT_METEORS,
    SLOT_EMPTY_SHUTDOWN,
    SLOT_ENABLE_METEORS,
    SLOT_DISABLE_METEORS,
    SLOT_SET_METEOR_PARAMS,
    SLOT_LOAD_METEOR_DEFAULTS,
    SLOT_SAVE_METEORS,
    SLOT_LOAD_METEORS,
    SLOT_CMD_METEOR,
    SLOT_UPDATE_WIND_ON_STREAM,
    SLOT_COUNT
};

enum ObjectKind { OBJ_FILE, OBJ_RECORD, OBJ_BANK };

const char* const kImportNames[RI_COUNT] = {
    "services.GameRand", "services.RandomInt", "services.FatalError", "services.BuildDataPath",
    "services.TdfFile_Construct", "services.TdfFile_Destruct", "services.TdfFile_LoadFile",
    "services.TdfFile_SelectRecord", "services.TdfRecord_GetFieldInt", "services.TdfRecord_GetFieldDouble",
    "services.TdfRecord_GetFieldString", "services.CommandArgs_GetIntArg", "services.HapiBank_OpenAccount",
    "services.HapiBank_SetIntegerItem", "services.HapiBank_GetIntegerItem", "weapons.SpawnProjectile",
    "weapons.FindWeaponByName",
};

bool s_on;
bool s_windowed;
unsigned s_first;
unsigned s_last;
int s_depth;
bool s_recording;
int s_slot;
int s_tick;
ReplayText s_out;
ReplayText s_call;
ReplayText s_tail;
ReplayText s_field;
ReplayKnown s_known;
ReplayOrdinals s_ordinals;

const WindMeteorsSeam* s_innerWind;
const WeaponsSeam* s_innerWeapons;
const FilesSeam* s_innerFiles;
WindMeteorsSeam s_wind;
WeaponsSeam s_weapons;
FilesSeam s_files;

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
    if (p == 0) {
        t.Add("null");
        return;
    }
    const unsigned char* base = g_game == 0 ? 0 : (const unsigned char*)g_game->weapons;
    const unsigned char* q = (const unsigned char*)p;
    unsigned span = sizeof(WeaponDef);
    if (base != 0 && q >= base && q < base + 256 * span && (unsigned)(q - base) % span == 0) {
        t.Add("weapondef#");
        t.AddUnsigned((unsigned)(q - base) / span);
    } else {
        t.Add("weapondef#?");
    }
}

void AddObject(ReplayText& t, unsigned kind, const void* p)
{
    static const char* const names[] = { "file#", "rec#", "bank#" };
    unsigned ordinal = s_ordinals.Of(kind, p);
    if (ordinal == 0) {
        t.Add("null");
        return;
    }
    t.Add(names[kind]);
    t.AddUnsigned(ordinal);
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

#pragma push_macro("g_randomSeed")
#undef g_randomSeed

bool FormatField(const ReplayField& f, ReplayText& out)
{
    out.Clear();
    const unsigned char* at = 0;
    if (f.base == RB_GAME) {
        if (g_game == 0)
            return false;
        at = (const unsigned char*)g_game + f.offset;
    } else if (f.base == RB_SIM) {
        if (g_simGlobals == 0)
            return false;
        at = (const unsigned char*)g_simGlobals + f.offset;
    } else {
        if (SimBlockBase() == 0)
            return false;
        if (f.offset == 2) {
            if (g_simGlobals == 0)
                return false;
            out.AddHex32(g_simGlobals->g_randomSeed);
        } else {
            out.AddHex32(GameHoldrand((RandStream)f.offset));
        }
        return true;
    }
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
        AddWeapon(out, stored == 0 ? 0 : (const unsigned char*)SimBlockBase() + stored);
        break;
    }
    }
    return true;
}

#pragma pop_macro("g_randomSeed")

void CaptureSet(const unsigned char* set, unsigned count, char tag)
{
    for (unsigned i = 0; i < count; i++) {
        const ReplayField& f = kReplayFields[set[i]];
        if (set[i] == RF_GAME_TICKS || !FormatField(f, s_field))
            continue;
        if (s_known.Changed(f.name, s_field.Data())) {
            s_call.Add("  ");
            s_call.AddChar(tag);
            s_call.AddChar(' ');
            s_call.Add(f.name);
            s_call.AddChar(' ');
            s_call.Add(s_field.Data());
            s_call.AddChar('\n');
        }
    }
}

void Enter(int slot)
{
    if (s_depth++ != 0)
        return;
    s_slot = slot;
    s_tick = g_game == 0 ? -1 : g_game->ticks;
    s_recording = !s_windowed || (s_tick >= 0 && (unsigned)s_tick >= s_first && (unsigned)s_tick <= s_last);
    if (!s_recording)
        return;
    s_ordinals.Reset();
    s_call.Clear();
    s_tail.Clear();
    if (s_tick >= 0) {
        s_field.Clear();
        s_field.AddInt(s_tick);
        s_known.Set("Game.ticks", s_field.Data());
    }
    const ReplaySlotSets& sets = kReplaySlots[slot];
    CaptureSet(sets.reads, sets.readCount, 'r');
    g_replayCalls = 1;
}

void Leave()
{
    if (--s_depth != 0 || !s_recording)
        return;
    g_replayCalls = 0;
    const ReplaySlotSets& sets = kReplaySlots[s_slot];
    CaptureSet(sets.writes, sets.writeCount, 'w');
    s_out.Add("call ");
    if (s_tick >= 0)
        s_out.AddInt(s_tick);
    else
        s_out.AddChar('-');
    s_out.AddChar(' ');
    s_out.Add(sets.name);
    s_out.AddChar('\n');
    s_out.AddText(s_call);
    s_out.AddText(s_tail);
    s_recording = false;
}

class SlotScope {
public:
    explicit SlotScope(int slot)
    {
        Enter(slot);
    }
    ~SlotScope()
    {
        Leave();
    }
    bool Open() const
    {
        return s_recording && s_depth == 1;
    }
};

void AddParams(ReplayText& t, const char* tag, const MeteorParams* p)
{
    t.Add("  ");
    t.Add(tag);
    t.Add(" params.name ");
    char name[0x21];
    memcpy(name, p->name, 0x20);
    name[0x20] = '\0';
    t.AddString(name);
    t.Add("\n  ");
    t.Add(tag);
    t.Add(" params.radius ");
    t.AddInt(p->radius);
    t.Add("\n  ");
    t.Add(tag);
    t.Add(" params.density ");
    AddFloatBits(t, p->density);
    t.Add("\n  ");
    t.Add(tag);
    t.Add(" params.duration ");
    AddFloatBits(t, p->duration);
    t.Add("\n  ");
    t.Add(tag);
    t.Add(" params.interval ");
    AddFloatBits(t, p->interval);
    t.AddChar('\n');
}

void __cdecl RecUpdateWind()
{
    SlotScope scope(SLOT_UPDATE_WIND);
    s_innerWind->UpdateWind();
}

void __stdcall RecUpdateMeteors()
{
    SlotScope scope(SLOT_UPDATE_METEORS);
    s_innerWind->UpdateMeteors();
}

void __stdcall RecInitMeteors()
{
    SlotScope scope(SLOT_INIT_METEORS);
    s_innerWind->InitMeteors();
}

void __stdcall RecEmptyShutdown()
{
    SlotScope scope(SLOT_EMPTY_SHUTDOWN);
    s_innerWind->EmptyShutdownPreCleanup();
}

void __stdcall RecEnableMeteors()
{
    SlotScope scope(SLOT_ENABLE_METEORS);
    s_innerWind->EnableMeteors();
}

void __stdcall RecDisableMeteors()
{
    SlotScope scope(SLOT_DISABLE_METEORS);
    s_innerWind->DisableMeteors();
}

void __stdcall RecSetMeteorParams(MeteorParams* params)
{
    SlotScope scope(SLOT_SET_METEOR_PARAMS);
    if (scope.Open())
        AddParams(s_call, "a", params);
    s_innerWind->SetMeteorParams(params);
}

void __fastcall RecLoadMeteorDefaults(MeteorParams* params)
{
    SlotScope scope(SLOT_LOAD_METEOR_DEFAULTS);
    s_innerWind->LoadMeteorDefaults(params);
    if (scope.Open())
        AddParams(s_tail, "o", params);
}

void __stdcall RecSaveMeteors(HapiBank* file)
{
    SlotScope scope(SLOT_SAVE_METEORS);
    if (scope.Open()) {
        s_call.Add("  a bank ");
        AddObject(s_call, OBJ_BANK, file);
        s_call.AddChar('\n');
    }
    s_innerWind->SaveMeteors(file);
}

void __stdcall RecLoadMeteors(HapiBank* file)
{
    SlotScope scope(SLOT_LOAD_METEORS);
    if (scope.Open()) {
        s_call.Add("  a bank ");
        AddObject(s_call, OBJ_BANK, file);
        s_call.AddChar('\n');
    }
    s_innerWind->LoadMeteors(file);
}

void __stdcall RecCmdMeteor(CommandArgs* args)
{
    SlotScope scope(SLOT_CMD_METEOR);
    if (scope.Open()) {
        s_call.Add("  a args.count ");
        s_call.AddInt(args->count);
        s_call.AddChar('\n');
        for (int i = 0; i < args->count && i < 0x14; i++) {
            s_call.Add("  a args[");
            s_call.AddInt(i);
            s_call.Add("] ");
            s_call.AddString(args->args[i]);
            s_call.AddChar('\n');
        }
    }
    s_innerWind->CmdMeteor(args);
}

void __cdecl RecUpdateWindOnStream(RandStream stream)
{
    SlotScope scope(SLOT_UPDATE_WIND_ON_STREAM);
    if (scope.Open()) {
        s_call.Add("  a stream ");
        AddStream(s_call, stream);
        s_call.AddChar('\n');
    }
    s_innerWind->UpdateWindOnStream(stream);
}

int __stdcall RecSpawnProjectile(WeaponDef* weapon, Vec3* pos, Vec3* vel, int flag)
{
    ReplayImport call(RI_SPAWN_PROJECTILE);
    call.Ptr(weapon);
    call.Ptr(pos);
    call.Ptr(vel);
    call.Int(flag);
    return call.Result(s_innerWeapons->SpawnProjectile(weapon, pos, vel, flag));
}

WeaponDef* __stdcall RecFindWeaponByName(char* name)
{
    ReplayImport call(RI_FIND_WEAPON);
    call.Ptr(name);
    return call.Result(s_innerWeapons->FindWeaponByName(name));
}

int __stdcall RecBankOpenAccount(HapiBank* self, char* name)
{
    ReplayImport call(RI_BANK_OPEN_ACCOUNT);
    call.Ptr(self);
    call.Ptr(name);
    return call.Result(s_innerFiles->BankOpenAccount(self, name));
}

int __stdcall RecBankSetInteger(HapiBank* self, const char* name, int value)
{
    ReplayImport call(RI_BANK_SET_INTEGER);
    call.Ptr(self);
    call.Ptr(name);
    call.Int(value);
    return call.Result(s_innerFiles->BankSetInteger(self, name, value));
}

int __stdcall RecBankGetInteger(HapiBank* self, char* name, int def)
{
    ReplayImport call(RI_BANK_GET_INTEGER);
    call.Ptr(self);
    call.Ptr(name);
    call.Int(def);
    return call.Result(s_innerFiles->BankGetInteger(self, name, def));
}

void AddResult(ReplayText& t, const ReplayArg& r)
{
    if (r.kind == 0) {
        t.Add(" = ");
        t.AddInt(r.v.i);
    }
}

void AddSpace(ReplayText& t)
{
    t.AddChar(' ');
}


}

void ReplayImportEnd(ReplayImport& c)
{
    ReplayText& t = s_call;
    const ReplayArg* a = c.args;
    t.Add("  i ");
    t.Add(kImportNames[c.id]);
    switch (c.id) {
    case RI_GAME_RAND:
        AddSpace(t);
        AddStream(t, a[0].v.i);
        AddResult(t, c.result);
        break;
    case RI_RANDOM_INT:
        AddSpace(t);
        t.AddInt(a[0].v.i);
        AddResult(t, c.result);
        break;
    case RI_FATAL_ERROR:
        AddSpace(t);
        t.AddString((const char*)a[0].v.p);
        break;
    case RI_BUILD_DATA_PATH:
        AddSpace(t);
        t.AddString((const char*)a[1].v.p);
        AddSpace(t);
        t.AddString((const char*)a[2].v.p);
        AddSpace(t);
        t.AddString((const char*)a[3].v.p);
        t.Add(" = ");
        t.AddString((const char*)c.result.v.p);
        break;
    case RI_TDF_CONSTRUCT:
    case RI_TDF_DESTRUCT:
        AddSpace(t);
        AddObject(t, OBJ_FILE, a[0].v.p);
        break;
    case RI_TDF_LOAD_FILE:
    case RI_TDF_SELECT_RECORD: {
        const TdfFile* file = (const TdfFile*)a[0].v.p;
        AddSpace(t);
        AddObject(t, OBJ_FILE, file);
        AddSpace(t);
        t.AddString((const char*)a[1].v.p);
        AddResult(t, c.result);
        if (c.id == RI_TDF_LOAD_FILE) {
            t.Add("\n  o ");
            AddObject(t, OBJ_FILE, file);
            t.Add(".root ");
            AddObject(t, OBJ_RECORD, file->root);
        }
        t.Add("\n  o ");
        AddObject(t, OBJ_FILE, file);
        t.Add(".current ");
        AddObject(t, OBJ_RECORD, file->current);
        break;
    }
    case RI_TDF_GET_INT:
        AddSpace(t);
        AddObject(t, OBJ_RECORD, a[0].v.p);
        AddSpace(t);
        t.AddString((const char*)a[1].v.p);
        AddSpace(t);
        t.AddInt(a[2].v.i);
        AddResult(t, c.result);
        break;
    case RI_TDF_GET_DOUBLE:
        AddSpace(t);
        AddObject(t, OBJ_RECORD, a[0].v.p);
        AddSpace(t);
        t.AddString((const char*)a[1].v.p);
        AddSpace(t);
        AddDoubleBits(t, a[2].v.d);
        t.Add(" = ");
        AddDoubleBits(t, c.result.v.d);
        break;
    case RI_TDF_GET_STRING:
        AddSpace(t);
        AddObject(t, OBJ_RECORD, a[0].v.p);
        AddSpace(t);
        t.AddString((const char*)a[2].v.p);
        AddSpace(t);
        t.AddInt(a[3].v.i);
        AddSpace(t);
        t.AddString((const char*)a[4].v.p);
        AddResult(t, c.result);
        t.Add("\n  o dst ");
        t.AddString((const char*)a[1].v.p);
        break;
    case RI_GET_INT_ARG:
        AddSpace(t);
        t.AddInt(a[0].v.i);
        AddSpace(t);
        t.AddInt(a[1].v.i);
        AddResult(t, c.result);
        break;
    case RI_BANK_OPEN_ACCOUNT:
    case RI_BANK_SET_INTEGER:
    case RI_BANK_GET_INTEGER:
        AddSpace(t);
        AddObject(t, OBJ_BANK, a[0].v.p);
        AddSpace(t);
        t.AddString((const char*)a[1].v.p);
        if (c.id != RI_BANK_OPEN_ACCOUNT) {
            AddSpace(t);
            t.AddInt(a[2].v.i);
        }
        AddResult(t, c.result);
        break;
    case RI_SPAWN_PROJECTILE:
        AddSpace(t);
        AddWeapon(t, a[0].v.p);
        t.Add(" pos=");
        AddVec3(t, (const Vec3*)a[1].v.p);
        t.Add(" vel=");
        AddVec3(t, (const Vec3*)a[2].v.p);
        AddSpace(t);
        t.AddInt(a[3].v.i);
        AddResult(t, c.result);
        break;
    case RI_FIND_WEAPON:
        AddSpace(t);
        t.AddString((const char*)a[0].v.p);
        t.Add(" = ");
        AddWeapon(t, c.result.v.p);
        if (c.result.v.p != 0) {
            t.Add("\n  o ");
            AddWeapon(t, c.result.v.p);
            t.Add(".flags ");
            t.AddHex32(((const WeaponDef*)c.result.v.p)->flags.value);
        }
        break;
    }
    t.AddChar('\n');
    g_replayCalls = 1;
}

int ReplayConfigure(const char* value, char* error, int size)
{
    char subsystem[32];
    unsigned first = 0;
    unsigned last = 0;
    int fields = sscanf(value, "%31s %u %u", subsystem, &first, &last);
    if (fields != 1 && fields != 3) {
        _snprintf(error, size, ": needs <subsystem> or <subsystem> <first tick> <last tick>");
        return 0;
    }
    if (strcmp(subsystem, "wind_meteors") != 0) {
        _snprintf(error, size, ": only wind_meteors can be recorded");
        return 0;
    }
    if (fields == 3 && (first == 0 || last < first)) {
        _snprintf(error, size, ": needs a first tick of at least 1 and a last tick not before it");
        return 0;
    }
    s_on = true;
    s_windowed = fields == 3;
    s_first = first;
    s_last = last;
    return 1;
}

void ReplayInstall()
{
    if (!s_on)
        return;
    s_innerWind = g_seams.wind_meteors;
    s_innerWeapons = g_seams.weapons;
    s_innerFiles = g_seams.files;
    s_wind = *s_innerWind;
    s_wind.UpdateWind = RecUpdateWind;
    s_wind.UpdateMeteors = RecUpdateMeteors;
    s_wind.InitMeteors = RecInitMeteors;
    s_wind.EmptyShutdownPreCleanup = RecEmptyShutdown;
    s_wind.EnableMeteors = RecEnableMeteors;
    s_wind.DisableMeteors = RecDisableMeteors;
    s_wind.SetMeteorParams = RecSetMeteorParams;
    s_wind.LoadMeteorDefaults = RecLoadMeteorDefaults;
    s_wind.SaveMeteors = RecSaveMeteors;
    s_wind.LoadMeteors = RecLoadMeteors;
    s_wind.CmdMeteor = RecCmdMeteor;
    s_wind.UpdateWindOnStream = RecUpdateWindOnStream;
    s_weapons = *s_innerWeapons;
    s_weapons.SpawnProjectile = RecSpawnProjectile;
    s_weapons.FindWeaponByName = RecFindWeaponByName;
    s_files = *s_innerFiles;
    s_files.BankOpenAccount = RecBankOpenAccount;
    s_files.BankSetInteger = RecBankSetInteger;
    s_files.BankGetInteger = RecBankGetInteger;
    g_seams.wind_meteors = &s_wind;
    g_seams.weapons = &s_weapons;
    g_seams.files = &s_files;
}

int ReplayWrite()
{
    if (!s_on)
        return 1;
    FILE* file = fopen("replay-wind_meteors.txt", "wb");
    char head[64];
    sprintf(head, "bt-replay %d\nsubsystem wind_meteors %d.%d\n", REPLAY_FORMAT, WIND_METEORS_VERSION_MAJOR, WIND_METEORS_VERSION_MINOR);
    return file != 0 && fputs(head, file) >= 0 && fwrite(s_out.Data(), 1, s_out.Size(), file) == s_out.Size() && fclose(file) == 0;
}
