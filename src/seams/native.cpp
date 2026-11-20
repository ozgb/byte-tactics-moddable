#include <stddef.h>
#include <vector>

#include "../../interfaces/seams.h"
#include "../../interfaces/scripts.h"
#include "../../interfaces/movement.h"
#include "../../interfaces/network.h"
#include "../../interfaces/mission.h"
#include "../../interfaces/ref.h"
#include "../state/block.h"
#include "../network/player.h"
#include "../game/game.h"
#include "../game/los_helpers.h"
#include "../files/files_impl.h"
#include "../files/files_bank.h"
#include "../game/unit_resources.h"
#include "../units/movement_class.h"
#include "../map/pass_map.h"
#include "../scripts/cob_script.h"
#include "../scripts/cob_types.h"
#include "../scripts/unit_script.h"
#include "../util/vec3.h"
#include "../mission/mission.h"
#include "../map/spatial_bucket.h"
#include "../game/mission_conditions.h"
#include "../scripts/object_state.h"
#include "../units/unit.h"
#include "../ai/pathfinder.h"
#include "../ai/squad_manager.h"
#include "../units/unit_type_set.h"
#include "../units/unit_ref.h"
#include "../units/sfx_occupy.h"
#include "../orders/order.h"
#include "../util/angles.h"

struct Player;
#include "pathfinding_world.h"
#include "../game/economy.h"

#include "../game/game_state.h"
#include "../map/meteors.h"
#include "../game/console_commands.h"
#include "../weapons/weapons.h"
#include "../weapons/weapons_apply.h"
#include "../network/net_game.h"
#include "../map/features.h"

struct Cell;
struct Point16;
struct Feature;
struct Rot16;
struct TntInfo;
struct SweetSpotObjectView;
struct Vec;
struct UnitDef;
struct WeaponDef;
struct Callback;

struct Order;
class MissionType;

#include "../ai/ai_player.h"
#include "../units/factory_index.h"
#include "../units/squads.h"
int __stdcall CanBuildAt(UnitDef* unit, Point16 cell, short type, Player* los);
int __stdcall CanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, Point16 cell, int flags);
struct UnitOrderTypeView;
struct FixedPosView;
#include "../game/players.h"
#include "../units/unit_targets.h"

Projectile* __stdcall FindTargetableProjectile(Unit* unit, unsigned int weapon);
int __stdcall WeaponCanReachPos(Unit* unit, Vec3* from, Vec3* to, int flags);
#include "../weapons/weapon_types.h"
#include "../weapons/explosions.h"
#include "../scripts/piece_queries.h"
Vec3 __stdcall GetPieceOffset(Unit* unit, int piece);
struct ModelBuildList;
#include "../ingame/info_panel.h"

#include "../units/unit_commands.h"

#include "../map/line_of_sight.h"
#include "../game/share_commands.h"
int __stdcall GetWeaponRange(int param1, unsigned int param2);
#include "../map/line_of_sight_index.h"

#include "../map/line_of_sight.h"
#include "../ingame/radar.h"
#include "../map/radar_capture.h"
#include "../ingame/radar_picture.h"

class HapiBank;
#include "../mission/map_list.h"
#include "../map/plot_map.h"
#include "../map/terrain.h"
int __stdcall PickRandomReclaimableResourcesInRadius(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount);
void __stdcall SetupPlayerSlot(int player, char type);
#include "../units/units.h"
struct OwnershipTransferPacket;
#include "../units/unit_types.h"
#include "../units/unit_save.h"
Unit* __stdcall CreateUnit(unsigned char player, unsigned short typeId, Vec3 pos, int param_4, int mode, int param_6);
UnitDef* __stdcall FindUnitType(const char* name);
int __stdcall FindMissionUnit(char* name, std::vector<Unit*>* units, int value);
void __stdcall AttachUnitToPiece(Unit* unit, Unit* target, char piece, char mode);
Angles16 __stdcall GetPieceAngles(Unit* unit, int piece);
#include "../ingame/control_panel.h"
#include "../scripts/piece_queries.h"
#include "../units/unit_position.h"
#include "../game/scheduler.h"
#include "../orders/orders_seam.h"
#include "../movement/movement_seam.h"
#include "../../interfaces/rand_stream.h"
#include "../state/sim_globals.h"




static void __stdcall UpdateAllUnits(void)
{
    SchedUnitsPhase();
}

static void __stdcall UpdateFeatures(void)
{
    SchedFeaturesPhase();
}

static void __stdcall UpdatePlayers(void)
{
    SchedPlayersPhase(RAND_STREAM_MAIN);
}

static void __stdcall UpdatePlayersOnStream(RandStream stream)
{
    SchedPlayersPhase(stream);
}

static int __stdcall FindScriptNative(CobScript* script, const char* name)
{
    return script->FindScript(name);
}

static int __stdcall StartScriptWithArgsNative(CobScript* script, char* name, Callback* callback, int update, int count, int a, int b, int c, int d)
{
    return script->StartScriptWithArgs(name, callback, update, count, a, b, c, d);
}

static int __stdcall StartScriptNative(CobScript* script, const char* name, Callback* callback, int update)
{
    return script->StartScript(name, callback, update);
}

static int __stdcall QueryScriptNative(CobScript* script, char* name, int* p2, int* p3, int* p4, int* p5)
{
    return script->QueryScript(name, p2, p3, p4, p5);
}

static void __stdcall UpdateSquadsNative(int player)
{
    SquadManager* manager = g_game->players[player].ai.get();
    if (manager == nullptr)
        return;
    manager->TickIfActive();
}

static void __stdcall FreeSquadManagerNative(Player* player)
{
    SquadManager* manager = player->ai.get();
    if (manager == nullptr)
        return;
    manager->DeleteTimers();
    delete manager;
    player->ai = nullptr;
}

static void __stdcall CreateSquadManagerNative(Player* player)
{
    player->ai = new (kSim) SquadManager(player);
}

static void __stdcall UnitTypeSetAddTypeOrCategoryNative(UnitTypeSet* self, char* text, int* out)
{
    self->AddTypeOrCategory(text, out);
}

static void __stdcall SetStateBitsNative(Unit* unit, int mask, int set)
{
    unit->SetStateBits(mask, set);
}

static unsigned char __fastcall ChooseWeaponNative(Unit* self)
{
    return self->ChooseWeapon();
}

static int __stdcall GetWeaponTargetPosNative(Unit* unit, Vec3* pos, int weapon)
{
    return GetWeaponTargetPos(unit, (Vec3*)pos, weapon);
}

static void __stdcall GetSweetSpotNative(Unit* unit, Vec3* out)
{
    GetSweetSpot((SweetSpotObjectView*)unit, (int)out);
}

static int __stdcall GetCellMeanHeightNative(Vec3* pos)
{
    return GetCellMeanHeight(pos);
}

static WeaponDef* __stdcall FindWeaponByNameNative(char* name)
{
    return (WeaponDef*)FindWeaponByName(name);
}

static int __stdcall GetWeaponRangeNative(Unit* unit, unsigned int weapon)
{
    return GetWeaponRange((int)unit, weapon);
}

static Mission* TheMission()
{
    return g_game->net.get();
}

static int __stdcall GetGameTypeNative()
{
    return TheMission()->GetGameType();
}

static char* __stdcall GetNameSlotNative(int index)
{
    return TheMission()->GetNameSlot(index);
}

static int __stdcall AdvanceMissionNative()
{
    return TheMission()->AdvanceMission();
}

static char* __stdcall GetCampaignNameNative()
{
    return TheMission()->GetCampaignName();
}

static int __stdcall GetMissionIndexNative()
{
    return TheMission()->GetMissionIndex();
}

static char* __stdcall GetMissionNameNative()
{
    return TheMission()->GetMissionName();
}

static int __stdcall GetStartPositionNative(Vec3* out, int id)
{
    return TheMission()->GetStartPosition(out, id);
}

static void __stdcall LoadCampaignNative(char* file)
{
    TheMission()->LoadCampaign(file);
}

static int __stdcall LoadMissionByNameNative(char* name)
{
    return TheMission()->LoadMissionByName(name);
}

static int __stdcall MissionExistsNative(int index)
{
    return TheMission()->MissionExists(index);
}

static void __stdcall RefreshMapListNative(int multiplayer)
{
    TheMission()->RefreshMapList(multiplayer);
}

static int __stdcall SelectMissionNative(int index)
{
    return TheMission()->SelectMission(index);
}

static int __stdcall ComputeMapChecksumNative()
{
    return TheMission()->ComputeMapChecksum();
}

static int __stdcall GetTerrainLengthNative()
{
    return TheMission()->GetTerrainLength();
}

static int __stdcall CountMissionsNative()
{
    return TheMission()->CountMissions();
}

static int __stdcall GetCampaignMissionNameNative(int index, char* out, int size)
{
    return TheMission()->GetCampaignMissionName(index, out, size);
}

static int __stdcall CountStartPositionsNative()
{
    return TheMission()->CountStartPositions();
}

static char* __stdcall GetBriefingNative()
{
    return TheMission()->GetBriefing();
}

static char* __stdcall GetDescriptionNative()
{
    return TheMission()->GetDescription();
}

static char* __stdcall GetPlanetNative()
{
    return TheMission()->GetPlanet();
}

static int __stdcall GetTerrainSizeTierNative()
{
    return TheMission()->GetTerrainSizeTier();
}

static char* __stdcall GetTranslatedNameNative()
{
    return TheMission()->GetTranslatedName();
}

static int __stdcall HasMissionNameNative()
{
    return TheMission()->HasMissionName() ? 1 : 0;
}

static int __stdcall SelectSchemaNative(int type, TdfFile* parser, char* schema)
{
    return TheMission()->SelectSchema(type, parser, schema);
}

static void __stdcall GetMissionParamsNative(MissionParams* out)
{
    TheMission()->GetParams(out);
}

static int __stdcall CountMissionUnitsNative()
{
    return TheMission()->CountUnits();
}

static int __stdcall GetMissionUnitNative(int index, MissionUnitInfo* out)
{
    return TheMission()->GetUnit(index, out);
}

static void __stdcall SetMissionUnitPositionNative(int index, Vec3* pos)
{
    TheMission()->SetUnitPosition(index, pos);
}

static int __stdcall CountMissionFeaturesNative()
{
    return TheMission()->CountFeatures();
}

static int __stdcall GetMissionFeatureNative(int index, MissionFeatureInfo* out)
{
    return TheMission()->GetFeature(index, out);
}

static char* __stdcall GetMemoryNative()
{
    return TheMission()->GetMemory();
}

static char* __stdcall GetNumPlayersNative()
{
    return TheMission()->GetNumPlayers();
}

static void __fastcall DeactivateConditionsNative(MissionConditions* self)
{
    self->Deactivate();
}

static void __fastcall LoadConditionsNative(MissionConditions* self, int unusedEdx, HapiBank* file)
{
    self->LoadConditions(file);
}

static void __fastcall SaveConditionsNative(MissionConditions* self, int unusedEdx, HapiBank* file)
{
    self->SaveConditions(file);
}

static void __fastcall RegisterConditionsNative(MissionConditions* self, int unusedEdx, TdfFile* mission)
{
    self->RegisterConditions((ConditionParams*)mission);
}

static void __fastcall FreeConditionsNative(MissionConditions* self)
{
    self->FreeConditions();
}

static void __stdcall DeleteConditionsNative(MissionConditions* self)
{
    delete self;
}

static MissionConditions* __stdcall NewConditionsNative(void)
{
    return new (kSim) MissionConditions;
}

static int __fastcall CanLoadNative(Unit* self, int unusedEdx, Unit* target)
{
    return self->CanLoad(target);
}

static int __fastcall CanReclaimNative(Unit* self, int unusedEdx, Unit* target)
{
    return self->CanReclaim(target);
}

static int __fastcall CanRepairNative(Unit* self, int unusedEdx, Unit* target)
{
    return self->CanRepair(target);
}

static void __fastcall UnitRefUnlinkNative(UnitRef* self)
{
    self->Unlink();
}

static int __stdcall StartScriptWithArgsByIndexNative(CobScript* script, int index, Callback* callback, int update, int count, int a, int b, int c, int d)
{
    return script->StartScriptWithArgsByIndex(index, callback, update, count, a, b, c, d);
}

static void __stdcall NewUnitScriptNative(Unit* unit)
{
    NewUnitScript(unit);
}

static void __stdcall SetCobNative(CobScript* script, ScriptTable* data)
{
    script->SetCob(data);
}

static ModelState* __stdcall CreatePlayerObjectStateNative(Object3do* model, ScriptTable* data, Unit* owner)
{
    return CreatePlayerObjectState(model, (ModelBuildList*)data, owner);
}

static void __stdcall SetObjectStateNative(UnitScript* script, ModelState* state)
{
    script->SetObjectState((ObjectState*)state);
}

static void __stdcall RunScriptsNative(CobScript* script, int elapsed)
{
    script->RunScripts(elapsed);
}

static void __stdcall SaveScriptStateNative(CobScript* script, HapiBank* file)
{
    script->SaveScriptState(file);
}

static int __stdcall LoadScriptStateNative(CobScript* script, HapiBank* file)
{
    return script->LoadScriptState(file);
}

static void __stdcall DeleteScriptNative(CobScript* script)
{
    delete script;
}

static void __stdcall InitObjectStateNative(ModelState* state, Object3do* model)
{
    InitObjectState(state, model);
}

static void __fastcall UpdateSfxOccupyNative(UnitMotion* self, int unusedEdx, Unit* unit)
{
    ((SfxOccupyTracker*)self)->UpdateSfxOccupy(unit);
}

static int __stdcall PassMapGetNative(int moveClass, int x, int y)
{
    return g_passMaps.entries[moveClass].Get(x, y);
}

static int __stdcall PassMapInBoundsNative(int moveClass, unsigned int x, unsigned int y)
{
    return g_passMaps.entries[moveClass].InBounds(x, y);
}

static void __stdcall RefreshMovedUnitsNative(int moveClass, Unit* unit)
{
    g_passMaps.entries[moveClass].RefreshMovedUnits((UnitPassMapView*)unit);
}

static void __stdcall RefreshUnitIfStaleNative(int moveClass, Unit* unit)
{
    g_passMaps.entries[moveClass].RefreshUnitIfStale((UnitPassMapView*)unit);
}

static void __stdcall UnitRefInitNative(UnitRef* ref, Unit* unit, OrderBase* value)
{
    ((PathOrderAttach*)ref)->PathOrderAttach::PathOrderAttach(unit, value);
}

static void __stdcall UnitRefSetUnitNative(UnitRef* ref, Unit* unit)
{
    ((PathOrderAttach*)ref)->SetUnit(unit);
}

Seams g_seams;

static WindMeteorsSeam s_wind_meteors;
static EconomySeam s_economy;
static FeaturesSeam s_features;
static AiSeam s_ai;
static PathfindingSeam s_pathfinding;
static WeaponsSeam s_weapons;
static TargetingSeam s_targeting;
static LosRadarSeam s_los_radar;
static OrdersSeam s_orders;
static UnitsSeam s_units;
static ScriptsSeam s_scripts;
static MovementSeam s_movement;
static PlayersSeam s_players;
static MapSeam s_map;
static MissionSeam s_mission;
static FilesSeam s_files;
static NetworkSeam s_network;

const PathfindingSeam* DefaultPathfindingSeam()
{
    return &s_pathfinding;
}

const EconomySeam* DefaultEconomySeam()
{
    return &s_economy;
}

const PlayersSeam* DefaultPlayersSeam()
{
    return &s_players;
}

const LosRadarSeam* DefaultLosRadarSeam()
{
    return &s_los_radar;
}

static_assert(sizeof(HapiBank) == sizeof(AccountBank), "HapiBank and AccountBank must have the same size");
static_assert(offsetof(HapiBank, bank) == offsetof(AccountBank, bank), "HapiBank and AccountBank must keep the bank pointer at the same offset");

static HapiBank* __stdcall BankInitNative(HapiBank* self)
{
    return (HapiBank*)((AccountBank*)self)->InitBank();
}

static void __stdcall BankCloseNative(HapiBank* self)
{
    ((AccountBank*)self)->CloseBank();
}

static void __stdcall BankNewNative(HapiBank* self)
{
    ((AccountBank*)self)->NewBank();
}

static int __stdcall BankOpenNative(HapiBank* self, char* filename, char* name, char* account)
{
    return ((AccountBank*)self)->OpenBank(filename, name, account);
}

static int __stdcall BankSaveNative(HapiBank* self, char* filename, char* name, int compress, int audit)
{
    return ((AccountBank*)self)->SaveBank(filename, name, compress, audit);
}

static int __stdcall BankOpenAccountNative(HapiBank* self, char* name)
{
    return ((AccountBank*)self)->OpenAccount(name);
}

static int __stdcall BankSetIntegerNative(HapiBank* self, const char* name, int value)
{
    return ((AccountBank*)self)->SetIntegerItem(name, value);
}

static int __stdcall BankSetDoubleNative(HapiBank* self, const char* name, double value)
{
    return ((AccountBank*)self)->SetDoubleItem(name, value);
}

static int __stdcall BankSetStringNative(HapiBank* self, const char* name, char* value)
{
    return ((AccountBank*)self)->SetStringItem(name, value);
}

static int __stdcall BankGetIntegerNative(HapiBank* self, char* name, int def)
{
    return ((AccountBank*)self)->GetIntegerItem(name, def);
}

static double __stdcall BankGetDoubleNative(HapiBank* self, char* name, double def)
{
    return ((AccountBank*)self)->GetDoubleItem(name, def);
}

static char* __stdcall BankGetStringNative(HapiBank* self, char* name, char* def)
{
    return ((AccountBank*)self)->GetStringItem(name, def);
}

static int __stdcall BankHasItemNative(HapiBank* self, const char* name)
{
    return ((AccountBank*)self)->HasItem(name);
}

static int __stdcall BankOpenNumberedBoxNative(HapiBank* self, int number)
{
    return ((AccountBank*)self)->OpenNumberedBox(number);
}

static int __stdcall BankOpenNamedBoxNative(HapiBank* self, char* name)
{
    return ((AccountBank*)self)->OpenNamedBox(name);
}

static int __stdcall BankGetBoxSizeNative(HapiBank* self)
{
    return ((AccountBank*)self)->GetBoxSize();
}

static void __stdcall BankSeekBoxNative(HapiBank* self, int pos)
{
    ((AccountBank*)self)->SeekBox(pos);
}

static int __stdcall BankReadBoxNative(HapiBank* self, void* dst, int len)
{
    return ((AccountBank*)self)->ReadBox(dst, len);
}

static int __stdcall BankWriteBoxNative(HapiBank* self, const void* src, int len)
{
    return ((AccountBank*)self)->WriteBox(src, len);
}

static void __stdcall ListDirectoryNative(const char* path, int dirsOnly, FileNameList* list)
{
    ListDirectory(path, dirsOnly, (std::vector<StringRef>*)list);
}

static void __stdcall FindFilesRecursiveNative(char* path, const char* pat, FileNameList* tree, int state, int recursive)
{
    FindFilesRecursive(path, pat, (std::vector<StringRef>*)tree, state, recursive);
}

void InstallNativeSeams()
{
    s_wind_meteors.UpdateWind = UpdateWindOnStream;
    s_wind_meteors.UpdateMeteors = UpdateMeteors;
    s_wind_meteors.Apply = WindMeteorsApply;
    s_wind_meteors.InitMeteors = InitMeteors;
    s_wind_meteors.EmptyShutdownPreCleanup = EmptyShutdownPreCleanup;
    s_wind_meteors.LoadMeteorDefaults = LoadMeteorDefaults;
    s_wind_meteors.SaveMeteors = SaveMeteors;
    s_wind_meteors.LoadMeteors = LoadMeteors;
    s_economy.RegionSize = EconomyRegionSize;
    s_economy.Init = EconomyInit;
    s_economy.Shutdown = EconomyShutdown;
    s_economy.Apply = EconomyApply;
    s_economy.Spend = EconomySpend;
    s_economy.UpdatePlayerEconomy = EconomyUpdatePlayer;
    s_economy.UpdateMetalExtraction = EconomyUpdateMetalExtraction;
    s_economy.UpdateResourceSharing = EconomyUpdateResourceSharing;
    s_economy.RequestGranted = EconomyRequestGranted;
    s_economy.GetEnergyUse = EconomyGetEnergyUse;
    s_economy.UnitAccount = EconomyUnitAccount;
    s_economy.WriteState = EconomyWriteState;
    s_features.UpdateFeatures = UpdateFeatures;
    s_features.LoadFeatureFileList = LoadFeatureFileList;
    s_features.FreeFeatureFileList = FreeFeatureFileList;
    s_features.NopAfterFeatureEnum = NopAfterFeatureEnum;
    s_features.ResolveFeatureLinks = ResolveFeatureLinks;
    s_features.InitFeatureAnimPool = InitFeatureAnimPool;
    s_features.FreeFeaturePool = FreeFeaturePool;
    s_features.PlaceMissionFeatures = PlaceMissionFeatures;
    s_features.StampFeatureMetal = StampFeatureMetal;
    s_features.GetCellFeature = GetCellFeature;
    s_features.FindOrLoadFeatureType = FindOrLoadFeatureType;
    s_features.GetFootprintCentre = GetFootprintCentre;
    s_features.SaveFeatures = SaveFeatures;
    s_features.LoadFeatures = LoadFeatures;
    s_features.Apply = ApplyEvent;
    s_features.FindFeatureAtPos = FindFeatureAtPos;
    s_features.Place = PlaceEvent;
    s_features.FeatureTypeCount = FeatureTypeCount;
    s_features.AnimateFeatureType = AnimateFeatureType;
    s_features.ScanFeatureCell = ScanFeatureCell;
    s_features.FirstFeatureSpot = FirstFeatureSpot;
    s_features.UpdateFeatureSpot = UpdateFeatureSpot;
    s_targeting.Apply = ApplyTargetingEvent;
    s_targeting.GetWeaponTargetUnit = GetWeaponTargetUnit;
    s_targeting.GetWeaponTargetPos = GetWeaponTargetPosNative;
    s_targeting.FindWeaponTarget = FindWeaponTarget;
    s_targeting.FindBestTargetIfFireAtWill = FindBestTargetIfFireAtWill;
    s_targeting.ChooseWeapon = ChooseWeaponNative;
    s_los_radar.Apply = ApplyLosRadarEvent;
    s_los_radar.GetMapCell = GetMapCell;
    s_los_radar.ClampWorldPosToTerrain = ClampWorldPosToTerrain;
    s_los_radar.ExpireEyeballs = ExpireEyeballs;
    s_los_radar.FreeMapResources = FreeMapResources;
    s_los_radar.FreeRadar = FreeRadar;
    s_los_radar.InitRadar = InitRadar;
    s_los_radar.LoadTntMap = LoadTntMap;
    s_los_radar.IsUnitVisibleToPlayer = IsUnitVisibleToPlayer;
    s_los_radar.GetMapCellAtPosition = GetMapCellAtPosition;
    s_los_radar.GetOriginCellAtPosition = GetOriginCellAtPosition;
    s_los_radar.UpdateUnitLineOfSight = UpdateUnitLineOfSight;
    s_los_radar.RecordRadarUnits = RecordRadarUnits;
    s_los_radar.RecordRadarMapped = RecordRadarMapped;
    s_los_radar.UpdateSensorRadarAndCloak = UpdateSensorRadarAndCloak;
    s_los_radar.SoundHeardMask = SoundHeardMask;
    s_pathfinding.RegionSize = PathfindingRegionSize;
    s_pathfinding.Init = PathfindingInit;
    s_pathfinding.Shutdown = PathfindingShutdown;
    s_pathfinding.Submit = PathfindingSubmit;
    s_pathfinding.Cancel = PathfindingCancel;
    s_pathfinding.CellsChanged = PathfindingCellsChanged;
    s_pathfinding.Step = PathfindingStep;
    s_pathfinding.Poll = PathfindingPoll;
    s_pathfinding.WriteState = PathfindingWriteState;
    s_pathfinding.SetSearchScale = PathfindingSetBaseScale;
    s_pathfinding.DebugCell = PathfindingDebugCell;
    s_map.PassMapGet = PassMapGetNative;
    s_map.PassMapInBounds = PassMapInBoundsNative;
    s_map.RefreshMovedUnits = RefreshMovedUnitsNative;
    s_map.RefreshUnitIfStale = RefreshUnitIfStaleNative;
    s_los_radar.HasReadyUnitInRange = HasReadyUnitInRange;
    s_los_radar.IsPointVisible = IsPointVisible;
    s_los_radar.GetVisibleEnemiesInRadius = GetVisibleEnemiesInRadius;
    s_los_radar.GetVisibleEnemiesInRadiusList = GetVisibleEnemiesInRadiusList;
    s_los_radar.GetSeenUnitsInRadius = GetSeenUnitsInRadius;
    s_los_radar.CreateUnitIndex = CreateUnitIndex;
    s_los_radar.FreeUnitIndex = FreeUnitIndex;
    s_los_radar.RefreshUnitIndex = RefreshUnitIndex;
    s_los_radar.RegionSize = LosRadarRegionSize;
    s_los_radar.Init = LosRadarInit;
    s_los_radar.Shutdown = LosRadarShutdown;
    s_los_radar.SightCache = LosRadarSightCache;
    s_los_radar.WriteState = LosRadarWriteState;
    s_units.GetFactoriesInRadius = GetFactoriesInRadius;
    s_units.CreateFactoryIndex = CreateFactoryIndex;
    s_units.FreeFactoryIndex = FreeFactoryIndex;
    s_units.RefreshFactoryIndex = RefreshFactoryIndex;
    s_units.WriteState = UnitsWriteState;
    s_map.GetGroundHeight = GetGroundHeight;
    s_scripts.GetSweetSpot = GetSweetSpotNative;
    s_scripts.FindScript = FindScriptNative;
    s_scripts.StartScriptWithArgs = StartScriptWithArgsNative;
    s_units.SetStateBits = SetStateBitsNative;
    s_map.GetCellMeanHeight = GetCellMeanHeightNative;
    s_weapons.FindWeaponByName = FindWeaponByNameNative;
    s_weapons.GetWeaponRange = GetWeaponRangeNative;
    s_weapons.WeaponCanReachUnit = WeaponCanReachUnit;
    s_weapons.FindTargetableProjectile = FindTargetableProjectile;
    s_weapons.WeaponCanReachPos = WeaponCanReachPos;
    s_weapons.AllocWeaponArray = AllocWeaponArray;
    s_weapons.FreeWeaponArray = FreeWeaponArray;
    s_weapons.LoadWeaponTypes = LoadWeaponTypes;
    s_weapons.FreeWeaponTypes = FreeWeaponTypes;
    s_weapons.InitExplosions = InitExplosions;
    s_weapons.FreeExplosions = FreeExplosions;
    s_weapons.UpdateUnitWeapons = UpdateUnitWeapons;
    s_ai.Apply = AiApply;
    s_ai.WriteState = AiWriteState;
    s_ai.DumpPlayerAI = DumpPlayerAI;
    s_ai.ResetAIPlayers = ResetAIPlayers;
    s_ai.CreatePlayerAI = CreatePlayerAI;
    s_ai.DestroyPlayerAI = DestroyPlayerAI;
    s_ai.UpdatePlayerAI = UpdatePlayerAI;
    s_ai.RebuildFeatureCells = RebuildFeatureCells;
    s_ai.ParseDownloadableAiWeightScripts = ParseDownloadableAiWeightScripts;
    s_ai.ReparseAiWeightScriptsIfLimitNotSticky = ReparseAiWeightScriptsIfLimitNotSticky;
    s_ai.CreateSquads = CreateSquads;
    s_ai.FreeSquads = FreeSquads;
    s_ai.UpdateSquads = UpdateSquadsNative;
    s_ai.FreeSquadManager = FreeSquadManagerNative;
    s_ai.CreateSquadManager = CreateSquadManagerNative;
    s_mission.GetGameType = GetGameTypeNative;
    s_mission.GetNameSlot = GetNameSlotNative;
    s_mission.SetMissionType = SetMissionType;
    s_mission.FreeOtaEnumCacheAndMission = FreeOtaEnumCacheAndMission;
    s_mission.AdvanceMission = AdvanceMissionNative;
    s_mission.GetCampaignName = GetCampaignNameNative;
    s_mission.GetMissionIndex = GetMissionIndexNative;
    s_mission.GetMissionName = GetMissionNameNative;
    s_mission.GetStartPosition = GetStartPositionNative;
    s_mission.LoadCampaign = LoadCampaignNative;
    s_mission.LoadMissionByName = LoadMissionByNameNative;
    s_mission.MissionExists = MissionExistsNative;
    s_mission.RefreshMapList = RefreshMapListNative;
    s_mission.SelectMission = SelectMissionNative;
    s_mission.ComputeMapChecksum = ComputeMapChecksumNative;
    s_mission.GetTerrainLength = GetTerrainLengthNative;
    s_mission.CountMissions = CountMissionsNative;
    s_mission.GetCampaignMissionName = GetCampaignMissionNameNative;
    s_mission.CountStartPositions = CountStartPositionsNative;
    s_mission.GetBriefing = GetBriefingNative;
    s_mission.GetDescription = GetDescriptionNative;
    s_mission.GetPlanet = GetPlanetNative;
    s_mission.GetTerrainSizeTier = GetTerrainSizeTierNative;
    s_mission.GetTranslatedName = GetTranslatedNameNative;
    s_mission.HasMissionName = HasMissionNameNative;
    s_mission.SelectSchema = SelectSchemaNative;
    s_mission.GetMissionParams = GetMissionParamsNative;
    s_mission.CountMissionUnits = CountMissionUnitsNative;
    s_mission.GetMissionUnit = GetMissionUnitNative;
    s_mission.SetMissionUnitPosition = SetMissionUnitPositionNative;
    s_mission.CountMissionFeatures = CountMissionFeaturesNative;
    s_mission.GetMissionFeature = GetMissionFeatureNative;
    s_mission.GetMemory = GetMemoryNative;
    s_mission.GetNumPlayers = GetNumPlayersNative;
    s_map.Apply = ApplyMapEvent;
    s_map.AllocateBuckets = MapAllocateBuckets;
    s_map.FreeBuckets = MapFreeBuckets;
    s_map.WriteState = MapWriteState;
    s_map.UnitsInRange = UnitsInRange;
    s_map.CanBuildAt = CanBuildAt;
    s_map.CanPlaceUnitFootprint = CanPlaceUnitFootprint;
    s_players.Apply = PlayersApply;
    s_players.LoadDefaultAIScript = LoadDefaultAIScript;
    s_units.NotifyUnitRefs = NotifyUnitRefs;
    s_units.UnitTypeSetAddTypeOrCategory = UnitTypeSetAddTypeOrCategoryNative;
    s_scripts.StartScript = StartScriptNative;
    s_scripts.QueryScript = QueryScriptNative;
    s_scripts.GetAimFromPosition = (void (__stdcall *)(Unit *, Vec3 *, unsigned char))GetAimFromPosition;
    s_scripts.GetPieceOffset = GetPieceOffset;
    s_scripts.GetPiecePosition = (Vec3 (__stdcall *)(Unit *, int))GetPiecePosition;
    s_units.MarkRecentlyDamaged = (void (__stdcall *)(Unit *))MarkRecentlyDamaged;
    s_map.BuildAllPassMaps = BuildAllPassMaps;
    s_map.FreePassMaps = FreePassMaps;
    s_map.LoadMappingData = LoadMappingData;
    s_map.LoadMetalPlotmap = LoadMetalPlotmap;
    s_map.LoadPlayerFeaturesPlotmap = LoadPlayerFeaturesPlotmap;
    s_map.SaveMappingData = SaveMappingData;
    s_map.SaveMetalPlotmap = SaveMetalPlotmap;
    s_map.SavePlayerFeaturesPlotmap = SavePlayerFeaturesPlotmap;
    s_map.SnapWorldPosToFootprint = SnapWorldPosToFootprint;
    s_map.CanPlaceFootprintAt = CanPlaceFootprintAt;
    s_map.IsPadSlotFree = IsPadSlotFree;
    s_map.PickRandomReclaimableResourcesInRadius = PickRandomReclaimableResourcesInRadius;
    s_players.RegionSize = PlayersRegionSize;
    s_players.Init = PlayersInit;
    s_players.Shutdown = PlayersShutdown;
    s_players.WriteState = PlayersWriteState;
    s_players.InitPlayers = InitPlayers;
    s_players.FreePlayers = FreePlayers;
    s_players.InitPlayerResources = InitPlayerResources;
    s_players.LoadPlayerControllers = LoadPlayerControllers;
    s_players.LoadPlayers = LoadPlayers;
    s_players.SavePlayers = SavePlayers;
    s_players.RebuildAIFeatureCells = RebuildAIFeatureCells;
    s_players.SetupPlayerSlot = SetupPlayerSlot;
    s_players.UpdateBlink = UpdateBlink;
    s_players.DeactivateConditions = DeactivateConditionsNative;
    s_players.LoadConditions = LoadConditionsNative;
    s_players.SaveConditions = SaveConditionsNative;
    s_players.RegisterConditions = RegisterConditionsNative;
    s_players.FreeConditions = FreeConditionsNative;
    s_players.DeleteConditions = DeleteConditionsNative;
    s_players.NewConditions = NewConditionsNative;
    s_players.UpdatePlayersOnStream = UpdatePlayersOnStream;
    s_players.UpdatePlayerHead = UpdatePlayerHead;
    s_players.UpdatePlayerBody = UpdatePlayerBody;
    s_players.UpdatePlayerTail = UpdatePlayerTail;
    s_players.UpdatePlayersFinish = UpdatePlayersFinish;
    s_units.AllocateUnitMemory = AllocateUnitMemory;
    s_units.FreeUnitMemory = FreeUnitMemory;
    s_units.FreeUnitInfo = FreeUnitInfo;
    s_units.RefreshUnitInfo = RefreshUnitInfo;
    s_units.LoadUnitTypes = LoadUnitTypes;
    s_units.FreeUnitTypes = FreeUnitTypes;
    s_units.LoadDownloadMenus = LoadDownloadMenus;
    s_units.FreeDownloadMenus = FreeDownloadMenus;
    s_units.FreeMovementClasses = FreeMovementClasses;
    s_units.InitUnitCategories = InitUnitCategories;
    s_units.FreeUnitCategories = FreeUnitCategories;
    s_units.KillAllUnits = KillAllUnits;
    s_units.KillUnitsOfType = KillUnitsOfType;
    s_units.ReloadUnitType = ReloadUnitType;
    s_units.KillUnit = KillUnit;
    s_units.LoadUnits = LoadUnits;
    s_units.SaveUnits = SaveUnits;
    s_units.LoadUnit = LoadUnit;
    s_units.CreateUnit = CreateUnit;
    s_units.FindUnitType = FindUnitType;
    s_units.FindUnitTypeId = FindUnitTypeId;
    s_units.FindMissionUnit = FindMissionUnit;
    s_units.AttachUnitToPiece = AttachUnitToPiece;
    s_scripts.GetNanoPiecePosition = (void (__stdcall *)(Unit *, Vec3 *))GetNanoPiecePosition;
    s_scripts.GetPieceAngles = GetPieceAngles;
    s_units.AddBuildProgress = AddBuildProgress;
    s_units.AddRepairProgress = AddRepairProgress;
    s_units.ApplyUnfinishedBuildDecay = ApplyUnfinishedBuildDecay;
    s_units.FinishConstruction = FinishConstruction;
    s_units.SetUnitPosition = SetUnitPosition;
    s_units.CanLoad = CanLoadNative;
    s_units.CanReclaim = CanReclaimNative;
    s_units.CanRepair = CanRepairNative;
    s_units.UnitRefUnlink = UnitRefUnlinkNative;
    s_units.UnitRefInit = UnitRefInitNative;
    s_units.UnitRefSetUnit = UnitRefSetUnitNative;
    s_units.GetMovementLimits = GetMovementLimits;
    s_scripts.StartScriptWithArgsByIndex = StartScriptWithArgsByIndexNative;
    s_units.UpdateSfxOccupy = UpdateSfxOccupyNative;
    s_scripts.PoseUnitModel = PoseUnitModel;
    s_scripts.CreateObjectState = CreateObjectState;
    s_scripts.FreeObjectState = FreeObjectState;
    s_scripts.UpdateObjectState = UpdateObjectState;
    s_orders.Apply = OrdersApply;
    s_orders.RunOrders = OrdersRunOrders;
    s_orders.RunSecondaryOrders = OrdersRunSecondaryOrders;
    s_orders.GetOrderFlags = OrdersGetOrderFlags;
    s_orders.GetOrderType = OrdersGetOrderType;
    s_orders.MissionTypeFromName = OrdersMissionTypeFromName;
    s_orders.RegisterAllOrderTypes = OrdersRegisterAllOrderTypes;
    s_orders.ClearOrderTypeTable = OrdersClearOrderTypeTable;
    s_orders.LoadOrders = OrdersLoadOrders;
    s_orders.SaveOrders = OrdersSaveOrders;
    s_orders.WriteState = OrdersWriteState;
    s_movement.Apply = MovementApply;
    s_movement.UpdateMotion = MovementUpdateMotion;
    s_movement.ApplyPathEvent = MovementApplyPathEvent;
    s_movement.GetMotionState = MovementGetMotionState;
    s_movement.ReadNetState = MovementReadNetState;
    s_movement.HasNetState = MovementHasNetState;
    s_movement.WriteNetState = MovementWriteNetState;
    s_movement.NewPathOrderAt = MovementNewPathOrderAt;
    s_movement.NewPathOrderToUnit = MovementNewPathOrderToUnit;
    s_movement.NewPathOrderToUnitAt = MovementNewPathOrderToUnitAt;
    s_movement.NewPathOrderToUnitAtAngle = MovementNewPathOrderToUnitAtAngle;
    s_movement.NewAirManeuverOrder = MovementNewAirManeuverOrder;
    s_movement.NewApproachRadius = MovementNewApproachRadius;
    s_movement.NewRingApproach = MovementNewRingApproach;
    s_movement.NewPointMarker = MovementNewPointMarker;
    s_movement.LoadTarget = MovementLoadTarget;
    s_movement.SaveTarget = MovementSaveTarget;
    s_movement.TargetType = MovementTargetType;
    s_movement.UnitTarget = MovementUnitTarget;
    s_movement.WriteState = MovementWriteState;
    s_units.UpdateUnitHead = UpdateUnitHead;
    s_units.UpdateUnitBody = UpdateUnitBody;
    s_units.UpdateUnitTail = UpdateUnitTail;
    s_units.UnitRefGet = UnitRefGet;
    s_units.UnitRefSetValue = UnitRefSetValue;
    s_scripts.ObjectStateSize = ObjectStateSize;
    s_scripts.NewUnitScript = NewUnitScriptNative;
    s_scripts.SetCob = SetCobNative;
    s_scripts.CreatePlayerObjectState = CreatePlayerObjectStateNative;
    s_scripts.SetObjectState = SetObjectStateNative;
    s_scripts.RunScripts = RunScriptsNative;
    s_scripts.SaveScriptState = SaveScriptStateNative;
    s_scripts.LoadScriptState = LoadScriptStateNative;
    s_scripts.DeleteScript = DeleteScriptNative;
    s_scripts.InitObjectState = InitObjectStateNative;
    s_weapons.UpdateProjectiles = UpdateProjectiles;
    s_weapons.UpdateExplosions = UpdateExplosions;
    s_weapons.Apply = WeaponsApply;
    s_weapons.LoadState = WeaponsLoadState;
    s_weapons.WriteState = WeaponsWriteState;
    s_units.UpdateAllUnits = UpdateAllUnits;
    s_players.UpdatePlayers = UpdatePlayers;
    s_files.OpenRead = HAPI_OpenFileRead;
    s_files.OpenAppend = HAPI_OpenFileAppend;
    s_files.Create = HAPI_CreateFile;
    s_files.Close = HAPI_CloseFile;
    s_files.IsInArchive = HAPI_IsInArchive;
    s_files.Seek = HAPI_SeekFile;
    s_files.Tell = HAPI_TellFile;
    s_files.Read = HAPI_readfromfile;
    s_files.Write = HAPI_WriteFile;
    s_files.Length = HAPI_FileLength;
    s_files.LengthByName = HAPI_FileLengthByName;
    s_files.ReadAt = HAPI_ReadFileAt;
    s_files.Load = HAPI_LoadFile;
    s_files.LoadOpenFile = HAPI_LoadOpenFile;
    s_files.WriteBuffer = WriteBufferToFile;
    s_files.Remove = RemoveFile;
    s_files.Rename = RenameFile;
    s_files.GetCurrentDriveLetter = GetCurrentDriveLetter;
    s_files.ChangeDrive = ChangeDrive;
    s_files.GetDriveDirectory = GetDriveDirectory;
    s_files.ChangeDirectory = ChangeDirectory;
    s_files.MatchWildcard = MatchWildcard;
    s_files.FindFirst = HAPI_FindFirst;
    s_files.FindNext = HAPI_FindNext;
    s_files.FindClose = HAPI_FindClose;
    s_files.CountDirectoryEntries = CountDirectoryEntries;
    s_files.ListDirectory = ListDirectoryNative;
    s_files.FindFilesRecursive = FindFilesRecursiveNative;
    s_files.MakeDirectoryPath = MakeDirectoryPath;
    s_files.SaveStartDirectory = SaveStartDirectory;
    s_files.GetStartDirectory = GetStartDirectory;
    s_files.AddArchive = HAPI_AddArchive;
    s_files.CloseArchive = HAPI_CloseArchive;
    s_files.DropMissingArchives = HAPI_DropMissingArchives;
    s_files.ResolveShadowedFiles = HAPI_ResolveShadowedFiles;
    s_files.LzssCompress = LzssCompress;
    s_files.LzssExpand = LzssExpand;
    s_files.LzssDisablePreset = LzssDisablePreset;
    s_files.LzssEnablePreset = LzssEnablePreset;
    s_files.BankInit = BankInitNative;
    s_files.BankClose = BankCloseNative;
    s_files.BankNew = BankNewNative;
    s_files.BankOpen = BankOpenNative;
    s_files.BankSave = BankSaveNative;
    s_files.BankOpenAccount = BankOpenAccountNative;
    s_files.BankSetInteger = BankSetIntegerNative;
    s_files.BankSetDouble = BankSetDoubleNative;
    s_files.BankSetString = BankSetStringNative;
    s_files.BankGetInteger = BankGetIntegerNative;
    s_files.BankGetDouble = BankGetDoubleNative;
    s_files.BankGetString = BankGetStringNative;
    s_files.BankHasItem = BankHasItemNative;
    s_files.BankOpenNumberedBox = BankOpenNumberedBoxNative;
    s_files.BankOpenNamedBox = BankOpenNamedBoxNative;
    s_files.BankGetBoxSize = BankGetBoxSizeNative;
    s_files.BankSeekBox = BankSeekBoxNative;
    s_files.BankReadBox = BankReadBoxNative;
    s_files.BankWriteBox = BankWriteBoxNative;
    s_network.BroadcastPlayerInfo = BroadcastPlayerInfo;
    s_network.SendNewUnit = SendNewUnit;
    s_network.SendBuilderLink = BroadcastBuilderLink;
    s_network.SendUnitKilled = NetSendUnitKilled;
    s_network.SendUnitDamage = NetSendUnitDamage;
    s_network.SendAttach = NetSendAttach;
    s_network.SendOwnershipTransfer = NetSendOwnershipTransfer;
    s_network.SendActivateFlags = NetSendActivateFlags;
    s_network.SendWeaponFire = NetSendWeaponFire;
    s_network.SendWeaponFireAt = NetSendWeaponFireAt;
    s_network.SendProjectileDetonated = NetSendProjectileDetonated;
    s_network.SendFeatureHit = NetSendFeatureHit;
    s_network.SendFeatureKilled = NetSendFeatureKilled;
    s_network.SendFeatureBurning = NetSendFeatureBurning;
    s_network.SendFeatureReclaimed = NetSendFeatureReclaimed;
    s_network.SendScriptCall = SendScriptCall;
    s_network.SendScriptCallByName = SendScriptCallByName;
    s_network.SendShareMetal = NetSendShareMetal;
    s_network.SendShareEnergy = NetSendShareEnergy;
    s_network.SendShareMapInfo = SendShareMapInfo;
    s_network.BroadcastPlayerEconomy = NetBroadcastPlayerEconomy;
    s_network.SendUnitStates = NetSendUnitStates;
    g_seams.wind_meteors = &s_wind_meteors;
    g_seams.economy = &s_economy;
    g_seams.features = &s_features;
    g_seams.ai = &s_ai;
    g_seams.pathfinding = &s_pathfinding;
    g_seams.weapons = &s_weapons;
    g_seams.targeting = &s_targeting;
    g_seams.los_radar = &s_los_radar;
    g_seams.orders = &s_orders;
    g_seams.units = &s_units;
    g_seams.players = &s_players;
    g_seams.map = &s_map;
    g_seams.scripts = &s_scripts;
    g_seams.movement = &s_movement;
    g_seams.mission = &s_mission;
    g_seams.files = &s_files;
    g_seams.network = &s_network;
    g_seams.pathworld = NativePathWorld();
}
