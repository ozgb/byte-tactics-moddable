#ifndef INTERFACES_WEAPONS_H
#define INTERFACES_WEAPONS_H

struct Unit;
struct Vec3;
struct WeaponDef;
struct Projectile;
struct StateWriter;

// Major version of the weapons interface, changed when a slot is removed or its signature changes.
#define WEAPONS_VERSION_MAJOR 3
// Minor version of the weapons interface, changed when a slot is added.
#define WEAPONS_VERSION_MINOR 0

// The fields every weapons event and effect starts with.
struct WeaponsEventHeader {
    // A kind of event or of effect.
    int kind;
    // The unit the kind is about, as its block offset (an offset from the block base, 0 for none: SPAWN, AREA_DAMAGE and AIM_DONE name none), as economy's events give a unit.
    unsigned int unit;
};

// The kinds of damage event other modules publish to weapons' inbox: every kind of damage to a unit, a weapon's or not, enters weapons here; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects one, which then changes nothing and has outcome 0, when it names a null unit or is smaller than its payload.
enum WeaponsDamageEventKind {
    // unit takes damage (WeaponsDamage); unless the kind of damage is 10, the amount is first scaled by unit's armour (its damage modifier while it is closed, and its experience), and the damage the unit takes is announced to the other machines when unit's player is remote; outcome 1.
    WEAPONS_DAMAGE = 256,
    // unit takes damage (WeaponsDamage) as another machine already computed and announced it; outcome 1.
    WEAPONS_REMOTE_DAMAGE = 257
};

// The kinds of effect weapons publishes to its outbox when a unit takes damage, in the order given; a unit that is not alive or is already dying takes none, and a repair (kind 10) only HEALTH.
enum WeaponsDamageEffectKind {
    // unit was damaged (WeaponsAttacked), by damage of any kind but repair (10) and 11.
    WEAPONS_EFFECT_ATTACKED = 272,
    // unit was damaged, of any kind but repair, for its health bar; published before ATTACKED; the header alone.
    WEAPONS_EFFECT_MARK_DAMAGED = 274,
    // unit's last damage and attacker change (WeaponsDamageRecord); published after ATTACKED.
    WEAPONS_EFFECT_DAMAGE_RECORD = 275,
    // unit's health changes (WeaponsHealth).
    WEAPONS_EFFECT_HEALTH = 276,
    // unit is paralysed for more ticks (WeaponsParalyze): when its first order is a paralysis, that order lasts longer, else a paralysis order is added after its orders.
    WEAPONS_EFFECT_PARALYZE = 277
};

// The kinds of event other modules publish to weapons' inbox about weapons, projectiles and explosions; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects one, which then changes nothing and has outcome 0, when: it is smaller than its payload; it names a null unit (but SPAWN, AREA_DAMAGE and AIM_DONE, which name none); RELEASE or CLAIM names a weapon outside 0 to 3, ADD_STOCKPILE one outside 0 to 2; AIM_DONE names no callback; SPAWN or AREA_DAMAGE names a weapon outside the table (SPAWN may name -1, no weapon); the kind is unknown.
enum WeaponsEventKind {
    // unit's death weapon explodes at its position (WeaponsDetonate); outcome 1.
    WEAPONS_DETONATE = 258,
    // unit's weapon is released from its orders, to choose its own targets (WeaponsRelease); a weapon that was claimed has its target cleared; outcome 1.
    WEAPONS_RELEASE = 259,
    // unit's weapon is claimed by its orders, which choose its targets (WeaponsClaim); a weapon that was released has its target cleared; outcome 1.
    WEAPONS_CLAIM = 260,
    // unit's three weapon slots are set up from its unit type: each slot's weapon, a reload of 0, an empty stockpile and the offset of its aim; then unit's script is told the longest reload of its weapons; the header alone; outcome 1.
    WEAPONS_INIT_SLOTS = 261,
    // The projectiles unit fired are removed; the header alone; outcome 1.
    WEAPONS_REMOVE_PROJECTILES = 262,
    // The aim script that a START_SCRIPT effect started returned (WeaponsAimDone): a weapon whose aim returned nonzero is aimed; outcome 1.
    WEAPONS_AIM_DONE = 263,
    // A piece of unit's model is blown off, as the explode opcode of a unit's script does (WeaponsExplodePiece): the piece flies, or shatters into debris, and the flags 0x100 to 0x2000 add explosions at it; outcome 1.
    WEAPONS_EXPLODE_PIECE = 264,
    // A weapon deals its area damage to the units around a position (WeaponsAreaDamage); outcome 1.
    WEAPONS_AREA_DAMAGE = 265,
    // A projectile starts (WeaponsSpawn); outcome 1, or 0 when the table of projectiles is full.
    WEAPONS_SPAWN = 266,
    // Each of unit's three weapons that stockpiles (flag bit 1) holds a stockpile (WeaponsSetStockpiles), as a unit captured by another player keeps them; outcome 1.
    WEAPONS_SET_STOCKPILES = 267,
    // unit's weapon holds one more round in its stockpile, as a stockpile weapon's build order finishes one (WeaponsAddStockpile); outcome 1.
    WEAPONS_ADD_STOCKPILE = 269
};

// The kinds of effect weapons publishes to its outbox about weapons, projectiles and explosions, for the modules that subscribe to them; each is a plain request, and the subscriber decides what it does with it.
enum WeaponsEffectKind {
    // A shot of unit hit (WeaponsHit).
    WEAPONS_EFFECT_HIT = 273,
    // A script of unit starts (WeaponsStartScript).
    WEAPONS_EFFECT_START_SCRIPT = 278,
    // A piece of unit's model is hidden, as the piece flies off (WeaponsDetachPiece).
    WEAPONS_EFFECT_DETACH_PIECE = 279,
    // unit fired a shot, a notice its orders wait on; the header alone.
    WEAPONS_EFFECT_SHOT_FIRED = 280,
    // unit is busy, as for its idle animation, until a tick (WeaponsBusy).
    WEAPONS_EFFECT_BUSY = 281,
    // unit fired a shot of a command-fire weapon, a notice its orders wait on; the header alone.
    WEAPONS_EFFECT_COMMAND_SHOT_FIRED = 282,
    // unit's target is out of the reach of its weapon, a notice its orders wait on; the header alone.
    WEAPONS_EFFECT_OUT_OF_REACH = 283
};

// The scripts of a unit that weapons starts, by a START_SCRIPT effect.
enum WeaponsScript {
    // No script: a script of an empty name, as a weapon of slot 3 starts.
    WEAPONS_SCRIPT_NONE = 0,
    // FirePrimary, when the first weapon fires.
    WEAPONS_SCRIPT_FIRE_PRIMARY = 1,
    // FireSecondary, when the second weapon fires.
    WEAPONS_SCRIPT_FIRE_SECONDARY = 2,
    // FireTertiary, when the third weapon fires.
    WEAPONS_SCRIPT_FIRE_TERTIARY = 3,
    // AimPrimary, to aim the first weapon.
    WEAPONS_SCRIPT_AIM_PRIMARY = 4,
    // AimSecondary, to aim the second weapon.
    WEAPONS_SCRIPT_AIM_SECONDARY = 5,
    // AimTertiary, to aim the third weapon.
    WEAPONS_SCRIPT_AIM_TERTIARY = 6,
    // RockUnit, as a shot rocks the unit that fired it.
    WEAPONS_SCRIPT_ROCK_UNIT = 7,
    // SetMaxReloadTime, the longest reload of the unit's weapons, in milliseconds.
    WEAPONS_SCRIPT_SET_MAX_RELOAD_TIME = 8,
    // HitByWeapon, the direction a hit came from.
    WEAPONS_SCRIPT_HIT_BY_WEAPON = 9,
    // TakeDamage, the share of its health a hit took.
    WEAPONS_SCRIPT_TAKE_DAMAGE = 10
};

// The payload of DAMAGE and REMOTE_DAMAGE: unit takes damage.
struct WeaponsDamage {
    // The kind and the unit that takes the damage.
    WeaponsEventHeader h;
    // The unit that dealt it, as its block offset (0 for none).
    unsigned int source;
    // The points of damage.
    int amount;
    // The kind of damage: 1 a weapon, 2 a paralyser, 10 a repair that heals, 11 the water, others the cause of a death.
    int damageKind;
    // The direction the damage came from, a 16-bit heading.
    int direction;
};

// The payload of DETONATE.
struct WeaponsDetonate {
    // The kind and the unit whose death weapon explodes.
    WeaponsEventHeader h;
    // 1 for the unit's self-destruct weapon, 0 for the weapon it explodes as.
    int selfDestruct;
};

// The payload of RELEASE.
struct WeaponsRelease {
    // The kind and the unit.
    WeaponsEventHeader h;
    // The weapon, 0 to 2, or 3 for all three.
    int weapon;
};

// The payload of CLAIM.
struct WeaponsClaim {
    // The kind and the unit.
    WeaponsEventHeader h;
    // The weapon, 0 to 2, or 3 for all three.
    int weapon;
};

// The payload of AIM_DONE.
struct WeaponsAimDone {
    // The kind; the unit is 0.
    WeaponsEventHeader h;
    // The aim callback of the weapon, as its block offset: the one the START_SCRIPT effect passed.
    unsigned int callback;
    // What the aim script returned.
    int result;
};

// The payload of EXPLODE_PIECE.
struct WeaponsExplodePiece {
    // The kind and the unit whose piece is blown off.
    WeaponsEventHeader h;
    // The piece of the unit's model.
    int piece;
    // The explode flags of the script's opcode.
    unsigned int flags;
};

// The payload of AREA_DAMAGE.
struct WeaponsAreaDamage {
    // The kind; the unit is 0.
    WeaponsEventHeader h;
    // The weapon, as its index in the table of weapons.
    int weapon;
    // The centre of the damage, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
};

// The payload of SPAWN.
struct WeaponsSpawn {
    // The kind; the unit is 0.
    WeaponsEventHeader h;
    // The weapon of the projectile, as its index in the table of weapons, or -1 for none.
    int weapon;
    // The position the projectile starts at, in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
    // The projectile's velocity, in world units per tick with 16 fractional bits: x, y and z.
    int vel[3];
    // 1 when the projectile is announced to the other machines.
    int announce;
};

// The payload of SET_STOCKPILES.
struct WeaponsSetStockpiles {
    // The kind and the unit.
    WeaponsEventHeader h;
    // The stockpile of each of the unit's three weapons.
    int stockpiles[3];
};

// The payload of ADD_STOCKPILE.
struct WeaponsAddStockpile {
    // The kind and the unit.
    WeaponsEventHeader h;
    // The weapon, 0 to 2.
    int weapon;
};

// The payload of ATTACKED.
struct WeaponsAttacked {
    // The kind and the unit that was damaged.
    WeaponsEventHeader h;
    // The unit that attacked it, as its block offset (0 when no unit did).
    unsigned int attacker;
    // The points of damage.
    int amount;
};

// The payload of DAMAGE_RECORD.
struct WeaponsDamageRecord {
    // The kind and the unit that was damaged.
    WeaponsEventHeader h;
    // The kind of the unit's last damage.
    int damageKind;
    // The unit that attacked it, as its block offset; when it is not 0, it and its player become the unit's last attacker.
    unsigned int attacker;
};

// The payload of HEALTH.
struct WeaponsHealth {
    // The kind and the unit.
    WeaponsEventHeader h;
    // The unit's new health.
    int health;
    // 1 when the unit is marked to die (a unit of a player who plays on this machine whose health fell to 0 or below).
    int dying;
};

// The payload of PARALYZE.
struct WeaponsParalyze {
    // The kind and the unit.
    WeaponsEventHeader h;
    // The ticks of paralysis to add.
    int ticks;
};

// The payload of HIT.
struct WeaponsHit {
    // The kind and the unit whose shot hit.
    WeaponsEventHeader h;
    // The damage dealt to units of other players.
    int enemyDamage;
    // The damage dealt to units of the unit's own player.
    int friendlyDamage;
};

// The payload of START_SCRIPT.
struct WeaponsStartScript {
    // The kind and the unit whose script starts.
    WeaponsEventHeader h;
    // The script, a WeaponsScript.
    int script;
    // The aim callback the script returns to, as its block offset (0 for none).
    unsigned int callback;
    // The script's flag update.
    int update;
    // 1 when the script is started with arguments, 0 when it takes none.
    int withArgs;
    // The number of arguments, when withArgs is 1.
    int count;
    // The arguments, when withArgs is 1.
    int args[4];
};

// The payload of DETACH_PIECE.
struct WeaponsDetachPiece {
    // The kind and the unit.
    WeaponsEventHeader h;
    // The piece of the unit's model.
    int piece;
    // 1 when the model is first posed from its angles and animation.
    int pose;
};

// The payload of BUSY.
struct WeaponsBusy {
    // The kind and the unit.
    WeaponsEventHeader h;
    // The tick the unit is busy until.
    int until;
};

// The state of one weapon slot of a unit that a saved game keeps, all but its target pair (targeting's LOAD).
struct WeaponsSlotState {
    // Whether the weapon's aim is done.
    int aimDone;
    // A byte of the weapon's type that the save keeps.
    int typeByte;
    // The offset of the weapon's aim.
    int aimOffset;
    // The weapon's reload.
    int reload;
    // The weapon's heading.
    int heading;
    // The weapon's pitch.
    int pitch;
    // The weapon's stockpile.
    int stockpile;
    // The slot's flag bits 0, 1, 2 to 3 and 4.
    int flags;
};

// Weapons, projectiles and explosions, and the damage rules for every kind of damage to a unit. The engine delivers weapons' pending inbox, through Apply, before each run (UpdateProjectiles, UpdateExplosions, UpdateUnitWeapons), and at each phase boundary.
struct WeaponsSeam {
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const WeaponsEventHeader* event, unsigned int size);
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
    // Sets one of a unit's weapon slots (0 to 2) to the state a saved game keeps, outside the tick; a slot outside 0 to 2 changes nothing. Hooks: none.
    void (__stdcall* LoadState)(Unit* unit, int slot, const WeaponsSlotState* state);
    // Writes weapons' state as records of the trace, which the engine dumps and does not hash.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
