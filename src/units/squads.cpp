#include <vector>
#include "../state/sim_allocator.h"
#include "../state/block.h"
#include <algorithm>

#include "squad.h"

#include "../orders/mission_type.h"

#pragma pack(push, 1)
struct Unit;

#include "../network/player.h"

#include "unit.h"
#pragma pack(pop)

void __cdecl GameFreeThunk(int* p);
#include "../../interfaces/seams.h"

// FUNCTION: 0x480190
void __stdcall CreateSquads(Player* owner)
{
    owner->groups = (Group*)SimAllocTagged("SQUADS", 10 * sizeof(Squad));
    for (int i = 0; i < 10; i++)
    {
        Squad* squad = &((Squad*)owner->groups.get())[i];
        squad->owner = owner;
        squad->index = i;
        squad->field_8 = 0;
        squad->field_c = 0;
        new (&squad->items) SimVector<int>;
    }
}

// FUNCTION: 0x4801f0
void __stdcall FreeSquads(Player* owner)
{
    if (owner->groups) {
        for (int i = 0; i < 10; i++)
            ((Squad*)owner->groups.get())[i].items.~vector();
        GameFreeThunk((int*)owner->groups.get());
        owner->groups = 0;
    }
}

static inline void RemoveFast(SimVector<Unit*>& v, Unit* u)
{
    SimVector<Unit*>::iterator it = std::find(v.begin(), v.end(), u);
    if (it != v.end()) {
        SimVector<Unit*>::iterator last = v.end() - 1;
        *it = *last;
        v.erase(last);
    }
}

// FUNCTION: 0x480250
void __stdcall SetUnitSquad(Unit* unit, int index)
{
    if (unit->group != -1)
        RemoveFast((SimVector<Unit*>&)((Squad*)((Player *)unit->player)->groups.get())[unit->group].items, unit);
    if (index != -1)
        ((SimVector<Unit*>&)((Squad*)((Player *)unit->player)->groups.get())[index].items).push_back(unit);
    unit->group = index;
}

// FUNCTION: 0x480460
void __stdcall OrderSquad(Player* owner, int key, unsigned char mode, int remove,
                            Unit* target, int flags, int param_7, int param_8)
{
    for (Unit* u = owner->unitsBegin; u <= owner->unitsEnd; u++) {
        if (((short)u->unitDefIndex) != 0 && u->group == key) {
            MissionType kind = g_seams.orders->GetOrderType(mode, u, target, (Vec3*)flags);
            g_seams.orders->AddOrder(kind, remove, u, target, (Vec3*)flags, param_7, param_8);
        }
    }
}
