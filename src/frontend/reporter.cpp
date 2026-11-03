#include <string.h>
#include <stdio.h>
#include "../../interfaces/services.h"
#include "../../interfaces/host.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../game/statistics.h"
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
typedef void (__cdecl *RiTerminateFn)();

extern RiEnableFn g_riEnable;
extern RiTerminateFn g_riTerminate;

#include "reporter.h"
#include "../ingame/info_panel.h"
#include "../network/online_report.h"
#include "../graphics/object_pool.h"

// FUNCTION: 0x46bf00
int __stdcall EnableReporter(int enable)
{
    if (g_riEnable != 0) {
        return g_riEnable(enable);
    }
    return 1;
}

// FUNCTION: 0x46bf30
int __stdcall LoadReporterDll(int* count, int names)
{
    char* net;

    ShutdownScoreTables();

    net = ((char*)g_game->dplConnection);
    if (net != 0) {
        memcpy(g_game->providerGuid, net + 0x10, 16);
    }

    if (g_userNameSetting[0] == 0) {
        if (memcmp(g_game->providerGuid, &g_dpspGuidIpx, 16) == 0) goto four;
        if (memcmp(g_game->providerGuid, &g_dpspGuidSerial, 16) == 0) goto four;
        if (memcmp(g_game->providerGuid, &g_dpspGuidModem, 16) == 0) {
            return 4;
        }
    }

    if (AllocScoreTables() == 2) {
        ShutdownScoreTables();
        return 2;
    }

    g_hapinetOnlineReport = 1;
    RISetCallbacks((int)ReportGameEventCallback, (int)ShowGameMessage);
    *count = 0;

    if (g_hapinetOnlineReport != 0) {
        return 0;
    }
four:
    return 4;
}

// FUNCTION: 0x46c190
void ShutdownScoreTables()
{
    g_hapinetOnlineReport = 0;
    RISetCallbacks(0, 0);
    if (g_reporterDll != 0) {
        host_net_uninit((host_net*)g_game->session_14_1);
        if (g_riTerminate != 0)
            g_riTerminate();
    }
    for (int i = 0; i < 10; i++) {
        if (g_onlineReportPlayers != 0) {
            GameFreeThunk(g_onlineReportPlayers[i]->allies);
            GameFreeThunk(g_onlineReportPlayers[i]);
        }
        if (g_onlineReportScoreBoards != 0) {
            GameFreeThunk(g_onlineReportScoreBoards[i]->scores);
            GameFreeThunk(g_onlineReportScoreBoards[i]);
        }
        if (g_onlineReportScores != 0) {
            GameFreeThunk(g_onlineReportScores[i]);
        }
    }
    if (g_onlineReportPlayers != 0) {
        GameFreeThunk(g_onlineReportPlayers);
        g_onlineReportPlayers = 0;
    }
    if (g_onlineReportScoreBoards != 0) {
        GameFreeThunk(g_onlineReportScoreBoards);
        g_onlineReportScoreBoards = 0;
    }
    if (g_onlineReportScores != 0) {
        GameFreeThunk(g_onlineReportScores);
        g_onlineReportScores = 0;
    }
}

// FUNCTION: 0x46bce0
int AllocScoreTables()
{
    char name[64];
    int failed;
    int i;

    g_onlineReportPlayers = (OnlineReportPlayer**)GameAllocIgnoreTag("PlayersArray", 0x28);
    g_onlineReportScoreBoards = (OnlineReportScoreBoard**)GameAllocIgnoreTag("ScoreBoardsArray", 0x28);
    g_onlineReportScores = (char**)GameAllocIgnoreTag("ScoresArray", 0x28);
    if (g_onlineReportPlayers == 0)
        goto failed;
    if (g_onlineReportScoreBoards == 0)
        goto failed;
    if (g_onlineReportScores == 0)
        goto failed;
    memset(g_onlineReportPlayers, 0, 0x28);
    memset(g_onlineReportScoreBoards, 0, 0x28);
    memset(g_onlineReportScores, 0, 0x28);
    failed = 0;
    i = 0;
    while (1) {
        if (i >= 10)
            break;
        sprintf(name, "PlayerInfo%d", i);
        g_onlineReportPlayers[i] = (OnlineReportPlayer*)GameAllocIgnoreTag(name, 0x18);
        if (g_onlineReportPlayers[i] == 0) {
            failed = 1;
            break;
        }
        sprintf(name, "Allies%d", i);
        g_onlineReportPlayers[i]->allies = (OnlineReportPlayer**)GameAllocIgnoreTag(name, 0x28);
        if (g_onlineReportPlayers[i]->allies == 0) {
            failed = 1;
            break;
        }
        sprintf(name, "ScoreBoard%d", i);
        g_onlineReportScoreBoards[i] = (OnlineReportScoreBoard*)GameAllocIgnoreTag(name, 0xc);
        if (g_onlineReportScoreBoards[i] == 0) {
            failed = 1;
            break;
        }
        sprintf(name, "ppScores%d", i);
        g_onlineReportScoreBoards[i]->scores = (OnlineReportScore**)GameAllocIgnoreTag(name, 0x24);
        if (g_onlineReportScoreBoards[i]->scores == 0) {
            failed = 1;
            break;
        }
        memset(g_onlineReportScoreBoards[i]->scores, 0, 0x24);
        sprintf(name, "Scores%d", i);
        g_onlineReportScores[i] = (char*)GameAllocIgnoreTag(name, 0x48);
        if (g_onlineReportScores[i] == 0) {
            failed = 1;
            break;
        }
        memset(g_onlineReportScores[i], 0, 0x48);
        int** slot = (int**)g_onlineReportScoreBoards[i]->scores;
        int j = 0;
        while (1) {
            if (j >= 0x48)
                break;
            *slot = (int*)(g_onlineReportScores[i] + j);
            j += 8;
            slot++;
        }
        i++;
    }
    if (!failed)
        return 0;
failed:
    ShutdownScoreTables();
    return 1;
}
