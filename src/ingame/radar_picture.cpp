#include "radar_picture.h"

#include "../graphics/gaf_frame.h"

#pragma pack(push, 1)

struct MapTileSet {
    int count;
    unsigned char (*cell)[32][32];
};

struct WeaponDef;

struct InterceptorDefFlags {
    unsigned int :29;
    unsigned int bit29 : 1;
    unsigned int :2;
};






#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../graphics/gaf.h"
#include "../graphics/surface.h"
#include "../graphics/draw.h"

void __stdcall FrameFromSurface(GafFrame* dst, void* src);
void* __stdcall AllocSurface(char* name, int width, int height);


// FUNCTION: 0x466780
void __stdcall BuildRadarPicture()
{
    int mapWidth = g_game->mapWidth;
    int mapHeight = g_game->mapHeight;
    int width;
    int height;
    if (mapWidth >= mapHeight) {
        width = 126;
        height = mapHeight * 126 / mapWidth;
        g_game->posX = 0;
        g_game->posY = (126 - height) / 2;
    } else {
        width = mapWidth * 126 / mapHeight;
        height = 126;
        g_game->posX = (126 - width) / 2;
        g_game->posY = 0;
    }
    g_game->width_142eb = width;
    g_game->screen_h = height;
    g_game->pictureSurface = AllocSurface(g_radarPictureName, width, height);
    GafFrame frame;
    FrameFromSurface(&frame, g_game->pictureSurface);
    if ((GafFrame *)g_game->radarFrame) {
        DownsampleFrame((GafFrame *)g_game->radarFrame, &frame);
        return;
    }
    int h2 = height * 2;
    int w2 = width * 2;
    void* temp = AllocSurface(g_radarPicTempName, w2, h2);
    for (int j = 0; j < h2; j++) {
        for (int i = 0; i < w2; i++) {
            int x = g_game->mapWidth * i / w2;
            int y = g_game->mapHeight * j / h2;
            int index = (y / 32) * (g_game->width / 2) + x / 32;
            unsigned short value = g_game->mapValues[index];
            if (value >= g_game->iconSet->count) {
                value = 0;
            }
            int pixel = g_game->iconSet->cell[value][y % 32][x % 32];
            DrawPixel((Surface *)temp, i, j, pixel);
        }
    }
    GafFrame tempFrame;
    FrameFromSurface(&tempFrame, temp);
    DownsampleFrame(&tempFrame, &frame);
    FreeSurface((Surface *)temp);
}

// FUNCTION: 0x4669b0
void InitRadar()
{
    BuildRadarPicture();
    g_game->finalSurface = AllocSurface(g_radarFinalName, g_game->dim.v[2], g_game->dim.v[3]);
    g_game->mappedSurface = AllocSurface(g_radarMappedName, g_game->dim.v[2], g_game->dim.v[3]);
    g_game->viewLeft = g_game->dim.v[0];
    g_game->viewTop = g_game->dim.v[1];
    g_game->viewRight = g_game->dim.v[2] + g_game->dim.v[0] - 1;
    {
        char* c = (char*)g_game;
        g_game->viewBottom = *(short*)(c + 0x142ed) + *(short*)(c + 0x142e9) - 1;
    }
    g_game->timer.word.flags.mapChanged = 1;
    g_game->timer.word.blinkTimer = 7;
    g_game->timer.word.flags.blinkOn = 0;
}
