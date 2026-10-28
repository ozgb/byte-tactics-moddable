#ifndef LIGHT_BAR_SPRITE_H
#define LIGHT_BAR_SPRITE_H

#pragma pack(push, 1)
struct LightBarSprite {
    char unknown_0[48];
    int field_30;
    unsigned short* lightbar;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(LightBarSprite) == 56, "LightBarSprite");
#endif

#endif
