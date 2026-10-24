#ifndef MOVEMENT_CLASS_H
#define MOVEMENT_CLASS_H

#include "../../interfaces/ref.h"
struct Point16;

struct Point16;
struct Source_00440340;
struct Object_00440af0;

class MovementClass {
public:
    int* name;
    short footprintX;
    short footprintZ;
    short maxWaterDepth;
    short minWaterDepth;
    unsigned char maxSlope;
    unsigned char badSlope;
    unsigned char maxWaterSlope;
    unsigned char badWaterSlope;
    unsigned int width;
    unsigned int height;
    Ref<unsigned int> cells;
    unsigned int lastTick;

    MovementClass();
    ~MovementClass();
    void ReadMoveInfo(Source_00440340* src);
    void ResizePassMap(unsigned int w, unsigned int h);
    void SetPassMapCell(int x, int y, int value);
    void RefreshPassMap(Point16 a, Point16 b);
    void RefreshMovedUnits(Object_00440af0* p);
    void RefreshUnitIfStale(Object_00440af0* p);
    int InBounds(unsigned int x, unsigned int y)
    {
        return x < width && y < height;
    }
    int Get(int x, int y)
    {
        return (cells[width * (y >> 4) + x] >> ((y & 0xf) << 1)) & 3;
    }
};

struct MovementClassTable {
    MovementClass entries[32];
};

MovementClass* __stdcall FindMovementClass(char* name);
void __stdcall NopAfterFeatureEnum(void);
void __stdcall BuildAllPassMaps(void);
void __stdcall FreeMovementClasses(void);
void __stdcall RefreshAllPassMaps(Point16 a, Point16 b);
int __stdcall FindUnusedLogo();

#endif
