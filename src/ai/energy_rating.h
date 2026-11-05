#ifndef AI_ENERGY_RATING_H
#define AI_ENERGY_RATING_H

#include <minmax.h>

struct UnitDef;

inline int ComputeEnergyRating(float energyCost, double(__stdcall* getEnergyUse)(UnitDef*), UnitDef* def)
{
#ifdef __clang__
    volatile float cost = energyCost * -0.0025f;
    return (char)max(0.0f, min(100.0f, cost - getEnergyUse(def) * 5.0f));
#else
    return (char)max(0.0f, min(100.0f, energyCost * -0.0025f - getEnergyUse(def) * 5.0f));
#endif
}

#endif
