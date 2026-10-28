#ifndef SUB_004679A0_B_H
#define SUB_004679A0_B_H

#pragma pack(push, 1)
struct LightBarSprite {
    char unknown_0[48];
    int field_30;
    unsigned short* lightbar;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(LightBarSprite) == 56, "Sub_004679a0_b");
#endif

#endif
