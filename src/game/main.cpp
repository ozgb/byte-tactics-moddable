#include "win_types.h"
#include <excpt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <new>
#include "../state/game_rand.h"
#include "../../interfaces/host.h"

struct FileHandle;

#include "../files/files_api.h"

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
int __cdecl SetOutOfMemoryHandler(int param_1);

// FUNCTION: 0x49e6f0
void InstallOutOfMemoryHandler()
{
    SetOutOfMemoryHandler((int)OutOfMemoryHandler);
}

void __stdcall ReportViaException(char* text);
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

class Sound {
public:
    char pad[0x294];
    Sound();
    void SetTrackCategory(int param_1);
    void ReapFinishedBuffers();
    void CloseCdAudio();
    void OpenCdAudio();
    void SetCdCallback(void (*param_1)());
    int GetTrackCategory();
    void EnableCdAudio(int param_1);
    void SetPlaybackOrder(int param_1);
};

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

union Word_0051f522 {
    unsigned short value;
    struct {
        unsigned short b0 : 1;
        unsigned short b1 : 1;
        unsigned short b2 : 1;
        unsigned short b3 : 1;
        unsigned short b4 : 1;
        unsigned short b5 : 1;
        unsigned short b6 : 1;
        unsigned short b7 : 1;
        unsigned short b8 : 1;
        unsigned short b9 : 1;
        unsigned short spare : 6;
    } bits;
};
union Dword_0051f410 {
    int value;
    struct {
        unsigned b0 : 1;
        unsigned b1 : 1;
        unsigned b2 : 1;
        unsigned b3 : 1;
        unsigned b4 : 1;
        unsigned b5 : 1;
        unsigned b6 : 1;
        unsigned b7 : 1;
        unsigned b8 : 1;
        unsigned b9 : 1;
        unsigned b10 : 1;
        unsigned b11 : 1;
        unsigned spare : 20;
    } bits;
};

#pragma pack(push, 2)
struct App_0049e830 {
    int hInstance;
    int nCmdShow;
    int className;
    int title;
    int unknown_10;
    int menuId;
    char unknown_18[0xe0 - 0x18];
    int active;
    char unknown_e4[0xf0 - 0xe4];
    Dword_0051f410 flags;
    char unknown_f4[0x1fa - 0xf4];
    int startWidth;
    int startHeight;
    Word_0051f522 video;
};
#pragma pack(pop)
extern App_0049e830 g_displayContext;


extern const char DAT_005097f4[];
extern const char g_languageValueName[];
extern const char g_translationFile[];
extern const char g_audioCdShellKey[];
extern const char g_cdShellValueName[];
extern const char g_defaultLanguage[];

#include "../debug/debug_lib_4da120.h"
#include "../frontend/endgame.h"
#include "cd_check.h"
#include "../frontend/frontend.h"
#include "game_state.h"
void __stdcall InitDisplayDefaults(void* param_1);
int __stdcall InitEnvironment(void* param_1);
void __stdcall EmptyPostArchiveMountHook(const char* param_1);
void __stdcall ShutdownEnvironment(void* param_1);
#include "../ingame/screenshots.h"
#include "../gui/mouse.h"
#include "main_49eda0.h"
void __stdcall ReadGameRegistryValue(const char* param_1, void* param_2, int* param_3);
void __stdcall WriteGameRegistryValue(const char* param_1, void* param_2, int param_3);
void __stdcall LoadTranslations(const char* param_1, char* param_2);

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
    g_displayContext.video.bits.b0 = ~g_cmdlineDisplaySeed;
    g_displayContext.video.bits.b1 = 1;
    g_displayContext.video.bits.b8 = 1;
    g_displayContext.video.bits.b4 = 1;
    g_displayContext.video.bits.b9 = 1;
    g_displayContext.video.bits.b5 = 1;
    g_displayContext.video.bits.b6 = 1;
    g_displayContext.video.bits.b7 = 1;
    g_displayContext.startHeight = 0x1e0;
    g_displayContext.hInstance = (int)hInstance;
    g_displayContext.nCmdShow = nCmdShow;
    g_displayContext.className = (int)g_windowClassName;
    g_displayContext.title = (int)g_appName;
    g_displayContext.menuId = 0;
    int bGameOk = InitEnvironment(&g_displayContext);
    if (bGameOk == lzero)
        return 0;
    InitTimers(0x1e);
    RegisterDataArchives();
    EmptyPostArchiveMountHook(DAT_005097f4);
    g_game->unknown_c = (Ctx_00499890*)(&g_displayContext);
    g_game->version = 3;
    g_game->versionMinor = 1;
    g_game->field_3 = 1;
    if (strlen(g_preferredLanguage) == 0) {
        size1c = 0x40;
        ReadGameRegistryValue(g_languageValueName, g_preferredLanguage, (int*)&size1c);
        if (g_preferredLanguage[0] == 0)
            strcpy(g_preferredLanguage, g_defaultLanguage);
    }
    LoadTranslations(g_translationFile, g_preferredLanguage);
    g_game->field_10 = (Class_004cdb40*)(new Sound);
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
        ReadGameRegistryValue(g_cdShellValueName, buf74, (int*)&size14);
        if (strlen(buf74) != 0)
            strcpy(buf40, buf74);
        else
            WriteGameRegistryValue(g_cdShellValueName, buf40, 0x32);
    }

    for (;;) {
        if (!host_pump_events(0))
            break;
        if (!DispatchHostEvents())
            break;
        if (g_displayContext.active == lzero && *(int*)((Sound*)g_game->field_10) != 0) {
            SaveCdLists();
            g_cdTrackCategory = ((Sound*)g_game->field_10)->GetTrackCategory();
            ((Sound*)g_game->field_10)->CloseCdAudio();
            g_cdNeedsReopenAfterFocus = 1;
        } else if (g_displayContext.active != lzero && *(int*)((Sound*)g_game->field_10) == 0
                   && g_cdNeedsReopenAfterFocus != 0) {
            ((Sound*)g_game->field_10)->OpenCdAudio();
            ((Sound*)g_game->field_10)->SetCdCallback(ReopenCdAudio);
            ((Sound*)g_game->field_10)->EnableCdAudio(g_game->musicMode & 1);
            ((Sound*)g_game->field_10)->SetPlaybackOrder(g_game->cdMode);
            ((Sound*)g_game->field_10)->SetTrackCategory(g_cdTrackCategory);
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
                ((Sound*)g_game->field_10)->ReapFinishedBuffers();
                g_lastSoundReapTick = tick;
            }
        }
    }

    if (g_displayContext.flags.bits.b11) {
        ShutdownMouse();
        ShutdownGame();
    }
    if (host_setting_set(HOST_SCOPE_SYSTEM, g_audioCdShellKey, NULL, HOST_SETTING_STRING, buf40, 0)
        != HOST_SETTING_NO_ACCESS) {
        strcpy(buf40, DAT_005119b8);
        WriteGameRegistryValue(g_cdShellValueName, buf40, 0x32);
    }
    ShutdownEnvironment(&g_displayContext);
    return 0;
}

// FUNCTION: 0x49ed90
void AtexitNoOp(void)
{
}

int __stdcall ExceptionFilter(EXCEPTION_POINTERS* exception, const char* thread);

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

char* __stdcall StripFileName(char* path);

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
