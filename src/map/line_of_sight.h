#ifndef LINE_OF_SIGHT_H
#define LINE_OF_SIGHT_H
struct Cell;

struct Cell;
struct Eye;
struct LosRadarEventHeader;
struct LosRadarMapInfo;
struct LosSightCache;
struct StateWriter;
struct Point16;
struct SightQuery;
struct Unit;
struct Vec3;

#pragma pack(push, 1)
struct Rec {
    Rec() { a = 0; b = 0; flags = 0; field_6 = 0; }
    unsigned char a;
    unsigned char b;
    int flags;
    int field_6;
};
#pragma pack(pop)

void __stdcall FreeLosTables();
Cell* __stdcall GetMapCellAtPosition(Vec3* pos);
unsigned int __stdcall SoundHeardMask(const Vec3* at);
Cell* __stdcall GetOriginCellAtPosition(Vec3* pos);
int __stdcall ApplyLosRadarEvent(const LosRadarEventHeader* event, unsigned int size);
unsigned int __stdcall LosRadarRegionSize(const LosRadarMapInfo* map);
void __stdcall LosRadarInit(const LosRadarMapInfo* map, void* region, unsigned int size);
void* __stdcall LosRadarShutdown();
void __stdcall LosRadarSightCache(Unit* unit, LosSightCache* out);
void __stdcall LosRadarWriteState(const StateWriter* w);
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
int __stdcall AddEyeball(Vec3* src, int sightDistance, int eyeHeight, int lifetime);
void __stdcall RevealNewUnit(Unit* unit);
void __stdcall ShareMapInfo(unsigned char from, unsigned char to);

Cell* __stdcall GetMapCell(int x, int y);
void __stdcall AddLineOfSight(SightQuery* params);
void __stdcall RemoveLineOfSight(SightQuery* params);
void __stdcall BuildFogTiles(void);

void __stdcall DrawMapTiles(void* surface);
void __stdcall DrawFogOfWar(void* surface);

#endif
