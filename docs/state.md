# State classes

`data/state_classes.csv` says, for every piece of the game's state, whether it belongs in the shared state block (#7) and whether presentation may touch it (#22). This file explains how the classes were chosen and what is left to split. It is the output of #125.

## The file

One row per item, sorted by kind, then address, then name. Kinds: `global` (a row of `data/globals.csv`), `static` (a variable the source defines that `data/globals.csv` does not list), `game-region` (a top-level field of `struct Game`) and `heap` (an allocation site in game logic).

Columns: `name, address, kind, proposed, class, rule, phases_write, phases_read, presentation, load_write, writers, readers, note, sim_bits, view_bits`. The tool's docstring describes each one.

The classes:

- **sim**: simulation state. Goes in the shared state block.
- **view**: one player's view and the UI. Presentation owns it.
- **platform**: devices, files, windows, network slots and other host state, and load-time asset inputs that the tick reads but nothing in game logic writes (#683). No tool treats the two differently, so there is no `load` class.
- **mixed**: used both ways. Split it; the note says how.
- **const**: read-only input to the sim: nothing writes it, it stays in the image, and `--check` fails if a strong writer appears (in load code or not). A weak writer (a `?` in `writers`, the address taken or the table passed to a function) is allowed when the `note` says what it does with the table; `--check` fails on weak writers with an empty note.

Scripts and movement own their state since #993: the COB scripts (`UnitScript`, its piece table and statics, the
read buffer of `LoadScriptState`) and the model state of a scripted unit are scripts', and `UnitMotion`, its goals and
the movement targets (`PathOrder`, `AirManeuverOrder`, `ApproachRadius`, `RingApproach`, `PointMarker`) are
movement's. Their notes name the owner. A movement target is allocated by the slot that makes it
(`NewPathOrderAtNative` and the others in `src/seams/native.cpp`), so one heap row per slot replaces the rows of the
orders that called `new` themselves. The classes did not change (sim), and the state stays hashed where it was
(section 5 of `docs/scripts-interface.md` and of `docs/movement-interface.md`).

`proposed` comes from the rules below and is rewritten on every run. `class` is the hand review, starts empty, and is kept by regeneration, as are `note`, `sim_bits` and `view_bits`.

```sh
uv run tools/stateclasses.py                 # regenerate, keeping class and note
uv run tools/stateclasses.py --check         # fail if anything but class and note differs
uv run tools/stateclasses.py --unclassified  # fail if a row has an empty class
uv run tools/test_stateclasses.py            # tests
```

## The rules

The evidence is `data/rwsets.csv` joined to the call graph. Three walks give each function the set of places that reach it:

- **Phase roots** (the calls of `RunGameSteps`, `hazards.ROOTS`): phases 0 to 8, so a read or write gets the phases it happens in.
- **Load roots** (`LoadMatch`, `AssignStartPositions`): `load_write`.
- **Presentation roots and directories**: the non-sim calls of `RunGameSteps`, `MainLoopTick`, the frame handlers, and every function under `src/graphics/`, `src/sound/`, `src/gui/`, `src/frontend/` and `src/ingame/`. The phase and load walks stop at such a function; `StepGafSequence` is the one exception.

`proposed`, first match wins:

- **platform**: a device, file, window or DirectX type or name, or only `src/debug/` and `src/harness/` touch it.
- **mixed**: a phase writes it and presentation writes it, or presentation writes what a phase reads (load counts like a phase).
- **sim**: a phase writes it, or load writes it and a phase reads it, or a phase reads it and nothing writes it (tables).
- **view**: presentation writes it and no phase reads it, or only presentation reads it.
- empty: only code outside every walk touches it.

Known limits:

- **Type-wide evidence.** A struct-typed global, a region, a static or a heap block takes the accesses of every use of the type, not of the instance. Rect and Vec3 collect everything.
- **Weak writes.** `*_maybe_written` (a non-const pointer argument) and `*_escape` (address taken) count as writes for phases and load, but only as reads for presentation. A phase can be flagged sim on a weak write alone, and presentation can hide a real write behind one. The rule text says `(weak writes only)`.
- **Name-based walks.** Presentation is partly defined by directory, so a sim helper under those directories is called presentation. Indirect calls need the hand-resolved edges in `data/hazard_edges.csv`; a missing edge is a missing phase.

## Counts

As of the last read of `data/state_classes.csv` (2604 rows). `tools/test_stateclasses.py` recounts the file by kind and class and fails when this table differs, so a change to the classes updates it in the same commit:

| kind | sim | view | platform | mixed | const | total |
|---|---|---|---|---|---|---|
| global | 47 | 353 | 287 | 0 | 61 | 748 |
| static | 6 | 28 | 679 | 0 | 71 | 784 |
| game-region | 112 | 491 | 130 | 49 | 0 | 782 |
| heap | 154 | 48 | 88 | 0 | 0 | 290 |

## Mixed cases and how to split them

The eleven game-region rows with class `mixed`. The ones split by bit carry the split in `sim_bits` and `view_bits`, a
mask each, counted from the row's first byte, on the row and on every alias of the same bytes (`uiOptionBits`,
`bits_14281` and the others); a bit in neither is platform or unused. The others are split by sub-field and have no
masks:

- `players` (+0x1b63): platform for the net slot, ping, `info` and `name`; sim for the rest (type, units, squads, explored map, resources, alliances).
- `localPlayer` (+0x2a42): sim reads it as the perspective player. Fix a sim perspective player at match start; keep a separate view local player. Both uses take the whole byte, so its sim bits and its view bits are both 0xff.
- `playerIndex` (+0x2a43): the watched player in the views, but sim reads it (unit flag, visibility, line of sight). Same split as `localPlayer`, with the same masks.
- `weapons` (+0x2cf3): view for the text, animation, model, sound, shake and colour fields; sim for damage, range, reload, velocity and flags.
- `mapFlags` (+0x14281): sim bits for line of sight and mapping mode; the "view needs refresh" bit cleared by the camera becomes view state. Sim bits 0xfff7 (bits 0 to 2, and the unnamed bits the trace hashes), view bits 0x0008.
- `defs` (+0x1439b): view for `soundcategory` and `field_18e`; sim for the rest.
- `uiOptionFlags` (+0x37f2f): sim bits for treedeath and the cheat toggles (halfshot, shootall, drop, radar); view bits for clock, noshake, selboxes; bit 0 (skip net timeouts) is platform. Sim bits 0x0788 (bit 3 and bits 7 to 10, the bits the trace hashes), view bits 0x0056 (`now` bit 1, selboxes bit 2, noshake bit 4, clock bit 6). The byte views take their byte of each: `uiOptionFlags_37f2f_1` 0x88 and 0x56, `matchFlags` (+0x37f30) 0x07 and 0x00.
- `net_flags` (+0x38d75, with its aliases `flags_38d75`, `loadingFlags`, `netBits`, `net_bits`): sim bits 0 and 1 (`DropPlayer`) and 2 (`MainLoopTick`, packet 21); bit 3 (start positions assigned, read by the load screen) is platform. Split bits 0 to 2 into a sim word. Sim bits 0x0007, no view bits, on `loadingFlags`; its other aliases are classed sim.
- `otherFlags` (+0x38a51): bit 0 is the pause request, written by the net and the UI. Give it to the driver; the other bits are UI. No sim bits: bit 0 is the driver's (platform), view bits 0xfffe (0xfe for the byte `pauseFlags`, 0xff for `pad_38a52`).
- `endGameFlags` (+0x3923b, with its aliases `endGameFlags_3923b`, `flagsByte_3923b`, `bits_3923b` and `bits`): bits 0 and 1, the debug display and debug mode, are view state; the rest is the simulation's ("The debug bits of endGameFlags" below). Sim bits 0xfffc, view bits 0x0003 (0xfc and 0x03 for the byte `flagsByte_3923b`).
- `net` (+0x391e9): the mission object. Sim for wind, tide, water damage, kill and time multipliers, start positions and the TDF lists; view for names, description, planet and the text fields.

## Presentation writes sim state (#22 crossings)

Found during review, and re-checked against the code for #783 and #1049. Functions are cited by name, never by line.
During play, presentation changes the simulation only through the events of `docs/commands.md` (sections 4 and 8);
`tools/stateclasses.py --check` fails when a presentation function (a function of the presentation subsystem in a
presentation folder, or a console command handler of src/game/console_commands.cpp's command table) writes a field
classed `sim` of the game or a global, a bit field of the game through the member that holds it, or a field of `Unit`,
`Player`, `Order`, `Feature` or `Projectile` that a phase or a load function touches, unless `PRESENTATION_SIM_WRITES`
in that tool names the write with its reason. A field classed `mixed` with `sim_bits` (`uiOptionFlags`, `mapFlags`,
`localPlayer`, `playerIndex`, `otherFlags`, `loadingFlags` and their aliases) is judged by bit: the write fails when the
bits it can change meet the sim bits. `tools/rwsets.py` gives those bits as a `mask_*` row where the syntax tree bounds
them (`|=`, `&=` or `^=` with a constant, or a write of a bit-field, for the member that holds it); a write with no mask
row can change any bit. So the camera's `mapFlags &= 0xfff7` passes, and a direct write of a cheat bit fails. A field
classed `mixed` without masks (`players`, `weapons`, `defs`, `net`) is split by sub-field and not checked.

Crossings left during play, each waiting on another decision:
- `CmdControl` and `CmdView` (console_commands.cpp) write `localPlayer` and `playerIndex`, which the tick reads. They
  become events (#1117).
- The share commands (`CmdShareMetal` and the others of share_commands.cpp) write a player's sharing switches and
  levels, in a network game only (no test-mode run has one).
- `WriteScreenshot` (keys.cpp), the poster key, turns mapping and line of sight off, calls `RecalculateLineOfSight`,
  draws the whole map and turns them back on with a second `RecalculateLineOfSight`. A presentation that draws from the
  view (#1052) needs neither. It also clears and restores the clock, bit 6 of `uiOptionFlags`, a view bit, but by
  assignment, which the mask cannot bound, so the table lists that write too.
- `DispatchOrdersPanelPageFlags` and `StepBuildMenuPageBack` (control_panel.cpp) write the build-menu page bits of
  `Unit.flags`: bit 0x400000, the build menu rather than the orders, and the page number. Only presentation reads
  them, but the trace still hashes them, since the mask covers only the selection bit 0x10. #1115 finds and masks
  the presentation writes that remain hashed.
- `DrawScorePanel` (game_dialogs.cpp) decrements `Player.rank` of each counted player above a rank no player holds,
  while it draws the score panel; the kill credit of `units.cpp` (the rank update when a unit dies) reads and writes the
  same ranks. It becomes the simulation's own compaction, or an event, in a later issue.
- The selection (`Unit.flags` bit 0x10 and the other selection fields) is written by the selection functions of
  selection.cpp; #1050 takes it out of the judged state.

Not crossings:
- `HandleGameKey` (keys.cpp), the debug key, toggles bits 0 and 1 of `endGameFlags` (the debug display and debug mode)
  through `endGameFlags_3923b`. Those two bits are its view bits, so the write passes by its mask.
- `DrawWeaponCoverage` (markers.cpp) writes `Order.cached` and bit 0x200000 of `Order.flags_42`, its own cache of the
  last place it saw a target: nothing in the tick reads them, and the trace leaves both out (tools/traceschema.py).
- The pause request, bit 0 of `otherFlags`, written by the options screens and the keys: it is the driver's, which
  reads it to decide whether to run a tick; the only phase that reads it is the network's timeout check, on the wall
  clock.
- The bit of `mapFlags` that the camera (camera.cpp, control_panel.cpp) clears, bit 3: it says the drawn fog of war is
  current, the tick only clears it and drawing reads and sets it, and the trace leaves it out (mask 0xfff7). The word's
  bits 0 to 2 are the simulation's, set by LOS_RADAR_SET_MODE.
- The lobby writes the packet manager and players before play. That is fine: nothing ticks yet. So do the frontend's
  menus (the difficulty, the number of players, the commander death rule) before a match loads.
- Not a crossing: `g_explodedPieces`. `DrawExplosions` and `DrawExplodedPieces` (explosions.cpp) only clear a slot when `DrawExplodedPiece` returns 0, which it never does.
- Not a crossing: `g_debrisMemCache`. The older note said the end-game screen frees it, but the only calls outside the explosion code are `InitMemoryCache` and `FreeMemoryCache` (endgame.cpp), which run at startup and through `atexit`. No commit removed a crossing there: the attribution was wrong when the note was written (6b9479b31).

## The size of the block

The sim block is a range of address space of 256 MiB (`kSimBlockCeilingBytes`) of which the first `size` bytes are
committed and allocated from. `size` is 32 MiB (`kSimBlockBytes`) at start-up and grows, never shrinks, to the size
of the match, a multiple of 65536 from 32 MiB to 256 MiB. It lives in the block's header (`BlockHeader::size`), so a
copy of the block carries it. The growth adds the new bytes at the top to the free list, and the allocator is first
fit from the bottom, so a larger block moves only the point where memory runs out: trace hashes use pointer IDs, not
addresses, and equal at any size.

I picked 256 MiB as eight times the default: it leaves an i686 process (2 GiB of user address space) room for its
own heap, it fits wasm32 (4 GiB) and the 2 GiB or so a browser gives a memory, and WASM mode commits the ceiling up
front in the one linear memory (`docs/module-abi.md`, 2.1).

The size is the base game's 32 MiB plus the bytes the loaded modules declare beyond the default's (today the
pathfinding region: `RegionSize` of the installed implementation minus the default's, if positive), rounded up to a
page. With no replacement it is exactly 32 MiB.

## The match setup

Two values every player agrees on, both 0 by default (`src/game/match_setup.h`):
- `blockBytes`: 0 sizes the block as above; otherwise it is the block's size, which must be at least what the
  modules need.
- `searchSteps`: the search steps per tick pathfinding is limited to, at most 2147483647; 0 leaves each implementation
  its own default budget (`docs/pathfinding-interface.md`). A larger value is refused wherever it comes from.

Where they come from and how they travel:
- **A test-mode run:** the scenario keys `block_bytes` and `search_steps` (`docs/scenario-format.md`); `game_type 3` puts
  them in the host's `PlayerInfo`, as a lobby does. A game outside test mode takes the switches `-blockbytes <n>` and
  `-searchsteps <n>` of the command line.
- **A lobby:** the host puts them in its `PlayerInfo`, bytes 173 to 178 (the unused tail `matchSetup`: a 16-bit count
  of 65536-byte pages, then a 32-bit step count), which the lobby already broadcasts to everyone, and every
  machine reads them from the host's record when the match loads (`LoadMatch`, game type 3). Zeros are the defaults,
  so a 3.1c host's lobby is unchanged. With `blockBytes` 0 each machine derives the size from its own modules, so players with different
  modules would differ; nothing checks that yet (a host that wants it checked sets `blockBytes`, which a machine
  whose modules need more refuses).
- **A saved game:** `SaveGameFile` writes the items `BlockBytes` and `SearchSteps` in the summary account, only when
  they are not the defaults, so a save of a default match is byte for byte what it was. A load reads them before the
  assets load and they replace the command line's.
- **The match:** `searchSteps` lives in the block header (`BlockHeader::searchSteps`), so a copy of the block and a
  roundtrip keep it. The `search` console command sets it.
- **Replays:** a recording of a subsystem (`docs/replay-format.md`) calls the world view, whose
  `SearchStepLimit` returns the value like any other read. The format is unchanged.

## Ownership at save and load

The access log (`tools/accesslog.py`, `lib/accesslog`) records the windows of every tick and, apart from them, a save
window around `SaveGameFile` and a load window around `LoadSavedGameState` and `LoadBattleAssets` (the match load, on the
load thread). `tools/privatestate.py --check` holds both to the owner rules of the tick: a module that writes or reads
another module's private state fails whether it does so inside a tick or while a game is saved or loaded. The summaries of
the two windows are `data/access_saveload/<scenario>.saveload.csv.gz`, and a save or load window adds no evidence to the
tick's. A module restores its own state at load, through its own load slot or through an event it consumes (targeting's
LOAD, map's LOAD_PATH_LOCK_STAMP, wind_meteors' LIMITS and RESET); section 5 of each interface document lists what it
restores. `tools/seamlink.py --private-state` reads `data/rwsets.csv`, which has every function and no windows, so it
covers the save and load code as it covers the tick's.

## The selection

The selection is view state (#1047, decided by the human on 2026-11-24), though it is a bit of the simulation's unit
record: `selected`, bit 0x10 of `Unit.flags` (+0x110). It stays there in this repository, as 3.1c lays it out; moving
it is work for the re-implementation. The trace masks it out of the `Unit` table (`tools/traceschema.py`, mask
0xffffffef), so no row hashes it (the phase rows, `world` and the end-of-tick `tick` row), and two runs that differ
only in what is selected have equal traces. The save keeps the bit as 3.1c writes it. What a presentation is judged on
is the UI log's `select` record (`docs/ui-log.md`), and the group command a click sends carries its units
(`OrdersGroupCommand`), so the simulation never needs the selection to carry out an order.

Who touches the bit:
- Presentation sets and clears it (the functions of selection.cpp) and reads it: the panels (control_panel.cpp,
  info_panel.cpp), and `SendGroupCommand` (selection.cpp), which turns the selection into the units of a group command.
  `ExpandGroupCommand`'s default reads the units from the command alone.
- Game logic reads it once: `UpdateUnitBody` (unit_position.cpp) clears it when the unit can no longer be selected (not
  selectable, still being built, in its hold-off after a transfer, or carried by a unit whose cargo is not selectable).
  The read changes nothing but the bit. It is 3.1c's deselection rule, a rule of the view that runs in the tick.
- Game logic writes it twice more, and reads it nowhere else: `InitUnitFromType` (units.cpp) clears it for a new unit, and
  `LoadUnit` (unit_save.cpp) restores it from the save.
- The harness reads it for the UI log's `select` record.

## The debug bits of endGameFlags

Bits 0 and 1 of `endGameFlags` (+0x3923b) are view state too: bit 0 is the debug display (`INFO ON`) and bit 1 debug
mode. The trace masks them out of the game scalars (mask 0xfffc). The word's other bits are the simulation's (bit 2,
the game is over, and bit 3, set when a saved game is chosen to load), so the rows of `data/state_classes.csv` for the whole word are `mixed`, with sim bits 0xfffc and view bits 0x0003.
The row `bits_3923b_1`, the member that names only bit 0, is `view`, so `tools/stateclasses.py --check` passes
`HandleDebugHotkey`'s write; `bits_3923b_2`, which names only bit 2, is `sim`.

- Presentation sets them: `HandleGameKey` (keys.cpp) flips bit 1 and clears bit 0 with it, and `HandleDebugHotkey`
  (game_dialogs.cpp) flips bit 0. It reads them in the panels (info_panel.cpp) and the keys (keys.cpp).
- Game logic reads bit 1 once: `CmdDebugBreak` (console_commands.cpp), a console command, which with the cheats on
  runs the out-of-memory tests. Nothing in the tick reads either bit.
- Game logic writes them once: `InitGame` (game_state.cpp) clears both.

## Determinism findings

- **`g_timeoutTimerStart`** is set from wall-clock `GetTicks()` in `CheckPlayerTimeouts` (net_game.cpp), and `IsTimedOut` reads it. This is network only and already a #40 task.

Fixed:

- **`DrawProjectiles`** (weapons.cpp) drew its lightning jitter from the sim CRT `rand()` stream. c1c5fdba8 (`Fix: Draw lightning jitter from the view stream`) moved it to `HarnessViewRand()`.
- **`CheckDefeat`** (victory.cpp) took `g_cdMismatchDefeatTick` from CRT `rand()` when the CD check set `g_cdPathMismatch`. d6016d686 (`Remove: CD check`) deleted the check, and `MissionConditions::CheckDefeat` now takes a `RandStream` it does not draw from.

## Limits and left out

- Load-time code is in the load walk, and a destructor's accesses are evidence (`delete` and temporaries are `call` rows in `data/rwsets.csv`). A destructor of a class that is neither user-provided nor virtual is not followed (see the docstring of `tools/rwsets.py`).
- The classes of clear-cut UI, debug and platform rows rest on the tool's evidence plus names. Rows that matter to the tick were checked against the code.
- Rows whose proposal changed when #745 made `stateclasses.py` read `link/globals.h` (#749). Each group is not sim for the reason given, so nobody has to audit it again:
  - Load-only rows (`sim: only load code touches it`: `count_147ab`, `entries_147af`, `loadProgress`, `loadPctUnits`, `loadPercent`, `progress_38d73`, `shade`, and the like): the rule proposes sim for a load write that no phase reads. They are art lists and loading-bar bytes that only load and drawing code touch.
  - Chat HUD (`chatHudRing`, `chatHudRing_12ef`, `entries`, `head`, `tail`): `AddMessage` runs in the tick and writes the ring, and only `DrawMessages` reads it, so it is an output of the tick to the screen.
  - Network debug knobs (`netSendDropPct`, `compressionDisabled`, class platform): set by console commands and the command line, read only by `NetCondenser::SendPacket`; they change the wire, not game state.
  - Game-name settings (`gameName`, `gameName_2bc1`, class platform): registry settings copied to the net session. The presentation writes the tool blames on them come from `LoadSettings` declaring `password` as one 71 KB array.
  - Redraw dirty bits (`viewDirtyFlags`, `flags_142f1`, `bits_142f1` and its pieces, `viewDirtyFlags_142f1`): line-of-sight code sets "redraw the minimap" bits and only drawing reads them.
  - The `f14353` debug counter (`activeUnitTickCount`): `UpdateAllUnits` counts and only `DrawUnitInfoPanel` shows it.
  - Camera-follow state (`tracked`, `value_142f3` and its `pad_142f3` pieces, `value_1434b`): the tick writes the followed unit and projectile position and only `UpdateCameraFollow` reads them, so they are outputs of the tick to the camera. `trackedPos` is a Vec3, so the tool's sim writers are type-wide.
  - Platform rows proposed view (`text_38b53` and `pad_38b53`, `movieCaptureIndex`, `nextMovieFrameTick`, `pendingSaveStore` and `pad_38d6b`): output directories and a save-store handle, files rather than state, which the tool has no rule for outside its name list.
  - Sim rows proposed mixed (`projectileCount`, `projectiles`, `mapWidth`, `mapHeight`, `featureCount`, `features`, `visibilityMask`, `cells`, `spatialGrid_1429f`, `overflowBucket`, `types`, `unitDefCount`, `list_391ed` and their `pad_` pieces): the presentation write is the `LoadSettings` password-array overlap (+0x2be3 to +0x1434d), a console command (`CmdEdge`), or a load or free routine that code outside the tick also calls (the match shutdown in `src/game/game_state.cpp`). The tick is their reader, and its writes are the ones in `phases_write`; the notes say which.
  - Alias pieces added by game.h (#211) take the proposal of their own accesses, not of their region: `textLines` (view), `matchFlags` (class mixed, see `uiOptionFlags` above), `p1b8a` and `p1cd5` (slices of `players`).
- Globals that no phase or load walk touches, settled from the code (#778):
  - Unreferenced constants and words (`DAT_004fc474`, `DAT_004fc978`, `DAT_004fcc48`, `DAT_004fd2e4`, `DAT_004fda70`, `DAT_004fdbe0`, `DAT_00501fcc`, `DAT_00502f98`, `DAT_00507b64`, `DAT_00507b68`, class const): no source names them and `orig/TotalA.exe` holds no 4-byte reference to any of the addresses. Their rows stay because `data/globals.csv` lists the definitions; the tool drops a row only when its global is no longer defined.
  - Command tables (`g_consoleCommands`, `g_cheatCommands`, `g_debugCommands`, class const): only `InitCommands` names them, and `RegisterCommands` copies from them without writing. The handlers they name are the crossings in "Presentation writes sim state".
  - `g_rotationMatrix` (class view): scratch of the matrix builder (0x4b71a7) and apply (0x4b72e2) in `src/util/fixed_trig.cpp`, which nothing calls: the exe has no call, jump or pointer to either address.
  - `g_emptyStringRep` and `g_emptyString` (class platform): the shared empty string of `StringRef` and its count. Game logic builds `StringRef`s at load and in phase 0 (`ComputeUnitScriptChecksum` through `ListDirectory`), but the walks stop at `src/graphics/surface.cpp`. The count only decides whether to free or copy, and never reaches zero, so no game value depends on it.
