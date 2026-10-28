#ifndef SQUAD_H
#define SQUAD_H

#include <vector>
#include "../state/sim_allocator.h"
#include "../../interfaces/ref.h"

struct Player;
struct Unit;

class Squad {
public:
    union { int field_0; Ref<Player> owner; };
    union { int field_4; int index; };
    int field_8;
    int field_c;
    SimVector<int> items;
    Squad(Squad&);
    Squad(Player*, int);
};

class UnitSwapEraseVector {
public:
    typedef std::vector<Unit*> UnitVector;
    UnitVector units;

    UnitVector::iterator EraseSwapBack(UnitVector::iterator where);
};

class IntDynArray {
public:
    std::vector<int> items;

    int EraseByValue(int value);
};

static_assert(__builtin_offsetof(Squad, items) == 0x10, "Squad::items");
static_assert(sizeof(Squad) == 0x20, "Squad size");

#endif
