#include <stddef.h>
#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/services.h"
#include "../../interfaces/ref.h"
#include "../network/player.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/rand_stream.h"
#include "../units/unit.h"
#include "game.h"
#include "scheduler.h"
#include "../harness/replay.h"
#include "../harness/replay_services.h"

#include "../../link/globals.h"

#if BT_ACCESS_LOG
#include "../../lib/accesslog/access_log.h"
#endif

namespace {

const unsigned int kEventWords = 20;
const unsigned int kInboxCapacity = 1024;

struct InboxEntry {
    SchedHandler handler;
    unsigned int words[kEventWords];
};

struct Inbox {
    InboxEntry entries[kInboxCapacity];
    unsigned int head;
    unsigned int tail;
};

Inbox s_inboxes[SCHED_MODULE_COUNT];
int s_inTick;

}

void __stdcall SchedDeliver(int module)
{
    Inbox* inbox = &s_inboxes[module];
    while (inbox->head != inbox->tail) {
        InboxEntry entry = inbox->entries[inbox->head % kInboxCapacity];
        inbox->head++;
        entry.handler(entry.words);
    }
}

void __stdcall SchedPublish(int outbox, const void* event, unsigned int size)
{
    ReplayImport call(RI_SCHED_PUBLISH);
    call.Int(outbox);
    call.Ptr(event);
    call.Int((int)size);
    if (size > sizeof(InboxEntry::words))
        FatalError((char*)"an event is larger than an inbox entry");
    int kind = *static_cast<const int*>(event);
    for (int i = 0; i < g_schedRouteCount; i++) {
        const SchedRoute* route = &g_schedRoutes[i];
        if (route->outbox != outbox || route->kind != kind)
            continue;
        if (route->synchronous || !s_inTick) {
            SchedDeliver(route->subscriber);
            route->handler(event);
            continue;
        }
        Inbox* inbox = &s_inboxes[route->subscriber];
        if (inbox->tail - inbox->head == kInboxCapacity)
            SchedDeliver(route->subscriber);
        InboxEntry* entry = &inbox->entries[inbox->tail % kInboxCapacity];
        entry->handler = route->handler;
        memset(entry->words, 0, sizeof(entry->words));
        memcpy(entry->words, event, size);
        inbox->tail++;
    }
}

static int SchedPending()
{
    for (int module = 0; module < SCHED_MODULE_COUNT; module++)
        if (s_inboxes[module].head != s_inboxes[module].tail)
            return 1;
    return 0;
}

void SchedBeginTick()
{
    s_inTick = 1;
}

void SchedPhaseBoundary()
{
    while (SchedPending())
        for (int module = 0; module < SCHED_MODULE_COUNT; module++)
            SchedDeliver(module);
}

void SchedEndTick()
{
    SchedPhaseBoundary();
    if (SchedPending())
        FatalError((char*)"an inbox holds an event at the tick boundary");
    s_inTick = 0;
}

void SchedUnitsPhase()
{
#if BT_ACCESS_LOG
    AccessLogPassScope accessPass(ACCESS_PASS_UNITS_PLAYERS);
#endif
    g_game->f14353 = 0;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active == 0)
            continue;
        unsigned char k = p->type;
        if (k != 1 && k != 2 && k != 3)
            continue;
        if (p->index == 0xa)
            continue;
#if BT_ACCESS_LOG
        AccessLogEntity();
#endif
        {
            Unit* last = p->unitsEnd;
            Unit* u = p->unitsBegin;
#if BT_ACCESS_LOG
            {
                AccessLogPassScope unitsPass(ACCESS_PASS_UNITS);
#endif
                while (u <= last) {
                    if (u->unitDefIndex != 0) {
#if BT_ACCESS_LOG
                        AccessLogEntity();
#endif
                        SchedDeliver(SCHED_UNITS);
                        g_seams.units->UpdateUnit(u);
                    }
                    u = (Unit*)((char*)u + 0x118);
                }
#if BT_ACCESS_LOG
            }
#endif
            if (g_game->netGameFlags & 1) {
                if (p->active != 0) {
                    unsigned char k4 = p->type;
                    if (k4 == 1 || k4 == 2) {
                        p->syncTick = g_game->ticks;
                        g_seams.network->SendUnitStates(p);
                    }
                }
            }
        }
    }
}

void SchedPlayersPhase(RandStream stream)
{
#if BT_ACCESS_LOG
    AccessLogPassScope accessPass(ACCESS_PASS_PLAYERS);
#endif
    g_seams.pathfinding->Step();
    PathEvent event;
    PathPoint waypoints[64];
    while (g_seams.pathfinding->Poll(&event, waypoints, 64)) {
        HarnessPathEvent(&event, waypoints, 64);
        g_seams.orders->ApplyPathEvent(&event, waypoints);
    }
    for (int player = 0; player < 10; player++) {
        SchedDeliver(SCHED_PLAYERS);
        int step = g_seams.players->UpdatePlayerHead(player, stream);
        if (step == 0)
            continue;
        if (step == 2) {
            SchedDeliver(SCHED_ECONOMY);
            g_seams.economy->UpdatePlayerEconomy(player);
        }
        SchedDeliver(SCHED_PLAYERS);
        g_seams.players->UpdatePlayerTail(player);
    }
    SchedDeliver(SCHED_PLAYERS);
    g_seams.players->UpdatePlayersFinish();
}
