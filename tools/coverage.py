"""Report how much of each subsystem the scenario suite runs: function, line and branch coverage.

    uv run tools/coverage.py --exe build/coverage/TotalA.exe --game-dir ~/.ta   # play the suite, print the table
    uv run tools/coverage.py --exe E --game-dir D --write                       # also write data/coverage.csv
    uv run tools/coverage.py --exe E --game-dir D --check                       # fail if a subsystem's coverage fell
    uv run tools/coverage.py --exe E --from PROFDATA [--write|--check]          # use a merged profile, play nothing
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

Counting, per function and then summed per subsystem. Functions: entered when the execution count is
above 0. Lines: the lines of the function's code regions in its primary file; a line's count is the
largest count of the regions that start on it, or else of the innermost region that covers it; the line
is hit when that count is above 0. Branches: each branch condition has a true and a false side, both
counted in the total, and each side taken at least once is covered.

data/coverage.csv: columns subsystem, function, file, functions, functions_entered, lines, lines_hit,
branches, branches_covered. One totals row per subsystem (function `*`, file empty) and one row per
function no scenario enters (functions 1, every other count 0), sorted by
subsystem, totals first. Percentages are derived. `--check` fails when a subsystem's function, line or
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
UNASSIGNED = "unassigned"
STATE_PREFIXES = ("interfaces/", "src/state/")
STATE = "state"
EXCLUDED_PREFIXES = ("src/harness/", "build/", "toolchain/")
COLUMNS = ["subsystem", "function", "file", "functions", "functions_entered", "lines", "lines_hit",
           "branches", "branches_covered"]
METRICS = (("functions", "functions", "functions_entered"), ("lines", "lines", "lines_hit"),
           ("branches", "branches", "branches_covered"))
CODE_REGION = 0
REGION_FILE_ID = 5
REGION_KIND = 7
BRANCH_FILE_ID = 7


@dataclass
class Function:
    """One function of the export: its place, whether it was entered, and its line and branch counts."""
    name: str
    file: str
    entered: bool
    lines: int
    lines_hit: int
    branches: int
    branches_covered: int


@dataclass
class Totals:
    """The sums of a subsystem's functions."""
    functions: int = 0
    functions_entered: int = 0
    lines: int = 0
    lines_hit: int = 0
    branches: int = 0
    branches_covered: int = 0
    never_entered: list[tuple[str, str]] = field(default_factory=list)

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


def plain_name(name: str) -> str:
    """The function's name without MSVC decoration or the `file.cpp:` prefix llvm-cov gives static functions."""
    if not name.startswith("?") and ":" in name:
        name = name.split(":", 1)[1]
    special = re.match(r"\?\?(_?[0-9A-Z])(?:\?\$([^@]+)@|([^@?]+)@)", name)
    if special:
        code, owner = special.group(1), special.group(2) or special.group(3)
        member = owner if code == "0" else "~" + owner if code == "1" else "operator?" + code
        return "%s::%s" % (owner, member)
    return blockpointers.demangle(name)


def region_lines(regions: list[list[int]], primary: set[int]) -> dict[int, int]:
    """Line -> count over the code regions in the primary file: the largest count of the regions starting on the line, else the innermost covering region's."""
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
    counts.update(starts)
    return counts


def parse_function(entry: dict) -> tuple[str, Function] | None:
    """(repository file, Function) of an export entry, or None when it is not game code (the file is outside the repository, in the harness or build)."""
    names = entry.get("filenames") or []
    if not names:
        return None
    rel = repo_path(names[0])
    if rel is None:
        return None
    primary = {i for i, n in enumerate(names) if n == names[0]}
    counts = region_lines(entry.get("regions", []), primary)
    branches = [b for b in entry.get("branches", []) if b[BRANCH_FILE_ID] in primary]
    return rel, Function(plain_name(entry["name"]), rel, entry.get("count", 0) > 0, len(counts),
                         sum(1 for c in counts.values() if c > 0), 2 * len(branches),
                         sum((b[4] > 0) + (b[5] > 0) for b in branches))


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


def summarize(export: dict, by_key: dict[tuple[str, str], str],
              by_name: dict[str, set[str]]) -> tuple[dict[str, Totals], int, int]:
    """(subsystem -> Totals, functions excluded, functions unassigned) of an llvm-cov export."""
    entered: dict[tuple[str, str, str], bool] = {}
    totals: dict[str, Totals] = {}
    excluded = unassigned = 0
    for data in export.get("data", []):
        for entry in data.get("functions", []):
            parsed = parse_function(entry)
            if parsed is None:
                excluded += 1
                continue
            _, fn = parsed
            subsystem = subsystem_of(fn, by_key, by_name)
            unassigned += subsystem == UNASSIGNED
            totals.setdefault(subsystem, Totals()).add(fn)
            key = (subsystem, fn.name, fn.file)
            entered[key] = entered.get(key, False) or fn.entered
    for (subsystem, name, file), was_entered in entered.items():
        if not was_entered:
            totals[subsystem].never_entered.append((name, file))
    for t in totals.values():
        t.never_entered.sort()
    return totals, excluded, unassigned


def csv_rows(totals: dict[str, Totals]) -> list[list[str]]:
    """The rows of data/coverage.csv: per subsystem a totals row, then a row per never-entered function."""
    rows = []
    for subsystem in sorted(totals):
        t = totals[subsystem]
        rows.append([subsystem, "*", "", t.functions, t.functions_entered, t.lines, t.lines_hit,
                     t.branches, t.branches_covered])
        rows += [[subsystem, name, file, 1, 0, 0, 0, 0, 0] for name, file in t.never_entered]
    return [[str(c) for c in r] for r in rows]


def write_csv(totals: dict[str, Totals], path: Path) -> None:
    """Write the rows of csv_rows to path."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(COLUMNS)
        w.writerows(csv_rows(totals))


def read_csv(path: Path) -> dict[str, Totals]:
    """The totals and never-entered lists that write_csv wrote."""
    totals: dict[str, Totals] = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            t = totals.setdefault(row["subsystem"], Totals())
            if row["function"] == "*":
                for _, attr, hit in METRICS:
                    setattr(t, attr, int(row[attr]))
                    setattr(t, hit, int(row[hit]))
            else:
                t.never_entered.append((row["function"], row["file"]))
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
    head = ["subsystem", "functions", "lines", "branches", "never entered"]
    rows = [head]
    for subsystem in sorted(totals):
        t = totals[subsystem]
        rows.append([subsystem, percent(t.functions_entered, t.functions), percent(t.lines_hit, t.lines),
                     percent(t.branches_covered, t.branches), str(len(t.never_entered))])
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
    profdata = args.profdata or play_suite(args.exe, args.game_dir, args.parallel or reference.default_parallel(),
                                           args.scenario)
    print(f"profile: {profdata}")
    by_key, by_name = read_subsystems()
    totals, excluded, unassigned = summarize(export(args.exe, profdata), by_key, by_name)
    print(table(totals))
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
