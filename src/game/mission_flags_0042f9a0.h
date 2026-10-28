#ifndef MISSION_FLAGS_0042F9A0_H
#define MISSION_FLAGS_0042F9A0_H

#pragma pack(push, 1)
struct AllMissionsFlags {
    unsigned short allMissions : 1;
    unsigned short : 15;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(AllMissionsFlags) == 2, "MissionFlags_0042f9a0");
#endif

#endif
