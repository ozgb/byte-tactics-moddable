#include <stdio.h>
#include "../../interfaces/ref.h"
#include <math.h>
#include <memory.h>
typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
struct Game;
extern Game* g_game;
unsigned long GetMilliseconds();
int __stdcall FormatNetStats(int);
int __stdcall DrawSelectedGoal(int,int);
int __stdcall DrawMapDebugOverlay(int);
int __stdcall DrawExplosions(int);
#include "../map/mission.h"

int __stdcall DrawUnit(int,int);
int __stdcall DrawOptionsScrollBar(int);
int __stdcall DrawMessages(int);
float __stdcall GetEnergyIncome(int);
float __stdcall GetEnergyUsage(int);
float __stdcall GetMetalIncome(int);
float __stdcall GetMetalUsage(int);
int __stdcall IsFootprintVisible(int,int,int,int,int,int);
int __stdcall DrawRadar(int);
int __stdcall BlitGafFrameAtOffset(int,int,int,int);
int __stdcall BlitSideLogoToRect(int,int,int,int);
int __stdcall DrawNetworkStats(int);
int __stdcall DrawStatusPanel(int);
int __stdcall DrawHitPointBar(int,int,int,int);
int __stdcall DrawSelectionBox(int,int);
int __stdcall BlitFeatureGaf(int,int,int,int);
int __stdcall DrawUnitInfoPanel(int);
int __stdcall DrawProfileBarLine(int,int,int);
int __stdcall DrawParticleList(int,int);
int __stdcall DrawMapTiles(int);
int __stdcall DrawFogOfWar(int);
int __stdcall FindNextSelectedUnit(int,int);
int __stdcall DrawSelectedUnitOrderOverlays(int,int);
int __stdcall DrawScorePanel(int);
int __stdcall DrawProjectiles(int);
int __stdcall BlitMenuLayers(int,int,int);
int GetFrameRate();
int GetScreenHeight();
int __stdcall PointInRect(int,int,int);
int __stdcall GetGafFrame(int,int);
int __stdcall DrawFrame(int,int,int,int);
int __stdcall DrawLine(int,int,int,int,int,int);
int __stdcall FillRectangle(int,int,int);
int __stdcall DrawRectangle(int,int,int);
int __stdcall SetTextColors(int,int);
int GetTextKeyColor();
int __stdcall SetFont(int);
int GetFontHeight();
int __stdcall GetTextWidth(int,int);
int __stdcall DrawString(int,int,int,int,int);
int __stdcall IsKeyDown(int);
int HideSoftwareCursor();
int ShowSoftwareCursor();
int __stdcall Translate(int);
int FlipScreen();
int __stdcall SetOffscreenSurface(int);
int __stdcall ResetClipRect(int);

#include "../graphics/surface.h"

void UpdateScreenShake(void);
void BeginMouseScroll(void);
void EndMouseScroll(void);
void UpdateMouseScroll(void);

#include "frame_timers.h"
static inline void ProfileMark(FrameTimers *p, int i)
{
  unsigned long t = GetMilliseconds();
  p->acc[i] += t - p->last;
  p->last = t;
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

#pragma pack(push, 1)
struct Tile_00468cf0 {
  char unknown_0[4];
  byte height;
  char unknown_5[3];
  ushort feature;
  char unknown_a[2];
  byte flags;
};

struct Feature_00468cf0 {
  char unknown_0[0x94];
  short footprintX;
  short footprintZ;
  char unknown_98[0xfa - 0x98];
  byte height;
  char unknown_fb[4];
  byte flags;
};
#pragma pack(pop)

struct Unit_00468cf0;
struct Bits8 { ushort b0:1; ushort b1:1; ushort b2:1; ushort b3:1; ushort b4:1; ushort b5:1; ushort b6:1; ushort b7:1; };
struct UnitFlags { uint kind:2; uint b2:1; uint b3:1; uint b4:1; uint b5:1; uint b6:1; uint b7:1; };

#pragma pack(push, 1)
struct SideData_00468cf0 {
  char unknown_0[0x95];
  unsigned char bSideId;
  char unknown_96[0x9b - 0x96];
  unsigned char gameOptFlags;
  char unknown_9c[0xb9 - 0x9c];
};

struct PlayerState_00468cf0 {
  char unknown_0[0x27];
  Ref<SideData_00468cf0> info;
  char unknown_2b[0x8c - 0x2b];
  float flEnergyAmount;
  char unknown_90[0x98 - 0x90];
  float flMetalAmount;
  char unknown_9c[0xa4 - 0x9c];
  float flEnergyStorage;
  float flMetalStorage;
  char unknown_ac[0xe4 - 0xac];
  float flShareThresholdMetal;
  float flShareThresholdEnergy;
  char unknown_ec[0xf8 - 0xec];
  unsigned int nDisplayTimer;
  char unknown_fc[0x146 - 0xfc];
  unsigned char bSlotIndex;
  char unknown_147[0x14b - 0x147];
};

struct Unit_00468cf0 {
  char unknown_0[0x6c];
  short xWhole;
  char unknown_6e[2];
  short yWhole;
  char unknown_72[2];
  short zWhole;
  char unknown_76[0x96 - 0x76];
  Ref<PlayerState_00468cf0> player;
  Ref<char> script;
  char unknown_9e[0xac - 0x9e];
  int group;
  char unknown_b0[0x110 - 0xb0];
  UnitFlags flags;
  char unknown_111[0x118 - 0x111];
};

struct SideDef_00468cf0 {
  char aName[0x1e];
  char aNamePrefix[4];
  char aCommander[0x20];
  Rect rcLogo;
  Rect rcEnergyBar;
  Rect rcEnergyNum;
  Rect rcMetalBar;
  Rect rcMetalNum;
  Rect rcTotalUnits;
  Rect rcTotalTime;
  Rect rcEnergyMax;
  Rect rcMetalMax;
  Rect rcEnergy0;
  Rect rcMetal0;
  Rect rcEnergyProduced;
  Rect rcEnergyConsumed;
  Rect rcMetalProduced;
  Rect rcMetalConsumed;
  Rect rcLogo2;
  Rect rcUnitName;
  Rect rcDamageBar;
  Rect rcUnitEnergyMake;
  Rect rcUnitEnergyUse;
  Rect rcUnitMetalMake;
  Rect rcUnitMetalUse;
  Rect rcMissionText;
  Rect rcUnitName2;
  Rect rcDamageBar2;
  Rect rcName;
  Rect rcDescription;
  Rect aReloadRects[3];
  int nEnergyColor;
  int nMetalColor;
  int nSideIndex;
  char* pFontData;
};

#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

static inline float Approach(float fcur, float ftarget)
{
  int cur = (int)fcur;
  int d = (int)ftarget - cur;
  if (d < 0) {
    d /= 8;
    if (d == 0)
      d = -1;
  }
  else if (d > 0) {
    d /= 8;
    if (d == 0)
      d = 1;
  }
  else
    d = 0;
  return (float)(d + cur);
}

static void DrawResourcePanel(Surface *ctx, PlayerState_00468cf0 *pl, Resources *res)
{
  Rect bar, box;
  char text[32];
  int side = pl->info->bSideId;
  SideDef_00468cf0 *sideDef = &(*(SideDef_00468cf0 (*)[5])&g_game->sideDefs)[side];
  SetFont((int)sideDef->pFontData);
  GetFontHeight();
  byte *pal = &g_game->colors[0];
  SetTextColors(pal[0xf], GetTextKeyColor());
  int bx = 0x81;
  do {
    ushort *gaf = (ushort *)GetGafFrame((int)g_game->sidePanelRows[side + (bx > 0x81) * 5], 0);
    BlitGafFrameAtOffset((int)ctx, (int)gaf, bx, 0);
    bx += *gaf;
  } while (bx < g_game->width_37e1f);
  BlitSideLogoToRect((int)ctx, (int)pl, (int)&sideDef->rcLogo, 0);
  Rect *r = &sideDef->rcEnergyBar;
  bar = *r;
  if (pl->flEnergyStorage > 0.0f) {
    bar.right = (int)((double)(bar.right - bar.left) * res->metal / pl->flEnergyStorage + bar.left);
    FillRectangle((int)ctx, (int)&bar, sideDef->nEnergyColor);
    if (pl->flShareThresholdEnergy > 0.0f && pl->flEnergyAmount > pl->flShareThresholdEnergy) {
      box = *r;
      box.left = (int)((double)(box.right - box.left) * pl->flShareThresholdEnergy / pl->flEnergyStorage + box.left);
      box.right = box.left + 2;
      FillRectangle((int)ctx, (int)&box, pal[0xc]);
    }
  }
  sprintf(text, "%d", (int)res->metal);
  DrawString((int)ctx, (int)text, sideDef->rcEnergyNum.left, sideDef->rcEnergyNum.top, -1);
  DrawString((int)ctx, (int)"0", sideDef->rcEnergy0.left, sideDef->rcEnergy0.top, -1);
  sprintf(text, "%d", (int)pl->flEnergyStorage);
  int w = GetTextWidth((int)sideDef->pFontData, (int)text);
  DrawString((int)ctx, (int)text, sideDef->rcEnergyMax.left - w, sideDef->rcEnergyMax.top, -1);
  if (res->metalIncome > 99999.0f)
    sprintf(text, "%dK", (int)res->metalIncome / 1000);
  else
    sprintf(text, "%d", (int)res->metalIncome);
  SetTextColors(pal[0xa], GetTextKeyColor());
  DrawString((int)ctx, (int)text, sideDef->rcEnergyProduced.left, sideDef->rcEnergyProduced.top, -1);
  if (res->metalUse < -99999.0f)
    sprintf(text, "%dK", (int)res->metalUse / 1000);
  else
    sprintf(text, "%d", abs((int)res->metalUse));
  SetTextColors(pal[0xc], GetTextKeyColor());
  DrawString((int)ctx, (int)text, sideDef->rcEnergyConsumed.left, sideDef->rcEnergyConsumed.top, -1);
  r = &sideDef->rcMetalBar;
  bar = *r;
  if (pl->flMetalStorage > 0.0f) {
    bar.right = (int)((double)(bar.right - bar.left) * res->energy / pl->flMetalStorage + bar.left);
    FillRectangle((int)ctx, (int)&bar, sideDef->nMetalColor);
    if (pl->flShareThresholdMetal > 0.0f && pl->flMetalAmount > pl->flShareThresholdMetal) {
      box = *r;
      box.left = (int)((double)(box.right - box.left) * pl->flShareThresholdMetal / pl->flMetalStorage + box.left);
      box.right = box.left + 2;
      FillRectangle((int)ctx, (int)&box, pal[0xc]);
    }
  }
  SetTextColors(pal[0xf], GetTextKeyColor());
  sprintf(text, "%d", (int)res->energy);
  DrawString((int)ctx, (int)text, sideDef->rcMetalNum.left, sideDef->rcMetalNum.top, -1);
  DrawString((int)ctx, (int)"0", sideDef->rcMetal0.left, sideDef->rcMetal0.top, -1);
  sprintf(text, "%d", (int)pl->flMetalStorage);
  w = GetTextWidth((int)sideDef->pFontData, (int)text);
  DrawString((int)ctx, (int)text, sideDef->rcMetalMax.left - w, sideDef->rcMetalMax.top, -1);
  sprintf(text, "%.1f", res->energyIncome);
  SetTextColors(pal[0xa], GetTextKeyColor());
  DrawString((int)ctx, (int)text, sideDef->rcMetalProduced.left, sideDef->rcMetalProduced.top, -1);
  sprintf(text, "%.1f", fabs(res->energyUse));
  SetTextColors(pal[0xc], GetTextKeyColor());
  DrawString((int)ctx, (int)text, sideDef->rcMetalConsumed.left, sideDef->rcMetalConsumed.top, -1);
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
  byte idx;
  int cy;
  Resources res;
  MapGrid *mv;
  byte *colors;
  int cx;
  int x;
  PlayerState_00468cf0 *player;
  int i, k;

  cx = (g_game->width_37e1f + 0x80) / 2;
  cy = g_game->field_37e23 / 2;
  SetOffscreenSurface((int)g_game->screen);
  ctx = **(Surface **)&g_game->screen;
  colors = &g_game->colors[0];
  HideSoftwareCursor();
  ctx.SetClipRect(g_game->rect);
  ProfileMark(&g_game->timers, 8);
  DrawMapTiles((int)&ctx);
  DrawMapDebugOverlay((int)&ctx);
  x = g_game->xWhole - g_game->scrollX + 0x80;
  char *&game = *(char**)&g_game;
  int *viewY = &((Game *)game)->scrollY;
  y = g_game->zWhole - (g_game->yWhole >> 1) - *viewY + 0x20;
  if ((char)g_game->debugMode == '\x02') {
    DrawLine((int)&ctx, x - 2, y, x + 2, y, colors[0xf]);
    DrawLine((int)&ctx, x, y - 2, x, y + 2, colors[0xf]);
  }
  ResetClipRect((int)&ctx);

  {
    PlayerState_00468cf0 *pl = &(*(PlayerState_00468cf0 (*)[11])&g_game->players_1b63)[g_game->playerIndex];
    res = (*(Resources*)&g_game->a);
    res.owner = pl->bSlotIndex;
    res.metal = Approach(res.metal, pl->flEnergyAmount);
    res.energy = Approach(res.energy, pl->flMetalAmount);
    res.maxMetal = pl->flEnergyStorage;
    res.maxEnergy = pl->flMetalStorage;
    if (res.metal > res.maxMetal)
      res.metal = res.maxMetal;
    if (res.energy > res.maxEnergy)
      res.energy = res.maxEnergy;
    static unsigned int s_incomesShownAt = ~0u;
    if (pl->nDisplayTimer != s_incomesShownAt) {
      s_incomesShownAt = pl->nDisplayTimer;
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
  DrawUnitInfoPanel((int)&ctx);
  DrawRadar((int)&ctx);
  ctx.SetClipRect(g_game->rect);
  ProfileMark(&g_game->timers, 3);

  idx = g_game->playerIndex;
  mv = &g_game->sortUnits;
  player = &(*(PlayerState_00468cf0 (*)[11])&g_game->players_1b63)[idx];
  SetFont((int)(*(SideDef_00468cf0 (*)[5])&g_game->sideDefs)[player->info->bSideId].pFontData);
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
    }
    else
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
    ushort *pIdx = g_game->list_1435f;
    k = 0;
    while (k < g_game->count_14367) {
      Unit_00468cf0 *unit = (Unit_00468cf0 *)((int)g_game->units.get() + *pIdx * 0x118);
      int row = ((int)unit->zWhole - g_game->scrollY) / 16 + 0x10;
      if (row >= 0 && row < mv->rows) {
        Unit_00468cf0 ***pCur = (Unit_00468cf0***)&mv->cursor[row];
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
      Tile_00468cf0 *tile = (Tile_00468cf0*)mv->tiles.get() + (mv->width * y + x);
      int c = 0;
      while (c < w) {
        tile->flags &= 0xfb;
        if (tile->feature < 0xfffb) {
          Feature_00468cf0 *feat = (Feature_00468cf0 *)g_game->features + tile->feature;
          if (feat->height < 10) {
            if ((feat->flags & 8) && ((tile->flags >> 3 & 0xf) != idx)) {
              if (IsFootprintVisible((int)player, x, y, feat->footprintX, feat->footprintZ, tile->height))
                BlitFeatureGaf((int)&ctx, (int)tile, x, y);
            }
            else
              BlitFeatureGaf((int)&ctx, (int)tile, x, y);
          }
          else
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
      Unit_00468cf0 **pUnit = (Unit_00468cf0**)mv->buf + row * mv->stride;
      for (k = 0; k < mv->count[row]; k++, pUnit++) {

        Unit_00468cf0 *u = *pUnit;
        if (u->flags.kind == 1 && param_1 != 0) {
          if (u->flags.b4)
            DrawSelectionBox((int)&ctx, (int)u);
          if (u->script != 0)
            DrawUnit((int)&ctx, (int)u);
        }
      }
      Tile_00468cf0 *tile = (Tile_00468cf0*)mv->tiles.get() + (y * mv->width + x0);
      for (int c = 0; c < w; c++, tile++) {
        x = x0 + c;

        if (tile->flags & 4) {
          Feature_00468cf0 *feat = (Feature_00468cf0 *)g_game->features + tile->feature;
          if ((feat->flags & 8) && ((tile->flags >> 3 & 0xf) != idx)) {
            if (IsFootprintVisible((int)player, x, y, (int)feat->footprintX, feat->footprintZ, tile->height))
              BlitFeatureGaf((int)&ctx, (int)tile, x, y);
          }
          else
            BlitFeatureGaf((int)&ctx, (int)tile, x, y);
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
      Unit_00468cf0 **pUnit = (Unit_00468cf0**)mv->buf + mv->stride * i;
      k = 0;
      while (k < mv->count[i]) {
        Unit_00468cf0 *u = *pUnit;
        if (u->flags.kind != 1) {
          if (u->flags.b4)
            DrawSelectionBox((int)&ctx, (int)u);
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
    DrawSelectedUnitOrderOverlays((int)&ctx, (int)&(*(void **)&g_game->value_142f3));

  if (param_1 != 0) {
    ushort *pIdx = g_game->list_1435f;
    for (k = 0; k < g_game->count_14367; k++, pIdx++) {
      Unit_00468cf0 *unit = (Unit_00468cf0 *)((int)g_game->units.get() + *pIdx * 0x118);
      if ((g_game->visualFlagsByte & 1) || unit->group != 0) {
        char str[2];
        str[1] = 0;
        x = unit->xWhole - g_game->scrollX + 0x80;
        y = unit->zWhole - g_game->scrollY - (unit->yWhole >> 1) + 0x20;
        if (g_game->visualFlagsByte & 1) {
          if ((char)unit->player->bSlotIndex == (char)idx)
            DrawHitPointBar((int)&ctx, (int)unit, x, y + 10);
          if ((char)unit->player->bSlotIndex == (char)idx && unit->group != 0) {
            str[0] = (char)unit->group + '0';
            DrawString((int)&ctx, (int)str, x, y + 0xe, -1);
          }
        }
      }
    }
    DrawParticleList((int)&ctx, 9);
  }
  ProfileMark(&g_game->timers, 4);
  if ((g_game->endGameFlags & 1) && (g_game->endGameFlags & 2) && param_1 != 0)
    DrawSelectedGoal((int)&ctx, FindNextSelectedUnit(0, 0));
  if (param_1 != 0)
    DrawFogOfWar((int)&ctx);
  ProfileMark(&g_game->timers, 5);

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
    DrawRectangle((int)&ctx, (int)&box, c);
    box.left++;
    box.top++;
    box.right--;
    box.bottom--;
    if ((char)g_game->orderMode == '\x0e')
      DrawRectangle((int)&ctx, (int)&box, c);
    else
      DrawRectangle((int)&ctx, (int)&box, *colors);
  }
  if (g_game->net->GetGameType() == 3 ||
      g_game->net->GetGameType() == 2) {
    ResetClipRect((int)&ctx);
    DrawScorePanel((int)&ctx);
    ctx.SetClipRect(g_game->rect);
  }
  DrawStatusPanel((int)&ctx);
  if (g_game->showBps != 0)
    DrawNetworkStats((int)&ctx);
  if (param_1 != 0)
    DrawMessages((int)&ctx);
  if ((g_game->flagsByte_3923b & 2) && param_1 != 0) {
    SetTextColors(colors[0xf], GetTextKeyColor());
    SetFont(g_game->fontComix);
    int ty = GetFontHeight() * 3 - 10;
    sprintf(debugText, "FRATE: %d\n", GetFrameRate());
    DrawString((int)&ctx, (int)debugText, 0x83, ty, -1);
    DrawString((int)&ctx, (int)"[Release]", 0xbc, ty, -1);
    sprintf(debugText, "MODE %s INFO %s", (*(Bits8*)&g_game->endGameFlags_3923b).b1 ? "DEBUG" : "NORMAL",
            (*(Bits8*)&g_game->endGameFlags_3923b).b0 ? "ON" : "OFF");
    DrawString((int)&ctx, (int)debugText, 0x1ee, ty, -1);
    ty += GetFontHeight();
    if (g_game->netGameFlags & 1) {
      FormatNetStats((int)debugText);
      DrawString((int)&ctx, (int)debugText, 0xbc, ty, -1);
    }
  }
  if (g_game->pauseFlags & 1)
    DrawFrame((int)&ctx, GetGafFrame(g_game->igpaused, 0), cx, cy);
  if (((*(PlayerState_00468cf0 (*)[11])&g_game->players_1b63)[g_game->localPlayer].info->gameOptFlags & 0x40) == 0) {
    if ((*(Bits8*)&g_game->endGameFlags_3923b).b5)
      DrawFrame((int)&ctx, GetGafFrame((int)g_game->image_14813, 0), cx, cy);
    if ((*(Bits8*)&g_game->endGameFlags_3923b).b6)
      DrawFrame((int)&ctx, GetGafFrame((int)g_game->image_14817, 0), cx, cy);
  }
  if ((*(Bits8*)&g_game->uiOptionFlags_37f2f).b6) {
    uint ticks = (unsigned int)g_game->ticks;
    uint hours = ticks / 108000;
    int rest = ticks - hours * 108000;
    int minutes = rest / 1800;
    int seconds = (rest - minutes * 1800) / 30;
    sprintf(gameTime, "%s : %02d:%02d:%02d", (char *)Translate((int)"Game Time"), hours, minutes, seconds);
    SetTextColors(colors[0xf], GetTextKeyColor());
    DrawString((int)&ctx, (int)gameTime, 0x82, -0x22 - GetFontHeight() + GetScreenHeight(), -1);
  }
  if ((*(Bits8*)&g_game->flags_38a51).b1)
    DrawFrame((int)&ctx, GetGafFrame(g_game->cursorHourglass, 0), g_game->width_37e1f - 0x10, g_game->field_37e23 - 0x50);
  ResetClipRect((int)&ctx);
  BlitMenuLayers((int)&g_game->field_519, (int)&ctx, (int)&g_game->rect);
  if (g_game->profileBarsEnabled != 0 && param_1 != 0) {
    DrawProfileBarLine((int)&ctx, (int)"Network", 0);
    DrawProfileBarLine((int)&ctx, (int)"Units", 1);
    DrawProfileBarLine((int)&ctx, (int)"Logic", 2);
    DrawProfileBarLine((int)&ctx, (int)"Render Static", 3);
    DrawProfileBarLine((int)&ctx, (int)"Render Stuff", 4);
    DrawProfileBarLine((int)&ctx, (int)"Render Fog", 5);
    DrawProfileBarLine((int)&ctx, (int)"SFX", 6);
    DrawProfileBarLine((int)&ctx, (int)"Weapon", 7);
    DrawProfileBarLine((int)&ctx, (int)"Misc", 8);
  }
  DrawOptionsScrollBar((int)&ctx);
  ShowSoftwareCursor();
  if (param_1 != 0 && param_2 != 0)
    FlipScreen();
  g_game->timers.AccumulateProfileTime(3);
}
