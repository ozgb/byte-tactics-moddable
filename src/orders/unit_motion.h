#ifndef UNIT_MOTION_H
#define UNIT_MOTION_H

#include "../util/vec3.h"
#include "../../interfaces/ref.h"

struct Unit;
class HapiBank;
class PathGoalVtableView;
class MotionSerializer;

class PathGoal;
class OrderFx;
class BlockMap;
class BlockMapIter;
class BlockInfo;
class FreeBlockMap;
class FreeBlockIter;
class FreeBlockAllocator;
class GroundAllyVisitor;
class DetectionVisitor;
class ClaimFootprintVisitor;
class SonarJamVisitor;
class RadarJamVisitor;
class NameMapTree;
class NameMapIter;
class NameMapAllocator;
class NameTable;
class NameKey;
class MapInsertResult;
class MapCacheEntry;
class UnitTypeSet;
class UnitTable;

#pragma pack(push, 1)

class UnitMotion {
public:
    union {
        Ref<PathGoalVtableView> obj;
        Ref<MotionSerializer> player;
    };
    int movementClass;
    Vec3 velocity;
    Vec3 p2;
    union {
        int speed;
        struct {
            short speedLow;
            short field_22;
        };
    };
    short turn;
    int pathLockStamp;
    int lastMoveTick;
    union {
        unsigned char flags;
        struct {
            unsigned char mode : 2;
            unsigned char flag : 1;
            unsigned char rest : 5;
        };
    };

    void UpdateVelocityFromHeading(Unit* unit, int amount);
    void SteerGroundUnit(Unit* unit);
    void ApplyBankAndPitch(Unit* unit, Vec3* v);
    void SetFlightMode(Unit* unit, int state);
    void SteerAircraft(Unit* unit);
    void UpdatePosition(Unit* unit);
    void UpdateMoveRate(Unit* unit);
    UnitMotion(Unit* unit);
    void DestroyObject();
    void UpdateMotion(Unit* u);
    void ApplyClampedTurnDelta(Unit* unit, short amount);
    void SaveMotion(Unit* unit, HapiBank* file);
    void LoadMotion(Unit* unit, HapiBank* file);
};

#pragma pack(pop)

#endif
