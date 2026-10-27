#include "win_types.h"
#include "../../interfaces/ref.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

#pragma pack(push, 1)

#include "../util/vec3.h"
#include "../util/point.h"

#include "../util/angles.h"

#include "../units/unit_def.h"
#include "../network/player.h"

#pragma pack(push, 1)
struct UnitDefFlags_00467440 {
    char unknown_0[0x241];
    unsigned int bit0_7 : 8;
    unsigned int bit8 : 1;
    unsigned int bit9_31 : 23;
};
#pragma pack(pop)

struct UnitType_004685a0 {
    char name[0x152];
    int count;
    Ref<unsigned short> ids;
    char unknown_15a[0x22f - 0x15a];
    unsigned char mobile;
};

#include "../network/player_info.h"

struct PlayerInfo_004685a0 {
    int f_0;
    char unknown_4[0x2b - 4];
    char name[0x73 - 0x2b];
    unsigned char controller;
    char unknown_74[0x146 - 0x74];
    unsigned char player;
    char unknown_147[0x22f - 0x147];
    unsigned char mobile;
};

struct Unit_00467960 {
    char unknown_0[0x110];
    unsigned int flags;
};

struct Object_004cb650;

#include "../units/unit.h"
#include "../weapons/weapon_def.h"
#include "../map/cell.h"
#include "map_grid.h"
#include "../map/mission.h"
#include "../orders/order_list.h"
#include "../units/unit_commands.h"
#include "../graphics/gaf_frame.h"

struct Team_004689c0 {
    char unknown_0[4];
    unsigned char* data;
    char unknown_8[347 - 8];
};

#include "../map/feature.h"

struct ObjectState {
    char unknown_0[0xc];
    Unit* unit;
};

struct DisplayContextView {
    char unknown_0[0x9c];
    int haveBackBuffer;
    char unknown_a0[0xf0 - 0xa0];
    unsigned char flags;
};

struct LayerView {
    int unknown_0;
    int value;
};

#include "feature_spot.h"

struct Def_004685a0 {
    char unknown_0[0x20];
    char name[0x229];
};

#include "../game/game.h"

class DetectionVisitor {
public:
    virtual void MarkUnitsInRadarOrSonarRadius(Unit* unit);
    int radarRangeSq;
    int sonarRangeSq;
    Vec3 pos;
};

class RadarJamVisitor {
public:
    virtual void ApplyRadarJamFlag(Unit* unit);
};

class SonarJamVisitor {
public:
    virtual void ApplySonarJamFlag(Unit* unit);
};

#include "../graphics/rect.h"

struct Colors_00467b60 {
    char unknown_0[4];
    unsigned char empty;
    char unknown_5[5];
    unsigned char full;
};

struct Quad_467c00 {
    Point p[4];
};

struct Entry_467c00 {
    unsigned short w;
    unsigned short h;
};

struct Mission_00467e50 {
    char pad_0[5];
    unsigned char state;
    char pad_6[0x16 - 0x6];
    Ref<Unit> target;
    char pad_1a[0x4a - 0x1a];
    Ref<Mission_00467e50> next;
};

#include "../orders/order_type_entry.h"
#include "../graphics/surface.h"

#include "frame_timers.h"

struct Flags_0046a530 {
    unsigned short low : 2;
    unsigned short flag : 1;
    unsigned short rest : 13;
};

struct Size_0046b9d0 {
    short w;
    short h;
};

class Prim_0046bae0 {
public:
    int colorIndex;
    int count;
    int always0;
    unsigned short* indices;
    int pic;
    char unknown_14[8];
    unsigned int flag0 : 1;
    unsigned int texIndexed : 1;
    unsigned int rest : 30;
};

class Object_0046bae0 {
public:
    int version;
    int count;
    int faceCount;
    int firstFace;
    char unknown_10[0x24 - 0x10];
    Vec3* verts;
    Prim_0046bae0* prims;
};

#pragma pack(pop)

#include "../../link/globals.h"
#include "../../interfaces/seams.h"


unsigned short* __stdcall FindGafEntry(void* gaf, const char* name);
int __stdcall GetGafFrame(unsigned short* param_1, int param_2);

void __stdcall DrawFrame(void* param_1, short* param_2, int x, int y);
void __stdcall DrawFrame(void* dst, void* bmp, int x, int y);

void __stdcall RotateByAngles(Vec3* in, Vec3* out, short* angles);
void __stdcall DrawLine(void* surface, int x1, int y1, int x2, int y2, int color);

void __stdcall FillRectangle(void* surface, Rect* rect, int color);
int __stdcall FillRectangle(void* surface, void* rect, int color);
void __stdcall FillRectangle(int surface, Rect* rect, int color);

void __stdcall DrawRectangle(void* surface, Rect* rect, int color);
void __stdcall DrawRectangle(void* surface, void* rect, int color);
void __stdcall DrawRectangle(int surface, Rect* rect, int color);

void __stdcall DrawFrameQuad(void* surf, void* entry, Quad_467c00* dst, Quad_467c00* src);

char* __stdcall Translate(char* text);
#include "../graphics/draw.h"
void __stdcall SetTextColors(int param_1, int param_2);
void __stdcall SetFont(int param_1);
void __stdcall DrawString(void* surface, const char* text, int x, int y, int maxWidth);
void __stdcall DrawString(int surface, const char* text, int x, int y, int maxWidth);

#include "../graphics/display.h"
void __stdcall SetOffscreenSurface(void* surf);
void FlipScreen();

#include "../network/net_stats.h"
#include "../ai/ai_player.h"

void __stdcall PlaySoundByName(char* name, int param_2);
#include "../gui/keyboard.h"
#include "../gui/gui.h"

void __stdcall GetObjectBounds(Object_004cb650* obj, Vec3* lo, Vec3* hi, int arg);
#include "info_panel.h"

int __stdcall GetGafSequenceFrame(short* ref);
void __stdcall FillPolygon(void* surface, Point* points, int count, int color);
void __stdcall DrawFrameQuad(void* surface, void* texture, Point* points, void* src);

int __stdcall GetCellHeight(void* p);

#include "../network/unit_sync.h"
void __stdcall AddMessage(char* param_1, int param_2, int param_3, int param_4);
#include "../gui/dialogs.h"

int __stdcall FormatNetStats(int);
int __stdcall DrawSelectedGoal(int, int);
int __stdcall DrawMapDebugOverlay(int);
int __stdcall DrawExplosions(int);
int __stdcall DrawUnit(int, int);
void __stdcall DrawUnit(void* dest, Unit* unit);
int __stdcall DrawOptionsScrollBar(int);
int __stdcall DrawMessages(int);
float __stdcall GetEnergyIncome(int);
float __stdcall GetEnergyUsage(int);
float __stdcall GetMetalIncome(int);
float __stdcall GetMetalUsage(int);
int __stdcall IsFootprintVisible(int, int, int, int, int, int);
int __stdcall DrawRadar(int);
int __stdcall DrawFogOfWar(int);
int __stdcall FindNextSelectedUnit(int, int);
int __stdcall DrawSelectedUnitOrderOverlays(int, int);
int __stdcall DrawScorePanel(int);
int __stdcall DrawProjectiles(int);
int __stdcall BlitMenuLayers(int, int, int);
int __stdcall DrawParticleList(int, int);
int __stdcall DrawMapTiles(int);
int __stdcall PointInRect(int, int, int);
int __stdcall DrawLine(int, int, int, int, int, int);
int __stdcall GetTextWidth(int, int);
int __stdcall Translate(int);
int __stdcall ResetClipRect(int);
int HideSoftwareCursor();
int ShowSoftwareCursor();
int __stdcall GetOrderTarget(void* unit);
int __stdcall GetGafSequenceFrame(Handle* h);
int __stdcall IsUnitVisibleToPlayer(void* map, void* u);
void __stdcall DrawFrameBlended(void* dest, short* frame, int x, int y);

static inline int IsExplored(Player* player, Vec3Words* pos)
{
    int tx = pos->xWhole >> 5;
    int ty = (pos->zWhole - (pos->yWhole >> 1)) >> 5;
    if ((unsigned int)tx < (unsigned int)player->exploredWidth && (unsigned int)ty < (unsigned int)player->exploredHeight &&
        player->explored[(unsigned int)player->exploredWidth * ty + tx] != 0)
        return 1;
    return 0;
}

static inline int IsSeen(Player* player, Vec3Words* pos)
{
    int tx = pos->xWhole >> 5;
    int ty = (pos->zWhole - (pos->yWhole >> 1)) >> 5;
    if (!((unsigned int)tx < (unsigned int)player->exploredWidth && (unsigned int)ty < (unsigned int)player->exploredHeight))
        return 0;
    return (g_game->visibilityMask[(unsigned int)player->exploredWidth * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

// FUNCTION: 0x467440
void UpdateSensorRadarAndCloak(void)
{
    if (g_game->numPlayers < 2) {
        return;
    }
    unsigned char player = g_game->playerIndex;
    Unit* first = g_game->units + 1;
    Unit* last = g_game->unitsEnd;
    Player* pl = &g_game->players[g_game->playerIndex];
    Unit* u;

    Unit* a;
    for (a = first; a <= last; a++) {
        if (a->flags & 0x10000000) {
            a->flags &= ~0x1000;
            if (a->playerIndex == player
                || (a->player->allied[pl->index] != 0
                    && (a->player->info->flags_97 & 0x40) != 0)
                || (pl->active != 0 && (pl->info->flags_9b & 0x40) != 0)) {
                a->flags |= 0x300;
            } else {
                a->flags &= ~0x700;
            }
        }
    }

    for (u = pl->unitsBegin; u <= pl->unitsEnd; u++) {
        if ((u->flags & 0x10000000) && !(u->flags & 0x4000) && (u->activateFlags & 1)) {
            if (u->def->radardistance != 0 || u->def->sonardistance != 0) {
                short a = u->def->radardistance;
                int t = a + u->yWhole * 2;
                short b = u->def->sonardistance;
                t = t * t;
                int s = (int)b * (int)b;
                if (a <= b) {
                    a = b;
                }
                Vec3* pp = &u->pos;
                DetectionVisitor v;
                v.radarRangeSq = t;
                v.sonarRangeSq = s;
                v.pos = u->pos;
                g_seams.map->VisitObjectsInRange((Vec3*)pp, (int)a << 16, (UnitVisitor*)&v);
            }
        }
    }

    for (u = first; u <= last; u++) {
        unsigned char ff;
        if ((u->flags & 0x10000000) && (ff = u->playerIndex, 1) && ff != pl->index && (u->activateFlags & 1)) {
            if (u->def->radardistancejam != 0) {
                int r = (int)u->def->radardistancejam << 16;
                Vec3* pp = &u->pos;
                RadarJamVisitor v;
                g_seams.map->VisitObjectsInRange((Vec3*)pp, r, (UnitVisitor*)&v);
            }
            if (u->def->sonardistancejam != 0) {
                int r2 = (int)u->def->sonardistancejam << 16;
                Vec3* pp2 = &u->pos;
                SonarJamVisitor v;
                g_seams.map->VisitObjectsInRange((Vec3*)pp2, r2, (UnitVisitor*)&v);
            }
        }
    }

    for (u = first; u <= last; u++) {
        if ((u->flags & 0x10000000) && u->player->active != 0) {
            char c = (char)u->player->type;
            if (c == 1 || c == 2) {
                if (u->def->flags2 & 0x2000) {
                    if (g_seams.ai->HasReadyUnitInRange(u->playerIndex, (Vec3*)&u->pos, u->def->mincloakdistance)) {
                        int& b0 = u->workTime;
                        b0 = g_game->ticks + 0x5a;
                        u->flags |= 0x1000;
                    }
                }
            }
        }
    }

    for (u = first; u <= last; u++) {
        unsigned int f = u->flags;
        if ((f & 0x10000000) && !(f & 0x100) && !(u->activateFlags & 4)) {
            Player* p2 = g_game->Current();
            unsigned int vis;
            if ((g_game->mapFlags & 2) == 2) {
                vis = IsExplored(p2, (Vec3Words*)&u->pos);
            } else {
                vis = IsSeen(p2, (Vec3Words*)&u->pos);
            }
            if ((int)vis) {
                u->flags = f | 0x100;
            }
        }
    }
}

// FUNCTION: 0x467840
void DetectionVisitor::MarkUnitsInRadarOrSonarRadius(Unit* unit)
{
    if (unit->unitDefIndex == 0 || unit->playerIndex == g_game->playerIndex) {
        return;
    }

    UnitDef* def = unit->def;
    if (((UnitDefFlags_00467440*)def)->bit8) {
        return;
    }

    int dz = unit->pos.z - this->pos.z;
    int dx = unit->pos.x - this->pos.x;
    int dist = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);

    if (unit->pos.y <= ((int)g_game->seaLevel << 16) && dist < this->sonarRangeSq) {
        unit->flags |= 0x200;
    }

    if (def->modelMaxY + unit->pos.y >= ((int)g_game->seaLevel << 16) && dist < this->radarRangeSq) {
        unit->flags |= 0x100;
    }
}

// FUNCTION: 0x467950
void __stdcall MarkRecentlyDamaged(void* param_1)
{
    *(char*)((char*)param_1 + 0xfa) = 0xf0;
}

// FUNCTION: 0x467960
void RadarJamVisitor::ApplyRadarJamFlag(Unit* unit)
{
    unit->flags = (unit->flags & ~0x100) | 0x400;
}

// FUNCTION: 0x467980
void SonarJamVisitor::ApplySonarJamFlag(Unit* unit)
{
    unit->flags = (unit->flags & ~0x200) | 0x400;
}

// FUNCTION: 0x4679a0
void LoadLightBar()
{
    Sub_004679a0_a* a = &g_game->a;
    a->field_1 = a->field_d = a->field_19 = a->field_1d = 0;
    Sub_004679a0_b* b = &g_game->b;
    b->field_30 = 0;
    b->lightbar = 0;
    unsigned short* frames = FindGafEntry(g_game->gaf, "LIGHTBAR");
    g_game->b.lightbar = (unsigned short*)GetGafFrame(frames, 1);
    g_game->b.lightbar[3] = 0;
    g_game->b.lightbar[2] = 0;
}

// FUNCTION: 0x467a20
void __stdcall BlitGafFrameAtOffset(void* param_1, short* param_2, int param_3, int param_4)
{
    DrawFrame(param_1, param_2, param_2[2] + param_3, param_2[3] + param_4);
}

// FUNCTION: 0x467a50
void __stdcall DrawRotatedQuadOutline(void* surface, Vec3* offset,
                            Vec3* corners, short* angles)
{
    Vec3* scratch = (Vec3 *)g_game->xform;
    Point* points = (Point *)g_game->projected;
    unsigned char color = g_game->colors[10];
    Vec3* c = corners;

    for (int i = 0; i < 4; i++) {
        RotateByAngles(c, scratch, angles);
        int y = (short)((scratch->y + offset->y) >> 16);
        int z = (short)((offset->z - scratch->z) >> 16);
        int x = (short)((scratch->x + offset->x) >> 16);
        points->x = x + 0x80;
        points->y = z - (y >> 1) + 0x20;
        c++;
        scratch++;
        points++;
    }

    Point* quad = (Point *)g_game->projected;
    DrawLine(surface, quad[0].x, quad[0].y, quad[1].x, quad[1].y, color);
    DrawLine(surface, quad[1].x, quad[1].y, quad[2].x, quad[2].y, color);
    DrawLine(surface, quad[2].x, quad[2].y, quad[3].x, quad[3].y, color);
    DrawLine(surface, quad[3].x, quad[3].y, quad[0].x, quad[0].y, color);
}

// FUNCTION: 0x467b60
void __stdcall DrawProgressBar(void* surface, int value, int max, Rect* rect, Colors_00467b60* colors, int dy)
{
    Rect r = *rect;
    r.top += dy;
    r.bottom += dy;
    if (value < 0)
        value = 0;
    if (value > max)
        value = max;
    r.right = (rect->right - rect->left) * value / max + r.left;
    FillRectangle(surface, &r, colors->full);
    if (r.right != rect->right) {
        r.left = r.right + 1;
        r.right = rect->right;
        FillRectangle(surface, &r, colors->empty);
    }
}

// FUNCTION: 0x467c00
void __stdcall BlitSideLogoToRect(void* surf, Player* player, Rect* rect, int dy)
{
    unsigned char idx = player->info->color;
    Entry_467c00* entry = (Entry_467c00*)*(void**)((char*)g_game->logos32 + idx * 8 + 0x28);

    Quad_467c00 src;
    src.p[0].x = 0;
    src.p[0].y = 0;
    src.p[1].x = entry->w;
    src.p[1].y = 0;
    src.p[2].x = entry->w;
    src.p[2].y = entry->h;
    src.p[3].x = 0;
    src.p[3].y = entry->h;

    Quad_467c00 dst;
    dst.p[0].x = rect->left;
    dst.p[0].y = rect->top + dy;
    dst.p[1].x = rect->right;
    dst.p[1].y = rect->top + dy;
    dst.p[2].x = rect->right;
    dst.p[2].y = rect->bottom + dy;
    dst.p[3].x = rect->left;
    dst.p[3].y = rect->bottom + dy;

    DrawFrameQuad(surf, entry, &dst, &src);
}

// FUNCTION: 0x467cb0
void __stdcall DrawKillCount(void* surface, Unit* unit, int x, int y)
{
    char buf[100];
    char* kills = Translate("kills");
    char* kill = Translate("kill");
    if (unit->killCount > 4) {
        sprintf(buf, "%d %s - %s", unit->killCount,
                unit->killCount == 1 ? kill : kills, Translate("Veteran"));
    } else {
        sprintf(buf, "%d %s", unit->killCount,
                unit->killCount == 1 ? kill : kills);
    }
    SetTextColors(g_game->colors[15], GetTextKeyColor());
    DrawString(surface, buf, x, y, -1);
}

// FUNCTION: 0x467d70
void DrawLightBars()
{
    void* surf = (void *)g_game->screen;
    SetOffscreenSurface(surf);
    FillSurface((Surface *)surf, 0);

    int side = g_game->players[g_game->playerIndex].info->side;

    short* bar = (short*)GetGafFrame(g_game->sidePanelTopSeq[side], 0);
    DrawFrame(surf, bar, bar[2] + 0x81, bar[3]);

    int dy = GetScreenHeight() - 0x20;
    bar = (short*)GetGafFrame(g_game->sidePanelBotSeq[side], 0);
    DrawFrame(surf, bar, bar[2] + 0x81, bar[3] + dy);

    bar = (short*)GetGafFrame(g_game->sidePanelSideSeq[side], 0);
    DrawFrame(surf, bar, bar[2], bar[3]);

    FlipScreen();
}

// FUNCTION: 0x467e50
int __stdcall DrawUnitStateProbe(void* surface)
{
    char buf[0x80];
    char* names[3];
    unsigned char* colors;
    Unit* unit;
    Rect r;
    int lineH;
    int y;
    int prev;
    Mission_00467e50* m;

    if (g_game->unitInfoHoverTrack == 0 || g_game->unitInfoHoverUnitId == 0)
        return 0;
    unit = (Unit*)((char*)g_game->units.get()
                            + g_game->unitInfoHoverUnitId * 0x118);
    if ((((int)unit->flags) & 0x10000000) == 0 || (((int)unit->flags) & 0x4000) != 0) {
        g_game->unitInfoHoverTrack = 0;
        g_game->unitInfoHoverUnitId = 0;
    }
    colors = &g_game->colors[0];
    SetTextColors(colors[15], GetTextKeyColor());
    SetFont(g_game->fontComix);
    lineH = GetFontHeight() + 3;
    y = lineH * 7;
    GetDisplayFieldE4();
    prev = g_probePanelBottom;
    r.left = 0x83;
    r.right = 0x191;
    r.top = y;
    r.bottom = prev == 0 ? lineH * 20 : prev;
    FadeRectangle((Surface *)surface, &r, -0x18);
    r.right++;
    r.bottom++;
    DrawRectangle(surface, &r, colors[5]);
    y += 3;
    DrawString(surface, "Unit State Probe", 0x86, y, -1);
    y += lineH;
    DrawString(surface, "================", 0x86, y, -1);
    y += lineH;
    sprintf(buf, "uid: %03d/%04x '%s'\n", unit->id, unit->id, ((char *)unit->def.get()));
    DrawString(surface, buf, 0x86, y, -1);
    y += lineH;
    sprintf(buf, "playerno: %d '%s' %s - %s\n",
            *(unsigned char*)((char*)((int *)unit->player.get()) + 0x146),
            (char*)((int *)unit->player.get()) + 0x2b,
            (*((int *)unit->player.get()) != 0
             && (*(unsigned char*)((char*)((int *)unit->player.get()) + 0x73) == 1
                 || *(unsigned char*)((char*)((int *)unit->player.get()) + 0x73) == 2))
                ? "LOCAL" : "REMOTE",
            *(unsigned char*)(((char *)unit->def.get()) + 0x22f) != 0 ? "MOBILE" : "BUILDING");
    DrawString(surface, buf, 0x86, y, -1);
    y += lineH;
    sprintf(buf, "controller: %d\n", *(unsigned char*)((char*)((int *)unit->player.get()) + 0x73));
    DrawString(surface, buf, 0x86, y, -1);
    y += lineH;
    sprintf(buf, "buildtimeleft: %1.3f\n", unit->buildLeft);
    DrawString(surface, buf, 0x86, y, -1);
    y += lineH;
    sprintf(buf, "damage: %d\n", unit->health);
    DrawString(surface, buf, 0x86, y, -1);
    y += lineH;
    names[0] = "NONE";
    names[1] = "GROUND";
    names[2] = "AIR";
    sprintf(buf, "occupy: %s\n", names[((int)unit->flags) & 3]);
    DrawString(surface, buf, 0x86, y, -1);
    y += lineH;
    if (*((int *)unit->player.get()) != 0
        && (*(unsigned char*)((char*)((int *)unit->player.get()) + 0x73) == 1
            || *(unsigned char*)((char*)((int *)unit->player.get()) + 0x73) == 2)) {
        sprintf(buf, "autotarget w[pri:sec:spe]: w[%c:%c:%c]\n",
                (unit->f_1f & 0x10) ? 'X' : '-',
                (unit->f_3b & 0x10) ? 'X' : '-',
                (unit->f_57 & 0x10) ? 'X' : '-');
        DrawString(surface, buf, 0x86, y, -1);
        y += lineH;
        if (((void *)unit->list) != 0) {
            DrawString(surface, "Mission Q:", 0x86, y, -1);
            y += lineH;
            for (m = (Mission_00467e50*)((void *)unit->list); m != 0;
                 m = m->next) {
                if (m->target != 0)
                    sprintf(buf, "    '%s' state: %d  tgt: '%s'\n",
                            ((OrderType*)((char*)m + 4))->GetTableEntry()->name,
                            m->state, (char*)m->target->def.get());
                else
                    sprintf(buf, "    '%s' state: %d\n",
                            ((OrderType*)((char*)m + 4))->GetTableEntry()->name,
                            m->state);
                DrawString(surface, buf, 0x86, y, -1);
                y += lineH;
            }
        }
        if (((void *)unit->list2) != 0) {
            DrawString(surface, "Background Mission Q:", 0x86, y, -1);
            y += lineH;
            for (m = (Mission_00467e50*)((void *)unit->list2); m != 0;
                 m = m->next) {
                if (m->target != 0)
                    sprintf(buf, "    '%s' state: %d  tgt: '%s'\n",
                            ((OrderType*)((char*)m + 4))->GetTableEntry()->name,
                            m->state, (char*)m->target->def.get());
                else
                    sprintf(buf, "    '%s' state: %d\n",
                            ((OrderType*)((char*)m + 4))->GetTableEntry()->name,
                            m->state);
                DrawString(surface, buf, 0x86, y, -1);
                y += lineH;
            }
        }
    }
    g_probePanelBottom = y;
    return 1;
}

// FUNCTION: 0x468310
void __stdcall DrawPercentBar(void* surface, Rect* rect, int percent)
{
    unsigned char& color = g_game->colors[15];
    DrawRectangle(surface, rect, color);
    if (percent >= 100)
        percent = 100;
    if (percent > 0) {
        rect->right = (rect->right - rect->left) * percent / 100 + rect->left;
        FillRectangle(surface, rect, color);
    }
}

// FUNCTION: 0x468380
void __stdcall DrawNetworkStats(void* surface)
{
    unsigned int sent;
    unsigned int received;
    char buf[0x20];
    Rect r;
    r.left = 0x81;
    r.right = 0xc1;
    r.top = g_game->screenHeight - 0x5f;
    r.bottom = r.top + 8;
    int h = GetFontHeight();
    GetByteRates(&sent, &received);
    sprintf(buf, "Send - %1.1f K/s", sent * 0.001);
    DrawString(surface, buf, r.left, r.top, -1);
    r.top += h;
    r.bottom = r.top + 8;
    DrawPercentBar(surface, &r, sent * 100 / 5600);
    r.left = 0x81;
    r.right = 0xc1;
    r.top = r.bottom + 1;
    sprintf(buf, "Receive - %1.1f K/s", received * 0.001);
    DrawString(surface, buf, r.left, r.top, -1);
    r.top += h;
    r.bottom = r.top + 8;
    DrawPercentBar(surface, &r, received * 100 / 5600);
}

static void Bar_004685a0(void* surface, Rect* rect, int percent)
{
    unsigned char& color = g_game->colors[15];
    DrawRectangle(surface, rect, color);
    percent = (percent >= 100) ? 100 : percent;
    if (percent > 0) {
        rect->right = (rect->right - rect->left) * percent / 100 + rect->left;
        FillRectangle(surface, rect, color);
    }
}

// FUNCTION: 0x4685a0
int __stdcall DrawUnitBuilderProbe(void* surface)
{
    if (g_game->unitBuilderHoverTrack == 0 || g_game->unitBuilderHoverUnitId == 0)
        return 0;
    Unit* unit = &g_game->units[g_game->unitBuilderHoverUnitId];
    if ((((int)unit->flags) & 0x10000000) == 0 || (((int)unit->flags) & 0x4000) != 0
            || ((UnitType_004685a0 *)unit->def.get())->ids == 0) {
        g_game->unitInfoHoverTrack = 0;
        g_game->unitInfoHoverUnitId = 0;
    }
    unsigned char* colors = g_game->colors;
    SetTextColors(colors[15], GetTextKeyColor());
    SetFont(g_game->fontComix);
    int lineHeight = GetFontHeight() + 3;
    int y = lineHeight * 3;
    GetDisplayFieldE4();
    Rect r;
    r.left = 0x83;
    r.right = 0x191;
    r.top = y;
    if (g_probePanelBottom == 0)
        r.bottom = lineHeight * 20;
    else
        r.bottom = g_probePanelBottom;
    FadeRectangle((Surface *)surface, &r, -0x18);
    r.right++;
    r.bottom++;
    DrawRectangle(surface, &r, colors[5]);
    char buf[0x80];
    y += 3;
    DrawString(surface, "Unit Builder Probe", 0x86, y, -1);
    y += lineHeight;
    DrawString(surface, "==================", 0x86, y, -1);
    y += lineHeight;
    sprintf(buf, "uid: %03d '%s'\n", unit->id, (char*)((UnitType_004685a0 *)unit->def.get()));
    DrawString(surface, buf, 0x86, y, -1);
    y += lineHeight;
    char* mobile = (char*)(((UnitType_004685a0 *)unit->def.get())->mobile ? "MOBILE" : "BUILDING");
    char* remote;
    if (((PlayerInfo_004685a0 *)unit->player.get())->f_0 != 0 && (((PlayerInfo_004685a0 *)unit->player.get())->controller == 1 || ((PlayerInfo_004685a0 *)unit->player.get())->controller == 2))
        remote = "LOCAL";
    else
        remote = "REMOTE";
    sprintf(buf, "playerno: %d '%s' %s - %s\n", ((PlayerInfo_004685a0 *)unit->player.get())->player, ((PlayerInfo_004685a0 *)unit->player.get())->name, remote, mobile);
    DrawString(surface, buf, 0x86, y, -1);
    y += lineHeight;
    sprintf(buf, "controller: %d\n\n", ((PlayerInfo_004685a0 *)unit->player.get())->controller);
    DrawString(surface, buf, 0x86, y, -1);
    y += lineHeight;
    if (((PlayerInfo_004685a0 *)unit->player.get())->f_0 != 0 && (((PlayerInfo_004685a0 *)unit->player.get())->controller == 1 || ((PlayerInfo_004685a0 *)unit->player.get())->controller == 2)) {
        sprintf(buf, "Units I can build, and the probabilities:\n",
                ((unit->f_1f & 0x10) ? 'X' : '-'),
                ((unit->f_3b & 0x10) ? 'X' : '-'),
                ((unit->f_57 & 0x10) ? 'X' : '-'));
        DrawString(surface, buf, 0x86, y, -1);
        int i = 0;
        y += lineHeight;
        if (((UnitType_004685a0 *)unit->def.get())->count > 0) {
            do {
                unsigned short id = ((UnitType_004685a0 *)unit->def.get())->ids[i];
                int prob = GetBuildRating(unit->playerIndex, id);
                Def_004685a0* def = &((Def_004685a0 *)g_game->defs.get())[id];
                Rect bar;
                bar.left = 0x88;
                bar.top = y + 1;
                bar.right = 0xa2;
                bar.bottom = bar.top + lineHeight - 6;
                Bar_004685a0(surface, &bar, prob);
                char* name = def->name;
                sprintf(buf, "       %3d %% - '%s'\n", prob, name);
                DrawString(surface, buf, 0x86, y, -1);
                y += lineHeight;
                i++;
            } while (i < ((UnitType_004685a0 *)unit->def.get())->count);
        }
    }
    g_probePanelBottom = y;
    return 1;
}

// FUNCTION: 0x4689c0
void __stdcall DrawStatusPanel(Surface* win)
{
    char buf[0x100];
    char num[0x34];
    int v = g_game->panel;
    if (g_statusPanelNextTick < (int)GetMilliseconds()) {
        g_statusPanelNextTick = GetMilliseconds() + 15;
        if (!IsKeyDown(0x20) || (g_game->team_index != -1 && ((unsigned char*)((Team_004689c0 *)g_game->layer)->data)[g_game->team_index * 347] == 3)) {
            if (v < 0) {
                if (v == -31)
                    PlaySoundByName("Panel", 0);
                int q = (0 - v) / 3;
                if (q <= 1)
                    q = 1;
                v += q;
                if (v == 0)
                    PlaySoundByName("Options", 0);
                g_game->panel = v;
            }
        } else {
            if (v > -31) {
                if (v == 0)
                    PlaySoundByName("Panel", 0);
                int q = (v + 31) / 3;
                if (q <= 1)
                    q = 1;
                v -= q;
                if (v == -31)
                    PlaySoundByName("Options", 0);
            }
            g_game->panel = v;
        }
    }
    if (v == 0)
        return;
    g_game->field_52d = g_game->field_525;
    Rect bounds;
    win->GetClipRect(&bounds);
    int left = bounds.left;
    int bottom = bounds.bottom + v;
    DrawFrame(win, g_game->sprite, left, bottom);
    unsigned int tick = (unsigned int)g_game->ticks;
    int hours = tick / 108000;
    int rest = tick - hours * 108000;
    int minutes = rest / 1800;
    int seconds = (rest - minutes * 1800) / 30;
    sprintf(buf, "%s : %02d:%02d:%02d", Translate("Game Time"), hours, minutes, seconds);
    DrawTextClipped(win, buf, left + 0x19, bottom + 0xa, -1, 0);
    int team = g_game->localPlayer;
    sprintf(buf, "%s : %d  (Max %d)", Translate("Total Units"),
            g_game->players[team].unitCount, g_game->maxUnitsSetting);
    DrawTextClipped(win, buf, left + 0xbe, bottom + 0xa, -1, 0);
    if ((unsigned short)g_game->effectiveGameSpeed == 10)
        sprintf(num, Translate("Normal"));
    else
        sprintf(num, "%+d", (int)(unsigned short)g_game->effectiveGameSpeed - 10);
    sprintf(buf, "%s %s", Translate("Game Speed"), num);
    if ((unsigned short)g_game->effectiveGameSpeed != g_game->gameSpeed)
        sprintf(buf + strlen(buf), " (%+d)", (int)g_game->gameSpeed - 10);
    DrawTextClipped(win, buf, left + 0x17c, bottom + 0xa, -1, 0);
    g_game->field_52d = g_game->field_521;
}

#pragma pack(push, 1)
struct Resources {
    char owner;
    float metal;
    float metalIncome;
    float metalUse;
    float energy;
    float energyIncome;
    float energyUse;
    float maxMetal;
    float maxEnergy;
};
#pragma pack(pop)

static inline float Approach(float fcur, float ftarget)
{
    int cur = (int)fcur;
    int d = (int)ftarget - cur;
    if (d < 0) {
        d /= 8;
        if (d == 0)
            d = -1;
    } else if (d > 0) {
        d /= 8;
        if (d == 0)
            d = 1;
    } else
        d = 0;
    return (float)(d + cur);
}

static void DrawResourcePanel(Surface* ctx, Player* pl, Resources* res)
{
    Rect bar, box;
    char text[32];
    int side = pl->info->side;
    auto* sideDef = &g_game->sideDefs[side];
    SetFont((int)sideDef->font);
    GetFontHeight();
    unsigned char* pal = &g_game->colors[0];
    SetTextColors(pal[0xf], GetTextKeyColor());
    int bx = 0x81;
    do {
        unsigned short* gaf = (unsigned short*)GetGafFrame(g_game->sidePanelRows[side + (bx > 0x81) * 5], 0);
        BlitGafFrameAtOffset(ctx, (short*)gaf, bx, 0);
        bx += *gaf;
    } while (bx < g_game->width_37e1f);
    BlitSideLogoToRect(ctx, pl, &sideDef->logo, 0);
    Rect* r = &sideDef->energyBar;
    bar = *r;
    if (pl->energyCapacity > 0.0f) {
        bar.right = (int)((double)(bar.right - bar.left) * res->metal / pl->energyCapacity + bar.left);
        FillRectangle(ctx, &bar, sideDef->energyColor);
        if (pl->shareEnergy > 0.0f && pl->energy > pl->shareEnergy) {
            box = *r;
            box.left = (int)((double)(box.right - box.left) * pl->shareEnergy / pl->energyCapacity + box.left);
            box.right = box.left + 2;
            FillRectangle(ctx, &box, pal[0xc]);
        }
    }
    sprintf(text, "%d", (int)res->metal);
    DrawString(ctx, text, sideDef->energyNum.left, sideDef->energyNum.top, -1);
    DrawString(ctx, "0", sideDef->energy0.left, sideDef->energy0.top, -1);
    sprintf(text, "%d", (int)pl->energyCapacity);
    int w = GetTextWidth((int)sideDef->font, (int)text);
    DrawString(ctx, text, sideDef->energyMax.left - w, sideDef->energyMax.top, -1);
    if (res->metalIncome > 99999.0f)
        sprintf(text, "%dK", (int)res->metalIncome / 1000);
    else
        sprintf(text, "%d", (int)res->metalIncome);
    SetTextColors(pal[0xa], GetTextKeyColor());
    DrawString(ctx, text, sideDef->energyProduced.left, sideDef->energyProduced.top, -1);
    if (res->metalUse < -99999.0f)
        sprintf(text, "%dK", (int)res->metalUse / 1000);
    else
        sprintf(text, "%d", abs((int)res->metalUse));
    SetTextColors(pal[0xc], GetTextKeyColor());
    DrawString(ctx, text, sideDef->energyConsumed.left, sideDef->energyConsumed.top, -1);
    r = &sideDef->metalBar;
    bar = *r;
    if (pl->metalCapacity > 0.0f) {
        bar.right = (int)((double)(bar.right - bar.left) * res->energy / pl->metalCapacity + bar.left);
        FillRectangle(ctx, &bar, sideDef->metalColor);
        if (pl->shareMetal > 0.0f && pl->metal > pl->shareMetal) {
            box = *r;
            box.left = (int)((double)(box.right - box.left) * pl->shareMetal / pl->metalCapacity + box.left);
            box.right = box.left + 2;
            FillRectangle(ctx, &box, pal[0xc]);
        }
    }
    SetTextColors(pal[0xf], GetTextKeyColor());
    sprintf(text, "%d", (int)res->energy);
    DrawString(ctx, text, sideDef->metalNum.left, sideDef->metalNum.top, -1);
    DrawString(ctx, "0", sideDef->metal0.left, sideDef->metal0.top, -1);
    sprintf(text, "%d", (int)pl->metalCapacity);
    w = GetTextWidth((int)sideDef->font, (int)text);
    DrawString(ctx, text, sideDef->metalMax.left - w, sideDef->metalMax.top, -1);
    sprintf(text, "%.1f", res->energyIncome);
    SetTextColors(pal[0xa], GetTextKeyColor());
    DrawString(ctx, text, sideDef->metalProduced.left, sideDef->metalProduced.top, -1);
    sprintf(text, "%.1f", fabs(res->energyUse));
    SetTextColors(pal[0xc], GetTextKeyColor());
    DrawString(ctx, text, sideDef->metalConsumed.left, sideDef->metalConsumed.top, -1);
}

static inline int ShowSelectBox(int drawObjects)
{
    if (drawObjects == 0)
        return 0;
    if (g_game->inputFlags & 8)
        return 1;
    if ((char)g_game->orderMode == '\x0e')
        return PointInRect((int)&g_game->rect, g_game->cursorScreenX, g_game->cursorScreenY) != 0;
    return 0;
}

// FUNCTION: 0x468cf0
void __stdcall DrawBattleFrame(int param_1, int param_2)
{
    char debugText[80];
    int y2;
    Surface ctx;
    char gameTime[256];
    int y;
    unsigned char idx;
    int cy;
    Resources res;
    MapGrid* mv;
    unsigned char* colors;
    int cx;
    int x;
    Player* player;
    int i, k;

    cx = (g_game->width_37e1f + 0x80) / 2;
    cy = g_game->screenHeight / 2;
    SetOffscreenSurface(g_game->screen);
    ctx = **(Surface**)&g_game->screen;
    colors = &g_game->colors[0];
    HideSoftwareCursor();
    ctx.SetClipRect(g_game->rect);
    g_game->timers.AccumulateProfileTime(8);
    DrawMapTiles((int)&ctx);
    DrawMapDebugOverlay((int)&ctx);
    x = g_game->xWhole - g_game->scrollX + 0x80;
    y = g_game->zWhole - (g_game->yWhole >> 1) - g_game->scrollY + 0x20;
    if ((char)g_game->debugMode == '\x02') {
        DrawLine((int)&ctx, x - 2, y, x + 2, y, colors[0xf]);
        DrawLine((int)&ctx, x, y - 2, x, y + 2, colors[0xf]);
    }
    ResetClipRect((int)&ctx);

    {
        Player* pl = &g_game->players_1b63[g_game->playerIndex];
        res = (*(Resources*)&g_game->a);
        res.owner = pl->index;
        res.metal = Approach(res.metal, pl->energy);
        res.energy = Approach(res.energy, pl->metal);
        res.maxMetal = pl->energyCapacity;
        res.maxEnergy = pl->metalCapacity;
        if (res.metal > res.maxMetal)
            res.metal = res.maxMetal;
        if (res.energy > res.maxEnergy)
            res.energy = res.maxEnergy;
        static unsigned int s_incomesShownAt = ~0u;
        if (pl->displayTimer != s_incomesShownAt) {
            s_incomesShownAt = pl->displayTimer;
            res.metalIncome = GetEnergyIncome((int)pl);
            res.metalUse = GetEnergyUsage((int)pl);
            res.energyIncome = GetMetalIncome((int)pl);
            res.energyUse = GetMetalUsage((int)pl);
        }
        if (memcmp((char*)&(*(Resources*)&g_game->a), &res, sizeof(res)) != 0) {
            (*(Resources*)&g_game->a) = res;
            DrawResourcePanel(&ctx, pl, &res);
        }
    }
    DrawUnitInfoPanel(&ctx);
    DrawRadar((int)&ctx);
    ctx.SetClipRect(g_game->rect);
    g_game->timers.AccumulateProfileTime(3);

    idx = g_game->playerIndex;
    mv = &g_game->sortUnits;
    player = &g_game->players_1b63[idx];
    SetFont((int)g_game->sideDefs[player->info->side].font);
    SetTextColors(g_game->colors[15], GetTextKeyColor());
    {
        int vx = g_game->scrollX / 16, vy = g_game->scrollY / 16, h, w, x0, y0, skip;
        i = 0;
        while (i < mv->rows) {
            mv->cursor[i] = mv->buf + i * mv->stride;
            mv->count[i] = 0;
            i++;
        }
        y0 = vy - 16;
        h = mv->rows;
        if (y0 < 0) {
            skip = -y0;
            h -= skip;
            y0 = 0;
        } else
            skip = 0;
        if (h + y0 > mv->height - 1)
            h = mv->height - y0 - 1;
        x0 = vx - 10;
        w = mv->stride;
        if (x0 < 0) {
            w += x0;
            x0 = 0;
        }
        if (w + x0 > mv->width - 1)
            w = mv->width - x0 - 1;
        unsigned short* pIdx = g_game->list_1435f;
        k = 0;
        while (k < g_game->count_14367) {
            Unit* unit = (Unit*)((int)g_game->units.get() + *pIdx * 0x118);
            int row = ((int)unit->zWhole - g_game->scrollY) / 16 + 0x10;
            if (row >= 0 && row < mv->rows) {
                Unit*** pCur = (Unit***)&mv->cursor[row];
                mv->count[row]++;
                if (*pCur != 0) {
                    **pCur = unit;
                    (*pCur)++;
                }
            }
            k++;
            pIdx++;
        }
        DrawParticleList((int)&ctx, 0);
        DrawParticleList((int)&ctx, 1);
        DrawParticleList((int)&ctx, 2);
        y = y0;
        i = 0;
        while (i < h) {
            x = x0;
            Cell* tile = (Cell*)mv->tiles.get() + (mv->width * y + x);
            int c = 0;
            while (c < w) {
                tile->flags &= 0xfb;
                if (tile->feature < 0xfffb) {
                    Feature* feat = g_game->features + tile->feature;
                    if (feat->height < 10) {
                        if ((feat->flagsHigh & 8) && ((tile->flags >> 3 & 0xf) != idx)) {
                            if (IsFootprintVisible((int)player, x, y, feat->footprintX, feat->footprintZ, tile->height))
                                BlitFeatureGaf(&ctx, tile, x, y);
                        } else
                            BlitFeatureGaf(&ctx, tile, x, y);
                    } else
                        tile->flags |= 4;
                }
                c++;
                x++;
                tile++;
            }
            i++;
            y++;
        }
        DrawParticleList((int)&ctx, 3);
        DrawParticleList((int)&ctx, 4);
        for (i = 0; i < h; i++) {
            int row = i + skip;
            y = y0 + i;
            Unit** pUnit = (Unit**)mv->buf + row * mv->stride;
            for (k = 0; k < mv->count[row]; k++, pUnit++) {

                Unit* u = *pUnit;
                if (u->mode == 1 && param_1 != 0) {
                    if (u->selected)
                        DrawSelectionBox((Vec3*)&ctx, u);
                    if (u->script != 0)
                        DrawUnit((int)&ctx, (int)u);
                }
            }
            Cell* tile = (Cell*)mv->tiles.get() + (y * mv->width + x0);
            for (int c = 0; c < w; c++, tile++) {
                x = x0 + c;

                if (tile->flags & 4) {
                    Feature* feat = g_game->features + tile->feature;
                    if ((feat->flagsHigh & 8) && ((tile->flags >> 3 & 0xf) != idx)) {
                        if (IsFootprintVisible((int)player, x, y, (int)feat->footprintX, feat->footprintZ, tile->height))
                            BlitFeatureGaf(&ctx, tile, x, y);
                    } else
                        BlitFeatureGaf(&ctx, tile, x, y);
                }
            }
        }
    }
    DrawParticleList((int)&ctx, 5);
    if (param_1 != 0) {
        DrawParticleList((int)&ctx, 6);
        DrawProjectiles((int)&ctx);
        DrawExplosions((int)&ctx);
        DrawParticleList((int)&ctx, 7);
        for (i = 0; i < mv->rows; i++) {
            Unit** pUnit = (Unit**)mv->buf + mv->stride * i;
            k = 0;
            while (k < mv->count[i]) {
                Unit* u = *pUnit;
                if (u->mode != 1) {
                    if (u->selected)
                        DrawSelectionBox((Vec3*)&ctx, u);
                    if (u->script != 0)
                        DrawUnit((int)&ctx, (int)u);
                }
                k++;
                pUnit++;
            }
        }
    }
    DrawParticleList((int)&ctx, 8);
    if (IsKeyDown(0xf9))
        DrawSelectedUnitOrderOverlays((int)&ctx, (int)&(*(void**)&g_game->value_142f3));

    if (param_1 != 0) {
        unsigned short* pIdx = g_game->list_1435f;
        for (k = 0; k < g_game->count_14367; k++, pIdx++) {
            Unit* unit = (Unit*)((int)g_game->units.get() + *pIdx * 0x118);
            if ((g_game->visualFlagsByte & 1) || unit->group != 0) {
                char str[2];
                str[1] = 0;
                x = unit->xWhole - g_game->scrollX + 0x80;
                y = unit->zWhole - g_game->scrollY - (unit->yWhole >> 1) + 0x20;
                if (g_game->visualFlagsByte & 1) {
                    if ((char)unit->player->index == (char)idx)
                        DrawHitPointBar(&ctx, unit, x, y + 10);
                    if ((char)unit->player->index == (char)idx && unit->group != 0) {
                        str[0] = (char)unit->group + '0';
                        DrawString(&ctx, str, x, y + 0xe, -1);
                    }
                }
            }
        }
        DrawParticleList((int)&ctx, 9);
    }
    g_game->timers.AccumulateProfileTime(4);
    if ((g_game->endGameFlags & 1) && (g_game->endGameFlags & 2) && param_1 != 0)
        DrawSelectedGoal((int)&ctx, FindNextSelectedUnit(0, 0));
    if (param_1 != 0)
        DrawFogOfWar((int)&ctx);
    g_game->timers.AccumulateProfileTime(5);

    if (ShowSelectBox(param_1)) {
        Rect box;
        int x1 = g_game->boxStartX - g_game->scrollX + 0x80;
        int y1 = g_game->boxStartZ - (g_game->boxStartHeight >> 1) - g_game->scrollY + 0x20;
        int x2 = g_game->boxEndX - g_game->scrollX + 0x80;
        int y2 = g_game->boxEndZ - (g_game->boxEndHeight >> 1) - g_game->scrollY + 0x20;
        int ci;
        if ((char)g_game->orderMode == '\x0e')
            ci = ((g_game->inputFlags & 0x40) ? 6 : 0) + 4;
        else
            ci = 0xf;
        int c = colors[ci];
        int t;
        if (x2 < x1) {
            t = x1;
            x1 = x2;
            x2 = t;
        }
        if (y2 < y1) {
            t = y1;
            y1 = y2;
            y2 = t;
        }
        box.left = x1;
        box.top = y1;
        box.right = x2;
        box.bottom = y2;
        DrawRectangle(&ctx, &box, c);
        box.left++;
        box.top++;
        box.right--;
        box.bottom--;
        if ((char)g_game->orderMode == '\x0e')
            DrawRectangle(&ctx, &box, c);
        else
            DrawRectangle(&ctx, &box, *colors);
    }
    if (g_game->net->GetGameType() == 3 ||
        g_game->net->GetGameType() == 2) {
        ResetClipRect((int)&ctx);
        DrawScorePanel((int)&ctx);
        ctx.SetClipRect(g_game->rect);
    }
    DrawStatusPanel(&ctx);
    if (g_game->showBps != 0)
        DrawNetworkStats(&ctx);
    if (param_1 != 0)
        DrawMessages((int)&ctx);
    if ((g_game->flagsByte_3923b & 2) && param_1 != 0) {
        SetTextColors(colors[0xf], GetTextKeyColor());
        SetFont(g_game->fontComix);
        int ty = GetFontHeight() * 3 - 10;
        sprintf(debugText, "FRATE: %d\n", GetFrameRate());
        DrawString(&ctx, debugText, 0x83, ty, -1);
        DrawString(&ctx, "[Release]", 0xbc, ty, -1);
        sprintf(debugText, "MODE %s INFO %s", g_game->endGameFlags_3923b.b1 ? "DEBUG" : "NORMAL",
                g_game->endGameFlags_3923b.b0 ? "ON" : "OFF");
        DrawString(&ctx, debugText, 0x1ee, ty, -1);
        ty += GetFontHeight();
        if (g_game->netGameFlags & 1) {
            FormatNetStats((int)debugText);
            DrawString(&ctx, debugText, 0xbc, ty, -1);
        }
    }
    if (g_game->pauseFlags & 1)
        DrawFrame(&ctx, (short*)GetGafFrame((unsigned short*)g_game->igpaused, 0), cx, cy);
    if ((g_game->players_1b63[g_game->localPlayer].info->flags_9b & 0x40) == 0) {
        if (g_game->bits.flag5)
            DrawFrame(&ctx, (short*)GetGafFrame((unsigned short*)g_game->image_14813, 0), cx, cy);
        if (g_game->bits.flag6)
            DrawFrame(&ctx, (short*)GetGafFrame((unsigned short*)g_game->image_14817, 0), cx, cy);
    }
    if (g_game->uiOptionFlags_37f2f.clock) {
        unsigned int ticks = (unsigned int)g_game->ticks;
        unsigned int hours = ticks / 108000;
        int rest = ticks - hours * 108000;
        int minutes = rest / 1800;
        int seconds = (rest - minutes * 1800) / 30;
        sprintf(gameTime, "%s : %02d:%02d:%02d", (char*)Translate((int)"Game Time"), hours, minutes, seconds);
        SetTextColors(colors[0xf], GetTextKeyColor());
        DrawString(&ctx, gameTime, 0x82, -0x22 - GetFontHeight() + GetScreenHeight(), -1);
    }
    if (g_game->bits_38a51_3.lagging)
        DrawFrame(&ctx, (short*)GetGafFrame((unsigned short*)g_game->cursorHourglass, 0), g_game->width_37e1f - 0x10, g_game->screenHeight - 0x50);
    ResetClipRect((int)&ctx);
    BlitMenuLayers((int)&g_game->gui, (int)&ctx, (int)&g_game->rect);
    if (g_game->profileBarsEnabled != 0 && param_1 != 0) {
        DrawProfileBarLine((int)&ctx, "Network", 0);
        DrawProfileBarLine((int)&ctx, "Units", 1);
        DrawProfileBarLine((int)&ctx, "Logic", 2);
        DrawProfileBarLine((int)&ctx, "Render Static", 3);
        DrawProfileBarLine((int)&ctx, "Render Stuff", 4);
        DrawProfileBarLine((int)&ctx, "Render Fog", 5);
        DrawProfileBarLine((int)&ctx, "SFX", 6);
        DrawProfileBarLine((int)&ctx, "Weapon", 7);
        DrawProfileBarLine((int)&ctx, "Misc", 8);
    }
    DrawOptionsScrollBar((int)&ctx);
    ShowSoftwareCursor();
    if (param_1 != 0 && param_2 != 0)
        FlipScreen();
    g_game->timers.AccumulateProfileTime(3);
}

// FUNCTION: 0x46a400
void FrameTimers::AccumulateProfileTime(int param_1)
{
    unsigned int result = GetMilliseconds();
    int edi = *(int*)((char*)this);
    int edx = result - edi;
    int field_val = *(int*)((char*)this + param_1 * 4 + 0x2c);
    field_val = field_val + edx;
    *(int*)((char*)this + param_1 * 4 + 0x2c) = field_val;
    *(int*)((char*)this) = result;
}

// FUNCTION: 0x46a430
void __stdcall DrawHitPointBar(void* surface, Unit* unit, int x, int y)
{
    if (unit->health <= 0)
        return;
    unsigned char* colors = g_game->colors;
    Rect r;
    r.left = x - 17;
    r.top = y - 2;
    r.right = x + 17;
    r.bottom = y + 2;
    FillRectangle(surface, &r, colors[0]);
    int width;
    r.left++;
    r.top++;
    r.bottom--;
    width = unit->health * 32 / unit->def->maxHealth;
    r.right = r.left + width;
    if (unit->health > (int)(unit->def->maxHealth / 3) * 2)
        FillRectangle(surface, &r, colors[10]);
    else if (unit->health > (int)(unit->def->maxHealth / 3))
        FillRectangle(surface, &r, colors[14]);
    else
        FillRectangle(surface, &r, colors[12]);
}

// FUNCTION: 0x46a530
void __stdcall DrawSelectionBox(Vec3* view, Unit* unit)
{
    Flags_0046a530* f = (Flags_0046a530*)((char*)g_game + 0x37f2f);
    if (f->flag) {
        Vec3 lo;
        Vec3 hi;
        GetObjectBounds(((Object_004cb650 **)g_game->types.get())[unit->unitDefIndex], &lo, &hi, 0);
        Vec3 corners[4];
        corners[0].x = lo.x;
        corners[0].y = lo.y;
        corners[0].z = lo.z;
        corners[1].x = hi.x;
        corners[1].y = lo.y;
        corners[1].z = lo.z;
        corners[2].x = hi.x;
        corners[2].y = lo.y;
        corners[2].z = hi.z;
        corners[3].x = lo.x;
        corners[3].y = lo.y;
        corners[3].z = hi.z;
        Vec3 pos;
        pos.x = unit->pos.x - (g_game->scrollX << 16);
        pos.y = unit->pos.y;
        pos.z = unit->pos.z - (g_game->scrollY << 16);
        DrawRotatedQuadOutline(view, &pos, corners, (short *)((char*)&unit->angles));
    }
}

static int DrawFlags()
{
    return g_game->visualFlagsByte;
}

#define DrawFlip(flip, dest, frame, x, y) \
    do { \
        if (flip) \
            DrawFrameBlended(dest, frame, x, y); \
        else \
            DrawFrame(dest, frame, x, y); \
    } while (0)

// FUNCTION: 0x46a610
void __stdcall BlitFeatureGaf(void* dest, Cell* cell, int ix, int iy)
{
    Feature* f = &g_game->features[cell->feature];
    int x = f->footprintX * 16 / 2 + (ix + 8) * 16 - g_game->scrollX;
    int s = cell->height;
    s += cell[1].height;
    s += cell[g_game->width].height;
    s += cell[g_game->width + 1].height;
    int shade = s >> 3;
    int y = f->footprintZ * 16 / 2 - shade + (iy + 2) * 16 - g_game->scrollY;
    if (cell->flags & 1) {
        FeatureSpot* spot = &g_game->spots[*(unsigned short*)&cell->offsetY];
        if (f->drawn) {
            if ((spot->spotFlags & 4) && (DrawFlags() & 0x10)) {
                short* frame = (short*)GetGafSequenceFrame(&spot->shadow);
                DrawFrame(dest, frame, x, y);
            }
            {
                short* frame = (short*)GetGafSequenceFrame(&spot->anim);
                DrawFrame(dest, frame, x, y);
            }
        } else {
            Unit* unit = g_game->unit;
            ObjectState* st = (ObjectState*)spot->state.get();
            unit->state = (SpotState*)st;
            st->unit = unit;
            unit->angles = spot->rot;
            unit->pos = spot->pos;
            DrawUnit(dest, unit);
        }
    } else {
        if (f->over) {
            if (f->shadowTable && (DrawFlags() & 0x10)) {
                short* frame = (short*)GetGafSequenceFrame(&f->shadowAnim);
                DrawFlip(f->flipShadow, dest, frame, x, y);
            }
            if (f->animTable) {
                short* frame = (short*)GetGafSequenceFrame(&f->anim);
                DrawFlip(f->flipAnim, dest, frame, x, y);
            }
        } else {
            if (f->shadowTable && (DrawFlags() & 0x10)) {
                short* frame = (short*)GetGafFrame(f->shadowTable, 0);
                DrawFlip(f->flipShadow, dest, frame, x, y);
            }
            if (f->animTable) {
                short* frame = (short*)GetGafFrame(f->animTable, 0);
                DrawFlip(f->flipAnim, dest, frame, x, y);
            }
        }
    }
}

static void DrawPanelBar(void* surface, Rect* bounds, int value, int maximum, int dy,
                             unsigned char* palette) {
    Rect rect = *bounds;
    rect.top = rect.top + dy;
    rect.bottom += dy;
    if (value < 0)
        value = 0;
    if (value > maximum)
        value = maximum;
    rect.right = rect.left + (bounds->right - bounds->left) * value / maximum;
    FillRectangle(surface, &rect, palette[10]);
    if (rect.right != bounds->right) {
        rect.left = rect.right + 1;
        rect.right = bounds->right;
        FillRectangle(surface, &rect, palette[4]);
    }
}

static float PositiveOrZero(float value) {
    float result = value > 0.0f ? value : 0.0f;
    return result;
}

// FUNCTION: 0x46a860
void __stdcall DrawUnitInfoPanel(void* surface) {
    int yOffset = g_game->screenHeight - g_game->baseHeight;
    Snapshot_0046a860 snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    char text[256];

    unsigned short flags = g_game->endGameFlags;
    if (flags & 1 && flags & 2) {
        int y = 0x81;
        unsigned char player = g_game->playerIndex;
        PlayerInfo* table = g_game->players[player].info;
        int idx = table->side;
        do {
            int dy = GetScreenHeight() - 0x20;
            unsigned short* icon = g_game->sidePanelBotSeq[idx];
            GafFrame* bmp = (GafFrame*)GetGafFrame(icon, 0);
            DrawFrame(surface, (void*)bmp, (short)bmp->xOffset + y,
                         (short)bmp->yOffset + dy);
            y += bmp->width;
        } while (y < g_game->width_37e1f);

        SetFont(g_game->fontComix);
        SetTextColors(0x53, GetTextKeyColor());
        int pfstate = g_game->screenHeight - GetFontHeight() - 1;
        sprintf(text, "PFSTATE %d, PFABLE %d\n", ((DisplayContextView *)g_game->displayContext)->flags & 1,
                ((DisplayContextView *)g_game->displayContext)->haveBackBuffer);
        DrawString(surface, text, 0x82, pfstate, -1);

        if (g_game->hoverUnitId != 0) {
            int idx = g_game->hoverUnitId & 0xffff;
            Unit* unit = g_game->units.get() + idx;
            sprintf(text, "MOVEORD: %d FIREORD: %d\n", (unit->flags >> 0x12) & 3,
                    (unit->flags >> 0x14) & 3);
            DrawString(surface, text, 0x108, pfstate, -1);
        }

        sprintf(text, "DELTATIME: %d\n", g_game->frames_38a3b);
        DrawString(surface, text, 0x190, pfstate, -1);

        sprintf(text, "GAMETIME: %d\n", g_game->ticks);
        DrawString(surface, text, 0x208, pfstate, -1);

        pfstate -= 0x10;
        sprintf(text, "X: %d  Y: %d\n", g_game->scrollX, g_game->scrollY);
        DrawString(surface, text, 0x82, pfstate, -1);

        sprintf(text, "UNITS %d\\%d\n", g_game->f14353, g_game->count_14367);
        DrawString(surface, text, 0x108, pfstate, -1);

        sprintf(text, "PACKETS: %d %d %d\n", g_game->players[1].messages,
                g_game->players[2].messages, g_game->players[3].messages);
        DrawString(surface, text, 0x190, pfstate, -1);

        int v = g_game->width;
        v *= (short)(unsigned short)g_game->y;
        v += (short)(unsigned short)g_game->x;
        unsigned char c = g_game->cells[v].height;
        sprintf(text, "XYH: %d %d %d\n", (short)(unsigned short)g_game->x,
                (short)(unsigned short)g_game->y, c);
        DrawString(surface, text, 0x208, pfstate, -1);
        return;
    }

    snapshot.button = g_game->selected;
    snapshot.selected = g_game->hoverUnitId;
    snapshot.feature = g_game->cellFeature;
    snapshot.height = g_game->selectionInfoCache.height;
    snapshot.width = g_game->selectionInfoCache.width;

    if (snapshot.selected != 0) {
        Unit* unit = g_game->units.get() + snapshot.selected;
        snapshot.health = (unsigned short)unit->health;
        snapshot.build = unit->killCount;
        snapshot.orderName = GetOrderName(unit);
        float f0 = unit->resourceSlot.energyUsePrev;
        float f1 = unit->resourceSlot.energyMakePrev;
        float f2 = unit->resourceSlot.metalUsePrev;
        float f3 = unit->resourceSlot.metalMakePrev;
        snapshot.values[0] = f0;
        snapshot.values[1] = f1;
        snapshot.values[2] = f2;
        snapshot.values[3] = f3;
        char* slot = (char*)unit + 0x1f;
        int* out = snapshot.weapons;
        for (int i = 0; i < 3; i++, out++, slot += 0x1c) {
            WeaponDef* weapon = *(WeaponDef**)(slot - 0xf);
            if (weapon->reloadTime > 0x1e) {
                if (*(unsigned char*)slot & 2)
                    *out = *(unsigned short*)(slot - 7);
                else
                    *out = -1;
            } else
                *out = -1;
        }

        snapshot.targetType = 0;
        if (unit->playerIndex == g_game->playerIndex) {
            Unit* result = (Unit*)GetOrderTarget(unit);
            if (result != 0) {
                snapshot.targetType = result->id;
                snapshot.targetHealth = (unsigned short)result->health;
            }
        }
    }

    char* saved = (char*)&g_game->selectionInfoCache;
    if (memcmp(saved, &snapshot, sizeof(snapshot)) == 0)
        return;
    memcpy(saved, &snapshot, sizeof(snapshot));

    char* palette = (char*)&g_game->colors[0];
    SetTextColors(0x53, GetTextKeyColor());

    int player = g_game->playerIndex;
    Player* playerInfo = &g_game->players[player];
    PlayerInfo* info = playerInfo->info;
    int side = info->side;
    auto* panel = &g_game->sideDefs[side];
    SetFont((int)panel->font);
    int y = 0x81;
    do {
        int dy = GetScreenHeight() - 0x20;
        PlayerInfo* loopInfo = playerInfo->info;
        int loopSide = loopInfo->side;
        unsigned short* icon = g_game->sidePanelBotSeq[loopSide];
        GafFrame* bmp = (GafFrame*)GetGafFrame(icon, 0);
        DrawFrame(surface, (void*)bmp, (short)bmp->xOffset + y,
                     (short)bmp->yOffset + dy);
        y += bmp->width;
    } while (y < g_game->width_37e1f);

    if (snapshot.button != -1) {
        strncpy(text, (char*)(snapshot.button * 0x15b + ((LayerView *)g_game->layer)->value + 2), 0x10);
        text[0x10] = 0;
        unsigned short type = FindUnitTypeId(text);
        if (type != 0) {
            UnitDef* entry = g_game->defs + type;
            if (_strcmpi(text, "CORBUILD") != 0) {
                sprintf(text, "%s  M:%d E:%d", entry->name, (int)entry->metalCost,
                        (int)entry->energyCost);
                DrawString(surface, text, panel->nameBlock.left,
                             panel->nameBlock.top + yOffset, -1);
                DrawString(surface, entry->description, panel->description.left,
                             panel->description.top + yOffset, -1);
                return;
            }
        }
    } else if (snapshot.selected != 0) {
        Unit* unit = g_game->units.get() + snapshot.selected;
        if (unit->unitDefIndex != 0) {
            char* playerMap = (char*)&g_game->players[g_game->playerIndex];
            if (IsUnitVisibleToPlayer(playerMap, unit)) {
                UnitDef* definition = unit->def;
                unsigned int unitFlags = definition->flags2;
                int flagsOk = ((unitFlags & 0x20000) | ((unitFlags >> 1) & 0x20000)) >> 0x11;
                int gameMode = g_game->net->GetGameType();
                if (gameMode == 3 && flagsOk)
                    strcpy(text, unit->player->name);
                else
                    strcpy(text, (char*)unit->def.get());
                int titleX = panel->unitName.left -
                             GetTextWidth((int)panel->font, (int)text) / 2;
                SetTextColors(0x53, GetTextKeyColor());
                DrawString(surface, text, titleX, yOffset + panel->unitName.top, -1);
                if (unit->player->index == g_game->playerIndex ||
                    !(unit->def->flags1 & 0x4000)) {
                    int maximum = unit->def->maxHealth;
                    DrawPanelBar(surface, &panel->damageBar, unit->health,
                                     maximum, yOffset, (unsigned char*)palette);
                }
                BlitSideLogoToRect(surface, unit->player, &panel->logo2, yOffset);
                if (unit->playerIndex == g_game->playerIndex ||
                    (g_game->flagsByte_3923b & 2)) {
                    char amount[100];
                    char killsText[100];
                    SetTextColors((unsigned char)palette[10], GetTextKeyColor());
                    sprintf(amount, "+%.1f", PositiveOrZero(snapshot.values[3]));
                    DrawString(surface, amount, panel->unitMetalMake.left,
                                 panel->unitMetalMake.top + yOffset, -1);
                    sprintf(amount, "+%.0f", PositiveOrZero(snapshot.values[1]));
                    DrawString(surface, amount, panel->unitEnergyMake.left,
                                 panel->unitEnergyMake.top + yOffset, -1);
                    SetTextColors((unsigned char)palette[12], GetTextKeyColor());
                    sprintf(amount, "-%.1f", PositiveOrZero(snapshot.values[2]));
                    DrawString(surface, amount, panel->unitMetalUse.left,
                                 panel->unitMetalUse.top + yOffset, -1);
                    sprintf(amount, "-%.0f", PositiveOrZero(snapshot.values[0]));
                    DrawString(surface, amount, panel->unitEnergyUse.left,
                                 panel->unitEnergyUse.top + yOffset, -1);
                    if ((unit->flags & 0x80000000) && unit->killCount) {
                        int killsY = panel->damageBar.bottom + yOffset + 2, killsX = panel->damageBar.left;
                        char* plural = Translate("kills");
                        char* singular = Translate("kill");
                        if (unit->killCount > 4)
                            sprintf(killsText, "%d %s - %s", unit->killCount,
                                    unit->killCount == 1 ? singular : plural,
                                    Translate("Veteran"));
                        else
                            sprintf(killsText, "%d %s", unit->killCount,
                                    unit->killCount == 1 ? singular : plural);
                        SetTextColors(g_game->colors[15], GetTextKeyColor());
                        DrawString(surface, killsText, killsX, killsY, -1);
                    }
                    if (snapshot.orderName) {
                        strcpy(amount, Translate((char*)snapshot.orderName));
                        int orderX = panel->missionText.left -
                                     GetTextWidth((int)panel->font, (int)amount) / 2;
                        SetTextColors(0x53, GetTextKeyColor());
                        DrawString(surface, amount, orderX, panel->missionText.top + yOffset, -1);
                    }
                }
                int progress = GetBuildWeaponPercent(unit);
                if (progress) {
                    if (unit->player->index != g_game->playerIndex)
                        return;
                    char* weaponText = Translate("Weapon");
                    int progressX = panel->unitName2.left -
                                    GetTextWidth((int)panel->font, (int)weaponText) / 2;
                    SetTextColors(0x53, GetTextKeyColor());
                    DrawString(surface, weaponText, progressX, panel->unitName2.top + yOffset, -1);
                    DrawPanelBar(surface, &panel->damageBar2, progress, 100, yOffset,
                                     (unsigned char*)palette);
                    return;
                }
                if (snapshot.targetType) {
                    Unit* target = g_game->units.get() + snapshot.targetType;
                    if (!target->unitDefIndex || !IsUnitVisibleToPlayer(playerMap, target))
                        return;
                    int targetX = panel->unitName2.left -
                                  GetTextWidth((int)panel->font, (int)(char*)target->def.get()) / 2;
                    SetTextColors(0x53, GetTextKeyColor());
                    DrawString(surface, (char*)target->def.get(), targetX,
                                 panel->unitName2.top + yOffset, -1);
                    if (target->player->index == g_game->playerIndex ||
                        !(target->def->flags1 & 0x4000)) {
                        int maximum = target->def->maxHealth;
                        DrawPanelBar(surface, &panel->damageBar2, target->health,
                                         maximum, yOffset, (unsigned char*)palette);
                    }
                }
            } else {
                char* prefix = (char*)(((unsigned char)(unit->flags >> 9) & 1) != 0 ? "S: " : "R: ");
                char* s = Translate("Unidentified object");
                sprintf(text, "%s%s", prefix, s);
                int w = GetTextWidth((int)panel->font, (int)text);
                int x = panel->unitName.left - w / 2;
                SetTextColors(0x53, GetTextKeyColor());
                DrawString(surface, text, x, panel->unitName.top + yOffset, -1);
                return;
            }
        }
    } else if (snapshot.feature != 0xffff) {
        Feature* feature = g_game->features + snapshot.feature;
        if ((feature->flagsHigh & 4) == 0 || (g_game->flagsByte_3923b & 2) != 0) {
            char mText[16];
            if (feature->metal != 0.0f)
                sprintf(mText, " M:%d", (int)feature->metal);
            else
                mText[0] = 0;
            char eText[16];
            if (feature->energy != 0.0f)
                sprintf(eText, " E:%d", (int)feature->energy);
            else
                eText[0] = 0;
            char* name = g_game->endGameFlags_3923b.b1 ? feature->name : feature->description;
            if ((feature->flagsHigh & 2) == 0)
                sprintf(text, "%s %s%s", Translate(name), mText, eText);
            else
                strcpy(text, Translate(name));
            SetTextColors(0x53, GetTextKeyColor());
            DrawString(surface, text, panel->nameBlock.left, panel->nameBlock.top + yOffset, -1);
        }
    }
}

// FUNCTION: 0x46b900
void __stdcall DrawProfileBarLine(int surface, const char* text, int index)
{
    Rect rect;
    int x = g_game->width_37e1f - 0x5a;
    int c = GetFontHeight();

    rect.left = x - 0xc8;
    rect.right = 0x27f;
    rect.top = 0x26;
    rect.bottom = c * 9 + 0x29;
    DrawRectangle(surface, &rect, 0xff);

    DrawString(surface, text, x + 5, c * index + 0x28, -1);

    int w = g_game->values[index] * 100 / g_game->total * 2;
    rect.left = x - w;
    rect.right = x;
    rect.bottom = c * index + 0x28;
    rect.top = rect.bottom;
    rect.bottom = rect.bottom + c;
    FillRectangle(surface, &rect, index + 1);
}

// FUNCTION: 0x46b9d0
void __stdcall DrawMapTileSelectionOutline(void* surface, short* pos, Size_0046b9d0 size, int kind)
{
    unsigned char* colors = (unsigned char*)g_game + 0xdcb;
    int h = GetCellHeight(pos);
    int y1 = (pos[1] << 4) - g_game->scrollY;
    int x1 = ((pos[0] + 8) << 4) - g_game->scrollX;
    Rect rect;
    rect.left = x1;
    rect.top = y1 - (h >> 1) + 0x20;
    rect.right = x1 + (size.w << 4);
    rect.bottom = rect.top + (size.h << 4);
    if (kind == 4) {
        rect.left++;
        rect.top++;
        rect.right--;
        rect.bottom--;
    }
    DrawRectangle(surface, &rect, colors[kind]);
}

// FUNCTION: 0x46ba80
void __stdcall DrawClosedPolygon(void* surface, Point* points, int count, int color)
{
    Point* p = points;
    for (int i = count - 1; i > 0; i--) {
        DrawLine(surface, p[0].x, p[0].y, p[1].x, p[1].y, color);
        p++;
    }
    DrawLine(surface, points->x, points->y, p->x, p->y, color);
}


// FUNCTION: 0x46bae0
void __stdcall DrawModel3doProjected(void* surface, Vec3* offset,
                            Object_0046bae0* obj, short* angles)
{
    Vec3* v = obj->verts;
    Vec3* scratch = (Vec3 *)g_game->xform;
    Point* points = (Point *)g_game->projected;
    int i = 0;
    for (; i < obj->count; i++, v++, scratch++, points++) {
        RotateByAngles(v, scratch, angles);
        int y = (short)((scratch->y + offset->y) >> 16);
        int z = (short)((offset->z - scratch->z) >> 16);
        int x = (short)((scratch->x + offset->x) >> 16);
        points->x = x + 0x80;
        points->y = z - (y >> 1) + 0x20;
    }

    int j;
    Prim_0046bae0* e = obj->prims;
    if (obj->firstFace != -1) {
        e++;
        j = 1;
    } else {
        j = 0;
    }
    for (; j < obj->faceCount; j++, e++) {
        unsigned short* idx = e->indices;
        Point* dst = (Point *)g_game->assemPoints;
        for (int k = 0; k < e->count; k++, idx++, dst++) {
            *dst = ((Point *)g_game->projected)[*idx];
        }
        if (!e->flag0) {
            if (e->count == 4) {
                void* tex;
                if (e->texIndexed)
                    tex = (void*)GetGafSequenceFrame((short*)((char*)e + 0x10));
                else
                    tex = (void*)e->pic;
                DrawFrameQuad(surface, tex, (Point *)g_game->assemPoints, 0);
            }
        } else {
            FillPolygon(surface, (Point *)g_game->assemPoints, e->count, e->colorIndex);
        }
    }
}

// FUNCTION: 0x46bc60
void __stdcall ReportGameEventCallback(int param_1)
{
    ReportGameEvent(param_1);
}

// FUNCTION: 0x46bc70
int __stdcall ShowGameMessage(char* text, int mode)
{
    int result = 1;
    if (mode == 0) {
        AddMessage(text, 0x10, 0, 10);
        g_game->bits_2bee_1.dirty = 1;
        return 1;
    }
    if (mode == 1) {
        int t = GetScreenWidth();
        result = OpenMessageBox((Gui *)(*(char (*)[4])&g_game->gui), text, (int)(t * 0.5), 1, 1);
    }
    return result;
}
