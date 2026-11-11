"""Check, over the stored access-log summaries, that pathfinding's state is private and that its written state shows no shadow field.

    uv run tools/accesslog.py run --build-dir build/access <every scenario>
    uv run tools/privatestate.py --store build/access/access    # keeps the reads files, gzipped, in data/access_reads
    uv run tools/privatestate.py --check                        # both checks below, on the stored data
    uv run tools/test_privatestate.py                           # tests, on synthetic summaries

Private. The state of the default pathfinder is one allocation of the state block (the region, allocated by the engine in LoadBattleAssets and set up by PathfindingInit
and reached through `Game::pathfinder`, the item `pathfinder`). tools/accesslog.py names the parts of the allocation as
`Pathfinder::field`, `PathExtra::field`, `Node::field`, `PathCell::field` and `PathPending::field` and the others
`<allocation>.<part>`. --check fails on any row of the stored summaries (data/access/*.edges.csv.gz, the cross, pass and
presentation files of data/access_phases and data/access_presentation, and the reads files) in which a subsystem other than
pathfinding is the writer or the reader of such an item. A writer or reader `-` is no subsystem.

Reads. `<scenario>.reads.csv` (item,reader,count,first_tick, written by lib/accesslog) lists every subsystem that read an item
at all in the windows of a tick, a subsystem's reads of its own writes included, which the edges files leave out
(lib/accesslog/classifier.h). The `presentation` window is not in it.

Shadow fields. The harness dumps sections 16 and 17 of docs/trace-format.md (a diagnostic, not hashed since bt-trace 4) from the records
that the default pathfinder writes (PathfindingWriteState): the
tables Pathfinder, PathfinderSearch and Node of data/trace_schema.json, the cells and dirty words of the grid, the open heap and
the three globals. EXPOSED maps each of those fields to the log item that holds it. --check fails when
  * a field of those tables has no entry in EXPOSED (the written state grew a field this check does not know),
  * an item of EXPOSED has no read by pathfinding in any stored scenario: the writer shows state that the simulation never
    reads, a shadow field. data/shadow_field_allow.csv (item,reason) lists the ones that are 3.1c state the trace has always
    hashed, each with a reason; a row for an item that is read is stale.
A field is judged by its item, so the ten cursors count as one item, and a field of a union as the first member of the union.
Standard library plus tools/depclass.py.
"""

import argparse
import csv
import gzip
import io
import json
import re
import sys
from pathlib import Path

import depclass
from sources import ROOT

OWNER = "pathfinding"
READS_DIR = ROOT / "data/access_reads"
READS_SUFFIX = ".reads.csv"
ACCESS_DIR = ROOT / "data/access"
PHASES_DIR = ROOT / "data/access_phases"
PRESENTATION_DIR = ROOT / "data/access_presentation"
SCHEMA = ROOT / "data/trace_schema.json"
ALLOW = ROOT / "data/shadow_field_allow.csv"
ALLOW_COLUMNS = ["item", "reason"]
NONE = "-"
REGION_ROOT = "pathfinder"
REGION_TYPES = ("Pathfinder", "PathExtra", "Node", "PathCell", "PathPending")
REGION_ALLOCATION = re.compile(r"AISearch region")
REGION_FUNCTION = "LoadBattleAssets"

RECORD = {"pool": "Pathfinder::pool", "items": "Pathfinder::items", "freeHead": "Pathfinder::freeHead", "used": "Pathfinder::used",
          "count": "Pathfinder::count", "topPopped": "Pathfinder::topPopped", "cells": "Pathfinder::grid.cells",
          "width": "Pathfinder::grid.width", "height": "Pathfinder::grid.height", "cellCount": "Pathfinder::grid.count",
          "dirty": "Pathfinder::grid.dirty", "stepsPerTick": "Pathfinder::stepsPerTick", "steps": "Pathfinder::steps",
          "costScale": "Pathfinder::costScale", "baseScale": "Pathfinder::baseScale", "object": "Pathfinder::object",
          "player": "Pathfinder::player", "cursor": "Pathfinder::cursor", "budget": "Pathfinder::budget"}
SEARCH = {"start_x": "Pathfinder::start.x", "start_y": "Pathfinder::start.y", "goalX": "Pathfinder::goalX",
          "goalY": "Pathfinder::goalY", "probe": "Pathfinder::probe", "range": "Pathfinder::range", "path": "Pathfinder::path",
          "target": "Pathfinder::target", "owner": "Pathfinder::owner", "turnCost": "Pathfinder::table",
          "stepCost": "Pathfinder::pairs"}
NODE = {"index": "Node::index", "pos_x": "Node::data.pos.x", "pos_y": "Node::data.pos.y", "g": "Node::data.g",
        "f": "Node::data.f", "steps": "Node::data.steps"}
GLOBALS = {"playerTickLoad": "PathExtra::tickLoad", "playerBudgetCap": "PathExtra::budgetCap",
           "budgetRefreshCounter": "PathExtra::refreshCounter"}
HEAP = "AISearch region.heap"
DIRTY = "AISearch region.dirty"
EXTRA = {"Node::data.penalty": "node penalty, hashed with a node whose g is not 0 (harness VisitPathNodes)",
         "PathCell::flags": "cell flags, hashed for the cells of the dirty words (harness VisitPathCells)",
         "PathCell::dir": "cell dir, hashed for an open or closed cell or one with flag 8",
         "PathCell::node": "cell node, hashed for an open or closed cell",
         HEAP: "the open heap's entries, hashed after pathfinder.heap.count",
         DIRTY: "the dirty words, hashed before the cells they cover"}
TABLES = (("Pathfinder", RECORD), ("PathfinderSearch", SEARCH), ("Node", NODE))


def exposed(schema: dict) -> tuple[dict, list]:
    """({item: [where it is hashed]}, [problems]) from the schema's tables and globals and the fixed entries of EXTRA."""
    out: dict[str, list[str]] = {}
    problems = []
    for table, names in TABLES:
        for f in schema["tables"][table]["fields"]:
            name = re.sub(r"\[\d+\]$", "", f["name"])
            item = names.get(name)
            if item is None:
                problems.append("the trace schema field %s.%s has no entry in privatestate.py" % (table, f["name"]))
                continue
            out.setdefault(item, []).append("%s.%s" % (table, f["name"]))
    for g in schema["globals"]["pathfinder"]:
        item = GLOBALS.get(g["name"])
        if item is None:
            problems.append("the trace schema global pathfinder.%s has no entry in privatestate.py" % g["name"])
            continue
        out.setdefault(item, []).append("pathfinder.%s" % g["name"])
    for item, where in EXTRA.items():
        out.setdefault(item, []).append(where)
    return out, problems


def is_private(item: str) -> bool:
    """Whether an item of the log is the region's root or a part of the default pathfinder's allocation."""
    if item == REGION_ROOT:
        return True
    if item.split("::")[0] in REGION_TYPES:
        return True
    return item.startswith(REGION_FUNCTION + ":") and REGION_ALLOCATION.search(item) is not None


def read_gz(path: Path) -> list[dict]:
    return list(csv.DictReader(io.StringIO(gzip.decompress(path.read_bytes()).decode())))


def stored_rows() -> dict[str, list[tuple[str, str, str]]]:
    """{file kind: [(writer, reader, item)]} of every stored summary with an item, `-` for a column the file does not have."""
    out: dict[str, list[tuple[str, str, str]]] = {"edges": [], "cross": [], "pass": [], "presentation": [], "reads": []}
    for kind, directory in (("edges", ACCESS_DIR), ("cross", PHASES_DIR), ("pass", PHASES_DIR), ("reads", READS_DIR)):
        suffix = ".%s.csv.gz" % kind
        for path in sorted(directory.glob("*" + suffix)):
            for r in read_gz(path):
                out[kind].append((r.get("writer", NONE), r.get("reader", NONE), r["item"]))
    for path in sorted(PRESENTATION_DIR.glob("*.csv")):
        with path.open(newline="") as fh:
            out["presentation"].extend((NONE, r["reader"], r["item"]) for r in csv.DictReader(fh))
    return out


def violations(rows: dict[str, list[tuple[str, str, str]]]) -> list[str]:
    """One message for each (file kind, writer, reader, item) of the rows in which a subsystem but pathfinding writes or reads private state."""
    found = set()
    for kind, triples in rows.items():
        for writer, reader, item in triples:
            if not is_private(item):
                continue
            for role, who in (("writer", writer), ("reader", reader)):
                if who not in (NONE, OWNER):
                    found.add((kind, role, who, item))
    return ["%s: %s %s on %s, which is private to %s" % (k, r, w, i, OWNER) for k, r, w, i in sorted(found)]


def canonical(item: str) -> str:
    """The item of a part of the allocation without the name of the allocating function and allocator."""
    if item.startswith(REGION_FUNCTION + ":"):
        found = REGION_ALLOCATION.search(item)
        if found:
            return item[found.start():]
    return item


def pathfinding_reads(reads: list[dict]) -> set[str]:
    return {canonical(r["item"]) for r in reads if r["reader"] == OWNER}


def shadow_problems(wanted: dict, read: set[str], allow: dict[str, str]) -> list[str]:
    """The shadow fields (an exposed item that pathfinding never reads, with no reason in the allow list) and the stale allow rows."""
    out = []
    for item in sorted(wanted):
        if item not in read and item not in allow:
            out.append("shadow field %s (%s) is hashed by the trace but pathfinding reads it in no scenario" % (item, ", ".join(wanted[item])))
        if item in read and item in allow:
            out.append("%s: stale row for %s, which pathfinding reads" % (ALLOW.name, item))
    for item in sorted(set(allow) - set(wanted)):
        out.append("%s: %s is not an exposed item" % (ALLOW.name, item))
    return out


def read_allow() -> dict[str, str]:
    if not ALLOW.is_file():
        return {}
    with ALLOW.open(newline="") as fh:
        return {r["item"]: r["reason"] for r in csv.DictReader(fh)}


def store(source: Path, directory: Path) -> int:
    """Keep each reads file of source (accesslog.py's output) in directory, sorted and gzipped."""
    import accesslog

    for stats in sorted(source.glob("*.stats.txt")):
        accesslog.check_stats(stats.name[: -len(".stats.txt")], stats)
    directory.mkdir(exist_ok=True)
    count = 0
    for path in sorted(source.glob("*" + READS_SUFFIX)):
        with path.open(newline="") as fh:
            rows = sorted(csv.DictReader(fh), key=lambda r: (r["item"], r["reader"]))
        buf = io.StringIO()
        w = csv.writer(buf, lineterminator="\n")
        w.writerow(["item", "reader", "count", "first_tick"])
        w.writerows([r["item"], r["reader"], r["count"], r["first_tick"]] for r in rows)
        (directory / (path.name + ".gz")).write_bytes(gzip.compress(buf.getvalue().encode(), 9, mtime=0))
        count += 1
    return count


def check() -> list[str]:
    problems = []
    names = {p.name[: -len(READS_SUFFIX + ".gz")] for p in READS_DIR.glob("*" + READS_SUFFIX + ".gz")}
    for s in depclass.missing_scenarios({n: [] for n in names}):
        problems.append("no summary data/access_reads/%s.reads.csv.gz for reference/scenarios/%s.txt" % (s, s))
    problems += violations(stored_rows())
    schema = json.loads(SCHEMA.read_text())
    wanted, bad = exposed(schema)
    problems += bad
    read = set()
    for path in sorted(READS_DIR.glob("*" + READS_SUFFIX + ".gz")):
        read |= pathfinding_reads(read_gz(path))
    problems += shadow_problems(wanted, read, read_allow())
    return problems


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--store", type=Path, metavar="DIR", help="store the reads files of DIR in data/access_reads")
    mode.add_argument("--check", action="store_true", help="fail on another subsystem's access to the region or on a shadow field")
    args = ap.parse_args()
    if args.store:
        print("privatestate: stored %d reads files" % store(args.store, READS_DIR))
        return 0
    problems = check()
    if problems:
        print("\n".join(problems))
        return 1
    print("privatestate: the region is private to %s in the stored summaries and every field the trace hashes is read" % OWNER)
    return 0


if __name__ == "__main__":
    sys.exit(main())
