#include <string.h>
#include <stdio.h>
#include "../../interfaces/host.h"

struct Guid_0046bf30 {
    unsigned long data1;
    unsigned long data2;
    unsigned long data3;
    unsigned long data4;
};

#pragma pack(push, 1)
#include "game.h"
#pragma pack(pop)

struct PlayerInfo_0046bce0 {
    char unknown_0[0x14];
    void* allies;
};

struct ScoreBoard_0046bce0 {
    char unknown_0[8];
    void* ppScores;
};

extern Game* g_game;
extern char DAT_00512c98[];
extern Guid_0046bf30 g_dpspGuidIpx;
extern Guid_0046bf30 g_dpspGuidSerial;
extern Guid_0046bf30 g_dpspGuidModem;
extern PlayerInfo_0046bce0** g_onlineReportPlayers;
extern ScoreBoard_0046bce0** g_onlineReportScoreBoards;
extern char** g_onlineReportScores;
extern int g_hapinetOnlineReport;
extern int g_reporterDll;

typedef int (__stdcall *EnableFn_0046bf30)(int);
typedef int (__stdcall *VersionFn_0046bf30)(char*);
typedef int (__stdcall *InitFn_0046bf30)(int*, int);
typedef int (__stdcall *ReportFn_0046bf30)(int, int, int, int, int, int, int, int, int, int);
typedef int (__stdcall *ChatFn_0046bf30)(int, char*);
typedef void (__stdcall *SetCbFn_0046bf30)(void (__stdcall* cb1)(int), int (__stdcall* cb2)(char*, int));
typedef int (__stdcall *TimerFn_0046bf30)();
typedef void (__cdecl *TermFn_0046bf30)();

extern EnableFn_0046bf30 g_riEnable;
extern InitFn_0046bf30 g_riInitializeEx;
extern VersionFn_0046bf30 g_riGetVersion;
extern TermFn_0046bf30 g_riTerminate;
extern SetCbFn_0046bf30 g_riSetCallbacks;
extern TimerFn_0046bf30 g_riIntervalTimer;
extern ReportFn_0046bf30 g_riReport;
extern ChatFn_0046bf30 g_riReportGameChat;

void ShutdownScoreTables();
void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size);
int AllocScoreTables();
void __stdcall ReportGameEventCallback(int event);
int __stdcall ShowGameMessage(char* text, int mode);
void __stdcall RISetCallbacks(int report, int notify);
void __cdecl GameFreeThunk(void* p);

// FUNCTION: 0x46bce0
int AllocScoreTables()
{
    char name[64];
    int failed;
    int i;

    g_onlineReportPlayers = (PlayerInfo_0046bce0**)GameAllocIgnoreTag("PlayersArray", 0x28);
    g_onlineReportScoreBoards = (ScoreBoard_0046bce0**)GameAllocIgnoreTag("ScoreBoardsArray", 0x28);
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
        g_onlineReportPlayers[i] = (PlayerInfo_0046bce0*)GameAllocIgnoreTag(name, 0x18);
        if (g_onlineReportPlayers[i] == 0) {
            failed = 1;
            break;
        }
        sprintf(name, "Allies%d", i);
        g_onlineReportPlayers[i]->allies = GameAllocIgnoreTag(name, 0x28);
        if (g_onlineReportPlayers[i]->allies == 0) {
            failed = 1;
            break;
        }
        sprintf(name, "ScoreBoard%d", i);
        g_onlineReportScoreBoards[i] = (ScoreBoard_0046bce0*)GameAllocIgnoreTag(name, 0xc);
        if (g_onlineReportScoreBoards[i] == 0) {
            failed = 1;
            break;
        }
        sprintf(name, "ppScores%d", i);
        g_onlineReportScoreBoards[i]->ppScores = GameAllocIgnoreTag(name, 0x24);
        if (g_onlineReportScoreBoards[i]->ppScores == 0) {
            failed = 1;
            break;
        }
        memset(g_onlineReportScoreBoards[i]->ppScores, 0, 0x24);
        sprintf(name, "Scores%d", i);
        g_onlineReportScores[i] = (char*)GameAllocIgnoreTag(name, 0x48);
        if (g_onlineReportScores[i] == 0) {
            failed = 1;
            break;
        }
        memset(g_onlineReportScores[i], 0, 0x48);
        int** slot = (int**)g_onlineReportScoreBoards[i]->ppScores;
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

// FUNCTION: 0x46bf00
int __stdcall EnableReporter(int enable)
{
    if (g_riEnable != 0) {
        return g_riEnable(enable);
    }
    return 1;
}

// FUNCTION: 0x46bf20
bool IsReporterDllLoaded()
{
    return g_reporterDll != 0;
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
            GameFreeThunk(g_onlineReportScoreBoards[i]->ppScores);
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
