# The scenario format

A scenario is a text file that tells a test-mode run of the game what match to play, what to feed it and
when to stop. The traces, sound logs, UI logs, frames and saves of the kit are the outputs of such runs, one
scenario each, so an engine that reads the same scenarios and plays the same match writes the same outputs.
The scenarios of the kit are in `reference/scenarios/`. This page says what each line of a scenario does and
what a test-mode run does with it. It describes behaviour only.

A run needs the game's data files (the archives of the install the stored outputs were made with, listed with
their hashes in `docs/reference-traces.md`). Map names, unit type names, side names, campaign names and
mission names in a scenario are the names the game data gives.

## 1. The file

One `key value` per line. A key is the first word of a line, the value is the rest of it with its leading and
trailing blanks removed, and it may contain spaces (a map name does). Blank lines and lines whose first
non-blank character is `#` are ignored. A line with a key and no value is an error. A key that is not in this
page is an error. A later line for the same key replaces an earlier one, except for the keys that this page
says take one line only (a second line is an error) and the keys that accumulate (`order`, `spawn`,
`console`, `input`, `record`, `roundtrip`).

A number is decimal, `0x` hexadecimal or `0` octal. A value that does not parse, or lies outside the range
given here, is an error. An error ends the run with status 2 and a message on the error stream that names the
line and the key. The file is read and checked before the first tick, apart from the errors that only the
match shows (an unknown map, a selector that names no live unit at its tick, a build site the game rejects).
Errors that the whole file decides (a missing `ticks` line, fewer
than two players) name no line.

A tick in any key is a game tick counted from 1, the number the trace calls `tick` (`docs/trace-format.md`,
section 2). The game runs 30 ticks per second of game time.

## 2. What a test-mode run does

A test-mode run starts from the scenario alone: it opens no menu, reads no keyboard or mouse (the only input is
what the scenario scripts), and uses no window, display or sound device. It starts the match the scenario
describes, plays it tick by tick, writes the outputs of section 7 and exits.

- **Seeds.** The game-logic random generator, the C runtime's `rand` and the presentation random generator
  (screen shake, particles and menu effects, which game logic never reads) start from `game_seed`,
  `crt_seed` and `view_seed`. The game-logic generator and `rand` are seeded again at the start of the match
  with the same values. Game logic reads no clock: its time is the tick.
- **Memory.** Every new allocation is zero before the game uses it, and memory grown by a reallocation is zero
  from the old size up to the new size (`docs/trace-format.md`, section 11, "Zero fill"). The reference run
  passes the game's `-memset=0` switch for the first rule. A run with `-memset=0xcdcdcdcd` fills each new
  allocation with 0xCD instead (the poison run of the same section); it is not a conformity run.
- **Steps per frame.** The game runs a number of ticks between two drawn frames. The key `steps_per_frame`
  sets it, 1 by default. The outputs other than the frames must not depend on it: a run with another value
  writes the same traces. A scenario with `input` lines needs it to be 1.
- **Arithmetic mode.** The floating-point control word the run starts with must be the one `fpu_control_word`
  names, ignoring the infinity-control bit (no processor since the 80387 uses it): both `0x9001F` and `0xD001F`
  pass. Otherwise the run exits with status 2.
- **Frozen quirk flags.** The quirk flags (section 6) are set when the run starts and checked before every tick.
  If they differ, the run exits with status 2 and the message `testmode: quirk flags changed at tick <n>`.
- **The end.** A run ends after the tick `ticks`, or at the end of the tick in which the game declares the match
  over (a victory or defeat, after the game's own countdown, a commander's death or the last enemy gone),
  whichever comes first. The tick that ends the match is played and traced in full. The run closes its outputs
  and exits with status 0. The game's end-of-game screen is never reached. The trace has no marker for the
  early end: it has fewer rows than `ticks` implies, and a build that ends the match on another tick differs
  from it by the length of its trace. A run that ends early because of the end-of-game state does not fail
  for a `frames` tick it did not reach.
- **Exit status.** 0 for a run that reached its end, 2 for a bad scenario or a failed check. The last message
  of a run is `testmode: reached tick <ticks>` or `testmode: game over at tick <n>` on the error stream.
- **Dump.** `-testdump <tick>`, a switch of the run and not a scenario key, also writes `dump.txt`
  (`docs/trace-format.md`, section 10). A tick that does not parse exits with status 2.

## 3. The match

### 3.1 Keys that set the match

| Key | Value | Default | What the run does with it |
| --- | --- | --- | --- |
| `game_seed` | number | 1 | Seed of the game-logic random generator. |
| `crt_seed` | number | 1 | Seed of the C runtime `rand`. |
| `view_seed` | number | 1 | Seed of the presentation random generator. |
| `fpu_control_word` | number | `0x9001F` | The control word the run must start with (section 2). |
| `ticks` | number from 1 | required | The last tick to play. |
| `steps_per_frame` | 1 to 8 | 1 | Ticks run between two drawn frames. |
| `game_type` | 1, 2 or 3 | 2 | 1 plays a mission of a campaign, 2 a skirmish, 3 the multiplayer rules without a network. One line only. See section 3.2. |
| `campaign` | the rest of the line | none | Game type 1 only, and required there: the campaign file (`camps/<name>.tdf`, without the extension; names have spaces). |
| `map` | the rest of the line | required | The map's name. Under game type 1, the mission's name in the campaign. A name the game data does not hold exits with status 2. |
| `difficulty` | 0 to 2 | 1 | The computer players' difficulty, one value for all of them. |
| `commander_death` | 0 or 1 | 1 | 1: the match ends when a commander dies. |
| `mapping` | 0 or 1 | 1 | 1: the terrain is blacked out until explored (the skirmish menu's "Terrain is blacked out until explored"). 0: the whole map is shown. |
| `line_of_sight` | 0 or 1 | 1 | 1: line of sight is on. |
| `los_type` | 0 or 1 | 1 | The line of sight type. |
| `start_positions` | `fixed` or `random` | `fixed` | `fixed`: player N starts at start position N. `random`: the start positions are shuffled. |
| `local_player` | slot 0 to 9 | the first human slot, else 0 | The slot whose view the game shows and whose state the victory and defeat conditions of a mission are checked for. It must be a slot some `player<N>.*` key names. |
| `save` | tick | none | Saves the game after the step of this tick (section 5). One line only. |
| `load` | a scenario's file name | none | Starts from a saved game instead of from a map and players (section 5). One line only. |
| `frames` | ticks | none | Ticks at which the run dumps the frame drawn at the end of that tick's loop pass (`docs/frame-dump.md`). One line only. |
| `quirks` | a quirk spec | `3.1c` | The quirk flags of the run (section 6). |
| `quirks_tamper` | tick | none | Test only: flips a quirk flag at this tick, so the frozen-flags check of section 2 exits with status 2. |
| `quirks_reach` | `<tick> <flag name>` | none | Test only: at that tick makes the game reach the guard of the named quirk flag (section 6). |
| `roundtrip` | tick | none | Test only, up to 16 lines: after the step of this tick, the run copies the shared state block, overwrites the block with a fill pattern and restores the copy, then plays on. A build that holds game state outside the block diverges from the stored traces. A tick of 0 or after `ticks` is an error. |
| `record` | `<subsystem>` or `<subsystem> <first tick> <last tick>` | none | Test only: makes the run write the replay recording of a subsystem when it ends (`docs/replay-format.md`). A subsystem is named once, a window is from tick 1 and its last tick is not before its first. The scenarios of the kit have none. |

`frames` takes ticks in ascending order, each from 1 to before `ticks`, at most 64 of them. A tick that the run
never draws as the last tick of a loop pass (with `steps_per_frame` above 1, a tick inside a pass) exits with
status 2.

`save` takes a tick from 1 to `ticks`.

### 3.2 Players

The keys `player<N>.*` describe the players of a skirmish or a multiplayer game. N is a slot from 0 to 9. A
slot exists when any key names it; the slots no key names are open. At least two slots must exist. The game's
player count is the highest named slot plus one. A map with fewer start positions than there are named slots
exits with status 2.

| Key | Value | Default | What the run does with it |
| --- | --- | --- | --- |
| `player<N>.controller` | `ai` or `human` | `ai` | `ai`: a computer player. `human`: the slot is played by the scenario, so it does nothing unless an `order`, `input` or `console` line acts for it. |
| `player<N>.side` | a side name | N mod 2 (0 is Arm, 1 is Core) | The side, by its name in the game's side data, any case. A name the game does not have exits with status 2. |
| `player<N>.team` | 0 to 5 | 5 | The ally group. 0 to 4 are teams, whose members are allied; 5 is no team. |
| `player<N>.color` | 0 to 9 | N | The colour index. |
| `player<N>.metal` | number | 1000 | The starting metal. |
| `player<N>.energy` | number | 1000 | The starting energy. |

### 3.3 Game types

**Game type 2** is a skirmish. Every key above applies. The run sets up the match as the skirmish menu does
after its last click, and the match starts from the loading screen.

**Game type 1** loads one mission of a campaign, as the campaign menu does. `campaign` names the campaign file
(`Battle Tactics - Arm Very Short`, `Arm Campaign`) and `map` the mission's name in that file (`Sub Hunting`,
`1: A Hero Returns`), as the mission list shows it. The mission's own file gives the units, their owners
(`Player=1` is slot 0) and the victory and defeat conditions, so a scenario names a slot for every owner the
mission uses, or the run stops with `Player number N invalid`. `difficulty` picks the mission's schema as the
campaign difficulty does: 0 prefers `Easy`, 1 `Medium`, 2 `Hard`, and each falls back to the others. The
mission decides `commander_death`, `mapping`, `line_of_sight`, `los_type`, `start_positions`,
`player<N>.metal` and `player<N>.energy`, so a scenario that sets one is an error. `game_type 1` without
`campaign`, and `campaign` without `game_type 1`, are errors. The mission's conditions are checked for the
human slot `local_player` names. Any mission map of the install can be used.

**Game type 3** runs the multiplayer rules without a network. The run fills in the lobby state the game
expects: every named slot is a connected player, the local slot is the host, `player<N>.team` makes the
alliances and both sides of each alliance agree to them. It then skips the lobby and starts the loading screen
itself, and before every step it sets every player's load progress to 100. `commander_death`, `mapping`,
`line_of_sight`, `los_type`, `start_positions` and the `metal` and `energy` of the local slot (which sets
every player's start) come from the scenario as in a skirmish. The packet phase of a tick never runs, so the
trace has no `net` rows.

## 4. Scripted lines

Four keys script what the game would otherwise get from a person. Each takes at most 256 lines (`input`:
1024). A line's tick is from 1 to `ticks`; a tick after the last tick is an error. Within one tick the run
does these in this order: the `spawn` lines, the `order` lines, the `console` lines and the `input` lines,
each in file order.

### 4.1 `order`

    order <tick> <unit selector> <kind> <target>
    order 5 0:ARMCOM:1 move 700 300
    order 5 0:ARMCOM:1 build ARMSOLAR 640 260
    order 150 0:ARMCOM:1 reclaim 0:ARMSOLAR:1

An `order` line orders the units of a `controller human` slot. A slot that is not human, a slot no key names
and a selector that names no live unit at its tick are errors.

- The order is issued at the start of `<tick>`, before the first phase, so the trace sees its effect in the
  same tick.
- A unit selector is `<slot>:<unit type>:<n>`: the n-th live unit of that type owned by that slot, in the order
  of the game's unit pool (the unit type is the unit's name in the game data, any case; n is from 1 to 4000).
  `<slot>:all` is all of that slot's live units.
- A target is `x z`, two world coordinates from 0 to 65535, or a unit selector that names one unit
  (`<slot>:all` is not allowed there). With a unit selector the order is issued as if the cursor were over that
  unit.
- `move`, `attack`, `patrol`, `guard` and `reclaim` take a target. `build <unit type> x z` takes the unit type
  before the position. `dgun` (special attack) and `teleport` take a position, `unload` a position, and `load`,
  `repair` and `capture` a unit.
- `stop`, `activate`, `deactivate` and `selfdestruct` take no target. `selfdestruct` is a toggle: it cancels
  the order if a selected unit already has one. `cloak on` and `cloak off` set the cloak. `movestate <n>` and
  `firestate <n>` set the standing order, n being 0, 1 or 2 (the value the order panel's button gives).
- `qbuild <unit type> [n]` adds n (default 1, at most 99) of a unit type to the build queue of each unit the
  selector names, as a click in a factory's build menu does. `qbuild MAKENUKE` and `qbuild MAKEANTI` queue a
  missile, as the weapon-build button does; the name goes to the game as written.
- A `+` before the kind (`+move 700 300`) is the shift key: the order joins the queue instead of replacing the
  orders, and repeating an identical queued order cancels it.

Each order is given as a mouse click gives it: the selection is set to the named units, the order mode and
cursor position are set, the order is issued, and the previous selection is put back. A `build` order is
issued as a click on a build site is, so a site the game rejects exits with status 2.

### 4.2 `spawn`

    spawn <tick> <slot> <unit type> <x> <z>
    spawn 10 1 ARMLAB 640 260

Creates one unit at the start of `<tick>`, before that tick's `order` lines. The slot must be a player the
scenario names (it need not be `human`), the unit type one exact unit type name with no wildcard, and `x z` a
world position. The unit stands at that position at the height of the ground there, snapped to its footprint as
the console cheat does, and it is created finished. An unknown unit type exits with status 2.

### 4.3 `console`

    console <tick> +<command> [arguments]
    console 1 +now Film Chris Include Reload Assert
    console 10 +nometal 0 50
    console 12 +armpw 0

Types a console command as the chat line does, at the start of `<tick>`. The text after the tick starts with
the `+` and is at most 79 characters. The run drops the `+` and gives the rest to the game's command handler.

- A command that the game allows only after the cheat code does nothing until the line
  `+now Film Chris Include Reload Assert` has run, so a scenario types the cheat code first. A skirmish or
  multiplayer game also enables the commands that are only for those.
- The chat line takes the cursor from the mouse. The run puts the cursor at the world position 640 260 before
  each console line, so `burnone` and `feature` act on the cell there and a unit name creates its unit there.
  `move <dx> <dz>` shifts the cursor, but the next console line sets it again, so it changes nothing a scenario
  can see. `give`, `nometal` and `noenergy` take the local player when no slot is given.
- A unit type name typed alone creates that unit for the slot in its first argument (`armpw 1`). The name must
  be a unit type of the game, any case, with no wildcard.

The commands a line may type, by their names in the game's command table (any case):

| Group | Commands |
| --- | --- |
| Cheat code | `now` |
| Resources and players | `atm`, `give`, `nometal`, `noenergy`, `ai`, `control`, `view` |
| Sharing (they do nothing without a network game) | `sharemetal`, `shareenergy`, `shareall`, `setsharemetal`, `setshareenergy` |
| Units | `reload`, `kill`, `assign`, `selectable`, `shootall`, `doubleshot`, `halfshot`, a unit type name |
| Features and the map | `feature`, `burnone`, `burnall`, `treedeath`, `meteor`, `sealevel`, `edge`, `search`, `move`, `drop` |
| Sight | `los`, `lostype`, `mapping`, `radar`, `nowisee` |
| Interface | `iface <0 or 1>`: the interface type, 0 for left-click orders and 1 for right-click orders |
| AI | `reloadaiprofiles` |
| Game end | `iwin`, `ilose` |
| No effect | `assert`, `dprint`, `mem`, `zbuffer` |

The sharing commands, `assert`, `dprint`, `mem`, `zbuffer`, `assign` (nothing is selected), `selectable`, `drop`
and `edge` change no field the trace hashes. A command that is not in the table, and a first word that is
neither a listed command nor a unit type, exits with status 2. The commands that are left out are the display,
sound, debugger and host commands, `compression`, `netstats`, `sharemapping`, `shareradar`, `include` and
`printweights`.

### 4.4 `input`

    input <tick> <event>
    input 20 mouse left down 384 240
    input 30 key down ctrl
    input 40 char a

Sends the game raw input, below the interface, so that the game's own code turns it into selections and
orders. The event is queued at the start of `<tick>`, and the interface handles it in the next frame, before the
phases of tick + 1. The events, the keys, the characters and the rules the lines must keep (one mouse event and
one key or `char` event per tick, a button released at most 1 tick after its press unless the mouse moved at
least 32 units in between, a position inside the 640 by 480 window, `steps_per_frame` 1) are in
`docs/ui-log.md`, "Scripted input". A breach is an error. A right-click gives an order only with interface type
1 and a left-click in an order mode only with type 0; a scenario that uses either types `console 1 +iface 1`
or `console 1 +iface 0` first (the default is type 0).

## 5. Saving and loading a game

`save <tick>` makes the run save the game after the step of that tick, to `savegame\harness.sav` in the run's
game directory, and write the options of the skirmish it started from beside it as `savegame\harness.opt`. The
save is the game's own saved-game file. The kit holds the stored save of its saving scenario in
`reference/saves/`; the format of the file is not specified by the kit.

`load <name>` names a scenario of the suite by its file name without the extension, one whose `save` line writes
the save (`load-midgame` loads `save-midgame`). The run starts the match as the game's load screen does,
from the files `savegame\harness.sav` and `savegame\harness.opt` of its game directory. The test harness that
plays the suite plays every saving scenario first and puts the two files it wrote in the loading run's game
directory before that run starts. A file that is missing or unreadable, a save that is not a skirmish, or a map
the game cannot load, exits with status 2 and a message that names the file.

- The options file is read as the skirmish menu's table is and the save then overwrites the fields it decides.
  A missing options file, or one of the wrong size, exits with status 2.
- **Keys it may still set:** `game_seed`, `crt_seed`, `view_seed`, `fpu_control_word`, `ticks`,
  `steps_per_frame`, `frames`, `quirks`, `quirks_tamper`, `quirks_reach`, `roundtrip` and `record`. The seeds
  take effect when the match loads.
- **Keys it may not set:** the save decides them, so a scenario with a `load` line that sets one exits with
  status 2 and names it: `game_type`, `campaign`, `map`, `difficulty`, `commander_death`, `mapping`,
  `line_of_sight`, `los_type`, `start_positions`, `local_player`, every `player<N>.*` key, `save`, `spawn`,
  `order` and `console`. `map` and the `player<N>.*` keys are not required.
- **The tick.** The save holds the game's tick counter, so the loaded game continues from the saved tick. The
  save is made after the step of its tick, so the first row of the loaded run's trace is tick saved tick + 1
  (901 for `save-midgame`), and the trace has no rows before it. `ticks` is the last tick traced, counted from
  the start of the game and not from the save (`load-midgame` has `ticks 1800`, like `save-midgame`). The run
  exits with status 2 if `ticks` is not greater than the saved tick, and the same holds for the ticks of
  `quirks_tamper` and `quirks_reach`.
- **What differs from an uninterrupted game.** The save does not hold the random streams, and the load seeds
  them again, so the loaded game does not match the uninterrupted one. Compare it with the stored traces of the
  loading scenario (`docs/reference-traces.md`, "A loaded game's trace").

## 6. Quirk flags

A quirk flag is a switch that changes one behaviour of the game that 3.1c has by accident or by a defect. With
every flag off the game is 3.1c, and the traces match the stored ones.

`quirks` takes a spec: a preset, then `+flag` or `-flag` words (`recommended -flag`). The preset `3.1c` is every
flag off and `recommended` is every flag on. A run with no `quirks` line is 3.1c. When any flag is on, the trace
header carries a `quirks` line (`docs/trace-format.md`, section 2).

`quirks_reach <tick> <flag name>` is for the flags whose guard no stock game data reaches. At that tick the run
makes the game reach the guard of one of three flags:

- `units.ignore-null-death-record` applies a unit-killed record whose unit id is 0;
- `units.skip-empty-yardmap` runs the commander's script step `set YARD_OPEN` on a unit with an empty yard map;
- `ai.patrol-null-enemy-skip` puts a unit in the first computer player's scout squad, sets its rally point to
  the origin and runs its scout timer with every player allied to it, so that no enemy is found.

Any other flag name is an error. With the flag off, the run faults at the guard's tick, as 3.1c does, and stops
before the last tick. The scenarios of the kit set no `quirks` line, so each plays as 3.1c.

## 7. The outputs

A test-mode run writes these files in its working directory. Two builds conform when their outputs are equal.

| File | Written | Specified by | Stored in the kit |
| --- | --- | --- | --- |
| `trace.txt` | every run: a hash per phase of each tick and a hash of the world tables | `docs/trace-format.md` | `reference/traces/<scenario>.txt`, as one digest per tick (`docs/trace-format.md`, section 9) |
| `dump.txt` | a run with `-testdump <tick>`: every hashed field after each phase of that tick | `docs/trace-format.md`, section 10 | not stored |
| `sound.txt` | every run: the sounds the game asks to start or stop | `docs/sound-log.md` | `reference/sounds/<scenario>.txt` |
| `ui.txt` | every run: the input delivered, the orders it turned into, the selection and the camera after each tick | `docs/ui-log.md` | `reference/ui/<scenario>.txt` |
| `frames/<tick>.bin` | a run whose scenario has a `frames` line: one file per listed tick | `docs/frame-dump.md` | `reference/frames/<scenario>/<tick>.bin` |
| `savegame\harness.sav`, `savegame\harness.opt` | a run whose scenario has a `save` line | section 5 | `reference/saves/<scenario>.sav` (the `.opt` file is not stored) |
| `replay-<subsystem>.txt` | a run with `record` lines | `docs/replay-format.md` | `tests/replay/<subsystem>/<scenario>.txt` |

The run's own `order` lines are not logged in `ui.txt`; only the orders the interface made from `input` lines
are. 