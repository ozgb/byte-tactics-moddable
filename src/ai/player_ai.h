#ifndef PLAYER_AI_H
#define PLAYER_AI_H

#include "../state/sim_allocator.h"
#include "../../interfaces/ref.h"

struct Player;
struct Unit;
struct UnitDef;
struct Vec3;

struct KeyedCell {
    Point16 pos;
    float key;
    KeyedCell() {}
    // FUNCTION: 0x40a5b0
    KeyedCell(const KeyedCell& o) : pos(o.pos), key(o.key) {}
    KeyedCell(short x, short y, float k) { pos.x = x; pos.y = y; key = k; }
    bool operator<(const KeyedCell& o) const { return key < o.key; }
};

struct BuildRatingWeights {
    char a;
    char b;
    char c;
};

struct PercentByte {
    unsigned char value;
};

struct UnitTypeInt {
    int unknown_0;
};

struct CenterPos {
    short x, y;
    int unknown_4;
    CenterPos(short ax, short ay) : unknown_4(0) { x = ax; y = ay; }
};

struct AiVec3 {
    int x, y, z;
    AiVec3(int ax, int ay, int az) : x(ax), y(ay), z(az) {}
    AiVec3() { *this = AiVec3(0, 0, 0); }
};

struct AiUnitList {
    SimVector<Unit*> units;
    void Clear() { units.clear(); }
};

struct FactoryUnitGroup {
    AiUnitList list;
    void Clear() { list.Clear(); }
};

class PlayerAI {
public:
    Ref<Player> owner;
    unsigned char index;
    AiUnitList visible;
    AiUnitList known;
    FactoryUnitGroup factories;
    AiVec3 centre;
    AiVec3 pos_41;
    SimVector<KeyedCell> cells;
    CenterPos center;
    SimVector<BuildRatingWeights> vec_65;
    int builders;
    int hasSpecial;
    SimVector<short> counts;
    SimVector<unsigned char> vec_8d;
    SimVector<unsigned char> weights;
    SimVector<PercentByte> vec_ad;
    SimVector<UnitTypeInt> vec_bd;
    SimVector<UnitTypeInt> values;
    SimVector<UnitTypeInt> locked;
    unsigned int lastTick;
    Point16 spacing0;
    Point16 offset0;
    int margin0;
    Point16 spacing1;
    Point16 offset1;
    int margin1;
    int searchRadius;

    PlayerAI(unsigned char player);
    void InitUnitTables();
    void InitPlacementGrid();
    void ComputeBaseWeights();
    bool FindCellNearFeatures(UnitDef* type, Vec3* pos, SimVector<KeyedCell>* list, int range, Point16* out);
    bool FindRandomPlacementCell(UnitDef* type, Vec3* pos, int range, Point16* out);
    void BuildFeatureCells();
    void RefreshUnitLists();
    void UpdateEveryThirtyTicks();
};

#define PLAYER_AI_OFFSET(field, off) \
    static_assert(__builtin_offsetof(PlayerAI, field) == off, "PlayerAI::" #field)
PLAYER_AI_OFFSET(visible, 0x05);
PLAYER_AI_OFFSET(known, 0x15);
PLAYER_AI_OFFSET(factories, 0x25);
PLAYER_AI_OFFSET(cells, 0x4d);
PLAYER_AI_OFFSET(vec_65, 0x65);
PLAYER_AI_OFFSET(counts, 0x7d);
PLAYER_AI_OFFSET(vec_8d, 0x8d);
PLAYER_AI_OFFSET(weights, 0x9d);
PLAYER_AI_OFFSET(vec_ad, 0xad);
PLAYER_AI_OFFSET(vec_bd, 0xbd);
PLAYER_AI_OFFSET(values, 0xcd);
PLAYER_AI_OFFSET(locked, 0xdd);
#undef PLAYER_AI_OFFSET
static_assert(sizeof(PlayerAI) == 0x10d, "PlayerAI size");

#endif
