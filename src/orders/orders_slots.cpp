#include <stdio.h>
#include <stddef.h>

#pragma pack(push, 1)
#include "order.h"
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#pragma pack(pop)

#include "../util/vec3.h"
#include "../state/block.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/state_writer.h"
#include "mission_type.h"
#include "order_list.h"
#include "order_queue.h"
#include "order_dispatch.h"
#include "orders_seam.h"

void __stdcall OrdersRunOrders(Unit* unit)
{
    RunOrders(unit);
}

void __stdcall OrdersRunSecondaryOrders(Unit* unit)
{
    RunSecondaryOrders(unit);
}

int __stdcall OrdersGetOrderFlags(Unit* unit)
{
    return GetOrderFlags(unit);
}

MissionType __stdcall OrdersGetOrderType(unsigned char mode, Unit* unit, Unit* target, Vec3* pos)
{
    return GetOrderType(mode, unit, target, pos);
}

MissionType __stdcall OrdersMissionTypeFromName(const char* name)
{
    return MissionType(name);
}

void __stdcall OrdersRegisterAllOrderTypes(void)
{
    RegisterAllOrderTypes();
}

void __stdcall OrdersClearOrderTypeTable(void)
{
    ClearOrderTypeTable();
}

void __stdcall OrdersLoadOrders(Unit* unit, HapiBank* file, int count)
{
    char name[32];
    Ref<Order>* normal = &unit->list;
    Ref<Order>* special = &unit->list2;
    int k = 0;
    if (count > 0) {
        do {
            sprintf(name, "u%04xm%04x", unit->id, k);
            Order* p = new (kSim) Order(unit, file, name);
            if (p->flags_42 & ORDER_FLAG_SECOND_LIST) {
                *special = p;
                special = &p->next;
            } else {
                *normal = p;
                normal = &p->next;
            }
            k++;
        } while (k < count);
    }
    if (unit->list != 0)
        ((Order*)unit->list)->ReattachFxToUnit();
}

int __stdcall OrdersSaveOrders(Unit* unit, HapiBank* file)
{
    char name[32];
    int n = 0;
    Order* c = (Order*)unit->list;
    while (c != 0) {
        sprintf(name, "u%04xm%04x", unit->id, n);
        c->SerializeToSave(unit, file, name);
        c = c->next;
        n++;
    }
    c = (Order*)unit->list2;
    while (c != 0) {
        sprintf(name, "u%04xm%04x", unit->id, n);
        c->SerializeToSave(unit, file, name);
        c = c->next;
        n++;
    }
    return n;
}

void __stdcall OrdersWriteState(const StateWriter* writer)
{
}
