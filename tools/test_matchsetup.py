"""Play the scenarios of tests/matchsetup/ under Wine and check what the match setup (docs/state.md, "The match setup") promises.

    uv run tools/test_matchsetup.py [--exe build/modern/TotalA.exe] [--game-dir ~/.ta] [--native-modules]

- block-large: console-units with a 128 MiB block equals the stored trace of console-units.
- block-roundtrip: orders-move-attack with a 128 MiB block, copied and attached at tick 200, equals its stored trace.
- steps-high, steps-low: a limit above and below the default's 1333, each played twice with identical traces, and different from each other.
- setup-save, setup-load: the save keeps the block size and the limit, and the loaded game plays them.
- hpa-derived: the demo pathfinder's region makes the engine size a block above 32 MiB.
"""

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SCENARIOS = ROOT / "tests" / "matchsetup"
FLOOR = 32 << 20


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
    run = next((ROOT / match.group(1)).iterdir())
    return run, (run / "wine.log").read_text(errors="replace")


def setup_line(log):
    found = re.findall(r"match setup block_bytes (\d+) search_steps (\d+)", log)
    return (int(found[-1][0]), int(found[-1][1])) if found else None


def digest(run):
    return tracedigest_lines(run / "trace.txt")


def tracedigest_lines(path):
    result = subprocess.run(["uv", "run", str(ROOT / "tools" / "tracedigest.py"), str(path), "--commit", "x", "--scenario", "x"],
                            capture_output=True, text=True, cwd=ROOT, check=True)
    return [line for line in result.stdout.splitlines() if re.match(r"\d+ [0-9a-f]{16}$", line)]


def stored_of(name):
    return [line for line in (ROOT / "reference" / "traces" / f"{name}.txt").read_text().splitlines() if re.match(r"\d+ [0-9a-f]{16}$", line)]


def check(ok, what):
    print(("ok   " if ok else "FAIL ") + what)
    if not ok:
        check.failed = True


check.failed = False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", default="build/modern/TotalA.exe")
    parser.add_argument("--game-dir", default=str(Path.home() / ".ta"))
    parser.add_argument("--native-modules", action="store_true")
    options = parser.parse_args()

    run, log = play(options, "block-large")
    stored = stored_of("console-units")
    check(digest(run) == stored, "block-large: a 128 MiB block plays the stored trace of console-units")
    check(setup_line(log) == (128 << 20, 20), "block-large: the match played with the 128 MiB block (and `search 20` from the script)")

    run, log = play(options, "block-roundtrip")
    check(digest(run) == stored_of("orders-move-attack"), "block-roundtrip: the copy of the grown block continues to the stored trace")

    traces = {}
    for name, limit in (("steps-high", 3000), ("steps-low", 600)):
        first, log = play(options, name)
        second, _ = play(options, name)
        traces[name] = digest(first)
        check(setup_line(log) == (FLOOR, limit), f"{name}: the match played with {limit} search steps and the 32 MiB block")
        check(traces[name] == digest(second), f"{name}: two runs have identical traces")
    check(traces["steps-high"] != traces["steps-low"], "the two limits play differently")

    run, log = play(options, "setup-save", collect=["savegame/harness.sav", "savegame/harness.opt"])
    check(setup_line(log) == (96 << 20, 3000), "setup-save: played with a 96 MiB block and 3000 steps")
    save, options_file = run / "savegame" / "harness.sav", run / "savegame" / "harness.opt"
    check(save.exists() and options_file.exists(), "setup-save: wrote a save and its options")
    loaded = []
    for _ in range(2):
        run2, log2 = play(options, "setup-load", put=[("savegame/harness.sav", save), ("savegame/harness.opt", options_file)])
        loaded.append(digest(run2))
        check(setup_line(log2) == (96 << 20, 3000), "setup-load: the loaded game plays the saved block size and limit")
    check(loaded[0] == loaded[1], "setup-load: two loads have identical traces")

    run, log = play(options, "hpa-derived")
    line = setup_line(log)
    check(line is not None and line[0] > FLOOR and line[0] % 65536 == 0, f"hpa-derived: the demo's region sized the block above 32 MiB ({line})")
    sys.exit(1 if check.failed else 0)


if __name__ == "__main__":
    main()
