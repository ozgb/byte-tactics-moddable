# The economy interface

Economy sits behind `interfaces/economy.h` (version 4.0), the pilot of the scheduler decided on #180 (2026-11-08).
3.1c's resource code is its default implementation (`src/game/economy.cpp`). This page is the inventory the interface
was cut from (#937):
- what other subsystems did to economy's state in 3.1c, and what each became;
- what economy did to others, and what each became;
- what economy reads, and what others read of it;
- what the trace hashes, and what it only dumps.

## 1. The model

The model is the human's of 2026-11-12 (#937). The scheduler is the engine's (`src/game/scheduler.cpp`), and it knows no
event kind:
- **Reads.** A module reads any module's state as of that module's last run, through views and queries.
- **Outboxes and the router.** A module changes another only by publishing an event, plain data whose first word is
  its kind, to its own outbox (the service `SchedPublish`). The router is a table of (outbox, kind, subscriber,
  handler) rows in a fixed order (`src/game/routes.cpp`), 3.1c's by default: it copies each event into the inbox of
  every subscriber whose row matches, in table order, when the event is published. Each subscriber owns its handler.
- **Inboxes.** A module consumes its whole inbox before every run: economy before `UpdatePlayerEconomy`,
  `UpdateMetalExtraction` and `UpdateResourceSharing`, units before each unit's update, players before each part of
  a player's update. The scheduler also drains every inbox at each phase boundary (`docs/schedule.md`), before the
  phase's trace row, and after `UpdateResourceSharing` in the tail, before the network packets are sent; at the tick
  boundary it drains them again and stops the game if one still holds an event. Outside a tick (set-up, loading) a
  published event is delivered at once.
- **Synchronous slots.** A same-statement answer comes only from a slot the interface marks synchronous. The engine
  delivers the module's pending inbox before it calls one, so the slot sees every event published before it.
- **Plain data.** Events carry no pointers: a LOAD carries the account's twelve words.

Economy's place in it:
- **Queued events into economy:** RESET, LOAD, REQUEST and INCOME, published by units, orders, players and features.
  Economy's handler (`src/seams/economy_world.cpp`) passes each to `Apply`.
- **Synchronous slots:** `Spend` (the SPEND of a weapon or a transfer) and `RequestGranted`, called through the
  services `ScheduleEconomySpend` and `ScheduleEconomyRequestGranted`, and the run `UpdateMetalExtraction`, called
  through `ScheduleMetalExtraction` when a unit is created or loaded (section 4).
- **Effects out of economy:** CLOAK, EXTRACTOR and SHARE, published to economy's outbox through the world view's
  `Emit`. Units subscribes to CLOAK and EXTRACTOR, players to SHARE; their handlers are adapters onto their own seam
  slots (`src/units/economy_effects.cpp`, `PlayersOnEconomyShare` in `src/game/players.cpp`). EXTRACTOR and SHARE
  are queued; CLOAK's route is synchronous, because the gate diverged with it queued (section 8).
- **Runs:** `UpdatePlayerEconomy` (the players phase), `UpdateMetalExtraction` (above) and `UpdateResourceSharing`
  (the tail of a network game), called by the engine only.

The parallel scheduler and performance are out of scope (#937).

## 2. Events into economy

| 3.1c site | 3.1c write | Event |
|---|---|---|
| unit creation (`InitUnitFromType`) | `Reset` of the unit's account | RESET (unit) |
| player set-up (`InitPlayerSlot`) | `Reset` of the player's account | RESET (unit 0) |
| loading a unit (`LoadUnitAccounts`) | the account read from the saved game | LOAD, with the account |
| building (`AddBuildProgress`, `BuildWeaponOrder`) | `RequestEnergyAndMetal`, which charged and answered | synchronous `RequestGranted`, then REQUEST |
| repairing (`AddRepairProgress`) | `RequestEnergy` | synchronous `RequestGranted`, then REQUEST |
| firing a weapon that costs resources (`UpdateUnitWeapons`) | `SpendEnergyAndMetal` | synchronous `Spend` |
| a player's transfer (`TransferMetal`, `TransferEnergy`) | `SpendEnergy` or `SpendMetal` of the giver, then `+=` into the receiver's account | synchronous `Spend` (if announced), then INCOME |
| a refund while building down (`AddBuildProgress`) or after a cancelled build (`BuildingBuildOrder`) | `+=` into the unit's metal income | INCOME |
| a kill of a unit under construction (`ApplyUnitDeath`, kill type 5) | `+=` into the attacker's metal income | INCOME |
| reclaiming a feature (`ReclaimFeature`) | `+=` into energy, then metal income | INCOME, INCOME |
| a commander respawn (the players loop) | `+=` into energy, then metal income | INCOME, INCOME |

**Mixed calls split.** A request both charged the account and told the caller whether it was granted. It is now a
query (`RequestGranted`) and an intent (REQUEST): the query's answer is the one 3.1c returned, since the request does
not change what decides it (the stall of the account). Spends were mixed only for the transfers and weapons, and
neither uses the answer: `Spend` is synchronous for the store it changes, which the same unit's or player's update
reads next (section 4).

**The handicap is economy's.** Every 3.1c site that added income to a computer player's account scaled it by the
difficulty (0.5 at the easiest, 0.7 at the next). The sites now pass the raw amount, and economy scales it. The
results are bit for bit 3.1c's: each site's arithmetic is `(float)((double)account + (double)amount * factor)`, and
a double holds every float sum exactly enough for the float rounding to agree.

**Refused intents.** An event is an intent, validated when it is applied:
- a queued event is applied by `Apply`, which returns its outcome: 1 for RESET, LOAD and INCOME; for a REQUEST, 1
  when granted and 0 when not (the account is stalled), charged either way, as in 3.1c. No caller waits for a queued
  event, so the outcome is returned to no one: economy's handler records it with the event in the trace's dump
  (section 33), and a refusal is seen there and in the state it leaves;
- a SPEND goes through the synchronous `Spend`, which returns 1 when applied and 0 when refused (its store is short),
  leaving the state as it was. The caller gets the outcome, and the service records it in the same dump.

Under the default no event 3.1c makes is refused except a SPEND that 3.1c also refused, and a REQUEST of a stalled
account, which 3.1c also charged without granting.

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

| 3.1c call | Effect | Subscriber's handler |
|---|---|---|
| `SetStateBits(unit, 4, paid)` in `UpdatePlayerEconomy`, for a cloaked unit's upkeep | CLOAK (synchronous) | `units.SetStateBits` |
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
  `Unit::extraction`. Others read them as they did. Economy is not yet their only writer (#948):
  - in the players phase, the commander respawn sets the storage bonuses (`SetStartingStorageBonus`);
  - in network play, `HandlePlayerEconomy` copies another player's figures from a packet;
  - the `ATM` console command (`CmdATM`) adds to the local player's stores, and the debug hotkey in the tick
    (`RunGameSteps`) fills every player's stores to capacity;
  - at set-up, `InitPlayerSlot` zeroes the fields, and `InitPlayerResources` and `SpawnCommanderAtStartPos` set the
    storage bonuses.
- **Queries:** `GetEnergyUse` (the computer player's ratings), `RequestGranted` (builders), `UnitAccount` (the unit's
  info panel and the saved game).

**As of economy's last run.** Every read above sees economy's state as of its last run or synchronous slot. Economy's
pending inbox can only hold RESET, LOAD, REQUEST and INCOME, and those write the accounts alone (the region and
`players[i].econ`); the stores and the other `Player` fields change only in `Spend` and in the runs, which deliver the
inbox first. So a read of the `Player` record sees what it saw in 3.1c. The accounts are read inside the tick only by
economy itself and by `RequestGranted`, which is synchronous for that reason: a RESET or LOAD in the inbox sets the
stall it answers from, and a unit created in a slot whose last account was stalled can ask in the same phase.
`UnitAccount` is called outside the tick (the saved game) or by presentation, after the tick boundary has emptied
the inboxes. `GetEnergyUse` reads no account.

**The immediate rows of `docs/schedule.md`.** Every row of `data/dependencies.csv` of class `immediate` that has
economy as its writer or reader is on the `Player` records: the access log names them by the raw members of `Game`
that overlay `players[]` (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`). Each read relied on a same-phase write and is
served as follows:

| Writer | Reader | Phase | The write | How the read is served now |
|---|---|---|---|---|
| economy | ai, los_radar, orders, players, targeting, units, weapons | units | a weapon's SPEND of the stores | the synchronous `Spend`, applied at 3.1c's statement |
| economy | ai, los_radar, orders, players, targeting, units | players | the settlement of the stores, incomes and usages, and a transfer's SPEND | the run `UpdatePlayerEconomy`, at 3.1c's point of the players loop, and the synchronous `Spend` |
| economy | map | units | a weapon's SPEND of the stores (dynamic only) | the synchronous `Spend` |
| units | economy | units | the unit and player fields units writes in a unit's update | economy's synchronous slots and `UpdateMetalExtraction` read units' state as of its last run, which is the update in progress |

No immediate row reads an account (`UnitResources`, the `Economy region`), so no read depended on a REQUEST, INCOME,
RESET or LOAD being applied at once. The effects are judged by the gate (section 8).

## 5. The private region and the trace

**The region.** A unit's account (12 floats: for energy and for metal, what it made, asked for, was granted, its
stall, and the last tick's make and use) was 48 bytes of the unit record. It is now economy's: `RegionSize` gives
`unitSlots * 48`, the engine allocates it (`SimAllocRegion`, tag `Economy region`) in `LoadBattleAssets` before the
players are set up, `Init` receives it and `Shutdown` hands it back, as pathfinding's. The 48 bytes of the unit record
stay, unused, so no other offset moves. Only economy reads or writes the region: `tools/privatestate.py --check`
judges it from the access log. (`tools/seamlink.py --private-state` covers pathfinding's region only.)
`tools/seamlink.py --pilot-seams` checks the seam statically: only economy and the engine call its writing slots, and
no query slot's function writes state or makes an indirect call other than the world view's reads. One case it can't
see: inside `UnitAccount` itself, a write to the region looks like the write to its `out` parameter.

**What the trace hashes** (`bt-trace 5`, `docs/trace-format.md`):
- the view state above, as before;
- not the unit record's account bytes, in both branches: a re-implementation may keep its accounts in any form;
- the view state that the accounts feed: the `Player` stores, incomes and usages, which the settlement computes from
  them. A pilot that refuses a SPEND the default grants diverges there from that tick on
  (`tests/economy_writer_test.cpp`);
- not the economy events: section 33 records every event and effect as it is delivered, with its outcome, in the
  dump only (moddable only: the reference has no such records), as a diagnostic for a divergence;
- economy's `WriteState` (the accounts of the units in play) is dumped (section 34) and not hashed.

## 6. Where entry points split

Every one of economy's writes and reads inside the tick falls inside one entity's update: a unit's in the units phase,
or a player's in the players phase (`docs/schedule.md`). The scheduler holds those loops: the units phase delivers
units' inbox and calls `units.UpdateUnit` for each unit, and the players phase delivers players' inbox and calls
`players.UpdatePlayerHead`, then (after delivering economy's inbox) `UpdatePlayerEconomy` when it is due, then
`players.UpdatePlayerTail`, for each player. The requests, refunds, resets and income published inside a unit's or a
player's update wait in economy's inbox until economy's next synchronous slot or run, or the phase boundary; the
spends are applied where they are made. A parallel scheduler would have to split the units' update at each spend, or
apply it before the update continues, as `docs/schedule.md` marks them.

## 7. What stays inside the default

The difficulty handicap, 3.1c's settlement of the accounts (stalls, carried shortfalls and their rounding), the cloak
costs, the wind and tide figures, metal makers and the sharing rules of network play are the default's, not the
interface's: another implementation receives the same events and may decide differently, at the cost of conformity.

## 8. What the gate decided

The model queues every event except SPEND. The full `--check --both-modes` is the judge, and an event whose queueing
diverges becomes synchronous, with the reason here.

**CLOAK is synchronous.** Queued, the tier diverged in conditions-dark-side, first at tick 2460, phase world, field
`players[1].ai.visible.size` (reference 0, queued 1). 3.1c clears a cloaked unit's bit inside `UpdatePlayerEconomy`,
and `UpdatePlayerTail`, later in the same player's update, runs the sensors (`los_radar.UpdateSensorRadarAndCloak`),
which read the bit to decide what player 1's AI sees. Queued, the bit changed only at the phase boundary, so player 0's
commander stayed visible to player 1 for the rest of the tick. A synchronous route delivers units' inbox and calls
`UnitsOnEconomyCloak` before `Emit` returns, at 3.1c's statement.

**Queued, and equal:** RESET, LOAD, REQUEST, INCOME and EXTRACTOR, in every scenario in both modes.

**SHARE is queued by the model's rule, untested.** No scenario shares resources (section 2), so the gate cannot judge
it; this is the analysis instead. `UpdateResourceSharing` runs once a tick, for the local player, and the scheduler
drains the inboxes right after it, before the tail sends its packets. Inside it, after the metal SHARE is published,
the code reads only energy (the giver's store and threshold, the allies' stores and capacities), which the metal
transfer does not change: it spends the giver's metal and adds income to the receiver's account. After the energy
SHARE it reads no store. So the transfers, with their own SPEND and INCOME, change the same state in the same order and
the same tick as in 3.1c. One order does change: on a tick that is a multiple of 900, the share-map packets
(`SendShareMapInfo`) are now queued before the share's packet, where 3.1c queued them after it; they carry independent
state, and network compatibility with 3.1c is not kept (#2).

**`RequestGranted` is synchronous by analysis, not by a divergence.** Its answer depends on the stall, which a RESET or
LOAD in economy's inbox sets (section 4), and a unit created in a slot whose last account was stalled can ask in the
same phase. Queuing it is not possible: the caller needs the answer in the statement.
