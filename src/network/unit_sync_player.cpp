#include <map>
#include <list>
#include <vector>

#include "unit_sync_state.h"

#include "../game/game.h"

#include "../../link/globals.h"

// FUNCTION: 0x46ded0
UnitSyncPlayer::~UnitSyncPlayer()
{
}

// FUNCTION: 0x470300 ??_GUnitSyncPlayer@@QAEPAXI@Z
void FinishUnitSync()
{
    ((UnitSync *)g_game->sync)->ApplyToUnitTypes();
    if ((UnitSync *)g_game->sync)
        delete (UnitSync *)g_game->sync;
    (*(UnitSync **)&g_game->sync) = 0;
}

// FUNCTION: 0x470390
UnitSyncPlayer::UnitSyncPlayer(const UnitSyncPlayer& other)
    : id(other.id), ids(other.ids), pairs(other.pairs),
      expected(other.expected), sent(other.sent), ackd(other.ackd),
      sub(other.sub)
{
}
