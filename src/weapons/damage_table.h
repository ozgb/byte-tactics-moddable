#ifndef DAMAGE_TABLE_H
#define DAMAGE_TABLE_H

#include "../map/los_tables.h"
#include "../state/block_vector.h"

#pragma pack(push, 1)

struct DamageTable {
    char less;
    BlockVector<DamageTableEntry> v;

    DamageTableEntry* LowerBound(const char* key);
};

#pragma pack(pop)

#endif
