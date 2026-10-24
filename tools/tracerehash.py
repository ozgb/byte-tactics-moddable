"""Recompute the rows of a trace.txt from a dump.txt, following docs/trace-format.md only.

    uv run tools/tracerehash.py dump.txt [--schema data/trace_schema.json] [--trace trace.txt]

Prints `<tick> <phase> <hash>` for every phase row and the world row of each dumped tick. With
--trace, each row is followed by `ok` or `differs` against the same row of the trace, and the exit
status is 1 if any row differs or the input cannot be parsed. A path ending in .xz is read compressed. Dumps of tick 1 also give the
sections 29 to 31 in the world row.
"""

import argparse
import json
import lzma
import re
import sys
from pathlib import Path

OFFSET_BASIS = 0xCBF29CE484222325
PRIME = 0x100000001B3
MASK64 = (1 << 64) - 1
END = 0xFFFFFFFF
PHASES = ("net", "units", "projectiles", "explosions", "players", "features", "wind", "meteors")
TIMER_CLASS = (0, 7, 2, 3, 6, 1, 2, 3, 4, 5)
AI_RAW_VECTORS = ("cells", "vec_65", "counts", "vec_8d", "weights", "vec_ad", "vec_bd", "values", "locked")
POS_Y_UNSET_KINDS = ("MobileBuild", "VTOL_MobileBuild", "Reclaim", "VTOL_Reclaim")


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


def parse_dump(text: str) -> dict[tuple[int, str], list[tuple[str, str]]]:
    """The (path, value) lines of each (tick, phase) block of a dump, in file order."""
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    if not lines or lines[0] != "bt-dump 3":
        raise Error("not a bt-dump 3 file")
    blocks: dict[tuple[int, str], list[tuple[str, str]]] = {}
    for n, line in enumerate(lines[1:], 2):
        parts = line.split(" ", 3)
        if len(parts) < 3 or not parts[0].isdigit() or parts[1] not in PHASES:
            raise Error(f"dump line {n}: cannot parse")
        value = parts[3] if len(parts) == 4 else ""
        blocks.setdefault((int(parts[0]), parts[1]), []).append((parts[2], value))
    return blocks


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
        self.fx_count = 0

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
            self.hash.h32(int(value, 16))
        else:
            self.hash.run(value_bytes(field, value))

    def read_table(self, table: str, prefix: str) -> dict[int, str]:
        values = {}
        for f in self.schema.fields(table):
            values[f["offset"]] = self.take(f"{prefix}.{field_name(f)}")
        return values

    def table(self, table: str, prefix: str) -> dict[int, str]:
        values = self.read_table(table, prefix)
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
        self.explosions()
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

    def explosions(self) -> None:
        h = self.hash
        h.h8(3)
        first = re.escape(field_name(self.schema.fields("ExplodedPiece")[0]))
        while (slot := self.next_index(rf"explodedPieces\[(\d+)\]\.{first}")) is not None:
            h.h32(slot)
            self.table("ExplodedPiece", f"explodedPieces[{slot}]")
            self.table("PieceRec", f"explodedPieces[{slot}].rec")
        h.end()
        h.h8(4)
        remembered = set()
        first_field = self.schema.fields("Debris")[0]
        first = re.escape(field_name(first_field))
        while (d := self.next_index(rf"debris\[(\d+)\]\.{first}")) is not None:
            h.h32(d)
            values = self.table("Debris", f"debris[{d}]")
            tag, index = self.pointer_id(values[first_field["offset"]])
            if tag == 8:
                remembered.add(index)
        h.end()
        h.h8(5)
        first = re.escape(field_name(self.schema.fields("Piece")[0]))
        seen = []
        while (i := self.next_index(rf"pieces\[(\d+)\]\.{first}")) is not None:
            h.h32(i)
            seen.append(i)
            self.table("Piece", f"pieces[{i}]")
            h.run(self.hex_run(f"pieces[{i}].vertices"))
            for k in range(6):
                self.table("Face", f"pieces[{i}].faces[{k}]")
        if set(seen) != remembered:
            raise Error(f"pieces in the dump {sorted(seen)} differ from the pieces debris points at {sorted(remembered)}")
        h.end()

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

    def world_row(self, tick: int = 0) -> int:
        self.hash.reset()
        self.game()
        self.cells()
        self.visibility()
        self.explored()
        self.spatial()
        self.eyes()
        self.pathfinder()
        self.move_classes()
        self.orders()
        self.motions()
        self.order_effects()
        self.scripts()
        self.player_ai()
        self.squad_managers()
        self.squads()
        self.unit_resources()
        self.conditions()
        if tick == 1:
            self.unit_definitions()
            self.weapon_definitions()
            self.mission()
        if self.pos != len(self.lines):
            raise Error(f"{len(self.lines) - self.pos} dump lines left after section 28, from {self.lines[self.pos][0]}")
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

    def eyes(self) -> None:
        h = self.hash
        h.h8(15)
        h.h32(int(self.take("eyes.count"), 16))
        first = re.escape(field_name(self.schema.fields("Eye")[0]))
        while (e := self.next_index(rf"eyes\[(\d+)\]\.{first}")) is not None:
            self.table("Eye", f"eyes[{e}]")
        h.end()

    def pathfinder(self) -> None:
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
        self.flat_list(self.schema.data["globals"]["pathfinder"], "pathfinder.")
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
        self.live = []
        while (i := self.next_index(r"units\[(\d+)\]\.head")) is not None:
            self.live.append(i)
            h.h32(i)
            h.pointer(*self.pointer_id(self.take(f"units[{i}].head")))
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

    def goal(self, prefix: str) -> None:
        h = self.hash
        cls = int(self.take(f"{prefix}.class"), 16)
        h.h32(cls)
        if cls == 1:
            self.table("PatrolGoal", prefix)
            h.run(self.hex_run(f"{prefix}.points"))
        elif cls == 2:
            self.table("AiSearchGoal", prefix)
            h.run(self.hex_run(f"{prefix}.points"))
        elif cls in (3, 4, 5):
            self.table("PackedGoal", prefix)
            if cls == 4:
                self.table("LiteGoalState", prefix)
        elif cls != 0:
            raise Error(f"{prefix}: unknown goal class {cls}")

    def motions(self) -> None:
        h = self.hash
        h.h8(21)
        for i in self.live:
            h.h32(i)
            present = self.present("UnitMotion", f"units[{i}].motion")
            h.h8(int(present))
            if present:
                self.table("UnitMotion", f"units[{i}].motion")
                self.goal(f"units[{i}].motion.goal")
        h.end()

    def order_effects(self) -> None:
        h = self.hash
        h.h8(22)
        fx_tables = {1: ("ApproachRadius",), 2: ("RingApproach",), 3: ("PointMarker",)}
        while (fx := self.next_index(r"orderFx\[(\d+)\]\.class")) is not None:
            prefix = f"orderFx[{fx}]"
            cls = int(self.take(f"{prefix}.class"), 16)
            h.h32(fx)
            h.h32(cls)
            if cls in fx_tables:
                self.table(fx_tables[cls][0], prefix)
            elif cls == 4:
                self.table("PathOrder", prefix)
                for extra in ("PathOrderPosY", "PathOrderHeading", "PathOrderPiece", "PathOrderFollow"):
                    if self.present(extra, prefix):
                        self.table(extra, prefix)
            elif cls == 5:
                self.table("AirManeuverOrder", prefix)
                if self.present("AirManeuverOrderHeading", prefix):
                    self.table("AirManeuverOrderHeading", prefix)
            else:
                raise Error(f"{prefix}: unknown order effect class {cls}")
        h.end()

    def scripts(self) -> None:
        h = self.hash
        h.h8(23)
        for i in self.live:
            h.h32(i)
            prefix = f"units[{i}].script"
            present = self.present("ScriptState", prefix)
            h.h8(int(present))
            if present:
                self.script(prefix)
        h.end()

    def script(self, prefix: str) -> None:
        h = self.hash
        self.table("ScriptState", prefix)
        for c in range(8):
            self.channel(f"{prefix}.channels[{c}]")
        h.h32(int(self.take(f"{prefix}.statics"), 16))
        h.run(self.hex_run(f"{prefix}.statics"))
        first = re.escape(field_name(self.schema.fields("ScriptPiece")[0]))
        pattern = rf"{re.escape(prefix)}\.pieces\[(\d+)\]\.{first}"
        while (p := self.next_index(pattern)) is not None:
            self.table("ScriptPiece", f"{prefix}.pieces[{p}]")
        first = re.escape(field_name(self.schema.fields("PieceTransform")[0]))
        pattern = rf"{re.escape(prefix)}\.transforms\[(\d+)\]\.{first}"
        while (q := self.next_index(pattern)) is not None:
            self.table("PieceTransform", f"{prefix}.transforms[{q}]")

    def channel(self, prefix: str) -> None:
        h = self.hash
        if int(self.lines[self.pos][1], 16) == 0:
            h.h32(int(self.take(f"{prefix}.state"), 16))
            return
        self.table("ScriptChannel", prefix)
        h.run(self.hex_run(f"{prefix}.stack"))
        for extra in ("ScriptChannelPiece", "ScriptChannelDelay", "ScriptChannelWait"):
            if self.present(extra, prefix):
                self.table(extra, prefix)

    def unit_vector(self, path: str) -> None:
        self.hash.h32(int(self.take(f"{path}.size"), 16))
        k = 0
        while self.peek() == f"{path}[{k}]":
            self.hash.pointer(*self.pointer_id(self.take(f"{path}[{k}]")))
            k += 1

    def raw_vector(self, path: str) -> None:
        self.hash.h32(int(self.take(path), 16))
        self.hash.run(self.hex_run(path))

    def player_ai(self) -> None:
        h = self.hash
        h.h8(24)
        for i in range(10):
            prefix = f"players[{i}].ai"
            h.h32(i)
            present = self.present("PlayerAI", prefix)
            h.h8(int(present))
            if not present:
                continue
            self.table("PlayerAI", prefix)
            for name in ("visible", "known", "factories"):
                self.unit_vector(f"{prefix}.{name}")
            for name in AI_RAW_VECTORS:
                self.raw_vector(f"{prefix}.{name}")
        h.end()

    def squad_managers(self) -> None:
        h = self.hash
        h.h8(25)
        for i in range(11):
            prefix = f"players[{i}].squadManager"
            h.h32(i)
            present = self.present("SquadManager", prefix)
            h.h8(int(present))
            if not present:
                continue
            self.table("SquadManager", prefix)
            for t in range(10):
                timer = f"{prefix}.timers[{t}]"
                h.h8(t)
                has = self.peek() == f"{timer}.class"
                h.h8(int(has))
                if not has:
                    continue
                cls = int(self.take(f"{timer}.class"), 16)
                if cls != TIMER_CLASS[t]:
                    raise Error(f"{timer}: class {cls} is not the class of slot {t}")
                h.h32(cls)
                name = {2: "AssaultTimer", 3: "EscortTimer", 5: "SpatialTimer"}.get(cls, "SquadTimer")
                self.table(name, timer)
        h.end()

    def squads(self) -> None:
        h = self.hash
        h.h8(26)
        for i in range(11):
            h.h32(i)
            present = self.present("Squad", f"players[{i}].squads[0]")
            h.h8(int(present))
            if not present:
                continue
            for s in range(10):
                prefix = f"players[{i}].squads[{s}]"
                self.table("Squad", prefix)
                self.unit_vector(f"{prefix}.items")
        h.end()

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

    def conditions(self) -> None:
        h = self.hash
        h.h8(28)
        present = self.present("MissionConditions", "conditions")
        h.h8(int(present))
        if present:
            values = self.table("MissionConditions", "conditions")
            for which, (list_name, count_offset) in enumerate((("victory", 0x40), ("defeat", 0x84))):
                count = int(values[count_offset], 16)
                if count > 16:
                    count = 0
                h.h8(which + 1)
                for k in range(count):
                    prefix = f"conditions.{list_name}[{k}]"
                    has = self.peek() == f"{prefix}.class"
                    h.h8(int(has))
                    if not has:
                        continue
                    cls = int(self.take(f"{prefix}.class"), 16)
                    h.h32(cls)
                    self.table("MissionCondition", prefix)
                    if cls == 16:
                        self.table("ConditionNumUnits", prefix)
                    elif cls in (12, 4):
                        self.table("ConditionNumLeft", prefix)
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


def rehash(schema: Schema, blocks: dict[tuple[int, str], list[tuple[str, str]]]) -> list[tuple[int, str, int]]:
    """The (tick, phase, hash) rows a dump gives, phases in order, `world` after `meteors`."""
    rows = []
    for tick in sorted({t for t, _ in blocks}):
        for phase in PHASES:
            if (tick, phase) not in blocks:
                continue
            walker = Walker(schema, blocks[(tick, phase)])
            rows.append((tick, phase, walker.phase_row(tick)))
            if phase == "meteors":
                rows.append((tick, "world", walker.world_row(tick)))
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dump", help="a dump.txt, or a dump.txt.xz")
    ap.add_argument("--schema", default="data/trace_schema.json")
    ap.add_argument("--trace", help="a trace.txt (or .xz) to compare the rows with")
    args = ap.parse_args()
    try:
        schema = Schema(json.loads(Path(args.schema).read_text()))
        rows = rehash(schema, parse_dump(read_text(args.dump)))
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
