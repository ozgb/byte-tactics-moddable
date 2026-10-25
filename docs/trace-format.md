# Trace format

This page defines the three files a test-mode run writes: `trace.txt`, `dump.txt` and the stored
digest files. It is enough to compute every row of `trace.txt` from the game state, and to
recompute the same rows from a `dump.txt`. It is written in terms of the state block layout and
`data/trace_schema.json`, so an engine in any language can follow it without the reference's source.

The format is `bt-trace 3`. Running the reference and the gate are in `reference/README.md`.

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
  (the two random generator states, the order kind names, the meteor, pathfinder budget and economy
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
  `H32(x)` hashes a u32 as four bytes, `HB(bytes)` hashes a run of bytes, `HP(value, target, isRef)`
  hashes a pointer ID (section 5), `HT(table, address)` hashes a table of the schema at an address
  (section 4). `END` is `H32(0xffffffff)`.
- A **live unit** is a unit slot whose field `unitDefIndex` (the u16 at +0xa6 of the unit) is not 0.

## 2. trace.txt

Plain text, LF line endings. The header is seven lines, in this order:

```
bt-trace 3
commit <40 lowercase hex digits, or "unknown">
scenario <the path as given to the run>
game_seed <decimal>
crt_seed <decimal>
view_seed <decimal>
fpu_control_word 0x<5 uppercase hex digits>
```

When any quirk flag is on, one more line follows, `quirks <8 lowercase hex digits> <spec>`. With
every flag off the line is absent. Then one row per line:

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

A test-mode tick therefore has 8 rows. The rows are flushed at the end of each tick.

## 3. The hash

The hash is a 64-bit FNV-1a variant, over a stream of bytes the walk produces (sections 6 to 8). The
state is one 64-bit value `h`, mod 2^64.

- Offset basis: `h = 0xcbf29ce484222325`. It is set at the start of every row, and again before
  the `world` row. (The two halves are 0xcbf29ce4 high and 0x84222325 low.)
- Prime: `P = 0x100000001b3`.
- To hash a run of `n` bytes `b[0..n)`:
  - while at least 8 bytes remain: `h = (h xor w) * P`, where `w` is the next 8 bytes read as a
    little-endian u64; then skip them;
  - for each remaining byte `x` (fewer than 8): `h = (h xor x) * P`.
- The state carries over between runs; every run is chunked on its own. Two runs `A` then `B` are not
  the same as one run `A ++ B` unless `len(A)` is a multiple of 8.
- `H8(x)` is the run of one byte `x`. `H32(x)` is the run of the four bytes of `x`, least significant
  first, so it is four byte steps (never a word step). `HB(bytes)` is one run.
- `HP(value, target, isRef)` is `H8(tag)` then `H32(index)`, where `(tag, index)` is the pointer ID
  of section 5.
- `END` is `H32(0xffffffff)`, the end of a section.

A plain FNV-1a over bytes would differ: only the 8-byte words use the word step.

## 4. The schema file

`data/trace_schema.json` lists the tables. Top-level keys:

| Key | Meaning |
| --- | --- |
| `trace_format` | The format number, 3. |
| `targets` | The pointer target names with their numeric `tag` (used in section 5). |
| `tables` | Table name to table, sorted by name. |
| `game_scalars` | Fields of the game record hashed in the `game` section (section 8.1), in hash order. |
| `wind` | Fields of the game record hashed in the wind section, in hash order. |
| `globals` | Named simulation globals: `meteors`, `game` and `pathfinder`, each a list in hash order. |

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

**Masks.** The masks the schema holds are the ones in the field lists. A masked field is read as
a number, ANDed, and hashed as 4 bytes whatever its size. Example: the `flags` of an `Order`
(offset 0x42, `mask` 0xffdfffff) drops bit 0x200000, which only drawing code sets.

## 5. Pointer IDs

A pointer is never hashed as an address. It is hashed as an ID: `H8(tag)` then `H32(index)`.

```
resolve(value, target, isRef):
    if value == 0:                      return (0, 0)          # null
    if isRef: value = value + block base
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
section 3, 4, 5 explosions (7.3)
section 6 players          (7.4)
section 7 features         (7.5)
section 8 wind             (7.6)
section 9 meteors          (7.7)
```

**The world row** starts again from the offset basis and hashes sections 10 to 28 in order, then
sections 29 to 31 when the tick is 1. It has no tick or random preamble.

Each section begins with `H8(section number)` and ends with `END`, except where noted (the
`explosions` group is three sections, 3 to 5, each with its own `END`; the pathfinder is two
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

### 7.3 explosions (sections 3, 4 and 5)

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
    for k in 0 .. 5: HT(Face, game+0x2590f + (i*6+k)*0x20)    # faces
END
```
The piece a debris record "points at" is the u32 at its offset 0: piece `i` is remembered if that
value is the address `game+0x1ab9f + i*0x34` for some i below 300 (the address of the first byte of
the piece).
Dump: `explodedPieces[slot].<field>` and `explodedPieces[slot].rec.<field>`; `debris[d].<field>`;
`pieces[i].<field>`, `pieces[i].vertices` (a `hex:` line of 96 bytes) and `pieces[i].faces[k].<field>`.

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
Dump: `game.<name>`. Masked fields are dumped as masked.

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
for r in 0 .. count-1: HT(Rec, recs + r*10)  # dump: spatial[r].<field>
overflow = ref at game+0x142b7
H8(overflow != null)
if overflow != null: HT(Rec, overflow)       # dump: spatial.overflow.<field>
END
```

### 8.6 eyes (section 15)

```
H8(15)
eyes = the u32 (plain address) at game+0x1427b;  count = u32 at game+0x14277
if eyes is null or count > 20: count = 0
H32(count)                                   # dump: eyes.count
for e in 0 .. count-1: HT(Eye, eyes + e*0x24)    # dump: eyes[e].<field>
END
```

### 8.7 pathfinder (sections 16 and 17)

`pf` is the ref at `game+0x14207`.

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
    HP(u32 at items + slot*4, node, isRef=false)     # dump: pathfinder.heap[slot]
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
Dump values of these are `VisitValue` style: `0x` plus `2*size` hex digits.

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
        mask = (1 << (tail*2)) - 1
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
    HP(u.head, link, isRef=false)           # u.head: the ref at offset 0xa2, as an address; dump: units[i].head
    for which in (0, 1):                    # 0: u.list (offset 0x5c), 1: u.list2 (offset 0x60)
        o = chain head as an address (the ref plus the block base)
        H8(which + 1)
        n = 0
        while o != 0 and n < 4096:
            HT(Order, o)                    # dump: units[i].list[n].<field>, or list2[n]
            if not posYUnset(o): HT(OrderPosY, o)
            o = ref at o+0x4a;  n += 1
        H8(0)
END
```
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
    goal(prefix = "units[i].motion.goal", g = ref at m+0)
END
```

`goal(g)`, with the goal's class by the kind of object it is (none 0, patrol 1, AI search 2, packed 3,
lite 4, packed position 5):

```
if g == null:  H32(0)                       # dump: <prefix>.class
else:
    H32(class)                              # dump: <prefix>.class; an object of no known class fails the run
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
    H32(id)
    H32(class of f)                         # dump: orderFx[id].class
    if the class is unknown (0):
        H32(a number identifying the object's type)   # dump: orderFx[id].vtableDelta; never seen in a conforming run
        continue
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
END
```
`fxPosYSet(f)`: the source order is the ref at `f+4`; the `PathOrderPosY` table is hashed unless that
order exists and its kind name is `VTOL_MobileBuild` or `VTOL_Reclaim`.
Dump: `orderFx[id].<field>`.

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
    HT(ScriptState, s)                                  # dump: units[i].script.<field>
    table = u32 at s+8 (a plain address);  statics = ref at s+0x10;  pieces = ref at s+0x14
    objectState = ref at s+0x540
    staticCount = pieceCount = 0
    if table != 0: staticCount = u32 at table+0x10;  pieceCount = u32 at table+8
    if staticCount > 4096 or statics == 0: staticCount = 0
    if pieceCount > 512: pieceCount = 0
    for c in 0 .. 7:  channel(s + 0x1c + c*0xa4)        # dump prefix units[i].script.channels[c]
    H32(staticCount)                                    # dump: units[i].script.statics
    HB(staticCount*4 bytes at statics)                  # dump: units[i].script.statics (hex:)
    if pieces == 0: pieceCount = 0
    for p in 0 .. pieceCount-1: HT(ScriptPiece, pieces + p*0x4c)       # dump: units[i].script.pieces[p].<field>
    if objectState == 0: pieceCount = 0
    for q in 0 .. pieceCount-1: HT(PieceTransform, objectState + 0x22 + q*0x36)   # dump: units[i].script.transforms[q].<field>

channel(ch):
    state = u32 at ch+0
    if state == 0:  H32(0)                              # dump: <prefix>.state
        return
    HT(ScriptChannel, ch)                               # dump: <prefix>.<field>
    sp = i32 at ch+8
    live = (sp < -1) ? 0 : sp + 1;  live = min(live, 32)
    HB(live*4 bytes at ch+0x24)                         # dump: <prefix>.stack (hex:)
    if (state AND 0xff000000) == 0x02000000:
        switch (state AND 0xf00000):
            0x100000, 0x200000:  HT(ScriptChannelPiece, ch)
            0x400000:            HT(ScriptChannelDelay, ch)
            0x800000:            HT(ScriptChannelWait, ch)
```
A channel with `state == 0` has exactly one line, `<prefix>.state` with the value `0x00000000`, and
no `.stack` line. A channel with a state other than 0 has the lines of its table (the first is
`<prefix>.state`, with a non-zero value), then `<prefix>.stack`, then any lines of the conditional
table. So a `.state` line of value 0 means a one-line channel.

The `statics` path has two lines in a row with the same path: the count line (`0x` and 8 digits),
then the `hex:` line of the bytes (`hex:` alone when the count is 0).

The stack holds the words up to and including `sp`: the channel's stack pointer `sp` is the index of
the top word, so `sp + 1` words are live.

### 8.13 player AI (section 24)

```
H8(24)
for i in 0 .. 9:
    ai = the AI record of player i       # 10 refs held in a simulation global
    H32(i);  H8(ai != null)
    if ai == null: continue
    HT(PlayerAI, ai)                                    # dump: players[i].ai.<field>
    unitVector("players[i].ai.visible",   ai + 0x05)
    unitVector("players[i].ai.known",     ai + 0x15)
    unitVector("players[i].ai.factories", ai + 0x25)
    rawVector(name, ai + offset, elementSize)  for these, in this order:
        cells 0x4d 8;  vec_65 0x65 3;  counts 0x7d 2;  vec_8d 0x8d 1;  weights 0x9d 1;
        vec_ad 0xad 1;  vec_bd 0xbd 4;  values 0xcd 4;  locked 0xdd 4
END
```
A **vector** at address V has a begin address (the u32 at V+4) and an end address (the u32 at V+8).
Its element count is `(end - begin) / elementSize` when `begin != 0`, `end >= begin`,
`(end - begin)` is a multiple of `elementSize` and the count is at most 2^20; otherwise 0.

```
unitVector(path, V):   n = count with element size 4
                       H32(n)                       # dump: <path>.size
                       for k in 0 .. n-1: HP(u32 at begin + 4k, unit, isRef=false)     # dump: <path>[k]
rawVector(path, V, e): n = count with element size e
                       H32(n)                       # dump: <path>
                       HB(n*e bytes at begin)       # dump: <path> (hex:)
```
The dump paths of `rawVector` are `players[i].ai.<name>`. Each `rawVector` has two lines with the
same path, in order: the count line (`0x` and 8 digits), then the `hex:` line of the bytes (`hex:`
alone when the count is 0). The `statics` path of section 8.12 is the same.

### 8.14 squad managers (section 25)

```
H8(25)
for i in 0 .. 10:
    mgr = ref at (game+0x1b63 + i*0x14b) + 0x74
    H32(i);  H8(mgr != null)
    if mgr == null: continue
    HT(SquadManager, mgr)                               # dump: players[i].squadManager.<field>
    for t in 0 .. 9:
        timer = ref at mgr + 0x11 + t*4
        H8(t);  H8(timer != null)
        if timer == null: continue
        class = timerClass[t]       # t=0:0 1:7 2:2 3:3 4:6 5:1 6:2 7:3 8:4 9:5
        H32(class)                                      # dump: players[i].squadManager.timers[t].class
        class 2 (assault): HT(AssaultTimer, timer); class 3 (escort): HT(EscortTimer, timer)
        class 5 (spatial): HT(SpatialTimer, timer);  any other class: HT(SquadTimer, timer)
END
```
The timer classes are: unknown 0, squad 1, assault 2, escort 3, squad scout 4, spatial 5, scout 6,
build 7. The class follows from the slot `t`.

### 8.15 squads (section 26)

```
H8(26)
for i in 0 .. 10:
    sq = ref at (player i) + 0x78
    H32(i);  H8(sq != null)
    if sq == null: continue
    for s in 0 .. 9:
        squad = sq + s*0x20
        HT(Squad, squad)                                # dump: players[i].squads[s].<field>
        unitVector("players[i].squads[s].items", squad + 0x10)
END
```

### 8.16 unit resources (section 27)

```
H8(27)
for i in 0 .. 10:
    econ = ref at (player i) + 0xec
    H32(i);  H8(econ != null)
    if econ != null: HT(UnitResources, econ)            # dump: players[i].econ.<field>
END
```

### 8.17 mission conditions (section 28)

```
H8(28)
cs = ref at game+0x391ed
H8(cs != null)
if cs != null:
    HT(MissionConditions, cs)                           # dump: conditions.<field>
    for list in (0, 1):                                 # 0: victory, 1: defeat
        count = u32 at cs + (list == 0 ? 0x40 : 0x84);  base = (list == 0 ? 0x00 : 0x44)
        if count > 16: count = 0
        H8(list + 1)
        for k in 0 .. count-1:
            c = ref at cs + base + k*4
            H8(c != null)
            if c == null: continue
            H32(class of c)                             # dump: conditions.victory[k].class (or defeat)
            HT(MissionCondition, c)
            if class == 16: HT(ConditionNumUnits, c)
            elif class == 12 or class == 4: HT(ConditionNumLeft, c)
END
```
The counts are the dumped fields `conditions.victoryCount` (offset 0x40) and
`conditions.defeatCount` (offset 0x84): read them from the lines of the table itself (a count above
16 counts as 0 for the loop). A null entry has no line, so its index `k` is simply missing from the
dump; the entries that are present are those with a `conditions.victory[k].class` (or `defeat`)
line.

Condition classes: 0 unknown (fails the run), defeat: passes z 1, passes x 2, death timer 3,
type killed 4, all of type 5, all killed 6, commander 7; victory: timer 8, passes z 9, passes x 10,
move to radius 11, kill type 12, kill all of type 13, capture 14, build 15, kill mobile 16, destroy
all 17, kill commander 18.

### 8.18 unit definitions (section 29, tick 1 only)

```
H8(29)
defs = ref at game+0x1439b;  count = u32 at game+0x1438f
if defs is null or count > 2^14: count = 0
H32(count)                                              # dump: defs.count
for i in 0 .. count-1:
    d = defs + i*0x249
    yardmap = ref at d+0x14e;  ids = ref at d+0x156;  idCount = u32 at d+0x152
    yardSize = (u32)(i16 at d+0x14a) * (u32)(i16 at d+0x14c)     # mod 2^32
    HT(UnitDefData, d)                                  # dump: defs[i].<field>
    if yardmap == null or yardSize > 65536: yardSize = 0
    H32(yardSize);  HB(yardSize bytes at yardmap)       # dump: defs[i].yardmap (count), defs[i].yardmap (hex:)
    if ids == null or idCount > 30: idCount = 0
    H32(idCount);   HB(idCount*2 bytes at ids)          # dump: defs[i].ids (count), defs[i].ids (hex:)
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
`H32` value written as 8 hex digits (`yardSize`, `idCount`, after the clamps above), then a `hex:`
line with the bytes that follow it in the hash (an empty `hex:` when the count is 0). The count
line is always present, so a reader reads the count from it and need not recompute it from the
definition's fields. The bytes are hashed as read, not written as hex numbers: `ids` is `idCount`
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

## 9. The per-tick digest

`reference/traces/<scenario>.txt` holds one digest per tick instead of the rows. For a tick, take its
rows in file order (`net` if present, the seven phases, then `world`). Build a byte string from them:
for each row, the 8 bytes of the 64-bit row hash, **little-endian** (the least significant byte
first; note this is the reverse of the order of the digits in the file). The digest is the standard
64-bit FNV-1a over that byte string, one byte at a time:

```
h = 0xcbf29ce484222325
for each byte x:  h = (h xor x) * 0x100000001b3   (mod 2^64)
```
It is written as 16 lowercase hex digits. A test-mode tick has 8 rows, so the string is 64 bytes.

The digest file is:

```
bt-digest 3
trace_format bt-trace 3
commit <40 hex digits, or unknown>
scenario <path>
game_seed <n>
crt_seed <n>
view_seed <n>
fpu_control_word 0x<hex>
<tick> <digest>
```
with one `<tick> <digest>` line per tick, in tick order. The `quirks` line of a trace is not copied.

## 10. dump.txt

The reference writes `dump.txt` when it is run with `-testdump <tick>` (and `-testmode`). It holds
every hashed value of that one tick, after each phase of the tick. The first line is `bt-dump 3`; every
other line is

```
<tick> <phase> <path> <value>
```

with single spaces between the first three fields and the value, which runs to the end of the line
(a `str:` value may itself contain spaces).

**Order.** For each phase of the tick that runs (`units` to `meteors`; `net` first if it runs) the
dump has, in order: the lines of the phase row (`gameTick`, `rng.parkMiller`, `rng.crtHoldrand`,
then sections 1 to 9), then the lines of the world sections (10 to 28, and 29 to 31 when the
tick is 1; sections 29 to 31 appear only at tick 1, in every phase block of that tick, after the
lines of section 28; in the `meteors` block they end the block). The lines come in exactly the order the hash consumes them, so reading the lines of one
(phase, part) block in order and applying the rules of sections 6 to 8 reproduces its hash.

The `meteors` block is one sequence: the phase-row lines come first and the world-row lines follow.
The phase row ends with the last `meteors.<name>` line (section 9), and the world row begins with
the first `game.<name>` line (section 10). Both parts have `units[i].*` and `players[i].*` lines, so
tell them apart by that position: lines before the boundary are the phase row's, lines from it on
are the world row's. The
`world` row is a trace row only after `meteors`, and its lines in the `meteors` block are its hash
input. The world lines in the other phases' blocks are the world tables at that phase; they have no
row of their own.

**What has no line.** Section numbers, the index words (`H32(i)`), presence bytes, `END` and the
`H8(which + 1)` and `H8(0)` markers of the order chains are hashed but not dumped. They follow from
the structure: the index is in the path; a presence byte is 1 exactly when the object's lines are
present; a section's bytes are `H8(section)` first and `END` last. A loop over a fixed range (the
players, the squad slots, the AI records, the timers) hashes its index and a presence byte of 0 for
every absent object, although such an object has no line. A loop over live units covers the units
with a `units[i].head` line in the block (every live unit has one).

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

1. Read the lines of the block, in order, and parse the paths into the structure of sections 6 to 8.
2. For each table at a path, look the fields up in `data/trace_schema.json` by name (or `+0x<offset>`).
   Turn each value back into bytes: a scalar's hex number into `size` little-endian bytes; a `hex:`
   run into its bytes; a pointer into its ID.
3. Rebuild each plan entry: a pointer or masked entry is one field's value (a masked field's
   line already holds the masked number: `H32` it); a `bytes` entry is the concatenation of the bytes
   of the fields it covers, in offset order. Hash the entries with section 3.
4. Add the items that have no line (above): the section number, the indices, the presence bytes, the
   chain markers and `END`.

### What you need besides the lines

- A table is hashed only when its lines are present: a unit, order, motion, script, AI record,
  squad manager, squads, econ, mission conditions, pathfinder, spatial overflow, mission, weapon
  damage table and category mask each have a presence byte (or are in a loop that skips them), which is
  1 exactly when their lines are in the block. A `0` presence byte has no line.
- The conditional tables of sections 8.9, 8.11 and 8.10 (`OrderPosY`, `PathOrderPosY`,
  `PathOrderHeading`, `PathOrderPiece`, `PathOrderFollow`, `AirManeuverOrderHeading`,
  `LiteGoalState`) and of 8.12 (`ScriptChannelPiece`, `ScriptChannelDelay`, `ScriptChannelWait`),
  and `MissionStartRest`, `PathfinderSearch`, are hashed exactly when the table's fields are present
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
- **Failures.** An object of unknown class (a goal, order effect or mission condition) fails the
  run in the reference, so its hash is never written.

## 12. Format numbers

`bt-trace N`, `bt-dump N` and `bt-digest N` carry the same number `N`, the format number, now 3.

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
