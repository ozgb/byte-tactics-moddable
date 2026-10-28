#ifndef NET_GAME_FLAGS_H
#define NET_GAME_FLAGS_H

#include "net_game_flag_bits.h"

#pragma pack(push, 1)
union NetGameFlags {
    union { unsigned short value; NetGameFlagBits bits; };
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(NetGameFlags) == 2, "NetGameFlags");
#endif

#endif
