#ifndef UNITS_FACTORY_INDEX_H
#define UNITS_FACTORY_INDEX_H

#include <vector>
#include "../state/block_vector.h"
#include "../../interfaces/ref.h"

struct Unit;
struct Vec3;
struct StateWriter;

#pragma pack(push, 1)
struct FactoryIndexEntry {
    int live;
    unsigned int lastTick;
    BlockVector<Ref<Unit> > factories;
};

struct FactoryIndex {
    FactoryIndexEntry players[10];
};
#pragma pack(pop)

void __stdcall CreateFactoryIndex(int player);
void __stdcall FreeFactoryIndex(int player);
void __stdcall RefreshFactoryIndex(int player);
int __stdcall GetFactoriesInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity);
const BlockVector<Ref<Unit> >* FactoryIndexOf(int player);
void __stdcall UnitsWriteState(const StateWriter* w);

#endif
