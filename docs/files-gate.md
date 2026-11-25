# The files gate

The files gate judges a data loader by its behaviour alone: how it mounts the archives of a game directory, which file a
name resolves to, what a listing holds and in what order, the bytes and return values of its reads, what it does with a
malformed archive, and how it reads a TDF file. The loader is one program; the gate runs it on fixture directories that
the kit's fixture tool builds, and compares what it writes with the vectors of `reference/files/`.

The vectors were made by the in-repo engine through the files service of `interfaces/files.h`, on the same fixtures.
Neither the fixtures nor the vectors hold any byte of the retail game.

## The fixtures

```sh
python3 tools/filesfixtures.py build DIR     # DIR/base, DIR/budget and DIR/malformed
python3 tools/filesfixtures.py check DIR     # compare a build with reference/files/fixtures.json
```

`tools/filesfixtures.py` needs Python 3.10 and its standard library, and gives the same bytes on every machine: the
digests of every file are in `reference/files/fixtures.json`, and a vector is only meaningful on fixtures that match them.

| Fixture | What it holds |
| --- | --- |
| `base` | loose files and directories; `rev31.gp3`, two `.ccx`, a `.ufo` and two `.hpi` archives (one named in capitals), each with another key; `rev32.gp3`, a valid archive the game does not mount; `x.hpi`, a file that is not an archive. The archives hold the same paths as each other and as the disk, names stored twice in one directory, a directory stored twice, a directory and a file of one name, a name with a `/` in it, and files stored plainly and in LZ77 and deflate chunks, with and without chunk encryption, across block boundaries and of exactly two blocks |
| `budget` | twelve valid `.hpi` archives and one that is not an archive, between the fifth and the sixth |
| `malformed` | archives with a bad magic, a bad version, no copyright footer, a footer in lower case, a footer with another year and one with letters for the year, and a good archive holding files whose chunks are broken: a bad checksum, a bad chunk magic, an unknown method, a wrong unpacked size, and a chunk table that runs past the end of the archive |

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
written as `\u00XX`, and the gate compares parsed values, so spacing within the JSON is free.

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

The names in this table are the slots of `interfaces/files.h`, which says what each takes and returns. A loader with
another interface maps each call onto its own; only the values are judged.

**The mount sequence** is what the game does once at start: it mounts `rev31.gp3`, then every `*.ccx`, then every
`*.ufo`, then the `*.hpi` archives, each group in the order of a listing of the game directory, and then marks the
archive entries that the disk or an earlier archive hides from listings. An archive whose header or footer is refused is
skipped, and so is one already mounted. A `mount` after the first is the same sequence again, on what is already
mounted. The vectors of the `mounting` group are its definition, the order and the skipping included.

**A fatal error.** Where 3.1c stops the game with an error message (a chunk that can't be unpacked, a TDF file it can't
parse), the loader writes the line `fatal<TAB>null` for that call, answers no further call, and exits with status 0.

**The TDF reading** is an object `{"name", "fields", "records"}` for the file (named `root`) and, inside it, each
section in the order of the file. `fields` lists the fields of a section in the order it keeps them (sorted by name
without regard to case), each as `[name, stored value, looked-up value, integer, fixed, double]`: the value stored with
that entry, the value a lookup of its name returns, the lookup's value read as a C `atoi` integer, as a 16.16 fixed
point number (the `atof` value times 65536, converted to an integer as the x87 does, so 0x80000000 when it is out of
range), and as the `atof` double of MSVC 5's C library (its 64 bits in hex, most significant first). The FBI unit files
are TDF files and are read the same way.

## The groups

| Group | What it covers |
| --- | --- |
| `mounting` | which archives are mounted, in what order, which are skipped, the limit on the `.hpi` archives, and a second `mount` |
| `lookup` | the file a name resolves to: the disk before the archives and an earlier archive before a later one, case, both separators, names stored twice, missing names and directories |
| `listing` | `find`, `count`, `list`, `tree` and `match` on the disk, in archives and across both, with the hidden entries left out |
| `reading` | `read`, `seek`, `tell`, `length`, `readat` and `load` on a loose file and on each way an archive stores a file, across block boundaries and at the end |
| `malformed` | the archives that are refused, those read anyway, and broken chunks |
| `formats` | the TDF reading of TDF and FBI files, a missing and an empty one, and a TDF file that can't be parsed |

A vector of `reference/files/<group>.json` is `{"name", "fixture", "calls", "results"}`: the calls, each a list of
strings, and the `[call, value]` the in-repo engine wrote for each.

## Known limits

- **COB, GAF and 3DO** are not judged here. What the engine takes from these files lives in its own structures (a unit
  script's code and the pieces it drives, a sprite's frames as the renderer keeps them, a model's piece tree), and a
  dump of those would name them. They are judged by the scenario mode (`docs/core-gate.md`) when a core can play a
  scenario: a unit that moves, animates or draws differently changes the traces.
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
| `kit` | `interfaces`: the newest `interfaces-v*` tag |
| `seconds` | wall time of the run |
| `groups` | by group: `status` and the number of `vectors` |
| `vectors` | by `<group>/<vector>`: `status` and, for `differs`, `line` (the first call that differs, from 1), `call` and `value` (what the loader wrote, or `missing`); for `failed`, `reason` |

A vector is `failed` when the loader did not start, exited with another status, ran for more than 120 seconds, or wrote
a line that is not a call name, a tab and JSON. A group is `failed` when one of its vectors is, else `differs` when one
differs. A `value` is what the loader produced; the stored value is never in the file.

`tools/filesgate.py retail --game-dir DIR --loader CMD` runs the listing and lookup calls on an installed game, for
every top-level directory and every file the listings name, with the in-repo engine and the loader, and prints the calls
where they differ. Its values are never stored or exported.
