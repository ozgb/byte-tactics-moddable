# The oracle data kit

This is everything Milestone 2 (#180) may read of the reference. `MANIFEST.json` lists every file with its
sha256 and size, the commit it was cut from, and the kit's version: the pair of tags (`interfaces-v*`,
`reference-traces-v*`) at that commit. A kit with `"version": null` is a trial and is not to be built against.

## What each part is

| Path | What it is | Specified by |
| --- | --- | --- |
| `docs/trace-format.md`, `data/trace_schema.json` | the trace format and its schema | `docs/trace-format.md` |
| `tools/tracerehash.py` | recomputes every trace hash from a test-mode dump, standard library only | `docs/trace-format.md` |
| `reference/traces/` | the stored traces, one per scenario | `docs/reference-traces.md` |
| `reference/scenarios/` | the scenarios the traces come from | `docs/scenario-format.md` |
| `reference/saves/` | the saved games some scenarios load | `docs/reference-traces.md` |
| `reference/sounds/` | the stored sound logs | `docs/sound-log.md` |
| `reference/frames/` | the stored frames | `docs/frame-dump.md` |
| `reference/ui/` | the stored UI logs | `docs/ui-log.md` |
| `interfaces/` | the subsystem interface headers (the seams) | `docs/module-abi.md` |
| `docs/scenario-format.md` | the scenario format, the test-mode run and its outputs | itself |
| `docs/module-abi.md` | the module ABI | itself |
| `wasm/modules.json` | the module manifest of the game: the loader's numbers, the signatures and each module's entry | `docs/module-abi.md`, section 4 |
| `tools/wasm_fuel.py` | the fuel pass: meters a finished module by the rule of ABI 11, standard library and `wasm-tools` only | `docs/module-abi.md`, section 11 |
| `tests/replay/` | the stored per-module replay recordings | `docs/replay-format.md` |
| `data/replay_schema.json` | the annotations and the fields, objects and slots the recordings name | `docs/replay-format.md` |
| `docs/sim-events.md` | the sim event queue: the record, the queue, each kind of event and when the logic records it, with a `Terms` section at its end (below) | itself |
| `interfaces/sim_events.csv` | the kinds of sim event as a table (below) | `docs/sim-events.md` |
| `layout.json` | the state block layout (below) | `docs/module-abi.md`, sections 7 to 9 |
| `coupling/` | what the subsystems read of each other's state, and what presentation reads (below) | itself |

## Presentation is not a module

The gate runner judges modules, and presentation (sound, frames, the UI) is not one. Milestone 2's build
writes its own sound log, UI log and frames in its test mode and compares them with the kit's stored ones.
The commands the reference used to write them are in `reference/README.md`, which stays in the source
repository; the formats are in `docs/sound-log.md`, `docs/frame-dump.md` and `docs/ui-log.md`.

## The manifest and the fuel pass

`wasm/modules.json` is the game's manifest as the loader reads it, with one change: the `cxx` and `c` fields of each
module entry (the module's source files) are removed, and the export refuses a manifest that still names a source.
ABI 9.1 requires a module's imports and exports to equal its entry exactly; the entry is the list to build against.

`tools/wasm_fuel.py` is the reference for the rule of ABI 11. `uv run tools/wasm_fuel.py module.wasm -o metered.wasm`
(or `python3`) applies it to a linked module and needs `wasm-tools` at the version `tools.wasm-tools` of the manifest;
`--check` tests a metered module's surface. A pass of your own is accepted when the gate accepts the module it makes.

## No allocator in the ABI

A module uses only block memory the engine allocated. The ABI has no allocator service, and adding one is a
decision for #180, not for the clean room.

## From the ABI document to layout.json

`layout.json` holds, for every struct the state block holds, its size and each field's name, offset, size
and type (a bit-field has `bit_offset` and `bit_width` in place of a size; `bases` and `vptr` appear when the
record has them). A field whose type is an unnamed union or struct (`WeaponDef::(unnamed)`) lists the bit-fields
inside it as `bits`: each is `{name, bit, width}`, with `bit` counted from the first bit of the field (`WeaponDef.flags`
has `meteor` at bit 5). It carries no cross-references to the decompilation (the export leaves out `data/layout_refs.csv`).

`docs/module-abi.md` names five records by role only. Each carries a `"role"` key in `layout.json`:

| Role | Record | Where the ABI document names it |
| --- | --- | --- |
| `game` | `Game` | section 8.2, `g_game` |
| `sim_globals` | `SimGlobals` | section 8.2, `g_simGlobals` |
| `params` | `MeteorParams` | the argument of `SetMeteorParams` and `LoadMeteorDefaults`, section 9 |
| `vec3` | `Vec3` | the position and velocity of `weapons.SpawnProjectile`, section 9.2 |
| `weapon` | `WeaponDef` | the first argument of `weapons.SpawnProjectile` and the result of `weapons.FindWeaponByName`, section 9.2 |

A pointer field stored in the block is a `Ref<T>`: a 32-bit block offset (section 7.1).

## The sim event queue

The logic tells the presentation what happened by recording events into a queue, and `docs/sim-events.md` states the queue,
the record and each kind. Three files carry it.

`layout.json` has the record `SimEvent` (32 bytes: `kind`, `flags`, `tick`, `args`) and the queue `SimEventQueue` (`head`,
`tail`, `dropped`, `reserved` and the ring `records`) next to the records of the state block.

`interfaces/sim_events.csv` has a row for each kind of `SimEventKind` in `interfaces/services.h`, sorted by value. The columns
are `kind` (the enumerator), `value`, `arguments` (how many arguments the event has, the second record's included), `records`
(0 for the empty record, 1, or 2 when the event has more than six arguments), `flags` (the bits of `SimEvent.flags` the kind
sets, or `none`), `phases` (where the call graph reaches a record of the kind, as the page's `Phases:` line states) and
`argument_list` (the arguments in order, joined by ` | `, each as `index type: meaning`, a range as `first-last`).

The page's conditions are written with the engine's own numbers: the flags of an order step, the states of an order, the
flags of a unit, the map flags, the kill types and the damage kinds. Its last section, `Terms`, defines every one of them, in
a table of the term, its meaning and the place in the kit that holds it (a field of `layout.json`, an interface name, or this
page). A term whose meaning the kit's data does not give is defined by what setting the flag does or what each state does.
The `Terms` section is the same for every export, because it is generated from one list of terms, and the export is refused
when the page uses a number the list does not define.
The kit's copy of the page sets each long numbered list of its `Tables` section one entry to a line, with the entries as they are.

## What the subsystems depend on

`coupling/` holds what the reference's 83 scenarios showed of how the subsystems depend on each other's state. A
subsystem is one of `wind_meteors`, `economy`, `features`, `ai`, `pathfinding`, `weapons`, `targeting`, `los_radar`,
`orders`, `units`, `players` and `map`. An item is a field of a record of `layout.json` (`Type::field`), a random stream
(`rng.*`), `sound`, `ui`, or a region of the block that the layout leaves unnamed. A phase is a window of a tick:
`net`, `players`, `units` and the others the trace format lists, `tail`, then `presentation`. An example is
`scenario@tick`: a scenario of `reference/scenarios/` and the tick at which the row was first seen. The files are CSV
with a header row, sorted. The edge between two subsystems is the writer of an item and its reader (`-` for no writer).

| File | Columns |
| --- | --- |
| `coupling/dependencies.csv` | `writer`, `reader`, `item`, `phase`, `class`, `runs`, `example`, `flags` |
| `coupling/phase_dependencies.csv` | `writer`, `reader`, `item`, `phase_from`, `phase_to`, `class`, `runs`, `example`, `flags` |
| `coupling/phase_boundaries.csv` | `boundary`, `needed`, `edges`, `direct`, `carried`, `mergeable`, `example` |
| `coupling/draws.csv` | `pass`, `stream`, `subsystem`, `site`, `entities`, `draws_min`, `draws_max`, `ticks`, `start_state`, `evidence`, `example` |
| `coupling/pass_reads.csv` | `pass`, `item`, `reader`, `evidence`, `runs`, `example` |
| `coupling/presentation_reads.csv` | `item`, `reader`, `pattern`, `covered_by_event`, `visibility`, `runs` |
| `coupling/dependency_checks.csv` | `writer`, `reader`, `item`, `phase`, `group`, `scenarios`, `result`, `scenarios_counted`, `defer_held`, `defer_third_stores` |

`runs` counts the scenarios that show the row in its class; `example` is one of them. `flags` is `dynamic-only` when
the runs found a dependency that no static reading of the code predicts. The classes of `dependencies.csv`, the first
that applies:

| Class | Meaning |
| --- | --- |
| `ordered` | the item is a random stream, the sound log or the UI log, or the events queued: the order of the draws or emissions is the state, whichever subsystem comes first |
| `immediate` | a run shows the reader reading the item after the writer wrote it in the same phase: the writer must run first |
| `deferrable` | the reader read the item before the writer wrote it, and no one read the writer's value before the phase ended: the write could wait until the end of the phase. `dependency_checks.csv` has the confirmation, a run of every scenario with such writes held back to the end of the phase |
| `read-only` | writer `-`: the reader reads the item and no run shows anyone writing it in that phase |
| `unobserved` | the code can pass the item between the two subsystems but no scenario does, so the runs say nothing about it; its phase is `-`, `runs` is 0 and `example` is empty |

`phase_dependencies.csv` is the same across phases, with `class` `immediate` (the reader of `phase_to` read what
`phase_from` wrote earlier in the tick: the writer's phase comes first) or `carried` (the reader read what a later
phase wrote in the tick before: the order of the phases is the state); a `phase_to` of `presentation` is a read by the
view. `phase_boundaries.csv` has a row per boundary between two windows (`1|2`, `7|tail`, `tail|presentation`):
`needed` is `yes` when an `immediate` row crosses it, `edges`, `direct` and `carried` count the rows that cross it, and
`mergeable` is `yes` when no row of either kind does; boundary `0|1` is `absent` because no scenario has a `net` window.

`draws.csv` has one row per pass, stream and draw site: `entities` is the most entities that drew in one tick,
`draws_min` and `draws_max` the least and most draws of one entity in a tick, `ticks` the ticks with a draw, and
`start_state` is `fixed` (every entity drew the same number every tick), `start-state` (the number varies but no
entity read a field that an earlier entity of the pass wrote before its last draw) or `depends-on-earlier` (with those
items in `evidence`). `site` numbers the rows of one pass, stream and subsystem. `pass_reads.csv` lists the items that
an entity of a pass read after an earlier entity of the pass wrote them: `same-record` when the write was to the same
record, `other-record` when it was to the same field of another record (weaker); a pass with none has item `-` and
evidence `none`. `runs` is the most scenarios that any one reader showed.

`presentation_reads.csv` lists the items that the view reads between ticks. `pattern` is `every-frame` (read every
tick), `ui-action` (read when the player acts), `after-event` (read after one kind of sim event, named in
`covered_by_event` as in `interfaces/services.h`) or `unknown`; `visibility` is `own` (the player's own records),
`per-player-filter` (units, features, projectiles and their kin, as line of sight shows them) or `all-players`; a `runs`
of 0 means no scenario showed the read.

Three kinds of item are named by what they are, not by the code that touches them. An `unobserved` row flagged `indirect`
is a call from the writer's subsystem into the reader's through a function pointer: its item is the slot, written
`interface.Slot` (the interface of `interfaces/` and the slot of its table, as in `economy.GetEnergyUse`), or `call` when no
interface declares it. A block of memory that the game allocates is named by its state class (`sim`, `view`, `platform`,
`mixed` or `const`: whether the simulation holds it), or `heap` when it has none. The random stream of
the game's generator is `rng`, as in the trace format. An item that `layout.json` and the rest of
the kit do not define (a table that no record of the layout holds) is named by the state class of the table or record it lies
in, or `heap`. Rows that share a key after these names merge into one, with the most runs and its example, so a row stands
for every edge of that kind between the two subsystems.
