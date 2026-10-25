"""Report how much of each subsystem the scenario suite runs: function, line and branch coverage.

    uv run tools/coverage.py --exe build/coverage/TotalA.exe --game-dir ~/.ta   # play the suite, print the table
    uv run tools/coverage.py --exe E --game-dir D --write                       # also write data/coverage.csv
    uv run tools/coverage.py --exe E --game-dir D --check                       # fail if a subsystem's coverage fell
    uv run tools/coverage.py --exe E --from PROFDATA [--write|--check]          # use a merged profile, play nothing
    uv run tools/coverage.py ... --live-counters FILE                           # default: live_counters.txt beside the exe
    uv run tools/coverage.py --exe E --game-dir D --scenario N... [--parallel N]   # a subset (no --write or --check)
    uv run tools/test_coverage.py                                               # tests, on hand-written exports

The exe is the coverage build (clang's -fprofile-instr-generate -fcoverage-mapping, #385). Every scenario
of reference/scenarios/ is played under Wine through tools/reference.py with LLVM_PROFILE_FILE naming
build/coverage-run/<scenario>/cov-%p.profraw, so parallel runs never share a file. A scenario that fails
or writes no profile exits 1: the coverage of a partial suite is not a baseline. The profiles are merged
(llvm-profdata merge -sparse) into build/coverage-run/merged.profdata, whose path is printed so that
`llvm-cov show <exe> -instr-profile=<it>` can show the source, and `llvm-cov export -format=text` gives
the mapping and counts that this tool reads.

Assignment. data/subsystems.csv keys a function by (name, file). A function of the export gets a plain
name (tools/blockpointers.py demangle, and `X::X`, `X::~X`, `X::operator<code>` for the decorated
constructors, destructors and operators it leaves alone). It is matched by that name and its primary file,
relative to the repository; when no row has that file, by name alone if every row of that name is in one
subsystem. A function still without a row is `state` when its file is under interfaces/ or src/state/ (the
shared state block, #7: Ref<T>, SimAllocator, the block heap), else the subsystem coupling.rule_of gives
its name and file (the rule that built subsystems.csv, without phase reachability). `unassigned` (counted
and printed) is left for a function that nothing resolves. Functions in src/harness/, build/, toolchain/ or outside the repository are
excluded (counted and printed).

Counting, per function and then summed per subsystem. Copies of one source function (a header function
`static` in every translation unit that includes it, named `<tu>.cpp:<decorated>`) are merged first, keyed by
(file, decorated name): entered if any copy is, a line hit if any copy hit it, a branch side covered if any
copy covered it, branches counted once per position. Functions: entered when the execution count is
above 0. Lines: the lines of the function's code regions in its primary file; a line's count is the
largest of the innermost covering region's count and the counts of the regions that start on it, as
llvm-cov show counts it; the line is hit when that count is above 0. Branches: each branch condition has a true and a false side, both
counted in the total, and each side taken at least once is covered.

Dead functions. 3.1c's code cannot reach a function when no code the link keeps contains it, so it is left
out of the totals and listed apart. The coverage link keeps every function (the profile data names each one),
so tools/aliases.py walks the sections again without the profile data as a root and writes the profile counters
the kept code increments to live_counters.txt beside the exe (--live-counters). A function is live when it was
entered or its counter (`___profc_<decorated name>`, `file.cpp_` before the name of a static) is in that file:
the counter of a function inlined into a kept function is there, though the function has no symbol, and the
counter of a function in a section nothing reaches is not. A function that is neither entered nor live is
dead. Of the copies of a source function, it is dead when all are.

data/coverage.csv: columns subsystem, function, file, functions, functions_entered, lines, lines_hit,
branches, branches_covered, decorated, dead. One totals row per subsystem (function `*`, file and decorated
empty, dead the number of dead functions, every other count over live functions only), one row per live
function no scenario enters (functions 1, every other count 0, decorated the export name that tells overloads
apart, dead 0) and one row per dead function (every count 0, dead 1), sorted by
subsystem, totals first, then by name, file and decorated name. Percentages are derived. `--check` fails when a subsystem's function, line or
branch fraction is below the file's, or the file is missing; rises and new functions only print a note.
"""

import argparse
import csv
import json
import os
import re
import subprocess
import sys
import tempfile
import threading
from dataclasses import dataclass, field
from pathlib import Path

import blockpointers
import coupling
import reference
import tracediff

ROOT = Path(__file__).resolve().parent.parent
SUBSYSTEMS = ROOT / "data/subsystems.csv"
OUT = ROOT / "data/coverage.csv"
RUN_DIR = ROOT / "build/coverage-run"
DEFAULT_EXE = ROOT / "build/coverage/TotalA.exe"
LIVE_COUNTERS = "live_counters.txt"
UNASSIGNED = "unassigned"
STATE_PREFIXES = ("interfaces/", "src/state/")
STATE = "state"
EXCLUDED_PREFIXES = ("src/harness/", "build/", "toolchain/")
COLUMNS = ["subsystem", "function", "file", "functions", "functions_entered", "lines", "lines_hit",
           "branches", "branches_covered", "decorated", "dead"]
METRICS = (("functions", "functions", "functions_entered"), ("lines", "lines", "lines_hit"),
           ("branches", "branches", "branches_covered"))
CODE_REGION = 0
REGION_FILE_ID = 5
REGION_KIND = 7
BRANCH_FILE_ID = 6


@dataclass
class Function:
    """One source function: its place, whether it was entered or is dead, and its line and branch counts."""
    name: str
    file: str
    entered: bool
    lines: int
    lines_hit: int
    branches: int
    branches_covered: int
    dead: bool = False


@dataclass
class Copy:
    """One export entry: a function's decorated name and place, its entry, per-line counts and per-position branch sides."""
    decorated: str
    name: str
    file: str
    entered: bool
    line_counts: dict[int, int]
    branch_sides: dict[tuple[int, int, int, int], tuple[bool, bool]]
    counter: str = ""


@dataclass
class Totals:
    """The sums of a subsystem's functions."""
    functions: int = 0
    functions_entered: int = 0
    lines: int = 0
    lines_hit: int = 0
    branches: int = 0
    branches_covered: int = 0
    dead: int = 0
    never_entered: list[tuple[str, str, str]] = field(default_factory=list)
    dead_functions: list[tuple[str, str, str]] = field(default_factory=list)

    def add(self, fn: Function) -> None:
        """Add one function to the sums."""
        self.functions += 1
        self.functions_entered += fn.entered
        self.lines += fn.lines
        self.lines_hit += fn.lines_hit
        self.branches += fn.branches
        self.branches_covered += fn.branches_covered


def percent(hit: int, total: int) -> str:
    """The fraction as `87.5%`, or `-` when there is nothing to cover."""
    return "%.1f%%" % (100 * hit / total) if total else "-"


def repo_path(filename: str) -> str | None:
    """The path relative to the repository, or None when the file is outside it or is not game code."""
    text = filename.replace("\\", "/")
    if text[1:3] == ":/":
        text = text[2:]
    try:
        rel = Path(text).resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return None
    return None if rel.startswith(EXCLUDED_PREFIXES) else rel


def decorated_name(name: str) -> str:
    """The export name without the `file.cpp:` prefix llvm-cov gives static functions."""
    if not name.startswith("?") and ":" in name:
        return name.split(":", 1)[1]
    return name


def plain_name(name: str) -> str:
    """The function's name without MSVC decoration or the `file.cpp:` prefix llvm-cov gives static functions."""
    name = decorated_name(name)
    special = re.match(r"\?\?(_?[0-9A-Z])(?:\?\$([^@]+)@|([^@?]+)@)", name)
    if special:
        code, owner = special.group(1), special.group(2) or special.group(3)
        member = owner if code == "0" else "~" + owner if code == "1" else "operator?" + code
        return "%s::%s" % (owner, member)
    return blockpointers.demangle(name)


def counter_name(name: str) -> str:
    """The profile counter name of an export function name: `file.cpp:f` is `file.cpp_f`."""
    if not name.startswith("?") and ":" in name:
        return name.replace(":", "_", 1)
    return name


def read_live_counters(path: Path) -> set[str]:
    """The counter names tools/aliases.py wrote: those the code the link keeps increments."""
    return set(path.read_text().split())


def region_lines(regions: list[list[int]], primary: set[int]) -> dict[int, int]:
    """Line -> count over the code regions in the primary file: the larger of the innermost covering region's count and the largest count of the regions starting on the line."""
    starts: dict[int, int] = {}
    wrapped: dict[int, tuple[tuple[int, int], int]] = {}
    for r in regions:
        if r[REGION_KIND] != CODE_REGION or r[REGION_FILE_ID] not in primary:
            continue
        first, col, last, end_col, count = r[:5]
        if end_col == 1 and last > first:
            last -= 1
        starts[first] = max(starts.get(first, 0), count)
        for line in range(first + 1, last + 1):
            if (first, col) >= wrapped.get(line, ((0, 0), 0))[0]:
                wrapped[line] = ((first, col), count)
    counts = {line: count for line, (_, count) in wrapped.items()}
    for line, count in starts.items():
        counts[line] = max(counts.get(line, 0), count)
    return counts


def parse_function(entry: dict) -> Copy | None:
    """The Copy of an export entry, or None when it is not game code (the file is outside the repository, in the harness or build)."""
    names = entry.get("filenames") or []
    if not names:
        return None
    rel = repo_path(names[0])
    if rel is None:
        return None
    primary = {i for i, n in enumerate(names) if n == names[0]}
    sides = {tuple(b[:4]): (b[4] > 0, b[5] > 0) for b in entry.get("branches", []) if b[BRANCH_FILE_ID] in primary}
    return Copy(decorated_name(entry["name"]), plain_name(entry["name"]), rel, entry.get("count", 0) > 0,
                region_lines(entry.get("regions", []), primary), sides, counter_name(entry["name"]))


def merge_copies(copies: list[Copy], live: set[str] | None = None) -> Function:
    """One Function from the copies of a source function: entered if any copy is, per line the largest count, per branch position either copy's covered sides. With live (the counters code the link keeps increments) it is dead when no copy is entered or live."""
    lines: dict[int, int] = {}
    sides: dict[tuple[int, int, int, int], tuple[bool, bool]] = {}
    for c in copies:
        for line, count in c.line_counts.items():
            lines[line] = max(lines.get(line, 0), count)
        for pos, (taken, other) in c.branch_sides.items():
            old = sides.get(pos, (False, False))
            sides[pos] = (old[0] or taken, old[1] or other)
    first = copies[0]
    dead = live is not None and not any(c.entered or c.counter in live for c in copies)
    return Function(first.name, first.file, any(c.entered for c in copies), len(lines),
                    sum(1 for n in lines.values() if n > 0), 2 * len(sides),
                    sum(a + b for a, b in sides.values()), dead)


def read_subsystems(path: Path = SUBSYSTEMS) -> tuple[dict[tuple[str, str], str], dict[str, set[str]]]:
    """((file, function) -> subsystem, function -> its subsystems) from data/subsystems.csv."""
    by_key: dict[tuple[str, str], str] = {}
    by_name: dict[str, set[str]] = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            by_key[(row["file"], row["function"])] = row["subsystem"]
            by_name.setdefault(row["function"], set()).add(row["subsystem"])
    return by_key, by_name


def subsystem_of(fn: Function, by_key: dict[tuple[str, str], str], by_name: dict[str, set[str]]) -> str:
    """The subsystem of the (file, name) row, else of the name when all its rows agree, else `state` under interfaces/ or src/state/, else coupling.rule_of's, else `unassigned`."""
    found = by_key.get((fn.file, fn.name))
    if found:
        return found
    owners = by_name.get(fn.name, set())
    if len(owners) == 1:
        return next(iter(owners))
    if fn.file.startswith(STATE_PREFIXES):
        return STATE
    return coupling.rule_of(fn.name, fn.file, {})[0] or UNASSIGNED


def summarize(export: dict, by_key: dict[tuple[str, str], str], by_name: dict[str, set[str]],
              live: set[str] | None = None) -> tuple[dict[str, Totals], int, int]:
    """(subsystem -> Totals, entries excluded, functions unassigned) of an llvm-cov export, counting each (file, decorated name) once. Dead functions (see merge_copies) are listed and counted apart, and left out of the sums."""
    groups: dict[tuple[str, str], list[Copy]] = {}
    missed: dict[str, list[tuple[str, str, str]]] = {}
    dead: dict[str, list[tuple[str, str, str]]] = {}
    totals: dict[str, Totals] = {}
    excluded = unassigned = 0
    for data in export.get("data", []):
        for entry in data.get("functions", []):
            copy = parse_function(entry)
            if copy is None:
                excluded += 1
                continue
            groups.setdefault((copy.file, copy.decorated), []).append(copy)
    for (_, decorated), copies in groups.items():
        fn = merge_copies(copies, live)
        subsystem = subsystem_of(fn, by_key, by_name)
        unassigned += subsystem == UNASSIGNED
        t = totals.setdefault(subsystem, Totals())
        if fn.dead:
            t.dead += 1
            dead.setdefault(subsystem, []).append((fn.name, fn.file, decorated))
            continue
        t.add(fn)
        if not fn.entered:
            missed.setdefault(subsystem, []).append((fn.name, fn.file, decorated))
    for subsystem, rows in missed.items():
        totals[subsystem].never_entered = sorted(rows)
    for subsystem, rows in dead.items():
        totals[subsystem].dead_functions = sorted(rows)
    return totals, excluded, unassigned


def csv_rows(totals: dict[str, Totals]) -> list[list[str]]:
    """The rows of data/coverage.csv: per subsystem a totals row, then a row per never-entered live function and per dead function."""
    rows = []
    for subsystem in sorted(totals):
        t = totals[subsystem]
        first = len(rows)
        rows.append([subsystem, "*", "", t.functions, t.functions_entered, t.lines, t.lines_hit,
                     t.branches, t.branches_covered, "", t.dead])
        rows += [[subsystem, name, file, 1, 0, 0, 0, 0, 0, decorated, 0] for name, file, decorated in t.never_entered]
        rows += [[subsystem, name, file, 0, 0, 0, 0, 0, 0, decorated, 1] for name, file, decorated in t.dead_functions]
        rows[first:] = sorted(rows[first:], key=lambda r: (r[1] != "*", r[1], r[2], r[9]))
    return [[str(c) for c in r] for r in rows]


def write_csv(totals: dict[str, Totals], path: Path) -> None:
    """Write the rows of csv_rows to path."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(COLUMNS)
        w.writerows(csv_rows(totals))


def read_csv(path: Path) -> dict[str, Totals]:
    """The totals, never-entered and dead lists that write_csv wrote (a file without the dead column has no dead functions)."""
    totals: dict[str, Totals] = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            t = totals.setdefault(row["subsystem"], Totals())
            if row["function"] == "*":
                for _, attr, hit in METRICS:
                    setattr(t, attr, int(row[attr]))
                    setattr(t, hit, int(row[hit]))
                t.dead = int(row.get("dead") or 0)
            elif int(row.get("dead") or 0):
                t.dead_functions.append((row["function"], row["file"], row["decorated"]))
            else:
                t.never_entered.append((row["function"], row["file"], row["decorated"]))
    return totals


def compare(want: dict[str, Totals], got: dict[str, Totals]) -> tuple[list[str], list[str]]:
    """(falls, notes): where a subsystem's function, line or branch fraction is below want's, and what rose or is new."""
    falls: list[str] = []
    notes: list[str] = []
    for subsystem in sorted(want):
        old, new = want[subsystem], got.get(subsystem, Totals())
        for label, total, hit in METRICS:
            o = (getattr(old, hit), getattr(old, total))
            n = (getattr(new, hit), getattr(new, total))
            if (n[1] == 0 and o[1] > 0) or n[0] * o[1] < o[0] * n[1]:
                falls.append("%s %s: %s (%d/%d) fell from %s (%d/%d)" % (
                    subsystem, label, percent(*n), *n, percent(*o), *o))
            elif n[0] * o[1] > o[0] * n[1]:
                notes.append("%s %s rose from %s to %s" % (subsystem, label, percent(*o), percent(*n)))
    for subsystem in sorted(set(got) - set(want)):
        notes.append("%s is new" % subsystem)
    return falls, notes


def table(totals: dict[str, Totals]) -> str:
    """The per-subsystem summary: function, line and branch coverage."""
    head = ["subsystem", "functions", "lines", "branches", "never entered", "dead"]
    rows = [head]
    for subsystem in sorted(totals):
        t = totals[subsystem]
        rows.append([subsystem, percent(t.functions_entered, t.functions), percent(t.lines_hit, t.lines),
                     percent(t.branches_covered, t.branches), str(len(t.never_entered)), str(t.dead)])
    widths = [max(len(r[i]) for r in rows) for i in range(len(head))]
    return "\n".join("  ".join(c.ljust(w) if i == 0 else c.rjust(w) for i, (c, w) in enumerate(zip(r, widths)))
                     for r in rows)


def windows_path(path: Path) -> str:
    """The Z: path Wine gives a host path."""
    return "Z:" + str(path.resolve()).replace("/", "\\")


def play_suite(exe: Path, game_dir: str | None, parallel: int, names: list[str] | None) -> Path:
    """Play the scenarios on the coverage exe, one profile directory each; the merged profile's path. Exits 1 on a failure or a missing profile."""
    scenarios = reference.scenario_files()
    unknown = [n for n in names or [] if n not in scenarios]
    if unknown:
        sys.exit("coverage: no such scenario: %s" % ", ".join(unknown))
    if names:
        scenarios = {n: scenarios[n] for n in names}
    if RUN_DIR.exists():
        sys.exit("coverage: %s exists from an earlier run; remove it first" % RUN_DIR.relative_to(ROOT))
    local = threading.local()
    original_play = reference.play_scenario
    original_tracediff_play = tracediff.play

    def tracediff_play(*args, **kwargs):
        name = getattr(local, "scenario", None)
        if name:
            for stale in (RUN_DIR / name).glob("*.profraw"):
                stale.unlink()
            profile = windows_path(RUN_DIR / name) + "\\cov-%p.profraw"
            kwargs["env"] = {**os.environ, "LLVM_PROFILE_FILE": profile}
        return original_tracediff_play(*args, **kwargs)

    def play_scenario(exe, scenario, *args, **kwargs):
        local.scenario = scenario.stem
        (RUN_DIR / scenario.stem).mkdir(parents=True, exist_ok=True)
        return original_play(exe, scenario, *args, **kwargs)

    failed: dict[str, str] = {}
    tracediff.play = tracediff_play
    reference.play_scenario = play_scenario
    try:
        reference.play_all(exe, scenarios, game_dir, parallel, failed=failed, retry_any=True)
    finally:
        tracediff.play = original_tracediff_play
        reference.play_scenario = original_play
    problems = [f"{n}: failed to run: {m}" for n, m in sorted(failed.items())]
    profiles = []
    for name in sorted(scenarios):
        found = sorted((RUN_DIR / name).glob("*.profraw"))
        if name not in failed and not found:
            problems.append(f"{name}: wrote no profile")
        profiles += found
    if problems:
        sys.exit("coverage: the suite is incomplete, so this is not a baseline:\n" + "\n".join(problems))
    merged = RUN_DIR / "merged.profdata"
    subprocess.run(["llvm-profdata", "merge", "-sparse", "-o", str(merged), *map(str, profiles)], check=True)
    return merged


def export(exe: Path, profdata: Path) -> dict:
    """The llvm-cov export (JSON) of the exe with the profile."""
    with tempfile.TemporaryFile("w+") as out:
        subprocess.run(["llvm-cov", "export", "-format=text", "-skip-expansions", str(exe),
                        f"-instr-profile={profdata}"], stdout=out, check=True)
        out.seek(0)
        return json.load(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--exe", type=Path, default=DEFAULT_EXE, help="the coverage build (default build/coverage/TotalA.exe)")
    parser.add_argument("--game-dir", help="the game's install (default: Steam's)")
    parser.add_argument("--parallel", type=int, metavar="N", help="scenarios played at once")
    parser.add_argument("--scenario", nargs="+", metavar="NAME", help="play only these (refuses --write and --check)")
    parser.add_argument("--from", dest="profdata", type=Path, metavar="PROFDATA",
                        help="use this merged profile and play nothing")
    parser.add_argument("--live-counters", type=Path, metavar="FILE",
                        help="the counters live code increments, from the coverage link (default: live_counters.txt beside the exe)")
    parser.add_argument("--write", action="store_true", help="write data/coverage.csv")
    parser.add_argument("--check", action="store_true", help="fail if a subsystem's coverage fell below data/coverage.csv")
    args = parser.parse_args()
    if args.scenario and (args.write or args.check):
        sys.exit("coverage: --scenario plays a subset; it cannot be used with --write or --check")
    if args.write and args.check:
        sys.exit("coverage: use --write or --check, not both")
    if args.check and not OUT.is_file():
        sys.exit(f"coverage: {OUT.relative_to(ROOT)} is missing; write it with --write")
    if not args.exe.is_file():
        sys.exit(f"coverage: no exe at {args.exe}")
    live_path = args.live_counters or args.exe.parent / LIVE_COUNTERS
    if not live_path.is_file():
        sys.exit(f"coverage: no {live_path}; rebuild the coverage build to write it")
    profdata = args.profdata or play_suite(args.exe, args.game_dir, args.parallel or reference.default_parallel(),
                                           args.scenario)
    print(f"profile: {profdata}")
    by_key, by_name = read_subsystems()
    totals, excluded, unassigned = summarize(export(args.exe, profdata), by_key, by_name,
                                           read_live_counters(live_path))
    print(table(totals))
    if args.write and not totals:
        sys.exit("coverage: no game function in the export; not writing an empty baseline")
    print(f"excluded (harness, build, outside the repository): {excluded} functions")
    print(f"unassigned: {unassigned} functions")
    if args.write:
        write_csv(totals, OUT)
        print(f"wrote {OUT.relative_to(ROOT)}")
    if args.check:
        falls, notes = compare(read_csv(OUT), totals)
        for note in notes:
            print(f"note: {note}")
        if notes:
            print("coverage rose or is new: run with --write to update the baseline")
        if falls:
            sys.exit("coverage fell:\n" + "\n".join(falls))
        print("coverage: no subsystem fell")


if __name__ == "__main__":
    main()
