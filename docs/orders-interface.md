# The orders interface

Orders sits behind `interfaces/orders.h` (version 5.0), the ninth module on the scheduler after economy
(`docs/economy-interface.md`), features (`docs/features-interface.md`), targeting (`docs/targeting-interface.md`),
los_radar (`docs/los-radar-interface.md`), map (`docs/map-interface.md`), players (`docs/players-interface.md`), ai
(`docs/ai-interface.md`) and weapons (`docs/weapons-interface.md`), and the tenth with movement
(`docs/movement-interface.md`), which goes on the scheduler in the same change (#985). It is the order lists of the
units: the order state machines of every kind of order, the queue each unit keeps, the commands a player gives, and the
return fire and paralysis a unit takes up when it is hit. 3.1c's code is its default implementation. This page is the
inventory the interface was cut from:
- what other subsystems did to orders' state in 3.1c, and what each became;
- what orders did to others, and what each became;
- what orders reads, and what others read of it;
- how the order of random draws and sim events is kept;
- what the trace hashes, and what it only dumps.

The movement half of the change (the motion record, the goals and the targets, and the scheduler's `UpdateMotion` step)
is on `docs/movement-interface.md`; this page refers to it where the two would repeat.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, events published
outside the tick delivered at once (#970), and events as plain data. This issue changes none of its rules. Event
kinds are unique across interfaces: economy uses 0 to 4 and 16 to 18, features 32 to 35 and 48 to 51, wind_meteors 64
to 67 and 80, targeting 96 to 100 and 112, los_radar 128 to 136, map 160 to 168 and 176, players 192 to 196 and 208 to
210, ai 224, 226 to 229 and 240, 241, 243, 245 and 246, weapons 256 to 267, 269 and 272 to 283 (its range is 256 to
287), orders 288 to 295 and 304 to 317 (its range is 288 to 319) and movement 320 to 331 and 336 to 347 (its range is
320 to 351).

Two rulings of the human shape this change:
- **Movement owns the unit record's motion fields** (2026-11-18). What orders tells a unit's motion, it tells movement
  (`docs/movement-interface.md`, sections 2 and 3).
- **No slot passes memory ownership across a seam, and major bumps are cheap** (2026-11-19). Orders 5.0 and movement
  3.0 take pointers out of their events and slots: a unit, an order, a target or a reference is its block offset.

Orders' place in it:
- **Events into orders:** ADD, QUEUE_BUILD, STOP, BE_CARRIED, STATUS_FLAGS, ATTACKED, PARALYZE and COMMAND, published by
  units, ai, weapons, movement and the engine (the UI, the console and the harness). Orders' handler (`OrdersInbox`,
  `src/seams/orders_world.cpp`) passes each to `Apply` and records it with its outcome in the trace's dump (section
  5). Ai's effects ADD_ORDER, QUEUE_BUILD and STOP, weapons' ATTACKED and PARALYZE and movement's ORDER_FLAGS reach it
  through `OrdersOnAiAddOrder`, `OrdersOnAiQueueBuild`, `OrdersOnAiStop`, `OrdersOnWeaponsAttacked`,
  `OrdersOnWeaponsParalyze` and `OrdersOnMovementOrderFlags` (the same file), which build the event and hand it to the
  same handler.
- **Routes:** each route's reason is in section 4.
- **Runs:** `RunOrders` and `RunSecondaryOrders`, called by the scheduler as steps of the units pass, after
  `UpdateUnitBody` and before movement's `UpdateMotion`, each with orders' inbox delivered first (section 6).
- **Synchronous calls out:** the calls that also return an answer stay synchronous slots of their owners, with a
  `TODO(#986)` in the code (section 3): `units.CreateUnit`, `AddBuildProgress` and `AddRepairProgress`,
  `scripts.GetNanoPiecePosition` and `QueryScript`. Movement's `New*` slots return the target they build and are
  synchronous by their contract.
- **Queries:** `GetOrderFlags`, `GetOrderType` and `MissionTypeFromName`, as of orders' last apply.
- **Effects out of orders:** thirteen effects to units (SET_STATE_BITS, SET_POSITION, ATTACH_TO_PIECE,
  FINISH_CONSTRUCTION, BUILD_DECAY, REPAIR, UNLINK, BUSY, SET_FLAGS, CLEAR_NET_DIRTY, SET_STANCE, SET_ANGLES and
  SET_BUILD_STATE) and one to scripts (START_SCRIPT, in three forms), besides the events to movement, weapons,
  targeting, features, economy and players that orders published before. Units and scripts are not on the model yet
  (#986), so their handlers (`src/units/orders_effects.cpp`, `src/scripts/orders_effects.cpp`) are adapters that hold
  3.1c's calls and writes.
- **Module ids:** orders keeps `SCHED_ORDERS = 3`, which its outbox already used; movement keeps `SCHED_MOVEMENT = 14`.
- **Versions:** orders 5.0 (11 slots; 4.0 had 18), movement 3.0, units 5.0 (`UpdateUnitBody` returns an int,
  `UpdateUnitTail`, `UnitRefGet` and `UnitRefSetValue` are new), services 2.1 (`HarnessOrdersEvent` and
  `HarnessMovementEvent`).

## 2. Events into orders

**Every slot of 4.0.** The 18 slots of orders 4.0, and what each became in 5.0:

| 4.0 slot | 5.0 |
|---|---|
| `AddOrder` | event: ADD, from units (a unit's initial mission, `unit_commands.cpp`) and from ai (ADD_ORDER); `Apply` calls `AddOrder` |
| (not a slot: `IssueOrCancelOrder`, the player's command) | event: COMMAND, from the engine (the UI), with the `queue` of the order's cancel rule |
| `QueueBuildOrder` | event: QUEUE_BUILD, from the engine (the build menu) and from ai (QUEUE_BUILD, by unit type name) |
| `DeleteOrders` | event: STOP, from units (`units.cpp`: a unit about to be freed) and from ai (STOP) |
| `AddBeCarriedOrder` | event: BE_CARRIED, from units (`UpdateUnitBody`, `unit_position.cpp`) |
| `OrderOrStatusFlags` | event: STATUS_FLAGS, from units (`unit_targets.cpp`, `unit.cpp`: the target a listener order follows changes or is lost) and from movement (ORDER_FLAGS, a target reports progress) |
| `IssueAttackOrder` | inside event ATTACKED: weapons' ATTACKED effect reaches `Apply`, which issues the return fire as 3.1c's adapter did (`IssueAttackOrder`, or targeting's SET_UNIT) |
| `AppendOrder`, `NewOrder` | inside event PARALYZE: the paralysis order, as `OrdersOnWeaponsParalyze` of #984 did it (the first order's `ticks` raised, else an order appended) |
| `ReattachFxToUnit` | inside the slot `LoadOrders`: the first order takes back its effects after the saved game's orders are given to the unit |
| `LoadOrder` | inside the slot `LoadOrders`, which reads the unit's orders by count |
| `SerializeOrderToSave` | inside the slot `SaveOrders`, which writes the unit's orders and returns how many |
| `RunOrders`, `RunSecondaryOrders` | runs, called by the scheduler |
| `GetOrderFlags`, `GetOrderType`, `MissionTypeFromName` | queries, unchanged |
| `RegisterAllOrderTypes`, `ClearOrderTypeTable` | set-up slots, outside the tick, unchanged |

`Apply` and `WriteState` are new (section 5). `LoadOrders` and `SaveOrders` are new as slots and replace the per-order
`LoadOrder`, `ReattachFxToUnit` and `SerializeOrderToSave`: the saved game's order list has no events, because it is
read outside the tick.

**The writers, one by one.** The tick's own writers are units, ai, weapons and movement; the writers outside the tick
are the UI, the console, the harness and the saved game's load.

| 3.1c site | 3.1c call | Event |
|---|---|---|
| units: a unit's initial mission, when a game loads (`unit_commands.cpp`, `RunInitialMission`, 14 sites) | `AddOrder` | ADD from units |
| units: a unit about to be freed (`units.cpp`) | `DeleteOrders(unit, 1)` | STOP from units, with `all` 1 |
| units: a unit that a carrier picks up (`UpdateUnitBody`, `unit_position.cpp`) | `AddBeCarriedOrder` | BE_CARRIED from units |
| units: the unit a listener order follows changes or is lost (`unit_targets.cpp`, `unit.cpp`) | `OrderOrStatusFlags` | STATUS_FLAGS from units, with the order's block offset |
| ai: its orders (`ai_effects`, #968) | `AddOrder`, `QueueBuildOrder`, `DeleteOrders` | ADD, QUEUE_BUILD, STOP through `OrdersOnAiAddOrder`, `OrdersOnAiQueueBuild` and `OrdersOnAiStop` (ai's three effects, #968) |
| weapons: a damaged unit (#984, #991) | `IssueAttackOrder`, `ai.ReactToAttack`'s return fire | ATTACKED through `OrdersOnWeaponsAttacked` |
| weapons: a paralyser hits (#984) | the paralysis order | PARALYZE through `OrdersOnWeaponsParalyze` |
| movement: a target reports the progress of its order (`order_targets.cpp`) | `OrderOrStatusFlags` | STATUS_FLAGS through `OrdersOnMovementOrderFlags` (movement's ORDER_FLAGS) |
| the UI: a command (`selection.cpp`, `control_panel.cpp`) | `IssueOrCancelOrder` | COMMAND from the engine |
| the UI: the build menu (`control_panel.cpp`) | `QueueBuildOrder` | QUEUE_BUILD from the engine |

**Writers outside the tick** (the player's commands and the saved game):
- **The UI, the console and the harness** publish COMMAND and QUEUE_BUILD from `SCHED_ENGINE` between ticks, so each
  event is delivered at once (#970).
- **A unit's initial mission** is units' ADD, published by `RunInitialMission`, which `CreateMissionUnits` calls while a
  game loads (`game_load.cpp`), before the first tick. The net phase publishes no event into orders: `net_game.cpp`
  calls none of the publish helpers.
- **A saved game's load** calls the slot `LoadOrders` for each unit from units' `LoadUnit` (`unit_save.cpp`), outside
  the tick, and publishes movement's LOAD (`docs/movement-interface.md`, section 2). `SaveOrders` is its writer's mirror.
- **The network's `ReceiveUnitStates`** (`unit_position.cpp`) calls movement's `ReadNetState` and `UpdateMotion`
  directly. It is the network's writer, and waits for #948 (parked), as #954's did: `TODO(#948)` in
  `ReceiveUnitStates` marks the `UpdateMotion` call. `ReadNetState` has no marker and is left as it was.

**Outcomes.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 48) and returned to no
one: 1 for every kind, and 0 for the events it refuses. `Apply` refuses, with outcome 0 and no change, an event smaller
than its kind's payload, an unknown kind, an event naming a null unit (all but STATUS_FLAGS, which names an order) and a
STATUS_FLAGS naming no order. ATTACKED does nothing (outcome 1) unless the attacker is a unit with a type, the
unit's player is active and of type 1 or 2, the unit can return fire, is complete, and the attacker is not an ally,
as 3.1c's adapter had it.

**Payloads.** Every event and effect starts with `OrdersEventHeader`: `kind` and `unit` (the unit it is about, a block
offset; 0 for none). Each kind has the payload of `interfaces/orders.h` (version 5.0) that starts with it, or is the
header alone. `Apply(event, size)` refuses an event smaller than its kind's payload before it reads a field.

| Kind | Payload | Fields after the header |
|---|---|---|
| ADD 288, COMMAND 295 | `OrdersAdd` | `type` (as `GetOrderType` returns it), `queue` (0 replaces the unit's orders that do not stay, 1 queues behind them), `target` (a block offset, 0 for none), `hasPos`, `pos[3]` (world coordinates with 16 fractional bits), `params[2]` |
| QUEUE_BUILD 289 | `OrdersQueueBuild` | `count` (more to queue, or, below 0, fewer), `name[32]` (a unit type, or a weapon to stockpile) |
| STOP 290 | `OrdersStop` | `all`: 1 deletes every order, 0 only those not marked to stay |
| BE_CARRIED 291 | the header alone | none |
| STATUS_FLAGS 292 | `OrdersStatusFlags` | `order` (a block offset), `flags` |
| ATTACKED 293 | `OrdersAttacked` | `attacker` (a block offset, 0 for none) |
| PARALYZE 294 | `OrdersParalyze` | `ticks` |

## 3. Effects out of orders

| 3.1c call | Effect | Subscriber's handler |
|---|---|---|
| `units.SetStateBits` | SET_STATE_BITS | `UnitsOnOrdersSetStateBits`: `units.SetStateBits(mask, set)` |
| `units.SetUnitPosition` | SET_POSITION | `UnitsOnOrdersSetPosition`: `units.SetUnitPosition(pos, mode)` |
| `units.AttachUnitToPiece` | ATTACH_TO_PIECE | `UnitsOnOrdersAttachToPiece`: `units.AttachUnitToPiece` |
| `units.FinishConstruction` | FINISH_CONSTRUCTION | `UnitsOnOrdersFinishConstruction`: `units.FinishConstruction(builder, unit)` |
| `units.ApplyUnfinishedBuildDecay` | BUILD_DECAY | `UnitsOnOrdersBuildDecay` |
| `units.AddRepairProgress`, where the caller uses no answer | REPAIR | `UnitsOnOrdersRepair`: `units.AddRepairProgress(builder, unit, amount)` |
| `units.UnitRefUnlink` | UNLINK | `UnitsOnOrdersUnlink`: `units.UnitRefUnlink` on the reference the payload names |
| the write of `Unit::workTime` for the idle animation and the build pauses | BUSY | `UnitsOnOrdersBusy`: `workTime` becomes `OrdersBusy::until` |
| the writes of `Unit::flags` under two masks | SET_FLAGS | `UnitsOnOrdersSetFlags`: `flags` is `(flags & ~clear) \| set` |
| the clearing of `Unit::netDirtyFlags` bits | CLEAR_NET_DIRTY | `UnitsOnOrdersClearNetDirty` |
| the writes of the movement stance, the fire stance and the group | SET_STANCE | `UnitsOnOrdersSetStance`: each field the `fields` bits name |
| the write of a unit's angles | SET_ANGLES | `UnitsOnOrdersSetAngles` |
| the write of `buildLeft` and `health` | SET_BUILD_STATE | `UnitsOnOrdersSetBuildState` |
| `scripts.StartScript`, `StartScriptWithArgs`, `StartScriptWithArgsByIndex` | START_SCRIPT | `ScriptsOnOrdersStartScript`: the script's start, by `OrdersStartScript::form` (by name, by name with arguments, by index with arguments); the three starts are one effect |

The handlers are the adapters of units and scripts, which hold 3.1c's writes until #986 puts units and scripts on the
scheduler. The effects kinds are 304 to 317, in the upper half of orders' block, so no second block was taken: BUSY,
SET_FLAGS, CLEAR_NET_DIRTY, SET_STANCE, SET_ANGLES and SET_BUILD_STATE are the six that go beyond the issue's list of
intents, for writes of the unit record that orders made directly.

**Payloads.**

| Kind | Payload | Fields after the header |
|---|---|---|
| SET_STATE_BITS 304 | `OrdersSetStateBits` | `mask`, `set` (1 sets the bits, 0 clears them) |
| SET_POSITION 305 | `OrdersSetPosition` | `pos[3]`, `mode` (the flight mode) |
| ATTACH_TO_PIECE 306 | `OrdersAttachToPiece` | `target` (the carrying unit, a block offset), `piece`, `mode` |
| FINISH_CONSTRUCTION 307 | `OrdersFinishConstruction` | `builder` (a block offset) |
| BUILD_DECAY 308 | `OrdersBuildDecay` | `ticks` |
| REPAIR 309 | `OrdersRepair` | `builder`, `amount` (a float) |
| UNLINK 310 | `OrdersUnlink` | `ref` (the unit reference, a block offset) |
| START_SCRIPT 311 | `OrdersStartScript` | `form` (`OrdersScriptForm`), `index`, `name[32]`, `callback` (a block offset, 0 for none), `update`, `count`, `args[4]` |
| BUSY 312 | `OrdersBusy` | `until` (a tick) |
| SET_FLAGS 313 | `OrdersSetFlags` | `clear`, `set` |
| CLEAR_NET_DIRTY 314 | `OrdersClearNetDirty` | `mask` |
| SET_STANCE 315 | `OrdersSetStance` | `fields` (`OrdersStanceField` bits: movement stance 1, fire stance 2, group 4), `moveOrder`, `fireOrder`, `group` |
| SET_ANGLES 316 | `OrdersSetAngles` | `bank`, `heading`, `pitch` |
| SET_BUILD_STATE 317 | `OrdersSetBuildState` | `buildLeft` (a float), `health` |

**Calls that also return an answer stay synchronous slots of their owners (#986).** Each is mixed: the answer is read
in the same statement, and each of them can draw from the game logic stream or record sim events, in the owner's own
code. Orders' callers carry a `TODO(#986)` in the code naming the answers they read:

| Call | Callers (`src/orders/`) |
|---|---|
| `scripts.GetNanoPiecePosition` | `SelfRepairOrder`, `CaptureOrder`, `ReclaimUnitOrder`, `RepairUnitOrder`, `RepairUnitNoMoveOrder`, `HelpBuildOrder`, `MobileBuildOrder`, `ReclaimOrder`, `ResurrectOrder`, `BuildingBuildOrder` (`unit_orders.cpp`); `VtolMobileBuildOrder`, `VtolRepairUnitOrder`, `VtolHelpBuildOrder`, `VtolReclaimOrder`, `VtolReclaimUnitOrder` (`vtol_orders.cpp`) |
| `units.AddRepairProgress` | `SelfRepairOrder`, `RepairUnitOrder`, `RepairUnitNoMoveOrder` (the answer decides the nano beam; the REPAIR effect above is the call where no answer is used) |
| `units.AddBuildProgress` | `BuildingBuildOrder`, `HelpBuildOrder`, `MobileBuildOrder` (`unit_orders.cpp`); `VtolMobileBuildOrder`, `VtolHelpBuildOrder` (`vtol_orders.cpp`) |
| `units.CreateUnit` | `BuildingBuildOrder`, `MobileBuildOrder`, `ResurrectOrder` (`unit_orders.cpp`); `VtolMobileBuildOrder` (`vtol_orders.cpp`) |
| `scripts.QueryScript` | `BuildingBuildOrder` (`unit_orders.cpp`); `FindLandingPad`, `VtolPickupOrder` (`vtol_orders.cpp`) |

(Each `TODO(#986)` marker sits above its function; `SelfRepairOrder`, `RepairUnitOrder` and `RepairUnitNoMoveOrder` carry
a second marker at the `AddRepairProgress` call.) `units.CreateUnit` is the one whose answer, the new unit, orders
keeps using as #967 left it (`docs/players-interface.md`), unchanged.

**Routes orders already published, kept as they were:** REQUEST and INCOME to economy (queued), RECLAIM and REMOVE to
features, SET_UNIT, SET_POS and CLEAR to targeting, GIVE_UNIT to players (`CaptureOrder`), and DAMAGE, RELEASE, CLAIM and
ADD_STOCKPILE to weapons (#984). `map.CanPlaceUnitFootprint` and `map.PickRandomReclaimableResourcesInRadius` go
through the `ScheduleMap` services as before, and `targeting.FindBestTargetIfFireAtWill` is a synchronous slot.
`pathfinding.Submit` and `Cancel` stay pathfinding's request slots, called from movement's goals.

**Orders to movement.** An order tells movement where a unit should go by events, SET_TARGET (the goal steers to a
target, or to none) and DESTROY_TARGET, published from `Order::SetAttachedFx` and the attach functions of
`order_queue.cpp`, and the VTOL orders' SET_FLIGHT_MODE, SET_ALTITUDE, SET_APPROACH_RADIUS and SET_HEADING
(`vtol_orders.cpp`). It builds a target with movement's `New*` slots, whose answer is the target's block offset, so those
stay synchronous slots of movement: the state machine uses the target in the next statement. Where a state machine reads
the motion in the same statement it steers it, it reads through movement's queries (`UnitTarget`, `TargetType`) or the
view. Everything is in `docs/movement-interface.md`, sections 2 and 3.

**Calls outside the interface, closed.**
- `docs/schedule.md` listed a call into orders outside its interface in `los_radar.UpdateSensorRadarAndCloak`, after
  `map.UnitsInRange`. It was the tool over-reaching, not a call in the code: `VisitUnitsInRange` calls its visitor's
  virtual `Visit`, and the tool resolved it to every visitor any caller passes, including orders'
  `DamagedAllyCollector`, which only `RepairPatrolOrder` passes. `tools/schedule.py` now follows a visitor only from the
  function that passes it, and `data/hazard_edges.csv` names the right passer, so `docs/schedule.md` lists no call into
  orders under los_radar.
- `units.LoadUnit` (the save loader) reached from `NewOrder`, `AddBeCarriedOrder` and `RunOrders` is a real call, of the
  load constructors (`Order::Order` calls `LoadUnit` directly, `order_queue.cpp`), and `schedule.py` follows a call into
  every function of its name, overloads included, which its Limits already state. The tool is unchanged.
- The orders function left outside `src/orders/`, `RegisterVtolOrders`, moved from `src/network/net_stats.cpp` to
  `src/orders/unit_orders.cpp`, where `order_list.cpp` calls it.
- The header's `Get` and `SetValue` of `PathOrderAttach` (the unit reference every order embeds) are units' slots now,
  `UnitRefGet` and `UnitRefSetValue` (units 5.0), so orders' calls to them go through `units.*` and pass
  `coupling --seams`. Orders calls `UnitRefGet` 39 times and `UnitRefSetValue` twice.
- `docs/schedule.md` at this change lists no call into ai or units outside its interface in `RunOrders` and
  `RunSecondaryOrders`. The issue's two entries, from the page at 768629d1a, were a call into ai after `map.GetGameType` and a call into units
  after `map.IsPadSlotFree`. The committed page has, in the same places, `units.SetUnitPosition` and
  `movement.PathOrderSetHeading`, and `movement.NewPathOrderToUnitAtAngle`: the goal and target objects they called moved
  to movement (#993) and are movement's slots now. `src/orders/` calls no function of ai.

## 4. Views and routes

**What orders reads:** a unit's position, cell, heading, flags, health, type and owner (units', and movement's writes
through units' handlers); its script and its model's pieces (scripts' queries); the weapon slots (weapons' view, read by
`WeaponCanReachUnit` and `GetWeaponRange`); the players' resources (economy's); the cells' heights, the pad slots and
the footprint (map's slots and services); the features and the visible enemies of a radius (features' and
los_radar's queries); and the motion (movement's `UnitTarget` and `TargetType` queries and its view), as of each
module's last run.

**What others read of orders:**
- **Views, in the state block, hashed:** the order list of a unit (`Unit::list`, read by units, ai, movement and
  targeting), the order's `flags_42` (ai, movement), `pos` (map, features, movement), `subFlags` and `unit` (movement)
  and `unknown_1a` (units), and the order's target (`Order::target`, written by units' unit reference code).
- **Queries:** `GetOrderFlags` (ai and units), `GetOrderType` (units) and `MissionTypeFromName` (units, weapons, the
  UI).

**As of orders' last run.** By #952's rule a query needs a delivery first only when an event that can be pending writes
what it reads. Every route into orders that is published inside the tick is synchronous, and the three queued ones
(section 8) are published only outside it, so no event is pending when a query runs: `GetOrderFlags` reads the first
order's `flags_42` and `GetOrderType` a unit's order table and flags, both written only by orders' own handlers at
their publisher's statement, and `MissionTypeFromName` reads a table that no event writes.

**The routes.** Every route, with its reason. The routes below that orders published before this change are final and
cite the document that decided them; the routes new in this change cite the gate (section 8) or #952's rule.

| Route | Now | Why |
|---|---|---|
| units ADD, to orders | synchronous | Published only by `RunInitialMission`, which `CreateMissionUnits` calls while a game loads (`game_load.cpp`), before the first tick, so it is delivered at once whichever its mode and no read can find it pending. Queued it stayed equal on the tier, as it must. |
| engine COMMAND, to orders | queued | Published only from the player's UI (`IssueOrderToSelection` in `selection.cpp`, `IssueMobileBuildOrders` in `control_panel.cpp`, called by the panels, the keys and the console's Assign) and by the harness's scripted orders (`HarnessBeforeStep`, before `SchedBeginTick`), all between ticks, where `SchedPublish` delivers at once (#970). No network path reaches them: `HandleNetPackets` calls none of these functions. Queued it stayed equal (section 8). |
| engine QUEUE_BUILD, to orders | queued | The same: published only by `QueueBuildOrderWithSound` (`control_panel.cpp`), from the build menu's click and the harness's scripted orders, between ticks. |
| units STOP, to orders | synchronous | The gate (section 8): queued, the scenarios crashed (exit code 3). |
| units BE_CARRIED, to orders | synchronous | The gate (section 8). |
| units STATUS_FLAGS, to orders | synchronous | #952's rule. Queued it stayed equal on the tier, but an order's own reference publishes it: `Order::~Order` and `UnitRefSetValue` clear the order's `link` through `ClearRef`, whose listener is that order, so a queued event would reach an order `RunOrders` has already freed; and `RunOrders` loops over the unit's orders in one call, reading the next order's `subFlags` (where `OrStatusFlags` writes) before orders' next delivery. |
| movement ORDER_FLAGS, to orders | synchronous | The gate (section 8). |
| ai ADD_ORDER, to orders (`OrdersOnAiAddOrder`) | synchronous | As before (the gate, `docs/ai-interface.md`, section 8). |
| ai QUEUE_BUILD, to orders (`OrdersOnAiQueueBuild`) | synchronous | As before (#952's rule, `docs/ai-interface.md`, section 4). |
| ai STOP, to orders (`OrdersOnAiStop`) | synchronous | As before (the gate, `docs/ai-interface.md`, section 8). |
| weapons ATTACKED, to orders (`OrdersOnWeaponsAttacked`) | synchronous | As before (the gate, `docs/ai-interface.md`, section 8). |
| weapons PARALYZE, to orders (`OrdersOnWeaponsParalyze`) | synchronous | As before (#952's rule, `docs/weapons-interface.md`, section 4). |
| weapons HIT, to units (`UnitsOnWeaponsHit`; was orders' `OrdersOnWeaponsHit`) | synchronous | As before (#952's rule, `docs/ai-interface.md`, section 4): the handler is units' now, and the write, `Unit::netDirtyFlags`, is the same. |
| orders SET_STATE_BITS, to units | synchronous | The gate (section 8): the traces were equal, the sim event digest diverged. |
| orders SET_POSITION, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but `TeleportOrder` moves other units, and the rest of the step reads every unit's position: `RunSecondaryOrders`' target searches and map's `UnitsInRange` read `Unit::pos` and the cells `SetUnitPosition` writes, before units' delivery after `UpdateMotion`. |
| orders ATTACH_TO_PIECE, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but a transport detaches itself (`OrdersPublishAttachToPiece(unit, 0, -1, 2)` in `PrepVtolClimb` and the VTOL orders that follow it), and the same unit's `UpdateMotion`, which the scheduler runs after `RunSecondaryOrders` with only movement's inbox delivered, reads `u->carrier` in `UnitMotion::UpdatePosition`; and the attach's handler publishes movement's SET_MODE, which the next statement reads (`unit->motion->flags & 3`). |
| orders FINISH_CONSTRUCTION, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but `BuildingBuildOrder`'s cancel branch publishes it for the half-built target and, in the next statement, a synchronous DAMAGE of 30000 on the same unit, and `ApplyUnitDamage` (`damage.cpp`) returns for a victim whose `flags & 0x4000` is set, a bit `FinishConstruction` sets (with `buildLeft` and `lastDamageType`) for a type with `b24`. |
| orders BUILD_DECAY, to units | queued | #952's rule finds no read: its only publisher is `GetBuiltOrder`, for the unit being built, and its handler (`ApplyUnfinishedBuildDecay`) is `AddBuildProgress` on that unit; the readers of `buildLeft` and `health` in `src/orders/` are of `order->target`, another unit, or in later runs of the order. |
| orders REPAIR, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but its handler (`AddRepairProgress`) spends the player's energy through economy's synchronous request, and `RunOrders`' loop and `RunSecondaryOrders` request energy for the unit's other orders before units' next delivery, reading the energy it would have spent. |
| orders UNLINK, to units | synchronous | The gate (section 8). |
| orders BUSY, to units | queued | #952's rule finds no read: `Unit::workTime` is read by economy's cloak upkeep (`economy.cpp`, through `WorldUnitInfo`) in the players phase, after a phase boundary has delivered it, and by the info panel, outside the tick; nothing in `src/orders/` reads it. |
| orders SET_FLAGS, to units | queued | #952's rule finds no read: it sets and clears bits 0x20, 0x800 and 0x8000 of `Unit::flags` (`MakeSelectableOrder`, `CloakOnOrder`, `CloakOffOrder`), whose readers are economy's and victory's in the players phase and the UI. |
| orders CLEAR_NET_DIRTY, to units | synchronous | The gate (section 8). |
| orders SET_STANCE, to units | queued | #952's rule finds no read: the stance is read back only at `unit_orders.cpp` lines 420 and 547, which copy one unit's stance into a SET_STANCE of another, and by the orders panel, outside the tick. |
| orders SET_ANGLES, to units | queued | #952's rule finds no read: its only publisher is `ResurrectOrder` (state 5), for the unit it has just created, whose own update comes later in the phase, after units' inbox is delivered at its head. |
| orders SET_BUILD_STATE, to units | queued | #952's rule finds no read: its only publisher is `ResurrectOrder` (state 5), for the unit it has just created, and the readers of that unit's `buildLeft` and `health` run in later states of the order or in the unit's own update. |
| orders START_SCRIPT, to scripts | synchronous | The gate (section 8). |
| orders REQUEST, to economy | queued | As before (`docs/economy-interface.md`, section 4). |
| orders INCOME, to economy | queued | As before (`docs/economy-interface.md`, section 4). |
| orders GIVE_UNIT, to players | synchronous | As before (the gate, `docs/players-interface.md`, section 8). |
| orders DAMAGE, to weapons | synchronous | As before (the gate, `docs/weapons-interface.md`, section 8). |
| orders RELEASE, to weapons | synchronous | As before (the gate, `docs/weapons-interface.md`, section 8). |
| orders CLAIM, to weapons | synchronous | As before (the gate, `docs/weapons-interface.md`, section 8). |
| orders ADD_STOCKPILE, to weapons | synchronous | As before (#952's rule, `docs/weapons-interface.md`, section 4). |
| orders RECLAIM, to features | synchronous | As before (`docs/features-interface.md`, sections 4 and 8). |
| orders REMOVE, to features | synchronous | As before (`docs/features-interface.md`, sections 4 and 8). |
| orders SET_UNIT, to targeting | synchronous | As before (`docs/targeting-interface.md`, section 8). |
| orders SET_POS, to targeting | synchronous | As before (`docs/targeting-interface.md`, section 8). |
| orders CLEAR, to targeting | synchronous | As before (`docs/targeting-interface.md`, section 8). |
| orders to movement (six routes) | synchronous | Each with its reason in `docs/movement-interface.md`, section 4. |

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv` has 80 rows of class `immediate` with orders as
writer or reader and the other side not movement alone (the rows between orders and movement are in the table too; the
rows of movement with other modules are on `docs/movement-interface.md`, section 4). The writes of the adapters of
orders' effects (the unit's flags, workTime, angles, health and the netDirty bits) are units' in them, and orders'
reads of the unit record are of units' view. Each pair is served as follows:

| Writer to reader | Rows | Items | Phase | How it is served |
|---|---|---|---|---|
| economy to orders | 5 | `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` (the `Player` records' resources) | players, units | A view: economy's view state, as of its last apply, read directly (`docs/economy-interface.md`, section 4). Orders' requests go through economy's `RequestGranted` query and its REQUEST and INCOME events. |
| los_radar to orders | 2 | the cells (`LoadTntMap` allocation), `Unit::flags` | units, players | A view: the cells, as of los_radar's last run, and the unit's flags, written by los_radar's own pass, which the scheduler runs before the unit's update. |
| map to orders | 3 | `Order::pos`, `Unit::flags`, `Unit::spatialBucket` | units | A view: map's bucket fields and `Unit::flags`, written by map's own code, as of the writer's last run; `Order::pos` is orders' own field, read by map. |
| movement to orders | 9 | `Order::subFlags`, `Unit::cell`, `flags`, `heading`, `pos`, `yFraction`, `zFraction`, `UnitMotion::flags`, `velocity` | units | A view of the unit record, written by movement's effects, whose handlers are units' adapters (`docs/movement-interface.md`, section 3), and by movement's own `UpdateMotion`, which the scheduler runs after `RunOrders` in the unit's update, so orders reads the previous tick's, as in 3.1c; `subFlags` is set by ORDER_FLAGS (STATUS_FLAGS on orders' side). |
| orders to ai | 2 | `Order::flags_42`, `Unit::list` | players | A view: the order list of a unit, read by ai's ticks in the players phase, as of orders' last run in the units phase. |
| orders to features | 1 | `Order::pos` | units | A view: the order's position, read by features as of orders' last write. |
| orders to map | 1 | `Order::pos` | units | A view: the order's position, read by map as of orders' last write. |
| orders to movement | 8 | `Order::flags_42`, `pos`, `subFlags`, `unit`, `Unit::flags`, `Unit::list` | units, players, projectiles | A view: the order a goal reads (`Order::unit`, `pos`, `flags_42`) and the unit's order list, as of orders' last write, in 3.1c's order within a unit's update (orders before movement). |
| orders to scripts | 2 | `Unit::field_64`, `Unit::netDirtyFlags` | units | A view of the unit record, written by orders' effects (SET_FLAGS, CLEAR_NET_DIRTY) through units' handlers, as of the handler's run. |
| orders to targeting | 2 | `Unit::netDirtyFlags` | projectiles, units | A view of the unit record, as of the handler's run of CLEAR_NET_DIRTY. |
| orders to units | 6 | `Order::unknown_1a`, `Unit::flags`, `health`, `netDirtyFlags` | players, projectiles, units | A view of orders' order list, and units' own fields written by units' adapters of orders' effects (SET_FLAGS, SET_BUILD_STATE, CLEAR_NET_DIRTY), as of the handler's run. |
| orders to weapons | 2 | `Unit::flags`, `health` | units | A view of the unit record, written by orders' effects in a unit's update; orders run after weapons in the unit's update, so weapons reads the previous tick's, as in 3.1c (`docs/weapons-interface.md`, section 4). |
| scripts to orders | 3 | `Unit::cobStateFlags`, `netDirtyFlags`, `script` | units | A view of the unit record and the unit's script, written by `RunScripts` in the unit's body, before orders' step (3.1c's order). |
| targeting to orders | 2 | `Unit::netDirtyFlags` | projectiles, units | A view of the unit record, as of targeting's last handler run. |
| units to orders | 26 | `Order::target`, `Unit::attacker`, `cargo`, `carrier`, `cell`, `cobStateFlags`, `def`, `flags`, `footprint`, `heading`, `health`, `lastAttackerSlot`, `motion`, `netDirtyFlags`, `pos`, `script`, `speed`, `unitDefIndex`, `yFraction`, `yWhole`, `zFraction`, `UnitMotion::flags`, and the `Player` records' resources | units, projectiles | A view of the unit record, written by units' own code and its adapters (the effects of weapons, movement and orders), read by orders in its run, in 3.1c's order within the unit's update (head, weapons, body, then orders, movement, tail). |
| weapons to orders | 6 | `Unit::f_1f`, `f_3b`, `f_57`, `weapon` | projectiles, units | A view of weapons' slots, read by orders in the unit's update (after weapons), and by ATTACKED's return fire at weapons' statement (`docs/weapons-interface.md`, section 4). |

How a pair is served, in the three ways of the table: **a view** is read directly from the state block as of the writer's
last run; **in 3.1c's order within a unit's update** means the scheduler's order (head, weapons, body, orders,
movement, tail) is the order of 3.1c's `UpdateUnit`; **a synchronous route** is the route of section 4 that is made
synchronous because a later read needs its write. Every effect route inside the tick is synchronous, each for the reason
in the table of routes above.

## 5. State and the trace

**Everything stays hashed in this change** (the human, 2026-11-16, on PR #978, and 2026-11-17, on #984): the order
lists, the order effects, the motions and the goals. #979 covers los_radar, map, players and ai only; what follows
moves out of the hash in one later format bump for weapons, orders and units. Nothing leaves the hash in this change.
The module has no private region, and its `WriteState` writes no record.

**The hashed state only orders reads, a candidate for that bump.** The order lists are read by units, ai, map,
movement, weapons and targeting only through the fields of section 4 (`Unit::list`, `Order::flags_42`, `pos`,
`subFlags`, `unit`, `target`, `unknown_1a`); a search of `data/dependencies.csv` finds no row naming another reader of
the rest of `Order`:

| State | Trace section | Written by | Read by |
|---|---|---|---|
| the order's `kind`, `state`, `wakeFrame`, `next`, its `flags` (but `flags_42`) and the per-kind counters of its `type`, `id`, `wait`, `ticks`, `time` union | the unit's order list, section 3 of the trace (`docs/trace-format.md`, section 8.10) | orders | orders (the `Order` fields `flags`, `next` and `unit` are logged read-only by orders itself) |
| the order effect class and its numbering (the targets of movement, `docs/movement-interface.md`, section 5) | section 22 | movement | movement |

`Unit::list` (the head of the order list) is read by ai, movement and units and stays hashed for them.

**Record and sections.** The module's `WriteState` writes no record (movement's neither). The trace dumps orders' events
in section 48 (`docs/trace-format.md`, section 8.32) and movement's in section 49 (section 8.33), and hashes none of
them. Sections 50 and 51 are reserved for units and scripts (#986). Section 48 is also the number of weapons' record 48,
`weaponsState`: records and sections are keyed by name in the dump (`ordersEvents.*`, `weaponsState`), so they do not
clash. Each record is an event with the outcome `Apply` gave, or an effect orders published (outcome 1), in orders'
order, after section 47's. The sections are new, so the trace format's schema changes.

`privatestate.py --check` and `seamlink.py --pilot-seams` check the seam: no other simulation subsystem writes orders'
state or calls its entry points, and no query slot writes state.

## 6. Random draws and sim events

Orders draws from the game logic stream (`RandomInt`) in its state machines (a wander's offset, a random delay, a pad's
choice) and records sim events (speech, a build's sounds and smoke) where 3.1c did. The scheduler keeps 3.1c's order of
runs in a unit's update: units' head, weapons' step, units' body, orders' `RunOrders`, then `RunSecondaryOrders`,
then movement's `UpdateMotion` (for a unit with a motion), then units' tail:

```
UpdateUnitHead(u); deliver units; UpdateUnitWeapons(u); deliver units;
if (UpdateUnitBody(u)) { deliver orders; RunOrders(u); deliver orders; RunSecondaryOrders(u);
                         if (u->motion) { deliver movement; UpdateMotion(u); moved = 1 } }
deliver units; UpdateUnitTail(u, moved)
```

3.1c called these from inside `UpdateUnit`; the split keeps 3.1c's order. Every route around these draws is either
synchronous, so the draw or record is made at the publisher's statement, or queued but published only outside the tick, where it is
delivered at once (section 8): the three queued routes are the engine's COMMAND and QUEUE_BUILD and movement's LOAD.

The class `ordered` of `data/dependencies.csv` has 18 rows with orders as writer or reader (the issue counted 16 when
it was filed; movement has none, because the access logs predate movement's module), on `rng.parkMiller` and on
`sim-event`. Each is kept in order as follows:

| Writer to reader | Kind | Phase | Runs | How it is kept in order |
|---|---|---|---|---|
| map to orders | `rng.parkMiller` | units | 4 | Map's draw inside a unit's update and orders' in its state machines, in the scheduler's order (head, weapons, body, orders, movement); the routes between them (`ADD_UNIT`, `REMOVE_UNIT` from movement, section 4) are synchronous (section 4). |
| orders to map | `rng.parkMiller` | units | 3 | The same: map's `Apply` draws nothing between orders' draws, and the order of the two runs in a unit's update is the scheduler's. |
| orders to scripts | `rng.parkMiller` | units | 12 | START_SCRIPT is orders' effect to scripts; a script's start can draw, so the route is synchronous: queued, it diverged (section 8). The statement that publishes it is where the draw is made. |
| orders to scripts | `sim-event` | units | 8 | The same for the sim events a started script records. |
| orders to targeting | `rng.parkMiller` | units | 7 | Targeting's draw (`FindBestTargetIfFireAtWill`, a synchronous slot) and SET_UNIT, SET_POS and CLEAR (synchronous routes, as before) are made at orders' statement. |
| orders to units | `rng.parkMiller` | units | 11 | The units adapters of orders' effects (REPAIR, FINISH_CONSTRUCTION, SET_POSITION, ATTACH_TO_PIECE, UNLINK, the state bits) call units' slots; every one of these effects is synchronous (section 4), so each draw is made at the publisher's statement. Units' `CreateUnit` and `AddBuildProgress` are synchronous slots. |
| orders to units | `sim-event` | projectiles | 1 | Units' record under a projectile's hit: orders' effect and weapons' route are in the same call chain (`docs/weapons-interface.md`, section 6). |
| orders to units | `sim-event` | units | 41 | The same adapters and slots: each record is made where the call was, because every one of these effects is synchronous (section 4). |
| orders to weapons | `rng.parkMiller` | units | 15 | DAMAGE, RELEASE, CLAIM and ADD_STOCKPILE from orders and PARALYZE and ATTACKED to orders are synchronous routes (`docs/weapons-interface.md`, section 4), so each draw is made at the publisher's statement; orders run after weapons in a unit's update, as in 3.1c. |
| orders to weapons | `sim-event` | units | 11 | The same for the sim events. |
| scripts to orders | `rng.parkMiller` | units | 10 | Scripts' draws (a unit's `RunScripts` in its body) come before orders' step in the unit's update; no event of orders lies between them. |
| scripts to orders | `sim-event` | units | 8 | The same. |
| targeting to orders | `rng.parkMiller` | units | 11 | Targeting's retargeting draw and orders' in the unit's update: its routes (SET_UNIT, SET_POS, CLEAR) are synchronous, and `FindBestTargetIfFireAtWill` is a synchronous slot. |
| units to orders | `rng.parkMiller` | units | 13 | Units' draws and orders' inside a unit's update: ADD, STOP, BE_CARRIED and STATUS_FLAGS from units and the effects to units are in 3.1c's order by the scheduler's; STOP, BE_CARRIED and STATUS_FLAGS are synchronous and ADD is published only outside the tick (section 4). |
| units to orders | `sim-event` | units | 50 | The same for the sim events (speech, smoke). |
| weapons to orders | `rng.parkMiller` | units | 13 | ATTACKED and PARALYZE to orders and DAMAGE, RELEASE and CLAIM from orders are synchronous routes (`docs/weapons-interface.md`, section 6), so each draw is made at the publisher's statement. |
| weapons to orders | `sim-event` | projectiles | 1 | The same, under a projectile. |
| weapons to orders | `sim-event` | units | 11 | The same, in a unit's update. |

## 7. What stays inside the default

3.1c's types and limits are the default's, not the interface's: the order kinds and their state machines, the order
type table, the order flags (the interface names only the ones callers need to name an order, `ORDER_FLAG_*`), the
`MissionType`, the order list a unit keeps, the saved game's order records and the network's order packets. An event
carries a unit as its block offset, an order, a target and a reference as block offsets, a unit type as its name bytes
and an order type as its index: no pointers, no `Unit*`, `Order*`, `OrderBase*`, `UnitMotion*` or `HapiBank*` in an
event. The set-up slots (`RegisterAllOrderTypes`, `ClearOrderTypeTable`) and the saved game's `LoadOrders` and
`SaveOrders` stay in `OrdersSeam` as 3.1c's code (section 2). `ApplyPathEvent`, the engine's delivery of pathfinding's
results, stays the engine's delivery as before, now movement's slot (`docs/movement-interface.md`, section 2).

## 8. What the gate decided

Every route into and out of orders that is new in this change was queued first, each on its own, with every other
route as in `src/game/routes.cpp` at the end of this change (the tier, with `--no-cache`, on the modern build; the gate
line of an equal run names the commit it ran at, 1a3180607, c5c394dfe, 0274316fe or 1f78163f3, since commits landed
meanwhile, and the table of routes was the same in each). A route whose queueing diverged stays synchronous. The
divergences are against the reference, with the first tick, phase and field of the comparison's report. Where the report's
first field is an `aiState` squad record that one side lacks, that is an artifact of the comparison, and the row gives the
tick and phase and the scenario's first `DIFF` line instead. A route that stayed equal is still synchronous when it can be
pending while a later read in the same phase needs what it writes (#952's rule, section 4); the routes it
finds no such read for are queued together (section 4), and the gate of this change ran with them queued. The engine's COMMAND and QUEUE_BUILD and movement's LOAD stay queued, since they are published only
outside the tick; with them and map's PLACED queued together, the tier was equal (38 of 38 scenarios at fdfcd3cae033).

| Route queued | Tier | First divergence |
|---|---|---|
| units ADD | equal | none (published only outside the tick, section 4) |
| engine COMMAND | equal | none (queued, section 4; queued with QUEUE_BUILD, LOAD and PLACED: 38 of 38 equal) |
| engine QUEUE_BUILD | equal | none (queued, section 4) |
| units STOP | crashed | mission-kill-of-type ended at tick 181 of 360 (exit code 3), and `stopped: 16 scenarios not finished`; eight-full-moon's first run failed too. No complete trace to compare. |
| units BE_CARRIED | diverged | orders-air, tick 1025: stored eca9728ab3cc3df1, got ff5198cece061df9. The comparison's replay of the scenario exited with code 2, so it gave no report. |
| units STATUS_FLAGS | equal | none (synchronous by #952's rule, section 4) |
| movement ORDER_FLAGS | diverged | orders-air, tick 940, world phase (the once-per-tick tables, hashed after `UpdateMeteors`): stored 394b77cd380e1e61, got a3baa9ecf36255af. |
| orders SET_STATE_BITS | traces equal, events diverged | eight-full-moon, tick 5636, units phase: the sim event digest (`5636 units 8`, stored 4bb93f76561b11ce, got ff3dbf0252c62e3c); also mission-kill-of-type tick 90, mission-passes-z tick 99, orders-air tick 1025 and ui-build-menu tick 87. |
| orders SET_POSITION | equal | none (synchronous by #952's rule, section 4) |
| orders ATTACH_TO_PIECE | equal | none (synchronous by #952's rule, section 4) |
| orders FINISH_CONSTRUCTION | equal | none (synchronous by #952's rule, section 4) |
| orders BUILD_DECAY | equal | none (queued, section 4: #952's rule finds no read) |
| orders REPAIR | equal | none (synchronous by #952's rule, section 4) |
| orders UNLINK | diverged | conditions-dark-side, tick 302: stored 75eb1125904e65c8, got d190d9504c9ba36a. The comparison's report timed out after 300 s. |
| orders BUSY | equal | none (queued, section 4: #952's rule finds no read) |
| orders SET_FLAGS | equal | none (queued, section 4: #952's rule finds no read) |
| orders CLEAR_NET_DIRTY | diverged | mission-build-unit, tick 79, units phase (`UpdateAllUnits`, 0x48ad30): `rng.parkMiller` (reference 0x770d7425, queued 0x1219520c). |
| orders SET_STANCE | equal | none (queued, section 4: #952's rule finds no read) |
| orders SET_ANGLES | equal | none (queued, section 4: #952's rule finds no read) |
| orders SET_BUILD_STATE | equal | none (queued, section 4: #952's rule finds no read) |
| orders START_SCRIPT | diverged | orders-air, tick 1688, world phase: stored 2f9520d4ff7b7f17, got 151c8cc16975ad77. |
| ai ADD_ORDER, QUEUE_BUILD, STOP | unchanged | synchronous as before (`docs/ai-interface.md`, section 8) |
| weapons ATTACKED, PARALYZE, HIT | unchanged | synchronous as before (`docs/ai-interface.md`, section 8, and `docs/weapons-interface.md`, section 8) |
| orders to movement (six routes) | | in `docs/movement-interface.md`, section 8 |

**Inbox depths.** A build that counted every publish and delivery played all 89 scenarios (87 wrote their counts;
console-ilose and console-iwin exit before the dump), with native modules, and every scenario stayed equal. No route
into orders is queued inside the tick: the three queued routes (the engine's COMMAND and QUEUE_BUILD, and movement's
LOAD) are published only outside it, where each event is delivered at once (#970). So orders' inbox never held an event
inside the tick, and its depth is 0. Orders' six queued effects go to units' inbox, which also takes other modules'
events; its deepest was 5 events (orders-air-repair, tick 757), and BUSY, the busiest, was queued 1,282,535 times.
Orders took at most 199 deliveries in one tick (edges-economy-targeting, tick 245), and deliveries nested at most 6
deep, counting every module (console-units, tick 150). A synchronous delivery into movement runs inside one into orders:
`Apply` of STOP calls `DeleteOrders`, and `Order::~Order` publishes movement's SET_TARGET and DESTROY_TARGET
(`order_queue.cpp`) through `SchedPublish`. A delivery into orders also runs inside orders' own run: `SelfDestructOrder`
(in `RunOrders`) publishes weapons' DAMAGE, which publishes ATTACKED, whose route reaches orders' `Apply`.
