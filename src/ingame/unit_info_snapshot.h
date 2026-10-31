#ifndef UNIT_INFO_SNAPSHOT_H
#define UNIT_INFO_SNAPSHOT_H

#pragma pack(push, 1)
struct UnitInfoSnapshot {
    const char* orderName;
    unsigned short selected;
    unsigned short health;
    unsigned short build;
    int weapons[3];
    float values[4];
    unsigned short targetType;
    unsigned short targetHealth;
    unsigned short feature;
    int button;
    int width;
    int height;
    int reserved;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(UnitInfoSnapshot) == 60, "UnitInfoSnapshot");
#endif

#endif
