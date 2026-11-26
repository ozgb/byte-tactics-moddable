"""The recorded player view: its layout from interfaces/view.h, its fields and its hash.

    uv run tools/viewblock.py show views/900.bin            # every field as `<path> <value>`
    uv run tools/viewblock.py diff a/views/900.bin b/views/900.bin   # the first field that differs, with both values

A test-mode run writes the view of a tick as views/<tick>.bin and hashes it in the `view` row (docs/view.md,
"The recorded view"). This module reads the layout of every struct from interfaces/view.h (plain scalar fields and
fixed arrays, constants from its #defines, the i686 Visual C++ rules: natural alignment, a double on 8 bytes, padding
zero), decodes the serialisation S of a block into (path, value) pairs in serialisation order, such as `units[3].x` and
`players[0].metal`, and compares two files.

Exit status of `diff`: 0 if the files are equal, 1 if they differ, 2 on a file that is not a view file.
"""

import argparse
import itertools
import re
import struct
import sys
from functools import lru_cache
from pathlib import Path
from typing import NamedTuple

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "interfaces/view.h"
MAGIC = b"BTVW"
FILE_VERSION = 1
FILE_HEADER = struct.pack("<4sHH", MAGIC, FILE_VERSION, 0)
ROOT_STRUCT = "PlayerView"
HEADER_END = "players"
TABLES = (("units", "unitCount"), ("pieces", "pieceCount"), ("orders", "orderCount"), ("blips", "blipCount"),
          ("features", "featureCount"), ("projectiles", "projectileCount"), ("debris", "debrisCount"),
          ("explodedPieces", "explodedPieceCount"), ("debrisVertices", "debrisVertexCount"), ("debrisFaces", "debrisFaceCount"),
          ("debrisIndices", "debrisIndexCount"), ("textures", "textureCount"), ("messages", "messageCount"))
SIGHT = "sight"

FNV_OFFSET = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3
MASK = (1 << 64) - 1

SCALARS = {
    "unsigned char": ("B", 1), "char": ("b", 1), "signed char": ("b", 1),
    "unsigned short": ("H", 2), "short": ("h", 2),
    "unsigned int": ("I", 4), "int": ("i", 4), "float": ("f", 4), "double": ("d", 8),
}


class Error(Exception):
    pass


class Field(NamedTuple):
    name: str
    type: str
    count: int | None
    offset: int


class Struct(NamedTuple):
    name: str
    fields: tuple[Field, ...]
    size: int
    align: int


def view_hash(data: bytes) -> int:
    """The trace hash (docs/trace-format.md, section 3) of `data` as one run, from the offset basis."""
    h = FNV_OFFSET
    whole = len(data) - len(data) % 8
    for (word,) in struct.iter_unpack("<Q", data[:whole]):
        h = ((h ^ word) * FNV_PRIME) & MASK
    for byte in data[whole:]:
        h = ((h ^ byte) * FNV_PRIME) & MASK
    return h


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def parse_header(text: str) -> tuple[dict[str, int], dict[str, str], list[tuple[str, list[tuple[str, str, str | None]]]]]:
    """(the integer #defines, the enum underlying types, the (name, [(type, field, count text)]) of each struct) of a view.h."""
    text = strip_comments(text)
    defines = {m[1]: int(m[2], 0) for m in re.finditer(r"^\s*#define\s+(\w+)\s+(0x[0-9a-fA-F]+|\d+)\s*$", text, re.M)}
    enums = {m[1]: m[2].strip() for m in re.finditer(r"\benum\s+(\w+)\s*:\s*([\w ]+?)\s*\{", text)}
    structs = []
    for m in re.finditer(r"\bstruct\s+(\w+)\s*\{(.*?)\};", text, re.S):
        fields = []
        for decl in m[2].split(";"):
            decl = decl.strip()
            if not decl:
                continue
            f = re.fullmatch(r"([\w ]+?)\s+(\w+)\s*(?:\[\s*(\w+)\s*\])?", decl)
            if not f:
                raise Error(f"view.h: cannot read the field `{decl}` of {m[1]}")
            fields.append((f[1].strip(), f[2], f[3]))
        structs.append((m[1], fields))
    return defines, enums, structs


@lru_cache(maxsize=None)
def layouts(path: Path = HEADER) -> dict[str, Struct]:
    """The layout of every struct of the header, by name."""
    defines, enums, parsed = parse_header(path.read_text())
    out: dict[str, Struct] = {}
    for name, decls in parsed:
        offset, align = 0, 1
        fields = []
        for type_, field, count_text in decls:
            type_ = enums.get(type_, type_)
            if type_ in SCALARS:
                size = a = SCALARS[type_][1]
            elif type_ in out:
                size, a = out[type_].size, out[type_].align
            else:
                raise Error(f"view.h: field {name}.{field} has the unknown type `{type_}`")
            count = None
            if count_text is not None:
                if count_text.isdigit():
                    count = int(count_text)
                elif count_text in defines:
                    count = defines[count_text]
                else:
                    raise Error(f"view.h: {name}.{field} has the unknown count `{count_text}`")
            offset = -(-offset // a) * a
            fields.append(Field(field, type_, count, offset))
            offset += size * (count if count is not None else 1)
            align = max(align, a)
        out[name] = Struct(name, tuple(fields), -(-offset // align) * align, align)
    return out


def constants(path: Path = HEADER) -> dict[str, int]:
    return parse_header(path.read_text())[0]


def field_names(struct_name: str) -> list[str]:
    return [f.name for f in layouts()[struct_name].fields]


def header_size() -> int:
    """The bytes of PlayerView before `players`."""
    return next(f.offset for f in layouts()[ROOT_STRUCT].fields if f.name == HEADER_END)


def capacity(table: str) -> int:
    field = next(f for f in layouts()[ROOT_STRUCT].fields if f.name == table)
    assert field.count is not None
    return field.count


def element_size(table: str) -> int:
    field = next(f for f in layouts()[ROOT_STRUCT].fields if f.name == table)
    return layouts()[field.type].size if field.type in layouts() else SCALARS[field.type][1]


def header_values(data: bytes) -> dict[str, tuple]:
    """Path -> (value, raw bytes, offset) of the header fields of PlayerView that `data` holds."""
    head = layouts()[ROOT_STRUCT]
    fields_ = tuple(itertools.takewhile(lambda f: f.name != HEADER_END, head.fields))
    return dict(scalar_values(data[:header_size()], head._replace(fields=fields_), 0, ""))


def serialisation_size(data: bytes) -> int | None:
    """The length S has, from the counts and the grid size in the header of `data`, or None when the header is short."""
    head = header_size()
    if len(data) < head:
        return None
    values = header_values(data)
    total = head + capacity("players") * element_size("players")
    for table, count in TABLES:
        total += min(values[count][0], capacity(table)) * element_size(table)
    return total + min(values["gridWidth"][0] * values["gridHeight"][0], capacity(SIGHT))


def scalar_values(data: bytes, struct_: Struct, base: int, prefix: str):
    """Yield (path, (value, raw bytes, offset)) for each scalar of struct_ at `base` that `data` holds, fields in declaration order."""
    for f in struct_.fields:
        yield from field_values(data, f, base, prefix)


def field_values(data: bytes, f: Field, base: int, prefix: str):
    if f.type in SCALARS:
        code, size = SCALARS[f.type]
        for i in range(f.count if f.count is not None else 1):
            at = base + f.offset + i * size
            if at + size > len(data):
                return
            raw = data[at:at + size]
            yield f"{prefix}{f.name}" + (f"[{i}]" if f.count is not None else ""), (struct.unpack("<" + code, raw)[0], raw, at)
    else:
        inner = layouts()[f.type]
        for i in range(f.count if f.count is not None else 1):
            at = base + f.offset + i * inner.size
            yield from scalar_values(data, inner, at, f"{prefix}{f.name}" + (f"[{i}]" if f.count is not None else "") + ".")


def runs(data: bytes) -> list[tuple[str, int, int, int]]:
    """The (table, offset in S, elements, element size) of each part of S after the header, in serialisation order."""
    values = header_values(data)
    at = header_size()
    out = [("players", at, capacity("players"), element_size("players"))]
    at += capacity("players") * element_size("players")
    for table, count in TABLES:
        n = min(values[count][0], capacity(table))
        out.append((table, at, n, element_size(table)))
        at += n * element_size(table)
    n = min(values["gridWidth"][0] * values["gridHeight"][0], capacity(SIGHT))
    out.append((SIGHT, at, n, 1))
    return out


def split(raw: bytes) -> bytes:
    """S of a view file's bytes; Error when the file does not start with the view file header."""
    if len(raw) < len(FILE_HEADER) or raw[:4] != MAGIC:
        raise Error("not a view file (no BTVW magic)")
    version, zero = struct.unpack_from("<HH", raw, 4)
    if version != FILE_VERSION or zero != 0:
        raise Error(f"view file version {version}, expected {FILE_VERSION}")
    return raw[len(FILE_HEADER):]


def fields(data: bytes) -> list[tuple[str, int | float, bytes]]:
    """(path, value, raw bytes) of each field that S `data` holds, in serialisation order.

    A block cut short ends with the last complete field; the sight grid is `sight[i]`, one byte a cell.
    """
    head = header_size()
    out = [(p, v, r) for p, (v, r, _) in header_values(data).items()]
    if len(data) < head:
        return out
    for table, at, n, size in runs(data):
        if table == SIGHT:
            out += [(f"sight[{i}]", data[at + i], data[at + i:at + i + 1]) for i in range(min(n, max(len(data) - at, 0)))]
            continue
        field = next(f for f in layouts()[ROOT_STRUCT].fields if f.name == table)
        for i in range(n):
            if at + (i + 1) * size > len(data):
                break
            if field.type in SCALARS:
                raw = data[at + i * size:at + (i + 1) * size]
                out.append((f"{table}[{i}]", struct.unpack("<" + SCALARS[field.type][0], raw)[0], raw))
                continue
            out += [(p, v, r) for p, (v, r, _) in scalar_values(data, layouts()[field.type], at + i * size, f"{table}[{i}].")]
    return out


def decode(data: bytes) -> list[tuple[str, int | float]]:
    """The (path, value) pairs of S `data`, such as ("units[3].x", 1024) and ("players[0].metal", 1000.0)."""
    return [(p, v) for p, v, _ in fields(data)]


def show_value(value: int | float) -> str:
    return f"{value:.9g}" if isinstance(value, float) else str(value)


def first_difference(a: bytes, b: bytes) -> tuple[str, str, str] | None:
    """(path, a's value, b's value) of the first field, in serialisation order, in which S `a` and S `b` differ; "-" when a side has none.

    A block that differs in padding or past its last whole field only is named `<padding or length>`.
    """
    fa, fb = fields(a), fields(b)
    for x, y in zip(fa, fb):
        if x[2] != y[2]:
            return x[0], show_value(x[1]), show_value(y[1])
    if len(fa) != len(fb):
        longer = fa if len(fa) > len(fb) else fb
        extra = longer[min(len(fa), len(fb))]
        return (extra[0], show_value(extra[1]) if longer is fa else "-", show_value(extra[1]) if longer is fb else "-")
    if a != b:
        return "<padding or length>", f"{len(a)} bytes", f"{len(b)} bytes"
    return None


def describe_difference(path_a: Path, path_b: Path) -> str | None:
    """The first differing field of two view files, `<path>: a=<value> b=<value>`, or None when the files are equal."""
    a, b = (split(Path(p).read_bytes()) for p in (path_a, path_b))
    found = first_difference(a, b)
    return f"{found[0]}: a={found[1]} b={found[2]}" if found else None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)
    sub.add_parser("show", help="print every field of a view file").add_argument("file")
    d = sub.add_parser("diff", help="name the first field in which two view files differ")
    d.add_argument("a")
    d.add_argument("b")
    args = ap.parse_args()
    try:
        if args.command == "show":
            for path, value in decode(split(Path(args.file).read_bytes())):
                print(f"{path} {show_value(value)}")
            return 0
        found = describe_difference(Path(args.a), Path(args.b))
        print(f"first differing field: {found}" if found else "view files identical")
        return 1 if found else 0
    except (Error, OSError) as e:
        print(f"viewblock: {e}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
