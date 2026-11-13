#ifndef GAME_ECONOMY_EVENTS_H
#define GAME_ECONOMY_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline EconUnit EconHandleOf(Unit* unit)
{
    return Ref<Unit>(unit).offset;
}

inline EconomyEvent EconEvent(int kind, int player, EconUnit unit, unsigned int resources, float energy, float metal)
{
    EconomyEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.player = player;
    event.unit = unit;
    event.resources = resources;
    event.energy = energy;
    event.metal = metal;
    return event;
}

inline void EconPublish(int outbox, int kind, int player, EconUnit unit, unsigned int resources, float energy, float metal)
{
    EconomyEvent event = EconEvent(kind, player, unit, resources, energy, metal);
    SchedPublish(outbox, &event, sizeof(event));
}

inline void EconPublishLoad(int outbox, int player, EconUnit unit, const EconAccount& account)
{
    EconomyEvent event = EconEvent(ECONOMY_LOAD, player, unit, 0, 0, 0);
    event.account = account;
    SchedPublish(outbox, &event, sizeof(event));
}

inline int EconSpend(int player, EconUnit unit, unsigned int resources, float energy, float metal)
{
    EconomyEvent event = EconEvent(ECONOMY_SPEND, player, unit, resources, energy, metal);
    return ScheduleEconomySpend(&event);
}

template <class U>
inline int EconRequest(int outbox, U* unit, unsigned int resources, float energy, float metal)
{
    int granted = ScheduleEconomyRequestGranted(EconHandleOf(unit), resources);
    EconPublish(outbox, ECONOMY_REQUEST, unit->playerIndex, EconHandleOf(unit), resources, energy, metal);
    return granted;
}

template <class U>
inline int EconRequestEnergy(int outbox, U* unit, float energy)
{
    return EconRequest(outbox, unit, ECON_ENERGY, energy, 0);
}

template <class U>
inline int EconRequestEnergyAndMetal(int outbox, U* unit, float energy, float metal)
{
    return EconRequest(outbox, unit, ECON_ENERGY | ECON_METAL, energy, metal);
}

#endif
