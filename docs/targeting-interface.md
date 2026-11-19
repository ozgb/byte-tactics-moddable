# The targeting interface

Targeting sits behind `interfaces/targeting.h` (version 2.1, which adds LOAD), the third module on the scheduler
after economy (`docs/economy-interface.md`) and features (`docs/features-interface.md`), and the first whose state
stays in the `Unit` record (#955). 3.1c's targeting code is its default implementation. This page is the inventory
the interface was cut from:
- what other subsystems did to targeting's state in 3.1c, and what each became;
- what targeting did to others, and what each became;
- what targeting reads, and what others read of it;
- how the order of random draws is kept;
- what the trace hashes, and what it only dumps.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, events published
outside the tick delivered at once (#970), and events as plain data. This issue changes none of its rules. Event
kinds are unique across interfaces: economy uses 0 to 4 and 16 to 18, features 32 to 35 and 48 to 51, targeting 96 to
100 and 112.

Targeting's place in it:
- **Events into targeting:** SET_UNIT, SET_POS, CLEAR, RESET and LOAD, published by orders, ai, weapons and units.
  Targeting's handler (`TargetingInbox`, `src/seams/targeting_world.cpp`) passes each to `Apply`.
- **Routes:** Of the ten routes, the nine into targeting and the effect CLEARED out of it, three are queued: units'
  RESET and LOAD and ai's SET_POS (#975, judged by `ai-antinuke-intercept`). The other seven are synchronous:
  three because the gate diverged with them queued (orders' SET_UNIT, weapons' CLEAR and CLEARED) and four by #952's
  rule (orders' SET_POS and CLEAR, ai's SET_UNIT and CLEAR) (sections 4 and 8).
- **Synchronous slots:** `GetWeaponTargetPos`, `FindWeaponTarget` and `FindBestTargetIfFireAtWill`, called through
  the services `ScheduleTargetingTargetPos`, `ScheduleTargetingFind` and `ScheduleTargetingFireAtWill`, which
  deliver targeting's inbox first (section 2).
- **Queries:** `GetWeaponTargetUnit`, called through `ScheduleTargetingTargetUnit`, which delivers targeting's inbox
  first, and `ChooseWeapon`, which reads only the weapon slots' flags and is called directly (section 4).
- **Effects out of targeting:** CLEARED, published to targeting's outbox (`SCHED_TARGETING`) when a weapon that had a
  target loses it. Units subscribes to it; its handler, `UnitsOnTargetingCleared` (`src/units/targeting_effects.cpp`),
  is an adapter onto scripts' `FindScript` and `StartScriptWithArgs` slots, as 3.1c called them.
- **Runs:** none. Targeting has no phase of its own and no loop the scheduler holds: it runs only when an event is
  applied or a slot is called, inside the runs of orders, ai, weapons and units.
- **A new outbox:** ai had none. Its SET_UNIT, SET_POS and CLEAR are published from `SCHED_AI`, which services 1.14 adds
  after `SCHED_TARGETING`, so `SCHED_MODULE_COUNT` is 11. Ai has no inbox: nothing routes to it.

## 2. Events into targeting

| 3.1c site | 3.1c call | Now |
|---|---|---|
| orders: attacking, guarding, following, suppressing, the air orders (`unit_orders.cpp`, `vtol_orders.cpp`) | `SetWeaponTargetUnit`, `SetWeaponTargetPos` | SET_UNIT, SET_POS from orders |
| orders: stopping, paralysis, standing fire, an air strike, the order wake that drops every target (`RunOrders`) | `ClearWeaponTarget` | CLEAR from orders |
| ai: a unit that is hit turns on its attacker (`ReactToAttack`) | `SetWeaponTargetUnit` | SET_UNIT from ai |
| ai: the squad manager's retargeting (`RetargetWeapon`) | `SetWeaponTargetPos` (an interceptor at a projectile), `SetWeaponTargetUnit`, `ClearWeaponTarget` | SET_POS, SET_UNIT, CLEAR from ai |
| weapons: a weapon claimed or released (`Unit::ClaimWeapons`, `Unit::ReleaseWeapons`) | the code of `ClearWeaponTarget`, inline | CLEAR from weapons |
| units: a unit set up (`InitUnitFromType`) | `ResetWeaponTarget` | RESET from units |
| units: a saved game loads (`LoadUnit`, `unit_save.cpp`) | the pair's four bytes written from the save | LOAD from units, outside the tick (2.1, #1010) |

**Weapons' inline clear.** `Unit::ClaimWeapons` and `Unit::ReleaseWeapons` (weapons' code) held a copy of
`ClearWeaponTarget`: they wrote the target pair and ran the unit's TargetCleared script themselves. They now publish
CLEAR, so targeting alone writes the pair (section 5). The slot flag bit they flip stays theirs.

**Answers.** No caller of the four intents used a value: 3.1c's `SetWeaponTargetUnit`, `SetWeaponTargetPos`,
`ClearWeaponTarget` and `ResetWeaponTarget` returned nothing. Three slots were mixed, and each stays a slot,
marked synchronous, because its caller uses the answer in the same statement:
- **`FindWeaponTarget`** draws from the Park-Miller stream (section 6) and returns the target ai's `RetargetWeapon`
  aims at next. Split into a query and an intent, the draw would still be made by the query, so the split gains
  nothing; the slot is synchronous.
- **`FindBestTargetIfFireAtWill`** is `FindWeaponTarget` for the first weapon of a unit that fires at will, and
  orders gives its answer to `IssueAttackOrder` in the same statement.
- **`GetWeaponTargetPos`** returns the point a weapon aims at to `UpdateUnitWeapons`, which aims and fires at it. When
  the target unit is gone it clears the pair and publishes CLEARED, as 3.1c ran the script there; and it calls
  `scripts.GetSweetSpot`, which is mixed itself (it runs the target's SweetSpot query script and poses the target's model,
  writing its pieces' state). Both happen at 3.1c's statement because the slot is synchronous.

**Outcomes.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 38) and returned to no
one: SET_UNIT and SET_POS 1, CLEAR and RESET 1 when the weapon had a target, 0 when not. `Apply` first rejects an
event with a null unit, a weapon outside 0 to 2, or (SET_UNIT) a null target, with outcome 0 and no change. 3.1c's
callers passed pointers and could publish none of these, so no trace moves; a module publishing from WASM can.

## 3. Effects out of targeting

| 3.1c call | Effect | Subscriber's handler |
|---|---|---|
| `scripts.FindScript(script, "StartBuilding")`, then `scripts.StartScriptWithArgs(script, "TargetCleared", ..., weapon)`, when a weapon that had a target loses it (`ClearWeaponTarget`, the dead target of `GetWeaponTargetPos`) | CLEARED, with the unit and the weapon | `UnitsOnTargetingCleared`: the same two calls |

The `FindScript` call looks up a function whose answer 3.1c threw away; the adapter keeps it, since the lookup is
scripts' code and its cost is not targeting's to judge.

## 4. Views

**What targeting reads:** a unit's weapon slots (the weapon, its flags and range), the unit's and the target's
position, flags, definition, player and kill count, the ground height (`map.GetGroundHeight`) and the sea level, a
target's sweet spot (`scripts.GetSweetSpot`) and its motion's body offset, the units a weapon may pick among (through
ai's and los_radar's views), all as of each module's last run. `FindWeaponTarget` calls the FilterTargets hook.

**What others read of targeting:**
- **The target pair, in the `Unit` record, hashed** (section 5). No code outside targeting reads it: every read went
  through `GetWeaponTargetUnit` or `GetWeaponTargetPos` in 3.1c, and goes through them now.
- **Queries:** `GetWeaponTargetUnit` and `ChooseWeapon`.
- **`Unit::netDirtyFlags`**, the notices a unit's orders wait on, is not targeting's: weapons sets the bits of a shot
  fired, a command fire and a target out of reach, orders consumes them in `RunOrders`, and units clears them as it
  sets a unit up. SET_UNIT and SET_POS clear the three weapon notices (and two others), as 3.1c did; that write
  stays in the events.

**As of targeting's last apply.** By #952's rule a query needs a delivery first only when an event that can be pending
writes what it reads, and that is a property of the code, not of what the scenarios reach.
- `GetWeaponTargetUnit` reads the pair, which every event writes, so its service delivers targeting's inbox first.
- `ChooseWeapon` reads only the slots' flags, which weapons writes and no event does, so it is called directly.
- `GetWeaponTargetPos`, `FindWeaponTarget` and `FindBestTargetIfFireAtWill` deliver first as synchronous slots.

So the pair is always read as 3.1c read it, whatever is queued. What a pending event can still get wrong is what it
writes outside the pair, and that decides each route:

| Route | Now | Why |
|---|---|---|
| orders SET_UNIT | synchronous | The gate (section 8). It clears the weapon notices of `netDirtyFlags`, and the unit's `RunOrders` re-reads them in the same update. |
| orders SET_POS | synchronous | #952's rule. `SuppressOrder` and the air orders (`unit_orders.cpp`, `vtol_orders.cpp`) publish it, set `order->flags` to 0x1c00 and return 1, and `RunOrders` then reads `netDirtyFlags` at once: with the event pending, the stale 0x400 and 0x1000 bits it clears would wake the order. |
| orders CLEAR | synchronous | #952's rule. A CLEAR of a weapon with a target publishes CLEARED, which starts the unit's TargetCleared thread. Pending, it would start after the other threads started on the unit later in its update, in another of the eight channels, and after a death later in the phase has freed the unit's script (a build with it queued crashed so, in `FindScript`). |
| ai SET_UNIT | synchronous | #952's rule. `ReactToAttack` runs where a hit is applied, in the projectiles phase among others, and ai reads the `netDirtyFlags` bits the event clears later in the same phase (the row targeting → ai on `Unit::netDirtyFlags`, projectiles, below). |
| ai CLEAR | synchronous | #952's rule. The squad manager's `RetargetWeapon` runs in the players phase before the player's economy, which publishes the extractors' SetSpeed scripts to units' inbox. The phase boundary delivers units before targeting, so a pending CLEAR's TargetCleared thread on an armed extractor would start after SetSpeed instead of before it. |
| ai SET_POS | queued | Only `RetargetWeapon` publishes it, for an interceptor aimed at a projectile, in the players phase. It writes the pair and the `netDirtyFlags` weapon notices; the pair is read only through targeting's slots, which deliver first, and nothing in the players phase reads or writes those notices (`RunOrders` runs only in the units phase, and victory reads bit 0, which the event keeps). `ai-antinuke-intercept` reaches it, and its trace is equal with the route queued (section 8). |
| units RESET | queued | `InitUnitFromType` publishes it for each slot of a unit it sets up. It writes only the pair, read only through targeting's slots, and publishes no effect. The `ReleaseWeapons` that follows it may publish a synchronous CLEAR of the same slot, which delivers the RESET first; the unit's `netDirtyFlags` is set to 0 after both. |
| units LOAD | queued | `LoadUnit` publishes it for each slot of a unit a saved game loads, outside the tick, where #970's rule delivers it at once. It writes only the pair and publishes no effect. |
| weapons CLEAR | synchronous | The gate (section 8). Its CLEARED starts the TargetCleared thread, whose channel among the unit's script threads is hashed. |
| CLEARED, targeting to units | synchronous | The gate (section 8), for the same channel order. |

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv`, regenerated on this change, has these rows of
class `immediate` with targeting as writer or reader. The access log names the target pair of slot 0, 1 and 2 by the
union members of `Unit` at its offset (`Unit::current`, `Unit::field_20`, `Unit::weapon2Target`), a slot's flags by
`Unit::f_1f`, `Unit::f_3b` and `Unit::f_57`, and its weapon by `Unit::weapon`, `Unit::f2c` and `Unit::f48`; the
player's fields by the `Game` union aliases (`unknown_1b8e`, `pad_1df4`, `unknown_1cd9`). No row names the target
pair: only targeting writes it, and only targeting reads it. Each row is served as follows:

| Writer → reader | Items | Phase | How it is served |
|---|---|---|---|
| weapons → targeting | a slot's flags (`Unit::f_1f`, `f_3b`, `f_57`) and weapon (`Unit::weapon`, `f2c`, `f48`) | units | Targeting reads them only in its synchronous slots and synchronous events, at the caller's statement, so it sees weapons' write as 3.1c did. |
| units, orders, map, ai, los_radar → targeting | `Unit::flags`, `def`, `unitDefIndex`, `killCount`, `pos`, `yFraction`, `zFraction`, `motion`, `UnitMotion::velocity` | units, players | The same: `FindWeaponTarget`, `FindBestTargetIfFireAtWill` and `GetWeaponTargetPos` read them at the caller's statement. The queued RESET reads none of them. |
| economy, players, units → targeting | the player's fields (`pad_1df4`, `unknown_1b8e`, `unknown_1cd9`) | units, players | The same, through `FindWeaponTarget`'s choice among enemies. |
| ai → targeting | `Unit::netDirtyFlags` | projectiles | SET_UNIT and SET_POS clear bits of it as a read-modify-write. Ai publishes only SET_UNIT in the projectiles phase (`ReactToAttack`), and it is synchronous, so the write lands on ai's bits as in 3.1c. Its queued SET_POS is published only in the players phase. |
| targeting → orders, units, weapons | `Unit::netDirtyFlags` | units | The bits SET_UNIT and SET_POS clear. Every route that publishes them in the units phase is synchronous. |
| targeting → ai | `Unit::netDirtyFlags` | projectiles | The same, for ai's SET_UNIT from `ReactToAttack`. |

## 5. State and the trace

Targeting keeps no private region, and so it has no `WriteState` report: its state is view state, hashed where it was.

- **The target pair** of each of a unit's three weapon slots (a target unit's number with the mark 0x8000, or a
  ground point's x and z in whole units, or none) stays in the `Unit` record and in the hash (section 1 of the trace, `units[i]`).
  It is targeting's alone to write: `privatestate.py --check` fails when another subsystem writes it inside the
  tick. A saved game loads outside the tick, where the access log records nothing, so the check cannot see the
  load: until #1010 units' `LoadUnit` wrote the pair itself and the check passed. LOAD holds the load to the same
  rule. Moving it to a
  private region would be a trace format bump with a reference half (#946's path) for no gain in what a
  re-implementation must match: every reader already reaches it through targeting's slots, and the hash is what shows
  a wrong target at the tick it is chosen (the negative test, `tests/targeting_writer_test.cpp`).
- **The rest of a weapon slot** (the weapon, its flags, reload, stockpile and aim angles) is weapons'.
- **The trace dumps targeting's events** in section 38 (`docs/trace-format.md`, section 8.26), after features'
  section 36, and hashes none of them: no trace format bump and no reference half. Each record is an event with the
  outcome `Apply` gave, or a CLEARED targeting published, in targeting's order.

## 6. Random draws

Targeting draws from one stream (`data/draws.csv`): `rng.parkMiller`, in `FindWeaponTarget`, to pick among the
enemies in reach, in the units phase (through `FindBestTargetIfFireAtWill`, from orders) and the players phase
(through `FindWeaponTarget`, from ai's retargeting). No event draws: SET_UNIT, SET_POS, CLEAR, RESET and LOAD write
the pair and nothing else of the streams. CLEARED starts a script thread, which draws only when it runs, in units' run.

The class `ordered` of `data/dependencies.csv` names each pair of subsystems whose draws interleave in one phase. Each
pair with targeting is kept in 3.1c's order as follows:

| Pair | Phase | How the order is kept |
|---|---|---|
| orders → targeting, targeting → orders | units | Orders calls `FindBestTargetIfFireAtWill` through `ScheduleTargetingFireAtWill` (`unit_orders.cpp`, `vtol_orders.cpp`), a synchronous slot: the draw is made at 3.1c's statement, between orders' own. |
| ai → targeting, targeting → ai | players | Ai's `RetargetWeapon` calls `FindWeaponTarget` through `ScheduleTargetingFind`, synchronous in the same way. |
| units, weapons, map → targeting, targeting → units, weapons, map | units | Targeting draws in the units phase only inside orders' synchronous calls (above), so its draws fall between units', weapons' and map's where 3.1c made them. No event draws, and the TargetCleared thread that CLEARED starts draws only when units runs it, in the channel the synchronous CLEARED keeps. |

## 7. What stays inside the default

3.1c's types and limits (three weapon slots, the pair's 16-bit halves and the mark 0x8000 of a target unit, the
ground point's whole units, the target choice's ranges and weights, the body offset scale of a moving target) are the
default's, not the interface's.

## 8. What the gate decided

Every event and the effect were queued first. The tier judged each build, and a route whose queueing diverged became
synchronous, with the reason here. A route that stayed equal but can be pending when a read in the same phase needs
what it writes became synchronous too, by #952's rule (section 4). The divergences are against the reference, with the
first tick, phase and field.

**Orders' SET_UNIT is synchronous.** With every route queued, the tier diverged in mission-radius at tick 1432, phase
world, field `units[3].list[0].state` (0x01 against 0x03), then the order's `flags6`, `wakeFrame` and `subFlags`. An
attack order (`AttackChaseOrder`) aims a weapon and returns, and the unit's `RunOrders` then reads `netDirtyFlags`:
with SET_UNIT pending, a weapon notice the event clears was still set, so the order woke a state early. With CLEARED
also synchronous the divergence was the same.

**Weapons' CLEAR and CLEARED are synchronous.** With orders' SET_UNIT synchronous, eight-full-moon diverged at tick
2404, phase world, fields `units[501].script.channels[2]` and `[3]` swapped. An instrumented build showed unit 501's
CLEARs coming from weapons' outbox (`Unit::ReleaseWeapons`, `Unit::ClaimWeapons`), and the TargetCleared thread took
another channel than in 3.1c. Queueing either one alone, the CLEAR with CLEARED synchronous or CLEARED with the CLEAR
synchronous, diverged at the same tick and fields: each delays the thread's start past other threads started on the
unit in the phase.

**The rest.** With those three synchronous, all 38 tier scenarios were equal, and #952's rule then made orders'
SET_POS and CLEAR and ai's SET_UNIT and CLEAR synchronous (section 4). That build passed the tier (38 scenarios equal)
and the full suite in both modes, with ai's SET_POS and units' RESET queued.

**Ai's SET_POS is queued (#975).** #955 left it synchronous because no scenario aimed an interceptor at a
projectile, so no trace had judged it queued. `ai-antinuke-intercept` does: the computer player's anti-nuke silo
(unit 252), holding one stockpiled missile, finds the human player's nuke in flight at tick 5694, and ai publishes a
SET_POS of its weapon 0 at the nuke's position in the players phase (outcome 1, section 38 of the dump). Queued, the
event waits in targeting's inbox until the players phase's boundary, alone; the silo fires at tick 5696 and its
missile meets the nuke at tick 5926. The scenario's trace is equal to the reference's with the route queued, in both
modes, and so is the full suite. Two queued routes are left: units' RESET and ai's SET_POS.

**Inbox depths.** With the final routes, over 88 of the 89 scenarios (load-midgame plays only after save-midgame, and
was left out), targeting's inbox held at most 9 events (long-greenhaven-8p, tick 52464), all of them units' RESETs
of units set up in one update; the capacity is 1024. RESET was queued in 52 scenarios, 5967 times. Ai's SET_POS was
queued in one, once (`ai-antinuke-intercept`, tick 5694). Of the deliveries of a non-empty inbox, 1719
came from a synchronous publish or slot that delivered first and 249 at a phase boundary (at most 6 events, long-great-divide-2p, tick 34050).
