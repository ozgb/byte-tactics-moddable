#ifndef VIEW_FLAGS_H
#define VIEW_FLAGS_H

#pragma pack(push, 1)
struct MapViewFlags {
    union {
        unsigned short raw;
        unsigned char rawByte;
        struct {
            unsigned short bit0 : 1;
            unsigned short bit1 : 1;
            unsigned short bit2 : 1;
            unsigned short bit3 : 1;
            unsigned short rest : 12;
        };
    };
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(MapViewFlags) == 2, "MapViewFlags");
#endif

#endif
