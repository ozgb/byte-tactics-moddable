# The movement interface

Movement sits behind `interfaces/movement.h` (version 4.0), the seam over a unit's motion, the goals it steers to and
the targets orders hand those goals (#993), and since #985 the tenth module on the scheduler, with orders
(`docs/orders-interface.md`). 3.1c's code is its default implementation. It was cut from orders (which held
`UnitMotion`, the order targets and the virtual calls on them) and units (which held the unit's reference) by #993, and
put on the scheduler by #985. This page is the inventory the interface was cut from, written from the code as it now is:
- where movement sits, and what it is not;
- what other subsystems did to movement's state in 3.1c, and what each became;
- what movement did to others (the unit record), and what each became;
- what movement reads, and what others read of it;
- how the order of random draws and sim events is kept;
- what stays inside the default, which files moved and the rules of `data/subsystems.csv`;
- the state, and what the trace hashes and what it only dumps.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, events published
outside the tick delivered at once (#970), and events as plain data. This issue changes none of its rules. Event kinds
are unique across interfaces: weapons uses 256 to 267, 269 and 272 to 283, orders 288 to 295 and 304 to 317 (its range
is 288 to 319) and movement 320 to 324, 326 to 331 and 336 to 347 (325, PLACED until 4.0, stays free; its range is 320 to 351); the others are listed in
`docs/weapons-interface.md`, section 1.

Two rulings of the human shape this change:
- **Movement owns the unit record's motion fields** (2026-11-18): the position, the angles, the cell, the mode and the
  moved mark that movement steers are movement's to write, and the motion record's fields are movement's alone, but
  for the path lock stamp, which is map's since 4.0 (the ruling of #1019, section 2).
  Units' and map's writes to `UnitMotion` belong to this change (section 2).
- **No slot passes memory ownership across a seam, and major bumps are cheap** (2026-11-19). A target (a `PathOrder`,
  an `ApproachRadius`, a `RingApproach`, a `PointMarker` or an `AirManeuverOrder`, all `OrderFx`) is named by its block
  offset everywhere outside `src/movement/`, and an event carries it as that offset.

Movement's place in it:
- **Events into movement:** NEW, DESTROY, LOAD, SET_MODE, SET_SPEED, SET_TARGET, DESTROY_TARGET,
  SET_FLIGHT_MODE, SET_ALTITUDE, SET_APPROACH_RADIUS and SET_HEADING, published by units and orders (PLACED, published by map, was removed in 4.0). Movement's
  handler (`MovementInbox`, `src/seams/movement_world.cpp`) passes each to `Apply` and records it with its outcome in
  the trace's dump (section 5).
- **Routes:** each route's reason is in section 4.
- **Runs:** `UpdateMotion`, called by the scheduler as a step of the units pass, for a unit with a motion, after
  `RunSecondaryOrders` and with movement's inbox delivered first (section 6). The network's `ReceiveUnitStates` calls it
  too, directly (section 2).
- **The engine's delivery:** `ApplyPathEvent` is pathfinding's result, delivered by the engine as before: the players
  phase (`SchedPlayersPhase`, `scheduler.cpp`) polls pathfinding and calls the slot for each event, with the waypoints
  `Poll` copied. It is not a route, and movement's inbox is not delivered for it.
- **Synchronous slots:** the `New*` slots build a target and return it, so the answer is the new target (section 2).
- **Queries:** `GetMotionState`, `HasNetState`, `TargetType` and `UnitTarget`, as of movement's last apply, and the view
  read `WriteNetState`.
- **Effects out of movement:** ORDER_FLAGS to orders; TURN, TILT, ANGLES, MOVE, MOVED, CHANGE_CELL, MOVE_RATE,
  SET_POSITION, SET_STATE_BITS and SFX_OCCUPY to units; START_SCRIPT to scripts; and the map's ADD_UNIT and REMOVE_UNIT
  and los_radar's UPDATE_UNIT, which `UnitMotion::UpdatePosition` publishes from `SCHED_MOVEMENT` (they were
  `TODO(#985)` markers published from `SCHED_ORDERS`; the routes of `src/game/routes.cpp` are keyed by the publisher).
  Their handlers are adapters in `src/seams/units_world.cpp` and `src/seams/scripts_world.cpp` (since #986 handlers of
  units' and scripts' inboxes, which `UnitsApply` and `ScriptsApply` consume).
- **Module ids:** movement keeps `SCHED_MOVEMENT = 14` from #993.
- **No WASM module** (the maintainer, 2026-11-19, on #993). The slots are native: `src/seams/native.cpp` fills them
  (`<Slot>Native`), and the harness records each call through a replay hook (`RecMovement<Slot>`,
  `src/harness/replay_hooks.inc`).
- **What it is.** A unit's motion record (`UnitMotion`: its velocity, turn, speed, move mode and the goal it steers
  to), the goals (`PathGoal`, `LiteGoal`, `PatrolGoal`, `PackedGoal`, `PackedPosGoal`, `AiSearchGoal`) and the targets
  an order attaches to a goal. It steers a unit one tick at a time, takes the results of path searches, and builds,
  loads, saves and frees those objects.
- **Versions.** Movement 4.0 (20 slots; 3.0 had 20 and 2.0 had 33), orders 5.0, units 5.0 (`UpdateUnitBody` returns an int, and
  `UpdateUnitTail`, `UnitRefGet` and `UnitRefSetValue` are new, section 2).
- **What it calls out.** Movement calls other subsystems through their seams: pathfinding (`Submit`, `Cancel`), scripts
  (`GetPiecePosition`, `GetPieceAngles`), units (`LoadUnit`, `UnitRefInit`, `UnitRefSetUnit`, `UnitRefUnlink`) and map
  (`GetGroundHeight`, and the service `ScheduleMapCanPlaceUnitFootprint`). Its writes to the unit record, the order and
  scripts are effects (section 3).

## 2. Events into movement

**Every slot of 2.0.** The 33 slots of movement 2.0, and what each became in 3.0:

| 2.0 slot | 3.0 |
|---|---|
| `NewUnitMotion` | event: NEW, from units (`units.cpp`: `CreateUnitMotion`, `InitUnit`, `InitSpawnedUnit`), the header alone |
| `DestroyMotionObject` | event: DESTROY, from units (`units.cpp`: `ApplyUnitDeath`), the header alone (below) |
| `LoadMotion` | event: LOAD, from units (`unit_save.cpp`: `LoadUnit`), with a `MovementMotionState` (a `MovementSavedState` since 4.0) |
| `SaveMotion` | query: `GetMotionState`, which fills the state a saved game keeps, called by units (`unit_save.cpp`: `SaveUnits`) |
| `UpdateMotion` | run, called by the scheduler, and by the network's `ReceiveUnitStates` (below) |
| `ApplyPathEvent` | the engine's delivery, unchanged (section 1) |
| `UnitMotionReadNetState` | slot `ReadNetState`, taking the unit, called by units (`ReceiveUnitStates`) |
| `GoalHasNetUnitState`, `GoalSerializeNetUnitState` | slots `HasNetState` (a query) and `WriteNetState` (a view read), taking the unit, called by the network (`NetSendUnitStates`) |
| `SetFlightMode` | event: SET_FLIGHT_MODE, from orders (the VTOL orders, `PrepVtolClimb`) |
| `PathOrderSetAltitude`, `AirManeuverOrderSetAltitude` | event: SET_ALTITUDE, from orders (`vtol_orders.cpp`); a move target and an air maneuver target take it, a target of another kind refuses |
| `PathOrderSetApproachRadius` | event: SET_APPROACH_RADIUS, from orders |
| `PathOrderSetHeading` | event: SET_HEADING, from orders (`VtolMobileBuildOrder`, `VtolHelpBuildOrder`) |
| `GoalSetPathOrder` | event: SET_TARGET, from orders (`Order::SetAttachedFx`, the attach functions of `order_queue.cpp`, `Order::ReattachFxToUnit`), with the target or 0 for none |
| `OrderFxDestroy` | event: DESTROY_TARGET, from orders (`Order::~Order` and the same functions), with the target |
| `NewPathOrderAt`, `NewPathOrderToUnit`, `NewPathOrderToUnitAt`, `NewPathOrderToUnitAtAngle`, `NewAirManeuverOrder`, `NewApproachRadius`, `NewRingApproach`, `NewPointMarker` | synchronous slots, unchanged but for the types: the order and the answer are block offsets |
| `LoadPathOrder`, `LoadAirManeuverOrder`, `LoadApproachRadius`, `LoadRingApproach`, `LoadPointMarker` | one slot, `LoadTarget(type, order, file, name)`, outside the tick |
| `OrderFxSerializeToSave` | slot `SaveTarget`, outside the tick |
| `OrderFxGetType` | query: `TargetType` |
| `MotionGoal`, `GoalTarget` | query: `UnitTarget(unit)`, which answers the target the goal of the unit's motion steers to, or 0 |
| (not slots: units' and map's writes to `UnitMotion`) | events: SET_MODE, SET_SPEED, PLACED (removed in 4.0, below) |

`Apply` and `WriteState` are new. The 33 slots of 2.0 became 20 in 3.0.

**Movement 4.0 (#1019): the path lock stamp is map's.** The maintainer ruled that map owns `UnitMotion::pathLockStamp` in place, as it owns the bucket fields and claim bits it keeps in units' record (`docs/map-interface.md`, section 3). The record's layout and the trace are unchanged. The kind PLACED 325 is removed and the number stays free: `Apply` rejects 325 as an unknown kind. `MovementMotionState` is replaced by `MovementSavedState`, the same fields without `pathLockStamp`, and the LOAD payload `MovementLoad` by `MovementLoadState`, under new names because a field removed in place is not allowed by any version bump. `GetMotionState` fills a `MovementSavedState`. Movement neither writes nor reports the stamp: the saved game sets it through map's LOAD_PATH_LOCK_STAMP and reads it through map's `PathLockStamp`, and `AddUnitToMap` writes it itself. The motion's constructor (NEW) no longer zeroes it: the record is allocated zeroed, so the stamp is 0 from NEW until map first stamps it, as before.

**The writers, one by one.** Units' and map's writes to the motion record became events (the ruling of 2026-11-18):

| 3.1c site | 3.1c write | Event |
|---|---|---|
| units: a unit gets its motion (`CreateUnitMotion`, `InitUnit`, `InitSpawnedUnit`) | `new UnitMotion(unit)` | NEW from units |
| units: a unit dies (`ApplyUnitDeath`) | `DestroyMotionObject`, then the motion freed | DESTROY from units |
| units: a saved game loads (`unit_save.cpp`) | `LoadMotion`: the record's fields read | LOAD from units |
| units: a unit carried or set down (`UpdateUnitBody`, `unit_position.cpp`) | `UnitMotion::mode` written | SET_MODE from units |
| units: a unit state arrives from the network (`ReadUnitState`) | `UnitMotion::speed` written | SET_SPEED from units |
| map: a unit is placed on the map (`AddUnitToMap`, `terrain.cpp`) | `UnitMotion::pathLockStamp` written with the tick | PLACED from map in 3.0; none in 4.0 (below) |
| orders: an order attaches a goal's target (`order_queue.cpp`) | `PathGoal::SetPathOrder` | SET_TARGET from orders |
| orders: an order is deleted or replaced (`order_queue.cpp`) | `delete` of the target | DESTROY_TARGET from orders |
| orders: a VTOL order (`vtol_orders.cpp`) | `UnitMotion::SetFlightMode` | SET_FLIGHT_MODE from orders |
| orders: a VTOL order | `PathOrder::SetAltitude`, `AirManeuverOrder::SetAltitude` | SET_ALTITUDE from orders |
| orders: a VTOL order | `PathOrder::SetApproachRadius` | SET_APPROACH_RADIUS from orders |
| orders: a VTOL order | `PathOrder::SetHeading` | SET_HEADING from orders |

`PathOrderAttach` stays units'. Its inline `Get` and `SetValue` became units' slots `UnitRefGet` (a view read) and
`UnitRefSetValue` (units 5.0), so that orders' and movement's calls to them go through `units.*`.

**Orders to movement, which become events and which stay synchronous slots.** #993 left all of them synchronous slots.
The calls whose caller uses no answer, and writes only, are the events above (SET_TARGET, DESTROY_TARGET,
SET_FLIGHT_MODE, SET_ALTITUDE, SET_APPROACH_RADIUS and SET_HEADING). The `New*` calls stay synchronous slots, because
the state machine takes the target's block offset the slot returns and uses it in the next statement (attaches it to the
order, hands it to a `Set*` event): the same-statement answer is the new target. The queries `UnitTarget` and
`TargetType` answer as of movement's last apply, and read the motion from the view.

**Calls of units to movement outside the tick.** `LoadMotion` and `SaveMotion` ran when a saved game loads and saves
(`LoadUnit`, `SaveUnits`): LOAD is published outside the tick and delivered at once (#970), and `GetMotionState` is a
query that writes nothing. `NewUnitMotion` is both: unit creation runs in the tick (a build, a spawn) and outside it when
a game is set up.

**The network's writers** wait for #948 (parked), as #954's did. `ReceiveUnitStates` (`unit_position.cpp`, called from
`HandleNetPackets`, phase 0) calls `ReadNetState` and `UpdateMotion` directly: `TODO(#948)` marks the `UpdateMotion`
call. `NetSendUnitStates` calls `HasNetState` and `WriteNetState`, which read.

**`UpdateMotion` and the scheduler.** The scheduler's units step is:

```
if (UpdateUnitBody(u)) { deliver orders; RunOrders(u); deliver orders; RunSecondaryOrders(u);
                         if (u->motion) { deliver movement; UpdateMotion(u); moved = 1 } }
deliver units; UpdateUnitTail(u, moved)
```

3.1c called these from inside `UpdateUnit`; the split keeps 3.1c's order. `UpdateUnitBody` returns 1 when the unit's player
is active and of type 1 or 2, and `UpdateUnitTail(u, moved)` runs the unit's height when `moved` is 1, then a pending kill.

**`ApplyPathEvent` stays the engine's delivery** (the issue allowed a route): it is pathfinding's result, delivered by the
engine as before from the players phase, with the waypoints `Poll` copied, so there is no event to route and the order
is the poll's. A started search sets the claim tick of the goal and a status flag on the order, and a finished one gives
the goal its waypoints.

**`DestroyMotionObject`'s virtual, resolved.** `docs/schedule.md` could not resolve the virtual call it makes. The
DESTROY event frees the motion, and the goal is freed through its virtual destructor, `obj->v0(1)`; each goal class's
destructor writes:

| Goal class | Destructor | What it writes |
|---|---|---|
| `PathGoal` | 0x44ef60 | nothing |
| `AiSearchGoal` | 0x44f450 | calls pathfinding's `Cancel(goalOffset)` |
| `LiteGoal` | 0x490840 | compiler-generated, nothing |
| `PackedGoal` | 0x490630 | compiler-generated, nothing |
| `PackedPosGoal` | 0x4909e0 | deletes its target (a `PathOrder`, whose `Destroy` unlinks its unit reference, or an `AirManeuverOrder`) and sets target to 0 |
| `PatrolGoal` | 0x44f590 | trivial |

Each frees the goal with `operator delete` (the sim block). The motion is freed by `operator delete` after.

**Outcomes.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 49) and returned to no
one: 1 for every kind, and 0 for the events it refuses. `Apply` refuses, with outcome 0 and no change, an event smaller
than its kind's payload, an unknown kind, an event naming a null unit (but DESTROY_TARGET, SET_ALTITUDE,
SET_APPROACH_RADIUS and SET_HEADING, which name a target instead), an event that needs a motion (all but NEW) naming a
unit with none, a DESTROY_TARGET, SET_ALTITUDE, SET_APPROACH_RADIUS or SET_HEADING naming no target, and a
SET_ALTITUDE for a target that is neither a move target nor an air maneuver (SET_APPROACH_RADIUS and SET_HEADING need a
move target).

**Payloads.** Every event and effect starts with `MovementEventHeader`: `kind` and `unit` (the unit it is about, a block
offset; 0 for the kinds about a target). Each kind has the payload of `interfaces/movement.h` (version 4.0) that starts
with it, or is the header alone. `Apply(event, size)` refuses an event smaller than its kind's payload.

| Kind | Payload | Fields after the header |
|---|---|---|
| NEW 320, DESTROY 321 | the header alone | none |
| LOAD 322 | `MovementLoadState` | `state` (`MovementSavedState`: `velocity[3]`, `p2[3]`, `speed`, `turn`, `mode`, `flag`) |
| SET_MODE 323, SET_FLIGHT_MODE 328 | `MovementSetMode` | `mode`, 0 to 3 |
| SET_SPEED 324 | `MovementSetSpeed` | `speed` |
| SET_TARGET 326, DESTROY_TARGET 327 | `MovementTarget` | `target` (a block offset, 0 for none) |
| SET_ALTITUDE 329, SET_APPROACH_RADIUS 330, SET_HEADING 331 | `MovementTargetValue` | `target`, `value` (the altitude, the radius or the heading) |

SET_MODE puts the motion in a flight mode and nothing else, as a carrier's cargo is set down or picked up;
SET_FLIGHT_MODE is `UnitMotion::SetFlightMode`, which grounds or lifts an aircraft with the mode's other changes.

## 3. Effects out of movement

Movement writes the unit record where 3.1c's motion code did: `UnitMotion::UpdatePosition`, the steering and
`UpdateMoveRate` (`src/movement/motion.cpp`). Each write became an effect that units' adapter applies (`src/seams/
units_world.cpp`), or stayed movement's own state.

| 3.1c write or call | Effect | Subscriber's handler |
|---|---|---|
| a target reports progress (`order_targets.cpp`): `OrderOrStatusFlags` on the order | ORDER_FLAGS | `OrdersOnMovementOrderFlags`: STATUS_FLAGS to orders' inbox |
| the heading turns and `moved` is set (`SteerGroundUnit`, `SteerAircraft`) | TURN | `UnitsOnMovementTurn`: `angles.heading` plus `delta`, and `moved` 1 |
| the bank and the pitch (`ApplyBankAndPitch`) | TILT | `UnitsOnMovementTilt` |
| the angles of a carried unit (`UpdatePosition`) | ANGLES | `UnitsOnMovementAngles` |
| a step inside the cell (`UpdatePosition`) | MOVE | `UnitsOnMovementMove`: `pos` and `moved` 1 |
| `moved` marked or cleared (`UpdatePosition`) | MOVED | `UnitsOnMovementMoved` |
| a step into another cell or flight mode (`UpdatePosition`) | CHANGE_CELL | `UnitsOnMovementChangeCell`: `pos`, `cell` and `mode` |
| the move rate (`UpdateMoveRate`) | MOVE_RATE | `UnitsOnMovementMoveRate`: bits 2 and 3 of `flags` |
| a carried unit follows its carrier's piece (`UpdatePosition`): `units.SetUnitPosition` | SET_POSITION | `UnitsOnMovementSetPosition`: `units.SetUnitPosition(pos, mode)` |
| `units.SetStateBits` (`motion.cpp`) | SET_STATE_BITS | `UnitsOnMovementSetStateBits`: `units.SetStateBits` |
| `units.UpdateSfxOccupy` (the unit's script is told whether it is in the water, on the ground or in the air) | SFX_OCCUPY | `UnitsOnMovementSfxOccupy`: `units.UpdateSfxOccupy`, the header alone |
| `scripts.StartScript` (StartMoving, StopMoving, MoveRate1 to 3) | START_SCRIPT | `ScriptsOnMovementStartScript`: starts the script `MovementStartScript::script` names, with no callback |
| `MapPublishUnit` (REMOVE_UNIT, ADD_UNIT) and `LosRadarPublishUpdateUnit` (`UpdatePosition`) | map's REMOVE_UNIT and ADD_UNIT, los_radar's UPDATE_UNIT | map's and los_radar's `Apply`, as before, published from `SCHED_MOVEMENT` |

The handlers are the adapters of units and scripts (`src/seams/units_world.cpp`, `src/seams/scripts_world.cpp`); since
#986 they publish units' and scripts' events, and the slots the third column names left the seams.

**Which `Unit` fields movement writes, and whether each is an effect or movement's view state.** The rule of the issue:
a field with another writer can't be movement's view state, as map's bucket fields are in the same record
(`docs/map-interface.md`, section 3). Units writes `pos`, the angles, `mode`, `moved` and `flags` too (in
`SetUnitPosition`, `ReadUnitState`, `CreateUnit` and a transport's `ApplyAttachUnit`: `src/units/units.cpp` writes
`mode` and `moved`, `unit_position.cpp` writes `pos`, `cell`, `mode`, `moved` and the angles, and `SetUnitPosition`
writes `pos` and `cell`):

| Field of `Unit` | Whose | Written how |
|---|---|---|
| `pos`, `yFraction`, `zFraction` (the position words) | units' record | by the effects MOVE, CHANGE_CELL and SET_POSITION; units writes it too |
| `angles` (`heading`, `pitch`, bank) | units' record | by TURN, TILT and ANGLES, and by orders' SET_ANGLES; units writes it too |
| `cell` | units' record | by CHANGE_CELL; units' `SetUnitPosition` and `CreateUnit` write it too |
| `mode` | units' record | by CHANGE_CELL; units writes it too |
| `moved` | units' record | by TURN, MOVE and MOVED; units writes it too |
| `flags` (the move rate, bits 2 and 3; the state bits) | units' record | by MOVE_RATE and SET_STATE_BITS; units writes it too |

None is movement's view state, so each is an effect, and the routes are the ones of section 4. The motion record
(`UnitMotion`) is movement's, and every write to it, units' and map's included, is movement's event now, but for the path lock stamp, which map writes itself (section 2).

**Writes movement keeps.** Movement writes `UnitMotion` (its velocity, turn, speed, mode, flag, `lastMoveTick`
and goal; not `pathLockStamp`, which is map's) and the goals and targets (section 5), in its own events and runs, and the claim tick of the
goal in `ApplyPathEvent`.

## 4. Views and routes

**What movement reads:** a unit's position, cell, heading, flags, type, carrier, footprint and owner (units' record,
and its own writes through units' handlers); the unit definition's movement fields (`UnitDefMoveView`); the carrier's
motion (`CarrierMotionView`); the player's type and activity (`PlayerTypeView`); `Game::ticks`, `gravity` and `seaLevel`;
the cells' heights and whether a footprint can be placed (map's `GetGroundHeight` and the service
`ScheduleMapCanPlaceUnitFootprint`); and a piece's position and angles (scripts' `GetPiecePosition` and
`GetPieceAngles`), as of each module's last run.

**What others read of movement:**
- **Views, in the state block, hashed:** the motion record and the goals (section 5), read by weapons (`UnitMotion::speed`,
  `velocity`), orders (`UnitMotion::flags`, `velocity`, in the VTOL orders), targeting (`velocity`), units (`speed`,
  `flags`, `lastMoveTick`, `PathOrder::pos`) and pathfinding (the goal's areas, by offset).
- **Queries:** `UnitTarget` and `TargetType` (orders), `GetMotionState` (units, a save) and `HasNetState` (the network).

**As of movement's last run.** By #952's rule a query needs a delivery first only when an event that can be pending
writes what it reads. Every route into movement that is published inside the tick is synchronous, and the three queued
ones (section 8) are published only outside it, so no event is pending when a query runs. `UnitTarget` reads the goal's
target, which SET_TARGET writes at its publisher's statement (the reason for that route's mode, below), and `TargetType`
the class of a target that the synchronous `New*` slots built. `GetMotionState` reads the motion record for a saved game,
outside the tick, after LOAD has been delivered, and `HasNetState` reads the goal's net state, which no event writes.

**The routes.** Every route, with its reason. The routes new in this change cite the gate (section 8) or #952's rule.

| Route | Now | Why |
|---|---|---|
| units NEW, to movement | synchronous | The gate (section 8). |
| units DESTROY, to movement | synchronous | #952's rule. Queued it stayed equal on the tier, but a unit can be killed inside its own update (`SelfDestructOrder` publishes a DAMAGE of 30000 on itself in `RunOrders`, and `ApplyUnitDeath` publishes DESTROY), and `SchedUnitsPhase` then tests `u->motion != 0`, delivers movement's inbox and calls `UpdateMotion(u)`, which would take the pointer the delivered DESTROY has just cleared. |
| units LOAD, to movement | queued | Published only by `LoadUnitMotion` (`unit_save.cpp`), which `LoadUnit` calls from `LoadUnits` (`settings_campaign.cpp`) while a saved game loads, outside the tick, where `SchedPublish` delivers at once (#970). Queued, with COMMAND, QUEUE_BUILD and (in 3.0) PLACED, it stayed equal (section 8). |
| units SET_MODE, to movement | synchronous | #952's rule. Queued it stayed equal on the tier (which has no transport), but `ApplyAttachUnit` publishes it when a unit is attached or detached, and `PrepVtolClimb` and the VTOL orders that follow it read `unit->motion->flags & 3`, the mode bits, in the statements after their ATTACH_TO_PIECE, before movement's inbox is delivered ahead of `UpdateMotion` (`vtol_orders.cpp`: `PrepVtolClimb` and the cases of lines 618, 1059, 1645 and 1894). |
| units SET_SPEED, to movement | synchronous | #952's rule. Queued it stayed equal on the tier, which has no network path: `ReadUnitState` publishes it in the net phase, and the next `ReceiveUnitStates` of the same phase calls `UpdateMotion` directly for every unit of its player (`TODO(#948)`), without a delivery, and `UpdateMotion` reads the speed. The same argument as the engine's REMOTE_DAMAGE row of `docs/weapons-interface.md`. |
| map PLACED, to movement | removed in 4.0 | Queued in 3.0: #952's rule found no read, as it wrote `UnitMotion::pathLockStamp` (the tick), which only `GetMotionState` (the saved game, outside the tick) and the trace's hash, taken after a phase boundary, read; map's `AddUnitToMap`, its refresh and `spatialBucket` code did not read it. Since 4.0 map writes the stamp itself (`docs/map-interface.md`, section 3), so there is no event. |
| orders SET_TARGET, to movement | synchronous | #952's rule. Queued it stayed equal on the tier, but an order can set its target and be deleted in the same `RunOrders` (or `AddOrder`'s pruning), and `Order::~Order` reads `UnitTarget` (`order_queue.cpp`) to decide whether the goal still steers to its own target: a pending SET_TARGET leaves the old target and the wrong branch. |
| orders DESTROY_TARGET, to movement | synchronous | #952's rule. Queued it stayed equal on the tier, but it frees the target's block in the sim heap, and the orders that run after it in the same `RunOrders` loop and in `RunSecondaryOrders` build new targets through the `New*` slots, whose allocations read the heap's free list: queued, they would get other offsets, which the hashed orders keep. |
| orders SET_FLIGHT_MODE, to movement | synchronous | #952's rule. Queued it stayed equal on the tier, but its handler (`SetFlightMode`) zeroes the motion's speed and velocity and publishes TILT and SET_STATE_BITS, and `RunOrders` goes on, in the same call, to the unit's next order, which reads `activateFlags` (`unit_orders.cpp`, the build orders) and the unit's pieces through scripts before movement's delivery. |
| orders SET_ALTITUDE, to movement | queued | #952's rule finds no read: it writes a target's altitude, read by the goal in `UpdateMotion` (after movement's delivery) and not by any statement of `src/orders/` or `src/units/`. |
| orders SET_APPROACH_RADIUS, to movement | queued | #952's rule finds no read: it writes a target's radius, read by the goal in `UpdateMotion` and by pathfinding's `Submit` through the goal's areas, called from that goal, both after movement's delivery. |
| orders SET_HEADING, to movement | queued | #952's rule finds no read: it writes a target's heading, read by the goal in `UpdateMotion`, after movement's delivery. |
| movement ORDER_FLAGS, to orders (`OrdersOnMovementOrderFlags`) | synchronous | The gate (section 8). |
| movement TURN, to units | synchronous | The gate (section 8). |
| movement TILT, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but `SetFlightMode` publishes it from SET_FLIGHT_MODE's handler, inside `RunOrders`, and the orders that follow in the same call read the unit's pieces through scripts' piece queries (`GetNanoPiecePosition`), which read the bank and pitch it writes, before units' delivery after `UpdateMotion`. |
| movement ANGLES, to units | synchronous | Since #986, because a script or a units query can read what it writes before units' next delivery (`docs/units-interface.md`, section 4). Before #986 it was queued, because #952's rule found no read in the publisher's code: it is published in the carried branch of `UpdatePosition`, after SET_POSITION, and the statement after it is MOVED; any later synchronous publish to units delivers units' inbox first, and no statement between it and that delivery reads the angles. |
| movement MOVE, to units | synchronous | The gate (section 8): the traces were equal, the sim event digest diverged. |
| movement MOVED, to units | queued | #952's rule finds no read: it writes `Unit::moved`, which no function of `src/` reads (it is written at five sites, and the scheduler hands `UpdateUnitTail` its own `moved`). |
| movement CHANGE_CELL, to units | synchronous | The gate (section 8). |
| movement MOVE_RATE, to units | queued | #952's rule finds no read: it writes bits 2 and 3 of `Unit::flags`, which `UpdateMoveRate` reads before it publishes, and economy and the UI read later (the players phase, after a phase boundary). |
| movement SET_POSITION, to units | synchronous | Since #986, because a script or a units query can read what it writes before units' next delivery (`docs/units-interface.md`, section 4). Before #986 it was queued, because #952's rule found no read in the publisher's code: it is published in the carried branch of `UpdatePosition`, whose next statements read the carrier's piece angles and motion, not the unit's position; any later synchronous publish to units delivers units' inbox first, and SFX_OCCUPY's handler reads `yWhole` after that delivery. |
| movement SET_STATE_BITS, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but `SetFlightMode` publishes it from SET_FLIGHT_MODE's handler, inside `RunOrders`, and the build orders that can follow in the same call read `activateFlags & 1` (`unit_orders.cpp`), which it writes, before units' delivery after `UpdateMotion`. |
| movement SFX_OCCUPY, to units | queued | #952's rule finds no read: it is the last publish of `UpdateMotion`, and its handler (`UpdateSfxOccupy`) reads `yWhole` and `flags & 3` and starts `setSFXoccupy` only when the state changed; nothing between it and units' delivery before the tail reads what it writes. |
| movement START_SCRIPT, to scripts | synchronous | The gate (section 8). |
| movement ADD_UNIT, to map | synchronous | As before (the gate, `docs/map-interface.md`, section 8); published from `SCHED_MOVEMENT` now, `SCHED_ORDERS` before. |
| movement REMOVE_UNIT, to map | synchronous | As before (`docs/map-interface.md`, section 8); published from `SCHED_MOVEMENT` now. |
| movement UPDATE_UNIT, to los_radar | synchronous | As before (`docs/los-radar-interface.md`, section 8); published from `SCHED_MOVEMENT` now. |

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv` has 64 rows of class `immediate` with movement as
writer or reader and the other side not orders (the rows between the two are in `docs/orders-interface.md`, section 4,
as are the rows of orders with the other modules). The writes of units' adapters of movement's effects (position, angles,
cell, mode, moved, flags) are units' in them. Each pair is served as follows:

| Writer to reader | Rows | Items | Phase | How it is served |
|---|---|---|---|---|
| economy to movement | 3 | `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` (the `Player` records' resources) | units | A view: economy's view state, as of its last apply, read directly (`docs/economy-interface.md`, section 4); the access log attributes the read to movement's call chain in the unit's update. |
| map to movement | 2 | `Unit::flags`, `Unit::spatialBucket` | units | A view: map's bucket fields and the unit's flags, written by map's own code, as of the writer's last run. |
| movement to los_radar | 4 | `Unit::flags`, `pos`, `yFraction`, `zFraction` | units | UPDATE_UNIT, a synchronous route (above), published after the unit's position effect, so los_radar reads the position the effect wrote, at movement's statement. |
| movement to map | 4 | `Unit::cell`, `flags`, `pos`, `zFraction` | units | REMOVE_UNIT and ADD_UNIT, synchronous routes, published around CHANGE_CELL at the statement 3.1c made them. |
| movement to scripts | 7 | `Unit::field_64`, `flags`, `heading`, `pitch`, `pos`, `yFraction`, `zFraction` | units | A view of the unit record, written by movement's effects through units' handlers in the unit's update; scripts run in units' body, before orders and movement in the unit's update (3.1c's order), so scripts read the previous tick's. |
| movement to targeting | 4 | `Unit::flags`, `pos`, `zFraction`, `UnitMotion::velocity` | units | A view, read by targeting's slots called from the unit's update, as of movement's last run. |
| movement to units | 12 | `PathOrder::pos`, `Unit::cell`, `field_64`, `flags`, `heading`, `pitch`, `pos`, `yFraction`, `zFraction`, `UnitMotion::flags`, `lastMoveTick`, `speed` | units | Units' handlers of movement's effects write the unit record, and units' own code (`UpdateUnitTail`: `UpdateUnitHeight`, the network's state) reads it after movement's run in the unit's update; the motion's `speed` and `lastMoveTick` are a view of movement's record. |
| movement to weapons | 8 | `Unit::flags`, `heading`, `pitch`, `pos`, `yFraction`, `zFraction`, `UnitMotion::speed`, `velocity` | units | A view: the unit's motion, written by `UpdateMotion` and read by `UpdateUnitWeapons` in the unit's update, in 3.1c's order (head, weapons, body, orders, movement, tail), so weapons reads the previous tick's (`docs/weapons-interface.md`, section 4). |
| scripts to movement | 1 | `Unit::script` | units | A view of the unit's script, read by movement's script queries, as of scripts' last run. |
| units to movement | 19 | `PathOrder::ref`, `Unit::carrier`, `cell`, `def`, `flags`, `footprint`, `heading`, `motion`, `pitch`, `pos`, `script`, `transportPiece`, `yFraction`, `yWhole`, `zFraction`, `UnitMotion::flags`, and the `Player` records' resources | units | A view of the unit record, written by units' own code and its adapters, read by movement in `UpdateMotion` in the unit's update, as of the writer's last statement; units' writes to the motion record itself are movement's events (section 2). |

How a pair is served, in the three ways of the table: **a view** is read directly from the state block as of the writer's
last run; **in 3.1c's order within a unit's update** means the scheduler's order is the order of 3.1c's `UpdateUnit`; **a
synchronous route** is the route above that is made synchronous because a later read needs its write. Every effect
route inside the tick is synchronous, each for the reason in the table of routes above.

## 5. State and the trace

Movement keeps no private region. Its state is sim state in the shared block, and since format 8 of the trace (#1024)
what only movement reads is not hashed: its `WriteState` dumps it in record 53, `movementState`. The heap rows of
`data/state_classes.csv` (`UnitMotion::UnitMotion: new ...`, the goals, and the `New*Native` and `Load*Native` rows) name
movement as the class's owner. The trace hashes the motions in section 21 and the order effects in section 22
(`docs/trace-format.md`, sections 8.10 and 8.11).

| Movement's state | Where | Read by | Hashed |
|---|---|---|---|
| `UnitMotion::velocity` | the unit's heap object (section 21) | movement, orders, targeting, weapons | yes |
| `UnitMotion::speed` | the same | movement, units (`ReadUnitState`, `WriteUnitState`), weapons (by view) | yes: units reads it in the unit's network state, and writes it by SET_SPEED |
| `UnitMotion::mode`, `flag` | the same | movement; units (`ApplyAttachUnit`, by SET_MODE) | yes |
| `UnitMotion::flags`, `field_22` | the same | movement, orders (the VTOL orders) | yes |
| `UnitMotion::pathLockStamp` | the same | map (the issue #1019 decision): `AddUnitToMap` sets it to the tick the unit was placed, its pass map refreshes stamp it again, and LOAD_PATH_LOCK_STAMP sets it from a saved game; movement does not write it | yes: it is map's field, kept in movement's motion record, as map keeps its bucket fields in units' record |
| `UnitMotion::movementClass` (and the planner view of its first field) | the same | movement, pathfinding (by offset) | yes |
| `UnitMotion::lastMoveTick` | the same | movement, units (the hover bob's damping reads the unit's age) | yes: it came back into the hash in format 8 |
| The goal pointer (`UnitMotion::player` / `obj`) | the same | movement, network, orders (through `UnitTarget`), pathfinding (by offset) | no: the pointer is a presence byte; the goal's fields left (below) |
| `UnitMotion::p2` and `turn` | the same | movement | no: record 53 |
| The goals' fields (`PatrolGoal`, `PackedGoal`, `LiteGoal`, `PackedPosGoal`, `AiSearchGoal`), `PathGoal::target` included | the goal's heap object | movement; orders gets the target only as the answer of the `UnitTarget` query; presentation's debug overlay (behind a debug toggle) | no: record 53 |
| The goal areas (`PathGoalArea`) | filled by the goals | movement; pathfinding, through `PathGoalCells`, `PathGoalContains` and `PathGoalDistance` | no |
| The targets' fields (`PathOrder`, `AirManeuverOrder`, `ApproachRadius`, `RingApproach`, `PointMarker`) but the unit reference | the order effect (section 22) | movement; `OrderFx::source` is the order | no: dumped under `orderFx[id].*` |
| The unit reference of a move target (`PathOrder::ref`, a `PathOrderAttach`) | the target | movement, units (the list, `UnitRefGet`, `UnitRefSetValue`), orders (`Order::link`, `Matches`) | yes: it is units' link (section 7) |

What movement writes of others' state, from `uv run tools/coupling.py --summary movement`: the unit record (`pos`,
`angles`, `flags`, `mode`, `moved`, units') by effects, `Order::subFlags` (orders') by ORDER_FLAGS, `PathRequest`
fields (pathfinding's) and the `AiSearchGoal` repath claim. It reads `Game::ticks`, `Game::gravity`, `Game::seaLevel`,
`Game::overflowBucket`, `Unit::motion`, `Unit::def` and `Unit::cell` among others, and the fields of the unit
definition, as of the last write of the owner.

**What left the hash,** in format 8: the motion's `p2` and `turn`; every field of the goals; every field of the
targets of section 22 but the unit reference. Section 21 hashes the motion without them and no goal, and section 22
hashes, for a move target, only the link of its unit reference. The debug overlay of the goals is presentation, and
draws only behind a debug toggle, so nothing is kept in the hash for it.

**Record 53, `movementState`** (dumped, never hashed; `docs/trace-format.md`, section 8.37). For each live unit with
a motion, one record `movementState.units[i]` holds `motion.p2.x`, `.y`, `.z`, `motion.turn`, `goal.class` (0 none, 1
patrol, 2 AI search, 3 packed, 4 lite, 5 packed position) and, by class, the goal's fields as `goal.<field>` (the
points of a patrol or AI search goal as `goal.points`). The targets of section 22 are not in the record: the dump
holds every one of them as `orderFx[id].*`, as before, but hashes only a move target's link. The reference build writes
the same lines itself.

**Where a wrong value shows.** With a wrong goal point, orders-move-attack first diverges at tick 25, phase units, `units[1].heading` (0xaa6e against 0xaa5c): at tick 20 both points of the commander's AI search goal were moved 8 along x. A wrong `p2` or `turn` changes where the
unit moves next, and diverges through its hashed position or heading.

**Reads through views.** Other subsystems read motion records by offset through views the access log names by their own
type, so `UnitMotion::...` does not show them: weapons reads the word at `+0x20` (`UnitMotion::speed`) as a range, a
parameter and an id and reads `+8` as a size; pathfinding's planner view reads the first two fields. A change
that moves `speed`, `velocity`, `movementClass` or the goal pointer out of the unit's record has to give these readers a
slot or keep the offsets. `AddUnitToMap`'s write of `pathLockStamp` (`+0x26`) was the event PLACED in 3.0; since 4.0 it is
map's own write of its field, so no other subsystem writes the motion record by offset.

**The trace dumps movement's events** in section 49 (`docs/trace-format.md`, section 8.33), after orders' section 48
(`docs/orders-interface.md`, section 5), and hashes none of them. Each record is an event with the outcome `Apply`
gave, or an effect movement published (outcome 1), in movement's order. Sections 50 and 51 are units' and
scripts' events (`docs/trace-format.md`, sections 8.34 and 8.35).

`privatestate.py --check` and `seamlink.py --pilot-seams` check the seam: no other simulation subsystem writes movement's
state or calls its entry points, and no query slot writes state.

**At load**: movement restores each unit's motion through the event LOAD (`MovementSavedState`), published by units' `LoadUnitMotion` (`unit_save.cpp`) from the unit's `u%04xmob` box, only when the unit's record has a motion, and each order's target (path order, air manoeuvre, approach radius, ring approach, point marker) through the slot `LoadTarget`, which orders' `Order` constructor calls. The path lock stamp read from the same box is map's: LOAD_PATH_LOCK_STAMP sets it.

## 6. Random draws and sim events

Movement's own code (`src/movement/`) makes no draw from the game logic stream and records no sim event: a draw or a
record falls in what its effects and publishes start (a script's start, map's and los_radar's updates). The class `ordered` of `data/dependencies.csv` has no row with movement as
writer or reader, because the access logs predate movement's module (the rows of the units pass are attributed to
orders and units): the draws that fall inside a unit's update around movement's step are the ordered rows of
`docs/orders-interface.md`, section 6.

The scheduler keeps the order of runs in a unit's update: units' head, weapons' step, units' body, orders' two runs, then
`UpdateMotion`, then units' tail. A route that carries a script's start (START_SCRIPT, whose script can draw and record)
or map's and los_radar's updates (which record sim events) is made synchronous when queueing moves them, and every route of section 4 inside the tick is synchronous: START_SCRIPT
diverged queued, and map's and los_radar's routes are as before (section 8). The three publishes in `UpdatePosition` keep 3.1c's order: map's REMOVE_UNIT, the position effect
(CHANGE_CELL), map's ADD_UNIT, the MOVED effect, los_radar's UPDATE_UNIT.

## 7. What stays inside the default

3.1c's types and limits are the default's, not the interface's: the goal classes and their virtual calls, the layout of
`UnitMotion`, the unit definition's movement fields, the saved game's motion and target records, and the network's goal
state. An event carries a unit and a target as their block offsets, and the motion's state as a plain
`MovementSavedState`: no pointers, no `Unit*`, `UnitMotion*` or `PathOrder*` in an event. The `New*` and `Load*` slots
build objects in the sim block (`new (kSim)`) and give the caller their block offset, a handle the caller keeps and
never reads through; the goal's virtual destructor frees its goal (section 2). `ApplyPathEvent` is the engine's
delivery (section 2).

### What moved

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
  are units' unit-list code (`Unit::head`, `UnitRef::Unlink`). Movement and orders reach it through units' slots
  `UnitRefInit`, `UnitRefSetUnit`, `UnitRefGet` and `UnitRefSetValue` (units 5.0: the inline `Get` and `SetValue` became slots, so
  that orders' calls pass `coupling --seams`), and through the existing `UnitRefUnlink`; all take a `UnitRef*`. Where the caller built the
  reference in place (`link(o, 0)`), the constructor now runs empty and `UnitRefInit` fills it; its destructor body
  moved into `UnitRef::Unlink`. The header moved from `src/orders/` to `src/units/`.
- **`MovementClass`** (`src/units/movement_class.cpp`): the movement class definition is units' and the pass maps are map's `PassMap` (`src/map/pass_map.cpp`,
  `docs/map-interface.md`), not the motion's. `UnitMotion::movementClass` only points at one.
- **Pathfinding.** `src/ai/pathfind*.cpp` stays pathfinding's. It reads a goal's areas through `PathGoalCells`,
  `PathGoalContains` and `PathGoalDistance` in `src/util/path_goal.cpp` (platform), not through a slot.
- **`UpdateUnitHeight`** and the unit's position code stay units'. Movement writes `Unit::pos` and `Unit::angles`
  through `UnitMotion::UpdatePosition` and its steering as effects (section 3), and `SetUnitPosition` is the adapter's call.
- **Orders' reads of motion fields.** Orders still reads `UnitMotion::flags`, `velocity` and `field_22` in the VTOL
  orders, as it did (section 5).
- **`RegisterVtolOrders`** is orders', not movement's: it moved from `src/network/net_stats.cpp` to
  `src/orders/unit_orders.cpp` (`docs/orders-interface.md`, section 3).

### The rules of `data/subsystems.csv`

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
| `PathOrderAttach::PathOrderAttach` | `src/units/unit_targets.cpp` | units (`name:Order`, override `units`) | The unit reference stays units' (section 7) |
| `PathOrderAttach::SetUnit` | `src/units/unit_targets.cpp` | units (`name:Order`, override `units`) | The same |
| `MakeVec3` | `src/movement/packed_goal.cpp` | platform (`file:src/movement/`, override `platform`) | A vector helper, as in every other subsystem |

The rows that moved with the files carry the same rule under their new file: the `UnitMotion` rows
(`src/movement/motion.cpp`) and the order target rows (`src/movement/order_targets.cpp`) went from `orders`,
`file:src/orders/` to `movement`, `file:src/movement/`; the rows of `PackedGoal`, `LiteGoal` and `PackedPosGoal` went
from the players' `victory.cpp` rule to `file:src/movement/`. No `name:` rule is new for movement.

### What is left

After this change:
- **Units and scripts (#986), done:** the adapters (`src/seams/units_world.cpp`, `src/seams/scripts_world.cpp`) are
  handlers of their inboxes. Units' slots `SetUnitPosition`, `SetStateBits` and `UpdateSfxOccupy` and scripts'
  `StartScript` left the seams: `UnitsApply` calls the first three, and the start is scripts' event START.
- **The network's writers (#948):** `ReceiveUnitStates` calls `ReadNetState` and `UpdateMotion` directly (`TODO(#948)`),
  and `NetSendUnitStates` calls the net-state slots.
- **The state** is in the shared block, and only what other subsystems read stays hashed (section 5).
- **The steering reads of orders** (the VTOL orders read `UnitMotion::flags`, `velocity`, `field_22` by view) stay views.

## 8. What the gate decided

Every route into and out of movement that is new in this change was queued first, each on its own, with every other
route as in `src/game/routes.cpp` at the end of this change (the tier, with `--no-cache`, on the modern build; the gate
line of an equal run names the commit it ran at, 1a3180607, c5c394dfe, 0274316fe or 1f78163f3, since commits landed
meanwhile, and the table of routes was the same in each). A route whose queueing diverged stays synchronous. The
divergences are against the reference, with the first tick, phase and field of the comparison's report. Where the report's
first field is an `aiState` squad record that one side lacks, that is an artifact of the comparison, and the row gives the
tick and phase and the scenario's first `DIFF` line instead. A route that stayed equal is still synchronous when it can be
pending while a later read in the same phase needs what it writes (#952's rule, section 4); the routes it
finds no such read for are queued together (section 4), and the gate of this change ran with them queued. Movement's LOAD stays queued, since it is published only while a saved game loads; with map's PLACED (removed in 4.0) and
the engine's COMMAND and QUEUE_BUILD queued together, the tier was equal (38 of 38 scenarios at fdfcd3cae033).

| Route queued | Tier | First divergence |
|---|---|---|
| units NEW | diverged | orders-air, tick 1025, world phase (the once-per-tick tables, hashed after `UpdateMeteors`): stored eca9728ab3cc3df1, got f8e6f6d49f962c8f. |
| units DESTROY | equal | none (synchronous by #952's rule, section 4) |
| units LOAD | equal | none (queued, section 4) |
| units SET_MODE | equal | none (synchronous by #952's rule, section 4) |
| units SET_SPEED | equal | none (synchronous by #952's rule, section 4; the tier has no network path) |
| map PLACED (removed in 4.0) | equal | none (queued in 3.0, section 4: #952's rule found no read) |
| orders SET_TARGET | equal | none (synchronous by #952's rule, section 4) |
| orders DESTROY_TARGET | equal | none (synchronous by #952's rule, section 4) |
| orders SET_FLIGHT_MODE | equal | none (synchronous by #952's rule, section 4) |
| orders SET_ALTITUDE | equal | none (queued, section 4: #952's rule finds no read) |
| orders SET_APPROACH_RADIUS | equal | none (queued, section 4: #952's rule finds no read) |
| orders SET_HEADING | equal | none (queued, section 4: #952's rule finds no read) |
| movement ORDER_FLAGS | diverged | orders-air, tick 940, world phase: stored 394b77cd380e1e61, got a3baa9ecf36255af. |
| movement TURN | diverged | mission-radius, tick 1, units phase (`UpdateAllUnits`, 0x48ad30): `units[9].pos.x` (reference 0x070000a1, queued 0x07000081). |
| movement TILT | equal | none (synchronous by #952's rule, section 4) |
| movement ANGLES | equal | none (queued, section 4: #952's rule finds no read; synchronous since #986, `docs/units-interface.md`, section 4) |
| movement MOVE | traces equal, events diverged | mission-capture, tick 1, units phase: the sim event digest (`1 units 36`, stored fb5750c1bbf846e8, got 555fb8667eecd559); also mission-passes-x, tick 5, and mission-radius, tick 1. |
| movement MOVED | equal | none (queued, section 4: #952's rule finds no read) |
| movement CHANGE_CELL | diverged | mission-radius, tick 20, world phase: `cells[3408]` (reference first byte 0x00, queued 0x09), 328 fields in all: the cell records of a unit's old and new footprint (map's ADD_UNIT and REMOVE_UNIT, synchronous, read the unit's cell, which CHANGE_CELL writes). |
| movement MOVE_RATE | equal | none (queued, section 4: #952's rule finds no read) |
| movement SET_POSITION | equal | none (queued, section 4: #952's rule finds no read; synchronous since #986, `docs/units-interface.md`, section 4) |
| movement SET_STATE_BITS | equal | none (synchronous by #952's rule, section 4) |
| movement SFX_OCCUPY | equal | none (queued, section 4: #952's rule finds no read) |
| movement START_SCRIPT | diverged | mission-capture, tick 181, world phase: stored ea630c9c9a33e44c, got e83cde9c9860c414. |
| movement ADD_UNIT, REMOVE_UNIT to map; UPDATE_UNIT to los_radar | unchanged | synchronous as before (`docs/map-interface.md` and `docs/los-radar-interface.md`, section 8); published from `SCHED_MOVEMENT` now |

**Inbox depths.** A build that counted every publish and delivery played all 89 scenarios (87 wrote their counts;
console-ilose and console-iwin exit before the dump), with native modules, and every scenario stayed equal. Four routes
into movement were queued inside the tick in 3.0: map's PLACED (removed in 4.0) and orders' SET_ALTITUDE, SET_APPROACH_RADIUS and SET_HEADING.
Movement's inbox held at most 3 events (long-greenhaven-8p, tick 48130), and `SchedEndTick` never found one. Units' LOAD
is published only while a saved game loads, outside the tick, where each event is delivered at once (#970). Movement's
five queued effects go to units' inbox, whose deepest was 5 events; SFX_OCCUPY, the busiest, was queued 7,268,389 times.
Movement took at most 147 deliveries in one tick (edges-economy-targeting, tick 245). A synchronous delivery into
movement runs inside another delivery in two places the code shows: `Apply` of orders' STOP calls `DeleteOrders`, and
`Order::~Order` publishes SET_TARGET and DESTROY_TARGET (`order_queue.cpp`), so movement's `Apply` runs inside orders'
`Apply`; and `ApplyAttachUnit`, the handler of orders' ATTACH_TO_PIECE, publishes SET_MODE. The network's
`ReceiveUnitStates` calls `UpdateMotion` directly, without a delivery (`TODO(#948)`).
