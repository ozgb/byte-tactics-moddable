#ifndef OPTIONS_SLIDER_MENU_H
#define OPTIONS_SLIDER_MENU_H

struct OptionsSliderLayer;

#pragma pack(push, 1)
struct OptionsSliderMenu {
    char unknown_0[0x18];
    OptionsSliderLayer* holder;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(OptionsSliderMenu) == 28, "OptionsSliderMenu");
#endif

#endif
