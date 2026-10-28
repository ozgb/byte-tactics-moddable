#ifndef SNAPSHOT_0046A860_H
#define SNAPSHOT_0046A860_H

#pragma pack(push, 1)
struct UnitInfoSnapshot {
    int orderName;
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
static_assert(sizeof(UnitInfoSnapshot) == 60, "Snapshot_0046a860");
#endif

#endif
