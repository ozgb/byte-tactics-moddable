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
PLAIN_POINTEES = {"int", "short", "char", "long", "float", "double", "bool"}
CONSTRUCTS = {"CXXConstructExpr", "CXXTemporaryObjectExpr"}
ASSIGNS = {"operator=", "operator+=", "operator-=", "operator*=", "operator/=", "operator|=", "operator&="}
FILLS = {"memset", "memcpy", "memmove", "strcpy", "strncpy", "sprintf"}
CASTS = {"ImplicitCastExpr", "ParenExpr", "CStyleCastExpr", "CXXStaticCastExpr", "CXXReinterpretCastExpr",
         "CXXConstCastExpr", "CXXFunctionalCastExpr"}
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
NATIVE_SELF = re.compile(r"^static [^;{(]*?\b(\w+)\s*\(\s*(?:const\s+)?\w+\s*\*\s*self\b", re.M)
NATIVE_SLOT = re.compile(r"^\s*s_(\w+)\.(\w+)\s*=\s*(?:\(.*\))?\s*(\w+);", re.M)

EFFECTS = {"RandomInt": "draw rng", "GameRand": "draw crt", "RecordSimEvent": "sim event"}
TRANSPARENT = {None, "seams", "platform", "game"}
ENGINE = {"presentation", "harness", "network", "files"}


def parameter_types(function_type: str) -> list[str]:
    """The parameter types of a function type, as written."""
    start = function_type.find("(")
    if start < 0:
        return []
    out, depth, current = [], 0, ""
    for ch in function_type[start + 1:]:
        if ch in "([<":
            depth += 1
        elif ch in ")]>":
            if depth == 0:
                break
            depth -= 1
        if ch == "," and depth == 0:
            out.append(current.strip())
            current = ""
        else:
            current += ch
    if current.strip():
        out.append(current.strip())
    return out


class OrderWalker(rwsets.Walker):
    """rwsets' walker that also keeps the calls in the order they run, with their line and whether the result is used."""

    def __init__(self, unit, name):
        super().__init__(unit, name)
        self.calls: list[list] = []
        self.stack: list[dict] = []
        self.pending: dict | None = None
        self.branches = 0
        self.local_field: str | None = None
        self.deferred: list[list] = []
        self.params: list[str] = []
        self.param_writes: set[int] = set()
        self.forwards: list[list] = []
        parts = name.split("::")
        self.constructor = len(parts) > 1 and parts[-1] == parts[-2]

    def visit(self, node: dict, mode: str = "R") -> None:
        if node.get("kind") in BRANCHING or (node.get("kind") == "BinaryOperator" and node.get("opcode") in ("&&", "||")):
            self.branches += 1
        if node.get("kind") in CONSTRUCTS:
            name = re.sub(r"^(?:const |struct |class )+", "", node.get("type", {}).get("qualType", ""))
            if re.fullmatch(r"\w+(?:::\w+)*", name):
                self.calls.append([node["_line"], "call", "%s::%s" % (name, name.split("::")[-1]), False, id(node)])
        if mode in ("W", "RW") and (node.get("kind") in rwsets.LVALUE_KINDS
                                    or (node.get("kind") == "UnaryOperator" and node.get("opcode") == "*")):
            root = self.param_root(node)
            if root is not None and root[1]:
                self.param_writes.add(root[0])
        assigns = node.get("kind") == "CompoundAssignOperator" or (node.get("kind") == "BinaryOperator"
                                                                    and node.get("opcode") == "=")
        if assigns:
            self.deferred.append([])
        self.stack.append(node)
        try:
            super().visit(node, mode)
        finally:
            self.stack.pop()
            if assigns:
                self.calls += self.deferred.pop()

    def record(self, kind: str, target: str) -> None:
        super().record(kind, target)
        if kind in ("call", "indirect_call") and self.pending is not None:
            node, self.pending = self.pending, None
            self.forward(node, kind, target)
            self.calls.append([node["_line"], kind, target, self.used(node) or self.out_arguments(node), id(node)])

    def access(self, category: str, target: str, mode: str) -> None:
        if category == "field" and target == self.local_field:
            return
        super().access(category, target, mode)
        if category in ("field", "global") and not (category == "global" and target.split("@")[0] == rwsets.SIM_POINTER):
            line = next((n["_line"] for n in reversed(self.stack) if "_line" in n), 0)
            for suffix in rwsets.MODE_KINDS[mode]:
                into = self.deferred[-1] if self.deferred and suffix in ("write", "maybe_written") else self.calls
                into.append([line, "%s_%s" % (category, suffix), target, False, None])

    def lvalue(self, node: dict, mode: str, path: bool = False) -> None:
        if node["kind"] != "MemberExpr" or path or rwsets.sim_global(node) or not self.local_object(node):
            super().lvalue(node, mode, path)
            return
        base = node.get("inner", [{}])[0]
        outer, self.local_field = self.local_field, "%s::%s" % (rwsets.record_name(base.get("type", {})),
                                                                node.get("name") or "<anon>")
        try:
            super().lvalue(node, mode, path)
        finally:
            self.local_field = outer

    def param_root(self, node: dict) -> tuple[int, bool] | None:
        """The parameter (from 0, -1 for the object of a method) an lvalue or pointer lies in, and whether reaching it
        goes through that pointer."""
        through = False
        while True:
            kind = node.get("kind")
            inner = node.get("inner", [])
            if kind in CASTS and inner:
                node = inner[0]
            elif kind == "MemberExpr" and inner:
                through = through or bool(node.get("isArrow"))
                node = inner[0]
            elif kind == "ArraySubscriptExpr" and inner:
                base = inner[0]
                through = through or not (base.get("kind") == "ImplicitCastExpr"
                                          and base.get("castKind") == "ArrayToPointerDecay")
                node = base
            elif kind == "UnaryOperator" and node.get("opcode") in ("*", "&") and inner:
                through = through or node.get("opcode") == "*"
                node = inner[0]
            elif kind == "CXXThisExpr":
                return -1, through
            elif kind == "DeclRefExpr":
                ref = node.get("referencedDecl", {})
                if ref.get("kind") == "ParmVarDecl" and ref.get("id") in self.params:
                    return self.params.index(ref["id"]), through
                return None
            else:
                return None

    def forward(self, node: dict, kind: str, target: str) -> None:
        """Note each parameter this call is handed a pointer or reference into (never a copy, nor a pointer to const),
        and where, so a write through it in the callee is seen."""
        inner = node.get("inner", [])
        if not inner:
            return
        callee = rwsets.strip(inner[0])
        if node.get("kind") == "CXXMemberCallExpr" and callee.get("kind") == "MemberExpr" and callee.get("inner"):
            root = self.param_root(callee["inner"][0])
            if root is not None:
                self.forwards.append([kind, target, -1, root[0]])
        params = parameter_types(rwsets.type_text(callee.get("type", {}))) if node.get("kind") == "CallExpr" else []
        for index, arg in enumerate(inner[1:]):
            root = self.param_root(arg)
            if root is None:
                continue
            arg_type = arg.get("type", {})
            core = arg
            while core.get("kind") in CASTS and core.get("inner"):
                core = core["inner"][0]
            if rwsets.is_pointer(arg_type):
                handed = not rwsets.pointee_const(arg_type)
            elif index < len(params) and params[index].endswith("&"):
                handed = not rwsets.pointee_const({"qualType": params[index]})
            else:
                handed = rwsets.is_pointer(core.get("type", {})) and not rwsets.pointee_const(core.get("type", {}))
            if handed:
                self.forwards.append([kind, target, index, root[0]])

    def local_object(self, node: dict) -> bool:
        """Whether a field access lands in a local object of the function, or in the object a constructor builds."""
        while True:
            kind = node.get("kind")
            inner = node.get("inner", [])
            if kind == "MemberExpr" and inner:
                if node.get("isArrow"):
                    root = rwsets.strip(inner[0])
                    return root.get("kind") == "CXXThisExpr" and self.constructor
                node = inner[0]
            elif kind in ("ParenExpr", "ImplicitCastExpr", "ArraySubscriptExpr") and inner:
                node = inner[0]
            elif kind == "DeclRefExpr":
                ref = node.get("referencedDecl", {})
                if ref.get("kind") not in ("VarDecl", "ParmVarDecl") or ref.get("id") not in self.locals:
                    return False
                return self.locals[ref["id"]] is None and not rwsets.type_text(ref.get("type", {})).strip().endswith("&")
            else:
                return False

    def out_arguments(self, node: dict) -> list[list]:
        """The arguments through which a call can hand back a value, as [index from 0, whether it is the address of a
        local of the caller]: the address of an object, an array, or a pointer to a non-const plain value, never a
        string literal."""
        out: list[list] = []
        if node.get("kind") not in ("CallExpr", "CXXMemberCallExpr"):
            return out
        for index, arg in enumerate(node.get("inner", [])[1:]):
            arg_type = arg.get("type", {})
            if not rwsets.is_pointer(arg_type) or rwsets.pointee_const(arg_type):
                continue
            inner = arg
            while inner.get("kind") in CASTS and inner.get("inner"):
                inner = inner["inner"][0]
            if inner.get("kind") == "StringLiteral":
                continue
            pointee = re.sub(r"^(?:struct |class |unsigned |signed )+", "", rwsets.type_text(arg_type)).rstrip(" *")
            address = inner.get("kind") == "UnaryOperator" and inner.get("opcode") == "&"
            array = arg.get("kind") == "ImplicitCastExpr" and arg.get("castKind") == "ArrayToPointerDecay"
            if address or array or pointee in PLAIN_POINTEES or pointee in VALUE_TYPES or pointee.startswith("std::"):
                local = (address or array) and self.local_variable(inner["inner"][0] if address else arg["inner"][0])
                out.append([index, local])
        return out

    def local_variable(self, node: dict) -> bool:
        """Whether an lvalue is a variable of the function (not static) or a part of one, reached with no pointer."""
        while node.get("kind") in ("ParenExpr", "ImplicitCastExpr", "ArraySubscriptExpr") or (
                node.get("kind") == "MemberExpr" and not node.get("isArrow")):
            if not node.get("inner"):
                return False
            if node["kind"] == "ArraySubscriptExpr" and node["inner"][0].get("castKind") != "ArrayToPointerDecay":
                return False
            node = node["inner"][0]
        ref = node.get("referencedDecl", {}) if node.get("kind") == "DeclRefExpr" else {}
        return ref.get("kind") in ("VarDecl", "ParmVarDecl") and ref.get("id") in self.locals \
            and self.locals[ref["id"]] is None

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
        walker.params = [c["id"] for c in node.get("inner", ()) if c.get("kind") == "ParmVarDecl"]
        for child in node.get("inner", ()):
            if child["kind"].endswith("Stmt") or child["kind"] == "CXXCtorInitializer":
                walker.visit(child, "R")
        returns = node.get("type", {}).get("qualType", "").split("(")[0].strip()
        self.functions.append({
            "function": name, "file": rel, "start": node["_line"], "end": node["_end"],
            "items": sorted(walker.items), "calls": [c[:4] for c in walker.calls], "branches": walker.branches,
            "returns": returns, "param_writes": sorted(walker.param_writes), "forwards": walker.forwards,
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
            marks.append(Mark("pass", number, ends[number], m.group(1).lower().replace("_", ".", 1)))
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
    access: str = ""


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
    param_writes: set = field(default_factory=set)
    forwards: list = field(default_factory=list)


class Program:
    """The functions, their subsystems, the seams and the marks: what the orders are walked over."""

    def __init__(self, functions: list[dict], subsystems: list[dict], edges: list[dict], seams_header: str,
                 native: str, sources: dict[str, str]):
        owner = {(r["function"], r["file"]): r["subsystem"] for r in subsystems}
        self.fns: dict[str, list[Fn]] = defaultdict(list)
        for f in functions:
            self.fns[f["function"]].append(Fn(f["function"], f["file"], f["start"], f["end"], f["calls"],
                                              {tuple(i) for i in f["items"]}, f["branches"], f["returns"],
                                              owner.get((f["function"], f["file"])), set(f.get("param_writes", ())),
                                              f.get("forwards", [])))
        self.seam_owner = {struct: name for struct, name in SEAM_FIELD.findall(seams_header) if name in SIMS}
        self.slots: dict[str, set[str]] = defaultdict(set)
        self.resolved: dict[str, list[str]] = defaultdict(list)
        tables = {name: struct for struct, name in NATIVE_TABLE.findall(native)}
        adapters = set(NATIVE_SELF.findall(native))
        self.methods: set[str] = set()
        for table, slot, impl in NATIVE_SLOT.findall(native):
            if table in tables:
                self.slots["%s::%s" % (tables[table], slot)].add(impl)
                if impl in adapters:
                    self.methods.add("%s::%s" % (tables[table], slot))
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
        self.through: dict[tuple, bool] = {}
        self.unknown: set[str] = set()

    def lookup(self, name: str, near: str = "") -> list[Fn]:
        """The definitions of a name, those in the caller's file first when there are several."""
        found = self.fns.get(name, [])
        same = [f for f in found if f.file == near]
        return same or found

    def writes_through(self, fn: Fn, index: int, stack: tuple = ()) -> bool | None:
        """Whether the function, or a function it hands the parameter to, writes through its parameter `index`: None
        when it hands the parameter to code that is not known (a library or an unresolved virtual call)."""
        key = (fn.name, fn.file, fn.start, index)
        if key in self.through:
            return self.through[key]
        if key in stack:
            return False
        found: bool | None = index in fn.param_writes
        for kind, target, to, mine in fn.forwards:
            if found:
                break
            if mine != index:
                continue
            if kind == "call" and (target in FILLS or target.startswith("operator")):
                found = True if to == 0 and (target in FILLS or target in ASSIGNS) else found
                continue
            how, names = self.targets(fn, kind, target)
            if how == "slot" and names[0].split("::")[0] not in self.seam_owner:
                continue
            impls = [g for name in (sorted(self.slots[names[0]]) if how == "slot" else names)
                     for g in self.lookup(name, fn.file) if g.sub not in ENGINE]
            if not impls:
                self.unknown.add(target)
                found = None
                continue
            for g in impls:
                inner = self.writes_through(g, to, stack + (key,))
                if inner or (inner is None and not found):
                    found = inner
        if not stack or found:
            self.through[key] = found
        return found

    def hands_back(self, used, impls: list[Fn], method: bool) -> bool:
        """Whether a call's value reaches the caller: the value returned, or an argument the callee writes through or
        hands to code that is not known (the address of a local of the caller, when no callee is known), never the
        record a method works on."""
        if used is True:
            return True
        for index, local in used or ():
            if method and index == 0:
                continue
            written = [self.writes_through(g, index) for g in impls]
            if any(w is not False for w in written) or (local and not impls):
                return True
        return False

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
        return self.expand_cut(fn, sub, stack, lines, own)[0]

    def expand_cut(self, fn: Fn, sub: str, stack: tuple = (), lines: tuple | None = None,
                   own: set | None = None) -> tuple[list[Node], set]:
        """The steps, and the functions on the stack whose calls were left out to break a cycle: a result cut by an
        ancestor is not kept, since the same function reached another way runs those calls."""
        key = (fn.name, fn.file, fn.start, sub)
        if lines is None and key in self.memo:
            if own is not None:
                own |= self.memo_own[key]
            return self.memo[key], set()
        mine: set = {(fn.name, fn.file, fn.start)}
        out: list[Node] = []
        cut: set = set()
        stack = stack + (key,)

        def inline(g: Fn) -> None:
            nonlocal out
            inner = (g.name, g.file, g.start, sub)
            if inner in stack:
                cut.add(inner)
                return
            steps, below = self.expand_cut(g, sub, stack, own=mine)
            out += steps
            cut.update(below)

        for line, kind, target, used in fn.calls:
            if lines is not None and not any(a <= line <= b for a, b in lines):
                continue
            if kind not in ("call", "indirect_call"):
                out.append(Node("access", target, sub, access=kind))
                continue
            how, names = self.targets(fn, kind, target)
            if how == "slot":
                slot = names[0]
                callee = self.seam_owner.get(slot.split("::")[0])
                if callee is None or callee == sub:
                    for impl in sorted(self.slots[slot]):
                        for g in self.lookup(impl):
                            inline(g)
                else:
                    impls = [g for impl in sorted(self.slots[slot]) for g in self.lookup(impl)]
                    out.append(Node("entry", self.slot_label(slot), callee,
                                    self.hands_back(used, impls, slot in self.methods), fn.name, slot))
                continue
            for name in names:
                if name in EFFECTS:
                    out.append(Node("effect", EFFECTS[name], sub))
                    continue
                found = self.lookup(name, fn.file)
                inside = [g for g in found if g.sub in TRANSPARENT or g.sub == sub]
                for g in inside or found:
                    if g.sub in ENGINE:
                        out.append(Node("engine", g.name, g.sub, used is True, fn.name))
                    elif g.sub in SIMS and g.sub != sub:
                        out.append(Node("direct", g.name, g.sub, self.hands_back(used, [g], False), fn.name))
                    else:
                        inline(g)
        cut.discard(key)
        if lines is None and not cut:
            self.memo[key] = out
            self.memo_own[key] = mine
        if own is not None:
            own |= mine
        return out, cut

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
    """A run of one pass: the work before its loops, one loop's entity, between two loops, or the work after."""
    kind: str
    index: int
    lines: list
    steps: list
    own: set


@dataclass
class Pass:
    name: str
    phase: str
    holder: str
    sub: str
    parts: list
    parent: str = ""
    depth: int = 0


def segments(begin: int, end: int, inner: list[Mark]) -> tuple[list, list]:
    """The line ranges of begin..end outside the inner passes, and the inner passes in it, each at its place: a list
    of ('lines', (a, b)) and ('pass', mark)."""
    out: list = []
    cursor = begin
    for m in sorted(inner, key=lambda m: m.line):
        if m.line < begin or m.end > end:
            continue
        if m.line > cursor:
            out.append(("lines", (cursor, m.line - 1)))
        out.append(("pass", m))
        cursor = m.end + 1
    if cursor <= end:
        out.append(("lines", (cursor, end)))
    return out, [m for kind, m in out if kind == "pass"]


def part_steps(program: Program, fn: Fn, sub: str, pieces: list, own: set) -> list[Node]:
    steps: list[Node] = []
    for kind, value in pieces:
        if kind == "pass":
            steps.append(Node("pass", value.name, sub))
        else:
            steps += program.expand(fn, sub, lines=[value], own=own)
    return steps


def pass_tree(program: Program, fn: Fn, sub: str, phase: str, holder: str) -> list[Pass]:
    """The passes a pass function marks, outermost first, each with its parts; an inner pass is a step of the part
    of its parent that holds it."""
    marks = program.marks_in(fn)
    scopes = [m for m in marks if m.kind == "pass"]
    entities = [m for m in marks if m.kind == "entity"]

    def inside(m: Mark, outer: Mark) -> bool:
        return m is not outer and outer.line <= m.line and m.end <= outer.end

    def parent(m: Mark) -> Mark | None:
        holders = [o for o in scopes if inside(m, o)]
        return max(holders, key=lambda o: o.line) if holders else None

    out: list[Pass] = []

    def visit(scope: Mark, depth: int) -> None:
        children = [m for m in scopes if parent(m) is scope]
        own_entities = [e for e in entities if scope.line <= e.line <= scope.end
                        and not any(c.line <= e.line <= c.end for c in children)]
        pieces: list = []
        cursor = scope.line
        for i, e in enumerate(own_entities, 1):
            if e.line > cursor:
                pieces.append(("between" if pieces else "before", i - 1, (cursor, e.line - 1)))
            pieces.append(("entity", i, (e.line, e.end)))
            cursor = e.end + 1
        if cursor <= scope.end:
            pieces.append(("after" if own_entities else "steps", len(own_entities), (cursor, scope.end)))
        parts = []
        for kind, index, (a, b) in pieces:
            split, _ = segments(a, b, children)
            own: set = set()
            parts.append(Part(kind, index, [v for k, v in split if k == "lines"], part_steps(program, fn, sub, split, own),
                              own))
        up = parent(scope)
        out.append(Pass(scope.name, phase, holder, sub, parts, up.name if up else "", depth))
        for child in sorted(children, key=lambda m: m.line):
            visit(child, depth + 1)

    for top in [m for m in scopes if parent(m) is None]:
        visit(top, 0)
    return out


def find_pass(program: Program, own: set) -> Fn | None:
    for key in sorted(own):
        f = program.fn_of(key)
        if f is not None and any(m.kind == "pass" for m in program.marks_in(f)):
            return f
    return None


def build(program: Program) -> dict:
    """The phases of a tick in order, the steps of each and the passes they hold."""
    tick, ranges = program.tick()
    phases = []
    passes = []
    for phase in PHASES:
        own: set = set()
        steps = program.expand(tick, "game", lines=[ranges[phase]], own=own)
        phases.append((phase, steps))
        if phase == "tail":
            passes.append(Pass("tail", phase, "", "game", [Part("steps", 0, [ranges[phase]], steps, own)]))
            continue
        for step in steps:
            if step.kind != "entry":
                continue
            fn = find_pass(program, program.entry(step.slot)[1])
            if fn is not None:
                passes += pass_tree(program, fn, step.sub, phase, step.label)
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
            elif n.kind == "direct":
                writes = writes or any(state_write(i) for f in self.program.lookup(n.label) if f.sub == n.sub
                                       for i in f.items)
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
        calls = [n for n in steps if n.kind in ("entry", "effect", "direct", "engine")]
        return not calls and len(body) <= 1 and all(f.branches == 0 for f in body)

    def silent(self, slot: str) -> bool:
        """An entry point in which nothing is seen to change: no write of any kind (through pointers and to plain values
        included), no draw and no sim event, nested entry points included."""
        _, slots = self.reach(slot)
        for s in slots:
            steps, own = self.program.entry(s)
            if any(n.kind == "effect" for n in steps):
                return False
            if any(k.endswith(("_write", "_maybe_written")) or (k, t) in ALLOCATIONS
                   for k, t in self.program.items_of(own)):
                return False
        return True

    def classify(self, node: Node) -> str:
        writes, _ = self.reach(node.slot)
        if not node.used:
            return "intent"
        if writes:
            return "mixed"
        return "view read" if self.view(node.slot) else "query"


@dataclass
class Placement:
    """Where an immediate edge falls: the kind, the part (and entry point) it is in, the writer's and reader's steps
    with their positions in that list, the writer's step when it also reaches a read (the split may lie in it), and
    for a write by the caller the last step listed before it."""
    kind: str
    where: str
    first: str = ""
    second: str = ""
    by_subsystem: bool = False
    first_at: int = 0
    second_at: int = 0
    inside: str = ""
    first_after: int = 0


class Splits:
    """Places each immediate edge in the orders: the first write before a later read, inside one part of a pass."""

    def __init__(self, program: Program):
        self.program = program
        self.memo: dict[tuple, bool] = {}
        self.closures: dict[str, list[str]] = {}
        self.lists: dict[tuple, tuple] = {}
        targets: set = set()
        for fns in program.fns.values():
            for f in fns:
                targets |= {t for _, t in f.items}
        self.targets = targets

    def matcher(self, item: str, loose: bool = False):
        """A test on access targets for the item, or None when no function can name it (or `loose`)."""
        if loose or ": " in item or item.startswith(("rng.", "unresolved:")):
            return None
        if "::" in item:
            return (lambda t: t == item) if item in self.targets else None
        names = {t for t in self.targets if t.split("@")[0] == item}
        return (lambda t: t.split("@")[0] == item) if names else None

    def hits(self, sub: str, own: set, who: str, match, write: bool) -> bool:
        if sub != who:
            return False
        kinds = WRITE_KINDS if write else READ_KINDS
        rows = [(k, t) for k, t in self.program.items_of(own) if k in kinds]
        if write:
            rows = [r for r in rows if state_write(r)]
        return any(match is None or match(t) for _, t in rows)

    def closure(self, slot: str) -> list[str]:
        """The entry point and every entry point it reaches, nested ones included."""
        if slot not in self.closures:
            seen = [slot]
            todo = [slot]
            while todo:
                for n in self.program.entry(todo.pop())[0]:
                    if n.kind == "entry" and n.slot not in seen:
                        seen.append(n.slot)
                        todo.append(n.slot)
            self.closures[slot] = seen
        return self.closures[slot]

    def reach(self, slot: str, who: str, item: str, match, write: bool) -> bool:
        """Whether the entry point, or one it reaches, is the writer's write (or the reader's read) of the item."""
        key = (slot, who, item if match is not None else "", write)
        if key not in self.memo:
            found = False
            for s in self.closure(slot):
                own_key = ("own", s, who, item if match is not None else "", write)
                if own_key not in self.memo:
                    sub = self.program.seam_owner[s.split("::")[0]]
                    self.memo[own_key] = self.hits(sub, self.program.entry(s)[1], who, match, write)
                if self.memo[own_key]:
                    found = True
                    break
            self.memo[key] = found
        return self.memo[key]

    def locate(self, where: str, steps: list, sub: str, writer: str, reader: str, item: str,
               stack: tuple = (), loose: bool = False) -> Placement | None:
        """The first write of the writer, in these steps or nested in one of them, followed by a read of the reader."""
        match = self.matcher(item, loose)
        loose = match is None
        writes = []
        reads = []
        for i, n in self.entries_of(steps):
            if self.reach(n.slot, writer, item, match, True):
                writes.append(i)
            if self.reach(n.slot, reader, item, match, False):
                reads.append(i)
        table = self.accesses_of(steps)
        for who, write, into in ((writer, True, writes), (reader, False, reads)):
            if sub == who:
                for target, where_at in table["write" if write else "read"].items():
                    if match is None or match(target):
                        into += where_at
        writes.sort()
        reads.sort()
        for w in writes:
            later = [r for r in reads if r > w]
            if not later:
                continue
            r = later[0]
            first, second = steps[w], steps[r]
            kind = {("entry", "entry"): "split", ("entry", "access"): "apply", ("access", "entry"): "before"}.get(
                (first.kind, second.kind))
            if kind is None:
                continue
            inside = first.label if first.kind == "entry" and w in reads and self.splits_inside(
                first, writer, reader, item, stack, loose) else ""
            return Placement(kind, where, first.label if first.kind == "entry" else "",
                             second.label if second.kind == "entry" else "", loose,
                             self.position(steps, w), self.position(steps, r), inside,
                             self.listed(steps, w) if first.kind == "access" else 0)
        for i in writes:
            n = steps[i]
            if n.kind == "entry" and i in reads and n.slot not in stack:
                inner, _ = self.program.entry(n.slot)
                found = self.locate("%s, in %s" % (where, code(n.label)), inner, n.sub, writer, reader, item,
                                    stack + (n.slot,), loose)
                if found:
                    return found
        return None

    def splits_inside(self, step: Node, writer: str, reader: str, item: str, stack: tuple, loose: bool) -> bool:
        """Whether a write of the writer followed by a read of the reader is found inside the step."""
        key = ("inside", step.slot, writer, reader, item, loose, stack)
        if key not in self.memo:
            self.memo[key] = False
            if step.slot not in stack:
                inner, _ = self.program.entry(step.slot)
                self.memo[key] = self.locate("", inner, step.sub, writer, reader, item, stack + (step.slot,),
                                             loose) is not None
        return self.memo[key]

    def entries_of(self, steps: list) -> list:
        key = ("entries", id(steps))
        if key not in self.lists:
            self.lists[key] = (steps, [(i, n) for i, n in enumerate(steps) if n.kind == "entry"])
        return self.lists[key][1]

    def accesses_of(self, steps: list) -> dict:
        """The positions of the state writes and the reads among the steps, by the item they touch."""
        key = ("accesses", id(steps))
        if key not in self.lists:
            table: dict = {"write": defaultdict(list), "read": defaultdict(list)}
            for i, n in enumerate(steps):
                if n.kind != "access":
                    continue
                if n.access in WRITE_KINDS and state_write((n.access, n.label)):
                    table["write"][n.label].append(i)
                elif n.access in READ_KINDS:
                    table["read"][n.label].append(i)
            self.lists[key] = (steps, table)
        return self.lists[key][1]

    def across(self, where: str, steps: list, writer: str, reader: str, item: str) -> Placement | None:
        """An entity whose steps write the item only after every read of it: the read is in a later entity."""
        match = self.matcher(item)
        for loose in ((match is None,) if match is None else (False, True)):
            m = None if loose else match
            entries = [(i, n) for i, n in enumerate(steps) if n.kind == "entry"]
            writes = [i for i, n in entries if self.reach(n.slot, writer, item, m, True)]
            reads = [i for i, n in entries if self.reach(n.slot, reader, item, m, False)]
            if writes and reads and reads[-1] <= writes[0]:
                return Placement("across", where, steps[writes[0]].label, steps[reads[0]].label, loose,
                                 self.position(steps, writes[0]), self.position(steps, reads[0]))
        return None

    @staticmethod
    def listed(steps: list, index: int) -> int:
        """How many steps the part's list shows up to this point."""
        return len({(n.kind, n.label) for n in steps[:index + 1] if n.kind in ("entry", "effect", "direct")})

    @staticmethod
    def position(steps: list, index: int) -> int:
        """The number of the step in the part's list as the page shows it: entry points, draws and sim events, each
        listed where it first occurs (1 up)."""
        seen: list = []
        for n in steps[:index + 1]:
            if n.kind in ("entry", "effect", "direct") and (n.kind, n.label) not in seen:
                seen.append((n.kind, n.label))
        target = steps[index]
        if target.kind != "entry":
            return 0
        return seen.index((target.kind, target.label)) + 1


CLASSES = ["view read", "query", "intent", "mixed"]
PLACEMENTS = {"split": "split", "apply": "apply before continuing", "before": "write before calling",
              "across": "across entities"}
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
    ("units.players", 1): "each player, in slot order from 0 to 9; a player is skipped unless it is active, its type is "
                          "1, 2 or 3 and its index is not 10.",
    ("units", 1): "each unit slot of the player, from the first to the last in index order; a slot is visited when it "
                  "holds a unit (its type index is not 0).",
    ("projectiles", 1): "each projectile slot, in index order, from 0 to the count of projectiles as it was when the pass "
                        "began. Projectiles that the pass adds are not visited in the same pass.",
    ("explosions.pieces", 1): "each of the 100 slots of the exploded pieces, in index order; an empty slot does nothing.",
    ("explosions.debris", 1): "each debris record, in index order, from 0 up to the count of debris records.",
    ("players", 1): "each player slot, in slot order from 0 to 9; a player is skipped unless it is active, its type is 1, "
                    "2 or 3 and its index is not 10.",
    ("players.los", 1): "each unit slot of the player, from the first to the last in index order, whether or not it "
                        "holds a unit.",
    ("features.anim", 1): "each feature type, in index order, from 0 to the count of feature types.",
    ("features.anim", 2): "the map scan, once per pass: the scan index steps down one cell (from the first cell it "
                          "wraps to the last), and the feature on the scanned cell may seed another nearby.",
    ("features", 1): "each record of the list of burning and animated features, from the head of the used list along "
                     "its links.",
    ("wind", 1): "the wind once per tick: the pass has one entity.",
    ("meteors", 1): "the meteor shower once per tick: the pass has one entity.",
}
SILENT_INTENTS = {
    "pathfinding.CellsChanged": "a notification: what it does is left to the implementation, and the reference's does "
                                "nothing.",
    "units.FindScript": "a lookup whose value the caller discards: it has no effect.",
    "orders.DestroyMotionObject": "it writes through a virtual call whose targets are not resolved.",
    "players.NotifyUnitCreated": "it writes through a virtual call whose targets are not resolved.",
    "players.NotifyUnitDied": "it writes through a virtual call whose targets are not resolved.",
    "weapons.WeaponAimCobComplete": "it writes through a virtual call whose targets are not resolved.",
}
LIMITS = """\
The orders are read from the program, not from a run. That decides what they claim:

- **Program order.** A step is listed where its call stands. The conditions around a call are left out: a step runs
  only when its conditions hold, and a call in a loop, or reached on several paths, can run more than once. A list
  names each step once, where it first occurs; the split points give a step by its number in that list, and say
  `again, after step n` (or `again, after the write`) when the call that matters is a later call of a step listed
  earlier. A write by the caller is placed by the steps listed before it.
- **Nested calls** are followed through everything the subsystem runs itself, through the virtual calls and function
  pointers whose targets are known, and through the engine's shared helpers. A call is followed into every function
  of its name, overloads included, so a list can hold steps (and a class count writes) that one call does not run.
  Calls into presentation, network and file code are left out of the lists, and so is the test harness.
- **Values.** The caller uses a call's value when it uses the value returned, or when it hands the call the address
  of an object, an array or a pointer to a plain value (never a string) that the call fills in: what it runs writes
  through that argument, or hands it to code outside the program (a library container, say). The address of a local
  of the caller counts also when no code for the entry point is known. An argument the call only reads is an input,
  and the record an entry point works on, given as its first argument (an account of the economy, say), is not a
  value handed back. That record is recognised by the name the native adapter of the slot gives its first
  parameter, so a slot whose first argument is a local object the callee fills (as some AI commands' sets of unit
  types are) is also not counted. An argument is followed into another function only as a pointer or reference to
  write through: a copy, or a pointer to const, is read. Writes made by a callback the caller passes (a visitor object, say) are not seen when the
  callback is reached through a cast to its base type, so such a call is classed by its other values only.
- **Writes**, for the classes, are the writes to a field of a record or to a variable of the game that the program
  shows, anywhere in what a call runs (nested entry points included, and for a call outside an interface its own
  writes, not those of what it calls in turn), and allocations, random draws and sim events. Writes to a local object
  of the function, or to the object a constructor builds, are not counted, and neither are writes to the fields of
  plain values (positions, points, fixed-point numbers) through a pointer, nor other writes through a bare pointer. A
  call whose value is used and that writes state only in those ways is classed as a view read or a query; an intent
  in which no write of any kind is found is listed under Intents with no write found.
- **Split points** place each `immediate` row of coupling/dependencies.csv (the access log, run on the scenario
  suite) at the first step of the writer that reaches a write of its item and is followed by a step of the reader that
  reaches a read of it, in one list (the code of the list's own subsystem counts as `the caller`). Where the program
  names the item, the two steps must touch it. Where it does not (a region of the game record, an allocation, or an
  item reached under another name), or no such pair is found, the row is matched by subsystem: the writer's first step
  that writes anything, and the reader's next step that reads anything. A row matched by subsystem says where a
  split can be, not where it must be.
- **Inside a step.** When the writer's step also holds a write of the item followed by a read of it by the reader,
  the split may be needed inside that step instead: the row names the step under `may split inside`. The search goes
  inside a step for the place itself only when its list holds no pair.
- **Across entities.** A row placed `across entities` has every step of the reader before the writer's first step
  in one entity's list, or in that step with no write of the writer followed by a read: the read is in a later
  entity, whose order the scheduler keeps, and no split inside the entity is needed. The column `across` counts the
  rows of a group whose item an earlier entity of the same pass wrote and a later one read (coupling/pass_reads.csv).
"""


def distinct(nodes: list, kinds=("entry", "effect", "direct", "pass")) -> list:
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
        parts.sort(key=lambda x: (x[1].kind != "entity", -x[0].depth))
        for loose in (False, True):
            for p, part in parts:
                found = self.splits.locate(self.part_name(p, part), part.steps, p.sub, d["writer"], d["reader"],
                                           d["item"], loose=loose)
                if found:
                    return found
        for p, part in parts:
            if part.kind == "entity":
                found = self.splits.across(self.part_name(p, part), part.steps, d["writer"], d["reader"], d["item"])
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
            elif n.kind == "pass":
                out.append("1. the pass %s (below)" % code(n.label))
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
            key = (d["writer"], d["reader"], found.kind, found.where, found.first, found.first_at, found.second,
                   found.second_at, found.inside, found.by_subsystem, found.first_after)
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
            if ", each entity" in found.where and found.kind != "across":
                inside[d["writer"]] += 1
                if d["reader"] != d["writer"]:
                    inside[d["reader"]] += 1
        rows = []
        for sub in SIMS:
            rows.append([sub] + ["%d / %d" % (made[sub][c], got[sub][c]) for c in CLASSES]
                        + [str(writes[sub]), str(reads[sub]), str(inside[sub])])
        return rows

    def silent_intents(self) -> list[str]:
        """The entry points called as intents in which no write of any kind, draw or sim event is found."""
        labels = {self.program.slot_label(s): s for s in self.reached}
        return sorted({label for (_, label), classes in self.calls.items()
                       if classes["intent"] and self.effects.silent(labels[label])})

    def unplaced(self) -> list[dict]:
        return [d for d, found in self.placed if found is None]

    def problems(self) -> list[str]:
        """What --check refuses besides a stale page: a step or a pass without its description, or one left over."""
        out = []
        names = {(p.name, part.index) for p in self.built["passes"] for part in p.parts if part.kind == "entity"}
        for key in sorted(names - set(LOOPS)):
            out.append("pass %s, loop %d has no description in LOOPS" % key)
        for key in sorted(set(LOOPS) - names):
            out.append("LOOPS describes pass %s, loop %d, which the marks do not hold" % key)
        engine = {n.label for _, steps in self.built["phases"] for n in steps if n.kind == "engine" and n.sub != "harness"}
        for name in sorted(engine - set(TICK_STEPS) - TICK_HIDDEN):
            out.append("the tick calls %s, which TICK_STEPS does not describe" % name)
        for name in sorted(set(TICK_STEPS) - engine):
            out.append("TICK_STEPS describes %s, which the tick does not call" % name)
        silent = self.silent_intents()
        for label in silent:
            if label not in SILENT_INTENTS:
                out.append("%s is called as an intent but no write is found in it: it may hand back a value the caller "
                           "uses; class it, or give the reason in SILENT_INTENTS" % label)
        for label in sorted(set(SILENT_INTENTS) - set(silent)):
            out.append("SILENT_INTENTS lists %s, which is no intent without a write" % label)
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
        "    applied before that call;",
        "  - **across entities:** the reader's steps all come before the writer's in one entity: the read is in a later",
        "    entity, and the entity order covers it.",
        "- **A call into a subsystem outside its interface:** a call into code another subsystem owns (a constructor or a",
        "  helper of its records) that goes through no seam. It has no class; its writes count for the entry point that",
        "  makes it.",
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
        loops = sorted({q.index for q in p.parts if q.kind == "entity"})
        said = " ".join(("Loop %d visits %s" % (i, LOOPS[(p.name, i)]) if len(loops) > 1
                         else "It visits %s" % LOOPS[(p.name, i)]) for i in loops)
        where = ("Nested in the pass %s, where its lists name it." % code(p.parent) if p.parent
                 else "Phase %s, through %s." % (p.phase, code(p.holder)))
        lines += ["### %s" % p.name, "", (where + " " + (said or "It has no loop of its own: it runs the passes it "
                                                          "names, in order.")).strip(), ""]
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
    lines += ["## Intents with no write found", "",
              "Entry points called as intents in which nothing is seen to change (Limits), with the reason.", ""]
    lines += table(["entry point", "why"], [[code(label), SILENT_INTENTS[label]] for label in page.silent_intents()
                                           if label in SILENT_INTENTS]) + [""]
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
        for (writer, reader, kind, where, first, first_at, second, second_at, inside, loose, after), n, across in \
                groups[phase]:
            if first:
                write = "%s (step %d)" % (code(first), first_at)
            else:
                write = "the caller, after step %d" % after if after else "the caller, before the first step"
            anchor = first_at or after
            if not second:
                read = "the caller"
            elif second_at > anchor or kind == "across":
                read = "%s (step %d)" % (code(second), second_at)
            elif first:
                read = "%s again, after step %d" % (code(second), first_at)
            else:
                read = "%s again, after the write" % code(second)
            rows.append([writer, reader, where, PLACEMENTS[kind], write, read, code(inside) if inside else "",
                         "by subsystem" if loose else "by item", str(n), str(across)])
        lines += table(["writer", "reader", "place", "split", "write in", "read in", "may split inside", "matched", "rows",
                        "across"], rows)
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
