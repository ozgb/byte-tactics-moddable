# The economy interface

Economy sits behind `interfaces/economy.h` (version 4.0), the pilot of the scheduler decided on #180 (2026-11-08).
3.1c's resource code is its default implementation (`src/game/economy.cpp`). This page is the inventory the interface
was cut from (#937):
- what other subsystems did to economy's state in 3.1c, and what each became;
- what economy did to others, and what each became;
- what economy reads, and what others read of it;
- what the trace hashes, and what it only dumps.

## 1. The model

A module reads through views and queries, and changes the world only by events. One scheduler, in the engine
(`src/game/scheduler.cpp`), applies the events depth-first, in 3.1c's order, each before the next call:
- **Events into economy** are what another subsystem used to write into economy's state. The writer hands the event
  to the service `ScheduleEconomyEvent`, at the statement where 3.1c wrote, and the scheduler applies it with
  `Apply` before the writer goes on.
- **Effects out of economy** are what economy used to do to another subsystem. Economy hands them to the world view's
  `Emit`, and the scheduler applies them before `Emit` returns.
- **Entry points** (`UpdatePlayerEconomy`, `UpdateMetalExtraction`, `UpdateResourceSharing`) are called by the engine
  only: by the scheduler's phase loops, the tail of the tick, and the service `ScheduleMetalExtraction`.

Because each event is applied at once, the order of every read and write is 3.1c's, and a single-threaded scheduler
needs no snapshot. The later parallel scheduler would need the splits listed in section 6.

## 2. Events into economy

| 3.1c site | 3.1c write | Event |
|---|---|---|
| unit creation (`InitUnitFromType`) | `Reset` of the unit's account | RESET (unit) |
| player set-up (`InitPlayerSlot`) | `Reset` of the player's account | RESET (unit 0) |
| loading a unit (`LoadUnitAccounts`) | the account read from the saved game | LOAD, with the account |
| building (`AddBuildProgress`, `BuildWeaponOrder`) | `RequestEnergyAndMetal`, which charged and answered | query `RequestGranted`, then REQUEST |
| repairing (`AddRepairProgress`) | `RequestEnergy` | query `RequestGranted`, then REQUEST |
| firing a weapon that costs resources (`UpdateUnitWeapons`) | `SpendEnergyAndMetal` | SPEND |
| a player's transfer (`TransferMetal`, `TransferEnergy`) | `SpendEnergy` or `SpendMetal` of the giver, then `+=` into the receiver's account | SPEND (if announced), then INCOME |
| a refund while building down (`AddBuildProgress`) or after a cancelled build (`BuildingBuildOrder`) | `+=` into the unit's metal income | INCOME |
| a kill of a unit under construction (`ApplyUnitDeath`, kill type 5) | `+=` into the attacker's metal income | INCOME |
| reclaiming a feature (`ReclaimFeature`) | `+=` into energy, then metal income | INCOME, INCOME |
| a commander respawn (the players loop) | `+=` into energy, then metal income | INCOME, INCOME |

**Mixed calls split.** A request both charged the account and told the caller whether it was granted. It is now a
query (`RequestGranted`) and an intent (REQUEST): the query's answer is the one 3.1c returned, since the request does
not change what decides it (the stall of the account). Spends were mixed only for the transfers, which ignore the
answer, and for weapons, which check the store first.

**The handicap is economy's.** Every 3.1c site that added income to a computer player's account scaled it by the
difficulty (0.5 at the easiest, 0.7 at the next). The sites now pass the raw amount, and economy scales it. The
results are bit for bit 3.1c's: each site's arithmetic is `(float)((double)account + (double)amount * factor)`, and
a double holds every float sum exactly enough for the float rounding to agree.

**Refused intents.** An event is an intent, checked when it is applied. `Apply` returns its outcome:
- a SPEND: 1 when applied, 0 when refused (its store is short), which leaves the state as it was;
- a REQUEST: 1 when granted, 0 when not (the account is stalled). It is charged either way, as in 3.1c;
- every other kind: 1.

The scheduler records the outcome with the event (trace section 33), so a refusal is seen by the trace and by any
caller that asks for it. Under this scheduler no event 3.1c makes is refused except a SPEND that 3.1c also refused.

Across the 85 reference scenarios, the scheduler records these events and effects:

| Kind | Outcome 1 | Outcome 0 |
|---|---:|---:|
| RESET | 3,642 | 0 |
| LOAD | 4 | 0 |
| REQUEST | 1,731,216 | 1,072,998 |
| SPEND | 6,212 | 0 |
| INCOME | 1,788 | 0 |
| CLOAK | 855,102 | 0 |
| EXTRACTOR | 640 | 0 |
| SHARE | 0 | 0 |

No scenario shares resources, so SHARE is not exercised.

## 3. Effects out of economy

| 3.1c call | Effect | Applied as |
|---|---|---|
| `SetStateBits(unit, 4, paid)` in `UpdatePlayerEconomy`, for a cloaked unit's upkeep | CLOAK | `units.SetStateBits` |
| the `SetSpeed` script call in `UpdateMetalExtraction` | EXTRACTOR | `units.StartScriptWithArgs` when the unit has a script |
| `TransferEnergy`/`TransferMetal` in `UpdateResourceSharing` | SHARE | `players.TransferEnergy` or `TransferMetal`, whose own SPEND and INCOME follow it |

`los_radar.GetMapCell`, economy's one query of another subsystem, became the world view's `CellMetal`. The share of
maps in `UpdateResourceSharing` (network play) stays as it is.

## 4. Views

**What economy reads** comes through `EconomyWorldView` (`src/seams/economy_world.cpp`): the tick, the difficulty,
wind and tide, each player's info and alliances, the units of a player and each unit's info (type, state, cell,
footprint, script), each unit type's resource figures, and a cell's metal. The default implementation still writes
its own view state where 3.1c keeps it (below).

**What others read of economy:**
- **Views, in the state block, hashed:** the resource fields of the `Player` record (stores, incomes, usages,
  capacities, totals, waste, storage bonuses, share levels), the player accounts `players[i].econ`, and
  `Unit::extraction`. Others read them as they did; only economy writes them during play.
- **Queries:** `GetEnergyUse` (the computer player's ratings), `RequestGranted` (builders), `UnitAccount` (the unit's
  info panel and the saved game).

## 5. The private region and the trace

**The region.** A unit's account (12 floats: for energy and for metal, what it made, asked for, was granted, its
stall, and the last tick's make and use) was 48 bytes of the unit record. It is now economy's: `RegionSize` gives
`unitSlots * 48`, the engine allocates it (`SimAllocRegion`, tag `Economy region`) in `LoadBattleAssets` before the
players are set up, `Init` receives it and `Shutdown` hands it back, as pathfinding's. The 48 bytes of the unit record
stay, unused, so no other offset moves. Only economy reads or writes the region (`tools/privatestate.py`,
`tools/seamlink.py`).

**What the trace hashes** (`bt-trace 5`, `docs/trace-format.md`):
- the view state above, as before;
- not the unit record's account bytes, in both branches: a re-implementation may keep its accounts in any form;
- the economy events (section 33): every event and effect the scheduler applies, in order, with its outcome. An
  implementation conforms when it applies the same events with the same outcomes, emits the same effects, and leaves
  the same view state;
- economy's `WriteState` (the accounts of the units in play) is dumped (section 34) and not hashed.

## 6. Where entry points split

Every one of economy's writes and reads inside the tick falls inside one entity's update: a unit's in the units phase,
or a player's in the players phase (`docs/schedule.md`). The scheduler holds those loops: the units phase calls
`units.UpdateUnit` for each unit, and the players phase calls `players.UpdatePlayerHead`, then economy's
`UpdatePlayerEconomy` when it is due, then `players.UpdatePlayerTail`, for each player. The events emitted inside a
unit's or a player's update (requests and spends by builders and weapons, refunds, transfers, the commander respawn)
are applied where they are made. A parallel scheduler would have to split the units' update at each of those points,
or apply them before the update continues, as `docs/schedule.md` marks them.

## 7. What stays inside the default

The difficulty handicap, 3.1c's settlement of the accounts (stalls, carried shortfalls and their rounding), the cloak
costs, the wind and tide figures, metal makers and the sharing rules of network play are the default's, not the
interface's: another implementation receives the same events and may decide differently, at the cost of conformity.
