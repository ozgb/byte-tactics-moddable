# The view

The view is what a presentation (drawing, the interface, sound and music) reads of the simulation, for one player. It is
a flat block, `PlayerView` of `interfaces/view.h`, with fixed-width fields and tables of fixed capacity, and no pointer
into the state block. The block is filled once after each tick, and the rule of visibility of each fact is applied
then: a presentation that reads only the block sees what 3.1c lets that player see, and nothing else. The same block
serves the local renderer, a network client that never holds the whole state, and a UI mod.

This page states each fact of the block: its source in the judged state (`docs/trace-format.md`, by the dump path of
the field, such as `units[i].health`) or in the sim events (`docs/sim-events.md`), and its rule of visibility. The rules
are 3.1c's, quirks included; a quirk is a fact of the view until a quirk flag (#1) gives a profile the choice. Nothing
fills the block yet: recording it every tick, as a hash and as the whole block on the ticks with stored frames, is
#1063.

The facts are what presentation reads: the last section, "The reads", lists every field of the simulation that a
function of presentation reads during play or in its menus, and says where each one is in the view, or why it is not.

## Words

- **Viewer:** the player whose sight, economy and units the view holds, `PlayerView.viewer`.
- **Controller:** the player whose units the machine's input commands, `PlayerView.controller`. It is the viewer,
  unless the console's `view` command has made the viewer another player ("The two players").
- **In sight:** a point is in the viewer's sight when the cell of the sight grid under it says so (`VIEW_SIGHT_IN_SIGHT`)
  with line of sight on, or when the cell is revealed (`VIEW_SIGHT_MAPPED` or in sight) with line of sight off. This is
  3.1c's one test, repeated by every draw that tests sight. The cell of a point is column `x >> 21` (the whole part
  over 32) and row `(z - y / 2) >> 21`, in the 16.16 world coordinates; a point off the grid is not in sight.
- **Own, allied:** a unit's owner is the viewer, or a player allied to the viewer.
- **Loaded data:** the unit, weapon, feature and side definitions, the models and the animations. The view names them
  by index; their contents come through the files service ("Loaded data").

## The two players

3.1c has two player fields where the view has two: `game.localPlayer` and `game.playerIndex` (`docs/trace-format.md`,
section 8.1). They are equal from the match's start. The console's `control` command sets both to another player, and
`view` sets only `game.playerIndex`. The view holds:

| Field | Source | Meaning |
|---|---|---|
| `viewer` | `game.playerIndex` | Whose sight decides what is drawn and heard; whose economy the resource bars show; whose units have health bars, group numbers and the unit panel's own-only lines. |
| `controller` | `game.localPlayer` | Whose units the selection and the orders act on; whose unit count the status panel shows; whose row the score panel highlights; whose notices (`SIM_EVENT_WATCH_PROMPT`, `SIM_EVENT_WATCH_MODE_NOTICE`, `SIM_EVENT_CD_ACTIVITY`) reach the presentation. |

The simulation also reads both fields as its perspective (line of sight, unit flags, which messages a command raises).
`docs/state.md` ("Mixed cases") splits each into the simulation's perspective, fixed by the match setup, and the view's
player; #1117 makes `view` and `control` events. After the split, the view's two fields come from the view's players,
and the simulation's perspective is not in the view: the view holds only what is drawn for the viewer and commanded by
the controller. While `view` has made them differ, 3.1c draws the viewer's units with the controller's input, so the
viewer's own units cannot be selected: `VIEW_RELATION_CONTROLLED` is clear on them.

## The world

| Field | Source | Visibility |
|---|---|---|
| `tick` | the tick counter, `game.ticks` (the u32 that every phase row hashes first) | all |
| `mapWidth`, `mapHeight` | the map's size in cells, the u32 at the game record's `width` and `height` (section 8.2) | all |
| `gridWidth`, `gridHeight` | `players[viewer].exploredWidth` and `exploredHeight` | all |
| `seaLevel` | `Game.seaLevel`, set when the map loads and changed by the map's events (section 8.28); no row hashes it | all |
| `windX`, `windZ` | `wind.windX`, `wind.windZ` (section 7.6) | all |
| `gravity` | `Game.gravity`, fixed for the match; no row hashes it | all |
| `sightOptions` | bits 0 to 2 of `game.mapFlags`: mapping off, line of sight on, true line of sight | all |
| `endFlags` | bits 2 (the match is over), 5 (won) and 6 (lost) of `game.endGameFlags` | all |
| `gameType` | `mission.type` (section 8.20) | all |
| `commanderDeath`, `maxUnits` | `Game.commanderDeath` and `Game.maxUnitsSetting`, options of the match setup that no row hashes | all |

Smoke and timed plumes drift by the wind and gravity of the tick they step in; no other particle reads them.

## Sight

`sight` holds, for each cell of the half-resolution grid (32 by 32 pixels), what the viewer knows of it:

- `VIEW_SIGHT_HIDDEN` when bit `viewer` of the cell's word in the visibility mask (`visibility[k]`, section 8.3) is clear;
- else `VIEW_SIGHT_IN_SIGHT` when the viewer's explored grid (`players[viewer].explored[k]`, section 8.4, a count of the
  viewer's eyes on the cell) is not 0;
- else `VIEW_SIGHT_MAPPED`.

With mapping off the mask starts all set, and with line of sight off the explored grid holds 1 everywhere, so nothing is
hidden. The fog drawn over the map follows from `sight` and `sightOptions`: black over a hidden cell, grey over a
mapped cell when line of sight is on. The radar picture's cells (`SIM_EVENT_RADAR_MAPPED`) are the same three values.

Allies do not share sight in 3.1c. An ally's mapping reaches the viewer only as bits of the visibility mask that the
simulation copies when the sharing of mapping is on.

## Units

A unit is in `units` when it is live (`units[i].unitDefIndex` not 0) and one of these holds:

- its owner is the viewer;
- it is not cloaked (bit 4 of `units[i].activateFlags` clear), it is not under water unless sonar has it (the top of
  its model, its `y` plus the definition's highest model point, is at or above the sea level, or bit 0x200 of
  `units[i].flags` is set), and one of the four corners of its model's box (its position plus the definition's lowest and highest model points) is in sight.

An ally's units pass by the second rule only: 3.1c draws an ally's unit only in the viewer's own sight, and a cloaked
ally's unit not at all. Radar does not put a unit in `units`; it makes a blip.

| Field | Source | Visibility |
|---|---|---|
| `id`, `unitDefIndex`, `owner` | `units[i].id`, `unitDefIndex`, `playerIndex` | every unit in the table |
| `relation` | the owner against `viewer`, `controller` and `players[viewer].allied`; `VIEW_RELATION_SELECTABLE` when bit 0x20 of `units[i].flags` is set, `buildLeft` is 0, `postTransferHoldoff` is 0 and the unit is carried by none or by a carrier with bit 0x40000000 set | every unit |
| `x`, `y`, `z`, `heading`, `pitch` | `units[i].pos.x`, `pos.y`, `pos.z`, `heading`, `pitch` | every unit |
| `flags` | `units[i].flags` with bit 0x10 (the selection) and the build menu's page bits cleared; for a unit not own, only bits 0x200 (sonar has it) and 0x10000000 (alive) | as said |
| `activateFlags`, `immobile` | `units[i].activateFlags`, and the unit's mobility | every unit |
| `ground` | 1 when the unit's mode is 1: it draws in the first pass, under projectiles | every unit |
| `health` | `units[i].health` | every unit; the bar of a unit not the viewer's is drawn only when its definition allows it |
| `buildLeft` | `units[i].buildLeft` | every unit |
| `carrier` | the unit's carrier; a carried unit is drawn with it | every unit |
| `recentlyDamaged` | `units[i].recentlyDamagedTimer` | every unit |
| `group` | `units[i].group`, the control group, drawn as a digit under the unit | the viewer's own units; 0 for the rest |
| `killCount`, `weaponBuild`, `orderKind`, `orderTarget`, `moveOrder`, `fireOrder`, `cloakOrder`, `metalMake`, `metalUse`, `energyMake`, `energyUse` | `units[i].killCount`; the stockpile progress of its weapon slots; its first order (section 8.9) and its target; its standing orders; its resources (section 8.16) | the viewer's own units; 0 for the rest |
| `firstOrder`, `orderCount` | its orders (section 8.9) | the controller's units; 0 for the rest |
| `firstPiece`, `pieceCount` | the posed pieces of its model, from the scripts' state (section 8.12) | every unit |

The unit panel shows a unit under the cursor that is not in `units`, a blip, as "unidentified object" with nothing else.
A unit's target in the panel is named only when the target is itself in `units`.

**Control groups.** `group` is the group a unit of the viewer's was put in, 1 to 9, and 0 for none. The view gives it
only for the viewer's own units, which are the only ones whose group 3.1c draws. Putting units in a group is a command
of the controller (`docs/commands.md`).

### Pieces

`pieces` holds, for each unit of `units`, the pose of each piece of its model, in the model's piece order: the offset
from the parent and the three rotations the unit's script has left it in, and whether the piece is shown. The model's
shape is loaded data.

## Blips

`blips` holds the units the radar picture shows, from `SIM_EVENT_RADAR_UNITS` of the tick: each live unit when mapping
and line of sight are both off (or the match shows every unit on radar), else each unit with bit 0x100 (radar, or sight
above water) or 0x200 (sonar) of `units[i].flags` set, and each of the viewer's own units. A blip gives the position and
the owner, whose colour it is drawn in, and never the unit's type. 3.1c's quirk: it gives the owner of every blip, an
unseen enemy's included.

The simulation decides the radar bits; jamming clears them, and an ally's jammer jams too.

## Orders

`orders` holds the queued orders of the controller's units, each unit's in the order they run (section 8.9): the kind,
the unit to build, the target and the place. The overlays of the selected units (build footprints, paths, range rings,
weapon coverage) draw from them. A target not in `units` is drawn at its last seen place, which presentation keeps.

## Features

A feature is in `features` unless its definition has `nodrawundergray` (bit 8 of its high flags; 3.1c also forces it
for the dragon's teeth and fortification walls) and the cell's owner is not the viewer and no corner of its footprint is
in sight. Every other feature, tree, rock and wreck, is drawn whatever the viewer has seen, under the fog: black fog hides
it, grey fog does not. #1 names this quirk `ui.map-features-ignore-los`; no flag has it yet.

| Field | Source | Visibility |
|---|---|---|
| `cellX`, `cellZ`, `featureIndex` | the anchor cell of the feature and the feature of its cell (`cells[i]`, section 8.2) | by the rule above |
| `frame`, `shadowFrame` | the frames of its spot's animation (`features[i].anim`, `shadow`, section 7.5) | the same |
| `model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` | a spot drawn as a model: `features[i].pos` and `rot` | the same |

The panel names a feature under the cursor, and its metal and energy, whatever the viewer's sight: 3.1c's quirk, and
the panel reads them from the feature definition, loaded data.

## Projectiles

A projectile is in `projectiles` when its slot is in use and its position is in sight. A shot of the viewer's own is
not exempt, in the battle view; on the radar picture it is (`SIM_EVENT_RADAR_UNITS`).

| Field | Source |
|---|---|
| `weaponIndex`, `owner`, `x`, `y`, `z` | `projectiles[i].type`, `owner`, `position` |
| `startX`, `startY`, `startZ`, `rotX`, `rotY`, `rotZ`, `propellerSpin`, `groundY`, `spawnTick`, `time` | the projectile's start, angles, spin, ground height, spawn tick and age, which weapons reports in record 48 (section 8.31) |

## Explosions and debris

`debris` holds the debris and explosion records and `explodedPieces` the pieces blown off units, from weapons' report
(record 48, section 8.31: `weaponsState.debris[d]`, `weaponsState.explodedPieces[slot]`, `weaponsState.pieces[i]`).
3.1c tests no sight for them: each is drawn anywhere on screen, and only the fog over its cell hides it. The faces a
piece is drawn from are those of the unit's model, loaded data.

## Players, economy and score

`players` has a row for each of the ten slots (`players[i]`, section 7.4).

| Field | Source | Visibility |
|---|---|---|
| `active`, `type`, `side`, `color`, `watching` | `players[i].active`, `type`, and the side, colour and watch flag of the player's setup | all |
| `allied` | `players[viewer].allied[i]` | all |
| `kills`, `losses`, `commanderKills`, `commanderLosses`, `unitsCreated`, `unitCount`, `rank` | `players[i]` of the same name | all: the score panel shows every player still in the match |
| `metal`, `energy`, their capacities, incomes and usages, `shareMetal`, `shareEnergy` | `players[i]` of the same name | the viewer's row only; 0 in the others |
| `displayTimer` | `players[i].displayTimer`: the income and usage readout refreshes when it changes | the viewer's row only |
| `totalMetalProduced`, `totalEnergyProduced`, `metalWasted`, `energyWasted` | `players[i]` of the same name | all, for the end-game screen |

The score panel shows only in skirmish and multiplayer, and counts commanders' kills when `commanderDeath` is 2. 3.1c
shows no ally's resources: the share dialog lists the players to give to, by name only.

Two quirks of 3.1c are the simulation's to fix, not the view's: the readout of each resource bar is fed from the other
resource's fields, as the source does it, and `players[i].rank` is compacted while the score panel draws (#1113).

## Messages

`messages` holds the sim events of the tick that reach the viewer, in the order they were raised, as records of
`docs/sim-events.md` without the tick. They are the kinds whose delivery depends on the player:

| Kind | Reaches the viewer when |
|---|---|
| `SIM_EVENT_SOUND_AT`, `SIM_EVENT_SOUND_AT_NAME` | bit `viewer` of the set of players who hear it is set |
| `SIM_EVENT_UNIT_SPEECH` | the unit is alive and the viewer's, and, with `SIM_EVENT_SPEECH_IF_NOT_VISIBLE`, it is off screen |
| `SIM_EVENT_BUILD_QUEUE_SOUND` | its player is the viewer |
| `SIM_EVENT_CD_ACTIVITY` | either player is the controller |
| `SIM_EVENT_WATCH_PROMPT`, `SIM_EVENT_WATCH_MODE_NOTICE` | its player is the controller |
| `SIM_EVENT_MESSAGE`, `SIM_EVENT_SOUND`, `SIM_EVENT_SCORE_FLASH`, `SIM_EVENT_FORCES_DESTROYED`, `SIM_EVENT_PLAYER_TOOK_LEAD`, `SIM_EVENT_PLAYER_LEFT` | always |

The particle, shake, radar and interface kinds stay in the sim event queue: they carry no player, and what is drawn of
them follows from `sight` (a particle is drawn when its position is in sight, a timed plume always) or applies to every
player (the screen shakes for every impact, wherever it is). The message log itself, its chat and its ageing, is
presentation state.

## What is not sim

Presentation reads these and owns them; they are presentation state, not facts of the view:

- the selection: bit 0x10 of `Unit.flags` and the selection state bits, taken out of the judged state by #1050;
- the build menu's page bits of `Unit.flags` (#1115);
- the camera, its position and the screen shake's offset;
- the unit and the feature under the cursor (the hover unit), and the hot spots of the radar picture's blips;
- the order mode the cursor is in, and the build placement's height;
- `interfaceType`, and the view options: the clock, the shake switch, the selection boxes, the health bars and the
  debug bits 0 and 1 of `endGameFlags` (#1050);
- the pause request and the lag marker, which are the driver's;
- the message log, the speech queue, the particle lists, the fog tiles and the score panel's flashes;
- the last place a target was seen, for the weapon coverage overlay.

## Loaded data

Presentation also reads the game data the simulation loaded. None of it comes through the view; the view names an entry
by its index, and presentation reads the entry through the files service (`interfaces/files.h`), from the game's files:

- the unit definitions (`unitDefIndex`): names, descriptions, costs, health, ranges, sizes, model bounds, build menus,
  sound categories and flags;
- the weapon definitions (`weaponIndex`): the kind of shot, colours, models, lifetimes, ranges and areas of effect;
- the feature definitions (`featureIndex`): footprints, heights, animations, shadows, flags, metal and energy;
- the side definitions (`ViewPlayer.side`): panel layout, fonts and bar colours;
- the models (3DO), the animations (GAF), the palettes, the sounds and the map's tiles.

A match can change a definition only through the match setup (the mission's unit restrictions), which presentation
reads with the setup, not the view.

## The reads

Every sim field (class `sim` or `mixed` in the state classes) that a function of presentation reads, by the access log
of the reference scenarios or by its static read set, with where it is in the view or why it is not. The list is
`data/view_reads.csv`, and `tools/presreads.py --check` fails when a read is missing here, or listed here and no longer
read. A read is left out when no display, sound or command depends on it during play: it is read only by the debug
overlays (which need the debug bits), only by the menus and setup screens, or only by a cache presentation owns.

| Read | In the view |
|---|---|
| `AiSearchGoal::active` | Left out: only the debug overlays draw it, which need the debug bits of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `AiSearchGoal::count` | Left out: only the debug overlays draw it, which need the debug bits of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `BroadcastToggleFlags::b0` | Left out: a key's toggle bit, presentation state of the input ("What is not sim"); no output of the tick depends on it. |
| `BuildMenuPage::count` | Loaded data: the build menus of the unit definitions, through the files service. |
| `BuildMenuPage::entries` | Loaded data: the build menus of the unit definitions, through the files service. |
| `Debris::obj` | `PlayerView.debris` (Explosions and debris). |
| `Debris::pos` | `PlayerView.debris` (Explosions and debris). |
| `Debris::ref1` | `PlayerView.debris` (Explosions and debris). |
| `Debris::ref2` | `PlayerView.debris` (Explosions and debris). |
| `the debris model cache` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `DebrisPiece::faceCount` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `DebrisPiece::faces` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `DebrisPiece::firstFace` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `DebrisPiece::vertexCount` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `DebrisPiece::verts` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `Feature::anim` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::animTable` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::drawn` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::energy` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::flags16` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::flagsHigh` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::flipAnim` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::flipShadow` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::footprint` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::footprintX` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::footprintZ` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::height` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::metal` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::over` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `Feature::shadowTable` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `FeatureAnimEntry::bytes_10` | `ViewFeature.frame`, `shadowFrame` and the model fields (Features). |
| `FeatureAnimEntry::bytes_8` | `ViewFeature.frame`, `shadowFrame` and the model fields (Features). |
| `FeatureAnimEntry::src` | `ViewFeature.frame`, `shadowFrame` and the model fields (Features). |
| `FeatureAnimEntry::state` | `ViewFeature.frame`, `shadowFrame` and the model fields (Features). |
| `FeaturePseudoUnit::def` | `ViewFeature` model fields (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::flags_110` | `ViewFeature` model fields (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::flags_114` | `ViewFeature` model fields (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::unitDefIndex` | `ViewFeature` model fields (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::unknown_0` | `ViewFeature` model fields (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::unknown_96` | `ViewFeature` model fields (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::unknown_a8` | `ViewFeature` model fields (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FogCell::level0` | `PlayerView.sight` (Sight): the fog drawn over a cell follows from it. |
| `FogCell::level1` | `PlayerView.sight` (Sight): the fog drawn over a cell follows from it. |
| `InfoModeFlags::b0` | `PlayerView.endFlags` for bits 2, 5 and 6; bits 0 and 1 are presentation state ("What is not sim"). |
| `InfoModeFlags::b1` | `PlayerView.endFlags` for bits 2, 5 and 6; bits 0 and 1 are presentation state ("What is not sim"). |
| `InfoModeFlags::raw` | `PlayerView.endFlags` for bits 2, 5 and 6; bits 0 and 1 are presentation state ("What is not sim"). |
| `the player slot block` | `PlayerView.sight` (Sight). |
| `the mapped memory` | `PlayerView.sight` (Sight). |
| `the plot memory` | `PlayerView.features` (the features of the cells) and `mapWidth`, `mapHeight`; a cell's height is shown only by the debug strip. |
| `MapViewFlags::bit0` | `PlayerView.sightOptions` (Sight); bit 3, the fog tiles' cache, is presentation state. |
| `MapViewFlags::bit1` | `PlayerView.sightOptions` (Sight); bit 3, the fog tiles' cache, is presentation state. |
| `MapViewFlags::bit2` | `PlayerView.sightOptions` (Sight); bit 3, the fog tiles' cache, is presentation state. |
| `MapViewFlags::raw` | `PlayerView.sightOptions` (Sight); bit 3, the fog tiles' cache, is presentation state. |
| `ModelState::bitmap` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::cacheDrawCount` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::count` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::owner` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::pieces` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::pos` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::root` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::shadow` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `Order::created` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::flags_42` | Left out: presentation's own cache of a target's last seen place, written and read only by the weapon coverage overlay; nothing in the tick reads it. |
| `Order::kind` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::next` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::pos` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::target` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::type` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::unit` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `PieceFrame::always0` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `PieceFrame::count` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `PieceFrame::flags` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `PieceFrame::indices` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `PieceFrame::ptr_10` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `PieceFrame::unknown_0` | `ViewDebris.firstFace`, `faceCount` and `ViewExplodedPiece`: the model faces a debris or exploded piece is drawn from; the faces themselves are loaded data (the unit's model, through the files service). |
| `Player::active` | `ViewPlayer.active`, `type` (Players). |
| `Player::alliance` | `ViewPlayer.allied` (Players). |
| `Player::allied` | `ViewPlayer.allied` (Players). |
| `Player::alliedBy` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::commanderKills` | `ViewPlayer` of the same name (Score). |
| `Player::commanderLosses` | `ViewPlayer` of the same name (Score). |
| `Player::displayTimer` | `ViewPlayer.displayTimer` (Economy). |
| `Player::energy` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::energyCapacity` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::energyIncome` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::energyUsage` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::energyWasted` | `ViewPlayer` of the same name (Score: the end-game screen). |
| `Player::explored` | `PlayerView.sight` (Sight). |
| `Player::exploredHeight` | `PlayerView.sight` (Sight). |
| `Player::exploredWidth` | `PlayerView.sight` (Sight). |
| `Player::firstIndex` | `ViewUnit.relation` (Units): the controller's units are the ones with `VIEW_RELATION_CONTROLLED`. |
| `Player::id` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::index` | `ViewPlayer.active`, `type` (Players). |
| `Player::info` | `ViewPlayer.side`, `color`, `watching` (Players). |
| `Player::joinTime` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::kills` | `ViewPlayer` of the same name (Score). |
| `Player::lastIndex` | `ViewUnit.relation` (Units): the controller's units are the ones with `VIEW_RELATION_CONTROLLED`. |
| `Player::losses` | `ViewPlayer` of the same name (Score). |
| `Player::messages` | Left out: only the debug overlays draw it, which need the debug bits of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `Player::metal` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::metalCapacity` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::metalIncome` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::metalUsage` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::metalWasted` | `ViewPlayer` of the same name (Score: the end-game screen). |
| `Player::ping` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::rank` | `ViewPlayer` of the same name (Score). |
| `Player::rejectReason` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::shareEnergy` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::shareMetal` | `ViewPlayer` of the same name, the viewer's only (Economy). |
| `Player::totalEnergyProduced` | `ViewPlayer` of the same name (Score: the end-game screen). |
| `Player::totalMetalProduced` | `ViewPlayer` of the same name (Score: the end-game screen). |
| `Player::type` | `ViewPlayer.active`, `type` (Players). |
| `Player::unitCount` | `ViewPlayer` of the same name (Score). |
| `Player::unitsBegin` | `ViewUnit.relation` (Units): the controller's units are the ones with `VIEW_RELATION_CONTROLLED`. |
| `Player::unitsCreated` | `ViewPlayer` of the same name (Score). |
| `Player::unitsEnd` | `ViewUnit.relation` (Units): the controller's units are the ones with `VIEW_RELATION_CONTROLLED`. |
| `PlayerInfoEntry::info` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Pool::entries` | `ViewFeature.frame`, `shadowFrame` and the model fields (Features). |
| `Projectile::active` | `PlayerView.projectiles` (Projectiles). |
| `Projectile::angles` | `PlayerView.projectiles` (Projectiles). |
| `Projectile::groundHeightAvg` | `ViewProjectile.groundY` (Projectiles): the two names are the same two bytes. |
| `Projectile::position` | `PlayerView.projectiles` (Projectiles). |
| `Projectile::propellerSpin` | `PlayerView.projectiles` (Projectiles). |
| `Projectile::radius` | `ViewProjectile.groundY` (Projectiles): the two names are the same two bytes. |
| `Projectile::spawnTick` | `PlayerView.projectiles` (Projectiles). |
| `Projectile::start` | `PlayerView.projectiles` (Projectiles). |
| `Projectile::time` | `PlayerView.projectiles` (Projectiles). |
| `Projectile::type` | `PlayerView.projectiles` (Projectiles). |
| `SideDef::energyColor` | Loaded data: the side definitions, indexed by `ViewPlayer.side`, through the files service. |
| `SideDef::font` | Loaded data: the side definitions, indexed by `ViewPlayer.side`, through the files service. |
| `SideDef::metalColor` | Loaded data: the side definitions, indexed by `ViewPlayer.side`, through the files service. |
| `UiOptionFlags::clock` | Presentation state ("What is not sim"). |
| `Unit::activateFlags` | `ViewUnit` of the same name (Units). |
| `Unit::buildLeft` | `ViewUnit` of the same name (Units). |
| `Unit::cargo` | `ViewUnit.carrier` (Units). |
| `Unit::cargoNext` | `ViewUnit.carrier` (Units). |
| `Unit::cargoSelectable` | `ViewUnit.relation`, bit `VIEW_RELATION_SELECTABLE` (Units). |
| `Unit::carrier` | `ViewUnit.carrier` (Units). |
| `Unit::cloakOn` | `ViewUnit.cloakOrder` (Units). |
| `Unit::def` | `ViewUnit.unitDefIndex` (Units). |
| `Unit::f2c` | `ViewUnit.weaponBuild` (Units), with the weapon definitions as loaded data. |
| `Unit::f48` | `ViewUnit.weaponBuild` (Units), with the weapon definitions as loaded data. |
| `Unit::f_1f` | Left out: only the debug overlays draw it, which need the debug bits of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `Unit::f_3b` | Left out: only the debug overlays draw it, which need the debug bits of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `Unit::f_57` | `ViewUnit.weaponBuild` (Units), with the weapon definitions as loaded data. |
| `Unit::fireOrder` | `ViewUnit` of the same name (Units). |
| `Unit::flags` | `ViewUnit.flags` (Units), cut by its rule; bit 0x10 and the build menu's page bits are presentation state. |
| `Unit::group` | `ViewUnit` of the same name (Units). |
| `Unit::health` | `ViewUnit` of the same name (Units). |
| `Unit::id` | `ViewUnit.id` (Units). |
| `Unit::immobile` | `ViewUnit` of the same name (Units). |
| `Unit::killCount` | `ViewUnit` of the same name (Units). |
| `Unit::list` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Unit::list2` | Left out: only the debug overlays draw it, which need the debug bits of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `Unit::mode` | `ViewUnit.ground` (Units). |
| `Unit::motion` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `Unit::moveOrder` | `ViewUnit` of the same name (Units). |
| `Unit::player` | `ViewUnit.owner` (Units). |
| `Unit::playerIndex` | `ViewUnit.owner` (Units). |
| `Unit::pos` | `ViewUnit.x`, `y`, `z` (Units). |
| `Unit::postTransferHoldoff` | `ViewUnit.relation`, bit `VIEW_RELATION_SELECTABLE` (Units). |
| `Unit::script` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `Unit::selectState` | `ViewUnit.relation`, bit `VIEW_RELATION_SELECTABLE` (Units). |
| `Unit::selectable` | `ViewUnit.relation`, bit `VIEW_RELATION_SELECTABLE` (Units). |
| `Unit::selected` | Presentation state: the selection ("What is not sim"). |
| `Unit::speed` | `ViewUnit.buildLeft` (Units): the same four bytes, read as the build progress. |
| `Unit::state` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `Unit::unitDefIndex` | `ViewUnit.unitDefIndex` (Units). |
| `Unit::unknown_4c` | `ViewUnit.weaponBuild` (Units), with the weapon definitions as loaded data. |
| `Unit::weapon` | `ViewUnit.weaponBuild` (Units), with the weapon definitions as loaded data. |
| `Unit::xWhole` | `ViewUnit.x`, `y`, `z` (Units). |
| `Unit::yFraction` | `ViewUnit.x`, `y`, `z` (Units). |
| `Unit::yWhole` | `ViewUnit.x`, `y`, `z` (Units). |
| `Unit::zBufferFlag` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `Unit::zFraction` | `ViewUnit.x`, `y`, `z` (Units). |
| `Unit::zWhole` | `ViewUnit.x`, `y`, `z` (Units). |
| `UnitDef::attackrunlength` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::buildMenuPageCount` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::buildRange` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::description` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::energyCost` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::explodeas` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::f14a` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::fbiChecksum` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `UnitDef::flags1` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::flags2` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::footprintz` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::id` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::ids` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::kamikazeUnsigned` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::maneuverleashUnsigned` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::maxHealth` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::metalCost` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::mincloakdistance` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::mobile` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelMaxX` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelMaxYHigh` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelMaxZ` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelMinX` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelMinY` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelMinZ` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelSizeX` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelSizeXHigh` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelSizeY` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::modelSizeZ` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::movementclass` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::name` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::norestrict` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `UnitDef::origin` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::radardistance` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::radardistancejam` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::range` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::sonardistance` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::sonardistancejam` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitDef::soundcategory` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `UnitMotion::speed` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `UnitTypeSet::bits` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `WeaponDef::areaOfEffect` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `WeaponDef::colour` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `WeaponDef::colour2` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `WeaponDef::coverage` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `WeaponDef::flags` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `WeaponDef::lifetime` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `WeaponDef::model` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `WeaponDef::range` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `WeaponDef::reloadTime` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `WeaponDef::shotKind` | Loaded data: the weapon definition named by `ViewProjectile.weaponIndex`, through the files service. |
| `bits` | `PlayerView.endFlags` for bits 2, 5 and 6; bits 0 and 1 are presentation state ("What is not sim"). |
| `bits_14281` | `PlayerView.sightOptions` (Sight); bit 3, the fog tiles' cache, is presentation state. |
| `bits_2a44` | Left out: network state, class platform in part: it picks the music's mood and the network lines, neither of which the display log, the sound log or a command records. |
| `bits_37f2f` | Presentation state ("What is not sim"). |
| `bits_38a51` | Presentation state: the driver's pause request and the lag marker ("What is not sim"). |
| `cells` | `PlayerView.features` (the features of the cells) and `mapWidth`, `mapHeight`; a cell's height is shown only by the debug strip. |
| `commanderDeath` | `PlayerView.commanderDeath` (Score). |
| `count_1491b` | `PlayerView.debris` (Explosions and debris). |
| `defs` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `difficulty` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `endGameFlags` | `PlayerView.endFlags` for bits 2, 5 and 6; bits 0 and 1 are presentation state ("What is not sim"). |
| `features` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `flagsByte_3923b` | `PlayerView.endFlags` for bits 2, 5 and 6; bits 0 and 1 are presentation state ("What is not sim"). |
| `flags_2a44` | Left out: network state, class platform in part: it picks the music's mood and the network lines, neither of which the display log, the sound log or a command records. |
| `flags_38d75` | Left out: network state, class platform in part: it picks the music's mood and the network lines, neither of which the display log, the sound log or a command records. |
| `g_emptyText` | Left out: a constant, the empty string. |
| `g_explodedPieces` | `PlayerView.explodedPieces` (Explosions and debris). |
| `g_lastPlaceHeight` | Presentation state ("What is not sim"). |
| `g_missionOrderTableVec` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `g_missionOrderTableVec, second copy` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `g_quirks` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `gravity` | `PlayerView.gravity` (The world). |
| `height` | `PlayerView.mapWidth`, `mapHeight` (The world). |
| `loaded` | Left out: network state, class platform in part: it picks the music's mood and the network lines, neither of which the display log, the sound log or a command records. |
| `localPlayer` | `PlayerView.controller` (The two players). |
| `mapFlags` | `PlayerView.sightOptions` (Sight); bit 3, the fog tiles' cache, is presentation state. |
| `mapHeight` | `PlayerView.mapWidth`, `mapHeight` (The world). |
| `mapWidth` | `PlayerView.mapWidth`, `mapHeight` (The world). |
| `maxUnitsSetting` | `PlayerView.maxUnits` (Score). |
| `netGameFlags` | Left out: network state, class platform in part: it picks the music's mood and the network lines, neither of which the display log, the sound log or a command records. |
| `numPlayers` | `ViewPlayer.active`, `type` (Players). |
| `otherFlags` | Presentation state: the driver's pause request and the lag marker ("What is not sim"). |
| `p1b8a` | `PlayerView.players` (Players): unnamed ranges of the player table. |
| `p1cd5` | `PlayerView.players` (Players): unnamed ranges of the player table. |
| `pad_14273` | `PlayerView.sight` (Sight). |
| `pad_1df4` | `PlayerView.players` (Players): unnamed ranges of the player table. |
| `pad_3816f` | Loaded data: the side definitions, indexed by `ViewPlayer.side`, through the files service. |
| `pauseFlags` | Presentation state: the driver's pause request and the lag marker ("What is not sim"). |
| `playerIndex` | `PlayerView.viewer` (The two players). |
| `projectileCount` | `PlayerView.projectiles` (Projectiles). |
| `projectiles` | `PlayerView.projectiles` (Projectiles). |
| `seaLevel` | `PlayerView.seaLevel` (The world). |
| `sides` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `spots` | `ViewFeature.frame`, `shadowFrame` and the model fields (Features). |
| `startPos` | Loaded data: the side definitions, indexed by `ViewPlayer.side`, through the files service. |
| `ticks` | `PlayerView.tick` (The world). |
| `types` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `uiOptionFlags` | Presentation state ("What is not sim"). |
| `uiOptionFlags_37f2f_1` | Presentation state ("What is not sim"). |
| `unit` | `ViewFeature` model fields (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `unitDefCount` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `units` | `PlayerView.units` (Units). |
| `unitsEnd` | `PlayerView.units` (Units). |
| `unknown_1b8e` | `PlayerView.players` (Players): unnamed ranges of the player table. |
| `unknown_1cd9` | `PlayerView.players` (Players): unnamed ranges of the player table. |
| `visibilityMask` | `PlayerView.sight` (Sight). |
| `width` | `PlayerView.mapWidth`, `mapHeight` (The world). |
| `windX` | `PlayerView.windX`, `windZ` (The world). |
| `windZ` | `PlayerView.windX`, `windZ` (The world). |
