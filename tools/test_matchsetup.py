"""Play the scenarios of tests/matchsetup/ under Wine and check what the match setup (docs/state.md, "The match setup") promises.

    uv run tools/test_matchsetup.py [--exe build/modern/TotalA.exe] [--game-dir ~/.ta] [--native-modules]

- block-large: console-units with a 128 MiB block equals the stored trace of console-units.
- block-roundtrip: orders-move-attack with a 128 MiB block, copied and attached at tick 200, equals its stored trace.
- steps-high, steps-low: a limit above and below the default's 1333, each played twice with identical traces, and different from each other.
- setup-save, setup-load: the save keeps the block size and the limit, and the loaded game plays them.
- hpa-derived: the demo pathfinder's region makes the engine size a block above 32 MiB.
- net-setup: a game_type 3 match plays the setup the host's PlayerInfo carries.

It prints one `ok` or `FAIL` line per check, then `<n> of 15 ok`. A check that compares traces fails when either has no ticks.
`tools/fullgate.py run` plays it nightly, after the coverage check.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

import tracedigest
from tracediff import parse_trace

ROOT = Path(__file__).resolve().parent.parent

SCENARIOS = ROOT / "tests" / "matchsetup"
FLOOR = 32 << 20
CHECKS = 15


def play(options, name, put=(), collect=()):
    """Plays tests/matchsetup/<name>.txt and returns (run directory, wine log text)."""
    command = ["uv", "run", str(ROOT / "tools" / "playtest.py"), "--exe", options.exe, "--game-dir", options.game_dir,
               "--testmode", str(SCENARIOS / f"{name}.txt"), "--testmode-timeout", "900"]
    if options.native_modules:
        command.append("--native-modules")
    for dest, source in put:
        command += ["--put", dest, str(source)]
    for path in collect:
        command += ["--collect", path]
    out = subprocess.run(command, capture_output=True, text=True, cwd=ROOT)
    match = re.search(r"in (build/playtest/\S+)", out.stdout)
    if out.returncode != 0 or match is None:
        sys.exit(f"{name}: the run failed\n{out.stdout}{out.stderr}")
    run = next(p for p in (ROOT / match.group(1)).iterdir() if p.is_dir())
    return run, (run / "wine.log").read_text(errors="replace")


def setup_line(log):
    found = re.findall(r"match setup block_bytes (\d+) search_steps (\d+)", log)
    return (int(found[-1][0]), int(found[-1][1])) if found else None


def digest(run):
    return digest_of(run / "trace.txt")


def digest_of(path):
    """The (tick, digest, state, view) rows of a trace.txt."""
    return tracedigest.digests(parse_trace(path)[1])


def stored_of(name):
    """The (tick, digest, state, view) rows of reference/traces/<name>.txt."""
    return tracedigest.parse_digest(ROOT / "reference" / "traces" / f"{name}.txt")[1]


def check(ok, what, traces=()):
    """Prints ok or FAIL for `what`; a check that compares traces fails when any of them has no ticks."""
    empty = [i for i, trace in enumerate(traces) if not trace]
    if empty:
        ok, what = False, f"{what} (no ticks in trace {', '.join(str(i + 1) for i in empty)} of {len(traces)})"
    print(("ok   " if ok else "FAIL ") + what)
    if ok:
        check.passed += 1
    else:
        check.failed = True


check.failed = False
check.passed = 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", default="build/modern/TotalA.exe")
    parser.add_argument("--game-dir", default=str(Path.home() / ".ta"))
    parser.add_argument("--native-modules", action="store_true")
    options = parser.parse_args()

    run, log = play(options, "block-large")
    played, stored = digest(run), stored_of("console-units")
    check(played == stored, "block-large: a 128 MiB block plays the stored trace of console-units", (played, stored))
    check(setup_line(log) == (128 << 20, 20), "block-large: the match played with the 128 MiB block (and `search 20` from the script)")

    run, log = play(options, "block-roundtrip")
    played, stored = digest(run), stored_of("orders-move-attack")
    check(played == stored, "block-roundtrip: the copy of the grown block continues to the stored trace", (played, stored))

    traces = {}
    for name, limit in (("steps-high", 3000), ("steps-low", 600)):
        first, log = play(options, name)
        second, _ = play(options, name)
        traces[name], again = digest(first), digest(second)
        check(setup_line(log) == (FLOOR, limit), f"{name}: the match played with {limit} search steps and the 32 MiB block")
        check(traces[name] == again, f"{name}: two runs have identical traces", (traces[name], again))
    high, low = traces["steps-high"], traces["steps-low"]
    first_differing = next((a[0] for a, b in zip(high, low) if a != b), None)
    check(high != low, f"the two limits play differently (first differing tick {first_differing})", (high, low))

    run, log = play(options, "setup-save", collect=["savegame/harness.sav", "savegame/harness.opt"])
    check(setup_line(log) == (96 << 20, 3000), "setup-save: played with a 96 MiB block and 3000 steps")
    save, options_file = run / "savegame" / "harness.sav", run / "savegame" / "harness.opt"
    check(save.exists() and options_file.exists(), "setup-save: wrote a save and its options")
    loaded = []
    for _ in range(2):
        run2, log2 = play(options, "setup-load", put=[("savegame/harness.sav", save), ("savegame/harness.opt", options_file)])
        loaded.append(digest(run2))
        check(setup_line(log2) == (96 << 20, 3000), "setup-load: the loaded game plays the saved block size and limit")
    check(loaded[0] == loaded[1], "setup-load: two loads have identical traces", loaded)

    run, log = play(options, "hpa-derived")
    line = setup_line(log)
    check(line is not None and line[0] > FLOOR and line[0] % 65536 == 0, f"hpa-derived: the demo's region sized the block above 32 MiB ({line})")

    run, log = play(options, "net-setup")
    check(setup_line(log) == (96 << 20, 3000), "net-setup: game_type 3 plays the block size and the limit from the host's PlayerInfo")
    print(f"{check.passed} of {CHECKS} ok")
    sys.exit(1 if check.failed or check.passed != CHECKS else 0)


if __name__ == "__main__":
    main()
