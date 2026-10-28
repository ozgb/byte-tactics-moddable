"""Fail when a simulation subsystem reaches the game around its seams, the state block and the engine services.

    uv run tools/seamlink.py --objects @build/modern/objects.rsp            # print every violation
    uv run tools/seamlink.py --objects @build/modern/objects.rsp --summary  # also count per subsystem
    uv run tools/seamlink.py --objects @build/modern/objects.rsp --write    # write data/seam_exceptions.csv
    uv run tools/seamlink.py --objects @build/modern/objects.rsp --check
    uv run tools/test_seamlink.py                                           # tests, on hand-written units

The build puts every function in its own COMDAT section, so the relocations of a
section name the symbols one function needs. For each function of a simulation
subsystem S (data/subsystems.csv, by source file and name), each symbol its code
references is allowed only when it is:

  seam          g_seams, or anything interfaces/seams.h declares outside the table struct
  state         g_game, a symbol defined in src/state/, or declared in interfaces/ref.h and not defined elsewhere
  service       declared at depth 0 in interfaces/services.h, or a member of a class it defines
  host          declared in interfaces/host.h
  bitexact      a bx_ name, or declared in lib/bitexact/bitexact.h
  library       a symbol no object of the build defines (the CRT, the MSVC 5 libraries, the imports), or
                one src/util/operator_new.cpp defines (operator new and delete)
  const         a data symbol whose data/state_classes.csv class is const or sim
  own           defined by S: a function of S in data/subsystems.csv, or data defined in a file
                whose functions all belong to S, or a compiler-made symbol (string literal,
                vtable, constant) defined by an object of a file with a function of S

Anything else is a violation, printed as `subsystem function -> symbol (kind, owner)`:
kind is code or data, owner is the subsystem that defines a code symbol or the file that
defines the rest. data/seam_exceptions.csv (subsystem,function,symbol,reason) lists the
violations that remain, each with a `TODO(#n): ...` reason. --check fails on a violation
without a row and on a row that no longer occurs, so the list only shrinks. Standard
library plus tools/objread.py.
"""

import argparse
import csv
import io
import os
import re
import sys
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from objread import parse
from sources import ROOT, base_name

SUBSYSTEMS = "data/subsystems.csv"
EXCEPTIONS = "data/seam_exceptions.csv"
COLUMNS = ["subsystem", "function", "symbol", "reason"]
SIMULATION = ("wind_meteors", "economy", "features", "ai", "pathfinding", "weapons", "targeting", "los_radar",
              "orders", "units", "players", "map")
STATE_DIR = "src/state/"
LIBRARY_FILE = "src/util/operator_new.cpp"
GAME_POINTER = "g_game"
STATE_CLASSES = "data/state_classes.csv"
CONST_CLASSES = ("const", "sim")
KINDS = ("seam", "state", "service", "host", "bitexact", "library", "own", "const")
STATE_HEADERS = ("interfaces/ref.h",)
SEAMS_HEADER = "interfaces/seams.h"
SERVICES_HEADER = "interfaces/services.h"
HOST_HEADER = "interfaces/host.h"
BITEXACT_HEADER = "lib/bitexact/bitexact.h"
SEAM_TABLE = "g_seams"
REASON = re.compile(r"TODO\(#[0-9]+\): \S")
REL_DIR32, REL_REL32 = 6, 20


@dataclass
class Unit:
    """One object of the build: the source file, the symbols it defines and the symbols each function needs."""
    file: str
    code: set = field(default_factory=set)
    data: set = field(default_factory=set)
    refs: list = field(default_factory=list)


@dataclass(frozen=True)
class Violation:
    subsystem: str
    function: str
    symbol: str
    kind: str
    owner: str

    def line(self) -> str:
        return f"{self.subsystem} {self.function} -> {self.symbol} ({self.kind}, {self.owner})"


def source_of(path: Path) -> str:
    """The source file an object of the build was compiled from."""
    parts = path.parts
    for i in range(len(parts) - 1, -1, -1):
        if parts[i] in ("src", "link", "lib"):
            return "/".join(parts[i:]).removesuffix(".obj").removesuffix(".o")
    return path.name


def unit_of(path: Path) -> Unit:
    """The definitions of an object, and each function's (name, referenced symbol) pairs."""
    obj = parse(path)
    unit = Unit(source_of(path))
    for sym in obj.externals.values():
        (unit.code if obj.secs[sym.section - 1].is_code else unit.data).add(sym.name)
    unit.data.update(obj.commons)
    for sec in obj.secs:
        if not sec.is_code or not sec.relocs:
            continue
        names = sorted((s.value, s.name) for s in obj.syms.values()
                       if s.section == sec.index and s.sclass in (2, 3) and not s.name.startswith("."))
        if not names:
            continue
        starts = [v for v, _ in names]
        for offset, index, kind in sec.relocs:
            target = obj.syms.get(index)
            if kind not in (REL_DIR32, REL_REL32) or target is None:
                continue
            if target.section > 0 and (target.sclass == 3 or target.name.startswith(".")):
                continue
            at = max(sum(1 for v in starts if v <= offset) - 1, 0)
            unit.refs.append((names[at][1], target.name))
    return unit


def normal(sym: str) -> str:
    """A symbol name as C declares it: the import prefix, underscore and stdcall suffix are removed."""
    if sym.startswith("__imp_"):
        sym = sym[len("__imp_"):]
    return base_name(sym)


def clean(text: str) -> str:
    """The text without comments, preprocessor lines and extern "C" wrappers."""
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    text = re.sub(r"^\s*#[^\n]*(\\\n[^\n]*)*", " ", text, flags=re.M)
    return re.sub(r'extern\s*"C"\s*\{', " ", text)


def members(text: str) -> set:
    """Class::name for each name declared directly inside the body of a class or struct: member functions, constructors and data."""
    text = clean(text)
    out = set()
    for m in re.finditer(r"\b(?:class|struct)\s+(\w+)\s*(?::[^{;]*)?\{", text):
        depth, i, start = 1, m.end(), m.end()
        flat = []
        while i < len(text) and depth:
            ch = text[i]
            if ch == "{":
                depth += 1
                flat.append(" ")
            elif ch == "}":
                depth -= 1
                flat.append(";" if depth == 1 else " ")
            elif depth == 1:
                flat.append(ch)
            i += 1
        for stmt in "".join(flat).split(";"):
            stmt = re.sub(r"\b(public|private|protected)\s*:", " ", stmt).strip()
            found = re.search(r"(~?[A-Za-z_]\w*)\s*\(", stmt)
            if found and not stmt.startswith(("typedef", "friend")):
                out.add(f"{m.group(1)}::{found.group(1)}")
    return out


def declared(text: str) -> set:
    """The names a header declares outside any braces: functions and extern variables."""
    text = clean(text)
    flat, depth = [], 0
    for ch in text:
        if ch == "{":
            depth += 1
            flat.append(" ")
        elif ch == "}":
            depth = max(depth - 1, 0)
            flat.append(";" if depth == 0 else " ")
        elif depth == 0:
            flat.append(ch)
    out = set()
    for stmt in "".join(flat).split(";"):
        stmt = stmt.strip()
        if not stmt or re.match(r"(typedef|class|struct|enum|union|using|template|static_assert)\b", stmt):
            continue
        m = re.search(r"([A-Za-z_]\w*)\s*\(", stmt)
        if m:
            out.add(m.group(1))
        elif stmt.startswith("extern"):
            m = re.search(r"([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*$", stmt)
            if m:
                out.add(m.group(1))
    return out


def header_names(root: Path, relative: str) -> set:
    path = root / relative
    return declared(path.read_text(encoding="latin-1")) if path.exists() else set()


@dataclass
class Rules:
    """What the build allows a subsystem, gathered from the tree and the units."""
    function_subsystem: dict
    file_subsystems: dict
    services: set
    host: set
    seams: set
    state_names: set
    bitexact_names: set
    const_names: set
    code_defs: dict
    data_defs: dict

    def sole_owner(self, file: str):
        """The subsystem that owns every function of the file, or None for a shared file."""
        subs = self.file_subsystems.get(file, set())
        return next(iter(subs)) if len(subs) == 1 else None

    def own_file(self, subsystem: str, file: str) -> bool:
        return self.sole_owner(file) == subsystem

    def code_owner(self, symbol: str) -> str:
        """The subsystem of the function that defines a code symbol, else the first defining file."""
        name = base_name(symbol)
        files = sorted(self.code_defs.get(symbol, []))
        subs = {self.function_subsystem[(f, name)] for f in files if (f, name) in self.function_subsystem}
        if len(subs) == 1:
            return next(iter(subs))
        return files[0] if files else "?"

    def data_owner(self, symbol: str) -> str:
        files = sorted(self.data_defs.get(symbol, []))
        if not files:
            return "?"
        sole = self.sole_owner(files[0])
        return sole if sole else files[0]


def read_subsystems(root: Path) -> tuple:
    path = root / SUBSYSTEMS
    by_function, by_file = {}, {}
    if path.exists():
        with path.open(newline="") as f:
            for row in csv.DictReader(f):
                by_function[(row["file"], row["function"])] = row["subsystem"]
                by_file.setdefault(row["file"], set()).add(row["subsystem"])
    return by_function, by_file


def const_names(root: Path) -> set:
    """The names data/state_classes.csv gives class const or sim."""
    path = root / STATE_CLASSES
    if not path.exists():
        return set()
    with path.open(newline="") as f:
        return {r["name"] for r in csv.DictReader(f) if r["class"] in CONST_CLASSES}


def build_rules(root: Path, units: list) -> Rules:
    by_function, by_file = read_subsystems(root)
    code_defs, data_defs = {}, {}
    for unit in units:
        for name in unit.code:
            code_defs.setdefault(name, set()).add(unit.file)
        for name in unit.data:
            data_defs.setdefault(name, set()).add(unit.file)
    services_path = root / SERVICES_HEADER
    services = header_names(root, SERVICES_HEADER)
    if services_path.exists():
        services |= members(services_path.read_text(encoding="latin-1"))
    state = set()
    for header in STATE_HEADERS:
        state |= header_names(root, header)
    return Rules(by_function, by_file, services, header_names(root, HOST_HEADER), header_names(root, SEAMS_HEADER) | {SEAM_TABLE}, state,
                 header_names(root, BITEXACT_HEADER), const_names(root), code_defs, data_defs)


def classify(rules: Rules, subsystem: str, symbol: str):
    """The kind of rule that lets a function of the subsystem reference the symbol, or None for a violation."""
    name = normal(symbol)
    last = name.split("::")[-1]
    if last == GAME_POINTER:
        return "state"
    if name in rules.seams or last in rules.seams:
        return "seam"
    files = rules.code_defs.get(symbol, set()) | rules.data_defs.get(symbol, set())
    in_state = all(f.startswith(STATE_DIR) for f in files)
    if (files and in_state) or (in_state and (name in rules.state_names or last in rules.state_names)):
        return "state"
    if name in rules.services or (last in rules.services and "::" not in name):
        return "service"
    if name in rules.host:
        return "host"
    if last.startswith("bx_") or last in rules.bitexact_names:
        return "bitexact"
    if not files or all(f == LIBRARY_FILE for f in files):
        return "library"
    if symbol in rules.code_defs:
        owners = {rules.function_subsystem.get((f, base_name(symbol))) for f in rules.code_defs[symbol]} - {None}
        if owners:
            return "own" if owners == {subsystem} else None
        return "own" if any(subsystem in rules.file_subsystems.get(f, ()) for f in rules.code_defs[symbol]) else None
    if any(rules.own_file(subsystem, f) for f in files) or compiler_made(symbol, rules, subsystem, files):
        return "own"
    return "const" if name in rules.const_names or last in rules.const_names else None


def allowed(rules: Rules, subsystem: str, symbol: str) -> bool:
    """True when a function of the subsystem may reference the symbol."""
    return classify(rules, subsystem, symbol) is not None


def compiler_made(symbol: str, rules: Rules, subsystem: str, files: set) -> bool:
    """String literals, vtables and floating-point constants an object of the subsystem's file defines."""
    return bool(re.match(r"\?\?_[C7]|__real@|__xmm@|\?\?_R", symbol)) and any(
        subsystem in rules.file_subsystems.get(f, ()) for f in files)


def violations(units: list, root: Path) -> list:
    return analyse(units, root)[0]


def analyse(units: list, root: Path) -> tuple:
    """(violations, {(subsystem, kind): allowed (function, symbol) pairs}) of the units."""
    rules = build_rules(root, units)
    found, kept = set(), {}
    for unit in units:
        for function, symbol in unit.refs:
            name = base_name(function)
            subsystem = rules.function_subsystem.get((unit.file, name))
            if subsystem not in SIMULATION:
                continue
            why = classify(rules, subsystem, symbol)
            if why:
                kept.setdefault((subsystem, why), set()).add((name, symbol))
                continue
            kind = "code" if symbol in rules.code_defs else "data"
            owner = rules.code_owner(symbol) if kind == "code" else rules.data_owner(symbol)
            found.add(Violation(subsystem, name, base_name(symbol), kind, owner))
    ordered = sorted(found, key=lambda v: (v.subsystem, v.function, v.symbol, v.kind, v.owner))
    return ordered, {k: len(v) for k, v in kept.items()}


def keys(vs: list) -> list:
    return sorted({(v.subsystem, v.function, v.symbol) for v in vs})


def read_exceptions(root: Path) -> list:
    path = root / EXCEPTIONS
    if not path.exists():
        return []
    with path.open(newline="") as f:
        return [(r["subsystem"], r["function"], r["symbol"], r["reason"]) for r in csv.DictReader(f)]


def render(rows: list) -> str:
    buf = io.StringIO()
    writer = csv.writer(buf, lineterminator="\n")
    writer.writerow(COLUMNS)
    writer.writerows(sorted(rows))
    return buf.getvalue()


def check(vs: list, rows: list) -> list:
    """The problems: a violation without a row, a row that no longer occurs, a row without a TODO(#n) reason."""
    problems = []
    listed = {(s, f, y) for s, f, y, _ in rows}
    found = set(keys(vs))
    for s, f, y in sorted(found - listed):
        problems.append(f"{s} {f} -> {y}: not allowed and not in {EXCEPTIONS}")
    for s, f, y, reason in sorted(rows):
        if (s, f, y) not in found:
            problems.append(f"{EXCEPTIONS}: stale row {s} {f} -> {y}, remove the row")
        if not REASON.match(reason):
            problems.append(f"{EXCEPTIONS}: {s} {f} -> {y}: reason {reason!r} is not TODO(#n): ...")
    return problems


def summary(vs: list, allowed_counts: dict = None) -> str:
    lines = []
    for s in SIMULATION:
        mine = [v for v in vs if v.subsystem == s]
        kinds = Counter(v.kind for v in mine)
        owners = Counter(v.owner for v in mine).most_common(5)
        top = ", ".join(f"{o} {n}" for o, n in owners) or "none"
        lines.append(f"{s}: {len(mine)} violations ({kinds['code']} code, {kinds['data']} data); top owners: {top}")
        if allowed_counts is not None:
            by = ", ".join(f"{k} {allowed_counts.get((s, k), 0)}" for k in KINDS)
            lines.append(f"{s}: allowed references by rule: {by}")
    total = Counter(v.kind for v in vs)
    lines.append(f"total: {len(vs)} violations ({total['code']} code, {total['data']} data)")
    if allowed_counts is not None:
        by = ", ".join(f"{k} {sum(n for (_, kk), n in allowed_counts.items() if kk == k)}" for k in KINDS)
        lines.append(f"total allowed references by rule: {by}")
    return "\n".join(lines)


def response(arg: str) -> list:
    if arg.startswith("@"):
        return [Path(line.strip()) for line in Path(arg[1:]).read_text().splitlines() if line.strip()]
    return [Path(arg)]


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--objects", nargs="+", required=True, help="object files, or @file listing them")
    parser.add_argument("--root", default=str(ROOT))
    parser.add_argument("--write", action="store_true", help=f"write {EXCEPTIONS}")
    parser.add_argument("--check", action="store_true", help=f"fail on a violation without a row in {EXCEPTIONS} or a stale row")
    parser.add_argument("--summary", action="store_true", help="also print the counts per subsystem")
    args = parser.parse_args(argv)
    root = Path(args.root)
    units = [unit_of(p) for a in args.objects for p in response(a)]
    vs, counts = analyse(units, root)
    if args.write:
        old = {(s, f, y): r for s, f, y, r in read_exceptions(root)}
        rows = [(s, f, y, old.get((s, f, y), "TODO(#0): triage")) for s, f, y in keys(vs)]
        (root / EXCEPTIONS).write_text(render(rows))
        return 0
    if args.check:
        problems = check(vs, read_exceptions(root))
        for line in problems:
            print(line)
        return 1 if problems else 0
    for v in vs:
        print(v.line())
    if args.summary:
        print(summary(vs, counts))
    return 0


if __name__ == "__main__":
    sys.exit(main())
