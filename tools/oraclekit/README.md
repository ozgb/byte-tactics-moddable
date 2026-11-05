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
| `layout.json` | the state block layout (below) | `docs/module-abi.md`, sections 7 to 9 |

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
