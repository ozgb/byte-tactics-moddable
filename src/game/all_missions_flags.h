#ifndef ALL_MISSIONS_FLAGS_H
#define ALL_MISSIONS_FLAGS_H

#pragma pack(push, 1)
struct AllMissionsFlags {
    unsigned short allMissions : 1;
    unsigned short : 15;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(AllMissionsFlags) == 2, "AllMissionsFlags");
#endif

#endif
