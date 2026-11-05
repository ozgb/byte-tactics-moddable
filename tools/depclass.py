"""Classify the edges between simulation subsystems inside a phase from the stored access-log summaries.

    uv run tools/accesslog.py run --build-dir build/access <every scenario>
    uv run tools/depclass.py --store build/access/access    # keeps the edges files, without the presentation rows, gzipped
    uv run tools/depclass.py --write         # writes data/dependencies.csv from data/access/*.edges.csv
    uv run tools/depclass.py --check         # fail if data/dependencies.csv differs from what the stored summaries regenerate,
                                             # if a scenario has no summary, or if a coupling.csv edge has no row
    uv run tools/test_depclass.py            # tests, on synthetic summaries

This is step 2 of #830. Its input is data/access/<scenario>.edges.csv.gz, one file per scenario of
reference/scenarios, which tools/accesslog.py (#835) writes (columns writer,reader,item,phase,evidence,count,
first_tick). The stored files are the output of `accesslog.py run` with the rows of the `presentation` window
dropped (their counts follow the wall clock; #839 reads them) and gzipped with a zero mtime, so that the same
rows give the same bytes; `tail` is kept and is classified as a phase.
This tool reads only those files, data/coupling.csv and the names of reference/scenarios, never build/.

Simulation subsystems are coupling.SIM_SUBSYSTEMS: wind_meteors, economy, features, ai, pathfinding, weapons,
targeting, los_radar, orders, units, players, map. A row whose writer or reader is anything else (game, network,
harness, seams, files, platform, presentation, unknown, or `-` for no subsystem) is not an edge between two
simulation subsystems, and neither is a row of one subsystem with itself or a row whose item is `unresolved:*`
(the access fell in an image or block range no state row names). Those rows are dropped, except that a `-`
writer is how read-only is found.

data/dependencies.csv has the columns writer,reader,item,phase,class,runs,example,flags, sorted. The edge is
(writer, reader, item, phase); the class is the first that applies, in this order (a run is one scenario):

  ordered     the item is a random stream (`rng.*`), `sound` or `ui`, or a row has the evidence `ordered`: the
              order of the draws or emissions is the state, whichever subsystem comes first. Any run.
  immediate   some run shows the evidence `read-after-write` for the edge.
  deferrable  some run enters the edge and no run shows a read after the write: the same run shows the writer's
              write left unread at the end of the phase (a `write-unread` row of the writer) and the reader's read
              of the item with no write before it (a `read-unwritten` row of the reader) in that phase. The
              reader read what it read before the writer wrote, and the writer's value was not read by anyone
              before the phase ended, so the write could be queued to the end of the phase.
  read-only   writer `-`, reader a simulation subsystem: the reader reads the item in the phase in some run and no
              run shows the item written in that phase by anyone (any subsystem, simulation or not).
  unobserved  an edge of data/coupling.csv between two simulation subsystems that no run enters (writer, reader and
              item with a row in no phase, in any class above), with phase `-`, runs 0 and example `-`.

`runs` is the number of scenarios that show the edge in the class it is given. `example` is
`<scenario>@<first_tick>`: the scenario with the smallest first tick among those runs, the alphabetically first
on a tie. For deferrable it is the later of the two rows' first ticks (when the edge was entered).
`flags` holds `dynamic-only` when data/coupling.csv has no row of any kind from the writer to the reader: the
runs found a dependency that the static analysis did not.

Coupling edges used. Only the `shared_write` rows of data/coupling.csv (from writes, to reads or writes the item
`target`) are state edges and can be unobserved: `from` is the writer and `to` the reader. The `call` rows between
simulation subsystems are none (tools/coupling.py --seams fails on one), and the `indirect` rows are calls through
seam slots and function pointers: they carry no state item, the access log records no calls, so they have no row
here.

Gaps. No scenario of the test mode runs marker 0 of a tick, so the `net` phase has no rows. 304 static and 130
global rows of data/state_classes.csv have no symbol in the map and never appear as items, so a coupling edge on one
of them is `unobserved` whatever the code does; about 2% of the accesses fall in `unresolved:image` or
`unresolved:block` and are not edges. Only native modules are instrumented, one thread is recorded, and the CRT's
string and sort functions are not seen (tools/accesslog.py lists the rest). An edge is classified from the 83
scenarios and no more: `deferrable` and `unobserved` claim only that those runs did not show a read, and a scenario
that reaches the code differently could. The replay corpus of #389 is not read: accesslog.py has no way to play a
replay.
"""

import argparse
import csv
import difflib
import gzip
import io
import sys
from collections import defaultdict
from pathlib import Path

import coupling
import sources

ROOT = sources.ROOT
ACCESS_DIR = ROOT / "data/access"
SUFFIX = ".edges.csv"
COUPLING_CSV = ROOT / "data/coupling.csv"
SCENARIOS = ROOT / "reference/scenarios"
OUTPUT = ROOT / "data/dependencies.csv"
COLUMNS = ["writer", "reader", "item", "phase", "class", "runs", "example", "flags"]
SIM = frozenset(coupling.SIM_SUBSYSTEMS)
NONE = "-"
ORDERED_ITEMS = ("sound", "ui")
ORDERED_PREFIX = "rng."
DYNAMIC_ONLY = "dynamic-only"
RANK = {"ordered": 0, "immediate": 1, "deferrable": 2, "read-only": 3, "unobserved": 4}


def is_ordered_item(item: str) -> bool:
    """Whether the item is a random stream or the sound or UI records, whose order is the state."""
    return item.startswith(ORDERED_PREFIX) or item in ORDERED_ITEMS


def read_summary(text: str) -> list[dict]:
    """The rows of one edges file."""
    return [{**r, "first_tick": int(r["first_tick"])} for r in csv.DictReader(io.StringIO(text))]


def load_summaries(directory: Path) -> dict[str, list[dict]]:
    """Scenario name to the rows of its stored edges file."""
    return {p.name[: -len(SUFFIX + ".gz")]: read_summary(gzip.decompress(p.read_bytes()).decode())
            for p in sorted(directory.glob("*" + SUFFIX + ".gz"))}


def store(source: Path, directory: Path) -> int:
    """Keep each edges file of source (accesslog.py's output) in directory, gzipped, without the presentation rows."""
    directory.mkdir(exist_ok=True)
    count = 0
    for path in sorted(source.glob("*" + SUFFIX)):
        lines = [line for line in path.read_text().splitlines(keepends=True) if ",presentation," not in line]
        name = directory / (path.name + ".gz")
        name.write_bytes(gzip.compress("".join(lines).encode(), 9, mtime=0))
        count += 1
    return count


def read_coupling(text: str) -> list[dict]:
    """The rows of data/coupling.csv."""
    return list(csv.DictReader(io.StringIO(text)))


def classify(summaries: dict[str, list[dict]], coupling_rows: list[dict]) -> list[dict]:
    """The rows of data/dependencies.csv for the summaries of the scenarios and the rows of data/coupling.csv."""
    hits: dict[tuple, dict[str, dict[str, int]]] = defaultdict(lambda: defaultdict(dict))
    written: set[tuple[str, str]] = set()
    reads: dict[tuple, dict[str, int]] = defaultdict(dict)
    entered: set[tuple[str, str, str]] = set()

    def hit(key: tuple, cls: str, scenario: str, tick: int) -> None:
        runs = hits[key][cls]
        runs[scenario] = min(tick, runs.get(scenario, tick))

    for scenario, rows in summaries.items():
        unread: dict[tuple, int] = {}
        unwritten: dict[tuple, int] = {}
        for r in rows:
            item, phase, writer, reader, tick = r["item"], r["phase"], r["writer"], r["reader"], r["first_tick"]
            if item.startswith("unresolved:"):
                continue
            if writer != NONE:
                written.add((item, phase))
            if reader in SIM and writer == NONE and r["evidence"] == "read-unwritten":
                reads[(reader, item, phase)][scenario] = min(tick, reads[(reader, item, phase)].get(scenario, tick))
                unwritten[(reader, item, phase)] = tick
            if writer in SIM and reader == NONE and r["evidence"] == "write-unread":
                unread[(writer, item, phase)] = tick
            if writer not in SIM or reader not in SIM or writer == reader:
                continue
            key = (writer, reader, item, phase)
            entered.add((writer, reader, item))
            if r["evidence"] == "ordered" or is_ordered_item(item):
                hit(key, "ordered", scenario, tick)
            elif r["evidence"] == "read-after-write":
                hit(key, "immediate", scenario, tick)
        by_item: dict[tuple, list[tuple[str, int]]] = defaultdict(list)
        for (writer, item, phase), tick in unread.items():
            by_item[(item, phase)].append((writer, tick))
        for (reader, item, phase), read_tick in unwritten.items():
            for writer, write_tick in by_item.get((item, phase), []):
                if writer == reader:
                    continue
                cls = "ordered" if is_ordered_item(item) else "deferrable"
                hit((writer, reader, item, phase), cls, scenario, max(read_tick, write_tick))
    for (reader, item, phase), runs in reads.items():
        if (item, phase) not in written:
            hits[(NONE, reader, item, phase)]["read-only"] = runs

    pairs = {(r["from"], r["to"]) for r in coupling_rows}
    out = []
    for key, classes in hits.items():
        cls = min(classes, key=RANK.__getitem__)
        if cls == "deferrable" and "immediate" in classes:
            cls = "immediate"
        runs = classes[cls]
        example = min(runs.items(), key=lambda kv: (kv[1], kv[0]))
        flags = DYNAMIC_ONLY if key[0] != NONE and (key[0], key[1]) not in pairs else ""
        out.append({"writer": key[0], "reader": key[1], "item": key[2], "phase": key[3], "class": cls,
                    "runs": len(runs), "example": "%s@%d" % (example[0], example[1]), "flags": flags})
    for r in coupling_rows:
        if r["kind"] == "shared_write" and r["from"] in SIM and r["to"] in SIM and r["from"] != r["to"] \
                and (r["from"], r["to"], r["target"]) not in entered:
            out.append({"writer": r["from"], "reader": r["to"], "item": r["target"], "phase": NONE,
                        "class": "unobserved", "runs": 0, "example": NONE, "flags": ""})
    return sorted(out, key=lambda r: (r["writer"], r["reader"], r["item"], r["phase"], r["class"]))


def render(rows: list[dict]) -> str:
    """The text of data/dependencies.csv."""
    buf = io.StringIO()
    w = csv.DictWriter(buf, COLUMNS, lineterminator="\n")
    w.writeheader()
    w.writerows(rows)
    return buf.getvalue()


def missing_scenarios(summaries: dict[str, list[dict]]) -> list[str]:
    """The scenarios of reference/scenarios that have no stored summary."""
    return sorted(p.stem for p in SCENARIOS.glob("*.txt") if p.stem not in summaries)


def uncovered(rows: list[dict], coupling_rows: list[dict]) -> list[tuple[str, str, str]]:
    """The shared_write edges between simulation subsystems of coupling.csv that have no row in rows."""
    have = {(r["writer"], r["reader"], r["item"]) for r in rows}
    return sorted({(c["from"], c["to"], c["target"]) for c in coupling_rows
                   if c["kind"] == "shared_write" and c["from"] in SIM and c["to"] in SIM and c["from"] != c["to"]
                   and (c["from"], c["to"], c["target"]) not in have})


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--store", type=Path, metavar="DIR", help="store the edges files of DIR in data/access")
    mode.add_argument("--write", action="store_true", help="write data/dependencies.csv")
    mode.add_argument("--check", action="store_true", help="fail if data/dependencies.csv is stale or a coupling edge has no row")
    args = ap.parse_args()
    if args.store:
        print("depclass: stored %d summaries" % store(args.store, ACCESS_DIR))
        return 0
    summaries = load_summaries(ACCESS_DIR)
    coupling_rows = read_coupling(COUPLING_CSV.read_text())
    rows = classify(summaries, coupling_rows)
    text = render(rows)
    if args.write:
        OUTPUT.write_text(text)
        print("depclass: %d rows from %d scenarios" % (len(rows), len(summaries)))
        return 0
    problems = []
    for name in missing_scenarios(summaries):
        problems.append("no summary data/access/%s.edges.csv.gz for reference/scenarios/%s.txt" % (name, name))
    for edge in uncovered(rows, coupling_rows):
        problems.append("coupling edge %s -> %s on %s has no row" % edge)
    have = OUTPUT.read_text() if OUTPUT.is_file() else ""
    if have != text:
        diff = difflib.unified_diff(have.splitlines(), text.splitlines(), "data/dependencies.csv", "regenerated", lineterm="", n=0)
        problems.append("data/dependencies.csv differs from the stored summaries (uv run tools/depclass.py --write):\n" + "\n".join(list(diff)[:40]))
    if problems:
        print("\n".join(problems))
        return 1
    print("depclass: data/dependencies.csv matches %d summaries, %d rows" % (len(summaries), len(rows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
