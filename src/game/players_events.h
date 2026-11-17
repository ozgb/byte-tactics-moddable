#ifndef GAME_PLAYERS_EVENTS_H
#define GAME_PLAYERS_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/players.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../units/unit_commands.h"

inline PlayersEvent PlayersEventOf(int kind)
{
    PlayersEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    return event;
}

template <class U>
inline PlayersEvent PlayersEventOf(int kind, U* unit)
{
    PlayersEvent event = PlayersEventOf(kind);
    event.unit = Ref<U>(unit).offset;
    return event;
}

template <class U>
inline void PlayersPublishUnit(int outbox, int kind, U* unit)
{
    PlayersEvent event = PlayersEventOf(kind, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishKillUnits(int outbox, int player)
{
    PlayersEvent event = PlayersEventOf(PLAYERS_KILL_UNITS);
    event.player = player;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void PlayersPublishGiveUnit(int outbox, U* unit, int player, const OwnershipTransferPacket* record)
{
    PlayersEvent event = PlayersEventOf(PLAYERS_GIVE_UNIT, unit);
    event.player = player;
    if (record != nullptr) {
        event.record = 1;
        event.hitPoints = record->hitPoints;
        event.buildPercent = record->buildPercent;
        event.angles[0] = record->angles.bank;
        event.angles[1] = (short)record->angles.heading;
        event.angles[2] = record->angles.pitch;
        event.stockpiles[0] = record->stockpile0;
        event.stockpiles[1] = record->stockpile1;
        event.stockpiles[2] = record->stockpile2;
    }
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishTransfer(int outbox, int from, int to, int resource, float amount, int announce)
{
    PlayersEvent event = PlayersEventOf(PLAYERS_TRANSFER);
    event.from = from;
    event.player = to;
    event.value = resource;
    event.amount = amount;
    event.announce = announce;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void PlayersPublishKillUnit(U* unit, int how)
{
    PlayersEvent event = PlayersEventOf(PLAYERS_EFFECT_KILL_UNIT, unit);
    event.value = how;
    HarnessPlayersEvent(&event, 1);
    SchedPublish(SCHED_PLAYERS, &event, sizeof(event));
}

template <class U>
inline void PlayersPublishSendUnit(U* unit, int player)
{
    PlayersEvent event = PlayersEventOf(PLAYERS_EFFECT_SEND_UNIT, unit);
    event.player = player;
    HarnessPlayersEvent(&event, 1);
    SchedPublish(SCHED_PLAYERS, &event, sizeof(event));
}

template <class U>
inline void PlayersPublishCaptureUnit(U* unit, int player, const PlayersEvent* record)
{
    PlayersEvent event = PlayersEventOf(PLAYERS_EFFECT_CAPTURE_UNIT, unit);
    event.player = player;
    event.record = record->record;
    event.hitPoints = record->hitPoints;
    event.buildPercent = record->buildPercent;
    memcpy(event.angles, record->angles, sizeof(event.angles));
    memcpy(event.stockpiles, record->stockpiles, sizeof(event.stockpiles));
    HarnessPlayersEvent(&event, 1);
    SchedPublish(SCHED_PLAYERS, &event, sizeof(event));
}

#endif
