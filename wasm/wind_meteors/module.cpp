#include <stdlib.h>
#include <string.h>
#include "src/state/game_rand.h"

#pragma pack(push, 1)
#include "src/game/game.h"
#pragma pack(pop)

#include "src/weapons/weapon_def.h"
#include "src/util/vec3.h"
#include "src/files/files_api.h"
#include "src/util/tdf.h"
#include "src/util/commands.h"
#include "interfaces/services.h"
#include "interfaces/seams.h"
#include "src/state/sim_globals.h"
#include "src/map/meteors.h"
#include "src/game/game_state.h"
#include "src/game/console_commands.h"

#define IMPORT(module, name) __attribute__((import_module(module), import_name(name)))
#define EXPORT(name) __attribute__((export_name(name)))

extern "C" {
IMPORT("services", "GameRand") int service_GameRand(RandStream stream);
IMPORT("services", "RandomInt") int service_RandomInt(int range);
IMPORT("services", "FatalError") void service_FatalError(char* message);
IMPORT("services", "BuildDataPath") char* service_BuildDataPath(char* buf, const char* dir, const char* name, const char* ext);
IMPORT("services", "TdfFile_Construct") void service_TdfFile_Construct(TdfFile* self);
IMPORT("services", "TdfFile_Destruct") void service_TdfFile_Destruct(TdfFile* self);
IMPORT("services", "TdfFile_LoadFile") int service_TdfFile_LoadFile(TdfFile* self, char* path);
IMPORT("services", "TdfFile_SelectRecord") int service_TdfFile_SelectRecord(TdfFile* self, char* name);
IMPORT("services", "TdfRecord_GetFieldInt") int service_TdfRecord_GetFieldInt(TdfRecord* self, const char* name, int def);
IMPORT("services", "TdfRecord_GetFieldDouble") double service_TdfRecord_GetFieldDouble(TdfRecord* self, const char* name, double def);
IMPORT("services", "TdfRecord_GetFieldString") int service_TdfRecord_GetFieldString(TdfRecord* self, char* dst, char* key, size_t size, char* def);
IMPORT("services", "HapiBank_OpenAccount") int service_HapiBank_OpenAccount(HapiBank* self, char* name);
IMPORT("services", "HapiBank_SetIntegerItem") int service_HapiBank_SetIntegerItem(HapiBank* self, const char* name, int value);
IMPORT("services", "HapiBank_GetIntegerItem") int service_HapiBank_GetIntegerItem(HapiBank* self, char* name, int def);
IMPORT("services", "CommandArgs_GetIntArg") int service_CommandArgs_GetIntArg(CommandArgs* self, int index, int def);
IMPORT("weapons", "SpawnProjectile") int weapons_SpawnProjectile(WeaponDef* weapon, Vec3* pos, Vec3* vel, int flag);
IMPORT("weapons", "FindWeaponByName") WeaponDef* weapons_FindWeaponByName(char* name);
}

int __cdecl GameRand(RandStream stream) { return service_GameRand(stream); }
int __stdcall RandomInt(int range) { return service_RandomInt(range); }
void __stdcall FatalError(char* message) { service_FatalError(message); }
char* __stdcall BuildDataPath(char* buf, const char* dir, const char* name, const char* ext)
{
    return service_BuildDataPath(buf, dir, name, ext);
}

TdfFile::TdfFile() { service_TdfFile_Construct(this); }
TdfFile::~TdfFile() { service_TdfFile_Destruct(this); }
int TdfFile::LoadFile(char* path) { return service_TdfFile_LoadFile(this, path); }
int TdfFile::SelectRecord(char* name) { return service_TdfFile_SelectRecord(this, name); }
int TdfRecord::GetFieldInt(const char* name, int def) { return service_TdfRecord_GetFieldInt(this, name, def); }
double TdfRecord::GetFieldDouble(const char* name, double def) { return service_TdfRecord_GetFieldDouble(this, name, def); }
int TdfRecord::GetFieldString(char* dst, char* key, size_t size, char* def)
{
    return service_TdfRecord_GetFieldString(this, dst, key, size, def);
}
int CommandArgs::GetIntArg(int index, int def) { return service_CommandArgs_GetIntArg(this, index, def); }

SimGlobals& SimCreate() { __builtin_trap(); }

extern "C" int bx_ScaledSin(short angle, int scale);
extern "C" int bx_ScaledCos(short angle, int scale);
int __cdecl ScaledSin(int angle, int scale) { return bx_ScaledSin((short)angle, scale); }
int __cdecl ScaledCos(int angle, int scale) { return bx_ScaledCos((short)angle, scale); }

Game* g_game;
SimGlobals* g_simGlobals;
char g_extTdf[4];
unsigned int g_blockBase;

static void __fastcall LoadMeteorDefaultsSlot(MeteorParams* params) { params->LoadMeteorDefaults(); }

static WindMeteorsSeam s_windMeteors = {
    .UpdateWind = UpdateWind,
    .UpdateMeteors = UpdateMeteors,
    .InitMeteors = InitMeteors,
    .EmptyShutdownPreCleanup = EmptyShutdownPreCleanup,
    .EnableMeteors = EnableMeteors,
    .DisableMeteors = DisableMeteors,
    .SetMeteorParams = SetMeteorParams,
    .LoadMeteorDefaults = LoadMeteorDefaultsSlot,
    .SaveMeteors = SaveMeteors,
    .LoadMeteors = LoadMeteors,
    .CmdMeteor = CmdMeteor,
    .UpdateWindOnStream = UpdateWindOnStream,
};

static WeaponsSeam s_weapons = {
    .SpawnProjectile = weapons_SpawnProjectile,
    .FindWeaponByName = weapons_FindWeaponByName,
};

static FilesSeam s_files = {
    .BankOpenAccount = service_HapiBank_OpenAccount,
    .BankSetInteger = service_HapiBank_SetIntegerItem,
    .BankGetInteger = service_HapiBank_GetIntegerItem,
};

Seams g_seams = {
    .wind_meteors = &s_windMeteors,
    .weapons = &s_weapons,
    .files = &s_files,
};

extern "C" {
EXPORT("UpdateWind") void export_UpdateWind(void) { s_windMeteors.UpdateWind(); }
EXPORT("UpdateMeteors") void export_UpdateMeteors(void) { s_windMeteors.UpdateMeteors(); }
EXPORT("InitMeteors") void export_InitMeteors(void) { s_windMeteors.InitMeteors(); }
EXPORT("EmptyShutdownPreCleanup") void export_EmptyShutdownPreCleanup(void) { s_windMeteors.EmptyShutdownPreCleanup(); }
EXPORT("EnableMeteors") void export_EnableMeteors(void) { s_windMeteors.EnableMeteors(); }
EXPORT("DisableMeteors") void export_DisableMeteors(void) { s_windMeteors.DisableMeteors(); }
EXPORT("SetMeteorParams") void export_SetMeteorParams(MeteorParams* params) { s_windMeteors.SetMeteorParams(params); }
EXPORT("LoadMeteorDefaults") void export_LoadMeteorDefaults(MeteorParams* params) { s_windMeteors.LoadMeteorDefaults(params); }
EXPORT("SaveMeteors") void export_SaveMeteors(HapiBank* file) { s_windMeteors.SaveMeteors(file); }
EXPORT("LoadMeteors") void export_LoadMeteors(HapiBank* file) { s_windMeteors.LoadMeteors(file); }
EXPORT("CmdMeteor") void export_CmdMeteor(CommandArgs* args) { s_windMeteors.CmdMeteor(args); }
EXPORT("UpdateWindOnStream") void export_UpdateWindOnStream(RandStream stream) { s_windMeteors.UpdateWindOnStream(stream); }
}

#include "layout_generated.h"
#include "wasm/wind_meteors/layout_asserts.h"
