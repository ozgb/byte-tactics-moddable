# The units interface

Units sits behind `interfaces/units.h` (version 7.2; 7.2 adds FORGET_ATTACKER; 7.1 adds the console's kinds CREATE, KILL_ALL and RELOAD_TYPE; 7.0 passes `CreateUnit`'s position by pointer and names its `finished` and `id`, writes the units of `GetFactoriesInRadius` to a caller buffer, has `FindMissionUnit` take the caller's table of the mission units' units and return the match after a given unit, in place of a vector, makes `CanLoad`, `CanReclaim`, `CanRepair` and `UnitRefUnlink` stdcall instead of thiscall with no unused register argument, and makes the read-only strings const), the seam over the unit record (`Unit`, 0x118 bytes,
`src/units/unit.h`) that every simulation module reads, and since #986 it is on the scheduler with scripts
(`interfaces/scripts.h`, version 3.0; `docs/scripts-interface.md`). 3.1c's code is its default implementation. This
page is the inventory the interface was cut from, written from the code as it now is:
- where units sits, and what it is not;
- what other subsystems did to the unit record in 3.1c, and what each became;
- what units does to others, and what each became;
- what units reads, what others read of it, and each route with its reason;
- the state, and what the trace hashes and what it only dumps;
- how the order of random draws and sim events is kept;
- what stays inside the default, and what the gate decided.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, events published
outside the tick delivered at once (#970), and events as plain data with one payload per kind (#1009). This issue
changes none of its rules. Event kinds are unique across interfaces: units uses 352 to 367 and 384 to 398, two blocks
(its 27 kinds do not fit in the 16 of one block's lower half, and units has no effects of its own, so the second
block's lower half takes the rest), and scripts 416 to 419 (its range is 416 to 447); weapons, orders and movement use
256 to 351 (`docs/movement-interface.md`, section 1).

The rulings of the human that shape this change:
- **Units owns the unit record** (the issue): every write to it is units' or the declared view state of another module
  (section 5). **Weapons owns the damage rules; units owns the record** (2026-11-17, on #984): weapons decides a hit's
  health and dying mark and units writes them (SET_HEALTH). **Movement owns the motion fields** (2026-11-18): the
  position, angles, cell, mode and moved mark movement steers are written by units' handlers of movement's effects,
  as movement says, and nothing else decides them.
- **Scripts are their own module** (2026-11-18, on #986), and **one pull request for units and scripts** (the
  maintainer, 2026-11-19): a script started or queried runs at once and writes the unit record, and the unit's update
  runs its scripts, so the routes between the two are settled together.
- **No slot passes memory ownership across a seam, and major bumps are cheap** (2026-11-19): every event names a unit,
  a reference or a model state by its block offset.

Units' place in it:
- **Events into units:** the 27 kinds of section 2, published by orders, movement, weapons, players, ai and economy
  (through their effects' adapters, now handlers of units' inbox) and by scripts. Units' inbox (`UnitsInbox`,
  `src/seams/units_world.cpp`) passes each to `Apply` (`UnitsApply`, `src/units/units_apply.cpp`) and records it with
  its outcome in the trace's dump (section 5).
- **Runs:** `UpdateUnitHead`, `UpdateUnitBody` and `UpdateUnitTail`, steps of the scheduler's unit loop
  (`SchedUnitsPhase`, `src/game/scheduler.cpp`), with units' inbox delivered before each; `RefreshFactoryIndex` in the
  players phase.
- **Synchronous slots with a same-statement answer:** `CreateUnit`, `LoadUnit`, `AddBuildProgress` and
  `AddRepairProgress` (section 2), and the unit reference slots `UnitRefInit`, `UnitRefSetUnit`, `UnitRefSetValue` and
  `UnitRefUnlink`, which build and link memory the caller owns.
- **Queries:** `FindUnitType`, `FindUnitTypeId`, `FindMissionUnit`, `CanLoad`, `CanReclaim`, `CanRepair`,
  `GetFactoriesInRadius`, `UnitRefGet`, `GetMovementLimits`, and the view read `WriteState`.
- **Effects out of units:** none of its own. Units publishes other modules' events (section 3).
- **Module ids:** units keeps `SCHED_UNITS = 2`, and scripts `SCHED_SCRIPTS` from #993.
- **No WASM module.** The slots are native: `src/seams/native.cpp` fills them, and the harness records each call
  through a replay hook (`src/harness/replay_hooks.inc`).
- **Versions.** Units 6.0 (8 slots fewer than 5.0 and `Apply` new), scripts 2.0 (3 fewer and `Apply` new), services 2.2
  (`HarnessUnitsEvent` and `HarnessScriptsEvent`, the trace's record of each event).

## 2. Events into units

**Every slot of 5.0 that left.** The 8 slots units 5.0 had for other modules' writes, and what each became in 6.0:

| 5.0 slot | 6.0 |
|---|---|
| `SetStateBits` | event SET_STATE_BITS, from ai (ACTIVATE), economy (CLOAK), orders and movement (their SET_STATE_BITS), and inside CAPTURE and SCRIPT_VALUE |
| `MarkRecentlyDamaged` | event MARK_DAMAGED, from weapons (MARK_DAMAGED) |
| `NotifyUnitRefs` | event NOTIFY_REFS, from weapons (ATTACKED, with the event 16) |
| `AttachUnitToPiece` | event ATTACH_TO_PIECE, from orders and scripts (`AttachUnit`, `DropUnit` of a unit's script); and from the engine (`RunInitialMission`, a unit's initial mission that loads it into a transport, while a mission loads) |
| `ApplyUnfinishedBuildDecay` | event BUILD_DECAY, from orders |
| `FinishConstruction` | event FINISH_CONSTRUCTION, from orders |
| `SetUnitPosition` | event SET_POSITION, from orders and movement |
| `UpdateSfxOccupy` | event SFX_OCCUPY, from movement |

**The writers, one by one.** Every adapter onto units' code is a handler of units' inbox, in
`src/seams/units_world.cpp`: it turns the foreign payload into a units event and passes it to `UnitsInbox`, refusing a
payload smaller than its struct as before. The adapters' names are unchanged; each route's mode is in section 4, where
six of the routes into units that were queued become synchronous.

| Publisher's effect | Handler | Units event |
|---|---|---|
| ai ACTIVATE | `UnitsOnAiActivate` | SET_STATE_BITS |
| ai SET_STATES | `UnitsOnAiSetStates` | SET_FLAGS, mask 0x3c0000 (the movement and fire states) |
| economy CLOAK | `UnitsOnEconomyCloak` | SET_STATE_BITS, mask 4 |
| players KILL_UNIT, SEND_UNIT, CAPTURE_UNIT | `UnitsOnPlayersKillUnit`, `UnitsOnPlayersSendUnit`, `UnitsOnPlayersCaptureUnit` | KILL, SEND, CAPTURE |
| weapons ATTACKED (two routes) | `UnitsOnWeaponsAttacked`, `UnitsOnWeaponsAttackedSpeech` | NOTIFY_REFS (event 16), ATTACKED_SPEECH |
| weapons HIT | `UnitsOnWeaponsHit` | SET_NET_DIRTY, the bit 0x4000 or 0x2000 by the damage it records |
| weapons SHOT_FIRED, COMMAND_SHOT_FIRED, OUT_OF_REACH | `UnitsOnWeaponsNotice` | SET_NET_DIRTY, each kind's bit |
| weapons MARK_DAMAGED, DAMAGE_RECORD, HEALTH, BUSY | `UnitsOnWeaponsMarkDamaged`, `UnitsOnWeaponsDamageRecord`, `UnitsOnWeaponsHealth`, `UnitsOnWeaponsBusy` | MARK_DAMAGED, DAMAGE_RECORD, SET_HEALTH, SET_WORK_TIME |
| orders' 13 effects to units | `UnitsOnOrders*` | SET_STATE_BITS, SET_POSITION, ATTACH_TO_PIECE, FINISH_CONSTRUCTION, BUILD_DECAY, REPAIR, UNLINK, SET_WORK_TIME (BUSY), SET_FLAGS, SET_NET_DIRTY (CLEAR_NET_DIRTY, which clears), SET_STANCE, SET_ANGLES, SET_BUILD_STATE |
| movement's 10 effects to units | `UnitsOnMovement*` | TURN, SET_ANGLES (TILT: bank and pitch; ANGLES: all three), MOVE, SET_MOVED, CHANGE_CELL, SET_FLAGS (MOVE_RATE, mask 0xc), SET_POSITION, SET_STATE_BITS, SFX_OCCUPY |
| scripts: `UnitScript::SetUnitValue` | units' own event, published from `SCHED_SCRIPTS` | SCRIPT_VALUE |
| scripts: `UnitScript::AttachUnit`, `DropUnit` | units' own event, published from `SCHED_SCRIPTS` | ATTACH_TO_PIECE |

Economy's EXTRACTOR, features' FREE_STATE and INIT_STATE and targeting's CLEARED, which #966 to #971 routed to units'
adapters, only ever reached scripts' code (a script start, or a feature's model state): they are handlers of scripts'
inbox now (`docs/scripts-interface.md`, section 6): FREE_STATE, INIT_STATE and CLEARED with their modes unchanged, and
EXTRACTOR, queued before, synchronous (section 4).

**The mixed slots.** Each writes and returns an answer the caller reads in the same statement, so each stays a
synchronous slot, marked so in `interfaces/units.h` and `interfaces/scripts.h`; none is split, because the answer is the
write's own result:
- **`CreateUnit`** returns the new unit, which orders' builders link into their order in the same statement
  (`BuildingBuildOrder`, `MobileBuildOrder`, `ResurrectOrder`, `VtolMobileBuildOrder`) and players' commander respawn
  uses (`UpdatePlayerBody`, #967). It draws from the game logic stream first thing.
- **`LoadUnit`** returns the unit a saved game's record names, for movement's and orders' load constructors, outside
  the tick.
- **`AddBuildProgress`** and **`AddRepairProgress`** return 1 when the unit is done, which the builder's order reads in
  the same statement to end its state (`BuildingBuildOrder`, `HelpBuildOrder`, `MobileBuildOrder` and their VTOL twins;
  `SelfRepairOrder`, `RepairUnitOrder`, `RepairUnitNoMoveOrder`). Orders' REPAIR effect is the call that uses no answer.
- **Scripts' `QueryScript`, `GetNanoPiecePosition`, `GetAimFromPosition` and `GetSweetSpot`** run a query script of
  the unit at once and return what it wrote (`docs/scripts-interface.md`, section 2).

The `TODO(#986)` markers that named these answers in `src/orders/unit_orders.cpp` and `vtol_orders.cpp` are gone.

**Writers outside the tick** (section 2 of the issue):
- **The load of a match.** `LoadMatch` (`game_load.cpp`, the mission's units) and `SpawnCommanderAtStartPos`
  (`main_loop.cpp`) call `CreateUnit`, and a saved game's `LoadUnits` (`settings_campaign.cpp`) reads every unit through `LoadUnit`; all
  before the first tick, so what they publish is delivered at once (#970).
- **The console.** `KillAllUnits`, `KillUnitsOfType` and `ReloadUnitType` (`console_commands.cpp`), and its `CreateUnit`
  of a unit type, are slots called between ticks. They wait for #948 (parked), as #954's did.
- **The network.** The net phase's unit packets (`ReadUnitState` through `ReceiveUnitStates`, `unit_position.cpp`;
  `CreateUnitFromPacket`, `net_game.cpp`) write the unit record directly, and wait for #948 as well.
- **The engine's shutdown** kills every unit through the slot `KillUnit` (`game_state.cpp`), outside the tick.

**Outcomes.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 50) and returned to no
one: 1 for every kind, 0 for the events it refuses, and CAPTURE's 0 when no copy could be made. `Apply` refuses, with
outcome 0 and no change, an event smaller than its kind's payload, an unknown kind, an event naming a null unit (all
but UNLINK, which names a reference, and CREATE, KILL_ALL and RELOAD_TYPE, which name none) and an UNLINK naming no reference. Since 7.1 the engine's console (the unit-name command, `kill`, `reload`, `selectable`) and the harness's scripted spawns publish CREATE, KILL_ALL, RELOAD_TYPE and SET_FLAGS from `SCHED_ENGINE`, in place of direct calls to the slots and direct writes to the unit record. Since 7.2 the debug hotkey that makes the cursor's unit forget its attacker publishes FORGET_ATTACKER from `SCHED_ENGINE` in place of the writes.

**Payloads.** Every event starts with `UnitsEventHeader`: `kind` and `unit` (the unit it is about, a block offset; 0
for none). Each kind has the payload of `interfaces/units.h` that starts with it, or is the header alone (SFX_OCCUPY,
ATTACKED_SPEECH, MARK_DAMAGED).

| Kind | Payload | Fields after the header |
|---|---|---|
| SET_STATE_BITS 352 | `UnitsSetStateBits` | `mask`, `set` |
| SET_FLAGS 353 | `UnitsSetFlags` | `clear`, `set` |
| SET_NET_DIRTY 354 | `UnitsSetNetDirty` | `clear`, `set` |
| SET_WORK_TIME 355 | `UnitsSetWorkTime` | `until` (a tick) |
| SET_POSITION 356 | `UnitsSetPosition` | `pos[3]`, `mode` |
| ATTACH_TO_PIECE 357 | `UnitsAttachToPiece` | `target` (a block offset, 0 to detach), `piece`, `mode` |
| FINISH_CONSTRUCTION 358 | `UnitsFinishConstruction` | `builder` |
| BUILD_DECAY 359 | `UnitsBuildDecay` | `ticks` |
| REPAIR 360 | `UnitsRepair` | `builder`, `amount` (a float) |
| UNLINK 361 | `UnitsUnlink` | `ref` (the reference, a block offset) |
| SET_STANCE 362 | `UnitsSetStance` | `fields` (`UnitsStanceField`), `moveOrder`, `fireOrder`, `group` |
| SET_ANGLES 363 | `UnitsSetAngles` | `fields` (`UnitsAngleField`), `bank`, `heading`, `pitch` |
| SET_BUILD_STATE 364 | `UnitsSetBuildState` | `buildLeft` (a float), `health` |
| TURN 365 | `UnitsTurn` | `delta` |
| MOVE 366 | `UnitsMove` | `pos[3]` |
| CHANGE_CELL 367 | `UnitsChangeCell` | `pos[3]`, `cell[2]`, `mode` |
| SET_MOVED 384 | `UnitsSetMoved` | `moved` |
| SFX_OCCUPY 385, ATTACKED_SPEECH 388, MARK_DAMAGED 389 | the header alone | none |
| SCRIPT_VALUE 386 | `UnitsScriptValue` | `which` (the COB unit value), `value` |
| NOTIFY_REFS 387 | `UnitsNotifyRefs` | `event` |
| DAMAGE_RECORD 390 | `UnitsDamageRecord` | `damageKind`, `attacker` |
| SET_HEALTH 391 | `UnitsSetHealth` | `health`, `dying` |
| KILL 392 | `UnitsKill` | `selfDestruct` |
| SEND 393 | `UnitsSend` | `player` |
| CAPTURE 394 | `UnitsCapture` | `player`, `present`, `hitPoints`, `buildPercent`, `angles[3]`, `stockpiles[3]` |
| CREATE 395 | `UnitsCreate` | `player` (a byte), `type` (16 bits), `pos[3]` (world coordinates, 16 fractional bits); names no unit; outcome 1 when a unit was made, 0 when none was |
| KILL_ALL 396 | the header alone | none; names no unit |
| RELOAD_TYPE 397 | `UnitsReloadType` | `type`: every unit of it is killed, then its definition is loaded again; names no unit; the only change to load-time data during play (section 5) |
| FORGET_ATTACKER 398 | the header alone | the unit: sets `lastAttackerSlot` to 10, the `Unit` flags bit 0x4000 (`panel.flag_110`, the panel refresh) and `attacker` to 0; outcome 1, 0 for a null unit |

## 3. Effects out of units

Units has no effect kinds: what it does to other modules goes out as their events, through the routes the issues
before this one gave it, and they are kept (section 4). This change adds one: **a unit's own script starts**
(`Unit::SetStateBits`' Activate, Deactivate, StartBuilding and StopBuilding, `UpdateWindGenerator`'s SetDirection and
SetSpeed, `sfx_occupy.cpp`'s setSFXoccupy, `InitUnitScript`'s Create and the death's Killed script) are scripts' event
START, published from `SCHED_UNITS` (`ScriptsPublishStart`, `src/game/scripts_events.h`) with 3.1c's arguments.

The calls units keeps into scripts are the life cycle of a unit's script and model state, synchronous slots by their
contract: `NewUnitScript`, `SetCob`, `CreatePlayerObjectState`, `SetObjectState`, `CreateObjectState` (`InitUnitScript`,
the unit's creation), `DeleteScript` and `FreeObjectState` (as the unit dies), `LoadScriptState` and `SaveScriptState`
(a saved game), and the queries with an answer (`QueryScript` "Killed" in `KillUnit`, `GetSweetSpot` in
`GetWeaponTargetPos`). `RunScripts` left units: the scheduler calls it (section 6).

`features.Place` stays a synchronous slot (`ScheduleFeaturesPlace`), `economy.UpdateMetalExtraction` goes through
`ScheduleMetalExtraction`, and units' reports to pathfinding stay its calls to pathfinding's seam, as #966 left them.
The scheduler names no event kind.

## 4. Views and routes

**What units reads:** the order list's first order and its flags (orders' `GetOrderFlags` and the view), the weapon
slots (weapons' view), the players' records, the map's cells and heights (map's slots and services), its script's
answers (scripts' queries), as of each module's last run.

**What others read of units:** the unit record, a view in the state block, hashed, read by every module, and the
queries of section 1. Section 5 says which fields are another module's view state.

**As of units' last run.** By #952's rule a query needs a delivery first only when an event that can be pending writes
what it reads. Five routes into units stay queued (the table below): orders' BUSY and SET_FLAGS and movement's MOVED,
MOVE_RATE and SFX_OCCUPY. They write `workTime`, the flag bits 0x20, 0x800, 0x8000 and 0xc, the moved mark and the SFX
tracker's state. Each query's reads, against those writes:
- `FindUnitType`, `FindUnitTypeId`, `FindMissionUnit` and `GetMovementLimits` read the unit types, which no event writes.
- `CanRepair` (`src/units/unit.cpp`) reads the builder's type and the other unit's `health`, type, `flags & 3` and
  `yWhole`. `CanLoad` reads both types, the carrier's cargo list and the other unit's `motion`, `flags & 3`, `pos.y` and
  `buildLeft`. `CanReclaim` reads both types and the other unit's `flags & 3`. No queued route writes any of these:
  `health` and `buildLeft` are written by orders' SET_BUILD_STATE and BUILD_DECAY, and `pos` by movement's and orders'
  SET_POSITION, all synchronous since this change (below). So the queries read them at the publisher's statement, as
  in 3.1c, wherever orders calls them.
- `UnitRefGet` reads a reference's unit, written by units' own code and by UNLINK (synchronous). `GetFactoriesInRadius`
  reads the factory index, which only `RefreshFactoryIndex` writes.

**Scripts' reads of the record.** A script reads the unit record through `UnitScript::GetUnitValue`
(`src/scripts/unit_script.cpp`), which reads:
- `health` (4) and `buildLeft` (17);
- the movement and fire stances, flag bits 18 to 21 (2 and 3);
- the piece positions, from `pos` and the angles (7 and 8);
- another unit's `pos`, behind its bit 0x10000000 (9 and 10);
- `heading` (12);
- and the bits that SCRIPT_VALUE writes (synchronous).

Which of these a script reads is in its COB code, which is data, so no argument from the publishers' code can show that
a pending write goes unread. A script runs at once:
- when a synchronous START reaches it, from units, weapons, orders, movement, targeting or economy;
- from the slots that take a channel (`StartScriptWithArgsByIndex`, `QueryScript`, `GetNanoPiecePosition`,
  `GetAimFromPosition`, `GetSweetSpot`).

Before this change, two queued writes met such a read:
- For a carried unit, `UnitMotion::UpdatePosition` publishes SET_POSITION and ANGLES. `UpdateMoveRate` then starts
  StopMoving with update 1, whose first slice could read 7, 8 or 12 before units' inbox was delivered.
- `StandingFireOrder` publishes SET_STANCE and then targeting's CLEAR, whose CLEARED starts TargetCleared at once, which
  could read 3.

So every route into units that writes a field in that list is synchronous: orders' SET_STANCE, SET_ANGLES,
SET_BUILD_STATE and BUILD_DECAY, and movement's ANGLES and SET_POSITION. Each lands its write at the publisher's
statement, as 3.1c did, and the tier stayed equal (section 8). The five routes still queued write nothing a script
reads: `GetUnitValue` reads `flags` only at bits 18 to 21 and 0x10000000, and no opcode reads `workTime`, the moved
mark or the SFX tracker.

**The routes.** Every route into or out of units and scripts, with its reason. The routes below that units published
or received before this change cite the document that decided them, except the seven this change makes synchronous
(EXTRACTOR, four of orders' and two of movement's), which cite the reads above; the routes new in this change cite the
gate (section 8) or #952's rule.

| Route | Now | Why |
|---|---|---|
| units START, to scripts | synchronous | The gate (section 8): queued, scenario `orders-air-fights` diverged. A script started runs its first slice at once in 3.1c and can draw and record sim events. |
| scripts SCRIPT_VALUE, to units | synchronous | The gate (section 8), and #952's rule: the same script thread reads the value back (`GetUnitValue` of ACTIVATION, `bit0`, `bit1`) in its next instruction. |
| scripts ATTACH_TO_PIECE, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but the handler (`AttachUnitToPiece`) reads the passenger's `cargo` and the carrier's `carrier`, which the previous attach or drop of the same script run writes (COB's attach and drop opcodes run in one thread slice, as a transport's TransportPickup and TransportDrop do), and it sends the attach to the network in the publisher's order; orders' ATTACH_TO_PIECE is synchronous for the same handler (`docs/orders-interface.md`, section 4). |
| scripts SET_YARD, to map | synchronous | The gate (section 8): queued, scenario `orders-air` diverged. Units' own SET_YARD is synchronous by the gate too (`docs/map-interface.md`, sections 4 and 8). |
| engine ATTACH_TO_PIECE, to units | queued | Published only by `RunInitialMission` while a mission loads, before the first tick, so it is delivered at once (#970) whichever its mode, as orders' ADD from the same function (`docs/orders-interface.md`, section 4). |
| economy EXTRACTOR, to scripts (`ScriptsOnEconomyExtractor`; was units') | synchronous | #952's rule. The start takes one of the script's thread channels, and channels are hashed (section 23 of `docs/trace-format.md`). Queued, the start waited in scripts' inbox. A synchronous route into units, the phase boundary (which delivers units before scripts) and scripts' slots that take a channel do not deliver that inbox, so a later start on the unit in the same phase could take the channel first. For example, a packet's `StartScriptWithArgsByIndex` could follow the `CreateUnitFromPacket` that made the extractor. Synchronous, the start runs at `UpdateMetalExtraction`'s statement, where 3.1c called it. EXTRACTOR was scripts' only queued route, so scripts' inbox is empty in the tick. |
| features FREE_STATE, INIT_STATE, to scripts (was units') | synchronous | As before (`docs/features-interface.md`, sections 4 and 8). |
| targeting CLEARED, to scripts (was units') | synchronous | As before (`docs/targeting-interface.md`, section 8). |
| economy CLOAK, to units | synchronous | As before (`docs/economy-interface.md`, section 4). |
| ai ACTIVATE, SET_STATES, to units | synchronous | As before (`docs/ai-interface.md`, section 8). |
| players KILL_UNIT, SEND_UNIT, CAPTURE_UNIT, to units | synchronous | As before (`docs/players-interface.md`, section 8). |
| weapons' 10 routes to units | synchronous | As before (`docs/weapons-interface.md`, sections 4 and 8; `docs/ai-interface.md`, section 4, for HIT). |
| orders' 13 routes to units | 11 synchronous, BUSY and SET_FLAGS queued | BUILD_DECAY, SET_STANCE, SET_ANGLES and SET_BUILD_STATE are synchronous since this change, because queries and scripts read what they write (above). The rest are as before (`docs/orders-interface.md`, section 4). The queued two rest on CLEAR_NET_DIRTY staying synchronous and unconditional, which it does: units' handler of it is a units event like the others. |
| movement's 10 routes to units | 7 synchronous, MOVED, MOVE_RATE and SFX_OCCUPY queued | ANGLES and SET_POSITION are synchronous since this change, because queries and scripts read what they write (above). The rest are as before (`docs/movement-interface.md`, section 4). |
| units' routes to economy, players, weapons, ai, features, targeting, los_radar, map, orders and movement | as they were | Each with its reason in its subscriber's document, unchanged. |

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv` has 166 rows of class `immediate` with units or
scripts as writer or reader (the issue counted 191 before #985 and this change took the adapters into units' and
scripts' inboxes). Units' own writes include its handlers': the effects of weapons, orders, movement, economy, ai and
players on the unit record are units' writes now, made where the publisher's statement is, since every route that
writes what a later read in the tick needs is synchronous (the table of routes above). Each pair is served as follows:

| Writer to reader | Rows | Items | Phase | How it is served |
|---|---|---|---|---|
| economy to units | 5 | the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | players, units | A view: economy's state as of its last run; units' requests go through economy's events (`docs/economy-interface.md`, section 4). |
| features to units | 1 | the cells (`LoadTntMap` allocation) | units | A view: the cells' feature words, as of features' last run. |
| los_radar to units | 3 | the cells, `Unit::flags` (the sight bits), `bits_14281` | players, units | A view: los_radar's sight state, written by its own pass, which the scheduler runs in 3.1c's place (`docs/los-radar-interface.md`, section 4). |
| map to units | 4 | the cells, `Unit::cargoNext`, `cobStateFlags`, `flags` | units | A view: map's bucket and claim fields of the record and the cells, written by map's own code and its synchronous routes (`docs/map-interface.md`, section 3), as of map's last run. |
| map to scripts | 1 | `Unit::cobStateFlags` | units | The same view, read by a script's opcode. |
| movement to units | 3 | `Unit::motion`, `UnitMotion::lastMoveTick`, `speed` | units | A view: movement's motion, written by `UpdateMotion`, which the scheduler runs before units' tail, as 3.1c's `UpdateUnit` did. |
| scripts to orders, units | 2 | `Unit::script` | units | A view: the script pointer, written by units' life-cycle calls into scripts (`NewUnitScript`, `DeleteScript`), synchronous slots. |
| scripts to weapons | 12 | the model state (`ModelState::pos`, `root`, `pieces`, `animDirty`, `cacheDrawCount` and the rest), `Unit::script`, `Vec3::x` | projectiles, units | A view of the piece state, written by `RunScripts` in the unit's update after weapons' step, so weapons reads the previous tick's, as in 3.1c; the aim queries (`QueryScript`, `GetAimFromPosition`) are synchronous slots (`docs/scripts-interface.md`, section 2). |
| targeting to units | 2 | `Unit::netDirtyFlags` | projectiles, units | A view: the bit targeting's handlers set, as of the handler's run. |
| units to ai | 6 | `Unit::activateFlags`, `def`, `flags`, the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | players, units | A view of the unit record as of units' last run; ai's ticks run in the players phase, after the unit loop. |
| units to economy | 3 | the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | units | A view, as of units' last write. |
| units to los_radar | 11 | `Unit::activateFlags`, `def`, `flags`, `pos`, `yFraction`, `zFraction`, the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | players, units | A view of the record; units' moves reach los_radar as UPDATE_UNIT, REMOVE_UNIT, REVEAL_UNIT and ADD_EYEBALL, synchronous (`docs/los-radar-interface.md`, section 4). |
| units to map | 13 | `Unit::cargo`, `cargoNext`, `carrier`, `cell`, `cobStateFlags`, `def`, `flags`, `footprint`, `pos`, `zFraction`, the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | units | A view of the record; units' eight routes to map carry the placements and moves, each with its mode in `docs/map-interface.md`, section 4. |
| units to movement | 16 | `PathOrder::ref`, `Unit::carrier`, `cell`, `def`, `flags`, `footprint`, `heading`, `pitch`, `pos`, `transportPiece`, `yFraction`, `yWhole`, `zFraction`, the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | units | A view of the record, in 3.1c's order within a unit's update (head, weapons, scripts, body, orders, movement, tail). |
| units to orders | 27 | `Order::target`, `Unit::attacker`, `cargo`, `carrier`, `cell`, `cobStateFlags`, `def`, `flags`, `footprint`, `heading`, `health`, `lastAttackerSlot`, `netDirtyFlags`, `pos`, `script`, `speed`, `unitDefIndex`, `yFraction`, `yWhole`, `zFraction`, the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | features, players, projectiles, units | A view of the record, written by units' own code and its handlers, read by orders in its run, in 3.1c's order within the unit's update; units' events to orders (STOP, BE_CARRIED, STATUS_FLAGS) are synchronous (`docs/orders-interface.md`, section 4). |
| units to players | 7 | `Unit::def`, `flags`, `unitDefIndex`, `field_1df2`, the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | units | A view; KILL_UNITS and UNIT_DIED are synchronous and UNIT_CREATED queued (`docs/players-interface.md`, section 4). |
| units to scripts | 14 | the model state's `bitmap`, `Unit::bank`, `cobStateFlags`, `def`, `flags`, `heading`, `health`, `pitch`, `pos`, `script`, `speed`, `state`, `yFraction`, `zFraction` | units | A view of the record, read by a script's opcodes when it runs: in `RunScripts`, after weapons and before units' body, or at once when units' START is delivered (synchronous, section 8). |
| units to targeting | 13 | `Unit::def`, `flags`, `killCount`, `netDirtyFlags`, `pos`, `unitDefIndex`, `yFraction`, `zFraction`, the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | players, projectiles, units | A view of the record; units' RESET to targeting is queued, and targeting's searches are synchronous slots (`docs/targeting-interface.md`, section 4). |
| units to weapons | 20 | `Unit::activateFlags`, `def`, `flags`, `heading`, `health`, `killCount`, `owner`, `pitch`, `pos`, `script`, `state`, `yFraction`, `yWhole`, `zFraction`, the game record's unnamed ranges (`unknown_1b8e`, `unknown_1cd9`, `pad_1df4`) | features, projectiles, units | A view of the record, read by weapons' step (after units' head) and by its projectiles in their phase; the damage rules are weapons', the record units' (`docs/weapons-interface.md`, section 4). |
| weapons to units | 3 | `Unit::b3a`, `b56`, `field_1e` (weapons' slots) | units | A view of weapons' slots, written by `UpdateUnitWeapons` before units' body. |

**A view** is read directly from the state block as of the writer's last run; **in 3.1c's order within a unit's
update** means the scheduler's order is that of 3.1c's `UpdateUnit`; **synchronous** is a route of the table above.

## 5. State and the trace

Units keeps no private region, and since format 8 of the trace (#1024) the fields of the unit record that only units
reads are not hashed: its `WriteState` dumps them in record 54, `unitsState`, after the factory index it writes in
record 25 (`docs/trace-format.md`, section 8.14, hashed as before). The unit record is section 1 of the trace
(section 7.1) and the scripts' objects are section 23 (`docs/scripts-interface.md`, section 6). Units' events are
dumped in section 50 of the trace (section 8.34) and scripts' in section 51 (section 8.35), never hashed.

**The unit record, field by field.** Who writes each field of `Unit` and who reads it, by the access logs of every
scenario (`data/access/`; `seams` is the glue of `src/seams/`, an event's handler, and `network` and `platform` read
the record outside the simulation). Units writes a field through its own code and through its handlers, so a write
another module asks for by an event is units'. The owner of a field is the module that alone writes it, or the module
whose view state it is:

| Owner | Fields | Written by | Read by |
|---|---|---|---|
| units | `pos` (`xWhole`, `yWhole`, `zWhole` and the fractions), `heading`, `pitch`, `speed`, `cell`, `footprint`, `def`, `unitDefIndex`, `health`, `activateFlags`, `state`, `cargo`, `carrier`, `transportPiece`, `attacker`, `lastAttackerSlot`, `lastDamageType`, `killCount`, `owner`, `bank` | units | every simulation module, by the field (`pos`, `def` and `flags` by all of them; `health` by orders, scripts and weapons; `cargo` and `carrier` by map, movement, orders, players and scripts) |
| units | `head`, `healthPercent`, `prevHealthPercent`, `hoverBobPhase`, `recentlyDamagedTimer`, `postTransferHoldoff`, `unknown_10a`, `zBufferFlag`, `unfinishedInitZero` | units | units (`recentlyDamagedTimer` also los_radar, `postTransferHoldoff` also players) |
| units | `id`, `player`, `playerIndex` | written when the unit is created (no write in the logged ticks) | every module |
| units, with views of others | `flags` | units; los_radar's sight and jam bits (`UpdateSensorRadarAndCloak` and the jam visitors); map's footprint and yard bits (`ClaimFootprintCells`, `AddUnitToMap`, `RemoveUnitFromMap`, `SetYardOpen`, `ForceNeighborFootprintReclaim`); presentation's selection bits | every module |
| units, with map's view | `cargoNext` (map's bucket link: `SpatialBucket::UnlinkUnit`, `PrependUnit`, `SetOwner`, `ClearFootprintAndUnlink`), `cobStateFlags` (the yard bit: `SetYardOpen`) | units, map | map, orders, scripts, units |
| units, with targeting's view | `netDirtyFlags` (`SetWeaponTargetUnit`, `SetWeaponTargetPos`) | units, targeting | orders, targeting, units |
| units, with ai's view | `group` (`SetUnitSquad`) | units, ai | ai, orders |
| units, with economy's view | `extraction` (`EconomyUpdateMetalExtraction`) | economy | economy's handler (`seams`) |
| units, with los_radar's view | `workTime` | units (`UnitsApply`, for weapons' and orders' BUSY); los_radar (`UpdateSensorRadarAndCloak` sets it to the tick plus 90 when an enemy unit is within the minimum cloak distance: the decloak hold-off) | economy's handler (`seams`), for the cloak upkeep |
| scripts | `script` | scripts; units, when it creates and frees the script (`InitUnitScript`, `ApplyUnitDeath`) | game, network, orders, scripts, units, weapons |
| weapons | the weapon slots: `weapon`, `type`, `f2c`, `f48`, `f_1f`, `f_3b`, `f_57`, `b3a`, `b56`, `field_18`, `field_1e`, `field_8`, `lastTick`, `weapon2AimCob`, `unknown_14`, `unknown_1a`, `unknown_1c`, `unknown_24`, `unknown_30`, `unknown_4c` | weapons | ai, los_radar, orders, targeting, units, weapons (`docs/weapons-interface.md`, section 5) |
| targeting | `current`, `field_20`, `weapon2Target` | targeting | targeting |
| orders | `list`, `list2` | orders | ai, movement, orders (`docs/orders-interface.md`, section 5) |
| movement | `motion` | movement | every module that moves or reads a unit's motion (`docs/movement-interface.md`, section 5) |
| map | `spatialBucket` | map | map, movement, orders (`docs/map-interface.md`, section 3) |

`privatestate.py --check` holds units and scripts to this: units owns the record but the items of the owners above it
in its table (targeting, weapons, orders, movement, map and scripts), with the views of the rows above named in its
`also_written_by`; scripts owns its script objects, the piece records and `Unit::script`, which units also writes. Any
other write fails the check. `seamlink.py --pilot-seams` checks that no other simulation module calls units' or
scripts' writing slots but the engine and the callers each slot names (section 2, the mixed slots; targeting's
`GetWeaponTargetPos` calls scripts' `GetSweetSpot`, mixed, as weapons calls `GetAimFromPosition`), and that no query
slot writes state.

**What is hashed, and who reads it.** The rest of the record is read by other modules and stays in section 1 with
the unit. Among the fields units owns, `workTime` is read by economy (the cloak upkeep, and los_radar writes it),
`recentlyDamagedTimer` by los_radar, `postTransferHoldoff` by players and `owner` by weapons.

**What left the hash.** `head` (and its link in section 20, which no longer hashes it), `healthPercent`,
`prevHealthPercent`, `hoverBobPhase`, `unknown_10a` (the 4 bytes at +0x10a), `zBufferFlag` and `unfinishedInitZero`.
- Nothing reads `healthPercent`, `prevHealthPercent`, `unknown_10a` and `unfinishedInitZero`: units writes them and
  nothing reads them back. Scripts does not read `healthPercent`: the script value for the health percentage is
  computed from `health` and the maximum health. A wrong value of these four delivers nothing, so no run can
  diverge on it, and the record only reports it.
- Only presentation reads `zBufferFlag`. A wrong value changes only what is drawn, so the frame dumps judge it and
  nothing is hashed in its place.
- `head` and `hoverBobPhase` drive the unit's own state. With a wrong `hoverBobPhase`, units-transport first diverges at tick 3500, phase units, `units[3].bank` (the bank angle, 0x0146 against 0xfeba): at tick 3500 every unit's phase was moved by a quarter turn.

Every field keeps its bytes in the unit record, so no offset moves.

**Record 54, `unitsState`** (dumped, never hashed; `docs/trace-format.md`, section 8.38). For each live unit, one
record `unitsState.units[i]` with `head`, `hoverBobPhase`, `healthPercent`, `prevHealthPercent`,
`unfinishedInitZero`, `unknown_10a` and `zBufferFlag`. The reference build writes the same record itself.

`lastMoveTick` of the unit's motion is read here (the hover bob's damping, by the unit's age), and is hashed in
section 21 again for it (`docs/movement-interface.md`, section 5). Scripts' state is in
`docs/scripts-interface.md`, section 6.

**At load**: `LoadUnits` reads every unit record of the save through `LoadUnit`, which creates the unit (`CreateUnit`), sets its fields and flags from the record, and hands the rest to the owners: LOAD to economy, movement, targeting and los_radar, SET_SQUAD to ai, LOAD_PATH_LOCK_STAMP to map, and the slots `LoadOrders` (orders), `LoadScriptState` (scripts) and `LoadState` (weapons, once per weapon slot); a unit a record names as its child or attacker is loaded first.

**Load-time data.** The unit types (each one's definition, model and COB script), the movement classes and the build
menus are loaded once, by the load of a match (`docs/schedule.md`, "The load"), and no tick writes them. During play
one event changes them, and only one: RELOAD_TYPE (`UNITS_RELOAD_TYPE`, 397), which the engine's console publishes
between ticks (section 2). It is applied whole, in one `Apply`: every unit of the type is killed, then the type's
definition is read again from its unit file and its COB script is loaded again in place of the old one; its model is
kept. When the type is 0, or its unit file is gone, the units are killed and nothing is loaded again. No other event,
slot or step of the tick writes the unit types, so a module may hold them read-only from the end of the load to the
end of the match, outside that one event. The data stays on the heap, outside the block (#1081); the recording form
that carries the files the load read is #1094's.

## 6. Random draws, sim events and the unit loop

Units draws from the game logic stream (`RandomInt`) when it creates or loads a unit (the build angle and the hover
phase, in `CreateUnit` and `LoadUnit`) and in the fire-at-will search's pick; scripts draw in COB's `rand` opcode.
Both record sim events (speech, sounds, smoke) where 3.1c did. Neither changes where a draw or a record is made: the
draw is in the same function, and only the way to that function changed.

**The unit loop.** `SchedUnitsPhase` (`src/game/scheduler.cpp`) holds 3.1c's loop over the players and their unit
slots, and 3.1c's `UpdateUnit` is split around the other modules' steps, in its order:

```
deliver units; UpdateUnitHead(u);
if (the player is human or AI) { deliver weapons; UpdateUnitWeapons(u); }
deliver units;
if (u->script) { deliver scripts; RunScripts(u->script, 1); }
deliver units;
if (UpdateUnitBody(u)) { deliver orders; RunOrders(u); deliver orders; RunSecondaryOrders(u);
                         if (u->motion) { deliver movement; UpdateMotion(u); moved = 1 } }
deliver units; UpdateUnitTail(u, moved)
```

after which, for a network game, each player's `SendUnitStates` runs where 3.1c ran it, after that player's units.
No route into scripts is queued (section 4), so every script start runs at its publisher's statement, as in 3.1c, and
scripts' inbox is delivered only before `RunScripts`, as each step's module is. The three parts of units' update call no other module's intent: the head counts
the unit and tells a wind generator's script the wind (scripts' START, synchronous, for `SetDirection` and
`SetSpeed`); the body counts the unit's timers down, recomputes its
health percentage, applies water damage by publishing weapons' DAMAGE (synchronous, so weapons' damage rules run at
the statement, as in 3.1c) and adds the unit's own healing through units' `AddRepairProgress`; it reads `mission`'s
`GetMissionParams`, a query. The tail sets the unit's height after a move and keeps the pending kill: `KillUnit` when
`killPending` is set, where 3.1c had it, at the end of the unit's update. `docs/schedule.md` lists the loop's steps.

**The ordered rows.** The class `ordered` of `data/dependencies.csv` has 40 rows with units or scripts as writer or
reader (the issue counted 18 for units when it was filed, before scripts were their own module), on
`rng.parkMiller` and on `sim-event`. In every row, the draw or record stays where 3.1c made it, for one of three
reasons: the two runs are steps of the unit loop above, in 3.1c's order; the route between them is synchronous, so the
handler's draw or record is made at the publisher's statement; or the route is queued and its handler is delivered
before the next step that draws (orders' six and movement's five queued routes into units, decided by #985 with their
reasons in `docs/orders-interface.md` and `docs/movement-interface.md`, section 4; SFX_OCCUPY, whose handler can start
`setSFXoccupy`, is the last publish of `UpdateMotion`). The full gate is equal with them as they are, sound logs and
event digests included.

| Writer to reader | Kind (runs) | Phase | How it is kept in order |
|---|---|---|---|
| features to units | `sim-event` (3, 2, 2) | features, projectiles, units | Features' records and units' come from synchronous routes (features' FREE_STATE and INIT_STATE to scripts, units' PLACE to features), at the publisher's statement. |
| los_radar to units | `sim-event` (9) | players | Los_radar's pass and units' records in the players phase run in the scheduler's order, unchanged; units' routes to los_radar are synchronous. |
| map to units, map to scripts | `rng.parkMiller` (1, 2) | units | Map's draw (`PickRandomReclaimableResourcesInRadius`, a slot) and units' or a script's draw in the unit's update, in the loop's order; scripts' SET_YARD to map is synchronous (section 8). |
| scripts to map | `rng.parkMiller` (2) | units | The same, the other way round. |
| orders to units | `rng.parkMiller` (11), `sim-event` (41, 1) | units, projectiles | Orders' synchronous effects to units (REPAIR, FINISH_CONSTRUCTION, ATTACH_TO_PIECE, UNLINK, SET_STATE_BITS, CLEAR_NET_DIRTY) are delivered at orders' statement, and `CreateUnit` and `AddBuildProgress` are synchronous slots; the six queued effects write fields and are delivered before the next step. |
| orders to scripts | `rng.parkMiller` (12), `sim-event` (8) | units | Orders' START_SCRIPT is synchronous to scripts, whose handler publishes units' START, synchronous: the script's first slice runs at orders' statement. |
| scripts to orders | `rng.parkMiller` (10), `sim-event` (8) | units | `RunScripts` runs before orders' step in the unit's update; a script started by orders runs at once (above). |
| players to units, units to players | `sim-event` (4, 3) | units | Players' KILL_UNIT, SEND_UNIT and CAPTURE_UNIT to units and units' KILL_UNITS and UNIT_DIED to players are synchronous (`docs/players-interface.md`, section 8). |
| scripts to targeting, targeting to scripts | `rng.parkMiller` (3, 3) | units | Targeting's searches are synchronous slots and its CLEARED to scripts is synchronous; the script's draws are in `RunScripts` or at a synchronous START. |
| scripts to units, units to scripts | `rng.parkMiller` (4, 6), `sim-event` (6, 3) | units | Units' START to scripts and scripts' SCRIPT_VALUE and ATTACH_TO_PIECE to units are synchronous (section 8): a script started by units runs at units' statement, and its writes back to the unit are made at the opcode. |
| scripts to weapons, weapons to scripts | `rng.parkMiller` (8, 9), `sim-event` (7, 8) | units | Weapons' START_SCRIPT and DETACH_PIECE to scripts and scripts' AIM_DONE and EXPLODE_PIECE to weapons are synchronous (`docs/weapons-interface.md`, section 4); `RunScripts` runs after weapons' step, as in 3.1c. |
| units to ai | `sim-event` (2) | players | Ai's ticks in the players phase, after the unit loop, read units' view; units' SET_SQUAD to ai is synchronous, as before (`docs/ai-interface.md`, section 4). |
| units to features | `sim-event` (2, 2, 2) | features, projectiles, units | Units' PLACE to features is synchronous; a dying unit's wreck is placed at units' statement. |
| units to los_radar | `sim-event` (6) | players | Units' UPDATE_UNIT, REMOVE_UNIT, REVEAL_UNIT and ADD_EYEBALL to los_radar are synchronous. |
| units to orders | `rng.parkMiller` (13), `sim-event` (50) | units | Units' draws and records in its head, body and handlers, and orders' in `RunOrders`, in the loop's order; units' STOP, BE_CARRIED and STATUS_FLAGS to orders are synchronous, and ADD is published only outside the tick. |
| units to targeting | `rng.parkMiller` (1) | units | Units' fire-at-will pick (`FindBestTargetIfFireAtWill`, called by orders as targeting's slot) is made at the caller's statement; RESET to targeting, queued, carries no draw. |
| units to weapons, weapons to units | `rng.parkMiller` (3, 2), `sim-event` (17, 32; 19, 31) | units, projectiles | Weapons' ten routes to units are synchronous (`docs/weapons-interface.md`, section 4), so a death, a hit's record or a unit's script start is made at weapons' statement; the body's water DAMAGE to weapons is synchronous. |

No route had to be made synchronous for a draw or a record beyond those of section 8: START's divergence, in
orders-air-fights at tick 497, is the first that a queued route moved (a script's thread state, then its draws).

## 7. What stays inside the default

3.1c's types and limits are the default's, not the interface's: the unit record and its 0x118 bytes, the unit type
table and its TDF fields, the movement class definitions, the unit categories, the factory index, the unit references
and their lists, the saved game's unit records and the network's unit packets. An event carries a unit, a reference
and an attacker as block offsets, a player as its index and a position as three fixed-point words: no `Unit*`,
`UnitRef*`, `CobScript*` or `Callback*` in an event. The set-up and file slots (`LoadUnitTypes`, `FreeUnitTypes`,
`InitUnitCategories`, the download menus, `AllocateUnitMemory`, `FreeUnitMemory`, `LoadUnits`, `SaveUnits`) and the
unit reference slots stay in `UnitsSeam` as 3.1c's code. A unit's script and model state stay scripts' objects that
units creates and frees through scripts' slots (section 3).

## 8. What the gate decided

Every route new in this change was queued first, each on its own, with every other route as in `src/game/routes.cpp`
at the end of this change (the tier, with `--no-cache`, on the modern build at 735de8ef4). A route whose queueing
diverged stays synchronous. The divergences are against the reference, with the first tick, phase and field of the
comparison's report; where the report's first field is an `aiState` squad record that one side lacks, an artifact of
the comparison, the row gives the first field of the unit or script that differs instead. The routes that moved from
units' adapters to scripts' handlers kept their modes, and so did every route the issues before this one decided. The
engine's ATTACH_TO_PIECE is published only outside the tick (section 4).

The review of this change made seven queued routes synchronous by #952's rule (section 4): economy's EXTRACTOR to
scripts, orders' BUILD_DECAY, SET_STANCE, SET_ANGLES and SET_BUILD_STATE, and movement's ANGLES and SET_POSITION. Queued,
they were equal on the full suite, as their own issues found. Synchronous, the tier in both modes gave "gate tier passed:
38 scenarios equal at c9bc6e64164d", where the line names the commit the build's working tree was on.

| Route queued | Tier | First divergence |
|---|---|---|
| units START, to scripts | diverged | orders-air-fights, tick 497, world phase (the once-per-tick tables, hashed after `UpdateMeteors`): `units[6].script.activeCount` (reference 4, queued 3), and the script's `channels[2].pc` (0x83, 0x114) and statics. |
| scripts SCRIPT_VALUE, to units | diverged | orders-tier2-core, tick 43, units phase (`UpdateAllUnits`, 0x48ad30): `units[2].activateFlags` (reference 0x02, queued 0x00). |
| scripts ATTACH_TO_PIECE, to units | equal | none (synchronous by #952's rule, section 4) |
| scripts SET_YARD, to map | diverged | orders-air, tick 1024, units phase: `units[2].bit0` (reference 0x04, queued 0x0c). |
| engine ATTACH_TO_PIECE, to units | | queued: published only outside the tick (section 4) |

With the routes as they are, the tier passed: "gate tier passed: 38 scenarios equal at 2136ce89ce4b". The gate of this
change is the full `--check --both-modes`, since the trace format changes (sections 50 and 51); its line is in the
pull request.

**Inbox depths.** A build that counted every publish and delivery played all 89 scenarios (87 wrote their counts), with native modules, and every scenario stayed equal. Units'
inbox held at most 5 events (orders-air-repair, tick 757), from the queued routes of orders and movement; SFX_OCCUPY,
the busiest, was queued 7,268,389 times, and orders' CLEAR_NET_DIRTY, synchronous, was delivered 5,442,764 times.
Units took at most 958 deliveries in one tick (long-greenhaven-8p, tick 52800). Scripts' inbox held at most 1 event
(units-transport-ship, tick 133): economy's EXTRACTOR, then its only queued route, queued 597 times in the tick; scripts took
at most 422 deliveries in one tick (edges-economy-targeting, tick 245), and units' START was published 21,305 times
in the tick and 1,883 outside it. ATTACH_TO_PIECE, from the engine and from scripts, was published 5 times in the tick
(synchronous) and 23 times outside it. Deliveries nested at most 6 deep, counting every module (console-units, tick
150), as before this change. The counts were taken before the review made EXTRACTOR and six routes into units synchronous (above);
since then scripts' inbox holds nothing in the tick, and units' inbox holds only BUSY, SET_FLAGS, MOVED, MOVE_RATE and
SFX_OCCUPY.
