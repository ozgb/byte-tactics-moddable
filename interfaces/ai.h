#ifndef INTERFACES_AI_H
#define INTERFACES_AI_H

namespace std {
template<class T> class allocator;
template<class T, class A> class vector;
}

template <class T> struct Ref;
struct Unit;
struct Player;
struct Vec3;
struct StateWriter;

// Major version of the ai interface, changed when a slot is removed or its signature changes.
#define AI_VERSION_MAJOR 4
// Minor version of the ai interface, changed when a slot is added.
#define AI_VERSION_MINOR 0

// The kinds of event other modules publish to ai's inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects an event, which then changes nothing and has outcome 0, when: REACT_TO_ATTACK, DAMAGE_SPLIT or SET_SQUAD names a null unit; SET_SQUAD names a squad outside -1 to 9, or a unit whose player has no squads; PLAN names a bit above 3; the kind is unknown.
enum AiEventKind {
    // unit was attacked by target (0 when no unit attacked it, or the attacker is dead) for value damage, and its player may react to it with orders and targets for unit; outcome 1.
    AI_REACT_TO_ATTACK = 224,
    // A shot of unit hit, dealing value damage to units of other players and extra damage to units of unit's own player; ai may tell unit's orders whether it hit mostly enemies (AI_EFFECT_HIT); outcome 1.
    AI_DAMAGE_SPLIT = 225,
    // unit joins squad value (0 to 9) of its player and leaves the squad it was in, or only leaves it when value is -1; outcome 1.
    AI_SET_SQUAD = 226,
    // Switches the AI commands (WEIGHT and LIMIT) on when value names the match's difficulty (bit 0 easy, bit 1 medium, bit 2 hard) or has bit 3 (any difficulty), and off otherwise; outcome 1.
    AI_PLAN = 227,
    // When the AI commands are on, scales by scale the build weight (0 to 100) of each unit type in types for every player with a squad manager, unless a WEIGHT with extra 1 fixed it; extra 1 fixes it; outcome 1, or 0 when the commands are off.
    AI_WEIGHT = 228,
    // When the AI commands are on, sets value as the build limit of each unit type in types for every active computer player, unless a LIMIT with extra 1 fixed it; extra 1 fixes it; outcome 1, or 0 when the commands are off.
    AI_LIMIT = 229
};

// The kinds of effect ai publishes to its outbox, for the modules that subscribe to them; each is a plain request, and the subscriber decides what it does with it.
enum AiEffectKind {
    // unit gets an order of type value (the kind orders' GetOrderType returned) against target or the position pos when hasPos is 1, with the order's two arguments params; when extra is 0 it replaces the unit's orders that are not marked to stay, when 1 it is queued behind them.
    AI_EFFECT_ADD_ORDER = 240,
    // The factory unit queues extra more of the unit type value to build.
    AI_EFFECT_QUEUE_BUILD = 241,
    // unit is ordered to attack target, as orders' IssueAttackOrder gives the order; ai publishes it only when unit is not target, its movement and firing states are not 0 (hold) and orders' GetOrderType names an order against target.
    AI_EFFECT_ATTACK = 242,
    // unit's orders are deleted: every order when value is 1, the ones not marked to stay when 0.
    AI_EFFECT_STOP = 243,
    // The holders of a reference to unit are told the event value (16: the unit was attacked).
    AI_EFFECT_NOTIFY_REFS = 244,
    // unit's state bits in value are set when extra is 1 and cleared when 0 (bit 0: the unit is switched on).
    AI_EFFECT_ACTIVATE = 245,
    // unit takes the movement state value (0 hold position, 1 manoeuvre, 2 roam) and the firing state extra (0 hold fire, 1 return fire, 2 fire at will).
    AI_EFFECT_SET_STATES = 246,
    // unit's orders are told that its shot hit mostly units of other players (value 1) or not (value 0).
    AI_EFFECT_HIT = 247
};

// An event (AiEventKind) or an effect (AiEffectKind), as plain data with no pointers; fields a kind does not use are 0.
struct AiEvent {
    // An AiEventKind or an AiEffectKind.
    int kind;
    // The unit the kind is about, as its block offset (an offset from the block base, 0 for none), as economy's events give a unit.
    unsigned int unit;
    // The other unit, as its block offset: the attacker of REACT_TO_ATTACK, the target of an order.
    unsigned int target;
    // The argument of the kind, as the kind says.
    int value;
    // The second argument of the kind, as the kind says.
    int extra;
    // The two arguments of an order (ADD_ORDER).
    int params[2];
    // 1 when pos holds a position.
    int hasPos;
    // A position in world coordinates with 16 fractional bits: x, y and z.
    int pos[3];
    // The factor of WEIGHT.
    float scale;
    // The unit types of WEIGHT and LIMIT, as a set of bits: type n is bit n % 32 of word n / 32.
    unsigned int types[16];
};

// Computer player decisions: what each player's AI sees and wants, and the orders and targets it gives its units. The engine delivers ai's pending inbox, through Apply, before each run (UpdateSquads, UpdatePlayerAI), and at each phase boundary.
struct AiSeam {
    // Consumes one queued event of the inbox and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const AiEvent* event);
    // Appends to out the enemies a player sees within a radius of a position. Hooks: none.
    void (__stdcall* GetVisibleEnemiesInRadius)(int player, const Vec3* pos, int radius, int flags, std::vector<Unit*, std::allocator<Unit*> >* out);
    // Returns whether a player has a ready unit within a range of a position. Hooks: none.
    bool (__stdcall* HasReadyUnitInRange)(int player, Vec3* pos, int range);
    // Returns 1 when a position is inside what a player's side can see. Hooks: none.
    int (__stdcall* IsPointVisible)(Player* player, Vec3* pos);
    // Writes a report on the AI of a player to a file. Hooks: none.
    void (__stdcall* DumpPlayerAI)(int player, void* file);
    // Resets the unit tables of the computer players and loads the default AI script again. Hooks: none.
    void (__stdcall* ResetAIPlayers)(void);
    // Appends to out the factories a player has within a radius of a position. Hooks: none.
    void (__stdcall* GetFactoriesInRadius)(int player, const Vec3* pos, int radius, std::vector<Unit*, std::allocator<Unit*> >* out);
    // Creates the AI state of a player. Hooks: none.
    void (__stdcall* CreatePlayerAI)(int player);
    // Frees the AI state of a player. Hooks: none.
    void (__stdcall* DestroyPlayerAI)(int player);
    // Advances what the AI of a player knows of the match for one tick. Hooks: none.
    void (__stdcall* UpdatePlayerAI)(int player);
    // Rebuilds the feature cells an AI player keeps. Hooks: none.
    void (__stdcall* RebuildFeatureCells)(int player);
    // Parses the AI weight scripts a player downloaded. Hooks: none.
    void (__stdcall* ParseDownloadableAiWeightScripts)(int player);
    // Parses a player's AI weight scripts again unless the unit limit is sticky. Hooks: none.
    void (__stdcall* ReparseAiWeightScriptsIfLimitNotSticky)(int player);
    // Creates the squads of a player. Hooks: none.
    void (__stdcall* CreateSquads)(Player* player);
    // Frees the squads of a player. Hooks: none.
    void (__stdcall* FreeSquads)(Player* player);
    // Runs one tick of a player's squads when it is an active computer player: the orders and targets its AI gives. Hooks: none.
    void (__stdcall* UpdateSquads)(int player);
    // Creates the squad manager of a player, which UpdateSquads runs. Hooks: none.
    void (__stdcall* CreateSquadManager)(Player* player);
    // Frees the squad manager of a player that CreateSquadManager made, and its timers. Hooks: none.
    void (__stdcall* FreeSquadManager)(Player* player);
    // Writes to out, as Ref<Unit>, the first capacity of the enemies a player sees within a radius of a position and returns how many there are. Hooks: none.
    int (__stdcall* GetVisibleEnemiesInRadiusList)(int player, const Vec3* pos, int radius, int flags, Ref<Unit>* out, int capacity);
    // Writes ai's state as records of the trace, which the engine dumps and does not hash.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
