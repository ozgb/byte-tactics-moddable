# The commands of a tick as events

A scenario scripts three kinds of line that the game would otherwise get from a person: `spawn`, `order` and `console`
(`docs/scenario-format.md`, section 4). This page says what each of them is, as events of the interfaces: which
events a line publishes, in which order, with which payload, from which state, and what it does when the line names
nothing that exists. A core that publishes the same events in the same order at the same point of the tick plays the
scenarios that the page lists as judged (section 7) to the stored traces, without anything else of the line's
meaning. The events themselves, and what each kind does when a module consumes it, are in the interface headers; this page
never repeats that.

## 1. The rule of delivery

- A command is delivered when no tick is running: before the first phase of the tick it precedes, in the step the
  scenario format calls the start of the tick. The tick a line names is that tick.
- The commands of one tick are delivered in the order of `docs/scenario-format.md`, section 4: the `spawn` lines, then
  the `order` lines, then the `console` lines, each group in file order. (`input` lines come after the console lines,
  are not commands of this page, and no scenario of section 7 has one.)
- The interface's records (section 8) come first. The interface acts after tick `N` has finished, so its records of
  that moment precede tick `N + 1`: they are delivered before the `spawn` lines of tick `N + 1`, in the order the
  interface published them. A core delivers them at the start of tick `N + 1`, as it does the scripted lines, and so
  does a run played from the command log (section 2.2). That holds for the last of them too: when the interface ends
  the run after tick `N` (the exit menu's Yes publishes PLAYERS_LEAVE_GAME), its records are stamped `N + 1`, and the
  log's `end` line (section 2) says that the run ended without playing tick `N + 1`.
- Every command publishes its events to the engine's outbox. An event published while no tick is running reaches
  the module that handles its kind at once: the module consumes it in its `Apply`, in the order of publication, before
  the next event of the command is published. An event that a handler publishes while it consumes one is delivered at
  once too, before the handler returns.
- The inboxes are empty again before the tick starts: nothing a command published waits for a phase.
- A command reads the state as it is when it is delivered, after every earlier command of the tick took effect. A
  guard that the page states ("a player that exists", "the cell has a feature") is evaluated at that moment, so a
  `console` line that follows another of the same tick sees its effects.
- A guard that fails publishes nothing, unless the page says otherwise. A guard that an event's `Apply` has (a
  player above 9, an amount of 0) is not checked by the command: the event is published, counted in the log (section
  2), and refused by its module with outcome 0.

## 2. The command log

A test-mode run writes `commands.txt` beside `trace.txt`. It is a diagnostic and an input a core can be played from;
its scripted lines are not a verdict and have no stored copy, and its interface records are judged in their decoded
form (section 2.1). Its first line is

    bt-commands 3

and its second, written before the first command, is

    units <offset> <stride>

the offset of unit 0 of the unit table in the state block and the size of a unit record, both in decimal: what a
reader needs to turn a unit ref of a payload into a unit index. Every later line but the last is one command, in the order the
commands were delivered: a scripted `spawn`, `order` or `console` line, or one event the interface published:

    <tick> <key> <line> <count>[ <KIND> <hex>]...

- `tick` is the tick the command precedes. For a scripted line it is the line's tick. The interface acts after a
  tick's simulation has finished (`docs/ui-log.md`, "Delivery"), so its event of the UI log's tick `N` precedes, and
  carries, tick `N + 1`, and comes before the scripted lines of tick `N + 1` in the file.
- `key` is `spawn`, `order`, `console` or `interface`.
- `line` is the line number of the scenario file, or `-` for the interface.
- `count` is the number of events the command published to the engine's outbox. Only outermost publications count:
  an event that a handler publishes while another event is being delivered is not counted and not listed. An
  interface line is one event, so its `count` is 1.
- After `count`, one ` <KIND> <hex>` pair for each counted event, in publication order: `KIND` is the enum name of
  the event's kind in its interface header (`PLAYERS_TRANSFER`), and `hex` is the event's payload bytes, all of them
  from its `kind` field on, in lowercase hexadecimal with no separators, each integer in the machine's little-endian
  byte order. A kind of a module whose events have no section of the trace is written `KIND_<n>` instead, its number
  in decimal: PATHFINDING_SET_SEARCH is `KIND_448`.
- `input` lines get no line of their own: what the interface makes of them is its records. A command that publishes
  nothing is a line with `count` 0 and no pairs.
- The interface's records are every event published to the engine's outbox after a tick's simulation has finished
  and before the next tick starts, outside the scripted lines: section 8 lists them. After the last tick nothing is
  recorded, except when the interface itself ends the run: its records of that moment are stamped the tick after the
  last.

Its last line, written when the run ends, is

    end <tick>

the last tick the run played, in decimal. Every line before it is stamped at most `<tick> + 1`, and a line stamped
`<tick> + 1` is a record of the interface's last act, the one that ended the run. A run that reached its `ticks` or
ended on the end-of-game state has no such line.

Format 1 had neither the `units` line nor the interface's records, and format 2 had no `end` line.

A core is played from the file by section 2.2: each line's events, at its tick, in the file's order, ending where the
`end` line says.

### 2.1 The interface's records, decoded

A payload holds a unit as a ref, its offset in the state block, which another build with another block layout does
not share. What is judged is therefore each interface record decoded, and that is what `reference/commands/` stores
for every scenario with `input` lines: `reference/commands/<scenario>.txt`, written by the reference build, which has
no state block and writes the same form from its own records of the interface's clicks. Its first line is

    bt-interface 1

and every later line is one interface record of `commands.txt`, in the file's order:

    <tick> <KIND>[ <field> <value>]...

- `tick` and `KIND` are the record's.
- The fields are those of the kind's entry in `data/trace_schema.json` (the `kinds` of the module's events section),
  in the entry's order, each at the offset that the sizes of the fields before it add up to: every field but `kind`
  that lies wholly inside the payload's bytes (an `outcome` field never does: the module writes it, the publisher
  does not).
- A `scalar` field is the little-endian signed integer of its size, in decimal. A `ref` field of target `unit` is
  `-` when it is 0, the unit index `(ref - offset) / stride` when the ref is a unit of the table (by the `units`
  line), and `heap` otherwise; a `ref` of any other target is `-` or `heap`. A `bytes` field is its bytes in
  lowercase hexadecimal.
- A kind with no entry in the schema is `<tick> KIND_<n> payload <hex>`.

A group command, for example, reads

    23 ORDERS_GROUP_COMMAND unit 1 mode 3 type 0 queue 0 target - hasPos 1 pos 000086020000010000003601 params 0000000000000000 units 0100...

where `unit` is the local player's first unit, bit `i` of `units` is the unit `unit + i`, and `pos` is three 16.16
fixed-point numbers (646, 1, 310). A group key (ctrl and a digit) is one `AI_SET_SQUAD` record for each of the local
player's units whose squad it sets, in unit order: the group's number for a selected unit, 0 for an unselected unit
that was in the group.

`tools/interfacelog.py` decodes a `commands.txt`. `tools/reference.py --check` decodes each run's file and compares
it, line by line, with the stored one, naming the first record that differs. A scenario with `input` lines must have
a stored file, a scenario without them must have none and its run must have no interface record, and a stored file
with no scenario fails.

### 2.2 Playing a core from the file

The file is an input as well as a record: a core that publishes each line's events at the line's tick gives the
traces of the run that wrote it, without the presentation and without the scenario's commands.

- **The switch.** A test-mode run started with `-testcommands <file>` (`tools/playtest.py --testcommands`) plays
  `<file>`, the `commands.txt` that a run of the same scenario on the same build wrote. It still reads and checks the
  scenario's `spawn`, `order`, `console` and `input` lines but delivers none of them; every other line of the scenario
  acts as usual.
- **The units line** must equal the build's (status 2 otherwise): a payload holds a unit as a ref into that build's
  block (section 2.1), so the file is played on the build that wrote it.
- **Delivery.** At the start of each tick, before its first phase, the run takes the file's lines of that tick in the
  file's order, which is the order of section 1 (the interface's records first, then the scripted lines). For each
  line it publishes each `<KIND> <hex>` pair's bytes, in order, to the engine's outbox, as one command: the kind is
  the payload's first four bytes, and the name is not read. A line whose tick is below the tick being started, or that
  is not in the format of section 2, stops the run with status 2. Lines after the last tick are not played.
- **The end.** The last line must be `end <tick>`, and no line may be stamped later than `<tick> + 1` (status 2
  otherwise, before the first tick). At the start of tick `<tick> + 1` the run delivers that tick's lines, which
  publish the interface's last act, and then ends without playing the tick, as the run that wrote the file ended: it
  closes its outputs, writes `testmode: -testcommands ended the run after tick <tick>` and exits with status 0, and
  does not fail for a frame it did not reach.
- **Its own log.** The run writes `commands.txt` as any run does, each line with the key, line field and count of the
  line it played, and its `end` line. A file played to the end is written again byte for byte, so its decoded records
  (section 2.1) are the stored ones.
- **What it cannot carry.** A console command of section 4.3 is not an event and one of section 4.4 touches only the
  presentation: neither leaves anything in the file. The presentation, with no `input` lines, does nothing, so a write
  of the interface that is not an event and that the trace hashes makes the run differ from the scenario. That is
  what the check below finds.

`tools/reference.py --check` plays each scenario with `input` lines a second time, from the `commands.txt` of its own
run in the same mode (the run `<scenario>+commands`). A scenario with `frontend 1` (`frontend-skirmish`) is not played
again: a run from the file delivers no `input` lines, so its menus never start the match. The run must give the scenario's stored traces, and its
`commands.txt` must decode to `reference/commands/<scenario>.txt`. `REPLAY_LEFT_OUT` in that tool lists the
scenarios whose run from the file still differs, each with the hashed field and its issue: their run is played and
its records judged, its traces are reported as `left` and not judged, and an entry whose run turns equal fails until
it is removed. No scenario is listed: the selection's bits (`Unit.flags` 0x10 and `selectState`, 0xc0) are out of the
trace and the view, so every run from the file equals its scenario's. `ui-dialogs` ends in the exit menu after tick
61: its `PLAYERS_LEAVE_GAME` is stamped tick 62, and its run from the file publishes it and ends at `end 61`, with as
many ticks as the scenario.

## 3. Reading a console line

The text after the tick and the `+` of a `console` line (at most 79 characters) is the command line.

- **Words.** The line is split at blanks (space, tab and the other white space characters) into words. A `#` ends the
  word it is in and the line: nothing after it is read. At most 20 words are kept. The first word is the command.
- **Numbers.** `argument n` is word n (the command is word 0). A number argument is the decimal integer the C library
  reads at the start of the word: leading white space, an optional sign, digits, and the first other character ends
  it; a word with no digits at its start is 0. An argument the line does not have is its default, which each command
  states (0 when it states none). The count of words is the number of words of the line, the command included.
- **Which command.** The command's first word is compared with the names of the table in section 4, in any case. A
  command that is not enabled does nothing (the line is then given to the unit-name handler, which is enabled only
  with the cheat code, and which finds no unit type of that name). A harness refuses, with status 2, a line whose first
  word is neither a command of the table nor the name of a unit type (`docs/scenario-format.md`, section 4.3).
- **Enabled.** Each command of the table has a class: `always`, `skirmish` or `cheat`. A line is run with the classes
  `always`, plus `skirmish` when the match is a skirmish (game type 2; the other game types of the scenarios do not
  allow it), or with all three classes when bit 0x2 of `uiOptionFlags` is set when the line is delivered. The unit
  names of section 4.2 are in the class `cheat`.
- **The cheat code.** The command `now` sets bit 0x2 of `uiOptionFlags` when its line has exactly six words and they
  are `now Film Chris Include Reload Assert` (the five after the command are compared exactly, in that case), and clears
  that bit otherwise. The bit is not part of the hashed word (the trace keeps bits 3 and 7 to 10 of `uiOptionFlags`).
  A scenario types the code in its first `console` line, and the classes are read again at each line.
- **The cursor.** Before each console line the cursor is at the world position 640 260: the position is 640 and
  260 world units, the height is the ground height there (the map's `GetGroundHeight`) but not less than the sea
  level, and the cell it points at is the cell of 16 by 16 world units that holds it, cell (40, 16). The cell's feature
  is what `GetCellFeature` answers for that cell: the feature type of the feature on it, following a cell of a larger
  feature's footprint back to the feature's own cell, and 0xffff when the cell has none. The cursor is a
  fixed input of a command, not state: a command that moves it (`move`) changes nothing a later line sees.
- **The local player.** `localPlayer` is the slot whose view the game shows (the scenario's `local_player`), and
  `playerIndex` is the slot the interface shows resources and conditions of; they are equal until `control` or `view`
  changes them. A command that says "the local player" or "the viewing player" reads that field when delivered.
- **A player that exists.** A player argument is cut to a byte (its value modulo 256) first. The player exists when it is
  below 10, its record is active, its type is 1, 2 or 3, and its index is not 10. A command that tests it and fails
  does nothing, unless it says otherwise.

## 4. The console commands

### 4.1 The table

The groups: `events` commands published events before this change; `converted` commands wrote simulation state
directly and now publish events; `direct` commands still write state directly and are not events (a scenario that uses
one is not judged; no command is in this group now); `inert` commands do nothing in any scenario (section 4.3), so a
core publishes nothing for them and a scenario that uses one is judged; `presentation` commands touch nothing the
simulation or the trace reads, and a core ignores them.
The class is the one of section 3. Command words are compared in any case.

| Command | Group | Class | Events |
| --- | --- | --- | --- |
| `give` | events | always | PLAYERS_TRANSFER |
| `meteor` | events | skirmish | WIND_METEORS_ENABLE, WIND_METEORS_DISABLE, WIND_METEORS_START |
| `assign` | events | cheat | ORDERS_GROUP_COMMAND |
| `atm` | converted | skirmish | PLAYERS_SET_STORE |
| `nometal` | converted | always | PLAYERS_SET_STORE |
| `noenergy` | converted | always | PLAYERS_SET_STORE |
| `sealevel` | converted | cheat | MAP_SET_SEA_LEVEL |
| `edge` | converted | cheat | MAP_SET_EDGE |
| `iwin` | converted | cheat | PLAYERS_KILL_UNITS, PLAYERS_END_GAME |
| `ilose` | converted | cheat | PLAYERS_KILL_UNITS, PLAYERS_END_GAME |
| `ai` | converted | cheat | PLAYERS_SET_TYPE |
| `kill` | converted | cheat | UNITS_KILL_ALL or PLAYERS_KILL_UNITS, then PLAYERS_DEACTIVATE_CONDITIONS |
| `reload` | converted | cheat | UNITS_RELOAD_TYPE |
| `reloadaiprofiles` | converted | cheat | AI_RELOAD_PROFILES |
| `burnall` | converted | cheat | FEATURES_REMOVE_ALL |
| `burnone` | converted | cheat | FEATURES_REMOVE |
| `feature` | converted | cheat | FEATURES_PLACE |
| `selectable` | converted | always | UNITS_SET_FLAGS |
| `armflea`, `armpw`, `corak` | converted | cheat | UNITS_CREATE |
| `control` | converted | cheat | PLAYERS_SET_CONTROL |
| `view` | converted | skirmish | PLAYERS_SET_VIEW |
| `doubleshot` | converted | skirmish | PLAYERS_SET_CHEATS |
| `halfshot` | converted | skirmish | PLAYERS_SET_CHEATS |
| `radar` | converted | skirmish | PLAYERS_SET_CHEATS |
| `shootall` | converted | always | PLAYERS_SET_CHEATS |
| `treedeath` | converted | cheat | PLAYERS_SET_CHEATS |
| `los` | converted | skirmish | LOS_RADAR_SET_MODE, LOS_RADAR_RECALCULATE |
| `lostype` | converted | always | LOS_RADAR_SET_MODE, LOS_RADAR_RECALCULATE |
| `mapping` | converted | skirmish | LOS_RADAR_SET_MODE, LOS_RADAR_RECALCULATE |
| `nowisee` | converted | skirmish | LOS_RADAR_SET_MODE, LOS_RADAR_RECALCULATE |
| `search` | converted | cheat | PATHFINDING_SET_SEARCH |
| `sharemetal` | inert | always | none |
| `shareenergy` | inert | always | none |
| `shareall` | inert | always | none |
| `setsharemetal` | inert | always | none |
| `setshareenergy` | inert | always | none |
| `now` | presentation | always | none |
| `iface` | presentation | always | none |
| `move` | presentation | cheat | none |
| `drop` | presentation | always | none |
| `assert` | presentation | cheat | none |
| `dprint` | presentation | cheat | none |
| `mem` | presentation | cheat | none |
| `zbuffer` | presentation | cheat | none |

The row of the unit names lists the names the scenarios type: a scenario that types another unit type name needs a
new name in that row. `meteor`, `atm`, `view`, `doubleshot`, `halfshot`, `radar`, `los`, `mapping` and `nowisee` are
in the class `skirmish`: a scenario of another game type runs them only after the cheat code.

### 4.2 Commands that publish events

Each command below says its guards and then its events, in publication order. "The cell" is the cursor's cell
(40, 16) and "the cursor's feature" is the answer of section 3. Amounts are floats.

- **`give <player> <amount> metal` and `give <player> <amount> energy`.** Let `to` be argument 1 (default 0, cut to a
  byte). Nothing when `to` is not a player that exists. Word 3 is compared in any case: `metal` publishes PLAYERS_TRANSFER
  with `from` the local player, `to`, `resource` 1, `amount` argument 2 (default 0) as a float, `announce` 1; `energy` does
  the same with `resource` 0; any other word 3 publishes nothing. An amount of 0 is still published (and refused with
  outcome 0).
- **`meteor [n]`.** One word: WIND_METEORS_START. More than one word: WIND_METEORS_ENABLE when argument 1 is not 0, else
  WIND_METEORS_DISABLE. The events are the header alone.
- **`assign <order type name> [a] [b]`.** Nothing at all when `MissionTypeFromName` answers 0 for word 1. Else one
  ORDERS_GROUP_COMMAND for the local player's selected units, as section 6.2 says it is built for an order with no mode
  and no position: `type` is that order type, `params[0]` is argument 1 read as a number (0 for a name), `params[1]` is
  argument 2, `queue` is bit 2 of the key flags of the interface's last mouse event (0 when the run has no `input`
  line), and `target` the unit under the cursor (0 for none). The command is published when no unit is selected too,
  with no bit set; no unit is selected in a scenario of section 7, so its expansion publishes no ORDERS_COMMAND.
- **`atm`.** Arguments are ignored. Two events: PLAYERS_SET_STORE with `player` the viewing player (`playerIndex`),
  `resource` 0, `add` 1, `amount` 1000; then the same with `resource` 1.
- **`nometal [player [amount]]`, `noenergy [player [amount]]`.** `player` is the local player when the line has one
  word, else argument 1 cut to a byte. Nothing unless it is a player that exists. PLAYERS_SET_STORE with `player`,
  `resource` 1 (`nometal`) or 0 (`noenergy`), `add` 0, `amount` argument 2 (default 0) as a float.
- **`sealevel [n]`.** MAP_SET_SEA_LEVEL with `level` argument 1 (default 0), always.
- **`edge [x [z]]`.** MAP_SET_EDGE with `x` argument 1 (default 32) and `z` argument 2 (default 128), always. (With no
  argument the playable width and height are what the match started with.)
- **`iwin`.** `s` is the side of the local player (0 Arm, 1 Core). PLAYERS_KILL_UNITS with `player` 0 when `s` is 1, else
  1; then PLAYERS_END_GAME with `victory` 1.
- **`ilose`.** PLAYERS_KILL_UNITS with `player` the side of the local player (0 or 1, used as a slot number); then
  PLAYERS_END_GAME with `victory` 0.
- **`ai [player]`.** `player` is argument 1 (default the local player), cut to a byte. Nothing unless it is a player that
  exists. PLAYERS_SET_TYPE with `player` and `type` 2 when the player's type is 1, else 1.
- **`kill [player]`.** With one word, UNITS_KILL_ALL; otherwise PLAYERS_KILL_UNITS with `player` argument 1 cut to a
  byte. Then, always, PLAYERS_DEACTIVATE_CONDITIONS.
- **`reload <unit type>`.** Nothing with one word. The unit type is looked up by name in any case (no wildcard); nothing
  when there is none. UNITS_RELOAD_TYPE with `type` its number.
- **`reloadaiprofiles`.** Arguments are ignored. AI_RELOAD_PROFILES, always, the header alone. The AI module reads the
  profile files itself when it consumes it: the lines of the AI script are AI data, not console lines, so they publish
  no event of their own.
- **`burnall`.** FEATURES_REMOVE_ALL, always.
- **`burnone`.** Nothing unless the cursor's feature is below 0xfffb (there is a feature). FEATURES_REMOVE with `x` 40,
  `z` 16, `force` 1.
- **`feature <name>`.** The name is looked up in any case among the feature types the match has already loaded (a type
  that no loaded map or unit refers to is not found, and none is loaded); nothing when there is none (an absent word 1
  is the empty name). FEATURES_PLACE with `x` 40, `z` 16, `feature` its number, `flags` 0, `owner` 10, and `pos` and
  `rot` 0. The feature module decides what happens on a cell that is occupied.
- **`selectable`.** For each record of the unit array from the second (index 1) to the last in use, in array order,
  whose `flags` has the alive bit 0x10000000: UNITS_SET_FLAGS with that unit, `clear` 0, `set` 0x20.
- **A unit type name typed alone.** The word is matched, in any case, against every unit type of the match, in type
  number order (a scenario names exactly one; the matching also takes `*` and `?`, which the scenario format does not
  allow). For each match: UNITS_CREATE with `player` argument 1 (default 0), `type` the type's number, and `pos` the
  cursor position moved by the map's `SnapWorldPosToFootprint` for that type. (For a second match the position is moved
  first by the previous unit's width; no scenario has one.) The units module makes the unit finished (outcome 1) or not
  (outcome 0: the type is 0 or not usable in this match, the player holds as many of it as the type allows, or the
  player has no free unit slot). The page does not define a player that does not exist: no scenario names one.
- **`doubleshot`, `halfshot`, `radar`, `shootall`, `treedeath`.** Each toggles one bit of `uiOptionFlags`: 0x80,
  0x100, 0x200, 0x400 and 0x8. When the bit is set, PLAYERS_SET_CHEATS with `clear` the bit and `set` 0; else with
  `clear` 0 and `set` the bit. Always one event. (The tick reads bit 3 when a feature burns, bit 9 when it records the
  radar picture and bit 10 when a unit picks a target; nothing reads bits 7 and 8, but the trace keeps them, so they
  are set the same way. The word's other bits, 0, 1, 2, 4 and 6, are the view's or the platform's.)
- **`los`, `lostype`, `mapping`.** Each toggles one bit of `mapFlags`: `los` bit 1 (0x2, line of sight), `lostype` bit
  2 (0x4) and `mapping` bit 0 (0x1). LOS_RADAR_SET_MODE with `clear` the bit when it is set and `set` the bit when it
  is not; then LOS_RADAR_RECALCULATE with `reset` 0 for `los` and `lostype` and 1 for `mapping`.
- **`nowisee`.** LOS_RADAR_SET_MODE with `clear` 3 and `set` 0, then LOS_RADAR_RECALCULATE with `reset` 1, always.
- **`search [steps [scale]]`.** Nothing when argument 1 is 0 and the line does not have exactly three words. Else
  PATHFINDING_SET_SEARCH with `steps` argument 1 as an unsigned number, `hasScale` 1 when the line has exactly three
  words, else 0, and `scale` argument 2 read as a decimal number (the C `atof`), times 65536, cut to an integer when
  `hasScale` is 1, else 0. The engine consumes it: when `steps` is not 0 the search steps per tick of the block header
  (the scenario key `search_steps`) become it, and when `hasScale` is 1 and the pathfinding module has the optional
  slot `SetSearchScale`, it is called with `scale`.
- **`control <player>`.** PLAYERS_SET_CONTROL with `player` argument 1 (default 0) cut to a byte, always. Players
  applies it only when `player` is a player that exists (below 10, active, of type 1 to 3 and with index other than
  10): `localPlayer` and `playerIndex` become it (outcome 1); else nothing changes (outcome 0).
- **`view <player>`.** PLAYERS_SET_VIEW with `player` as for `control`, always. The same test where players applies
  it; only `playerIndex` becomes it.

  Both fields stay game state that the tick reads (`docs/state.md`, "Mixed cases and how to split them"): `control`
  changes the player the simulation computes as this machine's, and `view` the player whose sight decides.

### 4.3 Commands that do nothing in a scenario

- **`sharemetal`, `shareenergy`, `shareall`, `setsharemetal`, `setshareenergy`.** They act only when bit 0 of
  `netGameFlags` is set, which no test-mode run does, so in every scenario they do nothing and publish no event. Their
  events belong to the multiplayer of Milestone 3. When it is set:
  `sharemetal` and `shareenergy` flip the local player's metal or energy sharing switch, add a line to the message log
  and send the player's info to the other machines; `shareall` does both and then publishes LOS_RADAR_SHARE_MAPPING and
  LOS_RADAR_SHARE_RADAR for the local player; `setsharemetal n` and `setshareenergy n` set the local player's metal or
  energy share level to the smaller of argument 1 and the player's storage capacity of that resource, and add a line to
  the message log.

### 4.4 Commands that touch only the presentation

- **`now`.** Sets or clears bit 0x2 of `uiOptionFlags` as section 3 says. It decides which later lines run; it changes
  nothing else, and the bit is not hashed.
- **`iface <n>`.** The interface type, 0 for left-click orders and 1 for right-click orders.
- **`move <dx> <dz>`.** Moves the cursor by dx and dz cells (16 world units each), for that line only; the next
  console line puts the cursor back.
- **`drop <n>`.** Sets bit 0x1 of `uiOptionFlags` to whether argument 1 is 0 (network timeouts); not hashed.
- **`assert`, `dprint`, `mem`, `zbuffer`.** Nothing.

## 5. `spawn`

`spawn <tick> <slot> <unit type> <x> <z>` publishes one event, UNITS_CREATE, with `player` the slot, `type` the unit type's
number, and `pos` as below. The unit type name is compared in any case. A line is refused with status 2 when the game has
no unit type of that name, or when `x` is not below the map's playable width or `z` not below its playable height (both in
world units, as `edge` last set them).

The position: `pos[0]` is `x` times 65536 and `pos[2]` is `z` times 65536. The height is the map's `GetGroundHeight` at that
position (-1 off the map), raised to the sea level when it is below it; `pos[1]` is that height times 65536. Then the
map's `SnapWorldPosToFootprint` for the unit type moves the position (it changes nothing for a mobile unit type; for
another it puts the footprint on the cell grid and sets the height). The event carries the moved position.

## 6. `order`

### 6.1 The table

The kinds of the `order` line, with a `+` before the kind meaning the shift key (a queued order). The mode is the
first argument of the orders query `GetOrderType`, which says what order type a unit gets against a target for that
mode; `-` means the kind does not use a mode. "Target" says whether a unit target of the line (`<slot>:<type>:<n>`) is
given to the order.

| Order | Mode | Target | Events |
| --- | --- | --- | --- |
| `move` | 2 | yes | ORDERS_GROUP_COMMAND |
| `attack` | 3 | yes | ORDERS_GROUP_COMMAND |
| `dgun` | 4 | yes | ORDERS_GROUP_COMMAND |
| `unload` | 5 | no | ORDERS_GROUP_COMMAND |
| `load` | 6 | yes | ORDERS_GROUP_COMMAND |
| `guard` | 7 | yes | ORDERS_GROUP_COMMAND |
| `repair` | 8 | yes | ORDERS_GROUP_COMMAND |
| `patrol` | 9 | yes | ORDERS_GROUP_COMMAND |
| `teleport` | 11 | yes | ORDERS_GROUP_COMMAND |
| `reclaim` | 12 | yes | ORDERS_GROUP_COMMAND |
| `capture` | 13 | yes | ORDERS_GROUP_COMMAND |
| `build` | 14 | no | ORDERS_GROUP_BUILD |
| `stop` | - | no | ORDERS_GROUP_COMMAND |
| `cloak` | - | no | ORDERS_GROUP_COMMAND |
| `activate` | - | no | ORDERS_GROUP_COMMAND |
| `deactivate` | - | no | ORDERS_GROUP_COMMAND |
| `movestate` | - | no | ORDERS_GROUP_COMMAND |
| `firestate` | - | no | ORDERS_GROUP_COMMAND |
| `selfdestruct` | - | no | ORDERS_CANCEL, or ORDERS_GROUP_COMMAND |
| `qbuild` | - | no | ORDERS_QUEUE_BUILD |

### 6.2 The units and the events

**The units that are ordered.** The selector `<slot>:<type>:<n>` names the n-th live unit of that unit type owned by
the slot, in the order of the unit array (a unit is live when its unit type number is not 0); `<slot>:all` names every live
unit of the slot. The line is refused with status 2 when it names no unit (fewer than n live units of the type, or none),
when a unit type name is unknown, when a unit target selector names no live unit, or when the position (given, or taken
from the target unit) has `x` not below the playable width or `z` not below the playable height. The slot is the local
player and the viewing player while the order is given. The named units are the selection: they, and no other live unit of
the slot, have the selected bit 0x10 of `flags` set while the order is given (a unit that is not live keeps its bit).

**The position.** `x z` of the line, or for a unit target the target's position in whole world units (its 16.16
coordinate shifted right by 16, arithmetically). `pos[0]` is x times 65536, `pos[2]` is z times 65536, and `pos[1]` is the
ground height there raised to the sea level, times 65536, as in section 5 (no footprint snapping). A line whose kind has no
position and no target (`stop`, `cloak`, `activate`, `deactivate`, `movestate`, `firestate`, `selfdestruct`, `qbuild`)
has no position: `hasPos` 0 and `pos` 0.

**The group command.** Every kind but `qbuild` publishes one event for the whole selection, as a click of the
interface does (`selfdestruct` only when it cancels nothing, below): ORDERS_GROUP_COMMAND, or ORDERS_GROUP_BUILD for
`build`, with the payload `OrdersGroupCommand`:

- `unit` (the header's) is the first unit of the slot's range of the unit array (the player's `unitsBegin`).
- `units` is a bit set: bit `i` (bit `i % 32` of word `i / 32`) is set when the unit `i` records after `unit` is
  selected, for every unit of the slot's range. A range holds at most 500 units (the unit limit), below the 512 bits.
- `mode` is the kind's mode of section 6.1 (0 for a kind with none, and for `build`); `type` is the order type of a kind
  with no mode (below) and 0 for one with a mode.
- `queue` is 1 when the line has a `+` and else 0, for every kind but `build` (0, the `+` changes nothing for it).
- `target` is the line's unit target (0 for none), whether or not the kind passes it on; 0 for `build`.
- `hasPos` and `pos` are the position above (`hasPos` 0 and `pos` 0 for a kind with none; for `build`, the site below).
- `params` is 0 and 0, but for `movestate`, `firestate` and `build` (below).

The orders module consumes the command with its interface slot `ExpandGroupCommand`, which publishes an ORDERS_COMMAND
for each unit it orders to the orders module's own outbox; each is delivered at once (section 1), in publication order,
and the command's outcome is their count. A mod may replace the slot (a formation); the default is 3.1c's rule, which
the rest of this section states as the ORDERS_COMMAND events it publishes, and with it the traces equal the stored ones.

**The kinds with a mode** (`move`, `attack`, `dgun`, `unload`, `load`, `guard`, `repair`, `patrol`, `teleport`,
`reclaim`, `capture`). Let the *target* be the line's unit target when the mode takes one (every mode but 5, 10 and
14) and the line has one, else none. The target is not ordered itself: it is left out of the selection. Let `n` be
the number of selected units but the target. The expansion publishes nothing when `n` is 0. Else, for each such unit `u`
in array order:

1. `type` is `GetOrderType(mode, u, target, position)`. Nothing for `u` when it is 0.
2. Nothing for `u` when `type` is the order type "Standing_FireOrder" and the unit type of `u` lacks the data flag
   `firestandorders` (bit 1 of `flags2` of the unit type), or when it is "Standing_MoveOrder" and the unit type lacks
   `mobilestandorders` (bit 0 of `flags2`). (Only `movestate` and `firestate` reach this.)
3. The position is the one above, with the formation spread when the order type's table flags have bit 2 (the order
   types "Move_Ground", "Patrol", "RepairPatrol", "VTOL_Move", "VTOL_Patrol" and "VTOL_RepairPatrol") and the line has a
   position:
   - `cx` is the sum over the `n` units of their x in whole world units (the 16.16 `pos.x` shifted right by 16
     arithmetically and cut to a signed 16-bit integer), divided by `n`, truncating toward 0; `cz` likewise with z.
     The centre is `cx * 65536`, `cz * 65536`.
   - With `dx = u.x - centre.x` and `dz = u.z - centre.z` (32-bit integers, 16.16, wrapping), and `s = (dx*dx >> 32) +
     (dz*dz >> 32)` (each square in 64 bits, shifted arithmetically, then cut to 32 bits), when `s <= n * 3000` the
     event's position is `pos[0] + dx`, `pos[1]`, `pos[2] + dz` (32-bit, wrapping); otherwise it is the position unmoved.
4. ORDERS_COMMAND with `unit` the unit, `type`, `queue` 1 when the line has a `+` and else 0, `target` the target unit (0
   for none), `hasPos` 1 and `pos` as step 3 (every kind with a mode has a position), `params` 0 and 0.

The target of a mode that does not take one (5, 14) is never an event target; an `unload` whose line names a unit
uses only its position.

**The kinds with no mode.** The units are the selection, `n` is their number, and the expansion publishes for each in
array order one ORDERS_COMMAND with `unit`, `queue` as above, `target` 0, `hasPos` 0 and `pos` 0, and
- `stop`: `type` is `MissionTypeFromName("STOP")`, `params` 0 and 0.
- `cloak on`: "CLOAK_ON"; `cloak off`: "CLOAK_OFF"; `activate`: "ACTIVATE"; `deactivate`: "DEACTIVATE"; `params` 0 and 0.
- `movestate n`: "STANDING_MOVEORDER", `params[0]` n (0, 1 or 2), `params[1]` 0; skipped for a unit as step 2 says.
- `firestate n`: "STANDING_FIREORDER", the same, with step 2's test.

**`selfdestruct`** toggles. In the order of the unit array, for each selected unit that already has an order of the type
`MissionTypeFromName("SELFDESTRUCT")` in its order list: ORDERS_CANCEL with `unit` and `type` that type. When no selected
unit had one, nothing was cancelled, and the line publishes ORDERS_GROUP_COMMAND with `type` that type, as the kinds
with no mode do: each selected unit gets an ORDERS_COMMAND of that type, `params` 0 and 0 (and `queue` from the `+`).

**`qbuild <unit type> [n]`.** For each selected unit in array order, ORDERS_QUEUE_BUILD with `unit`, `count` n (default 1,
1 to 99) and `name` the unit type as written in the line (at most 31 characters, zero-terminated). The name must be a unit type
of the game, except that `MAKENUKE` and `MAKEANTI`, in exactly that case, are passed as they are. A `+` changes nothing.

**`build <unit type> x z`.** The unit type must exist (else status 2). Let `fx` and `fz` be its `footprintx` and `footprintz`, and the
cursor's world position `X Z` the line's `x z` times 65536. The site's cell is `cx = (X - (fx << 19) + 0x80000) >> 20` and `cz =
(Z - (fz << 19) + 0x80000) >> 20` (arithmetic shifts, cut to signed 16 bits). The site is refused with status 2 when
the map's `CanBuildAt` for that unit type, that cell, type 0 and the ordering player answers 0 (the site is off the map,
blocked, too steep, not visible or not explored to that player, and the other rules of that slot). Else the site's
position is `pos[0] = (fx + cx*2) << 19`, `pos[2] = (fz + cz*2) << 19` and `pos[1]` the height that `CanBuildAt` left in its
placement record (the lowest ground height under the cells of the footprint that the yard map marks as ground level, or the
sea level minus the unit type's `draft` when it has none) times 65536. The line publishes ORDERS_GROUP_BUILD with `queue`
0, `target` 0, `hasPos` 1, `pos` the site, `params[0]` the unit type's number and `params[1]` 0. Its expansion publishes,
for each selected unit in array order whose unit type has bit 0x40 of `flags1` (builder): ORDERS_COMMAND with `unit`,
`type` `MissionTypeFromName("MOBILEBUILD")` when bit 0x800 of its unit type's `flags1` is clear, else
`MissionTypeFromName("VTOL_MOBILEBUILD")` when the unit type is `flying` (nothing otherwise), `queue` 0, `target` 0,
`hasPos` 1, `pos` the site, `params[0]` the unit type's number and `params[1]` 0. A selected unit that is not a builder
gets nothing.

## 7. The scenarios of the core's verdict

A core is judged on the scenarios marked `judged`: it turns each one's lines into events by this page, and its traces must
equal the stored ones. A scenario is `left out` when it needs something the core does not have: the presentation (its
`input` lines), saving and loading (not in the core spec), or a console command of the group `direct`, which is not an
event.
`tools/commands.py --check` keeps this table equal to the one of `reference/README.md` and refuses a judged scenario with
one of those, and a scenario that uses a console command or an order kind that this page does not list.

| Scenario | Verdict | Reason |
| --- | --- | --- |
| ai-antinuke-intercept | judged | - |
| allied-victory | judged | - |
| combat-deaths | judged | - |
| commander-death | judged | - |
| conditions-dark-side | judged | - |
| conditions-shore-to-shore | judged | - |
| console-economy | judged | - |
| console-edge | judged | - |
| console-features | judged | - |
| console-ilose | judged | - |
| console-iwin | judged | - |
| console-sight | judged | - |
| console-units | judged | - |
| display-chat-dither | left out | has `input` lines: it needs the presentation |
| display-debug-overlay | left out | has `input` lines: it needs the presentation |
| display-tab-menu | left out | has `input` lines: it needs the presentation |
| duel-coast-to-coast | judged | - |
| duel-great-divide | judged | - |
| duel-pass-dark | judged | - |
| duel-shore-to-shore | judged | - |
| edges-economy-targeting | judged | - |
| edges-targeting-lead | judged | - |
| eight-full-moon | judged | - |
| eight-greenhaven | judged | - |
| eight-pincushion-teams | judged | - |
| frontend-skirmish | left out | has `input` lines: it needs the presentation, and the menus set up its match |
| hailstorm-los-off | judged | - |
| load-midgame | left out | saves or loads a game: save and load are not in the core spec |
| load-targets | left out | saves or loads a game: save and load are not in the core spec |
| long-great-divide-2p | judged | - |
| long-greenhaven-8p | judged | - |
| los-off-mapped-ai | judged | - |
| meteors-earthquake | judged | - |
| meteors-hailstorm | judged | - |
| mission-build-unit | judged | - |
| mission-capture | judged | - |
| mission-defeat | judged | - |
| mission-defeat-all-units | judged | - |
| mission-defeat-of-type | judged | - |
| mission-defeat-passes-x | judged | - |
| mission-kill-mobile | judged | - |
| mission-kill-of-type | judged | - |
| mission-passes-x | judged | - |
| mission-passes-z | judged | - |
| mission-radius | judged | - |
| mission-timer | judged | - |
| mission-victory | judged | - |
| orders-air | judged | - |
| orders-air-fights | judged | - |
| orders-air-repair | judged | - |
| orders-attack-nomove | judged | - |
| orders-attack-nomove-range | judged | - |
| orders-build-patrol | judged | - |
| orders-factory | judged | - |
| orders-guard-attacked | judged | - |
| orders-guard-reclaim | judged | - |
| orders-immediate | judged | - |
| orders-move-attack | judged | - |
| orders-reclaim-feature | judged | - |
| orders-tier2-core | judged | - |
| orders-tier2-spider | judged | - |
| orders-tier2-vader | judged | - |
| orders-weapon-build | judged | - |
| quad-fox-holes | judged | - |
| quad-king-of-the-hill | judged | - |
| quad-lava-highground | judged | - |
| quad-two-continents | judged | - |
| save-midgame | left out | saves or loads a game: save and load are not in the core spec |
| save-targets | left out | saves or loads a game: save and load are not in the core spec |
| sensors-floating-teeth | judged | - |
| sensors-floating-teeth-los-off | judged | - |
| sensors-los-off | judged | - |
| sensors-sonar-jam | judged | - |
| share-all | judged | - |
| share-energy | judged | - |
| share-metal | judged | - |
| six-hundred-isles | judged | - |
| six-painted-desert | judged | - |
| six-yerrot-mountains | judged | - |
| skirmish2 | judged | - |
| skirmish4 | judged | - |
| skirmish4-long | judged | - |
| ten-lava-run | judged | - |
| ten-ring-atoll | judged | - |
| trio-anteer-strait | judged | - |
| trio-dark-side | judged | - |
| trio-etorrep-glacier | judged | - |
| ui-antinuke-range | left out | has `input` lines: it needs the presentation |
| ui-build-menu | left out | has `input` lines: it needs the presentation |
| ui-build-place | left out | has `input` lines: it needs the presentation |
| ui-click-select | left out | has `input` lines: it needs the presentation |
| ui-debug-keys | left out | has `input` lines: it needs the presentation |
| ui-dialogs | left out | has `input` lines: it needs the presentation |
| ui-drag-select | left out | has `input` lines: it needs the presentation |
| ui-groups | left out | has `input` lines: it needs the presentation |
| ui-keys | left out | has `input` lines: it needs the presentation |
| ui-left-click-orders | left out | has `input` lines: it needs the presentation |
| ui-right-click | left out | has `input` lines: it needs the presentation |
| ui-scroll-groups | left out | has `input` lines: it needs the presentation |
| ui-share-chat | left out | has `input` lines: it needs the presentation |
| units-transport | judged | - |
| units-transport-ship | judged | - |

## 8. The interface's commands

The interface changes the simulation during play only by publishing events to the engine's outbox, as a command does,
and the rule of delivery of section 1 holds for them: the interface runs while no tick does, so each event is delivered
at once. These are the events it publishes, besides the group commands of a click (section 6.2):

- **A click on a unit type of the build menu**, for the unit the menu shows when it is selected: ORDERS_QUEUE_BUILD
  with that unit, `count` 1 (5 with the shift key) for a left click and minus that for a right click, and `name` the
  button's name.
- **The quit dialog**, its first choice when the player asked to leave the game to the desktop: PLAYERS_LEAVE_GAME. The
  other choices of that dialog leave the battle screen and change nothing the simulation reads.
- **The dialog a defeated player gets**, its second choice (stop watching): PLAYERS_LEAVE_GAME. The first choice
  (keep watching) changes nothing the simulation reads.
- **The debug hotkeys**, with debug mode on: F11 turns it on when bit 1 of `uiOptionFlags` is set, which 3.1c sets
  only from the registry (a scenario sets it with `debug_keys 1`, docs/scenario-format.md). The key that fills every
  player's stores publishes, for each player that exists in slot order, PLAYERS_SET_STORE with `resource` 0, `add` 0 and
  `amount` the player's energy capacity, then the same with `resource` 1 and its metal capacity; the key that makes the
  unit under the cursor forget its attacker publishes UNITS_FORGET_ATTACKER with that unit, when there is one.
- **The share dialog** (game type 3, the `h` key), its OK button with a player chosen in the list, when that player is
  in the game and alive: PLAYERS_TRANSFER from the local player to that player with `resource` 1 and `amount` the metal
  slider, then with `resource` 0 and the energy slider, both with `announce` 1; then, when the units button is on,
  PLAYERS_GIVE_UNIT with that player and no record for each selected unit that is not a commander, not being built and
  not carried or carrying; then, when the map button is on, LOS_RADAR_SHARE_MAP from the local player to that player.
- **The chat line** (Enter): a line that starts with `+` runs as a console line (section 3) and publishes what that
  command publishes (section 4) as the interface's events; any other line is a chat message and publishes nothing.
- **The score panel**, on a frame it draws, when its pass over its own copy of the players' ranks closed a gap:
  PLAYERS_COMPACT_RANKS (`docs/players-interface.md`, section 2, "The ranks"). No scenario shows the panel yet.

The interface's events, the paths that publish them and the scenarios whose stored logs
(`reference/commands/<scenario>.txt`) hold them. `tools/commands.py --check` fails when an event of the first column is
in no stored log, or a stored log holds an interface event the table does not list.

| Interface event | Paths | Scenarios |
| --- | --- | --- |
| `AI_SET_SQUAD` | a control group key (Ctrl+digit) | ui-groups, ui-scroll-groups |
| `LOS_RADAR_SHARE_MAP` | the share dialog | ui-share-chat |
| `ORDERS_CANCEL` | the self-destruct key on a unit that is counting down | ui-keys |
| `ORDERS_GROUP_BUILD` | a build site placed from the build menu | ui-build-place |
| `ORDERS_GROUP_COMMAND` | a click order, a command key and a click, a minimap order, the self-destruct key | ui-dialogs, ui-groups, ui-keys, ui-left-click-orders, ui-right-click, ui-scroll-groups |
| `ORDERS_QUEUE_BUILD` | the build menu of a factory | ui-build-menu, ui-scroll-groups |
| `PLAYERS_COMPACT_RANKS` | the score panel, on a frame it draws | none |
| `PLAYERS_GIVE_UNIT` | the share dialog | ui-share-chat |
| `PLAYERS_LEAVE_GAME` | the quit dialog, the defeated player's dialog | ui-dialogs |
| `PLAYERS_SET_STORE` | the debug hotkey `=` | ui-debug-keys |
| `PLAYERS_TRANSFER` | the share dialog | ui-share-chat |
| `UNITS_FORGET_ATTACKER` | the debug hotkey `]` | ui-debug-keys |

The pause request (bit 0 of `otherFlags`) is the driver's: it decides whether ticks run, and the tick reads it only in
the network's timeout check, which runs on the wall clock. The bit of `mapFlags` that the camera clears (bit 3) says
whether the drawn fog of war is current: only drawing reads it, and the trace leaves it out. Neither is an event.
