#ifndef TERRAIN_H
#define TERRAIN_H
#include <vector>
#include "../util/vec3.h"

class PassMap;
struct Cell;

class ClaimFootprintVisitor;
class UnitDef;
struct Player;
struct Point16;
struct Unit;
struct UnitBucket;
struct Vec3;

struct ReclaimableResource {
    Vec3 pos;
    float val;
};

void __stdcall ClaimFootprintCells(Unit* obj);
void __stdcall ClaimFootprintsInArea(Point16 pos, Point16 size);
void __stdcall FUN_0047caf0(void);
void __stdcall SetOwner(Unit* unit, UnitBucket* bucket);
void __stdcall ClearFootprintAndUnlink(Unit* unit);
int __stdcall IsFootprintClear(Unit* obj, int flag);
int __stdcall SetYardOpen(Unit* obj, int flag);
void __stdcall ForceNeighborFootprintReclaim(Unit* obj);
void __stdcall SnapWorldPosToFootprint(UnitDef* unit, Vec3* pos);
int __stdcall GetPassMapFootprintValue(PassMap* obj, int x, int y, int w, int h);
unsigned int __stdcall GetPassMapFootprintValueWithEdgeStrips(PassMap* obj, int x, int y);
unsigned int __stdcall GetTerrainPassValue(int moveClass, int x, int y);
int __stdcall IsPadSlotFree(Unit* pad, int id);
void __stdcall VisitObjectsInArea(Point16 pos, Point16 size, ClaimFootprintVisitor* visitor);
int __stdcall UnitsInRange(const Vec3* at, int range, int skip, unsigned short* ids, int capacity);
struct MapEventHeader;
struct StateWriter;
void __stdcall MapAllocateBuckets();
void __stdcall MapFreeBuckets();
void __stdcall MapWriteState(const StateWriter* w);
int __stdcall ApplyMapEvent(const MapEventHeader* event, unsigned int size);

int __stdcall GetPassMapCellValue(PassMap* obj, Cell* cell);
void __stdcall AddUnitToMap(Unit* obj);
void __stdcall RemoveUnitFromMap(Unit* obj);
int __stdcall GetFootprintHeight(UnitDef* unit, Point16 cell);
int __stdcall CanPlaceFootprintAt(Unit* unit, Vec3* pos);
int __stdcall GetCellHeight(Point16* p);
int __stdcall GetCellMeanHeight(Vec3* p);
int __stdcall GetGroundHeight(Vec3* p);
int __stdcall FindHighestPointOnLine(Vec3 a, Vec3 b);
void __stdcall GetCellPosition(int x, int y, Vec3* out);

#endif
