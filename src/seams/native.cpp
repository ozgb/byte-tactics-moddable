#include <stddef.h>
#include <vector>

#include "../../interfaces/seams.h"
#include "../files/files_impl.h"
#include "../files/files_bank.h"
#include "../game/unit_resources.h"
#include "../units/movement_class.h"
#include "../units/cob_script.h"
#include "../util/vec3.h"
#include "../map/mission.h"
#include "../map/spatial_bucket.h"
#include "../game/mission_conditions.h"
#include "../units/unit.h"
#include "../ai/pathfinder.h"
#include "../ai/ai_search_goal.h"
#include "../orders/order_fx.h"
#include "../ai/squad_manager.h"
#include "../units/unit_type_set.h"
#include "../units/unit_ref.h"
#include "../units/sfx_occupy.h"
#include "../orders/unit_motion.h"
#include "../orders/order.h"
#include "../util/angles.h"

struct Player;

class MeteorParams {
public:
    void LoadMeteorDefaults();
};

#include "../game/game_state.h"
#include "../map/meteors.h"
#include "../game/console_commands.h"
#include "../weapons/weapons.h"
#include "../network/net_game.h"
#include "../map/features.h"

struct Cell;
struct Point16;
struct Feature;
struct Weapon;
struct Spot;
struct Rot16;
struct List_00421f20;
struct Pos_00485070;
struct Obj_0043e3c0;
struct Vec3_0049a0c0;
struct Vec3_0048a1e0;
struct Vec;
struct UnitDef;
struct WeaponDef;
struct Callback;

struct Order;
struct _iobuf;
struct Los_0047d2e0;
class MissionType;

#include "../ai/ai_player.h"
void __stdcall GetFactoriesInRadius(int player, const Vec* pos, int radius, std::vector<Unit*>* out);
#include "../units/squads.h"
int __stdcall CanBuildAt(UnitDef* unit, Point16 cell, short type, Los_0047d2e0* los);
int __stdcall CanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, Point16 cell, int flags);
void __stdcall AddOrder(MissionType kind, int remove, Unit* owner, Unit* target, Vec3* pos, int param_6, int param_7);
#include "../orders/order_list.h"
#include "../orders/order_queue.h"
MissionType __stdcall GetOrderType(unsigned char mode, Unit* unit, Unit* target, Vec3* pos);
#include "../orders/order_targets.h"
float __stdcall GetEnergyIncome(Player* player);
float __stdcall GetMetalIncome(Player* player);
#include "../game/players.h"
#include "../units/unit_targets.h"

Projectile* __stdcall FindTargetableProjectile(Unit* unit, unsigned int weapon);
int __stdcall WeaponCanReachPos(Unit* unit, Vec3* from, Vec3* to, int flags);
#include "../weapons/weapon_types.h"
#include "../weapons/explosions.h"
void __stdcall DamageUnit(Unit* source, Unit* target, int amount, int type, unsigned short extra);
void __stdcall RemoveUnitProjectiles(Unit* unit);
#include "../units/unit_scripts.h"
Vec3 __stdcall GetPieceOffset(Unit* unit, int piece);
#include "../ingame/info_panel.h"

#include "../units/unit_commands.h"
void __stdcall UpdatePlayerEconomy(Player* p);
#include "../units/unit_motion.h"

void __stdcall InitFeatureAnimPool(List_00421f20* list);
Spot* __stdcall PlaceFeature(Cell* cell, unsigned short feature, Vec3* pos, Rot16* rot, unsigned char owner);

int __stdcall GetWeaponTargetPos(Unit* unit, Vec3_0048a1e0* pos, int index);
Unit* __stdcall FindWeaponTarget(Unit* unit, unsigned char weapon, int useRange);

#include "../map/line_of_sight.h"
#include "../game/share_commands.h"
void __stdcall TransferMetal(unsigned char from, unsigned char to, float amount, int flag);
void __stdcall TransferEnergy(unsigned char from, unsigned char to, float amount, int flag);
int __stdcall GetGroundHeight(Pos_00485070* p);
void __stdcall GetSweetSpot(Obj_0043e3c0* obj, int out);
int __stdcall GetCellMeanHeight(Pos_00485070* p);
void __stdcall ApplyAreaDamageAt(void* owner, Vec3_0049a0c0* pos);
int __stdcall GetWeaponRange(int param1, unsigned int param2);
void __stdcall GetVisibleEnemiesInRadius(int player, const Vec* pos, int radius, int flags, std::vector<Unit*>* out);
int __stdcall IsPointVisible(Player* player, Vec3* pos);

#include "../map/line_of_sight.h"
#include "../map/radar.h"
int __stdcall IsUnitVisibleToPlayer(Player* player, Unit* unit);

class HapiBank;
#include "../map/map_list.h"
#include "../map/plot_map.h"
#include "../map/terrain.h"
void __stdcall AddUnitToMap(Unit* unit);
void __stdcall RemoveUnitFromMap(Unit* unit);
int __stdcall PickRandomReclaimableResourcesInRadius(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount);
void __stdcall SetupPlayerSlot(int player, char type);
#include "../units/units_485010.h"
struct OwnershipTransferPacket;
#include "../units/unit_types.h"
#include "../units/unit_save.h"
Unit* __stdcall CreateUnit(unsigned char player, unsigned short typeId, Vec3 pos, int param_4, int mode, int param_6);
UnitDef* __stdcall FindUnitType(const char* name);
int __stdcall FindMissionUnit(char* name, std::vector<Unit*>* units, int value);
void __stdcall AttachUnitToPiece(Unit* unit, Unit* target, char piece, char mode);
Angles16 __stdcall GetPieceAngles(Unit* unit, int piece);
#include "../ingame/control_panel.h"
#include "../units/unit_scripts.h"
#include "../units/unit_position.h"
#include "../orders/order_queue.h"

struct DamagedAllyCollector;



static void __stdcall ResetNative(UnitResources* self, unsigned char playerIndex)
{
    self->Reset(playerIndex);
}

static void __stdcall SaveUnitAccountsNative(UnitResources* self, UnitInfo* info, HapiBank* file)
{
    self->SaveUnitAccounts(info, file);
}

static void __stdcall LoadUnitAccountsNative(UnitResources* self, UnitInfo* info, HapiBank* file)
{
    self->LoadUnitAccounts(info, file);
}

static int __stdcall RequestEnergyNative(UnitResources* self, UnitResources* r, float amount)
{
    return self->RequestEnergy(r, amount);
}

static int __stdcall RequestEnergyAndMetalNative(UnitResources* self, float energy, float metal)
{
    return self->RequestEnergyAndMetal(energy, metal);
}

static int __stdcall SpendEnergyNative(UnitResources* self, float amount)
{
    return self->SpendEnergy(amount);
}

static int __stdcall SpendMetalNative(UnitResources* self, float amount)
{
    return self->SpendMetal(amount);
}

static int __stdcall SpendEnergyAndMetalNative(UnitResources* self, float energy, float metal)
{
    return self->SpendEnergyAndMetal(energy, metal);
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

static void __fastcall ClaimWeaponsNative(Unit* self, int unusedEdx, unsigned char index)
{
    self->ClaimWeapons(index);
}

static void __fastcall ReleaseWeaponsNative(Unit* self, int unusedEdx, unsigned char index)
{
    self->ReleaseWeapons(index);
}

static void __fastcall MarkOwnerNetDirtyFromDamageSplitNative(SquadManager* self, int unusedEdx, Projectile* projectile, int enemyDamage, int friendlyDamage)
{
    self->MarkOwnerNetDirtyFromDamageSplit(projectile, enemyDamage, friendlyDamage);
}

static void __fastcall SquadManagerTickIfActiveNative(SquadManager* self, int unusedEdx)
{
    self->TickIfActive();
}

static void __fastcall SquadManagerDeleteTimersNative(SquadManager* self, int unusedEdx)
{
    self->DeleteTimers();
}

static void __stdcall UnitTypeSetAddTypeOrCategoryNative(UnitTypeSet* self, char* text, int* out)
{
    self->AddTypeOrCategory(text, out);
}

static void __stdcall GetFactoriesInRadiusNative(int player, const Vec3* pos, int radius, std::vector<Unit*>* out)
{
    GetFactoriesInRadius(player, (const Vec*)pos, radius, out);
}

static void __stdcall SetStateBitsNative(Unit* unit, int mask, int set)
{
    unit->SetStateBits(mask, set);
}

#include "../weapons/weapons_49b720.h"

static unsigned char __fastcall ChooseWeaponNative(Unit* self)
{
    return self->ChooseWeapon();
}

static int __fastcall GetGameTypeNative(Mission* self)
{
    return self->GetGameType();
}

static void __stdcall SetWeaponTargetUnitNative(Unit* unit, Unit* target, int weapon)
{
    SetWeaponTargetUnit((char*)unit, (char*)target, weapon);
}

static void __stdcall SetWeaponTargetPosNative(Unit* unit, Vec3* pos, int weapon)
{
    SetWeaponTargetPos((char*)unit, &pos->x, weapon);
}

static int __stdcall GetWeaponTargetPosNative(Unit* unit, Vec3* pos, int weapon)
{
    return GetWeaponTargetPos(unit, (Vec3_0048a1e0*)pos, weapon);
}

static void __stdcall DamageFeatureNative(Cell* cell, int x, int z, WeaponDef* weapon)
{
    DamageFeature(cell, x, z, (Weapon*)weapon);
}

static Unit* __stdcall FindBestTargetIfFireAtWillNative(Unit* unit)
{
    return (Unit*)FindBestTargetIfFireAtWill(unit);
}

static int __stdcall GetGroundHeightNative(Vec3* pos)
{
    return GetGroundHeight((Pos_00485070*)pos);
}

static void __stdcall GetSweetSpotNative(Unit* unit, Vec3* out)
{
    GetSweetSpot((Obj_0043e3c0*)unit, (int)out);
}

static int __stdcall GetCellMeanHeightNative(Vec3* pos)
{
    return GetCellMeanHeight((Pos_00485070*)pos);
}

static void __stdcall ApplyAreaDamageAtNative(WeaponDef* weapon, Vec3* pos)
{
    ApplyAreaDamageAt(weapon, (Vec3_0049a0c0*)pos);
}

static WeaponDef* __stdcall FindWeaponByNameNative(char* name)
{
    return (WeaponDef*)FindWeaponByName(name);
}

static int __stdcall GetWeaponRangeNative(Unit* unit, unsigned int weapon)
{
    return GetWeaponRange((int)unit, weapon);
}

static void __stdcall GetVisibleEnemiesInRadiusNative(int player, const Vec3* pos, int radius, int flags, std::vector<Unit*>* out)
{
    GetVisibleEnemiesInRadius(player, (const Vec*)pos, radius, flags, out);
}

static char* __stdcall GetNameSlotNative(Mission* mission, int index)
{
    return mission->GetNameSlot(index);
}

static void __stdcall VisitObjectsInRangeNative(Vec3* pos, int range, UnitVisitor* visitor)
{
    VisitObjectsInRange(pos, range, *(DamagedAllyCollector*)visitor);
}

static void __fastcall SpatialBucketPrependUnitNative(SpatialBucket* self, int unusedEdx, Unit* unit)
{
    self->PrependUnit((int)unit);
}

static void __fastcall SpatialBucketUnlinkUnitNative(SpatialBucket* self, int unusedEdx, Unit* unit)
{
    self->UnlinkUnit(unit);
}

static void __fastcall RefreshPassMapNative(MovementClass* self, int unusedEdx, Point16 a, Point16 b)
{
    self->RefreshPassMap(a, b);
}

static int __fastcall AdvanceMissionNative(Mission* self)
{
    return self->AdvanceMission();
}

static char* __fastcall GetCampaignNameNative(Mission* self)
{
    return self->GetCampaignName();
}

static int __fastcall GetMissionIndexNative(Mission* self)
{
    return self->GetMissionIndex();
}

static char* __fastcall GetMissionNameNative(Mission* self)
{
    return self->GetMissionName();
}

static int __fastcall GetStartPositionNative(Mission* self, int unusedEdx, Vec3* out, int id)
{
    return self->GetStartPosition((Vec3*)out, id);
}

static void __fastcall LoadCampaignNative(Mission* self, int unusedEdx, char* file)
{
    self->LoadCampaign(file);
}

static int __fastcall LoadMissionByNameNative(Mission* self, int unusedEdx, char* name)
{
    return self->LoadMissionByName(name);
}

static int __fastcall MissionExistsNative(Mission* self, int unusedEdx, int index)
{
    return self->MissionExists(index);
}

static void __fastcall RefreshMapListNative(Mission* self, int unusedEdx, int multiplayer)
{
    self->RefreshMapList(multiplayer);
}

static int __fastcall SelectMissionNative(Mission* self, int unusedEdx, int index)
{
    return self->SelectMission(index);
}

static int __fastcall ComputeMapChecksumNative(Mission* self)
{
    return self->ComputeMapChecksum();
}

static int __fastcall GetTerrainLengthNative(Mission* self)
{
    return self->GetTerrainLength();
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
    self->RegisterConditions((Param_0048e010*)mission);
}

static void __fastcall FreeConditionsNative(MissionConditions* self)
{
    self->FreeConditions();
}

static void __stdcall DeleteConditionsNative(MissionConditions* self)
{
    delete self;
}

static void __fastcall NotifyUnitCreatedNative(MissionConditions* self, int unusedEdx, Unit* unit)
{
    self->NotifyUnitCreated(unit);
}

static void __fastcall NotifyUnitDiedNative(MissionConditions* self, int unusedEdx, Unit* unit)
{
    self->NotifyUnitDied(unit);
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

static void __fastcall UpdateSfxOccupyNative(UnitMotion* self, int unusedEdx, Unit* unit)
{
    ((Class_0043db50*)self)->UpdateSfxOccupy(unit);
}

static void __fastcall ReattachFxToUnitNative(Order* self)
{
    self->ReattachFxToUnit();
}

static int __fastcall SerializeOrderToSaveNative(Order* self, int unusedEdx, Unit* unit, HapiBank* file, char* name)
{
    return self->SerializeToSave(unit, file, name);
}

static void __fastcall DestroyMotionObjectNative(UnitMotion* self)
{
    self->DestroyObject();
}

static void __fastcall LoadMotionNative(UnitMotion* self, int unusedEdx, Unit* unit, HapiBank* file)
{
    self->LoadMotion(unit, file);
}

static void __fastcall SaveMotionNative(UnitMotion* self, int unusedEdx, Unit* unit, HapiBank* file)
{
    self->SaveMotion(unit, file);
}

static void __fastcall UpdateMotionNative(UnitMotion* self, int unusedEdx, Unit* unit)
{
    self->UpdateMotion(unit);
}

static int __stdcall MovementClassGetNative(MovementClass* self, int x, int y)
{
    return self->Get(x, y);
}

static int __stdcall MovementClassInBoundsNative(MovementClass* self, unsigned int x, unsigned int y)
{
    return self->InBounds(x, y);
}

static void __stdcall RefreshMovedUnitsNative(MovementClass* self, Unit* unit)
{
    self->RefreshMovedUnits((Object_00440af0*)unit);
}

static void __stdcall RefreshUnitIfStaleNative(MovementClass* self, Unit* unit)
{
    self->RefreshUnitIfStale((Object_00440af0*)unit);
}

static void __stdcall AiSearchGoalSetWaypointsNative(AiSearchGoal* self, Point16* points, int count)
{
    self->SetWaypoints(points, count);
}

static void __stdcall OrderFxAddFlagsNative(OrderFx* self, int flags)
{
    self->AddFlags(flags);
}

static void __fastcall RunSearchesNative(Pathfinder* self)
{
    self->RunSearches();
}

static void __stdcall AbortIfGoalMatchNative(Pathfinder* self, AiSearchGoal* goal)
{
    self->AbortIfGoalMatch((int)goal);
}

static void __stdcall DestroyPathfinderNative(Pathfinder* self)
{
    delete self;
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
static PlayersSeam s_players;
static MapSeam s_map;
static FilesSeam s_files;

static void __fastcall LoadMeteorDefaultsNative(MeteorParams* params)
{
    params->LoadMeteorDefaults();
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

static int __stdcall BankWriteBoxNative(HapiBank* self, void* src, int len)
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
    s_wind_meteors.UpdateWind = UpdateWind;
    s_wind_meteors.UpdateMeteors = UpdateMeteors;
    s_wind_meteors.InitMeteors = InitMeteors;
    s_wind_meteors.EmptyShutdownPreCleanup = EmptyShutdownPreCleanup;
    s_wind_meteors.EnableMeteors = EnableMeteors;
    s_wind_meteors.DisableMeteors = DisableMeteors;
    s_wind_meteors.SetMeteorParams = SetMeteorParams;
    s_wind_meteors.LoadMeteorDefaults = LoadMeteorDefaultsNative;
    s_wind_meteors.SaveMeteors = SaveMeteors;
    s_wind_meteors.LoadMeteors = LoadMeteors;
    s_wind_meteors.CmdMeteor = CmdMeteor;
    s_wind_meteors.UpdateWindOnStream = UpdateWindOnStream;
    s_economy.UpdateResourceSharing = UpdateResourceSharing;
    s_economy.GetEnergyUse = GetEnergyUse;
    s_economy.UpdatePlayerEconomy = UpdatePlayerEconomy;
    s_economy.UpdateMetalExtraction = UpdateMetalExtraction;
    s_economy.Reset = ResetNative;
    s_economy.SaveUnitAccounts = SaveUnitAccountsNative;
    s_economy.LoadUnitAccounts = LoadUnitAccountsNative;
    s_economy.RequestEnergy = RequestEnergyNative;
    s_economy.RequestEnergyAndMetal = RequestEnergyAndMetalNative;
    s_economy.SpendEnergy = SpendEnergyNative;
    s_economy.SpendMetal = SpendMetalNative;
    s_economy.SpendEnergyAndMetal = SpendEnergyAndMetalNative;
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
    s_features.PlaceFeature = PlaceFeature;
    s_features.RemoveFeature = RemoveFeature;
    s_features.ReclaimFeature = ReclaimFeature;
    s_features.FindOrLoadFeatureType = FindOrLoadFeatureType;
    s_features.DamageFeature = DamageFeatureNative;
    s_features.GetFootprintCentre = GetFootprintCentre;
    s_features.SaveFeatures = SaveFeatures;
    s_features.LoadFeatures = LoadFeatures;
    s_targeting.SetWeaponTargetUnit = SetWeaponTargetUnitNative;
    s_targeting.SetWeaponTargetPos = SetWeaponTargetPosNative;
    s_targeting.ClearWeaponTarget = ClearWeaponTarget;
    s_targeting.ResetWeaponTarget = ResetWeaponTarget;
    s_targeting.GetWeaponTargetUnit = GetWeaponTargetUnit;
    s_targeting.GetWeaponTargetPos = GetWeaponTargetPosNative;
    s_targeting.FindWeaponTarget = FindWeaponTarget;
    s_targeting.FindBestTargetIfFireAtWill = FindBestTargetIfFireAtWillNative;
    s_targeting.ChooseWeapon = ChooseWeaponNative;
    s_los_radar.GetMapCell = GetMapCell;
    s_los_radar.CmdShareMapping = CmdShareMapping;
    s_los_radar.CmdShareRadar = CmdShareRadar;
    s_los_radar.ClampWorldPosToTerrain = ClampWorldPosToTerrain;
    s_los_radar.ExpireEyeballs = ExpireEyeballs;
    s_los_radar.FreeMapResources = FreeMapResources;
    s_los_radar.FreeRadar = FreeRadar;
    s_los_radar.InitRadar = InitRadar;
    s_los_radar.LoadTntMap = LoadTntMap;
    s_los_radar.RecalculateLineOfSight = RecalculateLineOfSight;
    s_los_radar.IsUnitVisibleToPlayer = IsUnitVisibleToPlayer;
    s_los_radar.GetMapCellAtPosition = GetMapCellAtPosition;
    s_los_radar.GetOriginCellAtPosition = GetOriginCellAtPosition;
    s_los_radar.UpdateCellHeightRange = UpdateCellHeightRange;
    s_los_radar.UpdateUnitLineOfSight = UpdateUnitLineOfSight;
    s_los_radar.RemoveUnitLineOfSight = RemoveUnitLineOfSight;
    s_los_radar.RevealNewUnit = RevealNewUnit;
    s_los_radar.AddEyeball = AddEyeball;
    s_los_radar.DrawRadarUnits = DrawRadarUnits;
    s_los_radar.UpdateRadarMapped = UpdateRadarMapped;
    s_los_radar.UpdateSensorRadarAndCloak = UpdateSensorRadarAndCloak;
    s_pathfinding.RunSearches = RunSearchesNative;
    s_pathfinding.AbortIfGoalMatch = AbortIfGoalMatchNative;
    s_pathfinding.Destroy = DestroyPathfinderNative;
    s_orders.AiSearchGoalSetWaypoints = AiSearchGoalSetWaypointsNative;
    s_orders.OrderFxAddFlags = OrderFxAddFlagsNative;
    s_units.MovementClassGet = MovementClassGetNative;
    s_units.MovementClassInBounds = MovementClassInBoundsNative;
    s_units.RefreshMovedUnits = RefreshMovedUnitsNative;
    s_units.RefreshUnitIfStale = RefreshUnitIfStaleNative;
    s_ai.HasReadyUnitInRange = (bool (__stdcall *)(int, Vec3 *, int))HasReadyUnitInRange;
    s_ai.IsPointVisible = IsPointVisible;
    s_map.GetNameSlot = GetNameSlotNative;
    s_map.VisitObjectsInRange = VisitObjectsInRangeNative;
    s_players.TransferMetal = TransferMetal;
    s_players.TransferEnergy = TransferEnergy;
    s_units.GetGroundHeight = GetGroundHeightNative;
    s_units.GetSweetSpot = GetSweetSpotNative;
    s_units.FindScript = FindScriptNative;
    s_units.StartScriptWithArgs = StartScriptWithArgsNative;
    s_units.SetStateBits = SetStateBitsNative;
    s_map.GetCellMeanHeight = GetCellMeanHeightNative;
    s_map.RefreshAllPassMaps = RefreshAllPassMaps;
    s_map.GetGameType = GetGameTypeNative;
    s_weapons.FindWeaponByName = FindWeaponByNameNative;
    s_weapons.ApplyAreaDamageAt = ApplyAreaDamageAtNative;
    s_weapons.GetWeaponRange = GetWeaponRangeNative;
    s_weapons.WeaponCanReachUnit = WeaponCanReachUnit;
    s_ai.GetVisibleEnemiesInRadius = GetVisibleEnemiesInRadiusNative;
    s_weapons.FindTargetableProjectile = FindTargetableProjectile;
    s_weapons.WeaponCanReachPos = WeaponCanReachPos;
    s_weapons.AllocWeaponArray = AllocWeaponArray;
    s_weapons.FreeWeaponArray = FreeWeaponArray;
    s_weapons.LoadWeaponTypes = LoadWeaponTypes;
    s_weapons.FreeWeaponTypes = FreeWeaponTypes;
    s_weapons.InitExplosions = InitExplosions;
    s_weapons.FreeExplosions = FreeExplosions;
    s_weapons.DamageUnit = DamageUnit;
    s_weapons.FindFeatureAtPos = FindFeatureAtPos;
    s_weapons.ReleaseWeapons = ReleaseWeaponsNative;
    s_weapons.ClaimWeapons = ClaimWeaponsNative;
    s_weapons.DetonateUnitWeapon = DetonateUnitWeapon;
    s_weapons.InitUnitWeaponSlots = InitUnitWeaponSlots;
    s_weapons.RemoveUnitProjectiles = RemoveUnitProjectiles;
    s_weapons.UpdateUnitWeapons = UpdateUnitWeapons;
    s_ai.ReactToAttack = ReactToAttack;
    s_ai.MarkOwnerNetDirtyFromDamageSplit = MarkOwnerNetDirtyFromDamageSplitNative;
    s_orders.AppendOrder = AppendOrder;
    s_ai.DumpPlayerAI = DumpPlayerAI;
    s_ai.ResetAIPlayers = ResetAIPlayers;
    s_ai.GetFactoriesInRadius = GetFactoriesInRadiusNative;
    s_ai.RegisterAICommands = RegisterAICommands;
    s_ai.CreatePlayerAI = CreatePlayerAI;
    s_ai.DestroyPlayerAI = DestroyPlayerAI;
    s_ai.UpdatePlayerAI = UpdatePlayerAI;
    s_ai.RebuildFeatureCells = RebuildFeatureCells;
    s_ai.ParseDownloadableAiWeightScripts = ParseDownloadableAiWeightScripts;
    s_ai.ReparseAiWeightScriptsIfLimitNotSticky = ReparseAiWeightScriptsIfLimitNotSticky;
    s_ai.CreateSquads = CreateSquads;
    s_ai.FreeSquads = FreeSquads;
    s_ai.SetUnitSquad = SetUnitSquad;
    s_ai.SquadManagerTickIfActive = SquadManagerTickIfActiveNative;
    s_ai.SquadManagerDeleteTimers = SquadManagerDeleteTimersNative;
    s_map.CanBuildAt = CanBuildAt;
    s_map.CanPlaceUnitFootprint = CanPlaceUnitFootprint;
    s_orders.AddOrder = AddOrder;
    s_orders.DeleteOrders = DeleteOrders;
    s_orders.GetOrderFlags = (int (__stdcall *)(Unit *))GetOrderFlags;
    s_orders.GetOrderType = GetOrderType;
    s_orders.IssueAttackOrder = IssueAttackOrder;
    s_orders.RegisterAllOrderTypes = RegisterAllOrderTypes;
    s_orders.ClearOrderTypeTable = ClearOrderTypeTable;
    s_orders.CreatePathfinder = CreatePathfinder;
    s_orders.DestroyPathfinder = DestroyPathfinder;
    s_players.GetEnergyIncome = GetEnergyIncome;
    s_players.GetMetalIncome = GetMetalIncome;
    s_players.GetNetEnergy = (float (__stdcall *)(Player *))GetNetEnergy;
    s_players.GetNetMetal = (float (__stdcall *)(Player *))GetNetMetal;
    s_players.LoadDefaultAIScript = LoadDefaultAIScript;
    s_units.NotifyUnitRefs = NotifyUnitRefs;
    s_units.UnitTypeSetAddTypeOrCategory = UnitTypeSetAddTypeOrCategoryNative;
    s_units.StartScript = StartScriptNative;
    s_units.QueryScript = QueryScriptNative;
    s_units.GetAimFromPosition = (void (__stdcall *)(Unit *, Vec3 *, unsigned char))GetAimFromPosition;
    s_units.GetPieceOffset = GetPieceOffset;
    s_units.GetPiecePosition = (Vec3 (__stdcall *)(Unit *, int))GetPiecePosition;
    s_units.MarkRecentlyDamaged = (void (__stdcall *)(Unit *))MarkRecentlyDamaged;
    s_map.BuildAllPassMaps = BuildAllPassMaps;
    s_map.FreeOtaEnumCacheAndMission = FreeOtaEnumCacheAndMission;
    s_map.LoadMappingData = LoadMappingData;
    s_map.LoadMetalPlotmap = LoadMetalPlotmap;
    s_map.LoadPlayerFeaturesPlotmap = LoadPlayerFeaturesPlotmap;
    s_map.SaveMappingData = SaveMappingData;
    s_map.SaveMetalPlotmap = SaveMetalPlotmap;
    s_map.SavePlayerFeaturesPlotmap = SavePlayerFeaturesPlotmap;
    s_map.SetMissionType = SetMissionType;
    s_map.SnapWorldPosToFootprint = SnapWorldPosToFootprint;
    s_map.AddUnitToMap = AddUnitToMap;
    s_map.RemoveUnitFromMap = RemoveUnitFromMap;
    s_map.CanPlaceFootprintAt = CanPlaceFootprintAt;
    s_map.IsPadSlotFree = IsPadSlotFree;
    s_map.PickRandomReclaimableResourcesInRadius = PickRandomReclaimableResourcesInRadius;
    s_map.ClearFootprintAndUnlink = ClearFootprintAndUnlink;
    s_map.ForceNeighborFootprintReclaim = ForceNeighborFootprintReclaim;
    s_map.SetYardOpen = SetYardOpen;
    s_map.SpatialBucketPrependUnit = SpatialBucketPrependUnitNative;
    s_map.SpatialBucketUnlinkUnit = SpatialBucketUnlinkUnitNative;
    s_map.RefreshPassMap = RefreshPassMapNative;
    s_map.AdvanceMission = AdvanceMissionNative;
    s_map.GetCampaignName = GetCampaignNameNative;
    s_map.GetMissionIndex = GetMissionIndexNative;
    s_map.GetMissionName = GetMissionNameNative;
    s_map.GetStartPosition = GetStartPositionNative;
    s_map.LoadCampaign = LoadCampaignNative;
    s_map.LoadMissionByName = LoadMissionByNameNative;
    s_map.MissionExists = MissionExistsNative;
    s_map.RefreshMapList = RefreshMapListNative;
    s_map.SelectMission = SelectMissionNative;
    s_map.ComputeMapChecksum = ComputeMapChecksumNative;
    s_map.GetTerrainLength = GetTerrainLengthNative;
    s_players.InitPlayers = InitPlayers;
    s_players.FreePlayers = FreePlayers;
    s_players.InitPlayerResources = InitPlayerResources;
    s_players.LoadPlayerControllers = LoadPlayerControllers;
    s_players.LoadPlayers = LoadPlayers;
    s_players.SavePlayers = SavePlayers;
    s_players.RebuildAIFeatureCells = RebuildAIFeatureCells;
    s_players.SetupPlayerSlot = SetupPlayerSlot;
    s_players.UpdateBlink = UpdateBlink;
    s_players.KillPlayerUnits = KillPlayerUnits;
    s_players.GiveUnitToPlayer = GiveUnitToPlayer;
    s_players.DeactivateConditions = DeactivateConditionsNative;
    s_players.LoadConditions = LoadConditionsNative;
    s_players.SaveConditions = SaveConditionsNative;
    s_players.RegisterConditions = RegisterConditionsNative;
    s_players.FreeConditions = FreeConditionsNative;
    s_players.DeleteConditions = DeleteConditionsNative;
    s_players.NotifyUnitCreated = NotifyUnitCreatedNative;
    s_players.NotifyUnitDied = NotifyUnitDiedNative;
    s_players.UpdatePlayersOnStream = UpdatePlayersOnStream;
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
    s_units.GetNanoPiecePosition = (void (__stdcall *)(Unit *, Vec3 *))GetNanoPiecePosition;
    s_units.GetPieceAngles = GetPieceAngles;
    s_units.AddBuildProgress = AddBuildProgress;
    s_units.AddRepairProgress = AddRepairProgress;
    s_units.ApplyUnfinishedBuildDecay = ApplyUnfinishedBuildDecay;
    s_units.FinishConstruction = FinishConstruction;
    s_units.SetUnitPosition = SetUnitPosition;
    s_units.CanLoad = CanLoadNative;
    s_units.CanReclaim = CanReclaimNative;
    s_units.CanRepair = CanRepairNative;
    s_units.UnitRefUnlink = UnitRefUnlinkNative;
    s_units.StartScriptWithArgsByIndex = StartScriptWithArgsByIndexNative;
    s_units.UpdateSfxOccupy = UpdateSfxOccupyNative;
    s_units.PoseUnitModel = PoseUnitModel;
    s_orders.ReattachFxToUnit = ReattachFxToUnitNative;
    s_orders.SerializeOrderToSave = SerializeOrderToSaveNative;
    s_orders.AddBeCarriedOrder = AddBeCarriedOrder;
    s_orders.RunOrders = RunOrders;
    s_orders.RunSecondaryOrders = RunSecondaryOrders;
    s_orders.DestroyMotionObject = DestroyMotionObjectNative;
    s_orders.LoadMotion = LoadMotionNative;
    s_orders.SaveMotion = SaveMotionNative;
    s_orders.UpdateMotion = UpdateMotionNative;
    s_weapons.UpdateProjectiles = UpdateProjectiles;
    s_weapons.UpdateExplosions = UpdateExplosions;
    s_weapons.SpawnProjectile = SpawnProjectile;
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
    g_seams.files = &s_files;
}
