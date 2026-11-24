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
#include "interfaces/services.h"
#include "src/util/engine_services.h"
#include "interfaces/seams.h"
#include "src/state/sim_globals.h"
#include "src/map/meteors.h"
#include "src/game/game_state.h"

#define IMPORT(module, name) __attribute__((import_module(module), import_name(name)))
#define EXPORT(name) __attribute__((export_name(name)))

extern "C" {
IMPORT("services", "GameRand") int service_GameRand(RandStream stream);
IMPORT("services", "RandomInt") int service_RandomInt(int range);
IMPORT("services", "FatalError") void service_FatalError(const char* message);
IMPORT("services", "BuildDataPath") char* service_BuildDataPath(char* buf, const char* dir, const char* name, const char* ext);
IMPORT("services", "TdfFile_Construct") void service_TdfFile_Construct(TdfFileLayout* self);
IMPORT("services", "TdfFile_Destruct") void service_TdfFile_Destruct(TdfFileLayout* self);
IMPORT("services", "TdfFile_LoadFile") int service_TdfFile_LoadFile(TdfFileLayout* self, const char* path);
IMPORT("services", "TdfFile_SelectRecord") int service_TdfFile_SelectRecord(TdfFileLayout* self, const char* name);
IMPORT("services", "TdfRecord_GetFieldInt") int service_TdfRecord_GetFieldInt(TdfRecord* self, const char* name, int def);
IMPORT("services", "TdfRecord_GetFieldDouble") double service_TdfRecord_GetFieldDouble(TdfRecord* self, const char* name, double def);
IMPORT("services", "TdfRecord_GetFieldString") int service_TdfRecord_GetFieldString(TdfRecord* self, char* dst, const char* key, int size, const char* def);
IMPORT("services", "HapiBank_OpenAccount") int service_HapiBank_OpenAccount(HapiBank* self, const char* name);
IMPORT("services", "HapiBank_SetIntegerItem") int service_HapiBank_SetIntegerItem(HapiBank* self, const char* name, int value);
IMPORT("services", "Emit") void service_Emit(const void* effect, unsigned int size);
IMPORT("services", "HapiBank_GetIntegerItem") int service_HapiBank_GetIntegerItem(HapiBank* self, const char* name, int def);
IMPORT("weapons", "FindWeaponByName") WeaponDef* weapons_FindWeaponByName(const char* name);
}

int __cdecl GameRand(RandStream stream) { return service_GameRand(stream); }
int __stdcall RandomInt(int range) { return service_RandomInt(range); }
void __stdcall FatalError(const char* message) { service_FatalError(message); }
char* __stdcall BuildDataPath(char* buf, const char* dir, const char* name, const char* ext)
{
    return service_BuildDataPath(buf, dir, name, ext);
}

TdfFile::TdfFile() { service_TdfFile_Construct((TdfFileLayout*)this); }
TdfFile::~TdfFile() { service_TdfFile_Destruct((TdfFileLayout*)this); }
int TdfFile::LoadFile(const char* path) { return service_TdfFile_LoadFile((TdfFileLayout*)this, path); }
int TdfFile::SelectRecord(const char* name) { return service_TdfFile_SelectRecord((TdfFileLayout*)this, name); }
int TdfRecord::GetFieldInt(const char* name, int def) { return service_TdfRecord_GetFieldInt(this, name, def); }
double TdfRecord::GetFieldDouble(const char* name, double def) { return service_TdfRecord_GetFieldDouble(this, name, def); }
int TdfRecord::GetFieldString(char* dst, char* key, int size, char* def)
{
    return service_TdfRecord_GetFieldString(this, dst, key, size, def);
}
void __stdcall EmitFrom(int, const void* effect, unsigned int size) { service_Emit(effect, size); }

SimGlobals& SimCreate() { __builtin_trap(); }

extern "C" int bx_ScaledSin(short angle, int scale);
extern "C" int bx_ScaledCos(short angle, int scale);
int __cdecl ScaledSin(int angle, int scale) { return bx_ScaledSin((short)angle, scale); }
int __cdecl ScaledCos(int angle, int scale) { return bx_ScaledCos((short)angle, scale); }

Game* g_game;
SimGlobals* g_simGlobals;
char g_extTdf[4];
unsigned int g_blockBase;

static WindMeteorsSeam s_windMeteors = {
    .UpdateWind = UpdateWindOnStream,
    .UpdateMeteors = UpdateMeteors,
    .Apply = WindMeteorsApply,
    .InitMeteors = InitMeteors,
    .EmptyShutdownPreCleanup = EmptyShutdownPreCleanup,
    .LoadMeteorDefaults = LoadMeteorDefaults,
    .SaveMeteors = SaveMeteors,
    .LoadMeteors = LoadMeteors,
};

static WeaponsSeam s_weapons = {
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
EXPORT("UpdateWind") void export_UpdateWind(RandStream stream) { s_windMeteors.UpdateWind(stream); }
EXPORT("UpdateMeteors") void export_UpdateMeteors(void) { s_windMeteors.UpdateMeteors(); }
EXPORT("Apply") void export_Apply(const WindMeteorsEventHeader* event, unsigned int size) { s_windMeteors.Apply(event, size); }
EXPORT("InitMeteors") void export_InitMeteors(void) { s_windMeteors.InitMeteors(); }
EXPORT("EmptyShutdownPreCleanup") void export_EmptyShutdownPreCleanup(void) { s_windMeteors.EmptyShutdownPreCleanup(); }
EXPORT("LoadMeteorDefaults") void export_LoadMeteorDefaults(MeteorShowerParams* params) { s_windMeteors.LoadMeteorDefaults(params); }
EXPORT("SaveMeteors") void export_SaveMeteors(HapiBank* file) { s_windMeteors.SaveMeteors(file); }
EXPORT("LoadMeteors") void export_LoadMeteors(HapiBank* file) { s_windMeteors.LoadMeteors(file); }
}

#include "layout_generated.h"
#include "wasm/wind_meteors/layout_asserts.h"
