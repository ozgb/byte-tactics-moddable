#ifndef NET_GAME_FLAG_LOW_BITS_H
#define NET_GAME_FLAG_LOW_BITS_H

#pragma pack(push, 1)
struct NetGameFlagLowBits {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short b3 : 1;
    unsigned short b4 : 1;
    unsigned short : 11;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(NetGameFlagLowBits) == 2, "NetGameFlagLowBits");
#endif

#endif
