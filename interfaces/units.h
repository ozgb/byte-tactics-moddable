#ifndef INTERFACES_UNITS_H
#define INTERFACES_UNITS_H

#include "state_writer.h"

template <class T> struct Ref;
struct MapMoveLimits;
struct Unit;
struct UnitDef;
struct Vec3;
class HapiBank;
class OrderBase;
class UnitRef;
class UnitTypeSet;

// Major version of the units interface, changed when a slot is removed or its signature changes.
#define UNITS_VERSION_MAJOR 7
// Minor version of the units interface, changed when a slot is added.
#define UNITS_VERSION_MINOR 2

// The fields every units event starts with.
struct UnitsEventHeader {
    // A kind of event.
    int kind;
    // The unit the kind is about, as its block offset (an offset from the block base, 0 for none).
    unsigned int unit;
};

// The kinds of event other modules publish to units' inbox, in two blocks (352 to 367 and 384 to 398); each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects one, which then changes nothing and has outcome 0, when it is smaller than its payload, names a null unit (but UNLINK, which names a reference, and CREATE, KILL_ALL and RELOAD_TYPE, which name none), names a player outside 0 to 9 (SEND, CAPTURE), or its kind is unknown.
enum UnitsEventKind {
    // unit's state bits under a mask are set or cleared, which starts the scripts that announce the change (UnitsSetStateBits); outcome 1.
    UNITS_SET_STATE_BITS = 352,
    // unit's flag word changes under two masks (UnitsSetFlags); outcome 1.
    UNITS_SET_FLAGS = 353,
    // unit's network dirty bits change under two masks (UnitsSetNetDirty); outcome 1.
    UNITS_SET_NET_DIRTY = 354,
    // unit is busy until a tick (UnitsSetWorkTime); outcome 1.
    UNITS_SET_WORK_TIME = 355,
    // unit is placed at a position, with what placing it on the map does (UnitsSetPosition); outcome 1.
    UNITS_SET_POSITION = 356,
    // unit is attached to a piece of a target unit, or detached when the target is 0 (UnitsAttachToPiece); outcome 1.
    UNITS_ATTACH_TO_PIECE = 357,
    // unit, which a builder built, is finished (UnitsFinishConstruction); outcome 1. Hooks: UnitFinished.
    UNITS_FINISH_CONSTRUCTION = 358,
    // unit, unfinished and left alone, decays for a number of ticks (UnitsBuildDecay); outcome 1.
    UNITS_BUILD_DECAY = 359,
    // unit is repaired by a builder (UnitsRepair); outcome 1. Hooks: RepairAmount.
    UNITS_REPAIR = 360,
    // A reference an order holds to a unit is unlinked from that unit (UnitsUnlink); the header's unit is the unit the reference points at, which Apply does not read; outcome 1, or 0 for no reference.
    UNITS_UNLINK = 361,
    // unit takes its movement stance, its fire stance or its group (UnitsSetStance); outcome 1.
    UNITS_SET_STANCE = 362,
    // Some of unit's angles are set (UnitsSetAngles); outcome 1.
    UNITS_SET_ANGLES = 363,
    // unit's build progress and health are set (UnitsSetBuildState); outcome 1.
    UNITS_SET_BUILD_STATE = 364,
    // unit's heading turns, and unit has moved (UnitsTurn); outcome 1.
    UNITS_TURN = 365,
    // unit moves to a position, and has moved (UnitsMove); outcome 1.
    UNITS_MOVE = 366,
    // unit moves to a position in another map cell, in a movement mode (UnitsChangeCell); outcome 1.
    UNITS_CHANGE_CELL = 367,
    // unit has moved this tick, or not (UnitsSetMoved); outcome 1.
    UNITS_SET_MOVED = 384,
    // unit's script is told whether unit is in the water, on the ground or in the air, when that changed; the header alone; outcome 1. Hooks: UnitWaterState.
    UNITS_SFX_OCCUPY = 385,
    // A script of unit sets one of its unit's values, and marks unit dirty for the network (UnitsScriptValue); outcome 1.
    UNITS_SCRIPT_VALUE = 386,
    // Everything that holds a reference to unit is notified of an event (UnitsNotifyRefs); outcome 1.
    UNITS_NOTIFY_REFS = 387,
    // unit was attacked, and says so unless its first order or its attacker keeps it quiet; the header alone; outcome 1.
    UNITS_ATTACKED_SPEECH = 388,
    // unit is marked as recently damaged, for its health bar; the header alone; outcome 1.
    UNITS_MARK_DAMAGED = 389,
    // unit keeps the kind of the damage it last took and its attacker (UnitsDamageRecord); outcome 1.
    UNITS_DAMAGE_RECORD = 390,
    // unit's health is set, and unit is marked dying (UnitsSetHealth); outcome 1.
    UNITS_SET_HEALTH = 391,
    // unit is killed by its player, or self-destructs (UnitsKill); outcome 1. Hooks: UnitDestroyed.
    UNITS_KILL = 392,
    // unit is sent to another player (UnitsSend); outcome 1.
    UNITS_SEND = 393,
    // unit is captured by another player, who gets a copy of it (UnitsCapture); outcome 1, or 0 when no copy could be made. Hooks: UnitCreated.
    UNITS_CAPTURE = 394,
    // A unit of a type is created for a player at a position, finished and placed as the console creates it (UnitsCreate); outcome 1 when a unit was made, 0 when none was.
    UNITS_CREATE = 395,
    // Every unit that exists is killed, one by one in the order of the unit table; the header alone; outcome 1.
    UNITS_KILL_ALL = 396,
    // Every unit of a type is killed and the definition of the type is loaded again (UnitsReloadType); outcome 1.
    UNITS_RELOAD_TYPE = 397,
    // unit forgets who attacked it last: its last attacker slot is reset, its dying flag (bit 14 of Unit.flags, 0x4000) is set and its attacker is cleared; the header alone; outcome 1.
    UNITS_FORGET_ATTACKER = 398
};

// The payload of SET_STATE_BITS.
struct UnitsSetStateBits {
    // The kind and the unit.
    UnitsEventHeader h;
    // The state bits changed.
    int mask;
    // 1 to set them, 0 to clear them.
    int set;
};

// The payload of SET_FLAGS.
struct UnitsSetFlags {
    // The kind and the unit.
    UnitsEventHeader h;
    // The flag bits cleared first.
    unsigned int clear;
    // The flag bits set after.
    unsigned int set;
};

// The payload of SET_NET_DIRTY.
struct UnitsSetNetDirty {
    // The kind and the unit.
    UnitsEventHeader h;
    // The dirty bits cleared first.
    unsigned int clear;
    // The dirty bits set after.
    unsigned int set;
};

// The payload of SET_WORK_TIME.
struct UnitsSetWorkTime {
    // The kind and the unit.
    UnitsEventHeader h;
    // The tick the unit is busy until.
    int until;
};

// The payload of SET_POSITION.
struct UnitsSetPosition {
    // The kind and the unit.
    UnitsEventHeader h;
    // The position, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
    // How the unit is placed, as the placing code reads it.
    int mode;
};

// The payload of ATTACH_TO_PIECE.
struct UnitsAttachToPiece {
    // The kind and the unit attached or detached.
    UnitsEventHeader h;
    // The unit it is attached to, as its block offset (0 to detach it).
    unsigned int target;
    // The target's piece, or -1.
    int piece;
    // How it is attached or detached.
    int mode;
};

// The payload of FINISH_CONSTRUCTION.
struct UnitsFinishConstruction {
    // The kind and the unit finished.
    UnitsEventHeader h;
    // The builder, as its block offset.
    unsigned int builder;
};

// The payload of BUILD_DECAY.
struct UnitsBuildDecay {
    // The kind and the unit.
    UnitsEventHeader h;
    // The ticks of decay.
    int ticks;
};

// The payload of REPAIR.
struct UnitsRepair {
    // The kind and the unit repaired.
    UnitsEventHeader h;
    // The builder, as its block offset.
    unsigned int builder;
    // The repair progress added.
    float amount;
};

// The payload of UNLINK.
struct UnitsUnlink {
    // The kind and the unit the reference points at.
    UnitsEventHeader h;
    // The reference, as its block offset.
    unsigned int ref;
};

// The fields of a SET_STANCE event.
enum UnitsStanceField {
    // The movement stance.
    UNITS_STANCE_MOVE = 1,
    // The fire stance.
    UNITS_STANCE_FIRE = 2,
    // The group.
    UNITS_STANCE_GROUP = 4
};

// The payload of SET_STANCE.
struct UnitsSetStance {
    // The kind and the unit.
    UnitsEventHeader h;
    // The fields set (UnitsStanceField bits).
    int fields;
    // The movement stance.
    int moveOrder;
    // The fire stance.
    int fireOrder;
    // The group.
    int group;
};

// The angles of a SET_ANGLES event.
enum UnitsAngleField {
    // The bank.
    UNITS_ANGLE_BANK = 1,
    // The heading.
    UNITS_ANGLE_HEADING = 2,
    // The pitch.
    UNITS_ANGLE_PITCH = 4
};

// The payload of SET_ANGLES.
struct UnitsSetAngles {
    // The kind and the unit.
    UnitsEventHeader h;
    // The angles set (UnitsAngleField bits).
    int fields;
    // The bank, in 16-bit angle units.
    int bank;
    // The heading, in 16-bit angle units.
    int heading;
    // The pitch, in 16-bit angle units.
    int pitch;
};

// The payload of SET_BUILD_STATE.
struct UnitsSetBuildState {
    // The kind and the unit.
    UnitsEventHeader h;
    // The build progress left.
    float buildLeft;
    // The health.
    int health;
};

// The payload of TURN.
struct UnitsTurn {
    // The kind and the unit.
    UnitsEventHeader h;
    // The change of heading, in 16-bit angle units.
    int delta;
};

// The payload of MOVE.
struct UnitsMove {
    // The kind and the unit.
    UnitsEventHeader h;
    // The position, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
};

// The payload of CHANGE_CELL.
struct UnitsChangeCell {
    // The kind and the unit.
    UnitsEventHeader h;
    // The position, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
    // The map cell: x and y.
    int cell[2];
    // The movement mode.
    int mode;
};

// The payload of SET_MOVED.
struct UnitsSetMoved {
    // The kind and the unit.
    UnitsEventHeader h;
    // 1 when the unit has moved this tick, else 0.
    int moved;
};

// The payload of SCRIPT_VALUE.
struct UnitsScriptValue {
    // The kind and the unit.
    UnitsEventHeader h;
    // The value set, by its script number: 1 and 20 are state bits, 5, 6 and 19 the unit's script flags; any other only marks the unit dirty.
    int which;
    // The new value.
    int value;
};

// The payload of NOTIFY_REFS.
struct UnitsNotifyRefs {
    // The kind and the unit.
    UnitsEventHeader h;
    // The event the references are told of.
    int event;
};

// The payload of DAMAGE_RECORD.
struct UnitsDamageRecord {
    // The kind and the unit damaged.
    UnitsEventHeader h;
    // The kind of damage.
    int damageKind;
    // The attacker, as its block offset (0 for none, which leaves the unit's attacker as it was).
    unsigned int attacker;
};

// The payload of SET_HEALTH.
struct UnitsSetHealth {
    // The kind and the unit.
    UnitsEventHeader h;
    // The health.
    int health;
    // 1 when the unit is dying, else 0.
    int dying;
};

// The payload of KILL.
struct UnitsKill {
    // The kind and the unit.
    UnitsEventHeader h;
    // 1 when the unit self-destructs, 0 when its player kills it.
    int selfDestruct;
};

// The payload of CREATE.
struct UnitsCreate {
    // The kind; the unit is 0.
    UnitsEventHeader h;
    // The index of the player who owns the unit, truncated to a byte.
    int player;
    // The unit type, by its number in the match's table of unit types, truncated to 16 bits.
    int type;
    // The position, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
};

// The payload of RELOAD_TYPE.
struct UnitsReloadType {
    // The kind; the unit is 0.
    UnitsEventHeader h;
    // The unit type, by its number in the match's table of unit types.
    int type;
};

// The payload of SEND.
struct UnitsSend {
    // The kind and the unit.
    UnitsEventHeader h;
    // The index of the player the unit is sent to.
    int player;
};

// The payload of CAPTURE.
struct UnitsCapture {
    // The kind and the unit captured.
    UnitsEventHeader h;
    // The index of the player who captures it.
    int player;
    // 1 when the fields below describe the copy, as another machine sent them; 0 to copy the unit's own.
    int present;
    // The copy's health.
    int hitPoints;
    // The copy's build progress left.
    int buildPercent;
    // The copy's angles: bank, heading and pitch.
    int angles[3];
    // The copy's stockpiles of its three weapons.
    int stockpiles[3];
};

// Unit simulation.
struct UnitsSeam {
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one. Hooks: none.
    int (__stdcall* Apply)(const UnitsEventHeader* event, unsigned int size);
    // Advances every unit for one tick. Hooks: GameFrame.
    void (__stdcall* UpdateAllUnits)(void);
    // Adds a unit type, or the unit types of a category, to a set by name, and writes 1 to out for a type and 0 for a category (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* UnitTypeSetAddTypeOrCategory)(UnitTypeSet* self, const char* text, int* out);
    // Allocates the memory of the unit table. Hooks: none.
    void (__stdcall* AllocateUnitMemory)(void);
    // Frees the memory of the unit table. Hooks: none.
    void (__stdcall* FreeUnitMemory)(void);
    // Frees the unit type information read from the unit files. Hooks: none.
    void (__stdcall* FreeUnitInfo)(void);
    // Reads the unit type information again from the unit files. Hooks: none.
    void (__stdcall* RefreshUnitInfo)(void);
    // Loads every unit type. Hooks: none.
    void (__stdcall* LoadUnitTypes)(void);
    // Frees every unit type. Hooks: none.
    void (__stdcall* FreeUnitTypes)(void);
    // Loads the build menus of the unit types. Hooks: none.
    void (__stdcall* LoadDownloadMenus)(void);
    // Frees the build menus of the unit types. Hooks: none.
    void (__stdcall* FreeDownloadMenus)(void);
    // Frees the movement classes. Hooks: none.
    void (__stdcall* FreeMovementClasses)(void);
    // Builds the table of unit categories. Hooks: none.
    void (__stdcall* InitUnitCategories)(void);
    // Frees the table of unit categories. Hooks: none.
    void (__stdcall* FreeUnitCategories)(void);
    // Kills every unit. Hooks: none.
    void (__stdcall* KillAllUnits)(void);
    // Kills every unit of a type. Hooks: none.
    void (__stdcall* KillUnitsOfType)(short id);
    // Loads a unit type again from its file. Hooks: none.
    void (__stdcall* ReloadUnitType)(unsigned short id);
    // Kills a unit, with a cause; synchronous, outside the tick (the engine's shutdown kills every unit). Hooks: UnitDestroyed.
    void (__stdcall* KillUnit)(Unit* unit, int cause);
    // Reads the units of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadUnits)(HapiBank* file);
    // Writes the units of a saved game to a bank. Hooks: none.
    void (__stdcall* SaveUnits)(HapiBank* file);
    // Reads one unit of a saved game from a bank and returns it; synchronous: the caller reads the unit in the same statement. Hooks: none.
    Unit* (__stdcall* LoadUnit)(unsigned short id, HapiBank* file);
    // Creates a unit of a type for a player at a position and returns it, or 0; finished is 1 for a built unit and 0 for one under construction, and id is the unit slot to take or 0 for the first free one; synchronous: the caller reads the new unit in the same statement. Hooks: UnitCreated.
    Unit* (__stdcall* CreateUnit)(unsigned char player, unsigned short typeId, const Vec3* pos, int finished, int mode, int id);
    // Returns the unit type of a name, or 0 when there is none. Hooks: none.
    UnitDef* (__stdcall* FindUnitType)(const char* name);
    // Returns the index of the unit type of a name, or 0 when there is none. Hooks: none.
    unsigned short (__stdcall* FindUnitTypeId)(const char* name);
    // Looks a mission unit up by its name in the caller's table of the units made for the mission units (count entries, indexed by mission unit, and at most the mission's CountMissionUnits; a larger count is clamped to it), starting after the unit after when it is not null, and returns it or null; a count of 0 reads no entry, so the table may then be null. Hooks: none.
    Unit* (__stdcall* FindMissionUnit)(const char* name, const Ref<Unit>* units, int count, Unit* after);
    // Adds build progress to a unit under construction and returns 1 when the progress was paid for (a positive amount that the economy granted), else 0; synchronous: the builder's order reads the answer in the same statement. Hooks: UnitFinished.
    int (__stdcall* AddBuildProgress)(Unit* builder, Unit* unit, float amount);
    // Adds repair progress to a unit and returns 1 when it is done; synchronous: the builder's order reads the answer in the same statement. Hooks: RepairAmount.
    int (__stdcall* AddRepairProgress)(Unit* builder, Unit* unit, float amount);
    // Returns 1 when a unit can load a target. Hooks: none.
    int (__stdcall* CanLoad)(Unit* unit, Unit* target);
    // Returns 1 when a unit can reclaim a target. Hooks: none.
    int (__stdcall* CanReclaim)(Unit* unit, Unit* target);
    // Returns 1 when a unit can repair a target. Hooks: none.
    int (__stdcall* CanRepair)(Unit* unit, Unit* target);
    // Unlinks a reference from the list of the unit it points at. Hooks: none.
    void (__stdcall* UnitRefUnlink)(UnitRef* ref);
    // Starts one unit's update for one tick, before its weapons: the count of units updated and its wind generator; UpdateAllUnits calls it for each unit of each player. Hooks: none.
    void (__stdcall* UpdateUnitHead)(Unit* unit);
    // Continues one unit's update for one tick, after its weapons and its scripts: its timers, health percentage, water damage and self-repair; returns 1 when the unit's player is active and of type 1 or 2, so that its orders and its movement run next. Hooks: SelfHeal.
    int (__stdcall* UpdateUnitBody)(Unit* unit);
    // Finishes one unit's update for one tick, after its orders and its movement: its height when moved is 1 (its movement ran), then a pending death. Hooks: UnitDestroyed.
    void (__stdcall* UpdateUnitTail)(Unit* unit, int moved);
    // Writes to out, as Ref<Unit>, the first capacity of the factories of a player within a radius of a position that its last RefreshFactoryIndex found and that still have a pad (unit type flags 0x40 and 0x200) and are switched on, and returns how many there are. Hooks: none.
    int (__stdcall* GetFactoriesInRadius)(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity);
    // Creates the empty index of a player's factories, refreshed by the next RefreshFactoryIndex, outside the tick. Hooks: none.
    void (__stdcall* CreateFactoryIndex)(int player);
    // Frees the index of a player's factories, outside the tick. Hooks: none.
    void (__stdcall* FreeFactoryIndex)(int player);
    // Every 30 ticks of a player that has an index, refills it with the player's ready, finished units that have a pad (unit type flags 0x40 and 0x200) and are switched on. Hooks: none.
    void (__stdcall* RefreshFactoryIndex)(int player);
    // Builds the reference an order holds to a unit in memory the caller owns, links it to the unit when it has one and gives it the order that listens to the unit. Hooks: none.
    void (__stdcall* UnitRefInit)(UnitRef* ref, Unit* unit, OrderBase* value);
    // Points a reference at another unit, or at none, and links it into the list of that unit (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* UnitRefSetUnit)(UnitRef* ref, Unit* unit);
    // Returns the unit a reference points at, or null; a view read. Hooks: none.
    Unit* (__stdcall* UnitRefGet)(UnitRef* ref);
    // Gives a reference the order that listens to the unit it points at. Hooks: none.
    void (__stdcall* UnitRefSetValue)(UnitRef* ref, OrderBase* value);
    // Writes the state that decides what the implementation does next and that the engine does not already hash elsewhere; it reads only, and keeps nothing. Hooks: none.
    void (__stdcall* WriteState)(const StateWriter* writer);
    // Writes to out the footprint and limits of each movement class the game data defines, in index order, at most capacity of them, and returns how many it wrote; a query, outside the tick. Hooks: none.
    int (__stdcall* GetMovementLimits)(MapMoveLimits* out, int capacity);
};

#endif
