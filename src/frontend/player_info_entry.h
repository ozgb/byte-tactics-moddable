#ifndef PLAYER_INFO_ENTRY_H
#define PLAYER_INFO_ENTRY_H

#include "../../interfaces/ref.h"

struct PlayerInfo;

#pragma pack(push, 1)
struct PlayerInfoEntry {
    Ref<PlayerInfo> info;
    char unknown_4[0x14b - 4];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(PlayerInfoEntry) == 331, "PlayerInfoEntry");
#endif

#endif
