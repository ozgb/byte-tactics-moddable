#ifndef INTERFACES_NETWORK_H
#define INTERFACES_NETWORK_H

struct Unit;
struct Player;
struct Vec3;
struct UnitWeaponSlot;
struct WeaponDef;
struct Projectile;

// Major version of the network interface, changed when a slot is removed or its signature changes.
#define NETWORK_VERSION_MAJOR 1
// Minor version of the network interface, changed when a slot is added.
#define NETWORK_VERSION_MINOR 0

// The messages the simulation sends to the other players of a net game. Every slot reads its payload from the game state when it is called, and sends nothing outside a net game or from a player that is not local or computer-controlled.
struct NetworkSeam {
    // Sends every connected player's setup record and alliance to the remote humans, then flushes the queue. Hooks: none.
    void (__stdcall* BroadcastPlayerInfo)(void);
    // Announces a new unit (type, id, position, angles) from its owner. Hooks: none.
    void (__stdcall* SendNewUnit)(Unit* unit);
    // Announces that a builder constructs a unit, from the builder's owner. Hooks: none.
    void (__stdcall* SendBuilderLink)(Unit* builder, Unit* constructed);
    // Announces a unit's death (wreckage level 0 to 15, kill type 0 to 15) with its last attacker, from its owner. Hooks: none.
    void (__stdcall* SendUnitKilled)(Unit* unit, int severity, int wreckageLevel, int killType);
    // Announces damage dealt to a unit of a remote human, from the attacker's owner or, with no attacker, from the local player. Hooks: none.
    void (__stdcall* SendUnitDamage)(Unit* source, Unit* target, int amount, int kind, unsigned short extra);
    // Announces that a unit is attached to a piece of a carrier (or detached, with no carrier), from the local player. Hooks: none.
    void (__stdcall* SendAttach)(Unit* unit, Unit* carrier, char piece, char mode);
    // Hands a unit over to the remote human who owns the new player, with its build progress, health and stockpiles, from the unit's owner. Hooks: none.
    void (__stdcall* SendOwnershipTransfer)(Unit* unit, Player* newOwner);
    // Announces a unit's activation flags, from its owner. Hooks: none.
    void (__stdcall* SendActivateFlags)(Unit* unit);
    // Announces a weapon firing from a unit with the weapon, slot, target, origin and aim or velocity, from the shooter's owner. Hooks: none.
    void (__stdcall* SendWeaponFire)(Unit* shooter, UnitWeaponSlot* slot, Unit* target, Vec3* origin, Vec3* aimOrVel);
    // Announces a shot of a weapon from a position that has no shooter, from the local player. Hooks: none.
    void (__stdcall* SendWeaponFireAt)(unsigned char weaponIndex, Vec3* origin, Vec3* aimOrVel);
    // Announces that a projectile was detonated by an interceptor, from the owner of the attacker. Hooks: none.
    void (__stdcall* SendProjectileDetonated)(Unit* attacker, Projectile* projectile);
    // Tells the host that a weapon hit the feature on a cell, from the local player. Hooks: none.
    void (__stdcall* SendFeatureHit)(WeaponDef* weapon, int x, int z);
    // Announces that the feature on a cell was destroyed, from the local player. Hooks: none.
    void (__stdcall* SendFeatureKilled)(int x, int z);
    // Announces that the feature on a cell caught fire, from the local player. Hooks: none.
    void (__stdcall* SendFeatureBurning)(int x, int z);
    // Announces that a unit reclaimed the feature on a cell, from the unit's owner. Hooks: none.
    void (__stdcall* SendFeatureReclaimed)(Unit* reclaimer, int x, int z);
    // Calls the unit's script function by index on the remote humans, with up to four arguments, and returns 0 when nothing was sent. Hooks: none.
    int (__stdcall* SendScriptCall)(Unit* unit, short index, char argCount, int arg0, int arg1, int arg2, int arg3);
    // Calls the unit's script function by name on the remote humans, with up to four arguments, and returns 0 when nothing was sent. Hooks: none.
    int (__stdcall* SendScriptCallByName)(Unit* unit, char* name, char argCount, int arg0, int arg1, int arg2, int arg3);
    // Sends a gift of resource kind 1 (called metal by the sharing code) from slot from to the remote human in slot to. Hooks: none.
    void (__stdcall* SendShareMetal)(unsigned char from, unsigned char to, float amount);
    // Sends a gift of resource kind 2 (called energy by the sharing code) from slot from to the remote human in slot to. Hooks: none.
    void (__stdcall* SendShareEnergy)(unsigned char from, unsigned char to, float amount);
    // Tells the remote human in slot to that the player in slot from shares the map with them (a gift of kind 3 with no amount). Hooks: none.
    void (__stdcall* SendShareMapInfo)(unsigned char from, unsigned char to);
    // Sends a player's kills, losses, resource amounts and flow to every remote human who watches. Hooks: none.
    void (__stdcall* BroadcastPlayerEconomy)(Player* player);
    // Builds and sends the bit stream of a player's unit states (the motion of each unit that has one, and one full unit) to the remote humans. Hooks: none.
    void (__stdcall* SendUnitStates)(Player* owner);
};

#endif
