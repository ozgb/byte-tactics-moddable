#ifndef LIGHT_BAR_COUNTERS_H
#define LIGHT_BAR_COUNTERS_H

#pragma pack(push, 1)
struct LightBarCounters {
    char unknown_0[1];
    int field_1;
    char unknown_5[8];
    int field_d;
    char unknown_11[8];
    int field_19;
    int field_1d;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(LightBarCounters) == 33, "LightBarCounters");
#endif

#endif
