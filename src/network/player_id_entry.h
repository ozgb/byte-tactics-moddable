#ifndef PLAYER_ID_ENTRY_H
#define PLAYER_ID_ENTRY_H

#pragma pack(push, 1)
struct PlayerIdEntry {
    int id;
    char unknown_4[0x14b - 4];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(PlayerIdEntry) == 331, "PlayerIdEntry");
#endif

#endif
