# The mission interface

The mission sits behind `interfaces/mission.h` (version 2.0: 1.0 in #983, the slots of the record's fields in #1003, and 2.0 makes the strings the slots take and return const). It is the match's mission as the engine sees it:
the game type, the files of the match (the campaign, the terrain, the briefing), the campaign and its missions, the
start positions and the list of maps. By the human's ruling of 2026-11-17 on #980 it is its own interface, a module
that can be replaced like any other, rather than a part of map or of the engine. 3.1c's code is its default
implementation (`src/mission/`). This page is the inventory the interface was cut from:
- which slots the mission has, who calls each and whether inside the tick;
- what moved, and from where;
- the shape of the slots;
- what the mission keeps, and what the trace sees of it;
- what is left.

## 1. The model

The engine holds one mission, the record `Game::net` points to, and every slot works on it, so no slot takes the
mission as an argument. A string a slot returns, or writes into a struct the caller owns, points into the mission's own
record and stays valid until the mission loads another. Nothing outside the mission reads the record: every caller goes
through the slots (section 5), so a mission module with its own layout can serve them.

The mission is not on the scheduler: it has no events, no inbox, no runs and no module id. Every slot is a direct call:
- **Queries inside the tick:** `GetGameType`, `GetNameSlot` and `GetMissionParams`. They read a record nothing in the tick writes (the
  mission is loaded before the match and replaced only from the menus), so a read as of the call is a read as of the
  last load, and no order can differ from 3.1c's.
- **Everything else** runs outside the tick: from the front end's menus, the load of a match, the saved game and the
  end of a game. These slots load and replace the mission and read it for the menus.

The mission draws no random number (no row of `data/draws.csv`) and records no sim event.

## 2. The slots

Thirty-four slots. The first sixteen were map's (map 4.1, `docs/map-interface.md`, section 2); the next ten are for calls
3.1c's callers made straight into the mission's methods, which had no seam (mission 1.0); the last eight serve the fields
of the record that 3.1c's callers read directly (mission 1.1, section 5). The callers are counted by subsystem
(`data/rwsets.csv`, the reads of each slot, and `data/subsystems.csv`).

| Slot | 3.1c | Callers | Inside the tick |
|---|---|---|---|
| `GetGameType` | `Mission::GetGameType` | 48: units (`ApplyUnitDeath`, `ComparePlayers`), orders (`ResurrectOrder`), features (`DamageFeature`, `ReclaimFeature`), players (the player updates, the slot set-up and the victory conditions), ai (`GetBuildRating`), network, game, presentation (`LoadMapList` among them) | yes: units, orders, features, players, ai, network |
| `GetNameSlot` | `Mission::GetNameSlot` | 12: ai (`DumpPlayerAI`, `LoadAIProfiles`, the AI profile parser), players (`ApplyUseOnlyUnits`), los_radar (`LoadTntMap`), presentation | at an AI's load and the console's dump only |
| `SetMissionType` | `SetMissionType` | 12: game, presentation, network (the lobby), harness | no |
| `FreeOtaEnumCacheAndMission` | `FreeOtaEnumCacheAndMission` | 1: game (`ShutdownGame`) | no |
| `AdvanceMission` | `Mission::AdvanceMission` | 1: game (`SaveGameFile`) | no |
| `GetCampaignName` | `Mission::GetCampaignName` | 3: game, harness | no |
| `GetMissionIndex` | `Mission::GetMissionIndex` | 2: game, presentation | no |
| `GetMissionName` | `Mission::GetMissionName` | 13: game, network, presentation | no |
| `GetStartPosition` | `Mission::GetStartPosition` | 3: game (`LoadMatch`, `SpawnCommanderAtStartPos`), presentation (`CenterCameraOnStartPosition`) | no: the load and the camera |
| `LoadCampaign` | `Mission::LoadCampaign` | 10: game, presentation, harness | no |
| `LoadMissionByName` | `Mission::LoadMissionByName` | 17: game, presentation, harness | no |
| `MissionExists` | `Mission::MissionExists` | 3: game, presentation | no |
| `RefreshMapList` | `Mission::RefreshMapList` | 5: game, presentation | no |
| `SelectMission` | `Mission::SelectMission` | 7: game, presentation | no |
| `ComputeMapChecksum` | `Mission::ComputeMapChecksum` | 3: presentation (the lobby's map check) | no |
| `GetTerrainLength` | `Mission::GetTerrainLength` | 1: presentation (the lobby's map check) | no |
| `CountMissions` | `Mission::CountMissions` | 1: presentation (`BuildMissionNameList`) | no |
| `GetCampaignMissionName` | the loop of `Mission::BuildMissionList` | 1: presentation (`BuildMissionNameList`) | no |
| `CountStartPositions` | `Mission::CountStartPositions` | 2: presentation, harness | no |
| `GetBriefing` | `Mission::GetBriefing` | 1: presentation | no |
| `GetDescription` | `Mission::GetDescription` | 1: presentation | no |
| `GetPlanet` | `Mission::GetPlanet` | 1: presentation | no |
| `GetTerrainSizeTier` | `Mission::GetTerrainSizeTier` | 2: presentation (the lobby) | no |
| `GetTranslatedName` | `Mission::GetTranslatedName` | 2: presentation | no |
| `HasMissionName` | `Mission::HasMissionName` | 4: presentation | no |
| `SelectSchema` | `Mission::SelectSchema` | 1: presentation (`LoadMapList`) | no |
| `GetMissionParams` | the record's scalars | 16: units (`UpdateUnit`), weapons (`NoSeaLevelTrigger`, `UpdateExplosions`, `UpdateExplodedPiece`, `LoadWeaponType`), orders (`AirStrikeOrder`), players (`UpdatePlayerBody`, `InitPlayerResources`, `FillScoreTables`), ai (`FindBuildPosition`, `FindRandomPlacementCell`), los_radar (`LoadTntMap`, `ClearBorderFeatures`), presentation | yes: units, weapons, orders, players, ai |
| `CountMissionUnits` | `Mission::unitCount` | 2: game (`CreateMissionUnits`), units (`FindMissionUnit`) | no: the load |
| `GetMissionUnit` | `Mission::units` | 2: game (`CreateMissionUnits`), units (`FindMissionUnit`) | no: the load |
| `SetMissionUnitPosition` | the snap of `CreateMissionUnits`, in place | 1: game (`CreateMissionUnits`) | no: the load |
| `CountMissionFeatures` | `Mission::featureCount` | 1: features (`PlaceMissionFeatures`) | no: the load |
| `GetMissionFeature` | `Mission::features` | 1: features (`PlaceMissionFeatures`) | no: the load |
| `GetMemory` | `Mission::memory` | 1: presentation (`ShowSelectedMapInfo`) | no |
| `GetNumPlayers` | `Mission::numPlayers` | 1: presentation (`ShowSelectedMapInfo`) | no |

"Inside the tick" is the simulation's own calls. `data/hazards.csv` also marks some calls of the menus and of
`SaveGameFile` as reached from the tick, because the frame handlers that run them are reached from the main loop; they
run between ticks, from a click or the console, and read or replace the mission while nothing of the simulation runs.

The harness's test mode calls `SetMissionType`, `LoadCampaign`, `LoadMissionByName`, `GetCampaignName` and
`CountStartPositions` to set a scenario up (`BeginMission`, `src/harness/harness.cpp`), through the seam like any
other caller.

## 3. What moved

- **The slots:** the sixteen of map 4.1 left `MapSeam` (map 5.0); the ten others are new. `g_seams.mission` is the
  table (`interfaces/seams.h`), filled by `InstallNativeSeams` (`src/seams/native.cpp`) with fillers that call 3.1c's
  methods on the one mission.
- **The callers:** every call of a mission method outside `src/mission/` goes through the seam, and since 1.1 so does
  every read of the record's fields (section 5). `tools/seamlink.py --mission-record` fails on a new one.
- **The files:** `src/map/map_list.cpp`, `map_list.h`, `map_load.cpp`, `map_cache_entry.h` and `mission.h` moved to
  `src/mission/`. `data/subsystems.csv` gives every function of the folder to the subsystem `mission` (the rule
  `file:src/mission/`), and `tools/coupling.py` counts it as a simulation subsystem.
- **What stays in map:** the terrain, the ground height, the pass maps, the footprints and the placement tests
  (`docs/map-interface.md`). The mission's load still hands the map's parsed file to features, players and
  wind_meteors through their seams, as 3.1c's `LoadMission` called them.

## 4. The shape of the slots

3.1c's methods took the mission as `this`; the slots take no mission, since the engine holds one and every caller
passed the same record (`g_game->net`). That is the one change of shape. Everything else is 3.1c's type, as no new
decision was needed: a name is a `char*` into the mission's record, a start position a `Vec3*` the slot writes, the
map's parsed file of `SelectSchema` 3.1c's `TdfFile*`, and the game type an `int` (0 none, 1 campaign, 2 skirmish, 3
multiplayer). Every slot is `__stdcall`.

No slot passes ownership of memory across the seam (the ruling on #983's pull request). A mission module has its own
allocator, so memory it allocates can't be freed by its caller, and the reverse. 3.1c's `BuildMissionList` gave its
caller a list of the campaign's mission names to free. In its place:
- `CountMissions` returns how many missions the campaign has.
- `GetCampaignMissionName` writes the localised name of one mission to a buffer the caller owns.
- The front end builds the list its menus show in its own memory (`BuildMissionNameList`,
  `src/frontend/campaign_menu.cpp`). It is used by the campaign menus (`FillMissionList`, `FillMissionListInline`,
  `OpenNewGameMenu`) and the end screen (`OpenEndMissionScreen`).
- The front end makes the same allocation as before, and the mission selects the same records in the same order.
  `LoadMissionByName` still builds and frees its own list inside the mission.

The other slots were checked against the same rule:
- Every `char*` a slot returns points into the record the mission keeps.
- `GetStartPosition` writes to a `Vec3` the caller owns, and `SelectSchema` borrows the caller's parsed file and frees
  nothing.
- The slots that load or replace the mission (`SetMissionType`, `LoadCampaign`, `LoadMissionByName`, `SelectMission`,
  `AdvanceMission`, `RefreshMapList`) allocate and free only the mission's own memory.
- So do map 5.0's new slots (`GetGroundHeight`, `MovementClassGet`, `MovementClassInBounds` (renamed `PassMapGet` and `PassMapInBounds` in 7.0), `RefreshMovedUnits`,
  `RefreshUnitIfStale`).
- The slots of 1.1 hand plain data. `GetMissionParams`, `GetMissionUnit` and `GetMissionFeature` write to a struct the
  caller owns (`MissionParams`, `MissionUnitInfo`, `MissionFeatureInfo`); the names in the last two point into the
  record, as the other strings do. The units and the features are a count and a query by index, not a pointer to the
  mission's array. `SetMissionUnitPosition` copies a `Vec3` the caller owns into the mission's own record.
- `GetCampaignMissionName` returns 0 for a `size` below 1 and writes nothing, since its fallback would write
  `out[size - 1]` (the review of #983's pull request).

## 5. State and the trace

The mission's record (`Game::net`, `0xec4` bytes) is allocated by `SetMissionType` outside the tick and freed by
`FreeOtaEnumCacheAndMission`; its class in `data/state_classes.csv` is unchanged (mixed: the sim reads the wind, tide,
water damage, kill and time multipliers and the start positions; the names and texts are the view's). The lobby's
cache of map checksums (`s_mapCache`, `src/mission/map_list.cpp`) is platform state, as before. No trace field changed,
so the gate of this change was the tier.

No file outside `src/mission/` reads a field of the record (#1003). For each field 3.1c's callers read, the choice was a
slot (a query, read when needed) or a copy into the reader's own state taken at the load. Every field is a slot:

| Reader | Fields | Slot |
|---|---|---|
| features (`PlaceMissionFeatures`) | `features`, `featureCount` | `CountMissionFeatures`, `GetMissionFeature` |
| los_radar (`LoadTntMap`, `ClearBorderFeatures`) | `tidalStrength`, `surfaceMetal`, `minWindSpeed`, `maxWindSpeed`, `lavaWorld`, `gravity` | `GetMissionParams` |
| weapons (`weapons.cpp`, `explosions.cpp`, `weapon_types.cpp`) | `noSeaLevelTrigger`, `lavaWorld` | `GetMissionParams` |
| units (`UpdateUnit`, `FindMissionUnit`) | `waterDamage`, `waterDoesDamage`, `units`, `unitCount` | `GetMissionParams`, `CountMissionUnits`, `GetMissionUnit` |
| game (`CreateMissionUnits`) | `units`, `unitCount` | `CountMissionUnits`, `GetMissionUnit`, `SetMissionUnitPosition` |
| players (`InitPlayerResources`, `UpdatePlayerBody`, `FillScoreTables`) | `startMetal`, `startEnergy`, `lavaWorld`, `killMul`, `timeMul` | `GetMissionParams` |
| ai (`FindBuildPosition`, `FindRandomPlacementCell`) | `surfaceMetal` | `GetMissionParams` |
| orders (`AirStrikeOrder`) | `gravity` | `GetMissionParams` |
| presentation (`campaign_menu.cpp`, `endgame.cpp`) | `minWindSpeed`, `maxWindSpeed`, `gravity`, `killMul`, `timeMul` | `GetMissionParams` |
| presentation (`map_enum.cpp`) | `type` | `GetGameType` |
| presentation (`multi.cpp`) | `memory`, `numPlayers` | `GetMemory`, `GetNumPlayers` |
| presentation (`camera.cpp`) | the start positions (`rules`, `ruleCount`) | `GetStartPosition` |

Why slots and no copies:
- **The trace.** The trace hashes the record (the mission section of the constants, `docs/trace-format.md`), and none of
  the readers' own state holds these fields. A copy taken at the load would add hashed state to a reader or leave it out
  of the hash, and either changes the trace; a slot changes nothing that is hashed.
- **The order.** Each slot is a query of a record nothing in the tick writes (section 1), so a read through the slot
  is a read as of the last load, as 3.1c's direct read was.
- **The shape.** The scalars are one plain struct, `MissionParams`, which the mission fills on each call. `type` is
  not in it: `GetGameType` already serves it.

`SetMissionUnitPosition` is the one write. 3.1c's `CreateMissionUnits` snaps each mission unit's position to its
footprint (map's `SnapWorldPosToFootprint`) in the record itself, and the trace hashes the record's units. The load now
snaps a copy and writes it back through the slot, so the record ends as 3.1c's did, and the unit is created at the
same position.

`tools/seamlink.py --mission-record` (CI, the seamlink step) fails on a field, deref or call row of `data/rwsets.csv`
that reaches `Game::net` or a member of `Mission`, `MissionUnit`, `MissionFeature` or `MissionRule` from a function
outside the subsystem mission, other than the mission's fillers in `src/seams/native.cpp`.

**At load**: nothing is restored from the saved game's body. The save keeps only the mission's name, and the saved game's menu path (`savegame.cpp`) calls `LoadMissionByName` with it before `LoadSavedGameState` runs, so the record is read again from the mission's file, as for a new match.

## 6. Replays

The mission subsystem is `pending` in `data/replay_schema.json`: its 34 slots are annotated `record: false`, with the
reason `#660 part 3`, and nothing records or replays them yet. The recordings of the other subsystems are unchanged
by it: their imports of the mission (`GetGameType`, `GetNameSlot`) are not recorded either, as in map 4.1.

## 7. What is left

- **The trace of the record.** The harness hashes the record by 3.1c's layout (`VisitMission`,
  `src/harness/harness.cpp`). Every caller of the engine is served through the seam, but a mission module with its own
  layout would still need a way to hand the harness what it hashes, as the pilots' `WriteState` slots do.
- **A WASM module.** The mission has no module yet; the modern build runs the default as native code in every mode.
- **Replays** of the mission's slots (section 6).
