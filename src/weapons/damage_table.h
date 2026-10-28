#ifndef DAMAGE_TABLE_H
#define DAMAGE_TABLE_H

#include <vector>

struct DamageTableEntry;

#pragma pack(push, 1)

struct DamageTable {
    char less;
    std::vector<DamageTableEntry> v;

    DamageTableEntry* LowerBound(const char* key);
};

#pragma pack(pop)

#endif
