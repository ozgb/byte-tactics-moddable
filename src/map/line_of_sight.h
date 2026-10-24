#ifndef LINE_OF_SIGHT_H
#define LINE_OF_SIGHT_H
struct Cell;

struct Cell;
struct Eye;
struct Point16;
struct SightQuery;
struct Unit;
struct Vec3;

void __stdcall FreeLosTables();
void __stdcall InitEyeballs();
void __stdcall FreeEyeballs();
Cell* __stdcall GetMapCellAtPosition(Vec3* pos);
Cell* __stdcall GetOriginCellAtPosition(Vec3* pos);
void __stdcall RemoveUnitLineOfSight(Unit* unit);
int __stdcall IsEyeballExpired(Eye* obj);
void __stdcall ExpireEyeballs();
void __stdcall UpdateLineOfSight(SightQuery* params);
void __stdcall UpdateUnitLineOfSight(Unit* unit);
void __stdcall InitUnitSightCircleReveal(SightQuery* params);
void __stdcall BuildDerivedLayers(void);
void __stdcall UpdateCellHeightRange(Point16 pos, Point16 size);
void __stdcall UpdateAllCellHeightRanges();
void __stdcall ClearBorderFeatures();
void __stdcall LoadTntMap();
void __stdcall FreeMapResources();
void __stdcall ClampWorldPosToTerrain(int x, int y, Vec3* out);
void __stdcall RecalculateLineOfSight(int arg);
void __stdcall RevealAroundUnit(SightQuery* params);
void __stdcall AddEyeball(Vec3* src, int a, int b, int c);
void __stdcall RevealNewUnit(Unit* unit);

Cell* __stdcall GetMapCell(int x, int y);

#endif
