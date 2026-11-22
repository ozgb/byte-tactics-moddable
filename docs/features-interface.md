# The features interface

Features sits behind `interfaces/features.h` (version 4.0), the second module on the scheduler after economy
(`docs/economy-interface.md`), and the first with random draws to keep in order (#954). 3.1c's feature code is its
default implementation. This page is the inventory the interface was cut from:
- what other subsystems did to features' state in 3.1c, and what each became;
- what features did to others, and what each became;
- what features reads, and what others read of it;
- how the order of random draws and sim events is kept;
- what the trace hashes, and what it only dumps.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, and events as plain
data. This issue changes none of its rules. Event kinds are unique across interfaces, since one outbox can carry
events of several interfaces: economy uses 0 to 4 and 16 to 18, features 32 to 35 and 48 to 51.

Features' place in it:
- **Events into features:** PLACE, REMOVE, DAMAGE and RECLAIM, published by units, orders, weapons and the map
  loader. Features' handler (`FeaturesInbox`, `src/seams/features_world.cpp`) passes each to `Apply`. Each was
  queued first, and the gate made four routes synchronous (section 8): DAMAGE from weapons, RECLAIM from orders,
  PLACE from units (a corpse), and features' own AREA_DAMAGE to weapons. REMOVE from orders is synchronous by #952's
  rule (section 8). PLACE from the map loader is queued, but it is published only outside the tick, where every route
  delivers at once.
- **Synchronous slot:** `Place`, called through the service `ScheduleFeaturesPlace`, for the one caller that needs
  the answer in the same statement (section 2).
- **Effects out of features:** FREE_STATE, INIT_STATE, PASS_MAPS and AREA_DAMAGE, published to features' outbox.
  Units subscribes to FREE_STATE and INIT_STATE, map to PASS_MAPS (map has an inbox of its own, `SCHED_MAP`),
  weapons to AREA_DAMAGE; their handlers are adapters onto their own seam slots. FREE_STATE, INIT_STATE and
  PASS_MAPS are synchronous by #952's rule (section 4). The INCOME of a reclaim goes to economy, as it did since #937.
- **Runs:** the features phase. The scheduler (`SchedFeaturesPhase`) holds its loops as `docs/schedule.md` gives
  them: `AnimateFeatureType` for each feature type, `ScanFeatureCell` once, then `UpdateFeatureSpot` for each spot of
  the used list, from `FirstFeatureSpot` along the links each update returns. Features' inbox is delivered before the
  type loop, before the scan and before each spot.

## 2. Events into features

| 3.1c site | 3.1c call | Event |
|---|---|---|
| a unit's corpse above the sea (`CreateUnitCorpse`) | `PlaceFeature` | PLACE, with the unit's position, angles and player |
| a unit's corpse under the sea (`CreateUnitCorpse`) | `PlaceFeature`, then the corpse's velocity set through the spot it returned | synchronous `Place`, with the flag SINK |
| reclaiming (`ReclaimOrder`, `VtolReclaimOrder`) | `ReclaimFeature` | RECLAIM, with the unit's handle and the order's position |
| resurrecting a wreck (the order that creates a unit from a feature) | `RemoveFeature` | REMOVE |
| an explosion's area damage (`weapons.cpp`) | `DamageFeature` | DAMAGE, with the weapon's number |
| the map loader (`LoadTntMap`), outside the tick | `PlaceFeature` | PLACE, delivered at once |

**Answers.** No caller of `RemoveFeature`, `ReclaimFeature` or `DamageFeature` used the value returned, so none needs a
slot of its own, and features needs none of the "ask synchronously before the queued event" pattern that #952 judges
for economy's `RequestGranted`. Their routes are synchronous for what the rest of the phase reads (sections 4 and 8),
not for an answer. `PlaceFeature`'s value was used once: under the sea, whether the corpse took a spot of the
pool decides whether the unit's death leaves smoke, and the spot was then written by units (its sinking velocity).
That call is the synchronous `Place`, and the velocity is the event's flag SINK, applied by features.

**Refused intents.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 36) and
returned to no one: PLACE 1 when the feature took a spot, REMOVE 1 when a feature was removed, DAMAGE 1 when it killed
the feature, RECLAIM 1 when the unit reclaimed it.

Across the 88 scenarios of `reference-traces-v28`, on the routes of section 8 (an instrumented build of this change,
equal to the reference in every scenario):

| Kind | Outcome 1 | Outcome 0 |
|---|---|---|
| PLACE | 411 | 64,147 |
| REMOVE | 1 | 0 |
| DAMAGE | 885 | 690 |
| RECLAIM | 345 | 1 |

Most PLACEs are the map loader's and the scan's seeds: a feature without a 3D model (`noobject`, such as a tree) takes
no spot of the pool, so its outcome is 0 though it stands on its cells. Features published 63 FREE_STATE,
424 INIT_STATE, 67,117 PASS_MAPS and 271 AREA_DAMAGE.

## 3. Effects out of features

| 3.1c call | Effect | Subscriber's handler |
|---|---|---|
| `scripts.FreeObjectState` when a feature with a model goes | FREE_STATE | `scripts.FreeObjectState` |
| `scripts.CreateObjectState` when a feature with a model is placed | `scripts.ObjectStateSize` (a query), features allocates the state, then INIT_STATE | the model's state is built as `CreateObjectState` built it |
| `map.RefreshAllPassMaps` after a feature appears or goes | PASS_MAPS | `map.RefreshAllPassMaps` |
| `weapons.ApplyAreaDamageAt` from a burning feature (`SpreadFire`) | AREA_DAMAGE | `weapons.ApplyAreaDamageAt` |

**`CreateObjectState` split.** It was mixed: it allocated and built a model's piece state and returned it, and the
spot kept it. The size is now a query (`scripts.ObjectStateSize`, scripts 1.0), features allocates the state where 3.1c
did and keeps it, and the building is an intent. The trace hashes the state pointer as a heap pointer (`docs/trace-format.md`,
section 5), so where the allocator puts it is not seen, and nothing in the simulation reads the pieces of a feature's
model.

**`ReclaimFeature`'s calls outside an interface.** Before this change `docs/schedule.md` listed under
`features.ReclaimFeature` two calls into units and one into players outside their interfaces. They were not
`ReclaimFeature`'s own: they were the handlers of economy's effects (`UnitsOnEconomyCloak`,
`UnitsOnEconomyExtractor`, `PlayersOnEconomyShare`), which the route table calls when the scheduler delivers
economy's inbox, reached through `economy.Apply` from the reclaim's INCOME. They are routes already. Now
`docs/schedule.md` lists them under `features.Apply`, with features' own effect handlers
(`UnitsOnFeaturesFreeState`, `UnitsOnFeaturesInitState`, `MapOnFeaturesPassMaps`, `WeaponsOnFeaturesAreaDamage`), the
adapters onto units', map's and weapons' seam slots that `src/game/routes.cpp` registers. Features' code calls no other
subsystem outside an interface. A reclaim in a multiplayer game still sends its packet through
`network.SendFeatureReclaimed`, a seam slot, as 3.1c did; network is outside the model (section 8).

**Payloads.** Every event and effect starts with `FeaturesEventHeader`, which holds `kind` alone. Each kind has the payload of `interfaces/features.h` (version 4.0) that starts with it; a publisher passes the payload's size, `Apply(event, size)` refuses an event smaller than its kind's payload with outcome 0 and no change, and `Place(const FeaturesPlace*)` takes the PLACE payload.

| Kind | Payload | Fields after the header |
|---|---|---|
| PLACE 32 | `FeaturesPlace` | `x`, `z` (the cell), `feature` (the feature type), `flags` (`FEATURES_HAS_POS` 1, `FEATURES_HAS_ROT` 2, `FEATURES_SINK` 4), `owner` (10 for none), `pos[3]` (world coordinates, 16 fractional bits, with HAS_POS), `rot[3]` (16-bit angles, with HAS_ROT) |
| REMOVE 33 | `FeaturesRemove` | `x`, `z`, `force` (1 to remove even a feature that refuses removal) |
| DAMAGE 34 | `FeaturesDamage` | `x`, `z` (the cell the feature's footprint starts at), `weapon` |
| RECLAIM 35 | `FeaturesReclaim` | `unit` (the reclaiming unit, a block offset), `pos[3]` (the feature's position) |
| FREE_STATE 48 | `FeaturesFreeState` | `state`: the piece state to free, a block offset |
| INIT_STATE 49 | `FeaturesInitState` | `state` (the piece state, a block offset), `feature` (the type whose model it is built for) |
| PASS_MAPS 50 | `FeaturesPassMaps` | `x`, `z` (the footprint's first cell), `width`, `height` |
| AREA_DAMAGE 51 | `FeaturesAreaDamage` | `weapon`, `pos[3]` (the centre of the damage) |

## 4. Views

**What features reads:** the cells (through `los_radar.GetMapCell`), the ground and the mean height of a cell (`map.GetGroundHeight`,
`map.GetCellMeanHeight`), the game type (`mission.GetGameType`), the wind (wind_meteors' state), the gravity and the sea
level, and the weapon definitions it names, all as of each module's last run.

**What others read of features:**
- **Views, in the state block, hashed:** the spot pool (section 7 of the trace) and the feature fields of the map
  cells (the feature number, the spot or the footprint offsets, the spot bit and the owner; section 11). Weapons reads
  a spot's position for an explosion's distance, orders a spot's rotation when it resurrects a wreck, and map, units,
  weapons and los_radar read the cells. They stay view state that features writes, as `Player`'s totals did for
  economy: the cells are map's allocation, and the feature fields of a cell are features'.
- **Queries:** `GetCellFeature`, `GetFootprintCentre`, `FeatureTypeCount`, `FirstFeatureSpot` and `FindFeatureAtPos`
  (weapons' slot until #984: it reads only features' and los_radar's cells, so it moved here in 3.1; orders and
  players call it).

**As of features' last run.** Every read above sees features' state as of its last run or its last synchronous
apply. By #952's rule a query needs a delivery first only when an event that can be pending writes what it reads, and
that is a property of the code, not of what the scenarios reach. Inside the tick no event into features and no effect
out of it can be pending: every route of features in `src/game/routes.cpp` is synchronous but the map loader's PLACE,
which `LoadTntMap` publishes only outside the tick, where the scheduler delivers every route at once (#970). So no
query needs a delivery first. Each route that was queued at first fails the test, and is synchronous for it:
- **REMOVE** from orders: a second resurrection or a reclaim in the same phase reads the wreck's cells (section 8).
- **FREE_STATE:** `scripts.FreeObjectState` returns a model's point lists and state to the sim block's free list, and
  every allocation later in the phase reads that list. A feature that dies is replaced in the same call (its heap or
  burnt replacement, `PlaceFeatureAt`), whose `SimAllocTagged` would run with the free pending, and so would every
  `new (kSim)` of the rest of the phase (an order, a projectile). The trace does not hash where a block goes, but the
  free list is state in the block, and a queued free leaves it in another order than 3.1c's.
- **INIT_STATE:** building a model's state allocates the point list of each piece (`AddStateEntries`, "Point List")
  from the same free list, so a queued INIT_STATE moves those allocations after every other of the phase. The pieces
  themselves are read only between ticks (the feature's drawing, `info_panel.cpp`, and the save), and nothing in the
  tick reads the state before it is built; the allocations alone fail the test.
- **PASS_MAPS:** `RefreshAllPassMaps` rewrites the pass map words around the footprint from the cells and the units
  stamped on them as they are when it runs (`PassMap::Refresh` through
  `GetPassMapFootprintValueWithEdgeStrips`), and reports the change to pathfinding (`CellsChanged`). In the units
  phase units' own refreshes (`RefreshPassMapsForUnit`, `PassMap::RefreshMovedUnits`,
  `PassMap::RefreshUnitIfStale`) write the same words and report their own changes, and units move between
  them. A queued refresh would read the stamps after every unit's move and write last, so the words, and the order of
  the changes pathfinding is told of, would not be 3.1c's. Pathfinding reads the pass maps only in the players phase
  (`WorldPass`, `WorldTerrainPass` in `pathfinding_world.cpp`), but the writes race each other.

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv`, regenerated on this change, has these rows of
class `immediate` with features as writer or reader. The cells are named by their allocation (`LoadTntMap: ... PLOT
MEMORY`), and the spots by the raw fields of `FeatureAnimEntry` that overlay them (`bytes_8` holds a spot's x,
`bytes_10` its z and what follows). Each is served as follows:

| Writer | Reader | Phase | The write | How the read is served now |
|---|---|---|---|---|
| features | map, units, weapons | units, projectiles | a corpse placed, a wreck reclaimed, a feature killed or set alight | the synchronous routes PLACE (from units), RECLAIM and DAMAGE apply it at 3.1c's statement |
| features | map, units, weapons | features | features' own run: a fire spreads or burns out, a feature seeds another | the readers are called from that run, at once: weapons through the synchronous AREA_DAMAGE, map through `GetCellMeanHeight` on a falling spot's position, and the pass maps through the synchronous PASS_MAPS |
| features | weapons | units, projectiles | a spot placed or set alight, whose position an explosion measures from | DAMAGE and PLACE from units are synchronous |
| los_radar, map | features | units | the other fields of a cell (sight, the unit on it) | features reads them inside a synchronous apply, after los_radar's and map's direct writes, as in 3.1c |

Two rows are `deferrable`: orders reads a wreck's rotation when it resurrects it, and weapons reads a spot's position
in the projectiles phase. Neither needs the write at once. The rows where features read a unit's or an order's
position in the units phase (`Unit::pos`, `Unit::field_64`, `Order::pos`) are gone: the events carry the position
by value.

## 5. State and the trace

Features keeps no private region, and so it has no `WriteState` report: its state is view state, hashed where it was.

- **What features restores at load.** `InitFeatureAnimPool` sets up the spot pool and zeroes the scan index, in the map load, before the first tick.
- **The spot pool** (`FeatureAnimEntry`, 0x800 spots on a used and a free list, with the scan's cell index) is
  features' alone to write, and `privatestate.py --check` fails when another subsystem writes a spot's list links
  (`next`, `prev`), the scan index or a cell's feature fields. The rest of a spot is not checked: it holds the spot's
  animation cursors, which the GAF helpers step and read for features and for drawing, and the access log names
  those helpers' subsystems (game and presentation) as their writers. It stays in the
  hash (section 7 of the trace), because weapons and orders read it inside the tick: an explosion's distance to a
  burning feature is measured from its spot, and a resurrected unit takes its wreck's rotation from it. Moving it out
  of the hash would be a trace format bump with a reference half (#946's path), and would give those two reads queries
  of their own, for no gain in what a re-implementation must match: the spot fields the trace hashes are the ones
  that decide the game.
- **The feature fields of a map cell** (the feature number or a footprint code, and the footprint offsets or the
  spot) are features' to write; the cell grid is map's (the plot memory `LoadTntMap`
  allocates), and its other fields are map's and los_radar's. Map, units, orders, weapons and ai read the feature
  fields, as of features' last run (section 4). They stay hashed view state (section 11), as `Player`'s totals
  did for economy. Before the tick, `LoadTntMap` (`ClearBorderFeatures`) clears the features of the map's border cells
  as it builds the grid; that is set-up, outside the model.
- **The trace dumps features' events** in section 36 (`docs/trace-format.md`, section 8.25), after economy's section
  33 and wind_meteors' section 37, and hashes none of them: no trace format bump and no reference half. Each record is an event with the outcome
  `Apply` gave, or an effect features published, in features' order. A model's piece state is dumped as `null` or
  `heap`, never its address.

## 6. Random draws and sim events

Features draws from both game-logic streams (`data/draws.csv`):
- `rng.crtHoldrand`, two draws for each animated feature type in each features phase, as its animation steps
  (`AnimateFeatureType`), and two for the smoke of a burning feature (`UpdateFeatureSpot`);
- `rng.parkMiller`, in the scan when a feature seeds another (`ScanFeatureCell`), when a fire starts
  (`StartFeatureBurning`, its burn time) and when it spreads (`SpreadFire`).

The draws of the features phase are features' own, made in its run, in the order the scheduler's loops give, which is
3.1c's (section 1). A fire starts in three phases: in units and projectiles, when an explosion's area damage hits a
feature that burns (DAMAGE), and in features, when a fire spreads. The class `ordered` of `data/dependencies.csv`
names each pair of subsystems whose draws or sim events interleave in one phase. Each pair with features is kept in
3.1c's order as follows:

| Ordered edge | Stream | Phase | Example | What interleaves | Kept by |
|---|---|---|---|---|---|
| weapons and features, both ways | `rng.parkMiller` | units, projectiles | mission-kill-of-type@185, long-great-divide-2p@45551 | one explosion draws for the units it hits and, through `DamageFeature`, for each feature it sets alight, in the order of the cells | DAMAGE is synchronous |
| weapons and features, both ways | sim events | units, projectiles | ui-keys@99, long-greenhaven-8p@22536 | the same explosion's events and a fire's | DAMAGE is synchronous |
| ai and features, both ways | `rng.parkMiller` | projectiles | orders-guard-attacked@159, long-greenhaven-8p@27850 | a unit the same explosion hits reacts (`ReactToAttack`) between two fires' draws | DAMAGE is synchronous |
| ai and features, both ways | sim events | units, projectiles | mission-kill-of-type@185, long-greenhaven-8p@24144 | the same, with the events | DAMAGE is synchronous |
| ai and features, both ways | sim events | features | long-greenhaven-8p@22962, orders-guard-attacked@270 | a burning feature's area damage hits a unit, which reacts and records events between the spread's | AREA_DAMAGE is synchronous |

Queued, a DAMAGE would move a fire's draws to the phase boundary, after every other draw of the phase, and a queued
AREA_DAMAGE would move the reactions of the units a fire hits after the rest of the features phase; section 8 gives
where each diverged. No other event or effect of features draws: `data/draws.csv` lists no draw under REMOVE, PLACE, RECLAIM,
FREE_STATE, INIT_STATE or PASS_MAPS.

## 7. What stays inside the default

3.1c's types and limits (a pool of 0x800 spots, the feature codes 0xfffb to 0xffff of a cell, the burn and seed
rules, the death and burnt replacements) are the default's, not the interface's.

## 8. What the gate decided

Every event and effect was queued first. The tier judged the first build and the full `--check` the later ones, and a
route whose queueing diverged became synchronous, with the reason here. A route that stayed equal but can be pending
when a read in the same phase needs what it writes became synchronous too, by #952's rule. The scenarios that diverged are listed with
the first tick, phase and field, against the reference.

**DAMAGE is synchronous.** Queued, the tier diverged in mission-kill-of-type, first at tick 185, phase units
(`UpdateAllUnits`), fields `features[61..65].timer` and `units[316].list[0].wakeFrame`. An explosion that sets a
feature alight draws its burn time (`StartFeatureBurning`) between the draws for the units it hits (section 6).
Queued, the fires drew at the phase boundary, after every unit's update, so each draw after the first fire took
another number.

**RECLAIM is synchronous.** Queued, long-great-divide-2p diverged at tick 35726, phase units, first field
`rng.parkMiller`, then unit 271's heading, position and flags. A reclaiming unit takes the feature off its cells, and a
unit updated later in the same phase looks for something to reclaim near it (`PickRandomReclaimableResourcesInRadius`,
which reads the cells through `GetCellFeature`) and draws among what it finds. Queued, the feature was still on its
cells, so that unit found one more candidate and drew another number.

**PLACE from units is synchronous.** With RECLAIM synchronous and the corpse PLACE queued, long-great-divide-2p
diverged at tick 45416, phase units, first field `rng.parkMiller`, then unit 305's position, velocity and order
(`units[305].list[0].pos.x`). The same scan read a cell through `GetCellFeature` while the corpse of a
unit that died earlier in the phase was still in features' inbox (the instrumented build counted that one read, at
tick 45416), so the wreck was missing from what it chose among. A corpse is now placed at 3.1c's statement. The
underwater corpse was already the synchronous `Place` (section 2).

**AREA_DAMAGE is synchronous.** Queued, all 88 traces were equal, but the sim events were not: long-great-divide-2p at
tick 29988 and long-greenhaven-8p at tick 22962, both in the features phase. In long-great-divide-2p, the reference
records a unit's speech (`unit_speech`, unit 33, which a burning feature's area damage hit) where the queued build
recorded the next fire's `white_smoke` first. Queued, the units a fire hit reacted after the rest of the spot loop.

**REMOVE is synchronous by #952's rule, not by a divergence.** Queued, every scenario was equal in both modes, and
the instrumented build saw one REMOVE (orders-tier2-core, tick 615) with no read of what it writes while it was
pending. But it can be pending when a read in the same units phase needs it:
- Two units resurrect one wreck in one phase. The second `ResurrectOrder` (state 5, `unit_orders.cpp`) gives up when
  `GetCellFeature` of the wreck's cell is 0xfffb or more, that is when the wreck has gone. With the first's REMOVE
  pending the wreck is still on its cells, so the second creates a second unit and sends a second
  `SendFeatureReclaimed`.
- A unit reclaims the wreck after a resurrection in the same phase. The synchronous RECLAIM finds the wreck still on
  its cells and credits its metal, and the queued REMOVE then removes what the reclaim left there.

In 3.1c `RemoveFeature` ran at the statement, so neither can happen; synchronous, it runs there again.

**FREE_STATE, INIT_STATE and PASS_MAPS are synchronous by the same rule** (section 4). Queued, they were equal in
every scenario in both modes.

**The map loader's PLACE is queued.** `LoadTntMap` publishes it only outside the tick, where every route delivers at
once (#970's rule), so it is never pending.

**`Place` is synchronous by analysis, not by a divergence.** Its caller needs the answer in the statement (section
2).

**Inbox depths.** With the earlier routes queued, over the 88 scenarios features' inbox held at most one event (the
one REMOVE, in orders-tier2-core at tick 615), map's, which took PASS_MAPS, at most 158 (long-great-divide-2p, tick
40905), and units', which took FREE_STATE and INIT_STATE with economy's effects, at most 19 (skirmish4-long, tick
26096); the capacity is 1024. Now no event or effect of features is queued inside the tick: features' and map's
inboxes stay empty, and units' holds only economy's effects.
