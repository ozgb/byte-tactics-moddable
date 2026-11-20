# Subsystems

This file is the draft interface of each subsystem, from the coupling map of #128 (part of #6). For the nine subsystems that have a module issue (#9 and #11 to #18) it gives the entry points, the imports (calls out to other subsystems) and the sim read and write sets, so each issue starts from a draft rather than from the source. The issue's own text wins where the two differ, and each section ends with open questions where the data shows a coupling the issue did not expect.

Nothing here is typed by hand except the prose. Every list and count comes from the tool:

```sh
uv run tools/coupling.py                      # regenerate data/subsystems.csv (keeping override) and data/coupling.csv
uv run tools/coupling.py --check              # fail if either file differs from what the tool regenerates (CI runs this)
uv run tools/coupling.py --summary [NAME ...] # print entry points, imports and sim items per subsystem
```

To browse the same data with the source beside it (the subsystem map, a flow chart and call graph for each function, and who reads and writes each global and field), run `uv run tools/codemap.py`: it serves an interactive view on 127.0.0.1 and opens it in a browser.

The inputs are `data/rwsets.csv`, `data/hazard_edges.csv` and `data/state_classes.csv` (see `docs/state.md`); the tool's docstring has the rules that assign a function to a subsystem. The 5739 functions of `data/rwsets.csv` are in 22 subsystems, and the sim items are the rows of `data/state_classes.csv` of class `sim` or `mixed`, plus the struct fields classified the same way. A call or a shared item counts functions of the calling subsystem, not call sites.

The subsystems and the issue each maps to:

| subsystem | functions | issue |
|---|---|---|
| [wind_meteors](#wind_meteors) | 15 | #9 |
| [economy](#economy) | 52 | #11 |
| [features](#features) | 57 | #12 |
| [ai](#ai) | 132 | #13 |
| [pathfinding](#pathfinding) | 148 | #14 |
| [weapons](#weapons) | 113 | #15 |
| [targeting](#targeting) | 14 | #16 |
| [los_radar](#los_radar) | 112 | #17 |
| [orders](#orders) | 243 | #18 |
| [units](#the-remaining-subsystems) | 159 | none yet |
| [players](#the-remaining-subsystems) | 147 | none yet |
| [map](#the-remaining-subsystems) | 83 | none yet |
| [mission](#the-remaining-subsystems) | 48 | none yet |
| [scripts](#the-remaining-subsystems) | 88 | none yet |
| [movement](#the-remaining-subsystems) | 144 | none yet |
| [network](#the-remaining-subsystems) | 342 | none yet |
| [game](#the-remaining-subsystems) | 339 | none yet |
| [harness](#the-remaining-subsystems) | 875 | none yet |
| [presentation](#the-remaining-subsystems) | 1430 | none yet |
| [platform](#the-remaining-subsystems) | 823 | none yet |
| [seams](#the-remaining-subsystems) | 206 | none yet |
| [files](#the-remaining-subsystems) | 169 | none yet |

The eleven subsystems without a module issue are described briefly [below](#the-remaining-subsystems). #10 (per-module replay tests) uses all of them.

How to read a section. **Entry points** are the phase roots the subsystem holds and the functions other subsystems call, with the callers. The main loop and the frame handlers (`MainLoopTick`, `RunGameSteps`, `UpdateFramePacing`, `BattleFrame`, `EndGameFrame`, `MainFrameTick`) are in `game` by `override`, although most are in presentation folders (`src/ingame/`), so a phase root has callers in `game`, not in `presentation`. **Imports** are the functions it calls in other subsystems, by target. **Sim writes** and **sim reads** list items with their class from `data/state_classes.csv`. Lists are cut to the twelve most used named items. "Named" leaves out the shared value types (`Vec3`, `Point`, `Point16`, `Fixed`) and the per-file copies of a struct that the source declares several times (a name ending in an address, such as `Proj_0049c740`).


## wind_meteors

Issue #9. 15 functions. `UpdateWind`, `UpdateMeteors`, `Apply` (the inbox of the meteor set-up and console events), the meteor parameters and their save and load (`src/map/meteors.cpp`, plus `UpdateWindOnStream` in `src/game/game_state.cpp`).

Coverage: 0.0% of functions (0 of 15), 0.0% of lines, 0.0% of branches; 15 live function no scenario enters, and 0 more are dead (data/coverage.csv).

**Live functions no scenario enters** (15 of 15 after `combat-deaths`, `meteors-earthquake` and `meteors-hailstorm` (#419) and `save-midgame` (#422), and `load-midgame` (#442), and the six `console-` scenarios (#445); 0 are entered, 0.0%), by reason:

- Native modules switch only (14): `AddPoints`, `DisableMeteors`, `EnableMeteors`, `InitMeteors`, `LoadMeteorDefaults`, `LoadMeteors`, `MakePoint`, `SaveMeteors`, `SetMeteorParams`, `StartMeteorShower`, `StartShower`, `UpdateMeteors`, `UpdateWindOnStream`, `WindMeteorsApply`. Wind and meteors run in `wind_meteors.wasm` by default (#564), so their native code runs only with `-modules=native` (`reference.py --check --native-modules`), which the coverage run does not pass. Every scenario enters the module's copies. The console's `meteor` commands are events since #970: `CmdMeteor` is game code that publishes them, and `console-features` (#445) enters it and, through the module's `Apply`, the module's `StartMeteorShower` (`meteor` and `meteor 1`).
- Shutdown only (1): `EmptyShutdownPreCleanup`.

**Entry points.** Phase roots: `UpdateWindOnStream` (the `UpdateWind` slot) and `UpdateMeteors`, both called through the scheduler (`SchedWindPhase`, `SchedMeteorsPhase`), which delivers the inbox through `Apply` first. `UpdateWindOnStream` is also called from `LoadBattleAssets` (game).

8 functions are called from other subsystems. The most called, with their callers:

- `EmptyShutdownPreCleanup` (1 from game)
- `InitMeteors` (1 from game)
- `LoadMeteorDefaults` (1 from mission)
- `LoadMeteors` (1 from game)
- `SaveMeteors` (1 from game)
- `UpdateMeteors` (1 from game)
- `UpdateWindOnStream` (2 from game)
- `WindMeteorsApply` (1 from seams)
- By calling subsystem (functions called): game 6, mission 1, seams 1

**Imports.** 17 functions of 5 subsystems, by target, each with the number of functions it calls there:

- platform (10): `BuildDataPath`, `FatalError`, `RandomInt`, `TdfFile::LoadFile`, and 6 more
- files (3): `HapiBank::OpenAccount`, `HapiBank::GetIntegerItem`, `HapiBank::SetIntegerItem`
- seams (2): `FindWeaponByNameNative`, `WindMeteorsEmit`
- game (1): `GameRand`
- weapons (1): `FindWeaponByName`

**Sim writes.** 33 items in all, 28 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`g_meteorActive` (sim); `g_meteorNextHitTime` (sim); `g_meteorNextStrikeTime` (sim); `g_meteorOrigin` (sim); `g_meteorStrikeEndTime` (sim); `g_meteorTarget` (sim); `g_meteorsEnabled` (sim); `Game::windCounter` (sim); `Game::windDirection` (sim); `Game::windEnabled` (sim); `Game::windSpeed` (sim); `Game::windStrength` (sim)

**Sim reads.** 13 items only read, 13 named. The most used (`g_game`, the root pointer, left out):

`Game::ticks` (mixed); `Game::height` (sim); `Game::width` (sim); `Seams::weapons` (sim); `g_meteorWeaponName` (sim); `g_emptyText` (sim); `Game::weapons` (sim); `Game::windSpeedDenominator` (sim); `Game::windSpeedMax` (sim)

**Draft interface.**

- Exports: 8 functions (the two phase roots, `InitMeteors`, `LoadMeteors` and `SaveMeteors` for game, `WindMeteorsApply` for the scheduler, which delivers the map's set-up and the console's `meteor` commands as events, and `LoadMeteorDefaults` for map).
- Needs from seams: 17 functions in platform, files, weapons, game, seams.
- Owns: 22 named sim items that only this subsystem writes, for example `g_meteorActive`, `g_meteorNextStrikeTime`, `g_meteorNextHitTime`, `g_meteorOrigin`, `g_meteorStrikeEndTime`.
- Shares: 6 named sim item that other subsystems also write, for example `Game::windCounter`.
- Pilot fit (#9): Most of its imports are platform (TDF parsing, `RandomInt`, `FatalError`, the `HapiBank` account store). The only sim seam is weapons (`FindWeaponByName`); a meteor that falls reaches weapons as a SPAWN effect through the scheduler (`WindMeteorsEmit`).
- The wind output is in `Game` fields (`windSpeed`, `windDirection`, `windFraction`, `windVelX`, `windZ`) that only this module writes but units, economy, features and presentation read. The meteor schedule is in `g_meteor*` statics, which must move into the state block (#7).

**Open questions.**

- A meteor reaches `SpawnProjectile` through the scheduler's SPAWN effect, not a call; the weapons subscriber of that effect is the one place where wind_meteors' output enters another sim subsystem.
- `RandomInt` is counted as a platform import but draws from a game-logic stream (#3). Should the random streams be an engine service in the seam list instead?

## economy

Issue #11. 52 functions. Economy is on interface 5.0 behind the scheduler (#937, #1009): the per-unit resource accounts are private to the module, in a region the engine allocates at the size `RegionSize` asks for and hands to `Init`, and every write another subsystem makes to economy's state is an event (an `EconomyEventHeader` and its kind's payload) that the scheduler (`src/game/scheduler.cpp`) applies through `Apply`. Economy reaches the rest of the game through a world view (`src/seams/economy_world.cpp`) and hands its effects (cloak, extractor speed, sharing) back through `Emit`. The module holds the income step `EconomyUpdatePlayer`, metal extraction, resource sharing and the share console commands (`src/game/economy*.cpp`, `share_commands.cpp`).

Coverage: 94.4% of functions (51 of 54), 91.4% of lines, 73.9% of branches; 3 live functions no scenario enters, and 3 more are dead (data/coverage.csv).

**Live functions no scenario enters** (3 of 54 after `conditions-dark-side` and `conditions-shore-to-shore` (#420) and `save-midgame` (#422), and `load-midgame` (#442), and the six `console-` scenarios (#445), and `share-metal`, `share-energy` and `share-all` (#953); 51 are entered, 94.4%), by reason:

- Shutdown only (1): `EconomyShutdown` (the seam's `Shutdown`, called from `ShutdownIngameSystems`). Test mode ends with `host_exit(0)` right after the last tick, so no scenario runs the shutdown.
- Test only (1): `EconomyWriteState`. The seam's `WriteState` writes economy's report (record 34, `economyState`), which the harness dumps but does not hash, and only at the tick `-testdump` names; the coverage run does not pass it.
- No caller in the game (1): `RequestMetalOn`. `Apply` grants a REQUEST for metal alone through it, but the game requests only energy, or energy and metal together.
- Entered by the console: `CmdNoEnergy`, `CmdNoMetal`, `CmdSetShareEnergy`, `CmdSetShareMetal`, `CmdShareAll`, `CmdShareEnergy`, `CmdShareMetal` and `UnitResources::SpendMetal` (through `TransferMetal`, console `Give`). `console-economy` (#445) types them; the share commands do nothing without a network game (`netGameFlags & 1`), so they change no hashed field.

**Entry points.** Phase roots: `EconomyUpdateResourceSharing`, called from `RunGameSteps` only when `showStats` is set and the packet manager is in use, and it returns at once unless `netGameFlags & 1` (a network game). A test-mode run with a remote slot (`player<N>.remote`, the `share-` scenarios of #953) takes both branches too. The per-tick income step `EconomyUpdatePlayer` is not a root: the scheduler's players phase (`SchedPlayersPhase`) calls it, so most of the economy runs inside the players phase.

19 functions are called from other subsystems. The most called, with their callers:

- `EconomyRequestGranted` (2 from units, 1 from orders, 1 from seams)
- `EconomyUnitAccount` (1 from presentation, 2 from units)
- `EconomyRegionSize` (1 from game)
- `CmdNoEnergy` (1 from platform)
- `CmdNoMetal` (1 from platform)
- `CmdSetShareEnergy` (1 from platform)
- `CmdSetShareMetal` (1 from platform)
- By calling subsystem (functions called): platform 7, game 5, seams 4, units 2, ai 1, harness 1, orders 1, presentation 1

**Imports.** 23 functions of 4 subsystems, by target, each with the number of functions it calls there:

- seams (15): `WorldAllied`, `WorldCellMetal`, `WorldDifficulty`, `WorldEmit`, `WorldFirstUnit`, `WorldLastUnit`, `WorldNextUnit`, `WorldPlayerInfo`, `WorldSharingEnabled`, `WorldTick`, `WorldTide`, `WorldTypeInfo`, `WorldUnitInfo`, `WorldUnitSlot`, `WorldWind` (the world view)
- game (5): `IsPlaying`, `LosRadarPublishShareMapping`, `LosRadarPublishShareRadar`, `RecordMessage`, `SimBlockTrack`
- network (2): `BroadcastPlayerInfo`, `SendShareMapInfo`
- platform (1): `CommandArgs::GetIntArg`

**Sim writes.** 41 items in all, 41 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Player::energy` (mixed); `Player::metal` (mixed); `Player::energyCapacity` (sim); `Player::energyIncome` (sim); `Player::energyUsage` (sim); `Player::energyWasted` (sim); `Player::metalCapacity` (sim); `Player::metalIncome` (sim); `Player::metalUsage` (sim)

**Sim reads.** 75 items only read, 75 named. The most used (`g_game`, the root pointer, left out):

`Player::active` (mixed); `Player::type` (mixed); `Game::players` (sim); `Game::localPlayer` (mixed); `Game::netGameFlags` (mixed); `Player::index` (mixed); `Player::info` (mixed)

**Draft interface.**

- Exports: 19 functions (`UpdatePlayerEconomy`, `UpdateResourceSharing`, the spend and request calls (`UnitResources::SpendEnergy`, `SpendMetal`, `RequestEnergy`, `RequestEnergyAndMetal`), `GetEnergyUse` (ai) and `UpdateMetalExtraction` (units)).
- Needs from seams: 23 functions in seams, game, network, platform.
- Owns: 17 named sim items that only this subsystem writes, for example `UnitResources::energyUse`, `UnitResources::player`, `UnitResources::metalUse`, `PlayerInfo::shareMetal`, `UnitResources::energyUsePaid`.
- Shares: 24 named sim items that other subsystems also write, for example `Player::energy`, `Player::metal`, `Player::energyCapacity`, `Player::metalCapacity`, `Player::shareEnergy`.
- `Player::energy` and `Player::metal` are written by four subsystems (economy, game, players, presentation; class `mixed`), so the economy module cannot own them alone. The statistics (`energyWasted`, `totalMetalProduced` and the rest) are also written by network and players.
- Economy sends packets (`SendPacketToPlayer`, `BroadcastPlayerInfo`, `AddMessage`) and starts COB scripts (`CobScript::StartScriptWithArgs`); both are candidates for events instead of imports.

**Open questions.**

- Which subsystem owns the per-player account fields? A state-block struct for the accounts that economy writes and players and network only read would match the data better than the `Player` struct.
- Resolved by #965: the sharing of vision is los_radar's. `ShareAll` publishes SHARE_MAPPING and SHARE_RADAR to los_radar's inbox, and economy imports nothing from los_radar (`docs/los-radar-interface.md`).

## features

Issue #12. 57 functions. `UpdateFeatures` and feature loading, spreading, burning, reclaim and wreckage (`src/map/features*.cpp`).

Coverage: 98.2% of functions (54 of 55), 87.3% of lines, 73.7% of branches; 1 live functions no scenario enters, and 4 more are dead (data/coverage.csv).

**Live functions no scenario enters** (1 of 55 after `combat-deaths`, `meteors-earthquake` and `meteors-hailstorm` (#419) and `save-midgame` (#422), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), and the six `console-` scenarios (#445); 54 are entered, 98.2%), by reason:

- Shutdown only (1): `FreeFeaturePool` (called from the line-of-sight teardown).
- Entered by the console: `CmdBurnAll`, `CmdBurnOne`, `CmdFeature` and `RemoveAllFeatures`, by `console-features` (#445).

**Entry points.** Phase roots: `UpdateFeatures`, called from `RunGameSteps`.

28 functions are called from other subsystems. The most called, with their callers:

- `GetCellFeature` (1 from game, 1 from map, 1 from orders)
- `DamageFeature` (1 from network)
- `CmdBurnAll` (1 from platform)
- `CmdBurnOne` (1 from platform)
- `CmdFeature` (1 from platform)
- By calling subsystem (functions called): game 13, los_radar 4, network 3, platform 3, orders 2, seams 2, map 1, players 1, units 1, weapons 1

**Imports.** 53 functions of 10 subsystems, by target, each with the number of functions it calls there:

- platform (15): `TdfRecord::GetFieldString`, `RandomInt`, `TdfFile::~TdfFile`, `operator delete`, and 11 more
- game (12): `EconHandleOf`, `EconPublish`, `FeaturesHeaderOf`, `GameRand`, `RecordSoundAtName`, `RecordTimedSubParticles`, `RecordWhiteSmoke`, `SchedFeaturesPhase`, `SchedPublish`, `SimAllocTagged`, `SimRealloc`, `StepGafSequence`
- files (9): `HapiBank::GetBoxSize`, `HapiBank::OpenAccount`, `HapiBank::OpenNamedBox`, `HapiBank::SeekBox`, and 5 more
- seams (5): `GetGameTypeNative`, `GetCellMeanHeightNative`, `FindWeaponByNameNative`, `CountMissionFeaturesNative`, `GetMissionFeatureNative`
- network (4): `NetSendFeatureHit`, `NetSendFeatureKilled`, `NetSendFeatureBurning`, `NetSendFeatureReclaimed`
- presentation (4): `FindGafEntry`, `GetGafSequenceFrame`, `InitGafSequence`, `LoadGaf`
- harness (1): `HarnessFeaturesEvent`
- los_radar (1): `GetMapCell`
- map (1): `GetGroundHeight`
- scripts (1): `ObjectStateSize`

**Sim writes.** 95 items in all, 89 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Cell::spot` (mixed); `Cell::flags` (mixed); `Spot::damage` (mixed); `Spot::feature` (mixed); `Spot::used` (mixed); `Cell::feature` (mixed); `Coord::value` (sim); `Feature::object` (mixed)

**Sim reads.** 77 items only read, 77 named. The most used (`g_game`, the root pointer, left out):

`LosRadarSeam::GetMapCell` (sim); `Seams::los_radar` (sim); `Game::width` (sim); `Game::cells` (mixed); `g_emptyText` (sim); `Feature::name` (sim); `Game::height` (sim); `Game::spots` (sim); `Seams::map` (sim); `Feature::flags` (sim); `Feature::footprint` (sim); `Spot::anim` (sim)

**Draft interface.**

- Exports: 28 functions (`UpdateFeatures`, the cell accessor `GetCellFeature`, `PlaceFeature` (los_radar, units), `DamageFeature` (weapons, network), `ReclaimFeature` (orders), and the load, save, pool and file-list calls from game and los_radar).
- Needs from seams: 53 functions in platform, game, files, network, presentation, seams, harness, los_radar, map, scripts.
- Owns: 85 named sim items that only this subsystem writes, for example `Game::features`, `Game::featureCount`, `Pool::entries`, `Cell::spot`, `Spot::feature`.
- Shares: 4 named sim items that other subsystems also write, for example `Cell::feature`, `Cell::flags`.
- It imports from presentation (`EmitWhiteSmoke`, `PlaySoundAtByName`, the GAF loaders). Sim code that draws or plays sound has to raise events instead (#22).
- Features and weapons call each other: features calls `ApplyAreaDamageAt`, weapons calls `DamageFeature`. `Cell::feature` is also written by los_radar (`ClearFeature` is in `line_of_sight.cpp`, so it is los_radar by file).

**Open questions.**

- `ClearFeature` is a feature operation in a LOS file. Should it move to features, which would remove the second writer of `Cell::feature`?
- `Cell::spot`, `Cell::offsetX` and `Cell::offsetY` are written here and read by los_radar, map, orders and weapons. Are they feature state or part of the cell record that map owns?

## ai

Issue #13. 132 functions. The computer player: `UpdatePlayerAI`, build ratings, weight scripts and squads (`src/ai/ai_player*.cpp`, `player_ai.cpp`, `ai_profile.cpp`, most of `src/units/squads.cpp`).

Coverage: 79.8% of functions (87 of 109), 75.6% of lines, 72.9% of branches; 22 live functions no scenario enters, and 31 more are dead (data/coverage.csv).

**Live functions no scenario enters** (22 of 109 after #420 and `console-sight` (#445); 87 are entered, 79.8%), by reason:

- Shutdown only (4): `DestroyPlayerAI`, `FreeSquads`, `SquadManager::DeleteTimers` and `SquadTimer::~SquadTimer`. All four are called from `FreePlayers`, which `ShutdownIngameSystems` calls and no scenario runs.
- Console command only (1): `DumpPlayerAI` (console `PrintWeights`). It writes the file its second argument names, so the `console` key leaves it out (#445). `console-sight` enters `ResetAIPlayers` (console `ReloadAIProfiles`).
- Network only (2): `ProtectUnitDefsReadOnly` and `ProtectUnitDefsReadWrite`. Only `UnitSync::ApplyToUnitTypes` calls them.
- Test only (11): `AiWriteState`, `??$AiWriteRawVector@E@@YGXPBUStateWriter@@PBD1AAV?$BlockVector@E@@I@Z`, `??$AiWriteRawVector@F@@YGXPBUStateWriter@@PBD1AAV?$BlockVector@F@@I@Z`, `??$AiWriteRawVector@UBuildRatingWeights@@@@YGXPBUStateWriter@@PBD1AAV?$BlockVector@UBuildRatingWeights@@@@I@Z`, `??$AiWriteRawVector@UKeyedCell@@@@YGXPBUStateWriter@@PBD1AAV?$BlockVector@UKeyedCell@@@@I@Z`, `??$AiWriteRawVector@UPercentByte@@@@YGXPBUStateWriter@@PBD1AAV?$BlockVector@UPercentByte@@@@I@Z`, `??$AiWriteRawVector@UUnitTypeInt@@@@YGXPBUStateWriter@@PBD1AAV?$BlockVector@UUnitTypeInt@@@@I@Z`, `AiPlayerSlotOf`, `AiWritePlayerAI`, `AiWriteTimer` and `WriteAiRecords`. The seam's `WriteState` and its helpers write ai's report (record 43, `aiState`), which the harness dumps but does not hash, and only at the tick `-testdump` names; the coverage run does not pass it.
- Needs a case no scenario sets up yet (4): `SumUnitRatingsInRange` (the AI scout probe rating, which `SpatialTimer::OnTimer` calls only when its probe point is visible), `IsExplored`, `IsVisible` and `Offset` (statics in `ai_player.cpp` that only `SpatialTimer::OnTimer` calls). These are not inlined copies of a function that is entered elsewhere: the other `IsExplored`, `IsVisible` and `Offset` statics in the tree take other argument types and are other functions, and coverage counts an inlined body at its source. `OnTimer` returns before them when the spatial group (group 9) holds no unit, so a scenario needs an AI whose script gives that group units.

**Entry points.** Phase roots: none. `UpdatePlayerAI` and `UpdateSquads` (3.1c's `SquadManager::TickIfActive`) are run by the scheduler's players phase (`SchedPlayersPhase`), in each player's turn, after ai's inbox is delivered (#968, `docs/ai-interface.md`).

23 functions are called from other subsystems. The most called, with their callers:

- `AiApply` (1 from seams)
- `CopyDwordPair` (1 from platform)
- `CopyOneByte` (1 from platform)
- `UpdatePlayerAI` (1 from game)
- By calling subsystem (functions called): players 7, game 5, seams 4, network 2, platform 2, presentation 2, harness 1

**Imports.** 35 functions of 8 subsystems, by target, each with the number of functions it calls there:

- platform (17): `RandomInt`, `operator new`, `CommandArgs::InitArgs`, `ExecuteCommandText`, and 13 more
- seams (6): `ScheduleTargetingTargetUnit`, `ScheduleTargetingFind`, `GetGameTypeNative`, `GetNameSlotNative`, `ScheduleMapCanBuildAt`, `ScheduleMapCanPlaceUnitFootprint`
- game (4): `AiEventSize`, `Game::SlotOf`, `RecordBuildQueueSound`, `SimAllocTagged`
- los_radar (4): `GetMapCell`, `GetSeenUnitsInRadius`, `IsPointVisible`, `IsUnitVisibleToPlayer`
- economy (1): `GetEnergyUse`
- orders (1): `GetOrderFlags`
- 2 smaller targets: players (1), weapons (1)

**Sim writes.** 31 items in all, 26 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`SquadTimer::next` (sim); `PlayerAI::lastTick` (sim); `SquadManager::countdown` (sim); `Player::groups` (mixed); `SquadManager::cursor` (sim); `SquadManager::nextAction` (mixed); `g_aiCommandsEnabled` (sim); `PlayerAI::builders` (sim); `PlayerAI::centre` (sim); `Squad::index` (sim); `Squad::field_8` (sim); `Squad::field_c` (sim)

**Sim reads.** 128 items only read, 127 named. The most used (`g_game`, the root pointer, left out):

`Game::defs` (mixed); `Game::ticks` (mixed); `Game::unitDefCount` (mixed); `Unit::def` (mixed); `UnitDef::flags1` (mixed); `MissionParams::surfaceMetal` (sim); `Player::index` (mixed); `Player::type` (mixed); `Seams::mission` (sim); `Unit::player` (sim); `Player::active` (mixed); `Player::unitsBegin` (mixed)

**Draft interface.**

- Exports: 23 functions (`UpdatePlayerAI`, `SquadManager::TickIfActive`, the create, reset and destroy calls from game, `Apply` for the events of weapons, units and the engine (`docs/ai-interface.md`); since #991 it exports no query another subsystem calls: the unit index and its queries are los_radar's and the factory list units').
- Needs from seams: 35 functions in platform, seams, los_radar, game, economy, orders, players, weapons.
- Owns: 21 named sim items that only this subsystem writes, for example `g_playerAI`, `SquadManager::timers`, `SquadTimer::next`, `SquadManager::player`, `g_aiCommandsEnabled`.
- Shares: 5 named sim items that other subsystems also write, for example `Unit::group`.
- It acts through other modules: effects to orders (ADD_ORDER, QUEUE_BUILD, ATTACK, STOP, HIT) and units (NOTIFY_REFS, ACTIVATE, SET_STATES), and intents to targeting. Its own sim writes are few.
- Its read set is wide (items of `Unit`, `UnitDef`, `Player` and `Game`). A read-only view of the state block fits it better than a getter per field.

**Open questions.**

- The AI calls the pathfinder's open-list heap (`MakeHeap`, `PopHeapFirst`). Is that heap a pathfinding export or shared library code?
- `operator-` is used by orders. Does it belong in a shared maths seam instead of ai? (`GetFactoriesInRadius` moved to units in #991.)

## pathfinding

Issue #14. 148 functions. The search in `src/ai/pathfind*.cpp`: per-player cost tables, the node budget, the open heap and the grid.

Coverage: 88.5% of functions (69 of 78), 82.3% of lines, 69.4% of branches; 9 live functions no scenario enters, and 10 more are dead (data/coverage.csv).

The HPA* demo of `src/ai/pathfind_hpa.cpp` (#930) is counted apart, with the reason `demo: hpa` in `data/coverage.csv` (see Coverage): its 60 live functions are in none of these counts. The idle ai demo of `src/ai/ai_idle.cpp` (#968) is counted apart in the same way, with the reason `demo: idle ai`.

**Live functions no scenario enters** (9 of 78 after #421; 69 are entered, 88.5%), by reason:

- Unreachable (2): `Pathfinder::RemoveNode` and `OpenHeap::SiftDownInline`. `RunSearches` is the only caller of `RemoveNode`, and it zeroes `topPopped` just before the `if (!topPopped)` that guards the call, so the call is dead code in 3.1c. `SiftDownInline` is called only by `RemoveNode`.
- Shutdown only (1): `PathfindingShutdown` (the seam's `Shutdown`, called from `ShutdownIngameSystems`). Test mode ends with `host_exit(0)` right after the last tick, so no scenario runs the shutdown.
- Test only (5): `PathfindingWriteState`, `WriteNodes`, `WriteCells`, `OwnNodeIndex` and `NodeIsLive`. The seam's `WriteState` and its helpers write the pathfinder's report (sections 16 and 17), which the harness dumps but does not hash, and only at the tick `-testdump` names: `tools/tracediff.py` passes it at a divergence, and the coverage run does not. A run of `conditions-shore-to-shore` with `-testdump 60` enters all five.
- Console command, display, sound, debugger or host only (1): `PathfindingDebugCell` (the seam's `DebugCell`). Only the debug overlay of the console's debug mode draws it.

**Entry points.** Phase roots: none. `PathfindingStep` and `PathfindingPoll` are called by `UpdatePlayers` (players).

18 functions are called from other subsystems. The most called, with their callers:

- `PathfindingOnMapCellsChanged` (2 from game)
- `AdjustHeap` (2 from platform)
- `PathfindingCancel` (2 from movement)
- `PathfindingDebugCell` (1 from presentation)
- By calling subsystem (functions called): game 11, movement 4, harness 1, platform 1, presentation 1

**Imports.** 21 functions of 2 subsystems, by target, each with the number of functions it calls there:

- seams (17): `WorldBeginSearch`, `WorldEndSearch`, `WorldFirstUnit`, `WorldInBounds`, and 13 more
- platform (4): `PathGoalCells`, `PathGoalContains`, `PathGoalDistance`, `operator new`

**Sim writes.** 254 items in all, 252 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Node::index` (sim); `PathCell::flags` (sim); `OpenHeap::count` (sim); `OpenHeap::freeHead` (sim); `OpenHeap::topPopped` (sim); `Pathfinder::object` (mixed); `Pathfinder::owner` (mixed); `OpenHeap::used` (sim); `PathCell::dir` (sim); `Pathfinder::steps` (sim); `NodeData::f` (sim); `NodeData::g` (sim)

**Sim reads.** 50 items only read, 50 named. The most used (`g_game`, the root pointer, left out):

`Pathfinder::grid` (sim); `PathWorldView::Pass` (sim); `PathWorldView::InBounds` (sim); `PathWorldView::Seen` (sim); `PathWorldView::PlayerCount` (sim); `PathWorldView::UnitState` (sim); `Game::height` (sim); `Game::width` (sim); `NodeData::pos` (sim); `PathUnitState::footprintX` (sim); `PathRequest::goal` (sim); `PathWorldView::Tick` (sim)

**Draft interface.**

- Exports: 18 functions (`PathfindingStep` and `PathfindingPoll` (players), `PathfindingSubmit` and `PathfindingCancel` (orders), `PathfindingOnMapCellsChanged`, the route of map's CELLS_CHANGED (game's scheduler)).
- Needs from seams: 21 functions in seams, platform.
- Owns: 250 named sim items that only this subsystem writes, for example `OpenHeap::items`, `OpenHeap::pool`, `Node::index`, `OpenHeap::count`, `Grid::dirty`.
- Shares: 2 named sim items that other subsystems also write, for example `Grid::cells`, `Grid::width`, `Grid::count`, `Grid::height`.
- Few functions are called from outside, so the entry side is small. The import side is the world view (`PathWorldView`, filled in `src/seams/pathfinding_world.cpp`) and the goal geometry (`PathGoalContains`, `PathGoalDistance` and `PathGoalCells`, which orders and the search share). The search calls nothing in orders: a request carries its goal as data and the results come back as events that orders applies.
- The budget (`Pathfinder::budget` and the per-player counters in the region) is state this module writes alone; the trace hashes the events it delivers (section 32) and only dumps this state, through `PathfindingWriteState`.

**Open questions.**

- `Grid::cells`, `width` and `height` show as written by pathfinding, los_radar and map. Either `Grid` is one name for unrelated structs (type-wide evidence) or there is a real shared grid; a clang layout run would tell.

## weapons

Issue #15. 113 functions. `UpdateProjectiles`, `UpdateExplosions`, `UpdateUnitWeapons`, ballistics, damage and the weapon definitions (`src/weapons/*.cpp`).

Coverage: 87.3% of functions (96 of 110), 88.8% of lines, 74.0% of branches; 14 live functions no scenario enters, and 19 more are dead (data/coverage.csv).

**weapons, functions no scenario enters** (14 of 110 after the scenarios of `reference/scenarios/`; 96 are entered, 87.3%), by reason:

- Shutdown only (4): `DebrisCache::FreeBuffer`, `FreeExplosions`, `FreeWeaponArray` and `FreeWeaponTypes`. `ShutdownIngameSystems` calls them, and test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Network only (5): `ApplyProjectileHitPacket`, `ApplyWeaponFirePacket`, `FindRemoteProjectileForPacket`, `SamePos` and `SameTargetPos`. `ApplyProjectileHitPacket` and `ApplyWeaponFirePacket` are the handlers of the projectile packets of `net_game.cpp`, and the other three are statics only they call. Test mode runs every player on one machine.
- Test only (2): `WeaponsWriteState` and `CountExplosionTables`. The seam's `WriteState` writes weapons' report (record 48, `weaponsState`), with the table counts `CountExplosionTables` gives, which the harness dumps but does not hash, and only at the tick `-testdump` names; the coverage run does not pass it.
- Needs a case no scenario sets up yet (2): `??8@YG_NABVStringRef@@0@Z` and `Ne` (the `StringRef` comparison and its caller in the weapon loader, reached only when the damage table of a weapon already holds a key that sorts at or after the new one).
- Slot no code calls (1): `AimCobStub` (slot 1 of `g_weaponAimCobVtable`). It returns 0; not explained further.

**Entry points.** Phase roots: `UpdateProjectiles` and `UpdateExplosions`, which the scheduler calls (`SchedProjectilesPhase` and `SchedExplosionsPhase`, game). `UpdateUnitWeapons` is not a root: the scheduler's units phase (`SchedUnitsPhase`, game) calls it between `UpdateUnitHead` and `UpdateUnitBody` (units).

20 functions are called from other subsystems. The most called, with their callers:

- `WeaponCanReachUnit` (6 from orders, 1 from targeting)
- `FindWeaponByName` (1 from seams, 1 from wind_meteors)
- `Release` (6 from platform)
- `AllocWeaponArray` (1 from game)
- By calling subsystem (functions called): game 9, seams 3, network 2, presentation 2, ai 1, harness 1, orders 1, platform 1, targeting 1, wind_meteors 1

**Imports.** 72 functions of 11 subsystems, by target, each with the number of functions it calls there:

- platform (27): `RandomInt`, `GameFreeThunk`, `operator new`, `operator-`, and 23 more
- game (20): `EconHandleOf`, `EconSpend`, `FeaturesPublishDamage`, `GameRand`, `RecordBuildMenuUpdate`, `RecordCdActivity`, `RecordScreenShake`, `RecordSmoke`, `RecordSoundAt`, `RecordWeaponSmoke`, `RecordWhiteSmoke`, `SimAllocTagged`, `StepGafSequence`, `WeaponsAimScript`, `WeaponsEventSize`, `WeaponsFireScript`, `WeaponsPublishDamageRecord`, `WeaponsPublishHealth`, `WeaponsPublishMarkDamaged`, `WeaponsPublishParalyze`
- presentation (7): `AllocFrame`, `BuildLensFrame`, `FindGafEntry`, `GetGafFrame`, `InitGafSequence`, `Load3do`, `MirrorObject`
- network (5): `NetSendWeaponFire`, `NetSendUnitDamage`, `NetSendProjectileDetonated`, `NetSendWeaponFireAt`, `SendScriptCallByName`
- seams (5): `GetCellMeanHeightNative`, `GetGroundHeightNative`, `QueryScriptNative`, `ScheduleTargetingTargetPos`, and 1 more
- los_radar (2): `GetMapCellAtPosition`, `GetMapCell`
- scripts (2): `GetPieceOffset`, `GetPiecePosition`
- 4 smaller targets: features (1), files (1), map (1), units (1)

**Sim writes.** 173 items in all, 169 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Projectile::flags` (mixed); `Projectile::targetUnit` (mixed); `Game::projectileCount` (mixed); `Angles16::heading` (mixed); `Projectile::speed` (sim); `Angles16::pitch` (mixed); `Game::tracked` (mixed); `Game::trackedPos` (mixed)

**Sim reads.** 171 items only read, 170 named. The most used (`g_game`, the root pointer, left out):

`Unit::def` (mixed); `Unit::heading` (sim); `Game::ticks` (mixed); `Unit::player` (sim); `Game::seaLevel` (mixed); `Seams::scripts` (sim); `ScriptsSeam::QueryScript` (sim); `Game::gravity` (sim); `Unit::playerIndex` (sim); `MissionParams::noSeaLevelTrigger` (sim); `Unit::id` (sim)

**Draft interface.**

- Exports: 20 functions (the two roots, `UpdateUnitWeapons`, `WeaponsApply` and `WeaponsWriteState`, `WeaponCanReachUnit`, `FindWeaponByName`, `FindTargetableProjectile`, `GetWeaponRange` and the set-up and net packet handlers `ApplyWeaponFirePacket` and `ApplyProjectileHitPacket`).
- Needs from seams: 72 functions in platform, game, presentation, network, seams, los_radar, scripts, features, files, map, units.
- Owns: 149 named sim items that only this subsystem writes, for example `Game::projectiles`, `WeaponDef::index`, `Game::projectileCount`, `WeaponDef::speed`, `WeaponFirePacket::aimOrVel`.
- Shares: 20 named sim items that other subsystems also write, for example `Game::cameraSnapTicks`, `Unit::flags`, `g_explodedPieces`, `Game::count`, `Game::cameraFollowTrackObj`.
- A long import list: presentation (smoke, sound, screen shake, GAF and model loading) and units (COB scripts, piece positions) are among its targets.
- Most of its writes are not shared state: many `WeaponDef::*` fields written when the TDFs load, packet buffers (`WeaponFirePacket`, `UnitDamagePacketOut`) and per-file struct copies. The shared state is `Game::projectiles`, `Game::projectileCount`, `Unit::health`, `Unit::flags` and the weapon slot fields.
- It also writes view state: `Game::cameraSnapPos` and `cameraSnapTicks` (from `DetonateProjectile` and `CheckProjectileCollision`).

**Open questions.**

- `DamageUnit` writes `Unit::health`, which game, orders, players and units also write. Is damage a weapons export or a units operation that weapons requests? #15 already asks which machine applies damage in multiplayer.
- `Unit::ClaimWeapons` is called for orders only, through a seam. It is a lifecycle call, not a per-tick one; does it belong to a unit lifecycle seam?

## targeting

Issue #16. 14 functions. Target choice and the weapon target setters and getters (`FindWeaponTarget`, `FindBestTargetIfFireAtWill`, `SetWeaponTargetUnit`). It is small because the file rules put the order goal objects in orders; see the notes.

Coverage: 100.0% of functions (20 of 20), 96.1% of lines, 79.6% of branches; 0 live functions no scenario enters, and 1 more is dead (data/coverage.csv).

**Entry points.** Phase roots: none. Order handlers (orders) and the AI call it.

6 functions are called from other subsystems. The most called, with their callers:

- `ApplyTargetingEvent` (1 from seams)
- `GetWeaponTargetUnit` (1 from seams)
- `FindBestTargetIfFireAtWill` (1 from seams)
- `FindWeaponTarget` (1 from seams)
- `GetWeaponTargetPos` (1 from seams)
- `Unit::ChooseWeapon` (1 from seams)
- By calling subsystem (functions called): seams 6

**Imports.** 10 functions of 7 subsystems, by target, each with the number of functions it calls there:

- platform (3): `RandomInt`, `UnitSwapEraseVector::EraseSwapBack`, `UnitTypeSet::Contains`
- seams (2): `GetSweetSpotNative`, `GetWeaponRangeNative`
- game (1): `SchedPublish`
- harness (1): `HarnessTargetingEvent`
- los_radar (1): `GetVisibleEnemiesInRadius`
- map (1): `GetGroundHeight`
- weapons (1): `WeaponCanReachUnit`

**Sim writes.** 12 items in all, 9 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`UnitWeaponSlot::aimTargetXOrUnitId` (sim); `UnitWeaponSlot::aimTargetZOrUnitSentinel` (sim)

**Sim reads.** 38 items only read, 38 named. The most used (`g_game`, the root pointer, left out):

`Unit::weapons` (sim); `Game::units` (mixed); `Seams::map` (sim); `Unit::pos` (mixed); `Unit::unitDefIndex` (mixed); `UnitWeaponSlot::flags` (mixed); `UnitDef::range` (mixed); `MapSeam::GetGroundHeight` (sim); `LosRadarSeam::GetVisibleEnemiesInRadius` (sim); `Game::matchFlags` (sim); `Game::seaLevel` (mixed); `Player::active` (mixed)

**Draft interface.**

- Exports: 6 functions (all of them are target setters, getters and searches; `ClearWeaponTarget` and `GetWeaponTargetUnit` are the busiest).
- Needs from seams: 10 functions in platform, seams, game, harness, los_radar, map, weapons.
- Owns: 7 named sim items that only this subsystem writes, for example `UnitWeaponSlot::aimTargetXOrUnitId`, `UnitWeaponSlot::aimTargetZOrUnitSentinel`.
- Shares: 2 named sim items that other subsystems also write.
- It writes the target position of the weapon slot (`UnitWeaponSlot`) and no other named sim item; its other written items are `Vec3` and `Point` value types. The setters probably write the rest of the target through a pointer, which counts as a read.
- It reaches units through seams (COB script calls, `GetGroundHeightNative` and `GetSweetSpotNative`) and imports the range check from weapons (`WeaponCanReachUnit`) and `RandomInt` and `Bits::Test` from platform.

**Open questions.**

- With no state of its own, is targeting a set of functions over the weapon slot, and so part of weapons, as #16 allows?
- `unit_targets.cpp` is one file in 3.1c, split here by name between units, targeting, orders and weapons. The split is the rule's guess and needs a decision in #16.

## los_radar

Issue #17. 112 functions. Line of sight, radar and sonar, the LOS tables, the map cell accessor and the TNT loader (`src/map/line_of_sight*.cpp`, `los_tables*.cpp`, `radar.cpp`, and two `src/game/players_46*.cpp` files).

Coverage: 90.6% of functions (87 of 96), 87.6% of lines, 74.4% of branches; 9 live functions no scenario enters, and 22 more are dead (data/coverage.csv).

**Live functions no scenario enters** (9 of 96 after `sensors-los-off`, `los-off-mapped-ai` and `hailstorm-los-off` (#421), `sensors-sonar-jam` and `orders-tier2-core` (#426) and `sensors-floating-teeth` and `sensors-floating-teeth-los-off` (#437), and the six `console-` scenarios (#445); 87 are entered, 90.6%), by reason:

- Share dialog or network only (1): `ShareMapInfo` (the map-share button of the ally share dialog, or a share packet).
- Shutdown only (4): `FreeMapResources`, `FreeUnitIndex` and the static it calls, `ReleaseUnits`, and `LosRadarShutdown`. `FreeMapResources` and `LosRadarShutdown` are called from `ShutdownIngameSystems` and `FreeUnitIndex` from `FreePlayers`, which no scenario runs because test mode ends with `host_exit(0)` right after the last tick.
- Needs a case no scenario sets up yet (1): `GetSeenUnitsInRadius`. Its only caller is ai's `SumUnitRatingsInRange`, which no scenario enters (see ai).
- Slot no code calls (1): `GetVisibleEnemiesInRadiusList`, the list form of the slot added for the oracle kit (#823); no 3.1c code calls it.
- Network only (2): `CmdShareMapping` and `CmdShareRadar` (the share commands, which los_radar runs from its share events since #978 and which check `netGameFlags & 1`, a network game). Test mode runs a skirmish.
- Entered by the console: `CmdLOS`, `CmdLOSType` and `CmdRadar`, by `console-sight` (#445).

**Entry points.** Phase roots: none. `UpdatePlayers` (players) calls `RecalculateLineOfSight`, `UpdateUnitLineOfSight` and `UpdateSensorRadarAndCloak`, so LOS runs inside the players phase.

31 functions are called from other subsystems. The most called, with their callers:

- `GetMapCell` (14 from features, 1 from ai, 1 from map, 1 from network, 1 from presentation, 1 from seams, 1 from units, 1 from weapons)
- `GetMapCellAtPosition` (3 from weapons, 2 from orders, 1 from game, 1 from map, 1 from presentation)
- `IsUnitVisibleToPlayer` (4 from presentation, 1 from ai, 1 from scripts)
- `RecalculateLineOfSight` (1 from presentation)
- `UpdateUnitLineOfSight` (1 from game)
- By calling subsystem (functions called): game 13, presentation 7, seams 5, ai 4, players 4, orders 3, map 2, units 2, weapons 2, features 1, network 1, platform 1, scripts 1, targeting 1

**Imports.** 40 functions of 7 subsystems, by target, each with the number of functions it calls there:

- platform (16): `GameFreeThunk`, `StringRef::Assign`, `operator delete`, `GameAllocIgnoreTag`, and 12 more
- game (10): `FeaturesPublishPlace`, `Game::Current`, `Game::SlotOf`, `LosRadarEventSize`, `RecordMessage`, `RecordSimEvent`, `SimAlloc`, `SimAllocTagged`, `SimBlockTrack`, `SimCaptureAlloc`
- features (4): `FreeFeaturePool`, `InitFeatureAnimPool`, `PlaceFeature`, `PlaceMissionFeatures`
- map (3): `GetGroundHeight`, `MapAllocateBuckets`, `MapFreeBuckets`
- presentation (3): `BuildTedRadarFrame`, `GetGafFrame`, `StringRef::ReleaseRef`
- seams (3): `GetNameSlotNative`, `GetMissionParamsNative`, `VisitUnitsInRange`
- network (1): `BroadcastPlayerInfo`

**Sim writes.** 119 items in all, 114 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Game::count_14277` (mixed); `SightQuery::cacheCell` (mixed); `SightQuery::eyeHeight` (mixed); `SightQuery::frameIdx` (mixed); `SightQuery::player` (mixed); `SightQuery::pos` (mixed); `SightQuery::sightDistance` (mixed); `Unit::flags` (mixed); `Eye::expires` (mixed); `Eye::flagA` (mixed); `Eye::flagPtr` (mixed); `Eye::player` (mixed)

**Sim reads.** 127 items only read, 127 named. The most used (`g_game`, the root pointer, left out):

`Game::width` (sim); `Game::height` (sim); `Game::playerIndex` (mixed); `Game::seaLevel` (mixed); `Game::losTable` (sim); `Player::exploredWidth` (sim); `Unit::def` (mixed); `Player::explored` (sim); `FrameTable::count` (sim); `Unit::player` (sim); `Game::grid1` (sim)

**Draft interface.**

- Exports: 31 functions (the three players-phase calls, `GetMapCell` and `GetMapCellAtPosition`, `IsUnitVisibleToPlayer`, `RevealNewUnit`, `ShareMapping` and `ShareRadar` (economy), and the load, free and console calls).
- Needs from seams: 40 functions in platform, game, features, presentation, seams, map, network.
- Owns: 104 named sim items that only this subsystem writes, for example `SightQuery::sightDistance`, `Game::heightMap`, `SightQuery::cacheCell`, `SightQuery::frameIdx`, `Game::visibilityMask`.
- Shares: 10 named sim items that other subsystems also write, for example `Grid::height`, `Grid::width`, `GafFrame::xOffset`, `GafFrame::yOffset`, `Grid::cells`.
- Its presentation imports are the GAF frame reader and the TED radar frame builder (`BuildTedRadarFrame`); the radar picture and minimap drawing (`DrawRadarUnits`, `UpdateRadarMapped`, `ResizeRadarPicture`, `FreeRadar`) is presentation (src/ingame/radar.cpp). The line of sight code and the players phase record a snapshot of what the minimap shows through the `RecordRadarUnits` and `RecordRadarMapped` slots (src/map/radar_capture.cpp), and presentation draws from the snapshot at the drain (#647). This is the boundary #17 asks to keep clear.
- The result is `Game::visibilityMask`, which ai, economy, map, orders, pathfinding and presentation read. `Player::explored` is read here and written elsewhere.

**Open questions.**

- `GetMapCell` has callers in many subsystems. It is the map cell accessor, not a LOS function. Should map own it, with los_radar importing it?
- `Game::visibilityMask` is read by pathfinding and ai as well as presentation. Which part is sim state and which is the player view (#22)?

## orders

Issue #18. 243 functions. The order state machines, VTOL orders and order lists (`src/orders/*.cpp`). The unit motion (`UnitMotion`) and the goal objects are movement's since #993: orders makes, steers and frees them through `g_seams.movement`.

Coverage: 96.6% of functions (227 of 235), 85.9% of lines, 65.3% of branches; 8 live functions no scenario enters, and 17 more are dead (data/coverage.csv).

**Live functions no scenario enters** (8 of 235 after `orders-immediate`, `orders-factory` and `orders-air` (#418), the three scenarios of #419, the three of #421, the ten of #426 and `save-midgame` (#422), `load-midgame` (#442), and `units-transport` and `units-transport-ship` (#428), and `mission-defeat`, `mission-victory` and `mission-radius` (#444); 227 are entered, 96.6%), by reason:

- Save and load only (1): `OrderKindFromSavedIndex`. `save-midgame` enters the save side of `Order`, and `load-midgame` (#442) enters `LowerBoundOrderTypes`, `OrderTypeNameLess`, `OrderKindFromName`, `ResolveUnitTypeKey`, the load constructor of `Order` and `Order::ReattachFxToUnit`. The move targets' save and load are movement's since #993 (see movement).
- Info panel, cursor, build menu and shutdown only (1): `ClearOrderTypeTable`.
- No caller in the game (6): `RepairUnitNoMoveOrder`, `ReadyOrder`, `ClearAllWeaponTargets`, `VtolGetRepairedOrder`, `Sub` and `PathOrderAttach::~PathOrderAttach`. Nothing creates `VtolGetRepairedOrder` (only its definition and table row name it). The `Sub` is the one of `unit_orders.cpp` at about line 1433 (the one at about line 1989 is `TeleportOrder`'s helper and is entered). `PathOrderAttach::~PathOrderAttach` is the virtual destructor of the unit reference that an order and a move target embed: they are freed by their own `Destroy`, which unlinks the reference through units' `UnitRefUnlink`, and since #993 the unlink is `UnitRef::Unlink`'s own body rather than an inline call of the destructor.

**Entry points.** Phase roots: none. `UpdateAllUnits` (units) calls `RunOrders`.

34 functions are called from other subsystems. The most called, with their callers:

- `CopyDwordIfNonNull` (3 from platform)
- `Order::OrStatusFlags` (1 from seams)
- `DeleteOrder` (1 from harness, 1 from presentation)
- By calling subsystem (functions called): presentation 12, game 8, seams 7, units 6, harness 3, ai 1, platform 1

**Imports.** 101 functions of 13 subsystems, by target, each with the number of functions it calls there:

- movement (21): `PathGoal::SetPathOrder`, `OrderFx::Destroy`, `OrderFx::GetType`, `SerializeSave`, and 17 more
- seams (21): `ClaimWeaponsNative`, `ReleaseWeaponsNative`, `SetStateBitsNative`, `ScheduleTargetingTargetUnit`, and 17 more
- game (13): `EconHandleOf`, `EconPublish`, `FeaturesPublishRemove`, `RecordBuildMenuUpdate`, `RecordNanoParticles`, `RecordReverseNanoParticles`, `RecordSelectionOrdersDirty`, `RecordTeleportParticles`, `RecordUnitSpeech`, `WeaponsPublishAddStockpile`, `WeaponsPublishClaim`, `WeaponsPublishDamage`, `WeaponsPublishRelease`
- platform (10): `operator new`, `RandomInt`, `GetHeadingBetween`, `operator==`, and 6 more
- units (10): `AttachUnitToPiece`, `AddBuildProgress`, `AddRepairProgress`, `CreateUnit`, and 6 more
- files (7): `HapiBank::OpenNamedBox`, `HapiBank::SeekBox`, `HapiBank::ReadBox`, `HapiBank::WriteBox`, and 3 more
- network (5): `SendScriptCall`, `SendScriptCallByName`, `Player::GetEnergy`, `Player::GetMetal`, and 1 more
- 6 smaller targets: map (4), los_radar (3), scripts (3), features (2), economy (1), weapons (1)

**Sim writes.** 58 items in all, 52 after leaving out the shared value types (Vec3, Point, Point16, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Order::flags` (mixed); `Order::flags_42` (mixed); `Order::pos` (mixed); `Order::state` (mixed); `Order::next` (mixed); `Order::subFlags` (mixed); `Order::unit` (mixed); `Unit::workTime` (mixed); `Box::hi` (mixed); `Order::attached` (mixed); `Order::angle` (mixed)

**Sim reads.** 269 items only read, 268 named. The most used (`g_game`, the root pointer, left out):

`Unit::def` (mixed); `Seams::units` (sim); `Unit::motion` (mixed); `Order::target` (sim); `Seams::weapons` (sim); `Unit::list` (sim); `UnitDef::flags1` (mixed); `WeaponsSeam::WeaponCanReachUnit` (sim); `Unit::list2` (sim); `Unit::player` (sim); `Game::ticks` (mixed); `Seams::targeting` (sim)

**Draft interface.**

- Exports: 34 functions (`RunOrders`, `RunSecondaryOrders`, `AddOrder`, `DeleteOrders`, `GetOrderType`, `IssueAttackOrder`, `OrderType::GetTableEntry` (presentation)).
- Needs from seams: 101 functions in movement, seams, game, platform, units, files, network, los_radar, map, scripts, features, economy, weapons.
- Owns: 37 named sim items that only this subsystem writes, for example `Order::state`, `Order::pos`, `Order::next`, `Order::attached`, `Order::unit`.
- Shares: 15 named sim items that other subsystems also write, for example `Order::flags`, `Unit::pos`, `Unit::flags`, `Unit::health`, `Unit::buildLeft`.
- The units imports are the unit lifecycle and geometry: `CreateUnit`, `SetUnitPosition` and `Unit::SetStateBits`; the COB script calls and piece positions are imports from scripts. The weapons imports are mostly the claim and release of weapons, which orders reaches through seams.
- It is the heaviest writer of core unit fields: `Unit::pos` and `Unit::flags`. The fields are also written by units and presentation (`pos`), and by many other subsystems (`flags`).

**Open questions.**

- Which side owns `Unit::pos`, `flags`, `health` and `buildLeft` in the state block (#7): units, with orders going through its seam, or orders? This decides whether #18 can split.
- The split #18 allowed followed the `UnitMotion::` functions: motion and the goals are movement, a module of its own since #993 (`docs/movement-interface.md`).

## The remaining subsystems

Each has no module issue yet. Entry points and couplings come from the same tool (`uv run tools/coupling.py --summary NAME`); the numbers count functions.

**units** (159 functions). Phase root `UpdateAllUnits`, which calls `RunOrders` (orders) and `UpdateUnitWeapons` (weapons). 69 functions are called from outside; the busiest are `FindUnitTypeId` (18), `AttachUnitToPiece` (10), `CreateUnit` (9). It is called most by game (43 distinct functions) and seams (8). It imports 145 functions, mostly platform (47), presentation (7), seams (22) and files (16). It holds the unit table and lifecycle code (`CreateUnit`, `Unit::SetStateBits`); the COB scripts and piece queries are scripts' since #993, and it reaches them through `g_seams.scripts`. It is the other side of most of orders' writes. Coverage: 87.2% of functions (116 of 133), 85.0% of lines, 68.6% of branches; 17 live functions no scenario enters, and 27 more are dead (data/coverage.csv). The list of those 17 is under **units, functions no scenario enters** below.

**units, functions no scenario enters** (17 of 133 after `combat-deaths`, `meteors-earthquake` and `meteors-hailstorm` (#419) and `save-midgame` (#422), `load-midgame` (#442), and `units-transport` and `units-transport-ship` (#428), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), and the six `console-` scenarios (#445); 116 are entered, 87.2%), by reason:

- Load only (1): `IdAt` (a sort helper of the load). `save-midgame` (#422) enters the save side (`SaveUnits`, `CobScript::SaveScriptState` and the `UnitScript::IsPiece` slots it calls), and `load-midgame` (#442) enters `LoadUnits`, `LoadUnit` and `CobScript::LoadScriptState`.
- Network only (4): `WriteUnitState`, `ReceiveUnitStates`, `ReadUnitState` and `CreateUnitFromPacket`.
- Network game only (1): `UnitsOnPlayersSendUnit`. It applies players' SEND_UNIT effect, which `GiveUnitToPlayer` publishes only when the unit goes to a remote human (player type 3).
- Shutdown only (9): `FreeUnitMemory`, `FreeUnitTypes`, `FreeUnitInfo`, `FreeUnitCategories`, `FreeDownloadMenus`, `FreeMovementClasses`, the two statics only it calls, `FreeA` and `FreeC`, and `FreeFactoryIndex` (from `FreePlayers`).
- Entered by the console: `KillAllUnits`, `KillUnitsOfType` and `ReloadUnitType`, by `console-units` (#445). `Selectable` is entered too since the coverage of #652, by a path not yet traced.
- Unit info dialog only (2): `MakePropList` and its static `spd`.

**players** (147 functions). Phase root `UpdatePlayers`, which calls into economy (`UpdatePlayerEconomy`), ai (`UpdatePlayerAI`, squads), pathfinding (`RunSearches`) and los_radar (its LOS calls) in turn. 33 functions are called from outside, mostly by game (18) and presentation (3); the busiest are `SetupPlayerSlot` (3) and `ResetPlayerSlots` (3); its writes come in as events (`docs/players-interface.md`). It imports 63 functions: platform 13, network 5, seams 8, files 10, presentation 0. It holds the per-player state and victory conditions. Coverage: 66.0% of functions (103 of 156), 67.7% of lines, 61.0% of branches; 53 live functions no scenario enters, and 10 more are dead (data/coverage.csv). The list of those 53 follows this paragraph.

**players, functions no scenario enters** (53 of 156 after `conditions-dark-side` and `conditions-shore-to-shore` (#420), `save-midgame` (#422) and `allied-victory` (#432), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), the six `console-` scenarios (#445), and the ten `mission-` scenarios of #764; 103 are entered, 66.0%), by reason:

- Mission maps only (3): `DefeatAnyUnitPassesZ::DefeatAnyUnitPassesZ`, `DefeatAnyUnitPassesZ::IsSatisfied` and `DefeatAnyUnitPassesZ::VisitUnit`. `RegisterConditions` creates the condition classes from mission condition keys in the map file (`CommanderKilled`, `KillEnemyCommander`, `AllUnitsKilledOfType` and so on), in every game type, and `CheckVictory` and `CheckDefeat` read the lists only for game type 1. Three skirmish maps carry a condition key, all in `totala2.hpi`: Dark Side and Lava Alley carry `KillEnemyCommander`, and Shore to Shore carries `CommanderKilled`. The `conditions-dark-side` and `conditions-shore-to-shore` scenarios run them. The other keys appear on mission maps only, which have only `Easy`, `Medium` and `Hard` schemas and so load under game type 1 only (under game type 2, `Mission::SelectSchema` wants a `Network N` schema). The ten `mission-` scenarios of #764 and `mission-defeat`, `mission-victory` and `mission-radius` (#444) play missions of `tactics1.hpi`, `tactics2.hpi`, `tactics4.hpi`, `tactics5.hpi` and `tactics7.hpi` and enter every condition class but `DefeatAnyUnitPassesZ`: `mission-kill-of-type` the victory conditions `KillAllOfType` and `KillUnitType`, `mission-kill-mobile` `KillAllMobileUnits`, `mission-build-unit` `BuildUnitType`, `mission-capture` `CaptureUnitType`, `mission-passes-x` and `mission-passes-z` `UnitTypePassesX` and `UnitTypePassesZ`, `mission-timer` `VictoryTimerRunsOut`, and the defeat conditions `mission-defeat-all-units` `AllUnitsKilled`, `mission-defeat-of-type` `AllUnitsKilledOfType` and `mission-defeat-passes-x` `AnyUnitPassesX`. `AnyUnitPassesZ` is set by one mission, `CC19` (The Vebreen Fleet, `totala4.hpi`), and no scenario can load it: `RegisterDataArchives` adds at most ten `*.HPI` archives, so with the install's thirteen, `tactics1.hpi` to `tactics8.hpi`, `totala1.hpi` and `totala2.hpi` are added and `totala3.hpi`, `totala4.hpi` and `worlds.hpi` never are, and the campaign maps of `totala4.hpi` are not found. `SetStartingStorageBonus` and its static `AtLeast200` are entered from `InitPlayerResources` under game type 1 (the other call site is in `UpdatePlayers` under `commanderDeath == 2`, which the harness cannot set). Their `VisitUnit` slots are reached from their own `OnUnitDied` or `IsSatisfied`.
- Save and load only (36): `DefeatAllUnitsKilled::LoadState`, `DefeatAllUnitsKilled::SaveState`, `DefeatAllUnitsKilledOfType::LoadState`, `DefeatAllUnitsKilledOfType::SaveState`, `DefeatAnyUnitPassesX::LoadState`, `DefeatAnyUnitPassesX::SaveState`, `DefeatAnyUnitPassesZ::LoadState`, `DefeatAnyUnitPassesZ::SaveState`, `DefeatCommanderKilled::LoadState`, `DefeatCommanderKilled::SaveState`, `DefeatDeathTimerRunsOut::LoadState`, `DefeatDeathTimerRunsOut::SaveState`, `DefeatUnitTypeKilled::LoadState`, `DefeatUnitTypeKilled::SaveState`, `VictoryBuildUnitType::LoadState`, `VictoryBuildUnitType::SaveState`, `VictoryCaptureUnitType::LoadState`, `VictoryCaptureUnitType::SaveState`, `VictoryDestroyAllUnits::LoadState`, `VictoryDestroyAllUnits::SaveState`, `VictoryKillAllMobileUnits::LoadState`, `VictoryKillAllMobileUnits::SaveState`, `VictoryKillAllOfType::LoadState`, `VictoryKillAllOfType::SaveState`, `VictoryKillEnemyCommander::LoadState`, `VictoryKillEnemyCommander::SaveState`, `VictoryKillUnitType::LoadState`, `VictoryKillUnitType::SaveState`, `VictoryMoveUnitToRadius::LoadState`, `VictoryMoveUnitToRadius::SaveState`, `VictoryTimerRunsOut::LoadState`, `VictoryTimerRunsOut::SaveState`, `VictoryUnitTypePassesX::LoadState`, `VictoryUnitTypePassesX::SaveState`, `VictoryUnitTypePassesZ::LoadState`, `VictoryUnitTypePassesZ::SaveState`. `save-midgame` (#422) enters `MissionConditions::SaveConditions` and `SavePlayers`, but the save side of a condition runs only for a condition the map has, and The Pass has none, so the `SaveState` slots and their `LoadState` slots stay listed. `load-midgame` (#442) enters `LoadPlayerControllers`, `LoadPlayers` and `MissionConditions::LoadConditions`.
- Network only (6): `CountActiveAIPlayers`, `FillScoreTables` and `IsReporterDllLoaded` (the score tables of the online score reporter of the lobby and of `UnitSync`, and the AI count it reads), `PlayerRecordAt` (a static of `DrawSyncStatus`, the load-progress strip drawn only for game type 3 while waiting for other machines), and `RecordWatchModeNotice` and `RecordWatchPrompt` (the watch-mode notice and prompt of a defeated player).
- Menu or dialog click only (1): `SaveNumSkirmishPlayers` (called from the skirmish menu, which test mode skips).
- Shutdown only (2): `FreePlayers` and `PlayersShutdown` (both called from `ShutdownIngameSystems`, which no scenario runs).
- Test only (3): `PlayersWriteState`, `WriteCondition` and `HarnessConditionClass`. The seam's `WriteState` and its helpers write players' report (record 46, `playersState`), which the harness dumps but does not hash, and only at the tick `-testdump` names; the coverage run does not pass it.
- Network game only (2): `??$PlayersFillHeader@UPlayersSendUnit@@UUnit@@@@YGXPAUPlayersSendUnit@@HPAUUnit@@@Z` and `??$PlayersPublishSendUnit@UUnit@@@@YGXPAUUnit@@H@Z`. `GiveUnitToPlayer` publishes SEND_UNIT (whose header `PlayersFillHeader` fills) only when the unit goes to a remote human (player type 3).
- Entered by the console: `TransferEnergy` and `TransferMetal` (console `Give`, `console-economy`, through a TRANSFER event) and `MissionConditions::Deactivate` (console `Kill`, `console-units`) (#445).

**map** (83 functions). No phase root. 27 functions are called from outside, by seams (11), game (10), orders (4), presentation (4) and los_radar (3); the busiest are `GetGroundHeight` (22), `SnapWorldPosToFootprint` (4), `IsPadSlotFree` (2) and `CanBuildAt` (2). It imports 16 functions: files 5, platform 4, game 3, los_radar 2, features 1, harness 1. It holds the terrain and the ground height, the pass maps and when they are current, and the footprints (`docs/map-interface.md`). Coverage: 94.6% of functions (70 of 74), 93.9% of lines, 82.5% of branches; 4 live functions no scenario enters, and 9 more are dead (data/coverage.csv).

**mission** (48 functions). No phase root. The mission, the campaign and the list of maps (`src/mission`, `interfaces/mission.h`, #983). 34 functions are called from outside, by seams (32), game (2), harness (1), network (1) and presentation (1); the busiest are `SetMissionType` (12) and the 25 other slots' functions, each through its filler. It imports 42 functions: platform 25, files 6, seams 4, game 3, presentation 3, wind_meteors 1. Its calls into players (the conditions of a mission) and wind_meteors (`LoadMeteorDefaults`) go through their seams. The inventory is `docs/mission-interface.md`. Coverage: 59.6% of functions (28 of 47), 74.9% of lines, 62.7% of branches; 19 live functions no scenario enters, and 1 more are dead (data/coverage.csv). The list of those 19 is under **mission, functions no scenario enters** below.

**map, functions no scenario enters** (4 of 74 after `sensors-los-off`, `los-off-mapped-ai` and `hailstorm-los-off` (#421) and `save-midgame` (#422), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444); 70 are entered, 94.6%), by reason:

- Load only (1): `ForceNeighborFootprintReclaim` (called from `LoadUnit`). `save-midgame` (#422) enters the save side, and `load-midgame` (#442) enters `LoadMetalPlotmap`, `LoadPlayerFeaturesPlotmap` and `LoadMappingData` (the saved game files).
- Shutdown only (1): `MapFreeBuckets`. It is called from `ShutdownIngameSystems` through los_radar's `FreeMapResources`, which no scenario runs because test mode ends with `host_exit(0)` right after the last tick.
- Test only (1): `MapWriteState`. The seam's `WriteState` writes map's report (record 45, `mapState`), which the harness dumps but does not hash, and only at the tick `-testdump` names; the coverage run does not pass it.
- No caller in the game (1): `GetTerrainPassValue`. Its only caller is the pathfinding world's `TerrainPass` (`src/seams/pathfinding_world.cpp`, #930), which only the HPA* demo calls; 3.1c's pathfinder reads the pass maps instead.

**mission, functions no scenario enters** (19 of 47 after the scenarios of `reference/scenarios/`; 28 are entered, 59.6%), by reason:

- Shutdown only (1): `FreeOtaEnumCacheAndMission`. It is called from `ShutdownIngameSystems`, which no scenario runs because test mode ends with `host_exit(0)` right after the last tick.
- Network lobby and map check only (6): `Mission::ComputeMapChecksum`, `MapCacheEntry::MapCacheEntry`, `MapCacheEntry::MapCacheEntry` (the two constructors), `MapCacheEntry::operator?4`, `MapCacheEntry::SetChecksum` (all four only from `ComputeMapChecksum`) and `Mission::GetTerrainLength` (the map check of a joining player).
- Multiplayer lobby and menus only (7): `Mission::GetTerrainSizeTier` (the lobby colour), `Mission::GetDescription`, `Mission::GetPlanet`, `Mission::GetTranslatedName` and `Mission::HasMissionName` (the map list and mission menus), and `Mission::GetMemory` and `Mission::GetNumPlayers` (the lobby's map size line). Test mode calls only `LoadMissionByName`.
- Campaign menu, end screen and restart only (5): `Mission::GetBriefing`, `Mission::SelectMission`, `Mission::MissionExists`, `Mission::AdvanceMission` and `Mission::GetMissionIndex`.

**scripts** (88 functions). No phase root: units' `UpdateUnit` runs each unit's script through the `RunScripts` slot. The COB interpreter, the unit scripts and the piece state (`src/scripts`, `interfaces/scripts.h`, #993). 24 functions are called from outside, by seams (15), orders (3), movement (2), weapons (2), units (2), features (1) and presentation (1); the busiest are `GetNanoPiecePosition` (15) and `GetPiecePosition` (4). It imports 21 functions: game 7, platform 6, files 4, seams 1, los_radar 1, presentation 1. Its calls back into weapons (`WeaponAimCobComplete`, `CobScriptExplodePiece`), units (`AttachUnitToPiece`, `SetStateBits`) and map (`GetGroundHeight`) go through their seams. The inventory is `docs/scripts-interface.md`. Coverage: 76.5% of functions (62 of 81), 89.7% of lines, 84.7% of branches; 19 live functions no scenario enters, and 6 more are dead (data/coverage.csv). The list of those 19 is under **scripts, functions no scenario enters** below.

**scripts, functions no scenario enters** (19 of 81 after the scenarios of `reference/scenarios/`; 62 are entered, 76.5%), by reason:

- Base-class slots of `CobScript` that `UnitScript` overrides (13): `CobScript::AttachUnit`, `CobScript::DropUnit`, `CobScript::EmitSfx`, `CobScript::ExplodeLegacy`, `CobScript::ExplodePiece`, `CobScript::GetTransporterId`, `CobScript::GetUnitValue`, `CobScript::IsCarryingUnit`, `CobScript::IsPieceCached`, `CobScript::IsPieceShaded`, `CobScript::IsPieceVisible`, `CobScript::PlaySoundNoop`, `CobScript::SetUnitValue`. Every unit's script is a `UnitScript`, so the base bodies are never called.
- COB opcodes that no script of the game data uses (4): `UnitScript::IsCarryingUnit`, `UnitScript::GetTransporterId`, `UnitScript::PlaySoundNoop`, `Channel::XorOp`. No unit can reach them.
- Needs a case no scenario sets up yet (2): `UnitScript::ExplodeLegacy` (the empty body of COB opcode 0x10009000, which no scenario's script runs) and `CreateObjectState` (the piece state of a unit whose type has no script; since #954 a feature's model takes `ObjectStateSize` and the INIT_STATE effect instead, and every unit type of the game data has a script).

**movement** (144 functions). No phase root: units' `UpdateUnit` calls `UpdateMotion` through movement's seam. `UnitMotion`, the goal objects (`PathGoal`, `LiteGoal`, `PatrolGoal`, `PackedGoal`, `PackedPosGoal`, `AiSearchGoal`) and the move targets of an order (`PathOrder`, `AirManeuverOrder`, `ApproachRadius`, `RingApproach`, `PointMarker`) (`src/movement`, `interfaces/movement.h`, #993). 38 functions are called from outside, by orders (21), seams (10), network (6) and game (1); the busiest are `PathGoal::SetPathOrder` (6) and `AirManeuverOrder::Destroy` (5). It imports 33 functions: platform 12, seams 5, files 4, pathfinding 4, network 3, scripts 2, units 2. The inventory is `docs/movement-interface.md`. Coverage: 63.5% of functions (94 of 148), 69.3% of lines, 63.6% of branches; 54 live functions no scenario enters, and 8 more are dead (data/coverage.csv). The list of those 54 is under **movement, functions no scenario enters** below.

**movement, functions no scenario enters** (54 of 148 after the scenarios of `reference/scenarios/`; 94 are entered, 63.5%), by reason:

- Network only (23): `LiteGoal::HasNetUnitState`, `LiteGoal::SerializeNetUnitState`, `AiSearchGoal::HasNetUnitState`, `AiSearchGoal::SerializeNetUnitState`, `PackedPosGoal::PackedPosGoal`, `PackedPosGoal::~PackedPosGoal`, `PackedPosGoal::DeserializeNetUnitState`, `PathGoal::DeserializeNetUnitState`, `PathGoal::HasNetUnitState`, `PathGoal::SerializeNetUnitState`, `PathGoal::ExportGoalPose`, `PathGoal::FillWaypointWorldPos`, `PathGoal::HasReadyWaypoints`, `PathGoal::TickTowardGoal`, `PathGoal::TryClaimRepath`, `PatrolGoal::PatrolGoal`, `PatrolGoal::DeserializeNetUnitState`, `PatrolGoal::FillWaypointWorldPos`, `PatrolGoal::HasReadyWaypoints`, `OrderFx::WriteBits`, `AirManeuverOrder::SerializeToBits`, `PathOrder::SerializeToBits` and `UnitMotion::ReadNetState`. These are the net-state methods of the goal objects and the bit writers. `PatrolGoal` and `PackedPosGoal` are made only for a player of type 3 (a remote human), and the `PathGoal` methods only run through them.
- Save and load only (15): `SerializeSave`, `OrderFx::OrderFx`, `OrderFx::GetType`, `AirManeuverOrder::AirManeuverOrder`, `AirManeuverOrder::AirManeuverOrder`, `AirManeuverOrder::GetType`, `AirManeuverOrder::SerializeToSave`, `ApproachRadius::ApproachRadius`, `ApproachRadius::Serialize`, `PathOrder::PathOrder`, `PathOrder::PathOrder`, `PathOrder::GetType`, `PathOrder::SerializeToSave`, `RingApproach::RingApproach` and `RingApproach::Serialize`. The constructors are the load constructors. `SubmitRepath` (#930) enters `ApproachRadius::GetType` and `RingApproach::GetType`. `save-midgame` enters the save side of `PointMarker` and `UnitMotion`, and `load-midgame` (#442) enters `UnitMotion::LoadMotion` and the load constructor of `PointMarker`.
- Slot no code calls (8): `ApproachRadius::AppendGoalCell`, `ApproachRadius::ApproxDistExcess`, `RingApproach::AppendGoalCell`, `RingApproach::ApproxDistExcess`, `PointMarker::AppendGoalCell`, `PointMarker::ApproxDist`, `AiSearchGoal::TryClaimRepath` and `AppendAreaCells`. Since the pathfinding interface (#930) the pathfinder reads a goal's area (`GetGoalArea`) and keeps its own queue of repath requests, so no code calls these vtable slots of the goal objects any more; `AppendAreaCells` is called only by the three `AppendGoalCell`.
- No caller in the game (8): `TryGetDesiredHeading`, `OrderFx::ApproxDist`, `OrderFx::ContainsCell`, `OrderFx::Destroy`, `OrderFx::FillGoalCells`, `OrderFx::IsFxStyle`, `AirManeuverOrder::IsFxStyle` and `PathOrder::IsFxStyle`. The last seven are base-class slots of `OrderFx`, also of `AirManeuverOrder` and `PathOrder`.

**network** (342 functions). Phase root `HandleNetPackets`, which dispatches into every subsystem, so what it reaches says nothing about the owner. 97 functions are called from outside, mostly by presentation (54) and game (31); the busiest are `BroadcastPacket` (5), `BroadcastPlayerInfo` (19), `AddMessage` (8). It imports 118 functions: platform 51, presentation 24, players 5, units 7. Its packet handlers write many sim items (284 in all).

**network, functions no scenario enters** (243 of 297 after `save-midgame` (#422) and `allied-victory` (#432), and the six `console-` scenarios (#445), and `share-metal`, `share-energy` and `share-all` (#953); 54 are entered, 18.2%), by reason:

- Network only (153): `??$copy_backward@PAHPAH@std@@YGPAHPAH00@Z`, `??$copy_backward@PAVUnitSyncPlayer@@PAV1@@std@@YGPAVUnitSyncPlayer@@PAV1@00@Z`, `??$fill@PAHH@std@@YGXPAH0ABH@Z`, `??$fill@PAVUnitSyncPlayer@@V1@@std@@YGXPAVUnitSyncPlayer@@0ABV1@@Z`, `AddNetPlayer`, `AssignPlayerColor`, `AssignStartPositions`, `BitReader::ReadBits`, `BitReader::ReadSignedBits`, `BitWriter::BitWriter`, `BitWriter::FreeBuffer`, `BitWriter::GrowBuffer`, `BitWriter::SetByteAt`, `BitWriter::WriteBits`, `BroadcastAllyTeam`, `BroadcastPendingViewState`, `CheckDuplicatePlayerIds`, `CheckPlayerTimeouts`, `CloseNetSession`, `CommandAllowed`, `CopyWords`, `CountMessage`, `CountPacket`, `CreateNetGame`, `DefaultPacketHandler`, `DequeueByCalls`, `DropPlayer`, `EnumPlayersCallback`, `FindFirstActiveLocalIdInRejectPlayer`, `FindConnectedPlayerId`, `FindSlotByDpidInSetAlliance`, `FindSlotByDpidInRejectPlayer`, `FindLobbyHostDpid`, `FindLobbyHostPlayer`, `FindLobbyHostPlayerId`, `FindHostSlotInAssignStartPositions`, `FindSlotByIdInAddNetPlayer`, `FindSlotByIdInIsColorFree`, `FindSlotByIdInOpenTimeoutDialog`, `FirstJoinedId`, `FindFirstType1IdInAssignStartPositions`, `FormatNetStats`, `FrameQueue::QueueFrames`, `FrameQueue::ResetFrames`, `FrameQueue::~FrameQueue`, `FrameRing::Pop`, `FrameRing::Push`, `FreeTeam`, `GetCreatedLobbyInterface`, `GetFrame`, `GetHostDpid`, `GetLocalHumanDpid`, `GetPlayerDpid`, `GetSessionGuidInstance`, `HandlePing`, `HandlePlayerEconomy`, `HandleTimeoutDialog`, `InGame`, `InfoPacket`, `InitLobbiedConnection`, `InitNetConnection`, `InitPacketManager`, `IsActiveLocalInSetAlliance`, `IsColorFree`, `IsConnected`, `IsReusable`, `IsActiveLocalAiInSetAlliance`, `IsActiveRemoteInSetAllianceB`, `IsTimedOut`, `IsActiveLocalHumanInRemovePlayer`, `IsActiveRemoteInSetAlliance`, `IsActiveRemoteInRemovePlayer`, `IsValid`, `JoinLobbyGame`, `JoinLobbyGameEntry`, `JoinLobbyGameThread`, `JoinNetGame`, `LeaveNetGame`, `LocalPlayer`, `NetCondenser::Accumulate`, `NetCondenser::ReceivePacket`, `NetCondenser::SendPacket`, `NetCondenser::SendPacketTo`, `NetSendFeatureHit`, `NetSendFeatureReclaimed`, `NetSendOwnershipTransfer`, `NetSendUnitDamage`, `NetSendUnitStates`, `NetSendWeaponFire`, `NetSendWeaponFireAt`, `NextFrameSeqInReceiveFrame`, `OpenTimeoutDialog`, `PacketBuffer::AppendPacket`, `PacketBuffer::FreePackets`, `PacketBuffer::RemovePacket`, `PacketChannel::AddPacket`, `PacketChannel::AllocBuffer`, `PacketChannel::AllocPacket`, `PacketChannel::EnqueuePacket`, `PacketChannel::GrowPools`, `PacketChannel::InitPools`, `PacketChannel::SendQueued`, `PacketChannel::~PacketChannel`, `PacketManager::AppendToSendBuffer`, `PacketManager::FindChannel`, `PacketManager::HandleIntegrityNop`, `PacketManager::QueueOnChannel`, `PacketManager::QueuePacket`, `PacketManager::SetDefaultSendPacing`, `PacketReceiver::FindPlayerFrameInfo`, `PacketReceiver::ReceiveFrame`, `PacketReceiver::~PacketReceiver`, `PacketRing::Peek`, `PacketRing::Pop`, `PacketRing::PopPacket`, `PacketRing::Push`, `PacketRing::PushPacket`, `PacketTrace`, `PeekFrame`, `Player::FreeSideDataAndFogSightCounts`, `Player::IsPlayableSlot`, `PlayerFrameInfo::Initialize`, `PlayerFrameInfo::~PlayerFrameInfo`, `PrevFrameSeqInReceiveFrame`, `PumpNetPackets`, `QueryOnlineFlags`, `QueueCount`, `RIReport`, `RIReportGameChat`, `RISetCallbacks`, `ReceiveNetPacket`, `RejectPacketType2`, `RejectPacketType5`, `RejectPacketType6`, `RejectPacketType7`, `RejectPacketType8`, `RejectPlayer`, `RemovePlayer`, `ClearPlayerSlotInRemovePlayer`, `ReportGameChat`, `ReportPacketGap`, `SendLobbySyncRequests`, `SetAlliance`, `TakeFrame`, `UnitAt`, `UnitSync::CheckUnitAvailable`, `UnitSync::HandleSyncPacket`, `UnitSync::NotifyEntryChanged`, `UnitSync::ReceiveSyncPacket`, `UnitSync::SendSyncPacket`, `UnitSyncMap::FindExact`, `UnitSyncMap::LowerBound`, `UpdateTimeoutDialog`. Needs other machines or a network session; test mode runs every player on one machine. Covered by the replay recordings, #10 and #389.
- Network only (3): `HandleNetPackets`, `SendLoadProgress`, `SendProbe`. Test mode reached them only in the loading frames, which it has skipped since #885 (it waits for the load in one frame). `SendLoadProgress` has no other caller (`LoadingScreenFrame`). No scenario enters the others' other callers: the main loop's wait branch (`HandleNetPackets`, `SendProbe`), the lobby and the end-game screen and the in-game dialogs (`HandleNetPackets`). `PacketManager::SendAllQueued` is entered by the share pass of the `share-` scenarios (#953).
- Menu or front end only (73): `??$copy@PAVUnitSyncPlayer@@PAV1@@std@@YGPAVUnitSyncPlayer@@PAV1@00@Z`, `??$copy@PBUUnitSyncPacketElem@@PAU1@@std@@YGPAUUnitSyncPacketElem@@PBU1@0PAU1@@Z`, `AreAllPlayersReady`, `SyncRecordIntVectorBase::Capacity`, `SyncRecordIntVectorBase::count`, `SyncRecordIntVector::Size`, `SyncRecordIntVector::assign_first`, `SyncRecordIntVector::assign_second`, `SyncRecordIntVector::begin`, `SyncRecordIntVector::begin`, `SyncRecordIntVector::burn`, `SyncRecordIntVector::end`, `CopyDwordRangeOverwrite`, `CountComputerPlayers`, `CountHumanPlayers`, `CountLocalComputerPlayers`, `CreateLocalPlayer`, `CreateUnitSync`, `DeleteUnitSync`, `FindPlayerInUse`, `FindFlag97SlotInIsHostLocal`, `FindReadyPlayer`, `FindTarget`, `FinishUnitSync`, `IsUnitDefFlags2Bit16Set`, `GetRejectReasonText`, `IsHostLocal`, `IsHuman`, `IsPlaying`, `UnitSyncMapLess::operator?R`, `NopChecksumEntryDtor`, `OnlineGetLinkInfo`, `OnlineProcessButtonCommand`, `OnlineTranslate`, `PacketSequencer::Inl`, `PacketSequencer::PacketSequencer`, `PacketSequencer::SendUnsequenced`, `PacketSequencer::operator?4`, `Pass`, `RequestPlayerColor`, `SendChatMessage`, `SendChatPacket`, `SendNetHeartbeat`, `SyncChecksumVector::~SyncChecksumVector`, `SyncPlayerRecord::operator?4`, `UnitSync::AllPlayersSynced`, `UnitSync::AllowUnit`, `UnitSync::ApplyToUnitTypes`, `UnitSync::DisallowUnit`, `UnitSync::End`, `UnitSync::Find`, `UnitSync::GetSyncStatusText`, `UnitSync::GetUnitEntry`, `UnitSync::IsPlayerSynced`, `UnitSync::PopChangedEntry`, `UnitSync::ProcessSync`, `UnitSync::SetUnitLimit`, `UnitSync::UnitSync`, `UnitSync::~UnitSync`, `UnitSyncIter::UnitSyncIter`, `UnitSyncIter::operator?8`, `UnitSyncMap::Buynode`, `UnitSyncMap::End`, `UnitSyncMap::Erase`, `UnitSyncMap::Init`, `UnitSyncMap::UnitSyncMap`, `UnitSyncMap::operator?A`, `UnitSyncMap::~UnitSyncMap`, `UnitSyncPlayer::UnitSyncPlayer`, `UnitSyncPlayer::UnitSyncPlayer`, `UnitSyncPlayer::operator?4`, `UnitSyncPlayer::~UnitSyncPlayer`, `allocator::destroy`. The lobby, menus and dialogs, which test mode skips. Covered by #398's UI log.
- Console command, host only (4): `CmdCompression`, `GetByteRates`, `PacketManager::NopRet_D`, `SumMessageStats`. Network settings and statistics (`CmdCompression`, `CmdNetStats`), which change no game state, so #445's `console` key leaves them out. Covered by #398.
- Key, window or screenshot input only (2): `CycleMessageUnits`, `ScrollToNextMessageUnit`. Test mode sends no input. Covered by #398.
- Shutdown only (6): `EmptyAtexitHandler`, `LeaveNetGameCallback`, `NetCondenser::~NetCondenser`, `PacketManager::~PacketManager`, `ReleasePacketData`, `RemoveLocalPlayers`. `ShutdownIngameSystems` and exit paths; test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Command-line switch only (2): `Onl_LoadConfigFile`, `SetPacketRate`. Switches a scenario cannot pass (`-blockscan` and the network options).
- Entered by the console scenarios (#445): `SendShareEnergy`, `SendShareMetal`, `SendPacketToPlayer`, `FindSlotByDpidInSendPacketToPlayer`, `PlayerReady` and `Player::SetType`, which the console commands `Give` and `AI` reach.

**game** (339 functions). Phase root `StepAllGafSequences`. Start-up, load, save and shutdown code, and the main loop: `MainLoopTick`, `RunGameSteps` (which calls every phase root), `UpdateFramePacing`, `BattleFrame`, `EndGameFrame`, `MainFrameTick`, `RunEndGameState` and `UpdateBattleRoom` (the last two pump `HandleNetPackets`) are set to game by `override`, because what drives the step is not presentation. 206 functions are called from outside, by platform (84 distinct functions) and presentation (32); the busiest are `SetCursorMode` (19), `SimAllocTagged` (19), `AllocSite::~AllocSite` (12). It imports 584 functions, the most of any subsystem, 218 of them from presentation, then platform 103, network 31, files 23 and units 43.

**game, functions no scenario enters** (83 of 334 after `save-midgame` (#422) and `allied-victory` (#432), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), and the six `console-` scenarios (#445); 251 are entered, 75.1%), by reason:

- Network only (4): `PreBattleFrame`, `RecordMessage`, `UpdateBattleRoom`, `WeaponsPublishRemoteDamage` (the remote damage packet's event, which `HandleNetPackets` publishes). Needs other machines or a network session; test mode runs every player on one machine. Covered by the replay recordings, #10 and #389.
- Needs a case no scenario sets up yet (2): `RecordFocusCommander` (the camera move after a commander is created for a player who starts without one) and `RecordTeleportParticles` (the sparks of a teleport order).
- Menu or front end only (4): `Force640x480Surfaces`, `ReturnToMainMenuFrame`, `SaveAllMissionsSetting`, `SetGameMode`. The lobby, menus and dialogs, which test mode skips. Covered by #398's UI log.
- Save and load only (8): `DeleteHapiBank`, `ListSavedGames`, `LoadGameScreenHandler`, `SaveGameScreenHandler`, `SavedGameSelectHandler`, `ShowLoadGameScreen`, `ShowSaveGameScreen`, `ShowSavedGameInfo`. Test mode saves (the `save` key, `save-midgame`) and loads that save (the `load` key, `load-midgame`, #442), which enters `FreeSummaryBank`, `LoadSavedGameState`, `OpenHapiBank` and `OpenSummaryBank`. The screens and handlers of the save and load menus are never entered.
- Console command, game logic only (2): `CmdInclude` (it reads `debugdat\include.txt`) and `CmdPrintWeights` (it writes the file its second argument names). The `console` key leaves both out (#445): a scenario reads no file beyond the game data and writes none. The other 29 functions of this group, `ExecuteCommandLine` and `DefaultCommandHandler` among them, are entered by the six `console-` scenarios (#445).
- Console command, display, sound, debugger or host only (32): `CmdAntiAlias`, `CmdBigBrother`, `CmdCDPlay`, `CmdCDStop`, `CmdClock`, `CmdContour`, `CmdDebugBreak`, `CmdDither`, `CmdFShadow`, `CmdFilm`, `CmdFilmSpeed`, `CmdGamma`, `CmdLight`, `CmdLogo`, `CmdMakePoster`, `CmdMemDump`, `CmdMusicMode`, `CmdNetStats`, `CmdNoShake`, `CmdProfile`, `CmdRCache`, `CmdSave`, `CmdScreenChat`, `CmdScrollSpeed`, `CmdSelBoxes`, `CmdSenderror`, `CmdShading`, `CmdShadow`, `CmdSing`, `CmdSound3D`, `CmdSwitchAlt`, `CmdTShadow`. Commands that only change presentation or the host (display and sound settings, the camera, the debugger, files, network statistics). Typed console input; the `console` key (#445) leaves them out, except `CmdNow`, the cheat code `Now Film Chris Include Reload Assert` that enables the debug commands, which every console scenario types. Covered by #398.
- Console command only (4): `ConsoleShareMapping`, `ConsoleShareRadar`, `LosRadarPublishShareMapping`, `LosRadarPublishShareRadar`. The console commands `ShareMapping` and `ShareRadar` publish los_radar's share events (#978), and `CmdShareAll` publishes them only in a network game; no console scenario types either command.
- Share dialog or network only (1): `LosRadarPublishShareMap`. It publishes the map share event, from the map-share button of the ally share dialog (`src/ingame/game_dialogs.cpp`) or a share packet (`src/network/net_game.cpp`), as `ShareMapInfo` has.
- Key, window or screenshot input only (3): `InstallOutOfMemoryHandler`, `SaveCdLists`, `SetGameSpeed`. Test mode sends no input. Covered by #398.
- Shutdown only (8): `AtexitNoOp`, `DestroySimGlobals`, `FreeLogos`, `FreeSideFonts`, `ShutdownGame`, `ShutdownIngameAndQuit`, `ShutdownIngameSystems`, `WriteGameRegistryValue`. `ShutdownIngameSystems` and exit paths; test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Game over only (2): `EndGameFrame`, `RunEndGameState`. The end-game screen. A run ends at the end of the step that sets the game over flags (`commander-death` and `allied-victory` end at tick 360), before the frame code switches to `EndGameFrame`, so no scenario steps into it; the stats screen itself is #398's.
- Error paths only (6): `Fail`, `Fatal`, `Fatal`, `OutOfBlock`, `OutOfMemoryHandler`, `ReportViaException`. Fatal errors and out-of-memory handlers, which a passing run never reaches. The second `Fatal` is match setup's (`src/game/match_setup.cpp`), for a block size or search step count out of range.
- Test only (3): `QuirksFlipFirstSim`, `SimBlockForEach`, `SimBlockSize`. `QuirksFlipFirstSim` is the quirk-freeze check's deliberate failure (`quirks_tamper`), which exits 2 and so has no stored trace. `SimBlockForEach` is called only by the harness's block scan, which `reference.py --check` runs with `-blockscan` and the coverage run does not. `SimBlockSize` is called only by the harness's round trip of the block (`--roundtrip`), which the coverage run does not pass.
- Moddable-only key (1): `QuirksNormalise`. Entered by a `quirks` line, which the reference harness does not read, so a scenario with one has no stored trace.
- Native modules switch only (2): `Reserve`, `SimBlockNew`. With WASM modules (#564) the sim block is attached inside the modules' shared memory, so the block reserves its own memory only with `-modules=native`, which the coverage run does not pass. `SimBlockNew` is called by `Reserve` and, with native modules, by the harness's round trip.
- Command-line switch only (1): `SwitchValue`. It reads the `-blockbytes` and `-searchsteps` switches of match setup; test mode sets the match setup itself (`MatchSetupRequest`), so the command line is never read.

**files, functions no scenario enters** (30 of 142 after `save-midgame` (#422), and `load-midgame` (#442), and `mission-defeat`, `mission-victory` and `mission-radius` (#444), and the six `console-` scenarios (#445); 112 are entered, 78.9%), by reason:

- Network only (6): `LzssCompress`, `LzssDisablePreset`, `LzssDisablePreset`, `LzssEnablePreset`, `LzssEnablePreset`, `LzssExpand`. Needs other machines or a network session; test mode runs every player on one machine. Covered by the replay recordings, #10 and #389.
- Menu or front end only (5): `ChangeDirectory`, `ChangeDrive`, `GetCurrentDriveLetter`, `GetDriveDirectory`, `GetStartDirectory`. The lobby, menus and dialogs, which test mode skips. Covered by #398's UI log.
- Save and load only (2): `AccountBank::WriteAuditFile`, `encryptByte`. Test mode saves (the `save` key, `save-midgame`) and loads that save (the `load` key, `load-midgame`, #442), which enters the `GetDoubleItem`, `GetStringItem` and `ReadBox` readers of `AccountBank` and `HapiBank`.
- Entered by the console: `MatchWildcard`, reached only through `DefaultCommandHandler` (a unit name typed at the console), by `console-units` (#445).
- Key, window or screenshot input only (4): `HAPI_OpenFileAppend`, `HAPI_OpenFileAppend`, `HAPI_WriteFile`, `HAPI_WriteFile`. Test mode sends no input. Covered by #398.
- Shutdown only (2): `HAPI_CloseArchive`, `HAPI_CloseArchive`. `ShutdownIngameSystems` and exit paths; test mode ends with `host_exit` after the last tick, which ends the process with `TerminateProcess`.
- Error paths only (1): `SquashErrorString`. Fatal errors and out-of-memory handlers, which a passing run never reaches.
- Developer tool only (2): `HAPI_CreateFile`, `RenameFile`. The GUI layout editor.
- Game data only (8): `FreeArchiveRecord`, `HAPI_LoadOpenFile`, `HAPI_LoadOpenFile`, `HAPI_TellFile`, `RemoveFile`, `RemoveFile`, `WriteBufferToFile`, `WriteBufferToFile`. Entered only with files the stock install lacks (OVR banks, `download` TDFs, briefings) or with a cache missing (palette and shade tables written on first run). `AccountBank::GetIntegerItem`, `AccountBank::LoadAccount`, `HapiBank::GetIntegerItem`, the other copy of `HAPI_TellFile` and `SquashUnpackedSize` are entered by the load of `load-midgame` (#442).

**state, functions no scenario enters** (68 of 552 after the scenarios of `reference/scenarios/`; 484 are entered, 87.7%), by reason:

- Shutdown only (3): `BlockVector::~BlockVector`, `BlockVector::~BlockVector` and `BlockVector::~BlockVector`. They are the destructors of the damage table, LOS table and order table arrays, which are freed from `FreeWeaponTypes` and `FreeLosTables`, and no scenario runs the shutdown because test mode ends with `host_exit(0)` right after the last tick.
- Template instances no entered function uses (64): `??8@YG_NABU?$Ref@D@@PAD@Z`, `??8@YG_NABU?$Ref@G@@PAG@Z`, `??8@YG_NABU?$Ref@UFrameRing@@@@PAUFrameRing@@@Z`, `??8@YG_NABU?$Ref@VOrderFx@@@@PAVOrderFx@@@Z`, `??8@YG_NABU?$Ref@VPlayerFrameInfo@@@@PAVPlayerFrameInfo@@@Z`, `??9@YG_NABU?$Ref@D@@PAD@Z`, `??9@YG_NABU?$Ref@UHotRadarSlot@@@@PAUHotRadarSlot@@@Z`, `??9@YG_NABU?$Ref@UWeaponDef@@@@PAUWeaponDef@@@Z`, `??9@YG_NABU?$Ref@VPlayerFrameInfo@@@@PAVPlayerFrameInfo@@@Z`, `??9@YG_NABU?$Ref@VUnitDef@@@@PAVUnitDef@@@Z`, `BlockVector::Capacity`, `BlockVector::Capacity`, `BlockVector::Clear`, `BlockVector::CopyAssign`, `BlockVector::CopyAssign`, `BlockVector::CopyAssign`, `BlockVector::CopyAssign`, `BlockVector::CopyAssign`, `BlockVector::CopyAssign`, `BlockVector::CopyAssign`, `BlockVector::CopyAssign`, `BlockVector::CopyAssign`, `BlockVector::Erase`, `BlockVector::Erase`, `BlockVector::Erase`, `BlockVector::Erase`, `BlockVector::Erase`, `BlockVector::Erase`, `BlockVector::Erase`, `BlockVector::Erase`, `BlockVector::Erase`, `BlockVector::operator?4`, `BlockVector::operator?4`, `BlockVector::~BlockVector`, `BlockVector::~BlockVector`, `BlockVector::~BlockVector`, `BlockVector::~BlockVector`, `BlockVector::~BlockVector`, `BlockVector::~BlockVector`, `BlockVector::~BlockVector`, `Ref::Ref`, `Ref::Ref`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::get`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?B`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?C`, `Ref::operator?D`. These are the `Ref` accessors and comparisons of `interfaces/ref.h` and the `BlockVector` members of `src/state/block_vector.h`, one instance for each type and member. No function a scenario enters calls them.
- Error paths only (1): `RefOutsideBlock`. The fatal error of a `Ref` whose pointer lies outside the state block, which a passing run never reaches.

**harness** (875 functions). The test harness of #3 (`src/harness`). No phase root. 44 functions are called from outside; the busiest are `HarnessViewRand` (21), `HarnessCrtSeed` (2). It imports 79 functions (game 24, platform 28).

**presentation** (1430 functions). Rendering, UI and sound (`src/graphics`, `src/gui`, `src/frontend`, `src/ingame`, `src/sound`), except what a phase walk reaches, the main loop and frame handlers (game), and the timer, window and environment code of `src/graphics/display.cpp` (platform). No phase root, and none of its functions calls one (a test checks this). 274 functions are called from outside, mostly by game (218 distinct functions); the busiest are `PlaySoundByName` (7), `GetDisplay` (16), `GetGafFrame` (11). It imports 353 functions: platform 159, network 54, files 32, game 32, map 4. The reverse direction is the one to remove: the nine module subsystems import from presentation, weapons, los_radar and features most of all. Its coverage varies between runs in two functions. `UpdateCdCategoryByActivity` samples the sim's CD-activity counts on a wall-clock timer (`GetTicks`, which reads `host_time_ms`), so which of its branches run depends on how fast the run went. `Lock` (a static copy in each of `src/graphics/palette.cpp`, `pcx.cpp` and `surface.cpp`) waits on the blit-lock event only when another thread holds the lock, so its wait line and branch run in some runs and not others. `data/coverage.csv` marks both, each copy of `Lock` included, with an `unstable:` row and `tools/coverage.py` leaves their lines and branches out of the presentation totals; `FindGafEntry` also read as varying because a lost counter update makes llvm-cov report a saturated count, which the tool now ignores.

**platform** (823 functions). Devices, files, memory, the TDF and `HapiBank` readers, the console and the C library, and the timers (`GetTicks`, `GetMilliseconds`), window and environment code of `src/graphics/display.cpp` (set by `override`). No phase root. 310 functions are called from outside, by presentation (159 distinct functions), game (103), network (51) and units (47); the busiest are `GameFreeThunk` (162), `operator new` (123), `operator delete` (82), then `Translate` (77) and `GameAllocIgnoreTag` (77). It imports 181 functions: game 84, presentation 46.

**seams** (206 functions). No phase root. The native side of the seam function tables of #8 (`src/seams`). 151 functions are called from outside. It imports 158 functions.

**files** (169 functions). No phase root. The archive and bank readers (`src/files`: `HapiBank`, HPI and SQSH archives). 78 functions are called from outside.

## The sim event queue

Sim tells presentation what happened through one queue, so that sim calls no presentation function and keeps no result of one. The queue is a ring of 4096 records of 32 bytes (128 KiB) in the shared state block (one allocation tagged `SIM EVENTS`, made with the sim globals), classed `view`, and in no trace, so it never changes a digest. A save does not hold it: it is empty at the start of a match, after a load and after each frame's drain.

A record is, in order, `kind` (u16), `flags` (u16), `tick` (u32, the tick the event was recorded in) and six u32 arguments (unused ones are 0). The queue also holds a head and a tail, which count the records drained and recorded, and a count of the events dropped. The records between head and tail are the pending events, oldest first.

Sim records an event with `RecordSimEvent(kind, flags, args, count)`, a service of `interfaces/services.h` that a module can call. Values that sim owns (positions, amounts, flags) are copied into the record at that moment. A unit is recorded by its index in the unit table, never by address, a player by its index, and a text, a sound or a GUI screen by its number in a table published with the kind (below). No argument is an address. A check that reads only sim state (whether the unit is alive, for instance) is made by sim before it records. A check that depends on the machine (which player is the local one, the selection, the camera, the screen, the sound settings, a display option) is made by presentation at the drain, so that sim records the same events on every machine.

An event with more than six arguments (up to twelve) takes a second record straight after the first, with flag bit 15 set (`SIM_EVENT_CONTINUED`) and the seventh to twelfth arguments. Both records are recorded or neither: a queue with room for one only drops the event. The drain reads the pair as one event. The arguments below are listed as one run, in order, and a number in brackets is the count of values (a position is three: x, y, z, as fixed-point integers).

Presentation drains the queue at fixed points, in every mode: at the start of each frame (`MainFrameTick`), before the speech pump, which is the first reader of speech; on entry to `RunGameSteps`; in each step after every phase from 1 to 7 (the drain after phase 7 comes before the camera follow and the particle update); and at each step's end, before the step is closed, for what the later parts of the step recorded (the resource sharing of network play, the blink and the expiry of eyeballs); and in `ShutdownIngameSystems`, after the kill of every unit and before `FreeUnitMemory` frees the units, for what the kills of a game's shutdown recorded (a layout change and the units' last speech), which name units by index. The frame-start and entry drains take what was recorded outside any step (the units placed at the start of a match, which speak at once; a build menu change, a screen redraw, a line of speech from an order). Nothing is recorded between the last step's closing drain and the end of the call, so there is no drain after the loop. A drain takes the records in order, oldest first, and acts on each as the call it replaced did. Each drain sits before the first reader of what was recorded, because 3.1c applied these effects inline, where they were recorded: the speech pump reads the speech queue at the start of the next frame, and the camera follow and the particle update read shakes and new particles in the same step. A re-implementation must keep these drain points: draining later moves the shake and the first update of new particles by a step, which the traces cannot see but the stored frames can. Drawing from the view random stream, the only stream presentation draws from, moves with the effect to the drain, and it is not hashed. The drain after each phase, and not one after the last, is there for the order of those draws: in 3.1c every emitter draws when it is called, so the draws of a phase come before the draws of the next phase, and a draw made inline by a later phase (a puff of smoke from the projectiles) must come after the queued effects of the earlier phases (the wake of a ship). A drain only after phase 7 let the inline draw come first, which swapped the values two emitters received; every later shake offset and particle timer then differed, 20 to 35 ticks before any count of draws did. The view draws are counted in the UI log (`view` record, docs/ui-log.md), and the check compares them with the reference tick by tick.

When the queue is full, a test-mode run ends with a fatal error naming the queue, so that a scenario can never lose an event unseen. When playing, the new event is dropped and counted. The most that one step recorded in the 65 scenarios of the suite is 123 records, and the first drain after a load held 352 (336 plumes over the geothermal vents of one map, recorded while the features were placed), so the ring holds more than eleven times the larger count.

The radar events carry no values in the record: sim appends a snapshot to the radar arena, a second buffer of the state block (2 MiB, one allocation tagged `SIM RADAR`, classed `view`, in no trace), and records the snapshot's offset. A units snapshot is 8 bytes and 48 bytes per unit the radar shows and 12 bytes per projectile it shows. A mapped snapshot is 12 bytes and one byte per half-resolution map cell. The drain draws from the snapshots in order and, when the queue is empty, empties the arena. A full arena ends a test-mode run with a fatal error, as a full queue does, and drops the snapshot when playing.

To add a kind of event: add its name and the meaning of its arguments to `SimEventKind` in `interfaces/services.h`, record it in sim, handle it in the drain of presentation, and add its section to docs/sim-events.md.

The kinds, with the arguments of each and what the drain does:

| kind | arguments | the drain |
|---|---|---|
| 1, unit speech | the unit, the kind of line (the voice slot, as the unit's sound category names it), a speech text (below). Flag bit 0: only said while the unit is off screen | Skips the event when the unit does not belong to the local player, when it has died since (its alive bit is clear: its death removes its pending lines) and, for the flagged event, when the unit is on screen at that moment. Then the line enters the speech queue, which keeps one line per kind, orders the lines by priority and speaks the first when the speech interval has passed. A slot freed and given to a new unit in the same frame cannot be told from the old one, which is the one case that differs from speaking at the call |
| 2, sound | a sound name (below), the play flag | Plays the sound by name |
| 3, nano particles | the origin (3), the low and high corners of the target box (6), the particle list | A build, repair or resurrect beam of particles flows from the origin to the box |
| 4, reverse nano particles | the low and high corners of the source box (6), the origin (3), the particle list | A reclaim or capture beam of particles flows from the box to the origin |
| 5, teleport particles | the start (3), the end (3), the lifetime in ticks, the particle list | Sparks fly from the start to the end |
| 6, thrust particles | the start (3), the end (3), the period, the lifetime in ticks, the particle list | A jet plume streams from the start to the end |
| 7, wake particles | the start (3), the end (3), the period, the particle list | A wake trails from the start to the end |
| 8, bubbles | the start (3), the end (3), the period, the particle list | Bubbles rise from the start to the end |
| 9, weapon smoke | the position (3), the particle list | A puff of smoke where a weapon fired |
| 10, timed plume | the position (3), the particle list | A plume over a geothermal vent |
| 11, screen shake | the two offsets, the strength | Adds to the shake of the screen, unless the player turned screen shake off |
| 12, disc activity | the amount, two players | Counts the amount in the disc activity meter (which picks the music) when either player is the local player |
| 13, forces destroyed | the player | Prints the message that the side's forces are destroyed, in the side's name and with one of three texts chosen from the view random stream |
| 14, score flash | the killer's slot, the loser's player | Flashes the killer and the loser on the score panel, when the player's options show it |
| 15, selection orders dirty | the unit | Redraws the orders panel when the unit is selected and belongs to the local player |
| 16, build menu update | the unit | Writes the queued build counts into the build menu when the unit is the one shown |
| 17, deselect | the unit | Removes the unit from the selection when it can no longer be selected (unfinished, in transit or in a transport that is not allowed to carry it) |
| 18, redraw screen | a screen (below) | Marks the GUI as changed when that screen is on top |
| 19, focus commander | the force flag | Centres the camera on the commander of the viewed player and, with the flag, selects it and closes the open screens |
| 20, close screens | the force flag | Closes the screens above the game screen, unless a dialog is open and the flag is 0 |
| 21, player took the lead | the player, the kills that put it there | Prints the message that the player has taken the lead, in the player's name and with the kills |
| 22, player left | the player's network id | Prints the message that the player left the game, in the player's name and with one of eight texts chosen from the view random stream, unless the local player was rejected or no slot has the id |
| 23, build queue sound | the player of the unit, the change | Plays the add or subtract sound of the build menu when the player is the one shown |
| 25, watch prompt | the player | Asks the local player whether to keep watching after a defeat |
| 26, watch mode notice | the player | Tells the local player that hosting AI players that are still alive keeps the match going |
| 27, smoke | the position (3), the emit period, the lifetime in ticks, the particle list | A smoke plume rises from the position: over a corpse, or behind an explosion on land. The smoke picks its animation frames from the view random stream |
| 28, white smoke | the position (3), the particle list | A puff of white smoke: a burning feature, a projectile trail or end, a venting piece of a unit |
| 29, black smoke | the position (3), the particle list | A puff of black smoke from a unit's script |
| 30, sound at | the sound index of the game, the position (3), the players who hear it (bit n for player n) | Plays the sound at the position, when the sound settings allow it and the local player is one who hears it (the position is on the map and, as sim saw it when recording, explored or in sight of that player). With 3D sound off, a sound off screen plays quieter |
| 31, sound at name | a sound name (below), the position (3), the players who hear it | Plays the named sound at the position, with the same checks as kind 30 |
| 32, remove speech | the unit | Drops the unit's pending lines from the speech queue |
| 33, message | a message text (below), the value | Puts the line in the message log: the text with ON or OFF (value 0 or not) for a toggle, or with the number |

The particle list is the index of the list the particles are drawn in. A particle kind that sim cannot change is not an argument.

**Speech texts.** Argument 3 of a unit speech is 0 for the text of the line's kind, or one of these; the text is translated at the drain: 1 Attacking, 2 Building, 3 Building complete, 4 Capture failed, 5 Capturing, 6 Construction stopped, 7 Construction terminated, 8 Construction terminated by hostile action, 9 Guarding, 10 I can't get there, 11 I can't reach the construction site, 12 Landing, 13 Landing aborted, 14 Landing aborted: all pads are occupied, 15 Landing aborted: no pads available, 16 Landing failed, 17 Loading, 18 Loading unit, 19 Patrolling, 20 Preparing for transport, 21 Reclaiming, 22 Reclamation failed, 23 Repair aborted., 24 Repairing, 25 Repair mission failed, 26 Repairs unsuccessful., 27 Ressurection failed, 28 Resurrection complete, 29 Resurrection failed, 30 Starting construction, 31 Target area was blocked, 32 That unit cannot be captured, 33 That unit cannot be reclaimed, 34 That unit is a cloud of vapor and cannot be captured, 35 Transport mission failed, 36 Unable to create any more units, 37 Unable to unload unit, 38 Unit is too heavy to transport, 39 Unit is too large to transport, 40 Unit repaired, 41 Unloading, 42 Unloading process is proceeding non-optimally, 43 Waiting for target area to clear.

**Sound names.** 0 is "Victory Condition", the cue of a satisfied victory condition, and 1 is "treeburn", the sound of a feature catching fire.

**Screens.** 0 is "BUILDER.GUI", the build menu.

**Message texts.** 0 Toggled ShareMetal to: %s, 1 Toggled ShareEnergy to: %s, 2 Toggled ShareMapping to: %s, 3 Toggled ShareLOS to: %s, 4 Toggled ShareRadar to: %s (each with ON or OFF), 5 OK.  Will share metal if above %d, 6 OK.  Will share energy if above %d.

**What stays a call.** A call whose result sim uses, or that sim makes to set up a screen or load an asset, is not an event: the dialog handlers and the dialogs that sim builds and hands a handler to, the camera follow block of the unit update, the model state of COB pieces, the radar picture and the load-time asset calls are not on the queue (#608 removes them). The visibility check of a unit effect (`UnitScript::EmitSfx`) stays where it is, because it also gates the piece transforms that sim stores. An error box that stops the game until it is answered (a unit file or mission file that cannot be used) is a platform call through `host_message`, not an event. `GetLocalizedString` reads loaded data into the unit and mission definitions that sim keeps, so it is an engine service of `interfaces/services.h`, not an event.

## Calls that are not events

Sim calls these engine services of `interfaces/services.h` instead of recording an event, because they build or load what sim keeps and return a result: the GAF readers (`GetGafFrame`, `GetGafSequenceFrame`, `InitGafSequence`, `StepGafSequence`, `FindGafEntry`, `LoadGaf`), the model and script loaders (`Load3do`, `MirrorObject`, `BindModelTextures`, `GetObjectHeight`, `LoadCobScript`, `FreeCobScript`, `GetCobChecksum`), the frame and asset loaders (`AllocFrame`, `BuildLensFrame`, `StripExtension`, `LoadSoundByName`), the unit picture cache (`InitUnitPictureCache`, `FreeUnitPictureCache`, `ReleaseUnitPictures`), the map list (`LoadMapList`, `FreeOtaEnumCache`) and the radar frame of a TED map (`BuildTedRadarFrame`). The piece transforms (`CreateObjectState`, `FreeObjectState`, `UpdateObjectState`) are units code reached through three units seam slots, the conditions object is built through the players slot `NewConditions`, and the build count change is the orders slot `QueueBuildOrder`.

Presentation code that used to run inside a tick runs from the frame code now. The auto-follow camera block (the follow key, the selection and camera cycling) is `UpdateAutoFollow`, called by the frame code right after the unit update and before the phase hash, at the point of the tick where it used to run.

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

A live function that the suite cannot enter by design is counted apart, like a dead one: its row has `functions` 1, `dead` 0 and a reason starting `kit:` or `demo:`, and it is left out of every count of its subsystem, the never-entered column included. `kit:` marks the native of a seam slot that the oracle kit added and no 3.1c code calls (only in `seams`). `demo:` marks a function of the HPA* pathfinder demo (`src/ai/pathfind_hpa.cpp`, #930) or of the idle ai demo (`src/ai/ai_idle.cpp`, #968): alternatives to 3.1c's pathfinder and ai that a scenario can select only by changing its traces, so the pathfinding and ai rows measure the default modules alone. `--check` fails when such a function is entered, and `tools/coverage.py` prints both lists after its table.

| subsystem | entered of live functions | functions | lines | branches | never entered | dead |
|---|---|---|---|---|---|---|
| **wind_meteors** | 0 of 15 | 0.0% | 0.0% | 0.0% | 15 | 0 |
| **economy** | 51 of 54 | 94.4% | 91.4% | 73.9% | 3 | 3 |
| **features** | 54 of 55 | 98.2% | 87.3% | 73.7% | 1 | 4 |
| **ai** | 87 of 109 | 79.8% | 75.6% | 72.9% | 22 | 31 |
| **pathfinding** | 69 of 78 | 88.5% | 82.3% | 69.4% | 9 | 10 |
| **weapons** | 96 of 110 | 87.3% | 88.8% | 74.0% | 14 | 19 |
| **targeting** | 20 of 20 | 100.0% | 96.1% | 79.6% | 0 | 1 |
| **los_radar** | 87 of 96 | 90.6% | 87.6% | 74.4% | 9 | 22 |
| **orders** | 227 of 235 | 96.6% | 85.9% | 65.3% | 8 | 17 |
| **units** | 116 of 133 | 87.2% | 85.0% | 68.6% | 17 | 27 |
| **players** | 103 of 156 | 66.0% | 67.7% | 61.0% | 53 | 10 |
| **map** | 70 of 74 | 94.6% | 93.9% | 82.5% | 4 | 9 |
| **mission** | 28 of 47 | 59.6% | 74.9% | 62.7% | 19 | 1 |
| **scripts** | 62 of 81 | 76.5% | 89.7% | 84.7% | 19 | 6 |
| **movement** | 94 of 148 | 63.5% | 69.3% | 63.6% | 54 | 8 |
| files | 112 of 142 | 78.9% | 79.6% | 65.0% | 30 | 35 |
| game | 251 of 334 | 75.1% | 59.4% | 39.2% | 83 | 23 |
| network | 54 of 297 | 18.2% | 11.1% | 4.1% | 243 | 74 |
| platform | 400 of 748 | 53.5% | 41.0% | 28.0% | 348 | 86 |
| presentation | 565 of 1150 | 49.1% | 36.2% | 29.6% | 585 | 295 |
| seams | 179 of 205 | 87.3% | 90.7% | 49.0% | 26 | 0 |
| state | 484 of 552 | 87.7% | 70.2% | 49.3% | 68 | 59 |

Functions are assigned to a subsystem by `data/subsystems.csv` first, then by the names of special members (`X::X`, `X::~X`, `X::operator<code>`). A function still without a row is `state` when its file is under `interfaces/` or `src/state/`, else it gets the subsystem that the rules of `tools/coupling.py` give its name and file. Functions in `src/harness/`, `build/` and `toolchain/` are left out.

The runtime is clang's profile runtime (`lib/profile` of compiler-rt), built from source for i686 against the MSVC 5 C runtime in `CMakeLists.txt`. It works under Wine, so the fallback of clang's trace-pc-guard instrumentation was not needed. The game ends with `TerminateProcess`, and the runtime's own initialiser is not linked, so `host_exit` writes the profile itself (`__llvm_profile_initialize_file`, then `__llvm_profile_write_file`). `LLVM_PROFILE_FILE` takes a `Z:\` path.

## Notes from the data

- **`order_targets.cpp` is in orders.** It holds the order goal objects (`PathGoal`, `PointMarker`, `RingApproach` and the rest: most of the file is in orders), so the file rule puts it in `orders`, while #16 lists it for targeting. Targeting is then small, and most of it is from `unit_targets.cpp`. The pathfinding section shows the same line from the other side: the goals are what pathfinding calls into.
- **`movement_class.cpp` is units'.** The file holds the movement class table and its definition functions, plus `NopAfterFeatureEnum`, which is features'. The pass maps are map's, in `src/map/pass_map.cpp` (#1002), while #14 lists `movement_class.cpp` for pathfinding.
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
