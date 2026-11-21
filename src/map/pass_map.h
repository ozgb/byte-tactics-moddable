#ifndef MAP_PASS_MAP_H
#define MAP_PASS_MAP_H

#include "../../interfaces/ref.h"
#include "../util/vec3.h"

struct MapMoveLimits;
struct Point16;
struct Unit;

#pragma pack(push, 2)
struct MotionTickView {
    char unknown_0[0x26];
    unsigned int lastTick;
};

struct UnitPassMapView {
    Ref<MotionTickView> unit;
    char unknown_4[0x76 - 0x4];
    Point16 a;
    char unknown_7a[0x7e - 0x7a];
    Point16 b;
};
#pragma pack(pop)

class PassMap {
public:
    short footprintX;
    short footprintZ;
    short maxWaterDepth;
    short minWaterDepth;
    unsigned char maxSlope;
    unsigned char badSlope;
    unsigned char maxWaterSlope;
    unsigned char badWaterSlope;
    int defined;
    unsigned int width;
    unsigned int height;
    Ref<unsigned int> cells;
    unsigned int lastTick;

    void Resize(unsigned int w, unsigned int h);
    void SetCell(int x, int y, int value);
    void Build();
    void Refresh(Point16 a, Point16 b);
    void RefreshMovedUnits(UnitPassMapView* p);
    void RefreshUnitIfStale(UnitPassMapView* p);
    int InBounds(unsigned int x, unsigned int y) const
    {
        return x < width && y < height;
    }
    int Get(int x, int y) const
    {
        return (cells[width * (y >> 4) + x] >> ((y & 0xf) << 1)) & 3;
    }
};

struct PassMapTable {
    PassMap entries[32];
};

void __stdcall BuildAllPassMaps(const MapMoveLimits* limits, int count);
void __stdcall FreePassMaps(void);
void __stdcall RefreshAllPassMaps(Point16 a, Point16 b);
void __stdcall RefreshAllPassMapsForUnits(Point16 a, Point16 b);
void __stdcall RefreshPassMapsForUnit(UnitPassMapView* p);
int __stdcall PathLockStamp(Unit* unit);
void __stdcall LoadPathLockStamp(Unit* unit, int stamp);

#endif
