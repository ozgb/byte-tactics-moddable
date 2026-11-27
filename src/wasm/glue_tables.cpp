#include <wasmtime.h>
#include "wasmloader.h"
#include "../units/unit_def.h"
#include "../../interfaces/host.h"
#include "../util/vec3.h"
#include "../util/angles.h"
#include "../../interfaces/seams.h"
#include "../util/engine_services.h"
#include "module_glue.h"

namespace {

constexpr unsigned GlueMax(unsigned a, unsigned b)
{
    return a > b ? a : b;
}

const char* const kWindMeteorsSlots[] = {
    "UpdateWind",
    "UpdateMeteors",
    "Apply",
    "InitMeteors",
    "EmptyShutdownPreCleanup",
    "LoadMeteorDefaults",
    "SaveMeteors",
    "LoadMeteors",
};
wasmtime_func_t s_windMeteorsFuncs[8];
GlueTiming s_windMeteorsTiming[8];
extern GlueModule s_windMeteorsGlue;
const WindMeteorsSeam* s_windMeteorsNative;
WindMeteorsSeam s_windMeteorsSeam;
WindMeteorsSeam s_windMeteorsTimed;

void __cdecl WindMeteors_UpdateWind(RandStream stream)
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)stream;
    GlueCall(glue, 0, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall WindMeteors_UpdateMeteors()
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 1, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall WindMeteors_Apply(const WindMeteorsEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(sizeof(WindMeteorsEventHeader), sizeof(WindMeteorsParams)), sizeof(WindMeteorsLimits)) ? (unsigned)GlueMax(GlueMax(sizeof(WindMeteorsEventHeader), sizeof(WindMeteorsParams)), sizeof(WindMeteorsLimits)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 2, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 2, slotArgs, 2);
    GlueScratchRelease(mark);
}

void __stdcall WindMeteors_InitMeteors()
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 3, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall WindMeteors_EmptyShutdownPreCleanup()
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 4, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall WindMeteors_LoadMeteorDefaults(MeteorShowerParams* params)
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned paramsAt = GlueIn(glue, 5, params, sizeof(MeteorShowerParams), 0);
    slotArgs[0].i32 = (int)paramsAt;
    GlueCall(glue, 5, slotArgs, 1);
    GlueOut((void*)params, paramsAt, sizeof(MeteorShowerParams));
    GlueScratchRelease(mark);
}

void __stdcall WindMeteors_SaveMeteors(HapiBank* file)
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueHandleOf((void*)file);
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall WindMeteors_LoadMeteors(HapiBank* file)
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueHandleOf((void*)file);
    GlueCall(glue, 7, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __cdecl WindMeteors_TimedUpdateWind(RandStream stream)
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->UpdateWind(stream);
    GlueTimed(&s_windMeteorsGlue, 0, start);
}

void __stdcall WindMeteors_TimedUpdateMeteors()
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->UpdateMeteors();
    GlueTimed(&s_windMeteorsGlue, 1, start);
}

void __stdcall WindMeteors_TimedApply(const WindMeteorsEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->Apply(event, size);
    GlueTimed(&s_windMeteorsGlue, 2, start);
}

void __stdcall WindMeteors_TimedInitMeteors()
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->InitMeteors();
    GlueTimed(&s_windMeteorsGlue, 3, start);
}

void __stdcall WindMeteors_TimedEmptyShutdownPreCleanup()
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->EmptyShutdownPreCleanup();
    GlueTimed(&s_windMeteorsGlue, 4, start);
}

void __stdcall WindMeteors_TimedLoadMeteorDefaults(MeteorShowerParams* params)
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->LoadMeteorDefaults(params);
    GlueTimed(&s_windMeteorsGlue, 5, start);
}

void __stdcall WindMeteors_TimedSaveMeteors(HapiBank* file)
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->SaveMeteors(file);
    GlueTimed(&s_windMeteorsGlue, 6, start);
}

void __stdcall WindMeteors_TimedLoadMeteors(HapiBank* file)
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->LoadMeteors(file);
    GlueTimed(&s_windMeteorsGlue, 7, start);
}

void WindMeteorsUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.wind_meteors == &s_windMeteorsSeam)
        return;
    s_windMeteorsNative = g_seams.wind_meteors;
    s_windMeteorsSeam = *s_windMeteorsNative;
    s_windMeteorsSeam.UpdateWind = WindMeteors_UpdateWind;
    s_windMeteorsSeam.UpdateMeteors = WindMeteors_UpdateMeteors;
    s_windMeteorsSeam.Apply = WindMeteors_Apply;
    s_windMeteorsSeam.InitMeteors = WindMeteors_InitMeteors;
    s_windMeteorsSeam.EmptyShutdownPreCleanup = WindMeteors_EmptyShutdownPreCleanup;
    s_windMeteorsSeam.LoadMeteorDefaults = WindMeteors_LoadMeteorDefaults;
    s_windMeteorsSeam.SaveMeteors = WindMeteors_SaveMeteors;
    s_windMeteorsSeam.LoadMeteors = WindMeteors_LoadMeteors;
    g_seams.wind_meteors = &s_windMeteorsSeam;
}

void WindMeteorsTimeNative()
{
    s_windMeteorsNative = g_seams.wind_meteors;
    s_windMeteorsTimed = *s_windMeteorsNative;
    s_windMeteorsTimed.UpdateWind = WindMeteors_TimedUpdateWind;
    s_windMeteorsTimed.UpdateMeteors = WindMeteors_TimedUpdateMeteors;
    s_windMeteorsTimed.Apply = WindMeteors_TimedApply;
    s_windMeteorsTimed.InitMeteors = WindMeteors_TimedInitMeteors;
    s_windMeteorsTimed.EmptyShutdownPreCleanup = WindMeteors_TimedEmptyShutdownPreCleanup;
    s_windMeteorsTimed.LoadMeteorDefaults = WindMeteors_TimedLoadMeteorDefaults;
    s_windMeteorsTimed.SaveMeteors = WindMeteors_TimedSaveMeteors;
    s_windMeteorsTimed.LoadMeteors = WindMeteors_TimedLoadMeteors;
    g_seams.wind_meteors = &s_windMeteorsTimed;
}

const GlueModuleDef kWindMeteorsDef = {"wind_meteors", SCHED_WIND_METEORS, kWindMeteorsSlots, 8, s_windMeteorsFuncs, s_windMeteorsTiming, WindMeteorsUseSeam, WindMeteorsTimeNative, 0, 0};
GlueModule s_windMeteorsGlue = {&kWindMeteorsDef};

const char* const kEconomySlots[] = {
    "RegionSize",
    "Init",
    "Shutdown",
    "Apply",
    "Spend",
    "UpdatePlayerEconomy",
    "UpdateMetalExtraction",
    "UpdateResourceSharing",
    "RequestGranted",
    "GetEnergyUse",
    "UnitAccount",
};
wasmtime_func_t s_economyFuncs[11];
GlueTiming s_economyTiming[11];
extern GlueModule s_economyGlue;
const EconomySeam* s_economyNative;
EconomySeam s_economySeam;
EconomySeam s_economyTimed;

unsigned int __stdcall Economy_RegionSize(const EconMapInfo* map)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned mapAt = GlueIn(glue, 0, map, sizeof(EconMapInfo), 1);
    slotArgs[0].i32 = (int)mapAt;
    GlueCall(glue, 0, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

void __stdcall Economy_Init(const EconMapInfo* map, const EconomyWorldView* world, void* region, unsigned int size)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[4];
    slotArgs[0].i64 = 0;
    unsigned mapAt = GlueIn(glue, 1, map, sizeof(EconMapInfo), 1);
    slotArgs[0].i32 = (int)mapAt;
    glue->world = world;
    slotArgs[1].i32 = 0;
    glue->region = (void*)region;
    slotArgs[2].i32 = (int)GlueObject(glue, 1, region);
    slotArgs[3].i32 = (int)size;
    GlueCall(glue, 1, slotArgs, 4);
    GlueScratchRelease(mark);
}

void* __stdcall Economy_Shutdown()
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
    return (void*)GlueResult(glue, 2, (unsigned)slotArgs[0].i32, 1);
}

int __stdcall Economy_Apply(const EconomyEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(sizeof(EconomyEventHeader), sizeof(EconomyLoad)), sizeof(EconomyRequest)), sizeof(EconomySpend)), sizeof(EconomyIncome)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(sizeof(EconomyEventHeader), sizeof(EconomyLoad)), sizeof(EconomyRequest)), sizeof(EconomySpend)), sizeof(EconomyIncome)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 3, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 3, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Economy_Spend(const EconomySpend* spend)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned spendAt = GlueIn(glue, 4, spend, sizeof(EconomySpend), 1);
    slotArgs[0].i32 = (int)spendAt;
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Economy_UpdatePlayerEconomy(int player)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Economy_UpdateMetalExtraction(EconUnit unit)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)unit;
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Economy_UpdateResourceSharing(int player)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 7, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall Economy_RequestGranted(EconUnit unit, unsigned int resources)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)unit;
    slotArgs[1].i32 = (int)resources;
    GlueCall(glue, 8, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

double __stdcall Economy_GetEnergyUse(EconType type)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)type;
    GlueCall(glue, 9, slotArgs, 1);
    GlueScratchRelease(mark);
    return (double)slotArgs[0].f64;
}

void __stdcall Economy_UnitAccount(EconUnit unit, EconAccount* out)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)unit;
    unsigned outAt = GlueIn(glue, 10, out, sizeof(EconAccount), 0);
    slotArgs[1].i32 = (int)outAt;
    GlueCall(glue, 10, slotArgs, 2);
    GlueOut((void*)out, outAt, sizeof(EconAccount));
    GlueScratchRelease(mark);
}

unsigned int __stdcall Economy_TimedRegionSize(const EconMapInfo* map)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_economyNative->RegionSize(map);
    GlueTimed(&s_economyGlue, 0, start);
    return result;
}

void __stdcall Economy_TimedInit(const EconMapInfo* map, const EconomyWorldView* world, void* region, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_economyNative->Init(map, world, region, size);
    GlueTimed(&s_economyGlue, 1, start);
}

void* __stdcall Economy_TimedShutdown()
{
    unsigned long long start = host_time_counter();
    void* result = s_economyNative->Shutdown();
    GlueTimed(&s_economyGlue, 2, start);
    return result;
}

int __stdcall Economy_TimedApply(const EconomyEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_economyNative->Apply(event, size);
    GlueTimed(&s_economyGlue, 3, start);
    return result;
}

int __stdcall Economy_TimedSpend(const EconomySpend* spend)
{
    unsigned long long start = host_time_counter();
    int result = s_economyNative->Spend(spend);
    GlueTimed(&s_economyGlue, 4, start);
    return result;
}

void __stdcall Economy_TimedUpdatePlayerEconomy(int player)
{
    unsigned long long start = host_time_counter();
    s_economyNative->UpdatePlayerEconomy(player);
    GlueTimed(&s_economyGlue, 5, start);
}

void __stdcall Economy_TimedUpdateMetalExtraction(EconUnit unit)
{
    unsigned long long start = host_time_counter();
    s_economyNative->UpdateMetalExtraction(unit);
    GlueTimed(&s_economyGlue, 6, start);
}

void __stdcall Economy_TimedUpdateResourceSharing(int player)
{
    unsigned long long start = host_time_counter();
    s_economyNative->UpdateResourceSharing(player);
    GlueTimed(&s_economyGlue, 7, start);
}

int __stdcall Economy_TimedRequestGranted(EconUnit unit, unsigned int resources)
{
    unsigned long long start = host_time_counter();
    int result = s_economyNative->RequestGranted(unit, resources);
    GlueTimed(&s_economyGlue, 8, start);
    return result;
}

double __stdcall Economy_TimedGetEnergyUse(EconType type)
{
    unsigned long long start = host_time_counter();
    double result = s_economyNative->GetEnergyUse(type);
    GlueTimed(&s_economyGlue, 9, start);
    return result;
}

void __stdcall Economy_TimedUnitAccount(EconUnit unit, EconAccount* out)
{
    unsigned long long start = host_time_counter();
    s_economyNative->UnitAccount(unit, out);
    GlueTimed(&s_economyGlue, 10, start);
}

void EconomyUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.economy == &s_economySeam)
        return;
    s_economyNative = g_seams.economy;
    s_economySeam = *s_economyNative;
    s_economySeam.RegionSize = Economy_RegionSize;
    s_economySeam.Init = Economy_Init;
    s_economySeam.Shutdown = Economy_Shutdown;
    s_economySeam.Apply = Economy_Apply;
    s_economySeam.Spend = Economy_Spend;
    s_economySeam.UpdatePlayerEconomy = Economy_UpdatePlayerEconomy;
    s_economySeam.UpdateMetalExtraction = Economy_UpdateMetalExtraction;
    s_economySeam.UpdateResourceSharing = Economy_UpdateResourceSharing;
    s_economySeam.RequestGranted = Economy_RequestGranted;
    s_economySeam.GetEnergyUse = Economy_GetEnergyUse;
    s_economySeam.UnitAccount = Economy_UnitAccount;
    g_seams.economy = &s_economySeam;
}

void EconomyTimeNative()
{
    s_economyNative = g_seams.economy;
    s_economyTimed = *s_economyNative;
    s_economyTimed.RegionSize = Economy_TimedRegionSize;
    s_economyTimed.Init = Economy_TimedInit;
    s_economyTimed.Shutdown = Economy_TimedShutdown;
    s_economyTimed.Apply = Economy_TimedApply;
    s_economyTimed.Spend = Economy_TimedSpend;
    s_economyTimed.UpdatePlayerEconomy = Economy_TimedUpdatePlayerEconomy;
    s_economyTimed.UpdateMetalExtraction = Economy_TimedUpdateMetalExtraction;
    s_economyTimed.UpdateResourceSharing = Economy_TimedUpdateResourceSharing;
    s_economyTimed.RequestGranted = Economy_TimedRequestGranted;
    s_economyTimed.GetEnergyUse = Economy_TimedGetEnergyUse;
    s_economyTimed.UnitAccount = Economy_TimedUnitAccount;
    g_seams.economy = &s_economyTimed;
}

const GlueModuleDef kEconomyDef = {"economy", SCHED_ECONOMY, kEconomySlots, 11, s_economyFuncs, s_economyTiming, EconomyUseSeam, EconomyTimeNative, 1, 1};
GlueModule s_economyGlue = {&kEconomyDef};

const char* const kFeaturesSlots[] = {
    "UpdateFeatures",
    "GetCellFeature",
    "GetFootprintCentre",
    "Apply",
    "Place",
    "FeatureTypeCount",
    "AnimateFeatureType",
    "ScanFeatureCell",
    "FirstFeatureSpot",
    "UpdateFeatureSpot",
    "FindFeatureAtPos",
};
wasmtime_func_t s_featuresFuncs[11];
GlueTiming s_featuresTiming[11];
extern GlueModule s_featuresGlue;
const FeaturesSeam* s_featuresNative;
FeaturesSeam s_featuresSeam;
FeaturesSeam s_featuresTimed;

void __stdcall Features_UpdateFeatures()
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 0, slotArgs, 0);
    GlueScratchRelease(mark);
}

unsigned short __stdcall Features_GetCellFeature(Cell* cell)
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 1, cell);
    GlueCall(glue, 1, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned short)slotArgs[0].i32;
}

void __stdcall Features_GetFootprintCentre(const Point16* cell, Feature* def, Vec3* out)
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    unsigned cellAt = GlueIn(glue, 2, cell, sizeof(Point16), 1);
    slotArgs[0].i32 = (int)cellAt;
    slotArgs[1].i32 = (int)GlueObject(glue, 2, def);
    unsigned outAt = GlueIn(glue, 2, out, sizeof(Vec3), 0);
    slotArgs[2].i32 = (int)outAt;
    GlueCall(glue, 2, slotArgs, 3);
    GlueOut((void*)out, outAt, sizeof(Vec3));
    GlueScratchRelease(mark);
}

int __stdcall Features_Apply(const FeaturesEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(sizeof(FeaturesEventHeader), sizeof(FeaturesPlace)), sizeof(FeaturesRemove)), sizeof(FeaturesDamage)), sizeof(FeaturesReclaim)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(sizeof(FeaturesEventHeader), sizeof(FeaturesPlace)), sizeof(FeaturesRemove)), sizeof(FeaturesDamage)), sizeof(FeaturesReclaim)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 3, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 3, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Features_Place(const FeaturesPlace* place)
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned placeAt = GlueIn(glue, 4, place, sizeof(FeaturesPlace), 1);
    slotArgs[0].i32 = (int)placeAt;
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Features_FeatureTypeCount()
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Features_AnimateFeatureType(int type)
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)type;
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Features_ScanFeatureCell()
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 7, slotArgs, 0);
    GlueScratchRelease(mark);
}

int __stdcall Features_FirstFeatureSpot()
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 8, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Features_UpdateFeatureSpot(int spot)
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)spot;
    GlueCall(glue, 9, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

unsigned short __stdcall Features_FindFeatureAtPos(Vec3* pos, Point16* cell, Point16* size)
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    unsigned posAt = GlueIn(glue, 10, pos, sizeof(Vec3), 1);
    slotArgs[0].i32 = (int)posAt;
    unsigned cellAt = GlueIn(glue, 10, cell, sizeof(Point16), 0);
    slotArgs[1].i32 = (int)cellAt;
    unsigned sizeAt = GlueIn(glue, 10, size, sizeof(Point16), 0);
    slotArgs[2].i32 = (int)sizeAt;
    GlueCall(glue, 10, slotArgs, 3);
    GlueOut((void*)cell, cellAt, sizeof(Point16));
    GlueOut((void*)size, sizeAt, sizeof(Point16));
    GlueScratchRelease(mark);
    return (unsigned short)slotArgs[0].i32;
}

void __stdcall Features_TimedUpdateFeatures()
{
    unsigned long long start = host_time_counter();
    s_featuresNative->UpdateFeatures();
    GlueTimed(&s_featuresGlue, 0, start);
}

unsigned short __stdcall Features_TimedGetCellFeature(Cell* cell)
{
    unsigned long long start = host_time_counter();
    unsigned short result = s_featuresNative->GetCellFeature(cell);
    GlueTimed(&s_featuresGlue, 1, start);
    return result;
}

void __stdcall Features_TimedGetFootprintCentre(const Point16* cell, Feature* def, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_featuresNative->GetFootprintCentre(cell, def, out);
    GlueTimed(&s_featuresGlue, 2, start);
}

int __stdcall Features_TimedApply(const FeaturesEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->Apply(event, size);
    GlueTimed(&s_featuresGlue, 3, start);
    return result;
}

int __stdcall Features_TimedPlace(const FeaturesPlace* place)
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->Place(place);
    GlueTimed(&s_featuresGlue, 4, start);
    return result;
}

int __stdcall Features_TimedFeatureTypeCount()
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->FeatureTypeCount();
    GlueTimed(&s_featuresGlue, 5, start);
    return result;
}

void __stdcall Features_TimedAnimateFeatureType(int type)
{
    unsigned long long start = host_time_counter();
    s_featuresNative->AnimateFeatureType(type);
    GlueTimed(&s_featuresGlue, 6, start);
}

void __stdcall Features_TimedScanFeatureCell()
{
    unsigned long long start = host_time_counter();
    s_featuresNative->ScanFeatureCell();
    GlueTimed(&s_featuresGlue, 7, start);
}

int __stdcall Features_TimedFirstFeatureSpot()
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->FirstFeatureSpot();
    GlueTimed(&s_featuresGlue, 8, start);
    return result;
}

int __stdcall Features_TimedUpdateFeatureSpot(int spot)
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->UpdateFeatureSpot(spot);
    GlueTimed(&s_featuresGlue, 9, start);
    return result;
}

unsigned short __stdcall Features_TimedFindFeatureAtPos(Vec3* pos, Point16* cell, Point16* size)
{
    unsigned long long start = host_time_counter();
    unsigned short result = s_featuresNative->FindFeatureAtPos(pos, cell, size);
    GlueTimed(&s_featuresGlue, 10, start);
    return result;
}

void FeaturesUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.features == &s_featuresSeam)
        return;
    s_featuresNative = g_seams.features;
    s_featuresSeam = *s_featuresNative;
    s_featuresSeam.UpdateFeatures = Features_UpdateFeatures;
    s_featuresSeam.GetCellFeature = Features_GetCellFeature;
    s_featuresSeam.GetFootprintCentre = Features_GetFootprintCentre;
    s_featuresSeam.Apply = Features_Apply;
    s_featuresSeam.Place = Features_Place;
    s_featuresSeam.FeatureTypeCount = Features_FeatureTypeCount;
    s_featuresSeam.AnimateFeatureType = Features_AnimateFeatureType;
    s_featuresSeam.ScanFeatureCell = Features_ScanFeatureCell;
    s_featuresSeam.FirstFeatureSpot = Features_FirstFeatureSpot;
    s_featuresSeam.UpdateFeatureSpot = Features_UpdateFeatureSpot;
    s_featuresSeam.FindFeatureAtPos = Features_FindFeatureAtPos;
    g_seams.features = &s_featuresSeam;
}

void FeaturesTimeNative()
{
    s_featuresNative = g_seams.features;
    s_featuresTimed = *s_featuresNative;
    s_featuresTimed.UpdateFeatures = Features_TimedUpdateFeatures;
    s_featuresTimed.GetCellFeature = Features_TimedGetCellFeature;
    s_featuresTimed.GetFootprintCentre = Features_TimedGetFootprintCentre;
    s_featuresTimed.Apply = Features_TimedApply;
    s_featuresTimed.Place = Features_TimedPlace;
    s_featuresTimed.FeatureTypeCount = Features_TimedFeatureTypeCount;
    s_featuresTimed.AnimateFeatureType = Features_TimedAnimateFeatureType;
    s_featuresTimed.ScanFeatureCell = Features_TimedScanFeatureCell;
    s_featuresTimed.FirstFeatureSpot = Features_TimedFirstFeatureSpot;
    s_featuresTimed.UpdateFeatureSpot = Features_TimedUpdateFeatureSpot;
    s_featuresTimed.FindFeatureAtPos = Features_TimedFindFeatureAtPos;
    g_seams.features = &s_featuresTimed;
}

const GlueModuleDef kFeaturesDef = {"features", SCHED_FEATURES, kFeaturesSlots, 11, s_featuresFuncs, s_featuresTiming, FeaturesUseSeam, FeaturesTimeNative, 1, 1};
GlueModule s_featuresGlue = {&kFeaturesDef};

const char* const kAiSlots[] = {
    "Apply",
    "DumpPlayerAI",
    "CreatePlayerAI",
    "DestroyPlayerAI",
    "UpdatePlayerAI",
    "RebuildFeatureCells",
    "CreateSquads",
    "FreeSquads",
    "UpdateSquads",
    "CreateSquadManager",
    "FreeSquadManager",
};
wasmtime_func_t s_aiFuncs[11];
GlueTiming s_aiTiming[11];
extern GlueModule s_aiGlue;
const AiSeam* s_aiNative;
AiSeam s_aiSeam;
AiSeam s_aiTimed;

int __stdcall Ai_Apply(const AiEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > sizeof(AiEventHeader) ? (unsigned)sizeof(AiEventHeader) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Ai_DumpPlayerAI(int player, void* file)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    slotArgs[1].i32 = (int)GlueHandleOf((void*)file);
    GlueCall(glue, 1, slotArgs, 2);
    GlueScratchRelease(mark);
}

void __stdcall Ai_CreatePlayerAI(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Ai_DestroyPlayerAI(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Ai_UpdatePlayerAI(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Ai_RebuildFeatureCells(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Ai_CreateSquads(Player* player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 6, player);
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Ai_FreeSquads(Player* player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 7, player);
    GlueCall(glue, 7, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Ai_UpdateSquads(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 8, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Ai_CreateSquadManager(Player* player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 9, player);
    GlueCall(glue, 9, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Ai_FreeSquadManager(Player* player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 10, player);
    GlueCall(glue, 10, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall Ai_TimedApply(const AiEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_aiNative->Apply(event, size);
    GlueTimed(&s_aiGlue, 0, start);
    return result;
}

void __stdcall Ai_TimedDumpPlayerAI(int player, void* file)
{
    unsigned long long start = host_time_counter();
    s_aiNative->DumpPlayerAI(player, file);
    GlueTimed(&s_aiGlue, 1, start);
}

void __stdcall Ai_TimedCreatePlayerAI(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->CreatePlayerAI(player);
    GlueTimed(&s_aiGlue, 2, start);
}

void __stdcall Ai_TimedDestroyPlayerAI(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->DestroyPlayerAI(player);
    GlueTimed(&s_aiGlue, 3, start);
}

void __stdcall Ai_TimedUpdatePlayerAI(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->UpdatePlayerAI(player);
    GlueTimed(&s_aiGlue, 4, start);
}

void __stdcall Ai_TimedRebuildFeatureCells(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->RebuildFeatureCells(player);
    GlueTimed(&s_aiGlue, 5, start);
}

void __stdcall Ai_TimedCreateSquads(Player* player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->CreateSquads(player);
    GlueTimed(&s_aiGlue, 6, start);
}

void __stdcall Ai_TimedFreeSquads(Player* player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->FreeSquads(player);
    GlueTimed(&s_aiGlue, 7, start);
}

void __stdcall Ai_TimedUpdateSquads(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->UpdateSquads(player);
    GlueTimed(&s_aiGlue, 8, start);
}

void __stdcall Ai_TimedCreateSquadManager(Player* player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->CreateSquadManager(player);
    GlueTimed(&s_aiGlue, 9, start);
}

void __stdcall Ai_TimedFreeSquadManager(Player* player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->FreeSquadManager(player);
    GlueTimed(&s_aiGlue, 10, start);
}

void AiUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.ai == &s_aiSeam)
        return;
    s_aiNative = g_seams.ai;
    s_aiSeam = *s_aiNative;
    s_aiSeam.Apply = Ai_Apply;
    s_aiSeam.DumpPlayerAI = Ai_DumpPlayerAI;
    s_aiSeam.CreatePlayerAI = Ai_CreatePlayerAI;
    s_aiSeam.DestroyPlayerAI = Ai_DestroyPlayerAI;
    s_aiSeam.UpdatePlayerAI = Ai_UpdatePlayerAI;
    s_aiSeam.RebuildFeatureCells = Ai_RebuildFeatureCells;
    s_aiSeam.CreateSquads = Ai_CreateSquads;
    s_aiSeam.FreeSquads = Ai_FreeSquads;
    s_aiSeam.UpdateSquads = Ai_UpdateSquads;
    s_aiSeam.CreateSquadManager = Ai_CreateSquadManager;
    s_aiSeam.FreeSquadManager = Ai_FreeSquadManager;
    g_seams.ai = &s_aiSeam;
}

void AiTimeNative()
{
    s_aiNative = g_seams.ai;
    s_aiTimed = *s_aiNative;
    s_aiTimed.Apply = Ai_TimedApply;
    s_aiTimed.DumpPlayerAI = Ai_TimedDumpPlayerAI;
    s_aiTimed.CreatePlayerAI = Ai_TimedCreatePlayerAI;
    s_aiTimed.DestroyPlayerAI = Ai_TimedDestroyPlayerAI;
    s_aiTimed.UpdatePlayerAI = Ai_TimedUpdatePlayerAI;
    s_aiTimed.RebuildFeatureCells = Ai_TimedRebuildFeatureCells;
    s_aiTimed.CreateSquads = Ai_TimedCreateSquads;
    s_aiTimed.FreeSquads = Ai_TimedFreeSquads;
    s_aiTimed.UpdateSquads = Ai_TimedUpdateSquads;
    s_aiTimed.CreateSquadManager = Ai_TimedCreateSquadManager;
    s_aiTimed.FreeSquadManager = Ai_TimedFreeSquadManager;
    g_seams.ai = &s_aiTimed;
}

const GlueModuleDef kAiDef = {"ai", SCHED_AI, kAiSlots, 11, s_aiFuncs, s_aiTiming, AiUseSeam, AiTimeNative, 1, 1};
GlueModule s_aiGlue = {&kAiDef};

const char* const kPathfindingSlots[] = {
    "RegionSize",
    "Init",
    "Shutdown",
    "Submit",
    "Cancel",
    "CellsChanged",
    "Step",
    "Poll",
};
wasmtime_func_t s_pathfindingFuncs[8];
GlueTiming s_pathfindingTiming[8];
extern GlueModule s_pathfindingGlue;
const PathfindingSeam* s_pathfindingNative;
PathfindingSeam s_pathfindingSeam;
PathfindingSeam s_pathfindingTimed;

unsigned int __stdcall Pathfinding_RegionSize(const PathMapInfo* map)
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned mapAt = GlueIn(glue, 0, map, sizeof(PathMapInfo), 1);
    slotArgs[0].i32 = (int)mapAt;
    GlueCall(glue, 0, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

void __stdcall Pathfinding_Init(const PathMapInfo* map, void* region, unsigned int size)
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    unsigned mapAt = GlueIn(glue, 1, map, sizeof(PathMapInfo), 1);
    slotArgs[0].i32 = (int)mapAt;
    glue->region = (void*)region;
    slotArgs[1].i32 = (int)GlueObject(glue, 1, region);
    slotArgs[2].i32 = (int)size;
    GlueCall(glue, 1, slotArgs, 3);
    GlueScratchRelease(mark);
}

void* __stdcall Pathfinding_Shutdown()
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
    return (void*)GlueResult(glue, 2, (unsigned)slotArgs[0].i32, 1);
}

void __stdcall Pathfinding_Submit(const PathRequest* request)
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned requestAt = GlueIn(glue, 3, request, sizeof(PathRequest), 1);
    slotArgs[0].i32 = (int)requestAt;
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Pathfinding_Cancel(PathHandle requester)
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)requester;
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Pathfinding_CellsChanged(int left, int top, int right, int bottom, unsigned int classes, int cause)
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[6];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)left;
    slotArgs[1].i32 = (int)top;
    slotArgs[2].i32 = (int)right;
    slotArgs[3].i32 = (int)bottom;
    slotArgs[4].i32 = (int)classes;
    slotArgs[5].i32 = (int)cause;
    GlueCall(glue, 5, slotArgs, 6);
    GlueScratchRelease(mark);
}

void __stdcall Pathfinding_Step()
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 6, slotArgs, 0);
    GlueScratchRelease(mark);
}

int __stdcall Pathfinding_Poll(PathfindingEventHeader* out, unsigned int size, PathPoint* points, int capacity)
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[4];
    slotArgs[0].i64 = 0;
    unsigned outAt = GlueIn(glue, 7, out, (unsigned)(size) * 1, 0);
    slotArgs[0].i32 = (int)outAt;
    slotArgs[1].i32 = (int)size;
    unsigned pointsAt = GlueIn(glue, 7, points, (unsigned)(capacity) * sizeof(PathPoint), 0);
    slotArgs[2].i32 = (int)pointsAt;
    slotArgs[3].i32 = (int)capacity;
    GlueCall(glue, 7, slotArgs, 4);
    GlueOut((void*)out, outAt, (unsigned)(size) * 1);
    GlueOut((void*)points, pointsAt, (unsigned)(capacity) * sizeof(PathPoint));
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

unsigned int __stdcall Pathfinding_TimedRegionSize(const PathMapInfo* map)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_pathfindingNative->RegionSize(map);
    GlueTimed(&s_pathfindingGlue, 0, start);
    return result;
}

void __stdcall Pathfinding_TimedInit(const PathMapInfo* map, void* region, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->Init(map, region, size);
    GlueTimed(&s_pathfindingGlue, 1, start);
}

void* __stdcall Pathfinding_TimedShutdown()
{
    unsigned long long start = host_time_counter();
    void* result = s_pathfindingNative->Shutdown();
    GlueTimed(&s_pathfindingGlue, 2, start);
    return result;
}

void __stdcall Pathfinding_TimedSubmit(const PathRequest* request)
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->Submit(request);
    GlueTimed(&s_pathfindingGlue, 3, start);
}

void __stdcall Pathfinding_TimedCancel(PathHandle requester)
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->Cancel(requester);
    GlueTimed(&s_pathfindingGlue, 4, start);
}

void __stdcall Pathfinding_TimedCellsChanged(int left, int top, int right, int bottom, unsigned int classes, int cause)
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->CellsChanged(left, top, right, bottom, classes, cause);
    GlueTimed(&s_pathfindingGlue, 5, start);
}

void __stdcall Pathfinding_TimedStep()
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->Step();
    GlueTimed(&s_pathfindingGlue, 6, start);
}

int __stdcall Pathfinding_TimedPoll(PathfindingEventHeader* out, unsigned int size, PathPoint* points, int capacity)
{
    unsigned long long start = host_time_counter();
    int result = s_pathfindingNative->Poll(out, size, points, capacity);
    GlueTimed(&s_pathfindingGlue, 7, start);
    return result;
}

void PathfindingUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.pathfinding == &s_pathfindingSeam)
        return;
    s_pathfindingNative = g_seams.pathfinding;
    s_pathfindingSeam = *s_pathfindingNative;
    s_pathfindingSeam.RegionSize = Pathfinding_RegionSize;
    s_pathfindingSeam.Init = Pathfinding_Init;
    s_pathfindingSeam.Shutdown = Pathfinding_Shutdown;
    s_pathfindingSeam.Submit = Pathfinding_Submit;
    s_pathfindingSeam.Cancel = Pathfinding_Cancel;
    s_pathfindingSeam.CellsChanged = Pathfinding_CellsChanged;
    s_pathfindingSeam.Step = Pathfinding_Step;
    s_pathfindingSeam.Poll = Pathfinding_Poll;
    g_seams.pathfinding = &s_pathfindingSeam;
}

void PathfindingTimeNative()
{
    s_pathfindingNative = g_seams.pathfinding;
    s_pathfindingTimed = *s_pathfindingNative;
    s_pathfindingTimed.RegionSize = Pathfinding_TimedRegionSize;
    s_pathfindingTimed.Init = Pathfinding_TimedInit;
    s_pathfindingTimed.Shutdown = Pathfinding_TimedShutdown;
    s_pathfindingTimed.Submit = Pathfinding_TimedSubmit;
    s_pathfindingTimed.Cancel = Pathfinding_TimedCancel;
    s_pathfindingTimed.CellsChanged = Pathfinding_TimedCellsChanged;
    s_pathfindingTimed.Step = Pathfinding_TimedStep;
    s_pathfindingTimed.Poll = Pathfinding_TimedPoll;
    g_seams.pathfinding = &s_pathfindingTimed;
}

const GlueModuleDef kPathfindingDef = {"pathfinding", SCHED_PATHFINDING, kPathfindingSlots, 8, s_pathfindingFuncs, s_pathfindingTiming, PathfindingUseSeam, PathfindingTimeNative, 1, 1};
GlueModule s_pathfindingGlue = {&kPathfindingDef};

const char* const kWeaponsSlots[] = {
    "Apply",
    "UpdateProjectiles",
    "UpdateExplosions",
    "UpdateUnitWeapons",
    "FindWeaponByName",
    "GetWeaponRange",
    "WeaponCanReachUnit",
    "FindTargetableProjectile",
    "WeaponCanReachPos",
};
wasmtime_func_t s_weaponsFuncs[9];
GlueTiming s_weaponsTiming[9];
extern GlueModule s_weaponsGlue;
const WeaponsSeam* s_weaponsNative;
WeaponsSeam s_weaponsSeam;
WeaponsSeam s_weaponsTimed;

int __stdcall Weapons_Apply(const WeaponsEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > sizeof(WeaponsEventHeader) ? (unsigned)sizeof(WeaponsEventHeader) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Weapons_UpdateProjectiles()
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 1, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall Weapons_UpdateExplosions()
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 2, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall Weapons_UpdateUnitWeapons(Unit* unit)
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 3, unit);
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
}

WeaponDef* __stdcall Weapons_FindWeaponByName(const char* name)
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueText(glue, 4, name);
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
    return (WeaponDef*)GlueResult(glue, 4, (unsigned)slotArgs[0].i32, 1);
}

int __stdcall Weapons_GetWeaponRange(Unit* unit, unsigned int weapon)
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 5, unit);
    slotArgs[1].i32 = (int)weapon;
    GlueCall(glue, 5, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Weapons_WeaponCanReachUnit(Unit* shooter, Unit* target, unsigned char weapon)
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 6, shooter);
    slotArgs[1].i32 = (int)GlueObject(glue, 6, target);
    slotArgs[2].i32 = (int)weapon;
    GlueCall(glue, 6, slotArgs, 3);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

Projectile* __stdcall Weapons_FindTargetableProjectile(Unit* unit, unsigned int weapon)
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 7, unit);
    slotArgs[1].i32 = (int)weapon;
    GlueCall(glue, 7, slotArgs, 2);
    GlueScratchRelease(mark);
    return (Projectile*)GlueResult(glue, 7, (unsigned)slotArgs[0].i32, 1);
}

int __stdcall Weapons_WeaponCanReachPos(Unit* unit, Vec3* from, Vec3* to, int flags)
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[4];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 8, unit);
    unsigned fromAt = GlueIn(glue, 8, from, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)fromAt;
    unsigned toAt = GlueIn(glue, 8, to, sizeof(Vec3), 1);
    slotArgs[2].i32 = (int)toAt;
    slotArgs[3].i32 = (int)flags;
    GlueCall(glue, 8, slotArgs, 4);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Weapons_TimedApply(const WeaponsEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_weaponsNative->Apply(event, size);
    GlueTimed(&s_weaponsGlue, 0, start);
    return result;
}

void __stdcall Weapons_TimedUpdateProjectiles()
{
    unsigned long long start = host_time_counter();
    s_weaponsNative->UpdateProjectiles();
    GlueTimed(&s_weaponsGlue, 1, start);
}

void __stdcall Weapons_TimedUpdateExplosions()
{
    unsigned long long start = host_time_counter();
    s_weaponsNative->UpdateExplosions();
    GlueTimed(&s_weaponsGlue, 2, start);
}

void __stdcall Weapons_TimedUpdateUnitWeapons(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_weaponsNative->UpdateUnitWeapons(unit);
    GlueTimed(&s_weaponsGlue, 3, start);
}

WeaponDef* __stdcall Weapons_TimedFindWeaponByName(const char* name)
{
    unsigned long long start = host_time_counter();
    WeaponDef* result = s_weaponsNative->FindWeaponByName(name);
    GlueTimed(&s_weaponsGlue, 4, start);
    return result;
}

int __stdcall Weapons_TimedGetWeaponRange(Unit* unit, unsigned int weapon)
{
    unsigned long long start = host_time_counter();
    int result = s_weaponsNative->GetWeaponRange(unit, weapon);
    GlueTimed(&s_weaponsGlue, 5, start);
    return result;
}

int __stdcall Weapons_TimedWeaponCanReachUnit(Unit* shooter, Unit* target, unsigned char weapon)
{
    unsigned long long start = host_time_counter();
    int result = s_weaponsNative->WeaponCanReachUnit(shooter, target, weapon);
    GlueTimed(&s_weaponsGlue, 6, start);
    return result;
}

Projectile* __stdcall Weapons_TimedFindTargetableProjectile(Unit* unit, unsigned int weapon)
{
    unsigned long long start = host_time_counter();
    Projectile* result = s_weaponsNative->FindTargetableProjectile(unit, weapon);
    GlueTimed(&s_weaponsGlue, 7, start);
    return result;
}

int __stdcall Weapons_TimedWeaponCanReachPos(Unit* unit, Vec3* from, Vec3* to, int flags)
{
    unsigned long long start = host_time_counter();
    int result = s_weaponsNative->WeaponCanReachPos(unit, from, to, flags);
    GlueTimed(&s_weaponsGlue, 8, start);
    return result;
}

void WeaponsUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.weapons == &s_weaponsSeam)
        return;
    s_weaponsNative = g_seams.weapons;
    s_weaponsSeam = *s_weaponsNative;
    s_weaponsSeam.Apply = Weapons_Apply;
    s_weaponsSeam.UpdateProjectiles = Weapons_UpdateProjectiles;
    s_weaponsSeam.UpdateExplosions = Weapons_UpdateExplosions;
    s_weaponsSeam.UpdateUnitWeapons = Weapons_UpdateUnitWeapons;
    s_weaponsSeam.FindWeaponByName = Weapons_FindWeaponByName;
    s_weaponsSeam.GetWeaponRange = Weapons_GetWeaponRange;
    s_weaponsSeam.WeaponCanReachUnit = Weapons_WeaponCanReachUnit;
    s_weaponsSeam.FindTargetableProjectile = Weapons_FindTargetableProjectile;
    s_weaponsSeam.WeaponCanReachPos = Weapons_WeaponCanReachPos;
    g_seams.weapons = &s_weaponsSeam;
}

void WeaponsTimeNative()
{
    s_weaponsNative = g_seams.weapons;
    s_weaponsTimed = *s_weaponsNative;
    s_weaponsTimed.Apply = Weapons_TimedApply;
    s_weaponsTimed.UpdateProjectiles = Weapons_TimedUpdateProjectiles;
    s_weaponsTimed.UpdateExplosions = Weapons_TimedUpdateExplosions;
    s_weaponsTimed.UpdateUnitWeapons = Weapons_TimedUpdateUnitWeapons;
    s_weaponsTimed.FindWeaponByName = Weapons_TimedFindWeaponByName;
    s_weaponsTimed.GetWeaponRange = Weapons_TimedGetWeaponRange;
    s_weaponsTimed.WeaponCanReachUnit = Weapons_TimedWeaponCanReachUnit;
    s_weaponsTimed.FindTargetableProjectile = Weapons_TimedFindTargetableProjectile;
    s_weaponsTimed.WeaponCanReachPos = Weapons_TimedWeaponCanReachPos;
    g_seams.weapons = &s_weaponsTimed;
}

const GlueModuleDef kWeaponsDef = {"weapons", SCHED_WEAPONS, kWeaponsSlots, 9, s_weaponsFuncs, s_weaponsTiming, WeaponsUseSeam, WeaponsTimeNative, 1, 1};
GlueModule s_weaponsGlue = {&kWeaponsDef};

const char* const kTargetingSlots[] = {
    "Apply",
    "GetWeaponTargetUnit",
    "GetWeaponTargetPos",
    "FindWeaponTarget",
    "FindBestTargetIfFireAtWill",
    "ChooseWeapon",
    "SavedTargetPair",
};
wasmtime_func_t s_targetingFuncs[7];
GlueTiming s_targetingTiming[7];
extern GlueModule s_targetingGlue;
const TargetingSeam* s_targetingNative;
TargetingSeam s_targetingSeam;
TargetingSeam s_targetingTimed;

int __stdcall Targeting_Apply(const TargetingEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_targetingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(sizeof(TargetingEventHeader), sizeof(TargetingSetUnit)), sizeof(TargetingSetPos)), sizeof(TargetingLoad)) ? (unsigned)GlueMax(GlueMax(GlueMax(sizeof(TargetingEventHeader), sizeof(TargetingSetUnit)), sizeof(TargetingSetPos)), sizeof(TargetingLoad)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

Unit* __stdcall Targeting_GetWeaponTargetUnit(Unit* unit, int weapon)
{
    GlueModule* glue = &s_targetingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 1, unit);
    slotArgs[1].i32 = (int)weapon;
    GlueCall(glue, 1, slotArgs, 2);
    GlueScratchRelease(mark);
    return (Unit*)GlueResult(glue, 1, (unsigned)slotArgs[0].i32, 1);
}

int __stdcall Targeting_GetWeaponTargetPos(Unit* unit, Vec3* pos, int weapon)
{
    GlueModule* glue = &s_targetingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 2, unit);
    unsigned posAt = GlueIn(glue, 2, pos, sizeof(Vec3), 0);
    slotArgs[1].i32 = (int)posAt;
    slotArgs[2].i32 = (int)weapon;
    GlueCall(glue, 2, slotArgs, 3);
    GlueOut((void*)pos, posAt, sizeof(Vec3));
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

Unit* __stdcall Targeting_FindWeaponTarget(Unit* unit, unsigned char weapon, int useRange)
{
    GlueModule* glue = &s_targetingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 3, unit);
    slotArgs[1].i32 = (int)weapon;
    slotArgs[2].i32 = (int)useRange;
    GlueCall(glue, 3, slotArgs, 3);
    GlueScratchRelease(mark);
    return (Unit*)GlueResult(glue, 3, (unsigned)slotArgs[0].i32, 1);
}

Unit* __stdcall Targeting_FindBestTargetIfFireAtWill(Unit* unit)
{
    GlueModule* glue = &s_targetingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 4, unit);
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
    return (Unit*)GlueResult(glue, 4, (unsigned)slotArgs[0].i32, 1);
}

unsigned char __stdcall Targeting_ChooseWeapon(Unit* unit)
{
    GlueModule* glue = &s_targetingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 5, unit);
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned char)slotArgs[0].i32;
}

int __stdcall Targeting_SavedTargetPair(Unit* unit, int weapon)
{
    GlueModule* glue = &s_targetingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 6, unit);
    slotArgs[1].i32 = (int)weapon;
    GlueCall(glue, 6, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Targeting_TimedApply(const TargetingEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_targetingNative->Apply(event, size);
    GlueTimed(&s_targetingGlue, 0, start);
    return result;
}

Unit* __stdcall Targeting_TimedGetWeaponTargetUnit(Unit* unit, int weapon)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_targetingNative->GetWeaponTargetUnit(unit, weapon);
    GlueTimed(&s_targetingGlue, 1, start);
    return result;
}

int __stdcall Targeting_TimedGetWeaponTargetPos(Unit* unit, Vec3* pos, int weapon)
{
    unsigned long long start = host_time_counter();
    int result = s_targetingNative->GetWeaponTargetPos(unit, pos, weapon);
    GlueTimed(&s_targetingGlue, 2, start);
    return result;
}

Unit* __stdcall Targeting_TimedFindWeaponTarget(Unit* unit, unsigned char weapon, int useRange)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_targetingNative->FindWeaponTarget(unit, weapon, useRange);
    GlueTimed(&s_targetingGlue, 3, start);
    return result;
}

Unit* __stdcall Targeting_TimedFindBestTargetIfFireAtWill(Unit* unit)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_targetingNative->FindBestTargetIfFireAtWill(unit);
    GlueTimed(&s_targetingGlue, 4, start);
    return result;
}

unsigned char __stdcall Targeting_TimedChooseWeapon(Unit* unit)
{
    unsigned long long start = host_time_counter();
    unsigned char result = s_targetingNative->ChooseWeapon(unit);
    GlueTimed(&s_targetingGlue, 5, start);
    return result;
}

int __stdcall Targeting_TimedSavedTargetPair(Unit* unit, int weapon)
{
    unsigned long long start = host_time_counter();
    int result = s_targetingNative->SavedTargetPair(unit, weapon);
    GlueTimed(&s_targetingGlue, 6, start);
    return result;
}

void TargetingUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.targeting == &s_targetingSeam)
        return;
    s_targetingNative = g_seams.targeting;
    s_targetingSeam = *s_targetingNative;
    s_targetingSeam.Apply = Targeting_Apply;
    s_targetingSeam.GetWeaponTargetUnit = Targeting_GetWeaponTargetUnit;
    s_targetingSeam.GetWeaponTargetPos = Targeting_GetWeaponTargetPos;
    s_targetingSeam.FindWeaponTarget = Targeting_FindWeaponTarget;
    s_targetingSeam.FindBestTargetIfFireAtWill = Targeting_FindBestTargetIfFireAtWill;
    s_targetingSeam.ChooseWeapon = Targeting_ChooseWeapon;
    s_targetingSeam.SavedTargetPair = Targeting_SavedTargetPair;
    g_seams.targeting = &s_targetingSeam;
}

void TargetingTimeNative()
{
    s_targetingNative = g_seams.targeting;
    s_targetingTimed = *s_targetingNative;
    s_targetingTimed.Apply = Targeting_TimedApply;
    s_targetingTimed.GetWeaponTargetUnit = Targeting_TimedGetWeaponTargetUnit;
    s_targetingTimed.GetWeaponTargetPos = Targeting_TimedGetWeaponTargetPos;
    s_targetingTimed.FindWeaponTarget = Targeting_TimedFindWeaponTarget;
    s_targetingTimed.FindBestTargetIfFireAtWill = Targeting_TimedFindBestTargetIfFireAtWill;
    s_targetingTimed.ChooseWeapon = Targeting_TimedChooseWeapon;
    s_targetingTimed.SavedTargetPair = Targeting_TimedSavedTargetPair;
    g_seams.targeting = &s_targetingTimed;
}

const GlueModuleDef kTargetingDef = {"targeting", SCHED_TARGETING, kTargetingSlots, 7, s_targetingFuncs, s_targetingTiming, TargetingUseSeam, TargetingTimeNative, 1, 0};
GlueModule s_targetingGlue = {&kTargetingDef};

const char* const kLosRadarSlots[] = {
    "Apply",
    "GetMapCell",
    "GetMapCellAtPosition",
    "GetOriginCellAtPosition",
    "ClampWorldPosToTerrain",
    "IsUnitVisibleToPlayer",
    "SoundHeardMask",
    "UpdateUnitLineOfSight",
    "RecordRadarUnits",
    "UpdateSensorRadarAndCloak",
    "RecordRadarMapped",
    "ExpireEyeballs",
    "GetVisibleEnemiesInRadius",
    "HasReadyUnitInRange",
    "IsPointVisible",
    "GetSeenUnitsInRadius",
    "CreateUnitIndex",
    "FreeUnitIndex",
    "RefreshUnitIndex",
    "RegionSize",
    "Init",
    "Shutdown",
};
wasmtime_func_t s_losRadarFuncs[22];
GlueTiming s_losRadarTiming[22];
extern GlueModule s_losRadarGlue;
const LosRadarSeam* s_losRadarNative;
LosRadarSeam s_losRadarSeam;
LosRadarSeam s_losRadarTimed;

int __stdcall LosRadar_Apply(const LosRadarEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > sizeof(LosRadarEventHeader) ? (unsigned)sizeof(LosRadarEventHeader) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

Cell* __stdcall LosRadar_GetMapCell(int x, int y)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)x;
    slotArgs[1].i32 = (int)y;
    GlueCall(glue, 1, slotArgs, 2);
    GlueScratchRelease(mark);
    return (Cell*)GlueResult(glue, 1, (unsigned)slotArgs[0].i32, 1);
}

Cell* __stdcall LosRadar_GetMapCellAtPosition(Vec3* pos)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned posAt = GlueIn(glue, 2, pos, sizeof(Vec3), 1);
    slotArgs[0].i32 = (int)posAt;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
    return (Cell*)GlueResult(glue, 2, (unsigned)slotArgs[0].i32, 1);
}

Cell* __stdcall LosRadar_GetOriginCellAtPosition(Vec3* pos)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned posAt = GlueIn(glue, 3, pos, sizeof(Vec3), 1);
    slotArgs[0].i32 = (int)posAt;
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
    return (Cell*)GlueResult(glue, 3, (unsigned)slotArgs[0].i32, 1);
}

void __stdcall LosRadar_ClampWorldPosToTerrain(int x, int y, Vec3* out)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)x;
    slotArgs[1].i32 = (int)y;
    unsigned outAt = GlueIn(glue, 4, out, sizeof(Vec3), 0);
    slotArgs[2].i32 = (int)outAt;
    GlueCall(glue, 4, slotArgs, 3);
    GlueOut((void*)out, outAt, sizeof(Vec3));
    GlueScratchRelease(mark);
}

int __stdcall LosRadar_IsUnitVisibleToPlayer(Player* player, Unit* unit)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 5, player);
    slotArgs[1].i32 = (int)GlueObject(glue, 5, unit);
    GlueCall(glue, 5, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

unsigned int __stdcall LosRadar_SoundHeardMask(const Vec3* at)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned atAt = GlueIn(glue, 6, at, sizeof(Vec3), 1);
    slotArgs[0].i32 = (int)atAt;
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

void __stdcall LosRadar_UpdateUnitLineOfSight(Unit* unit)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 7, unit);
    GlueCall(glue, 7, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall LosRadar_RecordRadarUnits()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 8, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall LosRadar_UpdateSensorRadarAndCloak()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 9, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall LosRadar_RecordRadarMapped()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 10, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall LosRadar_ExpireEyeballs()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 11, slotArgs, 0);
    GlueScratchRelease(mark);
}

int __stdcall LosRadar_GetVisibleEnemiesInRadius(int player, const Vec3* pos, int radius, int flags, Ref<Unit>* out, int capacity)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[6];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    unsigned posAt = GlueIn(glue, 12, pos, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)posAt;
    slotArgs[2].i32 = (int)radius;
    slotArgs[3].i32 = (int)flags;
    unsigned outAt = GlueIn(glue, 12, out, (unsigned)(capacity) * sizeof(Ref<Unit>), 0);
    slotArgs[4].i32 = (int)outAt;
    slotArgs[5].i32 = (int)capacity;
    GlueCall(glue, 12, slotArgs, 6);
    GlueOut((void*)out, outAt, (unsigned)(capacity) * sizeof(Ref<Unit>));
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall LosRadar_HasReadyUnitInRange(int player, const Vec3* pos, int range)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    unsigned posAt = GlueIn(glue, 13, pos, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)posAt;
    slotArgs[2].i32 = (int)range;
    GlueCall(glue, 13, slotArgs, 3);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall LosRadar_IsPointVisible(Player* player, Vec3* pos)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 14, player);
    unsigned posAt = GlueIn(glue, 14, pos, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)posAt;
    GlueCall(glue, 14, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall LosRadar_GetSeenUnitsInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[5];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    unsigned posAt = GlueIn(glue, 15, pos, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)posAt;
    slotArgs[2].i32 = (int)radius;
    unsigned outAt = GlueIn(glue, 15, out, (unsigned)(capacity) * sizeof(Ref<Unit>), 0);
    slotArgs[3].i32 = (int)outAt;
    slotArgs[4].i32 = (int)capacity;
    GlueCall(glue, 15, slotArgs, 5);
    GlueOut((void*)out, outAt, (unsigned)(capacity) * sizeof(Ref<Unit>));
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall LosRadar_CreateUnitIndex(int player)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 16, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall LosRadar_FreeUnitIndex(int player)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 17, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall LosRadar_RefreshUnitIndex(int player)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 18, slotArgs, 1);
    GlueScratchRelease(mark);
}

unsigned int __stdcall LosRadar_RegionSize(const LosRadarMapInfo* map)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned mapAt = GlueIn(glue, 19, map, sizeof(LosRadarMapInfo), 1);
    slotArgs[0].i32 = (int)mapAt;
    GlueCall(glue, 19, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

void __stdcall LosRadar_Init(const LosRadarMapInfo* map, void* region, unsigned int size)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    unsigned mapAt = GlueIn(glue, 20, map, sizeof(LosRadarMapInfo), 1);
    slotArgs[0].i32 = (int)mapAt;
    glue->region = (void*)region;
    slotArgs[1].i32 = (int)GlueObject(glue, 20, region);
    slotArgs[2].i32 = (int)size;
    GlueCall(glue, 20, slotArgs, 3);
    GlueScratchRelease(mark);
}

void* __stdcall LosRadar_Shutdown()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 21, slotArgs, 1);
    GlueScratchRelease(mark);
    return (void*)GlueResult(glue, 21, (unsigned)slotArgs[0].i32, 1);
}

int __stdcall LosRadar_TimedApply(const LosRadarEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->Apply(event, size);
    GlueTimed(&s_losRadarGlue, 0, start);
    return result;
}

Cell* __stdcall LosRadar_TimedGetMapCell(int x, int y)
{
    unsigned long long start = host_time_counter();
    Cell* result = s_losRadarNative->GetMapCell(x, y);
    GlueTimed(&s_losRadarGlue, 1, start);
    return result;
}

Cell* __stdcall LosRadar_TimedGetMapCellAtPosition(Vec3* pos)
{
    unsigned long long start = host_time_counter();
    Cell* result = s_losRadarNative->GetMapCellAtPosition(pos);
    GlueTimed(&s_losRadarGlue, 2, start);
    return result;
}

Cell* __stdcall LosRadar_TimedGetOriginCellAtPosition(Vec3* pos)
{
    unsigned long long start = host_time_counter();
    Cell* result = s_losRadarNative->GetOriginCellAtPosition(pos);
    GlueTimed(&s_losRadarGlue, 3, start);
    return result;
}

void __stdcall LosRadar_TimedClampWorldPosToTerrain(int x, int y, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->ClampWorldPosToTerrain(x, y, out);
    GlueTimed(&s_losRadarGlue, 4, start);
}

int __stdcall LosRadar_TimedIsUnitVisibleToPlayer(Player* player, Unit* unit)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->IsUnitVisibleToPlayer(player, unit);
    GlueTimed(&s_losRadarGlue, 5, start);
    return result;
}

unsigned int __stdcall LosRadar_TimedSoundHeardMask(const Vec3* at)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_losRadarNative->SoundHeardMask(at);
    GlueTimed(&s_losRadarGlue, 6, start);
    return result;
}

void __stdcall LosRadar_TimedUpdateUnitLineOfSight(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->UpdateUnitLineOfSight(unit);
    GlueTimed(&s_losRadarGlue, 7, start);
}

void __stdcall LosRadar_TimedRecordRadarUnits()
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->RecordRadarUnits();
    GlueTimed(&s_losRadarGlue, 8, start);
}

void __stdcall LosRadar_TimedUpdateSensorRadarAndCloak()
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->UpdateSensorRadarAndCloak();
    GlueTimed(&s_losRadarGlue, 9, start);
}

void __stdcall LosRadar_TimedRecordRadarMapped()
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->RecordRadarMapped();
    GlueTimed(&s_losRadarGlue, 10, start);
}

void __stdcall LosRadar_TimedExpireEyeballs()
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->ExpireEyeballs();
    GlueTimed(&s_losRadarGlue, 11, start);
}

int __stdcall LosRadar_TimedGetVisibleEnemiesInRadius(int player, const Vec3* pos, int radius, int flags, Ref<Unit>* out, int capacity)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->GetVisibleEnemiesInRadius(player, pos, radius, flags, out, capacity);
    GlueTimed(&s_losRadarGlue, 12, start);
    return result;
}

int __stdcall LosRadar_TimedHasReadyUnitInRange(int player, const Vec3* pos, int range)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->HasReadyUnitInRange(player, pos, range);
    GlueTimed(&s_losRadarGlue, 13, start);
    return result;
}

int __stdcall LosRadar_TimedIsPointVisible(Player* player, Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->IsPointVisible(player, pos);
    GlueTimed(&s_losRadarGlue, 14, start);
    return result;
}

int __stdcall LosRadar_TimedGetSeenUnitsInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->GetSeenUnitsInRadius(player, pos, radius, out, capacity);
    GlueTimed(&s_losRadarGlue, 15, start);
    return result;
}

void __stdcall LosRadar_TimedCreateUnitIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->CreateUnitIndex(player);
    GlueTimed(&s_losRadarGlue, 16, start);
}

void __stdcall LosRadar_TimedFreeUnitIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->FreeUnitIndex(player);
    GlueTimed(&s_losRadarGlue, 17, start);
}

void __stdcall LosRadar_TimedRefreshUnitIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->RefreshUnitIndex(player);
    GlueTimed(&s_losRadarGlue, 18, start);
}

unsigned int __stdcall LosRadar_TimedRegionSize(const LosRadarMapInfo* map)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_losRadarNative->RegionSize(map);
    GlueTimed(&s_losRadarGlue, 19, start);
    return result;
}

void __stdcall LosRadar_TimedInit(const LosRadarMapInfo* map, void* region, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->Init(map, region, size);
    GlueTimed(&s_losRadarGlue, 20, start);
}

void* __stdcall LosRadar_TimedShutdown()
{
    unsigned long long start = host_time_counter();
    void* result = s_losRadarNative->Shutdown();
    GlueTimed(&s_losRadarGlue, 21, start);
    return result;
}

void LosRadarUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.los_radar == &s_losRadarSeam)
        return;
    s_losRadarNative = g_seams.los_radar;
    s_losRadarSeam = *s_losRadarNative;
    s_losRadarSeam.Apply = LosRadar_Apply;
    s_losRadarSeam.GetMapCell = LosRadar_GetMapCell;
    s_losRadarSeam.GetMapCellAtPosition = LosRadar_GetMapCellAtPosition;
    s_losRadarSeam.GetOriginCellAtPosition = LosRadar_GetOriginCellAtPosition;
    s_losRadarSeam.ClampWorldPosToTerrain = LosRadar_ClampWorldPosToTerrain;
    s_losRadarSeam.IsUnitVisibleToPlayer = LosRadar_IsUnitVisibleToPlayer;
    s_losRadarSeam.SoundHeardMask = LosRadar_SoundHeardMask;
    s_losRadarSeam.UpdateUnitLineOfSight = LosRadar_UpdateUnitLineOfSight;
    s_losRadarSeam.RecordRadarUnits = LosRadar_RecordRadarUnits;
    s_losRadarSeam.UpdateSensorRadarAndCloak = LosRadar_UpdateSensorRadarAndCloak;
    s_losRadarSeam.RecordRadarMapped = LosRadar_RecordRadarMapped;
    s_losRadarSeam.ExpireEyeballs = LosRadar_ExpireEyeballs;
    s_losRadarSeam.GetVisibleEnemiesInRadius = LosRadar_GetVisibleEnemiesInRadius;
    s_losRadarSeam.HasReadyUnitInRange = LosRadar_HasReadyUnitInRange;
    s_losRadarSeam.IsPointVisible = LosRadar_IsPointVisible;
    s_losRadarSeam.GetSeenUnitsInRadius = LosRadar_GetSeenUnitsInRadius;
    s_losRadarSeam.CreateUnitIndex = LosRadar_CreateUnitIndex;
    s_losRadarSeam.FreeUnitIndex = LosRadar_FreeUnitIndex;
    s_losRadarSeam.RefreshUnitIndex = LosRadar_RefreshUnitIndex;
    s_losRadarSeam.RegionSize = LosRadar_RegionSize;
    s_losRadarSeam.Init = LosRadar_Init;
    s_losRadarSeam.Shutdown = LosRadar_Shutdown;
    g_seams.los_radar = &s_losRadarSeam;
}

void LosRadarTimeNative()
{
    s_losRadarNative = g_seams.los_radar;
    s_losRadarTimed = *s_losRadarNative;
    s_losRadarTimed.Apply = LosRadar_TimedApply;
    s_losRadarTimed.GetMapCell = LosRadar_TimedGetMapCell;
    s_losRadarTimed.GetMapCellAtPosition = LosRadar_TimedGetMapCellAtPosition;
    s_losRadarTimed.GetOriginCellAtPosition = LosRadar_TimedGetOriginCellAtPosition;
    s_losRadarTimed.ClampWorldPosToTerrain = LosRadar_TimedClampWorldPosToTerrain;
    s_losRadarTimed.IsUnitVisibleToPlayer = LosRadar_TimedIsUnitVisibleToPlayer;
    s_losRadarTimed.SoundHeardMask = LosRadar_TimedSoundHeardMask;
    s_losRadarTimed.UpdateUnitLineOfSight = LosRadar_TimedUpdateUnitLineOfSight;
    s_losRadarTimed.RecordRadarUnits = LosRadar_TimedRecordRadarUnits;
    s_losRadarTimed.UpdateSensorRadarAndCloak = LosRadar_TimedUpdateSensorRadarAndCloak;
    s_losRadarTimed.RecordRadarMapped = LosRadar_TimedRecordRadarMapped;
    s_losRadarTimed.ExpireEyeballs = LosRadar_TimedExpireEyeballs;
    s_losRadarTimed.GetVisibleEnemiesInRadius = LosRadar_TimedGetVisibleEnemiesInRadius;
    s_losRadarTimed.HasReadyUnitInRange = LosRadar_TimedHasReadyUnitInRange;
    s_losRadarTimed.IsPointVisible = LosRadar_TimedIsPointVisible;
    s_losRadarTimed.GetSeenUnitsInRadius = LosRadar_TimedGetSeenUnitsInRadius;
    s_losRadarTimed.CreateUnitIndex = LosRadar_TimedCreateUnitIndex;
    s_losRadarTimed.FreeUnitIndex = LosRadar_TimedFreeUnitIndex;
    s_losRadarTimed.RefreshUnitIndex = LosRadar_TimedRefreshUnitIndex;
    s_losRadarTimed.RegionSize = LosRadar_TimedRegionSize;
    s_losRadarTimed.Init = LosRadar_TimedInit;
    s_losRadarTimed.Shutdown = LosRadar_TimedShutdown;
    g_seams.los_radar = &s_losRadarTimed;
}

const GlueModuleDef kLosRadarDef = {"los_radar", SCHED_LOS_RADAR, kLosRadarSlots, 22, s_losRadarFuncs, s_losRadarTiming, LosRadarUseSeam, LosRadarTimeNative, 1, 1};
GlueModule s_losRadarGlue = {&kLosRadarDef};

const char* const kOrdersSlots[] = {
    "Apply",
    "RunOrders",
    "RunSecondaryOrders",
    "GetOrderFlags",
    "GetOrderType",
    "MissionTypeFromName",
    "ExpandGroupCommand",
};
wasmtime_func_t s_ordersFuncs[7];
GlueTiming s_ordersTiming[7];
extern GlueModule s_ordersGlue;
const OrdersSeam* s_ordersNative;
OrdersSeam s_ordersSeam;
OrdersSeam s_ordersTimed;

int __stdcall Orders_Apply(const OrdersEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > sizeof(OrdersEventHeader) ? (unsigned)sizeof(OrdersEventHeader) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Orders_RunOrders(Unit* unit)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 1, unit);
    GlueCall(glue, 1, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Orders_RunSecondaryOrders(Unit* unit)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 2, unit);
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall Orders_GetOrderFlags(Unit* unit)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 3, unit);
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Orders_GetOrderType(unsigned char mode, Unit* unit, Unit* target, Vec3* pos)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[4];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)mode;
    slotArgs[1].i32 = (int)GlueObject(glue, 4, unit);
    slotArgs[2].i32 = (int)GlueObject(glue, 4, target);
    unsigned posAt = GlueIn(glue, 4, pos, sizeof(Vec3), 1);
    slotArgs[3].i32 = (int)posAt;
    GlueCall(glue, 4, slotArgs, 4);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Orders_MissionTypeFromName(const char* name)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueText(glue, 5, name);
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Orders_ExpandGroupCommand(const OrdersGroupCommand* command, unsigned int size)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned commandSize = size > sizeof(OrdersGroupCommand) ? (unsigned)sizeof(OrdersGroupCommand) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 6, command, commandSize, 1);
    slotArgs[1].i32 = (int)commandSize;
    GlueCall(glue, 6, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Orders_TimedApply(const OrdersEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_ordersNative->Apply(event, size);
    GlueTimed(&s_ordersGlue, 0, start);
    return result;
}

void __stdcall Orders_TimedRunOrders(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_ordersNative->RunOrders(unit);
    GlueTimed(&s_ordersGlue, 1, start);
}

void __stdcall Orders_TimedRunSecondaryOrders(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_ordersNative->RunSecondaryOrders(unit);
    GlueTimed(&s_ordersGlue, 2, start);
}

int __stdcall Orders_TimedGetOrderFlags(Unit* unit)
{
    unsigned long long start = host_time_counter();
    int result = s_ordersNative->GetOrderFlags(unit);
    GlueTimed(&s_ordersGlue, 3, start);
    return result;
}

int __stdcall Orders_TimedGetOrderType(unsigned char mode, Unit* unit, Unit* target, Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_ordersNative->GetOrderType(mode, unit, target, pos);
    GlueTimed(&s_ordersGlue, 4, start);
    return result;
}

int __stdcall Orders_TimedMissionTypeFromName(const char* name)
{
    unsigned long long start = host_time_counter();
    int result = s_ordersNative->MissionTypeFromName(name);
    GlueTimed(&s_ordersGlue, 5, start);
    return result;
}

int __stdcall Orders_TimedExpandGroupCommand(const OrdersGroupCommand* command, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_ordersNative->ExpandGroupCommand(command, size);
    GlueTimed(&s_ordersGlue, 6, start);
    return result;
}

void OrdersUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.orders == &s_ordersSeam)
        return;
    s_ordersNative = g_seams.orders;
    s_ordersSeam = *s_ordersNative;
    s_ordersSeam.Apply = Orders_Apply;
    s_ordersSeam.RunOrders = Orders_RunOrders;
    s_ordersSeam.RunSecondaryOrders = Orders_RunSecondaryOrders;
    s_ordersSeam.GetOrderFlags = Orders_GetOrderFlags;
    s_ordersSeam.GetOrderType = Orders_GetOrderType;
    s_ordersSeam.MissionTypeFromName = Orders_MissionTypeFromName;
    s_ordersSeam.ExpandGroupCommand = Orders_ExpandGroupCommand;
    g_seams.orders = &s_ordersSeam;
}

void OrdersTimeNative()
{
    s_ordersNative = g_seams.orders;
    s_ordersTimed = *s_ordersNative;
    s_ordersTimed.Apply = Orders_TimedApply;
    s_ordersTimed.RunOrders = Orders_TimedRunOrders;
    s_ordersTimed.RunSecondaryOrders = Orders_TimedRunSecondaryOrders;
    s_ordersTimed.GetOrderFlags = Orders_TimedGetOrderFlags;
    s_ordersTimed.GetOrderType = Orders_TimedGetOrderType;
    s_ordersTimed.MissionTypeFromName = Orders_TimedMissionTypeFromName;
    s_ordersTimed.ExpandGroupCommand = Orders_TimedExpandGroupCommand;
    g_seams.orders = &s_ordersTimed;
}

const GlueModuleDef kOrdersDef = {"orders", SCHED_ORDERS, kOrdersSlots, 7, s_ordersFuncs, s_ordersTiming, OrdersUseSeam, OrdersTimeNative, 1, 1};
GlueModule s_ordersGlue = {&kOrdersDef};

const char* const kUnitsSlots[] = {
    "Apply",
    "UpdateAllUnits",
    "KillUnit",
    "CreateUnit",
    "FindUnitType",
    "FindUnitTypeId",
    "AddBuildProgress",
    "AddRepairProgress",
    "CanLoad",
    "CanReclaim",
    "CanRepair",
    "UnitRefUnlink",
    "UpdateUnitHead",
    "UpdateUnitBody",
    "UpdateUnitTail",
    "GetFactoriesInRadius",
    "CreateFactoryIndex",
    "FreeFactoryIndex",
    "RefreshFactoryIndex",
    "UnitRefInit",
    "UnitRefSetUnit",
    "UnitRefGet",
    "UnitRefSetValue",
};
wasmtime_func_t s_unitsFuncs[23];
GlueTiming s_unitsTiming[23];
extern GlueModule s_unitsGlue;
const UnitsSeam* s_unitsNative;
UnitsSeam s_unitsSeam;
UnitsSeam s_unitsTimed;

int __stdcall Units_Apply(const UnitsEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > sizeof(UnitsEventHeader) ? (unsigned)sizeof(UnitsEventHeader) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Units_UpdateAllUnits()
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 1, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall Units_KillUnit(Unit* unit, int cause)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 2, unit);
    slotArgs[1].i32 = (int)cause;
    GlueCall(glue, 2, slotArgs, 2);
    GlueScratchRelease(mark);
}

Unit* __stdcall Units_CreateUnit(unsigned char player, unsigned short typeId, const Vec3* pos, int finished, int mode, int id)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[6];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    slotArgs[1].i32 = (int)typeId;
    unsigned posAt = GlueIn(glue, 3, pos, sizeof(Vec3), 1);
    slotArgs[2].i32 = (int)posAt;
    slotArgs[3].i32 = (int)finished;
    slotArgs[4].i32 = (int)mode;
    slotArgs[5].i32 = (int)id;
    GlueCall(glue, 3, slotArgs, 6);
    GlueScratchRelease(mark);
    return (Unit*)GlueResult(glue, 3, (unsigned)slotArgs[0].i32, 1);
}

UnitDef* __stdcall Units_FindUnitType(const char* name)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueText(glue, 4, name);
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
    return (UnitDef*)GlueResult(glue, 4, (unsigned)slotArgs[0].i32, 1);
}

unsigned short __stdcall Units_FindUnitTypeId(const char* name)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueText(glue, 5, name);
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned short)slotArgs[0].i32;
}

int __stdcall Units_AddBuildProgress(Unit* builder, Unit* unit, float amount)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 6, builder);
    slotArgs[1].i32 = (int)GlueObject(glue, 6, unit);
    slotArgs[2].f32 = (float)amount;
    GlueCall(glue, 6, slotArgs, 3);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Units_AddRepairProgress(Unit* builder, Unit* unit, float amount)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 7, builder);
    slotArgs[1].i32 = (int)GlueObject(glue, 7, unit);
    slotArgs[2].f32 = (float)amount;
    GlueCall(glue, 7, slotArgs, 3);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Units_CanLoad(Unit* unit, Unit* target)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 8, unit);
    slotArgs[1].i32 = (int)GlueObject(glue, 8, target);
    GlueCall(glue, 8, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Units_CanReclaim(Unit* unit, Unit* target)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 9, unit);
    slotArgs[1].i32 = (int)GlueObject(glue, 9, target);
    GlueCall(glue, 9, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Units_CanRepair(Unit* unit, Unit* target)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 10, unit);
    slotArgs[1].i32 = (int)GlueObject(glue, 10, target);
    GlueCall(glue, 10, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Units_UnitRefUnlink(UnitRef* ref)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 11, ref);
    GlueCall(glue, 11, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Units_UpdateUnitHead(Unit* unit)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 12, unit);
    GlueCall(glue, 12, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall Units_UpdateUnitBody(Unit* unit)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 13, unit);
    GlueCall(glue, 13, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Units_UpdateUnitTail(Unit* unit, int moved)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 14, unit);
    slotArgs[1].i32 = (int)moved;
    GlueCall(glue, 14, slotArgs, 2);
    GlueScratchRelease(mark);
}

int __stdcall Units_GetFactoriesInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[5];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    unsigned posAt = GlueIn(glue, 15, pos, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)posAt;
    slotArgs[2].i32 = (int)radius;
    unsigned outAt = GlueIn(glue, 15, out, (unsigned)(capacity) * sizeof(Ref<Unit>), 0);
    slotArgs[3].i32 = (int)outAt;
    slotArgs[4].i32 = (int)capacity;
    GlueCall(glue, 15, slotArgs, 5);
    GlueOut((void*)out, outAt, (unsigned)(capacity) * sizeof(Ref<Unit>));
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Units_CreateFactoryIndex(int player)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 16, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Units_FreeFactoryIndex(int player)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 17, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Units_RefreshFactoryIndex(int player)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 18, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Units_UnitRefInit(UnitRef* ref, Unit* unit, OrderBase* value)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 19, ref);
    slotArgs[1].i32 = (int)GlueObject(glue, 19, unit);
    slotArgs[2].i32 = (int)GlueObject(glue, 19, value);
    GlueCall(glue, 19, slotArgs, 3);
    GlueScratchRelease(mark);
}

void __stdcall Units_UnitRefSetUnit(UnitRef* ref, Unit* unit)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 20, ref);
    slotArgs[1].i32 = (int)GlueObject(glue, 20, unit);
    GlueCall(glue, 20, slotArgs, 2);
    GlueScratchRelease(mark);
}

Unit* __stdcall Units_UnitRefGet(UnitRef* ref)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 21, ref);
    GlueCall(glue, 21, slotArgs, 1);
    GlueScratchRelease(mark);
    return (Unit*)GlueResult(glue, 21, (unsigned)slotArgs[0].i32, 1);
}

void __stdcall Units_UnitRefSetValue(UnitRef* ref, OrderBase* value)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 22, ref);
    slotArgs[1].i32 = (int)GlueObject(glue, 22, value);
    GlueCall(glue, 22, slotArgs, 2);
    GlueScratchRelease(mark);
}

int __stdcall Units_TimedApply(const UnitsEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->Apply(event, size);
    GlueTimed(&s_unitsGlue, 0, start);
    return result;
}

void __stdcall Units_TimedUpdateAllUnits()
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UpdateAllUnits();
    GlueTimed(&s_unitsGlue, 1, start);
}

void __stdcall Units_TimedKillUnit(Unit* unit, int cause)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->KillUnit(unit, cause);
    GlueTimed(&s_unitsGlue, 2, start);
}

Unit* __stdcall Units_TimedCreateUnit(unsigned char player, unsigned short typeId, const Vec3* pos, int finished, int mode, int id)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_unitsNative->CreateUnit(player, typeId, pos, finished, mode, id);
    GlueTimed(&s_unitsGlue, 3, start);
    return result;
}

UnitDef* __stdcall Units_TimedFindUnitType(const char* name)
{
    unsigned long long start = host_time_counter();
    UnitDef* result = s_unitsNative->FindUnitType(name);
    GlueTimed(&s_unitsGlue, 4, start);
    return result;
}

unsigned short __stdcall Units_TimedFindUnitTypeId(const char* name)
{
    unsigned long long start = host_time_counter();
    unsigned short result = s_unitsNative->FindUnitTypeId(name);
    GlueTimed(&s_unitsGlue, 5, start);
    return result;
}

int __stdcall Units_TimedAddBuildProgress(Unit* builder, Unit* unit, float amount)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->AddBuildProgress(builder, unit, amount);
    GlueTimed(&s_unitsGlue, 6, start);
    return result;
}

int __stdcall Units_TimedAddRepairProgress(Unit* builder, Unit* unit, float amount)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->AddRepairProgress(builder, unit, amount);
    GlueTimed(&s_unitsGlue, 7, start);
    return result;
}

int __stdcall Units_TimedCanLoad(Unit* unit, Unit* target)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->CanLoad(unit, target);
    GlueTimed(&s_unitsGlue, 8, start);
    return result;
}

int __stdcall Units_TimedCanReclaim(Unit* unit, Unit* target)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->CanReclaim(unit, target);
    GlueTimed(&s_unitsGlue, 9, start);
    return result;
}

int __stdcall Units_TimedCanRepair(Unit* unit, Unit* target)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->CanRepair(unit, target);
    GlueTimed(&s_unitsGlue, 10, start);
    return result;
}

void __stdcall Units_TimedUnitRefUnlink(UnitRef* ref)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UnitRefUnlink(ref);
    GlueTimed(&s_unitsGlue, 11, start);
}

void __stdcall Units_TimedUpdateUnitHead(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UpdateUnitHead(unit);
    GlueTimed(&s_unitsGlue, 12, start);
}

int __stdcall Units_TimedUpdateUnitBody(Unit* unit)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->UpdateUnitBody(unit);
    GlueTimed(&s_unitsGlue, 13, start);
    return result;
}

void __stdcall Units_TimedUpdateUnitTail(Unit* unit, int moved)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UpdateUnitTail(unit, moved);
    GlueTimed(&s_unitsGlue, 14, start);
}

int __stdcall Units_TimedGetFactoriesInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->GetFactoriesInRadius(player, pos, radius, out, capacity);
    GlueTimed(&s_unitsGlue, 15, start);
    return result;
}

void __stdcall Units_TimedCreateFactoryIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->CreateFactoryIndex(player);
    GlueTimed(&s_unitsGlue, 16, start);
}

void __stdcall Units_TimedFreeFactoryIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->FreeFactoryIndex(player);
    GlueTimed(&s_unitsGlue, 17, start);
}

void __stdcall Units_TimedRefreshFactoryIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->RefreshFactoryIndex(player);
    GlueTimed(&s_unitsGlue, 18, start);
}

void __stdcall Units_TimedUnitRefInit(UnitRef* ref, Unit* unit, OrderBase* value)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UnitRefInit(ref, unit, value);
    GlueTimed(&s_unitsGlue, 19, start);
}

void __stdcall Units_TimedUnitRefSetUnit(UnitRef* ref, Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UnitRefSetUnit(ref, unit);
    GlueTimed(&s_unitsGlue, 20, start);
}

Unit* __stdcall Units_TimedUnitRefGet(UnitRef* ref)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_unitsNative->UnitRefGet(ref);
    GlueTimed(&s_unitsGlue, 21, start);
    return result;
}

void __stdcall Units_TimedUnitRefSetValue(UnitRef* ref, OrderBase* value)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UnitRefSetValue(ref, value);
    GlueTimed(&s_unitsGlue, 22, start);
}

void UnitsUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.units == &s_unitsSeam)
        return;
    s_unitsNative = g_seams.units;
    s_unitsSeam = *s_unitsNative;
    s_unitsSeam.Apply = Units_Apply;
    s_unitsSeam.UpdateAllUnits = Units_UpdateAllUnits;
    s_unitsSeam.KillUnit = Units_KillUnit;
    s_unitsSeam.CreateUnit = Units_CreateUnit;
    s_unitsSeam.FindUnitType = Units_FindUnitType;
    s_unitsSeam.FindUnitTypeId = Units_FindUnitTypeId;
    s_unitsSeam.AddBuildProgress = Units_AddBuildProgress;
    s_unitsSeam.AddRepairProgress = Units_AddRepairProgress;
    s_unitsSeam.CanLoad = Units_CanLoad;
    s_unitsSeam.CanReclaim = Units_CanReclaim;
    s_unitsSeam.CanRepair = Units_CanRepair;
    s_unitsSeam.UnitRefUnlink = Units_UnitRefUnlink;
    s_unitsSeam.UpdateUnitHead = Units_UpdateUnitHead;
    s_unitsSeam.UpdateUnitBody = Units_UpdateUnitBody;
    s_unitsSeam.UpdateUnitTail = Units_UpdateUnitTail;
    s_unitsSeam.GetFactoriesInRadius = Units_GetFactoriesInRadius;
    s_unitsSeam.CreateFactoryIndex = Units_CreateFactoryIndex;
    s_unitsSeam.FreeFactoryIndex = Units_FreeFactoryIndex;
    s_unitsSeam.RefreshFactoryIndex = Units_RefreshFactoryIndex;
    s_unitsSeam.UnitRefInit = Units_UnitRefInit;
    s_unitsSeam.UnitRefSetUnit = Units_UnitRefSetUnit;
    s_unitsSeam.UnitRefGet = Units_UnitRefGet;
    s_unitsSeam.UnitRefSetValue = Units_UnitRefSetValue;
    g_seams.units = &s_unitsSeam;
}

void UnitsTimeNative()
{
    s_unitsNative = g_seams.units;
    s_unitsTimed = *s_unitsNative;
    s_unitsTimed.Apply = Units_TimedApply;
    s_unitsTimed.UpdateAllUnits = Units_TimedUpdateAllUnits;
    s_unitsTimed.KillUnit = Units_TimedKillUnit;
    s_unitsTimed.CreateUnit = Units_TimedCreateUnit;
    s_unitsTimed.FindUnitType = Units_TimedFindUnitType;
    s_unitsTimed.FindUnitTypeId = Units_TimedFindUnitTypeId;
    s_unitsTimed.AddBuildProgress = Units_TimedAddBuildProgress;
    s_unitsTimed.AddRepairProgress = Units_TimedAddRepairProgress;
    s_unitsTimed.CanLoad = Units_TimedCanLoad;
    s_unitsTimed.CanReclaim = Units_TimedCanReclaim;
    s_unitsTimed.CanRepair = Units_TimedCanRepair;
    s_unitsTimed.UnitRefUnlink = Units_TimedUnitRefUnlink;
    s_unitsTimed.UpdateUnitHead = Units_TimedUpdateUnitHead;
    s_unitsTimed.UpdateUnitBody = Units_TimedUpdateUnitBody;
    s_unitsTimed.UpdateUnitTail = Units_TimedUpdateUnitTail;
    s_unitsTimed.GetFactoriesInRadius = Units_TimedGetFactoriesInRadius;
    s_unitsTimed.CreateFactoryIndex = Units_TimedCreateFactoryIndex;
    s_unitsTimed.FreeFactoryIndex = Units_TimedFreeFactoryIndex;
    s_unitsTimed.RefreshFactoryIndex = Units_TimedRefreshFactoryIndex;
    s_unitsTimed.UnitRefInit = Units_TimedUnitRefInit;
    s_unitsTimed.UnitRefSetUnit = Units_TimedUnitRefSetUnit;
    s_unitsTimed.UnitRefGet = Units_TimedUnitRefGet;
    s_unitsTimed.UnitRefSetValue = Units_TimedUnitRefSetValue;
    g_seams.units = &s_unitsTimed;
}

const GlueModuleDef kUnitsDef = {"units", SCHED_UNITS, kUnitsSlots, 23, s_unitsFuncs, s_unitsTiming, UnitsUseSeam, UnitsTimeNative, 1, 1};
GlueModule s_unitsGlue = {&kUnitsDef};

const char* const kPlayersSlots[] = {
    "Apply",
    "UpdatePlayers",
    "DeactivateConditions",
    "UpdatePlayersOnStream",
    "UpdatePlayerHead",
    "UpdatePlayerBody",
    "UpdatePlayerTail",
    "UpdatePlayersFinish",
    "RegionSize",
    "Init",
    "Shutdown",
};
wasmtime_func_t s_playersFuncs[11];
GlueTiming s_playersTiming[11];
extern GlueModule s_playersGlue;
const PlayersSeam* s_playersNative;
PlayersSeam s_playersSeam;
PlayersSeam s_playersTimed;

int __stdcall Players_Apply(const PlayersEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > sizeof(PlayersEventHeader) ? (unsigned)sizeof(PlayersEventHeader) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Players_UpdatePlayers()
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 1, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall Players_DeactivateConditions(MissionConditions* conditions)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 2, conditions);
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Players_UpdatePlayersOnStream(RandStream stream)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)stream;
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall Players_UpdatePlayerHead(int player)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Players_UpdatePlayerBody(int player, RandStream stream)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    slotArgs[1].i32 = (int)stream;
    GlueCall(glue, 5, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Players_UpdatePlayerTail(int player)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Players_UpdatePlayersFinish()
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 7, slotArgs, 0);
    GlueScratchRelease(mark);
}

unsigned int __stdcall Players_RegionSize(const PlayersMapInfo* info)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned infoAt = GlueIn(glue, 8, info, sizeof(PlayersMapInfo), 1);
    slotArgs[0].i32 = (int)infoAt;
    GlueCall(glue, 8, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

void __stdcall Players_Init(const PlayersMapInfo* info, void* region, unsigned int size)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    unsigned infoAt = GlueIn(glue, 9, info, sizeof(PlayersMapInfo), 1);
    slotArgs[0].i32 = (int)infoAt;
    glue->region = (void*)region;
    slotArgs[1].i32 = (int)GlueObject(glue, 9, region);
    slotArgs[2].i32 = (int)size;
    GlueCall(glue, 9, slotArgs, 3);
    GlueScratchRelease(mark);
}

void* __stdcall Players_Shutdown()
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 10, slotArgs, 1);
    GlueScratchRelease(mark);
    return (void*)GlueResult(glue, 10, (unsigned)slotArgs[0].i32, 1);
}

int __stdcall Players_TimedApply(const PlayersEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_playersNative->Apply(event, size);
    GlueTimed(&s_playersGlue, 0, start);
    return result;
}

void __stdcall Players_TimedUpdatePlayers()
{
    unsigned long long start = host_time_counter();
    s_playersNative->UpdatePlayers();
    GlueTimed(&s_playersGlue, 1, start);
}

void __stdcall Players_TimedDeactivateConditions(MissionConditions* conditions)
{
    unsigned long long start = host_time_counter();
    s_playersNative->DeactivateConditions(conditions);
    GlueTimed(&s_playersGlue, 2, start);
}

void __stdcall Players_TimedUpdatePlayersOnStream(RandStream stream)
{
    unsigned long long start = host_time_counter();
    s_playersNative->UpdatePlayersOnStream(stream);
    GlueTimed(&s_playersGlue, 3, start);
}

int __stdcall Players_TimedUpdatePlayerHead(int player)
{
    unsigned long long start = host_time_counter();
    int result = s_playersNative->UpdatePlayerHead(player);
    GlueTimed(&s_playersGlue, 4, start);
    return result;
}

int __stdcall Players_TimedUpdatePlayerBody(int player, RandStream stream)
{
    unsigned long long start = host_time_counter();
    int result = s_playersNative->UpdatePlayerBody(player, stream);
    GlueTimed(&s_playersGlue, 5, start);
    return result;
}

void __stdcall Players_TimedUpdatePlayerTail(int player)
{
    unsigned long long start = host_time_counter();
    s_playersNative->UpdatePlayerTail(player);
    GlueTimed(&s_playersGlue, 6, start);
}

void __stdcall Players_TimedUpdatePlayersFinish()
{
    unsigned long long start = host_time_counter();
    s_playersNative->UpdatePlayersFinish();
    GlueTimed(&s_playersGlue, 7, start);
}

unsigned int __stdcall Players_TimedRegionSize(const PlayersMapInfo* info)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_playersNative->RegionSize(info);
    GlueTimed(&s_playersGlue, 8, start);
    return result;
}

void __stdcall Players_TimedInit(const PlayersMapInfo* info, void* region, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_playersNative->Init(info, region, size);
    GlueTimed(&s_playersGlue, 9, start);
}

void* __stdcall Players_TimedShutdown()
{
    unsigned long long start = host_time_counter();
    void* result = s_playersNative->Shutdown();
    GlueTimed(&s_playersGlue, 10, start);
    return result;
}

void PlayersUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.players == &s_playersSeam)
        return;
    s_playersNative = g_seams.players;
    s_playersSeam = *s_playersNative;
    s_playersSeam.Apply = Players_Apply;
    s_playersSeam.UpdatePlayers = Players_UpdatePlayers;
    s_playersSeam.DeactivateConditions = Players_DeactivateConditions;
    s_playersSeam.UpdatePlayersOnStream = Players_UpdatePlayersOnStream;
    s_playersSeam.UpdatePlayerHead = Players_UpdatePlayerHead;
    s_playersSeam.UpdatePlayerBody = Players_UpdatePlayerBody;
    s_playersSeam.UpdatePlayerTail = Players_UpdatePlayerTail;
    s_playersSeam.UpdatePlayersFinish = Players_UpdatePlayersFinish;
    s_playersSeam.RegionSize = Players_RegionSize;
    s_playersSeam.Init = Players_Init;
    s_playersSeam.Shutdown = Players_Shutdown;
    g_seams.players = &s_playersSeam;
}

void PlayersTimeNative()
{
    s_playersNative = g_seams.players;
    s_playersTimed = *s_playersNative;
    s_playersTimed.Apply = Players_TimedApply;
    s_playersTimed.UpdatePlayers = Players_TimedUpdatePlayers;
    s_playersTimed.DeactivateConditions = Players_TimedDeactivateConditions;
    s_playersTimed.UpdatePlayersOnStream = Players_TimedUpdatePlayersOnStream;
    s_playersTimed.UpdatePlayerHead = Players_TimedUpdatePlayerHead;
    s_playersTimed.UpdatePlayerBody = Players_TimedUpdatePlayerBody;
    s_playersTimed.UpdatePlayerTail = Players_TimedUpdatePlayerTail;
    s_playersTimed.UpdatePlayersFinish = Players_TimedUpdatePlayersFinish;
    s_playersTimed.RegionSize = Players_TimedRegionSize;
    s_playersTimed.Init = Players_TimedInit;
    s_playersTimed.Shutdown = Players_TimedShutdown;
    g_seams.players = &s_playersTimed;
}

const GlueModuleDef kPlayersDef = {"players", SCHED_PLAYERS, kPlayersSlots, 11, s_playersFuncs, s_playersTiming, PlayersUseSeam, PlayersTimeNative, 1, 1};
GlueModule s_playersGlue = {&kPlayersDef};

const char* const kMapSlots[] = {
    "Apply",
    "UnitsInRange",
    "GetCellMeanHeight",
    "CanBuildAt",
    "CanPlaceUnitFootprint",
    "BuildAllPassMaps",
    "SnapWorldPosToFootprint",
    "CanPlaceFootprintAt",
    "IsPadSlotFree",
    "PickRandomReclaimableResourcesInRadius",
    "GetGroundHeight",
    "PassMapGet",
    "PassMapInBounds",
    "RefreshMovedUnits",
    "RefreshUnitIfStale",
};
wasmtime_func_t s_mapFuncs[15];
GlueTiming s_mapTiming[15];
extern GlueModule s_mapGlue;
const MapSeam* s_mapNative;
MapSeam s_mapSeam;
MapSeam s_mapTimed;

int __stdcall Map_Apply(const MapEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(MapEventHeader), sizeof(MapSetYard)), sizeof(MapPassMaps)), sizeof(MapPathLockStamp)), sizeof(MapSetSeaLevel)), sizeof(MapSetEdge)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(MapEventHeader), sizeof(MapSetYard)), sizeof(MapPassMaps)), sizeof(MapPathLockStamp)), sizeof(MapSetSeaLevel)), sizeof(MapSetEdge)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Map_UnitsInRange(const Vec3* pos, int range, int skip, unsigned short* ids, int capacity)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[5];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 1, pos);
    slotArgs[1].i32 = (int)range;
    slotArgs[2].i32 = (int)skip;
    unsigned idsAt = GlueIn(glue, 1, ids, (unsigned)(capacity) * 1, 0);
    slotArgs[3].i32 = (int)idsAt;
    slotArgs[4].i32 = (int)capacity;
    GlueCall(glue, 1, slotArgs, 5);
    GlueOut((void*)ids, idsAt, (unsigned)(capacity) * 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Map_GetCellMeanHeight(Vec3* pos)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned posAt = GlueIn(glue, 2, pos, sizeof(Vec3), 1);
    slotArgs[0].i32 = (int)posAt;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Map_CanBuildAt(UnitDef* unit, const Point16* cell, short type, Player* player)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[4];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 3, unit);
    unsigned cellAt = GlueIn(glue, 3, cell, sizeof(Point16), 1);
    slotArgs[1].i32 = (int)cellAt;
    slotArgs[2].i32 = (int)type;
    slotArgs[3].i32 = (int)GlueObject(glue, 3, player);
    GlueCall(glue, 3, slotArgs, 4);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Map_CanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, const Point16* cell, int flags)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[4];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 4, unit);
    slotArgs[1].i32 = (int)otherTypeId;
    unsigned cellAt = GlueIn(glue, 4, cell, sizeof(Point16), 1);
    slotArgs[2].i32 = (int)cellAt;
    slotArgs[3].i32 = (int)flags;
    GlueCall(glue, 4, slotArgs, 4);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Map_BuildAllPassMaps(const MapMoveLimits* limits, int count)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned limitsAt = GlueIn(glue, 5, limits, (unsigned)(count) * sizeof(MapMoveLimits), 1);
    slotArgs[0].i32 = (int)limitsAt;
    slotArgs[1].i32 = (int)count;
    GlueCall(glue, 5, slotArgs, 2);
    GlueScratchRelease(mark);
}

void __stdcall Map_SnapWorldPosToFootprint(UnitDef* unit, Vec3* pos)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 6, unit);
    unsigned posAt = GlueIn(glue, 6, pos, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)posAt;
    GlueCall(glue, 6, slotArgs, 2);
    GlueOut((void*)pos, posAt, sizeof(Vec3));
    GlueScratchRelease(mark);
}

int __stdcall Map_CanPlaceFootprintAt(Unit* unit, Vec3* pos)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 7, unit);
    unsigned posAt = GlueIn(glue, 7, pos, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)posAt;
    GlueCall(glue, 7, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Map_IsPadSlotFree(Unit* pad, int id)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 8, pad);
    slotArgs[1].i32 = (int)id;
    GlueCall(glue, 8, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Map_PickRandomReclaimableResourcesInRadius(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[6];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 9, pos);
    slotArgs[1].i32 = (int)range;
    slotArgs[2].i32 = (int)GlueObject(glue, 9, energy);
    slotArgs[3].i32 = (int)GlueObject(glue, 9, energyAmount);
    slotArgs[4].i32 = (int)GlueObject(glue, 9, metal);
    slotArgs[5].i32 = (int)GlueObject(glue, 9, metalAmount);
    GlueCall(glue, 9, slotArgs, 6);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Map_GetGroundHeight(Vec3* pos)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned posAt = GlueIn(glue, 10, pos, sizeof(Vec3), 1);
    slotArgs[0].i32 = (int)posAt;
    GlueCall(glue, 10, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Map_PassMapGet(int moveClass, int x, int y)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)moveClass;
    slotArgs[1].i32 = (int)x;
    slotArgs[2].i32 = (int)y;
    GlueCall(glue, 11, slotArgs, 3);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Map_PassMapInBounds(int moveClass, unsigned int x, unsigned int y)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)moveClass;
    slotArgs[1].i32 = (int)x;
    slotArgs[2].i32 = (int)y;
    GlueCall(glue, 12, slotArgs, 3);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Map_RefreshMovedUnits(int moveClass, Unit* unit)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)moveClass;
    slotArgs[1].i32 = (int)GlueObject(glue, 13, unit);
    GlueCall(glue, 13, slotArgs, 2);
    GlueScratchRelease(mark);
}

void __stdcall Map_RefreshUnitIfStale(int moveClass, Unit* unit)
{
    GlueModule* glue = &s_mapGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)moveClass;
    slotArgs[1].i32 = (int)GlueObject(glue, 14, unit);
    GlueCall(glue, 14, slotArgs, 2);
    GlueScratchRelease(mark);
}

int __stdcall Map_TimedApply(const MapEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->Apply(event, size);
    GlueTimed(&s_mapGlue, 0, start);
    return result;
}

int __stdcall Map_TimedUnitsInRange(const Vec3* pos, int range, int skip, unsigned short* ids, int capacity)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->UnitsInRange(pos, range, skip, ids, capacity);
    GlueTimed(&s_mapGlue, 1, start);
    return result;
}

int __stdcall Map_TimedGetCellMeanHeight(Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->GetCellMeanHeight(pos);
    GlueTimed(&s_mapGlue, 2, start);
    return result;
}

int __stdcall Map_TimedCanBuildAt(UnitDef* unit, const Point16* cell, short type, Player* player)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->CanBuildAt(unit, cell, type, player);
    GlueTimed(&s_mapGlue, 3, start);
    return result;
}

int __stdcall Map_TimedCanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, const Point16* cell, int flags)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->CanPlaceUnitFootprint(unit, otherTypeId, cell, flags);
    GlueTimed(&s_mapGlue, 4, start);
    return result;
}

void __stdcall Map_TimedBuildAllPassMaps(const MapMoveLimits* limits, int count)
{
    unsigned long long start = host_time_counter();
    s_mapNative->BuildAllPassMaps(limits, count);
    GlueTimed(&s_mapGlue, 5, start);
}

void __stdcall Map_TimedSnapWorldPosToFootprint(UnitDef* unit, Vec3* pos)
{
    unsigned long long start = host_time_counter();
    s_mapNative->SnapWorldPosToFootprint(unit, pos);
    GlueTimed(&s_mapGlue, 6, start);
}

int __stdcall Map_TimedCanPlaceFootprintAt(Unit* unit, Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->CanPlaceFootprintAt(unit, pos);
    GlueTimed(&s_mapGlue, 7, start);
    return result;
}

int __stdcall Map_TimedIsPadSlotFree(Unit* pad, int id)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->IsPadSlotFree(pad, id);
    GlueTimed(&s_mapGlue, 8, start);
    return result;
}

int __stdcall Map_TimedPickRandomReclaimableResourcesInRadius(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->PickRandomReclaimableResourcesInRadius(pos, range, energy, energyAmount, metal, metalAmount);
    GlueTimed(&s_mapGlue, 9, start);
    return result;
}

int __stdcall Map_TimedGetGroundHeight(Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->GetGroundHeight(pos);
    GlueTimed(&s_mapGlue, 10, start);
    return result;
}

int __stdcall Map_TimedPassMapGet(int moveClass, int x, int y)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->PassMapGet(moveClass, x, y);
    GlueTimed(&s_mapGlue, 11, start);
    return result;
}

int __stdcall Map_TimedPassMapInBounds(int moveClass, unsigned int x, unsigned int y)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->PassMapInBounds(moveClass, x, y);
    GlueTimed(&s_mapGlue, 12, start);
    return result;
}

void __stdcall Map_TimedRefreshMovedUnits(int moveClass, Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_mapNative->RefreshMovedUnits(moveClass, unit);
    GlueTimed(&s_mapGlue, 13, start);
}

void __stdcall Map_TimedRefreshUnitIfStale(int moveClass, Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_mapNative->RefreshUnitIfStale(moveClass, unit);
    GlueTimed(&s_mapGlue, 14, start);
}

void MapUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.map == &s_mapSeam)
        return;
    s_mapNative = g_seams.map;
    s_mapSeam = *s_mapNative;
    s_mapSeam.Apply = Map_Apply;
    s_mapSeam.UnitsInRange = Map_UnitsInRange;
    s_mapSeam.GetCellMeanHeight = Map_GetCellMeanHeight;
    s_mapSeam.CanBuildAt = Map_CanBuildAt;
    s_mapSeam.CanPlaceUnitFootprint = Map_CanPlaceUnitFootprint;
    s_mapSeam.BuildAllPassMaps = Map_BuildAllPassMaps;
    s_mapSeam.SnapWorldPosToFootprint = Map_SnapWorldPosToFootprint;
    s_mapSeam.CanPlaceFootprintAt = Map_CanPlaceFootprintAt;
    s_mapSeam.IsPadSlotFree = Map_IsPadSlotFree;
    s_mapSeam.PickRandomReclaimableResourcesInRadius = Map_PickRandomReclaimableResourcesInRadius;
    s_mapSeam.GetGroundHeight = Map_GetGroundHeight;
    s_mapSeam.PassMapGet = Map_PassMapGet;
    s_mapSeam.PassMapInBounds = Map_PassMapInBounds;
    s_mapSeam.RefreshMovedUnits = Map_RefreshMovedUnits;
    s_mapSeam.RefreshUnitIfStale = Map_RefreshUnitIfStale;
    g_seams.map = &s_mapSeam;
}

void MapTimeNative()
{
    s_mapNative = g_seams.map;
    s_mapTimed = *s_mapNative;
    s_mapTimed.Apply = Map_TimedApply;
    s_mapTimed.UnitsInRange = Map_TimedUnitsInRange;
    s_mapTimed.GetCellMeanHeight = Map_TimedGetCellMeanHeight;
    s_mapTimed.CanBuildAt = Map_TimedCanBuildAt;
    s_mapTimed.CanPlaceUnitFootprint = Map_TimedCanPlaceUnitFootprint;
    s_mapTimed.BuildAllPassMaps = Map_TimedBuildAllPassMaps;
    s_mapTimed.SnapWorldPosToFootprint = Map_TimedSnapWorldPosToFootprint;
    s_mapTimed.CanPlaceFootprintAt = Map_TimedCanPlaceFootprintAt;
    s_mapTimed.IsPadSlotFree = Map_TimedIsPadSlotFree;
    s_mapTimed.PickRandomReclaimableResourcesInRadius = Map_TimedPickRandomReclaimableResourcesInRadius;
    s_mapTimed.GetGroundHeight = Map_TimedGetGroundHeight;
    s_mapTimed.PassMapGet = Map_TimedPassMapGet;
    s_mapTimed.PassMapInBounds = Map_TimedPassMapInBounds;
    s_mapTimed.RefreshMovedUnits = Map_TimedRefreshMovedUnits;
    s_mapTimed.RefreshUnitIfStale = Map_TimedRefreshUnitIfStale;
    g_seams.map = &s_mapTimed;
}

const GlueModuleDef kMapDef = {"map", SCHED_MAP, kMapSlots, 15, s_mapFuncs, s_mapTiming, MapUseSeam, MapTimeNative, 1, 1};
GlueModule s_mapGlue = {&kMapDef};

const char* const kScriptsSlots[] = {
    "Apply",
    "StartScriptWithArgsByIndex",
    "QueryScript",
    "FindScript",
    "GetPieceOffset",
    "GetPiecePosition",
    "GetPieceAngles",
    "GetNanoPiecePosition",
    "GetAimFromPosition",
    "GetSweetSpot",
    "PoseUnitModel",
    "CreateObjectState",
    "FreeObjectState",
    "UpdateObjectState",
    "ObjectStateSize",
    "NewUnitScript",
    "SetCob",
    "CreatePlayerObjectState",
    "SetObjectState",
    "RunScripts",
    "DeleteScript",
};
wasmtime_func_t s_scriptsFuncs[21];
GlueTiming s_scriptsTiming[21];
extern GlueModule s_scriptsGlue;
const ScriptsSeam* s_scriptsNative;
ScriptsSeam s_scriptsSeam;
ScriptsSeam s_scriptsTimed;

int __stdcall Scripts_Apply(const ScriptsEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > sizeof(ScriptsEventHeader) ? (unsigned)sizeof(ScriptsEventHeader) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Scripts_StartScriptWithArgsByIndex(CobScript* script, int index, Callback* callback, int update, int count, int a, int b, int c, int d)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[9];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 1, script);
    slotArgs[1].i32 = (int)index;
    slotArgs[2].i32 = (int)GlueObject(glue, 1, callback);
    slotArgs[3].i32 = (int)update;
    slotArgs[4].i32 = (int)count;
    slotArgs[5].i32 = (int)a;
    slotArgs[6].i32 = (int)b;
    slotArgs[7].i32 = (int)c;
    slotArgs[8].i32 = (int)d;
    GlueCall(glue, 1, slotArgs, 9);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Scripts_QueryScript(CobScript* script, const char* name, int* arg0, int* arg1, int* arg2, int* arg3)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[6];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 2, script);
    slotArgs[1].i32 = (int)GlueText(glue, 2, name);
    unsigned arg0At = GlueIn(glue, 2, arg0, sizeof(int), 1);
    slotArgs[2].i32 = (int)arg0At;
    unsigned arg1At = GlueIn(glue, 2, arg1, sizeof(int), 1);
    slotArgs[3].i32 = (int)arg1At;
    unsigned arg2At = GlueIn(glue, 2, arg2, sizeof(int), 1);
    slotArgs[4].i32 = (int)arg2At;
    unsigned arg3At = GlueIn(glue, 2, arg3, sizeof(int), 1);
    slotArgs[5].i32 = (int)arg3At;
    GlueCall(glue, 2, slotArgs, 6);
    GlueOut((void*)arg0, arg0At, sizeof(int));
    GlueOut((void*)arg1, arg1At, sizeof(int));
    GlueOut((void*)arg2, arg2At, sizeof(int));
    GlueOut((void*)arg3, arg3At, sizeof(int));
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Scripts_FindScript(CobScript* script, const char* name)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 3, script);
    slotArgs[1].i32 = (int)GlueText(glue, 3, name);
    GlueCall(glue, 3, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Scripts_GetPieceOffset(Unit* unit, int piece, Vec3* out)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 4, unit);
    slotArgs[1].i32 = (int)piece;
    unsigned outAt = GlueIn(glue, 4, out, sizeof(Vec3), 0);
    slotArgs[2].i32 = (int)outAt;
    GlueCall(glue, 4, slotArgs, 3);
    GlueOut((void*)out, outAt, sizeof(Vec3));
    GlueScratchRelease(mark);
}

void __stdcall Scripts_GetPiecePosition(Unit* unit, int piece, Vec3* out)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 5, unit);
    slotArgs[1].i32 = (int)piece;
    unsigned outAt = GlueIn(glue, 5, out, sizeof(Vec3), 0);
    slotArgs[2].i32 = (int)outAt;
    GlueCall(glue, 5, slotArgs, 3);
    GlueOut((void*)out, outAt, sizeof(Vec3));
    GlueScratchRelease(mark);
}

void __stdcall Scripts_GetPieceAngles(Unit* unit, int piece, Angles16* out)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 6, unit);
    slotArgs[1].i32 = (int)piece;
    unsigned outAt = GlueIn(glue, 6, out, sizeof(Angles16), 0);
    slotArgs[2].i32 = (int)outAt;
    GlueCall(glue, 6, slotArgs, 3);
    GlueOut((void*)out, outAt, sizeof(Angles16));
    GlueScratchRelease(mark);
}

void __stdcall Scripts_GetNanoPiecePosition(Unit* unit, Vec3* out)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 7, unit);
    unsigned outAt = GlueIn(glue, 7, out, sizeof(Vec3), 0);
    slotArgs[1].i32 = (int)outAt;
    GlueCall(glue, 7, slotArgs, 2);
    GlueOut((void*)out, outAt, sizeof(Vec3));
    GlueScratchRelease(mark);
}

void __stdcall Scripts_GetAimFromPosition(Unit* unit, Vec3* out, unsigned char weapon)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 8, unit);
    unsigned outAt = GlueIn(glue, 8, out, sizeof(Vec3), 0);
    slotArgs[1].i32 = (int)outAt;
    slotArgs[2].i32 = (int)weapon;
    GlueCall(glue, 8, slotArgs, 3);
    GlueOut((void*)out, outAt, sizeof(Vec3));
    GlueScratchRelease(mark);
}

void __stdcall Scripts_GetSweetSpot(Unit* unit, Vec3* out)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 9, unit);
    unsigned outAt = GlueIn(glue, 9, out, sizeof(Vec3), 0);
    slotArgs[1].i32 = (int)outAt;
    GlueCall(glue, 9, slotArgs, 2);
    GlueOut((void*)out, outAt, sizeof(Vec3));
    GlueScratchRelease(mark);
}

void __stdcall Scripts_PoseUnitModel(Unit* unit)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 10, unit);
    GlueCall(glue, 10, slotArgs, 1);
    GlueScratchRelease(mark);
}

ModelState* __stdcall Scripts_CreateObjectState(Object3do* model)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 11, model);
    GlueCall(glue, 11, slotArgs, 1);
    GlueScratchRelease(mark);
    return (ModelState*)GlueResult(glue, 11, (unsigned)slotArgs[0].i32, 1);
}

void __stdcall Scripts_FreeObjectState(ModelState* state)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 12, state);
    GlueCall(glue, 12, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Scripts_UpdateObjectState(UnitDrawView* unit)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 13, unit);
    GlueCall(glue, 13, slotArgs, 1);
    GlueScratchRelease(mark);
}

unsigned int __stdcall Scripts_ObjectStateSize(Object3do* model)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 14, model);
    GlueCall(glue, 14, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

void __stdcall Scripts_NewUnitScript(Unit* unit)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 15, unit);
    GlueCall(glue, 15, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Scripts_SetCob(CobScript* script, ScriptTable* data)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 16, script);
    slotArgs[1].i32 = (int)GlueObject(glue, 16, data);
    GlueCall(glue, 16, slotArgs, 2);
    GlueScratchRelease(mark);
}

ModelState* __stdcall Scripts_CreatePlayerObjectState(Object3do* model, ScriptTable* data, Unit* owner)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 17, model);
    slotArgs[1].i32 = (int)GlueObject(glue, 17, data);
    slotArgs[2].i32 = (int)GlueObject(glue, 17, owner);
    GlueCall(glue, 17, slotArgs, 3);
    GlueScratchRelease(mark);
    return (ModelState*)GlueResult(glue, 17, (unsigned)slotArgs[0].i32, 1);
}

void __stdcall Scripts_SetObjectState(UnitScript* script, ModelState* state)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 18, script);
    slotArgs[1].i32 = (int)GlueObject(glue, 18, state);
    GlueCall(glue, 18, slotArgs, 2);
    GlueScratchRelease(mark);
}

void __stdcall Scripts_RunScripts(CobScript* script, int elapsed)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 19, script);
    slotArgs[1].i32 = (int)elapsed;
    GlueCall(glue, 19, slotArgs, 2);
    GlueScratchRelease(mark);
}

void __stdcall Scripts_DeleteScript(CobScript* script)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 20, script);
    GlueCall(glue, 20, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall Scripts_TimedApply(const ScriptsEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_scriptsNative->Apply(event, size);
    GlueTimed(&s_scriptsGlue, 0, start);
    return result;
}

int __stdcall Scripts_TimedStartScriptWithArgsByIndex(CobScript* script, int index, Callback* callback, int update, int count, int a, int b, int c, int d)
{
    unsigned long long start = host_time_counter();
    int result = s_scriptsNative->StartScriptWithArgsByIndex(script, index, callback, update, count, a, b, c, d);
    GlueTimed(&s_scriptsGlue, 1, start);
    return result;
}

int __stdcall Scripts_TimedQueryScript(CobScript* script, const char* name, int* arg0, int* arg1, int* arg2, int* arg3)
{
    unsigned long long start = host_time_counter();
    int result = s_scriptsNative->QueryScript(script, name, arg0, arg1, arg2, arg3);
    GlueTimed(&s_scriptsGlue, 2, start);
    return result;
}

int __stdcall Scripts_TimedFindScript(CobScript* script, const char* name)
{
    unsigned long long start = host_time_counter();
    int result = s_scriptsNative->FindScript(script, name);
    GlueTimed(&s_scriptsGlue, 3, start);
    return result;
}

void __stdcall Scripts_TimedGetPieceOffset(Unit* unit, int piece, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetPieceOffset(unit, piece, out);
    GlueTimed(&s_scriptsGlue, 4, start);
}

void __stdcall Scripts_TimedGetPiecePosition(Unit* unit, int piece, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetPiecePosition(unit, piece, out);
    GlueTimed(&s_scriptsGlue, 5, start);
}

void __stdcall Scripts_TimedGetPieceAngles(Unit* unit, int piece, Angles16* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetPieceAngles(unit, piece, out);
    GlueTimed(&s_scriptsGlue, 6, start);
}

void __stdcall Scripts_TimedGetNanoPiecePosition(Unit* unit, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetNanoPiecePosition(unit, out);
    GlueTimed(&s_scriptsGlue, 7, start);
}

void __stdcall Scripts_TimedGetAimFromPosition(Unit* unit, Vec3* out, unsigned char weapon)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetAimFromPosition(unit, out, weapon);
    GlueTimed(&s_scriptsGlue, 8, start);
}

void __stdcall Scripts_TimedGetSweetSpot(Unit* unit, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetSweetSpot(unit, out);
    GlueTimed(&s_scriptsGlue, 9, start);
}

void __stdcall Scripts_TimedPoseUnitModel(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->PoseUnitModel(unit);
    GlueTimed(&s_scriptsGlue, 10, start);
}

ModelState* __stdcall Scripts_TimedCreateObjectState(Object3do* model)
{
    unsigned long long start = host_time_counter();
    ModelState* result = s_scriptsNative->CreateObjectState(model);
    GlueTimed(&s_scriptsGlue, 11, start);
    return result;
}

void __stdcall Scripts_TimedFreeObjectState(ModelState* state)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->FreeObjectState(state);
    GlueTimed(&s_scriptsGlue, 12, start);
}

void __stdcall Scripts_TimedUpdateObjectState(UnitDrawView* unit)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->UpdateObjectState(unit);
    GlueTimed(&s_scriptsGlue, 13, start);
}

unsigned int __stdcall Scripts_TimedObjectStateSize(Object3do* model)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_scriptsNative->ObjectStateSize(model);
    GlueTimed(&s_scriptsGlue, 14, start);
    return result;
}

void __stdcall Scripts_TimedNewUnitScript(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->NewUnitScript(unit);
    GlueTimed(&s_scriptsGlue, 15, start);
}

void __stdcall Scripts_TimedSetCob(CobScript* script, ScriptTable* data)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->SetCob(script, data);
    GlueTimed(&s_scriptsGlue, 16, start);
}

ModelState* __stdcall Scripts_TimedCreatePlayerObjectState(Object3do* model, ScriptTable* data, Unit* owner)
{
    unsigned long long start = host_time_counter();
    ModelState* result = s_scriptsNative->CreatePlayerObjectState(model, data, owner);
    GlueTimed(&s_scriptsGlue, 17, start);
    return result;
}

void __stdcall Scripts_TimedSetObjectState(UnitScript* script, ModelState* state)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->SetObjectState(script, state);
    GlueTimed(&s_scriptsGlue, 18, start);
}

void __stdcall Scripts_TimedRunScripts(CobScript* script, int elapsed)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->RunScripts(script, elapsed);
    GlueTimed(&s_scriptsGlue, 19, start);
}

void __stdcall Scripts_TimedDeleteScript(CobScript* script)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->DeleteScript(script);
    GlueTimed(&s_scriptsGlue, 20, start);
}

void ScriptsUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.scripts == &s_scriptsSeam)
        return;
    s_scriptsNative = g_seams.scripts;
    s_scriptsSeam = *s_scriptsNative;
    s_scriptsSeam.Apply = Scripts_Apply;
    s_scriptsSeam.StartScriptWithArgsByIndex = Scripts_StartScriptWithArgsByIndex;
    s_scriptsSeam.QueryScript = Scripts_QueryScript;
    s_scriptsSeam.FindScript = Scripts_FindScript;
    s_scriptsSeam.GetPieceOffset = Scripts_GetPieceOffset;
    s_scriptsSeam.GetPiecePosition = Scripts_GetPiecePosition;
    s_scriptsSeam.GetPieceAngles = Scripts_GetPieceAngles;
    s_scriptsSeam.GetNanoPiecePosition = Scripts_GetNanoPiecePosition;
    s_scriptsSeam.GetAimFromPosition = Scripts_GetAimFromPosition;
    s_scriptsSeam.GetSweetSpot = Scripts_GetSweetSpot;
    s_scriptsSeam.PoseUnitModel = Scripts_PoseUnitModel;
    s_scriptsSeam.CreateObjectState = Scripts_CreateObjectState;
    s_scriptsSeam.FreeObjectState = Scripts_FreeObjectState;
    s_scriptsSeam.UpdateObjectState = Scripts_UpdateObjectState;
    s_scriptsSeam.ObjectStateSize = Scripts_ObjectStateSize;
    s_scriptsSeam.NewUnitScript = Scripts_NewUnitScript;
    s_scriptsSeam.SetCob = Scripts_SetCob;
    s_scriptsSeam.CreatePlayerObjectState = Scripts_CreatePlayerObjectState;
    s_scriptsSeam.SetObjectState = Scripts_SetObjectState;
    s_scriptsSeam.RunScripts = Scripts_RunScripts;
    s_scriptsSeam.DeleteScript = Scripts_DeleteScript;
    g_seams.scripts = &s_scriptsSeam;
}

void ScriptsTimeNative()
{
    s_scriptsNative = g_seams.scripts;
    s_scriptsTimed = *s_scriptsNative;
    s_scriptsTimed.Apply = Scripts_TimedApply;
    s_scriptsTimed.StartScriptWithArgsByIndex = Scripts_TimedStartScriptWithArgsByIndex;
    s_scriptsTimed.QueryScript = Scripts_TimedQueryScript;
    s_scriptsTimed.FindScript = Scripts_TimedFindScript;
    s_scriptsTimed.GetPieceOffset = Scripts_TimedGetPieceOffset;
    s_scriptsTimed.GetPiecePosition = Scripts_TimedGetPiecePosition;
    s_scriptsTimed.GetPieceAngles = Scripts_TimedGetPieceAngles;
    s_scriptsTimed.GetNanoPiecePosition = Scripts_TimedGetNanoPiecePosition;
    s_scriptsTimed.GetAimFromPosition = Scripts_TimedGetAimFromPosition;
    s_scriptsTimed.GetSweetSpot = Scripts_TimedGetSweetSpot;
    s_scriptsTimed.PoseUnitModel = Scripts_TimedPoseUnitModel;
    s_scriptsTimed.CreateObjectState = Scripts_TimedCreateObjectState;
    s_scriptsTimed.FreeObjectState = Scripts_TimedFreeObjectState;
    s_scriptsTimed.UpdateObjectState = Scripts_TimedUpdateObjectState;
    s_scriptsTimed.ObjectStateSize = Scripts_TimedObjectStateSize;
    s_scriptsTimed.NewUnitScript = Scripts_TimedNewUnitScript;
    s_scriptsTimed.SetCob = Scripts_TimedSetCob;
    s_scriptsTimed.CreatePlayerObjectState = Scripts_TimedCreatePlayerObjectState;
    s_scriptsTimed.SetObjectState = Scripts_TimedSetObjectState;
    s_scriptsTimed.RunScripts = Scripts_TimedRunScripts;
    s_scriptsTimed.DeleteScript = Scripts_TimedDeleteScript;
    g_seams.scripts = &s_scriptsTimed;
}

const GlueModuleDef kScriptsDef = {"scripts", SCHED_SCRIPTS, kScriptsSlots, 21, s_scriptsFuncs, s_scriptsTiming, ScriptsUseSeam, ScriptsTimeNative, 1, 1};
GlueModule s_scriptsGlue = {&kScriptsDef};

const char* const kMovementSlots[] = {
    "Apply",
    "UpdateMotion",
    "ApplyPathEvent",
    "HasNetState",
    "NewPathOrderAt",
    "NewPathOrderToUnit",
    "NewPathOrderToUnitAt",
    "NewPathOrderToUnitAtAngle",
    "NewAirManeuverOrder",
    "NewApproachRadius",
    "NewRingApproach",
    "NewPointMarker",
    "TargetType",
    "UnitTarget",
};
wasmtime_func_t s_movementFuncs[14];
GlueTiming s_movementTiming[14];
extern GlueModule s_movementGlue;
const MovementSeam* s_movementNative;
MovementSeam s_movementSeam;
MovementSeam s_movementTimed;

int __stdcall Movement_Apply(const MovementEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > sizeof(MovementEventHeader) ? (unsigned)sizeof(MovementEventHeader) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Movement_UpdateMotion(Unit* unit)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 1, unit);
    GlueCall(glue, 1, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall Movement_ApplyPathEvent(const PathfindingEventHeader* event, unsigned int size, const PathPoint* waypoints)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 2, event);
    slotArgs[1].i32 = (int)size;
    slotArgs[2].i32 = (int)GlueObject(glue, 2, waypoints);
    GlueCall(glue, 2, slotArgs, 3);
    GlueScratchRelease(mark);
}

int __stdcall Movement_HasNetState(Unit* unit)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 3, unit);
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

unsigned int __stdcall Movement_NewPathOrderAt(unsigned int order, const Vec3* pos)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)order;
    unsigned posAt = GlueIn(glue, 4, pos, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)posAt;
    GlueCall(glue, 4, slotArgs, 2);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

unsigned int __stdcall Movement_NewPathOrderToUnit(unsigned int order, Unit* unit)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)order;
    slotArgs[1].i32 = (int)GlueObject(glue, 5, unit);
    GlueCall(glue, 5, slotArgs, 2);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

unsigned int __stdcall Movement_NewPathOrderToUnitAt(unsigned int order, Unit* unit, const Vec3* pos)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)order;
    slotArgs[1].i32 = (int)GlueObject(glue, 6, unit);
    unsigned posAt = GlueIn(glue, 6, pos, sizeof(Vec3), 1);
    slotArgs[2].i32 = (int)posAt;
    GlueCall(glue, 6, slotArgs, 3);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

unsigned int __stdcall Movement_NewPathOrderToUnitAtAngle(unsigned int order, Unit* unit, short angle)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)order;
    slotArgs[1].i32 = (int)GlueObject(glue, 7, unit);
    slotArgs[2].i32 = (int)angle;
    GlueCall(glue, 7, slotArgs, 3);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

unsigned int __stdcall Movement_NewAirManeuverOrder(unsigned int order, const Vec3* from, const Vec3* to)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)order;
    unsigned fromAt = GlueIn(glue, 8, from, sizeof(Vec3), 1);
    slotArgs[1].i32 = (int)fromAt;
    unsigned toAt = GlueIn(glue, 8, to, sizeof(Vec3), 1);
    slotArgs[2].i32 = (int)toAt;
    GlueCall(glue, 8, slotArgs, 3);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

unsigned int __stdcall Movement_NewApproachRadius(unsigned int order, int x, int y, int radius)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[4];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)order;
    slotArgs[1].i32 = (int)x;
    slotArgs[2].i32 = (int)y;
    slotArgs[3].i32 = (int)radius;
    GlueCall(glue, 9, slotArgs, 4);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

unsigned int __stdcall Movement_NewRingApproach(unsigned int order, int x, int y, int inner, int outer)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[5];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)order;
    slotArgs[1].i32 = (int)x;
    slotArgs[2].i32 = (int)y;
    slotArgs[3].i32 = (int)inner;
    slotArgs[4].i32 = (int)outer;
    GlueCall(glue, 10, slotArgs, 5);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

unsigned int __stdcall Movement_NewPointMarker(unsigned int order, const Point16* pos, const Point16* size)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[3];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)order;
    unsigned posAt = GlueIn(glue, 11, pos, sizeof(Point16), 1);
    slotArgs[1].i32 = (int)posAt;
    unsigned sizeAt = GlueIn(glue, 11, size, sizeof(Point16), 1);
    slotArgs[2].i32 = (int)sizeAt;
    GlueCall(glue, 11, slotArgs, 3);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

int __stdcall Movement_TargetType(unsigned int target)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)target;
    GlueCall(glue, 12, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

unsigned int __stdcall Movement_UnitTarget(Unit* unit)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 13, unit);
    GlueCall(glue, 13, slotArgs, 1);
    GlueScratchRelease(mark);
    return (unsigned int)slotArgs[0].i32;
}

int __stdcall Movement_TimedApply(const MovementEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_movementNative->Apply(event, size);
    GlueTimed(&s_movementGlue, 0, start);
    return result;
}

void __stdcall Movement_TimedUpdateMotion(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_movementNative->UpdateMotion(unit);
    GlueTimed(&s_movementGlue, 1, start);
}

void __stdcall Movement_TimedApplyPathEvent(const PathfindingEventHeader* event, unsigned int size, const PathPoint* waypoints)
{
    unsigned long long start = host_time_counter();
    s_movementNative->ApplyPathEvent(event, size, waypoints);
    GlueTimed(&s_movementGlue, 2, start);
}

int __stdcall Movement_TimedHasNetState(Unit* unit)
{
    unsigned long long start = host_time_counter();
    int result = s_movementNative->HasNetState(unit);
    GlueTimed(&s_movementGlue, 3, start);
    return result;
}

unsigned int __stdcall Movement_TimedNewPathOrderAt(unsigned int order, const Vec3* pos)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPathOrderAt(order, pos);
    GlueTimed(&s_movementGlue, 4, start);
    return result;
}

unsigned int __stdcall Movement_TimedNewPathOrderToUnit(unsigned int order, Unit* unit)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPathOrderToUnit(order, unit);
    GlueTimed(&s_movementGlue, 5, start);
    return result;
}

unsigned int __stdcall Movement_TimedNewPathOrderToUnitAt(unsigned int order, Unit* unit, const Vec3* pos)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPathOrderToUnitAt(order, unit, pos);
    GlueTimed(&s_movementGlue, 6, start);
    return result;
}

unsigned int __stdcall Movement_TimedNewPathOrderToUnitAtAngle(unsigned int order, Unit* unit, short angle)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPathOrderToUnitAtAngle(order, unit, angle);
    GlueTimed(&s_movementGlue, 7, start);
    return result;
}

unsigned int __stdcall Movement_TimedNewAirManeuverOrder(unsigned int order, const Vec3* from, const Vec3* to)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewAirManeuverOrder(order, from, to);
    GlueTimed(&s_movementGlue, 8, start);
    return result;
}

unsigned int __stdcall Movement_TimedNewApproachRadius(unsigned int order, int x, int y, int radius)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewApproachRadius(order, x, y, radius);
    GlueTimed(&s_movementGlue, 9, start);
    return result;
}

unsigned int __stdcall Movement_TimedNewRingApproach(unsigned int order, int x, int y, int inner, int outer)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewRingApproach(order, x, y, inner, outer);
    GlueTimed(&s_movementGlue, 10, start);
    return result;
}

unsigned int __stdcall Movement_TimedNewPointMarker(unsigned int order, const Point16* pos, const Point16* size)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPointMarker(order, pos, size);
    GlueTimed(&s_movementGlue, 11, start);
    return result;
}

int __stdcall Movement_TimedTargetType(unsigned int target)
{
    unsigned long long start = host_time_counter();
    int result = s_movementNative->TargetType(target);
    GlueTimed(&s_movementGlue, 12, start);
    return result;
}

unsigned int __stdcall Movement_TimedUnitTarget(Unit* unit)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->UnitTarget(unit);
    GlueTimed(&s_movementGlue, 13, start);
    return result;
}

void MovementUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.movement == &s_movementSeam)
        return;
    s_movementNative = g_seams.movement;
    s_movementSeam = *s_movementNative;
    s_movementSeam.Apply = Movement_Apply;
    s_movementSeam.UpdateMotion = Movement_UpdateMotion;
    s_movementSeam.ApplyPathEvent = Movement_ApplyPathEvent;
    s_movementSeam.HasNetState = Movement_HasNetState;
    s_movementSeam.NewPathOrderAt = Movement_NewPathOrderAt;
    s_movementSeam.NewPathOrderToUnit = Movement_NewPathOrderToUnit;
    s_movementSeam.NewPathOrderToUnitAt = Movement_NewPathOrderToUnitAt;
    s_movementSeam.NewPathOrderToUnitAtAngle = Movement_NewPathOrderToUnitAtAngle;
    s_movementSeam.NewAirManeuverOrder = Movement_NewAirManeuverOrder;
    s_movementSeam.NewApproachRadius = Movement_NewApproachRadius;
    s_movementSeam.NewRingApproach = Movement_NewRingApproach;
    s_movementSeam.NewPointMarker = Movement_NewPointMarker;
    s_movementSeam.TargetType = Movement_TargetType;
    s_movementSeam.UnitTarget = Movement_UnitTarget;
    g_seams.movement = &s_movementSeam;
}

void MovementTimeNative()
{
    s_movementNative = g_seams.movement;
    s_movementTimed = *s_movementNative;
    s_movementTimed.Apply = Movement_TimedApply;
    s_movementTimed.UpdateMotion = Movement_TimedUpdateMotion;
    s_movementTimed.ApplyPathEvent = Movement_TimedApplyPathEvent;
    s_movementTimed.HasNetState = Movement_TimedHasNetState;
    s_movementTimed.NewPathOrderAt = Movement_TimedNewPathOrderAt;
    s_movementTimed.NewPathOrderToUnit = Movement_TimedNewPathOrderToUnit;
    s_movementTimed.NewPathOrderToUnitAt = Movement_TimedNewPathOrderToUnitAt;
    s_movementTimed.NewPathOrderToUnitAtAngle = Movement_TimedNewPathOrderToUnitAtAngle;
    s_movementTimed.NewAirManeuverOrder = Movement_TimedNewAirManeuverOrder;
    s_movementTimed.NewApproachRadius = Movement_TimedNewApproachRadius;
    s_movementTimed.NewRingApproach = Movement_TimedNewRingApproach;
    s_movementTimed.NewPointMarker = Movement_TimedNewPointMarker;
    s_movementTimed.TargetType = Movement_TimedTargetType;
    s_movementTimed.UnitTarget = Movement_TimedUnitTarget;
    g_seams.movement = &s_movementTimed;
}

const GlueModuleDef kMovementDef = {"movement", SCHED_MOVEMENT, kMovementSlots, 14, s_movementFuncs, s_movementTiming, MovementUseSeam, MovementTimeNative, 1, 1};
GlueModule s_movementGlue = {&kMovementDef};

const char* const kMissionSlots[] = {
    "GetGameType",
    "GetNameSlot",
    "GetMissionIndex",
    "GetMissionName",
    "GetStartPosition",
    "CountStartPositions",
    "HasMissionName",
    "GetMissionParams",
    "CountMissionUnits",
    "GetMissionUnit",
    "CountMissionFeatures",
    "GetMissionFeature",
};
wasmtime_func_t s_missionFuncs[12];
GlueTiming s_missionTiming[12];
extern GlueModule s_missionGlue;
const MissionSeam* s_missionNative;
MissionSeam s_missionSeam;
MissionSeam s_missionTimed;

int __stdcall Mission_GetGameType()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 0, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

const char* __stdcall Mission_GetNameSlot(int index)
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)index;
    GlueCall(glue, 1, slotArgs, 1);
    GlueScratchRelease(mark);
    return (const char*)GlueResult(glue, 1, (unsigned)slotArgs[0].i32, 0);
}

int __stdcall Mission_GetMissionIndex()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

const char* __stdcall Mission_GetMissionName()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
    return (const char*)GlueResult(glue, 3, (unsigned)slotArgs[0].i32, 0);
}

int __stdcall Mission_GetStartPosition(Vec3* out, int id)
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned outAt = GlueIn(glue, 4, out, sizeof(Vec3), 0);
    slotArgs[0].i32 = (int)outAt;
    slotArgs[1].i32 = (int)id;
    GlueCall(glue, 4, slotArgs, 2);
    GlueOut((void*)out, outAt, sizeof(Vec3));
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Mission_CountStartPositions()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Mission_HasMissionName()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall Mission_GetMissionParams(MissionParams* out)
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    unsigned outAt = GlueIn(glue, 7, out, sizeof(MissionParams), 0);
    slotArgs[0].i32 = (int)outAt;
    GlueCall(glue, 7, slotArgs, 1);
    GlueOut((void*)out, outAt, sizeof(MissionParams));
    GlueScratchRelease(mark);
}

int __stdcall Mission_CountMissionUnits()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 8, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Mission_GetMissionUnit(int index, MissionUnitInfo* out)
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)index;
    unsigned outAt = GlueIn(glue, 9, out, sizeof(MissionUnitInfo), 0);
    slotArgs[1].i32 = (int)outAt;
    GlueCall(glue, 9, slotArgs, 2);
    GlueOut((void*)out, outAt, sizeof(MissionUnitInfo));
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Mission_CountMissionFeatures()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 10, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Mission_GetMissionFeature(int index, MissionFeatureInfo* out)
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)index;
    unsigned outAt = GlueIn(glue, 11, out, sizeof(MissionFeatureInfo), 0);
    slotArgs[1].i32 = (int)outAt;
    GlueCall(glue, 11, slotArgs, 2);
    GlueOut((void*)out, outAt, sizeof(MissionFeatureInfo));
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall Mission_TimedGetGameType()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->GetGameType();
    GlueTimed(&s_missionGlue, 0, start);
    return result;
}

const char* __stdcall Mission_TimedGetNameSlot(int index)
{
    unsigned long long start = host_time_counter();
    const char* result = s_missionNative->GetNameSlot(index);
    GlueTimed(&s_missionGlue, 1, start);
    return result;
}

int __stdcall Mission_TimedGetMissionIndex()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->GetMissionIndex();
    GlueTimed(&s_missionGlue, 2, start);
    return result;
}

const char* __stdcall Mission_TimedGetMissionName()
{
    unsigned long long start = host_time_counter();
    const char* result = s_missionNative->GetMissionName();
    GlueTimed(&s_missionGlue, 3, start);
    return result;
}

int __stdcall Mission_TimedGetStartPosition(Vec3* out, int id)
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->GetStartPosition(out, id);
    GlueTimed(&s_missionGlue, 4, start);
    return result;
}

int __stdcall Mission_TimedCountStartPositions()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->CountStartPositions();
    GlueTimed(&s_missionGlue, 5, start);
    return result;
}

int __stdcall Mission_TimedHasMissionName()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->HasMissionName();
    GlueTimed(&s_missionGlue, 6, start);
    return result;
}

void __stdcall Mission_TimedGetMissionParams(MissionParams* out)
{
    unsigned long long start = host_time_counter();
    s_missionNative->GetMissionParams(out);
    GlueTimed(&s_missionGlue, 7, start);
}

int __stdcall Mission_TimedCountMissionUnits()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->CountMissionUnits();
    GlueTimed(&s_missionGlue, 8, start);
    return result;
}

int __stdcall Mission_TimedGetMissionUnit(int index, MissionUnitInfo* out)
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->GetMissionUnit(index, out);
    GlueTimed(&s_missionGlue, 9, start);
    return result;
}

int __stdcall Mission_TimedCountMissionFeatures()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->CountMissionFeatures();
    GlueTimed(&s_missionGlue, 10, start);
    return result;
}

int __stdcall Mission_TimedGetMissionFeature(int index, MissionFeatureInfo* out)
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->GetMissionFeature(index, out);
    GlueTimed(&s_missionGlue, 11, start);
    return result;
}

void MissionUseSeam(GlueModule* glue)
{
    (void)glue;
    if (g_seams.mission == &s_missionSeam)
        return;
    s_missionNative = g_seams.mission;
    s_missionSeam = *s_missionNative;
    s_missionSeam.GetGameType = Mission_GetGameType;
    s_missionSeam.GetNameSlot = Mission_GetNameSlot;
    s_missionSeam.GetMissionIndex = Mission_GetMissionIndex;
    s_missionSeam.GetMissionName = Mission_GetMissionName;
    s_missionSeam.GetStartPosition = Mission_GetStartPosition;
    s_missionSeam.CountStartPositions = Mission_CountStartPositions;
    s_missionSeam.HasMissionName = Mission_HasMissionName;
    s_missionSeam.GetMissionParams = Mission_GetMissionParams;
    s_missionSeam.CountMissionUnits = Mission_CountMissionUnits;
    s_missionSeam.GetMissionUnit = Mission_GetMissionUnit;
    s_missionSeam.CountMissionFeatures = Mission_CountMissionFeatures;
    s_missionSeam.GetMissionFeature = Mission_GetMissionFeature;
    g_seams.mission = &s_missionSeam;
}

void MissionTimeNative()
{
    s_missionNative = g_seams.mission;
    s_missionTimed = *s_missionNative;
    s_missionTimed.GetGameType = Mission_TimedGetGameType;
    s_missionTimed.GetNameSlot = Mission_TimedGetNameSlot;
    s_missionTimed.GetMissionIndex = Mission_TimedGetMissionIndex;
    s_missionTimed.GetMissionName = Mission_TimedGetMissionName;
    s_missionTimed.GetStartPosition = Mission_TimedGetStartPosition;
    s_missionTimed.CountStartPositions = Mission_TimedCountStartPositions;
    s_missionTimed.HasMissionName = Mission_TimedHasMissionName;
    s_missionTimed.GetMissionParams = Mission_TimedGetMissionParams;
    s_missionTimed.CountMissionUnits = Mission_TimedCountMissionUnits;
    s_missionTimed.GetMissionUnit = Mission_TimedGetMissionUnit;
    s_missionTimed.CountMissionFeatures = Mission_TimedCountMissionFeatures;
    s_missionTimed.GetMissionFeature = Mission_TimedGetMissionFeature;
    g_seams.mission = &s_missionTimed;
}

const GlueModuleDef kMissionDef = {"mission", -1, kMissionSlots, 12, s_missionFuncs, s_missionTiming, MissionUseSeam, MissionTimeNative, 1, 1};
GlueModule s_missionGlue = {&kMissionDef};

const char* __cdecl Import_economy_GetEnergyUse(wasmtime_val_raw_t* a)
{
    EconType type = (EconType)a[0].i32;
    double result = g_seams.economy->GetEnergyUse(type);
    a[0].f64 = (double)result;
    return nullptr;
}

const char* __cdecl Import_economy_RequestGranted(wasmtime_val_raw_t* a)
{
    EconUnit unit = (EconUnit)a[0].i32;
    unsigned int resources = (unsigned int)a[1].i32;
    int result = ScheduleEconomyRequestGranted(unit, resources);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_economy_Spend(wasmtime_val_raw_t* a)
{
    GLUE_NEED("economy.Spend", 0);
    const EconomySpend* spend = (const EconomySpend*)GlueLinear((unsigned)a[0].i32);
    int result = ScheduleEconomySpend(spend);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_economy_UpdateMetalExtraction(wasmtime_val_raw_t* a)
{
    EconUnit unit = (EconUnit)a[0].i32;
    ScheduleMetalExtraction(unit);
    return nullptr;
}

const char* __cdecl Import_econworld_Allied(wasmtime_val_raw_t* a)
{
    int from = (int)a[0].i32;
    int to = (int)a[1].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Allied", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->Allied(from, to);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_CellMetal(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int y = (int)a[1].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.CellMetal", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->CellMetal(x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_Difficulty(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Difficulty", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->Difficulty();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_Emit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("econworld.Emit", 0);
    const EconomyEventHeader* effect = (const EconomyEventHeader*)GlueLinear((unsigned)a[0].i32);
    unsigned int size = (unsigned int)a[1].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Emit", "the world view is not set");
    ((const EconomyWorldView*)GlueActive()->world)->Emit(effect, size);
    return nullptr;
}

const char* __cdecl Import_econworld_FirstUnit(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.FirstUnit", "the world view is not set");
    EconUnit result = ((const EconomyWorldView*)GlueActive()->world)->FirstUnit(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_LastUnit(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.LastUnit", "the world view is not set");
    EconUnit result = ((const EconomyWorldView*)GlueActive()->world)->LastUnit(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_NextUnit(wasmtime_val_raw_t* a)
{
    EconUnit unit = (EconUnit)a[0].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.NextUnit", "the world view is not set");
    EconUnit result = ((const EconomyWorldView*)GlueActive()->world)->NextUnit(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_PlayerInfo(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    GLUE_NEED("econworld.PlayerInfo", 1);
    EconPlayerInfo* out = (EconPlayerInfo*)GlueLinear((unsigned)a[1].i32);
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.PlayerInfo", "the world view is not set");
    ((const EconomyWorldView*)GlueActive()->world)->PlayerInfo(player, out);
    return nullptr;
}

const char* __cdecl Import_econworld_SharingEnabled(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.SharingEnabled", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->SharingEnabled();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_Tick(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Tick", "the world view is not set");
    unsigned int result = ((const EconomyWorldView*)GlueActive()->world)->Tick();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_Tide(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Tide", "the world view is not set");
    float result = ((const EconomyWorldView*)GlueActive()->world)->Tide();
    a[0].f32 = (float)result;
    return nullptr;
}

const char* __cdecl Import_econworld_TypeInfo(wasmtime_val_raw_t* a)
{
    EconType type = (EconType)a[0].i32;
    GLUE_NEED("econworld.TypeInfo", 1);
    EconTypeInfo* out = (EconTypeInfo*)GlueLinear((unsigned)a[1].i32);
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.TypeInfo", "the world view is not set");
    ((const EconomyWorldView*)GlueActive()->world)->TypeInfo(type, out);
    return nullptr;
}

const char* __cdecl Import_econworld_UnitInfo(wasmtime_val_raw_t* a)
{
    EconUnit unit = (EconUnit)a[0].i32;
    GLUE_NEED("econworld.UnitInfo", 1);
    EconUnitInfo* out = (EconUnitInfo*)GlueLinear((unsigned)a[1].i32);
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.UnitInfo", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->UnitInfo(unit, out);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_UnitSlot(wasmtime_val_raw_t* a)
{
    EconUnit unit = (EconUnit)a[0].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.UnitSlot", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->UnitSlot(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_econworld_Wind(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Wind", "the world view is not set");
    float result = ((const EconomyWorldView*)GlueActive()->world)->Wind();
    a[0].f32 = (float)result;
    return nullptr;
}

const char* __cdecl Import_features_FindFeatureAtPos(wasmtime_val_raw_t* a)
{
    GLUE_NEED("features.FindFeatureAtPos", 0);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("features.FindFeatureAtPos", 1);
    Point16* cell = (Point16*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("features.FindFeatureAtPos", 2);
    Point16* size = (Point16*)GlueLinear((unsigned)a[2].i32);
    unsigned short result = g_seams.features->FindFeatureAtPos(pos, cell, size);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_features_GetCellFeature(wasmtime_val_raw_t* a)
{
    GLUE_NEED("features.GetCellFeature", 0);
    Cell* cell = (Cell*)GlueLinear((unsigned)a[0].i32);
    unsigned short result = g_seams.features->GetCellFeature(cell);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_features_GetFootprintCentre(wasmtime_val_raw_t* a)
{
    GLUE_NEED("features.GetFootprintCentre", 0);
    const Point16* cell = (const Point16*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("features.GetFootprintCentre", 1);
    Feature* def = (Feature*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("features.GetFootprintCentre", 2);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[2].i32);
    g_seams.features->GetFootprintCentre(cell, def, out);
    return nullptr;
}

const char* __cdecl Import_los_radar_ClampWorldPosToTerrain(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int y = (int)a[1].i32;
    GLUE_NEED("los_radar.ClampWorldPosToTerrain", 2);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[2].i32);
    g_seams.los_radar->ClampWorldPosToTerrain(x, y, out);
    return nullptr;
}

const char* __cdecl Import_los_radar_GetMapCell(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int y = (int)a[1].i32;
    Cell* result = g_seams.los_radar->GetMapCell(x, y);
    unsigned address;
    const char* error = GlueAddressOf("los_radar.GetMapCell", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_los_radar_GetMapCellAtPosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.GetMapCellAtPosition", 0);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[0].i32);
    Cell* result = g_seams.los_radar->GetMapCellAtPosition(pos);
    unsigned address;
    const char* error = GlueAddressOf("los_radar.GetMapCellAtPosition", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_los_radar_GetOriginCellAtPosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.GetOriginCellAtPosition", 0);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[0].i32);
    Cell* result = g_seams.los_radar->GetOriginCellAtPosition(pos);
    unsigned address;
    const char* error = GlueAddressOf("los_radar.GetOriginCellAtPosition", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_los_radar_GetSeenUnitsInRadius(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    GLUE_NEED("los_radar.GetSeenUnitsInRadius", 1);
    const Vec3* pos = (const Vec3*)GlueLinear((unsigned)a[1].i32);
    int radius = (int)a[2].i32;
    GLUE_NEED("los_radar.GetSeenUnitsInRadius", 3);
    Ref<Unit>* out = (Ref<Unit>*)GlueLinear((unsigned)a[3].i32);
    int capacity = (int)a[4].i32;
    int result = g_seams.los_radar->GetSeenUnitsInRadius(player, pos, radius, out, capacity);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_los_radar_GetVisibleEnemiesInRadius(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    GLUE_NEED("los_radar.GetVisibleEnemiesInRadius", 1);
    const Vec3* pos = (const Vec3*)GlueLinear((unsigned)a[1].i32);
    int radius = (int)a[2].i32;
    int flags = (int)a[3].i32;
    GLUE_NEED("los_radar.GetVisibleEnemiesInRadius", 4);
    Ref<Unit>* out = (Ref<Unit>*)GlueLinear((unsigned)a[4].i32);
    int capacity = (int)a[5].i32;
    int result = g_seams.los_radar->GetVisibleEnemiesInRadius(player, pos, radius, flags, out, capacity);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_los_radar_IsPointVisible(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.IsPointVisible", 0);
    Player* player = (Player*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("los_radar.IsPointVisible", 1);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.los_radar->IsPointVisible(player, pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_los_radar_IsUnitVisibleToPlayer(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.IsUnitVisibleToPlayer", 0);
    Player* player = (Player*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("los_radar.IsUnitVisibleToPlayer", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.los_radar->IsUnitVisibleToPlayer(player, unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_los_radar_SoundHeardMask(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.SoundHeardMask", 0);
    const Vec3* at = (const Vec3*)GlueLinear((unsigned)a[0].i32);
    unsigned int result = g_seams.los_radar->SoundHeardMask(at);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_map_CanBuildAt(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.CanBuildAt", 0);
    UnitDef* unit = (UnitDef*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("map.CanBuildAt", 1);
    const Point16* cell = (const Point16*)GlueLinear((unsigned)a[1].i32);
    short type = (short)a[2].i32;
    GLUE_NEED("map.CanBuildAt", 3);
    Player* player = (Player*)GlueLinear((unsigned)a[3].i32);
    int result = ScheduleMapCanBuildAt(unit, *cell, type, player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_map_CanPlaceFootprintAt(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.CanPlaceFootprintAt", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("map.CanPlaceFootprintAt", 1);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.map->CanPlaceFootprintAt(unit, pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_map_CanPlaceUnitFootprint(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.CanPlaceUnitFootprint", 0);
    UnitDef* unit = (UnitDef*)GlueLinear((unsigned)a[0].i32);
    unsigned short otherTypeId = (unsigned short)a[1].i32;
    GLUE_NEED("map.CanPlaceUnitFootprint", 2);
    const Point16* cell = (const Point16*)GlueLinear((unsigned)a[2].i32);
    int flags = (int)a[3].i32;
    int result = ScheduleMapCanPlaceUnitFootprint(unit, otherTypeId, *cell, flags);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_map_GetCellMeanHeight(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.GetCellMeanHeight", 0);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[0].i32);
    int result = g_seams.map->GetCellMeanHeight(pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_map_GetGroundHeight(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.GetGroundHeight", 0);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[0].i32);
    int result = g_seams.map->GetGroundHeight(pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_map_IsPadSlotFree(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.IsPadSlotFree", 0);
    Unit* pad = (Unit*)GlueLinear((unsigned)a[0].i32);
    int id = (int)a[1].i32;
    int result = g_seams.map->IsPadSlotFree(pad, id);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_map_SnapWorldPosToFootprint(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.SnapWorldPosToFootprint", 0);
    UnitDef* unit = (UnitDef*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("map.SnapWorldPosToFootprint", 1);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[1].i32);
    g_seams.map->SnapWorldPosToFootprint(unit, pos);
    return nullptr;
}

const char* __cdecl Import_map_UnitsInRange(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.UnitsInRange", 0);
    const Vec3* pos = (const Vec3*)GlueLinear((unsigned)a[0].i32);
    int range = (int)a[1].i32;
    int skip = (int)a[2].i32;
    GLUE_NEED("map.UnitsInRange", 3);
    unsigned short* ids = (unsigned short*)GlueLinear((unsigned)a[3].i32);
    int capacity = (int)a[4].i32;
    int result = g_seams.map->UnitsInRange(pos, range, skip, ids, capacity);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_mission_GetGameType(wasmtime_val_raw_t* a)
{
    int result = g_seams.mission->GetGameType();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_mission_GetMissionParams(wasmtime_val_raw_t* a)
{
    GLUE_NEED("mission.GetMissionParams", 0);
    MissionParams* out = (MissionParams*)GlueLinear((unsigned)a[0].i32);
    g_seams.mission->GetMissionParams(out);
    return nullptr;
}

const char* __cdecl Import_mission_GetNameSlot(wasmtime_val_raw_t* a)
{
    int index = (int)a[0].i32;
    const char* result = g_seams.mission->GetNameSlot(index);
    unsigned address;
    const char* error = GlueAddressOf("mission.GetNameSlot", (const void*)result, 0, &address, "pointer");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_movement_NewAirManeuverOrder(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    GLUE_NEED("movement.NewAirManeuverOrder", 1);
    const Vec3* from = (const Vec3*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("movement.NewAirManeuverOrder", 2);
    const Vec3* to = (const Vec3*)GlueLinear((unsigned)a[2].i32);
    unsigned int result = g_seams.movement->NewAirManeuverOrder(order, from, to);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_movement_NewApproachRadius(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int radius = (int)a[3].i32;
    unsigned int result = g_seams.movement->NewApproachRadius(order, x, y, radius);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_movement_NewPathOrderAt(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    GLUE_NEED("movement.NewPathOrderAt", 1);
    const Vec3* pos = (const Vec3*)GlueLinear((unsigned)a[1].i32);
    unsigned int result = g_seams.movement->NewPathOrderAt(order, pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_movement_NewPathOrderToUnit(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    GLUE_NEED("movement.NewPathOrderToUnit", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    unsigned int result = g_seams.movement->NewPathOrderToUnit(order, unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_movement_NewPathOrderToUnitAt(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    GLUE_NEED("movement.NewPathOrderToUnitAt", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("movement.NewPathOrderToUnitAt", 2);
    const Vec3* pos = (const Vec3*)GlueLinear((unsigned)a[2].i32);
    unsigned int result = g_seams.movement->NewPathOrderToUnitAt(order, unit, pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_movement_NewPathOrderToUnitAtAngle(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    GLUE_NEED("movement.NewPathOrderToUnitAtAngle", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    short angle = (short)a[2].i32;
    unsigned int result = g_seams.movement->NewPathOrderToUnitAtAngle(order, unit, angle);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_movement_NewPointMarker(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    GLUE_NEED("movement.NewPointMarker", 1);
    const Point16* pos = (const Point16*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("movement.NewPointMarker", 2);
    const Point16* size = (const Point16*)GlueLinear((unsigned)a[2].i32);
    unsigned int result = g_seams.movement->NewPointMarker(order, pos, size);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_movement_NewRingApproach(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int inner = (int)a[3].i32;
    int outer = (int)a[4].i32;
    unsigned int result = g_seams.movement->NewRingApproach(order, x, y, inner, outer);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_movement_TargetType(wasmtime_val_raw_t* a)
{
    unsigned int target = (unsigned int)a[0].i32;
    int result = g_seams.movement->TargetType(target);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_movement_UnitTarget(wasmtime_val_raw_t* a)
{
    GLUE_NEED("movement.UnitTarget", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    unsigned int result = g_seams.movement->UnitTarget(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_network_BroadcastPlayerEconomy(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.BroadcastPlayerEconomy", 0);
    Player* player = (Player*)GlueLinear((unsigned)a[0].i32);
    g_seams.network->BroadcastPlayerEconomy(player);
    return nullptr;
}

const char* __cdecl Import_network_BroadcastPlayerInfo(wasmtime_val_raw_t* a)
{
    g_seams.network->BroadcastPlayerInfo();
    return nullptr;
}

const char* __cdecl Import_network_SendActivateFlags(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendActivateFlags", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    g_seams.network->SendActivateFlags(unit);
    return nullptr;
}

const char* __cdecl Import_network_SendAttach(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendAttach", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendAttach", 1);
    Unit* carrier = (Unit*)GlueLinear((unsigned)a[1].i32);
    char piece = (char)a[2].i32;
    char mode = (char)a[3].i32;
    g_seams.network->SendAttach(unit, carrier, piece, mode);
    return nullptr;
}

const char* __cdecl Import_network_SendBuilderLink(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendBuilderLink", 0);
    Unit* builder = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendBuilderLink", 1);
    Unit* constructed = (Unit*)GlueLinear((unsigned)a[1].i32);
    g_seams.network->SendBuilderLink(builder, constructed);
    return nullptr;
}

const char* __cdecl Import_network_SendFeatureBurning(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int z = (int)a[1].i32;
    g_seams.network->SendFeatureBurning(x, z);
    return nullptr;
}

const char* __cdecl Import_network_SendFeatureHit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendFeatureHit", 0);
    WeaponDef* weapon = (WeaponDef*)GlueLinear((unsigned)a[0].i32);
    int x = (int)a[1].i32;
    int z = (int)a[2].i32;
    g_seams.network->SendFeatureHit(weapon, x, z);
    return nullptr;
}

const char* __cdecl Import_network_SendFeatureKilled(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int z = (int)a[1].i32;
    g_seams.network->SendFeatureKilled(x, z);
    return nullptr;
}

const char* __cdecl Import_network_SendFeatureReclaimed(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendFeatureReclaimed", 0);
    Unit* reclaimer = (Unit*)GlueLinear((unsigned)a[0].i32);
    int x = (int)a[1].i32;
    int z = (int)a[2].i32;
    g_seams.network->SendFeatureReclaimed(reclaimer, x, z);
    return nullptr;
}

const char* __cdecl Import_network_SendNewUnit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendNewUnit", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    g_seams.network->SendNewUnit(unit);
    return nullptr;
}

const char* __cdecl Import_network_SendOwnershipTransfer(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendOwnershipTransfer", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendOwnershipTransfer", 1);
    Player* newOwner = (Player*)GlueLinear((unsigned)a[1].i32);
    g_seams.network->SendOwnershipTransfer(unit, newOwner);
    return nullptr;
}

const char* __cdecl Import_network_SendProjectileDetonated(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendProjectileDetonated", 0);
    Unit* attacker = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendProjectileDetonated", 1);
    Projectile* projectile = (Projectile*)GlueLinear((unsigned)a[1].i32);
    g_seams.network->SendProjectileDetonated(attacker, projectile);
    return nullptr;
}

const char* __cdecl Import_network_SendScriptCall(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendScriptCall", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    short index = (short)a[1].i32;
    char argCount = (char)a[2].i32;
    int arg0 = (int)a[3].i32;
    int arg1 = (int)a[4].i32;
    int arg2 = (int)a[5].i32;
    int arg3 = (int)a[6].i32;
    int result = g_seams.network->SendScriptCall(unit, index, argCount, arg0, arg1, arg2, arg3);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_network_SendScriptCallByName(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendScriptCallByName", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendScriptCallByName", 1);
    const char* name = (const char*)GlueLinear((unsigned)a[1].i32);
    char argCount = (char)a[2].i32;
    int arg0 = (int)a[3].i32;
    int arg1 = (int)a[4].i32;
    int arg2 = (int)a[5].i32;
    int arg3 = (int)a[6].i32;
    int result = g_seams.network->SendScriptCallByName(unit, name, argCount, arg0, arg1, arg2, arg3);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_network_SendShareEnergy(wasmtime_val_raw_t* a)
{
    unsigned char from = (unsigned char)a[0].i32;
    unsigned char to = (unsigned char)a[1].i32;
    float amount = (float)a[2].f32;
    g_seams.network->SendShareEnergy(from, to, amount);
    return nullptr;
}

const char* __cdecl Import_network_SendShareMapInfo(wasmtime_val_raw_t* a)
{
    unsigned char from = (unsigned char)a[0].i32;
    unsigned char to = (unsigned char)a[1].i32;
    g_seams.network->SendShareMapInfo(from, to);
    return nullptr;
}

const char* __cdecl Import_network_SendShareMetal(wasmtime_val_raw_t* a)
{
    unsigned char from = (unsigned char)a[0].i32;
    unsigned char to = (unsigned char)a[1].i32;
    float amount = (float)a[2].f32;
    g_seams.network->SendShareMetal(from, to, amount);
    return nullptr;
}

const char* __cdecl Import_network_SendUnitDamage(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendUnitDamage", 0);
    Unit* source = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendUnitDamage", 1);
    Unit* target = (Unit*)GlueLinear((unsigned)a[1].i32);
    int amount = (int)a[2].i32;
    int kind = (int)a[3].i32;
    unsigned short extra = (unsigned short)a[4].i32;
    g_seams.network->SendUnitDamage(source, target, amount, kind, extra);
    return nullptr;
}

const char* __cdecl Import_network_SendUnitKilled(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendUnitKilled", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int severity = (int)a[1].i32;
    int wreckageLevel = (int)a[2].i32;
    int killType = (int)a[3].i32;
    g_seams.network->SendUnitKilled(unit, severity, wreckageLevel, killType);
    return nullptr;
}

const char* __cdecl Import_network_SendWeaponFire(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendWeaponFire", 0);
    Unit* shooter = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendWeaponFire", 1);
    UnitWeaponSlot* slot = (UnitWeaponSlot*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("network.SendWeaponFire", 2);
    Unit* target = (Unit*)GlueLinear((unsigned)a[2].i32);
    GLUE_NEED("network.SendWeaponFire", 3);
    Vec3* origin = (Vec3*)GlueLinear((unsigned)a[3].i32);
    GLUE_NEED("network.SendWeaponFire", 4);
    Vec3* aimOrVel = (Vec3*)GlueLinear((unsigned)a[4].i32);
    g_seams.network->SendWeaponFire(shooter, slot, target, origin, aimOrVel);
    return nullptr;
}

const char* __cdecl Import_network_SendWeaponFireAt(wasmtime_val_raw_t* a)
{
    unsigned char weaponIndex = (unsigned char)a[0].i32;
    GLUE_NEED("network.SendWeaponFireAt", 1);
    Vec3* origin = (Vec3*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("network.SendWeaponFireAt", 2);
    Vec3* aimOrVel = (Vec3*)GlueLinear((unsigned)a[2].i32);
    g_seams.network->SendWeaponFireAt(weaponIndex, origin, aimOrVel);
    return nullptr;
}

const char* __cdecl Import_orders_GetOrderFlags(wasmtime_val_raw_t* a)
{
    GLUE_NEED("orders.GetOrderFlags", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int result = g_seams.orders->GetOrderFlags(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_orders_GetOrderType(wasmtime_val_raw_t* a)
{
    unsigned char mode = (unsigned char)a[0].i32;
    GLUE_NEED("orders.GetOrderType", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("orders.GetOrderType", 2);
    Unit* target = (Unit*)GlueLinear((unsigned)a[2].i32);
    GLUE_NEED("orders.GetOrderType", 3);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[3].i32);
    int result = g_seams.orders->GetOrderType(mode, unit, target, pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathfinding_Cancel(wasmtime_val_raw_t* a)
{
    PathHandle requester = (PathHandle)a[0].i32;
    g_seams.pathfinding->Cancel(requester);
    return nullptr;
}

const char* __cdecl Import_pathfinding_CellsChanged(wasmtime_val_raw_t* a)
{
    int left = (int)a[0].i32;
    int top = (int)a[1].i32;
    int right = (int)a[2].i32;
    int bottom = (int)a[3].i32;
    unsigned int classes = (unsigned int)a[4].i32;
    int cause = (int)a[5].i32;
    g_seams.pathfinding->CellsChanged(left, top, right, bottom, classes, cause);
    return nullptr;
}

const char* __cdecl Import_pathfinding_Submit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("pathfinding.Submit", 0);
    const PathRequest* request = (const PathRequest*)GlueLinear((unsigned)a[0].i32);
    g_seams.pathfinding->Submit(request);
    return nullptr;
}

const char* __cdecl Import_pathworld_BeginSearch(wasmtime_val_raw_t* a)
{
    PathUnit unit = (PathUnit)a[0].i32;
    g_seams.pathworld->BeginSearch(unit);
    return nullptr;
}

const char* __cdecl Import_pathworld_EndSearch(wasmtime_val_raw_t* a)
{
    PathUnit unit = (PathUnit)a[0].i32;
    g_seams.pathworld->EndSearch(unit);
    return nullptr;
}

const char* __cdecl Import_pathworld_FirstUnit(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    PathUnit result = g_seams.pathworld->FirstUnit(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_InBounds(wasmtime_val_raw_t* a)
{
    PathClass moveClass = (PathClass)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int result = g_seams.pathworld->InBounds(moveClass, x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_IsPlaying(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    int result = g_seams.pathworld->IsPlaying(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_LastUnit(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    PathUnit result = g_seams.pathworld->LastUnit(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_LineOfSight(wasmtime_val_raw_t* a)
{
    PathClass moveClass = (PathClass)a[0].i32;
    int x0 = (int)a[1].i32;
    int y0 = (int)a[2].i32;
    int x1 = (int)a[3].i32;
    int y1 = (int)a[4].i32;
    int result = g_seams.pathworld->LineOfSight(moveClass, x0, y0, x1, y1);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_MoveClass(wasmtime_val_raw_t* a)
{
    int index = (int)a[0].i32;
    PathClass result = g_seams.pathworld->MoveClass(index);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_NextUnit(wasmtime_val_raw_t* a)
{
    PathUnit unit = (PathUnit)a[0].i32;
    PathUnit result = g_seams.pathworld->NextUnit(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_Pass(wasmtime_val_raw_t* a)
{
    PathClass moveClass = (PathClass)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int result = g_seams.pathworld->Pass(moveClass, x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_PlayerCount(wasmtime_val_raw_t* a)
{
    int result = g_seams.pathworld->PlayerCount();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_SearchStepLimit(wasmtime_val_raw_t* a)
{
    unsigned int result = g_seams.pathworld->SearchStepLimit();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_Seen(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int result = g_seams.pathworld->Seen(player, x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_SeenSize(wasmtime_val_raw_t* a)
{
    GLUE_NEED("pathworld.SeenSize", 0);
    int* width = (int*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("pathworld.SeenSize", 1);
    int* height = (int*)GlueLinear((unsigned)a[1].i32);
    g_seams.pathworld->SeenSize(width, height);
    return nullptr;
}

const char* __cdecl Import_pathworld_TerrainPass(wasmtime_val_raw_t* a)
{
    PathClass moveClass = (PathClass)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int result = g_seams.pathworld->TerrainPass(moveClass, x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_Tick(wasmtime_val_raw_t* a)
{
    unsigned int result = g_seams.pathworld->Tick();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_UnitState(wasmtime_val_raw_t* a)
{
    PathUnit unit = (PathUnit)a[0].i32;
    GLUE_NEED("pathworld.UnitState", 1);
    PathUnitState* out = (PathUnitState*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.pathworld->UnitState(unit, out);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_pathworld_UnitsPerPlayer(wasmtime_val_raw_t* a)
{
    int result = g_seams.pathworld->UnitsPerPlayer();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_scripts_CreateObjectState(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.CreateObjectState", 0);
    Object3do* model = (Object3do*)GlueLinear((unsigned)a[0].i32);
    ModelState* result = g_seams.scripts->CreateObjectState(model);
    unsigned address;
    const char* error = GlueAddressOf("scripts.CreateObjectState", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_scripts_CreatePlayerObjectState(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.CreatePlayerObjectState", 0);
    Object3do* model = (Object3do*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.CreatePlayerObjectState", 1);
    ScriptTable* data = (ScriptTable*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("scripts.CreatePlayerObjectState", 2);
    Unit* owner = (Unit*)GlueLinear((unsigned)a[2].i32);
    ModelState* result = g_seams.scripts->CreatePlayerObjectState(model, data, owner);
    unsigned address;
    const char* error = GlueAddressOf("scripts.CreatePlayerObjectState", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_scripts_DeleteScript(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.DeleteScript", 0);
    CobScript* script = (CobScript*)GlueLinear((unsigned)a[0].i32);
    g_seams.scripts->DeleteScript(script);
    return nullptr;
}

const char* __cdecl Import_scripts_FindScript(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.FindScript", 0);
    CobScript* script = (CobScript*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.FindScript", 1);
    const char* name = (const char*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.scripts->FindScript(script, name);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_scripts_FreeObjectState(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.FreeObjectState", 0);
    ModelState* state = (ModelState*)GlueLinear((unsigned)a[0].i32);
    g_seams.scripts->FreeObjectState(state);
    return nullptr;
}

const char* __cdecl Import_scripts_GetAimFromPosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetAimFromPosition", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.GetAimFromPosition", 1);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[1].i32);
    unsigned char weapon = (unsigned char)a[2].i32;
    g_seams.scripts->GetAimFromPosition(unit, out, weapon);
    return nullptr;
}

const char* __cdecl Import_scripts_GetNanoPiecePosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetNanoPiecePosition", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.GetNanoPiecePosition", 1);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[1].i32);
    g_seams.scripts->GetNanoPiecePosition(unit, out);
    return nullptr;
}

const char* __cdecl Import_scripts_GetPieceAngles(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetPieceAngles", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int piece = (int)a[1].i32;
    GLUE_NEED("scripts.GetPieceAngles", 2);
    Angles16* out = (Angles16*)GlueLinear((unsigned)a[2].i32);
    g_seams.scripts->GetPieceAngles(unit, piece, out);
    return nullptr;
}

const char* __cdecl Import_scripts_GetPieceOffset(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetPieceOffset", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int piece = (int)a[1].i32;
    GLUE_NEED("scripts.GetPieceOffset", 2);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[2].i32);
    g_seams.scripts->GetPieceOffset(unit, piece, out);
    return nullptr;
}

const char* __cdecl Import_scripts_GetPiecePosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetPiecePosition", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int piece = (int)a[1].i32;
    GLUE_NEED("scripts.GetPiecePosition", 2);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[2].i32);
    g_seams.scripts->GetPiecePosition(unit, piece, out);
    return nullptr;
}

const char* __cdecl Import_scripts_GetSweetSpot(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetSweetSpot", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.GetSweetSpot", 1);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[1].i32);
    g_seams.scripts->GetSweetSpot(unit, out);
    return nullptr;
}

const char* __cdecl Import_scripts_NewUnitScript(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.NewUnitScript", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    g_seams.scripts->NewUnitScript(unit);
    return nullptr;
}

const char* __cdecl Import_scripts_ObjectStateSize(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.ObjectStateSize", 0);
    Object3do* model = (Object3do*)GlueLinear((unsigned)a[0].i32);
    unsigned int result = g_seams.scripts->ObjectStateSize(model);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_scripts_QueryScript(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.QueryScript", 0);
    CobScript* script = (CobScript*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.QueryScript", 1);
    const char* name = (const char*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("scripts.QueryScript", 2);
    int* arg0 = (int*)GlueLinear((unsigned)a[2].i32);
    GLUE_NEED("scripts.QueryScript", 3);
    int* arg1 = (int*)GlueLinear((unsigned)a[3].i32);
    GLUE_NEED("scripts.QueryScript", 4);
    int* arg2 = (int*)GlueLinear((unsigned)a[4].i32);
    GLUE_NEED("scripts.QueryScript", 5);
    int* arg3 = (int*)GlueLinear((unsigned)a[5].i32);
    int result = g_seams.scripts->QueryScript(script, name, arg0, arg1, arg2, arg3);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_scripts_SetCob(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.SetCob", 0);
    CobScript* script = (CobScript*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.SetCob", 1);
    ScriptTable* data = (ScriptTable*)GlueLinear((unsigned)a[1].i32);
    g_seams.scripts->SetCob(script, data);
    return nullptr;
}

const char* __cdecl Import_scripts_SetObjectState(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.SetObjectState", 0);
    UnitScript* script = (UnitScript*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.SetObjectState", 1);
    ModelState* state = (ModelState*)GlueLinear((unsigned)a[1].i32);
    g_seams.scripts->SetObjectState(script, state);
    return nullptr;
}

const char* __cdecl Import_targeting_ChooseWeapon(wasmtime_val_raw_t* a)
{
    GLUE_NEED("targeting.ChooseWeapon", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    unsigned char result = g_seams.targeting->ChooseWeapon(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_targeting_FindBestTargetIfFireAtWill(wasmtime_val_raw_t* a)
{
    GLUE_NEED("targeting.FindBestTargetIfFireAtWill", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    Unit* result = ScheduleTargetingFireAtWill(unit);
    unsigned address;
    const char* error = GlueAddressOf("targeting.FindBestTargetIfFireAtWill", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_targeting_FindWeaponTarget(wasmtime_val_raw_t* a)
{
    GLUE_NEED("targeting.FindWeaponTarget", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    unsigned char weapon = (unsigned char)a[1].i32;
    int useRange = (int)a[2].i32;
    Unit* result = ScheduleTargetingFind(unit, weapon, useRange);
    unsigned address;
    const char* error = GlueAddressOf("targeting.FindWeaponTarget", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_targeting_GetWeaponTargetPos(wasmtime_val_raw_t* a)
{
    GLUE_NEED("targeting.GetWeaponTargetPos", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("targeting.GetWeaponTargetPos", 1);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[1].i32);
    int weapon = (int)a[2].i32;
    int result = ScheduleTargetingTargetPos(unit, pos, weapon);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_targeting_GetWeaponTargetUnit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("targeting.GetWeaponTargetUnit", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int weapon = (int)a[1].i32;
    Unit* result = ScheduleTargetingTargetUnit(unit, weapon);
    unsigned address;
    const char* error = GlueAddressOf("targeting.GetWeaponTargetUnit", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_units_AddBuildProgress(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.AddBuildProgress", 0);
    Unit* builder = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.AddBuildProgress", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    float amount = (float)a[2].f32;
    int result = g_seams.units->AddBuildProgress(builder, unit, amount);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_units_AddRepairProgress(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.AddRepairProgress", 0);
    Unit* builder = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.AddRepairProgress", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    float amount = (float)a[2].f32;
    int result = g_seams.units->AddRepairProgress(builder, unit, amount);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_units_CanLoad(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.CanLoad", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.CanLoad", 1);
    Unit* target = (Unit*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.units->CanLoad(unit, target);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_units_CanReclaim(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.CanReclaim", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.CanReclaim", 1);
    Unit* target = (Unit*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.units->CanReclaim(unit, target);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_units_CanRepair(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.CanRepair", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.CanRepair", 1);
    Unit* target = (Unit*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.units->CanRepair(unit, target);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_units_CreateUnit(wasmtime_val_raw_t* a)
{
    unsigned char player = (unsigned char)a[0].i32;
    unsigned short typeId = (unsigned short)a[1].i32;
    GLUE_NEED("units.CreateUnit", 2);
    const Vec3* pos = (const Vec3*)GlueLinear((unsigned)a[2].i32);
    int finished = (int)a[3].i32;
    int mode = (int)a[4].i32;
    int id = (int)a[5].i32;
    Unit* result = g_seams.units->CreateUnit(player, typeId, pos, finished, mode, id);
    unsigned address;
    const char* error = GlueAddressOf("units.CreateUnit", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_units_FindUnitTypeId(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.FindUnitTypeId", 0);
    const char* name = (const char*)GlueLinear((unsigned)a[0].i32);
    unsigned short result = g_seams.units->FindUnitTypeId(name);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_units_GetFactoriesInRadius(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    GLUE_NEED("units.GetFactoriesInRadius", 1);
    const Vec3* pos = (const Vec3*)GlueLinear((unsigned)a[1].i32);
    int radius = (int)a[2].i32;
    GLUE_NEED("units.GetFactoriesInRadius", 3);
    Ref<Unit>* out = (Ref<Unit>*)GlueLinear((unsigned)a[3].i32);
    int capacity = (int)a[4].i32;
    int result = g_seams.units->GetFactoriesInRadius(player, pos, radius, out, capacity);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_units_UnitRefGet(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.UnitRefGet", 0);
    UnitRef* ref = (UnitRef*)GlueLinear((unsigned)a[0].i32);
    Unit* result = g_seams.units->UnitRefGet(ref);
    unsigned address;
    const char* error = GlueAddressOf("units.UnitRefGet", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_units_UnitRefInit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.UnitRefInit", 0);
    UnitRef* ref = (UnitRef*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.UnitRefInit", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("units.UnitRefInit", 2);
    OrderBase* value = (OrderBase*)GlueLinear((unsigned)a[2].i32);
    g_seams.units->UnitRefInit(ref, unit, value);
    return nullptr;
}

const char* __cdecl Import_units_UnitRefSetUnit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.UnitRefSetUnit", 0);
    UnitRef* ref = (UnitRef*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.UnitRefSetUnit", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    g_seams.units->UnitRefSetUnit(ref, unit);
    return nullptr;
}

const char* __cdecl Import_units_UnitRefSetValue(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.UnitRefSetValue", 0);
    UnitRef* ref = (UnitRef*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.UnitRefSetValue", 1);
    OrderBase* value = (OrderBase*)GlueLinear((unsigned)a[1].i32);
    g_seams.units->UnitRefSetValue(ref, value);
    return nullptr;
}

const char* __cdecl Import_units_UnitRefUnlink(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.UnitRefUnlink", 0);
    UnitRef* ref = (UnitRef*)GlueLinear((unsigned)a[0].i32);
    g_seams.units->UnitRefUnlink(ref);
    return nullptr;
}

const char* __cdecl Import_weapons_FindTargetableProjectile(wasmtime_val_raw_t* a)
{
    GLUE_NEED("weapons.FindTargetableProjectile", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    unsigned int weapon = (unsigned int)a[1].i32;
    Projectile* result = g_seams.weapons->FindTargetableProjectile(unit, weapon);
    unsigned address;
    const char* error = GlueAddressOf("weapons.FindTargetableProjectile", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_weapons_FindWeaponByName(wasmtime_val_raw_t* a)
{
    GLUE_NEED("weapons.FindWeaponByName", 0);
    const char* name = (const char*)GlueLinear((unsigned)a[0].i32);
    WeaponDef* result = g_seams.weapons->FindWeaponByName(name);
    unsigned address;
    const char* error = GlueAddressOf("weapons.FindWeaponByName", (const void*)result, 1, &address, "weapon");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl Import_weapons_GetWeaponRange(wasmtime_val_raw_t* a)
{
    GLUE_NEED("weapons.GetWeaponRange", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    unsigned int weapon = (unsigned int)a[1].i32;
    int result = g_seams.weapons->GetWeaponRange(unit, weapon);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_weapons_WeaponCanReachPos(wasmtime_val_raw_t* a)
{
    GLUE_NEED("weapons.WeaponCanReachPos", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("weapons.WeaponCanReachPos", 1);
    Vec3* from = (Vec3*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("weapons.WeaponCanReachPos", 2);
    Vec3* to = (Vec3*)GlueLinear((unsigned)a[2].i32);
    int flags = (int)a[3].i32;
    int result = g_seams.weapons->WeaponCanReachPos(unit, from, to, flags);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl Import_weapons_WeaponCanReachUnit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("weapons.WeaponCanReachUnit", 0);
    Unit* shooter = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("weapons.WeaponCanReachUnit", 1);
    Unit* target = (Unit*)GlueLinear((unsigned)a[1].i32);
    unsigned char weapon = (unsigned char)a[2].i32;
    int result = g_seams.weapons->WeaponCanReachUnit(shooter, target, weapon);
    a[0].i32 = (int)result;
    return nullptr;
}

}

GlueModule* const g_glueModules[] = {
    &s_windMeteorsGlue,
    &s_economyGlue,
    &s_featuresGlue,
    &s_aiGlue,
    &s_pathfindingGlue,
    &s_weaponsGlue,
    &s_targetingGlue,
    &s_losRadarGlue,
    &s_ordersGlue,
    &s_unitsGlue,
    &s_playersGlue,
    &s_mapGlue,
    &s_scriptsGlue,
    &s_movementGlue,
    &s_missionGlue,
};
const unsigned g_glueModuleCount = sizeof g_glueModules / sizeof g_glueModules[0];

const GlueImport g_glueInterfaceImports[] = {
    {"economy", "GetEnergyUse", "i>d", Import_economy_GetEnergyUse},
    {"economy", "RequestGranted", "ii>i", Import_economy_RequestGranted},
    {"economy", "Spend", "i>i", Import_economy_Spend},
    {"economy", "UpdateMetalExtraction", "i>", Import_economy_UpdateMetalExtraction},
    {"econworld", "Allied", "ii>i", Import_econworld_Allied},
    {"econworld", "CellMetal", "ii>i", Import_econworld_CellMetal},
    {"econworld", "Difficulty", ">i", Import_econworld_Difficulty},
    {"econworld", "Emit", "ii>", Import_econworld_Emit},
    {"econworld", "FirstUnit", "i>i", Import_econworld_FirstUnit},
    {"econworld", "LastUnit", "i>i", Import_econworld_LastUnit},
    {"econworld", "NextUnit", "i>i", Import_econworld_NextUnit},
    {"econworld", "PlayerInfo", "ii>", Import_econworld_PlayerInfo},
    {"econworld", "SharingEnabled", ">i", Import_econworld_SharingEnabled},
    {"econworld", "Tick", ">i", Import_econworld_Tick},
    {"econworld", "Tide", ">f", Import_econworld_Tide},
    {"econworld", "TypeInfo", "ii>", Import_econworld_TypeInfo},
    {"econworld", "UnitInfo", "ii>i", Import_econworld_UnitInfo},
    {"econworld", "UnitSlot", "i>i", Import_econworld_UnitSlot},
    {"econworld", "Wind", ">f", Import_econworld_Wind},
    {"features", "FindFeatureAtPos", "iii>i", Import_features_FindFeatureAtPos},
    {"features", "GetCellFeature", "i>i", Import_features_GetCellFeature},
    {"features", "GetFootprintCentre", "iii>", Import_features_GetFootprintCentre},
    {"los_radar", "ClampWorldPosToTerrain", "iii>", Import_los_radar_ClampWorldPosToTerrain},
    {"los_radar", "GetMapCell", "ii>i", Import_los_radar_GetMapCell},
    {"los_radar", "GetMapCellAtPosition", "i>i", Import_los_radar_GetMapCellAtPosition},
    {"los_radar", "GetOriginCellAtPosition", "i>i", Import_los_radar_GetOriginCellAtPosition},
    {"los_radar", "GetSeenUnitsInRadius", "iiiii>i", Import_los_radar_GetSeenUnitsInRadius},
    {"los_radar", "GetVisibleEnemiesInRadius", "iiiiii>i", Import_los_radar_GetVisibleEnemiesInRadius},
    {"los_radar", "IsPointVisible", "ii>i", Import_los_radar_IsPointVisible},
    {"los_radar", "IsUnitVisibleToPlayer", "ii>i", Import_los_radar_IsUnitVisibleToPlayer},
    {"los_radar", "SoundHeardMask", "i>i", Import_los_radar_SoundHeardMask},
    {"map", "CanBuildAt", "iiii>i", Import_map_CanBuildAt},
    {"map", "CanPlaceFootprintAt", "ii>i", Import_map_CanPlaceFootprintAt},
    {"map", "CanPlaceUnitFootprint", "iiii>i", Import_map_CanPlaceUnitFootprint},
    {"map", "GetCellMeanHeight", "i>i", Import_map_GetCellMeanHeight},
    {"map", "GetGroundHeight", "i>i", Import_map_GetGroundHeight},
    {"map", "IsPadSlotFree", "ii>i", Import_map_IsPadSlotFree},
    {"map", "SnapWorldPosToFootprint", "ii>", Import_map_SnapWorldPosToFootprint},
    {"map", "UnitsInRange", "iiiii>i", Import_map_UnitsInRange},
    {"mission", "GetGameType", ">i", Import_mission_GetGameType},
    {"mission", "GetMissionParams", "i>", Import_mission_GetMissionParams},
    {"mission", "GetNameSlot", "i>i", Import_mission_GetNameSlot},
    {"movement", "NewAirManeuverOrder", "iii>i", Import_movement_NewAirManeuverOrder},
    {"movement", "NewApproachRadius", "iiii>i", Import_movement_NewApproachRadius},
    {"movement", "NewPathOrderAt", "ii>i", Import_movement_NewPathOrderAt},
    {"movement", "NewPathOrderToUnit", "ii>i", Import_movement_NewPathOrderToUnit},
    {"movement", "NewPathOrderToUnitAt", "iii>i", Import_movement_NewPathOrderToUnitAt},
    {"movement", "NewPathOrderToUnitAtAngle", "iii>i", Import_movement_NewPathOrderToUnitAtAngle},
    {"movement", "NewPointMarker", "iii>i", Import_movement_NewPointMarker},
    {"movement", "NewRingApproach", "iiiii>i", Import_movement_NewRingApproach},
    {"movement", "TargetType", "i>i", Import_movement_TargetType},
    {"movement", "UnitTarget", "i>i", Import_movement_UnitTarget},
    {"network", "BroadcastPlayerEconomy", "i>", Import_network_BroadcastPlayerEconomy},
    {"network", "BroadcastPlayerInfo", ">", Import_network_BroadcastPlayerInfo},
    {"network", "SendActivateFlags", "i>", Import_network_SendActivateFlags},
    {"network", "SendAttach", "iiii>", Import_network_SendAttach},
    {"network", "SendBuilderLink", "ii>", Import_network_SendBuilderLink},
    {"network", "SendFeatureBurning", "ii>", Import_network_SendFeatureBurning},
    {"network", "SendFeatureHit", "iii>", Import_network_SendFeatureHit},
    {"network", "SendFeatureKilled", "ii>", Import_network_SendFeatureKilled},
    {"network", "SendFeatureReclaimed", "iii>", Import_network_SendFeatureReclaimed},
    {"network", "SendNewUnit", "i>", Import_network_SendNewUnit},
    {"network", "SendOwnershipTransfer", "ii>", Import_network_SendOwnershipTransfer},
    {"network", "SendProjectileDetonated", "ii>", Import_network_SendProjectileDetonated},
    {"network", "SendScriptCall", "iiiiiii>i", Import_network_SendScriptCall},
    {"network", "SendScriptCallByName", "iiiiiii>i", Import_network_SendScriptCallByName},
    {"network", "SendShareEnergy", "iif>", Import_network_SendShareEnergy},
    {"network", "SendShareMapInfo", "ii>", Import_network_SendShareMapInfo},
    {"network", "SendShareMetal", "iif>", Import_network_SendShareMetal},
    {"network", "SendUnitDamage", "iiiii>", Import_network_SendUnitDamage},
    {"network", "SendUnitKilled", "iiii>", Import_network_SendUnitKilled},
    {"network", "SendWeaponFire", "iiiii>", Import_network_SendWeaponFire},
    {"network", "SendWeaponFireAt", "iii>", Import_network_SendWeaponFireAt},
    {"orders", "GetOrderFlags", "i>i", Import_orders_GetOrderFlags},
    {"orders", "GetOrderType", "iiii>i", Import_orders_GetOrderType},
    {"pathfinding", "Cancel", "i>", Import_pathfinding_Cancel},
    {"pathfinding", "CellsChanged", "iiiiii>", Import_pathfinding_CellsChanged},
    {"pathfinding", "Submit", "i>", Import_pathfinding_Submit},
    {"pathworld", "BeginSearch", "i>", Import_pathworld_BeginSearch},
    {"pathworld", "EndSearch", "i>", Import_pathworld_EndSearch},
    {"pathworld", "FirstUnit", "i>i", Import_pathworld_FirstUnit},
    {"pathworld", "InBounds", "iii>i", Import_pathworld_InBounds},
    {"pathworld", "IsPlaying", "i>i", Import_pathworld_IsPlaying},
    {"pathworld", "LastUnit", "i>i", Import_pathworld_LastUnit},
    {"pathworld", "LineOfSight", "iiiii>i", Import_pathworld_LineOfSight},
    {"pathworld", "MoveClass", "i>i", Import_pathworld_MoveClass},
    {"pathworld", "NextUnit", "i>i", Import_pathworld_NextUnit},
    {"pathworld", "Pass", "iii>i", Import_pathworld_Pass},
    {"pathworld", "PlayerCount", ">i", Import_pathworld_PlayerCount},
    {"pathworld", "SearchStepLimit", ">i", Import_pathworld_SearchStepLimit},
    {"pathworld", "Seen", "iii>i", Import_pathworld_Seen},
    {"pathworld", "SeenSize", "ii>", Import_pathworld_SeenSize},
    {"pathworld", "TerrainPass", "iii>i", Import_pathworld_TerrainPass},
    {"pathworld", "Tick", ">i", Import_pathworld_Tick},
    {"pathworld", "UnitState", "ii>i", Import_pathworld_UnitState},
    {"pathworld", "UnitsPerPlayer", ">i", Import_pathworld_UnitsPerPlayer},
    {"scripts", "CreateObjectState", "i>i", Import_scripts_CreateObjectState},
    {"scripts", "CreatePlayerObjectState", "iii>i", Import_scripts_CreatePlayerObjectState},
    {"scripts", "DeleteScript", "i>", Import_scripts_DeleteScript},
    {"scripts", "FindScript", "ii>i", Import_scripts_FindScript},
    {"scripts", "FreeObjectState", "i>", Import_scripts_FreeObjectState},
    {"scripts", "GetAimFromPosition", "iii>", Import_scripts_GetAimFromPosition},
    {"scripts", "GetNanoPiecePosition", "ii>", Import_scripts_GetNanoPiecePosition},
    {"scripts", "GetPieceAngles", "iii>", Import_scripts_GetPieceAngles},
    {"scripts", "GetPieceOffset", "iii>", Import_scripts_GetPieceOffset},
    {"scripts", "GetPiecePosition", "iii>", Import_scripts_GetPiecePosition},
    {"scripts", "GetSweetSpot", "ii>", Import_scripts_GetSweetSpot},
    {"scripts", "NewUnitScript", "i>", Import_scripts_NewUnitScript},
    {"scripts", "ObjectStateSize", "i>i", Import_scripts_ObjectStateSize},
    {"scripts", "QueryScript", "iiiiii>i", Import_scripts_QueryScript},
    {"scripts", "SetCob", "ii>", Import_scripts_SetCob},
    {"scripts", "SetObjectState", "ii>", Import_scripts_SetObjectState},
    {"targeting", "ChooseWeapon", "i>i", Import_targeting_ChooseWeapon},
    {"targeting", "FindBestTargetIfFireAtWill", "i>i", Import_targeting_FindBestTargetIfFireAtWill},
    {"targeting", "FindWeaponTarget", "iii>i", Import_targeting_FindWeaponTarget},
    {"targeting", "GetWeaponTargetPos", "iii>i", Import_targeting_GetWeaponTargetPos},
    {"targeting", "GetWeaponTargetUnit", "ii>i", Import_targeting_GetWeaponTargetUnit},
    {"units", "AddBuildProgress", "iif>i", Import_units_AddBuildProgress},
    {"units", "AddRepairProgress", "iif>i", Import_units_AddRepairProgress},
    {"units", "CanLoad", "ii>i", Import_units_CanLoad},
    {"units", "CanReclaim", "ii>i", Import_units_CanReclaim},
    {"units", "CanRepair", "ii>i", Import_units_CanRepair},
    {"units", "CreateUnit", "iiiiii>i", Import_units_CreateUnit},
    {"units", "FindUnitTypeId", "i>i", Import_units_FindUnitTypeId},
    {"units", "GetFactoriesInRadius", "iiiii>i", Import_units_GetFactoriesInRadius},
    {"units", "UnitRefGet", "i>i", Import_units_UnitRefGet},
    {"units", "UnitRefInit", "iii>", Import_units_UnitRefInit},
    {"units", "UnitRefSetUnit", "ii>", Import_units_UnitRefSetUnit},
    {"units", "UnitRefSetValue", "ii>", Import_units_UnitRefSetValue},
    {"units", "UnitRefUnlink", "i>", Import_units_UnitRefUnlink},
    {"weapons", "FindTargetableProjectile", "ii>i", Import_weapons_FindTargetableProjectile},
    {"weapons", "FindWeaponByName", "i>i", Import_weapons_FindWeaponByName},
    {"weapons", "GetWeaponRange", "ii>i", Import_weapons_GetWeaponRange},
    {"weapons", "WeaponCanReachPos", "iiii>i", Import_weapons_WeaponCanReachPos},
    {"weapons", "WeaponCanReachUnit", "iii>i", Import_weapons_WeaponCanReachUnit},
};
const unsigned g_glueInterfaceImportCount = sizeof g_glueInterfaceImports / sizeof g_glueInterfaceImports[0];

const GlueRoute g_glueRoutes[] = {
    {SCHED_ENGINE, 32},
    {SCHED_ENGINE, 33},
    {SCHED_ENGINE, 36},
    {SCHED_ENGINE, 64},
    {SCHED_ENGINE, 65},
    {SCHED_ENGINE, 66},
    {SCHED_ENGINE, 67},
    {SCHED_ENGINE, 68},
    {SCHED_ENGINE, 69},
    {SCHED_ENGINE, 133},
    {SCHED_ENGINE, 134},
    {SCHED_ENGINE, 135},
    {SCHED_ENGINE, 136},
    {SCHED_ENGINE, 139},
    {SCHED_ENGINE, 170},
    {SCHED_ENGINE, 171},
    {SCHED_ENGINE, 192},
    {SCHED_ENGINE, 193},
    {SCHED_ENGINE, 196},
    {SCHED_ENGINE, 197},
    {SCHED_ENGINE, 198},
    {SCHED_ENGINE, 199},
    {SCHED_ENGINE, 200},
    {SCHED_ENGINE, 201},
    {SCHED_ENGINE, 202},
    {SCHED_ENGINE, 203},
    {SCHED_ENGINE, 204},
    {SCHED_ENGINE, 205},
    {SCHED_ENGINE, 226},
    {SCHED_ENGINE, 227},
    {SCHED_ENGINE, 228},
    {SCHED_ENGINE, 229},
    {SCHED_ENGINE, 230},
    {SCHED_ENGINE, 257},
    {SCHED_ENGINE, 289},
    {SCHED_ENGINE, 295},
    {SCHED_ENGINE, 296},
    {SCHED_ENGINE, 297},
    {SCHED_ENGINE, 298},
    {SCHED_ENGINE, 353},
    {SCHED_ENGINE, 357},
    {SCHED_ENGINE, 395},
    {SCHED_ENGINE, 396},
    {SCHED_ENGINE, 397},
    {SCHED_ENGINE, 398},
    {SCHED_ENGINE, 448},
    {SCHED_WIND_METEORS, 80},
    {SCHED_ECONOMY, 16},
    {SCHED_ECONOMY, 17},
    {SCHED_ECONOMY, 18},
    {SCHED_ECONOMY, 134},
    {SCHED_ECONOMY, 135},
    {SCHED_FEATURES, 4},
    {SCHED_FEATURES, 48},
    {SCHED_FEATURES, 49},
    {SCHED_FEATURES, 50},
    {SCHED_FEATURES, 51},
    {SCHED_AI, 96},
    {SCHED_AI, 97},
    {SCHED_AI, 98},
    {SCHED_AI, 240},
    {SCHED_AI, 241},
    {SCHED_AI, 243},
    {SCHED_AI, 245},
    {SCHED_AI, 246},
    {SCHED_WEAPONS, 34},
    {SCHED_WEAPONS, 98},
    {SCHED_WEAPONS, 272},
    {SCHED_WEAPONS, 273},
    {SCHED_WEAPONS, 274},
    {SCHED_WEAPONS, 275},
    {SCHED_WEAPONS, 276},
    {SCHED_WEAPONS, 277},
    {SCHED_WEAPONS, 278},
    {SCHED_WEAPONS, 279},
    {SCHED_WEAPONS, 280},
    {SCHED_WEAPONS, 281},
    {SCHED_WEAPONS, 282},
    {SCHED_WEAPONS, 283},
    {SCHED_TARGETING, 112},
    {SCHED_ORDERS, 2},
    {SCHED_ORDERS, 4},
    {SCHED_ORDERS, 33},
    {SCHED_ORDERS, 35},
    {SCHED_ORDERS, 96},
    {SCHED_ORDERS, 97},
    {SCHED_ORDERS, 98},
    {SCHED_ORDERS, 192},
    {SCHED_ORDERS, 256},
    {SCHED_ORDERS, 259},
    {SCHED_ORDERS, 260},
    {SCHED_ORDERS, 269},
    {SCHED_ORDERS, 295},
    {SCHED_ORDERS, 304},
    {SCHED_ORDERS, 305},
    {SCHED_ORDERS, 306},
    {SCHED_ORDERS, 307},
    {SCHED_ORDERS, 308},
    {SCHED_ORDERS, 309},
    {SCHED_ORDERS, 310},
    {SCHED_ORDERS, 311},
    {SCHED_ORDERS, 312},
    {SCHED_ORDERS, 313},
    {SCHED_ORDERS, 314},
    {SCHED_ORDERS, 315},
    {SCHED_ORDERS, 316},
    {SCHED_ORDERS, 317},
    {SCHED_ORDERS, 326},
    {SCHED_ORDERS, 327},
    {SCHED_ORDERS, 328},
    {SCHED_ORDERS, 329},
    {SCHED_ORDERS, 330},
    {SCHED_ORDERS, 331},
    {SCHED_UNITS, 0},
    {SCHED_UNITS, 1},
    {SCHED_UNITS, 2},
    {SCHED_UNITS, 4},
    {SCHED_UNITS, 32},
    {SCHED_UNITS, 99},
    {SCHED_UNITS, 100},
    {SCHED_UNITS, 128},
    {SCHED_UNITS, 129},
    {SCHED_UNITS, 130},
    {SCHED_UNITS, 131},
    {SCHED_UNITS, 137},
    {SCHED_UNITS, 138},
    {SCHED_UNITS, 160},
    {SCHED_UNITS, 161},
    {SCHED_UNITS, 162},
    {SCHED_UNITS, 163},
    {SCHED_UNITS, 164},
    {SCHED_UNITS, 165},
    {SCHED_UNITS, 166},
    {SCHED_UNITS, 169},
    {SCHED_UNITS, 193},
    {SCHED_UNITS, 194},
    {SCHED_UNITS, 195},
    {SCHED_UNITS, 226},
    {SCHED_UNITS, 256},
    {SCHED_UNITS, 258},
    {SCHED_UNITS, 259},
    {SCHED_UNITS, 261},
    {SCHED_UNITS, 262},
    {SCHED_UNITS, 267},
    {SCHED_UNITS, 288},
    {SCHED_UNITS, 290},
    {SCHED_UNITS, 291},
    {SCHED_UNITS, 292},
    {SCHED_UNITS, 320},
    {SCHED_UNITS, 321},
    {SCHED_UNITS, 322},
    {SCHED_UNITS, 323},
    {SCHED_UNITS, 324},
    {SCHED_UNITS, 416},
    {SCHED_PLAYERS, 0},
    {SCHED_PLAYERS, 4},
    {SCHED_PLAYERS, 128},
    {SCHED_PLAYERS, 133},
    {SCHED_PLAYERS, 208},
    {SCHED_PLAYERS, 209},
    {SCHED_PLAYERS, 210},
    {SCHED_MAP, 132},
    {SCHED_MAP, 176},
    {SCHED_SCRIPTS, 164},
    {SCHED_SCRIPTS, 263},
    {SCHED_SCRIPTS, 264},
    {SCHED_SCRIPTS, 357},
    {SCHED_SCRIPTS, 386},
    {SCHED_MOVEMENT, 128},
    {SCHED_MOVEMENT, 160},
    {SCHED_MOVEMENT, 161},
    {SCHED_MOVEMENT, 336},
    {SCHED_MOVEMENT, 337},
    {SCHED_MOVEMENT, 338},
    {SCHED_MOVEMENT, 339},
    {SCHED_MOVEMENT, 340},
    {SCHED_MOVEMENT, 341},
    {SCHED_MOVEMENT, 342},
    {SCHED_MOVEMENT, 343},
    {SCHED_MOVEMENT, 344},
    {SCHED_MOVEMENT, 345},
    {SCHED_MOVEMENT, 346},
    {SCHED_MOVEMENT, 347},
};
const unsigned g_glueRouteCount = sizeof g_glueRoutes / sizeof g_glueRoutes[0];
