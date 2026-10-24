#include <stdio.h>

#include "../state/block.h"
#include "box.h"
#include "vtol_orders_414770.h"

struct Unit;

#include "unit_motion.h"


#pragma pack(push, 1)
#include "../map/feature.h"

#include "../units/unit_def.h"

#include "../units/unit.h"
int __stdcall FindLandingPad(Unit*, int);
void __stdcall DrawUnit(void*, Unit*);

#include "order.h"

#include "path_fx.h"
#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../../interfaces/seams.h"
#include "../sound/sound.h"
void __stdcall EmitReverseNanoParticles(Box* from, Vec3* to, int count);

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

// FUNCTION: 0x414770
int __stdcall VtolReclaimOrder(Unit* unit, Order* order, int flags)
{
    Point16 cell;
    Point16 size;
    unsigned short index = g_seams.weapons->FindFeatureAtPos(&order->pos, &cell, &size);
    if (index == 0xffff) {
        QueueUnitSpeech(unit, 7, "Reclamation failed");
        return 8;
    }
    Feature* f = &g_game->features[index];
    if (!(f->flags & 0x80))
        return 8;
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800) && (unit->def->flags2 & 0x400)) {
            order->AnnounceStatusIfFlagged("Reclaiming");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->pos);
        order->SetAttachedFx(obj);
        order->flags = 0xe0;
        return 1;
    }
    case 2:
        if (flags & 0x40)
            return 8;
        order->time = (int)(30.0f - (f->value + f->metal) * -0.5f);
        QueueUnitSpeech(unit, 11, 0);
        return 1;
    case 3:
        order->SetDeadlineTicks(2);
        order->time -= 2;
        if (order->time <= 0)
            return 1;
        unit->workTime = g_game->ticks + 300;
        if (order->time > 30) {
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(unit, &nano);
            Box box;
            box.lo.x = cell.x << 20;
            box.lo.z = cell.y << 20;
            box.lo.y = g_seams.units->GetGroundHeight(&box.lo) << 16;
            box.hi = box.lo;
            box.hi.x += f->footprintX << 20;
            box.hi.z += f->footprintZ << 20;
            box.hi.y += f->height << 16;
            EmitReverseNanoParticles(&box, &nano, 6);
            EmitReverseNanoParticles(&box, &nano, 6);
        }
        return 2;
    case 4:
        g_seams.features->ReclaimFeature(unit, &order->pos);
        return 5;
    }
    return 7;
}
