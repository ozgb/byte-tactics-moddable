#ifndef SIDE_NAME_VIEW_H
#define SIDE_NAME_VIEW_H

#pragma pack(push, 1)
struct SideNameView {
    char name[0x232];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(SideNameView) == 562, "SideNameView");
#endif

#endif
