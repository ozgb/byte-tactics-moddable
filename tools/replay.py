"""Record, check and compare the seam call recordings of the subsystems (docs/replay-format.md).

    uv run tools/replay.py --record --exe build/modern/TotalA.exe --out DIR [--subsystem NAME]... [--native-modules] [--window FIRST LAST] [SCENARIO...]
    uv run tools/replay.py --check --exe build/modern/TotalA.exe [--subsystem NAME]... [--native-modules] [--json FILE] [SCENARIO...]
    uv run tools/replay.py --compare DIR_A DIR_B

`--record` plays each scenario of reference/scenarios/ (a name or a path; with none, the scenarios that
tests/replay/<subsystem>/ holds a recording of) under Wine, on a copy that ends with one `record <subsystem>` line
per subsystem, and writes <DIR>/<subsystem>/<scenario>.txt. `--subsystem` (repeatable) picks the subsystems; the
default is every subsystem that has a tests/replay/<subsystem>/ directory. `--native-modules` plays the modules as
native code (-modules=native). `--check` records into a temporary directory and fails when a recording differs from
the one in tests/replay/; `--json FILE` also writes, per recording, `equal` or the first difference as the played run has it
(call tick, slot, kind and name of the line, and its value; never the stored line), the form tools/oraclegate.py reads. `--compare` fails when two directories do not hold the same files with the same bytes, and
names the first differing line.
"""

import argparse
import json
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
SCHEMA = ROOT / "data/replay_schema.json"
WINDOWS = RECORDINGS / "windows.json"
FORMAT = 2
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


def schema_versions() -> dict[str, str]:
    """Subsystem -> its interface version as the header of a recording writes it."""
    model = json.loads(SCHEMA.read_text())
    return {name: "%d.%d" % tuple(spec["header"]["version"]) for name, spec in model["subsystems"].items()}


def default_subsystems() -> list[str]:
    """The subsystems that have a directory of recordings."""
    return [s for s in schema_versions() if (RECORDINGS / s).is_dir()]


def recorded_names(subsystems: list[str]) -> list[str]:
    """The scenarios tests/replay/ holds a recording of, for any of the subsystems."""
    return sorted({p.stem for s in subsystems for p in (RECORDINGS / s).glob("*.txt")})


def stored_windows() -> dict[str, tuple[int, int]]:
    """The windows of the stored recordings that cover only some ticks: "<subsystem>/<scenario>" -> (first, last)."""
    if not WINDOWS.is_file():
        return {}
    return {k: (v[0], v[1]) for k, v in json.loads(WINDOWS.read_text()).items()}


def with_record(text: str, subsystems: list[str] | str, window: tuple[int, int] | dict[str, tuple[int, int]] | None = None) -> str:
    """The scenario text with a record line per subsystem at the end; a dict window is per subsystem."""
    names = [subsystems] if isinstance(subsystems, str) else subsystems
    lines = ""
    for s in names:
        w = window.get(s) if isinstance(window, dict) else window
        lines += f"record {s}\n" if w is None else f"record {s} {w[0]} {w[1]}\n"
    return text + ("" if text.endswith("\n") else "\n") + lines


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


def divergence(stored: str, got: str) -> dict:
    """The first difference of two recordings described from `got` alone: the line number, the tick and slot of the call it is in,
    and the kind, name and value of the line (None for a line `got` does not have). {"status": "equal"} when they are the same."""
    if stored == got:
        return {"status": "equal"}
    a, b = stored.split("\n"), got.split("\n")
    at = next((i for i in range(max(len(a), len(b))) if (a[i] if i < len(a) else None) != (b[i] if i < len(b) else None)), 0)
    call = next((b[j] for j in range(min(at, len(b) - 1), -1, -1) if b[j].startswith("call ")), "")
    ticks, _, slot = call[5:].partition(" ")
    line = b[at] if at < len(b) else None
    found = {"status": "differs", "line": at + 1, "tick": int(ticks) if ticks.isdigit() else None, "slot": slot or None,
             "kind": None, "field": None, "value": None}
    if line is None or not line.strip():
        return found
    text = line.strip()
    if text.startswith("call "):
        _, _, ticks, slot = (text.split(" ") + ["", ""])[:4]
        found.update(kind="call", tick=int(ticks) if ticks.isdigit() else None, slot=slot or None, field=slot or None)
        return found
    kind, _, rest = text.partition(" ")
    field, _, value = rest.partition(" ")
    found.update(kind=kind, field=field, value=value)
    return found


def check_form(text: str, subsystem: str = "wind_meteors") -> list[str]:
    """Reasons the text is not a bt-replay 2 recording of the subsystem (empty when it is)."""
    lines = text.split("\n")
    problems = []
    versions = schema_versions()
    if lines[0] != f"bt-replay {FORMAT}" or lines[1].split(" ")[:2] != ["subsystem", subsystem] or lines[1].split(" ")[2:3] != [versions.get(subsystem)]:
        problems.append(f"the header is not bt-replay {FORMAT} and subsystem {subsystem} {versions.get(subsystem)}")
    if lines[-1] != "":
        problems.append("the file does not end with a newline")
    inside = False
    for n, line in enumerate(lines[2:-1], 3):
        if line.startswith("call "):
            inside = True
        elif not (inside and len(line) > 4 and line.startswith("  ") and line[2] in "raciow" and line[3] == " "):
            problems.append(f"line {n} is neither a call nor a line of one: {line!r}")
    return problems


def play_recording(exe: Path, scenario: Path, game_dir: str | None, native: bool, window: tuple[int, int] | dict[str, tuple[int, int]] | None,
                   directory: Path, subsystems: list[str]) -> dict[str, str]:
    """Play one scenario with a record line per subsystem and return the text of each recording it wrote."""
    directory.mkdir(parents=True, exist_ok=True)
    copy = directory / scenario.name
    copy.write_text(with_record(scenario.read_text(), subsystems, window))
    ticks = scenario_ticks(scenario.read_text())
    timeout = RUN_SECONDS_BASE + ticks / RUN_TICKS_PER_SECOND
    env = dict(os.environ, BT_NATIVE_MODULES="1" if native else "0")
    saver = tracediff.scenario_load(scenario)
    put = None
    scratch = None
    outputs = tuple(f"replay-{s}.txt" for s in subsystems)
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
        run = tracediff.play(exe, copy, game_dir, None, timeout, env=env, put=put, collect=outputs)
        try:
            return {s: tracediff.need(run, f"replay-{s}.txt").read_text() for s in subsystems}
        finally:
            shutil.rmtree(run.parent, ignore_errors=True)
    finally:
        if scratch is not None:
            shutil.rmtree(scratch, ignore_errors=True)


def record(exe: Path, plan: dict[str, list[str]], game_dir: str | None, native: bool, window: tuple[int, int] | None,
           out: Path, parallel: int) -> dict[tuple[str, str], str]:
    """Record the scenarios of the plan (scenario name -> subsystems) into `out`/<subsystem>/; the text by (subsystem, scenario)."""
    out.mkdir(parents=True, exist_ok=True)
    work = out / ".scenarios"
    results: dict[tuple[str, str], str] = {}
    stored = stored_windows()

    def one(name: str) -> None:
        scenario = scenario_path(name)
        chosen = window if window is not None else {
            s: stored[f"{s}/{scenario.stem}"] for s in plan[name] if f"{s}/{scenario.stem}" in stored}
        texts = play_recording(exe, scenario, game_dir, native, chosen, work / scenario.stem, plan[name])
        for subsystem, text in texts.items():
            problems = check_form(text, subsystem)
            if problems:
                raise Error(f"{scenario.stem}: the {subsystem} recording is malformed: {problems[0]}")
            (out / subsystem).mkdir(parents=True, exist_ok=True)
            (out / subsystem / (scenario.stem + ".txt")).write_bytes(text.encode("latin-1"))
            results[(subsystem, scenario.stem)] = text

    try:
        with ThreadPoolExecutor(parallel) as pool:
            list(pool.map(one, plan))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return results


def compare(a: Path, b: Path) -> list[str]:
    """The differences between two directories of recordings, one line each (empty when they are the same)."""
    names_a = sorted(str(p.relative_to(a)) for p in a.glob("*/*.txt"))
    names_b = sorted(str(p.relative_to(b)) for p in b.glob("*/*.txt"))
    found = [f"only in {a}: {n}" for n in names_a if n not in names_b] + [f"only in {b}: {n}" for n in names_b if n not in names_a]
    for name in names_a:
        if name in names_b and (a / name).read_bytes() != (b / name).read_bytes():
            found.append(f"{name}: {first_difference((a / name).read_text(), (b / name).read_text())}")
    return found


def plan_for(subsystems: list[str], names: list[str] | None) -> dict[str, list[str]]:
    """Scenario name -> the subsystems to record in it. Without names, the scenarios with a stored recording."""
    if names:
        return {n: list(subsystems) for n in names}
    plan: dict[str, list[str]] = {}
    for s in subsystems:
        for p in sorted((RECORDINGS / s).glob("*.txt")):
            plan.setdefault(p.stem, []).append(s)
    return dict(sorted(plan.items()))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("scenario", nargs="*")
    ap.add_argument("--record", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--compare", nargs=2, metavar=("DIR_A", "DIR_B"))
    ap.add_argument("--exe")
    ap.add_argument("--game-dir")
    ap.add_argument("--out", type=Path)
    ap.add_argument("--subsystem", action="append", default=[])
    ap.add_argument("--native-modules", action="store_true")
    ap.add_argument("--window", nargs=2, type=int, metavar=("FIRST", "LAST"))
    ap.add_argument("--json", type=Path, metavar="FILE", help="--check: write the verdict of each recording to FILE")
    ap.add_argument("--parallel", type=int, default=2)
    args = ap.parse_args()
    modes = [bool(args.record), bool(args.check), bool(args.compare)]
    if sum(modes) != 1:
        ap.error("give exactly one of --record, --check and --compare")
    if args.json and not args.check:
        ap.error("--json needs --check")
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
        known = schema_versions()
        for s in args.subsystem:
            if s not in known:
                raise Error(f"{s} is not a simulation subsystem")
        subsystems = args.subsystem or default_subsystems()
        if not subsystems:
            raise Error("no subsystem has recordings")
        window = tuple(args.window) if args.window else None
        if args.record:
            if not args.out:
                ap.error("--record needs --out")
            plan = plan_for(subsystems, args.scenario)
            results = record(exe, plan, args.game_dir, args.native_modules, window, args.out, args.parallel)
            for (subsystem, name), text in sorted(results.items()):
                print(f"{subsystem}/{name}: {len(text.encode('latin-1'))} bytes, {text.count(chr(10) + 'call ') + 1} calls")
            return 0
        (ROOT / "build/scratch").mkdir(parents=True, exist_ok=True)
        plan = plan_for(subsystems, args.scenario)
        with tempfile.TemporaryDirectory(prefix="replay-", dir=ROOT / "build/scratch") as tmp:
            record(exe, plan, args.game_dir, args.native_modules, None, Path(tmp), args.parallel)
            bad = 0
            verdicts: dict[str, dict] = {}
            for name, subs in plan.items():
                for subsystem in subs:
                    stored = RECORDINGS / subsystem / (scenario_path(name).stem + ".txt")
                    got = (Path(tmp) / subsystem / stored.name).read_text()
                    diff = first_difference(stored.read_text(), got) if stored.is_file() else "no stored recording"
                    print(f"{subsystem}/{stored.name}: {'equal' if diff is None else 'differs, ' + diff}")
                    bad += diff is not None
                    verdicts[f"{subsystem}/{stored.stem}"] = divergence(stored.read_text(), got) if stored.is_file() else {"status": "differs", "line": 0}
            if args.json:
                args.json.parent.mkdir(parents=True, exist_ok=True)
                args.json.write_text(json.dumps(verdicts, indent=1, sort_keys=True) + "\n")
            return 1 if bad else 0
    except (Error, tracediff.Error) as e:
        print(f"replay: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
