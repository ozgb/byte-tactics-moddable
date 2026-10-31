#ifndef FEATURE_ANIM_POOL_H
#define FEATURE_ANIM_POOL_H
#include "../../interfaces/ref.h"

struct UnitDef;

#pragma pack(push, 1)
struct FeatureAnimEntry {
    short next;
    short prev;
    char unknown_4[0x2c];
};

struct FeaturePseudoUnit {
    char unknown_0[0x92];
    Ref<UnitDef> def;
    char unknown_96[0xa6 - 0x96];
    unsigned short unitDefIndex;
    char unknown_a8[0x110 - 0xa8];
    int flags_110;
    int flags_114;
};

struct FeatureAnimPool {
    char unknown_0[16];
    Ref<FeatureAnimEntry> anim;
    Ref<FeaturePseudoUnit> featureUnit;
    int activeHead;
    int idleHead;
    int freeHead;
    char unknown_24[52];
    int nameCount;
    char unknown_5c[24];
    Ref<char[256]> names;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(FeatureAnimPool) == 120, "FeatureAnimPool");
#endif

#endif
