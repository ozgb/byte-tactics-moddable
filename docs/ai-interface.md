# The ai interface

Ai sits behind `interfaces/ai.h` (version 6.0), the seventh module on the scheduler after economy
(`docs/economy-interface.md`), features (`docs/features-interface.md`), targeting (`docs/targeting-interface.md`),
los_radar (`docs/los-radar-interface.md`), map (`docs/map-interface.md`) and players (`docs/players-interface.md`)
(#968). It is the computer players: what each one sees and wants, its squads and their timers, and the orders and
targets it gives its units, with the AI commands of the scripts and the console (`plan`, `weight`, `limit`). 3.1c's
code is its default implementation. This page is the inventory the interface was cut from:
- what other subsystems did to ai's state in 3.1c, and what each became;
- what ai did to others, and what each became;
- what ai reads, and what others read of it;
- how the order of random draws and sim events is kept;
- what the trace hashes, and what it only dumps.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, events published
outside the tick delivered at once (#970), and events as plain data. This issue changes none of its rules. Event
kinds are unique across interfaces: economy uses 0 to 4 and 16 to 18, features 32 to 35 and 48 to 51, wind_meteors 64
to 67 and 80, targeting 96 to 99 and 112, los_radar 128 to 136, map 160 to 168 and 176, players 192 to 196 and 208 to
210, ai 224, 226 to 229 and 240, 241, 243, 245 and 246 (its range is 224 to 255, effects from 240; 225, 242, 244 and 247 are not reused), weapons' effects 272 and 273.

Ai's place in it. Ai's interface has no query another module calls and no event that serves a player who is not a
computer player.
- **Events into ai:** REACT_TO_ATTACK, published by the engine's adapter of weapons' ATTACKED (`AiOnWeaponsAttacked`,
  `src/seams/ai_world.cpp`); SET_SQUAD, published by units and the engine (the squad keys and the test harness); PLAN,
  WEIGHT and LIMIT, published by the engine from the AI scripts and the console. Ai's handler (`AiInbox`,
  `src/seams/ai_world.cpp`) passes each to `Apply`, and records it with its outcome in the trace's dump (section 5).
- **Routes:** of the six routes into ai, two are synchronous: the REACT_TO_ATTACK adapter's, because the gate diverged
  with it queued, and units' SET_SQUAD by #952's rule. The engine's four (SET_SQUAD, PLAN, WEIGHT, LIMIT) are queued:
  the engine publishes them only outside the tick, where #970's rule delivers them at once. Of ai's five effects,
  ADD_ORDER, STOP and ACTIVATE are synchronous because the gate diverged with each queued, QUEUE_BUILD and SET_STATES
  by #952's rule (sections 4 and 8).
- **Runs:** `UpdateSquads` (3.1c's `SquadManager::TickIfActive`, behind the slot `SquadManagerTickIfActive` of ai 3.1)
  and `UpdatePlayerAI`, called by the scheduler's players phase (`SchedPlayersPhase`, `src/game/scheduler.cpp`) in each
  player's turn, right after players' `UpdatePlayerHead` and before the line of sight pass, where #967 put them. In
  the same place, right before `UpdatePlayerAI`, the scheduler calls `los_radar.RefreshUnitIndex(player)` and then
  `units.RefreshFactoryIndex(player)` (section 5). The scheduler delivers ai's inbox before each of the two runs.
  `UpdateSquads` takes a player's index; it does nothing for a player without a squad manager, as 3.1c's caller
  checked.
- **Synchronous calls out:** none. Ai calls no slot of another module that writes: every write it made in 3.1c is an
  effect (section 3) or an intent to targeting (#955, kept).
- **Queries:** orders' `GetOrderType` and `GetOrderFlags`, weapons' `WeaponCanReachUnit` and `WeaponCanReachPos`,
  economy's `GetEnergyUse`, units' and map's lookups, los_radar's `GetSeenUnitsInRadius`, and targeting's
  `FindWeaponTarget` split (#955), all as of the owner's last apply.
- **Views:** the units' fields ai reads (position, flags, orders, health, type, owner), the `Player` records with
  economy's incomes and usages, map's metal sums and the line of sight tables (section 4). Ai's own state is read by no
  other simulation module.
- **Effects out of ai:** ADD_ORDER, QUEUE_BUILD and STOP, to orders; ACTIVATE and SET_STATES, to units. Their
  handlers are adapters in `src/seams/orders_world.cpp` and `src/seams/units_world.cpp` (units' hand the effect to
  `UnitsApply`, `src/units/units_apply.cpp`). Ai records each effect in the trace's dump before it publishes it, as
  `docs/module-abi.md`, 9.3, asks of a module's effects. Its targets already were effects: SET_UNIT and CLEAR to
  targeting, synchronous, and SET_POS, queued (#955, #975; `docs/targeting-interface.md`, section 4). They are
  unchanged.
- **Module ids:** ai keeps `SCHED_AI = 10`, which #955 added for its targeting outbox.

## 2. Events into ai

| 3.1c site | 3.1c call | Now |
|---|---|---|
| weapons: a unit takes damage (`ApplyUnitDamage`) | `ReactToAttack` | weapons' effect ATTACKED (272), delivered by four routes in 3.1c's order: units' notice, the engine's adapter (`AiOnWeaponsAttacked`, `src/seams/ai_world.cpp`), which hands ai an `AI_REACT_TO_ATTACK` event, orders' return fire and units' "under attack" speech |
| weapons: a shot hits (`ApplyWeaponHit`, `DetonateProjectile`, `ApplyAreaDamage`) | `SquadManager::MarkOwnerNetDirtyFromDamageSplit`, on the shooter's owner's squad manager | weapons' effect HIT (273) to orders, naming the shooter; no longer an ai event |
| units: a unit made (`InitUnitFromType`) | `SetUnitSquad`, squad 0 | SET_SQUAD from units |
| units: a unit dies (`ApplyUnitDeath`) | `SetUnitSquad`, squad -1 | SET_SQUAD from units |
| units: a saved unit loaded (`LoadUnit`) | `SetUnitSquad`, the saved squad | SET_SQUAD from units, outside the tick |
| the squad keys (`CreateSquad`, `src/ingame/selection.cpp`) | `SetUnitSquad`, a direct call into ai's file | SET_SQUAD from the engine, outside the tick |
| the test harness: the quirk reach check puts a unit in the scout squad (`ReachQuirkGuard`) | `SetUnitSquad` | SET_SQUAD from the engine, outside the tick |
| the AI scripts and the console: `plan`, `weight`, `limit` (`CmdPlan`, `CmdWeight`, `CmdLimit`) | the slots `CmdPlan`, `CmdWeight`, `CmdLimit` | PLAN, WEIGHT, LIMIT from the engine, outside the tick |

**What each event writes.**
- **REACT_TO_ATTACK** serves the computer players only. Ai's interface has no event that serves a player of another
  type. Its publisher is the engine's adapter of weapons' ATTACKED (`AiOnWeaponsAttacked`); the rest of 3.1c's
  `ReactToAttack` is weapons' effect ATTACKED to units (the notice, NotifyUnitRefs 16), to orders (the return fire of a
  human or computer player's idle unit, `OrdersOnWeaponsAttacked`: an attack order from orders' own
  `IssueAttackOrder`, else targeting's SET_UNIT on orders' outbox) and to units (the "under attack" speech, unless the
  unit's order says not to). For a unit with type flag 0x1000 of an active computer player, ai draws the squad
  manager's next action (`RandomInt(300)`, section 6) and stops the unit (STOP). It writes the squad manager's
  `nextAction`; the STOP is an effect.
- **DAMAGE_SPLIT** is gone (ai 5.0): it served every player, and its only effect, HIT, set the wake bits
  (`netDirtyFlags` 0x4000 when the shot hit mostly enemies, enemy damage above twice the friendly damage, else 0x2000)
  that orders' `RunOrders` reads for every player's shooters. Weapons publishes `WEAPONS_EFFECT_HIT` (273) to orders
  (`OrdersOnWeaponsHit`, synchronous) and ai sees nothing.
- **SET_SQUAD** moves the unit from its squad's list to the new squad's (`Player::groups`) and sets `Unit::group`.
- **PLAN** switches the AI commands on or off (`g_aiCommandsEnabled`) from the match's difficulty.
- **WEIGHT** and **LIMIT** write the build weights and limits of the computer players' AI (`PlayerAI`).

**The AI commands are parsed by the engine.** #964's rule for writers from outside the tick: the console command's
argument parsing moved out of the module into the engine (`CmdPlanSlot`, `CmdWeightSlot`, `CmdLimitSlot`,
`src/game/console_commands.cpp`, registered by `RegisterAICommands`), and the module gets plain data. PLAN's words
(`easy`, `medium`, `hard`, `any`) become four bits, WEIGHT's and LIMIT's unit type or category becomes a set of type
bits (the engine asks units' `UnitTypeSetAddTypeOrCategory`, as 3.1c's `CmdWeight` did). 3.1c's quirk that `any` is
looked for only in the first argument is kept by the parser. The parser now names the types even when the commands are
off, where 3.1c's `CmdWeight` and `CmdLimit` returned first; `UnitTypeSetAddTypeOrCategory` writes only the set it is
given, so nothing else changes. Every AI command is published outside the tick (the scripts run when a match is set up
and on the console's `ReloadAIProfiles`), so each is consumed before the publishing statement returns, at 3.1c's point.

**Answers.** None. `ReactToAttack`, `MarkOwnerNetDirtyFromDamageSplit`, `SetUnitSquad` and the AI commands returned
nothing in 3.1c, so none is synchronous by its contract.

**Outcomes.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 42) and returned to no
one: 1 for REACT_TO_ATTACK, SET_SQUAD and PLAN; for WEIGHT and LIMIT, 1, or 0 when the AI commands are
off. `Apply` first rejects, with outcome 0 and no change, an event naming no unit (REACT_TO_ATTACK,
SET_SQUAD), a SET_SQUAD outside -1 to 9 or for a unit whose player has no squads, a PLAN with a bit above 3, and an
unknown kind. 3.1c's `SetUnitSquad` had no such checks and wrote outside the squads; every 3.1c caller passed a valid
squad, so no trace depends on it.

**Every slot of 3.1.** The 24 slots of ai 3.1, and what each became in 5.0:

| 3.1 slot | 4.0 |
|---|---|
| `ReactToAttack`, `SetUnitSquad`, `CmdPlan`, `CmdWeight`, `CmdLimit` | events: REACT_TO_ATTACK (narrowed to the computer player's part), SET_SQUAD, PLAN, WEIGHT, LIMIT, through the new slot `Apply` |
| `MarkOwnerNetDirtyFromDamageSplit` | gone from ai: weapons' effect HIT to orders |
| `SquadManagerTickIfActive`, `UpdatePlayerAI` | runs, called by the scheduler: `UpdateSquads` (by a player's index, not a `SquadManager*`) and `UpdatePlayerAI` |
| `SquadManagerDeleteTimers` | `FreeSquadManager`, the counterpart of `CreateSquadManager`: it deletes the timers and the squad manager, which players' `FreePlayers` did with ai's type |
| `GetVisibleEnemiesInRadius`, `GetVisibleEnemiesInRadiusList`, `HasReadyUnitInRange`, `IsPointVisible`, `GetFactoriesInRadius` | gone from ai in 5.0: los_radar's (the first four) and units' (the last), section 4 |
| `CreatePlayerAI`, `DestroyPlayerAI`, `CreateSquads`, `FreeSquads`, `CreateSquadManager`, `ResetAIPlayers`, `RebuildFeatureCells`, `ParseDownloadableAiWeightScripts`, `ReparseAiWeightScriptsIfLimitNotSticky`, `DumpPlayerAI` | set-up, load and debugging slots, outside the tick |
| (new) `WriteState` | ai's report for the trace's dump (section 5) |

**Calls outside the interface.** `docs/schedule.md` listed "a call into ai outside its interface" twice, in orders'
`TeleportOrder`. Neither was a call into the AI: `TeleportOrder` builds a `Vec`, a 3-vector type whose header lived in
ai's folder (`src/ai/vec.h`), so the file rule of `tools/coupling.py` put its inline operators in ai. The header moved
to `src/util/vec.h`, where the other maths helpers are, and the rows went. The direct calls into ai were the engine's
and the other subsystems' calls to the slots above and to `SetUnitSquad`; each is now an event.

## 3. Effects out of ai

Every write ai makes into another subsystem's state is an effect, delivered to an adapter of the subscriber:

| Effect | 3.1c call | Subscriber, handler | What it does |
|---|---|---|---|
| ADD_ORDER | `orders.AddOrder` (timers, squads' `Send`) | orders, `OrdersOnAiAddOrder` | gives the unit an order of the type ai chose with `GetOrderType`, against a unit or a position, replacing its orders or queued behind them |
| QUEUE_BUILD | `orders.QueueBuildOrder` (the build timer) | orders, `OrdersOnAiQueueBuild` | queues units to build at a factory, by type index; the adapter passes the type's name, as 3.1c did |
| STOP | `orders.DeleteOrders` (`ReactToAttack`, a computer player's unit) | orders, `OrdersOnAiStop` | deletes the unit's orders |
| ACTIVATE | `units.SetStateBits` (metal makers, `UpdateConverter`, the build timer) | units, `UnitsOnAiActivate` | switches the unit on or off |
| SET_STATES | a direct write of `Unit::flags` (`SquadManager::AssignSquads`) | units, `UnitsOnAiSetStates` | sets the unit's movement and firing states (bits 18 to 21 of `Unit::flags`) |

**ATTACK, NOTIFY_REFS and HIT left ai (#991); their numbers 242, 244 and 247 are not reused.** They served every
player, not only the computer players, so they are weapons' routes now (section 2): the notice is units'
(`UnitsOnWeaponsAttacked`, NotifyUnitRefs 16), the return fire is orders' (`OrdersOnWeaponsAttacked`, in
`src/seams/orders_world.cpp`), and the wake bits of HIT, which orders' `RunOrders` consumes as order events (units
clears them when a unit is made), are written by `OrdersOnWeaponsHit`.

**Return fire is an orders effect.** 3.1c's `ReactToAttack` used the answer of `IssueAttackOrder` in the same
statement: it retargeted the weapons only when no attack was ordered. Orders' handler keeps that: an idle unit of a type
1 or 2 player that can reach the attacker is ordered to attack with orders' own `IssueAttackOrder`, as 3.1c's code did,
else its weapons are retargeted with targeting's SET_UNIT, published on orders' outbox. It writes orders' state (the
order list) and asks targeting through orders' outbox, so ai no longer checks or publishes it.

**`FindWeaponTarget` keeps #955's split.** Ai's call of targeting's `FindWeaponTarget` stays the query and intent pair
#955 made, and its SET_UNIT and CLEAR to targeting stay synchronous, its SET_POS queued.

**Writes ai keeps.** After this change ai writes no field of another subsystem: `tools/rwsets.py` gives it no write of
`Unit` other than `Unit::group` (the squad, ai's own field of the unit), and none of `Order` or `UnitMotion`. It writes
its own: the `PlayerAI` of each player, the squad managers and their timers, the squads, `g_aiCommandsEnabled`.

**Payloads.** Every event and effect starts with `AiEventHeader`: `kind` and `unit` (the unit it is about, a block offset; 0 for PLAN, WEIGHT and LIMIT). Each kind has the payload of `interfaces/ai.h` (version 6.0) that starts with it, or is the header alone. `Apply(event, size)` refuses an event smaller than its kind's payload, with outcome 0 and no change, besides the refusals of section 2.

| Kind | Payload | Fields after the header |
|---|---|---|
| REACT_TO_ATTACK 224 | `AiReactToAttack` | `attacker` (a block offset, 0 for none or dead), `damage` |
| SET_SQUAD 226 | `AiSetSquad` | `squad`: 0 to 9, or -1 to only leave the squad |
| PLAN 227 | `AiPlan` | `difficulties`: bit 0 easy, 1 medium, 2 hard, 3 any |
| WEIGHT 228 | `AiWeight` | `scale`, `fix` (1 to fix the weights), `types[16]` (a set of unit types: type n is bit n % 32 of word n / 32) |
| LIMIT 229 | `AiLimit` | `limit`, `fix` (1 to fix the limits), `types[16]` (as in WEIGHT) |
| ADD_ORDER 240 | `AiAddOrder` | `type` (the order's type), `queue` (0 replace, 1 queue behind), `target` (a block offset, 0 for none), `hasPos`, `pos[3]` (world coordinates, 16 fractional bits), `params[2]` |
| QUEUE_BUILD 241 | `AiQueueBuild` | `type` (the unit type), `count` |
| STOP 243 | `AiStop` | `all`: 1 to delete every order, 0 only those not marked to stay |
| ACTIVATE 245 | `AiActivate` | `bits` (the state bits, bit 0 switched on), `on` (1 set, 0 clear) |
| SET_STATES 246 | `AiSetStates` | `movement` (0 hold position, 1 manoeuvre, 2 roam), `firing` (0 hold fire, 1 return fire, 2 fire at will) |

## 4. Views and queries

**What ai reads:** the units' position, flags, type, owner, health, build progress, speed, weapons, last attacker and
damage type, and their orders (`Unit::list`, `Order::flags_42`), written by units, orders and weapons; the `Player`
records, among them economy's `energyIncome`, `energyUsage`, `metalIncome` and `metalUsage` (`GetBuildRating`,
`UpdateConverter`, `BuildTimer::OnTimer`), which players 3.0 no longer serves through getters; map's metal sums
(`g_lastPlaceMetalSum`); the line of sight tables (`IsPointVisible`, now los_radar's query); and los_radar's visible
list, through `GetSeenUnitsInRadius`, which the spatial timer sums its ratings from (it read `visible` directly before).
All as of each module's last run.

**What others read of ai:** nothing. `tools/rwsets.py` has no read of `PlayerAI`, `SquadManager`, the squad timers,
`Group` or `g_aiCommandsEnabled` outside ai, the trace's harness and players' set-up of a player slot
(`SetupPlayerSlot` and `ResetPlayerSlots`, which write `Player::ai`, and `RebuildAIFeatureCells`, which reads it to call
ai's slot). `Unit::group` is read by orders' `GetBuiltOrder`, which copies a factory's squad to the unit it built for a
human player, and is written there too (3.1c's direct write, kept).

**The queries left ai.** Ai's interface has no query another module calls. The five it had read the per-player unit
index, which is not ai's (section 5), or no ai state at all:

| Query | Now | Callers inside the tick |
|---|---|---|
| `GetVisibleEnemiesInRadius`, `GetVisibleEnemiesInRadiusList` | los_radar's (`docs/los-radar-interface.md`); they read los_radar's visible and known lists, then the units' positions and flags | orders (`GuardNoMoveOrder`, `WaitOrder`) and targeting's weapon search (`FindWeaponTarget`); `GetVisibleEnemiesInRadiusList` has no caller in 3.1c's code (a kit slot) |
| `GetFactoriesInRadius` | units' (`interfaces/units.h`; units has no interface document yet); it reads units' factory list, then the factories' type and `activateFlags` | orders (the VTOL orders' pad search: `FindPads`, `Land`, `AirStrikeOrder`, ...) |
| `HasReadyUnitInRange` | los_radar's; it reads the visible list, then the units' flags | the info panel (`UpdateSensorRadarAndCloak`), presentation only |
| `IsPointVisible` | los_radar's (it never read ai state): the player's explored size and the line of sight mask (`Game::visibilityMask`) | weapons' drawing (`DrawProjectiles`) and players' `IsVisible` |
| (new) `GetSeenUnitsInRadius` | los_radar's: the buffer form, with no ready filter and no known fallback | ai's spatial timer |
**The routes.** Every route, with its reason:

| Route | Now | Why |
|---|---|---|
| weapons ATTACKED, to units, the engine's adapter of ai's REACT_TO_ATTACK, orders and units' speech | synchronous | The gate (section 8). Four routes in 3.1c's order of `ReactToAttack`: units' notice, ai's computer player part, orders' return fire, units' speech. |
| weapons HIT, to orders | synchronous | #952's rule. Queued it stayed equal on the tier. It sets bits 0x2000 or 0x4000 of the shooter's `Unit::netDirtyFlags` in the projectiles phase, and targeting's `SetWeaponTargetUnit` and `SetWeaponTargetPos` clear bits 10 to 14 of the same word (`&= 0x83ff`) later in the same phase, when ai or the unit's weapons retarget it: the `immediate` row orders → targeting, `Unit::netDirtyFlags`, projectiles (ai → targeting before HIT was an effect; mission-radius at tick 1355). Pending, the bit would be set after the clear instead of before it. |
| units SET_SQUAD | synchronous | #952's rule. Queued it stayed equal on the tier. Orders' `GetBuiltOrder` writes `Unit::group` of a human player's unit when it is finished, in the units phase, and units' SET_SQUAD 0 of the same unit, published when it is made: a unit made and finished in one units phase would get its squad 0 after the factory's, instead of before. A dying unit's SET_SQUAD -1 also leaves its squad's list holding it until the boundary. |
| engine SET_SQUAD | queued | Published only outside the tick: the squad keys (input, between ticks) and the test harness's quirk reach check (before the tick), where #970's rule delivers it at once. |
| engine PLAN, WEIGHT, LIMIT | queued | Published only outside the tick: the AI scripts at a match's set-up and the console, where #970's rule delivers them at once (section 2). |
| ai ADD_ORDER, to orders | synchronous | The gate (section 8). |
| ai QUEUE_BUILD, to orders | synchronous | #952's rule. Queued it stayed equal on the tier. It writes the factory's orders (`Unit::list`), which ai's timers read later in the same player's turn (`!u->list`, the build and scout timers): the read of ADD_ORDER's divergence. Ai's inbox delivery before a run does not deliver orders' inbox, so the order could be pending at that read. |
| ai STOP, to orders | synchronous | The gate (section 8). |
| ai ACTIVATE, to units | synchronous | The gate (section 8). |
| ai SET_STATES, to units | synchronous | #952's rule. Queued it stayed equal on the tier. It writes the movement and firing states (`Unit::flags`) in the squads' tick, and in the same player's turn ai's `UpdatePlayerAI`, the line of sight pass and targeting read them: the `immediate` rows units → los_radar and units → targeting, `Unit::flags`, players (ai → los_radar and ai → targeting before SET_STATES was an effect). |

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv`, regenerated on this change, has 41 rows of
class `immediate` with ai. In every one ai is the reader: ai writes nothing another module reads in the same phase,
because each of its writes into others is now an effect, applied by the subscriber's handler. The rows that had ai as
writer before this change (ai → los_radar and ai → targeting, `Unit::flags`, players; ai → targeting,
`Unit::netDirtyFlags`, projectiles) now have the handler's module as writer: units → los_radar and units → targeting,
`Unit::flags`, players (SET_STATES), and orders → targeting, `Unit::netDirtyFlags`, projectiles (weapons' HIT, delivered to orders), at the same
first ticks. Each row is served as follows:

| Writer → reader | Items | Phase | How it is served |
|---|---|---|---|
| economy → ai | `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` | players | The `Player` resources (`energyIncome`, `energyUsage`, `metalIncome`, `metalUsage` and the stored amounts; `docs/players-interface.md`, section 4, names the ranges), written by `UpdatePlayerEconomy` in each player's turn and read by ai's runs after it in the same turn and in the next players' turns, in the scheduler's order. |
| players → ai | `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` | players | The fields players writes in `UpdatePlayerHead`, read by ai's runs, which the scheduler calls right after the Head in the same turn. |
| los_radar → ai | `Unit::flags` | players | The sight bits the line of sight pass of an earlier player's turn writes, read by ai's `UpdatePlayerAI` in a later turn, in the scheduler's order. |
| units → ai | `Unit::flags`, `Unit::activateFlags` | players | The states and activation that ai's synchronous SET_STATES and ACTIVATE write through units' handlers at ai's statement, read by ai later in the same turn. |
| orders → ai | `Order::flags_42`, `Unit::list` | players | The orders ai's synchronous ADD_ORDER and QUEUE_BUILD give through orders' handlers at ai's statement, read by ai's timers later in the same turn (`!u->list`): the read of ADD_ORDER's divergence (section 8). |
| map → ai | `g_lastPlaceMetalSum` | players | The placement record map's synchronous slots `CanBuildAt` and `CanPlaceUnitFootprint` fill, read by ai right after the call (`docs/map-interface.md`). |
| weapons → ai | `Unit::f_1f`, `Unit::f_57` | players | The flags of the first and third weapon slots, written by weapons in calls made earlier in the same turn at their callers' statements, read by ai's checks of a unit's weapons. |
| weapons, orders → ai | `Unit::flags`, `Unit::f_1f`, `Unit::f_57`, `Unit::lastAttackerSlot`, `Unit::lastDamageType`, `Order::flags_42`, `Unit::list` | projectiles | The damaged unit, its weapons and its orders, read by ai's synchronous REACT_TO_ATTACK (now the engine's adapter of weapons' ATTACKED) at weapons' statement, after the damage and before the rest of the phase, as in 3.1c. |
| economy, units, map, orders, weapons → ai | `Unit::flags`, `Unit::pos`, `Unit::zFraction`, `Unit::def`, `Unit::speed`, `Unit::unitDefIndex`, `Unit::lastAttackerSlot`, `Unit::lastDamageType`, `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` | units | The unit and its player, read by ai's synchronous REACT_TO_ATTACK and units' synchronous SET_SQUAD at the publisher's statement in a unit's update, after the writes before it in the phase, as in 3.1c. |

## 5. State and the trace

Ai keeps no private region: its state stays in the state block, where it was, and since #979 it is no longer
hashed. #979 moved out of the hash what only its owner reads, for los_radar, players, ai and map in one format bump.

**What left the hash, for #979.** State that no other subsystem reads inside the tick but through ai's queries, now
dumped as record 43 (`aiState`) and not hashed:

| Ai's state | Where | Read by | Written by |
|---|---|---|---|
| The per-player AI (`PlayerAI`): its plans, build weights and limits, feature cells, base positions, unit counts and weighted centre | `g_playerAI`; record 43 `aiState.players[i]` | ai | ai (players creates and frees it through ai's set-up slots) |
| The squad managers (`SquadManager`) and their ten timer slots | `Player::ai`; record 43 `aiState.players[i].squadManager` | ai | ai (players' set-up calls `CreateSquadManager` and `FreeSquadManager`) |
| The squads (`Group`, with their unit lists) | `Player::groups`; record 43 `aiState.players[i].squads[s]` | ai | ai |

**What stays hashed, and who reads it.** `Unit::group` stays in the unit record: orders writes it (`GetBuiltOrder`),
units loads it, and ai reads it (`SetUnitSquad`). The `Player::ai` and `Player::groups` references stay in the
`Player` record, read by players (set-up, teardown and the rebuild of the feature cells) and ai. `g_aiCommandsEnabled` is not hashed and never
was: only `aiState.commandsEnabled` dumps it, so a wrong value shows in the dump and in the orders it leads to.

**The record 43 report** (dumped, never hashed; `docs/trace-format.md`). Its first lines are the report the module
wrote before #979 (`commandsEnabled`, `count`, and per squad manager its player, countdown, next action and the next
firing tick of each timer). After them, per player with a `PlayerAI`, the `PlayerAI` fields (without `hasSpecial`,
which is los_radar's) and its nine raw vectors as a size and the bytes of the elements; per player with a squad
manager, the manager's fields and each of its timers with its class number and fields; per player with squads, each of
the ten squads with its fields and its unit list. A replacement ai writes what it needs to be judged by, and the
default's report is required equal to the reference's.

**The unit index is not ai's any more.** The visible, known and factory lists left `PlayerAI` (#991). los_radar holds,
per player, the visible enemy list (`visible`), the known list (`known`, enemy units with flag 0x100) and `hasSpecial`,
in a record it allocates in the sim block behind a tracked pointer (`src/map/line_of_sight_index.cpp`); units holds the
factory list (`src/units/factory_index.cpp`). Players' set-up creates and frees each beside ai's `PlayerAI`
(`InitPlayerSlot`, `FreePlayers`), under the same condition. Every 30 ticks, from its own `lastTick`, the scheduler's
players phase calls `los_radar.RefreshUnitIndex(player)` and then `units.RefreshFactoryIndex(player)` right before
ai's `UpdatePlayerAI`, each with its inbox delivered first. 3.1c filled the three lists in one walk of the units; the
walks are now separate, but each list holds the same units in the same order, and no draw moves. CLOAK's divergence
when queued (`docs/economy-interface.md`, section 8) is los_radar's list built from a cloak bit that changed late.
Ai keeps in `PlayerAI` the unit counts, `builders`, the weighted centre, its own `lastTick`, the `RandomInt(0x1e)` draw
and `ComputeBaseWeights`, all of `UpdatePlayerAI`.

**The unit index is reported by its owners.** Section 24 and 25 of the trace hash the visible, known and factory lists
and `hasSpecial` through los_radar's and units' `WriteState`, not through `PlayerAI` (#979). `PlayerAI`'s bytes 0x05 to
0x34 and 0x79 to 0x7c are unused in the moddable build.
`privatestate.py --check` fails when another subsystem writes any of the items ai alone writes: its types (`PlayerAI`,
`SquadManager`, the squad timers, `Group` and the AI's helper types) and `Player::groups`, `g_aiCommandsEnabled` and
`g_playerAI`. `Unit::group` is not listed: orders writes it (`GetBuiltOrder`) and units loads it. `Player::ai` is not
listed either: players writes it at set-up. `seamlink.py --pilot-seams` checks the seam: only the engine calls `Apply`
and the runs, and ai has no query.

**The trace dumps ai's events** in section 42 (`docs/trace-format.md`, section 8.30), after players' section 41, and
hashes none of them. Each record is an event with the outcome `Apply` gave, or an effect ai published (`outcome` 1, as
it is recorded before the publish), in ai's order. Ai's `WriteState` report (record 43, `aiState`) is dumped and not
hashed (see above). The section is new, so the trace format's schema changes and the gate of this change is
the full suite.

## 6. Random draws and sim events

Ai draws from the game logic stream in its runs (the timers and `UpdatePlayerAI`'s `RandomInt(0x1e)`, in the players
phase) and in REACT_TO_ATTACK (the squad manager's next action, `RandomInt(300)`, for a computer player's unit with the
reacting flag), which the engine's adapter of weapons' ATTACKED delivers from inside a unit's update (units phase) and
a projectile's (projectiles phase). It records the build queue sound of the build timer and, through its effects, what
units and orders record (an activation's speech, the cancelled orders' sounds). The "under attack" speech and the
notice are units' now (`UnitsOnWeaponsAttacked`, `UnitsOnWeaponsAttackedSpeech`), and the return fire's draws and sim
events are orders' and targeting's (the attack order's sounds, the retargeting's draws): `OrdersOnWeaponsAttacked` runs
between them in 3.1c's order, so each draw and record is where it was.

The class `ordered` of `data/dependencies.csv` has 27 rows with ai, `rng.parkMiller` or `sim-event` edges. Each is
kept in order by a synchronous route or by the scheduler's order of the runs:

| Writer → reader | Kind | Phase | How it is kept in order |
|---|---|---|---|
| ai → weapons, weapons → ai | `rng.parkMiller` | units | REACT_TO_ATTACK's draw, delivered from weapons' ATTACKED inside a unit's update between weapons' own draws, is drawn at weapons' statement because the route is synchronous: queued, it moved after an explosion's draws (section 8). |
| ai → weapons, ai → features, weapons → ai, features → ai | `rng.parkMiller` | projectiles | The same draw under a projectile's damage, between the projectile's draws and the burning features', at weapons' statement. |
| ai → targeting, targeting → ai | `rng.parkMiller` | players | Ai's timers draw in its runs, and targeting draws in the retargeting ai's synchronous SET_UNIT intents make, interleaved at ai's statements in the player's turn, as in 3.1c. |
| ai → weapons, ai → orders, ai → units, ai → players, ai → features; weapons, orders, units, features → ai | `sim-event` | units | The "under attack" speech and the return fire's records (units' and orders' handlers of weapons' ATTACKED) and the sounds and speeches of ai's synchronous effects (orders' and units' handlers), recorded at their publishers' statements in the unit's update, between the sim events the others record before and after them. |
| ai → weapons, ai → features; weapons, features, orders → ai | `sim-event` | projectiles | The same records under a projectile's damage, at weapons' statement. |
| ai → features, features → ai | `sim-event` | features | The records of a unit damaged by a burning feature, at the feature's statement. |
| ai → los_radar, los_radar → ai, units → ai | `sim-event` | players | Ai's build queue sound and the speeches of its synchronous ACTIVATE (units' handler), and los_radar's radar records, in the order the scheduler calls ai's runs and the line of sight pass in each player's turn. ACTIVATE queued moved a speech after the build queue sound (section 8). |

## 7. What stays inside the default

3.1c's types and limits are the default's, not the interface's: the per-player AI and its script format, the squad
manager and its six timers, the ten squads and the squad numbers the timers use (scouts, builders, the air and sea
squads), the build weights' scale and the sticky flags, the plan's difficulties as bits. An event carries a unit as
its block offset and a player as its index. The interface names no 3.1c plan, weight or squad type: a replacement AI
needs only the views, the queries and the events, as the demo shows.

**The demo module.** `AiIdleSeam` (`src/ai/ai_idle.cpp`) is an AI that gives no orders at all. Its `UpdatePlayerAI` and
`UpdateSquads` do nothing: the visible and factory lists no longer depend on ai's run, because los_radar and units
refresh them (section 4), so every player's guard, weapon search and pad search work as with the standard ai. Its
`Apply` rejects REACT_TO_ATTACK for a unit of a computer player (outcome 0) and passes everything else to the default
(SET_SQUAD, PLAN, WEIGHT and LIMIT), so the squads and the AI commands stay consistent for the trace. It keeps the
default's set-up, save and load. A human player's units still return fire and their shooters still get the wake bits:
those are no longer ai's at all, so the demo leaves them as the standard ai does. Unlike a run with the standard ai, it
makes no `RandomInt(0x1e)` draws (the draw of `UpdatePlayerAI`), so the draws of the game logic stream after the first
30 ticks differ from a run with it; an idle run never conformed to the reference anyway. The scenario key `ai idle`
installs it (`docs/scenario-format.md`), and `tests/ai/idle-skirmish.txt` plays it to its end: a human Arm army against
a computer Core player that never moves, 3000 ticks. `los_radar_writer_test` checks that with it every player's visible
and factory lists still refresh every 30 ticks and no squad runs. It is counted apart in coverage (`demo:` rows of
`data/coverage.csv`), like the HPA* pathfinder.

**Left for later.** Ai's private region and its leaving the hash are #979's, with the trace format bump (section 5).

## 8. What the gate decided

Every route into and out of ai was queued first, each on its own, with the others synchronous (a build that read the
policy of each route from the environment, not merged). The tier judged each build, and a route whose queueing
diverged became synchronous. A route whose traces stayed equal but whose event digests differed (the sim events'
order, `reference/events`) diverged too. The divergences are against the reference, with the first tick, phase and
field. With every route queued at once except the engine's, the tier diverged first in orders-air-fights, tick 709,
world phase: `units[2].list[0].subFlags` (reference 0x18, queued 0x08), as NOTIFY_REFS does alone; with every route
synchronous, it was equal.

| Route queued | Tier | First divergence |
|---|---|---|
| weapons REACT_TO_ATTACK | diverged | eight-full-moon, tick 8081, units phase: `debris[16].vel.x` (reference 0xffff8560, queued 0xffffdb84), then 86 fields, the pieces of an explosion and `players[2].squadManager.nextAction` (reference 0x2086, queued 0x1ff8). The squad manager's draw (`RandomInt(300)`) moved after the explosion's. |
| weapons DAMAGE_SPLIT | equal | none (synchronous by #952's rule, section 4) |
| units SET_SQUAD | equal | none (synchronous by #952's rule, section 4) |
| engine SET_SQUAD | equal | none (queued, section 4) |
| engine PLAN, WEIGHT, LIMIT | equal | none (queued, section 4) |
| ai ADD_ORDER | diverged | mission-capture, tick 90, world phase: `units[262].list[0].kind` (reference 0x19, queued 0x22), then 19 fields. Ai read the unit's orders before the order was given, and gave it a second one (two ai records where the reference had one). |
| ai QUEUE_BUILD | equal | none (synchronous by #952's rule, section 4) |
| ai ATTACK | diverged in the event digests | eight-full-moon, tick 8080, projectiles phase: an "under attack" speech of unit 1251 that the reference does not record; also mission-radius, tick 1323, and orders-guard-attacked, tick 66. `ReactToAttack` reads the unit's order (`GetOrderFlags`) after it orders the attack; queued, it read the old order. The traces were equal. |
| ai STOP | diverged | eight-full-moon, tick 8080, projectiles phase: `units[501].head` (reference an order, queued none), then 79 fields, among them `units[1251].list`. |
| ai HIT | equal | none (synchronous by #952's rule, section 4) |
| ai NOTIFY_REFS | diverged | mission-kill-of-type, tick 182, world phase: `units[260].list[0].subFlags` (reference 0x18, queued 0x08), 1 field. The order that refers to the attacked unit had already run when the event reached it. |
| ai ACTIVATE | diverged in the event digests | mission-kill-of-type, tick 30, players phase: the activated unit's speech (`unit_speech`, unit 279) recorded after the build queue sound instead of before it. The traces were equal. |
| ai SET_STATES | equal | none (synchronous by #952's rule, section 4) |

With every route as above, the tier was equal (38 of 38 scenarios), and so was the full suite in both modes, the gate
of this change (178 of 178 scenarios equal; sound, UI logs and event digests 89 of 89, frames 6 of 6 and saves 1 of 1
in both modes).

**#991 replaced five of these routes.** #991 moved REACT_TO_ATTACK's every-player part, DAMAGE_SPLIT and the effects ATTACK,
NOTIFY_REFS and HIT out of ai (sections 2 and 3). The routes the table names for them are now weapons' routes: the
effect ATTACKED to units' notice, the engine's adapter of ai's REACT_TO_ATTACK, orders' return fire and units' speech, and the effect HIT to
orders, all synchronous, as the ai routes were. The tier gate with both module modes plays them equal: `76 of 76 scenarios equal`, sound, UI, event, frame and save logs equal in each mode.

**How often each route is taken.** A build that counts every publish into and out of ai, over the 89 scenarios with
native modules (all 89 equal to the reference): the events published inside the tick and outside it (when a game is
set up or loaded, the AI scripts and the console's commands), and in how many scenarios each was published at all.

| Route | Inside the tick | Scenarios | Outside the tick | Scenarios |
|---|---|---|---|---|
| weapons REACT_TO_ATTACK | 7,355 | 37 | 4 | 3 |
| weapons DAMAGE_SPLIT | 8,154 | 25 | 0 | 0 |
| units SET_SQUAD | 2,929 | 71 | 1,432 | 89 |
| engine SET_SQUAD | 0 | 0 | 3 | 1 |
| engine PLAN | 0 | 0 | 270 | 89 |
| engine WEIGHT | 0 | 0 | 8,818 | 89 |
| engine LIMIT | 0 | 0 | 73,768 | 89 |
| ai ADD_ORDER | 13,339 | 60 | 0 | 0 |
| ai QUEUE_BUILD | 477 | 18 | 0 | 0 |
| ai ATTACK | 881 | 7 | 0 | 0 |
| ai STOP | 1,001 | 7 | 2 | 2 |
| ai HIT | 8,154 | 25 | 0 | 0 |
| ai NOTIFY_REFS | 7,355 | 37 | 4 | 3 |
| ai ACTIVATE | 2,255 | 8 | 0 | 0 |
| ai SET_STATES | 801,234 | 73 | 0 | 0 |

The engine's routes are taken only outside the tick, so their queueing changes nothing: each is delivered at once
(#970). SET_STATES is published by the squad manager's `AssignSquads` for each unit of its player, whether or not the
states change, as 3.1c wrote them.

**Inbox depths.** Ai's inbox was never deeper than 0, at a publish or at a phase boundary, in any scenario: every
route published inside the tick is synchronous, and the engine's are published only outside it. The most ai events
and effects published inside one unit's update (`UpdateUnit`) was 107, in edges-economy-targeting at tick 244
(unit 255).
