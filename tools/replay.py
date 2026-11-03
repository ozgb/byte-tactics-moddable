"""Record, check and compare the seam call recordings of a subsystem (docs/replay-format.md).

    uv run tools/replay.py --record --exe build/modern/TotalA.exe --out DIR [--native-modules] [--window FIRST LAST] [SCENARIO...]
    uv run tools/replay.py --check --exe build/modern/TotalA.exe [--native-modules] [SCENARIO...]
    uv run tools/replay.py --compare DIR_A DIR_B

`--record` plays each scenario of reference/scenarios/ (a name or a path; with none, the scenarios that
tests/replay/ holds a recording of) under Wine, on a copy that ends with a `record wind_meteors` line, and writes
<DIR>/<scenario>.wind_meteors.txt. `--native-modules` plays the modules as native code (-modules=native).
`--check` records into a temporary directory and fails when a recording differs from the one in tests/replay/.
`--compare` fails when two directories do not hold the same files with the same bytes, and names the first
differing line.
"""

import argparse
import os
import shutil
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import tracediff

ROOT = Path(__file__).resolve().parent.parent
SCENARIOS = ROOT / "reference/scenarios"
RECORDINGS = ROOT / "tests/replay"
SUBSYSTEM = "wind_meteors"
SUFFIX = f".{SUBSYSTEM}.txt"
OUTPUT = f"replay-{SUBSYSTEM}.txt"
RUN_SECONDS_BASE = 120
RUN_TICKS_PER_SECOND = 25


class Error(Exception):
    pass


def scenario_path(name: str) -> Path:
    """The scenario file named by `name`: a path, or a name of reference/scenarios/."""
    path = Path(name)
    if path.is_file():
        return path.resolve()
    named = SCENARIOS / (name if name.endswith(".txt") else name + ".txt")
    if not named.is_file():
        raise Error(f"no scenario {name}")
    return named


def recorded_names() -> list[str]:
    """The scenarios tests/replay/ holds a recording of."""
    return sorted(p.name[: -len(SUFFIX)] for p in RECORDINGS.glob("*" + SUFFIX))


def with_record(text: str, window: tuple[int, int] | None = None) -> str:
    """The scenario text with a record line at the end."""
    spec = SUBSYSTEM if window is None else f"{SUBSYSTEM} {window[0]} {window[1]}"
    return text + ("" if text.endswith("\n") else "\n") + f"record {spec}\n"


def scenario_ticks(text: str) -> int:
    for line in text.splitlines():
        parts = line.split()
        if parts[:1] == ["ticks"] and len(parts) == 2 and parts[1].isdigit():
            return int(parts[1])
    raise Error("the scenario has no valid ticks line")


def first_difference(a: str, b: str) -> str | None:
    """The first line at which two recordings differ, as 'line N: A / B', or None when they are equal."""
    if a == b:
        return None
    la, lb = a.split("\n"), b.split("\n")
    for i in range(max(len(la), len(lb))):
        x = la[i] if i < len(la) else "<end>"
        y = lb[i] if i < len(lb) else "<end>"
        if x != y:
            return f"line {i + 1}: {x!r} / {y!r}"
    return "the files differ"


def check_form(text: str) -> list[str]:
    """Reasons the text is not a bt-replay 1 recording of the subsystem (empty when it is)."""
    lines = text.split("\n")
    problems = []
    if lines[:2] != ["bt-replay 1", f"subsystem {SUBSYSTEM} 1.2"]:
        problems.append("the header is not bt-replay 1 and subsystem wind_meteors 1.2")
    if lines[-1] != "":
        problems.append("the file does not end with a newline")
    inside = False
    for n, line in enumerate(lines[2:-1], 3):
        if line.startswith("call "):
            inside = True
        elif not (inside and len(line) > 4 and line.startswith("  ") and line[2] in "raiow" and line[3] == " "):
            problems.append(f"line {n} is neither a call nor a line of one: {line!r}")
    return problems


def play_recording(exe: Path, scenario: Path, game_dir: str | None, native: bool, window: tuple[int, int] | None,
                   directory: Path) -> str:
    """Play one scenario with a record line and return the text of the recording it wrote."""
    directory.mkdir(parents=True, exist_ok=True)
    copy = directory / scenario.name
    copy.write_text(with_record(scenario.read_text(), window))
    ticks = scenario_ticks(scenario.read_text())
    timeout = RUN_SECONDS_BASE + ticks / RUN_TICKS_PER_SECOND
    env = dict(os.environ, BT_NATIVE_MODULES="1" if native else "0")
    saver = tracediff.scenario_load(scenario)
    put = None
    scratch = None
    try:
        if saver is not None:
            saver_path = scenario.parent / f"{saver}.txt"
            scratch = Path(tempfile.mkdtemp(prefix="save-", dir=ROOT / "build/scratch"))
            run = tracediff.play(exe, saver_path, game_dir, None, timeout, env=env, collect=tracediff.SAVE_FILES)
            try:
                tracediff.copy_save(run, scratch)
            finally:
                shutil.rmtree(run.parent, ignore_errors=True)
            put = tracediff.save_put(scratch, scenario.stem, saver)
        run = tracediff.play(exe, copy, game_dir, None, timeout, env=env, put=put, collect=(OUTPUT,))
        try:
            return tracediff.need(run, OUTPUT).read_text()
        finally:
            shutil.rmtree(run.parent, ignore_errors=True)
    finally:
        if scratch is not None:
            shutil.rmtree(scratch, ignore_errors=True)


def record(exe: Path, names: list[str], game_dir: str | None, native: bool, window: tuple[int, int] | None,
           out: Path, parallel: int) -> dict[str, str]:
    """Record the scenarios into `out`; the text of each recording by scenario name."""
    out.mkdir(parents=True, exist_ok=True)
    work = out / ".scenarios"
    results: dict[str, str] = {}

    def one(name: str) -> None:
        scenario = scenario_path(name)
        text = play_recording(exe, scenario, game_dir, native, window, work / scenario.stem)
        problems = check_form(text)
        if problems:
            raise Error(f"{scenario.stem}: the recording is malformed: {problems[0]}")
        (out / (scenario.stem + SUFFIX)).write_bytes(text.encode("latin-1"))
        results[scenario.stem] = text

    try:
        with ThreadPoolExecutor(parallel) as pool:
            list(pool.map(one, names))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return results


def compare(a: Path, b: Path) -> list[str]:
    """The differences between two directories of recordings, one line each (empty when they are the same)."""
    names_a = sorted(p.name for p in a.glob("*" + SUFFIX))
    names_b = sorted(p.name for p in b.glob("*" + SUFFIX))
    found = [f"only in {a}: {n}" for n in names_a if n not in names_b] + [f"only in {b}: {n}" for n in names_b if n not in names_a]
    for name in names_a:
        if name in names_b and (a / name).read_bytes() != (b / name).read_bytes():
            found.append(f"{name}: {first_difference((a / name).read_text(), (b / name).read_text())}")
    return found


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("scenario", nargs="*")
    ap.add_argument("--record", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--compare", nargs=2, metavar=("DIR_A", "DIR_B"))
    ap.add_argument("--exe")
    ap.add_argument("--game-dir")
    ap.add_argument("--out", type=Path)
    ap.add_argument("--native-modules", action="store_true")
    ap.add_argument("--window", nargs=2, type=int, metavar=("FIRST", "LAST"))
    ap.add_argument("--parallel", type=int, default=2)
    args = ap.parse_args()
    modes = [bool(args.record), bool(args.check), bool(args.compare)]
    if sum(modes) != 1:
        ap.error("give exactly one of --record, --check and --compare")
    try:
        if args.compare:
            found = compare(Path(args.compare[0]), Path(args.compare[1]))
            for line in found:
                print(line)
            print("recordings identical" if not found else f"{len(found)} differences")
            return 1 if found else 0
        if not args.exe:
            ap.error("--record and --check need --exe")
        exe = Path(args.exe).resolve()
        names = args.scenario or recorded_names()
        window = tuple(args.window) if args.window else None
        if args.record:
            if not args.out:
                ap.error("--record needs --out")
            results = record(exe, names, args.game_dir, args.native_modules, window, args.out, args.parallel)
            for name, text in sorted(results.items()):
                print(f"{name}: {len(text.encode('latin-1'))} bytes, {text.count(chr(10) + 'call ') + 1} calls")
            return 0
        (ROOT / "build/scratch").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="replay-", dir=ROOT / "build/scratch") as tmp:
            record(exe, names, args.game_dir, args.native_modules, None, Path(tmp), args.parallel)
            bad = 0
            for name in names:
                stored = RECORDINGS / (scenario_path(name).stem + SUFFIX)
                got = (Path(tmp) / stored.name).read_text()
                diff = first_difference(stored.read_text(), got) if stored.is_file() else "no stored recording"
                print(f"{stored.name}: {'equal' if diff is None else 'differs, ' + diff}")
                bad += diff is not None
            return 1 if bad else 0
    except (Error, tracediff.Error) as e:
        print(f"replay: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
