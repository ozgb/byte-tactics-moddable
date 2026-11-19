#ifndef INTERFACES_WEAPONS_H
#define INTERFACES_WEAPONS_H

struct Unit;
struct Vec3;
struct WeaponDef;
struct Projectile;
struct StateWriter;

// Major version of the weapons interface, changed when a slot is removed or its signature changes.
#define WEAPONS_VERSION_MAJOR 2
// Minor version of the weapons interface, changed when a slot is added.
#define WEAPONS_VERSION_MINOR 0

// The kinds of damage event other modules publish to weapons' inbox: every kind of damage to a unit, a weapon's or not, enters weapons here; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects one, which then changes nothing and has outcome 0, when it names a null unit.
enum WeaponsDamageEventKind {
    // unit takes value points of damage of the kind extra (1 a weapon, 2 a paralyser, 10 a repair that heals, 11 the water, others the cause of a death) from target (0 for none), hit from the direction args[0]; unless the kind is 10, the amount is first scaled by unit's armour (its damage modifier while it is closed, and its experience), and the damage the unit takes is announced to the other machines when unit's player is remote; outcome 1.
    WEAPONS_DAMAGE = 256,
    // unit takes value points of damage of the kind extra from target (0 for none), hit from the direction args[0], as another machine already computed and announced it; outcome 1.
    WEAPONS_REMOTE_DAMAGE = 257
};

// The kinds of effect weapons publishes to its outbox when a unit takes damage, in the order given; a unit that is not alive or is already dying takes none, and a repair (kind 10) only HEALTH.
enum WeaponsDamageEffectKind {
    // unit was damaged by target (0 when no unit attacked it) for value points, by damage of any kind but repair (10) and 11.
    WEAPONS_EFFECT_ATTACKED = 272,
    // unit was damaged, of any kind but repair, for its health bar; published before ATTACKED.
    WEAPONS_EFFECT_MARK_DAMAGED = 274,
    // unit's last damage was of the kind value, and when target is not 0, target and target's player are unit's last attacker; published after ATTACKED.
    WEAPONS_EFFECT_DAMAGE_RECORD = 275,
    // unit's health becomes value, and when extra is 1 unit is marked to die (a unit of a player who plays on this machine whose health fell to 0 or below).
    WEAPONS_EFFECT_HEALTH = 276,
    // unit is paralysed for value more ticks: when its first order is a paralysis, that order lasts value ticks longer, else a paralysis order of value ticks is added after its orders.
    WEAPONS_EFFECT_PARALYZE = 277
};

// The kinds of event other modules publish to weapons' inbox about weapons, projectiles and explosions; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects one, which then changes nothing and has outcome 0, when: it names a null unit (but SPAWN and AREA_DAMAGE, which name none); RELEASE or CLAIM names a weapon outside 0 to 3, LOAD_SLOT or ADD_STOCKPILE one outside 0 to 2; AIM_DONE names no callback; SPAWN or AREA_DAMAGE names a weapon outside the table (SPAWN may name -1, no weapon); the kind is unknown.
enum WeaponsEventKind {
    // unit's death weapon explodes at its position: its self-destruct weapon when value is 1, the weapon it explodes as when 0; outcome 1.
    WEAPONS_DETONATE = 258,
    // unit's weapon value (0 to 2, or 3 for all three) is released from its orders, to choose its own targets; a weapon that was claimed has its target cleared; outcome 1.
    WEAPONS_RELEASE = 259,
    // unit's weapon value (0 to 2, or 3 for all three) is claimed by its orders, which choose its targets; a weapon that was released has its target cleared; outcome 1.
    WEAPONS_CLAIM = 260,
    // unit's three weapon slots are set up from its unit type: each slot's weapon, a reload of 0, an empty stockpile and the offset of its aim; then unit's script is told the longest reload of its weapons; outcome 1.
    WEAPONS_INIT_SLOTS = 261,
    // The projectiles unit fired are removed; outcome 1.
    WEAPONS_REMOVE_PROJECTILES = 262,
    // The aim script that a START_SCRIPT effect started with callback returned value: a weapon whose aim returned nonzero is aimed; outcome 1.
    WEAPONS_AIM_DONE = 263,
    // Piece value of unit's model is blown off with the flags extra, as the explode opcode of a unit's script does: the piece flies, or shatters into debris, and the flags 0x100 to 0x2000 add explosions at it; outcome 1.
    WEAPONS_EXPLODE_PIECE = 264,
    // The weapon value (its index in the table of weapons) deals its area damage at pos, to the units around it; outcome 1.
    WEAPONS_AREA_DAMAGE = 265,
    // A projectile of the weapon value (-1 for none) starts at pos with the velocity vel, announced to the other machines when extra is 1; outcome 1, or 0 when the table of projectiles is full.
    WEAPONS_SPAWN = 266,
    // Each of unit's three weapons that stockpiles (flag bit 1) holds the stockpile args[0] to args[2], as a unit captured by another player keeps them; outcome 1.
    WEAPONS_SET_STOCKPILES = 267,
    // unit's weapon slot value takes the state of a saved game: args[0] whether its aim is done, args[1] a byte of its weapon type that the save keeps, args[2] the offset of its aim, args[3] its reload, args[4] and args[5] its heading and pitch, args[6] its stockpile and args[7] its flag bits (bits 0, 1, 2 to 3 and 4); outcome 1.
    WEAPONS_LOAD_SLOT = 268,
    // unit's weapon value (0 to 2) holds one more round in its stockpile, as a stockpile weapon's build order finishes one; outcome 1.
    WEAPONS_ADD_STOCKPILE = 269
};

// The kinds of effect weapons publishes to its outbox about weapons, projectiles and explosions, for the modules that subscribe to them; each is a plain request, and the subscriber decides what it does with it.
enum WeaponsEffectKind {
    // A shot of unit hit, dealing value damage to units of other players and extra damage to units of unit's own player.
    WEAPONS_EFFECT_HIT = 273,
    // The script name of unit starts with the callback (0 for none) and the flag update; when extra is 1 it is given the value arguments of args, as a script started with arguments; when 0 it takes none.
    WEAPONS_EFFECT_START_SCRIPT = 278,
    // Piece value of unit's model is hidden, as the piece flies off; when extra is 1 the model is first posed from its angles and animation.
    WEAPONS_EFFECT_DETACH_PIECE = 279,
    // unit's network notice bits value (bits 10 to 12 of its dirty flags: a shot fired, a command shot fired, a target out of reach) are set.
    WEAPONS_EFFECT_NOTICE = 280,
    // unit is busy, as for its idle animation, until the tick value.
    WEAPONS_EFFECT_BUSY = 281
};

// An event (WeaponsDamageEventKind, WeaponsEventKind) or an effect (WeaponsDamageEffectKind, WeaponsEffectKind), as plain data with no pointers; fields a kind does not use are 0.
struct WeaponsEvent {
    // A kind of event or of effect.
    int kind;
    // The unit the kind is about, as its block offset (an offset from the block base, 0 for none), as economy's events give a unit.
    unsigned int unit;
    // The other unit, as its block offset: the source of DAMAGE, the attacker of ATTACKED and DAMAGE_RECORD.
    unsigned int target;
    // The argument of the kind, as the kind says.
    int value;
    // The second argument of the kind, as the kind says.
    int extra;
    // The aim callback of a weapon, as its block offset (0 for none): the one a START_SCRIPT effect passes and an AIM_DONE event names.
    unsigned int callback;
    // A position in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
    // A velocity in world units per tick with 16 fractional bits: x, y and z.
    int vel[3];
    // The further arguments of the kind, as the kind says.
    int args[8];
    // The name of a script, ended by a 0.
    char name[32];
};

// Weapons, projectiles and explosions, and the damage rules for every kind of damage to a unit. The engine delivers weapons' pending inbox, through Apply, before each run (UpdateProjectiles, UpdateExplosions, UpdateUnitWeapons), and at each phase boundary.
struct WeaponsSeam {
    // Consumes one queued event of the inbox and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const WeaponsEvent* event);
    // Moves every projectile and resolves its hits for one tick.
    void (__stdcall* UpdateProjectiles)(void);
    // Advances every explosion, exploded piece and piece of debris for one tick.
    void (__stdcall* UpdateExplosions)(void);
    // Aims and fires a unit's weapons for one tick, in the unit's update; the engine calls it for a unit of an active player of type 1 or 2. Hooks: none.
    void (__stdcall* UpdateUnitWeapons)(Unit* unit);
    // Finds a weapon definition by its TDF name, or null; a query. Hooks: none.
    WeaponDef* (__stdcall* FindWeaponByName)(char* name);
    // Returns the range of one of a unit's weapons; a view read. Hooks: none.
    int (__stdcall* GetWeaponRange)(Unit* unit, unsigned int weapon);
    // Returns whether one of a unit's weapons can reach another unit; a query. Hooks: none.
    int (__stdcall* WeaponCanReachUnit)(Unit* shooter, Unit* target, unsigned char weapon);
    // Returns the projectile of a unit's weapon that can be targeted, or null; a query. Hooks: none.
    Projectile* (__stdcall* FindTargetableProjectile)(Unit* unit, unsigned int weapon);
    // Returns whether a unit's weapon can reach a position from a position; a query. Hooks: none.
    int (__stdcall* WeaponCanReachPos)(Unit* unit, Vec3* from, Vec3* to, int flags);
    // Allocates the table of projectiles, outside the tick. Hooks: none.
    void (__stdcall* AllocWeaponArray)(void);
    // Frees the table of projectiles, outside the tick. Hooks: none.
    void (__stdcall* FreeWeaponArray)(void);
    // Loads the weapon definitions from the weapon TDF files, outside the tick. Hooks: none.
    void (__stdcall* LoadWeaponTypes)(void);
    // Frees the weapon definitions, outside the tick. Hooks: none.
    void (__stdcall* FreeWeaponTypes)(void);
    // Prepares the explosion tables, outside the tick. Hooks: none.
    void (__stdcall* InitExplosions)(void);
    // Frees the explosion tables, outside the tick. Hooks: none.
    void (__stdcall* FreeExplosions)(void);
    // Writes weapons' state as records of the trace, which the engine dumps and does not hash.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
