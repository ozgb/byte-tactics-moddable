# Replay recordings

A recording holds the calls one subsystem received in a test-mode run: the state each call could read, the
imports it made with their arguments and results, and the state it changed. `bt-replay 2` is the format.
Recordings are in `tests/replay/<subsystem>/<scenario>.txt`; wind_meteors, targeting, map, weapons, economy, players, features, mission, units, ai, los_radar and pathfinding have recordings so far. The tables behind the format are `data/replay_schema.json`
(`uv run tools/replayschema.py` writes it, and the generated sources, from the interface headers,
`data/subsystems.csv`, `data/rwsets.csv` and the annotations the file itself holds).

A recording does not depend on the module mode: the same file comes from `-modules=wasm` and `-modules=native`.

## Switching it on

A scenario line `record <subsystem>` makes the run write `replay-<subsystem>.txt` when it ends without
failure. `record <subsystem> <first tick> <last tick>` records only the calls whose tick is in the window.
Every simulation subsystem (the members of `Seams`) is accepted, and a scenario may carry several `record` lines,
one per subsystem, each with its own file and window. `tools/replay.py` adds the lines to copies of the
scenarios; `reference/scenarios/` never holds them. A stored recording that covers only a window is listed in
`tests/replay/windows.json` (`"<subsystem>/<scenario>": [first, last]`), and `--check` records it with that window.

```sh
uv run tools/replay.py --record --exe build/modern/TotalA.exe --out DIR [--subsystem NAME]... [--native-modules] [SCENARIO...]
uv run tools/replay.py --check --exe build/modern/TotalA.exe [--subsystem NAME]... [--native-modules] [SCENARIO...]
uv run tools/replay.py --compare DIR_A DIR_B
```

`--subsystem` is repeatable and defaults to every subsystem that has a `tests/replay/<subsystem>/` directory.
`--check` records again and fails when a recording of `tests/replay/` differs. `--compare` fails when two
directories differ and names the first differing line. Record on a fresh Wine prefix, as the gate runs.
Both play a scenario once for each subsystem it records: a run records one call at a time, so with two `record`
lines a call one subsystem makes into the other would be an import of the first and missing from the second's
recording. A run fails instead, with "a call of a recorded subsystem made inside a call of another recorded
subsystem is not recorded", when such a call happens.

## A play-test recording

A subsystem that only a network game reaches (`UpdateResourceSharing` returns unless `Game.netGameFlags` bit 0 is set, and a
transfer needs a remote partner) is recorded in a play-test instead of a scenario. A normal run (no `-testmode`) of an exe built
from this tree takes the switch `-record <subsystem>[:<first tick>:<last tick>]`, which does what the `record` line does (the
window is optional) and writes `replay-<subsystem>.txt` when the game exits through its main loop. A run without the switch is
unchanged. The play-test passes it to instance 1:

```sh
uv run tools/playtest.py --exe build/modern/TotalA.exe --scenario mp --record <subsystem>[:FIRST:LAST] --game-dir DIR
```

`tools/playtest.py` copies the file to the run's directory (`build/playtest/<session>/mp-modern/replay-<subsystem>.txt`), and it is
committed under `tests/replay/<subsystem>/` like any recording. A play-test has no reference trace to show that the game ran as 3.1c
would, and `tools/replay.py --check` cannot play it again, so `tests/replay/playtests.json` lists each one
(`"<subsystem>/<name>": "<the command that records it>"`) and `--check`, the default plan and `tools/oraclegate.py` leave it out.
CI replays it with the others (`tools/ci_replays.sh`). No play-test recording is stored at present: the one of economy went with its 3.0 interface.

## Lines

Text, LF line endings. The header is two lines: `bt-replay 2` and `subsystem wind_meteors 2.0` (the interface
version). A subsystem whose annotations are not complete yet (`status` in the schema is `pending`) writes
`incomplete` after the version, and the replayer refuses such a file. The file name carries the scenario.

A call is a line `call <ticks> <Slot>`, where `ticks` is `Game.ticks` when the call starts (`-` before the
game record exists) and `Slot` is a slot of the interface. Calls are in the order they start. A slot called by
another slot of the same subsystem (UpdateWind calls its helper for a stream in a native build) is part of the outer
call and has no line of its own. The lines of a call, indented by two spaces, in this order:

| Line | Meaning |
|---|---|
| `r <field> <value>` | a field of the slot's read set that differs from the last known value of that field |
| `a <name> <value>` | an argument of the slot that is not in the block (`params.radius`, `bank`, `args[0]`, `stream`, a scalar, `player`, a callback `done callback#3`) |
| `i <import> <arguments> = <result>` | one import call, in call order; its index is its position among the `i` lines of the call |
| `c <handle>#<n>.<Method> <arguments> [= <result>]` | indented by four spaces, right after an `i` line and before its `o` lines: one call the import made on a callback object it was given, in call order (see Callbacks) |
| `c <handle>#<n>.<Method> <arguments> [= <result>]` | indented by two spaces, after the `i` lines and before the closing `o` lines: one call the module made during the slot on a callback object it holds or was given, in call order (see Callbacks, "A callback held in state") |
| `o <name> <value>` | right after an `i` line: a value the import wrote through a pointer argument (`o dst "text"`, `o dst x:0102`) or reached through its result (`o weapondef#17.flags 0x00000001`, `o Cell#1.metal 12`) |
| `w <field> <value>` | a field of the slot's write set that differs from the last known value after the call |
| `o <name> <value>` | last in the call: a value the slot wrote through its own pointer argument, or `o result <value>` for its result |

A call without any such line is idle. The last known value of a field is the one on the latest `r` or `w` line
of any earlier call, or the call's own tick for `Game.ticks`. A field with no known value counts as different.
The known values start empty at the first recorded call.

An `o` line right after an `i` line (and after the `c` lines of that import) is the import's, until the first `o` line whose name begins with one of the
slot's closing names, and every `o` line from there on is closing. The closing names of a slot are the names it records for what it writes
through its own arguments and for its return value: `result`, and the name of each out argument (`pos.` for the out `Vec3` `pos`, which writes
`o pos.x`, `o pos.y` and `o pos.z`), and any `snapshot#` line. The generated runner gives them to the replayer with `SetClosing` (from
the schema entry of the slot), and the replayer compares the beginning of the name after `o `, so an import's own names (`out`, `dst`, `Unit#251.flags`)
never end its lines early, and a slot with no closing name has no closing `o` line. The recorder writes the closing lines last, so the last
import's `o` lines can be followed at once by closing ones:

```
  i los_radar.GetVisibleEnemiesInRadius 0 pos=(36700160,65536,15728640) 290 0
  o out []
  o result null
```

Here `o out []` is the import's and `o result null` is the slot's.

An out struct recorded `"when": "result != 0"` (`pos` of `GetWeaponTargetPos`) is written, and compared by the replayer, only when the result is nonzero: a call that returns 0
has no `o pos.*` line, the replayer's `Vec3` is zeroed before the call, and whatever the slot leaves in it is not compared (the result is).

## Objects

Game objects that a subsystem reaches through pointers are written as `Kind#N`: `Player#1`, `Unit#3`,
`UnitDef#2`. A field of one is `Kind#N.field`, and it appears in `r`, `w` and `o` lines exactly as a block field
does, with the same last-known-value rule (the known value is kept per `Kind#N.field`). The kinds, their fields and
their value forms are the `objects` of the schema:

| Scope | Kind | `N` | Where it lives |
|---|---|---|---|
| state | a kind with no `table` that is linked by a `Ref` | order of first appearance in the file, so it is stable across calls | in the block, linked by `Ref` |
| state, `table` | `Player`, `Unit`, `UnitDef`, `Projectile`, `MovementClass`, `PassMap`, `Cell`, `Feature`, `Spot`, `Pool` | the index in the table: `Player#3` is `Game.players[3]`, `Unit#17` is slot 17 of the unit pool, `UnitDef#2` is `Game.defs[2]`, `Projectile#3` is `Game.projectiles[3]`, `Cell#2051` is `Game.cells[2051]`, that is `y*width+x`, `Feature#5` is `Game.features[5]`, `Spot#9` is `Game.spots[9]` and `Pool#0` is `Game.pool` | an array in `Game` or `SimGlobals`, or a `Ref` to one |
| state, `inside` | `Handle` of a feature or a spot | the site, the container and the member: `Feature#5.anim`, `Spot#9.shadow` | a member of another kind (see An object embedded at several sites) |
| index | `weapondef` | the index in `Game.weapons` | in the block's weapon array |
| value | `MeteorShowerParams`, `Vec3` | none: the lines are written with the argument's name, `params.radius`, `pos.x` | an argument struct of one call |
| handle | `file`, `rec`, `bank` | order of first use in the outermost call | the import's own objects, not state |

A kind with a `table` is named by its index in a table that `Game` or `SimGlobals` holds, because the module walks it by
address (a loop from a begin pointer to an end pointer) and reaches its elements by pointer arithmetic (a cell pointer minus the table's base). The index
is zero-based and the same in every recording, wherever an element is first named. The replayer finds `Unit#N` at `N` times
the unit stride (0x118) in the pool, so a slice of the table is contiguous in its block as it was in the game's. Numbering by
first appearance could not rebuild a slice when one of its units had been named earlier as an argument. A pointer into the
table that is not at the start of an element, or past its count, is written `Kind#?`.

The annotation names the table once per kind, in the kind's `table`:

```json
"Projectile": {"type": "Projectile", "scope": "state",
               "table": {"base": "game", "member": "projectiles", "ref": true, "count": "projectileCount", "alloc": 1024}, "fields": []}
```

| Key | Meaning |
|---|---|
| `base` | `game` (a member of `Game`) or `sim` (a member of `SimGlobals`) |
| `member` | the member that is the table, or holds it; it may be a path, `g_movementClasses.entries` |
| `ref` | the member is a `Ref` to the first element; without it the elements are the member |
| `count` | the number of elements: an integer, or a length expression (see Arrays) over members of the base, `poolCount` |
| `alloc` | for a `ref` table: the elements the replayer makes, and stores in the count member, when a recording first names an element and the `Ref` is null; `"module"` for a table with a `capacity` that the module frees (see A table with a capacity) |
| `capacity` | the elements there is room for, when `count` is the live part of them: an integer, or a length expression over members of the base (see A table with a capacity) |

A `ref` table whose `count` is a product of two block fields (`Cell` is `width*height`) has no `alloc`. The replayer allocates the
product of the fields' current values and stores nothing, since neither factor is the count. The fields must therefore be set before
the first element is named: `Game.width` and `Game.height` are in the read set of every slot that reaches a `Cell`, so their `r`
lines come before the first `Cell#N`. A slot's annotation adds a field to its read set with `"reads": ["Game.width", "Game.height"]`,
and `replayschema.py --check` reports a complete subsystem's slot that reaches such a kind with a factor missing from its read set.

`UnitDef` is a `Ref` table over `defs` with `unitDefCount` and `alloc` 1024, and `UnitDef#N` is the index in `Game.defs`, the same in every recording.
The replayer takes a table's `alloc` elements from the sim block (`SimAlloc`), so `alloc` times the element size must fit in the block,
which is 32 MiB unless the match setup asks for more. A `UnitDef` is 585 bytes: 1024 of them take 599,040 bytes,
while 65536 would take 38,338,560, more than the block, and the replayer would stop with
`sim block: out of memory, the match setup's block size is too small`. 3.1c has 279 definitions; a mod whose recordings name
`UnitDef#1024` or higher needs `alloc` raised, and past about 57,000 definitions the block as well.

`Player` and `Unit` are instances of this form (`players` is `{"base": "game", "member": "players", "count": 10}`, `units` is a
`Ref` with `poolCount` and `alloc` 5001), so the text of every committed recording is the same as before the form had a name.

### Several all-of tables in one slot

A rule with `"rule": "all-of"` makes every element of a table kind a root of the call. A slot may have such a rule for several kinds: the
walk reaches every element of each, in the order of the kinds in the schema, and the slot lists them as `all-of` in the generated
`data/replay_schema.json`. Each kind is captured, compared and restored as one kind is. A slot with one all-of kind records the same
text as before.

```json
{"kind": "field_escape", "target": "Game.players", "rule": "all-of", "object": "Player"},
{"kind": "field_escape", "target": "Game.lanes", "rule": "all-of", "object": "FixtureLane"}
```

```
  r Player#2.id 12
  r FixtureLane#2.level 9
```

`replayschema.py --check` fails for an all-of rule whose `object` is not a kind with a `table` of `Game` or `SimGlobals`.

### All-of over a table behind a ref field

A rule with `"rule": "all-of"` may name a kind whose table is behind a ref field of another kind (`MissionRule`, the array `Mission.rules` leads to). The walk then
reaches every element of every owner it has named, up to the table's count or capacity (`Mission#0.rules[3].type`), and the owner's all-of rule must come first, so the owner
is a kind before the table's kind in the schema. A slot that loops over such an array by index (`Mission::CountStartPositions`) needs it: a ref field alone leads the walk to the first element.

### A table recorded as changed elements

A kind with a `table` may also carry `"delta": true`. A slot that is all-of over a delta kind does not put its elements in the walk's
list of objects, which holds 8192, so the table may be as large as the game's (`Game.cells` has `width*height` cells). The walk
follows the pointer and unit-id fields of each element, and capture and comparison visit the table element by element. The recorder
writes every field of the slot's sets for every element at the first record, and after that only the fields whose value changed since
the last line that named them, in the object form. An element that the walk also reaches through a ref with a limit (a `reach` list) is recorded and compared whole, without the limit. This is #689's changed runs for a table of structs:

```json
"FixtureCell": {"type": "FixtureCell", "scope": "state", "delta": true,
                "table": {"base": "game", "member": "p1b8a", "ref": true, "count": 9000, "alloc": 9000}, "fields": []}
```

```
  r FixtureCell#0+9000.unit 0*40 3 0*8959
  r FixtureCell#0+9000.metal 0*500 9 0*8499
  r FixtureCell#7000.metal 5
  r FixtureCell#12.flags 258
```

A read line names a run of elements when the field is a number (an integer, flags or an `f32`): `Kind#first+count.field` stands for
the same field of the `count` elements from `first` on, and its value is their values in order, a value that repeats `n` times written
`value*n`. The recorder writes the elements whose field changed (at the first record, every element) field by field, one line for each run
of neighbours, and a run of one element in the plain form. A line ends once its values pass 1,000 characters, and the run goes on in the next
line. Write lines, out lines and the fields that are not numbers keep one line for each element; a recording that names every
element in the plain form replays as before.

The first two lines are two of the three of the first record (one field of 9000 cells each); the last two are all of the second. A delta kind
is a state kind with a table, and no `inside`, `kinds` or `vtable`. An element that the walk also reached by a pointer is recorded once.
A kind that is not a delta, or a slot that is not all-of over it, behaves as before.
The table member is not an untracked target: a row that touches it needs no `exempt` entry.

A pointer field is written as the object it points at (`Unit#1`) or `null`. The reachable objects of a call are the
object arguments (`a player Player#1`) and every object their pointer fields lead to, following the pointer fields
that are in the slot's read or write set for the kind. At entry the read-set fields of each reachable object are
written as `r` lines, at exit the write-set fields as `w` lines. The walk order (arguments, then breadth first, fields
in table order) numbers the objects named by first appearance, and the replayer walks its own block the same way. The replayer allocates
`Kind#N` in the block when a line first names it, stores `Ref` pointers as offsets from the block base, and reports a
changed field at its `Kind#N.field`. A pointer to an object no earlier line named is written `Kind#?`.

A field of kind `ptr` is a raw pointer, not a `Ref`, to an object of a state kind: `{"name": "src", "kind": "ptr", "target": "FrameTable"}`
for `Handle.src`. It is written exactly as a `ref` is, `FrameTable#N` or `null`, but the replayer stores the address of its own copy
of the object, where a `ref` stores an offset from the block base. The walk does not follow a `ptr`, so the object it points at is
written only where something else reaches it (an argument, a result).

| Kind | Stored as | Written as |
|---|---|---|
| `ref` | an offset from the block base | `Unit#1`, `null` |
| `ptr` | the address of an object | `FrameTable#1`, `null` |

### A pointer to an object no line named

A `ptr` whose target no earlier line named gets the next `Kind#N` of its kind when it is first written, and the walk still does not
reach the target. A pointer set before the recording starts (a cursor's `src`, set when the game loaded the file) is therefore
`FrameTable#1` at the first call that records it, and no line carries the fields of `FrameTable#1`. The replayer makes an empty
object of the kind (every byte zero, as for a `ref` target) when a line first names it, and stores its address; two pointers to
one object have the same name and the same copy. What the module needs from such an object is recorded as the `o` lines of the import that answers for it. A pointer that no recorded line names is `FrameTable#?`, and restoring it is an error: the
replayer reports it as a line that cannot be restored.

```
  r Handle#1.src FrameTable#1
  r Handle#2.src FrameTable#1
```

### A condition on a field

A field may carry a `when`, `{"field": F, "mask": M, "equals": V}`: it is live only when `(F & M) == V`, where `F` is a field of
the same kind that holds an integer or flags (`flags16`, `kind`), and `mask` is optional and means every bit. The walk follows a
field that is not live no further, and capture, comparison and restore skip it: it is not written as a line, not compared and not
restored. This is for a union, whose live view depends on a bit, where restoring the dead view would overwrite the bytes of the live
one. A field that goes dead is forgotten, so when it is live again its line is written again even if its value is the one it had before,
because the replayer's bytes are by then those of the other view.

`"via": FIELD` reads `F` from another object: the one a `ref` or `ptr` field `FIELD` of the same kind points at, or, for an integer
field, the element it indexes in the table of the kind named by `"kind"`. An index outside the table, or a null pointer, makes the
condition false. A `unitid` field may have a condition too, which says that its bytes are an id (a value of `-32768` in the next slot),
and its field is not written as a line. A `unitid` field with no `when` holds an id whenever its value is not 0: `{"name": "occupant",
"kind": "unitid", "target": "Unit", "member": "unit", "reach": ["Unit.health"], "depth": 1}` leads the walk from a cell to
the unit that stands on it (`r Unit#5.health 4`), and an id of 0, or one past the unit pool, leads nowhere. The bytes themselves are
recorded by a field of an integer kind over the same `member` (`unit`).

### A unit reached by its id, per slot

The `reach` of a `unitid` field is the field's, shared by every slot whose walk crosses it. A slot that reads more of the unit than that
names its own reach with `by_id` in its annotation, a list of entries that each give `fields` (`unitid` fields, `Kind.field`), a `reach`
and its `depth`, checked as a field's `reach` is against the field's target:

```json
"by_id": [{"fields": ["Cell.occupant", "Cell.occupant2"], "reach": ["Unit.flags", "Unit.health", "Unit.def", "UnitDef.maxHealth"], "depth": 2}]
```

In that slot's walk a unit reached through one of the `fields` records the slot's `reach` instead of the field's, and the walk follows only
its pointers; every other slot keeps the field's. So a slot reaches the units of every cell of the map by their id without reaching all of
each one: weapons' collision and area damage read the cells around a shot, every cell a root of the call (`all-of`), and the units that stand
on them by id, and their `reach` leaves out `Unit.aimCob` (the three aim callbacks of each unit) that a walk of every unit would hold. A unit
that the walk reaches through an argument or a `ref` is recorded with the slot's sets in full, as before, wherever its id comes in the walk:
in a slot with `by_id` the units named by the elements of a table recorded as changed elements (`all-of`) are visited only after the rest
of the walk, so a unit that a shot names (`Projectile.attacker`) keeps every field the slot reads. A unit reached only by its id keeps the
limit, so a `reach` names every field of such a unit the slot reads. The generated JSON
gives the slot a `byId` list of `{"fields", "reach", "depth"}`. `replayschema.py --check` fails for an entry with no `fields`, a field that
is no `unitid` field or is named twice in the slot, an unknown key, and a `reach` that a field's `reach` would refuse.

```json
{"name": "scale", "kind": "f32", "member": "linked.scale", "when": {"field": "flags", "mask": 1, "equals": 1}},
{"name": "moved", "kind": "i32", "member": "ground.moved", "when": {"via": "player", "kind": "Player", "field": "flags", "mask": 4, "equals": 4}}
```

The replayer restores the `r` lines of a call in two passes: first the lines of the fields that a condition reads (`F` and the `via`
field, of every object), then the rest. A guarded field whose line comes before the line that decides it, in the same object or in
another one, is therefore restored from the call's own values, not from the previous call's. The object a `via` reads is recorded only
when the walk reaches it, and the field it reads only when it is in the slot's read set, so a rule of the slot adds them
(`fields`, with `reads`) when the walk would not.

### An object embedded at several sites

A kind that lives inside another kind lists its sites, `"inside": [{"kind": "Spot", "member": "anim"}, {"kind": "Feature", "member": "anim"}]`,
and each site is an `embed` field of its container (`{"name": "anim", "kind": "embed", "target": "Handle"}`). An embedded object is named
by its site, `Spot#3.anim`, in every line that names it: the walk's `r` and `w` lines (`r Spot#3.anim.index 2`), the `i` and `o` lines of a service
that takes it (`i services.<step cursor> Spot#3.anim`, `o Spot#3.anim.value 7`) and the argument values. The replayer resolves such a
name to the bytes at the site inside its container, and never allocates a separate object. A free-standing object of the same kind, one on a
caller's stack, is named by order as any other, `Handle#1`.

A container named by order (not a table kind) is named before its embedded object is: the walk names it, as it reaches the container first.

```
  r Spot#3.anim.index 2
  i services.<step cursor> Spot#3.anim = 1
  o Spot#3.anim.value 7
  i services.<step cursor> Handle#1 = 1
```

### Features, spots and the pool

`Feature` is the table `Game.features`: `Feature#N` is the feature type `N`, and it has `featureCount` elements, the size of the array the loader grows by one type at a time. `Spot` is the 0x800 entries of `Game.spots`, 0x30 bytes each, and `Pool` is the five words of `Game.pool` (`entries`, `featureUnit` and the heads of the used, rest and free lists, which link the spots by their `prev` and `next` indexes). `Game.spots` is the same word as `Pool.entries`.

Both structs are unions of views, and a field is recorded on one side of a union only:

| Kind | Bytes | Written as |
|---|---|---|
| `Feature` | `object` and `filename` share their first four bytes | `object` (a `ptr` to an `Object3do`) when bit 0 of `flags16` is clear, `filename` when it is set; both fields have a `when` on that bit |
| `Feature` | `flags16`, with `flags`, `flagsHigh`, `steep`, `noobject`, `animating` and the other bits | one `u16` field; the other names are `aliases` of it |
| `Feature` | `seq`, `seqshad`, and the six pointers that `seqburn` to `seqreclamateshad`, `burn`, `death` and `animIds` view | `ptr` fields to a `FrameTable`, the kind of an animation entry; `animTable`, `shadowTable`, `burn`, `death` and `animIds` are `aliases` |
| `Feature` | `footprint`, `footprintX`, `footprintZ` | one `point16`, with the other two as `aliases` |
| `Spot` | the two cursors `anim` and `shadow`, or `state`, `pos` and `vel`, or `frame`, `pos2` and `animIndex` | the cursors are `embed` fields of `Handle` with a `when` that reads bit 0 of `flags16` of `features[feature]` (`via`) and wants it set; `state`, `posX` to the z velocity have the same `when` and want it clear; `pos2` is an alias of `pos`, and `frame` and `animIndex` are the `index` and the `src` of the cursor, aliases of `Handle.index` and `Handle.src` |
| `Spot` | `x` and `z`, `cell`; `burnTime`, `timer`, `animBits`, `animHi`, `animLo`; the bits of `flags` | `cell` (a `point16`), `timer` and `flags` (a `u8` each), with the other names as `aliases` |

The `Handle` at `Feature#N.anim` and `Feature#N.shadowAnim`, and at `Spot#N.anim` and `Spot#N.shadow`, is named by its site everywhere, because a table kind finds the container of a pointer by its address, whether or not the walk has named it. A `Handle` the loader initialises in the entry `features[featureCount]`, one past the end of the table until the feature type loader returns, has no container yet and is named by order.

```
  r Feature#5.flags16 1
  r Feature#5.filename "tree1"
  r Spot#9.anim.index 2
  r Spot#9.timer 12
  i services.<step cursor> Spot#9.anim = 1
```

A pair of pointer fields that bound a slice of a table is a closed range: the schema marks the end field with
`"range_end_of": "unitsBegin"`, and `Player.unitsEnd` points at the last unit of the player's slice (the walk is
`u <= end`), not one past it. The end pointer is written as the object it points at, `Unit#249`, like any pointer. The
walk reaches every unit of `[begin, end]` in table order, whether or not the module's call touched it, because the
module's loop reads each one; when only one field of the pair is in the slot's sets, only that pointer is followed.

### An array of refs

A field of kind `refs` is an array of `count` refs inside its object (`Ref<SquadTimer> timers[10]`): `{"name": "timers", "kind": "refs", "count": 10, "target":
"SquadTimer", "poly": "SquadTimer"}`. Each element is a ref, `null` or an object of its kind (a base is named as the concrete kind its vtable says, with
`poly`, as for a `ref`). The walk follows every element. Every element is written at the first record, and after that a changed element is one
line named by its index:

```
  r SquadManager#1.timers[0] SquadTimer#1
  r SquadManager#1.timers[1] null
  r SquadManager#1.timers[3] AssaultTimer#2
```

### An array of embedded structs, and a site inside an element

A field of kind `embeds` is `count` structs of a kind laid out in place (`CobScript.channels`, `OpenHeap.pool`):
`{"name": "channels", "kind": "embeds", "count": 8, "target": "Channel"}`, and the kind lists the site with `[]` where the index goes:
`"inside": [{"kind": "CobScript", "member": "channels[]"}]`. An element is named by its site and index, and its fields are written one by one, so a changed element
is its changed fields: `r CobScript#3.channels[2].pc 7`. The index is read off the address, and the replayer resolves `CobScript#3.channels[2]` to the bytes inside the container.

A member of each element is a site too, when the array is of a struct that is no kind: `{"name": "callbacks", "kind": "embeds", "count": 3, "target": "AimCallback",
"member": "weapons[].callbacks"}` in `Unit`, and `"inside": [{"kind": "Unit", "member": "weapons[].callbacks"}]` in the kind, give `Unit#3.weapons[1].callbacks` in walk lines, service lines and argument values.

### A table behind a ref field

A kind whose elements are an array that a `ref` field of another kind points at (`Player.groups`, 10 `Group`s) is a table with an object as its `base`:
`"table": {"base": {"object": "Player", "field": "groups"}, "count": 10}`. The kind has no table of its own and is no `delta` or `all-of` kind: each element is named
by its owner's field and index, `Player#0.groups[3]`, in every line, and `SquadTimer.group`, a ref into it, is written `r SquadTimer#1.group Player#0.groups[3]`.
The replayer allocates the whole array the first time a line names one of its elements, sets the owner's field to it, and resolves a name to the element. The owner is named before its elements
(the walk names it, as it reaches the owner first), and an index past `count` names nothing.

### A table with a capacity

A table whose live elements are the first `count` of room for more (`Game.eyes` has `count_14277` live of 20, and an ADD_EYEBALL event writes element `count`,
past the live ones) has a `capacity` beside its `count`. `count` is a member that the recording holds like any block field (a `game_fields` or `sim_fields`
entry), and the replayer never stores into it: `alloc` stores into the count member, which would overwrite the live count. Elements are named up to the
capacity, so a pointer to element `count` is `Eye#2`, not `Eye#?`. The all-of walk and the other walks that follow a table go only as far as the live
count.

```json
"FixtureEye": {"type": "FixtureEye", "scope": "state",
               "table": {"base": "game", "member": "hotRadar", "ref": true, "count": "hotRadarCount", "capacity": 4, "alloc": "module"}, "fields": [...]}
```

| Key | Meaning |
|---|---|
| `capacity` | an integer, or a member of the base: the number of elements the table has room for, and the number the replayer allocates when a recording first names an element and the `Ref` is null |
| `alloc` | absent, and the replayer allocates with its own allocator; `"module"` for a table whose memory the module frees (the heap's growth routine calls `operator delete` on `OpenHeap.pool`), which the replayer allocates with the module's own `operator new` |

A member capacity is read when the table is allocated, so its field must be restored before the first element is named: `replayschema.py --check` fails for a
complete subsystem's slot that reaches the kind with a capacity member missing from its read set, as for the factors of a product count. The count and the
capacity members of a table with a capacity must be tracked block fields, and a `capacity` is not combined with an integer `alloc`. `"alloc": "module"` needs a
`ref` table. A table with no `capacity` is as before.

A table behind a ref field takes the same keys, `{"base": {"object": "OpenHeap", "field": "pool"}, "count": "used", "capacity": "capacity", "alloc": "module"}`,
where `count` and a member `capacity` name integer fields of the owner. The elements are still named `OpenHeap#1.pool[3]`, up to the capacity, and the
owner's capacity field must come before the ref field in the owner's list, so that it is restored first. Without a `capacity` the count is a positive integer, as before.

```
  r Game.hotRadarCount 2
  r FixtureEye#3.x 9
  r FixtureEye#3.pos (5,6)
```

A slot that is all-of over a kind with a capacity gets the whole table allocated before it runs, whether or not a line named an element, since the module may write past the live count (adding an eyeball writes element `count_14277`).

The replayer's table is outside the block, so a ref to it is the difference of two addresses, as any ref the module stores. A replay of a module that
frees such a table frees the replayer's memory, which is why the module's allocator makes it. The replayer calls the exe's `operator new`, so such a table is for native replays; a WASM
module has an allocator of its own that the host does not call.

### A pointer into the object itself

A field of kind `self` points into the object that holds it. `{"name": "screen", "kind": "self", "member": "screenPos"}` is `Eye.screen`, a `Ref` to the same
Eye's `screenPos`. It is never written as a line, never compared, and never restored from a line: the replayer sets it to the address of that member of the
object itself, whenever a name resolves to the object. An object that the replayed module moves by assignment (removing elements from the Eyes with a standard-library algorithm copies the
pointer, which then leads into the old element) has it set again by the next line that names it, and a recording from before and after the move is
the same text.

```json
{"name": "screen", "kind": "self", "member": "screenPos"},
{"name": "flagPtr", "kind": "self", "member": "flagB"}
```

With `"index": true` the field is a pointer to an element of an array: `{"name": "top", "kind": "self", "member": "pool", "index": true, "target": "Node"}` points
into the array that the `Ref` member `pool` holds. It is written as the index of the element, `r OpenHeap#1.top 2` (`null` for a null pointer, and `?` when the pointer is
not at the start of an element, which the replayer refuses), and restored as the address of that element of the array the member holds. The array does not move in the text when
the module reallocates it and re-points the field, since the index is the same. `member` has to be a `ref` field of the same kind, listed before the self field,
that leads to `target`, a state kind whose size is the stride; the replayer restores it first and the index counts the elements of the array it made.

A self field has a `member` and, only with `index`, a `target`. Both forms are 4-byte `Ref`s; the generated header checks the size.

| Field | Stored as | Written as |
|---|---|---|
| `self`, `member` | an offset to the member of the same object | never |
| `self`, `index` | an offset to the element of the array of the `Ref` member | `2`, `null` |

### A ref with a C type

A `ref` field whose C type is not `Ref` of its target kind has the type in `ctype`: `{"name": "player", "kind": "ref", "target": "Player", "ctype": "char"}` for
`Eye.player`, a `Ref<char>` that holds a `Player`. The line is written as any ref is, `Player#3`, and the generated header asserts that the member is a `Ref<char>`.
A `ref` with no `ctype` is checked for its size only, as before. An array of refs (`refs`) takes a `ctype` too, in place of the `Ref` of its target.

### A vtable pointer the code sets by hand

`"vptr": "<global>"` on a kind says that its first word is a pointer to a global table of 2 functions that
the code stores itself, and that the type has no virtual member. The recorder checks the word whenever it names or walks an object of the kind, and ends the run with `a AimCallback has a vptr that
is not <global>` when it differs, 0 included. `"vptr_unset": "<reason>"` on such a kind lets the word be 0 as well, for an object
the code has not set up yet, which is then read as any other: `WeaponAimCobCb` has it, since a unit's aim callbacks hold 0 until
weapons' `INIT_SLOTS` event stores the global. `replayschema.py --check` fails for a `vptr_unset` with no reason or on a kind with no `vptr`.
The word is never written; the replayer stores the address of its own copy of the global when it resolves the object.

The store of the word is not checked: the replayer has the global in the word before the call, and the recording has no
line for it. An `Apply` of `INIT_SLOTS` that leaves a unit's aim callbacks at 0 replays equal (an AIM_DONE replays equal too, since
the replayer sets the word again when it resolves the callback), and only a game fails, at the first AIM_DONE of that weapon,
which calls through the word. The six recorded `INIT_SLOTS` calls check the rest of what the event sets up (the imports and
each slot's `weapon<N>Def` and `weapon<N>Flags`), not the three words.

### An embedded polymorphic object

An `embed` field whose kind has a `vtable` and a site (`Order.link`, a `PathOrderAttach` in the order) is read as a `poly` ref is, with the vtable checked, but stored in place and named by its
site: `Order#3.link` in lines and arguments (`i units.UnitRefUnlink Order#3.link`), `r Order#3.link.next Order#4.link`. The kind has the `vtable` of its class, and is no kind of a base.

### A pointer that a slot argument carries to an embedded kind

A slot or import argument of another pointer type (`Callback*`) whose object is an embedded kind with a `vptr` is `{"mode": "object", "kind": "AimCallback", "line": "callback"}`, and is named by its site,
`a callback Unit#3.weapons[1].callbacks`, as any object argument is. `mode: callback` stays for the interfaces of `callbacks` with virtual methods, and the import handle that carries
the same object is named the same way.

### A pointer or a ref that a member of the block holds

A member of `Game` or `SimGlobals` that is a raw pointer (`Game.src1`, `Game.lists[3]`), or an array of refs (`g_explodedPieces`, 100 `Ref<ExplodedPiece>`), has no block field kind of its own. It is a table of a kind whose
type is a one-member carrier, so that the existing forms apply: `{"type": "StringRef", "table": {"base": "game", "member": "src1", "count": 1}, "fields": [{"name": "ptr", "kind": "ptr", "target": "FrameTable"}]}` for a pointer
(`StringRef` is a class of one `char* ptr`, and the generated header checks the size of the member against the count), and `{"type": "Ref<ExplodedPiece>", "table": {"base": "sim", "member": "g_explodedPieces", "count": 100},
"fields": [{"name": "piece", "kind": "ref", "target": "ExplodedPiece", "member": "offset"}]}` for the refs. The elements are named by their index (`GafSrc1#0.ptr`, `ExplodedSlot#7.piece`), and an `all-of` rule of the row
that reads the member (`field_read Game::src1`, `global_read g_explodedPieces`) makes them roots of the call.

## Values

| Value | Written as |
|---|---|
| int, short, byte | decimal, signed or unsigned as the field's type |
| byte array | `x:` and two hex digits per byte: `x:00010100` |
| flags | `0x` and eight hex digits |
| float, double | hex bits: `f32:0x3f800000`, `f64:0x3ff0000000000000` |
| bytes of a buffer given to or filled by an import | `x:` and two hex digits per byte |
| Point16 | `(x,y)` |
| Vec3 | `(x,y,z)` |
| string | double quotes, C escapes, up to the first NUL |
| string behind a pointer (`cstring`) | the same, or `null` for a null pointer |
| pointer to an object (`ref` or `ptr`) | `Unit#17`, `UnitDef#2`, `weapondef#17`, `Feature#5.anim`, or `null` |
| object handed to an import | `file#1`, `rec#2`, `bank#1` |
| random stream | `main` or `load` |

No address, no padding byte, no time and no module mode is ever written.

## Arrays

A field that is an array (kind `array`) has a length that is an expression over other members of the same object (or of
`Game` or `SimGlobals`, for a block field), and holds either a `Ref` to its elements (`"ref": true`) or the elements
themselves. The annotation, for `Player.explored` and `Game.visibilityMask`:

```json
{"name": "explored", "kind": "array", "elem": "u8", "ref": true, "length": "exploredSize"}
"visibilityMask": {"kind": "array", "elem": "u16", "ref": true, "length": "(width>>1)*(height>>1)"}
```

`elem` is the element type (its size is what counts: the elements are written as the bytes of memory), and `length` is an
integer, or one or two factors joined by `*`, each a member of the object, `member>>k`, `member+a` or `member+a>>k`, or an integer. The
addend comes before the shift, as in C: `(count+0xff)>>8` is the `(count + 0xff) >> 8` words that `grid.dirty` holds, where `count>>8` is a word short
whenever the count is not a multiple of 256. A member of an embedded struct is named by its path, `grid1.count*2` for `Game.grid1.count`. A member is one, two or
four bytes. The length is read from the object each time the array is written, so the members it names have to be recorded
(`exploredSize` is a field of `Player`) for the replayer to know the length at the call.

The first use of an array in a recording writes the whole of it, and every later use writes only the elements that differ from
the last known value. A line of an array is one of these:

| Line | Meaning |
|---|---|
| `r Player#3.explored [1500] 0:0108...` | the whole array: its length in elements, then the run at element 0 as hex; the replayer makes the array and zeroes it |
| `r Player#3.explored 512:0aff...` | the next chunk of the whole array: the index of its first element, then its elements |
| `r Player#3.explored 5:aabb 1000:cc` | a later call: each run of changed elements as `index:elements`, separated by spaces |
| `r Player#3.explored null` | the `Ref` is null (an inline array is never null) |

A whole write puts at most 512 bytes of elements on a line, and a delta line takes no run that would take it past 1,200
characters, so a long array is several lines with the same name, in order, and a long delta is several lines too. `[0]` is an array of no elements. A whole write also follows a null, and a change of length. `w` and `o`
lines have the same forms, against the value known after the last `r` or `w` line. The replayer keeps the array in its block
and applies the runs to it; at the end of the call it is compared with the value the lines made known, and a difference is
reported at the field, as `byte 25 of 32: recorded 15, replayed 14, 1 bytes differ`.

## Struct arguments

A struct argument is recorded by the lines of its fields (`a pos.x 1`), whatever the kind. Three more forms are for structs of
a value kind (`scope: value`, no row of the field sets matches its fields):

| Form | Annotation | Lines |
|---|---|---|
| nullable struct in | `{"mode": "struct", "kind": "Vec3", "dir": "in", "nullable": true}` | `a pos null` for a null pointer, else the lines of the fields; the replayer passes NULL; in an import line the value as `(x,y,z)` or `null` |
| array of structs in | `{"mode": "structs", "kind": "Point16", "count": "count"}` | `a points[0].x 1`, `a points[0].y 2`, `a points[1].x 3`, and so on for each of `count` elements (`count` names another integer argument); `a points null` and `a points []` for a null pointer and an empty array |
| struct by value | `{"mode": "value", "kind": "Angles16"}`, for an argument or `result` | `a kind (5)`, `o result (-3,40000,7)`, and in an import line ` (5)` and ` = (-3,40000,7)`: the values of its fields in order, in parentheses, written as the field's kind writes them |
| nullable in/out struct | `{"mode": "struct", "kind": "MissionType", "dir": "inout", "nullable": true}` | the value going in as `(5)`, or `null` for a null pointer, in an import line (` (5)`) and in the `a` lines of a slot (`a count (5)`); the value coming out as `o count (9)`, written only when the pointer is not null. The replayer passes the value by address (or NULL), and the stub of an import writes the `o` line back through the pointer |
| nullable out struct | `{"mode": "struct", "kind": "Point16", "dir": "out", "nullable": true, "when": "result != 65535"}` | for a pointer a caller may leave null and that the call fills only for some results (`features.FindFeatureAtPos`'s `cell` and `size`, passed as stack locals or as null): a slot writes `a cell null` for a null pointer and nothing else going in, and the `o cell.x 4` lines after the call only for a pointer that is not null and when `when` holds (`result != N`, N a number; without `when`, always); the replayer passes a null pointer when the line says so, and compares the fields only in the same case. In an import line the argument is not written, and the `o cell (4,5)` line follows it only for a pointer that is not null and when `when` holds, which the stub writes back |

`MissionType` (one field, `index`) and `Angles16` (`bank`, `heading`, `pitch`) are the kinds of the by-value form so far. The
count of the array form is read from the count argument's own line, so the argument must be recorded. A mismatch of a result
by value is reported at `result`, one of an element at `points[1].y`.

An in/out value (`"dir": "inout"`, `nullable` or not) is the pointer an import or a slot reads and writes back, and the kind is
the type it points at. Only the value is written, not a line per field: the same text as the by-value form. A slot lists each
in/out name in the closing names of its call, so that an `o count (9)` line after an import is the import's when the slot does
not name `count` itself. A mismatch of the value coming out is reported at the argument (`count`).

A `const` pointer to a value kind (`const Point16*`, `const Vec3*`) that a slot only reads is recorded as the value itself, the line of the
by-value form (`a cell (185,78)`, and ` (185,78)` in an import line). It needs no annotation: the generator takes it from the `const` in the
header (`"byPointer": true`), and the replayer passes the address of the parsed value. A pointer to a value kind that a slot fills is
`{"mode": "value", "kind": "Vec3", "dir": "out"}`, for a slot with no other result. It is recorded as the result of the call: ` = (x,y,z)`
in an import line and `o result (x,y,z)` in a slot, the lines of a slot that returns the value, and the stub of an import writes the parsed
value through the pointer.

A pointer to a scalar (`int*`, `short*`, `unsigned char*`, and the unsigned forms of the first two) that a callee reads and writes back,
as `QueryScript` does with `arg0` to `arg3`, is `{"mode": "value", "dir": "inout", "nullable": true}` with no `kind`. The lines are those of the
struct form with the value in parentheses: `a arg0 (5)` going in, `a arg1 null` for a null pointer, `o arg0 (9)` coming out when the pointer is not
null (in an import line ` (5)` and ` null`, and `o arg0 (9)` after it). The replayer passes the scalar by address, or NULL, and the stub of an
import writes the `o` line back through the pointer. It is neither `cast`, which stays `void*`, nor a one-field struct kind, which would
describe an `int` as something it is not. A `value` with a `dir` other than `inout` and `out`, a `kind` on an in/out scalar, and a pointer to any other type are errors.

### A void* argument with a known kind

A `void*` argument that always points at one kind takes `"cast": "Vec3*"` beside its mode: `{"mode": "struct", "kind": "Vec3",
"dir": "in", "nullable": true, "cast": "Vec3*"}`. The argument then takes the modes of that type (`object`, `struct`, `structs`
and `vec3`) and the kind is checked against the cast type. The interface keeps `void*`: the generated code casts the parameter to
the type, in the recorder and in the stub. A `cast` on an argument that is not `void*`, or on a mode that takes no pointer, is an error.

## Event arguments

The `Apply` slot of `wind_meteors`, `targeting`, `los_radar`, `map`, `weapons` and `features` takes `(const <Module>EventHeader* event, unsigned int size)`: the
header every event of the module starts with (its `kind`, and the fields all kinds share), and the size in bytes of the whole
event, payload included. Each kind has its own payload struct, which starts with the header (`TargetingSetPos` is a
`TargetingEventHeader h` and `int pos[3]`), or is the header alone (`TARGETING_CLEAR`). The annotation is
`{"mode": "event", "events": "targeting", "size": "size"}`; the table of kinds is the one the interface header declares: each
kind of its kind enums and the payload struct it carries, with the header struct for a kind that has no payload. It is the trace
schema's table of kinds, the one the trace dump and each `Apply`'s size check use too.

The argument is recorded as the lines of the payload struct of the event's `kind`, header fields first (`a event.kind 97`,
`a event.unit Unit#1`, `a event.weapon 2`, `a event.pos0 10`, `a event.pos1 -20`, `a event.pos2 30`), and then `a size 24` as the
scalar it is. The elements of an array member are `name0`, `name1` and so on; a block offset named `unit`, `target`, `source` or
`attacker` is a ref to a unit (the list the trace dump reads too, `docs/trace-format.md`), recorded as an object name, so that the slot's reads of that unit are recorded as an object argument's are. A kind
the table does not list is recorded as the header. Exactly `size` bytes of the event are copied before the lines are made, so
a payload field beyond `size` reads as 0 (an event shorter than its payload is recorded, and the slot answers it with outcome 0);
the replayer builds a zeroed buffer, restores the lines, and passes the buffer with the recorded `size`.

**The payload in memory.** A replayer builds the bytes the slot reads from the `a event.*` lines alone, so that a core and the
in-repo engine give a module the same event:

1. It takes the payload struct of the kind on the `a event.kind` line, from the module's interface header (the header struct alone
   for a kind with no payload struct, or for a kind the table does not list), and a buffer of 256 bytes, every byte 0.
2. The struct's layout is its declaration order, every field at the next offset with no padding: a payload holds only 4-byte
   fields (`int`, `unsigned int`, `float`), arrays of them and arrays of `char` (section 6.1 of `docs/module-abi.md`), so a field's
   offset is the sum of the sizes before it. The header comes first, at offset 0, with `kind` at offset 0. A member that is
   itself a struct (the `params` of `WindMeteorsParams`, a `MeteorShowerParams`) is laid out in place, and its fields' lines
   are named without it (`a event.weapon`, not `a event.params.weapon`).
3. Each `a event.<member>` line writes its value at its member's offset, as the value of that member's type (section "Values"):
   a decimal integer as its 32 bits, little-endian; `f32:0x...` as those 32 bits; a string into a `char` array as its bytes, then a
   0, with the rest of the array left 0 (a string as long as the array has no 0); `name0`, `name1`, ... to the elements of the array
   `name` in order; an object name (`Unit#1`, or the site of a `refs` member, `Unit#2.weapons[0].aimCob`) as the block offset
   (`docs/module-abi.md`, section 7.1) of the object the replayer placed for that name, and `null` as 0; a weapon number (the
   `weapons` members below) as the integer it is. A member with no line stays 0.
4. It passes the buffer as the slot's `linear` event argument and the value of the `a size` line as its second argument. The
   event is the first `size` bytes of the buffer: a slot reads none past them (it refuses a `size` below its kind's payload,
   `docs/module-abi.md`, section 6.1). The event's address is the replayer's choice (the in-repo host copies the event to the
   scratch area, section 7.2 there), and a module never depends on it.

For example, the lines `a event.kind 66`, `a event.weapon "Meteor"`, `a event.radius 300`, `a event.density f32:0x40000000`,
`a event.duration f32:0x40a00000`, `a event.interval f32:0x42700000` and `a size 52` of wind_meteors make `WindMeteorsParams`:
`kind` 66 at 0, the 32 bytes of `weapon` at 4 (`Meteor`, then 26 zero bytes), `radius` at 36, `density` at 40, `duration` at 44 and
`interval` at 48, 52 bytes in all, in hex
`42000000` `4d6574656f72` followed by 26 `00` bytes, `2c010000` `00000040` `0000a040` `00007042`.

`"unrecorded"` maps the enum name of a kind to the reason the slot does not record it: weapons' `Apply` leaves
`WEAPONS_EXPLODE_PIECE` (see "Not recorded"). The recorder passes an event of such a kind (one at least as long
as the header) through with no lines, as it does a slot with `"record": false`, so what the call changes reaches the next
recorded call as `r` lines. `replayschema.py --check` fails for a name that is no kind of the table and for an empty reason.
`"refs"` maps a struct member (`Struct.member`) to the state kind its block offset leads to, for a ref that is no unit:
`{"WeaponsAimDone.callback": "WeaponAimCobCb", "WeaponsStartScript.callback": "WeaponAimCobCb"}` makes `callback` of those
two payloads a ref to a weapon's aim callback, named by its site (`a event.callback Unit#2.weapons[0].aimCob`). The check fails
for a kind that is no state kind, for a member that is no `unsigned int` of a struct of the module's interface header, and for
a member that is a unit ref already.

`"weapons"` lists the payload members (`Struct.member`) that hold the number of a weapon, an `int`:
`["WeaponsAreaDamage.weapon", "WeaponsSpawn.weapon"]`. The line keeps the number (`a event.weapon 225`), and the recorder makes
`weapondef#225` a root of the call, so the slot's reads of that weapon are recorded as `r` lines, as a weapon argument's are (a
number outside the table, such as the -1 of a spawn with no weapon, adds no root). The check fails for a member that is no `int`
of a struct of the module's interface header, and for one named twice.

The `Emit` service, which a module calls to publish an effect, is one `value` followed by the `size` as an `int`
(`i services.Emit (80,227,136162080,88473600,172703296,-139810,-983040,139810,1) 36`). The value is the payload as the
value of the payload struct of the effect's kind in the recorded subsystem's event table, chosen by the kind (the effect's first
`int`). It is made from the first `size` bytes of the effect, the rest zero (the annotation's `size` names the argument), so a
shorter effect is never read past its end.

## Polymorphic objects

An object with a vptr is written as the concrete kind it is, chosen by comparing the object's vptr with the vtable of each kind
the base lists: `ApproachRadius#1`, never `OrderFx#1`, and no vptr or address is ever written. The annotation:

```json
"OrderFx": {"type": "OrderFx", "scope": "state", "kinds": ["ApproachRadius", "RingApproach", "PathOrder"], "fields": []}
"ApproachRadius": {"type": "ApproachRadius", "scope": "state", "vtable": "g_approachRadiusVtable", "fields": [...]}
{"name": "attached", "kind": "ref", "target": "OrderFx", "poly": "OrderFx"}
{"mode": "object", "kind": "OrderFx", "poly": "OrderFx"}
```

| Key | On | Meaning |
|---|---|---|
| `kinds` | a base kind | the concrete kinds the base may be; the base has no fields of its own, its kinds list theirs (the base part included, `source`) |
| `vtable` | a concrete kind | the global table of 12 functions the vptr points at; the tool declares it in the schema and `static_assert`s that the type is polymorphic and derives from the base |
| `vtable_symbol` | a concrete kind | for a class whose vtable the compiler emits and no source global names: the mangled symbol `??_7<Type>@@6B@`, or `??_7<Type>@@6B<Base>@@@` for a class with several bases, where `<Base>` is the first (the tool refuses any other) |
| `vtable_secondary` | a concrete kind with a `vtable_symbol` | the other bases of the class that hold a vtable of their own, by name: `["UnitScanVisitor"]` (see below) |
| `poly` | a `ref` field, an object argument or result | names the base the pointer is typed as; it has to be the `target` or `kind`. A pointer to a base without `poly` is an error |

The numbering is per concrete kind, in order of first appearance, as for any state kind. An example of the lines a call holds:

```
  r Order#1.attached ApproachRadius#1
  r Order#2.attached RingApproach#1
  r ApproachRadius#1.radius 40
  r ApproachRadius#1.source Order#1
  r RingApproach#1.inner 10
```

The fields of the object are those of its concrete kind. The replayer allocates `Kind#N` when a line first names it and stores its
own vtable for that kind as the vptr (the vtable global of the replayer's link, which holds the module sources), so a vtable is
never replayed as bytes; a `ref` accepts any kind of the base it is typed as. A vptr that matches none of the listed kinds is
an error: the recorder writes `OrderFx#?`, and the run ends with `cannot write a replay file: an OrderFx has a vtable that is none of
its kinds` (naming the symbol when the vptr is the vtable of another kind of the schema). A vtable the compiler emits, which no
global of the sources names, is a kind's `vtable_symbol` (see A class with a compiler-emitted vtable).

### Code pointers

A field of kind `code` holds a pointer to a function that is not a vtable. It is written `<table>[<index>]`, the index of the
function in the named table, or `null`. The tables are the `code_tables` annotation:

```json
"code_tables": {"orderNotify": {"entries": ["ready", "standby", ...], "headers": ["<interface header>", ...],
                                "fill": "orders.RegisterAllOrderTypes"}}
{"name": "notify", "kind": "code", "table": "orderNotify"}
```

```
  r OrderTableEntry#1.notify orderNotify[44]
```

`entries` are functions in the order of the table (the `notify` pointers `RegisterAllOrderTypes` copies into `g_missionOrderTableVec`,
in the order it registers them), each declared in one of the `headers`, which have to be `includes` of the schema. The tool writes the
table as addresses of the game's functions in the schema header, so the recorder finds the index of the game's function and
the replayer, linking its own functions, stores its own address for the index. `fill` is the slot (`seam.Slot`) that fills the game's
table, for the subsystem that owns it. The copy of the schema in the oracle kit names each entry by what it stands for, not by its
function: an `orderNotify` entry by the name of its order type in the order type table (the names `orders.MissionTypeFromName` takes;
two names joined by `/` when one function serves both, and `Ready` for the first, which has no name), and a `weaponFire` entry by the
bit of `WeaponDef.flags` that selects it (`turret`, else `vLaunch`, else `lineOfSight` or `selfProp`, else `dropped`). A kind of the `index` scope cannot hold a code field (a value kind can: an element of the order table, `OrderEntry.notify`; `WeaponDef.fire` is set when the weapon types load, to one of four functions): a second kind
of the same C type, `WeaponFire`, is a state kind with a `table` over the array (`{"base": "game", "member": "weapons", "count": 256}`, no `ref`) and `fire` as its only field, and a rule with `"rule": "all-of"` on `WeaponDef::fire` makes every weapon a root of the call, so `WeaponFire#17.fire` is
the handler of `weapondef#17`. The table is recorded whole at the first call that reads it (256 lines) and by changes after that. A function that is in none of the entries is an error that names the table (the recorder
writes `orderNotify[?]`), and an index past the table is refused by the replayer. The ground pickup order (`Ground_Pickup`) is an entry through a declaration that has the type its definition has (an `unsigned char` flags).

### A class with a compiler-emitted vtable

A class with real virtual methods that the code constructs itself (`AiSearchGoal`, `UnitScript`) has a vtable that the compiler emits, and no
global of the sources holds it. Its kind names the symbol: `"vtable_symbol": "??_7AiSearchGoal@@6B@"`. The generated
vtable file of the replayer declares a global for the kind, `extern void (* const g_vtsym_AiSearchGoal[])() __asm__("??_7AiSearchGoal@@6B@");`, so it is the
class's own vtable, in the game and in the replayer's link. A kind may name that global itself with a `vtable` beside the symbol
(`"vtable": NAME, "vtable_symbol": "??_7AssaultTimer@@6B@"` declares the global NAME), and the tool refuses a global that two kinds name. The symbol is the vtable of the kind's `type`, which the tool checks. Everything else is
as for a `vtable`: the kind belongs to a base with `kinds` and a `poly` ref (or is the only kind of its class, with no base and no site: `Order`), the recorder writes the concrete kind its vptr names, and the replayer
allocates the object and stores the class's vtable, so a virtual call on a rebuilt object runs the class's own method. The fields are the
members the schema lists; a base class's members (the goal's `target` and `owner`) are listed on each concrete kind. A class whose base is also made on its own gets a kind for the base part
(`PlainTimer`, type `SquadTimer`, listed among the kinds of the base kind `SquadTimer`). The kit's copy of the schema
leaves `vtable_symbol` out, as it does `vtable`. The native self-test builds `FixtureGoalA` and `FixtureGoalB` objects, records them and calls a method of a rebuilt one.

A class with several bases keeps one vtable pointer for each base that has virtual methods: the victory conditions of players derive from `MissionCondition` and, to be scanned over the units, from `UnitScanVisitor`
(`class DefeatAllUnitsKilled : public MissionCondition, public UnitScanVisitor`). The compiler emits `??_7DefeatAllUnitsKilled@@6BMissionCondition@@@` for the first and
`??_7DefeatAllUnitsKilled@@6BUnitScanVisitor@@@` for the second, so the kind's `vtable_symbol` is the first and `"vtable_secondary": ["UnitScanVisitor"]` lists the others. The recorder still
tells the kind by the first vptr. The replayer stores the class's vtable at the start of the object and, for each base of the list, the vtable of that base at the offset of its subobject
(the cast of the class to `UnitScanVisitor*`, computed by the generated vtable file of the replayer), so the module's call of a visitor method on the subobject reaches the class's own method.
The tool checks that the base is a polymorphic base of the type. The kit's copy of the schema leaves `vtable_secondary` out. The native self-test builds a `FixtureTwin`, which derives from `FixtureGoalBase` and `FixtureScanBase`, and calls a method through its second base.

## Callbacks

Some imports call back into the recorded subsystem while they run: the scan visitors of the victory conditions are called for
each unit and return a result. Each call the import made on the object it was
given is a `c` line right after the import's `i` line and before its `o` lines, indented as the `i` line plus two spaces, in call order:

```
  i fixture.Scan scanner#1 = 0
    c scanner#1.visit Unit#3 = 1
    c scanner#1.visit Unit#9 = 0
```

The object is a handle named by the interface's `handle` and numbered in the outermost call by first use, like `file#1`. The
arguments and the result are written by the same modes as the arguments of a slot (`scalar`, `text`, `object` and `value`; a result
is void, a scalar or an object). What a callback writes to the block is not a line of its own, it shows in the `w` lines of the
outer call, as any write of the module does. The annotation names each interface once, the argument of the import names the
interface:

```json
"callbacks": {"UnitScanVisitor": {"header": "<the header of the class>", "handle": "scanner",
              "methods": [{"name": "VisitUnit", "ret": "int", "args": [{"name": "unit", "type": "Unit*", "mode": "object", "kind": "Unit"}]}]}}
{"mode": "callback", "callback": "UnitScanVisitor"}
```

`methods` are the virtual methods of the class in the header, in declaration order (the tool checks the names against the header, and a
callback interface has no virtual destructor). The tool writes the classes that do the work, one for the recorder
and one for the replayer. A `callback` argument is allowed on a slot that a subsystem lists in its `imports`, and only there.

The recorder hooks the object's methods for the duration of the import: it points the object's vptr at a class of its own that
derives from the interface, so each call is recorded and then made on the object's own method, and it puts the vptr back when the
import returns. The object keeps its address and its identity, so a callback the callee keeps for later (the `Callback` of
`StartScriptWithArgsByIndex`) is the module's own object after the import, and a call it makes later is not part of any import. Two limits end the
run, naming the call: a callback made inside another callback, and an import made inside a callback, are not recorded, because their
lines could not be in call order. A callback that frees its own object is put back only when the vptr word still holds the recorder's.

The replayer stub makes the recorded calls in order, on the object the module passed, with the recorded arguments, after it has
matched the `i` line and before it applies the `o` lines. It compares each result with the recorded one and reports a difference at
`<import>.c[<k>]` (`import 0 fixture.Scan.c[1]: recorded 0, replayed 1`), and a call recorded on another handle than the
object's at the same place.

### A callback held in state

A callback object can also be state the module keeps: the argument of a slot (`StartScriptWithArgsByIndex` takes a `Callback*` and the channel it starts calls back when the
script ends), or a pointer in an object (`Weapon.callbacks`). Two forms name one.

A slot argument of mode `callback` (the same note as an import's, `{"mode": "callback", "callback": "Callback"}`) is `a <name> <handle>#<n>`, or `a <name> null`.
A `ref` or `ptr` field that holds one has a `callback` key and no `target`, which names the interface and keeps `target` for an array of block-held structs:

```json
{"name": "hook", "kind": "ptr", "callback": "Callback"}
{"name": "link", "kind": "ref", "callback": "Callback"}
```

It is written `callback#2` or `null` in an `r`, `w` or `o` line, as `Kind#N` is, and the walk does not follow it. The tool rejects `callback` on any other
field kind (the key of a `ref` field of `Game`, the `elem` of a vector, a result, an argument of another mode), on a field of a value kind, and with a
`target`; it checks the interface against `callbacks` and, in the generated tables, that the member is a `Callback*` for a `ptr` and a `Ref<Callback>` for a `ref`.
The field kinds there are the callback pointer and the callback reference, and a `when` cannot read through one.

The handle is numbered as the handles of an import are, by first use in the outermost call, and an object is named once, by the line that first
names it (`r FixtureNotify#1.hook callback#1`, `a done callback#3`). The replayer binds `callback#N` when it parses that line, not when the module first
calls it, so a restored object has the number the recorder gave it. The numbers belong to one call: the replayer builds a new object for each handle of each
call, so a call whose module calls a held callback has the `r` or `a` line that names it.

The calls the module makes on the object during the slot are `c` lines indented by two spaces, as the `a` and `r` lines are, after the `i` lines and before
the closing `o` lines, with the arguments and result the `c` lines of an import have:

```
  r FixtureNotify#1.hook callback#1
  a holder FixtureNotify#1
  a done callback#3
  c callback#1.Complete 5
  c callback#3.Complete 10
  o result 5
```

The replayer builds the object itself, one class per interface that the tool writes (`ReplayHeldCallback`: the methods of the
interface, each recording its call, the object in the block so that a `Ref` can name it), and stores it in the field or passes it to the slot. Each call
the module makes on it is written as the recorder writes it, named by handle, method and arguments, and returns the result of the recording's call at that
position (an empty one when there is none). After the slot returns, the replayer compares each object's calls with the `c` lines of its handle, in order,
and reports a difference at `<handle>#<n>.<Method>`:

```
callback#3.Complete: recorded Complete 11, replayed Complete 10
callback#1.Complete: recorded Complete 5, replayed no call (missing)
callback#3.Complete: recorded no call (extra), replayed Complete 10
```

A `c` line whose handle no `r` or `a` line of the call names is fatal, as is a call on a callback object that none of the call's lines named.
The result a held object returns is the recorded one, so it is not compared; the module's behaviour after it is, through the `w` and `o` lines.

The recorder writes the `a` line of a slot argument, opens a scope for the slot (the object's vptr is the recorder's while the slot runs, as for an import)
and writes its calls as these `c` lines, and writes the `r` value of a field. It opens no scope for an object that only a field holds, so the calls it gets
are not recorded until a real site is annotated (#714).

## Vectors

A field that is a `BlockVector<T>` (the first, last and end words of the block heap, 16 bytes with its padding) is a field of kind
`vector`. Its value starts with the length and the capacity in elements, `[<len>/<cap>]`, and an empty vector that has no storage is
`[0/0]`. The capacity is recorded because the layout of the block heap after later allocations depends on it. The annotation names the
mode of the elements:

```json
{"name": "visibleUnits", "kind": "vector", "member": "visible.units", "elem": {"mode": "object", "kind": "Unit"}}
{"name": "counts", "kind": "vector", "elem": {"mode": "scalar", "type": "short"}}
{"name": "cells", "kind": "vector", "elem": {"mode": "value", "kind": "KeyedCell"}}
```

| `elem` | Elements | Lines |
|---|---|---|
| `object` with a `kind` | `Ref<T>` to objects of the kind (a polymorphic base is allowed) | `r PlayerAI#1.visibleUnits [2/4] Unit#3,Unit#9`, the handles comma-separated, `null` for a null element, the whole vector at each change |
| `scalar` with a `type` | the bytes of memory, as for an array | `r PlayerAI#1.counts [3/4] 0:010002000300`; a later call with the same length and capacity is the changed runs, `r PlayerAI#1.counts 1:0900`, and a change of either writes the whole vector again |
| `value` with a value kind | a struct, a line per field | `r PlayerAI#1.cells [2/2]`, then `r PlayerAI#1.cells[0].pos (3,4)` and `r PlayerAI#1.cells[0].key f32:0x3fc00000` for each field of each element that differs from the last known value |

The tool checks the element type against the C++ `BlockVector<T>` with a `static_assert` in the schema header (`Ref<Unit>`, `short`,
`KeyedCell`), so a vector of another type is a compile error. A `member` may be a path, as `visible.units` for the vector inside the
embedded `AiUnitList`. A `BlockVector` member of `SimGlobals` is a vector too (see A vector in the sim globals); `Game` has none.

The walker reaches the object of every element of an `object` vector through its `Ref`, as it does through a `Ref` field, so the units a
window holds only in a vector are recorded and restored. For a `value` vector a bracket line whose capacity or length changed is followed by the
fields of the elements it newly holds, in full, because the recorder cannot know what a new slot held before.

The replayer allocates `cap` elements with `SimAlloc`, which is what `BlockVector` allocates with, stores the three words, and fills the
elements. A vector of structs whose capacity changes keeps its old elements, as the module's `Insert` does when it moves them. A
difference is reported at `<field>.length`, `<field>.capacity`, `<field>[<i>]` (the hex of a scalar element or the handle of an object
element) or `<field>[<i>].<f>` (a field of a struct element), for example `PlayerAI#1.counts.length: recorded 0, replayed 2`.

### A vector in the sim globals

A `SimGlobals` member that is a `BlockVector` (`g_losTables`, `g_missionOrderTableVec`) is a block field of kind `vector`, with the `elem` an object's vector field has, in `sim_fields`:

```json
"g_losTables": {"kind": "vector", "elem": {"mode": "value", "kind": "LosTable"}}
```

It is named as a block field is, `SimGlobals.g_losTables`, and written exactly as an object's vector is: the bracket, and the elements under their path, at every depth (a `LosTable` is
a vector of `LosLine`s, each a vector of steps):

```
  r SimGlobals.g_losTables [4/4]
  r SimGlobals.g_losTables[3].lines [6/6]
  r SimGlobals.g_losTables[3].lines[5].steps [2/2]
  r SimGlobals.g_losTables[3].lines[5].steps[1].b 9
```

A slot that reads or writes the global has it in its `reads` or `writes`, and the walker follows the elements of an `object` vector of a global as it follows those of an object's. A difference is reported at
`SimGlobals.g_losTables[3].lines[5].steps[0].b`. The generated header checks that the member is a block vector of the type of the element. A `Game` member cannot be a vector.

### Vectors of vectors

A value kind may have `vector` fields of its own, so the element of a vector can hold vectors, to any depth. `{"name": "mids", "kind": "vector", "elem": {"mode": "value", "kind": "Mid"}}`
in the object, `"Mid": {"type": "Mid", "scope": "value", "fields": [{"name": "leaves", "kind": "vector", "elem": {"mode": "value", "kind": "Leaf"}}, ...]}`, and a `Leaf` with
`{"name": "steps", "kind": "vector", "elem": {"mode": "scalar", "type": "short"}}` are three deep. A vector in an element is written as any vector is, with the element's path in its name,
and its own elements are lines under that path:

```
  r Nest#1.mids [2/2]
  r Nest#1.mids[0].tag 10
  r Nest#1.mids[0].leaves [2/2]
  r Nest#1.mids[0].leaves[1].kind 1
  r Nest#1.mids[0].leaves[1].steps [2/2] 0:01000200
  r Nest#1.mids[1].leaves[0].steps 1:0900
```

The rules of a vector hold at every level. An element that is new (past the length of the last bracket line) is written in full, its inner vectors and theirs included, because the recorder cannot know
what its slot held before; an old element writes only what changed, so one inner element that changed is the one line `r Nest#1.mids[1].leaves[0].steps 1:0900`. A change of an inner vector's length or capacity writes
its bracket, and, for a vector of structs or strings, the elements it newly holds. The replayer keeps the old elements, inner vectors included, when a capacity changes. A difference is reported at the path,
`Nest#1.mids[0].leaves[2].kind`, `Nest#1.mids[1].leaves[0].steps[1]`, `Nest#1.mids[0].leaves[1].steps.length`. The walker follows the `object` vectors of an object, not those inside a vector of structs, so the generator refuses an `object` vector in a value kind with an error until the walker follows it.

A kind that derives from `BlockVector<T>` (`LosTable` is a `BlockVector<LosLine>`) is its own vector: its field has `"base": true` and no `member`, and sits at offset 0, `{"name": "lines", "kind": "vector", "base": true,
"elem": {"mode": "value", "kind": "Line"}}`. The generated header checks the vector's element type, and that the kind is as big as a block vector.

## Strings behind pointers

A field that holds a pointer to a NUL-terminated string is a field of kind `cstring`: `{"name": "label", "kind": "cstring"}` for a `char*` (or `const char*`),
and with `"ref": true` for a `Ref<char>`. It is written as the text, in double quotes with C escapes, as an inline `string` is, or `null` for a null pointer, and
is recorded when it changes since the last line that named it, as any field is:

```
  r FixtureNamed#1.name "alpha"
  r FixtureNamed#1.ident "tag \"1\"\n"
  r FixtureNamed#1.name "beta"
  r FixtureNamed#1.name null
```

The replayer allocates the string (with its own allocator, as it allocates an array) and stores the address, or the offset from the block base for a `Ref<char>`, so each
line leaves a new string. The value must be quoted, and a line that has no allocator to allocate with is refused. A difference is reported at the field, as
`FixtureNamed#1.name: recorded "beta", replayed "Beta"`; the text is compared, never the address. A walk never follows a string. The generated header checks that
the member is a pointer to `char` (or a `Ref<char>`).

An array of strings has `"elem": "cstring"` (a `char*` per element) or `"elem": "cstring_ref"` (a `Ref<char>`), and the `ref`, `length` and capacity of any array.
Its storage is the elements in the object, a `Ref` to them (`"ref": true`), or, with `"raw": true`, a plain pointer to them (`char** names`):

```json
{"name": "names", "kind": "array", "elem": "cstring", "raw": true, "length": "count"}
```

It is written as its length in brackets, `r Script#1.names [3]` (`null` when the storage pointer is null), and each element is a line named by its index, `r Script#1.names[0] "idle"`,
every element after the bracket line, and after that only the elements that changed. The replayer allocates the elements (zeroed, so a null string) when it reads the bracket line. `raw` and the string
elements are for the fields of an object, not for `Game` or `SimGlobals`, and `raw` is only for strings.

A vector of strings is `{"name": "words", "kind": "vector", "elem": {"mode": "cstring"}}`, with `"ref": true` in the element for `BlockVector<Ref<char>>`. It is written as a
vector of structs is: the bracket `r Script#1.words [2/4]`, and a line per element named by its index, `r Script#1.words[1] "run"`, for each element that is new or changed since the last
line that named it, and the replayer keeps the old elements when the capacity changes. A difference is reported at `<field>.length`, `<field>.capacity` or `<field>[<i>]`.

| Annotation | Holds | Written as |
|---|---|---|
| `{"kind": "cstring"}` | `char*` | `"text"`, `null` |
| `{"kind": "cstring", "ref": true}` | `Ref<char>` | `"text"`, `null` |
| `{"kind": "array", "elem": "cstring"}`, `"cstring_ref"` | `length` strings | `[3]`, then `names[0] "text"` per element |
| `{"kind": "vector", "elem": {"mode": "cstring"}}`, with `"ref": true` | `BlockVector<char*>` | `[2/4]`, then `words[0] "text"` per element |

## Bytes behind a pointer

A `ptr` may carry `"blob": {"length": EXPRESSION}`: the pointer leads to bytes the module reads (a block of bytecode), and the recording holds them.
The length is an expression over members of the same object, as an array's is (an integer, or one or two factors of `member`, `member>>k`, `member+a` or `member+a>>k`,
joined by `*`), in bytes: `count*4` for `int* entries` of `count` ints.

```json
{"name": "entries", "kind": "ptr", "target": "alloc", "blob": {"length": "count*4"}},
{"name": "bits", "kind": "ptr", "target": "alloc", "blob": {"length": "(count+7)>>3"}}
```

The pointer is written as any `ptr` is, `alloc#1`, and the bytes are a second field of the object, named `entries[]` (the field name and `[]`), written as an array of bytes is,
whole at the first sight of the pointer's target and a run of changed bytes after that:

```
  r FixtureProgram#1.count 5
  r FixtureProgram#1.entries alloc#1
  r FixtureProgram#1.entries[] [20] 0:0a000000140000001e0000002800000032000000
  r FixtureProgram#1.entries[] 8:4d
  r FixtureProgram#1.entries alloc#4
  r FixtureProgram#1.entries[] [20] 0:0a000000140000004d0000002800000032000000
```

The bytes are kept per target, not per holder: a second pointer to a target that has been written writes no bytes and replays to the same block, and a changed byte is one delta line and one difference. A pointer to another block writes all its bytes again, even when they are the same, since the replayer's new target is empty until a line fills it; a null pointer is `null` on both lines;
a change of the length is a new bracket. The line of the bytes follows the line of the pointer, and the members of the length have to be recorded (in the slot's
sets), as an array's have. Both fields are in a slot's sets when the pointer is, and the target is a state kind named at first sight, as for a `Game` ref (`alloc` is one): no table, sites, kinds or vtable.

The replayer allocates the bytes, zeroed, when it reads a bracket line, fills them from the lines, stores the pointer, and names the new block with the ordinal the pointer's line gave, so the pointer still
compares as the object it was named. A difference is reported at `FixtureProgram#1.entries[]` as an array's is (`byte 12 of 24: recorded 28, replayed 01`). A `ptr` with no `blob` is as before:
an empty copy of its target kind (see A pointer to an object no line named), and no bytes.

## Snapshots

Some imports return a buffer that the slot then fills, and the bytes it leaves there are the output that counts: the engine's capture allocation takes a size and
an out offset and hands out a buffer of the capture arena for a radar snapshot. A service whose result is `snapshot` (`"result": "snapshot",
"bytes": "bytes", "offset": "offset"`, `bytes` naming its int argument and `offset` its skipped out argument) is written with the size
and a handle, numbered in the outermost call by first use, and the bytes follow at the end of the call:

```
  i services.<capture allocation> 64 = snapshot#1
  w Game.timer 0x00000000
  o snapshot#1 x:0100000002000000...
```

The offset the engine's arena handed out follows the handle as `+offset` when it is not 0 (`= snapshot#2+5728`): the engine clears the arena between two calls that the recording does not
show, so a buffer's offset, which the module passes on in an event (`RecordSimEvent`), is an input of the call. The replayer places the buffer at the offset and answers it; a line with no offset follows the buffers before it in the call, the first at the start of the arena. After the `w` lines and the closing `o` lines, the call has one
`o snapshot#<n> x:<bytes>` line per snapshot it allocated, the buffer's bytes as the slot left them, with a long buffer in several
lines of the same name, in order, of at most 512 bytes (the lines are joined). A null result (the arena was full) is `= null` and has no
line.

The replayer's answer allocates `bytes` from an arena of its own, zeroed, at the recorded offset, and returns that pointer and the offset in `offset`. At
slot exit it compares each buffer with the recorded bytes and reports a difference at `snapshot#<n>[<byte>]` (recorded and replayed
byte), at `snapshot#<n>.length` when the sizes differ, and at `snapshot#<n>` when a snapshot was recorded and not allocated, or the
other way round. The `o snapshot` lines are never the outputs of the import before them. The tool writes the recorder's and the
replayer's code for a `snapshot` service from its spec; the capture allocator is the one service of the kind, hooked with an import line that records the byte count and the offset
and a return that records the buffer. When a module links the radar source, the replayer does not define the function: the answer calls the real one and compares the buffer it returns. The self-test allocates two buffers
through a function with the signature of the capture allocator, one of 700 bytes, and checks the lines, an equal replay, a changed byte,
a snapshot the stub did not allocate and a buffer of another size.

## Fields

Block fields are named as `data/rwsets.csv` names them: `Game.windCounter` for the member `windCounter` of `Game`,
`SimGlobals.g_meteorActive` for the global `g_meteorActive`. `rng.main`, `rng.load` and `rng.seed` are the two
generator streams of the block head and `g_randomSeed`; they are in the sets of the slots that draw.
`weapondef#17.flags` is the flags word of a weapon the module reads through a pointer it was given.

### A ref field of Game

A member of `Game` that is a `Ref` to one object may be a field: `"slowest": {"kind": "ref", "target": "FixtureView"}` in `game_fields`.
It is written as a `ref` is, `Game.slowest FixtureView#1` or `null`, and the object is named at first sight, as the target of a `ptr` is.
The replayer allocates the empty object when a line first names it, and stores its offset. When the field is in a slot's read or write
set the walk reaches the object, so its fields in the object sets are recorded, as for any `ref`; a `null` leaves them out.

```
  r Game.slowest FixtureView#1
  r FixtureView#1.flags 4
  r Game.slowest null
```

The target is a state kind with no `table` behind a ref, `inside`, `kinds` or `vtable`, because only such a kind is named by first sight, and
`replayschema.py --check` fails otherwise. A pointer to a separate object of a table kind (an overflow bucket that is not in its table) is
`Kind#?` and cannot be rebuilt, so it needs a kind of its own.

The target may instead be a kind of a table of `Game` or `SimGlobals` (`"unitsEnd": {"kind": "ref", "target": "Unit"}`): the field is then written as the element,
`Game.unitsEnd Unit#2500`, as `Player.unitsEnd` is, and the replayer stores the address of that element of the table. A rule that makes the table kind all-of for the
rows that read the field (`field_read Game::unitsEnd`) makes the walk reach every unit up to it.

A member of `Game` that is a raw pointer (not a `Ref`) to one object is `{"kind": "ptr", "target": KIND}` and is written, restored and walked
the same way (`Game.losTable`, a `FrameTable*` into loaded GAF memory: `r Game.losTable FrameTable#1`, `r FrameTable#1.count 40`). The replayer allocates the empty
object and stores its address, so a service that is given the table (the frame lookup) names it `FrameTable#1` again.

### A field the replayer derives

`Game.unitsEnd` (the last slot of the unit pool, `units + poolCount * sizeof(Unit) - sizeof(Unit)`, set once by the unit module at match load) is a ref field to the unit table, recorded by the slots that read it. A recording without a line for it (los_radar's, whose slots read only `Game.poolCount`) leaves it null, and the replayer then sets it from `Game.units` and `Game.poolCount` before each call.

### An array of refs in the sim globals

A `SimGlobals` member that is an array of `Ref`s to separate objects (`g_playerAI[10]`, one `PlayerAI` a player) is `{"kind": "refs", "count": 10, "target": "PlayerAI"}`
in `sim_fields`. The generator makes it one block field of kind `ref` an element, named `SimGlobals.g_playerAI[3]`, and a slot that reads or writes the array has them all in its sets.
Each is written and restored as a `ref` of `Game` is, `r SimGlobals.g_playerAI[0] PlayerAI#1` or `null`, and the walk follows the object it names. The target is a state kind with no table, sites or kinds.

A single `Ref` member is `{"kind": "ref", "target": KIND}`, one block field named after the member. los_radar's unit index is one: `SimGlobals.g_losUnitIndex` holds a `LosUnitIndex`,
whose `players` are 10 embedded `LosUnitIndexEntry`s (`embeds`), named by their site. `CreateUnitIndex` allocates the index and stores the ref, so its `w` line names the object
at first sight, and the replayer names the module's block the same way when it compares the line; `RefreshUnitIndex` writes the lists:

```
  i services.SimAlloc 440 = alloc#1
  w SimGlobals.g_losUnitIndex LosUnitIndex#1
  w LosUnitIndex#1.players[0].live 1
  w LosUnitIndex#1.players[1].visible [2/2] Unit#1,Unit#2
```

### A service that leaves a block field behind

An entry of `services` may list `outs` of the form `{"label": "SimGlobals.g_lastPlaceMetalSum", "sim": "g_lastPlaceMetalSum"}`: the recorder writes `o SimGlobals.g_lastPlaceMetalSum 2007`
after the import line, and the replayer's answer stores the value in the global before it returns. `ScheduleMapCanBuildAt` and `ScheduleMapCanPlaceUnitFootprint` have it, since the map module
leaves the metal under the footprint in that global and ai reads it right after the call.
Their line also carries the unit type, the cell and, for `ScheduleMapCanBuildAt`, the player (`object` arguments of the kinds `UnitDef` and `Player`, and the
`Point16` cell, passed by value, as a `value` argument), so the site a module picks is compared:

```
  i services.ScheduleMapCanBuildAt UnitDef#268 (33,19) 0 null = 0
  o SimGlobals.g_lastPlaceMetalSum 223
  i services.ScheduleMapCanPlaceUnitFootprint UnitDef#268 0 (231,107) 1 = 0
  o SimGlobals.g_lastPlaceMetalSum 2007
```

### A random draw inside an import

An import can draw from the random streams although the module made no draw: `SchedPublish` runs the handlers of a synchronous route
before it returns, and the attacked effect of a shot that hits a computer player's unit draws its next action (`RandomInt`) in ai's
handler. The imports made inside an import are not recorded, so the recorder writes an `o` line after the import for each stream word
(`rng.main`, `rng.load`, `rng.seed`) that changed during it, and the replayer sets the word from it before the module goes on:

```
  i services.SchedPublish 6 16
  o rng.seed 0x53116dcd
```

`RandomInt` and `GameRand` have no such line: the replayer advances the word as the game does. Besides weapons' recordings,
players' `console-units` and `mission-capture` have one such line each.

## The schema and its annotations

`data/replay_schema.json` has the `annotations` (written by hand, kept as written by the tool) and the generated
`fields`, `objects` and `subsystems` (one entry per simulation subsystem, with its slots and their read and write
sets). The annotations hold what no header says. The copy in the oracle kit leaves out the fields that name the source: the
`includes`, the `functions` and `through` of each slot, the `header` of each code table and callback, and the `header`, `pre`
and `effect` of each service (the replayer's own code); its code tables name their entries as Code pointers says. It also leaves
out what only the recorder, the replayer and `replayschema.py --check` read, which names the source's globals, types and
functions: the `exempt`, `exempt_types` and `rules` annotations, the `id` of each service, the `vtable` or `vptr` of each kind,
the `aliases` of each field, the indices that tie a field or a slot argument to the replayer's table of reached fields, and the
kinds whose every object a slot walks (the effect of an `all-of` rule). A recording names
none of these: an object is named by its concrete kind, and a field by its own name. A service or callback interface whose name,
or a method's, is a name of the source that nothing else in the kit declares is left out too, since no recording of the kit
calls it; once a recording does, the recording declares it. Every other name of the copy is one that the interface headers,
`layout.json`, the module manifest or a recording declares (a field named by its member, `velY` for `vel.y`, takes its name
from the layout), and the export refuses a copy that names any other source name, giving its path in the schema.

- `game_fields`, `sim_fields`, `exempt`: the block fields the recorder tracks, and the targets it ignores.
- `objects`: each kind's C++ type, scope and fields (name, value kind, target kind of a pointer). The generated header
  checks every size against the real struct.
- `rules`: a hand rule for each row the field sets cannot name: a `field_escape` row, or a `deref_read` or
  `deref_write` on a bare `*float`, `*short`, `*int` and the like. A rule is matched by `kind` and `target` (and by
  `functions`, when it lists the functions whose rows it covers: `*float` is a field of one kind in one function and of another kind in a second), and any row, not only an escape or a bare
  pointer, may be given one when the rows name only a part of what the code touches (`Point16.x` is a member of `Unit.cell`
  and of `Unit.footprint`). `whole-object` records every field of the named object; `index-array` records the weapon array
  through `weapondef#N`; `fields` adds the object fields it lists under `reads` and `writes` (`Unit.extraction`,
  `Player.energy`), so that a write through a pointer is recorded as a `w` line of the field it lands on;
  `all-of` makes every slot of a table kind a root of the call (`Game.players` is walked by index), and a slot may have the rule for
  several kinds. A rule's `reads` and `writes` may also name block fields, `Game.visibilityMask` or `SimGlobals.g_x`, which join the slot's read and write sets: a write
  through a `*short` or a `memset` into `Game` is a `w` line of the field it lands on. A `deref_write` of a struct pointer `*T`, where `T` is the type of a
  state or index kind or a view of one (a type whose members the kind's fields have as `aliases`, a debris view of `Debris`), is a whole-struct assignment and
  needs no rule: every field of the kind joins the slot's writes, so the compaction of `UpdateExplosions` (`*e = e[1]`) records every field of each `Debris` it moves. `replayschema.py
  --check` fails for a row of a subsystem function with no rule and for a rule that matches no row.
- `objects`' extras: `table` (an object, see Objects) names a kind by its index in a table, `inside` (a list of `{"kind": "Feature",
  "member": "anim"}`) names every member of another kind that the kind lives in (`Feature#5.anim`), a field of kind
  `embed` leads to such an object, `range_end_of` marks the end pointer of a closed range, and `aliases` (`Unit.cloakOn`) are
  the other targets of the rows that one field records (a bit field is recorded as its word). `replayschema.py --check` fails for a site
  that is no `embed` field of its container, for one that two kinds claim, and for the old single-site object form.
- Array forms and hand-made vtables (see An array of refs, An array of embedded structs, A table behind a ref field, A vtable pointer the code sets by hand):
  field kinds `refs` and `embeds` with a `count`, a `table` whose `base` is `{"object", "field"}`, `inside` members with `[]`, and `vptr` (with an optional `vptr_unset` reason) on a kind.
  `replayschema.py --check` fails for a count that is no positive integer, a base that is no ref field leading to the kind, a site no `embeds` field leads to, a `vptr` that is no global name, is also a vtable, or is on a kind with a table, kinds or a vtable, and a `vptr_unset` with no reason or on a kind with no `vptr`.
- `when` (an object, see A condition on a field): `{"field", "equals"}` with an optional `mask`, `via` and `kind`, on a field of
  a state kind. `replayschema.py --check` fails for a field it reads that is not an integer or flags field, that has a `when` of its own, or
  a mask and a value that can never meet.
- Field kinds that point rather than hold a value. `unitid` is a 16-bit unit id (`weapon0TargetX` holds either an x position or
  a unit id; `weapon0TargetUnit` is the same bytes read as an id) that the walker follows to `Unit#<id>`, but only when its `when`
  field (`weapon0TargetZ`) equals the given value (-32768, the sentinel that says the slot holds a unit); it is never written
  as a line, and the unit it leads to records only the fields of its `reach`. A `weapon` field leads the walk to `weapondef#N`, whose fields the slot reads
  are recorded as `r weapondef#17.flags` lines. A field may name a `member` of its type that is not its own name
  (`weapons[1].aimTarget`, `pos.x`), so that an array element or a part of a `Vec3` is a field.
- `reach`: a list of `Kind.field` names with an explicit `depth` (the number of objects from the root to the farthest, at most 2),
  on a `unitid` field and on a `units` argument. An object reached through it is recorded with only the listed fields, and
  the walk follows only the listed pointers, so `Unit.def` with `UnitDef.flags1` is depth 2. `replayschema.py --check` fails
  when the depth is not the one the list has, or a name is no field.
- `reads` and `writes` of a slot's annotation: block fields (`Game.width`) added to the slot's read and write sets, for what the function rows do not show, as a write to
  `Game` through a pointer, or the factors of a product count. `replayschema.py --check` fails for a name that is no block field.
- A field a slot writes through a pointer it cannot name exactly (a weapon slot picked by an index) is also in its read set,
  so that a change made outside the seam is recorded at the entry as an `r` line, not mistaken for a write of the module.
  The same goes for a field the module writes only some of the time (the `workTime` of a unit, the fields of a new projectile): a field in the write set and not in
  the read set would show a change made outside the seam, such as the move of a projectile by `UpdateProjectiles`, as a `w` line the module never made. A rule on the `field_write` row of the field with `reads` naming it puts the field in the read set.
- `exempt_types`: `{"OpenHeap": "reason"}`, a struct type that is no kind and is genuinely local (a stack copy read from a bank, a
  union summed in a local). A field row (`field_read`, `field_write`, `field_maybe_written`) of a complete subsystem on a type that
  is none of these is an error naming the row (`field_read OpenHeap.count`): the type of a kind or of a value kind, an alias of a kind's field
  (`aliases`), a type in `exempt_types`, a target in `exempt` (one member of a type, for an anonymous struct: `<anon>.whole`), a hand rule for
  the row, the table of a seam (`Seams`, `UnitsSeam`), or a service class the subsystem lists in `services` (`TdfFile`, or the argument list of a command). A type with no
  reason is an error. A pending subsystem is not asked.
- `indirect_call` rows of a complete subsystem. Every `indirect_call` row of a function behind a recorded slot, and of every function
  `through` adds, is an error (`pending indirect`) unless one of these accounts for it: a call through a seam table (`[UnitsSeam.CanLoad]`)
  to a slot the subsystem lists in `imports`, or to a slot of its own seam whose functions are all behind the slot being checked; a
  `virtual Class.Method` dispatch where `Class` is a kind with `kinds` (`poly`) or a kind with a `vtable`, and the `Method` of each concrete kind's type is
  among the functions (a concrete type that defines no `Method` of its own is not asked, and the base's own `Method` is asked only when some concrete
  type does not define one); or a `through` entry in the slot's annotation.
- `through` (a slot annotation): `{"<row target>": ["fn", ...]}` adds the named functions to the slot, so that their sets and their own
  `indirect_call` rows count as the slot's, for a table of code pointers such as `g_missionOrderTableVec[].notify`.
  `{"<row target>": {"reason": "..."}}` says that the call does nothing on the recorded paths. The key is the whole target of the row
  or the name in its last brackets (`OrderTableEntry.notify`). `replayschema.py --check` fails for an entry with no functions and no reason, a function that is
  none of the rows' functions, and a key that matches no `indirect_call` row of the slot.
- `subsystems.<name>`: `status` (`complete` or `pending`, with the `issue` that completes it), the `imports` (slots of
  other seams the subsystem calls) and per slot the `functions` behind it, the `native` symbol the replayer calls,
  and how each pointer argument is recorded:

| `mode` | Argument | Lines |
|---|---|---|
| `scalar` | a number, `Point16`, `Vec3` by value | `a name value` |
| `stream` | `RandStream` | `a stream main` |
| `text` | `char*` | `a name "text"` |
| `handle` | `HapiBank*` and the like | `a bank bank#1` |
| `object` | a pointer to a state kind, in | `a player Player#1`, its fields as `r` and `w` |
| `struct` (`dir` in, out or inout) | a pointer to a value kind, `nullable` when in or inout | `a params.field value` in, `o params.field value` out; an out struct with `"when": "result != 0"` is recorded and compared only when the call returns nonzero, for a call that leaves the struct untouched otherwise (`GetWeaponTargetPos`, whose caller passes a stack local); a `Vec3` out of an import is the `o <name> (x,y,z)` line a `vec3` out argument writes, only when `when` holds, and its stub writes it (`weapons` imports `targeting.GetWeaponTargetPos`) |
| `event` (`events`, `size`, optional `unrecorded` and `refs`) | a pointer to a subsystem's event header and a `size` argument | the lines of the payload struct of the event's kind (see Event arguments) |
| `structs` | a pointer to a value kind and a `count` argument | `a points[0].x 1` for each element (see Struct arguments) |
| `value` | a value kind by value | `a kind (5)` (see Struct arguments) |
| `value` (`dir: inout`) | a pointer to an `int`, `short` or `unsigned char` (or the unsigned forms), `{"mode": "value", "dir": "inout", "nullable": true}` | `a count (5)` or `a count null`, and `o count (9)` when the pointer is not null (see Struct arguments) |
| `argv` | a pointer to the argument list of a command (a count and its strings) | `a args.count 2`, `a args[0] "x"` |
| `bytes-in`, `bytes-out` | `void*` with a `length` argument, or a `length` that is a path through `Game` | ` x:0102` in an import line, `o dst x:0102` after it (`bytes-out` is as long as the result, at most the length; with `"resultElem": "u16"` the result counts elements of that size, as `UnitsInRange`'s count of ids does, and the bytes are `result * 2`). A `length` of `Game.net.unitCount` is read from the state at the call (`g_game->net->unitCount`: every step but the last is a `Ref` or a pointer, a null one is a length of 0), and `"elem": "i32"` makes it a count of 4-byte elements |
| `bytes-out` (slot) | `void*` with a `length` argument that a recorded slot fills, and a result that is its length in bytes (`pathfinding.Poll`'s `out`, the payload of an event, whose length is the result and at most the size argument) | no `a` line; `o out x:0102` after the call, as long as the result and at most the length, and no line for a result of 0; the replayer passes a zeroed buffer of the length argument and compares the bytes (a difference is reported at the argument, `out`) |
| `scratch` (`capacity`) | a pointer to elements that a recorded slot fills, with an integer `capacity` argument after it (`pathfinding.Poll`'s `points`) | nothing: no `a` line and no `o` line. The replayer passes a zeroed buffer of `capacity` elements, and what the call writes to it is not compared. Only a recorded slot takes it (`--check` fails for an imported one), and `--check` fails for a `capacity` that does not name an integer argument after it |
| `scalar` (`dir: out`) | an `int*` or another scalar pointer that an import fills, `{"mode": "scalar", "dir": "out"}` (`pathworld.SeenSize`'s `width` and `height`) | nothing goes in the import line; `o width 12` follows it for a pointer that is not null, and the stub writes the value through the pointer. Only an import takes it (`--check` fails for a recorded slot), and the pointer is one of `int*`, `unsigned int*`, `short*`, `unsigned short*` and `unsigned char*` |
| `weapon` | `WeaponDef*` | `weapondef#17`, or `null`; as a slot argument `a weapon weapondef#17` and, before it, an `r weapondef#17.speed 40` line for each `WeaponDef` field in the read set of the slot (the weapon is a root of the walk) |
| `vec3` | `Vec3*` with a `label` | `pos=(x,y,z)` in an import line, or `pos=(x,z)` with `"axes": "xz"` for a slot that reads only those (`GetGroundHeight`), which also makes its own argument `a pos (x,z)` and the replayer pass y as 0; with `dir: out`, nothing in the import line and `o out (x,y,z)` after it, which the replayer's stub writes |
| `callback` | an object an import calls back on, an import's argument | ` visitor#1` in the import line, and a `c` line for each call the import made on the object (see Callbacks) |
| `callback` | `Callback*` and the like, a slot's argument | `a done callback#3` or `a done null`, and a `c` line for each call the module made on it during the slot (see "A callback held in state") |
| `vector` (`dir: out`) | a vector of `Point16` an import fills, `{"mode": "vector", "dir": "out", "elem": {"mode": "value", "kind": "Point16"}}` | `o goals [(3,4),(5,6)]` after the import line: the whole vector, each element as the by-value form writes it, `[]` when it is empty; the stub pushes the elements |
| `units` (`dir: out`) | a vector of `Unit` pointers an import fills | `o out [Unit#3,Unit#9]` after the import line, then `o Unit#3.flags 0x00000021` for each field of the `reach` of the elements that changed since last written; the stub pushes the named units and restores those fields |
| `units-buffer` (`dir: out`, `capacity`, `reach`, `depth`) | a buffer of unit references the caller supplies, with the argument that holds its capacity; the slot returns the count of units | the import line has the slot's own name and its arguments but the buffer and its capacity; then `o out [Unit#3,Unit#9]` for the first `min(count, capacity)` units and the `reach` lines of their fields, as for a unit list; the count is not printed, the length of the list is it. A count above the capacity fails the recording, which would lose units. The stub is the import of the slot itself: it reads the `o out` line, writes the first `capacity` units of the list and returns its length. A slot that has `playsAs` instead (and no `reach`) replays the line of that other recorded slot, with the same arguments but the buffer and capacity |
| `pending` | anything else | `?` |

A `bytes-in` import line carries the bytes the call passed in, so a `files.BankWriteBox` line is `i files.BankWriteBox bank#1 x:0102 2 = 2`: the handle, the bytes written, the length, then the result. It has no `o` line. A `bytes-out` import has no bytes in its line: `files.BankReadBox` shows `?` for the destination and the bytes arrive in the `o dst x:...` line after it, as long as the result.

A result is `scalar`, `value` (`o result (-3,40000,7)`), `object` (compared by its name, `o result Unit#12` or `o result null`), `text`, or `weapon` or `object` with `reach` fields recorded as `o weapondef#N.flags` or
A slot may also return `text` (`const char*`, as an import's result may): it is recorded as `o result "Canal Crossing"`, or `null`, and compared as the text, never the address (`mission.GetMissionName`). It may also return a `weapon`, recorded as `o result weapondef#17` (or `null`) with an `o weapondef#17.flags 4` line for each field of its `reach`, and compared by that name (`weapons.FindWeaponByName`).
`o Cell#N.metal` lines (the replayer returns an object it makes, with those fields set).

A `weapon` argument of a slot is recorded as `a weapon weapondef#17`, or `a weapon null`, and the weapon is a root of the slot's walk: the
fields of `WeaponDef` in the slot's read set are `r weapondef#17.<field>` lines before the call, as for a weapon the slot reaches through a unit, and
those in its write set are `w` lines after it. The replayer resolves `weapondef#17` to `&g_game->weapons[17]`, restores its `r` fields
before the call and passes that pointer; `null` stays a null pointer. A field that differs on replay is reported as `weapondef#17.<field>`.

A `pending` subsystem lists in its generated `pending` entry what is left (untracked targets, rows with no rule,
slots whose functions are not found, arguments with no annotation); its own issue empties that list. A complete
subsystem with anything pending is an error, and the error prints every pending item, one per line.

A slot annotated `"record": false` with a `"reason"` is classified and left out of the recording. The reason is `load class`
(type loading and presentation-side init and free, which the full traces already cover) or the issue that will record
the slot (`#711`). The tool makes no recording slot for it, so the recorder passes a call to it straight to the module and
writes no line, and it skips its functions, read and write sets, untracked targets and rules (a rule that only its
functions matched is a rule that matches no row). A slot that no subsystem imports skips its arguments too. A slot that
some subsystem lists in `imports`, another seam's or its own, keeps its `args` and `result` annotations, required and checked
as for a recorded slot, and still gets its import hook: a call made inside a recorded slot is an `i` line, and a call made
outside one writes nothing. The generated `unrecorded` entry of the subsystem lists each such slot with its reason, and the
replayer's native table keeps its `native` symbol in place. The tool rejects `"record": false` with no `"reason"` and a
`"reason"` on a recorded slot.

`uv run tools/replayschema.py` also writes the recorder's tables, the recording copy of every seam table with its argument
hooks and the import hooks, and the slot runners, import stubs and native seam tables of the replayer. The services imports (`GameRand`, `RandomInt`, `FatalError`, `BuildDataPath`, the TDF and command classes) are the `services`
annotation: each entry names its declaration, in `interfaces/services.h` or in the engine header that holds the services no
module imports (checked against the header), and how its arguments,
result and `o` lines are written. From it the tool writes the ids and names, the recorder's lines and
the replayer's answers. The one thing
it cannot write is the hook itself, an import-recording line at the top of each function in the game source:
those functions are the services themselves, reached by direct calls with no table to wrap, so the hook has to be in the
function; `replayschema.py --check` fails when a hook's arguments differ from its entry or an entry has no hook.

### The test schema

The kinds the self-tests need and no subsystem has (`FixtureView`, `FixtureCell`, `FixtureHolder` and the like, in the examples of this page) are
not in `data/replay_schema.json`. `tests/replay_fixtures.json` holds them (`includes`, `game_fields`, `objects`, `fixtures` for the slots the self-tests call, and `inside` for the sites they add
to a real kind), a header of the tests their structs, and `uv run tools/replayschema.py` writes the tables
of the real kinds with these appended, and their vtables. The recorder's tables take the first when
the fixture switch is defined, which only the native replay test, the object fixtures (a copy of the objects that defines it first)
and the fixture recorder (below) do, so the exe's recorder has no fixture kind, and no real member of `Game`
(`Game.slowest`, `p1b8a` and `p1cd5` are the hosts of the fixtures) is declared as anything but what it is. The fixture annotations are checked
with the rest.

Each slot of `fixtures` is classified as a slot of a subsystem is, and the tool writes three things for it, so that every form of the format
is exercised by the tests and `replayschema.py --check` keeps them current:

- The shapes: the import stub of the slot (`Imp_fixture_<Slot>`, which the self-tests call as the module would), and, for a slot whose
  arguments the runner can pass (the modes `scalar`, `stream`, `text`, `handle`, `object`, `struct`, `structs`, `value`, `argv` and `callback`, with a scalar, object or value result),
  its runner `Run_fixture_<Slot>` and `After_fixture_<Slot>`, which call `Native_fixture_<Slot>`, a function that the native replay test defines. A self-test loads the
  lines of a call, runs the pair and checks `g_diffs` and the first key.
- The fixture hooks: the recording wrapper of the slot (`RecFixture<Slot>`) and a `FixtureSeam` table of them. The fixture recorder includes
  the recorder with the fixture switch defined and then this file in a second anonymous namespace, so the wrappers are compiled with the recorder's own helpers.
  The translation unit is a compile check, built by the `replay_native_test` target and linked into no exe. A fixture slot with a `callback` argument is
  wrapped as a slot (the `a` line and the `c` lines of the slot); any other is wrapped as an import, whose lines the stub reads.

### Services

Every service is one `i services.<name>` line, written when the function returns, so its pointer arguments show what the function left
in them. The line carries the arguments by their modes and the result after ` = `:

| Argument `mode` | Written as | Notes |
|---|---|---|
| `stream`, `int`, `double`, `text` | `main`, `12`, `f64:0x...`, `"text"` | `text` is up to the first NUL |
| `handle` (`file`, `rec`, `bank`) | `rec#2` | numbered by first use in the outermost call |
| `skip` | nothing | an out buffer or `this` that the entry names for its outs and effects |
| `bytes` (`length`) | `x:0102ff` | the bytes of a buffer, as long as the `length` argument |
| `textn` (`length`) | `"meteor 1\nmeteor 0"` | a text of that many characters, not NUL-terminated |
| `ints` (`count`) | `[3,4]` | that many words behind a pointer |
| `object` (`kind`) | `Gaf#1` | an object of a kind of the schema, named by first appearance in the file |
| `object` (`kind`, `reach`) | `Handle#1` | an object the import writes: its `reach` fields follow the result as `o Handle#1.index 0` lines, and the replayer restores them |
| `allocated` | `alloc#3`, `PlayerInfo#1`, `alloc#?` | a block that an earlier alloc returned, `alloc#?` when none did |
| `commandargs` | `["x","y"]` or `null` | the strings of the argument list of a command |
| `value` (`kind`) | `(80,226,90599392,88473600,327630432,104857,-983040,163111,1)` or `null` | the fields of a value kind behind a pointer, or passed by value (a `Point16` cell), in the schema's order, as the tuple a value argument of a slot is written as; a different effect fails the replay at the import |
| `effect` (`size`) | `(112,Unit#3,0)` | the effect a module publishes (`services.Emit`): a struct behind a pointer whose kind is its first field, written as the value of the payload struct of that kind in the recorded subsystem's event table, from the first `size` bytes of the effect with the rest zero (see Event arguments); `size` names the int argument that follows it, `i services.Emit (112,Unit#3,0) 12` |

| Result | Written as |
|---|---|
| `int`, `double`, `text` | ` = 7`, ` = f64:0x...`, ` = "text"` or ` = null` |
| `handle` (`kind`) | ` = rec#2` or ` = null` |
| `object` (`kind`, `ctype`, `reach`) | ` = GafFrame#1`, then `o GafFrame#1.width 64` for each field of the `reach` |
| `alloc` (`size`, `tag`) | ` = alloc#1`, or ` = PlayerInfo#1` when the annotation `alloc_tags` names the tag |
| `snapshot` (`bytes`, `offset`) | ` = snapshot#1` (see Snapshots) |
| `arg` (`param`) | nothing: the function returns the argument (a service that fills an out argument `dst` and returns `dst`) |

An out is an `o` line after the import line: `o dest "Armada"` for a text written through a pointer, `o dst 98304` for an int
(`{"label": "dst", "int": "dst"}`), `o file#1.current rec#1` for a record a file now points at. An entry may also give `pre` lines
(what the replayer's definition does before it reads the line: it empties `root` and `current` for `TdfFile_Construct`, and builds the
formatted text of a service that formats its arguments from those arguments), `effect` lines (the replayer's definition does what
the function does to its object or to the state: `GameRand` advances its stream), `cast` and `into_size`.

A cursor that a service starts or steps is an `object` argument with a `reach`, so each call records the fields it left in it, a `ptr` among
them (the services that start a cursor on an animation entry and step it, whose `Handle.src` is the entry or `null`):

```
  i services.<start cursor> Handle#1 FrameTable#1 0
  o Handle#1.index 0
  o Handle#1.value 4
  o Handle#1.kind 1
  o Handle#1.src FrameTable#1
  i services.<step cursor> Handle#1 = 1
  o Handle#1.index 1
  o Handle#1.value 7
  o Handle#1.kind 1
  o Handle#1.src FrameTable#1
```

One line per group, the engine's own services written by what they do:

```
  i services.BuildDataPath "gamedata" "meteor" "tdf" = "gamedata\\meteor.tdf"
  i services.TdfFile_LoadFile file#1 "gamedata\\meteor.tdf" = 1
  o file#1.root rec#1
  o file#1.current null
  i services.TdfRecord_GetFieldString rec#2 "MeteorWeapon" 32 "" = 1
  o dst "Meteor"
  i services.<translation> "key" = "Teksti"
  i services.<checksum> x:0102ff 3 = 1234567
  i services.SimAlloc 64 = alloc#1
  i services.<tagged allocation> "Player Info" 185 = PlayerInfo#1
  i services.SimFree alloc#1
  i services.RecordSimEvent 2 1 [3,4] 2
  i services.GameRand main = 4620
  i services.<slot lookup> 77 = 2
  i services.<frame read> FrameTable#1 2 = GafFrame#1
  o GafFrame#1.width 64
```

The services of the files and data group are `BuildDataPath`, the TDF calls that the subsystems make on records and files
(`TdfFile_LoadFile`, `TdfFile_SelectRecord`, `TdfRecord_GetFieldInt` and the others of `interfaces/services.h`) and a formatted write to a
file (the format and the formatted text); the commands group is a translation of a text, the reset of a command argument list and a
checksum of a buffer; sim plumbing is `SimAlloc`, the tagged allocation (the size and the tag are compared), `SimFree`, `RecordSimEvent`
(kind, flags, args and count), `GameRand` and `RandomInt`, the capture allocation of Snapshots and the lookups between a player slot and
its network id; the load reads are the loads of an animation file, of an animation of it, of an entry and of a frame of an entry or of a
cursor, and the loads of a 3D model and of its height, with results that are objects of the load kinds `Gaf`, `FrameTable`, `GafFrame` and
`Object3do` and the fields that the subsystems read as the `reach`. The `HAPI_*` file calls and the boxes of `HapiBank` are not services of this group. The bank operations of a
saved game that a module calls are the imports `services.HapiBank_OpenAccount`, `services.HapiBank_SetIntegerItem` and
`services.HapiBank_GetIntegerItem` (the bank as a `handle`, `docs/module-abi.md` section 9.3), and the other slots of the `files` seam,
the box reads and seeks, are recorded as `files.<Slot>` imports, as any import is. A hook records only for the thread that opened the slot, so
the calls of the loading thread or the drawing code that overlap a slot are not its calls.

A service whose definition is in a source that the replayer links (`SimAlloc`, the tagged allocation and `SimFree`, which the block source defines) is not
defined by the replayer. The hook in the real function asks `call.Answered()`, which is true only while the replayer runs a slot, and the
generated answer takes the place of the function: it checks the `i` line the module would have written, makes the real allocation and names the
block as the recording does (`alloc#1`, or the object of the tag's kind) or, for `SimFree`, lets the real free go on. The replayer's own
allocations are never answered. A service in a source nothing links keeps the definition the tool writes.

A service that runs a command text through the command slots is re-entrant (`"nested": true`). The commands it runs through the slots named `Cmd...` are `c` lines after its `i` line, in call order,
as the calls of a callback are:

```
  i services.<run command text> "plan 1" 6 ["x"] -1 = 1
    c ai.CmdPlan ["plan","1"]
```

The arguments of a nested call are written by the modes of a slot argument and the result follows ` = `. What a nested call writes shows in the
`w` lines of the outer call. The services a nested call makes are ordinary `i` lines, and they come before the `i` line of
the re-entrant service, because that line is written when the function returns; the stub finds its own line among them, makes the nested
calls through the seam table of the module with the cursor set back over those lines, and reports a result that differs at
`<import>.c[<k>]`. A command that is not a seam slot is not recorded, a slot import of the nested call is not recorded, and a command run
inside a nested command fails the recording.

## A module's region and world view

Economy, pathfinding, los_radar and players take a region at match load (`RegionSize(info)`, `Init(info[, world], region, size)`, `Shutdown()`), and economy a world view (pathfinding reads its own from `g_seams`). The engine calls them before the first tick, so a window that starts later would never see them. The `region` annotation of a subsystem names its slots:

```json
"region": {"size": "RegionSize", "init": "Init", "shutdown": "Shutdown", "world": "econworld"}
```

`world` is optional (a world view that `Init` is given; one the module reads from `g_seams` has no `world` key, see The world view). `replayschema.py --check` fails for a slot that is not in the interface, a missing key and a world that is no entry of `WORLD_VIEWS`.

| Slot | Lines |
|---|---|
| `RegionSize` | `a map (2501,2)` (the map-info struct, a value kind: `EconMapInfo`, `PlayersMapInfo`, `LosRadarMapInfo`, `PathMapInfo`), `o result 120048` |
| `Init` | `a map (2501,2)`, `a region 4185712` (the block offset of the region), `a size 120048`, then `w region [120048]` and the runs of the region as the module left it; the world is no `a` line |
| `Shutdown` | `o result region` when it returns the region the module was given, else `o result null` |

`RegionSize` and `Init` are recorded whatever the window: the recorder writes them as the first calls of the file even when the window starts later, and the replayer runs the module's own `Init` on a region of the recorded size, so the module's statics are bound before the first windowed call. The region is zero when `Init` starts, in the game and in the replayer.

**The region is the module's own state.** It is the pseudo-field `region` of the schema (base `region`, a byte array: `elem` u8, length the size given at `Init`), and it is in the read and write set of every recorded slot of the subsystem but `RegionSize`, `Init` (write only) and `Shutdown` (read only). It is written with the array forms of Arrays: a whole write `[N]` the first time, then the runs that changed since the last line that named it (`r region 66:4841 12070:803f`). A whole write of the region leaves out each 512-byte chunk that is all zero, since a region is large and mostly zero, and the replayer zeroes the array on `[N]`: the first chunk then is the line `w region [120048]` alone. No other array leaves a chunk out. A difference is reported at `region`, as `byte 12033 of 120048: recorded 00, replayed 89`.

A field a slot writes only some of the time must be in its read set, for a window to replay it (a field written for the player of the call only, while every player is walked): the schema's rules for economy give `Player.energyIncome` and the ledger fields of `UnitResources` reads for that reason.

**The world view.** A world view is a table of calls a module is given at `Init`. `WORLD_VIEWS` of `replayschema.py` lists it (`econworld`: `EconomyWorldView`, in `interfaces/economy.h`), and it is a subsystem of the schema with `"world": true` that is never recorded (the `record` switch refuses it) and whose slots are all `"record": false` and are imports of the subsystem that lists them. Its calls are `i econworld.PlayerInfo 3 = (...)` lines with the forms of imports. An out struct of a void slot is the result (` = (1,2,3)`), and one of a slot that returns a value is `{"mode": "value", "dir": "out", "named": true, "line": "uinfo"}`: nothing in the import line and `o uinfo (...)` after it, which the stub writes back. A block offset of a unit (`EconUnit`) is `{"mode": "object", "kind": "Unit", "ref": true}`: `Unit#251`, in arguments and results. `Emit` is `{"mode": "effect", "events": "economy", "size": "size"}`: `i econworld.Emit (16,0,Unit#1,0) 16`, the effect as the value of its payload struct (see Services). The recorder wraps the world given to `Init` with a recording copy; the replayer passes stubs that answer from the lines.

`pathworld` (`PathWorldView`, in `interfaces/pathfinding.h`) is the world view of pathfinding. The module is not given it at `Init` but reads it from `g_seams.pathworld`, so it has no `world` key in pathfinding's `region` annotation and is listed in `SEAM_WORLD_VIEWS` of `replayschema.py`: the recorder replaces `g_seams.pathworld` with its recording copy when pathfinding is recorded, and the replayer binds `g_seams.pathworld` to the stubs. Its slots are all imports of pathfinding (`i pathworld.Tick = 1171`, `i pathworld.UnitState 3316696 = 1`, `i pathworld.Pass 780 15 11 = 1`). A unit, a movement class and a requester's handle (`PathUnit`, `PathClass`, `PathHandle`) are block offsets or numbers that pathfinding keeps in its region and only hands back to the world view, so they are scalars (the raw numbers), not `Unit#N`: the stubs return the recorded numbers and the region replays byte for byte. `SeenSize` writes two ints through its pointers, the scalar out form (`o width 224`; see Struct arguments).

### A region that holds block offsets

pathfinding's region holds the `Pathfinder`, whose `PathGrid.cells`, `dirty`, `OpenHeap.pool` and `items` are `Ref`s, offsets from the block base, into the region itself. The region form restores the bytes, so they replay as recorded only if the region is at the block offset it had. The `Init` line of every region form therefore records it (`a region 4185712`, the offset of the region from the block base, after `a map`), and the replayer allocates the region at exactly that offset, with no allocation of its own at another one. A recording whose offset the replayer cannot take is refused with the offset named, as `block offset 2098176 (64 bytes) is not free` (the range clashes with another region, or with the replayer's own `Game`) or `block offset ... is past the block's end`; it is never replayed at another offset. The self-test `a region at its recorded offset` places a region, refuses a second one that clashes with it, one at the offset of the replayer's `Game` and one past the block's end, each by its offset.

## Not recorded

No scenario reaches these paths of targeting, and no recording holds them. Each has its reason.

targeting:

- `ChooseWeapon` returning 1 or 2: no shipped unit has weapon slot 0 empty and slot 1 or 2 defined.
- `Apply` of a SET_POS with a z of -32768: it needs a world z of 32768, and scenarios and the harness reject positions off the map.
- `Apply` of an event it rejects (a null unit, a weapon outside 0 to 2, or a SET_UNIT with a null target): every publisher passes a live unit and its own weapon number.
- `FindWeaponTarget` rejecting a candidate that is not alive or is dying: the candidate list holds only live units, so neither check can fail.
- `GetWeaponTargetPos` with the lead condition "the weapon slot has no weapon" failing alone: its only caller, `UpdateUnitWeapons`, skips a slot whose `flags` lack the value 2 (bit 1), and the value 2 means "has a weapon": weapons' INIT_SLOTS event sets it when the slot's weapon has a nonzero index.
- `Unit::ChooseWeapon` returning 1 or 2 (weapon slot 0 empty, slot 1 or 2 defined): no shipped unit has that shape. Checked by reading every `units/*.fbi` and `weapons/*.tdf` of the 30 archives of `~/.ta` (`totala1` to `totala4`, `tactics1` to `tactics8`, `worlds`, `rev31.gp3`, the `.ufo` and `.ccx` files) with the `Hpi` reader of `tools/tdfnumbers.py`: no unit leaves its first weapon key empty, or names a weapon of ID 0 there, while its second or third weapon key names a weapon of nonzero ID.
- `Unit::ChooseWeapon` with no weapon at all (all three slots without the value 2, which returns 0 through the third branch): its only caller, the attack order (the order type `Attack_Chase`), runs only for a unit with the armed bit (flag 0x80000000), and a unit gets that bit at creation from bit 16 of its def's `flags1`, which the unit type loader sets only when one of the three weapon keys names a weapon other than the weapon of index 0 (the default, `NOWEAPON`). A slot holds the value 2 exactly when its weapon has a nonzero index, so an armed unit has a slot with it. The 821 `units/*.fbi` of the archives (read as above) have only the weapon shapes (slot 1, 2, 3) 100, 110, 101 and 111, or none at all, and no unit names `NOWEAPON`.
- `FindBestTargetIfFireAtWill` with a fire order of 3: `Unit.fireOrder` takes its value from `standingfireorder` of the unit's FBI at creation (the shipped units have 0, 2 or none, which is 2), from the `STANDING_FIREORDER` order (the order type `Standing_FireOrder`, whose mode the order panel gives as 0, 1 or 2 and the scenario `firestate` line limits to 0, 1 and 2), or by copy from the builder or factory (`getbuilt`). A 3 is only what the order panel shows when the selected units have different fire orders ("mixed"), and is never an order's mode. Fire order 1 is recorded: `orders-immediate`, ticks 740 to 770 only, holds the call at tick 752 (the `firestate 1` of tick 750, before `firestate 2` at 755 and `firestate 0` at 760), which returns null without an import.
- A negative `UnitDef.range`: the unit type loader stores the key `sightdistance` in it as a signed 16-bit number and nothing writes it again, so only a value below 0 or above 32767 gives one, and the `SightDistance` of the 821 shipped `units/*.fbi` lies between 55 and 490. `FindWeaponTarget` with `useRange` 0 and `FindBestTargetIfFireAtWill` pass it to `GetVisibleEnemiesInRadius` as the radius.
- `GetWeaponTargetPos` with a cruise weapon, or a weapon of speed 0, and a unit with more than five kills (the other lead conditions are met): not reached by the scenario suite; it would need a veteran missile silo or bomber.
- `los_radar.GetVisibleEnemiesInRadius` with a count above the capacity of its buffer: the callers size the buffer for every unit of the pool (one entry per unit slot), and the list holds each unit once, so the count never exceeds it. A recording of such a call would lose units, so the recorder fails the run instead. The replayer answers a call from the next recorded line with the same player, position, radius and flags: it writes the first `capacity` units of the recorded list, in order, and returns the recorded count; a repeat with the same arguments and a larger capacity, after a count above the capacity, is answered from the same line and takes no further import, and any other extra call is a divergence.
- A unit target with `unitDefIndex` 0 where the second condition of the guard (the aim target field being nonzero) is false: that condition is always true there.

features:

- `Apply` of RECLAIM: the slot leaves it `unrecorded` (#897); a feature reclaim needs a scenario that makes a unit reclaim one, which changes `reference/` and makes the full gate this issue's. The reclaim itself, which reads and writes economy's account and units, is reached only through it.
- `Apply` of REMOVE and `Place`: no recorded window removes a feature by event or places one through the synchronous slot. Placing a feature is reached through `Apply` of PLACE (`orders-air-fights`, a model with a spot), and removing one through the replacement of a dead feature.
- The start of a fire and the burning half of `UpdateFeatureSpot` (the smoke, the spread, the burn-out), and `ScanFeatureCell` seeding a feature: no recorded window holds a flamable feature hit by a fire-starting weapon, and no feature of these maps has a seed chance above 0.
- `FindFeatureAtPos`: called by the reclaim and resurrect orders (the reclaim, resurrect and VTOL reclaim orders) and by the players' mining check, none of which a recorded window reaches; its `cell` and `size` are the nullable out form.
- The network branches of the feature damage (`Game.netGameFlags`-dependent game type 3): the recordings are of games of type 2.
- `FreeFeaturePool` and the other load slots (`LoadFeatureFileList`, `FreeFeatureFileList`, `NopAfterFeatureEnum`, `ResolveFeatureLinks`, `InitFeatureAnimPool`, `PlaceMissionFeatures`, `StampFeatureMetal`, `FindOrLoadFeatureType`, `LoadFeatures`) are `load class`; `SaveFeatures` is a query of a saved game, outside the tick; `UpdateFeatures` is the scheduler's phase driver, which calls the slots below it, each recorded on its own.

mission:

- `GetMissionIndex` and `HasMissionName` are recorded slots that no committed recording calls (the campaign menus ask for them, not a match). `MissionExists` is `unrecorded`: it reads the campaign file through `TdfFile`.
- Every slot that loads or frees a mission (`SetMissionType`, `FreeOtaEnumCacheAndMission`, `LoadCampaign`, `LoadMissionByName`, `RefreshMapList`, `SelectMission`, `AdvanceMission`, `SelectSchema`, `SetMissionUnitPosition`) is `load class`; they build the mission's record from the map's files and the TDF parser, which the recording would have to hold as services.
- `GetCampaignName`, `GetCampaignMissionName`, `CountMissions`, `GetBriefing`, `GetDescription`, `GetPlanet`, `GetTerrainSizeTier`, `GetTranslatedName`, `GetMemory`, `GetNumPlayers`, `GetTerrainLength` and `ComputeMapChecksum` are unrecorded ("a query no match makes: it serves the menus and the map list"): they read the names and the map list of the menus, and no scenario reaches them in a match (`data/coverage.csv`: `menu`, `campaign-menu`, `map-check`).
- `GetMissionUnit` and `GetMissionFeature` with an index past the count (result 0, the out struct untouched): the callers loop to the count.

weapons:

- A shot of an interceptor (a vertical-launch weapon with the interceptor flag): the scenarios before #975 reach the vertical-launch handler 887 times and fire 32 missiles from it (`mission-radius` and `long-greenhaven-8p`), none with an interceptor weapon; `ai-antinuke-intercept` (#975) fires the suite's one interceptor shot at tick 5696, which no recording holds, so its `FindTargetableProjectile` call and `Projectile.interceptedProjectile` are never reached by a recorded shot. The anti-nuke of `orders-weapon-build` searches (the `FindTargetableProjectile` slot returns null at its 3 calls) and never fires. The vertical-launch shot of a missile is recorded in `mission-radius`.
- `network.SendWeaponFire` and `network.SendUnitDamage`, and the `Game.netGameFlags` branch that calls them: they act only in a network game.
- The hit of a paralyzer weapon, a DAMAGE event of kind 2: the recorded DAMAGE events are of kinds 3 and 10, and the paralysis order is orders' handler of weapons' PARALYZE effect.
- `Apply` of REMOTE_DAMAGE, SET_STOCKPILES and ADD_STOCKPILE: REMOTE_DAMAGE comes only from the network, and no recorded window builds or sets a stockpile.
- `Apply` of EXPLODE_PIECE: the slot leaves it `unrecorded`, so the exploded pieces of `combat-deaths` reach the recording only through what they change. It allocates the piece in the debris chunk cache, a block of chunks in the block that the schema has no form for, and reads the model of the unit (#1081). DETONATE, AREA_DAMAGE, SPAWN and REMOVE_PROJECTILES are recorded: the detonation and the area damage reach the cells around a shot as roots and their units by id (A unit reached by its id, per slot).
- The store of the aim callbacks' vtable word by `Apply` of `INIT_SLOTS`: the word is never recorded, and the replayer sets it before the call ("A vtable pointer the code sets by hand").

economy:

- `UpdateResourceSharing` and its `network.SendShareMapInfo` import: it returns unless `Game.netGameFlags` bit 0 is set, and a share needs a remote partner. No scenario is a network game, and no play-test recording is stored.
- `Shutdown`: the game calls it when a match is unloaded, which no scenario run does before it ends. The form is replayed by the self-test.
- `GetEnergyUse` is recorded at tick 0 only (`console-iwin`, the whole run, 100 ticks): the unit type loader calls it, and a window cannot start before tick 1.

players:

- The respawn of the commander (`UpdatePlayerBody` when `Game.commanderDeath` is 2): no scenario sets that option. Its imports (`units.CreateUnit`, `units.FindUnitTypeId`, `features.FindFeatureAtPos`, `map.GetCellMeanHeight`, `mission.GetMissionParams`) and the services it calls are annotated for the call, and unproven by a recording.
- `UpdatePlayersFinish` and `UpdatePlayerTail` of a network game reach `network.BroadcastPlayerInfo` and `network.SendShare*` only with a remote partner: `allied-victory` and `share-all` have one, and the committed windows hold `BroadcastPlayerEconomy` (tick 330) and the shares (tick 900); `BroadcastPlayerInfo` is called only by the watch-mode path of `UpdatePlayerBody`, which no scenario takes (`Game.mapFlags` bit 1 with a watching host).
- `VictoryMoveUnitToRadius` is no kind: it scans through `map.UnitsInRange`, whose buffer of unit ids has no form yet, and the recorder stops with `an MissionCondition has a vtable that is none of its kinds` on a scenario that creates one (`mission-radius` and `mission-defeat-passes-x`, whose missions have a radius condition).
- The victory and defeat conditions of the scenarios `mission-kill-mobile`, `mission-build-unit`, `mission-passes-x`, `mission-passes-z`, `mission-timer`, `mission-victory`, `mission-defeat-of-type`, `mission-defeat` and `commander-death` are kinds of the schema, and each scenario records and replays equal over a window of its checks, but no committed recording holds them: each file costs about 0.75 MB for the units of the players' ranges, which every window walks. `VictoryKillAllOfType`, `DefeatAllUnitsKilled` and `DefeatAllUnitsKilledOfType` are the three classes with a second base that the committed recordings scan with.
- `SaveConditions`, `LoadConditions`, `RegisterConditions`, `FreeConditions`, `DeleteConditions`, `NewConditions`, `LoadPlayers`, `SavePlayers`, `LoadPlayerControllers`, `InitPlayers`, `FreePlayers`, `SetupPlayerSlot`, `RebuildAIFeatureCells`, `LoadDefaultAIScript` and `InitPlayerResources` are not recorded (`load class`): the match is set up and torn down, saved and loaded with them, and the traces of every scenario already cover that.
- `UpdateBlink` is not recorded: only the in-game dialogs call it, and it steps the blink of the player markers, which the display reads.
- `UpdatePlayers` and `UpdatePlayersOnStream` are not recorded: they are the scheduler's phase driver, which calls `UpdatePlayerHead`, `UpdatePlayerBody`, `UpdatePlayerTail` and `UpdatePlayersFinish` through the seam table (each recorded on its own) and the other subsystems' slots.

units:

units is complete with part of its seam recorded: the slots below are `record: false` with the reason in the schema, and each has what it needs written here.

- `Apply`, `CreateUnit`, `KillUnit` and `AddBuildProgress`: their closure holds the unit's death, the start of its script, the clearing of its references and the end of its construction, which call `scripts.NewUnitScript`, `scripts.SetCob`, `scripts.CreateObjectState`, `scripts.SetObjectState`, `scripts.DeleteScript` and `scripts.FreeObjectState` (they write `Unit.script` and the model state, and no import form restores a written pointer to an object the import made), and walk the list of `PathOrderAttach` references that starts at `Unit.head` (the references live in `Order.link` and `PathOrder.ref`, kinds of orders and movement).
- `UnitRefUnlink`, `UnitRefInit`, `UnitRefSetUnit`, `UnitRefGet` and `UnitRefSetValue`: the reference is an embedded `PathOrderAttach` of an order, a kind that orders annotates.
- `UpdateUnitTail`: aligning a unit to the ground reads the unit's 3D model through `Game.types` (a view of the model and a height sample of each corner), which has no form.
- `CreateFactoryIndex`, `FreeFactoryIndex`, `RefreshFactoryIndex` and `GetFactoriesInRadius`: the factory index is a static of units outside the block (only the pointer is tracked by the block), so no field names it.
- `FindMissionUnit`: it is called only by the unit commands of a mission when the mission starts, and no scenario reaches it.
- `UpdateAllUnits` is the scheduler's loop over every unit. It calls `UpdateUnitHead`, `UpdateUnitBody` and `UpdateUnitTail` through the seam, and other subsystems' slots between them, so its recording would hold a call of another recorded subsystem inside it; the head and body slots are recorded one by one.
- `WriteState`, `SaveUnits`, `GetMovementLimits`, `CreateFactoryIndex` and `FreeFactoryIndex` are queries or set-up outside the tick, and the load slots are load class.
- `UpdateUnitBody`'s water damage (`mission.waterDoesDamage` with a damage and a unit at or under the sea level that does not float) and its selection reset (`selected` with `selectable` cleared): no recorded window has a mission that sets water damage, and a unit is selected only by the interface.
- `UpdateUnitHead`'s wind generator branch (`windgenerator` above 0 with the wind on): the recorded windows have no wind generator.
- `FindUnitType` with a name that is no type (it returns null): every recorded name is a type.

ai:

- `DumpPlayerAI`: its only caller is the console command `PrintWeights`, which writes a file, and no scenario runs it.
- `DestroyPlayerAI`, `FreeSquads` and `FreeSquadManager`: the players are torn down after the last tick of a match, and the recorder has closed its file by then.
- `Apply` of REACT_TO_ATTACK: no recorded window has a unit of a computer player under attack (the AI of `skirmish2` and `console-sight` does not meet an enemy within the windows).
- The imports `weapons.WeaponCanReachPos`, `weapons.FindTargetableProjectile` and `los_radar.GetSeenUnitsInRadius`, and `los_radar.IsPointVisible`, which the retargeting and the scouting call when an enemy is near: no recorded call reaches them.
- The `ScoutTimer` and `SquadScoutTimer` branches that need an enemy in sight, and the orders an assault timer gives (`orders.GetOrderType` is called once in `console-sight` and once in `skirmish2`).

pathfinding:

- `Poll`'s `points` is a `scratch` buffer, never recorded or compared: the waypoints are copied from the events the region holds, whose writes `Step` records, and movement records the waypoints it receives.
- `WriteState` is unrecorded ("a dump query, called from the harness only"), and so are `SetSearchScale` and `DebugCell`, which are optional and called by a console command and by the debug overlay.
- `CellsChanged` with the cause `PATH_CHANGE_TERRAIN` (1): the recorded windows reach only cause 2, the refresh of the stamps of units; no scenario changes terrain or features in a recorded window, and the default implementation ignores both.
- An event of kind PROGRESS, a search that is cut short (PARTIAL, FAILED or NEAREST) and an `Init` for another map size: every recorded search is a direct one that is found, ending in the tick it starts.
- The alternative implementation of the seam (a hierarchical search): the default module is the one the game runs, and the recordings are of it.

los_radar:

- `Apply` of SHARE_MAPPING and SHARE_RADAR, and `network.BroadcastPlayerInfo` (the share mapping and share radar commands): they act only in a network game (outcome 0 otherwise), and no scenario is one.
- `Apply` of SHARE_MAP (the shared map info), CELL_HEIGHTS and SIGHT_LOAD: no recorded scenario publishes them (the first comes from `UpdateResourceSharing` of a network game and the third from a loaded game, which the run-from-tick-0 recordings do not include).
- The sight circle drawn from a frame of the `vismask` table, `GafFrame::pixelsOrLayers` (`mapFlags` bit 2 clear, `+lostype` off): `console-sight` reaches it and is not recorded, because the pixels of a GAF frame are loaded engine memory that the replayer would have to rebuild as a blob. The recorded scenarios all draw sight from the height-line tables (`Game.grid1`, `SimGlobals.g_losTables`).
- `GetVisibleEnemiesInRadius` as a slot: its out buffer of units has no form in a slot (`units-buffer` is the form of an import), so the slot is `record: false` and targeting's recordings replay it as an import line.
- The slots no recording calls at top level: `GetOriginCellAtPosition`, `IsUnitVisibleToPlayer`, `IsPointVisible`, `HasReadyUnitInRange`, `FreeUnitIndex` and `Shutdown`. The first four run only inside `RefreshUnitIndex`, `UpdateSensorRadarAndCloak` and the engine's own callers; `UpdateSensorRadarAndCloak` calls `HasReadyUnitInRange` for each unit that can cloak, so both recordings replay it as part of that slot (it reads the `visible` lists of the index and decides `Unit.flags` bit 0x1000 and `Unit.workTime`); the last two run when a match is unloaded.
- `GetSeenUnitsInRadius`, `LoadMappingData`, `SaveMappingData`, `InitRadar`, `FreeRadar`, `LoadTntMap`, `FreeMapResources`: load class, or called by `ai` only.

## Replaying

The native replay test replays a recording against the native module (`replay_native_test.exe`, built by
`cmake --build build/modern --target replay_native_test`, run under Wine in `modern-compile`, with the WASM replays below).
The subsystem is read from the recording's header; the module sources of a subsystem come from `data/subsystems.csv`.
CI replays the recordings of a subsystem once it has an entry in `tests/replay_subsystems.json` (its injected change,
the recording that change is checked on, and for a WASM module its variant), and `tools/ci_replays.sh` fails naming
a `tests/replay/` directory with no entry, or an entry with no directory.
An entry's `inject` is one change (`define`, `recording`, `expect`, `field`) or a list of them: `replay_inject_test.exe` is
built with every define of every entry, and CI checks each change on its own recording, so each change's recording diverges at that
change first.

```sh
wine build/modern/replay_native_test.exe tests/replay/wind_meteors/*.txt
```

One block is kept across the calls of a file: each `r` line is restored into it, the outer slot runs, every import is
answered from its `i` and `o` lines, and the write set and the closing `o` lines are compared with the file. A random
draw returns the recorded result, and the stream words advance as the game's do, so `rng.*` is checked like any field.
A call that differs is reported with the first import or field that differs and both values, and the file stops
there. Each file is replayed twice and the two results must be identical.

`--self-test [FILE]` first round-trips a small graph of objects (a `Player` with two `Unit`s sharing a `UnitDef`)
through the text form and reports a changed field at its `Kind#N.field`. It then does the same for each form of the text
format, whether or not a recording uses it: the indexed tables (`Projectile#3.speed`, `MovementClass#5.maxSlope`), the sized arrays (whole lines,
chunks, deltas and a null, for an object and a block field, with a changed element reported at its field and byte), a nullable
struct, an array of structs and a struct by value (a changed field reported at `pos.z`, `points[1].y`, `result`), polymorphic objects (an
`Order` that leads to an `ApproachRadius` and a `RingApproach`, rebuilt with the replayer's vtables, and an `OrderTableEntry` whose `notify`
is a code pointer), block vectors (a `PlayerAI` whose unit list, cells and counts are pushed to, grown, emptied and compared) snapshots (two buffers of a fake capture allocation) and raw pointer fields (a `Handle` whose `src` is a `ptr`, rebuilt and compared, with a changed `src` reported at `Handle#1.src`; a `ptr` to an object no line named, restored to an empty copy) and masked views (a union whose live view switches on a masked bit, or on a bit of another object, with the dead view neither written, compared nor restored) and an object embedded at two sites (named by its site in the walk and in a service line, and restored inside its container) and services (the files, commands, sim plumbing, load reads and GAF cursors, with their changed arguments reported at the import) and callbacks (a visitor of the module called twice by a fake import, recorded as two `c` lines and replayed on a
second object, with a changed result reported at `import 0 fixture.Scan.c[0]`) and callbacks held in state (a `FixtureNotify` whose `ptr` and `ref` hold a callback and a slot
that gets a third: the module's calls on them replay equal, and a different, a missing, an extra and a misplaced call are each reported at `callback#3.Complete`). Three
more use the `Fixture*` kinds: a table with a capacity (`FixtureEye`, written past its live count, restored without a store into the count, allocated by the module's `operator new`
and freed by the test with `operator delete`), pointers into an object's own members (a `FixtureEye` moved by assignment between calls, and a `FixtureHeap` whose `top` is an index into its
own `pool`, which the module grows), and a write to a `Game` field through a pointer, reported at the field. With a
recording it finally mutates the recording and expects each mutation reported. `replay_inject_test.exe --expect CALL FIELD FILE` runs the module built
with one changed constant and expects that divergence.

The self-test also replays a module's region (`a module's region`): a whole write that leaves out the zero chunks, a changed run, and a changed byte reported at `region` with its byte, and a region at its recorded offset (see A module's region and world view, "A region that holds block offsets"). It runs the forms of out buffers too (`a byte buffer a slot fills`, `a scratch buffer`, `a scalar the import writes`).

The recordings of pathfinding, which the modern build runs as native code in every mode (it has no WASM module yet), hold five windows of 600 KB to 2 MB, each with the region form's `RegionSize` and `Init` first (the region at its recorded offset, `a region 4341424`, of 959,664 bytes). Every tick's `Step` is mostly imports, a walk of the unit slots through `pathworld.NextUnit` (about 2,700 lines), so a window is a few ticks: `orders-move-attack` (ticks 179 to 195: `Submit` once, `Cancel` once, `Step` 17, `Poll` 19, `CellsChanged` once; the search of tick 181 is a STARTED event of 24 bytes and a FINISHED one of 28, both found directly, with 101 `pathworld.Pass` imports, `UnitState`, `SeenSize`, `BeginSearch` and `EndSearch`), `skirmish2` (ticks 1169 to 1172: `Cancel` 2, `CellsChanged` 15 at tick 1170, `Submit` once, `Step` 4, `Poll` 6, the events of tick 1171), `orders-tier2-core` (ticks 1029 to 1031: `Cancel` 2, `Submit` once, the events of tick 1030, 299 `pathworld.Pass`), `orders-air` (ticks 3599 to 3601: `Cancel` and `Submit` once, the events of tick 3600) and `orders-factory` (ticks 17 to 20: `Cancel` once and `CellsChanged` 15 at tick 19, each with the cause 2 and its movement class bits). A unit's `PathUnit`, a movement class and a requester's `PathHandle` are numbers in the recording (`i pathworld.UnitState 3316696 = 1`, `a requester 5406984`), and `o out (0,(32,13),63113,780,2,2)` is the `PathUnitState` the stub writes. The event payload of `Poll` is `o out x:...` as long as the result (`x:0200000090983200...` for a FINISHED), and its `points` are a scratch buffer. The injected change is the default step budget of a search, 0x500 in place of 0x535 (the step-budget injection of the test build, which the game takes when the match sets no limit): `Step` adds a smaller share to the budget kept in the region, and the divergence is reported at call 1 of `skirmish2`, at `region`, byte 72:

```sh
wine build/modern/replay_inject_test.exe --expect 1 region tests/replay/pathfinding/skirmish2.txt
```

The recordings of economy: `console-economy` (ticks 30 to 200: `Apply` of RESET, REQUEST, SPEND and INCOME 36 times, `RequestGranted` 32, `Spend` 3, `UpdatePlayerEconomy` 12, `UpdateMetalExtraction` once, the `+give`, `+nometal` and `+noenergy` commands), `edges-economy-targeting` (ticks 10 to 60: `Apply` 58, `Spend` 35, `UpdateMetalExtraction` 55 with the `econworld.Emit` of the extractors' effects), `duel-shore-to-shore` (ticks 600 to 660, a computer player at difficulty 0: `UpdatePlayerEconomy` 6 with the half handicap), `save-midgame` (ticks 899 to 902: `UnitAccount` 4) and `console-iwin` (the whole run: `GetEnergyUse` 2232, the unit types). Each file starts with `RegionSize` and `Init` at tick 0, and the windowed ones then restore the region as the runs of `r region` lines. `replay_inject_test.exe` for economy is built with a handicap of 0.4 where the half handicap is 0.5, and the divergence is reported at call 2 of `duel-shore-to-shore`, at `region` (the account of a unit that the computer player gets half of):

```sh
wine build/modern/replay_inject_test.exe --expect 2 region tests/replay/economy/duel-shore-to-shore.txt
```

The recordings of players, every one with `RegionSize` and `Init` first and then the windowed calls (`UpdatePlayerHead` runs for each of the ten slots at each tick, `UpdatePlayerBody` for each player each 30 ticks, with the region restored as `r region` runs before it, and `UpdatePlayersFinish` at each tick):

| Recording | Window | Calls | Reaches |
|---|---|---|---|
| `skirmish2` | 29 to 32 | Head 40, Body 8, Finish 4, Tail 2 | a skirmish (game type 2): the first period of each player's `UpdatePlayerBody` at tick 30, the victory and defeat checks of the skirmish |
| `console-units` | 145 to 175 | Head 310, Body 62, Finish 31, `Apply` 9 (KILL_UNITS 3, UNIT_DIED 5, UNIT_CREATED 1), `DeactivateConditions` 2 | `+kill 1` and `+kill`: the kill of a player's units and its `SchedPublish` of the kill effects, the conditions told of the deaths |
| `mission-capture` | 495 to 502 | Head 80, Body 16, Finish 8, `Apply` 3 (GIVE_UNIT, UNIT_CREATED, UNIT_DIED) | a capture (game type 1): the gift of a unit, the capture condition told of the capture, the all-units-of-a-type defeat told of the death with `units.FindUnitTypeId` and the scan of the units |
| `mission-kill-of-type` | 180 to 190 | Head 110, Body 22, Finish 11, `Apply` 55 (UNIT_DIED) | `VictoryKillAllOfType` and `VictoryKillUnitType` counting the deaths, `units.FindUnitTypeId` 2 |
| `mission-defeat-all-units` | 335 to 362 | Head 260, Body 52, Finish 26, Tail 2 | the end of a lost mission: the all-units-killed defeat scans the units, the countdown ends at tick 360 and sets `Game.endGameFlags` |
| `allied-victory` | 329 to 362 | Head 340, Body 136, Finish 34, Tail 6 | game type 3: the allied victory check, `network.BroadcastPlayerEconomy` at tick 330 and the end of a won game at tick 360 (`Game.endGameVictory`) |
| `share-all` | 898 to 903 | Head 60, Body 12, Finish 6, Tail 2, `Apply` 2 (TRANSFER) | the metal and energy transfers with `services.ScheduleEconomySpend` and `network.SendShareMetal` and `SendShareEnergy` |

The injected change of players is the period of a player's `UpdatePlayerBody`, 30 ticks (`0x1e`) in the game and `0x1f` in `replay_inject_test.exe`: the divergence is reported at call 16 of `skirmish2`, at `region` (`byte 4 of 84: recorded 3c, replayed 3d`, the first body of a player at tick 30):

```sh
wine build/modern/replay_inject_test.exe --expect 16 region tests/replay/players/skirmish2.txt
```

`replay_inject_test.exe` is built with the target-scale injection of 0x6666 (the 0xcccc of the weapon target position, the part of the
distance a target's body offset moves the aim point), and the divergence is reported at call 356 of `skirmish4-long` and at `pos.x`, the
out `Vec3`:

```sh
wine build/modern/replay_inject_test.exe --expect 356 pos.x tests/replay/targeting/skirmish4-long.txt
```

The recordings of los_radar are whole runs from tick 0 (no window), so that the unit index, built by `CreateUnitIndex` at match load and rebuilt by `RefreshUnitIndex`, is in every one of them from the start. `console-units` (300 ticks: `UpdateUnitLineOfSight` 725, `RecordRadarUnits` 301, `RefreshUnitIndex` 602, `Apply` 26 of UPDATE_UNIT once, REMOVE_UNIT 7, REVEAL_UNIT 7, ADD_EYEBALL 3, RECALCULATE once and SIGHT_RESET 7, `UpdateSensorRadarAndCloak` 11, `RecordRadarMapped` 11, `SoundHeardMask` 21, `GetMapCellAtPosition` 78) holds the eyeballs and the removals of units, and `sensors-sonar-jam` (150 ticks: `Apply` of REVEAL_UNIT 3, RECALCULATE once and SIGHT_RESET 3, `UpdateSensorRadarAndCloak` 6, the jamming and sonar paths of its visitors) the sensors. A radar capture is a snapshot (`services.SimCaptureAlloc`), with the arena offset of the engine after the handle. The injected change is the refresh period of the unit index, 0x3c ticks in place of 0x1e (the refresh injection of the test build), reported at call 530 of `console-units`, where the index is not rebuilt and its first `services.SimAlloc` is missing:

```sh
wine build/modern/replay_inject_test.exe --expect 530 "import 0 services.SimAlloc" tests/replay/los_radar/console-units.txt
```

The recordings of targeting, which the modern build runs as native code in every mode (it has no WASM module yet): `orders-guard-attacked` (units that guard and are attacked: every
slot, and `Apply` of every kind but SET_POS: SET_UNIT 315 times, CLEAR 1014 times, RESET 18 times; candidates from `los_radar.GetVisibleEnemiesInRadius`, `ChooseWeapon`),
`orders-tier2-spider` (`Apply` of a SET_POS 8 times, and the ground positions of `GetWeaponTargetPos`), `orders-immediate`, ticks 740 to 770 only (a unit with fire order 1, 2 and 0 in turn, which searches only with 2), `edges-targeting-lead` (the whole run, 1300 ticks, listed below) and `skirmish4-long`, ticks
25750 to 25770 only, where a unit with more than five kills fires at a moving unit (the scale of `GetWeaponTargetPos`). `console-units` (the whole run) holds a
unit target whose `unitDefIndex` is 0, so `GetWeaponTargetPos` returns 0 and the target is cleared. `edges-economy-targeting` (the whole run, 260 ticks) holds
a search with more than 50 candidates, candidates at the same spot (`services.RandomInt` of 0), a unit with fire order 0, a kamikaze unit
among the candidates, a paralyzer weapon against a paralyzed target, a veteran commander (more than five kills) aiming at a building,
the D-gun (`Apply` of a SET_POS on weapon 2), and an aim at ground under the sea (`map.GetGroundHeight` returns 1). Each
`GetWeaponTargetPos` that returns nonzero records the `pos` it wrote (`o pos.x`); one that returns 0 leaves the stack local of its caller untouched, so
nothing is recorded of it.
`save-targets`, ticks 149 to 151 only, holds the `SavedTargetPair` calls of a saved game: the game is saved after the step of
tick 150, and the calls carry that tick. `Unit#2` (a tower attacking the Core commander, `Unit#251`) returns 0x800000fb (-2147483397) for
weapon 0, and `Unit#3` (a tower attacking the ground at (3140, 1180)) returns 0x049c0c44 (77335620) for weapons 0 and 1; every other
weapon returns 0x80000000. `load-targets`, ticks 150 to 151 only, loads that game: its `Apply` calls of LOAD give the same halves back
(`xOrUnit` 251 and `zOrSentinel` -32768, and 3140 and 1180).

`edges-targeting-lead` (the whole run) holds three outcomes the other recordings lack. All its lines are of `tests/replay/targeting/edges-targeting-lead.txt`:

- Ground height equal to the sea level: `sealevel 1`, then the D-gun at the ground at (850, 300), where `map.GetGroundHeight` returns 1: the call at tick 11, line 1332, where `GetWeaponTargetPos` returns 1 with `o pos.y 65536` (16 calls, from tick 11).
- Bit 10 (0x400) of `Game.uiOptionFlags` (`shootall`, tick 60) deciding a pick: the call at tick 83, line 4750, where `FindWeaponTarget` of the human commander `Unit#1` takes the enemy `CORSOLAR` (`Unit#502`, a def without `shootme`) as its only candidate and picks it.
- The lead check of `GetWeaponTargetPos` at 4, 5, 6 and 7 kills, in `Unit#2` (an `ARMHLT`) aiming at `CORCK` constructors that patrol: with 4 kills the aim point is the sweet spot while the target moves (first at tick 766, line 35313), with 5 kills the same (tick 866, line 39500), and with 6 kills the target's velocity is added (`o out` differs from `o pos.x` and `o pos.z`): tick 1072, line 48086, with `UnitMotion#8` velocity read just before it. The first calls at each count are at ticks 724 (4 kills, line 33597), 864 (5, line 39429), 998 (6, line 45010), 1127 (7, line 50344) and 1233 (8, line 54800). In this list a line is the `scripts.GetSweetSpot` import of the call, a few lines below its `call` line. A target that moves at 6 kills is one call in the run (and 62 at 7), because the tower kills quickly.

`replay_inject_test.exe` is also built with the mean-shift injection of 0 (the shift by one that halves the sum of the low and high corners of a cell in the cell mean height), and the divergence is reported at call 63 of `orders-air-repair` (its seventh `GetCellMeanHeight`; map 5.0's recording holds the `GetGroundHeight` calls too) and at `result`:

```sh
wine build/modern/replay_inject_test.exe --expect 63 result tests/replay/map/orders-air-repair.txt
```

The recordings of map 5.0 (map 7.0 recordings call the pass map slots `PassMapGet` and `PassMapInBounds`; map 7.1 recordings, of #1019, write the path lock stamp in ADD_UNIT where 7.0 published PLACED to movement), which the modern build runs as native code in every mode (it has no WASM module yet), where `Apply` is called with each event map is given: `ui-build-place` (the whole run: `BuildAllPassMaps` at load, `Apply` with PASS_MAPS, ADD_UNIT and REMOVE_UNIT, `CanBuildAt`, `CanPlaceUnitFootprint`, `SnapWorldPosToFootprint`, `GetGroundHeight`, `PassMapGet`, `PassMapInBounds`), `orders-air`, ticks 1020 to 1585 only
(`Apply` with ADD_UNIT, REMOVE_UNIT, SET_YARD, BUCKET_UNLINK and BUCKET_PREPEND, `CanPlaceFootprintAt`, `CanBuildAt`, `CanPlaceUnitFootprint`, `GetGroundHeight`, `PassMapGet`, `PassMapInBounds`) and `orders-air-repair`, ticks 380 to 385 only (`Apply` with ADD_UNIT, REMOVE_UNIT and CLEAR_UNIT, `IsPadSlotFree`, `CanPlaceUnitFootprint`, `GetCellMeanHeight`, `GetGroundHeight`). `RefreshMovedUnits` and `RefreshUnitIfStale`, map's pass-map refreshes since 5.0, are not recorded yet (`record: false`, #660 part 3): they write units' stamps and publish CELLS_CHANGED, which the recorder has no form for.
A window cannot start at tick 0, so only a whole run holds the load-time `BuildAllPassMaps`. The first call that names a field of `Cell` writes it for every cell of the table (a map of 224 by 102 cells is 22,848 elements of each field, a few lines a field as runs, "A table recorded as changed elements"),
which is most of each file, and after that only the cells a call changed. RECLAIM_FOOTPRINT is in no scenario.
The spatial grid `Game.cells_1429f` is the table of `SpatialBucket` (`width_142a3` times `height_142a7` buckets, included under `pack1:` because the game lays a bucket out in 10 bytes, and listed before `Cell` so that the units
of a chain are recorded whole before a cell's `occupant` reaches them with a limit), `Game.overflowBucket` is a `ref` to `OverflowBucket`, a kind of the same type with no table, and the unit a cell holds is reached through its
`occupant` field (a `unitid` over `unit`). A unit whose bucket is the overflow bucket is written `SpatialBucket#?` and cannot be replayed: no committed recording has one. The replayer defines `operator delete` over `SimFree`, as the game does.

`replay_inject_test.exe` is also built with the feature-HP injection of 20000 (the damage test of the feature damage, a feature killed by the damage it has taken, compared with 20000 more), and the divergence is reported at call 404 of `commander-death` and at the first import that differs, `import 1 los_radar.GetMapCell` (a feature that is not killed makes no kill call):

```sh
wine build/modern/replay_inject_test.exe --expect 404 "import 1 los_radar.GetMapCell" tests/replay/features/commander-death.txt
```

The recordings of features 5.0, which the modern build runs as native code in every mode (it has no WASM module yet), both in a window of ticks. The first call that names a field of `Cell` writes it for every cell of the table (about 43 KB of each file, as runs), and the first that names a `Feature` or a `Spot` every element of those tables:

- `commander-death`, ticks 181 to 186 (2,637 calls): the death of the commander at tick 182 is 30 DAMAGE events (`Apply`: 29 kill their feature, outcome 1, and one does not), each kill places a spot with the death animation (`services.InitGafSequence`, `los_radar.GetMapCell`, the pool's lists and the cell's flags written), then `UpdateFeatureSpot` (145 calls, `services.StepGafSequence`) walks the used list, and the phase's `FeatureTypeCount`, `AnimateFeatureType` (399 types a tick), `ScanFeatureCell`, `FirstFeatureSpot` and `GetCellFeature` are called every tick.
- `orders-air-fights`, ticks 708 to 713 (2,435 calls; `Apply`: 2 DAMAGE and 1 PLACE, all outcome 1): DAMAGE at tick 709 that kill features, and a PLACE of a feature with a model (`FEATURES_HAS_POS | FEATURES_HAS_ROT`, a spot with its piece state: `scripts.ObjectStateSize`, `services.SimAllocTagged "Object State"`, and the INIT_STATE and PASS_MAPS effects through `services.SchedPublish`), and 11 `UpdateFeatureSpot` calls.

`replay_inject_test.exe` is also built with the start-shift injection of 15 (the 16 of `rules[i].x << 16` in `Mission::GetStartPosition`), and the divergence is reported at call 237 of `console-ilose` and at `out.x` (recorded 36700160, replayed 18350080):

```sh
wine build/modern/replay_inject_test.exe --expect 237 out.x tests/replay/mission/console-ilose.txt
```

The recordings of mission 2.0, which the modern build runs as native code in every mode (it has no WASM module yet), both of the whole run (the second only under `long/`), because the load-time queries come before tick 1 and a window cannot start at tick 0. The mission is `Game.net`, one `Mission` with its names, figures and counts as fields, and `rules`, `units` and `features` as the tables behind it:

- `console-ilose` (a skirmish of 100 ticks, 452 calls): 261 `GetMissionParams` (the value `o result (...)` of `MissionParams`), 182 `GetGameType`, 2 `GetStartPosition` (`o out.x` and `out.z`, a `Vec3` out), 2 `GetNameSlot`, 2 `GetMissionName` (a `text` result), `CountStartPositions` (10 rules walked) and `CountMissionFeatures`.
- `long/mission-build-unit` (a campaign mission, 400 ticks, 26,804 calls, 7.3 MB; beside the set and not replayed in CI, which `console-ilose` alone makes; the only recording that reaches `GetMissionUnit`, `CountMissionUnits` and `GetMissionFeature`, whose calls are 13,000 `GetMissionParams` lines of 248 bytes in any campaign mission): 25,604 `GetMissionParams`, 635 `GetGameType`, 547 `GetMissionUnit` (`o out.name`, `out.ident` and `out.initialMission` are strings behind `cstring` fields), 4 `CountMissionUnits`, 4 `CountMissionFeatures` and 3 `GetMissionFeature`.

`replay_inject_test.exe` is also built with two injections of weapons. The reload-base injection of 150 (the 120 of `(120 - q) * reloadTime`, the reload of a weapon scaled by the health of the unit, in `UpdateUnitWeapons`) is reported at call 393 of `orders-guard-attacked` and at `Unit#2.weapon0ReloadTimer`, the first shot of the recording (recorded 12, replayed 15). The spawn-tick injection of 1 (a projectile that a SPAWN event starts is started one tick late) is reported at call 8 of `console-features`, its first SPAWN, and at `Projectile#0.spawnTick` (recorded 70, replayed 71); no unit of that window reloads, so the reload injection does not come first:

```sh
wine build/modern/replay_inject_test.exe --expect 393 Unit#2.weapon0ReloadTimer tests/replay/weapons/orders-guard-attacked.txt
wine build/modern/replay_inject_test.exe --expect 8 Projectile#0.spawnTick tests/replay/weapons/console-features.txt
```

The recordings of weapons, which the modern build runs as native code in every mode (it has no WASM module yet), in a window of ticks (a window cannot start at tick 0) but `console-ilose`, a whole run. They hold every slot of weapons, and the `Apply` of every kind but EXPLODE_PIECE, which it leaves unrecorded ("Event arguments", "Not recorded"): the damage, claims, releases, slot set-ups, aim answers, detonations, area damage, spawns and removals that reach weapons as events. `UpdateProjectiles` and the detonation and area damage reach every cell of the map (`all-of` over `Game.cells`, a table recorded as changed elements), so each file starts with a first record of every cell, written as runs: 102,400 cells in about 86 KB for `mission-timer`, and 262,144 cells in about 1 MB of the 2 MB of `long-greenhaven-8p`. The nine files are about 10 MB, and none is over 2.3 MB. An aim answer names the callback by its site, `a event.callback Unit#2.weapons[0].aimCob`, and writes its `nAimCobDone`:

- `orders-guard-attacked`, ticks 1 to 250 (`Apply`: 23 DAMAGE, 33 RELEASE, 140 CLAIM, 4 INIT_SLOTS and 39 AIM_DONE): two walkers (turret weapons) and a fighter (a line-of-sight weapon) guard and are attacked; `UpdateUnitWeapons` aims through the script (weapons' START_SCRIPT effect `AimPrimary` through `services.SchedPublish`, `scripts.GetAimFromPosition`, `network.SendScriptCallByName`), fires (`services.RandomInt` for the spread, the reload timer set and counted down), and `WeaponCanReachUnit` (167 calls) and `GetWeaponRange` (32) are called.
- `console-ilose`, the whole run of 100 ticks (1,930 calls; `Apply`: 1 DAMAGE, 9 RELEASE, 2 INIT_SLOTS, 1 DETONATE and 1 REMOVE_PROJECTILES): the load-time `FindWeaponByName` (1,790 calls: the first call names every weapon of the array, `r WeaponName#N.name "..."`, and each returns its weapon, `o result weapondef#N` with `o weapondef#N.flags`, or null), a few `UpdateUnitWeapons` and `GetWeaponRange`.
- `orders-air-fights`, ticks 100 to 260 (`Apply`: 5 RELEASE, 9 CLAIM, 1 INIT_SLOTS and 22 AIM_DONE): fighters shoot at each other (the line-of-sight handler, `WeaponCanReachUnit`).
- `orders-weapon-build`, ticks 700 to 780 (`Apply`: 3 RELEASE, 2 CLAIM and 1 INIT_SLOTS): an anti-nuke silo with an interceptor weapon, `FindTargetableProjectile` (3 calls, null) and the walk of the projectile table, and `UpdateUnitWeapons` of the units that have no target.
- `combat-deaths`, ticks 850 to 1000 (`Apply`: 7 DAMAGE, 7 RELEASE, 1 DETONATE and 2 REMOVE_PROJECTILES): a solar plant and a laser tower self-destruct (the DETONATE of the self-destruct weapon, and the REMOVE_PROJECTILES of a dead unit's shots); the window holds the `UpdateUnitWeapons` of the units around them. It also holds the 151 `UpdateExplosions` calls of the window, which are the ones that show the explosions: the explosion array `Game.debris` (`r Debris#4.velY 237482`, every one of the 300 elements named by its index, not only those up to `Game.count_1491b`: the slot moves the elements after one it removes down by one, so an effect it added past the count in the same call, which keeps the velocity and spin its element held before, ends below the count, with `Debris#N.ref1` and `ref2` the animation cursors that `services.InitGafSequence` starts and `services.StepGafSequence` steps), the pieces the units threw (`r ExplodedSlot#0.piece ExplodedPiece#1`, a ref of `g_explodedPieces`, and `PieceRec#1` behind it), the animation sources the module passes to the service (`r GafSrc1#0.ptr FrameTable#7`, a pointer that the replayer stores as the address of its own empty copy), and the height under each piece and each debris (`i map.GetCellMeanHeight pos=(x,z)`, 2,542 calls).
- `mission-radius`, ticks 1640 to 1650 (`Apply`: 4 RELEASE, 10 CLAIM and 11 AIM_DONE): a missile ship fires its vertical-launch weapon at tick 1644 (`Unit#269`, `weapondef#130`, whose `WeaponFire#130.fire` is `weaponFire[1]`). The shot reads its target (`targeting.GetWeaponTargetPos`, `targeting.GetWeaponTargetUnit`), queries the fire script (`scripts.QueryScript` `QueryPrimary`, `scripts.GetPieceOffset`) and starts it and the unit's rock script as START_SCRIPT effects (`services.SchedPublish`), records the sound and smoke events, pays (`economy.SpendEnergyAndMetal`) and writes the new `Projectile#4`. The window also holds a turret shot and a line-of-sight shot.
- `long-greenhaven-8p`, tick 30518 only: a bomber drops a bomb (`Unit#1783`, `weapondef#5`, whose `WeaponFire#5.fire` is `weaponFire[3]`), the first dropped shot of the suite: it reads its target and the weapon piece, records the sound event, pays and writes `Projectile#2` with the bomber's heading. One tick of the eight-player match is 287 calls.
- `mission-timer`, ticks 261 and 262 (`Apply`: 2 AREA_DAMAGE): a burning feature spreads its fire (features' area-damage effect, `a event.weapon 245`), and the area damage reads `weapondef#245` (a root of the call, "Event arguments"), the cells around the point and the units on them by id.
- `console-features`, ticks 69 to 86 (`Apply`: 2 SPAWN): two meteors (wind_meteors' spawn), each a projectile started at the point with the velocity of the event and the weapon its number names (`a event.weapon 225`, `weapondef#225` a root).

`Game.projectiles` is a table of 300 projectiles (the 300 of the game's `projectileCount < 300` guard), every element a root of `UpdateUnitWeapons` and `FindTargetableProjectile`, so a shot made at index `projectileCount` finds its element and the first call records all 300 with every field the slot reads (about 8,000 lines). `Game.tracked` and `Game.value_142f3`, which a new projectile writes and reads for the camera, are `exempt`.
The recorder's walker holds as many fields as the schema has walkable object fields (references, embedded objects, unit ids, weapons and vectors of objects), a count the generated schema states, so no slot set can overflow it.

A subsystem that imports a slot of another complete subsystem (`weapons` imports `targeting.GetWeaponTargetPos`, and `targeting` and `wind_meteors` import `weapons` slots) is replayed against the import stubs of that seam, not against the other module: `BindSeams` binds the stubs of the seams a recording's subsystem imports from, after the native tables.

The recordings of units (7.0), which the modern build runs as native code in every mode (it has no WASM module yet). They record the part of the seam that no pending subsystem's state is needed for (see "Not recorded"): `UpdateUnitHead` and `UpdateUnitBody` for each unit of each tick (the timers, the health percentage every 30 ticks, the self-repair through `AddRepairProgress` and `mission.GetMissionParams`, an `i` line whose result is the whole `MissionParams`), `AddRepairProgress`, `Unit::CanLoad`, `CanReclaim` and `CanRepair`, `FindUnitType` and `FindUnitTypeId`:

- `console-units` (the whole run, 170 ticks): 726 head and body calls, `FindUnitTypeId` 3 calls (2 at load, 1 at tick 140).
- `mission-victory` (the whole run): 1828 head and body calls, and the load-time `FindUnitType` (7 calls) and `FindUnitTypeId` (6 calls) with the names of all 178 unit types (`r UnitDef#N.unitname`), the only run with `FindUnitType` that is short; a window cannot start at tick 0.
- `orders-air-repair`, ticks 848 to 853 only: `AddRepairProgress` 4 calls (a unit repairing itself, with `services.ScheduleEconomyRequestGranted` and `services.SchedPublish`) and `CanRepair` 2 calls.
- `orders-guard-reclaim`, ticks 250 to 262 only: `CanReclaim` 6 calls.
- `units-transport`, ticks 4098 to 4101 only: `CanLoad` 1 call.
- `orders-guard-attacked`, ticks 28 to 32 only: the health percentage at tick 30.

`replay_inject_test.exe` is also built with the percentage-scale injection of 99 (the 100 of `health * 100 / maxHealth` in `UpdateUnitBody`), and the divergence is reported at call 25 of `orders-guard-attacked` and at `Unit#1.healthPercent` (recorded 100, replayed 99):

```sh
wine build/modern/replay_inject_test.exe --expect 25 Unit#1.healthPercent tests/replay/units/orders-guard-attacked.txt
```

`services.ScheduleEconomyRequestGranted` names its unit as `Unit#N`, not as its block offset, which differs between the recording and the replay: the argument is `{"mode": "object", "kind": "Unit", "ref": true}`, as a unit of the world view is.

The recordings of ai 6.0, which the modern build runs as native code in every mode: `console-sight` (the whole run, 200 ticks: `CreateSquads`, `CreateSquadManager` and `CreatePlayerAI` of both players at tick 0, `ParseDownloadableAiWeightScripts` and `ReparseAiWeightScriptsIfLimitNotSticky` with the console commands they run, `RebuildFeatureCells`, `ResetAIPlayers` (the console `reloadaiprofiles`), `Apply` of SET_SQUAD, PLAN, WEIGHT and LIMIT, and 402 calls each of `UpdatePlayerAI` and `UpdateSquads`, two players over 201 ticks) and `skirmish2`, ticks 250 to 700 only (the timers of a four-player skirmish: the build timer places factories with `ScheduleMapCanBuildAt` and `ScheduleMapCanPlaceUnitFootprint`, and the scout, squad scout, spatial and assault timers fire, 902 calls each of both slots over 451 ticks). Each file starts with a first record of every unit (`UpdatePlayerAI` reaches the whole pool) and every unit type, about 1.4 MB, and a call after that holds only what changed. The injected change is a squad period of 45, the 30 ticks that the squad manager's tick waits before it assigns squads again; it is reported at call 76 of `skirmish2` and at `SquadManager#1.countdown`:

```sh
wine build/modern/replay_inject_test.exe --expect 76 SquadManager#1.countdown tests/replay/ai/skirmish2.txt
```

### Against the WASM module

`--wasm MODULE.wasm` replays the same recordings against `wind_meteors.wasm` instead of the native module, with the
same import formatting, comparisons and checks:

```sh
cmake --build build/modern --target wasm_runtime wasm_runtime_inject
wine build/modern/replay_native_test.exe --wasm build/modern/wind_meteors.wasm tests/replay/wind_meteors/*.txt
wine build/modern/replay_native_test.exe --wasm build/modern/wind_meteors_inject.wasm --expect 2 SimGlobals.g_meteorScheduleGap tests/replay/wind_meteors/meteors-hailstorm.txt
```

The recordings don't depend on how the modern build runs a subsystem. A module of targeting or map, native or
WASM, has to replay them equal, as `wind_meteors.wasm` replays its own.

The WASM replay loads the module through the game's own module loader, as the game does: the
`wasmtime_host.dll` and `modules.json` beside the module file are used, the sim block of the module's memory is attached
as the replayer's block, and the slots are called through the seam the subsystem's glue builds. The glue calls are one
entry per subsystem in a table of the replayer (wind_meteors only for now); the module file's name selects the
entry, and the recording's subsystem must be the same one. Each replay of a file loads the module afresh, so the two
runs of a file start from the same state. A trap or a fuel overrun stops the run with `FAIL`.
`wind_meteors_inject.wasm` is the module built with the gap-scale injection of 31.0 (`wasm_modules.py build-variant`),
and the divergence is reported at the same call and field as for the native one.
