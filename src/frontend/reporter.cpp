#include <string.h>
#include "../../interfaces/host.h"

struct DpspGuid {
    unsigned long data1;
    unsigned long data2;
    unsigned long data3;
    unsigned long data4;
};

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

struct OnlineReportPlayer {
    char* name;
    int id;
    int flags;
    char* side;
    int numAllies;
    OnlineReportPlayer** allies;
};

struct OnlineReportScore {
    char* name;
    int value;
};

struct OnlineReportScoreBoard {
    int score;
    int count;
    OnlineReportScore** scores;
};

#include "../../link/globals.h"
extern char DAT_00512c98[];
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
#include "../game/statistics.h"
#include "../debug/debug_lib.h"
#include "../ingame/info_panel.h"
#include "../network/online_report.h"
#include "../graphics/object_pool.h"
#include "../data/exe_only.h"

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

    if (DAT_00512c98[0] == 0) {
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
