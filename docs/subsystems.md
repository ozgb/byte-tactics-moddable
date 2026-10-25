# Subsystems

This file is the draft interface of each subsystem, from the coupling map of #128 (part of #6). For the nine subsystems that have a module issue (#9 and #11 to #18) it gives the entry points, the imports (calls out to other subsystems) and the sim read and write sets, so each issue starts from a draft rather than from the source. The issue's own text wins where the two differ, and each section ends with open questions where the data shows a coupling the issue did not expect.

Nothing here is typed by hand except the prose. Every list and count comes from the tool:

```sh
uv run tools/coupling.py                      # regenerate data/subsystems.csv (keeping override) and data/coupling.csv
uv run tools/coupling.py --check              # fail if either file differs from what the tool regenerates (CI runs this)
uv run tools/coupling.py --summary [NAME ...] # print entry points, imports and sim items per subsystem
```

The inputs are `data/rwsets.csv`, `data/hazard_edges.csv` and `data/state_classes.csv` (see `docs/state.md`); the tool's docstring has the rules that assign a function to a subsystem. The 4519 functions of `data/rwsets.csv` are in 17 subsystems, and the sim items are the rows of `data/state_classes.csv` of class `sim` or `mixed`, plus the struct fields classified the same way. A call or a shared item counts functions of the calling subsystem, not call sites.

The subsystems and the issue each maps to:

| subsystem | functions | issue |
|---|---|---|
| [wind_meteors](#wind_meteors) | 15 | #9 |
| [economy](#economy) | 38 | #11 |
| [features](#features) | 62 | #12 |
| [ai](#ai) | 160 | #13 |
| [pathfinding](#pathfinding) | 59 | #14 |
| [weapons](#weapons) | 118 | #15 |
| [targeting](#targeting) | 10 | #16 |
| [los_radar](#los_radar) | 118 | #17 |
| [orders](#orders) | 468 | #18 |
| [units](#the-remaining-subsystems) | 193 | none yet |
| [players](#the-remaining-subsystems) | 149 | none yet |
| [map](#the-remaining-subsystems) | 109 | none yet |
| [network](#the-remaining-subsystems) | 450 | none yet |
| [game](#the-remaining-subsystems) | 160 | none yet |
| [harness](#the-remaining-subsystems) | 58 | none yet |
| [presentation](#the-remaining-subsystems) | 1514 | none yet |
| [platform](#the-remaining-subsystems) | 838 | none yet |

The eight subsystems without a module issue are described briefly [below](#the-remaining-subsystems). #10 (per-module replay tests) uses all of them.

How to read a section. **Entry points** are the phase roots the subsystem holds and the functions other subsystems call, with the callers. The main loop and the frame handlers (`MainLoopTick`, `RunGameSteps`, `UpdateFramePacing`, `BattleFrame`, `EndGameFrame`, `MainFrameTick`) are in `game` by `override`, although most are in presentation folders (`src/ingame/`), so a phase root has callers in `game`, not in `presentation`. **Imports** are the functions it calls in other subsystems, by target. **Sim writes** and **sim reads** list items with their class from `data/state_classes.csv` and the number of its functions that access the item; "also written by" names the other writers. Lists are cut to the twelve most used named items. "Named" leaves out the shared value types (`Vec3`, `Point`, `Fixed`) and the per-file copies of a struct that the source declares several times (a name ending in an address, such as `Proj_0049c740`).


## wind_meteors

Issue #9. 15 functions. `UpdateWind`, `UpdateMeteors`, the meteor parameters and their save and load (`src/map/meteors.cpp`, plus `UpdateWind` in `src/game/game_state.cpp`).

Coverage: 75.0% of functions (12 of 16), 76.9% of lines, 63.2% of branches; 4 live functions no scenario enters, and 0 more are dead (data/coverage.csv).

**Live functions no scenario enters** (4 of 16 after `combat-deaths`, `meteors-earthquake` and `meteors-hailstorm` (#419) and `save-midgame` (#422); 12 are entered, 75.0%), by reason:

- Console command only (2): `CmdMeteor` and `StartMeteorShower` (only `CmdMeteor` calls it; the scenario file has no console key).
- Load only (1): `LoadMeteors`.
- Shutdown only (1): `EmptyShutdownPreCleanup`.

**Entry points.** Phase roots: `UpdateWind` and `UpdateMeteors`, both called from `RunGameSteps`. `UpdateWind` is also called from `LoadBattleAssets` (game).

11 functions are called from other subsystems. The most called, with their callers:

- `UpdateWind` (2 from game)
- `CmdMeteor` (1 from platform)
- `DisableMeteors` (1 from map)
- `EmptyShutdownPreCleanup` (1 from game)
- `EnableMeteors` (1 from map)
- `InitMeteors` (1 from game)
- `LoadMeteors` (1 from game)
- By calling subsystem (functions called): game 6, map 4, platform 1

**Imports.** 14 functions of 2 subsystems, by target, each with the number of functions it calls there:

- platform (12): `HapiBank::OpenAccount`, `BuildDataPath`, `CommandArgs::GetIntArg`, `FatalError`, and 8 more
- weapons (2): `FindWeaponByName`, `SpawnProjectile`

**Sim writes.** 24 items in all, 19 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`g_meteorActive` (sim, 6 fn); `g_meteorNextStrikeTime` (sim, 6 fn); `g_meteorNextHitTime` (sim, 5 fn); `g_meteorOrigin` (sim, 5 fn); `g_meteorStrikeEndTime` (sim, 5 fn); `g_meteorTarget` (sim, 5 fn); `g_meteorsEnabled` (sim, 5 fn); `g_meteorScheduleGap` (sim, 3 fn); `g_meteorShowerDuration` (sim, 3 fn); `g_meteorSpawnInterval` (sim, 3 fn); `g_meteorSpawnMagnitude` (sim, 2 fn); `g_meteorWeapon` (sim, 2 fn)

**Sim reads.** 10 items only read, 9 named. The most used (`g_game`, the root pointer, left out):

`Game::gameTick` (mixed, 3 fn); `Game::mapHeightTiles` (sim, 2 fn); `Game::mapWidthTiles` (sim, 2 fn); `g_meteorWeaponName` (sim, 2 fn); `DAT_005119b8` (sim, 1 fn); `Game::field_38a47` (sim, 1 fn); `Game::windSpeedDenominator` (sim, 1 fn); `Game::windSpeedMax` (sim, 1 fn); `Game::windSpeedMin` (sim, 1 fn)

**Draft interface.**

- Exports: 11 functions (the two phase roots, `InitMeteors`, `LoadMeteors` and `SaveMeteors` for game, the map controls `EnableMeteors`, `DisableMeteors`, `SetMeteorParams` and `MeteorParams::LoadMeteorDefaults`, and the console command `CmdMeteor`).
- Needs from seams: 14 functions in platform, weapons.
- Owns: 18 named sim items that only this subsystem writes, for example `g_meteorActive`, `g_meteorNextStrikeTime`, `g_meteorNextHitTime`, `g_meteorOrigin`, `g_meteorStrikeEndTime`.
- Shares: 1 named sim item that other subsystems also write, for example `Game::windCounter`.
- Pilot fit (#9): 12 of its 14 imports are platform (TDF parsing, `RandomInt`, `FatalError`, the `HapiBank` account store). The only sim seam is weapons (`SpawnProjectile`, `FindWeaponByName`).
- The wind output is in `Game` fields (`windSpeed`, `windDirection`, `windFraction`, `windVelX`, `windZ`) that only this module writes but units, economy, features and presentation read. The meteor schedule is in `g_meteor*` statics, which must move into the state block (#7).

**Open questions.**

- `SpawnProjectile` is the one call into another sim subsystem. Does the pilot need a weapons seam (#8) before #15 exists, or a stub?
- `RandomInt` is counted as a platform import but draws from a game-logic stream (#3). Should the random streams be an engine service in the seam list instead?

## economy

Issue #11. 38 functions. The resource accounts (`UnitResources`), `UpdatePlayerEconomy`, resource sharing and the share console commands (`src/game/economy*.cpp`, `share_commands.cpp`, `UpdateResourceSharing` in `src/network/net_game.cpp`).

Coverage: 56.2% of functions (18 of 32), 63.9% of lines, 37.2% of branches; 14 live functions no scenario enters, and 7 more are dead (data/coverage.csv).

**Live functions no scenario enters** (14 of 32 after `conditions-dark-side` and `conditions-shore-to-shore` (#420) and `save-midgame` (#422); 18 are entered, 56.2%), by reason:

- Console command only (9): `CmdNoEnergy`, `CmdNoMetal`, `CmdSetShareEnergy`, `CmdSetShareMetal`, `CmdShareAll`, `CmdShareEnergy`, `CmdShareMetal`, and the statics `ShareEnergy` and `ShareMetal` (called only by `CmdShareAll`). #432 adds a `console` verb.
- Console command, share dialog or network only (2): `UnitResources::SpendEnergy` and `UnitResources::SpendMetal`, reached only through `TransferEnergy` and `TransferMetal` (console `Give`, the ally share dialog, share packets). #432 adds a `console` verb.
- Share dialog or network only (1): `ShareMapInfo` (the map-share button of the ally share dialog, or a share packet).
- Network game only (1): `UpdateResourceSharing`. `RunGameSteps` calls it only when `showStats` is set and the packet manager is in use, and it returns at once unless `netGameFlags & 1`.
- Load only (1): `UnitResources::LoadUnitAccounts`.

**Entry points.** Phase roots: `UpdateResourceSharing`, called from `RunGameSteps` only when `showStats` is set and the packet manager is in use, and it returns at once unless `netGameFlags & 1` (a network game). The per-tick income step `UpdatePlayerEconomy` is not a root: `UpdatePlayers` (players) calls it, so most of the economy runs inside the players phase.

20 functions are called from other subsystems. The most called, with their callers:

- `GetEnergyUse` (4 from ai)
- `ShareMapInfo` (1 from network, 1 from presentation)
- `UnitResources::RequestEnergyAndMetal` (1 from orders, 1 from units)
- `UnitResources::Reset` (1 from players, 1 from units)
- `UpdateMetalExtraction` (2 from units)
- `CmdNoEnergy` (1 from platform)
- `CmdNoMetal` (1 from platform)
- By calling subsystem (functions called): platform 7, units 6, players 4, ai 1, game 1, network 1

**Imports.** 18 functions of 6 subsystems, by target, each with the number of functions it calls there:

- platform (5): `CommandArgs::GetIntArg`, `HapiBank::OpenNamedBox`, `HapiBank::SeekBox`, `HapiBank::ReadBox`, and 1 more
- network (4): `AddMessage`, `BroadcastPlayerInfo`, `PlayerDpid_00457d30`, `SendPacketToPlayer`
- los_radar (3): `GetMapCell`, `ShareMapping`, `ShareRadar`
- game (2): `IsPlaying`, `SetSize_00496e90`
- players (2): `TransferEnergy`, `TransferMetal`
- units (2): `CobScript::StartScriptWithArgs`, `Unit::SetStateBits`

**Sim writes.** 41 items in all, 28 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Player::energy` (mixed, 7 fn, also written by game, players, presentation); `Player::metal` (mixed, 7 fn, also written by game, players, presentation); `UnitResources::energyUse` (mixed, 4 fn); `UnitResources::player` (mixed, 4 fn); `Player::energyCapacity` (sim, 3 fn, also written by players); `Player::metalCapacity` (sim, 3 fn, also written by players); `UnitResources::metalUse` (mixed, 3 fn); `Player::shareEnergy` (mixed, 2 fn, also written by players); `Player::shareMetal` (mixed, 2 fn, also written by players); `PlayerInfo::shareMetal` (mixed, 2 fn); `Unit::extraction` (mixed, 2 fn, also written by units); `UnitResources::energyUsePaid` (mixed, 2 fn)

**Sim reads.** 58 items only read, 52 named. The most used (`g_game`, the root pointer, left out):

`Game::localPlayer` (mixed, 8 fn); `Player::active` (mixed, 8 fn); `Player::type` (mixed, 8 fn); `Game::netGameFlags` (mixed, 7 fn); `Game::players` (sim, 7 fn); `Player::info` (mixed, 5 fn); `Game::difficulty` (mixed, 4 fn); `Player::index` (mixed, 3 fn); `Game::gameTick` (mixed, 2 fn); `Game::tidal` (sim, 2 fn); `Game::windFraction` (sim, 2 fn); `Unit::def` (mixed, 2 fn)

**Draft interface.**

- Exports: 20 functions (`UpdatePlayerEconomy`, `UpdateResourceSharing`, the spend and request calls (`UnitResources::SpendEnergy`, `SpendMetal`, `RequestEnergy`, `RequestEnergyAndMetal`), `GetEnergyUse` (ai) and `UpdateMetalExtraction` (units)).
- Needs from seams: 18 functions in platform, network, los_radar, game, players, units.
- Owns: 6 named sim items that only this subsystem writes, for example `UnitResources::energyUse`, `UnitResources::player`, `UnitResources::metalUse`, `PlayerInfo::shareMetal`, `UnitResources::energyUsePaid`.
- Shares: 22 named sim items that other subsystems also write, for example `Player::energy`, `Player::metal`, `Player::energyCapacity`, `Player::metalCapacity`, `Player::shareEnergy`.
- `Player::energy` and `Player::metal` are written by four subsystems (economy, game, players, presentation; class `mixed`), so the economy module cannot own them alone. The statistics (`energyWasted`, `totalMetalProduced` and the rest) are also written by network and players.
- Economy sends packets (`SendPacketToPlayer`, `BroadcastPlayerInfo`, `AddMessage`) and starts COB scripts (`CobScript::StartScriptWithArgs`); both are candidates for events instead of imports.

**Open questions.**

- Which subsystem owns the per-player account fields? A state-block struct for the accounts that economy writes and players and network only read would match the data better than the `Player` struct.
- `GetMapCell`, `ShareMapping` and `ShareRadar` are imports from los_radar: resource sharing also shares vision. Does that sharing belong to los_radar's interface or to economy's?

## features

Issue #12. 62 functions. `UpdateFeatures` and feature loading, spreading, burning, reclaim and wreckage (`src/map/features*.cpp`).

Coverage: 77.4% of functions (41 of 53), 73.3% of lines, 63.3% of branches; 12 live functions no scenario enters, and 7 more are dead (data/coverage.csv).

**Live functions no scenario enters** (12 of 53 after `combat-deaths`, `meteors-earthquake` and `meteors-hailstorm` (#419) and `save-midgame` (#422); 41 are entered, 77.4%), by reason:

- Console command only (5): `CmdBurnAll`, `CmdBurnOne`, `CmdFeature`, `FindFeatureType`, `RemoveAllFeatures`.
- Load only (4): `LoadFeatures` and the statics only it calls: `FeatureIndex`, `FindName`, `FreeFeatureList` (all in `features_424c00.cpp`).
- Shutdown only (1): `FreeFeaturePool` (called from the line-of-sight teardown).
- Cursor only (1): `GetFeature` (the static in `unit_scripts.cpp`, used by the reclaim checks of `GetOrderCursor`, which no scenario enters).
- Needs a case the scenarios do not set up (1): `FeatureVec3` (its one call site zeroes the velocity of a debris spot when it lands, `features.cpp:1054`).

**Entry points.** Phase roots: `UpdateFeatures`, called from `RunGameSteps`.

23 functions are called from other subsystems. The most called, with their callers:

- `GetCellFeature` (1 from game, 1 from map, 1 from orders)
- `DamageFeature` (1 from network, 1 from weapons)
- `PlaceFeature` (1 from los_radar, 1 from units)
- `ReclaimFeature` (2 from orders)
- `CmdBurnAll` (1 from platform)
- `CmdBurnOne` (1 from platform)
- `CmdFeature` (1 from platform)
- By calling subsystem (functions called): game 8, los_radar 5, orders 3, network 3, platform 3, units 2

**Imports.** 46 functions of 8 subsystems, by target, each with the number of functions it calls there:

- platform (25): `TdfRecord::GetFieldString`, `RandomInt`, `TdfFile::ResetCurrentRecord`, `TdfFile::SelectRecord`, and 21 more
- presentation (9): `InitGafSequence`, `FindGafEntry`, `FreeObjectState`, `CreateObjectState`, and 5 more
- network (4): `BroadcastPacket`, `GetLocalDpid`, `GetHostDpid`, `SendPacketToPlayer`
- map (3): `Mission::GetGameType`, `RefreshAllPassMaps`, `GetCellMeanHeight`
- weapons (2): `ApplyAreaDamageAt`, `FindWeaponByName`
- game (1): `StepGafSequence`
- los_radar (1): `GetMapCell`
- 1 smaller targets: units (1)

**Sim writes.** 70 items in all, 44 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Game::features` (mixed, 24 fn); `Cell::feature` (mixed, 15 fn, also written by los_radar); `Game::featureCount` (mixed, 13 fn); `Cell::flags` (mixed, 10 fn, also written by map); `Pool::entries` (mixed, 8 fn); `Cell::spot` (mixed, 6 fn); `Spot::feature` (mixed, 6 fn); `Cell::offsetX` (mixed, 5 fn); `Cell::offsetY` (mixed, 5 fn); `Pool::usedHead` (mixed, 5 fn); `Pool::freeHead` (mixed, 4 fn); `Spot::used` (mixed, 4 fn)

**Sim reads.** 73 items only read, 67 named. The most used (`g_game`, the root pointer, left out):

`Game::mapWidthTiles` (sim, 8 fn); `Feature::name` (sim, 6 fn); `Game::heightMap` (mixed, 6 fn); `DAT_005119b8` (sim, 5 fn); `Feature::footprint` (sim, 5 fn); `Game::mapHeightTiles` (sim, 5 fn); `Game::spots` (sim, 5 fn); `Feature::flags` (sim, 4 fn); `Spot::anim` (sim, 4 fn); `Feature::flags16` (sim, 3 fn); `Feature::noobject` (sim, 3 fn); `Game::mapInfo` (mixed, 3 fn)

**Draft interface.**

- Exports: 23 functions (`UpdateFeatures`, the cell accessor `GetCellFeature`, `PlaceFeature` (los_radar, units), `DamageFeature` (weapons, network), `ReclaimFeature` (orders), and the load, save, pool and file-list calls from game and los_radar).
- Needs from seams: 46 functions in platform, presentation, network, map, weapons, game, los_radar, units.
- Owns: 38 named sim items that only this subsystem writes, for example `Game::features`, `Game::featureCount`, `Pool::entries`, `Cell::spot`, `Spot::feature`.
- Shares: 6 named sim items that other subsystems also write, for example `Cell::feature`, `Cell::flags`, `FeatureDamagePacket::sub`, `FeatureDamagePacket::type`, `FeatureDamagePacket::x`.
- It has 9 imports from presentation (`EmitWhiteSmoke`, `PlaySoundAtByName`, the GAF loaders, `CreateObjectState`). Sim code that draws or plays sound has to raise events instead (#22).
- Features and weapons call each other: features calls `ApplyAreaDamageAt`, weapons calls `DamageFeature`. `Cell::feature` is also written by los_radar (`ClearFeature` is in `line_of_sight.cpp`, so it is los_radar by file).

**Open questions.**

- `ClearFeature` is a feature operation in a LOS file. Should it move to features, which would remove the second writer of `Cell::feature`?
- `Cell::spot`, `Cell::offsetX` and `Cell::offsetY` are written here and read by los_radar, map, orders and weapons. Are they feature state or part of the cell record that map owns?

## ai

Issue #13. 160 functions. The computer player: `UpdatePlayerAI`, build ratings, weight scripts and squads (`src/ai/ai_player*.cpp`, `player_ai.cpp`, `ai_profile.cpp`, most of `src/units/squads.cpp`).

Coverage: 88.6% of functions (93 of 105), 88.7% of lines, 81.9% of branches; 12 live functions no scenario enters, and 37 more are dead (data/coverage.csv).

**Live functions no scenario enters** (12 of 105, #420), by reason:

- Shutdown only (4): `DestroyPlayerAI`, `FreeSquads`, `SquadManager::DeleteTimers` and `SquadTimer::~SquadTimer`, all called from `FreePlayers`, which `ShutdownIngameSystems` calls and no scenario runs.
- Console command only (2): `DumpPlayerAI` (console `PrintWeights`) and `ResetAIPlayers` (console `ReloadAIProfiles`). #432 adds a `console` verb.
- Network only (2): `ProtectUnitDefsReadOnly` and `ProtectUnitDefsReadWrite`, called only from `UnitSync::ApplyToUnitTypes`.
- Needs a case no scenario sets up yet (4): `SumUnitRatingsInRange` (the AI scout probe rating, `SpatialTimer::OnTimer` calls it only when its probe point is visible) and the three statics only `SpatialTimer::OnTimer` calls: `IsExplored`, `IsVisible` and `Offset` in `ai_player_407e70.cpp`. These are not inlined copies of a function that is entered elsewhere: the other `IsExplored`, `IsVisible` and `Offset` statics in the tree take other argument types and are other functions, and coverage counts an inlined body at its source. `OnTimer` returns before them when the spatial group (group 9) holds no unit, so a scenario needs an AI whose script gives that group units.

**Entry points.** Phase roots: none. `UpdatePlayerAI` and `SquadManager::TickIfActive` are called by `UpdatePlayers` (players), so the AI runs inside the players phase.

32 functions are called from other subsystems. The most called, with their callers:

- `GetFactoriesInRadius` (9 from orders)
- `operator-` (5 from orders)
- `SetUnitSquad` (3 from units, 1 from presentation)
- `GetVisibleEnemiesInRadius` (2 from orders, 1 from targeting)
- `IsPointVisible` (2 from los_radar, 1 from presentation)
- `SquadManager::MarkOwnerNetDirtyFromDamageSplit` (3 from weapons)
- `Bits::Test` (1 from targeting)
- By calling subsystem (functions called): players 10, orders 5, platform 5, presentation 3, game 2, los_radar 2

**Imports.** 48 functions of 11 subsystems, by target, each with the number of functions it calls there:

- platform (13): `RandomInt`, `operator new`, `CommandArgs::GetArg`, `CommandArgs::InitArgs`, and 9 more
- orders (6): `AddOrder`, `GetOrderType`, `operator+`, `DeleteOrders`, and 2 more
- players (5): `GetNetEnergy`, `GetEnergyIncome`, `GetMetalIncome`, `GetNetMetal`, and 1 more
- targeting (5): `GetWeaponTargetUnit`, `SetWeaponTargetUnit`, `ClearWeaponTarget`, `FindWeaponTarget`, and 1 more
- map (4): `Mission::GetNameSlot`, `CanBuildAt`, `CanPlaceUnitFootprint`, `Mission::GetGameType`
- units (4): `Unit::SetStateBits`, `UnitTypeSet::AddTypeOrCategory`, `NotifyUnitRefs`, `RemoveFast`
- presentation (3): `GetBuildSiteMetal`, `QueueBuildOrder`, `QueueUnitSpeechIfNotVisible`
- 4 smaller targets: weapons (3), los_radar (2), pathfinding (2), economy (1)

**Sim writes.** 25 items in all, 15 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`g_playerAI` (sim, 22 fn); `Unit::flags` (mixed, 10 fn, also written by game, los_radar, map, orders, players, units, weapons); `SquadManager::timers` (sim, 8 fn); `SquadTimer::next` (sim, 7 fn); `SquadManager::player` (sim, 5 fn); `Unit::group` (mixed, 4 fn, also written by orders, units); `g_aiCommandsEnabled` (sim, 4 fn); `PlayerAI::lastTick` (sim, 3 fn); `PlayerView::squads` (mixed, 3 fn); `SquadManager::countdown` (sim, 3 fn); `PlayerAI::centre` (sim, 2 fn); `PlayerAI::hasSpecial` (sim, 2 fn)

**Sim reads.** 124 items only read, 101 named. The most used (`g_game`, the root pointer, left out):

`Game::gameTick` (mixed, 11 fn); `Game::unitDefs` (mixed, 11 fn); `Game::unitDefCount` (mixed, 9 fn); `Unit::def` (mixed, 8 fn); `Game::mapInfo` (mixed, 6 fn); `Player::index` (mixed, 6 fn); `Game::mapWidthWorld` (sim, 5 fn); `Player::active` (mixed, 5 fn); `Player::type` (mixed, 5 fn); `Unit::unitDefIndex` (mixed, 5 fn); `UnitDef::extractsMetal` (sim, 5 fn); `UnitDef::flags1` (mixed, 5 fn)

**Draft interface.**

- Exports: 32 functions (`UpdatePlayerAI`, `SquadManager::TickIfActive`, the create, reset and destroy calls from game, `SetUnitSquad` (units), `SquadManager::MarkOwnerNetDirtyFromDamageSplit` (weapons) and helpers other subsystems borrow: `GetFactoriesInRadius` (orders), `GetVisibleEnemiesInRadius` (orders, targeting), `IsPointVisible` (los_radar)).
- Needs from seams: 48 functions in platform, orders, players, targeting, map, units, presentation, weapons, los_radar, pathfinding, economy.
- Owns: 13 named sim items that only this subsystem writes, for example `g_playerAI`, `SquadManager::timers`, `SquadTimer::next`, `SquadManager::player`, `g_aiCommandsEnabled`.
- Shares: 2 named sim items that other subsystems also write, for example `Unit::flags`, `Unit::group`.
- It acts through other modules: orders (`AddOrder`, `IssueAttackOrder`, `DeleteOrders`), targeting (5 calls) and `Unit::SetStateBits`. Its own sim writes are few.
- Its read set is the widest of the nine (124 items: `Unit`, `UnitDef`, `Player`, `Game`). A read-only view of the state block fits it better than a getter per field.

**Open questions.**

- The AI calls the pathfinder's open-list heap (`MakeHeap`, `PopHeapFirst`). Is that heap a pathfinding export or shared library code?
- `GetFactoriesInRadius` (9 calls) and `operator-` (5) are used by orders. Do these helpers belong in a shared maths or query seam instead of ai?

## pathfinding

Issue #14. 59 functions. The search in `src/ai/pathfind*.cpp`: per-player cost tables, the node budget, the open heap and the grid.

Coverage: 93.8% of functions (45 of 48), 92.4% of lines, 85.4% of branches; 3 live functions no scenario enters, and 10 more are dead (data/coverage.csv).

**Live functions no scenario enters** (3 of 48, #421; 45 are entered, 93.8%), by reason:

- Unreachable (2): `Pathfinder::RemoveNode` and `OpenHeap::SiftDownInline`. `RunSearches` is the only caller of `RemoveNode`, and it zeroes `topPopped` just before the `if (!topPopped)` that guards the call, so the call is dead code in 3.1c. `SiftDownInline` is called only by `RemoveNode`.
- Shutdown only (1): `Pathfinder::~Pathfinder`, called from `DestroyPathfinder` in `ShutdownIngameSystems`. Test mode ends with `host_exit(0)` right after the last tick, so no scenario runs the shutdown.

**Entry points.** Phase roots: none. `Pathfinder::RunSearches` is called by `UpdatePlayers` (players).

4 functions are called from other subsystems. The most called, with their callers:

- `Pathfinder::AbortIfGoalMatch` (2 from orders)
- `PopHeapFirst` (2 from ai)
- `MakeHeap` (1 from ai)
- `Pathfinder::RunSearches` (1 from players)
- By calling subsystem (functions called): ai 2, orders 1, players 1

**Imports.** 24 functions of 3 subsystems, by target, each with the number of functions it calls there:

- orders (16): `AiSearchGoal::SetWaypoints`, `ApproachRadius::ApproxDistExcess`, `OrderFx::ApproxDist`, `PointMarker::ApproxDist`, and 12 more
- platform (4): `operator delete`, `operator new`, `GameAllocIgnoreTag`, `GameFreeThunk`
- units (4): `MovementClass::RefreshUnitIfStale`, `MovementClass::Get`, `MovementClass::InBounds`, `MovementClass::RefreshMovedUnits`

**Sim writes.** 46 items in all, 42 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`OpenHeap::items` (sim, 13 fn); `OpenHeap::pool` (sim, 12 fn); `Grid::cells` (mixed, 11 fn, also written by los_radar, map); `Node::index` (sim, 11 fn); `OpenHeap::count` (sim, 11 fn); `Grid::width` (mixed, 9 fn, also written by los_radar, map); `Grid::dirty` (sim, 8 fn); `PathCell::flags` (sim, 8 fn); `OpenHeap::topPopped` (sim, 7 fn); `OpenHeap::freeHead` (sim, 6 fn); `Pathfinder::costScale` (sim, 6 fn); `Pathfinder::object` (mixed, 6 fn)

**Sim reads.** 28 items only read, 25 named. The most used (`g_game`, the root pointer, left out):

`Pathfinder::grid` (sim, 6 fn); `g_dirDeltaX` (sim, 3 fn); `g_dirDeltaZ` (sim, 3 fn); `Game::mapHeightTiles` (sim, 2 fn); `Game::mapWidthTiles` (sim, 2 fn); `Game::players` (sim, 2 fn); `MovementClass::footprintX` (mixed, 2 fn); `MovementClass::footprintZ` (mixed, 2 fn); `NodeData::pos` (sim, 2 fn); `Unit::motion` (mixed, 2 fn); `g_turnCosts` (sim, 2 fn); `AiSearchGoal::target` (sim, 1 fn)

**Draft interface.**

- Exports: 4 functions (`Pathfinder::RunSearches` (players), `Pathfinder::AbortIfGoalMatch` (orders), `MakeHeap` and `PopHeapFirst` (ai)).
- Needs from seams: 24 functions in orders, platform, units.
- Owns: 38 named sim items that only this subsystem writes, for example `OpenHeap::items`, `OpenHeap::pool`, `Node::index`, `OpenHeap::count`, `Grid::dirty`.
- Shares: 4 named sim items that other subsystems also write, for example `Grid::cells`, `Grid::width`, `Grid::count`, `Grid::height`.
- Only 4 functions are called from outside, so the entry side is small. The import side is not: 16 of its 24 imports are goal-object methods (`PathGoal`, `PointMarker`, `RingApproach`, `OrderFx`, `ApproachRadius`, `AiSearchGoal`) that orders implements. The goals are a callback interface that orders passes in.
- The budget (`Pathfinder::budget`, `g_playerBudgetCap`, `g_budgetRefreshCounter`, `g_playerTickLoad`) is state this module writes alone: the limit #14 wants the module to read from outside.

**Open questions.**

- `Grid::cells`, `width` and `height` show as written by pathfinding, los_radar and map. Either `Grid` is one name for unrelated structs (type-wide evidence) or there is a real shared grid; a clang layout run would tell.
- The goals live in `order_targets.cpp` (orders) but only pathfinding calls `ApproxDist` and `ContainsCell`. Should the goal interface be declared by pathfinding?

## weapons

Issue #15. 118 functions. `UpdateProjectiles`, `UpdateExplosions`, `UpdateUnitWeapons`, ballistics, damage and the weapon definitions (`src/weapons/*.cpp`).

Coverage: 84.1% of functions (74 of 88), 84.2% of lines, 68.5% of branches; 14 live functions no scenario enters, and 20 more are dead (data/coverage.csv).

**Entry points.** Phase roots: `UpdateProjectiles` and `UpdateExplosions`, called from `RunGameSteps`. `UpdateUnitWeapons` is not a root: `UpdateAllUnits` (units) calls it.

33 functions are called from other subsystems. The most called, with their callers:

- `Unit::ClaimWeapons` (32 from orders)
- `Unit::ReleaseWeapons` (17 from orders, 1 from units)
- `DamageUnit` (4 from orders, 4 from units, 2 from players)
- `WeaponCanReachUnit` (7 from orders, 1 from ai, 1 from targeting)
- `FindFeatureAtPos` (3 from orders, 1 from players)
- `FindWeaponByName` (1 from features, 1 from units, 1 from wind_meteors)
- `GetWeaponRange` (2 from orders, 1 from targeting)
- By calling subsystem (functions called): units 12, game 8, orders 7, ai 3, network 3, players 3

**Imports.** 69 functions of 13 subsystems, by target, each with the number of functions it calls there:

- platform (25): `RandomInt`, `operator new`, `BuildDataPath`, `GameAllocIgnoreTag`, and 21 more
- presentation (16): `EmitWeaponSmoke`, `PlaySoundAt`, `EmitWhiteSmoke`, `AccumulateScreenShake`, and 12 more
- units (10): `CobScript::StartScriptWithArgs`, `GetAimFromPosition`, `CobScript::StartScript`, `CobScript::FindScript`, and 6 more
- network (3): `BroadcastPacket`, `GetLocalDpid`, `SendScriptCallByName`
- ai (2): `SquadManager::MarkOwnerNetDirtyFromDamageSplit`, `ReactToAttack`
- features (2): `DamageFeature`, `GetFootprintCentre`
- 6 smaller targets: los_radar (2), orders (2), targeting (2), economy (1), game (1), map (1)

**Sim writes.** 297 items in all, 141 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Game::projectiles` (mixed, 18 fn); `WeaponDef::index` (sim, 11 fn); `Game::projectileCount` (mixed, 9 fn); `WeaponDef::speed` (sim, 9 fn); `WeaponFirePacket::aimOrVel` (sim, 9 fn); `WeaponFirePacket::weaponDefIndex` (sim, 8 fn); `Game::projCount` (sim, 7 fn); `WeaponFirePacket::origin` (sim, 7 fn); `WeaponFirePacket::targetUnitId` (sim, 7 fn); `WeaponFirePacket::type` (sim, 7 fn); `WeaponDef::holdTime` (sim, 6 fn); `WeaponFirePacket::fireHeading` (sim, 6 fn)

**Sim reads.** 243 items only read, 118 named. The most used (`g_game`, the root pointer, left out):

`Game::seaLevel` (mixed, 9 fn); `Unit::player` (sim, 9 fn); `Unit::heading` (sim, 8 fn); `Unit::utype` (sim, 8 fn); `Game::gameTick` (mixed, 7 fn); `Game::gravity` (sim, 7 fn); `Unit::playerIndex` (sim, 6 fn); `Game::mapInfo` (mixed, 5 fn); `Unit::script` (mixed, 5 fn); `Game::netGameFlagsWord` (mixed, 4 fn); `Game::units` (mixed, 4 fn); `Game::weaponDefs` (sim, 4 fn)

**Draft interface.**

- Exports: 33 functions (the two roots, `UpdateUnitWeapons`, `Unit::ClaimWeapons` and `Unit::ReleaseWeapons` (orders), `DamageUnit` (orders, players, units), `WeaponCanReachUnit`, `WeaponCanReachPos`, `FindWeaponByName`, `SpawnProjectile` and the net packet handlers `ApplyWeaponFirePacket` and `ApplyProjectileHitPacket`).
- Needs from seams: 69 functions in platform, presentation, units, network, ai, features, harness, los_radar, orders, targeting, economy, game, map.
- Owns: 125 named sim items that only this subsystem writes, for example `Game::projectiles`, `WeaponDef::index`, `Game::projectileCount`, `WeaponDef::speed`, `WeaponFirePacket::aimOrVel`.
- Shares: 16 named sim items that other subsystems also write, for example `Game::cameraSnapTicks`, `Unit::flags`, `g_explodedPieces`, `Game::count`, `Game::cameraFollowTrackObj`.
- The largest import list of the nine: 69 functions in 13 subsystems, 16 of them presentation (smoke, sound, screen shake, GAF and model loading) and 10 units (COB scripts, piece positions).
- Most of the 297 writes are not shared state: dozens of `WeaponDef::*` fields written when the TDFs load, packet buffers (`WeaponFirePacket`, `UnitDamagePacketOut`) and per-file struct copies. The shared state is `Game::projectiles`, `Game::projectileCount`, `Unit::health`, `Unit::flags` and the weapon slot fields.
- It also writes view state: `Game::cameraSnapPos` and `cameraSnapTicks` (from `DetonateProjectile` and `CheckProjectileCollision`).

**Open questions.**

- `DamageUnit` writes `Unit::health`, which game, orders, players and units also write. Is damage a weapons export or a units operation that weapons requests? #15 already asks which machine applies damage in multiplayer.
- `Unit::ClaimWeapons` has 32 callers, all in orders. It is a lifecycle call, not a per-tick one; does it belong to a unit lifecycle seam?

## targeting

Issue #16. 10 functions. Target choice and the weapon target setters and getters (`FindWeaponTarget`, `FindBestTargetIfFireAtWill`, `SetWeaponTargetUnit`). It is small because the file rules put the order goal objects in orders; see the notes.

Coverage: 100.0% of functions (10 of 10), 96.2% of lines, 73.7% of branches; 0 live functions no scenario enters, and 1 more is dead (data/coverage.csv).

**Entry points.** Phase roots: none. Order handlers (orders) and the AI call it.

9 functions are called from other subsystems. The most called, with their callers:

- `SetWeaponTargetUnit` (8 from orders, 2 from ai)
- `ClearWeaponTarget` (5 from orders, 1 from ai)
- `FindBestTargetIfFireAtWill` (6 from orders)
- `GetWeaponTargetUnit` (3 from orders, 2 from ai, 1 from weapons)
- `SetWeaponTargetPos` (3 from orders, 1 from ai)
- `FindWeaponTarget` (1 from ai)
- `GetWeaponTargetPos` (1 from weapons)
- By calling subsystem (functions called): orders 6, ai 5, weapons 2, units 1

**Imports.** 11 functions of 5 subsystems, by target, each with the number of functions it calls there:

- units (5): `CobScript::FindScript`, `CobScript::StartScriptWithArgs`, `GetGroundHeight`, `GetSweetSpot`, and 1 more
- ai (2): `Bits::Test`, `GetVisibleEnemiesInRadius`
- weapons (2): `GetWeaponRange`, `WeaponCanReachUnit`
- platform (1): `RandomInt`
- presentation (1): `Class_004800c0::EraseSwapBack`

**Sim writes.** 5 items in all, 0 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

none

**Sim reads.** 33 items only read, 25 named. The most used (`g_game`, the root pointer, left out):

`Game::units` (mixed, 2 fn); `Unit::id` (sim, 2 fn); `Unit::script` (mixed, 2 fn); `Game::matchFlags` (sim, 1 fn); `Game::seaLevel` (mixed, 1 fn); `Player::active` (mixed, 1 fn); `Player::type` (mixed, 1 fn); `Unit::def` (mixed, 1 fn); `Unit::flags` (mixed, 1 fn); `Unit::killCount` (mixed, 1 fn); `Unit::mode20` (sim, 1 fn); `Unit::motion` (mixed, 1 fn)

**Draft interface.**

- Exports: 9 functions (all of them are target setters, getters and searches; `SetWeaponTargetUnit` (10 callers), `GetWeaponTargetUnit` (6), `ClearWeaponTarget` (6) and `FindBestTargetIfFireAtWill` (6) are the busiest).
- Needs from seams: 11 functions in units, ai, weapons, platform, presentation.
- Owns: no named sim item that only this subsystem writes.
- Shares: no named sim item that another subsystem also writes.
- It has no strong write to a named sim item. Its 5 written items are `Vec3` and `Point` value types. The target itself is held in the weapon slot (`UnitWeaponSlot`), and the setters probably write it through a pointer, which counts as a read.
- It imports the enemy search from ai (`GetVisibleEnemiesInRadius`, `Bits::Test`) and the range checks from weapons (`GetWeaponRange`, `WeaponCanReachUnit`). Its 5 units imports are COB script calls, `GetGroundHeight` and `GetSweetSpot`.

**Open questions.**

- With no state of its own, is targeting a set of functions over the weapon slot, and so part of weapons, as #16 allows?
- `unit_targets.cpp` is one file in 3.1c, split here by name into units 11, targeting 8, orders 3 and weapons 3. The split is the rule's guess and needs a decision in #16.

## los_radar

Issue #17. 118 functions. Line of sight, radar and sonar, the LOS tables, the map cell accessor and the TNT loader (`src/map/line_of_sight*.cpp`, `los_tables*.cpp`, `radar.cpp`, and two `src/game/players_46*.cpp` files).

Coverage: 75.3% of functions (67 of 89), 79.8% of lines, 69.7% of branches; 22 live functions no scenario enters, and 20 more are dead (data/coverage.csv).

**Live functions no scenario enters** (22 of 89 after `sensors-los-off`, `los-off-mapped-ai` and `hailstorm-los-off` (#421) and `sensors-sonar-jam` and `orders-tier2-core` (#426); 67 are entered, 75.3%), by reason:

- Shutdown only (4): `FreeRadar`, `FreeMapResources`, `LosTables::FreeTables` and `LosTables::~LosTables`. `FreeRadar` and `FreeMapResources` are called from `ShutdownIngameSystems`, which no scenario runs because test mode ends with `host_exit(0)` right after the last tick. `FreeTables` has no caller but `FreeLosTables`, which nothing calls, and the destructor is that of the static table, which never runs, because `host_exit` ends the process with `TerminateProcess`.
- Element destruction of the LOS table vectors (4): `DestroyPoint`, `DestroyLine` and `std::_Destroy` (both overloads). The vector code calls them when it destroys its elements, which only `FreeTables` and the destructor do.
- Element construction of the LOS table vectors (2): `StoreDwordIfDst` and `std::_Construct`. The tables are filled by `resize` in `LoadLosLine`, and no scenario enters these two; not explained further.
- Console command only (3): `CmdLOS`, `CmdLOSType` and `CmdRadar`. The game has no console in test mode (#432 adds a console key).
- Network only (2): `CmdShareMapping` and `CmdShareRadar`, the share commands, which all check `netGameFlags & 1` (a network game); test mode runs a skirmish.
- Menu and cursor only (2): `ResizeRadarPicture` (the multiplayer lobby map preview) and `Visible` (the mouse hover cursor of `GetOrderCursor`).
- Needs a case no scenario can set up yet (5): `ByteMap_004658e0::Get`, `MapSize_004658e0::Contains`, `IsExplored`, `IsSeen` and `IsFootprintVisible`, the five `nodrawundergray` feature checks of `DrawBattleFrame` in `players_4658e0.cpp`. A 3D feature drawn in the local camera view makes the test-mode build exit 1 (#437), so no scenario can enter them until #437 is fixed.

**Entry points.** Phase roots: none. `UpdatePlayers` (players) calls `RecalculateLineOfSight`, `UpdateUnitLineOfSight` and `UpdateSensorRadarAndCloak`, so LOS runs inside the players phase.

42 functions are called from other subsystems. The most called, with their callers:

- `GetMapCell` (13 from features, 2 from weapons, 1 from ai, 1 from economy, 1 from game, 1 from map, 1 from network, 1 from presentation, 1 from units)
- `GetMapCellAtPosition` (4 from orders, 3 from weapons, 1 from map, 1 from presentation)
- `IsUnitVisibleToPlayer` (4 from presentation, 1 from ai, 1 from units)
- `RecalculateLineOfSight` (3 from game, 1 from players, 1 from presentation)
- `UpdateUnitLineOfSight` (2 from players, 2 from units, 1 from orders)
- `StringRef::Assign` (2 from platform, 1 from map, 1 from units)
- `ClampWorldPosToTerrain` (2 from game, 1 from players)
- By calling subsystem (functions called): units 12, platform 9, game 8, presentation 8, map 7, players 6

**Imports.** 47 functions of 8 subsystems, by target, each with the number of functions it calls there:

- presentation (18): `GetGafFrame`, `DrawFrame`, `StringRef::ReleaseRef`, `AllocFrame`, and 14 more
- platform (16): `operator delete`, `GameFreeThunk`, `GameAllocIgnoreTag`, `TdfFile::SelectRecord`, and 12 more
- features (5): `FreeFeaturePool`, `InitFeatureAnimPool`, `PlaceFeature`, `PlaceMissionFeatures`, and 1 more
- ai (2): `IsPointVisible`, `HasReadyUnitInRange`
- map (2): `Mission::GetNameSlot`, `VisitObjectsInRange`
- network (2): `AddMessage`, `BroadcastPlayerInfo`
- game (1): `SaveSettings`
- 1 smaller targets: units (1)

**Sim writes.** 118 items in all, 103 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`SightQuery::sightDistance` (mixed, 11 fn); `Game::heightMap` (mixed, 9 fn); `SightQuery::cacheCell` (mixed, 9 fn); `SightQuery::frameIdx` (mixed, 9 fn); `Game::visibilityMask` (mixed, 8 fn); `Grid::height` (mixed, 7 fn, also written by map, pathfinding); `Grid::width` (mixed, 7 fn, also written by map, pathfinding); `SightQuery::player` (mixed, 7 fn); `GafFrame::xOffset` (mixed, 6 fn, also written by presentation, weapons); `GafFrame::yOffset` (mixed, 6 fn, also written by presentation, weapons); `Game::eyes` (sim, 6 fn); `Grid::cells` (mixed, 6 fn, also written by map, pathfinding)

**Sim reads.** 119 items only read, 78 named. The most used (`g_game`, the root pointer, left out):

`Game::mapWidthTiles` (sim, 16 fn); `Game::mapHeightTiles` (sim, 15 fn); `Game::playerIndex` (mixed, 14 fn); `Game::seaLevel` (mixed, 10 fn); `Game::losTable` (sim, 8 fn); `Unit::def` (mixed, 8 fn); `FrameTable::count` (sim, 6 fn); `Game::players` (sim, 6 fn); `Player::info` (mixed, 6 fn); `Unit::player` (sim, 6 fn); `Game::gameTick` (mixed, 5 fn); `Game::localPlayer` (mixed, 5 fn)

**Draft interface.**

- Exports: 42 functions (the three players-phase calls, `GetMapCell` and `GetMapCellAtPosition`, `IsUnitVisibleToPlayer`, `RevealNewUnit`, `ShareMapping` and `ShareRadar` (economy), and the load, free and console calls).
- Needs from seams: 47 functions in presentation, platform, features, ai, map, network, game, units.
- Owns: 85 named sim items that only this subsystem writes, for example `SightQuery::sightDistance`, `Game::heightMap`, `SightQuery::cacheCell`, `SightQuery::frameIdx`, `Game::visibilityMask`.
- Shares: 18 named sim items that other subsystems also write, for example `Grid::height`, `Grid::width`, `GafFrame::xOffset`, `GafFrame::yOffset`, `Grid::cells`.
- 18 imports are presentation (`AllocSurface`, `DrawCircle`, `DrawFrame`, `DownsampleFrame`) and 16 platform. The file rules put the radar picture and minimap code here (`DrawRadarUnits`, `ResizeRadarPicture`, the `Game::minimapGadget*` writes), so most of the 118 functions are likely view code. This is the boundary #17 asks to keep clear.
- The result is `Game::visibilityMask`, which ai, economy, map, orders, pathfinding and presentation read. `Player::explored` is read here and written elsewhere.

**Open questions.**

- `GetMapCell` has 22 callers in 9 subsystems. It is the map cell accessor, not a LOS function. Should map own it, with los_radar importing it?
- `Game::visibilityMask` is read by pathfinding and ai as well as presentation. Which part is sim state and which is the player view (#22)?

## orders

Issue #18. 468 functions. The order state machines, VTOL orders, order lists, unit motion (`UnitMotion`) and the order goal objects (`src/orders/*.cpp`, 14,000 lines). The largest subsystem.

Coverage: 79.0% of functions (294 of 372), 74.8% of lines, 58.2% of branches; 78 live functions no scenario enters, and 28 more are dead (data/coverage.csv).

**Live functions no scenario enters** (78 of 372 after `orders-immediate`, `orders-factory` and `orders-air` (#418), the three scenarios of #419, the three of #421, the ten of #426 and `save-midgame` (#422); 294 are entered, 79.0%), by reason:

- Network only (23): the net-state methods of the goal objects and the bit writers: `LiteGoal::HasNetUnitState`, `LiteGoal::SerializeNetUnitState`, `AiSearchGoal::HasNetUnitState`, `AiSearchGoal::SerializeNetUnitState`, `PackedPosGoal` (constructor, destructor, `DeserializeNetUnitState`), `PathGoal` (8: `DeserializeNetUnitState`, `HasNetUnitState`, `SerializeNetUnitState`, `ExportGoalPose`, `FillWaypointWorldPos`, `HasReadyWaypoints`, `TickTowardGoal`, `TryClaimRepath`), `PatrolGoal` (4: constructor, `DeserializeNetUnitState`, `FillWaypointWorldPos`, `HasReadyWaypoints`), `BitReader::ReadBit`, `OrderFx::WriteBits`, `AirManeuverOrder::SerializeToBits`, `PathOrder::SerializeToBits`. `PatrolGoal` and `PackedPosGoal` are made only for a player of type 3 (a remote human), and the `PathGoal` methods only run through them.
- Save and load only (27): `LowerBoundOrderTypes`, `OrderTypeNameLess`, `KindByIndex_0043a420`, `KindByName_0043a420`, `ResolveUnitTypeKey`, `UnitMotion::LoadMotion`, `Order::Order` (load constructor), `Order::ReattachFxToUnit`, `SerializeSave`, `OrderFx::OrderFx`, `OrderFx::GetType`, the load constructors, `GetType` and `Serialize` or `SerializeToSave` of `AirManeuverOrder`, `ApproachRadius`, `PathOrder` and `RingApproach`, and the load constructor of `PointMarker`. `save-midgame` enters the save side of `Order`, `PointMarker` and `UnitMotion`.
- Info panel, cursor, build menu and shutdown only (8): `GetOrderName`, `GetOrderTarget`, `GetBuildWeaponPercent` (info panel), `SumQueuedBuildCount` (build menu count text), `GetOrderCursor` (cursor), `ClearOrderTypeTable` and `DestroyPathfinder` (shutdown), and `Marked` (the smart right-click order, mode 1, which the scenario `order` line has no verb for).
- Campaign mission script only (4): `AttackUTypeOrder`, `WaitOrder`, `WaitForAttackOrder`, `MakeSelectableOrder`.
- No caller in the game (13): `RepairUnitNoMoveOrder`, `ReadyOrder`, `ClearTargets_0043b7c0`, `TryGetDesiredHeading`, `VtolGetRepairedOrder` (nothing creates the order: only its definition and table row name it), the `Sub` of `unit_orders.cpp` at about line 1433 (the one at about line 1989 is `TeleportOrder`'s helper and is entered), and the base-class slots `OrderFx::ApproxDist`, `ContainsCell`, `Destroy`, `FillGoalCells` and `IsFxStyle` (also of `AirManeuverOrder` and `PathOrder`).
- Transport load and unload only (3): `GroundPickupOrder`, `GroundUnloadOrder` and `WaitIfCobBusy` (called only by those two). No scenario carries a unit: a transport's script attaches its cargo at piece -1, and each tick 3.1c then reads the cargo's heading and pitch from the bytes before the carrier's piece block (`GetPieceAngles` with index -1); #428 defines those and brings the transport scenario back.
- Needs a fight the scenarios do not set up (1): `AddComponentwise`, which runs only when `WeaponCanReachUnit` fails twice in `AirToGroundHoverOrder`. Neither gunship's weapon (ARMBRAWL `VTOL_EMG`, CORAPE `VTOL_rocket`) is toair or ballistic, and the trials could not make it fail (out of range targets, a submarine, a wading unit).

**Entry points.** Phase roots: none. `UpdateAllUnits` (units) calls `RunOrders`; units also call `UnitMotion::UpdateMotion`.

62 functions are called from other subsystems. The most called, with their callers:

- `CopyDwordIfNonNull` (5 from platform)
- `GetOrderType` (3 from ai, 1 from game, 1 from presentation)
- `operator+` (2 from ai, 2 from weapons, 1 from presentation)
- `AddOrder` (3 from ai, 1 from game)
- `Order::OrStatusFlags` (4 from units)
- `AiSearchGoal::SetWaypoints` (3 from pathfinding)
- `ApproachRadius::ApproxDistExcess` (3 from pathfinding)
- By calling subsystem (functions called): units 21, pathfinding 16, presentation 13, ai 6, game 6, map 3

**Imports.** 86 functions of 13 subsystems, by target, each with the number of functions it calls there:

- units (26): `GetHeadingBetween`, `Unit::SetStateBits`, `AttachUnitToPiece`, `GetNanoPiecePosition`, and 22 more
- platform (11): `operator new`, `RandomInt`, `operator delete`, `HapiBank::OpenNamedBox`, and 7 more
- map (9): `CanPlaceUnitFootprint`, `VisitObjectsInRange`, `IsPadSlotFree`, `PickRandomReclaimableResourcesInRadius`, and 5 more
- network (7): `BitReader::ReadBits`, `BitWriter::WriteBits`, `SendScriptCallNoArgs`, `BitWriter::GrowBuffer`, and 3 more
- weapons (7): `Unit::ClaimWeapons`, `Unit::ReleaseWeapons`, `WeaponCanReachUnit`, `DamageUnit`, and 3 more
- presentation (6): `QueueUnitSpeech`, `EmitNanoParticles`, `MarkSelectionOrdersDirty`, `EmitReverseNanoParticles`, and 2 more
- targeting (6): `SetWeaponTargetUnit`, `FindBestTargetIfFireAtWill`, `ClearWeaponTarget`, `GetWeaponTargetUnit`, and 2 more
- 6 smaller targets: ai (5), features (3), los_radar (3), economy (1), pathfinding (1), players (1)

**Sim writes.** 149 items in all, 114 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Order::flags` (mixed, 95 fn, also written by presentation); `Unit::pos` (mixed, 66 fn, also written by presentation, units); `Order::state` (sim, 52 fn); `Order::pos` (sim, 39 fn); `Order::next` (mixed, 37 fn); `PathOrderAttach::owner` (mixed, 30 fn); `Unit::flags` (mixed, 25 fn, also written by ai, game, los_radar, map, players, units, weapons); `Unit::health` (mixed, 20 fn, also written by game, players, units, weapons); `Unit::buildLeft` (mixed, 15 fn, also written by players, units); `Angles16::heading` (mixed, 12 fn, also written by game, presentation, units); `Order::unit` (mixed, 12 fn); `PathGoal::owner` (sim, 11 fn)

**Sim reads.** 300 items only read, 193 named. The most used (`g_game`, the root pointer, left out):

`Unit::def` (mixed, 84 fn); `Unit::motion` (mixed, 49 fn); `Unit::list` (sim, 31 fn); `Unit::player` (sim, 23 fn); `UnitDef::altitude` (sim, 23 fn); `Order::target` (sim, 22 fn); `Unit::carrier` (mixed, 22 fn); `Unit::list2` (sim, 22 fn); `UnitDef::maxHealth` (sim, 20 fn); `UnitDef::flags` (sim, 18 fn); `g_missionOrderTableBegin` (sim, 16 fn); `UnitMotion::flags` (sim, 14 fn)

**Draft interface.**

- Exports: 62 functions (`RunOrders`, `RunSecondaryOrders`, `AddOrder`, `DeleteOrders`, `GetOrderType`, `IssueAttackOrder`, `OrderType::GetTableEntry` (presentation), the `UnitMotion::` functions (13 in orders, 4 called from outside: `UpdateMotion`, `LoadMotion`, `SaveMotion`, `DestroyObject`), and the goal methods pathfinding calls).
- Needs from seams: 86 functions in units, platform, map, network, weapons, presentation, targeting, ai, features, los_radar, economy, pathfinding, players.
- Owns: 87 named sim items that only this subsystem writes, for example `Order::state`, `Order::pos`, `Order::next`, `PathOrderAttach::owner`, `Order::unit`.
- Shares: 27 named sim items that other subsystems also write, for example `Order::flags`, `Unit::pos`, `Unit::flags`, `Unit::health`, `Unit::buildLeft`.
- The units imports (26) are the unit lifecycle and geometry: `CreateUnit`, `SetUnitPosition`, `Unit::SetStateBits`, COB script calls and piece positions. Weapons (7) is mostly `Unit::ClaimWeapons` and `Unit::ReleaseWeapons`.
- It is the heaviest writer of core unit fields: `Unit::pos` from 66 functions and `Unit::flags` from 25. The fields are also written by units and presentation (`pos`), and by seven other subsystems (`flags`).

**Open questions.**

- Which side owns `Unit::pos`, `flags`, `health` and `buildLeft` in the state block (#7): units, with orders going through its seam, or orders? This decides whether #18 can split.
- The split #18 allows would follow the `UnitMotion::` functions (13 in orders). Is motion a module of its own?

## The remaining subsystems

Each has no module issue yet. Entry points and couplings come from the same tool (`uv run tools/coupling.py --summary NAME`); the numbers count functions.

**units** (193 functions). Phase root `UpdateAllUnits`, which calls `RunOrders` (orders) and `UpdateUnitWeapons` (weapons). 68 functions are called from outside; the busiest are `GetHeadingBetween` (28), `Unit::SetStateBits` (24), `GetGroundHeight` (20). It is called most by orders (26 distinct functions) and game (24). It imports 179 functions, mostly platform (56), presentation (32), orders (21) and network (17). It holds the unit table and lifecycle code (`CreateUnit`, `Unit::SetStateBits`, COB scripts), and it is the other side of most of orders' writes. Coverage: 70.1% of functions (117 of 167), 74.0% of lines, 61.4% of branches; 50 live functions no scenario enters, and 34 more are dead (data/coverage.csv). The list of those 50 is under **units, functions no scenario enters** below.

**units, functions no scenario enters** (50 of 167 after `combat-deaths`, `meteors-earthquake` and `meteors-hailstorm` (#419) and `save-midgame` (#422); 117 are entered, 70.1%), by reason:

- Base-class slots of `CobScript` that `UnitScript` overrides (11): `CobScript::AttachUnit`, `CobScript::DropUnit`, `CobScript::EmitSfx`, `CobScript::GetTransporterId`, `CobScript::GetUnitValue`, `CobScript::IsCarryingUnit`, `CobScript::IsPieceCached`, `CobScript::IsPieceShaded`, `CobScript::IsPieceVisible`, `CobScript::PlaySoundNoop`, `CobScript::SetUnitValue`. Every unit's script is a `UnitScript`, so the base bodies are never called.
- COB opcodes that no script of the game data uses (4): `UnitScript::IsCarryingUnit`, `UnitScript::GetTransporterId`, `UnitScript::PlaySoundNoop`, `Channel::XorOp`. No unit can reach them.
- Load only (4): `LoadUnits`, `LoadUnit`, `Elem_00487af0` (a sort helper of the load) and `CobScript::LoadScriptState`. `save-midgame` (#422) enters the save side (`SaveUnits`, `CobScript::SaveScriptState` and the `UnitScript::IsPiece` slots it calls); test mode never loads a game.
- Network only (6): `SendUnitStates`, `WriteUnitState`, `ReceiveUnitStates`, `ReadUnitState`, `CheckMapCrc` (multiplayer map check), `FindUnusedLogo` (multiplayer lobby).
- Shutdown only (8): `FreeUnitMemory`, `FreeUnitTypes`, `FreeUnitInfo`, `FreeUnitCategories`, `FreeDownloadMenus`, `FreeMovementClasses` and the two statics only it calls, `FreeA` and `FreeC`.
- Console command only (4): `KillAllUnits`, `KillUnitsOfType`, `ReloadUnitType`, `Selectable` (the console table entry; the orders path has its own static).
- Transport load and unload only (3): `UnitScript::AttachUnit`, `UnitScript::DropUnit` and `GetUnit`. No scenario carries a unit: a transport's script attaches its cargo at piece -1, and each tick 3.1c then reads the cargo's heading and pitch from the bytes before the carrier's piece block (`GetPieceAngles` with index -1); #428 defines those and brings the transport scenario back.
- Unit info dialog only (2): `MakePropList` and its static `spd`.
- Campaign mission load and script only (7): `FindUnitType`, `Less_00488a50` (its binary-search comparator) and `vector::vector`, `vector::~vector`, `vector::operator?A` (the `std::vector<Unit*>` of `CreateMissionUnits`), which are all used by `CreateMissionUnits`; `FindMissionUnit` (the `G` and similar verbs of `RunInitialMission`); and `MissionSlot::MissionSlot` (the locals of `RunInitialMission`). Neither `CreateMissionUnits` nor `RunInitialMission` is entered.
- Slot no code calls (1): `AimCobStub`, slot 1 of `g_weaponAimCobVtable`. It returns 0; not explained further.

**players** (149 functions). Phase root `UpdatePlayers`, which calls into economy (`UpdatePlayerEconomy`), ai (`UpdatePlayerAI`, squads), pathfinding (`RunSearches`) and los_radar (three LOS calls) in turn. 41 functions are called from outside, mostly by game (17) and presentation (14); the busiest are `KillPlayerUnits` (5), `ShutdownScoreTables` (4), `TransferEnergy` (4). It imports 77 functions: platform 23, network 12, ai 10, presentation 9, los_radar 6. It holds the per-player state and victory conditions. Coverage: 36.4% of functions (52 of 143), 35.2% of lines, 24.9% of branches; 91 live functions no scenario enters, and 11 more are dead (data/coverage.csv). The list of those 91 follows this paragraph.

**Live functions no scenario enters** (89 of 143 after `conditions-dark-side` and `conditions-shore-to-shore` (#420), `save-midgame` (#422) and `allied-victory` (#432); 54 are entered, 37.8%), by reason:

- Game type 1 (campaign) only (23): `AtLeast200`, `DefeatAllUnitsKilled::IsSatisfied`, `DefeatAllUnitsKilled::VisitUnit`, `DefeatAnyUnitPassesX::IsSatisfied`, `DefeatAnyUnitPassesX::VisitUnit`, `DefeatAnyUnitPassesZ::IsSatisfied`, `DefeatAnyUnitPassesZ::VisitUnit`, `DefeatDeathTimerRunsOut::IsSatisfied`, `MissionCondition::IsSatisfied`, `MissionConditions::AllVictoryConditionsMet`, `MissionConditions::AnyDefeatConditionMet`, `SetStartingStorageBonus`, `UnitList::ForEach`, `VictoryBuildUnitType::IsSatisfied`, `VictoryBuildUnitType::VisitUnit`, `VictoryDestroyAllUnits::IsSatisfied`, `VictoryMoveUnitToRadius::IsSatisfied`, `VictoryMoveUnitToRadius::VisitUnit`, `VictoryTimerRunsOut::IsSatisfied`, `VictoryUnitTypePassesX::IsSatisfied`, `VictoryUnitTypePassesX::VisitUnit`, `VictoryUnitTypePassesZ::IsSatisfied`, `VictoryUnitTypePassesZ::VisitUnit`. `CheckVictory` and `CheckDefeat` read the condition lists only for game type 1; test mode fixes game type 2 (`SetMissionType(2)`, `src/harness/harness.cpp:560`). `SetStartingStorageBonus` and its static `AtLeast200` run from `InitPlayerResources` for game type 1 (the other call site is in `UpdatePlayers` under `commanderDeath == 2`, which the harness cannot set). #432 adds a `game_type` key.
- Mission maps only (14): `DefeatAllUnitsKilledOfType::DefeatAllUnitsKilledOfType`, `DefeatAllUnitsKilledOfType::OnUnitDied`, `DefeatAllUnitsKilledOfType::VisitUnit`, `DefeatAnyUnitPassesX::DefeatAnyUnitPassesX`, `DefeatAnyUnitPassesZ::DefeatAnyUnitPassesZ`, `DefeatDeathTimerRunsOut::DefeatDeathTimerRunsOut`, `DefeatUnitTypeKilled::DefeatUnitTypeKilled`, `DefeatUnitTypeKilled::OnUnitDied`, `VictoryCaptureUnitType::OnUnitCaptured`, `VictoryKillAllMobileUnits::OnUnitDied`, `VictoryKillAllMobileUnits::VisitUnit`, `VictoryKillAllOfType::OnUnitDied`, `VictoryKillAllOfType::VisitUnit`, `VictoryKillUnitType::OnUnitDied`. `RegisterConditions` creates these classes from mission condition keys in the map file (`CommanderKilled`, `KillEnemyCommander`, `AllUnitsKilledOfType` and so on), in every game type. Three skirmish maps carry a condition key, all in `totala2.hpi`: Dark Side and Lava Alley carry `KillEnemyCommander`, and Shore to Shore carries `CommanderKilled`. The `conditions-dark-side` and `conditions-shore-to-shore` scenarios run them, entering `VictoryKillEnemyCommander::OnUnitDied` and `DefeatCommanderKilled::OnUnitDied`. The keys behind the 14 listed constructors and handlers (`AllUnitsKilledOfType`, `UnitTypeKilled`, `AnyUnitPassesX`, `AnyUnitPassesZ`, `DeathTimerRunsOut`, `CaptureUnitType`, `KillAllMobileUnits`, `KillAllOfType` and `KillUnitType`) appear on no skirmish map, only on mission maps: the campaign maps of `totala4.hpi` and `ccmiss.ccx`, and the mission maps of `tactics1.hpi` to `tactics8.hpi`, so those constructors are never entered. The mission maps have only `Easy`, `Medium` and `Hard` schemas. Test mode runs a skirmish (`SetMissionType(2)`), and `Mission::SelectSchema` then wants a `Network N` schema, so loading a mission map fails with "No suitable schema type in mission file!", after `RegisterConditions` has run. They become reachable with #432's `game_type 1` key. Their `VisitUnit` slots are reached from their own `OnUnitDied`.
- Save and load only (39): `DefeatAllUnitsKilled::LoadState`, `DefeatAllUnitsKilled::SaveState`, `DefeatAllUnitsKilledOfType::LoadState`, `DefeatAllUnitsKilledOfType::SaveState`, `DefeatAnyUnitPassesX::LoadState`, `DefeatAnyUnitPassesX::SaveState`, `DefeatAnyUnitPassesZ::LoadState`, `DefeatAnyUnitPassesZ::SaveState`, `DefeatCommanderKilled::LoadState`, `DefeatCommanderKilled::SaveState`, `DefeatDeathTimerRunsOut::LoadState`, `DefeatDeathTimerRunsOut::SaveState`, `DefeatUnitTypeKilled::LoadState`, `DefeatUnitTypeKilled::SaveState`, `LoadPlayerControllers`, `LoadPlayers`, `MissionConditions::LoadConditions`, `VictoryBuildUnitType::LoadState`, `VictoryBuildUnitType::SaveState`, `VictoryCaptureUnitType::LoadState`, `VictoryCaptureUnitType::SaveState`, `VictoryDestroyAllUnits::LoadState`, `VictoryDestroyAllUnits::SaveState`, `VictoryKillAllMobileUnits::LoadState`, `VictoryKillAllMobileUnits::SaveState`, `VictoryKillAllOfType::LoadState`, `VictoryKillAllOfType::SaveState`, `VictoryKillEnemyCommander::LoadState`, `VictoryKillEnemyCommander::SaveState`, `VictoryKillUnitType::LoadState`, `VictoryKillUnitType::SaveState`, `VictoryMoveUnitToRadius::LoadState`, `VictoryMoveUnitToRadius::SaveState`, `VictoryTimerRunsOut::LoadState`, `VictoryTimerRunsOut::SaveState`, `VictoryUnitTypePassesX::LoadState`, `VictoryUnitTypePassesX::SaveState`, `VictoryUnitTypePassesZ::LoadState`, `VictoryUnitTypePassesZ::SaveState`. `save-midgame` (#422) enters `MissionConditions::SaveConditions` and `SavePlayers`, but the save side of a condition runs only for a condition the map has, and The Pass has none, so the `SaveState` slots stay listed. Test mode never loads a game.
- Network only (7): `AllocScoreTables`, `EnableReporter`, `FillScoreTables`, `IsReporterDllLoaded`, `LoadReporterDll`, `ShutdownScoreTables` (the online score reporter of the lobby and of `UnitSync`) and `PlayerRecordAt` (a static of `DrawSyncStatus`, the load-progress strip drawn only for game type 3 while waiting for other machines).
- Console command, share dialog or network only (2): `TransferEnergy`, `TransferMetal` (console `Give`, the ally share dialog and share packets).
- Menu or dialog click only (2): `ContinueWatchingCallback` (the continue-watching button after the local player is defeated) and `SaveNumSkirmishPlayers` (called from the skirmish menu, which test mode skips).
- Shutdown only (1): `FreePlayers` (called from `ShutdownIngameSystems`, which no scenario runs).
- Needs a case no scenario sets up yet (1): `MissionConditions::Deactivate` (the console `Kill` command, or a map with no starting units).

**map** (109 functions). No phase root. 51 functions are called from outside, by presentation (24), game (22), units (12) and orders (9); the busiest are `Mission::GetGameType` (45), `Mission::LoadMissionByName` (15), `Mission::GetMissionName` (12) (mission accessors) and `CanPlaceUnitFootprint` (8). It imports 62 functions: platform 36, los_radar 7, wind_meteors 4. It holds the terrain, the pass maps and footprints. Coverage: 76.5% of functions (75 of 98), 78.0% of lines, 68.6% of branches; 23 live functions no scenario enters, and 12 more are dead (data/coverage.csv).

**map, functions no scenario enters** (23 of 98 after `sensors-los-off`, `los-off-mapped-ai` and `hailstorm-los-off` (#421) and `save-midgame` (#422); 75 are entered, 76.5%), by reason:

- Load only (4): `ForceNeighborFootprintReclaim` (called from `LoadUnit`), `LoadMetalPlotmap`, `LoadPlayerFeaturesPlotmap` and `LoadMappingData` (the saved game files). `save-midgame` (#422) enters the save side; no scenario loads a game.
- Shutdown only (2): `FreeOtaEnumCacheAndMission`, called at the end of `ShutdownIngameSystems`, which no scenario runs because test mode ends with `host_exit(0)` right after the last tick, and `Vector_00438480::~Vector_00438480`, the destructor of the static `g_missionOrderTableVec`, which never runs, because `host_exit` ends the process with `TerminateProcess`.
- Network lobby and map check only (6): `Mission::ComputeMapChecksum`, `MapCacheEntry::MapCacheEntry` (both constructors), `MapCacheEntry::operator?4`, `MapCacheEntry::SetChecksum` (all four only from `ComputeMapChecksum`) and `Mission::GetTerrainLength` (the map check of a joining player).
- Multiplayer lobby and menus only (5): `Mission::GetTerrainSizeTier` (the lobby colour), `Mission::GetDescription`, `Mission::GetPlanet`, `Mission::GetTranslatedName` and `Mission::HasMissionName` (the map list and mission menus). Test mode calls only `LoadMissionByName`.
- Campaign menu, end screen and restart only (6): `Mission::BuildMissionList`, `Mission::GetBriefing`, `Mission::SelectMission`, `Mission::MissionExists`, `Mission::AdvanceMission` and `Mission::GetMissionIndex`.

**network** (450 functions). Phase root `HandleNetPackets`, which dispatches into every subsystem, so what it reaches says nothing about the owner. 103 functions are called from outside, mostly by presentation (65) and game (32); the busiest are `BroadcastPlayerInfo` (26), `BroadcastPacket` (23), `AddMessage` (18). It imports 86 functions: presentation 26, platform 21, players 11, units 10. Its packet handlers write many sim items (250 in all).

**network, functions no scenario enters** (284 of 333 after `save-midgame` (#422) and `allied-victory` (#432); 49 are entered, 14.7%), by reason:

- Network only (188): `AddNetPlayer`, `AssignPlayerColor`, `AssignStartPositions`, `BitReader::ReadBits`, `BitReader::ReadSignedBits`, `BitWriter::BitWriter`, `BitWriter::FreeBuffer`, `BitWriter::GrowBuffer`, `BitWriter::SetByteAt`, `BitWriter::WriteBits`, `BroadcastAllyTeam`, `BroadcastPendingViewState`, `CheckDuplicatePlayerIds`, `CheckPlayerTimeouts`, `CloseNetSession`, `CommandAllowed`, `CopyWords`, `CountActiveAIPlayers`, `CountMessage`, `CountPacket`, `CreateNetGame`, `CreateUnitFromPacket`, `DefaultPacketHandler`, `DequeueByCalls`, `DropPlayer`, `EnumPlayersCallback`, `FindActiveId_00453010`, `FindFrom_00450530`, `FindHost`, `FindIndex_00451bc0`, `FindIndex_00452960`, `FindIndex_00453010`, `FindOccupied_004568c0`, `FindPlayerByDpid`, `FindPlayerCall_00452cc0`, `FindPlayerIndex`, `FindPlayerIndex_0044fed0`, `FindPlayerIndex_00451bc0`, `FindPlayerIndex_004523e0`, `FindPlayerIndex_00453640`, `FindPlayerSlot`, `FindPlayer_00450530`, `FindPlayer_00452cc0`, `FindSlotCall_00452cc0`, `FindSlot_00450a10`, `FindSlot_00452570`, `FindSlot_00452cc0`, `FindSlot_00453a50`, `FindToB_00450530`, `FindTo_00450530`, `FirstConnectedId`, `FirstJoinedId`, `FirstJoinedId_004568c0`, `FormatNetStats`, `FrameQueue::QueueFrames`, `FrameQueue::ResetFrames`, `FrameQueue::~FrameQueue`, `FrameRing::Pop`, `FrameRing::Push`, `FreeTeam`, `GetCreatedLobbyInterface`, `GetFrame`, `GetHostDpid`, `GetLocalHumanDpid`, `GetPlayerDpid`, `GetPlayerField_0044fed0`, `GetPlayerId`, `GetPlayerId_00450530`, `GetPlayerId_00451bc0`, `GetPlayerId_004523e0`, `GetPlayerId_00453640`, `GetPlayerId_00456de0`, `GetSessionGuidInstance`, `GetSlotDpid_00452cc0`, `HandlePing`, `HandlePlayerEconomy`, `HandleTimeoutDialog`, `InGame`, `InfoPacket`, `InitLobbiedConnection`, `InitNetConnection`, `InitPacketManager`, `IsActive12_00452960`, `IsColorFree`, `IsConnected`, `IsConnected_004568c0`, `IsPlaying_00450530`, `IsPlaying_00450e20`, `IsPlaying_00450f90`, `IsPlaying_00452bd0`, `IsPlaying_00452cc0`, `IsReusable`, `IsState2_00452960`, `IsState3_00452960`, `IsTimedOut`, `IsType1_00452cc0`, `IsType3_00452960`, `IsType3_00452cc0`, `IsValid`, `JoinLobbyGame`, `JoinLobbyGameEntry`, `JoinLobbyGameThread`, `JoinNetGame`, `LeaveNetGame`, `Less_0046d6c0::operator?R`, `LocalPlayer`, `NetCondenser::Accumulate`, `NetCondenser::ReceivePacket`, `NetCondenser::SendPacket`, `NetCondenser::SendPacketTo`, `Next_00462f30`, `OpenTimeoutDialog`, `PacketBuffer::AppendPacket`, `PacketBuffer::FreePackets`, `PacketBuffer::RemovePacket`, `PacketChannel::AddPacket`, `PacketChannel::AllocBuffer`, `PacketChannel::AllocPacket`, `PacketChannel::EnqueuePacket`, `PacketChannel::GrowPools`, `PacketChannel::InitPools`, `PacketChannel::SendQueued`, `PacketChannel::~PacketChannel`, `PacketManager::AppendToSendBuffer`, `PacketManager::FindChannel`, `PacketManager::HandleIntegrityNop`, `PacketManager::QueueOnChannel`, `PacketManager::QueuePacket`, `PacketManager::SetDefaultSendPacing`, `PacketReceiver::FindPlayerFrameInfo`, `PacketReceiver::ReceiveFrame`, `PacketReceiver::~PacketReceiver`, `PacketRing::Peek`, `PacketRing::Pop`, `PacketRing::PopPacket`, `PacketRing::Push`, `PacketRing::PushPacket`, `PacketTrace`, `PeekFrame`, `Player::FreeSideDataAndFogSightCounts`, `Player::IsPlayableSlot`, `Player::SetType`, `PlayerById`, `PlayerByIndex`, `PlayerBySlot`, `PlayerDpid_00457d30`, `PlayerField`, `PlayerFrameInfo::Initialize`, `PlayerFrameInfo::~PlayerFrameInfo`, `PlayerId_004515d0`, `PlayerId_004568c0`, `Prev_00462f30`, `QueryOnlineFlags`, `QueueCount`, `RIReport`, `RIReportGameChat`, `RISetCallbacks`, `ReceiveNetPacket`, `RejectPacketType2`, `RejectPacketType5`, `RejectPacketType6`, `RejectPacketType7`, `RejectPacketType8`, `RejectPlayer`, `RemovePlayer`, `Remove_00452cc0`, `ReportGameChat`, `ReportPacketGap`, `SendLobbySyncRequests`, `SendPacketToPlayer`, `SetAlliance`, `TakeFrame`, `UnitAt`, `UnitSync::CheckUnitAvailable`, `UnitSync::End`, `UnitSync::Find`, `UnitSync::HandleSyncPacket`, `UnitSync::NotifyEntryChanged`, `UnitSync::ReceiveSyncPacket`, `UnitSync::SendSyncPacket`, `UnitSyncIter::UnitSyncIter`, `UnitSyncIter::operator?8`, `UnitSyncMap::End`, `UnitSyncMap::FindExact`, `UnitSyncMap::LowerBound`, `UpdateTimeoutDialog`, `Vec_0046d6c0::begin`, `Vec_0046d6c0::end`. Needs other machines or a network session; test mode runs every player on one machine. Covered by the replay recordings, #10 and #389.
- Menu or front end only (80): `??$copy@PBUElem_0046faf0@@PAU1@@std@@YGPAUElem_0046faf0@@PBU1@0PAU1@@Z`, `??$swap@W4_Redbl@?$_Tree@IU?$pair@$$CBIUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@@std@@YGXAAW4_Redbl@?$_Tree@IU?$pair@$$CBIUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@0@0@Z`, `AreAllPlayersReady`, `Class_00470250::Capacity`, `Class_00470250::count`, `Class_00470270::Size`, `Class_00470270::assign_first`, `Class_00470270::assign_second`, `Class_00470270::begin`, `Class_00470270::burn`, `Class_00470270::end`, `CopyDwordRangeOverwrite`, `CountComputerPlayers`, `CountHumanPlayers`, `CountLocalComputerPlayers`, `CreateLocalPlayer`, `CreateUnitSync`, `DeleteUnitSync`, `FindPlayerInUse`, `FindPlayer_00457a50`, `FindReadyPlayer`, `FindTarget`, `FinishUnitSync`, `FlagOf_0046d040`, `GetRejectReasonText`, `IsHostLocal`, `IsHuman`, `IsPlaying`, `NopChecksumEntryDtor`, `OnlineGetLinkInfo`, `OnlineProcessButtonCommand`, `OnlineTranslate`, `PacketSequencer::Inl`, `PacketSequencer::PacketSequencer`, `PacketSequencer::SendUnsequenced`, `PacketSequencer::operator?4`, `Pair_0046faf0::Pair_0046faf0`, `Pass`, `PlayerDpid`, `PlayerReady`, `RequestPlayerColor`, `SendChatMessage`, `SendChatPacket`, `SendNetHeartbeat`, `SendShareEnergy`, `SendShareMapInfo`, `SendShareMetal`, `SyncChecksumVector::~SyncChecksumVector`, `SyncPlayerRecord::operator?4`, `SyncTaggedVector::InitTaggedVector`, `SyncTempTaggedVector::SyncTempTaggedVector`, `SyncTempTaggedVector::~SyncTempTaggedVector`, `UnitSync::AllPlayersSynced`, `UnitSync::AllowUnit`, `UnitSync::ApplyToUnitTypes`, `UnitSync::DisallowUnit`, `UnitSync::GetSyncStatusText`, `UnitSync::GetUnitEntry`, `UnitSync::IsPlayerSynced`, `UnitSync::PopChangedEntry`, `UnitSync::ProcessSync`, `UnitSync::SetUnitLimit`, `UnitSync::UnitSync`, `UnitSync::~UnitSync`, `UnitSyncDel_0046c920::~UnitSyncDel_0046c920`, `UnitSyncMap::Buynode`, `UnitSyncMap::Init`, `UnitSyncMap::UnitSyncMap`, `UnitSyncMap::operator?A`, `UnitSyncPlayer::UnitSyncPlayer`, `UnitSyncPlayer::operator?4`, `UnitSyncPlayer::~UnitSyncPlayer`, `VecElems_0046d1a0::_Destroy`, `VecElems_0046d1a0::~VecElems_0046d1a0`, `Vec_0046d1a0::_Destroy`, `Vec_0046d1a0::~Vec_0046d1a0`, `Vec_0046d6c0::size`, `Vec_0046d860::Push`, `Wrap_0046e5e0::Wrap_0046e5e0`, `allocator::destroy`. The lobby, menus and dialogs, which test mode skips. Covered by #398's UI log.
- Console command, host only (4): `CmdCompression`, `GetByteRates`, `PacketManager::NopRet_D`, `SumMessageStats`. Network settings and statistics (`CmdCompression`, `CmdNetStats`), which change no game state, so #432's `console` verb leaves them out. Covered by #398.
- Key, window or screenshot input only (2): `CycleMessageUnits`, `ScrollToNextMessageUnit`. Test mode sends no input. Covered by #398.
- Shutdown only (6): `EmptyAtexitHandler`, `LeaveNetGameCallback`, `NetCondenser::~NetCondenser`, `PacketManager::~PacketManager`, `ReleasePacketData`, `RemoveLocalPlayers`. `ShutdownIngameSystems` and exit paths; test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Command-line switch only (2): `Onl_LoadConfigFile`, `SetPacketRate`. Switches a scenario cannot pass (`-blockscan` and the network options).

**game** (160 functions). Phase root `StepAllGafSequences`. Start-up, load, save and shutdown code, and the main loop: `MainLoopTick`, `RunGameSteps` (which calls every phase root), `UpdateFramePacing`, `BattleFrame`, `EndGameFrame`, `MainFrameTick`, `RunEndGameState` and `UpdateBattleRoom` (the last two pump `HandleNetPackets`) are set to game by `override`, because what drives the step is not presentation. 92 functions are called from outside, by platform (67 distinct functions) and presentation (24); the busiest are `SetCursorMode` (19), `SaveSettings` (9), `SavedGameSelectHandler` (7). It imports 444 functions, the most of any subsystem, 215 of them from presentation, then platform 88, network 32, units 24 and map 22.

**game, functions no scenario enters** (108 of 183 after `save-midgame` (#422) and `allied-victory` (#432); 75 are entered, 41.0%), by reason:

- Network only (2): `PreBattleFrame`, `UpdateBattleRoom`. Needs other machines or a network session; test mode runs every player on one machine. Covered by the replay recordings, #10 and #389.
- Menu or front end only (4): `Force640x480Surfaces`, `ReturnToMainMenuFrame`, `SaveAllMissionsSetting`, `SetGameMode`. The lobby, menus and dialogs, which test mode skips. Covered by #398's UI log.
- Save and load only (12): `DeleteSave_00492360`, `FreeSummaryBank`, `ListSavedGames`, `LoadGameScreenHandler`, `LoadSavedGameState`, `OpenHapiBank`, `OpenSummaryBank`, `SaveGameScreenHandler`, `SavedGameSelectHandler`, `ShowLoadGameScreen`, `ShowSaveGameScreen`, `ShowSavedGameInfo`. Test mode saves (the `save` key, `save-midgame`) but never loads a game. Loading needs a saved game file, which the suite does not have.
- Console command, game logic only (31): `CmdAI`, `CmdATM`, `CmdAssert`, `CmdAssign`, `CmdControl`, `CmdDPrint`, `CmdDoubleShot`, `CmdDrop`, `CmdEdge`, `CmdGive`, `CmdHalfShot`, `CmdILose`, `CmdIWin`, `CmdInclude`, `CmdKill`, `CmdMapping`, `CmdMem`, `CmdMove`, `CmdNowISee`, `CmdPrintWeights`, `CmdReload`, `CmdReloadAIProfiles`, `CmdSeaLevel`, `CmdSearch`, `CmdSelectable`, `CmdShootAll`, `CmdTreeDeath`, `CmdView`, `CmdZBuffer`, `DefaultCommandHandler`, `ExecuteCommandLine`. `ExecuteCommandLine` and `DefaultCommandHandler` are the console path itself, and the rest change game state (players, resources, units, features, the map edge and sea level, damage and visibility options), or change nothing (`CmdAssert`, `CmdDPrint`, `CmdMem`, `CmdZBuffer`). Typed console input; test mode types none. #432 adds a `console` verb.
- Console command, display, sound, debugger or host only (34): `CmdAntiAlias`, `CmdBigBrother`, `CmdCDPlay`, `CmdCDStop`, `CmdClock`, `CmdContour`, `CmdDebugBreak`, `CmdDither`, `CmdFShadow`, `CmdFilm`, `CmdFilmSpeed`, `CmdGamma`, `CmdIFace`, `CmdLight`, `CmdLogo`, `CmdMakePoster`, `CmdMemDump`, `CmdMusicMode`, `CmdNetStats`, `CmdNoShake`, `CmdNow`, `CmdProfile`, `CmdRCache`, `CmdSave`, `CmdScreenChat`, `CmdScrollSpeed`, `CmdSelBoxes`, `CmdSenderror`, `CmdShading`, `CmdShadow`, `CmdSing`, `CmdSound3D`, `CmdSwitchAlt`, `CmdTShadow`. Commands that only change presentation or the host (display and sound settings, the camera, the debugger, files, network statistics). Typed console input; test mode types none, and #432's `console` verb leaves them out. Covered by #398.
- Key, window or screenshot input only (3): `InstallOutOfMemoryHandler`, `SaveCdLists`, `SetGameSpeed`. Test mode sends no input. Covered by #398.
- Shutdown only (8): `AtexitNoOp`, `DestroySimGlobals`, `FreeLogos`, `FreeSideFonts`, `ShutdownGame`, `ShutdownIngameAndQuit`, `ShutdownIngameSystems`, `WriteGameRegistryValue`. `ShutdownIngameSystems` and exit paths; test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Campaign only (4): `ApplyUseOnlyUnits`, `CampaignSetupFrame`, `CreateMissionUnits`, `RunInitialMission`. Game type 1; test mode runs game types 2 and 3 only (the `game_type` key, #432). Type 1 needs the `campaign` key and comes with #444.
- Game over only (2): `EndGameFrame`, `RunEndGameState`. The end-game screen. A run ends at the end of the step that sets the game over flags (`commander-death` and `allied-victory` end at tick 360), before the frame code switches to `EndGameFrame`, so no scenario steps into it; the stats screen itself is #398's.
- Error paths only (5): `Fail`, `Fatal`, `OutOfBlock`, `OutOfMemoryHandler`, `ReportViaException`. Fatal errors and out-of-memory handlers, which a passing run never reaches.
- Test only (2): `QuirksFlipFirstSim`, `SimBlockForEach`. `QuirksFlipFirstSim` is the quirk-freeze check's deliberate failure (`quirks_tamper`), which exits 2 and so has no stored trace. `SimBlockForEach` is called only by the harness's block scan, which `reference.py --check` runs with `-blockscan` and the coverage run does not.
- Moddable-only key (1): `QuirksNormalise`. Entered by a `quirks` line, which the reference harness does not read, so a scenario with one has no stored trace.

**files, functions no scenario enters** (44 of 142 after `save-midgame` (#422); 98 are entered, 69.0%), by reason:

- Network only (4): `LzssCompress`, `LzssDisablePreset`, `LzssEnablePreset`, `LzssExpand`. Needs other machines or a network session; test mode runs every player on one machine. Covered by the replay recordings, #10 and #389.
- Menu or front end only (5): `ChangeDirectory`, `ChangeDrive`, `GetCurrentDriveLetter`, `GetDriveDirectory`, `GetStartDirectory`. The lobby, menus and dialogs, which test mode skips. Covered by #398's UI log.
- Save and load only (8): `AccountBank::GetDoubleItem`, `AccountBank::GetStringItem`, `AccountBank::ReadBox`, `AccountBank::WriteAuditFile`, `HapiBank::GetDoubleItem`, `HapiBank::GetStringItem`, `HapiBank::ReadBox`, `encryptByte`. Test mode saves (the `save` key, `save-midgame`) but never loads a game. Loading needs a saved game file, which the suite does not have.
- Console command only (1): `MatchWildcard`. Reached only through `DefaultCommandHandler` (a unit name typed at the console). #432 adds a `console` verb.
- Key, window or screenshot input only (2): `HAPI_OpenFileAppend`, `HAPI_WriteFile`. Test mode sends no input. Covered by #398.
- Shutdown only (1): `HAPI_CloseArchive`. `ShutdownIngameSystems` and exit paths; test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Error paths only (1): `SquashErrorString`. Fatal errors and out-of-memory handlers, which a passing run never reaches.
- Developer tool only (2): `HAPI_CreateFile`, `RenameFile`. The GUI layout editor.
- Game data only (10): `AccountBank::GetIntegerItem`, `AccountBank::LoadAccount`, `FreeRecord_004be180`, `HAPI_LoadOpenFile`, `HAPI_ReadFileAt`, `HAPI_TellFile`, `HapiBank::GetIntegerItem`, `RemoveFile`, `SquashUnpackedSize`, `WriteBufferToFile`. Entered only with files the stock install lacks (OVR banks, `download` TDFs, briefings) or with a cache missing (palette and shade tables written on first run).

**harness** (58 functions). The test harness of #3 (`src/harness`). No phase root. 10 functions are called from outside; the busiest are `HarnessViewRand` (20), `HarnessTestMode` (4), `HarnessCrtSeed` (2). It imports 11 functions (presentation 5, map 3).

**presentation** (1514 functions). Rendering, UI and sound (`src/graphics`, `src/gui`, `src/frontend`, `src/ingame`, `src/sound`), except what a phase walk reaches, the main loop and frame handlers (game), and the timer, window and environment code of `src/graphics/display.cpp` (platform). No phase root, and none of its functions calls one (a test checks this). 310 functions are called from outside, mostly by game (215 distinct functions); the busiest are `QueueUnitSpeech` (27), `PlaySoundByName` (18), `GetDisplay` (17). It imports 247 functions: platform 85, network 65, game 24, map 24, players 14. The reverse direction is the one to remove: 53 imports from presentation in the nine module subsystems, the largest in los_radar (18), weapons (16) and features (9).

**platform** (838 functions). Devices, files, memory, the TDF and `HapiBank` readers, the console and the C library, and the timers (`GetTicks`, `GetMilliseconds`), window and environment code of `src/graphics/display.cpp` (24 functions, set by `override`). No phase root. 178 functions are called from outside, by game (88 distinct functions), presentation (85), units (56) and map (36); the busiest are `operator new` (138), `GameFreeThunk` (134), `operator delete` (104), then `Translate` (84) and `RandomInt` (60). It imports 138 functions: game 67, presentation 44.

## Coverage

`data/coverage.csv` is the baseline of how much of each subsystem the 51 scenarios of `reference/scenarios/` run, measured with clang source-based coverage of the coverage build (#385). Besides one totals row per subsystem it lists each subsystem's never-entered functions and its dead functions. Raising it is #405.

```sh
tools/coverage_runtime.sh                     # fetch the profile runtime sources of compiler-rt
cmake -S . -B build/coverage -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-i686.cmake -DBT_COVERAGE=ON
cmake --build build/coverage
uv run tools/coverage.py --exe build/coverage/TotalA.exe --game-dir ~/.ta           # play the suite, print the table
uv run tools/coverage.py --exe build/coverage/TotalA.exe --game-dir ~/.ta --check   # fail if a subsystem's coverage fell
uv run tools/coverage.py --exe build/coverage/TotalA.exe --game-dir ~/.ta --write   # replace the baseline
```

The simulation subsystems are in bold. `state` is the shared state block code (`interfaces/ref.h`, `src/state/`), not a simulation subsystem. A function counts as entered when its execution count is above 0; a line is hit when its count is above 0; each branch condition has a true and a false side, and each side taken at least once is covered. Branches are `-` where the subsystem has none. The function counts are the compiler's: they include inline and compiler-generated functions, so they differ from the counts above (`orders` has 372 here and 468 there).

A dead function is one that 3.1c's code cannot reach: the modern link drops it, and no function the link keeps has it inlined. The coverage link keeps every function, because the profile data names each one, so `tools/aliases.py` walks the sections again without the profile data as a root and writes `live_counters.txt` beside the exe; a function that was not entered and whose profile counter is not in that file is dead. A function inlined into a kept function is live, though it has no symbol of its own. Dead functions have their own rows in `data/coverage.csv` (`dead` 1) and are left out of the function, line and branch counts and out of the never-entered column; the dead column gives their number. A static function's profile counter is named by the file's basename and the function (`file.cpp_<name>`), and `display.cpp`, `settings.cpp` and `sound.cpp` each occur twice in the tree, so a static with the same decorated name in both files of such a pair would share a counter name and a dead one could read as live; the baseline shows no such case.

| subsystem | functions entered | functions | lines | branches | never entered | dead |
|---|---|---|---|---|---|---|
| **wind_meteors** | 12 of 16 | 75.0% | 76.9% | 63.2% | 4 | 0 |
| **economy** | 18 of 32 | 56.2% | 63.9% | 37.2% | 14 | 7 |
| **features** | 41 of 53 | 77.4% | 73.3% | 63.3% | 12 | 7 |
| **ai** | 93 of 105 | 88.6% | 88.7% | 81.9% | 12 | 37 |
| **pathfinding** | 45 of 48 | 93.8% | 92.4% | 85.4% | 3 | 10 |
| **weapons** | 74 of 88 | 84.1% | 84.2% | 68.5% | 14 | 20 |
| **targeting** | 10 of 10 | 100.0% | 96.2% | 73.7% | 0 | 1 |
| **los_radar** | 67 of 89 | 75.3% | 79.8% | 69.7% | 22 | 20 |
| **orders** | 294 of 372 | 79.0% | 74.8% | 58.2% | 78 | 28 |
| **units** | 117 of 167 | 70.1% | 74.0% | 61.7% | 50 | 34 |
| **players** | 54 of 143 | 37.8% | 40.0% | 32.3% | 89 | 11 |
| **map** | 75 of 98 | 76.5% | 78.0% | 69.4% | 23 | 12 |
| files | 98 of 142 | 69.0% | 69.7% | 56.7% | 44 | 36 |
| game | 75 of 183 | 41.0% | 40.3% | 22.4% | 108 | 25 |
| network | 49 of 333 | 14.7% | 9.2% | 3.7% | 284 | 81 |
| platform | 314 of 646 | 48.6% | 34.3% | 22.3% | 332 | 82 |
| presentation | 445 of 1138 | 39.1% | 30.4% | 21.4% | 693 | 313 |
| seams | 83 of 103 | 80.6% | 90.6% | - | 20 | 0 |
| state | 315 of 386 | 81.6% | 79.1% | 49.4% | 71 | 22 |

Functions are assigned to a subsystem by `data/subsystems.csv` first, then by the names of special members (`X::X`, `X::~X`, `X::operator<code>`). A function still without a row is `state` when its file is under `interfaces/` or `src/state/`, else it gets the subsystem that the rules of `tools/coupling.py` give its name and file. Functions in `src/harness/`, `build/` and `toolchain/` are left out.

The runtime is clang's profile runtime (`lib/profile` of compiler-rt), built from source for i686 against the MSVC 5 C runtime in `CMakeLists.txt`. It works under Wine, so the fallback of clang's trace-pc-guard instrumentation was not needed. The game ends with `TerminateProcess`, and the runtime's own initialiser is not linked, so `host_exit` writes the profile itself (`__llvm_profile_initialize_file`, then `__llvm_profile_write_file`). `LLVM_PROFILE_FILE` takes a `Z:\` path.

## Notes from the data

- **`order_targets.cpp` is in orders.** It holds the order goal objects (`PathGoal`, `PointMarker`, `RingApproach` and the rest: 100 functions of the file are in orders), so the file rule puts it in `orders`, while #16 lists it for targeting. Targeting is then 10 functions, 8 of them from `unit_targets.cpp`. The pathfinding section shows the same line from the other side: the goals are what pathfinding calls into.
- **`movement_class.cpp` is in units and map.** Its functions are split between `units` (14) and `map` (7, the pass maps) and one in `features`, while #14 lists the file for pathfinding. Pathfinding imports four `MovementClass` functions from units.
- **`unit_targets.cpp` is split four ways** by the name rules: units 11, targeting 8, orders 3, weapons 3.
- **Phases cut across subsystems.** The phase roots are in wind_meteors, economy, features, weapons, units, players and game, and none is in ai, pathfinding, los_radar or orders. `UpdatePlayers` runs economy, ai, pathfinding and los_radar work, and `UpdateAllUnits` runs orders and weapons work. A module's entry point is often called from another subsystem's phase, not from the loop.
- **`g_game` is left out of the read lists.** Every subsystem reads it as the root pointer; the fields behind it are the items listed.

Limits of the data:

- `Game::` fields are keyed by name, not by offset. Each source file declares its own partial view of `struct Game`, so two names for one offset are two items, and two subsystems can share bytes the tables show as separate. A clang layout run would fix this.
- Only strong writes count as writes. An address taken or a pointer passed to a non-const parameter counts as a read, so a setter that writes through a pointer (targeting's) shows as a reader, and the callee's own rows show the write.
- Shared value types (`Vec3`, `Point`, `Fixed`) and unrelated structs with one name (`Grid`, `Pos`) give every subsystem that touches them a row. They are left out of the named lists but still counted in the totals.
- Calls through a function pointer or a virtual method appear only where `data/hazard_edges.csv` resolves them by hand.
- Destructors are missing from the call data until #133, so the cleanup calls of a subsystem are undercounted.
- A function in a mixed folder (`src/game`, `src/map`) is assigned by file, then by phase walk, then by name. The overrides in `data/subsystems.csv` (72) fix the known misses; a wrong assignment shows as a surprising coupling above.
