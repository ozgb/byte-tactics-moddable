"""Make the digest of a display list and compare display lists and digests (docs/display-log.md), standard library only.

    uv run tools/displaylog.py digest displaylist.txt                           # print the digest of every frame the list holds
    uv run tools/displaylog.py digest displaylist.txt --against display.txt    # compare it with a digest (exit 0 equal, 1 differs)
    uv run tools/displaylog.py digest displaylist.txt --against display.txt --end-of-tick
    uv run tools/displaylog.py compare a/displaylist.txt b/displaylist.txt     # first frame and first item that differ
    uv run tools/displaylog.py compare a.txt b.txt --end-of-tick               # items compared as sorted `<layer> <item>` lines
    uv run tools/displaylog.py against-digests a/display.txt b/display.txt     # compare two digest files

A display list (`bt-display 1.1`) holds frames: a frame line, a `picture` line for each picture the game made that the list
names for the first time, one line for each item and a cursor line. An item whose area
is empty changes no pixel and is left out when a list is read, as the game leaves it out of its log ("Draws that change no
pixel"), so a list that holds such items has the digest of the same list without them. The layer of an item is computed
from the lines alone ("Which orders count"): 0 when it depends on no item before it, else one more than the highest layer
of the items before it that it depends on. Two items depend on each other when they are on the same target with
overlapping areas, or when one reads a target (`blit`, `copy`, a picture named `target:<name>`) and the other draws on
that target.

The digest (`bt-display-digest 1`) has a line `<number> <tick> <count> <ordered> <set>` for each frame: `ordered` is the
hash of the frame's lines in order, and `set` the hash of the frame line, the sorted `<layer> <item>` lines and the cursor
line. With --end-of-tick only the frame numbers, ticks, counts and `set` are compared, and items as sorted layered lines.
Exit status: 0 equal, 1 differs, 2 error.
"""

import argparse
import sys
from pathlib import Path

DIGEST_HEADER = "bt-display-digest 1"
LOG_HEADER = "bt-display 1.1"
DIGEST_LOG_LINE = "log bt-display 1.1"
OFFSET_BASIS = 0xCBF29CE484222325
PRIME = 0x100000001B3
MASK64 = (1 << 64) - 1
TARGET_PREFIX = "target:"
PICTURE_KINDS = frozenset({"sprite", "sprite_opaque", "sprite_blend", "sprite_gray", "sprite_shadow", "sprite_lit",
                           "sprite_erase", "lens", "quad"})
SOURCE_KINDS = frozenset({"blit", "copy"})
KINDS = ("sprite", "sprite_opaque", "sprite_blend", "sprite_gray", "sprite_shadow", "sprite_lit", "sprite_erase", "lens", "quad",
         "blit", "copy", "clear", "fill", "fade", "gray", "dither", "outline", "outline_lit", "line", "pixel", "circle",
         "circle_dashed", "polygon", "text", "terrain", "pixels", "model", "cargo", "projectile", "debris", "shard")
CELL_SHIFT = 6
MAX_CELLS = 64


class Error(Exception):
    pass


def hash_bytes(data: bytes) -> int:
    """HB(data): the 64-bit hash of docs/trace-format.md, section 3, applied once to the bytes."""
    h = OFFSET_BASIS
    n = len(data)
    i = 0
    while n - i >= 8:
        h = ((h ^ int.from_bytes(data[i:i + 8], "little")) * PRIME) & MASK64
        i += 8
    while i < n:
        h = ((h ^ data[i]) * PRIME) & MASK64
        i += 1
    return h


class Item:
    """One item line of a frame: its line number, text, kind, target, area and the target it reads (None when it reads none)."""

    def __init__(self, number: int, line: str, kind: str, target: str, area: tuple[int, int, int, int], reads: str | None) -> None:
        self.number = number
        self.line = line
        self.kind = kind
        self.target = target
        self.area = area
        self.reads = reads
        self.layer = 0


class Frame:
    """One frame of a display list: the frame line, its items, the cursor line and the line number of each."""

    def __init__(self, number: int, tick: int, line_number: int, line: str) -> None:
        self.number = number
        self.tick = tick
        self.line_number = line_number
        self.line = line
        self.items: list[Item] = []
        self.pictures: list[tuple[int, str]] = []
        self.cursor = ""
        self.cursor_number = 0


def parse_item(number: int, line: str) -> Item:
    words = line.split(" ")
    if len(words) < 6:
        raise Error(f"line {number} has no kind, target and area: `{line}`")
    try:
        area = (int(words[2]), int(words[3]), int(words[4]), int(words[5]))
    except ValueError:
        raise Error(f"line {number} has an area that is not four numbers: `{line}`") from None
    kind = words[0]
    reads = None
    if kind in SOURCE_KINDS or kind in PICTURE_KINDS:
        if len(words) < 7:
            raise Error(f"line {number} has no source or picture: `{line}`")
        if kind in SOURCE_KINDS:
            reads = words[6]
        elif words[6].startswith(TARGET_PREFIX):
            reads = words[6][len(TARGET_PREFIX):]
    return Item(number, line, kind, words[1], area, reads)


class Picture:
    """A picture the game made, from its `picture` line: its name, offsets, transparent index, size and pixels."""

    def __init__(self, name: str, x_offset: int, y_offset: int, transparency: int, width: int, height: int, pixels: bytes) -> None:
        self.name = name
        self.x_offset = x_offset
        self.y_offset = y_offset
        self.transparency = transparency
        self.width = width
        self.height = height
        self.pixels = pixels


def parse_picture(number: int, line: str) -> Picture:
    """The picture of a `picture <name> <x offset> <y offset> <transparent index> <pixels>` line; an Error when it is not well formed."""
    words = line.split(" ")
    if len(words) != 6 or words[1][:1] not in ("@", "?"):
        raise Error(f"line {number} is not a picture line: `{line[:200]}`")
    try:
        size = words[1].lstrip("?").split(":")[-2]
        width, height = (int(v) for v in size.split("x"))
        x_offset, y_offset, transparency = int(words[2]), int(words[3]), int(words[4])
        pixels = bytes.fromhex(words[5])
    except (IndexError, ValueError):
        raise Error(f"line {number} is not a picture line: `{line[:200]}`") from None
    if width <= 0 or height <= 0 or len(pixels) != width * height or not 0 <= transparency <= 255:
        raise Error(f"line {number}: the pixels of {words[1]} do not fit its size")
    if words[1].split(":")[-1] != f"{hash_bytes(pixels):016x}":
        raise Error(f"line {number}: the pixels of {words[1]} do not have its hash")
    return Picture(words[1], x_offset, y_offset, transparency, width, height, pixels)


def parse_list(text: str) -> list[Frame]:
    """The frames of a display list (layers not yet computed), without the items whose area is empty; an Error for a list that is not well formed."""
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    if not lines or lines[0] != LOG_HEADER:
        raise Error(f"not a display list: the first line is not `{LOG_HEADER}`")
    frames: list[Frame] = []
    current: Frame | None = None
    for number, line in enumerate(lines[1:], 2):
        words = line.split(" ")
        if words[0] == "frame":
            if current is not None:
                raise Error(f"line {number}: the frame at line {current.line_number} has no cursor line")
            if len(words) != 5 or not words[1].isdigit() or not words[2].isdigit() or not words[3].isdigit():
                raise Error(f"line {number} is not a frame line: `{line}`")
            current = Frame(int(words[1]), int(words[2]), number, line)
        elif current is None:
            raise Error(f"line {number} is outside a frame: `{line}`")
        elif words[0] == "picture":
            if current.items:
                raise Error(f"line {number}: a picture line after an item of its frame")
            parse_picture(number, line)
            current.pictures.append((number, line))
        elif words[0] == "cursor":
            current.cursor = line
            current.cursor_number = number
            frames.append(current)
            current = None
        else:
            item = parse_item(number, line)
            if nonempty(item.area):
                current.items.append(item)
    if current is not None:
        raise Error(f"the frame at line {current.line_number} has no cursor line")
    return frames


def pictures(text: str) -> dict[str, Picture]:
    """Every picture a display list's `picture` lines define, by name (a later line for a name replaces an earlier one)."""
    found: dict[str, Picture] = {}
    for number, line in enumerate(text.split("\n"), 1):
        if line.startswith("picture "):
            picture = parse_picture(number, line)
            found[picture.name] = picture
    return found


def kinds(text: str) -> dict[str, int]:
    """How many items of each kind a display list holds, items with an empty area included."""
    found: dict[str, int] = {}
    for line in text.split("\n")[1:]:
        kind = line.split(" ", 1)[0]
        if kind and kind not in ("frame", "cursor", "picture"):
            found[kind] = found.get(kind, 0) + 1
    return found


def overlaps(a: tuple[int, int, int, int], b: tuple[int, int, int, int]) -> bool:
    return a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]


def nonempty(area: tuple[int, int, int, int]) -> bool:
    return area[0] < area[2] and area[1] < area[3]


def scan(by_layer: list[list[tuple[int, int, int, int]]], area: tuple[int, int, int, int], best: int) -> int:
    """The highest layer above `best` that holds an area overlapping `area`, else `best`."""
    for layer in range(len(by_layer) - 1, best, -1):
        for other in by_layer[layer]:
            if overlaps(area, other):
                return layer
    return best


class Surface:
    """The areas drawn so far on one target, by layer, in a grid of cells so that a query looks at the neighbours only."""

    def __init__(self) -> None:
        self.cells: dict[tuple[int, int], list[list[tuple[int, int, int, int]]]] = {}
        self.big: list[list[tuple[int, int, int, int]]] = []
        self.everything: list[list[tuple[int, int, int, int]]] = []
        self.max_draw = -1
        self.max_read = -1

    @staticmethod
    def span(area: tuple[int, int, int, int]) -> tuple[int, int, int, int]:
        return area[0] >> CELL_SHIFT, area[1] >> CELL_SHIFT, (area[2] - 1) >> CELL_SHIFT, (area[3] - 1) >> CELL_SHIFT

    @staticmethod
    def put(by_layer: list[list[tuple[int, int, int, int]]], layer: int, area: tuple[int, int, int, int]) -> None:
        while len(by_layer) <= layer:
            by_layer.append([])
        by_layer[layer].append(area)

    def add(self, area: tuple[int, int, int, int], layer: int) -> None:
        self.put(self.everything, layer, area)
        self.max_draw = max(self.max_draw, layer)
        cx0, cy0, cx1, cy1 = self.span(area)
        if (cx1 - cx0 + 1) * (cy1 - cy0 + 1) > MAX_CELLS:
            self.put(self.big, layer, area)
            return
        for cy in range(cy0, cy1 + 1):
            for cx in range(cx0, cx1 + 1):
                self.put(self.cells.setdefault((cx, cy), []), layer, area)

    def query(self, area: tuple[int, int, int, int]) -> int:
        """The highest layer of the areas drawn so far that overlap `area`, or -1."""
        cx0, cy0, cx1, cy1 = self.span(area)
        if (cx1 - cx0 + 1) * (cy1 - cy0 + 1) > MAX_CELLS:
            return scan(self.everything, area, -1)
        best = scan(self.big, area, -1)
        for cy in range(cy0, cy1 + 1):
            for cx in range(cx0, cx1 + 1):
                cell = self.cells.get((cx, cy))
                if cell is not None:
                    best = scan(cell, area, best)
        return best


def assign_layers(items: list[Item]) -> None:
    """Set the `layer` of each item of one frame, from the lines alone (docs/display-log.md, "Which orders count")."""
    surfaces: dict[str, Surface] = {}

    def surface(name: str) -> Surface:
        if name not in surfaces:
            surfaces[name] = Surface()
        return surfaces[name]

    for item in items:
        mine = surface(item.target)
        depends = max(mine.query(item.area), mine.max_read)
        if item.reads is not None:
            depends = max(depends, surface(item.reads).max_draw)
        item.layer = depends + 1
        mine.add(item.area, item.layer)
        if item.reads is not None:
            source = surface(item.reads)
            source.max_read = max(source.max_read, item.layer)


def parse_layered(text: str) -> list[Frame]:
    """The frames of a display list with the layer of each item set."""
    frames = parse_list(text)
    for frame in frames:
        assign_layers(frame.items)
    return frames


def ordered_bytes(frame: Frame) -> bytes:
    lines = [frame.line, *(item.line for item in frame.items), frame.cursor]
    return "".join(line + "\n" for line in lines).encode("latin-1")


def layered_lines(frame: Frame) -> list[tuple[str, int]]:
    """The `<layer> <item>` lines of a frame with their line numbers, sorted bytewise."""
    return sorted((f"{item.layer} {item.line}", item.number) for item in frame.items)


def set_bytes(frame: Frame) -> bytes:
    lines = [frame.line, *(text for text, _ in layered_lines(frame)), frame.cursor]
    return "".join(line + "\n" for line in lines).encode("latin-1")


def frame_digest(frame: Frame) -> str:
    return f"{frame.number} {frame.tick} {len(frame.items)} {hash_bytes(ordered_bytes(frame)):016x} {hash_bytes(set_bytes(frame)):016x}"


def digest_text(text: str) -> str:
    """The digest of a display list: two header lines and a line for each frame it holds."""
    out = [DIGEST_HEADER, DIGEST_LOG_LINE]
    out += [frame_digest(frame) for frame in parse_layered(text)]
    return "\n".join(out) + "\n"


def line_frame(line: str) -> tuple[int, int] | None:
    """The (frame number, tick) that a digest line starts with, or None."""
    words = line.split(" ")
    if len(words) >= 2 and words[0].isdigit() and words[1].isdigit():
        return int(words[0]), int(words[1])
    return None


def line_where(line: str) -> str:
    frame = line_frame(line)
    return f" (frame {frame[0]}, tick {frame[1]})" if frame else ""


def kept_lines(text: str, end_of_tick: bool) -> list[tuple[int, str, tuple[str, ...]]]:
    """(line number, line, what is compared) for each line of a digest; with `end_of_tick` the `ordered` field is left out."""
    kept = []
    for number, line in enumerate(text.splitlines(), 1):
        words = tuple(line.split(" "))
        if end_of_tick and number > 2 and len(words) == 5:
            words = words[:3] + words[4:]
        kept.append((number, line, words if end_of_tick else (line,)))
    return kept


def first_digest_difference(stored: str, got: str, end_of_tick: bool = False) -> tuple[int, str, str, str] | None:
    """(line number, stored line, got line, what it is) of the first line where two digests differ, or None.

    A line only one digest has is given with an empty line for the other, and `only the stored` or `only the got` as what it is."""
    want, have = kept_lines(stored, end_of_tick), kept_lines(got, end_of_tick)
    for (number, a, wa), (_, b, wb) in zip(want, have):
        if wa != wb:
            return number, a, b, "differs"
    if len(want) != len(have):
        number, line, _ = (want if len(want) > len(have) else have)[min(len(want), len(have))]
        return (number, line, "", "only the stored") if len(want) > len(have) else (number, "", line, "only the got")
    return None


def digest_difference(stored: str, got: str, end_of_tick: bool = False) -> str | None:
    """Where two digests first differ (the line number, its frame and tick, both lines), or None when they are equal."""
    found = first_digest_difference(stored, got, end_of_tick)
    if found is None:
        return None
    number, a, b, what = found
    where = line_where(a) or line_where(b)
    if what == "differs":
        return f"display digest line {number}{where}: stored `{a}`, got `{b}`"
    return f"display digest line {number}{where}: {what} digest has it (`{a or b}`)"


def digest_frame(stored: str, got: str, end_of_tick: bool = False) -> tuple[int, int] | None:
    """The (frame number, tick) of the first line where two digests differ, or None when they are equal or that line is no frame line."""
    found = first_digest_difference(stored, got, end_of_tick)
    return line_frame(found[1] or found[2]) if found else None


def frame_difference(a: Frame, b: Frame, end_of_tick: bool) -> str | None:
    """Where two frames of one number first differ (the first item by line number, both lines), or None when equal."""
    where = f"frame {a.number} (tick {a.tick})"
    if a.line != b.line:
        return f"{where}: frame line {a.line_number} of A `{a.line}`, line {b.line_number} of B `{b.line}`"
    for (nx, x), (ny, y) in zip(a.pictures, b.pictures):
        if x != y:
            return f"{where}: picture line {nx} of A `{x[:120]}`, line {ny} of B `{y[:120]}`"
    if len(a.pictures) != len(b.pictures):
        longer, which = (a.pictures, "A") if len(a.pictures) > len(b.pictures) else (b.pictures, "B")
        number, text = longer[min(len(a.pictures), len(b.pictures))]
        return f"{where}: only {which} has the picture line {number} (`{text[:120]}`)"
    if end_of_tick:
        one, two = layered_lines(a), layered_lines(b)
        for (x, nx), (y, ny) in zip(one, two):
            if x != y:
                return f"{where}: first item in layer order that differs: line {nx} of A `{x}`, line {ny} of B `{y}`"
        if len(one) != len(two):
            longer, which = (one, "A") if len(one) > len(two) else (two, "B")
            text, number = longer[min(len(one), len(two))]
            return f"{where}: only {which} has the item at line {number} (`{text}`) in layer order"
    else:
        for x, y in zip(a.items, b.items):
            if x.line != y.line:
                return f"{where}: item at line {x.number} of A `{x.line}`, line {y.number} of B `{y.line}`"
        if len(a.items) != len(b.items):
            longer, which = (a.items, "A") if len(a.items) > len(b.items) else (b.items, "B")
            extra = longer[min(len(a.items), len(b.items))]
            return f"{where}: only {which} has the item at line {extra.number} (`{extra.line}`)"
    if a.cursor != b.cursor:
        return f"{where}: cursor line {a.cursor_number} of A `{a.cursor}`, line {b.cursor_number} of B `{b.cursor}`"
    return None


def list_difference(a: str, b: str, end_of_tick: bool = False) -> str | None:
    """Where two display lists first differ (the first frame, and in it the first item), or None when they are the same.

    With `end_of_tick` the items of a frame are compared as sorted `<layer> <item>` lines."""
    one = {f.number: f for f in parse_layered(a)}
    two = {f.number: f for f in parse_layered(b)}
    for number in sorted(one.keys() | two.keys()):
        if number not in two:
            return f"frame {number} (tick {one[number].tick}): only A has it"
        if number not in one:
            return f"frame {number} (tick {two[number].tick}): only B has it"
        diff = frame_difference(one[number], two[number], end_of_tick)
        if diff:
            return diff
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)
    d = sub.add_parser("digest", help="print the digest of a display list, or compare it with a stored digest")
    d.add_argument("log", help="a display list (displaylist.txt)")
    d.add_argument("--against", metavar="DIGEST", help="compare with this stored digest instead of printing")
    d.add_argument("--end-of-tick", action="store_true", help="with --against: compare numbers, ticks, counts and `set` only")
    c = sub.add_parser("compare", help="compare two display lists: the first frame and item that differ")
    c.add_argument("a")
    c.add_argument("b")
    c.add_argument("--end-of-tick", action="store_true", help="compare the items of a frame as sorted `<layer> <item>` lines")
    g = sub.add_parser("against-digests", help="compare two digest files")
    g.add_argument("a")
    g.add_argument("b")
    g.add_argument("--end-of-tick", action="store_true", help="compare numbers, ticks, counts and `set` only")
    args = ap.parse_args()
    if args.command == "digest" and args.end_of_tick and args.against is None:
        ap.error("--end-of-tick needs --against")
    try:
        if args.command == "digest":
            text = digest_text(Path(args.log).read_text(encoding="latin-1"))
            if args.against is None:
                sys.stdout.write(text)
                return 0
            diff = digest_difference(Path(args.against).read_text(encoding="latin-1"), text, args.end_of_tick)
            print(diff or f"digests equal ({len(text.splitlines()) - 2} frames)")
        elif args.command == "compare":
            diff = list_difference(Path(args.a).read_text(encoding="latin-1"), Path(args.b).read_text(encoding="latin-1"), args.end_of_tick)
            print(diff or "display lists equal" + (" up to the order that does not count" if args.end_of_tick else ""))
        else:
            diff = digest_difference(Path(args.a).read_text(encoding="latin-1"), Path(args.b).read_text(encoding="latin-1"), args.end_of_tick)
            print(diff or "digests equal")
    except (Error, OSError) as e:
        print(f"displaylog: {e}", file=sys.stderr)
        return 2
    return 1 if diff else 0


if __name__ == "__main__":
    sys.exit(main())
