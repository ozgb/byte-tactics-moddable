#include "win_types.h"
#include <stdio.h>
#include <math.h>
#include "radar.h"
#include "../state/sim_radar.h"

#include "../util/point.h"

struct RadarQuad {
    Point p[4];
};

struct RadarSurfaceView {
    int width;
    int height;
    int pitch;
    int bits;
    int zPriority;
    int colorKey;
    unsigned short x;
    unsigned short y;
    char unknown_1c[0x10];
    unsigned int flag0 : 1;
    unsigned int flag1 : 1;
};

#include "../graphics/gaf_frame.h"

#pragma pack(push, 1)

#include "../map/radar_types.h"

#include "../network/player_info.h"

#include "../network/player.h"

#include "../weapons/weapon_def.h"

#include "../util/angles.h"
#include "../weapons/unit_weapon_slot.h"

#include "../units/unit.h"
#include "../weapons/projectile.h"

#include "../game/game.h"
#include "../ingame/hot_radar_slot.h"
#include "../graphics/display.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../graphics/gaf.h"
void __stdcall DrawFrame(void* surface, void* frame, int x, int y);
#include "../graphics/surface.h"
void __stdcall DrawFrameQuad(RadarSurfaceView* surface, void* pic, RadarQuad* dst, RadarQuad* src);
#include "../graphics/object_pool.h"

#include "../graphics/gaf.h"
#include "../graphics/draw.h"

void __stdcall DrawRectangle(void* param_1, void* param_2, int param_3);
void __stdcall DrawSurface(void* dst, void* bmp, int x, int y);

void* __stdcall GetGafFrame(void* a, int index);

// FUNCTION: 0x4665d0
void __stdcall ResizeRadarPicture(GafFrame* pic, int x, int y, int w, int h)
{
    if (pic == 0) {
        return;
    }
    int dwx;
    int dhy;
    int sw;
    int sh;
    RadarQuad src;
    RadarQuad dst;
    RadarSurfaceView surface;
    int dw = w - 0x20;
    int dh = h - 0x80;
    int ox;
    int oy;
    if (dw >= dh) {
        dwx = x;
        dhy = dh * y / dw;
        sw = pic->width;
        sh = dh * pic->height / dw;
        ox = 0;
        oy = (y - dhy) / 2;
    } else {
        dwx = dw * x / dh;
        dhy = y;
        sw = dw * pic->width / dh;
        sh = pic->height;
        ox = (x - dwx) / 2;
        oy = 0;
    }
    void* temp = AllocFrame("TEMP RADAR PIC", pic->width, pic->height);
    SurfaceFromFrame((Surface *)&surface, (GafFrame *)temp);
    DrawFrame(&surface, (short*)pic, 0, 0);
    pic->width = x;
    pic->height = y;
    SurfaceFromFrame((Surface *)&surface, pic);
    FillSurface((Surface *)&surface, 0);

    src.p[0].x = 0;
    src.p[0].y = 0;
    src.p[1].x = sw - 1;
    src.p[1].y = 0;
    src.p[2].x = sw - 1;
    src.p[2].y = sh - 1;
    src.p[3].x = 0;
    src.p[3].y = sh - 1;

    dst.p[0].x = ox;
    dst.p[0].y = oy;
    dst.p[1].x = ox + dwx;
    dst.p[1].y = oy;
    dst.p[2].x = ox + dwx;
    dst.p[2].y = oy + dhy;
    dst.p[3].x = ox;
    dst.p[3].y = oy + dhy;

    DrawFrameQuad(&surface, temp, &dst, &src);
    GameFreeThunk(temp);
}

// FUNCTION: 0x466aa0
void FreeRadar()
{
    FreeSurface((Surface *)g_game->pictureSurface);
    FreeSurface((Surface *)g_game->mappedSurface);
    FreeSurface((Surface *)g_game->finalSurface);
    g_game->pictureSurface = 0;
    g_game->mappedSurface = 0;
    g_game->finalSurface = 0;
}

// FUNCTION: 0x466b00
void __stdcall DrawRadar(void* param_1)
{
    if (g_game->timer.word.flags.pending) {
        g_game->timer.word.flags.pending = 0;
        DrawSurface(param_1, g_game->finalSurface, g_game->posX, g_game->posY);
        DrawRectangle(param_1, g_game->radarViewportRect, g_game->colors[14]);
    }
}

// FUNCTION: 0x466b70
void __stdcall CalcRadarViewportRect(int* param_1)
{
    param_1[0] = g_game->width_142eb * g_game->scrollX / g_game->mapWidth + g_game->posX;
    param_1[1] = g_game->screen_h * g_game->scrollY / g_game->mapHeight + g_game->posY;
    param_1[2] = g_game->width_142eb * g_game->screenTilesX * 16 / g_game->mapWidth + param_1[0] - 1;
    param_1[3] = g_game->screen_h * g_game->screenTilesY * 16 / g_game->mapHeight + param_1[1] - 1;
}

// FUNCTION: 0x466c20
void UpdateRadarMapped(const RadarMappedCapture* capture)
{
    if (g_game->mappedSurface == nullptr || g_game->pictureSurface == nullptr)
        return;
    unsigned char fog = g_game->textColor;
    const unsigned char* cell = (const unsigned char*)(capture + 1);
    unsigned char* dst = *(unsigned char**)((char*)g_game->mappedSurface + 0xc);
    unsigned char* src = *(unsigned char**)((char*)g_game->pictureSurface + 0xc);
    int halfWidth = capture->width / 2;
    int halfHeight = capture->height / 2;
    for (int i = 0; i < g_game->screen_h; i++) {
        int mapY = i * halfHeight;
        int mapX = 0;
        int j = 0;
        while (j < g_game->width_142eb) {
            int index = (mapY / g_game->screen_h) * halfWidth + mapX / g_game->width_142eb;
            if (cell[index] == RADAR_CELL_HIDDEN) {
                *dst = fog;
            } else if (cell[index] == RADAR_CELL_EXPLORED) {
                *dst = *src;
            } else {
                *dst = g_game->displayContext->grayTable[*src];
            }
            j++;
            mapX += halfWidth;
            src++;
            dst++;
        }
    }
    g_game->timer.word.flags.pending = 1;
}

// FUNCTION: 0x466dc0
void DrawRadarUnits(const RadarDrawCapture* capture)
{
    if (g_game->finalSurface == nullptr || g_game->mappedSurface == nullptr)
        return;
    unsigned char* base = (unsigned char*)g_game + 0xdcb;
    HotRadarSlot* out = g_game->hotRadar;

    g_game->hotRadarCount = 0;
    void* surface = g_game->finalSurface;
    DrawSurface(surface, g_game->mappedSurface, 0, 0);

    const RadarUnitCapture* u = (const RadarUnitCapture*)(capture + 1);
    for (unsigned int n = 0; n < capture->unitCount; n++, u++) {
        int x = u->xWhole * g_game->width_142eb / g_game->mapWidth;
        int y = u->yScaled * (int)g_game->screen_h / g_game->mapHeight;
        if (!(u->flags & RADAR_UNIT_DAMAGED) ||
            (g_game->timer.byte.field_142f0.b.hi & 1) != 0) {
            DrawFrame(surface, GetGafFrame(g_game->radlogo, u->color), x, y);
        }
        if ((short)u->id == (short)g_game->hoverUnitId) {
            DrawFrame(surface, GetGafFrame(g_game->radlogohigh, 0), x, y);
        }
        if (g_game->units[u->slot].selected) {
            if (u->flags & RADAR_UNIT_SHOWS_RANGES) {
                if (u->radarDistance != 0)
                    DrawCircle((Surface *)surface, x, y,
                        (int)g_game->width_142eb * u->radarDistance / g_game->mapWidth, base[0xa]);
                if (u->sonarDistance != 0)
                    DrawCircle((Surface *)surface, x, y,
                        (int)g_game->width_142eb * u->sonarDistance / g_game->mapWidth, base[0xa]);
                if (u->radarJamDistance != 0)
                    DrawCircle((Surface *)surface, x, y,
                        (int)g_game->width_142eb * u->radarJamDistance / g_game->mapWidth, base[0xc]);
                if (u->sonarJamDistance != 0)
                    DrawCircle((Surface *)surface, x, y,
                        (int)g_game->width_142eb * u->sonarJamDistance / g_game->mapWidth, base[0xc]);
            }
            if (u->flags & RADAR_UNIT_HAS_INTERCEPTORS) {
                for (int w = 0; w < 3; w++) {
                    if (u->weapons[w].interceptor) {
                        int r = ((int)g_game->width_142eb * (u->weapons[w].coverage - 0x200)) / g_game->mapWidth;
                        if (u->weapons[w].stocked)
                            DrawDashedCircle((Surface *)surface, x, y, r, base[0xf], 0x20,
                                         g_game->timer.byte.field_142f0.b.hi & 1);
                        else
                            DrawCircle((Surface *)surface, x, y, r, base[0xf]);
                    }
                }
            }
        }
        out->unitId = u->id;
        out->x = g_game->posX + x;
        out->y = g_game->posY + y;
        out++;
        g_game->hotRadarCount++;
    }

    const RadarProjectileCapture* p = (const RadarProjectileCapture*)u;
    for (unsigned int n = 0; n < capture->projectileCount; n++, p++) {
        int x = (int)g_game->width_142eb * p->x / g_game->mapWidth;
        int y = (int)g_game->screen_h * p->y / g_game->mapHeight;
        if (p->kind == RADAR_PROJECTILE_PIXEL)
            DrawPixel((Surface *)surface, x, y, base[0xe]);
        else
            DrawFrame(surface, GetGafFrame(g_game->nuclogo, p->color), x, y);
    }

    g_game->timer.byte.field_142f0.bits.bit1 = 1;
}
