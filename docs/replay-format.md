# Replay recordings

A recording holds the calls one subsystem received in a test-mode run: the state each call could read, the
imports it made with their arguments and results, and the state it changed. `bt-replay 1` is the format.
Recordings of the wind_meteors subsystem are in `tests/replay/`, one file per scenario:
`<scenario>.wind_meteors.txt`. The field tables are `data/replay_schema.json`
(`uv run tools/replayschema.py` writes it, from `data/rwsets.csv`).

A recording does not depend on the module mode: the same file comes from `-modules=wasm` and `-modules=native`.

## Switching it on

A scenario line `record wind_meteors` makes the run write `replay-wind_meteors.txt` when it ends without
failure. `record wind_meteors <first tick> <last tick>` records only the calls whose tick is in the window.
`tools/replay.py` adds the line to copies of the scenarios; `reference/scenarios/` never holds it.

```sh
uv run tools/replay.py --record --exe build/modern/TotalA.exe --out DIR [--native-modules] [SCENARIO...]
uv run tools/replay.py --check --exe build/modern/TotalA.exe [--native-modules] [SCENARIO...]
uv run tools/replay.py --compare DIR_A DIR_B
```

`--check` records again and fails when a recording of `tests/replay/` differs. `--compare` fails when two
directories differ and names the first differing line. Record on a fresh Wine prefix, as the gate runs.

## Lines

Text, LF line endings. The header is two lines: `bt-replay 1` and `subsystem wind_meteors 1.2` (the interface
version). The file name carries the scenario.

A call is a line `call <ticks> <Slot>`, where `ticks` is `Game.ticks` when the call starts (`-` before the
game record exists) and `Slot` is a slot of the interface. Calls are in the order they start. A slot called by
another slot of the same subsystem (UpdateWind calls UpdateWindOnStream in a native build) is part of the outer
call and has no line of its own. The lines of a call, indented by two spaces, in this order:

| Line | Meaning |
|---|---|
| `r <field> <value>` | a field of the slot's read set that differs from the last known value of that field |
| `a <name> <value>` | an argument of the slot that is not in the block (`MeteorParams`, `CommandArgs`, the stream) |
| `i <import> <arguments> = <result>` | one import call, in call order; its index is its position among the `i` lines of the call |
| `o <name> <value>` | right after an `i` line: a value the import wrote through a pointer argument or reached through its result |
| `w <field> <value>` | a field of the slot's write set that differs from the last known value after the call |
| `o <name> <value>` | last in the call: a value the slot wrote through its own pointer argument (`LoadMeteorDefaults`) |

A call without any such line is idle. The last known value of a field is the one on the latest `r` or `w` line
of any earlier call, or the call's own tick for `Game.ticks`. A field with no known value counts as different.
The known values start empty at the first recorded call.

## Values

| Value | Written as |
|---|---|
| int, short, byte | decimal, signed or unsigned as the field's type |
| flags | `0x` and eight hex digits |
| float, double | hex bits: `f32:0x3f800000`, `f64:0x3ff0000000000000` |
| Point16 | `(x,y)` |
| Vec3 | `(x,y,z)` |
| string | double quotes, C escapes, up to the first NUL |
| pointer into the weapon array | `weapondef#17`, or `null` |
| object handed to an import | `file#1`, `rec#2`, `bank#1`: numbered by first use in the outermost call |
| random stream | `main` or `load` |

No address, no padding byte, no time and no module mode is ever written.

## Fields

Fields are named as `data/rwsets.csv` names them: `Game.windCounter` for `Game::windCounter`,
`SimGlobals.g_meteorActive` for the global `g_meteorActive`. `rng.main`, `rng.load` and `rng.seed` are the two
generator streams of the block head and `g_randomSeed`; they are in the sets of the slots that draw.
`weapondef#17.flags` is the flags word of a weapon the module reads through a pointer it was given.
