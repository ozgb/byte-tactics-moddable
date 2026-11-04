#ifndef DAMAGED_ALLY_COLLECTOR_H
#define DAMAGED_ALLY_COLLECTOR_H
#include <vector>
#include "../../interfaces/map.h"

struct Player;
struct Unit;

class DamagedAllyCollector : public UnitVisitor {
public:
    Player* owner;
    std::vector<Unit*>* units;
    Unit* self;
    DamagedAllyCollector(Player* o, std::vector<Unit*>* v, Unit* s) : owner(o), units(v), self(s) {}
    virtual void Visit(Unit* unit);
};

#endif
