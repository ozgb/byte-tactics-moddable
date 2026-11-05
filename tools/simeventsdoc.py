"""Check that docs/sim-events.md has a section for every kind of sim event, with the phases the call graph gives.

    uv run tools/simeventsdoc.py --check    # fail, naming each problem, if the doc and the kinds differ
    uv run tools/simeventsdoc.py --phases   # print the computed phases of each kind
    uv run tools/simeventsdoc.py --emitters # print, for each subsystem, the kinds its code records
    uv run tools/test_simeventsdoc.py       # tests, on synthetic inputs

--emitters prints one line per subsystem of data/subsystems.csv, `<subsystem>: KIND, KIND` in the order of the enum, or
`<subsystem>: none`. A subsystem records a kind when one of its functions (by data/subsystems.csv) holds the kind's name in
its body or calls a function that does (a recording wrapper); a call into another subsystem's slot does not count, that
subsystem records it. A function that holds a kind name and that data/subsystems.csv does not map is printed to stderr.

The kinds are the enumerators of `enum SimEventKind` in interfaces/services.h. The heading convention of the doc: a kind
has a section when a heading of level 2 or 3 (`## NAME` or `### NAME`) is exactly the enumerator's name, with nothing
else on the line. The check fails for:

- a kind with no such heading;
- a heading that starts with `SIM_EVENT_` and is not the name of a kind (a section that names no kind);
- a kind with two headings;
- a kind section with no `Phases:` line, or whose `Phases:` line differs from the phases computed below.

The files that read or define the queue (NOT_RECORDERS) name every kind without recording one. The `Phases:` line of a section is `Phases: ` and the labels in LABEL_ORDER, separated by `, `, ending with a full stop
(`Phases: none.` for a kind that is never recorded). Prose about a phase goes in another line of the section.

The phases are static reachability over the call graph of data/rwsets.csv with the hand-resolved edges of
data/hazard_edges.csv, the virtual calls tools/coupling.py resolves to the functions they can run, and a call of a virtual method as a call of each override in a derived class. A function of the sources that holds a `SIM_EVENT_` kind name in its body (a recording wrapper,
or the function that records directly) records that kind. A kind is recorded from a phase when a root of that phase
(the roots of hazards.ROOTS, labelled as in stateclasses.PHASE_LABELS) reaches, by `call` rows, a function that calls
a wrapper of the kind or is itself a recorder. The walk stops at presentation code as stateclasses.Walks does, except
at a presentation function that records an event, which is sim code by that fact. `match load` is the load roots
reaching a recorder. `outside the steps` is a function that calls a wrapper of the kind and is reached by no phase root
and no load root (console commands, interface actions, shutdown). A path is a path in the call graph, not a run: it does
not say a scenario reaches it.
"""

import argparse
import csv
import re
import sys
from pathlib import Path

import coupling
import hazards
import stateclasses as sc

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "interfaces" / "services.h"
DOC = ROOT / "docs" / "sim-events.md"
SRC = ROOT / "src"

PHASE_NAMES = {"0": "net (0)", "1": "units (1)", "2": "projectiles (2)", "3": "explosions (3)", "4": "players (4)",
               "5": "features (5)", "6": "wind (6)", "7": "meteors (7)", "8": "resource sharing (8)"}
LOAD = "match load"
OUTSIDE = "outside the steps"
LABEL_ORDER = list(PHASE_NAMES.values()) + [LOAD, OUTSIDE]
NOT_RECORDERS = {"sim_event_drain.cpp", "sim_events.cpp", "sim_events.h"}

ENUM = re.compile(r"enum\s+SimEventKind\b[^{]*\{(.*?)\}", re.S)
ENUMERATOR = re.compile(r"^\s*(SIM_EVENT_[A-Z0-9_]+)\s*(?:=[^,]*)?,", re.M)
HEADING = re.compile(r"^(#{2,3})\s+(.*?)\s*$")


def kinds(header: str) -> list[str]:
    """Return the enumerator names of SimEventKind, in order."""
    match = ENUM.search(re.sub(r"//[^\n]*", "", header))
    if match is None:
        sys.exit("simeventsdoc: no enum SimEventKind in the header")
    return ENUMERATOR.findall(match.group(1))


def headings(doc: str) -> list[str]:
    """Return the text of the level 2 and 3 headings, outside code fences."""
    out = []
    fenced = False
    for line in doc.splitlines():
        if line.startswith("```"):
            fenced = not fenced
        elif not fenced:
            match = HEADING.match(line)
            if match:
                out.append(match.group(2))
    return out


KIND_TOKEN = re.compile(r"\bSIM_EVENT_[A-Z0-9_]+\b")
NOT_FUNCTION = {"if", "for", "while", "switch", "struct", "class", "enum", "union", "namespace", "extern", "catch"}


def function_bodies(text: str) -> list[tuple[str, str]]:
    """The (name, body) of each function defined at the top level of a source text, without comments."""
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    out = []
    depth, start, begin = 0, 0, 0
    for i, ch in enumerate(text):
        if ch == "{":
            if depth == 0:
                begin = i
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                head = text[start:begin]
                match = re.search(r"([~\w:]+)\s*\(", head)
                words = re.findall(r"\w+", head)
                if match and not NOT_FUNCTION & set(words[:1]) and "=" not in head.split("(")[0]:
                    out.append((match.group(1).split("::")[-1], text[begin:i]))
                start = i + 1
        elif ch == ";" and depth == 0:
            start = i + 1
    return out


def recorders(sources: dict[str, str], names: list[str]) -> dict[str, set[str]]:
    """The functions that hold the name of a kind in their body, by kind."""
    known = set(names)
    out: dict[str, set[str]] = {name: set() for name in names}
    for path, text in sources.items():
        if Path(path).name in NOT_RECORDERS:
            continue
        for function, body in function_bodies(text):
            for token in set(KIND_TOKEN.findall(body)) & known:
                out[token].add(function)
    return out


def emitters(names: list[str], sources: dict[str, str], rows: list[list[str]], edges: list[tuple[str, str]],
             subsystems: list[dict]) -> tuple[dict[str, list[str]], list[str]]:
    """The kinds recorded by the code of each subsystem (in the order of names), and the recorders no subsystem maps.

    `subsystems` has the rows of data/subsystems.csv (function, file, subsystem). A recorder is mapped by its name and the
    path of its file, else by its name alone; a caller of a recorder is mapped by its name."""
    graph = hazards.Graph(rows, edges)
    callers: dict[str, set[str]] = {}
    for function, targets in graph.calls.items():
        for target in targets:
            callers.setdefault(target, set()).add(function)
    by_name: dict[str, set[str]] = {}
    by_file: dict[tuple[str, str], set[str]] = {}
    for row in subsystems:
        by_name.setdefault(row["function"], set()).add(row["subsystem"])
        by_name.setdefault(row["function"].split("::")[-1], set()).add(row["subsystem"])
        by_file.setdefault((row["function"].split("::")[-1], row["file"]), set()).add(row["subsystem"])
    known = set(names)
    out: dict[str, set[str]] = {row["subsystem"]: set() for row in subsystems}
    unmapped: set[str] = set()
    for path, text in sources.items():
        if Path(path).name in NOT_RECORDERS:
            continue
        for function, body in function_bodies(text):
            held = set(KIND_TOKEN.findall(body)) & known
            if not held:
                continue
            owners: set[str] = set()
            for key, owner in by_file.items():
                if key[0] == function and path.endswith(key[1]):
                    owners |= owner
            owners = owners or by_name.get(function, set())
            if not owners:
                unmapped.add(function)
            for subsystem in owners:
                out.setdefault(subsystem, set()).update(held)
            for caller in callers.get(function, ()):
                for subsystem in by_name.get(caller, ()):
                    out.setdefault(subsystem, set()).update(held)
    return {s: [n for n in names if n in kinds_of] for s, kinds_of in sorted(out.items())}, sorted(unmapped)


def computed_phases(names: list[str], sources: dict[str, str], rows: list[list[str]], edges: list[tuple[str, str]]) -> dict:
    """The phase labels (of LABEL_ORDER) from which a call path reaches a record of each kind."""
    graph = hazards.Graph(rows, edges)
    walks = sc.Walks(rows, graph)
    by_kind = recorders(sources, names)
    callers: dict[str, set[str]] = {}
    for function, targets in graph.calls.items():
        for target in targets:
            callers.setdefault(target, set()).add(function)
    all_recorders = set().union(*by_kind.values()) if by_kind else set()
    direct = {f for f in all_recorders if f in graph.functions}
    stops = walks.stops - set().union(*(callers.get(w, set()) for w in all_recorders), direct)
    reached = {label: set() for label in PHASE_NAMES.values()}
    for root, label in sc.PHASE_LABELS.items():
        reached[PHASE_NAMES[label]] |= sc.reach(graph, [root], stops)
    load = sc.reach(graph, sc.LOAD_ROOTS, stops)
    anywhere = set().union(*reached.values()) | load | {sc.DRIVER}
    out = {}
    for name, wrappers in by_kind.items():
        sites = {f for w in wrappers for f in callers.get(w, ())} | (wrappers & graph.functions)
        labels = {label for label, seen in reached.items() if sites & seen}
        if sites & load:
            labels.add(LOAD)
        if sites - anywhere:
            labels.add(OUTSIDE)
        out[name] = [label for label in LABEL_ORDER if label in labels]
    return out


def override_edges(rows: list[list[str]], classes) -> list[tuple[str, str]]:
    """(caller, override) for each `call` row of a virtual method, to the overrides of the classes derived from the callee's."""
    defined = {r[1] for r in rows if r[3] == "function"}
    out = set()
    for _, function, _, kind, target in rows:
        cls, sep, method = target.rpartition("::")
        if kind != "call" or not sep or cls not in classes.bases and cls not in classes.virtuals:
            continue
        if ("~" if method.startswith("~") else method) not in classes.slots(cls):
            continue
        for callee in coupling.resolve_virtual(classes, cls, method, defined):
            if callee != target:
                out.add((function, callee))
    return sorted(out)


def phase_lines(doc: str) -> dict[str, str | None]:
    """The text after `Phases:` in the section of each kind (None when there is no such line)."""
    out: dict[str, str | None] = {}
    current = None
    fenced = False
    for line in doc.splitlines():
        if line.startswith("```"):
            fenced = not fenced
        match = None if fenced else HEADING.match(line)
        if match:
            current = match.group(2) if match.group(2).startswith("SIM_EVENT_") else None
            if current:
                out.setdefault(current, None)
        elif current and line.startswith("Phases:") and out[current] is None:
            out[current] = line[len("Phases:"):].strip()
    return out


def phase_problems(computed: dict, doc: str) -> list[str]:
    """One message for each kind whose Phases line is missing or differs from the computed phases."""
    lines = phase_lines(doc)
    out = []
    for name, labels in computed.items():
        want = ", ".join(labels) + "." if labels else "none."
        have = lines.get(name)
        if name not in lines:
            continue
        if have is None:
            out.append("kind %s has no `Phases:` line (the call graph gives `Phases: %s`)" % (name, want))
        elif have != want:
            out.append("kind %s: the page says `Phases: %s` and the call graph gives `Phases: %s`" % (name, have, want))
    return out


def problems(names: list[str], doc: str) -> list[str]:
    """Return one message for each way the doc and the kinds differ."""
    found = headings(doc)
    known = set(names)
    out = []
    for name in names:
        count = found.count(name)
        if count == 0:
            out.append("kind %s has no section (add `### %s`)" % (name, name))
        elif count > 1:
            out.append("kind %s has %d sections" % (name, count))
    for text in found:
        if text.startswith("SIM_EVENT_") and text not in known:
            out.append("section %r names no kind of SimEventKind" % text)
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if a kind has no section, a section names no kind, or a Phases line differs from the call graph")
    parser.add_argument("--phases", action="store_true", help="print the computed phases of each kind")
    parser.add_argument("--emitters", action="store_true", help="print the kinds recorded by the code of each subsystem")
    parser.add_argument("--header", type=Path, default=HEADER, help="the header (default interfaces/services.h)")
    parser.add_argument("--doc", type=Path, default=DOC, help="the doc (default docs/sim-events.md)")
    args = parser.parse_args()
    if args.check + args.phases + args.emitters != 1:
        sys.exit("simeventsdoc: pass exactly one of --check, --phases and --emitters")
    names = kinds(args.header.read_text())
    sources = {str(p): p.read_text(errors="replace") for p in sorted(SRC.rglob("*")) if p.suffix in (".h", ".cpp", ".inc")}
    rows, graph = hazards.load_graph()
    edges = hazards.read_edges(hazards.EDGES, graph.functions)
    edges += [(call.caller, callee) for call in coupling.virtual_inputs()[0] for callee in sorted(call.callees)]
    edges += override_edges(rows, coupling.Classes.read(ROOT))
    if args.emitters:
        with coupling.SUBSYSTEMS_CSV.open(newline="") as f:
            table = list(csv.DictReader(f))
        found, unmapped = emitters(names, sources, rows, edges, table)
        for subsystem, kinds_of in found.items():
            print("%s: %s" % (subsystem, ", ".join(kinds_of) or "none"))
        for function in unmapped:
            print("simeventsdoc: recorder %s has no row in data/subsystems.csv" % function, file=sys.stderr)
        return
    phases = computed_phases(names, sources, rows, edges)
    if args.phases:
        for name in names:
            print("%s: %s" % (name, ", ".join(phases[name]) or "none"))
        return
    doc = args.doc.read_text() if args.doc.exists() else ""
    found = problems(names, doc) + phase_problems(phases, doc)
    for message in found:
        print("simeventsdoc: " + message)
    if found:
        sys.exit(1)
    print("simeventsdoc: %d kinds, each with one section in %s" % (len(names), args.doc.name))


if __name__ == "__main__":
    main()
