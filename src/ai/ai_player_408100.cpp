#include "../state/sim_allocator.h"
#include "../../interfaces/ref.h"
#include "win_types.h"
#include <ta_types.h>
#include <time.h>
#include <memory.h>
#include <math.h>

#pragma pack(push, 1)
struct UnitDef_00408100 {
    char unknown_0[0x152];
    int count;
    char unknown_156[0x245 - 0x156];
    unsigned int unknown_bits0 : 12;
    unsigned int flag12 : 1;
    unsigned int unknown_bits13 : 19;
};

struct Order_00408100 {
    char unknown_0[0x42];
    unsigned int flags;
};

struct Unit_00408100 {
    char unknown_0[0x5c];
    Ref<Order_00408100> list;
    char unknown_60[0x6a - 0x60];
    Vec3 pos;
    char unknown_76[0x92 - 0x76];
    Ref<UnitDef_00408100> def;
};

struct Item_00408100 {
    char unknown_0[0x249];
};

struct Game_00408100 {
    char unknown_0[0x1422b];
    int mapPixelWidth;
    int mapPixelHeight;
    char unknown_14233[0x1439b - 0x14233];
    Ref<Item_00408100> unitDefs;
    char pad_1439f[0x38a47 - 0x1439f];
    unsigned int gameTick;
};
#pragma pack(pop)

struct Group_00408100 {
    char unknown_0[0x10];
    std::vector<Unit_00408100*> units;
};



extern Game_00408100* g_game;

void __stdcall GetBasePosition(int index, Vec3* out);
unsigned short __stdcall ChooseBuildOption(unsigned int player, Unit_00408100* unit);
int __stdcall FindBuildPosition(unsigned int player, Vec3* from, Item_00408100* item, Vec3* out);
int __stdcall GetBuilderCount(unsigned int player);
#include "../../interfaces/seams.h"
#include "../util/math_4b6c30.h"

int __cdecl ScaledSin(short angle, int scale);
int __cdecl ScaledCos(short angle, int scale);

static inline int Length(const Vec3& v)
{
    double x = v.x;
    double y = v.y;
    double z = v.z;
    double sq = x * x + y * y + z * z;
    int len = (int)sqrt(sq);
    return len;
}

static inline Vec3 Direction(short angle, int scale)
{
    Vec3 v;
    v.x = -ScaledSin(angle, scale);
    v.y = 0;
    v.z = -ScaledCos(angle, scale);
    return v;
}

static inline int FixMul(int a, int b)
{
    return (int)(((__int64)a * b) >> 16);
}

static inline int FixDiv(int a, int b)
{
    return (int)(((__int64)a << 16) / b);
}

static inline int MapRange()
{
    return (g_game->mapPixelWidth + g_game->mapPixelHeight) / 3 << 16;
}

// FUNCTION: 0x408100
void ScoutTimer::OnTimer()
{
    next = g_game->gameTick + 90;
    Vec3 origin;
    GetBasePosition(player, &origin);
    std::vector<Unit_00408100*>::iterator it;
    for (it = ((Group_00408100*)group.get())->units.begin(); it != ((Group_00408100*)group.get())->units.end(); ++it) {
        Unit_00408100* u = *it;
        if (u->def->count
            && (!(unsigned char)u->def->flag12
                || (GetBuilderCount(player) < 5 && g_game->gameTick >= (unsigned int)owner->nextAction))
            && (!u->list || !(u->list->flags & 8))) {
            unsigned short idx = ChooseBuildOption(player, u);
            if (idx) {
                Vec3 pos;
                pos.x = 0;
                pos.y = 0;
                pos.z = 0;
                int ok = FindBuildPosition(player, &u->pos, &g_game->unitDefs[idx], &pos);
                if (u->def->flag12) {
                    int range = MapRange();
                    origin.y = pos.y;
                    int len = Length(pos - origin);
                    if (len > range || len > range)
                        ok = 0;
                }
                if (ok) {
                    MissionType kind = g_seams.orders->GetOrderType(0xe, (Unit*)u, 0, &pos);
                    g_seams.orders->AddOrder(kind, 0, (Unit*)u, 0, &pos, idx, 1);
                }
            }
        }
    }
    for (it = ((Group_00408100*)group.get())->units.begin(); it != ((Group_00408100*)group.get())->units.end(); ++it) {
        Unit_00408100* u = *it;
        Vec3 target;
        if ((!u->list || (u->list->flags & 0x4000))
            && (!(unsigned char)u->def->flag12 || GetBuilderCount(player) >= 5)) {
            target = origin;
            if (u->def->flag12) {
                origin.y = u->pos.y;
                Vec3 d = origin - u->pos;
                if (Length(d) > 0x2800000)
                    d = Direction(RandomInt(0x10000), 0x2800000);
                target = origin + d;
                MissionType kind;
                kind = g_seams.orders->GetOrderType(2, (Unit*)u, 0, &target);
                g_seams.orders->AddOrder(kind, 0, (Unit*)u, 0, &target, 0, 0);
                kind = g_seams.orders->GetOrderType(9, (Unit*)u, 0, &origin);
                g_seams.orders->AddOrder(kind, 1, (Unit*)u, 0, &origin, 0, 0);
            } else {
                Vec3 d = origin - u->pos;
                int len = Length(d);
                if (len < 0x1400000) {
                    if (len < 0x100000) {
                        d = Direction(RandomInt(0x10000), 0x1400000);
                    } else {
                        int s = FixDiv(0x1400000, len);
                        d.x = FixMul(s, d.x);
                        d.y = FixMul(d.y, s);
                        d.z = FixMul(d.z, s);
                    }
                    target = u->pos + d;
                }
                MissionType kind = g_seams.orders->GetOrderType(9, (Unit*)u, 0, &target);
                g_seams.orders->AddOrder(kind, 0, (Unit*)u, 0, &target, 0, 0);
            }
        }
    }
}
