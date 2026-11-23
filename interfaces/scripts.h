#ifndef INTERFACES_SCRIPTS_H
#define INTERFACES_SCRIPTS_H

struct Unit;
struct ModelState;
struct Object3do;
struct UnitDrawView;
struct ScriptTable;
struct Callback;
struct Vec3;
struct StateWriter;
struct Angles16;
class CobScript;
class UnitScript;
class HapiBank;

// Major version of the scripts interface, changed when a slot is removed or its signature changes.
#define SCRIPTS_VERSION_MAJOR 3
// Minor version of the scripts interface, changed when a slot is added.
#define SCRIPTS_VERSION_MINOR 0

// The fields every scripts event starts with.
struct ScriptsEventHeader {
    // A kind of event.
    int kind;
    // The unit the kind is about, as its block offset (an offset from the block base, 0 for none).
    unsigned int unit;
};

// The kinds of event other modules publish to scripts' inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects one, which then changes nothing and has outcome 0, when it is smaller than its payload, names a null unit (but FREE_STATE and INIT_STATE, which name a piece state), names a null piece state (FREE_STATE, INIT_STATE), or its kind is unknown.
enum ScriptsEventKind {
    // A script of unit starts, and runs until it waits or ends (ScriptsStart); outcome 1, or 0 when unit has no script.
    SCRIPTS_START = 416,
    // A piece of unit's model is detached from it, as the piece flies off (ScriptsDetachPiece); outcome 1.
    SCRIPTS_DETACH_PIECE = 417,
    // A piece state and the pictures cached for it are freed (ScriptsState); outcome 1.
    SCRIPTS_FREE_STATE = 418,
    // A piece state that the publisher allocated is filled from a feature type's model (ScriptsState); outcome 1.
    SCRIPTS_INIT_STATE = 419
};

// How a START event names its script.
enum ScriptsStartForm {
    // By name, with no arguments.
    SCRIPTS_START_BY_NAME = 0,
    // By name, with arguments.
    SCRIPTS_START_BY_NAME_WITH_ARGS = 1,
    // By the index of its name, with arguments.
    SCRIPTS_START_BY_INDEX_WITH_ARGS = 2
};

// The payload of START.
struct ScriptsStart {
    // The kind and the unit whose script starts.
    ScriptsEventHeader h;
    // The form (ScriptsStartForm).
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

// The payload of DETACH_PIECE.
struct ScriptsDetachPiece {
    // The kind and the unit.
    ScriptsEventHeader h;
    // The piece of the unit's model.
    int piece;
    // 1 when the model is first posed from its angles and animation.
    int pose;
};

// The payload of FREE_STATE and INIT_STATE.
struct ScriptsState {
    // The kind; the unit is 0.
    ScriptsEventHeader h;
    // The piece state, as its block offset.
    unsigned int state;
    // The feature type whose model INIT_STATE fills it from (0 for FREE_STATE).
    int feature;
};

// Unit scripts and piece state.
struct ScriptsSeam {
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one. Hooks: none.
    int (__stdcall* Apply)(const ScriptsEventHeader* event, unsigned int size);
    // Starts a script by the index of its name with up to four arguments and returns its thread, for a unit's script that another machine started (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* StartScriptWithArgsByIndex)(CobScript* script, int index, Callback* callback, int update, int count, int a, int b, int c, int d);
    // Runs a named script that answers a query and returns 1, or 0 when it could not start; each non-null argument is passed to the script and receives its value back; synchronous: the script runs at once and the caller reads the answer in the same statement (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* QueryScript)(CobScript* script, const char* name, int* arg0, int* arg1, int* arg2, int* arg3);
    // Returns the index of a named script of a unit, or a negative number when it has none (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* FindScript)(CobScript* script, const char* name);
    // Writes to out the offset of a piece of a unit from its position. Hooks: none.
    void (__stdcall* GetPieceOffset)(Unit* unit, int piece, Vec3* out);
    // Writes to out the world position of a piece of a unit. Hooks: none.
    void (__stdcall* GetPiecePosition)(Unit* unit, int piece, Vec3* out);
    // Writes to out the angles of a piece of a unit. Hooks: none.
    void (__stdcall* GetPieceAngles)(Unit* unit, int piece, Angles16* out);
    // Runs the unit's script that names its nano piece and writes the piece's world position; synchronous: the caller reads the position in the same statement. Hooks: none.
    void (__stdcall* GetNanoPiecePosition)(Unit* unit, Vec3* out);
    // Runs the unit's scripts that name the piece a weapon aims from and writes the piece's world position; synchronous: the caller reads the position in the same statement. Hooks: none.
    void (__stdcall* GetAimFromPosition)(Unit* unit, Vec3* out, unsigned char weapon);
    // Runs the unit's script that names its sweet spot, the point weapons aim at, poses its model and writes the spot's position; synchronous: the caller reads the position in the same statement. Hooks: none.
    void (__stdcall* GetSweetSpot)(Unit* unit, Vec3* out);
    // Poses a unit's model from its current angles and animation, so piece reads depend on game state only. Hooks: none.
    void (__stdcall* PoseUnitModel)(Unit* unit);
    // Builds the piece state of a 3DO model, in the state block. Hooks: none.
    ModelState* (__stdcall* CreateObjectState)(Object3do* model);
    // Frees the piece state of a model and the pictures cached for it; synchronous, as its unit dies. Hooks: none.
    void (__stdcall* FreeObjectState)(ModelState* state);
    // Poses the model of a unit from its current angles and animation when they changed. Hooks: none.
    void (__stdcall* UpdateObjectState)(UnitDrawView* unit);
    // The bytes of the piece state of a 3DO model, which the caller allocates before it publishes the state to be built; a query. Hooks: none.
    unsigned int (__stdcall* ObjectStateSize)(Object3do* model);
    // Allocates the script of a unit and stores it in the unit. Hooks: none.
    void (__stdcall* NewUnitScript)(Unit* unit);
    // Gives a script the compiled script data of a unit type (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* SetCob)(CobScript* script, ScriptTable* data);
    // Builds the piece state of a unit from its model and script data and returns it, in the state block. Hooks: none.
    ModelState* (__stdcall* CreatePlayerObjectState)(Object3do* model, ScriptTable* data, Unit* owner);
    // Gives a unit script the piece state of its unit (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* SetObjectState)(UnitScript* script, ModelState* state);
    // Runs the threads of a script for a number of elapsed ticks (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* RunScripts)(CobScript* script, int elapsed);
    // Writes the threads of a script to a bank of a saved game (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* SaveScriptState)(CobScript* script, HapiBank* file);
    // Reads the threads of a script from a bank of a saved game and returns 1 when it could (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* LoadScriptState)(CobScript* script, HapiBank* file);
    // Destroys a unit script and frees its memory (the virtual deleting destructor); synchronous, as its unit dies. Hooks: none.
    void (__stdcall* DeleteScript)(CobScript* script);
    // Writes the private state of every unit script as records (section 55), for the trace's dump; reads only, changes nothing. Hooks: none.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
