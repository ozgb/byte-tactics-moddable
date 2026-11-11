"""Write docs/schedule.md, 3.1c's order of effects (#933), and check that it is up to date.

    uv run tools/schedule.py --write                    # regenerate docs/schedule.md
    uv run tools/schedule.py --check                    # fail if docs/schedule.md differs from what the inputs give
    uv run tools/schedule.py --check --build-dir D      # read the syntax trees of D (default build/rwsets)
    uv run tools/test_schedule.py                       # tests, on hand-written programs and rows

The tick, its phases and passes come from the marks the access log (#835) and the harness put in the source: the
phase marks of the tick function, each pass's scope and each loop's entity mark, found in the text since they are
compiled only into the access-log and harness builds. The order of entry points within a part of a pass is the
order of the call sites in the syntax trees (tools/rwsets.py's walker, which also gives each function's reads and
writes), with calls inside the subsystem inlined and seam calls (interfaces/seams.h, src/seams/native.cpp and the
slot rows of data/hazard_edges.csv) named `subsystem.Slot`. Each cross-subsystem call is classed by whether the
caller uses its value and whether what it runs writes state; each `immediate` row of data/dependencies.csv is placed
at the first write step before a read step (Splits). LOOPS and TICK_STEPS describe the loops and the tick's engine
steps in behavioural terms; --check also fails when a pass, loop or engine step has no description or a description
has no pass, loop or step, and when an immediate row cannot be placed. The page goes into the oracle kit, so it
names entry points and subsystems only, never another function or a source file.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import sys
from collections import Counter, defaultdict
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import astcache
import rwsets
from rwsets import ROOT

DEFAULT_BUILD_DIR = ROOT / "build/rwsets"
DATA = ROOT / "data"
DOC = ROOT / "docs" / "schedule.md"
SIMS = ["wind_meteors", "economy", "features", "ai", "pathfinding", "weapons", "targeting", "los_radar", "orders", "units",
        "players", "map"]
PHASES = ["net", "units", "projectiles", "explosions", "players", "features", "wind", "meteors", "tail"]
SEAM_HINT = re.compile(r"\[(\w+)::(\w+)\]$")
SLOT_REASON = re.compile(r"^(?:seam slot filled by InstallNativeSeams|slot of the PathWorldView table filled by NativePathWorld"
                         r" \([^)]*\)), (\w+)::(\w+),")
CONTROL = {"IfStmt": 0, "WhileStmt": 0, "SwitchStmt": 0, "DoStmt": 1, "ForStmt": 2}
BRANCHING = {"IfStmt", "WhileStmt", "SwitchStmt", "DoStmt", "ForStmt", "ConditionalOperator"}
CONSTRUCTS = {"CXXConstructExpr", "CXXTemporaryObjectExpr"}
WRAPPERS = {"ExprWithCleanups", "ParenExpr", "CStyleCastExpr", "ImplicitCastExpr", "CXXBindTemporaryExpr",
            "MaterializeTemporaryExpr"}
PASS_MARK = re.compile(r"\bAccessLogPassScope\s+\w+\(ACCESS_PASS_(\w+)\)")
ENTITY_MARK = re.compile(r"\bAccessLogEntity\(\)")
PHASE_MARK = re.compile(r"\bHarnessPhase\((\d+)\)")
TICK_BEGIN = re.compile(r"\bHarnessBeforeStep\(\)")
TICK_END = re.compile(r"\bHarnessAfterStep\(\)")
SEAMS_HEADER = ROOT / "interfaces" / "seams.h"
SEAM_FIELD = re.compile(r"^\s*(\w+)\*\s+(\w+);", re.M)
NATIVE = ROOT / "src" / "seams" / "native.cpp"
NATIVE_TABLE = re.compile(r"^static (\w+) s_(\w+);", re.M)
NATIVE_SLOT = re.compile(r"^\s*s_(\w+)\.(\w+)\s*=\s*(?:\(.*\))?\s*(\w+);", re.M)

EFFECTS = {"RandomInt": "draw rng", "GameRand": "draw crt", "RecordSimEvent": "sim event"}
TRANSPARENT = {None, "seams", "platform", "game"}
ENGINE = {"presentation", "harness", "network", "files"}


class OrderWalker(rwsets.Walker):
    """rwsets' walker that also keeps the calls in the order they run, with their line and whether the result is used."""

    def __init__(self, unit, name):
        super().__init__(unit, name)
        self.calls: list[list] = []
        self.stack: list[dict] = []
        self.pending: dict | None = None
        self.branches = 0

    def visit(self, node: dict, mode: str = "R") -> None:
        if node.get("kind") in BRANCHING or (node.get("kind") == "BinaryOperator" and node.get("opcode") in ("&&", "||")):
            self.branches += 1
        if node.get("kind") in CONSTRUCTS:
            name = re.sub(r"^(?:const |struct |class )+", "", node.get("type", {}).get("qualType", ""))
            if re.fullmatch(r"\w+(?:::\w+)*", name):
                self.calls.append([node["_line"], "call", "%s::%s" % (name, name.split("::")[-1]), False, id(node)])
        self.stack.append(node)
        try:
            super().visit(node, mode)
        finally:
            self.stack.pop()

    def record(self, kind: str, target: str) -> None:
        super().record(kind, target)
        if kind in ("call", "indirect_call") and self.pending is not None:
            node, self.pending = self.pending, None
            self.calls.append([node["_line"], kind, target, self.used(node), id(node)])

    def used(self, node: dict) -> bool:
        """Whether the value of a call is used: false when the call is a statement of its own or cast to void."""
        child = node
        for parent in reversed(self.stack[:-1]):
            kind = parent.get("kind")
            if kind == "CStyleCastExpr" and parent.get("type", {}).get("qualType") == "void":
                return False
            if kind in WRAPPERS:
                child = parent
                continue
            if kind in ("CompoundStmt", "CaseStmt", "DefaultStmt", "LabelStmt"):
                return False
            if kind in CONTROL:
                inner = parent.get("inner", [])
                index = CONTROL[kind]
                return len(inner) > index and inner[index] is child
            return True
        return False

    def call(self, node: dict) -> None:
        start = len(self.calls)
        self.pending = node
        super().call(node)
        self.pending = None
        mine = [i for i in range(start, len(self.calls)) if self.calls[i][4] == id(node)]
        if mine:
            self.calls.append(self.calls.pop(mine[0]))


class OrderAnalyzer(rwsets.Analyzer):
    """rwsets' analyzer with OrderWalker, keeping each function's calls in order, its end line and its return type."""

    def function(self, node: dict, name: str) -> None:
        rel = self.relative(node["_file"])
        if rel is None:
            return
        rwsets.SIM.current_file = rel
        walker = OrderWalker(self, name)
        walker.prepare(node)
        for child in node.get("inner", ()):
            if child["kind"].endswith("Stmt") or child["kind"] == "CXXCtorInitializer":
                walker.visit(child, "R")
        returns = node.get("type", {}).get("qualType", "").split("(")[0].strip()
        self.functions.append({
            "function": name, "file": rel, "start": node["_line"], "end": node["_end"],
            "items": sorted(walker.items), "calls": [c[:4] for c in walker.calls], "branches": walker.branches,
            "returns": returns,
        })


def analyse(job) -> list[dict]:
    """The functions of one translation unit, from the shared syntax tree."""
    unit, _ = rwsets.analyse_unit(job, OrderAnalyzer)
    return unit.functions


def extract(build_dir: Path, jobs: int = 8) -> list[dict]:
    """Every function definition of src/, once, with its ordered calls."""
    astcache.configure(build_dir)
    commands = rwsets.compile_commands(build_dir, [])
    sys.setrecursionlimit(100000)
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(analyse, [(ROOT, argv, directory) for _, argv, directory in commands], chunksize=4))
    seen = set()
    out = []
    for result in results:
        for fn in result:
            key = (fn["function"], fn["file"], fn["start"])
            if key not in seen:
                seen.add(key)
                out.append(fn)
    return sorted(out, key=lambda f: (f["file"], f["start"], f["function"]))


C_NOISE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)


def blank(text: str) -> str:
    """The text with comments and literals blanked, line breaks kept."""
    return C_NOISE.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), text)


def block_ends(text: str) -> list[int]:
    """For each line (index 1 up), the line of the brace that closes the innermost block open at its start."""
    lines = text.split("\n")
    stack: list[int] = []
    close_of: dict[tuple, int] = {}
    starts: list[tuple] = []
    for number, line in enumerate(lines, 1):
        starts.append(tuple(stack))
        for ch in line:
            if ch == "{":
                stack.append(number)
            elif ch == "}" and stack:
                close_of[tuple(stack)] = number
                stack.pop()
    return [0] + [close_of.get(inner, len(lines)) if inner else len(lines) for inner in starts]


@dataclass
class Mark:
    kind: str
    line: int
    end: int
    name: str = ""


def file_marks(text: str) -> list[Mark]:
    """The pass, entity, phase and tick marks of a source file, each with the last line it covers."""
    clean = blank(text)
    ends = block_ends(clean)
    marks: list[Mark] = []
    for number, line in enumerate(clean.split("\n"), 1):
        m = PASS_MARK.search(line)
        if m:
            marks.append(Mark("pass", number, ends[number], m.group(1).lower()))
        if ENTITY_MARK.search(line):
            marks.append(Mark("entity", number, ends[number]))
        m = PHASE_MARK.search(line)
        if m:
            marks.append(Mark("phase", number, number, m.group(1)))
        if TICK_BEGIN.search(line):
            marks.append(Mark("tick", number, number, "begin"))
        if TICK_END.search(line):
            marks.append(Mark("tick", number, number, "end"))
    entities = [m for m in marks if m.kind == "entity"]
    for a, b in zip(entities, entities[1:]):
        if b.end == a.end and b.line <= a.end:
            a.end = b.line - 1
    return marks


@dataclass
class Node:
    """One step in an order: an entry point of another subsystem, an effect, or a call into the engine."""
    kind: str
    label: str
    sub: str
    used: bool = False
    caller: str = ""
    slot: str = ""
    own: set = field(default_factory=set)
    children: list = field(default_factory=list)


@dataclass
class Fn:
    name: str
    file: str
    start: int
    end: int
    calls: list
    items: set
    branches: int
    returns: str
    sub: str | None


class Program:
    """The functions, their subsystems, the seams and the marks: what the orders are walked over."""

    def __init__(self, functions: list[dict], subsystems: list[dict], edges: list[dict], seams_header: str,
                 native: str, sources: dict[str, str]):
        owner = {(r["function"], r["file"]): r["subsystem"] for r in subsystems}
        self.fns: dict[str, list[Fn]] = defaultdict(list)
        for f in functions:
            self.fns[f["function"]].append(Fn(f["function"], f["file"], f["start"], f["end"], f["calls"],
                                              {tuple(i) for i in f["items"]}, f["branches"], f["returns"],
                                              owner.get((f["function"], f["file"]))))
        self.seam_owner = {struct: name for struct, name in SEAM_FIELD.findall(seams_header) if name in SIMS}
        self.slots: dict[str, set[str]] = defaultdict(set)
        self.resolved: dict[str, list[str]] = defaultdict(list)
        tables = {name: struct for struct, name in NATIVE_TABLE.findall(native)}
        for table, slot, impl in NATIVE_SLOT.findall(native):
            if table in tables:
                self.slots["%s::%s" % (tables[table], slot)].add(impl)
        for r in edges:
            m = SLOT_REASON.match(r["reason"])
            if m:
                self.slots["%s::%s" % m.groups()].add(r["target"])
            else:
                self.resolved[r["caller"]].append(r["target"])
        self.marks: dict[str, list[Mark]] = {path: file_marks(text) for path, text in sources.items()}
        self.memo: dict[tuple, list[Node]] = {}
        self.memo_own: dict[tuple, set] = {}
        self.entries: dict[str, tuple[list[Node], set]] = {}

    def lookup(self, name: str, near: str = "") -> list[Fn]:
        """The definitions of a name, those in the caller's file first when there are several."""
        found = self.fns.get(name, [])
        same = [f for f in found if f.file == near]
        return same or found

    def marks_in(self, fn: Fn) -> list[Mark]:
        return [m for m in self.marks.get(fn.file, []) if fn.start <= m.line <= fn.end]

    def targets(self, fn: Fn, kind: str, target: str) -> tuple[str, list[str]]:
        """('slot', [slot]) for a seam call, else ('call', [function names]) the call can reach."""
        if kind == "call":
            return "call", [target]
        m = SEAM_HINT.search(target)
        if m and "%s::%s" % m.groups() in self.slots:
            return "slot", ["%s::%s" % m.groups()]
        if target.startswith("virtual "):
            method = target.split("::")[-1]
            return "call", [t for t in self.resolved.get(fn.name, []) if t.split("::")[-1] == method]
        return "call", list(self.resolved.get(fn.name, []))

    def expand(self, fn: Fn, sub: str, stack: tuple = (), lines: tuple | None = None, own: set | None = None) -> list[Node]:
        """The steps of a function run on behalf of `sub`, with the calls inside `sub` inlined."""
        key = (fn.name, fn.file, fn.start, sub)
        if lines is None and key in self.memo:
            if own is not None:
                own |= self.memo_own[key]
            return self.memo[key]
        mine: set = {(fn.name, fn.file, fn.start)}
        out: list[Node] = []
        stack = stack + (key,)
        for line, kind, target, used in fn.calls:
            if lines is not None and not lines[0] <= line <= lines[1]:
                continue
            how, names = self.targets(fn, kind, target)
            if how == "slot":
                slot = names[0]
                callee = self.seam_owner.get(slot.split("::")[0])
                if callee is None or callee == sub:
                    for impl in sorted(self.slots[slot]):
                        for g in self.lookup(impl):
                            if (g.name, g.file, g.start, sub) not in stack:
                                out += self.expand(g, sub, stack, own=mine)
                else:
                    out.append(Node("entry", self.slot_label(slot), callee, used, fn.name, slot))
                continue
            for name in names:
                if name in EFFECTS:
                    out.append(Node("effect", EFFECTS[name], sub))
                    continue
                found = self.lookup(name, fn.file)
                inside = [g for g in found if g.sub in TRANSPARENT or g.sub == sub]
                for g in inside or found:
                    if g.sub in ENGINE:
                        out.append(Node("engine", g.name, g.sub, used, fn.name))
                    elif g.sub in SIMS and g.sub != sub:
                        out.append(Node("direct", g.name, g.sub, used, fn.name))
                    elif (g.name, g.file, g.start, sub) not in stack:
                        out += self.expand(g, sub, stack, own=mine)
        if lines is None:
            self.memo[key] = out
            self.memo_own[key] = mine
        if own is not None:
            own |= mine
        return out

    def slot_label(self, slot: str) -> str:
        struct, name = slot.split("::")
        return "%s.%s" % (self.seam_owner[struct], name)

    def entry(self, slot: str) -> tuple[list[Node], set]:
        """The steps of an entry point and the functions it runs itself, once per slot."""
        if slot not in self.entries:
            self.entries[slot] = ([], set())
            sub = self.seam_owner[slot.split("::")[0]]
            steps: list[Node] = []
            own: set = set()
            for impl in sorted(self.slots[slot]):
                for g in self.lookup(impl):
                    steps += self.expand(g, sub, own=own)
            self.entries[slot] = (steps, own)
        return self.entries[slot]

    def fn_of(self, key: tuple) -> Fn | None:
        for f in self.fns.get(key[0], []):
            if (f.file, f.start) == (key[1], key[2]):
                return f
        return None

    def items_of(self, own: set) -> set:
        out: set = set()
        for key in own:
            f = self.fn_of(key)
            if f is not None:
                out |= f.items
        return out

    def tick(self) -> tuple[Fn, dict[str, tuple[int, int]]]:
        """The function that plays one tick, and the lines of each phase in it."""
        for fns in self.fns.values():
            for f in fns:
                marks = self.marks_in(f)
                begin = [m.line for m in marks if m.kind == "tick" and m.name == "begin"]
                end = [m.line for m in marks if m.kind == "tick" and m.name == "end"]
                if begin and end:
                    bounds = [begin[0]] + [m.line for m in sorted(marks, key=lambda m: int(m.name) if m.kind == "phase" else 0)
                                           if m.kind == "phase"]
                    phases = {PHASES[i]: (bounds[i] + 1, bounds[i + 1]) for i in range(len(bounds) - 1)}
                    phases["tail"] = (bounds[-1] + 1, end[0])
                    return f, phases
        raise SystemExit("schedule: no function holds the tick marks (HarnessBeforeStep, HarnessPhase(n), HarnessAfterStep)")


@dataclass
class Part:
    """A run of one pass: the work before its loops, one loop's entity, or the work after."""
    kind: str
    index: int
    lines: tuple[int, int]
    steps: list
    own: set


@dataclass
class Pass:
    name: str
    phase: str
    holder: str
    sub: str
    parts: list


def pass_parts(program: Program, fn: Fn, sub: str, name: str) -> list[Part]:
    """The parts of a pass function: before the first loop, each entity, between and after."""
    marks = program.marks_in(fn)
    entities = [m for m in marks if m.kind == "entity"]
    begin = next(m.line for m in marks if m.kind == "pass" and m.name == name)
    end = next(m.end for m in marks if m.kind == "pass" and m.name == name)
    parts: list[Part] = []
    cursor = begin
    for i, e in enumerate(entities, 1):
        loop_start = cursor
        if e.line > cursor:
            parts.append(("between" if parts else "before", i - 1, (cursor, e.line - 1)))
        parts.append(("entity", i, (e.line, e.end)))
        cursor = e.end + 1
        del loop_start
    if cursor <= end:
        parts.append(("after", len(entities), (cursor, end)))
    out = []
    for kind, index, lines in parts:
        own: set = set()
        steps = program.expand(fn, sub, lines=lines, own=own)
        out.append(Part(kind, index, lines, steps, own))
    return out


def find_pass(program: Program, own: set) -> tuple[Fn, str] | None:
    for key in sorted(own):
        f = program.fn_of(key)
        if f is None:
            continue
        names = [m.name for m in program.marks_in(f) if m.kind == "pass"]
        if names:
            return f, names[0]
    return None


def build(program: Program) -> dict:
    """The phases of a tick in order, the steps of each and the passes they hold."""
    tick, ranges = program.tick()
    phases = []
    passes = []
    for phase in PHASES:
        own: set = set()
        steps = program.expand(tick, "game", lines=ranges[phase], own=own)
        phases.append((phase, steps))
        if phase == "tail":
            passes.append(Pass("tail", phase, "", "game", [Part("steps", 0, ranges[phase], steps, own)]))
            continue
        for step in steps:
            if step.kind != "entry":
                continue
            found = find_pass(program, program.entry(step.slot)[1])
            if found:
                fn, name = found
                passes.append(Pass(name, phase, step.label, step.sub, pass_parts(program, fn, step.sub, name)))
    return {"phases": phases, "passes": passes}


VALUE_TYPES = {"Vec3", "Vec3f", "Vector3s", "Point", "Point16", "Fixed", "PathPoint", "<anon>"}
ALLOCATIONS = {("call", "operator new"), ("call", "operator delete")}
WRITE_KINDS = ("field_write", "global_write")
READ_KINDS = ("field_read", "global_read", "field_escape", "global_escape", "field_maybe_written", "global_maybe_written")


def state_write(item: tuple) -> bool:
    """A write the classes count: a field of a type that is not a plain value, or a global."""
    kind, target = item
    if item in ALLOCATIONS:
        return True
    if kind == "field_write":
        return target.split("::")[0] not in VALUE_TYPES
    return kind == "global_write"


class Effects:
    """What each entry point does, over everything it runs, nested entry points included."""

    def __init__(self, program: Program):
        self.program = program
        self.memo: dict[str, tuple[bool, set]] = {}

    def reach(self, slot: str, stack: tuple = ()) -> tuple[bool, set]:
        """(effects, slots): whether the entry point, or one it calls, writes state, draws or emits; and the slots it reaches."""
        if slot in self.memo:
            return self.memo[slot]
        steps, own = self.program.entry(slot)
        writes = any(state_write(i) for i in self.program.items_of(own))
        slots = {slot}
        for n in steps:
            if n.kind == "effect":
                writes = True
            elif n.kind == "entry" and n.slot not in stack and n.slot != slot:
                w, s = self.reach(n.slot, stack + (slot,))
                writes = writes or w
                slots |= s
        if not stack:
            self.memo[slot] = (writes, slots)
        return writes, slots

    def view(self, slot: str) -> bool:
        """An entry point that only reads data: one function besides the seam wrapper, without branches or calls out."""
        steps, own = self.program.entry(slot)
        fns = [self.program.fn_of(k) for k in own]
        body = [f for f in fns if f is not None and f.sub not in ("seams",)]
        return not steps and len(body) <= 1 and all(f.branches == 0 for f in body)

    def classify(self, node: Node) -> str:
        writes, _ = self.reach(node.slot)
        if not node.used:
            return "intent"
        if writes:
            return "mixed"
        return "view read" if self.view(node.slot) else "query"


@dataclass
class Placement:
    """Where an immediate edge falls: the kind of split, the part and entry point it is in, and the steps around it."""
    kind: str
    where: str
    first: str = ""
    second: str = ""
    by_subsystem: bool = False


class Splits:
    """Places each immediate edge in the orders: the first write before a read, inside one part of a pass."""

    def __init__(self, program: Program):
        self.program = program
        self.memo: dict[tuple, bool] = {}
        targets: set = set()
        for fns in program.fns.values():
            for f in fns:
                targets |= {t for _, t in f.items}
        self.targets = targets

    def matcher(self, item: str, loose: bool = False):
        """A test on (kind, target) rows for the item, or None when no row of any function can name it (or `loose`)."""
        if loose or ": " in item or item.startswith(("rng.", "unresolved:")):
            return None
        if "::" in item:
            return (lambda t: t == item) if item in self.targets else None
        names = {t for t in self.targets if t.split("@")[0] == item}
        return (lambda t: t.split("@")[0] == item) if names else None

    def hits(self, sub: str, own: set, who: str, match, write: bool) -> bool:
        if sub != who:
            return False
        if match is None:
            return True
        kinds = WRITE_KINDS if write else READ_KINDS
        return any(k in kinds and match(t) for k, t in self.program.items_of(own))

    def reach(self, slot: str, who: str, item: str, match, write: bool, stack: tuple = ()) -> bool:
        key = (slot, who, item if match is not None else "", write)
        if key in self.memo:
            return self.memo[key]
        steps, own = self.program.entry(slot)
        sub = self.program.seam_owner[slot.split("::")[0]]
        found = self.hits(sub, own, who, match, write) or any(
            self.reach(n.slot, who, item, match, write, stack + (slot,))
            for n in steps if n.kind == "entry" and n.slot not in stack and n.slot != slot)
        if not stack:
            self.memo[key] = found
        return found

    def locate(self, where: str, steps: list, own: set, sub: str, writer: str, reader: str, item: str,
               stack: tuple = (), loose: bool = False) -> Placement | None:
        """The first place in these steps, or nested in one of them, where the writer's write comes before the reader's read."""
        match = self.matcher(item, loose)
        entries = [n for n in steps if n.kind == "entry"]
        w = [i for i, n in enumerate(entries) if self.reach(n.slot, writer, item, match, True)]
        r = [i for i, n in enumerate(entries) if self.reach(n.slot, reader, item, match, False)]
        loose = match is None
        if w and r and w[0] < r[-1]:
            j = next(j for j in r if j > w[0])
            return Placement("split", where, entries[w[0]].label, entries[j].label, loose)
        if w and self.hits(sub, own, reader, match, False):
            return Placement("apply", where, entries[w[0]].label, "", loose)
        if r and self.hits(sub, own, writer, match, True):
            return Placement("before", where, "", entries[r[0]].label, loose)
        for n in entries:
            if n.slot in stack:
                continue
            if self.reach(n.slot, writer, item, match, True) and self.reach(n.slot, reader, item, match, False):
                inner_steps, inner_own = self.program.entry(n.slot)
                found = self.locate("%s, in %s" % (where, code(n.label)), inner_steps, inner_own, n.sub, writer, reader, item, stack + (n.slot,), loose)
                if found:
                    return found
        return None

    def order(self, steps: list, own: set, sub: str, writer: str, reader: str, item: str, loose: bool = False) -> str:
        """`reader-first` when the steps reach both sides but the reader first only, `absent` when they reach one side or none."""
        match = self.matcher(item, loose)
        entries = [n for n in steps if n.kind == "entry"]
        w = [i for i, n in enumerate(entries) if self.reach(n.slot, writer, item, match, True)] + \
            ([-1] if self.hits(sub, own, writer, match, True) else [])
        r = [i for i, n in enumerate(entries) if self.reach(n.slot, reader, item, match, False)] + \
            ([-1] if self.hits(sub, own, reader, match, False) else [])
        if w and r:
            return "reader-first"
        return "absent"


CLASSES = ["view read", "query", "intent", "mixed"]
PLACEMENTS = {"split": "split", "apply": "apply before continuing", "before": "write before calling"}
TICK_STEPS = {
    "HandleNetPackets": "the network packets received since the last tick are applied (network play only: a test-mode run "
                        "does not run this phase)",
    "UpdateAutoFollow": "presentation: when auto-follow is on, the followed unit is cycled",
    "DrainSimEvents": "the sim-event queue is drained (`docs/sim-events.md`)",
    "UpdateCameraFollow": "presentation: the camera follows the followed unit",
    "UpdateParticles": "presentation: the particles advance",
    "PacketManager::SendAllQueued": "the queued network packets are sent (network play only)",
}
TICK_HIDDEN = {"AccumulateProfileTime"}
LOOPS = {
    ("units", 1): "each unit of each player. The players are taken in slot order, 0 to 9; a player is skipped unless it is "
                  "active, its type is 1, 2 or 3 and its index is not 10. Within a player, its unit slots are taken from the "
                  "first to the last in index order, and a slot is visited when it holds a unit (its type index is not 0).",
    ("projectiles", 1): "each projectile slot, in index order, from 0 to the count of projectiles as it was when the pass "
                        "began. Projectiles that the pass adds are not visited in the same pass.",
    ("explosions", 1): "each of the 100 slots of the exploded pieces, in index order; an empty slot does nothing.",
    ("explosions", 2): "each debris record, in index order, from 0 up to the count of debris records.",
    ("players", 1): "each player slot, in slot order from 0 to 9; a player is skipped unless it is active, its type is 1, "
                    "2 or 3 and its index is not 10.",
    ("features", 1): "each record of the list of burning and animated features, from the head of the used list along its "
                     "links.",
    ("wind", 1): "the wind once per tick: the pass has one entity.",
    ("meteors", 1): "the meteor shower once per tick: the pass has one entity.",
}
LIMITS = """\
The orders are read from the program, not from a run. That decides what they claim:

- **Program order.** A step is listed where its call stands. The conditions around a call are left out: a step runs
  only when its conditions hold, and a call in a loop, or reached on several paths, can run more than once. A list
  names each step once, where it first occurs.
- **Nested calls** are followed through everything the subsystem runs itself, through the virtual calls and function
  pointers whose targets are known, and through the engine's shared helpers. Calls into presentation, network and file
  code are left out of the lists, and so is the test harness.
- **Writes**, for the classes, are the writes to a field of a record or to a variable of the game that the program
  shows, anywhere in what a call runs (nested entry points included), and random draws and sim events. Writes to the
  fields of plain values (positions, points, fixed-point numbers) are not counted, because a local value and one in
  the state look alike there, and neither are writes through a bare pointer. A call whose value is used and that
  writes state only in those ways is classed as a view read or a query.
- **Split points** place each `immediate` row of coupling/dependencies.csv (the access log, run on the scenario
  suite) at the first step that writes its item before a later step that reads it. Where the program names the item,
  the two steps must touch it. Where it does not (a region of the game record, an allocation, or an item reached under
  another name), or no such pair of steps is found, the row is placed by subsystem: at the first step of the writer
  before a later step of the reader. Those rows are marked `by subsystem`; the split they name is the earliest one that
  can be needed.
- **Across entities.** The column `across` counts the rows of a group whose item an earlier entity of the same pass
  wrote and a later one read (coupling/pass_reads.csv). Such a row needs the order of the entities, which the
  scheduler keeps, and may need no split inside the entity.
"""


def distinct(nodes: list, kinds=("entry", "effect", "direct")) -> list:
    """The steps of these kinds, each once, in the order they first occur."""
    out: dict[tuple, Node] = {}
    for n in nodes:
        if n.kind in kinds:
            out.setdefault((n.kind, n.label), n)
    return list(out.values())


def cell(text: str) -> str:
    return str(text).replace("|", "\\|")


def table(header: list[str], rows: list[list]) -> list[str]:
    return (["| " + " | ".join(header) + " |", "|" + "---|" * len(header)]
            + ["| " + " | ".join(cell(c) for c in r) + " |" for r in rows])


def code(text: str) -> str:
    return "`%s`" % text


class Page:
    """Everything the page says, computed once from the program and the data rows."""

    def __init__(self, program: Program, deps: list[dict], pass_reads: list[dict]):
        self.program = program
        self.built = build(program)
        self.effects = Effects(program)
        self.splits = Splits(program)
        self.deps = [d for d in deps if d["class"] == "immediate" and d["writer"] in SIMS and d["reader"] in SIMS
                     and d["writer"] != d["reader"]]
        self.across = {(r["pass"], r["item"], r["reader"]) for r in pass_reads}
        self.calls: dict[tuple, Counter] = defaultdict(Counter)
        self.reached: list[str] = []
        self.walk_calls()
        self.placed = [(d, self.place(d)) for d in self.deps]

    def walk_calls(self) -> None:
        """Every cross-subsystem call a tick reaches, as (caller subsystem, entry point) with the classes of its sites."""
        seen: set = set()

        def walk(steps: list, caller: str) -> None:
            for n in steps:
                if n.kind != "entry":
                    continue
                if caller != "game":
                    self.calls[(caller, n.label)][self.effects.classify(n)] += 1
                if n.slot not in seen:
                    seen.add(n.slot)
                    self.reached.append(n.slot)
                    walk(self.program.entry(n.slot)[0], n.sub)

        for _, steps in self.built["phases"]:
            walk(steps, "game")

    def place(self, d: dict) -> Placement | None:
        parts = [(p, part) for p in self.built["passes"] if p.phase == d["phase"] for part in p.parts]
        parts.sort(key=lambda x: x[1].kind != "entity")
        for loose in (False, True):
            for p, part in parts:
                found = self.splits.locate(self.part_name(p, part), part.steps, part.own, p.sub, d["writer"],
                                           d["reader"], d["item"], loose=loose)
                if found:
                    return found
        return None

    def part_name(self, p: Pass, part: Part) -> str:
        loops = sum(1 for q in p.parts if q.kind == "entity")
        if part.kind == "entity":
            return "%s, each entity%s" % (p.name, " of loop %d" % part.index if loops > 1 else "")
        if part.kind == "steps":
            return "%s, its steps" % p.name
        if part.kind == "between":
            return "%s, between loops %d and %d" % (p.name, part.index, part.index + 1)
        return "%s, %s the loop%s" % (p.name, part.kind, "s" if loops > 1 else "")

    def site_classes(self, label: str, caller: str | None = None) -> list[str]:
        seen: Counter = Counter()
        for (who, callee), classes in self.calls.items():
            if callee == label and caller in (None, who):
                seen.update(classes)
        return [c for c in CLASSES if seen[c]]

    def steps_list(self, nodes: list, caller: str) -> list[str]:
        items = distinct(nodes)
        if not items:
            return ["None."]
        out = []
        for n in items:
            if n.kind == "entry":
                classes = {self.effects.classify(m) for m in nodes if m.kind == "entry" and m.label == n.label}
                out.append("1. %s: %s" % (code(n.label), ", ".join(c for c in CLASSES if c in classes)))
            elif n.kind == "effect":
                out.append("1. " + {"draw rng": "a draw from the Park-Miller generator (`rng`)",
                                    "draw crt": "a draw from the C runtime generator (`rng.crtHoldrand`)",
                                    "sim event": "a sim event is recorded"}[n.label])
            else:
                out.append("1. a call into %s outside its interface" % n.sub)
        return out

    def tick_rows(self) -> list[list[str]]:
        rows = []
        for number, (phase, steps) in enumerate(self.built["phases"]):
            said = []
            for n in steps:
                if n.kind == "entry":
                    text = "%s (%s)" % (code(n.label), "the pass below" if self.pass_of(n.label) else "once")
                elif n.kind == "engine" and n.label in TICK_STEPS:
                    text = TICK_STEPS[n.label]
                else:
                    continue
                if text not in said:
                    said.append(text)
            rows.append([str(number) if phase != "tail" else "after", phase, "; ".join(said) or "nothing"])
        return rows

    def pass_of(self, label: str) -> Pass | None:
        return next((p for p in self.built["passes"] if p.holder == label), None)

    def groups(self) -> dict[str, list[tuple]]:
        """The placed rows by phase, grouped by writer, reader and place, with how many rows and how many go across entities."""
        out: dict[str, Counter] = defaultdict(Counter)
        across: dict[tuple, int] = Counter()
        for d, found in self.placed:
            if found is None:
                continue
            key = (d["writer"], d["reader"], found.kind, found.where, found.first, found.second, found.by_subsystem)
            out[d["phase"]][key] += 1
            names = {p.name for p in self.built["passes"] if p.phase == d["phase"]}
            if any((name, d["item"], d["reader"]) in self.across for name in names):
                across[(d["phase"],) + key] += 1
        return {phase: [(key, n, across[(phase,) + key]) for key, n in sorted(rows.items())]
                for phase, rows in out.items()}

    def summary_rows(self) -> list[list]:
        made: dict[str, Counter] = defaultdict(Counter)
        got: dict[str, Counter] = defaultdict(Counter)
        for (caller, label), classes in self.calls.items():
            callee = label.split(".")[0]
            for c in classes:
                made[caller][c] += 1
                got[callee][c] += 1
        writes: Counter = Counter()
        reads: Counter = Counter()
        inside: Counter = Counter()
        for d, found in self.placed:
            if found is None:
                continue
            writes[d["writer"]] += 1
            reads[d["reader"]] += 1
            if ", each entity" in found.where:
                inside[d["writer"]] += 1
                if d["reader"] != d["writer"]:
                    inside[d["reader"]] += 1
        rows = []
        for sub in SIMS:
            rows.append([sub] + ["%d / %d" % (made[sub][c], got[sub][c]) for c in CLASSES]
                        + [str(writes[sub]), str(reads[sub]), str(inside[sub])])
        return rows

    def unplaced(self) -> list[dict]:
        return [d for d, found in self.placed if found is None]

    def problems(self) -> list[str]:
        """What --check refuses besides a stale page: a step or a pass without its description, or one left over."""
        out = []
        names = {(p.name, part.index) for p in self.built["passes"] for part in p.parts if part.kind == "entity"}
        names |= {(p.name, 1) for p in self.built["passes"] if p.name != "tail" and not any(
            part.kind == "entity" for part in p.parts)}
        for key in sorted(names - set(LOOPS)):
            out.append("pass %s, loop %d has no description in LOOPS" % key)
        for key in sorted(set(LOOPS) - names):
            out.append("LOOPS describes pass %s, loop %d, which the marks do not hold" % key)
        engine = {n.label for _, steps in self.built["phases"] for n in steps if n.kind == "engine" and n.sub != "harness"}
        for name in sorted(engine - set(TICK_STEPS) - TICK_HIDDEN):
            out.append("the tick calls %s, which TICK_STEPS does not describe" % name)
        for name in sorted(set(TICK_STEPS) - engine):
            out.append("TICK_STEPS describes %s, which the tick does not call" % name)
        for d in self.unplaced():
            out.append("no split point for the immediate row %s -> %s, %s, phase %s" % (d["writer"], d["reader"], d["item"],
                                                                                   d["phase"]))
        return out


def render(page: Page) -> str:
    """docs/schedule.md."""
    lines = [
        "# The order of effects",
        "",
        "What 3.1c does in one tick, in what order, and where one subsystem's write is read by another before the tick",
        "ends. The scheduler of the clean-room engine (#180) owns this order: reads are views, writes are events, and",
        "where a write is read back inside one entity's update, either the entry point is split there or the event is",
        "applied before the module continues. Generated by the schedule tool from the program and the access log; do not",
        "edit by hand.",
        "",
        "## Terms",
        "",
        "- **Tick:** one game step. It runs the phases below in order.",
        "- **Phase:** one numbered part of the tick, as the traces name it (`docs/trace-format.md`).",
        "- **Pass:** the work of one phase that visits entities: the work before its loop, the loop, the work after.",
        "- **Entity:** what one turn of a pass's loop updates: a unit, a projectile, a player and so on.",
        "- **Entry point:** a slot of a subsystem's interface, written `subsystem.Slot` (`interfaces/`).",
        "- **Class** of a call from one subsystem to another's entry point, judged by everything the call runs:",
        "  - **view read:** reads data and returns it, with no logic of its own and no side effects;",
        "  - **query:** runs the other subsystem's logic and returns a value, with no side effects;",
        "  - **intent:** the caller uses no value from it: it is called for its effects (writes to state, random",
        "    draws, sim events);",
        "  - **mixed:** writes state and returns a value the caller uses, so it must be split into a query and an intent.",
        "- **Immediate edge:** a row of coupling/dependencies.csv of class `immediate`: within one phase, the reader",
        "  reads an item after the writer wrote it.",
        "- **Split point:** where an immediate edge falls in the orders below, as one of:",
        "  - **split:** a step of the writer, then a later step of the reader, in the same list: the writer's event must",
        "    be applied between the two;",
        "  - **apply before continuing:** a step of the writer, read afterwards by the code that called it: the event",
        "    must be applied when the step returns;",
        "  - **write before calling:** the caller wrote the item before calling a step of the reader: its event must be",
        "    applied before that call.",
        "",
        "## The tick",
        "",
        "Phase numbers are the trace's. A phase that calls a pass is described under Passes.",
        "",
    ]
    lines += table(["phase", "name", "what runs, in order"], page.tick_rows())
    lines += ["", "## Passes", ""]
    for p in page.built["passes"]:
        if p.name == "tail":
            continue
        lines += ["### %s" % p.name, "",
                  "Phase %s, through %s. %s" % (p.phase, code(p.holder), " ".join(
                      "Loop %d visits %s" % (i, LOOPS[(p.name, i)]) if sum(q.kind == "entity" for q in p.parts) > 1
                      else "It visits %s" % LOOPS[(p.name, i)]
                      for i in sorted({q.index for q in p.parts if q.kind == "entity"} or {1}))), ""]
        for part in p.parts:
            title = page.part_name(p, part).split(", ", 1)[1]
            lines += ["%s:" % (title[0].upper() + title[1:]), ""] + page.steps_list(part.steps, p.sub) + [""]
    tail = next(p for p in page.built["passes"] if p.name == "tail")
    lines += ["### After the phases", "", "The steps of the tick after its last phase:", ""]
    lines += page.steps_list(tail.parts[0].steps, "game") + [""]
    lines += ["## Entry points", "",
              "The steps each entry point reached by a tick runs, in order, nested entry points by name (their own steps are",
              "listed under them). Entry points that run no other subsystem's step are left out.", ""]
    for slot in sorted(page.reached, key=page.program.slot_label):
        steps, _ = page.program.entry(slot)
        sub = page.program.seam_owner[slot.split("::")[0]]
        if not distinct(steps):
            continue
        lines += ["### %s" % code(page.program.slot_label(slot)), ""] + page.steps_list(steps, sub) + [""]
    lines += ["## Cross-subsystem calls", "",
              "Every call from one subsystem to another's entry point that a tick reaches, with the classes of its call",
              "sites (one caller can use the value at one site and not at another). The tick itself calls the passes and",
              "is not listed.", ""]
    rows = []
    for (caller, label), classes in sorted(page.calls.items()):
        rows.append([caller, code(label), ", ".join(c for c in CLASSES if classes[c])])
    lines += table(["caller", "entry point", "class"], rows)
    lines += ["", "## Split points", "",
              "Each immediate edge between two subsystems, grouped by phase, writer, reader and place; `rows` is how many",
              "rows of coupling/dependencies.csv the group holds, and `across` how many of those an earlier entity of",
              "the same pass also wrote (Limits). The place names the part of the pass and, after `in`, the entry point",
              "the split lies in.", ""]
    groups = page.groups()
    for phase in PHASES:
        if phase not in groups:
            continue
        lines += ["### Phase %s" % phase, ""]
        rows = []
        for (writer, reader, kind, where, first, second, loose), n, across in groups[phase]:
            read = ("a later " if second == first else "") + code(second) if second else "the caller"
            rows.append([writer, reader, where, PLACEMENTS[kind], code(first) if first else "the caller", read, "by subsystem" if loose else "by item", str(n),
                         str(across)])
        lines += table(["writer", "reader", "place", "split", "write in", "read in", "matched", "rows", "across"], rows)
        lines += [""]
    lines += ["## By subsystem", "",
              "For each subsystem: the calls it makes to other subsystems' entry points in each class, and the calls it",
              "receives (`made / received`, counting each caller and entry point once per class), then the immediate edges",
              "it writes and reads, and how many of its edges fall inside one entity's update.", ""]
    lines += table(["subsystem"] + CLASSES + ["edges written", "edges read", "inside an entity"], page.summary_rows())
    lines += ["", "## Limits", "", LIMITS.rstrip("\n")]
    return "\n".join(lines) + "\n"


def load(build_dir: Path, jobs: int) -> Page:
    """The page from the syntax trees of the build directory and the data files."""
    functions = extract(build_dir, jobs)
    rows = lambda name: list(csv.DictReader((DATA / name).open(newline="")))
    files = {f["file"] for f in functions}
    sources = {path: (ROOT / path).read_text(errors="replace") for path in sorted(files)}
    program = Program(functions, rows("subsystems.csv"), rows("hazard_edges.csv"), SEAMS_HEADER.read_text(),
                      NATIVE.read_text(), sources)
    return Page(program, rows("dependencies.csv"), rows("pass_reads.csv"))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="regenerate the page")
    mode.add_argument("--check", action="store_true", help="fail if the page is stale or an input is not covered")
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR, help="a compile-only build with compile commands")
    parser.add_argument("--doc", type=Path, default=DOC)
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    args = parser.parse_args(argv)
    page = load(args.build_dir, args.jobs)
    problems = page.problems()
    for line in problems:
        print("schedule: " + line, file=sys.stderr)
    text = render(page)
    if args.write:
        args.doc.write_text(text)
        print("schedule: wrote %s" % args.doc)
        return 1 if problems else 0
    if problems:
        return 1
    current = args.doc.read_text() if args.doc.exists() else ""
    if current != text:
        print("schedule: %s is stale; run uv run tools/schedule.py --write" % args.doc, file=sys.stderr)
        return 1
    print("schedule: %s is current" % args.doc)
    return 0


if __name__ == "__main__":
    sys.exit(main())
