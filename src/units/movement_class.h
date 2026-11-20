#ifndef MOVEMENT_CLASS_H
#define MOVEMENT_CLASS_H

struct MapMoveLimits;
struct PathLockDstView;
struct PathLockSrcView;
struct TdfParserView;

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

    MovementClass();
    ~MovementClass();
    void ReadMoveInfo(TdfParserView* src);
};

struct MovementClassTable {
    MovementClass entries[32];
};

MovementClass* __stdcall FindMovementClass(char* name);
void __stdcall NopAfterFeatureEnum(void);
void __stdcall FreeMovementClasses(void);
int __stdcall GetMovementLimits(MapMoveLimits* out, int capacity);

void __stdcall CopyPathLockFields(PathLockDstView* dst, PathLockSrcView* src);

#endif
