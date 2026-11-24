#ifndef GAME_ECONOMY_EVENTS_H
#define GAME_ECONOMY_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/ref.h"
#include "../util/engine_services.h"

struct Unit;

inline EconUnit EconHandleOf(Unit* unit)
{
    return Ref<Unit>(unit).offset;
}

inline EconomyEventHeader EconHeader(int kind, int player, EconUnit unit)
{
    EconomyEventHeader header;
    header.kind = kind;
    header.player = player;
    header.unit = unit;
    return header;
}


inline void EconPublish(int outbox, int kind, int player, EconUnit unit, unsigned int resources, float energy, float metal)
{
    if (kind == ECONOMY_RESET) {
        EconomyEventHeader event = EconHeader(kind, player, unit);
        SchedPublish(outbox, &event, sizeof(event));
        return;
    }
    EconomyIncome event;
    event.h = EconHeader(kind, player, unit);
    event.resources = resources;
    event.energy = energy;
    event.metal = metal;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void EconPublishLoad(int outbox, int player, EconUnit unit, const EconAccount& account)
{
    EconomyLoad event;
    event.h = EconHeader(ECONOMY_LOAD, player, unit);
    event.account = account;
    SchedPublish(outbox, &event, sizeof(event));
}

inline int EconSpend(int player, EconUnit unit, unsigned int resources, float energy, float metal)
{
    struct EconomySpend event;
    event.h = EconHeader(ECONOMY_SPEND, player, unit);
    event.resources = resources;
    event.energy = energy;
    event.metal = metal;
    return ScheduleEconomySpend(&event);
}

template <class U>
inline int EconRequest(int outbox, U* unit, unsigned int resources, float energy, float metal)
{
    int granted = ScheduleEconomyRequestGranted(EconHandleOf(unit), resources);
    EconomyRequest event;
    event.h = EconHeader(ECONOMY_REQUEST, unit->playerIndex, EconHandleOf(unit));
    event.resources = resources;
    event.energy = energy;
    event.metal = metal;
    SchedPublish(outbox, &event, sizeof(event));
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
