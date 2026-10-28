#ifndef LOADING_PLAYER_SLOTS_H
#define LOADING_PLAYER_SLOTS_H

#pragma pack(push, 1)
struct LoadingPlayerSlots {
    int inGame[10];
    int unknown_28;
    int flag40[10];
    char unknown_54[56];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(LoadingPlayerSlots) == 140, "LoadingPlayerSlots");
#endif

#endif
