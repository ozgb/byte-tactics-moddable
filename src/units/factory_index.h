#ifndef UNITS_FACTORY_INDEX_H
#define UNITS_FACTORY_INDEX_H

#include "../state/block_vector.h"
#include "../../interfaces/ref.h"

struct Unit;
struct Vec3;

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
void __stdcall GetFactoriesInRadius(int player, const Vec3* pos, int radius, std::vector<Unit*>* out);
const BlockVector<Ref<Unit> >* FactoryIndexOf(int player);

#endif
