#ifndef INTERFACES_QUIRKS_H
#define INTERFACES_QUIRKS_H

// Major version of the quirks interface, changed when a flag is removed or reordered.
#define QUIRKS_VERSION_MAJOR 1
// Minor version of the quirks interface, changed when a flag is appended at the end.
#define QUIRKS_VERSION_MINOR 0

// The quirk flags of a match, one bool per flag in the order of quirks.def; a new flag goes at the end.
struct Quirks {
    // units.ignore-null-death-record (sim): ignore a unit-killed record with unit id 0.
    bool ignoreNullDeathRecord;
    // ai.patrol-null-enemy-skip (sim): the scout squad timer sends nothing when no enemy is left.
    bool patrolNullEnemySkip;
    // units.skip-empty-yardmap (sim): setting YARD_OPEN on a unit with no yardmap does nothing.
    bool skipEmptyYardmap;
    // util.bound-language-copy (view): a language name copied into its 64-byte buffer stops at 63 bytes.
    bool boundLanguageCopy;
    // units.render-pose-reads (sim): piece queries read the renderer's posed model, as 3.1c does, instead of re-posing it from game state.
    bool renderPoseReads;
    // units.hover-bob-wall-clock (sim): a hover unit's bob phase comes from the wall clock.
    bool hoverBobWallClock;
    // frontend.bound-logo-select (view): the list of free logos stops at 24, so every index fits its array.
    bool boundLogoSelect;
};

#endif
