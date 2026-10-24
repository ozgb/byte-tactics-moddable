#ifndef TERRAIN_H
#define TERRAIN_H
class MovementClass;
struct Cell;

class ClaimFootprintVisitor;
class DamagedAllyCollector;
class MovementClass;
class UnitDef;
struct Point16;
struct Unit;
struct UnitBucket;
struct Vec3;

void __stdcall ClaimFootprintCells(Unit* obj);
void __stdcall ClaimFootprintsInArea(Point16 pos, Point16 size);
void __stdcall FUN_0047caf0(void);
void __stdcall SetOwner(Unit* unit, UnitBucket* bucket);
void __stdcall ClearFootprintAndUnlink(Unit* unit);
int __stdcall IsFootprintClear(Unit* obj, int flag);
void __stdcall SetYardOpen(Unit* obj, int flag);
void __stdcall ForceNeighborFootprintReclaim(Unit* obj);
void __stdcall SnapWorldPosToFootprint(UnitDef* unit, Vec3* pos);
int __stdcall GetPassMapFootprintValue(MovementClass* obj, int x, int y, int w, int h);
unsigned int __stdcall GetPassMapFootprintValueWithEdgeStrips(MovementClass* obj, int x, int y);
int __stdcall IsPadSlotFree(Unit* pad, int id);
void __stdcall VisitObjectsInArea(Point16 pos, Point16 size, ClaimFootprintVisitor* visitor);
void __stdcall VisitObjectsInRange(Vec3* pos, int range, DamagedAllyCollector& visitor);

int __stdcall GetPassMapCellValue(MovementClass* obj, Cell* cell);

#endif
