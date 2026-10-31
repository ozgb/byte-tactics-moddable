#ifndef SQUAD_H
#define SQUAD_H

#include <vector>
#include "../state/block_vector.h"
#include "../../interfaces/ref.h"

struct Player;
struct Unit;

class Squad {
public:
    union { int field_0; Ref<Player> owner; };
    union { int field_4; int index; };
    int field_8;
    int field_c;
    BlockVector<Ref<Unit> > items;
    Squad(Squad&);
    Squad(Player*, int);
};

class UnitSwapEraseVector {
public:
    typedef std::vector<Unit*> UnitVector;
    UnitVector units;

    // FUNCTION: 0x4800c0
    UnitVector::iterator EraseSwapBack(UnitVector::iterator where)
    {
        UnitVector::iterator last = units.end() - 1;
        *where = *last;
        units.erase(last);
        return where;
    }
};

class IntDynArray {
public:
    std::vector<int> items;

    int EraseByValue(int value);
};

static_assert(__builtin_offsetof(Squad, items) == 0x10, "Squad::items");
static_assert(sizeof(Squad) == 0x20, "Squad size");

#endif
