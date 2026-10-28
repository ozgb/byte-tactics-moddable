#include <string.h>
#include "../state/block.h"
#include "../state/game_rand.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "win_types.h"
#include "../../interfaces/host.h"
#include "../files/files_api.h"
#include "../harness/condition_class.h"
#include "victory_conditions.h"
#include "../state/sim_globals.h"
#include "victory.h"

typedef void* SimView_DAT_005119b8;
#undef DAT_005119b8
#define DAT_005119b8 (*reinterpret_cast<SimView_DAT_005119b8*>(&Sim().DAT_005119b8))

struct Unit;
class HapiBank;

#include "../util/vec3.h"

static inline Vec3 MakeVec3(int ax, int ay, int az)
{
    Vec3 r;
    r.x = ax;
    r.y = ay;
    r.z = az;
    return r;
}

class TdfRecord;

struct ConditionParams {
    char unknown_0[4];
    TdfRecord* reader;
};

#include "../../interfaces/ref.h"
#include "../units/unit_def.h"
#pragma pack(push, 1)
struct PlayerInfoSide {
    char unknown_0[0x95];
    unsigned char side;
};

struct PlayerInfoLink {
    char unknown_0[0x27];
    Ref<PlayerInfoSide> owner;
};

#include "../units/unit.h"

struct UnitList {
    Ref<Unit> first;
    Ref<Unit> last;

    void ForEach(UnitScanVisitor* visitor)
    {
        for (Unit* unit = first; unit <= last; unit++) {
            if (((short)unit->unitDefIndex) != 0) {
                int result = visitor->VisitUnit(unit);
                if (!result) {
                    break;
                }
            }
        }
    }
};

#include "../network/player.h"
#include "../network/player_info.h"

struct PlayerName {
    char unknown_0[0x22];
    char commander[0x20];
    char unknown_42[0x232 - 0x42];
};

#include "../map/mission.h"

#include "game.h"
#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;

void __stdcall PlaySoundByName(char* str, int flag);
#include "../../interfaces/seams.h"

static inline void AddCondition(Ref<MissionCondition>* list, int* count, MissionCondition* value)
{
    list[*count] = value;
    (*count)++;
}

// FUNCTION: 0x48dfb0
void MissionConditions::FreeConditions()
{
    int i;
    for (i = 0; i < victoryCount; i++) {
        operator delete(victory[i]);
    }
    for (i = 0; i < defeatCount; i++) {
        operator delete(defeat[i]);
    }
}

// FUNCTION: 0x48e010
void MissionConditions::RegisterConditions(ConditionParams* p)
{
    char buf[0x100];
    char stype[0x100];

    int r1 = p->reader->GetFieldInt("KillEnemyCommander", 0);
    if (r1 != 0) {
        AddCondition(victory, &victoryCount, new (kSim) VictoryKillEnemyCommander);
    }
    int r2 = p->reader->GetFieldInt("DestroyAllUnits", 0);
    if (r2 != 0) {
        AddCondition(victory, &victoryCount, new (kSim) VictoryDestroyAllUnits);
    }
    int r3 = p->reader->GetFieldInt("KillAllMobileUnits", 0);
    if (r3 != 0) {
        AddCondition(victory, &victoryCount, new (kSim) VictoryKillAllMobileUnits);
    }
    int r4 = p->reader->GetFieldString(buf, "BuildUnitType", 0x100, (char*)&DAT_005119b8);
    if (r4 != 0) {
        AddCondition(victory, &victoryCount, new (kSim) VictoryBuildUnitType(buf));
    }
    int r5 = p->reader->GetFieldString(buf, "CaptureUnitType", 0x100, (char*)&DAT_005119b8);
    if (r5 != 0) {
        AddCondition(victory, &victoryCount, new (kSim) VictoryCaptureUnitType(buf));
    }
    int r6 = p->reader->GetFieldString(buf, "KillAllOfType", 0x100, (char*)&DAT_005119b8);
    if (r6 != 0) {
        AddCondition(victory, &victoryCount, new (kSim) VictoryKillAllOfType(buf));
    }
    int r7 = p->reader->GetFieldString(buf, "KillUnitType", 0x100, (char*)&DAT_005119b8);
    if (r7 != 0) {
        int n;
        sscanf(buf, "%[a-zA-Z],%i", stype, &n);
        AddCondition(victory, &victoryCount, new (kSim) VictoryKillUnitType(stype, n));
    }
    int r8 = p->reader->GetFieldString(buf, "MoveUnitToRadius", 0x100, (char*)&DAT_005119b8);
    if (r8 != 0) {
        int a, b, c;
        sscanf(buf, "%[a-zA-Z],%i,%i,%i", stype, &a, &b, &c);
        AddCondition(victory, &victoryCount, new (kSim) VictoryMoveUnitToRadius(stype, a, b, c));
    }
    int r9 = p->reader->GetFieldString(buf, "UnitTypePassesX", 0x100, (char*)&DAT_005119b8);
    if (r9 != 0) {
        int n;
        sscanf(buf, "%[a-zA-Z],%i", stype, &n);
        AddCondition(victory, &victoryCount, new (kSim) VictoryUnitTypePassesX(stype, n));
    }
    int r10 = p->reader->GetFieldString(buf, "UnitTypePassesZ", 0x100, (char*)&DAT_005119b8);
    if (r10 != 0) {
        int n;
        sscanf(buf, "%[a-zA-Z],%i", stype, &n);
        AddCondition(victory, &victoryCount, new (kSim) VictoryUnitTypePassesZ(stype, n));
    }
    {
        int t = p->reader->GetFieldInt("VictoryTimerRunsOut", 0);
        if (t > 0) {
            AddCondition(victory, &victoryCount, new (kSim) VictoryTimerRunsOut(t));
        }
    }
    int r11 = p->reader->GetFieldInt("CommanderKilled", 0);
    if (r11 != 0) {
        AddCondition(defeat, &defeatCount, new (kSim) DefeatCommanderKilled);
    }
    int r12 = p->reader->GetFieldInt("AllUnitsKilled", 0);
    if (r12 != 0) {
        AddCondition(defeat, &defeatCount, new (kSim) DefeatAllUnitsKilled);
    }
    int r13 = p->reader->GetFieldString(buf, "AllUnitsKilledOfType", 0x100, (char*)&DAT_005119b8);
    if (r13 != 0) {
        AddCondition(defeat, &defeatCount, new (kSim) DefeatAllUnitsKilledOfType(buf));
    }
    int r14 = p->reader->GetFieldString(buf, "UnitTypeKilled", 0x100, (char*)&DAT_005119b8);
    if (r14 != 0) {
        int n;
        sscanf(buf, "%[a-zA-Z],%i", stype, &n);
        AddCondition(defeat, &defeatCount, new (kSim) DefeatUnitTypeKilled(stype, n));
    }
    {
        int t = p->reader->GetFieldInt("DeathTimerRunsOut", 0);
        if (t > 0) {
            AddCondition(defeat, &defeatCount, new (kSim) DefeatDeathTimerRunsOut(t));
        }
    }
    {
        int t = p->reader->GetFieldInt("AnyUnitPassesX", -1);
        if (t >= 0) {
            AddCondition(defeat, &defeatCount, new (kSim) DefeatAnyUnitPassesX(t));
        }
    }
    {
        int t = p->reader->GetFieldInt("AnyUnitPassesZ", -1);
        if (t >= 0) {
            AddCondition(defeat, &defeatCount, new (kSim) DefeatAnyUnitPassesZ(t));
        }
    }
    if (victoryCount == 0) {
        AddCondition(victory, &victoryCount, new (kSim) VictoryDestroyAllUnits);
    }
    if (defeatCount == 0) {
        AddCondition(defeat, &defeatCount, new (kSim) DefeatAllUnitsKilled);
    }
}

// FUNCTION: 0x48ea00
int MissionCondition::IsSatisfied()
{
    return satisfied;
}

// FUNCTION: 0x48ea10
void MissionCondition::OnUnitDied(Unit*)
{
}

// FUNCTION: 0x48ea20
void MissionCondition::OnUnitCaptured(Unit*)
{
}

// FUNCTION: 0x48ea30
void MissionCondition::OnUnitCreated(Unit*)
{
}

#include "../../link/globals.h"

// FUNCTION: 0x48ea40
void VictoryKillEnemyCommander::OnUnitDied(Unit* unit)
{
    if (unit->playerIndex == 1) {
        if (_strcmpi(unit->def->unitname, (*(PlayerName(*)[5])&g_game->sideDefs)[((PlayerInfoLink *)unit->player.get())->owner->side].commander) == 0) {
            satisfied = 1;
            if (celebrated == 0) {
                PlaySoundByName("Victory Condition", 0);
                celebrated = 1;
            }
        }
    }
}

// FUNCTION: 0x48eac0
void VictoryKillEnemyCommander::SaveState(HapiBank* obj)
{
    obj->OpenAccount(g_killEnemyCommanderKey);
    obj->SetIntegerItem(g_satisfiedKey, satisfied);
    obj->SetIntegerItem(g_celebratedKey, celebrated);
}

// FUNCTION: 0x48eb00
void VictoryKillEnemyCommander::LoadState(HapiBank* obj)
{
    obj->OpenAccount(g_killEnemyCommanderKey);
    satisfied = obj->GetIntegerItem(g_satisfiedKey, 0);
    celebrated = obj->GetIntegerItem(g_celebratedKey, 0);
}

// FUNCTION: 0x48eb40
int VictoryDestroyAllUnits::IsSatisfied()
{
    if (g_game->field_1df2 == 0) {
        if (celebrated == 0) {
            PlaySoundByName("Victory Condition", 0);
            celebrated = 1;
        }
        return 1;
    }
    return 0;
}

// FUNCTION: 0x48eb80
void VictoryDestroyAllUnits::SaveState(HapiBank* obj)
{
    obj->OpenAccount(g_destroyAllUnitsKey);
    obj->SetIntegerItem(g_satisfiedKey, satisfied);
    obj->SetIntegerItem(g_celebratedKey, celebrated);
}

// FUNCTION: 0x48ebc0
void VictoryDestroyAllUnits::LoadState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_DestroyAllUnits");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}


// FUNCTION: 0x48ec00
int VictoryKillAllMobileUnits::VisitUnit(Unit* unit)
{
    if (unit->motion != 0) {
        numUnits++;
    }
    return numUnits <= 1;
}

// FUNCTION: 0x48ec20
void VictoryKillAllMobileUnits::OnUnitDied(Unit* unit)
{
    if (unit->playerIndex == 1 && unit->motion != 0) {
        numUnits = 0;
        (*(struct UnitList*)((char*)g_game + 0x1d15)).ForEach(this);
        if (numUnits <= 1) {
            satisfied = 1;
            if (celebrated == 0) {
                PlaySoundByName("Victory Condition", 0);
                celebrated = 1;
            }
        }
    }
}

// FUNCTION: 0x48ecb0
void VictoryKillAllMobileUnits::SaveState(HapiBank* obj)
{
    obj->OpenAccount(g_killAllMobileUnitsKey);
    obj->SetIntegerItem(g_numUnitsKey, numUnits);
    obj->SetIntegerItem(g_satisfiedKey, satisfied);
    obj->SetIntegerItem(g_celebratedKey, celebrated);
}

// FUNCTION: 0x48ed00
void VictoryKillAllMobileUnits::LoadState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_KillAllMobileUnits");
    numUnits = obj->GetIntegerItem("NumUnits", 0);
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48ed50
int VictoryBuildUnitType::VisitUnit(Unit* unit)
{
    if (((short)unit->unitDefIndex) == id && unit->buildLeft == 0.0f) {
        satisfied = 1;
        if (celebrated == 0) {
            PlaySoundByName("Victory Condition", 0);
            celebrated = 1;
        }
    }
    return satisfied == 0;
}

// FUNCTION: 0x48edb0
int VictoryBuildUnitType::IsSatisfied()
{
    if (satisfied != 0) {
        return 1;
    }
    if (id == 0) {
        id = g_seams.units->FindUnitTypeId(name);
    }
    (*(struct UnitList*)((char*)g_game + 0x1bca)).ForEach(this);
    return satisfied;
}

// FUNCTION: 0x48ee30
void VictoryBuildUnitType::SaveState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_BuildUnitType");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48ee70
void VictoryBuildUnitType::LoadState(HapiBank* obj)
{
    obj->OpenAccount(g_buildUnitTypeKey);
    satisfied = obj->GetIntegerItem(g_satisfiedKey, 0);
    celebrated = obj->GetIntegerItem(g_celebratedKey, 0);
}

// FUNCTION: 0x48eeb0
void VictoryCaptureUnitType::OnUnitCaptured(Unit* unit)
{
    if (unit->playerIndex == 1 && _strcmpi(name, unit->def->unitname) == 0) {
        satisfied = 1;
        if (celebrated == 0) {
            PlaySoundByName("Victory Condition", 0);
            celebrated = 1;
        }
    }
}

// FUNCTION: 0x48ef00
void VictoryCaptureUnitType::SaveState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_CaptureUnitType");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48ef40
void VictoryCaptureUnitType::LoadState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_CaptureUnitType");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48ef80
int VictoryKillAllOfType::VisitUnit(Unit* unit)
{
    if (((short)unit->unitDefIndex) == id) {
        count++;
    }
    return count <= 1;
}

// FUNCTION: 0x48efb0
void VictoryKillAllOfType::OnUnitDied(Unit* unit)
{
    if (satisfied == 0 && unit->playerIndex == 1 && _strcmpi(name, unit->def->unitname) == 0) {
        id = g_seams.units->FindUnitTypeId(name);
        count = 0;
        (*(struct UnitList*)((char*)g_game + 0x1d15)).ForEach(this);
        if (count <= 1) {
            satisfied = 1;
            if (celebrated == 0) {
                PlaySoundByName("Victory Condition", 0);
                celebrated = 1;
            }
        }
    }
}

// FUNCTION: 0x48f070
void VictoryKillAllOfType::SaveState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_KillAllOfType");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48f0b0
void VictoryKillAllOfType::LoadState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_KillAllOfType");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}


// FUNCTION: 0x48f0f0
void VictoryKillUnitType::OnUnitDied(Unit* unit)
{
    if (numLeftToKill > 0 && unit->playerIndex == 1 && _strcmpi(name, unit->def->unitname) == 0) {
        if (--numLeftToKill <= 0) {
            satisfied = 1;
            if (celebrated == 0) {
                PlaySoundByName("Victory Condition", 0);
                celebrated = 1;
            }
        }
    }
}

// FUNCTION: 0x48f160
void VictoryKillUnitType::SaveState(HapiBank* obj)
{
    obj->OpenAccount(g_killUnitTypeKey);
    obj->SetIntegerItem(g_numLeftToKillKey, numLeftToKill);
    obj->SetIntegerItem(g_satisfiedKey, satisfied);
    obj->SetIntegerItem(g_celebratedKey, celebrated);
}

// FUNCTION: 0x48f1b0
void VictoryKillUnitType::LoadState(HapiBank* obj)
{
    obj->OpenAccount(g_killUnitTypeKey);
    numLeftToKill = obj->GetIntegerItem(g_numLeftToKillKey, 0);
    satisfied = obj->GetIntegerItem(g_satisfiedKey, 0);
    celebrated = obj->GetIntegerItem(g_celebratedKey, 0);
}

// FUNCTION: 0x48f200
int VictoryMoveUnitToRadius::IsSatisfied()
{
    if (pos.y == 0x12345678) {
        g_seams.los_radar->ClampWorldPosToTerrain(pos.x, pos.z, &pos);
    }
    g_seams.map->VisitObjectsInRange(&pos, radius, (UnitVisitor*)(RadiusUnitVisitor*)this);
    return satisfied;
}

// FUNCTION: 0x48f250
void VictoryMoveUnitToRadius::VisitUnit(Unit* unit)
{
    if (unit->playerIndex == 0) {
        if (name[0] != 0 && _strcmpi(name, unit->def->unitname) != 0) {
            return;
        }
        if ((unit->flags & 0x20) && unit->buildLeft == 0.0f && unit->postTransferHoldoff == 0
            && ((unit->carrier) == 0 || (unit->carrier->flags & 0x40000000))) {
            satisfied = 1;
            if (celebrated == 0) {
                PlaySoundByName("Victory Condition", 0);
                celebrated = 1;
            }
        }
    }
}

// FUNCTION: 0x48f2f0
void VictoryMoveUnitToRadius::SaveState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_MoveUnitToRadius");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48f330
void VictoryMoveUnitToRadius::LoadState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_MoveUnitToRadius");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48f370
int VictoryUnitTypePassesX::VisitUnit(Unit* unit)
{
    if (name[0] == 0 || _strcmpi(name, unit->def->unitname) == 0) {
        if (abs(unit->cell.x - x) <= 2) {
            satisfied = 1;
            if (celebrated == 0) {
                PlaySoundByName("Victory Condition", 0);
                celebrated = 1;
            }
        }
    }
    return satisfied == 0;
}

// FUNCTION: 0x48f3e0
int VictoryUnitTypePassesX::IsSatisfied()
{
    if (satisfied == 0) {
        (*(struct UnitList*)((char*)g_game + 0x1bca)).ForEach(this);
    }
    return satisfied;
}

// FUNCTION: 0x48f440
void VictoryUnitTypePassesX::SaveState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_UnitTypePassesX");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48f480
void VictoryUnitTypePassesX::LoadState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_UnitTypePassesX");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48f4c0
int VictoryUnitTypePassesZ::VisitUnit(Unit* unit)
{
    if (name[0] == 0 || _strcmpi(name, unit->def->unitname) == 0) {
        if (abs(unit->cell.y - z) <= 2) {
            satisfied = 1;
            if (celebrated == 0) {
                PlaySoundByName("Victory Condition", 0);
                celebrated = 1;
            }
        }
    }
    return satisfied == 0;
}

// FUNCTION: 0x48f530
int VictoryUnitTypePassesZ::IsSatisfied()
{
    if (satisfied == 0) {
        (*(struct UnitList*)((char*)g_game + 0x1bca)).ForEach(this);
    }
    return satisfied;
}

// FUNCTION: 0x48f590
void VictoryUnitTypePassesZ::SaveState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_UnitTypePassesZ");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48f5d0
void VictoryUnitTypePassesZ::LoadState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_UnitTypePassesZ");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48f610
int VictoryTimerRunsOut::IsSatisfied()
{
    unsigned int game_val = ((unsigned int)g_game->ticks);
    return game_val >= endTick;
}

// FUNCTION: 0x48f630
void VictoryTimerRunsOut::SaveState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_VictoryTimerRunsOut");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48f670
void VictoryTimerRunsOut::LoadState(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_VictoryTimerRunsOut");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48f6b0
void DefeatCommanderKilled::OnUnitDied(Unit* unit)
{
    if (unit->playerIndex == 0) {
        if (_strcmpi(unit->def->unitname, (*(PlayerName(*)[5])&g_game->sideDefs)[((PlayerInfoLink *)unit->player.get())->owner->side].commander) == 0) {
            satisfied = 1;
        }
    }
}

// FUNCTION: 0x48f710
void DefeatCommanderKilled::SaveState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_CommanderKilled");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48f750
void DefeatCommanderKilled::LoadState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_CommanderKilled");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48f790
int DefeatAllUnitsKilled::VisitUnit(Unit* unit)
{
    if ((unit->flags & 0x20) && unit->buildLeft == 0.0f && unit->postTransferHoldoff == 0
        && ((unit->carrier) == 0 || (unit->carrier->flags & 0x40000000))) {
        satisfied = 0;
    }
    return satisfied;
}

// FUNCTION: 0x48f7e0
int DefeatAllUnitsKilled::IsSatisfied()
{
    satisfied = 1;
    (*(struct UnitList*)((char*)g_game + 0x1bca)).ForEach(this);
    return satisfied;
}

// FUNCTION: 0x48f840
void DefeatAllUnitsKilled::SaveState(HapiBank* obj)
{
    obj->OpenAccount(g_allUnitsKilledKey);
    obj->SetIntegerItem(g_satisfiedKey, satisfied);
    obj->SetIntegerItem(g_celebratedKey, celebrated);
}

// FUNCTION: 0x48f880
void DefeatAllUnitsKilled::LoadState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AllUnitsKilled");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48f8c0
void DefeatUnitTypeKilled::OnUnitDied(Unit* unit)
{
    if (_strcmpi(name, unit->def->unitname) == 0) {
        if (--numLeftToKill <= 0) {
            satisfied = 1;
        }
    }
}

// FUNCTION: 0x48f900
void DefeatUnitTypeKilled::SaveState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_UnitTypeKilled");
    obj->SetIntegerItem("NumLeftToKill", numLeftToKill);
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48f950
void DefeatUnitTypeKilled::LoadState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_UnitTypeKilled");
    numLeftToKill = obj->GetIntegerItem("NumLeftToKill", 0);
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48f9a0
int DefeatAllUnitsKilledOfType::VisitUnit(Unit* unit)
{
    if (((short)unit->unitDefIndex) == id) {
        count++;
    }
    return count <= 1;
}

// FUNCTION: 0x48f9d0
void DefeatAllUnitsKilledOfType::OnUnitDied(Unit* unit)
{
    if (_strcmpi(name, unit->def->unitname) == 0) {
        id = g_seams.units->FindUnitTypeId(name);
        count = 0;
        (*(struct UnitList*)((char*)g_game + 0x1bca)).ForEach(this);
        (*(struct UnitList*)((char*)g_game + 0x1d15)).ForEach(this);
        if (count <= 1) {
            satisfied = 1;
        }
    }
}

// FUNCTION: 0x48fab0
void DefeatAllUnitsKilledOfType::SaveState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AllUnitsKilledOfType");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48faf0
void DefeatAllUnitsKilledOfType::LoadState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AllUnitsKilledOfType");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48fb30
int DefeatAnyUnitPassesX::VisitUnit(Unit* unit)
{
    int diff = abs((int)unit->cell.x - x);

    if (diff <= 2) {
        satisfied = 1;
    }
    return satisfied == 0;
}

// FUNCTION: 0x48fb60
int DefeatAnyUnitPassesX::IsSatisfied()
{
    if (satisfied == 0) {
        (*(struct UnitList*)((char*)g_game + 0x1d15)).ForEach(this);
    }
    return satisfied;
}

// FUNCTION: 0x48fbc0
void DefeatAnyUnitPassesX::SaveState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AnyUnitPassesX");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48fc00
void DefeatAnyUnitPassesX::LoadState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AnyUnitPassesX");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48fc40
int DefeatAnyUnitPassesZ::VisitUnit(Unit* unit)
{
    int diff = abs((int)unit->cell.y - z);

    if (diff <= 2) {
        satisfied = 1;
    }
    return satisfied == 0;
}

// FUNCTION: 0x48fc70
int DefeatAnyUnitPassesZ::IsSatisfied()
{
    if (satisfied == 0) {
        (*(struct UnitList*)((char*)g_game + 0x1d15)).ForEach(this);
    }
    return satisfied;
}

// FUNCTION: 0x48fcd0
void DefeatAnyUnitPassesZ::SaveState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AnyUnitPassesZ");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48fd10
void DefeatAnyUnitPassesZ::LoadState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AnyUnitPassesZ");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48fd50
int DefeatDeathTimerRunsOut::IsSatisfied()
{
    unsigned int game_val = ((unsigned int)g_game->ticks);
    return game_val >= (unsigned int)endTick;
}

// FUNCTION: 0x48fd70
void DefeatDeathTimerRunsOut::SaveState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_DeathTimerRunsOut");
    obj->SetIntegerItem("Satisfied", satisfied);
    obj->SetIntegerItem("Celebrated", celebrated);
}

// FUNCTION: 0x48fdb0
void DefeatDeathTimerRunsOut::LoadState(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_DeathTimerRunsOut");
    satisfied = obj->GetIntegerItem("Satisfied", 0);
    celebrated = obj->GetIntegerItem("Celebrated", 0);
}

// FUNCTION: 0x48fdf0
void MissionConditions::SaveConditions(HapiBank* file)
{
    if (g_seams.map->GetGameType(g_game->net) == 1) {
        int i;
        for (i = 0; i < victoryCount; i++) {
            victory[i]->SaveState(file);
        }
        for (i = 0; i < defeatCount; i++) {
            defeat[i]->SaveState(file);
        }
    }
}

// FUNCTION: 0x48fe60
void MissionConditions::LoadConditions(HapiBank* file)
{
    if (g_seams.map->GetGameType(g_game->net) == 1) {
        int i;
        for (i = 0; i < victoryCount; i++) {
            victory[i]->LoadState(file);
        }
        for (i = 0; i < defeatCount; i++) {
            defeat[i]->LoadState(file);
        }
    }
}

// FUNCTION: 0x48fed0
int MissionConditions::AllVictoryConditionsMet()
{
    if (victoryCount == 0) {
        victory[victoryCount] = new (kSim) VictoryDestroyAllUnits;
        victoryCount++;
    }
    for (int i = 0; i < victoryCount; i++) {
        if (!victory[i]->IsSatisfied())
            return 0;
    }
    return 1;
}

// FUNCTION: 0x48ff40
int MissionConditions::AnyDefeatConditionMet()
{
    if (defeatCount == 0) {
        defeat[defeatCount] = new (kSim) DefeatAllUnitsKilled;
        defeatCount++;
    }
    for (int i = 0; i < defeatCount; i++) {
        if (defeat[i]->IsSatisfied())
            return 1;
    }
    return 0;
}

// FUNCTION: 0x48ffd0
int AreEnemiesEliminated()
{
    Player* p = &g_game->players[g_game->localPlayer];
    for (unsigned char i = 0; i < 10; i++) {
        if (i != g_game->localPlayer && !p->allied[i] && g_game->players[i].unitCount != 0)
            return 0;
    }
    return 1;
}

// FUNCTION: 0x490050
int IsLocalPlayerEliminated()
{
    return g_game->players[g_game->localPlayer].unitCount == 0 ? 1 : 0;
}

// FUNCTION: 0x490080
int CheckAlliedVictory()
{
    Player* mine;
    Player* other;
    unsigned char i;
    int j;
    unsigned char state;

    if (g_game->commanderDeath == 2) {
        return 0;
    }
    mine = &g_game->players[g_game->localPlayer];
    for (i = 0; i < 10; i++) {
        other = &g_game->players[i];
        if (i == g_game->localPlayer) {
            continue;
        }
        if (other->active == 0) {
            continue;
        }
        state = other->type;
        if (state == 1 || state == 2 || state == 3) {
            if (other->index == 0xa) {
                continue;
            }
            if (other->info->flags_9b & 0x40) {
                continue;
            }
            if (other->unitsCreated == 0) {
                return 0;
            }
            if (state == 1 || state == 2 || state == 3) {
                if (other->unitCount == 0) {
                    continue;
                }
                if (!(other->info->flags_9d & 2)) {
                    return 0;
                }
                if (!(mine->info->flags_9d & 2)) {
                    return 0;
                }
                if (mine->allied[i] == 0) {
                    return 0;
                }
                if (mine->alliedBy[i] == 0) {
                    return 0;
                }
                for (j = 0; j < 10; j++) {
                    Player* o = &g_game->players[j];
                    if (o->active != 0
                        && (o->type == 1 || o->type == 2 || o->type == 3)
                        && o->index != 0xa
                        && (o->unitCount != 0 || o->unitsCreated == 0)) {
                        if (other->allied[j] == 0) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 1;
}

// FUNCTION: 0x490200
int IsLocalPlayerOutOfUnits()
{
    return g_game->players[g_game->localPlayer].unitCount == 0 ? 1 : 0;
}

// FUNCTION: 0x490230
int MissionConditions::CheckVictory()
{
    if (active == 0)
        return 0;
    switch (g_seams.map->GetGameType(g_game->net)) {
    case 1:
        return AllVictoryConditionsMet();
    case 2: {
        unsigned char player = g_game->localPlayer;
        Player* me = &g_game->players[player];
        for (unsigned char i = 0; i < 10; i++) {
            unsigned char j = i;
            if (i == g_game->localPlayer)
                continue;
            if (me->allied[j])
                continue;
            if (g_game->players[j].unitCount != 0)
                return 0;
        }
        return 1;
    }
    case 3:
        return CheckAlliedVictory();
    }
    return 0;
}

// FUNCTION: 0x490360
int MissionConditions::CheckDefeat(RandStream stream)
{
    if (active) {
        switch (g_seams.map->GetGameType(g_game->net)) {
        case 1:
            return AnyDefeatConditionMet();
        case 2:
            return g_game->players[g_game->localPlayer].unitCount == 0;
        case 3:
            return g_game->players[g_game->localPlayer].unitCount == 0;
        }
    }
    return 0;
}

// FUNCTION: 0x4904b0
void MissionConditions::Deactivate()
{
    active = 0;
}

// FUNCTION: 0x4904c0
void MissionConditions::NotifyUnitDied(Unit* unit)
{
    int i;
    for (i = 0; i < victoryCount; i++)
        victory[i]->OnUnitDied(unit);
    for (i = 0; i < defeatCount; i++)
        defeat[i]->OnUnitDied(unit);
}

// FUNCTION: 0x490520
void MissionConditions::NotifyUnitCaptured(Unit* unit)
{
    int i;
    for (i = 0; i < victoryCount; i++) {
        victory[i]->OnUnitCaptured(unit);
    }
    for (i = 0; i < defeatCount; i++) {
        defeat[i]->OnUnitCaptured(unit);
    }
}

// FUNCTION: 0x490580
void MissionConditions::NotifyUnitCreated(Unit* unit)
{
    int i;
    for (i = 0; i < victoryCount; i++) {
        victory[i]->OnUnitCreated(unit);
    }
    for (i = 0; i < defeatCount; i++) {
        defeat[i]->OnUnitCreated(unit);
    }
}

#include "../units/unit_def.h"

#pragma pack(push, 1)
struct UnitDefSeaFlags {
    char unknown_0[0x241];
    unsigned int unknown_241_0 : 22;
    unsigned int seaUnit : 1;
    unsigned int unknown_241_1 : 9;
};
#pragma pack(pop)

#include "../network/bit_writer.h"

#include "../network/bit_reader.h"

struct GoalOwnerUnit;

#include "../orders/unit_motion.h"
#include "../util/angles.h"

class AiSearchGoal;

#include "../orders/air_maneuver_order.h"

#include "packed_goal.h"
#include "../orders/order_goals.h"

unsigned short __stdcall GetHeadingBetween(Vec3* from, Vec3* to);

// FUNCTION: 0x4905e0
// FUNCTION: 0x490630 ??_GPackedGoal@@UAEPAXI@Z
PackedGoal::PackedGoal(GoalOwnerUnit* p)
    : PathGoal((Unit*)p)
{
    pos = p->pos;
    vel = MakeVec3(0, 0, 0);
    heading = p->angles.heading;
}

// FUNCTION: 0x490650
void PackedGoal::ExportGoalPose(Vec3* outPos, Vec3* outVel,
                                  short* outHeading)
{
    *outPos = pos;
    *outVel = vel;
    *outHeading = heading;
}

// FUNCTION: 0x490690
void PackedGoal::TickTowardGoal()
{
    if (!target)
        return;
    Vec3 old = pos;
    ((OrderFx*)target)->FillWorldPos(&pos);
    vel = pos - old;
    int dist = (int)_hypot(owner->pos.x - pos.x, owner->pos.z - pos.z);
    if (dist > 0xa00000) {
        if (((UnitDefSeaFlags*)owner->def.get())->seaUnit)
            pos.y = (g_game->seaLevel + owner->def->altitude) << 16;
        else
            pos.y = (owner->def->altitude + ((unsigned char*)owner->spatialBucket.get())[1]) << 16;
    }
    if (dist > 0x1400000 || (!((OrderFx*)target)->GetDesiredHeading((unsigned short*)&heading) && dist > 0x100000))
        heading = (short)GetHeadingBetween(&owner->pos, &pos);
    if (((OrderFx*)target)->ContainsUnit((Unit*)owner)) {
        ((OrderFx*)target)->AddFlags(0x20);
        if (!((OrderFx*)target)->KeepAfterComplete())
            SetPathOrder(0);
    }
}

// FUNCTION: 0x4907e0
// FUNCTION: 0x490840 ??_GLiteGoal@@UAEPAXI@Z
LiteGoal::LiteGoal(Unit* p)
    : PackedGoal((GoalOwnerUnit*)p)
{
    dirty = 1;
    mode = 0;
}

// FUNCTION: 0x490860
void LiteGoal::SetPathOrder(void* param)
{
    PathGoal::SetPathOrder(param);
    dirty = 1;
}

// FUNCTION: 0x490880
void LiteGoal::TickTowardGoal()
{
    if ((((GoalMotionFlags*)owner->motion.get())->flags & 3) != mode)
        dirty = 1;
    PackedGoal::TickTowardGoal();
}

// FUNCTION: 0x4908b0
int LiteGoal::HasNetUnitState()
{
    return netDirtyFlags & 1;
}

// FUNCTION: 0x4908c0
void LiteGoal::SerializeNetUnitState(BitWriter* stream)
{
    if (target == 0) {
        stream->WriteBits(0, 2);
    } else if (((OrderFx*)target)->GetType() == 2) {
        stream->WriteBits(1, 2);
        ((OrderFx*)target)->SerializeToBits(stream);
    } else if (((OrderFx*)target)->GetType() == 3) {
        stream->WriteBits(2, 2);
        ((OrderFx*)target)->SerializeToBits(stream);
    }
    stream->WriteBits(((GoalMotionFlags*)owner->motion.get())->mode, 2);
    state = ((GoalMotionFlags*)owner->motion.get())->mode << 1;
}

// FUNCTION: 0x490940
PackedPosGoal::PackedPosGoal(Unit* p)
    : PackedGoal((GoalOwnerUnit*)p)
{
}

// FUNCTION: 0x4909a0 ??_GPackedPosGoal@@UAEPAXI@Z
// FUNCTION: 0x4909e0
PackedPosGoal::~PackedPosGoal()
{
    delete (OrderFx*)target;
    target = 0;
}

// FUNCTION: 0x490a10
void PackedPosGoal::DeserializeNetUnitState(BitReader* reader)
{
    if (target) {
        delete (OrderFx*)target;
        target = 0;
    }
    int kind = reader->ReadBits(2);
    if (kind == 1)
        target = new (kSim) PathOrderView((GoalOwnerUnit*)owner.get(), reader);
    else if (kind == 2)
        target = new (kSim) AirManeuverOrder(owner, reader);
    int state = reader->ReadBits(2);
    owner->motion->SetFlightMode(owner, state);
}

// FUNCTION: 0x490aa0
void QueryGlobalMemoryStatus(void)
{
    host_memory mem;
    host_memory_status(&mem);
}

#define CONDITION_PROBE(cls, probe)                 \
    {                                               \
        probe;                                      \
        if (*(void**)&p == vtable)                  \
            return cls;                             \
    }

int HarnessConditionClass(void* condition)
{
    void* vtable = *(void**)condition;
    CONDITION_PROBE(COND_VICTORY_KILL_COMMANDER, VictoryKillEnemyCommander p);
    CONDITION_PROBE(COND_VICTORY_DESTROY_ALL, VictoryDestroyAllUnits p);
    CONDITION_PROBE(COND_VICTORY_KILL_MOBILE, VictoryKillAllMobileUnits p);
    CONDITION_PROBE(COND_VICTORY_BUILD, VictoryBuildUnitType p(""));
    CONDITION_PROBE(COND_VICTORY_CAPTURE, VictoryCaptureUnitType p(""));
    CONDITION_PROBE(COND_VICTORY_KILL_ALL_OF_TYPE, VictoryKillAllOfType p(""));
    CONDITION_PROBE(COND_VICTORY_KILL_TYPE, VictoryKillUnitType p("", 0));
    CONDITION_PROBE(COND_VICTORY_MOVE_TO_RADIUS, VictoryMoveUnitToRadius p("", 0, 0, 0));
    CONDITION_PROBE(COND_VICTORY_PASSES_X, VictoryUnitTypePassesX p("", 0));
    CONDITION_PROBE(COND_VICTORY_PASSES_Z, VictoryUnitTypePassesZ p("", 0));
    CONDITION_PROBE(COND_VICTORY_TIMER, VictoryTimerRunsOut p(0));
    CONDITION_PROBE(COND_DEFEAT_COMMANDER, DefeatCommanderKilled p);
    CONDITION_PROBE(COND_DEFEAT_ALL_KILLED, DefeatAllUnitsKilled p);
    CONDITION_PROBE(COND_DEFEAT_ALL_OF_TYPE, DefeatAllUnitsKilledOfType p(""));
    CONDITION_PROBE(COND_DEFEAT_TYPE_KILLED, DefeatUnitTypeKilled p("", 0));
    CONDITION_PROBE(COND_DEFEAT_DEATH_TIMER, DefeatDeathTimerRunsOut p(0));
    CONDITION_PROBE(COND_DEFEAT_PASSES_X, DefeatAnyUnitPassesX p(0));
    CONDITION_PROBE(COND_DEFEAT_PASSES_Z, DefeatAnyUnitPassesZ p(0));
    return COND_UNKNOWN;
}
