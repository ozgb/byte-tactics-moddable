#include "win_types.h"
#include <string.h>

#include "../map/mission.h"

#include "../network/player.h"
#include "../network/player_info.h"
#include "endgame_41dc20.h"

#pragma pack(push, 1)
struct Stat_0041dc20 {
    char name[30];
    int kills;
    int losses;
    int energyProduced;
    int metalProduced;
    int energyWasted;
    int metalWasted;
    int score;
};

struct Game_0041dc20 {
    char unknown_0[0x1b63];
    Player players[10];
    char pad_2851[0x38a47 - 0x2851];
    unsigned int gameTick;
    char unknown_38a4b[0x38dd9 - 0x38a4b];
    Stat_0041dc20 stats[10];
    char pad_3901d[0x3918f - 0x3901d];
    int maxKills;
    int maxLosses;
    int max_26;
    int max_2a;
    int max_2e;
    int max_32;
    int maxScore;
    int mission;
    int won;
    char unknown_391b3[0x391cf - 0x391b3];
    char results[0x391e9 - 0x391cf];
    Ref<Mission> mapInfo;
    char unknown_391ed[0x3923b - 0x391ed];
    unsigned short bits0_3923b : 2;
    unsigned short bit2_3923b : 1;
    unsigned short bit3_3923b : 1;
    unsigned short bit4_3923b : 1;
    unsigned short bits5_3923b : 11;
};
#pragma pack(pop)

extern Game_0041dc20* g_game;

// FUNCTION: 0x41dc20
void CollectEndGameStats()
{
    Stat_0041dc20* stats = g_game->stats;
    g_game->won = g_game->bit4_3923b;
    if (g_game->mapInfo->GetGameType() == 1) {
        g_game->mission = g_game->mapInfo->GetMissionIndex();
        g_game->results[g_game->mission] = g_game->won ? 'W' : 'L';
    }
    g_game->maxKills = 10;
    g_game->maxLosses = 10;
    g_game->max_26 = 100;
    g_game->max_2a = 100;
    g_game->max_2e = 100;
    g_game->max_32 = 100;
    g_game->maxScore = 100;
    memset(stats, 0, sizeof g_game->stats);
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if ((p->active && (p->type == 1 || p->type == 2 || p->type == 3) && p->index != 10 && !p->info->bit6) || p->unitsCreated) {
            if (p->rejectReason == 0) {
                strncpy(stats[i].name, p->name, 30);
                stats[i].kills = p->kills;
                stats[i].losses = p->losses;
                stats[i].energyProduced = (int)p->totalEnergyProduced;
                stats[i].metalProduced = (int)p->totalMetalProduced;
                stats[i].energyWasted = (int)p->energyWasted;
                stats[i].metalWasted = (int)p->metalWasted;
                Mission* c = g_game->mapInfo;
                int t = (int)(g_game->gameTick / 60 * c->timeMul);
                stats[i].score = t + (int)(stats[i].kills * c->killMul);
                if (stats[i].score < 0)
                    stats[i].score = 0;
                if (stats[i].kills > g_game->maxKills)
                    g_game->maxKills = stats[i].kills;
                if (stats[i].losses > g_game->maxLosses)
                    g_game->maxLosses = stats[i].losses;
                if (stats[i].energyProduced > g_game->max_26)
                    g_game->max_26 = stats[i].energyProduced;
                if (stats[i].metalProduced > g_game->max_2a)
                    g_game->max_2a = stats[i].metalProduced;
                if (stats[i].energyWasted > g_game->max_2e)
                    g_game->max_2e = stats[i].energyWasted;
                if (stats[i].metalWasted > g_game->max_32)
                    g_game->max_32 = stats[i].metalWasted;
                if (stats[i].score > g_game->maxScore)
                    g_game->maxScore = stats[i].score;
            }
        }
    }
}
