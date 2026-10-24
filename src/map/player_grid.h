#ifndef PLAYER_GRID_H
#define PLAYER_GRID_H

#include "../../interfaces/ref.h"

#pragma pack(push, 1)
struct PlayerGrid {
    Ref<unsigned char> cells;
    unsigned int width;
    unsigned int height;
    char unknown_c[319];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(PlayerGrid) == 331, "PlayerGrid");
#endif

#endif
