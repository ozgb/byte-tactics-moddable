#ifndef DAMAGED_ALLY_COLLECTOR_H
#define DAMAGED_ALLY_COLLECTOR_H
#include <vector>

struct Player;
struct Unit;

class DamagedAllyCollector {
public:
    Player* owner;
    std::vector<Unit*>* units;
    Unit* self;
    DamagedAllyCollector(Player* o, std::vector<Unit*>* v, Unit* s) : owner(o), units(v), self(s) {}
    void Visit(Unit* unit);
};

#endif
