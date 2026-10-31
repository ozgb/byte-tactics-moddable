struct Unit;

#include "unit_orders.h"
namespace std {
inline void _Construct(Unit** dest, Unit* const& src) { CopyDwordIfNonNull((int *)dest, (int *)&src); }
}

#include <vector>
#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "../state/block.h"
#include "../util/vec3.h"

static inline short Whole(const int& v) { return ((short*)&v)[1]; }

static inline Vec3 Vec3Add(const Vec3& a, const Vec3& b) {
    Vec3 r; r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; return r;
}

static inline Vec3 Vec3Sub(const Vec3& a, const Vec3& b) {
    Vec3 r; r.z = a.z - b.z; r.y = a.y - b.y; r.x = a.x - b.x; return r;
}

#include "box.h"

struct UnitDef;

#include "../units/cob_script.h"

#include "../sound/sound.h"
#include "../state/sim_speech.h"
#include "../state/sim_effects.h"
struct Chunk;
class SquadManager;
class AssaultTimer;
class EscortTimer;
class SquadScoutTimer;


#include "../game/unit_resources.h"

#include "mission_type.h"

#include "path_order_attach.h"

#pragma pack(push, 1)
#include "../units/unit_def.h"

#include "../network/player.h"

#include "../weapons/weapon_def.h"

#include "../game/game.h"

struct CloakDefView {
    char unknown_0[0x245];
    unsigned int lowbits : 13;
    unsigned int flag : 1;
};

struct UnitDefActivateFlagView {
    char unknown_0[0x245];
    unsigned int unused_bits : 2;
    unsigned int flag : 1;
    unsigned int unused_bits2 : 29;
};

#include "../util/angles.h"
#include "../weapons/unit_weapon_slot.h"

#include "order.h"
int __stdcall FindWeaponTarget(Unit*, unsigned char, int);
void __stdcall ScaleUnitWeights(int, unsigned int*, float, int);
void __stdcall SetUnitLimits(int, unsigned int*, int, int);
#include "../ingame/control_panel.h"

#include "../units/unit.h"
inline int Unit::Ready() { return (flags & 0x10000000) && !(flags & 0x4000); }
#pragma pack(pop)
#include "../map/feature.h"
#include "../map/cell.h"
#include "../map/mission.h"
#include "vtol_orders.h"
#include "../ingame/markers.h"
#include "win_types.h"
#include "../ai/vec.h"

inline int Order::Advance(int distance) {
    ((Order*)this)->AttachApproachRadiusGoal(&target->pos, distance);
    step++;
    return 1;
}

#include "../../interfaces/seams.h"

#include "../../link/globals.h"

void __stdcall RegisterOrderTypes(void* table, int id);
#include "order_list.h"
struct UnitOrderTypeView;
struct FixedPosView;
#include "order_dispatch.h"
#include "order_queue.h"
Unit* __stdcall CreateUnit(unsigned char player, unsigned short type, Vec3 pos,
                           int a, int b, int c);
#include "../graphics/particles.h"
char* __stdcall lstrcpynA(char* dest, const char* src, int count);
int DrawWrappedText(char*, char*, int, int, int, int, int);
extern char DAT_004fc6e8[];
extern const float DAT_004fc930, DAT_004fc934, DAT_004fc938, DAT_004fc93c;
int __stdcall GetHeadingBetween(Vec3* from, Vec3* to);
int __stdcall BroadcastPacket(int player, void* data, int size);
int __stdcall PickRandomReclaimableResourcesInRadius(Vec3* pos, int range, Vec3** energy, float* energyAmount,
                           Vec3** metal, float* metalAmount);
void WriteScreenshot(char*, char*, int, int, int, int);
int ScanDirectory(char*, char*, char*, int, int, int);
int __stdcall GroundPickupOrder(Unit* unit, Order* order, int flags);


// FUNCTION: 0x401c20
int __stdcall StopOrder(Unit* unit, Order* order, int unused)
{
    order->AnnounceStatusIfFlagged(SIM_SPEECH_KIND_TEXT);
    g_seams.targeting->ClearWeaponTarget(unit, 0);
    g_seams.targeting->ClearWeaponTarget(unit, 1);
    g_seams.targeting->ClearWeaponTarget(unit, 2);
    if ((unit->flags & 3) == 2 && (unit->def->flags1 & 0x800)) {
        AppendOrder(unit, new (kSim) Order("VTOL_LANDIFCAN", 0, &unit->pos, 0, 0, 0));
    }
    return 5;
}

// FUNCTION: 0x401cc0
int __stdcall MakeSelectableOrder(Unit* unit, Order* order, int unused)
{
    unit->flags = (unit->flags & ~0x8000) | 0x20;
    return 5;
}

// FUNCTION: 0x401ce0
int __stdcall WaitOrder(Unit* unit, Order* order, int flags)
{
    if(order->radius) {
        std::vector<Unit*> units;
        g_seams.ai->GetVisibleEnemiesInRadius(unit->playerIndex,&unit->pos,order->radius,0,&units);
        if(!units.empty()) return 5;
        if(order->wait<=0) return 5;
        int delay=RandomInt(30)+150;
        order->wait-=delay;
        order->SetDeadlineTicks(delay);
        return 2;
    }
    unsigned state=0;
    state=order->state;
    switch(state) {
    case 0: order->SetDeadlineTicks(order->wait); return 1;
    case 1: return 5;
    default: return 7;
    }
}

struct PadWordE { int field; };

// FUNCTION: 0x401fd0
int __stdcall WaitForAttackOrder(int unused1, Order* obj, int unused3)
{
    if (obj->target == 0) {
        return 5;
    }
    unsigned int s = 0;
    s = obj->state;
    switch (s) {
    case 0:
        obj->flags = 0x18;
        return 1;
    case 1:
        return 5;
    default:
        return 7;
    }
}

// FUNCTION: 0x402010
int __stdcall SelfDestructOrder(Unit* unit, Order* order, int flags)
{
    if(!(order->count&0xf0000000)) order->count=unit->def->selfdestructcountdown|0xf0000000;
    if(!order->done && unit->def->selfdestructcountdown>0) {
        int sounds[6]={22,21,20,19,18,17};
        if(flags&2) {
            if(!(unit->flags&0x4000)) { RecordUnitSpeech(unit,23,SIM_SPEECH_KIND_TEXT); return 5; }
        } else {
            int count=order->count&0x0fffffff;
            if(!count) order->done=1;
            else order->count=(count-1)|0xf0000000;
            if(count>=0) {
                RecordUnitSpeech(unit,sounds[count],SIM_SPEECH_KIND_TEXT);
                if(count>0) {
                    order->SetDeadlineTicks(30);
                    order->flags|=2; return 1;
                }
                if(count==0) {
                    order->SetDeadlineTicks(RandomInt(15));
                    order->flags|=2; return 1;
                }
            }
            g_seams.weapons->DamageUnit(unit,unit,30000,3,0);
        }
    } else g_seams.weapons->DamageUnit(unit,unit,30000,3,0);
    return 5;
}

// FUNCTION: 0x402160
int __stdcall AttackNoMoveOrder(Unit* unit, Order* order, int flags)
{
    if (order->target == 0 || (flags & 0x10808) != 0)
        return 5;
    switch (order->state) {
    case 0:
        order->AnnounceStatusIfFlagged(SIM_SPEECH_KIND_TEXT);
        return 1;
    case 1:
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.targeting->SetWeaponTargetUnit(unit, order->target, 0);
        order->flags = 0x11808;
        return 1;
    case 2:
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        return 9;
    default:
        return 7;
    }
}

// FUNCTION: 0x4021f0
int __stdcall GuardNoMoveOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 0x10008) {
        order->state = 3;
        return 2;
    }
    switch (order->state) {
    case 0:
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->SetDeadlineTicks(0x1e);
        return 1;
    case 1: {
        order->link.SetUnit(g_seams.targeting->GetWeaponTargetUnit(unit, 0));
        Unit* t = order->target;
        if (t != 0 && (t->flags & 0x10000000)) {
            order->pos = t->pos;
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
            g_seams.targeting->SetWeaponTargetUnit(unit, order->target, 0);
            order->wait = 0;
            order->waitLimit = RandomInt(3) + 3;
            return 1;
        }
        order->SetDeadlineTicks(0x1e);
        return 2;
    }
    case 2:
        if (flags & 0x4000)
            order->wait = 0;
        else
            order->wait++;
        if (order->wait <= order->waitLimit && g_seams.weapons->WeaponCanReachUnit(unit, order->target, 0)) {
            order->flags |= 0x7008;
            return 2;
        }
        if (RandomInt(100) < 0x50) {
            order->wait = 0;
            return 1;
        }
        break;
    case 3: {
        std::vector<Unit*> units;
        g_seams.ai->GetVisibleEnemiesInRadius(unit->playerIndex, &order->pos, 0x280, 0, &units);
        if (!units.empty()) {
            order->link.SetUnit(units[RandomInt(units.size())]);
            g_seams.targeting->SetWeaponTargetUnit(unit, order->target, 0);
            order->state = 1;
            return 2;
        }
        break;
    }
    default:
        return 7;
    }
    return 0;
}


// FUNCTION: 0x402430
int __stdcall SelfRepairOrder(Unit* unit, Order* order, int unused)
{
    if (order->target == 0) {
        RecordUnitSpeech(unit, 7, SIM_SPEECH_REPAIR_ABORTED);
        return 8;
    }
    switch (order->state) {
    case 0:
        if (!(order->target->def->flags1Byte & 0x40))
            return 7;
        if (order->target->buildLeft == 0.0f && (unit->activateFlags & 1)) {
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            return 1;
        }
        return 8;
    case 1:
        if ((unsigned int)unit->health >= unit->def->maxHealth)
            return 1;
        unit->workTime = g_game->ticks + 0x96;
        if (g_seams.units->AddRepairProgress(order->target, unit, (float)(order->target->def->buildRate / 30))) {
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(order->target, &nano);
            Box box;
            box.hi = unit->pos;
            box.lo = unit->pos;
            box.lo.x += unit->def->modelMinX;
            box.lo.z += unit->def->modelMinZ;
            box.hi.x += unit->def->modelMaxX;
            box.hi.z += unit->def->modelMaxZ;
            box.hi.y += unit->def->modelMaxY;
            RecordNanoParticles(nano, box, 6);
        }
        order->SetDeadlineTicks(1);
        order->flags |= 8;
        return 2;
    case 2:
        RecordUnitSpeech(unit, 10, SIM_SPEECH_UNIT_REPAIRED);
        return 5;
    default:
        return 7;
    }
}

static inline Point16 GridCell(Vec3 pos, Point16 size)
{
    Point16 cell;
    cell.x = (pos.x - (size.x << 19) + 0x80000) >> 20;
    cell.y = (pos.z - (size.y << 19) + 0x80000) >> 20;
    return cell;
}


// FUNCTION: 0x402640
int __stdcall BuildingBuildOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 2) {
        if (order->target != 0) {
            float refund = (unsigned int)((1.0f - order->target->buildLeft) * order->target->def->metalCost);
            if (unit->resourceSlot.player->active && unit->resourceSlot.player->type == 2) {
                switch (g_game->difficulty) {
                case 1:
                    unit->resourceSlot.metalMake += refund * 0.7;
                    break;
                case 0:
                    unit->resourceSlot.metalMake += refund * 0.5;
                    break;
                default:
                    unit->resourceSlot.metalMake += (double)refund;
                    break;
                }
            } else {
                unit->resourceSlot.metalMake += (double)refund;
            }
            g_seams.units->FinishConstruction(unit, order->target);
            g_seams.weapons->DamageUnit(unit, order->target, 30000, 9, 0);
        }
        g_seams.units->SetStateBits(unit, 9, 0);
        RecordBuildMenuUpdate(unit);
        return 5;
    }
    if (flags & 8) {
        RecordUnitSpeech(unit, 7, SIM_SPEECH_CONSTRUCTION_STOPPED);
        order->count--;
        RecordBuildMenuUpdate(unit);
        return 0;
    }
    switch (order->state) {
    case 0:
        order->link.SetUnit(0);
        if (unit->flags & 0x20000000) {
            if (order->count <= 0) {
                g_seams.units->SetStateBits(unit, 1, 0);
                return 5;
            }
            g_seams.units->SetStateBits(unit, 1, 1);
            return 1;
        }
        break;
    case 1:
        return WaitIfNotInBuildStance(unit, order, 2);
    case 2: {
        int piece = -1;
        g_seams.units->QueryScript((CobScript*)unit->script.get(), "QueryBuildInfo", &piece, 0, 0, 0);
        order->pos = g_seams.units->GetPiecePosition(unit, piece);
        UnitDef* ut = &g_game->defs[order->unitType];
        Point16 cell = GridCell(order->pos, *(Point16*)&ut->footprintx);
        if (!g_seams.map->CanPlaceUnitFootprint(ut, 0, cell, unit->flags & 3)) {
            order->SetDeadlineTicks(15);
            order->flags |= 2;
            return 2;
        }
        order->link.SetUnit(g_seams.units->CreateUnit(unit->playerIndex, order->unitType, order->pos, 0, 1, 0));
        if (order->target == 0) {
            RecordUnitSpeech(unit, 7, SIM_SPEECH_UNABLE_TO_CREATE_ANY_MORE_UNITS);
            order->SetDeadlineTicks(300);
            order->flags |= 2;
            return 2;
        }
        RecordUnitSpeech(unit, 9, SIM_SPEECH_STARTING_CONSTRUCTION);
        g_seams.units->AttachUnitToPiece(order->target, unit, piece, 1);
        order->target->moveOrder = unit->moveOrder;
        order->target->fireOrder = unit->fireOrder;
        AddOrder("getbuilt", 1, order->target, unit, 0, 0, 0);
        g_seams.units->SetStateBits(unit, 8, 1);
        RecordBuildMenuUpdate(unit);
        return 1;
    }
    case 3:
        if (order->target != 0) {
            if (g_seams.units->AddBuildProgress(unit, order->target, (float)(unit->def->buildRate / 30))) {
                Vec3 nano;
                g_seams.units->GetNanoPiecePosition(unit, &nano);
                Box box;
                box.hi = order->target->pos;
                box.lo = order->target->pos;
                box.lo.x += order->target->def->modelMinX;
                box.lo.z += order->target->def->modelMinZ;
                box.hi.x += order->target->def->modelMaxX;
                box.hi.z += order->target->def->modelMaxZ;
                box.hi.y += order->target->def->modelMaxY;
                RecordNanoParticles(nano, box, 6);
            }
            if (order->target->buildLeft != 0.0f) {
                order->SetDeadlineTicks(1);
                order->flags |= 0xa;
                return 2;
            }
            return 1;
        }
        break;
    case 4:
        RecordUnitSpeech(unit, 8, SIM_SPEECH_KIND_TEXT);
        g_seams.units->SetStateBits(unit, 8, 0);
        g_seams.units->FinishConstruction(unit, order->target);
        order->link.SetUnit(0);
        order->count--;
        RecordBuildMenuUpdate(unit);
        return 0;
    }
    return 7;
}

// FUNCTION: 0x402b70
int __stdcall BuildWeaponOrder(Unit* unit, Order* order, int unused)
{
    int energyCharge;
    WeaponDef* t = unit->weapons[order->weapon].weapon;
    switch (order->state) {
    case 0:
        if (order->count <= 0)
            return 5;
        if (unit->weapons[order->weapon].stockpile >= 200) {
            order->SetDeadlineTicks(300);
            return 2;
        }
        order->progress = 0;
        return 1;
    case 1: {
        float fnext, ftotal, fprev;
        int prev = order->progress;
        int next = prev + 5 < t->reloadTime ? prev + 5 : t->reloadTime;
        int total = t->reloadTime;
        fnext = next;
        ftotal = total;
        fprev = prev;
        int metalCharge = (int)(fnext * t->metalPerShot / ftotal) - (int)(fprev * t->metalPerShot / ftotal);
        energyCharge = (int)(fnext * t->energyPerShot / ftotal) - (int)(fprev * t->energyPerShot / ftotal);
        if (g_seams.economy->RequestEnergyAndMetal(&unit->resourceSlot, energyCharge, metalCharge)) {
            order->progress = next;
            if (next >= t->reloadTime)
                return 1;
            order->SetDeadlineTicks(5);
            return 2;
        }
        order->SetDeadlineTicks(10);
        return 2;
    }
    case 2:
        unit->weapons[order->weapon].stockpile++;
        order->count--;
        RecordBuildMenuUpdate(unit);
        return 0;
    default:
        return 7;
    }
}

// FUNCTION: 0x402d10
int __stdcall ParalyzeOrder(Unit* unit, Order* order, int unused)
{
    if (order->ticks == 0) {
        g_seams.units->SetStateBits(unit, 0x10, 0);
        return 5;
    }
    if (order->ticks > 0x708)
        order->ticks = 0x708;
    g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
    for (char i = 0; i < 3; i++)
        g_seams.targeting->ClearWeaponTarget(unit, i);
    order->SetAttachedFx(0);
    order->SetDeadlineTicks(order->ticks);
    order->ticks = 0;
    g_seams.units->SetStateBits(unit, 0x10, 1);
    return 1;
}

// FUNCTION: 0x402da0
int __stdcall GetBuiltOrder(Unit* unit, Order* order, unsigned int flags)
{
    if (unit->buildLeft == 0.0f) {
        RecordSelectionOrdersDirty(unit);
        if (unit->motion) {
            int queued = 0;
            if (order->target) {
                MissionType move("QMove");
                MissionType patrol("QPatrol");
                for (Order* node = order->target->list; node; node = node->next) {
                    MissionType kind;
                    if (node->kind == move)
                        kind = GetOrderType(2, unit, 0, (Vec3*)node->Position());
                    else if (node->kind.index == patrol.index)
                        kind = GetOrderType(9, unit, 0, (Vec3*)node->Position());
                    if (kind.index) {
                        AddOrder(kind, 1, unit, 0, node->Position(), 0, 0);
                        queued = 1;
                    }
                }
                if (unit->Ready() && order->Target()->Ready()) {
                    unit->moveOrder = order->Target()->moveOrder;
                    unit->fireOrder = order->Target()->fireOrder;
                    if (unit->player->active && unit->player->type == 1)
                        unit->group = order->Target()->group;
                }
            }
            if (!queued)
                AddOrder("PARK", 1, unit, 0, 0, 0, 0);
        }
        return 5;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        order->SetDeadlineTicks(300);
        order->Wait();
        return 1;
    case 1:
        order->SetDeadlineTicks(30);
        order->Wait();
        return 1;
    case 2:
        if (flags & 0x8000) {
            order->SetDeadlineTicks(30);
        } else if (flags & 1) {
            order->SetDeadlineTicks(11);
            g_seams.units->ApplyUnfinishedBuildDecay(unit, 11);
        }
        order->Wait();
        return 2;
    default:
        return 7;
    }
}

// FUNCTION: 0x402fc0
int __stdcall BeCarriedOrder(Unit* unit, Order* order, int unused)
{
    if (unit->carrier == 0) {
        return 5;
    }
    switch (order->state) {
    case 0:
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        return 1;
    case 1:
        order->SetDeadlineTicks(10);
        return 2;
    default:
        return 7;
    }
}

// FUNCTION: 0x403010
int __stdcall ActivateOrder(Unit* unit, int unused1, int unused2)
{
    if (((UnitDefActivateFlagView*)unit->def.get())->flag) {
        g_seams.units->SetStateBits(unit, 1, 1);
    }
    return 5;
}

// FUNCTION: 0x403040
int __stdcall DeactivateOrder(Unit* unit, int unused1, int unused2)
{
    if (((UnitDefActivateFlagView*)unit->def.get())->flag) {
        g_seams.units->SetStateBits(unit, 1, 0);
    }
    return 5;
}

// FUNCTION: 0x403070
int __stdcall CloakOnOrder(char* param1, int unused1, int unused2)
{
    CloakDefView* p = (CloakDefView*)((Unit*)param1)->def.get();
    if (p->flag) {
        *(unsigned int*)(param1 + 0x110) |= 0x800;
    }
    return 5;
}

// FUNCTION: 0x4030a0
int __stdcall CloakOffOrder(char* unit, int unused1, int unused2)
{
    CloakDefView* p = (CloakDefView*)((Unit*)unit)->def.get();
    if (p->flag) {
        *(unsigned int*)(unit + 0x110) &= ~0x800;
    }
    return 5;
}

// FUNCTION: 0x4030d0
int __stdcall StandingMoveOrder(Unit* unit, Order* order, int unused)
{
    unit->moveOrder = order->mode;
    return 5;
}

// FUNCTION: 0x403100
int __stdcall StandingFireOrder(Unit* unit, Order* order, int unused)
{
    unit->fireOrder = order->mode;
    if (order->mode == 0 || order->mode == 1) {
        for (char i = 0; i < 3; i++) {
            if (unit->weapons[i].flags & 0x10) {
                g_seams.targeting->ClearWeaponTarget(unit, i);
            }
        }
    }
    return 5;
}

// FUNCTION: 0x403160
int __stdcall QMoveQPatrolOrder(int unused1, void* order, int unused3)
{
    ((Order*)order)->SetDeadlineTicks(0x3c);
    return 6;
}

struct Unit;
struct Order;
struct View;
struct Pos;

#pragma pack(push, 1)
struct UnitOrderType {
    const char* status;
    int (__stdcall* run)(Unit* unit, Order* order, int flags);
    void (__stdcall* draw)(void* surface, View* view, Order* order, Pos* out, int flag);
    int target;
    unsigned int flags;
    unsigned char internal;
    const char* name;
};
#pragma pack(pop)

// FUNCTION: 0x403180
void RegisterUnitOrders()
{
#ifdef __clang__
    extern const UnitOrderType g_unitOrders[23];
    RegisterOrderTypes((void*)g_unitOrders, 0x17);
#else
    extern char g_unitOrders[];
    RegisterOrderTypes(g_unitOrders, 0x17);
#endif
}

// FUNCTION: 0x403190
int __stdcall AttackSpecialOrder(Unit* unit, Order* order, int unused)
{
    order->MergeFlagsFromTable(GetOrderType(3, unit, (Unit*)order->target, 0).index);
    order->weapon = 2;
    return 2;
}

// FUNCTION: 0x4031d0
int __stdcall MoveGroundOrder(Unit* unit, Order* order, int flags)
{
    switch (order->state) {
    case 0:
        if (unit->carrier != 0)
            return 7;
        order->AnnounceStatusIfFlagged(SIM_SPEECH_KIND_TEXT);
        order->AttachApproachRadiusGoal(&order->pos, order->approachRadius + 4);
        order->flags = 0xe0;
        return 1;
    case 1:
        if (flags & 0x20) {
            RecordUnitSpeech(unit, 6, SIM_SPEECH_KIND_TEXT);
            return 5;
        }
        return 9;
    default:
        return 7;
    }
}

// FUNCTION: 0x403260
int __stdcall AttackKamikazeOrder(Unit* unit, Order* order, unsigned flags)
{
    if(flags&0x10008) return 5;
    if(order->target) order->pos=order->target->pos;
    unsigned state=0; state=order->state;
    switch(state) {
    case 0:
        if(unit->carrier) return 7;
        order->AnnounceStatusIfFlagged(SIM_SPEECH_KIND_TEXT);
        order->AttachApproachRadiusGoal(&order->pos,(unsigned short)unit->def->kamikazedistance<16?16:(unsigned short)unit->def->kamikazedistance);
        order->SetDeadlineTicks(60);
        order->flags|=0xe0; return 1;
    case 1:
        if(flags&0x20) {
            RecordUnitSpeech(unit,6,SIM_SPEECH_KIND_TEXT);
            AppendOrder(unit,new (kSim) Order("SELFDESTRUCT",0,0,1,0,0));
            return 5;
        }
        if(flags&0x40) return 8;
        order->state=0; return 2;
    default: return 7;
    }
}

// FUNCTION: 0x4033a0
int __stdcall PatrolOrder(Unit* unit, Order* order, int flags)
{
    unsigned state=0; state=order->state;
    switch(state) {
    case 0:
        if(!unit->motion) return 7;
        order->AnnounceStatusIfFlagged(SIM_SPEECH_KIND_TEXT);
        EnsurePatrolReturnOrder(unit,order);
        order->SetDeadlineTicks(1); return 1;
    case 1:
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->AttachApproachRadiusGoal(&order->pos,0);
        order->SetDeadlineTicks(15);
        order->flags|=0xe0; return 1;
    case 2:
        if(flags&0xe0) { order->state=1; return 6; }
        {
            Order* next=(Order*)g_seams.targeting->FindBestTargetIfFireAtWill(unit);
            if(next && IssueAttackOrder(unit,(Unit *)next,0)) { order->flags=0; order->state=1; return 3; }
        }
        order->SetDeadlineTicks(RandomInt(30)+30);
        order->state=1; return 4;
    default: return 7;
    }
}

struct PadWord0 { int field; };
struct PadWord1 { int field; };
struct PadWord2 { int field; };
struct PadWord3 { int field; };
struct PadWord4 { int field; };
struct PadWord5 { int field; };
struct PadWord6 { int field; };
struct PadWord7 { int field; };


#include "../map/feature_spot.h"



#include "../map/features.h"

#include "../map/terrain.h"

static const float kZeroFloatOrders = 0.0f;


#include "../util/math.h"

// FUNCTION: 0x4034a0
int __stdcall AttackChaseOrder(Unit* unit, Order* order, unsigned int flags)
{
    int weapon = order->weapon;
    if ((flags & 0x800) || !order->target || (flags & 0x10008)) return 5;
    if (order->range && (int)_hypot(Whole(unit->pos.x) - order->start.x,
                                    Whole(unit->pos.z) - order->start.y) >= order->range)
        return 5;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (!unit->motion || unit->def->flying || !(unit->flags & 0x80000000)) break;
        order->AnnounceStatusIfFlagged(SIM_SPEECH_KIND_TEXT);
        order->pos = unit->pos;
        order->step = 0;
        if (!weapon) order->weapon = g_seams.targeting->ChooseWeapon(unit);
        return 1;
    case 1:
        order->SetAttachedFx(0);
        if (flags & 0x3000) return 1;
        if (!g_seams.weapons->WeaponCanReachUnit(unit, order->target, weapon)) return 1;
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 2);
        g_seams.targeting->SetWeaponTargetUnit(unit, order->target, weapon);
        order->flags = 0x13808;
        return 2;
    case 2:
        weapon = g_seams.weapons->GetWeaponRange(unit, weapon);
        switch (order->step) {
        case 0:
            return order->Advance(weapon);
        case 1: case 2: case 3: case 4:
            if (abs(unit->pos.y - order->target->pos.y) > 0x80000) {
                order->AttachApproachRadiusGoal(&order->target->pos, weapon / 2);
                order->step = 6;
                return 1;
            } else {
                int angle = GetHeadingBetween(&order->target->pos, &unit->pos);
                angle += RandomInt(0x8000) - 0x4000;
                int distance = weapon << 16;
                int dx = -ScaledSin(angle, distance);
                int dz = -ScaledCos(angle, distance);
                Vec3* target = &order->target->pos;
                Vec3 offset = { dx, 0, dz };
                Vec3 pos = Vec3Add(*target, offset);
                order->AttachApproachRadiusGoal(&pos, weapon / 4);
                return 1;
            }
        case 5:
            return order->Advance(weapon / 2);
        case 6:
            order->AttachApproachRadiusGoal(&order->target->pos, 0);
            break;
        case 7:
            order->AttachRingApproachGoal(&order->target->pos, weapon, weapon / 2);
            break;
        case 8:
            order->AttachRingApproachGoal(&order->target->pos, weapon * 2, weapon);
            order->step = 0;
            return 1;
        default: return 7;
        }
        order->step++;
        return 1;
    case 3:
        if (flags & 0x40e0) { order->state = 1; return 4; }
        if (g_seams.weapons->WeaponCanReachUnit(unit, order->target, weapon)) {
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 2);
            g_seams.targeting->SetWeaponTargetUnit(unit, order->target, weapon);
            order->flags = 0x148e8;
            order->SetDeadlineTicks(30);
            return 2;
        }
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->flags = 0x100e8;
        order->SetDeadlineTicks(30);
        return 2;
    }
    return 7;
}

// FUNCTION: 0x4038a0
int __stdcall SuppressOrder(Unit* unit,Order* order,unsigned flags)
{
    if(flags&0x800) return 5;
    unsigned state=0; state=order->state;
    switch(state) {
    case 0:
        if(unit->def->flying) return 8;
        order->AnnounceStatusIfFlagged(SIM_SPEECH_KIND_TEXT);
        order->radius=g_seams.weapons->GetWeaponRange(unit,(unsigned char)order->weapon); return 1;
    case 1:
        if(order->weapon==2) {
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            g_seams.targeting->SetWeaponTargetPos(unit,&order->pos,2);
            order->flags=0x1c00; return 1;
        }
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 1);
        g_seams.targeting->SetWeaponTargetPos(unit,&order->pos,0);
        g_seams.targeting->SetWeaponTargetPos(unit,&order->pos,1);
        order->flags=0x1c00; return 1;
    case 2:
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        if(flags&0x400) { order->state=1; return 6; }
        if(unit->motion) {
            if(order->radius<=0) return 9;
            order->AttachApproachRadiusGoal(&order->pos,order->radius);
            order->flags=0xe0;
            order->radius-=RandomInt(g_seams.weapons->GetWeaponRange(unit,(unsigned char)order->weapon)/3);
            order->state=1; return 4;
        }
        return 9;
    default: return 7;
    }
}

static inline UnitDef* Definitions() { return g_game->defs; }

static inline Point16 WorldToCell(Vec3 v, Point16 footprint)
{
    Point16 c;
    c.x = (v.x - (footprint.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (footprint.y << 19) + 0x80000) >> 20;
    return c;
}

static inline void CellToWorld(Point16 footprint, Point16 c, Vec3* v)
{
    v->x = (footprint.x + c.x * 2) << 19;
    v->z = (footprint.y + c.y * 2) << 19;
}

int __stdcall ClassifyEighths(double x)
{
    if (x == 8.0) return 1;
    if (x == -8.0) return 2;
    if (x == 16.0) return 3;
    if (x == 0.0) return 4;
    return 5;
}

// FUNCTION: 0x404270
int __stdcall CaptureOrder(Unit* unit, Order* order, unsigned int flags)
{
    Unit* target = order->link.Get();
    if (!target || (flags & 0x10008)) {
        RecordUnitSpeech(unit, 7, SIM_SPEECH_CAPTURE_FAILED);
        return 8;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0: {
        if (!unit->motion) return 7;
        if (!(unit->def->flags2 & 0x1000)) return 7;
        if (target->def->capture) {
            RecordUnitSpeech(unit, 7, SIM_SPEECH_THAT_UNIT_CANNOT_BE_CAPTURED);
            return 8;
        }
        if (target->buildLeft != kZeroFloatOrders) {
            RecordUnitSpeech(unit, 7, SIM_SPEECH_THAT_UNIT_IS_A_CLOUD_OF_VAPOR_AND_CANNOT_BE_CAPTURED);
            return 8;
        }
        order->AnnounceStatusIfFlagged(SIM_SPEECH_CAPTURING);
        order->duration = (int)(order->link.Get()->def->EnergyCost() / 2000 + order->link.Get()->def->MetalCost() / 140 + 150);
        order->duration = order->duration < 1800 ? order->duration : 1800;
        order->duration = (order->link.Get()->health + order->link.Get()->def->MaxHealth()) * order->duration / (order->link.Get()->def->maxHealth * 2);
        int experience = 0;
        experience = order->link.Get()->killCount;
        order->duration = ((experience / 5 + 10) * order->duration * 10) / 100;
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        order->AttachBuildFootprintMarker(order->link.Get()->cell, order->link.Get()->footprint);
        order->flags = 0x100e8;
        return 1;
    }
    case 1: {
        if (flags & 0x40) return 8;
        Vec3* position = &unit->pos;
        Fixed distance;
        distance.value = (int)_hypot(unit->pos.x - target->pos.x, unit->pos.z - target->pos.z);
        int gap = distance.whole - (int)(_hypot(unit->footprint.x, unit->footprint.y) * 8.0);
        gap += (int)(_hypot(order->link.Get()->footprint.x, order->link.Get()->footprint.y) * -8.0);
        unsigned int range = 0;
        range = unit->def->buildRange;
        if (gap > (int)range) return 0;
        StartBuildingScript(unit, order, GetHeadingBetween(position, &order->link.Get()->pos) - unit->angles.heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 0x10008);
    case 3:
        RecordUnitSpeech(unit, 11, SIM_SPEECH_KIND_TEXT);
        return 1;
    case 4: {
        if (target->motion && (target->flags & 0xc)) {
            StopBuildingScript(unit, order);
            order->SetDeadlineTicks(30);
            return 0;
        }
        if (order->elapsed >= order->duration) return 1;
        Vec3 start;
        g_seams.units->GetNanoPiecePosition(order->source, &start);
        Vec3 bounds[2];
        bounds[1] = order->link.Get()->pos;
        bounds[0] = order->link.Get()->pos;
        bounds[0].x += order->link.Get()->def->modelMinX;
        bounds[0].z += order->link.Get()->def->modelMinZ;
        bounds[1].x += order->link.Get()->def->modelMaxX;
        bounds[1].z += order->link.Get()->def->modelMaxZ;
        bounds[1].y += order->link.Get()->def->modelMaxY;
        RecordReverseNanoParticles(bounds, start, 6);
        unit->workTime = g_game->ticks + 900;
        order->elapsed += 2;
        order->SetDeadlineTicks(2);
        return 2;
    }
    case 5:
        g_seams.players->GiveUnitToPlayer(target, unit->player, 0);
        RecordUnitSpeech(unit, 16, SIM_SPEECH_KIND_TEXT);
        return 5;
    }
    return 7;
}

static inline int SquaredDistance(int dx, int dz)
{
    __int64 x = dx;
    __int64 z = dz;
    return (int)((x * x) >> 32) + (int)((z * z) >> 32);
}

float HalfNegated(float x)
{
    return x * -0.5f;
}


// FUNCTION: 0x404730
int __stdcall ReclaimUnitOrder(Unit* unit, Order* order, unsigned int flags)
{
    Unit* target = order->link.Get();
    if (!target || (flags & 0x10008)) return 5;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags2 & 0x400)) {
            if (!g_seams.units->CanReclaim(unit, 0, target)) {
                RecordUnitSpeech(unit, 7, SIM_SPEECH_THAT_UNIT_CANNOT_BE_RECLAIMED);
                RecordUnitSpeech(unit, 7, SIM_SPEECH_RECLAMATION_FAILED);
                return 8;
            }
            order->AnnounceStatusIfFlagged(SIM_SPEECH_RECLAIMING);
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            return 1;
        }
        RecordUnitSpeech(unit, 7, SIM_SPEECH_RECLAMATION_FAILED);
        return 7;
    case 1:
        if (flags & 0x20) return 1;
        order->AttachBuildFootprintMarker(target->cell, target->footprint);
        order->flags |= 0x100e8;
        order->SetDeadlineTicks(15);
        order->elapsed = ComputeReclaimDamagePulse(unit, order->link.Get(), 15);
        order->duration = 0;
        return 2;
    case 2:
        if (flags & 0x40) return 9;
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &target->pos) - unit->angles.heading);
        return 1;
    case 3:
        return WaitIfNotInBuildStance(unit, order, 0x10008);
    case 4:
        RecordUnitSpeech(unit, 11, SIM_SPEECH_KIND_TEXT);
        return 1;
    case 5: {
        Vec3 delta = Vec3Sub(unit->pos, target->pos);
        int range = 0;
        range = unit->def->buildRange;
        range += target->def->radius184;
        int square = SquaredDistance(delta.x, delta.z);
        if (square <= range * range && g_seams.units->CanReclaim(unit, 0, order->link.Get())) {
            if (order->duration >= 15) {
                g_seams.weapons->DamageUnit(unit, order->link.Get(), order->elapsed, 5, 0);
                order->duration = 0;
            }
            unit->workTime = g_game->ticks + 900;
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(order->source, &start);
            Vec3 bounds[2];
            bounds[1] = order->link.Get()->pos;
            bounds[0] = order->link.Get()->pos;
            bounds[0].x += order->link.Get()->def->modelMinX;
            bounds[0].z += order->link.Get()->def->modelMinZ;
            bounds[1].x += order->link.Get()->def->modelMaxX;
            bounds[1].z += order->link.Get()->def->modelMaxZ;
            bounds[1].y += order->link.Get()->def->modelMaxY;
            RecordReverseNanoParticles(bounds, start, 6);
            order->SetDeadlineTicks(2);
            order->duration += 2;
            return 2;
        }
        order->SetDeadlineTicks(15);
        StopBuildingScript(unit, order);
        return 0;
    }
    }
    return 7;
}

// FUNCTION: 0x405300
int __stdcall RepairUnitOrder(Unit* unit, Order* order, int flags)
{
    if (!order->target) {
        RecordUnitSpeech(unit, 7, SIM_SPEECH_REPAIRS_UNSUCCESSFUL);
        return 5;
    }
    if (order->range && (int)_hypot(Whole(unit->pos.x) - order->start.x, Whole(unit->pos.z) - order->start.y) >= order->range)
        return 5;
    if (order->target->mode != 1) {
        RecordUnitSpeech(unit, 7, SIM_SPEECH_REPAIRS_UNSUCCESSFUL);
        return 5;
    }
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1Byte & 0x40) && order->target->buildLeft == kZeroFloatOrders) {
            order->AnnounceStatusIfFlagged(SIM_SPEECH_REPAIRING);
            return 1;
        }
        break;
    case 1: {
        if (flags & 0x40)
            return 8;
        Fixed distance;
        distance.value = (int)_hypot(unit->pos.x - order->target->pos.x, unit->pos.z - order->target->pos.z);
        int gap = distance.whole - (int)(_hypot(unit->footprint.x, unit->footprint.y) * 8.0);
        gap += (int)(_hypot(order->target->footprint.x, order->target->footprint.y) * -8.0);
        unsigned int range = 0;
        range = unit->def->buildRange;
        if (gap > (int)range) {
            order->AttachBuildFootprintMarker(order->target->cell, order->target->footprint);
            order->SetDeadlineTicks(RandomInt(30) + 30);
            order->flags |= 0xe8;
            return 2;
        }
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &order->target->pos) - unit->angles.heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 8);
    case 3:
        if ((unsigned int)order->target->health >= order->target->def->maxHealth)
            return 1;
        if (order->target->flags & 0xc) {
            StopBuildingScript(unit, order);
            order->SetDeadlineTicks(15);
            return 0;
        }
        unit->workTime = g_game->ticks + 150;
        if (g_seams.units->AddRepairProgress(unit, order->target, (float)(unit->def->buildRate / 30))) {
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(order->source, &nano);
            Box box;
            box.hi = order->target->pos;
            box.lo = order->target->pos;
            box.lo.x += order->target->def->modelMinX;
            box.lo.z += order->target->def->modelMinZ;
            box.hi.x += order->target->def->modelMaxX;
            box.hi.z += order->target->def->modelMaxZ;
            box.hi.y += order->target->def->modelMaxY;
            RecordNanoParticles(nano, box, 6);
        }
        order->SetDeadlineTicks(1);
        order->flags |= 8;
        return 2;
    case 4:
        RecordUnitSpeech(unit, 10, SIM_SPEECH_UNIT_REPAIRED);
        return 5;
    }
    return 7;
}

// FUNCTION: 0x405740
int __stdcall RepairUnitNoMoveOrder(Unit* unit, Order* order, int unused)
{
    Unit* target = order->link.Get();
    if (!target) {
        RecordUnitSpeech(unit, 7, SIM_SPEECH_REPAIRS_UNSUCCESSFUL);
        return 5;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (!(unit->def->flags1Byte & 0x40)) return 7;
        if (target->buildLeft == kZeroFloatOrders && (unit->activateFlags & 1)) {
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            return 1;
        }
        return 8;
    case 1: {
        if ((unsigned int)target->health >= target->def->maxHealth) return 1;
        if (target->flags & 0xc) return 1;
        unit->workTime = g_game->ticks + 150;
        int rate = 0;
        rate = unit->def->buildRate;
        if (g_seams.units->AddRepairProgress(unit, order->link.Get(), (float)(rate / 30))) {
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(order->source, &start);
            Vec3 bounds[2];
            bounds[1] = order->link.Get()->pos;
            bounds[0] = order->link.Get()->pos;
            bounds[0].x += order->link.Get()->def->modelMinX;
            bounds[0].z += order->link.Get()->def->modelMinZ;
            bounds[1].x += order->link.Get()->def->modelMaxX;
            bounds[1].z += order->link.Get()->def->modelMaxZ;
            bounds[1].y += order->link.Get()->def->modelMaxY;
            RecordNanoParticles(start, bounds, 6);
        }
        order->SetDeadlineTicks(1);
        order->flags |= 8;
        return 2;
    }
    case 2:
        RecordUnitSpeech(unit, 10, SIM_SPEECH_UNIT_REPAIRED);
        return 5;
    }
    return 7;
}

// FUNCTION: 0x405d90
void DamagedAllyCollector::CollectDamagedAlly(Unit* unit)
{
    if (unit == self) return;
    unsigned int index = 0;
    index = unit->player->index;
    if (!owner->allied[index]) return;
    unsigned int kind = unit->flags & 3;
    if ((unsigned char)kind != 1) return;
    if ((unsigned int)unit->health >= unit->def->maxHealth && unit->buildLeft == kZeroFloatOrders) return;
    if (unit->lastAttackerSlot == owner->index && unit->lastDamageType == 5) return;
    units->push_back(unit);
}

// FUNCTION: 0x405fe0
int __stdcall StandbyOrder(Unit* unit,Order* order,int flags)
{
    unsigned state=0; state=order->state;
    switch(state) {
    case 0:
        if(!unit->motion) return 7;
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->flags|=0x10000;
        order->SetDeadlineTicks(1); return 1;
    case 1:
        {
            Unit* next=g_seams.targeting->FindBestTargetIfFireAtWill(unit);
            if(next && IssueAttackOrder(unit,(Unit *)(Order*)next,0)) return 5;
        }
        order->flags|=0x10000;
        order->SetDeadlineTicks(RandomInt(30)+30); return 2;
    default: return 7;
    }
}

// FUNCTION: 0x406090
int __stdcall StandbyMineOrder(Unit* unit, Order* order, int unused)
{
    switch (order->state) {
    case 0:
        if (!(unit->flags & 0x20000000)) return 7;
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->flags |= 0x10000;
        order->SetDeadlineTicks(1);
        return 1;
    case 1:
        {
            Unit* other = g_seams.targeting->FindBestTargetIfFireAtWill(unit);
            if (other && other->mode == 1 && (unit->flags & 0x300000)) {
                AppendOrder(unit, new (kSim) Order("SELFDESTRUCT", 0, 0, 1, 0, 0));
                return 5;
            }
            order->flags |= 0x10000;
            order->SetDeadlineTicks(RandomInt(30) + 30);
            return 2;
        }
    default: return 7;
    }
}

static inline Point16 MakePoint(int x,int y) { Point16 p; p.x=x; p.y=y; return p; }
static inline Point16 SubPoint(Point16 a,Point16 b) { Point16 p; p.x=a.x-b.x; p.y=a.y-b.y; return p; }
static inline Point16 Convert(Vec3 v) { Point16 p; p.x=(unsigned)v.x>>20; p.y=(unsigned)v.z>>20; return p; }

// FUNCTION: 0x4061a0
int __stdcall ParkOrder(Unit* unit,Order* order,int flags)
{
    switch(order->state) {
    case 0:
        if(!unit->motion) return 7;
        if(unit->def->flying) {
            order->pos=unit->pos;
            MissionType mission("VTOL_MOVE"); order->MergeFlagsFromTable(mission.index);
            return 0;
        }
        {
            int size=unit->footprint.x;
            if(unit->def->minwaterdepth>=0) size+=3;
            Point16 start=SubPoint(Convert(unit->pos),MakePoint(size*4,size*3));
            Point16 extent=MakePoint(size*8,size*6);
            order->AttachBuildFootprintMarker(start,extent);
            order->flags=0xe0;
            return 1;
        }
    case 1:
        if(flags&0x20) return 5;
        if(order->next) return 5;
        order->SetDeadlineTicks(30); return 0;
    default: return 7;
    }
}

static inline int Contains(unsigned int* bits, unsigned short index) { return bits[index >> 5] & (1 << (index & 31)); }

static inline Vec3 Offset(short angle, int distance) { Vec3 v; v.x=-ScaledSin(angle,distance); v.y=0; v.z=-ScaledCos(angle,distance); return v; }

// FUNCTION: 0x406780
int __stdcall GroundPickupOrder(Unit* unit, Order* order, unsigned char flags)
{
    Unit* target = order->target;
    if (target && !(flags & 8)) {
        switch(order->state) {
        case 0:
            if (!unit->motion) break;
            if (!(unit->def->flags2 & 0x100)) break;
            if (target->footprint.x > (short)unit->def->capacity) {
                RecordUnitSpeech(unit, 7, SIM_SPEECH_UNIT_IS_TOO_LARGE_TO_TRANSPORT); return 8;
            }
            order->AnnounceStatusIfFlagged(SIM_SPEECH_LOADING_UNIT); return 1;
        case 1: return WaitIfCobBusy((char *)unit, (char *)order, 8);
        case 2:
            {
            int id = target->id;
            g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), "TransportPickup", 0, 1, 1, id, 0, 0, 0);
            RecordUnitSpeech(unit, 12, SIM_SPEECH_KIND_TEXT);
            ++order->attempts;
            order->SetDeadlineTicks(15); return 1;
            }
        case 3: return WaitIfCobBusy((char *)unit, (char *)order, 8);
        case 4:
            if (target->carrier) return 5;
            if (order->attempts >= 3) return 9;
            order->AttachApproachRadiusGoal(&target->pos, 0);
            order->flags = 0xe8; return 1;
        case 5: order->SetAttachedFx(0); return 0;
        default: break;
        }
        return 7;
    }
    RecordUnitSpeech(unit, 7, SIM_SPEECH_TRANSPORT_MISSION_FAILED); return 8;
}

// FUNCTION: 0x406900
int __stdcall GroundUnloadOrder(Unit* unit, Order* order, int flags)
{
    if (flags&8) {
        RecordUnitSpeech(unit,7,SIM_SPEECH_UNLOADING_PROCESS_IS_PROCEEDING_NON_OPTIMALLY);
        return 8;
    }
    switch(order->state) {
    case 0:
        if (!unit->motion) break;
        if (!(unit->def->flags2&0x100)) break;
        order->link.SetUnit(unit->cargo);
        if (!order->target) return 5;
        order->AnnounceStatusIfFlagged(SIM_SPEECH_UNLOADING);
        g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), "TransportDrop",0,1,1,order->target->id,
            (order->pos.x&0xffff0000)+(order->pos.z>>16),0,0);
        ++order->attempts;
        order->SetDeadlineTicks(15);
        return 1;
    case 1: return WaitIfCobBusy((char *)unit,(char *)order,8);
    case 2:
        if (order->target->carrier!=unit) { RecordUnitSpeech(unit,13,SIM_SPEECH_KIND_TEXT); return 5; }
        if (order->attempts>=3) return 9;
        if ((unsigned char)(unit->def->flags1>>12)&1)
            order->AttachApproachRadiusGoal(&order->pos,(int)(unit->def->height180*1.5));
        else order->AttachApproachRadiusGoal(&order->pos,0);
        order->flags=0xe8;
        return 1;
    case 3: return 0;
    }
    return 7;
}

static inline Vec3 Add(const Vec3& a,const Vec3& b) { Vec3 r; r.x=a.x+b.x; r.y=a.y+b.y; r.z=a.z+b.z; return r; }
static inline Vec3 Sub(const Vec3& a,const Vec3& b) { Vec3 r; r.x=a.x-b.x; r.y=a.y-b.y; r.z=a.z-b.z; return r; }


// FUNCTION: 0x406bf0
void RegisterGroundOrders()
{
#ifdef __clang__
    extern const UnitOrderType g_groundOrders[22];
    RegisterOrderTypes((void*)g_groundOrders, 0x16);
#else
    extern char g_groundOrders[];
    RegisterOrderTypes(g_groundOrders, 0x16);
#endif
}

// FUNCTION: 0x401e00
int __stdcall AttackUTypeOrder(Unit* unit, Order* order, int unused)
{
    unsigned int s = 0;
    s = order->state;
    switch (s) {
    case 0:
        if (!(unit->def->flags2Byte & 0x10))
            return 7;
        order->SetDeadlineTicks(RandomInt(0x5a) + 1);
        return 1;
    case 1: {
        Unit* best = 0;
        int bestDist = 0x7fffffff;
        for (Unit* u = g_game->units + 1; u <= g_game->unitsEnd; u++) {
            if (u->unitDefIndex == order->id && unit->player->allied[u->player->index] == 0) {
                int dz = u->pos.z - unit->pos.z;
                int dx = u->pos.x - unit->pos.x;
                int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
                d -= RandomInt(d / 2);
                if (d <= bestDist) {
                    best = u;
                    bestDist = d;
                }
            }
        }
        if (best) {
            MissionType kind = GetOrderType(3, unit, best, 0);
            AppendOrder(unit, new (kSim) Order(kind, best, 0, 0, 0, 0));
            return 0;
        }
        return 5;
    }
    default:
        return 7;
    }
}

struct Projectile;

struct Vec3Sum : Vec3 {
    Vec3 operator+(const Vec3& other) const {
        Vec3 r;
        r.x = x + other.x;
        r.y = y + other.y;
        r.z = z + other.z;
        return r;
    }
};

// FUNCTION: 0x403f70
int __stdcall HelpBuildOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 2) { RecordSelectionOrdersDirty(unit); return 5; }
    Unit* target = order->target;
    if (!target) {
        RecordUnitSpeech(unit, 7, SIM_SPEECH_CONSTRUCTION_TERMINATED);
        return 8;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0: {
        if (!unit->motion || !(unit->def->flags1Byte & 0x40)) break;
        double x = target->def->footprintx;
        double y = target->def->footprintz;
        int radius = (int)(sqrt(x * x + y + y) * 16.0) / 2;
        unsigned int range = 0;
        range = unit->def->buildRange;
        order->AttachRingApproachGoal(&target->pos, range + radius, radius);
        order->flags = 0xe8;
        return 1;
    }
    case 1:
        if (flags & 0x40) {
            RecordUnitSpeech(unit, 7, SIM_SPEECH_I_CANT_GET_THERE);
            return 8;
        }
        if (target->buildLeft == 0.0f) return 5;
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &order->target->pos) - unit->angles.heading);
        RecordSelectionOrdersDirty(unit);
        return 1;
    case 2:
        return WaitIfNotInBuildStance(unit, order, 10);
    case 3: {
        int rate = 0;
        rate = unit->def->buildRate;
        if (g_seams.units->AddBuildProgress(unit, target, (float)(rate / 30))) {
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(unit, &start);
            Vec3 bounds[2];
            bounds[0] = (Vec3Sum&)order->target->pos + *(Vec3*)&order->target->def->modelMinX;
            bounds[1] = (Vec3Sum&)order->target->pos + *(Vec3*)&order->target->def->modelMaxX;
            RecordNanoParticles(start, bounds, 6);
        }
        unit->workTime = g_game->ticks + 300;
        if (order->target->buildLeft != 0.0f) {
            order->SetDeadlineTicks(1);
            order->flags |= 0xa;
            return 2;
        }
        return 1;
    }
    case 4:
        RecordUnitSpeech(unit, 8, SIM_SPEECH_BUILDING_COMPLETE);
        order->flags |= 2;
        return 5;
    }
    return 7;
}


// GLOBAL: 0x4fc490
extern const UnitOrderType g_unitOrders[23] = {
    {"Stopping", StopOrder, 0, 0, 0x13, 0, "Stop"},
    {"Attacking", AttackNoMoveOrder, DrawWeaponCoverage, 8, 0x28001, 0, "Attack_NoMove"},
    {"Activate", (int (__stdcall *)(Unit *, Order *, int))ActivateOrder, 0, 0, 0x1006013, 0, "Activate"},
    {"Deactivate", (int (__stdcall *)(Unit *, Order *, int))DeactivateOrder, 0, 0, 0x1006013, 0, "Deactivate"},
    {"Cloaking", (int (__stdcall *)(Unit *, Order *, int))CloakOnOrder, 0, 0, 0x1006013, 0, "Cloak_On"},
    {"Decloaking", (int (__stdcall *)(Unit *, Order *, int))CloakOffOrder, 0, 0, 0x1006013, 0, "Cloak_Off"},
    {"Acknowledged", StandingMoveOrder, 0, 0, 0x1006013, 0, "Standing_MoveOrder"},
    {"Acknowledged", StandingFireOrder, 0, 0, 0x1006013, 0, "Standing_FireOrder"},
    {"Nanolathing", BuildingBuildOrder, 0, 0, 0x10010c13, 0, "BuildingBuild"},
    {"Nanolathing", BuildWeaponOrder, 0, 0, 0xc014013, 0, "BuildWeapon"},
    {"SELF DESTRUCT ENGAGED", SelfDestructOrder, 0, 0, 0x4004013, 0, "SelfDestruct"},
    {"SELF DESTRUCT ENGAGED", SelfDestructOrder, 0, 0, 0x13, 0, "SelfDestructFG"},
    {"Paralyzed", ParalyzeOrder, 0, 0, 0x2413, 0, "Paralyze"},
    {"Under construction", (int (__stdcall *)(Unit *, Order *, int))GetBuiltOrder, 0, 0, 0x22413, 0, "GetBuilt"},
    {"Being transported", BeCarriedOrder, 0, 0, 0x2413, 0, "BeCarried"},
    {"Unit is available", MakeSelectableOrder, 0, 0, 0x413, 0, "MakeSelectable"},
    {"Waiting", WaitOrder, 0, 0, 0x413, 0, "Wait"},
    {"Waiting for attack", (int (__stdcall *)(Unit *, Order *, int))WaitForAttackOrder, 0, 0, 0x20413, 0, "WaitForAttack"},
    {"Attacking", AttackUTypeOrder, 0, 0, 0x413, 0, "AttackUType"},
    {"Ready", GuardNoMoveOrder, 0, 0, 0x2013, 0, "Guard_NoMove"},
    {"Repairing", SelfRepairOrder, 0, 0, 0x20413, 1, "SelfRepair"},
    {"Ready with orders", (int (__stdcall *)(Unit *, Order *, int))QMoveQPatrolOrder, DrawPathAnim, 2, 0x4000e, 0, "QMove"},
    {"Ready with orders", (int (__stdcall *)(Unit *, Order *, int))QMoveQPatrolOrder, DrawPathAnim, 2, 0x40007, 0, "QPatrol"},
};

// GLOBAL: 0x4fc6e8
extern const UnitOrderType g_groundOrders[22] = {
    {"Standby", StandbyOrder, 0, 0x10, 0x200000f, 0, "Standby"},
    {"Standby", StandbyMineOrder, 0, 0x10, 0x200000f, 1, "Standby_Mine"},
    {"Moving", MoveGroundOrder, DrawPathAnim, 0x12, 0x4020e, 0, "Move_Ground"},
    {"Guarding", FollowGroundOrder, DrawPathAnim, 0x12, 0x20005, 0, "Follow_Ground"},
    {"Suppressing fire", (int (__stdcall *)(Unit *, Order *, int))SuppressOrder, DrawWeaponCoverage, 8, 0x41001, 0, "Suppress"},
    {"Attacking", (int (__stdcall *)(Unit *, Order *, int))AttackChaseOrder, DrawWeaponCoverage, 8, 0x28001, 0, "Attack_Chase"},
    {"Attacking", (int (__stdcall *)(Unit *, Order *, int))AttackKamikazeOrder, DrawWeaponCoverage, 8, 0x60001, 0, "Attack_Kamikaze"},
    {"Annihilating", AttackSpecialOrder, DrawWeaponCoverage, 8, 0x68001, 0, "AttackSpecial"},
    {"Parking", ParkOrder, 0, 0, 0xe, 0, "Park"},
    {"Patrolling", PatrolOrder, DrawPathAnim, 0x12, 0x41207, 0, "Patrol"},
    {"Loading", GroundPickupOrder, DrawWeaponCoverage, 8, 0x2000c, 0, "Ground_Pickup"},
    {"Unloading", GroundUnloadOrder, DrawWeaponCoverage, 8, 0x4000d, 0, "Ground_Unload"},
    {"Teleporting", TeleportOrder, DrawWeaponCoverage, 8, 0x60009, 0, "Teleport"},
    {"Nanolathing", MobileBuildOrder, DrawBuildFootprint, 0x13, 0x10050800, 0, "MobileBuild"},
    {"Nanolathing", HelpBuildOrder, DrawWeaponCoverage, 0x18, 0x10020806, 0, "HelpBuild"},
    {"Repair patrol", RepairPatrolOrder, DrawPathAnim, 0x12, 0x41207, 0, "RepairPatrol"},
    {"Repairing", RepairUnitOrder, DrawPathAnim, 0x12, 0x10020006, 0, "RepairUnit"},
    {"Capturing", (int (__stdcall *)(Unit *, Order *, int))CaptureOrder, DrawWeaponCoverage, 8, 0x20004, 0, "Capture"},
    {"Resurrecting", ResurrectOrder, DrawPathAnim, 0x12, 0x2000b, 0, "Resurrect"},
    {"Reclaiming", ReclaimOrder, DrawPathAnim, 0x12, 0x1008000b, 0, "Reclaim"},
    {"Reclaiming", (int (__stdcall *)(Unit *, Order *, int))ReclaimUnitOrder, DrawPathAnim, 0x12, 0x1002000b, 0, "ReclaimUnit"},
    {"Repairing", RepairUnitNoMoveOrder, DrawWeaponCoverage, 0x18, 0x20006, 0, "RepairUnitNoMove"},
};

// GLOBAL: 0x4fca18
extern const UnitOrderType g_vtolOrders[22] = {
    {"Standby", VtolStandbyOrder, 0, 0, 0x200000f, 0, "VTOL_Standby"},
    {"Moving", VtolMoveOrder, DrawPathAnim, 2, 0x4020e, 0, "VTOL_Move"},
    {"Landing", VtolLandingOrder, DrawWeaponCoverage, 8, 0x6000e, 0, "VTOL_Landing"},
    {"Loading", VtolPickupOrder, DrawWeaponCoverage, 8, 0x20008, 0, "VTOL_Pickup"},
    {"Unloading", VtolUnloadOrder, DrawWeaponCoverage, 8, 0x40009, 0, "VTOL_Unload"},
    {"Guarding", VtolFollowOrder, DrawPathAnim, 2, 0x20005, 0, "VTOL_Follow"},
    {"Patrolling", VtolPatrolOrder, DrawPathAnim, 2, 0x41207, 0, "VTOL_Patrol"},
    {"Airstrike", (int (__stdcall *)(Unit *, Order *, int))AirStrikeOrder, DrawWeaponCoverage, 8, 0x60002, 0, "AirStrike"},
    {"Engaging target", AirToAirOrder, DrawWeaponCoverage, 8, 0x20001, 0, "AirToAir"},
    {"Engaging target", AirToGroundOrder, DrawWeaponCoverage, 8, 0x20001, 0, "AirToGround"},
    {"Engaging target", AirToGroundHoverOrder, DrawWeaponCoverage, 8, 0x20001, 0, "AirToGroundHover"},
    {"Nanolathing", VtolMobileBuildOrder, DrawBuildFootprint, 3, 0x10050800, 0, "VTOL_MobileBuild"},
    {"Nanolathing", VtolHelpBuildOrder, DrawWeaponCoverage, 8, 0x10020806, 0, "VTOL_HelpBuild"},
    {"Repair patrol", (int (__stdcall *)(Unit *, Order *, int))VtolRepairPatrolOrder, DrawPathAnim, 2, 0x41207, 0, "VTOL_RepairPatrol"},
    {"Repairing", VtolRepairUnitOrder, DrawPathAnim, 2, 0x10020006, 0, "VTOL_RepairUnit"},
    {"Reclaiming", VtolReclaimOrder, DrawPathAnim, 2, 0x1008000b, 0, "VTOL_Reclaim"},
    {"Reclaiming", (int (__stdcall *)(Unit *, Order *, int))VtolReclaimUnitOrder, DrawPathAnim, 2, 0x1002000b, 0, "VTOL_ReclaimUnit"},
    {"Evading", VtolEvadeOrder, 0, 0, 0x13, 0, "VTOL_Evade"},
    {"Seeking to attack", VtolSeekAttackOrder, 0, 0, 0x60013, 0, "VTOL_SeekAttack"},
    {"Seeking to guard", VtolSeekGuardOrder, 0, 0, 0x60013, 0, "VTOL_SeekGuard"},
    {"Under repair", VtolGetRepairedOrder, 0, 0, 0x20013, 0, "VTOL_GetRepaired"},
    {"Seeking to land", VtolLandIfCanOrder, 0, 0, 0x40013, 0, "VTOL_LandIfCan"},
};

// GLOBAL: 0x4fd288
extern const UnitOrderType g_readyOrder[1] = {
    {"Ready", (int (__stdcall *)(Unit *, Order *, int))ReadyOrder, 0, 0, 0xf, 0, ""},
};

extern const int kAlignPad = 0;


static inline Vec3 PosPlus(const Vec3& a, const Vec3& other)
{
    Vec3 r;
    r.x = a.x + other.x;
    r.y = a.y + other.y;
    r.z = a.z + other.z;
    return r;
}
struct Unit;

static inline Point16 WorldToCell16(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}
struct Feature;
// FUNCTION: 0x403a20
int __stdcall MobileBuildOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 8) {
        RecordUnitSpeech(unit, 7, SIM_SPEECH_CONSTRUCTION_TERMINATED);
        RecordSelectionOrdersDirty(unit);
        return 8;
    }
    if (flags & 2) {
        RecordSelectionOrdersDirty(unit);
        return 5;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0: {
        UnitDef* def = &Definitions()[order->type];
        Point16 cell = WorldToCell(order->pos, *(Point16*)&def->footprintx);
        CellToWorld(*(Point16*)&def->footprintx, cell, &order->pos);
        order->retries = 0;
        order->AttachBuildFootprintMarker((*(Point16*)&cell), (*(Point16*)&def->footprintx));
        order->flags = 0xe0;
        return 1;
    }
    case 1: {
        UnitDef* def = &Definitions()[order->type];
        if (flags & 0x40) {
            Fixed distance;
            distance.value = (int)_hypot(unit->pos.x - order->pos.x, unit->pos.z - order->pos.z);
            int gap = distance.whole - (int)(_hypot(unit->footprint.x, unit->footprint.y) * 8.0);
            gap += (int)(_hypot(def->footprintx, def->footprintz) * -8.0);
            unsigned int range = 0;
            range = unit->def->buildRange;
            if (gap > (int)range) {
                RecordUnitSpeech(unit, 7, SIM_SPEECH_I_CANT_REACH_THE_CONSTRUCTION_SITE);
                return 8;
            }
        }
        if (!g_seams.map->CanPlaceUnitFootprint(def, 0, WorldToCell16(order->pos, *(Point16*)&Definitions()[order->type].footprintx), 1)) {
            if (!order->retries)
                RecordUnitSpeech(unit, 7, SIM_SPEECH_WAITING_FOR_TARGET_AREA_TO_CLEAR);
            else if (order->retries > 10) {
                RecordUnitSpeech(unit, 7, SIM_SPEECH_TARGET_AREA_WAS_BLOCKED);
                return 8;
            }
            order->retries++;
            order->SetDeadlineTicks(30);
            return 2;
        }
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        g_seams.map->SnapWorldPosToFootprint(def, &order->pos);
        ((PathOrderAttach*)((char*)order + 0x12))->SetUnit(
            g_seams.units->CreateUnit(unit->playerIndex, (short)order->type, order->pos, 0, 1, 0));
        if (!order->target) {
            RecordUnitSpeech(unit, 7, SIM_SPEECH_UNABLE_TO_CREATE_ANY_MORE_UNITS);
            order->SetDeadlineTicks(300);
            return 2;
        }
        RecordUnitSpeech(unit, 9, SIM_SPEECH_STARTING_CONSTRUCTION);
        RecordSelectionOrdersDirty(unit);
        AddOrder("getbuilt", 1, order->target, unit, 0, 0, 0);
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &order->target->pos) - unit->heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 10);
    case 3: {
        int rate = 0;
        rate = unit->def->buildRate;
        if (g_seams.units->AddBuildProgress(unit, order->target, (float)(rate / 30))) {
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(unit, &start);
            Vec3 bounds[2];
            bounds[0] = PosPlus(order->target->pos, *(Vec3*)&order->target->def->modelMinX);
            bounds[1] = PosPlus(order->target->pos, *(Vec3*)&order->target->def->modelMaxX);
            RecordNanoParticles(start, bounds, 6);
        }
        unit->workTime = g_game->ticks + 300;
        if (order->target->buildLeft != 0.0f) {
            order->SetDeadlineTicks(1);
            order->flags |= 0xa;
            return 2;
        }
        return 1;
    }
    case 4:
        RecordUnitSpeech(unit, 8, SIM_SPEECH_BUILDING_COMPLETE);
        return 5;
    }
    return 7;
}






// FUNCTION: 0x404ad0
int __stdcall ReclaimOrder(Unit* unit, Order* order, int flags)
{
    Point16 cell;
    Point16 size;
    unsigned short index = g_seams.weapons->FindFeatureAtPos(&order->pos, &cell, &size);
    if (index == 0xffff) {
        RecordUnitSpeech(unit, 7, SIM_SPEECH_RECLAMATION_FAILED);
        return 8;
    }
    Feature* f = &g_game->features[index];
    if (!(f->flags & 0x80))
        return 8;
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags2 & 0x400)) {
            order->AttachBuildFootprintMarker(cell, size);
            order->flags = 0xe0;
            return 1;
        }
        break;
    case 1: {
        if (flags & 0x40)
            return 8;
        order->time = (int)(15.0f - (f->energy + f->metal) * -0.5f);
        Vec3 pos;
        pos.x = (size.x + cell.x * 2) << 19;
        pos.z = (size.y + cell.y * 2) << 19;
        pos.y = (RandomInt(f->height) + g_seams.units->GetGroundHeight(&pos)) << 16;
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &pos) - (unsigned short)unit->heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 0);
    case 3:
        RecordUnitSpeech(unit, 11, SIM_SPEECH_KIND_TEXT);
    case 4:
        order->SetDeadlineTicks(2);
        order->time -= 2;
        if (order->time <= 0)
            return 1;
        unit->workTime = g_game->ticks + 300;
        if (order->time > 15) {
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(unit, &nano);
            Box box;
            box.lo.x = cell.x << 20;
            box.lo.z = cell.y << 20;
            box.lo.y = g_seams.units->GetGroundHeight(&box.lo) << 16;
            box.hi = box.lo;
            box.hi.x += f->footprintX << 20;
            box.hi.z += f->footprintZ << 20;
            box.hi.y += f->height << 16;
            RecordReverseNanoParticles(box, nano, 6);
            RecordReverseNanoParticles(box, nano, 6);
        }
        return 2;
    case 5:
        g_seams.features->ReclaimFeature(unit, &order->pos);
        return 5;
    }
    return 7;
}

struct Unit;


// FUNCTION: 0x405980
int __stdcall RepairPatrolOrder(Unit* unit, Order* order, int flags)
{
    for (;;) {
        switch (order->state) {
        case 0:
            if (order->target) order->pos = order->target->pos;
            EnsurePatrolReturnOrder(unit, order);
            return 1;
        case 1: {
            if (flags & 0xe0) return 6;
            order->AttachApproachRadiusGoal(&order->pos, 16);
            order->SetDeadlineTicks(60);
            order->flags |= 0xe0;
            if (unit->player->energy >= unit->player->energyCapacity * 0.2) {
                std::vector<Unit*> units;
                int range = unit->def->range << 16;
                DamagedAllyCollector collector(unit->player, &units, unit);
                g_seams.map->VisitObjectsInRange(&unit->pos, range, (UnitVisitor*)&collector);
                if (!units.empty()) {
                    Unit* target = units[RandomInt(units.size())];
                    if (unit->player->allied[target->player->index]) {
                        MissionType kind = GetOrderType(8, unit, target, nullptr);
                        if (kind.index) {
                            if (IssueRepairOrder(unit, target, 0)) return 6;
                            return 3;
                        }
                    }
                }
            }
            Player* owner = unit->player;
            if (unit->player->energy < 0.2 * owner->energyCapacity ||
                unit->player->metal < unit->player->metalCapacity * 0.2) {
                Vec3 energyPos, metalPos;
                Vec3* energy = &energyPos;
                Vec3* metal = &metalPos;
                int range2 = unit->def->range << 16;
                do {} while (0);
                float energyAmount, metalAmount;
                if (g_seams.map->PickRandomReclaimableResourcesInRadius(&unit->pos, range2, &energy, &energyAmount, &metal, &metalAmount)) {
                    if (metal && unit->player->metal < unit->player->metalCapacity * 0.2) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("RECLAIM", 0, metal, 0, 0, 0));
                        order->SetAttachedFx(0);
                        order->flags = 0;
                        return 3;
                    }
                    if (energy && unit->player->energy < unit->player->energyCapacity * 0.2) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("RECLAIM", 0, energy, 0, 0, 0));
                        order->flags = 0;
                        return 3;
                    }
                    if (metal && unit->player->metal + metalAmount <= unit->player->metalCapacity) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("RECLAIM", 0, metal, 0, 0, 0));
                        order->flags = 0;
                        return 3;
                    }
                    if (energy && unit->player->energy + energyAmount <= unit->player->energyCapacity) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("RECLAIM", 0, energy, 0, 0, 0));
                        order->flags = 0;
                        return 3;
                    }
                }
            }
            return 2;
        }
        }
        return 7;
    }
}

class UnitMotion;
struct Player;
struct Unit;
struct Order;

struct Feature;
// FUNCTION: 0x406300
int __stdcall FollowGroundOrder(Unit* unit, Order* order, int flags)
{
    if (!order->target) return 5;
    if (unit->carrier) return 7;
    if ((unsigned char)(order->target->def->flags1 >> 11) & 1) return 8;
    unsigned int state=0; state=order->state;
    switch(state) {
    case 0: {
        order->AnnounceStatusIfFlagged(SIM_SPEECH_GUARDING);
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->approachRadius=(unit->footprint.x + order->target->footprint.x + 2) << 4;
        int distance=order->approachRadius << 16;
        short angle=RandomInt(0x10000);
        order->pos=Offset(angle,distance);
        return 1;
    }
    case 1: {
        Unit* attacker=order->target->attacker;
        if (attacker && !attacker->player.get()->allied[unit->player.get()->index] && (flags & 0x10) &&
            !Contains(unit->def->categories,attacker->unitDefIndex)) {
            if (IssueAttackOrder(unit,attacker,1)) { order->flags=0; return 3; }
            if (unit->flags & 0x300000) {
                for (unsigned char i=0;i<3;++i) {
                    UnitWeaponSlot* weapon=&unit->weapons[i];
                    if ((weapon->flags&2) && (weapon->flags&0x10) && !((unsigned char)(((WeaponDef*)weapon->weapon)->flags.value >> 26)&1)) {
                        Unit* target=g_seams.targeting->GetWeaponTargetUnit(unit,i);
                        if (!target || !g_seams.weapons->WeaponCanReachUnit(unit,target,i) || Contains(unit->def->weaponCategories[i],target->unitDefIndex))
                            g_seams.targeting->SetWeaponTargetUnit(unit,attacker,i);
                    }
                }
            }
        }
        if ((unsigned int)order->target->health < order->target->def->maxHealth && (unit->def->flags1&0x40)) {
            MissionType kind=GetOrderType(8, unit, (Unit*)order->target, nullptr);
            if(kind.index) {
                order->SetAttachedFx(0);
                AppendOrder(unit,new (kSim) Order(kind,order->target,0,0,0,0));
                order->flags=0; return 3;
            }
        }
        if (order->target->list && order->target->list->kind.index &&
            (unit->def->flags1&0x40) && (order->target->def->flags1&0x40) &&
            (order->target->list->flags_42&0x100000) && unit!=order->target->list->Target()) {
            int building=order->target->list->kind=="MobileBuild" ||
                         order->target->list->kind=="BuildingBuild";
            Order* other=order->target->list;
            int actionable=((other->flags_42&0x200) && other->target) || (other->flags_42&0x400);
            MissionType kind;
            if (!building && actionable) {
                order->SetAttachedFx(0);
                kind=order->target->list->kind;
                AppendOrder(unit,new (kSim) Order(kind,order->Target()->list->target,order->Target()->list->Position(),0,0,0));
                order->flags=0; return 3;
            }
            if (building && other->target) {
                order->SetAttachedFx(0);
                kind=MissionType("HelpBuild");
                AppendOrder(unit,new (kSim) Order(kind,order->Target()->list->target,order->Target()->list->Position(),0,0,0));
                order->flags=0; return 3;
            }
        }
follow:
        Vec3 pos=order->target->pos+order->pos;
        order->AttachApproachRadiusGoal(&pos,order->approachRadius/2);
        order->SetDeadlineTicks(30);
        order->flags|=0x18;
        return 2;
    }
    }
    return 7;
}

static inline Vec Add(const Vec& a,const Vec& b) { Vec r; r.x=a.x+b.x; r.y=a.y+b.y; r.z=a.z+b.z; return r; }
static inline Vec Sub(const Vec& a,const Vec& b) { Vec r; r.x=a.x-b.x; r.y=a.y-b.y; r.z=a.z-b.z; return r; }
// FUNCTION: 0x406aa0
int __stdcall TeleportOrder(Unit* unit,Order* order,int unused)
{
    Vec low = Add(*(Vec*)&unit->pos, *(Vec*)&unit->def.get()->modelMinX);
    Vec high = Add(*(Vec*)&unit->pos, *(Vec*)&unit->def.get()->modelMaxX);
    for(Unit* p=g_game->units;p<=g_game->unitsEnd;++p) {
        if(p->pos.x >= low.x && p->pos.x <= high.x && p->pos.z >= low.z && p->pos.z <= high.z && p->pos.y >= low.y && p->pos.y <= high.y && unit != p) {
            Vec dest = Sub(Add(*(Vec*)&order->pos,*(Vec*)&p->pos),*(Vec*)&unit->pos);
            RecordTeleportParticles(p->pos,*(Vec3*)&dest,30,5);
            g_seams.units->SetUnitPosition(p,*(Vec3*)&dest,1);
        }
    }
    return 5;
}

// FUNCTION: 0x406c70
void __stdcall CopyDwordIfNonNull(int* dest, int* src)
{
    if (dest != 0) {
        *dest = *src;
    }
}

// FUNCTION: 0x404db0
int __stdcall ResurrectOrder(Unit* unit, Order* order, int flags)
{
    Point16 cell;
    Point16 size;
    Feature* f = &g_game->features[0xffff];
    if (order->state <= 5) {
        unsigned short index = g_seams.weapons->FindFeatureAtPos(&order->pos, &cell, &size);
        if (index == 0xffff) {
            RecordUnitSpeech(unit, 7, SIM_SPEECH_RESURRECTION_FAILED);
            return 8;
        }
        f = &g_game->features[index];
        if (!(f->flags & 0x80))
            return 8;
    }
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags2 & 0x800)) {
            order->AttachBuildFootprintMarker(cell, size);
            order->flags = 0xe0;
            return 1;
        }
        return 7;
    case 1: {
        if (flags & 0x40)
            return 8;
        Vec3 pos;
        pos.x = (size.x + cell.x * 2) << 19;
        pos.z = (size.y + cell.y * 2) << 19;
        pos.y = (RandomInt(f->height) + g_seams.units->GetGroundHeight(&pos)) << 16;
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &pos) - unit->angles.heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 0);
    case 3: {
        char name[0x40];
        {
            int n = 0;
            while (n < 0x3f && f->name[n]) {
                name[n] = f->name[n];
                n++;
            }
            name[n] = 0;
        }
        for (int i = 0; i < 0x40; i++) {
            if (name[i] == '_') {
                name[i] = 0;
                break;
            }
        }
        order->unitType = g_seams.units->FindUnitTypeId(name);
        if (!order->unitType) {
            RecordUnitSpeech(unit, 7, SIM_SPEECH_RESSURECTION_FAILED);
            return 8;
        }
        order->countdown = (int)(g_game->defs[order->unitType].buildtime * 0.3 / (unit->def->buildRate / 30));
        RecordUnitSpeech(unit, 11, SIM_SPEECH_KIND_TEXT);
        return 1;
    }
    case 4:
        if (order->countdown--) {
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(unit, &nano);
            Box box;
            box.lo.x = cell.x << 20;
            box.lo.z = cell.y << 20;
            box.lo.y = g_seams.units->GetGroundHeight(&box.lo) << 16;
            box.hi = box.lo;
            box.hi.x += f->footprintX << 20;
            box.hi.z += f->footprintZ << 20;
            box.hi.y += f->height << 16;
            RecordNanoParticles(nano, box, 6);
            unit->workTime = g_game->ticks + 300;
            order->SetDeadlineTicks(1);
            return 2;
        }
        break;
    case 5: {
        order->link.SetUnit(g_seams.units->CreateUnit(unit->playerIndex, order->unitType, order->pos, 0, 1, 0));
        if (!order->target) {
            RecordUnitSpeech(unit, 7, SIM_SPEECH_UNABLE_TO_CREATE_ANY_MORE_UNITS);
            order->SetDeadlineTicks(300);
            return 2;
        }
        Cell* c = g_seams.los_radar->GetOriginCellAtPosition(&order->pos);
        if (g_seams.features->GetCellFeature(c) >= 0xfffb)
            return 8;
        FeatureSpot* spot = &g_game->spots[c->offsetY | (c->offsetX << 8)];
        if (spot)
            order->target->angles = spot->rot;
        g_seams.features->RemoveFeature(g_seams.los_radar->GetMapCellAtPosition(&order->pos), 0);
        if (g_seams.map->GetGameType(g_game->net) == 3) {
            FeatureDamagePacket packet;
            int n = c - g_game->cells;
            int w = g_game->width;
            packet.type = 0x0f;
            packet.sub = 0xff;
            Point16 xz;
            xz.x = n % w;
            xz.y = n / w;
            packet.x = xz.x;
            packet.z = xz.y;
            BroadcastPacket(unit->player->id, &packet, 6);
        }
        order->target->buildLeft = 0;
        order->target->health = 1;
        RecordSelectionOrdersDirty(unit);
        break;
    }
    case 6: {
        RecordUnitSpeech(unit, 8, SIM_SPEECH_RESURRECTION_COMPLETE);
        MissionType kind = GetOrderType(8, unit, (Unit*)order->target, nullptr);
        if (kind.index)
            AppendOrder(unit, new (kSim) Order(kind, order->target, 0, 0, 0, 0));
        return 5;
    }
    default:
        return 7;
    }
    return 1;
}
