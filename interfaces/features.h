#ifndef INTERFACES_FEATURES_H
#define INTERFACES_FEATURES_H

struct Cell;
struct Feature;
struct Point16;
struct Vec3;
struct TntInfo;
class HapiBank;

// Major version of the features interface, changed when a slot is removed or its signature changes.
#define FEATURES_VERSION_MAJOR 3
// Minor version of the features interface, changed when a slot is added.
#define FEATURES_VERSION_MINOR 0

// The kinds of event other modules publish to features' inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published, at once when its route is synchronous.
enum FeaturesEventKind {
    // Puts feature on cell (x, z), at pos and rot when flags say so, for owner; outcome 1 when it took a spot of the pool, 0 when it took none or was refused.
    FEATURES_PLACE = 32,
    // Removes the feature on cell (x, z), or on the cell its footprint starts from; value 1 removes even a feature that refuses removal; outcome 1 when one was removed.
    FEATURES_REMOVE = 33,
    // Weapon (value, the number of its definition) hits the feature whose footprint starts at cell (x, z), which may kill it or set it alight; outcome 1 when the feature was killed.
    FEATURES_DAMAGE = 34,
    // The unit (unit, its block offset) reclaims the feature at pos; outcome 1 when it did, and the feature's energy and metal go to the unit's account as income.
    FEATURES_RECLAIM = 35
};

// The kinds of effect features publishes to its outbox, for the modules that subscribe to them.
enum FeaturesEffectKind {
    // A model's piece state that a removed feature held, state (its block offset), is no longer needed and is freed.
    FEATURES_EFFECT_FREE_STATE = 48,
    // The piece state at state (its block offset, allocated by features with the size ObjectStateSize gave) is built for the model of feature.
    FEATURES_EFFECT_INIT_STATE = 49,
    // The features on the cells of the footprint at (x, z), width by height cells, changed: the passability of those cells is computed again.
    FEATURES_EFFECT_PASS_MAPS = 50,
    // A burning feature deals the area damage of weapon (value, the number of its definition) around pos.
    FEATURES_EFFECT_AREA_DAMAGE = 51
};

// The bits of FeaturesEvent::flags.
enum FeaturesEventFlags {
    // pos is given; without it a placed feature stands at the centre of its footprint.
    FEATURES_HAS_POS = 1,
    // rot is given; without it a placed feature is not rotated.
    FEATURES_HAS_ROT = 2,
    // A placed feature that took a spot starts to sink, as a corpse in water does.
    FEATURES_SINK = 4
};

// An event (FeaturesEventKind) or an effect (FeaturesEffectKind), as plain data with no pointers; fields a kind does not use are 0.
struct FeaturesEvent {
    // A FeaturesEventKind or a FeaturesEffectKind.
    int kind;
    // FeaturesEventFlags bits.
    int flags;
    // The cell, in map cells: the one a kind acts on, or the first cell of a footprint.
    int x, z;
    // The feature type, by its number in the match's table of feature types.
    int feature;
    // The player that owns a placed feature, 10 for none.
    int owner;
    // The unit that reclaims, as its block offset (an offset from the block base, 0 for none), as economy's events give a unit.
    unsigned int unit;
    // The argument of the kind, as the kind says.
    int value;
    // A model's piece state, as its block offset.
    unsigned int state;
    // The size of a footprint, in cells.
    int width, height;
    // A position, in world coordinates with 16 fractional bits.
    int pos[3];
    // A rotation, in the game's 16-bit angles, x, y and z.
    int rot[3];
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
    unsigned short (__stdcall* FindOrLoadFeatureType)(char* name);
    // Returns the world position of the centre of a feature's footprint; a query. Hooks: none.
    Vec3 (__stdcall* GetFootprintCentre)(Point16* cell, Feature* def);
    // Writes the features to a saved game; it reads only. Hooks: none.
    void (__stdcall* SaveFeatures)(HapiBank* file);
    // Reads the features from a saved game, outside the tick. Hooks: none.
    void (__stdcall* LoadFeatures)(HapiBank* file);
    // Consumes one queued event of the inbox and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const FeaturesEvent* event);
    // Synchronous: applies a FEATURES_PLACE after the engine delivered the pending inbox, and returns its outcome, visible at once; for a caller that needs it in the same statement. Hooks: none.
    int (__stdcall* Place)(const FeaturesEvent* place);
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
};

#endif
