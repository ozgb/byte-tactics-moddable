#ifndef INTERFACES_QUIRKS_H
#define INTERFACES_QUIRKS_H

// Major version of the quirks interface, changed when a flag is retired: it keeps its slot under a retired name, since a flag never moves.
#define QUIRKS_VERSION_MAJOR 1
// Minor version of the quirks interface, changed when a flag is appended at the end.
#define QUIRKS_VERSION_MINOR 1

// The quirk flags of a match, one int per flag (0 or 1) in the order of quirks.def; a new flag goes at the end, and no flag moves or goes away.
struct Quirks {
    // units.ignore-null-death-record (sim): ignore a unit-killed record with unit id 0.
    int ignoreNullDeathRecord;
    // ai.patrol-null-enemy-skip (sim): the scout squad timer sends nothing when no enemy is left.
    int patrolNullEnemySkip;
    // units.skip-empty-yardmap (sim): setting YARD_OPEN on a unit with no yardmap does nothing.
    int skipEmptyYardmap;
    // util.bound-language-copy (view): a language name copied into its 64-byte buffer stops at 63 bytes.
    int boundLanguageCopy;
    // units.render-pose-reads (sim): piece queries read the renderer's posed model, as 3.1c does, instead of re-posing it from game state.
    int renderPoseReads;
    // units.hover-bob-wall-clock (sim): a hover unit's bob phase comes from the wall clock.
    int hoverBobWallClock;
    // frontend.bound-logo-select (view): the list of free logos stops at 24, so every index fits its array.
    int boundLogoSelect;
    // players.rank-compaction-per-tick (sim): the tick compacts the ranks of the score panel once, instead of each drawn frame of the panel.
    int rankCompactionPerTick;
};

#endif
