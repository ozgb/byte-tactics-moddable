# Reference build and scenarios

The reference is the matching build: the source compiled with Visual C++ 5.0,
plus the harness in `src/harness/harness.cpp`. It is built on the `reference`
branch, cut from `moddable` at 875d19d5c03eb254d636077d0f0d3cd86e08ced2, with
that branch's own `tools/reference.py HEAD` (the original compiler under Wine,
and `reference/changed.txt` and checkall as the allowlist). `moddable` has no
MSVC 5 compiler, linker or Wine wrapper. It builds with clang and LLVM 22.1.8
against MSVC 5's `INCLUDE` and `LIB` directories, and
`build/modern/TotalA.exe` is what the gate checks.

A harness or engine change lands on both branches: cherry-pick the same commit
to `reference`, run `uv run tools/reference.py HEAD --traces` there, and tag a
new `reference-traces-v<n>` when a digest changes. `uv run tools/reference.py --traces`
on `moddable` does the second step in a worktree of `origin/reference`
(`build/reference/branch`) and copies the traces into `reference/traces/`.

`--traces` plays `moddable`'s scenarios: it copies this tree's
`reference/scenarios/` into the branch worktree, replacing the branch's set, so
a new scenario needs no cherry-pick to `reference`. It refuses to replace
`reference/traces/` unless the trace names equal the scenario names, and the
error lists the missing and the extra ones.

## Running a scenario

The exe takes a scenario file with `-testmode`. It starts a skirmish from the
file without menus or input, plays `ticks` game ticks, and exits with status 0.
A bad scenario or a failed check exits with status 2 and a message on stderr
that names the line.

```sh
build/modern/TotalA.exe -testmode reference/scenarios/skirmish2.txt -memset=0
WINEARCH=win64 uv run tools/playtest.py --exe build/modern/TotalA.exe \
  --testmode reference/scenarios/skirmish2.txt     # under Wine, headless
```

A `-testmode` run uses the game's headless host: it opens no window and no
DirectDraw or DirectSound device, so it needs no X display and no sound device
(`playtest.py --testmode` starts no Xvfb or audio sink and needs only `wine`).

The FPU check ignores the infinity-control bit, which no x87 since the 80387
uses, so both `0x9001F` and Wine's `0xD001F` pass.

## Scenario format

One `key value` per line. Blank lines and lines starting with `#` are ignored.
Numbers are decimal, `0x` hex or `0` octal. Unknown keys and values that do not
parse are errors. A later line for the same key replaces an earlier one.
Defaults are the ones in `LoadSettings`, never the registry.

| Key | Default | Meaning |
| --- | --- | --- |
| `game_seed` | 1 | Seed of the game-logic random generator |
| `crt_seed` | 1 | Seed of the C runtime `rand` |
| `view_seed` | 1 | Seed of the presentation random generator (screen shake, particles, menu effects) |
| `fpu_control_word` | `0x9001F` | Expected `_control87` value, ignoring the infinity-control bit; the run exits 2 if it differs |
| `ticks` | required | Game ticks to play (30 per second of game time) |
| `steps_per_frame` | 1 | Game ticks run between two drawn frames, 1 to 8; the traces must not depend on it |
| `quirks` | `3.1c` | Quirk flags for the run, a spec: a preset (`3.1c`, all off; `recommended`, all on) then `+flag` or `-flag` words, as in `recommended -flag` |
| `quirks_tamper` | none | Test only: flips a quirk flag at this tick, so the freeze check exits 2 (see Quirk flags) |
| `quirks_reach` | none | Test only: `<tick> <flag name>`, at that tick makes the game reach the guard of `units.ignore-null-death-record`, `ai.patrol-null-enemy-skip` or `units.skip-empty-yardmap` (see Quirk flags) |
| `save` | none | Saves the game with `SaveGameFile` to `savegame\harness.sav` in the run's game directory after the step of this tick, 1 to `ticks`; one line only |
| `game_type` | 2 | 2: skirmish, 3: multiplayer without a network; one line only (see Game types). Type 1, a campaign mission, needs the `campaign` key and comes with #444 |
| `map` | required | Map name, the rest of the line (names have spaces) |
| `difficulty` | 1 | AI difficulty 0 to 2, one value for all computer players |
| `commander_death` | 1 | 1: the game ends when a commander dies |
| `mapping` | 1 | 1: terrain is blacked out until explored (the skirmish menu's "Terrain is blacked out until explored"); 0: the whole map is shown |
| `line_of_sight` | 1 | 1: line of sight is on |
| `los_type` | 1 | Line of sight type, 0 or 1 |
| `start_positions` | `fixed` | `fixed`: player N starts at position N; `random`: shuffled |
| `local_player` | the first human player, else 0 | Slot whose view the game shows; must be a slot some `player<N>.*` key names |
| `player<N>.controller` | `ai` | `ai` or `human` (a human does nothing) |
| `player<N>.side` | N mod 2 (0 is Arm, 1 is Core) | Side name from `sidedata.tdf`, any case |
| `player<N>.team` | 5 | Ally group 0 to 4; 5 is no team |
| `player<N>.color` | N | Colour index 0 to 9 |
| `player<N>.metal` | 1000 | Starting metal |
| `player<N>.energy` | 1000 | Starting energy |

`player<N>.*` keys take N from 0 to 9. A slot exists when any key names it,
slots no key names are open, and at least 2 slots must exist. The game's player
count is the highest named slot plus one.

## Game types

`game_type` is the number the game keeps as `Mission::type`. With 2, the default,
a scenario is a skirmish and nothing below applies.

**Game type 1**, a mission of a campaign, is not accepted yet: it needs the `campaign` key, which comes with #444.

**Game type 3** runs the multiplayer rules without a network: the harness fills in
the lobby state the exe expects (every named slot is a connected player, the
local slot is the host, `player<N>.team` makes the alliances and both sides of
each alliance agree to it) and starts the loading screen itself, so the lobby is
skipped. `commander_death`, `mapping`, `line_of_sight`, `los_type`, `start_positions`
and the `metal` and `energy` of the local slot (which sets every player's start) come
from the scenario as in a skirmish.

## Game over

A run ends at its last tick, or earlier at game over. When 3.1c sets
`endGameFlags` to 4 or 0x10 (a victory or defeat after its countdown, a
commander's death, the last enemy gone) the harness ends the run at the end of
that tick's step, as at the last tick: it closes `trace.txt`, writes the block
scan, logs `testmode: game over at tick <n>` and exits 0. `ticks` stays the
upper bound. The trace holds no marker: it is shorter than `ticks`, and a build
that ends the game on another tick differs from the reference in length, which
the comparison reports as a divergence at the first missing row.

The run ends before 3.1c's own frame code sees the flag, which would shut the
game's systems down and switch to the end-of-game screen. That way every tick that
ran is traced, and the run never reaches the statistics screen (the
presentation, not game logic).

## Quirk flags

The flags are listed in `src/quirks/quirks.def`. A normal run takes a spec with the `-quirks=<spec>`
switch, in the same syntax as the `quirks` key: a preset, then `+flag` or `-flag` words. With
no spec every flag is off, which is 3.1c. In test mode the scenario's `quirks` key sets the flags.

In test mode the flags are frozen: the exe checks them before every tick, and if they changed it exits
2 with `testmode: quirk flags changed at tick <n>`. `reference/tests/quirks-tamper.txt` (a
`quirks_tamper` line) trips it on purpose, and is expected to exit with that message:

```sh
uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/tests/quirks-tamper.txt --game-dir ~/.ta
```

No stock game data reaches the guards of the first three flags, so a `quirks_reach <tick> <flag name>`
line makes the exe reach one at that tick: `units.ignore-null-death-record` applies a unit-killed
record whose unit id is 0, `units.skip-empty-yardmap` runs the commander's COB `set YARD_OPEN`, and
`ai.patrol-null-enemy-skip` puts a unit in the first computer player's scout squad, sets its rally
point to the origin and runs its scout timer with every player allied to it, so no enemy is found.
`reference/tests/quirks-<flag name>.txt` does this for each flag, with `quirks 3.1c +<flag name>`:

```sh
uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/tests/quirks-units.ignore-null-death-record.txt --game-dir ~/.ta
uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/tests/quirks-ai.patrol-null-enemy-skip.txt --game-dir ~/.ta
uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/tests/quirks-units.skip-empty-yardmap.txt --game-dir ~/.ta
```

Each runs to the last tick. With the `quirks` line removed, the exe faults at the guard's tick, as 3.1c
does, and the run stops before the last tick.

## Scripted orders

A scenario can order the units of a `controller human` slot (a human otherwise
does nothing). One `order` line per order; lines may repeat and accumulate.

```
order <tick> <unit selector> <kind> <target>
order 5 0:ARMCOM:1 move 700 300
order 5 0:ARMCOM:1 build ARMSOLAR 640 260
order 150 0:ARMCOM:1 reclaim 0:ARMSOLAR:1
```

- `<tick>` is a game tick from 1 to `ticks`. The order is issued at the start of
  that tick, before the first phase, so the trace sees its effect in the same
  tick. Orders on one tick are issued in file order.
- A unit selector is `<slot>:<unit type>:<n>`, the n-th live unit of that type
  owned by that slot in pool-index order (the unit type name is the unit's name
  in the game data, any case), or `<slot>:all`, all of that slot's live units.
- A target is `x z` in world units, or a unit selector that names one unit
  (`<slot>:all` is not allowed there). With a unit selector the order is
  issued as if the cursor were over that unit.
- Kinds: `move`, `attack`, `patrol`, `guard` and `reclaim` take a target.
  `build <unit type> x z` takes the unit type before the position.
- More kinds that take a target, as the order buttons and the cursor modes do:
  `dgun` (special attack), `unload` (position), `load` (unit), `repair` (unit),
  `teleport` (position) and `capture` (unit).
- Kinds that take no target, as the order panel buttons and the self-destruct
  key do: `stop`, `activate`, `deactivate`, `selfdestruct` (a toggle: it cancels
  the order if a selected unit already has one), `cloak on` and `cloak off`, and
  `movestate <n>` and `firestate <n>`, where n is 0, 1 or 2, the value the
  button passes to the standing order.
- `qbuild <unit type> [n]` adds n (default 1, at most 99) of a unit type to the
  build queue of each unit the selector names, as a click in a factory's build
  menu does. The selector names the factory; the order has no target.
  `qbuild MAKENUKE` and `qbuild MAKEANTI` queue a missile, as the weapon-build
  button does; the name goes to the game as written, with no unit type lookup.
- A `+` before the kind (`+move 700 300`) is the shift key: the order joins the
  queue instead of replacing the orders, and repeating an identical queued
  order cancels it.

Each order is issued as a mouse click does: the harness selects the named
units, sets the order mode and cursor position, and calls the game's
`IssueOrderToSelection` (`build` calls `IssueMobileBuildOrders`, as a click on a
build site does, and `qbuild` calls `QueueBuildOrder`). The kinds without a
target call it with the order's mission type, as the buttons do. The selection is put back afterwards.

A scenario can also create units. One `spawn` line per unit:

```
spawn <tick> <slot> <unit type> <x> <z>
spawn 10 1 ARMLAB 640 260
```

- The unit is created at the start of `<tick>`, before that tick's orders, so an
  `order` on the same tick can select it. `<slot>` is a player the scenario
  names (it need not be `human`), `<unit type>` is one exact unit type name (no
  wildcard) and `x z` is a world position, with the height of the ground there.
- It is what the console cheat does for one unit: the position is snapped to
  the unit's footprint, then the unit is created for the slot. A spawned unit is
  created finished, as the console cheat makes it, so a scenario that spawns
  tests what the unit does, not how it was built. The tech chain is tested by
  the scenarios that build.

A bad selector, kind or position, a slot that is not `human` (a `spawn` needs only a named slot), a tick after the
last tick, an unknown unit type, a build site the game rejects, or a selector
that names no live unit at its tick exits with status 2 and a message that
names the line.

## Traces

In test mode the exe writes `trace.txt` in its working directory: a hash of the
game state after each phase of each tick, then a hash of the world tables, flushed
every tick. LF line endings. The header lines, the phases and their order, what each
row covers, the hash and the pointer IDs are defined in
[docs/trace-format.md](../docs/trace-format.md); the hashed fields are
`data/trace_schema.json`. Two builds conform when their traces are identical.

When any quirk flag is on, a `quirks <8 hex digits hash> <spec>` line follows
`fpu_control_word`. With every flag off the line is absent, so the traces of 3.1c do
not change.

`tools/playtest.py --testmode` copies `trace.txt` (and `dump.txt`) into the
run's directory under `build/playtest/`, and copies `commit.txt` from the exe's
directory into the game directory for the header.

## Dumps

`-testdump <tick>` (`--testdump <tick>` for playtest.py, with `-testmode`) also
writes `dump.txt`: every hashed field after each phase of that tick, the `world`
tables included. It is ignored without `-testmode`; a value that does not parse
exits with status 2. A dump of a large map is large: the cells, the explored
grids and the pass maps are one line per record. The line format, the paths and
the pointer IDs (`unit:12`, `heap`, `null`, and so on) are in
[docs/trace-format.md](../docs/trace-format.md), "dump.txt" and "Pointer IDs".

A field that differs between two builds is the same `<tick> <phase> <path>`
with another `<value>`.

## Comparing two builds

```sh
WINEARCH=win64 uv run tools/tracediff.py A/TotalA.exe B/TotalA.exe \
  --scenario reference/scenarios/skirmish2.txt
```

It runs both exes, reports the first tick and phase whose hashes differ, then
reruns both with `--testdump` at that tick and lists the fields that differ:

```
first divergence: tick 900, phase units
  phase function: UpdateAllUnits (FUNCTION: 0x48ad30)
  first differing field: units[12].health: a=0x0064 b=0x0063
  all 1 differing fields:
    units[12].health: a=0x0064 b=0x0063
```

The phase function is the one `RunGameSteps` calls just before the phase's hash
(`PHASE_FUNCTIONS` in `tools/tracediff.py`): `net` `HandleNetPackets`, `units`
`UpdateAllUnits`, `projectiles` `UpdateProjectiles`, `explosions`
`UpdateExplosions`, `players` `UpdatePlayers`, `features` `UpdateFeatures`,
`wind` `UpdateWind` and `meteors` `UpdateMeteors`. The `world` row follows `UpdateMeteors`
and has no function of its own.

Traces with different scenario or seeds are refused (exit 2).
Exit status is 0 for identical traces, 1 for a divergence and 2 for an error.
`--traces A B` compares existing `trace.txt` files without running, and
`--dumps A B --at <tick> <phase>` diffs two `dump.txt` files.

## The suite

`reference/scenarios/` holds the scenario suite: AI-versus-AI games (and the three `orders-` scenarios, which script a human slot) that vary the
map, the player count (2, 3, 4, 6, 8 and 10), the sides, ally teams, fixed and
random start positions, the options (difficulty, line of sight, mapping,
starting resources) and the length, up to 54000 ticks (30 minutes). Each has its
own game, C runtime and view seeds (the first three predate the suite and share
seeds) and runs deterministically: two runs give identical traces.

| Name | Map | Players | Ticks | Covers |
| --- | --- | ---: | ---: | --- |
| allied-victory | Metal Heck | 4 | 2400 | Game type 3: players 0 and 1 allied against players 2 and 3, whose commanders self-destruct, so allied victory ends the run early |
| commander-death | The Pass | 2 | 1800 | The human Arm commander self-destructs, so commander death ends the run about 150 ticks later |
| duel-coast-to-coast | Coast To Coast | 2 | 5400 | Two Core players on Coast To Coast, three minutes, difficulty 2 |
| duel-great-divide | Great Divide | 2 | 3600 | Two Arm players on Great Divide, random start positions, two minutes |
| duel-pass-dark | The Pass | 2 | 2700 | Two players on The Pass with the map unmapped and line of sight off, ninety seconds |
| duel-shore-to-shore | Shore to Shore | 2 | 2700 | Arm against Core on Shore to Shore with 5000 metal and energy, no commander death, difficulty 0 |
| eight-full-moon | Full Moon | 8 | 9000 | Eight players on Full Moon, alternating sides, five minutes |
| eight-greenhaven | Greenhaven | 8 | 5400 | Eight Arm players on Greenhaven, random start positions, three minutes |
| eight-pincushion-teams | Pincushion | 8 | 12600 | Eight players on Pincushion in four allied pairs, seven minutes |
| long-great-divide-2p | Great Divide | 2 | 54000 | Two players on Great Divide for 54000 ticks (thirty minutes), a long run that no commander death ends |
| long-greenhaven-8p | Greenhaven | 8 | 54000 | Eight players on Greenhaven in two allied teams of four for 54000 ticks (thirty minutes), a long run |
| orders-build-patrol | The Pass | 2 | 600 | Human Arm commander builds a solar collector, then patrols (`build`, `patrol`) |
| orders-guard-reclaim | The Pass | 2 | 600 | Human Arm commander builds a solar collector, guards it, then reclaims it (`build`, `guard`, `reclaim`) |
| orders-move-attack | The Pass | 2 | 600 | Human Arm commander moves, attacks the Core commander, then the whole slot moves (`move`, `attack`, `<slot>:all`) |
| quad-fox-holes | Fox Holes | 4 | 7200 | Four players on Fox Holes, alternating sides, four minutes |
| quad-king-of-the-hill | King of the Hill | 4 | 9000 | Four players on King of the Hill in two allied teams of two, five minutes |
| quad-lava-highground | Lava Highground | 4 | 5400 | Four Arm players on Lava Highground, random start positions, three minutes |
| quad-two-continents | Two Continents | 4 | 6000 | Four players on Two Continents in teams of players 0 and 3 against 1 and 2, random start positions, 100 metal and energy |
| six-hundred-isles | Hundred Isles | 6 | 9000 | Six players on Hundred Isles, alternating sides, five minutes |
| six-painted-desert | Painted Desert | 6 | 12600 | Six Core players on Painted Desert, random start positions, seven minutes |
| six-yerrot-mountains | Yerrot Mountains | 6 | 7200 | Six players on Yerrot Mountains in two allied teams of three, four minutes |
| skirmish2 | The Pass | 2 | 1800 | Two computer players on a small map, one minute of game time |
| skirmish4-long | Metal Heck | 4 | 27000 | Same as skirmish4 for 27000 ticks: reaches past tick 25786, where a hover unit's ground bob once read the wall clock |
| skirmish4 | Metal Heck | 4 | 9000 | Four computer players, one team each, five minutes of game time |
| ten-lava-run | Lava Run | 10 | 5400 | Ten players on Lava Run, alternating sides, three minutes |
| ten-ring-atoll | Ring Atoll | 10 | 9000 | Ten players on Ring Atoll, random start positions, five minutes, difficulty 2 |
| trio-anteer-strait | Anteer Strait | 3 | 5400 | Three players on Anteer Strait, players 0 and 1 allied, three minutes, line of sight type 0 |
| trio-dark-side | Dark Side | 3 | 9000 | Three players on Dark Side, fixed start positions, five minutes |
| trio-etorrep-glacier | Etorrep Glacier | 3 | 3600 | Three Core players on Etorrep Glacier, random start positions, two minutes |

## Stored traces and digests

`reference/traces/<scenario>.txt` holds one digest per tick, not the phase
hashes: the digest of a tick is computed from the tick's rows as
[docs/trace-format.md](../docs/trace-format.md), "The per-tick digest", says. In
test mode there are 8 rows per tick (the 7 phases `units` to `meteors`, since `net`
does not run, then `world`).

`tools/tracedigest.py trace.txt` converts a `trace.txt` to this form.

## Gate

```sh
export WINEARCH=win64
cmake -S . -B build/modern -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-i686.cmake   # once
cmake --build build/modern
rm -r build/playtest/prefix     # a fresh prefix; tools/playtest.py makes it again
uv run tools/reference.py --check --exe build/modern/TotalA.exe --game-dir ~/.ta   # the gate
```

The gate is the modern build's `--check`: every scenario must report equal.
`--check` without `--exe` is an error, since `moddable` does not build the
reference. Add `--game-dir <dir>` when the game is not in a Steam directory.

The gate applies when a pull request changes `src/`, `include/`,
`lib/bitexact/`, `reference/`, or the tools that build or run the exe
(`tools/reference.py`, `tools/playtest.py` and the like). Other
changes do not need it.

`--traces` writes every trace with the reference branch's sha as `commit`. `--check` plays
every scenario (`--parallel N` at a time, default half the CPUs and at most 6), prints one line per scenario with its run time
and a total, and exits 0 when every digest equals the stored one. It exits 1
when a trace has no scenario or a scenario has no trace, and on a difference:
it builds the reference from the commit the trace records (a commit of the
`reference` branch or of `moddable` before the cut, built with its own tools and
cached in `build/reference/<sha>`), prints the `tools/tracediff.py` report for the first
differing scenario and exits 1. `--check --exe <path>` checks the given exe, and its report is `tracediff.py` run on the reference exe and
that exe: the first differing tick, phase, field and the function the phase runs.
A differing scenario's run is kept: its `trace.txt` (and `dump.txt`, if any) is copied to
`build/scratch/check-<scenario>-<time>/`, and the `DIFF` line ends with that path, so a one-off
divergence can be diagnosed later with the file mode of `tools/tracediff.py` against a good run.

`tools/playtest.py` polls every 5 seconds for an error dialog and fails the run at once, with the
kind in the reason, `result.json` and `failed.png`: `dialog-directx` (a dialog-sized window before
tick 1: "Environment Initialization Failed"), `dialog-crt` (the "Microsoft Visual C++ Runtime
Library" box, as for R6025) or `dialog-cavedog` (the fatal error box). `reference.py` tries a run
once more after a nonzero exit or `dialog-directx`; any other dialog, a hang and a failure after
tick 1 are reported as they are.

`--check --steps-per-frame <n>` (1 to 8) plays every scenario with that `steps_per_frame` and compares with
the stored traces; it is a check on presentation code, not the gate.

`--check --exe <exe> --quirks <spec>` plays every scenario with a `quirks <spec>` line added to a
temporary copy (before the first `order` line), and prints one line per scenario: `equal` (the digests
equal the stored ones; the header's `quirks` line is not compared), `differs` with the first differing
tick and phase with the function the phase runs, or `FAIL` for a crash, a hang, a dialog or a nonzero
exit. The stored digests have no phases, so each differing scenario is played again with every flag off
(the gate proves that equals the reference) and the two `trace.txt` files are compared; both run
directories are kept (`build/scratch/check-<scenario>-<time>/` and `check-<scenario>-alloff-<time>/`) and the line ends with their paths. If that rerun fails, or
differs from the stored trace (the exe does not pass the gate, which is not the flag's effect), the scenario is `FAIL`.
The total counts equal, differ and failed. A difference is not a failure: the exit status is 1 only when a
scenario failed (or a trace has no scenario). It builds no reference, and it is not the gate. It
works with `--quick`, `--steps-per-frame` and `--parallel`, and not with `--poison`.

The full `--check` is what a pull request's gate needs.

## Block pointers

`-blockscan` (`--blockscan` for playtest.py, with `-testmode`) makes the game list the pointers out of the shared
state block. At the end of a scanned tick, after the last phase, it walks every live allocation of the block and
the `SimGlobals` struct, reads a 4-byte word at every byte offset (the packed structs keep pointers at odd
offsets, such as `Unit.state` at 0x9e), and keeps the ones that point into a live heap allocation outside the
block. `-blockscan` scans every tick; `-blockscan=N` (playtest.py `--blockscan-every N`) scans the ticks
divisible by N and every tick of the last 64. When the run ends it writes `blockptrs.txt` beside `trace.txt`:
tab-separated, a header (`bt-blockptrs 2`, `commit`, `every`, `ticks`, the ticks scanned) and one row per distinct (holder site, holder tag, offset,
target site, target tag) with the holder (`SimGlobals` or the allocation site, a return address in the exe),
the offset in the holder, the target site, the ticks it was seen in and the smallest and largest offset it
pointed at in the target. The scan does not change the traces.

`reference.py --check` always runs with the scan on, every 8th tick and every tick of the last 64 (`-blockscan=8`),
resolves the sites with the linker map beside the exe
(`build/modern/TotalA.map`) and compares every row with `data/block_pointers.csv`, after printing the trace
results. A word whose holder the file has no wildcard row for (see below), or a scenario with no `blockptrs.txt`,
is printed (scenario, holder, offset, target) and `--check` exits 1. `--quick` does not check the pointers.
`--save-blockptrs <dir>` writes each scenario's file to `<dir>/<scenario>.txt`, and
`uv run tools/blockpointers.py --from <dir>` regenerates the data file from them, keeping the hand-set `class` and
`note` columns; without `--from` it plays the suite itself with the scan on every tick, which is how the list
should be made (a `--from` of sampled files warns). The every-tick run is slow (its timeout allows 6 ticks a
second), retries a scenario once on any failure, and with `--save-scans DIR` saves the scans that finished even when
another scenario failed; it then lists the failures, exits 1 and leaves the data file alone. `--scenario NAME...`
(with `--save-scans`) plays only those, so a failed one can be re-run and the saved scans merged with `--from DIR`.
`tools/blockpointers.py --check` (CI) checks the file.

Addresses outside the block differ from run to run, so the (holder, offsets, target) rows can never be complete: they
are the record that later issues use, and an every-tick regeneration produces them. The gate matches by holder. Each
holder has a wildcard row with `*` as its `offsets` and as its `target` for every class among its pair rows, and one
for `not-a-pointer` (its words that only look like addresses). A word in a holder with a wildcard row is covered,
whatever its offset and target; a heap address in any other holder fails the gate. Wildcard rows have `0` scenarios,
`0` ticks and `-` target_offsets, a class and a note; regeneration keeps them as written. They are keyed by
(holder, class), the pair rows by (holder, target), and they sort by holder, then wildcard rows first, by class.
`blockpointers.py --check` fails on a holder with pair rows that lacks one of them. A holder the scans have not met
yet is added by hand as a `*`/`*` row, with its class and a note.

## Zero fill and poison mode

Game memory comes from `GameAlloc` (and `operator new`, which calls it), which is plain
`malloc`: a new block holds whatever the heap had, and the game reads some of those bytes
(the spatial grid's `a` and `b` come from the edge cells' `high`, which nothing writes). The
reference defines them: every test-mode run passes the game's own `-memset=0` switch (a debug
option of the game's library: `IsMemSet`, `GetMemSetValue`, `FillPattern` in `GameAlloc`), so
every `GameAlloc` and `operator new` block is zero before the game uses it. `tools/playtest.py
--testmode` adds it; run the exe by hand with it, or the traces will not match the stored ones.
`IsNearFillPattern` returns false for a zero fill, as with the switch off. The exe says so on
stderr at the start (`testmode: allocations are filled with 0x00000000`).

`--poison` finds the bytes whose starting value reaches the hashed state:

```sh
uv run tools/reference.py --check --exe build/modern/TotalA.exe --poison      # lists the bytes whose starting value reaches hashed state
uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/scenarios/skirmish2.txt --poison
```

It passes `-memset=0xcdcdcdcd` instead, so each block is filled with 0xCD. `GameCalloc` still
zeroes. What neither fill reaches: `realloc` growth, memory from `malloc`, `HeapAlloc` or
`VirtualAlloc` called directly, and stack and static data. The run lists the bytes whose
initial contents reach hashed state. They are either harness skips (never read by the game,
listed in `data/trace_coverage.csv`) or real uninitialised reads, which stay traced and go on the
list of #7 tasks. A difference from the stored digests is expected for the latter, so it is not a
failure of the run. On a difference the report reruns tracediff with B poisoned (`--poison-b`), so
it compares the runs that differed.

## Quick check

```sh
uv run tools/reference.py --check --exe build/modern/TotalA.exe --quick     # a fixed subset, for a fast look
```

`--quick` is the same `--check` restricted to the five scenarios in the
`QUICK` constant of `tools/reference.py`. Its scenarios take about 1 to 2
minutes at the default `--parallel`. A pass says so and reminds you that it is not the gate; a failure
prints the same `tools/tracediff.py` report as the full run. `--quick` goes with
`--check` and `--exe`, and `--traces` rejects it. It is no substitute for the
full `--check`, which is what a pull request's gate needs.

| Name | Players | Sides | Start positions |
| --- | ---: | --- | --- |
| duel-coast-to-coast | 2 | Core | fixed |
| duel-great-divide | 2 | Arm | random |
| eight-full-moon | 8 | Arm and Core | fixed |
| eight-greenhaven | 8 | Arm | random |
| orders-move-attack | 2 | Arm and Core, scripted orders | fixed |

`tools/tracediff.py --ignore-view-seed` accepts traces whose headers differ only
in `view_seed`; with `--scenario` given twice, run mode runs A on the first file
and B on the second.

Run time of the suite (27 scenarios), measured on a 20-core machine with Wine and
other jobs running, not counting the build. Each run hardlinks its Wine prefix from
the template and copies only the registry files, so runs share nothing else, and the
longest scenarios start first. `--check` with the default 6 scenarios at a time took
5:39 and 6:16 (load average 30 and 50), and 7:57 with `--parallel 2`. The code before
this change took 9:59 and 9:19 on the same machine. `long-greenhaven-8p` alone takes
about 4:30 and sets the floor. Digests are identical between `--parallel 1` and the
default.
