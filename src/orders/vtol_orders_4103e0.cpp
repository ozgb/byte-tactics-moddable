#include <vector>
#include "vtol_orders_4103e0.h"
struct Unit;
namespace std {
template<> class vector<Unit*,allocator<Unit*> > {
    unsigned char alloc; Unit** first; Unit** last; Unit** endStorage;
public:
    ~vector();
    unsigned int size() const;
    unsigned int count() const { return begin()==0 ? 0 : end()-begin(); }
    bool empty() const { return size()==0; }
    Unit** begin() const { return first; }
    Unit** end() const { return last; }
    Unit*& operator[](unsigned int n) { return *(begin()+n); }
};
}
#include "../state/block.h"
#include "../util/vec3.h"
struct Order;
#include "mission_type.h"

#pragma pack(push, 1)
#include "../weapons/weapon_def.h"
#include "../units/unit_def.h"
struct Owner { char pad0[0x108]; unsigned char allied[0x3e]; unsigned char index; };
#include "unit_motion.h"
class BitWriter;
class OpenHeap;
class ScoutTimer;
class EscortTimer;
class AssaultTimer;
class SquadManager;
#include "../units/unit.h"

#include "../../interfaces/seams.h"
#include "order.h"

#include "path_fx.h"
#include "../game/game.h"
#pragma pack(pop)
#include "../../link/globals.h"
class Class_0044e730 { public: void SetApproachRadius(int); };
class Class_0044e6c0 { public: void SetAltitude(int); };
short __stdcall GetHeadingBetween(Vec3*, Vec3*);
union Fixed { int value; struct { unsigned short frac; short whole; } parts; };
#include "vtol_orders.h"
static inline Vec3 Direction(short angle, int range) { Fixed distance; distance.value=range; return DirectionFromAngle(angle,distance); }
Vec3 __stdcall AddVec3(const Vec3&, const Vec3&);
#include "order_list.h"

MissionType __stdcall GetOrderType(unsigned char, Unit*, Unit*, int);
#include "../util/math_4b6c30.h"
int __cdecl ScaledSin(short, int);
int __cdecl ScaledCos(short, int);
static inline int Contains(unsigned int* bits, unsigned short index) { return bits[index >> 5] & (1 << (index & 31)); }
static inline Vec3 Offset(short angle, int distance) { Vec3 v; v.x=-ScaledSin(angle,distance); v.y=0; v.z=-ScaledCos(angle,distance); return v; }

class LandingPadList : public std::vector<Unit*> { public: LandingPadList(); };
void __stdcall AddOrder(int kind, int remove, Unit* owner, void* id, Vec3* pos, int param_6, int param_7);
// FUNCTION: 0x4103e0
int __stdcall VtolSeekAttackOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 0x40) return 5;
    if ((int)unit->spatialBucket.get() == (int)g_game->overflowBucket.get()) {
        Vec3 center;
        center.x=(g_game->mapWidth/2)<<16;
        center.z=(g_game->mapHeight/2)<<16;
        short angle=GetHeadingBetween(&unit->pos,&center);
        Vec3 pos=AddVec3(unit->pos,Offset(angle,0x3200000));
        Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
        ((Class_0044e730*)move)->SetApproachRadius(128);
        order->flags|=0xe0;
        order->SetAttachedFx(move);
        return 2;
    }
    unsigned int state=0; state=order->state;
    switch(state) {
    case 0:
        if (unit->motion && (unit->def->flags1&0x800)) {
            if (order->target) {
                if (IssueAttackOrder(unit,order->target,0)) { order->flags=0; return 0; }
            } else {
                if (!order->pos.x && !order->pos.z && !order->pos.y) order->pos=unit->pos;
                order->angle=RandomInt(0x10000);
                order->parity=order->angle&1;
                g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
                if (unit->carrier) g_seams.units->AttachUnitToPiece(unit,0,-1,2);
                g_seams.units->SetStateBits(unit, 1,1);
                if ((unit->motion->flags&3)==1) {
                    unit->motion->SetFlightMode(unit,2);
                    Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,unit->pos);
                    ((Class_0044e6c0*)move)->SetAltitude(unit->def->altitude/2);
                    order->SetAttachedFx(move);
                    order->flags|=0xe0;
                }
            }
            return 1;
        }
        break;
    case 1: {
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        if ((unsigned int)unit->health < (unit->def->maxHealth>>2)*3) {
            LandingPadList pads;
            g_seams.ai->GetFactoriesInRadius(((Owner*)unit->player.get())->index,&unit->pos,0xf00,&pads);
            if (!pads.empty()) {
                order->SetAttachedFx(0);
                Unit* pad=pads[RandomInt(pads.count())];
                AppendOrder(unit,new (kSim) Order("VTOL_LANDING",pad,0,0,0,0));
                order->flags=0;
                return 0;
            }
        }
        Unit* target=g_seams.targeting->FindBestTargetIfFireAtWill(unit);
        if (target && IssueAttackOrder(unit,target,0)) return 5;
        if (flags&0xe0) order->angle+=-RandomInt(0x2000)-0x5555;
        Vec3 pos=AddVec3(order->pos,Offset((short)order->angle,(((WeaponDef*)unit->weapons[0].weapon)->range+160)<<16));
        Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
        ((Class_0044e730*)move)->SetApproachRadius(128);
        order->SetAttachedFx(move);
        order->SetDeadlineTicks(RandomInt(30)+30);
        order->flags|=0xe0;
        return 2;
    }
    }
    return 7;
}
