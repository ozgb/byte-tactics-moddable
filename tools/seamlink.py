"""Fail when a simulation subsystem reaches the game around its seams, the state block and the engine services.

    uv run tools/seamlink.py --objects @build/modern/objects.rsp            # print every violation
    uv run tools/seamlink.py --objects @build/modern/objects.rsp --summary  # also count per subsystem
    uv run tools/seamlink.py --objects @build/modern/objects.rsp --write    # write data/seam_exceptions.csv
    uv run tools/seamlink.py --objects @build/modern/objects.rsp --check
    uv run tools/seamlink.py --private-state                                # pathfinding's region, from data/rwsets.csv
    uv run tools/seamlink.py --pilot-seams                                  # economy's writing and query slots, from data/rwsets.csv
    uv run tools/test_seamlink.py                                           # tests, on hand-written units

The build puts every function in its own COMDAT section, so the relocations of a
section name the symbols one function needs. For each function of a simulation
subsystem S (data/subsystems.csv, by source file and name), each symbol its code
references is allowed only when it is the following. A function with no row (an inline,
template or compiler-made function such as Ref<T>::Ref, a static initialiser or a scalar
deleting destructor) is judged for every simulation subsystem that has a function in its
source file; a file with no simulation function is not checked:

  seam          g_seams, or anything interfaces/seams.h declares outside the table struct, or, for one of the phase slots'
                fillers in src/seams/native.cpp (UpdateAllUnits, UpdatePlayers, UpdatePlayersOnStream) only, a function of
                the scheduler (src/game/scheduler.cpp) that data/subsystems.csv gives a `phase:` rule (the engine's entry
                point for a phase, which the default seam slot of it calls)
  state         g_game, a symbol defined in src/state/, or declared in interfaces/ref.h and not defined elsewhere
  service       declared at depth 0 in interfaces/services.h, or a member of a class it defines
  host          declared in interfaces/host.h
  bitexact      a bx_ name, or declared in lib/bitexact/bitexact.h
  library       a symbol no object of the build defines (the CRT, the MSVC 5 libraries, the imports), or
                one src/util/operator_new.cpp defines (operator new and delete), or throw info (_TI,
                _CTA, _CT) or a symbol whose own scope is std (a member or static of a std template)
  const         a data symbol whose data/state_classes.csv class is const or sim
  own           defined by S: a function of S in data/subsystems.csv, or data defined in a file
                whose functions all belong to S, or a compiler-made symbol (string literal,
                vtable, constant) defined by an object of a file with a function of S

Anything else is a violation, printed as `subsystem function -> symbol (kind, owner)`:
kind is code or data, owner is the subsystem that defines a code symbol or the file that
defines the rest. data/seam_exceptions.csv (subsystem,function,symbol,reason) lists the
violations that remain, each with a `TODO(#n): ...` reason. --check fails on a violation
without a row and on a row that no longer occurs, so the list only shrinks. A row whose
symbol starts with `virtual ` is a virtual call between subsystems, which
`tools/coupling.py --virtual --check` judges and this tool ignores. 
--private-state fails when a function of any subsystem other than pathfinding reads, writes or lets escape
Game::pathfinder or a member of a type of pathfinding's state region, from the field and deref rows of
data/rwsets.csv and the owners of data/subsystems.csv.
--pilot-seams judges economy's seam (interfaces/economy.h), which is applied by the engine and written by nothing else, from
data/rwsets.csv, the header and src/seams/native.cpp. It fails on:

  slots         a slot of EconomySeam in neither list below, a listed slot that is not one, or a listed slot that the
                EconomySeam table of src/seams/native.cpp does not fill (`<table>.<slot> = <function>;`)
  callers       an indirect call through a writing slot (Apply, Spend, UpdatePlayerEconomy, UpdateMetalExtraction,
                UpdateResourceSharing, Init, Shutdown), or a field_read or field_escape of one (which a call through a
                copy of the slot needs), by any function but economy's own (data/subsystems.csv) and the engine's: the
                files of the scheduler (src/game/scheduler.cpp, which routes the events and runs the phases), of economy's
                glue (src/seams/economy_world.cpp: its inbox handler and the synchronous services), and of the harness's
                recorders (src/harness/replay_hooks.inc, which forward a slot call to the slot they replaced),
                and the functions LoadBattleAssets and ShutdownIngameSystems (src/game/game_state.cpp: Init and Shutdown
                at set-up and teardown) and RunGameSteps (src/ingame/game_dialogs.cpp: the tick's UpdateResourceSharing).
                A presentation or unowned function fails like any other.
  queries       for each query slot (RegionSize, RequestGranted, GetEnergyUse, UnitAccount, WriteState), the function
                that fills it and every function it calls by name (rwsets keys functions by name, so every overload),
                when one
                  writes anything (a *_write or *_maybe_written row), except a field or deref write, in the filler
                  itself, of the type that a non-const pointer parameter of the slot points to or of a struct type that
                  it holds (UnitAccount's out: EconAccount and EconLedger), and except the world view's info structs
                  (EconUnitInfo, EconTypeInfo, EconPlayerInfo), which no state holds;
                  makes an indirect call other than through a read-only slot of EconomyWorldView (all but Emit) or a
                  slot of a struct that a parameter of the query slot points to (WriteState's StateWriter);
                  calls a function data/rwsets.csv has no rows for (memset, memcpy, a template), but for operator=, the
                  assignment rwsets records as the write itself, and X87LeTest41, a template that compares two values;
                  or has inline assembly.
                An escape (an address taken, such as UnitAccountOf's of the region) is no write: a write through it is a
                row of the function that makes it, which the walk reaches, or of a call it is passed to (maybe_written).

rwsets records no base expression, so in UnitAccount's filler a write of an EconAccount through another pointer (the region,
from UnitAccountOf) can't be told from the write through `out`; everywhere else such a write fails. tools/privatestate.py
judges the region's writers by subsystem, from the access log, and can't tell a query slot from a writing one. Standard
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
from aliases import definition_aliases
from sources import ROOT, base_name

SUBSYSTEMS = "data/subsystems.csv"
EXCEPTIONS = "data/seam_exceptions.csv"
COLUMNS = ["subsystem", "function", "symbol", "reason"]
SIMULATION = ("wind_meteors", "economy", "features", "ai", "pathfinding", "weapons", "targeting", "los_radar",
              "orders", "units", "players", "map")
STATE_DIR = "src/state/"
LIBRARY_FILE = "src/util/operator_new.cpp"
GAME_POINTER = "g_game"
THROW_PREFIXES = ("_TI", "_CTA", "_CT", "__TI", "__CTA", "__CT")
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
VIRTUAL_PREFIX = "virtual "
REL_DIR32, REL_REL32 = 6, 20
RWSETS = "data/rwsets.csv"
PRIVATE_OWNER = "pathfinding"
PRIVATE_ITEMS = ("Game::pathfinder",)
PRIVATE_TYPES = ("NodeData", "Node", "PathCell", "PathGrid", "TurnCostTable", "TurnCostPair", "OpenHeap", "PathPending",
                 "PathExtra", "Pathfinder")
PRIVATE_KINDS = ("field_read", "field_write", "field_escape", "field_maybe_written", "deref_read", "deref_write",
                 "deref_escape", "deref_maybe_written")
ECONOMY_OWNER = "economy"
ECONOMY_WRITING_SLOTS = ("Apply", "Spend", "UpdatePlayerEconomy", "UpdateMetalExtraction", "UpdateResourceSharing", "Init", "Shutdown")
ECONOMY_QUERY_SLOTS = ("RegionSize", "RequestGranted", "GetEnergyUse", "UnitAccount", "WriteState")
ECONOMY_HEADER = "interfaces/economy.h"
ECONOMY_SEAM = "EconomySeam"
ECONOMY_WORLD = "EconomyWorldView"
ECONOMY_WORLD_READS = ("Tick", "Difficulty", "SharingEnabled", "Wind", "Tide", "PlayerInfo", "Allied", "FirstUnit", "LastUnit",
                       "NextUnit", "UnitSlot", "UnitInfo", "TypeInfo", "CellMetal")
ECONOMY_INFO_TYPES = ("EconUnitInfo", "EconTypeInfo", "EconPlayerInfo")
ENGINE_FUNCTIONS = (("LoadBattleAssets", "src/game/game_state.cpp"), ("ShutdownIngameSystems", "src/game/game_state.cpp"),
                    ("RunGameSteps", "src/ingame/game_dialogs.cpp"))
ROWLESS_CALLS = ("operator=", "X87LeTest41")
NATIVE_TABLE = "src/seams/native.cpp"
SCHEDULER_FILE = "src/game/scheduler.cpp"
ECONOMY_GLUE_FILE = "src/seams/economy_world.cpp"
ENGINE_FILES = (SCHEDULER_FILE, ECONOMY_GLUE_FILE, "src/harness/replay_hooks.inc")
PHASE_FILLERS = ((NATIVE_TABLE, "UpdateAllUnits"), (NATIVE_TABLE, "UpdatePlayers"), (NATIVE_TABLE, "UpdatePlayersOnStream"))
QUERY_WRITE_KINDS = ("field_write", "field_maybe_written", "global_write", "global_maybe_written", "deref_write", "deref_maybe_written")
OUT_KINDS = ("field_write", "deref_write")
SLOT_READ_KINDS = ("field_read", "field_escape")
SLOT_POINTER = re.compile(r"\(\s*__stdcall\s*\*\s*(\w+)\s*\)\s*\(([^)]*)\)")


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
    phases: set = field(default_factory=set)

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


def phase_functions(root: Path) -> set:
    """The functions of the scheduler's file that data/subsystems.csv gives a `phase:` rule: the engine's entry points for a phase, which the default seam slots call."""
    path = root / SUBSYSTEMS
    if not path.exists():
        return set()
    with path.open(newline="") as f:
        return {r["function"] for r in csv.DictReader(f)
                if r.get("rule", "").startswith("phase:") and r["file"] == SCHEDULER_FILE}


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
                 header_names(root, BITEXACT_HEADER), const_names(root), code_defs, data_defs, phase_functions(root))


def classify(rules: Rules, subsystem: str, symbol: str, caller: tuple = None):
    """The kind of rule that lets a function of the subsystem reference the symbol, or None for a violation.

    caller is the referencing function's (file, name); only a phase slot's filler may reach a phase function."""
    name = normal(symbol)
    last = name.split("::")[-1]
    if last == GAME_POINTER:
        return "state"
    if name in rules.seams or last in rules.seams or (last in rules.phases and "::" not in name and caller in PHASE_FILLERS):
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
    if not files or all(f == LIBRARY_FILE for f in files) or compiler_library(symbol):
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


def name_chain(text: str, i: int) -> tuple:
    """The fragments of a decorated qualifier chain starting at i, and the index after its closing @."""
    out = []
    while text[i] != "@":
        if text.startswith("?$", i):
            j = text.index("@", i)
            out.append(text[i:j])
            i = template_args(text, j + 1)
        elif text[i].isdigit():
            out.append(text[i])
            i += 1
        else:
            j = text.index("@", i)
            out.append(text[i:j])
            i = j + 1
    return out, i + 1


def template_args(text: str, i: int) -> int:
    """The index after the @ that closes the template arguments starting at i."""
    while text[i] != "@":
        c = text[i]
        if c in "UVT":
            i = name_chain(text, i + 1)[1]
        elif c == "W":
            i = name_chain(text, i + 2)[1]
        elif c == "$":
            i = text.index("@", i)
        else:
            i += 1
    return i + 1


def own_scope_is_std(symbol: str) -> bool:
    """True when the outermost qualifier of the symbol's own name is std, whatever its type arguments are."""
    text = symbol
    local = re.search(r"\?\d+\?\?", text)
    if local:
        text = "?" + text[local.end():]
    if not text.startswith("?"):
        return False
    start = 1
    if text.startswith("??"):
        start = 4 if text[2] == "_" else 3
    try:
        fragments, _ = name_chain(text, start)
    except (IndexError, ValueError):
        return base_name(symbol).startswith("std::")
    return len(fragments) > 1 and fragments[-1] == "std" or base_name(symbol).startswith("std::")


def compiler_library(symbol: str) -> bool:
    """Throw info and catchable types (_TI, _CTA, _CT) and symbols whose own scope is std."""
    return symbol.startswith(THROW_PREFIXES) or own_scope_is_std(symbol)


def compiler_made(symbol: str, rules: Rules, subsystem: str, files: set) -> bool:
    """String literals, vtables and floating-point constants an object of the subsystem's file defines."""
    return bool(re.match(r"\?\?_[C7]|__real@|__xmm@|\?\?_R", symbol)) and any(
        subsystem in rules.file_subsystems.get(f, ()) for f in files)


def violations(units: list, root: Path, aliases: dict = None) -> list:
    return analyse(units, root, aliases)[0]


def analyse(units: list, root: Path, aliases: dict = None) -> tuple:
    """(violations, {(subsystem, kind): allowed (function, symbol) pairs}) of the units.

    A call to a symbol no object defines, which the link resolves through aliases to a definition at the same
    address, is judged against that definition."""
    rules = build_rules(root, units)
    aliases = aliases or {}
    found, kept = set(), {}
    for unit in units:
        for function, symbol in unit.refs:
            name = base_name(function)
            listed = rules.function_subsystem.get((unit.file, name))
            if listed is None:
                judged = sorted(s for s in rules.file_subsystems.get(unit.file, ()) if s in SIMULATION)
            else:
                judged = [listed] if listed in SIMULATION else []
            for subsystem in judged:
                landed = symbol
                why = classify(rules, subsystem, symbol, (unit.file, name))
                target = aliases.get(symbol)
                if why == "library" and target and (target in rules.code_defs or target in rules.data_defs):
                    landed = target
                    why = classify(rules, subsystem, target, (unit.file, name))
                if why:
                    kept.setdefault((subsystem, why), set()).add((name, symbol))
                    continue
                kind = "code" if landed in rules.code_defs else "data"
                owner = rules.code_owner(landed) if kind == "code" else rules.data_owner(landed)
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
    rows = [r for r in rows if not r[2].startswith(VIRTUAL_PREFIX)]
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


def private_state(root: Path) -> list:
    """(subsystem, function, kind, target) of every access to pathfinding's region by another subsystem."""
    owners = {}
    with (root / SUBSYSTEMS).open(newline="") as f:
        for r in csv.DictReader(f):
            owners[(r["function"], r["file"])] = r["subsystem"]
    types = set(PRIVATE_TYPES)
    found = set()
    with (root / RWSETS).open(newline="") as f:
        for r in csv.DictReader(f):
            if r["kind"] not in PRIVATE_KINDS:
                continue
            target = r["target"]
            if target not in PRIVATE_ITEMS and not types & set(re.findall(r"\w+", target)):
                continue
            owner = owners.get((r["function"], r["file"]), "unowned")
            if owner != PRIVATE_OWNER:
                found.add((owner, r["function"], r["kind"], target))
    return sorted(found)


def struct_body(text: str, name: str) -> str:
    """The body of a struct the text defines, without comments, or an empty string."""
    text = clean(text)
    m = re.search(r"\bstruct\s+%s\s*\{" % re.escape(name), text)
    if not m:
        return ""
    depth, i = 1, m.end()
    while i < len(text) and depth:
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        i += 1
    return text[m.end():i - 1]


def slot_parameters(header: str, struct: str) -> dict:
    """{slot: [parameter declaration]} of the function pointer slots of a struct the header defines."""
    return {m.group(1): [p.strip() for p in m.group(2).split(",") if p.strip() and p.strip() != "void"]
            for m in SLOT_POINTER.finditer(struct_body(header, struct))}


def pointee(parameter: str):
    """(type, writable) of a pointer parameter, or None for any other."""
    if "*" not in parameter:
        return None
    words = re.findall(r"\w+", parameter.split("*")[0])
    names = [w for w in words if w not in ("const", "struct", "unsigned", "signed", "volatile")]
    return (names[-1] if names else "void", "const" not in words)


def held_types(header: str, struct: str) -> set:
    """The struct, and every struct type the header defines that it holds as a member, at any depth."""
    out, todo = set(), [struct]
    while todo:
        s = todo.pop()
        if s in out:
            continue
        out.add(s)
        for stmt in struct_body(header, s).split(";"):
            words = re.findall(r"\w+", stmt)
            if words and struct_body(header, words[0]):
                todo.append(words[0])
    return out


def written_type(target: str) -> str:
    """The type a field or deref row writes: Type of `Type::field` or of `*Type`."""
    return target.lstrip("*").split("::")[0].strip()


def native_fillers(native: str) -> dict:
    """{slot: function} that src/seams/native.cpp assigns to the slots of its EconomySeam table."""
    table = re.search(r"\b%s\s+(\w+)\s*;" % ECONOMY_SEAM, native)
    if not table:
        return {}
    return {m.group(1): m.group(2) for m in re.finditer(r"\b%s\.(\w+)\s*=\s*(\w+)\s*;" % re.escape(table.group(1)), native)}


def query_problems(functions: dict, owners: dict, slot: str, filler: str, out_types: set, callable_types: set) -> set:
    """The ("query", subsystem, function, what) problems of the filler of a query slot and every function it calls by name."""
    found = set()
    what = f"{ECONOMY_SEAM}::{slot}"
    todo, seen = [filler], set()
    while todo:
        fn = todo.pop()
        if fn in seen:
            continue
        seen.add(fn)
        if fn not in functions:
            found.add(("query", "unowned", fn, f"{what} calls {fn}, which {RWSETS} has no rows for"))
            continue
        for r in functions[fn]:
            owner = owners.get((fn, r["file"]), "unowned")
            kind, target = r["kind"], r["target"]
            if kind == "call":
                if target not in ROWLESS_CALLS or target in functions:
                    todo.append(target)
            elif kind == "indirect_call":
                via = re.findall(r"\[(\w+)::(\w+)\]", target)
                if not via or not all((t == ECONOMY_WORLD and s in ECONOMY_WORLD_READS) or (t != ECONOMY_WORLD and t in callable_types)
                                      for t, s in via):
                    found.add(("query", owner, fn, f"{what} calls {target}"))
            elif kind == "asm":
                found.add(("query", owner, fn, f"{what} has inline assembly"))
            elif kind in QUERY_WRITE_KINDS:
                written = written_type(target)
                if written in ECONOMY_INFO_TYPES or (fn == filler and kind in OUT_KINDS and written in out_types):
                    continue
                found.add(("query", owner, fn, f"{what} {kind.replace('_', ' ')} {target}"))
    return found


def pilot_seams(root: Path) -> list:
    """(kind, subsystem, function, what) of every problem the module docstring lists under --pilot-seams: kind slot, call or query."""
    owners = {}
    with (root / SUBSYSTEMS).open(newline="") as f:
        for r in csv.DictReader(f):
            owners[(r["function"], r["file"])] = r["subsystem"]
    with (root / RWSETS).open(newline="") as f:
        rows = list(csv.DictReader(f))
    header_path, native_path = root / ECONOMY_HEADER, root / NATIVE_TABLE
    header = header_path.read_text(encoding="latin-1") if header_path.exists() else ""
    fillers = native_fillers(native_path.read_text(encoding="latin-1") if native_path.exists() else "")
    slots = slot_parameters(header, ECONOMY_SEAM)
    found = set()
    for slot in sorted(set(slots) - set(ECONOMY_WRITING_SLOTS) - set(ECONOMY_QUERY_SLOTS)):
        found.add(("slot", ECONOMY_OWNER, slot, f"{ECONOMY_SEAM}::{slot} is in neither the writing nor the query slots"))
    for slot in ECONOMY_WRITING_SLOTS + ECONOMY_QUERY_SLOTS:
        if slot not in slots:
            found.add(("slot", ECONOMY_OWNER, slot, f"{ECONOMY_SEAM}::{slot} is not a slot of {ECONOMY_SEAM} in {ECONOMY_HEADER}"))
        elif slot not in fillers:
            found.add(("slot", ECONOMY_OWNER, slot, f"{ECONOMY_SEAM}::{slot} is not filled in {NATIVE_TABLE}"))
    writing = {f"{ECONOMY_SEAM}::{s}" for s in ECONOMY_WRITING_SLOTS}
    for r in rows:
        if r["kind"] == "indirect_call":
            used = re.findall(r"\[(%s::\w+)\]" % ECONOMY_SEAM, r["target"])
        elif r["kind"] in SLOT_READ_KINDS:
            used = [r["target"]]
        else:
            continue
        owner = owners.get((r["function"], r["file"]), "unowned")
        engine = r["file"] in ENGINE_FILES or (r["function"], r["file"]) in ENGINE_FUNCTIONS
        for slot in used:
            if slot in writing and owner != ECONOMY_OWNER and not engine:
                found.add(("call", owner, r["function"], slot))
    functions = {}
    for r in rows:
        functions.setdefault(r["function"], []).append(r)
    for slot in ECONOMY_QUERY_SLOTS:
        if slot not in slots or slot not in fillers:
            continue
        pointers = [p for p in map(pointee, slots[slot]) if p]
        out_types = set().union(*[held_types(header, t) for t, writable in pointers if writable])
        found |= query_problems(functions, owners, slot, fillers[slot], out_types, {t for t, _ in pointers})
    return sorted(found)


def response(arg: str) -> list:
    if arg.startswith("@"):
        return [Path(line.strip()) for line in Path(arg[1:]).read_text().splitlines() if line.strip()]
    return [Path(arg)]


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--objects", nargs="+", default=[], help="object files, or @file listing them")
    parser.add_argument("--private-state", action="store_true",
                        help=f"fail on an access to pathfinding's state region by another subsystem, from {RWSETS}")
    parser.add_argument("--pilot-seams", action="store_true",
                        help=f"fail on a call into economy's writing slots by another subsystem or a write behind its query slots, from {RWSETS}")
    parser.add_argument("--root", default=str(ROOT))
    parser.add_argument("--write", action="store_true", help=f"write {EXCEPTIONS}")
    parser.add_argument("--check", action="store_true", help=f"fail on a violation without a row in {EXCEPTIONS} or a stale row")
    parser.add_argument("--summary", action="store_true", help="also print the counts per subsystem")
    args = parser.parse_args(argv)
    root = Path(args.root)
    if args.private_state:
        bad = private_state(root)
        for owner, function, kind, target in bad:
            print(f"{owner} {function} {kind} {target}: pathfinding's region is private to {PRIVATE_OWNER}")
        return 1 if bad else 0
    if args.pilot_seams:
        bad = pilot_seams(root)
        for kind, owner, function, what in bad:
            if kind == "slot":
                print(f"{what}: every slot of {ECONOMY_SEAM} must be listed and filled")
            elif kind == "call":
                print(f"{owner} {function} calls {what}: only the engine applies economy's writing slots")
            else:
                print(f"{owner} {function} behind {what}: economy's query slots write no state and call no writer")
        return 1 if bad else 0
    if not args.objects:
        parser.error("--objects is required")
    units = [unit_of(p) for a in args.objects for p in response(a)]
    vs, counts = analyse(units, root, definition_aliases([p for a in args.objects for p in response(a)]))
    if args.write:
        old = {(s, f, y): r for s, f, y, r in read_exceptions(root)}
        rows = [(s, f, y, old.get((s, f, y), "TODO(#0): triage")) for s, f, y in keys(vs)]
        rows += [(s, f, y, r) for (s, f, y), r in old.items() if y.startswith(VIRTUAL_PREFIX)]
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
