"""List the sim state that presentation reads between ticks, with how often and what a player view must do about it.

    uv run tools/presreads.py --write                  # writes data/presentation_reads.csv
    uv run tools/presreads.py --check                  # fail if the file differs from what the stored data regenerate
    uv run tools/presreads.py --store build/access/access   # reduce the access log's presentation files into data/access_presentation/
    uv run tools/test_presreads.py                     # tests, on synthetic inputs

This is step 5 of #830. data/presentation_reads.csv has one row per (item, reader subsystem) for each item of sim state
that the presentation window of a run read, or that a function of the presentation subsystem reads by the static
read sets of data/rwsets.csv although no run reached it. Columns:

  item                 the access log's item name: a global or a region of Game as data/state_classes.csv names it,
                       or `Type::field` for a record's field
  reader               the subsystem of the function that read it (data/subsystems.csv); `presentation` for the
                       static-only rows
  pattern              every-frame, ui-action, after-event or unknown (below)
  covered_by_event     a SIM_EVENT_* kind of interfaces/services.h (name as written there), or empty
  visibility           own, all-players or per-player-filter (below)
  runs                 the number of scenarios of reference/scenarios whose presentation window showed the row;
                       0 for a static-only row

Input. `tools/accesslog.py run` writes build/access/access/<scenario>.presentation.csv (see its docstring): per item,
reader and function, in how many presentation windows (one per tick, from HarnessAfterStep to the next HarnessBeforeStep)
the function read the item and, for each SimEvent kind, in how many of those windows the tick before had emitted it.
Those counts follow the wall clock (how many frames the main loop draws, when a timer fires), so `--store` keeps only
what two runs of a scenario agree on and writes it to data/access_presentation/<scenario>.csv with the columns
item,reader,function,container,every,after:

  every      1 when the function read the item in every active window (a window in which anything was read)
  after      the SimEvent kinds (numbers, `|` between) that preceded every window the function read the item in and that
             did not precede every window of the run (so a kind that every tick emits, like the radar capture, says
             nothing); `-` when the function read it in fewer than 3 windows, which is too few to tell
  container  the variable, region of Game or allocation the item lies in: the row of state_classes.csv that gives its class

`--store` keeps the rows whose container is of class sim or mixed in data/state_classes.csv and drops the rest.

Sim items. An item is sim state when the class column of its container's row in data/state_classes.csv is `sim` or
`mixed` (a mixed row holds state that the sim writes and presentation also writes; the sim part still has to be in the view).
The tool reports the rows it dropped as view, platform or const state in --write's output.

Rows. One per (item, reader subsystem), over all the stored scenarios; `runs` counts the scenarios in which any function of
that subsystem read the item.

pattern, in this order:
  every-frame  `every` holds for the row in any scenario
  after-event  the reading windows of the row always followed one event kind in every scenario that could tell: per scenario
               the intersection of the `after` sets of the functions, then the intersection over the scenarios
  ui-action    a function that read the item is reached from UI input handling in data/rwsets.csv: the call graph from the
               in-game input handlers listed in UI_ROOTS (keys, clicks, the order and build panels, the edge scroll)
  after-event  (again) a function that read the item is reached from exactly one case of DrainSimEvents
  unknown      none of the above
Scripted input is not logged: the harness switches the access log off while it plays a scenario's UI records, so the
`ui-action` pattern comes from data/rwsets.csv only, never from a run. The call graph follows direct calls and not calls
through function pointers or virtual methods, so a reach is an upper bound; a function that no direct call reaches is
`unknown`.

covered_by_event. The kind whose case of DrainSimEvents (src/ingame/sim_event_drain.cpp) reaches the reading function by direct
calls in data/rwsets.csv, when exactly one kind reaches a function of the row; else the kind the runs found to precede it
(the smallest, when more); empty when neither says. The name is the enum's, as in interfaces/services.h.

visibility, by the item's type or container (VISIBILITY below): `per-player-filter` for units, features, projectiles,
debris and their pieces and models, and a unit's orders (what another player sees depends on line of sight and radar),
`own` for the per-player records (the player array and the unnamed ranges of its elements, resources, the local player, the
eyes, the line-of-sight tables and the fog cells), `all-players` for the rest (map, wind, tide, gravity, the tick, the
unit and weapon definitions). The rule is the two sets in VISIBILITY; the first pass over the runs' items found the
player ranges that the layout leaves unnamed (`unknown_1b8e`, `p1b8a` and the like) and the orders and fog cells wrongly
under all-players, and they are listed there by name.

static-only rows. For each function of the subsystem `presentation` in data/subsystems.csv, the `field_read` and `global_read`
rows of data/rwsets.csv name the items it reads. An item with no row from any run becomes a row with reader `presentation`,
runs 0 and the pattern by static reach only. The class of a `Type::field` item no run showed comes from the containers that
runs showed for other fields of Type, else from a heap row of state_classes.csv named for the type, else from the
members of struct Game in src/game/game.h; a type whose containers are all sim or mixed is sim (or mixed), one whose
containers are none is not; with the two kinds mixed, or with no container, the item is left out and counted.

Limits: the evidence is the 83 scenarios' test mode, native modules only (see accesslog.py); a window is a tick's whole
stretch of main-loop work, so `every-frame` means every tick's, not every drawn frame's; the wall clock makes the
rare reads (timers, flashes) come and go between two runs of a scenario.
"""

import argparse
import csv
import io
import re
import sys
from collections import defaultdict
from pathlib import Path

import blockpointers
import sources

ROOT = sources.ROOT
OUT = ROOT / "data/presentation_reads.csv"
STORED = ROOT / "data/access_presentation"
STATE_CLASSES = ROOT / "data/state_classes.csv"
RWSETS = ROOT / "data/rwsets.csv"
SUBSYSTEMS = ROOT / "data/subsystems.csv"
SERVICES = ROOT / "interfaces/services.h"
DRAIN = ROOT / "src/ingame/sim_event_drain.cpp"
GAME_HEADER = ROOT / "src/game/game.h"
COLUMNS = ["item", "reader", "pattern", "covered_by_event", "visibility", "runs"]
STORED_COLUMNS = ["item", "reader", "function", "container", "every", "after"]
SIM_CLASSES = ("sim", "mixed")
MIN_WINDOWS = 3
THIN = "-"
UI_ROOTS = [
    "HandleGameKey", "HandleLeftClick", "HandleRightClick", "HandleBuildPanelClick", "HandleOrderButtonClick",
    "HandleOrdersPanelClick", "HandleViewMapClick", "HandleOptionsPanelClick", "HandleInGameOptionsClick",
    "HandleButtonInput", "HandleHotspotInput", "HandleListBoxInput", "HandleSliderInput", "HandleTextInput",
    "HandleTextEditKey", "HandleVirtualKey", "HandleDebugHotkey", "GetKeyboardInput", "UpdateEdgeScroll",
    "UpdateMouseScroll", "BeginMouseScroll", "EndMouseScroll", "ClickSelectHoverUnit", "CenterCameraOnRadarClick",
]
VISIBILITY = {
    "per-player-filter": {
        "types": {"Unit", "UnitMotion", "Feature", "FeatureAnimEntry", "FeaturePseudoUnit", "Projectile", "Debris",
                  "DebrisPiece", "PieceFrame", "ModelState", "DebrisCache", "Order"},
        "names": {"units", "unitsEnd", "unit", "projectiles", "projectileCount", "features", "featureCount", "debris",
                  "g_explodedPieces", "slots", "frames", "vertices"},
    },
    "own": {
        "types": {"Player", "PlayerRef", "PlayerInfo", "PlayerInfoEntry", "PlayerIdEntry", "PlayerGrid", "UnitResources",
                  "Eyeball", "Resources", "FogCell"},
        "names": {"players", "localPlayer", "playerIndex", "playerIds", "eyes", "losTable", "visibilityMask", "numPlayers",
                  "unknown_1b8e", "unknown_1cd9", "p1b8a", "p1cd5", "pad_1df4", "InitPlayerSlot: operator new unsigned char"},
    },
}
KIND_LINE = re.compile(r"^\s*(SIM_EVENT_[A-Z0-9_]+)\s*=\s*(\d+)\s*,", re.M)
CASE_LINE = re.compile(r"\bcase\s+(SIM_EVENT_[A-Z0-9_]+)\s*:")
MEMBER = re.compile(r"^\s+(?:struct\s+)?([A-Z]\w*)\s+(\w+)(?:\[\w+\])*;", re.M)
CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "case", "break", "static_cast", "const", "else"}


def read_csv(path: Path) -> list[dict]:
    """The rows of a csv file, by column name."""
    with path.open(newline="") as fh:
        return list(csv.DictReader(fh))


def kind_names(text: str) -> dict[int, str]:
    """{number: name} of the SIM_EVENT_* enumerators of interfaces/services.h, without SIM_EVENT_NONE."""
    return {int(n): k for k, n in KIND_LINE.findall(text) if int(n) != 0}


def dispatch_calls(text: str) -> dict[str, set[str]]:
    """{kind name: names called in its case} of the switch in DrainSimEvents; a case runs to the next one."""
    start = text.index("void DrainSimEvents()")
    body = text[start:]
    cases = list(CASE_LINE.finditer(body))
    out: dict[str, set[str]] = {}
    for i, m in enumerate(cases):
        end = cases[i + 1].start() if i + 1 < len(cases) else body.find("default:", m.end())
        out.setdefault(m.group(1), set()).update(n for n in CALL.findall(body[m.end():end]) if n not in KEYWORDS)
    return out


class CallGraph:
    """The direct calls of data/rwsets.csv by function name, and what it reads."""

    def __init__(self, rows: list[dict]):
        self.calls: dict[str, set[str]] = defaultdict(set)
        self.functions: set[str] = set()
        self.reads: dict[tuple[str, str], set[str]] = defaultdict(set)
        for r in rows:
            if r["kind"] == "function":
                self.functions.add(r["function"])
            elif r["kind"] == "call":
                self.calls[r["function"]].add(r["target"])
            elif r["kind"] in ("field_read", "global_read"):
                self.reads[(r["function"], r["file"])].add(r["target"])

    def closure(self, starts) -> set[str]:
        """The functions the starting ones reach by direct calls, the starting ones included."""
        seen: set[str] = set()
        stack = [s for s in starts if s in self.functions]
        while stack:
            f = stack.pop()
            if f in seen:
                continue
            seen.add(f)
            stack.extend(c for c in self.calls.get(f, ()) if c not in seen)
        return seen

    def known(self, name: str) -> str | None:
        """The rwsets spelling of a function name from the access log, or None."""
        for candidate in dict.fromkeys([name, sources.base_name(name), blockpointers.demangle(name)]):
            if candidate in self.functions:
                return candidate
        return None


def classes_of(rows: list[dict]) -> dict[str, str]:
    """{row name: class} of data/state_classes.csv; a name on more than one row keeps the first non-empty class."""
    out: dict[str, str] = {}
    for r in rows:
        if r["class"] and not out.get(r["name"]):
            out[r["name"]] = r["class"]
    return out


def reduce_scenario(rows: list[dict], stats: dict[str, int], classes: dict[str, str]) -> list[dict]:
    """The stable columns of one scenario's presentation.csv: every, after and the container, for sim and mixed containers."""
    active = stats["presentation_active_windows"]
    out = []
    for r in rows:
        if classes.get(r["container"]) not in SIM_CLASSES:
            continue
        windows = int(r["windows"])
        kinds = {}
        for part in filter(None, r["events"].split(";")):
            k, _, c = part.partition(":")
            kinds[int(k)] = int(c)
        if windows < MIN_WINDOWS:
            after = THIN
        else:
            after = "|".join(str(k) for k in sorted(kinds)
                             if k != 0 and kinds[k] == windows and stats.get("presentation_kind_%d" % k, 0) < active)
        out.append({"item": r["item"], "reader": r["reader"], "function": r["function"], "container": r["container"],
                    "every": "1" if active > 0 and windows == active else "0", "after": after})
    return sorted(out, key=lambda r: tuple(r[c] for c in STORED_COLUMNS))


def read_stats(path: Path) -> dict[str, int]:
    """The `name value` lines of a stats file."""
    out = {}
    for line in path.read_text().splitlines():
        key, _, value = line.partition(" ")
        out[key] = int(value)
    return out


def write_rows(path: Path, columns: list[str], rows: list[dict]) -> None:
    """Write rows as csv with \\n line ends."""
    buffer = io.StringIO()
    w = csv.DictWriter(buffer, columns, lineterminator="\n")
    w.writeheader()
    w.writerows(rows)
    path.write_text(buffer.getvalue())


def store(access_dir: Path, out_dir: Path, classes: dict[str, str]) -> int:
    """Reduce every <scenario>.presentation.csv of access_dir into out_dir; the number of scenarios."""
    out_dir.mkdir(exist_ok=True)
    count = 0
    for path in sorted(access_dir.glob("*.presentation.csv")):
        name = path.name[: -len(".presentation.csv")]
        rows = reduce_scenario(read_csv(path), read_stats(access_dir / (name + ".stats.txt")), classes)
        write_rows(out_dir / (name + ".csv"), STORED_COLUMNS, rows)
        count += 1
    return count


def load_stored(directory: Path) -> dict[str, list[dict]]:
    """{scenario: rows} of the stored directory."""
    return {p.stem: read_csv(p) for p in sorted(directory.glob("*.csv"))}


def visibility(item: str, container: str | None) -> str:
    """own, all-players or per-player-filter by the item's type (before `::`) or its container."""
    kind = item.split("::")[0] if "::" in item else None
    for label in ("per-player-filter", "own"):
        rule = VISIBILITY[label]
        if kind in rule["types"] or item in rule["names"] or container in rule["names"]:
            return label
    return "all-players"


def type_classes(stored: dict[str, list[dict]], classes: dict[str, str], rows: list[dict],
                 header: str = "") -> dict[str, set[str]]:
    """{type: classes of the containers that runs showed for its fields}, plus the heap rows named for a type and the
    members of struct Game (src/game/game.h) whose name is a region row."""
    out: dict[str, set[str]] = defaultdict(set)
    regions = region_index(rows)
    for kind, name in MEMBER.findall(header):
        if name in regions and classes.get(regions[name]):
            out[kind].add(classes[regions[name]])
    for scenario in stored.values():
        for r in scenario:
            if "::" in r["item"]:
                out[r["item"].split("::")[0]].add(classes.get(r["container"], ""))
    for r in rows:
        if r["kind"] == "heap":
            last = r["name"].rsplit(" ", 1)[-1]
            if re.fullmatch(r"[A-Z]\w+", last) and r["class"]:
                out[last].add(r["class"])
    return out


def region_index(rows: list[dict]) -> dict[str, str]:
    """{field of Game: name of the region row that holds it}."""
    out = {}
    for r in rows:
        if r["kind"] == "game-region":
            for part in r["name"].split("|"):
                out.setdefault(part, r["name"])
    return out


def static_item(target: str, classes: dict[str, str], regions: dict[str, str],
                types: dict[str, set[str]]) -> tuple[str, str] | None:
    """(item name, class) of a field_read or global_read target of rwsets.csv, or None when it names no state item."""
    if "::" in target:
        owner, _, name = target.partition("::")
        if owner == "Game":
            region = regions.get(name)
            return (region, classes.get(region, "")) if region else None
        if owner == "SimGlobals":
            return (name, classes.get(name, "")) if name in classes else None
        found = types.get(owner, set()) - {""}
        if found and found <= set(SIM_CLASSES):
            return target, "sim" if "sim" in found else "mixed"
        if found and not found & set(SIM_CLASSES):
            return target, min(found)
        return None
    return (target, classes[target]) if target in classes else None


def build(stored: dict[str, list[dict]], classes_rows: list[dict], rwsets_rows: list[dict], subsystem_rows: list[dict],
          services: str, drain: str, header: str = "") -> tuple[list[dict], dict[str, int]]:
    """The rows of data/presentation_reads.csv and a report of what was left out."""
    kinds = kind_names(services)
    kind_number = {n: k for k, n in kinds.items()}
    graph = CallGraph(rwsets_rows)
    ui = graph.closure(UI_ROOTS)
    reach: dict[str, set[str]] = defaultdict(set)
    for kind, names in dispatch_calls(drain).items():
        for f in graph.closure(names):
            reach[f].add(kind)
    classes = classes_of(classes_rows)

    def sole_kinds(names) -> list[str]:
        """The kinds of the cases that reach a function of names, counting only a function that one case reaches."""
        return sorted({next(iter(reach[f])) for f in names if f and len(reach.get(f, ())) == 1}, key=kind_number.get)

    groups: dict[tuple[str, str], dict] = {}
    for scenario, rows in stored.items():
        per_scenario: dict[tuple[str, str], list[dict]] = defaultdict(list)
        for r in rows:
            per_scenario[(r["item"], r["reader"])].append(r)
        for key, found in per_scenario.items():
            g = groups.setdefault(key, {"runs": 0, "every": False, "functions": set(), "after": None, "container": set()})
            g["runs"] += 1
            g["container"].update(r["container"] for r in found)
            g["every"] = g["every"] or any(r["every"] == "1" for r in found)
            g["functions"].update(r["function"] for r in found)
            sets = [set(map(int, r["after"].split("|"))) if r["after"] else set() for r in found if r["after"] != THIN]
            if sets:
                here = set.intersection(*sets)
                g["after"] = here if g["after"] is None else g["after"] & here
    out = []
    for (item, reader), g in groups.items():
        known = [graph.known(f) for f in g["functions"]]
        static_kinds = sole_kinds(known)
        run_kinds = sorted(g["after"] or ())
        covered = static_kinds[0] if static_kinds else (kinds[run_kinds[0]] if run_kinds else "")
        if g["every"]:
            pattern = "every-frame"
        elif run_kinds:
            pattern = "after-event"
        elif any(f in ui for f in known if f):
            pattern = "ui-action"
        elif static_kinds:
            pattern = "after-event"
        else:
            pattern = "unknown"
        out.append({"item": item, "reader": reader, "pattern": pattern, "covered_by_event": covered,
                    "visibility": visibility(item, min(g["container"])), "runs": g["runs"]})
    report = {"runtime": len(out), "static_unclassified": 0, "static_not_sim": 0}
    reached = {r["item"] for r in out}
    presentation = {(r["function"], r["file"]) for r in subsystem_rows if r["subsystem"] == "presentation"}
    regions = region_index(classes_rows)
    types = type_classes(stored, classes, classes_rows, header)
    wanted: dict[str, set[str]] = defaultdict(set)
    for (function, file), targets in graph.reads.items():
        if (function, file) not in presentation:
            continue
        for target in targets:
            found = static_item(target, classes, regions, types)
            if found is None:
                report["static_unclassified"] += 1
            elif found[1] not in SIM_CLASSES:
                report["static_not_sim"] += 1
            elif found[0] not in reached:
                wanted[found[0]].add(function)
    for item, functions in wanted.items():
        static_kinds = sole_kinds(functions)
        if static_kinds:
            pattern = "after-event"
        elif any(f in ui for f in functions):
            pattern = "ui-action"
        else:
            pattern = "unknown"
        out.append({"item": item, "reader": "presentation", "pattern": pattern,
                    "covered_by_event": static_kinds[0] if static_kinds else "",
                    "visibility": visibility(item, None), "runs": 0})
    report["static_only"] = len(wanted)
    return sorted(out, key=lambda r: (r["item"], r["reader"])), report


def render(rows: list[dict]) -> str:
    """The text of data/presentation_reads.csv."""
    buffer = io.StringIO()
    w = csv.DictWriter(buffer, COLUMNS, lineterminator="\n")
    w.writeheader()
    w.writerows(rows)
    return buffer.getvalue()


def generate() -> tuple[str, dict[str, int]]:
    """The file's text and the report, from the stored data and the data files."""
    rows, report = build(load_stored(STORED), read_csv(STATE_CLASSES), read_csv(RWSETS), read_csv(SUBSYSTEMS),
                         SERVICES.read_text(), DRAIN.read_text(), GAME_HEADER.read_text())
    return render(rows), report


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--write", action="store_true", help="write data/presentation_reads.csv")
    ap.add_argument("--check", action="store_true", help="fail if data/presentation_reads.csv is out of date")
    ap.add_argument("--store", type=Path, metavar="DIR", help="reduce the presentation files of DIR into data/access_presentation/")
    args = ap.parse_args()
    if args.store:
        print("presreads: stored %d scenarios in %s" % (store(args.store, STORED, classes_of(read_csv(STATE_CLASSES))), STORED))
        return 0
    text, report = generate()
    print("presreads: %d rows from runs, %d static-only; static items left out: %d unclassified, %d not sim" % (
        report["runtime"], report["static_only"], report["static_unclassified"], report["static_not_sim"]))
    if args.write:
        OUT.write_text(text)
        return 0
    if args.check:
        if not OUT.is_file() or OUT.read_text() != text:
            old = OUT.read_text().splitlines() if OUT.is_file() else []
            new = text.splitlines()
            for i in range(max(len(old), len(new))):
                a = old[i] if i < len(old) else "<end>"
                b = new[i] if i < len(new) else "<end>"
                if a != b:
                    print("presreads: data/presentation_reads.csv differs at line %d:\n  file:      %s\n  generated: %s" % (i + 1, a, b))
                    break
            print("presreads: run `uv run tools/presreads.py --write`")
            return 1
        print("presreads: data/presentation_reads.csv is current")
        return 0
    sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
