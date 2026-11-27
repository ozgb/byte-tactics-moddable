# The core gate

The core gate judges an engine core by what it does itself, before any module of the game exists: the hash of the
state, the random streams, the sim event queue, the delivery of commands, the module loader, fuel, hosting a module
against its recordings, the files service, the console lines a core reads alone, and the results of the runtime. It does not play a scenario. The same gate judges a core on scenarios
later (the scenario mode, below), with the same command line and the same result.

The vectors are in `reference/core/` and `reference/files/`, and the runtime's in `tests/runtime/vectors/` (the kit's
`runtime/vectors/`). Each was made by the in-repo engine, and a vector the
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
| `host` | hosting a module: the recordings of `tests/replay/wind_meteors/` replayed against a wind_meteors module, as `docs/replay-format.md` states ("Replaying"), and the import refusals and the full handle table of `docs/module-abi.md` sections 8.1 and 13 on fixture modules |
| `files` | the files gate (`docs/files-gate.md`), with its own groups |
| `console` | reading a console line (`docs/commands.md`, section 3) and the events of the commands of section 4.2 that need nothing but a stated match setup |
| `runtime` | the results of the runtime calls of the game logic: the entries of the runtime vectors, one call per record |

A vector file is `reference/core/<group>.json`: `{"format", "group", "vectors"}`, where each vector is `{"name",
"calls", "results"}`: the calls, each a list of strings, and the `[call, value]` the in-repo engine wrote for each.
The input files the calls name are in `reference/core/<group>/`, and the gate copies them into the inputs directory at
the same relative paths, unpacking each `.xz` file under its name without `.xz`. It also puts there
`trace_schema.json` (the kit's `data/trace_schema.json`) and `module.wasm`, the core's wind_meteors module.

### hash

| Call | Arguments | Value |
| --- | --- | --- |
| `rows` | dump, schema | the rows the dump gives, by tick and then in the row order of `docs/trace-format.md`, section 2, whatever the order of the dump's blocks: a list of `[tick, phase, hash]`, `tick` an integer, `phase` a row name of `docs/trace-format.md`, section 2 (`units` to `meteors`, `world` and `tick`), and `hash` its 16 lowercase hex digits |

The dump is a `dump.txt` of a test-mode run (`docs/trace-format.md`, section 10), and the schema is the kit's
`data/trace_schema.json`, which the gate copies to `trace_schema.json`. A dump gives a phase row for each phase block,
the `world` row after the `meteors` row, and the `tick` row of its `tick` block. The rows of a vector are the ones the
in-repo engine wrote to `trace.txt` in the run that wrote the dump. The vectors are named `<scenario>-<tick>`, and
between them they reach:

| Vector | What it reaches |
| --- | --- |
| `edges-economy-targeting-200` | the overflow record (`rec:overflow`), `link:` pointers of the units' order lists, and the pathfinder's `node:` pointers, which sections 16 and 17 dump and no row hashes |
| `orders-attack-nomove-1` | tick 1, the only tick whose rows hash sections 29 to 31 (the unit types, the weapons and the mission), and live features |
| `orders-air-fights-700` | non-null `list2` pointers, the overflow record, `link:` pointers, and path-order effects |
| `orders-guard-attacked-300` | live projectiles, a live feature, the overflow record, `link:` pointers and path-order effects |
| `meteors-earthquake-102` | many live features (the earthquake's) and a projectile |

No vector reaches a `link:` value of 2^31 or more (a link into a path order, `docs/trace-format.md`): no dump of the
scenarios that was searched holds one (Known limits). No vector's dump holds a string that contains a newline byte
(0x0a), which the dump cannot carry (`docs/trace-format.md`, section 8.19), so the gate never judges one.

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
| `deliver` | script | the lines of `commands.txt` (`docs/commands.md`, section 2) for the commands of the script, without its `bt-commands`, `units` and `end` lines, in the order the core delivers them: a list of strings |

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
Section 5.2's refusals of the numbers are vectors for each number a manifest has: `stack_size` (`manifest_stack_negative`,
`manifest_stack_fraction`), `fuel_budget` (`manifest_fuel_negative`, `manifest_fuel_fraction`, `manifest_fuel_text`, and
`manifest_fuel_zero`, since section 11.3's range starts at 1) and `size_budget` (`manifest_size_budget_negative`,
`manifest_size_budget_fraction`). `manifest_pages_max` accepts `memory_pages` 65536 with the block at its ceiling and
a `fuel_budget` of 2147483647, and the fuel group's `budget_max` runs a module on that budget.

| Call | Arguments | Value |
| --- | --- | --- |
| `manifest` | manifest | `null` when the manifest is accepted; otherwise an error (below) |
| `memory` | none | `null` once the memory and the table are created; otherwise an error |
| `load` | name, module | `null` when the module is loaded (section 5.1, step 4); otherwise an error |
| `tick` | none | `null`: a new tick starts, so the next slot call of each module is the first of its tick and sets its fuel (section 11.2) |
| `call` | name, export, then i32 arguments | `{"result": n}` for a call that returns an i32, `{"result": null}` for one that returns nothing; otherwise an error |
| `fuel` | name | the module's counter, as its `__fuel_get` returns it; the call sets no fuel, so before the module's first `call` it is the counter's initial value |
| `word` | address | the i32 at that address of the shared memory |

An error is `{"error": CLASS, "modules": [names]}`: its class, below, and the modules it names, by class:

- `manifest` and `layout` name none.
- `module_entry` names the entry's name when the entry has one (`manifest_no_data_base`: `["a"]`). It names none when
  the entry has no name, or when the fault is in `modules` itself (`manifest_no_name`, `manifest_no_modules`,
  `manifest_modules_missing`: `[]`). A name listed twice is named once (`manifest_duplicate`: `["a"]`).
- `window` and `window_reaches` name the module at fault (`manifest_window_memory`, `manifest_window_table`,
  `manifest_reaches_block`, `manifest_reaches_scratch`: `["b"]`).
- `overlap` names both modules, the earlier in the manifest first (`manifest_overlap`, `manifest_overlap_table`:
  `["a", "b"]`).
- The classes of `load` and `call` name the module loaded or called (`not_in_manifest`: `["zz"]`), except the
  interface-major refusal at `load`, a `manifest` error, which names none (`load_major_newer`).

Every error ends the run (section 13), so it is the last line. Only a module whose manifest entry has a
`fuel_budget` gets its fuel set; a `tick` before its first call is not needed for that.

| Class | Section | When |
| --- | --- | --- |
| `manifest` | 5.2, 10 | the manifest is not valid JSON or not an object (a number anywhere in it that has a fraction or an exponent, as `4096.5`, `4096.0` or `1e3`, or is above 2147483647, makes it invalid JSON: `manifest_data_base_fraction`), a required number of section 4.1 (`memory_pages`, `table_size`, `window`, `table_slots`) is missing, a value is not a non-negative integer (a negative `data_base` or `table_base` of a module entry included), `memory_pages` or `window` is 0 or `memory_pages` above 65536, `stack_size` is not a non-negative integer, `block_base` comes without `block_size`, `scratch` is not `[base, size]`, a module entry's `fuel_budget` is not an integer from 1 to 2147483647 or its `size_budget` not a non-negative integer; and at `load`, a refusal of the interface majors (section 10.2: a module built against a major the manifest's does not match, with no adapter, or an adapter of section 10.2 that is refused), whose `modules` is empty |
| `layout` | 5.2 | the block or the scratch area does not fit the memory, the scratch area passes the block base, or `block_max` is below `block_size` |
| `module_entry` | 5.2 | `modules` missing, not a list or empty, an entry that is not an object or has no name, a name listed twice, a missing `data_base` or `table_base`, `exports` or `data` present and not a list of names |
| `window` | 5.2 | a module's window does not fit the memory, or its table range does not fit the table |
| `window_reaches` | 5.2 | a window reaches the block base or the scratch base |
| `overlap` | 5.2 | two windows overlap, or two table ranges |
| `not_in_manifest` | 5.2 | a module loaded that the manifest does not list |
| `loaded_twice` | 5.2 | a module loaded a second time |
| `no_memory` | 5.2 | a module loaded before the memory exists |
| `compile` | 5.2 | the module does not compile |
| `instantiate` | 5.2 | the module does not instantiate: an import the host did not define, an import of another type, a memory or table import whose limits do not fit, or a trap in its start function (whatever the trap) |
| `export` | 5.2 | at `load`, a name of `exports` that is not a function export, or of `data` that is not an i32 global export; at `call`, an export the module does not have |
| `unreachable` | 13 | a trap at `unreachable` |
| `memory_access` | 13 | an access outside the memory |
| `divide` | 13 | an integer division by zero, an integer overflow trap, or a float-to-integer conversion of a value out of range or a NaN |
| `table` | 13 | a call through an empty table entry, one of the wrong type, or an index at or past `table_size` (`trap_table_range`: index 100 of a 16-entry table) |
| `stack` | 13 | the engine's stack overflows |
| `out_of_fuel` | 13 | the module's counter went below zero (section 11) |

Beyond the memory and the table, the host defines one import at `memory`, `fixture.mix`, of the signature
`[[int, int, int], int]`: it returns a + 10 * b + 100 * c of its arguments a, b and c, in i32 arithmetic that wraps. The
fixture modules import nothing else, except the ones that refuse to instantiate. `host_import` calls it unadapted.

**NaN.** `nan_canonical` returns and stores computed NaNs (section 3.2): f32 0/0, -1/0 (an infinity, -8388608), a NaN
with a payload plus 1 (of either sign), the square root of -1, and an f64 0/0 and -5/0 (an infinity) stored to memory
and read back as two words. Every NaN is the canonical one, 2143289344 (0x7fc00000), and 0 then 2146959360 (0x7ff80000)
for the f64.

**Adapters** (section 10.2). A module built against major 1 of an interface whose manifest major is 2 loads only
through an adapter of its entry, and each refusal is a `manifest` error at `load`, with `modules` empty:

| Vector | What it judges |
| --- | --- |
| `adapter_slot` | four adapted slots of `adapt.wasm`: `f` called with two arguments reaches the module's three as `[1, 0, {"const": 7}]` (2, 3 gives 723); `g` with none gets the constant 4 and returns the constant 9; `h` returns nothing (`"void"`) though the module's returns an int; `f16`, of 15 parameters now and 16 in the module, passes the 15 reversed and the constant 100 last (1 to 15 gives 15 - 100, -85) |
| `adapter_import` | an adapted import: the module's `fixture.mix` of two parameters reaches the host's of three as `[1, {"const": 5}, 0]` (2, 3 gives 253) |
| `adapter_none` | a slot that changed with no adapter |
| `adapter_kind`, `adapter_const_kind` | an `args` entry that changes a parameter's kind (`linear` to `int`), a constant for a parameter that is not `int` |
| `adapter_index`, `adapter_index_negative` | an `args` index at or past the caller's parameter count, or negative |
| `adapter_args_length` | `args` of another length than the callee's parameters |
| `adapter_result_form`, `adapter_result_const_kind`, `adapter_result_pass_void`, `adapter_result_pass_kind`, `adapter_result_void` | a `result` that is not `"pass"`, `"void"` or `{"const": n}`; a constant for a result that is not `int`; `"pass"` when the caller's result is `void`, or of another kind; `"void"` when the caller expects a result |
| `adapter_values` | a signature of 17 parameters (the most is 16) |
| `adapter_import_undefined`, `adapter_import_signature` | an adapted import the host does not define, or defines with another signature than the manifest's |
| `major_not_number`, `major_manifest_not_number` | a module's major, or the manifest's, that is not a number |

A `block_size` without `block_base` is no block, and is not refused. `exports` and `data` may be left out of an entry: the
module is then loaded with no slots and no data words, and `call` still reaches any function the module exports, since
`call` names an export, not a slot of `exports`. A module whose entry has no `fuel_budget` gets no fuel set: `fuel` gives its
counter's initial value, 9223372036854775807, which its calls lower as they burn fuel (the fuel group's `unbudgeted`).

**Which error comes first.** When a manifest holds more than one fault, the error is the first fault in this order:

1. the manifest as a whole: valid JSON (so a number with a fraction or an exponent, wherever it is, is a fault of this
   step, before any `layout` fault) and an object, then the required numbers in the order `memory_pages`, `table_size`,
   `window`, `table_slots`, then `stack_size`, then their ranges (`manifest`);
2. the block, when `block_base` is present: `block_base` and `block_size` (`manifest`), then `block_max`, then whether
   the block fits the memory (`layout`);
3. `scratch`: its form (`manifest`), then whether it fits the memory and lies below the block base (`layout`);
4. `modules` (`module_entry`), then each entry in the manifest's order, each one whole before the next: an object with
   a name, `data_base` and then `table_base` (missing: `module_entry`; negative: `manifest`), `fuel_budget` and then
   `size_budget` (`manifest`), `exports` and then `data`, then a name an earlier entry has (`module_entry`);
5. once every entry is read, each module in the manifest's order: its window and table range (`window`), its window
   against the block base and then the scratch base (`window_reaches`), then its overlap with each later module in
   turn, each pair checked memory first and then table before the next pair (`overlap`). So a `window` or
   `window_reaches` fault of a module comes before its overlap with a later one, and an overlap of two modules comes
   before a `window` fault of a third, later module;
6. at `load`: a module not in the manifest (`not_in_manifest`), no memory (`no_memory`), a module loaded already
   (`loaded_twice`), the interface majors (`manifest`), then `compile`, `instantiate` and `export`.

A vector `pair_<a>_<b>` holds a fault of class `a` and one of class `b` in one manifest, and expects `a`, for each pair
of the classes `manifest`, `layout`, `module_entry`, `window`, `window_reaches` and `overlap` (in the pairs of `window`,
`window_reaches` and `overlap`, both faults are of the first module). The `manifest` fault of each `pair_manifest_<b>`
vector is a step 1 fault (`window` 0), so the order list above, not this sentence, decides between a `manifest` fault of
a later step and a fault of an earlier one: a `scratch` that is not `[base, size]` (step 3, `manifest`) against a block
that does not fit the memory (step 2, `layout`) gives `layout`. `manifest_entry` in a name is a `manifest` fault of an
entry (a negative `data_base` or `table_base`), which comes in step 4, after `layout`.

The gate makes no `call` or `fuel` call that names a module not loaded, and no `fuel` call on a module without
`__fuel_get`: they are not part of the call text, and a core may answer them as it likes.

### host

| Call | Arguments | Value |
| --- | --- | --- |
| `replay` | recording, module | a list with one entry for each `call` line of the recording, in order: the list of the `w` lines, then the closing `o` lines (`docs/replay-format.md`, "Lines"), that the core's replay of that call gives, each a string without its indentation, as `"w SimGlobals.g_meteorScheduleGap 1800"` |

The recording, in `reference/core/host/`, is a file of `tests/replay/wind_meteors/` with what the core must produce taken
out: every `w` line and every closing `o` line (`docs/replay-format.md`, "Lines") is withheld, and the rest (the `call`
lines, the `r`, `a`, `i` and `c` lines, and each import's own `o` lines) is kept in order. The module is `module.wasm`,
except for the refusal vectors below.
The core replays the recording as `docs/replay-format.md` states: it restores each `r` line, builds the slot's arguments
from the `a` lines (an event payload as "Event arguments", "The payload in memory", states), calls the slot of its
module, answers each import from its `i` and its own `o` lines, and writes, for the call, the fields of the write set
whose value after the call differs from the last known value, and the closing lines of what the slot wrote through its
own arguments and returned. The last known value of a field is as `docs/replay-format.md`, "Lines", defines it, with
the `w` lines the core wrote in place of the withheld ones. The expected values are the lines the gate withheld, which the in-repo engine writes
when it runs its own wind_meteors module on the same input, so a core passes only by running the module and reading
what it wrote: the stored lines are not in its input (`tools/test_coregate.py` checks that a core that echoes the lines
of a recording fails). A trap or running out of fuel ends the run with an error of the loader classes, and two errors
of section 13 end it with an error that also carries the text: an import's refusal, `{"error": "import", "modules":
["wind_meteors"], "message": TEXT}`, where TEXT is section 13's text from `import ` to the end of its line, and a full
handle table (section 8.1), `{"error": "glue", "modules": ["wind_meteors"], "message": "wind_meteors: the handle
table is full"}`.

**The refusal vectors.** Each replays a short recording of `reference/core/host/` against a fixture module beside it,
`wind_meteors_<vector>.wasm`, made from its `.wat` with the fuel pass. A fixture is shaped as a wind_meteors module of
the game manifest (`wasm/modules.json`): it imports the memory, the table and the services it calls, exports the four
data words at 0x30000 to 0x3000c and the eight slots, and does in one slot what its vector judges:

| Vector | Slot and imports | Value |
| --- | --- | --- |
| `import_pointer` | `FatalError(-16)` | `import FatalError: pointer argument 0 is outside the module memory` |
| `import_pointer_last` | `BuildDataPath` with argument 3 the memory's size, the first address past it | `import BuildDataPath: pointer argument 3 is outside the module memory` |
| `import_buffer_size` | `TdfFile_LoadFile` issues handle 1, then `TdfRecord_GetFieldString` of it with `size` 0 | `import TdfRecord_GetFieldString: the buffer size is below 1` |
| `import_buffer_past` | `TdfFile_LoadFile` issues handle 1, then `TdfRecord_GetFieldString` of it with a buffer of 5 bytes 4 below the memory's end | `import TdfRecord_GetFieldString: the buffer runs past the module memory` |
| `import_null_handle` | `TdfRecord_GetFieldInt` of handle 0 | `import TdfRecord_GetFieldInt: null handle` |
| `import_unknown_handle` | `TdfFile_LoadFile` issues handle 1, then `TdfRecord_GetFieldInt` of handle 2 | `import TdfRecord_GetFieldInt: unknown handle 2` |
| `import_stale_handle` | the first call keeps handle 1 in a static, the second passes it back: the table was cleared when the first returned | `import TdfRecord_GetFieldInt: unknown handle 1` |
| `handles_32` | `LoadMeteorDefaults` gets a root record and 31 `TdfFile_SelectRecord` results, 32 distinct records in one call, and writes the last handle to `params.radius` | `o params.radius 32`: 32 handles fit, numbered from 1 |
| `handles_full` | as `handles_32`, with a 33rd record | `wind_meteors: the handle table is full` (class `glue`) |
| `import_string_cut` | `LoadMeteorDefaults` reads the field `Meteor` with `size` 3 into `params.weapon` | `o params.weapon "Me"`: the copy is cut to `size` - 1 bytes and a 0 (`docs/replay-format.md`, a text out with a `size`) |
| `emit_size`, `emit_past`, `emit_kind`, `emit_payload_size`, `emit_weapon` | `Emit` of a SPAWN of 3 bytes, of one that runs past the memory, of kind 81, of 40 bytes, and of weapon 256 | section 13's five `Emit` texts; a size of 3 is refused as outside an effect before its kind is read (section 9.3 gives the order of the checks) |

The gate never gives the core the in-repo engine's wind_meteors module. The module is the one that comes with the
core's submission (below): the clean room's own, which the module gate has judged.

### console

A vector states a match, then gives console lines, and each line is run as a `console` line of a scenario is when it is
delivered (`docs/commands.md`, section 3). The events a line publishes are not delivered to any module: no state but bit
0x2 of `uiOptionFlags` changes between lines.

| Call | Arguments | Value |
| --- | --- | --- |
| `match` | game type, local player, viewing player, code | `null`: the game type (1, 2 or 3), `localPlayer`, `playerIndex`, and bit 0x2 of `uiOptionFlags` (1 set, 0 clear) |
| `slot` | slot, active, type, side, index | `null`: the player of that slot (0 to 9): whether its record is active (1) or not (0), its type, the side of its player info (0 Arm, 1 Core) and its index |
| `line` | text | `{"events": [[kind, payload], ...], "code": bit}`: the events the line publishes, in publication order, and bit 0x2 of `uiOptionFlags` after it |

The setup is the whole match: a vector gives `match` and then a `slot` for each of the ten slots before its first line,
and nothing else of a match is stated. The text is the line's bytes as lowercase hex (so that a tab in it is not a
separator): at most 79 bytes, each from 0x01 to 0x7f. An event is written as a line of `commands.txt` holds it
(`docs/commands.md`, section 2): the kind's name, and the event's bytes as lowercase hex.

A line's first word is one of `give`, `meteor`, `atm`, `nometal`, `noenergy`, `sealevel`, `iwin`, `ilose`, `ai`, `kill`,
`burnall` and `now`, in any case, or the line has no word. The other commands need what the setup does not state (the
cursor, the units, the unit and feature types) or are not events (section 4.3), and the scenario mode judges them.

For example, with the code set, the local player 0 and its side 0:

```text
line	6977696e
line	{"events": [["PLAYERS_KILL_UNITS", "c10000000000000001000000"], ["PLAYERS_END_GAME", "c70000000000000001000000"]], "code": 1}
```

The first line is the call (`iwin`), the second what the core writes for it.

### runtime

The vectors are the runtime vectors (the kit's `runtime/vectors/`; its `README.md` states each entry and the record
format). Each entry is one vector, named after the entry, and each of its records is one call, its edge records first
and then its random ones:

| Call | Arguments | Value |
| --- | --- | --- |
| the entry's name | the record's inputs, as one JSON array | the record's outputs, as a JSON array |

The fields of both are in the order of the entry's `inputs` and `outputs` in `vectors.json`, which names each field
(the `README.md` entry of `apply_rotation` spells its order out), and of the types of `layouts.txt`: an `i32` is a signed integer, a `u32` an unsigned
one (a float stored as a `u32` stays its 32-bit pattern), an `f64` its 64-bit pattern as an unsigned integer, and a `str`
or `bytes<N>` its bytes as lowercase hex (a `str` without its length). The gate gives the core only the inputs and
compares what it writes with the record's outputs. A vector that differs names the entry, and its `line` is the first
record that differs, counted from 1.

A call is one line: the entry's name, a tab, and the inputs as one JSON array; the core writes the entry's name, a tab,
and the outputs as one JSON array. In each example below, the first line is the call and the second what the core
writes for it.

A record of `atoi` (`str` in, `i32` out):

```text
atoi	["202d3132"]
atoi	[-12]
```

A record of `sqrt` (`f64` in and out: 4.0 is the pattern 0x4010000000000000 and 2.0 is 0x4000000000000000, each
written as the decimal unsigned integer of its 64 bits, never as a JSON number with a fraction):

```text
sqrt	[4616189618054758400]
sqrt	[4611686018427387904]
```

A record of `load_sequence` (`u32`, `i32` in; `bytes40`, `bytes16` out: each `bytes<N>` is one JSON string of exactly
2N lowercase hex digits, the bytes in memory order, so each 4-byte integer in it is little-endian):

```text
load_sequence	[0, 3]
load_sequence	["01000000020000000000000003000000040000000500000006000000070000000800000009000000", "523c0000e84c00005f6e0000c34b0000"]
```

A record of `rand` (`u32` in; `bytes40000` out, one string of 80000 hex digits, shortened here after its first 32 bytes):

```text
rand	[0]
rand	["26000000271e0000f65200008509000097220000152e0000ad2000001d7e0000..."]
```

A `str` output is written as its bytes in hex too, without its length, as a `str` input is (`itoa` of `[-12, 10]`
writes `["2d3132"]`).

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

## The freeze

The human froze the core's five headers at their `interfaces-v35` majors until a clean-room core passes this gate
(#1042, 2026-11-25): `services.h` 4, `rand_stream.h` 1, `state_writer.h` 1, `sim_events.h` 1 and `files.h` 3. Minors
(additions) still land. What the headers mean is frozen too (how `Emit` picks an outbox, sim event order, the order of
random draws, what `WriteState` hashes), and that shows in the stored vectors, so they are frozen as well.

`data/frozen_interfaces.csv` holds one row per frozen header (`header,major,decision`), and CI enforces it:

- `tools/abidiff.py` fails a header whose major is not its row's, even when the bump would otherwise pass.
- `tools/coregate.py --check-frozen` fails a pull request that changes or removes a vector of
  `reference/core/<group>.json` (by name) or a file under `reference/core/<group>/`, compared with the merge base with
  `origin/moddable`. Added vectors, files and groups pass.

A sanctioned change edits the CSV in the same pull request, and the maintainer checks the citation in review:

- **A major bump** moves the header's row to the new major and sets `decision` to the human's decision (an issue or
  comment link).
- **A re-record** adds or edits the row `reference/core/<group>.json` (`major` empty) with a decision. A change to what
  the group judges needs the human; a re-record that only changes a vector's inputs may cite the maintainer when the
  pull request shows the judged value is unchanged. Only a row the pull request adds or edits lets the group change, so
  a row left from an earlier one unfreezes nothing.

Lifting the freeze removes the header rows.

## Known limits

- **The order of the tick** (`docs/schedule.md`) is judged by no group: it needs the modules to run. The scenario mode
  judges it, once enough modules exist.
- **Translating a scripted line into events** is judged only by the scenario mode: a `spawn` or `order` line can't
  become events without the map (its ground height and footprint snapping) and the orders (`GetOrderType`). The
  `commands` group judges what the core itself does with them: the order of delivery, the tick each command precedes,
  and that only outermost publications are logged and counted. The `console` group judges the console lines that need
  nothing but the match setup it states; the others wait for the scenario mode too.
- **The limit of 20 words** of a console line changes nothing any command of the `console` group reads (none reads
  past word 5, and none compares the count of words with a number above 6), so no vector can tell it apart.
- **The radar arena** of `docs/sim-events.md` is judged by no group: its snapshots are written by the logic of
  los_radar, so only the scenario mode reaches it, once los_radar is a module. A core may leave it out until then.
- **A link into a path order** in the hash group: the rule that hashes a `link:` value of 2^31 or more is stated
  (`docs/trace-format.md`), but no dump of the scenarios searched (each scenario at two points, and the order
  scenarios at ten to twenty ticks) holds one.
- **The `node:` pointers** of the pathfinder are in the dumps but in no row (sections 16 and 17 are dumped only), so
  they change no row and no vector judges them: a core need not read them.
- **The stack-depth limit:** the `stack` class is judged by `trap_stack`, which recurses without end, so any limit
  passes it. How deep a module may call before the engine's stack overflows (the host's, not the module's
  `stack_size`, which nothing guards, `docs/module-abi.md` section 2.2) is not judged.
- **The services a module imports** (`docs/module-abi.md`, section 9.3) are judged by the `host` group only as far as
  the recordings call them. Of the import refusals of section 13, the `host` group judges every one but two:
  `returned a buffer other than the one it was given` (`BuildDataPath`) and `returned a weapon outside the sim block`
  (`FindWeaponByName`) are refusals of the result of the engine's own service, and a replay answers each import from
  its recording, which gives the result the engine gave, always the buffer and a weapon of the block. A core checks
  them in its own services, and only the scenario mode reaches them. The glue errors of a slot (`slot <name>: ...`)
  are judged by no group.
- **Hidden state** (section 12) is checked by the module gate on each module. No group of the core gate runs a module
  over ticks with its statics hashed (a replay doesn't check them: `import_stale_handle` keeps a static between calls),
  so a core's own check, and its error text, are judged only by the scenario mode.
- **`memory` at 65536 pages** is not judged: `manifest_pages_max` accepts the manifest, but the in-repo engine is a
  32-bit program that can't create 4 GiB of memory, so no vector calls `memory` on it. A core must not need 4 GiB of
  RAM for it (a reserved, untouched memory is enough), since a grader may have less.
- **Numbers above 2147483647** in a manifest are refused as invalid JSON (section 4.1), so no vector holds a larger
  `fuel_budget` or `size_budget`.
- **Runtime results at large sizes:** a `sprintf` width or precision above 4096 and a `shuffle` of more than 2^24
  elements are in no record (`tests/runtime/vectors/README.md`, "Left out"), and a core may refuse them.
