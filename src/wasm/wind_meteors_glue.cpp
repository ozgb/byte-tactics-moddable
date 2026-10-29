#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include "../state/game_rand.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../weapons/weapon_def.h"
#include "../util/vec3.h"
#include "../files/files_bank.h"
#include "../util/commands.h"
#include "../../interfaces/services.h"
#include "../../interfaces/seams.h"
#include "../state/sim_globals.h"
#include "../map/meteors.h"
#include "../game/los_helpers.h"
#include "../state/block.h"
#include "../../interfaces/rand_stream.h"
#include "../../interfaces/wind_meteors.h"
#include <wasmtime.h>
#include "wasmloader.h"
#include "modules.h"

namespace {

wl_loader* s_loader;
wl_module* s_module;
char s_error[240];

const char* ImportError(const char* import, const char* what)
{
    sprintf(s_error, "import %s: %s", import, what);
    return s_error;
}

const unsigned kHandleCapacity = 32;
void* s_handles[kHandleCapacity];
unsigned s_handleCount;
int s_slotDepth;

unsigned HandleOf(void* native)
{
    if (native == nullptr)
        return 0;
    for (unsigned i = 0; i < s_handleCount; i++)
        if (s_handles[i] == native)
            return i + 1;
    if (s_handleCount == kHandleCapacity)
        ModulesFatal("wind_meteors: the handle table is full");
    s_handles[s_handleCount] = native;
    return ++s_handleCount;
}

const char* NativeOf(const char* import, unsigned handle, void** native)
{
    if (handle > s_handleCount) {
        sprintf(s_error, "import %s: unknown handle %u", import, handle);
        return s_error;
    }
    *native = handle == 0 ? nullptr : s_handles[handle - 1];
    return nullptr;
}

const char* LiveOf(const char* import, unsigned handle, void** native)
{
    const char* error = NativeOf(import, handle, native);
    if (error == nullptr && *native == nullptr) {
        sprintf(s_error, "import %s: null handle", import);
        return s_error;
    }
    return error;
}

const char* FileToNative(const char* import, TdfFile* file)
{
    void* root;
    void* current;
    const char* error = NativeOf(import, (unsigned)file->root, &root);
    if (error == nullptr)
        error = NativeOf(import, (unsigned)file->current, &current);
    if (error != nullptr)
        return error;
    file->root = (TdfRecord*)root;
    file->current = (TdfRecord*)current;
    return nullptr;
}

void FileToHandles(TdfFile* file)
{
    file->root = (TdfRecord*)HandleOf(file->root);
    file->current = (TdfRecord*)HandleOf(file->current);
}

int InMemory(unsigned p)
{
    return p < ModulesMemory()->size;
}

void* Linear(unsigned p)
{
    return p == 0 ? nullptr : ModulesMemory()->base + p;
}

#define NEED(import, slot) \
    if (!InMemory((unsigned)a[slot].i32)) \
        return ImportError(import, "pointer argument " #slot " is outside the module memory")

#define IMPORT(name) const char* __cdecl name(void* user, wasmtime_val_raw_t* a)

IMPORT(GameRandImport)
{
    (void)user;
    a[0].i32 = GameRand((RandStream)a[0].i32);
    return nullptr;
}

IMPORT(RandomIntImport)
{
    (void)user;
    a[0].i32 = RandomInt(a[0].i32);
    return nullptr;
}

IMPORT(FatalErrorImport)
{
    (void)user;
    NEED("FatalError", 0);
    FatalError((char*)Linear(a[0].i32));
    return nullptr;
}

IMPORT(BuildDataPathImport)
{
    (void)user;
    NEED("BuildDataPath", 0);
    NEED("BuildDataPath", 1);
    NEED("BuildDataPath", 2);
    NEED("BuildDataPath", 3);
    char* result = BuildDataPath((char*)Linear(a[0].i32), (const char*)Linear(a[1].i32), (const char*)Linear(a[2].i32), (const char*)Linear(a[3].i32));
    if (result != (char*)Linear(a[0].i32))
        return ImportError("BuildDataPath", "returned a buffer other than the one it was given");
    return nullptr;
}

IMPORT(TdfFileConstructImport)
{
    (void)user;
    NEED("TdfFile_Construct", 0);
    new (Linear(a[0].i32)) TdfFile();
    return nullptr;
}

IMPORT(TdfFileDestructImport)
{
    (void)user;
    NEED("TdfFile_Destruct", 0);
    TdfFile* file = (TdfFile*)Linear(a[0].i32);
    const char* error = FileToNative("TdfFile_Destruct", file);
    if (error != nullptr)
        return error;
    file->~TdfFile();
    FileToHandles(file);
    return nullptr;
}

IMPORT(TdfFileLoadFileImport)
{
    (void)user;
    NEED("TdfFile_LoadFile", 0);
    NEED("TdfFile_LoadFile", 1);
    TdfFile* file = (TdfFile*)Linear(a[0].i32);
    const char* error = FileToNative("TdfFile_LoadFile", file);
    if (error != nullptr)
        return error;
    a[0].i32 = file->LoadFile((char*)Linear(a[1].i32));
    FileToHandles(file);
    return nullptr;
}

IMPORT(TdfFileSelectRecordImport)
{
    (void)user;
    NEED("TdfFile_SelectRecord", 0);
    NEED("TdfFile_SelectRecord", 1);
    TdfFile* file = (TdfFile*)Linear(a[0].i32);
    const char* error = FileToNative("TdfFile_SelectRecord", file);
    if (error != nullptr)
        return error;
    a[0].i32 = file->SelectRecord((char*)Linear(a[1].i32));
    FileToHandles(file);
    return nullptr;
}

IMPORT(TdfRecordGetFieldIntImport)
{
    (void)user;
    NEED("TdfRecord_GetFieldInt", 1);
    void* record;
    const char* error = LiveOf("TdfRecord_GetFieldInt", (unsigned)a[0].i32, &record);
    if (error != nullptr)
        return error;
    a[0].i32 = ((TdfRecord*)record)->GetFieldInt((const char*)Linear(a[1].i32), a[2].i32);
    return nullptr;
}

IMPORT(TdfRecordGetFieldDoubleImport)
{
    (void)user;
    NEED("TdfRecord_GetFieldDouble", 1);
    void* record;
    const char* error = LiveOf("TdfRecord_GetFieldDouble", (unsigned)a[0].i32, &record);
    if (error != nullptr)
        return error;
    a[0].f64 = ((TdfRecord*)record)->GetFieldDouble((const char*)Linear(a[1].i32), a[2].f64);
    return nullptr;
}

IMPORT(TdfRecordGetFieldStringImport)
{
    (void)user;
    NEED("TdfRecord_GetFieldString", 1);
    NEED("TdfRecord_GetFieldString", 2);
    NEED("TdfRecord_GetFieldString", 4);
    void* record;
    const char* error = LiveOf("TdfRecord_GetFieldString", (unsigned)a[0].i32, &record);
    if (error != nullptr)
        return error;
    a[0].i32 = ((TdfRecord*)record)->GetFieldString((char*)Linear(a[1].i32), (char*)Linear(a[2].i32), (size_t)(unsigned)a[3].i32, (char*)Linear(a[4].i32));
    return nullptr;
}

IMPORT(CommandArgsGetIntArgImport)
{
    (void)user;
    NEED("CommandArgs_GetIntArg", 0);
    a[0].i32 = ((CommandArgs*)Linear(a[0].i32))->GetIntArg(a[1].i32, a[2].i32);
    return nullptr;
}

IMPORT(HapiBankOpenAccountImport)
{
    (void)user;
    NEED("HapiBank_OpenAccount", 1);
    void* bank;
    const char* error = LiveOf("HapiBank_OpenAccount", (unsigned)a[0].i32, &bank);
    if (error != nullptr)
        return error;
    a[0].i32 = ((HapiBank*)bank)->OpenAccount((char*)Linear(a[1].i32));
    return nullptr;
}

IMPORT(HapiBankSetIntegerItemImport)
{
    (void)user;
    NEED("HapiBank_SetIntegerItem", 1);
    void* bank;
    const char* error = LiveOf("HapiBank_SetIntegerItem", (unsigned)a[0].i32, &bank);
    if (error != nullptr)
        return error;
    a[0].i32 = ((HapiBank*)bank)->SetIntegerItem((const char*)Linear(a[1].i32), a[2].i32);
    return nullptr;
}

IMPORT(HapiBankGetIntegerItemImport)
{
    (void)user;
    NEED("HapiBank_GetIntegerItem", 1);
    void* bank;
    const char* error = LiveOf("HapiBank_GetIntegerItem", (unsigned)a[0].i32, &bank);
    if (error != nullptr)
        return error;
    a[0].i32 = ((HapiBank*)bank)->GetIntegerItem((char*)Linear(a[1].i32), a[2].i32);
    return nullptr;
}

IMPORT(SpawnProjectileImport)
{
    (void)user;
    NEED("SpawnProjectile", 0);
    NEED("SpawnProjectile", 1);
    NEED("SpawnProjectile", 2);
    a[0].i32 = g_seams.weapons->SpawnProjectile((WeaponDef*)Linear(a[0].i32), (Vec3*)Linear(a[1].i32), (Vec3*)Linear(a[2].i32), a[3].i32);
    return nullptr;
}

IMPORT(FindWeaponByNameImport)
{
    (void)user;
    NEED("FindWeaponByName", 0);
    WeaponDef* weapon = g_seams.weapons->FindWeaponByName((char*)Linear(a[0].i32));
    if (weapon != nullptr && !SimOwns(weapon))
        return ImportError("FindWeaponByName", "returned a weapon outside the sim block");
    a[0].i32 = weapon == nullptr ? 0 : (int)((unsigned char*)weapon - ModulesMemory()->base);
    return nullptr;
}

struct ImportDef {
    const char* module;
    const char* name;
    const char* signature;
    wl_import_fn function;
};

const ImportDef kImports[] = {
    {"services", "GameRand", "i>i", GameRandImport},
    {"services", "RandomInt", "i>i", RandomIntImport},
    {"services", "FatalError", "i>", FatalErrorImport},
    {"services", "BuildDataPath", "iiii>i", BuildDataPathImport},
    {"services", "TdfFile_Construct", "i>", TdfFileConstructImport},
    {"services", "TdfFile_Destruct", "i>", TdfFileDestructImport},
    {"services", "TdfFile_LoadFile", "ii>i", TdfFileLoadFileImport},
    {"services", "TdfFile_SelectRecord", "ii>i", TdfFileSelectRecordImport},
    {"services", "TdfRecord_GetFieldInt", "iii>i", TdfRecordGetFieldIntImport},
    {"services", "TdfRecord_GetFieldDouble", "iid>d", TdfRecordGetFieldDoubleImport},
    {"services", "TdfRecord_GetFieldString", "iiiii>i", TdfRecordGetFieldStringImport},
    {"services", "CommandArgs_GetIntArg", "iii>i", CommandArgsGetIntArgImport},
    {"services", "HapiBank_OpenAccount", "ii>i", HapiBankOpenAccountImport},
    {"services", "HapiBank_SetIntegerItem", "iii>i", HapiBankSetIntegerItemImport},
    {"services", "HapiBank_GetIntegerItem", "iii>i", HapiBankGetIntegerItemImport},
    {"weapons", "SpawnProjectile", "iiii>i", SpawnProjectileImport},
    {"weapons", "FindWeaponByName", "i>i", FindWeaponByNameImport},
};

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

const char* const kSlotNames[SLOT_COUNT] = {
    "UpdateWind", "UpdateMeteors", "InitMeteors", "EmptyShutdownPreCleanup", "EnableMeteors", "DisableMeteors",
    "SetMeteorParams", "LoadMeteorDefaults", "SaveMeteors", "LoadMeteors", "CmdMeteor", "UpdateWindOnStream",
};

wasmtime_func_t s_slots[SLOT_COUNT];
unsigned s_gameAddress;
unsigned s_simAddress;
unsigned s_extTdfAddress;
unsigned s_blockBaseAddress;

void Stop(Slot slot, const char* what)
{
    char text[200];
    sprintf(text, "slot %s: %s", kSlotNames[slot], what);
    ModulesFatal(text);
}

void Prepare(Slot slot)
{
    void* sim = &Sim();
    if (!SimOwns(g_game))
        Stop(slot, "g_game is not in the sim block");
    if (!SimOwns(sim))
        Stop(slot, "g_simGlobals is not in the sim block");
    unsigned game = (unsigned)((unsigned char*)g_game - ModulesMemory()->base);
    unsigned sims = (unsigned)((unsigned char*)sim - ModulesMemory()->base);
    memcpy(ModulesMemory()->base + s_gameAddress, &game, 4);
    memcpy(ModulesMemory()->base + s_simAddress, &sims, 4);
    memcpy(ModulesMemory()->base + s_extTdfAddress, g_extTdf, 4);
    unsigned blockBase = (unsigned)((unsigned char*)SimBlockBase() - ModulesMemory()->base);
    memcpy(ModulesMemory()->base + s_blockBaseAddress, &blockBase, 4);
}

void Call(Slot slot, wasmtime_val_raw_t* a, size_t count)
{
    Prepare(slot);
    s_slotDepth++;
    wl_call(s_loader, s_module, &s_slots[slot], a, count);
    if (--s_slotDepth == 0)
        s_handleCount = 0;
}

void Call0(Slot slot)
{
    wasmtime_val_raw_t a[1];
    a[0].i32 = 0;
    Call(slot, a, 0);
}

void Call1(Slot slot, unsigned arg)
{
    wasmtime_val_raw_t a[1];
    a[0].i32 = (int)arg;
    Call(slot, a, 1);
}

unsigned ToScratch(Slot slot, const void* source, unsigned size, int copyIn)
{
    if (size > ModulesMemory()->scratchSize)
        Stop(slot, "the argument does not fit the scratch area");
    if (copyIn)
        memcpy(ModulesMemory()->base + ModulesMemory()->scratch, source, size);
    return ModulesMemory()->scratch;
}

unsigned ParamsArgument(Slot slot, MeteorParams* params, int copyIn)
{
    if (SimOwns(params))
        return (unsigned)((unsigned char*)params - ModulesMemory()->base);
    return ToScratch(slot, params, sizeof(MeteorParams), copyIn);
}

void __cdecl UpdateWindThunk()
{
    Call0(SLOT_UPDATE_WIND);
}

void __stdcall UpdateMeteorsThunk()
{
    Call0(SLOT_UPDATE_METEORS);
}

void __stdcall InitMeteorsThunk()
{
    Call0(SLOT_INIT_METEORS);
}

void __stdcall EmptyShutdownPreCleanupThunk()
{
    Call0(SLOT_EMPTY_SHUTDOWN);
}

void __stdcall EnableMeteorsThunk()
{
    Call0(SLOT_ENABLE_METEORS);
}

void __stdcall DisableMeteorsThunk()
{
    Call0(SLOT_DISABLE_METEORS);
}

void __stdcall SetMeteorParamsThunk(MeteorParams* params)
{
    Call1(SLOT_SET_METEOR_PARAMS, ParamsArgument(SLOT_SET_METEOR_PARAMS, params, 1));
}

void __fastcall LoadMeteorDefaultsThunk(MeteorParams* params)
{
    unsigned argument = ParamsArgument(SLOT_LOAD_METEOR_DEFAULTS, params, 1);
    Call1(SLOT_LOAD_METEOR_DEFAULTS, argument);
    if (!SimOwns(params))
        memcpy(params, ModulesMemory()->base + argument, sizeof(MeteorParams));
}

void __stdcall SaveMeteorsThunk(HapiBank* file)
{
    Call1(SLOT_SAVE_METEORS, HandleOf(file));
}

void __stdcall LoadMeteorsThunk(HapiBank* file)
{
    Call1(SLOT_LOAD_METEORS, HandleOf(file));
}

void __stdcall CmdMeteorThunk(CommandArgs* args)
{
    Call1(SLOT_CMD_METEOR, ToScratch(SLOT_CMD_METEOR, args, sizeof(CommandArgs), 1));
}

void __cdecl UpdateWindOnStreamThunk(RandStream stream)
{
    Call1(SLOT_UPDATE_WIND_ON_STREAM, (unsigned)stream);
}

WindMeteorsSeam s_seam;

}

int __stdcall WindMeteorsInstall(wl_loader* loader)
{
    s_loader = loader;
    for (unsigned i = 0; i < sizeof kImports / sizeof kImports[0]; i++) {
        const ImportDef& def = kImports[i];
        if (!wl_define_import(loader, def.module, def.name, def.signature, def.function, nullptr))
            return 0;
    }
    return 1;
}

int __stdcall WindMeteorsBind(wl_loader* loader, wl_module* module)
{
    s_module = module;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (!wl_export_func(loader, module, kSlotNames[i], &s_slots[i]))
            return 0;
    }
    for (size_t i = 0; i < wl_data_count(module); i++) {
        const char* name = wl_data_name(module, i);
        if (strcmp(name, "g_game") == 0)
            s_gameAddress = wl_data_address(module, i);
        else if (strcmp(name, "g_simGlobals") == 0)
            s_simAddress = wl_data_address(module, i);
        else if (strcmp(name, "g_extTdf") == 0)
            s_extTdfAddress = wl_data_address(module, i);
        else if (strcmp(name, "g_blockBase") == 0)
            s_blockBaseAddress = wl_data_address(module, i);
    }
    if (s_gameAddress == 0 || s_simAddress == 0 || s_extTdfAddress == 0 || s_blockBaseAddress == 0)
        return 0;
    s_seam.UpdateWind = UpdateWindThunk;
    s_seam.UpdateMeteors = UpdateMeteorsThunk;
    s_seam.InitMeteors = InitMeteorsThunk;
    s_seam.EmptyShutdownPreCleanup = EmptyShutdownPreCleanupThunk;
    s_seam.EnableMeteors = EnableMeteorsThunk;
    s_seam.DisableMeteors = DisableMeteorsThunk;
    s_seam.SetMeteorParams = SetMeteorParamsThunk;
    s_seam.LoadMeteorDefaults = LoadMeteorDefaultsThunk;
    s_seam.SaveMeteors = SaveMeteorsThunk;
    s_seam.LoadMeteors = LoadMeteorsThunk;
    s_seam.CmdMeteor = CmdMeteorThunk;
    s_seam.UpdateWindOnStream = UpdateWindOnStreamThunk;
    return 1;
}

void __stdcall WindMeteorsUseSeam()
{
    g_seams.wind_meteors = &s_seam;
}

void __stdcall WindMeteorsDataRanges(unsigned* addresses, unsigned* count)
{
    addresses[0] = s_gameAddress;
    addresses[1] = s_simAddress;
    addresses[2] = s_extTdfAddress;
    addresses[3] = s_blockBaseAddress;
    *count = 4;
}
