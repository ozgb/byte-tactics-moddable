#ifndef FLAGS16_00499200_H
#define FLAGS16_00499200_H

#include "bit_flags16_00499200.h"

#pragma pack(push, 1)
union NetGameFlags {
    union { unsigned short value; NetGameFlagBits bits; };
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(NetGameFlags) == 2, "Flags16_00499200");
#endif

#endif
