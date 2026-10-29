#ifndef MOVIES_H
#define MOVIES_H

#include "win_types.h"

struct SmackHandleView;
struct SmackerSurface;

class MoviePlayer {
public:
    SmackHandleView* smack;
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

int __stdcall GetBuildSiteMetal(void);
int __stdcall GetBuildSiteHeight(void);

#endif
