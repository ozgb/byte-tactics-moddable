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

inline int EconEmit(int kind, int player, EconUnit unit, unsigned int resources, float energy, float metal, const EconAccount* account = nullptr)
{
    EconomyEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.player = player;
    event.unit = unit;
    event.resources = resources;
    event.energy = energy;
    event.metal = metal;
    event.account = account;
    return ScheduleEconomyEvent(&event);
}

template <class U>
inline int EconRequest(U* unit, unsigned int resources, float energy, float metal)
{
    int granted = g_seams.economy->RequestGranted(EconHandleOf(unit), resources);
    EconEmit(ECONOMY_REQUEST, unit->playerIndex, EconHandleOf(unit), resources, energy, metal);
    return granted;
}

template <class U>
inline int EconRequestEnergy(U* unit, float energy)
{
    return EconRequest(unit, ECON_ENERGY, energy, 0);
}

template <class U>
inline int EconRequestEnergyAndMetal(U* unit, float energy, float metal)
{
    return EconRequest(unit, ECON_ENERGY | ECON_METAL, energy, metal);
}

#endif
