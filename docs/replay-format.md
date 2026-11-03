# Replay recordings

A recording holds the calls one subsystem received in a test-mode run: the state each call could read, the
imports it made with their arguments and results, and the state it changed. `bt-replay 2` is the format.
Recordings are in `tests/replay/<subsystem>/<scenario>.txt`; wind_meteors and economy have recordings so far. The tables behind the format are `data/replay_schema.json`
(`uv run tools/replayschema.py` writes it, and the generated sources, from the interface headers,
`data/subsystems.csv`, `data/rwsets.csv` and the annotations the file itself holds).

A recording does not depend on the module mode: the same file comes from `-modules=wasm` and `-modules=native`.

## Switching it on

A scenario line `record <subsystem>` makes the run write `replay-<subsystem>.txt` when it ends without
failure. `record <subsystem> <first tick> <last tick>` records only the calls whose tick is in the window.
Every simulation subsystem (the members of `Seams`) is accepted, and a scenario may carry several `record` lines,
one per subsystem, each with its own file and window. `tools/replay.py` adds the lines to copies of the
scenarios; `reference/scenarios/` never holds them. A stored recording that covers only a window is listed in
`tests/replay/windows.json` (`"<subsystem>/<scenario>": [first, last]`), and `--check` records it with that window.

```sh
uv run tools/replay.py --record --exe build/modern/TotalA.exe --out DIR [--subsystem NAME]... [--native-modules] [SCENARIO...]
uv run tools/replay.py --check --exe build/modern/TotalA.exe [--subsystem NAME]... [--native-modules] [SCENARIO...]
uv run tools/replay.py --compare DIR_A DIR_B
```

`--subsystem` is repeatable and defaults to every subsystem that has a `tests/replay/<subsystem>/` directory.
`--check` records again and fails when a recording of `tests/replay/` differs. `--compare` fails when two
directories differ and names the first differing line. Record on a fresh Wine prefix, as the gate runs.

## Lines

Text, LF line endings. The header is two lines: `bt-replay 2` and `subsystem wind_meteors 1.2` (the interface
version). A subsystem whose annotations are not complete yet (`status` in the schema is `pending`) writes
`incomplete` after the version, and the replayer refuses such a file. The file name carries the scenario.

A call is a line `call <ticks> <Slot>`, where `ticks` is `Game.ticks` when the call starts (`-` before the
game record exists) and `Slot` is a slot of the interface. Calls are in the order they start. A slot called by
another slot of the same subsystem (UpdateWind calls UpdateWindOnStream in a native build) is part of the outer
call and has no line of its own. The lines of a call, indented by two spaces, in this order:

| Line | Meaning |
|---|---|
| `r <field> <value>` | a field of the slot's read set that differs from the last known value of that field |
| `a <name> <value>` | an argument of the slot that is not in the block (`params.radius`, `bank`, `args[0]`, `stream`, a scalar, `player`) |
| `i <import> <arguments> = <result>` | one import call, in call order; its index is its position among the `i` lines of the call |
| `o <name> <value>` | right after an `i` line: a value the import wrote through a pointer argument (`o dst "text"`, `o dst x:0102`) or reached through its result (`o weapondef#17.flags 0x00000001`, `o Cell#1.metal 12`) |
| `w <field> <value>` | a field of the slot's write set that differs from the last known value after the call |
| `o <name> <value>` | last in the call: a value the slot wrote through its own pointer argument, or `o result <value>` for its result |

A call without any such line is idle. The last known value of a field is the one on the latest `r` or `w` line
of any earlier call, or the call's own tick for `Game.ticks`. A field with no known value counts as different.
The known values start empty at the first recorded call.

## Objects

Game objects that a subsystem reaches through pointers are written as `Kind#N`: `Player#1`, `Unit#3`,
`UnitDef#2`. A field of one is `Kind#N.field`, and it appears in `r`, `w` and `o` lines exactly as a block field
does, with the same last-known-value rule (the known value is kept per `Kind#N.field`). The kinds, their fields and
their value forms are the `objects` of the schema:

| Scope | Kind | `N` | Where it lives |
|---|---|---|---|
| state | `UnitDef` and the like | order of first appearance in the file, so it is stable across calls | in the block, linked by `Ref` |
| state, `table` | `Player`, `Unit` | the slot: `Player#3` is `Game::players[3]`, `Unit#17` is slot 17 of the unit table | the array in the block, or the slice of the unit pool |
| state, `inside` | `UnitResources` of a unit | the unit and the member: `Unit#17.resourceSlot` | a member of another kind |
| index | `weapondef` | the index in `Game::weapons` | in the block's weapon array |
| value | `MeteorParams` | none: the lines are written with the argument's name, `params.radius` | an argument struct of one call |
| handle | `file`, `rec`, `bank` | order of first use in the outermost call | the import's own objects, not state |

A kind with a `table` is named by its slot because the module walks it by address (`for (u = begin; u <= end; u++)`):
the replayer lays `Unit#N` out at `N` times the unit stride (0x118) in a pool it makes when a recording first names a
unit, and `Player#N` is in the player array of its `Game`, so a slice of the table is contiguous in the replayer's block
as it was in the game's. Numbering by first appearance could not rebuild a slice when one of its units had been named
earlier as an argument.

A pointer field is written as the object it points at (`Unit#1`) or `null`. The reachable objects of a call are the
object arguments (`a player Player#1`) and every object their pointer fields lead to, following the pointer fields
that are in the slot's read or write set for the kind. At entry the read-set fields of each reachable object are
written as `r` lines, at exit the write-set fields as `w` lines. The walk order (arguments, then breadth first, fields
in table order) numbers the objects named by first appearance, and the replayer walks its own block the same way. The replayer allocates
`Kind#N` in the block when a line first names it, stores `Ref` pointers as offsets from the block base, and reports a
changed field at its `Kind#N.field`. A pointer to an object no earlier line named is written `Kind#?`.

A pair of pointer fields that bound a slice of a table is a closed range: the schema marks the end field with
`"range_end_of": "unitsBegin"`, and `Player::unitsEnd` points at the last unit of the player's slice (the walk is
`u <= end`), not one past it. The end pointer is written as the object it points at, `Unit#249`, like any pointer. The
walk reaches every unit of `[begin, end]` in table order, whether or not the module's call touched it, because the
module's loop reads each one; when only one field of the pair is in the slot's sets, only that pointer is followed.

## Values

| Value | Written as |
|---|---|
| int, short, byte | decimal, signed or unsigned as the field's type |
| byte array | `x:` and two hex digits per byte: `x:00010100` |
| flags | `0x` and eight hex digits |
| float, double | hex bits: `f32:0x3f800000`, `f64:0x3ff0000000000000` |
| bytes of a buffer given to or filled by an import | `x:` and two hex digits per byte |
| Point16 | `(x,y)` |
| Vec3 | `(x,y,z)` |
| string | double quotes, C escapes, up to the first NUL |
| pointer to an object | `Unit#17`, `UnitDef#2`, `weapondef#17`, `Unit#17.resourceSlot`, or `null` |
| object handed to an import | `file#1`, `rec#2`, `bank#1` |
| random stream | `main` or `load` |

No address, no padding byte, no time and no module mode is ever written.

## Fields

Block fields are named as `data/rwsets.csv` names them: `Game.windCounter` for `Game::windCounter`,
`SimGlobals.g_meteorActive` for the global `g_meteorActive`. `rng.main`, `rng.load` and `rng.seed` are the two
generator streams of the block head and `g_randomSeed`; they are in the sets of the slots that draw.
`weapondef#17.flags` is the flags word of a weapon the module reads through a pointer it was given.

## The schema and its annotations

`data/replay_schema.json` has the `annotations` (written by hand, kept as written by the tool) and the generated
`fields`, `objects` and `subsystems` (one entry per simulation subsystem, with its slots and their read and write
sets). The annotations hold what no header says:

- `game_fields`, `sim_fields`, `exempt`: the block fields the recorder tracks, and the targets it ignores.
- `objects`: each kind's C++ type, scope and fields (name, value kind, target kind of a pointer). The generated header
  checks every size against the real struct.
- `rules`: a hand rule for each row the field sets cannot name: a `field_escape` row, or a `deref_read` or
  `deref_write` on a bare `*float`, `*short`, `*int` and the like. A rule is matched by `kind` and `target` (and by
  `functions`, when it lists the functions whose rows it covers: `*float` is the `energyMake` of a unit account in
  `AddIncome` and the extraction rate of a unit in `UpdateMetalExtraction`), and any row, not only an escape or a bare
  pointer, may be given one when the rows name only a part of what the code touches (`Point16::x` is a member of `Unit::cell`
  and of `Unit::footprint`). `whole-object` records every field of the named object; `index-array` records the weapon array
  through `weapondef#N`; `fields` adds the object fields it lists under `reads` and `writes` (`Unit.extraction`,
  `UnitResources.energyMake`), so that a write through a pointer is recorded as a `w` line of the field it lands on;
  `all-of` makes every slot of a table kind a root of the call (`Game::players` is walked by index). `replayschema.py
  --check` fails for a row of a subsystem function with no rule and for a rule that matches no row.
- `objects`' extras: `table` (`units` or `players`) names a kind by its slot in a table, `inside` (`{"kind": "Unit",
  "member": "resourceSlot"}`) names a kind that lives in a member of another kind (`Unit#17.resourceSlot`), a field of kind
  `embed` leads to such an object, `range_end_of` marks the end pointer of a closed range, and `aliases` (`Unit::cloakOn`) are
  the other targets of the rows that one field records (a bit field is recorded as its word).
- `subsystems.<name>`: `status` (`complete` or `pending`, with the `issue` that completes it), the `imports` (slots of
  other seams the subsystem calls) and per slot the `functions` behind it, the `native` symbol the replayer calls,
  and how each pointer argument is recorded:

| `mode` | Argument | Lines |
|---|---|---|
| `scalar` | a number, `Point16`, `Vec3` by value | `a name value` |
| `stream` | `RandStream` | `a stream main` |
| `text` | `char*` | `a name "text"` |
| `handle` | `HapiBank*` and the like | `a bank bank#1` |
| `object` | a pointer to a state kind, in | `a player Player#1`, its fields as `r` and `w` |
| `struct` (`dir` in or out) | a pointer to a value kind | `a params.field value` in, `o params.field value` out |
| `argv` | `CommandArgs*` | `a args.count 2`, `a args[0] "x"` |
| `bytes-in`, `bytes-out` | `void*` with a `length` argument | ` x:0102` in an import line, `o dst x:0102` after it (`bytes-out` is as long as the result, at most the length) |
| `weapon` | `WeaponDef*` | `weapondef#17` |
| `vec3` | `Vec3*` with a `label` | `pos=(x,y,z)` in an import line |
| `pending` | anything else | `?` |

A result is `scalar`, `object`, `text`, or `weapon` or `object` with `reach` fields recorded as `o weapondef#N.flags` or
`o Cell#N.metal` lines (the replayer returns an object it makes, with those fields set).
A `pending` subsystem lists in its generated `pending` entry what is left (untracked targets, rows with no rule,
slots whose functions are not found, arguments with no annotation); its own issue empties that list. A complete
subsystem with anything pending is an error.

`uv run tools/replayschema.py` also writes `src/harness/replay_schema.h` (the tables), `src/harness/replay_hooks.inc` (the
recording copy of every seam table, its argument hooks and the import hooks) and `tests/replay_slots.h` (the slot
runners, import stubs and native seam tables of the replayer). The services imports (`GameRand`, `RandomInt`, `FatalError`, `BuildDataPath`, the TDF and command classes) are the `services`
annotation: each entry names its declaration in `interfaces/services.h` (checked against the header) and how its arguments,
result and `o` lines are written. From it the tool writes `src/harness/replay_services.h` (the ids and names),
`src/harness/replay_services.inc` (the recorder's lines) and `tests/replay_services.h` (the replayer's answers). The one thing
it cannot write is the hook itself, `ReplayImport call(RI_X); call.Int(..)` at the top of each function in the game source:
those functions are the services themselves, reached by direct calls with no table to wrap, so the hook has to be in the
function; `replayschema.py --check` fails when a hook's arguments differ from its entry or an entry has no hook.

## Replaying

`tests/replay_native_test.cpp` replays a recording against the native module (`replay_native_test.exe`, built by
`cmake --build build/modern --target replay_native_test`, run under Wine in `modern-compile`, with the WASM replays below).
The subsystem is read from the recording's header; the module sources of a subsystem come from `data/subsystems.csv`
(`REPLAY_SUBSYSTEMS` in `CMakeLists.txt`).

```sh
wine build/modern/replay_native_test.exe tests/replay/wind_meteors/*.txt
```

One block is kept across the calls of a file: each `r` line is restored into it, the outer slot runs, every import is
answered from its `i` and `o` lines, and the write set and the closing `o` lines are compared with the file. A random
draw returns the recorded result, and the stream words advance as the game's do, so `rng.*` is checked like any field.
A call that differs is reported with the first import or field that differs and both values, and the file stops
there. Each file is replayed twice and the two results must be identical.

`--self-test [FILE]` first round-trips a small graph of objects (a `Player` with two `Unit`s sharing a `UnitDef`)
through the text form and reports a changed field at its `Kind#N.field`; with a recording it then mutates the
recording and expects each mutation reported. `replay_inject_test.exe --expect CALL FIELD FILE` runs the module built
with one changed constant and expects that divergence.

The same exe replays economy, whose module is native code in every mode: `replay_inject_test.exe` is built with
`BT_REPLAY_INJECT_WIND_SCALE=3.0`, which scales the wind income in `UpdatePlayerEconomy`, and the divergence is reported at the
first call whose wind generators were on and at `Player#0.energy`:

```sh
wine build/modern/replay_native_test.exe tests/replay/economy/*.txt
wine build/modern/replay_inject_test.exe --expect 5501 Player#0.energy tests/replay/economy/duel-shore-to-shore.txt
```

The recordings of economy and the scenarios they come from: `duel-shore-to-shore` (wind generators from tick 1620, metal
extractors from tick 480), `duel-coast-to-coast` (tidal generators from tick 3330, extractors) and `load-midgame` (the
accounts read back from a saved game, `LoadUnitAccounts`, and extractors) and `ten-ring-atoll`, ticks 6270 to 6270 only, where
the first metal maker earns income (the `makesMetal` branch of `UpdatePlayerEconomy`). `UpdatePlayerEconomy` walks every unit of the
player's slice of the unit table on each call, so a recording carries the `r` lines of all the slice at the first call and the
changes after it.

### Against the WASM module

`--wasm MODULE.wasm` replays the same recordings against `wind_meteors.wasm` instead of the native module, with the
same import formatting, comparisons and checks:

```sh
cmake --build build/modern --target wasm_runtime wasm_runtime_inject
wine build/modern/replay_native_test.exe --wasm build/modern/wind_meteors.wasm tests/replay/wind_meteors/*.txt
wine build/modern/replay_native_test.exe --wasm build/modern/wind_meteors_inject.wasm --expect 2 SimGlobals.g_meteorScheduleGap tests/replay/wind_meteors/meteors-hailstorm.txt
```

`tests/replay_wasm.cpp` loads the module through `lib/wasmloader` as `src/wasm/modules.cpp` does in the game: the
`wasmtime_host.dll` and `modules.json` beside the module file are used, the sim block of the module's memory is attached
as the replayer's block, and the slots are called through the seam the subsystem's glue builds. The glue calls are one
entry per subsystem in a table at the top of that file (wind_meteors only for now); the module file's name selects the
entry, and the recording's subsystem must be the same one. Each replay of a file loads the module afresh, so the two
runs of a file start from the same state. A trap or a fuel overrun stops the run with `FAIL`.
`wind_meteors_inject.wasm` is the module built with `BT_REPLAY_INJECT_GAP_SCALE=31.0` (`wasm_modules.py build-variant`),
and the divergence is reported at the same call and field as for the native one.
