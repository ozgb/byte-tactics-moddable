#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/players.h"
#include "../../interfaces/services.h"
#include "../game/players_events.h"
#include "players_world.h"

#include "../../link/globals.h"

void PlayersInbox(const void* event)
{
    const PlayersEvent* e = static_cast<const PlayersEvent*>(event);
    HarnessPlayersEvent(e, g_seams.players->Apply(e));
}

void PlayersOnEconomyShare(const void* event)
{
    const EconomyEvent* effect = static_cast<const EconomyEvent*>(event);
    PlayersEvent transfer = PlayersEventOf(PLAYERS_TRANSFER);
    transfer.from = effect->player;
    transfer.player = effect->value;
    transfer.announce = 1;
    if (effect->resources & ECON_METAL) {
        transfer.value = 1;
        transfer.amount = effect->metal;
    } else {
        transfer.value = 0;
        transfer.amount = effect->energy;
    }
    PlayersInbox(&transfer);
}
