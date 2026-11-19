# The movement interface

Movement sits behind `interfaces/movement.h` (version 1.0), the seam over a unit's motion, the goals it steers to and
the targets orders hand those goals (#993). 3.1c's code is its default implementation. It is cut from orders (which held
`UnitMotion`, the order targets and the virtual calls on them) and units (which held the unit's reference), and the
cut left orders at 4.0 and units at 2.0. This page is the inventory the interface was cut from, written from the code
as it now is:
- where movement sits, and what it is not;
- each slot, its callers, and whether they run in the tick;
- which files and functions moved, and which stayed;
- the rules of `data/subsystems.csv`;
- the state, and the hashed state only movement reads;
- what the next issues decide.

## 1. The model

Movement is a seam over native code and nothing else, like the other simulation subsystems whose code is 3.1c's. It is
cut from the same scheme as `docs/targeting-interface.md` and `docs/map-interface.md`, with none of their parts that
need a scheduler:
- **No scheduler.** Movement has no inbox, no events, no pass and no `SCHED_*` run. `SCHED_MOVEMENT` (14) exists in
  `interfaces/services.h` (services 1.19) as an id and nothing more, as `SCHED_SCRIPTS` (13) does for scripts. Callers
  call the slots directly and synchronously, as they called orders' and units' slots before (section 2).
- **No WASM module** (the maintainer, 2026-11-19, on #993). The slots are native: `src/seams/native.cpp` fills them
  (`<Slot>Native`), and the harness records each call through a replay hook (`RecMovement<Slot>`,
  `src/harness/replay_hooks.inc`).
- **What it is.** A unit's motion record (`UnitMotion`: its velocity, turn, speed, move mode and the goal it steers
  to), the goals (`PathGoal`, `LiteGoal`, `PatrolGoal`, `PackedGoal`, `PackedPosGoal`, `AiSearchGoal`) and the targets
  an order attaches to a goal (`PathOrder`, `AirManeuverOrder`, `ApproachRadius`, `RingApproach`, `PointMarker`, all
  `OrderFx`). It steers a unit one tick at a time, takes the results of path searches, and builds, loads, saves and
  frees those objects.
- **Versions.** Movement 1.0 (33 slots), orders 4.0 (its motion slots left), units 2.0 (its script, piece and model
  state slots left for scripts, #993; it gained `UnitRefInit` and `UnitRefSetUnit`, section 3).
- **What it calls out.** Movement calls other subsystems only through their seams: pathfinding (`Submit`, `Cancel`),
  scripts (`GetPiecePosition`, `GetPieceAngles`, `StartScript`), units (`GetGroundHeight`, `SetStateBits`,
  `SetUnitPosition`, `UpdateSfxOccupy`, `UnitRefSetUnit`, `UnitRefUnlink`, `LoadUnit`) and map
  (`ScheduleMapCanPlaceUnitFootprint`). Its three publishes of other modules' events are section 6.
- **Who takes it on.** Orders (#985) puts movement on the scheduler with itself, as both run in the units phase;
  scripts go with units (#986); weapons (#984) addresses scripts' slots. None of the three changes what this page lists.

## 2. The slots

Callers are named by subsystem and file. "In the tick" says whether the caller runs inside a tick (`UpdateUnits` and the
phases around it, `RunOrders`, `HandleNetPackets`, unit creation and death) or outside it (load, save, presentation).
Every slot has the harness replay hook as a caller of the inner seam in test mode; it is not repeated in the table.

| Slot | What it does | Callers | In the tick | Came from |
|---|---|---|---|---|
| `UpdateMotion` | Steers a unit, moves it and updates its move rate for one tick | units (`unit_position.cpp`: `UpdateUnitBody`, `ReceiveUnitStates`) | yes | `OrdersSeam::UpdateMotion` |
| `ApplyPathEvent` | Applies a path search event to its requester | game (`scheduler.cpp`: `SchedPlayersPhase`, for each event pathfinding polls) | yes | `OrdersSeam::ApplyPathEvent` |
| `NewUnitMotion` | Allocates and builds a unit's motion | units (`units.cpp`: `CreateUnitMotion`, `InitUnit`, `InitSpawnedUnit`) | both: the creation code runs in the tick and outside it when a game is set up | `OrdersSeam::NewUnitMotion` |
| `LoadMotion` | Reads a unit's motion from a saved game | units (`unit_save.cpp`: `LoadUnit`) | no: load | `OrdersSeam::LoadMotion` |
| `SaveMotion` | Writes a unit's motion to a saved game | units (`unit_save.cpp`: `SaveUnits`) | no: save | `OrdersSeam::SaveMotion` |
| `DestroyMotionObject` | Frees what a unit's motion holds | units (`units.cpp`: `ApplyUnitDeath`) | yes | `OrdersSeam::DestroyMotionObject` |
| `UnitMotionReadNetState` | Reads a unit's motion from a packet, through its goal | units (`unit_position.cpp`: `ReceiveUnitStates`) | yes (`HandleNetPackets` is phase 0) | `OrdersSeam::UnitMotionReadNetState` |
| `SetFlightMode` | Grounds or lifts an aircraft | orders (`vtol_orders.cpp`: the VTOL orders, `PrepVtolClimb`) | yes | new: was a direct call of `UnitMotion::SetFlightMode` from `vtol_orders.cpp` |
| `PathOrderSetAltitude` | Sets the altitude of a move target | orders (`vtol_orders.cpp`) | yes | new: was a direct call of `PathOrder::SetAltitude` from `vtol_orders.cpp` |
| `PathOrderSetApproachRadius` | Sets how near a move target a unit has to come | orders (`vtol_orders.cpp`) | yes | new: was a direct call of `PathOrder::SetApproachRadius` from `vtol_orders.cpp` |
| `PathOrderSetHeading` | Sets the heading a unit takes on arrival | orders (`vtol_orders.cpp`: `VtolMobileBuildOrder`, `VtolHelpBuildOrder`) | yes | new: was a direct call of `PathOrder::SetHeading` from `vtol_orders.cpp` |
| `AirManeuverOrderSetAltitude` | Sets the altitude of an air maneuver target | orders (`vtol_orders.cpp`: `AirToAirOrder`) | yes | new: was a direct call of `AirManeuverOrder::SetAltitude` from `vtol_orders.cpp` |
| `NewPathOrderAt` | Builds a move target at a position | orders (`vtol_orders.cpp`: the VTOL orders and their helpers) | yes | new: was `new (kSim) PathOrder(order, pos)` in `vtol_orders.cpp` |
| `NewPathOrderToUnit` | Builds a move target that follows a unit | orders (`vtol_orders.cpp`: `VtolPickupOrder`, `AirStrikeOrder`) | yes | new: was `new (kSim) PathOrder(order, unit)` in `vtol_orders.cpp` |
| `NewPathOrderToUnitAt` | Builds a move target at a position near a unit | orders (`vtol_orders.cpp`: `AirToGroundHoverOrder`) | yes | new: was `new (kSim) PathOrder(order, unit, pos)` in `vtol_orders.cpp` |
| `NewPathOrderToUnitAtAngle` | Builds a move target that follows a unit at an angle | orders (`vtol_orders.cpp`: `VtolPickupOrder`, `VtolLandingOrder`) | yes | new: was `new (kSim) PathOrder(order, unit, angle)` in `vtol_orders.cpp` |
| `NewAirManeuverOrder` | Builds an air maneuver target from two positions | orders (`vtol_orders.cpp`: `AirToAirOrder`) | yes | new: was `new (kSim) AirManeuverOrder(order, from, to)` in `vtol_orders.cpp` |
| `NewApproachRadius` | Builds a disc target | orders (`order_queue.cpp`: `Order::AttachApproachRadiusGoal`, called from `unit_orders.cpp`) | yes | new: was `new (kSim) ApproachRadius(...)` in `order_queue.cpp` |
| `NewRingApproach` | Builds a ring target | orders (`order_queue.cpp`: `Order::AttachRingApproachGoal`, called from `unit_orders.cpp`) | yes | new: was `new (kSim) RingApproach(...)` in `order_queue.cpp` |
| `NewPointMarker` | Builds a footprint marker target | orders (`order_queue.cpp`: `Order::AttachBuildFootprintMarker`, called from `unit_orders.cpp`) | yes | new: was `new (kSim) PointMarker(...)` in `order_queue.cpp` |
| `LoadPathOrder` | Builds a move target from a saved game | orders (`order_queue.cpp`: `Order::Order`, the load constructor) | no: load | new: was `new (kSim) PathOrder(order, file, name)` in `order_queue.cpp` |
| `LoadAirManeuverOrder` | Builds an air maneuver target from a saved game | orders (`order_queue.cpp`: `Order::Order`) | no: load | new: was `new (kSim) AirManeuverOrder(order, file, name)` in `order_queue.cpp` |
| `LoadApproachRadius` | Builds a disc target from a saved game | orders (`order_queue.cpp`: `Order::Order`) | no: load | new: was `new (kSim) ApproachRadius(order, file, name)` in `order_queue.cpp` |
| `LoadRingApproach` | Builds a ring target from a saved game | orders (`order_queue.cpp`: `Order::Order`) | no: load | new: was `new (kSim) RingApproach(order, file, name)` in `order_queue.cpp` |
| `LoadPointMarker` | Builds a footprint marker target from a saved game | orders (`order_queue.cpp`: `Order::Order`) | no: load | new: was `new (kSim) PointMarker(order, file, name)` in `order_queue.cpp` |
| `MotionGoal` | Returns the goal that steers a unit's movement, a query | orders (`order_queue.cpp`: the same functions as `GoalSetPathOrder`) | both: as `GoalSetPathOrder` | new: was a read of `UnitMotion::obj` through orders' own view (`UnitMotionSlotView`) in `order_queue.cpp` |
| `GoalTarget` | Returns the target a goal steers to, or null, a query | orders (`order_queue.cpp`: `Order::~Order`) | yes | new: was a read of `PathGoal::target` through orders' own view (`OrderFxSlot::current`) in `order_queue.cpp` |
| `GoalSetPathOrder` | Hands a goal the target it steers to, or null | orders (`order_queue.cpp`: `Order::SetAttachedFx`, `Order::AttachApproachRadiusGoal`, `AttachRingApproachGoal`, `AttachBuildFootprintMarker`, `Order::~Order`, and `Order::ReattachFxToUnit`, which `LoadUnit` calls) | both: orders run in the tick; `ReattachFxToUnit` runs on load | new: was the virtual `SetPathOrder` (called through its vtable slot or as `slot->Attach`) from `order_queue.cpp` |
| `OrderFxDestroy` | Destroys a target and frees it; the target is never null (orders checks first, where 3.1c's `delete` did nothing on null) | orders (`order_queue.cpp`: the same functions) | both: as `GoalSetPathOrder`, and `Order::~Order` also runs when units are freed | new: was `delete (OrderFx*)attached` in `order_queue.cpp` |
| `OrderFxGetType` | Returns the kind of a target | orders (`order_queue.cpp`: `Order::SerializeToSave`) | no: save | new: was the virtual `GetType` from `order_queue.cpp` |
| `OrderFxSerializeToSave` | Writes a target to a saved game | orders (`order_queue.cpp`: `Order::SerializeToSave`) | no: save | new: was the virtual `SerializeToSave` from `order_queue.cpp` |
| `GoalHasNetUnitState` | Says whether a goal has network state to send | network (`net_game.cpp`: `NetSendUnitStates`) | yes | new: was the virtual `HasNetUnitState` (`v7`) from `net_game.cpp` |
| `GoalSerializeNetUnitState` | Writes a goal's network state to a packet | network (`net_game.cpp`: `NetSendUnitStates`) | yes | new: was the virtual `WriteTo` from `net_game.cpp` |

`NetSendUnitStates` is called from the units phase of the scheduler (`scheduler.cpp`, `SendUnitStates`). Orders'
`Order::~Order` runs when orders are deleted in the tick (`DeleteOrders`) and when a unit is freed.

No slot has a caller in presentation. Every caller of a slot is a subsystem that called the same code directly in 3.1c,
so no call moved in time: the cut changed how the call is made, not when.

## 3. What moved

**Files moved** (`git diff -M`, bc4825ffd), `src/orders/` to `src/movement/`: `air_maneuver_order.h`, `approach_areas.h`,
`order_fx.h`, `order_goals.h`, `order_target_types.h`, `order_targets.cpp`, `order_targets.h`, `path_order.h`,
`unit_motion.h`. From `src/game/`: `goal_owner.h`, `packed_goal.h`. From `src/ai/`: `ai_search_goal.h`.

**Functions moved between files:**
- `UnitMotion`'s methods, `UnitMotion::UnitMotion` and the load and save functions of `src/orders/order_list.cpp` to the
  new `src/movement/motion.cpp`: `ApplyClampedTurnDelta`, `UpdateVelocityFromHeading`, `SteerGroundUnit`,
  `ApplyBankAndPitch`, `SetFlightMode`, `SteerAircraft`, `UpdatePosition`, `UpdateMoveRate`, `UnitMotion`,
  `DestroyObject`, `UpdateMotion`, `SaveMotion` and `LoadMotion`. The remaining functions of `motion.cpp`
  are the natives' helpers and the net-state reader.
- The goal classes of `src/game/victory.cpp` (`PackedGoal`, `LiteGoal`, `PackedPosGoal`) to the new
  `src/movement/packed_goal.cpp`. `victory.cpp` keeps the victory code (players).

**Files that split:** `order_list.cpp` (motion left it for `motion.cpp`; orders keeps the order list) and `victory.cpp`
(the goals left it for `packed_goal.cpp`). `src/movement/unit_def_move_view.h` is new: the fields of a unit definition
that movement reads, as a view.

**What stayed, and why:**
- **`PathOrderAttach`** (`src/units/path_order_attach.h`; `PathOrderAttach::PathOrderAttach` and `SetUnit` in
  `src/units/unit_targets.cpp`). #993 lists them as movement's. They stay units': `PathOrderAttach` is the unit
  reference (`UnitRef`) that orders (`Order::link`) and movement (`PathOrder::ref`) both embed, and its link and unlink
  are units' unit-list code (`Unit::head`, `UnitRef::Unlink`). Movement and orders reach it through units' new slots
  `UnitRefInit` and `UnitRefSetUnit`, and through the existing `UnitRefUnlink` (units 2.0); all three take a `UnitRef*`. Where the caller built the
  reference in place (`link(o, 0)`), the constructor now runs empty and `UnitRefInit` fills it; its destructor body
  moved into `UnitRef::Unlink`. The header moved from `src/orders/` to `src/units/`.
- **`MovementClass`** (`src/units/movement_class.cpp`): the pass map and the movement class table are units' and map's
  (`docs/map-interface.md`), not the motion's. `UnitMotion::movementClass` only points at one.
- **Pathfinding.** `src/ai/pathfind*.cpp` stays pathfinding's. It reads a goal's areas through `PathGoalCells`,
  `PathGoalContains` and `PathGoalDistance` in `src/util/path_goal.cpp` (platform), not through a slot.
- **`UpdateUnitHeight`** and the unit's position code stay units'. Movement writes `Unit::pos` and `Unit::angles` through
  `UnitMotion::UpdatePosition` and its steering, and reaches units' `SetUnitPosition` through its slot.
- **Orders' reads of motion fields.** Orders still reads `UnitMotion::flags`, `velocity` and `field_22` in the VTOL
  orders, as it did (section 5).

## 4. The rules of `data/subsystems.csv`

`tools/coupling.py` gives each function a subsystem by a rule, `file:` (`FILES`), `name:` (`NAMES`) or `default:`
(`FOLDER_DEFAULT`), and an override column that wins (`data/subsystems.csv`, the `override` column). #993's change to
the tool:
- **`FILES`** gains `("src/movement/", "movement")` (and scripts' `("src/scripts/", "scripts")`), before the other
  entries.
- **`FOLDER_DEFAULT`** gains `"src/movement/": "movement"` (and `"src/scripts/": "scripts"`).
- **`SIM_SUBSYSTEMS`** gains `"scripts"` and `"movement"`.

So every function in `src/movement/` is movement's by `file:src/movement/`: 144 functions (`motion.cpp` 28,
`order_targets.cpp` 102, `packed_goal.cpp` 11, `order_fx.h` 2, `ai_search_goal.h` 1), but for the override below.

Rows that stay elsewhere, by override:

| Function | File | Subsystem | Why |
|---|---|---|---|
| `PathOrderAttach::PathOrderAttach` | `src/units/unit_targets.cpp` | units (`name:Order`, override `units`) | The unit reference stays units' (section 3) |
| `PathOrderAttach::SetUnit` | `src/units/unit_targets.cpp` | units (`name:Order`, override `units`) | The same |
| `MakeVec3` | `src/movement/packed_goal.cpp` | platform (`file:src/movement/`, override `platform`) | A vector helper, as in every other subsystem |

The rows that moved with the files carry the same rule under their new file: the `UnitMotion` rows
(`src/movement/motion.cpp`) and the order target rows (`src/movement/order_targets.cpp`) went from `orders`,
`file:src/orders/` to `movement`, `file:src/movement/`; the rows of `PackedGoal`, `LiteGoal` and `PackedPosGoal` went
from the players' `victory.cpp` rule to `file:src/movement/`. No `name:` rule is new for movement.

## 5. State and the trace

Movement keeps no private region, and so it has no `WriteState` report: its state is sim state in the shared block, hashed where it was. It
stays where it is and stays hashed in this change (the human, 2026-11-16 and 2026-11-17, on #978 and #984); the heap
rows of `data/state_classes.csv` (`UnitMotion::UnitMotion: new ...`, the goals, and the `New*Native` and `Load*Native`
rows) name movement as the class's owner, and the format bump of weapons, orders and units takes out of the hash what
only its owner reads. The trace hashes the motions and goals in section 21 and the order effects in section 22
(`docs/trace-format.md`, sections 8.10 and 8.11); this change edits neither.

| Movement's state | Where | Read by | Only movement reads it |
|---|---|---|---|
| The motion record (`UnitMotion`: `velocity`, `p2`, `turn`, `lastMoveTick`) | the unit's heap object, hashed (section 21) | movement | yes |
| `UnitMotion::speed` | the same | movement, units (`ReadUnitState`, `WriteUnitState`) | no: units reads and writes it in the unit's network state |
| `UnitMotion::mode`, `flag` | the same | movement; units (`ApplyAttachUnit` writes `mode`) | no |
| `UnitMotion::flags`, `velocity`, `field_22` | the same | movement, orders (the VTOL orders: `PrepVtolClimb`, `VtolFollowOrder`, `VtolHelpBuildOrder`, `VtolMobileBuildOrder`, `VtolSeekAttackOrder`, `AirToAirOrder`, `AirStrikeOrder`) | no |
| `UnitMotion::pathLockStamp` | the same | movement; map writes it by offset (`AddUnitToMap`, `+0x26`) | no |
| `UnitMotion::movementClass` (and the planner view of its first field) | the same | movement, pathfinding (`src/seams/pathfinding_world.cpp`, by offset) | no |
| The goal pointer (`UnitMotion::player` / `obj`) | the same | movement, network (`NetSendUnitStates` passes it to the slots), orders (through `MotionGoal`), pathfinding (by offset) | no: it is the object the slots take |
| The target a goal steers to (`PathGoal::target`) | the goal, hashed | movement, orders (through `GoalTarget`) | no: orders reads it through the slot |
| The goals' fields (`PatrolGoal`, `PackedGoal`, `LiteGoal`, `PackedPosGoal`, `AiSearchGoal`) | the goal's heap object, hashed (section 21) | movement; presentation (`AiSearchGoal::DrawOnSurface` reads `active`, `count` and `owner`, console debug overlay) | yes, in the simulation |
| The goal areas (`PathGoalArea`) | filled by the goals | movement; pathfinding, through `PathGoalCells`, `PathGoalContains` and `PathGoalDistance` | no |
| The targets' fields (`PathOrder`, `AirManeuverOrder`, `ApproachRadius`, `RingApproach`, `PointMarker`) | the order effect, hashed (section 22) | movement; `OrderFx::source` is the order | yes: orders reaches them only through the slots |
| The unit reference of a move target (`PathOrder::ref`, a `PathOrderAttach`) | the target, hashed | movement, units (the list), orders (`Order::link`, `Order::SerializeToSave`, `Matches`) | no: it is units' (section 3) |

What movement writes of others' state, from `uv run tools/coupling.py --summary movement`: `Unit::pos`, `Unit::angles`,
`Unit::flags`, `Unit::mode` and `Unit::moved` (units'), `Order::subFlags` (orders'), `PathRequest` fields
(pathfinding's) and the `AiSearchGoal` repath claim. It reads `Game::ticks`, `Game::gravity`, `Game::seaLevel`,
`Game::overflowBucket`, `Unit::motion`, `Unit::def` and `Unit::cell` among others, and the fields of the unit
definition, as of the last write of the owner.

**The hashed state only movement reads**, for the later format bump:
- the motion record's `velocity` is read by orders and its `p2`, `turn` and `lastMoveTick` by movement alone, so the
  bump can drop `p2`, `turn` and `lastMoveTick` and keep the fields others read;
- every field of the goals (`PatrolGoal` points and count, `AiSearchGoal` points, flags and repath claim,
  `PackedGoal` pose, `LiteGoal` mode, dirty flags and state), but for what the debug overlay of presentation reads;
- every field of the targets of section 22 (`PathOrder` altitude, approach radius, heading, piece, position and follow
  offset, `AirManeuverOrder`, `ApproachRadius`, `RingApproach`, `PointMarker`), the order effect class and its
  numbering, and not the unit reference of a move target (section 3).

**Reads through views.** Other subsystems read motion records by offset through views the access log names by their own
type, so `UnitMotion::...` does not show them: weapons reads the word at `+0x20` (`UnitMotion::speed`) as a range, a
parameter and an id (`MotionRangeView`, `DroppedMotionParamView`, `DroppedMotionIdView` in `weapons.cpp`) and
`explosions.cpp` reads `+8` as a size (`Size`); pathfinding's `UnitMotionPlannerView` reads the first two fields;
`AddUnitToMap` in `terrain.cpp` writes `pathLockStamp` (`+0x26`). A bump that moves `speed`, `velocity`,
`movementClass` or the goal pointer out of the unit's record has to give these readers a slot or keep the offsets.

## 6. What the next issues decide

This issue closes the cut and leaves these to #984, #985 and #986:
- **#985 puts movement on the scheduler with orders.** It decides which slots become events, and whether movement has a
  pass or runs as part of orders' unit update. `UpdateMotion` is called from units' `UpdateUnitBody` after `RunOrders` and
  `RunSecondaryOrders`, so the order of the two runs is the order the scheduler has to keep.
- **The three publishes in `UnitMotion::UpdatePosition`** (`src/movement/motion.cpp`) carry `TODO(#985)` markers: map's
  `MAP_REMOVE_UNIT` and `MAP_ADD_UNIT` and los_radar's update of the unit, all published from `SCHED_ORDERS`. They move
  to `SCHED_MOVEMENT` when movement has its own outbox, as the routes of `src/game/routes.cpp` are keyed by the
  publisher.
- **The steering slots** (`SetFlightMode`, `PathOrderSetAltitude`, `PathOrderSetApproachRadius`, `PathOrderSetHeading`,
  `AirManeuverOrderSetAltitude`) and the construction slots are called by orders' state machines, which use no answer
  but the target a `New*` slot returns; #985 decides which of them become events and which stay synchronous.
- **`ApplyPathEvent`** is called from the players phase at the end of pathfinding's poll; its place in the order of
  phases is the model pathfinding (`docs/pathfinding-interface.md`) and #985 settle.
- **The load, save and network slots** (`LoadMotion`, `SaveMotion`, the `Load*` and `OrderFx*` slots, `GoalHasNetUnitState`,
  `GoalSerializeNetUnitState`) run outside the tick or on the network's side; nothing here makes them events.
- **The state** stays hashed where it is until the format bump of section 5 (weapons, orders and units), which #984,
  #985 and #986 take with their own sections of the hash.
- **Weapons (#984)** reads motion fields by view (section 5) and calls scripts' slots, not movement's, so it adds no
  caller to this page.
