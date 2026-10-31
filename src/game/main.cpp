#include "win_types.h"
#include <excpt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <new>
#include <ctype.h>
#include <malloc.h>
#include "../quirks/quirks.h"
#include "../network/online.h"
#include "../ingame/camera.h"
#include "../frontend/options.h"
#include "../network/net_game.h"
#include "../state/game_rand.h"
#include "../../interfaces/host.h"

struct FileHandle;

#include "../files/files_api.h"

#include "../sound/sound.h"

// FUNCTION: 0x49e640
void __stdcall AppendToDebugFile(char* text)
{
    FileHandle* file = HAPI_OpenFileAppend("DEBUG.FIL");
    if (!file)
        file = HAPI_CreateFile("DEBUG.FIL");
    HAPI_WriteFile(file, text, strlen(text) + 1);
    HAPI_CloseFile(file);
}

#include "main.h"
#include "../debug/debug_lib.h"

// FUNCTION: 0x49e680
void __stdcall ReportViaException(const char* message)
{
    int zero = 0;
    __try {
        __asm {
            push eax
            push edx
            div zero
            pop edx
            pop eax
        }
    } __except (host_crash_report(GetExceptionInformation(), message), EXCEPTION_EXECUTE_HANDLER) {
    }
}

// FUNCTION: 0x49e6f0
void InstallOutOfMemoryHandler()
{
    SetOutOfMemoryHandler((int)OutOfMemoryHandler);
}

#include "../debug/debug_lib.h"

// FUNCTION: 0x49e700
void OutOfMemoryHandler()
{
    void* file = host_file_open_append("ErrorLog.txt");
    if (file) {
        host_file_write(file, "Out of memory!\r\nYour hard disk may be full\r\n",
                        strlen("Out of memory!\r\nYour hard disk may be full\r\n"));
        host_file_close(file);
    }
    ReportViaException("Out of memory handler");
    host_message(NULL, "Total Annihilation", "Out of memory!\r\nYour hard disk may be full\r\n",
                 HOST_MESSAGE_ERROR | HOST_MESSAGE_SYSTEM_MODAL | HOST_MESSAGE_TOPMOST);
    NopAbortHook();
    raise(SIGABRT);
    _exit(3);
}

#include "../sound/sound.h"

int RIReport(int, int, int, int, int, int, int, int, int, int);
void ScaleUnitWeights(int, unsigned int*, float, int);
void __stdcall DrawFrame(void* surface, void* header, int x, int y);

#pragma pack(push, 1)
#include "game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

extern Game* g_game;

#include "../graphics/display.h"

extern unsigned char g_atexitRegistered;
extern char* g_appName;
extern char* g_windowClassName;
extern int g_cmdlineDisplaySeed;
extern char g_preferredLanguage[];
extern int g_cdTrackCategory;
extern int g_cdNeedsReopenAfterFocus;
extern DWORD g_lastSoundReapTick;

extern Display g_displayContext;


extern const char DAT_005097f4[];
extern const char g_languageValueName[];
extern const char g_translationFile[];
extern const char g_audioCdShellKey[];
extern const char g_cdShellValueName[];
extern const char g_defaultLanguage[];

#include "../frontend/endgame.h"
#include "cd_check.h"
#include "../frontend/frontend.h"
#include "game_state.h"
void __stdcall EmptyPostArchiveMountHook(const char* param_1);
#include "../ingame/screenshots.h"
#include "../gui/mouse.h"
#include "settings.h"
#include "../util/tdf.h"

#include "../harness/harness.h"

// FUNCTION: 0x49e830
int __stdcall GameMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                           LPSTR lpCmdLine, int nCmdShow)
{
    DWORD size14;
    DWORD size1c;
    DWORD size20;
    char buf40[0x34];
    char buf74[0x32];
    int lzero = 0;

    HarnessInit(lpCmdLine);
    (void)hPrevInstance;

    InitDebugSupport(8);
    SetOutOfMemoryHandler((int)OutOfMemoryHandler);
    if ((g_atexitRegistered & 1) == 0) {
        g_atexitRegistered |= 1;
        atexit((void (__cdecl *)(void))AtexitNoOp);
    }
    CreateGameObject();
    if (host_instance_running(g_appName))
        return -1;
    GameSrand(RAND_STREAM_MAIN, HarnessCrtSeed(time(0)));
    if (ParseCommandLine(lpCmdLine, g_appName) == 0)
        return 1;
    InitDisplayDefaults(&g_displayContext);
    g_displayContext.startWidth = 0x280;
    g_displayContext.videoFlags.bits.b0 = ~g_cmdlineDisplaySeed;
    g_displayContext.videoFlags.bits.b1 = 1;
    g_displayContext.videoFlags.bits.b8 = 1;
    g_displayContext.videoFlags.bits.b4 = 1;
    g_displayContext.videoFlags.bits.b9 = 1;
    g_displayContext.videoFlags.bits.b5 = 1;
    g_displayContext.videoFlags.bits.b6 = 1;
    g_displayContext.videoFlags.bits.b7 = 1;
    g_displayContext.startHeight = 0x1e0;
    g_displayContext.hInstance = hInstance;
    g_displayContext.nCmdShow = nCmdShow;
    g_displayContext.className = g_windowClassName;
    g_displayContext.title = g_appName;
    g_displayContext.menuId = 0;
    int bGameOk = InitEnvironment(&g_displayContext);
    if (bGameOk == lzero)
        return 0;
    InitTimers(0x1e);
    RegisterDataArchives();
    EmptyPostArchiveMountHook(DAT_005097f4);
    g_game->displayContext = &g_displayContext;
    g_game->version = 3;
    g_game->versionMinor = 1;
    g_game->field_3 = 1;
    if (strlen(g_preferredLanguage) == 0) {
        size1c = 0x40;
        ReadGameRegistryValue(g_languageValueName, g_preferredLanguage, (unsigned int *)((int*)&size1c));
        if (g_preferredLanguage[0] == 0)
            strcpy(g_preferredLanguage, g_defaultLanguage);
    }
    LoadTranslations((char *)g_translationFile, g_preferredLanguage);
    g_game->sound = (SoundView*)(new Sound);
    EmptyPreFrontendInitHook();
    InitGame();

    size20 = 0x32;
    size14 = 0x32;
    int shellRead = host_setting_get(HOST_SCOPE_SYSTEM, g_audioCdShellKey, NULL, HOST_SETTING_STRING, buf40, (unsigned int*)&size20);
    if (shellRead != HOST_SETTING_NO_ACCESS) {
        if (shellRead == 1) {
            host_setting_set(HOST_SCOPE_SYSTEM, g_audioCdShellKey, NULL, HOST_SETTING_STRING, DAT_005119b8, 1);
        } else {
            strcpy(buf40, DAT_005119b8);
        }
        size14 = 0x32;
        ReadGameRegistryValue(g_cdShellValueName, buf74, (unsigned int *)((int*)&size14));
        if (strlen(buf74) != 0)
            strcpy(buf40, buf74);
        else
            WriteGameRegistryValue((void *)g_cdShellValueName, buf40, 0x32);
    }

    for (;;) {
        if (!host_pump_events(0))
            break;
        if (!DispatchHostEvents())
            break;
        if (g_displayContext.active == lzero && ((Sound*)g_game->sound)->open != 0) {
            SaveCdLists();
            g_cdTrackCategory = ((Sound*)g_game->sound)->GetTrackCategory();
            ((Sound*)g_game->sound)->CloseCdAudio();
            g_cdNeedsReopenAfterFocus = 1;
        } else if (g_displayContext.active != lzero && ((Sound*)g_game->sound)->open == 0
                   && g_cdNeedsReopenAfterFocus != 0) {
            ((Sound*)g_game->sound)->OpenCdAudio();
            ((Sound*)g_game->sound)->SetCdCallback(ReopenCdAudio);
            ((Sound*)g_game->sound)->EnableCdAudio(g_game->musicMode & 1);
            ((Sound*)g_game->sound)->SetPlaybackOrder(g_game->cdMode);
            ((Sound*)g_game->sound)->SetTrackCategory(g_cdTrackCategory);
            ReopenCdAudio();
            g_cdNeedsReopenAfterFocus = 0;
        }
        if (g_displayContext.active == 0 && (g_game->flags_2a44 & 1) == 0) {
            if (!host_pump_events(1))
                break;
            if (!DispatchHostEvents())
                break;
            continue;
        }
        MainFrameTick();
        {
            DWORD tick = host_time_ms();
            if ((int)(tick - g_lastSoundReapTick) >= 100) {
                ((Sound*)g_game->sound)->ReapFinishedBuffers();
                g_lastSoundReapTick = tick;
            }
        }
    }

    if (g_displayContext.flags.bits.quitting) {
        ShutdownMouse();
        ShutdownGame();
    }
    if (host_setting_set(HOST_SCOPE_SYSTEM, g_audioCdShellKey, NULL, HOST_SETTING_STRING, buf40, 0)
        != HOST_SETTING_NO_ACCESS) {
        strcpy(buf40, DAT_005119b8);
        WriteGameRegistryValue((void *)g_cdShellValueName, buf40, 0x32);
    }
    ShutdownEnvironment(&g_displayContext);
    return 0;
}

// FUNCTION: 0x49ed90
void AtexitNoOp(void)
{
}

#include "../graphics/pcx.h"

// FUNCTION: 0x49eda0
int __stdcall GameEntry(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine, int nCmdShow)
{
    __try {
        return GameMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    } __except (ExceptionFilter(GetExceptionInformation(), "Main Thread")) {
    }
    return 0;
}

// FUNCTION: 0x49ee30
int __stdcall ParseCommandLine(char* cmdLine, char* appName)
{
    char* copy = (char*)_alloca(strlen(cmdLine) + 1);
    strcpy(copy, cmdLine);
    g_game->lobbyTimeoutSecs = 30;
    g_game->netSendDropPct = 0;
    g_game->cmdlineSkipIntroMovie = 0;

    char* p = strtok(copy, " \t");
    while (p) {
        if (*p != '-' && *p != '/') {
            if (g_quirks.boundLanguageCopy) {
                strncpy(g_preferredLanguage, p, 0x3f);
                g_preferredLanguage[0x3f] = 0;
            } else {
                strcpy(g_preferredLanguage, p);
            }
        } else if (!IsDebugKeyword(p)) {
            switch (p[1]) {
            case 'B':
            case 'b':
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    if (!_strcmpi(p, "lock"))
                        *(unsigned short*)&g_game->lockFlags |= 1;
                    else if (!_strcmpi(p, "deathends"))
                        ;
                    else if (!_strcmpi(p, "deathplays"))
                        ;
                    else if (!_strcmpi(p, "deathmatch"))
                        ;
                    else if (!_strcmpi(p, "fixedloc"))
                        ;
                    else if (!_strcmpi(p, "mapping"))
                        ;
                    else if (!_strcmpi(p, "circlos"))
                        ;
                    else if (!_strcmpi(p, "truelos"))
                        ;
                    else if (!_strcmpi(p, "permlos"))
                        ;
                    else if (!_strcmpi(p, "cheating"))
                        ;
                    else if (!_strcmpi(p, "watching"))
                        ;
                }
                break;
            case 'C':
            case 'c':
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    try {
                        Onl_LoadConfigFile(p);
                    } catch (...) {
                    }
                }
                g_game->cmdlineSkipIntroMovie = 1;
                break;
            case 'D':
            case 'd':
            {
                char mode = p[2];
                g_cmdlineDisplaySeed = 2;
                if (mode != 'f' && mode != 'F')
                    g_cmdlineDisplaySeed = 3;
                break;
            }
            case 'E':
            case 'e': {
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    int v = -1;
                    if (*p != '-')
                        v = atoi(p);
                    if (v < 0 || v > 100)
                        v = 0;
                    g_game->netSendDropPct = v;
                }
                break;
            }
            case 'F':
            case 'f':
                SetBypassDriveScan(1);
                break;
            case 'H':
            case 'h':
                p += 2;
                if (*p) {
                    SetHostGameName(1, p);
                } else {
                    char* q = strtok(NULL, " \t");
                    if (q && *q && *q != '-')
                        SetHostGameName(1, q);
                }
                break;
            case 'L':
            case 'l':
                SetCommandLineUnusedFlagL(0);
                break;
            case 'N':
            case 'n':
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p && *p != '-') {
                    char* colon = strchr(p, ':');
                    int n = atoi(p);
                    if (n == 1 && colon)
                        SetDirectConnectAddress(colon + 1);
                    SetDirectPlayProvider(n);
                }
                g_game->cmdlineSkipIntroMovie = 1;
                break;
            case 'P':
            case 'p':
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    int v = -1;
                    if (*p != '-')
                        v = atoi(p);
                    SetPacketRate(v);
                }
                break;
            case 'R':
            case 'r': {
                char* args = cmdLine + (p - copy) + 2;
                while (isspace(*args))
                    args++;
                char moduleName[_MAX_PATH];
                unsigned int len = host_file_exe_path(moduleName, _MAX_PATH);
                int registered = 0;
                if (len > 0) {
                    char drive[_MAX_DRIVE];
                    char ext[_MAX_EXT];
                    char folder[_MAX_DIR + 2];
                    char fname[_MAX_FNAME + _MAX_EXT];
                    char* lastChar;
                    fname[0] = 0;
                    ext[0] = 0;
                    _splitpath(moduleName, drive, folder + 2, fname, ext);
                    strcat(fname, ext);
                    folder[0] = drive[0];
                    folder[1] = drive[1];
                    lastChar = &folder[strlen(folder) - 1];
                    if (strchr("/\\", *lastChar))
                        *lastChar = 0;
                    registered = host_net_register_application(appName, fname, args, folder);
                }
                if (!registered) {
                    host_alert();
                    p = Translate("DirectPlay registration failed.");
                    host_message(NULL, appName, p, HOST_MESSAGE_ERROR);
                }
                return 0;
            }
            case 'S':
            case 's':
                SetNoDirectSound();
                break;
            case 'T':
            case 't':
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    int v = -1;
                    if (*p != '-')
                        v = atoi(p);
                    if (v < 30 || v > 300)
                        v = 30;
                    g_game->lobbyTimeoutSecs = v;
                }
                break;
            case 'W':
            case 'w':
                SetUseWindowsSound();
                break;
            }
        }
        p = strtok(NULL, " \t");
    }
    return 1;
}

#include "../graphics/palette.h"

// FUNCTION: 0x49f540
void ChdirToExeDirectory()
{
    char path[256];

    host_file_exe_path(path, 256);
    StripFileName(path);
    host_dir_set_current(path);
}

// FUNCTION: 0x49f580
int GetPreferredLanguage(void)
{
    return 0 < strlen(g_preferredLanguage) ? (int)(const void*)g_preferredLanguage : 0;
}

// FUNCTION: 0x49f5a0
UINT __stdcall GetPreferenceInt(char* key, int defaultValue)
{
    char exePath[256];
    char iniPath[256];

    host_file_exe_path(exePath, 256);
    StripFileName(exePath);
    sprintf(iniPath, "%s\\totala.ini", exePath);
    return host_file_config_int(iniPath, "Preferences", key, defaultValue);
}
