#ifndef GRAPHICS_OPTION_FLAGS_H
#define GRAPHICS_OPTION_FLAGS_H

#pragma pack(push, 1)
struct GraphicsOptionFlags {
    unsigned short b0 : 1;
    unsigned short b1_6 : 6;
    unsigned short b7 : 1;
    unsigned short b8 : 1;
    unsigned short rest : 7;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(GraphicsOptionFlags) == 2, "GraphicsOptionFlags");
#endif

#endif
