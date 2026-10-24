#include "../map/mission.h"

#include "victory_conditions.h"

#include "../network/player.h"
#include "../state/block.h"
#include "../../interfaces/seams.h"

#pragma pack(push, 1)
#include "game.h"
#pragma pack(pop)

#include "../../link/globals.h"

int __fastcall CheckAlliedVictory(void* self);

inline int MissionConditions::AllVictoryConditionsMet()
{
    if (victoryCount == 0) {
        victory[victoryCount] = new (kSim) VictoryDestroyAllUnits;
        victoryCount++;
    }
    for (int i = 0; i < victoryCount; i++) {
        if (!victory[i]->IsSatisfied())
            return 0;
    }
    return 1;
}

// FUNCTION: 0x490230
int MissionConditions::CheckVictory()
{
    if (active == 0)
        return 0;
    switch (g_seams.map->GetGameType(g_game->net)) {
    case 1:
        return AllVictoryConditionsMet();
    case 2: {
        unsigned char player = g_game->localPlayer;
        Player* me = &g_game->players[player];
        for (unsigned char i = 0; i < 10; i++) {
            unsigned char j = i;
            if (i == g_game->localPlayer)
                continue;
            if (me->allied[j])
                continue;
            if (g_game->players[j].unitCount != 0)
                return 0;
        }
        return 1;
    }
    case 3:
        return CheckAlliedVictory(this);
    }
    return 0;
}
