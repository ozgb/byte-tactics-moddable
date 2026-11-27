# The view

The view is what a presentation (drawing, the interface, sound and music) reads of the simulation, for one player. It is
a flat block, `PlayerView` of `interfaces/view.h`, with fixed-width fields and tables of fixed capacity, and no pointer
into the state block. The block is filled once after each tick, and the rule of visibility of each fact is applied
then: a presentation that reads only the block sees what 3.1c lets that player see, and nothing else. The same block
serves the local renderer, a network client that never holds the whole state, and a UI mod.

This page states each fact of the block: its source in the judged state (`docs/trace-format.md`, by the dump path of
the field, such as `units[i].health`) or in the sim events (`docs/sim-events.md`), and its rule of visibility. The rules
are 3.1c's, quirks included; a quirk is a fact of the view until a quirk flag (#1) gives a profile the choice. A test-mode
run fills the block at the end of every tick and records it: as the `view` row of the trace on every tick, and as a whole
file on the ticks with a stored frame ("The recorded view").

The facts are what presentation reads: the last section, "The reads", lists every field of the simulation that a
function of presentation reads during play or in its menus, and says where each one is in the view, or why it is not.

## Words

- **Viewer:** the player whose sight, economy and units the view holds, `PlayerView.viewer`.
- **Controller:** the player whose units the machine's input commands, `PlayerView.controller`. It is the viewer,
  unless the console's `view` command has made the viewer another player ("The two players").
- **In sight:** a point is in the viewer's sight when the cell of the sight grid under it says so (`VIEW_SIGHT_IN_SIGHT`)
  with line of sight on, or when the cell is revealed (`VIEW_SIGHT_MAPPED` or in sight) with line of sight off. This is
  3.1c's one test, repeated by every draw that tests sight. The cell of a point is taken on the whole parts of its 16.16
  world coordinates (the signed 16-bit values `xw`, `yw`, `zw`): column `xw >> 5` and row `(zw - (yw >> 1)) >> 5`, with
  arithmetic shifts. A point whose column or row is outside the grid is not in sight.
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

The simulation also reads both fields (line of sight, unit flags, which messages a command raises), so both are game
state: `control` and `view` publish PLAYERS_SET_CONTROL and PLAYERS_SET_VIEW, and players writes the fields when it
applies them (`docs/players-interface.md`). While `view` has made them differ, 3.1c draws the viewer's units with the controller's
input, so the viewer's own units cannot be selected: `VIEW_RELATION_CONTROLLED` is clear on them.

## The world

| Field | Source | Visibility |
|---|---|---|
| `tick` | the tick counter, `game.ticks` (the u32 that every phase row hashes first) | all |
| `mapWidth`, `mapHeight` | the map's size in cells, the u32 at the game record's `width` and `height` (section 8.2) | all |
| `gridWidth`, `gridHeight` | `players[viewer].exploredWidth` and `exploredHeight` | all |
| `seaLevel` | `Game.seaLevel`, set when the map loads and changed by the map's events (section 8.28); no row hashes it | all |
| `windX`, `windZ` | `wind.windX`, `wind.windZ` (section 7.6) | all |
| `gravity` | `Game.gravity`, fixed for the match; no row hashes it | all |
| `sightOptions` | masks 0x1, 0x2 and 0x4 of `game.mapFlags`: 0x1 mapping on (the map starts unexplored), 0x2 line of sight on, 0x4 true line of sight | all |
| `endFlags` | masks 0x4 (the match is over), 0x20 (won) and 0x40 (lost) of `game.endGameFlags` | all |
| `gameType` | `mission.type` (section 8.20) | all |
| `commanderDeath`, `maxUnits` | `Game.commanderDeath` and `Game.maxUnitsSetting`, options of the match setup that no row hashes | all |

Smoke and timed plumes drift by the wind and gravity of the tick they step in; no other particle reads them.

## Capacities

Each table of the block has a fixed capacity (`VIEW_MAX_*` in `interfaces/view.h`). Most are the game's own bounds:

| Table | Capacity | Basis |
|---|---|---|
| `players` | 10 | the ten player slots |
| `units`, `blips` | 5001 | ten players of at most 500 units (`maxUnits` is capped at 500), and the pool's spare entry |
| `projectiles`, `debris` | 300 | the simulation's tables of projectiles and of debris |
| `explodedPieces` | 100 | the simulation's table of exploded pieces |
| `messages` | 4096 | the sim event queue of one tick (`docs/sim-events.md`, "The queue") |

The game sets no bound on the others: a model's pieces, a unit's queued orders, a map's features and its size, and the faces
and textures of the models debris is drawn from. `debrisVertices` holds 8 vertices for each of the 300 debris slots and one for
each 12 bytes of the 100000-byte cache the exploded pieces are kept in, more than the game's debris can have at once. Their
capacities are larger than any map and model of the game data, and a fill past one drops entries by a fixed rule and
sets a bit of `PlayerView.truncated`, so that a dropped entry is never silent:

| Table | Capacity | Past it |
|---|---|---|
| `pieces` | 131072 | the first unit in `units` whose pieces do not all fit, and every unit after it, has `pieceCount` 0 and no pieces; `VIEW_TRUNCATED_PIECES` |
| `orders` | 65536 | the same for orders, with `orderCount`; `VIEW_TRUNCATED_ORDERS` |
| `features` | 65536 | the features after the first 65536, in the order of their anchor cells (row by row), are dropped; `VIEW_TRUNCATED_FEATURES` |
| `debrisVertices`, `debrisFaces`, `debrisIndices`, `textures` | 10733, 16384, 65536, 1024 | the first debris row, in `debris` then `explodedPieces` order, whose geometry does not all fit any of the four tables, and every row after it, has no vertices and no faces (`firstVertex`, `vertexCount`, `firstFace` and `faceCount` 0), and nothing it would have added stays in the tables; `VIEW_TRUNCATED_DEBRIS` |
| `sight` | 1048576 cells (a grid of 1024 by 1024, a map of 32768 pixels a side) | `gridWidth` and `gridHeight` stay the grid's; the cells past the first 1048576, row by row, are dropped, and a draw reads them as `VIEW_SIGHT_HIDDEN`; `VIEW_TRUNCATED_SIGHT` |

## The recorded view

A test-mode run (`docs/scenario-format.md`) fills a `PlayerView` at the end of each tick, after every step of the tick and
before the tick's save and round trip, for the viewer and controller the judged state names (section "The two players"),
and records it twice: as the `view` row of `trace.txt` on every tick, and as a file on some ticks. Both are made from one byte
string, the serialisation S. This section defines S, the row and the file exactly enough to compute them from a filled block.

**The layout.** The structs are those of `interfaces/view.h` with the layout of a 32-bit Visual C++ build for x86: every member
at an offset that is a multiple of its size (a `double` at a multiple of 8), a struct as aligned as its most aligned member
and its size rounded up to that alignment, little-endian, `float` and `double` as IEEE 754 binary32 and binary64. The padding bytes
between members and at the end of a struct are zero. The sizes in bytes: `ViewPlayer` 104, `ViewUnit` 84, `ViewPiece` 20,
`ViewOrder` 24, `ViewBlip` 16, `ViewFeature` 32, `ViewProjectile` 48, `ViewDebris` 44, `ViewExplodedPiece` 40,
`ViewDebrisVertex` 12, `ViewDebrisFace` 16, `ViewTexture` 32, `ViewMessage` 28; a debris index is 2 bytes.
The fields of `PlayerView` before `players`, its header, take 104 bytes (`versionMajor` at 0, `tick` at 4, `mapWidth` at 20,
`unitCount` at 52, `debrisVertexCount` at 84, `textureCount` at 96, `messageCount` at 100). `tools/viewblock.py` reads the layout from the header file and decodes any block.

**S.** The serialisation of a filled block is the concatenation of

1. the header: the bytes of `PlayerView` from offset 0 up to the `players` member;
2. `players[0]` to `players[9]`, all ten, in use or not;
3. the used rows of each table, in this order: `units[0..unitCount)`, `pieces[0..pieceCount)`, `orders[0..orderCount)`,
   `blips[0..blipCount)`, `features[0..featureCount)`, `projectiles[0..projectileCount)`, `debris[0..debrisCount)`,
   `explodedPieces[0..explodedPieceCount)`, `debrisVertices[0..debrisVertexCount)`, `debrisFaces[0..debrisFaceCount)`,
   `debrisIndices[0..debrisIndexCount)`, `textures[0..textureCount)`, `messages[0..messageCount)`;
4. `sight[0..n)` with `n` the smaller of `gridWidth * gridHeight` and `VIEW_MAX_GRID_CELLS` (a grid cut at its capacity is
   written cut, "Capacities").

A count is never above its table's capacity ("Capacities"). The tables are written whole, each element as its struct: a field
that is `reserved` is zero and is part of S. S is 1144 bytes (104 and ten players) when no table has a row and the grid is empty, and under 9 million at the capacities.

**The `view` row.** `<tick> view <16 hex>` follows the `tick` row of the same tick, and is the trace hash
(`docs/trace-format.md`, section 3) from the offset basis of S taken as one run: 8-byte little-endian words with the word step
while at least 8 bytes remain, then the byte step for the rest. It has no preamble: no tick number, no random state and no section
number or `END`; `tick` inside the header is the tick of the row. `tools/viewblock.py` has the function (`view_hash`).

**The file.** On the dump tick (`-testdump <tick>`) and on every tick of the scenario's `frames` line the run also writes
`views/<tick>.bin`, beside `trace.txt` (and the frame `frames/<tick>.bin` of `docs/frame-dump.md`, whose tick it shares). All numbers
are little-endian.

| Offset | Size | Content |
| --- | --- | --- |
| 0 | 4 | The bytes `BTVW` |
| 4 | 2 | Version, 1 |
| 6 | 2 | Reserved, 0 |
| 8 | the length of S | S |

The hash of the `view` row of that tick is the hash of the bytes from offset 8 to the end of the file. The reference stores the
views of the frame ticks in `reference/views/<scenario>/<tick>.bin`, which `tools/reference.py --check` compares byte for byte
with a run's, naming the first differing field in serialisation order (`tools/viewblock.py diff A B`).

**What it is for.** A presentation reads only the view, so a presentation is judged on one tick by the view file of that tick in, and the
display list (`docs/display-log.md`) and the frame (`docs/frame-dump.md`) of that tick out. A core that fills the block is judged
on the `view` rows: equal where 3.1c's are.

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
- it is not cloaked (mask 0x4 of `units[i].activateFlags` clear), it is not under water unless sonar has it (the top of
  its model, its `y` plus the definition's highest model point, is at or above the sea level, or bit 0x200 of
  `units[i].flags` is set), and one of four corners of its model's box is in sight. With `lo` the unit's position plus the definition's lowest model
  point and `hi` its position plus the highest, and `size` the model's size, the four are tested in this order:
  (`lo.x`, `hi.y`, `lo.z`), (`lo.x + size.x`, `hi.y`, `lo.z`), (`lo.x + size.x`, `hi.y - size.y`, `lo.z + size.z`) and
  (`lo.x`, `hi.y - size.y`, `lo.z + size.z`).

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
| `stockpile` | `units[i].weapons[0]`'s stockpiled shots (the byte 0x1e into the unit), which the build menu draws beside a stockpiled weapon | the viewer's own units; 0 for the rest |
| `killCount`, `weaponBuild`, `orderKind`, `orderTarget`, `moveOrder`, `fireOrder`, `cloakOrder`, `metalMake`, `metalUse`, `energyMake`, `energyUse` | `units[i].killCount`; the stockpile progress of its weapon slots, `progress * 100 / reload` of the first weapon-build order, clamped to 0 to 255, and 0 when no such order names a slot 0 to 2 with a weapon, a reload of 0 or a progress of 0 or less; its first order (section 8.9) and its target, the target only when bit 0x8 of the order's flags is set, as 3.1c's presentation reads it, and 0 otherwise; its standing orders; its resources (section 8.16) | the viewer's own units; 0 for the rest |
| `firstOrder`, `orderCount` | its orders (section 8.9); `firstOrder` is 0 when the count is 0 | the controller's units; 0 for the rest |
| `firstPiece`, `pieceCount` | the posed pieces of its model, from the scripts' state (section 8.12); `firstPiece` is 0 when the count is 0 | every unit |

The unit panel shows a unit under the cursor that is not in `units`, a blip, as "Unidentified object", with the prefix
"S: " when the blip's `VIEW_BLIP_SONAR` is set and "R: " when it is clear, and nothing else.
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
An order's `y` is 0 when its kind is `MobileBuild`, `VTOL_MobileBuild`, `Reclaim` or `VTOL_Reclaim`: those orders never
write their `pos.y`, which the trace leaves out for the same reason (`docs/trace-format.md`, section 8.9).

## Features

Every feature on the map is in `features`, in the order of the anchor cells, row by row. Its `drawn` is 0 when its
definition has `nodrawundergray` (mask 0x8 of its high flags; 3.1c also forces it for the dragon's teeth and
fortification walls), the cell's owner is not the viewer, and neither of two points, in whole world units, is in sight:
the cell's corner (`16 * cellX`, the cell's height, `16 * cellZ`) and the corner across the footprint (`16 * (cellX +
footprintX)`, the same height, `16 * (cellZ + footprintZ)`, with the footprint of its definition). A feature with
`drawn` 0 gives only its cell and its definition. Every other feature, tree, rock and wreck, has `drawn` 1 and is drawn
whatever the viewer has seen, under the fog: black fog hides it, grey fog does not. #1 names this quirk
`ui.map-features-ignore-los`; no flag has it yet.

| Field | Source | Visibility |
|---|---|---|
| `cellX`, `cellZ`, `featureIndex` | the anchor cell of the feature and the feature of its cell (`cells[i]`, section 8.2) | every feature |
| `drawn` | the rule above | every feature |
| `frame`, `shadowFrame` | the frames of its spot's animation (`features[i].anim`, `shadow`, section 7.5) | `drawn` 1; 0 for the rest |
| `model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` | a spot drawn as a model: `features[i].pos` and `rot` | `drawn` 1; 0 for the rest |

The panel names the feature of the cell under the cursor, and its metal and energy, whatever the viewer's sight, a
feature with `drawn` 0 included: 3.1c's quirk. The cell's entry in `features` gives the definition, and the name, metal and
energy are the definition's, loaded data.

## Projectiles

A projectile is in `projectiles` when it is among the first `projectileCount` entries of the projectile table, its
`active` (the count of shots of its burst still to fire) is 0, and its position is in sight. A shot of the viewer's own is
not exempt, in the battle view; on the radar picture it is (`SIM_EVENT_RADAR_UNITS`).

| Field | Source |
|---|---|
| `weaponIndex`, `owner`, `x`, `y`, `z` | `projectiles[i].type`, `owner`, `position` |
| `startX`, `startY`, `startZ`, `rotX`, `rotY`, `rotZ`, `propellerSpin`, `groundY`, `spawnTick`, `time` | the projectile's start, angles, spin, ground height, spawn tick and age, which weapons reports in record 48 (section 8.31) |

## Explosions and debris

`debris` holds the debris and explosion records and `explodedPieces` the pieces blown off units, from weapons' report
(record 48, section 8.31: `weaponsState.debris[d]`, `weaponsState.explodedPieces[slot]`, `weaponsState.pieces[i]`).
3.1c tests no sight for them: each is drawn anywhere on screen, and only the fog over its cell hides it. Their shapes are
in the geometry tables `debrisVertices`, `debrisFaces`, `debrisIndices` and `textures`, which both tables point into.

`debris` has a row for each entry `d` of the debris table, in the table's order, an explosion with no model included.
The fields of a row come from `weaponsState.debris[d]`:

- `frame` and `shadowFrame` are `ref1.index` and `ref2.index`.
- `x`, `y`, `z` are `pos`, and `rotX`, `rotY`, `rotZ` are `angle_x`, `angle_y`, `angle_z`.
- `sprite` is the index, in the sprite catalogue below, of the animation `ref1.src` names: the first entry equal to it,
  and 0xffff when `ref1.src` is null or equals no entry.
- `shadowSprite` is the index, in the shadow catalogue below, of the first of its three animations equal to `ref2.src`,
  and 0xffff when `ref2.src` is null or equals none.
- The geometry is that of the debris slot `obj` names (`weaponsState.pieces[i]`, one of the 300 slots that start 0x1ab9f
  bytes into the game record, 0x34 bytes each), and none when `obj` is null or names no slot: its `vertexCount`
  vertices (`vertices`, at most 8) and its faces `faces[k]` for `k` from 0 to `faceCount` (at most 6), from 1 when the
  slot's `firstFace` is not -1. A slot's faces are a copy of the broken face, taken when the piece breaks, with a
  team-coloured texture already turned into the owner's frame.

`explodedPieces` has a row for each slot of the exploded-piece table that holds a block, in slot order, from
`weaponsState.explodedPieces[slot]`:

- `owner` is the current owner of the unit slot `unit` names, as 3.1c's draw reads it each frame: the dead unit's
  owner until the slot is reused, then the new unit's. `owner` is 0 when `unit` names no slot.
- `x`, `y`, `z` are the piece record's `pos` and `rotX`, `rotY`, `rotZ` its angles `angle_10`, `angle_12` and `angle_14`.
- `trail` is 1 when bit 0x2 of `flags` is set, else 2 when bit 0x1 is set, else 0.
- The geometry is that of the model piece `rec.desc` names, loaded data that the piece keeps for its flight: the
  piece's vertices as the model file gives them, unrotated (3.1c rotates a copy of them on every frame it draws), and
  its faces from 0, or from 1 when the piece's first face is set, as for a debris slot. A null `rec.desc` gives none.

**The geometry.** A row's vertices are `debrisVertices[firstVertex..firstVertex + vertexCount)`, 16.16 fixed point in
the piece's own frame, which the row's position and rotation place. Its faces are `debrisFaces[firstFace..firstFace +
faceCount)`, in the order 3.1c visits them. A face's vertices are `debrisIndices[firstIndex..firstIndex + indexCount)`,
each an index of the row's vertices counted from `firstVertex`, in the order the model gives them. The rows are filled
in `debris` order, then `explodedPieces` order, each appending its vertices, faces and indices; a row's runs are
contiguous.

A face's `kind` and its texture follow the face's flags, as 3.1c's draw does:

- bit 0x1 set: `VIEW_FACE_FILLED`, a polygon of the face's vertices filled with `colour`, the low byte of the face's
  colour word, a palette index;
- else, a face with other than 4 vertices: `VIEW_FACE_NONE`, which 3.1c does not draw;
- else `VIEW_FACE_TEXTURED`, a quad drawn with frame `frame` of texture `texture`:
  - with bit 0x2 set, the texture is the animation the face's source names. Its frame is the owner's colour index
    (`players[owner]`'s colour) when bit 0x4 is set too and the row is an exploded piece; otherwise it is the low 16 bits
    of the face's frame word.
  - with bit 0x2 clear, the face names a frame by itself. The texture is the first animation that holds that frame,
    searching the model textures' animation files in load order and then the side logos' file, and `frame` is its index
    there. That is the search the game makes when it binds a model's textures.
  - when no texture is found, `texture` is 0xffff.

`textures` names each texture once, in the order of first use: `name` is the animation's name in its GAF file, 32
bytes with zero padding. A presentation finds it the same way, in the model textures and then the logos.

**The sprite catalogue** lists the explosion animations in this order:

| Index | Animation |
| --- | --- |
| 0 | `h2oboom2` of `fx.gaf`, the sea splash |
| 1 | `lavasplash` of `fx.gaf` |
| 2 | `flamestream` of `fx.gaf` |
| 3 | `explosion` of `fx.gaf` |
| 4 to 7 | `explode2`, `explode3`, `explode4` and `explode5` of `fx.gaf` |
| 8 | `nuke1` of `fx.gaf` |
| 9 + 2w | the splash of weapon definition `w`: its `lavaexplosionart` entry of `lavaexplosiongaf` on a lava world, else its `waterexplosionart` of `waterexplosiongaf` |
| 10 + 2w | the explosion of weapon definition `w`: its `explosionart` entry of `explosiongaf` |

These are the nine animation pointers that start 0x147eb bytes into the game record, then each weapon definition's
`splash` and `explosionSeq`. A weapon whose definition has no such entry has a null pointer there, which no debris names.

**The shadow catalogue** holds three animations that the game makes at load rather than loads: round explosion shadows
drawn from the load stream's noise, 12 frames from 64 to 8 pixels across (index 0), 15 frames from 128 to 16 (index 1)
and 15 frames from 200 to 32 (index 2). They are the three animation pointers that start 0x1ab8f bytes into the game
record.

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

On each frame it draws the score panel, 3.1c closes the gaps that defeated players leave in `players[i].rank`: for
each place from 0 below the number of players, when no counted player holds it, every counted player above it moves
down one. A counted player is one the panel lists: active, a human or a computer, not defeated (no units left after
making one) and not only watching. A presentation runs that pass on its own copy of `rank` and draws from the copy;
when the pass changed a place, it publishes `PLAYERS_COMPACT_RANKS` (`docs/players-interface.md`), and the simulation
runs the same pass on its ranks.

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

- the selection: bit 0x10 of `Unit.flags`, taken out of the judged state by #1050, and the selection state bits 6 and
  7 (`selectState`), still hashed until #1132;
- the build menu's page bits of `Unit.flags`, still hashed: unit creation sets them too, and no run from the command
  log differs on them (`docs/state.md`);
- the camera, its position and the screen shake's offset;
- the unit and the feature under the cursor (the hover unit), and the hot spots of the radar picture's blips;
- the order mode the cursor is in, and the build placement's height;
- `interfaceType`, and the view options: the clock, the shake switch, the selection boxes, the health bars and the
  debug masks 0x1 and 0x2 of `endGameFlags` (#1050);
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
of the reference scenarios or by its static read set, with where it is in the view or why it is not. Each name is a
field of the state block layout (`layout.json`), a global of it, or a named block of it. (For contributors to the source
repository: the list is generated there, and a check fails when a read is missing here or a row names no read.) A read
is left out when no display, sound or command depends on it during play: it is read only by the debug overlays (which
need the debug bits), only by the menus and setup screens, or only by a cache presentation owns.

| Read | In the view |
|---|---|
| `AiSearchGoal::active` | Left out: only the debug overlays draw it, which need the debug masks 0x1 and 0x2 of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `AiSearchGoal::count` | Left out: only the debug overlays draw it, which need the debug masks 0x1 and 0x2 of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `BroadcastToggleFlags::b0` | Left out: a key's toggle bit, presentation state of the input ("What is not sim"); no output of the tick depends on it. |
| `BuildMenuPage::count` | Loaded data: the build menus of the unit definitions, through the files service. |
| `BuildMenuPage::entries` | Loaded data: the build menus of the unit definitions, through the files service. |
| `Debris::obj` | `PlayerView.debris` (Explosions and debris). |
| `Debris::pos` | `PlayerView.debris` (Explosions and debris). |
| `Debris::ref1` | `PlayerView.debris` (Explosions and debris). |
| `Debris::ref2` | `PlayerView.debris` (Explosions and debris). |
| `the debris model cache` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `DebrisPiece::faceCount` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `DebrisPiece::faces` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `DebrisPiece::firstFace` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `DebrisPiece::vertexCount` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `DebrisPiece::verts` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
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
| `FeatureAnimEntry::bytes_10` | `ViewFeature.frame`, `shadowFrame` and `ViewFeature.model` (Features). |
| `FeatureAnimEntry::bytes_8` | `ViewFeature.frame`, `shadowFrame` and `ViewFeature.model` (Features). |
| `FeatureAnimEntry::src` | `ViewFeature.frame`, `shadowFrame` and `ViewFeature.model` (Features). |
| `FeatureAnimEntry::state` | `ViewFeature.frame`, `shadowFrame` and `ViewFeature.model` (Features). |
| `FeaturePseudoUnit::def` | `ViewFeature.model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::flags_110` | `ViewFeature.model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::flags_114` | `ViewFeature.model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::unitDefIndex` | `ViewFeature.model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::unknown_0` | `ViewFeature.model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::unknown_96` | `ViewFeature.model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FeaturePseudoUnit::unknown_a8` | `ViewFeature.model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `FogCell::level0` | `PlayerView.sight` (Sight): the fog drawn over a cell follows from it. |
| `FogCell::level1` | `PlayerView.sight` (Sight): the fog drawn over a cell follows from it. |
| `InfoModeFlags::b0` | `PlayerView.endFlags` for masks 0x4, 0x20 and 0x40; masks 0x1 and 0x2 are presentation state ("What is not sim"). |
| `InfoModeFlags::b1` | `PlayerView.endFlags` for masks 0x4, 0x20 and 0x40; masks 0x1 and 0x2 are presentation state ("What is not sim"). |
| `InfoModeFlags::raw` | `PlayerView.endFlags` for masks 0x4, 0x20 and 0x40; masks 0x1 and 0x2 are presentation state ("What is not sim"). |
| `the player slot block` | `PlayerView.sight` (Sight). |
| `the mapped memory` | `PlayerView.sight` (Sight). |
| `the plot memory` | `PlayerView.features` (the features of the cells) and `mapWidth`, `mapHeight`; a cell's height is shown only by the debug strip. |
| `the unit model table` | Loaded data: the model of each unit type, which the battle frame draws a unit with and the cursor hit-tests it against, through the files service. |
| `MapViewFlags::bit0` | `PlayerView.sightOptions` (Sight); mask 0x8, the fog tiles' cache, is presentation state. |
| `MapViewFlags::bit1` | `PlayerView.sightOptions` (Sight); mask 0x8, the fog tiles' cache, is presentation state. |
| `MapViewFlags::bit2` | `PlayerView.sightOptions` (Sight); mask 0x8, the fog tiles' cache, is presentation state. |
| `MapViewFlags::raw` | `PlayerView.sightOptions` (Sight); mask 0x8, the fog tiles' cache, is presentation state. |
| `ModelState::bitmap` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::cacheDrawCount` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::count` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::owner` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::pieces` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::pos` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::root` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `ModelState::shadow` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `Order::created` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::flags_42` | `ViewUnit.orderTarget` through bit 0x8 and `ViewUnit.weaponBuild` through bit 0x80000 (Units); the weapon coverage overlay's cache of a target's last seen place in it is presentation's own. |
| `Order::kind` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::next` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::pos` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::target` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::type` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Order::unit` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `PieceFrame::always0` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `PieceFrame::count` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `PieceFrame::flags` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `PieceFrame::indices` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `PieceFrame::ptr_10` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `PieceFrame::fillColor` | `PlayerView.debrisVertices`, `debrisFaces`, `debrisIndices` and `textures` (Explosions and debris): the geometry a debris or exploded piece is drawn from. |
| `Player::active` | `ViewPlayer.active`, `type` (Players). |
| `Player::alliance` | `ViewPlayer.allied` (Players). |
| `Player::allied` | `ViewPlayer.allied` (Players). |
| `Player::alliedBy` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::commanderKills` | `ViewPlayer.commanderKills` (Score). |
| `Player::commanderLosses` | `ViewPlayer.commanderLosses` (Score). |
| `Player::displayTimer` | `ViewPlayer.displayTimer` (Economy). |
| `Player::energy` | `ViewPlayer.energy`, the viewer's only (Economy). |
| `Player::energyCapacity` | `ViewPlayer.energyCapacity`, the viewer's only (Economy). |
| `Player::energyIncome` | `ViewPlayer.energyIncome`, the viewer's only (Economy). |
| `Player::energyUsage` | `ViewPlayer.energyUsage`, the viewer's only (Economy). |
| `Player::energyWasted` | `ViewPlayer.energyWasted` (Score: the end-game screen). |
| `Player::explored` | `PlayerView.sight` (Sight). |
| `Player::exploredHeight` | `PlayerView.sight` (Sight). |
| `Player::exploredWidth` | `PlayerView.sight` (Sight). |
| `Player::firstIndex` | `ViewUnit.relation` (Units): the controller's units are the ones with `VIEW_RELATION_CONTROLLED`. |
| `Player::id` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::index` | `ViewPlayer.active`, `type` (Players). |
| `Player::info` | `ViewPlayer.side`, `color`, `watching` (Players). |
| `Player::joinTime` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::kills` | `ViewPlayer.kills` (Score). |
| `Player::lastIndex` | `ViewUnit.relation` (Units): the controller's units are the ones with `VIEW_RELATION_CONTROLLED`. |
| `Player::losses` | `ViewPlayer.losses` (Score). |
| `Player::messages` | Left out: only the debug overlays draw it, which need the debug masks 0x1 and 0x2 of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `Player::metal` | `ViewPlayer.metal`, the viewer's only (Economy). |
| `Player::metalCapacity` | `ViewPlayer.metalCapacity`, the viewer's only (Economy). |
| `Player::metalIncome` | `ViewPlayer.metalIncome`, the viewer's only (Economy). |
| `Player::metalUsage` | `ViewPlayer.metalUsage`, the viewer's only (Economy). |
| `Player::metalWasted` | `ViewPlayer.metalWasted` (Score: the end-game screen). |
| `Player::ping` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::rank` | `ViewPlayer.rank` (Score). |
| `Player::rejectReason` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Player::shareEnergy` | `ViewPlayer.shareEnergy`, the viewer's only (Economy). |
| `Player::shareMetal` | `ViewPlayer.shareMetal`, the viewer's only (Economy). |
| `Player::totalEnergyProduced` | `ViewPlayer.totalEnergyProduced` (Score: the end-game screen). |
| `Player::totalMetalProduced` | `ViewPlayer.totalMetalProduced` (Score: the end-game screen). |
| `Player::type` | `ViewPlayer.active`, `type` (Players). |
| `Player::unitCount` | `ViewPlayer.unitCount` (Score). |
| `Player::unitsBegin` | `ViewUnit.relation` (Units): the controller's units are the ones with `VIEW_RELATION_CONTROLLED`. |
| `Player::unitsCreated` | `ViewPlayer.unitsCreated` (Score). |
| `Player::unitsEnd` | `ViewUnit.relation` (Units): the controller's units are the ones with `VIEW_RELATION_CONTROLLED`. |
| `PlayerInfoEntry::info` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `Pool::entries` | `ViewFeature.frame`, `shadowFrame` and `ViewFeature.model` (Features). |
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
| `Unit::activateFlags` | `ViewUnit.activateFlags` (Units). |
| `Unit::buildLeft` | `ViewUnit.buildLeft` (Units). |
| `Unit::cargo` | `ViewUnit.carrier` (Units). |
| `Unit::cargoNext` | `ViewUnit.carrier` (Units). |
| `Unit::cargoSelectable` | `ViewUnit.relation`, bit `VIEW_RELATION_SELECTABLE` (Units). |
| `Unit::carrier` | `ViewUnit.carrier` (Units). |
| `Unit::cloakOn` | `ViewUnit.cloakOrder` (Units). |
| `Unit::def` | `ViewUnit.unitDefIndex` (Units). |
| `Unit::f2c` | `ViewUnit.weaponBuild` (Units), with the weapon definitions as loaded data. |
| `Unit::f48` | `ViewUnit.weaponBuild` (Units), with the weapon definitions as loaded data. |
| `Unit::f_1f` | Left out: only the debug overlays draw it, which need the debug masks 0x1 and 0x2 of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `Unit::f_3b` | Left out: only the debug overlays draw it, which need the debug masks 0x1 and 0x2 of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `Unit::f_57` | `ViewUnit.weaponBuild` (Units), with the weapon definitions as loaded data. |
| `Unit::fireOrder` | `ViewUnit.fireOrder` (Units). |
| `Unit::flags` | `ViewUnit.flags` (Units), cut by its rule; bit 0x10 and the build menu's page bits are presentation state. |
| `Unit::group` | `ViewUnit.group` (Units). |
| `Unit::health` | `ViewUnit.health` (Units). |
| `Unit::id` | `ViewUnit.id` (Units). |
| `Unit::immobile` | `ViewUnit.immobile` (Units). |
| `Unit::killCount` | `ViewUnit.killCount` (Units). |
| `Unit::list` | `PlayerView.orders` and `ViewUnit.orderKind`, `orderTarget` (Orders); the table of order kinds is loaded data. |
| `Unit::list2` | Left out: only the debug overlays draw it, which need the debug masks 0x1 and 0x2 of `endGameFlags` (presentation state); with them clear no display, sound or command depends on it. |
| `Unit::mode` | `ViewUnit.ground` (Units). |
| `Unit::motion` | `PlayerView.pieces` through `ViewUnit.firstPiece` and `pieceCount` (Units): the pose the model is drawn in. |
| `Unit::moveOrder` | `ViewUnit.moveOrder` (Units). |
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
| `bits` | `PlayerView.endFlags` for masks 0x4, 0x20 and 0x40; masks 0x1 and 0x2 are presentation state ("What is not sim"). |
| `bits_14281` | `PlayerView.sightOptions` (Sight); mask 0x8, the fog tiles' cache, is presentation state. |
| `bits_2a44` | Left out: network state, class platform in part: it picks the music's mood and the network lines, neither of which the display log, the sound log or a command records. |
| `bits_37f2f` | Presentation state ("What is not sim"). |
| `bits_38a51` | Presentation state: the driver's pause request and the lag marker ("What is not sim"). |
| `block` | Left out: read only by the options menu, which copies the options it may restore; no display, sound or command depends on it during play. |
| `cells` | `PlayerView.features` (the features of the cells) and `mapWidth`, `mapHeight`; a cell's height is shown only by the debug strip. |
| `commanderDeath` | `PlayerView.commanderDeath` (Score). |
| `count_1491b` | `PlayerView.debris` (Explosions and debris). |
| `defs` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `difficulty` | Left out: read only by the menus and setup screens, before or after play; the match setup supplies it, not the view. |
| `endGameFlags` | `PlayerView.endFlags` for masks 0x4, 0x20 and 0x40; masks 0x1 and 0x2 are presentation state ("What is not sim"). |
| `features` | Loaded data: the feature definition named by `ViewFeature.featureIndex`, through the files service. |
| `field_1df2` | `ViewPlayer.unitCount` (Score): the item is `players[1].unitCount`, which the share dialog reads with `unitsCreated`, when it opens and on its OK, to offer and accept only a player still alive. |
| `flagsByte_3923b` | `PlayerView.endFlags` for masks 0x4, 0x20 and 0x40; masks 0x1 and 0x2 are presentation state ("What is not sim"). |
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
| `mapFlags` | `PlayerView.sightOptions` (Sight); mask 0x8, the fog tiles' cache, is presentation state. |
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
| `spots` | `ViewFeature.frame`, `shadowFrame` and `ViewFeature.model` (Features). |
| `startPos` | Loaded data: the side definitions, indexed by `ViewPlayer.side`, through the files service. |
| `ticks` | `PlayerView.tick` (The world). |
| `types` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `uiOptionFlags` | Presentation state ("What is not sim"). |
| `uiOptionFlags_37f2f_1` | Presentation state ("What is not sim"). |
| `unit` | `ViewFeature.model`, `x`, `y`, `z`, `rotX`, `rotY`, `rotZ` (Features): the scratch unit a model feature is drawn through holds only the feature's pose. |
| `unitDefCount` | Loaded data: the unit definition named by `ViewUnit.unitDefIndex`, through the files service. |
| `units` | `PlayerView.units` (Units). |
| `unitsEnd` | `PlayerView.units` (Units). |
| `unknown_1b8e` | `PlayerView.players` (Players): unnamed ranges of the player table. |
| `unknown_1cd9` | `PlayerView.players` (Players): unnamed ranges of the player table. |
| `visibilityMask` | `PlayerView.sight` (Sight). |
| `width` | `PlayerView.mapWidth`, `mapHeight` (The world). |
| `windX` | `PlayerView.windX`, `windZ` (The world). |
| `windZ` | `PlayerView.windX`, `windZ` (The world). |
