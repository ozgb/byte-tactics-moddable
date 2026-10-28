#ifndef MISSION_OPTION_FLAGS_H
#define MISSION_OPTION_FLAGS_H

#pragma pack(push, 1)
struct MissionOptionFlags {
    int value;
    int flag_4;
    int flag_8;
    int flag_c;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(MissionOptionFlags) == 16, "MissionOptionFlags");
#endif

#endif
