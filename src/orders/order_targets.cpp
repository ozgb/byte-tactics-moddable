#include <memory.h>
#include "../state/block.h"
#include "../files/files_api.h"
#include <stdlib.h>
#include <vector>
#include "../util/vec3.h"
#include <algorithm>
#include <math.h>

extern void* g_approachRadiusVtable[];
extern void* g_ringApproachVtable[];
extern void* g_pointMarkerVtable[];

extern void __cdecl operator delete(void* p);

#include "order_fx.h"

void RegisterUnitOrders();
int UpdatePlacementGhostValidity();
void InitMissionStatus();


typedef std::vector<Point16> Vec_0044d0e0;

#include "approach_areas.h"

typedef std::vector<Point16> Vec_0044d560;

unsigned short __stdcall GetHeadingBetween(void* from, void* to);

int __cdecl ScaledSin(short angle, int scale);
int __cdecl ScaledCos(short angle, int scale);

#pragma pack(push, 1)
#include "../units/unit_def.h"
#pragma pack(pop)

#pragma pack(push, 1)
#include "../units/unit.h"
#include "../weapons/weapon_def.h"
#pragma pack(pop)

typedef std::vector<Point16> Vec_0044da00;

#pragma pack(push, 2)

class BitWriter {
public:
    int bit;
    int index;
    int capacity;
    unsigned int* data;
    unsigned int buffer[0x100];
    void GrowBuffer();
    void WriteBits(int value, int bits);
};

class BitReader;
struct Order;
#include "path_order_attach.h"

class __declspec(novtable) PathOrder : public OrderFx {
public:
    union {
        unsigned short flags;
        short field_8;
        struct {
            unsigned short : 6;
            unsigned short headingFlag : 1;
            unsigned short : 9;
        };
        struct {
            unsigned short : 4;
            unsigned short radiusFlag : 1;
            unsigned short : 11;
        };
    };
    short approachRadius;
    short altitude;
    short heading;
    short piece;
    Ref<Unit> unit;
    PathOrderAttach ref;
    union {
        Vec3 pos;
        struct {
            int pos_x;
            int pos_y;
            int pos_z;
        };
    };
    int followOffset;

    PathOrder(int owner, HapiBank* file, char* name);
    PathOrder(Unit* owner_, BitReader* reader);
    PathOrder(Order* order, Unit* unit);
    PathOrder(Order* source, int unit, short value);
    PathOrder(Order* source, const Vec3& p);
    PathOrder(Order* source, int unit, const Vec3& p);
    void SetAltitude(int param_1);
    void SetHeading(short v);
    void SetApproachRadius(short v);

    void SerializeToBits(BitWriter* stream);
    int GetType();
    int IsFxStyle();
    void* Destroy(unsigned char flag);
    int SerializeToSave(int unused, HapiBank* file, char* name);
    int KeepAfterComplete();
    int FillWorldPos(Vec3* out);
    int GetDesiredHeading(unsigned short* out);
    int IsComplete(Unit* arg);
};

#pragma pack(pop)

class PathOrderAttach;

#include "path_order_attach.h"

class UnitRef {
public:
    void Unlink();
};

extern void* g_pathOrderVtable[];
extern void* g_airManeuverOrderVtable[];

#pragma pack(push, 2)
struct PathOrderRecord {
    int unknown_0;
    int unknown_4;
    short id1;
    void* ref_vt;
    void* ref_owner;
    void* ref_next;
    int ref_value;
    short id2;
    short f1;
    short f2;
    short f3;
    short f4;
    short f5;
    Vec3 pos;
    int i4;
};

#pragma pack(pop)

class BitReader {
public:
    unsigned int* data;
    int index;
    int bit;
    int ReadBits(int bits);

    int ReadBit()
    {
        int r = (data[index] & (1 << bit)) != 0;
        if (++bit == 32) {
            bit = 0;
            index++;
        }
        return r;
    }
};

class Pathfinder;

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

#include "../ai/pathfinder.h"

#include "../../interfaces/seams.h"

#pragma pack(push, 2)
#include "order.h"

#pragma pack(pop)

#pragma pack(push, 2)

#pragma pack(push, 2)
#include "air_maneuver_order.h"
#pragma pack(pop)

int RIReport(int, int, int, int, int, int, int, int, int, int);
int DrawWrappedText(char*, char*, int, int, int, int, int);
void CmdMem(int);

typedef std::vector<Point16> Vec_0044ee60;
typedef void (Vec_0044ee60::*DestroyFn_0044ee60)(Vec_0044ee60::iterator, Vec_0044ee60::iterator);

struct PointVectorDestroyAccess : Vec_0044ee60 {
    static DestroyFn_0044ee60 fn;
};

typedef std::vector<Point16> Vec_0044ee70;
typedef Vec_0044ee70::size_type (Vec_0044ee70::*SizeFn_0044ee70)() const;

typedef std::vector<Point16> Vec_0044ee90;
typedef Vec_0044ee90::iterator (Vec_0044ee90::*UcopyFn_0044ee90)(
    Vec_0044ee90::const_iterator, Vec_0044ee90::const_iterator, Vec_0044ee90::iterator);

struct PointVectorUcopyAccess : Vec_0044ee90 {
    static UcopyFn_0044ee90 fn;
};

typedef std::vector<Point16> Vec_0044eec0;
typedef void (Vec_0044eec0::*UfillFn_0044eec0)(
    Vec_0044eec0::iterator, Vec_0044eec0::size_type, const Point16&);

struct PointVectorUfillAccess : Vec_0044eec0 {
    static UfillFn_0044eec0 fn;
};

#include "order_target_types.h"

#include "../game/goal_owner.h"

#include "../ai/ai_search_goal.h"
#include "order_goals.h"

#include "unit_motion.h"

static inline UnitMotion* Motion(Unit* unit)
{
    return unit->motion;
}

#include "order_targets.h"

void FreePictureCache();
int MainLoopContinueStub();
int MainLoopContinueStub_B();
int MainLoopContinueStub_C();
int LoadDefaultPalette();
extern int Pad6260_01;
extern int Pad6260_02;
extern int Pad6260_03;
extern int Pad6260_04;
extern int Pad6260_05;
extern int Pad6260_06;
extern int Pad6260_07;
extern int Pad6260_08;
extern int Pad6260_09;
extern int Pad6260_10;
extern int Pad6260_11;
extern int Pad6260_12;
extern int Pad6260_13;
extern int Pad6260_14;
extern int Pad6260_15;
extern int Pad6260_16;
extern int Pad6260_17;
extern int Pad6260_18;
extern int Pad6260_19;
extern int Pad6260_20;
extern int Pad6260_21;
extern int Pad6260_22;
extern int Pad6260_23;
extern int Pad6260_24;
extern int Pad6260_25;
extern int Pad6260_26;
extern int Pad6260_27;
extern int Pad6260_28;
extern int Pad6260_29;
extern int Pad6260_30;
extern int Pad6260_31;
extern int Pad6260_32;
extern int Pad6260_33;
extern int Pad6260_34;
extern int Pad6260_35;
extern int Pad6260_36;
extern int Pad6260_37;
extern int Pad6260_38;
extern int Pad6260_39;
extern int Pad6260_40;
extern int Pad6260_41;
extern int Pad6260_42;
extern int Pad6260_43;
extern int Pad6260_44;
extern int Pad6260_45;
extern int Pad6260_46;
extern int Pad6260_47;
extern int Pad6260_48;
extern int Pad6260_49;
extern int Pad6260_50;
extern int Pad6260_51;
extern int Pad6260_52;
extern int Pad6260_53;
extern int Pad6260_54;
extern int Pad6260_55;
extern int Pad6260_56;
extern int Pad6260_57;
extern int Pad6260_58;
extern int Pad6260_59;
extern int Pad6260_60;
extern int Pad6260_61;
extern int Pad6260_62;
extern int Pad6260_63;
extern int Pad6260_64;
extern int Pad6260_65;
extern int Pad6260_66;
extern int Pad6260_67;

void __cdecl RotatePoint2D(short angle, int* xy);

// FUNCTION: 0x44ce40
int OrderFx::GetType()
{
    return 1;
}

// FUNCTION: 0x44ce50
void* OrderFx::Destroy(unsigned char flag)
{
    SetVtable(g_orderFxVtable);
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// FUNCTION: 0x44ce70 ??0OrderFx@@QAE@HHH@Z
OrderFx::OrderFx(int arg1, int arg2, int arg3)
{
    SetVtable(g_orderFxVtable);
}

// FUNCTION: 0x44ce80
int __stdcall SerializeSave(int, int, int)
{
    return 0;
}

// FUNCTION: 0x44ce90
void OrderFx::FillGoalCells(void* cells)
{
    ((std::vector<void*>*)cells)->clear();
}

// FUNCTION: 0x44cec0
int OrderFx::ApproxDist(int, int)
{
    return 0;
}

#pragma auto_inline(off)
// FUNCTION: 0x44ced0
void OrderFx::AddFlags(int param_1)
{
    Order* val = source;
    if (val != 0) {
        val->subFlags |= param_1;
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x44cef0
int OrderFx::KeepAfterComplete()
{
    return 0;
}

// FUNCTION: 0x44cf00
int OrderFx::ContainsUnit(Unit* unit)
{
    short esi = unit->cell.y;
    short eax = unit->cell.x;

    return ContainsCell(eax, esi);
}

// FUNCTION: 0x44cf20
int OrderFx::ContainsCell(int, int)
{
    return 0;
}

// FUNCTION: 0x44cf30
int OrderFx::IsFxStyle()
{
    return 1;
}

// FUNCTION: 0x44cf40
int __stdcall TryGetDesiredHeading(int)
{
    return 0;
}

// FUNCTION: 0x44cf50
void OrderFx::WriteBits(int)
{
}

// FUNCTION: 0x44cf60 ??0ApproachRadius@@QAE@PAUOrder@@HHH@Z
ApproachRadius::ApproachRadius(Order* source, int x, int y, int r)
    : OrderFx(source)
{
    SetVtable(g_approachRadiusVtable);
    Point16 org = source->unit->footprint;
    Point16 p;
    p.x = (x - (org.x << 19) + 0x80000) >> 20;
    p.y = (y - (org.y << 19) + 0x80000) >> 20;
    pos = *(Point16*)&p;
    radius = r;
    radiusSq = (r / 16) * (r / 16);
}

// FUNCTION: 0x44cfe0
int ApproachRadius::GetType()
{
    return 4;
}

// FUNCTION: 0x44cff0
void* ApproachRadius::Destroy(unsigned char flag)
{
    SetVtable(g_orderFxVtable);
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// FUNCTION: 0x44d010 ??0ApproachRadius@@QAE@HPAVHapiBank@@PAD@Z
ApproachRadius::ApproachRadius(int owner, HapiBank* file, char* name)
    : OrderFx((Order*)owner)
{
    SetVtable(g_approachRadiusVtable);
    int bad = 0;
    if (owner == 0 || file == 0)
        bad = 1;
    if (name == 0)
        bad = 1;
    if (!bad) {
        file->OpenNamedBox(name);
        file->SeekBox(0);
        ApproachRadiusRecord hdr;
        if (file->ReadBox(&hdr, 16) == 16) {
            v.x = hdr.v.x;
            v.y = hdr.v.y;
            v.z = hdr.v.z;
        }
    }
}

// FUNCTION: 0x44d090
int ApproachRadius::Serialize(int unused, HapiBank* file, char* name)
{
    ApproachRadiusRecord hdr;
    hdr.v.x = v.x;
    hdr.v.y = v.y;
    hdr.v.z = v.z;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 16);
    return 1;
}

// FUNCTION: 0x44d0e0
void ApproachRadius::AppendGoalCell(void* cells)
{
    Vec_0044d0e0* list = (Vec_0044d0e0*)cells;
    list->clear();
    list->push_back(*(Point16*)&pos);
}

// FUNCTION: 0x44d290
int ApproachRadius::ContainsCell(int px, int py)
{
    int dy = py - pos.y;
    int dx = px - pos.x;
    return dx * dx + dy * dy <= radiusSq;
}

static inline void ToWorld(int* out, Point16 a, Point16 b)
{
    out[0] = (a.x * 2 + b.x) << 19;
    out[2] = (a.y * 2 + b.y) << 19;
}

// FUNCTION: 0x44d2c0
int ApproachRadius::FillWorldPos(int* out)
{
    ToWorld(out, pos, ((Order*)source)->unit->footprint);
    return 1;
}

// FUNCTION: 0x44d310
int ApproachRadius::ContainsUnit(Unit* obj)
{
    int dy = obj->cell.y - pos.y;
    int dx = obj->cell.x - pos.x;
    return dx * dx + dy * dy <= radiusSq;
}

// FUNCTION: 0x44d350
int ApproachRadius::ApproxDistExcess(int px, int py)
{
    int dx = abs(px - pos.x);
    int dy = abs(py - pos.y);
    int d;
    if (dx > dy)
        d = dy * 7 + dx * 18;
    else
        d = dx * 7 + dy * 18;
    if (d < radius)
        return 0;
    return d - radius;
}

// FUNCTION: 0x44d3b0 ??0RingApproach@@QAE@PAUOrder@@HHHH@Z
RingApproach::RingApproach(Order* source, int x, int y, int r1, int r2)
    : OrderFx(source)
{
    SetVtable(g_ringApproachVtable);
    Point16 org = source->unit->footprint;
    Point16 p;
    p.x = (x - (org.x << 19) + 0x80000) >> 20;
    p.y = (y - (org.y << 19) + 0x80000) >> 20;
    pos = *(Point16*)&p;
    inner = r2;
    outer = r1;
    outerSq = (r1 / 16) * (r1 / 16);
    innerSq = (r2 / 16) * (r2 / 16);
}

// FUNCTION: 0x44d440
int RingApproach::GetType()
{
    return 5;
}

// FUNCTION: 0x44d450
void* RingApproach::Destroy(unsigned char flag)
{
    SetVtable(g_orderFxVtable);
    if ((flag & 1) != 0) {
        operator delete(this);
    }
    return this;
}

// FUNCTION: 0x44d470 ??0RingApproach@@QAE@HPAVHapiBank@@PAD@Z
RingApproach::RingApproach(int owner, HapiBank* file, char* name)
    : OrderFx((Order*)owner)
{
    SetVtable(g_ringApproachVtable);
    int bad = 0;
    if (owner == 0 || file == 0)
        bad = 1;
    if (name == 0)
        bad = 1;
    if (!bad) {
        file->OpenNamedBox(name);
        file->SeekBox(0);
        RingApproachRecord hdr;
        if (file->ReadBox(&hdr, 24) == 24) {
            data.a = hdr.pos;
            data.b = hdr.inner;
            data.c = hdr.outer;
            data.d = hdr.innerSq;
            data.e = hdr.outerSq;
        }
    }
}

// FUNCTION: 0x44d500
int RingApproach::Serialize(int unused, HapiBank* file, char* name)
{
    RingApproachRecord hdr;
    hdr.pos = data.a;
    hdr.inner = data.b;
    hdr.outer = data.c;
    hdr.innerSq = data.d;
    hdr.outerSq = data.e;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 24);
    return 1;
}

// FUNCTION: 0x44d560
void RingApproach::AppendGoalCell(void* cells)
{
    Vec_0044d560* list = (Vec_0044d560*)cells;
    list->clear();
    Point16 p = *(Point16*)&pos;
    p.y = p.y + (outer + inner) / 32;
    list->push_back(p);
}

static inline void ToWorld(Vec3* out, Point16 a, Point16 b)
{
    out->x = (a.x * 2 + b.x) << 19;
    out->z = (a.y * 2 + b.y) << 19;
}

static inline Vec3 Direction(short angle, int scale)
{
    Vec3 v;
    v.x = -ScaledSin(angle, scale);
    v.y = 0;
    v.z = -ScaledCos((short)angle, (int)scale);
    return v;
}

static inline int MeanRadius(RingApproach* c)
{
    int r1 = c->inner;
    int r2 = c->outer;
    return (r1 + r2) / 2;
}

// FUNCTION: 0x44d720
int RingApproach::FillWorldPos(Vec3* out)
{
    Unit* inner = ((Order*)source)->unit;
    ToWorld(out, pos, inner->footprint);
    short angle = GetHeadingBetween(&inner->pos, out);
    Vec3 d = Direction(angle, MeanRadius(this) << 16);
    out->x -= d.x;
    out->y -= d.y;
    out->z -= d.z;
    return 1;
}

// FUNCTION: 0x44d7c0
int RingApproach::ContainsCell(int px, int py)
{
    int dy = py - y;
    int dx = px - x;
    int distSq = dy * dy + dx * dx;
    return distSq <= outerSq && distSq >= innerSq;
}

// FUNCTION: 0x44d800
int RingApproach::ContainsUnit(Unit* unit)
{
    int dy = unit->cell.y - y;
    int dx = unit->cell.x - x;
    int distSq = dx * dx + dy * dy;
    return distSq <= outerSq && distSq >= innerSq;
}

// FUNCTION: 0x44d840
int RingApproach::ApproxDistExcess(int px, int py)
{
    int dx = abs(px - x);
    int dy = abs(py - y);
    int d;
    if (dx > dy)
        d = dy * 7 + dx * 18;
    else
        d = dx * 7 + dy * 18;
    if (d > outer)
        return d - outer;
    if (d < inner)
        return inner - d;
    return 0;
}

// FUNCTION: 0x44d8a0 ??0PointMarker@@QAE@PAUOrder@@UPoint16@@1@Z
PointMarker::PointMarker(Order* owner, Point16 pos, Point16 size)
    : OrderFx(owner)
{
    SetVtable(g_pointMarkerVtable);
    x1 = pos.x - owner->unit->footprint.x;
    y1 = pos.y - owner->unit->footprint.y;
    x2 = size.x + pos.x;
    y2 = size.y + pos.y;
}

// FUNCTION: 0x44d900
int PointMarker::GetType()
{
    return 6;
}

// FUNCTION: 0x44d910
void* PointMarker::Destroy(unsigned char should_delete)
{
    PointMarker* esi = this;
    esi->SetVtable(g_orderFxVtable);
    if (should_delete & 1) {
        operator delete(esi);
    }
    return esi;
}

// FUNCTION: 0x44d930 ??0PointMarker@@QAE@HPAVHapiBank@@PAD@Z
PointMarker::PointMarker(int owner, HapiBank* file, char* name)
    : OrderFx((Order*)owner)
{
    SetVtable(g_pointMarkerVtable);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    PointMarkerRecord hdr;
    if (file->ReadBox(&hdr, 20) == 20) {
        x1 = hdr.x1;
        x2 = hdr.x2;
        y1 = hdr.y1;
        y2 = hdr.y2;
    }
}

// FUNCTION: 0x44d9a0
int PointMarker::Serialize(int unused, HapiBank* file, char* name)
{
    PointMarkerRecord hdr;
    hdr.x1 = x1;
    hdr.x2 = x2;
    hdr.y1 = y1;
    hdr.y2 = y2;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 20);
    return 1;
}

static inline int MidX_44dc60(PointMarker* w)
{
    return (w->x1 + w->x2) / 2;
}

// FUNCTION: 0x44dc60
int PointMarker::FillWorldPos(int* out)
{
    short avg = (short)MidX_44dc60(this);
    short z = (short)y2;
    Point16 pos = ((Order*)source)->unit->footprint;
    out[0] = (pos.x + avg * 2) << 0x13;
    out[2] = (pos.y + z * 2) << 0x13;
    return 1;
}

// FUNCTION: 0x44dcb0
int PointMarker::ContainsCell(int x, int y)
{
    if ((x == x1 || x == x2) && y >= y1 && y <= y2) {
        return 1;
    }
    if ((y == y1 || y == y2) && x >= x1 && x <= x2) {
        return 1;
    }
    return 0;
}

#define Min_0044dd00(a, b) ((a) < (b) ? (a) : (b))

// FUNCTION: 0x44dd00
int PointMarker::ApproxDist(int x, int y)
{
    int dx;
    if (x < x1) {
        dx = x1 - x;
    }
    else if (x > x2) {
        dx = x - x2;
    }
    else {
        if (y < y1)
            return (y1 - y) * 16;
        if (y > y2)
            return (y - y2) * 16;
        int mx = Min_0044dd00(x - x1, x2 - x);
        int my = Min_0044dd00(y - y1, y2 - y);
        return Min_0044dd00(mx, my) * 16;
    }

    int dy;
    if (y < y1)
        dy = y1 - y;
    else if (y > y2)
        dy = y - y2;
    else
        return dx * 16;

    if (dx > dy)
        return dx * 16 + dy * 6;
    return dy * 16 + dx * 6;
}

// FUNCTION: 0x44ddc0
void PathOrder::SerializeToBits(BitWriter* stream)
{
    stream->WriteBits(flags, 8);
    if ((flags & 1) != 0) {
        stream->WriteBits(piece, 0x10);
        stream->WriteBits((int)(unsigned short)(ref.owner == 0 ? 0 : (unsigned short)((short)ref.owner->id)), 0x10);
    }
    if ((flags & 0x10) != 0) {
        stream->WriteBits(approachRadius, 0x10);
    }
    if ((flags & 8) != 0) {
        stream->WriteBits(altitude, 0x10);
    }
    if ((flags & 0x40) != 0) {
        stream->WriteBits(heading, 0x10);
    }
    if ((flags & 0x20) != 0) {
        stream->WriteBits(pos.x, 0x20);
        stream->WriteBits(pos.y, 0x20);
        stream->WriteBits(pos.z, 0x20);
    }
}

// FUNCTION: 0x44de80 ??0PathOrder@@QAE@HPAVHapiBank@@PAD@Z
PathOrder::PathOrder(int owner, HapiBank* file, char* name)
    : OrderFx((Order*)owner), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    PathOrderRecord rec;
    ((PathOrderAttach*)&rec.ref_vt)->PathOrderAttach::PathOrderAttach(0, 0);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    if (file->ReadBox(&rec, 0x36) == 0x36) {
        unit = g_seams.units->LoadUnit(rec.id1, file);
        ref.SetUnit((Unit*)g_seams.units->LoadUnit(rec.id2, file));
        field_8 = rec.f1;
        approachRadius = rec.f2;
        altitude = rec.f3;
        heading = rec.f4;
        piece = rec.f5;
        pos = *(Vec3*)&rec.pos;
        followOffset = rec.i4;
    }
    g_seams.units->UnitRefUnlink(((UnitRef*)&rec.ref_vt));
}

// FUNCTION: 0x44df70
int PathOrder::GetType()
{
    return 2;
}

// FUNCTION: 0x44df80
void* PathOrder::Destroy(unsigned char flag)
{
    g_seams.units->UnitRefUnlink(((UnitRef*)&ref));
    SetVtable(g_orderFxVtable);
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// FUNCTION: 0x44dfb0
int PathOrder::SerializeToSave(int unused, HapiBank* file, char* name)
{
    PathOrderRecord rec;
    PathOrderAttach* ref = (PathOrderAttach*)&rec.ref_vt;
    ref->PathOrderAttach::PathOrderAttach(0, 0);
    if (unit == 0)
        rec.id1 = 0;
    else
        rec.id1 = ((short)unit->id);
    if (this->ref.owner == 0)
        rec.id2 = 0;
    else
        rec.id2 = ((short)this->ref.owner->id);
    rec.f1 = flags;
    rec.f2 = approachRadius;
    rec.f3 = altitude;
    rec.f4 = heading;
    rec.f5 = piece;
    rec.pos = pos;
    rec.i4 = followOffset;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&rec, 0x36);
    g_seams.units->UnitRefUnlink(((UnitRef*)&rec.ref_vt));
    return 1;
}

// FUNCTION: 0x44e080 ??0PathOrder@@QAE@PAUUnit@@PAVBitReader@@@Z
PathOrder::PathOrder(Unit* owner_, BitReader* reader)
    : OrderFx(0), unit(owner_), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    flags = reader->ReadBits(8);
    if (flags & 1) {
        piece = reader->ReadBits(0x10);
        unsigned short index = reader->ReadBits(0x10);
        ref.SetUnit(index == 0 ? 0 : (Unit*)&g_game->units[index]);
    }
    if (flags & 0x10)
        approachRadius = reader->ReadBits(0x10);
    else
        approachRadius = 0;
    if (flags & 8)
        altitude = reader->ReadBits(0x10);
    else
        altitude = 0;
    if (flags & 0x40)
        heading = reader->ReadBits(0x10);
    else
        heading = 0;
    if (flags & 0x20) {
        pos_x = reader->ReadBits(0x20);
        pos_y = reader->ReadBits(0x20);
        pos_z = reader->ReadBits(0x20);
    }
}

// FUNCTION: 0x44e190 ??0PathOrder@@QAE@PAUOrder@@PAUUnit@@@Z
PathOrder::PathOrder(Order* order, Unit* unit)
    : OrderFx(order), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    ref.SetUnit((Unit*)unit);
    approachRadius = 0;
    altitude = 0;
    this->unit = order->unit;
    pos = this->unit->pos;
    piece = -1;
    if ((unsigned char)(ref.owner->def->flags1 >> 11) & 1) {
        field_8 = 7;
        int t = this->unit->weapons[0].weapon->range;
        if (t != 0)
            followOffset = t << 16;
        else
            followOffset = 0x640000;
    } else {
        field_8 = 1;
    }
}

// FUNCTION: 0x44e250 ??0PathOrder@@QAE@PAUOrder@@HF@Z
PathOrder::PathOrder(Order* source, int unit, short value)
    : OrderFx(source), piece(value), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    approachRadius = 0;
    altitude = 0;
    field_8 = 5;
    this->unit = source->unit;
    pos = this->unit->pos;
    ref.SetUnit((Unit*)unit);
}

// FUNCTION: 0x44e2d0 ??0PathOrder@@QAE@PAUOrder@@ABUVec3@@@Z
PathOrder::PathOrder(Order* source, const Vec3& p)
    : OrderFx(source), ref(0, 0), pos(p)
{
    SetVtable(g_pathOrderVtable);
    approachRadius = 0;
    altitude = 0;
    piece = -1;
    field_8 = 0x20;
    unit = source->unit;
}

// FUNCTION: 0x44e330 ??0PathOrder@@QAE@PAUOrder@@HABUVec3@@@Z
PathOrder::PathOrder(Order* source, int unit, const Vec3& p)
    : OrderFx(source), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    ref.SetUnit((Unit*)unit);
    pos = p;
    approachRadius = 0;
    altitude = 0;
    piece = -1;
    this->unit = source->unit;
    field_8 = 0xa3;
}

// FUNCTION: 0x44e3a0
int PathOrder::KeepAfterComplete() {
    if ((flags & 1) == 0 || ref.owner == 0) {
        return 0;
    }
    return 1;
}

static inline Vec3 Direction_0044e3c0(short angle, int scale)
{
    Vec3 v;
    v.x = -ScaledSin(angle, scale);
    v.y = 0;
    v.z = -ScaledCos(angle, scale);
    return v;
}

// FUNCTION: 0x44e3c0
int PathOrder::FillWorldPos(Vec3* out)
{
    unsigned short f = flags;
    if ((f & 1) && !(f & 0x80)) {
        if (ref.owner == 0 || (int)ref.owner->spatialBucket.get() == (int)g_game->overflowBucket.get())
            return 0;
        Vec3* p = &pos;
        *p = g_seams.units->GetPiecePosition(ref.owner, piece);
        if (flags & 2) {
            short angle = ref.owner->heading;
            if (flags & 0x40)
                angle += heading;
            Vec3 d = Direction_0044e3c0(angle, followOffset);
            p->x -= d.x;
            p->y -= d.y;
            p->z -= d.z;
        }
        pos.y += altitude << 16;
    } else {
        if ((f & 8) == 0) {
            UnitDef* def = unit->def;
            if (def->b22)
                pos.y = (def->altitude + g_game->seaLevel) << 16;
            else
                pos.y = (def->altitude + *((unsigned char*)unit->spatialBucket.get() + 1)) << 16;
        }
    }
    if (pos.y > 0x1ff0000)
        pos.y = 0x1ff0000;
    *out = pos;
    return 1;
}

// FUNCTION: 0x44e530
int PathOrder::GetDesiredHeading(unsigned short* out)
{
    unsigned short f = flags;
    if ((f & 0x84) && ref.owner != 0) {
        if (f & 2) {
            *out = GetHeadingBetween(&unit->pos, &ref.owner->pos);
            return 1;
        }
        if (f & 0x40) {
            *out = heading;
            return 1;
        }
        *out = ref.owner->heading;
        return 1;
    }
    if (f & 0x40) {
        *out = heading;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x44e5b0
int PathOrder::IsComplete(Unit* arg)
{
    Vec3 at;
    FillWorldPos(&at);
    float dist = (float)_hypot((double)(arg->pos.x - at.x), (double)(arg->pos.z - at.z)) * 1.52587890625e-05f;
    unsigned short flags = this->flags;
    if (flags & 0x10) {
        return (double)approachRadius > dist;
    }
    else {
        if (dist > 0.5f)
            return 0;
        if ((flags & 1) && ref.owner == 0)
            return 0;
        if ((flags & 4) && (unsigned short)arg->heading != (unsigned short)ref.owner->heading)
            return 0;
        if ((flags & 8) && abs(arg->pos.y - at.y) > 0x10000)
            return 0;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x44e6b0
int PathOrder::IsFxStyle()
{
    return 0;
}

#define max(a, b) (((a) > (b)) ? (a) : (b))

// FUNCTION: 0x44e6c0
void PathOrder::SetAltitude(int param_1)
{
    field_8 |= 8;
    altitude = param_1;
    if ((unsigned char)field_8 & 0x20) {
        pos.y = (max(g_seams.units->GetGroundHeight((Vec3*)&pos), g_game->seaLevel) + param_1) << 16;
        if (pos.y > 0x1ff0000)
            pos.y = 0x1ff0000;
    }
}

// FUNCTION: 0x44e720
void PathOrder::SetHeading(short v)
{
    headingFlag = 1;
    heading = v;
}

// FUNCTION: 0x44e730
void PathOrder::SetApproachRadius(short v)
{
    radiusFlag = 1;
    approachRadius = v;
}

// FUNCTION: 0x44e740 ??0AirManeuverOrder@@QAE@PAUOrder@@ABUVec3@@1@Z
AirManeuverOrder::AirManeuverOrder(Order* source, const Vec3& a, const Vec3& b)
    : OrderFx(source)
{
    SetVtable(g_airManeuverOrderVtable);
    self = source->unit;
    flags = 0;
    saveOnly22 = 0;
    target = a;
    other = b;
}

// FUNCTION: 0x44e7a0
int AirManeuverOrder::GetType()
{
    return 3;
}

// FUNCTION: 0x44e7b0
void* AirManeuverOrder::Destroy(int param_1)
{
    SetVtable(g_orderFxVtable);
    if ((param_1 & 1) != 0) {
        operator delete(this);
    }
    return this;
}

// FUNCTION: 0x44e7d0 ??0AirManeuverOrder@@QAE@HPAVHapiBank@@PAD@Z
AirManeuverOrder::AirManeuverOrder(int owner, HapiBank* file, char* name)
{
    source = (Order*)owner;
    SetVtable(g_airManeuverOrderVtable);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    AirManeuverOrderRecord hdr;
    if (file->ReadBox(&hdr, 0x2a) == 0x2a) {
        self = g_seams.units->LoadUnit(hdr.id, file);
        flags = hdr.flag;
        target = hdr.target;
        other = hdr.other;
        saveOnly22 = hdr.value_24;
        heading = hdr.heading;
        value_26 = hdr.value_28;
    }
}

// FUNCTION: 0x44e880
int AirManeuverOrder::SerializeToSave(Order* order, HapiBank* file, char* name)
{
    AirManeuverOrderRecord hdr;
    if (!self) {
        hdr.unit_id = 0;
    } else {
        hdr.unit_id = self->id;
    }
    hdr.flag = flags;
    hdr.target = target;
    hdr.other = other;
    hdr.value_24 = saveOnly22;
    hdr.heading = heading;
    hdr.value_28 = value_26;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 0x2a);
    return 1;
}

// FUNCTION: 0x44e930
void AirManeuverOrder::SerializeToBits(BitWriter* stream)
{
    stream->WriteBits(flags, 1);
    stream->WriteBits(target.x, 0x20);
    stream->WriteBits(target.y, 0x20);
    stream->WriteBits(target.z, 0x20);
    stream->WriteBits(other.x, 0x20);
    stream->WriteBits(other.y, 0x20);
    stream->WriteBits(other.z, 0x20);
    if ((flags & 1) != 0) {
        stream->WriteBits(heading, 0x10);
    }
}

// FUNCTION: 0x44e9c0 ??0AirManeuverOrder@@QAE@PAUUnit@@PAVBitReader@@@Z
AirManeuverOrder::AirManeuverOrder(Unit* owner, BitReader* reader)
{
    source = 0;
    self = owner;
    SetVtable(g_airManeuverOrderVtable);
    flags = reader->ReadBits(1);
    target.x = reader->ReadBits(0x20);
    target.y = reader->ReadBits(0x20);
    target.z = reader->ReadBits(0x20);
    other.x = reader->ReadBits(0x20);
    other.y = reader->ReadBits(0x20);
    other.z = reader->ReadBits(0x20);
    if (flags & 1) {
        heading = reader->ReadBits(0x10);
    }
}

// FUNCTION: 0x44ea50
int AirManeuverOrder::KeepAfterComplete()
{
    return 0;
}

// FUNCTION: 0x44ea60
int AirManeuverOrder::FillWorldPos(Vec3* out)
{
    *out = target;
    target.x += other.x;
    target.z += other.z;
    Vec3 v;
    v.x = 0;
    v.y = 0;
    v.z = 0;
    unsigned short h = GetHeadingBetween(&v, &other);
    if ((flags & 1) && h != heading) {
        short diff = heading - h;
        v.x = other.x;
        v.y = other.z;
        short step = diff;
        unsigned short max = self->def->turnrate;
        if (step >= (max >> 3))
            step = max >> 3;
        else if (step <= -(max >> 3))
            step = -(max >> 3);
        RotatePoint2D(-step, (int*)&v);
        other.x = v.x;
        other.z = v.y;
        other.y = 0;
    }
    return 1;
}

// FUNCTION: 0x44eb40
int AirManeuverOrder::GetDesiredHeading(unsigned short* out)
{
    *out = GetHeadingBetween(&self->pos, &target);
    return 1;
}

// FUNCTION: 0x44eb60
int AirManeuverOrder::IsComplete(Unit* unit)
{
    if ((float)_hypot(unit->pos.x - target.x, unit->pos.z - target.z) / 65536.0f < 48.0f)
        return 1;
    if (flags & 1) {
        Vec3 origin;
        origin.x = 0;
        origin.y = 0;
        origin.z = 0;
        if (GetHeadingBetween(&origin, &other) == heading)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x44ec00
int AirManeuverOrder::IsFxStyle()
{
    return 0;
}

// FUNCTION: 0x44ec10
void AirManeuverOrder::SetAltitude(int)
{
}

// FUNCTION: 0x44ec20
void AirManeuverOrder::SetHeading(short v)
{
    headingSet = 1;
    heading = v;
}

// FUNCTION: 0x44ee60 ?_Destroy@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@IAEXPAUPoint16@@0@Z
DestroyFn_0044ee60 PointVectorDestroyAccess::fn = &PointVectorDestroyAccess::_Destroy;

// FUNCTION: 0x44ee70 ?size@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@QBEIXZ
SizeFn_0044ee70 g_size_0044ee70 = &Vec_0044ee70::size;

// FUNCTION: 0x44ee90 ?_Ucopy@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@IAEPAUPoint16@@PBU3@0PAU3@@Z
UcopyFn_0044ee90 PointVectorUcopyAccess::fn = &PointVectorUcopyAccess::_Ucopy;

// FUNCTION: 0x44eec0 ?_Ufill@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@IAEXPAUPoint16@@IABU3@@Z
UfillFn_0044eec0 PointVectorUfillAccess::fn = &PointVectorUfillAccess::_Ufill;

// FUNCTION: 0x44eef0
Point16* __stdcall CopyDwordRangeUnchecked(Point16* first, Point16* last, Point16* dest)
{
    for (; first != last; ++dest, ++first)
        *dest = *first;
    return dest;
}

// FUNCTION: 0x44ef20
// FUNCTION: 0x44ef60 ??_GPathGoal@@UAEPAXI@Z
PathGoal::PathGoal(Unit* p)
{
    owner = p;
    target = 0;
}

// FUNCTION: 0x44ef40
void PathGoal::FillWaypointWorldPos(Vec3*, int, int)
{
}

// FUNCTION: 0x44ef50
void PathGoal::DrawOnSurface(void*)
{
}

// FUNCTION: 0x44ef80
int PathGoal::HasReadyWaypoints()
{
    return target != 0;
}

// FUNCTION: 0x44ef90
void PathGoal::SetPathOrder(void* param)
{
    if (target != 0) {
        target->AddFlags(0x80);
    }
    target = (OrderFx*)param;
}

// FUNCTION: 0x44efb0
void PathGoal::TickTowardGoal()
{
}

// FUNCTION: 0x44efc0
void PathGoal::SerializeNetUnitState(BitWriter*)
{
}

// FUNCTION: 0x44efd0
void PathGoal::DeserializeNetUnitState(BitReader*)
{
}

// FUNCTION: 0x44efe0
int PathGoal::HasNetUnitState()
{
    return 0;
}

// FUNCTION: 0x44eff0
AiSearchGoal* PathGoal::TryClaimRepath()
{
    return 0;
}

// FUNCTION: 0x44f000
void PathGoal::ExportGoalPose(Vec3*, Vec3*, short*)
{
}

// FUNCTION: 0x44f010
AiSearchGoal::AiSearchGoal(Unit* p)
    : PathGoal(p)
{
    count = 0;
    active = 0;
    flag_1 = 0;
    flag_3 = 1;
    repathClaimTick = 0;
}

// FUNCTION: 0x44f080
void AiSearchGoal::SetWaypoints(Point16* src, int n)
{
    if (n == 0) {
        if (target && target->ContainsUnit(owner) == 0)
            ((OrderFx*)target)->AddFlags(0x40);
        active = 0;
    } else {
        if (n >= 20)
            n = 20;
        count = n;
        std::copy(src, src + n, points);
        active = 1;
    }
    flag_1 = 0;
    flag_3 = 1;
}

// FUNCTION: 0x44f100
void AiSearchGoal::TruncateWaypointsFrom(int n)
{
    if (n != 0) {
        std::copy(points + n, points + count, points);
        count -= n;
        if (count < 2)
            active = 0;
        flag_3 = 1;
    }
}

// FUNCTION: 0x44f150
void AiSearchGoal::FillWaypointWorldPos(Vec3* out, int unused, int n)
{
    for (int i = 0; i < n; i++) {
        int j = i < count ? i : count - 1;
        out[i].x = points[j].x << 16;
        out[i].y = 0;
        out[i].z = points[j].y << 16;
    }
}

// FUNCTION: 0x44f1a0
void AiSearchGoal::TickTowardGoal()
{
    if (target) {
        if (target->ContainsUnit(owner)) {
            ((OrderFx*)target)->AddFlags(0x20);
            if (!target->KeepAfterComplete())
                SetPathOrder(0);
        }
    }
    if (count >= 2) {
        int dx = owner->zWhole - points[1].y;
        int dy = owner->xWhole - points[1].x;
        if (dy * dy + dx * dx <= 25) {
            std::copy(points + 1, points + count, points);
            int n = --count;
            if (n < 2)
                active = 0;
            flag_3 = 1;
        }
    }
    if (target && (Motion(owner)->flags & 4 || count < 2))
        flag_1 = 1;
}

// FUNCTION: 0x44f260
AiSearchGoal* AiSearchGoal::TryClaimRepath()
{
    if (flags & 2) {
        unsigned int limit = (unsigned int)g_game->ticks;
        if (limit >= repathClaimTick + 0x3c) {
            repathClaimTick = limit;
            return this;
        }
    }
    return 0;
}

// FUNCTION: 0x44f290
int AiSearchGoal::HasReadyWaypoints()
{
    return flags & 1;
}

// FUNCTION: 0x44f2a0
void AiSearchGoal::SetPathOrder(void* param)
{
    g_seams.pathfinding->AbortIfGoalMatch(g_game->pathfinder, this);
    if (target)
        target->AddFlags(0x80);
    active = 0;
    target = (OrderFx*)param;
    if (param == 0) {
        flag_1 = 0;
    } else {
        flag_1 = 1;
        if (count >= 3) {
            if (target->ContainsCell(points[count - 1].x >> 4, points[count - 1].y >> 4)) {
                active = 1;
                flag_1 = 0;
            }
        }
        if (!active) {
            Vec3 p;
            if (target->FillWorldPos(&p)) {
                if (count >= 3) {
                    int sx = points[count - 1].x << 16;
                    int sz = points[count - 1].y << 16;
                    int d1 = (int)_hypot(owner->pos.x - p.x,
                                         owner->pos.z - p.z);
                    int d2 = (int)_hypot(sx - p.x,
                                         sz - p.z);
                    if (d2 * 2 < d1)
                        active = 1;
                }
                if (!active) {
                    Order* t = owner->list;
                    if (t && !(t->flags_42 & 0x800000)) {
                        count = 2;
                        points[0].x = owner->xWhole;
                        points[0].y = owner->zWhole;
                        points[1].x = (short)(p.x >> 16);
                        points[1].y = (short)(p.z >> 16);
                        active = 1;
                    }
                }
            }
        }
    }
    if (repathClaimTick <= (unsigned int)g_game->ticks - 10)
        repathClaimTick = 0;
    flag_3 = 1;
}

// FUNCTION: 0x44f040 ??_GAiSearchGoal@@UAEPAXI@Z
// FUNCTION: 0x44f450
AiSearchGoal::~AiSearchGoal()
{
    g_seams.pathfinding->AbortIfGoalMatch(g_game->pathfinder, this);
}

// FUNCTION: 0x44f480
int AiSearchGoal::HasNetUnitState()
{
    return (flags & 8) || ((Motion(owner)->flags ^ flags) & 4);
}

// FUNCTION: 0x44f4a0
void AiSearchGoal::SerializeNetUnitState(BitWriter* stream)
{
    int n;
    if (active) {
        n = count < 3 ? count : 3;
    } else {
        n = 0;
    }
    if (Motion(owner)->flags & 4) {
        stream->data[stream->bit] |= 1 << stream->index;
    }
    stream->index++;
    if (stream->index == 0x20) {
        stream->index = 0;
        stream->bit++;
        if (stream->bit == stream->capacity) {
            stream->GrowBuffer();
        }
        stream->data[stream->bit] = 0;
    }
    stream->WriteBits(n, 2);
    for (int i = 0; i < n; i++) {
        stream->WriteBits(points[i].x, 0x10);
        stream->WriteBits(points[i].y, 0x10);
    }
    flag_2 = (Motion(owner)->flags & 4) != 0;
    flag_3 = 0;
}

// FUNCTION: 0x44f570
// FUNCTION: 0x44f590 ??_GPatrolGoal@@UAEPAXI@Z
PatrolGoal::PatrolGoal(Unit* p)
    : PathGoal(p)
{
    count = 0;
}

// FUNCTION: 0x44f5b0
int PatrolGoal::HasReadyWaypoints()
{
    return count >= 2;
}

// FUNCTION: 0x44f5c0
void PatrolGoal::DeserializeNetUnitState(BitReader* reader)
{
    Motion(owner)->flag = reader->ReadBit();
    count = reader->ReadBits(2);
    for (int i = 0; i < count; i++) {
        points[i].x = reader->ReadBits(16);
        points[i].y = reader->ReadBits(16);
    }
}

// FUNCTION: 0x44f650
void PatrolGoal::FillWaypointWorldPos(Vec3* out, int unused, int n)
{
    for (int i = 0; i < n; i++) {
        int j = i < count ? i : count - 1;
        out[i].x = points[j].x << 16;
        out[i].y = 0;
        out[i].z = points[j].y << 16;
    }
}

// FUNCTION: 0x44f6a0
void CreatePathfinder()
{
    g_game->pathfinder = new (kSim) Pathfinder;
}

// FUNCTION: 0x44f6e0
void DestroyPathfinder()
{
    g_seams.pathfinding->Destroy(g_game->pathfinder);
    g_game->pathfinder = 0;
}
