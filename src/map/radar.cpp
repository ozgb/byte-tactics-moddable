#include "win_types.h"
#include <stdio.h>
#include <math.h>
#include "radar.h"

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

#include "../network/player_info.h"

#include "../network/player.h"

#include "../weapons/weapon_def.h"

#include "../util/angles.h"
#include "../weapons/unit_weapon_slot.h"

struct RadarUnitDefView {
    char unknown_0[0x204];
    short radardistance;
    short sonardistance;
    char unknown_208[0x20a - 0x208];
    short radardistancejam;
    short sonardistancejam;
    char unknown_20e[0x241 - 0x20e];
    InterceptorDefFlags flags1;
    unsigned char flags2;
};

#include "../units/unit.h"
#include "../weapons/projectile.h"

#include "../game/game.h"
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
void UpdateRadarMapped()
{
    if (g_game->timer.word.flags.mapChanged) {
        g_game->timer.word.flags.mapChanged = 0;
        unsigned char fog = g_game->textColor;
        Player* t = &g_game->players_1b63_14b[g_game->playerIndex];
        unsigned int mask = 1 << g_game->playerIndex;
        unsigned char* dst = *(unsigned char**)((char*)g_game->mappedSurface + 0xc);
        unsigned char* src = *(unsigned char**)((char*)g_game->pictureSurface + 0xc);
        int halfWidth = g_game->width / 2;
        int halfHeight = g_game->height / 2;
        for (int i = 0; i < g_game->screen_h; i++) {
            int mapY = i * halfHeight;
            int mapX = 0;
            int j = 0;
            while (j < g_game->width_142eb) {
                int index = (mapY / g_game->screen_h) * halfWidth + mapX / g_game->width_142eb;
                if (!(unsigned short)(g_game->visibilityMask[index] & mask)) {
                    *dst = fog;
                } else if (t->explored[index]) {
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
}

static Player* Player_Get(unsigned char p)
{
    return &g_game->players_1b63_14b[0] + p;
}

static inline int ExploredOnRadar(Player* pi, int px, int py)
{
    int tx = px >> 5;
    int ty = py >> 5;
    if ((unsigned int)tx < (unsigned int)pi->exploredWidth && (unsigned int)ty < (unsigned int)pi->exploredHeight &&
        pi->explored[(unsigned int)pi->exploredWidth * ty + tx])
        return 1;
    return 0;
}

static inline int VisibleOnRadar(Player* pi, int px, int py)
{
    int tx = px >> 5;
    int ty = py >> 5;
    if (!((unsigned int)tx < (unsigned int)pi->exploredWidth && (unsigned int)ty < (unsigned int)pi->exploredHeight)) {
        return 0;
    }
    Player* b = pi;
    return (g_game->visibilityMask[(unsigned int)b->exploredWidth * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int IsOnRadar(int px, int py)
{
    Player* pi = Player_Get(g_game->playerIndex);
    if ((g_game->mapFlags_14281.raw & 2) == 2)
        return ExploredOnRadar(pi, px, py);
    return VisibleOnRadar(pi, px, py);
}

static inline int ScaledRadarY(Unit* u)
{
    return ((int)u->zWhole - ((int)u->yWhole >> 1)) * (int)g_game->screen_h;
}

// FUNCTION: 0x466dc0
void DrawRadarUnits(void)
{
    unsigned char* base = (unsigned char*)g_game + 0xdcb;
    unsigned short* out = (unsigned short *)g_game->hotRadar;

    g_game->hotRadarCount = 0;
    void* surface = g_game->finalSurface;
    DrawSurface(surface, g_game->mappedSurface, 0, 0);

    int enabled;
    if (g_game->mapFlags_14281.bit0 || g_game->mapFlags_14281.bit1)
        enabled = 0;
    else
        enabled = 1;
    if ((g_game->matchFlags & 2))
        enabled = 1;

    Unit* u = g_game->units;
    Unit* end = g_game->unitsEnd;
    if (u <= end) {
        do {
            RadarUnitDefView* type = (RadarUnitDefView*)u->def.get();
            if (u->unitDefIndex != 0) {
                if (enabled != 0 || (u->flags & 0x300) != 0 ||
                    u->playerIndex == g_game->playerIndex) {
                    int x = u->xWhole * g_game->width_142eb /
                            g_game->mapWidth;
                    int y = ScaledRadarY(u) / g_game->mapHeight;
                    if (u->recentlyDamagedTimer == 0 ||
                        (g_game->timer.byte.field_142f0.b.hi & 1) != 0) {
                        DrawFrame(surface,
                            GetGafFrame(g_game->radlogo,
                                Player_Get(u->playerIndex)->info->color),
                            x, y);
                    }
                    if ((short)u->id == (short)g_game->hoverUnitId) {
                        DrawFrame(surface,
                            GetGafFrame(g_game->radlogohigh, 0), x, y);
                    }
                    if (u->selected) {
                        if ((u->activateFlags & 1) != 0 ||
                            (type->flags2 & 4) == 0) {
                            if (type->radardistance != 0)
                                DrawCircle((Surface *)surface, x, y,
                                    (int)g_game->width_142eb * type->radardistance /
                                    g_game->mapWidth, base[0xa]);
                            if (type->sonardistance != 0)
                                DrawCircle((Surface *)surface, x, y,
                                    (int)g_game->width_142eb * type->sonardistance /
                                    g_game->mapWidth, base[0xa]);
                            if (type->radardistancejam != 0)
                                DrawCircle((Surface *)surface, x, y,
                                    (int)g_game->width_142eb * type->radardistancejam /
                                    g_game->mapWidth, base[0xc]);
                            if (type->sonardistancejam != 0)
                                DrawCircle((Surface *)surface, x, y,
                                    (int)g_game->width_142eb * type->sonardistancejam /
                                    g_game->mapWidth, base[0xc]);
                        }
                        if (type->flags1.bit29) {
                            UnitWeaponSlot* slot = u->weapons;
                            int n = 3;
                            do {
                                if (slot->weapon->flags.b.interceptor) {
                                    int r = ((int)g_game->width_142eb *
                                             (slot->weapon->coverage - 0x200)) /
                                            g_game->mapWidth;
                                    if (slot->stockpile != 0)
                                        DrawDashedCircle((Surface *)surface, x, y, r, base[0xf],
                                                     0x20,
                                                     g_game->timer.byte.field_142f0.b.hi & 1);
                                    else
                                        DrawCircle((Surface *)surface, x, y, r, base[0xf]);
                                }
                                slot++;
                                n--;
                            } while (n != 0);
                        }
                    }
                    out[0] = u->id;
                    *(int*)(out + 1) = g_game->posX + x;
                    *(int*)(out + 3) = g_game->posY + y;
                    out += 5;
                    g_game->hotRadarCount++;
                }
            }
            u = (Unit*)((char*)u + 0x118);
        } while (u <= g_game->unitsEnd);
    }

    Projectile* p = g_game->projectiles;
    int i = 0;
    if (g_game->projectileCount > 0) {
        do {
            int px = p->positionWords.xWhole;
            int x = (int)g_game->width_142eb * px / g_game->mapWidth;
            int py = p->positionWords.zWhole - ((int)p->positionWords.yWhole >> 1);
            int y = (int)g_game->screen_h * py / g_game->mapHeight;
            if ((p->type->flags.value & 0x60000000) == 0) {
                if ((p->type->flags.value & 0x40) == 0) {
                    if (IsOnRadar(px, py) ||
                        p->owner ==
                            g_game->playerIndex) {
                        DrawPixel((Surface *)surface, x, y, base[0xe]);
                    }
                }
            } else {
                if (IsOnRadar(px, py) ||
                    p->attacker->playerIndex ==
                        g_game->playerIndex) {
                    DrawFrame(surface,
                        GetGafFrame(g_game->nuclogo,
                            Player_Get(
                                p->owner)->info->color),
                        x, y);
                }
            }
            i++;
            p++;
        } while (i < g_game->projectileCount);
    }

    g_game->timer.byte.field_142f0.bits.bit1 = 1;
}
