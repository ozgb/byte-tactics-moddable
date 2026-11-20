#include <stddef.h>
#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/players.h"
#include "../../interfaces/services.h"
#include "../game/players_events.h"
#include "players_world.h"

void PlayersInbox(const void* event, unsigned int size)
{
    const PlayersEventHeader* e = static_cast<const PlayersEventHeader*>(event);
    HarnessPlayersEvent(e, size, g_seams.players->Apply(e, size));
}

void PlayersOnEconomyShare(const void* event, unsigned int size)
{
    if (size < sizeof(EconomyShare))
        return;
    const EconomyShare* effect = static_cast<const EconomyShare*>(event);
    PlayersTransfer transfer;
    memset(&transfer, 0, sizeof(transfer));
    transfer.h.kind = PLAYERS_TRANSFER;
    transfer.from = effect->h.player;
    transfer.to = effect->to;
    transfer.announce = 1;
    if (effect->resources & ECON_METAL) {
        transfer.resource = 1;
        transfer.amount = effect->metal;
    } else {
        transfer.resource = 0;
        transfer.amount = effect->energy;
    }
    PlayersInbox(&transfer, sizeof(transfer));
}
