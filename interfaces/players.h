#ifndef INTERFACES_PLAYERS_H
#define INTERFACES_PLAYERS_H

#include "rand_stream.h"

class HapiBank;
class MissionConditions;
class TdfFile;

// Major version of the players interface, changed when a slot is removed or its signature changes.
#define PLAYERS_VERSION_MAJOR 3
// Minor version of the players interface, changed when a slot is added.
#define PLAYERS_VERSION_MINOR 0

// The kinds of event other modules publish to players' inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects an event, which then changes nothing and has outcome 0, when: GIVE_UNIT names a null unit or a player above 9, or the match has no conditions; KILL_UNITS names a player above 10 (10 is the record past the ten players); UNIT_CREATED and UNIT_DIED name a null unit, or the match has no conditions; TRANSFER names a giver or a player above 10, or a resource other than 0 and 1.
enum PlayersEventKind {
    // Hands unit to player: the conditions are told of the capture, and units is told to send it to a remote owner or to copy it for a local one (with the fields of an ownership record when record is 1); outcome 1 when players published the hand-over (SEND_UNIT or CAPTURE_UNIT), which units can still refuse (a local copy past the unit limit is not made), 0 when the unit was already the player's, dead, dying or the player could not take it.
    PLAYERS_GIVE_UNIT = 192,
    // Kills every unit of player that is alive and not dying, one by one in the order of the player's units: by its own damage when its owner plays, else by self-destruct; outcome 1.
    PLAYERS_KILL_UNITS = 193,
    // Tells the victory and defeat conditions that unit was created; outcome 1.
    PLAYERS_UNIT_CREATED = 194,
    // Tells the victory and defeat conditions that unit died; outcome 1.
    PLAYERS_UNIT_DIED = 195,
    // Moves amount of resource (0 energy, 1 metal) from player from to player: when announce is 1 it is clamped to what from has, spent from from's store and sent to the other machines; outcome 1, or 0 when either player is 10 or the amount is 0.
    PLAYERS_TRANSFER = 196
};

// The kinds of effect players publishes to its outbox, for the modules that subscribe to them.
enum PlayersEffectKind {
    // unit dies as its player's units are killed: value 0 by 30000 damage it deals itself, value 1 by self-destruct (its weapon detonates, it is marked dying and killed).
    PLAYERS_EFFECT_KILL_UNIT = 208,
    // unit goes to player, who plays on another machine: it stops being selectable by its old owner, waits out the transfer holdoff, is announced to the other machines and takes 30000 damage.
    PLAYERS_EFFECT_SEND_UNIT = 209,
    // A copy of unit is made for player, with the health, build progress, angles and stockpiles of the ownership record when record is 1, else with unit's own and unit takes 30000 damage, and with unit's state bits.
    PLAYERS_EFFECT_CAPTURE_UNIT = 210
};

// An event (PlayersEventKind) or an effect (PlayersEffectKind), as plain data with no pointers; fields a kind does not use are 0.
struct PlayersEvent {
    // A PlayersEventKind or a PlayersEffectKind.
    int kind;
    // The unit, as its block offset (an offset from the block base, 0 for none), as economy's events give a unit.
    unsigned int unit;
    // The player the kind is about (0 to 9, 10 for none): the receiver of a unit or a transfer, or the player whose units are killed.
    int player;
    // The giver of a transfer.
    int from;
    // The argument of the kind, as the kind says.
    int value;
    // The amount of a transfer.
    float amount;
    // 1 when a transfer is announced (clamped, spent and sent to the other machines), 0 when it only credits the receiver.
    int announce;
    // 1 when the fields below come from an ownership record received from another machine.
    int record;
    // The record's health.
    int hitPoints;
    // The record's build progress.
    int buildPercent;
    // The record's angles: bank, heading and pitch.
    short angles[3];
    // The record's stockpiles of the unit's three weapons.
    unsigned char stockpiles[3];
};

// Per-player state: the players, their mission conditions and the end of the game. The engine delivers players' pending inbox, through Apply, before each run (UpdatePlayerHead, UpdatePlayerBody, UpdatePlayerTail, UpdatePlayersFinish), and at each phase boundary.
struct PlayersSeam {
    // Consumes one queued event of the inbox and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const PlayersEvent* event);
    // Advances every player's state for one tick.
    void (__stdcall* UpdatePlayers)(void);
    // Runs the AI script of the mission, or the default one, and parses the AI weight scripts of the computer players. Hooks: none.
    void (__stdcall* LoadDefaultAIScript)(void);
    // Sets up the player slots at the start of a match. Hooks: none.
    void (__stdcall* InitPlayers)(void);
    // Frees the player slots at the end of a match. Hooks: none.
    void (__stdcall* FreePlayers)(void);
    // Gives every player the metal and energy the mission starts with. Hooks: none.
    void (__stdcall* InitPlayerResources)(void);
    // Reads which player is a human and which a computer from a saved game. Hooks: none.
    void (__stdcall* LoadPlayerControllers)(HapiBank* file);
    // Reads the players of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadPlayers)(HapiBank* file);
    // Writes the players of a saved game to a bank. Hooks: none.
    void (__stdcall* SavePlayers)(HapiBank* file);
    // Rebuilds the feature cells every computer player keeps. Hooks: none.
    void (__stdcall* RebuildAIFeatureCells)(void);
    // Fills a player slot as a human, a computer or an empty one. Hooks: none.
    void (__stdcall* SetupPlayerSlot)(int player, char type);
    // Steps the blinking of the player markers. Hooks: none.
    void (__stdcall* UpdateBlink)(void);
    // Stops the victory and defeat conditions of the mission from being checked (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* DeactivateConditions)(MissionConditions* conditions);
    // Reads the conditions state of a saved game from a bank (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* LoadConditions)(MissionConditions* conditions, int unusedEdx, HapiBank* file);
    // Writes the conditions state of a saved game to a bank (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* SaveConditions)(MissionConditions* conditions, int unusedEdx, HapiBank* file);
    // Reads the victory and defeat conditions from a mission file (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* RegisterConditions)(MissionConditions* conditions, int unusedEdx, TdfFile* mission);
    // Frees the condition objects (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* FreeConditions)(MissionConditions* conditions);
    // Frees the condition objects and then the conditions object itself. Hooks: none.
    void (__stdcall* DeleteConditions)(MissionConditions* conditions);
    // Advances every player's state for one tick like UpdatePlayers, but draws from the given stream; UpdatePlayers draws from the main stream.
    void (__stdcall* UpdatePlayersOnStream)(RandStream stream);
    // Builds an empty conditions object, active and with no victory or defeat conditions. Hooks: none.
    MissionConditions* (__stdcall* NewConditions)(void);
    // Runs the start of one player's tick: returns 0 when the slot holds no player that plays, 1 when the engine runs its computer player's turn, its line of sight pass and then its body. Hooks: none.
    int (__stdcall* UpdatePlayerHead)(int player);
    // Runs one player's tick from after its line of sight pass up to the settlement of its economy, drawing from the given stream: returns 0 when the player has no more to do this tick, 1 when only its tail remains, 2 when its economy is due and then its tail. Hooks: none.
    int (__stdcall* UpdatePlayerBody)(int player, RandStream stream);
    // Runs the rest of one player's tick, after the settlement of its economy and, for the viewing player, the engine's radar passes. Hooks: none.
    void (__stdcall* UpdatePlayerTail)(int player);
    // Runs what is left of the players' tick after the last player: the end of a network game when no player is left to fight. Hooks: none.
    void (__stdcall* UpdatePlayersFinish)(void);
};

#endif
