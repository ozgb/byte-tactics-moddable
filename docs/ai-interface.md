# The ai interface

Ai sits behind `interfaces/ai.h` (version 4.0), the seventh module on the scheduler after economy
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
210, ai 224 to 229 and 240 to 247 (its range is 224 to 255, effects from 240).

Ai's place in it:
- **Events into ai:** REACT_TO_ATTACK and DAMAGE_SPLIT, published by weapons; SET_SQUAD, published by units and the
  engine (the squad keys and the test harness); PLAN, WEIGHT and LIMIT, published by the engine from the AI scripts and
  the console. Ai's handler (`AiInbox`, `src/seams/ai_world.cpp`) passes each to `Apply`, and records it with its
  outcome in the trace's dump (section 5).
- **Routes:** of the seven routes into ai, three are synchronous: weapons' REACT_TO_ATTACK because the gate diverged
  with it queued, weapons' DAMAGE_SPLIT and units' SET_SQUAD by #952's rule. The engine's four (SET_SQUAD, PLAN,
  WEIGHT, LIMIT) are queued: the engine publishes them only outside the tick, where #970's rule delivers them at once.
  Of ai's eight effects, ADD_ORDER, ATTACK, STOP, NOTIFY_REFS and ACTIVATE are synchronous because the gate diverged
  with each queued, QUEUE_BUILD, HIT and SET_STATES by #952's rule (sections 4 and 8).
- **Runs:** `UpdateSquads` (3.1c's `SquadManager::TickIfActive`, behind the slot `SquadManagerTickIfActive` of ai 3.1)
  and `UpdatePlayerAI`, called by the scheduler's players phase (`SchedPlayersPhase`, `src/game/scheduler.cpp`) in each
  player's turn, right after players' `UpdatePlayerHead` and before the line of sight pass, where #967 put them. The
  scheduler delivers ai's inbox before each of the two runs. `UpdateSquads` takes a player's index; it does nothing for
  a player without a squad manager, as 3.1c's caller checked.
- **Synchronous calls out:** none. Ai calls no slot of another module that writes: every write it made in 3.1c is an
  effect (section 3) or an intent to targeting (#955, kept).
- **Queries:** orders' `GetOrderType` and `GetOrderFlags`, weapons' `WeaponCanReachUnit` and `WeaponCanReachPos`,
  economy's `GetEnergyUse`, units' and map's lookups, and targeting's `FindWeaponTarget` split (#955), all as of the
  owner's last apply.
- **Views:** the units' fields ai reads (position, flags, orders, health, type, owner), the `Player` records with
  economy's incomes and usages, map's metal sums and the line of sight tables (section 4). Ai's own state is read by no
  other simulation module except through its five queries (section 4).
- **Effects out of ai:** ADD_ORDER, QUEUE_BUILD, ATTACK, STOP and HIT, to orders; NOTIFY_REFS, ACTIVATE and
  SET_STATES, to units. Neither is on the model yet, so their handlers (`src/orders/ai_effects.cpp`,
  `src/units/ai_effects.cpp`) are adapters that call today's seam slots or make 3.1c's write. Ai records each effect in
  the trace's dump before it publishes it, as `docs/module-abi.md`, 9.3, asks of a module's effects. Its targets
  already were effects: SET_UNIT and CLEAR to targeting, synchronous, and SET_POS, queued (#955, #975;
  `docs/targeting-interface.md`, section 4). They are unchanged.
- **Module ids:** ai keeps `SCHED_AI = 10`, which #955 added for its targeting outbox.

## 2. Events into ai

| 3.1c site | 3.1c call | Now |
|---|---|---|
| weapons: a unit takes damage (`ApplyUnitDamage`) | `ReactToAttack` | REACT_TO_ATTACK from weapons |
| weapons: a shot hits (`ApplyWeaponHit`, `DetonateProjectile`, `ApplyAreaDamage`) | `SquadManager::MarkOwnerNetDirtyFromDamageSplit`, on the shooter's owner's squad manager | DAMAGE_SPLIT from weapons, naming the shooter |
| units: a unit made (`InitUnitFromType`) | `SetUnitSquad`, squad 0 | SET_SQUAD from units |
| units: a unit dies (`ApplyUnitDeath`) | `SetUnitSquad`, squad -1 | SET_SQUAD from units |
| units: a saved unit loaded (`LoadUnit`) | `SetUnitSquad`, the saved squad | SET_SQUAD from units, outside the tick |
| the squad keys (`CreateSquad`, `src/ingame/selection.cpp`) | `SetUnitSquad`, a direct call into ai's file | SET_SQUAD from the engine, outside the tick |
| the test harness: the quirk reach check puts a unit in the scout squad (`ReachQuirkGuard`) | `SetUnitSquad` | SET_SQUAD from the engine, outside the tick |
| the AI scripts and the console: `plan`, `weight`, `limit` (`CmdPlan`, `CmdWeight`, `CmdLimit`) | the slots `CmdPlan`, `CmdWeight`, `CmdLimit` | PLAN, WEIGHT, LIMIT from the engine, outside the tick |

**What each event writes.**
- **REACT_TO_ATTACK** serves the units of every player, human players' included, not only the computer players'.
  For any attacked unit it tells the unit's references of the attack (NOTIFY_REFS 16) and records the unit's "under
  attack" speech (a sim event) unless its order says not to. For a unit of a human or a computer player (player
  type 1 or 2) that can fight back and is not allied to the attacker, it orders an attack on the attacker when the unit is idle
  and its weapon reaches (ATTACK), else retargets its weapons (targeting's SET_UNIT): this is how a human player's
  idle units return fire. Only for a unit of a computer player with the reacting flag does it draw the squad manager's
  next action (`RandomInt(300)`, section 6) and stop the unit (STOP). It writes the squad manager's `nextAction`; the
  rest are effects. A replacement AI that drops this event takes return fire and the reference notices away from
  human players too.
- **DAMAGE_SPLIT** publishes HIT, for a shooter of any player: whether the shot hit mostly units of other players
  (more than twice the damage dealt to its own player's), the wake bits its orders read. It writes nothing of ai's.
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
one: 1 for REACT_TO_ATTACK, DAMAGE_SPLIT, SET_SQUAD and PLAN; for WEIGHT and LIMIT, 1, or 0 when the AI commands are
off. `Apply` first rejects, with outcome 0 and no change, an event naming no unit (REACT_TO_ATTACK, DAMAGE_SPLIT,
SET_SQUAD), a SET_SQUAD outside -1 to 9 or for a unit whose player has no squads, a PLAN with a bit above 3, and an
unknown kind. 3.1c's `SetUnitSquad` had no such checks and wrote outside the squads; every 3.1c caller passed a valid
squad, so no trace depends on it.

**Every slot of 3.1.** The 24 slots of ai 3.1, and what each became in 4.0:

| 3.1 slot | 4.0 |
|---|---|
| `ReactToAttack`, `MarkOwnerNetDirtyFromDamageSplit`, `SetUnitSquad`, `CmdPlan`, `CmdWeight`, `CmdLimit` | events: REACT_TO_ATTACK, DAMAGE_SPLIT, SET_SQUAD, PLAN, WEIGHT, LIMIT, through the new slot `Apply` |
| `SquadManagerTickIfActive`, `UpdatePlayerAI` | runs, called by the scheduler: `UpdateSquads` (by a player's index, not a `SquadManager*`) and `UpdatePlayerAI` |
| `SquadManagerDeleteTimers` | `FreeSquadManager`, the counterpart of `CreateSquadManager`: it deletes the timers and the squad manager, which players' `FreePlayers` did with ai's type |
| `GetVisibleEnemiesInRadius`, `GetVisibleEnemiesInRadiusList`, `GetFactoriesInRadius`, `HasReadyUnitInRange`, `IsPointVisible` | queries, unchanged (section 4) |
| `CreatePlayerAI`, `DestroyPlayerAI`, `CreateSquads`, `FreeSquads`, `CreateSquadManager`, `ResetAIPlayers`, `RebuildFeatureCells`, `ParseDownloadableAiWeightScripts`, `ReparseAiWeightScriptsIfLimitNotSticky`, `DumpPlayerAI` | set-up, load and debugging slots, outside the tick |
| (new) `WriteState` | ai's report for the trace's dump (section 5) |

**Calls outside the interface.** `docs/schedule.md` listed "a call into ai outside its interface" twice, in orders'
`TeleportOrder`. Neither was a call into the AI: `TeleportOrder` builds a `Vec`, a 3-vector type whose header lived in
ai's folder (`src/ai/vec.h`), so the file rule of `tools/coupling.py` put its inline operators in ai. The header moved
to `src/util/vec.h`, where the other maths helpers are, and the rows went. The direct calls into ai were the engine's
and the other subsystems' calls to the slots above and to `SetUnitSquad`; each is now an event.

## 3. Effects out of ai

Every write ai made into another subsystem's state is an effect, delivered to an adapter of the subscriber:

| Effect | 3.1c call | Subscriber, handler | What it does |
|---|---|---|---|
| ADD_ORDER | `orders.AddOrder` (timers, squads' `Send`) | orders, `OrdersOnAiAddOrder` | gives the unit an order of the type ai chose with `GetOrderType`, against a unit or a position, replacing its orders or queued behind them |
| QUEUE_BUILD | `orders.QueueBuildOrder` (the build timer) | orders, `OrdersOnAiQueueBuild` | queues units to build at a factory, by type index; the adapter passes the type's name, as 3.1c did |
| ATTACK | `orders.IssueAttackOrder` (`ReactToAttack`) | orders, `OrdersOnAiAttack` | orders the unit to attack the attacker |
| STOP | `orders.DeleteOrders` (`ReactToAttack`) | orders, `OrdersOnAiStop` | deletes the unit's orders |
| HIT | a bit field of the shooter's `Unit::netDirtyFlags` (`MarkOwnerNetDirtyFromDamageSplit`) | orders, `OrdersOnAiHit` | sets bit 0x4000 (mostly enemies) or 0x2000 of the flags orders' `RunOrders` consumes |
| NOTIFY_REFS | `units.NotifyUnitRefs` (`ReactToAttack`) | units, `UnitsOnAiNotifyRefs` | tells the orders that refer to the unit that it was attacked (event 16) |
| ACTIVATE | `units.SetStateBits` (metal makers, `UpdateConverter`, the build timer) | units, `UnitsOnAiActivate` | switches the unit on or off |
| SET_STATES | a direct write of `Unit::flags` (`SquadManager::AssignSquads`) | units, `UnitsOnAiSetStates` | sets the unit's movement and firing states (bits 18 to 21 of `Unit::flags`) |

The netDirty bits of HIT have no owner but the unit's orders: orders' `RunOrders` (`src/orders/order_list.cpp`)
consumes them as order events, and units clears them when a unit is made. So HIT is an effect to orders, and the
adapter writes the bits as 3.1c's view did.

**`IssueAttackOrder` is split.** 3.1c's `ReactToAttack` used its answer in the same statement: it retargets the weapons
only when no attack was ordered. `IssueAttackOrder` refuses when the unit is its target, when its movement or firing
state is 0 (hold position, hold fire), or when orders' `GetOrderType` names no order against the target, and otherwise
always gives the order. Ai now makes those checks itself (`AttackOrdered`, `src/ai/ai_player.cpp`), from the unit's
flags it reads as a view and from `GetOrderType`, a query that writes nothing (it is called again by the adapter, with
the same answer), and publishes ATTACK only when they pass. So the answer is ai's, and ATTACK is a plain effect. Its
route is synchronous all the same, because the gate diverged with it queued (section 8): the end of `ReactToAttack`
reads the unit's order (`GetOrderFlags`) that ATTACK writes.

**`FindWeaponTarget` keeps #955's split.** Ai's call of targeting's `FindWeaponTarget` stays the query and intent pair
#955 made, and its SET_UNIT and CLEAR to targeting stay synchronous, its SET_POS queued.

**Writes ai keeps.** After this change ai writes no field of another subsystem: `tools/rwsets.py` gives it no write of
`Unit` other than `Unit::group` (the squad, ai's own field of the unit), and none of `Order` or `UnitMotion`. It writes
its own: the `PlayerAI` of each player, the squad managers and their timers, the squads, `g_aiCommandsEnabled`.

## 4. Views

**What ai reads:** the units' position, flags, type, owner, health, build progress, speed, weapons, last attacker and
damage type, and their orders (`Unit::list`, `Order::flags_42`), written by units, orders and weapons; the `Player`
records, among them economy's `energyIncome`, `energyUsage`, `metalIncome` and `metalUsage` (`GetBuildRating`,
`UpdateConverter`, `BuildTimer::OnTimer`), which players 3.0 no longer serves through getters; map's metal sums
(`g_lastPlaceMetalSum`); the line of sight tables (`IsPointVisible`). All as of each module's last run.

**What others read of ai:** nothing of its state but through the queries. `tools/rwsets.py` has no read of `PlayerAI`,
`SquadManager`, the squad timers, `Group` or `g_aiCommandsEnabled` outside ai, the trace's harness and players'
set-up of a player slot (`SetupPlayerSlot` and `ResetPlayerSlots`, which write `Player::ai`, and `RebuildAIFeatureCells`, which reads it to call ai's slot). `Unit::group` is read by
orders' `GetBuiltOrder`, which copies a factory's squad to the unit it built for a human player, and is written there
too (3.1c's direct write, kept).

**The queries.**

| Query | Reads | Callers inside the tick |
|---|---|---|
| `GetVisibleEnemiesInRadius`, `GetVisibleEnemiesInRadiusList` | ai's private state: the player's list of visible enemy units (`PlayerAI::visible`), and the known units (`known`) when the AI has the special flag; then the units' positions and flags | orders (`GuardNoMoveOrder`, `WaitOrder`) and targeting's weapon search (`FindWeaponTarget`); `GetVisibleEnemiesInRadiusList` has no caller in 3.1c's code (a kit slot) |
| `GetFactoriesInRadius` | ai's private state: the player's factory list (`PlayerAI::factories`), then the factories' type and `activateFlags` | orders (the VTOL orders' pad search: `FindPads`, `Land`, `AirStrikeOrder`, ...) |
| `HasReadyUnitInRange` | ai's private state: the visible list, then the units' flags | the info panel (`UpdateSensorRadarAndCloak`), presentation only |
| `IsPointVisible` | only los_radar's views: the player's explored size and the line of sight mask (`Game::visibilityMask`); nothing of ai | weapons' drawing (`DrawProjectiles`) and players' `IsVisible` |

`IsPointVisible` reads no ai state, and none of its callers is ai: it belongs with los_radar's queries. #991 moves it,
in the same bump of los_radar and ai. The four others read the visible
and factory lists, which only ai writes (`UpdatePlayerAI`), so they are ai's queries, answered as of ai's last run.

**The routes.** Every route, with its reason:

| Route | Now | Why |
|---|---|---|
| weapons REACT_TO_ATTACK | synchronous | The gate (section 8). |
| weapons DAMAGE_SPLIT | synchronous | #952's rule. Queued it stayed equal on the tier. Its HIT sets bits 0x2000 or 0x4000 of the shooter's `Unit::netDirtyFlags` in the projectiles phase, and targeting's `SetWeaponTargetUnit` and `SetWeaponTargetPos` clear bits 10 to 14 of the same word (`&= 0x83ff`) later in the same phase, when ai or the unit's weapons retarget it: the `immediate` row orders → targeting, `Unit::netDirtyFlags`, projectiles (ai → targeting before HIT was an effect; mission-radius at tick 1355). Pending, the bit would be set after the clear instead of before it. |
| units SET_SQUAD | synchronous | #952's rule. Queued it stayed equal on the tier. Orders' `GetBuiltOrder` writes `Unit::group` of a human player's unit when it is finished, in the units phase, and units' SET_SQUAD 0 of the same unit, published when it is made: a unit made and finished in one units phase would get its squad 0 after the factory's, instead of before. A dying unit's SET_SQUAD -1 also leaves its squad's list holding it until the boundary. |
| engine SET_SQUAD | queued | Published only outside the tick: the squad keys (input, between ticks) and the test harness's quirk reach check (before the tick), where #970's rule delivers it at once. |
| engine PLAN, WEIGHT, LIMIT | queued | Published only outside the tick: the AI scripts at a match's set-up and the console, where #970's rule delivers them at once (section 2). |
| ai ADD_ORDER, to orders | synchronous | The gate (section 8). |
| ai QUEUE_BUILD, to orders | synchronous | #952's rule. Queued it stayed equal on the tier. It writes the factory's orders (`Unit::list`), which ai's timers read later in the same player's turn (`!u->list`, the build and scout timers): the read of ADD_ORDER's divergence. Ai's inbox delivery before a run does not deliver orders' inbox, so the order could be pending at that read. |
| ai ATTACK, to orders | synchronous | The gate (section 8). |
| ai STOP, to orders | synchronous | The gate (section 8). |
| ai HIT, to orders | synchronous | #952's rule, as DAMAGE_SPLIT: the bits it sets are cleared by targeting later in the projectiles phase. |
| ai NOTIFY_REFS, to units | synchronous | The gate (section 8). |
| ai ACTIVATE, to units | synchronous | The gate (section 8). |
| ai SET_STATES, to units | synchronous | #952's rule. Queued it stayed equal on the tier. It writes the movement and firing states (`Unit::flags`) in the squads' tick, and in the same player's turn ai's `UpdatePlayerAI`, the line of sight pass and targeting read them: the `immediate` rows units → los_radar and units → targeting, `Unit::flags`, players (ai → los_radar and ai → targeting before SET_STATES was an effect). |

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv`, regenerated on this change, has 41 rows of
class `immediate` with ai. In every one ai is the reader: ai writes nothing another module reads in the same phase,
because each of its writes into others is now an effect, applied by the subscriber's handler. The rows that had ai as
writer before this change (ai → los_radar and ai → targeting, `Unit::flags`, players; ai → targeting,
`Unit::netDirtyFlags`, projectiles) now have the handler's module as writer: units → los_radar and units → targeting,
`Unit::flags`, players (SET_STATES), and orders → targeting, `Unit::netDirtyFlags`, projectiles (HIT), at the same
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
| weapons, orders → ai | `Unit::flags`, `Unit::f_1f`, `Unit::f_57`, `Unit::lastAttackerSlot`, `Unit::lastDamageType`, `Order::flags_42`, `Unit::list` | projectiles | The damaged unit, its weapons and its orders, read by ai's synchronous REACT_TO_ATTACK at weapons' statement, after the damage and before the rest of the phase, as in 3.1c. |
| economy, units, map, orders, weapons → ai | `Unit::flags`, `Unit::pos`, `Unit::zFraction`, `Unit::def`, `Unit::speed`, `Unit::unitDefIndex`, `Unit::lastAttackerSlot`, `Unit::lastDamageType`, `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` | units | The unit and its player, read by ai's synchronous REACT_TO_ATTACK and units' synchronous SET_SQUAD at the publisher's statement in a unit's update, after the writes before it in the phase, as in 3.1c. |

## 5. State and the trace

Ai's state stays where it is and stays hashed in this change (the human, 2026-11-16, on PR #978); #979 moves out of
the hash what only its owner reads, for los_radar, players, ai and map in one format bump.

**What only ai reads, for #979.** Hashed state that no other subsystem reads inside the tick but through ai's queries:

| Ai's state | Where | Read by | Written by |
|---|---|---|---|
| The per-player AI (`PlayerAI`): its plans, build weights and limits, feature cells, base positions, and the visible, known and factory lists | `g_playerAI`, hashed as `players[i].ai` (`VisitPlayerAI`) | ai; orders through `GetVisibleEnemiesInRadius` and `GetFactoriesInRadius` | ai (players creates and frees it through ai's set-up slots) |
| The squad managers (`SquadManager`) and their six timers | `Player::ai`, hashed as `players[i].squadManager` | ai | ai (players' set-up calls `CreateSquadManager` and `FreeSquadManager`) |
| The squads (`Group`, with their unit lists) | `Player::groups`, hashed as `players[i].squads` | ai | ai |
| `g_aiCommandsEnabled` | the state block, hashed | ai | ai |

`players[i].ai.visible`, where economy's CLOAK diverges when queued (`docs/economy-interface.md`, section 8), is read
by no other module: its readers outside ai are the queries `GetVisibleEnemiesInRadius`, `GetVisibleEnemiesInRadiusList`
and `HasReadyUnitInRange`, which ai answers, and the trace. CLOAK's divergence there is ai's list built from a
cloak bit that changed late (los_radar's sensors read the bit, and ai's refresh reads what they decide), not another
module reading ai's list.

`privatestate.py --check` fails when another subsystem writes any of the items ai alone writes: its types (`PlayerAI`,
`SquadManager`, the squad timers, `Group` and the AI's helper types) and `Player::groups`, `g_aiCommandsEnabled` and
`g_playerAI`. `Unit::group` is not listed: orders writes it (`GetBuiltOrder`) and units loads it. `Player::ai` is not
listed either: players writes it at set-up. `seamlink.py --pilot-seams` checks the seam: only the engine calls `Apply`
and the runs, and the five queries write no state.

**The trace dumps ai's events** in section 42 (`docs/trace-format.md`, section 8.30), after players' section 41, and
hashes none of them. Each record is an event with the outcome `Apply` gave, or an effect ai published (`outcome` 1, as
it is recorded before the publish), in ai's order. Ai's `WriteState` report (record 43, `aiState`) is dumped and not
hashed: whether the AI commands are on, and per squad manager its player, countdown, next action and the
next firing tick of each timer. The section is new, so the trace format's schema changes and the gate of this change is
the full suite.

## 6. Random draws and sim events

Ai draws from the game logic stream in its runs (the timers, in the players phase) and in REACT_TO_ATTACK (the squad
manager's next action, `RandomInt(300)`), which weapons publishes from inside a unit's update (units phase) and a
projectile's (projectiles phase). It records sim events: the "under attack" speech of REACT_TO_ATTACK, the build queue
sound of the build timer, and, through its effects, what units and orders record (an activation's speech, the
cancelled orders' sounds).

The class `ordered` of `data/dependencies.csv` has 27 rows with ai, `rng.parkMiller` or `sim-event` edges. Each is
kept in order by a synchronous route or by the scheduler's order of the runs:

| Writer → reader | Kind | Phase | How it is kept in order |
|---|---|---|---|
| ai → weapons, weapons → ai | `rng.parkMiller` | units | REACT_TO_ATTACK's draw, published by weapons from inside a unit's update between weapons' own draws, is drawn at weapons' statement because the route is synchronous: queued, it moved after an explosion's draws (section 8). |
| ai → weapons, ai → features, weapons → ai, features → ai | `rng.parkMiller` | projectiles | The same draw under a projectile's damage, between the projectile's draws and the burning features', at weapons' statement. |
| ai → targeting, targeting → ai | `rng.parkMiller` | players | Ai's timers draw in its runs, and targeting draws in the retargeting ai's synchronous SET_UNIT intents make, interleaved at ai's statements in the player's turn, as in 3.1c. |
| ai → weapons, ai → orders, ai → units, ai → players, ai → features; weapons, orders, units, features → ai | `sim-event` | units | The "under attack" speech of REACT_TO_ATTACK and the sounds and speeches of ai's synchronous effects (orders' and units' handlers), recorded at their publishers' statements in the unit's update, between the sim events the others record before and after them. |
| ai → weapons, ai → features; weapons, features, orders → ai | `sim-event` | projectiles | The same records under a projectile's damage, at weapons' statement. |
| ai → features, features → ai | `sim-event` | features | The records of a unit damaged by a burning feature, at the feature's statement. |
| ai → los_radar, los_radar → ai, units → ai | `sim-event` | players | Ai's build queue sound and the speeches of its synchronous ACTIVATE (units' handler), and los_radar's radar records, in the order the scheduler calls ai's runs and the line of sight pass in each player's turn. ACTIVATE queued moved a speech after the build queue sound (section 8). |

## 7. What stays inside the default

3.1c's types and limits are the default's, not the interface's: the per-player AI and its script format, the squad
manager and its six timers, the ten squads and the squad numbers the timers use (scouts, builders, the air and sea
squads), the build weights' scale and the sticky flags, the plan's difficulties as bits. An event carries a unit as
its block offset and a player as its index. The interface names no 3.1c plan, weight or squad type: a replacement AI
needs only the views, the queries and the events, as the demo shows.

**The demo module.** `AiIdleSeam` (`src/ai/ai_idle.cpp`) is an AI that gives no orders at all. Its `UpdateSquads`
does nothing. It keeps the default `UpdatePlayerAI`, because that run refreshes every player's visible-enemy and
factory lists every 30 ticks, and the queries of section 4 answer from them for every player's guard, wait, weapon
search and pad search: an index, not a decision (the refresh's base-weight draw stays in the stream too).
`ai_writer_test` checks that the demo runs it for every player. Its `Apply` rejects REACT_TO_ATTACK for a unit of a
computer player and DAMAGE_SPLIT for a shooter of a computer player (outcome 0), and passes the rest to the default, so the squads and the AI commands stay consistent for the trace.
A human player's units still return fire and their shooters still get HIT's wake bits: only the computer player is
idle. The scenario key `ai idle` installs it
(`docs/scenario-format.md`), and `tests/ai/idle-skirmish.txt` plays it to its end: a human Arm army against a
computer Core player that never moves, 3000 ticks. It is counted apart in coverage (`demo:` rows of
`data/coverage.csv`), like the HPA* pathfinder.

**Left for later.** `IsPointVisible`'s move to los_radar (section 4) is #991's, with the visible and factory lists,
their queries and REACT_TO_ATTACK's every-player part. Ai's private region and its leaving the hash are #979's.

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
