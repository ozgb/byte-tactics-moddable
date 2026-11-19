#ifndef INTERFACES_SCRIPTS_H
#define INTERFACES_SCRIPTS_H

struct Unit;
struct ModelState;
struct Object3do;
struct UnitDrawView;
struct ScriptTable;
struct Callback;
struct Vec3;
struct Angles16;
class CobScript;
class UnitScript;
class HapiBank;

// Major version of the scripts interface, changed when a slot is removed or its signature changes.
#define SCRIPTS_VERSION_MAJOR 1
// Minor version of the scripts interface, changed when a slot is added.
#define SCRIPTS_VERSION_MINOR 0

// Unit scripts and piece state.
struct ScriptsSeam {
    // Starts a named script with a callback and returns its thread, or a negative number (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* StartScript)(CobScript* script, const char* name, Callback* callback, int update);
    // Starts a named script with up to four arguments and returns its thread, or a negative number (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* StartScriptWithArgs)(CobScript* script, char* name, Callback* callback, int update, int count, int a, int b, int c, int d);
    // Starts a script by the index of its name with up to four arguments and returns its thread (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* StartScriptWithArgsByIndex)(CobScript* script, int index, Callback* callback, int update, int count, int a, int b, int c, int d);
    // Runs a named script that only answers a query and returns its result (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* QueryScript)(CobScript* script, char* name, int* p2, int* p3, int* p4, int* p5);
    // Returns the index of a named script of a unit, or a negative number when it has none (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* FindScript)(CobScript* script, const char* name);
    // Returns the offset of a piece of a unit from its position. Hooks: none.
    Vec3 (__stdcall* GetPieceOffset)(Unit* unit, int piece);
    // Returns the world position of a piece of a unit. Hooks: none.
    Vec3 (__stdcall* GetPiecePosition)(Unit* unit, int piece);
    // Returns the angles of a piece of a unit. Hooks: none.
    Angles16 (__stdcall* GetPieceAngles)(Unit* unit, int piece);
    // Writes the world position of the nano piece of a unit. Hooks: none.
    void (__stdcall* GetNanoPiecePosition)(Unit* unit, Vec3* out);
    // Writes the world position a unit's weapon aims from. Hooks: none.
    void (__stdcall* GetAimFromPosition)(Unit* unit, Vec3* out, unsigned char weapon);
    // Writes the position of a unit's sweet spot, the point weapons aim at. Hooks: none.
    void (__stdcall* GetSweetSpot)(Unit* unit, Vec3* out);
    // Poses a unit's model from its current angles and animation, so piece reads depend on game state only. Hooks: none.
    void (__stdcall* PoseUnitModel)(Unit* unit);
    // Builds the piece state of a 3DO model, in the state block. Hooks: none.
    ModelState* (__stdcall* CreateObjectState)(Object3do* model);
    // Frees the piece state of a model and the pictures cached for it. Hooks: none.
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
    void (__stdcall* SetObjectState)(UnitScript* script, int state);
    // Runs the threads of a script for a number of elapsed ticks (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* RunScripts)(CobScript* script, int elapsed);
    // Writes the threads of a script to a bank of a saved game (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* SaveScriptState)(CobScript* script, HapiBank* file);
    // Reads the threads of a script from a bank of a saved game and returns 1 when it could (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* LoadScriptState)(CobScript* script, HapiBank* file);
    // Destroys a unit script and frees its memory (the virtual deleting destructor). Hooks: none.
    void (__stdcall* DeleteScript)(CobScript* script);
    // Fills the piece state of a model that the caller allocated, in the state block. Hooks: none.
    void (__stdcall* InitObjectState)(ModelState* state, Object3do* model);
};

#endif
