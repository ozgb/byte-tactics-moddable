#include <stdio.h>
#include <stddef.h>

#pragma pack(push, 1)
#include "order.h"
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#include "../game/game.h"
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
#include "../../link/globals.h"

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

static const char* const kOrderFields[2][12] = {
    {"list[%u].kind", "list[%u].state", "list[%u].flags6", "list[%u].wakeFrame", "list[%u].start.x", "list[%u].start.y", "list[%u].arg0", "list[%u].arg1",
     "list[%u].arg2", "list[%u].created", "list[%u].subFlags", "list[%u].attached"},
    {"list2[%u].kind", "list2[%u].state", "list2[%u].flags6", "list2[%u].wakeFrame", "list2[%u].start.x", "list2[%u].start.y", "list2[%u].arg0", "list2[%u].arg1",
     "list2[%u].arg2", "list2[%u].created", "list2[%u].subFlags", "list2[%u].attached"},
};

static void WriteChain(const StateWriter* w, Order* order, int which)
{
    for (unsigned int n = 0; order != nullptr && n < 4096; n++) {
        const char* const* names = kOrderFields[which];
        w->Int(names[0], (int)n, 1, order->kind.index);
        w->Int(names[1], (int)n, 1, order->state);
        w->Int(names[2], (int)n, 4, order->flags);
        w->Int(names[3], (int)n, 4, order->wakeFrame);
        w->Int(names[4], (int)n, 2, (unsigned short)order->start.x);
        w->Int(names[5], (int)n, 2, (unsigned short)order->start.y);
        w->Int(names[6], (int)n, 4, (unsigned int)order->type);
        w->Int(names[7], (int)n, 4, (unsigned int)order->radius);
        w->Int(names[8], (int)n, 4, (unsigned int)order->progress);
        w->Int(names[9], (int)n, 4, order->created);
        w->Int(names[10], (int)n, 4, order->subFlags);
        w->Handle(names[11], (int)n, STATE_HANDLE_HEAP, order->attached.offset);
        order = order->next.get();
    }
}

void __stdcall OrdersWriteState(const StateWriter* w)
{
    Unit* units = g_game->units.get();
    char name[64];
    for (unsigned int i = 0; units != nullptr && i < g_game->poolCount; i++) {
        Unit* u = &units[i];
        if (u->unitDefIndex == 0 || (u->list.offset == 0 && u->list2.offset == 0))
            continue;
        sprintf(name, "ordersState.units[%u]", i);
        w->Begin(52, name);
        WriteChain(w, u->list.get(), 0);
        WriteChain(w, u->list2.get(), 1);
    }
}
