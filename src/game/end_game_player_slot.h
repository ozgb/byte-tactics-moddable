#ifndef END_GAME_PLAYER_SLOT_H
#define END_GAME_PLAYER_SLOT_H

#pragma pack(push, 1)
struct EndGamePlayerSlot {
    unsigned char active;
    char unknown_1[29];
    int stats[7];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(EndGamePlayerSlot) == 58, "EndGamePlayerSlot");
#endif

#endif
