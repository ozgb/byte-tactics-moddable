# Trace format

This page defines the three files a test-mode run writes: `trace.txt`, `dump.txt` and the stored
digest files. It is enough to compute every row of `trace.txt` from the game state, and to
recompute the same rows from a `dump.txt`. It is written in terms of the state block layout and
`data/trace_schema.json`, so an engine in any language can follow it without the reference's source.

The format is `bt-trace 10`. The scenarios that make a trace, and the switches of a test-mode run, are in `docs/scenario-format.md`.

Contents:

1. [Conventions](#1-conventions)
2. [trace.txt](#2-tracetxt)
3. [The hash](#3-the-hash)
4. [The schema file](#4-the-schema-file)
5. [Pointer IDs](#5-pointer-ids)
6. [What a row hashes](#6-what-a-row-hashes)
7. [Sections of the phase rows](#7-sections-of-the-phase-rows)
8. [Sections of the world row](#8-sections-of-the-world-row)
9. [The per-tick digest](#9-the-per-tick-digest)
10. [dump.txt](#10-dumptxt)
11. [Rules that decide hashed bytes](#11-rules-that-decide-hashed-bytes)
12. [Format numbers](#12-format-numbers)
13. [Appendix: layout constants](#13-appendix-layout-constants)

## 1. Conventions

- All multi-byte values are little-endian. "u8", "u16", "u32" and "i32" are unsigned or signed
  integers of 8, 16 or 32 bits; "i16" is a signed 16-bit integer.
- **The state** has two parts. The **game record** is one flat array of bytes, addressed by offset
  from its first byte (written `game+0x1b63`). The **state block** holds everything the simulation
  allocates (units, projectiles, orders, scripts, the map, and so on). A few simulation globals
  (the two random generator states, the order kind names, the meteor and economy
  counters, the table of moving classes, and the arrays of exploded pieces and AI records) live
  outside both; they are named where they are used.
- **Addresses.** A pointer-sized field holds a 32-bit address. A field of kind `ref` (see section 4)
  stores `address - base of the state block`, with 0 for null. A field of kind `pointer` stores the
  address itself. Both are compared as addresses: a `ref` first has the block base added. A value
  "designates element k" of an array when it equals the address of the first byte of element k. An
  engine that lays its memory out differently implements the same test on its own positions.
- **"ref at X"** means: read the u32 at X; if it is 0 the result is null, otherwise the result is the
  address `value + block base`. **"plain address at X"** means the u32 at X as it stands. Where the
  pseudo-code below says "ref at" or "plain address at", that is what is read; a bare "u32 at" or
  "u16 at" is a number.
- **Zero state.** Every byte of the game record and of every block allocated by the simulation is
  zero until written (section 11). Bytes that are never written hash as zero.
- A **record at address A** means the bytes starting at A; the offsets in the schema file are from A.
- Pseudo-code below uses these hash operations (defined in section 3): `H8(x)` hashes one byte,
  `H32(x)` hashes a u32 as four bytes, `HB(bytes)` hashes a run of bytes, `HP(value, target, is_ref)`
  hashes a pointer ID (section 5), `HT(table, address)` hashes a table of the schema at an address
  (section 4). `END` is `H32(0xffffffff)`.
- A **live unit** is a unit slot whose field `unitDefIndex` (the u16 at +0xa6 of the unit) is not 0.

## 2. trace.txt

Plain text, LF line endings. The header is seven lines, in this order:

```
bt-trace 10
commit <40 lowercase hex digits, or "unknown">
scenario <the path as given to the run>
game_seed <decimal>
crt_seed <decimal>
view_seed <decimal>
fpu_control_word 0x<5 uppercase hex digits>
```

When any quirk flag is on, one more line follows, `quirks <8 lowercase hex digits> <spec>`. With
every flag off the line is absent. When the scenario selects the demo pathfinder (`pathfinder hpa`,
`docs/scenario-format.md`), the line `pathfinder hpa` follows the `quirks` line, if any; it is absent otherwise. Then one row per line:

```
<tick> <phase> <16 lowercase hex digits>
```

`tick` is the decimal game tick, counted from 1 (the tick counter is incremented, then the phases
of that tick run). The 16 digits are the 64-bit hash as `high word` then `low word`, each 8 digits:
the hash `0x0123456789abcdef` is written `0123456789abcdef`.

**Phases.** A tick has these rows, in this order:

| Row | When it is written |
| --- | --- |
| `net` | After the network packets of the tick are handled. Only on a tick that handles packets, which means a networked game (bit 0 of `netGameFlags` set). Test-mode runs never write it. |
| `units` | After the units are updated. |
| `projectiles` | After the projectiles are updated. |
| `explosions` | After the exploded pieces and debris are updated. |
| `players` | After the players are updated. |
| `features` | After the features are updated. |
| `wind` | After the wind is updated. |
| `meteors` | After the meteors are updated. |
| `world` | Immediately after `meteors`; the hash state is reset for it. |
| `tick` | At the end of the tick, after every step that follows the last phase (`docs/schedule.md`, "After the phases"), and before the tick's save and round trip. The last tick of a run writes it too, before the run exits. |
| `view` | Immediately after `tick`, in the same state: the player view of the end of the tick (section 6, `docs/view.md`, "The recorded view"). |

A test-mode tick therefore has 10 rows. The rows are flushed at the end of each tick.

## 3. The hash

The hash is a 64-bit FNV-1a variant, over a stream of bytes the walk produces (sections 6 to 8). The
state is one 64-bit value `h`, mod 2^64.

- Offset basis: `h = 0xcbf29ce484222325`. It is set at the start of every row, and again before
  the `world` row, the `tick` row and the `view` row. (The two halves are 0xcbf29ce4 high and 0x84222325 low.)
- Prime: `P = 0x100000001b3`.
- To hash a run of `n` bytes `b[0..n)`:
  - while at least 8 bytes remain: `h = (h xor w) * P`, where `w` is the next 8 bytes read as a
    little-endian u64; then skip them;
  - for each remaining byte `x` (fewer than 8): `h = (h xor x) * P`.
- The state carries over between runs; every run is chunked on its own. Two runs `A` then `B` are not
  the same as one run `A ++ B` unless `len(A)` is a multiple of 8.
- `H8(x)` is the run of one byte `x`. `H32(x)` is the run of the four bytes of `x`, least significant
  first, so it is four byte steps (never a word step). `HB(bytes)` is one run.
- `HP(value, target, is_ref)` is `H8(tag)` then `H32(index)`, where `(tag, index)` is the pointer ID
  of section 5.
- `END` is `H32(0xffffffff)`, the end of a section.

A plain FNV-1a over bytes would differ: only the 8-byte words use the word step.

## 4. The schema file

`data/trace_schema.json` lists the tables. Top-level keys:

| Key | Meaning |
| --- | --- |
| `trace_format` | The format number, 10 (section 12). |
| `targets` | The pointer target names with their numeric `tag` (used in section 5). |
| `tables` | Table name to table, sorted by name. |
| `game_scalars` | Fields of the game record hashed in the `game` section (section 8.1), in hash order. |
| `wind` | Fields of the game record hashed in the wind section, in hash order. |
| `globals` | Named simulation globals: `meteors`, `game` and `pathfinder`, each a list in hash order. |
| `results` | The path results section (section 8.21), which is dumped and not hashed: its `section` number, the most waypoints of an event it dumps (`points`), and the `fields` of an event in dump order. |
| `packets` | The share packets section (section 8.23), which is dumped and not hashed: its `section` number and the `fields` of a packet in dump order. |
| `economy` | The economy events section (section 8.22), which is dumped and not hashed: its `section` number and its `kinds`. |
| `windMeteors` | The wind_meteors events section (section 8.24), which is dumped and not hashed: its `section` number and its `kinds`. |
| `features` | The features events section (section 8.25), which is dumped and not hashed: its `section` number and its `kinds`. |
| `targeting` | The targeting events section (section 8.26), which is dumped and not hashed: its `section` number and its `kinds`. |
| `los_radar` | The los_radar events section (section 8.27), which is dumped and not hashed: its `section` number and its `kinds`. |
| `map` | The map events section (section 8.28), which is dumped and not hashed: its `section` number and its `kinds`. |
| `playersEvents` | The players events section (section 8.29), which is dumped and not hashed: its `section` number and its `kinds`. |
| `aiEvents` | The ai events section (section 8.30), which is dumped and not hashed: its `section` number and its `kinds`. |
| `weaponsEvents` | The weapons events section (section 8.31), which is dumped and not hashed: its `section` number and its `kinds`. |
| `ordersEvents` | The orders events section (section 8.32), which is dumped and not hashed: its `section` number and its `kinds`. |
| `movementEvents` | The movement events section (section 8.33), which is dumped and not hashed: its `section` number and its `kinds`. |

The event sections (the last nine rows above) are not lists of fields but of kinds. `kinds` maps each kind value
(the first int of a record, as a decimal string) to an object with the kind's enum `name`, the `size` in bytes of its
payload (without the `outcome`) and its own `fields`: the fields the kind has, in dump order, each with `name`,
`size`, `kind` and `target` as below but with no `offset`. A field of the kind `handle` is a 4-byte block offset that the dump
writes as `null` when 0 and `heap` otherwise. A nested record is flattened into dotted names (`record.hitPoints`), or
is one `bytes` field. Every list starts with `kind` and ends with `outcome`, except those of `windMeteors`, which have
no `outcome`. A reader must read the kind before the fields.

A **table** has `size` (the byte size of the record it describes), `fields`, `plan`, and for most
tables `excluded`. A table with `split_of` is a part of another table: it holds fields of that
record (`size` is that record's size) that are hashed only under a condition. Each part is hashed on
its own, with its own plan; the sections say when.

A **field** (in `fields`, `game_scalars` and `wind`) has:

| Key | Meaning |
| --- | --- |
| `name` | The field's name. It may be empty (a gap with no name; the dump calls it `+0x<offset in lowercase hex>`) and may be a path such as `target.x` or an index such as `cursor[3]`. |
| `offset` | Byte offset from the start of the record. |
| `size` | Size in bytes. |
| `kind` | `scalar` (a number of 1, 2, 4 or 8 bytes), `bytes` (a run, or a number of another size), `pointer` or `ref` (4 bytes, section 5). |
| `target` | For `pointer` and `ref`: the target name (`heap`, `unit`, ...). Otherwise null. |
| `mask` | If present, the value is read as a little-endian number of `size` bytes, ANDed with `mask`, and hashed as a u32. |
| `mask_reason` | Why bits are dropped. For information. |

The **plan** is what the hash walks. `fields` lists every hashed field, and the dump prints them.
`plan` is the same bytes arranged for hashing: a pointer or `ref` and a masked field each stay a
plan entry of their own; every other field is a `bytes` entry, and **adjacent** `bytes` entries
(next offset equals previous offset plus size, with no excluded field between) are merged into one
entry. So hashing a table is:

```
HT(table, A):  for each entry E of table.plan, in order:
    if E.kind is pointer or ref:   HP(u32 at A+E.offset, E.target, E.kind == ref)
    elif E.mask is present:        H32( (little-endian number of E.size bytes at A+E.offset) AND E.mask )
    else:                          HB( bytes A+E.offset .. A+E.offset+E.size )
```

The merge matters: one `HB` over 12 merged bytes is one word step and four byte steps, but three
`HB` of 4 bytes each is twelve byte steps.

Lists that are not tables (`game_scalars`, `wind`) are walked field by field with no merging: each
entry is hashed as a plan entry of the same kind rule (pointer, mask, or `HB` of `size` bytes).
`globals` entries have no `offset`; they are named values of `size` bytes, hashed the same way
(`pointer` or `ref` with a `target` is `HP`, otherwise `HB` of the value's bytes; a global never has
a mask).

`excluded` lists fields of the record that are not hashed through the table, with the reason. Some
are hashed another way (a count and a run, the class of the object) described in the sections;
others are left out for good.
An `excluded` entry named `+0x<low>..+0x<high>` leaves out every field of the record that starts in that
range of offsets, the bounds included (the Unit table leaves out `+0xbc..+0xeb`, the resource account).

**Masks.** The masks the schema holds are the ones in the field lists. A masked field is read as
a number, ANDed, and hashed as 4 bytes whatever its size. Example: the `flags` of an `Order`
(offset 0x42, `mask` 0xffdfffff) drops bit 0x200000, which only drawing code sets.

## 5. Pointer IDs

A pointer is never hashed as an address. It is hashed as an ID: `H8(tag)` then `H32(index)`.

```
resolve(value, target, is_ref):
    if value == 0:                      return (0, 0)          # null
    if is_ref: value = value + block base
    tag = 1 (heap), index = 0
    switch target:
       unit:       array = units array, stride 0x118, count = the pool count
       player:     array = game+0x1b63, stride 0x14b, count 11
       unitdef:    array = the unit definitions, stride 0x249, count = the definition count
       weapondef:  array = game+0x2cf3, stride 0x115, count 256
       projectile: array = the projectiles, stride 0x6b, count = the projectile count
       feature:    array = the feature definitions (a plain address), stride 0x100, count = the feature count
       piece:      array = game+0x1ab9f, stride 0x34, count 300
       rec:        if value is the overflow record: return (9, 0xffffffff)
                   array = the spatial grid, stride 10, count = the spatial grid count
       node:       if there is no pathfinder: return (1, 0)
                   array = the pathfinder's node pool, stride 0x14, count = the pathfinder's `used`
       moveclass:  array = the moving class table, stride 0x20, count 32
       order, orderfx, link:  look value up in the ID maps of this tick (below);
                   found: return (12 | 13 | 14, id); not found: return (1, 0)
       heap, or any other:    return (1, 0)
    if array base != 0 and value >= base and value - base < stride * count
       and (value - base) mod stride == 0:
          return (tag of target, (value - base) / stride)
    return (1, 0)
```

The tags are the `tag` values of the `targets` list: heap 1, unit 2, player 3, unitdef 4,
weapondef 5, projectile 6, feature 7, piece 8, rec 9, node 10, moveclass 11, order 12, orderfx 13,
link 14. Tag 0 is null. The array bases and counts are read from the game record (appendix,
section 13), at the time of hashing.

A non-null pointer that designates no element of its target array is `heap`: tag 1, index 0.
Likewise a non-null value of target `heap`, `order`, `orderfx` or `link` that is not in the maps.

**The ID maps** (orders, order effects and links) are rebuilt once per world row, before the
sections that use them (section 8.9), and used by that row only. Pointer fields of target `order`,
`orderfx` or `link` occur only in tables that the world row hashes after the maps are built.

The numbering walk, over the live units in unit index order:

```
orderCount = 0; fxCount = 0
for each live unit u, in index order:
    for chain in (u.list, u.list2):
        o = chain head (a ref); steps = 0
        while o != 0 and steps < 4096:
            register_order(o)
            register_fx(o.attached)                # the order's `attached` ref, offset 0x52
            o = o.next                             # ref at offset 0x4a
            steps += 1
    m = u's motion (the ref at offset 0 of the unit)
    if m != 0 and m.goal != 0:                     # the ref at offset 0 of the motion record
        register_fx(goal.target)                   # the `target` ref, offset 4 of the goal

register_order(o):   id = orderCount++
                     map_order[o] = id
                     map_link[o + 0x12] = id                 # the link embedded in the order
register_fx(f):      if f == 0 or f already in map_fx: return
                     id = fxCount++
                     map_fx[f] = id
                     if class of f is PathOrder:             # section 8.11
                         map_link[f + 0x16] = id | 0x80000000    # the link embedded in a path order
```

An order met a second time (the same address in two chains) is numbered again, and the map keeps its
latest number. An order effect keeps the number it got when it was first met. Addresses here are
absolute (the `ref` plus the block base). If more than 65536 orders or effects are met the run
fails.

The dump never shows addresses, so a reader of a dump never rebuilds these maps. Every `HP(...)`
link, order and order effect pointer is already written as its ID (`order:5`, `orderfx:2`, `link:N`,
`null` or `heap`), and the dump holds the lines to hash it as it is.

In the dump, `order:5` is tag 12, index 5; a `link` index of 2147483650 is link ID `2 | 0x80000000`,
the link inside order effect 2.

## 6. What a row hashes

**Phase rows** (`net` if present, `units`, `projectiles`, `explosions`, `players`, `features`,
`wind`, `meteors`) all hash the same things, in the same order, at the moment the phase ends. The
phase name only says when the state was read.

```
h = offset basis
H32(game tick)                       # the u32 at game+0x38a47
H32(random state 1)                  # the state of the Park-Miller generator the game logic draws from
H32(random state 2)                  # the main thread's C runtime generator state ("holdrand")
section 1 units            (7.1)
section 2 projectiles      (7.2)
section 3, 4, 5 explosions (7.3; no longer hashed, format 8)
section 6 players          (7.4)
section 7 features         (7.5)
section 8 wind             (7.6)
section 9 meteors          (7.7)
```

**The world row** starts again from the offset basis and hashes sections 10 to 14, 18 to 23, 24, 25 and 27 in order,
then sections 29 to 31 when the tick is 1. It has no tick or random preamble. Sections 15, 26 and 28 are not in
the row since format 6, and sections 24 and 25 hold other content (sections 8.13 and 8.14).

**The `tick` row** (format 9) is the state at the end of the tick, the whole of what the other rows judge, read once
everything in the tick has run. It is one hash from the offset basis: exactly what a phase row hashes, then, with no
reset, exactly what the world row hashes.

```
h = offset basis
H32(game tick)
H32(random state 1)
H32(random state 2)
sections 1, 2, 6, 7, 8, 9                       (as in a phase row)
sections 10 to 14, 18 to 25, 27                 (as in the world row)
sections 29 to 31                               (tick 1 only)
```

Its hash is not the world row's hash continued, nor a hash of the other rows: the world part is hashed on the state
at the end of the tick, which the steps after the last phase may have changed (an eyeball that expires clears
visibility, a resource share moves metal or energy between players). An engine that reaches the same end-of-tick
state by another order of work inside the tick has the same `tick` row and may have other phase rows.

**The `view` row** (format 10) is the player view of the end of the tick: the block of `interfaces/view.h` that the run fills for the
viewer and the controller, read right after the `tick` row from the same state. It is one hash from the offset basis, with the word
step of section 3, over one run: the serialisation S of the filled block, defined in `docs/view.md`, "The recorded view" (the header of the
block, the ten players, the used rows of each table in declaration order and the sight grid; no section number, no `END`, no
preamble). It judges what a presentation is given, and is not part of the `tick` row: a core with the same state and another view
has the same `tick` row and another `view` row.

The rows hash state, and events are only dumped. Sections 16 and 17 (the pathfinder's own report of its
state, section 8.7), economy's own report (record 34, section 8.22), the path results (section 32,
section 8.21), the economy events (section 33, section 8.22), the share packets (section 35,
section 8.23), wind_meteors' events (section 37, section 8.24), the features events (section 36, section 8.25), targeting's events (section 38, section 8.26) los_radar's events (section 39, section 8.27) map's events (section 40, section 8.28) players' events (section 41, section 8.29) ai's events (section 42, section 8.30) weapons' events (section 47, section 8.31), orders' events (section 48, section 8.32) movement's events (section 49, section 8.33), units' events (section 50, section 8.34) and scripts' events (section 51, section 8.35) are in a dump and in no row, as diagnostics, and so are the records 43 to 46, 48 and 52 to 55 (ai's, los_radar's, map's, players', weapons', orders', movement's, units' and scripts' reports, sections 8.30, 8.6, 8.28, 8.17, 8.31, 8.36, 8.37, 8.38 and 8.39). Pathfinding and economy are judged by the state they leave in the sections that are
hashed: what the orders and goals keep of a path (sections 20 and 21), and the players' and units'
resource fields.

Each section begins with `H8(section number)` and ends with `END`, except where noted (the
`explosions` group is three sections, 3 to 5, each with its own `END`, until format 7; the pathfinder's dump is two
sections, 16 and 17).

## 7. Sections of the phase rows

Each section lists what it hashes and what it dumps. "Dump" gives the path of each line in
`dump.txt` (section 10); the values are as in section 10.

Tables are written `HT(Name, A)`. `A` is the address of the record. Counts and addresses come from
the game record (appendix, section 13) at the time of hashing.

### 7.1 units (section 1)

```
H8(1)
for i in 0 .. poolCount-1:                 # poolCount: the u16 at game+0x14351; no units array: no loop
    u = unitsBase + i * 0x118
    if unitDefIndex of u is 0: continue    # not live
    H32(i)
    HT(Unit, u)
END
```
Dump: `units[i].<field>`.

Since format 8 the table `Unit` leaves out what only one module reads, and the module reports it in a record that no
row hashes:
- weapons': in each of the three weapon slots (the slot k starts at byte 4 + 0x1c*k of the unit), the aim callback
  (8 bytes), `muzzleAimFromDeltaZ`, `reloadTimer`, `fireHeading` and `firePitch`, as the byte ranges `+0x8..+0xf`,
  `+0x14..+0x1d`, `+0x24..+0x2b`, `+0x30..+0x39`, `+0x40..+0x47` and `+0x4c..+0x55`. The slot's target pair,
  `weapon`, `stockpile` and `flags` stay hashed. Record 48 (section 8.31) reports them.
- units': `head`, `hoverBobPhase`, `healthPercent`, `prevHealthPercent`, `unfinishedInitZero`, the 4 bytes at `+0x10a`
  and `zBufferFlag`. Record 54 (section 8.38) reports them. Nothing reads `healthPercent`, `prevHealthPercent`,
  `unknown_10a` and `unfinishedInitZero`, and only drawing reads `zBufferFlag`.

The excluded fields have no `units[i].<field>` line; the dump holds them under the record's prefix.

`flags` (+0x110) is hashed with the mask 0xffffff2f: bit 0x10, the selection, and bits 6 and 7, its visit mark
(`selectState`), are view state, so two runs that differ only in what is selected
or which units focusing has visited have equal rows. The dump prints the masked value.

### 7.2 projectiles (section 2)

```
H8(2)
for i in 0 .. projectileCount-1:           # the u32 at game+0x141f3
    p = projectilesBase + i * 0x6b
    if (u16 at p+0x69) AND 2: continue     # a free slot
    H32(i);  HT(Projectile, p)
END
```
Dump: `projectiles[i].<field>`.

Since format 8 the table `Projectile` hashes only `type`, `position`, `target`, `attacker` and `owner`. These left, and
record 48 (section 8.31) reports them: `start`, `direction` and `angles` (each its three components), `speed`,
`range`, `spawnTick`, `time`, `nextSmokeTick`, the handle at `+0x4e`, the one at `+0x56`, `cellX`, `cellZ`, `radius`,
`active`, `piece`, `propellerSpin`, `compactIndex` and `flags`. The skip test (a free slot) is unchanged and still
reads the u16 at `p+0x69`. The excluded fields have no `projectiles[i].<field>` line.

### 7.3 explosions (sections 3, 4 and 5, until format 7)

Sections 3, 4 and 5 are not in the phase rows since format 8: no `H8(3)` to `H8(5)` and no `END` of them is hashed,
and the harness visits nothing there. The phase list is unchanged: the `explosions` row still exists and hashes the same
things as the other phase rows (the tick, the random states, and sections 1, 2 and 6 to 9). Only weapons reads the exploded
pieces, the debris and the pieces of debris, and drawing shows them, so weapons reports them in record 48 (section
8.31) and the frame dumps judge a wrong value; nothing is hashed in its place. The walk below is the one of format 7 (record 48 writes the same entries in the same order, but hashes none of them):

```
H8(3)                                      # exploded pieces
for slot in 0 .. 99:
    b = exploded piece block of the slot   # 100 refs held in a simulation global; null: skip
    H32(slot);  HT(ExplodedPiece, b);  HT(PieceRec, b + 0x30)
END

H8(4)                                      # debris
n = min(the u32 at game+0x1491b, 300)
for d in 0 .. n-1:
    e = game+0x1491f + d*0x54
    if the u32 at e is 0: continue
    H32(d);  HT(Debris, e)
    # remember the piece the debris points at (see below)
END

H8(5)                                      # pieces used by debris
for i in 0 .. 299 such that piece i was remembered:
    H32(i)
    HT(Piece, game+0x1ab9f + i*0x34)
    HB(96 bytes at game+0x1e88f + i*96)                       # vertices
    for k in 0 .. 5: HT(Face, game+0x2590f + i*0xc0 + k*0x20)    # faces
END
```
The piece a debris record "points at" is the u32 at its offset 0: piece `i` is remembered if that
value is the address `game+0x1ab9f + i*0x34` for some i below 300 (the address of the first byte of
the piece).
Dump (format 7): `explodedPieces[slot].<field>` and `explodedPieces[slot].rec.<field>`; `debris[d].<field>`;
`pieces[i].<field>`, `pieces[i].vertices` (a `hex:` line of 96 bytes) and `pieces[i].faces[k].<field>`. Format 8
writes them as `weaponsState.explodedPieces[slot].<field>` (and `.rec.<field>`), `weaponsState.debris[d].<field>`,
`weaponsState.pieces[i].<field>`, `.vertices` and `.faces[k].<field>`, never hashed.

### 7.4 players (section 6)

```
H8(6)
for i in 0 .. 10:   H32(i);  HT(Player, game+0x1b63 + i*0x14b)
END
```
Dump: `players[i].<field>`.

### 7.5 features (section 7)

The feature spots are in a pool of 0x800 entries of 0x30 bytes (the pool is a ref at
`game+0x1420b`; no pool: the section is just `H8(7)` and `END`). Two singly linked lists thread it:
the used list, whose head is the i32 at `game+0x14213`, and the rest list, head at `game+0x14217`.
Each entry's first two bytes (an i16) are the index of the next entry.

```
list[0..0x7ff] = 0
for which in (1, 2):                         # 1: used list, 2: rest list
    idx = head of that list (an i32);  guard = 0
    while 0 <= idx < 0x800 and guard < 0x800:
        list[idx] = which                    # a later list overwrites an earlier mark
        idx = i16 at entry idx (sign-extended);  guard += 1
H8(7)
for i in 0 .. 0x7ff with list[i] != 0, ascending:
    spot = pool + i*0x30
    feature = u16 at spot+0x2c
    table = SpotAnim if feature < featureCount and the feature definitions are not null and
            (u16 at definitions + feature*0x100 + 0xfe) AND 1; else SpotObject
    H32(i);  H8(list[i]);  HT(table, spot)
END
```
Dump: `features[i].list` (value `0x01` or `0x02`) and `features[i].<field>`.

The two tables are told apart by the field names of the lines. The first line after
`features[i].next` is `features[i].anim.index` for a SpotAnim and `features[i].state` for a
SpotObject.

### 7.6 wind (section 8)

```
H8(8)
for each field F of the schema's `wind` list, in order: hash F at address game+0 (pointer, mask or HB rule of section 4)
END
```
Dump: `wind.<name>`.

### 7.7 meteors (section 9)

```
H8(9)
for each entry G of globals.meteors, in order: hash the global G (HB of its `size` bytes; the global `weapon` is a `ref` and is HP)
END
```
Dump: `meteors.<name>`.

## 8. Sections of the world row

### 8.1 game scalars (section 10)

```
H8(10)
for each field F of game_scalars, in order:   hash F at game+F.offset
for each entry G of globals.game, in order:   hash G
END
```
Dump: `game.<name>`. Masked fields are dumped as masked. `endGameFlags` is masked with 0xfffc: bits 0 and 1, the debug
display and debug mode, are view state.

### 8.2 map cells (section 11)

Cells are 13 bytes. With `width` the u32 at `game+0x14233` and `height` the u32 at `game+0x14237`,
`n = width * height` (mod 2^32), and `n = 0` when it is above 2^22.

```
H8(11)
H32(n)                                       # dump: cells.count
if the cell array (ref at game+0x14287) is not null:
    copy = the n*13 bytes of the cell array
    for each cell c of the copy:
        feature = u16 at c+8
        c[12] = c[12] AND NOT (0x04 OR 0x80)                       # flags: drop the two bits that only drawing uses
        if feature >= 0xfffb and feature != 0xfffe:  c[10] = c[11] = 0
    HB(all n*13 bytes of the copy, as one run)
END
```
Dump: `cells.count`, then `cells[i]` per cell with the bytes of the modified copy. A `cells[i]` line
is `hex:` and the 13 bytes of that cell, lowest address first. When the cell array is null there are
no `cells[i]` lines, only `cells.count`.

### 8.3 visibility (section 12)

```
H8(12)
m = n / 4 (integer division, n as in 8.2)
H32(m)                                       # dump: visibility.count
if the visibility mask (ref at game+0x14273) is not null:  HB(m*2 bytes of it, one run)
END
```
Dump: `visibility.count`, then `visibility[k]` per 2 bytes.

### 8.4 explored grids (section 13)

```
H8(13)
for who in 0 .. 9:
    P = game+0x1b63 + who*0x14b
    cells = the ref at P+0x7c;  size = the u32 at P+0x88
    if cells is null or size > 2^22: size = 0
    H32(size)                                # dump: players[who].explored
    if size != 0: HB(size bytes of cells, one run)       # dump: players[who].explored[k], one byte each
END
```

### 8.5 spatial grid (section 14)

```
H8(14)
H32(u32 at game+0x142a3)                     # dump: spatial.width
H32(u32 at game+0x142a7)                     # dump: spatial.height
recs = ref at game+0x1429f;  count = u32 at game+0x142ab
if recs is null or count > 2^20: count = 0
H32(count)                                   # dump: spatial.count
for r in 0 .. count-1: HT(Rec, recs + r*10)  # dump: spatial[r].b
overflow = ref at game+0x142b7
H8(overflow != null)
if overflow != null: HT(Rec, overflow)       # dump: spatial.overflow.b
END
```
Since format 6 the table `Rec` holds only `b` (byte 1 of a record): `a`, `flags` and `first` are read only by map, and
map's report (record 45, section 8.28) dumps them. Orders reads `b` through the spatial bucket of a unit.

### 8.6 eyes and the sight cache (section 15 until format 5, now record 44)

Section 15 is not in the world row since format 6: only los_radar reads the eyeballs, and its effect is in the visibility
mask (section 8.3). los_radar reports them, with the unit sight cache that left the `Unit` table, in record 44, through
the `StateWriter` of the seam's `WriteState`, in the same call as section 24 (section 8.13), before it. The record is dumped
and never hashed, with the prefix `losRadarState.`:

```
losRadarState.unitSlots                       # n, the unit slots: the pool count at los_radar's Init
for s in 0 .. n-1:
    losRadarState.sight[s].cell               # 4 bytes: the cell of the last sight (was Unit+0x7a, x low short, z high short)
    losRadarState.sight[s].frameIdx           # 1 byte (was Unit+0xf8)
losRadarState.eyes.count                      # game+0x14277, 0 when the table is null or the count is above 20
for e in 0 .. count-1, the fields of the table Eye in order, without `screen`:
    losRadarState.eyes[e].<field>             # player (1 byte, the index), x, flagA, flagB, pos.x, pos.y, pos.z, expires, screenPos
```
The bytes 0x7a to 0x7d and 0xf8 of a `Unit` stay in the record, unused (`unusedLosCache`, `unusedLosSightFrame`), and the table
`Unit` no longer hashes them.

### 8.7 pathfinder (sections 16 and 17, dumped only)

These two sections are not part of the world row: no row hashes them. A dump still holds their
lines, at their place between sections 15 and 18, as a diagnostic: when a row differs, they show the
module's state at that tick, as the path results (section 32) show what it delivered. The walk below gives the order and the values of
those lines; the hash it describes is computed and dropped.

`pf` is the ref at `game+0x14207`: the start of the pathfinder's region in the state block. The three arrays of section 17 (`globals.pathfinder`) are in the same region; the default pathfinder writes them through the `WriteState` slot of the pathfinding seam (`interfaces/state_writer.h`), in the order below, and the lines dumped are the same. An implementation that is not the default writes its own records in section 16 (the demo's `hpa`), and nothing in section 17.

```
H8(16)
H8(pf != null)
if pf != null:
    HT(Pathfinder, pf)                       # dump: pathfinder.<field>
    if the u32 at pf+0x58 (the `object` field) != 0:  HT(PathfinderSearch, pf)
    nodes(pf)
    cells(pf)
END
H8(17)
for each entry G of globals.pathfinder, in order: hash G        # dump: pathfinder.<name>
END
```

`nodes(pf)`:

```
pool = ref at pf+0;  items = ref at pf+4;  freeHead = i32 at pf+8
used = u32 at pf+0xc;  count = u32 at pf+0x14
if pool is null or items is null or used > 2^19 or count > used:  used = 0; count = 0
free[0..used) = false
steps = 0
while steps < used and 0 <= freeHead < used:        # the free list: node k's first four bytes are the next free index
    if free[freeHead]: break
    free[freeHead] = true;  freeHead = i32 at pool + freeHead*0x14;  steps += 1
for node in 0 .. used-1, not free:
    n = pool + node*0x14
    H32(node);  HT(Node, n)                          # dump: pathfinder.pool[node].<field>
    if the i32 at n+8 (the field `g`) != 0:
        HB(2 bytes: the u16 at n+0x10)               # dump: pathfinder.pool[node].penalty, 0x%04x
H32(count)                                           # dump: pathfinder.heap.count
for slot in 0 .. count-1:
    HP(u32 at items + slot*4, node, is_ref=false)     # dump: pathfinder.heap[slot]
```
The `items` array holds plain node addresses. The `node` target is resolved against the pool of
this pathfinder, using the unclamped `used` count read from the record.

`cells(pf)`:

```
cells = the u32 (plain address) at pf+0x1c;  dirty = u32 (plain address) at pf+0x2c;  count = u32 at pf+0x28
if cells is null or dirty is null or count > 2^22:  count = 0
words = (count + 255) / 256
for w in 0 .. words-1:
    bits = u32 at dirty + 4*w
    if bits == 0: continue
    H32(bits)                                        # dump: pathfinder.dirty[w], 0x%08x
    for bit in 0 .. 31, where bit `bit` of `bits` is set:
        for k in 0 .. 7:
            index = w*256 + bit*8 + k
            if index >= count: break
            c = cells + index*4
            flags = c[0];  state = flags AND 3
            HB(flags as 1 byte)                      # dump: pathfinder.cells[index].flags
            if (flags AND 8) or state == 1 or state == 2:  HB(c[1])          # .dir
            if state == 1 or state == 2:                    HB(u16 at c+2)   # .node
```
Dump values of these are written as `0x` plus `2*size` hex digits.

A `.dir` or `.node` line is present only when its condition above holds, and a line that is present
is hashed. A cell with neither condition has only its `.flags` line.

### 8.8 moving classes and pass maps (sections 18 and 19)

```
H8(18)
for c in 0 .. 31:  HT(MoveClass, moveClassTable + c*0x20)       # dump: moveClasses[c].<field>
END

H8(19)
for c in 0 .. 31:
    e = moveClassTable + c*0x20
    cells = ref at e+0x18;  width = u32 at e+0x10;  height = u32 at e+0x14
    full = height / 16;  tail = height AND 15
    skip c if cells is null, or width == 0, or height == 0, or width > 4096,
              or width * ((height + 15) / 16) > 2^22
    H32(c)
    HB(full*width*4 bytes of cells, one run)         # dump: moveClasses[c].passMap[k], k from 0, 4 bytes each
    if tail != 0:
        mask = 2^(2*tail) - 1
        row[x] = (u32 at cells + (full*width + x)*4) AND mask,  for x in 0 .. width-1
        HB(row, width*4 bytes, one run)              # dump: moveClasses[c].passMap[k], k from full*width
END
```
The first `HB` is one run; so is the second (the tail row is a separate run).

A `moveClasses[c].passMap[k]` line is `hex:` and the 4 bytes of word `k`, lowest address first (the
words in memory order, not written as a number). The mask is already applied in the dump: the lines
from `full*width` on hold the masked tail row, and the lines before them are the cells as they are.

### 8.9 orders (section 20)

First the maps of section 5 are built (the numbering walk). Then:

```
H8(20)
for each live unit u, in index order, i its index:
    H32(i)
    for which in (0, 1):                    # 0: u.list (offset 0x5c), 1: u.list2 (offset 0x60)
        o = chain head as an address (the ref plus the block base)
        H8(which + 1)
        n = 0
        while o != 0 and n < 4096:
            HT(Order, o)                    # dump: units[i].list[n].<field>, or list2[n]; the table without the fields below
            if not posYUnset(o): HT(OrderPosY, o)
            o = ref at o+0x4a;  n += 1
        H8(0)
END
```
Since format 8 the table `Order` hashes `unit`, the link (`linkOwner`, `linkNext`, `linkValue`), `target`,
`unknown_1a`, `pos.x`, `pos.z` and `flags_42` (the byte at +0x42, masked). Only orders reads the rest: `kind`, `state`,
`flags6` (the word at +6), `wakeFrame`, `start`, `arg0`, `arg1`, `arg2`, `created`, `subFlags` and `attached`, and
record 52 (section 8.36) reports them. The walk, `H32(i)`, the two chains and their markers are unchanged, and the
unit's `head` is no longer hashed here (it is units', record 54, section 8.38).

The order's kind is the u8 at offset 4, and its **kind name** is the name the game's order kind table
gives that number. `posYUnset(o)` is true when the kind name is `MobileBuild`, `VTOL_MobileBuild`,
`Reclaim` or `VTOL_Reclaim`: those orders never write their `pos.y` (the field of the table
`OrderPosY`), so it is not hashed.

### 8.10 motions and goals (section 21)

```
H8(21)
for each live unit u, in index order:
    m = ref at u+0 (the motion record)
    H32(i);  H8(m != null)
    if m == null: continue
    HT(UnitMotion, m)                       # dump: units[i].motion.<field>
END
```

Since format 8 the table `UnitMotion` leaves out `p2` (x, y, z) and `turn`, which only movement reads, and hashes
`lastMoveTick` (the 4 bytes at +0x2a) again: units reads it for the hover bob's damping. **No goal is hashed**: the
section has no goal walk, no class and no goal fields. Movement reports the motion's `p2` and `turn` and the goal in
record 53 (section 8.37). `goal(g)` below describes what that record writes: the prefix is
`movementState.units[i].goal`, each `H32` is a 4-byte value line, each `HB` a `hex:` line, and nothing is hashed.

`goal(g)`, with the goal's class by the kind of object it is (none 0, patrol 1, AI search 2, packed 3,
lite 4, packed position 5, any other kind 6, never seen in a conforming run):

```
if g == null:  H32(0)                       # dump: <prefix>.class
else:
    H32(class)                              # dump: <prefix>.class; 6 for a goal of none of these kinds, with no fields
    patrol (1):     HT(PatrolGoal, g);   n = min(u32 at g+0x18, 3);   HB(n*4 bytes at g+0xc)     # dump: <prefix>.points
    AI search (2):  HT(AiSearchGoal, g); n = min(u32 at g+0x5c, 20);  HB(n*4 bytes at g+0xc)
    packed (3), lite (4), packed position (5):
                    HT(PackedGoal, g);   if class == 4: HT(LiteGoalState, g)
```
Dump: `<prefix>.<field>` for the tables and `<prefix>.points` as one `hex:` line.

### 8.11 order effects (section 22)

```
H8(22)
for id in 0 .. fxCount-1:                   # the order effects in numbering order (section 5)
    f = the effect with number id
    path order (4):  HT(PathOrderRef, f)                 # dump: orderFx[id].refOwner, .refNext, .refValue
    unknown (0):     H32(id);  H32(0);  H32(a number identifying the object's type)   # never seen in a conforming run
    approach radius (1), ring approach (2), point marker (3), air maneuver (5): nothing is hashed
    any other class: the dump fails (section 11)
END
```
The harness also dumps every effect, whatever its class, with these lines (none hashed, but the ones of `PathOrderRef`
above):

```
orderFx[id].class                           # the class of f
if the class is unknown (0): orderFx[id].vtableDelta
approach radius (1):  HT(ApproachRadius, f)
ring approach (2):    HT(RingApproach, f)
point marker (3):     HT(PointMarker, f)
path order (4):       flags = u16 at f+8
                      HT(PathOrder, f)
                      if fxPosYSet(f):            HT(PathOrderPosY, f)
                      if flags AND 0x40:          HT(PathOrderHeading, f)
                      if flags AND 1:             HT(PathOrderPiece, f)
                      if (flags AND 0x82) == 2:   HT(PathOrderFollow, f)
air maneuver (5):     flags = u16 at f+8
                      HT(AirManeuverOrder, f)
                      if flags AND 1:             HT(AirManeuverOrderHeading, f)
```
`fxPosYSet(f)`: the source order is the ref at `f+4`; the `PathOrderPosY` table is dumped unless that
order exists and its kind name is `VTOL_MobileBuild` or `VTOL_Reclaim`.
Dump: `orderFx[id].<field>`.

Since format 8 section 22 hashes only, for a path order (a move target), the table `PathOrderRef` (a
split of `PathOrder`: `refOwner`, `refNext`, `refValue`, units' link, which units reads). Not its number: the numbers
count the targets of every class, and the other classes are movement's own. Nothing is hashed for classes 1, 2, 3 and 5, not
even their `H32(id)` or class. Class 0 (an effect of none of the five types) still hashes `H32(id)`, its class and the type
number. A class above 5 is none of the format's: the reference never writes one, and a dump that holds one fails, so it has no
hash (section 11). The fields of the targets are only
movement's, and a wrong value of one shows through the unit's motion.

### 8.12 scripts (section 23)

```
H8(23)
for each live unit u, in index order:
    s = ref at u+0x9a (the script state)
    H32(i);  H8(s != null)
    if s != null: script(i, s)
END
```

```
script(i, s):
    table = u32 at s+8 (a plain address);  pieces = ref at s+0x14
    object_state = ref at s+0x540
    pieceCount = 0
    if table != 0: pieceCount = u32 at table+8
    if pieceCount > 512 or pieces == 0 or object_state == 0: pieceCount = 0
    for q in 0 .. pieceCount-1: HT(PieceTransform, object_state + 0x22 + q*0x36)   # dump: units[i].script.transforms[q].<field>
```

The presence byte is 1 for every live unit whose script state is not null, whatever its piece count: a script with no
pieces (no table, a piece count above 512, a null piece list or a null object state) hashes `H32(i)`, `H8(1)` and no
transform. The dump has no line for `i` or the presence byte. A reader of a dump takes the presence from record 55: the
unit has a script state exactly when record 55 has its `scriptsState.units[i]` lines (section 8.39), which scripts writes
for every script state, pieces or none, so a unit with a `scriptsState.units[i].scale` line and no `transforms` lines
hashes `H8(1)` and nothing more.

Since format 8 section 23 hashes only the piece transforms, which weapons reads directly. The script's own words, the 8
channels, the statics and the piece records are only scripts', and record 55 (section 8.39) reports them. The
description below is the layout of what record 55 writes, with the prefix `scriptsState.units[i]` (so the channel prefix is
`scriptsState.units[i].channels[c]`), and the lines are not hashed:

```
script state:
    table = u32 at s+8;  statics = ref at s+0x10;  pieces = ref at s+0x14
    staticCount = pieceCount = 0
    if table != 0: staticCount = u32 at table+0x10;  pieceCount = u32 at table+8
    if staticCount > 4096 or statics == 0: staticCount = 0
    if pieceCount > 512: pieceCount = 0
    for c in 0 .. 7:  channel(s + 0x1c + c*0xa4)        # prefix channels[c]
    count and bytes of the statics
    if pieces == 0: pieceCount = 0
    for p in 0 .. pieceCount-1: the table ScriptPiece at pieces + p*0x4c

channel(ch):
    state = u32 at ch+0
    if state == 0:  H32(0)                              # dump: <prefix>.state
        return
    HT(ScriptChannel, ch)                               # dump: <prefix>.<field>
    sp = i32 at ch+8
    live = (sp < -1) ? 0 : sp + 1;  live = min(live, 32)
    HB(live*4 bytes at ch+0x24)                         # dump: <prefix>.stack (hex:)
    if (state AND 0xff000000) == 0x02000000:
        switch (state AND 0x00f00000):
            0x00100000, 0x00200000:  HT(ScriptChannelPiece, ch)
            0x00400000:              HT(ScriptChannelDelay, ch)
            0x00800000:              HT(ScriptChannelWait, ch)
```
A channel with `state == 0` has exactly one line, `<prefix>.state` with the value `0x00000000`, and
no `.stack` line. A channel with a state other than 0 has the lines of its table (the first is
`<prefix>.state`, with a non-zero value), then `<prefix>.stack`, then any lines of the conditional
table. So a `.state` line of value 0 means a one-line channel.

The `statics` path has two lines in a row with the same path: the count line (`0x` and 8 digits),
then the `hex:` line of the bytes (`hex:` alone when the count is 0).

The stack holds the words up to and including `sp`: the channel's stack pointer `sp` is the index of
the top word, so `sp + 1` words are live.

### 8.13 los_radar's unit index (section 24)

Since format 6 los_radar writes this section itself, through the `StateWriter` of its `WriteState` seam slot, and the
harness hashes the records of sections 24 and 25 into the row (a record of any other section is only dumped). The harness calls
los_radar's `WriteState` and then units' after section 23 and before section 27 of the world row (the order of section 6), in every phase.

```
for i in 0 .. 9 whose index entry is live:       # the player has an AI record: the simulation global of 10 refs is not null
    H8(24)                                       # Begin(24, "losRadarIndex[i]")
    H32(i)                                       # dump: losRadarIndex[i].player
    H32(hasSpecial)                              # dump: losRadarIndex[i].hasSpecial
    unitVector("losRadarIndex[i].visible")       # the enemy units the player sees
    unitVector("losRadarIndex[i].known")         # the enemy units the player knows
    END
```
A **vector** has a begin address and an end address, and `unitVector` is as in the earlier formats:

```
unitVector(path):   n = the element count
                    H32(n)                       # dump: <path>.size
                    for k in 0 .. n-1: HP(the unit ref k, unit, is_ref=false)     # dump: <path>[k]
```
A player without a live entry has no record, no hash input and no line. The values are the ones the earlier `players[i].ai.visible`,
`players[i].ai.known` and `players[i].ai.hasSpecial` held. Nothing of the `PlayerAI` record is hashed now: the table PlayerAI and its
raw vectors (`cells`, `vec_65`, `counts`, `vec_8d`, `weights`, `vec_ad`, `vec_bd`, `values`, `locked`) are only dumped (record 43, section 8.30).

### 8.14 units' factory index (section 25)

Units writes this section, as los_radar writes section 24, and the harness hashes it in the same way, after it:

```
for i in 0 .. 9 whose index entry is live:       # the player has an AI record
    H8(25)                                       # Begin(25, "unitsFactories[i]")
    H32(i)                                       # dump: unitsFactories[i].player
    unitVector("unitsFactories[i].factories")    # the player's factories
    END
```
The squad managers and their timers that section 25 hashed until format 5 are only dumped now (`aiState.players[i].squadManager.*`, section 8.30).
The timer classes are: unknown 0, squad 1, assault 2, escort 3, squad scout 4, spatial 5, scout 6,
build 7. The class follows from the slot `t` (t=0:0 1:7 2:2 3:3 4:6 5:1 6:2 7:3 8:4 9:5).

### 8.15 squads (section 26, until format 5)

Section 26 is not in the world row since format 6. The squads are only dumped (`aiState.players[i].squads[s].*`, section 8.30).

### 8.16 unit resources (section 27)

```
H8(27)
for i in 0 .. 10:
    econ = ref at (player i) + 0xec
    H32(i);  H8(econ != null)
    if econ != null: HT(UnitResources, econ)            # dump: players[i].econ.<field>
END
```

### 8.17 mission conditions (section 28 until format 5, now record 46)

Section 28 is not in the world row since format 6. Players reports the conditions and the clocks it owns in record 46,
through the `StateWriter` of its `WriteState`, dumped and never hashed, with the prefix `playersState.`, after
`mapState.*`:

```
playersState.<end-game countdown>              # 2 bytes (game+0x39239, set by the engine at set-up, unused); the leaf is the countdown's name in the game record
for i in 0 .. 9:
    playersState.players[i].<update time>      # 4 bytes (Player's unused words); the leaf is the update clock's name in the player record
    playersState.players[i].<win/lose time>    # 4 bytes; the leaf is the win/lose clock's name in the player record
playersState.conditions.present                # 1 when the list ref at game+0x391ed is not null
if present:
    playersState.conditions.victoryCount       # the u32 at 0x40 of the list
    playersState.conditions.defeatCount        # the u32 at 0x84
    playersState.conditions.active
    for list in victory, defeat; k in 0 .. count-1 (a count above 16 counts as 0) whose entry is not null:
        playersState.conditions.victory[k].class (or defeat)     # the class below
        playersState.conditions.victory[k].satisfied
        playersState.conditions.victory[k].celebrated
        playersState.conditions.victory[k].numUnits              # class 16 only
        playersState.conditions.victory[k].numLeftToKill         # classes 12 and 4 only
```
Condition classes: 0 unknown, defeat: passes z 1, passes x 2, death timer 3,
type killed 4, all of type 5, all killed 6, commander 7; victory: timer 8, passes z 9, passes x 10,
move to radius 11, kill type 12, kill all of type 13, capture 14, build 15, kill mobile 16, destroy
all 17, kill commander 18. A null entry has no lines. An entry of class 0 fails the run (section 11).
The numbers are written as `0x` and twice the byte count of hex digits.

### 8.18 unit definitions (section 29, tick 1 only)

```
H8(29)
defs = ref at game+0x1439b;  count = u32 at game+0x1438f
if defs is null or count > 2^14: count = 0
H32(count)                                              # dump: defs.count
for i in 0 .. count-1:
    d = defs + i*0x249
    yardmap = ref at d+0x14e;  ids = ref at d+0x156;  id_count = u32 at d+0x152
    yard_size = (u32)(i16 at d+0x14a) * (u32)(i16 at d+0x14c)     # mod 2^32
    HT(UnitDefData, d)                                  # dump: defs[i].<field>
    if yardmap == null or yard_size > 65536: yard_size = 0
    H32(yard_size);  HB(yard_size bytes at yardmap)       # dump: defs[i].yardmap (count), defs[i].yardmap (hex:)
    if ids == null or id_count > 30: id_count = 0
    H32(id_count);   HB(id_count*2 bytes at ids)          # dump: defs[i].ids (count), defs[i].ids (hex:)
    for m in 0 .. 3:
        mask = u32 (plain address) at d+0x231+m*4 for m < 3, at d+0x23d for m == 3
        H8(mask != 0)
            if mask != 0: HB(0x40 bytes at mask)            # dump: defs[i].categories[m] (hex:)
END
```
The dumped `defs.count` is the count after the clamp (0 when the definitions are null or the raw
count is above 2^14), written as 8 hex digits; the definitions that follow are `defs[0]` to
`defs[count-1]`, each with the lines of its `UnitDefData` fields.

`yardmap` and `ids` each have two lines at the same path, in this order: a count line, the
`H32` value written as 8 hex digits (`yard_size`, `id_count`, after the clamps above), then a `hex:`
line with the bytes that follow it in the hash (an empty `hex:` when the count is 0). The count
line is always present, so a reader reads the count from it and need not recompute it from the
definition's fields. The bytes are hashed as read, not written as hex numbers: `ids` is `id_count`
little-endian 16-bit values in memory order.

`defs[i].categories[m]` has a line (`hex:` and the 0x40 bytes) only when the mask address is not
zero. When the line is absent the presence byte is 0 and nothing else is hashed; the mask `m` is
present exactly when its line is. Mask 3 is the one at 0x23d; masks 0 to 2 are the ones at 0x231,
0x235 and 0x239.

`UnitDefData` has pointer fields (`movementclass`, `weapons[0]` to `weapons[2]`, `explodeas`,
`selfdestructas`). They are `ref` fields of the schema, so they are hashed and dumped as IDs by
section 5: `null` for 0, `weapondef:<n>` or `moveclass:<n>` for a ref that falls on an element of
that table, `heap` for any other non-zero ref. Nothing in sections 29 to 31 is hashed as an
address.

### 8.19 weapon definitions (section 30, tick 1 only)

```
H8(30)
for i in 0 .. 255:
    w = game+0x2cf3 + i*0x115
    HT(WeaponDefData, w)                                # dump: weapons[i].<field>
    table = u32 at w+0x64 (plain address)
    H8(table != 0)
    if table != 0:
        first = u32 at table+5;  last = u32 at table+9           # unaligned
        n = (first != 0 and last >= first and (last-first) mod 8 == 0) ? (last-first)/8 : 0
        if n > 2^20: n = 0
        H32(n)                                          # dump: weapons[i].damage.entries (always present)
        for k in 0 .. n-1:
            e = first + k*8
            string(u32 at e)                            # dump: weapons[i].damage[k].name
            H32(u32 at e+4)                             # dump: weapons[i].damage[k].value
END
```
`WeaponDefData` has no pointer fields: the table address at 0x64 is not one of its schema fields (the
field named `damage`, a 16-bit number, is an ordinary scalar with the path `weapons[i].damage`, and
is not the table). The table's lines are `weapons[i].damage.entries`, then for each entry `k` the
lines `weapons[i].damage[k].name` and `weapons[i].damage[k].value`. All of them are present exactly
when the table address is not zero; the `entries` line is then present even when `n` is 0 (it
holds the `n` after the clamps above, as 8 hex digits). The `name` line is `null` (when the entry's
string address is 0) or a `str:` value, and the `value` line is the entry's second word as 8 hex
digits (the `H32` value).

`string(p)`: `H8(p != null)`; if not null, `len` is the number of bytes before the first 0 byte, at most
255; `H32(len)`; `HB(len bytes at p)`. Dump: `null` or `str:` followed by the `len` bytes.
The bytes are written as they are: no quoting and no escaping, any byte value other than 0 as that
byte (bytes of 0x80 and above are not translated), and the line ends at the next newline. The
length is not written; it is the number of bytes after `str:` up to the end of the line (an empty
string is `str:` alone and is not `null`). A string of 255 or more bytes is cut at 255. A string
cannot hold a 0 byte, since that ends it. A string that holds a newline byte (0x0a) cannot be read
back from the dump, because its line ends there; take such a block from the trace instead.
Read the dump
as bytes (for instance as Latin-1) and split lines at 0x0a only, so that a carriage return or a
byte of 0x80 or above stays in the value.

### 8.20 mission (section 31, tick 1 only)

The mission record is present when `game+0x391e9` is not zero. When it is, the lines
`mission.<field>` of `MissionData` are in the dump and the presence byte is 1; when it is zero
there are no `mission.*` lines, the presence byte is 0 and the section is only `H8(31)`, `H8(0)`,
`END`. A scenario that is not a mission (a skirmish) can still have a record, so decide from the
lines, not from the scenario.

```
H8(31)
m = ref at game+0x391e9
H8(m != null)
if m != null:
    HT(MissionData, m)                                  # dump: mission.<field>
    if the i32 at m+0 (the mission type) == 1: HT(MissionStartRest, m)   # a campaign mission only
    units = ref at m+0xdac, unitCount = u32 at m+0xdb0
    rules = ref at m+0xdb4, ruleCount = u32 at m+0xdb8
    features = ref at m+0xdbc, featureCount = u32 at m+0xdc0
    each count is 0 when its array is null or the count exceeds 65536
    H32(unitCount)                                      # dump: mission.units.count
    for ku in 0 .. unitCount-1:
        u = units + ku*0x24
        string(ref at u+0)   # name         dump: mission.units[ku].name
        string(ref at u+4)   # ident        dump: mission.units[ku].ident
        string(ref at u+8)   # initialMission   dump: mission.units[ku].initialMission
        HB(0x18 bytes at u+0xc)                         # dump: mission.units[ku] (hex:)
    H32(ruleCount);  HB(ruleCount*12 bytes at rules)    # dump: mission.rules.count, mission.rules (hex:)
    H32(featureCount)                                   # dump: mission.features.count
    for kf in 0 .. featureCount-1:
        f = features + kf*0x88
        string(f)            # the name is 0x80 inline bytes; never null     dump: mission.features[kf].name
        HB(8 bytes at f+0x80)                           # dump: mission.features[kf].pos (hex:)
END
```
A `ref` that is zero gives a null string pointer. `MissionData` and `MissionStartRest` have no
pointer fields.

The three counts are written after the clamp: a count is 0 when its array is null or the raw
count is above 65536, and `mission.units.count`, `mission.rules.count` and
`mission.features.count` hold that clamped value as 8 hex digits (always present when the record is).
`MissionStartRest` follows `MissionData` (its lines `mission.startMetalRest` and
`mission.startEnergyRest`) exactly when the mission type is 1, and so exactly when those lines are
present.

Line forms, for `ku` in `0 .. unitCount-1` and `kf` in `0 .. featureCount-1`:

| Path | Value |
| --- | --- |
| `mission.units[ku].name`, `.ident`, `.initialMission` | `null` or `str:` (three lines, in this order) |
| `mission.units[ku]` | `hex:` and the 0x18 bytes at offset 0xc of the unit entry |
| `mission.rules.count` | 8 hex digits |
| `mission.rules` | `hex:` and `ruleCount*12` bytes (an empty `hex:` when 0) |
| `mission.features[kf].name` | `str:` (read from the 0x80 inline bytes, and on past them into the next bytes of the entry if none of the 0x80 is zero, at most 255 bytes in all) |
| `mission.features[kf].pos` | `hex:` and 8 bytes |

The string refs of a unit entry are relative to the start of the state block, as every ref is.

### 8.21 path results (section 32, dumped only)

The events pathfinding delivered in this tick, in the order the engine took them from its `Poll` slot
(`interfaces/pathfinding.h`) in the `players` phase, with a buffer of 64 waypoints. No row hashes them
(section 6): a dump holds their lines, after those of section 27 (of section 31 at tick 1), as a diagnostic. The
host records each event as it takes it, before the engine acts on it, and empties the record after the
world row of the tick, so the lines a dump writes after an earlier phase hold the events delivered so
far. The fields are `results.fields` of the schema, in this order.

```
results.count                                  # n
for k in 0 .. n-1, in delivery order:
    results[k].kind
    results[k].status
    results[k].unit                            # the event's unit, a pointer ID with target unit (is_ref=true)
    results[k].tick
    results[k].count
    results[k].points                          # hex: the first min(max(count, 0), 64) waypoints, 4 bytes each: i16 x, i16 y
```

`results.count` and the four numbers of an event are written as `0x` and 8 hex digits.

- `kind` is 0 (a search started), 1 (a route so far, the search goes on) or 2 (a search ended).
- `status` is, for a start, 0 when the unit's cell is in the goal or a straight walk from it reaches
  the goal and 1 otherwise; for an end, 0 (found), 1 (partial: the route was cut short), 2 (failed,
  `count` 0) or 3 (nearest: the route ends at the reachable cell nearest the goal).
- The unit is the unit slot the request was for, as a pointer ID with target `unit` (section 5),
  null for none.
- `tick` is the game tick of the step that produced the event, `count` the waypoints of the route,
  which may be more than the 64 the buffer holds. Waypoints are in world units.
- The requester's handles are not dumped. What the requester keeps of a result is hashed where it is
  kept (sections 20 and 21).

3.1c's search, the default, delivers for each search it starts one start event and then one end event,
in the step that ends it: found with the corners of the route from the start (at most 64), partial with
the 64 corners nearest the start when the route had more, or failed. It never delivers a route so far
or a nearest end. A search that ends as it starts (the unit's cell is in the goal or off the map, or
the straight walk from it ends no cheaper than the start) delivers its start event and a failed end in
the same step.

### 8.22 economy events (section 33, dumped only)

The events and effects of economy in this tick (`interfaces/economy.h`). No row hashes them
(section 6): a dump holds their lines, after those of section 32, as a diagnostic. The reference
build does not record them, so its dumps have no `economy.*` lines. An event is recorded when economy
consumes it from its inbox, right after `Apply` returns, with the outcome `Apply` gave, and a SPEND when
the synchronous `Spend` answers, with its answer; an effect is recorded when economy publishes it to its
outbox, with outcome 1. So the records are in economy's order of consumption and publication, and the
events that an effect causes (a share's SPEND and INCOME) follow it. The host empties
the record after the world row of the tick, so the lines a dump writes after an earlier phase hold the
records made so far. Each record has the fields of its kind, the `fields` of its entry in `economy.kinds` of the schema, in the order of that list.
An event shorter than its kind's payload is recorded zero-filled past its size and dumped with every field of its kind, so
it reads as a full event whose missing fields are 0, with outcome 0; the dump does not write its size. The same holds for
the event sections 8.24 to 8.31.

```
economy.count                                  # n
for k in 0 .. n-1, in application order, by the kind of the record:
    economy[k].kind
    economy[k].<each field of that kind, in the order of its row below>
```

`economy.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. Fields in the table below are of the type shown: a plain field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 0 | ECONOMY_RESET | `player`, `unit` (unit), `outcome` |
| 1 | ECONOMY_LOAD | `player`, `unit` (unit), `account` (48 bytes), `outcome` |
| 2 | ECONOMY_REQUEST | `player`, `unit` (unit), `resources`, `energy`, `metal`, `outcome` |
| 3 | ECONOMY_SPEND | `player`, `unit` (unit), `resources`, `energy`, `metal`, `outcome` |
| 4 | ECONOMY_INCOME | `player`, `unit` (unit), `resources`, `energy`, `metal`, `outcome` |
| 16 | ECONOMY_EFFECT_CLOAK | `player`, `unit` (unit), `keep`, `outcome` |
| 17 | ECONOMY_EFFECT_EXTRACTOR | `player`, `unit` (unit), `metal`, `outcome` |
| 18 | ECONOMY_EFFECT_SHARE | `player`, `unit` (unit), `resources`, `energy`, `metal`, `to`, `outcome` |

- `kind` is an event: 0 RESET, 1 LOAD, 2 REQUEST, 3 SPEND, 4 INCOME; or an effect: 16 EFFECT_CLOAK,
  17 EFFECT_EXTRACTOR, 18 EFFECT_SHARE. `player` is the player index of the account's player (a unit's owner, or
  the player for unit 0), or the giver of a share. `unit` is null for a player's own account and for the kinds that
  name no unit.
- `resources` holds the bits of the resources the record is about: 1 energy, 2 metal. An INCOME carries exactly
  one bit. `energy` and `metal` are the amounts as IEEE 754 single bits. The `metal` of EFFECT_EXTRACTOR is the
  metal under the footprint (an integer); `keep` of EFFECT_CLOAK is 1 to keep the unit cloaked and 0 to uncloak it;
  `to` of EFFECT_SHARE is the player that receives the share.
- `account` of a LOAD is the 12 float words of a unit's resource account (the energy ledger, then the metal ledger,
  six floats each), in the order a saved game stores it, copied when the event is recorded.
- `outcome` is the result of `Apply`: 1, or 0 for a SPEND whose store was short; it is 1 for an effect.
- The resource accounts of units are private to economy and are not hashed (the Unit table leaves out
  bytes 0xbc to 0xeb, section 4); the player accounts of section 27 and the players' resource fields
  still are. What economy's `WriteState` reports (its record, numbered 34) is dumped between sections 15 and
  18, with the prefix `economyState.`, and is not hash input. ai's report (record 43, section 8.30) follows it.

### 8.23 share packets (section 35, dumped only)

The resource share packets the step sends to a remote slot (`player<N>.remote`, `docs/scenario-format.md`):
the metal and energy an ally gives and the share map. No row hashes them (section 6): a dump holds
their lines, after those of section 33, at the end of the `meteors` block only. A test-mode run has
no network, so these packets are recorded instead of sent, in the order the step sends them, and the
record is emptied after the step. A run with no remote slot sends none, and its dump has
`packets.count 0x00000000`. The fields are `packets.fields` of the schema, in this order.

```
packets.count                                  # n
for k in 0 .. n-1, in sending order:
    packets[k].type
    packets[k].subtype
    packets[k].from
    packets[k].to
    packets[k].amount
```

`packets.count` and the numbers of a record are written as `0x` and 8 hex digits.

- `type` is the packet type, 0x16 for every share packet.
- `subtype` is 1 for an energy share, 2 for a metal share and 3 for the share map.
- `from` and `to` are the network IDs of the giver and the receiver: the `id` field of each one's
  player record (section 7.4), or 0xffffffff for a player of type 0.
- `amount` is the amount given as IEEE 754 single bits (0 for the share map).
- A step records at most 64 packets; a run that sends more fails.

Which packets a step sends, and when, is behaviour of the build, not of the state: two builds whose
rows match can send the same packets in a different order within a step. The lines are a diagnostic of what the receiver would see.

### 8.24 wind_meteors events (section 37, dumped only)

The events and effects of wind_meteors in this tick (`interfaces/wind_meteors.h`). No row hashes them
(section 6): a dump holds their lines, after those of section 33, as a diagnostic. The reference
build does not record them, so its dumps have no `windMeteors.*` lines. An event is recorded when
wind_meteors consumes it from its inbox, right after `Apply` returns; an effect is recorded when
wind_meteors publishes it to its outbox. So the records are in wind_meteors' order of consumption and
publication. The host empties the record after the world row of the tick, so the lines a dump writes
after an earlier phase hold the records made so far. Each record has the fields of its kind, the `fields` of its entry in `windMeteors.kinds` of the schema, in the order of that list.

```
windMeteors.count                                  # n
for k in 0 .. n-1, in order of consumption and publication:
    windMeteors[k].kind
    windMeteors[k].<each field of that kind, in the order of its row below>
```

`windMeteors.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. Fields in the table below are of the type shown: a plain field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 64 | WIND_METEORS_ENABLE | none |
| 65 | WIND_METEORS_DISABLE | none |
| 66 | WIND_METEORS_PARAMS | `params` (48 bytes) |
| 67 | WIND_METEORS_START | none |
| 68 | WIND_METEORS_LIMITS | `min`, `max` |
| 69 | WIND_METEORS_RESET | none |
| 80 | WIND_METEORS_EFFECT_SPAWN | `weapon`, `pos` (12 bytes), `vel` (12 bytes), `announce` |

- `kind` is an event: 64 ENABLE, 65 DISABLE, 66 PARAMS, 67 START; or an effect: 80 EFFECT_SPAWN. ENABLE, DISABLE and
  START have no fields but `kind`. The section has no `outcome`.
- `params` of a PARAMS event is the meteor shower parameters: the weapon name in 32 bytes, then the radius, the
  density, the duration and the interval, 4 bytes each, the last three as IEEE 754 single bits.
- A SPAWN effect has `weapon` (the meteor weapon's slot in the match's table of weapons), `pos` and `vel` (the
  position and the velocity, 3 ints each) and `announce` (whether the spawn is announced to the other players).
- The meteor schedule and the 13 meteor globals are private to wind_meteors and are still hashed (section
  7.7); the wind fields of the game record are hashed in section 7.6.

### 8.25 features events (section 36, dumped only)

The events and effects of features in this tick (`interfaces/features.h`). No row hashes them
(section 6): a dump holds their lines, after those of section 37, as a diagnostic. The reference
build does not record them, so its dumps have no `features.*` lines. An event is recorded when features
consumes it from its inbox, right after `Apply` returns, with the outcome `Apply` gave, and a PLACE when
the synchronous `Place` answers, with its answer; an effect is recorded when features publishes it to its
outbox, with outcome 1. So the records are in features' order of consumption and publication, and the
effects an event causes come before the event: a DAMAGE that kills a feature dumps its FREE_STATE and PASS_MAPS first. The host
empties the record after the world row of the tick, so the lines a dump writes after an earlier phase
hold the records made so far. Each record has the fields of its kind, the `fields` of its entry in `features.kinds` of the schema, in the order of that list.

```
features.count                                  # n
for k in 0 .. n-1, in application order, by the kind of the record:
    features[k].kind
    features[k].<each field of that kind, in the order of its row below>
```

`features.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. Fields in the table below are of the type shown: a plain field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 32 | FEATURES_PLACE | `x`, `z`, `feature`, `flags`, `owner`, `pos` (12 bytes), `rot` (12 bytes), `outcome` |
| 33 | FEATURES_REMOVE | `x`, `z`, `force`, `outcome` |
| 34 | FEATURES_DAMAGE | `x`, `z`, `weapon`, `outcome` |
| 35 | FEATURES_RECLAIM | `unit` (unit), `pos` (12 bytes), `outcome` |
| 48 | FEATURES_EFFECT_FREE_STATE | `state` (handle), `outcome` |
| 49 | FEATURES_EFFECT_INIT_STATE | `state` (handle), `feature`, `outcome` |
| 50 | FEATURES_EFFECT_PASS_MAPS | `x`, `z`, `width`, `height`, `outcome` |
| 51 | FEATURES_EFFECT_AREA_DAMAGE | `weapon`, `pos` (12 bytes), `outcome` |

- `kind` is an event: 32 PLACE, 33 REMOVE, 34 DAMAGE, 35 RECLAIM; or an effect: 48 EFFECT_FREE_STATE,
  49 EFFECT_INIT_STATE, 50 EFFECT_PASS_MAPS, 51 EFFECT_AREA_DAMAGE.
- `flags` of a PLACE holds the bits: 1 `pos` is given, 2 `rot` is given, 4 the placed feature starts to sink.
- `x` and `z` are the cell the record acts on, or the first cell of a footprint; `width` and `height` the size of a
  footprint in cells. `feature` is the feature type by its number, `owner` the player that owns a placed feature
  (10 for none). `force` of a REMOVE is 1 to remove even a feature that refuses removal; `weapon` of a DAMAGE or an
  EFFECT_AREA_DAMAGE is the number of the weapon's definition.
- `unit` of a RECLAIM is the unit that reclaims, a pointer ID with target `unit` (section 5).
- `state` is the block offset of a piece state: `null` for 0, `heap` for any other value, because the offset depends
  on the allocator and is not behaviour.
- `pos` is a position in world coordinates with 16 fractional bits and `rot` a rotation in 16-bit angles, x, y and z.
- `outcome` is the result of `Apply` or of `Place`, or 1 for an effect.

### 8.26 targeting events (section 38, dumped only)

The events and effects of targeting in this tick (`interfaces/targeting.h`). No row hashes them
(section 6): a dump holds their lines, after those of section 36, as a diagnostic. The reference
build does not record them, so its dumps have no `targeting.*` lines. An event is recorded when targeting
consumes it from its inbox, right after `Apply` returns, with the outcome `Apply` gave; an effect is recorded
when targeting publishes it to its outbox, with outcome 1. So the records are in targeting's order of
consumption and publication. The host empties the record after the world row of the tick, so the lines a
dump writes after an earlier phase hold the records made so far. Each record has the fields of its kind, the `fields` of its entry in `targeting.kinds` of the schema, in the order of that list.

```
targeting.count                                  # n
for k in 0 .. n-1, in application order, by the kind of the record:
    targeting[k].kind
    targeting[k].<each field of that kind, in the order of its row below>
```

`targeting.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. Fields in the table below are of the type shown: a plain field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 96 | TARGETING_SET_UNIT | `unit` (unit), `weapon`, `target` (unit), `outcome` |
| 97 | TARGETING_SET_POS | `unit` (unit), `weapon`, `pos` (12 bytes), `outcome` |
| 98 | TARGETING_CLEAR | `unit` (unit), `weapon`, `outcome` |
| 99 | TARGETING_RESET | `unit` (unit), `weapon`, `outcome` |
| 100 | TARGETING_LOAD | `unit` (unit), `weapon`, `xOrUnit`, `zOrSentinel`, `outcome` |
| 112 | TARGETING_EFFECT_CLEARED | `unit` (unit), `weapon`, `outcome` |

- `kind` is an event: 96 SET_UNIT, 97 SET_POS, 98 CLEAR, 99 RESET, 100 LOAD; or an effect: 112 EFFECT_CLEARED.
  CLEAR, RESET and EFFECT_CLEARED have only the header fields (`kind`, `unit`, `weapon`) and `outcome`.
- `unit` is the unit whose weapon aims, a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for
  a pointer that is not in the unit table. `weapon` is the weapon of the unit, 0 to 2. `target` of a SET_UNIT is the
  unit aimed at, resolved as `unit` is.
- `pos` of a SET_POS is the position aimed at in world coordinates with 16 fractional bits, x, y and z. `xOrUnit` and
  `zOrSentinel` of a LOAD are the two halves of the saved target pair.
- `outcome` is the result of `Apply` (1 for SET_UNIT, SET_POS and LOAD; for CLEAR and RESET 1 when the weapon had a
  target and 0 when not), or 1 for an effect.

### 8.27 los_radar events (section 39, dumped only)

The events of los_radar in this tick (`interfaces/los_radar.h`). No row hashes them (section 6): a dump
holds their lines, after those of section 38, as a diagnostic. The reference build does not record them, so
its dumps have no `los_radar.*` lines. An event is recorded when los_radar consumes it from its inbox, right
after `Apply` returns, with the outcome `Apply` gave; los_radar publishes no effects. So the records are in
los_radar's order of consumption. The host empties the record after the world row of the tick, so the lines
a dump writes after an earlier phase hold the records made so far. The record has no fixed capacity, as a tick
may hold an event for each unit that moved; a run that cannot hold it stops with an error. Each record has the fields of its kind, the `fields` of its entry in `los_radar.kinds` of the schema, in the order of that list.

```
los_radar.count                                  # n
for k in 0 .. n-1, in application order, by the kind of the record:
    los_radar[k].kind
    los_radar[k].<each field of that kind, in the order of its row below>
```

`los_radar.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. Fields in the table below are of the type shown: a plain field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 128 | LOS_RADAR_UPDATE_UNIT | `unit` (unit), `outcome` |
| 129 | LOS_RADAR_REMOVE_UNIT | `unit` (unit), `outcome` |
| 130 | LOS_RADAR_REVEAL_UNIT | `unit` (unit), `outcome` |
| 131 | LOS_RADAR_ADD_EYEBALL | `pos` (12 bytes), `sightDistance`, `eyeHeight`, `lifetime`, `outcome` |
| 132 | LOS_RADAR_CELL_HEIGHTS | `cell` (8 bytes), `size` (8 bytes), `outcome` |
| 133 | LOS_RADAR_RECALCULATE | `reset`, `outcome` |
| 134 | LOS_RADAR_SHARE_MAPPING | `player`, `outcome` |
| 135 | LOS_RADAR_SHARE_RADAR | `player`, `outcome` |
| 136 | LOS_RADAR_SHARE_MAP | `from`, `to`, `outcome` |
| 137 | LOS_RADAR_SIGHT_RESET | `unit` (unit), `outcome` |
| 138 | LOS_RADAR_SIGHT_LOAD | `unit` (unit), `cell`, `frameIdx`, `outcome` |

- `kind` is an event: 128 UPDATE_UNIT, 129 REMOVE_UNIT, 130 REVEAL_UNIT, 131 ADD_EYEBALL, 132 CELL_HEIGHTS,
  133 RECALCULATE, 134 SHARE_MAPPING, 135 SHARE_RADAR, 136 SHARE_MAP, 137 SIGHT_RESET, 138 SIGHT_LOAD.
- `unit` of UPDATE_UNIT, REMOVE_UNIT, REVEAL_UNIT, SIGHT_RESET and SIGHT_LOAD is the unit whose line of sight changes,
  a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for a pointer that is not in the unit table.
- `player` of SHARE_MAPPING and SHARE_RADAR is the player that shares, 0 to 9. `from` and `to` of SHARE_MAP are the
  player whose map is shared and the player it is shown to, 0 to 9. `reset` of RECALCULATE is 1 when each cell is
  first reset (to show nothing to every player when mapping is on, to show every cell to every player when it is off).
- `pos`, `sightDistance`, `eyeHeight` and `lifetime` of an ADD_EYEBALL are the position in world coordinates with 16
  fractional bits, x, y and z, the sight distance and the height of the eye above the position, in world units, and
  the lifetime in ticks.
- `cell` is the first cell, x and y, and `size` the width and height, in cells, of the rectangle of a CELL_HEIGHTS.
- `cell` and `frameIdx` of a SIGHT_LOAD are the sight cache it sets for the unit: the cell of the last sight circle
  (x in the low 16 bits, z in the high 16) and the table frame index.
- `outcome` is the result of `Apply`: 1 for every kind but these: 0 when the kind names a unit and `unit` is null; for
  ADD_EYEBALL 0 when line of sight is off or no eyeball is free; for SHARE_MAPPING and SHARE_RADAR 0 outside a
  network game.

### 8.28 map events (section 40, dumped only)

The events and effects of map in this tick (`interfaces/map.h`). No row hashes them (section 6): a dump
holds their lines, after those of section 39, as a diagnostic. The reference build does not record them, so
its dumps have no `map.*` lines. An event is recorded when map consumes it from its inbox, right after
`Apply` returns, with the outcome `Apply` gave; an effect is recorded when map publishes it, just before `SchedPublish`, with outcome 1.
So the records are in the order map finished consuming and published them: an effect that `Apply` publishes
(the CELLS_CHANGED of a building's ADD_UNIT, of a SET_YARD or of a PASS_MAPS) comes before the event that caused it,
which is recorded only once `Apply` has returned its outcome. A reader at a divergence takes the event after a run of
effects as their cause. The host empties the record after the world
row of the tick, so the lines a dump writes after an earlier phase hold the records made so far. The record has
no fixed capacity, as a tick may hold an event for each unit that moved; a run that cannot hold it stops with
an error. Each record has the fields of its kind, the `fields` of its entry in `map.kinds` of the schema, in the order of that list.

```
map.count                                  # n
for k in 0 .. n-1, in order, by the kind of the record:
    map[k].kind
    map[k].<each field of that kind, in the order of its row below>
```

`map.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. Fields in the table below are of the type shown: a plain field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 160 | MAP_ADD_UNIT | `unit` (unit), `outcome` |
| 161 | MAP_REMOVE_UNIT | `unit` (unit), `outcome` |
| 162 | MAP_CLEAR_UNIT | `unit` (unit), `outcome` |
| 163 | MAP_RECLAIM_FOOTPRINT | `unit` (unit), `outcome` |
| 164 | MAP_SET_YARD | `unit` (unit), `open`, `outcome` |
| 165 | MAP_BUCKET_UNLINK | `unit` (unit), `outcome` |
| 166 | MAP_BUCKET_PREPEND | `unit` (unit), `outcome` |
| 168 | MAP_PASS_MAPS | `unit` (unit), `cell` (8 bytes), `size` (8 bytes), `outcome` |
| 169 | MAP_LOAD_PATH_LOCK_STAMP | `unit` (unit), `stamp`, `outcome` |
| 176 | MAP_EFFECT_CELLS_CHANGED | `unit` (unit), `rect` (16 bytes), `classes`, `cause`, `outcome` |

- `kind` is an event or an effect: 160 ADD_UNIT, 161 REMOVE_UNIT, 162 CLEAR_UNIT, 163 RECLAIM_FOOTPRINT, 164 SET_YARD,
  165 BUCKET_UNLINK, 166 BUCKET_PREPEND, 168 PASS_MAPS, 169 LOAD_PATH_LOCK_STAMP (map 7.1), and the effect 176
  EFFECT_CELLS_CHANGED. 167 was
  REFRESH_PASS_MAP, which map 5.0 (#983) removed: the refreshes of the pass maps around a search are map's own slots,
  and each pass map they compute again is a CELLS_CHANGED record. Every kind but SET_YARD, PASS_MAPS,
  LOAD_PATH_LOCK_STAMP and EFFECT_CELLS_CHANGED has only `kind`, `unit` and `outcome`.
- `unit` is the unit the event names, a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for a
  pointer that is not in the unit table.
- `open` of SET_YARD is 1 to open the yard and 0 to close it.
- `stamp` of LOAD_PATH_LOCK_STAMP is the path lock stamp, a tick, that the saved game keeps for the unit's movement.
- `cell` is the first cell, x and y, and `size` the width and height, in cells, of the rectangle of a PASS_MAPS.
- `rect` of EFFECT_CELLS_CHANGED is the rectangle, left, top, right and bottom, in cells, `classes` the movement
  classes whose pass maps changed, a bit for each class index, and `cause` the reason for the change.
- `outcome` is the result of `Apply`: 1 for every kind but these: 0 when the kind names a unit and `unit` is null; for
  SET_YARD 0 when the footprint was not clear; for LOAD_PATH_LOCK_STAMP 0 when the unit has no movement. It is 1 for an
  effect.

Map's own report (record 45, written through the `StateWriter` of the seam's `WriteState`) is dumped and never hashed, with the
prefix `mapState.`, after the `aiState.*` lines:

```
mapState.spatial.count                         # as section 14 counts it
for r in 0 .. count-1:
    mapState.spatial[r].a                      # 1 byte
    mapState.spatial[r].flags                  # 4 bytes
    mapState.spatial[r].first                  # a pointer ID with target unit
mapState.overflow.present                      # 1 when the overflow record exists
if present: mapState.overflow.a, .flags, .first   # as a record above
```

### 8.29 players events (section 41, dumped only)

The events and effects of players in this tick (`interfaces/players.h`). No row hashes them (section 6): a dump
holds their lines, after those of section 40, as a diagnostic. The reference build does not record them, so
its dumps have no `playersEvents.*` lines. An event is recorded when players consumes it from its inbox, right
after `Apply` returns, with the outcome `Apply` gave; an effect is recorded when the scheduler delivers it to
its subscriber, with outcome 1. So the records are in the order players consumed and the scheduler delivered
them. The host empties the record after the world row of the tick, so the lines a dump writes after an earlier
phase hold the records made so far. The record has no fixed capacity; a run that cannot hold it stops with an
error. Each record has the fields of its kind, the `fields` of its entry in `playersEvents.kinds` of the schema, in the order of that list. The section is dumped, never hashed.

```
playersEvents.count                                  # n
for k in 0 .. n-1, in order, by the kind of the record:
    playersEvents[k].kind
    playersEvents[k].<each field of that kind, in the order of its row below>
```

`playersEvents.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. Fields in the table below are of the type shown: a plain field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 192 | PLAYERS_GIVE_UNIT | `unit` (unit), `player`, `record.present`, `record.hitPoints`, `record.buildPercent`, `record.angles` (12 bytes), `record.stockpiles` (12 bytes), `outcome` |
| 193 | PLAYERS_KILL_UNITS | `unit` (unit), `player`, `outcome` |
| 194 | PLAYERS_UNIT_CREATED | `unit` (unit), `outcome` |
| 195 | PLAYERS_UNIT_DIED | `unit` (unit), `outcome` |
| 196 | PLAYERS_TRANSFER | `unit` (unit), `from`, `to`, `resource`, `amount`, `announce`, `outcome` |
| 208 | PLAYERS_EFFECT_KILL_UNIT | `unit` (unit), `selfDestruct`, `outcome` |
| 209 | PLAYERS_EFFECT_SEND_UNIT | `unit` (unit), `player`, `outcome` |
| 210 | PLAYERS_EFFECT_CAPTURE_UNIT | `unit` (unit), `player`, `record.present`, `record.hitPoints`, `record.buildPercent`, `record.angles` (12 bytes), `record.stockpiles` (12 bytes), `outcome` |

- `kind` is an event into players' inbox: 192 GIVE_UNIT, 193 KILL_UNITS, 194 UNIT_CREATED, 195 UNIT_DIED,
  196 TRANSFER; or an effect players publishes to units: 208 EFFECT_KILL_UNIT, 209 EFFECT_SEND_UNIT,
  210 EFFECT_CAPTURE_UNIT. UNIT_CREATED and UNIT_DIED have only `kind`, `unit` and `outcome`.
- `unit` is the unit the kind names, a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for a
  pointer that is not in the unit table.
- `player` is the player the kind is about, 0 to 9, or 10 for none: the receiver of a unit, or the player whose units
  are killed. `from` and `to` of a TRANSFER are the giver and the receiver; `resource` is the resource moved, `amount`
  the amount, the bits of a float, and `announce` is 1 when a transfer is announced (clamped, spent and sent to the
  other machines) and 0 when it only credits the receiver. `selfDestruct` of EFFECT_KILL_UNIT is 0 when the unit dies
  by 30000 damage it deals itself and 1 when it self-destructs.
- `record.present` is 1 when the other `record` fields come from an ownership record received from another machine:
  `record.hitPoints`, `record.buildPercent`, `record.angles` (bank, heading and pitch, three little-endian i32) and
  `record.stockpiles` (of the unit's three weapons, three little-endian i32).
- `outcome` is the result of `Apply`: for GIVE_UNIT 1 when the unit changed hands and 0 when it was already the
  player's, dead, dying or the player could not take it; for TRANSFER 1, or 0 when either player is 10 or the amount is
  0; 0 for any event that names a null unit or a player above 10; 1 for the other kinds. It is 1 for an effect.

### 8.30 ai events (section 42, dumped only) and ai's report (record 43)

The events and effects of ai in this tick (`interfaces/ai.h`). No row hashes them (section 6): a dump holds
their lines, after those of section 41, as a diagnostic. The reference build does not record them, so its
dumps have no `aiEvents.*` lines. An event is recorded when ai consumes it from its inbox, right after `Apply`
returns, with the outcome `Apply` gave; an effect is recorded when ai publishes it, with outcome 1. So the
records are in the order ai consumed and published them. The host empties the record after the world row of
the tick, so the lines a dump writes after an earlier phase hold the records made so far. Each record has the fields of its kind, the `fields` of its entry in `aiEvents.kinds` of the schema, in the order of that list. The section is dumped, never hashed.

```
aiEvents.count                                  # n
for k in 0 .. n-1, in order, by the kind of the record:
    aiEvents[k].kind
    aiEvents[k].<each field of that kind, in the order of its row below>
```

`aiEvents.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. Fields in the table below are of the type shown: a plain field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 224 | AI_REACT_TO_ATTACK | `unit` (unit), `attacker` (unit), `damage`, `outcome` |
| 226 | AI_SET_SQUAD | `unit` (unit), `squad`, `outcome` |
| 227 | AI_PLAN | `unit` (unit), `difficulties`, `outcome` |
| 228 | AI_WEIGHT | `unit` (unit), `scale`, `fix`, `types` (64 bytes), `outcome` |
| 229 | AI_LIMIT | `unit` (unit), `limit`, `fix`, `types` (64 bytes), `outcome` |
| 230 | AI_RELOAD_PROFILES | `unit` (unit), `outcome` |
| 240 | AI_EFFECT_ADD_ORDER | `unit` (unit), `type`, `queue`, `target` (unit), `hasPos`, `pos` (12 bytes), `params` (8 bytes), `outcome` |
| 241 | AI_EFFECT_QUEUE_BUILD | `unit` (unit), `type`, `count`, `outcome` |
| 243 | AI_EFFECT_STOP | `unit` (unit), `all`, `outcome` |
| 245 | AI_EFFECT_ACTIVATE | `unit` (unit), `bits`, `on`, `outcome` |
| 246 | AI_EFFECT_SET_STATES | `unit` (unit), `movement`, `firing`, `outcome` |

- `kind` is an event into ai's inbox: 224 REACT_TO_ATTACK, 226 SET_SQUAD, 227 PLAN, 228 WEIGHT, 229 LIMIT,
  230 RELOAD_PROFILES; or an
  effect ai publishes: 240 EFFECT_ADD_ORDER, 241 EFFECT_QUEUE_BUILD, 243 EFFECT_STOP, 245 EFFECT_ACTIVATE,
  246 EFFECT_SET_STATES. In ai 5.0 the kinds 225 (DAMAGE_SPLIT), 242 (ATTACK), 244 (NOTIFY_REFS) and 247 (HIT) are
  gone and not reused.
- `unit` is the unit the kind is about, `attacker` the unit that attacks in a REACT_TO_ATTACK and `target` the unit an
  order is aimed at, each a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for a pointer that
  is not in the unit table.
- `damage`, `squad`, `difficulties`, `limit`, `fix`, `type`, `queue`, `count`, `all`, `bits`, `on`, `movement` and
  `firing` are the arguments of their kinds; `params` the two arguments of an ADD_ORDER, `hasPos` is 1 when `pos` holds
  a position (x, y and z with 16 fractional bits), `scale` is the factor of WEIGHT (the bits of a float) and `types`
  the unit types of WEIGHT and LIMIT as a set of bits: type n is bit n % 32 of word n / 32.
- `outcome` is the result of `Apply` for an event: 1, or 0 for a WEIGHT or LIMIT while the AI commands are off. It is
  1 for an effect.

ai's own report (record 43, written through the `StateWriter` of the seam's `WriteState`) is dumped and never
hashed, after the `economyState.*` lines that follow the pathfinder's, with the prefix `aiState.`:

```
aiState.commandsEnabled                        # 1 when the AI commands are on
aiState.count                                  # n, the players that have a squad manager
for k in 0 .. n-1, in player order:
    aiState.manager[k].player
    aiState.manager[k].countdown
    aiState.manager[k].nextAction
    aiState.manager[k].timers                  # hex: the 40 bytes, the tick each of the 10 squad timers runs next, 0 for none
```

Since format 6 the same report holds, after those lines, what sections 24 to 26 hashed of `PlayerAI`, the squad managers and the squads
(each a record 43 begun by the name shown, the fields as `<name>.<field>`):

```
for i in 0 .. 9 whose AI record is not null:
    aiState.players[i].ai.<field>              # the table PlayerAI in order, without hasSpecial (los_radar's, section 8.13)
    aiState.players[i].ai.<vector>.size        # each raw vector: cells 0x4d 8; vec_65 0x65 3; counts 0x7d 2; vec_8d 0x8d 1;
    aiState.players[i].ai.<vector>             #   weights 0x9d 1; vec_ad 0xad 1; vec_bd 0xbd 4; values 0xcd 4; locked 0xdd 4
                                               #   (the element count, then the hex: bytes)
for i in 0 .. 9 whose player has a squad manager:
    aiState.players[i].squadManager.<field>    # the table SquadManager
    aiState.players[i].squadManager.timers[t].class    # for each of the 10 slots t with a timer, the class of section 8.14
    aiState.players[i].squadManager.timers[t].<field>  # the table of that class: SquadTimer, AssaultTimer, EscortTimer or SpatialTimer
for i in 0 .. 9 whose player has squads, s in 0 .. 9:
    aiState.players[i].squads[s].<field>       # the table Squad
    aiState.players[i].squads[s].items.size, aiState.players[i].squads[s].items[k]   # the unit vector
```

### 8.31 weapons events (section 47, dumped only) and weapons' report (record 48)

The events and effects of weapons in this tick (`interfaces/weapons.h`). No row hashes them (section 6): a dump
holds their lines, after those of section 42, as a diagnostic. The reference build does not record them, so its
dumps have no `weaponsEvents.*` lines. An event is recorded when weapons consumes it from its inbox, right after
`Apply` returns, with the outcome `Apply` gave; an effect is recorded when weapons publishes it, with outcome 1.
So the records are in the order weapons consumed and published them. The host empties the record after the world
row of the tick, so the lines a dump writes after an earlier phase hold the records made so far. Each record has the fields of its kind, the `fields` of its entry in `weaponsEvents.kinds` of the schema, in the order of that list. The section is dumped, never hashed.

```
weaponsEvents.count                                  # n
for k in 0 .. n-1, in order, by the kind of the record:
    weaponsEvents[k].kind
    weaponsEvents[k].<each field of that kind, in the order of its row below>
```

`weaponsEvents.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. Fields in the table below are of the type shown: a plain field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 256 | WEAPONS_DAMAGE | `unit` (unit), `source` (unit), `amount`, `damageKind`, `direction`, `outcome` |
| 257 | WEAPONS_REMOTE_DAMAGE | `unit` (unit), `source` (unit), `amount`, `damageKind`, `direction`, `outcome` |
| 258 | WEAPONS_DETONATE | `unit` (unit), `selfDestruct`, `outcome` |
| 259 | WEAPONS_RELEASE | `unit` (unit), `weapon`, `outcome` |
| 260 | WEAPONS_CLAIM | `unit` (unit), `weapon`, `outcome` |
| 261 | WEAPONS_INIT_SLOTS | `unit` (unit), `outcome` |
| 262 | WEAPONS_REMOVE_PROJECTILES | `unit` (unit), `outcome` |
| 263 | WEAPONS_AIM_DONE | `unit` (unit), `callback`, `result`, `outcome` |
| 264 | WEAPONS_EXPLODE_PIECE | `unit` (unit), `piece`, `flags`, `outcome` |
| 265 | WEAPONS_AREA_DAMAGE | `unit` (unit), `weapon`, `pos` (12 bytes), `outcome` |
| 266 | WEAPONS_SPAWN | `unit` (unit), `weapon`, `pos` (12 bytes), `vel` (12 bytes), `announce`, `outcome` |
| 267 | WEAPONS_SET_STOCKPILES | `unit` (unit), `stockpiles` (12 bytes), `outcome` |
| 269 | WEAPONS_ADD_STOCKPILE | `unit` (unit), `weapon`, `outcome` |
| 272 | WEAPONS_EFFECT_ATTACKED | `unit` (unit), `attacker` (unit), `amount`, `outcome` |
| 273 | WEAPONS_EFFECT_HIT | `unit` (unit), `enemyDamage`, `friendlyDamage`, `outcome` |
| 274 | WEAPONS_EFFECT_MARK_DAMAGED | `unit` (unit), `outcome` |
| 275 | WEAPONS_EFFECT_DAMAGE_RECORD | `unit` (unit), `damageKind`, `attacker` (unit), `outcome` |
| 276 | WEAPONS_EFFECT_HEALTH | `unit` (unit), `health`, `dying`, `outcome` |
| 277 | WEAPONS_EFFECT_PARALYZE | `unit` (unit), `ticks`, `outcome` |
| 278 | WEAPONS_EFFECT_START_SCRIPT | `unit` (unit), `script`, `callback`, `update`, `withArgs`, `count`, `args` (16 bytes), `outcome` |
| 279 | WEAPONS_EFFECT_DETACH_PIECE | `unit` (unit), `piece`, `pose`, `outcome` |
| 280 | WEAPONS_EFFECT_SHOT_FIRED | `unit` (unit), `outcome` |
| 281 | WEAPONS_EFFECT_BUSY | `unit` (unit), `until`, `outcome` |
| 282 | WEAPONS_EFFECT_COMMAND_SHOT_FIRED | `unit` (unit), `outcome` |
| 283 | WEAPONS_EFFECT_OUT_OF_REACH | `unit` (unit), `outcome` |

- `kind` is a damage event into weapons' inbox: 256 DAMAGE, 257 REMOTE_DAMAGE; another event: 258 DETONATE, 259 RELEASE,
  260 CLAIM, 261 INIT_SLOTS, 262 REMOVE_PROJECTILES, 263 AIM_DONE, 264 EXPLODE_PIECE, 265 AREA_DAMAGE, 266 SPAWN,
  267 SET_STOCKPILES, 269 ADD_STOCKPILE (268 LOAD_SLOT is gone: the seam's `LoadState` is called directly); a damage
  effect weapons publishes: 272 EFFECT_ATTACKED, 274 EFFECT_MARK_DAMAGED, 275 EFFECT_DAMAGE_RECORD, 276 EFFECT_HEALTH,
  277 EFFECT_PARALYZE; or another effect: 273 EFFECT_HIT, 278 EFFECT_START_SCRIPT, 279 EFFECT_DETACH_PIECE,
  280 EFFECT_SHOT_FIRED, 281 EFFECT_BUSY, 282 EFFECT_COMMAND_SHOT_FIRED, 283 EFFECT_OUT_OF_REACH. INIT_SLOTS,
  REMOVE_PROJECTILES, EFFECT_MARK_DAMAGED, EFFECT_SHOT_FIRED, EFFECT_COMMAND_SHOT_FIRED and EFFECT_OUT_OF_REACH have
  only `kind`, `unit` and `outcome`. The three shot kinds are what the earlier NOTICE effect carried in its bits
  0x400, 0x800 and 0x1000.
- `unit` is the unit the kind is about, a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for a
  pointer that is not in the unit table. `source` of a DAMAGE or REMOTE_DAMAGE and `attacker` of an EFFECT_ATTACKED or
  EFFECT_DAMAGE_RECORD are the other unit, resolved the same way.
- `callback` of an AIM_DONE or EFFECT_START_SCRIPT is the aim callback of a weapon, as its block offset (0 for none).
  `script` of EFFECT_START_SCRIPT is the script to start: 0 none, 1 to 3 the fire scripts of the three weapons, 4 to 6
  their aim scripts, 7 rock unit, 8 set max reload time, 9 hit by weapon, 10 take damage; `update`, `withArgs`,
  `count` and `args` (four i32) are how it starts.
- `pos` is a position in world coordinates with 16 fractional bits (x, y and z), `vel` a velocity in world units per
  tick with 16 fractional bits (x, y and z). `weapon` is a weapon definition's number for AREA_DAMAGE and SPAWN, the unit's weapon slot (0 to 2, or 3 for all
  three) for RELEASE and CLAIM, and the slot (0 to 2) for ADD_STOCKPILE; `stockpiles` the three weapon
  stockpiles of SET_STOCKPILES, and `amount`, `damageKind`, `direction`, `enemyDamage`, `friendlyDamage`, `health`,
  `dying`, `ticks`, `until`, `piece`, `flags`, `pose`, `selfDestruct`, `announce` and `result` the arguments of their
  kinds, as the kind says.
- `outcome` is the result of `Apply` for an event, and 1 for an effect.

weapons' own report (record 48, written through the `StateWriter` of the seam's `WriteState`) is dumped and never
hashed, after the `aiState.*` lines that follow the economy's, with the prefix `weaponsState.`. Each line is a 4-byte
integer, the number of used entries of a table:

```
weaponsState.projectiles                       # the projectile table
weaponsState.explosions                        # the explosions table
weaponsState.debris                            # the debris table
weaponsState.pieces                            # the exploded-piece table
```

Since format 8 the record also holds what left the hash, in this order (each line `weaponsState.<name>.<field>`, the
fields in the schema's order, a pointer as `null` or `heap`):

```
weaponsState.explodedPieces[s].<field>         # for each slot s in 0 .. 99 with a block: the table ExplodedPiece, then PieceRec as rec.<field>
weaponsState.debris[d].<field>                 # for every d in 0 .. min(count, 300)-1, a debris without a model included: the table Debris
weaponsState.pieces[i].<field>                 # for each piece of debris i remembered: the table Piece,
weaponsState.pieces[i].vertices                #   a hex: line of 96 bytes, and for k in 0 .. 5
weaponsState.pieces[i].faces[k].<field>        #   the table Face
weaponsState.units[i].weapons[k].aimCob.vtable # for each live unit i, k in 0 .. 2: the slot words that left the hash:
weaponsState.units[i].weapons[k].aimCob.done   #   the aim callback (the u32 at +4 as a handle, then the 4 bytes at +8),
weaponsState.units[i].weapons[k].muzzleAimFromDeltaZ   # 4 bytes
weaponsState.units[i].weapons[k].reloadTimer   # 2 bytes
weaponsState.units[i].weapons[k].fireHeading   # 2 bytes
weaponsState.units[i].weapons[k].firePitch     # 2 bytes
weaponsState.projectiles[p].<field>            # for each used projectile p (the skip test of section 7.2): start.x, start.y, start.z,
                                               #   direction.x, .y, .z, angles.bank, .heading, .pitch, speed, range, spawnTick, time,
                                               #   nextSmokeTick, the handle at +0x4e, the handle at +0x56, cellX, cellZ, radius, active,
                                               #   piece, propellerSpin, compactIndex (2 bytes) and flags (2 bytes)
```

The counts come first, then these lines in this order. The reference build writes the same lines, and the four counts
only in the moddable build. A unit's slot k starts at byte 4 + 0x1c*k of the unit; the aim callback's handle is the u32 at +4 of the
slot and `done` the 4 bytes at +8.
### 8.32 orders events (section 48, dumped only)

The events and effects of orders in this tick (`interfaces/orders.h`). No row hashes them (section 6): a dump
holds their lines, after those of section 47, as a diagnostic. The reference build does not record them, so its
dumps have no `ordersEvents.*` lines. An event is recorded when orders consumes it from its inbox, right after
`Apply` returns, with the outcome `Apply` gave; an effect is recorded when orders publishes it, with outcome 1.
So the records are in the order orders consumed and published them. The host empties the record after the world
row of the tick, so the lines a dump writes after an earlier phase hold the records made so far. Each record has the fields of its kind, the `fields` of its entry in `ordersEvents.kinds` of the schema, in the order of that list. The section is dumped, never hashed.

```
ordersEvents.count                                  # n
for k in 0 .. n-1, in order, by the kind of the record:
    ordersEvents[k].kind
    ordersEvents[k].<each field of that kind, in the order of its row below>
```

`ordersEvents.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. A field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 288 | ORDERS_ADD | `unit` (unit), `type`, `queue`, `target` (unit), `hasPos`, `pos` (12 bytes), `params` (8 bytes), `outcome` |
| 289 | ORDERS_QUEUE_BUILD | `unit` (unit), `count`, `name` (32 bytes), `outcome` |
| 290 | ORDERS_STOP | `unit` (unit), `all`, `outcome` |
| 291 | ORDERS_BE_CARRIED | `unit` (unit), `outcome` |
| 292 | ORDERS_STATUS_FLAGS | `unit` (unit), `order` (handle), `flags`, `outcome` |
| 293 | ORDERS_ATTACKED | `unit` (unit), `attacker` (unit), `outcome` |
| 294 | ORDERS_PARALYZE | `unit` (unit), `ticks`, `outcome` |
| 295 | ORDERS_COMMAND | `unit` (unit), `type`, `queue`, `target` (unit), `hasPos`, `pos` (12 bytes), `params` (8 bytes), `outcome` |
| 304 | ORDERS_EFFECT_SET_STATE_BITS | `unit` (unit), `mask`, `set`, `outcome` |
| 305 | ORDERS_EFFECT_SET_POSITION | `unit` (unit), `pos` (12 bytes), `mode`, `outcome` |
| 306 | ORDERS_EFFECT_ATTACH_TO_PIECE | `unit` (unit), `target` (unit), `piece`, `mode`, `outcome` |
| 307 | ORDERS_EFFECT_FINISH_CONSTRUCTION | `unit` (unit), `builder` (unit), `outcome` |
| 308 | ORDERS_EFFECT_BUILD_DECAY | `unit` (unit), `ticks`, `outcome` |
| 309 | ORDERS_EFFECT_REPAIR | `unit` (unit), `builder` (unit), `amount`, `outcome` |
| 310 | ORDERS_EFFECT_UNLINK | `unit` (unit), `ref` (handle), `outcome` |
| 311 | ORDERS_EFFECT_START_SCRIPT | `unit` (unit), `form`, `index`, `name` (32 bytes), `callback`, `update`, `count`, `args` (16 bytes), `outcome` |
| 312 | ORDERS_EFFECT_BUSY | `unit` (unit), `until`, `outcome` |
| 313 | ORDERS_EFFECT_SET_FLAGS | `unit` (unit), `clear`, `set`, `outcome` |
| 314 | ORDERS_EFFECT_CLEAR_NET_DIRTY | `unit` (unit), `mask`, `outcome` |
| 315 | ORDERS_EFFECT_SET_STANCE | `unit` (unit), `fields`, `moveOrder`, `fireOrder`, `group`, `outcome` |
| 316 | ORDERS_EFFECT_SET_ANGLES | `unit` (unit), `bank`, `heading`, `pitch`, `outcome` |
| 317 | ORDERS_EFFECT_SET_BUILD_STATE | `unit` (unit), `buildLeft`, `health`, `outcome` |

- `unit` is the unit the kind is about, and `target`, `attacker` and `builder` are other units, each a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for a pointer that is not in the unit table. `order` and `ref` are an order and a unit reference of a unit, as block offsets (handles).
- `pos` is a position in world coordinates with 16 fractional bits (x, y and z), `params` the two arguments of an order, `hasPos` is 1 when `pos` holds the order's position, and `callback` the callback told when a script returns, as its block offset. `type`, `queue`, `all`, `count`, `name`, `flags`, `ticks`, `mask`, `set`, `mode`, `piece`, `amount`, `form`, `index`, `update`, `args`, `until`, `clear`, `fields`, `moveOrder`, `fireOrder`, `group`, `bank`, `heading`, `pitch`, `buildLeft` and `health` are the arguments of their kinds, as the kind says. 288 to 295 are events into orders' inbox (ADD, QUEUE_BUILD, STOP, BE_CARRIED, STATUS_FLAGS, ATTACKED, PARALYZE, COMMAND) and 304 and up are effects orders publishes.
- `outcome` is the result of `Apply` for an event, and 1 for an effect.


### 8.33 movement events (section 49, dumped only)

The events and effects of movement in this tick (`interfaces/movement.h`). No row hashes them (section 6): a dump
holds their lines, after those of section 48, as a diagnostic. The reference build does not record them, so its
dumps have no `movementEvents.*` lines. An event is recorded when movement consumes it from its inbox, right after
`Apply` returns, with the outcome `Apply` gave; an effect is recorded when movement publishes it, with outcome 1.
So the records are in the order movement consumed and published them. The host empties the record after the world
row of the tick, so the lines a dump writes after an earlier phase hold the records made so far. Each record has the fields of its kind, the `fields` of its entry in `movementEvents.kinds` of the schema, in the order of that list. The section is dumped, never hashed.

```
movementEvents.count                                  # n
for k in 0 .. n-1, in order, by the kind of the record:
    movementEvents[k].kind
    movementEvents[k].<each field of that kind, in the order of its row below>
```

`movementEvents.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. A field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 320 | MOVEMENT_NEW | `unit` (unit), `outcome` |
| 321 | MOVEMENT_DESTROY | `unit` (unit), `outcome` |
| 322 | MOVEMENT_LOAD | `unit` (unit), `state` (40 bytes), `outcome` |
| 323 | MOVEMENT_SET_MODE | `unit` (unit), `mode`, `outcome` |
| 324 | MOVEMENT_SET_SPEED | `unit` (unit), `speed`, `outcome` |
| 326 | MOVEMENT_SET_TARGET | `unit` (unit), `target` (handle), `outcome` |
| 327 | MOVEMENT_DESTROY_TARGET | `unit` (unit), `target` (handle), `outcome` |
| 328 | MOVEMENT_SET_FLIGHT_MODE | `unit` (unit), `mode`, `outcome` |
| 329 | MOVEMENT_SET_ALTITUDE | `unit` (unit), `target` (handle), `value`, `outcome` |
| 330 | MOVEMENT_SET_APPROACH_RADIUS | `unit` (unit), `target` (handle), `value`, `outcome` |
| 331 | MOVEMENT_SET_HEADING | `unit` (unit), `target` (handle), `value`, `outcome` |
| 336 | MOVEMENT_EFFECT_ORDER_FLAGS | `unit` (unit), `order` (handle), `flags`, `outcome` |
| 337 | MOVEMENT_EFFECT_TURN | `unit` (unit), `delta`, `outcome` |
| 338 | MOVEMENT_EFFECT_TILT | `unit` (unit), `bank`, `pitch`, `outcome` |
| 339 | MOVEMENT_EFFECT_ANGLES | `unit` (unit), `bank`, `heading`, `pitch`, `outcome` |
| 340 | MOVEMENT_EFFECT_MOVE | `unit` (unit), `pos` (12 bytes), `outcome` |
| 341 | MOVEMENT_EFFECT_MOVED | `unit` (unit), `moved`, `outcome` |
| 342 | MOVEMENT_EFFECT_CHANGE_CELL | `unit` (unit), `pos` (12 bytes), `cell` (8 bytes), `mode`, `outcome` |
| 343 | MOVEMENT_EFFECT_MOVE_RATE | `unit` (unit), `rate`, `outcome` |
| 344 | MOVEMENT_EFFECT_SET_POSITION | `unit` (unit), `pos` (12 bytes), `mode`, `outcome` |
| 345 | MOVEMENT_EFFECT_SET_STATE_BITS | `unit` (unit), `mask`, `set`, `outcome` |
| 346 | MOVEMENT_EFFECT_SFX_OCCUPY | `unit` (unit), `outcome` |
| 347 | MOVEMENT_EFFECT_START_SCRIPT | `unit` (unit), `script`, `outcome` |

- `unit` is the unit the kind is about, a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for a pointer that is not in the unit table. `target` and `order` are a movement target and an order, as block offsets (handles).
- `state` of LOAD is the unit's motion state, 10 words (40 bytes), `pos` a position in world coordinates with 16 fractional bits (x, y and z), and `cell` a cell (x and y). `mode`, `speed`, `value`, `flags`, `delta`, `bank`, `pitch`, `heading`, `moved`, `rate`, `mask`, `set` and `script` are the arguments of their kinds, as the kind says. 320 to 331 but 325, which movement 4.0 removed (PLACED), are events into movement's inbox and 336 and up are effects movement publishes.
- `outcome` is the result of `Apply` for an event, and 1 for an effect.

### 8.34 units events (section 50, dumped only)

The events of units in this tick (`interfaces/units.h`). No row hashes them (section 6): a dump
holds their lines, after those of section 49, as a diagnostic. The reference build does not record them, so its
dumps have no `unitsEvents.*` lines. An event is recorded when units consumes it from its inbox, right after
`Apply` returns, with the outcome `Apply` gave. So the records are in the order units consumed them. The host empties the record after the world
row of the tick, so the lines a dump writes after an earlier phase hold the records made so far. Each record has the fields of its kind, the `fields` of its entry in `unitsEvents.kinds` of the schema, in the order of that list. The section is dumped, never hashed.

```
unitsEvents.count                                  # n
for k in 0 .. n-1, in order, by the kind of the record:
    unitsEvents[k].kind
    unitsEvents[k].<each field of that kind, in the order of its row below>
```

`unitsEvents.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. A field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 352 | UNITS_SET_STATE_BITS | `unit` (unit), `mask`, `set`, `outcome` |
| 353 | UNITS_SET_FLAGS | `unit` (unit), `clear`, `set`, `outcome` |
| 354 | UNITS_SET_NET_DIRTY | `unit` (unit), `clear`, `set`, `outcome` |
| 355 | UNITS_SET_WORK_TIME | `unit` (unit), `until`, `outcome` |
| 356 | UNITS_SET_POSITION | `unit` (unit), `pos` (12 bytes), `mode`, `outcome` |
| 357 | UNITS_ATTACH_TO_PIECE | `unit` (unit), `target` (unit), `piece`, `mode`, `outcome` |
| 358 | UNITS_FINISH_CONSTRUCTION | `unit` (unit), `builder` (unit), `outcome` |
| 359 | UNITS_BUILD_DECAY | `unit` (unit), `ticks`, `outcome` |
| 360 | UNITS_REPAIR | `unit` (unit), `builder` (unit), `amount`, `outcome` |
| 361 | UNITS_UNLINK | `unit` (unit), `ref` (handle), `outcome` |
| 362 | UNITS_SET_STANCE | `unit` (unit), `fields`, `moveOrder`, `fireOrder`, `group`, `outcome` |
| 363 | UNITS_SET_ANGLES | `unit` (unit), `fields`, `bank`, `heading`, `pitch`, `outcome` |
| 364 | UNITS_SET_BUILD_STATE | `unit` (unit), `buildLeft`, `health`, `outcome` |
| 365 | UNITS_TURN | `unit` (unit), `delta`, `outcome` |
| 366 | UNITS_MOVE | `unit` (unit), `pos` (12 bytes), `outcome` |
| 367 | UNITS_CHANGE_CELL | `unit` (unit), `pos` (12 bytes), `cell` (8 bytes), `mode`, `outcome` |
| 384 | UNITS_SET_MOVED | `unit` (unit), `moved`, `outcome` |
| 385 | UNITS_SFX_OCCUPY | `unit` (unit), `outcome` |
| 386 | UNITS_SCRIPT_VALUE | `unit` (unit), `which`, `value`, `outcome` |
| 387 | UNITS_NOTIFY_REFS | `unit` (unit), `event`, `outcome` |
| 388 | UNITS_ATTACKED_SPEECH | `unit` (unit), `outcome` |
| 389 | UNITS_MARK_DAMAGED | `unit` (unit), `outcome` |
| 390 | UNITS_DAMAGE_RECORD | `unit` (unit), `damageKind`, `attacker` (unit), `outcome` |
| 391 | UNITS_SET_HEALTH | `unit` (unit), `health`, `dying`, `outcome` |
| 392 | UNITS_KILL | `unit` (unit), `selfDestruct`, `outcome` |
| 393 | UNITS_SEND | `unit` (unit), `player`, `outcome` |
| 394 | UNITS_CAPTURE | `unit` (unit), `player`, `present`, `hitPoints`, `buildPercent`, `angles` (12 bytes), `stockpiles` (12 bytes), `outcome` |

- `unit` is the unit the kind is about, a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for a pointer that is not in the unit table; `target`, `builder` and `attacker` are other units in the same form. `ref` is a unit reference as a block offset (handle). `pos` is a position in world coordinates with 16 fractional bits (x, y and z), `cell` a cell (x and y), `angles` and `stockpiles` are 3 words each. The other fields are the arguments of their kinds, as the kind says. 352 to 367 and 384 to 394 are the events units consumes.
- `outcome` is the result of `Apply`.

### 8.35 scripts events (section 51, dumped only)

The events of scripts in this tick (`interfaces/scripts.h`). No row hashes them (section 6): a dump
holds their lines, after those of section 50, as a diagnostic. The reference build does not record them, so its
dumps have no `scriptsEvents.*` lines. An event is recorded when scripts consumes it from its inbox, right after
`Apply` returns, with the outcome `Apply` gave. So the records are in the order scripts consumed them. The host empties the record after the world
row of the tick, so the lines a dump writes after an earlier phase hold the records made so far. Each record has the fields of its kind, the `fields` of its entry in `scriptsEvents.kinds` of the schema, in the order of that list. The section is dumped, never hashed.

```
scriptsEvents.count                                  # n
for k in 0 .. n-1, in order, by the kind of the record:
    scriptsEvents[k].kind
    scriptsEvents[k].<each field of that kind, in the order of its row below>
```

`scriptsEvents.count` and the numbers of a record are written as `0x` and 8 hex digits. A reader reads `kind` first, then the fields of that kind and no others: the fields differ from kind to kind, and a kind has no line for a field of another kind's payload. A field is a number (a float is its 4 bytes as a u32), `(unit)` a pointer ID with target `unit` (is_ref=true, section 5), `(handle)` a block offset written `null` when 0 and `heap` otherwise, never the number, and `(N bytes)` the hex: of the N bytes.

| Kind | Name | Fields after `kind`, in dump order |
| --- | --- | --- |
| 416 | SCRIPTS_START | `unit` (unit), `form`, `index`, `name` (32 bytes), `callback` (handle), `update`, `count`, `args` (16 bytes), `outcome` |
| 417 | SCRIPTS_DETACH_PIECE | `unit` (unit), `piece`, `pose`, `outcome` |
| 418 | SCRIPTS_FREE_STATE | `unit` (unit), `state` (handle), `feature`, `outcome` |
| 419 | SCRIPTS_INIT_STATE | `unit` (unit), `state` (handle), `feature`, `outcome` |

- `unit` is the unit the kind is about, a pointer ID with target `unit` (section 5): `units:N`, `null`, or `heap` for a pointer that is not in the unit table. `callback` and `state` are a callback and a piece state, as block offsets (handles). The other fields are the arguments of their kinds, as the kind says. 416 to 419 are the events scripts consumes.
- `outcome` is the result of `Apply`.


### 8.36 orders' report (record 52)

Since format 8 orders writes record 52 through the `StateWriter` of its `WriteState`: what only orders reads of the order
lists. It is dumped and never hashed, with the prefix `ordersState.`, after `weaponsState.*`. For each live unit whose
`list` or `list2` (the refs at +0x5c and +0x60) is not null, one record `ordersState.units[i]`. For each chain `list`,
`list2` and each order n along it (at most 4096, the limit of section 8.9), the lines `list[n].<field>` or
`list2[n].<field>`:

| Field | Size | Kind |
| --- | --- | --- |
| `kind` | 1 | scalar (+4) |
| `state` | 1 | scalar (+5) |
| `flags6` | 4 | scalar (+6, the order's `flags`) |
| `wakeFrame` | 4 | scalar (+0xa) |
| `start.x` | 2 | scalar (+0x2e) |
| `start.y` | 2 | scalar (+0x30) |
| `arg0` | 4 | scalar (+0x36) |
| `arg1` | 4 | scalar (+0x3a) |
| `arg2` | 4 | scalar (+0x3e) |
| `created` | 4 | scalar (+0x46) |
| `subFlags` | 4 | scalar (+0x4e) |
| `attached` | 4 | pointer (+0x52): `null` or `heap` |

### 8.37 movement's report (record 53)

Since format 8 movement writes record 53, dumped and never hashed, with the prefix `movementState.`, after `ordersState.*`.
For each live unit with a motion (the ref at +0 of the unit), one record `movementState.units[i]`:

| Field | Size | Kind |
| --- | --- | --- |
| `motion.p2.x`, `motion.p2.y`, `motion.p2.z` | 4 | scalar (the motion at +0x14, +0x18, +0x1c) |
| `motion.turn` | 2 | scalar (the motion at +0x24) |
| `goal.class` | 4 | scalar: 0 none, 1 patrol, 2 AI search, 3 packed, 4 lite, 5 packed position, 6 any other kind (the numbers of section 8.10) |

then, by the goal's class, the lines `goal.<field>`:
- 1: the table `PatrolGoal`, then `goal.points`, a bytes field of min(count, 3) * 4 bytes at +0xc;
- 2: the table `AiSearchGoal` (flags masked 0x0b), then `goal.points`, min(count, 20) * 4 bytes at +0xc;
- 3 and 5: the table `PackedGoal`;
- 4: the table `PackedGoal`, then the table `LiteGoalState` (`netDirty` masked 0x07).

The targets of section 22 are not in this record: the dump holds them as `orderFx[id].*` (section 8.11).

### 8.38 units' report (record 54)

Since format 8 units writes record 54 in its `WriteState`, after the lines of section 25, dumped and never hashed, with the
prefix `unitsState.`. For each live unit, one record `unitsState.units[i]`:

| Field | Size | Kind |
| --- | --- | --- |
| `head` | 4 | pointer (+0xa2): `null` or `heap` |
| `hoverBobPhase` | 2 | scalar (+0xaa) |
| `healthPercent` | 1 | scalar (+0xf6) |
| `prevHealthPercent` | 1 | scalar (+0xf7) |
| `unfinishedInitZero` | 4 | scalar (+0x100) |
| `unknown_10a` | 4 | bytes (+0x10a), a `hex:` line |
| `zBufferFlag` | 4 | scalar (+0x114) |

### 8.39 scripts' report (record 55)

Since format 8 scripts writes record 55 in its `WriteState` (a slot since scripts' interface 2.1), dumped and never hashed,
with the prefix `scriptsState.`, after `playersState.*`. For each live unit with a script (the ref at +0x9a), one record
`scriptsState.units[i]`, with these lines in order:
1. The table `ScriptState` (`scale`, `table`, `checksum`, `changed`, `activeCount`), as the lines `<field>`.
2. For each channel c in 0 .. 7 at script + 0x1c + c * 0xa4, as in section 8.12: `channels[c].state` alone when its state is 0;
   else the table `ScriptChannel` (`state`, `pc`, `sp`, `signal`, `callback`) as `channels[c].<field>`, `channels[c].stack`
   (a bytes field of live * 4 bytes at +0x24, live = 0 when `sp` < -1, else sp + 1, at most 32) and, when
   `state & 0xff000000` is 0x02000000, by `state & 0x00f00000`: 0x00100000 or 0x00200000 the table `ScriptChannelPiece`,
   0x00400000 `ScriptChannelDelay`, 0x00800000 `ScriptChannelWait`.
3. `statics.count` (4 bytes), then `statics` (a bytes field of that count * 4 bytes), the count as in section 8.12 (0 when the
   table or the statics ref is null or the count is above 4096).
4. For each piece p in 0 .. pieceCount-1 (as in section 8.12): the table `ScriptPiece` at pieces + p * 0x4c, as `pieces[p].<field>`.

A scalar of a table is `0x` and twice its size in hex digits, a masked scalar is written masked, a bytes field is a `hex:` line
and a pointer is `null` or `heap`; a field with no name is `+0x<offset>`.

## 9. The per-tick digest

`reference/traces/<scenario>.txt` holds one digest per tick instead of the rows. For a tick, take its
rows in file order (`net` if present, the seven phases, then `world`, `tick` and `view`). Build a byte string from them:
for each row, the 8 bytes of the 64-bit row hash, **little-endian** (the least significant byte
first; note this is the reverse of the order of the digits in the file). The digest is the standard
64-bit FNV-1a over that byte string, one byte at a time:

```
h = 0xcbf29ce484222325
for each byte x:  h = (h xor x) * 0x100000001b3   (mod 2^64)
```
It is written as 16 lowercase hex digits. A test-mode tick has 10 rows, so the string is 80 bytes.

The **state** of a tick is its `tick` row (section 6), as the trace writes it: 16 lowercase hex digits. The
digest judges every row, so it enforces the order of the phases inside the tick; the state judges only the end
of the tick. The **view** of a tick is its `view` row (section 6), the same way: 16 lowercase hex digits. The digest covers it, and so a
difference of the view alone shows in the digest. `tools/reference.py --check` compares the digests, and `--check --end-of-tick` the
states, reporting the digests and the views beside them.

The digest file is:

```
bt-digest 10
trace_format bt-trace 10
commit <40 hex digits, or unknown>
scenario <path>
game_seed <n>
crt_seed <n>
view_seed <n>
fpu_control_word 0x<hex>
<tick> <digest> <state> <view>
```
with one `<tick> <digest> <state> <view>` line per tick, in tick order. The `quirks` line of a trace is not copied.

## 10. dump.txt

The reference writes `dump.txt` when it is run with `-testdump <tick>` (and `-testmode`). It holds
every hashed value of that one tick, after each phase of the tick, and the lines no row hashes: those
of sections 16 and 17, of the reports of economy, ai, weapons, orders, movement, units and scripts, and of sections 32, 33, 35, 36 to 42 and 47 to 51. The first line is `bt-dump N`, N the format number (section 12); every
other line is

```
<tick> <phase> <path> <value>
```

with single spaces between the first three fields and the value, which runs to the end of the line
(a `str:` value may itself contain spaces).

**Order.** For each phase of the tick that runs (`units` to `meteors`; `net` first if it runs) the
dump has, in order: the lines of the phase row (`gameTick`, the Park-Miller state of `rng`, `rng.crtHoldrand`,
then sections 1 to 9), then the lines of the world sections (10 to 14, 18 to 25 and 27, and 29 to 31 when the
tick is 1; sections 29 to 31 appear only at tick 1, in every phase block of that tick, after the
lines of section 27), then the lines of sections 32, 33, 37, 36, 38 to 42 and 47 to 51, in that order, which end the block and are
not hash input (sections 8.21, 8.22, 8.24, 8.25, 8.26, 8.27, 8.28, 8.29, 8.30, 8.31, 8.32, 8.33, 8.34 and 8.35). The `meteors` block ends with the lines of section 35 after them
(section 8.23), also not hash input. The `pathfinder.*` lines of
sections 16 and 17, and the `economyState.*`, `aiState.*`, `weaponsState.*`, `ordersState.*`, `movementState.*`, `mapState.*`, `playersState.*` and `scriptsState.*` lines of the own reports of economy, ai, weapons, orders, movement, map, players and scripts that follow them, sit between those of
sections 14 and 18 but are not hash input (sections 8.7, 8.22, 8.30, 8.31, 8.36, 8.37, 8.28, 8.17 and 8.39), the `losRadarState.*` lines sit before those of section 24 (section 8.6), and the `unitsState.*` lines sit after those of section 25 (section 8.38). Every
other line comes in exactly the order the hash consumes it, so reading the lines of one (phase, part)
block in order, skipping the lines that step 1 of "To recompute a hash from the dump" (below) names, at the places it names them, and
applying the rules of sections 6 to 8 reproduces its hash.

The `meteors` block is one sequence: the phase-row lines come first and the world-row lines follow.
The phase row ends with the last `meteors.<name>` line (section 9), and the world row begins with
the first `game.<name>` line (section 10). Both parts have `units[i].*` and `players[i].*` lines, so
tell them apart by that position: lines before the boundary are the phase row's, lines from it on
are the world row's. The
`world` row is a trace row only after `meteors`, and its lines in the `meteors` block are its hash
input. The world lines in the other phases' blocks are the world tables at that phase; they have no
row of their own.

**The `tick` block** (format 9) comes last, after the `meteors` block and its section 35 lines. It has the same
parts in the same order as a phase block (the phase-row lines, the world lines, then the lines that are not hash
input, without section 35), read at the end of the tick. Its phase-row lines and world lines, in order and with
no reset between them, are the hash input of the `tick` row.

**The view has no block.** The `view` row is not in the dump: its input is a file. A run with `-testdump <tick>` also writes
`views/<tick>.bin` beside `dump.txt` (`docs/view.md`, "The recorded view"), and the row of that tick is the hash of the file's bytes from
offset 8 (`tools/tracerehash.py` recomputes it, with `tools/viewblock.py`). A reader of the dump has nothing new to skip.

**What has no line.** Section numbers, the index words (`H32(i)`), presence bytes, `END` and the
`H8(which + 1)` and `H8(0)` markers of the order chains are hashed but not dumped. They follow from
the structure: the index is in the path; a presence byte is 1 exactly when the object's lines are
present; a section's bytes are `H8(section)` first and `END` last. A loop over a fixed range (the
players, the squad slots, the AI records, the timers) hashes its index and a presence byte of 0 for
every absent object, although such an object has no line. A loop over live units covers the units
with a `units[i].unitDefIndex` line in the block (every live unit has one).

**Values.**

| Syntax | Meaning |
| --- | --- |
| `0x` and `2*size` lowercase hex digits | A `scalar` field: its bytes, most significant first (the number written in hex). A masked field is written as the masked number. A counted value (`gameTick`, `rng.*`, a count, a class, a dirty word) is written the same way with 8 digits for a u32, 4 for a u16, 2 for a byte. |
| `hex:` and the bytes, 2 digits each, lowest address first | A `bytes` field, a raw run, a blob record. Can be empty (`hex:`). |
| `null` | A pointer with ID (0, 0). |
| `heap` | ID (1, 0). |
| `rec:overflow` | ID (9, 0xffffffff). |
| `<name>:<index>` | ID with the tag of the target name and the index in decimal. |
| `str:` and the text | A string (section 8.19). |
| `null` | A null string pointer. |

For a table, the paths are `<path>.<name>` for every field in the table's `fields`, in order, where
the name is empty the path is `<path>.+0x<offset>`. The dump prints `fields`, not `plan`, and does
not print excluded fields.

To recompute a hash from the dump:

1. Read the lines of the block in order. The phase-row lines, from `gameTick` to the last `meteors.<name>` line, are
   all read: none is skipped there, so a `features[i].*` line of section 7 is hash input although it shares its first
   name with the `features.*` events (section 8.25). Lines are skipped only in the world part, from the first `game.<name>` line on, and
   only at four places, a line being skipped when its path is one of the listed names followed by `.` (or, in the last
   list, by `.` or `[`, so `economy[0].kind` is skipped and `economyState.x` is not an `economy` line):
   - after the `pathfinder.*` lines of sections 16 and 17 (read for their shape, not hashed; section 8.7), the lines of
     `economyState`, `aiState`, `mapState`, `playersState`, `weaponsState`, `ordersState`, `movementState` and
     `scriptsState` (sections 8.22, 8.30, 8.28, 8.17, 8.31, 8.36, 8.37 and 8.39);
   - before the first line of section 24, the lines of `losRadarState` (section 8.6);
   - after the last line of section 25, the lines of `unitsState` (section 8.38);
   - after the last world section (27, or 31 at tick 1), every line to the end of the block whose path starts with
     `results`, `economy`, `features`, `packets`, `windMeteors`, `targeting`, `los_radar`, `map`, `playersEvents`,
     `aiEvents`, `weaponsEvents`, `ordersEvents`, `movementEvents`, `unitsEvents` or `scriptsEvents` (sections 8.21 to
     8.35); a line left after them is an error.

   `tools/tracerehash.py` reads the block this way. Parse the rest of the paths into the structure of sections 6 to
   8. The skipped `scriptsState.*` lines are not parsed into tables, but they still give section 23's presence byte:
   a unit's is 1 exactly when the block has its `scriptsState.units[i].scale` line (section 8.12).
2. For each table at a path, look the fields up in `data/trace_schema.json` by name (or `+0x<offset>`).
   Turn each value back into bytes: a scalar's hex number into `size` little-endian bytes; a `hex:`
   run into its bytes; a pointer into its ID.
3. Rebuild each plan entry: a pointer or masked entry is one field's value (a masked field's
   line already holds the masked number: `H32` it); a `bytes` entry is the concatenation of the bytes
   of the fields it covers, in offset order. Hash the entries with section 3.
4. Add the items that have no line (above): the section number, the indices, the presence bytes, the
   chain markers and `END`.

### What you need besides the lines

- A table is hashed only when its lines are present: a unit, order, motion, AI record,
  squad manager, squads, econ, mission conditions, spatial overflow, mission, weapon
  damage table and category mask each have a presence byte (or are in a loop that skips them), which is
  1 exactly when their lines are in the block. A `0` presence byte has no line. A script's presence byte is the
  exception: it comes from record 55's lines, not from lines of a hashed table (section 8.12).
- The conditional tables of sections 8.9, 8.11 and 8.10 (`OrderPosY`, `PathOrderPosY`,
  `PathOrderHeading`, `PathOrderPiece`, `PathOrderFollow`, `AirManeuverOrderHeading`,
  `LiteGoalState`) and `MissionStartRest` are hashed exactly when the table's fields are present
  at the same path.
- `SpotObject` against `SpotAnim` is told by the names of the fields at the path: the line after
  `features[i].next` is `anim.index` for a SpotAnim and `state` for a SpotObject (section 7.5).

## 11. Rules that decide hashed bytes

- **Zero fill.** New allocations are zero (the reference's test mode passes `-memset=0`), and
  memory grown by a reallocation is zero from the old size up to the new size. The reference never
  plays with uninitialised memory. A field that is never written hashes as zero. An engine
  must produce the same zeros, including in padding and unused tails that a plan entry covers.
- **Poison.** The reference can also run with every new allocation filled with 0xCD instead of zero
  (`--poison` in the tools). That run lists the bytes whose starting value reaches hashed state. Each
  is either left out of the schema (see `excluded` and the masks) or a real read of memory the
  original never initialised, which stays hashed and which the zero fill defines. A digest that
  differs only in a poison run is therefore expected for the second kind.
- **Skips.** A field the original never writes is left out through `excluded`, a `mask`, or one of
  the conditional tables of the sections above: the `pos.y` of `MobileBuild`, `VTOL_MobileBuild`,
  `Reclaim` and `VTOL_Reclaim` orders; the `pos.y` of a path order whose source order is
  `VTOL_MobileBuild` or `VTOL_Reclaim`; the `followOffset` of a path order unless
  `(flags AND 0x82) == 2`; bit 2 of the AI search goal flags (the mask `0x0b`); the start resource
  fields of players 3 to 10 of a mission that is not a campaign mission (`MissionStartRest`); and
  the bits the masks drop. The list of these, with the evidence, is in `docs/reference-traces.md`
  and `data/trace_coverage.csv`.
- **Map cells.** The cell blob is hashed after the edit of section 8.2, not as stored.
- **Clamps.** The limits (4096 orders in a chain, 32 stack words, 20 and 3 goal points, 512 script
  pieces, 4096 statics, 2^22 cells, and so on) are part of the format: a state beyond a limit hashes
  as if the count were as the section says.
- **Failures.** A mission condition of unknown class (0) fails the run in the reference, so its
  hash is never written (section 8.17). A dump that holds an order effect of a class above 5 fails
  too: the reference never writes one, so it has no hash (section 8.11). An order effect of
  class 0 is hashed, and a goal of class 6 is reported with no fields (section 8.10): neither fails.

## 12. Format numbers

`bt-trace N`, `bt-dump N` and `bt-digest N` carry the same number `N`, the format number, now 10.
Format 4 took sections 16 and 17 out of the world row (they are only dumped) and added section 32.
Format 5 took section 32 out of the world row and added section 33 (the economy events, section 8.22)
after it, both only dumped, and took the resource account of a unit (bytes 0xbc to 0xeb of a `Unit`) out
of the hash of the Unit table. Section 35 (the share packets, section 8.23) was added in format 5,
without a new number: it is only dumped, after every line a row hashes, and changes no row, no digest
and no line syntax. A reader that rejects unknown lines at the end of a block must learn to skip
`packets.*`, as it skips `results.*` and `economy.*`. Section 37 (wind_meteors' events, section 8.24) was
added the same way, after section 33's lines: a reader must also skip `windMeteors.*`.
Section 36 (the features events, section 8.25) was added in format 5 in the same way, after section 37's lines,
and a reader must skip `features.*` too. Section 38 (targeting's events, section 8.26) was added the
same way, after section 36's lines: a reader must also skip `targeting.*`. Section 39 (los_radar's events, section 8.27) was added
the same way, after section 38's lines: a reader must also skip `los_radar.*`. Section 40 (map's events, section 8.28) was added the same way, after
section 39's lines: a reader must also skip `map.*`. Section 41 (players' events, section 8.29) was added the same way, after section 40's lines: a reader must also skip `playersEvents.*`. Section 42 (ai's events, section 8.30) and record 43 (ai's report, `aiState.*`) were added the same way: a reader must also skip `aiEvents.*` and `aiState.*`. Section 47 (weapons' events, section 8.31) and record 48 (weapons' report, `weaponsState.*`) were added the same way, and a reader must also skip `weaponsEvents.*` and `weaponsState.*`. Sections 48 and 49 (orders' and movement's events, sections 8.32 and 8.33) were added the same way, and a reader must also skip `ordersEvents.*` and `movementEvents.*`. Sections 50 and 51 (units' and scripts' events, sections 8.34 and 8.35) were added the same way, and a reader must also skip `unitsEvents.*` and `scriptsEvents.*`. Sections 42, 47, 48, 49, 50 and 51 are new in the trace format.

Format 6 took the state that only its owner reads out of the hash (#979). Sections 15 (the eyes), 26 (the squads) and 28 (the mission conditions)
left the world row, and section 25 (the squad managers) and section 24 (the player AI) were replaced by the records that units and los_radar write
(sections 8.13 and 8.14: the factory index, the unit index and `hasSpecial`, in the new `Begin(24 or 25)` records of the `StateWriter`); the table Rec
hashes `b` only; the sight cache of a unit (`Unit::unusedLosCache`, `unusedLosSightFrame`), the clocks of a player (`Player::unusedUpdateTime`,
`unusedWinLoseTime`) and the end-game countdown (a game scalar) left the tables. What left is dumped, never hashed, in the
records 43 (ai, extended), 44 (los_radar), 45 (map) and 46 (players), with the prefixes `aiState.`, `losRadarState.`, `mapState.` and `playersState.`. A reader of
format 5 must read the new world row; a reader of the dump must skip the four prefixes. The digests of the stored scenarios change, and so does the tag.

Format 7 gave each kind of the event sections its own field list (sections 8.22 and 8.24 to 8.33; the schema's
`economy`, `features`, `windMeteors`, `targeting`, `los_radar`, `map`, `playersEvents`, `aiEvents`, `weaponsEvents`, `ordersEvents` and `movementEvents`
entries hold `kinds` instead of one `fields` list, and economy's separate `account` is a field of the LOAD kind). A
record of a section prints the fields of its kind only, and a reader must read the kind before the fields. The
payloads are the structs of the interface headers, so a kind's fields are the header's, then the payload's, then
`outcome`; the `unit`, `target`, `source` and `attacker` fields that name a unit are pointer IDs as before. No row
and no digest changes, and the line syntax is the same: a reader that skips the event prefixes changes only its
number.

Format 8 took the state that only its owner reads out of the hash for weapons, orders, movement, units and scripts (#1024).
Sections 3, 4 and 5 (the exploded pieces, the debris and the pieces of debris) left the phase rows. The tables `Unit` and
`Projectile` and the tables of sections 20 to 23 hash less (sections 7.1, 7.2 and 8.9 to 8.12): `UnitMotion::lastMoveTick` is
hashed again, section 21 hashes no goal, section 22 hashes only a path target's link (the table `PathOrderRef`) and section 23 hashes only the piece transforms. Record 48
grew, and four records are new: 52 `ordersState`, 53 `movementState`, 54 `unitsState` and 55 `scriptsState` (sections 8.31 and 8.36 to 8.39),
with the prefixes `weaponsState.`, `ordersState.`, `movementState.`, `unitsState.` and `scriptsState.`. A reader of format 7 must
read the new rows, and a reader of the dump must skip the four new prefixes. The digests of the stored scenarios change, and so does the tag.

Format 9 added the `tick` row (section 6), the ninth row of a test-mode tick, after `world`: the state at the end of the tick, read
after every step that follows the last phase (#1037). A dump has a `tick` block after the `meteors` block (section 10), and a digest
file line is `<tick> <digest> <state>` (section 9), the digest folding nine rows. No other row changes, but every digest does, and so
does the tag.

Format 10 added the `view` row (section 6), the tenth row of a test-mode tick, after `tick`: the hash of the player view of the end of the
tick (#1063, `docs/view.md`, "The recorded view"). A run writes `views/<tick>.bin` on the dump tick and on the ticks of a `frames` line
(section 10), and a digest file line is `<tick> <digest> <state> <view>` (section 9), the digest folding ten rows. No other row changes, but
every digest does, and so does the tag.

**A change needs a new format number** when a reader of this page would have to read it again, that is
when it changes any of:

- the header lines of `trace.txt`, or the syntax of a row, a dump line or a digest file;
- the phases, their order, or the rows of a tick, including when `net` and `world` appear;
- what a row covers: the sections, their order or numbers, a walk order, a limit or clamp, the
  presence bytes, the way an `END` or a count is hashed;
- the pointer ID rules: the tags, the numbering walks, the test of a pointer against its target array;
- the serialisation: the hash, the offset basis, the prime, the word step, the byte order, how a
  field kind is turned into bytes (including masks and the plan merge rule);
- the per-tick digest;
- the schema file's own format (the keys of `data/trace_schema.json` and what they mean).

**A change does not need a new number** when it only changes which fields the tables hold: a
field added to or removed from `data/trace_schema.json`, a new mask, a new exclusion. That changes the
digests, so it needs a new tag (below), but this page and the schema together still define the
format.

**Every new format number needs a new tag `reference-traces-v<n>`**, as does every other change to the
digests (a new or changed schema table, a new scenario, a corrected hash). A tag is never moved. The
tag number `n` counts tags; it is not the format number: `reference-traces-v1`, `-v2` and `-v3` are all
`bt-trace 3`. When the format number changes, the constant the tools write it from, this page, the stored
digests and the tag are changed together, and `docs/reference-traces.md` names the new tag.

## 13. Appendix: layout constants

Game record offsets and strides used by the walks above (offsets from the start of the game record;
"ref" and "plain" are the field kinds of section 1).

| What | Offset | Kind | Notes |
| --- | --- | --- | --- |
| number of players | 0x2a3c | u16 | in `game_scalars` |
| players | 0x1b63 | inline array | 11 records of 0x14b bytes |
| weapon definitions | 0x2cf3 | inline array | 256 records of 0x115 bytes |
| projectile count | 0x141f3 | u32 | |
| projectiles | 0x141f7 | ref | records of 0x6b bytes |
| spot pool | 0x1420b | ref | 0x800 entries of 0x30 bytes |
| spot used / rest list heads | 0x14213 / 0x14217 | i32 | |
| pathfinder | 0x14207 | ref | one record of 0xc9 bytes |
| map width / height | 0x14233 / 0x14237 | u32 | |
| feature count | 0x14253 | u32 | |
| feature definitions | 0x1426f | plain | records of 0x100 bytes |
| cells | 0x14287 | ref | 13 bytes a cell |
| visibility mask | 0x14273 | ref | |
| eye count / eyes | 0x14277 / 0x1427b | u32 / plain | 20 slots of 0x24 bytes |
| spatial grid | 0x1429f | ref | records of 10 bytes; width 0x142a3, height 0x142a7, count 0x142ab, overflow record (ref) 0x142b7 |
| unit pool count | 0x14351 | u16 | |
| units | 0x14357 | ref | records of 0x118 bytes |
| definition count / definitions | 0x1438f / 0x1439b | u32 / ref | records of 0x249 bytes |
| debris count / debris | 0x1491b / 0x1491f | u32 / inline array | 0x54 bytes each, at most 300 |
| pieces | 0x1ab9f | inline array | 300 records of 0x34 bytes |
| piece vertices | 0x1e88f | inline | 96 bytes a piece |
| piece faces | 0x2590f | inline | 6 records of 0x20 bytes a piece |
| game tick | 0x38a47 | u32 | |
| mission | 0x391e9 | ref | |
| mission conditions | 0x391ed | ref | |

Offsets used inside records, outside the schema (the schema holds the other fields):

| Record | Offset | Meaning |
| --- | --- | --- |
| Unit | 0x0 | ref to the motion record |
| Unit | 0x5c / 0x60 | `list` / `list2`, refs to the first order of each chain |
| Unit | 0x9a / 0x9e / 0xa2 | `script`, `state`, `head` refs |
| Unit | 0xa6 | `unitDefIndex`, u16; 0 means the slot is free |
| Projectile | 0x69 | u16 flags; bit 2 means the slot is free |
| Order | 0x4 / 0x12 / 0x4a / 0x52 | kind (u8), embedded link, `next` ref, `attached` ref |
| Order effect | 0x0 / 0x4 | class tag, `source` ref (order) |
| Path order | 0x8 / 0x16 | flags (u16), embedded link |
| Air maneuver order | 0x8 | flags (u16) |
| Goal | 0x4 | `target` ref (order effect) |
| Patrol goal | 0x18 / 0xc | point count, points (3 at most) |
| AI search goal | 0x5c / 0xc | point count, points (20 at most) |
| Script state | 0x8 / 0x10 / 0x14 / 0x1c / 0x540 | table address, statics, pieces, channels (8 of 0xa4 bytes), object state |
| Script table | 0x8 / 0x10 | piece count, static count |
| Script channel | 0x0 / 0x8 / 0x24 | state, `sp`, stack (32 words) |
| Object state | 0x22 | first piece transform (0x36 bytes each) |
| Player | 0x74 / 0x78 / 0x7c / 0x88 / 0xec | AI, squads, explored cells, explored size, resources |
| Pathfinder | 0x0 / 0x4 / 0x8 / 0xc / 0x14 / 0x1c / 0x28 / 0x2c / 0x58 | pool, items, free head, used, heap count, cells, cell count, dirty words, object |
| Node | 0x8 / 0x10 | `g`, penalty |
| Move class | 0x10 / 0x14 / 0x18 | width, height, pass map cells (ref) |
| Mission | 0x0, 0xdac to 0xdc0 | type, then units / rules / features with their counts |
| Unit definition | 0x14a / 0x14c / 0x14e / 0x152 / 0x156 / 0x231 / 0x23d | footprint x, z, yard map, id count, ids, weapon category masks (3), category mask |
| Weapon definition | 0x64 | damage table address |
| Mission conditions | 0x0 / 0x40 / 0x44 / 0x84 | victory refs, victory count, defeat refs, defeat count |

Simulation globals the walks read, outside the game record: the Park-Miller generator state and
the main CRT generator state (the two random words of the row preamble), the movement class table
(32 records of 0x20 bytes), the array of 100 exploded piece refs, the array of 10 AI record refs,
and the order kind table (kind number to name), plus the named values of `globals`.
