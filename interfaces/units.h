#ifndef INTERFACES_UNITS_H
#define INTERFACES_UNITS_H

#include "state_writer.h"

namespace std {
template<class T> class allocator;
template<class T, class A> class vector;
}

struct Unit;
struct UnitDef;
struct Vec3;
class HapiBank;
class OrderBase;
class UnitMotion;
class UnitRef;
class UnitTypeSet;

// Major version of the units interface, changed when a slot is removed or its signature changes.
#define UNITS_VERSION_MAJOR 4
// Minor version of the units interface, changed when a slot is added.
#define UNITS_VERSION_MINOR 0

// Unit simulation.
struct UnitsSeam {
    // Advances every unit for one tick.
    void (__stdcall* UpdateAllUnits)(void);
    // Sets or clears the bits of a unit's state given by a mask (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* SetStateBits)(Unit* unit, int mask, int set);
    // Marks a unit as recently damaged, for its health bar. Hooks: none.
    void (__stdcall* MarkRecentlyDamaged)(Unit* unit);
    // Notifies everything that holds a reference to a unit of an event. Hooks: none.
    void (__stdcall* NotifyUnitRefs)(Unit* unit, int event);
    // Adds a unit type, or the unit types of a category, to a set by name, and writes 1 to out for a type and 0 for a category (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* UnitTypeSetAddTypeOrCategory)(UnitTypeSet* self, char* text, int* out);
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
    // Kills a unit, with a cause. Hooks: none.
    void (__stdcall* KillUnit)(Unit* unit, int cause);
    // Reads the units of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadUnits)(HapiBank* file);
    // Writes the units of a saved game to a bank. Hooks: none.
    void (__stdcall* SaveUnits)(HapiBank* file);
    // Reads one unit of a saved game from a bank and returns it. Hooks: none.
    Unit* (__stdcall* LoadUnit)(unsigned short id, HapiBank* file);
    // Creates a unit of a type for a player at a position and returns it, or 0. Hooks: none.
    Unit* (__stdcall* CreateUnit)(unsigned char player, unsigned short typeId, Vec3 pos, int param_4, int mode, int param_6);
    // Returns the unit type of a name, or 0 when there is none. Hooks: none.
    UnitDef* (__stdcall* FindUnitType)(const char* name);
    // Returns the index of the unit type of a name, or 0 when there is none. Hooks: none.
    unsigned short (__stdcall* FindUnitTypeId)(const char* name);
    // Looks a mission unit up by its name among the units made so far. Hooks: none.
    int (__stdcall* FindMissionUnit)(char* name, std::vector<Unit*, std::allocator<Unit*> >* units, int value);
    // Attaches a unit to a piece of another unit, or detaches it when the target is 0. Hooks: none.
    void (__stdcall* AttachUnitToPiece)(Unit* unit, Unit* target, char piece, char mode);
    // Adds build progress to a unit under construction and returns 1 when it is done. Hooks: none.
    int (__stdcall* AddBuildProgress)(Unit* builder, Unit* unit, float amount);
    // Adds repair progress to a unit and returns 1 when it is done. Hooks: none.
    int (__stdcall* AddRepairProgress)(Unit* builder, Unit* unit, float amount);
    // Lets a unit that stopped being built lose some of its progress. Hooks: none.
    void (__stdcall* ApplyUnfinishedBuildDecay)(Unit* unit, int ticks);
    // Completes the unit a builder was building. Hooks: none.
    void (__stdcall* FinishConstruction)(Unit* builder, Unit* unit);
    // Moves a unit to a position and returns its new flags. Hooks: none.
    int (__stdcall* SetUnitPosition)(Unit* unit, Vec3 pos, int param);
    // Returns 1 when a unit can load a target (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* CanLoad)(Unit* unit, int unusedEdx, Unit* target);
    // Returns 1 when a unit can reclaim a target (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* CanReclaim)(Unit* unit, int unusedEdx, Unit* target);
    // Returns 1 when a unit can repair a target (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* CanRepair)(Unit* unit, int unusedEdx, Unit* target);
    // Unlinks a reference from the list of the unit it points at (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* UnitRefUnlink)(UnitRef* ref);
    // Starts the script that tells a unit's script whether the unit is in the water, on the ground or in the air, when that changed (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* UpdateSfxOccupy)(UnitMotion* motion, int unusedEdx, Unit* unit);
    // Starts one unit's update for one tick, before its weapons: the count of units updated and its wind generator; UpdateAllUnits calls it for each unit of each player. Hooks: none.
    void (__stdcall* UpdateUnitHead)(Unit* unit);
    // Finishes one unit's update for one tick, after its weapons: its scripts, timers, health percentage, water damage, self-repair, orders, motion and a pending death. Hooks: none.
    void (__stdcall* UpdateUnitBody)(Unit* unit);
    // Appends to out the factories of a player within a radius of a position that its last RefreshFactoryIndex found and that still have a pad (unit type flags 0x40 and 0x200) and are switched on. Hooks: none.
    void (__stdcall* GetFactoriesInRadius)(int player, const Vec3* pos, int radius, std::vector<Unit*, std::allocator<Unit*> >* out);
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
    // Writes the state that decides what the implementation does next and that the engine does not already hash elsewhere; it reads only, and keeps nothing.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
