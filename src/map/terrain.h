#ifndef TERRAIN_H
#define TERRAIN_H
#include <vector>
class UnitVisitor;

class MovementClass;
struct Cell;

class ClaimFootprintVisitor;
class MovementClass;
class UnitDef;
struct Player;
struct Point16;
struct Unit;
struct UnitBucket;
struct Vec3;

class DamagedAllyCollector {
public:
    Player* owner;
    std::vector<Unit*>* units;
    Unit* self;
    DamagedAllyCollector(Player* o, std::vector<Unit*>* v, Unit* s) : owner(o), units(v), self(s) {}
    virtual void CollectDamagedAlly(Unit* unit);
};

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
void __stdcall AddUnitToMap(Unit* obj);
void __stdcall RemoveUnitFromMap(Unit* obj);
int __stdcall GetFootprintHeight(UnitDef* unit, Point16 cell);
int __stdcall CanPlaceFootprintAt(Unit* unit, Vec3* pos);
int __stdcall GetCellHeight(Point16* p);
int __stdcall GetCellMeanHeight(Vec3* p);
int __stdcall FindHighestPointOnLine(Vec3 a, Vec3 b);
void __stdcall GetCellPosition(int x, int y, Vec3* out);

void __stdcall VisitObjectsInRect(int x1, int y1, int x2, int y2, UnitVisitor* visitor);

#endif
