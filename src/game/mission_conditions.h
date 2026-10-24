#ifndef MISSION_CONDITIONS_H
#define MISSION_CONDITIONS_H

#include "../../interfaces/rand_stream.h"
#include "../../interfaces/ref.h"

class HapiBank;
struct Param_0048e010;
struct Unit;

class MissionCondition {
public:
    int satisfied;
    int celebrated;

    MissionCondition() { satisfied = celebrated = 0; }
    virtual int IsSatisfied();
    virtual void OnUnitDied(Unit* unit);
    virtual void OnUnitCaptured(Unit* unit);
    virtual void OnUnitCreated(Unit* unit);
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

class MissionConditions {
public:
    Ref<MissionCondition> victory[16];
    int victoryCount;
    Ref<MissionCondition> defeat[16];
    int defeatCount;
    int active;

    MissionConditions();
    ~MissionConditions() { FreeConditions(); }
    void FreeConditions();
    void RegisterConditions(Param_0048e010* p);
    void SaveConditions(HapiBank* file);
    void LoadConditions(HapiBank* file);
    int AllVictoryConditionsMet();
    int AnyDefeatConditionMet();
    int CheckVictory();
    int CheckDefeat(RandStream stream);
    void Deactivate();
    void NotifyUnitDied(Unit* unit);
    void NotifyUnitCaptured(Unit* unit);
    void NotifyUnitCreated(Unit* unit);
};

#endif
