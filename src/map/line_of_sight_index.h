#ifndef LINE_OF_SIGHT_INDEX_H
#define LINE_OF_SIGHT_INDEX_H

#include <vector>
#include "../state/block_vector.h"
#include "../../interfaces/ref.h"

struct Player;
struct Unit;
struct Vec3;
struct StateWriter;

#pragma pack(push, 1)
struct LosUnitIndexEntry {
    int live;
    unsigned int lastTick;
    int hasSpecial;
    BlockVector<Ref<Unit> > visible;
    BlockVector<Ref<Unit> > known;
};

struct LosUnitIndex {
    LosUnitIndexEntry players[10];
};
#pragma pack(pop)

void __stdcall CreateUnitIndex(int player);
void __stdcall FreeUnitIndex(int player);
void __stdcall RefreshUnitIndex(int player);
int __stdcall GetVisibleEnemiesInRadius(int player, const Vec3* pos, int radius, int flags, Ref<Unit>* out, int capacity);
int __stdcall HasReadyUnitInRange(int player, const Vec3* p, int range);
int __stdcall IsPointVisible(Player* player, Vec3* point);
int __stdcall GetSeenUnitsInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity);
const LosUnitIndexEntry* LosUnitIndexOf(int player);
void WriteLosUnitIndex(const StateWriter* w);

#endif
