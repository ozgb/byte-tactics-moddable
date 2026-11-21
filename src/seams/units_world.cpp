#include <stddef.h>
#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/ai.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/players.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/movement.h"
#include "../../interfaces/units.h"
#include "../../interfaces/services.h"
#include "../game/units_events.h"
#include "units_world.h"

#include "../../link/globals.h"

void UnitsInbox(const void* event, unsigned int size)
{
    const UnitsEventHeader* e = static_cast<const UnitsEventHeader*>(event);
    HarnessUnitsEvent(e, size, g_seams.units->Apply(e, size));
}

void UnitsOnAiActivate(const void* event, unsigned int size)
{
    if (size < sizeof(AiActivate))
        return;
    const AiActivate* effect = static_cast<const AiActivate*>(event);
    UnitsSetStateBits e;
    e.h = UnitsHeaderOf(UNITS_SET_STATE_BITS);
    e.h.unit = effect->h.unit;
    e.mask = (int)effect->bits;
    e.set = effect->on;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnAiSetStates(const void* event, unsigned int size)
{
    if (size < sizeof(AiSetStates))
        return;
    const AiSetStates* effect = static_cast<const AiSetStates*>(event);
    UnitsSetFlags e;
    e.h = UnitsHeaderOf(UNITS_SET_FLAGS);
    e.h.unit = effect->h.unit;
    e.clear = 0x3c0000u;
    e.set = ((unsigned int)effect->movement << 18) | ((unsigned int)effect->firing << 20);
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnEconomyCloak(const void* event, unsigned int size)
{
    if (size < sizeof(EconomyCloak))
        return;
    const EconomyCloak* effect = static_cast<const EconomyCloak*>(event);
    UnitsSetStateBits e;
    e.h = UnitsHeaderOf(UNITS_SET_STATE_BITS);
    e.h.unit = effect->h.unit;
    e.mask = 4;
    e.set = effect->keep;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnPlayersKillUnit(const void* event, unsigned int size)
{
    if (size < sizeof(PlayersKillUnit))
        return;
    const PlayersKillUnit* effect = static_cast<const PlayersKillUnit*>(event);
    UnitsKill e;
    e.h = UnitsHeaderOf(UNITS_KILL);
    e.h.unit = effect->h.unit;
    e.selfDestruct = effect->selfDestruct;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnPlayersSendUnit(const void* event, unsigned int size)
{
    if (size < sizeof(PlayersSendUnit))
        return;
    const PlayersSendUnit* effect = static_cast<const PlayersSendUnit*>(event);
    UnitsSend e;
    e.h = UnitsHeaderOf(UNITS_SEND);
    e.h.unit = effect->h.unit;
    e.player = effect->player;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnPlayersCaptureUnit(const void* event, unsigned int size)
{
    if (size < sizeof(PlayersCaptureUnit))
        return;
    const PlayersCaptureUnit* effect = static_cast<const PlayersCaptureUnit*>(event);
    UnitsCapture e;
    e.h = UnitsHeaderOf(UNITS_CAPTURE);
    e.h.unit = effect->h.unit;
    e.player = effect->player;
    e.present = effect->record.present;
    e.hitPoints = effect->record.hitPoints;
    e.buildPercent = effect->record.buildPercent;
    memcpy(e.angles, effect->record.angles, sizeof(e.angles));
    memcpy(e.stockpiles, effect->record.stockpiles, sizeof(e.stockpiles));
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnWeaponsAttacked(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsAttacked))
        return;
    const WeaponsAttacked* effect = static_cast<const WeaponsAttacked*>(event);
    UnitsNotifyRefs e;
    e.h = UnitsHeaderOf(UNITS_NOTIFY_REFS);
    e.h.unit = effect->h.unit;
    e.event = 16;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnWeaponsHit(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsHit))
        return;
    const WeaponsHit* effect = static_cast<const WeaponsHit*>(event);
    UnitsSetNetDirty e;
    e.h = UnitsHeaderOf(UNITS_SET_NET_DIRTY);
    e.h.unit = effect->h.unit;
    e.clear = 0;
    e.set = (effect->enemyDamage > effect->friendlyDamage * 2) ? 0x4000 : 0x2000;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnWeaponsAttackedSpeech(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsAttacked))
        return;
    const WeaponsAttacked* effect = static_cast<const WeaponsAttacked*>(event);
    UnitsEventHeader e = UnitsHeaderOf(UNITS_ATTACKED_SPEECH);
    e.unit = effect->h.unit;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnWeaponsMarkDamaged(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsEventHeader))
        return;
    const WeaponsEventHeader* effect = static_cast<const WeaponsEventHeader*>(event);
    UnitsEventHeader e = UnitsHeaderOf(UNITS_MARK_DAMAGED);
    e.unit = effect->unit;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnWeaponsDamageRecord(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsDamageRecord))
        return;
    const WeaponsDamageRecord* effect = static_cast<const WeaponsDamageRecord*>(event);
    UnitsDamageRecord e;
    e.h = UnitsHeaderOf(UNITS_DAMAGE_RECORD);
    e.h.unit = effect->h.unit;
    e.damageKind = effect->damageKind;
    e.attacker = effect->attacker;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnWeaponsHealth(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsHealth))
        return;
    const WeaponsHealth* effect = static_cast<const WeaponsHealth*>(event);
    UnitsSetHealth e;
    e.h = UnitsHeaderOf(UNITS_SET_HEALTH);
    e.h.unit = effect->h.unit;
    e.health = effect->health;
    e.dying = effect->dying;
    UnitsInbox(&e, sizeof(e));
}

static int NoticeBit(int kind)
{
    switch (kind) {
    case WEAPONS_EFFECT_SHOT_FIRED:
        return 0x400;
    case WEAPONS_EFFECT_COMMAND_SHOT_FIRED:
        return 0x800;
    }
    return 0x1000;
}

void UnitsOnWeaponsNotice(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsEventHeader))
        return;
    const WeaponsEventHeader* effect = static_cast<const WeaponsEventHeader*>(event);
    UnitsSetNetDirty e;
    e.h = UnitsHeaderOf(UNITS_SET_NET_DIRTY);
    e.h.unit = effect->unit;
    e.clear = 0;
    e.set = (unsigned short)NoticeBit(effect->kind);
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnWeaponsBusy(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsBusy))
        return;
    const WeaponsBusy* effect = static_cast<const WeaponsBusy*>(event);
    UnitsSetWorkTime e;
    e.h = UnitsHeaderOf(UNITS_SET_WORK_TIME);
    e.h.unit = effect->h.unit;
    e.until = effect->until;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersSetStateBits(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetStateBits))
        return;
    const OrdersSetStateBits* effect = static_cast<const OrdersSetStateBits*>(event);
    UnitsSetStateBits e;
    e.h = UnitsHeaderOf(UNITS_SET_STATE_BITS);
    e.h.unit = effect->h.unit;
    e.mask = effect->mask;
    e.set = effect->set;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersSetPosition(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetPosition))
        return;
    const OrdersSetPosition* effect = static_cast<const OrdersSetPosition*>(event);
    UnitsSetPosition e;
    e.h = UnitsHeaderOf(UNITS_SET_POSITION);
    e.h.unit = effect->h.unit;
    memcpy(e.pos, effect->pos, sizeof(e.pos));
    e.mode = effect->mode;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersAttachToPiece(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersAttachToPiece))
        return;
    const OrdersAttachToPiece* effect = static_cast<const OrdersAttachToPiece*>(event);
    UnitsAttachToPiece e;
    e.h = UnitsHeaderOf(UNITS_ATTACH_TO_PIECE);
    e.h.unit = effect->h.unit;
    e.target = effect->target;
    e.piece = effect->piece;
    e.mode = effect->mode;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersFinishConstruction(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersFinishConstruction))
        return;
    const OrdersFinishConstruction* effect = static_cast<const OrdersFinishConstruction*>(event);
    UnitsFinishConstruction e;
    e.h = UnitsHeaderOf(UNITS_FINISH_CONSTRUCTION);
    e.h.unit = effect->h.unit;
    e.builder = effect->builder;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersBuildDecay(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersBuildDecay))
        return;
    const OrdersBuildDecay* effect = static_cast<const OrdersBuildDecay*>(event);
    UnitsBuildDecay e;
    e.h = UnitsHeaderOf(UNITS_BUILD_DECAY);
    e.h.unit = effect->h.unit;
    e.ticks = effect->ticks;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersRepair(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersRepair))
        return;
    const OrdersRepair* effect = static_cast<const OrdersRepair*>(event);
    UnitsRepair e;
    e.h = UnitsHeaderOf(UNITS_REPAIR);
    e.h.unit = effect->h.unit;
    e.builder = effect->builder;
    e.amount = effect->amount;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersUnlink(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersUnlink))
        return;
    const OrdersUnlink* effect = static_cast<const OrdersUnlink*>(event);
    UnitsUnlink e;
    e.h = UnitsHeaderOf(UNITS_UNLINK);
    e.h.unit = effect->h.unit;
    e.ref = effect->ref;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersBusy(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersBusy))
        return;
    const OrdersBusy* effect = static_cast<const OrdersBusy*>(event);
    UnitsSetWorkTime e;
    e.h = UnitsHeaderOf(UNITS_SET_WORK_TIME);
    e.h.unit = effect->h.unit;
    e.until = effect->until;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersSetFlags(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetFlags))
        return;
    const OrdersSetFlags* effect = static_cast<const OrdersSetFlags*>(event);
    UnitsSetFlags e;
    e.h = UnitsHeaderOf(UNITS_SET_FLAGS);
    e.h.unit = effect->h.unit;
    e.clear = effect->clear;
    e.set = effect->set;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersClearNetDirty(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersClearNetDirty))
        return;
    const OrdersClearNetDirty* effect = static_cast<const OrdersClearNetDirty*>(event);
    UnitsSetNetDirty e;
    e.h = UnitsHeaderOf(UNITS_SET_NET_DIRTY);
    e.h.unit = effect->h.unit;
    e.clear = effect->mask;
    e.set = 0;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersSetStance(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetStance))
        return;
    const OrdersSetStance* effect = static_cast<const OrdersSetStance*>(event);
    UnitsSetStance e;
    e.h = UnitsHeaderOf(UNITS_SET_STANCE);
    e.h.unit = effect->h.unit;
    e.fields = 0;
    if (effect->fields & ORDERS_STANCE_MOVE)
        e.fields |= UNITS_STANCE_MOVE;
    if (effect->fields & ORDERS_STANCE_FIRE)
        e.fields |= UNITS_STANCE_FIRE;
    if (effect->fields & ORDERS_STANCE_GROUP)
        e.fields |= UNITS_STANCE_GROUP;
    e.moveOrder = effect->moveOrder;
    e.fireOrder = effect->fireOrder;
    e.group = effect->group;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersSetAngles(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetAngles))
        return;
    const OrdersSetAngles* effect = static_cast<const OrdersSetAngles*>(event);
    UnitsSetAngles e;
    e.h = UnitsHeaderOf(UNITS_SET_ANGLES);
    e.h.unit = effect->h.unit;
    e.fields = UNITS_ANGLE_BANK | UNITS_ANGLE_HEADING | UNITS_ANGLE_PITCH;
    e.bank = effect->bank;
    e.heading = effect->heading;
    e.pitch = effect->pitch;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnOrdersSetBuildState(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetBuildState))
        return;
    const OrdersSetBuildState* effect = static_cast<const OrdersSetBuildState*>(event);
    UnitsSetBuildState e;
    e.h = UnitsHeaderOf(UNITS_SET_BUILD_STATE);
    e.h.unit = effect->h.unit;
    e.buildLeft = effect->buildLeft;
    e.health = effect->health;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementTurn(const void* event, unsigned int size)
{
    if (size < sizeof(MovementTurn))
        return;
    const MovementTurn* effect = static_cast<const MovementTurn*>(event);
    UnitsTurn e;
    e.h = UnitsHeaderOf(UNITS_TURN);
    e.h.unit = effect->h.unit;
    e.delta = effect->delta;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementTilt(const void* event, unsigned int size)
{
    if (size < sizeof(MovementTilt))
        return;
    const MovementTilt* effect = static_cast<const MovementTilt*>(event);
    UnitsSetAngles e;
    e.h = UnitsHeaderOf(UNITS_SET_ANGLES);
    e.h.unit = effect->h.unit;
    e.fields = UNITS_ANGLE_BANK | UNITS_ANGLE_PITCH;
    e.bank = effect->bank;
    e.heading = 0;
    e.pitch = effect->pitch;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementAngles(const void* event, unsigned int size)
{
    if (size < sizeof(MovementAngles))
        return;
    const MovementAngles* effect = static_cast<const MovementAngles*>(event);
    UnitsSetAngles e;
    e.h = UnitsHeaderOf(UNITS_SET_ANGLES);
    e.h.unit = effect->h.unit;
    e.fields = UNITS_ANGLE_BANK | UNITS_ANGLE_HEADING | UNITS_ANGLE_PITCH;
    e.bank = effect->bank;
    e.heading = effect->heading;
    e.pitch = effect->pitch;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementMove(const void* event, unsigned int size)
{
    if (size < sizeof(MovementMove))
        return;
    const MovementMove* effect = static_cast<const MovementMove*>(event);
    UnitsMove e;
    e.h = UnitsHeaderOf(UNITS_MOVE);
    e.h.unit = effect->h.unit;
    memcpy(e.pos, effect->pos, sizeof(e.pos));
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementMoved(const void* event, unsigned int size)
{
    if (size < sizeof(MovementMoved))
        return;
    const MovementMoved* effect = static_cast<const MovementMoved*>(event);
    UnitsSetMoved e;
    e.h = UnitsHeaderOf(UNITS_SET_MOVED);
    e.h.unit = effect->h.unit;
    e.moved = effect->moved;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementChangeCell(const void* event, unsigned int size)
{
    if (size < sizeof(MovementChangeCell))
        return;
    const MovementChangeCell* effect = static_cast<const MovementChangeCell*>(event);
    UnitsChangeCell e;
    e.h = UnitsHeaderOf(UNITS_CHANGE_CELL);
    e.h.unit = effect->h.unit;
    memcpy(e.pos, effect->pos, sizeof(e.pos));
    memcpy(e.cell, effect->cell, sizeof(e.cell));
    e.mode = effect->mode;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementMoveRate(const void* event, unsigned int size)
{
    if (size < sizeof(MovementMoveRate))
        return;
    const MovementMoveRate* effect = static_cast<const MovementMoveRate*>(event);
    UnitsSetFlags e;
    e.h = UnitsHeaderOf(UNITS_SET_FLAGS);
    e.h.unit = effect->h.unit;
    e.clear = 0xc;
    e.set = (unsigned int)((effect->rate & 3) << 2);
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementSetPosition(const void* event, unsigned int size)
{
    if (size < sizeof(MovementSetPosition))
        return;
    const MovementSetPosition* effect = static_cast<const MovementSetPosition*>(event);
    UnitsSetPosition e;
    e.h = UnitsHeaderOf(UNITS_SET_POSITION);
    e.h.unit = effect->h.unit;
    memcpy(e.pos, effect->pos, sizeof(e.pos));
    e.mode = effect->mode;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementSetStateBits(const void* event, unsigned int size)
{
    if (size < sizeof(MovementSetStateBits))
        return;
    const MovementSetStateBits* effect = static_cast<const MovementSetStateBits*>(event);
    UnitsSetStateBits e;
    e.h = UnitsHeaderOf(UNITS_SET_STATE_BITS);
    e.h.unit = effect->h.unit;
    e.mask = effect->mask;
    e.set = effect->set;
    UnitsInbox(&e, sizeof(e));
}

void UnitsOnMovementSfxOccupy(const void* event, unsigned int size)
{
    if (size < sizeof(MovementEventHeader))
        return;
    const MovementEventHeader* effect = static_cast<const MovementEventHeader*>(event);
    UnitsEventHeader e = UnitsHeaderOf(UNITS_SFX_OCCUPY);
    e.unit = effect->unit;
    UnitsInbox(&e, sizeof(e));
}
