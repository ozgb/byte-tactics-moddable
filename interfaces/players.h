#ifndef INTERFACES_PLAYERS_H
#define INTERFACES_PLAYERS_H

#include "rand_stream.h"
#include "state_writer.h"

class HapiBank;
class MissionConditions;
class TdfFile;

// Major version of the players interface, changed when a slot is removed or its signature changes.
#define PLAYERS_VERSION_MAJOR 4
// Minor version of the players interface, changed when a slot is added.
#define PLAYERS_VERSION_MINOR 0

// The fields every players event and effect starts with.
struct PlayersEventHeader {
    // A PlayersEventKind or a PlayersEffectKind.
    int kind;
    // The unit, as its block offset (an offset from the block base, 0 for none: KILL_UNITS and TRANSFER name none), as economy's events give a unit.
    unsigned int unit;
};

// The kinds of event other modules publish to players' inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. Apply rejects an event, which then changes nothing and has outcome 0, when: it is smaller than its payload; GIVE_UNIT names a null unit or a player above 9, or the match has no conditions; KILL_UNITS names a player above 10 (10 is the record past the ten players); UNIT_CREATED and UNIT_DIED name a null unit, or the match has no conditions; TRANSFER names a giver or a player above 10, or a resource other than 0 and 1.
enum PlayersEventKind {
    // Hands unit to a player (PlayersGiveUnit): the conditions are told of the capture, and units is told to send it to a remote owner or to copy it for a local one (with the fields of the ownership record when it is present); outcome 1 when players published the hand-over (SEND_UNIT or CAPTURE_UNIT), which units can still refuse (a local copy past the unit limit is not made), 0 when the unit was already the player's, dead, dying or the player could not take it. Hooks: UnitTransfer.
    PLAYERS_GIVE_UNIT = 192,
    // Kills every unit of a player that is alive and not dying, one by one in the order of the player's units: by its own damage when its owner plays, else by self-destruct (PlayersKillUnits); outcome 1.
    PLAYERS_KILL_UNITS = 193,
    // Tells the victory and defeat conditions that unit was created; the header alone; outcome 1.
    PLAYERS_UNIT_CREATED = 194,
    // Tells the victory and defeat conditions that unit died; the header alone; outcome 1.
    PLAYERS_UNIT_DIED = 195,
    // Moves an amount of a resource from one player to another (PlayersTransfer): when it is announced it is clamped to what the giver has, spent from the giver's store and sent to the other machines; outcome 1, or 0 when either player is 10 or the amount is 0.
    PLAYERS_TRANSFER = 196
};

// The kinds of effect players publishes to its outbox, for the modules that subscribe to them.
enum PlayersEffectKind {
    // unit dies as its player's units are killed (PlayersKillUnit).
    PLAYERS_EFFECT_KILL_UNIT = 208,
    // unit goes to a player who plays on another machine (PlayersSendUnit): it stops being selectable by its old owner, waits out the transfer holdoff, is announced to the other machines and takes 30000 damage.
    PLAYERS_EFFECT_SEND_UNIT = 209,
    // A copy of unit is made for a player (PlayersCaptureUnit), with the health, build progress, angles and stockpiles of the ownership record when it is present, else with unit's own and unit takes 30000 damage, and with unit's state bits.
    PLAYERS_EFFECT_CAPTURE_UNIT = 210
};

// The fields of an ownership record another machine sent with a unit it handed over.
struct PlayersOwnershipRecord {
    // 1 when the fields below come from an ownership record, 0 when there is none and they are 0.
    int present;
    // The record's health.
    int hitPoints;
    // The record's build progress.
    int buildPercent;
    // The record's angles, 16-bit values sign-extended: bank, heading and pitch.
    int angles[3];
    // The record's stockpiles of the unit's three weapons, 0 to 255.
    int stockpiles[3];
};

// The payload of GIVE_UNIT.
struct PlayersGiveUnit {
    // The kind and the unit handed over.
    PlayersEventHeader h;
    // The player who is given the unit, 0 to 9.
    int player;
    // The ownership record that came with it.
    PlayersOwnershipRecord record;
};

// The payload of KILL_UNITS.
struct PlayersKillUnits {
    // The kind; the unit is 0.
    PlayersEventHeader h;
    // The player whose units are killed, 0 to 10.
    int player;
};

// The payload of TRANSFER.
struct PlayersTransfer {
    // The kind; the unit is 0.
    PlayersEventHeader h;
    // The player who gives.
    int from;
    // The player who receives.
    int to;
    // The resource: 0 energy, 1 metal.
    int resource;
    // The amount.
    float amount;
    // 1 when the transfer is announced (clamped, spent and sent to the other machines), 0 when it only credits the receiver.
    int announce;
};

// The payload of KILL_UNIT.
struct PlayersKillUnit {
    // The kind and the unit that dies.
    PlayersEventHeader h;
    // 0 by 30000 damage the unit deals itself, 1 by self-destruct (its weapon detonates, it is marked dying and killed).
    int selfDestruct;
};

// The payload of SEND_UNIT.
struct PlayersSendUnit {
    // The kind and the unit.
    PlayersEventHeader h;
    // The player it goes to, 0 to 9.
    int player;
};

// The payload of CAPTURE_UNIT.
struct PlayersCaptureUnit {
    // The kind and the unit copied.
    PlayersEventHeader h;
    // The player the copy is made for, 0 to 9.
    int player;
    // The ownership record the copy takes its fields from.
    PlayersOwnershipRecord record;
};

// The match players sets its private state up for.
struct PlayersMapInfo {
    // The number of players.
    int playerCount;
};

// Per-player state: the players, their mission conditions and the end of the game. The engine delivers players' pending inbox, through Apply, before each run (UpdatePlayerHead, UpdatePlayerBody, UpdatePlayerTail, UpdatePlayersFinish), and at each phase boundary.
struct PlayersSeam {
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one. Hooks: none.
    int (__stdcall* Apply)(const PlayersEventHeader* event, unsigned int size);
    // Advances every player's state for one tick. Hooks: none.
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
    // Stops the victory and defeat conditions of the mission from being checked. Hooks: none.
    void (__stdcall* DeactivateConditions)(MissionConditions* conditions);
    // Reads the conditions state of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadConditions)(MissionConditions* conditions, HapiBank* file);
    // Writes the conditions state of a saved game to a bank. Hooks: none.
    void (__stdcall* SaveConditions)(MissionConditions* conditions, HapiBank* file);
    // Reads the victory and defeat conditions from a mission file. Hooks: none.
    void (__stdcall* RegisterConditions)(MissionConditions* conditions, TdfFile* mission);
    // Frees the condition objects. Hooks: none.
    void (__stdcall* FreeConditions)(MissionConditions* conditions);
    // Frees the condition objects and then the conditions object itself. Hooks: none.
    void (__stdcall* DeleteConditions)(MissionConditions* conditions);
    // Advances every player's state for one tick like UpdatePlayers, but draws from the given stream; UpdatePlayers draws from the main stream. Hooks: none.
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
    // The bytes of private state the implementation needs for a match; called before Init. Hooks: none.
    unsigned int (__stdcall* RegionSize)(const PlayersMapInfo* info);
    // Builds the state for a match in a region the engine allocated of the size RegionSize gave; called once per match, before InitPlayers and before any event. Hooks: none.
    void (__stdcall* Init)(const PlayersMapInfo* info, void* region, unsigned int size);
    // Ends the match and returns the region, which the engine frees. Hooks: none.
    void* (__stdcall* Shutdown)(void);
    // Writes the state that decides what the implementation does next and that the engine does not already hash elsewhere; it reads only, and keeps nothing. Hooks: none.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
