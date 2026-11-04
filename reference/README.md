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

`--traces` also copies the frames the branch wrote (`reference/frames/<scenario>/<tick>.bin`, for the scenarios with a `frames` line) into this tree's `reference/frames/`, replacing the old set.
`--check` compares each such scenario's run frames byte for byte with them (docs/frame-dump.md); a missing stored frame fails.

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
An exe built from the `reference` branch has no headless host and exits at tick 0 without a display, so
`tracediff.py --reference a|b` starts that side with `playtest.py --xserver`, an invisible Xvfb display.

The FPU check ignores the infinity-control bit, which no x87 since the 80387
uses, so both `0x9001F` and Wine's `0xD001F` pass.

## Scenario format

The format of a scenario file (every key, the scripted `order`, `spawn`, `console` and `input` lines, the game
types, loading and saving a game, the quirk flags, and the end of a run) is specified in
[docs/scenario-format.md](../docs/scenario-format.md), which the data kit ships. It is the one source: this page
keeps the commands of the tools that play scenarios.

## Saving and loading a game

The saving run's `harness.sav` is stored by `--traces` in `reference/saves/<scenario>.sav` (`harness.opt` is never
stored), and `--check` compares the modern build's with it by decoded contents (`tools/savefile.py`, which leaves
out the bytes the format takes from the clock or from uninitialised memory) and reports `saves: N of N equal`.
`tools/reference.py` (`--check`, `--traces`, and the tools that play the suite) plays every scenario with no `load`
line first, then the loading ones; it copies the saving run's `savegame/harness.sav` and `savegame/harness.opt` out
of its game copy and places them in the loading run's own game copy before that exe starts (both files are removed
from a game copy first, so a file of the install never stands in). The two runs are separate processes. A loading
scenario whose saving scenario is not played, fails, or writes no save or no options file fails, with a message that
names both. `tools/tracediff.py` does both steps for a scenario with a `load` line.

By hand, play the saving scenario with `--collect savegame/harness.sav --collect savegame/harness.opt` (the files land
in the run's directory), then the loading one with `--put savegame/harness.sav <that file> --put savegame/harness.opt <that file>`:

```sh
WINEARCH=win64 uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/scenarios/save-midgame.txt --collect savegame/harness.sav --collect savegame/harness.opt
WINEARCH=win64 uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/scenarios/load-midgame.txt --put savegame/harness.sav <run directory>/savegame/harness.sav --put savegame/harness.opt <run directory>/savegame/harness.opt
```

## Quirk flags

The flags are listed in `src/quirks/quirks.def`. A normal run takes a spec with the `-quirks=<spec>` switch, in the
same syntax as the `quirks` key (the spec's section 6); with no spec every flag is off, which is 3.1c. In test mode
the scenario's `quirks` key sets the flags and they are frozen.

`reference/tests/quirks-tamper.txt` (a `quirks_tamper` line) trips the freeze check on purpose, and is expected to
exit with `testmode: quirk flags changed at tick <n>`:

```sh
uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/tests/quirks-tamper.txt --game-dir ~/.ta
```

`reference/tests/quirks-<flag name>.txt` reaches the guard of each flag a `quirks_reach` line names, with
`quirks 3.1c +<flag name>`:

```sh
uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/tests/quirks-units.ignore-null-death-record.txt --game-dir ~/.ta
uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/tests/quirks-ai.patrol-null-enemy-skip.txt --game-dir ~/.ta
uv run tools/playtest.py --exe build/modern/TotalA.exe --testmode reference/tests/quirks-units.skip-empty-yardmap.txt --game-dir ~/.ta
```

Each runs to the last tick. With the `quirks` line removed, the exe faults at the guard's tick, as 3.1c does, and
the run stops before the last tick.

## UI logs

`--check` compares each run's `ui.txt` line by line with `reference/ui/<scenario>.txt` and reports the first line
that differs (format and meaning in [docs/ui-log.md](../docs/ui-log.md)). It fails when a stored log has no
scenario, when a `ui-*` scenario has no stored log, and when a `ui-*` scenario's log has no order, queued build,
cancel or group line and selects no unit (a click that gives nothing is a broken scenario). A scenario with no
stored log is only counted. `--traces` copies the logs the reference branch wrote into `reference/ui/`. A scenario
with `input` lines runs at one step per frame even under `--steps-per-frame`.

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
| console-economy | The Pass | 2 | 200 | Console: resources (`atm`, `nometal`, `noenergy`, `give`), the share commands, the AI toggle (`ai`) and the player switches (`control`, `view`) |
| console-features | The Pass | 2 | 300 | Console: `feature`, `burnone`, `burnall`, `treedeath`, `meteor` (a shower, meteors on and off) and `sealevel` |
| console-ilose | The Pass | 2 | 100 | Console: `ilose`, so the local side loses its units and the run ends at tick 30 |
| console-iwin | The Pass | 2 | 100 | Console: `iwin`, so the other side loses its units and the run ends at tick 30 |
| console-sight | The Pass | 2 | 200 | Console: `los`, `lostype`, `mapping`, `radar`, `nowisee`, then `ai` and `reloadaiprofiles` |
| console-units | The Pass | 2 | 300 | Console: unit names typed alone, `move`, `reload`, `kill`, the damage toggles, `search`, `edge`, `assign` and `selectable` |
| duel-coast-to-coast | Coast To Coast | 2 | 5400 | Two Core players on Coast To Coast, three minutes, difficulty 2 |
| duel-great-divide | Great Divide | 2 | 3600 | Two Arm players on Great Divide, random start positions, two minutes |
| duel-pass-dark | The Pass | 2 | 2700 | Two players on The Pass with the map unmapped and line of sight off, ninety seconds |
| duel-shore-to-shore | Shore to Shore | 2 | 2700 | Arm against Core on Shore to Shore with 5000 metal and energy, no commander death, difficulty 0 |
| eight-full-moon | Full Moon | 8 | 9000 | Eight players on Full Moon, alternating sides, five minutes |
| eight-greenhaven | Greenhaven | 8 | 5400 | Eight Arm players on Greenhaven, random start positions, three minutes |
| eight-pincushion-teams | Pincushion | 8 | 12600 | Eight players on Pincushion in four allied pairs, seven minutes |
| long-great-divide-2p | Great Divide | 2 | 54000 | Two players on Great Divide for 54000 ticks (thirty minutes), a long run that no commander death ends |
| long-greenhaven-8p | Greenhaven | 8 | 54000 | Eight players on Greenhaven in two allied teams of four for 54000 ticks (thirty minutes), a long run |
| mission-defeat | Sub Hunting | 2 | 2400 | Game type 1, the Arm Very Short campaign on Easy: the human sonar buoy self-destructs, so a defeat condition ends the run early |
| mission-radius | Battle of Ansapp Sea | 2 | 12000 | Game type 1 on Easy: the human carrier is ordered into the target radius, so the victory condition `MoveUnitToRadius` ends the run early |
| mission-victory | Sub Hunting | 2 | 2400 | Game type 1 on Hard: the second player's subs self-destruct, so a victory condition ends the run early |
| orders-build-patrol | The Pass | 2 | 600 | Human Arm commander builds a solar collector, then patrols (`build`, `patrol`) |
| orders-guard-reclaim | The Pass | 2 | 600 | Human Arm commander builds a solar collector, guards it, then reclaims it (`build`, `guard`, `reclaim`) |
| orders-move-attack | The Pass | 2 | 600 | Human Arm commander moves, attacks the Core commander, then the whole slot moves (`move`, `attack`, `<slot>:all`) |
| quad-fox-holes | Fox Holes | 4 | 7200 | Four players on Fox Holes, alternating sides, four minutes |
| quad-king-of-the-hill | King of the Hill | 4 | 9000 | Four players on King of the Hill in two allied teams of two, five minutes |
| quad-lava-highground | Lava Highground | 4 | 5400 | Four Arm players on Lava Highground, random start positions, three minutes |
| quad-two-continents | Two Continents | 4 | 6000 | Four players on Two Continents in teams of players 0 and 3 against 1 and 2, random start positions, 100 metal and energy |
| sensors-floating-teeth | Hundred Isles | 2 | 400 | Human Arm commander with dragon's teeth owned by the AI player in its camera view (`spawn` for two slots, the feature draw through `IsFootprintVisible` and `IsExplored`) |
| sensors-floating-teeth-los-off | Hundred Isles | 2 | 400 | The same with `line_of_sight 0`, so the feature draw goes through `IsSeen` |
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
| ui-build-menu | The Pass | 2 | 200 | Scripted build menu clicks, the build menu and its queue counts in frames |
| ui-build-place | The Pass | 2 | 200 | Scripted build placement (`IssueMobileBuildOrders`) |
| ui-click-select | The Pass | 2 | 120 | Scripted click selection (`input`, interface type 1), the commander and unit panels in frames |
| ui-drag-select | The Pass | 2 | 200 | Scripted box selection with and without Shift |
| ui-groups | The Pass | 2 | 300 | Scripted control groups |
| ui-keys | The Pass | 2 | 300 | Scripted keyboard shortcuts: attack, patrol, special attack, stop, queueing, self-destruct |
| ui-left-click-orders | The Pass | 2 | 260 | Scripted left-click orders and command characters (interface type 0) |
| ui-right-click | The Pass | 2 | 260 | Scripted right-click move and attack, the minimap scroll |

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
cmake --build build/modern --target wasm_runtime   # once, and again after a change to wasm/ or a module's sources
rm -r build/playtest/prefix     # a fresh prefix; tools/playtest.py makes it again
uv run tools/reference.py --check --tier --exe build/modern/TotalA.exe --game-dir ~/.ta   # the pull request gate
uv run tools/reference.py --check --both-modes --exe build/modern/TotalA.exe --game-dir ~/.ta   # every scenario, in both module modes: the full gate
```

The gate is the modern build's `--check`: every scenario it plays must report equal. There are two tiers:
`--tier` plays the scenarios of `GATE_TIER` (see "Gate tier" below) and is the gate of a pull request; the
full `--check` plays all of them and is the gate of the changes in "When the full suite is the gate".
The build runs its modules in WASM (`wind_meteors` so far): the `wasm_runtime` target builds
`wasmtime_host.dll` and `wind_meteors.wasm` (it needs cargo, the game build does not) and copies them and
`wasm/modules.json` next to `TotalA.exe`, and `tools/playtest.py` copies the three into each game
directory, failing with a message naming the target when they are missing. `--check --native-modules`
plays every scenario with `-modules=native` instead, the modules as native code, which needs no runtime
files. Both modes must report every scenario equal, and `--check --both-modes` is that in one command: one
pool plays every scenario once in WASM and once native (each line ends its name with `[wasm]` or `[native]`,
the sound logs, frames, saves, block pointer scans and fuel are judged per mode and their lines start with the mode),
and it exits 0 only when both modes do. The first difference in either mode stops every run of the pool, and its `DIFF`
line names the mode. `--tier` is still one mode, unless it is given `--both-modes` too.
Every game run of a `--check` holds one of the machine's run slots for its whole run: `bt-gate-slot-<i>` files in `$XDG_RUNTIME_DIR`
(`/tmp` without it), each taken with `flock`, so that a slot is free again when its holder dies. There are half the CPUs of them, at most
`GATE_SLOTS` (10, beside `MAX_PARALLEL`; `BT_GATE_SLOTS` overrides), and all the gates on the machine together play at most that
many games. `--parallel` still caps one gate's runs. A tier gate keeps its pool busy only at the start (`eight-pincushion-teams` runs 37 to 119
seconds, the others 15 or less), and one exclusive lock left the other slots idle while the next gate waited: with slots, three tier gates
started together finish in about 100 seconds against about 160 one after the other (measured on #760 with 20 CPUs: 6 slots 170 seconds, 10 slots 100, 14 slots 88 with each run slower under the load, so
the default stays at half the CPUs). A run that has to wait prints once, `waiting for a run slot, held by the gates of <pid> (<worktree>), ...`.
A scenario with a `load` line starts as soon as the scenario it loads has finished (ahead of the scenarios not yet started), so
`load-midgame` no longer waits for `eight-pincushion-teams`.
A second `--check` with the same cache key waits for the first on a key lock (`bt-gate-key-<key>.lock`) and then reports its cached pass (below),
rather than playing the same exe twice. `--no-lock` takes no slot and no key lock, for a run that is not a gate.
A passing `--check` is cached: its output, date and commit go to `~/.cache/bt-gate/<key>.txt` (shared by every worktree on the machine),
where the key is a SHA-256 of the exe, `wasmtime_host.dll`, the `.wasm` modules and `modules.json` beside it, `reference/scenarios`, `traces`,
`sounds`, `ui`, `frames` and `saves`, `tools/playtest/`, `tools/reference.py`, `playtest.py`, `tracediff.py`, `tracedigest.py`, `savefile.py` and `blockpointers.py` with every `tools/` module they import (found from the source), `data/block_pointers.csv`, the `TotalA.map` beside the exe (or its absence), and the options that change what is played
(tier, mode, `--steps-per-frame`, `--quirks`, `--roundtrip`, `--poison`); the `commit.txt` beside the exe is not part of it. A failing run writes nothing.
The key is looked up before the key lock and again after it, so a run that waited behind a gate of the same inputs reports that gate's pass. On a hit
`--check` prints `cached pass from <date> at <commit>` and the stored lines and exits 0; this counts as a gate run, and the pull request body says "cached".
`--no-cache` plays anyway. (A `--both-modes` run exits 1 until #654, so it caches nothing yet.)
`uv run tools/wasm_modules.py glue-fatal --game-dir ~/.ta` (Wine only, not in CI) runs the exe on a bogus `meteor.tdf` and checks that the module's `FatalError` ends the game with exit code 2 after `wl_call` returned.
`--check` without `--exe` is an error, since `moddable` does not build the
reference. Add `--game-dir <dir>` when the game is not in a Steam directory.

The gate applies when a pull request changes `src/`, `include/`,
`interfaces/`, `lib/` (the bit-exact maths and the WASM loader), `wasm/`,
`reference/`, the build (`CMakeLists.txt`, `cmake/`), the layout and
trace format data (`data/layout_refs.csv`, `data/trace_schema.json`,
`tools/traceschema.py`), or the tools that run the exe
(`tools/reference.py`, `tools/playtest.py`). Other changes do not need
it. The merges that did are the ones counted for the full-suite cadence
below.

`--traces` writes every trace with the reference branch's sha as `commit`. `--check` plays
every scenario (`--parallel N` at a time, default half the CPUs and at most 6, or 10 with `--both-modes`), prints one line per scenario with its run time
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

A run that dies leaves three records:

- `crash.txt`, beside `trace.txt` in the game copy (test mode only): a pure virtual call writes `purecall from 0x<address>`,
  the address of the instruction after the call, before the runtime error R6025 and exit code 255. `playtest.py` copies it into
  the run's directory and quotes it in the `summary.md` notes. Look the address up in the exe's link map
  (`build/modern/TotalA.map`).
- The dialog text: the failure message, and with it the `summary.md` row and a line in `steps.log`, ends with `text "..."`, the
  text of the message box (Wine's `msgbox` channel, which `playtest.py` always turns on, in `wine.log`). A test-mode run is headless and
  shows no box, so only a play-test with a display has this record; a test-mode run has `crash.txt` and `wine.log`.
- The failed attempt: when `reference.py` tries a run again, it copies the failed attempt's `playtest.py` session
  (`wine.log`, `steps.log`, `summary.md`, `trace.txt`, `crash.txt`) to `build/scratch/check-<scenario>-failed-<time>/`, whether the
  retry passes or not, and the console line `run failed, trying again` ends with `[failed attempt kept: <path>]`. Its `wine.log`
  is taken with `WINEDEBUG=-all,+seh,+msgbox` (a play-test with a display uses `-all,err+seh,+msgbox`; `--debug` sets it), so
  it names the exception chain of the faulting thread, and a startup page fault such as the one in `FlipScreen` has a backtrace.

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
works with `--tier`, `--steps-per-frame` and `--parallel`, and not with `--poison`.

### When the full suite is the gate

The gate of a pull request is the tier (`--tier`), for every change the gate applies to, the state block
layout, the host interface and platform code included. The full `--check` stays the gate only of a pull
request that regenerates the stored reference data: `reference/` traces, sounds, UI logs, frames or
saves, or the trace format (`data/trace_schema.json`, `tools/traceschema.py`), since that data is
produced for every scenario anyway. The body says which tier it ran.

The play-tests (`tools/playtest.py` smoke, full, mp and music) are not part of a pull request's gate.
They run with the full `--check` on `moddable`, below.

#286 passed the gate and broke `mp`: it is the example of what the periodic play-tests are for. The
trade is speed for a later catch: a change of that kind can merge first and be caught at the next
periodic run.

### The full suite on moddable

The full `--check` runs on the tip of `moddable` once every 5 merged pull requests that needed the
gate. `uv run tools/fullgate.py` prints how many have merged since the newest `Full gate on <sha>`
comment on the tracking issue (#632), and `uv run tools/fullgate.py due` exits 1 at 5 or more. When
it is due, the lead runs `uv run tools/fullgate.py run` on the tip. It plays the full `--check
--both-modes --no-cache` (no `--tier`) and then, straight after it on a fresh prefix, `--check --no-cache
--roundtrip auto`, which saves the block at an early, a middle and the second to last tick of every scenario,
restores it and carries on, so that state outside the block shows as a divergence from the stored traces. The
round trip run scans and checks block pointers as the full run does. `run` prints the comment text and posts
nothing:

```
Full gate on <sha>: <n> of <n> equal (both modes), block pointers: <result>, round trip: <n> of <n> equal
```

The lead posts it on #632 and adds the play-tests (`tools/playtest.py` smoke, full, mp and music) and their results.
A run that fails appends `, both modes exit <n>` or `, round trip exit <n>`, and names round trip block pointer
problems in parentheses after the round trip count.

**A periodic failure is fixed afterwards, not by freezing merges.** The lead files it at once as an
issue, first in #2's order, with the failing scenario or play-test and the merges since the last pass
(`tools/tracediff.py` gives the first tick of a divergence). Merges continue; the next periodic run
must pass.

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
`-blockscanverify` (playtest.py `--blockscan-verify`) also scans every allocation byte by byte against spans rebuilt from the
allocation registry, and ends the run with exit 2 unless both scans find the same rows; `tools/test_blockscan.py` plays
scenarios with it.

`reference.py --check` always runs with the scan on, every 8th tick and every tick of the last 64 (`-blockscan=8`),
resolves the sites with the linker map beside the exe
(`build/modern/TotalA.map`) and compares every row with `data/block_pointers.csv`, after printing the trace
results. A word whose holder the file has no wildcard row for (see below), or a scenario with no `blockptrs.txt`,
is printed (scenario, holder, offset, target) and `--check` exits 1. `--tier` checks the pointers of its scenarios the same way: a site is named by its rank among its function's
allocator calls in the exe, not by the scans, so a tier run names its rows as the full suite does.
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

## Gate tier

```sh
uv run tools/reference.py --check --tier --exe build/modern/TotalA.exe --game-dir ~/.ta
```

`--tier` is the same `--check` restricted to the 30 scenarios of the `GATE_TIER` constant of
`tools/reference.py`, 33,910 ticks of the suite's 324,610 (10.4%), none longer than 9000. It is the
gate of a pull request, except for the changes in "When the full suite is the gate". A pass says how
many scenarios were equal and at which commit; a failure prints the same `tools/tracediff.py` report
as the full run. `--tier` goes with `--check` and `--exe`, and `--traces` rejects it. It checks the
block pointers of its scenarios against data/block_pointers.csv, since the names do not depend on the scenarios played.

The tier was chosen greedily from a coverage run of the suite (tools/coverage.py): the scenario
adding the most lines per tick (a floor of 600 ticks each), until one adds fewer than 20, starting
from the forced 8-player game `eight-full-moon` and leaving out every scenario longer than 9000 ticks
(`--choose-tier --eight eight-full-moon --max-ticks 9000`). The rule's pick differs from the earlier 26:
the `ui-*` scenarios (`ui-keys`, `ui-groups`, `ui-build-menu`, `ui-build-place`,
`ui-left-click-orders`) come in, and `eight-pincushion-teams` (12,600 ticks) and `orders-factory` go out. `uv run tools/coverage.py --exe build/coverage/TotalA.exe --game-dir
~/.ta --tier` plays the tier on the coverage build, prints its lines and functions as a share of the
full suite's in `data/coverage.csv`, and exits 1 below 96% of the lines. Run it when a scenario is
added or removed and when the tier changes.

`uv run tools/coverage.py --exe build/coverage/TotalA.exe --choose-tier` (`--runs DIR`, `--max-ticks N`, `--eight NAME`) plays
nothing: it reproduces this rule from the per-scenario profiles a coverage run left in `build/coverage-run/` and prints the
scenarios it takes, with their players, ticks and new lines, and their lines and functions as a share of the union of every
scenario's. `--max-ticks` leaves out the scenarios longer than N ticks; `--eight` takes the named 8-player game first, as the only one.

`uv run tools/coverage.py --slots ai` (repeatable, or `all`; `--runs DIR`, `--json FILE`) plays nothing: it reads the per-scenario
profiles a coverage run left in `build/coverage-run/` and prints, for each slot of `interfaces/ai.h`, the scenarios that enter
it with their entry counts and ticks, or `never`, and a smallest set of scenarios that enters every slot some scenario enters.
A `<Slot>Native` binding is a wrapper (its count is the number of seam calls); any other binding is direct, and its count is an
upper bound. Replay parts pick their recordings from it.

| Name | Players | Ticks |
| --- | ---: | ---: |
| allied-victory | 4 | 360 |
| commander-death | 2 | 360 |
| conditions-dark-side | 2 | 3200 |
| console-economy | 2 | 200 |
| console-features | 2 | 300 |
| console-sight | 2 | 200 |
| console-units | 2 | 300 |
| eight-full-moon | 8 | 9000 |
| hailstorm-los-off | 2 | 150 |
| load-midgame | 0 | 900 |
| mission-radius | 0 | 2700 |
| mission-victory | 0 | 360 |
| orders-air | 2 | 4600 |
| orders-air-fights | 2 | 1500 |
| orders-air-repair | 2 | 900 |
| orders-build-patrol | 2 | 600 |
| orders-guard-attacked | 2 | 600 |
| orders-guard-reclaim | 2 | 600 |
| orders-immediate | 2 | 1000 |
| orders-tier2-core | 2 | 1300 |
| orders-tier2-spider | 2 | 700 |
| orders-tier2-vader | 2 | 420 |
| save-midgame | 2 | 1800 |
| sensors-floating-teeth-los-off | 2 | 400 |
| ui-build-menu | 2 | 200 |
| ui-build-place | 2 | 200 |
| ui-groups | 2 | 300 |
| ui-keys | 2 | 300 |
| ui-left-click-orders | 2 | 260 |
| units-transport-ship | 2 | 200 |

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
