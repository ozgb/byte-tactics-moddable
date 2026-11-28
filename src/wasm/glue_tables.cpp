#include <wasmtime.h>
#include "../units/unit_def.h"
#include "../../interfaces/host.h"
#include "../util/vec3.h"
#include "../util/angles.h"
#include "../../interfaces/wind_meteors.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/features.h"
#include "../../interfaces/ai.h"
#include "../../interfaces/pathfinding.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/targeting.h"
#include "../../interfaces/los_radar.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/units.h"
#include "../../interfaces/players.h"
#include "../../interfaces/map.h"
#include "../../interfaces/scripts.h"
#include "../../interfaces/movement.h"
#include "../../interfaces/mission.h"
#include "../../interfaces/network.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/rand_stream.h"
#include "../../interfaces/services.h"
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

void __cdecl GlueWindMeteorsUpdateWind(RandStream stream)
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)stream;
    GlueCall(glue, 0, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueWindMeteorsUpdateMeteors()
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 1, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GlueWindMeteorsApply(const WindMeteorsEventHeader* event, unsigned int size)
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

void __stdcall GlueWindMeteorsInitMeteors()
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 3, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GlueWindMeteorsEmptyShutdownPreCleanup()
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 4, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GlueWindMeteorsLoadMeteorDefaults(MeteorShowerParams* params)
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

void __stdcall GlueWindMeteorsSaveMeteors(HapiBank* file)
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueHandleOf((void*)file);
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueWindMeteorsLoadMeteors(HapiBank* file)
{
    GlueModule* glue = &s_windMeteorsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueHandleOf((void*)file);
    GlueCall(glue, 7, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __cdecl GlueWindMeteorsTimedUpdateWind(RandStream stream)
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->UpdateWind(stream);
    GlueTimed(&s_windMeteorsGlue, 0, start);
}

void __stdcall GlueWindMeteorsTimedUpdateMeteors()
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->UpdateMeteors();
    GlueTimed(&s_windMeteorsGlue, 1, start);
}

void __stdcall GlueWindMeteorsTimedApply(const WindMeteorsEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->Apply(event, size);
    GlueTimed(&s_windMeteorsGlue, 2, start);
}

void __stdcall GlueWindMeteorsTimedInitMeteors()
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->InitMeteors();
    GlueTimed(&s_windMeteorsGlue, 3, start);
}

void __stdcall GlueWindMeteorsTimedEmptyShutdownPreCleanup()
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->EmptyShutdownPreCleanup();
    GlueTimed(&s_windMeteorsGlue, 4, start);
}

void __stdcall GlueWindMeteorsTimedLoadMeteorDefaults(MeteorShowerParams* params)
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->LoadMeteorDefaults(params);
    GlueTimed(&s_windMeteorsGlue, 5, start);
}

void __stdcall GlueWindMeteorsTimedSaveMeteors(HapiBank* file)
{
    unsigned long long start = host_time_counter();
    s_windMeteorsNative->SaveMeteors(file);
    GlueTimed(&s_windMeteorsGlue, 6, start);
}

void __stdcall GlueWindMeteorsTimedLoadMeteors(HapiBank* file)
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
    s_windMeteorsSeam.UpdateWind = GlueWindMeteorsUpdateWind;
    s_windMeteorsSeam.UpdateMeteors = GlueWindMeteorsUpdateMeteors;
    s_windMeteorsSeam.Apply = GlueWindMeteorsApply;
    s_windMeteorsSeam.InitMeteors = GlueWindMeteorsInitMeteors;
    s_windMeteorsSeam.EmptyShutdownPreCleanup = GlueWindMeteorsEmptyShutdownPreCleanup;
    s_windMeteorsSeam.LoadMeteorDefaults = GlueWindMeteorsLoadMeteorDefaults;
    s_windMeteorsSeam.SaveMeteors = GlueWindMeteorsSaveMeteors;
    s_windMeteorsSeam.LoadMeteors = GlueWindMeteorsLoadMeteors;
    g_seams.wind_meteors = &s_windMeteorsSeam;
}

void WindMeteorsTimeNative()
{
    s_windMeteorsNative = g_seams.wind_meteors;
    s_windMeteorsTimed = *s_windMeteorsNative;
    s_windMeteorsTimed.UpdateWind = GlueWindMeteorsTimedUpdateWind;
    s_windMeteorsTimed.UpdateMeteors = GlueWindMeteorsTimedUpdateMeteors;
    s_windMeteorsTimed.Apply = GlueWindMeteorsTimedApply;
    s_windMeteorsTimed.InitMeteors = GlueWindMeteorsTimedInitMeteors;
    s_windMeteorsTimed.EmptyShutdownPreCleanup = GlueWindMeteorsTimedEmptyShutdownPreCleanup;
    s_windMeteorsTimed.LoadMeteorDefaults = GlueWindMeteorsTimedLoadMeteorDefaults;
    s_windMeteorsTimed.SaveMeteors = GlueWindMeteorsTimedSaveMeteors;
    s_windMeteorsTimed.LoadMeteors = GlueWindMeteorsTimedLoadMeteors;
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

unsigned int __stdcall GlueEconomyRegionSize(const EconMapInfo* map)
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

void __stdcall GlueEconomyInit(const EconMapInfo* map, const EconomyWorldView* world, void* region, unsigned int size)
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

void* __stdcall GlueEconomyShutdown()
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
    return (void*)GlueResult(glue, 2, (unsigned)slotArgs[0].i32, 1);
}

int __stdcall GlueEconomyApply(const EconomyEventHeader* event, unsigned int size)
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

int __stdcall GlueEconomySpend(const EconomySpend* spend)
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

void __stdcall GlueEconomyUpdatePlayerEconomy(int player)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueEconomyUpdateMetalExtraction(EconUnit unit)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)unit;
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueEconomyUpdateResourceSharing(int player)
{
    GlueModule* glue = &s_economyGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 7, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall GlueEconomyRequestGranted(EconUnit unit, unsigned int resources)
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

double __stdcall GlueEconomyGetEnergyUse(EconType type)
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

void __stdcall GlueEconomyUnitAccount(EconUnit unit, EconAccount* out)
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

unsigned int __stdcall GlueEconomyTimedRegionSize(const EconMapInfo* map)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_economyNative->RegionSize(map);
    GlueTimed(&s_economyGlue, 0, start);
    return result;
}

void __stdcall GlueEconomyTimedInit(const EconMapInfo* map, const EconomyWorldView* world, void* region, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_economyNative->Init(map, world, region, size);
    GlueTimed(&s_economyGlue, 1, start);
}

void* __stdcall GlueEconomyTimedShutdown()
{
    unsigned long long start = host_time_counter();
    void* result = s_economyNative->Shutdown();
    GlueTimed(&s_economyGlue, 2, start);
    return result;
}

int __stdcall GlueEconomyTimedApply(const EconomyEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_economyNative->Apply(event, size);
    GlueTimed(&s_economyGlue, 3, start);
    return result;
}

int __stdcall GlueEconomyTimedSpend(const EconomySpend* spend)
{
    unsigned long long start = host_time_counter();
    int result = s_economyNative->Spend(spend);
    GlueTimed(&s_economyGlue, 4, start);
    return result;
}

void __stdcall GlueEconomyTimedUpdatePlayerEconomy(int player)
{
    unsigned long long start = host_time_counter();
    s_economyNative->UpdatePlayerEconomy(player);
    GlueTimed(&s_economyGlue, 5, start);
}

void __stdcall GlueEconomyTimedUpdateMetalExtraction(EconUnit unit)
{
    unsigned long long start = host_time_counter();
    s_economyNative->UpdateMetalExtraction(unit);
    GlueTimed(&s_economyGlue, 6, start);
}

void __stdcall GlueEconomyTimedUpdateResourceSharing(int player)
{
    unsigned long long start = host_time_counter();
    s_economyNative->UpdateResourceSharing(player);
    GlueTimed(&s_economyGlue, 7, start);
}

int __stdcall GlueEconomyTimedRequestGranted(EconUnit unit, unsigned int resources)
{
    unsigned long long start = host_time_counter();
    int result = s_economyNative->RequestGranted(unit, resources);
    GlueTimed(&s_economyGlue, 8, start);
    return result;
}

double __stdcall GlueEconomyTimedGetEnergyUse(EconType type)
{
    unsigned long long start = host_time_counter();
    double result = s_economyNative->GetEnergyUse(type);
    GlueTimed(&s_economyGlue, 9, start);
    return result;
}

void __stdcall GlueEconomyTimedUnitAccount(EconUnit unit, EconAccount* out)
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
    s_economySeam.RegionSize = GlueEconomyRegionSize;
    s_economySeam.Init = GlueEconomyInit;
    s_economySeam.Shutdown = GlueEconomyShutdown;
    s_economySeam.Apply = GlueEconomyApply;
    s_economySeam.Spend = GlueEconomySpend;
    s_economySeam.UpdatePlayerEconomy = GlueEconomyUpdatePlayerEconomy;
    s_economySeam.UpdateMetalExtraction = GlueEconomyUpdateMetalExtraction;
    s_economySeam.UpdateResourceSharing = GlueEconomyUpdateResourceSharing;
    s_economySeam.RequestGranted = GlueEconomyRequestGranted;
    s_economySeam.GetEnergyUse = GlueEconomyGetEnergyUse;
    s_economySeam.UnitAccount = GlueEconomyUnitAccount;
    g_seams.economy = &s_economySeam;
}

void EconomyTimeNative()
{
    s_economyNative = g_seams.economy;
    s_economyTimed = *s_economyNative;
    s_economyTimed.RegionSize = GlueEconomyTimedRegionSize;
    s_economyTimed.Init = GlueEconomyTimedInit;
    s_economyTimed.Shutdown = GlueEconomyTimedShutdown;
    s_economyTimed.Apply = GlueEconomyTimedApply;
    s_economyTimed.Spend = GlueEconomyTimedSpend;
    s_economyTimed.UpdatePlayerEconomy = GlueEconomyTimedUpdatePlayerEconomy;
    s_economyTimed.UpdateMetalExtraction = GlueEconomyTimedUpdateMetalExtraction;
    s_economyTimed.UpdateResourceSharing = GlueEconomyTimedUpdateResourceSharing;
    s_economyTimed.RequestGranted = GlueEconomyTimedRequestGranted;
    s_economyTimed.GetEnergyUse = GlueEconomyTimedGetEnergyUse;
    s_economyTimed.UnitAccount = GlueEconomyTimedUnitAccount;
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

void __stdcall GlueFeaturesUpdateFeatures()
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 0, slotArgs, 0);
    GlueScratchRelease(mark);
}

unsigned short __stdcall GlueFeaturesGetCellFeature(Cell* cell)
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

void __stdcall GlueFeaturesGetFootprintCentre(const Point16* cell, Feature* def, Vec3* out)
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

int __stdcall GlueFeaturesApply(const FeaturesEventHeader* event, unsigned int size)
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

int __stdcall GlueFeaturesPlace(const FeaturesPlace* place)
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

int __stdcall GlueFeaturesFeatureTypeCount()
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall GlueFeaturesAnimateFeatureType(int type)
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)type;
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueFeaturesScanFeatureCell()
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 7, slotArgs, 0);
    GlueScratchRelease(mark);
}

int __stdcall GlueFeaturesFirstFeatureSpot()
{
    GlueModule* glue = &s_featuresGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 8, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall GlueFeaturesUpdateFeatureSpot(int spot)
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

unsigned short __stdcall GlueFeaturesFindFeatureAtPos(Vec3* pos, Point16* cell, Point16* size)
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

void __stdcall GlueFeaturesTimedUpdateFeatures()
{
    unsigned long long start = host_time_counter();
    s_featuresNative->UpdateFeatures();
    GlueTimed(&s_featuresGlue, 0, start);
}

unsigned short __stdcall GlueFeaturesTimedGetCellFeature(Cell* cell)
{
    unsigned long long start = host_time_counter();
    unsigned short result = s_featuresNative->GetCellFeature(cell);
    GlueTimed(&s_featuresGlue, 1, start);
    return result;
}

void __stdcall GlueFeaturesTimedGetFootprintCentre(const Point16* cell, Feature* def, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_featuresNative->GetFootprintCentre(cell, def, out);
    GlueTimed(&s_featuresGlue, 2, start);
}

int __stdcall GlueFeaturesTimedApply(const FeaturesEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->Apply(event, size);
    GlueTimed(&s_featuresGlue, 3, start);
    return result;
}

int __stdcall GlueFeaturesTimedPlace(const FeaturesPlace* place)
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->Place(place);
    GlueTimed(&s_featuresGlue, 4, start);
    return result;
}

int __stdcall GlueFeaturesTimedFeatureTypeCount()
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->FeatureTypeCount();
    GlueTimed(&s_featuresGlue, 5, start);
    return result;
}

void __stdcall GlueFeaturesTimedAnimateFeatureType(int type)
{
    unsigned long long start = host_time_counter();
    s_featuresNative->AnimateFeatureType(type);
    GlueTimed(&s_featuresGlue, 6, start);
}

void __stdcall GlueFeaturesTimedScanFeatureCell()
{
    unsigned long long start = host_time_counter();
    s_featuresNative->ScanFeatureCell();
    GlueTimed(&s_featuresGlue, 7, start);
}

int __stdcall GlueFeaturesTimedFirstFeatureSpot()
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->FirstFeatureSpot();
    GlueTimed(&s_featuresGlue, 8, start);
    return result;
}

int __stdcall GlueFeaturesTimedUpdateFeatureSpot(int spot)
{
    unsigned long long start = host_time_counter();
    int result = s_featuresNative->UpdateFeatureSpot(spot);
    GlueTimed(&s_featuresGlue, 9, start);
    return result;
}

unsigned short __stdcall GlueFeaturesTimedFindFeatureAtPos(Vec3* pos, Point16* cell, Point16* size)
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
    s_featuresSeam.UpdateFeatures = GlueFeaturesUpdateFeatures;
    s_featuresSeam.GetCellFeature = GlueFeaturesGetCellFeature;
    s_featuresSeam.GetFootprintCentre = GlueFeaturesGetFootprintCentre;
    s_featuresSeam.Apply = GlueFeaturesApply;
    s_featuresSeam.Place = GlueFeaturesPlace;
    s_featuresSeam.FeatureTypeCount = GlueFeaturesFeatureTypeCount;
    s_featuresSeam.AnimateFeatureType = GlueFeaturesAnimateFeatureType;
    s_featuresSeam.ScanFeatureCell = GlueFeaturesScanFeatureCell;
    s_featuresSeam.FirstFeatureSpot = GlueFeaturesFirstFeatureSpot;
    s_featuresSeam.UpdateFeatureSpot = GlueFeaturesUpdateFeatureSpot;
    s_featuresSeam.FindFeatureAtPos = GlueFeaturesFindFeatureAtPos;
    g_seams.features = &s_featuresSeam;
}

void FeaturesTimeNative()
{
    s_featuresNative = g_seams.features;
    s_featuresTimed = *s_featuresNative;
    s_featuresTimed.UpdateFeatures = GlueFeaturesTimedUpdateFeatures;
    s_featuresTimed.GetCellFeature = GlueFeaturesTimedGetCellFeature;
    s_featuresTimed.GetFootprintCentre = GlueFeaturesTimedGetFootprintCentre;
    s_featuresTimed.Apply = GlueFeaturesTimedApply;
    s_featuresTimed.Place = GlueFeaturesTimedPlace;
    s_featuresTimed.FeatureTypeCount = GlueFeaturesTimedFeatureTypeCount;
    s_featuresTimed.AnimateFeatureType = GlueFeaturesTimedAnimateFeatureType;
    s_featuresTimed.ScanFeatureCell = GlueFeaturesTimedScanFeatureCell;
    s_featuresTimed.FirstFeatureSpot = GlueFeaturesTimedFirstFeatureSpot;
    s_featuresTimed.UpdateFeatureSpot = GlueFeaturesTimedUpdateFeatureSpot;
    s_featuresTimed.FindFeatureAtPos = GlueFeaturesTimedFindFeatureAtPos;
    g_seams.features = &s_featuresTimed;
}

const GlueModuleDef kFeaturesDef = {"features", SCHED_FEATURES, kFeaturesSlots, 11, s_featuresFuncs, s_featuresTiming, FeaturesUseSeam, FeaturesTimeNative, 1, 0};
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

int __stdcall GlueAiApply(const AiEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(AiEventHeader), sizeof(AiReactToAttack)), sizeof(AiSetSquad)), sizeof(AiPlan)), sizeof(AiWeight)), sizeof(AiLimit)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(AiEventHeader), sizeof(AiReactToAttack)), sizeof(AiSetSquad)), sizeof(AiPlan)), sizeof(AiWeight)), sizeof(AiLimit)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall GlueAiDumpPlayerAI(int player, void* file)
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

void __stdcall GlueAiCreatePlayerAI(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueAiDestroyPlayerAI(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueAiUpdatePlayerAI(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueAiRebuildFeatureCells(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueAiCreateSquads(Player* player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 6, player);
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueAiFreeSquads(Player* player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 7, player);
    GlueCall(glue, 7, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueAiUpdateSquads(int player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 8, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueAiCreateSquadManager(Player* player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 9, player);
    GlueCall(glue, 9, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueAiFreeSquadManager(Player* player)
{
    GlueModule* glue = &s_aiGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 10, player);
    GlueCall(glue, 10, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall GlueAiTimedApply(const AiEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_aiNative->Apply(event, size);
    GlueTimed(&s_aiGlue, 0, start);
    return result;
}

void __stdcall GlueAiTimedDumpPlayerAI(int player, void* file)
{
    unsigned long long start = host_time_counter();
    s_aiNative->DumpPlayerAI(player, file);
    GlueTimed(&s_aiGlue, 1, start);
}

void __stdcall GlueAiTimedCreatePlayerAI(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->CreatePlayerAI(player);
    GlueTimed(&s_aiGlue, 2, start);
}

void __stdcall GlueAiTimedDestroyPlayerAI(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->DestroyPlayerAI(player);
    GlueTimed(&s_aiGlue, 3, start);
}

void __stdcall GlueAiTimedUpdatePlayerAI(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->UpdatePlayerAI(player);
    GlueTimed(&s_aiGlue, 4, start);
}

void __stdcall GlueAiTimedRebuildFeatureCells(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->RebuildFeatureCells(player);
    GlueTimed(&s_aiGlue, 5, start);
}

void __stdcall GlueAiTimedCreateSquads(Player* player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->CreateSquads(player);
    GlueTimed(&s_aiGlue, 6, start);
}

void __stdcall GlueAiTimedFreeSquads(Player* player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->FreeSquads(player);
    GlueTimed(&s_aiGlue, 7, start);
}

void __stdcall GlueAiTimedUpdateSquads(int player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->UpdateSquads(player);
    GlueTimed(&s_aiGlue, 8, start);
}

void __stdcall GlueAiTimedCreateSquadManager(Player* player)
{
    unsigned long long start = host_time_counter();
    s_aiNative->CreateSquadManager(player);
    GlueTimed(&s_aiGlue, 9, start);
}

void __stdcall GlueAiTimedFreeSquadManager(Player* player)
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
    s_aiSeam.Apply = GlueAiApply;
    s_aiSeam.DumpPlayerAI = GlueAiDumpPlayerAI;
    s_aiSeam.CreatePlayerAI = GlueAiCreatePlayerAI;
    s_aiSeam.DestroyPlayerAI = GlueAiDestroyPlayerAI;
    s_aiSeam.UpdatePlayerAI = GlueAiUpdatePlayerAI;
    s_aiSeam.RebuildFeatureCells = GlueAiRebuildFeatureCells;
    s_aiSeam.CreateSquads = GlueAiCreateSquads;
    s_aiSeam.FreeSquads = GlueAiFreeSquads;
    s_aiSeam.UpdateSquads = GlueAiUpdateSquads;
    s_aiSeam.CreateSquadManager = GlueAiCreateSquadManager;
    s_aiSeam.FreeSquadManager = GlueAiFreeSquadManager;
    g_seams.ai = &s_aiSeam;
}

void AiTimeNative()
{
    s_aiNative = g_seams.ai;
    s_aiTimed = *s_aiNative;
    s_aiTimed.Apply = GlueAiTimedApply;
    s_aiTimed.DumpPlayerAI = GlueAiTimedDumpPlayerAI;
    s_aiTimed.CreatePlayerAI = GlueAiTimedCreatePlayerAI;
    s_aiTimed.DestroyPlayerAI = GlueAiTimedDestroyPlayerAI;
    s_aiTimed.UpdatePlayerAI = GlueAiTimedUpdatePlayerAI;
    s_aiTimed.RebuildFeatureCells = GlueAiTimedRebuildFeatureCells;
    s_aiTimed.CreateSquads = GlueAiTimedCreateSquads;
    s_aiTimed.FreeSquads = GlueAiTimedFreeSquads;
    s_aiTimed.UpdateSquads = GlueAiTimedUpdateSquads;
    s_aiTimed.CreateSquadManager = GlueAiTimedCreateSquadManager;
    s_aiTimed.FreeSquadManager = GlueAiTimedFreeSquadManager;
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

unsigned int __stdcall GluePathfindingRegionSize(const PathMapInfo* map)
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

void __stdcall GluePathfindingInit(const PathMapInfo* map, void* region, unsigned int size)
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

void* __stdcall GluePathfindingShutdown()
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
    return (void*)GlueResult(glue, 2, (unsigned)slotArgs[0].i32, 1);
}

void __stdcall GluePathfindingSubmit(const PathRequest* request)
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

void __stdcall GluePathfindingCancel(PathHandle requester)
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)requester;
    GlueCall(glue, 4, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GluePathfindingCellsChanged(int left, int top, int right, int bottom, unsigned int classes, int cause)
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

void __stdcall GluePathfindingStep()
{
    GlueModule* glue = &s_pathfindingGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 6, slotArgs, 0);
    GlueScratchRelease(mark);
}

int __stdcall GluePathfindingPoll(PathfindingEventHeader* out, unsigned int size, PathPoint* points, int capacity)
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

unsigned int __stdcall GluePathfindingTimedRegionSize(const PathMapInfo* map)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_pathfindingNative->RegionSize(map);
    GlueTimed(&s_pathfindingGlue, 0, start);
    return result;
}

void __stdcall GluePathfindingTimedInit(const PathMapInfo* map, void* region, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->Init(map, region, size);
    GlueTimed(&s_pathfindingGlue, 1, start);
}

void* __stdcall GluePathfindingTimedShutdown()
{
    unsigned long long start = host_time_counter();
    void* result = s_pathfindingNative->Shutdown();
    GlueTimed(&s_pathfindingGlue, 2, start);
    return result;
}

void __stdcall GluePathfindingTimedSubmit(const PathRequest* request)
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->Submit(request);
    GlueTimed(&s_pathfindingGlue, 3, start);
}

void __stdcall GluePathfindingTimedCancel(PathHandle requester)
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->Cancel(requester);
    GlueTimed(&s_pathfindingGlue, 4, start);
}

void __stdcall GluePathfindingTimedCellsChanged(int left, int top, int right, int bottom, unsigned int classes, int cause)
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->CellsChanged(left, top, right, bottom, classes, cause);
    GlueTimed(&s_pathfindingGlue, 5, start);
}

void __stdcall GluePathfindingTimedStep()
{
    unsigned long long start = host_time_counter();
    s_pathfindingNative->Step();
    GlueTimed(&s_pathfindingGlue, 6, start);
}

int __stdcall GluePathfindingTimedPoll(PathfindingEventHeader* out, unsigned int size, PathPoint* points, int capacity)
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
    s_pathfindingSeam.RegionSize = GluePathfindingRegionSize;
    s_pathfindingSeam.Init = GluePathfindingInit;
    s_pathfindingSeam.Shutdown = GluePathfindingShutdown;
    s_pathfindingSeam.Submit = GluePathfindingSubmit;
    s_pathfindingSeam.Cancel = GluePathfindingCancel;
    s_pathfindingSeam.CellsChanged = GluePathfindingCellsChanged;
    s_pathfindingSeam.Step = GluePathfindingStep;
    s_pathfindingSeam.Poll = GluePathfindingPoll;
    g_seams.pathfinding = &s_pathfindingSeam;
}

void PathfindingTimeNative()
{
    s_pathfindingNative = g_seams.pathfinding;
    s_pathfindingTimed = *s_pathfindingNative;
    s_pathfindingTimed.RegionSize = GluePathfindingTimedRegionSize;
    s_pathfindingTimed.Init = GluePathfindingTimedInit;
    s_pathfindingTimed.Shutdown = GluePathfindingTimedShutdown;
    s_pathfindingTimed.Submit = GluePathfindingTimedSubmit;
    s_pathfindingTimed.Cancel = GluePathfindingTimedCancel;
    s_pathfindingTimed.CellsChanged = GluePathfindingTimedCellsChanged;
    s_pathfindingTimed.Step = GluePathfindingTimedStep;
    s_pathfindingTimed.Poll = GluePathfindingTimedPoll;
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

int __stdcall GlueWeaponsApply(const WeaponsEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(WeaponsEventHeader), sizeof(WeaponsDamage)), sizeof(WeaponsDamage)), sizeof(WeaponsDetonate)), sizeof(WeaponsRelease)), sizeof(WeaponsClaim)), sizeof(WeaponsAimDone)), sizeof(WeaponsExplodePiece)), sizeof(WeaponsAreaDamage)), sizeof(WeaponsSpawn)), sizeof(WeaponsSetStockpiles)), sizeof(WeaponsAddStockpile)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(WeaponsEventHeader), sizeof(WeaponsDamage)), sizeof(WeaponsDamage)), sizeof(WeaponsDetonate)), sizeof(WeaponsRelease)), sizeof(WeaponsClaim)), sizeof(WeaponsAimDone)), sizeof(WeaponsExplodePiece)), sizeof(WeaponsAreaDamage)), sizeof(WeaponsSpawn)), sizeof(WeaponsSetStockpiles)), sizeof(WeaponsAddStockpile)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall GlueWeaponsUpdateProjectiles()
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 1, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GlueWeaponsUpdateExplosions()
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 2, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GlueWeaponsUpdateUnitWeapons(Unit* unit)
{
    GlueModule* glue = &s_weaponsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 3, unit);
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
}

WeaponDef* __stdcall GlueWeaponsFindWeaponByName(const char* name)
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

int __stdcall GlueWeaponsGetWeaponRange(Unit* unit, unsigned int weapon)
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

int __stdcall GlueWeaponsWeaponCanReachUnit(Unit* shooter, Unit* target, unsigned char weapon)
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

Projectile* __stdcall GlueWeaponsFindTargetableProjectile(Unit* unit, unsigned int weapon)
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

int __stdcall GlueWeaponsWeaponCanReachPos(Unit* unit, Vec3* from, Vec3* to, int flags)
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

int __stdcall GlueWeaponsTimedApply(const WeaponsEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_weaponsNative->Apply(event, size);
    GlueTimed(&s_weaponsGlue, 0, start);
    return result;
}

void __stdcall GlueWeaponsTimedUpdateProjectiles()
{
    unsigned long long start = host_time_counter();
    s_weaponsNative->UpdateProjectiles();
    GlueTimed(&s_weaponsGlue, 1, start);
}

void __stdcall GlueWeaponsTimedUpdateExplosions()
{
    unsigned long long start = host_time_counter();
    s_weaponsNative->UpdateExplosions();
    GlueTimed(&s_weaponsGlue, 2, start);
}

void __stdcall GlueWeaponsTimedUpdateUnitWeapons(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_weaponsNative->UpdateUnitWeapons(unit);
    GlueTimed(&s_weaponsGlue, 3, start);
}

WeaponDef* __stdcall GlueWeaponsTimedFindWeaponByName(const char* name)
{
    unsigned long long start = host_time_counter();
    WeaponDef* result = s_weaponsNative->FindWeaponByName(name);
    GlueTimed(&s_weaponsGlue, 4, start);
    return result;
}

int __stdcall GlueWeaponsTimedGetWeaponRange(Unit* unit, unsigned int weapon)
{
    unsigned long long start = host_time_counter();
    int result = s_weaponsNative->GetWeaponRange(unit, weapon);
    GlueTimed(&s_weaponsGlue, 5, start);
    return result;
}

int __stdcall GlueWeaponsTimedWeaponCanReachUnit(Unit* shooter, Unit* target, unsigned char weapon)
{
    unsigned long long start = host_time_counter();
    int result = s_weaponsNative->WeaponCanReachUnit(shooter, target, weapon);
    GlueTimed(&s_weaponsGlue, 6, start);
    return result;
}

Projectile* __stdcall GlueWeaponsTimedFindTargetableProjectile(Unit* unit, unsigned int weapon)
{
    unsigned long long start = host_time_counter();
    Projectile* result = s_weaponsNative->FindTargetableProjectile(unit, weapon);
    GlueTimed(&s_weaponsGlue, 7, start);
    return result;
}

int __stdcall GlueWeaponsTimedWeaponCanReachPos(Unit* unit, Vec3* from, Vec3* to, int flags)
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
    s_weaponsSeam.Apply = GlueWeaponsApply;
    s_weaponsSeam.UpdateProjectiles = GlueWeaponsUpdateProjectiles;
    s_weaponsSeam.UpdateExplosions = GlueWeaponsUpdateExplosions;
    s_weaponsSeam.UpdateUnitWeapons = GlueWeaponsUpdateUnitWeapons;
    s_weaponsSeam.FindWeaponByName = GlueWeaponsFindWeaponByName;
    s_weaponsSeam.GetWeaponRange = GlueWeaponsGetWeaponRange;
    s_weaponsSeam.WeaponCanReachUnit = GlueWeaponsWeaponCanReachUnit;
    s_weaponsSeam.FindTargetableProjectile = GlueWeaponsFindTargetableProjectile;
    s_weaponsSeam.WeaponCanReachPos = GlueWeaponsWeaponCanReachPos;
    g_seams.weapons = &s_weaponsSeam;
}

void WeaponsTimeNative()
{
    s_weaponsNative = g_seams.weapons;
    s_weaponsTimed = *s_weaponsNative;
    s_weaponsTimed.Apply = GlueWeaponsTimedApply;
    s_weaponsTimed.UpdateProjectiles = GlueWeaponsTimedUpdateProjectiles;
    s_weaponsTimed.UpdateExplosions = GlueWeaponsTimedUpdateExplosions;
    s_weaponsTimed.UpdateUnitWeapons = GlueWeaponsTimedUpdateUnitWeapons;
    s_weaponsTimed.FindWeaponByName = GlueWeaponsTimedFindWeaponByName;
    s_weaponsTimed.GetWeaponRange = GlueWeaponsTimedGetWeaponRange;
    s_weaponsTimed.WeaponCanReachUnit = GlueWeaponsTimedWeaponCanReachUnit;
    s_weaponsTimed.FindTargetableProjectile = GlueWeaponsTimedFindTargetableProjectile;
    s_weaponsTimed.WeaponCanReachPos = GlueWeaponsTimedWeaponCanReachPos;
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

int __stdcall GlueTargetingApply(const TargetingEventHeader* event, unsigned int size)
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

Unit* __stdcall GlueTargetingGetWeaponTargetUnit(Unit* unit, int weapon)
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

int __stdcall GlueTargetingGetWeaponTargetPos(Unit* unit, Vec3* pos, int weapon)
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

Unit* __stdcall GlueTargetingFindWeaponTarget(Unit* unit, unsigned char weapon, int useRange)
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

Unit* __stdcall GlueTargetingFindBestTargetIfFireAtWill(Unit* unit)
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

unsigned char __stdcall GlueTargetingChooseWeapon(Unit* unit)
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

int __stdcall GlueTargetingSavedTargetPair(Unit* unit, int weapon)
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

int __stdcall GlueTargetingTimedApply(const TargetingEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_targetingNative->Apply(event, size);
    GlueTimed(&s_targetingGlue, 0, start);
    return result;
}

Unit* __stdcall GlueTargetingTimedGetWeaponTargetUnit(Unit* unit, int weapon)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_targetingNative->GetWeaponTargetUnit(unit, weapon);
    GlueTimed(&s_targetingGlue, 1, start);
    return result;
}

int __stdcall GlueTargetingTimedGetWeaponTargetPos(Unit* unit, Vec3* pos, int weapon)
{
    unsigned long long start = host_time_counter();
    int result = s_targetingNative->GetWeaponTargetPos(unit, pos, weapon);
    GlueTimed(&s_targetingGlue, 2, start);
    return result;
}

Unit* __stdcall GlueTargetingTimedFindWeaponTarget(Unit* unit, unsigned char weapon, int useRange)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_targetingNative->FindWeaponTarget(unit, weapon, useRange);
    GlueTimed(&s_targetingGlue, 3, start);
    return result;
}

Unit* __stdcall GlueTargetingTimedFindBestTargetIfFireAtWill(Unit* unit)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_targetingNative->FindBestTargetIfFireAtWill(unit);
    GlueTimed(&s_targetingGlue, 4, start);
    return result;
}

unsigned char __stdcall GlueTargetingTimedChooseWeapon(Unit* unit)
{
    unsigned long long start = host_time_counter();
    unsigned char result = s_targetingNative->ChooseWeapon(unit);
    GlueTimed(&s_targetingGlue, 5, start);
    return result;
}

int __stdcall GlueTargetingTimedSavedTargetPair(Unit* unit, int weapon)
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
    s_targetingSeam.Apply = GlueTargetingApply;
    s_targetingSeam.GetWeaponTargetUnit = GlueTargetingGetWeaponTargetUnit;
    s_targetingSeam.GetWeaponTargetPos = GlueTargetingGetWeaponTargetPos;
    s_targetingSeam.FindWeaponTarget = GlueTargetingFindWeaponTarget;
    s_targetingSeam.FindBestTargetIfFireAtWill = GlueTargetingFindBestTargetIfFireAtWill;
    s_targetingSeam.ChooseWeapon = GlueTargetingChooseWeapon;
    s_targetingSeam.SavedTargetPair = GlueTargetingSavedTargetPair;
    g_seams.targeting = &s_targetingSeam;
}

void TargetingTimeNative()
{
    s_targetingNative = g_seams.targeting;
    s_targetingTimed = *s_targetingNative;
    s_targetingTimed.Apply = GlueTargetingTimedApply;
    s_targetingTimed.GetWeaponTargetUnit = GlueTargetingTimedGetWeaponTargetUnit;
    s_targetingTimed.GetWeaponTargetPos = GlueTargetingTimedGetWeaponTargetPos;
    s_targetingTimed.FindWeaponTarget = GlueTargetingTimedFindWeaponTarget;
    s_targetingTimed.FindBestTargetIfFireAtWill = GlueTargetingTimedFindBestTargetIfFireAtWill;
    s_targetingTimed.ChooseWeapon = GlueTargetingTimedChooseWeapon;
    s_targetingTimed.SavedTargetPair = GlueTargetingTimedSavedTargetPair;
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

int __stdcall GlueLosRadarApply(const LosRadarEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(LosRadarEventHeader), sizeof(LosRadarUnit)), sizeof(LosRadarUnit)), sizeof(LosRadarUnit)), sizeof(LosRadarAddEyeball)), sizeof(LosRadarCellHeights)), sizeof(LosRadarRecalculate)), sizeof(LosRadarSharePlayer)), sizeof(LosRadarSharePlayer)), sizeof(LosRadarShareMap)), sizeof(LosRadarUnit)), sizeof(LosRadarSightLoad)), sizeof(LosRadarSetMode)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(LosRadarEventHeader), sizeof(LosRadarUnit)), sizeof(LosRadarUnit)), sizeof(LosRadarUnit)), sizeof(LosRadarAddEyeball)), sizeof(LosRadarCellHeights)), sizeof(LosRadarRecalculate)), sizeof(LosRadarSharePlayer)), sizeof(LosRadarSharePlayer)), sizeof(LosRadarShareMap)), sizeof(LosRadarUnit)), sizeof(LosRadarSightLoad)), sizeof(LosRadarSetMode)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

Cell* __stdcall GlueLosRadarGetMapCell(int x, int y)
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

Cell* __stdcall GlueLosRadarGetMapCellAtPosition(Vec3* pos)
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

Cell* __stdcall GlueLosRadarGetOriginCellAtPosition(Vec3* pos)
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

void __stdcall GlueLosRadarClampWorldPosToTerrain(int x, int y, Vec3* out)
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

int __stdcall GlueLosRadarIsUnitVisibleToPlayer(Player* player, Unit* unit)
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

unsigned int __stdcall GlueLosRadarSoundHeardMask(const Vec3* at)
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

void __stdcall GlueLosRadarUpdateUnitLineOfSight(Unit* unit)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 7, unit);
    GlueCall(glue, 7, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueLosRadarRecordRadarUnits()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 8, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GlueLosRadarUpdateSensorRadarAndCloak()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 9, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GlueLosRadarRecordRadarMapped()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 10, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GlueLosRadarExpireEyeballs()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 11, slotArgs, 0);
    GlueScratchRelease(mark);
}

int __stdcall GlueLosRadarGetVisibleEnemiesInRadius(int player, const Vec3* pos, int radius, int flags, Ref<Unit>* out, int capacity)
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

int __stdcall GlueLosRadarHasReadyUnitInRange(int player, const Vec3* pos, int range)
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

int __stdcall GlueLosRadarIsPointVisible(Player* player, Vec3* pos)
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

int __stdcall GlueLosRadarGetSeenUnitsInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
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

void __stdcall GlueLosRadarCreateUnitIndex(int player)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 16, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueLosRadarFreeUnitIndex(int player)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 17, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueLosRadarRefreshUnitIndex(int player)
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 18, slotArgs, 1);
    GlueScratchRelease(mark);
}

unsigned int __stdcall GlueLosRadarRegionSize(const LosRadarMapInfo* map)
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

void __stdcall GlueLosRadarInit(const LosRadarMapInfo* map, void* region, unsigned int size)
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

void* __stdcall GlueLosRadarShutdown()
{
    GlueModule* glue = &s_losRadarGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 21, slotArgs, 1);
    GlueScratchRelease(mark);
    return (void*)GlueResult(glue, 21, (unsigned)slotArgs[0].i32, 1);
}

int __stdcall GlueLosRadarTimedApply(const LosRadarEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->Apply(event, size);
    GlueTimed(&s_losRadarGlue, 0, start);
    return result;
}

Cell* __stdcall GlueLosRadarTimedGetMapCell(int x, int y)
{
    unsigned long long start = host_time_counter();
    Cell* result = s_losRadarNative->GetMapCell(x, y);
    GlueTimed(&s_losRadarGlue, 1, start);
    return result;
}

Cell* __stdcall GlueLosRadarTimedGetMapCellAtPosition(Vec3* pos)
{
    unsigned long long start = host_time_counter();
    Cell* result = s_losRadarNative->GetMapCellAtPosition(pos);
    GlueTimed(&s_losRadarGlue, 2, start);
    return result;
}

Cell* __stdcall GlueLosRadarTimedGetOriginCellAtPosition(Vec3* pos)
{
    unsigned long long start = host_time_counter();
    Cell* result = s_losRadarNative->GetOriginCellAtPosition(pos);
    GlueTimed(&s_losRadarGlue, 3, start);
    return result;
}

void __stdcall GlueLosRadarTimedClampWorldPosToTerrain(int x, int y, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->ClampWorldPosToTerrain(x, y, out);
    GlueTimed(&s_losRadarGlue, 4, start);
}

int __stdcall GlueLosRadarTimedIsUnitVisibleToPlayer(Player* player, Unit* unit)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->IsUnitVisibleToPlayer(player, unit);
    GlueTimed(&s_losRadarGlue, 5, start);
    return result;
}

unsigned int __stdcall GlueLosRadarTimedSoundHeardMask(const Vec3* at)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_losRadarNative->SoundHeardMask(at);
    GlueTimed(&s_losRadarGlue, 6, start);
    return result;
}

void __stdcall GlueLosRadarTimedUpdateUnitLineOfSight(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->UpdateUnitLineOfSight(unit);
    GlueTimed(&s_losRadarGlue, 7, start);
}

void __stdcall GlueLosRadarTimedRecordRadarUnits()
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->RecordRadarUnits();
    GlueTimed(&s_losRadarGlue, 8, start);
}

void __stdcall GlueLosRadarTimedUpdateSensorRadarAndCloak()
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->UpdateSensorRadarAndCloak();
    GlueTimed(&s_losRadarGlue, 9, start);
}

void __stdcall GlueLosRadarTimedRecordRadarMapped()
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->RecordRadarMapped();
    GlueTimed(&s_losRadarGlue, 10, start);
}

void __stdcall GlueLosRadarTimedExpireEyeballs()
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->ExpireEyeballs();
    GlueTimed(&s_losRadarGlue, 11, start);
}

int __stdcall GlueLosRadarTimedGetVisibleEnemiesInRadius(int player, const Vec3* pos, int radius, int flags, Ref<Unit>* out, int capacity)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->GetVisibleEnemiesInRadius(player, pos, radius, flags, out, capacity);
    GlueTimed(&s_losRadarGlue, 12, start);
    return result;
}

int __stdcall GlueLosRadarTimedHasReadyUnitInRange(int player, const Vec3* pos, int range)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->HasReadyUnitInRange(player, pos, range);
    GlueTimed(&s_losRadarGlue, 13, start);
    return result;
}

int __stdcall GlueLosRadarTimedIsPointVisible(Player* player, Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->IsPointVisible(player, pos);
    GlueTimed(&s_losRadarGlue, 14, start);
    return result;
}

int __stdcall GlueLosRadarTimedGetSeenUnitsInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
{
    unsigned long long start = host_time_counter();
    int result = s_losRadarNative->GetSeenUnitsInRadius(player, pos, radius, out, capacity);
    GlueTimed(&s_losRadarGlue, 15, start);
    return result;
}

void __stdcall GlueLosRadarTimedCreateUnitIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->CreateUnitIndex(player);
    GlueTimed(&s_losRadarGlue, 16, start);
}

void __stdcall GlueLosRadarTimedFreeUnitIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->FreeUnitIndex(player);
    GlueTimed(&s_losRadarGlue, 17, start);
}

void __stdcall GlueLosRadarTimedRefreshUnitIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->RefreshUnitIndex(player);
    GlueTimed(&s_losRadarGlue, 18, start);
}

unsigned int __stdcall GlueLosRadarTimedRegionSize(const LosRadarMapInfo* map)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_losRadarNative->RegionSize(map);
    GlueTimed(&s_losRadarGlue, 19, start);
    return result;
}

void __stdcall GlueLosRadarTimedInit(const LosRadarMapInfo* map, void* region, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_losRadarNative->Init(map, region, size);
    GlueTimed(&s_losRadarGlue, 20, start);
}

void* __stdcall GlueLosRadarTimedShutdown()
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
    s_losRadarSeam.Apply = GlueLosRadarApply;
    s_losRadarSeam.GetMapCell = GlueLosRadarGetMapCell;
    s_losRadarSeam.GetMapCellAtPosition = GlueLosRadarGetMapCellAtPosition;
    s_losRadarSeam.GetOriginCellAtPosition = GlueLosRadarGetOriginCellAtPosition;
    s_losRadarSeam.ClampWorldPosToTerrain = GlueLosRadarClampWorldPosToTerrain;
    s_losRadarSeam.IsUnitVisibleToPlayer = GlueLosRadarIsUnitVisibleToPlayer;
    s_losRadarSeam.SoundHeardMask = GlueLosRadarSoundHeardMask;
    s_losRadarSeam.UpdateUnitLineOfSight = GlueLosRadarUpdateUnitLineOfSight;
    s_losRadarSeam.RecordRadarUnits = GlueLosRadarRecordRadarUnits;
    s_losRadarSeam.UpdateSensorRadarAndCloak = GlueLosRadarUpdateSensorRadarAndCloak;
    s_losRadarSeam.RecordRadarMapped = GlueLosRadarRecordRadarMapped;
    s_losRadarSeam.ExpireEyeballs = GlueLosRadarExpireEyeballs;
    s_losRadarSeam.GetVisibleEnemiesInRadius = GlueLosRadarGetVisibleEnemiesInRadius;
    s_losRadarSeam.HasReadyUnitInRange = GlueLosRadarHasReadyUnitInRange;
    s_losRadarSeam.IsPointVisible = GlueLosRadarIsPointVisible;
    s_losRadarSeam.GetSeenUnitsInRadius = GlueLosRadarGetSeenUnitsInRadius;
    s_losRadarSeam.CreateUnitIndex = GlueLosRadarCreateUnitIndex;
    s_losRadarSeam.FreeUnitIndex = GlueLosRadarFreeUnitIndex;
    s_losRadarSeam.RefreshUnitIndex = GlueLosRadarRefreshUnitIndex;
    s_losRadarSeam.RegionSize = GlueLosRadarRegionSize;
    s_losRadarSeam.Init = GlueLosRadarInit;
    s_losRadarSeam.Shutdown = GlueLosRadarShutdown;
    g_seams.los_radar = &s_losRadarSeam;
}

void LosRadarTimeNative()
{
    s_losRadarNative = g_seams.los_radar;
    s_losRadarTimed = *s_losRadarNative;
    s_losRadarTimed.Apply = GlueLosRadarTimedApply;
    s_losRadarTimed.GetMapCell = GlueLosRadarTimedGetMapCell;
    s_losRadarTimed.GetMapCellAtPosition = GlueLosRadarTimedGetMapCellAtPosition;
    s_losRadarTimed.GetOriginCellAtPosition = GlueLosRadarTimedGetOriginCellAtPosition;
    s_losRadarTimed.ClampWorldPosToTerrain = GlueLosRadarTimedClampWorldPosToTerrain;
    s_losRadarTimed.IsUnitVisibleToPlayer = GlueLosRadarTimedIsUnitVisibleToPlayer;
    s_losRadarTimed.SoundHeardMask = GlueLosRadarTimedSoundHeardMask;
    s_losRadarTimed.UpdateUnitLineOfSight = GlueLosRadarTimedUpdateUnitLineOfSight;
    s_losRadarTimed.RecordRadarUnits = GlueLosRadarTimedRecordRadarUnits;
    s_losRadarTimed.UpdateSensorRadarAndCloak = GlueLosRadarTimedUpdateSensorRadarAndCloak;
    s_losRadarTimed.RecordRadarMapped = GlueLosRadarTimedRecordRadarMapped;
    s_losRadarTimed.ExpireEyeballs = GlueLosRadarTimedExpireEyeballs;
    s_losRadarTimed.GetVisibleEnemiesInRadius = GlueLosRadarTimedGetVisibleEnemiesInRadius;
    s_losRadarTimed.HasReadyUnitInRange = GlueLosRadarTimedHasReadyUnitInRange;
    s_losRadarTimed.IsPointVisible = GlueLosRadarTimedIsPointVisible;
    s_losRadarTimed.GetSeenUnitsInRadius = GlueLosRadarTimedGetSeenUnitsInRadius;
    s_losRadarTimed.CreateUnitIndex = GlueLosRadarTimedCreateUnitIndex;
    s_losRadarTimed.FreeUnitIndex = GlueLosRadarTimedFreeUnitIndex;
    s_losRadarTimed.RefreshUnitIndex = GlueLosRadarTimedRefreshUnitIndex;
    s_losRadarTimed.RegionSize = GlueLosRadarTimedRegionSize;
    s_losRadarTimed.Init = GlueLosRadarTimedInit;
    s_losRadarTimed.Shutdown = GlueLosRadarTimedShutdown;
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

int __stdcall GlueOrdersApply(const OrdersEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(OrdersEventHeader), sizeof(OrdersAdd)), sizeof(OrdersQueueBuild)), sizeof(OrdersStop)), sizeof(OrdersStatusFlags)), sizeof(OrdersAttacked)), sizeof(OrdersParalyze)), sizeof(OrdersAdd)), sizeof(OrdersCancel)), sizeof(OrdersGroupCommand)), sizeof(OrdersGroupCommand)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(OrdersEventHeader), sizeof(OrdersAdd)), sizeof(OrdersQueueBuild)), sizeof(OrdersStop)), sizeof(OrdersStatusFlags)), sizeof(OrdersAttacked)), sizeof(OrdersParalyze)), sizeof(OrdersAdd)), sizeof(OrdersCancel)), sizeof(OrdersGroupCommand)), sizeof(OrdersGroupCommand)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall GlueOrdersRunOrders(Unit* unit)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 1, unit);
    GlueCall(glue, 1, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueOrdersRunSecondaryOrders(Unit* unit)
{
    GlueModule* glue = &s_ordersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 2, unit);
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall GlueOrdersGetOrderFlags(Unit* unit)
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

int __stdcall GlueOrdersGetOrderType(unsigned char mode, Unit* unit, Unit* target, Vec3* pos)
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

int __stdcall GlueOrdersMissionTypeFromName(const char* name)
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

int __stdcall GlueOrdersExpandGroupCommand(const OrdersGroupCommand* command, unsigned int size)
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

int __stdcall GlueOrdersTimedApply(const OrdersEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_ordersNative->Apply(event, size);
    GlueTimed(&s_ordersGlue, 0, start);
    return result;
}

void __stdcall GlueOrdersTimedRunOrders(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_ordersNative->RunOrders(unit);
    GlueTimed(&s_ordersGlue, 1, start);
}

void __stdcall GlueOrdersTimedRunSecondaryOrders(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_ordersNative->RunSecondaryOrders(unit);
    GlueTimed(&s_ordersGlue, 2, start);
}

int __stdcall GlueOrdersTimedGetOrderFlags(Unit* unit)
{
    unsigned long long start = host_time_counter();
    int result = s_ordersNative->GetOrderFlags(unit);
    GlueTimed(&s_ordersGlue, 3, start);
    return result;
}

int __stdcall GlueOrdersTimedGetOrderType(unsigned char mode, Unit* unit, Unit* target, Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_ordersNative->GetOrderType(mode, unit, target, pos);
    GlueTimed(&s_ordersGlue, 4, start);
    return result;
}

int __stdcall GlueOrdersTimedMissionTypeFromName(const char* name)
{
    unsigned long long start = host_time_counter();
    int result = s_ordersNative->MissionTypeFromName(name);
    GlueTimed(&s_ordersGlue, 5, start);
    return result;
}

int __stdcall GlueOrdersTimedExpandGroupCommand(const OrdersGroupCommand* command, unsigned int size)
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
    s_ordersSeam.Apply = GlueOrdersApply;
    s_ordersSeam.RunOrders = GlueOrdersRunOrders;
    s_ordersSeam.RunSecondaryOrders = GlueOrdersRunSecondaryOrders;
    s_ordersSeam.GetOrderFlags = GlueOrdersGetOrderFlags;
    s_ordersSeam.GetOrderType = GlueOrdersGetOrderType;
    s_ordersSeam.MissionTypeFromName = GlueOrdersMissionTypeFromName;
    s_ordersSeam.ExpandGroupCommand = GlueOrdersExpandGroupCommand;
    g_seams.orders = &s_ordersSeam;
}

void OrdersTimeNative()
{
    s_ordersNative = g_seams.orders;
    s_ordersTimed = *s_ordersNative;
    s_ordersTimed.Apply = GlueOrdersTimedApply;
    s_ordersTimed.RunOrders = GlueOrdersTimedRunOrders;
    s_ordersTimed.RunSecondaryOrders = GlueOrdersTimedRunSecondaryOrders;
    s_ordersTimed.GetOrderFlags = GlueOrdersTimedGetOrderFlags;
    s_ordersTimed.GetOrderType = GlueOrdersTimedGetOrderType;
    s_ordersTimed.MissionTypeFromName = GlueOrdersTimedMissionTypeFromName;
    s_ordersTimed.ExpandGroupCommand = GlueOrdersTimedExpandGroupCommand;
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

int __stdcall GlueUnitsApply(const UnitsEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(UnitsEventHeader), sizeof(UnitsSetStateBits)), sizeof(UnitsSetFlags)), sizeof(UnitsSetNetDirty)), sizeof(UnitsSetWorkTime)), sizeof(UnitsSetPosition)), sizeof(UnitsAttachToPiece)), sizeof(UnitsFinishConstruction)), sizeof(UnitsBuildDecay)), sizeof(UnitsRepair)), sizeof(UnitsUnlink)), sizeof(UnitsSetStance)), sizeof(UnitsSetAngles)), sizeof(UnitsSetBuildState)), sizeof(UnitsTurn)), sizeof(UnitsMove)), sizeof(UnitsChangeCell)), sizeof(UnitsSetMoved)), sizeof(UnitsScriptValue)), sizeof(UnitsNotifyRefs)), sizeof(UnitsDamageRecord)), sizeof(UnitsSetHealth)), sizeof(UnitsKill)), sizeof(UnitsSend)), sizeof(UnitsCapture)), sizeof(UnitsCreate)), sizeof(UnitsReloadType)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(UnitsEventHeader), sizeof(UnitsSetStateBits)), sizeof(UnitsSetFlags)), sizeof(UnitsSetNetDirty)), sizeof(UnitsSetWorkTime)), sizeof(UnitsSetPosition)), sizeof(UnitsAttachToPiece)), sizeof(UnitsFinishConstruction)), sizeof(UnitsBuildDecay)), sizeof(UnitsRepair)), sizeof(UnitsUnlink)), sizeof(UnitsSetStance)), sizeof(UnitsSetAngles)), sizeof(UnitsSetBuildState)), sizeof(UnitsTurn)), sizeof(UnitsMove)), sizeof(UnitsChangeCell)), sizeof(UnitsSetMoved)), sizeof(UnitsScriptValue)), sizeof(UnitsNotifyRefs)), sizeof(UnitsDamageRecord)), sizeof(UnitsSetHealth)), sizeof(UnitsKill)), sizeof(UnitsSend)), sizeof(UnitsCapture)), sizeof(UnitsCreate)), sizeof(UnitsReloadType)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall GlueUnitsUpdateAllUnits()
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 1, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GlueUnitsKillUnit(Unit* unit, int cause)
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

Unit* __stdcall GlueUnitsCreateUnit(unsigned char player, unsigned short typeId, const Vec3* pos, int finished, int mode, int id)
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

UnitDef* __stdcall GlueUnitsFindUnitType(const char* name)
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

unsigned short __stdcall GlueUnitsFindUnitTypeId(const char* name)
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

int __stdcall GlueUnitsAddBuildProgress(Unit* builder, Unit* unit, float amount)
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

int __stdcall GlueUnitsAddRepairProgress(Unit* builder, Unit* unit, float amount)
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

int __stdcall GlueUnitsCanLoad(Unit* unit, Unit* target)
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

int __stdcall GlueUnitsCanReclaim(Unit* unit, Unit* target)
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

int __stdcall GlueUnitsCanRepair(Unit* unit, Unit* target)
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

void __stdcall GlueUnitsUnitRefUnlink(UnitRef* ref)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 11, ref);
    GlueCall(glue, 11, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueUnitsUpdateUnitHead(Unit* unit)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 12, unit);
    GlueCall(glue, 12, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall GlueUnitsUpdateUnitBody(Unit* unit)
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

void __stdcall GlueUnitsUpdateUnitTail(Unit* unit, int moved)
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

int __stdcall GlueUnitsGetFactoriesInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
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

void __stdcall GlueUnitsCreateFactoryIndex(int player)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 16, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueUnitsFreeFactoryIndex(int player)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 17, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueUnitsRefreshFactoryIndex(int player)
{
    GlueModule* glue = &s_unitsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 18, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueUnitsUnitRefInit(UnitRef* ref, Unit* unit, OrderBase* value)
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

void __stdcall GlueUnitsUnitRefSetUnit(UnitRef* ref, Unit* unit)
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

Unit* __stdcall GlueUnitsUnitRefGet(UnitRef* ref)
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

void __stdcall GlueUnitsUnitRefSetValue(UnitRef* ref, OrderBase* value)
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

int __stdcall GlueUnitsTimedApply(const UnitsEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->Apply(event, size);
    GlueTimed(&s_unitsGlue, 0, start);
    return result;
}

void __stdcall GlueUnitsTimedUpdateAllUnits()
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UpdateAllUnits();
    GlueTimed(&s_unitsGlue, 1, start);
}

void __stdcall GlueUnitsTimedKillUnit(Unit* unit, int cause)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->KillUnit(unit, cause);
    GlueTimed(&s_unitsGlue, 2, start);
}

Unit* __stdcall GlueUnitsTimedCreateUnit(unsigned char player, unsigned short typeId, const Vec3* pos, int finished, int mode, int id)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_unitsNative->CreateUnit(player, typeId, pos, finished, mode, id);
    GlueTimed(&s_unitsGlue, 3, start);
    return result;
}

UnitDef* __stdcall GlueUnitsTimedFindUnitType(const char* name)
{
    unsigned long long start = host_time_counter();
    UnitDef* result = s_unitsNative->FindUnitType(name);
    GlueTimed(&s_unitsGlue, 4, start);
    return result;
}

unsigned short __stdcall GlueUnitsTimedFindUnitTypeId(const char* name)
{
    unsigned long long start = host_time_counter();
    unsigned short result = s_unitsNative->FindUnitTypeId(name);
    GlueTimed(&s_unitsGlue, 5, start);
    return result;
}

int __stdcall GlueUnitsTimedAddBuildProgress(Unit* builder, Unit* unit, float amount)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->AddBuildProgress(builder, unit, amount);
    GlueTimed(&s_unitsGlue, 6, start);
    return result;
}

int __stdcall GlueUnitsTimedAddRepairProgress(Unit* builder, Unit* unit, float amount)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->AddRepairProgress(builder, unit, amount);
    GlueTimed(&s_unitsGlue, 7, start);
    return result;
}

int __stdcall GlueUnitsTimedCanLoad(Unit* unit, Unit* target)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->CanLoad(unit, target);
    GlueTimed(&s_unitsGlue, 8, start);
    return result;
}

int __stdcall GlueUnitsTimedCanReclaim(Unit* unit, Unit* target)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->CanReclaim(unit, target);
    GlueTimed(&s_unitsGlue, 9, start);
    return result;
}

int __stdcall GlueUnitsTimedCanRepair(Unit* unit, Unit* target)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->CanRepair(unit, target);
    GlueTimed(&s_unitsGlue, 10, start);
    return result;
}

void __stdcall GlueUnitsTimedUnitRefUnlink(UnitRef* ref)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UnitRefUnlink(ref);
    GlueTimed(&s_unitsGlue, 11, start);
}

void __stdcall GlueUnitsTimedUpdateUnitHead(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UpdateUnitHead(unit);
    GlueTimed(&s_unitsGlue, 12, start);
}

int __stdcall GlueUnitsTimedUpdateUnitBody(Unit* unit)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->UpdateUnitBody(unit);
    GlueTimed(&s_unitsGlue, 13, start);
    return result;
}

void __stdcall GlueUnitsTimedUpdateUnitTail(Unit* unit, int moved)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UpdateUnitTail(unit, moved);
    GlueTimed(&s_unitsGlue, 14, start);
}

int __stdcall GlueUnitsTimedGetFactoriesInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
{
    unsigned long long start = host_time_counter();
    int result = s_unitsNative->GetFactoriesInRadius(player, pos, radius, out, capacity);
    GlueTimed(&s_unitsGlue, 15, start);
    return result;
}

void __stdcall GlueUnitsTimedCreateFactoryIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->CreateFactoryIndex(player);
    GlueTimed(&s_unitsGlue, 16, start);
}

void __stdcall GlueUnitsTimedFreeFactoryIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->FreeFactoryIndex(player);
    GlueTimed(&s_unitsGlue, 17, start);
}

void __stdcall GlueUnitsTimedRefreshFactoryIndex(int player)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->RefreshFactoryIndex(player);
    GlueTimed(&s_unitsGlue, 18, start);
}

void __stdcall GlueUnitsTimedUnitRefInit(UnitRef* ref, Unit* unit, OrderBase* value)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UnitRefInit(ref, unit, value);
    GlueTimed(&s_unitsGlue, 19, start);
}

void __stdcall GlueUnitsTimedUnitRefSetUnit(UnitRef* ref, Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_unitsNative->UnitRefSetUnit(ref, unit);
    GlueTimed(&s_unitsGlue, 20, start);
}

Unit* __stdcall GlueUnitsTimedUnitRefGet(UnitRef* ref)
{
    unsigned long long start = host_time_counter();
    Unit* result = s_unitsNative->UnitRefGet(ref);
    GlueTimed(&s_unitsGlue, 21, start);
    return result;
}

void __stdcall GlueUnitsTimedUnitRefSetValue(UnitRef* ref, OrderBase* value)
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
    s_unitsSeam.Apply = GlueUnitsApply;
    s_unitsSeam.UpdateAllUnits = GlueUnitsUpdateAllUnits;
    s_unitsSeam.KillUnit = GlueUnitsKillUnit;
    s_unitsSeam.CreateUnit = GlueUnitsCreateUnit;
    s_unitsSeam.FindUnitType = GlueUnitsFindUnitType;
    s_unitsSeam.FindUnitTypeId = GlueUnitsFindUnitTypeId;
    s_unitsSeam.AddBuildProgress = GlueUnitsAddBuildProgress;
    s_unitsSeam.AddRepairProgress = GlueUnitsAddRepairProgress;
    s_unitsSeam.CanLoad = GlueUnitsCanLoad;
    s_unitsSeam.CanReclaim = GlueUnitsCanReclaim;
    s_unitsSeam.CanRepair = GlueUnitsCanRepair;
    s_unitsSeam.UnitRefUnlink = GlueUnitsUnitRefUnlink;
    s_unitsSeam.UpdateUnitHead = GlueUnitsUpdateUnitHead;
    s_unitsSeam.UpdateUnitBody = GlueUnitsUpdateUnitBody;
    s_unitsSeam.UpdateUnitTail = GlueUnitsUpdateUnitTail;
    s_unitsSeam.GetFactoriesInRadius = GlueUnitsGetFactoriesInRadius;
    s_unitsSeam.CreateFactoryIndex = GlueUnitsCreateFactoryIndex;
    s_unitsSeam.FreeFactoryIndex = GlueUnitsFreeFactoryIndex;
    s_unitsSeam.RefreshFactoryIndex = GlueUnitsRefreshFactoryIndex;
    s_unitsSeam.UnitRefInit = GlueUnitsUnitRefInit;
    s_unitsSeam.UnitRefSetUnit = GlueUnitsUnitRefSetUnit;
    s_unitsSeam.UnitRefGet = GlueUnitsUnitRefGet;
    s_unitsSeam.UnitRefSetValue = GlueUnitsUnitRefSetValue;
    g_seams.units = &s_unitsSeam;
}

void UnitsTimeNative()
{
    s_unitsNative = g_seams.units;
    s_unitsTimed = *s_unitsNative;
    s_unitsTimed.Apply = GlueUnitsTimedApply;
    s_unitsTimed.UpdateAllUnits = GlueUnitsTimedUpdateAllUnits;
    s_unitsTimed.KillUnit = GlueUnitsTimedKillUnit;
    s_unitsTimed.CreateUnit = GlueUnitsTimedCreateUnit;
    s_unitsTimed.FindUnitType = GlueUnitsTimedFindUnitType;
    s_unitsTimed.FindUnitTypeId = GlueUnitsTimedFindUnitTypeId;
    s_unitsTimed.AddBuildProgress = GlueUnitsTimedAddBuildProgress;
    s_unitsTimed.AddRepairProgress = GlueUnitsTimedAddRepairProgress;
    s_unitsTimed.CanLoad = GlueUnitsTimedCanLoad;
    s_unitsTimed.CanReclaim = GlueUnitsTimedCanReclaim;
    s_unitsTimed.CanRepair = GlueUnitsTimedCanRepair;
    s_unitsTimed.UnitRefUnlink = GlueUnitsTimedUnitRefUnlink;
    s_unitsTimed.UpdateUnitHead = GlueUnitsTimedUpdateUnitHead;
    s_unitsTimed.UpdateUnitBody = GlueUnitsTimedUpdateUnitBody;
    s_unitsTimed.UpdateUnitTail = GlueUnitsTimedUpdateUnitTail;
    s_unitsTimed.GetFactoriesInRadius = GlueUnitsTimedGetFactoriesInRadius;
    s_unitsTimed.CreateFactoryIndex = GlueUnitsTimedCreateFactoryIndex;
    s_unitsTimed.FreeFactoryIndex = GlueUnitsTimedFreeFactoryIndex;
    s_unitsTimed.RefreshFactoryIndex = GlueUnitsTimedRefreshFactoryIndex;
    s_unitsTimed.UnitRefInit = GlueUnitsTimedUnitRefInit;
    s_unitsTimed.UnitRefSetUnit = GlueUnitsTimedUnitRefSetUnit;
    s_unitsTimed.UnitRefGet = GlueUnitsTimedUnitRefGet;
    s_unitsTimed.UnitRefSetValue = GlueUnitsTimedUnitRefSetValue;
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

int __stdcall GluePlayersApply(const PlayersEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(PlayersEventHeader), sizeof(PlayersGiveUnit)), sizeof(PlayersKillUnits)), sizeof(PlayersTransfer)), sizeof(PlayersSetStore)), sizeof(PlayersSetType)), sizeof(PlayersEndGame)), sizeof(PlayersSetCheats)), sizeof(PlayersSetPlayer)), sizeof(PlayersSetPlayer)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(PlayersEventHeader), sizeof(PlayersGiveUnit)), sizeof(PlayersKillUnits)), sizeof(PlayersTransfer)), sizeof(PlayersSetStore)), sizeof(PlayersSetType)), sizeof(PlayersEndGame)), sizeof(PlayersSetCheats)), sizeof(PlayersSetPlayer)), sizeof(PlayersSetPlayer)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall GluePlayersUpdatePlayers()
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 1, slotArgs, 0);
    GlueScratchRelease(mark);
}

void __stdcall GluePlayersDeactivateConditions(MissionConditions* conditions)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 2, conditions);
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GluePlayersUpdatePlayersOnStream(RandStream stream)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)stream;
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall GluePlayersUpdatePlayerHead(int player)
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

int __stdcall GluePlayersUpdatePlayerBody(int player, RandStream stream)
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

void __stdcall GluePlayersUpdatePlayerTail(int player)
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)player;
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GluePlayersUpdatePlayersFinish()
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 7, slotArgs, 0);
    GlueScratchRelease(mark);
}

unsigned int __stdcall GluePlayersRegionSize(const PlayersMapInfo* info)
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

void __stdcall GluePlayersInit(const PlayersMapInfo* info, void* region, unsigned int size)
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

void* __stdcall GluePlayersShutdown()
{
    GlueModule* glue = &s_playersGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 10, slotArgs, 1);
    GlueScratchRelease(mark);
    return (void*)GlueResult(glue, 10, (unsigned)slotArgs[0].i32, 1);
}

int __stdcall GluePlayersTimedApply(const PlayersEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_playersNative->Apply(event, size);
    GlueTimed(&s_playersGlue, 0, start);
    return result;
}

void __stdcall GluePlayersTimedUpdatePlayers()
{
    unsigned long long start = host_time_counter();
    s_playersNative->UpdatePlayers();
    GlueTimed(&s_playersGlue, 1, start);
}

void __stdcall GluePlayersTimedDeactivateConditions(MissionConditions* conditions)
{
    unsigned long long start = host_time_counter();
    s_playersNative->DeactivateConditions(conditions);
    GlueTimed(&s_playersGlue, 2, start);
}

void __stdcall GluePlayersTimedUpdatePlayersOnStream(RandStream stream)
{
    unsigned long long start = host_time_counter();
    s_playersNative->UpdatePlayersOnStream(stream);
    GlueTimed(&s_playersGlue, 3, start);
}

int __stdcall GluePlayersTimedUpdatePlayerHead(int player)
{
    unsigned long long start = host_time_counter();
    int result = s_playersNative->UpdatePlayerHead(player);
    GlueTimed(&s_playersGlue, 4, start);
    return result;
}

int __stdcall GluePlayersTimedUpdatePlayerBody(int player, RandStream stream)
{
    unsigned long long start = host_time_counter();
    int result = s_playersNative->UpdatePlayerBody(player, stream);
    GlueTimed(&s_playersGlue, 5, start);
    return result;
}

void __stdcall GluePlayersTimedUpdatePlayerTail(int player)
{
    unsigned long long start = host_time_counter();
    s_playersNative->UpdatePlayerTail(player);
    GlueTimed(&s_playersGlue, 6, start);
}

void __stdcall GluePlayersTimedUpdatePlayersFinish()
{
    unsigned long long start = host_time_counter();
    s_playersNative->UpdatePlayersFinish();
    GlueTimed(&s_playersGlue, 7, start);
}

unsigned int __stdcall GluePlayersTimedRegionSize(const PlayersMapInfo* info)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_playersNative->RegionSize(info);
    GlueTimed(&s_playersGlue, 8, start);
    return result;
}

void __stdcall GluePlayersTimedInit(const PlayersMapInfo* info, void* region, unsigned int size)
{
    unsigned long long start = host_time_counter();
    s_playersNative->Init(info, region, size);
    GlueTimed(&s_playersGlue, 9, start);
}

void* __stdcall GluePlayersTimedShutdown()
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
    s_playersSeam.Apply = GluePlayersApply;
    s_playersSeam.UpdatePlayers = GluePlayersUpdatePlayers;
    s_playersSeam.DeactivateConditions = GluePlayersDeactivateConditions;
    s_playersSeam.UpdatePlayersOnStream = GluePlayersUpdatePlayersOnStream;
    s_playersSeam.UpdatePlayerHead = GluePlayersUpdatePlayerHead;
    s_playersSeam.UpdatePlayerBody = GluePlayersUpdatePlayerBody;
    s_playersSeam.UpdatePlayerTail = GluePlayersUpdatePlayerTail;
    s_playersSeam.UpdatePlayersFinish = GluePlayersUpdatePlayersFinish;
    s_playersSeam.RegionSize = GluePlayersRegionSize;
    s_playersSeam.Init = GluePlayersInit;
    s_playersSeam.Shutdown = GluePlayersShutdown;
    g_seams.players = &s_playersSeam;
}

void PlayersTimeNative()
{
    s_playersNative = g_seams.players;
    s_playersTimed = *s_playersNative;
    s_playersTimed.Apply = GluePlayersTimedApply;
    s_playersTimed.UpdatePlayers = GluePlayersTimedUpdatePlayers;
    s_playersTimed.DeactivateConditions = GluePlayersTimedDeactivateConditions;
    s_playersTimed.UpdatePlayersOnStream = GluePlayersTimedUpdatePlayersOnStream;
    s_playersTimed.UpdatePlayerHead = GluePlayersTimedUpdatePlayerHead;
    s_playersTimed.UpdatePlayerBody = GluePlayersTimedUpdatePlayerBody;
    s_playersTimed.UpdatePlayerTail = GluePlayersTimedUpdatePlayerTail;
    s_playersTimed.UpdatePlayersFinish = GluePlayersTimedUpdatePlayersFinish;
    s_playersTimed.RegionSize = GluePlayersTimedRegionSize;
    s_playersTimed.Init = GluePlayersTimedInit;
    s_playersTimed.Shutdown = GluePlayersTimedShutdown;
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

int __stdcall GlueMapApply(const MapEventHeader* event, unsigned int size)
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

int __stdcall GlueMapUnitsInRange(const Vec3* pos, int range, int skip, unsigned short* ids, int capacity)
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

int __stdcall GlueMapGetCellMeanHeight(Vec3* pos)
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

int __stdcall GlueMapCanBuildAt(UnitDef* unit, const Point16* cell, short type, Player* player)
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

int __stdcall GlueMapCanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, const Point16* cell, int flags)
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

void __stdcall GlueMapBuildAllPassMaps(const MapMoveLimits* limits, int count)
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

void __stdcall GlueMapSnapWorldPosToFootprint(UnitDef* unit, Vec3* pos)
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

int __stdcall GlueMapCanPlaceFootprintAt(Unit* unit, Vec3* pos)
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

int __stdcall GlueMapIsPadSlotFree(Unit* pad, int id)
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

int __stdcall GlueMapPickRandomReclaimableResourcesInRadius(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount)
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

int __stdcall GlueMapGetGroundHeight(Vec3* pos)
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

int __stdcall GlueMapPassMapGet(int moveClass, int x, int y)
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

int __stdcall GlueMapPassMapInBounds(int moveClass, unsigned int x, unsigned int y)
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

void __stdcall GlueMapRefreshMovedUnits(int moveClass, Unit* unit)
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

void __stdcall GlueMapRefreshUnitIfStale(int moveClass, Unit* unit)
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

int __stdcall GlueMapTimedApply(const MapEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->Apply(event, size);
    GlueTimed(&s_mapGlue, 0, start);
    return result;
}

int __stdcall GlueMapTimedUnitsInRange(const Vec3* pos, int range, int skip, unsigned short* ids, int capacity)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->UnitsInRange(pos, range, skip, ids, capacity);
    GlueTimed(&s_mapGlue, 1, start);
    return result;
}

int __stdcall GlueMapTimedGetCellMeanHeight(Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->GetCellMeanHeight(pos);
    GlueTimed(&s_mapGlue, 2, start);
    return result;
}

int __stdcall GlueMapTimedCanBuildAt(UnitDef* unit, const Point16* cell, short type, Player* player)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->CanBuildAt(unit, cell, type, player);
    GlueTimed(&s_mapGlue, 3, start);
    return result;
}

int __stdcall GlueMapTimedCanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, const Point16* cell, int flags)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->CanPlaceUnitFootprint(unit, otherTypeId, cell, flags);
    GlueTimed(&s_mapGlue, 4, start);
    return result;
}

void __stdcall GlueMapTimedBuildAllPassMaps(const MapMoveLimits* limits, int count)
{
    unsigned long long start = host_time_counter();
    s_mapNative->BuildAllPassMaps(limits, count);
    GlueTimed(&s_mapGlue, 5, start);
}

void __stdcall GlueMapTimedSnapWorldPosToFootprint(UnitDef* unit, Vec3* pos)
{
    unsigned long long start = host_time_counter();
    s_mapNative->SnapWorldPosToFootprint(unit, pos);
    GlueTimed(&s_mapGlue, 6, start);
}

int __stdcall GlueMapTimedCanPlaceFootprintAt(Unit* unit, Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->CanPlaceFootprintAt(unit, pos);
    GlueTimed(&s_mapGlue, 7, start);
    return result;
}

int __stdcall GlueMapTimedIsPadSlotFree(Unit* pad, int id)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->IsPadSlotFree(pad, id);
    GlueTimed(&s_mapGlue, 8, start);
    return result;
}

int __stdcall GlueMapTimedPickRandomReclaimableResourcesInRadius(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->PickRandomReclaimableResourcesInRadius(pos, range, energy, energyAmount, metal, metalAmount);
    GlueTimed(&s_mapGlue, 9, start);
    return result;
}

int __stdcall GlueMapTimedGetGroundHeight(Vec3* pos)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->GetGroundHeight(pos);
    GlueTimed(&s_mapGlue, 10, start);
    return result;
}

int __stdcall GlueMapTimedPassMapGet(int moveClass, int x, int y)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->PassMapGet(moveClass, x, y);
    GlueTimed(&s_mapGlue, 11, start);
    return result;
}

int __stdcall GlueMapTimedPassMapInBounds(int moveClass, unsigned int x, unsigned int y)
{
    unsigned long long start = host_time_counter();
    int result = s_mapNative->PassMapInBounds(moveClass, x, y);
    GlueTimed(&s_mapGlue, 12, start);
    return result;
}

void __stdcall GlueMapTimedRefreshMovedUnits(int moveClass, Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_mapNative->RefreshMovedUnits(moveClass, unit);
    GlueTimed(&s_mapGlue, 13, start);
}

void __stdcall GlueMapTimedRefreshUnitIfStale(int moveClass, Unit* unit)
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
    s_mapSeam.Apply = GlueMapApply;
    s_mapSeam.UnitsInRange = GlueMapUnitsInRange;
    s_mapSeam.GetCellMeanHeight = GlueMapGetCellMeanHeight;
    s_mapSeam.CanBuildAt = GlueMapCanBuildAt;
    s_mapSeam.CanPlaceUnitFootprint = GlueMapCanPlaceUnitFootprint;
    s_mapSeam.BuildAllPassMaps = GlueMapBuildAllPassMaps;
    s_mapSeam.SnapWorldPosToFootprint = GlueMapSnapWorldPosToFootprint;
    s_mapSeam.CanPlaceFootprintAt = GlueMapCanPlaceFootprintAt;
    s_mapSeam.IsPadSlotFree = GlueMapIsPadSlotFree;
    s_mapSeam.PickRandomReclaimableResourcesInRadius = GlueMapPickRandomReclaimableResourcesInRadius;
    s_mapSeam.GetGroundHeight = GlueMapGetGroundHeight;
    s_mapSeam.PassMapGet = GlueMapPassMapGet;
    s_mapSeam.PassMapInBounds = GlueMapPassMapInBounds;
    s_mapSeam.RefreshMovedUnits = GlueMapRefreshMovedUnits;
    s_mapSeam.RefreshUnitIfStale = GlueMapRefreshUnitIfStale;
    g_seams.map = &s_mapSeam;
}

void MapTimeNative()
{
    s_mapNative = g_seams.map;
    s_mapTimed = *s_mapNative;
    s_mapTimed.Apply = GlueMapTimedApply;
    s_mapTimed.UnitsInRange = GlueMapTimedUnitsInRange;
    s_mapTimed.GetCellMeanHeight = GlueMapTimedGetCellMeanHeight;
    s_mapTimed.CanBuildAt = GlueMapTimedCanBuildAt;
    s_mapTimed.CanPlaceUnitFootprint = GlueMapTimedCanPlaceUnitFootprint;
    s_mapTimed.BuildAllPassMaps = GlueMapTimedBuildAllPassMaps;
    s_mapTimed.SnapWorldPosToFootprint = GlueMapTimedSnapWorldPosToFootprint;
    s_mapTimed.CanPlaceFootprintAt = GlueMapTimedCanPlaceFootprintAt;
    s_mapTimed.IsPadSlotFree = GlueMapTimedIsPadSlotFree;
    s_mapTimed.PickRandomReclaimableResourcesInRadius = GlueMapTimedPickRandomReclaimableResourcesInRadius;
    s_mapTimed.GetGroundHeight = GlueMapTimedGetGroundHeight;
    s_mapTimed.PassMapGet = GlueMapTimedPassMapGet;
    s_mapTimed.PassMapInBounds = GlueMapTimedPassMapInBounds;
    s_mapTimed.RefreshMovedUnits = GlueMapTimedRefreshMovedUnits;
    s_mapTimed.RefreshUnitIfStale = GlueMapTimedRefreshUnitIfStale;
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

int __stdcall GlueScriptsApply(const ScriptsEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(sizeof(ScriptsEventHeader), sizeof(ScriptsStart)), sizeof(ScriptsDetachPiece)), sizeof(ScriptsState)), sizeof(ScriptsState)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(sizeof(ScriptsEventHeader), sizeof(ScriptsStart)), sizeof(ScriptsDetachPiece)), sizeof(ScriptsState)), sizeof(ScriptsState)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall GlueScriptsStartScriptWithArgsByIndex(CobScript* script, int index, Callback* callback, int update, int count, int a, int b, int c, int d)
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

int __stdcall GlueScriptsQueryScript(CobScript* script, const char* name, int* arg0, int* arg1, int* arg2, int* arg3)
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

int __stdcall GlueScriptsFindScript(CobScript* script, const char* name)
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

void __stdcall GlueScriptsGetPieceOffset(Unit* unit, int piece, Vec3* out)
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

void __stdcall GlueScriptsGetPiecePosition(Unit* unit, int piece, Vec3* out)
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

void __stdcall GlueScriptsGetPieceAngles(Unit* unit, int piece, Angles16* out)
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

void __stdcall GlueScriptsGetNanoPiecePosition(Unit* unit, Vec3* out)
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

void __stdcall GlueScriptsGetAimFromPosition(Unit* unit, Vec3* out, unsigned char weapon)
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

void __stdcall GlueScriptsGetSweetSpot(Unit* unit, Vec3* out)
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

void __stdcall GlueScriptsPoseUnitModel(Unit* unit)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 10, unit);
    GlueCall(glue, 10, slotArgs, 1);
    GlueScratchRelease(mark);
}

ModelState* __stdcall GlueScriptsCreateObjectState(Object3do* model)
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

void __stdcall GlueScriptsFreeObjectState(ModelState* state)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 12, state);
    GlueCall(glue, 12, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueScriptsUpdateObjectState(UnitDrawView* unit)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 13, unit);
    GlueCall(glue, 13, slotArgs, 1);
    GlueScratchRelease(mark);
}

unsigned int __stdcall GlueScriptsObjectStateSize(Object3do* model)
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

void __stdcall GlueScriptsNewUnitScript(Unit* unit)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 15, unit);
    GlueCall(glue, 15, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueScriptsSetCob(CobScript* script, ScriptTable* data)
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

ModelState* __stdcall GlueScriptsCreatePlayerObjectState(Object3do* model, ScriptTable* data, Unit* owner)
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

void __stdcall GlueScriptsSetObjectState(UnitScript* script, ModelState* state)
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

void __stdcall GlueScriptsRunScripts(CobScript* script, int elapsed)
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

void __stdcall GlueScriptsDeleteScript(CobScript* script)
{
    GlueModule* glue = &s_scriptsGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 20, script);
    GlueCall(glue, 20, slotArgs, 1);
    GlueScratchRelease(mark);
}

int __stdcall GlueScriptsTimedApply(const ScriptsEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_scriptsNative->Apply(event, size);
    GlueTimed(&s_scriptsGlue, 0, start);
    return result;
}

int __stdcall GlueScriptsTimedStartScriptWithArgsByIndex(CobScript* script, int index, Callback* callback, int update, int count, int a, int b, int c, int d)
{
    unsigned long long start = host_time_counter();
    int result = s_scriptsNative->StartScriptWithArgsByIndex(script, index, callback, update, count, a, b, c, d);
    GlueTimed(&s_scriptsGlue, 1, start);
    return result;
}

int __stdcall GlueScriptsTimedQueryScript(CobScript* script, const char* name, int* arg0, int* arg1, int* arg2, int* arg3)
{
    unsigned long long start = host_time_counter();
    int result = s_scriptsNative->QueryScript(script, name, arg0, arg1, arg2, arg3);
    GlueTimed(&s_scriptsGlue, 2, start);
    return result;
}

int __stdcall GlueScriptsTimedFindScript(CobScript* script, const char* name)
{
    unsigned long long start = host_time_counter();
    int result = s_scriptsNative->FindScript(script, name);
    GlueTimed(&s_scriptsGlue, 3, start);
    return result;
}

void __stdcall GlueScriptsTimedGetPieceOffset(Unit* unit, int piece, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetPieceOffset(unit, piece, out);
    GlueTimed(&s_scriptsGlue, 4, start);
}

void __stdcall GlueScriptsTimedGetPiecePosition(Unit* unit, int piece, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetPiecePosition(unit, piece, out);
    GlueTimed(&s_scriptsGlue, 5, start);
}

void __stdcall GlueScriptsTimedGetPieceAngles(Unit* unit, int piece, Angles16* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetPieceAngles(unit, piece, out);
    GlueTimed(&s_scriptsGlue, 6, start);
}

void __stdcall GlueScriptsTimedGetNanoPiecePosition(Unit* unit, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetNanoPiecePosition(unit, out);
    GlueTimed(&s_scriptsGlue, 7, start);
}

void __stdcall GlueScriptsTimedGetAimFromPosition(Unit* unit, Vec3* out, unsigned char weapon)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetAimFromPosition(unit, out, weapon);
    GlueTimed(&s_scriptsGlue, 8, start);
}

void __stdcall GlueScriptsTimedGetSweetSpot(Unit* unit, Vec3* out)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->GetSweetSpot(unit, out);
    GlueTimed(&s_scriptsGlue, 9, start);
}

void __stdcall GlueScriptsTimedPoseUnitModel(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->PoseUnitModel(unit);
    GlueTimed(&s_scriptsGlue, 10, start);
}

ModelState* __stdcall GlueScriptsTimedCreateObjectState(Object3do* model)
{
    unsigned long long start = host_time_counter();
    ModelState* result = s_scriptsNative->CreateObjectState(model);
    GlueTimed(&s_scriptsGlue, 11, start);
    return result;
}

void __stdcall GlueScriptsTimedFreeObjectState(ModelState* state)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->FreeObjectState(state);
    GlueTimed(&s_scriptsGlue, 12, start);
}

void __stdcall GlueScriptsTimedUpdateObjectState(UnitDrawView* unit)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->UpdateObjectState(unit);
    GlueTimed(&s_scriptsGlue, 13, start);
}

unsigned int __stdcall GlueScriptsTimedObjectStateSize(Object3do* model)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_scriptsNative->ObjectStateSize(model);
    GlueTimed(&s_scriptsGlue, 14, start);
    return result;
}

void __stdcall GlueScriptsTimedNewUnitScript(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->NewUnitScript(unit);
    GlueTimed(&s_scriptsGlue, 15, start);
}

void __stdcall GlueScriptsTimedSetCob(CobScript* script, ScriptTable* data)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->SetCob(script, data);
    GlueTimed(&s_scriptsGlue, 16, start);
}

ModelState* __stdcall GlueScriptsTimedCreatePlayerObjectState(Object3do* model, ScriptTable* data, Unit* owner)
{
    unsigned long long start = host_time_counter();
    ModelState* result = s_scriptsNative->CreatePlayerObjectState(model, data, owner);
    GlueTimed(&s_scriptsGlue, 17, start);
    return result;
}

void __stdcall GlueScriptsTimedSetObjectState(UnitScript* script, ModelState* state)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->SetObjectState(script, state);
    GlueTimed(&s_scriptsGlue, 18, start);
}

void __stdcall GlueScriptsTimedRunScripts(CobScript* script, int elapsed)
{
    unsigned long long start = host_time_counter();
    s_scriptsNative->RunScripts(script, elapsed);
    GlueTimed(&s_scriptsGlue, 19, start);
}

void __stdcall GlueScriptsTimedDeleteScript(CobScript* script)
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
    s_scriptsSeam.Apply = GlueScriptsApply;
    s_scriptsSeam.StartScriptWithArgsByIndex = GlueScriptsStartScriptWithArgsByIndex;
    s_scriptsSeam.QueryScript = GlueScriptsQueryScript;
    s_scriptsSeam.FindScript = GlueScriptsFindScript;
    s_scriptsSeam.GetPieceOffset = GlueScriptsGetPieceOffset;
    s_scriptsSeam.GetPiecePosition = GlueScriptsGetPiecePosition;
    s_scriptsSeam.GetPieceAngles = GlueScriptsGetPieceAngles;
    s_scriptsSeam.GetNanoPiecePosition = GlueScriptsGetNanoPiecePosition;
    s_scriptsSeam.GetAimFromPosition = GlueScriptsGetAimFromPosition;
    s_scriptsSeam.GetSweetSpot = GlueScriptsGetSweetSpot;
    s_scriptsSeam.PoseUnitModel = GlueScriptsPoseUnitModel;
    s_scriptsSeam.CreateObjectState = GlueScriptsCreateObjectState;
    s_scriptsSeam.FreeObjectState = GlueScriptsFreeObjectState;
    s_scriptsSeam.UpdateObjectState = GlueScriptsUpdateObjectState;
    s_scriptsSeam.ObjectStateSize = GlueScriptsObjectStateSize;
    s_scriptsSeam.NewUnitScript = GlueScriptsNewUnitScript;
    s_scriptsSeam.SetCob = GlueScriptsSetCob;
    s_scriptsSeam.CreatePlayerObjectState = GlueScriptsCreatePlayerObjectState;
    s_scriptsSeam.SetObjectState = GlueScriptsSetObjectState;
    s_scriptsSeam.RunScripts = GlueScriptsRunScripts;
    s_scriptsSeam.DeleteScript = GlueScriptsDeleteScript;
    g_seams.scripts = &s_scriptsSeam;
}

void ScriptsTimeNative()
{
    s_scriptsNative = g_seams.scripts;
    s_scriptsTimed = *s_scriptsNative;
    s_scriptsTimed.Apply = GlueScriptsTimedApply;
    s_scriptsTimed.StartScriptWithArgsByIndex = GlueScriptsTimedStartScriptWithArgsByIndex;
    s_scriptsTimed.QueryScript = GlueScriptsTimedQueryScript;
    s_scriptsTimed.FindScript = GlueScriptsTimedFindScript;
    s_scriptsTimed.GetPieceOffset = GlueScriptsTimedGetPieceOffset;
    s_scriptsTimed.GetPiecePosition = GlueScriptsTimedGetPiecePosition;
    s_scriptsTimed.GetPieceAngles = GlueScriptsTimedGetPieceAngles;
    s_scriptsTimed.GetNanoPiecePosition = GlueScriptsTimedGetNanoPiecePosition;
    s_scriptsTimed.GetAimFromPosition = GlueScriptsTimedGetAimFromPosition;
    s_scriptsTimed.GetSweetSpot = GlueScriptsTimedGetSweetSpot;
    s_scriptsTimed.PoseUnitModel = GlueScriptsTimedPoseUnitModel;
    s_scriptsTimed.CreateObjectState = GlueScriptsTimedCreateObjectState;
    s_scriptsTimed.FreeObjectState = GlueScriptsTimedFreeObjectState;
    s_scriptsTimed.UpdateObjectState = GlueScriptsTimedUpdateObjectState;
    s_scriptsTimed.ObjectStateSize = GlueScriptsTimedObjectStateSize;
    s_scriptsTimed.NewUnitScript = GlueScriptsTimedNewUnitScript;
    s_scriptsTimed.SetCob = GlueScriptsTimedSetCob;
    s_scriptsTimed.CreatePlayerObjectState = GlueScriptsTimedCreatePlayerObjectState;
    s_scriptsTimed.SetObjectState = GlueScriptsTimedSetObjectState;
    s_scriptsTimed.RunScripts = GlueScriptsTimedRunScripts;
    s_scriptsTimed.DeleteScript = GlueScriptsTimedDeleteScript;
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

int __stdcall GlueMovementApply(const MovementEventHeader* event, unsigned int size)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[2];
    slotArgs[0].i64 = 0;
    unsigned eventSize = size > GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(MovementEventHeader), sizeof(MovementLoadState)), sizeof(MovementSetMode)), sizeof(MovementSetSpeed)), sizeof(MovementTarget)), sizeof(MovementTarget)), sizeof(MovementSetMode)), sizeof(MovementTargetValue)), sizeof(MovementTargetValue)), sizeof(MovementTargetValue)) ? (unsigned)GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(GlueMax(sizeof(MovementEventHeader), sizeof(MovementLoadState)), sizeof(MovementSetMode)), sizeof(MovementSetSpeed)), sizeof(MovementTarget)), sizeof(MovementTarget)), sizeof(MovementSetMode)), sizeof(MovementTargetValue)), sizeof(MovementTargetValue)), sizeof(MovementTargetValue)) : (unsigned)size;
    slotArgs[0].i32 = (int)GlueIn(glue, 0, event, eventSize, 1);
    slotArgs[1].i32 = (int)eventSize;
    GlueCall(glue, 0, slotArgs, 2);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall GlueMovementUpdateMotion(Unit* unit)
{
    GlueModule* glue = &s_movementGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    slotArgs[0].i32 = (int)GlueObject(glue, 1, unit);
    GlueCall(glue, 1, slotArgs, 1);
    GlueScratchRelease(mark);
}

void __stdcall GlueMovementApplyPathEvent(const PathfindingEventHeader* event, unsigned int size, const PathPoint* waypoints)
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

int __stdcall GlueMovementHasNetState(Unit* unit)
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

unsigned int __stdcall GlueMovementNewPathOrderAt(unsigned int order, const Vec3* pos)
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

unsigned int __stdcall GlueMovementNewPathOrderToUnit(unsigned int order, Unit* unit)
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

unsigned int __stdcall GlueMovementNewPathOrderToUnitAt(unsigned int order, Unit* unit, const Vec3* pos)
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

unsigned int __stdcall GlueMovementNewPathOrderToUnitAtAngle(unsigned int order, Unit* unit, short angle)
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

unsigned int __stdcall GlueMovementNewAirManeuverOrder(unsigned int order, const Vec3* from, const Vec3* to)
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

unsigned int __stdcall GlueMovementNewApproachRadius(unsigned int order, int x, int y, int radius)
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

unsigned int __stdcall GlueMovementNewRingApproach(unsigned int order, int x, int y, int inner, int outer)
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

unsigned int __stdcall GlueMovementNewPointMarker(unsigned int order, const Point16* pos, const Point16* size)
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

int __stdcall GlueMovementTargetType(unsigned int target)
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

unsigned int __stdcall GlueMovementUnitTarget(Unit* unit)
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

int __stdcall GlueMovementTimedApply(const MovementEventHeader* event, unsigned int size)
{
    unsigned long long start = host_time_counter();
    int result = s_movementNative->Apply(event, size);
    GlueTimed(&s_movementGlue, 0, start);
    return result;
}

void __stdcall GlueMovementTimedUpdateMotion(Unit* unit)
{
    unsigned long long start = host_time_counter();
    s_movementNative->UpdateMotion(unit);
    GlueTimed(&s_movementGlue, 1, start);
}

void __stdcall GlueMovementTimedApplyPathEvent(const PathfindingEventHeader* event, unsigned int size, const PathPoint* waypoints)
{
    unsigned long long start = host_time_counter();
    s_movementNative->ApplyPathEvent(event, size, waypoints);
    GlueTimed(&s_movementGlue, 2, start);
}

int __stdcall GlueMovementTimedHasNetState(Unit* unit)
{
    unsigned long long start = host_time_counter();
    int result = s_movementNative->HasNetState(unit);
    GlueTimed(&s_movementGlue, 3, start);
    return result;
}

unsigned int __stdcall GlueMovementTimedNewPathOrderAt(unsigned int order, const Vec3* pos)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPathOrderAt(order, pos);
    GlueTimed(&s_movementGlue, 4, start);
    return result;
}

unsigned int __stdcall GlueMovementTimedNewPathOrderToUnit(unsigned int order, Unit* unit)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPathOrderToUnit(order, unit);
    GlueTimed(&s_movementGlue, 5, start);
    return result;
}

unsigned int __stdcall GlueMovementTimedNewPathOrderToUnitAt(unsigned int order, Unit* unit, const Vec3* pos)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPathOrderToUnitAt(order, unit, pos);
    GlueTimed(&s_movementGlue, 6, start);
    return result;
}

unsigned int __stdcall GlueMovementTimedNewPathOrderToUnitAtAngle(unsigned int order, Unit* unit, short angle)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPathOrderToUnitAtAngle(order, unit, angle);
    GlueTimed(&s_movementGlue, 7, start);
    return result;
}

unsigned int __stdcall GlueMovementTimedNewAirManeuverOrder(unsigned int order, const Vec3* from, const Vec3* to)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewAirManeuverOrder(order, from, to);
    GlueTimed(&s_movementGlue, 8, start);
    return result;
}

unsigned int __stdcall GlueMovementTimedNewApproachRadius(unsigned int order, int x, int y, int radius)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewApproachRadius(order, x, y, radius);
    GlueTimed(&s_movementGlue, 9, start);
    return result;
}

unsigned int __stdcall GlueMovementTimedNewRingApproach(unsigned int order, int x, int y, int inner, int outer)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewRingApproach(order, x, y, inner, outer);
    GlueTimed(&s_movementGlue, 10, start);
    return result;
}

unsigned int __stdcall GlueMovementTimedNewPointMarker(unsigned int order, const Point16* pos, const Point16* size)
{
    unsigned long long start = host_time_counter();
    unsigned int result = s_movementNative->NewPointMarker(order, pos, size);
    GlueTimed(&s_movementGlue, 11, start);
    return result;
}

int __stdcall GlueMovementTimedTargetType(unsigned int target)
{
    unsigned long long start = host_time_counter();
    int result = s_movementNative->TargetType(target);
    GlueTimed(&s_movementGlue, 12, start);
    return result;
}

unsigned int __stdcall GlueMovementTimedUnitTarget(Unit* unit)
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
    s_movementSeam.Apply = GlueMovementApply;
    s_movementSeam.UpdateMotion = GlueMovementUpdateMotion;
    s_movementSeam.ApplyPathEvent = GlueMovementApplyPathEvent;
    s_movementSeam.HasNetState = GlueMovementHasNetState;
    s_movementSeam.NewPathOrderAt = GlueMovementNewPathOrderAt;
    s_movementSeam.NewPathOrderToUnit = GlueMovementNewPathOrderToUnit;
    s_movementSeam.NewPathOrderToUnitAt = GlueMovementNewPathOrderToUnitAt;
    s_movementSeam.NewPathOrderToUnitAtAngle = GlueMovementNewPathOrderToUnitAtAngle;
    s_movementSeam.NewAirManeuverOrder = GlueMovementNewAirManeuverOrder;
    s_movementSeam.NewApproachRadius = GlueMovementNewApproachRadius;
    s_movementSeam.NewRingApproach = GlueMovementNewRingApproach;
    s_movementSeam.NewPointMarker = GlueMovementNewPointMarker;
    s_movementSeam.TargetType = GlueMovementTargetType;
    s_movementSeam.UnitTarget = GlueMovementUnitTarget;
    g_seams.movement = &s_movementSeam;
}

void MovementTimeNative()
{
    s_movementNative = g_seams.movement;
    s_movementTimed = *s_movementNative;
    s_movementTimed.Apply = GlueMovementTimedApply;
    s_movementTimed.UpdateMotion = GlueMovementTimedUpdateMotion;
    s_movementTimed.ApplyPathEvent = GlueMovementTimedApplyPathEvent;
    s_movementTimed.HasNetState = GlueMovementTimedHasNetState;
    s_movementTimed.NewPathOrderAt = GlueMovementTimedNewPathOrderAt;
    s_movementTimed.NewPathOrderToUnit = GlueMovementTimedNewPathOrderToUnit;
    s_movementTimed.NewPathOrderToUnitAt = GlueMovementTimedNewPathOrderToUnitAt;
    s_movementTimed.NewPathOrderToUnitAtAngle = GlueMovementTimedNewPathOrderToUnitAtAngle;
    s_movementTimed.NewAirManeuverOrder = GlueMovementTimedNewAirManeuverOrder;
    s_movementTimed.NewApproachRadius = GlueMovementTimedNewApproachRadius;
    s_movementTimed.NewRingApproach = GlueMovementTimedNewRingApproach;
    s_movementTimed.NewPointMarker = GlueMovementTimedNewPointMarker;
    s_movementTimed.TargetType = GlueMovementTimedTargetType;
    s_movementTimed.UnitTarget = GlueMovementTimedUnitTarget;
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

int __stdcall GlueMissionGetGameType()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 0, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

const char* __stdcall GlueMissionGetNameSlot(int index)
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

int __stdcall GlueMissionGetMissionIndex()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 2, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

const char* __stdcall GlueMissionGetMissionName()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 3, slotArgs, 1);
    GlueScratchRelease(mark);
    return (const char*)GlueResult(glue, 3, (unsigned)slotArgs[0].i32, 0);
}

int __stdcall GlueMissionGetStartPosition(Vec3* out, int id)
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

int __stdcall GlueMissionCountStartPositions()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 5, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall GlueMissionHasMissionName()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 6, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

void __stdcall GlueMissionGetMissionParams(MissionParams* out)
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

int __stdcall GlueMissionCountMissionUnits()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 8, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall GlueMissionGetMissionUnit(int index, MissionUnitInfo* out)
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

int __stdcall GlueMissionCountMissionFeatures()
{
    GlueModule* glue = &s_missionGlue;
    unsigned mark = GlueScratchMark();
    wasmtime_val_raw_t slotArgs[1];
    slotArgs[0].i64 = 0;
    GlueCall(glue, 10, slotArgs, 1);
    GlueScratchRelease(mark);
    return (int)slotArgs[0].i32;
}

int __stdcall GlueMissionGetMissionFeature(int index, MissionFeatureInfo* out)
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

int __stdcall GlueMissionTimedGetGameType()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->GetGameType();
    GlueTimed(&s_missionGlue, 0, start);
    return result;
}

const char* __stdcall GlueMissionTimedGetNameSlot(int index)
{
    unsigned long long start = host_time_counter();
    const char* result = s_missionNative->GetNameSlot(index);
    GlueTimed(&s_missionGlue, 1, start);
    return result;
}

int __stdcall GlueMissionTimedGetMissionIndex()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->GetMissionIndex();
    GlueTimed(&s_missionGlue, 2, start);
    return result;
}

const char* __stdcall GlueMissionTimedGetMissionName()
{
    unsigned long long start = host_time_counter();
    const char* result = s_missionNative->GetMissionName();
    GlueTimed(&s_missionGlue, 3, start);
    return result;
}

int __stdcall GlueMissionTimedGetStartPosition(Vec3* out, int id)
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->GetStartPosition(out, id);
    GlueTimed(&s_missionGlue, 4, start);
    return result;
}

int __stdcall GlueMissionTimedCountStartPositions()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->CountStartPositions();
    GlueTimed(&s_missionGlue, 5, start);
    return result;
}

int __stdcall GlueMissionTimedHasMissionName()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->HasMissionName();
    GlueTimed(&s_missionGlue, 6, start);
    return result;
}

void __stdcall GlueMissionTimedGetMissionParams(MissionParams* out)
{
    unsigned long long start = host_time_counter();
    s_missionNative->GetMissionParams(out);
    GlueTimed(&s_missionGlue, 7, start);
}

int __stdcall GlueMissionTimedCountMissionUnits()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->CountMissionUnits();
    GlueTimed(&s_missionGlue, 8, start);
    return result;
}

int __stdcall GlueMissionTimedGetMissionUnit(int index, MissionUnitInfo* out)
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->GetMissionUnit(index, out);
    GlueTimed(&s_missionGlue, 9, start);
    return result;
}

int __stdcall GlueMissionTimedCountMissionFeatures()
{
    unsigned long long start = host_time_counter();
    int result = s_missionNative->CountMissionFeatures();
    GlueTimed(&s_missionGlue, 10, start);
    return result;
}

int __stdcall GlueMissionTimedGetMissionFeature(int index, MissionFeatureInfo* out)
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
    s_missionSeam.GetGameType = GlueMissionGetGameType;
    s_missionSeam.GetNameSlot = GlueMissionGetNameSlot;
    s_missionSeam.GetMissionIndex = GlueMissionGetMissionIndex;
    s_missionSeam.GetMissionName = GlueMissionGetMissionName;
    s_missionSeam.GetStartPosition = GlueMissionGetStartPosition;
    s_missionSeam.CountStartPositions = GlueMissionCountStartPositions;
    s_missionSeam.HasMissionName = GlueMissionHasMissionName;
    s_missionSeam.GetMissionParams = GlueMissionGetMissionParams;
    s_missionSeam.CountMissionUnits = GlueMissionCountMissionUnits;
    s_missionSeam.GetMissionUnit = GlueMissionGetMissionUnit;
    s_missionSeam.CountMissionFeatures = GlueMissionCountMissionFeatures;
    s_missionSeam.GetMissionFeature = GlueMissionGetMissionFeature;
    g_seams.mission = &s_missionSeam;
}

void MissionTimeNative()
{
    s_missionNative = g_seams.mission;
    s_missionTimed = *s_missionNative;
    s_missionTimed.GetGameType = GlueMissionTimedGetGameType;
    s_missionTimed.GetNameSlot = GlueMissionTimedGetNameSlot;
    s_missionTimed.GetMissionIndex = GlueMissionTimedGetMissionIndex;
    s_missionTimed.GetMissionName = GlueMissionTimedGetMissionName;
    s_missionTimed.GetStartPosition = GlueMissionTimedGetStartPosition;
    s_missionTimed.CountStartPositions = GlueMissionTimedCountStartPositions;
    s_missionTimed.HasMissionName = GlueMissionTimedHasMissionName;
    s_missionTimed.GetMissionParams = GlueMissionTimedGetMissionParams;
    s_missionTimed.CountMissionUnits = GlueMissionTimedCountMissionUnits;
    s_missionTimed.GetMissionUnit = GlueMissionTimedGetMissionUnit;
    s_missionTimed.CountMissionFeatures = GlueMissionTimedCountMissionFeatures;
    s_missionTimed.GetMissionFeature = GlueMissionTimedGetMissionFeature;
    g_seams.mission = &s_missionTimed;
}

const GlueModuleDef kMissionDef = {"mission", -1, kMissionSlots, 12, s_missionFuncs, s_missionTiming, MissionUseSeam, MissionTimeNative, 1, 1};
GlueModule s_missionGlue = {&kMissionDef};

const char* __cdecl GlueImportEconomyGetEnergyUse(wasmtime_val_raw_t* a)
{
    EconType type = (EconType)a[0].i32;
    double result = g_seams.economy->GetEnergyUse(type);
    a[0].f64 = (double)result;
    return nullptr;
}

const char* __cdecl GlueImportEconomyRequestGranted(wasmtime_val_raw_t* a)
{
    EconUnit unit = (EconUnit)a[0].i32;
    unsigned int resources = (unsigned int)a[1].i32;
    int result = ScheduleEconomyRequestGranted(unit, resources);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconomySpend(wasmtime_val_raw_t* a)
{
    GLUE_NEED("economy.Spend", 0);
    const EconomySpend* spend = (const EconomySpend*)GlueLinear((unsigned)a[0].i32);
    int result = ScheduleEconomySpend(spend);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconomyUpdateMetalExtraction(wasmtime_val_raw_t* a)
{
    EconUnit unit = (EconUnit)a[0].i32;
    ScheduleMetalExtraction(unit);
    return nullptr;
}

const char* __cdecl GlueImportEconworldAllied(wasmtime_val_raw_t* a)
{
    int from = (int)a[0].i32;
    int to = (int)a[1].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Allied", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->Allied(from, to);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldCellMetal(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int y = (int)a[1].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.CellMetal", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->CellMetal(x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldDifficulty(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Difficulty", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->Difficulty();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldEmit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("econworld.Emit", 0);
    const EconomyEventHeader* effect = (const EconomyEventHeader*)GlueLinear((unsigned)a[0].i32);
    unsigned int size = (unsigned int)a[1].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Emit", "the world view is not set");
    ((const EconomyWorldView*)GlueActive()->world)->Emit(effect, size);
    return nullptr;
}

const char* __cdecl GlueImportEconworldFirstUnit(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.FirstUnit", "the world view is not set");
    EconUnit result = ((const EconomyWorldView*)GlueActive()->world)->FirstUnit(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldLastUnit(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.LastUnit", "the world view is not set");
    EconUnit result = ((const EconomyWorldView*)GlueActive()->world)->LastUnit(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldNextUnit(wasmtime_val_raw_t* a)
{
    EconUnit unit = (EconUnit)a[0].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.NextUnit", "the world view is not set");
    EconUnit result = ((const EconomyWorldView*)GlueActive()->world)->NextUnit(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldPlayerInfo(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    GLUE_NEED("econworld.PlayerInfo", 1);
    EconPlayerInfo* out = (EconPlayerInfo*)GlueLinear((unsigned)a[1].i32);
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.PlayerInfo", "the world view is not set");
    ((const EconomyWorldView*)GlueActive()->world)->PlayerInfo(player, out);
    return nullptr;
}

const char* __cdecl GlueImportEconworldSharingEnabled(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.SharingEnabled", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->SharingEnabled();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldTick(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Tick", "the world view is not set");
    unsigned int result = ((const EconomyWorldView*)GlueActive()->world)->Tick();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldTide(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Tide", "the world view is not set");
    float result = ((const EconomyWorldView*)GlueActive()->world)->Tide();
    a[0].f32 = (float)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldTypeInfo(wasmtime_val_raw_t* a)
{
    EconType type = (EconType)a[0].i32;
    GLUE_NEED("econworld.TypeInfo", 1);
    EconTypeInfo* out = (EconTypeInfo*)GlueLinear((unsigned)a[1].i32);
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.TypeInfo", "the world view is not set");
    ((const EconomyWorldView*)GlueActive()->world)->TypeInfo(type, out);
    return nullptr;
}

const char* __cdecl GlueImportEconworldUnitInfo(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportEconworldUnitSlot(wasmtime_val_raw_t* a)
{
    EconUnit unit = (EconUnit)a[0].i32;
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.UnitSlot", "the world view is not set");
    int result = ((const EconomyWorldView*)GlueActive()->world)->UnitSlot(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportEconworldWind(wasmtime_val_raw_t* a)
{
    if (GlueActive() == nullptr || GlueActive()->world == nullptr)
        return GlueImportError("econworld.Wind", "the world view is not set");
    float result = ((const EconomyWorldView*)GlueActive()->world)->Wind();
    a[0].f32 = (float)result;
    return nullptr;
}

const char* __cdecl GlueImportFeaturesFindFeatureAtPos(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportFeaturesGetCellFeature(wasmtime_val_raw_t* a)
{
    GLUE_NEED("features.GetCellFeature", 0);
    Cell* cell = (Cell*)GlueLinear((unsigned)a[0].i32);
    unsigned short result = g_seams.features->GetCellFeature(cell);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportFeaturesGetFootprintCentre(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportFeaturesPlace(wasmtime_val_raw_t* a)
{
    GLUE_NEED("features.Place", 0);
    const FeaturesPlace* place = (const FeaturesPlace*)GlueLinear((unsigned)a[0].i32);
    int result = ScheduleFeaturesPlace(place);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportLosRadarClampWorldPosToTerrain(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int y = (int)a[1].i32;
    GLUE_NEED("los_radar.ClampWorldPosToTerrain", 2);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[2].i32);
    g_seams.los_radar->ClampWorldPosToTerrain(x, y, out);
    return nullptr;
}

const char* __cdecl GlueImportLosRadarGetMapCell(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int y = (int)a[1].i32;
    Cell* result = g_seams.los_radar->GetMapCell(x, y);
    unsigned address;
    const char* error = GlueAddressOf("los_radar.GetMapCell", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl GlueImportLosRadarGetMapCellAtPosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.GetMapCellAtPosition", 0);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[0].i32);
    Cell* result = g_seams.los_radar->GetMapCellAtPosition(pos);
    unsigned address;
    const char* error = GlueAddressOf("los_radar.GetMapCellAtPosition", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl GlueImportLosRadarGetOriginCellAtPosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.GetOriginCellAtPosition", 0);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[0].i32);
    Cell* result = g_seams.los_radar->GetOriginCellAtPosition(pos);
    unsigned address;
    const char* error = GlueAddressOf("los_radar.GetOriginCellAtPosition", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl GlueImportLosRadarGetSeenUnitsInRadius(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportLosRadarGetVisibleEnemiesInRadius(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportLosRadarIsPointVisible(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.IsPointVisible", 0);
    Player* player = (Player*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("los_radar.IsPointVisible", 1);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.los_radar->IsPointVisible(player, pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportLosRadarIsUnitVisibleToPlayer(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.IsUnitVisibleToPlayer", 0);
    Player* player = (Player*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("los_radar.IsUnitVisibleToPlayer", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.los_radar->IsUnitVisibleToPlayer(player, unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportLosRadarSoundHeardMask(wasmtime_val_raw_t* a)
{
    GLUE_NEED("los_radar.SoundHeardMask", 0);
    const Vec3* at = (const Vec3*)GlueLinear((unsigned)a[0].i32);
    unsigned int result = g_seams.los_radar->SoundHeardMask(at);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMapCanBuildAt(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportMapCanPlaceFootprintAt(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.CanPlaceFootprintAt", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("map.CanPlaceFootprintAt", 1);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.map->CanPlaceFootprintAt(unit, pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMapCanPlaceUnitFootprint(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportMapGetCellMeanHeight(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.GetCellMeanHeight", 0);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[0].i32);
    int result = g_seams.map->GetCellMeanHeight(pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMapGetGroundHeight(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.GetGroundHeight", 0);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[0].i32);
    int result = g_seams.map->GetGroundHeight(pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMapIsPadSlotFree(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.IsPadSlotFree", 0);
    Unit* pad = (Unit*)GlueLinear((unsigned)a[0].i32);
    int id = (int)a[1].i32;
    int result = g_seams.map->IsPadSlotFree(pad, id);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMapSnapWorldPosToFootprint(wasmtime_val_raw_t* a)
{
    GLUE_NEED("map.SnapWorldPosToFootprint", 0);
    UnitDef* unit = (UnitDef*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("map.SnapWorldPosToFootprint", 1);
    Vec3* pos = (Vec3*)GlueLinear((unsigned)a[1].i32);
    g_seams.map->SnapWorldPosToFootprint(unit, pos);
    return nullptr;
}

const char* __cdecl GlueImportMapUnitsInRange(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportMissionGetGameType(wasmtime_val_raw_t* a)
{
    int result = g_seams.mission->GetGameType();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMissionGetMissionParams(wasmtime_val_raw_t* a)
{
    GLUE_NEED("mission.GetMissionParams", 0);
    MissionParams* out = (MissionParams*)GlueLinear((unsigned)a[0].i32);
    g_seams.mission->GetMissionParams(out);
    return nullptr;
}

const char* __cdecl GlueImportMissionGetNameSlot(wasmtime_val_raw_t* a)
{
    int index = (int)a[0].i32;
    const char* result = g_seams.mission->GetNameSlot(index);
    unsigned address;
    const char* error = GlueAddressOf("mission.GetNameSlot", (const void*)result, 0, &address, "pointer");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl GlueImportMovementNewAirManeuverOrder(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportMovementNewApproachRadius(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int radius = (int)a[3].i32;
    unsigned int result = g_seams.movement->NewApproachRadius(order, x, y, radius);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMovementNewPathOrderAt(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    GLUE_NEED("movement.NewPathOrderAt", 1);
    const Vec3* pos = (const Vec3*)GlueLinear((unsigned)a[1].i32);
    unsigned int result = g_seams.movement->NewPathOrderAt(order, pos);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMovementNewPathOrderToUnit(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    GLUE_NEED("movement.NewPathOrderToUnit", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    unsigned int result = g_seams.movement->NewPathOrderToUnit(order, unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMovementNewPathOrderToUnitAt(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportMovementNewPathOrderToUnitAtAngle(wasmtime_val_raw_t* a)
{
    unsigned int order = (unsigned int)a[0].i32;
    GLUE_NEED("movement.NewPathOrderToUnitAtAngle", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    short angle = (short)a[2].i32;
    unsigned int result = g_seams.movement->NewPathOrderToUnitAtAngle(order, unit, angle);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportMovementNewPointMarker(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportMovementNewRingApproach(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportMovementUnitTarget(wasmtime_val_raw_t* a)
{
    GLUE_NEED("movement.UnitTarget", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    unsigned int result = g_seams.movement->UnitTarget(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportNetworkBroadcastPlayerEconomy(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.BroadcastPlayerEconomy", 0);
    Player* player = (Player*)GlueLinear((unsigned)a[0].i32);
    g_seams.network->BroadcastPlayerEconomy(player);
    return nullptr;
}

const char* __cdecl GlueImportNetworkBroadcastPlayerInfo(wasmtime_val_raw_t* a)
{
    g_seams.network->BroadcastPlayerInfo();
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendActivateFlags(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendActivateFlags", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    g_seams.network->SendActivateFlags(unit);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendAttach(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportNetworkSendBuilderLink(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendBuilderLink", 0);
    Unit* builder = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendBuilderLink", 1);
    Unit* constructed = (Unit*)GlueLinear((unsigned)a[1].i32);
    g_seams.network->SendBuilderLink(builder, constructed);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendFeatureBurning(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int z = (int)a[1].i32;
    g_seams.network->SendFeatureBurning(x, z);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendFeatureHit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendFeatureHit", 0);
    WeaponDef* weapon = (WeaponDef*)GlueLinear((unsigned)a[0].i32);
    int x = (int)a[1].i32;
    int z = (int)a[2].i32;
    g_seams.network->SendFeatureHit(weapon, x, z);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendFeatureKilled(wasmtime_val_raw_t* a)
{
    int x = (int)a[0].i32;
    int z = (int)a[1].i32;
    g_seams.network->SendFeatureKilled(x, z);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendFeatureReclaimed(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendFeatureReclaimed", 0);
    Unit* reclaimer = (Unit*)GlueLinear((unsigned)a[0].i32);
    int x = (int)a[1].i32;
    int z = (int)a[2].i32;
    g_seams.network->SendFeatureReclaimed(reclaimer, x, z);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendNewUnit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendNewUnit", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    g_seams.network->SendNewUnit(unit);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendOwnershipTransfer(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendOwnershipTransfer", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendOwnershipTransfer", 1);
    Player* newOwner = (Player*)GlueLinear((unsigned)a[1].i32);
    g_seams.network->SendOwnershipTransfer(unit, newOwner);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendProjectileDetonated(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendProjectileDetonated", 0);
    Unit* attacker = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("network.SendProjectileDetonated", 1);
    Projectile* projectile = (Projectile*)GlueLinear((unsigned)a[1].i32);
    g_seams.network->SendProjectileDetonated(attacker, projectile);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendScriptCall(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportNetworkSendScriptCallByName(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportNetworkSendShareEnergy(wasmtime_val_raw_t* a)
{
    unsigned char from = (unsigned char)a[0].i32;
    unsigned char to = (unsigned char)a[1].i32;
    float amount = (float)a[2].f32;
    g_seams.network->SendShareEnergy(from, to, amount);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendShareMapInfo(wasmtime_val_raw_t* a)
{
    unsigned char from = (unsigned char)a[0].i32;
    unsigned char to = (unsigned char)a[1].i32;
    g_seams.network->SendShareMapInfo(from, to);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendShareMetal(wasmtime_val_raw_t* a)
{
    unsigned char from = (unsigned char)a[0].i32;
    unsigned char to = (unsigned char)a[1].i32;
    float amount = (float)a[2].f32;
    g_seams.network->SendShareMetal(from, to, amount);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendUnitDamage(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportNetworkSendUnitKilled(wasmtime_val_raw_t* a)
{
    GLUE_NEED("network.SendUnitKilled", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int severity = (int)a[1].i32;
    int wreckageLevel = (int)a[2].i32;
    int killType = (int)a[3].i32;
    g_seams.network->SendUnitKilled(unit, severity, wreckageLevel, killType);
    return nullptr;
}

const char* __cdecl GlueImportNetworkSendWeaponFire(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportNetworkSendWeaponFireAt(wasmtime_val_raw_t* a)
{
    unsigned char weaponIndex = (unsigned char)a[0].i32;
    GLUE_NEED("network.SendWeaponFireAt", 1);
    Vec3* origin = (Vec3*)GlueLinear((unsigned)a[1].i32);
    GLUE_NEED("network.SendWeaponFireAt", 2);
    Vec3* aimOrVel = (Vec3*)GlueLinear((unsigned)a[2].i32);
    g_seams.network->SendWeaponFireAt(weaponIndex, origin, aimOrVel);
    return nullptr;
}

const char* __cdecl GlueImportOrdersGetOrderFlags(wasmtime_val_raw_t* a)
{
    GLUE_NEED("orders.GetOrderFlags", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int result = g_seams.orders->GetOrderFlags(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportOrdersGetOrderType(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportPathfindingCancel(wasmtime_val_raw_t* a)
{
    PathHandle requester = (PathHandle)a[0].i32;
    g_seams.pathfinding->Cancel(requester);
    return nullptr;
}

const char* __cdecl GlueImportPathfindingSubmit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("pathfinding.Submit", 0);
    const PathRequest* request = (const PathRequest*)GlueLinear((unsigned)a[0].i32);
    g_seams.pathfinding->Submit(request);
    return nullptr;
}

const char* __cdecl GlueImportPathworldBeginSearch(wasmtime_val_raw_t* a)
{
    PathUnit unit = (PathUnit)a[0].i32;
    g_seams.pathworld->BeginSearch(unit);
    return nullptr;
}

const char* __cdecl GlueImportPathworldEndSearch(wasmtime_val_raw_t* a)
{
    PathUnit unit = (PathUnit)a[0].i32;
    g_seams.pathworld->EndSearch(unit);
    return nullptr;
}

const char* __cdecl GlueImportPathworldFirstUnit(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    PathUnit result = g_seams.pathworld->FirstUnit(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldInBounds(wasmtime_val_raw_t* a)
{
    PathClass moveClass = (PathClass)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int result = g_seams.pathworld->InBounds(moveClass, x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldIsPlaying(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    int result = g_seams.pathworld->IsPlaying(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldLastUnit(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    PathUnit result = g_seams.pathworld->LastUnit(player);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldLineOfSight(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportPathworldMoveClass(wasmtime_val_raw_t* a)
{
    int index = (int)a[0].i32;
    PathClass result = g_seams.pathworld->MoveClass(index);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldNextUnit(wasmtime_val_raw_t* a)
{
    PathUnit unit = (PathUnit)a[0].i32;
    PathUnit result = g_seams.pathworld->NextUnit(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldPass(wasmtime_val_raw_t* a)
{
    PathClass moveClass = (PathClass)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int result = g_seams.pathworld->Pass(moveClass, x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldPlayerCount(wasmtime_val_raw_t* a)
{
    int result = g_seams.pathworld->PlayerCount();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldSearchStepLimit(wasmtime_val_raw_t* a)
{
    unsigned int result = g_seams.pathworld->SearchStepLimit();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldSeen(wasmtime_val_raw_t* a)
{
    int player = (int)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int result = g_seams.pathworld->Seen(player, x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldSeenSize(wasmtime_val_raw_t* a)
{
    GLUE_NEED("pathworld.SeenSize", 0);
    int* width = (int*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("pathworld.SeenSize", 1);
    int* height = (int*)GlueLinear((unsigned)a[1].i32);
    g_seams.pathworld->SeenSize(width, height);
    return nullptr;
}

const char* __cdecl GlueImportPathworldTerrainPass(wasmtime_val_raw_t* a)
{
    PathClass moveClass = (PathClass)a[0].i32;
    int x = (int)a[1].i32;
    int y = (int)a[2].i32;
    int result = g_seams.pathworld->TerrainPass(moveClass, x, y);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldTick(wasmtime_val_raw_t* a)
{
    unsigned int result = g_seams.pathworld->Tick();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldUnitState(wasmtime_val_raw_t* a)
{
    PathUnit unit = (PathUnit)a[0].i32;
    GLUE_NEED("pathworld.UnitState", 1);
    PathUnitState* out = (PathUnitState*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.pathworld->UnitState(unit, out);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportPathworldUnitsPerPlayer(wasmtime_val_raw_t* a)
{
    int result = g_seams.pathworld->UnitsPerPlayer();
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportScriptsCreateObjectState(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.CreateObjectState", 0);
    Object3do* model = (Object3do*)GlueLinear((unsigned)a[0].i32);
    ModelState* result = g_seams.scripts->CreateObjectState(model);
    unsigned address;
    const char* error = GlueAddressOf("scripts.CreateObjectState", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl GlueImportScriptsCreatePlayerObjectState(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportScriptsDeleteScript(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.DeleteScript", 0);
    CobScript* script = (CobScript*)GlueLinear((unsigned)a[0].i32);
    g_seams.scripts->DeleteScript(script);
    return nullptr;
}

const char* __cdecl GlueImportScriptsFindScript(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.FindScript", 0);
    CobScript* script = (CobScript*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.FindScript", 1);
    const char* name = (const char*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.scripts->FindScript(script, name);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportScriptsFreeObjectState(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.FreeObjectState", 0);
    ModelState* state = (ModelState*)GlueLinear((unsigned)a[0].i32);
    g_seams.scripts->FreeObjectState(state);
    return nullptr;
}

const char* __cdecl GlueImportScriptsGetAimFromPosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetAimFromPosition", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.GetAimFromPosition", 1);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[1].i32);
    unsigned char weapon = (unsigned char)a[2].i32;
    g_seams.scripts->GetAimFromPosition(unit, out, weapon);
    return nullptr;
}

const char* __cdecl GlueImportScriptsGetNanoPiecePosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetNanoPiecePosition", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.GetNanoPiecePosition", 1);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[1].i32);
    g_seams.scripts->GetNanoPiecePosition(unit, out);
    return nullptr;
}

const char* __cdecl GlueImportScriptsGetPieceAngles(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetPieceAngles", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int piece = (int)a[1].i32;
    GLUE_NEED("scripts.GetPieceAngles", 2);
    Angles16* out = (Angles16*)GlueLinear((unsigned)a[2].i32);
    g_seams.scripts->GetPieceAngles(unit, piece, out);
    return nullptr;
}

const char* __cdecl GlueImportScriptsGetPieceOffset(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetPieceOffset", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int piece = (int)a[1].i32;
    GLUE_NEED("scripts.GetPieceOffset", 2);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[2].i32);
    g_seams.scripts->GetPieceOffset(unit, piece, out);
    return nullptr;
}

const char* __cdecl GlueImportScriptsGetPiecePosition(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetPiecePosition", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    int piece = (int)a[1].i32;
    GLUE_NEED("scripts.GetPiecePosition", 2);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[2].i32);
    g_seams.scripts->GetPiecePosition(unit, piece, out);
    return nullptr;
}

const char* __cdecl GlueImportScriptsGetSweetSpot(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.GetSweetSpot", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.GetSweetSpot", 1);
    Vec3* out = (Vec3*)GlueLinear((unsigned)a[1].i32);
    g_seams.scripts->GetSweetSpot(unit, out);
    return nullptr;
}

const char* __cdecl GlueImportScriptsNewUnitScript(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.NewUnitScript", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    g_seams.scripts->NewUnitScript(unit);
    return nullptr;
}

const char* __cdecl GlueImportScriptsObjectStateSize(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.ObjectStateSize", 0);
    Object3do* model = (Object3do*)GlueLinear((unsigned)a[0].i32);
    unsigned int result = g_seams.scripts->ObjectStateSize(model);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportScriptsQueryScript(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportScriptsSetCob(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.SetCob", 0);
    CobScript* script = (CobScript*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.SetCob", 1);
    ScriptTable* data = (ScriptTable*)GlueLinear((unsigned)a[1].i32);
    g_seams.scripts->SetCob(script, data);
    return nullptr;
}

const char* __cdecl GlueImportScriptsSetObjectState(wasmtime_val_raw_t* a)
{
    GLUE_NEED("scripts.SetObjectState", 0);
    UnitScript* script = (UnitScript*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("scripts.SetObjectState", 1);
    ModelState* state = (ModelState*)GlueLinear((unsigned)a[1].i32);
    g_seams.scripts->SetObjectState(script, state);
    return nullptr;
}

const char* __cdecl GlueImportTargetingChooseWeapon(wasmtime_val_raw_t* a)
{
    GLUE_NEED("targeting.ChooseWeapon", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    unsigned char result = g_seams.targeting->ChooseWeapon(unit);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportTargetingFindBestTargetIfFireAtWill(wasmtime_val_raw_t* a)
{
    GLUE_NEED("targeting.FindBestTargetIfFireAtWill", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    Unit* result = ScheduleTargetingFireAtWill(unit);
    unsigned address;
    const char* error = GlueAddressOf("targeting.FindBestTargetIfFireAtWill", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl GlueImportTargetingFindWeaponTarget(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportTargetingGetWeaponTargetPos(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportTargetingGetWeaponTargetUnit(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportUnitsAddBuildProgress(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportUnitsAddRepairProgress(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportUnitsCanLoad(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.CanLoad", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.CanLoad", 1);
    Unit* target = (Unit*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.units->CanLoad(unit, target);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportUnitsCanReclaim(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.CanReclaim", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.CanReclaim", 1);
    Unit* target = (Unit*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.units->CanReclaim(unit, target);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportUnitsCanRepair(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.CanRepair", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.CanRepair", 1);
    Unit* target = (Unit*)GlueLinear((unsigned)a[1].i32);
    int result = g_seams.units->CanRepair(unit, target);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportUnitsCreateUnit(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportUnitsFindUnitTypeId(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.FindUnitTypeId", 0);
    const char* name = (const char*)GlueLinear((unsigned)a[0].i32);
    unsigned short result = g_seams.units->FindUnitTypeId(name);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportUnitsGetFactoriesInRadius(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportUnitsUnitRefGet(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.UnitRefGet", 0);
    UnitRef* ref = (UnitRef*)GlueLinear((unsigned)a[0].i32);
    Unit* result = g_seams.units->UnitRefGet(ref);
    unsigned address;
    const char* error = GlueAddressOf("units.UnitRefGet", (const void*)result, 1, &address, "object");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl GlueImportUnitsUnitRefInit(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportUnitsUnitRefSetUnit(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.UnitRefSetUnit", 0);
    UnitRef* ref = (UnitRef*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.UnitRefSetUnit", 1);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[1].i32);
    g_seams.units->UnitRefSetUnit(ref, unit);
    return nullptr;
}

const char* __cdecl GlueImportUnitsUnitRefSetValue(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.UnitRefSetValue", 0);
    UnitRef* ref = (UnitRef*)GlueLinear((unsigned)a[0].i32);
    GLUE_NEED("units.UnitRefSetValue", 1);
    OrderBase* value = (OrderBase*)GlueLinear((unsigned)a[1].i32);
    g_seams.units->UnitRefSetValue(ref, value);
    return nullptr;
}

const char* __cdecl GlueImportUnitsUnitRefUnlink(wasmtime_val_raw_t* a)
{
    GLUE_NEED("units.UnitRefUnlink", 0);
    UnitRef* ref = (UnitRef*)GlueLinear((unsigned)a[0].i32);
    g_seams.units->UnitRefUnlink(ref);
    return nullptr;
}

const char* __cdecl GlueImportWeaponsFindTargetableProjectile(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportWeaponsFindWeaponByName(wasmtime_val_raw_t* a)
{
    GLUE_NEED("weapons.FindWeaponByName", 0);
    const char* name = (const char*)GlueLinear((unsigned)a[0].i32);
    WeaponDef* result = g_seams.weapons->FindWeaponByName(name);
    unsigned address;
    const char* error = GlueAddressOf("weapons.FindWeaponByName", (const void*)result, 1, &address, "weapon");
    a[0].i32 = (int)address;
    return error;
}

const char* __cdecl GlueImportWeaponsGetWeaponRange(wasmtime_val_raw_t* a)
{
    GLUE_NEED("weapons.GetWeaponRange", 0);
    Unit* unit = (Unit*)GlueLinear((unsigned)a[0].i32);
    unsigned int weapon = (unsigned int)a[1].i32;
    int result = g_seams.weapons->GetWeaponRange(unit, weapon);
    a[0].i32 = (int)result;
    return nullptr;
}

const char* __cdecl GlueImportWeaponsWeaponCanReachPos(wasmtime_val_raw_t* a)
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

const char* __cdecl GlueImportWeaponsWeaponCanReachUnit(wasmtime_val_raw_t* a)
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

GlueModule* const kGlueModules[] = {
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

const GlueImport kGlueInterfaceImports[] = {
    {"economy", "GetEnergyUse", "i>d", GlueImportEconomyGetEnergyUse},
    {"economy", "RequestGranted", "ii>i", GlueImportEconomyRequestGranted},
    {"economy", "Spend", "i>i", GlueImportEconomySpend},
    {"economy", "UpdateMetalExtraction", "i>", GlueImportEconomyUpdateMetalExtraction},
    {"econworld", "Allied", "ii>i", GlueImportEconworldAllied},
    {"econworld", "CellMetal", "ii>i", GlueImportEconworldCellMetal},
    {"econworld", "Difficulty", ">i", GlueImportEconworldDifficulty},
    {"econworld", "Emit", "ii>", GlueImportEconworldEmit},
    {"econworld", "FirstUnit", "i>i", GlueImportEconworldFirstUnit},
    {"econworld", "LastUnit", "i>i", GlueImportEconworldLastUnit},
    {"econworld", "NextUnit", "i>i", GlueImportEconworldNextUnit},
    {"econworld", "PlayerInfo", "ii>", GlueImportEconworldPlayerInfo},
    {"econworld", "SharingEnabled", ">i", GlueImportEconworldSharingEnabled},
    {"econworld", "Tick", ">i", GlueImportEconworldTick},
    {"econworld", "Tide", ">f", GlueImportEconworldTide},
    {"econworld", "TypeInfo", "ii>", GlueImportEconworldTypeInfo},
    {"econworld", "UnitInfo", "ii>i", GlueImportEconworldUnitInfo},
    {"econworld", "UnitSlot", "i>i", GlueImportEconworldUnitSlot},
    {"econworld", "Wind", ">f", GlueImportEconworldWind},
    {"features", "FindFeatureAtPos", "iii>i", GlueImportFeaturesFindFeatureAtPos},
    {"features", "GetCellFeature", "i>i", GlueImportFeaturesGetCellFeature},
    {"features", "GetFootprintCentre", "iii>", GlueImportFeaturesGetFootprintCentre},
    {"features", "Place", "i>i", GlueImportFeaturesPlace},
    {"los_radar", "ClampWorldPosToTerrain", "iii>", GlueImportLosRadarClampWorldPosToTerrain},
    {"los_radar", "GetMapCell", "ii>i", GlueImportLosRadarGetMapCell},
    {"los_radar", "GetMapCellAtPosition", "i>i", GlueImportLosRadarGetMapCellAtPosition},
    {"los_radar", "GetOriginCellAtPosition", "i>i", GlueImportLosRadarGetOriginCellAtPosition},
    {"los_radar", "GetSeenUnitsInRadius", "iiiii>i", GlueImportLosRadarGetSeenUnitsInRadius},
    {"los_radar", "GetVisibleEnemiesInRadius", "iiiiii>i", GlueImportLosRadarGetVisibleEnemiesInRadius},
    {"los_radar", "IsPointVisible", "ii>i", GlueImportLosRadarIsPointVisible},
    {"los_radar", "IsUnitVisibleToPlayer", "ii>i", GlueImportLosRadarIsUnitVisibleToPlayer},
    {"los_radar", "SoundHeardMask", "i>i", GlueImportLosRadarSoundHeardMask},
    {"map", "CanBuildAt", "iiii>i", GlueImportMapCanBuildAt},
    {"map", "CanPlaceFootprintAt", "ii>i", GlueImportMapCanPlaceFootprintAt},
    {"map", "CanPlaceUnitFootprint", "iiii>i", GlueImportMapCanPlaceUnitFootprint},
    {"map", "GetCellMeanHeight", "i>i", GlueImportMapGetCellMeanHeight},
    {"map", "GetGroundHeight", "i>i", GlueImportMapGetGroundHeight},
    {"map", "IsPadSlotFree", "ii>i", GlueImportMapIsPadSlotFree},
    {"map", "SnapWorldPosToFootprint", "ii>", GlueImportMapSnapWorldPosToFootprint},
    {"map", "UnitsInRange", "iiiii>i", GlueImportMapUnitsInRange},
    {"mission", "GetGameType", ">i", GlueImportMissionGetGameType},
    {"mission", "GetMissionParams", "i>", GlueImportMissionGetMissionParams},
    {"mission", "GetNameSlot", "i>i", GlueImportMissionGetNameSlot},
    {"movement", "NewAirManeuverOrder", "iii>i", GlueImportMovementNewAirManeuverOrder},
    {"movement", "NewApproachRadius", "iiii>i", GlueImportMovementNewApproachRadius},
    {"movement", "NewPathOrderAt", "ii>i", GlueImportMovementNewPathOrderAt},
    {"movement", "NewPathOrderToUnit", "ii>i", GlueImportMovementNewPathOrderToUnit},
    {"movement", "NewPathOrderToUnitAt", "iii>i", GlueImportMovementNewPathOrderToUnitAt},
    {"movement", "NewPathOrderToUnitAtAngle", "iii>i", GlueImportMovementNewPathOrderToUnitAtAngle},
    {"movement", "NewPointMarker", "iii>i", GlueImportMovementNewPointMarker},
    {"movement", "NewRingApproach", "iiiii>i", GlueImportMovementNewRingApproach},
    {"movement", "UnitTarget", "i>i", GlueImportMovementUnitTarget},
    {"network", "BroadcastPlayerEconomy", "i>", GlueImportNetworkBroadcastPlayerEconomy},
    {"network", "BroadcastPlayerInfo", ">", GlueImportNetworkBroadcastPlayerInfo},
    {"network", "SendActivateFlags", "i>", GlueImportNetworkSendActivateFlags},
    {"network", "SendAttach", "iiii>", GlueImportNetworkSendAttach},
    {"network", "SendBuilderLink", "ii>", GlueImportNetworkSendBuilderLink},
    {"network", "SendFeatureBurning", "ii>", GlueImportNetworkSendFeatureBurning},
    {"network", "SendFeatureHit", "iii>", GlueImportNetworkSendFeatureHit},
    {"network", "SendFeatureKilled", "ii>", GlueImportNetworkSendFeatureKilled},
    {"network", "SendFeatureReclaimed", "iii>", GlueImportNetworkSendFeatureReclaimed},
    {"network", "SendNewUnit", "i>", GlueImportNetworkSendNewUnit},
    {"network", "SendOwnershipTransfer", "ii>", GlueImportNetworkSendOwnershipTransfer},
    {"network", "SendProjectileDetonated", "ii>", GlueImportNetworkSendProjectileDetonated},
    {"network", "SendScriptCall", "iiiiiii>i", GlueImportNetworkSendScriptCall},
    {"network", "SendScriptCallByName", "iiiiiii>i", GlueImportNetworkSendScriptCallByName},
    {"network", "SendShareEnergy", "iif>", GlueImportNetworkSendShareEnergy},
    {"network", "SendShareMapInfo", "ii>", GlueImportNetworkSendShareMapInfo},
    {"network", "SendShareMetal", "iif>", GlueImportNetworkSendShareMetal},
    {"network", "SendUnitDamage", "iiiii>", GlueImportNetworkSendUnitDamage},
    {"network", "SendUnitKilled", "iiii>", GlueImportNetworkSendUnitKilled},
    {"network", "SendWeaponFire", "iiiii>", GlueImportNetworkSendWeaponFire},
    {"network", "SendWeaponFireAt", "iii>", GlueImportNetworkSendWeaponFireAt},
    {"orders", "GetOrderFlags", "i>i", GlueImportOrdersGetOrderFlags},
    {"orders", "GetOrderType", "iiii>i", GlueImportOrdersGetOrderType},
    {"pathfinding", "Cancel", "i>", GlueImportPathfindingCancel},
    {"pathfinding", "Submit", "i>", GlueImportPathfindingSubmit},
    {"pathworld", "BeginSearch", "i>", GlueImportPathworldBeginSearch},
    {"pathworld", "EndSearch", "i>", GlueImportPathworldEndSearch},
    {"pathworld", "FirstUnit", "i>i", GlueImportPathworldFirstUnit},
    {"pathworld", "InBounds", "iii>i", GlueImportPathworldInBounds},
    {"pathworld", "IsPlaying", "i>i", GlueImportPathworldIsPlaying},
    {"pathworld", "LastUnit", "i>i", GlueImportPathworldLastUnit},
    {"pathworld", "LineOfSight", "iiiii>i", GlueImportPathworldLineOfSight},
    {"pathworld", "MoveClass", "i>i", GlueImportPathworldMoveClass},
    {"pathworld", "NextUnit", "i>i", GlueImportPathworldNextUnit},
    {"pathworld", "Pass", "iii>i", GlueImportPathworldPass},
    {"pathworld", "PlayerCount", ">i", GlueImportPathworldPlayerCount},
    {"pathworld", "SearchStepLimit", ">i", GlueImportPathworldSearchStepLimit},
    {"pathworld", "Seen", "iii>i", GlueImportPathworldSeen},
    {"pathworld", "SeenSize", "ii>", GlueImportPathworldSeenSize},
    {"pathworld", "TerrainPass", "iii>i", GlueImportPathworldTerrainPass},
    {"pathworld", "Tick", ">i", GlueImportPathworldTick},
    {"pathworld", "UnitState", "ii>i", GlueImportPathworldUnitState},
    {"pathworld", "UnitsPerPlayer", ">i", GlueImportPathworldUnitsPerPlayer},
    {"scripts", "CreateObjectState", "i>i", GlueImportScriptsCreateObjectState},
    {"scripts", "CreatePlayerObjectState", "iii>i", GlueImportScriptsCreatePlayerObjectState},
    {"scripts", "DeleteScript", "i>", GlueImportScriptsDeleteScript},
    {"scripts", "FindScript", "ii>i", GlueImportScriptsFindScript},
    {"scripts", "FreeObjectState", "i>", GlueImportScriptsFreeObjectState},
    {"scripts", "GetAimFromPosition", "iii>", GlueImportScriptsGetAimFromPosition},
    {"scripts", "GetNanoPiecePosition", "ii>", GlueImportScriptsGetNanoPiecePosition},
    {"scripts", "GetPieceAngles", "iii>", GlueImportScriptsGetPieceAngles},
    {"scripts", "GetPieceOffset", "iii>", GlueImportScriptsGetPieceOffset},
    {"scripts", "GetPiecePosition", "iii>", GlueImportScriptsGetPiecePosition},
    {"scripts", "GetSweetSpot", "ii>", GlueImportScriptsGetSweetSpot},
    {"scripts", "NewUnitScript", "i>", GlueImportScriptsNewUnitScript},
    {"scripts", "ObjectStateSize", "i>i", GlueImportScriptsObjectStateSize},
    {"scripts", "QueryScript", "iiiiii>i", GlueImportScriptsQueryScript},
    {"scripts", "SetCob", "ii>", GlueImportScriptsSetCob},
    {"scripts", "SetObjectState", "ii>", GlueImportScriptsSetObjectState},
    {"targeting", "ChooseWeapon", "i>i", GlueImportTargetingChooseWeapon},
    {"targeting", "FindBestTargetIfFireAtWill", "i>i", GlueImportTargetingFindBestTargetIfFireAtWill},
    {"targeting", "FindWeaponTarget", "iii>i", GlueImportTargetingFindWeaponTarget},
    {"targeting", "GetWeaponTargetPos", "iii>i", GlueImportTargetingGetWeaponTargetPos},
    {"targeting", "GetWeaponTargetUnit", "ii>i", GlueImportTargetingGetWeaponTargetUnit},
    {"units", "AddBuildProgress", "iif>i", GlueImportUnitsAddBuildProgress},
    {"units", "AddRepairProgress", "iif>i", GlueImportUnitsAddRepairProgress},
    {"units", "CanLoad", "ii>i", GlueImportUnitsCanLoad},
    {"units", "CanReclaim", "ii>i", GlueImportUnitsCanReclaim},
    {"units", "CanRepair", "ii>i", GlueImportUnitsCanRepair},
    {"units", "CreateUnit", "iiiiii>i", GlueImportUnitsCreateUnit},
    {"units", "FindUnitTypeId", "i>i", GlueImportUnitsFindUnitTypeId},
    {"units", "GetFactoriesInRadius", "iiiii>i", GlueImportUnitsGetFactoriesInRadius},
    {"units", "UnitRefGet", "i>i", GlueImportUnitsUnitRefGet},
    {"units", "UnitRefInit", "iii>", GlueImportUnitsUnitRefInit},
    {"units", "UnitRefSetUnit", "ii>", GlueImportUnitsUnitRefSetUnit},
    {"units", "UnitRefSetValue", "ii>", GlueImportUnitsUnitRefSetValue},
    {"units", "UnitRefUnlink", "i>", GlueImportUnitsUnitRefUnlink},
    {"weapons", "FindTargetableProjectile", "ii>i", GlueImportWeaponsFindTargetableProjectile},
    {"weapons", "FindWeaponByName", "i>i", GlueImportWeaponsFindWeaponByName},
    {"weapons", "GetWeaponRange", "ii>i", GlueImportWeaponsGetWeaponRange},
    {"weapons", "WeaponCanReachPos", "iiii>i", GlueImportWeaponsWeaponCanReachPos},
    {"weapons", "WeaponCanReachUnit", "iii>i", GlueImportWeaponsWeaponCanReachUnit},
};

const GlueRoute kGlueRoutes[] = {
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

}

GlueModule* const* GlueModules(unsigned* count)
{
    *count = sizeof kGlueModules / sizeof kGlueModules[0];
    return kGlueModules;
}

const GlueImport* GlueInterfaceImports(unsigned* count)
{
    *count = sizeof kGlueInterfaceImports / sizeof kGlueInterfaceImports[0];
    return kGlueInterfaceImports;
}

const GlueRoute* GlueRoutes(unsigned* count)
{
    *count = sizeof kGlueRoutes / sizeof kGlueRoutes[0];
    return kGlueRoutes;
}
