#ifndef HOT_RADAR_SLOT_H
#define HOT_RADAR_SLOT_H

#pragma pack(push, 1)
struct HotRadarSlot {
    unsigned short unitId;
    int x;
    int y;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(HotRadarSlot) == 10, "HotRadarSlot");
#endif

#endif
