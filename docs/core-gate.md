# The core gate

The core gate judges an engine core by what it does itself, before any module of the game exists: the hash of the
state, the random streams, the sim event queue, the delivery of commands, the module loader, fuel, hosting a module
against its recordings, and the files service. It does not play a scenario. The same gate judges a core on scenarios
later (the scenario mode, below), with the same command line and the same result.

The vectors are in `reference/core/` and `reference/files/`. Each was made by the in-repo engine, and a vector the
in-repo engine can't pass is wrong.

## Running a core

A core is one program. The gate runs it once for each vector, as

```sh
CORE GROUP CALLS OUT
```

in a directory that holds the vector's input files (the gate's inputs directory, below). `GROUP` is the name of a group
of the table below, `CALLS` is a text file of calls, one per line, and `OUT` is the file the core writes, one line for
each call in order. A file that a call names is a path relative to the current directory, with `/` as its separator.
The core exits with status 0 when it has answered every call or stopped at an error that ends the run (below). Each run
starts from nothing: no stream seeded, an empty queue, no manifest loaded, no module loaded.

The files group is the one exception: the gate runs it as the files gate runs a loader, `CORE files --game DIR CALLS OUT`
(`docs/files-gate.md`, "Running a loader").

A call is its name and its arguments, separated by tab characters. A line of `OUT` is the call's name, a tab, and its
value as JSON. The gate compares parsed values, so spacing within the JSON is free, but the types must match: an integer
is written as one (`1`, not `1.0` or `true`), and `null` is not `0`. Every number in a call is decimal, and may be
negative where a signed value is meant.

**An error that ends the run.** Where the engine ends the game (a full sim event queue in a test-mode run, a loader
refusal, a trap, running out of fuel), the core writes the call's line with the value the group states for it, answers
no further call, and exits with status 0. The gate then compares the lines it wrote with the stored ones, so an error at
the wrong call, or a missing one, is `differs`.

## The groups

| Group | What it covers |
| --- | --- |
| `hash` | the rows of `docs/trace-format.md` (sections 3, 6 and 10), recomputed from a dump, the `tick` row among them |
| `rand` | the random streams of the game logic and the generator of `RandomInt` (`interfaces/rand_stream.h`, `interfaces/services.h`): seeding, each kind of draw, and the stream words after each |
| `events` | the sim event queue of `docs/sim-events.md`, "The queue": recording, two-record events, the ring, the overflow in test mode and when playing, and draining |
| `commands` | the delivery of the commands of a tick (`docs/commands.md`, sections 1 and 2): their order and the tick each precedes |
| `loader` | the manifest and module checks of `docs/module-abi.md`, section 5.2, and the start-up errors and traps of section 13 |
| `fuel` | the fuel of section 11: the budget set for each tick, the fuel left, and the call at which a module runs out |
| `host` | hosting a module: the recordings of `tests/replay/wind_meteors/` replayed against a wind_meteors module, as `docs/replay-format.md` states ("Replaying") |
| `files` | the files gate (`docs/files-gate.md`), with its own groups |

A vector file is `reference/core/<group>.json`: `{"format", "group", "vectors"}`, where each vector is `{"name",
"calls", "results"}`: the calls, each a list of strings, and the `[call, value]` the in-repo engine wrote for each.
The input files the calls name are in `reference/core/<group>/`, and the gate copies them into the inputs directory at
the same relative paths, unpacking each `.xz` file under its name without `.xz`. It also puts there
`trace_schema.json` (the kit's `data/trace_schema.json`) and `module.wasm`, the core's wind_meteors module.

### hash

| Call | Arguments | Value |
| --- | --- | --- |
| `rows` | dump, schema | the rows the dump gives, in the order of the dump's blocks: a list of `[tick, phase, hash]`, `tick` an integer, `phase` a row name of `docs/trace-format.md`, section 2 (`units` to `meteors`, `world` and `tick`), and `hash` its 16 lowercase hex digits |

The dump is a `dump.txt` of a test-mode run (`docs/trace-format.md`, section 10), and the schema is the kit's
`data/trace_schema.json`, which the gate copies to `trace_schema.json`. A dump gives a phase row for each phase block,
the `world` row after the `meteors` row, and the `tick` row of its `tick` block. The rows of a vector are the ones the
in-repo engine wrote to `trace.txt` in the run that wrote the dump. The dumps are of ticks after the first, so they
hold no section 29 to 31.

### rand

A stream is `main` or `load` (`RAND_STREAM_MAIN` and `RAND_STREAM_LOAD`), and `seed` names the generator of
`RandomInt`, which is not a stream of `GameRand`. A word is a stream's or the generator's 32-bit state, as an unsigned
integer.

| Call | Arguments | Value |
| --- | --- | --- |
| `srand` | stream, seed | seeds the stream: its word afterwards |
| `rand` | stream | `GameRand`: `[value, word]`, the value drawn and the stream's word afterwards |
| `shuffle` | stream, d | one draw of a shuffle from the stream for a range of `d` (the draw of a shuffle that widens the value 15 bits at a time while the range is above 0x7fff, and returns it modulo `d`): `[value, word]` |
| `seed` | seed | seeds the generator: its word afterwards |
| `int` | range | `RandomInt(range)`: `[value, word]` (a range below 2 gives 0 and leaves the word alone) |
| `state` | none | `[main, load, seed]`: the three words |

Every vector seeds each stream and the generator before it draws from them, so the words before the first seeding are
not judged. These are the words a trace hashes as `rng.crtHoldrand` (the `main` stream) and the Park-Miller state of `rng` (the
generator); the `load` stream is in no row.

### events

The queue is the one of `docs/sim-events.md`. A record is written `[kind, flags, tick, [six arguments]]`, each an
unsigned integer. A run starts in test mode, with the tick counter 0 and the queue empty.

| Call | Arguments | Value |
| --- | --- | --- |
| `mode` | `test` or `play` | `null`: the mode of the run from now on |
| `tick` | n | `null`: the tick counter that the events recorded from now on carry |
| `record` | kind, flags, then 0 to 12 arguments | `[head, tail, dropped]` afterwards; in test mode, an event that does not fit ends the run with the value `"full"` |
| `repeat` | count, kind, flags, then 0 to 12 arguments | `record` of the same event `count` times: `[head, tail, dropped]` after the last; in test mode, the one that does not fit ends the run with `"full"` |
| `counters` | none | `[head, tail, dropped, reserved]` |
| `ring` | index | the record at that index of the ring (0 to 4095) |
| `pending` | none | the pending records, oldest first, without draining them |
| `drain` | none | the records the drain hands over, oldest first; the queue is empty afterwards |

An event of seven to twelve arguments is two records, as `docs/sim-events.md`, "The record", states, and both are in
`pending`, `drain` and the ring. `flags` is the event's flags; the core sets bit 15 of the second record itself.

### commands

| Call | Arguments | Value |
| --- | --- | --- |
| `deliver` | script | the lines of `commands.txt` (`docs/commands.md`, section 2) for the commands of the script, without its first line, in the order the core delivers them: a list of strings |

A script is a text file whose first line is `bt-script 1`; every later line is one scripted line of a scenario, in the
order of the scenario file, as `<line> <key> <tick>` followed by what the command publishes, as a line of
`commands.txt` holds it after its tick, key and line (`<count>[ <KIND> <hex>]...`). Turning a line into events needs the
modules (the map, the orders), so the script gives them, as the in-repo engine published them in a test-mode run of the
scenario; what the group judges is the delivery: the order of section 1 (the ticks in order, and within a tick the
`spawn` lines, then the `order` lines, then the `console` lines, each group in file order), and the tick each command
precedes. The vectors' scripts are made from scenarios of `docs/commands.md`, section 7, judged ones, and the expected
lines are the `commands.txt` the in-repo engine wrote for them.

### loader and fuel

The two groups share their calls. A vector's manifest and modules are files of `reference/core/loader/` or
`reference/core/fuel/`, made from the WebAssembly text beside each module (`NAME.wat`), with the fuel pass of
`docs/module-abi.md` section 11 applied to the modules of the fuel group. The manifests have the layout of section 4.

| Call | Arguments | Value |
| --- | --- | --- |
| `manifest` | manifest | `null` when the manifest is accepted; otherwise an error (below) |
| `memory` | none | `null` once the memory and the table are created; otherwise an error |
| `load` | name, module | `null` when the module is loaded (section 5.1, step 4); otherwise an error |
| `tick` | none | `null`: a new tick starts, so the next slot call of each module is the first of its tick and sets its fuel (section 11.2) |
| `call` | name, export, then i32 arguments | `{"result": n}` for a call that returns an i32, `{"result": null}` for one that returns nothing; otherwise an error |
| `fuel` | name | the module's counter, as its `__fuel_get` returns it; the call sets no fuel, so before the module's first `call` it is the counter's initial value |
| `word` | address | the i32 at that address of the shared memory |

An error is `{"error": CLASS, "modules": [names]}`: its class, below, and the modules the message names (both, for an
overlap; the module whose entry is wrong for a `module_entry` error that has one; none for an error of the manifest
itself). Every error ends the run (section 13), so it is the last line. Only a module whose manifest entry has a
`fuel_budget` gets its fuel set; a `tick` before its first call is not needed for that.

| Class | Section | When |
| --- | --- | --- |
| `manifest` | 5.2 | the manifest is not valid JSON or not an object, a required field is missing, a number is not a non-negative integer, `memory_pages` or `window` is 0 or `memory_pages` above 65536, `scratch` is not `[base, size]` |
| `layout` | 5.2 | the block or the scratch area does not fit the memory, the scratch area passes the block base, or `block_max` is below `block_size` |
| `module_entry` | 5.2 | no modules, a module without a name, a name listed twice, a missing `data_base` or `table_base`, `exports` or `data` that is not a list of names |
| `window` | 5.2 | a module's window does not fit the memory, or its table range does not fit the table |
| `window_reaches` | 5.2 | a window reaches the block base or the scratch base |
| `overlap` | 5.2 | two windows overlap, or two table ranges |
| `not_in_manifest` | 5.2 | a module loaded that the manifest does not list |
| `loaded_twice` | 5.2 | a module loaded a second time |
| `no_memory` | 5.2 | a module loaded before the memory exists |
| `compile` | 5.2 | the module does not compile |
| `instantiate` | 5.2 | the module does not instantiate: an import the host did not define, an import of another type, a memory or table import whose limits do not fit |
| `export` | 5.2 | a name of `exports` that is not a function export, or of `data` that is not an i32 global export |
| `unreachable` | 13 | a trap at `unreachable` |
| `memory_access` | 13 | an access outside the memory |
| `divide` | 13 | an integer division by zero, or an integer overflow trap |
| `table` | 13 | a call through an empty table entry or one of the wrong type |
| `stack` | 13 | the engine's stack overflows |
| `out_of_fuel` | 13 | the module's counter went below zero (section 11) |

The host defines no import beyond the memory and the table, so every fixture module imports only those, except the
ones that refuse to instantiate.

### host

| Call | Arguments | Value |
| --- | --- | --- |
| `replay` | recording, module | a list with one entry for each `call` line of the recording, in order: the list of the `w` lines, then the closing `o` lines (`docs/replay-format.md`, "Lines"), that the core's replay of that call gives, each a string without its indentation, as `"w SimGlobals.g_meteorScheduleGap 1800"` |

The recording is a copy, in `reference/core/host/`, of a file of `tests/replay/wind_meteors/`, and the module is
`module.wasm`. The core replays the
recording as `docs/replay-format.md` states: it restores each `r` line, calls the slot with the `a` lines, answers each
import from its `i` and `o` lines, and writes, for the call, the fields of the write set that differ from the last known
value and the closing lines. The expected values are the recording's own lines. A trap or running out of fuel ends the
run with an error of the loader classes.

The gate never gives the core the in-repo engine's wind_meteors module. The module is the one that comes with the
core's submission (below): the clean room's own, which the module gate has judged.

### files

The gate judges the core as `tools/filesgate.py check --loader CORE` does (`docs/files-gate.md`), so the core answers
the files gate's calls as a loader does, and reports the files gate's groups (`formats`, `listing`, `lookup`,
`mounting`, `reading`, `malformed`, `cob`, `gaf` and `3do`) under `files` in the result, with the files gate's own
verdicts. When the files gate can't run (its exit status 2: its vectors can't be read or its fixtures don't rebuild),
the group is `failed`, with the files gate's message as its `reason`.

## The scenario mode

```sh
CORE play SCENARIO --out DIR
```

plays a scenario of `reference/scenarios/` (`docs/scenario-format.md`) in a test-mode run and writes `DIR/trace.txt`
(`docs/trace-format.md`, section 2) and `DIR/events.txt` (the sim event log, `docs/events-log.md`). The gate compares
each with the stored digests: the trace on its `state` column (the `tick` row), and the event log on the `tick` lines of
its digest, for the scenarios of `docs/commands.md`, section 7, marked judged. It is stated now and judged later: until a
core can play a scenario (it needs the modules of every subsystem), the result reports it `not_judged`, and no vector of
it is stored.

## Running a core the gate doesn't trust

**The form.** A core is one file: an x86_64 Linux executable (ELF, 64-bit, little-endian, machine 62), statically linked
(musl), so that it needs nothing of the machine it runs in but the kernel. It is built in the clean-room image
(`docs/cleanroom.md`) and comes with the wind_meteors module the `host` group runs it against.

**The sandbox.** The gate runs it only inside a microVM of that image, the sandbox of the clean room, never on the host:

- one machine for the gate run, with no network device, made from the clean-room image;
- one directory of the host shared with it, at the same path inside the machine: the inputs directory, the core and its
  module, and the files the core writes. Nothing else of the host is visible in the machine;
- each run of the core is one command executed in the machine, with a time limit of 120 seconds;
- the gate writes every input, the calls of every vector included, into the shared directory before the core first
  runs, and nothing after. It reads each `OUT` without following a link the core made, through descriptors of the
  inputs directory taken before the first run, and compares it with the stored vectors on the host. The stored values
  never enter the machine, and only `result.json` crosses back to the clean room, as for a module
  (`docs/module-abi.md`, section 15, and `docs/cleanroom.md`, "Gate").

**How a run can fail.** Each run of a vector gets one of these, and the others of its group still run:

| What the core does | The vector's verdict |
| --- | --- |
| exits with status 0 and writes a line for each call, or stops at an error that ends the run | `equal` or `differs`, by comparison |
| writes fewer lines than the stored vector has, and exits 0 | `differs`, at the first missing line, with the value `missing` |
| crashes, or exits with a status other than 0 | `failed`, `reason` `exit status N` (a signal is reported as its negative number) |
| runs past the time limit | `failed`, `reason` `timed out after 120 s`; the gate stops waiting for it, and the machine is deleted at the end of the gate run |
| writes no `OUT`, or a line that is not a call name, a tab and JSON | `failed`, `reason` `no output file` or `malformed output: ...` |
| leaves at `OUT` something other than a regular file (a link, a directory, a pipe), or replaces a directory on its path with one | `failed`, `reason` `OUT is not a regular file` |
| writes an `OUT` of more than 256 MiB | `failed`, `reason` `OUT is larger than 268435456 bytes` |
| can't be started | `failed`, `reason` `could not start the core: ...` |

**The mailbox.** Through the mailbox of the clean room, a request is `outbox/ID/core` (the executable),
`outbox/ID/module.wasm` (its wind_meteors module) and `outbox/ID/ready`, and the answer is `inbox/ID.json`, the
`result.json` below. A request with a `subsystem` file is a module request (`docs/module-abi.md`, section 15). The broker
runs every core request in the sandbox. Running a core directly on the host is only for the in-repo engine's adapter,
which is trusted, and `tools/coregate.py` does it only when told so with `--trusted`.

## The verdict

```sh
uv run tools/coregate.py CORE --sandbox IMAGE --module MODULE.wasm [--group G] [--out DIR]   # a submitted core
uv run tools/coregate.py [--group G] [--out DIR] --trusted                                   # the in-repo engine
```

writes `DIR/result.json` (default `build/core-gate/result.json`) in the fields of `docs/module-abi.md`, section 15:

| Field | Meaning |
| --- | --- |
| `format` | 1 |
| `verdict` | `equal`, `differs`, `failed` or `refused` |
| `suite` | `core` |
| `core` | `sha256` and `bytes` of the core (absent for the in-repo engine) |
| `kit` | `interfaces` and `reference_traces`: the newest `interfaces-v*` and `reference-traces-v*` tags |
| `seconds` | wall time of the run |
| `message` | with `refused`: why the core was not run |
| `groups` | by group: `status` and the number of `vectors`; `files` also has `groups`, the files gate's own groups, or a `reason` when the files gate could not run; `play` is `not_judged` |
| `vectors` | by `<group>/<vector>` (the files group's as `files/<group>/<vector>`): `status` and, for `differs`, `line` (the first call that differs, from 1), `call` and `value` (what the core wrote, or `missing`); for `failed`, `reason` |

A group is `failed` when one of its vectors is, else `differs` when one differs. The verdict is `failed` when a group
is, else `differs` when one differs, and `play` does not count. The verdict is `refused` when the core is not an
x86_64 Linux executable or the sandbox could not start, and then `message` says which and there are no groups. A
`value` is what the core produced; the stored value is never in the file.

Exit status: 0 when every judged group is equal, 1 when one differs or failed, 2 when the core was refused or the gate
could not run (the vectors can't be read, or neither `--sandbox` nor `--trusted` was given).

## Known limits

- **The order of the tick** (`docs/schedule.md`) is judged by no group: it needs the modules to run. The scenario mode
  judges it, once enough modules exist.
- **Translating a scripted line into events** is judged only by the scenario mode: a `spawn` or `order` line can't
  become events without the map (its ground height and footprint snapping) and the orders (`GetOrderType`). The
  `commands` group judges what the core itself does with them: the order of delivery, the tick each command precedes,
  and that only outermost publications are logged and counted.
- **The radar arena** of `docs/sim-events.md` is not called: its snapshots are written by the logic of los_radar.
- **The services a module imports** (`docs/module-abi.md`, section 9.3), and the import refusals and glue errors of
  section 13, are judged by the `host` group only as far as the recordings call them; the loader's fixture modules import
  nothing but the memory and the table.
- **Hidden state** (section 12) is checked by the module gate on each module, not here.
