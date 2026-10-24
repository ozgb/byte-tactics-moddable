#ifndef UNIT_RESOURCES_H
#define UNIT_RESOURCES_H

#include "../../interfaces/ref.h"

struct Player;
struct UnitInfo;
class HapiBank;

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

    void SaveUnitAccounts(UnitInfo* info, HapiBank* file);
    void LoadUnitAccounts(UnitInfo* info, HapiBank* file);
    void Reset(unsigned char playerIndex);
    int RequestEnergy(UnitResources* r, float amount);
    int RequestEnergyAndMetal(float dx, float dy);
    int SpendEnergy(float amount);
    int SpendMetal(float amount);
    int SpendEnergyAndMetal(float energy, float metal);
};

#endif
