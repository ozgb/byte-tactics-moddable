#ifndef SOUND_SETTING_FLAGS_H
#define SOUND_SETTING_FLAGS_H

#include "sound_setting_bits.h"

#pragma pack(push, 1)
union SoundSettingFlags {
    union { unsigned short word; SoundSettingBits bits; };
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(SoundSettingFlags) == 2, "SoundSettingFlags");
#endif

#endif
