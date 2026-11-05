"""Write docs/dependencies.md from the dependency classes of #830 and check that it is up to date.

    uv run tools/dependenciesdoc.py --write    # regenerate docs/dependencies.md from the data files
    uv run tools/dependenciesdoc.py --check    # fail if docs/dependencies.md differs from what the data files give
    uv run tools/test_dependenciesdoc.py       # tests, on synthetic inputs

The inputs are data/dependencies.csv, phase_dependencies.csv, phase_boundaries.csv, draws.csv, pass_reads.csv,
dependency_checks.csv and presentation_reads.csv, the scenario names of reference/scenarios and
coupling.SIM_SUBSYSTEMS. Every number of the doc comes from them. The prose of the limits section is fixed in this file.
"""

import argparse
import csv
import sys
from collections import Counter, defaultdict
from pathlib import Path

import coupling

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "data"
DOC = ROOT / "docs" / "dependencies.md"
SCENARIOS = ROOT / "reference" / "scenarios"

CLASSES = ["ordered", "immediate", "deferrable", "read-only", "unobserved"]
LISTED = ["ordered", "immediate", "deferrable"]
BLOCKING = ("immediate", "ordered")
EDGE_CAP = 40
PASS_CAP = 15

LIMITS = """\
Step 1 (the access log, tools/accesslog.py) sees less than the whole program, and every class inherits that:

- Only native modules are instrumented. The WASM modules are not, so a run in WASM mode records nothing.
- One thread is recorded. Accesses of other threads are not seen.
- The CRT's string and sort functions are not instrumented, so what they read or write is not attributed.
- The rest of the list in the docstring of tools/accesslog.py applies as written there.
- Coverage: an edge is classified from the scenarios of the suite and no more. `deferrable`, `read-only` and
  `unobserved` claim only that those runs did not show a read after a write. A scenario that reaches the code
  differently could.

Gaps in what the data say:

- There are no `net` rows. No scenario of the test mode runs marker 0 of a tick, so the `net` phase and the
  boundary `0|1` have no edges.
- Some rows of data/state_classes.csv have no map symbol, so they never appear as items. A coupling edge on one of
  them is `unobserved` whatever the code does.
- Accesses to `unresolved:*` items (an image or block range that no state row names) are dropped and are not edges.
- The replay corpus (#389) is not played by accesslog.py, so the classes come from the scenario suite alone (#847).
- The `ui-action` rows of data/presentation_reads.csv are an upper bound: they come from the static reach of the input
  handlers, because scripted UI input is not logged.
- Static-only items of ambiguous or unseen types have no row.
- Events after marker 7 are not attributed to a phase of their own.
- Open gaps of #866: nested passes, the unresolved sim-event queue, and presentation rows in the phase files.
"""


def read_csv(path: Path) -> list[dict]:
    """Return the rows of a csv file as dicts."""
    with open(path, newline="") as handle:
        return list(csv.DictReader(handle))


def load(data: Path = DATA, scenarios: Path = SCENARIOS) -> dict:
    """Read every input file into one dict."""
    names = sorted(p.stem for p in scenarios.iterdir() if p.is_file())
    return {
        "deps": read_csv(data / "dependencies.csv"),
        "phasedeps": read_csv(data / "phase_dependencies.csv"),
        "bounds": read_csv(data / "phase_boundaries.csv"),
        "draws": read_csv(data / "draws.csv"),
        "passreads": read_csv(data / "pass_reads.csv"),
        "checks": read_csv(data / "dependency_checks.csv"),
        "pres": read_csv(data / "presentation_reads.csv"),
        "sims": list(coupling.SIM_SUBSYSTEMS),
        "scenarios": len(names),
    }


def cell(text: str) -> str:
    """Escape a table cell."""
    return text.replace("|", "\\|")


def code(text: str) -> str:
    """Return text as an escaped code span, or a dash when empty."""
    return "`%s`" % cell(text) if text else "-"


def table(header: list[str], rows: list[list[str]]) -> list[str]:
    """Return the lines of a Markdown table."""
    out = ["| " + " | ".join(header) + " |", "|" + "|".join("---" for _ in header) + "|"]
    out += ["| " + " | ".join(row) + " |" for row in rows]
    return out


def tight_groups(deps: list[dict], sims: list[str]) -> dict[str, list[list[str]]]:
    """Return the groups of two or more subsystems joined by immediate or ordered edges, per phase and in the union."""
    edges: dict[str, set] = defaultdict(set)
    simset = set(sims)
    for row in deps:
        if row["class"] in BLOCKING and row["writer"] in simset and row["reader"] in simset and row["writer"] != row["reader"]:
            edges[row["phase"]].add((row["writer"], row["reader"]))
            edges["all"].add((row["writer"], row["reader"]))
    return {phase: components(pairs, sims) for phase, pairs in sorted(edges.items(), key=lambda kv: (kv[0] == "all", kv[0]))}


def components(pairs: set, sims: list[str]) -> list[list[str]]:
    """Return the connected components of two or more members of an undirected graph, in the order of sims."""
    parent = {s: s for s in sims}

    def find(x: str) -> str:
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for a, b in pairs:
        parent[find(a)] = find(b)
    groups: dict[str, list[str]] = defaultdict(list)
    for s in sims:
        groups[find(s)].append(s)
    found = [g for g in groups.values() if len(g) >= 2]
    return sorted(found, key=lambda g: (-len(g), sims.index(g[0])))


def cut_from(sub: str, deps: list[dict], sims: list[str]) -> tuple[list[str], dict[str, dict]]:
    """Return the subsystems sub can be cut from and, for the others, the blocking edge that shows the most runs."""
    blockers: dict[str, dict] = {}
    for row in deps:
        if row["class"] not in BLOCKING:
            continue
        if row["writer"] == sub:
            other = row["reader"]
        elif row["reader"] == sub:
            other = row["writer"]
        else:
            continue
        if other == sub or other not in sims:
            continue
        best = blockers.get(other)
        if best is None or (-int(row["runs"]), row["class"] != "ordered") < (-int(best["runs"]), best["class"] != "ordered"):
            blockers[other] = row
    return [s for s in sims if s != sub and s not in blockers], blockers


def held_claim(checks: list[dict]) -> str:
    """Return the sentence about defer_held, built from the rows of dependency_checks.csv."""
    if not checks:
        return "There are no deferrable edges, so there are no confirmation runs."
    nonzero = [r for r in checks if int(r["defer_held"]) != 0]
    groups = {r["group"] for r in checks}
    counted = sum(int(r["scenarios_counted"]) for r in {r["group"]: r for r in checks}.values())
    if not nonzero:
        return ("`defer_held` is 0 for every one of the %d edges in %d groups (%d scenario runs counted in all): "
                "in no run did a reader meet a write that the barrier held back." % (len(checks), len(groups), counted))
    each = "; ".join("%s: %s" % (g, sorted({r["defer_held"] for r in nonzero if r["group"] == g})[-1])
                     for g in sorted({r["group"] for r in nonzero}))
    return ("`defer_held` is not 0 everywhere. It is above 0 for %d of %d edges, in %d groups (%s); there a reader "
            "met a held write, and the `equal` result of the group is the evidence." % (len(nonzero), len(checks),
                                                                                      len({r["group"] for r in nonzero}), each))


def edge_rows(rows: list[dict], sub: str, cls: str) -> list[list[str]]:
    """Return table rows of the edges of one class that sub is on, most runs first."""
    mine = [r for r in rows if r["class"] == cls and sub in (r["writer"], r["reader"])]
    mine.sort(key=lambda r: (-int(r["runs"]), r["writer"], r["reader"], r["item"], r["phase"]))
    return [[("writes to " + r["reader"]) if r["writer"] == sub else ("reads from " + r["writer"]), code(r["item"]),
             r["phase"], r["runs"], r["example"]] for r in mine]


def section(sub: str, d: dict) -> list[str]:
    """Return the lines of the section of one subsystem."""
    deps, sims = d["deps"], d["sims"]
    out = ["## %s" % sub, ""]
    count = Counter()
    for r in deps:
        if r["writer"] == sub:
            count[(r["class"], "w")] += 1
        if r["reader"] == sub:
            count[(r["class"], "r")] += 1
    out += ["### Edges by class", ""]
    out += table(["class", "as writer", "as reader"], [[c, str(count[(c, "w")]), str(count[(c, "r")])] for c in CLASSES])
    out.append("")
    for cls in LISTED:
        rows = edge_rows(deps, sub, cls)
        out.append("**%s** (%d)." % (cls.capitalize(), len(rows)))
        out.append("")
        if not rows:
            out += ["None.", ""]
            continue
        out += table(["direction", "item", "phase", "runs", "example"], rows[:EDGE_CAP])
        if len(rows) > EDGE_CAP:
            out += ["", "The %d rows with the most runs are shown; the other %d are in data/dependencies.csv." %
                    (EDGE_CAP, len(rows) - EDGE_CAP)]
        out.append("")
    for cls in ("read-only", "unobserved"):
        per_other: dict[str, Counter] = defaultdict(Counter)
        for r in deps:
            if r["class"] != cls:
                continue
            if r["writer"] == sub:
                per_other[r["reader"]]["writer"] += 1
            if r["reader"] == sub:
                per_other[r["writer"] if r["writer"] != "-" else "(no writer)"]["reader"] += 1
        out.append("**%s**, by the other subsystem." % cls.capitalize())
        out.append("")
        if not per_other:
            out += ["None.", ""]
            continue
        rows = [[o, str(c["writer"]), str(c["reader"])] for o, c in sorted(per_other.items())]
        out += table(["other", "%s is writer" % sub, "%s is reader" % sub], rows) + [""]
    can, blockers = cut_from(sub, deps, sims)
    out += ["### Cut from", ""]
    out.append("Can be cut from, by snapshots and phase-end events alone: %s." % (", ".join(can) if can else "none"))
    out.append("")
    if blockers:
        rows = [[o, r["class"], "%s -> %s" % (r["writer"], r["reader"]), code(r["item"]), r["phase"], r["runs"], r["example"]]
                for o, r in sorted(blockers.items(), key=lambda kv: sims.index(kv[0]))]
        out += ["Cannot be cut from, with the edge of the most runs:", ""]
        out += table(["subsystem", "class", "edge", "item", "phase", "runs", "example"], rows)
    else:
        out.append("There is no subsystem it cannot be cut from.")
    out += ["", "### Phase boundaries", ""]
    pairs: dict[tuple, Counter] = defaultdict(Counter)
    for r in d["phasedeps"]:
        for role in ("writer", "reader"):
            if r[role] == sub:
                pairs[(r["phase_from"], r["phase_to"])][(r["class"], role)] += 1
    if pairs:
        rows = [["%s -> %s" % k, str(c[("immediate", "writer")]), str(c[("immediate", "reader")]),
                 str(c[("carried", "writer")]), str(c[("carried", "reader")])] for k, c in sorted(pairs.items())]
        out += table(["phase pair", "immediate as writer", "immediate as reader", "carried as writer", "carried as reader"], rows)
        out += ["", "The rows themselves are in data/phase_dependencies.csv."]
    else:
        out.append("No row of data/phase_dependencies.csv has it as writer or reader.")
    out += ["", "### Random draws", ""]
    draws = [r for r in d["draws"] if r["subsystem"] == sub]
    if draws:
        out += table(["pass", "stream", "function", "entities", "draws", "start_state", "evidence"],
                     [[r["pass"], r["stream"], code(r["function"]), r["entities"],
                       r["draws_min"] if r["draws_min"] == r["draws_max"] else "%s to %s" % (r["draws_min"], r["draws_max"]),
                       r["start_state"], cell(r["evidence"]) or "-"] for r in draws])
    else:
        out.append("No draws.")
    out += ["", "### Within-pass reads", ""]
    reads = [r for r in d["passreads"] if r["reader"] == sub and r["item"] != "-"]
    if reads:
        grouped: dict[tuple, list] = defaultdict(list)
        for r in reads:
            grouped[(r["pass"], r["function"])].append(r)
        rows = []
        for (p, fn), rs in sorted(grouped.items(), key=lambda kv: (-len(kv[1]), kv[0])):
            same = sum(1 for r in rs if r["evidence"] == "same-record")
            top = max(rs, key=lambda r: int(r["runs"]))
            rows.append([p, code(fn), str(len(rs)), str(same), code(top["item"]), top["runs"]])
        out += ["%d rows in data/pass_reads.csv, grouped by pass and function (the item shown is the one most runs saw):" %
                len(reads), ""]
        out += table(["pass", "function", "rows", "same-record", "top item", "runs"], rows[:PASS_CAP])
        if len(rows) > PASS_CAP:
            out += ["", "The %d groups with the most rows are shown." % PASS_CAP]
    else:
        out.append("No row of data/pass_reads.csv has it as reader.")
    out += ["", "### Deferral confirmation", ""]
    checks = {(r["writer"], r["reader"], r["item"], r["phase"]): r for r in d["checks"]}
    rows = []
    for r in sorted((r for r in deps if r["class"] == "deferrable" and sub in (r["writer"], r["reader"])),
                    key=lambda r: (r["writer"], r["reader"], r["item"], r["phase"])):
        c = checks.get((r["writer"], r["reader"], r["item"], r["phase"]))
        rows.append([r["writer"] + " -> " + r["reader"], code(r["item"]), r["phase"]] +
                    ([c["group"], c["scenarios"], c["result"], c["defer_held"], c["defer_third_stores"]] if c else ["no run"] + ["-"] * 4))
    if rows:
        out += table(["edge", "item", "phase", "group", "scenarios", "result", "defer_held", "defer_third_stores"], rows)
    else:
        out.append("It has no deferrable edge.")
    return out + [""]


def render(d: dict) -> str:
    """Return the text of docs/dependencies.md for the data in d."""
    deps, sims = d["deps"], d["sims"]
    n = Counter(r["class"] for r in deps)
    out = ["# Dependencies between the subsystems", "",
           "This doc is generated by `uv run tools/dependenciesdoc.py --write` from the files of data/ and is checked "
           "in CI. Do not edit it by hand.", "",
           "It classifies the state that the %d simulation subsystems share, so that a reader who has not read the source "
           "can see which subsystems can be cut apart and which must stay together. The classes come from %d scenarios "
           "of reference/scenarios (tools/depclass.py, tools/phaseclass.py, tools/presreads.py; step 1 is "
           "tools/accesslog.py)." % (len(sims), d["scenarios"]), "",
           "## Classes", "",
           "An edge is a writer, a reader, an item and a phase. The class is the first that applies:", "",
           "- **ordered**: the order of draws from a random stream, sounds or UI records is the state, whichever subsystem comes first. "
           "The streams are items (`rng.parkMiller` for RandomInt, `rng.crtHoldrand` and `rng.crtLoad` for GameRand), so two "
           "subsystems that draw from one stream in a phase already have an `ordered` row; the draws tables below add the counts "
           "per entity, not new edges.",
           "- **immediate**: some run shows a read of the item after the writer wrote it in the same phase. The writer must run first.",
           "- **deferrable**: the reader read before the writer wrote, and nobody read the writer's value before the phase "
           "ended. The write could be queued to the end of the phase.",
           "- **read-only**: the reader reads the item and no run shows anyone writing it in that phase.",
           "- **unobserved**: an edge of data/coupling.csv that no run enters.", "",
           "Edges in data/dependencies.csv: %s." % ", ".join("%d %s" % (n[c], c) for c in CLASSES), "",
           "## Cutting rule", "",
           "S can be cut from T when no edge between them, in either direction, is `immediate` or `ordered` within a phase "
           "in data/dependencies.csv. Edges that cross a phase boundary (data/phase_dependencies.csv), and `deferrable`, "
           "`read-only` and `unobserved` edges, can be served by a phase-end snapshot or event.", ""]
    out += ["## Subsystems", ""]
    rows = []
    for s in sims:
        can, blockers = cut_from(s, deps, sims)
        rows.append([s] +
                    [str(sum(1 for r in deps if r["class"] == c and s in (r["writer"], r["reader"]))) for c in CLASSES] +
                    ["%d of %d" % (len(can), len(sims) - 1)])
    out += table(["subsystem"] + CLASSES + ["can be cut from"], rows) + [""]
    out += ["## Tightly coupled groups", "",
            "These are the largest sets of simulation subsystems joined by `immediate` or `ordered` edges. Each must stay one "
            "ordered pass per phase. They are the connected components, of two or more members, of the undirected graph "
            "of those edges, per phase and over all phases.", ""]
    groups = tight_groups(deps, sims)
    if not groups:
        out += ["There are none.", ""]
    for phase, gs in groups.items():
        title = "All phases together" if phase == "all" else "Phase `%s`" % phase
        out.append("**%s**:" % title)
        out.append("")
        out += ["- %d members: %s" % (len(g), ", ".join(g)) for g in gs]
        out.append("")
    out += ["## Phase boundaries", "",
            "From data/phase_boundaries.csv. `needed` is `yes` when an `immediate` row spans the boundary. `direct` rows "
            "span no other boundary. `carried` rows go the other way: the reader needs the writer's phase to come after it.", ""]
    out += table(["boundary", "needed", "edges", "direct", "carried", "mergeable", "example"],
                 [[cell(r["boundary"]), r["needed"], r["edges"], r["direct"], r["carried"], r["mergeable"],
                   r["example"] or "-"] for r in d["bounds"]]) + [""]
    out += ["## Deferral confirmation", "",
            "Each deferrable edge was held back to the end of its phase in a full-suite run (data/dependency_checks.csv) "
            "and the traces compared. The runs confirm the rule and the barrier within the suite's coverage.", ""]
    results = Counter(r["result"] for r in d["checks"])
    out.append("Results: %s." % (", ".join("%d %s" % (v, k) for k, v in sorted(results.items())) or "none"))
    out += ["", held_claim(d["checks"]), "",
            "The runs add no evidence on paths the suite does not reach. The control (tests/defer/immediate-control.defer) shows that the barrier changes a "
            "trace on an immediate edge, so the barrier can change what a run shows.", ""]
    groups_seen = {}
    for r in d["checks"]:
        groups_seen.setdefault(r["group"], r)
    out += table(["group", "edges", "scenarios", "counted", "result", "defer_held", "defer_third_stores"],
                 [[g, str(sum(1 for r in d["checks"] if r["group"] == g)), r["scenarios"], r["scenarios_counted"],
                   r["result"], r["defer_held"], r["defer_third_stores"]] for g, r in sorted(groups_seen.items())]) + [""]
    out += ["## Presentation reads", "",
            "The sim state that presentation reads between ticks (data/presentation_reads.csv, %d rows), by pattern and "
            "visibility. Rows of `ui-action` are an upper bound (see the limits)." % len(d["pres"]), ""]
    patterns = sorted({r["pattern"] for r in d["pres"]})
    visibilities = sorted({r["visibility"] for r in d["pres"]})
    counts = Counter((r["pattern"], r["visibility"]) for r in d["pres"])
    rows = [[p] + [str(counts[(p, v)]) for v in visibilities] + [str(sum(counts[(p, v)] for v in visibilities))] for p in patterns]
    rows.append(["all"] + [str(sum(counts[(p, v)] for p in patterns)) for v in visibilities] + [str(len(d["pres"]))])
    out += table(["pattern"] + visibilities + ["all"], rows) + [""]
    out += ["## Limits and known gaps", ""] + LIMITS.splitlines() + [""]
    for s in sims:
        out += section(s, d)
    return "\n".join(out).rstrip("\n") + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the doc differs from the data")
    parser.add_argument("--write", action="store_true", help="rewrite the doc")
    parser.add_argument("--doc", type=Path, default=DOC, help="the doc (default docs/dependencies.md)")
    args = parser.parse_args()
    if args.check == args.write:
        sys.exit("dependenciesdoc: pass exactly one of --check and --write")
    text = render(load())
    if args.write:
        args.doc.write_text(text)
        print("dependenciesdoc: wrote %s" % args.doc.name)
        return
    if not args.doc.exists() or args.doc.read_text() != text:
        print("dependenciesdoc: %s is out of date; run `uv run tools/dependenciesdoc.py --write` and commit the result" %
              args.doc.name)
        sys.exit(1)
    print("dependenciesdoc: %s matches the data" % args.doc.name)


if __name__ == "__main__":
    main()
