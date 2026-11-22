#ifndef INTERFACES_WIND_METEORS_H
#define INTERFACES_WIND_METEORS_H

#include "rand_stream.h"

class HapiBank;

// Major version of the wind_meteors interface, changed when a slot is removed or its signature changes.
#define WIND_METEORS_VERSION_MAJOR 3
// Minor version of the wind_meteors interface, changed when a slot is added.
#define WIND_METEORS_VERSION_MINOR 1

// The meteor showers of a map, as plain data: the map's own figures or the default data file's.
struct MeteorShowerParams {
    // The name of the meteor weapon, ended by a 0.
    char weapon[32];
    // How far from the centre of a shower a meteor may fall (the map's MeteorRadius).
    int radius;
    // Meteors per second while a shower lasts.
    float density;
    // How long a shower lasts, in seconds.
    float duration;
    // The time from the end of one shower to the start of the next, in seconds.
    float interval;
};

// The field every wind_meteors event and effect starts with.
struct WindMeteorsEventHeader {
    // A WindMeteorsEventKind or a WindMeteorsEffectKind.
    int kind;
};

// The kinds of event the engine publishes to wind_meteors' inbox; each is consumed by Apply in the order it was published, at once outside a tick; one smaller than its payload changes nothing.
enum WindMeteorsEventKind {
    // Meteor showers fall from the next one due; the header alone.
    WIND_METEORS_ENABLE = 64,
    // A shower that falls due from now on drops no meteor, though its centre and the next one's time are still drawn and set; one already falling ends as planned; the header alone.
    WIND_METEORS_DISABLE = 65,
    // Sets the meteor weapon's name and the shower timing (WindMeteorsParams); InitMeteors looks the weapon up.
    WIND_METEORS_PARAMS = 66,
    // Starts a shower now, centred on a random cell, whether showers are enabled or not; the header alone.
    WIND_METEORS_START = 67,
    // Sets the wind speed limits of the map (WindMeteorsLimits), when the map loads.
    WIND_METEORS_LIMITS = 68,
    // Sets the wind's speed divisor to 5000 and its timer to 0, at match load; the header alone.
    WIND_METEORS_RESET = 69
};

// The kinds of effect wind_meteors publishes to its outbox, for the modules that subscribe to them.
enum WindMeteorsEffectKind {
    // A meteor falls: a projectile of the meteor weapon starts at a position with a velocity (WindMeteorsSpawn).
    WIND_METEORS_EFFECT_SPAWN = 80
};

// The payload of PARAMS.
struct WindMeteorsParams {
    // The kind.
    WindMeteorsEventHeader h;
    // The parameters it sets.
    MeteorShowerParams params;
};

// The payload of LIMITS.
struct WindMeteorsLimits {
    // The kind.
    WindMeteorsEventHeader h;
    // The slowest wind of the map.
    int min;
    // The fastest wind of the map.
    int max;
};

// The payload of SPAWN.
struct WindMeteorsSpawn {
    // The kind.
    WindMeteorsEventHeader h;
    // The meteor weapon, as its slot in the match's table of weapons.
    int weapon;
    // The position the projectile starts at, in world units: x, y and z.
    int pos[3];
    // The projectile's velocity per tick, in world units: x, y and z.
    int vel[3];
    // 1 when the spawn is announced to the other players of a network game.
    int announce;
};

// Wind and meteors: the wind the Game record shows and the meteor schedule. The engine delivers wind_meteors' pending inbox, through Apply, before each run (UpdateWind, UpdateMeteors) and at each phase boundary.
struct WindMeteorsSeam {
    // Runs the wind pass on a stream: once a tick in the wind phase (main stream) and once at match load (load stream); the wind it writes to the Game record is visible when it returns. Hooks: none.
    void (__cdecl* UpdateWind)(RandStream stream);
    // Runs the meteors pass, once a tick in the meteors phase; a meteor that falls is published as a SPAWN effect. Hooks: none.
    void (__stdcall* UpdateMeteors)(void);
    // Consumes one event of the inbox, its payload of size bytes; called by the engine only, and the event's effect is visible when it returns. Hooks: none.
    void (__stdcall* Apply)(const WindMeteorsEventHeader* event, unsigned int size);
    // Resets the meteor schedule and looks up the meteor weapon by name, at match start, outside the tick. Hooks: none.
    void (__stdcall* InitMeteors)(void);
    // Called when a match shuts down, outside the tick; does nothing in 3.1c. Hooks: none.
    void (__stdcall* EmptyShutdownPreCleanup)(void);
    // A query: fills params from the default meteor data file, for a map that gives no showers of its own; changes no state. Hooks: none.
    void (__stdcall* LoadMeteorDefaults)(MeteorShowerParams* params);
    // Writes the meteor state to the "Meteor" account of a saved game, outside the tick. Hooks: none.
    void (__stdcall* SaveMeteors)(HapiBank* file);
    // Reads the meteor state from the "Meteor" account of a saved game, outside the tick; visible when it returns. Hooks: none.
    void (__stdcall* LoadMeteors)(HapiBank* file);
};

#endif
