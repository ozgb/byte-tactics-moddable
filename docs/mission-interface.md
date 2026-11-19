# The mission interface

The mission sits behind `interfaces/mission.h` (version 1.0, #983). It is the match's mission as the engine sees it:
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
mission as an argument. A string a slot returns points into the mission's own record and stays valid until the mission
loads another.

The mission is not on the scheduler: it has no events, no inbox, no runs and no module id. Every slot is a direct call:
- **Queries inside the tick:** `GetGameType` and `GetNameSlot`. They read a record nothing in the tick writes (the
  mission is loaded before the match and replaced only from the menus), so a read as of the call is a read as of the
  last load, and no order can differ from 3.1c's.
- **Everything else** runs outside the tick: from the front end's menus, the load of a match, the saved game and the
  end of a game. These slots load and replace the mission and read it for the menus.

The mission draws no random number (no row of `data/draws.csv`) and records no sim event.

## 2. The slots

Twenty-six slots. The first sixteen were map's (map 4.1, `docs/map-interface.md`, section 2); the last ten are for calls
3.1c's callers made straight into the mission's methods, which had no seam. The callers are counted by subsystem
(`data/rwsets.csv`, the reads of each slot, and `data/subsystems.csv`).

| Slot | 3.1c | Callers | Inside the tick |
|---|---|---|---|
| `GetGameType` | `Mission::GetGameType` | 47: units (`ApplyUnitDeath`, `ComparePlayers`), orders (`ResurrectOrder`), features (`DamageFeature`, `ReclaimFeature`), players (the player updates, the slot set-up and the victory conditions), ai (`GetBuildRating`), network, game, presentation | yes: units, orders, features, players, ai, network |
| `GetNameSlot` | `Mission::GetNameSlot` | 12: ai (`DumpPlayerAI`, the AI profile parser), players (`LoadDefaultAIScript`, `ApplyUseOnlyUnits`), los_radar (`LoadTntMap`), presentation | at an AI's load and the console's dump only |
| `SetMissionType` | `SetMissionType` | 12: game, presentation, network (the lobby), harness | no |
| `FreeOtaEnumCacheAndMission` | `FreeOtaEnumCacheAndMission` | 1: game (`ShutdownGame`) | no |
| `AdvanceMission` | `Mission::AdvanceMission` | 1: game (`SaveGameFile`) | no |
| `GetCampaignName` | `Mission::GetCampaignName` | 3: game, harness | no |
| `GetMissionIndex` | `Mission::GetMissionIndex` | 2: game, presentation | no |
| `GetMissionName` | `Mission::GetMissionName` | 13: game, network, presentation | no |
| `GetStartPosition` | `Mission::GetStartPosition` | 2: game (`LoadMatch`, `SpawnCommanderAtStartPos`) | no: the load |
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
- **The callers:** every call of a mission method outside `src/mission/` goes through the seam. The mission's fields
  that the simulation reads directly (section 5) are views, not calls, and stay as they are.
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
- So do map 5.0's new slots (`GetGroundHeight`, `MovementClassGet`, `MovementClassInBounds`, `RefreshMovedUnits`,
  `RefreshUnitIfStale`).

## 5. State and the trace

The mission's record (`Game::net`, `0xec4` bytes) is allocated by `SetMissionType` outside the tick and freed by
`FreeOtaEnumCacheAndMission`; its class in `data/state_classes.csv` is unchanged (mixed: the sim reads the wind, tide,
water damage, kill and time multipliers and the start positions; the names and texts are the view's). The lobby's
cache of map checksums (`s_mapCache`, `src/mission/map_list.cpp`) is platform state, as before. No trace field changed,
so the gate of this change was the tier.

What the simulation still reads of the record directly, as views of a record nothing in the tick writes:
- features: the mission's features and their count, at the load of a match;
- los_radar: the tidal strength, the surface metal, the wind's limits, `lavaWorld` and the gravity, at the load;
- weapons: `noSeaLevelTrigger` and `lavaWorld`;
- units: the mission's units and their count, `waterDamage` and `waterDoesDamage`;
- players: the start metal and energy;
- ai: the surface metal.

Each is read as of the last load, so no order of 3.1c's changes. Turning them into slots or into copies taken at the
load is #1003 (section 7).

## 6. Replays

The mission subsystem is `pending` in `data/replay_schema.json`: its 26 slots are annotated `record: false`, with the
reason `#660 part 3`, and nothing records or replays them yet. The recordings of the other subsystems are unchanged
by it: their imports of the mission (`GetGameType`, `GetNameSlot`) are not recorded either, as in map 4.1.

## 7. What is left

- **The views of section 5.** A re-implementation of the mission must keep the record's layout today, since the
  simulation reads its fields directly. #1003 makes them queries or copies taken at the load, with the front end's
  direct reads of the record (`campaign_menu.cpp`, `map_enum.cpp`).
- **A WASM module.** The mission has no module yet; the modern build runs the default as native code in every mode.
- **Replays** of the mission's slots (section 6).
