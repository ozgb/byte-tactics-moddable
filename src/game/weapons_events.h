#ifndef GAME_WEAPONS_EVENTS_H
#define GAME_WEAPONS_EVENTS_H

#include <string.h>

#include "../../interfaces/weapons.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline WeaponsEffect WeaponsEffectOf(int kind)
{
    WeaponsEffect effect;
    memset(&effect, 0, sizeof(effect));
    effect.kind = kind;
    return effect;
}

template <class U>
inline void WeaponsPublishAttacked(U* unit, Unit* attacker, int amount)
{
    WeaponsEffect effect = WeaponsEffectOf(WEAPONS_EFFECT_ATTACKED);
    effect.unit = Ref<U>(unit).offset;
    effect.target = Ref<Unit>(attacker).offset;
    effect.value = amount;
    SchedPublish(SCHED_WEAPONS, &effect, sizeof(effect));
}

template <class U>
inline void WeaponsPublishHit(U* shooter, int enemyDamage, int friendlyDamage)
{
    WeaponsEffect effect = WeaponsEffectOf(WEAPONS_EFFECT_HIT);
    effect.unit = Ref<U>(shooter).offset;
    effect.value = enemyDamage;
    effect.extra = friendlyDamage;
    SchedPublish(SCHED_WEAPONS, &effect, sizeof(effect));
}

#endif
