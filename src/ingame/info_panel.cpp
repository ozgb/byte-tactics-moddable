#include "win_types.h"
#include "../../interfaces/ref.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>

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

struct Owner_00467440 {
    void* active;
    char unknown_4[0x27 - 0x4];
    Ref<PlayerInfo> data;
    char unknown_2b[0x73 - 0x2b];
    char type;
    char unknown_74[0x108 - 0x74];
    unsigned char allied[1];
};

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

struct MapSize_00467440 {
    unsigned int width;
    unsigned int height;

    int Contains(unsigned int tx, unsigned int ty)
    {
        return tx < width && ty < height;
    }
};

struct ByteMap_00467440 {
    Ref<unsigned char> data;
    MapSize_00467440 size;

    unsigned char Get(int x, int y) { return data[size.width * y + x]; }
};

struct PlayerInfo_00467440 {
    void* active;
    char unknown_4[0x27 - 0x4];
    Ref<PlayerInfo> data;
    char unknown_2b[0x67 - 0x2b];
    Ref<Unit> unitsBegin;
    Ref<Unit> unitsEnd;
    char unknown_6f[0x7c - 0x6f];
    ByteMap_00467440 explored;
    char unknown_88[0x144 - 0x88];
    unsigned short unitCount;
    unsigned char index;
    char unknown_147[0x14b - 0x147];
};

struct Team_004689c0 {
    char unknown_0[4];
    unsigned char* data;
    char unknown_8[347 - 8];
};

#include "../graphics/handle.h"

struct Feature {
    char name[0x94];
    Point16 footprint;
    char unknown_98[0xac - 0x98];
    unsigned short* animTable;
    unsigned short* shadowTable;
    char unknown_b4[0xcc - 0xb4];
    Handle anim;
    Handle shadowAnim;
    char unknown_e4[0xfe - 0xe4];
    unsigned short drawn : 1;
    unsigned short over : 1;
    unsigned short flipAnim : 1;
    unsigned short flipShadow : 1;
    unsigned short unknown_bits : 4;
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

struct MissionName_00467e50 {
    char pad_0[0x15];
    char* name;
};

struct OrderType {
    MissionName_00467e50* GetTableEntry();
};

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

extern Game* g_game;

extern int g_probePanelBottom;
extern int g_statusPanelNextTick;

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
int GetTextKeyColor();
void __stdcall SetTextColors(int param_1, int param_2);
void __stdcall SetFont(int param_1);
int GetFontHeight();
void __stdcall DrawString(void* surface, const char* text, int x, int y, int maxWidth);
void __stdcall DrawString(int surface, const char* text, int x, int y, int maxWidth);

int GetScreenHeight();
int GetScreenWidth();
void __stdcall SetOffscreenSurface(void* surf);
void __stdcall FillSurface(void* surf, int mode);
void FlipScreen();
void GetDisplayFieldE4();
unsigned long GetMilliseconds();

void __stdcall FadeRectangle(void* surface, Rect* rect, int level);
void __stdcall FadeRectangle(void* surface, void* rect, int level);
void __stdcall GetByteRates(unsigned int* sent, unsigned int* received);
int __stdcall GetBuildRating(int player, unsigned short type);

void __stdcall PlaySoundByName(char* name, int param_2);
int __stdcall IsKeyDown(int key);
void __stdcall DrawTextClipped(void* surf, void* text, int x, int y, int color, int just);

void __stdcall GetObjectBounds(Object_004cb650* obj, Vec3* lo, Vec3* hi, int arg);
void __stdcall DrawRotatedQuadOutline(void* surface, Vec3* offset, Vec3* corners, short* angles);
void __stdcall DrawRotatedQuadOutline(Vec3* view, Vec3* pos, Vec3* corners, void* unit);

int __stdcall GetGafSequenceFrame(short* ref);
void __stdcall FillPolygon(void* surface, Point* points, int count, int color);
void __stdcall DrawFrameQuad(void* surface, void* texture, Point* points, void* src);

int __stdcall GetCellHeight(void* p);

void __stdcall ReportGameEvent(int param_1);
void __stdcall AddMessage(char* param_1, int param_2, int param_3, int param_4);
int __stdcall OpenMessageBox(char* dest, char* text, int param_3, int param_4, int param_5);

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
    unsigned char color = g_game->field_dd5;
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
    FillSurface(surf, 0);

    int side = (*(PlayerInfo_00467440 (*)[10])&g_game->players)[g_game->playerIndex].data->side;

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
    FadeRectangle(surface, &r, -0x18);
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
    unsigned char& color = g_game->field_dda;
    DrawRectangle(surface, rect, color);
    if (percent >= 100)
        percent = 100;
    if (percent > 0) {
        rect->right = (rect->right - rect->left) * percent / 100 + rect->left;
        FillRectangle(surface, rect, color);
    }
}

static void Bar(void* surface, Rect* rect, int percent)
{
    unsigned char& color = g_game->field_dda;
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
    Bar(surface, &r, sent * 100 / 5600);
    r.left = 0x81;
    r.right = 0xc1;
    r.top = r.bottom + 1;
    sprintf(buf, "Receive - %1.1f K/s", received * 0.001);
    DrawString(surface, buf, r.left, r.top, -1);
    r.top += h;
    r.bottom = r.top + 8;
    Bar(surface, &r, received * 100 / 5600);
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
    FadeRectangle(surface, &r, -0x18);
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
            (*(PlayerInfo_00467440 (*)[10])&g_game->players)[team].unitCount, g_game->maxUnitsSetting);
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
        DrawRotatedQuadOutline(view, &pos, corners, (char*)&unit->angles);
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

void ResetCameraState(void);
void ClampCameraPosition(void);
void ClampCameraTarget(void);

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
        Point* dst = (Point *)g_game->field_1438b;
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
                DrawFrameQuad(surface, tex, (Point *)g_game->field_1438b, 0);
            }
        } else {
            FillPolygon(surface, (Point *)g_game->field_1438b, e->count, e->colorIndex);
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
        result = OpenMessageBox((*(char (*)[4])&g_game->field_519), text, (int)(t * 0.5), 1, 1);
    }
    return result;
}
