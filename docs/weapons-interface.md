# The weapons interface

Weapons sits behind `interfaces/weapons.h` (version 3.0), the eighth module on the scheduler after economy
(`docs/economy-interface.md`), features (`docs/features-interface.md`), targeting (`docs/targeting-interface.md`),
los_radar (`docs/los-radar-interface.md`), map (`docs/map-interface.md`), players (`docs/players-interface.md`) and ai
(`docs/ai-interface.md`) (#984). It is the weapons, projectiles and explosions of the simulation, and the damage rules
for every kind of damage to a unit: the unit's weapons aiming and firing in its update, the projectile pass, the
explosion pass, the debris and the exploded pieces, an area's damage, and what a unit takes when it is hit, repaired,
self-destructed or killed. 3.1c's code is its default implementation. This page is the inventory the interface was cut
from:
- what other subsystems did to weapons' state in 3.1c, and what each became;
- what weapons did to others, and what each became;
- what weapons reads, and what others read of it;
- how the order of random draws and sim events is kept;
- what the trace hashes, and what it only dumps.

## 1. The model

The model is the human's of 2026-11-12, as `docs/economy-interface.md`, section 1, gives it: reads as of each module's
last run, outboxes routed through the table of `src/game/routes.cpp` in 3.1c's order, inboxes consumed before each
run and drained at each phase boundary, synchronous slots that deliver the module's inbox first, events published
outside the tick delivered at once (#970), and events as plain data. This issue changes none of its rules. Event
kinds are unique across interfaces: economy uses 0 to 4 and 16 to 18, features 32 to 35 and 48 to 51, wind_meteors 64
to 67 and 80, targeting 96 to 100 and 112, los_radar 128 to 136, map 160 to 168 and 176, players 192 to 196 and 208 to
210, ai 224, 226 to 229 and 240, 241, 243, 245 and 246, weapons 256 to 267, 269 and 272 to 283 (its range is 256 to 287).

The human ruled on 2026-11-17 that **weapons owns the damage rules and units owns the record**: weapons computes the
damage (the armour, the kill count, the network packet, the effects that tell ai, orders and units) and publishes the
writes to the unit record as effects (section 3).

Weapons' place in it:
- **Events into weapons:** DAMAGE, REMOTE_DAMAGE, DETONATE, RELEASE, CLAIM, INIT_SLOTS, REMOVE_PROJECTILES,
  AIM_DONE, EXPLODE_PIECE, AREA_DAMAGE, SPAWN, SET_STOCKPILES and ADD_STOCKPILE, published by units, orders, scripts,
  features, wind_meteors and the engine (the network). Weapons' handler (`WeaponsInbox`,
  `src/seams/weapons_world.cpp`) passes each to `Apply`, and records it with its outcome in the trace's dump
  (section 5). Features' AREA_DAMAGE and wind_meteors' SPAWN reach it through `WeaponsOnFeaturesAreaDamage` and
  `WeaponsOnMeteorSpawn`, which build the event and hand it to the same handler; their routes in `src/game/routes.cpp`
  are as they were (AREA_DAMAGE synchronous, SPAWN queued).
- **Routes:** of the fifteen routes into weapons, fourteen are synchronous (seven because the gate diverged with them
  queued, six by #952's rule, and features' AREA_DAMAGE as it was). Only wind_meteors' SPAWN stays queued (the saved game's slots no longer go through an event: the slot `LoadState`, below). Of weapons' effects, START_SCRIPT,
  DETACH_PIECE and HEALTH are synchronous because the gate diverged with them queued, and MARK_DAMAGED,
  DAMAGE_RECORD, PARALYZE, SHOT_FIRED, COMMAND_SHOT_FIRED, OUT_OF_REACH and BUSY by #952's rule; ATTACKED (four routes), HIT, DAMAGE to features and CLEAR to
  targeting are as they were (sections 4 and 8).
- **Runs:** `UpdateProjectiles`, `UpdateExplosions` and `UpdateUnitWeapons`. The scheduler calls the first two as the
  projectiles and explosions phases (`SchedProjectilesPhase`, `SchedExplosionsPhase`, `src/game/scheduler.cpp`) and
  `UpdateUnitWeapons` as a step of the units pass, for a unit of an active player of type 1 or 2, each with weapons'
  inbox delivered first. Units 3.0 split `UpdateUnit` around it into `UpdateUnitHead` and `UpdateUnitBody`, as #965
  split `UpdatePlayerHead` from `UpdatePlayerBody`, and the scheduler delivers units' inbox before each.
- **Synchronous calls out:** `scripts.QueryScript` and `scripts.GetAimFromPosition` (mixed, section 3),
  `economy.Spend`, `targeting.GetWeaponTargetPos` and `targeting.FindWeaponTarget`, which stay synchronous slots of
  their owners.
- **Queries:** `FindWeaponByName`, `WeaponCanReachUnit`, `FindTargetableProjectile` and `WeaponCanReachPos`, as of
  weapons' last apply, and the view read `GetWeaponRange`.
- **Effects out of weapons:** the damage effects ATTACKED, MARK_DAMAGED, DAMAGE_RECORD, HEALTH and PARALYZE, and the
  effects HIT, START_SCRIPT, DETACH_PIECE, SHOT_FIRED, COMMAND_SHOT_FIRED, OUT_OF_REACH and BUSY, to units, orders, ai and scripts, besides DAMAGE to
  features and CLEAR to targeting. Units and scripts are not on the model yet, so their handlers
  (`src/units/weapons_effects.cpp`, `src/scripts/weapons_effects.cpp`) are adapters that hold 3.1c's writes.
- **Module ids:** weapons keeps `SCHED_WEAPONS = 6`, which its outbox already used.

## 2. Events into weapons

**The damage kinds** are a group of their own in `interfaces/weapons.h` (`WeaponsDamageEventKind`,
`WeaponsDamageEffectKind`). `DAMAGE` is the entry of every kind of damage to a unit, not only a weapon's, and the
group keeps a later split of the projectiles from the damage rules cheap:

| 3.1c site | 3.1c call | Now |
|---|---|---|
| a weapon's hit, a projectile's or an explosion's (`weapons.cpp`) | `DamageUnit` (kind 1, paralyser 2) | weapons' own code, in the same pass: `DamageUnit` and its effects, called directly |
| self-destruct and order damage (`unit_orders.cpp`, `vtol_orders.cpp`) | `DamageUnit` | DAMAGE from orders |
| water damage (`UpdateUnitBody`, `unit_position.cpp`) | `DamageUnit` (kind 11) | DAMAGE from units |
| the console's kills and a unit's death by a player's kill (`unit_commands.cpp`, `players_effects.cpp`) | `DamageUnit` | DAMAGE from units |
| a dying transport's cargo (`ApplyUnitDeath`, `units.cpp`) | `DamageUnit` | DAMAGE from units |
| build progress and repair (`AddBuildProgress`, `AddRepairProgress`, `control_panel.cpp`) | `DamageUnit` | DAMAGE from units (kinds 9 and 10) |
| the network: a damage packet arrives (`HandleNetPackets`, `net_game.cpp`) | `ApplyUnitDamage` | REMOTE_DAMAGE from the engine |
| a damaged unit (`ApplyUnitDamage`) | the writes to the unit record, `ai.ReactToAttack`, the script starts | the effects MARK_DAMAGED, ATTACKED, DAMAGE_RECORD, HEALTH, PARALYZE and START_SCRIPT (section 3) |

DAMAGE carries a `WeaponsDamage`: the `source` (a block offset, 0 for none), the `amount`, the `damageKind` (1 a weapon, 2
a paralyser, 10 a repair that heals, 11 the water, others the cause of a death) and the `direction` (a 16-bit heading). REMOTE_DAMAGE carries what
another machine already computed and announced, and skips the armour and the network announcement.

**The other events:**

| 3.1c site | 3.1c call | Event |
|---|---|---|
| units: `KillUnit` with a self-destruct, a player's kill (`UnitsOnPlayersKillUnit`) | `DetonateUnitWeapon` | DETONATE from units, with `WeaponsDetonate::selfDestruct` 1 for the self-destruct weapon |
| orders: the order's start and end (`unit_orders.cpp`, `vtol_orders.cpp`, `order_queue.cpp`) | `Unit::ClaimWeapons` | CLAIM from orders, with `WeaponsClaim::weapon` the weapon (0 to 2) or 3 for all |
| orders and units: the order's end, a unit's creation (`InitUnitFromType`) | `Unit::ReleaseWeapons` | RELEASE from orders and from units |
| units: a unit is built or created from a packet (`units.cpp`) | `InitUnitWeaponSlots` | INIT_SLOTS from units |
| units: a unit dies (`ApplyUnitDeath`) | `RemoveUnitProjectiles` | REMOVE_PROJECTILES from units |
| units: a unit captured by another player (`UnitsOnPlayersCaptureUnit`) | the stockpiles copied into the new unit's slots | SET_STOCKPILES from units |
| units: a saved game loads (`unit_save.cpp`) | the slot's fields restored, but its target pair | the slot `LoadState(unit, slot, state)`, called by units for each slot while a saved game loads, outside the tick, with a `WeaponsSlotState`; the target pair is targeting's, restored by targeting's LOAD (`docs/targeting-interface.md`, section 2) |
| orders: a stockpile weapon's build order finishes a round (`BuildWeaponOrder`, `unit_orders.cpp`) | the slot's `stockpile` raised by one | ADD_STOCKPILE from orders, with `WeaponsAddStockpile::weapon` the weapon (0 to 2) |
| scripts: an aim thread ends, or the aim script is missing (`cob.cpp`) | `WeaponAimCobComplete` | AIM_DONE from scripts |
| scripts: the explode opcode (`script_explode.cpp`) | `CobScriptExplodePiece` | EXPLODE_PIECE from scripts |
| features: a burning feature (`SpreadFire`), by features' effect AREA_DAMAGE | `ApplyAreaDamageAt` | AREA_DAMAGE, through `WeaponsOnFeaturesAreaDamage` |
| wind_meteors: a meteor appears, by its effect SPAWN | `SpawnProjectile` | SPAWN, through `WeaponsOnMeteorSpawn` |

**Every slot of 1.6.** The 23 slots of weapons 1.6, and what each became in 2.0:

| 1.6 slot | 2.0 |
|---|---|
| `DamageUnit` | event: DAMAGE, through the new slot `Apply` |
| `DetonateUnitWeapon`, `InitUnitWeaponSlots`, `RemoveUnitProjectiles`, `WeaponAimCobComplete`, `CobScriptExplodePiece`, `ApplyAreaDamageAt`, `SpawnProjectile` | events: DETONATE, INIT_SLOTS, REMOVE_PROJECTILES, AIM_DONE, EXPLODE_PIECE, AREA_DAMAGE, SPAWN |
| (not slots: `Unit::ClaimWeapons`, `Unit::ReleaseWeapons`, `ApplyUnitDamage` from the network) | events: CLAIM, RELEASE, REMOTE_DAMAGE |
| `UpdateProjectiles`, `UpdateExplosions`, `UpdateUnitWeapons` | runs, called by the scheduler |
| `FindWeaponByName`, `WeaponCanReachUnit`, `FindTargetableProjectile`, `WeaponCanReachPos` | queries, unchanged |
| `GetWeaponRange` | view read, unchanged |
| `FindFeatureAtPos` | moved to features 3.1: it reads only features' and los_radar's cells |
| `AllocWeaponArray`, `FreeWeaponArray`, `LoadWeaponTypes`, `FreeWeaponTypes`, `InitExplosions`, `FreeExplosions` | set-up slots, outside the tick, unchanged |

Weapons 2.0 has no synchronous slot of its own: every caller that needed a same-statement answer in 3.1c has none
now (below). `WriteState` is new (section 5).

**Answers.** One caller used a value of an event: `SpawnProjectile`'s, whether the table of projectiles had room; the
caller (wind_meteors) ignored it, and `Apply` returns it as the outcome. `DamageUnit`, `DetonateUnitWeapon`,
`ReleaseWeapons`, `ClaimWeapons`, `InitUnitWeaponSlots`, `RemoveUnitProjectiles`, `WeaponAimCobComplete`,
`CobScriptExplodePiece` and `ApplyAreaDamageAt` returned nothing, so none of them is synchronous by its contract:
every same-statement order is a route's (section 4).

**Outcomes.** `Apply` returns the outcome, recorded with the event in the trace's dump (section 47) and returned to no
one: 1 for every kind, and 0 for SPAWN when the table of projectiles is full. `Apply` first rejects, with outcome 0
and no change, an event naming a null unit (all but SPAWN, AREA_DAMAGE and AIM_DONE), a RELEASE or CLAIM outside
weapon 0 to 3, an ADD_STOCKPILE outside 0 to 2, an AIM_DONE with no callback, a SPAWN or AREA_DAMAGE
naming a weapon outside the table (SPAWN may name -1), and an unknown kind.

**Calls outside the interface.** `docs/schedule.md` listed, on the damage path of `DamageUnit`, `DetonateUnitWeapon`,
`UpdateProjectiles` and `UpdateUnitWeapons`, a call into orders outside its interface (after `ai.ReactToAttack`, before
`orders.MissionTypeFromName`), and in `UpdateUnitWeapons` a call into units outside its interface (after
`scripts.GetAimFromPosition`). The first was the paralysis branch, which compared the first order's kind
(`MissionType::operator==`, `src/orders/mission_type.h`) and appended the order: it is now PARALYZE, the order's own
code in its handler (`OrdersOnWeaponsParalyze`, `src/orders/weapons_effects.cpp`). The second was the write to the unit
record (`MarkRecentlyDamaged` and the fields of section 3), now units' effects. The calls into weapons outside its
interface after `map.Apply` were the AREA_DAMAGE adapter, which now builds an event for `Apply`.

## 3. Effects out of weapons

| 3.1c call | Effect | Subscriber's handler |
|---|---|---|
| `units.MarkRecentlyDamaged` in `ApplyUnitDamage` | MARK_DAMAGED | `UnitsOnWeaponsMarkDamaged`: `units.MarkRecentlyDamaged` |
| `ai.ReactToAttack` and the notices, return fire and speech | ATTACKED | four routes in 3.1c's order: `UnitsOnWeaponsAttacked`, `AiOnWeaponsAttacked`, `OrdersOnWeaponsAttacked`, `UnitsOnWeaponsAttackedSpeech` (as #991 left them) |
| the writes of the last attacker and damage kind | DAMAGE_RECORD | `UnitsOnWeaponsDamageRecord`: `lastDamageType`, and when `WeaponsDamageRecord::attacker` is not 0, `lastAttackerSlot` and `attacker` |
| the write of `health` and the dying flag | HEALTH | `UnitsOnWeaponsHealth`: `health`, and flag 0x4000 when `WeaponsHealth::dying` is 1 |
| the paralysis order (`orders.MissionTypeFromName`, `NewOrder`, `AppendOrder`, the first order's `ticks`) | PARALYZE | `OrdersOnWeaponsParalyze`: adds `WeaponsParalyze::ticks` ticks to a first order that is a paralysis, else appends one |
| a shot hits (`ApplyWeaponHit`, `DetonateProjectile`, `ApplyAreaDamage`) | HIT | `OrdersOnWeaponsHit`: the `netDirtyFlags` split (as #991 left it) |
| `scripts.StartScript`, `StartScriptWithArgs` (fire, aim, rock, hit-by-weapon and take-damage scripts) | START_SCRIPT | `ScriptsOnWeaponsStartScript`: the script's start, with the arguments when `WeaponsStartScript::withArgs` is 1; the `WeaponsScript` the payload names becomes the script's name |
| `scripts.PoseUnitModel`, the piece's hide (`DetachPiece` of an exploded piece) | DETACH_PIECE | `ScriptsOnWeaponsDetachPiece` |
| the notice bits written on a unit (`netDirtyFlags`) | SHOT_FIRED, COMMAND_SHOT_FIRED, OUT_OF_REACH | `UnitsOnWeaponsNotice`: ORs bit 0x400 (SHOT_FIRED), 0x800 (COMMAND_SHOT_FIRED) or 0x1000 (OUT_OF_REACH) into `netDirtyFlags`; each kind is the header alone |
| the unit's `workTime` set for the idle animation | BUSY | `UnitsOnWeaponsBusy`: `workTime` becomes `WeaponsBusy::until` |
| `features.DamageFeature` | DAMAGE (features') | features' `Apply`, as before |
| `targeting.ClearWeaponTarget` | CLEAR (targeting's) | targeting's `Apply`, as before |

The handlers are the adapters of units, orders and scripts, which hold 3.1c's writes until #986 puts units and scripts
on the scheduler.

**Which unit-record fields are weapons' and which are units'.** The rule is the human's of 2026-11-17: weapons owns
the rules, units owns the record.

| Field of `Unit` | Whose | Written how |
|---|---|---|
| the three weapon slots, `Unit::weapons[0]` to `[2]` (`UnitWeaponSlot`, `src/weapons/unit_weapon_slot.h`) but the target pair: `aimCob` (`ppVtbl`, `nAimCobDone`), `weapon`, `muzzleAimFromDeltaZ`, `reloadTimer`, `fireHeading`, `firePitch`, `stockpile`, `flags`; also read by their union names `weapon` (slot 0's weapon), `f_1f` (slot 0's flags), `f2c` and `f_3b` (slot 1's weapon and flags), `f48` and `f_57` (slot 2's weapon and flags) | weapons' view state, as map's bucket fields are in the same record (`docs/map-interface.md`, section 3) | by weapons' own events (INIT_SLOTS, RELEASE, CLAIM, SET_STOCKPILES, ADD_STOCKPILE, AIM_DONE), the slot `LoadState` and runs (`UpdateUnitWeapons`) |
| the target pair of each slot, `aimTargetXOrUnitId` and `aimTargetZOrUnitSentinel`, the slot's first four bytes | targeting's (`docs/targeting-interface.md`, section 5) | by targeting's events (SET_UNIT, SET_POS, CLEAR, RESET, LOAD) and its slot `GetWeaponTargetPos`; a saved game's pair by LOAD, which units publishes as the save loads |
| `health`, `flags` (0x4000 dying), `attacker`, `lastAttackerSlot`, `lastDamageType` | units' record | by the effects HEALTH and DAMAGE_RECORD, units' handlers; `health` has other writers (self-repair, build progress, orders) |
| `recentlyDamagedTimer` | units' record | by MARK_DAMAGED |
| `workTime` | units' record | by BUSY (orders write it too) |
| the notice bits of `netDirtyFlags` (bits 10 to 14, mask 0x7c00) | units' record | by SHOT_FIRED, COMMAND_SHOT_FIRED and OUT_OF_REACH; targeting's SET_UNIT and SET_POS clear them, and HIT sets bits 13 and 14 for orders |
| the first order's `ticks`, and the paralysis order | orders' record | by PARALYZE |

**`scripts.QueryScript` and `scripts.GetAimFromPosition` stay synchronous slots of scripts (#986).** They are mixed:
they run a unit's script, which can draw from the game logic stream and record sim events, and weapons uses the answer
in the same statement (the aim target, the position a weapon fires from). Nothing about them changes in this
issue.

**Writes weapons keeps.** After this change weapons writes no field of the unit record outside its slots: the damage
rules run in `src/weapons/damage.cpp` (`DamageUnit`, `ApplyUnitDamage`, `ParalyzeUnit`), and what they decide goes out
as the effects above. It writes its own state: the projectile table, the explosions, the debris and the exploded pieces
(section 5).

**Payloads.** Every event and effect starts with `WeaponsEventHeader`: `kind` and `unit` (the unit it is about, a block offset; 0 for SPAWN, AREA_DAMAGE and AIM_DONE, which name none). Each kind has the payload of `interfaces/weapons.h` (version 3.0) that starts with it, or is the header alone. `Apply(event, size)` refuses an event smaller than its kind's payload, with outcome 0 and no change, besides the refusals of section 2.

| Kind | Payload | Fields after the header |
|---|---|---|
| DAMAGE 256, REMOTE_DAMAGE 257 | `WeaponsDamage` | `source` (the unit that dealt it, a block offset, 0 for none), `amount`, `damageKind` (1 a weapon, 2 a paralyser, 10 a repair that heals, 11 the water, others the cause of a death), `direction` (a 16-bit heading) |
| DETONATE 258 | `WeaponsDetonate` | `selfDestruct`: 1 for the unit's self-destruct weapon, 0 for the weapon it explodes as |
| RELEASE 259 | `WeaponsRelease` | `weapon`: 0 to 2, or 3 for all three |
| CLAIM 260 | `WeaponsClaim` | `weapon`: 0 to 2, or 3 for all three |
| INIT_SLOTS 261, REMOVE_PROJECTILES 262 | the header alone | none |
| AIM_DONE 263 | `WeaponsAimDone` | `callback` (the aim callback START_SCRIPT passed, a block offset), `result` (what the aim script returned) |
| EXPLODE_PIECE 264 | `WeaponsExplodePiece` | `piece`, `flags` (the explode flags of the script's opcode) |
| AREA_DAMAGE 265 | `WeaponsAreaDamage` | `weapon` (its index in the table of weapons), `pos[3]` (the centre, world coordinates with 16 fractional bits) |
| SPAWN 266 | `WeaponsSpawn` | `weapon` (-1 for none), `pos[3]`, `vel[3]` (per tick, 16 fractional bits), `announce` (1 when announced to the other machines) |
| SET_STOCKPILES 267 | `WeaponsSetStockpiles` | `stockpiles[3]`: the stockpile of each of the unit's three weapons |
| ADD_STOCKPILE 269 | `WeaponsAddStockpile` | `weapon`: 0 to 2 |
| ATTACKED 272 | `WeaponsAttacked` | `attacker` (a block offset, 0 when no unit did), `amount` |
| HIT 273 | `WeaponsHit` | `enemyDamage` (dealt to units of other players), `friendlyDamage` (dealt to units of the unit's own player) |
| MARK_DAMAGED 274 | the header alone | none |
| DAMAGE_RECORD 275 | `WeaponsDamageRecord` | `damageKind`, `attacker` (when not 0, it and its player become the unit's last attacker) |
| HEALTH 276 | `WeaponsHealth` | `health` (the new health), `dying` (1 when the unit is marked to die) |
| PARALYZE 277 | `WeaponsParalyze` | `ticks`: the ticks of paralysis to add |
| START_SCRIPT 278 | `WeaponsStartScript` | `script` (a `WeaponsScript`), `callback` (the aim callback it returns to, a block offset, 0 for none), `update` (the script's flag update), `withArgs`, `count`, `args[4]` |
| DETACH_PIECE 279 | `WeaponsDetachPiece` | `piece`, `pose` (1 when the model is first posed from its angles and animation) |
| SHOT_FIRED 280, COMMAND_SHOT_FIRED 282, OUT_OF_REACH 283 | the header alone | none: these three are the notices its orders wait on, 0x400, 0x800 and 0x1000 of the unit's `netDirtyFlags` |
| BUSY 281 | `WeaponsBusy` | `until`: the tick the unit is busy until |

**`WeaponsScript`.** START_SCRIPT names the script by an enum, not by a name: NONE 0 (a script of an empty name, as a fire script's index 3 does), FIRE_PRIMARY 1, FIRE_SECONDARY 2, FIRE_TERTIARY 3 (the first, second and third weapon fires), AIM_PRIMARY 4, AIM_SECONDARY 5, AIM_TERTIARY 6, ROCK_UNIT 7, SET_MAX_RELOAD_TIME 8 (the longest reload of the unit's weapons, in milliseconds), HIT_BY_WEAPON 9 (the direction a hit came from) and TAKE_DAMAGE 10 (the share of its health a hit took). The scripts side maps each back to the name of the unit script it starts.

**`LoadState`.** `LoadState(unit, slot, state)` replaces the LOAD_SLOT event of weapons 2.0. Units calls it for each of a unit's slots while a saved game loads, outside the tick, with a `WeaponsSlotState`: `aimDone`, `typeByte` (a byte of the weapon's type that the save keeps), `aimOffset`, `reload`, `heading`, `pitch`, `stockpile` and `flags` (the slot's flag bits 0, 1, 2 to 3 and 4). It sets the slot's fields as the save held them and writes nothing else; the slot's target pair is restored by targeting's LOAD.

**NOTICE.** Weapons 2.0 had one NOTICE effect whose value held the bits to set in a unit's `netDirtyFlags`. The three bits are now kinds of their own, SHOT_FIRED (0x400), COMMAND_SHOT_FIRED (0x800) and OUT_OF_REACH (0x1000), each the header alone, and the units side ORs the bit of the kind it receives.

## 4. Views and routes

**What weapons reads:** a unit's position, heading, pitch, velocity, flags, health, kill count, type and owner; its
orders' target positions; its script and its model's pieces (as scripts' piece queries and views); the players'
resources (economy's); the cells' heights and features (map's `GetCellMeanHeight`, features' `FindFeatureAtPos`), as of
each module's last run.

**What others read of weapons:**
- **Views, in the state block, hashed:** the weapon slots of the unit record (section 3), read by targeting, orders,
  ai and units, and the projectile table, read by ai (`Projectile::attacker`), los_radar (`Projectile::position`) and
  map (radar capture). `GetWeaponRange` is a view read, from orders and targeting.
- **Queries:** `FindWeaponByName`, `WeaponCanReachUnit`, `FindTargetableProjectile` and `WeaponCanReachPos`.

**As of weapons' last run.** By #952's rule a query needs a delivery first only when an event that can be pending
writes what it reads. Weapons' queries read the weapon slots, the projectile table and the definitions. The only route
into weapons that is queued is SPAWN (wind_meteors'
effect), which writes the projectile table, and nothing queries that table's new entry before the phase boundary
delivers it.

**The routes.** Every route, with its reason:

| Route | Now | Why |
|---|---|---|
| orders DAMAGE | synchronous | The gate (section 8). |
| orders RELEASE | synchronous | The gate (section 8). |
| orders CLAIM | synchronous | The gate (section 8). |
| orders ADD_STOCKPILE | synchronous | #952's rule. Queued it stayed equal on the two scenarios with a stockpile weapon (the tier has none), but it can be pending while a later read in the same phase needs the stockpile: a capture later in the units phase copies the unit's stockpiles into the new unit (players' CAPTURE_UNIT, units' handler publishes SET_STOCKPILES with them), and the network's unit state packet sends them. |
| units DAMAGE | synchronous | The gate (section 8): the traces were equal, the sim event digest diverged. The route also carries the build progress and repair damage of `AddBuildProgress` and `AddRepairProgress`, which units' functions publish on units' outbox. |
| units DETONATE | synchronous | The gate (section 8). |
| units RELEASE | synchronous | #952's rule. Queued it stayed equal on the tier, but `InitUnitFromType` releases each slot of a unit created in the units phase (a build), which is updated later in the same phase when its slot follows its builder's, and `UpdateUnitWeapons` reads the released slots. Queued, the release would also land after INIT_SLOTS (synchronous), reversing the two writes to the same slots. |
| units INIT_SLOTS | synchronous | The gate (section 8). |
| units REMOVE_PROJECTILES | synchronous | #952's rule. Queued it stayed equal on the tier, but a unit dies in the units phase (`ApplyUnitDeath`), and later units' `UpdateUnitWeapons` read the projectile table (`FindTargetableProjectile`, an anti-missile weapon's target) before the phase boundary. |
| units SET_STOCKPILES | synchronous | #952's rule. Queued it stayed equal on the tier, but the captured unit's (or copy's) stockpiles are read by its own `UpdateUnitWeapons` (a stockpile weapon fires only with stock) and by the network's unit-state packet, later in the same phase. |
| units `LoadState` | slot | Called directly, not an event: it replaces LOAD_SLOT, which was queued and equal. Units calls it for each slot while a saved game loads (`unit_save.cpp`), outside the tick. |
| engine REMOTE_DAMAGE | synchronous | #952's rule. Queued it stayed equal on the tier, which has no network path: the net phase (`HandleNetPackets`) applies packets in order, and the packets after it in the same phase read the damaged unit (a unit's state packet, a kill packet). The same argument as the engine's KILL_UNITS and GIVE_UNIT rows of `docs/players-interface.md`. |
| scripts AIM_DONE | synchronous | #952's rule. Queued it stayed equal on the tier, but a missing aim script answers at once, inside weapons' own START_SCRIPT in `UpdateUnitWeapons` (`StartScriptWithArgsByIndex`, `cob.cpp`), and the same update then reads `aimCob.nAimCobDone` in the fire step before the phase ends. |
| scripts EXPLODE_PIECE | synchronous | The gate (section 8). |
| features AREA_DAMAGE | synchronous | As before (the gate, `docs/features-interface.md`, section 8). |
| wind_meteors SPAWN | queued | As before (`docs/wind-meteors-interface.md`, section 3). |
| weapons MARK_DAMAGED, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but it writes `recentlyDamagedTimer`, which units reads later in the units phase (`units.cpp`, `unit_position.cpp`, `radar_capture.cpp`) after damage dealt in that phase (orders' self-destruct, build decay, melee and instant weapons). It is also one of the damage group, whose effects keep 3.1c's order only when all are delivered at publish. |
| weapons DAMAGE_RECORD, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but `lastAttackerSlot` and `lastDamageType` are read by orders (`unit_orders.cpp`, `vtol_orders.cpp`: return fire, guard) and by units later in the same phase; the same damage-group argument. |
| weapons HEALTH, to units | synchronous | The gate (section 8). |
| weapons PARALYZE, to orders | synchronous | #952's rule. Queued it stayed equal on the tier, but the paralysis order is read by the unit's own `RunOrders` later in the same units phase (weapons run before orders in a unit's update). |
| weapons START_SCRIPT, to scripts | synchronous | The gate (section 8). |
| weapons DETACH_PIECE, to scripts | synchronous | The gate (section 8). |
| weapons SHOT_FIRED, COMMAND_SHOT_FIRED and OUT_OF_REACH, to units | synchronous | #952's rule, as NOTICE had it. Queued it stayed equal on the tier, but the notice bits (`netDirtyFlags` 10 to 14) are written on a target in `UpdateUnitWeapons` and read by the target's targeting and orders and by the network's state packet later in the same phase. |
| weapons BUSY, to units | synchronous | #952's rule. Queued it stayed equal on the tier, but it writes `workTime`, which the unit's orders read (nine reads in `unit_orders.cpp`) later in the same unit update. |
| weapons ATTACKED (four routes), HIT | synchronous | As #991 left them (`docs/ai-interface.md`, section 8). |
| weapons DAMAGE to features | synchronous | As before (the gate, `docs/features-interface.md`, section 8). |
| weapons CLEAR to targeting | synchronous | As before (the gate, `docs/targeting-interface.md`, section 8). |

**The immediate rows of `docs/schedule.md`.** `data/dependencies.csv` has 74 rows of class `immediate` with weapons as
writer or reader, from access logs recorded on this change. The writes of units' handlers of weapons' effects (health,
flags, attacker, the damage record, the notice bits) are units' in them. Each pair is served as follows:

| Writer → reader | Items | Phase | How it is served |
|---|---|---|---|
| economy → weapons | `unknown_1b8e`, `unknown_1cd9`, `pad_1df4` (the `Player` records' resources) | units | A view: economy's view state, as of its last apply, read directly (`docs/economy-interface.md`, section 4). Weapons' spends go through economy's synchronous `Spend`. |
| features → weapons | `FeatureAnimEntry::bytes_8`, `bytes_10`, the cells (`LoadTntMap` allocation) | features, projectiles, units | A view: features' spots and the cells' feature fields, which features writes in its own phase; every route of features is synchronous but its map loader's, so nothing pends. |
| los_radar, map → weapons | the cells (`LoadTntMap` allocation), `Unit::flags` (map's) | units | A view: the cells, as of the writer's last run, and the unit's flags map writes in its own pass, which the scheduler runs before weapons' step. |
| movement, units → weapons | `Unit::pos`, `heading`, `pitch`, `yFraction`, `zFraction`, `yWhole`, `flags`, `UnitMotion::speed`, `velocity`, `Unit::motion` | units | A view: the unit's motion, written by `UpdateMotion` and units' body and read by `UpdateUnitWeapons` in the unit's update, in 3.1c's order (head, weapons, body). |
| orders → weapons | `Unit::flags`, `health` | units | A view of the unit record, written by orders in a unit's update (mission-kill-of-type, tick 182; orders-tier2-core, tick 615); orders run after weapons in the unit's update, so weapons reads the previous tick's, as in 3.1c. |
| scripts → weapons | `ModelState::*`, `Unit::script`, `Vec3::x` | units, projectiles | A view of the piece state, through scripts' piece queries; the script's own writes happen in `RunScripts`, in units' body, so weapons reads the previous tick's. |
| units → weapons | `Unit::def`, `activateFlags`, `flags`, `health`, `killCount`, `owner`, `state`, `script`, `pos`, `heading`, `pitch`, `motion`, and the `Player` records | features, projectiles, units | A view of the unit record, written by units' handlers and its own code, read by weapons in its run and in DAMAGE's rules at the caller's statement. |
| weapons → ai | `Unit::f_1f`, `f_57` (the flags of slots 0 and 2) | players | A view: weapons' slots, read by ai's ticks in the players phase, as of weapons' last run. |
| weapons → los_radar | `Projectile::position` | projectiles | A view: the projectile table, read by los_radar's line of sight pass after the projectile pass. |
| weapons → map | `Debris::pos` | explosions | A view: the access log attributes to map the read of the debris position, which the explosion pass passes to map's `GetCellMeanHeight` (a query of map's, as of its last apply). |
| weapons → orders | `Unit::f_1f`, `f_3b`, `f_57`, `weapon` | units, projectiles | A view: the slots, read by orders in the unit's update (after weapons) and by return fire (the synchronous ATTACKED route) at weapons' statement. |
| weapons → targeting | `Unit::weapon`, `f2c`, `f48`, `f_1f`, `f_3b`, `f_57` | units | A view: the slots' weapons and flags, read by targeting's `FindWeaponTarget` and `GetWeaponTargetPos`, synchronous slots called from `UpdateUnitWeapons` and ai. |
| weapons → units | `Unit::field_1e`, `b3a`, `b56` (the stockpiles of the three slots) | units | A view: the stockpiles ADD_STOCKPILE and SET_STOCKPILES write (both synchronous), read by units' handler of a capture to copy them into the new unit (mission-capture, tick 499). |

## 5. State and the trace

**Everything stays hashed in this change** (the human, 2026-11-16, on PR #978, and 2026-11-17, on #984): the projectile
table, the explosions, the debris, the exploded pieces and the weapon slots. #979 covers los_radar, map, players and ai
only; what follows moves out of the hash in one later format bump for weapons, orders and units. The module has no
private region and no state of its own that leaves the hash here.

**The hashed state only weapons reads, a candidate for that bump.** A search of the source (`Game::debris`,
`Game::count_1491b`, `Game::slots`, `g_explodedPieces`, `g_debrisMemCache`) finds no reader outside `src/weapons/` and
the trace harness but the info panel's drawing of a piece (`DrawDebrisProjected`, a presentation function) and the
engine's set-up and free (`endgame.cpp`):

| State | Trace section | Written by | Read by |
|---|---|---|---|
| the debris table, `Game::count_1491b` and `Game::debris[300]` (the explosions and the pieces of debris, 0x54 bytes each) | 4 | weapons | weapons (the explosions pass and `WriteState`); the access log also names map as a reader of `Debris::pos` (section 4), though map's code never names the table |
| the pieces used by debris, `Game::slots[300]` and their vertices and faces | 5 | weapons | weapons |
| the exploded pieces, `g_explodedPieces[100]` and their blocks | 3 | weapons | weapons |

**What stays hashed for others to read:** the projectile table (section 2 of the trace), read by ai
(`Projectile::attacker`), los_radar (`Projectile::position`) and map (`radar_capture.cpp`); and the weapon slots in the
unit record (section 3), read by ai, orders, targeting and units.

**Record 48, `weaponsState`** (dumped, never hashed; `docs/trace-format.md`, section 8.31): four counts, the used
entries of the projectile table, of the explosions, of the debris and of the exploded pieces. The debris table holds
the explosions (entries with no model object) and the pieces of debris, and `WriteState` counts them apart.

`privatestate.py --check` and `seamlink.py --pilot-seams` check the seam: no other simulation subsystem writes weapons'
state or calls its entry points, and no query slot writes state.

**The trace dumps weapons' events** in section 47 (`docs/trace-format.md`, section 8.31), after ai's section 42, and
hashes none of them. Each record is an event with the outcome `Apply` gave, or an effect weapons published (`outcome`
1), in weapons' order. The section is new, so the trace format's schema changes.

## 6. Random draws and sim events

Weapons draws from the game logic stream (`RandomInt`) when a shot spreads or a projectile explodes, in the fire step of
`UpdateUnitWeapons` (units phase), in the projectile pass and in an explosion's debris. It records sim events (an
explosion's sound and smoke, a unit's speech) where 3.1c did. Every route around these draws is synchronous (section
4) but SPAWN (a meteor's projectile, which draws nothing in its own event), so the
order in which events are published is the order of the calls in 3.1c, and each draw and record is made at the
statement that makes it. The scheduler keeps the order of runs in a unit's update: units' head, weapons' step, units'
body.

The class `ordered` of `data/dependencies.csv` has 30 rows with weapons as writer or reader (the issue counted 28 when
it was filed), on `rng.parkMiller` and on `sim-event`. Each is kept in order as follows:

| Writer → reader | Kind | Phase | Runs | How it is kept in order |
|---|---|---|---|---|
| ai → weapons | `rng.parkMiller` | projectiles | 3 | REACT_TO_ATTACK's draw, made by ai's handler of the synchronous ATTACKED route at weapons' statement, between weapons' own draws and the projectile's or the explosion's (the same argument as `docs/ai-interface.md`, section 6). |
| ai → weapons | `rng.parkMiller` | units | 1 | REACT_TO_ATTACK's draw, made by ai's handler of the synchronous ATTACKED route at weapons' statement, between weapons' own draws and the projectile's or the explosion's (the same argument as `docs/ai-interface.md`, section 6). |
| features → weapons | `rng.parkMiller` | projectiles | 2 | A projectile's damage and its burning features: DAMAGE to features and AREA_DAMAGE from features are both synchronous, so each draw is made at the publisher's statement, in 3.1c's order around the units' damage. |
| features → weapons | `rng.parkMiller` | units | 2 | A unit's death or damage and a feature set alight: DAMAGE to features is synchronous, so the feature's draw falls between weapons' own, at weapons' statement. |
| features → weapons | `sim-event` | projectiles | 2 | A projectile's damage and its burning features: DAMAGE to features and AREA_DAMAGE from features are both synchronous, so each record is made at the publisher's statement, in 3.1c's order around the units' damage. |
| map → weapons | `rng.parkMiller` | units | 2 | Map's draw inside a unit's update and weapons' in `UpdateUnitWeapons`, in the scheduler's order (head, weapons, body); no queued event lies between them. |
| orders → weapons | `rng.parkMiller` | units | 15 | Orders' draws and weapons': DAMAGE, RELEASE and CLAIM from orders, and PARALYZE and HIT to orders, are synchronous routes, so each draw is made at the publisher's statement in the unit's update (orders run after weapons in the unit's update, as in 3.1c). |
| orders → weapons | `sim-event` | units | 11 | Orders' records and weapons': DAMAGE, RELEASE and CLAIM from orders, and PARALYZE and HIT to orders, are synchronous routes, so each record is made at the publisher's statement in the unit's update (orders run after weapons in the unit's update, as in 3.1c). |
| scripts → weapons | `rng.parkMiller` | units | 8 | Scripts' draws (the explode opcode, an aim thread's end) and weapons': AIM_DONE and EXPLODE_PIECE are synchronous routes, START_SCRIPT and DETACH_PIECE too (EXPLODE_PIECE and START_SCRIPT diverged queued, section 8), so each draw is made at the statement that publishes it. |
| scripts → weapons | `sim-event` | units | 7 | Scripts' records (the explode opcode, an aim thread's end) and weapons': AIM_DONE and EXPLODE_PIECE are synchronous routes, START_SCRIPT and DETACH_PIECE too (EXPLODE_PIECE and START_SCRIPT diverged queued, section 8), so each record is made at the statement that publishes it. |
| targeting → weapons | `rng.parkMiller` | units | 3 | Targeting's retargeting draw and weapons' in `UpdateUnitWeapons`: `FindWeaponTarget` and `GetWeaponTargetPos` are synchronous slots, and weapons' CLEAR to targeting is a synchronous route, so each is made at the caller's statement. |
| units → weapons | `rng.parkMiller` | units | 3 | Units' draws and weapons' inside a unit's update: DAMAGE, DETONATE, RELEASE, INIT_SLOTS, REMOVE_PROJECTILES and SET_STOCKPILES from units and the damage effects to units are synchronous, so each draw is made at the publisher's statement. |
| units → weapons | `sim-event` | projectiles | 17 | Units' records (the speech and smoke of a damaged or dying unit) and weapons' under a projectile: units' routes into weapons and weapons' effects to units are synchronous, so each record is made at the publisher's statement. |
| units → weapons | `sim-event` | units | 32 | Units' records and weapons' inside a unit's update: DAMAGE, DETONATE, RELEASE, INIT_SLOTS, REMOVE_PROJECTILES and SET_STOCKPILES from units and the damage effects to units are synchronous, so each record is made at the publisher's statement. |
| weapons → ai | `rng.parkMiller` | projectiles | 2 | REACT_TO_ATTACK's draw, made by ai's handler of the synchronous ATTACKED route at weapons' statement, between weapons' own draws and the projectile's or the explosion's (the same argument as `docs/ai-interface.md`, section 6). |
| weapons → ai | `rng.parkMiller` | units | 1 | REACT_TO_ATTACK's draw, made by ai's handler of the synchronous ATTACKED route at weapons' statement, between weapons' own draws and the projectile's or the explosion's (the same argument as `docs/ai-interface.md`, section 6). |
| weapons → features | `rng.parkMiller` | projectiles | 1 | A projectile's damage and its burning features: DAMAGE to features and AREA_DAMAGE from features are both synchronous, so each draw is made at the publisher's statement, in 3.1c's order around the units' damage. |
| weapons → features | `rng.parkMiller` | units | 2 | A unit's death or damage and a feature set alight: DAMAGE to features is synchronous, so the feature's draw falls between weapons' own, at weapons' statement. |
| weapons → features | `sim-event` | projectiles | 5 | A projectile's damage and its burning features: DAMAGE to features and AREA_DAMAGE from features are both synchronous, so each record is made at the publisher's statement, in 3.1c's order around the units' damage. |
| weapons → features | `sim-event` | units | 2 | A unit's death or damage and a feature set alight: DAMAGE to features is synchronous, so the feature's record falls between weapons' own, at weapons' statement. |
| weapons → map | `rng.parkMiller` | units | 2 | Map's draw inside a unit's update and weapons' in `UpdateUnitWeapons`, in the scheduler's order (head, weapons, body); no queued event lies between them. |
| weapons → orders | `rng.parkMiller` | units | 13 | Orders' draws and weapons': DAMAGE, RELEASE and CLAIM from orders, and PARALYZE and HIT to orders, are synchronous routes, so each draw is made at the publisher's statement in the unit's update (orders run after weapons in the unit's update, as in 3.1c). |
| weapons → orders | `sim-event` | projectiles | 1 | Orders' records and weapons': DAMAGE, RELEASE and CLAIM from orders, and PARALYZE and HIT to orders, are synchronous routes, so each record is made at the publisher's statement in the unit's update (orders run after weapons in the unit's update, as in 3.1c). |
| weapons → orders | `sim-event` | units | 11 | Orders' records and weapons': DAMAGE, RELEASE and CLAIM from orders, and PARALYZE and HIT to orders, are synchronous routes, so each record is made at the publisher's statement in the unit's update (orders run after weapons in the unit's update, as in 3.1c). |
| weapons → scripts | `rng.parkMiller` | units | 9 | Scripts' draws (the explode opcode, an aim thread's end) and weapons': AIM_DONE and EXPLODE_PIECE are synchronous routes, START_SCRIPT and DETACH_PIECE too (EXPLODE_PIECE and START_SCRIPT diverged queued, section 8), so each draw is made at the statement that publishes it. |
| weapons → scripts | `sim-event` | units | 8 | Scripts' records (the explode opcode, an aim thread's end) and weapons': AIM_DONE and EXPLODE_PIECE are synchronous routes, START_SCRIPT and DETACH_PIECE too (EXPLODE_PIECE and START_SCRIPT diverged queued, section 8), so each record is made at the statement that publishes it. |
| weapons → targeting | `rng.parkMiller` | units | 6 | Targeting's retargeting draw and weapons' in `UpdateUnitWeapons`: `FindWeaponTarget` and `GetWeaponTargetPos` are synchronous slots, and weapons' CLEAR to targeting is a synchronous route, so each is made at the caller's statement. |
| weapons → units | `rng.parkMiller` | units | 2 | Units' draws and weapons' inside a unit's update: DAMAGE, DETONATE, RELEASE, INIT_SLOTS, REMOVE_PROJECTILES and SET_STOCKPILES from units and the damage effects to units are synchronous, so each draw is made at the publisher's statement. |
| weapons → units | `sim-event` | projectiles | 19 | Units' records (the speech and smoke of a damaged or dying unit) and weapons' under a projectile: units' routes into weapons and weapons' effects to units are synchronous, so each record is made at the publisher's statement. |
| weapons → units | `sim-event` | units | 31 | Units' records and weapons' inside a unit's update: DAMAGE, DETONATE, RELEASE, INIT_SLOTS, REMOVE_PROJECTILES and SET_STOCKPILES from units and the damage effects to units are synchronous, so each record is made at the publisher's statement. |

## 7. What stays inside the default

3.1c's types and limits are the default's, not the interface's: the table of 300 projectiles, the 300 explosions and
pieces of debris and the 100 exploded pieces, the three weapon slots of a unit and their layout in the unit record,
the damage formulas (the armour from `damagemodifier` and `killCount`, the kinds of damage), the network packets of a
shot and a damage, and the weapon definitions' files. An event carries a unit as its block offset, a weapon as its
index in the table, an aim callback as its block offset, and a script's name as bytes: no pointers. The set-up slots
(`AllocWeaponArray`, `FreeWeaponArray`, `LoadWeaponTypes`, `FreeWeaponTypes`, `InitExplosions`, `FreeExplosions`) stay
in `WeaponsSeam` as 3.1c's code (section 2).

## 8. What the gate decided

Every route into and out of weapons that is new in this change was queued first, each on its own, with every other
route synchronous (the tier, with `--no-cache`, at 1664f6157 plus the one-line change that queued the route). A route
whose queueing diverged became synchronous. The divergences are against the reference, with the first tick, phase and
field. With every route synchronous, the tier was equal (38 of 38 scenarios, in both modes). A route that stayed equal
is still synchronous when it can be pending while a later read in the same phase needs what it writes (#952's rule,
section 4).

| Route queued | Tier | First divergence |
|---|---|---|
| orders DAMAGE | diverged | mission-victory, tick 181, units phase (`UpdateAllUnits`, 0x48ad30): `rng.parkMiller` (reference 0x613fc705, queued 0x01f484b2). |
| orders RELEASE | diverged | orders-guard-attacked, tick 159, projectiles phase (`UpdateProjectiles`, 0x49b720): `units[251].+0xba` (reference 0x0000, queued 0x4000). |
| orders CLAIM | diverged | mission-radius, tick 1325, units phase: `units[4].f_1f` (reference 0x03, queued 0x02). |
| orders ADD_STOCKPILE | equal | none (synchronous by #952's rule, section 4). The tier has no stockpile weapon, so it was queued on its own on ai-antinuke-intercept and orders-weapon-build, two scenarios that build stockpile rounds, and both were equal |
| units DAMAGE | traces equal, events diverged | conditions-dark-side, tick 2674, units phase: the sim event digest, reference record `2674 units unit_speech 0x1 252 2 0` against `0x0 1 16 0`; also mission-capture, tick 499, units phase. |
| units DETONATE | diverged | mission-victory, tick 188, units phase: `debris[45].obj` (reference piece 0, queued piece 1). |
| units RELEASE | equal | none (synchronous by #952's rule, section 4) |
| units INIT_SLOTS | diverged | mission-kill-of-type, tick 99, units phase: `units[261].+0x14` (reference 0xfddf, queued 0x0320). |
| units REMOVE_PROJECTILES | equal | none (synchronous by #952's rule, section 4) |
| units SET_STOCKPILES | equal | none (synchronous by #952's rule, section 4) |
| units LOAD_SLOT | equal | none (queued, section 4) |
| engine REMOTE_DAMAGE | equal | none (synchronous by #952's rule, section 4; the tier has no network path) |
| scripts AIM_DONE | equal | none (synchronous by #952's rule, section 4) |
| scripts EXPLODE_PIECE | diverged (events) | conditions-dark-side, tick 452, units phase: the sim event digest, reference record `452 units smoke 0x0 63503019 3810299 143206279 7 15 9` against `452 units remove_unit_speech 0x0 2` (the piece's smoke moved after other sim events); also eight-full-moon, tick 6158, and mission-build-unit, tick 71. |
| features AREA_DAMAGE | unchanged | synchronous as before |
| wind_meteors SPAWN | unchanged | queued as before |
| weapons MARK_DAMAGED | equal | none (synchronous by #952's rule, section 4) |
| weapons DAMAGE_RECORD | equal | none (synchronous by #952's rule, section 4) |
| weapons HEALTH | diverged | mission-victory, tick 181, units phase: `rng.parkMiller` (reference 0x613fc705, queued 0x01f484b2). |
| weapons PARALYZE | equal | none (synchronous by #952's rule, section 4) |
| weapons START_SCRIPT | diverged | mission-radius, tick 1279, world phase (the once-per-tick tables, hashed after `UpdateMeteors`): `units[3].script.changed` (reference 1, queued 0); queued, the script started at the phase boundary instead of in the weapon's update, and ai's fields differ first. |
| weapons DETACH_PIECE | diverged | conditions-dark-side, tick 452, units phase: `explodedPieces[0].rec.flags` (reference 0x0006, queued 0x0007). |
| weapons NOTICE | equal | none (synchronous by #952's rule, section 4) |
| weapons BUSY | equal | none (synchronous by #952's rule, section 4) |
| weapons ATTACKED, HIT | unchanged | synchronous as #991 left them (`docs/ai-interface.md`, section 8) |
| weapons DAMAGE to features, CLEAR to targeting | unchanged | synchronous as before (`docs/features-interface.md` and `docs/targeting-interface.md`, section 8) |

**Inbox depths.** Every route into weapons but one is synchronous, so its inbox holds an event only while wind_meteors'
SPAWN waits for the phase boundary: it was never deeper than 1, in any scenario (the SPAWNs of meteors-hailstorm and
hailstorm-los-off, 150 each, and of console-features and meteors-earthquake, each delivered before the next was
published). The slot `LoadState` is called only while a save loads, outside the tick. The most
events delivered to weapons inside one tick was 541, in edges-economy-targeting at tick 245. A synchronous delivery
into weapons ran inside another one 7 times, all orders' RELEASE: a DAMAGE (or features' AREA_DAMAGE) publishes
ATTACKED, ai's handler reacts to the attack with an order that releases the unit's weapons, and that RELEASE runs
inside the outer `Apply`, where 3.1c called `ReleaseWeapons` from inside `ai.ReactToAttack`, inside `DamageUnit`.

**How often each route is taken.** A build that counts every publish into and out of weapons, over the 89 scenarios
with native modules (all 89 equal to the reference): the events published inside the tick and outside it (when a game
is set up or loaded, and the console's commands), and in how many scenarios each was published at all. A route with
several subscribers (ATTACKED to units has two) counts once per subscriber. ADD_STOCKPILE came after the count.

| Route | Inside the tick | Scenarios | Outside the tick | Scenarios |
|---|---|---|---|---|
| orders DAMAGE | 392 | 22 | 0 | 0 |
| orders RELEASE | 37461 | 89 | 149 | 36 |
| orders CLAIM | 17934 | 67 | 0 | 0 |
| orders ADD_STOCKPILE | not counted | | | |
| units DAMAGE | 17778 | 17 | 4 | 3 |
| units DETONATE | 784 | 30 | 0 | 0 |
| units RELEASE | 5976 | 53 | 4272 | 89 |
| units INIT_SLOTS | 1992 | 53 | 1424 | 89 |
| units REMOVE_PROJECTILES | 937 | 35 | 4 | 1 |
| units SET_STOCKPILES | 2 | 2 | 0 | 0 |
| units LOAD_SLOT | 0 | 0 | 12 | 1 |
| engine REMOTE_DAMAGE | 0 | 0 | 0 | 0 |
| scripts AIM_DONE | 10122 | 27 | 0 | 0 |
| scripts EXPLODE_PIECE | 6402 | 27 | 0 | 0 |
| features AREA_DAMAGE | 271 | 7 | 0 | 0 |
| wind_meteors SPAWN | 313 | 4 | 0 | 0 |
| weapons ATTACKED, to units | 14710 | 37 | 8 | 3 |
| weapons ATTACKED, to ai | 7355 | 37 | 4 | 3 |
| weapons ATTACKED, to orders | 7355 | 37 | 4 | 3 |
| weapons HIT, to orders | 8154 | 25 | 0 | 0 |
| weapons MARK_DAMAGED, to units | 7355 | 37 | 4 | 3 |
| weapons DAMAGE_RECORD, to units | 7355 | 37 | 4 | 3 |
| weapons HEALTH, to units | 25058 | 36 | 4 | 3 |
| weapons PARALYZE, to orders | 12 | 2 | 0 | 0 |
| weapons START_SCRIPT, to scripts | 37956 | 71 | 1424 | 89 |
| weapons DETACH_PIECE, to scripts | 3641 | 24 | 0 | 0 |
| weapons NOTICE, to units | 147992 | 27 | 0 | 0 |
| weapons BUSY, to units | 6211 | 26 | 0 | 0 |
| weapons DAMAGE, to features | 1575 | 26 | 0 | 0 |
| weapons CLEAR, to targeting | 15772 | 66 | 39 | 13 |

No scenario plays a network game, so the engine's REMOTE_DAMAGE is never taken.
