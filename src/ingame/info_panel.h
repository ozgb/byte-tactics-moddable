#ifndef INFO_PANEL_H
#define INFO_PANEL_H
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

#endif
