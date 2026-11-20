# The map interface

Map sits behind `interfaces/map.h` (version 7.0), the fifth module on the scheduler after economy
(`docs/economy-interface.md`), features (`docs/features-interface.md`), targeting (`docs/targeting-interface.md`) and
los_radar (`docs/los-radar-interface.md`) (#966). It is the map's units and terrain as the simulation sees them: which
unit stands on each cell (the footprints), the height of the ground, the spatial grid of units (the buckets), the pass
map of each movement class and when it is current, and the placement tests. 3.1c's code is its default
implementation. Map 5.0 (#983) gave the mission and campaign data their own interface (`docs/mission-interface.md`)
and took the pass maps over from units end to end, their reads and their lazy refresh, by the human's ruling of
2026-11-18 on #986. Map 7.0 (#1002) split the pass maps out of units' `MovementClass` into map's own record,
`PassMap`, so that no slot of map takes a units record (section 5). This page is the inventory the interface was cut from:
- what other subsystems did to map's state in 3.1c, and what each became;
- what map did to others, and what each became;
- what map reads, and what others read of it;
- how the order of random draws is kept;
- what the trace hashes, and what it only dumps.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, events published
outside the tick delivered at once (#970), and events as plain data. This issue changes none of its rules. Event
kinds are unique across interfaces: economy uses 0 to 4 and 16 to 18, features 32 to 35 and 48 to 51, wind_meteors 64
to 67 and 80, targeting 96 to 99 and 112, los_radar 128 to 136, map 160 to 166, 168 and 176 (167 was
REFRESH_PASS_MAP, which map 5.0 removed).

Map's place in it:
- **Events into map:** ADD_UNIT, REMOVE_UNIT, CLEAR_UNIT, RECLAIM_FOOTPRINT, SET_YARD, BUCKET_UNLINK, BUCKET_PREPEND
  and PASS_MAPS, published by units and orders, and PASS_MAPS from features' effect of the same name.
  Map's handler (`MapInbox`, `src/seams/map_world.cpp`) passes each to `Apply`; features' PASS_MAPS reaches it through
  `MapOnFeaturesPassMaps`, the route #971 added, which now builds a PASS_MAPS event and hands it to the same handler;
  it moved from map's files into the same glue, as it is the engine's adapter.
- **Routes:** all ten routes into map are synchronous: six because the gate diverged with them queued, units'
  ADD_UNIT and BUCKET_PREPEND by #952's rule, features' PASS_MAPS as #971 left it, and RECLAIM_FOOTPRINT because no
  scenario reaches it (sections 4 and 8). Of map's two effects, CELLS_CHANGED is synchronous by #952's rule and
  CELL_HEIGHTS stays queued.
- **Runs:** none. Map has no pass of its own: every write it makes inside the tick is on behalf of a caller, and the
  scheduler gains no pass for it. Map's inbox is delivered at each phase boundary and before each synchronous slot.
- **Synchronous slots:** `CanBuildAt`, `CanPlaceUnitFootprint` and `PickRandomReclaimableResourcesInRadius`, called
  through the services `ScheduleMapCanBuildAt`, `ScheduleMapCanPlaceUnitFootprint` and `ScheduleMapPickReclaimable`
  (services 1.16), which deliver map's inbox first. Each caller needs the answer in the same statement (section 2).
  `RefreshMovedUnits` and `RefreshUnitIfStale` (5.0), the lazy refresh of the pass maps, are called by pathfinding's
  world view at the start and end of a search (`WorldBeginSearch`, `WorldEndSearch`, `src/seams/pathfinding_world.cpp`),
  which deliver map's inbox first too.
- **Queries:** `UnitsInRange`, `GetCellMeanHeight`, `SnapWorldPosToFootprint`, `CanPlaceFootprintAt`,
  `IsPadSlotFree`, and since 5.0 `GetGroundHeight`, `PassMapGet` and `PassMapInBounds` (`MovementClassGet` and
  `MovementClassInBounds` before 7.0), called directly, as
  of map's last apply.
- **Views:** the footprint fields of the cell records, through los_radar's cell view (section 4); a unit's bucket
  (`Unit::spatialBucket`) and the bucket of units off the map (`Game::overflowBucket`); the pass maps; the placement
  record (`g_lastPlaceMetalSum`, `g_lastPlaceHeight`).
- **Effects out of map:** CELLS_CHANGED, to pathfinding, when a pass map changed, the refreshes of 5.0 included; pathfinding is not on the model
  yet, so its handler is an adapter onto its seam slot `CellsChanged` (`PathfindingOnMapCellsChanged`,
  `src/game/routes.cpp`). The writes map makes into units' records stay view state that map writes in its own events
  (section 3).
- **Module ids:** map keeps `SCHED_MAP = 7`, which #971 added. Pathfinding gets `SCHED_PATHFINDING = 12` (services
  1.16) as the subscriber of CELLS_CHANGED, so `SCHED_MODULE_COUNT` is 13.

## 2. Events into map

| 3.1c site | 3.1c call | Now |
|---|---|---|
| units: a unit created (`CreateUnit`, `CreateUnitFromPacket`), moved to another cell (`SetUnitPosition`), its state read from a packet (`ReadUnitState`) | `AddUnitToMap` | ADD_UNIT from units |
| orders: a moving unit enters another cell (`UnitMotion::UpdatePosition`) | `AddUnitToMap` | ADD_UNIT from orders |
| units: a unit leaves its cell (`SetUnitPosition`, `ReadUnitState`) | `RemoveUnitFromMap` | REMOVE_UNIT from units |
| orders: a moving unit leaves its cell (`UnitMotion::UpdatePosition`) | `RemoveUnitFromMap` | REMOVE_UNIT from orders |
| units: a unit dies (`ApplyUnitDeath`) | `ClearFootprintAndUnlink` | CLEAR_UNIT from units |
| units: a unit read from a saved game (`LoadUnit`) | `ForceNeighborFootprintReclaim` | RECLAIM_FOOTPRINT from units |
| units: a unit's script sets YARD_OPEN (`UnitScript::SetUnitValue`) | `SetYardOpen` | SET_YARD from units |
| units: a unit boards a carrier (`ApplyAttachUnit`) | `SpatialBucketUnlinkUnit` | BUCKET_UNLINK from units |
| units: a unit leaves a carrier (`ApplyAttachUnit`) | `SpatialBucketPrependUnit` | BUCKET_PREPEND from units |
| a stale pass map under a unit, found by pathfinding's world view (`PassMap::RefreshMovedUnits`, `RefreshUnitIfStale`) | `RefreshPassMap` (`PassMap::Refresh` since 7.0) | map's own work since 5.0: the synchronous slots `RefreshMovedUnits` and `RefreshUnitIfStale` (REFRESH_PASS_MAP from units in 4.x) |
| features: the PASS_MAPS effect (a feature placed or removed) | `RefreshAllPassMaps` | PASS_MAPS, through `MapOnFeaturesPassMaps` |

**Answers.** No caller used a value: 3.1c's `AddUnitToMap`, `RemoveUnitFromMap`, `ClearFootprintAndUnlink`,
`ForceNeighborFootprintReclaim`, the bucket calls and the pass map refreshes returned nothing, and `SetYardOpen`'s
result was thrown away. So none of these is synchronous by its contract: every same-statement order is a route's.

**Outcomes.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 40) and returned to no
one: 1 for every kind, except SET_YARD (0 when the footprint is not clear for the new state, or the unit has an empty
yard map and is skipped). `Apply` first rejects an event naming no unit (every kind but PASS_MAPS), a negative size
(PASS_MAPS), a unit with no bucket or not in its bucket's chain (BUCKET_UNLINK), a unit with no bucket or already in
its bucket's chain (BUCKET_PREPEND), and an unknown kind, with outcome 0 and no change. A PASS_MAPS of size 0 is
applied: it still covers its cell grown by each class's footprint, as in 3.1c. 3.1c's callers could publish none of these, so no trace moves; a module publishing from WASM
can.

**The mixed slots.**
- **`CanPlaceUnitFootprint` is synchronous, not split.** Every caller branches on its answer in the same statement:
  orders' build, unload and drop orders, ai's random placement, players' commander placement and a script's
  `DropUnit`. Its write is the placement record (`g_lastPlaceMetalSum` and `g_lastPlaceHeight`, through `CanBuildAt`,
  for a type that does not move), which the caller reads right after the answer (ai's `FindRandomPlacementCell` reads
  `g_lastPlaceMetalSum` in the same statement). A split into a query and an intent would need the intent applied before
  the caller's next statement, so it would be a synchronous route under another name, and the query would read the
  footprints as of map's last apply, which the synchronous call already guarantees. `CanBuildAt` is synchronous for the
  same reason (ai's `FindCellNearFeatures`).
- **`PickRandomReclaimableResourcesInRadius` is synchronous; the draw stays in map.** It scans the cells in the
  radius for reclaimable features, then draws from the game logic stream three times per kind of resource it found (an
  energy and a metal list), and keeps the best of the three. How many draws there are, and their ranges, depend on
  the scan, so a caller-side draw would need the two candidate lists back from a query and then the pick itself in
  orders' two callers (`RepairPatrolOrder`, `VtolRepairPatrolOrder`): map's choice of what to reclaim would move into
  orders, out of the default a re-implementation of map replaces. Called at the caller's statement, the draws land
  in 3.1c's order (section 6). It writes no state of map's: only the caller's out parameters and the stream.

**Every slot of 3.0.** The 40 slots of map 3.0, and what each became in 4.0:

| 3.0 slot | 4.0 |
|---|---|
| `AddUnitToMap`, `RemoveUnitFromMap`, `ClearFootprintAndUnlink`, `ForceNeighborFootprintReclaim`, `SetYardOpen`, `SpatialBucketUnlinkUnit`, `SpatialBucketPrependUnit`, `RefreshPassMap`, `RefreshAllPassMaps` | events: ADD_UNIT, REMOVE_UNIT, CLEAR_UNIT, RECLAIM_FOOTPRINT, SET_YARD, BUCKET_UNLINK, BUCKET_PREPEND, REFRESH_PASS_MAP, PASS_MAPS, through `Apply` |
| `VisitObjectsInRange` | the query `UnitsInRange` (below) |
| `CanBuildAt`, `CanPlaceUnitFootprint`, `PickRandomReclaimableResourcesInRadius` | synchronous slots, through the `ScheduleMap` services |
| `GetCellMeanHeight`, `SnapWorldPosToFootprint`, `CanPlaceFootprintAt`, `IsPadSlotFree` | queries |
| `GetGameType`, `GetNameSlot` | queries of the mission, read inside the tick |
| `BuildAllPassMaps`, `FreeOtaEnumCacheAndMission`, `SetMissionType`, `LoadMappingData`, `LoadMetalPlotmap`, `LoadPlayerFeaturesPlotmap`, `SaveMappingData`, `SaveMetalPlotmap`, `SavePlayerFeaturesPlotmap` | set-up, save and load slots, outside the tick |
| `AdvanceMission`, `GetCampaignName`, `GetMissionIndex`, `GetMissionName`, `GetStartPosition`, `LoadCampaign`, `LoadMissionByName`, `MissionExists`, `RefreshMapList`, `SelectMission`, `ComputeMapChecksum`, `GetTerrainLength` | the mission, campaign and map-list slots, outside the tick |

**Every slot of 4.1.** What map 5.0 changed:

| 4.1 | 5.0 |
|---|---|
| `GetGameType`, `GetNameSlot`, `SetMissionType`, `FreeOtaEnumCacheAndMission`, `AdvanceMission`, `GetCampaignName`, `GetMissionIndex`, `GetMissionName`, `GetStartPosition`, `LoadCampaign`, `LoadMissionByName`, `MissionExists`, `RefreshMapList`, `SelectMission`, `ComputeMapChecksum`, `GetTerrainLength` | the mission interface 1.0, without the `Mission*` argument (`docs/mission-interface.md`) |
| the event REFRESH_PASS_MAP | the synchronous slots `RefreshMovedUnits` and `RefreshUnitIfStale`, moved from units 2.0 (units 3.0 no longer has them) |
| units' `MovementClassGet` and `MovementClassInBounds` | queries of map |
| units' `GetGroundHeight` | a query of map: it interpolates the heights of the cells, terrain map alone keeps |
| every other slot | unchanged |

**Every slot of 6.0.** What map 7.0 changed (#1002):

| 6.0 | 7.0 |
|---|---|
| `BuildAllPassMaps(void)`, which read the limits from units' movement classes | `BuildAllPassMaps(limits, count)`: the limits as plain data in a buffer the engine owns, which map copies (`MapMoveLimits`, from units' new slot `GetMovementLimits`, units 4.1) |
| `MovementClassGet(MovementClass*, x, y)`, `MovementClassInBounds(MovementClass*, x, y)` | `PassMapGet(moveClass, x, y)`, `PassMapInBounds(moveClass, x, y)`: the class by its index, 0 to 31 |
| `RefreshMovedUnits(MovementClass*, Unit*)`, `RefreshUnitIfStale(MovementClass*, Unit*)` | the same, with the class by its index |
| units' `FreeMovementClasses`, which also freed the pass maps | map's `FreePassMaps`, a shutdown slot the engine calls after units' `FreeMovementClasses`, which now frees only the names |
| every other slot | unchanged |

**`VisitObjectsInRange` became a query.** 3.1c's took a `UnitVisitor` and called its `Visit` for each unit within
range of a position, in the spatial grid's order. The callback is not plain data and would run the caller's code
inside map's. Map 4.0's `UnitsInRange` writes the ids of the units in range to a buffer the caller gives, in the
same order, skipping the first `skip` of them, and returns how many it wrote, at most the buffer's capacity, as
pathfinding's `Poll` fills a caller's buffer. The service `VisitUnitsInRange` (services 1.16, `src/seams/map_world.cpp`) pages
through it, 256 ids at a time, and calls the visitor for each unit, so its callers (orders' patrol and repair
orders, the victory condition `VictoryMoveUnitToRadius`, los_radar's `UpdateSensorRadarAndCloak`) keep their
visitors. The order is 3.1c's because no visitor moves a unit or publishes to map: they count, collect or test the
units they are given. That is the service's contract: a visitor must not move a unit or change map's grid, since
`VisitUnitsInRange` takes a page of ids before it calls the visitors, and a changed grid would page differently
once more than 256 units are in range. `VisitObjectsInRect`, 3.1c's visitor over a rectangle, had no caller; it is
removed, so map calls no visitor.

**The mission, campaign and map-list slots left map in 5.0.** They are not simulation: they read and write the
`Mission` object and the map list, from the front end, the load of a match and the end of a game, and two of them,
`GetGameType` and `GetNameSlot`, are queries read inside the tick. By the human's ruling of 2026-11-17 on #980 they
are the mission interface's (`interfaces/mission.h`, `docs/mission-interface.md`), a module that can be replaced like
any other.

**The pass maps are map's end to end since 5.0.** In 4.x map wrote the pass maps under its events, while units
answered the reads (`MovementClassGet`, `MovementClassInBounds`) and ran the lazy refresh around a search
(`RefreshMovedUnits`, `RefreshUnitIfStale`), which published REFRESH_PASS_MAP back to map and reported to
pathfinding itself. By the human's ruling of 2026-11-18 on #986 the four are map's slots: the reads are queries, the
refresh is map's own synchronous work, and the event is gone. The refresh writes the class's window of stale stamps
(`MovementClass::lastTick`) and a unit's stamp (`UnitMotion::pathLockStamp`), both map's bookkeeping of when a pass
map is current, and reports each rewrite through CELLS_CHANGED (section 3). The movement class definition stays in
units: the classes read from `MOVEINFO.TDF` (`ReadMoveInfo`, the `MovementClass` constructor and destructor,
`FindMovementClass`, `FreeMovementClasses`, `GetMovementLimits`, `CopyPathLockFields`) are units' functions in
`src/units/movement_class.cpp`. Since 7.0 the pass maps are map's own record, `PassMap` in `src/map/pass_map.cpp`
(the `file:src/map/pass_map` rule of `data/subsystems.csv`, with no hand-set row), and the stamp window is
`PassMap::lastTick` (section 5).

## 3. Effects out of map

**CELLS_CHANGED, to pathfinding.** When map computes a pass map again, it tells pathfinding which cells of which
movement classes changed and why (a `PathChange`), as 3.1c called `CellsChanged`. Map publishes one for each
movement class it computed again: under PASS_MAPS (the terrain changed), under ADD_UNIT of a building, SET_YARD and
RECLAIM_FOOTPRINT (all classes over the footprint), and under REMOVE_UNIT (`RefreshPassMapsForUnit`: the classes whose
pass map changed since the unit was last stamped). It records each in the trace's dump just before `SchedPublish`, as `docs/module-abi.md`, 9.3,
asks of a module's effects. The adapter calls pathfinding's `CellsChanged` with the effect's fields. Since 5.0 the
refreshes around a search (`RefreshMovedUnits`, `RefreshUnitIfStale`) publish it as well, one for each pass map they
compute again, with cause `PATH_CHANGE_UNITS`; in 4.x they were units' direct calls of pathfinding's seam. The route is
synchronous, so pathfinding hears each at the statement that rewrote the pass map, in 3.1c's order.

**Writes into units' records.** The issue counted writes into `Unit` and `Order` fields. Each stays view state that
map alone writes, inside its own events, at the publisher's statement, as los_radar's sensor bits did
(`docs/los-radar-interface.md`, section 3):
- **`Unit::spatialBucket`, and `Unit::cargoNext` of a unit that is not carried:** the unit's bucket, and its link in
  the bucket's chain. They are map's spatial grid, kept in the unit's record. `cargoNext` is one field with two
  chains: while a unit is carried (`carrier` is set) it links the carrier's cargo, which units writes
  (`ApplyAttachUnit`); otherwise it links the bucket, which map writes (ADD_UNIT, REMOVE_UNIT, CLEAR_UNIT and the
  bucket events). Orders reads the bucket to tell a unit off the map (`overflowBucket`) and to place an air unit's
  height, in the same update; the synchronous routes keep that.
- **`Unit::flags` 0x4000000 and 0x8000000:** the footprint claim bits. 0x8000000 says a unit must claim its cells
  again, 0x4000000 that its footprint lies on cells another unit holds; ADD_UNIT, REMOVE_UNIT, CLEAR_UNIT, SET_YARD
  and RECLAIM_FOOTPRINT set and clear them, and the claim reads them back. They share the word with the other bits
  of `flags`, which units, orders and los_radar write; the access log names the word, so its rows (section 4) are
  every reader of any bit.
- **`Unit::bit2`:** the yard state that SET_YARD writes, which the yard map's test reads back (`ClaimFootprintCells`,
  `AddUnitToMap`). The access log names it as `Unit::cobStateFlags`, the union over it.
- **`UnitMotion::pathLockStamp`:** ADD_UNIT stamps a mobile unit's motion with the tick it was placed; map's pass map
  refreshes (`RefreshPassMapsForUnit` under REMOVE_UNIT, and the slots `RefreshMovedUnits` and `RefreshUnitIfStale`)
  compare it with the movement class's tick and stamp it again.
- **`Order::pos`:** written by `SnapWorldPosToFootprint`, a query that writes only the position it is given; the
  caller passes its order's position. It is the caller's own write through a query, not map's.

An effect for any of these would need a synchronous route, because units and orders read each of them back in the
same update (the `immediate` rows of section 4), and would only move the write.

**Payloads.** Every event and effect starts with `MapEventHeader`: `kind` and `unit` (a block offset; 0 for PASS_MAPS and CELLS_CHANGED, which name none). Each kind has the payload of `interfaces/map.h` (version 7.0, unchanged since 6.0) that starts with it, or is the header alone. `Apply(event, size)` refuses, with outcome 0 and no change, an event smaller than its kind's payload, one that names a null unit, a bucket change the unit's bucket does not allow, or a rectangle of negative size.

| Kind | Payload | Fields after the header |
|---|---|---|
| ADD_UNIT 160, REMOVE_UNIT 161, CLEAR_UNIT 162, RECLAIM_FOOTPRINT 163, BUCKET_UNLINK 165, BUCKET_PREPEND 166 | the header alone | none |
| SET_YARD 164 | `MapSetYard` | `open`: 1 to open the yard, 0 to close it |
| PASS_MAPS 168 | `MapPassMaps` | `cell[2]` (the rectangle's first cell), `size[2]` (width and height in cells) |
| CELLS_CHANGED 176 | `MapCellsChanged` | `rect[4]` (left, top, right, bottom), `classes` (a bit for each movement class whose pass maps changed), `cause` (a `PathChange`) |

## 4. Views

**What map reads:** a unit's cell, position, `zFraction`, footprint, flags, definition (footprint, yard map, slope and
water limits, `mobile`), motion and carrier chain (`carrier`, `cargo`, `cargoNext`); the cells (`los_radar.GetMapCell`
and the cell records, section below), the feature on a cell (`features.GetCellFeature`) and a feature's flags, metal
and energy; what the viewing player sees (`IsUnitVisibleToPlayer`, `los_radar.IsPointVisible` for `CanBuildAt`'s
visibility check); the players' alliance words (`pad_1df4`, `unknown_1b8e`, `unknown_1cd9`); the movement classes
(`g_movementClassEntries`: their sizes, footprints and limits, which units reads from `MOVEINFO.TDF` at load). All as
of each module's last run, read inside map's events, synchronous slots and
queries, at the caller's statement.

**The cell view.** Map writes three fields of each cell record of the cell view (`docs/los-radar-interface.md`,
section 4), all through its events:
- **`unit` and `unit2`:** the units whose footprints cover the cell (a ground and an air or water unit), written by
  ADD_UNIT, REMOVE_UNIT, CLEAR_UNIT, RECLAIM_FOOTPRINT and SET_YARD (the claim, `ClaimFootprintCells`).
- **`flags`, the footprint bit:** bit 1, a building's yard cell, which ADD_UNIT sets and REMOVE_UNIT clears where bit 0
  of the building's yard map is set. The owner and spot bits are features' (#954) and the rest are set at load.

The pass maps are not cell fields: they are map's `PassMap` records (`g_passMaps`) and their words (allocated by
`BuildAllPassMaps`), which map's PASS_MAPS and the refresh slots write and pathfinding reads through map's queries
`PassMapGet` and `PassMapInBounds`, called by its world view (`WorldPass`, `WorldInBounds`, in the players phase) with
the class's index.
Map's CELL_HEIGHTS to los_radar, published by ADD_UNIT and REMOVE_UNIT for a building's footprint, stays queued, as
#965 left it: it rewrites `high` and `low` from the cells' `height`, which nothing writes in the tick, so no read in the
phase can see a difference while it is pending (`docs/los-radar-interface.md`, section 4).

**What others read of map:**
- **Queries:** `UnitsInRange` (through the service `VisitUnitsInRange`), `GetCellMeanHeight` (features, players,
  weapons),
  `SnapWorldPosToFootprint`, `CanPlaceFootprintAt`, `IsPadSlotFree` (orders), `GetGroundHeight` (features, los_radar,
  movement, orders, scripts, targeting, units, weapons and the minimap's cursor), `PassMapGet` and `PassMapInBounds`
  (pathfinding's world view).
- **Views:** the footprint fields of the cells; `Unit::spatialBucket` and `overflowBucket` (orders, units); the claim
  bits of `Unit::flags`; the pass maps (pathfinding); `g_lastPlaceMetalSum` (ai, right after a synchronous call).

**As of map's last apply.** The queries answer from map's state with no delivery first. By #952's rule a query needs
one only when an event that can be pending writes what it reads. No route into map can be pending inside the tick
(every one that can be published there is synchronous), so every read, through a query or a view, sees map's state as
3.1c did.

**The routes.** Every route, with its reason:

| Route | Now | Why |
|---|---|---|
| units ADD_UNIT | synchronous | #952's rule. Queued it stayed equal on the tier, but it can be pending while a later read in the same phase needs what it writes: a unit created or moved in the units phase is on the cells and in a bucket that the next units' moves test (`CanPlaceFootprintAt`, `CanPlaceUnitFootprint` for a build), that orders reads (`spatialBucket`) and that `UnitsInRange` walks, all before the phase boundary. Orders' ADD_UNIT, the same write from another publisher, diverged queued (section 8). |
| orders ADD_UNIT | synchronous | The gate (section 8). |
| units REMOVE_UNIT | synchronous | The gate (section 8). |
| orders REMOVE_UNIT | synchronous | The gate (section 8). |
| units CLEAR_UNIT | synchronous | The gate (section 8). |
| units SET_YARD | synchronous | The gate (section 8). |
| units BUCKET_UNLINK | synchronous | The gate (section 8). |
| units BUCKET_PREPEND | synchronous | #952's rule. Queued it stayed equal on the tier, but a unit that leaves a carrier in the units phase is missing from its bucket until the phase boundary, and orders' patrol and repair orders and the victory conditions walk the buckets (`UnitsInRange`) later in the same phase. BUCKET_UNLINK, its pair, diverged queued (section 8). |
| units REFRESH_PASS_MAP | removed in 5.0 | Map's own synchronous slots `RefreshMovedUnits` and `RefreshUnitIfStale` instead, called at the search's statement after map's inbox is delivered (section 8). Queued in 4.0, the event diverged. |
| units RECLAIM_FOOTPRINT | synchronous | No scenario reaches it (section 8): `LoadUnit` publishes it only for a saved unit whose yard is open, and the one saved game of the suite has none. It is published only as a saved game is read, outside the tick, where #970's rule delivers it at once anyway, so the policy changes no order. |
| features PASS_MAPS | synchronous | #971 made it synchronous by #952's rule (`docs/features-interface.md`, section 4); unchanged. |
| map CELLS_CHANGED, to pathfinding | synchronous | #952's rule. Queued it stayed equal on the tier, which says little: the default pathfinding marks the stale clusters of its hierarchy from it, and the scenarios' paths came out the same. It can be pending while a later read in the same phase needs it: a path submitted later in the units phase (orders' `Submit`) reads the stale marks, and in 4.x units' own `CellsChanged` reports, direct calls into pathfinding, interleaved with map's in that phase (queued, map's would land after all of units', in another order than 3.1c's). Since 5.0 those reports are map's CELLS_CHANGED too, so the route carries them all at their statements. |
| map CELL_HEIGHTS, to los_radar | queued | Unchanged from #965 (above). |

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv`, regenerated on this change, has these rows of
class `immediate` with map as writer or reader. The cells are named by their allocation (`PLOT MEMORY`), whose one
item is every field of every cell, so a row with it is any field. Each is served as follows:

| Writer → reader | Items | Phase | How it is served |
|---|---|---|---|
| map → units, orders, ai, players, targeting, weapons, los_radar | `Unit::flags` | units | The claim bits, written inside map's synchronous events at the publisher's statement, as in 3.1c. The readers that read other bits of the word read them as before. |
| map → orders | `Unit::spatialBucket` | units | Written by the synchronous ADD_UNIT, REMOVE_UNIT and CLEAR_UNIT, at the publisher's statement. |
| map → units | `Unit::cargoNext` | units | The bucket chain, written by the synchronous ADD_UNIT, REMOVE_UNIT, CLEAR_UNIT and the bucket events, at the publisher's statement. |
| map → units | `Unit::cobStateFlags` | units | `bit2`, written by the synchronous SET_YARD. |
| map → orders | `Order::pos` | units | The caller's own position, written through the query `SnapWorldPosToFootprint` (section 3). |
| map → units, features, los_radar, weapons | `PLOT MEMORY` | units | The footprint fields, written by the synchronous events; the readers read them, or their own fields of the same records. |
| map → ai | `g_lastPlaceMetalSum` | players | Written by the synchronous `CanBuildAt` and `CanPlaceUnitFootprint`, read by the same caller right after. |
| units, orders, weapons → map | `Unit::cell`, `pos`, `zFraction`, `flags`, `footprint`, `def`, `motion`, `carrier`, `cargo`, `cargoNext`, `cobStateFlags` | units | Map reads them inside its synchronous events and slots, at the publisher's statement: REMOVE_UNIT before the publisher writes the new cell, ADD_UNIT after it, as 3.1c's calls did. |
| orders → map | `Order::pos` | units | Read by the synchronous `CanPlaceUnitFootprint` and the queries at the caller's statement. |
| units → map | `UnitMotion::pathLockStamp`, `g_movementClassEntries` | players | In 4.x read by the synchronous REFRESH_PASS_MAP, which units published from pathfinding's world view in the players phase. Since 5.0 the refresh that writes them is map's synchronous `RefreshMovedUnits` and `RefreshUnitIfStale`, run at the search's statement, so the writer is map itself; the row names units until `data/dependencies.csv` is next regenerated from access logs. |
| economy, units → map | `pad_1df4`, `unknown_1b8e`, `unknown_1cd9` | units | The players' alliance words, read by `CanBuildAt` and the claim at the caller's statement. |
| features, los_radar → map | `PLOT MEMORY`, `MAPPED MEMORY`, `FeatureAnimEntry::bytes_8`, `bytes_10` | units, features, projectiles | Features' and los_radar's fields of the cells and the spots, written by their own synchronous routes and runs; map reads them in its synchronous events and slots, after those writes, as in 3.1c. |
| weapons → map | `Debris::pos` | explosions | Read by `GetCellMeanHeight`, a query, at the caller's statement. |

## 5. State and the trace

Map keeps no private region, but it has a `WriteState` since 4.1 (#979) and it allocates and frees the bucket array.
What only map reads left the hash in trace format 6 and is reported by `WriteState`, dumped and required equal for the
default, as record 45 `mapState`: for each bucket (`spatial.count` of them) the fields `a`, `flags` and `first`, and
for the bucket of units off the map `overflow.present` and the same three. What other subsystems read stays hashed:
orders reads the byte `b` of a bucket (`src/orders/order_targets.cpp`, the bucket of a unit) through
`Unit::spatialBucket`, and orders and others compare `Unit::spatialBucket` with `overflowBucket`, so section 14 keeps
`spatial.width`, `spatial.height`, `spatial.count`, each `spatial[r].b` and `spatial.overflow.b`, and drops `a`, `flags`
and `first`.

| Map's state | Where | Read by | Only map reads it |
|---|---|---|---|
| The spatial grid: the buckets (`Game::cells_1429f`, a `UnitBucket` or `SpatialBucket` head per square of 8 by 8 cells) | state block, hashed | map | yes |
| The bucket of units off the map (`Game::overflowBucket`) | state block, hashed | map, orders | no: orders tests a unit's bucket against it |
| A unit's bucket and chain link (`Unit::spatialBucket`; `Unit::cargoNext` of a unit not carried) | the unit, hashed | map, orders, units | no |
| The footprints: `unit`, `unit2` and the yard bit of the cells | the cell records, hashed (section 11) | map, units, orders, weapons, los_radar, features | no |
| The claim bits of `Unit::flags`, `Unit::bit2` | the unit, hashed | map, units | no: units reads the word |
| The pass maps (`g_passMaps`, the 32 `PassMap` records, and the words `BuildAllPassMaps` allocates) | sim globals and state block, hashed | map, pathfinding (through its world view) | no |
| The placement record: `g_lastPlaceMetalSum` | sim globals, hashed | map, ai | no |
| The placement record: `g_lastPlaceHeight` | sim globals, hashed | map (the sim); presentation's `movies.cpp` | yes, in the simulation |

`privatestate.py --check` fails when another subsystem writes any of the items map alone writes: `Unit::spatialBucket`,
the bucket heads (`UnitBucket::first`, `SpatialBucket::head`), the pass maps and the placement record
(`g_lastPlaceHeight`, `g_lastPlaceMetalSum`). The bucket array (`cells_1429f`) and `overflowBucket` are map's own since 4.1: map's `AllocateBuckets` and
`FreeBuckets` make and free them, and los_radar's `BuildDerivedLayers` and `FreeMapResources` call the slots at the
statements that allocated and freed them (the allocations, their sizes, tags and order are those of 3.1c). The
allocation fills `a` (the highest ground in the bucket), `b` (the highest of its neighbours) and the edge bits of
`flags`, which the one function wrote together with the allocation, so it moved with it. After this change `Rec::first`
is written by `SetOwner`, `UnlinkBucketUnit`, `SpatialBucket::UnlinkUnit` and `SpatialBucket::PrependUnit` (map's
events), and `a`, `b` and `flags` by `AllocateBuckets` only. The cells are not listed (presentation, features and los_radar write other fields of the same
records, and the access log names them as one item), nor are `Unit::cargoNext`, `Unit::flags`, `Unit::bit2` and
`Unit::cobStateFlags`, which units writes too.

**The movement class split (7.0, #1002).** 3.1c's `MovementClass` was one 32-byte record per class, 32 of them in
`g_movementClasses`, holding units' definition and map's pass map side by side. Every field and what it does:

| Field (3.1c offset) | Job | Written by | Read by | 7.0 |
|---|---|---|---|---|
| `name` (+0x00) | the class's name, a game heap string; 0 marks an undefined class | units (`ReadMoveInfo`, `FreeMovementClasses`) | units (`FindMovementClass`); map's `BuildAllPassMaps` tested it to skip a class | units'. Map keeps `PassMap::defined` instead, set for each class `GetMovementLimits` returns |
| `footprintX`, `footprintZ` (+0x04, +0x06) | the footprint the pass map is built for | units (`ReadMoveInfo`) | units (`LoadUnitFbi`, into the unit type); map (the build, the refresh and the terrain values) | units' definition; map reads its own copy in `PassMap` |
| `maxWaterDepth`, `minWaterDepth` (+0x08, +0x0a) | the water depth limits of a passable cell | units | units (`LoadUnitFbi`); map (`GetPassMapCellValue`) | as the footprint |
| `maxSlope`, `badSlope`, `maxWaterSlope`, `badWaterSlope` (+0x0c to +0x0f) | the slope limits of a passable and of a costly cell | units | units (`LoadUnitFbi`); map (`GetPassMapCellValue`) | as the footprint |
| `width`, `height` (+0x10, +0x14) | the pass map's size in cells, the map's | map (`BuildAllPassMaps`, `Resize`) | map (the reads, the refresh) | map's, `PassMap` |
| `cells` (+0x18) | the pass map words, 2 bits a cell, 16 rows a word | map (`BuildAllPassMaps`, `Build`, `Refresh`), freed by units' `FreeMovementClasses` | map; pathfinding through map's queries | map's, `PassMap`; freed by map's `FreePassMaps` |
| `lastTick` (+0x1c) | the window of stale stamps of the lazy refresh | map (`RefreshMovedUnits`) | map (`RefreshPassMapsForUnit`, `RefreshUnitIfStale`) | map's, `PassMap` |

Units keeps the first 16 bytes (`MovementClass`, a 512-byte `g_movementClasses`); map keeps `PassMap` (32 bytes:
a copy of the 12 bytes of limits, `defined`, `width`, `height`, `cells` and `lastTick`), 32 of them in `g_passMaps` in
the sim globals, a row of `data/state_classes.csv` of its own. The sim globals grow by 512 bytes, and the fields after
the table move. At load the engine (`LoadBattleAssets`) asks units for the limits of the named classes into a buffer it
owns (`GetMovementLimits(out, capacity)`, which returns the count, each entry carrying its class index) and hands them
to `BuildAllPassMaps(limits, count)`, which copies them; no memory changes owner across the seam. The progress bar
counts the classes `BuildAllPassMaps` was given, as 3.1c counted the named ones.

Every place a `MovementClass*` crossed a seam in 6.0, and what it became:

| Where | 6.0 | 7.0 |
|---|---|---|
| map's slots `MovementClassGet`, `MovementClassInBounds`, `RefreshMovedUnits`, `RefreshUnitIfStale` | a `MovementClass*` into units' table | the class index (`PassMapGet`, `PassMapInBounds`, and the two refreshes) |
| map's `BuildAllPassMaps` | read every class of units' table | the limits as `MapMoveLimits` |
| pathfinding's world view (`src/seams/pathfinding_world.cpp`: `WorldPass`, `WorldInBounds`, `WorldTerrainPass`, `WorldBeginSearch`, `WorldEndSearch`) | passed the class pathfinding names (`PathClass`, the block offset of units' record) or the motion's `owner` to map | turns it into the index first (`ClassIndex`); `PathClass` and the pathfinding interface are unchanged |
| `UnitMotion::owner` and `movementClass` (`src/movement/unit_motion.h`), `UnitDef::movementclass` (`src/units/unit_def.h`, and its view in `src/movement/unit_def_move_view.h`) | `Ref`s to units' record | unchanged: they name the definition, which stays units'; map never reads them |
| units' `LoadUnitFbi` (`src/units/unit_types.cpp`) | read the definition and named the class by `FindMovementClass` | unchanged, inside units |
| map's terrain values (`src/map/terrain.cpp`: `GetPassMapCellValue`, `GetPassMapFootprintValue`, `GetPassMapFootprintValueWithEdgeStrips`, `FootprintValue`, `FootprintValueWithEdgeStrips`, `GetTerrainPassValue`) | took a `MovementClass*` | take map's `PassMap*`, and `GetTerrainPassValue` the index |
| the replay (`ROK_MOVEMENTCLASS`) | one object kind for the whole record, the slot `MovementClassGet` recorded its class as an object | `MovementClass` holds the definition fields only, a new kind `PassMap` (`g_passMaps.entries`) the pass map fields, and the slots record the index |
| `src/state/layout_asserts.cpp`, `src/state/sim_globals.*` | the 32-byte record | the generated asserts of both records, and `g_passMaps` in `SimGlobals` |

**The trace does not change.** The harness hashes the same 32-byte `MoveClass` record for each class
(`kMoveClassTable`, which `tools/traceschema.py` keeps as a synthetic 32-byte `MovementClass`): bytes 0 to 15 from units' `g_movementClasses` and bytes 16 to 31 from
map's `g_passMaps`, in 3.1c's order, and the `moveclass` refs (a unit's motion and type) resolve to the same index, as
the stride is now units' 16-byte record. So the format and every stored trace are unchanged, and the gate is the tier.

**The trace dumps map's events** in section 40 (`docs/trace-format.md`, section 8.28), after los_radar's section 39,
and hashes none of them. Each record is an event with the outcome `Apply` gave, or a CELLS_CHANGED map published, in
map's order. The section is new, so the trace format's schema changes and the gate of this change is the full suite.
Since 5.0 the dump has no REFRESH_PASS_MAP records and a CELLS_CHANGED for each pass map the refreshes around a
search compute again; nothing hashed changes, so the gate of 5.0 is the tier.

## 6. Random draws and sim events

Map draws from one stream, in one place: `PickRandomReclaimableResourcesInRadius` draws `rng.parkMiller` three times
for each kind of resource it found (`data/draws.csv`: 3 to 6 draws, in the units phase). No event of map draws.

The class `ordered` of `data/dependencies.csv` has the edges on `rng.parkMiller` in the units phase between map and
each of orders, targeting, units and weapons, both ways: their draws interleave with map's within a phase. Each is
kept in 3.1c's order because map's draws happen only inside the synchronous `PickRandomReclaimableResourcesInRadius`,
at the statement of orders' patrol and repair orders that call it, between the draws those subsystems make before and
after it in the same unit's update and the next. A queued event of map could not move a draw, as none draws.

Map records no sim event.

## 7. What stays inside the default

3.1c's types and limits are the default's, not the interface's: the cell record's layout, the bucket grid (a bucket
per 8 by 8 cells, 10 bytes each), the overflow bucket, the yard map's encoding, the pass map words and their edge
strips, the 32 movement classes, the placement record, and the scratch lists of `PickRandomReclaimableResourcesInRadius`.
An event carries a unit as its block offset, a movement class as its index, and a rectangle in cells. The set-up,
save and load slots stay in `MapSeam` as 3.1c's code. The pass map slots (`PassMapGet`, `PassMapInBounds`,
`RefreshMovedUnits`, `RefreshUnitIfStale`) take the class by its index since 7.0, and the refreshes a `Unit*`, of which
they read the motion's stamp, its cell and footprint: 3.1c's types where the change needed no new decision. The 32
classes and the copied limits (`MapMoveLimits`) are the default's too. The window of 30 ticks and the stamps are the default's. The movement class definition, which
`MOVEINFO.TDF` gives, is units' (section 2); the mission, campaign and map-list slots are the mission interface's.

## 8. What the gate decided

Every route into map was queued first, each on its own, with the others synchronous. The tier judged each build,
and a route whose queueing diverged became synchronous. The divergences are against the reference, with the first
tick, phase and field.

| Route queued | Tier | First divergence |
|---|---|---|
| units ADD_UNIT | equal | none (synchronous by #952's rule, section 4) |
| orders ADD_UNIT | diverged | orders-air-fights, tick 1276, units phase: `units[4].field_64`, then 96 fields of the units' headings, positions and motion. |
| units REMOVE_UNIT | diverged | mission-capture, tick 1, world phase: `cells[561]` still names unit 5 (reference `00…`, queued `05…`). |
| orders REMOVE_UNIT | diverged | mission-radius, tick 20, world phase: `cells[3408]` still names unit 9. |
| units CLEAR_UNIT | diverged | conditions-dark-side: equal to tick 451, then the game exits with code 3 during tick 452. |
| units SET_YARD | diverged | orders-air, tick 1024, units phase: `units[2].bit0` (reference 0x04, queued 0x0c), then the unit's script channels. |
| units BUCKET_UNLINK | diverged | orders-air, tick 1025, world phase: `spatial[61].first` (reference unit 2, queued none), then the game exits with code 3. |
| units BUCKET_PREPEND | equal | none (synchronous by #952's rule, section 4) |
| units REFRESH_PASS_MAP | diverged | mission-radius, tick 60, world phase: `pathfinder.used` (reference 0x1f1, queued 0x1d9), then unit 1's path (`units[1].motion.goal`). |
| map CELLS_CHANGED | equal | none (synchronous by #952's rule, section 4) |

With every route as above, the tier was equal (38 of 38 scenarios), and so was the full suite in both modes (178 of
178 games), the gate of this change.

**How often each route is taken.** A build that counts every publish into and out of map, over the 89 scenarios with
native modules: the events published inside the tick and outside it (when a game is set up or loaded), and in how
many scenarios each was published at all.

| Route | Inside the tick | Scenarios | Outside the tick | Scenarios |
|---|---|---|---|---|
| units ADD_UNIT | 2,050 | 55 | 1,424 | 89 |
| orders ADD_UNIT | 349,229 | 77 | 0 | 0 |
| units REMOVE_UNIT | 58 | 5 | 0 | 0 |
| orders REMOVE_UNIT | 349,229 | 77 | 0 | 0 |
| units CLEAR_UNIT | 937 | 35 | 4 | 1 |
| units RECLAIM_FOOTPRINT | 0 | 0 | 0 | 0 |
| units SET_YARD | 1,775 | 23 | 0 | 0 |
| units BUCKET_UNLINK | 488 | 25 | 23 | 2 |
| units BUCKET_PREPEND | 414 | 18 | 0 | 0 |
| units REFRESH_PASS_MAP | 44,107 | 75 | 0 | 0 |
| map CELLS_CHANGED | 116,056 | 80 | 21,557 | 29 |
| map CELL_HEIGHTS | 1,848 | 58 | 337 | 27 |

Features' PASS_MAPS is not counted here: it reaches map through features' effect (`docs/features-interface.md`).

**Inbox depths.** Map's inbox was never deeper than 0 at a phase boundary in any scenario: every route into map that
is taken inside the tick is synchronous, so nothing waits in it. The most map events published inside one unit's
update (`UpdateUnit`) was 4, in mission-timer at tick 186 (unit 361).

**Map 5.0 (#983).** The pass maps moved with the refresh as map's own work, the first of #986's two choices, and the
tier was equal (38 of 38 scenarios); the fallback (only the two reads move, the refresh stays a synchronous slot of
units) was not needed. The CELLS_CHANGED effects of the refreshes reach pathfinding through the synchronous route at
the statements units' direct calls stood at, so no draw or write moved.

**Map 7.0 (#1002).** The pass maps left units' `MovementClass` for map's own `PassMap` table, and map's slots take the
class index. The trace format and every stored trace are unchanged (section 5), and the tier was equal in both modes
(76 of 76 games, 38 scenarios). The sim globals grew by 512 bytes, so the smoke, full, music and mp play-tests were
run too, and passed.
