#ifndef INTERFACES_MOVEMENT_H
#define INTERFACES_MOVEMENT_H

class BitReader;
class BitWriter;
class HapiBank;
class OrderFx;
class UnitMotion;
class PathGoal;
class PathOrder;
class ApproachRadius;
class RingApproach;
class PointMarker;
class AirManeuverOrder;
struct Unit;
struct Order;
struct PathfindingEventHeader;
struct PathPoint;
struct Point16;
struct Vec3;

// Major version of the movement interface, changed when a slot is removed or its signature changes.
#define MOVEMENT_VERSION_MAJOR 2
// Minor version of the movement interface, changed when a slot is added.
#define MOVEMENT_VERSION_MINOR 0

// Unit motion, its goals and the targets the goals steer to.
struct MovementSeam {
    // Steers a unit, moves it and updates its move rate for one tick (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* UpdateMotion)(UnitMotion* motion, int unusedEdx, Unit* unit);
    // Applies an event of a path search, its payload of size bytes, to its requester, with the waypoints Poll copied for it; an event smaller than its payload changes nothing: a started search sets the claim tick of the goal and a status flag on the order, a finished one gives the goal its waypoints. Hooks: none.
    void (__stdcall* ApplyPathEvent)(const PathfindingEventHeader* event, unsigned int size, const PathPoint* waypoints);
    // Allocates the movement of a unit from the sim block, builds it and stores it in the unit. Hooks: none.
    void (__stdcall* NewUnitMotion)(Unit* unit);
    // Reads the movement of a unit from a saved game bank (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* LoadMotion)(UnitMotion* motion, int unusedEdx, Unit* unit, HapiBank* file);
    // Writes the movement of a unit to a saved game bank (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* SaveMotion)(UnitMotion* motion, int unusedEdx, Unit* unit, HapiBank* file);
    // Frees what the movement of a unit holds (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* DestroyMotionObject)(UnitMotion* motion);
    // Reads the network state of a unit's movement from a packet, through the goal that moves it. Hooks: none.
    void (__stdcall* UnitMotionReadNetState)(UnitMotion* motion, BitReader* reader);
    // Switches the flight mode of a unit's movement, which grounds or lifts an aircraft (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* SetFlightMode)(UnitMotion* motion, Unit* unit, int state);
    // Sets the altitude an order that moves a unit holds it at (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* PathOrderSetAltitude)(PathOrder* order, int altitude);
    // Sets how near a move target a unit has to come (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* PathOrderSetApproachRadius)(PathOrder* order, short radius);
    // Sets the heading a unit takes when it arrives (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* PathOrderSetHeading)(PathOrder* order, short heading);
    // Sets the altitude of an air maneuver target (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* AirManeuverOrderSetAltitude)(AirManeuverOrder* order, int altitude);
    // Allocates a move target at a position from the sim block, builds it for an order and returns it. Hooks: none.
    PathOrder* (__stdcall* NewPathOrderAt)(Order* source, const Vec3* pos);
    // Allocates a move target that follows a unit from the sim block, builds it for an order and returns it. Hooks: none.
    PathOrder* (__stdcall* NewPathOrderToUnit)(Order* source, Unit* unit);
    // Allocates a move target at a position near a unit from the sim block, builds it for an order and returns it. Hooks: none.
    PathOrder* (__stdcall* NewPathOrderToUnitAt)(Order* source, Unit* unit, const Vec3* pos);
    // Allocates a move target that follows a unit at an angle from the sim block, builds it for an order and returns it. Hooks: none.
    PathOrder* (__stdcall* NewPathOrderToUnitAtAngle)(Order* source, Unit* unit, short angle);
    // Allocates an air maneuver target from the sim block, builds it for an order from two positions and returns it. Hooks: none.
    AirManeuverOrder* (__stdcall* NewAirManeuverOrder)(Order* source, const Vec3* from, const Vec3* to);
    // Allocates a disc target from the sim block, builds it for an order and returns it. Hooks: none.
    ApproachRadius* (__stdcall* NewApproachRadius)(Order* source, int x, int y, int radius);
    // Allocates a ring target from the sim block, builds it for an order and returns it. Hooks: none.
    RingApproach* (__stdcall* NewRingApproach)(Order* source, int x, int y, int inner, int outer);
    // Allocates a footprint marker target from the sim block, builds it for an order and returns it. Hooks: none.
    PointMarker* (__stdcall* NewPointMarker)(Order* source, Point16 pos, Point16 size);
    // Allocates a move target from the sim block, reads it from a saved game bank under a name and returns it. Hooks: none.
    PathOrder* (__stdcall* LoadPathOrder)(Order* source, HapiBank* file, char* name);
    // Allocates an air maneuver target from the sim block, reads it from a saved game bank under a name and returns it. Hooks: none.
    AirManeuverOrder* (__stdcall* LoadAirManeuverOrder)(Order* source, HapiBank* file, char* name);
    // Allocates a disc target from the sim block, reads it from a saved game bank under a name and returns it. Hooks: none.
    ApproachRadius* (__stdcall* LoadApproachRadius)(Order* source, HapiBank* file, char* name);
    // Allocates a ring target from the sim block, reads it from a saved game bank under a name and returns it. Hooks: none.
    RingApproach* (__stdcall* LoadRingApproach)(Order* source, HapiBank* file, char* name);
    // Allocates a footprint marker target from the sim block, reads it from a saved game bank under a name and returns it. Hooks: none.
    PointMarker* (__stdcall* LoadPointMarker)(Order* source, HapiBank* file, char* name);
    // Returns the goal that steers a unit's movement. Hooks: none.
    PathGoal* (__stdcall* MotionGoal)(UnitMotion* motion);
    // Returns the target a goal steers to, or null. Hooks: none.
    OrderFx* (__stdcall* GoalTarget)(PathGoal* goal);
    // Hands a goal the target it steers to, or null (the virtual SetPathOrder). Hooks: none.
    void (__stdcall* GoalSetPathOrder)(PathGoal* goal, void* target);
    // Destroys a target and frees its memory (the virtual deleting destructor); the target is never null. Hooks: none.
    void (__stdcall* OrderFxDestroy)(OrderFx* target);
    // Returns the kind of a target, 0 for none (the virtual GetType). Hooks: none.
    int (__stdcall* OrderFxGetType)(OrderFx* target);
    // Writes a target to a bank of a saved game under a name and returns 1 when it was written (the virtual SerializeToSave). Hooks: none.
    int (__stdcall* OrderFxSerializeToSave)(OrderFx* target, Order* order, HapiBank* file, char* name);
    // Returns nonzero when a goal has network state to send (the virtual HasNetUnitState). Hooks: none.
    int (__stdcall* GoalHasNetUnitState)(PathGoal* goal);
    // Writes the network state of a goal to a packet (the virtual SerializeNetUnitState). Hooks: none.
    void (__stdcall* GoalSerializeNetUnitState)(PathGoal* goal, BitWriter* stream);
};

#endif
