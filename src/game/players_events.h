#ifndef GAME_PLAYERS_EVENTS_H
#define GAME_PLAYERS_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/players.h"
#include "players_event_size.h"
#include "../../interfaces/ref.h"
#include "../util/engine_services.h"
#include "../units/unit_commands.h"

struct Player;
struct Unit;

void __stdcall KillPlayerUnits(unsigned char player);
int __stdcall GiveUnitToPlayer(Unit* unit, Player* other, const PlayersGiveUnit* event);

template <class E, class U>
inline void PlayersFillHeader(E* event, int kind, U* unit)
{
    memset(event, 0, sizeof(*event));
    event->h.kind = kind;
    event->h.unit = Ref<U>(unit).offset;
}

template <class U>
inline void PlayersPublishUnit(int outbox, int kind, U* unit)
{
    PlayersEventHeader event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.unit = Ref<U>(unit).offset;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishKillUnits(int outbox, int player)
{
    PlayersKillUnits event;
    memset(&event, 0, sizeof(event));
    event.h.kind = PLAYERS_KILL_UNITS;
    event.player = player;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishSetStore(int outbox, int player, int resource, int add, float amount)
{
    PlayersSetStore event;
    memset(&event, 0, sizeof(event));
    event.h.kind = PLAYERS_SET_STORE;
    event.player = player;
    event.resource = resource;
    event.add = add;
    event.amount = amount;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishSetType(int outbox, int player, int type)
{
    PlayersSetType event;
    memset(&event, 0, sizeof(event));
    event.h.kind = PLAYERS_SET_TYPE;
    event.player = player;
    event.type = type;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishEndGame(int outbox, int victory)
{
    PlayersEndGame event;
    memset(&event, 0, sizeof(event));
    event.h.kind = PLAYERS_END_GAME;
    event.victory = victory;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishDeactivateConditions(int outbox)
{
    PlayersEventHeader event;
    memset(&event, 0, sizeof(event));
    event.kind = PLAYERS_DEACTIVATE_CONDITIONS;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishSetCheats(int outbox, unsigned int clear, unsigned int set)
{
    PlayersSetCheats event;
    memset(&event, 0, sizeof(event));
    event.h.kind = PLAYERS_SET_CHEATS;
    event.clear = clear;
    event.set = set;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishLeaveGame(int outbox)
{
    PlayersEventHeader event;
    memset(&event, 0, sizeof(event));
    event.kind = PLAYERS_LEAVE_GAME;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void PlayersPublishGiveUnit(int outbox, U* unit, int player, const OwnershipTransferPacket* record)
{
    PlayersGiveUnit event;
    PlayersFillHeader(&event, PLAYERS_GIVE_UNIT, unit);
    event.player = player;
    if (record != nullptr) {
        event.record.present = 1;
        event.record.hitPoints = record->hitPoints;
        event.record.buildPercent = record->buildPercent;
        event.record.angles[0] = record->angles.bank;
        event.record.angles[1] = (short)record->angles.heading;
        event.record.angles[2] = record->angles.pitch;
        event.record.stockpiles[0] = record->stockpile0;
        event.record.stockpiles[1] = record->stockpile1;
        event.record.stockpiles[2] = record->stockpile2;
    }
    SchedPublish(outbox, &event, sizeof(event));
}

inline void PlayersPublishTransfer(int outbox, int from, int to, int resource, float amount, int announce)
{
    PlayersTransfer event;
    memset(&event, 0, sizeof(event));
    event.h.kind = PLAYERS_TRANSFER;
    event.from = from;
    event.to = to;
    event.resource = resource;
    event.amount = amount;
    event.announce = announce;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void PlayersPublishKillUnit(U* unit, int how)
{
    PlayersKillUnit event;
    PlayersFillHeader(&event, PLAYERS_EFFECT_KILL_UNIT, unit);
    event.selfDestruct = how;
    HarnessPlayersEvent(&event.h, sizeof(event), 1);
    SchedPublish(SCHED_PLAYERS, &event, sizeof(event));
}

template <class U>
inline void PlayersPublishSendUnit(U* unit, int player)
{
    PlayersSendUnit event;
    PlayersFillHeader(&event, PLAYERS_EFFECT_SEND_UNIT, unit);
    event.player = player;
    HarnessPlayersEvent(&event.h, sizeof(event), 1);
    SchedPublish(SCHED_PLAYERS, &event, sizeof(event));
}

template <class U>
inline void PlayersPublishCaptureUnit(U* unit, int player, const PlayersGiveUnit* give)
{
    PlayersCaptureUnit event;
    PlayersFillHeader(&event, PLAYERS_EFFECT_CAPTURE_UNIT, unit);
    event.player = player;
    event.record = give->record;
    HarnessPlayersEvent(&event.h, sizeof(event), 1);
    SchedPublish(SCHED_PLAYERS, &event, sizeof(event));
}

#endif
