#include "win_types.h"
#include <string.h>

#include "../map/mission.h"

#include "../network/player.h"

#pragma pack(push, 1)
#include "../network/player_info.h"

struct Game_0046c2a0 {
    char unknown_0[0x1b63];
    Player players[10];
    char pad_2851[0x2a42 - 0x2851];
    unsigned char localPlayer;
    char unknown_2a43[0x38a47 - 0x2a43];
    unsigned int gameTick;
    char unknown_38a4b[0x391e9 - 0x38a4b];
    Ref<Mission> mapInfo;
    char unknown_391ed[0x3923b - 0x391ed];
    unsigned short bits0_3923b : 2;
    unsigned short bit2_3923b : 1;
    unsigned short bit3_3923b : 1;
    unsigned short bit4_3923b : 1;
    unsigned short bits5_3923b : 11;
};
#pragma pack(pop)

struct PlayerInfo_0046c2a0 {
    char* name;
    int id;
    int flags;
    char* side;
    int numAllies;
    PlayerInfo_0046c2a0** allies;
};

struct Score_0046c2a0 {
    char* name;
    int value;
};

struct ScoreBoard_0046c2a0 {
    int score;
    int count;
    Score_0046c2a0** scores;
};

extern Game_0046c2a0* g_game;
extern PlayerInfo_0046c2a0** g_onlineReportPlayers;
extern ScoreBoard_0046c2a0** g_onlineReportScoreBoards;
#include "../data/exe_only.h"

// FUNCTION: 0x46c2a0
int FillScoreTables()
{
    int j;
    int count = 0;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if ((p->active && (p->type == 1 || p->type == 2 || p->type == 3) && p->index != 10) || p->unitsCreated || p->active) {
            count++;
            g_onlineReportPlayers[i]->name = p->name;
            g_onlineReportPlayers[i]->id = p->id;
            g_onlineReportPlayers[i]->flags = 1;
            if ((p->active && p->type == 2) || (p->active && p->type == 3 && p->info->kind == 2))
                g_onlineReportPlayers[i]->flags |= 2;
            if (p->active && p->info->bit6)
                g_onlineReportPlayers[i]->flags |= 4;
            if (p->active && (p->info->flags_97 & 1))
                g_onlineReportPlayers[i]->flags |= 8;
            g_onlineReportPlayers[i]->side = g_sideNames[p->info->side];
            int n = 0;
            PlayerInfo_0046c2a0** allies = g_onlineReportPlayers[i]->allies;
            PlayerInfo_0046c2a0** a = allies;
            for (j = 0; j < 10; j++) {
                if (j != i && p->allied[j]) {
                    n++;
                    *a++ = g_onlineReportPlayers[j];
                }
            }
            g_onlineReportPlayers[i]->numAllies = n;
            for (; n < 10; n++)
                allies[n] = 0;
            Mission* c = g_game->mapInfo;
            int score = (int)(g_game->gameTick / 60 * c->timeMul);
            score += (int)(p->kills * c->killMul);
            g_onlineReportScoreBoards[i]->score = score;
            g_onlineReportScoreBoards[i]->count = 9;
            Score_0046c2a0* s;
            s = g_onlineReportScoreBoards[i]->scores[0];
            s->name = g_scoreNames[0];
            s->value = p->kills;
            s = g_onlineReportScoreBoards[i]->scores[1];
            s->name = g_scoreNames[1];
            s->value = p->losses;
            s = g_onlineReportScoreBoards[i]->scores[2];
            s->name = g_scoreNames[2];
            s->value = (int)p->totalEnergyProduced;
            s = g_onlineReportScoreBoards[i]->scores[3];
            s->name = g_scoreNames[3];
            s->value = (int)p->totalMetalProduced;
            s = g_onlineReportScoreBoards[i]->scores[4];
            s->name = g_scoreNames[4];
            s->value = (int)p->energyWasted;
            s = g_onlineReportScoreBoards[i]->scores[5];
            s->name = g_scoreNames[5];
            s->value = (int)p->metalWasted;
            s = g_onlineReportScoreBoards[i]->scores[6];
            s->name = g_scoreNames[6];
            s->value = p->commanderKills;
            s = g_onlineReportScoreBoards[i]->scores[7];
            s->name = g_scoreNames[7];
            s->value = p->commanderLosses;
            s = g_onlineReportScoreBoards[i]->scores[8];
            s->name = g_scoreNames[8];
            s->value = (i == g_game->localPlayer || (p->active && p->type == 2)) ? g_game->bit4_3923b : 0;
        }
    }
    return count;
}
