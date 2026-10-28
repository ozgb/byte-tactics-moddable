#include "win_types.h"
#include "../units/unit_def.h"
#include "../weapons/weapon_def.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#pragma pack(push, 1)

#include "../util/vec3.h"

struct Pos {
    Fixed x;
    Fixed y;
    Fixed z;

    Pos operator+(const Pos& other) const
    {
        Pos r;
        r.x.value = x.value + other.x.value;
        r.y.value = y.value + other.y.value;
        r.z.value = z.value + other.z.value;
        return r;
    }
};

#include "../util/vec3.h"
#include "../../interfaces/ref.h"

static inline int Vec3Length(const Vec3* v)
{
    double fx = v->x;
    double fy = v->y;
    double fz = v->z;
    return (int)sqrt(fx * fx + fy * fy + fz * fz);
}

struct View {
    char unknown_0[0x2c];
    int scroll_x;
    int scroll_y;
};

struct Anim {
    unsigned short count;
    char unknown_2[0x2c - 2];
    unsigned short duration;
};

struct Player;

#include "../units/unit.h"

#include "../orders/order.h"

struct MissionOrderTableEntry {
    char unknown_0[0x10];
    unsigned char markerAnim;
    char unknown_11[0x19 - 0x11];
};

#include "../game/game.h"
#include "../state/sim_globals.h"

#pragma pack(pop)

#include "../../link/globals.h"
typedef MissionOrderTableEntry* SimView_g_missionOrderTableBegin;
#undef g_missionOrderTableBegin
#define g_missionOrderTableBegin (*reinterpret_cast<SimView_g_missionOrderTableBegin*>(reinterpret_cast<char*>(&g_missionOrderTableVec) + 0x4))

void __stdcall DrawLine(void* surface, int x1, int y1, int x2, int y2, int color);
void __stdcall DrawString(void* surface, const char* text, int x, int y, int maxWidth);
int __stdcall GetGroundHeight(Pos* pos);
int __cdecl ScaledSin(int angle, int radius);
int __cdecl ScaledCos(int angle, int radius);
void __stdcall DrawFrame(void* surface, void* bmp, int x, int y);
void __stdcall DrawFrameBlended(void* dest, void* bmp, int x, int y);
int __stdcall IsUnitVisibleToPlayer(Player* owner, Unit* unit);
void __stdcall DrawRangeCircle(void* surface, View* view, Pos* pos,
                            int radius, int color, const char* text, int index);
#include "markers.h"

static int ScreenX(View* v, int x)
{
    return x - v->scroll_x + 0x80;
}

static int ScreenY(View* v, int z, int y)
{
    return z - (y >> 1) - v->scroll_y + 0x20;
}

// FUNCTION: 0x438c00
void __stdcall DrawBuildFootprint(void* surface, View* view, Order* order,
                            Pos* out, int unused)
{
    if ((unsigned short)order->type == 0)
        return;
    UnitDef* type = &g_game->defs.get()[(unsigned short)order->type];
    Pos lo = *(Pos*)&order->pos + *(Pos*)&type->modelMinX;
    Pos hi = *(Pos*)&order->pos + *(Pos*)&type->modelMaxX;
    Rect r;
    r.left = ScreenX(view, lo.x.whole);
    r.top = ScreenY(view, lo.z.whole, lo.y.whole);
    r.right = ScreenX(view, hi.x.whole);
    r.bottom = ScreenY(view, hi.z.whole, lo.y.whole);
    int level = __min(__max((unsigned int)g_game->ticks - order->created, 0), 10);
    int dx = (r.right - r.left) * level / 10;
    int dy = (r.bottom - r.top) * level / 10;
    unsigned char outer;
    unsigned char inner;
    if (order->unit->selected) {
        outer = g_game->colors[3];
        inner = g_game->colors[10];
    } else {
        outer = g_game->colors[1];
        inner = g_game->colors[9];
    }
    DrawLine(surface, r.left + dx - 1, r.top - 1, r.left + dx - 1, r.bottom + 1, outer);
    DrawLine(surface, r.right - dx + 1, r.top - 1, r.right - dx + 1, r.bottom + 1, outer);
    DrawLine(surface, r.left - 1, r.top + dy - 1, r.right + 1, r.top + dy - 1, outer);
    DrawLine(surface, r.left - 1, r.bottom - dy + 1, r.right + 1, r.bottom - dy + 1, outer);
    DrawLine(surface, r.left + dx, r.top, r.left + dx, r.bottom, inner);
    DrawLine(surface, r.right - dx, r.top, r.right - dx, r.bottom, inner);
    DrawLine(surface, r.left, r.top + dy, r.right, r.top + dy, inner);
    DrawLine(surface, r.left, r.bottom - dy, r.right, r.bottom - dy, inner);
    *out = *(Pos*)&order->pos;
}

static inline Vec3 Offset(int angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin(angle, distance);
    v.y = 0;
    int z = ScaledCos(angle, distance);
    v.z = -z;
    return v;
}

static inline Pos operator-(const Pos& p, const Vec3& v)
{
    Pos r;
    r.x.value = p.x.value - v.x;
    r.y.value = p.y.value - v.y;
    r.z.value = p.z.value - v.z;
    return r;
}

// FUNCTION: 0x438ea0
void __stdcall DrawRangeCircle(void* surface, View* view, Pos* pos, int radius,
                            int color, const char* text, int index)
{
    int angle = 0;
    if (radius) {
        int lx = 0;
        int ly = 0;
        double d = radius * TWO_PI;
        int n = (int)(d * ONE_EIGHTH);
        int i = 0;
        int x2;
        int y2;
        if (i <= n) {
            int step = 0x10000 / n;
            int rad = radius << 16;
            do {
                Pos p1 = *pos - Offset(angle, rad);
                p1.y.whole = __max(pos->y.whole, GetGroundHeight(&p1));
                angle += step;
                Pos p2 = *pos - Offset(angle, rad);
                p2.y.whole = __max(pos->y.whole, GetGroundHeight(&p2));
                int sx = view->scroll_x;
                int sy = view->scroll_y;
                y2 = p2.z.whole - (p2.y.whole >> 1) - sy + 0x20;
                x2 = p2.x.whole - sx + 0x80;
                DrawLine(surface, p1.x.whole - sx + 0x80,
                             p1.z.whole - (p1.y.whole >> 1) - sy + 0x20, x2, y2, color);
                if (i == index * 3) {
                    lx = x2;
                    ly = y2;
                }
                i++;
            } while (i <= n);
        }
        if (text) {
            if (lx == 0 && ly == 0) {
                lx = x2;
                ly = y2;
            }
            DrawString(surface, text, lx, ly + 4, -1);
        }
    }
}

// FUNCTION: 0x4390a0
void __stdcall DrawUnitRangeRings(void* surface, View* view, Order* order,
                            int unused1, int unused2)
{
    Unit* unit = order->unit;
    UnitDef* def = unit->def.get();
    int index = 0;
    if (g_game->showRanges == 0) {
        short mincloak = def->mincloakdistance;
        if (mincloak != 0 && (unit->activateFlags & 4)) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, mincloak, g_game->colors[15], 0, 0);
        }
        if ((def->flags1 & 0x10000000) && def->explodeas != 0) {
            int r = def->explodeas->areaOfEffect;
            r = r >> 1;
            unsigned int t = ((unsigned int)g_game->ticks % 60) * r * 2 / 60;
            int radius = (t < 8) ? 8 : t;
            if (radius >= r)
                radius = r;
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, radius, g_game->colors[12], 0, 0);
            if (unit->motion != 0) {
                DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->kamikazeUnsigned,
                             g_game->colors[12], 0, 0);
                return;
            }
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->range, g_game->colors[12], 0, 0);
            return;
        }
    } else {
        short mincloak = def->mincloakdistance;
        if (mincloak != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, mincloak, g_game->colors[14],
                         "mincloak", index++);
        }
        if (def->range != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->range, g_game->colors[14],
                         "sight", index++);
        }
        if (def->radardistance != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->radardistance, g_game->colors[14],
                         "radar", index++);
        }
        if (def->sonardistance != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->sonardistance, g_game->colors[14],
                         "sonar", index++);
        }
        if (def->radardistancejam != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->radardistancejam, g_game->colors[14],
                         "radarjam", index++);
        }
        if (def->sonardistancejam != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->sonardistancejam, g_game->colors[14],
                         "sonarjam", index++);
        }
        if (def->buildRange != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->buildRange, g_game->colors[14],
                         "build distance", index++);
        }
        if (def->maneuverleashUnsigned != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->maneuverleashUnsigned, g_game->colors[14],
                         "maneuver", index++);
        }
        if (def->kamikazeUnsigned != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, def->kamikazeUnsigned,
                         g_game->colors[14], "kamikazedistance", index);
        }
        int color;
        if ((unsigned int)g_game->ticks & 1)
            color = g_game->colors[4];
        else
            color = g_game->colors[12];
        if ((unit->weapons[0].flags & 2) && unit->weapons[0].weapon.get()->range != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, unit->weapons[0].weapon.get()->range, color,
                         "weapon1 range", 0);
        }
        if ((unit->weapons[1].flags & 2) && unit->weapons[1].weapon.get()->range != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, unit->weapons[1].weapon.get()->range, color,
                         "weapon2 range", 1);
        }
        if ((unit->weapons[0].flags & 2) && unit->weapons[2].weapon.get()->range != 0) {
            DrawRangeCircle(surface, view, (Pos*)&order->unit->pos, unit->weapons[2].weapon.get()->range, color,
                         "weapon3 range", 2);
        }
    }
}

static Pos offset_004394e0(int dx, int dy, int dz, int f)
{
    Pos d;
    d.x.value = (int)(((__int64)dx * f) >> 16);
    d.y.value = (int)(((__int64)dy * f) >> 16);
    d.z.value = (int)(((__int64)dz * f) >> 16);
    return d;
}

struct Trail_004394e0 {
    Pos start;
    Vec3 delta;
    int dist;
    int pos;
    int idx;
    void* surface;
    View* view;
    Anim* anim;

    void Run()
    {
        for (; pos < dist; pos += 0x300000) {
            int f = (int)(((__int64)pos << 16) / dist);
            Pos o = offset_004394e0(delta.x, delta.y, delta.z, f);
            Pos p;
            p.x.value = start.x.value + o.x.value;
            p.y.value = start.y.value + o.y.value;
            p.z.value = start.z.value + o.z.value;
            DrawFrame(surface, *(void**)((char*)anim + idx * 8 + 0x28),
                         p.x.whole - view->scroll_x + 0x80,
                         p.z.whole - (p.y.whole >> 1) - view->scroll_y + 0x20);
            idx = (idx + 1) % anim->count;
        }
    }
};

// FUNCTION: 0x4394e0
void __stdcall DrawPathAnim(void* surface, View* view,
                            Order* order, Pos* out, int flag)
{
    Pos start = *out;
    DrawWeaponCoverage(surface, view, order, out, flag);
    if (flag == 0)
        return;

    Pos end = *out;
    int t = __max((unsigned int)g_game->ticks - order->created, 0);
    Vec3 d;
    d.x = end.x.value - start.x.value;
    d.y = end.y.value - start.y.value;
    d.z = end.z.value - start.z.value;
    int dist = Vec3Length(&d);
    if (dist < 0x10000)
        return;

    Anim* anim = (Anim *)g_game->pathIcon;
    int pos = (t % 30) * 0x300000 / 30;
    unsigned short len = anim->duration;
    int frames = len < 1 ? 1 : (int)len;
    unsigned int idx = (t / frames) % anim->count;

    Trail_004394e0 tr;
    tr.start = start;
    tr.delta = d;
    tr.dist = dist;
    tr.idx = idx;
    tr.surface = surface;
    tr.view = view;
    tr.pos = pos;
    tr.anim = anim;
    tr.Run();
}

// FUNCTION: 0x439740
void __stdcall DrawWeaponCoverage(void* surface, View* view, Order* order,
                            Pos* out, int unused)
{
    char buf[0x40];
    Pos pos;
    Unit* u = order->unit;
    if (order->target != 0) {
        if (IsUnitVisibleToPlayer(u->player, order->target) == 0 && (order->flags_42 & 0x200000) != 0) {
            pos.x.value = order->cached.x << 16;
            pos.y.value = order->target->pos.y;
            pos.z.value = order->cached.y << 16;
        } else {
            pos = *(Pos*)&order->target->pos;
            order->cached.x = pos.x.whole;
            order->cached.y = pos.z.whole;
            order->flags_42 |= 0x200000;
        }
    } else {
        pos = *(Pos*)&order->pos;
    }
    if (g_missionOrderTableBegin[order->kind.index].markerAnim == 0) {
        *out = pos;
        return;
    }
    if (g_game->showRanges != 0 &&
        (g_missionOrderTableBegin[order->kind.index].markerAnim == 1 || g_missionOrderTableBegin[order->kind.index].markerAnim == 2)) {
        int color;
        if ((unsigned int)g_game->ticks & 1)
            color = g_game->colors[4];
        else
            color = g_game->colors[12];
        for (int i = 0; i < 3; i++) {
            if (u->weapons[(unsigned char)i].flags & 2) {
                if (u->weapons[i].weapon.get()->areaOfEffect != 0) {
                    sprintf(buf, "weapon %d - area of effect", i);
                    DrawRangeCircle(surface, view, &pos, u->weapons[i].weapon.get()->areaOfEffect, color, buf, 0);
                }
                if (u->weapons[i].weapon.get()->coverage != 0) {
                    sprintf(buf, "weapon %d - coverage", i);
                    DrawRangeCircle(surface, view, &pos, u->weapons[i].weapon.get()->coverage, color, buf, 1);
                }
            }
        }
        unsigned short len = u->def->attackrunlength;
        if (len != 0)
            DrawRangeCircle(surface, view, &pos, len, color, "attack length", 2);
    }
    Anim* anim = (*(Anim * (*)[21])&g_game->cursorModeSeq)[g_missionOrderTableBegin[order->kind.index].markerAnim];
    unsigned int n = (unsigned int)g_game->ticks / ((unsigned int)anim->duration * 2);
    n = n % anim->count;
    void* bmp = *(void**)((char*)anim + n * 8 + 0x28);
    DrawFrameBlended(surface, bmp, pos.x.whole - view->scroll_x + 0x80,
                 pos.z.whole - (pos.y.whole >> 1) - view->scroll_y + 0x20);
    *out = pos;
}

// FUNCTION: 0x4399f0
void __stdcall DrawOrderRangeRing(void* surface, View* view, Order* order,
                            Pos* out, int unused)
{
    Pos p;
    int height;
    if (order->target != 0) {
        p = *(Pos*)&order->target->pos;
        height = order->target->def->modelSizeXHigh;
    } else {
        p = *(Pos*)&order->pos;
        height = 0x20;
    }
    int ry = (int)(height * 0.89);
    int xc = p.x.whole - view->scroll_x + 0x80;
    int yc = p.z.whole - view->scroll_y - (p.y.whole >> 1) + 0x20;
    int x1 = xc + height;
    int y1 = yc;
    for (int angle = 0x1000; angle <= 0x10000; angle += 0x1000) {
        int nx = ScaledCos(angle, height) + xc;
        int ny = ScaledSin(angle, ry) + yc;
        DrawLine(surface, x1, y1, nx, ny, g_game->colors[12]);
        x1 = nx;
        y1 = ny;
    }
    *out = p;
}
