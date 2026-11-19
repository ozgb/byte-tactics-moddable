#include "win_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../state/block.h"
#include "../util/vec3.h"
#include "../../interfaces/ref.h"

static inline int Vec3Length(const Vec3* v)
{
    double fx = v->x;
    double fy = v->y;
    double fz = v->z;
    return (int)sqrt(fx * fx + fy * fy + fz * fz);
}

static inline void Vec3Scale(Vec3* v, int s)
{
    v->x = (int)(((__int64)v->x * s) >> 16);
    v->y = (int)(((__int64)v->y * s) >> 16);
    v->z = (int)(((__int64)v->z * s) >> 16);
}

static inline void Vec3Add(Vec3* v, const Vec3* o)
{
    v->x += o->x;
    v->y += o->y;
    v->z += o->z;
}
#include "../files/files_bank.h"

#pragma pack(push, 1)

#define ORDER_MAX(a, b) ((a) > (b) ? (a) : (b))

static inline void MulFixed(int& v, int f)
{
    v = (int)(((__int64)v * f) >> 16);
}

static inline Vec3 OrderVec3(int a, int b, int c)
{
    Vec3 v;
    v.x = a;
    v.y = b;
    v.z = c;
    return v;
}

static inline Vec3 PosPlus(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}
static inline Vec3 PosMinus(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

#include "../util/angles.h"
#include "../../interfaces/seams.h"
#include "../game/map_events.h"
#include "../../interfaces/map.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/services.h"
#include "../game/targeting_events.h"
#include "../game/los_radar_events.h"

#include "../network/player.h"

struct PlayerTypeView {
    int active;
    char unknown_4[0x73 - 0x4];
    unsigned char type;
    char unknown_74[0xe4 - 0x74];
    unsigned short field_e4;
};

struct MotionVelocityView {
    char unknown_0[8];
    Vec3 velocity;
    char unknown_14[0x20 - 0x14];
    int speed;
};

struct CarrierMotionView {
    Ref<MotionVelocityView> motion;
};
#include "unit_def_move_view.h"


#include "../scripts/cob_script.h"

struct Unit;

#include "../orders/order.h"



#include "../game/game.h"

#include "../units/unit.h"

static inline short PosXWhole(Unit* unit) { return *(short*)((char*)&unit->pos + 2); }
static inline short PosZWhole(Unit* unit) { return *(short*)((char*)&unit->pos + 0xa); }

#include "../orders/order_type_entry.h"
#include "../state/sim_globals.h"


#include "order_goals.h"
#include "ai_search_goal.h"
#include "unit_motion.h"
#pragma pack(pop)
#include "../orders/unit_orders.h"
#include "../orders/order_list.h"

#include "../../link/globals.h"

void __stdcall DeleteOrder(Unit* owner, Order* node);

extern int __cdecl _strcmpi(const char*, const char*);


void __stdcall DrawUnitRangeRings(void* a, void* b, Order* e, Vec3* p, int c);
extern OrderTableEntry g_readyOrder[];
extern signed char g_slopeSpeedFactor[];
#include "../util/math.h"
void __cdecl RotatePoint2D(short angle, int* xz);
int __cdecl AngleOfVector(int x, int z);
int __stdcall GetHeadingBetween(Vec3* from, Vec3* to);
struct UnitOrderTypeView;
struct FixedPosView;
#include "../orders/order_dispatch.h"
void RegisterUnitOrders();


#pragma pack(push, 1)
struct UnitMotionSaveRecord {
    Vec3 velocity;
    Vec3 p2;
    int speed;
    short turn;
    int pathLockStamp;
    unsigned char mode : 2;
    unsigned char flag : 1;
};

class PathGoalVtableView {
public:
    virtual void v0(int);
    virtual void v1();
    virtual void v2();
    virtual void v3(Vec3* out, int first, int count);
    virtual void v4(Vec3* a, Vec3* b, short* heading);
    virtual int v5();
    virtual AiSearchGoal* v6();
    virtual int v7();
    virtual void v8();
    virtual void v9(BitReader* reader);
};

#include "../units/sfx_occupy.h"
#pragma pack(pop)

// FUNCTION: 0x43cbb0
void UnitMotion::ApplyClampedTurnDelta(Unit* unit, short amount)
{
    if (amount != 0) {
        unsigned short max = ((UnitDefMoveView*)unit->def.get())->max_turn;
        if (amount >= max)
            turn = max;
        else if (amount <= -max)
            turn = -max;
        else
            turn = amount;
        unit->angles.heading += turn;
        unit->moved = 1;
    } else {
        turn = 0;
    }
}

static inline void ClampToZero(int& value)
{
    if (value < 0)
        value = 0;
}

static inline void AddFixed(int& v, float f)
{
    v += (int)((double)f * 65536.0);
}

static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin((short)angle, distance);
    v.y = 0;
    v.z = -ScaledCos((short)angle, distance);
    return v;
}

static inline void ClampToCell(Vec3& pos, Point16 cell, Point16 draft)
{
    int cx = (draft.x + cell.x * 2) << 19;
    int cz = (draft.y + cell.y * 2) << 19;
    if (pos.x > cx + 0x7ffff)
        pos.x = cx + 0x7ffff;
    else if (pos.x < cx - 0x7ffff)
        pos.x = cx - 0x7ffff;
    if (pos.z > cz + 0x7ffff)
        pos.z = cz + 0x7ffff;
    else if (pos.z < cz - 0x7ffff)
        pos.z = cz - 0x7ffff;
}

// FUNCTION: 0x43cc20
void UnitMotion::UpdateVelocityFromHeading(Unit* unit, int amount)
{
    speed = speed + amount;
    ClampToZero(speed);

    int idx = unit->angles.pitch >> 11;
    if (idx < -5)
        idx = -5;
    if (idx > 5)
        idx = 5;

    int range = (int)(((__int64)(g_slopeSpeedFactor[idx] << 16) * ((UnitDefMoveView*)unit->def.get())->maxvelocity) >> 16);
    range = (int)(((__int64)range << 16) / 0x640000);
    if (unit->yWhole < g_game->seaLevel && !(((UnitDefMoveView*)unit->def.get())->flags1 & 0x81000))
        range = (int)(((__int64)range * 0x8000) >> 16);
    if (speed > range)
        speed = range;

    int dist = speed;
    unsigned short angle = unit->angles.heading;
    Vec3 v;
    v.x = -ScaledSin((short)angle, dist);
    v.y = 0;
    v.z = -ScaledCos((short)angle, dist);
    velocity = v;
}

// FUNCTION: 0x43cd20
void UnitMotion::SteerGroundUnit(Unit* unit)
{
    if (obj->v5() == 0) {
        turn = 0;
        const int& amount = -((UnitDefMoveView*)unit->def.get())->brakerate;
        UpdateVelocityFromHeading(unit, amount);
        return;
    }

    Vec3 p[3];
    obj->v3(p, 0, 3);

    Vec3* ppos = &unit->pos;
    Vec3 d = PosMinus(p[1], *ppos);
    int gap1 = (int)_hypot(d.x, d.z);

    int dz;
    if (gap1 > 0x500000) {
        int dx = p[1].x - p[0].x;
        dz = p[1].z - p[0].z;
        int len = (int)_hypot(dx, dz);
        if (len >= 0x10000) {
            int ndx = (int)(((__int64)dx << 16) / len);
            int ndz = (int)(((__int64)dz << 16) / len);
            int back = gap1 - 0x500000;
            if (back > len)
                back = len;
            p[1].x -= (int)(((__int64)ndx * back) >> 16);
            p[1].z -= (int)(((__int64)ndz * back) >> 16);
        }
    }

    int az = p[1].z - unit->pos.z;
    int ax = p[1].x - unit->pos.x;
    int d1 = (int)(((__int64)ax * ax) >> 32) + (int)(((__int64)az * az) >> 32);

    short ang = (short)GetHeadingBetween(ppos, &p[1]);
    short diff = ang - unit->angles.heading;
    int sdiff = diff;
    int adiff = abs(sdiff);

    int bz = p[2].z - unit->pos.z;
    int bx = p[2].x - unit->pos.x;
    int d2 = (int)(((__int64)bx * bx) >> 32) + (int)(((__int64)bz * bz) >> 32);

    if (diff != 0) {
        unsigned short max = ((UnitDefMoveView*)unit->def.get())->max_turn;
        if (sdiff >= max)
            turn = max;
        else if (sdiff <= -max)
            turn = -max;
        else
            turn = diff;
        unit->angles.heading += turn;
        unit->moved = 1;
    } else {
        turn = 0;
    }

    int turned = (int)((((__int64)(adiff & 0xffff) * (__int64)speed)
                        / ((UnitDefMoveView*)unit->def.get())->max_turn));
    int rate = ((UnitDefMoveView*)unit->def.get())->brakerate;
    int spd = speed;
    int t = (int)(((__int64)spd * spd) >> 16);
    int q = (int)(((__int64)t << 16) / (2 * rate));
    int r = (int)(((__int64)q * q) >> 32);
    int lim = (int)(((__int64)turned * turned) >> 32) * 4;

    if (d1 > lim && d2 > r)
        UpdateVelocityFromHeading(unit, ((UnitDefMoveView*)unit->def.get())->acceleration);
    else
        UpdateVelocityFromHeading(unit, -rate);
}

#pragma auto_inline(off)
// FUNCTION: 0x43d0d0
void UnitMotion::ApplyBankAndPitch(Unit* owner, Vec3* v)
{
    Vec3Scale(&p2, 0xf333);
    Vec3Add(&p2, v);
    int xz[2];
    xz[0] = p2.x;
    xz[1] = p2.z;
    RotatePoint2D(owner->angles.heading, xz);
    int n = (int)(((__int64)g_game->gravity << 16) / 0xccd);
    owner->angles.bank = (short)AngleOfVector((int)(((__int64)((UnitDefMoveView*)owner->def.get())->bankscale * -xz[0]) >> 16), n);
    owner->angles.pitch = (short)AngleOfVector((int)(((__int64)((UnitDefMoveView*)owner->def.get())->pitchscale * -xz[0]) >> 16), n);
}
#pragma auto_inline(on)

// FUNCTION: 0x43d210
void UnitMotion::SetFlightMode(Unit* owner, int newState)
{
    if (mode != newState) {
        if (newState == 1) {
            speed = 0;
            Vec3 zero = OrderVec3(0, 0, 0);
            velocity = zero;
            ApplyBankAndPitch(owner, &zero);
            g_seams.units->SetStateBits(owner, 1, 0);
        } else {
            g_seams.units->SetStateBits(owner, 1, 1);
        }
        mode = newState;
    }
}

// FUNCTION: 0x43d290
void UnitMotion::SteerAircraft(Unit* unit) {
    if (mode != 2) {
        velocity = OrderVec3(0, 0, 0);
        speed = 0;
        turn = 0;
        return;
    }

    Vec3 old = velocity;
    Vec3 a;
    Vec3 b;
    short heading;
    obj->v4(&a, &b, &heading);

    UnitDefMoveView* type = (UnitDefMoveView*)unit->def.get();
    const float eps = 1.52587890625e-05f;

    float f18 = (float)type->acceleration * eps;
    int q = (int)(((__int64)type->acceleration << 16) / type->maxvelocity);
    int scale = 0x10000 - q;
    Vec3Scale(&velocity, scale);

    float dist = (float)_hypot(velocity.x, velocity.z) * eps;
    float maxd = (float)((UnitDefMoveView*)unit->def.get())->brakerate * eps;
    if (dist > maxd) {
        int f = (int)((double)maxd / dist * 65536.0);
        MulFixed(velocity.x, f);
        MulFixed(velocity.z, f);
        int g = (int)(((double)dist - maxd) * 65536.0);
        velocity += Offset(unit->angles.heading, g);
    }

    Vec3 da = PosMinus(unit->pos, a);
    Vec3 db = PosMinus(velocity, b);

    if ((int)unit->spatialBucket.get() != (int)g_game->overflowBucket.get()) {
        int lim;
        if ((speed & -4) < 0x40000)
            lim = 0x10000;
        else
            lim = speed >> 2;
        if (da.y <= -lim)
            velocity.y = lim;
        else if (da.y >= lim)
            velocity.y = -lim;
        else
            velocity.y = -da.y;
    }

#ifdef __clang__
    double hd = _hypot(da.x, da.z) * eps;
#else
    float hd = (float)_hypot(da.x, da.z) * eps;
#endif
    short d = (short)(heading - unit->angles.heading);
    if (d != 0) {
        unsigned short max = ((UnitDefMoveView*)unit->def.get())->max_turn;
        if (d >= (int)max)
            turn = max;
        else if (d <= -(int)max)
            turn = (short)-max;
        else
            turn = d;
        unit->angles.heading = (short)(unit->angles.heading + turn);
        unit->moved = 1;
    } else {
        turn = 0;
    }

    if (hd < 8.0f)
        hd = 8.0f;

#ifdef __clang__
    double k = -sqrt((2.0 * f18) / hd);
    double vxd = (double)da.x * k * eps - (double)db.x * eps;
    double vzd = (double)da.z * k * eps - (double)db.z * eps;
    float vx = vxd;
    float vz = vzd;
    double vxs = vx;
    double mag = _hypot(vxd, vzd);
    if (mag > f18) {
        vxs = vx * (f18 / mag);
        vz = vz * (f18 / mag);
    }
#else
    float k = -sqrt((2.0f * f18) / hd);
    float vx = ((float)da.x) * k * eps - (float)db.x * eps;
    float vz = (float)da.z * k * eps - (float)db.z * eps;
    float mag = (float)_hypot(vx, vz);
    if (mag > f18) {
        vx = vx * (f18 / mag);
        vz = vz * (f18 / mag);
    }

#endif

#ifdef __clang__
    velocity.x += (int)(vxs * 65536.0);
#else
    AddFixed(velocity.x, vx);
#endif
    AddFixed(velocity.z, vz);
    speed = Vec3Length(&velocity);

    {
        Vec3 delta = PosMinus(velocity, old);
        ApplyBankAndPitch(unit, &delta);
    }
}

// FUNCTION: 0x43d6d0
void UnitMotion::UpdatePosition(Unit* u)
{
    if (u->carrier != 0) {
        Vec3 v;
        v = g_seams.scripts->GetPiecePosition(u->carrier, (signed char)u->transportPiece);
        if (((UnitDefMoveView*)u->def.get())->b19) {
            v.y = ORDER_MAX(v.y, MakeFixed(((UnitDefMoveView*)u->def.get())->draft * 0xffff + g_game->seaLevel).value);
        }
        g_seams.units->SetUnitPosition(u, v, mode);
        Angles16 o = g_seams.scripts->GetPieceAngles(u->carrier, (signed char)u->transportPiece);
        u->angles = o;
        if (((CarrierMotionView*)u->carrier.get())->motion != 0) {
            speed = ((CarrierMotionView*)u->carrier.get())->motion->speed;
            velocity = ((CarrierMotionView*)u->carrier.get())->motion->velocity;
        } else {
            speed = 0;
            Vec3 zero = OrderVec3(0, 0, 0);
            velocity = zero;
        }
        u->moved = 0;
        return;
    }

    Vec3 pos;
    pos = PosPlus(u->pos, velocity);
    int m = mode;
    if (pos.x == u->pos.x && pos.z == u->pos.z && pos.y == u->pos.y && m == u->mode)
        return;

    lastMoveTick = g_game->ticks;
    Point16 draft = *(Point16*)&u->footprint;
    Point16 cell;
    cell.x = (pos.x - draft.x * 0x80000 + 0x80000) >> 20;
    cell.y = (pos.z - draft.y * 0x80000 + 0x80000) >> 20;
    if (cell.x == u->cell.x && cell.y == u->cell.y && m == u->mode) {
        u->pos = pos;
        u->moved = 1;
        return;
    }

    if (((PlayerTypeView*)u->player.get())->active != 0) {
        if (((PlayerTypeView*)u->player.get())->type == 1 || ((PlayerTypeView*)u->player.get())->type == 2)
            flag = ScheduleMapCanPlaceUnitFootprint((UnitDef*)u->def, (unsigned short)u->id, *(Point16*)&cell, m) == 0;
    }

    if (flag) {
        ClampToCell(pos, *(Point16*)&u->cell, *(Point16*)&u->footprint);

        if (speed > (((UnitDefMoveView*)u->def.get())->maxvelocity / 2)) {
            int half = ((UnitDefMoveView*)u->def.get())->maxvelocity / 2;
            speed = half;
            unsigned short angle = u->angles.heading;
            Vec3 vec;
            vec.x = -ScaledSin((short)angle, half);
            vec.y = 0;
            int z = -ScaledCos((short)angle, half);
            vec.z = z;
            velocity = vec;
        }
        u->pos = pos;
        u->moved = 1;
        return;
    }

    // TODO(#985): publish from SCHED_MOVEMENT
    MapPublishUnit(SCHED_ORDERS, MAP_REMOVE_UNIT, u);
    u->pos = pos;
    *(Point16*)&u->cell = cell;
    u->mode = (short)m;
    // TODO(#985): publish from SCHED_MOVEMENT
    MapPublishUnit(SCHED_ORDERS, MAP_ADD_UNIT, u);
    u->moved = 1;
    // TODO(#985): publish from SCHED_MOVEMENT
    LosRadarPublishUpdateUnit(SCHED_ORDERS, u);
}

// FUNCTION: 0x43da70
void UnitMotion::UpdateMoveRate(Unit* unit)
{
    int rate;
    if ((flags & 4) == 0 && unit->carrier == 0
        && (speed != 0 || turn != 0)) {
        if (speed <= ((UnitDefMoveView*)unit->def.get())->moverate1) {
            rate = 1;
        } else {
            rate = 2 + (speed > ((UnitDefMoveView*)unit->def.get())->moverate2);
        }
    } else {
        rate = 0;
    }
    if (rate == (int)((unit->flags >> 2) & 3))
        return;
    if (rate == 0) {
        g_seams.scripts->StartScript((CobScript*)unit->script.get(), "StopMoving", nullptr, 1);
    } else if ((unit->flags & 0xc) == 0) {
        g_seams.scripts->StartScript((CobScript*)unit->script.get(), "StartMoving", nullptr, 1);
    }
    switch (rate) {
    case 1:
        g_seams.scripts->StartScript((CobScript*)unit->script.get(), "MoveRate1", nullptr, 1);
        break;
    case 2:
        g_seams.scripts->StartScript((CobScript*)unit->script.get(), "MoveRate2", nullptr, 1);
        break;
    case 3:
        g_seams.scripts->StartScript((CobScript*)unit->script.get(), "MoveRate3", nullptr, 1);
        break;
    }
    unit->flags = (unit->flags & 0xfffffff3) | ((rate & 3) << 2);
}

// FUNCTION: 0x43dc00
UnitMotion::UnitMotion(Unit* unit)
{
    velocity = OrderVec3(0, 0, 0);
    speed = 0;
    turn = 0;
    pathLockStamp = 0;
    p2 = OrderVec3(0, 0, 0);
    mode = 1;
    flag = 0;
    movementClass = ((UnitDefMoveView*)unit->def.get())->movementclass.get();
    if (((PlayerTypeView*)unit->player.get())->active != 0 && ((PlayerTypeView*)unit->player.get())->type == 3) {
        if (((UnitDefMoveView*)unit->def.get())->flag_800)
            obj = (PathGoalVtableView*)new (kSim) PackedPosGoal(unit);
        else
            obj = (PathGoalVtableView*)new (kSim) PatrolGoal(unit);
    } else {
        if (((UnitDefMoveView*)unit->def.get())->flag_800)
            obj = (PathGoalVtableView*)new (kSim) LiteGoal(unit);
        else
            obj = (PathGoalVtableView*)new (kSim) AiSearchGoal(unit);
    }
}

// FUNCTION: 0x43dd10
void UnitMotion::DestroyObject()
{
    if (obj != 0) {
        obj->v0(1);
    }
}

AiSearchGoal* UnitMotion::ClaimRepath()
{
    return obj->v6();
}

void UnitMotion::ReadNetState(BitReader* reader)
{
    obj->v9(reader);
}

// FUNCTION: 0x43dd20
void UnitMotion::UpdateMotion(Unit* u)
{
    obj->v2();
    if (((UnitDefMoveView*)u->def.get())->flag_800)
        SteerAircraft(u);
    else
        SteerGroundUnit(u);
    UpdatePosition(u);
    UpdateMoveRate(u);
    g_seams.units->UpdateSfxOccupy((UnitMotion*)this, 0, u);
}

// FUNCTION: 0x43dd70
void UnitMotion::SaveMotion(Unit* info, HapiBank* file)
{
    char name[32];
    UnitMotionSaveRecord hdr;
    hdr.velocity = velocity;
    hdr.p2 = p2;
    hdr.speed = speed;
    hdr.turn = turn;
    hdr.pathLockStamp = pathLockStamp;
    hdr.mode = mode;
    hdr.flag = flag;
    sprintf(name, "u%04xmob", info->id);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 0x23);
}

// FUNCTION: 0x43de30
void UnitMotion::LoadMotion(Unit* unit, HapiBank* file)
{
    char name[32];
    UnitMotionSaveRecord rec;
    sprintf(name, "u%04xmob", unit->id);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->ReadBox(&rec, 0x23);
    velocity = rec.velocity;
    p2 = rec.p2;
    speed = rec.speed;
    turn = rec.turn;
    pathLockStamp = rec.pathLockStamp;
    mode = rec.mode;
    flag = rec.flag;
}
