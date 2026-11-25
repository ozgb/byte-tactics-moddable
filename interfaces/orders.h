#ifndef INTERFACES_ORDERS_H
#define INTERFACES_ORDERS_H

class HapiBank;
struct Unit;
struct Vec3;
struct StateWriter;

// Major version of the orders interface, changed when a slot is removed or its signature changes.
#define ORDERS_VERSION_MAJOR 6
// Minor version of the orders interface, changed when a slot is added.
#define ORDERS_VERSION_MINOR 1

// Order flag: the order has a target unit.
#define ORDER_FLAG_TARGET 0x200
// Order flag: the order has a position.
#define ORDER_FLAG_POSITION 0x400
// Order flag: the order announces its status once when it starts.
#define ORDER_FLAG_ANNOUNCE 0x2000
// Order flag: a unit with this first order counts as idle.
#define ORDER_FLAG_IDLE 0x20000
// Order flag: the order goes in the second order list of its unit.
#define ORDER_FLAG_SECOND_LIST 0x40000
// Order flag: a unit with this first order raises no under attack line.
#define ORDER_FLAG_NO_UNDERATTACK_LINE 0x80

// The fields every orders event and effect starts with.
struct OrdersEventHeader {
    // A kind of event or of effect.
    int kind;
    // The unit the kind is about, as its block offset (an offset from the block base, 0 for none).
    unsigned int unit;
};

// The kinds of event other modules publish to orders' inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects one, which then changes nothing and has outcome 0, when it is smaller than its payload, names a null unit (but STATUS_FLAGS, which names an order), or its kind is unknown.
enum OrdersEventKind {
    // unit gets an order of a kind against a target unit or a position (OrdersAdd); outcome 1.
    ORDERS_ADD = 288,
    // The count of a unit type queued in unit's build list changes, by the type's name (OrdersQueueBuild); outcome 1.
    ORDERS_QUEUE_BUILD = 289,
    // unit's orders are deleted (OrdersStop); outcome 1.
    ORDERS_STOP = 290,
    // unit's orders that do not stay are dropped and it waits to be carried by its carrier; the header alone; outcome 1.
    ORDERS_BE_CARRIED = 291,
    // Status flags are set on an order, as the unit it listens to changes or the target that moves its unit reports progress (OrdersStatusFlags); outcome 1, or 0 for no order.
    ORDERS_STATUS_FLAGS = 292,
    // unit was damaged by an attacker (OrdersAttacked): a unit of an active player of type 1 or 2 that can return fire attacks it, or aims its free weapons at it; outcome 1.
    ORDERS_ATTACKED = 293,
    // unit is paralysed for more ticks (OrdersParalyze): when its first order is a paralysis, that order lasts longer, else a paralysis order is added after its orders; outcome 1.
    ORDERS_PARALYZE = 294,
    // The player commands unit (OrdersAdd): with queue 1, an order of the same type against the same target, at the same position give or take 16 world units, is deleted from unit's orders when it has one, and is otherwise added as ADD adds it; outcome 1. Hooks: AllowCommand.
    ORDERS_COMMAND = 295,
    // unit's first order of an order type is deleted (OrdersCancel); outcome 1, or 0 when it has none.
    ORDERS_CANCEL = 296
};

// The kinds of effect orders publishes to its outbox, for the modules that subscribe to them; each is a plain request, and the subscriber decides what it does with it.
enum OrdersEffectKind {
    // unit's state bits change (OrdersSetStateBits).
    ORDERS_EFFECT_SET_STATE_BITS = 304,
    // unit is placed at a position (OrdersSetPosition).
    ORDERS_EFFECT_SET_POSITION = 305,
    // unit is attached to a piece of another unit, or detached from it (OrdersAttachToPiece).
    ORDERS_EFFECT_ATTACH_TO_PIECE = 306,
    // unit, which a builder built, is finished (OrdersFinishConstruction).
    ORDERS_EFFECT_FINISH_CONSTRUCTION = 307,
    // unit, unfinished and left alone, decays for a number of ticks (OrdersBuildDecay).
    ORDERS_EFFECT_BUILD_DECAY = 308,
    // unit is repaired by a builder (OrdersRepair).
    ORDERS_EFFECT_REPAIR = 309,
    // A reference an order holds to a unit is unlinked from that unit (OrdersUnlink).
    ORDERS_EFFECT_UNLINK = 310,
    // A script of unit starts (OrdersStartScript).
    ORDERS_EFFECT_START_SCRIPT = 311,
    // unit is busy until a tick (OrdersBusy).
    ORDERS_EFFECT_BUSY = 312,
    // unit's flag word changes under two masks (OrdersSetFlags).
    ORDERS_EFFECT_SET_FLAGS = 313,
    // Network dirty flags of unit are cleared (OrdersClearNetDirty).
    ORDERS_EFFECT_CLEAR_NET_DIRTY = 314,
    // unit takes its movement stance, its fire stance or its group (OrdersSetStance).
    ORDERS_EFFECT_SET_STANCE = 315,
    // unit's angles are set (OrdersSetAngles).
    ORDERS_EFFECT_SET_ANGLES = 316,
    // unit's build progress and health are set (OrdersSetBuildState).
    ORDERS_EFFECT_SET_BUILD_STATE = 317
};

// The payload of ADD and COMMAND.
struct OrdersAdd {
    // The kind and the unit ordered.
    OrdersEventHeader h;
    // The order's type, as GetOrderType returns it.
    int type;
    // 0 to replace the unit's orders that are not marked to stay, 1 to queue it behind them.
    int queue;
    // The target unit, as its block offset (0 for none).
    unsigned int target;
    // 1 when pos holds the order's position.
    int hasPos;
    // The order's position, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
    // The order's two arguments.
    int params[2];
};

// The payload of QUEUE_BUILD.
struct OrdersQueueBuild {
    // The kind and the factory unit.
    OrdersEventHeader h;
    // How many more to queue, or, below 0, how many fewer.
    int count;
    // The name of the unit type, or of a weapon to stockpile (MAKENUKE, MAKEANTI), zero-terminated.
    char name[32];
};

// The payload of STOP.
struct OrdersStop {
    // The kind and the unit.
    OrdersEventHeader h;
    // 1 to delete every order, 0 only the ones not marked to stay.
    int all;
};

// The payload of CANCEL.
struct OrdersCancel {
    // The kind and the unit.
    OrdersEventHeader h;
    // The order type, as GetOrderType returns it, truncated to a byte.
    int order;
};

// The payload of STATUS_FLAGS.
struct OrdersStatusFlags {
    // The kind and the unit of the order, or 0.
    OrdersEventHeader h;
    // The order, as its block offset.
    unsigned int order;
    // The flags to set.
    unsigned int flags;
};

// The payload of ATTACKED.
struct OrdersAttacked {
    // The kind and the unit damaged.
    OrdersEventHeader h;
    // The attacking unit, as its block offset (0 for none).
    unsigned int attacker;
};

// The payload of PARALYZE.
struct OrdersParalyze {
    // The kind and the unit.
    OrdersEventHeader h;
    // The ticks of paralysis added.
    int ticks;
};

// The payload of SET_STATE_BITS.
struct OrdersSetStateBits {
    // The kind and the unit.
    OrdersEventHeader h;
    // The state bits to change.
    int mask;
    // 1 to set them, 0 to clear them.
    int set;
};

// The payload of SET_POSITION.
struct OrdersSetPosition {
    // The kind and the unit.
    OrdersEventHeader h;
    // The position, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
    // The flight mode the unit is placed in.
    int mode;
};

// The payload of ATTACH_TO_PIECE.
struct OrdersAttachToPiece {
    // The kind and the unit attached.
    OrdersEventHeader h;
    // The unit carrying it, as its block offset (0 for none).
    unsigned int target;
    // The piece of the carrying unit.
    int piece;
    // How the unit is attached.
    int mode;
};

// The payload of FINISH_CONSTRUCTION.
struct OrdersFinishConstruction {
    // The kind and the unit finished.
    OrdersEventHeader h;
    // The building unit, as its block offset.
    unsigned int builder;
};

// The payload of BUILD_DECAY.
struct OrdersBuildDecay {
    // The kind and the unit.
    OrdersEventHeader h;
    // The ticks of decay.
    int ticks;
};

// The payload of REPAIR.
struct OrdersRepair {
    // The kind and the unit repaired.
    OrdersEventHeader h;
    // The building unit, as its block offset.
    unsigned int builder;
    // The repair progress added.
    float amount;
};

// The payload of UNLINK.
struct OrdersUnlink {
    // The kind and the unit the reference points at.
    OrdersEventHeader h;
    // The reference, as its block offset.
    unsigned int ref;
};

// The payload of BUSY.
struct OrdersBusy {
    // The kind and the unit.
    OrdersEventHeader h;
    // The tick until which the unit is busy.
    int until;
};

// The payload of SET_FLAGS.
struct OrdersSetFlags {
    // The kind and the unit.
    OrdersEventHeader h;
    // The flag bits cleared first.
    unsigned int clear;
    // The flag bits set after.
    unsigned int set;
};

// The payload of CLEAR_NET_DIRTY.
struct OrdersClearNetDirty {
    // The kind and the unit.
    OrdersEventHeader h;
    // The dirty bits cleared.
    unsigned int mask;
};

// The fields of a SET_STANCE effect.
enum OrdersStanceField {
    // The movement stance.
    ORDERS_STANCE_MOVE = 1,
    // The fire stance.
    ORDERS_STANCE_FIRE = 2,
    // The group.
    ORDERS_STANCE_GROUP = 4
};

// The payload of SET_STANCE.
struct OrdersSetStance {
    // The kind and the unit.
    OrdersEventHeader h;
    // The fields set (OrdersStanceField bits).
    int fields;
    // The movement stance.
    int moveOrder;
    // The fire stance.
    int fireOrder;
    // The group.
    int group;
};

// The payload of SET_ANGLES.
struct OrdersSetAngles {
    // The kind and the unit.
    OrdersEventHeader h;
    // The bank.
    int bank;
    // The heading.
    int heading;
    // The pitch.
    int pitch;
};

// The payload of SET_BUILD_STATE.
struct OrdersSetBuildState {
    // The kind and the unit.
    OrdersEventHeader h;
    // The part of the unit still to build, 0 when finished.
    float buildLeft;
    // The health.
    int health;
};

// The forms of a START_SCRIPT effect.
enum OrdersScriptForm {
    // The script by name, with no arguments.
    ORDERS_SCRIPT_BY_NAME = 0,
    // The script by name, with count arguments.
    ORDERS_SCRIPT_BY_NAME_WITH_ARGS = 1,
    // The script by its index in the unit's script, with count arguments.
    ORDERS_SCRIPT_BY_INDEX_WITH_ARGS = 2
};

// The payload of START_SCRIPT.
struct OrdersStartScript {
    // The kind and the unit whose script starts.
    OrdersEventHeader h;
    // The form (OrdersScriptForm).
    int form;
    // The script's index, for BY_INDEX_WITH_ARGS.
    int index;
    // The script's name, zero-terminated, for the forms by name.
    char name[32];
    // The callback told when the script returns, as its block offset (0 for none).
    unsigned int callback;
    // The update argument of the start.
    int update;
    // The count of arguments.
    int count;
    // The arguments.
    int args[4];
};

// Unit orders and command queues.
struct OrdersSeam {
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one. Hooks: none.
    int (__stdcall* Apply)(const OrdersEventHeader* event, unsigned int size);
    // Runs the first order of a unit for one tick, in the unit's update; the engine calls it for a unit of an active player of type 1 or 2. Hooks: AllowBuild, AircraftSeekRepair, GuardResponse, IdleBuilderAction.
    void (__stdcall* RunOrders)(Unit* unit);
    // Runs the secondary orders of a unit for one tick, after RunOrders. Hooks: none.
    void (__stdcall* RunSecondaryOrders)(Unit* unit);
    // Returns the flags of the first order of a unit, or 0 when it has none; a query. Hooks: none.
    int (__stdcall* GetOrderFlags)(Unit* unit);
    // Returns the index in the order type table of the kind of order a unit gets against a target unit or position for a relation mode; a query. Hooks: AllowCommand.
    int (__stdcall* GetOrderType)(unsigned char mode, Unit* unit, Unit* target, Vec3* pos);
    // Returns the index of the order kind a name in the order type table stands for; a query. Hooks: none.
    int (__stdcall* MissionTypeFromName)(const char* name);
    // Registers the order types of every module in the order type table, outside the tick. Hooks: none.
    void (__stdcall* RegisterAllOrderTypes)(void);
    // Empties the order type table, outside the tick. Hooks: none.
    void (__stdcall* ClearOrderTypeTable)(void);
    // Reads count orders of a unit from a saved game bank, gives them to the unit and lets its first order take back its effects, outside the tick. Hooks: none.
    void (__stdcall* LoadOrders)(Unit* unit, HapiBank* file, int count);
    // Writes the orders of a unit to a saved game bank and returns how many it wrote, outside the tick. Hooks: none.
    int (__stdcall* SaveOrders)(Unit* unit, HapiBank* file);
    // Writes orders' state as records of the trace, which the engine dumps and does not hash. Hooks: none.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
