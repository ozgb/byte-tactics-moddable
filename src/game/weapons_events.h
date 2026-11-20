#ifndef GAME_WEAPONS_EVENTS_H
#define GAME_WEAPONS_EVENTS_H

#include <string.h>

#include "../../interfaces/weapons.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline WeaponsEventHeader WeaponsHeaderOf(int kind)
{
    WeaponsEventHeader header;
    header.kind = kind;
    header.unit = 0;
    return header;
}

template <class U>
inline WeaponsEventHeader WeaponsHeaderOf(int kind, U* unit)
{
    WeaponsEventHeader header = WeaponsHeaderOf(kind);
    header.unit = Ref<U>(unit).offset;
    return header;
}

inline unsigned int WeaponsEventSize(int kind)
{
    switch (kind) {
    case WEAPONS_DAMAGE:
    case WEAPONS_REMOTE_DAMAGE:
        return sizeof(WeaponsDamage);
    case WEAPONS_DETONATE:
        return sizeof(WeaponsDetonate);
    case WEAPONS_RELEASE:
        return sizeof(WeaponsRelease);
    case WEAPONS_CLAIM:
        return sizeof(WeaponsClaim);
    case WEAPONS_AIM_DONE:
        return sizeof(WeaponsAimDone);
    case WEAPONS_EXPLODE_PIECE:
        return sizeof(WeaponsExplodePiece);
    case WEAPONS_AREA_DAMAGE:
        return sizeof(WeaponsAreaDamage);
    case WEAPONS_SPAWN:
        return sizeof(WeaponsSpawn);
    case WEAPONS_SET_STOCKPILES:
        return sizeof(WeaponsSetStockpiles);
    case WEAPONS_ADD_STOCKPILE:
        return sizeof(WeaponsAddStockpile);
    case WEAPONS_EFFECT_ATTACKED:
        return sizeof(WeaponsAttacked);
    case WEAPONS_EFFECT_HIT:
        return sizeof(WeaponsHit);
    case WEAPONS_EFFECT_DAMAGE_RECORD:
        return sizeof(WeaponsDamageRecord);
    case WEAPONS_EFFECT_HEALTH:
        return sizeof(WeaponsHealth);
    case WEAPONS_EFFECT_PARALYZE:
        return sizeof(WeaponsParalyze);
    case WEAPONS_EFFECT_START_SCRIPT:
        return sizeof(WeaponsStartScript);
    case WEAPONS_EFFECT_DETACH_PIECE:
        return sizeof(WeaponsDetachPiece);
    case WEAPONS_EFFECT_BUSY:
        return sizeof(WeaponsBusy);
    }
    return sizeof(WeaponsEventHeader);
}

inline void WeaponsPublishEffect(const void* effect, unsigned int size)
{
    HarnessWeaponsEvent(static_cast<const WeaponsEventHeader*>(effect), size, 1);
    SchedPublish(SCHED_WEAPONS, effect, size);
}

inline void WeaponsPublishDamage(int outbox, Unit* source, Unit* victim, int amount, int type, unsigned short direction)
{
    WeaponsDamage event;
    event.h = WeaponsHeaderOf(WEAPONS_DAMAGE, victim);
    event.source = Ref<Unit>(source).offset;
    event.amount = amount;
    event.damageKind = type;
    event.direction = direction;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishRemoteDamage(int outbox, Unit* victim, Unit* attacker, int amount, int kind, unsigned char heading)
{
    WeaponsDamage event;
    event.h = WeaponsHeaderOf(WEAPONS_REMOTE_DAMAGE, victim);
    event.source = Ref<Unit>(attacker).offset;
    event.amount = amount;
    event.damageKind = kind;
    event.direction = heading << 8;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishDetonate(int outbox, Unit* unit, int selfDestruct)
{
    WeaponsDetonate event;
    event.h = WeaponsHeaderOf(WEAPONS_DETONATE, unit);
    event.selfDestruct = selfDestruct;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishRelease(int outbox, Unit* unit, int index)
{
    WeaponsRelease event;
    event.h = WeaponsHeaderOf(WEAPONS_RELEASE, unit);
    event.weapon = index;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishClaim(int outbox, Unit* unit, int index)
{
    WeaponsClaim event;
    event.h = WeaponsHeaderOf(WEAPONS_CLAIM, unit);
    event.weapon = index;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishInitSlots(int outbox, Unit* unit)
{
    WeaponsEventHeader event = WeaponsHeaderOf(WEAPONS_INIT_SLOTS, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishRemoveProjectiles(int outbox, Unit* unit)
{
    WeaponsEventHeader event = WeaponsHeaderOf(WEAPONS_REMOVE_PROJECTILES, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class C>
inline void WeaponsPublishAimDone(int outbox, C* callback, int result)
{
    WeaponsAimDone event;
    event.h = WeaponsHeaderOf(WEAPONS_AIM_DONE);
    event.callback = Ref<C>(callback).offset;
    event.result = result;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void WeaponsPublishExplodePiece(int outbox, U* unit, int piece, unsigned int flags)
{
    WeaponsExplodePiece event;
    event.h = WeaponsHeaderOf(WEAPONS_EXPLODE_PIECE, unit);
    event.piece = piece;
    event.flags = flags;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishSetStockpiles(int outbox, Unit* unit, int first, int second, int third)
{
    WeaponsSetStockpiles event;
    event.h = WeaponsHeaderOf(WEAPONS_SET_STOCKPILES, unit);
    event.stockpiles[0] = first;
    event.stockpiles[1] = second;
    event.stockpiles[2] = third;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishAddStockpile(int outbox, Unit* unit, int slot)
{
    WeaponsAddStockpile event;
    event.h = WeaponsHeaderOf(WEAPONS_ADD_STOCKPILE, unit);
    event.weapon = slot;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void WeaponsPublishAttacked(U* unit, Unit* attacker, int amount)
{
    WeaponsAttacked effect;
    effect.h = WeaponsHeaderOf(WEAPONS_EFFECT_ATTACKED, unit);
    effect.attacker = Ref<Unit>(attacker).offset;
    effect.amount = amount;
    WeaponsPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void WeaponsPublishHit(U* shooter, int enemyDamage, int friendlyDamage)
{
    WeaponsHit effect;
    effect.h = WeaponsHeaderOf(WEAPONS_EFFECT_HIT, shooter);
    effect.enemyDamage = enemyDamage;
    effect.friendlyDamage = friendlyDamage;
    WeaponsPublishEffect(&effect, sizeof(effect));
}

inline void WeaponsPublishMarkDamaged(Unit* unit)
{
    WeaponsEventHeader effect = WeaponsHeaderOf(WEAPONS_EFFECT_MARK_DAMAGED, unit);
    WeaponsPublishEffect(&effect, sizeof(effect));
}

inline void WeaponsPublishDamageRecord(Unit* unit, int kind, Unit* attacker)
{
    WeaponsDamageRecord effect;
    effect.h = WeaponsHeaderOf(WEAPONS_EFFECT_DAMAGE_RECORD, unit);
    effect.damageKind = kind;
    effect.attacker = Ref<Unit>(attacker).offset;
    WeaponsPublishEffect(&effect, sizeof(effect));
}

inline void WeaponsPublishHealth(Unit* unit, int health, int dying)
{
    WeaponsHealth effect;
    effect.h = WeaponsHeaderOf(WEAPONS_EFFECT_HEALTH, unit);
    effect.health = health;
    effect.dying = dying;
    WeaponsPublishEffect(&effect, sizeof(effect));
}

inline void WeaponsPublishParalyze(Unit* unit, int ticks)
{
    WeaponsParalyze effect;
    effect.h = WeaponsHeaderOf(WEAPONS_EFFECT_PARALYZE, unit);
    effect.ticks = ticks;
    WeaponsPublishEffect(&effect, sizeof(effect));
}

inline int WeaponsFireScript(int index)
{
    return index >= 0 && index < 3 ? WEAPONS_SCRIPT_FIRE_PRIMARY + index : WEAPONS_SCRIPT_NONE;
}

inline int WeaponsAimScript(int index)
{
    return index >= 0 && index < 3 ? WEAPONS_SCRIPT_AIM_PRIMARY + index : WEAPONS_SCRIPT_NONE;
}

template <class U>
inline void WeaponsPublishStartScript(U* unit, int script, unsigned int callback, int update)
{
    WeaponsStartScript effect;
    memset(&effect, 0, sizeof(effect));
    effect.h = WeaponsHeaderOf(WEAPONS_EFFECT_START_SCRIPT, unit);
    effect.script = script;
    effect.callback = callback;
    effect.update = update;
    WeaponsPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void WeaponsPublishStartScriptWithArgs(U* unit, int script, unsigned int callback, int update, int count, int a, int b, int c, int d)
{
    WeaponsStartScript effect;
    effect.h = WeaponsHeaderOf(WEAPONS_EFFECT_START_SCRIPT, unit);
    effect.script = script;
    effect.callback = callback;
    effect.update = update;
    effect.withArgs = 1;
    effect.count = count;
    effect.args[0] = a;
    effect.args[1] = b;
    effect.args[2] = c;
    effect.args[3] = d;
    WeaponsPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void WeaponsPublishDetachPiece(U* unit, int piece, int pose)
{
    WeaponsDetachPiece effect;
    effect.h = WeaponsHeaderOf(WEAPONS_EFFECT_DETACH_PIECE, unit);
    effect.piece = piece;
    effect.pose = pose;
    WeaponsPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void WeaponsPublishNotice(U* unit, int bits)
{
    static const int kinds[3] = {WEAPONS_EFFECT_SHOT_FIRED, WEAPONS_EFFECT_COMMAND_SHOT_FIRED, WEAPONS_EFFECT_OUT_OF_REACH};
    for (int i = 0; i < 3; i++) {
        if (bits & 0x400 << i) {
            WeaponsEventHeader effect = WeaponsHeaderOf(kinds[i], unit);
            WeaponsPublishEffect(&effect, sizeof(effect));
        }
    }
}

template <class U>
inline void WeaponsPublishBusy(U* unit, int until)
{
    WeaponsBusy effect;
    effect.h = WeaponsHeaderOf(WEAPONS_EFFECT_BUSY, unit);
    effect.until = until;
    WeaponsPublishEffect(&effect, sizeof(effect));
}

#endif
