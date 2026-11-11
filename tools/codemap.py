"""Browse the game as an interactive architecture map, flow charts and data-flow views.

    uv run tools/codemap.py                # serve on http://127.0.0.1:8123 and open a browser
    uv run tools/codemap.py --port 9000 --no-open
    uv run tools/codemap.py --stats        # build the model, print how much resolved, serve nothing
    uv run tools/test_codemap.py           # tests, on hand-written sources and the committed data

The model comes from what CI already keeps current, and nothing is compiled:

  data/rwsets.csv         calls, reads and writes of every function, from the clang syntax tree
  data/subsystems.csv     the subsystem of every function
  data/hazard_edges.csv   hand-resolved indirect calls: order tables, vtables, menu handlers
  data/coupling.csv       the state two subsystems both write
  data/state_classes.csv  sim, view or platform, with the notes on each item
  data/coverage.csv       functions 3.1c's code cannot reach
  interfaces/*.h          the seam slots and their one-line docs
  src/seams/native.cpp    which function fills each seam slot

Seams. A call through `g_seams.weapons->SpawnProjectile` is recorded as an indirect call of a
slot. The model joins that to InstallNativeSeams's assignment and, when the slot holds a `...Native`
wrapper that only forwards, to the function behind it, so the graphs run straight through the seam
and keep the slot's name and doc on the edge.

Flow charts and call arguments come from a small statement parser over the source text with
comments, strings and BT_ACCESS_LOG blocks blanked. The set of calls is the clang one: a name only
counts as a call when rwsets.csv says the function calls it. A function the parser can't follow
falls back to a flat list of its calls, in source order.
"""

import argparse
import bisect
import csv
import json
import re
import subprocess
import sys
import webbrowser
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, unquote, urlparse

ROOT = Path(__file__).resolve().parent.parent
UI = Path(__file__).resolve().parent / "codemap_ui"
SOURCE_DIRS = ("src", "interfaces", "include", "lib")
SOURCE_SUFFIXES = {".c", ".cpp", ".h", ".hpp", ".inc"}
UI_TYPES = {".html": "text/html", ".js": "text/javascript", ".css": "text/css"}

SUBSYSTEMS = {
    "presentation": (0, "Rendering, menus, in-game UI and sound. Runs outside the tick phases; the UI issues orders into the game."),
    "game": (0, "Start-up, match load and save, and the main loop: RunGameSteps calls every phase of a tick."),
    "network": (0, "Packets, players joining and the per-tick HandleNetPackets, which dispatches into every subsystem."),
    "harness": (0, "Test mode only: scenarios, per-phase state hashes and replays."),
    "units": (1, "The UpdateAllUnits phase: each unit's life, position and COB scripts. Runs orders and weapons per unit."),
    "weapons": (1, "The projectile and explosion phases, ballistics, damage and the weapon definitions."),
    "players": (1, "The UpdatePlayers phase: runs economy, AI, pathfinding and line of sight for each player in turn."),
    "features": (1, "Trees, rocks and wrecks: the UpdateFeatures phase, burning, spreading and reclaim."),
    "wind_meteors": (1, "The UpdateWind and UpdateMeteors phases. The smallest module and the WASM pilot."),
    "orders": (2, "The order state machines each unit runs, order lists, unit motion and order goals. The largest subsystem."),
    "ai": (2, "The computer player: build ratings, weight scripts and squads."),
    "pathfinding": (2, "The path search: per-player cost tables, the node budget, the open heap and the grid."),
    "targeting": (2, "Choosing targets, and the weapon target setters and getters."),
    "los_radar": (2, "Line of sight, radar and sonar, the LOS tables and the map cell accessor."),
    "economy": (2, "Resource accounts, per-player economy and resource sharing."),
    "map": (3, "Mission data and its accessors. No phase of its own."),
    "files": (3, "Archive and bank readers: HapiBank, HPI and SQSH."),
    "platform": (3, "Devices, memory, TDF parsing, the console, timers and the C library."),
    "seams": (3, "The native side of the seam tables. Drawn through: its calls appear on the real callers."),
}
HIDDEN_FROM_MAP = {"seams"}

SKIP_CALL_NAMES = {"if", "while", "for", "switch", "return", "sizeof", "catch", "__except", "defined"}
SIGNATURE_NOISE = re.compile(r"\b(?:__stdcall|__fastcall|__cdecl|__thiscall|static|inline|extern)\b")
SEAM_SLOT = re.compile(r"\[(\w+Seam)::(\w+)\]")
SEAM_FIELD = re.compile(r"^\s*(\w+Seam)\*\s+(\w+);", re.M)
SLOT_ASSIGN = re.compile(r"\b(s_\w+)\.(\w+)\s*=\s*(?:\(.*\))?\s*&?([\w:]+)\s*;")
SLOT_TABLE = re.compile(r"\bstatic\s+(\w+Seam)\s+(s_\w+)\s*;")
TOP_DIRECTIVE = re.compile(r"\s*#\s*(\w+)(.*)")
DROPPED_CONDITION = re.compile(r"\s*(?:0|BT_ACCESS_LOG)\b")
DEFINITION = re.compile(
    r"((?:[A-Za-z_]\w*(?:<[^<>();{}]*>)?\s*::\s*)*(?:~?[A-Za-z_]\w*|operator\s*(?:\(\s*\)|[^\s(\w]+)))\s*\("
)
TYPE_DEFINITION = re.compile(r"\b(?:struct|class)\s+(\w+)\b[^;{()]*\{")
CALL_NAME = re.compile(r"(~?[A-Za-z_]\w*)\s*\(")
IDENT = re.compile(r"[A-Za-z_]\w*")
PARAM_NAME = re.compile(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*$")


def mask_source(text):
    """Blanks comments, literal contents and preprocessor lines, keeping every offset."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            while i < n and text[i] != "\n":
                out[i] = " "
                i += 1
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            for j in range(i, end):
                if text[j] != "\n":
                    out[j] = " "
            i = end
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                if text[j] == "\\":
                    out[j] = " "
                    j += 1
                    if j >= n:
                        break
                out[j] = " "
                j += 1
            i = j + 1
        else:
            i += 1
    return _mask_directives("".join(out))


def _mask_directives(text):
    lines = text.split("\n")
    stack = []
    continuing = False
    for index, line in enumerate(lines):
        directive = None if continuing else TOP_DIRECTIVE.match(line)
        was_directive = continuing or directive is not None
        if directive:
            word, rest = directive.groups()
            if word in ("if", "ifdef", "ifndef"):
                dropped = word == "if" and bool(DROPPED_CONDITION.match(rest))
                stack.append({"drop": dropped, "taken": not dropped})
            elif word in ("else", "elif") and stack:
                stack[-1]["drop"] = stack[-1]["taken"]
                stack[-1]["taken"] = True
            elif word == "endif" and stack:
                stack.pop()
        continuing = was_directive and line.rstrip().endswith("\\")
        if was_directive or any(level["drop"] for level in stack):
            lines[index] = " " * len(line)
    return "\n".join(lines)


def close_of(text, start, opener="(", closer=")"):
    """The offset of the bracket closing the one at start, or -1."""
    depth = 0
    for i in range(start, len(text)):
        if text[i] == opener:
            depth += 1
        elif text[i] == closer:
            depth -= 1
            if depth == 0:
                return i
    return -1


def skip_space(text, i):
    while i < len(text) and text[i].isspace():
        i += 1
    return i


def line_starts(text):
    starts = [0]
    starts.extend(m.end() for m in re.finditer("\n", text))
    return starts


def line_of(starts, offset):
    return bisect.bisect_right(starts, offset)


def normalise_name(name):
    name = re.sub(r"<[^<>]*>", "", name)
    return re.sub(r"\s+", "", name)


@dataclass
class Definition:
    name: str
    start: int
    open_paren: int
    close_paren: int
    body: int
    end: int


def find_definitions(masked):
    """Every function body in a masked file, as Definition records in file order."""
    found = []
    initialisers_end = 0
    for match in DEFINITION.finditer(masked):
        name = normalise_name(match.group(1))
        if match.start() < initialisers_end or name.split("::")[-1] in SKIP_CALL_NAMES:
            continue
        open_paren = match.end() - 1
        close_paren = close_of(masked, open_paren)
        if close_paren < 0:
            continue
        i = skip_space(masked, close_paren + 1)
        if masked.startswith("const", i):
            i = skip_space(masked, i + 5)
        if masked.startswith(":", i) and not masked.startswith("::", i):
            depth = 0
            j = i
            while j < len(masked) and not (masked[j] == "{" and depth == 0):
                depth += (masked[j] == "(") - (masked[j] == ")")
                j += 1
            i = j
        if i >= len(masked) or masked[i] != "{":
            continue
        end = close_of(masked, i, "{", "}")
        if end >= 0:
            found.append(Definition(name, match.start(), open_paren, close_paren, i, end))
            initialisers_end = i
    return found


def split_arguments(masked, raw, start, end):
    """The raw text of each comma-separated argument between two offsets."""
    if not masked[start:end].strip():
        return []
    parts, depth, last = [], 0, start
    for i in range(start, end):
        c = masked[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "," and depth == 0:
            parts.append(raw[last:i])
            last = i + 1
    parts.append(raw[last:end])
    return [" ".join(p.split()) for p in parts]


def parameter_names(params):
    """The name of each parameter in a parameter list, or '' where the list has none."""
    if not params.strip() or params.strip() == "void":
        return []
    names = []
    depth, last = 0, 0
    for i, c in enumerate(params + ","):
        if c in "([<":
            depth += 1
        elif c in ")]>":
            depth -= 1
        elif c == "," and depth == 0:
            part = params[last:i].split("=")[0]
            last = i + 1
            words = IDENT.findall(part)
            match = PARAM_NAME.search(part.strip())
            names.append(match.group(1) if match and len(words) > 1 else "")
    return names


class FlowError(Exception):
    pass


class FlowParser:
    """Turns one function body into a tree of if, loop, switch and call nodes."""

    def __init__(self, masked, raw, starts, callee_for):
        self.masked = masked
        self.raw = raw
        self.starts = starts
        self.callee_for = callee_for

    def parse(self, body, end):
        self.i = body + 1
        self.end = end
        return self.sequence(end)

    def line(self, offset):
        return line_of(self.starts, offset)

    def text(self, start, end):
        return " ".join(self.raw[start:end].split())

    def sequence(self, end):
        items = []
        while True:
            self.i = skip_space(self.masked, self.i)
            if self.i >= end:
                break
            before = self.i
            items.extend(self.statement())
            if self.i <= before:
                raise FlowError(f"no progress at offset {before}")
        self.i = end + 1
        return items

    def word(self):
        match = IDENT.match(self.masked, self.i)
        return match.group(0) if match else ""

    def paren_range(self):
        self.i = skip_space(self.masked, self.i)
        if self.masked[self.i : self.i + 1] != "(":
            raise FlowError("expected (")
        close = close_of(self.masked, self.i)
        if close < 0:
            raise FlowError("unbalanced (")
        start = self.i + 1
        self.i = close + 1
        return start, close

    def body(self):
        self.i = skip_space(self.masked, self.i)
        return self.statement()

    def condition(self):
        start, close = self.paren_range()
        return {"cond": self.text(start, close), "calls": self.calls(start, close)}

    def statement(self):
        at = self.i
        if self.masked[at] == "{":
            close = close_of(self.masked, at, "{", "}")
            if close < 0:
                raise FlowError("unbalanced {")
            self.i = at + 1
            return self.sequence(close)
        if self.masked[at] == ";":
            self.i += 1
            return []
        word = self.word()
        handler = getattr(self, "on_" + word.lstrip("_"), None) if word else None
        if handler:
            self.i += len(word)
            return handler(at)
        if word and self.is_label(at + len(word)):
            self.i = self.masked.index(":", at) + 1
            return [{"t": "label", "text": word, "line": self.line(at)}]
        return self.simple(at)

    def is_label(self, after):
        j = skip_space(self.masked, after)
        return self.masked[j : j + 1] == ":" and self.masked[j : j + 2] != "::"

    def on_if(self, at):
        node = {"t": "if", "line": self.line(at), **self.condition(), "then": self.body(), "else": []}
        self.i = skip_space(self.masked, self.i)
        if self.word() == "else":
            self.i += 4
            node["else"] = self.body()
        return [node]

    def loop(self, kind, at):
        return [{"t": "loop", "kind": kind, "line": self.line(at), **self.condition(), "body": self.body()}]

    def on_while(self, at):
        return self.loop("while", at)

    def on_for(self, at):
        return self.loop("for", at)

    def on_do(self, at):
        body = self.body()
        self.i = skip_space(self.masked, self.i)
        if self.word() != "while":
            raise FlowError("do without while")
        self.i += 5
        node = {"t": "loop", "kind": "do", "line": self.line(at), **self.condition(), "body": body}
        self.i = skip_space(self.masked, self.i)
        if self.masked[self.i : self.i + 1] == ";":
            self.i += 1
        return [node]

    def on_switch(self, at):
        node = {"t": "switch", "line": self.line(at), **self.condition()}
        cases, current = [], None
        for item in self.body():
            if item["t"] == "case":
                if current is not None and not current["body"]:
                    current["labels"].append(item["text"])
                    continue
                current = {"labels": [item["text"]], "line": item["line"], "body": []}
                cases.append(current)
            elif current is not None:
                current["body"].append(item)
        node["cases"] = cases
        return [node]

    def on_case(self, at):
        start = self.i
        j = start
        depth = 0
        while j < self.end:
            c = self.masked[j]
            depth += (c == "(") - (c == ")")
            if c == ":" and depth == 0 and self.masked[j + 1] != ":" and self.masked[j - 1] != ":":
                break
            j += 1
        self.i = j + 1
        return [{"t": "case", "text": self.text(start, j), "line": self.line(at)}]

    def on_default(self, at):
        self.i = self.masked.index(":", at) + 1
        return [{"t": "case", "text": "default", "line": self.line(at)}]

    def on_try(self, at):
        return self.body()

    def on_finally(self, at):
        return self.body()

    def on_catch(self, at):
        return [{"t": "if", "line": self.line(at), "cond": "catch", "calls": self.condition()["calls"], "then": self.body(), "else": []}]

    def on_except(self, at):
        return self.on_catch(at)

    def jump(self, kind, at):
        start = self.i
        close = self.statement_end(start)
        self.i = close + 1
        node = {"t": kind, "text": self.text(start, close), "line": self.line(at), "calls": self.calls(start, close)}
        return [node]

    def on_return(self, at):
        return self.jump("return", at)

    def on_goto(self, at):
        return self.jump("goto", at)

    def on_break(self, at):
        return self.jump("break", at)

    def on_continue(self, at):
        return self.jump("continue", at)

    def statement_end(self, start):
        depth = 0
        for j in range(start, self.end):
            c = self.masked[j]
            if c in "([{":
                depth += 1
            elif c in ")]}":
                depth -= 1
            elif c == ";" and depth == 0:
                return j
        raise FlowError("statement without ;")

    def simple(self, at):
        close = self.statement_end(at)
        self.i = close + 1
        calls = self.calls(at, close)
        kind = "call" if calls else "stmt"
        return [{"t": kind, "text": self.text(at, close), "line": self.line(at), "calls": calls}]

    def calls(self, start, end):
        out = []
        for match in CALL_NAME.finditer(self.masked, start, end):
            name = match.group(1)
            if name in SKIP_CALL_NAMES:
                continue
            callee = self.callee_for(name)
            if callee is None:
                continue
            open_paren = match.end() - 1
            close = close_of(self.masked, open_paren)
            if close < 0 or close > end:
                continue
            values = split_arguments(self.masked, self.raw, open_paren + 1, close)
            out.append((close, {**callee, "line": self.line(match.start()), "values": values}))
        return [call for _, call in sorted(out, key=lambda pair: pair[0])]


@dataclass(eq=False)
class Function:
    id: int
    name: str
    file: str
    address: str
    subsystem: str
    rows: list = field(default_factory=list)
    line: int = 0
    end: int = 0
    signature: str = ""
    params: str = ""
    flow: list = field(default_factory=list)
    flow_ok: bool = False
    dead: bool = False
    out: dict = field(default_factory=dict)
    into: dict = field(default_factory=dict)
    reads: list = field(default_factory=list)
    writes: list = field(default_factory=list)
    pointer_accesses: int = 0

    @property
    def short(self):
        return self.name.split("::")[-1]


@dataclass
class Edge:
    kinds: set = field(default_factory=set)
    via: str = ""
    reason: str = ""
    sites: list = field(default_factory=list)


@dataclass
class Slot:
    key: str
    subsystem: str
    name: str
    doc: str
    params: str
    header: str
    line: int
    function: Function = None


def read_csv(path):
    with open(path, newline="", encoding="utf-8", errors="replace") as handle:
        return list(csv.DictReader(handle))


class Model:
    """The whole program as functions, edges, data items and seam slots."""

    def __init__(self, root=ROOT):
        self.root = Path(root)
        self.functions = []
        self.by_key = {}
        self.by_name = defaultdict(list)
        self.slots = {}
        self.items = {}
        self.types = {}
        self.sources = {}
        self.shared = defaultdict(list)
        self.commit = self.git_commit()
        self.load_functions()
        self.load_slots()
        self.load_sources()
        self.resolve_calls()
        self.load_data_items()
        self.load_types()
        self.parse_flows()

    def git_commit(self):
        result = subprocess.run(
            ["git", "merge-base", "HEAD", "origin/moddable"], cwd=self.root, capture_output=True, text=True
        )
        return result.stdout.strip() if result.returncode == 0 else "moddable"

    def data(self, name):
        return read_csv(self.root / "data" / name)

    def load_functions(self):
        subsystems = {(r["function"], r["file"]): r["subsystem"] for r in self.data("subsystems.csv")}
        for row in self.data("rwsets.csv"):
            key = (row["function"], row["file"])
            function = self.by_key.get(key)
            if function is None:
                subsystem = subsystems.get(key, "platform")
                function = Function(len(self.functions), row["function"], row["file"], row["address"], subsystem)
                self.functions.append(function)
                self.by_key[key] = function
                self.by_name[row["function"]].append(function)
            if row["kind"] != "function":
                function.rows.append((row["kind"], row["target"]))
        for row in self.data("coverage.csv"):
            function = self.by_key.get((row["function"], row["file"]))
            if function is not None:
                function.dead = row["dead"] == "1"

    def load_slots(self):
        interfaces = self.root / "interfaces"
        seams_header = (interfaces / "seams.h").read_text(encoding="utf-8", errors="replace")
        seam_subsystem = {seam: name for seam, name in SEAM_FIELD.findall(seams_header)}
        for path in sorted(interfaces.glob("*.h")):
            text = path.read_text(encoding="utf-8", errors="replace")
            for match in re.finditer(r"struct\s+(\w+Seam)\s*\{", text):
                seam = match.group(1)
                close = close_of(text, match.end() - 1, "{", "}")
                self.parse_slots(seam, seam_subsystem.get(seam, ""), path, text, match.end(), close)
        native = (self.root / "src/seams/native.cpp").read_text(encoding="utf-8", errors="replace")
        tables = {table: seam for seam, table in SLOT_TABLE.findall(native)}
        self.slot_fillers = {}
        for table, slot, value in SLOT_ASSIGN.findall(native):
            if table in tables:
                self.slot_fillers[f"{tables[table]}::{slot}"] = value.lstrip("&")

    def parse_slots(self, seam, subsystem, path, text, start, end):
        relative = path.relative_to(self.root).as_posix()
        doc = ""
        for offset, line in self.lines_between(text, start, end):
            stripped = line.strip()
            if stripped.startswith("//"):
                doc = stripped[2:].strip()
                continue
            match = re.search(r"\(\s*\w*\*\s*(\w+)\s*\)\s*\((.*)\)\s*;", stripped)
            if match:
                key = f"{seam}::{match.group(1)}"
                number = text.count("\n", 0, offset) + 1
                self.slots[key] = Slot(key, subsystem, match.group(1), doc, match.group(2), relative, number)
            doc = ""

    @staticmethod
    def lines_between(text, start, end):
        offset = start
        for line in text[start:end].split("\n"):
            yield offset, line
            offset += len(line) + 1

    def load_sources(self):
        self.source_files = set()
        for directory in SOURCE_DIRS:
            for path in (self.root / directory).rglob("*"):
                if path.is_file() and path.suffix in SOURCE_SUFFIXES:
                    self.source_files.add(path.relative_to(self.root).as_posix())
        per_file = defaultdict(list)
        for function in self.functions:
            per_file[function.file].append(function)
        for relative, functions in per_file.items():
            path = self.root / relative
            if not path.is_file():
                continue
            raw = path.read_text(encoding="utf-8", errors="replace")
            masked = mask_source(raw)
            starts = line_starts(raw)
            definitions = find_definitions(masked)
            self.sources[relative] = (raw, masked, starts)
            self.place_functions(functions, definitions, raw, masked, starts)

    def place_functions(self, functions, definitions, raw, masked, starts):
        exact, by_last = defaultdict(list), defaultdict(list)
        for definition in definitions:
            exact[definition.name].append(definition)
            by_last[definition.name.split("::")[-1]].append(definition)
        for function in functions:
            name = normalise_name(function.name)
            candidates = exact.get(name) or by_last.get(name.split("::")[-1]) or []
            definition = self.pick_definition(candidates, function, raw, starts)
            if definition is None:
                continue
            function.definition = definition
            function.line = line_of(starts, definition.start)
            function.end = line_of(starts, definition.end)
            declaration_start = definition.start
            while declaration_start > 0 and masked[declaration_start - 1] not in ";{}":
                declaration_start -= 1
            function.signature = " ".join(SIGNATURE_NOISE.sub("", masked[declaration_start:definition.body]).split())
            function.params = masked[definition.open_paren + 1 : definition.close_paren]

    @staticmethod
    def pick_definition(candidates, function, raw, starts):
        if not candidates:
            return None
        if function.address and len(candidates) > 1:
            for definition in candidates:
                number = line_of(starts, definition.start)
                before = raw[starts[max(number - 3, 0)] : definition.start]
                if function.address[2:].lstrip("0") in before.lower():
                    return definition
        return candidates[0]

    def choose(self, name, caller):
        """The function a bare name means to a caller: its own file, then its subsystem, then the first."""
        options = self.by_name.get(name)
        if not options:
            return None
        return next(
            (o for o in options if o.file == caller.file),
            next((o for o in options if o.subsystem == caller.subsystem), options[0]),
        )

    def through_native(self, function):
        """Replaces a seams wrapper that only forwards with the function it forwards to."""
        if function is None or function.subsystem != "seams":
            return function
        targets = {
            self.choose(target, function) for kind, target in function.rows if kind == "call"
        } - {None}
        real = [t for t in targets if t.subsystem != "seams"]
        return real[0] if len(real) == 1 else function

    def resolve_calls(self):
        for slot in self.slots.values():
            filler = self.slot_fillers.get(slot.key)
            if filler is None:
                continue
            options = self.by_name.get(filler) or self.by_name.get(filler.split("::")[-1]) or []
            slot.function = self.through_native(options[0]) if options else None
        self.call_names = {}
        for function in self.functions:
            names = {}
            for kind, target in function.rows:
                if kind == "call":
                    callee = self.through_native(self.choose(target, function))
                    if callee is not None:
                        self.add_edge(function, callee, "call")
                        names[target.split("::")[-1]] = {"id": callee.id, "name": callee.name, "via": ""}
                elif kind == "indirect_call":
                    match = SEAM_SLOT.search(target)
                    if match:
                        key = f"{match.group(1)}::{match.group(2)}"
                        slot = self.slots.get(key)
                        callee = slot.function if slot else None
                        if callee is not None:
                            self.add_edge(function, callee, "seam", via=key)
                        names[match.group(2)] = {
                            "id": callee.id if callee else None,
                            "name": callee.name if callee else match.group(2),
                            "via": key,
                        }
                elif kind == "fn_ref":
                    callee = self.choose(target, function)
                    if callee is not None and function.subsystem != "seams":
                        self.add_edge(function, callee, "ref")
            self.call_names[function.id] = names
        for row in self.data("hazard_edges.csv"):
            if row["reason"].startswith("seam slot"):
                continue
            for caller in self.by_name.get(row["caller"], []):
                callee = self.through_native(self.choose(row["target"], caller))
                if callee is not None:
                    self.add_edge(caller, callee, "dispatch", reason=row["reason"])
        for row in self.data("coupling.csv"):
            if row["kind"] == "shared_write":
                self.shared[(row["from"], row["to"])].append(row["target"])

    def add_edge(self, caller, callee, kind, via="", reason=""):
        edge = caller.out.get(callee.id)
        if edge is None:
            edge = Edge()
            caller.out[callee.id] = edge
            callee.into[caller.id] = edge
        edge.kinds.add(kind)
        edge.via = edge.via or via
        edge.reason = edge.reason or reason

    def load_data_items(self):
        classes = {}
        for row in self.data("state_classes.csv"):
            key = row["name"]
            if row["kind"] == "game-region":
                key = f"Game::{key}"
            classes[key] = row
        kind_map = {
            "global_read": ("global", "read"),
            "global_write": ("global", "write"),
            "global_maybe_written": ("global", "maybe"),
            "global_escape": ("global", "escape"),
            "field_read": ("field", "read"),
            "field_write": ("field", "write"),
            "field_maybe_written": ("field", "maybe"),
            "field_escape": ("field", "escape"),
        }
        globals_defined = {row["name"]: row for row in self.data("globals.csv")}
        for function in self.functions:
            for kind, target in function.rows:
                if kind.startswith("deref"):
                    function.pointer_accesses += 1
                if kind not in kind_map:
                    continue
                item_kind, access = kind_map[kind]
                item = self.items.get(target)
                if item is None:
                    row = classes.get(target, {})
                    defined = globals_defined.get(target.split("@")[0], {})
                    item = {
                        "name": target,
                        "kind": item_kind,
                        "class": row.get("class", ""),
                        "note": row.get("note", ""),
                        "ctype": defined.get("type", ""),
                        "file": defined.get("defined", ""),
                        "read": [],
                        "write": [],
                        "maybe": [],
                        "escape": [],
                    }
                    self.items[target] = item
                item[access].append(function.id)
                (function.writes if access in ("write", "maybe") else function.reads).append((target, access))

    def load_types(self):
        for relative in sorted(self.source_files):
            raw, masked, starts = self.source_for(relative)
            for match in TYPE_DEFINITION.finditer(masked):
                name = match.group(1)
                end = close_of(masked, match.end() - 1, "{", "}")
                if end < 0:
                    continue
                known = self.types.get(name)
                rank = (relative.endswith(".h"), end - match.start())
                if known is None or rank > known["rank"]:
                    self.types[name] = {
                        "name": name,
                        "file": relative,
                        "line": line_of(starts, match.start()),
                        "body": match.end(),
                        "end": end,
                        "rank": rank,
                    }

    def source_for(self, relative):
        if relative not in self.sources:
            raw = (self.root / relative).read_text(encoding="utf-8", errors="replace")
            self.sources[relative] = (raw, mask_source(raw), line_starts(raw))
        return self.sources[relative]

    def type_fields(self, name):
        """The fields of a struct in declaration order, with the declaration text of each."""
        known = self.types.get(name)
        if known is None:
            return []
        _, masked, starts = self.source_for(known["file"])
        fields, last, i = [], known["body"], known["body"]
        while i < known["end"]:
            c = masked[i]
            if c == "{":
                if masked[last:i].strip().endswith(")") or masked[last:i].strip().endswith("const"):
                    i = close_of(masked, i, "{", "}")
                last = i + 1
            elif c == "}":
                last = i + 1
            elif c == ";":
                declaration = re.sub(r"\b(?:public|private|protected)\s*:", "", " ".join(masked[last:i].split())).strip()
                last = i + 1
                match = re.search(r"\s\**&?([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*(?::\s*\d+)?$", declaration)
                if match and "(" not in declaration:
                    fields.append({"name": match.group(1), "decl": declaration, "line": line_of(starts, i)})
            i += 1
        return fields

    def parse_flows(self):
        for function in self.functions:
            definition = getattr(function, "definition", None)
            if definition is None:
                continue
            raw, masked, starts = self.sources[function.file]
            names = self.call_names[function.id]
            parser = FlowParser(masked, raw, starts, names.get)
            try:
                function.flow = parser.parse(definition.body, definition.end)
                function.flow_ok = True
            except (FlowError, ValueError, IndexError, RecursionError):
                parser.i, parser.end = definition.body, definition.end
                calls = parser.calls(definition.body, definition.end)
                function.flow = [
                    {"t": "call", "text": "", "line": call["line"], "calls": [call]} for call in calls
                ]
            self.annotate(function, function.flow)

    def annotate(self, function, nodes):
        for node in nodes:
            for call in node.get("calls", []):
                self.annotate_call(function, call)
            for key in ("then", "else", "body"):
                self.annotate(function, node.get(key, []))
            for case in node.get("cases", []):
                self.annotate(function, case["body"])

    def annotate_call(self, function, call):
        callee = self.functions[call["id"]] if call["id"] is not None else None
        slot = self.slots.get(call["via"])
        params = parameter_names(slot.params if slot else (callee.params if callee else ""))
        values = call.pop("values")
        call["args"] = [
            {"param": params[i] if i < len(params) else "", "value": value} for i, value in enumerate(values)
        ]
        call["doc"] = slot.doc if slot else ""
        call["subsystem"] = callee.subsystem if callee else ""
        if callee is not None:
            edge = function.out.get(callee.id)
            if edge is not None:
                edge.sites.append({"line": call["line"], "args": call["args"]})

    def stats(self):
        placed = sum(1 for f in self.functions if f.line)
        parsed = sum(1 for f in self.functions if f.flow_ok)
        edges = sum(len(f.out) for f in self.functions)
        seam_edges = sum(1 for f in self.functions for e in f.out.values() if "seam" in e.kinds)
        resolved_slots = sum(1 for s in self.slots.values() if s.function is not None)
        return {
            "functions": len(self.functions),
            "placed": placed,
            "flow_parsed": parsed,
            "edges": edges,
            "seam_edges": seam_edges,
            "slots": len(self.slots),
            "slots_resolved": resolved_slots,
            "data_items": len(self.items),
            "types": len(self.types),
        }

    def index(self):
        functions = [
            [f.name, f.file, f.subsystem, f.address, len(f.into), len(f.out), int(f.dead)]
            for f in self.functions
        ]
        subsystems = []
        counts = Counter(f.subsystem for f in self.functions)
        for name, (band, blurb) in SUBSYSTEMS.items():
            subsystems.append({
                "name": name,
                "band": band,
                "blurb": blurb,
                "functions": counts[name],
                "hidden": name in HIDDEN_FROM_MAP,
            })
        return {
            "commit": self.commit,
            "root": self.root.as_posix(),
            "functions": functions,
            "subsystems": subsystems,
            "edges": self.subsystem_edges(),
            "items": [[item["name"], item["kind"], item["class"]] for item in self.items.values()],
            "types": sorted(self.types),
            "files": sorted(self.source_files),
            "tick": self.tick_phases(),
        }

    def subsystem_edges(self):
        totals = Counter()
        seam_totals = Counter()
        for function in self.functions:
            for callee_id, edge in function.out.items():
                callee = self.functions[callee_id]
                if callee.subsystem == function.subsystem or edge.kinds == {"ref"}:
                    continue
                pair = (function.subsystem, callee.subsystem)
                totals[pair] += 1
                if "seam" in edge.kinds:
                    seam_totals[pair] += 1
        return [
            {"from": a, "to": b, "calls": n, "seam": seam_totals[(a, b)], "shared": len(self.shared.get((a, b), []))}
            for (a, b), n in sorted(totals.items())
        ]

    def tick_phases(self):
        function = next(iter(self.by_name.get("RunGameSteps", [])), None)
        if function is None:
            return {"function": None, "phases": []}
        phases = []
        seen = set()

        def visit(nodes):
            for node in nodes:
                for call in node.get("calls", []):
                    if call["via"] and call["id"] is not None and call["id"] not in seen:
                        seen.add(call["id"])
                        phases.append({"id": call["id"], "name": call["name"], "subsystem": call["subsystem"], "doc": call["doc"]})
                for key in ("then", "else", "body"):
                    visit(node.get(key, []))

        visit(function.flow)
        return {"function": function.id, "phases": phases}

    def function_payload(self, function_id):
        function = self.functions[function_id]
        slot = next((s for s in self.slots.values() if s.function is function), None)

        def edge_view(other_id, edge, outgoing):
            other = self.functions[other_id]
            sites = edge.sites if outgoing else []
            return {
                "id": other.id,
                "name": other.name,
                "subsystem": other.subsystem,
                "kinds": sorted(edge.kinds),
                "via": edge.via,
                "reason": edge.reason,
                "line": sites[0]["line"] if sites else 0,
                "args": sites[0]["args"] if sites else [],
                "sites": len(sites),
            }

        callees = [edge_view(i, e, True) for i, e in function.out.items()]
        callees.sort(key=lambda c: (c["line"] == 0, c["line"], c["name"]))
        callers = [edge_view(i, e, False) for i, e in function.into.items()]
        for caller in callers:
            edge = function.into[caller["id"]]
            if edge.sites:
                caller["args"] = edge.sites[0]["args"]
        callers.sort(key=lambda c: (c["subsystem"], c["name"]))
        return {
            "id": function.id,
            "name": function.name,
            "file": function.file,
            "address": function.address,
            "subsystem": function.subsystem,
            "line": function.line,
            "end": function.end,
            "signature": function.signature,
            "flow": function.flow,
            "flowOk": function.flow_ok,
            "dead": function.dead,
            "callers": callers,
            "callees": callees,
            "reads": self.access_view(function.reads),
            "writes": self.access_view(function.writes),
            "pointerAccesses": function.pointer_accesses,
            "slot": self.slot_view(slot) if slot else None,
        }

    def access_view(self, accesses):
        view = []
        seen = set()
        for name, access in accesses:
            if name in seen:
                continue
            seen.add(name)
            item = self.items[name]
            view.append({"name": name, "access": access, "class": item["class"], "note": item["note"]})
        return sorted(view, key=lambda v: v["name"])

    def slot_view(self, slot):
        return {
            "key": slot.key,
            "subsystem": slot.subsystem,
            "doc": slot.doc,
            "params": slot.params,
            "file": slot.header,
            "line": slot.line,
            "function": slot.function.id if slot.function else None,
            "functionName": slot.function.name if slot.function else slot.name,
        }

    def function_brief(self, function_id):
        function = self.functions[function_id]
        return {"id": function.id, "name": function.name, "subsystem": function.subsystem, "file": function.file}

    def item_payload(self, name):
        item = self.items.get(name)
        if item is None:
            return None
        declaration = self.item_declaration(name, item)
        view = {key: item[key] for key in ("name", "kind", "class", "note", "ctype")}
        view["declaration"] = declaration
        for access in ("read", "write", "maybe", "escape"):
            view[access] = [self.function_brief(i) for i in sorted(set(item[access]))]
        owner = name.split("::")[0] if "::" in name else ""
        view["owner"] = owner if owner in self.types else ""
        return view

    def item_declaration(self, name, item):
        if "::" in name:
            owner, member = name.rsplit("::", 1)
            known = self.types.get(owner)
            if known is None:
                return None
            for declared in self.type_fields(owner):
                if declared["name"] == member:
                    return {"file": known["file"], "line": declared["line"], "text": declared["decl"]}
            return {"file": known["file"], "line": known["line"], "text": ""}
        bare = name.split("@")[0]
        if item["file"] and item["file"] in self.source_files:
            _, masked, starts = self.source_for(item["file"])
            match = re.search(rf"\b{re.escape(bare)}\b", masked)
            if match:
                return {"file": item["file"], "line": line_of(starts, match.start()), "text": item["ctype"]}
        return None

    def type_payload(self, name):
        known = self.types.get(name)
        if known is None:
            return None
        fields = []
        for declared in self.type_fields(name):
            item = self.items.get(f"{name}::{declared['name']}", {})
            fields.append({
                **declared,
                "reads": len(set(item.get("read", []))),
                "writes": len(set(item.get("write", []))),
                "class": item.get("class", ""),
                "note": item.get("note", ""),
            })
        return {"name": name, "file": known["file"], "line": known["line"], "fields": fields}

    def subsystem_payload(self, name):
        members = [f for f in self.functions if f.subsystem == name]
        if not members and name not in SUBSYSTEMS:
            return None
        member_ids = {f.id for f in members}
        slots = [self.slot_view(s) for s in self.slots.values() if s.subsystem == name]
        called_from_outside = Counter()
        imports = defaultdict(Counter)
        for function in members:
            for caller in function.into:
                if caller not in member_ids:
                    called_from_outside[function.id] += 1
            for callee_id in function.out:
                callee = self.functions[callee_id]
                if callee_id not in member_ids:
                    imports[callee.subsystem][callee_id] += 1
        owned = []
        for item in self.items.values():
            if item["class"] not in ("sim", "mixed") or not item["write"]:
                continue
            writers = {self.functions[i].subsystem for i in item["write"]}
            if writers == {name}:
                owned.append(item["name"])
        files = Counter(f.file for f in members)
        return {
            "name": name,
            "blurb": SUBSYSTEMS.get(name, (0, ""))[1],
            "functions": len(members),
            "slots": slots,
            "entry": [{**self.function_brief(i), "callers": n} for i, n in called_from_outside.most_common(30)],
            "imports": {
                sub: [self.function_brief(i) for i, _ in counter.most_common(12)]
                for sub, counter in sorted(imports.items(), key=lambda pair: -sum(pair[1].values()))
            },
            "owned": sorted(owned)[:40],
            "ownedCount": len(owned),
            "files": [{"file": file, "functions": n} for file, n in files.most_common()],
        }

    def edge_payload(self, source, target):
        calls = []
        for function in self.functions:
            if function.subsystem != source:
                continue
            for callee_id, edge in function.out.items():
                callee = self.functions[callee_id]
                if callee.subsystem == target and edge.kinds != {"ref"}:
                    calls.append({
                        "caller": self.function_brief(function.id),
                        "callee": self.function_brief(callee.id),
                        "kinds": sorted(edge.kinds),
                        "via": edge.via,
                        "args": edge.sites[0]["args"] if edge.sites else [],
                    })
        calls.sort(key=lambda c: (c["callee"]["name"], c["caller"]["name"]))
        return {"from": source, "to": target, "calls": calls, "shared": sorted(self.shared.get((source, target), []))}

    def file_payload(self, relative):
        if relative not in self.source_files:
            return None
        members = sorted((f for f in self.functions if f.file == relative), key=lambda f: f.line)
        return {
            "file": relative,
            "functions": [{**self.function_brief(f.id), "line": f.line, "signature": f.signature} for f in members],
        }

    def source_payload(self, relative):
        """A source file's text, for paths inside the scanned source folders only."""
        if relative not in self.source_files:
            return None
        return {"file": relative, "text": self.source_for(relative)[0]}


def make_handler(model, port):
    allowed_hosts = {f"127.0.0.1:{port}", f"localhost:{port}"}

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args):
            pass

        def send_body(self, body, content_type, status=200):
            self.send_response(status)
            self.send_header("Content-Type", content_type + "; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(body)

        def send_json(self, value):
            if value is None:
                self.send_body(b'{"error":"not found"}', "application/json", 404)
            else:
                self.send_body(json.dumps(value, separators=(",", ":")).encode(), "application/json")

        def do_GET(self):
            if self.headers.get("Host") not in allowed_hosts:
                self.send_body(b"forbidden host", "text/plain", 403)
                return
            url = urlparse(self.path)
            if url.path.startswith("/api/"):
                query = {key: values[0] for key, values in parse_qs(url.query).items()}
                try:
                    self.send_json(self.api(url.path[5:], query))
                except (KeyError, IndexError, ValueError):
                    self.send_json(None)
                return
            name = unquote(url.path).lstrip("/") or "index.html"
            path = UI / name
            if "/" not in name and path.suffix in UI_TYPES and path.is_file():
                self.send_body(path.read_bytes(), UI_TYPES[path.suffix])
            else:
                self.send_body(b"not found", "text/plain", 404)

        def api(self, route, query):
            if route == "index":
                return model.index()
            if route == "fn":
                index = int(query["id"])
                return model.function_payload(index) if index >= 0 else None
            if route == "item":
                return model.item_payload(query["name"])
            if route == "type":
                return model.type_payload(query["name"])
            if route == "sub":
                return model.subsystem_payload(query["name"])
            if route == "edge":
                return model.edge_payload(query["from"], query["to"])
            if route == "file":
                return model.file_payload(query["path"])
            if route == "src":
                return model.source_payload(query["path"])
            return None

    return Handler


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", type=int, default=8123)
    parser.add_argument("--no-open", action="store_true", help="do not open a browser")
    parser.add_argument("--stats", action="store_true", help="print how much of the model resolved and exit")
    args = parser.parse_args(argv)
    print("codemap: reading data/ and parsing the source...", flush=True)
    model = Model()
    stats = model.stats()
    print(
        f"codemap: {stats['functions']} functions ({stats['placed']} located, {stats['flow_parsed']} flow charts), "
        f"{stats['edges']} call edges ({stats['seam_edges']} through seams), "
        f"{stats['slots_resolved']}/{stats['slots']} seam slots resolved, "
        f"{stats['data_items']} data items, {stats['types']} types"
    )
    if args.stats:
        return 0
    try:
        server = ThreadingHTTPServer(("127.0.0.1", args.port), make_handler(model, args.port))
    except OSError as error:
        print(f"codemap: cannot listen on 127.0.0.1:{args.port} ({error.strerror}); try --port", file=sys.stderr)
        return 1
    url = f"http://127.0.0.1:{args.port}/"
    print(f"codemap: serving {url} (Ctrl-C to stop)", flush=True)
    if not args.no_open:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
