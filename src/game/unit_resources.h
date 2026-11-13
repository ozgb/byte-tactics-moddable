#ifndef UNIT_RESOURCES_H
#define UNIT_RESOURCES_H

#include "../../interfaces/ref.h"

struct Player;

class UnitResources {
public:
    float energyMake;
    float energyUse;
    float energyUsePaid;
    float energyStall;
    float energyMakePrev;
    float energyUsePrev;
    float metalMake;
    float metalUse;
    float metalUsePaid;
    float metalStall;
    float metalMakePrev;
    float metalUsePrev;
    Ref<Player> player;
};

#endif
