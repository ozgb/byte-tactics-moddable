# The players interface

Players sits behind `interfaces/players.h` (version 3.0), the sixth module on the scheduler after economy
(`docs/economy-interface.md`), features (`docs/features-interface.md`), targeting (`docs/targeting-interface.md`),
los_radar (`docs/los-radar-interface.md`) and map (`docs/map-interface.md`) (#967). It is the players as the
simulation sees them: each player's update (its resources' bookkeeping, the commander's respawn, the end of the game),
the victory and defeat conditions of a mission, a unit given to another player, a player's units killed, and the
transfers of metal and energy. 3.1c's code is its default implementation. This page is the inventory the interface
was cut from:
- what other subsystems did to players' state in 3.1c, and what each became;
- what players did to others, and what each became;
- what players reads, and what others read of it;
- how the order of random draws and sim events is kept;
- what the trace hashes, and what it only dumps.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, events published
outside the tick delivered at once (#970), and events as plain data. This issue changes none of its rules. Event
kinds are unique across interfaces: economy uses 0 to 4 and 16 to 18, features 32 to 35 and 48 to 51, wind_meteors 64
to 67 and 80, targeting 96 to 99 and 112, los_radar 128 to 136, map 160 to 168 and 176, players 192 to 196 and 208 to
210 (its range is 192 to 223, effects from 208).

Players' place in it:
- **Events into players:** GIVE_UNIT, KILL_UNITS, UNIT_CREATED, UNIT_DIED and TRANSFER, published by units, orders
  and the engine (the network, the console and the in-game dialogs), and TRANSFER from economy's SHARE effect.
  Players' handler (`PlayersInbox`, `src/seams/players_world.cpp`) passes each to `Apply`, and records it with its
  outcome in the trace's dump (section 5). Economy's SHARE reaches it through `PlayersOnEconomyShare`, which builds a
  TRANSFER and hands it to the same handler; it moved from players' files into the same glue, as it is the engine's
  adapter, and it now goes through `Apply` instead of the seam slots `TransferMetal` and `TransferEnergy`.
- **Routes:** of the seven routes into players, six are synchronous: units' KILL_UNITS and UNIT_DIED and orders'
  GIVE_UNIT because the gate diverged with them queued, and the engine's KILL_UNITS, GIVE_UNIT and TRANSFER by #952's
  rule. Units' UNIT_CREATED stays queued, and so does economy's SHARE (#953). Of players' three effects, CAPTURE_UNIT
  is synchronous because the gate diverged with it queued, KILL_UNIT and SEND_UNIT by #952's rule (sections 4 and 8).
- **Runs:** `UpdatePlayerHead`, `UpdatePlayerBody`, `UpdatePlayerTail` and `UpdatePlayersFinish`, called by the
  scheduler's players phase (`SchedPlayersPhase`, `src/game/scheduler.cpp`), which delivers players' inbox before each.
  The scheduler now holds ai's two ticks of a player, `SquadManagerTickIfActive` and `UpdatePlayerAI`, which 3.1c's
  `UpdatePlayerHead` called: the head returns 1 when the engine is to run the computer player's turn, the line of sight
  pass and then the body, and the engine calls ai's ticks right after it, at 3.1c's statement. `UpdateBlink` stays a
  step of the tail, called by `RunGameSteps` as before.
- **Synchronous calls out:** the commander's respawn calls units' `CreateUnit` and reads the new unit in the same
  statement (section 3). The placement test of the respawn is map's synchronous `CanPlaceUnitFootprint`, through
  `ScheduleMapCanPlaceUnitFootprint` (services 1.16).
- **Queries:** map's `GetCellMeanHeight`, `GetGameType` and `GetNameSlot`, units' `FindUnitTypeId`, weapons'
  `FindFeatureAtPos`, los_radar's `ClampWorldPosToTerrain` and the service `VisitUnitsInRange`, all as of the owner's
  last apply.
- **Views:** the `Player` records (`Game::players`), which players, economy, units and the network write
  (section 4), and the conditions' objects, which only players reads.
- **Effects out of players:** KILL_UNIT, SEND_UNIT and CAPTURE_UNIT, to units. Units is not on the model yet, so its
  handlers (`src/units/players_effects.cpp`) are adapters that hold 3.1c's writes into the unit and its calls into
  weapons, network and units. Players records each effect in the trace's dump before it publishes it, as
  `docs/module-abi.md`, 9.3, asks of a module's effects.
- **Module ids:** players keeps `SCHED_PLAYERS = 4`, which economy's routes already used as an outbox.

## 2. Events into players

| 3.1c site | 3.1c call | Now |
|---|---|---|
| units: a unit created (`CreateUnit`, `CreateUnitFromPacket`) | `MissionConditions::NotifyUnitCreated` | UNIT_CREATED from units |
| units: a unit dies (`ApplyUnitDeath`) | `MissionConditions::NotifyUnitDied` | UNIT_DIED from units |
| units: a commander dies in a game that ends with its commander (`KillUnit`) | `KillPlayerUnits` | KILL_UNITS from units |
| orders: a capture finishes (`CaptureOrder`) | `GiveUnitToPlayer` | GIVE_UNIT from orders |
| the network: a player leaves (`RemovePlayer`) | `KillPlayerUnits` | KILL_UNITS from the engine |
| the network: an ownership record arrives (packet 0x14) | `GiveUnitToPlayer`, with the record | GIVE_UNIT from the engine, with the record's fields |
| the network: a share of metal or energy arrives (packet 0x16) | `TransferMetal`, `TransferEnergy`, not announced | TRANSFER from the engine |
| the console: `+kill`, `+iwin`, `+ilose` (`CmdKill`, `CmdIWin`, `CmdILose`) | `KillPlayerUnits` | KILL_UNITS from the engine |
| the console: `+give` (`CmdGive`) | `TransferMetal`, `TransferEnergy`, announced | TRANSFER from the engine |
| the in-game dialogs: give the selected units (`GiveSelectedUnitsToPlayer`), share resources | `GiveUnitToPlayer`, `TransferMetal`, `TransferEnergy` | GIVE_UNIT, TRANSFER from the engine |
| economy: the SHARE effect (`UpdateResourceSharing`) | `TransferMetal`, `TransferEnergy`, announced | TRANSFER, through `PlayersOnEconomyShare` |

**What each event writes.**
- **UNIT_CREATED and UNIT_DIED** call the conditions' `OnUnitCreated` and `OnUnitDied` on each victory and defeat
  condition of the mission (`Game::list_391ed`). They are virtual calls, resolved here over the 18 condition types of
  `src/game/victory.cpp`. No type overrides `OnUnitCreated`, so UNIT_CREATED writes nothing in 3.1c's conditions.
  `OnUnitDied` of `VictoryKillEnemyCommander`, `VictoryKillAllMobileUnits`, `VictoryKillAllOfType` and
  `VictoryKillUnitType` writes the condition's counter and `satisfied` flag and records the victory sound
  (`RecordSound(SIM_SOUND_VICTORY_CONDITION)`, a sim event); `VictoryKillAllMobileUnits` and `VictoryKillAllOfType`
  walk the dying unit's player's units for the ones left, and `VictoryKillAllOfType` asks units' `FindUnitTypeId`.
  `DefeatCommanderKilled`, `DefeatUnitTypeKilled` and `DefeatAllUnitsKilledOfType` write their counter and flag and
  record nothing. They read the dying unit's type, owner and flags, which units has already written when it publishes.
- **GIVE_UNIT** tells the conditions of the capture (`OnUnitCaptured`: `VictoryCaptureUnitType` writes its counter and
  flag and records the victory sound), then decides how the unit changes hands, from the unit's owner and the
  receiver's type, and publishes SEND_UNIT (a receiver on another machine) or CAPTURE_UNIT (a local one) (section 3).
  It writes no unit: the writes are the effects'.
- **KILL_UNITS** walks the player's units and publishes a KILL_UNIT for each that is alive and not dying (section 3).
- **TRANSFER** clamps an announced amount to the giver's store, spends it through economy's synchronous `Spend`
  (`ScheduleEconomySpend`), publishes INCOME to the receiver, and sends the share packet when announced. 3.1c's names
  are kept: `TransferMetal` moves energy and `TransferEnergy` moves metal, so TRANSFER's `value` names the resource
  (0 energy, 1 metal) instead.

**Answers.** One caller used a value: none. `GiveUnitToPlayer`, `KillPlayerUnits`, the conditions' notices and the
transfers returned nothing in 3.1c, so none of them is synchronous by its contract: every same-statement order is a
route's (section 4).

**Outcomes.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 41) and returned to no
one: 1 for UNIT_CREATED, UNIT_DIED and KILL_UNITS; for GIVE_UNIT, 1 when the unit changed hands and 0 when it was
already the receiver's, dead, dying, or the receiver could not take it; for TRANSFER, 1, or 0 when either player is
slot 10 or the amount is 0. `Apply` first rejects an event naming no unit (GIVE_UNIT, UNIT_CREATED, UNIT_DIED), a
player above 9 for GIVE_UNIT or above 10 for KILL_UNITS and TRANSFER, a resource other than 0 or 1, a UNIT_CREATED or
UNIT_DIED with no conditions, and an unknown kind, with outcome 0 and no change. KILL_UNITS takes slot 10, the
`Player` record past the ten players (`pad_2851`), because 3.1c's `KillPlayerUnits` is reachable with it from the
network (`RemovePlayer` of an unknown id); a module publishing from WASM can reach the others.

**Every slot of 2.0.** The 30 slots of players 2.0, and what each became in 3.0:

| 2.0 slot | 3.0 |
|---|---|
| `GiveUnitToPlayer`, `KillPlayerUnits`, `NotifyUnitCreated`, `NotifyUnitDied`, `TransferMetal`, `TransferEnergy` | events: GIVE_UNIT, KILL_UNITS, UNIT_CREATED, UNIT_DIED, TRANSFER, through the new slot `Apply` |
| `GetEnergyIncome`, `GetMetalIncome`, `GetNetEnergy`, `GetNetMetal` | removed: they read the `Player` record's incomes and usages, economy's view state, which ai and the info panel now read directly |
| `UpdatePlayerHead`, `UpdatePlayerBody`, `UpdatePlayerTail`, `UpdatePlayersFinish` | runs, called by the scheduler; the head no longer calls ai's ticks |
| `UpdatePlayers`, `UpdatePlayersOnStream` | the whole players phase, as the engine calls it outside the scheduler's loop (the load of a match) |
| `UpdateBlink` | a step of the tail, called by the engine |
| `LoadDefaultAIScript`, `InitPlayers`, `FreePlayers`, `InitPlayerResources`, `LoadPlayerControllers`, `LoadPlayers`, `SavePlayers`, `RebuildAIFeatureCells`, `SetupPlayerSlot` | set-up, save and load slots, outside the tick |
| `NewConditions`, `RegisterConditions`, `DeactivateConditions`, `LoadConditions`, `SaveConditions`, `FreeConditions`, `DeleteConditions` | the conditions' set-up, save and load slots, outside the tick |

**Calls outside the interface.** `docs/schedule.md` listed "a call into players outside its interface" 32 times, in
the units, projectiles, features, features.anim, meteors and tail passes, each right after an `X.Apply` step. Every one
of them is an adapter delivery, not a call outside the interface: it is economy's SHARE effect delivered to
`PlayersOnEconomyShare`, which `SchedDeliver` and the phase boundary reach after any module's apply, and which called
3.1c's `TransferMetal` and `TransferEnergy` directly, from players' file. The adapter now lives in the seams glue and
builds a TRANSFER for `Apply`, so it is the engine's delivery of players' own event, and the listing names it so. The
real direct calls into players were the engine's and the other subsystems' calls to the slots above; each is now an
event. Players' own calls into units, for the units issue: `CreateUnit` (the commander's respawn, synchronous,
section 3) and `FindUnitTypeId` (the respawn and three conditions, a query of the unit types); both go through units'
seam.

## 3. Effects out of players

**KILL_UNIT, SEND_UNIT and CAPTURE_UNIT, to units.** Each holds a write into a unit that 3.1c made inside players:
- **KILL_UNIT** (`value` 0 or 1): a unit of a player whose units are killed dies, by 30000 damage dealt by itself
  (`weapons.DamageUnit`) when its owner plays on this machine, or by self-destruct (`weapons.DetonateUnitWeapon`, the
  dying flag 0x4000 of `Unit::flags`, `units.KillUnit`) otherwise.
- **SEND_UNIT:** a unit given to a player on another machine stops being selectable (`Unit::flags` 0x10), waits out
  the transfer holdoff (`Unit::postTransferHoldoff`), is announced (`network.SendOwnershipTransfer`) and takes 30000
  damage.
- **CAPTURE_UNIT:** a copy of the unit is made for the receiver (`units.CreateUnit`), with the ownership record's
  health, build progress, angles and stockpiles when the event came from the network, else with the unit's own, and
  the unit takes 30000 damage; the copy takes the unit's state bits (`units.SetStateBits`).

The handlers (`UnitsOnPlayersKillUnit`, `UnitsOnPlayersSendUnit`, `UnitsOnPlayersCaptureUnit`,
`src/units/players_effects.cpp`) are units' adapters, as economy's CLOAK and EXTRACTOR are
(`src/units/economy_effects.cpp`): units owns the writes, and the effects carry a unit as its block offset and a
player as its index, so they are plain data. The three routes are synchronous (section 4), so
each write lands at the publisher's statement, as 3.1c's did: inside units' or orders' update that published the
GIVE_UNIT or KILL_UNITS, or inside the network packet that did.

**The commander's respawn is synchronous, not split.** In a game whose commander respawns (`UpdatePlayerBody`, game
type 1), players places a new commander at a random free position and calls units' `CreateUnit`, then, in the same
statement, gives the new unit's account its starting energy and metal (two INCOMEs to economy, keyed by the unit's
handle and its `playerIndex`), recalculates line of sight and records the camera's focus. A split into an intent would
need the unit back before the next statement, so it would be a synchronous route under another name. `CreateUnit` is a
call into units' seam, which is not on the scheduler, and it stays one; when units is cut (#2), it is a synchronous slot
of units. The respawn draws its position from the game logic stream (`RandomInt`), only in deathmatch, which no
scenario plays (section 6).

**Writes players keeps.** After this change players writes no field of a unit: `tools/rwsets.py` gives it no write of
`Unit`, `UnitMotion` or `Order`. It writes its own: the `Player` records' fields it owns (section 4), the conditions'
objects, and `Game::endGameCountdown` and `Game::endGameFlags`.

## 4. Views

**What players reads:** a unit's owner, type, flags and position (the conditions, the respawn and the kills); the
players' units (`unitsBegin`, `unitsEnd`, `unitCount`); the `Player` records' resources, which economy writes; the
players' alliance words; the mission's game type and name slot (map's queries); the cells' heights and features (the
respawn's placement). All as of each module's last run.

**What others read of players:** the `Player` records' fields players writes (`active`, `type`, `unitsCreated`,
`kills`, `losses`, the alliance words), read by nearly every subsystem; the end of the game
(`Game::endGameCountdown`, `endGameFlags`), read by the engine; the conditions, read by no other subsystem.

**The unnamed ranges.** The access log names three items of `Game` that are not players' fields:
`unknown_1b8e`, `unknown_1cd9` and `pad_1df4`. They are the second view of the `Player` array that 3.1c's code kept
for the first two players (the union over `Game::players`, `src/game/game.h`), and the log resolves an address to
the first item that covers it. Each holds:

| Item | Offset, size | What it holds |
|---|---|---|
| `p1b8a` | 7050, 4 | `players[0].info` |
| `unknown_1b8e` | 7054, 327 | `players[0]`, bytes 43 to 330 (from `name` to the end), then `players[1]`, bytes 0 to 38 (from `active` to `unknown_23`) |
| `p1cd5` | 7381, 4 | `players[1].info` |
| `unknown_1cd9` | 7385, 281 | `players[1]`, bytes 43 to 323 (from `name` to `alliance`) |
| `field_1df2` | 7666, 2 | `players[1].unitCount` (the condition `VictoryDestroyAllUnits` reads it by this name) |
| `pad_1df4` | 7668, 2988 | `players[1]` from `index` (byte 326) to the end, `players[2]` to `players[9]`, the record past them (`pad_2851`, slot 10), and the start of `duplicateIds` |

So a row with one of them is a row of the `Player` field at that offset. Each field's owner, the subsystem that writes
it inside the tick (from `data/rwsets.csv`):

| `Player` fields | Owner inside the tick |
|---|---|
| `energy`, `metal`, their incomes, usages, capacities, totals, wasted amounts and storage bonuses, `shareMetal`, `shareEnergy` | economy (players writes them at set-up and load) |
| `econ`, `updateTime`, `winLoseTime`, `displayTimer`, `firstIndex`, `lastIndex` | players |
| `unitsBegin`, `unitsEnd`, `unitCount`, `unitsCreated`, `kills`, `losses`, `commanderKills`, `commanderLosses`, `rank`, `flags` | units (players writes them at set-up and load, and `rank` and `flags` in its update) |
| `active`, `type`, `id`, `index`, `info`, `name`, `allied`, `alliedBy`, `alliance`, `shareLos`, `shareVision`, `shareMapping`, `explored`, the keep-alive, lobby and ping fields | the network and the front end, outside the simulation's phases (players writes several at set-up and load) |
| `ai`, `groups` | ai (players creates and frees the computer player) |

The economy, units → map, ai, los_radar, targeting rows on these items are economy's and units' writes of the fields
above, read by their readers; players' own rows are its writes of `rank`, `flags`, the timers and the end of the game,
as the table below says.

**The routes.** Every route, with its reason:

| Route | Now | Why |
|---|---|---|
| units KILL_UNITS | synchronous | The gate (section 8). |
| engine KILL_UNITS | synchronous | #952's rule. Queued it stayed equal on the tier, which has no network path: the engine publishes it from `RemovePlayer`, a packet of the net phase (`HandleNetPackets`), and the packets after it in the same phase read the dead units (a unit's state packet, a kill packet). The console's `+kill`, `+iwin` and `+ilose` publish it outside the tick (the console runs before `SchedBeginTick`, in the harness and in the main loop's input), where #970's rule delivers it at once. |
| units UNIT_CREATED | queued | Equal on the tier, and #952's rule does not need it: no condition type overrides `OnUnitCreated`, so the event writes nothing in the default, and nothing in the phase can read a difference while it is pending. Its order against UNIT_DIED is kept, as a synchronous route delivers players' inbox before it hands over its event. |
| units UNIT_DIED | synchronous | The gate (section 8). |
| orders GIVE_UNIT | synchronous | The gate (section 8). |
| engine GIVE_UNIT | synchronous | #952's rule. Queued it stayed equal on the tier, which has no network path: the engine publishes it from the ownership record of the net phase, and the packets after it in the same phase name the copy or the given unit (its state, its orders). The in-game dialog publishes it outside the tick, where #970's rule delivers it at once. |
| engine TRANSFER | synchronous | #952's rule. Queued it stayed equal on the tier: the engine publishes it from a share packet of the net phase (not announced), and from the console and the dialogs (announced, outside the tick, delivered at once by #970's rule). A share packet's write is the INCOME it publishes to economy: queued, that INCOME would enter economy's inbox at the phase boundary, after the economy events that the later packets of the same phase publish (the RESET of a unit created from a packet), in another order than 3.1c's. |
| economy SHARE | queued | Unchanged from #953 (`docs/economy-interface.md`, section 8): it is published in the tail by `UpdateResourceSharing`, and the scheduler drains the inboxes right after it. |
| players KILL_UNIT, to units | synchronous | #952's rule. Queued it stayed equal on the tier: KILL_UNITS is synchronous, and the kills waited in units' inbox until the next unit's update. They can be pending while later reads in the same phase need what they write: the commander that ends the game dies in the units phase or in the projectiles phase (a weapon's hit), and the next units' updates and projectiles read the killed units' health and flags, and draw from the game logic stream in their own order around the kills' explosions. |
| players SEND_UNIT, to units | synchronous | #952's rule; no scenario reaches it (the receiver must play on another machine). Queued, the given unit would stay selectable and take its damage at the next delivery, after later reads in the same phase of its flags and health. |
| players CAPTURE_UNIT, to units | synchronous | The gate (section 8). |

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv`, regenerated on this change, has these rows of
class `immediate` with players as writer or reader. Each is served as follows:

| Writer → reader | Items | Phase | How it is served |
|---|---|---|---|
| economy → players | `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` | players | The `Player` resources, written by `UpdatePlayerEconomy`, which the scheduler calls in a player's turn between its Body and its Tail, as 3.1c's `UpdatePlayers` did. Players reads them after it in the same turn and in the next players' turns. |
| economy → players | `unknown_1b8e` | tail | The resources `UpdateResourceSharing` writes, read by the TRANSFER that economy's SHARE builds. SHARE is queued and delivered when the scheduler drains the inboxes right after `UpdateResourceSharing` (section 4). |
| players → ai, los_radar | `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` | players | The fields players writes in `UpdatePlayerHead` (`rank`, `flags`, the timers), read by ai's two ticks and the LOS loop, which the scheduler calls right after the Head in the same player's turn. |
| players → targeting, units | `unknown_1b8e`, `pad_1df4` | players | The same fields, read by targeting and units in the calls ai's ticks make, later in the same player's turn. |
| players → los_radar | `bits_142f1` | tail | `UpdateBlink` flips the blink bit, and los_radar sets and reads the redraw bits of the same word in the tail. They are different bits of one word, at the engine's statements, in 3.1c's order. |
| units, weapons → players | `Unit::def`, `flags`, `motion`, `unitDefIndex` | units | The dying, killed or captured unit, read by players' synchronous UNIT_DIED, KILL_UNITS and GIVE_UNIT at the publisher's statement, before the publisher frees or changes the unit. |
| units → players | `field_1df2`, `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` | units | The counts and the unit lists of the `Player` records (`unitCount`, `unitsBegin`, `unitsEnd`, `kills`, `losses`), written by units, read by the conditions in the synchronous UNIT_DIED at the publisher's statement, after units' own writes, as in 3.1c. |

## 5. State and the trace

Players keeps no private region, and so it has no `WriteState` report: its state is view state, hashed where it was.
It stays where it is and stays hashed in this change; #979 moves out of the hash what only its owner reads, for
los_radar, players, ai and map in one format bump. The Head and Body split of a player's update stays as #962 made it,
and so does the synchronous RECALCULATE to los_radar.

**What only players reads, for #979.** Hashed state that no other subsystem reads inside the tick:

| Players' state | Where | Read by | Written by |
|---|---|---|---|
| The conditions: `MissionConditions`, the `MissionCondition` list and the 18 `Victory*` and `Defeat*` types | the conditions' allocation (`Game::list_391ed`), hashed (`trace_schema`'s `MissionCondition` rows) | players | players |
| `Player::updateTime`, `Player::winLoseTime` | the `Player` records, hashed | players | players |
| `Game::endGameCountdown` | the state block, hashed | players | players (the engine sets it at set-up) |

`privatestate.py --check` fails when another subsystem writes any of the items players alone writes: the conditions'
types and `Player::updateTime` and `Player::winLoseTime`. `endGameCountdown` is not listed: the engine sets it when a
game starts. `seamlink.py --pilot-seams` checks the seam: only players and the engine call `Apply` and the runs.

**The trace dumps players' events** in section 41 (`docs/trace-format.md`, section 8.29), after map's section 40, and
hashes none of them. Each record is an event with the outcome `Apply` gave (`outcome` 0 or 1), or an effect players
published (`outcome` 1, as it is recorded before the publish), in players' order. The section is new, so the trace
format's schema changes and the gate of this change is the full suite.

## 6. Random draws and sim events

Players draws from the game logic stream in one place: the commander's respawn draws a position (`RandomInt`) in
deathmatch, at its own statement in the players phase. No scenario draws there (`data/draws.csv` has no players row),
and no event of players draws.

Players records sim events in its events: the victory sound of a condition (`RecordSound`), under UNIT_DIED and
GIVE_UNIT. The class `ordered` of `data/dependencies.csv` has these `sim-event` edges with players:

| Writer → reader | Phase | How it is kept in order |
|---|---|---|
| players → units, ai → players | units | The victory sound of the synchronous UNIT_DIED, recorded at the publisher's statement in the unit's update, between the sim events units and ai record before and after it. |
| players → los_radar, los_radar → players | players | The victory sound of `CheckVictory` in a player's Body, and los_radar's radar records (`RecordRadarUnits`, `RecordRadarMapped`), recorded in the order the scheduler calls them in each player's turn (section 1), as in 3.1c. |

## 7. What stays inside the default

3.1c's types and limits are the default's, not the interface's: the 18 condition types and their file format, the
`Player` record's layout and the slot past the ten players, the end-of-game countdown, the respawn's placement search,
and the rule that decides between SEND_UNIT and CAPTURE_UNIT. An event carries a unit as its block offset and a player
as its index; an ownership record's fields travel in the event. The conditions' set-up, save and load slots, and the
players' own, stay in `PlayersSeam` as 3.1c's code (section 2).

## 8. What the gate decided

Every route into and out of players was queued first, each on its own, with the others synchronous (a build that read
the policy of each route from the environment, not merged). The tier judged each build, and a route whose queueing
diverged became synchronous. The divergences are against the reference, with the first tick, phase and field. With
every route queued at once, the tier diverged first in conditions-dark-side, tick 2674, as orders' GIVE_UNIT does
alone; with every route synchronous, it was equal.

| Route queued | Tier | First divergence |
|---|---|---|
| units KILL_UNITS | diverged | eight-full-moon, tick 8081, units phase: `rng.parkMiller` (reference 0x615c60c2, queued 0x18443035), then the exploded pieces and 13,188 fields. A commander's death kills its player's units; queued, the kills (and their explosions' draws) came after the rest of the phase's draws. |
| engine KILL_UNITS | equal | none (synchronous by #952's rule, section 4) |
| units UNIT_CREATED | equal | none (queued, section 4) |
| units UNIT_DIED | diverged | mission-radius, tick 2523, world phase: `features[0].x` (reference 0x0064, queued 0), then 448 fields, among them `conditions.defeat[0].satisfied` (reference 1, queued 0) and `numLeftToKill` (reference 0, queued 1). The defeat condition had not yet counted the last death when the world phase was hashed, so the mission did not end on the reference's tick. |
| orders GIVE_UNIT | diverged | conditions-dark-side, tick 2674, units phase: `units[1].cargoNext` (reference none, queued unit 252), then the captured unit's claim bit (`units[2].flags`), `players[1].unitCount` and the cells under it. The copy of the captured unit was made at the phase boundary, after the units updated after the capturer had seen the old unit. |
| engine GIVE_UNIT | equal | none (synchronous by #952's rule, section 4) |
| engine TRANSFER | equal | none (synchronous by #952's rule, section 4) |
| players KILL_UNIT | equal | none (synchronous by #952's rule, section 4) |
| players SEND_UNIT | equal | none (synchronous by #952's rule; no scenario reaches it) |
| players CAPTURE_UNIT | diverged | conditions-dark-side, tick 2674, units phase: `units[2].+0xba` (reference 0x0004, queued 0), then the captured unit's script channels. |

With every route as above, the tier was equal (38 of 38 scenarios), and so was the full suite in both modes, the gate
of this change (178 of 178 scenarios equal; sound, UI logs and event digests 89 of 89, frames 6 of 6 and saves 1 of 1
in both modes).

**How often each route is taken.** A build that counts every publish into and out of players, over the 89 scenarios
with native modules (all 89 equal to the reference): the events published inside the tick and outside it (when a game
is set up or loaded, and the console's commands), and in how many scenarios each was published at all.

| Route | Inside the tick | Scenarios | Outside the tick | Scenarios |
|---|---|---|---|---|
| units KILL_UNITS | 10 | 8 | 1 | 1 |
| engine KILL_UNITS | 0 | 0 | 3 | 3 |
| units UNIT_CREATED | 1,992 | 53 | 1,424 | 89 |
| units UNIT_DIED | 937 | 35 | 4 | 1 |
| orders GIVE_UNIT | 2 | 2 | 0 | 0 |
| engine GIVE_UNIT | 0 | 0 | 0 | 0 |
| engine TRANSFER | 0 | 0 | 3 | 1 |
| economy SHARE | 42 | 3 | 0 | 0 |
| players KILL_UNIT | 50 | 3 | 4 | 3 |
| players SEND_UNIT | 0 | 0 | 0 | 0 |
| players CAPTURE_UNIT | 2 | 2 | 0 | 0 |

No scenario plays a network game, so the engine's routes are taken only by the console, outside the tick.

**Inbox depths.** Players' inbox was never deeper than 3, at a publish or at a phase boundary, in any scenario:
long-greenhaven-8p at tick 43852, three UNIT_CREATED of units made in the units phase, waiting for the boundary. The
SHARE of the `share-` scenarios waits only from `UpdateResourceSharing` to the drain right after it. The most players
events published inside one unit's update (`UpdateUnit`) was 2, in mission-timer at tick 186 (unit 361).
