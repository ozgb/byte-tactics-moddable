#include <stddef.h>

#pragma pack(push, 1)
#include "../weapons/unit_weapon_slot.h"
#include "../network/player.h"
#include "unit.h"
#pragma pack(pop)

#include "../util/vec3.h"
#include "../util/angles.h"
#include "../state/sim_speech.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/units.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/ref.h"
#include "../util/engine_services.h"
#include "../game/game.h"
#include "../game/units_event_size.h"
#include "../game/weapons_events.h"
#include "../ingame/control_panel.h"
#include "../ingame/info_panel.h"
#include "unit_position.h"
#include "unit_targets.h"
#include "unit_ref.h"
#include "sfx_occupy.h"
#include "units.h"
#include "unit_types.h"
#include "units_seam.h"

#include "../../link/globals.h"

Unit* __stdcall CreateUnit(unsigned char player, unsigned short typeId, Vec3 pos, int finished, int mode, int id);

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

static int KnownKind(int kind)
{
    return (kind >= UNITS_SET_STATE_BITS && kind <= UNITS_CHANGE_CELL) || (kind >= UNITS_SET_MOVED && kind <= UNITS_FORGET_ATTACKER);
}

static void UnitsApplyAttackedSpeech(Unit* unit)
{
    if (!(g_seams.orders->GetOrderFlags(unit) & ORDER_FLAG_NO_UNDERATTACK_LINE) &&
        (unit->lastAttackerSlot != unit->playerIndex || unit->lastDamageType == 1))
        RecordUnitSpeech(unit, 2, SIM_SPEECH_KIND_TEXT, SIM_EVENT_SPEECH_IF_NOT_VISIBLE);
}

static void UnitsApplyScriptValue(Unit* unit, int which, int value)
{
    switch (which) {
    case 1:
        unit->SetStateBits(1, value);
        break;
    case 5:
        unit->bit0 = value;
        break;
    case 6:
        unit->bit1 = value;
        break;
    case 19:
        unit->bit3 = value;
        break;
    case 20:
        unit->SetStateBits(2, value);
        break;
    }
    unit->dirty = true;
}

static void UnitsApplyKill(Unit* u, int selfDestruct)
{
    if (selfDestruct == 0) {
        WeaponsPublishDamage(SCHED_UNITS, u, u, 0x7530, 3, 0);
    } else {
        WeaponsPublishDetonate(SCHED_UNITS, u, 1);
        u->flags |= 0x4000;
        KillUnit(u, 3);
    }
}

static void UnitsApplySend(Unit* unit, int player)
{
    Player* other = &g_game->players[player];
    unit->flags &= ~0x10;
    unit->postTransferHoldoff = 0x96;
    g_seams.network->SendOwnershipTransfer(unit, other);
    WeaponsPublishDamage(SCHED_UNITS, 0, unit, 30000, 4, 0);
}

static int UnitsApplyCapture(Unit* unit, const UnitsCapture* e)
{
    Player* other = &g_game->players[e->player];
    Unit* n = CreateUnit(other->index, unit->unitDefIndex, *(Vec3*)&unit->pos, 1, unit->flags & 3, 0);
    if (!n)
        return 0;
    n->flags &= 0xffc3ffff;
    if (e->present) {
        n->health = (short)e->hitPoints;
        n->buildLeft = (float)e->buildPercent;
        Angles16 angles;
        angles.bank = (short)e->angles[0];
        angles.heading = (unsigned short)e->angles[1];
        angles.pitch = (short)e->angles[2];
        n->angles = angles;
        WeaponsPublishSetStockpiles(SCHED_UNITS, n, (unsigned char)e->stockpiles[0], (unsigned char)e->stockpiles[1], (unsigned char)e->stockpiles[2]);
    } else {
        n->health = unit->health;
        n->speed_bits = unit->speed_bits;
        n->angles = unit->angles;
        WeaponsPublishSetStockpiles(SCHED_UNITS, n, unit->weapons[0].stockpile, unit->weapons[1].stockpile, unit->weapons[2].stockpile);
        WeaponsPublishDamage(SCHED_UNITS, 0, unit, 30000, 4, 0);
    }
    n->SetStateBits(unit->activateFlags, 1);
    n->SetStateBits(~unit->activateFlags, 0);
    return 1;
}

int __stdcall UnitsApply(const UnitsEventHeader* e, unsigned int size)
{
    if (size < sizeof(UnitsEventHeader))
        return 0;
    if (!KnownKind(e->kind))
        return 0;
    if (size < UnitsEventSize(e->kind))
        return 0;
    if (e->kind == UNITS_UNLINK) {
        Ref<UnitRef> ref;
        ref.offset = reinterpret_cast<const UnitsUnlink*>(e)->ref;
        if (ref.get() == nullptr)
            return 0;
        ref.get()->Unlink();
        return 1;
    }
    switch (e->kind) {
    case UNITS_CREATE: {
        const UnitsCreate* p = reinterpret_cast<const UnitsCreate*>(e);
        Vec3 pos;
        pos.x = p->pos[0];
        pos.y = p->pos[1];
        pos.z = p->pos[2];
        return CreateUnit((unsigned char)p->player, (unsigned short)p->type, pos, 1, 1, 0) != nullptr;
    }
    case UNITS_KILL_ALL:
        KillAllUnits();
        return 1;
    case UNITS_RELOAD_TYPE: {
        const UnitsReloadType* p = reinterpret_cast<const UnitsReloadType*>(e);
        KillUnitsOfType((short)p->type);
        ReloadUnitType((unsigned short)p->type);
        return 1;
    }
    }
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return 0;
    switch (e->kind) {
    case UNITS_SET_STATE_BITS: {
        const UnitsSetStateBits* p = reinterpret_cast<const UnitsSetStateBits*>(e);
        unit->SetStateBits(p->mask, p->set);
        break;
    }
    case UNITS_SET_FLAGS: {
        const UnitsSetFlags* p = reinterpret_cast<const UnitsSetFlags*>(e);
        unit->flags = (unit->flags & ~p->clear) | p->set;
        break;
    }
    case UNITS_FORGET_ATTACKER:
        unit->lastAttackerSlot = 10;
        unit->panel.flag_110 = 1;
        unit->attacker = 0;
        break;
    case UNITS_SET_NET_DIRTY: {
        const UnitsSetNetDirty* p = reinterpret_cast<const UnitsSetNetDirty*>(e);
        unit->netDirtyFlags = (unsigned short)((unit->netDirtyFlags & ~p->clear) | p->set);
        break;
    }
    case UNITS_SET_WORK_TIME:
        unit->workTime = reinterpret_cast<const UnitsSetWorkTime*>(e)->until;
        break;
    case UNITS_SET_POSITION: {
        const UnitsSetPosition* p = reinterpret_cast<const UnitsSetPosition*>(e);
        Vec3 pos;
        pos.x = p->pos[0];
        pos.y = p->pos[1];
        pos.z = p->pos[2];
        SetUnitPosition(unit, pos, p->mode);
        break;
    }
    case UNITS_ATTACH_TO_PIECE: {
        const UnitsAttachToPiece* p = reinterpret_cast<const UnitsAttachToPiece*>(e);
        AttachUnitToPiece(unit, UnitOfOffset(p->target), (char)p->piece, (char)p->mode);
        break;
    }
    case UNITS_FINISH_CONSTRUCTION:
        FinishConstruction(UnitOfOffset(reinterpret_cast<const UnitsFinishConstruction*>(e)->builder), unit);
        break;
    case UNITS_BUILD_DECAY:
        ApplyUnfinishedBuildDecay(unit, reinterpret_cast<const UnitsBuildDecay*>(e)->ticks);
        break;
    case UNITS_REPAIR: {
        const UnitsRepair* p = reinterpret_cast<const UnitsRepair*>(e);
        AddRepairProgress(UnitOfOffset(p->builder), unit, p->amount);
        break;
    }
    case UNITS_SET_STANCE: {
        const UnitsSetStance* p = reinterpret_cast<const UnitsSetStance*>(e);
        if (p->fields & UNITS_STANCE_MOVE)
            unit->moveOrder = p->moveOrder;
        if (p->fields & UNITS_STANCE_FIRE)
            unit->fireOrder = p->fireOrder;
        if (p->fields & UNITS_STANCE_GROUP)
            unit->group = p->group;
        break;
    }
    case UNITS_SET_ANGLES: {
        const UnitsSetAngles* p = reinterpret_cast<const UnitsSetAngles*>(e);
        if (p->fields & UNITS_ANGLE_BANK)
            unit->angles.bank = (short)p->bank;
        if (p->fields & UNITS_ANGLE_HEADING)
            unit->angles.heading = (unsigned short)p->heading;
        if (p->fields & UNITS_ANGLE_PITCH)
            unit->angles.pitch = (short)p->pitch;
        break;
    }
    case UNITS_SET_BUILD_STATE: {
        const UnitsSetBuildState* p = reinterpret_cast<const UnitsSetBuildState*>(e);
        unit->buildLeft = p->buildLeft;
        unit->health = (short)p->health;
        break;
    }
    case UNITS_TURN:
        unit->angles.heading = (unsigned short)(unit->angles.heading + reinterpret_cast<const UnitsTurn*>(e)->delta);
        unit->moved = 1;
        break;
    case UNITS_MOVE: {
        const UnitsMove* p = reinterpret_cast<const UnitsMove*>(e);
        unit->pos.x = p->pos[0];
        unit->pos.y = p->pos[1];
        unit->pos.z = p->pos[2];
        unit->moved = 1;
        break;
    }
    case UNITS_CHANGE_CELL: {
        const UnitsChangeCell* p = reinterpret_cast<const UnitsChangeCell*>(e);
        unit->pos.x = p->pos[0];
        unit->pos.y = p->pos[1];
        unit->pos.z = p->pos[2];
        unit->cell.x = (short)p->cell[0];
        unit->cell.y = (short)p->cell[1];
        unit->mode = (short)p->mode;
        break;
    }
    case UNITS_SET_MOVED:
        unit->moved = reinterpret_cast<const UnitsSetMoved*>(e)->moved;
        break;
    case UNITS_SFX_OCCUPY:
        ((SfxOccupyTracker*)unit->motion.get())->UpdateSfxOccupy(unit);
        break;
    case UNITS_SCRIPT_VALUE: {
        const UnitsScriptValue* p = reinterpret_cast<const UnitsScriptValue*>(e);
        UnitsApplyScriptValue(unit, p->which, p->value);
        break;
    }
    case UNITS_NOTIFY_REFS:
        NotifyUnitRefs(unit, reinterpret_cast<const UnitsNotifyRefs*>(e)->event);
        break;
    case UNITS_ATTACKED_SPEECH:
        UnitsApplyAttackedSpeech(unit);
        break;
    case UNITS_MARK_DAMAGED:
        MarkRecentlyDamaged(unit);
        break;
    case UNITS_DAMAGE_RECORD: {
        const UnitsDamageRecord* p = reinterpret_cast<const UnitsDamageRecord*>(e);
        unit->lastDamageType = (unsigned char)p->damageKind;
        Unit* attacker = UnitOfOffset(p->attacker);
        if (attacker) {
            unit->lastAttackerSlot = attacker->playerIndex;
            unit->attacker = attacker;
        }
        break;
    }
    case UNITS_SET_HEALTH: {
        const UnitsSetHealth* p = reinterpret_cast<const UnitsSetHealth*>(e);
        unit->health = (short)p->health;
        if (p->dying == 1)
            unit->flags |= 0x4000;
        break;
    }
    case UNITS_KILL:
        UnitsApplyKill(unit, reinterpret_cast<const UnitsKill*>(e)->selfDestruct);
        break;
    case UNITS_SEND:
        if ((unsigned int)reinterpret_cast<const UnitsSend*>(e)->player >= 10)
            return 0;
        UnitsApplySend(unit, reinterpret_cast<const UnitsSend*>(e)->player);
        break;
    case UNITS_CAPTURE:
        if ((unsigned int)reinterpret_cast<const UnitsCapture*>(e)->player >= 10)
            return 0;
        return UnitsApplyCapture(unit, reinterpret_cast<const UnitsCapture*>(e));
    }
    return 1;
}
