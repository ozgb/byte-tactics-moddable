#ifndef VIEW_FLAGS_H
#define VIEW_FLAGS_H

#pragma pack(push, 1)
struct MapViewFlags {
    unsigned short bit0 : 1;
    unsigned short bit1 : 1;
    unsigned short bit2 : 1;
    unsigned short rest : 13;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(MapViewFlags) == 2, "MapViewFlags");
#endif

#endif
