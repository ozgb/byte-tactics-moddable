# The los_radar interface

Los_radar sits behind `interfaces/los_radar.h` (version 5.1; 5.1 adds the console's kind SET_MODE; 5.0 writes the units of `GetVisibleEnemiesInRadius` to a caller buffer, in place of a vector and its list twin, and `HasReadyUnitInRange` returns int instead of bool and takes a const position), the fourth module on the scheduler after economy
(`docs/economy-interface.md`), features (`docs/features-interface.md`) and targeting (`docs/targeting-interface.md`)
(#965). It is line of sight, radar and sonar: what each player sees and has explored, the eyeballs (temporary views),
the terrain height range of the cells, the radar picture's snapshots, and (since 3.1) each player's index of the enemy units it sees. 3.1c's code is its default implementation.
This page is the inventory the interface was cut from:
- what other subsystems did to los_radar's state in 3.1c, and what each became;
- what los_radar did to others, and what each became;
- what los_radar reads, and what others read of it, the map's cells among them;
- how the order of sim events is kept;
- what the trace hashes, and what it only dumps.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, events published
outside the tick delivered at once (#970), and events as plain data. This issue changes none of its rules. Event
kinds are unique across interfaces: economy uses 0 to 4 and 16 to 18, features 32 to 35 and 48 to 51, wind_meteors 64
to 67 and 80, targeting 96 to 99 and 112, los_radar 128 to 139 (effects would take 144 to 159; it has none).

Los_radar's place in it:
- **Events into los_radar:** UPDATE_UNIT, REMOVE_UNIT, REVEAL_UNIT, ADD_EYEBALL, CELL_HEIGHTS, RECALCULATE,
  SHARE_MAPPING, SHARE_RADAR and SHARE_MAP, published by units, orders, map, players and the engine; and,
  since 3.2, SIGHT_RESET and SIGHT_LOAD, which units publishes (`LosRadarPublishSightReset`, `LosRadarPublishSightLoad`) as it sets a unit up and loads a saved game; and, since 5.1, SET_MODE, which the engine's console publishes (`los`, `lostype`, `mapping`, `nowisee`) in place of its writes to the line of sight bits of the game record, each before its RECALCULATE.
  Los_radar's handler (`LosRadarInbox`, `src/seams/los_radar_world.cpp`) passes each to `Apply`.
- **Routes:** of the fifteen routes, every one that can be published inside the tick is synchronous except map's
  CELL_HEIGHTS: three because the gate diverged with them queued, two by #952's rule, and three because no scenario
  reaches them, so nothing has tested them queued (sections 4 and 8). The engine's
  RECALCULATE, SET_MODE, SHARE_MAPPING and SHARE_RADAR are queued, and since they are published outside the tick they are
  delivered at once.
- **Runs:** the engine runs los_radar's passes where 3.1c ran them, in `SchedPlayersPhase` (`src/game/scheduler.cpp`):
  the `players.los` pass, `UpdateUnitLineOfSight` for each unit slot of a player that holds a unit, between
  `UpdatePlayerHead` and `UpdatePlayerBody`; for the viewing player, `RecordRadarUnits` after it, and
  `UpdateSensorRadarAndCloak` and `RecordRadarMapped` after the player's economy and before `UpdatePlayerTail`; and,
  once a tick in the tail, `ExpireEyeballs` (`SchedExpireEyeballs`). Each is preceded by a delivery of los_radar's
  inbox. Every 30 ticks of a player, in the same players phase and right before ai's `UpdatePlayerAI`, the engine runs
  `RefreshUnitIndex(player)` (section 5), with the inbox delivered first. Players no longer calls any of them: its update is split into `UpdatePlayerHead`, `UpdatePlayerBody` and
  `UpdatePlayerTail` (players 2.0), and the head's and body's answers tell the engine whether the player goes on.
- **Queries:** `IsUnitVisibleToPlayer`, `SoundHeardMask` and `ClampWorldPosToTerrain`, and, since 3.1,
  `GetVisibleEnemiesInRadius`, `HasReadyUnitInRange`, `IsPointVisible` and
  `GetSeenUnitsInRadius`, called directly, as of los_radar's last apply (section 4).
- **Views:** the cell records, through `GetMapCell`, `GetMapCellAtPosition` and `GetOriginCellAtPosition`
  (section 4), and the sensor bits of `Unit::flags` (section 3).
- **Effects out of los_radar:** none. What it changes in others' records stays view state that los_radar alone writes,
  in its own passes (section 3).
- **A new module id:** `SCHED_LOS_RADAR = 11` (services 1.15), so `SCHED_MODULE_COUNT` is 12.

## 2. Events into los_radar

| 3.1c site | 3.1c call | Now |
|---|---|---|
| units: a unit moved or placed (`SetUnitPosition`), a unit's state read from a packet (`ReadUnitState`) | `UpdateUnitLineOfSight` | UPDATE_UNIT from units |
| orders: a moving unit's position (`UnitMotion::UpdatePosition`) | `UpdateUnitLineOfSight` | UPDATE_UNIT from orders |
| players: every unit of a player (`RefreshOwnedUnitsLineOfSight`, which nothing calls) | `UpdateUnitLineOfSight` | UPDATE_UNIT from players |
| players: the players phase's pass over each unit slot of a player | `UpdateUnitLineOfSight` | the engine's `players.los` pass (section 1) |
| units: a unit dies (`ApplyUnitDeath`) | `RemoveUnitLineOfSight`, `AddEyeball` (the dying unit's last view) | REMOVE_UNIT, ADD_EYEBALL from units |
| units: a unit is created (`CreateUnit`, `CreateUnitFromPacket`) | `RevealNewUnit` | REVEAL_UNIT from units |
| map: a unit added to or taken off the map (`AddUnitToMap`, `RemoveUnitFromMap`) | `UpdateCellHeightRange` | CELL_HEIGHTS from map |
| players: the defeated local player's commander placed again, or the defeated local player made a watcher (`UpdatePlayerBody`) | `RecalculateLineOfSight(1)` | RECALCULATE from players |
| the console (`CmdLOS`, `CmdLOSType`, `CmdMapping`, `CmdNowISee`) | writes to the line of sight bits of the game record | SET_MODE from the engine, before the RECALCULATE |
| the console (`CmdLOS`, `CmdLOSType`, `CmdMapping`, `CmdNowISee`), the load of a match (`LoadMatch`) | `RecalculateLineOfSight` | RECALCULATE from the engine |
| the console (`ShareMapping`, `ShareRadar`) | `CmdShareMapping`, `CmdShareRadar` (slots) | SHARE_MAPPING, SHARE_RADAR from the engine |
| the console (`ShareAll`, economy's code) | `CmdShareMapping`, `CmdShareRadar` | SHARE_MAPPING, SHARE_RADAR from the engine |
| the network: a share-map packet (`HandleNetPackets`), the share dialog (`HandleShareDialogEvent`) | `ShareMapInfo`, inline in 3.1c's units code | SHARE_MAP from the engine |
| players: the viewing player's radar snapshot, sensors and mapped snapshot | `RecordRadarUnits`, `UpdateSensorRadarAndCloak`, `RecordRadarMapped` | the engine's passes (section 1) |
| the tail of a tick | `ExpireEyeballs` | the engine's pass (section 1) |

**`ShareMapInfo`.** 3.1c's `ShareMapInfo` (0x485420) wrote a player's visibility into another's, and sat with units'
code, classified economy. It is los_radar's state, so the function moved into los_radar's default
(`src/map/line_of_sight.cpp`) and its two callers, both outside any module's run (the net phase's packet handler and
the share dialog), publish SHARE_MAP.

**The share commands.** `CmdShareMapping` and `CmdShareRadar` were slots, called by the console and by `ShareAll`.
They toggle a player's sharing flag, which los_radar reads, and announce it. The console runs outside the tick, so
#964's rule (wind_meteors' console commands) applies: the command publishes an event and the event is delivered at
once. The console's handlers stay in the engine (`ConsoleShareMapping`, `ConsoleShareRadar`); the toggles are the
default's `CmdShareMapping(player)` and `CmdShareRadar(player)`, reached only through `Apply`.

**Answers.** No caller used a value: 3.1c's `UpdateUnitLineOfSight`, `RemoveUnitLineOfSight`, `RevealNewUnit`,
`AddEyeball`, `UpdateCellHeightRange`, `RecalculateLineOfSight` and `ShareMapInfo` returned nothing, and the share
commands' results were thrown away. So no slot of los_radar is synchronous by its contract: every same-statement
order is a route's.

**Outcomes.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 39) and returned to no
one: 1 for every kind, except ADD_EYEBALL (0 when line of sight is off or all eyeballs are in use), SHARE_MAPPING and
SHARE_RADAR (0 outside a network game). `Apply` first rejects an event naming no unit (UPDATE_UNIT, REMOVE_UNIT,
REVEAL_UNIT) or a player outside 0 to 9 (the SHARE kinds), with outcome 0 and no change. 3.1c's callers could publish
none of these, so no trace moves; a module publishing from WASM can.

**Every slot of 2.1.** The 22 slots of los_radar 2.1, and what each became in 3.0:

| 2.1 slot | 3.0 |
|---|---|
| `GetMapCell`, `GetMapCellAtPosition`, `GetOriginCellAtPosition` | views: the cell records (section 4) |
| `ClampWorldPosToTerrain`, `IsUnitVisibleToPlayer`, `SoundHeardMask` | queries |
| `UpdateUnitLineOfSight` | the engine's `players.los` pass; its other callers publish UPDATE_UNIT |
| `RecordRadarUnits`, `UpdateSensorRadarAndCloak`, `RecordRadarMapped`, `ExpireEyeballs` | the engine's passes |
| `RemoveUnitLineOfSight`, `RevealNewUnit`, `AddEyeball`, `UpdateCellHeightRange`, `RecalculateLineOfSight` | events: REMOVE_UNIT, REVEAL_UNIT, ADD_EYEBALL, CELL_HEIGHTS, RECALCULATE |
| `CmdShareMapping`, `CmdShareRadar` | events: SHARE_MAPPING, SHARE_RADAR |
| `LoadTntMap`, `InitRadar`, `FreeRadar`, `FreeMapResources` | set-up slots, outside the tick |

**The slots of 3.1.** Los_radar 3.1 adds nine slots (#991), five queries and the index's four:

| Slot | What it is |
|---|---|
| `GetVisibleEnemiesInRadius` | query, formerly ai's: the ready enemy units within a radius of a position that a player saw at its last `RefreshUnitIndex`, or, when it saw none there and has a finished unit of a type with flag 0x400 switched on, the ready enemy units with flag 0x100 there; it writes the first `capacity` of them to the caller's buffer and returns how many there are. Its callers are orders' guard and wait and targeting's weapon search. |
| `HasReadyUnitInRange` | query, formerly ai's: whether an enemy unit the player saw is within a range, ready (flag 0x10000000) and not dying (flag 0x4000); the info panel calls it |
| `IsPointVisible` | query, formerly ai's: whether the viewing player sees a position, on the grid of the player's explored map; weapons' drawing and players' `IsVisible` call it |
| `GetSeenUnitsInRadius` | new query, the buffer form of the visible query with no ready filter and no known fallback; ai's spatial timer sums its ratings from it (it read the list directly before) |
| `CreateUnitIndex`, `FreeUnitIndex` | set-up slots, outside the tick: players' set-up creates and frees a player's index beside ai's per-player state (`InitPlayerSlot`, `FreePlayers`), under the same condition |
| `RefreshUnitIndex` | the engine's run every 30 ticks of a player that has an index (section 5) |

**The slots and events of 3.2.** Los_radar 3.2 adds five slots at the end, and two event kinds (#979):

| Name | What it is |
|---|---|
| `RegionSize`, `Init`, `Shutdown` | as economy's: the engine asks for the bytes of the private region for a map (`LosRadarMapInfo`, the unit slots), allocates it in `LoadBattleAssets`, gives it to `Init`, and frees what `Shutdown` returns |
| `SightCache` | query: fills a `LosSightCache` (`cell`, `frameIdx`) with a unit's cache, for a saved game |
| `WriteState` | writes record 44 `losRadarState` and the hashed records of section 24 (section 5) |

**The slots of 4.1.** Los_radar 4.1 adds two set-up slots at the end, outside the tick, which left map's seam (map 8.0, `docs/map-interface.md`):

| Slot | What it is |
|---|---|
| `LoadMappingData`, `SaveMappingData` | read and write the visibility mask, the allocation `LoadTntMap` tags "MAPPED MEMORY", as the "Mapping" account of a saved game; `LoadSavedGameState` and `SaveGameFile` call them |
| SIGHT_RESET (137) | event: sets a unit's cache back to zero, as units' set-up did to the bytes; outcome 1, 0 for a null unit |
| SIGHT_LOAD (138) | event: sets a unit's cache to the `cell` and `frameIdx` of the payload (`LosRadarSightLoad`), as a saved game restores it; outcome 1, 0 for a null unit |
| SET_MODE (139) | event: `mapFlags` = (`mapFlags` & ~`clear`) \| `set` (`LosRadarSetMode`), the settings bits 0 to 2; outcome 1, 0 when `clear` or `set` has a bit outside 0x7 |

## 3. Effects out of los_radar

Los_radar publishes nothing. The issue counted two kinds of write into others' records; each stays view state that
los_radar alone writes, in its own passes, at 3.1c's statement:

- **`Unit::flags`, the sensor bits.** `UpdateSensorRadarAndCloak` clears 0x1000 and sets or clears 0x100, 0x200 and
  0x400 on every unit: 0x100 is seen by the viewing player (own or allied units, radar, or the cell in sight), 0x200 is
  on sonar, 0x400 is jammed, 0x1000 is a cloaked unit revealed by an enemy in its decloak distance (with its hold-off,
  `Unit::workTime`, 90 ticks). Ai, orders, players, targeting and units read them in the players phase (the five
  `immediate` rows of section 4). The pass is an engine run, not an event: it runs at 3.1c's place in the viewing
  player's update, so every reader sees the bits as 3.1c left them. The bits stay in the hashed `Unit` record. An
  effect to units would have to be synchronous for the same reason, and would only move the write.
- **`bits_14281`, the line of sight settings of the game record.** Bits 0 to 2 (mapping, line of sight and its type)
  are written only outside the tick, by the console (through SET_MODE since 5.1), the options and the load of a match, and read by los_radar and
  units as settings. Bit 3 is a redraw request for the fog of war, set by `RecalculateLineOfSight` and
  `RevealAroundUnit` and cleared by the fog's drawing; the trace masks it out (0xfff7). The field stays in the game
  record as view state: the settings' writers are the engine's, and los_radar writes only bit 3.

**Payloads.** Every event starts with `LosRadarEventHeader`, which holds `kind` alone. Each kind has the payload of `interfaces/los_radar.h` (version 5.1; the payloads are unchanged since 4.1 besides the new kind) that starts with it. `Apply(event, size)` refuses, with outcome 0 and no change, an event smaller than its kind's payload, one that names a unit with a null unit, and a SET_MODE with a bit outside 0x7 in `clear` or `set`.

| Kind | Payload | Fields after the header |
|---|---|---|
| UPDATE_UNIT 128, REMOVE_UNIT 129, REVEAL_UNIT 130, SIGHT_RESET 137 | `LosRadarUnit` | `unit`: the unit, a block offset |
| ADD_EYEBALL 131 | `LosRadarAddEyeball` | `pos[3]` (world coordinates, 16 fractional bits), `sightDistance` (world units), `eyeHeight` (world units), `lifetime` (ticks) |
| CELL_HEIGHTS 132 | `LosRadarCellHeights` | `cell[2]` (the rectangle's first cell), `size[2]` (its width and height in cells) |
| RECALCULATE 133 | `LosRadarRecalculate` | `reset`: 1 to first reset what each cell shows |
| SHARE_MAPPING 134, SHARE_RADAR 135 | `LosRadarSharePlayer` | `player`: the player whose sharing toggles |
| SHARE_MAP 136 | `LosRadarShareMap` | `from` (the player whose sight is shown), `to` (the player it is shown to) |
| SIGHT_LOAD 138 | `LosRadarSightLoad` | `unit`, `cell` (as `LosSightCache::cell`), `frameIdx` (as `LosSightCache::frameIdx`, 0 to 255) |
| SET_MODE 139 | `LosRadarSetMode` | `clear`, `set`: the `mapFlags` bits to clear (first) and to set, within 0x7 |

## 4. Views

**What los_radar reads:** a unit's position and fractions, flags, `activateFlags`, definition (sight, radar and sonar
distances, jamming, cloak), player and the sight cache (`Unit::losCacheCell`, `losCacheCellZ`, `losSightFrameIdx`);
the players' alliances, sharing flags and the fields of the game record behind `pad_1df4`, `unknown_1b8e` and
`unknown_1cd9` (a player's alliance and sharing words); projectiles' positions (the radar snapshot); and map's `UnitsInRange`. The
queries `HasReadyUnitInRange` and `IsPointVisible` are its own since 3.1; they were ai's in 3.0. All as of each
module's last run.

**The cell view.** `GetMapCell`, `GetMapCellAtPosition` and `GetOriginCellAtPosition` return a pointer into the cell
records that `LoadTntMap` allocates (the plot memory), and their callers (economy, features, map, units, weapons)
read fields of it. The decision here is a view of plain cell records as of each owner's last run, not a query per
field: the record is a fixed 3.1c layout every caller already reads, and splitting it into queries would add a call
per field read in the units phase's hottest loops for no change in what is read. The view is read only; each field
has one owner:
- **los_radar:** `high` and `low`, the height range of a cell and its neighbours, written by CELL_HEIGHTS
  (`UpdateCellHeightRange`), and `height` and `metal`, written by `LoadTntMap` when the map loads and by nothing in the
  tick.
- **map:** `unit` and `unit2`, the units standing on the cell, and the cell's passability bits of `flags`.
- **features:** `feature` and `spot` (or the footprint offsets) and the owner and spot bits of `flags` (#954,
  `docs/features-interface.md`, section 4).

The layers of what each player sees and has explored and the eyeballs are not in the cell records: they are
los_radar's own allocations (section 5).

**What others read of los_radar:**
- **Queries:** `IsUnitVisibleToPlayer` (the index refresh, orders' and unit scripts' visibility tests, the selection
  and markers), `SoundHeardMask` (which players hear a sound), `ClampWorldPosToTerrain`, and the five of 3.1 (orders'
  guard and wait, targeting's weapon search, the info panel, players, weapons and ai's spatial
  timer; section 2).
- **Views:** the cell records (above), and the sensor bits of `Unit::flags` (section 3).

**As of los_radar's last apply.** The queries answer from los_radar's state with no delivery first. By #952's rule a
query needs one only when an event that can be pending writes what it reads. The only route that can be pending in
the tick is CELL_HEIGHTS, and no query reads `high` or `low`. So every read, through a query or the view, sees
los_radar's state as 3.1c did.

**The routes.** Every route, with its reason:

| Route | Now | Why |
|---|---|---|
| units UPDATE_UNIT | synchronous | #952's rule. `SetUnitPosition` runs in the units phase, and later in the same phase orders (`order_dispatch.cpp`) and the piece queries (`src/scripts/piece_queries.cpp`) test visibility through `IsUnitVisibleToPlayer`, and map's `CanPlace` reads what the viewing player sees: with the move pending they would see the unit's sight where it was. |
| orders UPDATE_UNIT | synchronous | The gate (section 8). |
| players UPDATE_UNIT | synchronous | Untested. `RefreshOwnedUnitsLineOfSight` has no caller, so no scenario publishes it, and a route nothing exercises takes 3.1c's order. |
| units REMOVE_UNIT | synchronous | The gate (section 8). |
| units REVEAL_UNIT | synchronous | The gate (section 8). |
| units ADD_EYEBALL | synchronous | #952's rule. A unit's death adds an eyeball of the viewing player at its place, in the units phase among others, and later in the phase orders, unit scripts and map read the viewing player's sight through the same queries as above. |
| map CELL_HEIGHTS | queued | It writes `high` and `low` from the cells' `height`, which nothing writes in the tick (`data/rwsets.csv` has no write of `Cell::height`; `LoadTntMap` fills it at load, and `UpdateAllCellHeightRanges` then sets every cell's range). So the recompute writes the values the cells already hold, and no read in the phase can see a difference while it is pending. |
| players RECALCULATE | synchronous | Untested: no scenario publishes it (section 8). The argument against queueing it stands too, by #952's rule: the body of a player's update publishes it when the defeated local player's commander is placed again (the commander death option 2) or the defeated local player becomes a watcher, and the next players' updates in the same phase read sight through `IsUnitVisibleToPlayer` (ai's visible list) before any delivery of los_radar's inbox. |
| engine RECALCULATE, SET_MODE, SHARE_MAPPING, SHARE_RADAR, SHARE_MAP | queued, delivered at once | Published outside the tick (the console, the load of a match, the share dialog), so #970's rule delivers each at once, in 3.1c's order. |
| engine SHARE_MAP | synchronous | Untested. The share-map packet is handled in the net phase, inside the tick, and no scenario receives one, so the route takes 3.1c's order. |

The engine's SHARE_MAP row is both: the table holds one row, synchronous, which matters only inside the tick; the
dialog's SHARE_MAP is outside it and delivered at once either way.

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv`, regenerated on this change, has these rows of
class `immediate` with los_radar as writer or reader. Each row is served as follows:

The access log names the player's fields by the `Game` union aliases (`unknown_1b8e`, `pad_1df4`, `unknown_1cd9`),
the cell array by its allocation (`PLOT MEMORY`) and the visibility mask by its own (`MAPPED MEMORY`); an allocation
row is one item for every field of every cell, so two subsystems that write and read different fields of the same
cells share a row.

| Writer → reader | Items | Phase | How it is served |
|---|---|---|---|
| units, orders, map, ai → los_radar | `Unit::flags`, `activateFlags`, `def`, `pos`, `yFraction`, `zFraction`, and the sight cache units sets up (`losCacheCell`, `losCacheCellZ`, `losSightFrameIdx`) | units, players | Los_radar reads them in its synchronous events (UPDATE_UNIT, REMOVE_UNIT, REVEAL_UNIT, ADD_EYEBALL) at the publisher's statement, and in its passes (the `players.los` pass, `RecordRadarUnits`, `UpdateSensorRadarAndCloak`), which the engine runs at 3.1c's place. The one event queued in the tick, CELL_HEIGHTS, reads none of them. |
| economy, players, units → los_radar | the player's fields (`pad_1df4`, `unknown_1b8e`, `unknown_1cd9`) | units, players | The same: the alliance and shared-sight bits are read by the synchronous events and the passes at 3.1c's statement. |
| players → los_radar | `bits_142f1` | tail | One byte holds players' blink bit, which the tail toggles, and los_radar's map-changed bit, which `ExpireEyeballs` sets as a read-modify-write when an eyeball expires. The engine runs `ExpireEyeballs` at 3.1c's place in the tail, after players' toggle, so each write lands on the other's bit as before. |
| weapons → los_radar | `Projectile::position` | projectiles | Read only by the queries `GetMapCellAtPosition` and `SoundHeardMask`, at the caller's statement. |
| map, features → los_radar | `PLOT MEMORY` | units | Map writes the cells' `unit`/`unit2` and passability bits and features the feature fields (section 3); los_radar reads `height`, `high` and `low`, which neither writes. The features row is the one CELL_HEIGHTS a pending event shares a tick with (sensors-floating-teeth-los-off, tick 10): disjoint fields, so its order does not matter. |
| los_radar → units, map, features, orders, weapons | `PLOT MEMORY` | units | Los_radar's only write to the cells is `high` and `low`, by CELL_HEIGHTS, with the values they already hold (section 4). The readers' rows are their reads of their own fields or of `high`/`low`, which a pending CELL_HEIGHTS cannot change. |
| los_radar → map | `MAPPED MEMORY` | units | The visibility mask, written by the synchronous UPDATE_UNIT, REMOVE_UNIT, REVEAL_UNIT and ADD_EYEBALL at the publisher's statement, as in 3.1c. |
| los_radar → units | `bits_14281` | units | Los_radar writes bit 3, the fog redraw request, from `RevealAroundUnit` under the same synchronous events, as a write of the whole byte; units reads the settings bits 0 to 2 of that byte. The write lands at the publisher's statement, as in 3.1c. |
| los_radar → ai, orders, targeting, units | `Unit::flags` | players | The sensor bits (0x100, 0x200, 0x400, 0x1000), written only by `UpdateSensorRadarAndCloak`, which the engine runs at 3.1c's place between players' updates. |

## 5. State and the trace

A saved game gives los_radar back the visibility layer, from the "Mapping" account, through its own `LoadMappingData`, which copies the box into the mask in chunks, so that the write is los_radar's and not the files module's.

Los_radar has a private region since 3.2 (`RegionSize`, `Init` and `Shutdown`, as economy's), allocated in
`LoadBattleAssets` with the tag "LosRadar region" and freed in `ShutdownIngameSystems`, and a `WriteState` that reports
what left the hash: record 44 `losRadarState` (the sight cache of every unit slot and the eyeballs), dumped and
required equal for the default, and the hashed records of section 24 (the unit index). Section 24 and record 44 are
written in one call, record 44 first.

- **The sight, radar and mapped layers and the eyeballs** are los_radar's own allocations in the state block: the
  visibility mask (a 16-bit word per square of the map, a bit per player that sees it, in the allocation `LoadTntMap` tags "MAPPED
  MEMORY"), each player's explored grid (a count per cell of the units that see it, allocated by `InitPlayerSlot`),
  and the eyeball table (`eyes`) with its count. The mask and the explored grids are hashed (sections 12 and 13 of the
  trace): they are what other subsystems read (`IsUnitVisibleToPlayer`, `IsPointVisible`, ai's and orders' reads of the
  visible lists, the radar picture), the contract a replacement is judged on, and the hash is what shows a wrong sight at
  the tick it happens (the negative test, `tests/los_radar_writer_test.cpp`).
  `privatestate.py --check` fails when another subsystem writes any of them.
- **The eyeballs** (`eyes` and `count_14277`) left the hash in trace format 6 (section 15 is gone). Only los_radar
  reads them (`ExpireEyeballs`, `AddEyeball`, and the sight of the eyeball it draws), and their effect is in the mask.
  They stay in the allocation `LoadTntMap` makes, and `WriteState` dumps them in record 44 (`eyes.count` and, per
  eyeball, `player`, `x`, `flagA`, `flagB`, `pos`, `expires` and `screenPos`).
- **The per-player unit index** (3.1) is los_radar's: per player, the visible enemy list (`visible`), the known list
  (`known`, enemy units with flag 0x100) and `hasSpecial`, in a record allocated in the sim block behind the sim global
  `g_losUnitIndex`, a `Ref` (`src/map/line_of_sight_index.cpp`). It left ai's `PlayerAI`, where 3.1c kept it. `CreateUnitIndex` and
  `FreeUnitIndex` make and free it, and every 30 ticks, from the index's own `lastTick`, `RefreshUnitIndex` refills it
  in the players phase right before ai's `UpdatePlayerAI`. 3.1c filled it, with the factory list (now units'), in one
  walk of the units; the walks are now separate, but each list holds the same units in the same order, and no draw
  moves. Since trace format 6 `WriteState` writes the index: section 24 holds one record `losRadarIndex[i]` for each live
  player (`player`, `hasSpecial`, then `visible.size` and `visible[k]`, `known.size` and `known[k]` as unit handles),
  hashed, at the place where the harness read the index before, so a replacement los_radar's lists are the ones
  hashed, not the default's. The harness no longer reaches into the index.
- **The unit sight cache** (3.2) moved out of the unit into the region: one entry per unit slot (`LosSightCache`: the
  `cell` of the last sight circle, x in the low 16 bits and z in the high 16, and the `frameIdx` of the table frame).
  Only `UpdateUnitLineOfSight`, `RemoveUnitLineOfSight`, `RevealNewUnit` and `RecalculateLineOfSight` read or write it.
  The 12 bytes of the unit record stay, unused (`Unit::unusedLosCache` at 0x7a and `unusedLosSightFrame` at 0xf8), so
  no other offset moves, as economy's 48. Units' set-up applies SIGHT_RESET (the cache back to zero, as it zeroed the
  bytes), the saved game reads it through `SightCache` and the load applies SIGHT_LOAD. Both events are published
  through synchronous routes (`src/game/routes.cpp`), so `Apply` runs at the statement that wrote the bytes in 3.1c, not queued. It is dumped in record 44, as `sight[s].cell` and
  `sight[s].frameIdx` for each unit slot s, and is not hashed. **A wrong cache still diverges in hashed state.** The
  two divergences that showed the cache in #978 differ in what they reach. Conditions-dark-side's fault (tick 1243)
  never reaches the view state: the mask and the explored grids stay equal, so a wrong cache there is seen only
  in record 44. Eight-full-moon's first diverges at tick 6946, phase world, `players[0].explored[6786]` (05 against 04): a sight circle drawn from a wrong cache leaves a different count in the explored grid. The hash catches
  a cache that changes what a player sees; one that does not is los_radar's own bookkeeping, and `WriteState` is
  what shows it.
- **The bucket array moved to map** (3.2 of this interface and 4.1 of map's): los_radar no longer allocates or frees
  `Game::cells_1429f` and `overflowBucket`. `BuildDerivedLayers` and `FreeMapResources` call map's `AllocateBuckets`
  and `FreeBuckets` at the statements that did it, so the allocations, their sizes, tags and order are the same
  (`docs/map-interface.md`, section 5).
- **The trace dumps los_radar's events** in section 39 (`docs/trace-format.md`, section 8.27), after targeting's
  section 38, and hashes none of them. Each record is an event with the outcome `Apply` gave, in los_radar's order.
  SIGHT_RESET and SIGHT_LOAD are in section 39 like the other events: SIGHT_RESET with the field `unit`, and SIGHT_LOAD
  with `unit`, `cell` and `frameIdx`.

## 6. Random draws and sim events

Los_radar draws from no stream (`data/draws.csv`): no event and no pass draws.

It records two sim events, `SIM_EVENT_RADAR_UNITS` (in `RecordRadarUnits`) and `SIM_EVENT_RADAR_MAPPED` (in
`RecordRadarMapped`), both in the players phase. The class `ordered` of `data/dependencies.csv` names each pair of
subsystems whose sim events interleave in one phase. Each pair with los_radar is kept in 3.1c's order because the
engine runs both passes at 3.1c's place, and no event of los_radar records one:

| Pair | Phase | How the order is kept |
|---|---|---|
| ai → los_radar, los_radar → ai | players | Ai records its sim events in a player's head and body (its computer player's turn). The engine runs `RecordRadarUnits` between the viewing player's head and body, and `RecordRadarMapped` between its economy and its tail, as 3.1c's players code called them, so the records interleave as before. |
| players → los_radar, los_radar → players | players | The same: the head, body and tail are players' code, and the two passes run between them at 3.1c's statements. |
| units → los_radar, los_radar → units | players | Units records sim events in the players phase only from inside players' and economy's calls (scripts started and units created there), which stay in their place between the passes. |

Of los_radar's events only SHARE_MAPPING and SHARE_RADAR record a sim event (`SIM_EVENT_MESSAGE`, the toggle's
message). Both are published outside the tick and delivered at once, at 3.1c's statement, so no route moves a sim
event in the tick.

## 7. What stays inside the default

3.1c's types and limits (the eyeball table and its size, the sight circle tables and their levels of detail, the
explored layer's per-cell counts, the radar picture's size and snapshots, the 0x118 stride of a unit slot the pass
walks, the cell record's layout) are the default's, not the interface's. The event carries a unit as its handle, an
eyeball's position in world coordinates, and a rectangle in cells.

## 8. What the gate decided

Every event was queued first. The tier judged each build, and a route whose queueing diverged became synchronous,
with the reason here. A route that stayed equal but can be pending when a read in the same phase needs what it writes
became synchronous too, by #952's rule (section 4), and a route no scenario reaches takes 3.1c's order (#972). The
divergences are against the reference, with the first tick, phase and field.

**Units' REMOVE_UNIT is synchronous.** Queued, mission-victory diverged at tick 181, phase world, field
`players[1].explored[5962]` (reference 00, queued 01). The explored layer counts the units that see each cell. A
dying unit's sight was taken off after the death had cleared its state, so the cells it saw kept their count.

**Orders' UPDATE_UNIT is synchronous.** With REMOVE_UNIT synchronous, conditions-dark-side diverged at tick 1243,
phase units, field `units[1].losSightFrameIdx` (reference 0x5b, queued 0x61): the sight cache of a unit that orders
moved.

**Units' REVEAL_UNIT is synchronous.** With those two synchronous, eight-full-moon diverged at tick 5636, phase
units, field `units[1260].losSightFrameIdx` (reference 0x32, queued 0x2f): the sight cache of a unit created in the
phase.

**The rest.** With those three synchronous, all 38 tier scenarios were equal, with units' and players' UPDATE_UNIT,
ADD_EYEBALL, CELL_HEIGHTS and players' RECALCULATE queued. #952's rule then made units' UPDATE_UNIT and ADD_EYEBALL
synchronous, and the untested rule players' UPDATE_UNIT, players' RECALCULATE and the engine's SHARE_MAP (section 4).
CELL_HEIGHTS stays queued: it rewrites values no tick changes.

**CLOAK stays synchronous.** Economy's CLOAK route, queued again with los_radar's passes on the scheduler, diverged
at the same place as before: conditions-dark-side, tick 2460, phase world, `players[1].ai.visible.size` (reference 0,
queued 1). `UpdateSensorRadarAndCloak` now delivers los_radar's inbox first, but the cloak bit it reads is units'
state, and a queued CLOAK waits in units' inbox. `docs/economy-interface.md`, section 8, has the argument.

**How often each route runs, and inbox depths.** A measuring build (not merged) counted every publish to los_radar
with the final routes, over 88 of the 89 scenarios with native modules (load-midgame plays only after save-midgame,
and was left out):

| Route | Inside the tick | Outside the tick (delivered at once) |
|---|---|---|
| orders UPDATE_UNIT | 349201, synchronous, in 76 scenarios | none |
| units UPDATE_UNIT | 58, synchronous, in 5 | none |
| units REVEAL_UNIT | 1989, synchronous, in 52 | 1420, in 88 (the units of the map's start) |
| units REMOVE_UNIT | 933, synchronous, in 34 | 4, in 1 |
| units ADD_EYEBALL | 174, synchronous, in 25 | 2, in 1 |
| map CELL_HEIGHTS | 1845, queued, in 57 | 335, in 26 |
| engine RECALCULATE | none | 95, in 88 (the load of a match) |
| players UPDATE_UNIT, players RECALCULATE, engine SHARE_MAP, SHARE_MAPPING, SHARE_RADAR | none | none |

Only CELL_HEIGHTS is ever pending in the tick. Los_radar's inbox held at most 4 events (sensors-floating-teeth-los-off,
tick 10), all CELL_HEIGHTS of units placed in one update; the capacity is 1024. Every synchronous publish and every
pass delivers it first, so it never holds anything at a pass.

No scenario reaches seven of the routes. Of them, players' UPDATE_UNIT, players' RECALCULATE and the engine's SHARE_MAP
can be published inside the tick, and are synchronous for it; the other four are published only outside the tick.

The final build passes the full suite in both modes (`reference.py --check --both-modes`, the gate when the trace
schema changes): 178 of 178 runs (89 scenarios, each in WASM and with native modules) equal, and every sound log, ui
log, event digest, frame and save equal.
