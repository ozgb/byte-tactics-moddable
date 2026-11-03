#include <string.h>
#include <stdio.h>
#include "../../interfaces/host.h"

#pragma pack(push, 1)
#include "game.h"
#pragma pack(pop)

#include "statistics.h"
#include "../../link/globals.h"
extern char g_userNameSetting[];
extern DpspGuid g_dpspGuidIpx;
extern DpspGuid g_dpspGuidSerial;
extern DpspGuid g_dpspGuidModem;
extern OnlineReportPlayer** g_onlineReportPlayers;
extern OnlineReportScoreBoard** g_onlineReportScoreBoards;
extern char** g_onlineReportScores;
extern int g_reporterDll;

typedef int (__stdcall *RiEnableFn)(int);
typedef int (__stdcall *RiGetVersionFn)(char*);
typedef int (__stdcall *RiInitializeExFn)(int*, int);
typedef int (__stdcall *RiReportFn)(int, int, int, int, int, int, int, int, int, int);
typedef int (__stdcall *RiReportGameChatFn)(int, char*);
typedef void (__stdcall *RiSetCallbacksFn)(void (__stdcall* cb1)(int), int (__stdcall* cb2)(char*, int));
typedef int (__stdcall *RiIntervalTimerFn)();
typedef void (__cdecl *RiTerminateFn)();

extern RiInitializeExFn g_riInitializeEx;
extern RiGetVersionFn g_riGetVersion;
extern RiSetCallbacksFn g_riSetCallbacks;
extern RiIntervalTimerFn g_riIntervalTimer;
extern RiReportFn g_riReport;
extern RiReportGameChatFn g_riReportGameChat;

#include "../frontend/reporter.h"
#include "../debug/debug_lib.h"
#include "../ingame/info_panel.h"
#include "../network/online_report.h"
#include "../graphics/object_pool.h"
#include "../map/mission.h"
#include "../network/player_info.h"
#include "../data/exe_only.h"

// FUNCTION: 0x46bf20
bool IsReporterDllLoaded()
{
    return g_reporterDll != 0;
}

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
            OnlineReportPlayer** allies = g_onlineReportPlayers[i]->allies;
            OnlineReportPlayer** a = allies;
            for (j = 0; j < 10; j++) {
                if (j != i && p->allied[j]) {
                    n++;
                    *a++ = g_onlineReportPlayers[j];
                }
            }
            g_onlineReportPlayers[i]->numAllies = n;
            for (; n < 10; n++)
                allies[n] = 0;
            Mission* c = g_game->net;
            int score = (int)((unsigned int)g_game->ticks / 60 * c->timeMul);
            score += (int)(p->kills * c->killMul);
            g_onlineReportScoreBoards[i]->score = score;
            g_onlineReportScoreBoards[i]->count = 9;
            OnlineReportScore* s;
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
            s->value = (i == g_game->localPlayer || (p->active && p->type == 2)) ? g_game->endGameVictory : 0;
        }
    }
    return count;
}
