"""The conformity gate: play the scenarios and compare their digests with reference/traces/.

    uv run tools/reference.py --check --exe build/modern/TotalA.exe    # every scenario: the gate of the changes in reference/README.md, "Gate"
    uv run tools/reference.py --check --exe build/modern/TotalA.exe --tier     # the pull request gate: the scenarios of GATE_TIER
    uv run tools/reference.py --traces          # rewrite reference/traces/ from the reference branch

`--check --exe` plays every scenario in reference/scenarios/ (`--parallel N` at a
time, default half the CPUs and at most 6 (10 with `--both-modes`), under Wine through tools/playtest.py)
and compares the digest of each tick (tools/tracedigest.py) with the stored one. It also compares
each run's sound.txt with reference/sounds/<scenario>.txt, line by line, when that file exists (docs/sound-log.md), each run's ui.txt with reference/ui/<scenario>.txt the same way (docs/ui-log.md), the decoded interface records of each run's commands.txt with reference/commands/<scenario>.txt the same way (tools/interfacelog.py), and the digest of each run's events.txt with reference/events/<scenario>.txt the same way (docs/events-log.md), naming the first differing record by playing the reference build.
Each run's display.txt is compared, line by line, with reference/display/<scenario>.txt, and for a scenario with a `frames` line its displaylist.txt with reference/displaylists/<scenario>.txt frame by frame (docs/display-log.md, tools/displaylog.py); when a digest differs at a frame with no stored list, the first differing item is named by playing the reference build (tools/tracediff.py `--display`).
A scenario with a `frames` line also has its frame dumps compared, byte for byte, with reference/frames/<scenario>/ (docs/frame-dump.md); a missing stored frame fails.
Its view files (views/<tick>.bin, the player view of each frame tick, docs/view.md "The recorded view") are compared the same way with reference/views/<scenario>/, naming the first field that differs (tools/viewblock.py); the report line is `views: K of M scenarios equal, ...`, and a missing stored view fails.
It also compares the harness.sav of each scenario with a `save` line with
reference/saves/<scenario>.sav, when that file exists, by decoded contents (tools/savefile.py leaves out the bytes taken from the clock or from uninitialised memory).
Each scenario with `input` lines is played again from its own run's commands.txt (`<scenario>+commands`, docs/commands.md,
section 2.2, and reference/README.md, "Interface commands"): it must give the stored traces and decode to the stored interface
records, except that the traces of a scenario of REPLAY_LEFT_OUT are reported and not judged.
A run that ends before the stored trace does is reported as `EARLY <scenario>: ended at tick T of N (<exit code, signal or timeout>)`, with
the host's fatal line if the run left one, and fails the check without running tracediff (reference/README.md).
On the first difference it builds the reference from the commit the trace
records, prints tools/tracediff.py's report for that scenario and exits 1. A
missing or extra trace or scenario exits 1. `--tier` restricts `--check` to the
scenarios named in GATE_TIER, the pull request gate (tools/coverage.py --tier checks that they cover enough).

`--check --end-of-tick` judges each scenario on the `state` column of the digest files alone, the hash of the `tick` row (the whole
judged state at the end of the tick), not on the digest of all the rows of the tick. It is the verdict for a re-implementation that keeps the
end-of-tick contract with its own order inside the tick (#1037). Each scenario's line then ends with ` [digest differs from tick T]` when its
digest column differs, and with ` [view differs from tick T]` when its `view` column (the hash of the `view` row) differs; neither fails it, and
`K of M scenarios equal on every row` follows the total. The view files of the frames are compared as in a full check and reported, but a
difference does not fail an end-of-tick check: the state stays the verdict.
The sim event digests (`bt-event-digest 2`) are then compared on their `tick` lines alone, and the first differing record is named under the same rule (tools/tracediff.py `--events --end-of-tick`).
The display digests and lists are compared on frame numbers, ticks, counts and `set`, and on items as sorted `<layer> <item>` lines (tools/displaylog.py `--end-of-tick`).

`--check --both-modes` plays every scenario once in WASM and once with `-modules=native` in one pool (each line names its
mode, the sound logs and block pointer scans are kept apart per mode) and passes only when both modes equal the stored traces.
An exe with accesslog.attr beside it (an access-log build, tools/accesslog.py) is played without the block pointer scan, and
the check prints `block pointers: not scanned, access-log build` instead of the block pointer report.
Every game run of a `--check` holds one of the machine's run slots (files bt-gate-slot-<i> in $XDG_RUNTIME_DIR, else /tmp, each
taken with flock; half the CPUs and at most GATE_SLOTS of them, $BT_GATE_SLOTS overrides), so that all the gates on a machine together
play at most that many games: gates run at the same time and fill the slots one gate's longest scenario leaves idle, where one
exclusive lock made them queue. A run that waits for a slot prints once which gates hold them (`<pid> (<worktree>)`).
A second `--check` with the same cache key waits for the first on a key lock (bt-gate-key-<key>.lock) and then reports its cached pass.
`--no-lock` takes no slot and no key lock. A scenario with a `load` line starts as soon as the scenario it loads has finished.
A passing `--check` stores its output under ~/.cache/bt-gate/<key>.txt, the key being a SHA-256 of the exe, the files beside it
(wasmtime_host.dll, *.wasm, modules.json), reference/{scenarios,traces,sounds,ui,commands,events,display,displaylists,frames,views,saves}, the four tools that play and compare
(commit.txt is left out) and the options; a later `--check` with the same key, looked up before the lock and again after it, prints
`cached pass from <date> at <commit>` and the stored lines and exits 0. `--no-cache` plays anyway.

Every scenario is played with new game allocations zeroed (the game's own -memset=0 switch, through
tools/playtest.py --testmode). `--check --native-modules` runs the exe's modules as native code (-modules=native) instead of in WASM. `--check --poison` fills them with 0xCD instead (-memset=0xcdcdcdcd); a
table that reads uninitialised bytes may then differ from the stored digests, and the report lists those.

`--check --exe <exe> --quirks <spec>` plays every scenario with `quirks <spec>` added and prints, per
scenario, equal, differs (the first differing tick and phase) or failed. A scenario that differs is
played again with every flag off and compared with that run's phases. A difference is not a failure:
the exit status is 1 only when a scenario failed, or its all-off rerun failed or differed from the
stored trace. It builds no reference and is not the gate.

`--check --exe <exe> --roundtrip T[,T...]` (or `auto`: early, middle and last tick minus one of each scenario) adds `roundtrip T`
lines to every scenario: at the end of tick T the run copies the sim block and continues from the copy at another base address
(modules run natively; with WASM modules the block is scrambled and restored in place). The traces must equal the stored ones,
and the block pointers are scanned and checked as in any full `--check`.

`--traces` checks out origin/reference in build/reference/branch, runs that
branch's own `tools/reference.py HEAD --traces` there (it links the matching
build with the original compiler and plays the scenarios), and copies the
traces it wrote into reference/traces/, the sound logs it wrote into reference/sounds/ (if it wrote any), the UI logs it wrote into reference/ui/, the interface commands it wrote into reference/commands/, the sim event logs it wrote into reference/events/ (if it wrote any), the display digests and lists it wrote into reference/display/ and reference/displaylists/ (if it wrote any), the
frames it wrote into reference/frames/ (if it wrote any), the views of those frames (views/<tick>.bin) into reference/views/ (if it wrote any) and the saves it wrote into reference/saves/ (if it wrote any).
Set WINEARCH=win64 for Wine; `--game-dir` names the game's
install.
"""

import argparse
import ast
import contextlib
import fcntl
import functools
import hashlib
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time
from collections.abc import Callable
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path

import blockpointers
import displaylog
import displayreplay
import eventdigest
import interfacelog
import playtest
import savefile
import tracediff
import tracedigest
import viewblock

ROOT = Path(__file__).resolve().parent.parent
OUTPUTS = ("TotalA.exe", "SMACKW32.DLL", "commit.txt")
SCENARIOS = ROOT / "reference/scenarios"
TRACES = ROOT / "reference/traces"
SOUNDS = ROOT / "reference/sounds"
SOUND_FILE = "sound.txt"
UIS = ROOT / "reference/ui"
UI_FILE = "ui.txt"
COMMANDS = ROOT / "reference/commands"
COMMANDS_FILE = "commands.txt"
EVENTS = ROOT / "reference/events"
EVENTS_FILE = "events.txt"
SETUPS = ROOT / "reference/setup"
SETUP_FILE = "setup.txt"
READBACK_SUFFIX = "-readback"
DISPLAYS = ROOT / "reference/display"
DISPLAY_FILE = "display.txt"
DISPLAYLISTS = ROOT / "reference/displaylists"
DISPLAYLIST_FILE = "displaylist.txt"
UI_PREFIX = "ui-"
UI_EFFECTS = ("group", "groupsel")
FRAMES = ROOT / "reference/frames"
FRAME_HEADER_SIZE = 16
FRAME_PALETTE_SIZE = 1024
VIEWS = ROOT / "reference/views"
VIEWS_DIR = tracediff.VIEWS_DIR
SAVES = ROOT / "reference/saves"
SAVE_FILE = tracediff.SAVE_FILES[0]
MAX_PARALLEL = 6
MAX_PARALLEL_BOTH_MODES = 10
GATE_SLOTS = 10
GATE_TIER = (
    "allied-victory",
    "commander-death",
    "conditions-dark-side",
    "console-economy",
    "console-features",
    "console-sight",
    "console-units",
    "eight-full-moon",
    "hailstorm-los-off",
    "load-midgame",
    "load-targets",
    "mission-build-unit",
    "mission-capture",
    "mission-defeat-passes-x",
    "mission-kill-mobile",
    "mission-kill-of-type",
    "mission-passes-x",
    "mission-passes-z",
    "mission-radius",
    "mission-victory",
    "orders-air",
    "orders-air-fights",
    "orders-air-repair",
    "orders-build-patrol",
    "orders-guard-attacked",
    "orders-guard-reclaim",
    "orders-immediate",
    "orders-tier2-core",
    "orders-tier2-spider",
    "orders-tier2-vader",
    "save-midgame",
    "save-targets",
    "sensors-floating-teeth-los-off",
    "share-all",
    "ui-build-menu",
    "ui-build-place",
    "ui-debug-keys",
    "ui-dialogs",
    "ui-groups",
    "ui-keys",
    "ui-left-click-orders",
    "ui-scroll-groups",
    "ui-share-chat",
    "units-transport-ship",
)
RUN_SECONDS_BASE = 120
DEFAULT_TICKS_PER_SECOND = 25
RUN_TICKS_PER_SECOND = DEFAULT_TICKS_PER_SECOND
RUN_TICKS_PER_SECOND_SCAN_EVERY_TICK = 6
RUN_ATTEMPTS = 2
GATE_SCAN_EVERY = 8
FUEL_FILE = "fuel.txt"
FUEL: dict[str, dict[str, tuple[int, int]]] = {}
SOUND_LOGS: dict[str, str | None] = {}
UI_LOGS: dict[str, str | None] = {}
INTERFACE_LOGS: dict[str, str | None] = {}
COMMAND_TEXTS: dict[str, str | None] = {}
REPLAY_SUFFIX = "+commands"
REPLAY_LEFT_OUT: dict[str, str] = {}
EVENT_LOGS: dict[str, str | None] = {}
SETUP_LOGS: dict[str, str | None] = {}
DISPLAY_LOGS: dict[str, str | None] = {}
DISPLAY_LISTS: dict[str, str | None] = {}
FRAME_DUMPS: dict[str, dict[int, bytes]] = {}
VIEW_DUMPS: dict[str, dict[int, bytes]] = {}
SAVE_BYTES: dict[str, bytes | None] = {}
MODES = ("wasm", "native")
SLOT_FILE = "bt-gate-slot-{}"
KEY_LOCK_FILE = "bt-gate-key-{}.lock"
SLOTS_ENV = "BT_GATE_SLOTS"
SLOT_POLL_SECONDS = 0.25
CACHE_ENV = "BT_GATE_CACHE"
KEY_TREES = ("reference/scenarios", "reference/traces", "reference/sounds", "reference/ui", "reference/commands", "reference/events", "reference/display", "reference/displaylists", "reference/frames", "reference/views", "reference/saves", "reference/setup",
            "tools/playtest")
KEY_TOOLS = ("tools/reference.py", "tools/playtest.py", "tools/tracediff.py", "tools/tracedigest.py", "tools/savefile.py",
             "tools/blockpointers.py", "tools/eventdigest.py", "tools/displaylog.py", "tools/displayreplay.py", "tools/tracerehash.py",
             "tools/interfacelog.py")
KEY_DATA = ("data/block_pointers.csv", "docs/sim-events.md")
KEY_RUNTIME_FILES = ("wasmtime_host.dll", "modules.json")
RETRY_DIALOG = "dialog-directx"
WATCH_SECONDS = 0.5
STOP_SECONDS = 30


def reference_env(environ: dict[str, str], root: Path) -> dict[str, str]:
    env = {k: v for k, v in environ.items() if k != "VIRTUAL_ENV"}
    pin = root / ".python-version"
    if pin.is_file() and "UV_PYTHON" not in env:
        env["UV_PYTHON"] = pin.read_text().strip()
    return env


REF_ENV = reference_env(os.environ, ROOT)


def git(*args: str, cwd: Path = ROOT, check: bool = True, capture: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(["git", "-C", str(cwd), *args], check=check, text=True,
                          capture_output=capture)


def fail(message: str) -> None:
    sys.exit(f"reference: {message}")


def resolve(ref: str) -> str:
    """The commit a ref names."""
    proc = git("rev-parse", "--verify", "-q", f"{ref}^{{commit}}", check=False, capture=True)
    if proc.returncode:
        fail(f"{ref} names no commit")
    return proc.stdout.strip()


def registered_worktrees() -> set[Path]:
    out = git("worktree", "list", "--porcelain", capture=True).stdout
    return {Path(line[len("worktree "):]).resolve() for line in out.splitlines() if line.startswith("worktree ")}


def prepare_worktree(directory: Path, sha: str) -> None:
    if directory.resolve() in registered_worktrees() and not (directory / ".git").exists():
        removed = git("worktree", "remove", "--force", str(directory), check=False, capture=True)
        if removed.returncode and directory.resolve() in registered_worktrees():
            sys.stderr.write(removed.stderr)
            fail(f"cannot repair the stale worktree registration for {directory}")
    if (directory / ".git").exists():
        git("checkout", "-q", "-f", "--detach", sha, cwd=directory)
        git("clean", "-fdq", cwd=directory)
    else:
        directory.parent.mkdir(parents=True, exist_ok=True)
        git("worktree", "add", "-q", "--detach", str(directory), sha)

    link = directory / "toolchain"
    link.unlink(missing_ok=True)
    link.symlink_to(ROOT / "toolchain")
    (directory / "orig").mkdir(exist_ok=True)
    (directory / "build").mkdir(exist_ok=True)
    for name, target in (("orig/TotalA.exe", ROOT / "orig/TotalA.exe"),
                         ("build/ghidra", ROOT / "build/ghidra")):
        if target.exists():
            link = directory / name
            link.unlink(missing_ok=True)
            link.symlink_to(target)


def build(directory: Path, sha: str) -> None:
    proc = subprocess.run(["uv", "run", "tools/link.py", "--carve"], cwd=directory, env=REF_ENV)
    if proc.returncode:
        fail(f"tools/link.py --carve failed ({proc.returncode})")
    for name in ("TotalA.exe", "SMACKW32.DLL"):
        shutil.copyfile(directory / "build/link" / name, directory / name)
    (directory / "commit.txt").write_text(sha + "\n")


def clock(seconds: float) -> str:
    m, s = divmod(int(round(seconds)), 60)
    return f"{m}:{s:02d}"


def scenario_files() -> dict[str, Path]:
    return {p.stem: p for p in sorted(SCENARIOS.glob("*.txt"))}


def default_parallel(both_modes: bool = False) -> int:
    """Half the CPUs, at least 1 and at most MAX_PARALLEL (MAX_PARALLEL_BOTH_MODES when both modes are played)."""
    return max(1, min(MAX_PARALLEL_BOTH_MODES if both_modes else MAX_PARALLEL, (os.cpu_count() or 2) // 2))


def scenario_ticks(scenario: Path) -> int:
    """The scenario's `ticks` value, or a tracediff.Error if it has none that parses."""
    for line in scenario.read_text().splitlines():
        words = line.split()
        if words and words[0] == "ticks":
            try:
                return int(words[1], 0)
            except (IndexError, ValueError):
                break
    raise tracediff.Error(f"{scenario}: the scenario has no valid ticks line")


def scenario_frames(scenario: Path) -> list[int]:
    """The ticks of the scenario's `frames` line (none when it has no such line)."""
    for line in scenario.read_text().splitlines():
        words = line.split()
        if words and words[0] == "frames":
            try:
                return [int(word) for word in words[1:]]
            except ValueError:
                raise tracediff.Error(f"{scenario}: the frames line has a tick that is not a number")
    return []


DIGEST = 1
STATE = 2
VIEW = 3


def difference(want: tuple[dict[str, str], list[tuple[int, str, str, str]]], header: dict[str, str],
               digests: list[tuple[int, str, str, str]], column: int = DIGEST) -> str | None:
    """Why a run differs from the stored (header, digests) in `column` (DIGEST or STATE), or None when it equals them."""
    want_header, want_digests = want
    diff = first_difference(want_digests, digests, column)
    seeds = [k for k in ("game_seed", "crt_seed", "view_seed") if want_header.get(k) != header[k]]
    if seeds:
        diff = diff or "the scenario's seeds differ from the stored trace: " + ", ".join(seeds)
    return diff


def keep_run(run: Path, scenario: str, suffix: str = "", logs: bool = False, trace: bytes = b"") -> Path:
    """Copy the run's trace.txt (and dump.txt and blockptrs.txt, if any) to build/scratch/check-<scenario>-<time>/.

    With `logs`, also the run's result.json, crash.txt and Wine's stderr (wine.log), which hold the host's last words.
    `trace` is the trace text already read from the run, kept as trace.txt when the run's directory no longer has one.
    """
    kept = ROOT / "build/scratch" / f"check-{scenario}{suffix}-{time.strftime('%Y%m%d-%H%M%S')}"
    kept.mkdir(parents=True, exist_ok=True)
    names = ["trace.txt", SOUND_FILE, UI_FILE, COMMANDS_FILE, EVENTS_FILE, DISPLAY_FILE, DISPLAYLIST_FILE, "dump.txt", "blockptrs.txt"]
    if logs:
        names += ["result.json", "crash.txt", *sorted(p.name for p in run.glob("wine*.log"))]
    for name in names:
        if (run / name).is_file():
            shutil.copy2(run / name, kept / name)
    if trace and not (kept / "trace.txt").is_file():
        (kept / "trace.txt").write_bytes(trace)
    return kept


HOST_MESSAGE = re.compile(r"trapped|out of fuel|fatal|statics|Unhandled exception|page fault|purecall", re.I)


class EarlyExit(tracediff.Error):
    """A run that ended before its trace did; str() is the report line."""


def trace_end(path: Path) -> tuple[int, bool]:
    """The last tick that a trace.txt holds whole, and whether its last line is cut (the file does not end in a newline).

    A cut line belongs to its tick when it starts with that tick's number and a space, and that tick is then not whole.
    """
    data = path.read_bytes()
    cut = bool(data) and not data.endswith(b"\n")
    body, _, fragment = data.rpartition(b"\n") if cut else (data, b"", b"")
    last = 0
    for line in reversed(body[-4096:].decode(errors="replace").splitlines()):
        row = tracediff.parse_row(line)
        if row:
            last = row[0]
            break
    if cut and fragment.split(b" ")[0] == str(last).encode() and b" " in fragment:
        last -= 1
    return last, cut


def exit_description(run: Path) -> str:
    """How the game ended, from the run's result.json: `timeout`, `signal SIGKILL`, `exit code 2` or `exit status unknown`."""
    try:
        result = json.loads((run / "result.json").read_text())
    except (OSError, ValueError):
        return "exit status unknown"
    if "still runs after" in str(result.get("reason", "")):
        return "timeout"
    codes = [c for c in result.get("exit_codes", []) if c is not None]
    if not codes:
        return "exit status unknown"
    code = codes[0]
    if code < 0:
        try:
            return f"signal {signal.Signals(-code).name}"
        except ValueError:
            return f"signal {-code}"
    return f"exit code {code}" + (f" (0x{code:X})" if code > 255 else "")


def host_message(run: Path) -> str | None:
    """The first line of the run's crash.txt or wine logs that looks like the host's fatal message, cut to 200 characters, or None."""
    for name in ("crash.txt", *sorted(p.name for p in run.glob("wine*.log"))):
        try:
            lines = (run / name).read_text(errors="replace").splitlines()
        except OSError:
            continue
        for line in lines:
            if HOST_MESSAGE.search(line):
                return f"{name}: {line.strip()[:200]}"
    return None


def early_exit(run: Path, key: str, end: int, kept: Path | None) -> EarlyExit:
    """The EarlyExit for a run that ended at the last whole tick of its trace.txt, short of `end`; `kept` is where its files are kept."""
    last, _ = trace_end(run / "trace.txt")
    text = f"EARLY {key}: ended at tick {last} of {end} ({exit_description(run)})"
    host = host_message(run)
    if host:
        text += f", host said {host}"
    return EarlyExit(text + (f" [kept: {kept.relative_to(ROOT)}]" if kept else ""))


def keep_failed_attempt(error: Exception, scenario: str) -> Path | None:
    """Copy the playtest.py session a failed attempt left (its wine.log, steps.log, summary.md, trace.txt and crash.txt)
    to build/scratch/check-<scenario>-failed-<time>/, or None when the error names no session."""
    m = re.search(r"^logs and screenshots: (.+)/$", str(error), re.M)
    if not m or not (ROOT / m[1]).is_dir():
        return None
    kept = ROOT / "build/scratch" / f"check-{scenario}-failed-{time.strftime('%Y%m%d-%H%M%S')}"
    shutil.copytree(ROOT / m[1], kept, dirs_exist_ok=True)
    return kept


def failed_early(error: Exception, scenario: str, key: str, end: int) -> EarlyExit | None:
    """The EarlyExit for a failed run whose session (named in `error`) holds a trace that stops short of tick `end`, else None.

    The session is kept as keep_failed_attempt does.
    """
    m = re.search(r"^logs and screenshots: (.+)/$", str(error), re.M)
    if not m or not (ROOT / m[1]).is_dir():
        return None
    runs = [d for d in (ROOT / m[1]).iterdir() if (d / "trace.txt").is_file()]
    if len(runs) != 1 or trace_end(runs[0] / "trace.txt")[0] >= end:
        return None
    kept = keep_failed_attempt(error, scenario)
    return early_exit(runs[0], key, end, kept)


def should_retry(message: str) -> bool:
    """Whether a failed run is tried again: an exe crash or the DirectX init dialog, not a hang or another dialog.

    playtest.py names an error dialog's kind (dialog-directx, dialog-crt, ...) in the failure. The DirectX
    box is only classified so before the first tick; the runtime error (R6025) is a real fault.
    """
    kind = re.search(r"dialog-\w+", message)
    if kind:
        return kind[0] == RETRY_DIALOG
    return "still runs after" not in message


def parse_fuel(path: Path) -> dict[str, tuple[int, int]]:
    """The most fuel one tick burned in a run, by module: {module: (fuel, tick)}; empty when the run wrote no fuel.txt (native modules)."""
    if not path.is_file():
        return {}
    lines = path.read_text().splitlines()
    if not lines or lines[0] != "bt-fuel 1":
        raise tracediff.Error(f"{path}: not a bt-fuel 1 file")
    found = {}
    for line in lines[1:]:
        module, fuel, tick = line.split("\t")
        found[module] = (int(fuel), int(tick))
    return found


def report_fuel(exe: Path, scenarios: dict[str, Path], fuel_by_name: dict[str, dict[str, tuple[int, int]]] | None = None,
                native: bool | None = None, label: str = "") -> int:
    """Print the most fuel one tick burned, per scenario and module, and the largest per module; 1 if a WASM run reported none.

    `fuel_by_name` is the fuel of the runs to report by scenario name (default: FUEL), `native` whether they ran as native code (default: BT_NATIVE_MODULES).
    """
    fuel = FUEL if fuel_by_name is None else fuel_by_name
    if (os.environ.get("BT_NATIVE_MODULES") == "1" if native is None else native) or not playtest.runs_wasm_modules(exe):
        print(label + "fuel: not counted (the modules ran as native code)")
        return 0
    missing = [name for name in scenarios if not fuel.get(name)]
    for name in scenarios:
        for module, (burned, tick) in sorted(fuel.get(name, {}).items()):
            print(f"{label}fuel {name}: {module} {burned} at tick {tick}")
    for name in missing:
        print(f"{label}fuel {name}: no {FUEL_FILE}: the run counted no fuel")
    largest: dict[str, tuple[int, str, int]] = {}
    for name in scenarios:
        for module, (burned, tick) in fuel.get(name, {}).items():
            if module not in largest or burned > largest[module][0]:
                largest[module] = (burned, name, tick)
    for module, (burned, name, tick) in sorted(largest.items()):
        print(f"{label}fuel max: {module} {burned} per tick ({name}, tick {tick}) over {len(scenarios) - len(missing)} scenarios")
    return 1 if missing else 0


def tag(name: str, mode: str | None) -> str:
    """The name of a scenario's run in `mode`: `name [mode]`, or just `name` when only one mode is played."""
    return name if mode is None else f"{name} [{mode}]"


def untag(key: str) -> str:
    """The scenario of a run's name (see tag)."""
    return key.partition(" [")[0]


def mode_of(key: str) -> str | None:
    """The mode of a run's name (see tag), or None for a plain scenario name."""
    return key.partition(" [")[2].rstrip("]") or None


def mode_view(store: dict, scenarios: dict[str, Path], mode: str | None) -> dict:
    """The entries of a per-run store (keyed as tag does) that belong to `mode`, keyed by the plain scenario name."""
    return {name: store[tag(name, mode)] for name in scenarios if tag(name, mode) in store}


def replay_name(name: str) -> str:
    """The name of the run of scenario `name` played from the commands.txt of its own run (docs/commands.md, section 2.2)."""
    return name + REPLAY_SUFFIX


def replay_source(name: str) -> str | None:
    """The scenario whose commands.txt the run `name` plays (see replay_name), or None for a scenario's own run."""
    return name[:-len(REPLAY_SUFFIX)] if name.endswith(REPLAY_SUFFIX) else None


def with_replays(scenarios: dict[str, Path]) -> dict[str, Path]:
    """The runs from commands.txt of the scenarios with `input` lines, by replay_name, each with its scenario's file.

    A scenario with `frontend 1` has none: a run from the file delivers no `input` lines, so its menus never start
    the match.
    """
    return {replay_name(name): path for name, path in scenarios.items()
            if interfacelog.has_input(path.read_text()) and not scenario_frontend(path)}


def default_modes(both_modes: bool) -> tuple[str | None, ...]:
    """The modes a check plays each scenario in: both of MODES, or one run per scenario (mode None, set by BT_NATIVE_MODULES)."""
    return MODES if both_modes else (None,)


def lock_dir() -> Path:
    """The directory of the run slots and key locks: $XDG_RUNTIME_DIR, else /tmp."""
    return Path(os.environ.get("XDG_RUNTIME_DIR") or "/tmp")


def gate_slots() -> int:
    """The number of run slots on the machine: $BT_GATE_SLOTS, else half the CPUs, at least 1 and at most GATE_SLOTS."""
    return max(1, int(os.environ.get(SLOTS_ENV) or min(GATE_SLOTS, (os.cpu_count() or 2) // 2)))


def key_lock_path(key: str) -> Path:
    """The file that one gate key's runs are serialised on."""
    return lock_dir() / KEY_LOCK_FILE.format(key[:16])


def cache_dir() -> Path:
    """The directory of cached passes, shared by every worktree on the machine: $BT_GATE_CACHE, else ~/.cache/bt-gate."""
    return Path(os.environ.get(CACHE_ENV) or Path.home() / ".cache/bt-gate")


def hash_tree(digest, root: Path, relative: str) -> None:
    """Feed the paths and contents of every file under root/relative to digest, in path order."""
    base = root / relative
    for path in sorted(base.rglob("*")) if base.is_dir() else []:
        if path.is_file():
            digest.update(f"{path.relative_to(root).as_posix()}\0".encode())
            digest.update(path.read_bytes())
            digest.update(b"\0")


def local_imports(root: Path, relative: str) -> set[str]:
    """The tools/ modules (paths relative to root) that the sources in relative import, directly or through each other."""
    found: set[str] = set()
    pending = [relative]
    while pending:
        path = root / pending.pop()
        try:
            tree = ast.parse(path.read_bytes())
        except (OSError, SyntaxError, ValueError):
            continue
        for node in ast.walk(tree):
            names = [a.name for a in node.names] if isinstance(node, ast.Import) else \
                [node.module or ""] if isinstance(node, ast.ImportFrom) and not node.level else []
            for name in names:
                module = f"tools/{name.partition('.')[0]}.py"
                if (root / module).is_file() and module not in found:
                    found.add(module)
                    pending.append(module)
    return found


def gate_key(exe: Path, options: dict[str, object], root: Path = ROOT) -> str:
    """The SHA-256 of what decides a --check's result: the exe, the runtime files and modules beside it, the stored
    scenarios, traces, logs and frames, the tools that play and compare and every tools/ module they import, the block pointer
    table, the exe's .map, and the options (commit.txt is left out)."""
    digest = hashlib.sha256()
    beside = sorted([exe.parent / n for n in KEY_RUNTIME_FILES] + list(exe.parent.glob("*.wasm")))
    for path in [exe] + beside:
        digest.update(f"{path.name}\0".encode())
        digest.update(path.read_bytes() if path.is_file() else b"-")
        digest.update(b"\0")
    for relative in KEY_TREES:
        hash_tree(digest, root, relative)
    imported = set().union(*(local_imports(root, relative) for relative in KEY_TOOLS))
    for relative in sorted({*KEY_TOOLS, *imported}):
        digest.update(f"{relative}\0".encode())
        digest.update((root / relative).read_bytes())
        digest.update(b"\0")
    for path in [root / relative for relative in KEY_DATA] + [exe.with_suffix(".map")]:
        digest.update(f"{path.name}\0".encode())
        digest.update(path.read_bytes() if path.is_file() else b"-")
        digest.update(b"\0")
    for name in sorted(options):
        digest.update(f"{name}={options[name]!r}\0".encode())
    return digest.hexdigest()


def cached_pass(key: str) -> str | None:
    """The stored report of a passing run of key (its first lines are `date`, `commit`), or None."""
    path = cache_dir() / f"{key}.txt"
    return path.read_text() if path.is_file() else None


def store_pass(key: str, output: str, commit: str, now: datetime | None = None) -> Path:
    """Write the report of a passing run under key: the date, the commit, then the output."""
    directory = cache_dir()
    directory.mkdir(parents=True, exist_ok=True)
    date = (now or datetime.now(timezone.utc)).strftime("%Y-%m-%d %H:%M:%S UTC")
    path = directory / f"{key}.txt"
    temporary = path.with_name(f"{path.name}.{os.getpid()}")
    temporary.write_text(f"date: {date}\ncommit: {commit}\n{output}")
    os.replace(temporary, path)
    return path


def report_cached(stored: str) -> None:
    """Print `cached pass from <date> at <commit>` and the stored output."""
    lines = stored.split("\n", 2)
    date = lines[0].removeprefix("date: ")
    commit = lines[1].removeprefix("commit: ")
    print(f"cached pass from {date} at {commit}")
    sys.stdout.write(lines[2] if len(lines) > 2 else "")


class Tee:
    """A stdout that also keeps what was written to it."""

    def __init__(self, stream):
        self.stream = stream
        self.parts: list[str] = []

    def write(self, text: str) -> int:
        self.parts.append(text)
        return self.stream.write(text)

    def flush(self) -> None:
        self.stream.flush()

    def __getattr__(self, name: str):
        return getattr(self.stream, name)

    def text(self) -> str:
        return "".join(self.parts)


def open_lock_file(path: Path) -> int:
    """Open (creating, world-writable) a lock file."""
    fd = os.open(path, os.O_RDWR | os.O_CREAT, 0o666)
    try:
        os.fchmod(fd, 0o666)
    except OSError:
        pass
    return fd


def holder_of(path: Path) -> str:
    """`<pid> (<worktree>)` of the process whose entry is in a lock file, or `another process` while it has not written it."""
    for _ in range(20):
        try:
            text = path.read_text().strip()
        except OSError:
            text = ""
        if text:
            pid, _, worktree = text.partition(" ")
            return f"{pid} ({worktree})"
        time.sleep(0.1)
    return "another process"


def take(fd: int, worktree: Path) -> None:
    """Record this process in a lock file it holds."""
    os.ftruncate(fd, 0)
    os.write(fd, f"{os.getpid()} {worktree}\n".encode())


class KeyLock:
    """An exclusive lock on one file, held for a whole --check, so that a second gate of the same key waits and then finds the first's cached pass.

    The holder writes its pid and worktree into the file; a second process prints who holds it and waits.
    """

    def __init__(self, path: Path, worktree: Path | None = None):
        self.path = path
        self.worktree = worktree or ROOT
        self.fd: int | None = None

    def __enter__(self) -> "KeyLock":
        self.fd = open_lock_file(self.path)
        try:
            fcntl.flock(self.fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print(f"waiting for the gate run of {holder_of(self.path)}", flush=True)
            fcntl.flock(self.fd, fcntl.LOCK_EX)
        take(self.fd, self.worktree)
        return self

    def __exit__(self, *exc) -> None:
        if self.fd is not None:
            os.ftruncate(self.fd, 0)
            os.close(self.fd)
            self.fd = None


class RunSlots:
    """The machine's run slots: `count` files in `directory`, of which every run of every gate holds one (`flock`) for its whole run.

    All gates on the machine together play at most `count` games, and a slot is released when its holder dies.
    A process that has to wait prints, once, which gates hold the slots.
    """

    def __init__(self, directory: Path | None = None, count: int | None = None, worktree: Path | None = None):
        self.directory = directory or lock_dir()
        self.count = count or gate_slots()
        self.worktree = worktree or ROOT
        self.told = False
        self.guard = threading.Lock()

    def paths(self) -> list[Path]:
        return [self.directory / SLOT_FILE.format(i) for i in range(self.count)]

    def tell(self) -> None:
        with self.guard:
            if self.told:
                return
            self.told = True
        holders = dict.fromkeys(holder_of(p) for p in self.paths() if p.exists() and p.read_text().strip())
        print(f"waiting for a run slot, held by the gates of {', '.join(holders) or 'other processes'}", flush=True)

    def acquire(self) -> int:
        """Block until a slot is free, hold it and return its file descriptor."""
        paths = self.paths()
        start = os.getpid() % len(paths)
        while True:
            for i in range(len(paths)):
                fd = open_lock_file(paths[(start + i) % len(paths)])
                try:
                    fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                except BlockingIOError:
                    os.close(fd)
                    continue
                take(fd, self.worktree)
                return fd
            self.tell()
            time.sleep(SLOT_POLL_SECONDS)

    @contextlib.contextmanager
    def slot(self):
        """Hold one slot for the body."""
        fd = self.acquire()
        try:
            yield
        finally:
            os.ftruncate(fd, 0)
            os.close(fd)


GATE_RUN_SLOTS: RunSlots | None = None


def run_slot():
    """The context of one run: a slot of GATE_RUN_SLOTS, or nothing while the gate takes none (--no-lock)."""
    return GATE_RUN_SLOTS.slot() if GATE_RUN_SLOTS is not None else contextlib.nullcontext()


GAME_TRACE_SLACK = 1.0


def game_trace(pid: int, started: float) -> Path | None:
    """The trace.txt that the playtest.py process `pid`, started at `started` (time.time()), has its exe write, or None before the run has a game directory.

    A directory named for an earlier session (a killed playtest.py whose pid came round again) is skipped. The name
    holds the session's local time to the second, truncated, so it can read up to a second before the real time.
    """
    work = playtest.OUT / "work"
    if not work.is_dir():
        return None
    found = []
    for d in work.iterdir():
        m = re.match(rf"(\d{{8}}-\d{{6}})-{pid}-", d.name)
        if m and datetime.strptime(m[1], "%Y%m%d-%H%M%S").timestamp() >= started - GAME_TRACE_SLACK:
            found.append(d)
    found.sort()
    return found[-1] / "i1/game/trace.txt" if found else None


@dataclass
class LiveRun:
    """A scenario's running playtest.py process and how far its trace.txt has been read."""
    proc: subprocess.Popen
    scenario: str = ""
    path: Path | None = None
    offset: int = 0
    seen: int = 0
    started: float = field(default_factory=time.time)
    tail: tracedigest.TraceTail = field(default_factory=tracedigest.TraceTail)
    read: bytearray = field(default_factory=bytearray)


@dataclass
class Found:
    """The first run seen to differ from its stored trace (`name` is the run's name, see tag)."""
    name: str
    diff: str
    ticks: int
    elapsed: float
    kept: Path | None
    early: bool = False
    trace: Path | None = None
    tick: int = 0
    stored: str = ""
    got: str = ""


class Watch:
    """Compares each running scenario's trace.txt, as it grows, with the stored digests, and stops every run at the first difference."""

    def __init__(self, expected: dict[str, tuple], locate: Callable[[int, float], Path | None] | None = None,
                 column: int = DIGEST):
        self.expected = expected
        self.column = column
        self.locate = locate or game_trace
        self.lock = threading.Lock()
        self.halted = threading.Event()
        self.found: Found | None = None
        self.live: dict[str, LiveRun] = {}

    def started(self, name: str) -> Callable[[subprocess.Popen], None]:
        """The callback that tracediff.play calls with the run's process; `name` is the run's name (see tag)."""
        def register(proc: subprocess.Popen) -> None:
            with self.lock:
                self.live[name] = LiveRun(proc, untag(name))
                if self.halted.is_set():
                    self.stop(proc)
        return register

    def finished(self, name: str) -> None:
        with self.lock:
            self.live.pop(name, None)

    def poll(self) -> None:
        """Read what each running scenario has written since the last poll, and halt at the first digest that differs."""
        with self.lock:
            live = list(self.live.items())
        for name, run in live:
            if self.halted.is_set():
                return
            try:
                self.check(name, run)
            except (tracediff.Error, OSError):
                continue

    def check(self, name: str, run: LiveRun) -> None:
        run.path = run.path or self.locate(run.proc.pid, run.started)
        if run.path is None:
            return
        with run.path.open("rb") as f:
            f.seek(run.offset)
            data = f.read()
        run.offset += len(data)
        run.read += data
        want = self.expected[run.scenario][1]
        for got in run.tail.feed(data):
            index = run.seen
            run.seen += 1
            diff = first_difference(want[index:index + 1], [got], self.column)
            if diff:
                self.halt(name, diff, run.seen, time.time() - run.started, keep_run(run.path.parent, name, trace=bytes(run.read)),
                          trace=run.path,
                          digest=(want[index][0], want[index][self.column], got[self.column]) if index < len(want) else None)
                return

    def halt(self, name: str, diff: str, ticks: int, elapsed: float, kept: Path | None, early: bool = False,
             trace: Path | None = None, digest: tuple[int, str, str] | None = None) -> None:
        """Record the first difference (or, with `early`, early exit, `diff` being its report line) and stop every running scenario (the first call wins).

        `trace` is the file that the watch read, and `digest` the (tick, stored, got) of the difference.
        """
        with self.lock:
            if self.found:
                return
            self.found = Found(name, diff, ticks, elapsed, kept, early, trace, *(digest or (0, "", "")))
            self.halted.set()
            procs = [r.proc for r in self.live.values()]
        for proc in procs:
            self.stop(proc)

    def stop(self, proc: subprocess.Popen) -> None:
        """Ask a run's playtest.py to stop (it ends its own Wine processes), and kill it if it is still there after STOP_SECONDS."""
        if proc.poll() is not None:
            return
        try:
            proc.terminate()
        except OSError:
            return
        timer = threading.Timer(STOP_SECONDS, lambda: proc.poll() is None and proc.kill())
        timer.daemon = True
        timer.start()


def recheck(found: Found, column: int = DIGEST) -> str | None:
    """After a halt, what the kept trace.txt holds at the tick that the watch found different in `column`: a line to print, or None when the file agrees with the watch."""
    if found.early or not found.kept or not found.stored:
        return None
    if not (found.kept / "trace.txt").is_file():
        return f"recheck: no trace.txt was kept in {found.kept.name}: the run's directory was gone before it was copied"
    try:
        data = (found.kept / "trace.txt").read_bytes()
        done = {row[0]: row[column] for row in tracedigest.TraceTail().feed(data)}
    except (OSError, tracedigest.Error):
        return f"recheck: cannot read {found.kept.name}/trace.txt"
    if found.tick not in done:
        return f"recheck: the kept trace does not reach tick {found.tick}"
    if done[found.tick] == found.stored:
        return (f"the watch read a different value than the file now holds: tick {found.tick}, stored {found.stored}, "
                f"the watch read {found.got} from {found.trace}, the kept file holds {done[found.tick]}")
    return None


def scenario_frontend(scenario: Path) -> bool:
    """Whether the scenario has a `frontend 1` line, so that the menus set up its match and its setup log is stored."""
    values = [line.split()[1:] for line in scenario.read_text().splitlines() if line.split()[:1] == ["frontend"]]
    for value in values:
        if value not in (["0"], ["1"]):
            sys.exit(f"{scenario}: frontend takes 0 or 1, not {' '.join(value) or 'nothing'}")
    return ["1"] in values


def scenario_saves(scenario: Path) -> bool:
    """Whether the scenario has a `save` line, so that its run writes harness.sav."""
    return any(line.split()[:1] == ["save"] for line in scenario.read_text().splitlines())


def run_timeout(ticks: int, scan_every_tick: bool = False) -> float:
    """The seconds a run of `ticks` ticks may take before it counts as a hang, at RUN_TICKS_PER_SECOND (--ticks-per-second)."""
    if scan_every_tick:
        return RUN_SECONDS_BASE + ticks / (RUN_TICKS_PER_SECOND_SCAN_EVERY_TICK * RUN_TICKS_PER_SECOND / DEFAULT_TICKS_PER_SECOND)
    return RUN_SECONDS_BASE + ticks / RUN_TICKS_PER_SECOND


def play_scenario(exe: Path, scenario: Path, game_dir: str | None,
                  want: tuple[dict[str, str], list[tuple[int, str, str, str]]] | None = None, poison: bool = False,
                  keep_always: bool = False, scans: dict[str, str | None] | None = None,
                  scan_every: int = 1, retry_any: bool = False, save_to: Path | None = None,
                  load_from: Path | None = None, watch: "Watch | None" = None,
                  mode: str | None = None, column: int = DIGEST, name: str | None = None,
                  testcommands: Path | None = None) -> tuple[dict[str, str], list[tuple[int, str, str, str]], float, Path | None]:
    """Run the scenario under Wine: the trace header, the digest of each tick, the seconds taken.

    With `want` (the stored header and digests), a run that differs is kept: the last element is the
    directory holding its trace.txt, otherwise None. With `keep_always` the run is kept whatever it did.

    A run whose exe crashes (a nonzero exit) is tried once more, since the reference exe has faulted
    at startup in FlipScreen with a NULL primary surface (2 in about 1150 runs, cause not found, #120),
    and so is one that stopped on the DirectX initialisation dialog before
    tick 1 (startup under load). A run that is still going at the timeout is a hang, and a run that
    stopped on any other error dialog (R6025 and the like) is a game fault: both fail at once.

    With `scans` (a dict) the run has the block pointer scan on (every `scan_every` ticks), and its blockptrs.txt text (None if the run
    wrote none) is stored there under the scenario's name before the run directory is removed. A scan of every
    tick is slow (about 20 ticks a second at the end of a long run), so its timeout uses
    RUN_TICKS_PER_SECOND_SCAN_EVERY_TICK, scaled as --ticks-per-second scales the other. With `retry_any` a failed run is tried once more whatever the failure.

    With `save_to` (a directory), each of tracediff.SAVE_FILES the run wrote in its game copy is copied there by its
    file name before the run's directory is removed (nothing is copied for a file the run wrote none of). With
    `load_from` (a directory), its files are placed in the run's game copy at those paths before the exe starts.

    With `watch`, the run's process is registered there (see Watch), and a run that differs at its end halts the watch.
    `column` (DIGEST or STATE) is the column of the digests that decides whether the run differs.

    With `mode` ("wasm" or "native"), the run's modules run in that mode whatever BT_NATIVE_MODULES says, and its
    sound log, frames, save, scan and fuel are stored under tag(scenario, mode).

    With `testcommands` (a commands.txt), the run plays that file's commands instead of the scenario's, and `name` (the
    run's name in place of the scenario's, see replay_name) keys what it stores. The commands.txt of a run of a scenario
    with `input` lines is stored in COMMAND_TEXTS under its key.
    """
    stem = name or scenario.stem
    key = tag(stem, mode)
    ticks = scenario_ticks(scenario)
    timeout = run_timeout(ticks, scans is not None and scan_every == 1)
    end = want[1][-1][0] if want is not None and want[1] else ticks
    t0 = time.time()
    for attempt in range(RUN_ATTEMPTS):
        try:
            frame_files = tuple(f"frames/{tick}.bin" for tick in scenario_frames(scenario))
            view_files = tuple(f"{VIEWS_DIR}/{tick}.bin" for tick in scenario_frames(scenario))
            extra: dict = {"collect": (FUEL_FILE, SOUND_FILE, UI_FILE, COMMANDS_FILE, EVENTS_FILE, DISPLAY_FILE, DISPLAYLIST_FILE, SETUP_FILE, *frame_files, *view_files, *dict.fromkeys(
                (*(tracediff.SAVE_FILES if save_to is not None else ()), *((SAVE_FILE,) if scenario_saves(scenario) else ()))))}
            if watch is not None:
                extra["started"] = watch.started(key)
            if mode is not None:
                extra["env"] = dict(os.environ, BT_NATIVE_MODULES="1" if mode == "native" else "0")
            if load_from is not None:
                extra["put"] = tracediff.save_put(load_from, stem, load_from.name)
            if testcommands is not None:
                extra["testcommands"] = testcommands
            run = tracediff.play(exe, scenario, game_dir, None, timeout, poison, scan_every if scans is not None else 0, **extra)
            break
        except tracediff.Error as e:
            if (watch is not None and watch.halted.is_set()) or attempt == RUN_ATTEMPTS - 1 or not (retry_any or should_retry(str(e))):
                early = failed_early(e, stem, key, end) if want is not None and not (watch is not None and watch.halted.is_set()) else None
                if early is not None:
                    if watch is not None:
                        watch.halt(key, str(early), 0, time.time() - t0, None, early=True)
                    raise early from e
                raise
            failed = keep_failed_attempt(e, stem)
            where = f" [failed attempt kept: {failed.relative_to(ROOT)}]" if failed else ""
            print(f"{stem}: run failed, trying again: {last_lines(e)}{where}", flush=True)
    elapsed = time.time() - t0
    if watch is not None:
        watch.finished(key)
    kept = None
    try:
        if save_to is not None:
            tracediff.copy_save(run, save_to)
        FUEL[key] = parse_fuel(run / FUEL_FILE)
        sound_log = run / SOUND_FILE
        SOUND_LOGS[key] = sound_log.read_text() if sound_log.is_file() else None
        ui_log = run / UI_FILE
        UI_LOGS[key] = ui_log.read_text() if ui_log.is_file() else None
        commands_log = run / COMMANDS_FILE
        INTERFACE_LOGS[key] = (
            interfacelog.decode(commands_log.read_text(), trace_schema()) if commands_log.is_file() else None)
        if testcommands is None and scenario.is_file() and interfacelog.has_input(scenario.read_text()):
            COMMAND_TEXTS[key] = commands_log.read_text() if commands_log.is_file() else None
        events_log = run / EVENTS_FILE
        EVENT_LOGS[key] = event_digest(events_log)
        setup_log = run / SETUP_FILE
        SETUP_LOGS[key] = setup_log.read_text() if setup_log.is_file() else None
        display_log = run / DISPLAY_FILE
        DISPLAY_LOGS[key] = display_log.read_text() if display_log.is_file() else None
        display_list = run / DISPLAYLIST_FILE
        DISPLAY_LISTS[key] = display_list.read_text() if display_list.is_file() else None
        FRAME_DUMPS[key] = {tick: (run / "frames" / f"{tick}.bin").read_bytes()
                            for tick in scenario_frames(scenario) if (run / "frames" / f"{tick}.bin").is_file()}
        VIEW_DUMPS[key] = {tick: (run / VIEWS_DIR / f"{tick}.bin").read_bytes()
                           for tick in scenario_frames(scenario) if (run / VIEWS_DIR / f"{tick}.bin").is_file()}
        if scenario_saves(scenario):
            saved = run / SAVE_FILE
            SAVE_BYTES[key] = saved.read_bytes() if saved.is_file() else None
        trace = tracediff.need(run, "trace.txt")
        last, cut = trace_end(trace)
        if cut or (want is not None and last < end):
            kept = keep_run(run, stem, f"-{mode}" if mode else "", logs=True)
            early = early_exit(run, key, end, kept)
            if watch is not None:
                watch.halt(key, str(early), last, elapsed, kept, early=True)
            raise early
        header, rows = tracediff.parse_trace(trace)
        for seed in ("game_seed", "crt_seed", "view_seed"):
            if seed not in header:
                raise tracediff.Error(f"{scenario}: the trace has no {seed} line")
        digests = tracedigest.digests(rows)
        if scans is not None:
            scanned = run / "blockptrs.txt"
            scans[key] = scanned.read_text() if scanned.is_file() else None
        diff = difference(want, header, digests, column) if want is not None else None
        if keep_always or diff:
            kept = keep_run(run, stem, ("-alloff" if keep_always else "") + (f"-{mode}" if mode else ""))
        if watch is not None and diff:
            watch.halt(key, diff, len(digests), elapsed, kept)
    finally:
        shutil.rmtree(run.parent, ignore_errors=True)
    return header, digests, elapsed, kept


def last_lines(error: Exception, count: int = 1) -> str:
    """The last `count` non-empty lines of an error's message, each cut to 200 characters."""
    lines = [line[:200] for line in str(error).splitlines() if line.strip()]
    return "\n    ".join(lines[-count:]) or type(error).__name__


def with_savers(scenarios: dict[str, Path], available: dict[str, Path]) -> dict[str, Path]:
    """`scenarios` plus every scenario of `available` that one of them loads, so a loader is never played without its saver."""
    chosen = dict(scenarios)
    for path in scenarios.values():
        saver = tracediff.scenario_load(path)
        if saver in available:
            chosen[saver] = available[saver]
    return chosen


def play_all(exe: Path, scenarios: dict[str, Path], game_dir: str | None, parallel: int,
             expected: dict[str, tuple] | None = None, poison: bool = False,
             failed: dict[str, str] | None = None, keep_always: bool = False,
             scans: dict[str, str | None] | None = None, scan_every: int = 1,
             retry_any: bool = False, watch: Watch | None = None, modes: tuple[str | None, ...] = (None,),
             column: int = DIGEST) -> dict[str, tuple]:
    """Every scenario's play_scenario result, `parallel` at a time and longest first; exits 1 if one fails to run.

    With `modes` of more than the default (None), each scenario is played once per mode in the same pool, and the
    results, `failed`, `scans` and the stores of play_scenario are keyed by tag(scenario, mode); a loader is played
    in its saver's mode.

    With `failed` (a dict), a scenario that fails to run is recorded there as name -> message and play_all
    returns without exiting. With `keep_always`, every run's trace.txt is kept (see play_scenario). With `scans`,
    every run has the block pointer scan on (every `scan_every` ticks) and leaves its blockptrs.txt there; `retry_any` is passed on to play_scenario.

    Each run has its own session directory, Wine prefix and game directory (tools/playtest.py
    names them by its pid; a test-mode run is headless, with no X display or audio sink), so runs share nothing but the template prefix,
    which they only hardlink. With `expected`, a scenario that differs keeps its trace (see play_scenario).
    With `watch`, scenarios not yet started are skipped once the watch halts, and the runs it stopped are not failures.
    `column` is passed on to play_scenario.

    A run named by replay_name plays the commands.txt that its scenario's own run in the same mode wrote (COMMAND_TEXTS),
    and starts as soon as that run has finished, as a loader does; it fails without running when that run failed or
    wrote none. A run from commands.txt of a scenario in REPLAY_LEFT_OUT is played without `expected` and `watch`, so
    that its known difference neither keeps its trace nor stops the other runs.

    The longest scenarios start first; a scenario with a `load` line starts as soon as the scenario it names has finished
    (ahead of the scenarios not yet started), in its mode and with any `parallel`: the scenario it names saves a game (the `save` key), the run's savegame/harness.sav and savegame/harness.opt are copied out of its game
    copy into a temporary directory, and put into the loading run's own game copy. A loader whose saver is not among
    `scenarios` (see with_savers), failed to run, or wrote no save fails without running, with a message naming both.
    """
    playtest.template_prefix()
    results: dict[str, tuple] = {}
    failures: list[str] = []
    errors: dict[str, str] = {}
    keys = [tag(name, mode) for mode in modes for name in scenarios]
    loads = {tag(name, mode): (tag(saver, mode) if (saver := tracediff.scenario_load(path)) is not None else None)
             for mode in modes for name, path in scenarios.items()}
    sources = {tag(name, mode): tag(source, mode) for mode in modes for name in scenarios
               if (source := replay_source(name)) is not None}
    after = {key: sources.get(key) or loads[key] for key in keys}
    (ROOT / "build/scratch").mkdir(parents=True, exist_ok=True)
    saves = Path(tempfile.mkdtemp(prefix="saves-", dir=ROOT / "build/scratch"))

    def load_problem(key: str) -> str | None:
        saver = loads[key]
        if saver is None:
            return None
        if saver not in keys:
            return f"{key}: load {saver}: {saver} is not among the scenarios played"
        if saver in errors:
            return f"{key}: load {saver}: {saver} failed to run"
        try:
            tracediff.save_put(saves / saver, key, saver)
        except tracediff.Error as e:
            return str(e)
        return None

    def commands_file(key: str) -> Path:
        source = sources[key]
        text = COMMAND_TEXTS.get(source)
        if source not in keys or source in errors or text is None:
            raise tracediff.Error(f"{key}: {source} failed to run or wrote no {COMMANDS_FILE}")
        path = saves / (re.sub(r"[^A-Za-z0-9+-]", "_", key) + ".txt")
        path.write_text(text)
        return path

    def one(key: str) -> None:
        if watch is not None and watch.halted.is_set():
            return
        name, mode = untag(key), mode_of(key)
        try:
            problem = load_problem(key)
            if problem:
                raise tracediff.Error(problem)
            saver = loads[key]
            replay = {"name": name, "testcommands": commands_file(key)} if key in sources else {}
            listed = replay_source(name) in REPLAY_LEFT_OUT
            with run_slot():
                results[key] = play_scenario(exe, scenarios[name], game_dir, None if listed else (expected or {}).get(name), poison, keep_always, scans, scan_every, retry_any,
                                             save_to=saves / key if key in loads.values() else None,
                                             load_from=saves / saver if saver else None,
                                             **({"watch": watch} if watch is not None and not listed else {}),
                                             **({"mode": mode} if mode is not None else {}),
                                             **({"column": column} if column != DIGEST else {}), **replay)
        except (tracediff.Error, OSError) as e:
            if watch is not None and watch.halted.is_set():
                return
            errors[key] = str(e) if isinstance(e, EarlyExit) else last_lines(e, 5)
            if failed is not None:
                failed[key] = errors[key]
            elif isinstance(e, EarlyExit):
                failures.append(str(e))
            else:
                failures.append(f"{key}: failed to run, playtest.py ended with:\n    {last_lines(e, 5)}")

    def longest_first(key: str) -> int:
        try:
            return -scenario_ticks(scenarios[untag(key)])
        except tracediff.Error:
            return 0

    ordered = sorted(keys, key=longest_first)
    cond = threading.Condition()
    pending = [k for k in ordered if after[k] is None]
    waiting = [k for k in ordered if after[k] is not None]
    ready: list[str] = []
    finished: set[str] = set()
    running = 0
    crashes: list[BaseException] = []

    def promote() -> None:
        """Move the loaders and runs from commands.txt whose saver or source has finished (or is not played at all) from `waiting` to `ready`; the caller holds `cond`."""
        for k in list(waiting):
            if after[k] not in keys or after[k] in finished:
                waiting.remove(k)
                ready.append(k)

    def worker() -> None:
        nonlocal running
        while True:
            with cond:
                while True:
                    promote()
                    if ready or pending:
                        break
                    if not running and not waiting:
                        return
                    if not running:
                        ready.extend(waiting)
                        waiting.clear()
                        break
                    cond.wait()
                key = ready.pop(0) if ready else pending.pop(0)
                running += 1
            try:
                one(key)
            except BaseException as e:
                crashes.append(e)
            finally:
                with cond:
                    running -= 1
                    finished.add(key)
                    cond.notify_all()

    try:
        threads = [threading.Thread(target=worker) for _ in range(parallel)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        if crashes:
            raise crashes[0]
    finally:
        shutil.rmtree(saves, ignore_errors=True)
    if failures:
        fail("scenarios failed to run:\n" + "\n".join(failures))
    return results


def with_steps_per_frame(scenarios: dict[str, Path], steps: int, directory: Path) -> dict[str, Path]:
    """Copies of the scenarios in `directory`, each ending with a steps_per_frame line (a later line replaces an earlier one), except a scenario with an `input` line, which stays at 1."""
    directory.mkdir(parents=True, exist_ok=True)
    copies = {}
    for name, path in scenarios.items():
        text = path.read_text()
        copies[name] = directory / path.name
        if any(line.split(None, 1)[:1] == ["input"] for line in text.splitlines()):
            copies[name].write_text(text)
            continue
        copies[name].write_text(text + ("" if text.endswith("\n") else "\n") + f"steps_per_frame {steps}\n")
    return copies


def with_quirks(scenarios: dict[str, Path], spec: str, directory: Path) -> dict[str, Path]:
    """Copies of the scenarios in `directory`, each with a `quirks <spec>` line before its first order line (else at the end)."""
    directory.mkdir(parents=True, exist_ok=True)
    copies = {}
    for name, path in scenarios.items():
        lines = path.read_text().splitlines()
        at = next((i for i, line in enumerate(lines) if line.split()[:1] == ["order"]), len(lines))
        lines.insert(at, f"quirks {spec}")
        copies[name] = directory / path.name
        copies[name].write_text("\n".join(lines) + "\n")
    return copies


def roundtrip_ticks(spec: str, ticks: int) -> list[int]:
    """The ticks of a `--roundtrip` spec for a scenario of `ticks` ticks: `auto` is early, middle and last minus one; else the listed ticks."""
    if spec == "auto":
        return sorted({max(1, ticks // 10), max(1, ticks // 2), max(1, ticks - 1)})
    try:
        listed = [int(t) for t in spec.split(",")]
    except ValueError:
        fail(f"--roundtrip takes auto or ticks such as 100,2000, not {spec!r}")
    if not listed or min(listed) < 1:
        fail("--roundtrip ticks start at 1")
    return listed


def with_roundtrip(scenarios: dict[str, Path], spec: str, directory: Path) -> dict[str, Path]:
    """Copies of the scenarios in `directory`, each ending with a `roundtrip <T>` line per tick of `spec` (see roundtrip_ticks)."""
    directory.mkdir(parents=True, exist_ok=True)
    copies = {}
    for name, path in scenarios.items():
        text = path.read_text()
        match = re.search(r"^ticks\s+(\d+)\s*$", text, re.MULTILINE)
        if match is None:
            fail(f"scenario {name} has no ticks line to place a roundtrip by")
        ticks = int(match.group(1))
        lines = "".join(f"roundtrip {t}\n" for t in roundtrip_ticks(spec, ticks))
        copies[name] = directory / path.name
        copies[name].write_text(text + ("" if text.endswith("\n") else "\n") + lines)
    return copies


def first_difference(stored: list[tuple[int, str, str, str]], got: list[tuple[int, str, str, str]], column: int = DIGEST) -> str | None:
    """A description of the first tick at which `column` (DIGEST or STATE) differs, or None."""
    for a, b in zip(stored, got):
        if (a[0], a[column]) != (b[0], b[column]):
            return f"tick {a[0]}: stored {a[column]}, got {b[column]}"
    if len(stored) != len(got):
        longer, which = (stored, "stored") if len(stored) > len(got) else (got, "got")
        return f"tick {longer[min(len(stored), len(got))][0]}: only the {which} trace has it"
    return None


def view_note(stored: list[tuple[int, str, str, str]], got: list[tuple[int, str, str, str]]) -> str:
    """` [view differs from tick T]` for the first tick at which the view column differs (or the traces' lengths), else the empty string."""
    for a, b in zip(stored, got):
        if (a[0], a[VIEW]) != (b[0], b[VIEW]):
            return f" [view differs from tick {a[0]}]"
    if len(stored) != len(got):
        longer = stored if len(stored) > len(got) else got
        return f" [view differs from tick {longer[min(len(stored), len(got))][0]}]"
    return ""


def digest_note(stored: list[tuple[int, str, str, str]], got: list[tuple[int, str, str, str]]) -> str:
    """` [digest differs from tick T]` for the first tick at which the digest column differs (or the traces' lengths), else the empty string."""
    for a, b in zip(stored, got):
        if (a[0], a[DIGEST]) != (b[0], b[DIGEST]):
            return f" [digest differs from tick {a[0]}]"
    if len(stored) != len(got):
        longer = stored if len(stored) > len(got) else got
        return f" [digest differs from tick {longer[min(len(stored), len(got))][0]}]"
    return ""


def every_row_line(equal: int, total: int) -> str:
    """The line that follows the total of an --end-of-tick check: how many scenarios have their digest column equal as well."""
    return f"{equal} of {total} scenarios equal on every row"


def sound_difference(stored: str, got: str) -> str | None:
    """Where two sound logs first differ (a line number with both lines), or None when they are equal."""
    want, have = stored.splitlines(), got.splitlines()
    for number, (a, b) in enumerate(zip(want, have), 1):
        if a != b:
            return f"sound line {number}: stored `{a}`, got `{b}`"
    if len(want) != len(have):
        number = min(len(want), len(have)) + 1
        longer, which = (want, "stored") if len(want) > len(have) else (have, "got")
        return f"sound line {number}: only the {which} log has it (`{longer[number - 1]}`)"
    return None


def sound_report(scenarios: dict[str, Path], logs: dict[str, str | None], directory: Path) -> tuple[list[str], list[str]]:
    """(the scenarios whose sound log differs from reference/sounds/, the report lines): stored logs missing are only counted."""
    differing: list[str] = []
    lines: list[str] = []
    unstored: list[str] = []
    equal = 0
    for name in scenarios:
        stored = directory / f"{name}.txt"
        if not stored.is_file():
            unstored.append(name)
            continue
        got = logs.get(name)
        diff = sound_difference(stored.read_text(), got) if got is not None else "the run wrote no sound.txt"
        if diff:
            differing.append(name)
            lines.append(f"DIFF {name} {diff}")
        else:
            equal += 1
    lines.append(f"sound logs: {equal} of {len(scenarios)} equal, {len(differing)} differ, {len(unstored)} with no stored log in {directory.relative_to(ROOT)}")
    return differing, lines


def event_digest(log: Path) -> str | None:
    """The digest text of the run's events.txt (eventdigest.digest_text), None when the run wrote none; a log that cannot be digested gives its error as the text."""
    if not log.is_file():
        return None
    try:
        return eventdigest.digest_text(log.read_text(encoding="latin-1"))
    except eventdigest.Error as e:
        return f"unreadable: {e}\n"


def events_report(scenarios: dict[str, Path], logs: dict[str, str | None], directory: Path,
                  end_of_tick: bool = False) -> tuple[list[str], list[str]]:
    """(the scenarios whose sim event digest differs from reference/events/, the report lines): stored digests missing are only counted.

    `logs` hold each run's digest text (eventdigest.digest_text). With `end_of_tick` the digests are compared on their `tick` lines alone."""
    differing: list[str] = []
    lines: list[str] = []
    unstored: list[str] = []
    equal = 0
    for name in scenarios:
        stored = directory / f"{name}.txt"
        if not stored.is_file():
            unstored.append(name)
            continue
        got = logs.get(name)
        diff = eventdigest.digest_difference(stored.read_text(), got, end_of_tick) if got is not None else f"the run wrote no {EVENTS_FILE}"
        if diff:
            differing.append(name)
            lines.append(f"DIFF {name} {diff}")
        else:
            equal += 1
    lines.append(f"event digests: {equal} of {len(scenarios)} equal, {len(differing)} differ, {len(unstored)} with no stored digest in {directory.relative_to(ROOT)}")
    return differing, lines


def stored_without_scenario(directory: Path, known: set[str], what: str, suffix: str = ".txt") -> tuple[list[str], list[str]]:
    """(the names, the report lines) of the stored files of `directory` (`<name><suffix>`) that no scenario owns."""
    stems = [p.name[:-len(suffix)] for p in sorted(directory.glob(f"*{suffix}"))] if directory.is_dir() else []
    names = [stem for stem in stems if stem not in known]
    return names, [f"DIFF {name} stored {what} has no scenario of that name" for name in names]


def display_report(scenarios: dict[str, Path], logs: dict[str, str | None], directory: Path, end_of_tick: bool = False,
                   known: set[str] | None = None) -> tuple[list[str], list[str]]:
    """(the scenarios whose display digest differs from reference/display/, the report lines): stored digests missing are only counted.

    `logs` hold each run's display.txt. With `end_of_tick` the digests are compared on numbers, ticks, counts and `set` alone. A stored digest with no scenario fails."""
    known = set(scenario_files()) if known is None else known
    differing: list[str] = []
    lines: list[str] = []
    unstored: list[str] = []
    equal = 0
    for name in scenarios:
        stored = directory / f"{name}.txt"
        if not stored.is_file():
            unstored.append(name)
            continue
        got = logs.get(name)
        diff = displaylog.digest_difference(stored.read_text(), got, end_of_tick) if got is not None else f"the run wrote no {DISPLAY_FILE}"
        if diff:
            differing.append(name)
            lines.append(f"DIFF {name} {diff}")
        else:
            equal += 1
    extra, extra_lines = stored_without_scenario(directory, known, "display digest")
    differing += extra
    lines += extra_lines
    lines.append(f"display digests: {equal} of {len(scenarios)} equal, {len(differing)} differ, {len(unstored)} with no stored digest in {directory.relative_to(ROOT)}")
    return differing, lines


def display_list_report(scenarios: dict[str, Path], logs: dict[str, str | None], directory: Path, end_of_tick: bool = False,
                        known: set[str] | None = None) -> tuple[list[str], list[str]]:
    """(the scenarios whose display list differs from reference/displaylists/, the report lines).

    Only a scenario with a `frames` line has a list (every frame up to its last `frames` tick), stored as `<scenario>.txt.xz`; one
    with no stored list is only counted. With `end_of_tick` the items of a frame are compared as sorted layered lines. A stored list
    with no scenario fails."""
    known = set(scenario_files()) if known is None else known
    framed = [name for name, path in scenarios.items() if scenario_frames(path)]
    differing: list[str] = []
    lines: list[str] = []
    unstored: list[str] = []
    equal = 0
    for name in framed:
        stored = displayreplay.stored_list(name, directory)
        if not stored.is_file():
            unstored.append(name)
            continue
        got = logs.get(name)
        try:
            diff = displaylog.list_difference(displayreplay.read_list(stored), got, end_of_tick) if got is not None else f"the run wrote no {DISPLAYLIST_FILE}"
        except displaylog.Error as e:
            diff = f"unreadable display list: {e}"
        if diff:
            differing.append(name)
            lines.append(f"DIFF {name} display list {diff}")
        else:
            equal += 1
    extra, extra_lines = stored_without_scenario(directory, known, "display list", displayreplay.LIST_SUFFIX)
    differing += extra
    lines += extra_lines
    lines.append(f"display lists: {equal} of {len(framed)} equal, {len(differing)} differ, {len(unstored)} with no stored list in {directory.relative_to(ROOT)}")
    return differing, lines


def unlisted_frame(name: str, logs: dict[str, str | None], stored: str, end_of_tick: bool, directory: Path) -> int | None:
    """The tick of the first frame whose display digest differs from `stored` when no stored display list holds that frame, else None."""
    got = logs.get(name)
    if got is None:
        return None
    frame = displaylog.digest_frame(stored, got, end_of_tick)
    if frame is None:
        return None
    listed = displayreplay.stored_list(name, directory)
    if listed.is_file() and re.search(rf"^frame {frame[0]} ", displayreplay.read_list(listed), re.M):
        return None
    return frame[1]


@functools.cache
def trace_schema() -> dict:
    return json.loads((ROOT / "data/trace_schema.json").read_text())


def ui_difference(stored: str, got: str) -> str | None:
    """Where two UI logs first differ (a line number with both lines), or None when they are equal."""
    want, have = stored.splitlines(), got.splitlines()
    for number, (a, b) in enumerate(zip(want, have), 1):
        if a != b:
            return f"ui line {number}: stored `{a}`, got `{b}`"
    if len(want) != len(have):
        number = min(len(want), len(have)) + 1
        longer, which = (want, "stored") if len(want) > len(have) else (have, "got")
        return f"ui line {number}: only the {which} log has it (`{longer[number - 1]}`)"
    return None


def ui_gives_nothing(log: str, interface: str | None = None) -> bool:
    """Whether a UI log has no group line and never selects a unit, and the decoded interface commands have no record."""
    if interface is not None and len(interface.splitlines()) > 1:
        return False
    for line in log.splitlines():
        words = line.split()
        if len(words) >= 2 and words[1] in UI_EFFECTS:
            return False
        if len(words) >= 3 and words[1] == "select" and words[2] != "-":
            return False
    return True


def ui_report(scenarios: dict[str, Path], logs: dict[str, str | None], directory: Path,
              known: set[str] | None = None, interface_logs: dict[str, str | None] | None = None) -> tuple[list[str], list[str]]:
    """(the scenarios whose UI log fails, the report lines).

    A scenario fails when its stored log in `directory` differs from the run's, when its name starts with ui- and it
    has no stored log or its run gives no command (`interface_logs`) and selects nothing, and a stored log with no scenario (among
    `known`, default every scenario file) is reported too. Other scenarios without a stored log are only counted.
    """
    known = set(scenario_files()) if known is None else known
    differing: list[str] = []
    lines: list[str] = []
    unstored: list[str] = []
    equal = 0
    for name in scenarios:
        stored = directory / f"{name}.txt"
        got = logs.get(name)
        ui_scenario = name.startswith(UI_PREFIX)
        if not stored.is_file():
            if ui_scenario:
                differing.append(name)
                lines.append(f"DIFF {name} has no stored UI log in {directory.relative_to(ROOT)}")
            else:
                unstored.append(name)
            continue
        diff = ui_difference(stored.read_text(), got) if got is not None else f"the run wrote no {UI_FILE}"
        if not diff and ui_scenario and ui_gives_nothing(got or "", (interface_logs or {}).get(name)):
            diff = "the input gives no command and selects no unit"
        if diff:
            differing.append(name)
            lines.append(f"DIFF {name} {diff}")
        else:
            equal += 1
    for path in sorted(directory.glob("*.txt")) if directory.is_dir() else []:
        if path.stem not in known:
            differing.append(path.stem)
            lines.append(f"DIFF {path.stem} stored UI log has no scenario of that name")
    lines.append(f"ui logs: {equal} of {len(scenarios)} equal, {len(differing)} differ, {len(unstored)} with no stored log in {directory.relative_to(ROOT)}")
    return differing, lines


def interface_difference(stored: str, got: str) -> str | None:
    """Where two decoded interface command logs first differ (a line number with both lines), or None when equal."""
    want, have = stored.splitlines(), got.splitlines()
    for number, (a, b) in enumerate(zip(want, have), 1):
        if a != b:
            return f"interface line {number}: stored `{a}`, got `{b}`"
    if len(want) != len(have):
        number = min(len(want), len(have)) + 1
        longer, which = (want, "stored") if len(want) > len(have) else (have, "got")
        return f"interface line {number}: only the {which} log has it (`{longer[number - 1]}`)"
    return None


def interface_report(scenarios: dict[str, Path], logs: dict[str, str | None], directory: Path,
                     known: set[str] | None = None, texts: dict[str, str] | None = None) -> tuple[list[str], list[str]]:
    """(the scenarios whose interface commands fail, the report lines).

    A scenario with an `input` line (its text from `texts`, default its file) must have a stored file in `directory`
    equal to the decoded run. A scenario without one must have no stored file and its run no record. A stored file
    with no scenario (among `known`, default every scenario file) fails too.
    """
    known = set(scenario_files()) if known is None else known
    differing: list[str] = []
    lines: list[str] = []
    with_input = equal = 0
    relative = directory.relative_to(ROOT)
    for name, path in scenarios.items():
        text = texts[name] if texts is not None else path.read_text()
        stored = directory / f"{name}.txt"
        got = logs.get(name)
        if interfacelog.has_input(text):
            with_input += 1
            if not stored.is_file():
                differing.append(name)
                lines.append(f"DIFF {name} has no stored interface commands in {relative}")
                continue
            diff = interface_difference(stored.read_text(), got) if got is not None else f"the run wrote no {COMMANDS_FILE}"
            if diff:
                differing.append(name)
                lines.append(f"DIFF {name} {diff}")
            else:
                equal += 1
            continue
        records = (got or "").splitlines()[1:]
        if stored.is_file():
            differing.append(name)
            lines.append(f"DIFF {name} stored interface commands for a scenario without input lines")
        elif records:
            differing.append(name)
            lines.append(f"DIFF {name} the interface sent commands without input lines: {records[0]}")
    for path in sorted(directory.glob("*.txt")) if directory.is_dir() else []:
        if path.stem not in known:
            differing.append(path.stem)
            lines.append(f"DIFF {path.stem} stored interface commands have no scenario of that name")
    lines.append(f"interface commands: {equal} of {with_input} equal, {len(differing)} differ")
    return differing, lines


def frame_difference(stored: bytes, got: bytes) -> str | None:
    """Where two frame dumps first differ ("header", "palette" or the first pixel with both values), or None when equal."""
    if stored[:FRAME_HEADER_SIZE] != got[:FRAME_HEADER_SIZE]:
        return "header"
    palette = slice(FRAME_HEADER_SIZE, FRAME_HEADER_SIZE + FRAME_PALETTE_SIZE)
    if stored[palette] != got[palette]:
        return "palette"
    pixels = FRAME_HEADER_SIZE + FRAME_PALETTE_SIZE
    width = int.from_bytes(stored[6:8], "little")
    for offset, (a, b) in enumerate(zip(stored[pixels:], got[pixels:])):
        if a != b:
            return f"pixel x {offset % width} y {offset // width}: stored {a}, got {b}"
    if len(stored) != len(got):
        return f"size: stored {len(stored)} bytes, got {len(got)}"
    return None


def frame_report(scenarios: dict[str, Path], dumps: dict[str, dict[int, bytes]], directory: Path) -> tuple[list[str], list[str]]:
    """(the scenarios whose frames differ from directory/<scenario>/<tick>.bin, the report lines).

    Only a scenario with a `frames` line is judged, and there a missing stored frame or a frame the run did not write is a difference.
    """
    differing: list[str] = []
    lines: list[str] = []
    judged = equal = 0
    for name, path in scenarios.items():
        ticks = scenario_frames(path)
        if not ticks:
            continue
        judged += 1
        got = dumps.get(name, {})
        found = ""
        for tick in ticks:
            stored = directory / name / f"{tick}.bin"
            if not stored.is_file():
                found = f"frame {tick}: no stored frame in {(directory / name).relative_to(ROOT)}"
            elif tick not in got:
                found = f"frame {tick}: the run wrote no frame"
            else:
                diff = frame_difference(stored.read_bytes(), got[tick])
                found = f"frame {tick}: {diff}" if diff else ""
            if found:
                break
        if found:
            differing.append(name)
            lines.append(f"DIFF {name} {found}")
        else:
            equal += 1
    lines.append(f"frames: {equal} of {judged} scenarios equal, {len(differing)} differ, {len(scenarios) - judged} with no frames line")
    return differing, lines


def view_file_difference(stored: bytes, got: bytes) -> str | None:
    """The first field in which two view files differ (`<path>: stored <value>, got <value>`), or None when they are equal."""
    try:
        want, have = viewblock.split(stored), viewblock.split(got)
    except viewblock.Error as e:
        return str(e)
    found = viewblock.first_difference(want, have)
    return f"{found[0]}: stored {found[1]}, got {found[2]}" if found else None


def view_report(scenarios: dict[str, Path], dumps: dict[str, dict[int, bytes]], directory: Path) -> tuple[list[str], list[str]]:
    """(the scenarios whose views differ from directory/<scenario>/<tick>.bin, the report lines).

    Only a scenario with a `frames` line is judged, and there a missing stored view or a view the run did not write is a difference.
    """
    differing: list[str] = []
    lines: list[str] = []
    judged = equal = 0
    for name, path in scenarios.items():
        ticks = scenario_frames(path)
        if not ticks:
            continue
        judged += 1
        got = dumps.get(name, {})
        found = ""
        for tick in ticks:
            stored = directory / name / f"{tick}.bin"
            if not stored.is_file():
                found = f"view {tick}: no stored view in {(directory / name).relative_to(ROOT)}"
            elif tick not in got:
                found = f"view {tick}: the run wrote no view"
            else:
                diff = view_file_difference(stored.read_bytes(), got[tick])
                found = f"view {tick}: {diff}" if diff else ""
            if found:
                break
        if found:
            differing.append(name)
            lines.append(f"DIFF {name} {found}")
        else:
            equal += 1
    lines.append(f"views: {equal} of {judged} scenarios equal, {len(differing)} differ, {len(scenarios) - judged} with no frames line")
    return differing, lines


def save_difference(stored: bytes, got: bytes) -> str | None:
    """The first difference between two saves (tools/savefile.py compares their decoded contents), or None when they are equal."""
    found = savefile.differences(stored, got)
    return found[0] + (f" (and {len(found) - 1} more)" if len(found) > 1 else "") if found else None


def save_report(scenarios: dict[str, Path], saves: dict[str, bytes | None], directory: Path) -> tuple[list[str], list[str]]:
    """(the scenarios whose harness.sav differs from reference/saves/, the report lines): stored saves missing are only counted."""
    differing: list[str] = []
    lines: list[str] = []
    unstored: list[str] = []
    equal = 0
    saving = [n for n, path in scenarios.items() if scenario_saves(path)]
    for name in saving:
        stored = directory / f"{name}.sav"
        if not stored.is_file():
            unstored.append(name)
            continue
        got = saves.get(name)
        diff = save_difference(stored.read_bytes(), got) if got is not None else "the run wrote no harness.sav"
        if diff:
            differing.append(name)
            lines.append(f"DIFF {name} {diff}")
        else:
            equal += 1
    lines.append(f"saves: {equal} of {len(saving)} equal, {len(differing)} differ, {len(unstored)} with no stored save in {directory.relative_to(ROOT)}")
    return differing, lines


def setup_report(scenarios: dict[str, Path], logs: dict[str, str | None], directory: Path) -> tuple[list[str], list[str]]:
    """(the frontend scenarios whose setup log differs from reference/setup/ or has none stored, the report lines)."""
    differing: list[str] = []
    lines: list[str] = []
    frontend = [n for n, path in scenarios.items() if scenario_frontend(path)]
    for name in frontend:
        stored = directory / f"{name}.txt"
        got = logs.get(name)
        if not stored.is_file():
            diff = f"has no stored setup log in {directory.relative_to(ROOT)}"
        elif got is None:
            diff = f"the run wrote no {SETUP_FILE}"
        elif got != stored.read_text():
            diff = "the setup log differs from the stored one"
        else:
            continue
        differing.append(name)
        lines.append(f"DIFF {name} {diff}")
    lines.append(f"setup logs: {len(frontend) - len(differing)} of {len(frontend)} equal")
    return differing, lines


def readback_scenario(scenario: Path, setup: str) -> str:
    """The scenario that plays the stored setup log `setup` of the frontend scenario for one tick, with the scenario's seeds."""
    seeds = [line for line in scenario.read_text().splitlines() if line.split()[:1] in (["game_seed"], ["crt_seed"], ["view_seed"])]
    return setup + "".join(f"{line}\n" for line in seeds) + "ticks 1\n"


def check_readbacks(exe: Path, game_dir: str | None, parallel: int, scenarios: dict[str, Path],
                    expected: dict[str, tuple], modes: tuple[str | None, ...], directory: Path) -> int:
    """Play each frontend scenario's stored setup log (reference/setup/) as a scenario for one tick, in each of `modes`, and
    print whether its first tick equals the stored trace's; the number that differ or fail to run."""
    played: dict[str, Path] = {}
    want: dict[str, tuple] = {}
    directory.mkdir(parents=True, exist_ok=True)
    for name, path in scenarios.items():
        stored = SETUPS / f"{name}.txt"
        if not scenario_frontend(path) or not stored.is_file() or not expected[name][1]:
            continue
        readback = directory / f"{name}{READBACK_SUFFIX}.txt"
        readback.write_text(readback_scenario(path, stored.read_text()))
        played[readback.stem] = readback
        want[readback.stem] = (expected[name][0], expected[name][1][:1])
    if not played:
        return 0
    failed: dict[str, str] = {}
    results = play_all(exe, played, game_dir, parallel, want, failed=failed, **({"modes": modes} if modes != (None,) else {}))
    bad = 0
    for mode in modes:
        for name in played:
            key = tag(name, mode)
            if key in failed:
                diff = f"failed to run: {failed[key]}"
            else:
                diff = difference(want[name], *results[key][:2])
            print(f"{'DIFF' if diff else 'ok':4} {key} (the stored setup log, tick 1)" + (f" {diff}" if diff else ""))
            bad += bool(diff)
    total = len(played) * len(modes)
    print(f"setup readbacks: {total - bad} of {total} equal the first tick of the stored trace")
    return bad


def check_traces(exe: Path, game_dir: str | None, parallel: int, tier: bool = False, poison: bool = False,
                 steps_per_frame: int | None = None, quirks: str | None = None,
                 save_blockptrs: Path | None = None, roundtrip: str | None = None, both_modes: bool = False,
                 end_of_tick: bool = False) -> None:
    scenarios = scenario_files()
    column = STATE if end_of_tick else DIGEST
    modes = default_modes(both_modes)
    stored = {p.stem: p for p in sorted(TRACES.glob("*.txt"))} if TRACES.is_dir() else {}
    if tier:
        missing = [n for n in GATE_TIER if n not in scenarios or n not in stored]
        if missing:
            fail("the gate tier names scenarios without both a scenario and a trace: " + ", ".join(missing))
        scenarios = {n: scenarios[n] for n in GATE_TIER}
        stored = {n: stored[n] for n in GATE_TIER}
    problems = [f"scenario {n} has no trace in {TRACES.relative_to(ROOT)}" for n in scenarios if n not in stored]
    problems += [f"trace {n} has no scenario in {SCENARIOS.relative_to(ROOT)}" for n in stored if n not in scenarios]
    if not scenarios or problems:
        fail("\n".join(problems) or f"no scenarios in {SCENARIOS.relative_to(ROOT)}")
    expected = {}
    for name, path in stored.items():
        try:
            expected[name] = tracedigest.parse_digest(path)
        except tracediff.Error as e:
            fail(str(e))

    played = scenarios
    scratch = ROOT / "build/scratch" / f"steps-per-frame-{os.getpid()}"
    if steps_per_frame is not None:
        played = with_steps_per_frame(scenarios, steps_per_frame, scratch)
    if roundtrip is not None:
        played = with_roundtrip(played, roundtrip, scratch / "roundtrip")
    base = played
    if quirks is not None:
        played = with_quirks(played, quirks, scratch / "quirks")
    replays = with_replays(scenarios) if quirks is None and roundtrip is None and steps_per_frame is None else {}
    if replays:
        played = {**played, **replays}
        expected = {**expected, **{name: expected[replay_source(name)] for name in replays}}
    off_results: dict[str, tuple] = {}
    off_failed: dict[str, str] = {}
    failed: dict[str, str] = {}
    instrumented = (exe.parent / "accesslog.attr").is_file()
    scans: dict[str, str | None] | None = {} if quirks is None and not instrumented else None
    watch = Watch(expected, column=column) if quirks is None else None
    watching = threading.Event()

    def monitor() -> None:
        while not watching.wait(WATCH_SECONDS):
            watch.poll()

    monitor_thread = threading.Thread(target=monitor, daemon=True)
    if watch is not None:
        monitor_thread.start()
    t0 = time.time()
    try:
        results = play_all(exe, played, game_dir, parallel, expected, poison, failed if quirks is not None else None,
                           scans=scans, scan_every=GATE_SCAN_EVERY, **({"watch": watch} if watch is not None else {}),
                           **({"modes": modes} if both_modes else {}),
                           **({"column": column} if column != DIGEST else {}))
        if quirks is not None:
            differing = [n for n in scenarios if n in results and n not in failed and difference(expected[n], *results[n][:2])]
            if differing:
                off_results = play_all(exe, with_savers({n: base[n] for n in differing}, base), game_dir, parallel, expected, False, off_failed, True)
    finally:
        watching.set()
        shutil.rmtree(scratch, ignore_errors=True)
    wall = time.time() - t0
    if watch is not None and watch.found and len(results) < (len(scenarios) + len(replays)) * len(modes):
        found = watch.found
        where = f" [kept: {found.kept.relative_to(ROOT)}]" if found.kept else ""
        trace = f" [trace: {shown(found.trace)}]" if found.trace else ""
        print(found.diff if found.early else f"DIFF {found.name} ({found.ticks} ticks, {clock(found.elapsed)}) {found.diff}{trace}{where}")
        note = recheck(found, column)
        if note:
            print(note)
        print(f"stopped: {(len(scenarios) + len(replays)) * len(modes) - len(results)} scenarios not finished")
        if found.early:
            print("tracediff not run: the run ended early, so there is no complete trace to compare")
            sys.exit(1)
        if replay_source(untag(found.name)) is not None:
            report_replay_divergence()
        if mode_of(found.name) == "native":
            os.environ["BT_NATIVE_MODULES"] = "1"
        report_divergence(untag(found.name), expected, exe, scenarios, poison, game_dir, end_of_tick)
    if quirks is not None:
        report_quirks(scenarios, expected, results, failed, quirks, wall, off_results, off_failed)
        return
    bad: list[str] = []
    every_row = 0
    for mode in modes:
        for name in scenarios:
            key = tag(name, mode)
            header, digests, elapsed, kept = results[key]
            diff = difference(expected[name], header, digests, column)
            note = digest_note(expected[name][1], digests) + view_note(expected[name][1], digests) if end_of_tick else ""
            every_row += end_of_tick and not digest_note(expected[name][1], digests)
            where = f" [kept: {kept.relative_to(ROOT)}]" if kept else ""
            logged = SOUND_LOGS.get(key)
            sounds = f", {max(len(logged.splitlines()) - 1, 0)} sound lines" if logged is not None else ""
            print(f"{'DIFF' if diff else 'ok':4} {key} ({len(digests)} ticks, {clock(elapsed)}{sounds})" + (f" {diff}{where}" if diff else "") + note)
            if diff:
                bad.append(key)
    total = len(scenarios) * len(modes)
    replay_bad, replay_stale, replay_lines = replay_report(replays, modes, results, expected, column)
    if replays:
        print("\n".join(replay_lines[:-1]))
    print(f"{total - len(bad)} of {total} scenarios equal, total {clock(wall)}")
    if replays:
        print(replay_lines[-1])
    if end_of_tick:
        print(every_row_line(every_row, total))
    sound_bad = ui_bad = events_bad = display_bad = frame_bad = view_bad = save_bad = setup_bad = 0
    pointers = 0
    first_events: tuple[str, str] | None = None
    first_display: tuple[str, str, int] | None = None
    for mode in modes:
        label = f"[{mode}] " if both_modes else ""
        sound_names, sound_lines = sound_report(scenarios, mode_view(SOUND_LOGS, scenarios, mode), SOUNDS)
        print("\n".join(label + line for line in sound_lines))
        interface_view = mode_view(INTERFACE_LOGS, scenarios, mode)
        ui_names, ui_lines = ui_report(scenarios, mode_view(UI_LOGS, scenarios, mode), UIS, interface_logs=interface_view)
        print("\n".join(label + line for line in ui_lines))
        interface_names, interface_lines = interface_report(scenarios, interface_view, COMMANDS)
        print("\n".join(label + line for line in interface_lines))
        if replays:
            sources = {replay_source(name): scenarios[replay_source(name)] for name in replays}
            replay_view = {source: INTERFACE_LOGS.get(tag(replay_name(source), mode)) for source in sources}
            replay_names, replay_lines = interface_report(sources, replay_view, COMMANDS)
            print("\n".join(f"{label}from {COMMANDS_FILE}: {line}" for line in replay_lines))
            interface_names += replay_names
        events_names, events_lines = events_report(scenarios, mode_view(EVENT_LOGS, scenarios, mode), EVENTS, end_of_tick)
        print("\n".join(label + line for line in events_lines))
        display_names, display_lines = display_report(scenarios, mode_view(DISPLAY_LOGS, scenarios, mode), DISPLAYS, end_of_tick)
        print("\n".join(label + line for line in display_lines))
        list_names, list_lines = display_list_report(scenarios, mode_view(DISPLAY_LISTS, scenarios, mode), DISPLAYLISTS, end_of_tick)
        print("\n".join(label + line for line in list_lines))
        frame_names, frame_lines = frame_report(scenarios, mode_view(FRAME_DUMPS, scenarios, mode), FRAMES)
        print("\n".join(label + line for line in frame_lines))
        view_names, view_lines = view_report(scenarios, mode_view(VIEW_DUMPS, scenarios, mode), VIEWS)
        print("\n".join(label + line for line in view_lines))
        save_names, save_lines = save_report(scenarios, mode_view(SAVE_BYTES, scenarios, mode), SAVES)
        print("\n".join(label + line for line in save_lines))
        setup_names, setup_lines = setup_report(scenarios, mode_view(SETUP_LOGS, scenarios, mode), SETUPS)
        print("\n".join(label + line for line in setup_lines))
        setup_bad += len(setup_names)
        sound_bad += len(sound_names)
        ui_bad += len(ui_names) + len(interface_names)
        events_bad += len(events_names)
        if events_names and first_events is None:
            first_events = (events_names[0], mode)
        display_bad += len(display_names) + len(list_names)
        if first_display is None:
            for name in display_names:
                stored = DISPLAYS / f"{name}.txt"
                tick = unlisted_frame(name, mode_view(DISPLAY_LOGS, scenarios, mode), stored.read_text(), end_of_tick, DISPLAYLISTS) if stored.is_file() else None
                if tick is not None:
                    first_display = (name, mode, tick)
                    break
        frame_bad += len(frame_names)
        view_bad += 0 if end_of_tick else len(view_names)
        save_bad += len(save_names)
        if scans is not None:
            pointers += report_block_pointers(exe, mode_view(scans, scenarios, mode), save_blockptrs / mode if save_blockptrs and both_modes else save_blockptrs, label)
        elif instrumented:
            print(f"{label}block pointers: not scanned, access-log build")
        pointers += report_fuel(exe, scenarios, mode_view(FUEL, scenarios, mode), mode == "native" if both_modes else None, label)
    display_replay_bad = 0
    if game_dir is not None and quirks is None and not instrumented:
        framed = {name: path for name, path in scenarios.items() if scenario_frames(path)}
        replay_names, replay_lines = displayreplay.check(exe, game_dir, framed, max(1, parallel), run_slot)
        print("\n".join(replay_lines))
        display_replay_bad = len(replay_names)
    if not bad and replay_bad:
        report_replay_divergence()
    if not bad and replay_stale:
        sys.exit(1)
    if not bad:
        setup_bad += check_readbacks(exe, game_dir, parallel, scenarios, expected, modes, scratch / "readback")
        shutil.rmtree(scratch, ignore_errors=True)
        if first_events is not None:
            report_event_records(first_events[0], first_events[1], expected, exe, scenarios, poison, game_dir, end_of_tick)
        if first_display is not None:
            report_display_items(*first_display, expected, exe, scenarios, poison, game_dir, end_of_tick)
        if pointers or sound_bad or ui_bad or events_bad or display_bad or frame_bad or view_bad or save_bad or setup_bad or display_replay_bad:
            sys.exit(1)
        if tier:
            print(tier_message(len(scenarios), head_commit()))
        return

    if mode_of(bad[0]) == "native":
        os.environ["BT_NATIVE_MODULES"] = "1"
    report_divergence(untag(bad[0]), expected, exe, scenarios, poison, game_dir, end_of_tick)


def replay_report(replays: dict[str, Path], modes: tuple[str | None, ...], results: dict[str, tuple],
                  expected: dict[str, tuple], column: int = DIGEST) -> tuple[list[str], list[str], list[str]]:
    """(the runs from commands.txt that differ, the runs of REPLAY_LEFT_OUT that are equal, the report lines).

    A run of a scenario in REPLAY_LEFT_OUT that differs is a `left` line with its reason, and fails nothing. The last
    line counts the runs.
    """
    bad: list[str] = []
    stale: list[str] = []
    lines: list[str] = []
    listed = 0
    for mode in modes:
        for name in replays:
            key = tag(name, mode)
            header, digests, elapsed, kept = results[key]
            diff = difference(expected[name], header, digests, column)
            reason = REPLAY_LEFT_OUT.get(replay_source(name))
            where = f" [kept: {kept.relative_to(ROOT)}]" if kept else ""
            if reason and diff:
                listed += 1
                lines.append(f"left {key} ({len(digests)} ticks, {clock(elapsed)}) {diff}: {reason}")
            elif reason:
                stale.append(key)
                lines.append(f"DIFF {key} ({len(digests)} ticks, {clock(elapsed)}) equal, but REPLAY_LEFT_OUT lists it: remove it")
            else:
                lines.append(f"{'DIFF' if diff else 'ok':4} {key} ({len(digests)} ticks, {clock(elapsed)})" + (f" {diff}{where}" if diff else ""))
                if diff:
                    bad.append(key)
    runs = len(replays) * len(modes)
    lines.append(f"{runs - len(bad) - len(stale) - listed} of {runs} runs from {COMMANDS_FILE} equal, {listed} left out (REPLAY_LEFT_OUT)")
    return bad, stale, lines


def report_replay_divergence() -> None:
    """Say why no tracediff follows a run from commands.txt that differs, then exit 1."""
    print(f"\ntracediff not run: a run from {COMMANDS_FILE} has no reference build to compare with. Its scenario's own run "
          "is equal, so a write of the interface or the console that is not an event is still hashed "
          "(docs/commands.md, section 2.2): compare the kept trace with the scenario's own run (tools/tracediff.py --traces).")
    sys.exit(1)


def report_divergence(name: str, expected: dict[str, tuple], exe: Path, scenarios: dict[str, Path], poison: bool,
                      game_dir: str | None, end_of_tick: bool = False) -> None:
    """Run tracediff on the scenario `name` against the reference build of its stored trace (on the `tick` rows alone with `end_of_tick`), then exit 1."""
    sha = expected[name][0].get("commit", "unknown")
    print(f"\ntracediff for {name}: a = the reference at {sha}, b = {exe}", flush=True)
    if resolve_quiet(sha) is None:
        fail(f"{name}: the trace's commit {sha} is not in this repository")
    ref_dir = build_cached(sha)
    subprocess.run(tracediff_command(ref_dir / "TotalA.exe", exe, scenarios[name], poison, game_dir, end_of_tick))
    sys.exit(1)


def report_event_records(name: str, mode: str, expected: dict[str, tuple], exe: Path, scenarios: dict[str, Path],
                         poison: bool, game_dir: str | None, end_of_tick: bool = False) -> None:
    """Play the reference build of the scenario's stored trace and the exe on `name`, and print the first record of their event logs that differs (under the end-of-tick rule of docs/events-log.md with `end_of_tick`)."""
    sha = expected[name][0].get("commit", "unknown")
    print(f"\nevent records for {name}: a = the reference at {sha}, b = {exe} ({mode})", flush=True)
    if resolve_quiet(sha) is None:
        fail(f"{name}: the trace's commit {sha} is not in this repository")
    ref_dir = build_cached(sha)
    if mode == "native":
        os.environ["BT_NATIVE_MODULES"] = "1"
    cmd = tracediff_command(ref_dir / "TotalA.exe", exe, scenarios[name], poison, game_dir, end_of_tick) + ["--events"]
    subprocess.run(cmd)


def report_display_items(name: str, mode: str, tick: int, expected: dict[str, tuple], exe: Path, scenarios: dict[str, Path],
                         poison: bool, game_dir: str | None, end_of_tick: bool = False) -> None:
    """Play the reference build of the scenario's stored trace and the exe on `name` with `-testdisplay <tick>`, and print the first item of their display lists that differs (under the end-of-tick rule of docs/display-log.md with `end_of_tick`)."""
    sha = expected[name][0].get("commit", "unknown")
    print(f"\ndisplay items for {name} at tick {tick}: a = the reference at {sha}, b = {exe} ({mode})", flush=True)
    if resolve_quiet(sha) is None:
        fail(f"{name}: the trace's commit {sha} is not in this repository")
    ref_dir = build_cached(sha)
    if mode == "native":
        os.environ["BT_NATIVE_MODULES"] = "1"
    cmd = tracediff_command(ref_dir / "TotalA.exe", exe, scenarios[name], poison, game_dir, end_of_tick) + ["--display", "--display-tick", str(tick)]
    subprocess.run(cmd)


def head_commit() -> str:
    """The short sha of the checked-out commit, or `unknown` outside a repository."""
    sha = resolve_quiet("HEAD")
    return sha[:12] if sha else "unknown"


def tier_message(count: int, sha: str) -> str:
    """The line a passing --tier run ends with: the tier and the commit, and that the full suite is a separate run."""
    return (f"gate tier passed: {count} scenarios equal at {sha}; the full --check is the gate of the changes "
            f"that reference/README.md, \"Gate\", lists")


def report_block_pointers(exe: Path, scans: dict[str, str | None], save: Path | None, label: str = "") -> int:
    """Print the pointers out of the block that data/block_pointers.csv does not cover; how many there are.

    A site is named by its rank among its function's allocator calls in the exe, not by the scans, so the tier's scenarios are named as the full suite's are.
    With `save`, each scenario's blockptrs.txt is also written there as <scenario>.txt.
    """
    if save is not None:
        save.mkdir(parents=True, exist_ok=True)
        for name, text in scans.items():
            if text is not None:
                (save / f"{name}.txt").write_text(text)
    problems = blockpointers.check_scans(scans, exe.with_suffix(".map"), blockpointers.default_build_dir())
    for line in problems:
        print(f"{label}BLOCK POINTER {line}")
    if problems:
        print(f"{label}{len(problems)} problems with the pointers out of the block; add them to data/block_pointers.csv "
              f"(uv run tools/blockpointers.py --from <dir>, see reference/README.md)")
    else:
        print(f"{label}block pointers: all covered by data/block_pointers.csv ({len(scans)} scenarios)")
    return len(problems)


def tracediff_command(ref_exe: Path, exe: Path, scenario: Path, poison: bool, game_dir: str | None,
                      end_of_tick: bool = False) -> list[str]:
    cmd = [sys.executable, str(ROOT / "tools/tracediff.py"), str(ref_exe), str(exe),
           "--scenario", str(scenario), "--reference", "a"]
    if end_of_tick:
        cmd.append("--end-of-tick")
    if poison:
        cmd.append("--poison-b")
    if game_dir:
        cmd += ["--game-dir", game_dir]
    return cmd


def shown(path: Path) -> Path:
    """The path relative to the repository when it is inside it."""
    try:
        return path.relative_to(ROOT)
    except ValueError:
        return path


def quirk_divergence(kept: Path, off_kept: Path) -> str | None:
    """The first tick and phase at which two kept runs' trace.txt differ, with the phase's function, or None."""
    _, rows = tracediff.parse_trace(kept / "trace.txt")
    _, off_rows = tracediff.parse_trace(off_kept / "trace.txt")
    found = tracediff.first_divergence(off_rows, rows)
    if found is None:
        return None
    return f"tick {found[0]}, phase {found[1]} ({tracediff.describe_phase(found[1])})"


def report_quirks(scenarios: dict[str, Path], expected: dict[str, tuple], results: dict[str, tuple],
                  failed: dict[str, str], spec: str, wall: float,
                  off_results: dict[str, tuple], off_failed: dict[str, str]) -> None:
    """Print one line per scenario (equal, differs at a tick and phase, or failed) and a total; exits 1 only if one failed.

    A scenario that differs from the stored trace is played again with every flag off (`off_results`,
    `off_failed`), whose phases the quirks run is compared with. Both runs' directories are kept. A
    difference is not a failure, but a rerun that fails or that differs from the stored trace is.
    """
    equal = differ = bad = 0
    for name in scenarios:
        lines = failed.get(name)
        if lines is not None:
            bad += 1
            print(f"FAIL {name}: {lines}")
            continue
        header, digests, elapsed, kept = results[name]
        diff = difference(expected[name], header, digests)
        if not diff:
            equal += 1
            print(f"equal   {name} ({len(digests)} ticks, {clock(elapsed)})")
            continue
        if name in off_failed:
            bad += 1
            print(f"FAIL {name}: the all-off rerun failed: {off_failed[name]}")
            continue
        off_header, off_digests, _, off_kept = off_results[name]
        off_diff = difference(expected[name], off_header, off_digests)
        if off_diff:
            bad += 1
            print(f"FAIL {name}: all-off run differs from the stored trace, {off_diff} [kept: {shown(off_kept)}]")
            continue
        differ += 1
        divergence = None
        try:
            divergence = quirk_divergence(kept, off_kept)
        except (tracediff.Error, OSError, ValueError) as e:
            divergence = f"{diff} (no phase: {last_lines(e)})"
        print(f"differs {name} ({len(digests)} ticks, {clock(elapsed)}) {divergence or diff}"
              f" [kept: {shown(kept)}, all-off: {shown(off_kept)}]")
    print(f"quirks {spec}: {equal} equal, {differ} differ, {bad} failed of {len(scenarios)} scenarios, total {clock(wall)}")
    if bad:
        sys.exit(1)


def resolve_quiet(ref: str) -> str | None:
    proc = git("rev-parse", "--verify", "-q", f"{ref}^{{commit}}", check=False, capture=True)
    return proc.stdout.strip() if proc.returncode == 0 else None


def build_cached(sha: str) -> Path:
    """build/reference/<sha>, built if it holds no exe of that commit."""
    directory = ROOT / "build/reference" / sha
    stamp = directory / "commit.txt"
    if not (directory / "TotalA.exe").is_file() or not stamp.is_file() or stamp.read_text().strip() != sha:
        for name in OUTPUTS:
            (directory / name).unlink(missing_ok=True)
        prepare_worktree(directory, sha)
        build(directory, sha)
    return directory


def reference_branch() -> Path:
    git("fetch", "-q", "origin", "reference", check=False)
    directory = ROOT / "build/reference/branch"
    prepare_worktree(directory, resolve("origin/reference"))
    return directory


def name_mismatch(produced: set[str], expected: set[str]) -> str:
    """Empty when the sets are equal, else a message naming the missing and the extra names."""
    if produced == expected:
        return ""
    missing = ", ".join(sorted(expected - produced)) or "none"
    extra = ", ".join(sorted(produced - expected)) or "none"
    return f"the traces do not match moddable's scenarios; missing: {missing}; extra: {extra}"


def copy_frames(source: Path) -> None:
    """Replace reference/frames/ with the <scenario>/<tick>.bin files of `source` (left alone when it holds none)."""
    produced = sorted(source.glob("*/*.bin"))
    if not produced:
        print("the reference branch wrote no frames; reference/frames/ is untouched")
        return
    shutil.rmtree(FRAMES, ignore_errors=True)
    for path in produced:
        (FRAMES / path.parent.name).mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, FRAMES / path.parent.name / path.name)
    print(f"{len(produced)} frames copied to {FRAMES.relative_to(ROOT)}")


def copy_views(source: Path) -> None:
    """Replace reference/views/ with the <scenario>/<tick>.bin files of `source` (left alone when it holds none)."""
    produced = sorted(source.glob("*/*.bin"))
    if not produced:
        print("the reference branch wrote no views; reference/views/ is untouched")
        return
    shutil.rmtree(VIEWS, ignore_errors=True)
    for path in produced:
        (VIEWS / path.parent.name).mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, VIEWS / path.parent.name / path.name)
    print(f"{len(produced)} views copied to {VIEWS.relative_to(ROOT)}")


def copy_saves(directory: Path) -> None:
    """Replace reference/saves/ by the saves the reference branch checkout `directory` wrote (none written leaves it as it is)."""
    saves = sorted((directory / "reference/saves").glob("*.sav"))
    if not saves:
        print("the reference branch wrote no saves; reference/saves/ is untouched")
        return
    SAVES.mkdir(parents=True, exist_ok=True)
    for old in SAVES.glob("*.sav"):
        old.unlink()
    for path in saves:
        shutil.copyfile(path, SAVES / path.name)
    print(f"{len(saves)} saves copied to {SAVES.relative_to(ROOT)}")


def copy_ui(directory: Path) -> None:
    """Replace reference/ui/ by the UI logs the reference branch checkout `directory` wrote (none written leaves it as it is)."""
    logs = sorted((directory / "reference/ui").glob("*.txt"))
    if not logs:
        print("the reference branch wrote no UI logs; reference/ui/ is untouched")
        return
    UIS.mkdir(parents=True, exist_ok=True)
    for old in UIS.glob("*.txt"):
        old.unlink()
    for path in logs:
        shutil.copyfile(path, UIS / path.name)
    print(f"{len(logs)} UI logs copied to {UIS.relative_to(ROOT)}")


def copy_commands(directory: Path) -> None:
    """Replace reference/commands/ by the decoded interface commands the reference branch checkout `directory` wrote (none written leaves it as it is)."""
    logs = sorted((directory / "reference/commands").glob("*.txt"))
    if not logs:
        print("the reference branch wrote no interface commands; reference/commands/ is untouched")
        return
    COMMANDS.mkdir(parents=True, exist_ok=True)
    for old in COMMANDS.glob("*.txt"):
        old.unlink()
    for path in logs:
        shutil.copyfile(path, COMMANDS / path.name)
    print(f"{len(logs)} interface command logs copied to {COMMANDS.relative_to(ROOT)}")


def copy_events(directory: Path) -> None:
    """Replace reference/events/ by the digests of the sim event logs the reference branch checkout `directory` wrote (none written leaves it as it is)."""
    logs = sorted((directory / "reference/events").glob("*.txt"))
    if not logs:
        print("the reference branch wrote no event logs; reference/events/ is untouched")
        return
    EVENTS.mkdir(parents=True, exist_ok=True)
    for old in EVENTS.glob("*.txt"):
        old.unlink()
    for path in logs:
        (EVENTS / path.name).write_text(eventdigest.digest_text(path.read_text(encoding="latin-1")))
    print(f"{len(logs)} event digests written to {EVENTS.relative_to(ROOT)}")


def copy_display(directory: Path) -> None:
    """Replace reference/display/ and reference/displaylists/ by the files the reference branch checkout `directory` wrote (none written leaves a directory as it is).

    The display lists are stored as xz (displayreplay.pack_list), `<scenario>.txt.xz`."""
    for source, dest in ((directory / "reference/display", DISPLAYS), (directory / "reference/displaylists", DISPLAYLISTS)):
        logs = sorted(source.glob("*.txt"))
        if not logs:
            print(f"the reference branch wrote no files in {dest.relative_to(ROOT)}; it is untouched")
            continue
        dest.mkdir(parents=True, exist_ok=True)
        for old in (*dest.glob("*.txt"), *dest.glob(f"*{displayreplay.LIST_SUFFIX}")):
            old.unlink()
        copied = 0
        for path in logs:
            # TODO(#1166): store the display digest of a frontend scenario once the menu cursor is drawn on the frames, not the wall clock
            if dest == DISPLAYS and (SCENARIOS / path.name).exists() and scenario_frontend(SCENARIOS / path.name):
                continue
            if dest == DISPLAYLISTS:
                (dest / f"{path.stem}{displayreplay.LIST_SUFFIX}").write_bytes(displayreplay.pack_list(path.read_text(encoding="latin-1")))
            else:
                shutil.copyfile(path, dest / path.name)
            copied += 1
        print(f"{copied} display files copied to {dest.relative_to(ROOT)}")


def copy_setup(directory: Path) -> None:
    """Replace reference/setup/ by the setup logs the reference branch checkout `directory` wrote (none written leaves it as it is)."""
    logs = sorted((directory / "reference/setup").glob("*.txt"))
    if not logs:
        print("the reference branch wrote no setup logs; reference/setup/ is untouched")
        return
    shutil.rmtree(SETUPS, ignore_errors=True)
    SETUPS.mkdir(parents=True)
    for path in logs:
        shutil.copyfile(path, SETUPS / path.name)
    print(f"{len(logs)} setup logs copied to {SETUPS.relative_to(ROOT)}")


def write_traces(game_dir: str | None, parallel: int, ticks_per_second: float | None = None) -> None:
    directory = reference_branch()
    branch_scenarios = directory / "reference/scenarios"
    shutil.rmtree(branch_scenarios, ignore_errors=True)
    shutil.copytree(SCENARIOS, branch_scenarios)
    cmd = ["uv", "run", "tools/reference.py", "HEAD", "--traces", "--parallel", str(parallel)]
    if game_dir:
        cmd += ["--game-dir", game_dir]
    if ticks_per_second is not None:
        cmd += ["--ticks-per-second", f"{ticks_per_second:g}"]
    if subprocess.run(cmd, cwd=directory, env=REF_ENV).returncode:
        fail("tools/reference.py --traces failed on the reference branch")
    produced = sorted((directory / "reference/traces").glob("*.txt"))
    if not produced:
        fail("the reference branch wrote no traces; reference/traces/ is untouched")
    mismatch = name_mismatch({p.stem for p in produced}, set(scenario_files()))
    if mismatch:
        fail(f"{mismatch}; reference/traces/ is untouched")
    TRACES.mkdir(parents=True, exist_ok=True)
    for old in TRACES.glob("*.txt"):
        old.unlink()
    for path in produced:
        shutil.copyfile(path, TRACES / path.name)
    print(f"{len(produced)} traces copied to {TRACES.relative_to(ROOT)}")
    copy_frames(directory / "reference/frames")
    copy_views(directory / "reference/views")
    copy_saves(directory)
    copy_ui(directory)
    copy_commands(directory)
    copy_events(directory)
    copy_display(directory)
    copy_setup(directory)
    sounds = sorted((directory / "reference/sounds").glob("*.txt"))
    if not sounds:
        print("the reference branch wrote no sound logs; reference/sounds/ is untouched")
        return
    SOUNDS.mkdir(parents=True, exist_ok=True)
    for old in SOUNDS.glob("*.txt"):
        old.unlink()
    for path in sounds:
        shutil.copyfile(path, SOUNDS / path.name)
    print(f"{len(sounds)} sound logs copied to {SOUNDS.relative_to(ROOT)}")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--traces", action="store_true", help="rewrite reference/traces/ from the reference branch")
    ap.add_argument("--check", action="store_true", help="play every scenario and compare with reference/traces/")
    ap.add_argument("--tier", action="store_true",
                    help="--check: play only GATE_TIER, the pull request gate (not the gate of the changes that need the full suite)")
    ap.add_argument("--native-modules", action="store_true",
                    help="--check: play every scenario with -modules=native, so that the modules run as native code (the default runs them in WASM; sets BT_NATIVE_MODULES=1 for tools/playtest.py)")
    ap.add_argument("--both-modes", action="store_true",
                    help="--check: play every scenario twice in one pool, once with the modules in WASM and once with -modules=native, and pass only when both modes equal the stored traces (the gate)")
    ap.add_argument("--no-lock", action="store_true",
                    help="--check: do not take the machine-wide gate lock (by default a second --check on the machine waits for the first), for runs that are not gates")
    ap.add_argument("--no-cache", action="store_true",
                    help="--check: play even when the same exe, files and options passed before (by default a cached pass in ~/.cache/bt-gate is reported instead)")
    ap.add_argument("--poison", action="store_true",
                    help="--check: play the scenarios with new game allocations filled with 0xCD; list the bytes whose starting value reaches hashed state; a difference is expected for real uninitialised reads")
    ap.add_argument("--quirks", metavar="SPEC",
                    help="--check: play every scenario with `quirks SPEC` added and report which equal the stored traces and which differ, with the first tick and phase (a differing scenario is rerun with all flags off; a difference is not a failure; no reference build); not the gate")
    ap.add_argument("--roundtrip", metavar="T[,T...]|auto",
                    help="--check: play every scenario with a `roundtrip T` line per tick, which copies the sim block at the end of tick T, moves it to another base (or scrambles and restores it in place when the modules run in WASM) and continues; the traces must still equal the stored ones; `auto` picks early, middle and last tick minus one from each scenario's ticks")
    ap.add_argument("--end-of-tick", action="store_true",
                    help="--check: judge each scenario on the end-of-tick state alone (the `tick` row), and report the digest of all the rows without failing on it; the verdict for a re-implementation that keeps the end-of-tick contract with its own order inside the tick (#1037)")
    ap.add_argument("--exe", metavar="PATH", help="--check: the exe to check (build/modern/TotalA.exe)")
    ap.add_argument("--save-blockptrs", metavar="DIR", type=Path,
                    help="--check: write each scenario's blockptrs.txt to DIR/<scenario>.txt (input of blockpointers.py --from)")
    ap.add_argument("--steps-per-frame", type=int, metavar="N",
                    help="--check: play every scenario with steps_per_frame N (1 to 8) and compare with the stored traces")
    ap.add_argument("--game-dir", help="--traces, --check: the game's install (default: Steam's)")
    ap.add_argument("--parallel", type=int, default=None, metavar="N",
                    help="--traces, --check: scenarios played at once (default: half the CPUs, at most 6, or 10 with --both-modes)")
    ap.add_argument("--ticks-per-second", type=float, default=None, metavar="N",
                    help=f"--traces, --check: the slowest game speed that is not a hang, in the run timeout (default {DEFAULT_TICKS_PER_SECOND})")
    args = ap.parse_args()
    if args.parallel is None:
        args.parallel = default_parallel(args.both_modes)
    if args.ticks_per_second is not None and args.ticks_per_second <= 0:
        ap.error("--ticks-per-second takes a number above 0")
    if args.ticks_per_second is not None:
        global RUN_TICKS_PER_SECOND
        RUN_TICKS_PER_SECOND = args.ticks_per_second
    if args.parallel < 1:
        ap.error("--parallel takes 1 or more")
    if args.traces == args.check:
        ap.error("give one of --traces and --check")
    if args.poison and not args.check:
        ap.error("--poison needs --check")
    if args.native_modules and not args.check:
        ap.error("--native-modules needs --check")
    if args.both_modes and (not args.check or args.native_modules or args.quirks is not None):
        ap.error("--both-modes needs --check, and cannot be combined with --native-modules or --quirks")
    if args.no_lock and not args.check:
        ap.error("--no-lock needs --check")
    if args.no_cache and not args.check:
        ap.error("--no-cache needs --check")
    if args.native_modules:
        os.environ["BT_NATIVE_MODULES"] = "1"
    if args.save_blockptrs and (not args.check or args.quirks is not None):
        ap.error("--save-blockptrs needs --check, and cannot be combined with --quirks")
    if args.tier and not args.check:
        ap.error("--tier needs --check; it cannot be used with --traces")
    if args.steps_per_frame is not None and not args.check:
        ap.error("--steps-per-frame needs --check")
    if args.steps_per_frame is not None and not 1 <= args.steps_per_frame <= 8:
        ap.error("--steps-per-frame takes 1 to 8")
    if args.quirks is not None and not args.check:
        ap.error("--quirks needs --check")
    if args.quirks is not None and not args.quirks.strip():
        ap.error("--quirks takes a spec, such as recommended or '3.1c +flag'")
    if args.quirks is not None and args.poison:
        ap.error("--quirks cannot be combined with --poison")
    if args.roundtrip is not None and not args.check:
        ap.error("--roundtrip needs --check")
    if args.roundtrip is not None and (args.quirks is not None or args.poison or args.save_blockptrs):
        ap.error("--roundtrip cannot be combined with --quirks, --poison or --save-blockptrs")
    if args.roundtrip is not None:
        roundtrip_ticks(args.roundtrip, 100)
    if args.end_of_tick and not args.check:
        ap.error("--end-of-tick needs --check; it cannot be used with --traces")
    if args.end_of_tick and args.quirks is not None:
        ap.error("--end-of-tick cannot be combined with --quirks")
    if args.exe and not args.check:
        ap.error("--exe needs --check")
    if args.traces:
        write_traces(args.game_dir, args.parallel, args.ticks_per_second)
        return
    if not args.exe:
        ap.error("--check needs --exe: the gate is the modern build, --exe build/modern/TotalA.exe")
    if not Path(args.exe).is_file():
        ap.error(f"{args.exe} is not a file")
    exe = Path(args.exe).resolve()
    key = None
    if not args.no_cache and not args.save_blockptrs:
        key = gate_key(exe, {"tier": args.tier, "mode": "both" if args.both_modes else "native" if args.native_modules else "wasm",
                             "steps_per_frame": args.steps_per_frame, "quirks": args.quirks, "roundtrip": args.roundtrip,
                             "poison": args.poison, "end_of_tick": args.end_of_tick}, ROOT)
        stored = cached_pass(key)
        if stored is not None:
            report_cached(stored)
            return
    global GATE_RUN_SLOTS
    GATE_RUN_SLOTS = None if args.no_lock else RunSlots()
    with KeyLock(key_lock_path(key)) if key is not None and not args.no_lock else contextlib.nullcontext():
        if key is not None:
            stored = cached_pass(key)
            if stored is not None:
                report_cached(stored)
                return
        tee = Tee(sys.stdout)
        sys.stdout = tee
        try:
            check_traces(exe, args.game_dir, args.parallel, args.tier, args.poison, args.steps_per_frame,
                         args.quirks, args.save_blockptrs, args.roundtrip, args.both_modes, args.end_of_tick)
        finally:
            sys.stdout = tee.stream
        if key is not None:
            store_pass(key, tee.text(), head_commit())


if __name__ == "__main__":
    main()
