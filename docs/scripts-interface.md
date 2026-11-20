# The scripts interface

Scripts sit behind `interfaces/scripts.h` (version 1.0), the first of two simulation subsystems cut out of units and
orders in #993 (the other is movement, `docs/movement-interface.md`). Scripts are a unit's COB scripts as the
simulation sees them: the interpreter and its threads, the piece state a script poses, and the piece queries (where a
piece is, what angles it has, where a weapon aims from). 3.1c's code is its default implementation. This page is the
inventory the interface was cut from:
- the slots, who calls each, and whether it runs inside the tick;
- what moved, and what stayed where it was;
- the rules of `data/subsystems.csv` for the new directory;
- the hashed state, and the part of it only scripts reads.

## 1. The model

Scripts are not on the scheduler. They have no inbox, no events, no pass and no `SCHED_*` run. `SCHED_SCRIPTS = 13`
exists in `interfaces/services.h` (services 1.19) as an id only, as `SCHED_MOVEMENT = 14` does for movement, so
`SCHED_MODULE_COUNT` is 15. Callers call the seam slots directly and synchronously, as they called units' slots before
the cut: a slot answers at the statement that called it, and a script's thread runs when `RunScripts` is called
(section 2).

There is no WASM module (the maintainer, 2026-11-19, on #993, option 1). The seam is over native code, like the other
simulation subsystems: the fillers are in `src/seams/native.cpp` (`<Slot>Native`), and `g_seams.scripts` is set there.

Versions: scripts 1.0. Units went to 2.0: the 16 slots that moved here left `UnitsSeam`, and the minor restarted at 0.
Orders went to 4.0 for movement's slots (`docs/movement-interface.md`). Units gained `UnitRefInit` and `UnitRefSetUnit`
for movement, not for scripts.

What scripts call out to. A thread runs a unit's COB code, and some of its operations reach other subsystems. They
are scripts' imports, all through seams or services (`uv run tools/coupling.py --summary scripts`):
- **units**, through `UnitsSeam`: `AttachUnitToPiece` (the ATTACH and DROP operations), `SetStateBits` (the unit values
  that set the unit's flags); map, through `MapSeam`: `GetGroundHeight` (a unit value; units' until map 5.0);
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

The next issues take scripts on: #986 puts them on the scheduler with units, and #984 (weapons) addresses scripts'
slots (section 6).

## 2. The slots

The first 16 slots came from `UnitsSeam` with the same signatures. The last 9 are new: each was a direct call of the
script code from units' files (or a `delete`), and each is now a slot, so no subsystem calls into `src/scripts/`
except through `g_seams.scripts`. Every slot has a harness replay hook (`RecScripts<Slot>`,
`src/harness/replay_hooks.inc`), which the callers below do not list again.

"In the tick" is yes when the callers run under a phase (`UpdateAllUnits`, `RunOrders`, `UpdateProjectiles`, phase 0's
`HandleNetPackets`) and no for load, save and drawing; both when the slot has callers of each kind. A caller named
by its function is in the file given.

| Slot | What it does | Callers | In the tick | From |
|---|---|---|---|---|
| `StartScript` | starts a named script with a callback | movement (`motion.cpp`: `UnitMotion::UpdateMoveRate`); orders (`vtol_orders.cpp`: `VtolLandIfCanOrder`, `VtolLandingOrder`, `VtolPickupOrder`, `VtolUnloadOrder`); units (`unit.cpp`: `Unit::SetStateBits`; `units.cpp`: `InitUnitScript`); weapons (`weapons.cpp`: `FireBallisticProjectile`, `FireLineOfSightProjectile`, `FireVLaunchProjectile`) | yes | `UnitsSeam::StartScript` |
| `StartScriptWithArgs` | starts a named script with up to four arguments | orders (`unit_orders.cpp`: `GroundPickupOrder`, `GroundUnloadOrder`; `vtol_orders.cpp`: `VtolPickupOrder`); units (`economy_effects.cpp`, `targeting_effects.cpp`, `sfx_occupy.cpp`, `unit_motion.cpp`: `UpdateWindGenerator`, `unit_targets.cpp`: `ApplyUnitDamage`, `units.cpp`: `ApplyUnitDeath`); weapons (`weapons.cpp`: the three `Fire*Projectile`, `RockUnit`, `InitUnitWeaponSlots`, `UpdateUnitWeapons`) | yes | `UnitsSeam::StartScriptWithArgs` |
| `StartScriptWithArgsByIndex` | starts a script by the index of its name | network (`net_game.cpp`: `HandleNetPackets`, a script call from a packet); orders (`order_queue.cpp`: `StartBuildingScript`, `StopBuildingScript`, `Order::~Order`) | yes | `UnitsSeam::StartScriptWithArgsByIndex` |
| `QueryScript` | runs a query script and returns its result | orders (`unit_orders.cpp`: `BuildingBuildOrder`; `vtol_orders.cpp`: `FindLandingPad`, `VtolPickupOrder`); units (`units.cpp`: `KillUnit`); weapons (`weapon_pieces.cpp`: `QueryWeaponPiece`) | yes | `UnitsSeam::QueryScript` |
| `FindScript` | the index of a named script | network (`net_game.cpp`: `SendScriptCallByName`, `SendScriptCallNoArgsByName`, which orders reach through `g_seams.network`); orders (`order_queue.cpp`); units (`targeting_effects.cpp`: `UnitsOnTargetingCleared`) | yes | `UnitsSeam::FindScript` |
| `GetPieceOffset` | offset of a piece from the unit | orders (`vtol_orders.cpp`: `VtolPickupOrder`); weapons (`weapon_pieces.cpp`: `GetWeaponPiecePosition`) | yes | `UnitsSeam::GetPieceOffset` |
| `GetPiecePosition` | world position of a piece | movement (`motion.cpp`: `UnitMotion::UpdatePosition`; `order_targets.cpp`: `PathOrder::FillWorldPos`); orders (`unit_orders.cpp`: `BuildingBuildOrder`); weapons (`script_explode.cpp`: `UnitScript::ExplodePiece`) | yes | `UnitsSeam::GetPiecePosition` |
| `GetPieceAngles` | angles of a piece | movement (`motion.cpp`: `UnitMotion::UpdatePosition`, a unit carried on a piece) | yes | `UnitsSeam::GetPieceAngles` |
| `GetNanoPiecePosition` | world position of the nano piece | orders (`unit_orders.cpp`: ten build, repair, reclaim, capture and resurrect orders; `vtol_orders.cpp`: five VTOL ones) | yes | `UnitsSeam::GetNanoPiecePosition` |
| `GetAimFromPosition` | the position a weapon aims from | weapons (`weapons.cpp`: `CalcAimAngles`, `FireTurretWeapon`, `InitUnitWeaponSlots`, `UpdateUnitWeapons`) | yes | `UnitsSeam::GetAimFromPosition` |
| `GetSweetSpot` | the point weapons aim at | units (`unit_targets.cpp`: `GetWeaponTargetPos`) | yes | `UnitsSeam::GetSweetSpot` |
| `PoseUnitModel` | poses a unit's model from its angles and animation | weapons (`explosions.cpp`: `StartExplodePiece`) | yes | `UnitsSeam::PoseUnitModel` |
| `CreateObjectState` | builds the piece state of a model (a feature, or a unit with no script) | units (`units.cpp`: `InitUnitScript`) | both | `UnitsSeam::CreateObjectState` |
| `FreeObjectState` | frees a piece state and its cached pictures | units (`units.cpp`: `ApplyUnitDeath`; `features_effects.cpp`: `UnitsOnFeaturesFreeState`) | yes | `UnitsSeam::FreeObjectState` |
| `UpdateObjectState` | poses a model from its current angles when they changed | presentation (`model_render.cpp`: `DrawUnit`) | no | `UnitsSeam::UpdateObjectState` |
| `ObjectStateSize` | bytes of a model's piece state, a query | features (`features.cpp`: `PlaceFeature`) | both | `UnitsSeam::ObjectStateSize` |
| `NewUnitScript` | allocates a unit's script and stores it in the unit | units (`units.cpp`: `InitUnitScript`) | both | new: was a direct call of `NewUnitScript` from `units.cpp` |
| `SetCob` | gives a script its unit type's compiled data | units (`units.cpp`: `InitUnitScript`) | both | new: was `CobScript::SetCob` from `units.cpp` |
| `CreatePlayerObjectState` | builds a scripted unit's piece state | units (`units.cpp`: `InitUnitScript`) | both | new: was a direct call of `CreatePlayerObjectState` from `units.cpp` |
| `SetObjectState` | gives a unit script its piece state | units (`units.cpp`: `InitUnitScript`) | both | new: was `UnitScript::SetObjectState` from `units.cpp` |
| `RunScripts` | runs a script's threads for the elapsed ticks | units (`unit_position.cpp`: `UpdateUnitBody`) | yes | new: was `CobScript::RunScripts` from `unit_position.cpp` |
| `SaveScriptState` | writes a script's threads to a saved game | units (`unit_save.cpp`: `SaveUnits`) | no | new: was `CobScript::SaveScriptState` from `unit_save.cpp` |
| `LoadScriptState` | reads a script's threads from a saved game | units (`unit_save.cpp`: `LoadUnit`) | no | new: was `CobScript::LoadScriptState` from `unit_save.cpp` |
| `DeleteScript` | destroys a unit script (the virtual deleting destructor) | units (`units.cpp`: `ApplyUnitDeath`) | yes | new: was a `delete` of the `CobScript` from `units.cpp` |
| `InitObjectState` | fills a model's piece state the caller allocated | units (`features_effects.cpp`: `UnitsOnFeaturesInitState`) | yes | new: was a direct call of `InitObjectState` from `features_effects.cpp` |

Notes on the table:
- **`InitUnitScript` stays in units** (`units.cpp`, `FUNCTION: 0x485d40`): it is the unit's creation, and it now
  makes six calls through the seam where it made direct ones. Creation runs in the tick (a unit built or spawned,
  or made from a packet) and when a game starts, so the five slots only it calls are marked both.
- **`ObjectStateSize` and the two feature slots.** A feature's piece state is made by `PlaceFeature` (in the tick for
  a feature placed by an event, outside it at load and by the console) and freed and filled by the effects
  FREE_STATE and INIT_STATE (`docs/features-interface.md`), which units applies through `FreeObjectState` and
  `InitObjectState`.
- **`UpdateObjectState` is a drawing call.** `DrawUnit` calls it in the draw pass, which poses the model for
  display. Presentation does not read a unit's pose through any other slot. `PoseUnitModel` is the simulation's
  version for a unit whose pose the simulation reads (a piece blown off, `StartExplodePiece`).
- **No slot is a callback.** The imports of section 1 are calls from scripts out, not slots of this header.
- **Slots whose callers all sit in one subsystem** (`GetSweetSpot` in units, `GetAimFromPosition` in weapons,
  the save and load pair in units) are still slots: the interface is the set of what other code calls, whoever
  that is today.

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
every `g_seams.units->` call of the 16 slots, is now `g_seams.scripts->`: the files are in `src/orders/`
(`order_list.cpp`, `order_queue.cpp`, `order_targets.cpp`, `unit_orders.cpp`, `vtol_orders.cpp`), `src/units/`
(`economy_effects.cpp`, `features_effects.cpp`, `targeting_effects.cpp`, `unit.cpp`, `unit_motion.cpp`,
`unit_position.cpp`, `unit_save.cpp`, `unit_targets.cpp`, `units.cpp`), `src/weapons/`, `src/network/net_game.cpp`,
`src/map/features.cpp` and `src/graphics/model_render.cpp`.

## 4. The rules of `data/subsystems.csv`

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

## 5. State and the trace

Scripts keep no private region, so they have no `WriteState` report: their state is the script object of each unit
and its piece state, hashed where they are (the human, 2026-11-16 and 2026-11-17, on #978 and #984). It stays hashed
in this change as section 23 of `docs/trace-format.md` (8.12, "scripts"), and the units' section hashes the unit's
`script` and `state` pointers as before. `docs/trace-format.md` is not edited here (#979 works there). The scripts'
class rows in `data/state_classes.csv` name `src/scripts/` as the owner (`NewUnitScript: new UnitScript`,
`CreateObjectState` and `CreatePlayerObjectState: GameAllocIgnoreTag ModelState`). A later format bump, for weapons,
orders and units, moves out of the hash what only its owner reads (as #979 does for los_radar, players, ai and map);
the last list below is what scripts adds to that bump.

| Scripts' state | Where | Read by | Only scripts reads it |
|---|---|---|---|
| The threads: 8 `Channel` records of a `CobScript` (state, program counter, stack pointer, the stack of 32 words, signal, delay, wait, piece and axis, the callback pointer) | the unit's script object, hashed (section 23, `channels[c]`) | scripts | yes |
| The script's statics (`CobScript::statics`, as many words as the compiled data names) | a block allocation, hashed (`script.statics`) | scripts | yes |
| The piece records (`Piece`: move, turn and spin targets, speeds, acceleration, active bits) | a block allocation, hashed (`script.pieces[p]`) | scripts | yes |
| The script's own words (`CobScript::scale`, `table`, `changed`, `activeCount`; `UnitScript::state`) | the script object, hashed (`script.<field>`) | scripts | yes |
| The piece transforms (`PieceState`: translation, rotation, visible, cached, shaded) | the model state of the unit, hashed (`script.transforms[q]`) | scripts (the piece queries); presentation (drawing, `model_render.cpp`) | yes, in the simulation: drawing reads the pose, nothing else in the tick does |
| The model state's derived data (`ModelState::pos`, `root`, `animDirty`, `cacheDrawCount`; each piece's cached `points`) | the model state, not hashed | scripts, presentation | not hashed: the derived position, the caches and the draw frame are view |
| The model state's `unit` back-pointer and `drawFrame` | the model state, a pointer marker only | scripts, presentation; units writes them (`InitUnitScript`) | no: units writes them |
| A unit's `script` and `state` pointers (`Unit::script`, `Unit::state`) | the unit record, hashed | units, and every caller of a slot, which passes the script as its first argument | no |
| The unit's bit0, bit1 and bit3 (`UnitScript::SetUnitValue`, `GetUnitValue`) | the unit record, hashed with the unit | scripts write them directly; the word they sit in is read as `Unit::flags` by units, orders and map | no |
| The feature's model state (`PlaceFeature`, `FREE_STATE`, `INIT_STATE`) | the feature, hashed as a pointer marker | features, presentation; scripts builds and fills it | no |

Scripts are not among the owners of `privatestate.py` yet (#986 adds them with units). With them, `--check` would fail
when another subsystem writes any of the items scripts alone writes: the
channels, the statics, the piece records and the transforms. By the access log (`uv run tools/coupling.py --summary
scripts`), the sim items scripts writes are those and the unit's `script` pointer and bits above; presentation writes
`ModelState::cacheDrawCount` and `ObjectState::unit` (drawing), and units writes the model state's `unit` pointer.
Scripts read, and write nothing of, the unit's position, heading, angles, flags, health, `cargo`, `cargoNext`,
`carrier`, `def` and `state`.

**Hashed state only scripts reads,** for the later format bump (weapons, orders and units):
- the 8 channels of each unit's script: state, program counter, stack pointer, the live stack words, signal, delay,
  wait, piece and axis (`script.channels[c].*`, including `.stack`);
- the statics (`script.statics`);
- the piece records (`script.pieces[p].*`);
- the script's own words (`script.<field>`: `scale`, the table pointer, `changed`, `activeCount`, `UnitScript::state`);
- the piece transforms (`script.transforms[q].*`), only scripts reading them in the simulation: the piece queries
  (`GetPiecePosition` and the others) are scripts' slots, and drawing reads them outside it.

Nothing else in the hash is scripts'. The unit's `script`, `state` and bits stay in the unit's section with the
unit.

## 6. What the next issues decide

This issue leaves the following to the issues that put the module on the model, and opens none of them:
- **#986 (units, with scripts on the scheduler).** Which of the slots become events. The candidates are the ones whose
  callers use no answer: `StartScript`, `StartScriptWithArgs` and `StartScriptWithArgsByIndex` (their results are
  mostly dropped, but not in `StartScript` of `InitUnitScript`'s "Create", nor in the `FindScript`/index pairs of
  `order_queue.cpp`, which use the index), `SetCob`, `SetObjectState`, `SaveScriptState` and `DeleteScript`. The
  queries (`QueryScript`, the piece getters, `ObjectStateSize`) and `FindScript` answer in the caller's statement.
  Whether `RunScripts`, called from `UpdateUnitBody` for each unit in 3.1c's order, becomes a pass of scripts, and where
  scripts' draws of `RandomInt` sit in it.
- **#984 (weapons), done (`docs/weapons-interface.md`).** Weapons publishes its script starts as the effect
  START_SCRIPT (its `WeaponsScript` names the script, which scripts maps back to the unit script's name) and a piece's detaching as DETACH_PIECE, both to scripts; scripts publishes a weapon's aim callback
  (`WeaponAimCobComplete`) as weapons' event AIM_DONE and the explode opcode (`CobScriptExplodePiece`) as
  EXPLODE_PIECE. `QueryScript` and `GetAimFromPosition` stay synchronous slots of scripts (mixed: they run a unit's
  script, and weapons uses the answer in the same statement), for #986.
- **The format bump for weapons, orders and units.** Moving the state section 5 lists out of the hash, as #979 does
  for los_radar, players, ai and map.
- **The piece queries stay scripts'.** They read the piece state, and their callers (weapons, orders, movement) reach
  them through this seam like any other slot.
