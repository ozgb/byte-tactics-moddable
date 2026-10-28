#ifndef BROADCAST_TOGGLE_FLAGS_H
#define BROADCAST_TOGGLE_FLAGS_H

#pragma pack(push, 1)
struct BroadcastToggleFlags {
    unsigned short b0 : 1;
    unsigned short rest : 15;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(BroadcastToggleFlags) == 2, "BroadcastToggleFlags");
#endif

#endif
