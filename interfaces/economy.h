#ifndef INTERFACES_ECONOMY_H
#define INTERFACES_ECONOMY_H

#include "state_writer.h"

// Major version of the economy interface, changed when a slot is removed or its signature changes.
#define ECONOMY_VERSION_MAJOR 4
// Minor version of the economy interface, changed when a slot is added.
#define ECONOMY_VERSION_MINOR 0

// A unit, as an opaque handle the world view gives (0 for none, which names a player's own account in an event).
typedef unsigned int EconUnit;
// A unit type, as its number in the match's table of unit types.
typedef unsigned int EconType;

// The resources an event or a query is about, as bits.
enum EconResource {
    // Energy.
    ECON_ENERGY = 1,
    // Metal.
    ECON_METAL = 2
};

// One resource of an account, as the six figures a tick settles.
struct EconLedger {
    // What it produced since the last settlement.
    float make;
    // What it asked for since the last settlement.
    float use;
    // What of use was granted.
    float usePaid;
    // The shortfall carried into this tick; a request is refused while it is above 0.
    float stall;
    // What it produced up to the last settlement.
    float makePrev;
    // What it asked for up to the last settlement.
    float usePrev;
};

// The resource account of a unit or of a player, in the order a saved game stores it.
struct EconAccount {
    // Energy.
    EconLedger energy;
    // Metal.
    EconLedger metal;
};

// The kinds of event other modules publish to economy's inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published.
enum EconomyEventKind {
    // Queued: clears an account and gives it to player; outcome 1.
    ECONOMY_RESET = 0,
    // Queued: sets a unit's account to account, read from a saved game; outcome 1.
    ECONOMY_LOAD = 1,
    // Queued: a unit asks for the energy (and metal) of this tick's work, which is charged as asked; outcome 1 when it is granted, as RequestGranted said.
    ECONOMY_REQUEST = 2,
    // Not queued: a call to the synchronous Spend slot, which takes energy and metal from the store of the account's player and books it as used; outcome 1, or 0 when the store is short and nothing changes.
    ECONOMY_SPEND = 3,
    // Queued: adds income to an account; the implementation decides what the account's player is given; outcome 1.
    ECONOMY_INCOME = 4
};

// The kinds of effect economy publishes to its outbox, for the modules that subscribe to them.
enum EconomyEffectKind {
    // Synchronous: a cloaked unit's upkeep was or was not paid this tick, value 1 to keep it cloaked and 0 to uncloak it; delivered before Emit returns, since sensors read it in the same player's update.
    ECONOMY_EFFECT_CLOAK = 16,
    // A metal extractor's rate changed: value is the metal under its footprint, which its script is told.
    ECONOMY_EFFECT_EXTRACTOR = 17,
    // Gives resources of player to player value, as a share between allies: energy or metal as resources says.
    ECONOMY_EFFECT_SHARE = 18
};

// An event (EconomyEventKind) or an effect (EconomyEffectKind), as plain data with no pointers; fields an event does not use are 0.
struct EconomyEvent {
    // An EconomyEventKind or an EconomyEffectKind.
    int kind;
    // The player whose account it is (0 to 9), or who gives in a share.
    int player;
    // The unit whose account it is, or 0 for the player's own account.
    EconUnit unit;
    // The EconResource bits it is about.
    unsigned int resources;
    // The energy amount.
    float energy;
    // The metal amount.
    float metal;
    // The effect's argument, as its kind says.
    int value;
    // The account a LOAD sets; 0 for the other kinds.
    EconAccount account;
};

// The map a match starts on.
struct EconMapInfo {
    // The number of unit slots.
    int unitSlots;
    // The number of players.
    int playerCount;
};

// What economy reads of a unit.
struct EconUnitInfo {
    // Its player (0 to 9), the one whose account it is.
    int player;
    // Its type.
    EconType type;
    // Whether it is a structure, which is switched on and off, not a unit that moves.
    int structure;
    // Whether it is switched on.
    int active;
    // Whether it is set to cloak.
    int cloakOn;
    // Whether it may not cloak now.
    int cloakBlocked;
    // Whether it moved this tick.
    int moving;
    // The tick from which a cloaked unit pays its upkeep again.
    unsigned int workTime;
    // How much of it is left to build, from 0 (finished) to 1.
    float buildLeft;
    // The metal an extractor makes per tick, as UpdateMetalExtraction last set it.
    float extraction;
    // Its cell, the top left of its footprint.
    int cellX, cellY;
    // The size of its footprint, in cells.
    int footprintX, footprintY;
    // Whether it has a script.
    int hasScript;
};

// What economy reads of a unit type.
struct EconTypeInfo {
    // Energy produced per tick.
    float energyMake;
    // Energy used per tick when switched on.
    float energyUse;
    // Metal produced per tick.
    float metalMake;
    // Metal extracted per tick per unit of metal under the footprint.
    float extractsMetal;
    // Energy made per unit of wind.
    float windGenerator;
    // Energy made per unit of tide.
    float tidalGenerator;
    // Energy used per tick to stay cloaked.
    float cloakCost;
    // Energy used per tick to stay cloaked while moving.
    float cloakCostMoving;
    // Energy storage it adds.
    float energyStorage;
    // Metal storage it adds.
    float metalStorage;
    // Whether it turns energy into metal.
    int makesMetal;
};

// What economy reads of a player.
struct EconPlayerInfo {
    // Whether the slot is in the match.
    int active;
    // How the player is controlled: 1 local, 2 computer, 3 remote.
    int controller;
    // The player's number as the slot records it; 10 for none.
    int index;
    // Whether the player's setup says it is a human: the kind of player that takes a share.
    int human;
    // Whether the player shares metal, energy and its map with allies, from its setup.
    int shareMetal, shareEnergy, shareMapping;
    // Whether the player is still in the game: it has units, or never had any.
    int inGame;
    // Whether the player's storage bonus counts.
    int storage;
};

// The world as economy sees it, read only apart from Emit, as of each module's last run; given by the engine.
struct EconomyWorldView {
    // The tick being played.
    unsigned int (__stdcall* Tick)(void);
    // The difficulty the computer players are set to, 0 easiest.
    int (__stdcall* Difficulty)(void);
    // Whether the match is a network game that shares resources between allies.
    int (__stdcall* SharingEnabled)(void);
    // The wind's strength, in the units windGenerator multiplies.
    float (__stdcall* Wind)(void);
    // The tide's strength, in the units tidalGenerator multiplies.
    float (__stdcall* Tide)(void);
    // Fills a player's info.
    void (__stdcall* PlayerInfo)(int player, EconPlayerInfo* out);
    // Whether two players are allied, as from sees it.
    int (__stdcall* Allied)(int from, int to);
    // The first unit slot of a player.
    EconUnit (__stdcall* FirstUnit)(int player);
    // The last unit slot of a player.
    EconUnit (__stdcall* LastUnit)(int player);
    // The unit slot after a slot.
    EconUnit (__stdcall* NextUnit)(EconUnit unit);
    // The index of a unit's slot, from 0 to unitSlots - 1.
    int (__stdcall* UnitSlot)(EconUnit unit);
    // Fills a unit's info; returns 0 when the unit is not in play, which settlement skips.
    int (__stdcall* UnitInfo)(EconUnit unit, EconUnitInfo* out);
    // Fills a unit type's info.
    void (__stdcall* TypeInfo)(EconType type, EconTypeInfo* out);
    // The metal of a map cell, or -1 when the cell is off the map.
    int (__stdcall* CellMetal)(int x, int y);
    // Publishes an effect to economy's outbox; the router delivers it to its subscribers at the next phase boundary, or before Emit returns for a kind its table marks synchronous.
    void (__stdcall* Emit)(const EconomyEvent* effect);
};

// Resources: the players' stores and the accounts of players and units. The engine delivers economy's pending inbox, through Apply, before each run (UpdatePlayerEconomy, UpdateMetalExtraction, UpdateResourceSharing) and each synchronous slot, and at each phase boundary.
struct EconomySeam {
    // The bytes of private state the implementation needs for a map; called before Init.
    unsigned int (__stdcall* RegionSize)(const EconMapInfo* map);
    // Builds the state for a match in a region the engine allocated of the size RegionSize gave; called once per match, before the first tick and before any event.
    void (__stdcall* Init)(const EconMapInfo* map, const EconomyWorldView* world, void* region, unsigned int size);
    // Ends the match and returns the region, which the engine frees.
    void* (__stdcall* Shutdown)(void);
    // Consumes one queued event of the inbox and returns its outcome, 0 for a refused intent, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const EconomyEvent* event);
    // Synchronous: applies a spend (an event of kind ECONOMY_SPEND) after the engine delivered the pending inbox, and returns its outcome, visible at once. Hooks: none.
    int (__stdcall* Spend)(const EconomyEvent* spend);
    // Settles one player's units for a tick, in the players phase: charges their upkeep, adds their income and emits their effects. Hooks: ResourceIncome, StatsAccumulate.
    void (__stdcall* UpdatePlayerEconomy)(int player);
    // Sets a metal extractor's rate from the metal under its footprint and emits its effect; called when the unit is created or loaded. Hooks: none.
    void (__stdcall* UpdateMetalExtraction)(EconUnit unit);
    // Shares one player's surplus with its allies, by effects, in the tick's tail of a network game. Hooks: none.
    void (__stdcall* UpdateResourceSharing)(int player);
    // Synchronous: whether a request for the given resources by a unit would be granted now, after the engine delivered the pending inbox; a query, called just before the REQUEST it decides. Hooks: none.
    int (__stdcall* RequestGranted)(EconUnit unit, unsigned int resources);
    // Returns the energy a unit type uses per tick, or minus what it makes from wind or tide, as the exact product, not rounded to 32 bits; a query. Hooks: none.
    double (__stdcall* GetEnergyUse)(EconType type);
    // Fills a unit's account as it stands, for a saved game or a display; a query. Hooks: none.
    void (__stdcall* UnitAccount)(EconUnit unit, EconAccount* out);
    // Writes the state that decides what the implementation does next and that the engine does not already hash elsewhere; it reads only, and keeps nothing.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
