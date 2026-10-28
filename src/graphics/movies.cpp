#include <stdio.h>
#include <windows.h>
#include "../../interfaces/host.h"
#include "../util/file_printf.h"

int __stdcall HandleHostEvent(host_event* ev);

struct Smk_0047c3a0 {
    unsigned int version;
    unsigned int width;
    unsigned int height;
    unsigned int frames;
    char unknown_10[0x68 - 0x10];
    unsigned int newPalette;
    unsigned char rgb[256][3];
    char unknown_36c[0x370 - 0x36c];
    unsigned short palType;
    char unknown_372[0x374 - 0x372];
    int frameNum;
    char unknown_378[0x380 - 0x378];
    int lastLeft;
    int lastTop;
    int lastWidth;
    int lastHeight;
};

extern "C" __declspec(dllimport) int __stdcall SmackSoundEnable(HWND hwnd);
extern "C" __declspec(dllimport) int __stdcall SmackSoundUseDirectSound(void* sound);
extern "C" __declspec(dllimport) Smk_0047c3a0* __stdcall SmackOpen(char* path, unsigned int flags, int extra);
extern "C" __declspec(dllimport) unsigned int __stdcall SmackSoundOnOff(Smk_0047c3a0* smk, unsigned int on);
extern "C" __declspec(dllimport) void __stdcall SmackGoto(Smk_0047c3a0* smk, unsigned int frame);
extern "C" __declspec(dllimport) void __stdcall SmackDoFrame(Smk_0047c3a0* smk);
extern "C" __declspec(dllimport) void __stdcall SmackNextFrame(Smk_0047c3a0* smk);
extern "C" __declspec(dllimport) void __stdcall SmackToBuffer(Smk_0047c3a0* smk, unsigned int left, unsigned int top, unsigned int width, unsigned int height, unsigned int bufferHeight, void* buffer);
extern "C" __declspec(dllimport) int __stdcall SmackToBufferRect(Smk_0047c3a0* smk, int flag);
extern "C" __declspec(dllimport) unsigned int __stdcall SmackWait(Smk_0047c3a0* smack);

extern "C" __declspec(dllimport) void __stdcall SmackClose(void* smack);

struct SmkStats_0047c530 {
    unsigned int totalTime;
    unsigned int unknown_04;
    unsigned int openTime;
    unsigned int frames;
    unsigned int framesSkipped;
    unsigned int unknown_14;
    unsigned int timeBlit;
    unsigned int readTime;
    unsigned int decompTime;
    unsigned int unknown_24;
    unsigned int readSpeed;
    unsigned int slowestFrameTime;
    unsigned int slowest2FrameTime;
    unsigned int unknown_34;
    unsigned int unknown_38;
    unsigned int averageFrameSize;
    unsigned int highest1SecRate;
    unsigned int unknown_44;
    unsigned int highestMemAmount;
    unsigned int totalExtraMemory;
    unsigned int highestExtraUsed;
};

extern "C" __declspec(dllimport) void __stdcall SmackSummary(Smk_0047c3a0* smk, SmkStats_0047c530* stats);

struct Surfaces_0047bdf0 {
    void* ddraw;
    void* primary;
    void* back;
    void* clipper;
    void* palette;
};
static_assert(sizeof(Surfaces_0047bdf0) == 0x14, "Surfaces_0047bdf0 size");

struct Display_0047bdf0 {
    char unknown_0[0x24];
    void* sound;
    char unknown_28[0x40 - 0x28];
    HWND hwnd;
    char unknown_44[0x84 - 0x44];
    Surfaces_0047bdf0 surfaces;
    char unknown_98[0xf0 - 0x98];
    unsigned short flags;
};

struct Display_0047c3a0 {
    char unknown_0[8];
    unsigned int width;
    unsigned int height;
    void* palette;
};

#pragma pack(push, 1)
#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

#include "../../link/globals.h"

Display_0047bdf0* GetDisplay();
#include "display.h"
void __stdcall SetOffscreenSurface(Display_0047c3a0* display);
void FlipScreen();

struct Smk_0047bf70_b {
    char unknown_0[0x2c];
    DWORD paletteColors;
    char unknown_30[0x3c - 0x30];
    DWORD palette;
    char unknown_40[0x43c - 0x40];
    DWORD paletteType;
};

extern "C" __declspec(dllimport) Smk_0047bf70_b *__stdcall SmackBufferOpen(HWND hwnd, HDC hdc, int width, int height, int flags, int background);
extern "C" __declspec(dllimport) void __stdcall SmackBufferNewPalette(Smk_0047bf70_b *smk, unsigned char *rgb, unsigned short flag);
extern "C" __declspec(dllimport) void __stdcall SmackColorRemap(Smk_0047c3a0 *smk, DWORD *palette, DWORD paletteColors, DWORD paletteType);

struct SmackerSurface;

class MoviePlayer {
public:
    Smk_0047c3a0* smack;
    unsigned int frame;
    int stopped;
    HWND hwnd;
    PALETTEENTRY entries[256];
    int paletteResult;
    int hasSurfaces;
    char unknown_418[0x5b4 - 0x418];
    char pad_5b4[4];

    MoviePlayer(char* path, int a, int b, int c, int d, int e);
    void UpdatePalette(void);
    void ClearScreen();
    void PlayFrame(HWND hwnd);
    void WriteSmackStats();
    void Play();
    void Close();
    int SetupDirectDraw();
    int GetBlitMode(SmackerSurface* unused);
};

static_assert(sizeof(MoviePlayer) == 0x5b8, "MoviePlayer size");

// FUNCTION: 0x47bdf0
MoviePlayer::MoviePlayer(char* path, int a, int b, int c, int d, int e)
{
    stopped = 0;
    frame = 0;
    if (a)
        SmackSoundEnable((HWND)b);
    void* sound = host_sound_native();
    SmackSoundUseDirectSound(sound);
    smack = SmackOpen(path, 0xfe100, -1);
    if (!smack)
        FatalError("Could not open movie file, please check filename in INI.");
    SmackSoundOnOff(smack, sound != 0);

    Display_0047bdf0* disp = GetDisplay();
    hwnd = disp->hwnd;
    if ((disp->flags & 2) && host_display_is_open()) {
        hasSurfaces = 0;
    } else if (!(disp->flags & 2) && !host_display_is_open()) {
        if (!SetupDirectDraw())
            FatalError("Could not setup Direct Draw to play movie.");
        hasSurfaces = 1;
    }

    host_frame_fill(HOST_FRAME_FRONT, 0);
}

// FUNCTION: 0x47bf20
void MoviePlayer::Close()
{
    SmackClose(smack);
    if (hasSurfaces)
        host_display_close();
}

// FUNCTION: 0x47bf70
int MoviePlayer::SetupDirectDraw()
{
    POINT point;
    int i;

    point.x = 0;
    point.y = 0;
    host_window_client_origin((int*)&point.x, (int*)&point.y);
    int shown = host_display_mode_set(0, 0, HOST_DISPLAY_PRIMARY_ONLY, 0);
    if (shown == HOST_DISPLAY_FAILED)
        goto fail;
    if (shown == HOST_DISPLAY_OK) {
        paletteResult = GetBlitMode(0);
        if (paletteResult == 0) {
            host_palette_system((unsigned char*)entries);
            for (i = 0; i < 10; ++i) entries[i].peFlags = 0;
            for (i = 10; i < 246; ++i) entries[i].peFlags = 4;
            for (i = 246; i < 256; ++i) entries[i].peFlags = 0;
            host_palette_create((unsigned char*)entries);
        }
        host_window_resize(smack->width, smack->height);
        host_window_raise();
    }
    if (smack->newPalette) {
        Smk_0047bf70_b *smk = SmackBufferOpen(hwnd, 0, 0x280, 0x1e0, 0, 0);
        SmackBufferNewPalette(smk, &smack->rgb[0][0], smack->palType);
        SmackColorRemap(smack, &smk->palette, smk->paletteColors, smk->paletteType);
    }
    return 1;
fail:
    return 0;
}

// FUNCTION: 0x47c150
int MoviePlayer::GetBlitMode(SmackerSurface* unused)
{
    host_pixel_format format;
    host_frame_pixel_format(&format);
    if (format.bits == 8)
        return 0;
    if (format.red == 0xf800 && format.green == 0x7e0 && format.blue == 0x1f)
        return 0xc0000000;
    if (format.red == 0x7c00 && format.green == 0x3e0 && format.blue == 0x1f)
        return 0x80000000;
    if (format.red == 0xf800 && format.green == 0x7c0 && format.blue == 0x3f)
        return 0xa0000000;
    if (format.red == 0xfc00 && format.green == 0x3e0 && format.blue == 0x1f)
        return 0xe0000000;
    host_message(NULL, "Smacker Error", "Unsupported pixel format.", HOST_MESSAGE_PLAIN);
    return 0;
}

// FUNCTION: 0x47c2a0
void MoviePlayer::UpdatePalette(void)
{
    unsigned char* src = smack->rgb[0];
    for (int i = 0; i < 256; i++) {
        entries[i].peRed = *src++;
        entries[i].peGreen = *src++;
        entries[i].peBlue = *src++;
    }
    host_palette_set(0, 256, (unsigned char*)entries);
}

// FUNCTION: 0x47c2f0
void MoviePlayer::ClearScreen()
{
    host_frame_fill(HOST_FRAME_FRONT, 0);
}

// FUNCTION: 0x47c3a0
void MoviePlayer::PlayFrame(HWND hwnd)
{
    if (!host_window_has_focus())
        return;
    if (stopped)
        return;
    if (smack->newPalette) {
        unsigned char* src = smack->rgb[0];
        for (int i = 0; i < 256; i++) {
            entries[i].peRed = *src++;
            entries[i].peGreen = *src++;
            entries[i].peBlue = *src++;
        }
        host_palette_set(0, 256, (unsigned char*)entries);
    }
    SetOffscreenSurface((Display_0047c3a0 *)g_game->screen);
    SmackToBuffer(smack, 0, (0x1e0 - smack->height) >> 1, ((Display_0047c3a0 *)g_game->screen)->width, smack->height, ((Display_0047c3a0 *)g_game->screen)->height, 0);
    SmackDoFrame(smack);
    FlipScreen();
    if (SmackToBufferRect(smack, 1) && smack->lastLeft == 0 && smack->lastTop == 0
        && smack->lastWidth == smack->width && smack->lastHeight == smack->height) {
        frame = smack->frameNum + 1;
    }
    if (smack->frameNum == smack->frames - 1) {
        stopped = 1;
        return;
    }
    SmackNextFrame(smack);
}

// FUNCTION: 0x47c530
void MoviePlayer::WriteSmackStats()
{
    SmkStats_0047c530 stats;
    SmackSummary(smack, &stats);
    void* file = host_file_create("stats.txt", HOST_FILE_TEXT);
    if (file) {
        FilePrintf(file, "Frames Per Sec\t%d\n", 1000 * stats.frames / stats.totalTime);
        FilePrintf(file, "Total Playback Time\t%d\n", 1000 * stats.totalTime / stats.totalTime);
        FilePrintf(file, "Time to Open File\t%d\n", stats.openTime);
        FilePrintf(file, "Total Frames Played\t%d\n", stats.frames);
        FilePrintf(file, "SkippedFrames\t%d\n", stats.framesSkipped);
        FilePrintf(file, "TotalBlitTime\t%d\n", stats.timeBlit);
        FilePrintf(file, "TotalReadTime\t%d\n", stats.readTime);
        FilePrintf(file, "TotalDecompTime\t%d\n", stats.decompTime);
        FilePrintf(file, "TotalReadSpeed\t%d bytes/sec\n", stats.readSpeed);
        FilePrintf(file, "SlowestFrameTime\t%d\n", stats.slowestFrameTime);
        FilePrintf(file, "Slowest2FrameTime\t%d\n", stats.slowest2FrameTime);
        FilePrintf(file, "AverageFrameSize\t%d\n", stats.averageFrameSize);
        FilePrintf(file, "Highest1SecRate\t%d\n", stats.highest1SecRate);
        FilePrintf(file, "HighestMemAmount\t%d\n", stats.highestMemAmount);
        FilePrintf(file, "TotalExtraMemory\t%d\n", stats.totalExtraMemory);
        FilePrintf(file, "HighestExtraUsed\t%d\n", stats.highestExtraUsed);
        host_file_close(file);
    }
}

// FUNCTION: 0x47c6c0
void MoviePlayer::Play()
{
    host_event ev;
    while (!stopped) {
        if (!host_pump_events(0))
            host_post_quit();
        if (host_event_next(&ev)) {
            do {
                if (ev.type == HOST_EVENT_SYS_KEY_DOWN && ev.a == 0x73) {
                    stopped = 1;
                    g_game->introMovieLoop = 0;
                    host_post_quit();
                    return;
                }
                if (ev.type == HOST_EVENT_CHAR) {
                    stopped = 1;
                    g_game->introMovieLoop = 0;
                }
                HandleHostEvent(&ev);
            } while (host_event_next(&ev));
        } else if (!SmackWait(smack)) {
            PlayFrame(hwnd);
        }
    }
}

// FUNCTION: 0x47c770
int GetBuildSiteMetal(void)
{
    return g_lastPlaceMetalSum;
}

// FUNCTION: 0x47c780
int GetBuildSiteHeight(void)
{
    return g_lastPlaceHeight;
}
