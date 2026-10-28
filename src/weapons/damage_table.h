#ifndef DAMAGE_TABLE_H
#define DAMAGE_TABLE_H

#include <vector>

struct DamageTableEntry;

#pragma pack(push, 1)

struct Table_00499cd0 {
    char less;
    std::vector<DamageTableEntry> v;

    DamageTableEntry* LowerBound(const char* key);
};

#pragma pack(pop)

#endif
