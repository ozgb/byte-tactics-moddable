# The files gate

The files gate judges a data loader by its behaviour alone: how it mounts the archives of a game directory, which file a
name resolves to, what a listing holds and in what order, the bytes and return values of its reads, what it does with a
malformed archive, and how it reads a TDF file, a COB script, a GAF image and a 3DO model. The loader is one program; the gate runs it on fixture directories that
the kit's fixture tool builds, and compares what it writes with the vectors of `reference/files/`.

The vectors were made by the in-repo engine through the files service of `interfaces/files.h`, on the same fixtures.
Neither the fixtures nor the vectors hold any byte of the retail game.

## The fixtures

```sh
python3 tools/filesfixtures.py build DIR     # DIR/base, DIR/budget and DIR/malformed; DIR must be empty or new
python3 tools/filesfixtures.py check DIR     # compare a build with reference/files/fixtures.json
```

`build` refuses a fixture directory that already holds anything, and `check` reports every file that is missing, differs
or is not in `fixtures.json`.

`tools/filesfixtures.py` needs Python 3.10 and its standard library, and gives the same bytes on every machine: the
digests of every file are in `reference/files/fixtures.json`, and a vector is only meaningful on fixtures that match them.

| Fixture | What it holds |
| --- | --- |
| `base` | loose files and directories; `rev31.gp3`, two `.ccx`, a `.ufo` and four `.hpi` archives (one named in capitals, and `ex.hpi` and `e_x.hpi`, whose order depends on the case folding below), each with another key; two loose names, `arm_x.fbi` and `armx.fbi`, likewise; `rev32.gp3`, a valid archive the game does not mount; `x.hpi`, a file that is not an archive. The archives hold the same paths as each other and as the disk, names stored twice in one directory, a directory stored twice, a directory and a file of one name, a name with a `/` in it, and files stored plainly and in LZ77 and deflate chunks, with and without chunk encryption, across block boundaries and of exactly two blocks; and loose COB, GAF and 3DO files under `scripts`, `anims` and `objects3d` (below) |
| `budget` | twelve valid `.hpi` archives and one that is not an archive, between the fifth and the sixth |
| `malformed` | archives with a bad magic, a bad version, no copyright footer, a footer in lower case, a footer with another year and one with letters for the year, and a good archive holding files whose chunks are broken: a bad checksum, a bad chunk magic, an unknown method, a wrong unpacked size, and a chunk table that runs past the end of the archive; and, for each of COB, GAF and 3DO, a truncated file and one with a count that runs past its end ("Malformed COB, GAF and 3DO files") |

The archive format is the one the fixture tool writes: a 20-byte header (`HAPI`, the version bytes `00 00 01 00`, the
size of the header and directory, a key byte and the offset of the root directory), the directory, the file data, and
the footer `Copyright NNNN Cavedog Entertainment`. A chunked file is a table of chunk sizes followed by `SQSH` chunks of
at most 65536 unpacked bytes each. The tool's `archive` and `chunk` functions are the definition.

## Running a loader

```sh
LOADER files --game DIR CALLS OUT
```

`DIR` is the fixture directory, and the loader treats it as the game's directory (the directory it mounts from and the
current directory every relative name is read against). `CALLS` is a text file of calls, one per line, and `OUT` is the
file the loader writes, one line for each call in order. The loader exits with status 0 when it has answered every call
or stopped at a fatal error (below). Each run starts with nothing mounted and no file open.

A call is its name and its arguments, separated by tab characters; an argument may be empty. A line of `OUT` is the call's
name, a tab, and its value as JSON. A JSON string holds the bytes of a name or a value: a byte outside 0x20 to 0x7e is
written as `\u00XX`, and the gate compares parsed values, so spacing within the JSON is free. The types must match:
an integer is written as one (`1`, not `1.0` or `true`), and `null` is not `0`.

| Call | Arguments | Value |
| --- | --- | --- |
| `mount` | none | the game's mount sequence (below); the file names of every mounted archive, in search order, as a list of strings |
| `open` | name | the next handle number (1, 2, and so on, counting only successful opens) or `null` |
| `close` | handle | the files service's `Close` return value |
| `inarchive` | handle | `IsInArchive`: 1 for a file read from an archive, 0 for a loose one |
| `seek` | handle, position | `Seek`'s return value (position -1 means the end) |
| `tell` | handle | `Tell` |
| `length` | handle | `Length` |
| `read` | handle, size | `[n, sha]`: `Read`'s return value and the SHA-256 (lower-case hex) of the first `n` bytes read, or of no bytes when `n` is 0 or less |
| `lengthbyname` | name | `LengthByName` |
| `readat` | name, position, size | `ReadAt` into a buffer of `size` zero bytes: the SHA-256 of all `size` bytes of the buffer afterwards, or `null` when it returns no buffer |
| `load` | name | `Load`: `[size, sha]` with the size it stores and the SHA-256 of the data, or `[size, null]` when it returns no data (`size` is then what it stored: -1) |
| `find` | path, state, recursive | `FindFirst` then `FindNext` until it fails, then `FindClose`: a list of `[name, attributes, size]`, where attributes keeps only the read-only (0x01) and directory (0x10) bits |
| `count` | path, flag | `CountDirectoryEntries` |
| `list` | path, directories only | `ListDirectory`: the list of names |
| `tree` | path, pattern, state, recursive | `FindFilesRecursive`: the list of paths |
| `match` | string, pattern | `MatchWildcard`: 1 or 0 |
| `tdf` | name | the TDF reading of the file (below), or `null` when the file is missing or empty |
| `cob` | name | the COB reading of the file (below), or `null` when the file is missing or empty |
| `gaf` | name | the GAF reading of the file (below), or `null` when the file is missing or empty |
| `3do` | name | the 3DO reading of the file (below), or `null` when the file is missing or empty |

The names in this table are the slots of `interfaces/files.h`, which says what each takes and returns. A loader with
another interface maps each call onto its own; only the values are judged.

**The order of a disk listing.** A listing of a directory on the disk (the `find`, `count`, `list` and `tree` entries
that come from the disk, and the mount sequence below) is sorted by name: the ASCII letters `a` to `z` are folded to upper case and
the bytes compared as unsigned values, directories and files alike (`.` and `..` included). Folding to upper case puts `_` (0x5F) and
`[\]^` after the letters, so `armx.fbi` comes before `arm_x.fbi`; folding to lower case would put them the other way.
The disk's entries come first, then each archive's in search order, each in the order the archive stores them.

**The mount sequence** is what the game does once at start: it mounts `rev31.gp3`, then every `*.ccx`, then every
`*.ufo`, then the `*.hpi` archives, each group in the order of a listing of the game directory, and then marks the
archive entries that the disk or an earlier archive hides from listings. An archive whose header or footer is refused is
skipped, and so is one already mounted. A `mount` after the first is the same sequence again, on what is already
mounted. The vectors of the `mounting` group are its definition, the order and the skipping included.

**A fatal error.** Where 3.1c stops the game with an error message (a chunk that can't be unpacked, a TDF file it can't
parse), the loader writes the line `fatal<TAB>null` for that call, answers no further call, and exits with status 0.

**The TDF reading** is an object `{"name", "fields", "records"}` for the file (named `root`) and, inside it, each
section in the order of the file. `fields` lists the fields of a section in the order it keeps them, each as
`[name, stored value, looked-up value, integer, fixed, double]`: the value stored with that entry, the value a lookup of
its name returns, the lookup's value read as a C `atoi` integer, as a 16.16 fixed point number (the `atof` value times
65536, converted to an integer as MSVC 5's `_ftol` does: the low 32 bits of the 64-bit truncation toward zero, so `40000`
gives -1673527296), and as the `atof` double of MSVC 5's C library (its 64 bits in hex, most significant first). The FBI
unit files are TDF files and are read the same way.

The fields are kept sorted by name without regard to case. A field is placed by a binary search for the first kept field
whose name is not less than its own, without regard to case. When that field's name is the same, case included, the new
value replaces its value; otherwise the new field is inserted before it. So a name that differs only in case from earlier
ones goes before all of them (`Name`, then `name`, then `Name` again keeps three fields, the last one first), and a
lookup returns the first of them.

## COB, GAF and 3DO

The game loads each of these files whole through the `Load` call above and reads it in place: every offset below is
from the start of the file, and the game reads only what the reading names. A word is a little-endian 32-bit
signed integer, and a name is the bytes from its offset up to a NUL. The fixture tool's `cob`, `gaf` and `model`
functions write each layout, and are its definition as much as this text.

**COB.** The header is eleven words: `version`, the script count, the piece count, the code length (in words), the
static variable count, the pair count, and the offsets of the script start table, the script name table, the piece
name table, the code and the pair table. The script start table holds a word for each script (the index of its first
code word), the two name tables a name offset for each script and each piece, and the pair table two words for each
pair. The reading is an object:

| Field | Value |
| --- | --- |
| `version` | the first header word |
| `scripts` | `[name, start]` for each script, in the order of the tables |
| `pieces` | the piece names, in order |
| `statics` | the static variable count |
| `pairs` | `[first word, second word]` for each pair: the second word is an offset, which the game turns into an address but never reads through |
| `code` | the code length's words from the code offset |

The game doesn't read the code length; the reading uses it to bound the code. Every count is signed: a negative one
gives an empty list (and a negative static count is reported as it is).

**GAF.** The file starts with three words: a version, the entry count and a zero, then an offset for each entry. The
entry count is read as its low 16 bits, as a signed number: a count word of 0x00020001 holds one entry, and one of
0x0000FFFF none. An entry is a 16-bit frame count, a byte `loop` (offset 2), a byte and a word the game doesn't read,
a 32-byte name (offset 8, NUL-terminated), and then, at offset 40, two words for each frame: the frame's offset and
its duration. A frame is 24 bytes: width and height (16 bits, unsigned), x and y (16 bits, signed), the transparent
index, `compressed`, the subframe count and `blend` (a byte each), a word the game doesn't read, the offset of the
frame's data and a word it doesn't read. The frame and subframe counts are unsigned. The reading is the list of entries, each `{"name", "loop", "frames"}`, and
a frame is:

| Field | Value |
| --- | --- |
| `width`, `height`, `x`, `y`, `transparent`, `compressed`, `blend` | the frame's fields |
| `duration` | the low 16 bits of the frame's duration word (a top-level frame only) |
| `frames` | when the subframe count is not 0: the subframes, each a frame without `duration`; the data is a word for each, the subframe's offset |
| `pixels` | otherwise: the rows, top first, each a list of `width` palette indices, `null` for a pixel the row leaves transparent |

A frame is compressed when `compressed` is not 0 (any value, not only 1); a frame with subframes is read the same
whatever its flag. A frame whose `compressed` is 0 holds `width` × `height` bytes, row by row, and every byte is a pixel (the transparent
index is a pixel value like any other here; drawing is what skips it). A compressed frame holds, for each row, a
16-bit byte count and then that many bytes of commands, each a byte `b`: when bit 0 is set, `b >> 1` pixels are
transparent; when bit 1 is set, the next byte is repeated `(b >> 2) + 1` times; otherwise the next `(b >> 2) + 1`
bytes are copied. A copied or repeated byte is a pixel even when it equals the transparent index. A row's pixels start
transparent, and it decodes until it has `width` pixels: a run that goes past the end of the row is cut there (the
rest of a cut literal is not read), and a row whose byte count is 0 stays transparent. The byte count is used only to
find the next row: bytes after the command that fills the row are skipped, and a row whose commands give fewer than
`width` pixels goes on reading commands from the bytes that follow it in the file (the next row's byte count first)
until it has them.

**3DO.** A piece is thirteen words: a version, the vertex count, the primitive count, the selection primitive (-1 for
none), the offset from its parent (x, y, z), the offset of its name (0 for none), a word the game makes an address when it is not 0 but doesn't
read through, the offsets of its vertices and of its primitives, and the offsets of its next sibling and its first child (0 for none).
The first piece is at offset 0. A vertex is three words (x, y, z), and a primitive is eight words: its colour, its
vertex count, a word the game makes an address when it is not 0 but doesn't read through, the offset of its vertex
indices (16 bits each, unsigned), the offset of
its texture name (0 for none) and three words the game doesn't read. The reading is the list of the first piece and
its siblings, in order, each (a negative vertex or primitive count gives an empty list):

| Field | Value |
| --- | --- |
| `name` | the name, or `null` |
| `offset` | `[x, y, z]` |
| `vertices` | `[x, y, z]` for each vertex |
| `selection` | the selection primitive after the reordering below |
| `primitives` | `{"colour", "vertices", "texture"}` for each primitive, after the reordering below; `texture` is `null` when there is none |
| `children` | the list of the first child and its siblings, read the same way |

The game reorders each piece's primitives as it loads them. When the selection primitive is not -1 and the piece has
primitives, the selection primitive and the first change places and the selection becomes 0. Then the primitives
from the second on are bubble-sorted by height: passes compare each primitive from the second to the one before
last with the next, and swap them when the first is higher, until a pass swaps nothing. A primitive's height is the
sum of its vertices' y, divided by its vertex count as C divides integers (truncating toward zero), so -0.5 is as high
as 0. The first primitive never moves in the sort.

**Malformed COB, GAF and 3DO files.** The game checks nothing in these files, so the `malformed` fixture's six files
are not called: in each the game reads outside the file's data, which gives no value to compare.

| File | What the game does |
| --- | --- |
| `scripts\trunc.cob` | its tables are past the end, and the game reads and rewrites them there |
| `scripts\over.cob` | a script count of 100000: the game reads and rewrites the name table past the end |
| `anims\trunc.gaf` | the file ends inside the first entry's name: that entry's frame table and the other two entries are past the end, and the game reads them there |
| `anims\over.gaf` | a frame count of 30000: the game reads frame offsets past the end |
| `objects3d\trunc.3do` | the vertices, primitives and children are past the end |
| `objects3d\over.3do` | a primitive count of 100000: the game reads and sorts primitives past the end |

Nor are these called: a short compressed row (above) that reads past the end of the file, a GAF subframe with subframes of its own (the game doesn't make their offsets addresses), a 3DO
primitive with no vertices when the sort reaches it (the game divides by zero), a selection primitive past the
primitive count, and an index past a piece's vertices. The game makes each offset an address in place, by adding the
address of the file's data to the word, so a word it reaches twice is rewritten twice and no longer an offset. So
these are not called either: a COB table or pair table that overlaps another, a GAF frame that two entries or two
frames name, a 3DO piece that two pieces name as sibling or child, and a 3DO piece or primitive whose address-made
word (word 8 of a piece, word 2 of a primitive) is also a word the game rewrites through another offset.

## The groups

| Group | What it covers |
| --- | --- |
| `mounting` | which archives are mounted, in what order, which are skipped, the limit on the `.hpi` archives, and a second `mount` |
| `lookup` | the file a name resolves to: the disk before the archives and an earlier archive before a later one, case, both separators, names stored twice, missing names and directories |
| `listing` | `find`, `count`, `list`, `tree` and `match` on the disk, in archives and across both, with the hidden entries left out |
| `reading` | `read`, `seek`, `tell`, `length`, `readat` and `load` on a loose file and on each way an archive stores a file, across block boundaries and at the end |
| `malformed` | the archives that are refused, those read anyway, and broken chunks |
| `formats` | the TDF reading of TDF and FBI files, a missing and an empty one, and a TDF file that can't be parsed |
| `cob` | the COB reading of a script with several scripts, pieces and a pair, of one with none, of one with negative counts, and of a missing and an empty file |
| `gaf` | the GAF reading of compressed and raw frames (skipped, empty, literal, cut repeat and cut literal rows in the middle and at the end, a row with bytes after it fills, a short row that reads on, pixels equal to the transparent index, a `compressed` of 2), subframes under a compressed parent, an empty entry, count words with high bits and with the sign bit, and a missing and an empty file |
| `3do` | the 3DO reading of a nested tree with siblings, a piece without a name, primitives with and without a texture, the reordering with and without a selection primitive, a selection on a piece with no primitives, negative counts, and a missing and an empty file |

A vector of `reference/files/<group>.json` is `{"name", "fixture", "calls", "results"}`: the calls, each a list of
strings, and the `[call, value]` the in-repo engine wrote for each.

## Known limits

- **What the game does after loading** a COB, GAF or 3DO file is not part of its reading: the COB checksum, the
  mirroring and textures of a 3DO model, and how a GAF frame is drawn (the transparent index, `blend`).
- **The TDF section checksum** that the engine keeps with each section is not part of the reading.
- **The CD-ROM drives**, which 3.1c also mounts from, are not part of the mount sequence the fixtures can exercise.
- **Reads past the end of an archived file** are not called: 3.1c reads outside its buffers there, so there is no
  value to compare.
- **Time stamps** of listed entries, and the attribute bits other than read-only and directory, are not compared: they
  depend on the host's file system.

## The verdict

`tools/filesgate.py check --loader CMD` builds the fixtures, runs every vector and writes `result.json` in the fields of
`docs/module-abi.md` section 15:

| Field | Meaning |
| --- | --- |
| `format` | 1 |
| `verdict` | `equal`, `differs` or `failed` |
| `suite` | `files` |
| `kit` | `interfaces` and `reference_traces`: the newest `interfaces-v*` and `reference-traces-v*` tags |
| `seconds` | wall time of the run |
| `groups` | by group: `status` and the number of `vectors` |
| `vectors` | by `<group>/<vector>`: `status` and, for `differs`, `line` (the first call that differs, from 1), `call` and `value` (what the loader wrote, or `missing`); for `failed`, `reason` |

A vector is `failed` when the loader did not start, exited with another status, ran for more than 120 seconds, or wrote
a line that is not a call name, a tab and JSON. A group is `failed` when one of its vectors is, else `differs` when one
differs. A `value` is what the loader produced; the stored value is never in the file.

Exit status: 0 when every group is equal, 1 when one differs or failed, and 2 when the gate could not run (the rebuilt
fixtures don't match `fixtures.json`, or the vectors can't be read), with a message and no `result.json`.

`tools/filesgate.py retail --game-dir DIR --loader CMD` runs listing and lookup calls on an installed game with the
in-repo engine and the loader, and prints the calls where they differ: a fixed set of listings, a `find` and a `count` of
every top-level directory, and `lengthbyname` and two `load`s (upper and lower case) of each file that its `tree`
listings of `units` and `gamedata` name. Its values are never stored or exported.
