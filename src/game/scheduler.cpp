#include <stddef.h>
#include <string.h>

#include "../../interfaces/seams.h"
#include "../util/engine_services.h"
#include "../network/player.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/features.h"
#include "../../interfaces/pathfinding.h"
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

const unsigned int kEventWords = 32;
const unsigned int kInboxCapacity = 1024;

struct InboxEntry {
    SchedHandler handler;
    unsigned int size;
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
        entry.handler(entry.words, entry.size);
    }
}

int __stdcall SchedInboxHolds(int module, int kind, unsigned int word, unsigned int value)
{
    Inbox* inbox = &s_inboxes[module];
    for (unsigned int i = inbox->head; i != inbox->tail; i++) {
        const InboxEntry* entry = &inbox->entries[i % kInboxCapacity];
        if (entry->words[0] == (unsigned int)kind && entry->words[word] == value)
            return 1;
    }
    return 0;
}

void __stdcall ScheduleUnitsDeliver()
{
    ReplayImport call(RI_SCHEDULE_UNITS_DELIVER);
    SchedDeliver(SCHED_UNITS);
}

void __stdcall SchedPublish(int outbox, const void* event, unsigned int size)
{
    ReplayImport call(RI_SCHED_PUBLISH);
    call.Int(outbox);
    call.Ptr(event);
    call.Int((int)size);
    if (size > sizeof(InboxEntry::words))
        FatalError((char*)"an event is larger than an inbox entry");
    if (size < sizeof(int))
        FatalError((char*)"an event is smaller than its kind");
    int kind = *static_cast<const int*>(event);
    int count;
    const SchedRoute* routes = SchedRoutes(&count);
    for (int i = 0; i < count; i++) {
        const SchedRoute* route = &routes[i];
        if (route->outbox != outbox || route->kind != kind)
            continue;
        if (route->synchronous || !s_inTick) {
            SchedDeliver(route->subscriber);
            route->handler(event, size);
            continue;
        }
        Inbox* inbox = &s_inboxes[route->subscriber];
        if (inbox->tail - inbox->head == kInboxCapacity)
            SchedDeliver(route->subscriber);
        InboxEntry* entry = &inbox->entries[inbox->tail % kInboxCapacity];
        entry->handler = route->handler;
        entry->size = size;
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
                        g_seams.units->UpdateUnitHead(u);
                        if (u->player->active != 0) {
                            unsigned char weaponsKind = u->player->type;
                            if (weaponsKind == 1 || weaponsKind == 2) {
                                SchedDeliver(SCHED_WEAPONS);
                                g_seams.weapons->UpdateUnitWeapons(u);
                            }
                        }
                        SchedDeliver(SCHED_UNITS);
                        if (u->script != 0) {
                            SchedDeliver(SCHED_SCRIPTS);
                            g_seams.scripts->RunScripts((CobScript*)u->script.get(), 1);
                        }
                        SchedDeliver(SCHED_UNITS);
                        int moved = 0;
                        if (g_seams.units->UpdateUnitBody(u)) {
                            SchedDeliver(SCHED_ORDERS);
                            g_seams.orders->RunOrders(u);
                            SchedDeliver(SCHED_ORDERS);
                            g_seams.orders->RunSecondaryOrders(u);
                            if (u->motion != 0) {
                                SchedDeliver(SCHED_MOVEMENT);
                                g_seams.movement->UpdateMotion(u);
                                moved = 1;
                            }
                        }
                        SchedDeliver(SCHED_UNITS);
                        g_seams.units->UpdateUnitTail(u, moved);
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

void SchedProjectilesPhase()
{
    SchedDeliver(SCHED_WEAPONS);
    g_seams.weapons->UpdateProjectiles();
}

void SchedExplosionsPhase()
{
    SchedDeliver(SCHED_WEAPONS);
    g_seams.weapons->UpdateExplosions();
}

void SchedFeaturesPhase()
{
#if BT_ACCESS_LOG
    AccessLogPassScope accessPass(ACCESS_PASS_FEATURES);
#endif
#if BT_ACCESS_LOG
    {
    AccessLogPassScope animPass(ACCESS_PASS_FEATURES_ANIM);
#endif
    SchedDeliver(SCHED_FEATURES);
    int types = g_seams.features->FeatureTypeCount();
    for (int k = 0; k < types; k++) {
#if BT_ACCESS_LOG
        AccessLogEntity();
#endif
        g_seams.features->AnimateFeatureType(k);
    }
#if BT_ACCESS_LOG
    AccessLogEntity();
#endif
    SchedDeliver(SCHED_FEATURES);
    g_seams.features->ScanFeatureCell();
#if BT_ACCESS_LOG
    }
#endif
    int i = g_seams.features->FirstFeatureSpot();
    while (i != -1) {
#if BT_ACCESS_LOG
        AccessLogEntity();
#endif
        SchedDeliver(SCHED_FEATURES);
        i = g_seams.features->UpdateFeatureSpot(i);
    }
}

void SchedPlayersPhase(RandStream stream)
{
#if BT_ACCESS_LOG
    AccessLogPassScope accessPass(ACCESS_PASS_PLAYERS);
#endif
    g_seams.pathfinding->Step();
    PathfindingFinished event;
    PathPoint waypoints[64];
    for (;;) {
        memset(&event, 0, sizeof event);
        int size = g_seams.pathfinding->Poll(&event.h, sizeof event, waypoints, 64);
        if (size < (int)sizeof(PathfindingEventHeader))
            break;
        unsigned int got = (unsigned int)size < sizeof event ? (unsigned int)size : sizeof event;
        HarnessPathEvent(&event.h, got, waypoints, 64);
        g_seams.movement->ApplyPathEvent(&event.h, got, waypoints);
    }
    for (int player = 0; player < 10; player++) {
        SchedDeliver(SCHED_PLAYERS);
        if (g_seams.players->UpdatePlayerHead(player) == 0)
            continue;
        SchedDeliver(SCHED_AI);
        g_seams.ai->UpdateSquads(player);
        SchedDeliver(SCHED_LOS_RADAR);
        g_seams.los_radar->RefreshUnitIndex(player);
        SchedDeliver(SCHED_UNITS);
        g_seams.units->RefreshFactoryIndex(player);
        SchedDeliver(SCHED_AI);
        g_seams.ai->UpdatePlayerAI((unsigned char)player);
        {
            Player* owner = &g_game->players[player];
#if BT_ACCESS_LOG
            AccessLogPassScope losPass(ACCESS_PASS_PLAYERS_LOS);
#endif
            Unit* first = owner->unitsBegin;
            if (first != nullptr) {
                for (Unit* u = first; u <= owner->unitsEnd; u++) {
#if BT_ACCESS_LOG
                    AccessLogEntity();
#endif
                    if (u->flags & 0x10000000) {
                        SchedDeliver(SCHED_LOS_RADAR);
                        g_seams.los_radar->UpdateUnitLineOfSight(u);
                    }
                }
            }
        }
        if ((unsigned char)player == g_game->playerIndex) {
            SchedDeliver(SCHED_LOS_RADAR);
            g_seams.los_radar->RecordRadarUnits();
        }
        SchedDeliver(SCHED_PLAYERS);
        int step = g_seams.players->UpdatePlayerBody(player, stream);
        if (step == 0)
            continue;
        if (step == 2) {
            SchedDeliver(SCHED_ECONOMY);
            g_seams.economy->UpdatePlayerEconomy(player);
        }
        if ((unsigned char)player == g_game->playerIndex) {
            SchedDeliver(SCHED_LOS_RADAR);
            g_seams.los_radar->UpdateSensorRadarAndCloak();
            SchedDeliver(SCHED_LOS_RADAR);
            g_seams.los_radar->RecordRadarMapped();
        }
        SchedDeliver(SCHED_PLAYERS);
        g_seams.players->UpdatePlayerTail(player);
    }
    SchedDeliver(SCHED_PLAYERS);
    g_seams.players->UpdatePlayersFinish();
}

void SchedExpireEyeballs()
{
    SchedDeliver(SCHED_LOS_RADAR);
    g_seams.los_radar->ExpireEyeballs();
}

void SchedWindPhase(RandStream stream)
{
    SchedDeliver(SCHED_WIND_METEORS);
    g_seams.wind_meteors->UpdateWind(stream);
}

void SchedMeteorsPhase()
{
    SchedDeliver(SCHED_WIND_METEORS);
    g_seams.wind_meteors->UpdateMeteors();
}
