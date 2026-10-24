#include "../../interfaces/ref.h"
#ifndef SQUAD_MANAGER_H
#define SQUAD_MANAGER_H

struct Player;
struct Unit;
struct Vec3;
struct Projectile;
class SquadTimer;

class SquadManager {
public:
    Ref<Player> player;
    unsigned char index;
    int countdown;
    int unused_9;
    int nextAction;
    Ref<SquadTimer> timers[10];
    Ref<Unit> cursor;

    SquadManager(Player* p);
    void MarkOwnerNetDirtyFromDamageSplit(Projectile* projectile, int a, int b);
    void AssignSquads();
    void RetargetWeapons(int force);
    void TickIfActive();
    void DeleteTimers();
    Unit* FindNearestEnemyUnit(int x, int y, int z);
    Unit* FindNearestEnemyUnit(Vec3 pos);
    void TickTimers();
};

#endif
