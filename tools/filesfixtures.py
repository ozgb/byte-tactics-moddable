"""Build the game directories the files gate runs a loader in (#1046), standard library only.

    python3 tools/filesfixtures.py build DIR            # every fixture, each in DIR/<fixture>/
    python3 tools/filesfixtures.py check DIR            # exit 1 when a built file differs from reference/files/fixtures.json
    python3 tools/filesfixtures.py digests              # print the digests that reference/files/fixtures.json holds

Every byte is made here: loose files, and archives in the HAPI format of docs/files-gate.md ("The archive format") with
stored, LZ77 and deflate chunks, obfuscation keys and the cases of each group. Nothing comes from retail data. The
output depends only on this file: the deflate and LZ77 encoders are this file's own, so a rebuild anywhere gives the
same bytes, which `check` compares with the digests the vectors were made on.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DIGESTS = ROOT / "reference" / "files" / "fixtures.json"
FOOTER = b"Copyright 1997 Cavedog Entertainment"
BLOCK = 0x10000


def derived_key(stored: int) -> int:
    """The key the data is obfuscated with, from the key byte of the header."""
    if stored == 0:
        return 0
    return ~((stored << 2) | (stored >> 6)) & 0xFF


def obfuscate(data: bytes, start: int, key: int) -> bytes:
    """Bytes as stored at file offset `start` of an archive whose derived key is `key`."""
    if key == 0:
        return data
    return bytes(b ^ 0xFF ^ ((start + i) & 0xFF) ^ key for i, b in enumerate(data))


class Bits:
    """A deflate bit stream, least significant bit first."""

    def __init__(self) -> None:
        self.out = bytearray()
        self.acc = 0
        self.n = 0

    def put(self, value: int, count: int) -> None:
        self.acc |= value << self.n
        self.n += count
        while self.n >= 8:
            self.out.append(self.acc & 0xFF)
            self.acc >>= 8
            self.n -= 8

    def code(self, value: int, count: int) -> None:
        self.put(int(format(value, "0%db" % count)[::-1], 2), count)

    def done(self) -> bytes:
        if self.n:
            self.out.append(self.acc & 0xFF)
        return bytes(self.out)


LENGTH_EXTRA = [0] * 8 + [1] * 4 + [2] * 4 + [3] * 4 + [4] * 4 + [5] * 4 + [0]
DIST_EXTRA = [0, 0, 0, 0] + [i // 2 for i in range(2, 28)]


def bases(first: int, extra: list[int]) -> list[int]:
    out = []
    for e in extra:
        out.append(first)
        first += 1 << e
    return out


LENGTH_BASE = bases(3, LENGTH_EXTRA[:-1]) + [258]
DIST_BASE = bases(1, DIST_EXTRA)


def matches(data: bytes, window: int, longest: int, shortest: int, usable=lambda pos, src: True):
    """Greedy (position, length, source) matches and literals (length 0) over `data`."""
    heads: dict[bytes, list[int]] = {}
    i = 0
    while i < len(data):
        best, src = 0, -1
        key = data[i:i + 3]
        if len(key) == 3:
            for cand in reversed(heads.get(key, [])[-32:]):
                if i - cand > window:
                    break
                if not usable(i, cand):
                    continue
                n = 0
                while n < longest and i + n < len(data) and data[cand + n] == data[i + n]:
                    n += 1
                if n > best:
                    best, src = n, cand
        step = best if best >= shortest else 1
        for j in range(i, i + step):
            heads.setdefault(data[j:j + 3], []).append(j)
        yield (i, best, src) if best >= shortest else (i, 0, -1)
        i += step


def deflate(data: bytes) -> bytes:
    """A zlib stream of one fixed-Huffman block."""
    bits = Bits()
    bits.put(1, 1)
    bits.put(1, 2)

    def literal(v: int) -> None:
        if v < 144:
            bits.code(0x30 + v, 8)
        elif v < 256:
            bits.code(0x190 + v - 144, 9)
        elif v < 280:
            bits.code(v - 256, 7)
        else:
            bits.code(0xC0 + v - 280, 8)

    for pos, n, src in matches(data, 32768, 258, 3):
        if n == 0:
            literal(data[pos])
            continue
        li = max(i for i, b in enumerate(LENGTH_BASE) if b <= n)
        literal(257 + li)
        bits.put(n - LENGTH_BASE[li], LENGTH_EXTRA[li])
        dist = pos - src
        di = max(i for i, b in enumerate(DIST_BASE) if b <= dist)
        bits.code(di, 5)
        bits.put(dist - DIST_BASE[di], DIST_EXTRA[di])
    literal(256)
    return b"\x78\x01" + bits.done() + struct.pack(">I", zlib.adler32(data))


def lz77(data: bytes) -> bytes:
    """The LZ77 stream of method 1: a 4096-byte ring written from position 1, matches name a ring position."""
    out = bytearray()
    items: list[bytes] = []
    flags = 0

    def push(item: bytes, is_match: bool) -> None:
        nonlocal flags
        if is_match:
            flags |= 1 << len(items)
        items.append(item)
        if len(items) == 8:
            flush()

    def flush() -> None:
        nonlocal flags, items
        out.append(flags)
        for item in items:
            out.extend(item)
        flags, items = 0, []

    def usable(pos: int, src: int) -> bool:
        return (src + 1) & 0xFFF != 0

    for pos, n, src in matches(data, 4095, 17, 2, usable):
        if n == 0:
            push(bytes([data[pos]]), False)
        else:
            push(struct.pack("<H", (((src + 1) & 0xFFF) << 4) | (n - 2)), True)
    push(b"\x00\x00", True)
    if items:
        flush()
    return bytes(out)


def chunk(data: bytes, method: int, encrypt: int = 0, *, magic: bytes = b"SQSH", bad_sum: bool = False,
          size: int | None = None) -> bytes:
    """One SQSH chunk of up to 65536 bytes."""
    body = lz77(data) if method == 1 else deflate(data) if method == 2 else data
    if encrypt:
        body = bytes(((b ^ i) + i) & 0xFF for i, b in enumerate(body))
    total = sum(body) + (1 if bad_sum else 0)
    header = magic + struct.pack("<BBBiii", 2, method, encrypt, len(body), len(data) if size is None else size,
                                 total)
    return header + body


class File:
    """A file of an archive: `methods` is None for a stored file, else one chunk method per 64 KiB block."""

    def __init__(self, name: str, data: bytes, methods=None, encrypt: int = 0, flag: int | None = None,
                 chunks=None, table_extra: int = 0) -> None:
        self.name, self.data, self.methods, self.encrypt = name, data, methods, encrypt
        self.flag = flag
        self.chunks = chunks
        self.table_extra = table_extra

    def payload(self) -> tuple[bytes, int]:
        if self.methods is None and self.chunks is None:
            return self.data, 0
        if self.chunks is not None:
            parts = self.chunks
        else:
            blocks = [self.data[i:i + BLOCK] for i in range(0, len(self.data), BLOCK)]
            methods = self.methods if isinstance(self.methods, list) else [self.methods] * len(blocks)
            parts = [chunk(b, methods[i % len(methods)], self.encrypt) for i, b in enumerate(blocks)]
        sizes = [len(p) for p in parts]
        sizes[-1] += self.table_extra
        return struct.pack("<%dI" % len(parts), *sizes) + b"".join(parts), (self.flag or 1)


class Dir:
    def __init__(self, name: str, children: list) -> None:
        self.name, self.children = name, children


def archive(entries: list, key: int = 0, *, magic: bytes = b"HAPI", version: bytes = b"\x00\x00\x01\x00",
            footer: bytes = FOOTER) -> bytes:
    """An archive holding `entries` (File and Dir, in order, names may repeat) with header key byte `key`."""
    blob = bytearray(20)
    files: list[tuple[int, File]] = []

    def grow(n: int) -> int:
        at = len(blob)
        blob.extend(b"\0" * n)
        return at

    def directory(children: list) -> int:
        at = grow(8)
        table = grow(9 * len(children))
        struct.pack_into("<II", blob, at, len(children), table)
        for i, child in enumerate(children):
            name = grow(len(child.name) + 1)
            blob[name:name + len(child.name)] = child.name.encode("latin-1")
            if isinstance(child, Dir):
                struct.pack_into("<IIB", blob, table + 9 * i, name, directory(child.children), 1)
            else:
                info = grow(9)
                files.append((info, child))
                struct.pack_into("<IIB", blob, table + 9 * i, name, info, 0)
        return at

    root = directory(entries)
    data = bytearray()
    start = len(blob)
    for info, f in files:
        body, flag = f.payload()
        struct.pack_into("<IIB", blob, info, start + len(data), len(f.data), flag)
        data.extend(obfuscate(body, start + len(data), derived_key(key)))
    blob[0:20] = magic + version + struct.pack("<IB3xI", len(blob), key, root)
    blob[20:] = obfuscate(bytes(blob[20:]), 20, derived_key(key))
    return bytes(blob) + bytes(data) + footer


def text(origin: str, path: str) -> bytes:
    return ("%s holds %s\r\n" % (origin, path)).encode()


def pattern(n: int, seed: int) -> bytes:
    """`n` bytes that compress somewhat: runs of a counter mixed with an LCG."""
    out = bytearray()
    x = seed
    while len(out) < n:
        x = (x * 1103515245 + 12345) & 0x7FFFFFFF
        if x & 0x300:
            out.extend(b"unit%05d;" % ((x >> 8) % 977))
        else:
            out.append((x >> 16) & 0xFF)
    return bytes(out[:n])


TDF_SAMPLE = b"""// sample: the parser's cases
[Alpha]
\t{
\tName = First unit ;
\tname=second wins;
\tName = third ;
\tZeta=  42abc  ;
\tFloat=0.15;
\tNeg=-7.5;
\tBig=40000;
\tHex=0x10;
\tEmpty=;
\tSpaced key = a = b ;
\t/* a block
\t   comment */ Inner=1;
\t[Child]
\t\t{
\t\tx=1;
\t\t}
\t[child]
\t\t{
\t\ty=2;
\t\t}
\t}
[Beta]{a=1;}
"""

TDF_BROKEN = b"[Alpha]\n{\nkey=value\n}\n"

ARM_FBI = b"""[UNITINFO]
\t{
\tUnitName=ARMFIX;
\tName=Fixture Kbot;
\tBuildCostMetal=120;
\tMaxDamage=650;
\tAcceleration=0.12;
\t}
"""


class Blob:
    """A file laid out front to back: `take` reserves bytes and returns their offset."""

    def __init__(self) -> None:
        self.data = bytearray()

    def take(self, n: int) -> int:
        at = len(self.data)
        self.data.extend(b"\0" * n)
        return at

    def put(self, raw: bytes) -> int:
        at = len(self.data)
        self.data.extend(raw)
        return at

    def string(self, name: str) -> int:
        return self.put(name.encode("latin-1") + b"\0")


def cob(scripts: list[tuple[str, int]], pieces: list[str], statics: int, pairs: list[tuple[int, str]],
        code: list[int], version: int = 4) -> bytes:
    """A COB script in the layout of docs/files-gate.md ("COB"): the header, the code, the tables, then the names."""
    b = Blob()
    b.take(44)
    code_at = b.put(struct.pack("<%di" % len(code), *code))
    starts_at = b.put(struct.pack("<%di" % len(scripts), *[s for _, s in scripts]))
    names_at = b.take(4 * len(scripts))
    pieces_at = b.take(4 * len(pieces))
    pairs_at = b.take(8 * len(pairs))
    for i, (name, _) in enumerate(scripts):
        struct.pack_into("<i", b.data, names_at + 4 * i, b.string(name))
    for i, name in enumerate(pieces):
        struct.pack_into("<i", b.data, pieces_at + 4 * i, b.string(name))
    for i, (word, name) in enumerate(pairs):
        struct.pack_into("<ii", b.data, pairs_at + 8 * i, word, b.string(name))
    struct.pack_into("<11i", b.data, 0, version, len(scripts), len(pieces), len(code), statics, len(pairs),
                     starts_at, names_at, pieces_at, code_at, pairs_at)
    return bytes(b.data)


class Frame:
    """A GAF frame: `rows` of palette indices (None for a pixel a compressed row skips), or `subframes`.

    A compressed frame's row may be given as `bytes`, the row's encoded form as stored.
    """

    def __init__(self, width: int, height: int, x: int, y: int, transparent: int, *, rows=None,
                 compressed: int = 0, blend: int = 0, subframes=None) -> None:
        self.width, self.height, self.x, self.y = width, height, x, y
        self.transparent, self.rows, self.compressed, self.blend = transparent, rows, compressed, blend
        self.subframes = subframes


def gaf_row(row: list) -> bytes:
    """One row of a compressed frame: skips of transparent pixels, repeats, and literal runs."""
    out = bytearray()
    i = 0
    while i < len(row):
        if row[i] is None:
            n = 1
            while i + n < len(row) and row[i + n] is None and n < 127:
                n += 1
            out.append(n << 1 | 1)
        else:
            n = 1
            while i + n < len(row) and row[i + n] == row[i] and n < 64:
                n += 1
            if n >= 2:
                out += bytes([(n - 1) << 2 | 2, row[i]])
            else:
                while i + n < len(row) and row[i + n] is not None and n < 64 and \
                        (i + n + 1 >= len(row) or row[i + n + 1] != row[i + n]):
                    n += 1
                out += bytes([(n - 1) << 2]) + bytes(row[i:i + n])
        i += n
    return bytes(out)


def gaf(entries: list[tuple[str, int, list[tuple[Frame, int]]]], count: int | None = None) -> bytes:
    """A GAF file of (name, loop, [(frame, duration)]) entries, in the layout of docs/files-gate.md ("GAF")."""
    b = Blob()
    b.take(12 + 4 * len(entries))
    struct.pack_into("<III", b.data, 0, 0x00010100, len(entries) if count is None else count, 0)

    def frame(f: Frame) -> int:
        at = b.take(24)
        if f.subframes:
            data = b.take(4 * len(f.subframes))
            for k, sub in enumerate(f.subframes):
                struct.pack_into("<I", b.data, data + 4 * k, frame(sub))
        elif f.compressed:
            data = len(b.data)
            for row in f.rows:
                raw = row if isinstance(row, bytes) else gaf_row(row)
                b.put(struct.pack("<H", len(raw)) + raw)
        else:
            data = b.put(bytes(p for row in f.rows for p in row))
        struct.pack_into("<HHhhBBBBIII", b.data, at, f.width, f.height, f.x, f.y, f.transparent, f.compressed,
                         len(f.subframes or ()), f.blend, 0, data, 0)
        return at

    for i, (name, loop, frames) in enumerate(entries):
        at = b.take(40 + 8 * len(frames))
        struct.pack_into("<HBBI32s", b.data, at, len(frames), loop, 0, 0, name.encode("latin-1"))
        struct.pack_into("<I", b.data, 12 + 4 * i, at)
        for j, (f, duration) in enumerate(frames):
            struct.pack_into("<II", b.data, at + 40 + 8 * j, frame(f), duration)
    return bytes(b.data)


class Piece:
    """A 3DO piece: `primitives` are (colour, vertex indices, texture name or None)."""

    def __init__(self, name: str | None, offset: tuple[int, int, int], vertices: list[tuple[int, int, int]],
                 primitives: list[tuple[int, list[int], str | None]], selection: int = -1, children=()) -> None:
        self.name, self.offset, self.vertices, self.primitives = name, offset, vertices, primitives
        self.selection, self.children = selection, list(children)


def model(pieces: list[Piece]) -> bytes:
    """A 3DO model whose top level is `pieces`, in the layout of docs/files-gate.md ("3DO"); the first is at offset 0."""
    b = Blob()

    def chain(rest: list[Piece]) -> int:
        if not rest:
            return 0
        p = rest[0]
        at = b.take(52)
        name = 0 if p.name is None else b.string(p.name)
        verts = b.put(b"".join(struct.pack("<iii", *v) for v in p.vertices))
        prims = b.take(32 * len(p.primitives))
        for i, (colour, indices, texture) in enumerate(p.primitives):
            idx = b.put(struct.pack("<%dH" % len(indices), *indices))
            tex = 0 if texture is None else b.string(texture)
            struct.pack_into("<8i", b.data, prims + 32 * i, colour, len(indices), 0, idx, tex, 0, 0, 0)
        child = chain(p.children)
        sibling = chain(rest[1:])
        struct.pack_into("<13i", b.data, at, 1, len(p.vertices), len(p.primitives), p.selection, *p.offset, name, 0,
                         verts, prims, sibling, child)
        return at

    chain(pieces)
    return bytes(b.data)


ARMFIX_COB = cob([("Create", 0), ("Activate", 4), ("Deactivate", 7)], ["base", "turret", "barrel"], 5,
                 [(9, "pair")], [0x10021001, 0, 1, -65536, 0x10011000, 2, 0x7FFFFFFF, 0x10064000, -1, 3, 0x10012000])
NONE_COB = cob([], [], 0, [], [])
NEGATIVE_COB = struct.pack("<6i", 4, -1, -1, -3, -2, -1) + NONE_COB[24:]

GAF_IDLE = Frame(6, 4, 3, 2, 9, compressed=1, rows=[
    [None, None, 5, 5, 5, 7],
    b"",
    [11, 23, 3, 47, 5, 60],
    bytes([1 << 1 | 1, (8 - 1) << 2 | 2, 0x22]),
])
GAF_RAW = Frame(4, 3, -1, -2, 0, rows=[[0, 1, 2, 3], [9, 9, 0, 0], [255, 128, 7, 0]])
GAF_CUTS = Frame(4, 5, 0, 0, 7, compressed=2, rows=[
    bytes([1 << 1 | 1, (5 - 1) << 2 | 2, 7]),
    bytes([(6 - 1) << 2, 21, 7, 33, 44, 55, 66]),
    bytes([(4 - 1) << 2 | 2, 5, 0, 0]),
    bytes([(2 - 1) << 2, 9, 8]),
    [10, 11, 12, 13],
])
GAF_LAYERED = Frame(5, 5, 2, 2, 0, compressed=1, subframes=[
    Frame(2, 2, 1, 1, 3, rows=[[3, 4], [4, 3]]),
    Frame(3, 2, 0, 0, 0, compressed=1, blend=1, rows=[[None, 8, None], [6, 6, None]]),
])
FIXTURE_GAF = gaf([("Idle", 1, [(GAF_IDLE, 2), (GAF_RAW, 0x00010004)]),
                   ("Empty", 0, []),
                   ("Layered", 0, [(GAF_LAYERED, 1)]),
                   ("Cuts", 1, [(GAF_CUTS, 3)])])
COUNT_GAF = gaf([("Only", 0, [(Frame(1, 1, 0, 0, 0, rows=[[42]]), 1)])], count=0x00020001)
NEGATIVE_GAF = gaf([], count=0x0000FFFF)

FIXTURE_3DO = model([
    Piece("base", (0, 0, 0),
          [(0, 0, 0), (10, -1, 0), (0, 0, 10), (5, 4, 5), (1, -3, 1), (2, 8, 2)],
          [(1, [0, 1, 2], "skin"), (2, [3, 4, 5], "skin"), (3, [0, 2], None), (4, [1, 4, 0, 2], None),
           (5, [5, 3], "glow")],
          selection=3,
          children=[
              Piece("turret", (0, 3 * 65536, 0), [(0, 0, 0), (1, 1, 1), (-1, 2, -1)],
                    [(6, [0, 1, 2], "armtex"), (7, [2, 1, 0], None)],
                    children=[Piece("barrel", (0, 0, 2 * 65536), [(0, 0, 4)], [], selection=2)]),
              Piece(None, (-65536, 0, 0), [], []),
          ]),
    Piece("debris", (7, 8, 9), [(1, 2, 3)], [(8, [0], None)]),
])
NEGATIVE_3DO = model([Piece("negative", (1, 2, 3), [], [], selection=1)])
NEGATIVE_3DO = NEGATIVE_3DO[:4] + struct.pack("<ii", -1, -2) + NEGATIVE_3DO[12:]
FLAT_3DO = model([Piece("flat", (0, 0, 0), [(0, 5, 0), (0, 2, 0), (0, 1, 0)],
                        [(1, [0], None), (2, [1], None), (3, [2], None)])])


def base() -> dict[str, bytes]:
    files: dict[str, bytes] = {
        "readme.txt": text("loose", "readme.txt"),
        "units/arm.fbi": ARM_FBI,
        "units/Loose.fbi": text("loose", "units\\Loose.fbi"),
        "units/Shadow.TXT": text("loose", "units\\Shadow.TXT"),
        "units/arm_x.fbi": text("loose", "units\\arm_x.fbi"),
        "units/armx.fbi": text("loose", "units\\armx.fbi"),
        "gamedata/sample.tdf": TDF_SAMPLE,
        "gamedata/broken.tdf": TDF_BROKEN,
        "notes/": b"",
        "data/loose.bin": pattern(70000, 12),
        "x.hpi": b"not an archive\r\n",
        "scripts/armfix.cob": ARMFIX_COB,
        "scripts/none.cob": NONE_COB,
        "anims/fixture.gaf": FIXTURE_GAF,
        "anims/count.gaf": COUNT_GAF,
        "anims/negative.gaf": NEGATIVE_GAF,
        "scripts/negative.cob": NEGATIVE_COB,
        "objects3d/negative.3do": NEGATIVE_3DO,
        "objects3d/fixture.3do": FIXTURE_3DO,
        "objects3d/flat.3do": FLAT_3DO,
    }
    big = pattern(150000, 1)
    files["rev31.gp3"] = archive([
        Dir("units", [File("arm.fbi", text("rev31.gp3", "units\\arm.fbi")),
                      File("core.fbi", text("rev31.gp3", "units\\core.fbi")),
                      File("Shadow.txt", text("rev31.gp3", "units\\Shadow.txt"))]),
        File("dup.txt", text("rev31.gp3", "dup.txt first")),
        File("dup.txt", text("rev31.gp3", "dup.txt second")),
        Dir("Mixed", [File("Case.Txt", text("rev31.gp3", "Mixed\\Case.Txt"))]),
        Dir("data", [File("big.bin", big, 1),
                     File("big2.bin", pattern(140000, 2), 2, flag=2),
                     File("small.lz", pattern(100, 3), 1),
                     File("exact.bin", pattern(2 * BLOCK, 4), 1),
                     File("enc.bin", pattern(70000, 5), 2, encrypt=1, flag=2),
                     File("mixed.bin", pattern(200000, 6), [1, 2], encrypt=1),
                     File("stored.bin", pattern(70000, 7)),
                     File("empty.bin", b"")]),
        Dir("twin", [File("a.txt", text("rev31.gp3", "twin\\a.txt"))]),
        Dir("twin", [File("b.txt", text("rev31.gp3", "twin\\b.txt"))]),
        Dir("blk", [File("inner.txt", text("rev31.gp3", "blk\\inner.txt"))]),
        File("blk", text("rev31.gp3", "blk")),
        File("a/b.txt", text("rev31.gp3", "a/b.txt")),
        Dir("gamedata", [File("sidedata.tdf", b"[SIDE0]{name=ARM;}\r\n"),
                         File("sample.tdf", b"[Shadowed]{a=1;}\r\n")]),
    ])
    files["a.ccx"] = archive([
        Dir("units", [File("core.fbi", text("a.ccx", "units\\core.fbi")),
                      File("only_ccx.fbi", text("a.ccx", "units\\only_ccx.fbi")),
                      File("lz.fbi", pattern(3000, 8), 1)]),
        Dir("data", [File("keyed.bin", pattern(90000, 9), 1),
                     File("keyed_stored.bin", pattern(5000, 10))]),
    ], key=0x31)
    files["b.ccx"] = archive([
        Dir("units", [File("only_ccx.fbi", text("b.ccx", "units\\only_ccx.fbi")),
                      File("b.fbi", text("b.ccx", "units\\b.fbi"))]),
    ], key=0xFF)
    files["z.ufo"] = archive([
        Dir("units", [File("ufo.fbi", text("z.ufo", "units\\ufo.fbi"))]),
        File("readme.txt", text("z.ufo", "readme.txt")),
    ], key=0x07)
    files["c.hpi"] = archive([
        Dir("units", [File("c.fbi", text("c.hpi", "units\\c.fbi")),
                      File("core.fbi", text("c.hpi", "units\\core.fbi")),
                      File("arm.fbi", text("c.hpi", "units\\arm.fbi"))]),
        Dir("notes", [File("n.txt", text("c.hpi", "notes\\n.txt"))]),
    ], key=0x80)
    files["D.HPI"] = archive([Dir("UNITS", [File("D.FBI", text("D.HPI", "UNITS\\D.FBI"))])])
    files["e_x.hpi"] = archive([Dir("units", [File("fold.fbi", text("e_x.hpi", "units\\fold.fbi"))])], key=0x21)
    files["ex.hpi"] = archive([Dir("units", [File("fold.fbi", text("ex.hpi", "units\\fold.fbi"))])], key=0x22)
    files["rev32.gp3"] = archive([File("rev32.txt", text("rev32.gp3", "rev32.txt"))])
    return files


def budget() -> dict[str, bytes]:
    files: dict[str, bytes] = {}
    for i in range(1, 13):
        name = "h%02d.hpi" % i
        files[name] = archive([File("id.txt", text(name, "id.txt")), File("h%02d.txt" % i, text(name, "own"))])
    files["h05bad.hpi"] = b"HAPI but no archive\r\n"
    return files


def malformed() -> dict[str, bytes]:
    good = [File("good.txt", text("good.hpi", "good.txt"))]
    files: dict[str, bytes] = {
        "badmagic.hpi": archive(good, magic=b"HAPX"),
        "badver.hpi": archive(good, version=b"\x00\x00\x02\x00"),
        "nofooter.hpi": archive(good, footer=b""),
        "lower.hpi": archive(good, footer=FOOTER.replace(b"Copyright", b"copyright")),
        "year.hpi": archive([File("year.txt", text("year.hpi", "year.txt"))],
                            footer=FOOTER.replace(b"1997", b"2026")),
        "digits.hpi": archive([File("digits.txt", text("digits.hpi", "digits.txt"))],
                              footer=FOOTER.replace(b"1997", b"abcd")),
    }
    files["scripts/trunc.cob"] = ARMFIX_COB[:60]
    files["scripts/over.cob"] = struct.pack("<ii", 4, 100000) + ARMFIX_COB[8:]
    files["anims/trunc.gaf"] = FIXTURE_GAF[:40]
    files["anims/over.gaf"] = FIXTURE_GAF[:struct.unpack_from("<I", FIXTURE_GAF, 12)[0]] + struct.pack("<H", 30000) \
        + FIXTURE_GAF[struct.unpack_from("<I", FIXTURE_GAF, 12)[0] + 2:]
    files["objects3d/trunc.3do"] = FIXTURE_3DO[:52]
    files["objects3d/over.3do"] = FIXTURE_3DO[:8] + struct.pack("<i", 100000) + FIXTURE_3DO[12:]
    block = pattern(1000, 11)
    files["good.hpi"] = archive(good + [
        File("badsum.bin", block, chunks=[chunk(block, 1, bad_sum=True)]),
        File("badsqsh.bin", block, chunks=[chunk(block, 1, magic=b"SQSX")]),
        File("badmethod.bin", block, chunks=[chunk(block, 5)]),
        File("badsize.bin", block, chunks=[chunk(block, 1, size=999)]),
        File("trunc.bin", block, 1, table_extra=100),
    ], key=0x55)
    return files


FIXTURES = {"base": base, "budget": budget, "malformed": malformed}


def build(out: Path) -> dict[str, dict[str, dict]]:
    """Write every fixture under `out` and return their digests."""
    digests: dict[str, dict[str, dict]] = {}
    for name, make in FIXTURES.items():
        top = out / name
        if top.exists() and any(top.iterdir()):
            raise SystemExit("filesfixtures: %s is not empty; build into an empty directory" % top)
        top.mkdir(parents=True, exist_ok=True)
        digests[name] = {}
        for path, data in sorted(make().items()):
            if path.endswith("/"):
                (top / path).mkdir(parents=True, exist_ok=True)
                continue
            target = top / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            digests[name][path] = {"sha256": hashlib.sha256(data).hexdigest(), "size": len(data)}
    return digests


def digests_of() -> dict[str, dict[str, dict]]:
    out: dict[str, dict[str, dict]] = {}
    for name, make in FIXTURES.items():
        out[name] = {p: {"sha256": hashlib.sha256(d).hexdigest(), "size": len(d)}
                     for p, d in sorted(make().items()) if not p.endswith("/")}
    return out


def check(out: Path, stored: dict) -> list[str]:
    """The fixture files under `out` that are missing, differ from `stored` or are not in it."""
    problems = []
    for name, files in stored.items():
        top = out / name
        if top.is_dir():
            for target in sorted(top.rglob("*")):
                rel = target.relative_to(top).as_posix()
                if target.is_file() and rel not in files:
                    problems.append("extra: %s/%s" % (name, rel))
        for path, want in files.items():
            target = out / name / path
            if not target.is_file():
                problems.append("missing: %s/%s" % (name, path))
            elif hashlib.sha256(target.read_bytes()).hexdigest() != want["sha256"]:
                problems.append("differs: %s/%s" % (name, path))
    return problems


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("build").add_argument("dir", type=Path)
    sub.add_parser("check").add_argument("dir", type=Path)
    sub.add_parser("digests")
    args = parser.parse_args()
    if args.command == "build":
        build(args.dir)
        print("filesfixtures: wrote %s" % ", ".join(FIXTURES))
    elif args.command == "check":
        problems = check(args.dir, json.loads(DIGESTS.read_text())["fixtures"])
        print("\n".join(problems) or "filesfixtures: every fixture file matches")
        sys.exit(1 if problems else 0)
    else:
        print(json.dumps(digests_of(), indent=1))


if __name__ == "__main__":
    main()
