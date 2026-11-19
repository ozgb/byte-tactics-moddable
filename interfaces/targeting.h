#ifndef INTERFACES_TARGETING_H
#define INTERFACES_TARGETING_H

struct Unit;
struct Vec3;

// Major version of the targeting interface, changed when a slot is removed or its signature changes.
#define TARGETING_VERSION_MAJOR 2
// Minor version of the targeting interface, changed when a slot is added.
#define TARGETING_VERSION_MINOR 0

// The kinds of event other modules publish to targeting's inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. An event with a null unit, a weapon outside 0 to 2, or (SET_UNIT) a null target changes nothing; outcome 0.
enum TargetingEventKind {
    // Aims weapon of unit at the unit target, and clears the notices of unit's weapons that its orders wait on: bits 10 to 14 of unit's dirty flags (the mask 0x7c00, the same for every weapon; bit 10 a shot fired, 11 a command shot fired, 12 a target out of reach); outcome 1.
    TARGETING_SET_UNIT = 96,
    // Aims weapon of unit at the ground position pos (x and z), and clears the same notices, bits 10 to 14 of unit's dirty flags, for every weapon alike; outcome 1.
    TARGETING_SET_POS = 97,
    // Stops weapon of unit aiming; when it had a target, TARGETING_EFFECT_CLEARED tells the unit's script; outcome 1 when it had a target, 0 when not.
    TARGETING_CLEAR = 98,
    // Drops the target of weapon of unit without telling its script, as a unit is set up; outcome 1 when it had a target, 0 when not.
    TARGETING_RESET = 99
};

// The kinds of effect targeting publishes to its outbox, for the modules that subscribe to them.
enum TargetingEffectKind {
    // The target of weapon of unit was cleared: the unit's script runs its TargetCleared function with the weapon's number.
    TARGETING_EFFECT_CLEARED = 112
};

// An event (TargetingEventKind) or an effect (TargetingEffectKind), as plain data with no pointers; fields a kind does not use are 0.
struct TargetingEvent {
    // A TargetingEventKind or a TargetingEffectKind.
    int kind;
    // The unit whose weapon aims, as its block offset (an offset from the block base, 0 for none), as economy's events give a unit.
    unsigned int unit;
    // The weapon of the unit, 0 to 2.
    int weapon;
    // The unit aimed at, as its block offset.
    unsigned int target;
    // The position aimed at, in world coordinates with 16 fractional bits.
    int pos[3];
};

// Target selection for weapons. The engine delivers targeting's pending inbox, through Apply, before each slot below but Apply and ChooseWeapon, and at each phase boundary.
struct TargetingSeam {
    // Consumes one queued event of the inbox and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const TargetingEvent* event);
    // Returns the unit a weapon aims at, or null; a query. Hooks: none.
    Unit* (__stdcall* GetWeaponTargetUnit)(Unit* unit, int weapon);
    // Synchronous: writes the position a weapon aims at and returns whether it has a target, for a caller that aims or fires at it in the same statement; a target unit is gone when its slot holds no unit (unitDefIndex 0), and is then cleared first, as TARGETING_CLEAR clears it, and the answer is 0; a target that died but whose slot still holds it, or whose slot a new unit took, is not gone, and the slot is aimed at. Hooks: none.
    int (__stdcall* GetWeaponTargetPos)(Unit* unit, Vec3* pos, int weapon);
    // Synchronous: picks the best enemy for one weapon, within its range or the unit's sight, drawing from the generator of the service RandomInt (rng.seed in the recordings, not a game logic stream), for a caller that aims at it in the same statement. Hooks: FilterTargets.
    Unit* (__stdcall* FindWeaponTarget)(Unit* unit, unsigned char weapon, int useRange);
    // Synchronous: picks the best enemy for a unit that fires at will, as FindWeaponTarget does for its first weapon, or returns null. Hooks: FilterTargets.
    Unit* (__stdcall* FindBestTargetIfFireAtWill)(Unit* unit);
    // Returns the weapon a unit fires first at its target; a query (thiscall in 3.1c, this first). Hooks: none.
    unsigned char (__fastcall* ChooseWeapon)(Unit* unit);
};

#endif
