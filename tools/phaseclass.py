"""Classify what the simulation does across the phases of a tick, in its random draws and within its passes, from the
stored access-log summaries.

    uv run tools/accesslog.py run --build-dir build/access <every scenario>
    uv run tools/phaseclass.py --store build/access/access    # keeps the four phase files of each scenario, gzipped
    uv run tools/phaseclass.py --write       # writes data/phase_dependencies.csv, phase_boundaries.csv, draws.csv, pass_reads.csv, presentation_feeds.csv
    uv run tools/phaseclass.py --check       # fail if one of them differs from what the stored files regenerate, or if a
                                             # scenario lacks a file, or if a phase boundary or pass has no row
    uv run tools/test_phaseclass.py          # tests, on synthetic files

This is step 4 of #830 (#838). Its input is data/access_phases/<scenario>.<kind>.csv.gz, written by
tools/accesslog.py (#835) next to the edges file that tools/depclass.py (#836) reads, with the rows of the
`presentation` window dropped or, for a row that touches it, its count dropped (the frames drawn between two ticks
follow the wall clock), gzipped with a zero mtime and sorted. A cross row whose reader is `presentation` is dropped
by --store whatever its phases: whether and how often presentation reads follows the wall clock, and
data/presentation_reads.csv (#839) records those reads, by presence per run. So two --store runs of the same build
give the same bytes. The kinds:

  cross    writer,reader,item,phase_from,phase_to,evidence,count,first_tick. A read in phase_to of an item last written
           in phase_from by another subsystem. `read-after-write` has phase_from before phase_to in the same tick;
           `read-before-write` has the writer in a later phase of the tick before (the read saw the old value because the
           write comes after it in the tick).
  draws    pass,stream,subsystem,function,entities,draws_min,draws_max,ticks,first_tick,tainted,items. Per pass and
           stream and the function that called the generator: how many draws one entity made in one tick.
  pass     pass,item,reader,function,evidence,count,first_tick. A later entity of a pass read an item that an earlier
           entity of the same pass wrote in it.
  ordered  writer,reader,item,phase,evidence,count,first_tick. The `sim-event` stream of the sim event queue, in the format
           of the edges files; tools/depclass.py reads it with them and classes its rows `ordered`.

data/phase_dependencies.csv has the columns writer,reader,item,phase_from,phase_to,class,runs,example,flags, sorted. A
row is an edge between two different simulation subsystems (coupling.SIM_SUBSYSTEMS, as tools/depclass.py counts
them) across phases. A row whose reader is `presentation` is left out of it and of data/phase_boundaries.csv, in
--write and in --check alike, because the stored files no longer hold it and data/presentation_reads.csv covers it. A row
whose writer is `presentation` and whose reader is a simulation subsystem stays in it, and --check fails on each such
row is a presentation feed: presentation must not feed the simulation except through the player view, so each feed
is listed with its reason in data/presentation_feeds.csv (below) until the player view carries it. A row with any
other writer outside the simulation subsystems is dropped. Items `unresolved:*` are dropped. The class is
`immediate` when a run shows a read in phase_to of what phase_from wrote earlier in the tick (the reader needs the
writer's phase to come first), and `carried` when a run shows a read in phase_to of what phase_from, a later phase,
wrote in the tick before (the reader needs the writer's phase to come after it; the order of a tick's phases is
the state). A key with both in different runs has both rows. `runs`, `example` and `flags` are as in
data/dependencies.csv; `dynamic-only` is set from data/coupling.csv in the same way.

data/presentation_feeds.csv has the columns writer,reader,item,phase,example,reason, sorted: one row per presentation
feed, that is per writer `presentation`, reader (a simulation subsystem), item and `phase`, which is the phase_to of the
cross rows, the phase in which the simulation read what presentation wrote (phase_from is where the log last saw the
write, which differs between runs of one feed and between its `immediate` and `carried` rows, so phase_to is the stable
part). `example` is the `<scenario>@<tick>` of the smallest tick over those rows. `reason` is written by hand. The
feeds are not rows of data/phase_dependencies.csv and the boundaries do not count them: the feeds file is their
record, and the two files hold the edges between simulation subsystems, and the reads of presentation are left to
data/presentation_reads.csv. --write keeps the reason of each feed that still occurs, adds the feeds that are new with an
empty reason and drops the ones no run shows. --check fails on a feed that is not listed, on a listed one that no run
shows any more (stale) and on a listed one with an empty reason, naming each, and on a file that differs from what --write
gives.

data/phase_boundaries.csv has one row per boundary between two windows of a tick: `0|1` (absent: no scenario of the test
mode runs marker 0, so there is no `net` window), `1|2` to `6|7`, `7|tail` and `tail|presentation`. `needed` is `yes`
when an `immediate` row spans it (phase_from up to the boundary, phase_to after it; for `tail|presentation`, a row
whose phase_to is `presentation`), `edges` counts those rows, `direct` counts those that span no other boundary (phase_from just before it, phase_to just after; for
`tail|presentation`, all of them; no row ends in `presentation` unless its reader is a simulation subsystem), `carried` counts the `carried` rows that span it the
other way (phase_to up to the boundary, phase_from after it), and `mergeable` is `yes` when both are 0: no state
that the runs saw crosses it, so the two windows could be merged or run in the other order without a read seeing a
different value. `example` is the `example` of the spanning `immediate` row with the most runs. If the contract were
the state at the end of the tick, the boundaries with `needed` `no` are the ones nothing observed would miss.

data/draws.csv has the columns pass,stream,subsystem,function,entities,draws_min,draws_max,ticks,start_state,evidence,example,
one row per pass and stream and calling function. `entities` is the most entities that drew in one tick, `draws_min` and
`draws_max` the least and most draws of one entity in a tick (an entity that drew), `ticks` the ticks that had a draw, summed
over the scenarios. `start_state` is advisory: `fixed` when every entity made the same number of draws every tick;
`start-state` when the number varies but no entity read, before its last draw, a field that an earlier entity of the
pass had written; `depends-on-earlier` otherwise, with those fields (up to four a scenario, the most common first) in
`evidence`. The reads of the generators themselves are not counted, and whether the number follows from the values
drawn earlier is not seen at all.

data/pass_reads.csv has the columns pass,item,reader,function,evidence,runs,example: the items that an entity read after an
earlier entity of the pass wrote them, by the function that read. `evidence` is `same-record` when the earlier write
was to the record (the allocation and element) that the read hit, else `other-record` (the item is the same field of
another record: the log keeps one last writer per item, so this is the weaker evidence). A pass with no such read has
one row with item `-` and evidence `none`.
"""

import argparse
import csv
import difflib
import gzip
import io
import sys
from collections import defaultdict
from pathlib import Path

import depclass
import sources

ROOT = sources.ROOT
PHASES_DIR = ROOT / "data/access_phases"
PHASE_DEPENDENCIES = ROOT / "data/phase_dependencies.csv"
PHASE_BOUNDARIES = ROOT / "data/phase_boundaries.csv"
DRAWS = ROOT / "data/draws.csv"
PASS_READS = ROOT / "data/pass_reads.csv"
PRESENTATION_FEEDS = ROOT / "data/presentation_feeds.csv"
KINDS = ("cross", "draws", "pass", "ordered")
PHASE_NAMES = ["net", "units", "projectiles", "explosions", "players", "features", "wind", "meteors", "tail", "presentation"]
PHASE_INDEX = {name: i for i, name in enumerate(PHASE_NAMES)}
PRESENTATION = "presentation"
PASSES = ["units", "projectiles", "explosions", "players", "features", "wind", "meteors", "tail"]
SORT_COLUMNS = {"cross": ("writer", "reader", "item", "phase_from", "phase_to", "evidence"),
                "draws": ("pass", "stream", "subsystem", "function"),
                "pass": ("pass", "item", "reader", "function", "evidence"),
                "ordered": ("writer", "reader", "item", "phase", "evidence")}
IMMEDIATE = "immediate"
CARRIED = "carried"
EVIDENCE_CLASS = {"read-after-write": IMMEDIATE, "read-before-write": CARRIED}
PHASE_DEPENDENCIES_COLUMNS = ["writer", "reader", "item", "phase_from", "phase_to", "class", "runs", "example", "flags"]
BOUNDARY_COLUMNS = ["boundary", "needed", "edges", "direct", "carried", "mergeable", "example"]
DRAWS_COLUMNS = ["pass", "stream", "subsystem", "function", "entities", "draws_min", "draws_max", "ticks", "start_state",
                 "evidence", "example"]
PASS_READS_COLUMNS = ["pass", "item", "reader", "function", "evidence", "runs", "example"]
FEEDS_COLUMNS = ["writer", "reader", "item", "phase", "example", "reason"]
CROSS_COLUMNS = ["writer", "reader", "item", "phase_from", "phase_to", "evidence", "count", "first_tick"]
EVIDENCE_ITEMS = 4


def read_rows(text: str) -> list[dict]:
    """The rows of one phase file."""
    return list(csv.DictReader(io.StringIO(text)))


def render_rows(columns: list[str], rows: list[dict]) -> str:
    """The text of a CSV file with the given columns."""
    buf = io.StringIO()
    w = csv.DictWriter(buf, columns, lineterminator="\n", quoting=csv.QUOTE_MINIMAL)
    w.writeheader()
    w.writerows(rows)
    return buf.getvalue()


def reduce_rows(kind: str, rows: list[dict]) -> list[dict]:
    """The rows of a phase file as stored: without what follows the wall clock, sorted."""
    keep = []
    for r in rows:
        if kind == "cross" and r["reader"] == PRESENTATION:
            continue
        if kind == "cross" and PRESENTATION in (r["phase_from"], r["phase_to"]):
            r = {**r, "count": "0"}
        elif kind == "draws" and r["pass"] == PRESENTATION:
            continue
        elif kind == "ordered" and r["phase"] == PRESENTATION:
            continue
        keep.append(r)
    return sorted(keep, key=lambda r: tuple(r[c] for c in SORT_COLUMNS[kind]))


def store(source: Path, directory: Path) -> int:
    """Keep each phase file of source (accesslog.py's output) in directory, reduced and gzipped; the number of scenarios."""
    import accesslog

    for stats in sorted(source.glob("*.stats.txt")):
        accesslog.check_stats(stats.name[: -len(".stats.txt")], stats)
    directory.mkdir(exist_ok=True)
    names = sorted(p.name[: -len(".edges.csv")] for p in source.glob("*.edges.csv"))
    for name in names:
        for kind in KINDS:
            path = source / ("%s.%s.csv" % (name, kind))
            rows = read_rows(path.read_text())
            header = path.read_text().splitlines()[0].split(",")
            text = render_rows(header, reduce_rows(kind, rows))
            (directory / ("%s.%s.csv.gz" % (name, kind))).write_bytes(gzip.compress(text.encode(), 9, mtime=0))
    return len(names)


def load_kind(directory: Path, kind: str) -> dict[str, list[dict]]:
    """Scenario name to the rows of its stored file of the kind."""
    suffix = ".%s.csv.gz" % kind
    return {p.name[: -len(suffix)]: read_rows(gzip.decompress(p.read_bytes()).decode())
            for p in sorted(directory.glob("*" + suffix))}


def hit(table: dict, key: tuple, scenario: str, tick: int) -> None:
    """Note that scenario shows key, first at tick."""
    runs = table[key]
    runs[scenario] = min(tick, runs.get(scenario, tick))


def best_example(runs: dict[str, int]) -> str:
    """`<scenario>@<tick>`: the run with the smallest tick, the alphabetically first on a tie."""
    scenario, tick = min(runs.items(), key=lambda kv: (kv[1], kv[0]))
    return "%s@%d" % (scenario, tick)


def phase_dependencies(cross: dict[str, list[dict]], coupling_rows: list[dict]) -> list[dict]:
    """The rows of data/phase_dependencies.csv."""
    sim = depclass.SIM
    hits: dict[tuple, dict[str, int]] = defaultdict(dict)
    for scenario, rows in cross.items():
        for r in rows:
            writer, reader, item = r["writer"], r["reader"], r["item"]
            cls = EVIDENCE_CLASS[r["evidence"]]
            if writer == reader or item.startswith("unresolved:") or reader not in sim:
                continue
            if writer not in sim and writer != PRESENTATION:
                continue
            hit(hits, (writer, reader, item, r["phase_from"], r["phase_to"], cls), scenario, int(r["first_tick"]))
    pairs = {(c["from"], c["to"]) for c in coupling_rows}
    out = []
    for key, runs in hits.items():
        writer, reader = key[0], key[1]
        flags = depclass.DYNAMIC_ONLY if (writer, reader) not in pairs else ""
        out.append({"writer": writer, "reader": reader, "item": key[2], "phase_from": key[3], "phase_to": key[4],
                    "class": key[5], "runs": len(runs), "example": best_example(runs), "flags": flags})
    return sorted(out, key=lambda r: (r["writer"], r["reader"], r["item"], r["phase_from"], r["phase_to"], r["class"]))


def feed_key(row: dict) -> tuple:
    """The identity of a presentation feed."""
    return (row["writer"], row["reader"], row["item"], row["phase"])


def example_tick(example: str) -> tuple:
    """The sort key of a `<scenario>@<tick>` example: tick, then scenario."""
    scenario, tick = example.rsplit("@", 1)
    return (int(tick), scenario)


def feed_rows(deps: list[dict], listed: list[dict]) -> list[dict]:
    """The rows of data/presentation_feeds.csv for the rows of phase_dependencies() and the listed feeds: the reasons of the listed ones are kept."""
    reasons = {feed_key(r): r["reason"] for r in listed}
    groups: dict[tuple, list[str]] = defaultdict(list)
    for r in deps:
        if r["writer"] == PRESENTATION:
            groups[(r["writer"], r["reader"], r["item"], r["phase_to"])].append(r["example"])
    return [dict(zip(FEEDS_COLUMNS, key + (min(examples, key=example_tick), reasons.get(key, ""))))
            for key, examples in sorted(groups.items())]


def feed_problems(feeds: list[dict], listed: list[dict]) -> list[str]:
    """What is wrong with the listed feeds: the feeds in feeds that are not listed, the listed ones that are stale, and the listed ones without a reason."""
    have = {feed_key(r) for r in listed}
    seen = {feed_key(r) for r in feeds}
    problems = []
    for r in feeds:
        if feed_key(r) not in have:
            problems.append("presentation feeds the simulation and the feed is not listed in data/presentation_feeds.csv: %s -> %s, item %s, read in %s (%s)" %
                            (r["writer"], r["reader"], r["item"], r["phase"], r["example"]))
    for r in listed:
        if feed_key(r) not in seen:
            problems.append("data/presentation_feeds.csv lists a feed that no run shows any more (stale): %s -> %s, item %s, read in %s" %
                            (r["writer"], r["reader"], r["item"], r["phase"]))
        elif not r["reason"].strip():
            problems.append("data/presentation_feeds.csv lists a feed with an empty reason: %s -> %s, item %s, read in %s" %
                            (r["writer"], r["reader"], r["item"], r["phase"]))
    return problems


def boundary_names() -> list[tuple[str, int, int]]:
    """The boundary labels with the indexes of the windows they separate, in tick order."""
    labels = lambda i: str(i) if i < PHASE_INDEX["tail"] else PHASE_NAMES[i]
    return [("%s|%s" % (labels(i), labels(i + 1)), i, i + 1) for i in range(len(PHASE_NAMES) - 1)]


def phase_boundaries(rows: list[dict]) -> list[dict]:
    """The rows of data/phase_boundaries.csv for the rows of data/phase_dependencies.csv."""
    out = []
    for label, before, after in boundary_names():
        if before == 0:
            out.append({"boundary": label, "needed": "absent", "edges": 0, "direct": 0, "carried": 0, "mergeable": "absent", "example": ""})
            continue
        if after == PHASE_INDEX[PRESENTATION]:
            spanning = [r for r in rows if r["class"] == IMMEDIATE and r["phase_to"] == PRESENTATION]
            back: list[dict] = []
        else:
            spanning = [r for r in rows if r["class"] == IMMEDIATE and r["phase_to"] != PRESENTATION
                        and PHASE_INDEX[r["phase_from"]] <= before < PHASE_INDEX[r["phase_to"]]]
            back = [r for r in rows if r["class"] == CARRIED and r["phase_to"] != PRESENTATION
                    and PHASE_INDEX[r["phase_to"]] <= before < PHASE_INDEX[r["phase_from"]]]
        example = ""
        if spanning:
            top = min(spanning, key=lambda r: (-r["runs"], r["writer"], r["reader"], r["item"], r["phase_from"], r["phase_to"]))
            example = top["example"]
        direct = [r for r in spanning if PHASE_INDEX[r["phase_to"]] == after and (PHASE_INDEX[r["phase_from"]] == before or after == PHASE_INDEX[PRESENTATION])]
        out.append({"boundary": label, "needed": "yes" if spanning else "no", "edges": len(spanning), "direct": len(direct), "carried": len(back),
                    "mergeable": "yes" if not spanning and not back else "no", "example": example})
    return out


def start_state(low: int, high: int, tainted: bool) -> str:
    """fixed, start-state or depends-on-earlier, from the least and most draws of an entity and whether any entity read a field an earlier one wrote."""
    if low == high:
        return "fixed"
    return "depends-on-earlier" if tainted else "start-state"


def draws(draws_by_scenario: dict[str, list[dict]]) -> list[dict]:
    """The rows of data/draws.csv."""
    groups: dict[tuple, list[tuple[str, dict]]] = defaultdict(list)
    for scenario, rows in draws_by_scenario.items():
        for r in rows:
            groups[(r["pass"], r["stream"], r["subsystem"], r["function"])].append((scenario, r))
    out = []
    for key, members in groups.items():
        low = min(int(r["draws_min"]) for _, r in members)
        high = max(int(r["draws_max"]) for _, r in members)
        tainted = any(r["tainted"] == "1" for _, r in members)
        counts: dict[str, int] = defaultdict(int)
        for _, r in members:
            if r["tainted"] == "1":
                for item in filter(None, r["items"].split(";")):
                    counts[item] += 1
        state = start_state(low, high, tainted)
        evidence = ";".join(sorted(counts, key=lambda i: (-counts[i], i))[:EVIDENCE_ITEMS]) if state == "depends-on-earlier" else ""
        runs = {s: int(r["first_tick"]) for s, r in members}
        out.append({"pass": key[0], "stream": key[1], "subsystem": key[2], "function": key[3],
                    "entities": max(int(r["entities"]) for _, r in members), "draws_min": low, "draws_max": high,
                    "ticks": sum(int(r["ticks"]) for _, r in members), "start_state": state, "evidence": evidence,
                    "example": best_example(runs)})
    return sorted(out, key=lambda r: (r["pass"], r["stream"], r["subsystem"], r["function"]))


def pass_reads(pass_by_scenario: dict[str, list[dict]]) -> list[dict]:
    """The rows of data/pass_reads.csv."""
    hits: dict[tuple, dict[str, int]] = defaultdict(dict)
    for scenario, rows in pass_by_scenario.items():
        for r in rows:
            if r["item"].startswith("unresolved:"):
                continue
            hit(hits, (r["pass"], r["item"], r["reader"], r["function"], r["evidence"]), scenario, int(r["first_tick"]))
    out = [{"pass": k[0], "item": k[1], "reader": k[2], "function": k[3], "evidence": k[4], "runs": len(runs),
            "example": best_example(runs)} for k, runs in hits.items()]
    seen = {r["pass"] for r in out}
    for name in PASSES:
        if name not in seen:
            out.append({"pass": name, "item": "-", "reader": "-", "function": "-", "evidence": "none", "runs": 0, "example": ""})
    return sorted(out, key=lambda r: (r["pass"], r["item"], r["reader"], r["function"], r["evidence"]))


def problems_of(directory: Path, tables: dict[str, list[dict]]) -> list[str]:
    """What is missing from the stored files and the rows: scenarios without a file, windows or passes without a row."""
    problems = []
    names = [p.stem for p in depclass.SCENARIOS.glob("*.txt")]
    for kind in KINDS:
        have = {p.name[: -len(".%s.csv.gz" % kind)] for p in directory.glob("*.%s.csv.gz" % kind)}
        for name in sorted(set(names) - have):
            problems.append("no summary data/access_phases/%s.%s.csv.gz for reference/scenarios/%s.txt" % (name, kind, name))
    for r in tables["phase_dependencies"]:
        for phase in (r["phase_from"], r["phase_to"]):
            if phase not in PHASE_INDEX:
                problems.append("phase_dependencies row with an unknown phase %s" % phase)
    expect = [label for label, _, _ in boundary_names()]
    if [r["boundary"] for r in tables["phase_boundaries"]] != expect:
        problems.append("phase_boundaries has no row for each of %s" % ", ".join(expect))
    seen = {r["pass"] for r in tables["pass_reads"]}
    for name in PASSES:
        if name not in seen:
            problems.append("pass_reads has no row for the pass %s" % name)
    return problems


def build(directory: Path, coupling_rows: list[dict], listed: list[dict] | None = None) -> dict[str, list[dict]]:
    """All five tables from the stored files of directory and the listed feeds; `feeds_listed` is the listed feeds, not a table."""
    listed = listed or []
    every = phase_dependencies(load_kind(directory, "cross"), coupling_rows)
    deps = [r for r in every if r["writer"] != PRESENTATION]
    return {"phase_dependencies": deps, "phase_boundaries": phase_boundaries(deps), "presentation_feeds": feed_rows(every, listed),
            "feeds_listed": listed, "draws": draws(load_kind(directory, "draws")),
            "pass_reads": pass_reads(load_kind(directory, "pass"))}


def check(directory: Path, tables: dict[str, list[dict]], outputs: dict[str, tuple[Path, list[str]]] | None = None) -> list[str]:
    """The problems with the stored files and the five data files: what problems_of finds, the feeds that are unlisted, stale or without a reason, and each data file that differs from its rows."""
    problems = problems_of(directory, tables)
    if "feeds_listed" in tables:
        problems += feed_problems(tables["presentation_feeds"], tables["feeds_listed"])
    for name, (path, columns) in (outputs or OUTPUTS).items():
        text = render_rows(columns, tables[name])
        have = path.read_text() if path.is_file() else ""
        if have != text:
            rel = path.relative_to(ROOT) if path.is_relative_to(ROOT) else path
            diff = difflib.unified_diff(have.splitlines(), text.splitlines(), str(rel), "regenerated", lineterm="", n=0)
            problems.append("%s differs from the stored files (uv run tools/phaseclass.py --write):\n%s" % (rel, "\n".join(list(diff)[:40])))
    return problems


def stored_tables() -> dict[str, list[dict]]:
    """build() on the stored files of data/access_phases, the coupling rows of data/coupling.csv and data/presentation_feeds.csv."""
    listed = read_rows(PRESENTATION_FEEDS.read_text()) if PRESENTATION_FEEDS.is_file() else []
    return build(PHASES_DIR, depclass.read_coupling(depclass.COUPLING_CSV.read_text()), listed)


def check_stored() -> list[str]:
    """check() on stored_tables()."""
    return check(PHASES_DIR, stored_tables())


OUTPUTS = {"phase_dependencies": (PHASE_DEPENDENCIES, PHASE_DEPENDENCIES_COLUMNS), "phase_boundaries": (PHASE_BOUNDARIES, BOUNDARY_COLUMNS),
           "draws": (DRAWS, DRAWS_COLUMNS), "pass_reads": (PASS_READS, PASS_READS_COLUMNS),
           "presentation_feeds": (PRESENTATION_FEEDS, FEEDS_COLUMNS)}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--store", type=Path, metavar="DIR", help="store the phase files of DIR in data/access_phases")
    mode.add_argument("--write", action="store_true", help="write the five data files")
    mode.add_argument("--check", action="store_true", help="fail if a data file is stale or a boundary or pass has no row")
    args = ap.parse_args()
    if args.store:
        print("phaseclass: stored %d scenarios" % store(args.store, PHASES_DIR))
        return 0
    tables = stored_tables()
    if args.write:
        for name, (path, columns) in OUTPUTS.items():
            path.write_text(render_rows(columns, tables[name]))
        print("phaseclass: " + ", ".join("%d %s" % (len(tables[n]), n) for n in OUTPUTS))
        return 0
    problems = check(PHASES_DIR, tables)
    if problems:
        print("\n".join(problems))
        return 1
    print("phaseclass: " + ", ".join("%d %s" % (len(tables[n]), n) for n in OUTPUTS) + " match the stored files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
