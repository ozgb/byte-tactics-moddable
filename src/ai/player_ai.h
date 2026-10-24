#ifndef PLAYER_AI_H
#define PLAYER_AI_H

#include "../state/sim_allocator.h"
#include "../../interfaces/ref.h"

struct Player;
struct Unit;
struct UnitDef;
struct Vec3;

struct Elem_0040cc40 {
    Point16 pos;
    float key;
    Elem_0040cc40() {}
    // FUNCTION: 0x40a5b0
    Elem_0040cc40(const Elem_0040cc40& o) : pos(o.pos), key(o.key) {}
    Elem_0040cc40(short x, short y, float k) { pos.x = x; pos.y = y; key = k; }
    bool operator<(const Elem_0040cc40& o) const { return key < o.key; }
};

struct Elem_0040cfb0 {
    char a;
    char b;
    char c;
};

struct Elem_0040d4f0 {
    unsigned char value;
};

struct Elem_0040d550 {
    int unknown_0;
};

struct Pos_00409160 {
    short x, y;
    int unknown_4;
    Pos_00409160(short ax, short ay) : unknown_4(0) { x = ax; y = ay; }
};

struct Vec3_00409160 {
    int x, y, z;
    Vec3_00409160(int ax, int ay, int az) : x(ax), y(ay), z(az) {}
    Vec3_00409160() { *this = Vec3_00409160(0, 0, 0); }
};

struct UnitList_00409160 {
    SimVector<Unit*> units;
    void Clear() { units.clear(); }
};

struct Group_00409160 {
    UnitList_00409160 list;
    void Clear() { list.Clear(); }
};

class PlayerAI {
public:
    Ref<Player> owner;
    unsigned char index;
    UnitList_00409160 visible;
    UnitList_00409160 known;
    Group_00409160 factories;
    Vec3_00409160 centre;
    Vec3_00409160 pos_41;
    SimVector<Elem_0040cc40> cells;
    Pos_00409160 center;
    SimVector<Elem_0040cfb0> vec_65;
    int builders;
    int hasSpecial;
    SimVector<short> counts;
    SimVector<unsigned char> vec_8d;
    SimVector<unsigned char> weights;
    SimVector<Elem_0040d4f0> vec_ad;
    SimVector<Elem_0040d550> vec_bd;
    SimVector<Elem_0040d550> values;
    SimVector<Elem_0040d550> locked;
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
    bool FindCellNearFeatures(UnitDef* type, Vec3* pos, SimVector<Elem_0040cc40>* list, int range, Point16* out);
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
