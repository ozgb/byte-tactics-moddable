#ifndef SQUAD_H
#define SQUAD_H

#include "../state/sim_allocator.h"
#include "../../interfaces/ref.h"

struct Player;

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

static_assert(__builtin_offsetof(Squad, items) == 0x10, "Squad::items");
static_assert(sizeof(Squad) == 0x20, "Squad size");

#endif
