#ifndef GAME_WEAPONS_EVENTS_H
#define GAME_WEAPONS_EVENTS_H

#include <string.h>

#include "../../interfaces/weapons.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline WeaponsEvent WeaponsEventOf(int kind)
{
    WeaponsEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    return event;
}

template <class U>
inline WeaponsEvent WeaponsEventOf(int kind, U* unit)
{
    WeaponsEvent event = WeaponsEventOf(kind);
    event.unit = Ref<U>(unit).offset;
    return event;
}

inline void WeaponsPublishEffect(const WeaponsEvent& effect)
{
    HarnessWeaponsEvent(&effect, 1);
    SchedPublish(SCHED_WEAPONS, &effect, sizeof(effect));
}

inline void WeaponsPublishDamage(int outbox, Unit* source, Unit* victim, int amount, int type, unsigned short direction)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_DAMAGE, victim);
    event.target = Ref<Unit>(source).offset;
    event.value = amount;
    event.extra = type;
    event.args[0] = direction;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishRemoteDamage(int outbox, Unit* victim, Unit* attacker, int amount, int kind, unsigned char heading)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_REMOTE_DAMAGE, victim);
    event.target = Ref<Unit>(attacker).offset;
    event.value = amount;
    event.extra = kind;
    event.args[0] = heading << 8;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishDetonate(int outbox, Unit* unit, int selfDestruct)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_DETONATE, unit);
    event.value = selfDestruct;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishRelease(int outbox, Unit* unit, int index)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_RELEASE, unit);
    event.value = index;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishClaim(int outbox, Unit* unit, int index)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_CLAIM, unit);
    event.value = index;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishInitSlots(int outbox, Unit* unit)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_INIT_SLOTS, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishRemoveProjectiles(int outbox, Unit* unit)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_REMOVE_PROJECTILES, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class C>
inline void WeaponsPublishAimDone(int outbox, C* callback, int result)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_AIM_DONE);
    event.callback = Ref<C>(callback).offset;
    event.value = result;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void WeaponsPublishExplodePiece(int outbox, U* unit, int piece, unsigned int flags)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_EXPLODE_PIECE, unit);
    event.value = piece;
    event.extra = (int)flags;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishSetStockpiles(int outbox, Unit* unit, int first, int second, int third)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_SET_STOCKPILES, unit);
    event.args[0] = first;
    event.args[1] = second;
    event.args[2] = third;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishAddStockpile(int outbox, Unit* unit, int slot)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_ADD_STOCKPILE, unit);
    event.value = slot;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void WeaponsPublishLoadSlot(int outbox, Unit* unit, int slot, const int* args)
{
    WeaponsEvent event = WeaponsEventOf(WEAPONS_LOAD_SLOT, unit);
    event.value = slot;
    memcpy(event.args, args, sizeof(event.args));
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void WeaponsPublishAttacked(U* unit, Unit* attacker, int amount)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_ATTACKED, unit);
    effect.target = Ref<Unit>(attacker).offset;
    effect.value = amount;
    WeaponsPublishEffect(effect);
}

template <class U>
inline void WeaponsPublishHit(U* shooter, int enemyDamage, int friendlyDamage)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_HIT, shooter);
    effect.value = enemyDamage;
    effect.extra = friendlyDamage;
    WeaponsPublishEffect(effect);
}

inline void WeaponsPublishMarkDamaged(Unit* unit)
{
    WeaponsPublishEffect(WeaponsEventOf(WEAPONS_EFFECT_MARK_DAMAGED, unit));
}

inline void WeaponsPublishDamageRecord(Unit* unit, int kind, Unit* attacker)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_DAMAGE_RECORD, unit);
    effect.value = kind;
    effect.target = Ref<Unit>(attacker).offset;
    WeaponsPublishEffect(effect);
}

inline void WeaponsPublishHealth(Unit* unit, int health, int dying)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_HEALTH, unit);
    effect.value = health;
    effect.extra = dying;
    WeaponsPublishEffect(effect);
}

inline void WeaponsPublishParalyze(Unit* unit, int ticks)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_PARALYZE, unit);
    effect.value = ticks;
    WeaponsPublishEffect(effect);
}

template <class U>
inline void WeaponsPublishStartScript(U* unit, const char* name, unsigned int callback, int update)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_START_SCRIPT, unit);
    if (name != nullptr)
        strncpy(effect.name, name, sizeof(effect.name) - 1);
    effect.callback = callback;
    effect.args[4] = update;
    WeaponsPublishEffect(effect);
}

template <class U>
inline void WeaponsPublishStartScriptWithArgs(U* unit, const char* name, unsigned int callback, int update, int count, int a, int b, int c, int d)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_START_SCRIPT, unit);
    if (name != nullptr)
        strncpy(effect.name, name, sizeof(effect.name) - 1);
    effect.callback = callback;
    effect.extra = 1;
    effect.value = count;
    effect.args[0] = a;
    effect.args[1] = b;
    effect.args[2] = c;
    effect.args[3] = d;
    effect.args[4] = update;
    WeaponsPublishEffect(effect);
}

template <class U>
inline void WeaponsPublishDetachPiece(U* unit, int piece, int pose)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_DETACH_PIECE, unit);
    effect.value = piece;
    effect.extra = pose;
    WeaponsPublishEffect(effect);
}

template <class U>
inline void WeaponsPublishNotice(U* unit, int bits)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_NOTICE, unit);
    effect.value = bits;
    WeaponsPublishEffect(effect);
}

template <class U>
inline void WeaponsPublishBusy(U* unit, int until)
{
    WeaponsEvent effect = WeaponsEventOf(WEAPONS_EFFECT_BUSY, unit);
    effect.value = until;
    WeaponsPublishEffect(effect);
}

#endif
