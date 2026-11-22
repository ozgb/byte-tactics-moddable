# The scripts interface

Scripts sit behind `interfaces/scripts.h` (version 2.1), the first of two simulation subsystems cut out of units and
orders in #993 (the other is movement, `docs/movement-interface.md`). Scripts are a unit's COB scripts as the
simulation sees them: the interpreter and its threads, the piece state a script poses, and the piece queries (where a
piece is, what angles it has, where a weapon aims from). 3.1c's code is its default implementation. This page is the
inventory the interface was cut from:
- the slots, the events, who calls and publishes each, and whether it runs inside the tick;
- what moved, and what stayed where it was;
- the rules of `data/subsystems.csv` for the new directory;
- the routes into and out of scripts, each with its reason;
- the hashed state, and the part of it only scripts reads.

## 1. The model

Scripts are on the scheduler since #986, with units. They have an inbox (`ScriptsInbox`, `src/seams/scripts_world.cpp`)
that passes each event to `Apply` (`ScriptsApply`, `src/scripts/scripts_apply.cpp`) and records it with its outcome in
the trace's dump (section 6). They have four events (section 2) and no pass of their own: `RunScripts` is a step of the
scheduler's unit loop (`SchedUnitsPhase`, `src/game/scheduler.cpp`), between `UpdateUnitWeapons` and `UpdateUnitBody`,
with scripts' inbox delivered before it and units' before and after it. No route into scripts is queued (section 4),
so scripts' inbox is empty in the tick. `SCHED_SCRIPTS = 13` is the module's id and `SCHED_MODULE_COUNT` is 15. The
synchronous slots, which `src/seams/native.cpp` installs directly, answer at the statement that called them, as in 1.0;
with nothing pending in scripts' inbox, they deliver nothing first. They read units' record as it is when they are
called; every route into units that writes what a script reads is synchronous (`docs/units-interface.md`, section 4).

There is no WASM module (the maintainer, 2026-11-19, on #993, option 1). The seam is over native code, like the other
simulation subsystems: the fillers are in `src/seams/native.cpp` (`<Slot>Native`), and `g_seams.scripts` is set there.

Versions: scripts 2.1 (`WriteState` is new, #1024), after 2.0 (`StartScript`, `StartScriptWithArgs` and `InitObjectState` left the seam and `Apply` is new),
units 6.0 (`docs/units-interface.md`) and services 2.2 (`HarnessUnitsEvent` and `HarnessScriptsEvent`). Before #986
scripts were 1.0: units went to 2.0 when the 16 slots that moved here left `UnitsSeam`, and orders to 4.0 for
movement's slots (`docs/movement-interface.md`).

What scripts call out to. A thread runs a unit's COB code, and some of its operations reach other subsystems. They
are scripts' imports, all through seams or services (`uv run tools/coupling.py --summary scripts`):
- **units**, through units' events: UNITS_ATTACH_TO_PIECE (the ATTACH and DROP operations) and UNITS_SCRIPT_VALUE (the
  unit values that set the unit's flags), published from `SCHED_SCRIPTS`; **map**: MAP_SET_YARD (the YARD_OPEN value,
  published as units does) and, through `MapSeam`, `GetGroundHeight` (a unit value; units' until map 5.0);
- **weapons**, through `WeaponsSeam`: `WeaponAimCobComplete` (a thread that ends or returns to a weapon's aim callback,
  `cob.cpp`) and `CobScriptExplodePiece` (the EXPLODE operation);
- **los_radar**, through `LosRadarSeam`: `IsUnitVisibleToPlayer` (the effects that only the local player sees);
- **map**, through the service `ScheduleMapCanPlaceUnitFootprint` (a script's `DropUnit`);
- **game and presentation:** the effect records (`RecordWakeParticles`, `RecordThrustParticles`, `RecordWhiteSmoke`,
  `RecordBlackSmoke`, `RecordBubbles`), `ReleaseUnitPictures` (freeing a model's cached pictures) and the allocator
  (`SimAllocTagged`, `SimRealloc`).

Scripts draw from the game logic random stream: the RANDOM operation of the interpreter (`cob.cpp`) calls `RandomInt`.
The draw is made inside a script's thread, so its place in the order of draws is the order in which `RunScripts` runs
the unit's threads, which is 3.1c's.

Scripts' effects on units and map are events now, and every other module's start of a script is the event START
(section 2); section 7 lists what is left.

## 2. The slots

Of the slots of 1.0, `StartScript`, `StartScriptWithArgs` and `InitObjectState` are gone: every caller publishes the
event START (for the first two) or INIT_STATE (for the third), which scripts' `Apply` consumes. The other 22 are
synchronous slots, and `WriteState` (2.1) is the 23rd, the engine's report of state. No subsystem calls into `src/scripts/` except through `g_seams.scripts`. Every slot has a harness
replay hook (`RecScripts<Slot>`, `src/harness/replay_hooks.inc`), which the callers below do not list again.

"In the tick" is yes when the callers run under a phase (`UpdateAllUnits`, `RunOrders`, `UpdateProjectiles`, phase 0's
`HandleNetPackets`) and no for load, save and drawing; both when the slot has callers of each kind. A caller named
by its function is in the file given.

The slots that run a script or read its pose and answer in the caller's statement are the mixed ones: `QueryScript`,
`GetNanoPiecePosition`, `GetAimFromPosition` and `GetSweetSpot` (and `FindScript`, the piece getters and
`ObjectStateSize`, which only read). They stay synchronous slots: a query's script runs at once, may draw from the
random stream and write the unit, and the caller uses the result in the same statement, so it cannot wait for a
delivery.

| Slot | What it does | Callers | In the tick | From |
|---|---|---|---|---|
| `StartScriptWithArgsByIndex` | starts a script by the index of its name | network (`net_game.cpp`: `HandleNetPackets`, a script call from a packet); orders (`order_queue.cpp`: `StartBuildingScript`, `StopBuildingScript`, `Order::~Order`) | yes | `UnitsSeam::StartScriptWithArgsByIndex` |
| `QueryScript` | runs a query script and returns its result | orders (`unit_orders.cpp`: `BuildingBuildOrder`; `vtol_orders.cpp`: `FindLandingPad`, `VtolPickupOrder`); units (`units.cpp`: `KillUnit`); weapons (`weapon_pieces.cpp`: `QueryWeaponPiece`) | yes | `UnitsSeam::QueryScript` |
| `FindScript` | the index of a named script | network (`net_game.cpp`: `SendScriptCallByName`, `SendScriptCallNoArgsByName`, which orders reach through `g_seams.network`); orders (`order_queue.cpp`) | yes | `UnitsSeam::FindScript` |
| `GetPieceOffset` | offset of a piece from the unit | orders (`vtol_orders.cpp`: `VtolPickupOrder`); weapons (`weapon_pieces.cpp`: `GetWeaponPiecePosition`) | yes | `UnitsSeam::GetPieceOffset` |
| `GetPiecePosition` | world position of a piece | movement (`motion.cpp`: `UnitMotion::UpdatePosition`; `order_targets.cpp`: `PathOrder::FillWorldPos`); orders (`unit_orders.cpp`: `BuildingBuildOrder`); weapons (`script_explode.cpp`: `UnitScript::ExplodePiece`) | yes | `UnitsSeam::GetPiecePosition` |
| `GetPieceAngles` | angles of a piece | movement (`motion.cpp`: `UnitMotion::UpdatePosition`, a unit carried on a piece) | yes | `UnitsSeam::GetPieceAngles` |
| `GetNanoPiecePosition` | world position of the nano piece | orders (`unit_orders.cpp`: ten build, repair, reclaim, capture and resurrect orders; `vtol_orders.cpp`: five VTOL ones) | yes | `UnitsSeam::GetNanoPiecePosition` |
| `GetAimFromPosition` | the position a weapon aims from | weapons (`weapons.cpp`: `CalcAimAngles`, `FireTurretWeapon`, `InitUnitWeaponSlots`, `UpdateUnitWeapons`) | yes | `UnitsSeam::GetAimFromPosition` |
| `GetSweetSpot` | the point weapons aim at | targeting (`unit_targets.cpp`: `GetWeaponTargetPos`) | yes | `UnitsSeam::GetSweetSpot` |
| `PoseUnitModel` | poses a unit's model from its angles and animation | weapons (`explosions.cpp`: `StartExplodePiece`) | yes | `UnitsSeam::PoseUnitModel` |
| `CreateObjectState` | builds the piece state of a model (a feature, or a unit with no script) | units (`units.cpp`: `InitUnitScript`) | both | `UnitsSeam::CreateObjectState` |
| `FreeObjectState` | frees a piece state and its cached pictures | units (`units.cpp`: `ApplyUnitDeath`); features' FREE_STATE reaches the same code through scripts' `Apply`, not this slot | yes | `UnitsSeam::FreeObjectState` |
| `UpdateObjectState` | poses a model from its current angles when they changed | presentation (`model_render.cpp`: `DrawUnit`) | no | `UnitsSeam::UpdateObjectState` |
| `ObjectStateSize` | bytes of a model's piece state, a query | features (`features.cpp`: `PlaceFeature`) | both | `UnitsSeam::ObjectStateSize` |
| `NewUnitScript` | allocates a unit's script and stores it in the unit | units (`units.cpp`: `InitUnitScript`) | both | new: was a direct call of `NewUnitScript` from `units.cpp` |
| `SetCob` | gives a script its unit type's compiled data | units (`units.cpp`: `InitUnitScript`) | both | new: was `CobScript::SetCob` from `units.cpp` |
| `CreatePlayerObjectState` | builds a scripted unit's piece state | units (`units.cpp`: `InitUnitScript`) | both | new: was a direct call of `CreatePlayerObjectState` from `units.cpp` |
| `SetObjectState` | gives a unit script its piece state | units (`units.cpp`: `InitUnitScript`) | both | new: was `UnitScript::SetObjectState` from `units.cpp` |
| `RunScripts` | runs a script's threads for the elapsed ticks | the scheduler (`SchedUnitsPhase`) | yes | new: was `CobScript::RunScripts` from `unit_position.cpp`, called by `UpdateUnitBody` |
| `SaveScriptState` | writes a script's threads to a saved game | units (`unit_save.cpp`: `SaveUnits`) | no | new: was `CobScript::SaveScriptState` from `unit_save.cpp` |
| `LoadScriptState` | reads a script's threads from a saved game | units (`unit_save.cpp`: `LoadUnit`) | no | new: was `CobScript::LoadScriptState` from `unit_save.cpp` |
| `DeleteScript` | destroys a unit script (the virtual deleting destructor) | units (`units.cpp`: `ApplyUnitDeath`) | yes | new: was a `delete` of the `CobScript` from `units.cpp` |
| `WriteState` | writes record 55 `scriptsState`: the script state that left the hash (section 6); it reads only scripts' own state and writes through the `StateWriter` | the engine (the trace harness, at a dump tick) | yes | new in 2.1 |

Notes on the table:
- **`InitUnitScript` stays in units** (`units.cpp`, `FUNCTION: 0x485d40`): it is the unit's creation, and it makes
  five calls through the seam where it made direct ones, and publishes START for the script's Create. Creation runs in the tick (a unit built or spawned,
  or made from a packet) and when a game starts, so the slots only it calls are marked both.
- **`ObjectStateSize` and the two feature slots.** A feature's piece state is made by `PlaceFeature` (in the tick for
  a feature placed by an event, outside it at load and by the console) and freed and filled by the effects
  FREE_STATE and INIT_STATE (`docs/features-interface.md`), which scripts' handlers (`ScriptsOnFeaturesFreeState` and
  `ScriptsOnFeaturesInitState`) turn into scripts' events of the same names. `FreeObjectState` stays a slot for a unit's
  death; INIT_STATE has no slot.
- **`RunScripts` is the scheduler's step,** between `UpdateUnitWeapons` and `UpdateUnitBody` for each unit with a
  script, in 3.1c's order, so scripts' draws of `RandomInt` sit where they did.
- **`UpdateObjectState` is a drawing call.** `DrawUnit` calls it in the draw pass, which poses the model for
  display. Presentation does not read a unit's pose through any other slot. `PoseUnitModel` is the simulation's
  version for a unit whose pose the simulation reads (a piece blown off, `StartExplodePiece`).
- **No slot is a callback.** The imports of section 1 are calls from scripts out, not slots of this header.
- **Slots whose callers all sit in one subsystem** (`GetSweetSpot` in units, `GetAimFromPosition` in weapons,
  the save and load pair in units) are still slots: the interface is the set of what other code calls, whoever
  that is today.

**The events.** Four kinds, each a plain-data payload (`interfaces/scripts.h`, `src/game/scripts_events.h` builds
and publishes them). `Apply` rejects an event smaller than its payload, naming no unit (but FREE_STATE and INIT_STATE,
which name a piece state) or of an unknown kind: it changes nothing and has outcome 0.

| Kind | Payload | Fields | Outcome | Published by |
|---|---|---|---|---|
| START (416) | `ScriptsStart` | the unit; the form (by name, by name with arguments, by index with arguments); the index; the name (32 bytes); the callback, as its block offset; the update argument; the argument count; four arguments | 1, or 0 when the unit has no script | units (`SCHED_UNITS`: its own starts, section 3 of `docs/units-interface.md`); weapons (START_SCRIPT, `ScriptsOnWeaponsStartScript`); orders (`ScriptsOnOrdersStartScript`); movement (`ScriptsOnMovementStartScript`); economy (EXTRACTOR, `ScriptsOnEconomyExtractor`); targeting (CLEARED, `ScriptsOnTargetingCleared`) |
| DETACH_PIECE (417) | `ScriptsDetachPiece` | the unit; the piece; whether the model is posed first | 1 | weapons (`ScriptsOnWeaponsDetachPiece`) |
| FREE_STATE (418) | `ScriptsState` | the piece state, as its block offset | 1 | features (`ScriptsOnFeaturesFreeState`) |
| INIT_STATE (419) | `ScriptsState` | the piece state, as its block offset; the feature type | 1 | features (`ScriptsOnFeaturesInitState`) |

The adapters onto scripts' code are handlers of scripts' inbox in `src/seams/scripts_world.cpp`: each translates the
foreign payload into one of these events and calls `ScriptsInbox`. The files `src/scripts/{weapons,orders,movement}_effects.cpp`
and the `src/units/*_effects.cpp` that were the adapters are gone.

## 3. What moved

**Files from `src/units/` to `src/scripts/`** (renamed, content kept except where said):

| From | To |
|---|---|
| `src/units/cob.cpp` | `src/scripts/cob.cpp` (5 lines fewer: its calls out now go through seams) |
| `src/units/cob_script.h` | `src/scripts/cob_script.h` |
| `src/units/cob_types.h` | `src/scripts/cob_types.h` |
| `src/units/object_state.cpp` | `src/scripts/object_state.cpp` |
| `src/units/object_state.h` | `src/scripts/object_state.h` (6 lines added: the declarations the slots call) |
| `src/units/unit_script.cpp` | `src/scripts/unit_script.cpp` (without `ExplodePiece`, see below) |
| `src/units/unit_script.h` | `src/scripts/unit_script.h` |
| `src/units/unit_script_alloc.cpp` | `src/scripts/unit_script_alloc.cpp` |

**New in `src/scripts/`:** `object_view.h` (the `Object` view of a unit's position and script that the piece queries
read) and `piece_queries.cpp` and `piece_queries.h`.

**`src/units/unit_scripts.cpp` and its header are gone.** The file held what the piece queries, orders, units and
weapons shared, and was split by owner:

| Function | Now in |
|---|---|
| `GetPieceOffset`, `GetPiecePosition`, `GetPieceCenter`, `GetPieceAngles`, `GetAimFromPosition`, `PoseUnitModel`, `GetSweetSpot`, `GetNanoPiecePosition` | `src/scripts/piece_queries.cpp` (scripts) |
| `QueryWeaponPiece`, `GetWeaponPiecePosition` | `src/weapons/weapon_pieces.cpp` (weapons) |
| `OrderModeTakesTarget`, `GetOrderCursor` | `src/orders/order_cursor.cpp`, `src/orders/order_dispatch.cpp` (orders) |
| `SfxOccupyTracker::UpdateSfxOccupy` | `src/units/sfx_occupy.cpp` (units) |

**Weapons' pieces of the script files** moved to `src/weapons/`: `UnitScript::ExplodePiece` and
`CobScript::ExplodePiece` (`script_explode.cpp`, taken out of `unit_script.cpp`), `WeaponAimCobCb::OnAimCobReturn`
and `AimCobStub` (`weapon_aim_cob_cb.cpp`, taken out of `unit_script_calls.cpp`), and the header
`weapon_aim_cob_cb.h` (renamed from `src/units/`). The two `ExplodePiece` methods stay methods of the scripts'
classes (the interpreter's EXPLODE operation calls the virtual), but their code is weapons': it breaks a model's
piece into debris, so scripts reach it through `WeaponsSeam::CobScriptExplodePiece`.

**What stayed where it was:**
- **`src/units/unit_script_calls.cpp`**, which the issue names, is not a scripts file. It holds
  `OctantBetween`, `StepTowards`, `StepPointTowards`, `ChebyshevDistance`, `ClampPointToSegment` and `Sub`, plane
  maths that no script code calls (its two script-named functions left it for weapons, above). It stays with units.
- **`InitUnitScript`** and the rest of a unit's creation and death (`units.cpp`): the unit's lifecycle is units'.
- **Loading and freeing the compiled script data** (`LoadCobScript`, `FreeCobScript`, `GetCobChecksum` in
  `src/util/int_map.cpp`; `ComputeUnitScriptChecksum` in `src/game/data_files.cpp`): file and platform code, outside
  the tick.
- **`WaitIfCobBusy`** (`src/orders/order_queue.cpp`) reads the unit's `cobStateFlags`, a field of the unit, so it is
  orders'.

**The callers moved to the seam.** Every direct call of `CobScript` or `UnitScript` from outside `src/scripts/`, and
every `g_seams.units->` call of the 16 slots, was made `g_seams.scripts->`: the files were in `src/orders/`
(`order_list.cpp`, `order_queue.cpp`, `order_targets.cpp`, `unit_orders.cpp`, `vtol_orders.cpp`), `src/units/`
(`unit.cpp`, `unit_motion.cpp`, `unit_position.cpp`, `unit_save.cpp`, `unit_targets.cpp`, `units.cpp`, and the
`*_effects.cpp` adapters that #986 later replaced), `src/weapons/`, `src/network/net_game.cpp`,
`src/map/features.cpp` and `src/graphics/model_render.cpp`.

## 4. The routes

Every route into and out of scripts, with its mode. Synchronous means the handler runs inside `SchedPublish`, at the
statement of the publisher. The first rows are new in #986 and rest on the gate; the table of units' routes with the
same reasons is in `docs/units-interface.md`, section 4.

| Route | Mode | Why |
|---|---|---|
| units START, to scripts (`ScriptsInbox`) | synchronous | Queued, scenario `orders-air-fights` diverged at tick 497, world phase, `units[6].script.activeCount` 4 against 3: a started script runs its first slice at once. |
| scripts SCRIPT_VALUE, to units (UNITS_SCRIPT_VALUE) | synchronous | Queued, scenario `orders-tier2-core` diverged at tick 43, units phase, `units[2].activateFlags` 0x02 against 0x00: the thread reads the value back. |
| scripts SET_YARD, to map (MAP_SET_YARD, the YARD_OPEN value) | synchronous | Queued, scenario `orders-air` diverged at tick 1024, units phase, `units[2].bit0` 0x04 against 0x0c. |
| scripts ATTACH_TO_PIECE, to units (UNITS_ATTACH_TO_PIECE) | synchronous | #952's rule: queued it stayed equal, but the handler reads the passenger's `cargo` and the carrier's `carrier`, which the previous attach or drop of the same script run writes, and sends the attach to the network in the publisher's order. Orders' ATTACH_TO_PIECE is synchronous for the same handler. |
| weapons START_SCRIPT, DETACH_PIECE, to scripts | synchronous | As before (`docs/weapons-interface.md`, sections 4 and 8). |
| orders START_SCRIPT, to scripts | synchronous | As before (`docs/orders-interface.md`, section 4). |
| movement START_SCRIPT, to scripts | synchronous | As before (`docs/movement-interface.md`, section 4). |
| economy EXTRACTOR, to scripts | synchronous | #952's rule: the start takes a thread channel, which a later start on the unit in the same phase could take first while it waited (`docs/units-interface.md`, section 4). It was queued before. |
| features FREE_STATE, INIT_STATE, to scripts | synchronous | As before (`docs/features-interface.md`, sections 4 and 8). |
| targeting CLEARED, to scripts | synchronous | As before (`docs/targeting-interface.md`, section 8). |
| scripts AIM_DONE, EXPLODE_PIECE, to weapons | synchronous | As before (`docs/weapons-interface.md`, section 4). |

All of them are in `src/game/routes.cpp`. Of the routes to scripts' handlers that were units', FREE_STATE,
INIT_STATE and CLEARED keep their modes, and EXTRACTOR becomes synchronous.

## 5. The rules of `data/subsystems.csv`

- **One `file:` rule for the new directory.** `tools/coupling.py` lists `("src/scripts/", "scripts")` first in
  `FILES`, and `"src/scripts/": "scripts"` in `FOLDER_DEFAULT`, and adds `scripts` to `SIM_SUBSYSTEMS`. Every function
  of the 6 `.cpp` and 1 `.h` files above gets the rule `file:src/scripts/` and no override. The file rule is first
  in `FILES`, so it wins over `src/units/` for the moved functions, and no `name:` rule is needed to pull them out of
  units.
- **No `name:` rule and no override for what stays elsewhere.** Each function that is scripts' by name but not by
  file is placed by the file rule of the directory it is in, and that is the right answer:
  - `UnitScript::ExplodePiece` and `CobScript::ExplodePiece` (in `src/weapons/`): weapons, `file:src/weapons/`;
  - `QueryWeaponPiece`, `GetWeaponPiecePosition`, `WeaponAimCobCb::OnAimCobReturn`, `AimCobStub`: weapons;
  - `InitUnitScript`, `SfxOccupyTracker::UpdateSfxOccupy`: units, `default:src/units/`;
  - `OrderModeTakesTarget`, `GetOrderCursor`, `WaitIfCobBusy`: orders, `file:src/orders/`. The old rows were
    `name:Target` with the override `orders` and `name:Order`, both from `unit_scripts.cpp` in units' folder; the
    move makes the override unnecessary, and it is gone;
  - `LoadCobScript`, `FreeCobScript`, `GetCobChecksum`, `ComputeUnitScriptChecksum`: platform.
- **The moved rows** keep their function names and change the file column. The regenerated `data/subsystems.csv`,
  `data/coupling.csv`, `data/rwsets.csv`, `data/crt_calls.csv` and `data/state_classes.csv` carry the move.

## 6. State and the trace

Scripts keep no private region. Since format 8 of the trace (#1024) the script state that only scripts reads is not
hashed, and scripts' `WriteState` (a slot since 2.1) dumps it in record 55, `scriptsState`. The state stays where it
was: the script object of each unit, its statics and its piece records are scripts' own allocations, and
`data/state_classes.csv` names `src/scripts/` as their owner (`NewUnitScript: new UnitScript`, `CreateObjectState` and
`CreatePlayerObjectState: GameAllocIgnoreTag ModelState`). Scripts' events are dumped in section 51 of the trace
(units' in section 50), never hashed (`docs/trace-format.md`, sections 8.34 and 8.35).

| Scripts' state | Where | Read by | Hashed |
|---|---|---|---|
| The threads: 8 `Channel` records of a `CobScript` (state, program counter, stack pointer, the stack of 32 words, signal, delay, wait, piece and axis, the callback pointer) | the unit's script object | scripts | no: record 55 `channels[c]` |
| The script's statics (`CobScript::statics`, as many words as the compiled data names) | a block allocation | scripts | no: record 55 `statics` |
| The piece records (`Piece`: move, turn and spin targets, speeds, acceleration, active bits) | a block allocation | scripts | no: record 55 `pieces[p]` |
| The script's own words (`CobScript::scale`, `table`, `checksum`, `changed`, `activeCount`) | the script object | scripts | no: record 55, the fields of the script state |
| The piece transforms (`PieceState`: translation, rotation, visible, cached, shaded) | the model state of the unit (section 23, `script.transforms[q]`) | scripts (the piece queries); weapons (reads them for the pieces it blows off and for debris); presentation (drawing) | yes |
| The model state's derived data (`ModelState::pos`, `root`, `animDirty`, `cacheDrawCount`; each piece's cached `points`) | the model state | scripts, presentation | not hashed: the derived position, the caches and the draw frame are view |
| The model state's `unit` back-pointer and `drawFrame` | the model state, a pointer marker only | scripts, presentation; units writes them (`InitUnitScript`) | no: units writes them |
| A unit's `script` and `state` pointers (`Unit::script`, `Unit::state`) | the unit record, hashed | units, and every caller of a slot, which passes the script as its first argument | yes |
| The unit's bit0, bit1 and bit3 (`UnitScript::SetUnitValue`, `GetUnitValue`) | the unit record, hashed with the unit | scripts write them directly; the word they sit in is read as `Unit::flags` by units, orders and map | yes |
| The feature's model state (`PlaceFeature`, `FREE_STATE`, `INIT_STATE`) | the feature, hashed as a pointer marker | features, presentation; scripts builds and fills it | yes |

**What is hashed, and who reads it.** Section 23 (`docs/trace-format.md`, section 8.12) hashes, for each live unit, a
presence byte and, for a unit with a script, only the piece transforms. Weapons reads them directly, for the pieces it
blows off and the debris it makes of them, so they stay. The unit's `script`, `state` and bits stay in the unit's
section with the unit.

**What left the hash.** The 8 channels (state, program counter, stack pointer, the live stack words, signal, callback
and the delay, wait, piece and axis of a channel that has them), the statics, the piece records and the script's own
words (`scale`, the table pointer, `checksum`, `changed`, `activeCount`). Only scripts reads them.

**Record 55, `scriptsState`** (dumped, never hashed; `docs/trace-format.md`, section 8.39). For each live unit with a
script, one record `scriptsState.units[i]` holds the fields of the script state, the 8 channels as
`channels[c].<field>` (and `.stack`), `statics.count` and `statics`, and the piece records as `pieces[p].<field>`.
The reference build writes the same record itself. `WriteState` reads only scripts' state and writes through the
`StateWriter`.

**Where a wrong value shows.** The channels, statics and pieces decide which script runs and what it does. With a
wrong program counter, TAMPER(scripts:pc). A wrong statics word, signal or piece target diverges through what the
script then sets: the piece transforms (hashed), the unit's bits, and the events it publishes.

This change adds scripts to the owners of `privatestate.py`. With them, `--check` fails when another subsystem writes
any of the items scripts alone writes: the script objects (`CobScript`, `UnitScript`, the channels), the statics and
the piece records, and the unit's `script` pointer, which units also writes when it creates and frees the script
(`InitUnitScript`, `ApplyUnitDeath`). The model state is not claimed: presentation (`cacheDrawCount`, `shadow`) and
units (`bitmap`) write parts of it. By the access log (`uv run tools/coupling.py --summary
scripts`), the sim items scripts writes are those and the unit's `script` pointer and bits above; presentation writes
`ModelState::cacheDrawCount` and `ObjectState::unit` (drawing), and units writes the model state's `unit` pointer.
Scripts read, and write nothing of, the unit's position, heading, angles, flags, health, `cargo`, `cargoNext`,
`carrier`, `def` and `state`.

## 7. What is left

The following is left, and no issue is opened for it here:
- **#986, done.** Units and scripts are on the scheduler. Every start of a script is the event START and the filling
  of a feature's piece state is INIT_STATE (section 2); scripts' effects on units and map are events published from
  `SCHED_SCRIPTS` (section 4); `RunScripts` is a step of the unit loop. The queries, the life cycle of a unit's script
  and the save and load pair stay synchronous slots, as section 2 says.
- **#984 (weapons), done (`docs/weapons-interface.md`).** Weapons publishes its script starts as the effect
  START_SCRIPT and a piece's detaching as DETACH_PIECE, both to scripts, whose handlers turn them into START and
  DETACH_PIECE; scripts publishes a weapon's aim callback (`WeaponAimCobComplete`) as weapons' event AIM_DONE and the
  explode opcode (`CobScriptExplodePiece`) as EXPLODE_PIECE. `QueryScript` and `GetAimFromPosition` stay synchronous
  slots of scripts (mixed: they run a unit's script, and weapons uses the answer in the same statement).
- **The format bump for weapons, orders and units.** Moving the state section 6 lists out of the hash, as #979 does
  for los_radar, players, ai and map.
- **The piece queries stay scripts'.** They read the piece state, and their callers (weapons, orders, movement) reach
  them through this seam like any other slot.
