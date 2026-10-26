# Quirk flags

A quirk flag changes one piece of 3.1c behaviour. Every flag is off by
default, and with every flag off the game traces identically to the
reference. The design is in
[#40](https://github.com/ozgb/byte-tactics-moddable/issues/40), and the
mechanism landed in
[#168](https://github.com/ozgb/byte-tactics-moddable/issues/168).

The list of flags is [`src/quirks/quirks.def`](../src/quirks/quirks.def).
This page describes each one, and gets a row whenever a flag is added there.

## Choosing flags

A flag set is a spec: a preset, then `+flag` or `-flag` words.

- **Presets:** `3.1c` turns every flag off (the default), and `recommended`
  turns every flag on.
- **Examples:** `3.1c +units.skip-empty-yardmap`, or `recommended
  -units.hover-bob-wall-clock`.

You can give the spec in three places:

- **Command line:** `-quirks=<spec>` for a normal run.
- **Test scenario:** a `quirks <spec>` line. When both are given, the
  scenario wins.
- **Suite run:** `uv run tools/reference.py --check --exe <exe> --quirks
  <spec>` plays every scenario with that set.

The set is fixed before the match loads and never changes during it. In test
mode, the game exits with code 2 if the flags change mid-match.

**Scope:** a `sim` flag changes game state. Sim flags go into the flag hash,
and the hash goes into the trace header whenever any flag is on. A `view`
flag only touches presentation or start-up state, so it isn't hashed.

More on the scenario keys: [`reference/README.md`](../reference/README.md),
"Quirk flags".

## Bug fixes

These four fix 3.1c bugs. With the flag on, the game takes the fixed path,
and with it off, it does what 3.1c does. Each fix is written from the bug and
our own reading of the 3.1c code, not from Open Annihilation's (which is
GPL, #45).

### `units.ignore-null-death-record`

- **Scope:** sim.
- **Where:** `ApplyUnitDeath` (0x4866d0), `src/units/units_485010.cpp`.
- **3.1c:** a unit-killed record with unit id 0 resolves to a null unit, and
  the game reads through it and crashes.
- **On:** the record is ignored.
- **Reached by:** only a received packet can carry id 0. The test scenario
  [`quirks-units.ignore-null-death-record.txt`](../reference/tests/quirks-units.ignore-null-death-record.txt)
  forces it.
- **Added in:** [#168](https://github.com/ozgb/byte-tactics-moddable/issues/168)
  (PR [#255](https://github.com/ozgb/byte-tactics-moddable/pull/255)).

### `ai.patrol-null-enemy-skip`

- **Scope:** sim.
- **Where:** the AI's scout squad timer, `SquadScoutTimer::OnTimer`
  (0x407ae0), `src/ai/ai_player.cpp`.
- **3.1c:** when the squad's rally point is the origin and there is no enemy
  left, it sends the squad to the position of a null target, and crashes.
- **On:** the timer returns without sending the squad anywhere.
- **Reached by:** no stock game. The test scenario
  [`quirks-ai.patrol-null-enemy-skip.txt`](../reference/tests/quirks-ai.patrol-null-enemy-skip.txt)
  sets it up.
- **Added in:** [#168](https://github.com/ozgb/byte-tactics-moddable/issues/168)
  (PR [#255](https://github.com/ozgb/byte-tactics-moddable/pull/255)).

### `units.skip-empty-yardmap`

- **Scope:** sim.
- **Where:** `SetYardOpen` (0x47dac0), `src/map/terrain.cpp`.
- **3.1c:** a unit script that sets `YARD_OPEN` on a unit with no yardmap
  (any mobile unit) makes the game read through the null yardmap.
- **On:** the call does nothing for a unit with no yardmap.
- **Reached by:** no stock unit script does this, but a modded one can. The
  test scenario
  [`quirks-units.skip-empty-yardmap.txt`](../reference/tests/quirks-units.skip-empty-yardmap.txt)
  makes the commander's script do it.
- **Added in:** [#168](https://github.com/ozgb/byte-tactics-moddable/issues/168)
  (PR [#255](https://github.com/ozgb/byte-tactics-moddable/pull/255)).

### `util.bound-language-copy`

- **Scope:** view.
- **Where:** `ParseCommandLine` (`src/game/main.cpp`) into
  `g_preferredLanguage`, and `LoadTranslations` (`src/util/tdf.cpp`)
  into `g_language`.
- **3.1c:** both copy a language name into a 64-byte buffer with no bound,
  so a long name on the command line or in the translation file overflows it.
- **On:** both copies stop at 63 bytes plus the terminator.
- **Added in:** [#169](https://github.com/ozgb/byte-tactics-moddable/issues/169)
  (PR [#265](https://github.com/ozgb/byte-tactics-moddable/pull/265)).

## 3.1c's non-deterministic reads

In these two places, 3.1c's game logic reads something other than game
state, so two runs of the same game can differ, and so can two network
peers. The fork's default (flag off) reads game state only. The reference
build behaves the same way in test mode, so traces still match. Turning the
flag **on** brings back 3.1c's original read. The result then depends on the
machine and isn't deterministic.

### `units.render-pose-reads`

- **Scope:** sim.
- **Where:** `GetSweetSpot` (0x43e3c0), `src/units/unit_scripts.cpp`, which
  sets the aim point of every shot at a unit. Also `StartExplodePiece`
  (0x421620), `src/weapons/explosions.cpp`, which places exploded pieces and
  debris.
- **3.1c:** both read the unit's posed model, which only the renderer
  refreshes. Their result therefore depends on which frames were drawn.
- **Off (default):** both re-pose the model from the unit's game state
  before reading it.
- **On:** both read the renderer's copy, as 3.1c does.
- **Added in:** [#113](https://github.com/ozgb/byte-tactics-moddable/issues/113)
  (PR [#280](https://github.com/ozgb/byte-tactics-moddable/pull/280)).

### `units.hover-bob-wall-clock`

- **Scope:** sim.
- **Where:** `AlignUnitToGround` (0x48a490), `src/units/unit_targets.cpp`.
- **3.1c:** a hover unit's bob phase comes from the wall clock. The phase
  feeds the unit's heading, pitch and position.
- **Off (default):** the phase comes from the game tick.
- **On:** the phase comes from the wall clock, through the host interface.
- **Added in:** [#76](https://github.com/ozgb/byte-tactics-moddable/issues/76)
  (PR [#282](https://github.com/ozgb/byte-tactics-moddable/pull/282)).

## Audits

The compiler-warning audit
([#169](https://github.com/ozgb/byte-tactics-moddable/issues/169)) recorded a
verdict for every warning in a game-logic file in
[`data/quirk_candidates.csv`](../data/quirk_candidates.csv). Of its 61
warnings, 56 are not bugs and 5 are harmless bugs that don't reach game
state, so none became a flag.
