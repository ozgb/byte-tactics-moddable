# Subsystems

This file is the draft interface of each subsystem, from the coupling map of #128 (part of #6). For the nine subsystems that have a module issue (#9 and #11 to #18) it gives the entry points, the imports (calls out to other subsystems) and the sim read and write sets, so each issue starts from a draft rather than from the source. The issue's own text wins where the two differ, and each section ends with open questions where the data shows a coupling the issue did not expect.

Nothing here is typed by hand except the prose. Every list and count comes from the tool:

```sh
uv run tools/coupling.py                      # regenerate data/subsystems.csv (keeping override) and data/coupling.csv
uv run tools/coupling.py --check              # fail if either file differs from what the tool regenerates (CI runs this)
uv run tools/coupling.py --summary [NAME ...] # print entry points, imports and sim items per subsystem
```

The inputs are `data/rwsets.csv`, `data/hazard_edges.csv` and `data/state_classes.csv` (see `docs/state.md`); the tool's docstring has the rules that assign a function to a subsystem. The 4408 functions of `data/rwsets.csv` are in 19 subsystems, and the sim items are the rows of `data/state_classes.csv` of class `sim` or `mixed`, plus the struct fields classified the same way. A call or a shared item counts functions of the calling subsystem, not call sites.

The subsystems and the issue each maps to:

| subsystem | functions | issue |
|---|---|---|
| [wind_meteors](#wind_meteors) | 16 | #9 |
| [economy](#economy) | 32 | #11 |
| [features](#features) | 47 | #12 |
| [ai](#ai) | 134 | #13 |
| [pathfinding](#pathfinding) | 56 | #14 |
| [weapons](#weapons) | 101 | #15 |
| [targeting](#targeting) | 11 | #16 |
| [los_radar](#los_radar) | 95 | #17 |
| [orders](#orders) | 377 | #18 |
| [units](#the-remaining-subsystems) | 192 | none yet |
| [players](#the-remaining-subsystems) | 144 | none yet |
| [map](#the-remaining-subsystems) | 107 | none yet |
| [network](#the-remaining-subsystems) | 331 | none yet |
| [game](#the-remaining-subsystems) | 210 | none yet |
| [harness](#the-remaining-subsystems) | 172 | none yet |
| [presentation](#the-remaining-subsystems) | 1393 | none yet |
| [platform](#the-remaining-subsystems) | 718 | none yet |
| [seams](#the-remaining-subsystems) | 103 | none yet |
| [files](#the-remaining-subsystems) | 169 | none yet |

The ten subsystems without a module issue are described briefly [below](#the-remaining-subsystems). #10 (per-module replay tests) uses all of them.

How to read a section. **Entry points** are the phase roots the subsystem holds and the functions other subsystems call, with the callers. The main loop and the frame handlers (`MainLoopTick`, `RunGameSteps`, `UpdateFramePacing`, `BattleFrame`, `EndGameFrame`, `MainFrameTick`) are in `game` by `override`, although most are in presentation folders (`src/ingame/`), so a phase root has callers in `game`, not in `presentation`. **Imports** are the functions it calls in other subsystems, by target. **Sim writes** and **sim reads** list items with their class from `data/state_classes.csv`. Lists are cut to the twelve most used named items. "Named" leaves out the shared value types (`Vec3`, `Point`, `Point16`, `Fixed`) and the per-file copies of a struct that the source declares several times (a name ending in an address, such as `Proj_0049c740`).


## wind_meteors

Issue #9. 16 functions. `UpdateWind`, `UpdateMeteors`, the meteor parameters and their save and load (`src/map/meteors.cpp`, plus `UpdateWind` in `src/game/game_state.cpp`).

Coverage: 93.8% of functions (15 of 16), 96.4% of lines, 76.3% of branches; 1 live function no scenario enters, and 0 more are dead (data/coverage.csv).

**Live functions no scenario enters** (1 of 16 after `combat-deaths`, `meteors-earthquake` and `meteors-hailstorm` (#419) and `save-midgame` (#422), and `load-midgame` (#442), and the six `console-` scenarios (#445); 15 are entered, 93.8%), by reason:

- Shutdown only (1): `EmptyShutdownPreCleanup`. `console-features` (#445) enters `CmdMeteor` and `StartMeteorShower` (`meteor` and `meteor 1`).

**Entry points.** Phase roots: `UpdateWind` and `UpdateMeteors`, both called from `RunGameSteps`. `UpdateWind` is also called from `LoadBattleAssets` (game).

10 functions are called from other subsystems. The most called, with their callers:

- `MeteorParams::LoadMeteorDefaults` (1 from map, 1 from seams)
- `CmdMeteor` (1 from platform)
- `DisableMeteors` (1 from map)
- `EmptyShutdownPreCleanup` (1 from game)
- `EnableMeteors` (1 from map)
- `InitMeteors` (1 from game)
- `LoadMeteors` (1 from game)
- By calling subsystem (functions called): game 5, map 4, platform 1, seams 1

**Imports.** 18 functions of 5 subsystems, by target, each with the number of functions it calls there:

- platform (11): `BuildDataPath`, `CommandArgs::GetIntArg`, `FatalError`, `RandomInt`, and 7 more
- files (3): `HapiBank::OpenAccount`, `HapiBank::GetIntegerItem`, `HapiBank::SetIntegerItem`
- weapons (2): `FindWeaponByName`, `SpawnProjectile`
- game (1): `GameRand`
- seams (1): `FindWeaponByNameNative`

**Sim writes.** 24 items in all, 19 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`g_meteorActive` (sim); `g_meteorNextHitTime` (sim); `g_meteorNextStrikeTime` (sim); `g_meteorOrigin` (sim); `g_meteorStrikeEndTime` (sim); `g_meteorTarget` (sim); `g_meteorsEnabled` (sim); `Game::windCounter` (sim); `Game::windDirection` (sim); `Game::windEnabled` (sim); `Game::windSpeed` (sim); `Game::windStrength` (sim)

**Sim reads.** 13 items only read, 13 named. The most used (`g_game`, the root pointer, left out):

`Game::ticks` (mixed); `Game::height` (sim); `Game::width` (sim); `Seams::weapons` (sim); `g_meteorWeaponName` (sim); `DAT_005119b8` (sim); `Game::weapons` (sim); `Game::windSpeedDenominator` (sim); `Game::windSpeedMax` (sim)

**Draft interface.**

- Exports: 10 functions (the two phase roots, `InitMeteors`, `LoadMeteors` and `SaveMeteors` for game, the map controls `EnableMeteors`, `DisableMeteors`, `SetMeteorParams` and `MeteorParams::LoadMeteorDefaults`, and the console command `CmdMeteor`).
- Needs from seams: 18 functions in platform, files, weapons, game, seams.
- Owns: 18 named sim items that only this subsystem writes, for example `g_meteorActive`, `g_meteorNextStrikeTime`, `g_meteorNextHitTime`, `g_meteorOrigin`, `g_meteorStrikeEndTime`.
- Shares: 1 named sim item that other subsystems also write, for example `Game::windCounter`.
- Pilot fit (#9): Most of its imports are platform (TDF parsing, `RandomInt`, `FatalError`, the `HapiBank` account store). The only sim seam is weapons (`SpawnProjectile`, `FindWeaponByName`).
- The wind output is in `Game` fields (`windSpeed`, `windDirection`, `windFraction`, `windVelX`, `windZ`) that only this module writes but units, economy, features and presentation read. The meteor schedule is in `g_meteor*` statics, which must move into the state block (#7).

**Open questions.**

- `SpawnProjectile` is the one call into another sim subsystem. Does the pilot need a weapons seam (#8) before #15 exists, or a stub?
- `RandomInt` is counted as a platform import but draws from a game-logic stream (#3). Should the random streams be an engine service in the seam list instead?

## economy

Issue #11. 32 functions. The resource accounts (`UnitResources`), `UpdatePlayerEconomy`, resource sharing and the share console commands (`src/game/economy*.cpp`, `share_commands.cpp`, `UpdateResourceSharing` in `src/network/net_game.cpp`).

Coverage: 93.1% of functions (27 of 29), 78.1% of lines, 43.8% of branches; 2 live functions no scenario enters, and 4 more are dead (data/coverage.csv).

**Live functions no scenario enters** (2 of 29 after `conditions-dark-side` and `conditions-shore-to-shore` (#420) and `save-midgame` (#422), and `load-midgame` (#442), and the six `console-` scenarios (#445); 27 are entered, 93.1%), by reason:

- Share dialog or network only (1): `ShareMapInfo` (the map-share button of the ally share dialog, or a share packet).
- Network game only (1): `UpdateResourceSharing`. `RunGameSteps` calls it only when `showStats` is set and the packet manager is in use, and it returns at once unless `netGameFlags & 1`.
- Entered by the console: `CmdNoEnergy`, `CmdNoMetal`, `CmdSetShareEnergy`, `CmdSetShareMetal`, `CmdShareAll`, `CmdShareEnergy`, `CmdShareMetal` and `UnitResources::SpendMetal` (through `TransferMetal`, console `Give`). `console-economy` (#445) types them; the share commands do nothing without a network game (`netGameFlags & 1`), so they change no hashed field.

**Entry points.** Phase roots: `UpdateResourceSharing`, called from `RunGameSteps` only when `showStats` is set and the packet manager is in use, and it returns at once unless `netGameFlags & 1` (a network game). The per-tick income step `UpdatePlayerEconomy` is not a root: `UpdatePlayers` (players) calls it, so most of the economy runs inside the players phase.

20 functions are called from other subsystems. The most called, with their callers:

- `GetEnergyUse` (4 from ai)
- `ShareMapInfo` (1 from network, 1 from presentation)
- `UpdateMetalExtraction` (2 from units)
- `CmdNoEnergy` (1 from platform)
- `CmdNoMetal` (1 from platform)
- `CmdSetShareEnergy` (1 from platform)
- `CmdSetShareMetal` (1 from platform)
- By calling subsystem (functions called): seams 8, platform 7, ai 1, game 1, network 1, players 1, presentation 1, units 1

**Imports.** 17 functions of 7 subsystems, by target, each with the number of functions it calls there:

- files (4): `HapiBank::OpenNamedBox`, `HapiBank::SeekBox`, `HapiBank::ReadBox`, `HapiBank::WriteBox`
- network (4): `AddMessage`, `BroadcastPlayerInfo`, `PlayerDpid_00457d30`, `SendPacketToPlayer`
- los_radar (3): `CmdShareMapping`, `CmdShareRadar`, `GetMapCell`
- players (2): `TransferEnergy`, `TransferMetal`
- seams (2): `SetStateBitsNative`, `StartScriptWithArgsNative`
- game (1): `SetStartingStorageBonusInline`
- platform (1): `CommandArgs::GetIntArg`

**Sim writes.** 42 items in all, 41 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Player::energy` (mixed); `Player::metal` (mixed); `UnitResources::energyUse` (mixed); `UnitResources::energyUsePaid` (mixed); `UnitResources::metalUse` (mixed); `Player::energyCapacity` (sim); `Player::energyIncome` (sim); `Player::energyUsage` (sim); `Player::energyWasted` (sim); `Player::metalCapacity` (sim); `Player::metalIncome` (sim); `Player::metalUsage` (sim)

**Sim reads.** 63 items only read, 58 named. The most used (`g_game`, the root pointer, left out):

`Player::active` (mixed); `Player::type` (mixed); `Game::players` (sim); `Game::localPlayer` (mixed); `Game::netGameFlags` (mixed); `UnitResources::energyMake` (mixed); `Game::difficulty` (mixed); `UnitResources::energyStall` (sim); `UnitResources::metalMake` (mixed); `Player::index` (mixed); `Player::info` (mixed); `Game::ticks` (mixed)

**Draft interface.**

- Exports: 20 functions (`UpdatePlayerEconomy`, `UpdateResourceSharing`, the spend and request calls (`UnitResources::SpendEnergy`, `SpendMetal`, `RequestEnergy`, `RequestEnergyAndMetal`), `GetEnergyUse` (ai) and `UpdateMetalExtraction` (units)).
- Needs from seams: 17 functions in files, network, los_radar, players, seams, game, platform.
- Owns: 18 named sim items that only this subsystem writes, for example `UnitResources::energyUse`, `UnitResources::player`, `UnitResources::metalUse`, `PlayerInfo::shareMetal`, `UnitResources::energyUsePaid`.
- Shares: 23 named sim items that other subsystems also write, for example `Player::energy`, `Player::metal`, `Player::energyCapacity`, `Player::metalCapacity`, `Player::shareEnergy`.
- `Player::energy` and `Player::metal` are written by four subsystems (economy, game, players, presentation; class `mixed`), so the economy module cannot own them alone. The statistics (`energyWasted`, `totalMetalProduced` and the rest) are also written by network and players.
- Economy sends packets (`SendPacketToPlayer`, `BroadcastPlayerInfo`, `AddMessage`) and starts COB scripts (`CobScript::StartScriptWithArgs`); both are candidates for events instead of imports.

**Open questions.**

- Which subsystem owns the per-player account fields? A state-block struct for the accounts that economy writes and players and network only read would match the data better than the `Player` struct.
- `GetMapCell`, `ShareMapping` and `ShareRadar` are imports from los_radar: resource sharing also shares vision. Does that sharing belong to los_radar's interface or to economy's?

## features

Issue #12. 47 functions. `UpdateFeatures` and feature loading, spreading, burning, reclaim and wreckage (`src/map/features*.cpp`).

Coverage: 95.5% of functions (42 of 44), 81.9% of lines, 69.6% of branches; 2 live functions no scenario enters, and 4 more are dead (data/coverage.csv).

**Live functions no scenario enters** (2 of 44 after `combat-deaths`, `meteors-earthquake` and `meteors-hailstorm` (#419) and `save-midgame` (#422), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), and the six `console-` scenarios (#445); 42 are entered, 95.5%), by reason:

- Shutdown only (1): `FreeFeaturePool` (called from the line-of-sight teardown).
- Cursor only (1): `GetFeature` (the static in `unit_scripts.cpp`, used by the reclaim checks of `GetOrderCursor`, which no scenario enters).
- Entered by the console: `CmdBurnAll`, `CmdBurnOne`, `CmdFeature` and `RemoveAllFeatures`, by `console-features` (#445).

**Entry points.** Phase roots: `UpdateFeatures`, called from `RunGameSteps`.

23 functions are called from other subsystems. The most called, with their callers:

- `GetCellFeature` (1 from game, 1 from map, 1 from orders)
- `DamageFeature` (1 from network, 1 from seams)
- `PlaceFeature` (1 from los_radar, 1 from units)
- `ReclaimFeature` (2 from orders)
- `CmdBurnAll` (1 from platform)
- `CmdBurnOne` (1 from platform)
- `CmdFeature` (1 from platform)
- By calling subsystem (functions called): game 8, los_radar 5, network 3, orders 3, platform 3, units 2, map 1, seams 1, weapons 1

**Imports.** 48 functions of 8 subsystems, by target, each with the number of functions it calls there:

- platform (16): `TdfRecord::GetFieldString`, `RandomInt`, `TdfFile::~TdfFile`, `operator delete`, and 12 more
- files (9): `HapiBank::GetBoxSize`, `HapiBank::OpenAccount`, `HapiBank::OpenNamedBox`, `HapiBank::SeekBox`, and 5 more
- presentation (9): `InitGafSequence`, `FreeObjectState`, `CreateObjectState`, `EmitTimedSubParticles`, and 5 more
- seams (5): `GetGameTypeNative`, `GetGroundHeightNative`, `ApplyAreaDamageAtNative`, `FindWeaponByNameNative`, and 1 more
- network (4): `BroadcastPacket`, `GetLocalDpid`, `GetHostDpid`, `SendPacketToPlayer`
- game (3): `GameRand`, `SimAllocTagged`, `StepGafSequence`
- los_radar (1): `GetMapCell`
- map (1): `RefreshAllPassMaps`

**Sim writes.** 83 items in all, 71 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Cell::spot` (mixed); `Cell::flags` (mixed); `FeatureDamagePacket::sub` (mixed); `FeatureDamagePacket::type` (mixed); `FeatureDamagePacket::x` (mixed); `FeatureDamagePacket::z` (mixed); `Spot::damage` (mixed); `Spot::feature` (mixed); `Spot::used` (mixed); `Cell::feature` (mixed); `Coord::value` (sim); `Feature::object` (mixed)

**Sim reads.** 58 items only read, 58 named. The most used (`g_game`, the root pointer, left out):

`LosRadarSeam::GetMapCell` (sim); `Seams::los_radar` (sim); `Game::width` (sim); `Game::cells` (mixed); `DAT_005119b8` (sim); `Feature::name` (sim); `Game::height` (sim); `Game::spots` (sim); `Seams::map` (sim); `Feature::flags` (sim); `Feature::footprint` (sim); `Spot::anim` (sim)

**Draft interface.**

- Exports: 23 functions (`UpdateFeatures`, the cell accessor `GetCellFeature`, `PlaceFeature` (los_radar, units), `DamageFeature` (weapons, network), `ReclaimFeature` (orders), and the load, save, pool and file-list calls from game and los_radar).
- Needs from seams: 48 functions in platform, files, presentation, seams, network, game, los_radar, map.
- Owns: 64 named sim items that only this subsystem writes, for example `Game::features`, `Game::featureCount`, `Pool::entries`, `Cell::spot`, `Spot::feature`.
- Shares: 7 named sim items that other subsystems also write, for example `Cell::feature`, `Cell::flags`, `FeatureDamagePacket::sub`, `FeatureDamagePacket::type`, `FeatureDamagePacket::x`.
- It imports from presentation (`EmitWhiteSmoke`, `PlaySoundAtByName`, the GAF loaders, `CreateObjectState`). Sim code that draws or plays sound has to raise events instead (#22).
- Features and weapons call each other: features calls `ApplyAreaDamageAt`, weapons calls `DamageFeature`. `Cell::feature` is also written by los_radar (`ClearFeature` is in `line_of_sight.cpp`, so it is los_radar by file).

**Open questions.**

- `ClearFeature` is a feature operation in a LOS file. Should it move to features, which would remove the second writer of `Cell::feature`?
- `Cell::spot`, `Cell::offsetX` and `Cell::offsetY` are written here and read by los_radar, map, orders and weapons. Are they feature state or part of the cell record that map owns?

## ai

Issue #13. 134 functions. The computer player: `UpdatePlayerAI`, build ratings, weight scripts and squads (`src/ai/ai_player*.cpp`, `player_ai.cpp`, `ai_profile.cpp`, most of `src/units/squads.cpp`).

Coverage: 89.4% of functions (93 of 104), 89.0% of lines, 83.0% of branches; 11 live functions no scenario enters, and 35 more are dead (data/coverage.csv).

**Live functions no scenario enters** (11 of 104 after #420 and `console-sight` (#445); 93 are entered, 89.4%), by reason:

- Shutdown only (4): `DestroyPlayerAI`, `FreeSquads`, `SquadManager::DeleteTimers` and `SquadTimer::~SquadTimer`. All four are called from `FreePlayers`, which `ShutdownIngameSystems` calls and no scenario runs.
- Console command only (1): `DumpPlayerAI` (console `PrintWeights`). It writes the file its second argument names, so the `console` key leaves it out (#445). `console-sight` enters `ResetAIPlayers` (console `ReloadAIProfiles`).
- Network only (2): `ProtectUnitDefsReadOnly` and `ProtectUnitDefsReadWrite`. Only `UnitSync::ApplyToUnitTypes` calls them.
- Needs a case no scenario sets up yet (4): `SumUnitRatingsInRange` (the AI scout probe rating, which `SpatialTimer::OnTimer` calls only when its probe point is visible), `IsExplored`, `IsVisible` and `Offset` (statics in `ai_player.cpp` that only `SpatialTimer::OnTimer` calls). These are not inlined copies of a function that is entered elsewhere: the other `IsExplored`, `IsVisible` and `Offset` statics in the tree take other argument types and are other functions, and coverage counts an inlined body at its source. `OnTimer` returns before them when the spatial group (group 9) holds no unit, so a scenario needs an AI whose script gives that group units.

**Entry points.** Phase roots: none. `UpdatePlayerAI` and `SquadManager::TickIfActive` are called by `UpdatePlayers` (players), so the AI runs inside the players phase.

29 functions are called from other subsystems. The most called, with their callers:

- `SetUnitSquad` (3 from units, 1 from presentation)
- `IsPointVisible` (2 from presentation, 1 from los_radar)
- `CmdLimit` (1 from platform)
- `CmdPlan` (1 from platform)
- `CmdWeight` (1 from platform)
- `CopyDwordPair` (1 from platform)
- `CopyOneByte` (1 from platform)
- By calling subsystem (functions called): players 8, platform 5, seams 5, presentation 4, game 2, los_radar 2, network 2, orders 1, units 1, weapons 1

**Imports.** 39 functions of 11 subsystems, by target, each with the number of functions it calls there:

- platform (19): `RandomInt`, `operator new`, `CommandArgs::GetArg`, `CommandArgs::InitArgs`, and 15 more
- seams (4): `SetWeaponTargetUnitNative`, `UnitTypeSetAddTypeOrCategoryNative`, `GetNameSlotNative`, `SetWeaponTargetPosNative`
- orders (3): `DeleteOrders`, `GetOrderFlags`, `IssueAttackOrder`
- presentation (3): `GetBuildSiteMetal`, `QueueBuildOrder`, `QueueUnitSpeechIfNotVisible`
- targeting (3): `GetWeaponTargetUnit`, `ClearWeaponTarget`, `FindWeaponTarget`
- los_radar (2): `GetMapCell`, `IsUnitVisibleToPlayer`
- economy (1): `GetEnergyUse`
- 4 smaller targets: game (1), players (1), units (1), weapons (1)

**Sim writes.** 29 items in all, 19 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`SquadTimer::next` (sim); `PlayerAI::lastTick` (sim); `SquadManager::countdown` (sim); `Player::groups` (mixed); `SquadManager::cursor` (sim); `SquadManager::nextAction` (mixed); `g_aiCommandsEnabled` (sim); `PlayerAI::builders` (sim); `PlayerAI::centre` (sim); `PlayerAI::hasSpecial` (sim); `Squad::field_8` (sim); `Squad::field_c` (sim)

**Sim reads.** 136 items only read, 131 named. The most used (`g_game`, the root pointer, left out):

`Game::defs` (mixed); `Game::ticks` (mixed); `Game::unitDefCount` (mixed); `Unit::def` (mixed); `UnitDef::flags1` (mixed); `Game::net` (mixed); `Player::index` (mixed); `Player::type` (mixed); `Seams::map` (sim); `Unit::player` (sim); `Player::active` (mixed); `Player::unitsBegin` (mixed)

**Draft interface.**

- Exports: 29 functions (`UpdatePlayerAI`, `SquadManager::TickIfActive`, the create, reset and destroy calls from game, `SetUnitSquad` (units), `SquadManager::MarkOwnerNetDirtyFromDamageSplit` (weapons) and helpers other subsystems borrow: `GetFactoriesInRadius` (orders), `GetVisibleEnemiesInRadius` (orders, targeting), `IsPointVisible` (los_radar)).
- Needs from seams: 39 functions in platform, seams, orders, presentation, targeting, los_radar, economy, game, players, units, weapons.
- Owns: 17 named sim items that only this subsystem writes, for example `g_playerAI`, `SquadManager::timers`, `SquadTimer::next`, `SquadManager::player`, `g_aiCommandsEnabled`.
- Shares: 2 named sim items that other subsystems also write, for example `Unit::flags`, `Unit::group`.
- It acts through other modules: orders (`AddOrder`, `IssueAttackOrder`, `DeleteOrders`), targeting and `Unit::SetStateBits`. Its own sim writes are few.
- Its read set is wide (items of `Unit`, `UnitDef`, `Player` and `Game`). A read-only view of the state block fits it better than a getter per field.

**Open questions.**

- The AI calls the pathfinder's open-list heap (`MakeHeap`, `PopHeapFirst`). Is that heap a pathfinding export or shared library code?
- `GetFactoriesInRadius` and `operator-` are used by orders. Do these helpers belong in a shared maths or query seam instead of ai?

## pathfinding

Issue #14. 56 functions. The search in `src/ai/pathfind*.cpp`: per-player cost tables, the node budget, the open heap and the grid.

Coverage: 93.8% of functions (45 of 48), 92.4% of lines, 87.1% of branches; 3 live functions no scenario enters, and 10 more are dead (data/coverage.csv).

**Live functions no scenario enters** (3 of 48 after #421; 45 are entered, 93.8%), by reason:

- Unreachable (2): `Pathfinder::RemoveNode` and `OpenHeap::SiftDownInline`. `RunSearches` is the only caller of `RemoveNode`, and it zeroes `topPopped` just before the `if (!topPopped)` that guards the call, so the call is dead code in 3.1c. `SiftDownInline` is called only by `RemoveNode`.
- Shutdown only (1): `Pathfinder::~Pathfinder` (called from `DestroyPathfinder` in `ShutdownIngameSystems`). Test mode ends with `host_exit(0)` right after the last tick, so no scenario runs the shutdown.

**Entry points.** Phase roots: none. `Pathfinder::RunSearches` is called by `UpdatePlayers` (players).

5 functions are called from other subsystems. The most called, with their callers:

- `AdjustHeap` (2 from platform)
- `PathGrid::At` (1 from presentation)
- `Pathfinder::AbortIfGoalMatch` (1 from seams)
- `Pathfinder::RunSearches` (1 from seams)
- By calling subsystem (functions called): seams 3, platform 1, presentation 1

**Imports.** 26 functions of 4 subsystems, by target, each with the number of functions it calls there:

- orders (14): `ApproachRadius::ApproxDistExcess`, `OrderFx::ApproxDist`, `PointMarker::ApproxDist`, `RingApproach::ApproxDistExcess`, and 10 more
- seams (6): `AiSearchGoalSetWaypointsNative`, `MovementClassGetNative`, `MovementClassInBoundsNative`, `OrderFxAddFlagsNative`, and 2 more
- platform (4): `operator delete`, `GameFreeThunk`, `operator new`, `operator==`
- game (2): `SimAlloc`, `SimAllocTagged`

**Sim writes.** 46 items in all, 42 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Node::index` (sim); `PathCell::flags` (sim); `OpenHeap::count` (sim); `OpenHeap::freeHead` (sim); `OpenHeap::topPopped` (sim); `Pathfinder::object` (mixed); `Pathfinder::owner` (mixed); `OpenHeap::used` (sim); `PathCell::dir` (sim); `Pathfinder::steps` (sim); `NodeData::f` (sim); `NodeData::g` (sim)

**Sim reads.** 32 items only read, 30 named. The most used (`g_game`, the root pointer, left out):

`Pathfinder::grid` (sim); `Seams::orders` (sim); `Seams::units` (sim); `OrdersSeam::AiSearchGoalSetWaypoints` (sim); `Game::height` (sim); `Game::players` (sim); `Game::width` (sim); `MovementClass::footprintX` (mixed); `MovementClass::footprintZ` (mixed); `NodeData::pos` (sim); `Unit::motion` (mixed); `UnitsSeam::RefreshUnitIfStale` (sim)

**Draft interface.**

- Exports: 5 functions (`Pathfinder::RunSearches` (players), `Pathfinder::AbortIfGoalMatch` (orders), `MakeHeap` and `PopHeapFirst` (ai)).
- Needs from seams: 26 functions in orders, seams, platform, game.
- Owns: 40 named sim items that only this subsystem writes, for example `OpenHeap::items`, `OpenHeap::pool`, `Node::index`, `OpenHeap::count`, `Grid::dirty`.
- Shares: 2 named sim items that other subsystems also write, for example `Grid::cells`, `Grid::width`, `Grid::count`, `Grid::height`.
- Few functions are called from outside, so the entry side is small. The import side is not: many of its imports are goal-object methods (`PathGoal`, `PointMarker`, `RingApproach`, `OrderFx`, `ApproachRadius`, `AiSearchGoal`) that orders implements. The goals are a callback interface that orders passes in.
- The budget (`Pathfinder::budget`, `g_playerBudgetCap`, `g_budgetRefreshCounter`, `g_playerTickLoad`) is state this module writes alone: the limit #14 wants the module to read from outside.

**Open questions.**

- `Grid::cells`, `width` and `height` show as written by pathfinding, los_radar and map. Either `Grid` is one name for unrelated structs (type-wide evidence) or there is a real shared grid; a clang layout run would tell.
- The goals live in `order_targets.cpp` (orders) but only pathfinding calls `ApproxDist` and `ContainsCell`. Should the goal interface be declared by pathfinding?

## weapons

Issue #15. 101 functions. `UpdateProjectiles`, `UpdateExplosions`, `UpdateUnitWeapons`, ballistics, damage and the weapon definitions (`src/weapons/*.cpp`).

Coverage: 84.1% of functions (74 of 88), 84.6% of lines, 70.4% of branches; 14 live functions no scenario enters, and 17 more are dead (data/coverage.csv).

**weapons, functions no scenario enters** (14 of 88 after the scenarios of `reference/scenarios/`; 74 are entered, 84.1%), by reason:

- Shutdown only (3): `FreeExplosions`, `FreeWeaponArray` and `FreeWeaponTypes`. `ShutdownIngameSystems` calls them, and test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Network only (5): `ApplyProjectileHitPacket`, `ApplyWeaponFirePacket`, `FindRemoteProjectile_0049d270`, `SamePos` and `SamePos_0049d270`. `ApplyProjectileHitPacket` and `ApplyWeaponFirePacket` are the handlers of the projectile packets of `net_game.cpp`, and the other three are statics only they call. Test mode runs every player on one machine.
- Base-class slots of `CobScript` that `UnitScript` overrides (2): `CobScript::ExplodeLegacy` and `CobScript::ExplodePiece`. Every unit's script is a `UnitScript`, so the base bodies are never called.
- Needs a case no scenario sets up yet (4): `Length` (the static of `weapons.cpp` that `GetProjectileAimPoint` calls only for a cruise weapon), `??8@YG_NABVStringRef@@0@Z` and `Ne` (the `StringRef` comparison and its caller in the weapon loader, reached only when the damage table of a weapon already holds a key that sorts at or after the new one) and `UnitScript::ExplodeLegacy` (the empty body of COB opcode 0x10009000, which no scenario's script runs).

**Entry points.** Phase roots: `UpdateProjectiles` and `UpdateExplosions`, called from `RunGameSteps`. `UpdateUnitWeapons` is not a root: `UpdateAllUnits` (units) calls it.

28 functions are called from other subsystems. The most called, with their callers:

- `DamageUnit` (4 from orders, 4 from units, 2 from players)
- `WeaponCanReachUnit` (5 from orders, 1 from targeting)
- `FindFeatureAtPos` (3 from orders, 1 from players)
- `DetonateUnitWeapon` (1 from players, 1 from units)
- `FindWeaponByName` (1 from seams, 1 from wind_meteors)
- `WeaponAimCobCb::OnAimCobReturn` (2 from units)
- `AllocWeaponArray` (1 from game)
- By calling subsystem (functions called): game 8, units 8, seams 5, network 3, orders 3, players 3, wind_meteors 2, ai 1, targeting 1

**Imports.** 68 functions of 12 subsystems, by target, each with the number of functions it calls there:

- platform (27): `RandomInt`, `GameFreeThunk`, `operator new`, `operator-`, and 23 more
- presentation (16): `EmitWeaponSmoke`, `PlaySoundAt`, `EmitWhiteSmoke`, `AccumulateScreenShake`, and 12 more
- seams (9): `StartScriptWithArgsNative`, `StartScriptNative`, `GetCellMeanHeightNative`, `MarkOwnerNetDirtyFromDamageSplitNative`, and 5 more
- game (3): `GameRand`, `SimAllocTagged`, `StepGafSequence`
- network (3): `BroadcastPacket`, `GetLocalDpid`, `SendScriptCallByName`
- units (3): `GetPieceOffset`, `GetPiecePosition`, `MarkRecentlyDamaged`
- los_radar (2): `GetMapCellAtPosition`, `GetMapCell`
- 5 smaller targets: ai (1), features (1), files (1), orders (1), targeting (1)

**Sim writes.** 203 items in all, 156 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Projectile::flags` (mixed); `Projectile::targetUnit` (mixed); `Game::projectileCount` (mixed); `Angles16::heading` (mixed); `Projectile::speed` (sim); `WeaponFirePacket::aimOrVel` (sim); `WeaponFirePacket::origin` (sim); `WeaponFirePacket::type` (sim); `WeaponFirePacket::weaponDefIndex` (sim); `Angles16::pitch` (mixed); `Game::tracked` (mixed); `Game::trackedPos` (mixed)

**Sim reads.** 184 items only read, 136 named. The most used (`g_game`, the root pointer, left out):

`Seams::units` (sim); `Unit::def` (mixed); `Unit::heading` (sim); `Game::ticks` (mixed); `Unit::player` (sim); `Game::seaLevel` (mixed); `UnitsSeam::StartScriptWithArgs` (sim); `Unit::script` (mixed); `Game::gravity` (sim); `Unit::playerIndex` (sim); `Game::net` (mixed); `Unit::id` (sim)

**Draft interface.**

- Exports: 28 functions (the two roots, `UpdateUnitWeapons`, `Unit::ClaimWeapons` and `Unit::ReleaseWeapons` (orders), `DamageUnit` (orders, players, units), `WeaponCanReachUnit`, `WeaponCanReachPos`, `FindWeaponByName`, `SpawnProjectile` and the net packet handlers `ApplyWeaponFirePacket` and `ApplyProjectileHitPacket`).
- Needs from seams: 68 functions in platform, presentation, seams, game, network, units, los_radar, ai, features, files, orders, targeting.
- Owns: 131 named sim items that only this subsystem writes, for example `Game::projectiles`, `WeaponDef::index`, `Game::projectileCount`, `WeaponDef::speed`, `WeaponFirePacket::aimOrVel`.
- Shares: 25 named sim items that other subsystems also write, for example `Game::cameraSnapTicks`, `Unit::flags`, `g_explodedPieces`, `Game::count`, `Game::cameraFollowTrackObj`.
- A long import list: presentation (smoke, sound, screen shake, GAF and model loading) and units (COB scripts, piece positions) are among its targets.
- Most of its writes are not shared state: many `WeaponDef::*` fields written when the TDFs load, packet buffers (`WeaponFirePacket`, `UnitDamagePacketOut`) and per-file struct copies. The shared state is `Game::projectiles`, `Game::projectileCount`, `Unit::health`, `Unit::flags` and the weapon slot fields.
- It also writes view state: `Game::cameraSnapPos` and `cameraSnapTicks` (from `DetonateProjectile` and `CheckProjectileCollision`).

**Open questions.**

- `DamageUnit` writes `Unit::health`, which game, orders, players and units also write. Is damage a weapons export or a units operation that weapons requests? #15 already asks which machine applies damage in multiplayer.
- `Unit::ClaimWeapons` is called for orders only, through a seam. It is a lifecycle call, not a per-tick one; does it belong to a unit lifecycle seam?

## targeting

Issue #16. 11 functions. Target choice and the weapon target setters and getters (`FindWeaponTarget`, `FindBestTargetIfFireAtWill`, `SetWeaponTargetUnit`). It is small because the file rules put the order goal objects in orders; see the notes.

Coverage: 100.0% of functions (10 of 10), 96.2% of lines, 73.7% of branches; 0 live functions no scenario enters, and 1 more is dead (data/coverage.csv).

**Entry points.** Phase roots: none. Order handlers (orders) and the AI call it.

9 functions are called from other subsystems. The most called, with their callers:

- `ClearWeaponTarget` (5 from orders, 1 from ai)
- `GetWeaponTargetUnit` (3 from orders, 2 from ai, 1 from weapons)
- `FindBestTargetIfFireAtWill` (1 from seams)
- `FindWeaponTarget` (1 from ai)
- `GetWeaponTargetPos` (1 from seams)
- `ResetWeaponTarget` (1 from units)
- `SetWeaponTargetPos` (1 from seams)
- By calling subsystem (functions called): seams 5, ai 3, orders 2, units 1, weapons 1

**Imports.** 10 functions of 4 subsystems, by target, each with the number of functions it calls there:

- seams (6): `FindScriptNative`, `StartScriptWithArgsNative`, `GetGroundHeightNative`, `GetSweetSpotNative`, and 2 more
- platform (2): `Bits::Test`, `RandomInt`
- presentation (1): `Class_004800c0::EraseSwapBack`
- weapons (1): `WeaponCanReachUnit`

**Sim writes.** 7 items in all, 2 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`UnitWeaponSlot::aimTargetXOrUnitId` (sim); `UnitWeaponSlot::aimTargetZOrUnitSentinel` (sim)

**Sim reads.** 39 items only read, 34 named. The most used (`g_game`, the root pointer, left out):

`Unit::weapons` (sim); `Game::units` (mixed); `Seams::units` (sim); `Unit::script` (mixed); `Unit::unitDefIndex` (mixed); `UnitWeaponSlot::flags` (mixed); `UnitsSeam::FindScript` (sim); `UnitsSeam::StartScriptWithArgs` (sim); `AiSeam::GetVisibleEnemiesInRadius` (sim); `Game::matchFlags` (sim); `Game::seaLevel` (mixed); `Player::active` (mixed)

**Draft interface.**

- Exports: 9 functions (all of them are target setters, getters and searches; `ClearWeaponTarget` and `GetWeaponTargetUnit` are the busiest).
- Needs from seams: 10 functions in seams, platform, presentation, weapons.
- Owns: 2 named sim items that only this subsystem writes, for example `UnitWeaponSlot::aimTargetXOrUnitId`, `UnitWeaponSlot::aimTargetZOrUnitSentinel`.
- Shares: 0 named sim items that other subsystems also write.
- It writes the target position of the weapon slot (`UnitWeaponSlot`) and no other named sim item; its other written items are `Vec3` and `Point` value types. The setters probably write the rest of the target through a pointer, which counts as a read.
- It reaches units through seams (COB script calls, `GetGroundHeightNative` and `GetSweetSpotNative`) and imports the range check from weapons (`WeaponCanReachUnit`) and `RandomInt` and `Bits::Test` from platform.

**Open questions.**

- With no state of its own, is targeting a set of functions over the weapon slot, and so part of weapons, as #16 allows?
- `unit_targets.cpp` is one file in 3.1c, split here by name between units, targeting, orders and weapons. The split is the rule's guess and needs a decision in #16.

## los_radar

Issue #17. 95 functions. Line of sight, radar and sonar, the LOS tables, the map cell accessor and the TNT loader (`src/map/line_of_sight*.cpp`, `los_tables*.cpp`, `radar.cpp`, and two `src/game/players_46*.cpp` files).

Coverage: 88.9% of functions (64 of 72), 82.8% of lines, 72.9% of branches; 8 live functions no scenario enters, and 22 more are dead (data/coverage.csv).

**Live functions no scenario enters** (8 of 72 after `sensors-los-off`, `los-off-mapped-ai` and `hailstorm-los-off` (#421), `sensors-sonar-jam` and `orders-tier2-core` (#426) and `sensors-floating-teeth` and `sensors-floating-teeth-los-off` (#437), and the six `console-` scenarios (#445); 64 are entered, 88.9%), by reason:

- Shutdown only (4): `FreeRadar`, `FreeMapResources`, `LosTables::FreeTables` and `LosTables::~LosTables`. `FreeRadar` and `FreeMapResources` are called from `ShutdownIngameSystems`, which no scenario runs because test mode ends with `host_exit(0)` right after the last tick. `FreeTables` has no caller but `FreeLosTables`, which nothing calls, and the destructor is that of the static table, which never runs, because `host_exit` ends the process with `TerminateProcess`.
- Network only (2): `CmdShareMapping` and `CmdShareRadar` (the share commands, which all check `netGameFlags & 1`, a network game). Test mode runs a skirmish.
- Menu and cursor only (2): `ResizeRadarPicture` (the multiplayer lobby map preview) and `Visible` (the mouse hover cursor of `GetOrderCursor`).
- Entered by the console: `CmdLOS`, `CmdLOSType` and `CmdRadar`, by `console-sight` (#445).

**Entry points.** Phase roots: none. `UpdatePlayers` (players) calls `RecalculateLineOfSight`, `UpdateUnitLineOfSight` and `UpdateSensorRadarAndCloak`, so LOS runs inside the players phase.

30 functions are called from other subsystems. The most called, with their callers:

- `GetMapCell` (13 from features, 2 from weapons, 1 from ai, 1 from economy, 1 from map, 1 from network, 1 from presentation, 1 from units)
- `GetMapCellAtPosition` (3 from weapons, 2 from orders, 1 from game, 1 from map, 1 from presentation)
- `IsUnitVisibleToPlayer` (4 from presentation, 1 from ai, 1 from units)
- `RecalculateLineOfSight` (3 from game, 1 from players, 1 from presentation)
- `UpdateUnitLineOfSight` (2 from units, 1 from orders, 1 from players)
- `CmdShareMapping` (1 from economy, 1 from platform)
- `CmdShareRadar` (1 from economy, 1 from platform)
- By calling subsystem (functions called): presentation 8, game 6, map 6, players 6, units 6, platform 5, economy 3, orders 3, ai 2, network 2, weapons 2, features 1

**Imports.** 47 functions of 7 subsystems, by target, each with the number of functions it calls there:

- platform (16): `GameFreeThunk`, `StringRef::Assign`, `operator delete`, `GameAllocIgnoreTag`, and 12 more
- presentation (15): `GetGafFrame`, `DrawFrame`, `AllocFrame`, `AllocSurface`, and 11 more
- features (5): `FreeFeaturePool`, `InitFeatureAnimPool`, `PlaceFeature`, `PlaceMissionFeatures`, and 1 more
- game (4): `Game::Current`, `SaveSettings`, `SimAlloc`, `SimAllocTagged`
- seams (3): `GetGroundHeightNative`, `GetNameSlotNative`, `VisitObjectsInRangeNative`
- ai (2): `HasReadyUnitInRange`, `IsPointVisible`
- network (2): `AddMessage`, `BroadcastPlayerInfo`

**Sim writes.** 109 items in all, 101 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Game::count_14277` (mixed); `SightQuery::cacheCell` (mixed); `SightQuery::eyeHeight` (mixed); `SightQuery::frameIdx` (mixed); `SightQuery::player` (mixed); `SightQuery::pos` (mixed); `SightQuery::sightDistance` (mixed); `Unit::flags` (mixed); `Eye::expires` (mixed); `Eye::flagA` (mixed); `Eye::flagPtr` (mixed); `Eye::player` (mixed)

**Sim reads.** 110 items only read, 99 named. The most used (`g_game`, the root pointer, left out):

`Game::width` (sim); `Game::height` (sim); `Game::playerIndex` (mixed); `Game::seaLevel` (mixed); `Game::losTable` (sim); `Player::exploredWidth` (sim); `Unit::def` (mixed); `Player::explored` (sim); `FrameTable::count` (sim); `Game::eyes` (sim); `Unit::player` (sim); `Game::grid1` (sim)

**Draft interface.**

- Exports: 30 functions (the three players-phase calls, `GetMapCell` and `GetMapCellAtPosition`, `IsUnitVisibleToPlayer`, `RevealNewUnit`, `ShareMapping` and `ShareRadar` (economy), and the load, free and console calls).
- Needs from seams: 47 functions in platform, presentation, features, game, seams, ai, network.
- Owns: 88 named sim items that only this subsystem writes, for example `SightQuery::sightDistance`, `Game::heightMap`, `SightQuery::cacheCell`, `SightQuery::frameIdx`, `Game::visibilityMask`.
- Shares: 13 named sim items that other subsystems also write, for example `Grid::height`, `Grid::width`, `GafFrame::xOffset`, `GafFrame::yOffset`, `Grid::cells`.
- Many imports are presentation (`AllocSurface`, `DrawCircle`, `DrawFrame`, `DownsampleFrame`) and platform. The file rules put the radar picture and minimap code here (`DrawRadarUnits`, `ResizeRadarPicture`, the `Game::minimapGadget*` writes), so most of its functions are likely view code. This is the boundary #17 asks to keep clear.
- The result is `Game::visibilityMask`, which ai, economy, map, orders, pathfinding and presentation read. `Player::explored` is read here and written elsewhere.

**Open questions.**

- `GetMapCell` has callers in many subsystems. It is the map cell accessor, not a LOS function. Should map own it, with los_radar importing it?
- `Game::visibilityMask` is read by pathfinding and ai as well as presentation. Which part is sim state and which is the player view (#22)?

## orders

Issue #18. 377 functions. The order state machines, VTOL orders, order lists, unit motion (`UnitMotion`) and the order goal objects (`src/orders/*.cpp`). The largest subsystem.

Coverage: 83.1% of functions (305 of 367), 79.4% of lines, 61.6% of branches; 62 live functions no scenario enters, and 26 more are dead (data/coverage.csv).

**Live functions no scenario enters** (62 of 367 after `orders-immediate`, `orders-factory` and `orders-air` (#418), the three scenarios of #419, the three of #421, the ten of #426 and `save-midgame` (#422), `load-midgame` (#442), and `units-transport` and `units-transport-ship` (#428), and `mission-defeat`, `mission-victory` and `mission-radius` (#444); 305 are entered, 83.1%), by reason:

- Network only (22): `LiteGoal::HasNetUnitState`, `LiteGoal::SerializeNetUnitState`, `AiSearchGoal::HasNetUnitState`, `AiSearchGoal::SerializeNetUnitState`, `PackedPosGoal::PackedPosGoal`, `PackedPosGoal::~PackedPosGoal`, `PackedPosGoal::DeserializeNetUnitState`, `PathGoal::DeserializeNetUnitState`, `PathGoal::HasNetUnitState`, `PathGoal::SerializeNetUnitState`, `PathGoal::ExportGoalPose`, `PathGoal::FillWaypointWorldPos`, `PathGoal::HasReadyWaypoints`, `PathGoal::TickTowardGoal`, `PathGoal::TryClaimRepath`, `PatrolGoal::PatrolGoal`, `PatrolGoal::DeserializeNetUnitState`, `PatrolGoal::FillWaypointWorldPos`, `PatrolGoal::HasReadyWaypoints`, `OrderFx::WriteBits`, `AirManeuverOrder::SerializeToBits` and `PathOrder::SerializeToBits`. These are the net-state methods of the goal objects and the bit writers. `PatrolGoal` and `PackedPosGoal` are made only for a player of type 3 (a remote human), and the `PathGoal` methods only run through them.
- Save and load only (18): `KindByIndex_0043a420`, `SerializeSave`, `OrderFx::OrderFx`, `OrderFx::GetType`, `AirManeuverOrder::AirManeuverOrder`, `AirManeuverOrder::AirManeuverOrder`, `AirManeuverOrder::GetType`, `AirManeuverOrder::SerializeToSave`, `ApproachRadius::ApproachRadius`, `ApproachRadius::GetType`, `ApproachRadius::Serialize`, `PathOrder::PathOrder`, `PathOrder::PathOrder`, `PathOrder::GetType`, `PathOrder::SerializeToSave`, `RingApproach::RingApproach`, `RingApproach::GetType` and `RingApproach::Serialize`. The constructors are the load constructors. `save-midgame` enters the save side of `Order`, `PointMarker` and `UnitMotion`, and `load-midgame` (#442) enters `LowerBoundOrderTypes`, `OrderTypeNameLess`, `KindByName_0043a420`, `ResolveUnitTypeKey`, `UnitMotion::LoadMotion`, the load constructors of `Order` and `PointMarker`, and `Order::ReattachFxToUnit`.
- Info panel, cursor, build menu and shutdown only (8): `GetOrderName`, `GetOrderTarget`, `GetBuildWeaponPercent` (info panel), `SumQueuedBuildCount` (build menu count text), `GetOrderCursor` (cursor), `ClearOrderTypeTable` and `DestroyPathfinder` (shutdown), and `Marked` (the smart right-click order, mode 1, which the scenario `order` line has no verb for).
- Campaign mission script only (1): `WaitForAttackOrder`. `mission-defeat`, `mission-victory` and `mission-radius` (#444) enter `AttackUTypeOrder`, `WaitOrder` and `MakeSelectableOrder`.
- No caller in the game (13): `RepairUnitNoMoveOrder`, `ReadyOrder`, `ClearTargets_0043b7c0`, `TryGetDesiredHeading`, `VtolGetRepairedOrder`, `Sub`, `OrderFx::ApproxDist`, `OrderFx::ContainsCell`, `OrderFx::Destroy`, `OrderFx::FillGoalCells`, `OrderFx::IsFxStyle`, `AirManeuverOrder::IsFxStyle` and `PathOrder::IsFxStyle`. Nothing creates `VtolGetRepairedOrder` (only its definition and table row name it). The `Sub` is the one of `unit_orders.cpp` at about line 1433 (the one at about line 1989 is `TeleportOrder`'s helper and is entered). The last five are base-class slots of `OrderFx`, also of `AirManeuverOrder` and `PathOrder`.

**Entry points.** Phase roots: none. `UpdateAllUnits` (units) calls `RunOrders`; units also call `UnitMotion::UpdateMotion`.

59 functions are called from other subsystems. The most called, with their callers:

- `CopyDwordIfNonNull` (4 from platform)
- `Order::OrStatusFlags` (4 from units)
- `ApproachRadius::ApproxDistExcess` (3 from pathfinding)
- `OrderFx::ApproxDist` (3 from pathfinding)
- `PointMarker::ApproxDist` (3 from pathfinding)
- `RingApproach::ApproxDistExcess` (3 from pathfinding)
- `DeleteOrder` (1 from harness, 1 from presentation)
- By calling subsystem (functions called): units 15, pathfinding 14, presentation 12, seams 8, game 4, ai 3, map 3, harness 2, platform 1, weapons 1

**Imports.** 94 functions of 13 subsystems, by target, each with the number of functions it calls there:

- seams (26): `ClaimWeaponsNative`, `ReleaseWeaponsNative`, `SetStateBitsNative`, `SetWeaponTargetUnitNative`, and 22 more
- units (13): `GetNanoPiecePosition`, `AttachUnitToPiece`, `AddBuildProgress`, `AddRepairProgress`, and 9 more
- platform (12): `operator new`, `RandomInt`, `GetHeadingBetween`, `operator==`, and 8 more
- network (10): `BitReader::ReadBits`, `BitWriter::WriteBits`, `SendScriptCallNoArgs`, `BitReader::ReadBit`, and 6 more
- files (7): `HapiBank::OpenNamedBox`, `HapiBank::SeekBox`, `HapiBank::ReadBox`, `HapiBank::WriteBox`, and 3 more
- map (7): `CanPlaceUnitFootprint`, `IsPadSlotFree`, `PickRandomReclaimableResourcesInRadius`, `SnapWorldPosToFootprint`, and 3 more
- presentation (6): `QueueUnitSpeech`, `EmitNanoParticles`, `MarkSelectionOrdersDirty`, `EmitReverseNanoParticles`, and 2 more
- 6 smaller targets: features (3), los_radar (3), weapons (3), targeting (2), ai (1), players (1)

**Sim writes.** 127 items in all, 118 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Order::flags` (mixed); `Order::flags_42` (mixed); `Order::pos` (mixed); `Order::state` (mixed); `Order::next` (mixed); `Order::subFlags` (mixed); `Order::unit` (mixed); `Unit::workTime` (mixed); `Box::hi` (mixed); `Order::attached` (mixed); `AiSearchGoal::flag_3` (mixed); `Order::angle` (mixed)

**Sim reads.** 283 items only read, 220 named. The most used (`g_game`, the root pointer, left out):

`Unit::def` (mixed); `Seams::units` (sim); `Unit::motion` (mixed); `Order::target` (sim); `Seams::weapons` (sim); `Unit::list` (sim); `UnitDef::flags1` (mixed); `WeaponsSeam::ClaimWeapons` (sim); `Unit::list2` (sim); `Unit::player` (sim); `Game::ticks` (mixed); `Seams::targeting` (sim)

**Draft interface.**

- Exports: 59 functions (`RunOrders`, `RunSecondaryOrders`, `AddOrder`, `DeleteOrders`, `GetOrderType`, `IssueAttackOrder`, `OrderType::GetTableEntry` (presentation), the `UnitMotion::` functions called from outside (`UpdateMotion`, `LoadMotion`, `SaveMotion`, `DestroyObject`), and the goal methods pathfinding calls).
- Needs from seams: 94 functions in seams, units, platform, network, files, map, presentation, features, los_radar, weapons, targeting, ai, players.
- Owns: 89 named sim items that only this subsystem writes, for example `Order::state`, `Order::pos`, `Order::next`, `PathOrderAttach::owner`, `Order::unit`.
- Shares: 29 named sim items that other subsystems also write, for example `Order::flags`, `Unit::pos`, `Unit::flags`, `Unit::health`, `Unit::buildLeft`.
- The units imports are the unit lifecycle and geometry: `CreateUnit`, `SetUnitPosition`, `Unit::SetStateBits`, COB script calls and piece positions. The weapons imports are mostly the claim and release of weapons, which orders reaches through seams.
- It is the heaviest writer of core unit fields: `Unit::pos` and `Unit::flags`. The fields are also written by units and presentation (`pos`), and by many other subsystems (`flags`).

**Open questions.**

- Which side owns `Unit::pos`, `flags`, `health` and `buildLeft` in the state block (#7): units, with orders going through its seam, or orders? This decides whether #18 can split.
- The split #18 allows would follow the `UnitMotion::` functions. Is motion a module of its own?

## The remaining subsystems

Each has no module issue yet. Entry points and couplings come from the same tool (`uv run tools/coupling.py --summary NAME`); the numbers count functions.

**units** (192 functions). Phase root `UpdateAllUnits`, which calls `RunOrders` (orders) and `UpdateUnitWeapons` (weapons). 63 functions are called from outside; the busiest are `FindUnitTypeId` (18), `GetNanoPiecePosition` (15), `AttachUnitToPiece` (10). It is called most by game (22 distinct functions) and seams (18). It imports 173 functions, mostly platform (46), presentation (32), seams (19) and files (17). It holds the unit table and lifecycle code (`CreateUnit`, `Unit::SetStateBits`, COB scripts), and it is the other side of most of orders' writes. Coverage: 80.4% of functions (131 of 163), 84.5% of lines, 71.1% of branches; 32 live functions no scenario enters, and 33 more are dead (data/coverage.csv). The list of those 32 is under **units, functions no scenario enters** below.

**units, functions no scenario enters** (32 of 163 after `combat-deaths`, `meteors-earthquake` and `meteors-hailstorm` (#419) and `save-midgame` (#422), `load-midgame` (#442), and `units-transport` and `units-transport-ship` (#428), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), and the six `console-` scenarios (#445); 131 are entered, 80.4%), by reason:

- Base-class slots of `CobScript` that `UnitScript` overrides (11): `CobScript::AttachUnit`, `CobScript::DropUnit`, `CobScript::EmitSfx`, `CobScript::GetTransporterId`, `CobScript::GetUnitValue`, `CobScript::IsCarryingUnit`, `CobScript::IsPieceCached`, `CobScript::IsPieceShaded`, `CobScript::IsPieceVisible`, `CobScript::PlaySoundNoop`, `CobScript::SetUnitValue`. Every unit's script is a `UnitScript`, so the base bodies are never called.
- COB opcodes that no script of the game data uses (4): `UnitScript::IsCarryingUnit`, `UnitScript::GetTransporterId`, `UnitScript::PlaySoundNoop`, `Channel::XorOp`. No unit can reach them.
- Load only (1): `Elem_00487af0` (a sort helper of the load). `save-midgame` (#422) enters the save side (`SaveUnits`, `CobScript::SaveScriptState` and the `UnitScript::IsPiece` slots it calls), and `load-midgame` (#442) enters `LoadUnits`, `LoadUnit` and `CobScript::LoadScriptState`.
- Network only (4): `SendUnitStates`, `WriteUnitState`, `ReceiveUnitStates` and `ReadUnitState`.
- Shutdown only (8): `FreeUnitMemory`, `FreeUnitTypes`, `FreeUnitInfo`, `FreeUnitCategories`, `FreeDownloadMenus`, `FreeMovementClasses` and the two statics only it calls, `FreeA` and `FreeC`.
- Console command only (1): `Selectable` (the console table entry; `CmdSelectable` loops over the units itself and never calls it, and the orders path has its own static). `console-units` (#445) enters `KillAllUnits`, `KillUnitsOfType` and `ReloadUnitType`.
- Unit info dialog only (2): `MakePropList` and its static `spd`.
- Slot no code calls (1): `AimCobStub` (slot 1 of `g_weaponAimCobVtable`). It returns 0; not explained further.

**players** (144 functions). Phase root `UpdatePlayers`, which calls into economy (`UpdatePlayerEconomy`), ai (`UpdatePlayerAI`, squads), pathfinding (`RunSearches`) and los_radar (its LOS calls) in turn. 40 functions are called from outside, mostly by game (14) and presentation (14); the busiest are `KillPlayerUnits` (5), `ShutdownScoreTables` (4), `TransferEnergy` (4). It imports 79 functions: platform 14, network 12, seams 11, files 10, presentation 9. It holds the per-player state and victory conditions. Coverage: 51.7% of functions (74 of 143), 52.8% of lines, 44.2% of branches; 69 live functions no scenario enters, and 11 more are dead (data/coverage.csv). The list of those 69 follows this paragraph.

**players, functions no scenario enters** (69 of 143 after `conditions-dark-side` and `conditions-shore-to-shore` (#420), `save-midgame` (#422) and `allied-victory` (#432), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), and the six `console-` scenarios (#445); 74 are entered, 51.7%), by reason:

- Game type 1 (campaign) only (14): `DefeatAllUnitsKilled::IsSatisfied`, `DefeatAllUnitsKilled::VisitUnit`, `DefeatAnyUnitPassesX::IsSatisfied`, `DefeatAnyUnitPassesX::VisitUnit`, `DefeatAnyUnitPassesZ::IsSatisfied`, `DefeatAnyUnitPassesZ::VisitUnit`, `UnitList::ForEach`, `VictoryBuildUnitType::IsSatisfied`, `VictoryBuildUnitType::VisitUnit`, `VictoryTimerRunsOut::IsSatisfied`, `VictoryUnitTypePassesX::IsSatisfied`, `VictoryUnitTypePassesX::VisitUnit`, `VictoryUnitTypePassesZ::IsSatisfied`, `VictoryUnitTypePassesZ::VisitUnit`. `CheckVictory` and `CheckDefeat` read the condition lists only for game type 1, which `mission-defeat`, `mission-victory` and `mission-radius` (#444) run through the `campaign` key. The three missions carry only some of the condition classes, so the others in this list belong to mission maps no scenario loads. `SetStartingStorageBonus` and its static `AtLeast200` are entered from `InitPlayerResources` under game type 1 (the other call site is in `UpdatePlayers` under `commanderDeath == 2`, which the harness cannot set).
- Mission maps only (9): `DefeatAllUnitsKilledOfType::VisitUnit`, `DefeatAnyUnitPassesX::DefeatAnyUnitPassesX`, `DefeatAnyUnitPassesZ::DefeatAnyUnitPassesZ`, `VictoryCaptureUnitType::OnUnitCaptured`, `VictoryKillAllMobileUnits::OnUnitDied`, `VictoryKillAllMobileUnits::VisitUnit`, `VictoryKillAllOfType::OnUnitDied`, `VictoryKillAllOfType::VisitUnit`, `VictoryKillUnitType::OnUnitDied`. `RegisterConditions` creates these classes from mission condition keys in the map file (`CommanderKilled`, `KillEnemyCommander`, `AllUnitsKilledOfType` and so on), in every game type. Three skirmish maps carry a condition key, all in `totala2.hpi`: Dark Side and Lava Alley carry `KillEnemyCommander`, and Shore to Shore carries `CommanderKilled`. The `conditions-dark-side` and `conditions-shore-to-shore` scenarios run them, entering `VictoryKillEnemyCommander::OnUnitDied` and `DefeatCommanderKilled::OnUnitDied`. The keys behind the constructors and handlers (`AllUnitsKilledOfType`, `UnitTypeKilled`, `AnyUnitPassesX`, `AnyUnitPassesZ`, `DeathTimerRunsOut`, `CaptureUnitType`, `KillAllMobileUnits`, `KillAllOfType` and `KillUnitType`) appear on no skirmish map, only on mission maps: the campaign maps of `totala4.hpi` and `ccmiss.ccx`, and the mission maps of `tactics1.hpi` to `tactics8.hpi`. The mission maps have only `Easy`, `Medium` and `Hard` schemas, so they load under game type 1 only (under game type 2, `Mission::SelectSchema` wants a `Network N` schema). `mission-defeat` and `mission-victory` (Sub Hunting) enter `DefeatUnitTypeKilled`, `DefeatAllUnitsKilledOfType` and `VictoryDestroyAllUnits`, and `mission-radius` (Battle of Ansapp Sea) enters `VictoryMoveUnitToRadius`, `DefeatUnitTypeKilled` and `DefeatDeathTimerRunsOut`. Their `VisitUnit` slots are reached from their own `OnUnitDied`.
- Save and load only (36): `DefeatAllUnitsKilled::LoadState`, `DefeatAllUnitsKilled::SaveState`, `DefeatAllUnitsKilledOfType::LoadState`, `DefeatAllUnitsKilledOfType::SaveState`, `DefeatAnyUnitPassesX::LoadState`, `DefeatAnyUnitPassesX::SaveState`, `DefeatAnyUnitPassesZ::LoadState`, `DefeatAnyUnitPassesZ::SaveState`, `DefeatCommanderKilled::LoadState`, `DefeatCommanderKilled::SaveState`, `DefeatDeathTimerRunsOut::LoadState`, `DefeatDeathTimerRunsOut::SaveState`, `DefeatUnitTypeKilled::LoadState`, `DefeatUnitTypeKilled::SaveState`, `VictoryBuildUnitType::LoadState`, `VictoryBuildUnitType::SaveState`, `VictoryCaptureUnitType::LoadState`, `VictoryCaptureUnitType::SaveState`, `VictoryDestroyAllUnits::LoadState`, `VictoryDestroyAllUnits::SaveState`, `VictoryKillAllMobileUnits::LoadState`, `VictoryKillAllMobileUnits::SaveState`, `VictoryKillAllOfType::LoadState`, `VictoryKillAllOfType::SaveState`, `VictoryKillEnemyCommander::LoadState`, `VictoryKillEnemyCommander::SaveState`, `VictoryKillUnitType::LoadState`, `VictoryKillUnitType::SaveState`, `VictoryMoveUnitToRadius::LoadState`, `VictoryMoveUnitToRadius::SaveState`, `VictoryTimerRunsOut::LoadState`, `VictoryTimerRunsOut::SaveState`, `VictoryUnitTypePassesX::LoadState`, `VictoryUnitTypePassesX::SaveState`, `VictoryUnitTypePassesZ::LoadState`, `VictoryUnitTypePassesZ::SaveState`. `save-midgame` (#422) enters `MissionConditions::SaveConditions` and `SavePlayers`, but the save side of a condition runs only for a condition the map has, and The Pass has none, so the `SaveState` slots and their `LoadState` slots stay listed. `load-midgame` (#442) enters `LoadPlayerControllers`, `LoadPlayers` and `MissionConditions::LoadConditions`.
- Network only (7): `AllocScoreTables`, `EnableReporter`, `FillScoreTables`, `IsReporterDllLoaded`, `LoadReporterDll`, `ShutdownScoreTables` (the online score reporter of the lobby and of `UnitSync`) and `PlayerRecordAt` (a static of `DrawSyncStatus`, the load-progress strip drawn only for game type 3 while waiting for other machines).
- Menu or dialog click only (2): `ContinueWatchingCallback` (the continue-watching button after the local player is defeated) and `SaveNumSkirmishPlayers` (called from the skirmish menu, which test mode skips).
- Shutdown only (1): `FreePlayers` (called from `ShutdownIngameSystems`, which no scenario runs).
- Entered by the console: `TransferEnergy` and `TransferMetal` (console `Give`, `console-economy`) and `MissionConditions::Deactivate` (console `Kill`, `console-units`) (#445).

**map** (107 functions). No phase root. 51 functions are called from outside, by presentation (24), seams (19), game (10) and orders (7); the busiest are `Mission::GetGameType` (21), `Mission::LoadMissionByName` (14), `SetMissionType` (12) and `Mission::GetMissionName` (10) (mission accessors). It imports 67 functions: platform 29, files 12, los_radar 6, game 4, wind_meteors 4. It holds the terrain, the pass maps and footprints. Coverage: 80.6% of functions (79 of 98), 86.8% of lines, 75.9% of branches; 19 live functions no scenario enters, and 12 more are dead (data/coverage.csv).

**map, functions no scenario enters** (19 of 98 after `sensors-los-off`, `los-off-mapped-ai` and `hailstorm-los-off` (#421) and `save-midgame` (#422), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444); 79 are entered, 80.6%), by reason:

- Load only (1): `ForceNeighborFootprintReclaim` (called from `LoadUnit`). `save-midgame` (#422) enters the save side, and `load-midgame` (#442) enters `LoadMetalPlotmap`, `LoadPlayerFeaturesPlotmap` and `LoadMappingData` (the saved game files).
- Shutdown only (2): `FreeOtaEnumCacheAndMission` and `Vector_00438480::~Vector_00438480`. The first is called at the end of `ShutdownIngameSystems`, which no scenario runs because test mode ends with `host_exit(0)` right after the last tick. The second is the destructor of the static `g_missionOrderTableVec`, which never runs, because `host_exit` ends the process with `TerminateProcess`.
- Network lobby and map check only (6): `Mission::ComputeMapChecksum`, `MapCacheEntry::MapCacheEntry`, `MapCacheEntry::MapCacheEntry` (the two constructors), `MapCacheEntry::operator?4`, `MapCacheEntry::SetChecksum` (all four only from `ComputeMapChecksum`) and `Mission::GetTerrainLength` (the map check of a joining player).
- Multiplayer lobby and menus only (5): `Mission::GetTerrainSizeTier` (the lobby colour), `Mission::GetDescription`, `Mission::GetPlanet`, `Mission::GetTranslatedName` and `Mission::HasMissionName` (the map list and mission menus). Test mode calls only `LoadMissionByName`.
- Campaign menu, end screen and restart only (5): `Mission::GetBriefing`, `Mission::SelectMission`, `Mission::MissionExists`, `Mission::AdvanceMission` and `Mission::GetMissionIndex`.

**network** (331 functions). Phase root `HandleNetPackets`, which dispatches into every subsystem, so what it reaches says nothing about the owner. 95 functions are called from outside, mostly by presentation (54) and game (31); the busiest are `BroadcastPacket` (23), `BroadcastPlayerInfo` (20), `AddMessage` (14). It imports 116 functions: platform 48, presentation 23, players 11, units 10. Its packet handlers write many sim items (238 in all).

**network, functions no scenario enters** (237 of 287 after `save-midgame` (#422) and `allied-victory` (#432), and the six `console-` scenarios (#445); 50 are entered, 17.4%), by reason:

- Network only (149): `??$copy_backward@PAHPAH@std@@YGPAHPAH00@Z`, `??$copy_backward@PAVUnitSyncPlayer@@PAV1@@std@@YGPAVUnitSyncPlayer@@PAV1@00@Z`, `??$fill@PAHH@std@@YGXPAH0ABH@Z`, `??$fill@PAVUnitSyncPlayer@@V1@@std@@YGXPAVUnitSyncPlayer@@0ABV1@@Z`, `AddNetPlayer`, `AssignPlayerColor`, `AssignStartPositions`, `BitReader::ReadBit`, `BitReader::ReadBits`, `BitReader::ReadSignedBits`, `BitWriter::BitWriter`, `BitWriter::FreeBuffer`, `BitWriter::GrowBuffer`, `BitWriter::SetByteAt`, `BitWriter::WriteBits`, `BroadcastAllyTeam`, `BroadcastPendingViewState`, `CheckDuplicatePlayerIds`, `CheckPlayerTimeouts`, `CloseNetSession`, `CommandAllowed`, `CopyWords`, `CountActiveAIPlayers`, `CountMessage`, `CountPacket`, `CreateNetGame`, `CreateUnitFromPacket`, `DefaultPacketHandler`, `DequeueByCalls`, `DropPlayer`, `EnumPlayersCallback`, `FindActiveId_00453010`, `FindConnectedPlayerId`, `FindIndex_00452960`, `FindIndex_00453010`, `FindLobbyHostDpid`, `FindLobbyHostPlayer`, `FindLobbyHostPlayerId`, `FindOccupied_004568c0`, `FindSlot_00450a10`, `FindSlot_00452570`, `FindSlot_00453a50`, `FirstJoinedId`, `FirstJoinedId_004568c0`, `FormatNetStats`, `FrameQueue::QueueFrames`, `FrameQueue::ResetFrames`, `FrameQueue::~FrameQueue`, `FrameRing::Pop`, `FrameRing::Push`, `FreeTeam`, `GetCreatedLobbyInterface`, `GetFrame`, `GetHostDpid`, `GetLocalHumanDpid`, `GetPlayerDpid`, `GetSessionGuidInstance`, `HandlePing`, `HandlePlayerEconomy`, `HandleTimeoutDialog`, `InGame`, `InfoPacket`, `InitLobbiedConnection`, `InitNetConnection`, `InitPacketManager`, `IsActive12_00452960`, `IsColorFree`, `IsConnected`, `IsReusable`, `IsState2_00452960`, `IsState3_00452960`, `IsTimedOut`, `IsType1_00452cc0`, `IsType3_00452960`, `IsType3_00452cc0`, `IsValid`, `JoinLobbyGame`, `JoinLobbyGameEntry`, `JoinLobbyGameThread`, `JoinNetGame`, `LeaveNetGame`, `LocalPlayer`, `NetCondenser::Accumulate`, `NetCondenser::ReceivePacket`, `NetCondenser::SendPacket`, `NetCondenser::SendPacketTo`, `Next_00462f30`, `OpenTimeoutDialog`, `PacketBuffer::AppendPacket`, `PacketBuffer::FreePackets`, `PacketBuffer::RemovePacket`, `PacketChannel::AddPacket`, `PacketChannel::AllocBuffer`, `PacketChannel::AllocPacket`, `PacketChannel::EnqueuePacket`, `PacketChannel::GrowPools`, `PacketChannel::InitPools`, `PacketChannel::SendQueued`, `PacketChannel::~PacketChannel`, `PacketManager::AppendToSendBuffer`, `PacketManager::FindChannel`, `PacketManager::HandleIntegrityNop`, `PacketManager::QueueOnChannel`, `PacketManager::QueuePacket`, `PacketManager::SetDefaultSendPacing`, `PacketReceiver::FindPlayerFrameInfo`, `PacketReceiver::ReceiveFrame`, `PacketReceiver::~PacketReceiver`, `PacketRing::Peek`, `PacketRing::Pop`, `PacketRing::PopPacket`, `PacketRing::Push`, `PacketRing::PushPacket`, `PacketTrace`, `PeekFrame`, `Player::FreeSideDataAndFogSightCounts`, `Player::IsPlayableSlot`, `PlayerDpid_00457d30`, `PlayerFrameInfo::Initialize`, `PlayerFrameInfo::~PlayerFrameInfo`, `Prev_00462f30`, `QueryOnlineFlags`, `QueueCount`, `RIReport`, `RIReportGameChat`, `RISetCallbacks`, `ReceiveNetPacket`, `RejectPacketType2`, `RejectPacketType5`, `RejectPacketType6`, `RejectPacketType7`, `RejectPacketType8`, `RejectPlayer`, `RemovePlayer`, `Remove_00452cc0`, `ReportGameChat`, `ReportPacketGap`, `SendLobbySyncRequests`, `SetAlliance`, `TakeFrame`, `UnitAt`, `UnitSync::CheckUnitAvailable`, `UnitSync::HandleSyncPacket`, `UnitSync::NotifyEntryChanged`, `UnitSync::ReceiveSyncPacket`, `UnitSync::SendSyncPacket`, `UnitSyncMap::FindExact`, `UnitSyncMap::LowerBound`, `UpdateTimeoutDialog`. Needs other machines or a network session; test mode runs every player on one machine. Covered by the replay recordings, #10 and #389.
- Menu or front end only (74): `??$copy@PAVUnitSyncPlayer@@PAV1@@std@@YGPAVUnitSyncPlayer@@PAV1@00@Z`, `??$copy@PBUElem_0046faf0@@PAU1@@std@@YGPAUElem_0046faf0@@PBU1@0PAU1@@Z`, `AreAllPlayersReady`, `Class_00470250::Capacity`, `Class_00470250::count`, `Class_00470270::Size`, `Class_00470270::assign_first`, `Class_00470270::assign_second`, `Class_00470270::begin`, `Class_00470270::begin`, `Class_00470270::burn`, `Class_00470270::end`, `CopyDwordRangeOverwrite`, `CountComputerPlayers`, `CountHumanPlayers`, `CountLocalComputerPlayers`, `CreateLocalPlayer`, `CreateUnitSync`, `DeleteUnitSync`, `FindPlayerInUse`, `FindPlayer_00457a50`, `FindReadyPlayer`, `FindTarget`, `FinishUnitSync`, `FlagOf_0046d040`, `GetRejectReasonText`, `IsHostLocal`, `IsHuman`, `IsPlaying`, `Less_0046d6c0::operator?R`, `NopChecksumEntryDtor`, `OnlineGetLinkInfo`, `OnlineProcessButtonCommand`, `OnlineTranslate`, `PacketSequencer::Inl`, `PacketSequencer::PacketSequencer`, `PacketSequencer::SendUnsequenced`, `PacketSequencer::operator?4`, `Pass`, `RequestPlayerColor`, `SendChatMessage`, `SendChatPacket`, `SendNetHeartbeat`, `SendShareMapInfo`, `SyncChecksumVector::~SyncChecksumVector`, `SyncPlayerRecord::operator?4`, `UnitSync::AllPlayersSynced`, `UnitSync::AllowUnit`, `UnitSync::ApplyToUnitTypes`, `UnitSync::DisallowUnit`, `UnitSync::End`, `UnitSync::Find`, `UnitSync::GetSyncStatusText`, `UnitSync::GetUnitEntry`, `UnitSync::IsPlayerSynced`, `UnitSync::PopChangedEntry`, `UnitSync::ProcessSync`, `UnitSync::SetUnitLimit`, `UnitSync::UnitSync`, `UnitSync::~UnitSync`, `UnitSyncIter::UnitSyncIter`, `UnitSyncIter::operator?8`, `UnitSyncMap::Buynode`, `UnitSyncMap::End`, `UnitSyncMap::Erase`, `UnitSyncMap::Init`, `UnitSyncMap::UnitSyncMap`, `UnitSyncMap::operator?A`, `UnitSyncMap::~UnitSyncMap`, `UnitSyncPlayer::UnitSyncPlayer`, `UnitSyncPlayer::UnitSyncPlayer`, `UnitSyncPlayer::operator?4`, `UnitSyncPlayer::~UnitSyncPlayer`, `allocator::destroy`. The lobby, menus and dialogs, which test mode skips. Covered by #398's UI log.
- Console command, host only (4): `CmdCompression`, `GetByteRates`, `PacketManager::NopRet_D`, `SumMessageStats`. Network settings and statistics (`CmdCompression`, `CmdNetStats`), which change no game state, so #445's `console` key leaves them out. Covered by #398.
- Key, window or screenshot input only (2): `CycleMessageUnits`, `ScrollToNextMessageUnit`. Test mode sends no input. Covered by #398.
- Shutdown only (6): `EmptyAtexitHandler`, `LeaveNetGameCallback`, `NetCondenser::~NetCondenser`, `PacketManager::~PacketManager`, `ReleasePacketData`, `RemoveLocalPlayers`. `ShutdownIngameSystems` and exit paths; test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Command-line switch only (2): `Onl_LoadConfigFile`, `SetPacketRate`. Switches a scenario cannot pass (`-blockscan` and the network options).
- Entered by the console scenarios (#445): `SendShareEnergy`, `SendShareMetal`, `SendPacketToPlayer`, `FindIndex_00451bc0`, `PlayerReady` and `Player::SetType`, which the console commands `Give` and `AI` reach.

**game** (210 functions). Phase root `StepAllGafSequences`. Start-up, load, save and shutdown code, and the main loop: `MainLoopTick`, `RunGameSteps` (which calls every phase root), `UpdateFramePacing`, `BattleFrame`, `EndGameFrame`, `MainFrameTick`, `RunEndGameState` and `UpdateBattleRoom` (the last two pump `HandleNetPackets`) are set to game by `override`, because what drives the step is not presentation. 111 functions are called from outside, by platform (73 distinct functions) and presentation (26); the busiest are `SetCursorMode` (19), `SimAllocTagged` (18), `AllocSite::~AllocSite` (12). It imports 469 functions, the most of any subsystem, 216 of them from presentation, then platform 94, network 31, files 23 and units 22.

**game, functions no scenario enters** (70 of 183 after `save-midgame` (#422) and `allied-victory` (#432), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), and the six `console-` scenarios (#445); 113 are entered, 61.7%), by reason:

- Network only (2): `PreBattleFrame`, `UpdateBattleRoom`. Needs other machines or a network session; test mode runs every player on one machine. Covered by the replay recordings, #10 and #389.
- Menu or front end only (4): `Force640x480Surfaces`, `ReturnToMainMenuFrame`, `SaveAllMissionsSetting`, `SetGameMode`. The lobby, menus and dialogs, which test mode skips. Covered by #398's UI log.
- Save and load only (8): `DeleteHapiBank`, `ListSavedGames`, `LoadGameScreenHandler`, `SaveGameScreenHandler`, `SavedGameSelectHandler`, `ShowLoadGameScreen`, `ShowSaveGameScreen`, `ShowSavedGameInfo`. Test mode saves (the `save` key, `save-midgame`) and loads that save (the `load` key, `load-midgame`, #442), which enters `FreeSummaryBank`, `LoadSavedGameState`, `OpenHapiBank` and `OpenSummaryBank`. The screens and handlers of the save and load menus are never entered.
- Console command, game logic only (2): `CmdInclude` (it reads `debugdat\include.txt`) and `CmdPrintWeights` (it writes the file its second argument names). The `console` key leaves both out (#445): a scenario reads no file beyond the game data and writes none. The other 29 functions of this group, `ExecuteCommandLine` and `DefaultCommandHandler` among them, are entered by the six `console-` scenarios (#445).
- Console command, display, sound, debugger or host only (33): `CmdAntiAlias`, `CmdBigBrother`, `CmdCDPlay`, `CmdCDStop`, `CmdClock`, `CmdContour`, `CmdDebugBreak`, `CmdDither`, `CmdFShadow`, `CmdFilm`, `CmdFilmSpeed`, `CmdGamma`, `CmdIFace`, `CmdLight`, `CmdLogo`, `CmdMakePoster`, `CmdMemDump`, `CmdMusicMode`, `CmdNetStats`, `CmdNoShake`, `CmdProfile`, `CmdRCache`, `CmdSave`, `CmdScreenChat`, `CmdScrollSpeed`, `CmdSelBoxes`, `CmdSenderror`, `CmdShading`, `CmdShadow`, `CmdSing`, `CmdSound3D`, `CmdSwitchAlt`, `CmdTShadow`. Commands that only change presentation or the host (display and sound settings, the camera, the debugger, files, network statistics). Typed console input; the `console` key (#445) leaves them out, except `CmdNow`, the cheat code `Now Film Chris Include Reload Assert` that enables the debug commands, which every console scenario types. Covered by #398.
- Key, window or screenshot input only (3): `InstallOutOfMemoryHandler`, `SaveCdLists`, `SetGameSpeed`. Test mode sends no input. Covered by #398.
- Shutdown only (8): `AtexitNoOp`, `DestroySimGlobals`, `FreeLogos`, `FreeSideFonts`, `ShutdownGame`, `ShutdownIngameAndQuit`, `ShutdownIngameSystems`, `WriteGameRegistryValue`. `ShutdownIngameSystems` and exit paths; test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Game over only (2): `EndGameFrame`, `RunEndGameState`. The end-game screen. A run ends at the end of the step that sets the game over flags (`commander-death` and `allied-victory` end at tick 360), before the frame code switches to `EndGameFrame`, so no scenario steps into it; the stats screen itself is #398's.
- Error paths only (5): `Fail`, `Fatal`, `OutOfBlock`, `OutOfMemoryHandler`, `ReportViaException`. Fatal errors and out-of-memory handlers, which a passing run never reaches.
- Test only (2): `QuirksFlipFirstSim`, `SimBlockForEach`. `QuirksFlipFirstSim` is the quirk-freeze check's deliberate failure (`quirks_tamper`), which exits 2 and so has no stored trace. `SimBlockForEach` is called only by the harness's block scan, which `reference.py --check` runs with `-blockscan` and the coverage run does not.
- Moddable-only key (1): `QuirksNormalise`. Entered by a `quirks` line, which the reference harness does not read, so a scenario with one has no stored trace.

**files, functions no scenario enters** (30 of 142 after `save-midgame` (#422), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), and the six `console-` scenarios (#445); 112 are entered, 78.9%), by reason:

- Network only (6): `LzssCompress`, `LzssDisablePreset`, `LzssDisablePreset`, `LzssEnablePreset`, `LzssEnablePreset`, `LzssExpand`. Needs other machines or a network session; test mode runs every player on one machine. Covered by the replay recordings, #10 and #389.
- Menu or front end only (5): `ChangeDirectory`, `ChangeDrive`, `GetCurrentDriveLetter`, `GetDriveDirectory`, `GetStartDirectory`. The lobby, menus and dialogs, which test mode skips. Covered by #398's UI log.
- Save and load only (2): `AccountBank::WriteAuditFile`, `encryptByte`. Test mode saves (the `save` key, `save-midgame`) and loads that save (the `load` key, `load-midgame`, #442), which enters the `GetDoubleItem`, `GetStringItem` and `ReadBox` readers of `AccountBank` and `HapiBank`.
- Entered by the console: `MatchWildcard`, reached only through `DefaultCommandHandler` (a unit name typed at the console), by `console-units` (#445).
- Key, window or screenshot input only (4): `HAPI_OpenFileAppend`, `HAPI_OpenFileAppend`, `HAPI_WriteFile`, `HAPI_WriteFile`. Test mode sends no input. Covered by #398.
- Shutdown only (2): `HAPI_CloseArchive`, `HAPI_CloseArchive`. `ShutdownIngameSystems` and exit paths; test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Error paths only (1): `SquashErrorString`. Fatal errors and out-of-memory handlers, which a passing run never reaches.
- Developer tool only (2): `HAPI_CreateFile`, `RenameFile`. The GUI layout editor.
- Game data only (8): `FreeRecord_004be180`, `HAPI_LoadOpenFile`, `HAPI_LoadOpenFile`, `HAPI_TellFile`, `RemoveFile`, `RemoveFile`, `WriteBufferToFile`, `WriteBufferToFile`. Entered only with files the stock install lacks (OVR banks, `download` TDFs, briefings) or with a cache missing (palette and shade tables written on first run). `AccountBank::GetIntegerItem`, `AccountBank::LoadAccount`, `HapiBank::GetIntegerItem`, the other copy of `HAPI_TellFile` and `SquashUnpackedSize` are entered by the load of `load-midgame` (#442).

**state, functions no scenario enters** (52 of 345 after the scenarios of `reference/scenarios/`; 293 are entered, 84.9%), by reason:

- Template instances no entered function uses (51): `??8@YG_NABU?$Ref@D@@PAD@Z`, `??8@YG_NABU?$Ref@G@@PAG@Z`, `??8@YG_NABU?$Ref@UProjectile@@@@PAUProjectile@@@Z`, `??8@YG_NABU?$Ref@UUnit_0043e490@@@@PAUUnit_0043e490@@@Z`, `??8@YG_NABU?$Ref@VOrderFx@@@@PAVOrderFx@@@Z`, `??8@YG_NABU?$Ref@VPlayerFrameInfo@@@@PAVPlayerFrameInfo@@@Z`, `??9@YG_NABU?$Ref@D@@PAD@Z`, `??9@YG_NABU?$Ref@UWeaponDef@@@@PAUWeaponDef@@@Z`, `??9@YG_NABU?$Ref@VPlayerFrameInfo@@@@PAVPlayerFrameInfo@@@Z`, `??9@YG_NABU?$Ref@VUnitDef@@@@PAVUnitDef@@@Z`, `Ref::Ref`, `Ref::Ref`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?D`, `SimAllocator::deallocate`, `SimAllocator::destroy`, `SimAllocator::destroy`, `SimAllocator::destroy`, `SimAllocator::destroy`, `SimAllocator::destroy`, `SimAllocator::destroy`. These are the `Ref` accessors and comparisons of `interfaces/ref.h` and the `SimAllocator` hooks of `src/state/sim_allocator.h`, one instance for each type and member. No function a scenario enters calls them.
- Error paths only (1): `RefOutsideBlock`. The fatal error of a `Ref` whose pointer lies outside the state block, which a passing run never reaches.

**harness** (172 functions). The test harness of #3 (`src/harness`). No phase root. 10 functions are called from outside; the busiest are `HarnessViewRand` (21), `HarnessCrtSeed` (2), `HarnessTestMode` (2). It imports 51 functions (game 15, platform 14).

**presentation** (1393 functions). Rendering, UI and sound (`src/graphics`, `src/gui`, `src/frontend`, `src/ingame`, `src/sound`), except what a phase walk reaches, the main loop and frame handlers (game), and the timer, window and environment code of `src/graphics/display.cpp` (platform). No phase root, and none of its functions calls one (a test checks this). 310 functions are called from outside, mostly by game (216 distinct functions); the busiest are `QueueUnitSpeech` (27), `PlaySoundByName` (18), `GetDisplay` (16). It imports 336 functions: platform 156, network 54, files 31, game 26, map 24. The reverse direction is the one to remove: the nine module subsystems import from presentation, weapons, los_radar and features most of all. Its coverage varies between runs in two functions. `UpdateCdCategoryByActivity` samples the sim's CD-activity counts on a wall-clock timer (`GetTicks`, which reads `host_time_ms`), so which of its branches run depends on how fast the run went. `Lock` in `src/graphics/palette.cpp` waits on the blit-lock event only when another thread holds the lock, so its wait line and branch run in some runs and not others. `data/coverage.csv` marks both with an `unstable:` row and `tools/coverage.py` leaves their lines and branches out of the presentation totals; `FindGafEntry` also read as varying because a lost counter update makes llvm-cov report a saturated count, which the tool now ignores.

**platform** (718 functions). Devices, files, memory, the TDF and `HapiBank` readers, the console and the C library, and the timers (`GetTicks`, `GetMilliseconds`), window and environment code of `src/graphics/display.cpp` (set by `override`). No phase root. 290 functions are called from outside, by presentation (156 distinct functions), game (94), network (48) and units (46); the busiest are `GameFreeThunk` (164), `operator new` (123), `operator delete` (86), then `Translate` (78) and `GameAllocIgnoreTag` (77). It imports 170 functions: game 73, presentation 46.

**seams** (103 functions). No phase root. The native side of the seam function tables of #8 (`src/seams`). 98 functions are called from outside. It imports 103 functions.

**files** (169 functions). No phase root. The archive and bank readers (`src/files`: `HapiBank`, HPI and SQSH archives). 78 functions are called from outside.

## Coverage

`data/coverage.csv` is the baseline of how much of each subsystem the scenarios of `reference/scenarios/` run, measured with clang source-based coverage of the coverage build (#385). Besides one totals row per subsystem it lists each subsystem's never-entered functions and its dead functions. Raising it is #405.

```sh
tools/coverage_runtime.sh                     # fetch the profile runtime sources of compiler-rt
cmake -S . -B build/coverage -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-i686.cmake -DBT_COVERAGE=ON
cmake --build build/coverage
uv run tools/coverage.py --exe build/coverage/TotalA.exe --game-dir ~/.ta           # play the suite, print the table
uv run tools/coverage.py --exe build/coverage/TotalA.exe --game-dir ~/.ta --check   # fail if a subsystem's coverage fell
uv run tools/coverage.py --exe build/coverage/TotalA.exe --game-dir ~/.ta --write   # replace the baseline
```

Every column counts functions the way `tools/coverage.py` does, which is not the way the table at the top does: `entered of live functions` and `never entered` count the live functions of the compiler's export (inline and generated ones included) and `dead` the functions 3.1c cannot reach, while the table at the top counts every function of `data/rwsets.csv`. The `Coverage:` line of each section above counts the same way as this table. The simulation subsystems are in bold. `state` is the shared state block code (`interfaces/ref.h`, `src/state/`), not a simulation subsystem. A function counts as entered when its execution count is above 0; a line is hit when its count is above 0; each branch condition has a true and a false side, and each side taken at least once is covered. Branches are `-` where the subsystem has none. The function counts are the compiler's: they include inline and compiler-generated functions, so they differ from the counts above.

A dead function is one that 3.1c's code cannot reach: the modern link drops it, and no function the link keeps has it inlined. The coverage link keeps every function, because the profile data names each one, so `tools/aliases.py` walks the sections again without the profile data as a root and writes `live_counters.txt` beside the exe; a function that was not entered and whose profile counter is not in that file is dead. A function inlined into a kept function is live, though it has no symbol of its own. Dead functions have their own rows in `data/coverage.csv` (`dead` 1) and are left out of the function, line and branch counts and out of the never-entered column; the dead column gives their number. A static function's profile counter is named by the file's basename and the function (`file.cpp_<name>`), and `display.cpp`, `settings.cpp` and `sound.cpp` each occur twice in the tree, so a static with the same decorated name in both files of such a pair would share a counter name and a dead one could read as live; the baseline shows no such case.

A live function whose lines or branches follow timing (a wall-clock sampler, a wait on a lock another thread holds) is marked by hand with a row of `functions` 0, `dead` 0 and a reason starting `unstable:`. The function stays in the function counts, and its lines and branches are left out of the totals and the covered counts of its subsystem. `--write` keeps the row, and `--check` and `--write` fail on a row that matches no live function. A branch side whose count is 2^62 or more (an underflowed counter, from an update lost between threads) is not covered.

| subsystem | entered of live functions | functions | lines | branches | never entered | dead |
|---|---|---|---|---|---|---|
| **wind_meteors** | 15 of 16 | 93.8% | 96.4% | 76.3% | 1 | 0 |
| **economy** | 27 of 29 | 93.1% | 78.1% | 43.8% | 2 | 4 |
| **features** | 42 of 44 | 95.5% | 81.9% | 69.6% | 2 | 4 |
| **ai** | 93 of 104 | 89.4% | 89.0% | 83.0% | 11 | 35 |
| **pathfinding** | 45 of 48 | 93.8% | 92.4% | 87.1% | 3 | 10 |
| **weapons** | 74 of 88 | 84.1% | 84.6% | 70.4% | 14 | 17 |
| **targeting** | 10 of 10 | 100.0% | 96.2% | 73.7% | 0 | 1 |
| **los_radar** | 64 of 72 | 88.9% | 82.8% | 72.9% | 8 | 22 |
| **orders** | 305 of 367 | 83.1% | 79.4% | 61.6% | 62 | 26 |
| **units** | 131 of 163 | 80.4% | 84.5% | 71.1% | 32 | 33 |
| **players** | 74 of 143 | 51.7% | 52.8% | 44.2% | 69 | 11 |
| **map** | 79 of 98 | 80.6% | 86.8% | 75.9% | 19 | 12 |
| files | 112 of 142 | 78.9% | 79.6% | 65.0% | 30 | 35 |
| game | 113 of 183 | 61.7% | 50.4% | 31.4% | 70 | 24 |
| network | 50 of 287 | 17.4% | 10.7% | 4.5% | 237 | 73 |
| platform | 318 of 639 | 49.8% | 34.9% | 22.8% | 321 | 83 |
| presentation | 454 of 1107 | 41.0% | 31.0% | 21.6% | 653 | 294 |
| seams | 94 of 103 | 91.3% | 95.8% | - | 9 | 0 |
| state | 293 of 345 | 84.9% | 80.7% | 50.8% | 52 | 21 |

Functions are assigned to a subsystem by `data/subsystems.csv` first, then by the names of special members (`X::X`, `X::~X`, `X::operator<code>`). A function still without a row is `state` when its file is under `interfaces/` or `src/state/`, else it gets the subsystem that the rules of `tools/coupling.py` give its name and file. Functions in `src/harness/`, `build/` and `toolchain/` are left out.

The runtime is clang's profile runtime (`lib/profile` of compiler-rt), built from source for i686 against the MSVC 5 C runtime in `CMakeLists.txt`. It works under Wine, so the fallback of clang's trace-pc-guard instrumentation was not needed. The game ends with `TerminateProcess`, and the runtime's own initialiser is not linked, so `host_exit` writes the profile itself (`__llvm_profile_initialize_file`, then `__llvm_profile_write_file`). `LLVM_PROFILE_FILE` takes a `Z:\` path.

## Notes from the data

- **`order_targets.cpp` is in orders.** It holds the order goal objects (`PathGoal`, `PointMarker`, `RingApproach` and the rest: most of the file is in orders), so the file rule puts it in `orders`, while #16 lists it for targeting. Targeting is then small, and most of it is from `unit_targets.cpp`. The pathfinding section shows the same line from the other side: the goals are what pathfinding calls into.
- **`movement_class.cpp` is in units and map.** Its functions are split between `units`, `map` (the pass maps) and `features`, while #14 lists the file for pathfinding.
- **`unit_targets.cpp` is split four ways** by the name rules: units and targeting hold most of it, orders and weapons the rest.
- **Phases cut across subsystems.** The phase roots are in wind_meteors, economy, features, weapons, units, players and game, and none is in ai, pathfinding, los_radar or orders. `UpdatePlayers` runs economy, ai, pathfinding and los_radar work, and `UpdateAllUnits` runs orders and weapons work. A module's entry point is often called from another subsystem's phase, not from the loop.
- **`g_game` is left out of the read lists.** Every subsystem reads it as the root pointer; the fields behind it are the items listed.

Limits of the data:

- `Game::` fields are keyed by name, not by offset. Each source file declares its own partial view of `struct Game`, so two names for one offset are two items, and two subsystems can share bytes the tables show as separate. A clang layout run would fix this.
- Only strong writes count as writes. An address taken or a pointer passed to a non-const parameter counts as a read, so a setter that writes through a pointer (targeting's) shows as a reader, and the callee's own rows show the write.
- Shared value types (`Vec3`, `Point`, `Point16`, `Fixed`) and unrelated structs with one name (`Grid`, `Pos`) give every subsystem that touches them a row. The value types are left out of the named counts and lists but still counted in the totals; the unrelated structs are not left out.
- Calls through a function pointer or a virtual method appear only where `data/hazard_edges.csv` resolves them by hand.
- Destructors are missing from the call data until #133, so the cleanup calls of a subsystem are undercounted.
- A function in a mixed folder (`src/game`, `src/map`) is assigned by file, then by phase walk, then by name. The overrides in `data/subsystems.csv` fix the known misses; a wrong assignment shows as a surprising coupling above.
