#include "win_types.h"

#include "box.h"

#pragma pack(push, 1)
#include "../map/feature.h"

#include "../units/unit_def.h"

#include "../units/unit.h"

#include "order.h"

#include "../game/game.h"

#include "../../interfaces/seams.h"
#pragma pack(pop)

#include "../../link/globals.h"

void __stdcall QueueUnitSpeech(Unit* unit, int kind, char* text);
#include "../util/math_4b6c30.h"
unsigned short __stdcall GetHeadingBetween(Vec3* from, Vec3* to);
void __stdcall StartBuildingScript(Unit* unit, Order* order, short turn);
int __stdcall WaitIfNotInBuildStance(Unit* unit, Order* order, int flags);
void __stdcall EmitReverseNanoParticles(Box* from, Vec3* to, int count);
void __stdcall AddOrder(int kind, int remove, Unit* owner, void* id, Vec3* pos, int typeId, int amount);

// FUNCTION: 0x404ad0
int __stdcall ReclaimOrder(Unit* unit, Order* order, int flags)
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
        if (unit->motion && (unit->def->flags2 & 0x400)) {
            order->AttachBuildFootprintMarker(cell, size);
            order->flags = 0xe0;
            return 1;
        }
        break;
    case 1: {
        if (flags & 0x40)
            return 8;
        order->time = (int)(15.0f - (f->metal + f->value) * -0.5f);
        Vec3 pos;
        pos.x = (size.x + cell.x * 2) << 19;
        pos.z = (size.y + cell.y * 2) << 19;
        pos.y = (RandomInt(f->height) + g_seams.units->GetGroundHeight(&pos)) << 16;
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &pos) - (unsigned short)unit->heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 0);
    case 3:
        QueueUnitSpeech(unit, 11, 0);
    case 4:
        order->SetDeadlineTicks(2);
        order->time -= 2;
        if (order->time <= 0)
            return 1;
        unit->workTime = g_game->ticks + 300;
        if (order->time > 15) {
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
    case 5:
        g_seams.features->ReclaimFeature(unit, &order->pos);
        return 5;
    }
    return 7;
}
