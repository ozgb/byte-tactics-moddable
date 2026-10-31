#ifndef INFO_PANEL_H
#define INFO_PANEL_H
struct DebrisPiece;
struct Vec3;

struct Cell;
struct Surface;
struct Unit;
struct Vec3;

struct Vec3;

struct Point;
struct Rect;
struct Unit;

void __stdcall LoadLightBar();
void __stdcall DrawKillCount(void* surface, Unit* unit, int x, int y);
int __stdcall DrawUnitStateProbe(void* surface);
void __stdcall DrawPercentBar(void* surface, Rect* rect, int percent);
int __stdcall DrawUnitBuilderProbe(void* surface);
void __stdcall DrawClosedPolygon(void* surface, Point* points, int count, int color);
void __stdcall ReportGameEventCallback(int);
int __stdcall ShowGameMessage(char* text, int mode);

void __stdcall MarkRecentlyDamaged(void*);
void __stdcall DrawRotatedQuadOutline(void* surface, Vec3* offset, Vec3* corners, short* angles);
void __stdcall DrawLightBars();
void __stdcall UpdateSensorRadarAndCloak(void);
void __stdcall DrawBattleFrame(int, int);

void __stdcall BlitGafFrameAtOffset(void*, short*, int, int);
void __stdcall DrawNetworkStats(void* surface);
void __stdcall DrawStatusPanel(Surface* win);
void __stdcall DrawHitPointBar(void* surface, Unit* unit, int x, int y);
void __stdcall DrawSelectionBox(Vec3* view, Unit* unit);
void __stdcall BlitFeatureGaf(void* dest, Cell* cell, int ix, int iy);
void __stdcall DrawUnitInfoPanel(void* surface);
void __stdcall DrawProfileBarLine(int surface, const char* text, int index);

void __stdcall DrawDebrisProjected(void* surface, Vec3* offset, DebrisPiece* piece, short* angles);

#endif
