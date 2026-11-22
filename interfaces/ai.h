#ifndef INTERFACES_AI_H
#define INTERFACES_AI_H

struct Player;
struct StateWriter;

// Major version of the ai interface, changed when a slot is removed or its signature changes.
#define AI_VERSION_MAJOR 6
// Minor version of the ai interface, changed when a slot is added.
#define AI_VERSION_MINOR 0

// The fields every ai event and effect starts with.
struct AiEventHeader {
    // An AiEventKind or an AiEffectKind.
    int kind;
    // The unit the kind is about, as its block offset (an offset from the block base, 0 for none: PLAN, WEIGHT and LIMIT name none), as economy's events give a unit.
    unsigned int unit;
};

// The kinds of event other modules publish to ai's inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects an event, which then changes nothing and has outcome 0, when: it is smaller than its payload; REACT_TO_ATTACK or SET_SQUAD names a null unit; SET_SQUAD names a squad outside -1 to 9, or a unit whose player has no squads; PLAN names a bit above 3; the kind is unknown.
enum AiEventKind {
    // unit was attacked (AiReactToAttack); when unit's player is an active computer player, its AI may react with orders for unit; outcome 1. Hooks: AIOnDamaged.
    AI_REACT_TO_ATTACK = 224,
    // unit joins a squad of its player and leaves the squad it was in, or only leaves it (AiSetSquad); outcome 1.
    AI_SET_SQUAD = 226,
    // Switches the AI commands (WEIGHT and LIMIT) on when the plan names the match's difficulty, and off otherwise (AiPlan); outcome 1.
    AI_PLAN = 227,
    // When the AI commands are on, scales the build weight (0 to 100) of each unit type of a set for every player with a squad manager, unless a fixing WEIGHT fixed it (AiWeight); outcome 1, or 0 when the commands are off.
    AI_WEIGHT = 228,
    // When the AI commands are on, sets the build limit of each unit type of a set for every active computer player, unless a fixing LIMIT fixed it (AiLimit); outcome 1, or 0 when the commands are off.
    AI_LIMIT = 229
};

// The kinds of effect ai publishes to its outbox, for the modules that subscribe to them; each is a plain request, and the subscriber decides what it does with it.
enum AiEffectKind {
    // unit gets an order (AiAddOrder).
    AI_EFFECT_ADD_ORDER = 240,
    // The factory unit queues more of a unit type to build (AiQueueBuild).
    AI_EFFECT_QUEUE_BUILD = 241,
    // unit's orders are deleted (AiStop).
    AI_EFFECT_STOP = 243,
    // Some of unit's state bits are set or cleared (AiActivate).
    AI_EFFECT_ACTIVATE = 245,
    // unit takes a movement state and a firing state (AiSetStates).
    AI_EFFECT_SET_STATES = 246
};

// The payload of REACT_TO_ATTACK.
struct AiReactToAttack {
    // The kind and the unit attacked.
    AiEventHeader h;
    // The attacker, as its block offset (0 when no unit attacked, or the attacker is dead).
    unsigned int attacker;
    // The damage dealt.
    int damage;
};

// The payload of SET_SQUAD.
struct AiSetSquad {
    // The kind and the unit.
    AiEventHeader h;
    // The squad it joins, 0 to 9, or -1 to only leave its squad.
    int squad;
};

// The payload of PLAN.
struct AiPlan {
    // The kind; the unit is 0.
    AiEventHeader h;
    // The difficulties the commands are for: bit 0 easy, bit 1 medium, bit 2 hard, bit 3 any.
    unsigned int difficulties;
};

// The payload of WEIGHT.
struct AiWeight {
    // The kind; the unit is 0.
    AiEventHeader h;
    // The factor the weights are scaled by.
    float scale;
    // 1 to fix the weights so that later WEIGHT events leave them, 0 not to.
    int fix;
    // The unit types, as a set of bits: type n is bit n % 32 of word n / 32.
    unsigned int types[16];
};

// The payload of LIMIT.
struct AiLimit {
    // The kind; the unit is 0.
    AiEventHeader h;
    // The build limit.
    int limit;
    // 1 to fix the limits so that later LIMIT events leave them, 0 not to.
    int fix;
    // The unit types, as a set of bits: type n is bit n % 32 of word n / 32.
    unsigned int types[16];
};

// The payload of ADD_ORDER.
struct AiAddOrder {
    // The kind and the unit ordered.
    AiEventHeader h;
    // The order's type, the kind orders' GetOrderType returned.
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
struct AiQueueBuild {
    // The kind and the factory unit.
    AiEventHeader h;
    // The unit type to build.
    int type;
    // How many more to queue.
    int count;
};

// The payload of STOP.
struct AiStop {
    // The kind and the unit.
    AiEventHeader h;
    // 1 to delete every order, 0 only the ones not marked to stay.
    int all;
};

// The payload of ACTIVATE.
struct AiActivate {
    // The kind and the unit.
    AiEventHeader h;
    // The state bits (bit 0: the unit is switched on).
    unsigned int bits;
    // 1 to set them, 0 to clear them.
    int on;
};

// The payload of SET_STATES.
struct AiSetStates {
    // The kind and the unit.
    AiEventHeader h;
    // The movement state: 0 hold position, 1 manoeuvre, 2 roam.
    int movement;
    // The firing state: 0 hold fire, 1 return fire, 2 fire at will.
    int firing;
};

// Computer player decisions: what each computer player's AI wants, and the orders and targets it gives its units; other modules call no slot of it but through the engine. The engine delivers ai's pending inbox, through Apply, before each run (UpdateSquads, UpdatePlayerAI), and at each phase boundary.
struct AiSeam {
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one. Hooks: none.
    int (__stdcall* Apply)(const AiEventHeader* event, unsigned int size);
    // Writes a report on the AI of a player to a file. Hooks: none.
    void (__stdcall* DumpPlayerAI)(int player, void* file);
    // Resets the unit tables of the computer players and loads the default AI script again. Hooks: none.
    void (__stdcall* ResetAIPlayers)(void);
    // Creates the AI state of a player. Hooks: none.
    void (__stdcall* CreatePlayerAI)(int player);
    // Frees the AI state of a player. Hooks: none.
    void (__stdcall* DestroyPlayerAI)(int player);
    // Advances what the AI of a player knows of its own units for one tick. Hooks: none.
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
    // Runs one tick of a player's squads when it is an active computer player: the orders and targets its AI gives. Hooks: AIAssignSquad, AIAttackReady, AIPatrolTarget, AIFactoryTick, AIBuilderAvailable.
    void (__stdcall* UpdateSquads)(int player);
    // Creates the squad manager of a player, which UpdateSquads runs. Hooks: none.
    void (__stdcall* CreateSquadManager)(Player* player);
    // Frees the squad manager of a player that CreateSquadManager made, and its timers. Hooks: none.
    void (__stdcall* FreeSquadManager)(Player* player);
    // Writes ai's state as records of the trace, which the engine dumps and does not hash. Hooks: none.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
