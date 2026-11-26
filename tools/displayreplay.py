"""Replay the stored display lists and compare what they draw with the stored frames (docs/display-log.md, "The replay").

    uv run tools/displayreplay.py --check --game-dir ~/.ta                            # every scenario with a `frames` line
    uv run tools/displayreplay.py --check --game-dir ~/.ta ui-click-select            # some of them
    uv run tools/displayreplay.py --check --exe build/modern/TotalA.exe --game-dir ~/.ta
    uv run tools/displayreplay.py --kinds                                             # the kinds each stored list covers

For each scenario with a `frames` line, the exe plays the scenario in test mode with `-testreplay`: once the game has
loaded, at the first frame of tick 1 or later, it draws the scenario's stored list (reference/displaylists/<scenario>.txt.xz)
frame by frame from blank targets with the game's data and its own drawing functions, writes frames/<tick>.bin at the
scenario's frames ticks, and exits. Every frame it writes must equal reference/frames/<scenario>/<tick>.bin byte for byte,
and every item of the list must have been drawn. A difference is a hole in the log, not in the replay.

`--kinds` prints, for each kind of item, the stored lists that hold it, and exits 1 when a kind is in none of them and is
not one of LEFT_OUT. Exit status of --check: 0 every frame equal, 1 a difference, 2 an error.
"""

import argparse
import concurrent.futures
import contextlib
import lzma
import re
import shutil
import sys
import tempfile
from collections.abc import Callable
from pathlib import Path

import displaylog
import tracediff

ROOT = Path(__file__).resolve().parent.parent
SCENARIOS = ROOT / "reference/scenarios"
DISPLAYLISTS = ROOT / "reference/displaylists"
FRAMES = ROOT / "reference/frames"
LIST_SUFFIX = ".txt.xz"
REPLAY_FILE = "displayreplay.txt"
LOG_PREFIX = "testreplay: "
FRAME_HEADER_SIZE = 16
FRAME_PALETTE_SIZE = 1024
SECONDS = 900
LEFT_OUT: dict[str, str] = {}


class Error(Exception):
    pass


def scenario_frames(scenario: Path) -> list[int]:
    """The ticks of a scenario's `frames` line (none when it has no such line)."""
    for line in scenario.read_text().splitlines():
        words = line.split()
        if words and words[0] == "frames":
            return [int(w) for w in words[1:]]
    return []


def stored_list(name: str, directory: Path = DISPLAYLISTS) -> Path:
    return directory / f"{name}{LIST_SUFFIX}"


def read_list(path: Path) -> str:
    """The text of a stored display list (xz)."""
    return lzma.decompress(path.read_bytes()).decode("latin-1")


def pack_list(text: str) -> bytes:
    """A display list as it is stored: xz at the highest preset, which is the same bytes for the same text."""
    return lzma.compress(text.encode("latin-1"), preset=9 | lzma.PRESET_EXTREME)


def frame_size(frame: bytes) -> tuple[int, int]:
    if len(frame) < FRAME_HEADER_SIZE + FRAME_PALETTE_SIZE or frame[:4] != b"BTFR":
        raise Error("not a frame dump")
    return int.from_bytes(frame[6:8], "little"), int.from_bytes(frame[8:10], "little")


def frame_difference(stored: bytes, got: bytes) -> str | None:
    """How two frame dumps differ ("header", "palette", or the number of pixels with their box and the first), or None when equal."""
    width, _ = frame_size(stored)
    frame_size(got)
    if stored == got:
        return None
    if stored[:FRAME_HEADER_SIZE] != got[:FRAME_HEADER_SIZE]:
        return "header"
    palette = slice(FRAME_HEADER_SIZE, FRAME_HEADER_SIZE + FRAME_PALETTE_SIZE)
    if stored[palette] != got[palette]:
        return "palette"
    start = FRAME_HEADER_SIZE + FRAME_PALETTE_SIZE
    a, b = stored[start:], got[start:]
    if len(a) != len(b):
        return f"size: stored {len(stored)} bytes, got {len(got)}"
    count = 0
    first = None
    x0 = y0 = 1 << 30
    x1 = y1 = -1
    for offset, (p, q) in enumerate(zip(a, b)):
        if p != q:
            x, y = offset % width, offset // width
            if first is None:
                first = (x, y, p, q)
            count += 1
            x0, y0, x1, y1 = min(x0, x), min(y0, y), max(x1, x), max(y1, y)
    assert first is not None
    return (f"{count} pixels differ in x {x0}..{x1} y {y0}..{y1}, the first at x {first[0]} y {first[1]}: "
            f"stored {first[2]}, got {first[3]}")


def replay_notes(log: str) -> list[str]:
    """The replay's own lines of a run's log, without the prefix."""
    return [line[len(LOG_PREFIX):].strip() for line in log.splitlines() if line.startswith(LOG_PREFIX)]


def undrawn(notes: list[str]) -> list[str]:
    """The notes that name items the replay could not draw (`<n> <kind> items not drawn`)."""
    return [note for note in notes if re.fullmatch(r"\d+ .+ items not drawn", note)]


def judge(name: str, ticks: list[int], stored: dict[int, bytes | None], got: dict[int, bytes], notes: list[str]) -> str | None:
    """Why a scenario's replay fails (the first frame that differs, a frame missing, an item not drawn), or None when it passes."""
    for tick in ticks:
        want = stored.get(tick)
        if want is None:
            return f"frame {tick}: no stored frame in {FRAMES.relative_to(ROOT)}/{name}"
        if tick not in got:
            return f"frame {tick}: the replay wrote no frame" + (f" ({notes[-1]})" if notes else "")
        diff = frame_difference(want, got[tick])
        if diff:
            return f"frame {tick}: {diff}"
    left = undrawn(notes)
    if left:
        return "; ".join(left)
    return None


def run_log(text: str) -> str:
    """The game's log of the run that playtest.py output `text` names, or "" when there is none."""
    m = re.search(r"^logs and screenshots: (.+)/$", text, re.M)
    if not m:
        return ""
    logs = list((ROOT / m[1]).glob("*/wine.log"))
    return logs[0].read_text(errors="replace") if logs else ""


def replay(exe: Path, scenario: Path, listing: Path, game_dir: str | None) -> tuple[dict[int, bytes], list[str]]:
    """Play `scenario` with the display list `listing` replayed: (the frames written by tick, the replay's notes)."""
    ticks = scenario_frames(scenario)
    collect = tuple(f"frames/{tick}.bin" for tick in ticks)
    try:
        run = tracediff.play(exe, scenario, game_dir, None, SECONDS, put={REPLAY_FILE: listing}, collect=collect,
                             testreplay=REPLAY_FILE)
    except tracediff.Error as e:
        notes = replay_notes(run_log(str(e)))
        if notes:
            return {}, notes
        raise Error(f"{scenario.stem}: the replay did not run: {str(e).splitlines()[0]}") from None
    try:
        frames = {tick: (run / "frames" / f"{tick}.bin").read_bytes() for tick in ticks if (run / "frames" / f"{tick}.bin").is_file()}
        log = run / "wine.log"
        notes = replay_notes(log.read_text(errors="replace")) if log.is_file() else []
    finally:
        shutil.rmtree(run.parent, ignore_errors=True)
    return frames, notes


def framed(names: list[str] | None = None) -> dict[str, Path]:
    """The scenarios with a `frames` line, by name (only `names` when given; an Error for a name with no such scenario)."""
    paths = {p.stem: p for p in sorted(SCENARIOS.glob("*.txt"))}
    if names:
        missing = [n for n in names if n not in paths]
        if missing:
            raise Error(f"no scenario {', '.join(missing)} in {SCENARIOS.relative_to(ROOT)}")
        paths = {n: paths[n] for n in names}
    return {n: p for n, p in paths.items() if scenario_frames(p)}


def check(exe: Path, game_dir: str | None, scenarios: dict[str, Path], jobs: int = 4,
          slot: Callable[[], contextlib.AbstractContextManager] = contextlib.nullcontext) -> tuple[list[str], list[str]]:
    """(the scenarios whose replay fails, the report lines) for the scenarios given, each a scenario with a `frames` line."""
    lines: dict[str, str] = {}
    bad: list[str] = []
    scratch_root = ROOT / "build/scratch"
    scratch_root.mkdir(parents=True, exist_ok=True)

    def one(name: str, scenario: Path) -> tuple[str, str | None]:
        stored = stored_list(name)
        if not stored.is_file():
            return name, f"no stored list {stored.relative_to(ROOT)}"
        ticks = scenario_frames(scenario)
        with tempfile.TemporaryDirectory(prefix="replay-", dir=scratch_root) as scratch:
            listing = Path(scratch) / REPLAY_FILE
            listing.write_bytes(lzma.decompress(stored.read_bytes()))
            with slot():
                got, notes = replay(exe, scenario, listing, game_dir)
        want = {tick: (FRAMES / name / f"{tick}.bin").read_bytes() if (FRAMES / name / f"{tick}.bin").is_file() else None
                for tick in ticks}
        return name, judge(name, ticks, want, got, notes)

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        for name, diff in pool.map(lambda item: one(*item), scenarios.items()):
            lines[name] = f"DIFF {name} display replay {diff}" if diff else f"ok   {name} display replay ({len(scenario_frames(scenarios[name]))} frames)"
            if diff:
                bad.append(name)
    report = [lines[name] for name in scenarios]
    report.append(f"display replay: {len(scenarios) - len(bad)} of {len(scenarios)} scenarios draw their stored frames")
    return bad, report


def coverage(directory: Path = DISPLAYLISTS) -> dict[str, list[str]]:
    """For each kind of displaylog.KINDS, the stored lists that hold an item of it."""
    found: dict[str, list[str]] = {kind: [] for kind in displaylog.KINDS}
    for path in sorted(directory.glob(f"*{LIST_SUFFIX}")):
        for kind in displaylog.kinds(read_list(path)):
            found.setdefault(kind, []).append(path.name[:-len(LIST_SUFFIX)])
    return found


def coverage_report(found: dict[str, list[str]]) -> tuple[list[str], list[str]]:
    """(the kinds no stored list holds that are not LEFT_OUT, the report lines)."""
    missing = [kind for kind, names in found.items() if not names and kind not in LEFT_OUT]
    lines = []
    for kind, names in found.items():
        if names:
            lines.append(f"{kind}: {', '.join(names)}")
        elif kind in LEFT_OUT:
            lines.append(f"{kind}: left out ({LEFT_OUT[kind]})")
        else:
            lines.append(f"{kind}: in no stored list")
    return missing, lines


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("scenarios", nargs="*", help="scenario names (default: every scenario with a frames line)")
    ap.add_argument("--check", action="store_true", help="replay the stored lists and compare with the stored frames")
    ap.add_argument("--kinds", action="store_true", help="print the stored lists that hold each kind of item")
    ap.add_argument("--exe", type=Path, default=ROOT / "build/modern/TotalA.exe", help="the exe that replays (default: the modern build)")
    ap.add_argument("--game-dir", help="the game data, as for tools/playtest.py")
    ap.add_argument("--jobs", type=int, default=4, help="scenarios replayed at once (default 4)")
    args = ap.parse_args()
    if args.check == args.kinds:
        ap.error("give --check or --kinds")
    try:
        if args.kinds:
            missing, lines = coverage_report(coverage())
            print("\n".join(lines))
            return 1 if missing else 0
        if not args.exe.is_file():
            raise Error(f"no exe {args.exe}")
        bad, lines = check(args.exe, args.game_dir, framed(args.scenarios), args.jobs)
        print("\n".join(lines))
        return 1 if bad else 0
    except (Error, tracediff.Error, OSError, lzma.LZMAError) as e:
        print(f"displayreplay: {e}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
