#ifndef MAIN2_MENU_H
#define MAIN2_MENU_H

#pragma pack(push, 1)
struct Main2Menu {
    int field_51d;
    char unknown_8[0x18 - 0x8];
    void* data;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Main2Menu) == 24, "Main2Menu");
#endif

#endif
