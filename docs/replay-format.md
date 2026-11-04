# Replay recordings

A recording holds the calls one subsystem received in a test-mode run: the state each call could read, the
imports it made with their arguments and results, and the state it changed. `bt-replay 2` is the format.
Recordings are in `tests/replay/<subsystem>/<scenario>.txt`; wind_meteors, economy and targeting have recordings so far. The tables behind the format are `data/replay_schema.json`
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

## Lines

Text, LF line endings. The header is two lines: `bt-replay 2` and `subsystem wind_meteors 1.2` (the interface
version). A subsystem whose annotations are not complete yet (`status` in the schema is `pending`) writes
`incomplete` after the version, and the replayer refuses such a file. The file name carries the scenario.

A call is a line `call <ticks> <Slot>`, where `ticks` is `Game.ticks` when the call starts (`-` before the
game record exists) and `Slot` is a slot of the interface. Calls are in the order they start. A slot called by
another slot of the same subsystem (UpdateWind calls UpdateWindOnStream in a native build) is part of the outer
call and has no line of its own. The lines of a call, indented by two spaces, in this order:

| Line | Meaning |
|---|---|
| `r <field> <value>` | a field of the slot's read set that differs from the last known value of that field |
| `a <name> <value>` | an argument of the slot that is not in the block (`params.radius`, `bank`, `args[0]`, `stream`, a scalar, `player`) |
| `i <import> <arguments> = <result>` | one import call, in call order; its index is its position among the `i` lines of the call |
| `c <handle>#<n>.<Method> <arguments> [= <result>]` | indented by four spaces, right after an `i` line and before its `o` lines: one call the import made on a callback object it was given, in call order (see Callbacks) |
| `o <name> <value>` | right after an `i` line: a value the import wrote through a pointer argument (`o dst "text"`, `o dst x:0102`) or reached through its result (`o weapondef#17.flags 0x00000001`, `o Cell#1.metal 12`) |
| `w <field> <value>` | a field of the slot's write set that differs from the last known value after the call |
| `o <name> <value>` | last in the call: a value the slot wrote through its own pointer argument, or `o result <value>` for its result |

A call without any such line is idle. The last known value of a field is the one on the latest `r` or `w` line
of any earlier call, or the call's own tick for `Game.ticks`. A field with no known value counts as different.
The known values start empty at the first recorded call.

## Objects

Game objects that a subsystem reaches through pointers are written as `Kind#N`: `Player#1`, `Unit#3`,
`UnitDef#2`. A field of one is `Kind#N.field`, and it appears in `r`, `w` and `o` lines exactly as a block field
does, with the same last-known-value rule (the known value is kept per `Kind#N.field`). The kinds, their fields and
their value forms are the `objects` of the schema:

| Scope | Kind | `N` | Where it lives |
|---|---|---|---|
| state | `UnitDef` and the like | order of first appearance in the file, so it is stable across calls | in the block, linked by `Ref` |
| state, `table` | `Player`, `Unit`, `Projectile`, `MovementClass`, `Cell`, `Feature`, `Spot`, `Pool` | the index in the table: `Player#3` is `Game::players[3]`, `Unit#17` is slot 17 of the unit pool, `Projectile#3` is `Game::projectiles[3]`, `Cell#2051` is `Game::cells[2051]`, that is `y*width+x`, `Feature#5` is `Game::features[5]`, `Spot#9` is `Game::spots[9]` and `Pool#0` is `Game::pool` | an array in `Game` or `SimGlobals`, or a `Ref` to one |
| state, `inside` | `UnitResources` of a unit, `Handle` of a feature or a spot | the site, the container and the member: `Unit#17.resourceSlot`, `Feature#5.anim`, `Spot#9.shadow` | a member of another kind (see An object embedded at several sites) |
| index | `weapondef` | the index in `Game::weapons` | in the block's weapon array |
| value | `MeteorParams`, `Vec3` | none: the lines are written with the argument's name, `params.radius`, `pos.x` | an argument struct of one call |
| handle | `file`, `rec`, `bank` | order of first use in the outermost call | the import's own objects, not state |

A kind with a `table` is named by its index in a table that `Game` or `SimGlobals` holds, because the module walks it by
address (`for (u = begin; u <= end; u++)`) and reaches its elements by pointer arithmetic (`cell - g_game->cells`). The index
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
| `alloc` | for a `ref` table: the elements the replayer makes, and stores in the count member, when a recording first names an element and the `Ref` is null |

A `ref` table whose `count` is a product of two block fields (`Cell` is `width*height`) has no `alloc`. The replayer allocates the
product of the fields' current values and stores nothing, since neither factor is the count. The fields must therefore be set before
the first element is named: `Game.width` and `Game.height` are in the read set of every slot that reaches a `Cell`, so their `r`
lines come before the first `Cell#N`. A slot's annotation adds a field to its read set with `"reads": ["Game.width", "Game.height"]`,
and `replayschema.py --check` reports a complete subsystem's slot that reaches such a kind with a factor missing from its read set.

`Player` and `Unit` are instances of this form (`players` is `{"base": "game", "member": "players", "count": 10}`, `units` is a
`Ref` with `poolCount` and `alloc` 5001), so the text of every committed recording is the same as before the form had a name.
The table member is not an untracked target: a row that touches it needs no `exempt` entry.

A pointer field is written as the object it points at (`Unit#1`) or `null`. The reachable objects of a call are the
object arguments (`a player Player#1`) and every object their pointer fields lead to, following the pointer fields
that are in the slot's read or write set for the kind. At entry the read-set fields of each reachable object are
written as `r` lines, at exit the write-set fields as `w` lines. The walk order (arguments, then breadth first, fields
in table order) numbers the objects named by first appearance, and the replayer walks its own block the same way. The replayer allocates
`Kind#N` in the block when a line first names it, stores `Ref` pointers as offsets from the block base, and reports a
changed field at its `Kind#N.field`. A pointer to an object no earlier line named is written `Kind#?`.

A field of kind `ptr` is a raw pointer, not a `Ref`, to an object of a state kind: `{"name": "src", "kind": "ptr", "target": "FrameTable"}`
for `Handle::src`. It is written exactly as a `ref` is, `FrameTable#N` or `null`, but the replayer stores the address of its own copy
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
condition false. A `unitid` field has a condition too, which says that its bytes are an id (a value of `-32768` in the next slot),
and its field is not written as a line.

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
that takes it (`i services.StepGafSequence Spot#3.anim`, `o Spot#3.anim.value 7`) and the argument values. The replayer resolves such a
name to the bytes at the site inside its container, and never allocates a separate object. A free-standing object of the same kind, one on a
caller's stack, is named by order as any other, `Handle#1`.

A container named by order (not a table kind) is named before its embedded object is: the walk names it, as it reaches the container first.

```
  r Spot#3.anim.index 2
  i services.StepGafSequence Spot#3.anim = 1
  o Spot#3.anim.value 7
  i services.StepGafSequence Handle#1 = 1
```

### Features, spots and the pool

`Feature` is the table `Game::features`: `Feature#N` is the feature type `N`, and it has `featureCount` elements, the size of the array the loader grows by one type at a time. `Spot` is the 0x800 entries of `Game::spots`, 0x30 bytes each, and `Pool` is the five words of `Game::pool` (`entries`, `featureUnit` and the heads of the used, rest and free lists, which link the spots by their `prev` and `next` indexes). `Game::spots` is the same word as `Pool.entries`.

Both structs are unions of views, and a field is recorded on one side of a union only:

| Kind | Bytes | Written as |
|---|---|---|
| `Feature` | `object` and `filename` share their first four bytes | `object` (a `ptr` to an `Object3do`) when bit 0 of `flags16` is clear, `filename` when it is set; both fields have a `when` on that bit |
| `Feature` | `flags16`, with `flags`, `flagsHigh`, `steep`, `noobject`, `animating` and the other bits | one `u16` field; the other names are `aliases` of it |
| `Feature` | `seq`, `seqshad`, and the six pointers that `seqburn` to `seqreclamateshad`, `burn`, `death` and `animIds` view | `ptr` fields to a `FrameTable`, the kind of an animation entry; `animTable`, `shadowTable`, `burn`, `death` and `animIds` are `aliases` |
| `Feature` | `footprint`, `footprintX`, `footprintZ` | one `point16`, with the other two as `aliases` |
| `Spot` | the two cursors `anim` and `shadow`, or `state`, `pos` and `vel`, or `frame`, `pos2` and `animIndex` | the cursors are `embed` fields of `Handle` with a `when` that reads bit 0 of `flags16` of `features[feature]` (`via`) and wants it set; `state`, `posX` to `velZ` have the same `when` and want it clear; `pos2` is an alias of `pos`, and `frame` and `animIndex` are the `index` and the `src` of the cursor, aliases of `Handle.index` and `Handle.src` |
| `Spot` | `x` and `z`, `cell`; `burnTime`, `timer`, `animBits`, `animHi`, `animLo`; the bits of `flags` | `cell` (a `point16`), `timer` and `flags` (a `u8` each), with the other names as `aliases` |

The `Handle` at `Feature#N.anim` and `Feature#N.shadowAnim`, and at `Spot#N.anim` and `Spot#N.shadow`, is named by its site everywhere, because a table kind finds the container of a pointer by its address, whether or not the walk has named it. A `Handle` the loader initialises in the entry `features[featureCount]`, one past the end of the table until `LoadFeatureType` returns, has no container yet and is named by order.

```
  r Feature#5.flags16 1
  r Feature#5.filename "tree1"
  r Spot#9.anim.index 2
  r Spot#9.timer 12
  i services.StepGafSequence Spot#9.anim = 1
```

A pair of pointer fields that bound a slice of a table is a closed range: the schema marks the end field with
`"range_end_of": "unitsBegin"`, and `Player::unitsEnd` points at the last unit of the player's slice (the walk is
`u <= end`), not one past it. The end pointer is written as the object it points at, `Unit#249`, like any pointer. The
walk reaches every unit of `[begin, end]` in table order, whether or not the module's call touched it, because the
module's loop reads each one; when only one field of the pair is in the slot's sets, only that pointer is followed.

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
| pointer to an object (`ref` or `ptr`) | `Unit#17`, `UnitDef#2`, `weapondef#17`, `Unit#17.resourceSlot`, or `null` |
| object handed to an import | `file#1`, `rec#2`, `bank#1` |
| random stream | `main` or `load` |

No address, no padding byte, no time and no module mode is ever written.

## Arrays

A field that is an array (kind `array`) has a length that is an expression over other members of the same object (or of
`Game` or `SimGlobals`, for a block field), and holds either a `Ref` to its elements (`"ref": true`) or the elements
themselves. The annotation, for `Player::explored` and `Game::visibilityMask`:

```json
{"name": "explored", "kind": "array", "elem": "u8", "ref": true, "length": "exploredSize"}
"visibilityMask": {"kind": "array", "elem": "u16", "ref": true, "length": "(width>>1)*(height>>1)"}
```

`elem` is the element type (its size is what counts: the elements are written as the bytes of memory), and `length` is an
integer, or one or two factors joined by `*`, each a member of the object, `member>>k`, or an integer. A member is one, two or
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
| nullable struct in | `{"mode": "struct", "kind": "Vec3", "dir": "in", "nullable": true}` | `a pos null` for a null pointer, else the lines of the fields; the replayer passes NULL |
| array of structs in | `{"mode": "structs", "kind": "Point16", "count": "count"}` | `a points[0].x 1`, `a points[0].y 2`, `a points[1].x 3`, and so on for each of `count` elements (`count` names another integer argument); `a points null` and `a points []` for a null pointer and an empty array |
| struct by value | `{"mode": "value", "kind": "Angles16"}`, for an argument or `result` | `a kind (5)`, `o result (-3,40000,7)`, and in an import line ` (5)` and ` = (-3,40000,7)`: the values of its fields in order, in parentheses, written as the field's kind writes them |

`MissionType` (one field, `index`) and `Angles16` (`bank`, `heading`, `pitch`) are the kinds of the by-value form so far. The
count of the array form is read from the count argument's own line, so the argument must be recorded. A mismatch of a result
by value is reported at `result`, one of an element at `points[1].y`.

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
| `vtable` | a concrete kind | the global the vptr points at (`extern VirtualFunction const g_approachRadiusVtable[12]` in `src/data/vtables.cpp`); the tool declares it in the schema and `static_assert`s that the type is polymorphic and derives from the base |
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
its kinds` (naming the symbol, `g_pathOrderVtable`, when the vptr is the vtable of another kind of the schema). Only a vtable that is
a global of the sources can be a kind's vtable. A vtable the compiler emits from a class has no name to compare with.

### Code pointers

A field of kind `code` holds a pointer to a function that is not a vtable. It is written `<table>[<index>]`, the index of the
function in the named table, or `null`. The tables are the `code_tables` annotation:

```json
"code_tables": {"orderNotify": {"entries": ["ReadyOrder", "StandbyOrder", ...], "headers": ["src/orders/unit_orders.h", ...],
                                "fill": "orders.RegisterAllOrderTypes"}}
{"name": "notify", "kind": "code", "table": "orderNotify"}
```

```
  r OrderTableEntry#1.notify orderNotify[44]
```

`entries` are functions in the order of the table (the `notify` pointers `RegisterAllOrderTypes` copies into `g_missionOrderTableVec`,
in the order it registers them), each declared in one of the `headers`, which have to be `includes` of the schema. The tool writes the
table as `(const void*)StopOrder` addresses into the schema header, so the recorder finds the index of the game's function and
the replayer, linking its own functions, stores its own address for the index. `fill` is the slot (`seam.Slot`) that fills the game's
table, for the subsystem that owns it. A function that is in none of the entries is an error that names the table (the recorder
writes `orderNotify[?]`), and an index past the table is refused by the replayer. `GroundPickupOrder` is not an entry, because
no header declares it with the type its definition has.

## Callbacks

Some imports call back into the recorded subsystem while they run: `map.VisitObjectsInRange` calls `visitor->Visit(unit)` for each
unit it finds, and the scan visitors of the victory conditions return a result. Each call the import made on the object it was
given is a `c` line right after the import's `i` line and before its `o` lines, indented as the `i` line plus two spaces, in call order:

```
  i map.VisitObjectsInRange (100,200) 300 visitor#1
    c visitor#1.Visit Unit#3
    c visitor#1.Visit Unit#9
  i fixture.Scan scanner#1 = 0
    c scanner#1.VisitUnit Unit#3 = 1
    c scanner#1.VisitUnit Unit#9 = 0
```

The object is a handle named by the interface's `handle` and numbered in the outermost call by first use, like `file#1`. The
arguments and the result are written by the same modes as the arguments of a slot (`scalar`, `text`, `object` and `value`; a result
is void, a scalar or an object). What a callback writes to the block is not a line of its own, it shows in the `w` lines of the
outer call, as any write of the module does. The annotation names each interface once, the argument of the import names the
interface:

```json
"callbacks": {"UnitScanVisitor": {"header": "src/game/victory_conditions.h", "handle": "scanner",
              "methods": [{"name": "VisitUnit", "ret": "int", "args": [{"name": "unit", "type": "Unit*", "mode": "object", "kind": "Unit"}]}]}}
{"mode": "callback", "callback": "UnitVisitor"}
```

`methods` are the virtual methods of the class in the header, in declaration order (the tool checks the names against the header, and a
callback interface has no virtual destructor). The tool writes the classes that do the work into `src/harness/replay_callbacks.h`
and `tests/replay_callbacks.h`. A `callback` argument is allowed on a slot that a subsystem lists in its `imports`, and only there.

The recorder hooks the object's methods for the duration of the import: it points the object's vptr at a class of its own that
derives from the interface, so each call is recorded and then made on the object's own method, and it puts the vptr back when the
import returns. The object keeps its address and its identity, so a callback the callee keeps for later (the `Callback` of
`StartScriptWithArgs`) is the module's own object after the import, and a call it makes later is not part of any import. Two limits end the
run, naming the call: a callback made inside another callback, and an import made inside a callback, are not recorded, because their
lines could not be in call order. A callback that frees its own object is put back only when the vptr word still holds the recorder's.

The replayer stub makes the recorded calls in order, on the object the module passed, with the recorded arguments, after it has
matched the `i` line and before it applies the `o` lines. It compares each result with the recorded one and reports a difference at
`<import>.c[<k>]` (`import 0 map.VisitObjectsInRange.c[1]: recorded 0, replayed 1`), and a call recorded on another handle than the
object's at the same place.

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
embedded `AiUnitList`. Only the fields of an object are vectors, not the block fields.

The walker reaches the object of every element of an `object` vector through its `Ref`, as it does through a `Ref` field, so the units a
window holds only in a vector are recorded and restored. For a `value` vector a bracket line whose capacity or length changed is followed by the
fields of the elements it newly holds, in full, because the recorder cannot know what a new slot held before.

The replayer allocates `cap` elements with `SimAlloc`, which is what `BlockVector` allocates with, stores the three words, and fills the
elements. A vector of structs whose capacity changes keeps its old elements, as the module's `Insert` does when it moves them. A
difference is reported at `<field>.length`, `<field>.capacity`, `<field>[<i>]` (the hex of a scalar element or the handle of an object
element) or `<field>[<i>].<f>` (a field of a struct element), for example `PlayerAI#1.counts.length: recorded 0, replayed 2`.

## Snapshots

Some imports return a buffer that the slot then fills, and the bytes it leaves there are the output that counts: `SimCaptureAlloc(bytes,
&offset)` hands out a buffer of the capture arena for a radar snapshot. A service whose result is `snapshot` (`"result": "snapshot",
"bytes": "bytes", "offset": "offset"`, `bytes` naming its int argument and `offset` its skipped out argument) is written with the size
and a handle, numbered in the outermost call by first use, and the bytes follow at the end of the call:

```
  i services.SimCaptureAlloc 64 = snapshot#1
  w Game.timer 0x00000000
  o snapshot#1 x:0100000002000000...
```

No line is written for the offset, because it is the replayer's own. After the `w` lines and the closing `o` lines, the call has one
`o snapshot#<n> x:<bytes>` line per snapshot it allocated, the buffer's bytes as the slot left them, with a long buffer in several
lines of the same name, in order, of at most 512 bytes (the lines are joined). A null result (the arena was full) is `= null` and has no
line.

The replayer's answer allocates `bytes` from an arena of its own, zeroed, and returns that pointer and its own offset in `offset`. At
slot exit it compares each buffer with the recorded bytes and reports a difference at `snapshot#<n>[<byte>]` (recorded and replayed
byte), at `snapshot#<n>.length` when the sizes differ, and at `snapshot#<n>` when a snapshot was recorded and not allocated, or the
other way round. The `o snapshot` lines are never the outputs of the import before them. The tool writes the recorder's and the
replayer's code for a `snapshot` service from its spec; `SimCaptureAlloc` is the one service of the kind, hooked in `sim_radar.cpp`
with `ReplayImport call(RI_SIM_CAPTURE_ALLOC); call.Int(bytes); call.Ptr(offset);` and `return call.Result(buffer);`. When a module
links `sim_radar.cpp`, the replayer does not define the function: the answer calls the real one and compares the buffer it returns. The self-test allocates two buffers
through a function with the signature of `SimCaptureAlloc`, one of 700 bytes, and checks the lines, an equal replay, a changed byte,
a snapshot the stub did not allocate and a buffer of another size.

## Fields

Block fields are named as `data/rwsets.csv` names them: `Game.windCounter` for `Game::windCounter`,
`SimGlobals.g_meteorActive` for the global `g_meteorActive`. `rng.main`, `rng.load` and `rng.seed` are the two
generator streams of the block head and `g_randomSeed`; they are in the sets of the slots that draw.
`weapondef#17.flags` is the flags word of a weapon the module reads through a pointer it was given.

## The schema and its annotations

`data/replay_schema.json` has the `annotations` (written by hand, kept as written by the tool) and the generated
`fields`, `objects` and `subsystems` (one entry per simulation subsystem, with its slots and their read and write
sets). The annotations hold what no header says:

- `game_fields`, `sim_fields`, `exempt`: the block fields the recorder tracks, and the targets it ignores.
- `objects`: each kind's C++ type, scope and fields (name, value kind, target kind of a pointer). The generated header
  checks every size against the real struct.
- `rules`: a hand rule for each row the field sets cannot name: a `field_escape` row, or a `deref_read` or
  `deref_write` on a bare `*float`, `*short`, `*int` and the like. A rule is matched by `kind` and `target` (and by
  `functions`, when it lists the functions whose rows it covers: `*float` is the `energyMake` of a unit account in
  `AddIncome` and the extraction rate of a unit in `UpdateMetalExtraction`), and any row, not only an escape or a bare
  pointer, may be given one when the rows name only a part of what the code touches (`Point16::x` is a member of `Unit::cell`
  and of `Unit::footprint`). `whole-object` records every field of the named object; `index-array` records the weapon array
  through `weapondef#N`; `fields` adds the object fields it lists under `reads` and `writes` (`Unit.extraction`,
  `UnitResources.energyMake`), so that a write through a pointer is recorded as a `w` line of the field it lands on;
  `all-of` makes every slot of a table kind a root of the call (`Game::players` is walked by index). `replayschema.py
  --check` fails for a row of a subsystem function with no rule and for a rule that matches no row.
- `objects`' extras: `table` (an object, see Objects) names a kind by its index in a table, `inside` (a list of `{"kind": "Unit",
  "member": "resourceSlot"}`) names every member of another kind that the kind lives in (`Unit#17.resourceSlot`), a field of kind
  `embed` leads to such an object, `range_end_of` marks the end pointer of a closed range, and `aliases` (`Unit::cloakOn`) are
  the other targets of the rows that one field records (a bit field is recorded as its word). `replayschema.py --check` fails for a site
  that is no `embed` field of its container, for one that two kinds claim, and for the old single-site object form.
- `when` (an object, see A condition on a field): `{"field", "equals"}` with an optional `mask`, `via` and `kind`, on a field of
  a state kind. `replayschema.py --check` fails for a field it reads that is not an integer or flags field, that has a `when` of its own, or
  a mask and a value that can never meet.
- Field kinds that point rather than hold a value. `unitid` is a 16-bit unit id (`weapon0TargetX` holds either an x position or
  a unit id; `weapon0TargetUnit` is the same bytes read as an id) that the walker follows to `Unit#<id>`, but only when its `when`
  field (`weapon0TargetZ`) equals the given value (-32768, the sentinel that says the slot holds a unit); it is never written
  as a line, and the unit it leads to records only the fields of its `reach`. A `weapon` field leads the walk to `weapondef#N`, whose fields the slot reads
  are recorded as `r weapondef#17.flags` lines. A field may name a `member` of its type that is not its own name
  (`weapons[1].aimTargetXOrUnitId`, `pos.x`), so that an array element or a part of a `Vec3` is a field.
- `reach`: a list of `Kind.field` names with an explicit `depth` (the number of objects from the root to the farthest, at most 2),
  on a `unitid` field and on a `units` argument. An object reached through it is recorded with only the listed fields, and
  the walk follows only the listed pointers, so `Unit.def` with `UnitDef.flags1` is depth 2. `replayschema.py --check` fails
  when the depth is not the one the list has, or a name is no field.
- A field a slot writes through a pointer it cannot name exactly (a weapon slot picked by an index) is also in its read set,
  so that a change made outside the seam is recorded at the entry as an `r` line, not mistaken for a write of the module.
- `exempt_types`: `{"OpenHeap": "reason"}`, a struct type that is no kind and is genuinely local (a stack copy read from a bank, a
  union summed in a local). A field row (`field_read`, `field_write`, `field_maybe_written`) of a complete subsystem on a type that
  is none of these is an error naming the row (`field_read OpenHeap::count`): the type of a kind or of a value kind, an alias of a kind's field
  (`aliases`), a type in `exempt_types`, a target in `exempt` (one member of a type, for an anonymous struct: `<anon>::whole`), a hand rule for
  the row, the table of a seam (`Seams`, `UnitsSeam`), or a service class the subsystem lists in `services` (`TdfFile`, `CommandArgs`). A type with no
  reason is an error. A pending subsystem is not asked.
- `indirect_call` rows of a complete subsystem. Every `indirect_call` row of a function behind a recorded slot, and of every function
  `through` adds, is an error (`pending indirect`) unless one of these accounts for it: a call through a seam table (`[UnitsSeam::SetStateBits]`)
  to a slot the subsystem lists in `imports`, or to a slot of its own seam whose functions are all behind the slot being checked; a
  `virtual Class::Method` dispatch where `Class` is a kind with `kinds` (`poly`) or a kind with a `vtable`, and the `Method` of each concrete kind's type is
  among the functions (a concrete type that defines no `Method` of its own is not asked, and the base's own `Method` is asked only when some concrete
  type does not define one); or a `through` entry in the slot's annotation.
- `through` (a slot annotation): `{"<row target>": ["fn", ...]}` adds the named functions to the slot, so that their sets and their own
  `indirect_call` rows count as the slot's, for a table of code pointers such as `g_missionOrderTableVec[].notify`.
  `{"<row target>": {"reason": "..."}}` says that the call does nothing on the recorded paths. The key is the whole target of the row
  or the name in its last brackets (`OrderTableEntry::notify`). `replayschema.py --check` fails for an entry with no functions and no reason, a function that is
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
| `struct` (`dir` in or out) | a pointer to a value kind, `nullable` when in | `a params.field value` in, `o params.field value` out; an out struct with `"when": "result != 0"` is recorded and compared only when the call returns nonzero, for a call that leaves the struct untouched otherwise (`GetWeaponTargetPos`, whose caller passes a stack local) |
| `structs` | a pointer to a value kind and a `count` argument | `a points[0].x 1` for each element (see Struct arguments) |
| `value` | a value kind by value | `a kind (5)` (see Struct arguments) |
| `argv` | `CommandArgs*` | `a args.count 2`, `a args[0] "x"` |
| `bytes-in`, `bytes-out` | `void*` with a `length` argument | ` x:0102` in an import line, `o dst x:0102` after it (`bytes-out` is as long as the result, at most the length) |
| `weapon` | `WeaponDef*` | `weapondef#17` |
| `vec3` | `Vec3*` with a `label` | `pos=(x,y,z)` in an import line, or `pos=(x,z)` with `"axes": "xz"` for an import that reads only those (`GetGroundHeight`); with `dir: out`, nothing in the import line and `o out (x,y,z)` after it, which the replayer's stub writes |
| `callback` | `UnitVisitor*` and the like, an import's argument | ` visitor#1` in the import line, and a `c` line for each call the import made on the object (see Callbacks) |
| `units` (`dir: out`) | `std::vector<Unit*>*` an import fills | `o out [Unit#3,Unit#9]` after the import line, then `o Unit#3.flags 0x00000021` for each field of the `reach` of the elements that changed since last written; the stub pushes the named units and restores those fields |
| `pending` | anything else | `?` |

A result is `scalar`, `value` (`o result (-3,40000,7)`), `object` (compared by its name, `o result Unit#12` or `o result null`), `text`, or `weapon` or `object` with `reach` fields recorded as `o weapondef#N.flags` or
`o Cell#N.metal` lines (the replayer returns an object it makes, with those fields set).
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

`uv run tools/replayschema.py` also writes `src/harness/replay_schema.h` (the tables), `src/harness/replay_hooks.inc` (the
recording copy of every seam table, its argument hooks and the import hooks) and `tests/replay_slots.h` (the slot
runners, import stubs and native seam tables of the replayer). The services imports (`GameRand`, `RandomInt`, `FatalError`, `BuildDataPath`, the TDF and command classes) are the `services`
annotation: each entry names its declaration in `interfaces/services.h` (checked against the header) and how its arguments,
result and `o` lines are written. From it the tool writes `src/harness/replay_services.h` (the ids and names),
`src/harness/replay_services.inc` (the recorder's lines) and `tests/replay_services.h` (the replayer's answers). The one thing
it cannot write is the hook itself, `ReplayImport call(RI_X); call.Int(..)` at the top of each function in the game source:
those functions are the services themselves, reached by direct calls with no table to wrap, so the hook has to be in the
function; `replayschema.py --check` fails when a hook's arguments differ from its entry or an entry has no hook.

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
| `commandargs` | `["x","y"]` or `null` | the strings of a `CommandArgs` |

| Result | Written as |
|---|---|
| `int`, `double`, `text` | ` = 7`, ` = f64:0x...`, ` = "text"` or ` = null` |
| `handle` (`kind`) | ` = rec#2` or ` = null` |
| `object` (`kind`, `ctype`, `reach`) | ` = GafFrame#1`, then `o GafFrame#1.width 64` for each field of the `reach` |
| `alloc` (`size`, `tag`) | ` = alloc#1`, or ` = PlayerInfo#1` when the annotation `alloc_tags` names the tag |
| `snapshot` (`bytes`, `offset`) | ` = snapshot#1` (see Snapshots) |
| `arg` (`param`) | nothing: the function returns the argument (`GetFieldFixed` returns `dst`) |

An out is an `o` line after the import line: `o dest "Armada"` for a text written through a pointer, `o dst 98304` for an int
(`{"label": "dst", "int": "dst"}`), `o file#1.current rec#1` for a record a file now points at. An entry may also give `pre` lines
(the replayer builds the formatted text of `FilePrintf` from its own arguments), `effect` lines (the replayer's definition does what
the function does to its object: `count = 0;` for `InitArgs`), `cast` and `into_size`.

A cursor that a service starts or steps is an `object` argument with a `reach`, so each call records the fields it left in it, a `ptr` among
them (`InitGafSequence` and `StepGafSequence`, whose `Handle::src` is the entry or `null`):

```
  i services.InitGafSequence Handle#1 FrameTable#1 0
  o Handle#1.index 0
  o Handle#1.value 4
  o Handle#1.kind 1
  o Handle#1.src FrameTable#1
  i services.StepGafSequence Handle#1 = 1
  o Handle#1.index 1
  o Handle#1.value 7
  o Handle#1.kind 1
  o Handle#1.src FrameTable#1
```

One line per group, from the self-test:

```
  i services.TdfRecord_FindSubRecord rec#1 "units" = rec#2
  i services.TdfRecord_GetFieldFixed rec#3 "scale" 5
  o dst 98304
  i services.FilePrintf "%d units" "3 units" = 7
  i services.Translate "key" = "Teksti"
  i services.ComputeChecksum x:0102ff 3 = 1234567
  i services.SimAlloc 64 = alloc#1
  i services.SimAllocTagged "Player Info" 185 = PlayerInfo#1
  i services.SimFree alloc#1
  i services.RecordSimEvent 2 1 [3,4] 2
  i services.FindSlotByDpid 77 = 2
  i services.GetGafFrame FrameTable#1 2 = GafFrame#1
  o GafFrame#1.width 64
```

The services of the files and data group are `FilePrintf` (the format and the formatted text) and the TDF calls that the subsystems make on records
and files; the commands group is `Translate`, `CommandArgs::InitArgs` and `ComputeChecksum`; sim plumbing is `SimAlloc`, `SimAllocTagged` (the size
and the tag are compared), `SimFree`, `RecordSimEvent` (kind, flags, args and count), `SimCaptureAlloc` and the slot lookups `FindSlotByDpid` and
`GetSlotDpid`; the load reads are `LoadGaf`, `LoadAnimGaf`, `FindGafEntry`, `GetGafFrame`, `GetGafSequenceFrame`, `Load3do`, `LoadObject3d` and
`GetObjectHeight`, with results that are objects of the load kinds `Gaf`, `FrameTable`, `GafFrame` and `Object3do` and the fields that
the subsystems read as the `reach`. The `HAPI_*` file calls and the boxes of `HapiBank` are not services: they are slots of the `files` seam
and an import of the subsystems that call them, recorded as any import is. A hook records only for the thread that opened the slot, so
the calls of the loading thread or the drawing code that overlap a slot are not its calls.

A service whose definition is in a source that the replayer links (`SimAlloc`, `SimAllocTagged` and `SimFree`, which `block.cpp` defines) is not
defined by the replayer. The hook in the real function asks `call.Answered()`, which is true only while the replayer runs a slot, and the
generated answer takes the place of the function: it checks the `i` line the module would have written, makes the real allocation and names the
block as the recording does (`alloc#1`, or the object of the tag's kind) or, for `SimFree`, lets the real free go on. The replayer's own
allocations are never answered. A service in a source nothing links keeps the definition the tool writes.

`ExecuteCommandText` is re-entrant. The commands it runs through the slots named `Cmd...` are `c` lines after its `i` line, in call order,
as the calls of a callback are:

```
  i services.CommandArgs_GetIntArg 1 0 = 1
  i services.ExecuteCommandText "meteor 1" 8 ["x"] -1 = 1
    c wind_meteors.CmdMeteor ["meteor","1"]
```

The arguments of a nested call are written by the modes of a slot argument and the result follows ` = `. What a nested call writes shows in the
`w` lines of the outer call. The services a nested call makes are ordinary `i` lines, and they come before the `i` line of
`ExecuteCommandText`, because that line is written when the function returns; the stub finds its own line among them, makes the nested
calls through the seam table of the module with the cursor set back over those lines, and reports a result that differs at
`<import>.c[<k>]`. A command that is not a seam slot is not recorded, a slot import of the nested call is not recorded, and a command run
inside a nested command fails the recording.

## Replaying

`tests/replay_native_test.cpp` replays a recording against the native module (`replay_native_test.exe`, built by
`cmake --build build/modern --target replay_native_test`, run under Wine in `modern-compile`, with the WASM replays below).
The subsystem is read from the recording's header; the module sources of a subsystem come from `data/subsystems.csv`
(`REPLAY_SUBSYSTEMS` in `CMakeLists.txt`).

```sh
wine build/modern/replay_native_test.exe tests/replay/wind_meteors/*.txt
```

One block is kept across the calls of a file: each `r` line is restored into it, the outer slot runs, every import is
answered from its `i` and `o` lines, and the write set and the closing `o` lines are compared with the file. A random
draw returns the recorded result, and the stream words advance as the game's do, so `rng.*` is checked like any field.
A call that differs is reported with the first import or field that differs and both values, and the file stops
there. Each file is replayed twice and the two results must be identical.

`--self-test [FILE]` first round-trips a small graph of objects (a `Player` with two `Unit`s sharing a `UnitDef`)
through the text form and reports a changed field at its `Kind#N.field`. It then does the same for each form that no
recording uses yet: the indexed tables (`Projectile#3.speed`, `MovementClass#5.maxSlope`), the sized arrays (whole lines,
chunks, deltas and a null, for an object and a block field, with a changed element reported at its field and byte), a nullable
struct, an array of structs and a struct by value (a changed field reported at `pos.z`, `points[1].y`, `result`), polymorphic objects (an
`Order` that leads to an `ApproachRadius` and a `RingApproach`, rebuilt with the replayer's vtables, and an `OrderTableEntry` whose `notify`
is a code pointer), block vectors (a `PlayerAI` whose unit list, cells and counts are pushed to, grown, emptied and compared) snapshots (two buffers of a fake `SimCaptureAlloc`) and raw pointer fields (a `Handle` whose `src` is a `ptr`, rebuilt and compared, with a changed `src` reported at `Handle#1.src`; a `ptr` to an object no line named, restored to an empty copy) and masked views (a union whose live view switches on a masked bit, or on a bit of another object, with the dead view neither written, compared nor restored) and an object embedded at two sites (named by its site in the walk and in a service line, and restored inside its container) and services (the files, commands, sim plumbing, load reads and GAF cursors, with their changed arguments reported at the import) and callbacks (a visitor of the module called twice by a fake import, recorded as two `c` lines and replayed on a
second object, with a changed result reported at `import 0 fixture.Scan.c[0]`). With a
recording it finally mutates the recording and expects each mutation reported. `replay_inject_test.exe --expect CALL FIELD FILE` runs the module built
with one changed constant and expects that divergence.

The same exe replays economy, whose module is native code in every mode: `replay_inject_test.exe` is built with
`BT_REPLAY_INJECT_WIND_SCALE=3.0`, which scales the wind income in `UpdatePlayerEconomy`, and the divergence is reported at the
first call whose wind generators were on and at `Player#0.energy`:

```sh
wine build/modern/replay_native_test.exe tests/replay/economy/*.txt
wine build/modern/replay_inject_test.exe --expect 5501 Player#0.energy tests/replay/economy/duel-shore-to-shore.txt
```

The recordings of economy and the scenarios they come from: `duel-shore-to-shore` (wind generators from tick 1620, metal
extractors from tick 480), `duel-coast-to-coast` (tidal generators from tick 3330, extractors) and `load-midgame` (the
accounts read back from a saved game, `LoadUnitAccounts`, and extractors) and `ten-ring-atoll`, ticks 6270 to 6270 only, where
the first metal maker earns income (the `makesMetal` branch of `UpdatePlayerEconomy`). `UpdatePlayerEconomy` walks every unit of the
player's slice of the unit table on each call, so a recording carries the `r` lines of all the slice at the first call and the
changes after it.

`replay_inject_test.exe` is also built with `BT_REPLAY_INJECT_TARGET_SCALE=0x6666` (the 0xcccc of `GetWeaponTargetPos`, the part of the
distance a target's body offset moves the aim point), and the divergence is reported at call 352 of `skirmish4-long` and at `pos.x`, the
out `Vec3`:

```sh
wine build/modern/replay_inject_test.exe --expect 352 pos.x tests/replay/targeting/skirmish4-long.txt
```

The recordings of targeting, which is native code in every mode: `orders-guard-attacked` (units that guard and are attacked: every
slot but `SetWeaponTargetPos`, `SetWeaponTargetUnit` 315 times, candidates from `ai.GetVisibleEnemiesInRadius`, `ChooseWeapon`),
`orders-tier2-spider` (`SetWeaponTargetPos` 8 times, and the ground positions of `GetWeaponTargetPos`) and `skirmish4-long`, ticks
25750 to 25770 only, where a unit with more than five kills fires at a moving unit (the scale of `GetWeaponTargetPos`). Each
`GetWeaponTargetPos` that returns nonzero records the `pos` it wrote (`o pos.x`); one that returns 0 leaves the stack local of its caller untouched, so
nothing is recorded of it.

### Against the WASM module

`--wasm MODULE.wasm` replays the same recordings against `wind_meteors.wasm` instead of the native module, with the
same import formatting, comparisons and checks:

```sh
cmake --build build/modern --target wasm_runtime wasm_runtime_inject
wine build/modern/replay_native_test.exe --wasm build/modern/wind_meteors.wasm tests/replay/wind_meteors/*.txt
wine build/modern/replay_native_test.exe --wasm build/modern/wind_meteors_inject.wasm --expect 2 SimGlobals.g_meteorScheduleGap tests/replay/wind_meteors/meteors-hailstorm.txt
```

`tests/replay_wasm.cpp` loads the module through `lib/wasmloader` as `src/wasm/modules.cpp` does in the game: the
`wasmtime_host.dll` and `modules.json` beside the module file are used, the sim block of the module's memory is attached
as the replayer's block, and the slots are called through the seam the subsystem's glue builds. The glue calls are one
entry per subsystem in a table at the top of that file (wind_meteors only for now); the module file's name selects the
entry, and the recording's subsystem must be the same one. Each replay of a file loads the module afresh, so the two
runs of a file start from the same state. A trap or a fuel overrun stops the run with `FAIL`.
`wind_meteors_inject.wasm` is the module built with `BT_REPLAY_INJECT_GAP_SCALE=31.0` (`wasm_modules.py build-variant`),
and the divergence is reported at the same call and field as for the native one.
