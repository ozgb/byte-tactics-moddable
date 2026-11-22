#ifndef INTERFACES_FEATURES_H
#define INTERFACES_FEATURES_H

struct Cell;
struct Feature;
struct Point16;
struct Vec3;
struct TntInfo;
class HapiBank;

// Major version of the features interface, changed when a slot is removed or its signature changes.
#define FEATURES_VERSION_MAJOR 4
// Minor version of the features interface, changed when a slot is added.
#define FEATURES_VERSION_MINOR 0

// The field every features event and effect starts with.
struct FeaturesEventHeader {
    // A FeaturesEventKind or a FeaturesEffectKind.
    int kind;
};

// The kinds of event other modules publish to features' inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published, at once when its route is synchronous; one smaller than its payload changes nothing and has outcome 0.
enum FeaturesEventKind {
    // Puts a feature on a cell (FeaturesPlace); outcome 1 when it took a spot of the pool, 0 when it took none or was refused.
    FEATURES_PLACE = 32,
    // Removes the feature on a cell, or on the cell its footprint starts from (FeaturesRemove); outcome 1 when one was removed.
    FEATURES_REMOVE = 33,
    // A weapon hits the feature whose footprint starts at a cell, which may kill it or set it alight (FeaturesDamage); outcome 1 when the feature was killed.
    FEATURES_DAMAGE = 34,
    // A unit reclaims the feature at a position (FeaturesReclaim); outcome 1 when it did, and the feature's energy and metal go to the unit's account as income.
    FEATURES_RECLAIM = 35
};

// The kinds of effect features publishes to its outbox, for the modules that subscribe to them.
enum FeaturesEffectKind {
    // A model's piece state that a removed feature held is no longer needed and is freed (FeaturesFreeState).
    FEATURES_EFFECT_FREE_STATE = 48,
    // A piece state is built for the model of a feature (FeaturesInitState).
    FEATURES_EFFECT_INIT_STATE = 49,
    // The features on the cells of a footprint changed: the passability of those cells is computed again (FeaturesPassMaps).
    FEATURES_EFFECT_PASS_MAPS = 50,
    // A burning feature deals the area damage of a weapon around a position (FeaturesAreaDamage).
    FEATURES_EFFECT_AREA_DAMAGE = 51
};

// The bits of FeaturesPlace::flags.
enum FeaturesEventFlags {
    // pos is given; without it a placed feature stands at the centre of its footprint.
    FEATURES_HAS_POS = 1,
    // rot is given; without it a placed feature is not rotated.
    FEATURES_HAS_ROT = 2,
    // A placed feature that took a spot starts to sink, as a corpse in water does.
    FEATURES_SINK = 4
};

// The payload of PLACE.
struct FeaturesPlace {
    // The kind.
    FeaturesEventHeader h;
    // The cell's x, in map cells.
    int x;
    // The cell's z, in map cells.
    int z;
    // The feature type, by its number in the match's table of feature types.
    int feature;
    // FeaturesEventFlags bits.
    int flags;
    // The player that owns the feature, 10 for none.
    int owner;
    // The feature's position, in world coordinates with 16 fractional bits, when flags has FEATURES_HAS_POS.
    int pos[3];
    // The feature's rotation, in the game's 16-bit angles x, y and z, when flags has FEATURES_HAS_ROT.
    int rot[3];
};

// The payload of REMOVE.
struct FeaturesRemove {
    // The kind.
    FeaturesEventHeader h;
    // The cell's x, in map cells.
    int x;
    // The cell's z, in map cells.
    int z;
    // 1 to remove even a feature that refuses removal.
    int force;
};

// The payload of DAMAGE.
struct FeaturesDamage {
    // The kind.
    FeaturesEventHeader h;
    // The x of the cell the feature's footprint starts at, in map cells.
    int x;
    // The z of that cell, in map cells.
    int z;
    // The weapon, by the number of its definition.
    int weapon;
};

// The payload of RECLAIM.
struct FeaturesReclaim {
    // The kind.
    FeaturesEventHeader h;
    // The unit that reclaims, as its block offset (an offset from the block base), as economy's events give a unit.
    unsigned int unit;
    // The position of the feature, in world coordinates with 16 fractional bits.
    int pos[3];
};

// The payload of FREE_STATE.
struct FeaturesFreeState {
    // The kind.
    FeaturesEventHeader h;
    // The piece state, as its block offset.
    unsigned int state;
};

// The payload of INIT_STATE.
struct FeaturesInitState {
    // The kind.
    FeaturesEventHeader h;
    // The piece state, as its block offset, allocated by features with the size ObjectStateSize gave.
    unsigned int state;
    // The feature type whose model it is built for.
    int feature;
};

// The payload of PASS_MAPS.
struct FeaturesPassMaps {
    // The kind.
    FeaturesEventHeader h;
    // The x of the footprint's first cell, in map cells.
    int x;
    // The z of the footprint's first cell, in map cells.
    int z;
    // The width of the footprint, in cells.
    int width;
    // The height of the footprint, in cells.
    int height;
};

// The payload of AREA_DAMAGE.
struct FeaturesAreaDamage {
    // The kind.
    FeaturesEventHeader h;
    // The weapon, by the number of its definition.
    int weapon;
    // The centre of the damage, in world coordinates with 16 fractional bits.
    int pos[3];
};

// Map features such as wrecks and trees. The engine delivers features' pending inbox, through Apply, before each run of the features phase (AnimateFeatureType, ScanFeatureCell, UpdateFeatureSpot) and each synchronous slot, and at each phase boundary.
struct FeaturesSeam {
    // Runs the features phase: the engine's scheduler calls the slots below for each feature type, the map scan and each spot of the used list.
    void (__stdcall* UpdateFeatures)(void);
    // Reads the feature file list from the data files at match start. Hooks: none.
    void (__stdcall* LoadFeatureFileList)(void);
    // Releases the feature file list when a match shuts down. Hooks: none.
    void (__stdcall* FreeFeatureFileList)(void);
    // Does nothing in 3.1c, called after the feature file list is read. Hooks: none.
    void (__stdcall* NopAfterFeatureEnum)(void);
    // Links each feature to its dead and burnt versions by name. Hooks: none.
    void (__stdcall* ResolveFeatureLinks)(void);
    // Prepares the pool of feature animations for a map. Hooks: none.
    void (__stdcall* InitFeatureAnimPool)(TntInfo* info);
    // Releases the pool of feature animations. Hooks: none.
    void (__stdcall* FreeFeaturePool)(void);
    // Places the features a mission file lists, at match load. Hooks: none.
    void (__stdcall* PlaceMissionFeatures)(void);
    // Adds the metal of the features to the metal map, at match load. Hooks: none.
    void (__stdcall* StampFeatureMetal)(void);
    // Returns the feature number on a cell, or a code for none or a part of a larger footprint; a query, as of features' last run. Hooks: none.
    unsigned short (__stdcall* GetCellFeature)(Cell* cell);
    // Returns the number of a feature type by name, loading it when it is new; called while unit types load. Hooks: none.
    unsigned short (__stdcall* FindOrLoadFeatureType)(const char* name);
    // Returns the world position of the centre of a feature's footprint; a query. Hooks: none.
    Vec3 (__stdcall* GetFootprintCentre)(Point16* cell, Feature* def);
    // Writes the features to a saved game; it reads only. Hooks: none.
    void (__stdcall* SaveFeatures)(HapiBank* file);
    // Reads the features from a saved game, outside the tick. Hooks: none.
    void (__stdcall* LoadFeatures)(HapiBank* file);
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const FeaturesEventHeader* event, unsigned int size);
    // Synchronous: applies a placement after the engine delivered the pending inbox, and returns its outcome, visible at once; for a caller that needs it in the same statement. Hooks: none.
    int (__stdcall* Place)(const FeaturesPlace* place);
    // The number of feature types, the count of the features phase's first loop; a query. Hooks: none.
    int (__stdcall* FeatureTypeCount)(void);
    // Advances the animation of one feature type for one tick, in the features phase. Hooks: none.
    void (__stdcall* AnimateFeatureType)(int type);
    // Steps the map scan by one cell, where a feature may seed another nearby, once per features phase. Hooks: none.
    void (__stdcall* ScanFeatureCell)(void);
    // The first spot of the used list, or -1 when it is empty; a query, read once at the start of the spot loop. Hooks: none.
    int (__stdcall* FirstFeatureSpot)(void);
    // Advances one spot of the used list for one tick (a feature that falls, burns or dies) and returns the spot after it, read before the update, or -1. Hooks: none.
    int (__stdcall* UpdateFeatureSpot)(int spot);
    // Returns the feature on the cell under a position (the cell that holds the feature when the position is on a part of a larger footprint), filling in that cell and the feature's footprint when they are not null, or 0xffff when there is none; a query, as of features' last run. Hooks: none.
    unsigned short (__stdcall* FindFeatureAtPos)(Vec3* pos, Point16* cell, Point16* size);
};

#endif
