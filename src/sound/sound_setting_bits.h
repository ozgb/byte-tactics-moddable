#ifndef SOUND_SETTING_BITS_H
#define SOUND_SETTING_BITS_H

#pragma pack(push, 1)
struct SoundSettingBits {
    char unknown_0[2];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(SoundSettingBits) == 2, "SoundSettingBits");
#endif

#endif
