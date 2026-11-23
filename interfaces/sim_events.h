#ifndef INTERFACES_SIM_EVENTS_H
#define INTERFACES_SIM_EVENTS_H

// The kinds of sim event, each with the meaning of its arguments (positions are fixed-point integers; players and units are indices).
enum SimEventKind : unsigned short {
    // Marks an empty record, never recorded.
    SIM_EVENT_NONE = 0,
    // A unit speaks: the unit index, the speech kind and a SimSpeechText.
    SIM_EVENT_UNIT_SPEECH = 1,
    // A sound plays: a SimSoundName and the play flag.
    SIM_EVENT_SOUND = 2,
    // Nano particles flow from a point to a box: the origin (3), the box low and high corners (6) and the particle list.
    SIM_EVENT_NANO_PARTICLES = 3,
    // Nano particles flow from a box to a point: the box low and high corners (6), the origin (3) and the particle list.
    SIM_EVENT_REVERSE_NANO_PARTICLES = 4,
    // Teleport sparks fly between two points: the start (3), the end (3), the lifetime and the particle list.
    SIM_EVENT_TELEPORT_PARTICLES = 5,
    // Thrust particles stream between two points: the start (3), the end (3), the period, the lifetime and the particle list.
    SIM_EVENT_THRUST_PARTICLES = 6,
    // A wake trails between two points: the start (3), the end (3), the period and the particle list.
    SIM_EVENT_WAKE_PARTICLES = 7,
    // Bubbles rise between two points: the start (3), the end (3), the period and the particle list.
    SIM_EVENT_BUBBLES = 8,
    // A weapon puffs smoke: the position (3) and the particle list.
    SIM_EVENT_WEAPON_SMOKE = 9,
    // A timed plume of particles rises: the position (3) and the particle list.
    SIM_EVENT_TIMED_SUB_PARTICLES = 10,
    // The screen shakes: the two offsets and the strength.
    SIM_EVENT_SCREEN_SHAKE = 11,
    // The disc activity meter counts a hit: the amount and two players, the meter counts it when either is the local player.
    SIM_EVENT_CD_ACTIVITY = 12,
    // A side's forces are all destroyed: the player index.
    SIM_EVENT_FORCES_DESTROYED = 13,
    // The score panel flashes a kill and a loss: the killer's player index (never 10, none) and the loser's player index.
    SIM_EVENT_SCORE_FLASH = 14,
    // The orders panel of a unit needs a refresh: the unit index.
    SIM_EVENT_SELECTION_ORDERS_DIRTY = 15,
    // The build menu and its queued build counts refresh when the unit has the focus: the unit index.
    SIM_EVENT_BUILD_MENU_UPDATE = 16,
    // A unit that can no longer be selected leaves the selection: the unit index.
    SIM_EVENT_DESELECT_IF_INELIGIBLE = 17,
    // The GUI redraws when a SimScreenName is on top: the screen.
    SIM_EVENT_GUI_CHANGED_IF_SCREEN = 18,
    // The camera and the selection go to the commander: the force flag.
    SIM_EVENT_FOCUS_COMMANDER = 19,
    // The GUI closes screens down to the game screen: the force flag.
    SIM_EVENT_POP_TO_GAME_LAYOUT = 20,
    // A player takes the lead: the player index and the kills that put it there.
    SIM_EVENT_PLAYER_TOOK_LEAD = 21,
    // A player has no units left in a game with leaving players: the player's network id.
    SIM_EVENT_PLAYER_LEFT = 22,
    // A build count changed: the player index of the unit and the change, the sound plays when that player is the one shown.
    SIM_EVENT_BUILD_QUEUE_SOUND = 23,
    // A defeated player may keep watching: the player index, the question shows when it is the local player.
    SIM_EVENT_WATCH_PROMPT = 25,
    // A player who hosts AI players that are still alive is placed in watch mode: the player index, the notice shows when it is the local player.
    SIM_EVENT_WATCH_MODE_NOTICE = 26,
    // A smoke plume rises: the position (3), the emit period, the lifetime in ticks and the particle list.
    SIM_EVENT_SMOKE = 27,
    // A puff of white smoke: the position (3) and the particle list.
    SIM_EVENT_WHITE_SMOKE = 28,
    // A puff of black smoke: the position (3) and the particle list.
    SIM_EVENT_BLACK_SMOKE = 29,
    // A sound plays at a position: the sound index of the game, the position (3) and the set of players who hear it (bit n for player n).
    SIM_EVENT_SOUND_AT = 30,
    // A named sound plays at a position: a SimSoundName, the position (3) and the set of players who hear it (bit n for player n).
    SIM_EVENT_SOUND_AT_NAME = 31,
    // A unit's pending speech lines are dropped: the unit index.
    SIM_EVENT_REMOVE_UNIT_SPEECH = 32,
    // A line enters the message log: a SimMessageText and its value.
    SIM_EVENT_MESSAGE = 33,
    // The radar picture shows the units and projectiles of a snapshot: its offset in the radar arena.
    SIM_EVENT_RADAR_UNITS = 34,
    // The radar picture marks the cells a snapshot says are explored or hidden: its offset in the radar arena.
    SIM_EVENT_RADAR_MAPPED = 35,
};
// Flag of SIM_EVENT_UNIT_SPEECH: the unit stays silent when it is on screen.
#define SIM_EVENT_SPEECH_IF_NOT_VISIBLE 1u
// Flag of a record that carries the arguments past the sixth of the event before it.
#define SIM_EVENT_CONTINUED 0x8000u

// The texts that go with a unit's speech line; 0 is the text of the line's kind.
enum SimSpeechText : unsigned short {
    SIM_SPEECH_KIND_TEXT = 0,
    SIM_SPEECH_ATTACKING = 1,
    SIM_SPEECH_BUILDING = 2,
    SIM_SPEECH_BUILDING_COMPLETE = 3,
    SIM_SPEECH_CAPTURE_FAILED = 4,
    SIM_SPEECH_CAPTURING = 5,
    SIM_SPEECH_CONSTRUCTION_STOPPED = 6,
    SIM_SPEECH_CONSTRUCTION_TERMINATED = 7,
    SIM_SPEECH_CONSTRUCTION_TERMINATED_BY_HOSTILE_ACTION = 8,
    SIM_SPEECH_GUARDING = 9,
    SIM_SPEECH_I_CANT_GET_THERE = 10,
    SIM_SPEECH_I_CANT_REACH_THE_CONSTRUCTION_SITE = 11,
    SIM_SPEECH_LANDING = 12,
    SIM_SPEECH_LANDING_ABORTED = 13,
    SIM_SPEECH_LANDING_ABORTED_ALL_PADS_ARE_OCCUPIED = 14,
    SIM_SPEECH_LANDING_ABORTED_NO_PADS_AVAILABLE = 15,
    SIM_SPEECH_LANDING_FAILED = 16,
    SIM_SPEECH_LOADING = 17,
    SIM_SPEECH_LOADING_UNIT = 18,
    SIM_SPEECH_PATROLLING = 19,
    SIM_SPEECH_PREPARING_FOR_TRANSPORT = 20,
    SIM_SPEECH_RECLAIMING = 21,
    SIM_SPEECH_RECLAMATION_FAILED = 22,
    SIM_SPEECH_REPAIR_ABORTED = 23,
    SIM_SPEECH_REPAIRING = 24,
    SIM_SPEECH_REPAIR_MISSION_FAILED = 25,
    SIM_SPEECH_REPAIRS_UNSUCCESSFUL = 26,
    SIM_SPEECH_RESSURECTION_FAILED = 27,
    SIM_SPEECH_RESURRECTION_COMPLETE = 28,
    SIM_SPEECH_RESURRECTION_FAILED = 29,
    SIM_SPEECH_STARTING_CONSTRUCTION = 30,
    SIM_SPEECH_TARGET_AREA_WAS_BLOCKED = 31,
    SIM_SPEECH_THAT_UNIT_CANNOT_BE_CAPTURED = 32,
    SIM_SPEECH_THAT_UNIT_CANNOT_BE_RECLAIMED = 33,
    SIM_SPEECH_THAT_UNIT_IS_A_CLOUD_OF_VAPOR_AND_CANNOT_BE_CAPTURED = 34,
    SIM_SPEECH_TRANSPORT_MISSION_FAILED = 35,
    SIM_SPEECH_UNABLE_TO_CREATE_ANY_MORE_UNITS = 36,
    SIM_SPEECH_UNABLE_TO_UNLOAD_UNIT = 37,
    SIM_SPEECH_UNIT_IS_TOO_HEAVY_TO_TRANSPORT = 38,
    SIM_SPEECH_UNIT_IS_TOO_LARGE_TO_TRANSPORT = 39,
    SIM_SPEECH_UNIT_REPAIRED = 40,
    SIM_SPEECH_UNLOADING = 41,
    SIM_SPEECH_UNLOADING_PROCESS_IS_PROCEEDING_NON_OPTIMALLY = 42,
    SIM_SPEECH_WAITING_FOR_TARGET_AREA_TO_CLEAR = 43,
    SIM_SPEECH_TEXT_COUNT = 44,
};

// The sounds that sim plays by name.
enum SimSoundName : unsigned short {
    SIM_SOUND_VICTORY_CONDITION = 0,
    SIM_SOUND_TREEBURN = 1,
    SIM_SOUND_COUNT = 2,
};

// The GUI screens that sim asks about.
enum SimScreenName : unsigned short {
    SIM_SCREEN_BUILDER = 0,
    SIM_SCREEN_COUNT = 1,
};

// The lines that sim puts in the message log; the value is on or off for a toggle, else a number.
enum SimMessageText : unsigned short {
    SIM_MESSAGE_SHARE_METAL_TOGGLED = 0,
    SIM_MESSAGE_SHARE_ENERGY_TOGGLED = 1,
    SIM_MESSAGE_SHARE_MAPPING_TOGGLED = 2,
    SIM_MESSAGE_SHARE_LOS_TOGGLED = 3,
    SIM_MESSAGE_SHARE_RADAR_TOGGLED = 4,
    SIM_MESSAGE_SHARE_METAL_ABOVE = 5,
    SIM_MESSAGE_SHARE_ENERGY_ABOVE = 6,
    SIM_MESSAGE_TEXT_COUNT = 7,
};

#endif
