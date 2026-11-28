"""Record, check and compare the seam call recordings of the subsystems (docs/replay-format.md).

    uv run tools/replay.py --record --exe build/modern/TotalA.exe --out DIR [--subsystem NAME]... [--native-modules] [--window FIRST LAST] [SCENARIO...]
    uv run tools/replay.py --check --exe build/modern/TotalA.exe [--subsystem NAME]... [--native-modules] [--json FILE] [SCENARIO...]
    uv run tools/replay.py --compare DIR_A DIR_B

`--record` plays each scenario of reference/scenarios/ (a name or a path; with none, the scenarios that
tests/replay/<subsystem>/ holds a recording of, in it or in a directory below it such as long/) under Wine, once per subsystem, on a copy that ends with that
subsystem's `record` line, and writes <DIR>/<subsystem>/<scenario>.txt, or <DIR>/<subsystem>/long/<scenario>.txt when the stored one is there. `--subsystem` (repeatable) picks the subsystems; the
default is every subsystem that has a tests/replay/<subsystem>/ directory. `--native-modules` plays the modules as
native code (-modules=native). `--check` records into a temporary directory and fails when a recording differs from
the one in tests/replay/ (every recording below tests/replay/<subsystem>/, long/ included, but the play-test recordings that
tests/replay/playtests.json lists), and names each by its path (`mission/long/mission-build-unit.txt`); `--json FILE` also writes, per recording, `equal` or the first difference as the played run has it
(call tick, slot, kind and name of the line, and its value; never the stored line), keyed by that path without `.txt`, the form tools/oraclegate.py reads. `--compare` fails when two directories do not hold the same files with the same bytes, long/ included, and
names the first differing line.

The comparison of `--check` frees what #1209 says a module's behaviour does not depend on: `services.GetGafFrame` is a query answered
by its arguments, so its calls (and their answers) may come in any order and number, but a call with arguments the stored recording
has no answer to, or an answer that differs, is a difference (reported as the query). An answer names its frame `GafFrame#N`, numbered
by first appearance, so only the calls that answer a frame already named are free; two calls that name new frames in the other order
differ; and the y of the position of
`los_radar.GetMapCellAtPosition` is an unread argument. Everything else is compared as text.
"""

import argparse
import json
import os
import re
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
PLAYTESTS = RECORDINGS / "playtests.json"
PHASES = RECORDINGS / "phases.json"
FORMAT = 2
RUN_SECONDS_BASE = 600
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
    """Subsystem -> its interface version as the header of a recording writes it; a world view is never recorded."""
    model = json.loads(SCHEMA.read_text())
    return {name: "%d.%d" % tuple(spec["header"]["version"]) for name, spec in model["subsystems"].items() if not spec["header"].get("world")}


def default_subsystems() -> list[str]:
    """The subsystems that have a directory of recordings."""
    return [s for s in schema_versions() if (RECORDINGS / s).is_dir()]


def playtest_recordings() -> dict[str, str]:
    """The stored recordings that a play-test made, not a scenario: "<subsystem>/<name>" -> the command that records it.
    --check does not record them again."""
    if not PLAYTESTS.is_file():
        return {}
    return json.loads(PLAYTESTS.read_text())


def recording_name(path: Path, root: Path | None = None) -> str:
    """The name of a stored recording: its path below tests/replay/ (or `root`) without `.txt` (`mission/long/mission-build-unit`)."""
    return path.relative_to(root or RECORDINGS).with_suffix("").as_posix()


def stored_files(directory: Path) -> list[Path]:
    """Every recording below a directory of recordings, in a subdirectory such as long/ too, but none under a hidden directory."""
    return sorted(p for p in directory.rglob("*.txt") if not any(part.startswith(".") for part in p.relative_to(directory).parts))


def scenario_recordings(subsystem: str) -> list[Path]:
    """The stored recordings of the subsystem that a scenario of reference/scenarios/ makes, long/ included."""
    skipped = playtest_recordings()
    return [p for p in stored_files(RECORDINGS / subsystem) if recording_name(p) not in skipped]


def stored_recording(subsystem: str, scenario: str) -> Path:
    """The stored recording of the scenario for the subsystem, in tests/replay/<subsystem>/ or below it; tests/replay/<subsystem>/<scenario>.txt when there is none."""
    found = [p for p in scenario_recordings(subsystem) if p.stem == scenario]
    if len(found) > 1:
        raise Error(f"{subsystem} has {len(found)} stored recordings of {scenario}: " + ", ".join(recording_name(p) for p in found))
    return found[0] if found else RECORDINGS / subsystem / (scenario + ".txt")


def recorded_names(subsystems: list[str]) -> list[str]:
    """The scenarios tests/replay/ holds a recording of, for any of the subsystems."""
    return sorted({p.stem for s in subsystems for p in scenario_recordings(s)})


def stored_windows() -> dict[str, tuple[int, int]]:
    """The windows of the stored recordings that cover only some ticks: "<subsystem>/<scenario>" -> (first, last)."""
    if not WINDOWS.is_file():
        return {}
    return {k: (v[0], v[1]) for k, v in json.loads(WINDOWS.read_text()).items()}


def stored_phases() -> set[str]:
    """The stored recordings that record the phase driver whole, not its slots: "<subsystem>/<scenario>"."""
    if not PHASES.is_file():
        return set()
    return set(json.loads(PHASES.read_text()))


def with_record(text: str, subsystems: list[str] | str, window: tuple[int, int] | dict[str, tuple[int, int]] | None = None,
                phase: frozenset[str] | set[str] = frozenset()) -> str:
    """The scenario text with a record line per subsystem at the end; a dict window is per subsystem, and a subsystem of
    `phase` records its phase driver."""
    names = [subsystems] if isinstance(subsystems, str) else subsystems
    lines = ""
    for s in names:
        w = window.get(s) if isinstance(window, dict) else window
        tail = " phase" if s in phase else ""
        lines += f"record {s}{tail}\n" if w is None else f"record {s} {w[0]} {w[1]}{tail}\n"
    return text + ("" if text.endswith("\n") else "\n") + lines


def scenario_ticks(text: str) -> int:
    for line in text.splitlines():
        parts = line.split()
        if parts[:1] == ["ticks"] and len(parts) == 2 and parts[1].isdigit():
            return int(parts[1])
    raise Error("the scenario has no valid ticks line")


QUERY_IMPORT = "  i services.GetGafFrame "
UNREAD_Y = re.compile(r"^(  i los_radar\.GetMapCellAtPosition pos=\([^,)]*,)[^,)]*(,)")


def reduce_recording(text: str) -> tuple[list[tuple[int, str, str]], list[tuple[int, str, str]]]:
    """A recording without what the scenario route frees (#1209): its lines as (index, line, line as compared), where the y of
    a GetMapCellAtPosition position is masked, and its GetGafFrame answers as (index, arguments, answer text), taken out of the lines."""
    lines = text.split("\n")
    kept = []
    queries = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith(QUERY_IMPORT):
            arguments, _, result = line.partition(" = ")
            j = i + 1
            prefix = f"  o {result}." if result else None
            while j < len(lines) and (lines[j].startswith("    c ") or lines[j].startswith("  b ") or (prefix is not None and lines[j].startswith(prefix))):
                j += 1
            queries.append((i, arguments, "\n".join(lines[i:j])))
            i = j
            continue
        kept.append((i, line, UNREAD_Y.sub(r"\1*\2", line)))
        i += 1
    return kept, queries


def first_mismatch(stored: str, got: str) -> tuple[int, str] | None:
    """The first difference as (index of the line of `got` it is at, 'line N: A / B' or the query it is), or None when only freed things differ."""
    a, na = reduce_recording(stored)
    b, nb = reduce_recording(got)
    found = None
    for k in range(max(len(a), len(b))):
        x = a[k][2] if k < len(a) else None
        y = b[k][2] if k < len(b) else None
        if x != y:
            at = b[k][0] if k < len(b) else len(got.split("\n"))
            found = (at, f"line {at + 1}: {(a[k][1] if k < len(a) else '<end>')!r} / {(b[k][1] if k < len(b) else '<end>')!r}")
            break
    answers: dict[str, set[str]] = {}
    for _, arguments, answer in na:
        answers.setdefault(arguments, set()).add(answer)
    for at, arguments, answer in nb:
        if found is not None and at >= found[0]:
            break
        if arguments not in answers:
            return at, f"query services.GetGafFrame at line {at + 1}: no answer to these arguments: {arguments.strip()!r}"
        if answer not in answers[arguments]:
            return at, f"query services.GetGafFrame at line {at + 1}: the answer to {arguments.strip()!r} differs"
    return found


def first_difference(a: str, b: str) -> str | None:
    """The first line at which two recordings differ, as 'line N: A / B' (or the GetGafFrame query that has no recorded answer), or None when they are equal
    or differ only in what the scenario route frees: the order and count of GetGafFrame calls and the y of a GetMapCellAtPosition position."""
    if a == b:
        return None
    found = first_mismatch(a, b)
    return None if found is None else found[1]


def divergence(stored: str, got: str) -> dict:
    """The first difference of two recordings described from `got` alone: the line number, the tick and slot of the call it is in,
    and the kind, name and value of the line (None for a line `got` does not have). {"status": "equal"} when they are the same
    or differ only in what first_difference frees."""
    if stored == got:
        return {"status": "equal"}
    mismatch = first_mismatch(stored, got)
    if mismatch is None:
        return {"status": "equal"}
    at = mismatch[0]
    b = got.split("\n")
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
    loading = False
    caching = False
    for n, line in enumerate(lines[2:-1], 3):
        if line.startswith("call "):
            inside = True
        elif not inside and line.startswith("load "):
            loading = True
            caching = False
        elif not inside and loading and (line.startswith("  x ") or line.startswith("  bind ")):
            pass
        elif not inside and line.startswith("cache "):
            loading = False
            caching = True
        elif not inside and caching and line.startswith("  block "):
            pass
        elif not (inside and len(line) > 4 and line.startswith("  ") and line[2] in "rabciowd" and line[3] == " "):
            problems.append(f"line {n} is neither a call nor a line of one: {line!r}")
    return problems


def play_recording(exe: Path, scenario: Path, game_dir: str | None, native: bool, window: tuple[int, int] | dict[str, tuple[int, int]] | None,
                   directory: Path, subsystems: list[str], phase: frozenset[str] | set[str] = frozenset()) -> dict[str, str]:
    """Play one scenario with a record line per subsystem and return the text of each recording it wrote."""
    directory.mkdir(parents=True, exist_ok=True)
    copy = directory / scenario.name
    copy.write_text(with_record(scenario.read_text(), subsystems, window, phase))
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
            (ROOT / "build/scratch").mkdir(parents=True, exist_ok=True)
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
           out: Path, parallel: int, phase: bool = False) -> dict[tuple[str, str], str]:
    """Record the scenarios of the plan (scenario name -> subsystems) into `out`/<subsystem>/; the text by (subsystem, scenario)."""
    out.mkdir(parents=True, exist_ok=True)
    work = out / ".scenarios"
    results: dict[tuple[str, str], str] = {}
    stored = stored_windows()
    phases = stored_phases()

    def one(name: str) -> None:
        scenario = scenario_path(name)
        texts: dict[str, str] = {}
        for s in plan[name]:
            chosen = window if window is not None else stored.get(f"{s}/{scenario.stem}")
            whole = {s} if phase or f"{s}/{scenario.stem}" in phases else set()
            texts.update(play_recording(exe, scenario, game_dir, native, chosen, work / f"{scenario.stem}-{s}", [s], whole))
        for subsystem, text in texts.items():
            problems = check_form(text, subsystem)
            if problems:
                raise Error(f"{scenario.stem}: the {subsystem} recording is malformed: {problems[0]}")
            target = out / stored_recording(subsystem, scenario.stem).relative_to(RECORDINGS)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(text.encode("latin-1"))
            results[(subsystem, scenario.stem)] = text

    try:
        with ThreadPoolExecutor(parallel) as pool:
            list(pool.map(one, plan))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return results


def compare(a: Path, b: Path) -> list[str]:
    """The differences between two directories of recordings, one line each (empty when they are the same)."""
    names_a = [p.relative_to(a).as_posix() for p in stored_files(a) if len(p.relative_to(a).parts) > 1]
    names_b = [p.relative_to(b).as_posix() for p in stored_files(b) if len(p.relative_to(b).parts) > 1]
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
        for p in scenario_recordings(s):
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
    ap.add_argument("--phase", action="store_true", help="--record: record each subsystem's phase driver whole")
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
            results = record(exe, plan, args.game_dir, args.native_modules, window, args.out, args.parallel, args.phase)
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
                    stored = stored_recording(subsystem, scenario_path(name).stem)
                    got = (Path(tmp) / stored.relative_to(RECORDINGS)).read_text()
                    diff = first_difference(stored.read_text(), got) if stored.is_file() else "no stored recording"
                    print(f"{stored.relative_to(RECORDINGS).as_posix()}: {'equal' if diff is None else 'differs, ' + diff}")
                    bad += diff is not None
                    verdicts[recording_name(stored)] = divergence(stored.read_text(), got) if stored.is_file() else {"status": "differs", "line": 0}
            if args.json:
                args.json.parent.mkdir(parents=True, exist_ok=True)
                args.json.write_text(json.dumps(verdicts, indent=1, sort_keys=True) + "\n")
            return 1 if bad else 0
    except (Error, tracediff.Error) as e:
        print(f"replay: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
