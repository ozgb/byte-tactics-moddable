#ifndef INTERFACES_MOVEMENT_H
#define INTERFACES_MOVEMENT_H

class BitReader;
class BitWriter;
class HapiBank;
struct Unit;
struct PathfindingEventHeader;
struct PathPoint;
struct Point16;
struct Vec3;
struct StateWriter;

// Major version of the movement interface, changed when a slot is removed or its signature changes.
#define MOVEMENT_VERSION_MAJOR 4
// Minor version of the movement interface, changed when a slot is added.
#define MOVEMENT_VERSION_MINOR 0

// The fields every movement event and effect starts with.
struct MovementEventHeader {
    // A kind of event or of effect.
    int kind;
    // The unit the kind is about, as its block offset (an offset from the block base, 0 for none).
    unsigned int unit;
};

// The kinds of event other modules publish to movement's inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects one, which then changes nothing and has outcome 0, when it is smaller than its payload, or its kind is unknown (325 is none since 4.0), or: for the kinds about a target (DESTROY_TARGET, SET_ALTITUDE, SET_APPROACH_RADIUS, SET_HEADING), it names no target, and their unit is not checked; for the other kinds, it names a null unit, or a unit with no movement (all of them but NEW). The kinds about a target then depend on the target's kind, as TargetType gives it.
enum MovementEventKind {
    // unit gets its movement, steered by a goal that suits its player and its unit type; the header alone; outcome 1.
    MOVEMENT_NEW = 320,
    // unit's movement and its goal are freed, and unit has no movement; the header alone; outcome 1.
    MOVEMENT_DESTROY = 321,
    // unit's movement takes the state a saved game keeps (MovementLoadState); outcome 1.
    MOVEMENT_LOAD = 322,
    // unit's movement is put in a flight mode without the flight mode's other changes, as a carrier sets down or picks up its cargo (MovementSetMode); outcome 1.
    MOVEMENT_SET_MODE = 323,
    // unit's movement takes a speed, as another machine sent it (MovementSetSpeed); outcome 1.
    MOVEMENT_SET_SPEED = 324,
    // unit's goal steers to a target, or to none (MovementTarget); outcome 1.
    MOVEMENT_SET_TARGET = 326,
    // A target is destroyed and its memory freed (MovementTarget); outcome 1.
    MOVEMENT_DESTROY_TARGET = 327,
    // unit's movement switches its flight mode, which grounds or lifts an aircraft (MovementSetMode); outcome 1.
    MOVEMENT_SET_FLIGHT_MODE = 328,
    // A move target (kind 2) or an air maneuver target (kind 3) holds its unit at an altitude, outcome 1; a target of another kind (1, 4, 5 or 6) is left as it was, outcome 0 (MovementTargetValue).
    MOVEMENT_SET_ALTITUDE = 329,
    // A move target (kind 2) sets how near its unit has to come, outcome 1; a target of another kind (1, 3, 4, 5 or 6) is left as it was, outcome 0 (MovementTargetValue).
    MOVEMENT_SET_APPROACH_RADIUS = 330,
    // A move target (kind 2) sets the heading its unit takes when it arrives, outcome 1; a target of another kind (1, 3, 4, 5 or 6) is left as it was, outcome 0 (MovementTargetValue).
    MOVEMENT_SET_HEADING = 331
};

// The kinds of effect movement publishes to its outbox, for the modules that subscribe to them; each is a plain request, and the subscriber decides what it does with it.
enum MovementEffectKind {
    // A target reports the progress of the order it moves a unit for: status flags are set on the order (MovementOrderFlags).
    MOVEMENT_EFFECT_ORDER_FLAGS = 336,
    // unit's heading turns and unit has moved (MovementTurn).
    MOVEMENT_EFFECT_TURN = 337,
    // unit's bank and pitch change (MovementTilt).
    MOVEMENT_EFFECT_TILT = 338,
    // unit's bank, heading and pitch change (MovementAngles).
    MOVEMENT_EFFECT_ANGLES = 339,
    // unit moves to a position in its cell and has moved (MovementMove).
    MOVEMENT_EFFECT_MOVE = 340,
    // unit is marked as moved or not moved (MovementMoved).
    MOVEMENT_EFFECT_MOVED = 341,
    // unit moves to a position in another cell, or in another flight mode (MovementChangeCell).
    MOVEMENT_EFFECT_CHANGE_CELL = 342,
    // unit's move rate changes (MovementMoveRate).
    MOVEMENT_EFFECT_MOVE_RATE = 343,
    // unit is placed at a position, as a carried unit follows its carrier's piece (MovementSetPosition).
    MOVEMENT_EFFECT_SET_POSITION = 344,
    // unit's state bits change (MovementSetStateBits).
    MOVEMENT_EFFECT_SET_STATE_BITS = 345,
    // unit's script is told whether the unit is in the water, on the ground or in the air, when that changed; the header alone.
    MOVEMENT_EFFECT_SFX_OCCUPY = 346,
    // A script of unit starts (MovementStartScript).
    MOVEMENT_EFFECT_START_SCRIPT = 347
};

// The state of a unit's movement that a saved game keeps, but for the path lock stamp, which is map's.
struct MovementSavedState {
    // The velocity, in world units with 16 fractional bits: x, y and z.
    int velocity[3];
    // The smoothed acceleration that banks and pitches an aircraft: x, y and z.
    int smoothedAcceleration[3];
    // The speed.
    int speed;
    // The turn of the last tick.
    int turn;
    // The flight mode, 0 to 3.
    int mode;
    // The movement's flag, 0 or 1.
    int flag;
};

// The payload of LOAD.
struct MovementLoadState {
    // The kind and the unit.
    MovementEventHeader h;
    // The state.
    MovementSavedState state;
};

// The payload of SET_MODE and SET_FLIGHT_MODE.
struct MovementSetMode {
    // The kind and the unit.
    MovementEventHeader h;
    // The flight mode, 0 to 3.
    int mode;
};

// The payload of SET_SPEED.
struct MovementSetSpeed {
    // The kind and the unit.
    MovementEventHeader h;
    // The speed.
    int speed;
};

// The payload of SET_TARGET and DESTROY_TARGET.
struct MovementTarget {
    // The kind and the unit the target moves, or 0.
    MovementEventHeader h;
    // The target, as its block offset (0 for none).
    unsigned int target;
};

// The payload of SET_ALTITUDE, SET_APPROACH_RADIUS and SET_HEADING.
struct MovementTargetValue {
    // The kind and the unit the target moves, or 0.
    MovementEventHeader h;
    // The target, as its block offset.
    unsigned int target;
    // The altitude, the radius or the heading.
    int value;
};

// The payload of ORDER_FLAGS.
struct MovementOrderFlags {
    // The kind and the unit moved.
    MovementEventHeader h;
    // The order, as its block offset.
    unsigned int order;
    // The flags to set.
    unsigned int flags;
};

// The payload of TURN.
struct MovementTurn {
    // The kind and the unit.
    MovementEventHeader h;
    // The change of heading.
    int delta;
};

// The payload of TILT.
struct MovementTilt {
    // The kind and the unit.
    MovementEventHeader h;
    // The bank.
    int bank;
    // The pitch.
    int pitch;
};

// The payload of ANGLES.
struct MovementAngles {
    // The kind and the unit.
    MovementEventHeader h;
    // The bank.
    int bank;
    // The heading.
    int heading;
    // The pitch.
    int pitch;
};

// The payload of MOVE.
struct MovementMove {
    // The kind and the unit.
    MovementEventHeader h;
    // The position, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
};

// The payload of MOVED.
struct MovementMoved {
    // The kind and the unit.
    MovementEventHeader h;
    // 1 when the unit has moved, 0 when not.
    int moved;
};

// The payload of CHANGE_CELL.
struct MovementChangeCell {
    // The kind and the unit.
    MovementEventHeader h;
    // The position, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
    // The cell: x and y.
    int cell[2];
    // The flight mode.
    int mode;
};

// The payload of MOVE_RATE.
struct MovementMoveRate {
    // The kind and the unit.
    MovementEventHeader h;
    // The move rate, 0 (still) to 3.
    int rate;
};

// The payload of SET_POSITION.
struct MovementSetPosition {
    // The kind and the unit.
    MovementEventHeader h;
    // The position, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
    // The flight mode the unit is placed in.
    int mode;
};

// The payload of SET_STATE_BITS.
struct MovementSetStateBits {
    // The kind and the unit.
    MovementEventHeader h;
    // The state bits to change.
    int mask;
    // 1 to set them, 0 to clear them.
    int set;
};

// The scripts a START_SCRIPT effect starts.
enum MovementScript {
    // StartMoving.
    MOVEMENT_SCRIPT_START_MOVING = 0,
    // StopMoving.
    MOVEMENT_SCRIPT_STOP_MOVING = 1,
    // MoveRate1.
    MOVEMENT_SCRIPT_MOVE_RATE_1 = 2,
    // MoveRate2.
    MOVEMENT_SCRIPT_MOVE_RATE_2 = 3,
    // MoveRate3.
    MOVEMENT_SCRIPT_MOVE_RATE_3 = 4
};

// The payload of START_SCRIPT.
struct MovementStartScript {
    // The kind and the unit whose script starts.
    MovementEventHeader h;
    // The script (MovementScript).
    int script;
};

// Unit motion, its goals and the targets the goals steer to. A target is named by its block offset, a handle the caller keeps and never reads through.
struct MovementSeam {
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one. Hooks: none.
    int (__stdcall* Apply)(const MovementEventHeader* event, unsigned int size);
    // Steers a unit, moves it and updates its move rate for one tick, in the unit's update; the engine calls it for a unit with a movement after its orders. Hooks: none.
    void (__stdcall* UpdateMotion)(Unit* unit);
    // Applies an event of a path search, its payload of size bytes, to its requester, with the waypoints Poll copied for it; the engine's delivery of pathfinding's results; an event smaller than its payload changes nothing: a started search sets the claim tick of the goal and a status flag on the order, a finished one gives the goal its waypoints. Hooks: none.
    void (__stdcall* ApplyPathEvent)(const PathfindingEventHeader* event, unsigned int size, const PathPoint* waypoints);
    // Fills the state of a unit's movement that a saved game keeps, outside the tick; a query. Hooks: none.
    void (__stdcall* GetMotionState)(Unit* unit, MovementSavedState* out);
    // Reads the network state of a unit's movement from a packet, through the goal that moves it. Hooks: none.
    void (__stdcall* ReadNetState)(Unit* unit, BitReader* reader);
    // Returns nonzero when the goal of a unit's movement has network state to send; a query. Hooks: none.
    int (__stdcall* HasNetState)(Unit* unit);
    // Writes the network state of the goal of a unit's movement to a packet; a view read. Hooks: none.
    void (__stdcall* WriteNetState)(Unit* unit, BitWriter* stream);
    // Builds a move target at a position for an order (its block offset) and returns it; synchronous: the answer is the new target. Hooks: none.
    unsigned int (__stdcall* NewPathOrderAt)(unsigned int order, const Vec3* pos);
    // Builds a move target that follows a unit for an order and returns it; synchronous: the answer is the new target. Hooks: none.
    unsigned int (__stdcall* NewPathOrderToUnit)(unsigned int order, Unit* unit);
    // Builds a move target at a position near a unit for an order and returns it; synchronous: the answer is the new target. Hooks: none.
    unsigned int (__stdcall* NewPathOrderToUnitAt)(unsigned int order, Unit* unit, const Vec3* pos);
    // Builds a move target that follows a unit at an angle for an order and returns it; synchronous: the answer is the new target. Hooks: none.
    unsigned int (__stdcall* NewPathOrderToUnitAtAngle)(unsigned int order, Unit* unit, short angle);
    // Builds an air maneuver target from two positions for an order and returns it; synchronous: the answer is the new target. Hooks: none.
    unsigned int (__stdcall* NewAirManeuverOrder)(unsigned int order, const Vec3* from, const Vec3* to);
    // Builds a disc target for an order and returns it; synchronous: the answer is the new target. Hooks: none.
    unsigned int (__stdcall* NewApproachRadius)(unsigned int order, int x, int y, int radius);
    // Builds a ring target for an order and returns it; synchronous: the answer is the new target. Hooks: none.
    unsigned int (__stdcall* NewRingApproach)(unsigned int order, int x, int y, int inner, int outer);
    // Builds a footprint marker target for an order and returns it; synchronous: the answer is the new target. Hooks: none.
    unsigned int (__stdcall* NewPointMarker)(unsigned int order, const Point16* pos, const Point16* size);
    // Reads a target of a kind (as TargetType returns it, 2 to 6) for an order from a saved game bank under a name and returns it, or 0 for another kind, outside the tick. Hooks: none.
    unsigned int (__stdcall* LoadTarget)(int type, unsigned int order, HapiBank* file, const char* name);
    // Writes a target for an order to a saved game bank under a name, outside the tick. Hooks: none.
    void (__stdcall* SaveTarget)(unsigned int target, unsigned int order, HapiBank* file, const char* name);
    // Returns the kind of a target, 0 for none: 1 a plain target, 2 a move target, 3 an air maneuver target, 4 a disc, 5 a ring, 6 a footprint marker; a query. Hooks: none.
    int (__stdcall* TargetType)(unsigned int target);
    // Returns the target the goal of a unit's movement steers to, or 0, also for a unit with no movement; a query. Hooks: none.
    unsigned int (__stdcall* UnitTarget)(Unit* unit);
    // Writes movement's state as records of the trace, which the engine dumps and does not hash. Hooks: none.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
