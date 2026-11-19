#include <stddef.h>
#include <vector>

#include "../../interfaces/seams.h"
#include "../../interfaces/scripts.h"
#include "../../interfaces/movement.h"
#include "../../interfaces/network.h"
#include "../../interfaces/ref.h"
#include "../state/block.h"
#include "../network/player.h"
#include "../game/game.h"
#include "../game/los_helpers.h"
#include "../files/files_impl.h"
#include "../files/files_bank.h"
#include "../game/unit_resources.h"
#include "../units/movement_class.h"
#include "../scripts/cob_script.h"
#include "../scripts/cob_types.h"
#include "../scripts/unit_script.h"
#include "../util/vec3.h"
#include "../map/mission.h"
#include "../map/spatial_bucket.h"
#include "../game/mission_conditions.h"
#include "../scripts/object_state.h"
#include "../units/unit.h"
#include "../ai/pathfinder.h"
#include "../movement/ai_search_goal.h"
#include "../movement/order_fx.h"
#include "../movement/path_order.h"
#include "../movement/approach_areas.h"
#include "../movement/air_maneuver_order.h"
#include "../ai/squad_manager.h"
#include "../units/unit_type_set.h"
#include "../units/unit_ref.h"
#include "../units/sfx_occupy.h"
#include "../movement/unit_motion.h"
#include "../orders/order.h"
#include "../orders/order_base.h"
#include "../util/angles.h"

struct Player;
#include "pathfinding_world.h"
#include "../game/economy.h"

#include "../game/game_state.h"
#include "../map/meteors.h"
#include "../game/console_commands.h"
#include "../weapons/weapons.h"
#include "../network/net_game.h"
#include "../map/features.h"

struct Cell;
struct Point16;
struct Feature;
struct Rot16;
struct TntInfo;
struct GroundHeightPos;
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
#include "../orders/order_list.h"
#include "../orders/order_list.h"
#include "../orders/order_queue.h"
struct UnitOrderTypeView;
struct FixedPosView;
#include "../orders/order_dispatch.h"
#include "../movement/order_targets.h"
#include "../game/players.h"
#include "../units/unit_targets.h"

Projectile* __stdcall FindTargetableProjectile(Unit* unit, unsigned int weapon);
int __stdcall WeaponCanReachPos(Unit* unit, Vec3* from, Vec3* to, int flags);
#include "../weapons/weapon_types.h"
#include "../weapons/explosions.h"
void __stdcall DamageUnit(Unit* source, Unit* target, int amount, int type, unsigned short extra);
#include "../scripts/piece_queries.h"
Vec3 __stdcall GetPieceOffset(Unit* unit, int piece);
struct ModelBuildList;
#include "../ingame/info_panel.h"

#include "../units/unit_commands.h"
#include "../units/unit_motion.h"

#include "../map/line_of_sight.h"
#include "../game/share_commands.h"
int __stdcall GetGroundHeight(GroundHeightPos* p);
int __stdcall GetWeaponRange(int param1, unsigned int param2);
#include "../map/line_of_sight_index.h"

#include "../map/line_of_sight.h"
#include "../ingame/radar.h"
#include "../map/radar_capture.h"
#include "../ingame/radar_picture.h"

class HapiBank;
#include "../map/map_list.h"
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
#include "../../interfaces/rand_stream.h"
#include "../orders/order_queue.h"




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

static void __fastcall ClaimWeaponsNative(Unit* self, int unusedEdx, unsigned char index)
{
    self->ClaimWeapons(index);
}

static void __fastcall ReleaseWeaponsNative(Unit* self, int unusedEdx, unsigned char index)
{
    self->ReleaseWeapons(index);
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

static int __fastcall GetGameTypeNative(Mission* self)
{
    return self->GetGameType();
}

static int __stdcall GetWeaponTargetPosNative(Unit* unit, Vec3* pos, int weapon)
{
    return GetWeaponTargetPos(unit, (Vec3*)pos, weapon);
}

static int __stdcall GetGroundHeightNative(Vec3* pos)
{
    return GetGroundHeight((GroundHeightPos*)pos);
}

static void __stdcall GetSweetSpotNative(Unit* unit, Vec3* out)
{
    GetSweetSpot((SweetSpotObjectView*)unit, (int)out);
}

static int __stdcall GetCellMeanHeightNative(Vec3* pos)
{
    return GetCellMeanHeight(pos);
}

static void __stdcall ApplyAreaDamageAtNative(WeaponDef* weapon, Vec3* pos)
{
    ApplyAreaDamageAt(weapon, (Vec3*)pos);
}

static WeaponDef* __stdcall FindWeaponByNameNative(char* name)
{
    return (WeaponDef*)FindWeaponByName(name);
}

static int __stdcall GetWeaponRangeNative(Unit* unit, unsigned int weapon)
{
    return GetWeaponRange((int)unit, weapon);
}

static char* __stdcall GetNameSlotNative(Mission* mission, int index)
{
    return mission->GetNameSlot(index);
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

static void __fastcall ReattachFxToUnitNative(Order* self)
{
    self->ReattachFxToUnit();
}

static int __fastcall SerializeOrderToSaveNative(Order* self, int unusedEdx, Unit* unit, HapiBank* file, char* name)
{
    return self->SerializeToSave(unit, file, name);
}

static Order* __stdcall LoadOrderNative(Unit* unit, HapiBank* file, char* name)
{
    return new (kSim) Order(unit, file, name);
}

static Order* __stdcall NewOrderNative(MissionType kind, Unit* target, void* pos, int a, int b, int c)
{
    return new (kSim) Order(kind, target, pos, a, b, c);
}

static MissionType __stdcall MissionTypeFromNameNative(const char* name)
{
    return MissionType(name);
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
    self->RefreshMovedUnits((UnitPassMapView*)unit);
}

static void __stdcall RefreshUnitIfStaleNative(MovementClass* self, Unit* unit)
{
    self->RefreshUnitIfStale((UnitPassMapView*)unit);
}

static void __stdcall OrderOrStatusFlagsNative(OrderBase* order, unsigned int flags)
{
    order->OrStatusFlags(flags);
}

static void __stdcall WeaponAimCobCompleteNative(Callback* callback, int result)
{
    callback->Complete(result);
}

static void __stdcall CobScriptExplodePieceNative(CobScript* script, int piece, unsigned int flags)
{
    script->ExplodePiece(piece, flags);
}

static void __fastcall DestroyMotionObjectNative(UnitMotion* self)
{
    self->DestroyObject();
}

static void __stdcall NewUnitMotionNative(Unit* unit)
{
    unit->motion = new (kSim) UnitMotion(unit);
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

static void __stdcall UnitMotionReadNetStateNative(UnitMotion* motion, BitReader* reader)
{
    motion->ReadNetState(reader);
}

static void __stdcall SetFlightModeNative(UnitMotion* self, Unit* unit, int state)
{
    self->SetFlightMode(unit, state);
}

static void __stdcall PathOrderSetAltitudeNative(PathOrder* self, int altitude)
{
    self->SetAltitude(altitude);
}

static void __stdcall PathOrderSetApproachRadiusNative(PathOrder* self, short radius)
{
    self->SetApproachRadius(radius);
}

static void __stdcall PathOrderSetHeadingNative(PathOrder* self, short heading)
{
    self->SetHeading(heading);
}

static void __stdcall AirManeuverOrderSetAltitudeNative(AirManeuverOrder* self, int altitude)
{
    self->SetAltitude(altitude);
}

static PathOrder* __stdcall NewPathOrderAtNative(Order* source, const Vec3* pos)
{
    PathOrder* target = new (kSim) PathOrder(source, *pos);
    return target;
}

static PathOrder* __stdcall NewPathOrderToUnitNative(Order* source, Unit* unit)
{
    PathOrder* target = new (kSim) PathOrder(source, unit);
    return target;
}

static PathOrder* __stdcall NewPathOrderToUnitAtNative(Order* source, Unit* unit, const Vec3* pos)
{
    PathOrder* target = new (kSim) PathOrder(source, unit, *pos);
    return target;
}

static PathOrder* __stdcall NewPathOrderToUnitAtAngleNative(Order* source, Unit* unit, short angle)
{
    PathOrder* target = new (kSim) PathOrder(source, unit, angle);
    return target;
}

static AirManeuverOrder* __stdcall NewAirManeuverOrderNative(Order* source, const Vec3* from, const Vec3* to)
{
    AirManeuverOrder* target = new (kSim) AirManeuverOrder(source, *from, *to);
    return target;
}

static ApproachRadius* __stdcall NewApproachRadiusNative(Order* source, int x, int y, int radius)
{
    ApproachRadius* target = new (kSim) ApproachRadius(source, x, y, radius);
    return target;
}

static RingApproach* __stdcall NewRingApproachNative(Order* source, int x, int y, int inner, int outer)
{
    RingApproach* target = new (kSim) RingApproach(source, x, y, inner, outer);
    return target;
}

static PointMarker* __stdcall NewPointMarkerNative(Order* source, Point16 pos, Point16 size)
{
    PointMarker* target = new (kSim) PointMarker(source, pos, size);
    return target;
}

static PathOrder* __stdcall LoadPathOrderNative(Order* source, HapiBank* file, char* name)
{
    PathOrder* target = new (kSim) PathOrder((int)source, file, name);
    return target;
}

static AirManeuverOrder* __stdcall LoadAirManeuverOrderNative(Order* source, HapiBank* file, char* name)
{
    AirManeuverOrder* target = new (kSim) AirManeuverOrder((int)source, file, name);
    return target;
}

static ApproachRadius* __stdcall LoadApproachRadiusNative(Order* source, HapiBank* file, char* name)
{
    ApproachRadius* target = new (kSim) ApproachRadius((int)source, file, name);
    return target;
}

static RingApproach* __stdcall LoadRingApproachNative(Order* source, HapiBank* file, char* name)
{
    RingApproach* target = new (kSim) RingApproach((int)source, file, name);
    return target;
}

static PointMarker* __stdcall LoadPointMarkerNative(Order* source, HapiBank* file, char* name)
{
    PointMarker* target = new (kSim) PointMarker((int)source, file, name);
    return target;
}

static PathGoal* __stdcall MotionGoalNative(UnitMotion* motion)
{
    return (PathGoal*)motion->obj.get();
}

static OrderFx* __stdcall GoalTargetNative(PathGoal* goal)
{
    return goal->target.get();
}

static void __stdcall GoalSetPathOrderNative(PathGoal* goal, void* target)
{
    goal->SetPathOrder(target);
}

static void __stdcall OrderFxDestroyNative(OrderFx* target)
{
    delete target;
}

static int __stdcall OrderFxGetTypeNative(OrderFx* target)
{
    return target->GetType();
}

static int __stdcall OrderFxSerializeToSaveNative(OrderFx* target, Order* order, HapiBank* file, char* name)
{
    return target->SerializeToSave(order, file, name);
}

static int __stdcall GoalHasNetUnitStateNative(PathGoal* goal)
{
    return goal->HasNetUnitState();
}

static void __stdcall GoalSerializeNetUnitStateNative(PathGoal* goal, BitWriter* stream)
{
    goal->SerializeNetUnitState(stream);
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
    s_movement.ApplyPathEvent = ApplyPathEvent;
    s_orders.OrderOrStatusFlags = OrderOrStatusFlagsNative;
    s_movement.UnitMotionReadNetState = UnitMotionReadNetStateNative;
    s_units.MovementClassGet = MovementClassGetNative;
    s_units.MovementClassInBounds = MovementClassInBoundsNative;
    s_units.RefreshMovedUnits = RefreshMovedUnitsNative;
    s_units.RefreshUnitIfStale = RefreshUnitIfStaleNative;
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
    s_map.GetNameSlot = GetNameSlotNative;
    s_units.GetGroundHeight = GetGroundHeightNative;
    s_scripts.GetSweetSpot = GetSweetSpotNative;
    s_scripts.FindScript = FindScriptNative;
    s_scripts.StartScriptWithArgs = StartScriptWithArgsNative;
    s_units.SetStateBits = SetStateBitsNative;
    s_map.GetCellMeanHeight = GetCellMeanHeightNative;
    s_map.GetGameType = GetGameTypeNative;
    s_weapons.FindWeaponByName = FindWeaponByNameNative;
    s_weapons.ApplyAreaDamageAt = ApplyAreaDamageAtNative;
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
    s_weapons.DamageUnit = DamageUnit;
    s_weapons.FindFeatureAtPos = FindFeatureAtPos;
    s_weapons.ReleaseWeapons = ReleaseWeaponsNative;
    s_weapons.ClaimWeapons = ClaimWeaponsNative;
    s_weapons.DetonateUnitWeapon = DetonateUnitWeapon;
    s_weapons.InitUnitWeaponSlots = InitUnitWeaponSlots;
    s_weapons.RemoveUnitProjectiles = RemoveUnitProjectiles;
    s_weapons.UpdateUnitWeapons = UpdateUnitWeapons;
    s_weapons.WeaponAimCobComplete = WeaponAimCobCompleteNative;
    s_weapons.CobScriptExplodePiece = CobScriptExplodePieceNative;
    s_ai.Apply = AiApply;
    s_ai.WriteState = AiWriteState;
    s_orders.AppendOrder = AppendOrder;
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
    s_map.Apply = ApplyMapEvent;
    s_map.AllocateBuckets = MapAllocateBuckets;
    s_map.FreeBuckets = MapFreeBuckets;
    s_map.WriteState = MapWriteState;
    s_map.UnitsInRange = UnitsInRange;
    s_map.CanBuildAt = CanBuildAt;
    s_map.CanPlaceUnitFootprint = CanPlaceUnitFootprint;
    s_orders.AddOrder = AddOrder;
    s_orders.DeleteOrders = DeleteOrders;
    s_orders.GetOrderFlags = (int (__stdcall *)(Unit *))GetOrderFlags;
    s_orders.GetOrderType = GetOrderType;
    s_orders.IssueAttackOrder = IssueAttackOrder;
    s_orders.RegisterAllOrderTypes = RegisterAllOrderTypes;
    s_orders.ClearOrderTypeTable = ClearOrderTypeTable;
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
    s_map.FreeOtaEnumCacheAndMission = FreeOtaEnumCacheAndMission;
    s_map.LoadMappingData = LoadMappingData;
    s_map.LoadMetalPlotmap = LoadMetalPlotmap;
    s_map.LoadPlayerFeaturesPlotmap = LoadPlayerFeaturesPlotmap;
    s_map.SaveMappingData = SaveMappingData;
    s_map.SaveMetalPlotmap = SaveMetalPlotmap;
    s_map.SavePlayerFeaturesPlotmap = SavePlayerFeaturesPlotmap;
    s_map.SetMissionType = SetMissionType;
    s_map.SnapWorldPosToFootprint = SnapWorldPosToFootprint;
    s_map.CanPlaceFootprintAt = CanPlaceFootprintAt;
    s_map.IsPadSlotFree = IsPadSlotFree;
    s_map.PickRandomReclaimableResourcesInRadius = PickRandomReclaimableResourcesInRadius;
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
    s_movement.SetFlightMode = SetFlightModeNative;
    s_movement.PathOrderSetAltitude = PathOrderSetAltitudeNative;
    s_movement.PathOrderSetApproachRadius = PathOrderSetApproachRadiusNative;
    s_movement.PathOrderSetHeading = PathOrderSetHeadingNative;
    s_movement.AirManeuverOrderSetAltitude = AirManeuverOrderSetAltitudeNative;
    s_movement.NewPathOrderAt = NewPathOrderAtNative;
    s_movement.NewPathOrderToUnit = NewPathOrderToUnitNative;
    s_movement.NewPathOrderToUnitAt = NewPathOrderToUnitAtNative;
    s_movement.NewPathOrderToUnitAtAngle = NewPathOrderToUnitAtAngleNative;
    s_movement.NewAirManeuverOrder = NewAirManeuverOrderNative;
    s_movement.NewApproachRadius = NewApproachRadiusNative;
    s_movement.NewRingApproach = NewRingApproachNative;
    s_movement.NewPointMarker = NewPointMarkerNative;
    s_movement.LoadPathOrder = LoadPathOrderNative;
    s_movement.LoadAirManeuverOrder = LoadAirManeuverOrderNative;
    s_movement.LoadApproachRadius = LoadApproachRadiusNative;
    s_movement.LoadRingApproach = LoadRingApproachNative;
    s_movement.LoadPointMarker = LoadPointMarkerNative;
    s_movement.MotionGoal = MotionGoalNative;
    s_movement.GoalTarget = GoalTargetNative;
    s_movement.GoalSetPathOrder = GoalSetPathOrderNative;
    s_movement.OrderFxDestroy = OrderFxDestroyNative;
    s_movement.OrderFxGetType = OrderFxGetTypeNative;
    s_movement.OrderFxSerializeToSave = OrderFxSerializeToSaveNative;
    s_movement.GoalHasNetUnitState = GoalHasNetUnitStateNative;
    s_movement.GoalSerializeNetUnitState = GoalSerializeNetUnitStateNative;
    s_units.UnitRefInit = UnitRefInitNative;
    s_units.UnitRefSetUnit = UnitRefSetUnitNative;
    s_scripts.StartScriptWithArgsByIndex = StartScriptWithArgsByIndexNative;
    s_units.UpdateSfxOccupy = UpdateSfxOccupyNative;
    s_scripts.PoseUnitModel = PoseUnitModel;
    s_scripts.CreateObjectState = CreateObjectState;
    s_scripts.FreeObjectState = FreeObjectState;
    s_scripts.UpdateObjectState = UpdateObjectState;
    s_units.UpdateUnit = UpdateUnit;
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
    s_orders.ReattachFxToUnit = ReattachFxToUnitNative;
    s_orders.SerializeOrderToSave = SerializeOrderToSaveNative;
    s_orders.AddBeCarriedOrder = AddBeCarriedOrder;
    s_orders.RunOrders = RunOrders;
    s_orders.RunSecondaryOrders = RunSecondaryOrders;
    s_movement.DestroyMotionObject = DestroyMotionObjectNative;
    s_movement.NewUnitMotion = NewUnitMotionNative;
    s_orders.LoadOrder = LoadOrderNative;
    s_orders.MissionTypeFromName = MissionTypeFromNameNative;
    s_orders.NewOrder = NewOrderNative;
    s_orders.QueueBuildOrder = QueueBuildOrder;
    s_movement.LoadMotion = LoadMotionNative;
    s_movement.SaveMotion = SaveMotionNative;
    s_movement.UpdateMotion = UpdateMotionNative;
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
    g_seams.files = &s_files;
    g_seams.network = &s_network;
    g_seams.pathworld = NativePathWorld();
}
