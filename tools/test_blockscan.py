"""Plays scenarios with the exe's block pointer scan checking itself: `uv run tools/test_blockscan.py [--game-dir DIR] [SCENARIO ...]`.

Each run passes -blockscanverify, which scans every block a second time byte by byte, against spans rebuilt from the
registry, and fails the run unless both scans find the same rows.
"""

import argparse
import shutil
import sys
from pathlib import Path

import tracediff

ROOT = Path(__file__).resolve().parent.parent
EXE = ROOT / "build/modern/TotalA.exe"
SCENARIOS = ("skirmish2", "orders-air")
SAME = "the fast scan and the plain scan found the same rows"


def verify(exe: Path, name: str, game_dir: str | None) -> str | None:
    """None when the scan of the scenario agreed with the plain scan, else why not."""
    scenario = ROOT / "reference/scenarios" / f"{name}.txt"
    try:
        run = tracediff.play(exe, scenario, game_dir, None, timeout=1800, blockscan=8, blockscan_verify=True)
    except tracediff.Error as e:
        return str(e).strip().splitlines()[-1]
    try:
        log = (run / "wine.log").read_text(errors="replace")
        return None if SAME in log else f"{name}: the log has no line saying the scans agreed"
    finally:
        shutil.rmtree(run.parent, ignore_errors=True)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("scenarios", nargs="*", default=SCENARIOS)
    ap.add_argument("--exe", type=Path, default=EXE)
    ap.add_argument("--game-dir")
    args = ap.parse_args()
    failed = 0
    for name in args.scenarios:
        problem = verify(args.exe.resolve(), name, args.game_dir)
        print(f"{name}: {'ok' if problem is None else 'FAILED: ' + problem}", flush=True)
        failed += problem is not None
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
