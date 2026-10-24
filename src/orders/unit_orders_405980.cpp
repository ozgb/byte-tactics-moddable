#include <vector>
#include "../state/block.h"
#include "../util/vec3.h"
struct Unit;
#include "mission_type.h"

#pragma pack(push, 1)
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#include "order.h"
#pragma pack(pop)
#include "../../interfaces/seams.h"
class DamagedAllyCollector {
public:
    Player* owner;
    std::vector<Unit*>* units;
    Unit* self;
    DamagedAllyCollector(Player* o, std::vector<Unit*>* v, Unit* s) : owner(o), units(v), self(s) {}
    virtual void CollectDamagedAlly(Unit*);
};
#include "order_list.h"
#include "../util/math_4b6c30.h"
MissionType __stdcall GetOrderType(unsigned char, Unit*, Unit*, int);

// FUNCTION: 0x405980
int __stdcall RepairPatrolOrder(Unit* unit, Order* order, int flags)
{
    for (;;) {
        switch (order->state) {
        case 0:
            if (order->target) order->pos = order->target->pos;
            EnsurePatrolReturnOrder(unit, order);
            return 1;
        case 1: {
            if (flags & 0xe0) return 6;
            order->AttachApproachRadiusGoal(&order->pos, 16);
            order->SetDeadlineTicks(60);
            order->flags |= 0xe0;
            if (unit->player->energy >= unit->player->energyCapacity * 0.2) {
                std::vector<Unit*> units;
                int range = unit->def->range << 16;
                DamagedAllyCollector collector(unit->player, &units, unit);
                g_seams.map->VisitObjectsInRange(&unit->pos, range, (UnitVisitor*)&collector);
                if (!units.empty()) {
                    Unit* target = units[RandomInt(units.size())];
                    if (unit->player->allied[target->player->index]) {
                        MissionType kind = GetOrderType(8, unit, target, 0);
                        if (kind.index) {
                            if (IssueRepairOrder(unit, target, 0)) return 6;
                            return 3;
                        }
                    }
                }
            }
            Player* owner = unit->player;
            if (unit->player->energy < 0.2 * owner->energyCapacity ||
                unit->player->metal < unit->player->metalCapacity * 0.2) {
                Vec3 energyPos, metalPos;
                Vec3* energy = &energyPos;
                Vec3* metal = &metalPos;
                int range2 = unit->def->range << 16;
                do {} while (0);
                float energyAmount, metalAmount;
                if (g_seams.map->PickRandomReclaimableResourcesInRadius(&unit->pos, range2, &energy, &energyAmount, &metal, &metalAmount)) {
                    if (metal && unit->player->metal < unit->player->metalCapacity * 0.2) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("RECLAIM", 0, metal, 0, 0, 0));
                        order->SetAttachedFx(0);
                        order->flags = 0;
                        return 3;
                    }
                    if (energy && unit->player->energy < unit->player->energyCapacity * 0.2) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("RECLAIM", 0, energy, 0, 0, 0));
                        order->flags = 0;
                        return 3;
                    }
                    if (metal && unit->player->metal + metalAmount <= unit->player->metalCapacity) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("RECLAIM", 0, metal, 0, 0, 0));
                        order->flags = 0;
                        return 3;
                    }
                    if (energy && unit->player->energy + energyAmount <= unit->player->energyCapacity) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("RECLAIM", 0, energy, 0, 0, 0));
                        order->flags = 0;
                        return 3;
                    }
                }
            }
            return 2;
        }
        }
        return 7;
    }
}

