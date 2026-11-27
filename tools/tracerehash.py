"""Recompute the rows of a trace.txt from a dump.txt, following docs/trace-format.md only.

    uv run tools/tracerehash.py dump.txt [--schema data/trace_schema.json] [--trace trace.txt] [--views DIR]

Prints `<tick> <phase> <hash>` for every phase row and the world row of each dumped tick, and the `tick` row of a tick
whose dump has a `tick` block: the phase rows' hash continued, without a reset, over the world tables. The `view` row is not in a dump:
it is rehashed from the view file of its tick (`<DIR>/<tick>.bin`, the file the run writes on the dump tick; DIR defaults to `views/` beside the
dump), as the trace hash of the serialisation S (tools/viewblock.py, docs/view.md "The recorded view"), for each dumped tick that has one. With
--trace, each row is followed by `ok` or `differs` against the same row of the trace, and the exit
status is 1 if any row differs or the input cannot be parsed. A path ending in .xz is read compressed. Dumps of tick 1 also give the
sections 29 to 31 in the world row. Sections 24 and 25 are the records los_radar and units write through the state writer (`losRadarIndex[i].*`, `unitsFactories[i].*`). The pathfinder's own report (sections 16 and 17) is read from the dump and checked for
shape but not hashed; economy's report (`economyState.*`), ai's report (`aiState.*`), map's report (`mapState.*`), players' report (`playersState.*`), weapons' report (`weaponsState.*`), orders' report (`ordersState.*`), movement's report (`movementState.*`), units' report (`unitsState.*`), scripts' report (`scriptsState.*`), los_radar's sight cache and eyes (`losRadarState.*`), the path results (section 32), the economy events
(section 33), the share packets (section 35), wind_meteors' events (section 37), targeting's events (section 38), los_radar's events (section 39), map's events (section 40), players' events (section 41), ai's events (section 42), weapons' events (section 47), orders' events (section 48), movement's events (section 49), units' events (section 50) and scripts' events (section 51) are dumped only, and skipped.
"""

import argparse
import json
import lzma
import re
import sys
from pathlib import Path

import viewblock

OFFSET_BASIS = 0xCBF29CE484222325
PRIME = 0x100000001B3
MASK64 = (1 << 64) - 1
END = 0xFFFFFFFF
PHASES = ("net", "units", "projectiles", "explosions", "players", "features", "wind", "meteors", "tick")
ROW_ORDER = (*PHASES[:-1], "world", "tick", "view")
POS_Y_UNSET_KINDS = ("MobileBuild", "VTOL_MobileBuild", "Reclaim", "VTOL_Reclaim")
DUMPED_ONLY = ("results.", "results[", "economy.", "economy[", "features.", "features[", "packets.", "packets[",
               "windMeteors.", "windMeteors[", "targeting.", "targeting[", "los_radar.", "los_radar[", "map.", "map[",
               "playersEvents.", "playersEvents[", "aiEvents.", "aiEvents[", "weaponsEvents.", "weaponsEvents[",
               "ordersEvents.", "ordersEvents[", "movementEvents.", "movementEvents[",
               "unitsEvents.", "unitsEvents[", "scriptsEvents.", "scriptsEvents[")


class Error(Exception):
    pass


def read_text(path: str) -> str:
    if path.endswith(".xz"):
        with lzma.open(path, "rt", encoding="latin-1", newline="\n") as f:
            return f.read()
    return Path(path).read_text(encoding="latin-1")


class Hasher:
    def __init__(self) -> None:
        self.h = OFFSET_BASIS

    def reset(self) -> None:
        self.h = OFFSET_BASIS

    def run(self, data: bytes) -> None:
        h = self.h
        n = len(data)
        i = 0
        while n - i >= 8:
            w = int.from_bytes(data[i : i + 8], "little")
            h = ((h ^ w) * PRIME) & MASK64
            i += 8
        while i < n:
            h = ((h ^ data[i]) * PRIME) & MASK64
            i += 1
        self.h = h

    def h8(self, x: int) -> None:
        self.run(bytes([x & 0xFF]))

    def h32(self, x: int) -> None:
        for shift in (0, 8, 16, 24):
            self.run(bytes([(x >> shift) & 0xFF]))

    def end(self) -> None:
        self.h32(END)

    def pointer(self, tag: int, index: int) -> None:
        self.h8(tag)
        self.h32(index)


def field_name(field: dict) -> str:
    return field["name"] or "+0x%x" % field["offset"]


def parse_dump(text: str, trace_format: int) -> dict[tuple[int, str], list[tuple[str, str]]]:
    """The (path, value) lines of each (tick, phase) block of a bt-dump <trace_format> dump, in file order."""
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    if not lines or lines[0] != f"bt-dump {trace_format}":
        raise Error(f"not a bt-dump {trace_format} file")
    blocks: dict[tuple[int, str], list[tuple[str, str]]] = {}
    for n, line in enumerate(lines[1:], 2):
        parts = line.split(" ", 3)
        if len(parts) < 3 or not parts[0].isdigit() or parts[1] not in PHASES:
            raise Error(f"dump line {n}: cannot parse")
        value = parts[3] if len(parts) == 4 else ""
        blocks.setdefault((int(parts[0]), parts[1]), []).append((parts[2], value))
    return blocks


def parse_blocks(text: str, trace_format: int) -> list[tuple[tuple[int, str], list[tuple[str, str]]]]:
    """The blocks of a dump in file order: each a run of consecutive lines of one (tick, phase), so a block repeated
    later in the file is a block of its own."""
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    if not lines or lines[0] != f"bt-dump {trace_format}":
        raise Error(f"not a bt-dump {trace_format} file")
    blocks: list[tuple[tuple[int, str], list[tuple[str, str]]]] = []
    for n, line in enumerate(lines[1:], 2):
        parts = line.split(" ", 3)
        if len(parts) < 3 or not parts[0].isdigit() or parts[1] not in PHASES:
            raise Error(f"dump line {n}: cannot parse")
        key = (int(parts[0]), parts[1])
        if not blocks or blocks[-1][0] != key:
            blocks.append((key, []))
        blocks[-1][1].append((parts[2], parts[3] if len(parts) == 4 else ""))
    return blocks


FX_CLASS = re.compile(r"orderFx\[\d+\]\.class")
CONDITION_CLASS = re.compile(r"playersState\.conditions\.(victory|defeat)\[\d+\]\.class")


def check_failures(blocks) -> None:
    """Refuse a dump that holds, in any block, an order effect of a class above 5 or a mission condition of class 0
    (docs/trace-format.md, section 11): the reference fails the run on each, so neither has a hash."""
    for (tick, phase), lines in blocks:
        for path, value in lines:
            fx, condition = FX_CLASS.fullmatch(path), CONDITION_CLASS.fullmatch(path)
            if not (fx or condition):
                continue
            try:
                number = int(value, 16)
            except ValueError:
                raise Error(f"{tick} {phase} {path}: {value!r} is not a hex value") from None
            if fx and number > 5:
                raise Error(f"{tick} {phase} {path}: an order effect of class {number} fails the run")
            if condition and number == 0:
                raise Error(f"{tick} {phase} {path}: a mission condition of class 0 fails the run")


def parse_trace_rows(text: str) -> dict[tuple[int, str], str]:
    rows: dict[tuple[int, str], str] = {}
    for line in text.splitlines():
        words = line.split()
        if len(words) == 3 and words[0].isdigit() and re.fullmatch(r"[0-9a-f]{16}", words[2]):
            rows[(int(words[0]), words[1])] = words[2]
    return rows


class Schema:
    def __init__(self, data: dict) -> None:
        self.data = data
        self.tags = {t["name"]: t["tag"] for t in data["targets"]}
        self.tables = data["tables"]

    def fields(self, table: str) -> list[dict]:
        return self.tables[table]["fields"]

    def plan(self, table: str) -> list[dict]:
        return self.tables[table]["plan"]


def value_bytes(field: dict, value: str) -> bytes:
    if value.startswith("hex:"):
        raw = bytes.fromhex(value[4:])
        return raw
    if value.startswith("0x"):
        return int(value, 16).to_bytes(field["size"], "little")
    raise Error(f"field {field_name(field)}: cannot read value {value!r}")


class Walker:
    def __init__(self, schema: Schema, lines: list[tuple[str, str]]) -> None:
        self.schema = schema
        self.lines = lines
        self.pos = 0
        self.hash = Hasher()
        self.live: list[int] = []
        self.scripted: set[int] = set()
        self.fx_count = 0

    def skip_dumped(self, prefixes: tuple[str, ...]) -> None:
        while (self.peek() or "").startswith(prefixes):
            self.pos += 1

    def peek(self) -> str | None:
        return self.lines[self.pos][0] if self.pos < len(self.lines) else None

    def take(self, path: str) -> str:
        if self.pos >= len(self.lines):
            raise Error(f"dump ended where {path} was expected")
        got, value = self.lines[self.pos]
        if got != path:
            raise Error(f"expected {path}, found {got} (line {self.pos} of the block)")
        self.pos += 1
        return value

    def take_if(self, path: str) -> str | None:
        return self.take(path) if self.peek() == path else None

    def next_index(self, pattern: str) -> int | None:
        path = self.peek()
        if path is None:
            return None
        m = re.fullmatch(pattern, path)
        return int(m.group(1)) if m else None

    def first_path(self, table: str, prefix: str) -> str:
        return f"{prefix}.{field_name(self.schema.fields(table)[0])}"

    def present(self, table: str, prefix: str) -> bool:
        return self.peek() == self.first_path(table, prefix)

    def pointer_id(self, value: str) -> tuple[int, int]:
        if value == "null":
            return 0, 0
        if value == "heap":
            return 1, 0
        if value == "rec:overflow":
            return 9, END
        name, _, index = value.partition(":")
        if name not in self.schema.tags or not index.isdigit():
            raise Error(f"cannot read pointer {value!r}")
        return self.schema.tags[name], int(index)

    def hash_field(self, field: dict, value: str) -> None:
        if field["kind"] in ("pointer", "ref"):
            self.hash.pointer(*self.pointer_id(value))
        elif "mask" in field:
            self.hash.h32(int(value, 16) & field["mask"])
        else:
            self.hash.run(value_bytes(field, value))

    def read_table(self, table: str, prefix: str) -> dict[int, str]:
        values = {}
        for f in self.schema.fields(table):
            values[f["offset"]] = self.take(f"{prefix}.{field_name(f)}")
        return values

    def table(self, table: str, prefix: str) -> dict[int, str]:
        return self.hash_table(table, self.read_table(table, prefix), prefix)

    def hash_table(self, table: str, values: dict[int, str], prefix: str) -> dict[int, str]:
        by_offset = {f["offset"]: f for f in self.schema.fields(table)}
        for entry in self.schema.plan(table):
            if entry["kind"] in ("pointer", "ref") or "mask" in entry:
                self.hash_field(by_offset[entry["offset"]], values[entry["offset"]])
                continue
            lo, hi = entry["offset"], entry["offset"] + entry["size"]
            raw = b""
            for f in sorted(self.schema.fields(table), key=lambda f: f["offset"]):
                if f["offset"] >= lo and f["offset"] + f["size"] <= hi and f["kind"] not in ("pointer", "ref") and "mask" not in f:
                    raw += value_bytes(f, values[f["offset"]])
            if len(raw) != entry["size"]:
                raise Error(f"{table} at {prefix}: plan entry at {lo} has {len(raw)} bytes, expected {entry['size']}")
            self.hash.run(raw)
        return values

    def flat_list(self, fields: list[dict], prefix: str) -> None:
        for f in fields:
            self.hash_field(f, self.take(f"{prefix}{field_name(f)}"))

    def hex_run(self, path: str) -> bytes:
        value = self.take(path)
        if not value.startswith("hex:"):
            raise Error(f"{path}: expected hex:, found {value!r}")
        return bytes.fromhex(value[4:])

    def collect_run(self, pattern: str, width: int | None = None) -> bytes:
        out = b""
        while self.next_index(pattern) is not None:
            _, value = self.lines[self.pos]
            self.pos += 1
            if value.startswith("hex:"):
                out += bytes.fromhex(value[4:])
            elif value.startswith("0x"):
                out += int(value, 16).to_bytes(width or (len(value) - 2) // 2, "little")
            else:
                raise Error(f"cannot read value {value!r}")
        return out

    def phase_row(self, tick: int) -> int:
        h = self.hash
        h.reset()
        h.h32(int(self.take("gameTick"), 16))
        h.h32(int(self.take("rng.parkMiller"), 16))
        h.h32(int(self.take("rng.crtHoldrand"), 16))
        self.units()
        self.projectiles()
        self.players()
        self.features()
        self.wind()
        self.meteors()
        return h.h

    def indexed_tables(self, table: str, pattern: str, prefix_format: str) -> None:
        while True:
            i = self.next_index(pattern)
            if i is None:
                return
            self.hash.h32(i)
            self.table(table, prefix_format % i)

    def units(self) -> None:
        self.hash.h8(1)
        first = re.escape(field_name(self.schema.fields("Unit")[0]))
        self.indexed_tables("Unit", rf"units\[(\d+)\]\.{first}", "units[%d]")
        self.hash.end()

    def projectiles(self) -> None:
        self.hash.h8(2)
        first = re.escape(field_name(self.schema.fields("Projectile")[0]))
        self.indexed_tables("Projectile", rf"projectiles\[(\d+)\]\.{first}", "projectiles[%d]")
        self.hash.end()

    def players(self) -> None:
        self.hash.h8(6)
        for i in range(11):
            self.hash.h32(i)
            self.table("Player", f"players[{i}]")
        self.hash.end()

    def features(self) -> None:
        h = self.hash
        h.h8(7)
        while (i := self.next_index(r"features\[(\d+)\]\.list")) is not None:
            which = int(self.take(f"features[{i}].list"), 16)
            h.h32(i)
            h.h8(which)
            prefix = f"features[{i}]"
            for name in ("SpotAnim", "SpotObject"):
                want = [f"{prefix}.{field_name(f)}" for f in self.schema.fields(name)]
                got = [p for p, _ in self.lines[self.pos : self.pos + len(want)]]
                if want == got:
                    self.table(name, prefix)
                    break
            else:
                raise Error(f"{prefix}: fields match neither SpotAnim nor SpotObject")
        h.end()

    def wind(self) -> None:
        self.hash.h8(8)
        self.flat_list(self.schema.data["wind"], "wind.")
        self.hash.end()

    def meteors(self) -> None:
        self.hash.h8(9)
        self.flat_list(self.schema.data["globals"]["meteors"], "meteors.")
        self.hash.end()

    def records(self, pattern: str) -> list[int]:
        found = []
        for path, _ in self.lines:
            m = re.fullmatch(pattern, path)
            if m:
                found.append(int(m.group(1)))
        return found

    def world_row(self, tick: int = 0, reset: bool = True) -> int:
        if reset:
            self.hash.reset()
        self.live = self.records(r"unitsState\.units\[(\d+)\]\.head")
        self.scripted = set(self.records(r"scriptsState\.units\[(\d+)\]\.scale"))
        self.game()
        self.cells()
        self.visibility()
        self.explored()
        self.spatial()
        self.pathfinder()
        self.move_classes()
        self.orders()
        self.motions()
        self.order_effects()
        self.scripts()
        self.los_radar_index()
        self.factory_index()
        self.unit_resources()
        if tick == 1:
            self.unit_definitions()
            self.weapon_definitions()
            self.mission()
        self.skip_dumped(DUMPED_ONLY)
        if self.pos != len(self.lines):
            raise Error(f"{len(self.lines) - self.pos} dump lines left after the world row, from {self.lines[self.pos][0]}")
        return self.hash.h

    def game(self) -> None:
        self.hash.h8(10)
        self.flat_list(self.schema.data["game_scalars"], "game.")
        self.flat_list(self.schema.data["globals"]["game"], "game.")
        self.hash.end()

    def cells(self) -> None:
        h = self.hash
        h.h8(11)
        h.h32(int(self.take("cells.count"), 16))
        h.run(self.collect_run(r"cells\[(\d+)\]"))
        h.end()

    def visibility(self) -> None:
        h = self.hash
        h.h8(12)
        h.h32(int(self.take("visibility.count"), 16))
        h.run(self.collect_run(r"visibility\[(\d+)\]"))
        h.end()

    def explored(self) -> None:
        h = self.hash
        h.h8(13)
        for who in range(10):
            h.h32(int(self.take(f"players[{who}].explored"), 16))
            h.run(self.collect_run(rf"players\[{who}\]\.explored\[(\d+)\]", 1))
        h.end()

    def spatial(self) -> None:
        h = self.hash
        h.h8(14)
        for name in ("width", "height", "count"):
            h.h32(int(self.take(f"spatial.{name}"), 16))
        first = re.escape(field_name(self.schema.fields("Rec")[0]))
        while (r := self.next_index(rf"spatial\[(\d+)\]\.{first}")) is not None:
            self.table("Rec", f"spatial[{r}]")
        overflow = self.present("Rec", "spatial.overflow")
        h.h8(int(overflow))
        if overflow:
            self.table("Rec", "spatial.overflow")
        h.end()

    def pathfinder(self) -> None:
        hashed = self.hash
        self.hash = Hasher()
        try:
            self.pathfinder_report()
        finally:
            self.hash = hashed
        self.skip_dumped(("economyState.", "aiState.", "mapState.", "playersState.", "weaponsState.",
                           "ordersState.", "movementState.", "scriptsState."))

    def pathfinder_report(self) -> None:
        h = self.hash
        h.h8(16)
        present = self.present("Pathfinder", "pathfinder")
        h.h8(int(present))
        if present:
            self.table("Pathfinder", "pathfinder")
            if self.present("PathfinderSearch", "pathfinder"):
                self.table("PathfinderSearch", "pathfinder")
            self.pathfinder_nodes()
            self.pathfinder_cells()
        h.end()
        h.h8(17)
        globals_ = self.schema.data["globals"]["pathfinder"]
        if globals_ and self.peek() == "pathfinder." + field_name(globals_[0]):
            self.flat_list(globals_, "pathfinder.")
        h.end()

    def pathfinder_nodes(self) -> None:
        h = self.hash
        first = re.escape(field_name(self.schema.fields("Node")[0]))
        while (node := self.next_index(rf"pathfinder\.pool\[(\d+)\]\.{first}")) is not None:
            h.h32(node)
            self.table("Node", f"pathfinder.pool[{node}]")
            penalty = self.take_if(f"pathfinder.pool[{node}].penalty")
            if penalty is not None:
                h.run(int(penalty, 16).to_bytes(2, "little"))
        h.h32(int(self.take("pathfinder.heap.count"), 16))
        while self.next_index(r"pathfinder\.heap\[(\d+)\]") is not None:
            _, value = self.lines[self.pos]
            self.pos += 1
            h.pointer(*self.pointer_id(value))

    def pathfinder_cells(self) -> None:
        h = self.hash
        while (w := self.next_index(r"pathfinder\.dirty\[(\d+)\]")) is not None:
            bits = int(self.take(f"pathfinder.dirty[{w}]"), 16)
            h.h32(bits)
            while (index := self.next_index(r"pathfinder\.cells\[(\d+)\]\.flags")) is not None:
                h.run(int(self.take(f"pathfinder.cells[{index}].flags"), 16).to_bytes(1, "little"))
                direction = self.take_if(f"pathfinder.cells[{index}].dir")
                if direction is not None:
                    h.run(int(direction, 16).to_bytes(1, "little"))
                node = self.take_if(f"pathfinder.cells[{index}].node")
                if node is not None:
                    h.run(int(node, 16).to_bytes(2, "little"))

    def move_classes(self) -> None:
        h = self.hash
        h.h8(18)
        classes = []
        for c in range(32):
            classes.append(self.table("MoveClass", f"moveClasses[{c}]"))
        h.end()
        h.h8(19)
        for c, values in enumerate(classes):
            if self.peek() is None or not self.peek().startswith(f"moveClasses[{c}].passMap["):
                continue
            width, height = int(values[0x10], 16), int(values[0x14], 16)
            full, tail = height // 16, height & 15
            h.h32(c)
            h.run(self.pass_map_words(c, 0, full * width))
            if tail:
                h.run(self.pass_map_words(c, full * width, width))
        h.end()

    def pass_map_words(self, c: int, start: int, count: int) -> bytes:
        out = b""
        for k in range(start, start + count):
            out += self.hex_run(f"moveClasses[{c}].passMap[{k}]")
        return out

    def orders(self) -> None:
        h = self.hash
        h.h8(20)
        for i in self.live:
            h.h32(i)
            for which, chain in enumerate(("list", "list2")):
                h.h8(which + 1)
                n = 0
                while self.present("Order", f"units[{i}].{chain}[{n}]"):
                    prefix = f"units[{i}].{chain}[{n}]"
                    self.table("Order", prefix)
                    if self.present("OrderPosY", prefix):
                        self.table("OrderPosY", prefix)
                    n += 1
                h.h8(0)
        h.end()

    def motions(self) -> None:
        h = self.hash
        h.h8(21)
        for i in self.live:
            h.h32(i)
            present = self.present("UnitMotion", f"units[{i}].motion")
            h.h8(int(present))
            if present:
                self.table("UnitMotion", f"units[{i}].motion")
        h.end()

    def order_effects(self) -> None:
        h = self.hash
        h.h8(22)
        while (fx := self.next_index(r"orderFx\[(\d+)\]\.class")) is not None:
            prefix = f"orderFx[{fx}]"
            cls = int(self.take(f"{prefix}.class"), 16)
            start = self.pos
            while (self.peek() or "").startswith(f"{prefix}."):
                self.pos += 1
            group = dict(self.lines[start : self.pos])
            if cls == 4:
                values = {}
                for f in self.schema.fields("PathOrderRef"):
                    values[f["offset"]] = group.get(f"{prefix}.{field_name(f)}")
                    if values[f["offset"]] is None:
                        raise Error(f"{prefix}: {field_name(f)} is missing")
                self.hash_table("PathOrderRef", values, prefix)
            elif cls == 0:
                h.h32(fx)
                h.h32(cls)
                h.h32(int(group[f"{prefix}.vtableDelta"], 16))
            elif cls not in (1, 2, 3, 5):
                raise Error(f"{prefix}: unknown order effect class {cls}")
        h.end()

    def scripts(self) -> None:
        h = self.hash
        h.h8(23)
        for i in self.live:
            h.h32(i)
            h.h8(int(i in self.scripted))
            if i in self.scripted:
                self.transforms(f"units[{i}].script")
        h.end()

    def transforms(self, prefix: str) -> None:
        first = re.escape(field_name(self.schema.fields("PieceTransform")[0]))
        pattern = rf"{re.escape(prefix)}\.transforms\[(\d+)\]\.{first}"
        while (q := self.next_index(pattern)) is not None:
            self.table("PieceTransform", f"{prefix}.transforms[{q}]")

    def unit_vector(self, path: str) -> None:
        self.hash.h32(int(self.take(f"{path}.size"), 16))
        k = 0
        while self.peek() == f"{path}[{k}]":
            self.hash.pointer(*self.pointer_id(self.take(f"{path}[{k}]")))
            k += 1

    def los_radar_index(self) -> None:
        h = self.hash
        self.skip_dumped(("losRadarState.",))
        while (i := self.next_index(r"losRadarIndex\[(\d+)\]\.player")) is not None:
            prefix = f"losRadarIndex[{i}]"
            h.h8(24)
            h.h32(int(self.take(f"{prefix}.player"), 16))
            h.h32(int(self.take(f"{prefix}.hasSpecial"), 16))
            self.unit_vector(f"{prefix}.visible")
            self.unit_vector(f"{prefix}.known")
            h.end()

    def factory_index(self) -> None:
        h = self.hash
        while (i := self.next_index(r"unitsFactories\[(\d+)\]\.player")) is not None:
            prefix = f"unitsFactories[{i}]"
            h.h8(25)
            h.h32(int(self.take(f"{prefix}.player"), 16))
            self.unit_vector(f"{prefix}.factories")
            h.end()
        self.skip_dumped(("unitsState.",))

    def unit_resources(self) -> None:
        h = self.hash
        h.h8(27)
        for i in range(11):
            h.h32(i)
            present = self.present("UnitResources", f"players[{i}].econ")
            h.h8(int(present))
            if present:
                self.table("UnitResources", f"players[{i}].econ")
        h.end()

    def string(self, path: str) -> None:
        value = self.take(path)
        if value == "null":
            self.hash.h8(0)
            return
        if not value.startswith("str:"):
            raise Error(f"{path}: expected null or str:, found {value!r}")
        raw = value[4:].encode("latin-1")
        self.hash.h8(1)
        self.hash.h32(len(raw))
        self.hash.run(raw)

    def counted_run(self, path: str) -> None:
        self.hash.h32(int(self.take(path), 16))
        self.hash.run(self.hex_run(path))

    def unit_definitions(self) -> None:
        h = self.hash
        h.h8(29)
        h.h32(int(self.take("defs.count"), 16))
        first = re.escape(field_name(self.schema.fields("UnitDefData")[0]))
        while (i := self.next_index(rf"defs\[(\d+)\]\.{first}")) is not None:
            prefix = f"defs[{i}]"
            self.table("UnitDefData", prefix)
            self.counted_run(f"{prefix}.yardmap")
            self.counted_run(f"{prefix}.ids")
            for m in range(4):
                present = self.peek() == f"{prefix}.categories[{m}]"
                h.h8(int(present))
                if present:
                    h.run(self.hex_run(f"{prefix}.categories[{m}]"))
        h.end()

    def weapon_definitions(self) -> None:
        h = self.hash
        h.h8(30)
        for i in range(256):
            prefix = f"weapons[{i}]"
            self.table("WeaponDefData", prefix)
            present = self.peek() == f"{prefix}.damage.entries"
            h.h8(int(present))
            if not present:
                continue
            h.h32(int(self.take(f"{prefix}.damage.entries"), 16))
            k = 0
            while self.peek() == f"{prefix}.damage[{k}].name":
                self.string(f"{prefix}.damage[{k}].name")
                h.h32(int(self.take(f"{prefix}.damage[{k}].value"), 16))
                k += 1
        h.end()

    def mission(self) -> None:
        h = self.hash
        h.h8(31)
        present = self.present("MissionData", "mission")
        h.h8(int(present))
        if present:
            self.table("MissionData", "mission")
            if self.present("MissionStartRest", "mission"):
                self.table("MissionStartRest", "mission")
            h.h32(int(self.take("mission.units.count"), 16))
            k = 0
            while self.peek() == f"mission.units[{k}].name":
                prefix = f"mission.units[{k}]"
                for name in ("name", "ident", "initialMission"):
                    self.string(f"{prefix}.{name}")
                h.run(self.hex_run(prefix))
                k += 1
            h.h32(int(self.take("mission.rules.count"), 16))
            h.run(self.hex_run("mission.rules"))
            h.h32(int(self.take("mission.features.count"), 16))
            k = 0
            while self.peek() == f"mission.features[{k}].name":
                self.string(f"mission.features[{k}].name")
                h.run(self.hex_run(f"mission.features[{k}].pos"))
                k += 1
        h.end()


def rehash(schema: Schema, blocks) -> list[tuple[int, str, int]]:
    """The (tick, phase, hash) rows a dump gives, by tick and then in the row order of docs/trace-format.md, section 2,
    whatever the order of its blocks: `blocks` is parse_dump's dict or parse_blocks' list, and each block of the list
    gives its rows, a block that appears twice among them."""
    items = list(blocks.items()) if isinstance(blocks, dict) else list(blocks)
    check_failures(items)
    rows = []
    for (tick, phase), lines in sorted(items, key=lambda item: (item[0][0], PHASES.index(item[0][1]))):
        walker = Walker(schema, lines)
        if phase != "tick":
            rows.append((tick, phase, walker.phase_row(tick)))
        if phase == "meteors":
            rows.append((tick, "world", walker.world_row(tick)))
        if phase == "tick":
            walker.phase_row(tick)
            rows.append((tick, "tick", walker.world_row(tick, reset=False)))
    return rows


def view_rows(directory: Path, ticks: list[int]) -> list[tuple[int, str, int]]:
    """The (tick, "view", hash) of each tick in `ticks` that has a view file in `directory`."""
    rows = []
    for tick in ticks:
        path = directory / f"{tick}.bin"
        if path.is_file():
            try:
                rows.append((tick, "view", viewblock.view_hash(viewblock.split(path.read_bytes()))))
            except viewblock.Error as e:
                raise Error(f"{path}: {e}") from None
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dump", help="a dump.txt, or a dump.txt.xz")
    ap.add_argument("--schema", default="data/trace_schema.json")
    ap.add_argument("--trace", help="a trace.txt (or .xz) to compare the rows with")
    ap.add_argument("--views", help="the directory of the view files <tick>.bin (default: views/ beside the dump)")
    args = ap.parse_args()
    try:
        schema = Schema(json.loads(Path(args.schema).read_text()))
        blocks = parse_dump(read_text(args.dump), schema.data["trace_format"])
        rows = rehash(schema, blocks)
        views = Path(args.views) if args.views else Path(args.dump).resolve().parent / "views"
        rows += view_rows(views, sorted({t for t, _ in blocks}))
        rows.sort(key=lambda row: (row[0], ROW_ORDER.index(row[1])))
        expected = parse_trace_rows(read_text(args.trace)) if args.trace else None
    except (Error, OSError, ValueError, KeyError) as e:
        print(f"tracerehash: {e}", file=sys.stderr)
        return 1
    failed = False
    for tick, phase, value in rows:
        text = f"{tick} {phase} {value:016x}"
        if expected is not None:
            same = expected.get((tick, phase)) == f"{value:016x}"
            failed = failed or not same
            text += " ok" if same else " differs"
        print(text)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
