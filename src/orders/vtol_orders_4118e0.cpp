#include "../../interfaces/seams.h"
#include "win_types.h"
#include <math.h>

struct Point { short x, y; };
#include "../state/block.h"
#include "../util/vec3.h"

struct Unit;
#include "unit_motion.h"
#include "path_order_attach.h"
#include "mission_type.h"

void UpdateMouseScroll();
void UpdateEdgeScroll();
void CenterCameraOnRadarClick();
#include "../units/cob_script.h"

#include "../files/files_api.h"
#include "../sound/sound.h"
struct BmpFileHeader;
struct BmpInfo;
struct BmpInfoHeader;
struct CalcedExplosion;

#pragma pack(push, 1)
#include "../units/unit_def.h"
#include "../weapons/weapon_def.h"
#include "../units/unit.h"
unsigned short __stdcall ChooseBuildOption(unsigned int, Unit*);
void __stdcall ClearWeaponTarget(Unit*, int);
void __stdcall DetonateUnitWeapon(Unit*, int);
void __stdcall DrawUnit(void*, Unit*);

#include "order.h"
int __stdcall DirectionFromStep(int, int);
#include "path_fx.h"
#pragma pack(pop)

int __stdcall RandomInt(int);
int __cdecl ScaledSin(short, int);
int __cdecl ScaledCos(short, int);
void __stdcall QueueUnitSpeech(Unit*, int, const char*);
void __stdcall AppendOrder(Unit*, Order*);

static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin(angle, distance);
    v.y = 0;
    v.z = -ScaledCos(angle, distance);
    return v;
}

static inline Vec3 Add(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

void __stdcall PrepVtolClimb(Unit* unit, Order* order, unsigned int flags)
{
    g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
    if (unit->carrier)
        g_seams.units->AttachUnitToPiece(unit, 0, -1, 2);
    g_seams.units->SetStateBits(unit, 1, 1);
    if ((unit->motion->flags & 3) == 1) {
        unit->motion->SetFlightMode(unit, 2);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, unit->pos);
        obj->SetAltitude(unit->def->altitude / 2);
        order->SetAttachedFx(obj);
        order->flags |= flags | 0xe0;
    }
}

int __stdcall FindLandingPad(Unit* unit, int pad)
{
    if (pad != -1 && g_seams.map->IsPadSlotFree(unit, pad)) {
        return pad;
    }
    int pads[4];
    pads[0] = -1;
    pads[1] = -1;
    pads[2] = -1;
    pads[3] = -1;
    g_seams.units->QueryScript((CobScript*)unit->script.get(), "QueryLandingPad", &pads[0], &pads[1], &pads[2], &pads[3]);
    for (int i = 0; i < 4; i++) {
        if (pads[i] != -1 && g_seams.map->IsPadSlotFree(unit, pads[i])) {
            return pads[i];
        }
    }
    return -1;
}

// FUNCTION: 0x4118e0
int __stdcall VtolLandingOrder(Unit* unit, Order* order, int flags)
{
    int radius = ((WeaponDef*)unit->weapon)->range;
    Unit* host = order->target;
    if (!host) {
        QueueUnitSpeech(unit, 7, "Landing aborted");
        return 8;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            order->AnnounceStatusIfFlagged("Landing");
            PrepVtolClimb(unit, order, 0);
            order->angle = RandomInt(0x10000);
            return 1;
        }
        break;
    case 1: {
        if (FindLandingPad(order->target, FindLandingPad(host, -1)) != -1) {
            order->state = 2;
            return 2;
        }
        Vec3 dest = Add(order->target->pos, Offset(order->angle, radius << 16));
        order->angle += 0x4000;
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, dest);
        obj->SetApproachRadius(0x80);
        order->SetAttachedFx(obj);
        order->flags = 0xe8;
        order->state = 1;
        return 2;
    }
    case 2: {
        Class_0044e250* obj = new (kSim) Class_0044e250(order, order->target, -1);
        ((Class_0044e2d0*)obj)->SetApproachRadius(0xa0);
        order->SetAttachedFx(obj);
        order->flags = 0xe8;
        return 1;
    }
    case 3: {
        int pad = FindLandingPad(host, -1);
        order->angle = pad;
        if (pad == -1) {
            QueueUnitSpeech(unit, 7, "Landing failed");
            return 0;
        }
        Class_0044e250* obj = new (kSim) Class_0044e250(order, order->target, order->angle);
        ((Class_0044e2d0*)obj)->SetApproachRadius(0x30);
        order->SetAttachedFx(obj);
        order->flags = 0xe8;
        return 1;
    }
    case 4:
        return 1;
    case 5: {
        if (flags & 0x20)
            return 1;
        int pad = FindLandingPad(host, order->angle);
        order->angle = pad;
        if (pad == -1) {
            QueueUnitSpeech(unit, 7, "Landing aborted: all pads are occupied");
            return 0;
        }
        Class_0044e250* obj = new (kSim) Class_0044e250(order, order->target, order->angle);
        if (unit->cargo)
            ((Class_0044e2d0*)obj)->SetAltitude(unit->cargo->def->field_170);
        else
            ((Class_0044e2d0*)obj)->SetAltitude(0);
        g_seams.units->StartScript((CobScript*)unit->script.get(), "EndTransport", 0, 1);
        order->SetAttachedFx(obj);
        order->SetDeadlineTicks(0xf);
        order->state = 5;
        order->flags |= 0xe8;
        return 2;
    }
    case 6: {
        int dropped = 0;
        if (flags & 0x40)
            return 8;
        if (!g_seams.map->IsPadSlotFree(host, order->angle)) {
            QueueUnitSpeech(unit, 7, "Landing aborted: no pads available");
            return 0;
        }
        if (unit->cargo) {
            g_seams.units->StartScript((CobScript*)unit->script.get(), "EndTransport", 0, 0);
            g_seams.units->AttachUnitToPiece(unit->cargo, order->target, order->angle, 0);
            dropped = 1;
        } else {
            g_seams.units->AttachUnitToPiece(unit, order->target, order->angle, 0);
        }
        if (!dropped && unit->health < unit->def->maxHealth
            && (order->target->def->flags1 & 0x200)
            && (order->target->def->flags1 & 0x40)
            && order->target->buildLeft == 0.0f) {
            order->SetAttachedFx(0);
            AppendOrder(unit, new (kSim) Order("SELFREPAIR", order->target, 0, 0, 0, 0));
        }
        return 5;
    }
    }
    return 7;
}
