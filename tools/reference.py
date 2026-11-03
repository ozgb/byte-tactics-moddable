"""The conformity gate: play the scenarios and compare their digests with reference/traces/.

    uv run tools/reference.py --check --exe build/modern/TotalA.exe    # every scenario: the gate of the changes in reference/README.md, "Gate"
    uv run tools/reference.py --check --exe build/modern/TotalA.exe --tier     # the pull request gate: the scenarios of GATE_TIER
    uv run tools/reference.py --traces          # rewrite reference/traces/ from the reference branch

`--check --exe` plays every scenario in reference/scenarios/ (`--parallel N` at a
time, default half the CPUs and at most 6 (10 with `--both-modes`), under Wine through tools/playtest.py)
and compares the digest of each tick (tools/tracedigest.py) with the stored one. It also compares
each run's sound.txt with reference/sounds/<scenario>.txt, line by line, when that file exists (docs/sound-log.md), and each run's ui.txt with reference/ui/<scenario>.txt the same way (docs/ui-log.md).
A scenario with a `frames` line also has its frame dumps compared, byte for byte, with reference/frames/<scenario>/ (docs/frame-dump.md); a missing stored frame fails.
It also compares the harness.sav of each scenario with a `save` line with
reference/saves/<scenario>.sav, when that file exists, by decoded contents (tools/savefile.py leaves out the bytes taken from the clock or from uninitialised memory).
On the first difference it builds the reference from the commit the trace
records, prints tools/tracediff.py's report for that scenario and exits 1. A
missing or extra trace or scenario exits 1. `--tier` restricts `--check` to the
scenarios named in GATE_TIER, the pull request gate (tools/coverage.py --tier checks that they cover enough).

`--check --both-modes` plays every scenario once in WASM and once with `-modules=native` in one pool (each line names its
mode, the sound logs and block pointer scans are kept apart per mode) and passes only when both modes equal the stored traces.
A `--check` holds an exclusive lock on $XDG_RUNTIME_DIR/bt-gate.lock (else /tmp) for the whole run, so that one machine plays
one gate at a time; a second one prints `waiting for the gate run of <pid> (<worktree>)` and waits. `--no-lock` skips it.
A passing `--check` stores its output under ~/.cache/bt-gate/<key>.txt, the key being a SHA-256 of the exe, the files beside it
(wasmtime_host.dll, *.wasm, modules.json), reference/{scenarios,traces,sounds,ui,frames,saves}, the four tools that play and compare
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
(modules run natively; with WASM modules the block is scrambled and restored in place). The traces must equal the stored ones.

`--traces` checks out origin/reference in build/reference/branch, runs that
branch's own `tools/reference.py HEAD --traces` there (it links the matching
build with the original compiler and plays the scenarios), and copies the
traces it wrote into reference/traces/, the sound logs it wrote into reference/sounds/ (if it wrote any), the UI logs it wrote into reference/ui/, the
frames it wrote into reference/frames/ (if it wrote any) and the saves it wrote into reference/saves/ (if it wrote any).
Set WINEARCH=win64 for Wine; `--game-dir` names the game's
install.
"""

import argparse
import ast
import contextlib
import fcntl
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time
from collections.abc import Callable
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path

import blockpointers
import playtest
import savefile
import tracediff
import tracedigest

ROOT = Path(__file__).resolve().parent.parent
OUTPUTS = ("TotalA.exe", "SMACKW32.DLL", "commit.txt")
SCENARIOS = ROOT / "reference/scenarios"
TRACES = ROOT / "reference/traces"
SOUNDS = ROOT / "reference/sounds"
SOUND_FILE = "sound.txt"
UIS = ROOT / "reference/ui"
UI_FILE = "ui.txt"
UI_PREFIX = "ui-"
UI_EFFECTS = ("order", "qbuild", "cancel", "group", "groupsel")
FRAMES = ROOT / "reference/frames"
FRAME_HEADER_SIZE = 16
FRAME_PALETTE_SIZE = 1024
SAVES = ROOT / "reference/saves"
SAVE_FILE = tracediff.SAVE_FILES[0]
MAX_PARALLEL = 6
MAX_PARALLEL_BOTH_MODES = 10
GATE_TIER = (
    "allied-victory",
    "commander-death",
    "conditions-dark-side",
    "console-economy",
    "console-features",
    "console-sight",
    "console-units",
    "eight-pincushion-teams",
    "hailstorm-los-off",
    "load-midgame",
    "mission-radius",
    "mission-victory",
    "orders-air",
    "orders-air-fights",
    "orders-air-repair",
    "orders-build-patrol",
    "orders-factory",
    "orders-guard-attacked",
    "orders-guard-reclaim",
    "orders-immediate",
    "orders-tier2-core",
    "orders-tier2-spider",
    "orders-tier2-vader",
    "save-midgame",
    "sensors-floating-teeth-los-off",
    "units-transport-ship",
)
RUN_SECONDS_BASE = 120
RUN_TICKS_PER_SECOND = 25
RUN_TICKS_PER_SECOND_SCAN_EVERY_TICK = 6
RUN_ATTEMPTS = 2
GATE_SCAN_EVERY = 8
BLOCK_POINTERS_TIER_TEXT = "block pointers: full suite only"
FUEL_FILE = "fuel.txt"
FUEL: dict[str, dict[str, tuple[int, int]]] = {}
SOUND_LOGS: dict[str, str | None] = {}
UI_LOGS: dict[str, str | None] = {}
FRAME_DUMPS: dict[str, dict[int, bytes]] = {}
SAVE_BYTES: dict[str, bytes | None] = {}
MODES = ("wasm", "native")
LOCK_FILE = "bt-gate.lock"
CACHE_ENV = "BT_GATE_CACHE"
KEY_TREES = ("reference/scenarios", "reference/traces", "reference/sounds", "reference/ui", "reference/frames", "reference/saves",
            "tools/playtest")
KEY_TOOLS = ("tools/reference.py", "tools/playtest.py", "tools/tracediff.py", "tools/tracedigest.py", "tools/savefile.py",
             "tools/blockpointers.py")
KEY_DATA = ("data/block_pointers.csv",)
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


def difference(want: tuple[dict[str, str], list[tuple[int, str]]], header: dict[str, str],
               digests: list[tuple[int, str]]) -> str | None:
    """Why a run differs from the stored (header, digests), or None when it equals them."""
    want_header, want_digests = want
    diff = first_difference(want_digests, digests)
    seeds = [k for k in ("game_seed", "crt_seed", "view_seed") if want_header.get(k) != header[k]]
    if seeds:
        diff = diff or "the scenario's seeds differ from the stored trace: " + ", ".join(seeds)
    return diff


def keep_run(run: Path, scenario: str, suffix: str = "") -> Path:
    """Copy the run's trace.txt (and dump.txt and blockptrs.txt, if any) to build/scratch/check-<scenario>-<time>/."""
    kept = ROOT / "build/scratch" / f"check-{scenario}{suffix}-{time.strftime('%Y%m%d-%H%M%S')}"
    kept.mkdir(parents=True, exist_ok=True)
    for name in ("trace.txt", SOUND_FILE, UI_FILE, "dump.txt", "blockptrs.txt"):
        if (run / name).is_file():
            shutil.copy2(run / name, kept / name)
    return kept


def keep_failed_attempt(error: Exception, scenario: str) -> Path | None:
    """Copy the playtest.py session a failed attempt left (its wine.log, steps.log, summary.md, trace.txt and crash.txt)
    to build/scratch/check-<scenario>-failed-<time>/, or None when the error names no session."""
    m = re.search(r"^logs and screenshots: (.+)/$", str(error), re.M)
    if not m or not (ROOT / m[1]).is_dir():
        return None
    kept = ROOT / "build/scratch" / f"check-{scenario}-failed-{time.strftime('%Y%m%d-%H%M%S')}"
    shutil.copytree(ROOT / m[1], kept, dirs_exist_ok=True)
    return kept


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


def default_modes(both_modes: bool) -> tuple[str | None, ...]:
    """The modes a check plays each scenario in: both of MODES, or one run per scenario (mode None, set by BT_NATIVE_MODULES)."""
    return MODES if both_modes else (None,)


def lock_path() -> Path:
    """The file the gate lock is taken on: under $XDG_RUNTIME_DIR, else /tmp."""
    return Path(os.environ.get("XDG_RUNTIME_DIR") or "/tmp") / LOCK_FILE


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


class GateLock:
    """An exclusive lock on one file, held for a whole --check, so that one machine plays one gate at a time.

    The holder writes its pid and worktree into the file; a second process prints who holds it and waits.
    """

    def __init__(self, path: Path | None = None, worktree: Path | None = None):
        self.path = path or lock_path()
        self.worktree = worktree or ROOT
        self.fd: int | None = None

    def holder(self) -> str:
        """`<pid> (<worktree>)` of the process that holds the lock, as it wrote it, or `another process` while it has not."""
        for _ in range(20):
            text = self.path.read_text().strip()
            if text:
                pid, _, worktree = text.partition(" ")
                return f"{pid} ({worktree})"
            time.sleep(0.1)
        return "another process"

    def __enter__(self) -> "GateLock":
        self.fd = os.open(self.path, os.O_RDWR | os.O_CREAT, 0o666)
        try:
            os.fchmod(self.fd, 0o666)
        except OSError:
            pass
        try:
            fcntl.flock(self.fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print(f"waiting for the gate run of {self.holder()}", flush=True)
            fcntl.flock(self.fd, fcntl.LOCK_EX)
        os.ftruncate(self.fd, 0)
        os.write(self.fd, f"{os.getpid()} {self.worktree}\n".encode())
        return self

    def __exit__(self, *exc) -> None:
        if self.fd is not None:
            os.ftruncate(self.fd, 0)
            os.close(self.fd)
            self.fd = None


def game_trace(pid: int) -> Path | None:
    """The trace.txt that the playtest.py process `pid` has its exe write, or None before the run has a game directory."""
    work = playtest.OUT / "work"
    if not work.is_dir():
        return None
    found = sorted(d for d in work.iterdir() if re.match(rf"\d{{8}}-\d{{6}}-{pid}-", d.name))
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


@dataclass
class Found:
    """The first run seen to differ from its stored trace (`name` is the run's name, see tag)."""
    name: str
    diff: str
    ticks: int
    elapsed: float
    kept: Path | None


class Watch:
    """Compares each running scenario's trace.txt, as it grows, with the stored digests, and stops every run at the first difference."""

    def __init__(self, expected: dict[str, tuple], locate: Callable[[int], Path | None] | None = None):
        self.expected = expected
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
        run.path = run.path or self.locate(run.proc.pid)
        if run.path is None:
            return
        with run.path.open("rb") as f:
            f.seek(run.offset)
            data = f.read()
        run.offset += len(data)
        want = self.expected[run.scenario][1]
        for got in run.tail.feed(data):
            index = run.seen
            run.seen += 1
            if want[index:index + 1] != [got]:
                diff = first_difference(want[index:index + 1], [got])
                self.halt(name, diff, run.seen, time.time() - run.started, keep_run(run.path.parent, name))
                return

    def halt(self, name: str, diff: str, ticks: int, elapsed: float, kept: Path | None) -> None:
        """Record the first difference and stop every running scenario (the first call wins)."""
        with self.lock:
            if self.found:
                return
            self.found = Found(name, diff, ticks, elapsed, kept)
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


def scenario_saves(scenario: Path) -> bool:
    """Whether the scenario has a `save` line, so that its run writes harness.sav."""
    return any(line.split()[:1] == ["save"] for line in scenario.read_text().splitlines())


def play_scenario(exe: Path, scenario: Path, game_dir: str | None,
                  want: tuple[dict[str, str], list[tuple[int, str]]] | None = None, poison: bool = False,
                  keep_always: bool = False, scans: dict[str, str | None] | None = None,
                  scan_every: int = 1, retry_any: bool = False, save_to: Path | None = None,
                  load_from: Path | None = None, watch: "Watch | None" = None,
                  mode: str | None = None) -> tuple[dict[str, str], list[tuple[int, str]], float, Path | None]:
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
    RUN_TICKS_PER_SECOND_SCAN_EVERY_TICK. With `retry_any` a failed run is tried once more whatever the failure.

    With `save_to` (a directory), each of tracediff.SAVE_FILES the run wrote in its game copy is copied there by its
    file name before the run's directory is removed (nothing is copied for a file the run wrote none of). With
    `load_from` (a directory), its files are placed in the run's game copy at those paths before the exe starts.

    With `watch`, the run's process is registered there (see Watch), and a run that differs at its end halts the watch.

    With `mode` ("wasm" or "native"), the run's modules run in that mode whatever BT_NATIVE_MODULES says, and its
    sound log, frames, save, scan and fuel are stored under tag(scenario, mode).
    """
    key = tag(scenario.stem, mode)
    ticks = scenario_ticks(scenario)
    rate = RUN_TICKS_PER_SECOND_SCAN_EVERY_TICK if scans is not None and scan_every == 1 else RUN_TICKS_PER_SECOND
    timeout = RUN_SECONDS_BASE + ticks / rate
    t0 = time.time()
    for attempt in range(RUN_ATTEMPTS):
        try:
            frame_files = tuple(f"frames/{tick}.bin" for tick in scenario_frames(scenario))
            extra: dict = {"collect": (FUEL_FILE, SOUND_FILE, UI_FILE, *frame_files, *dict.fromkeys(
                (*(tracediff.SAVE_FILES if save_to is not None else ()), *((SAVE_FILE,) if scenario_saves(scenario) else ()))))}
            if watch is not None:
                extra["started"] = watch.started(key)
            if mode is not None:
                extra["env"] = dict(os.environ, BT_NATIVE_MODULES="1" if mode == "native" else "0")
            if load_from is not None:
                extra["put"] = tracediff.save_put(load_from, scenario.stem, load_from.name)
            run = tracediff.play(exe, scenario, game_dir, None, timeout, poison, scan_every if scans is not None else 0, **extra)
            break
        except tracediff.Error as e:
            if (watch is not None and watch.halted.is_set()) or attempt == RUN_ATTEMPTS - 1 or not (retry_any or should_retry(str(e))):
                raise
            failed = keep_failed_attempt(e, scenario.stem)
            where = f" [failed attempt kept: {failed.relative_to(ROOT)}]" if failed else ""
            print(f"{scenario.stem}: run failed, trying again: {last_lines(e)}{where}", flush=True)
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
        FRAME_DUMPS[key] = {tick: (run / "frames" / f"{tick}.bin").read_bytes()
                            for tick in scenario_frames(scenario) if (run / "frames" / f"{tick}.bin").is_file()}
        if scenario_saves(scenario):
            saved = run / SAVE_FILE
            SAVE_BYTES[key] = saved.read_bytes() if saved.is_file() else None
        header, rows = tracediff.parse_trace(tracediff.need(run, "trace.txt"))
        for seed in ("game_seed", "crt_seed", "view_seed"):
            if seed not in header:
                raise tracediff.Error(f"{scenario}: the trace has no {seed} line")
        digests = tracedigest.digests(rows)
        if scans is not None:
            scanned = run / "blockptrs.txt"
            scans[key] = scanned.read_text() if scanned.is_file() else None
        diff = difference(want, header, digests) if want is not None else None
        if keep_always or diff:
            kept = keep_run(run, scenario.stem, ("-alloff" if keep_always else "") + (f"-{mode}" if mode else ""))
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
             retry_any: bool = False, watch: Watch | None = None, modes: tuple[str | None, ...] = (None,)) -> dict[str, tuple]:
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

    A scenario with a `load` line runs after every scenario that has none, in any mode and with any `parallel`:
    the scenario it names saves a game (the `save` key), the run's savegame/harness.sav and savegame/harness.opt are copied out of its game
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

    def one(key: str) -> None:
        if watch is not None and watch.halted.is_set():
            return
        name, mode = untag(key), mode_of(key)
        try:
            problem = load_problem(key)
            if problem:
                raise tracediff.Error(problem)
            saver = loads[key]
            results[key] = play_scenario(exe, scenarios[name], game_dir, (expected or {}).get(name), poison, keep_always, scans, scan_every, retry_any,
                                         save_to=saves / key if key in loads.values() else None,
                                         load_from=saves / saver if saver else None,
                                         **({"watch": watch} if watch is not None else {}),
                                         **({"mode": mode} if mode is not None else {}))
        except (tracediff.Error, OSError) as e:
            if watch is not None and watch.halted.is_set():
                return
            errors[key] = last_lines(e, 5)
            if failed is not None:
                failed[key] = errors[key]
            else:
                failures.append(f"{key}: failed to run, playtest.py ended with:\n    {last_lines(e, 5)}")

    def longest_first(key: str) -> int:
        try:
            return -scenario_ticks(scenarios[untag(key)])
        except tracediff.Error:
            return 0

    ordered = sorted(keys, key=longest_first)
    try:
        with ThreadPoolExecutor(parallel) as pool:
            list(pool.map(one, [k for k in ordered if loads[k] is None]))
            list(pool.map(one, [k for k in ordered if loads[k] is not None]))
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


def first_difference(stored: list[tuple[int, str]], got: list[tuple[int, str]]) -> str | None:
    """A description of the first tick at which the digests differ, or None."""
    for a, b in zip(stored, got):
        if a != b:
            return f"tick {a[0]}: stored {a[1]}, got {b[1]}"
    if len(stored) != len(got):
        longer, which = (stored, "stored") if len(stored) > len(got) else (got, "got")
        return f"tick {longer[min(len(stored), len(got))][0]}: only the {which} trace has it"
    return None


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


def ui_gives_nothing(log: str) -> bool:
    """Whether a UI log has no order, queued build, cancel or group line and never selects a unit (a broken ui scenario)."""
    for line in log.splitlines():
        words = line.split()
        if len(words) >= 2 and words[1] in UI_EFFECTS:
            return False
        if len(words) >= 3 and words[1] == "select" and words[2] != "-":
            return False
    return True


def ui_report(scenarios: dict[str, Path], logs: dict[str, str | None], directory: Path,
              known: set[str] | None = None) -> tuple[list[str], list[str]]:
    """(the scenarios whose UI log fails, the report lines).

    A scenario fails when its stored log in `directory` differs from the run's, when its name starts with ui- and it
    has no stored log or its run's log gives no order and selects nothing, and a stored log with no scenario (among
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
        if not diff and ui_scenario and ui_gives_nothing(got or ""):
            diff = "the input gives no order and selects no unit"
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


def check_traces(exe: Path, game_dir: str | None, parallel: int, tier: bool = False, poison: bool = False,
                 steps_per_frame: int | None = None, quirks: str | None = None,
                 save_blockptrs: Path | None = None, roundtrip: str | None = None, both_modes: bool = False) -> None:
    scenarios = scenario_files()
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
    off_results: dict[str, tuple] = {}
    off_failed: dict[str, str] = {}
    failed: dict[str, str] = {}
    scans: dict[str, str | None] | None = {} if quirks is None and roundtrip is None else None
    watch = Watch(expected) if quirks is None else None
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
                           **({"modes": modes} if both_modes else {}))
        if quirks is not None:
            differing = [n for n in scenarios if n in results and n not in failed and difference(expected[n], *results[n][:2])]
            if differing:
                off_results = play_all(exe, with_savers({n: base[n] for n in differing}, base), game_dir, parallel, expected, False, off_failed, True)
    finally:
        watching.set()
        shutil.rmtree(scratch, ignore_errors=True)
    wall = time.time() - t0
    if watch is not None and watch.found and len(results) < len(scenarios) * len(modes):
        found = watch.found
        where = f" [kept: {found.kept.relative_to(ROOT)}]" if found.kept else ""
        print(f"DIFF {found.name} ({found.ticks} ticks, {clock(found.elapsed)}) {found.diff}{where}")
        print(f"stopped: {len(scenarios) * len(modes) - len(results)} scenarios not finished")
        if mode_of(found.name) == "native":
            os.environ["BT_NATIVE_MODULES"] = "1"
        report_divergence(untag(found.name), expected, exe, scenarios, poison, game_dir)
    if quirks is not None:
        report_quirks(scenarios, expected, results, failed, quirks, wall, off_results, off_failed)
        return
    bad: list[str] = []
    for mode in modes:
        for name in scenarios:
            key = tag(name, mode)
            header, digests, elapsed, kept = results[key]
            diff = difference(expected[name], header, digests)
            where = f" [kept: {kept.relative_to(ROOT)}]" if kept else ""
            logged = SOUND_LOGS.get(key)
            sounds = f", {max(len(logged.splitlines()) - 1, 0)} sound lines" if logged is not None else ""
            print(f"{'DIFF' if diff else 'ok':4} {key} ({len(digests)} ticks, {clock(elapsed)}{sounds})" + (f" {diff}{where}" if diff else ""))
            if diff:
                bad.append(key)
    total = len(scenarios) * len(modes)
    print(f"{total - len(bad)} of {total} scenarios equal, total {clock(wall)}")
    sound_bad = ui_bad = frame_bad = save_bad = 0
    pointers = 0
    for mode in modes:
        label = f"[{mode}] " if both_modes else ""
        sound_names, sound_lines = sound_report(scenarios, mode_view(SOUND_LOGS, scenarios, mode), SOUNDS)
        print("\n".join(label + line for line in sound_lines))
        ui_names, ui_lines = ui_report(scenarios, mode_view(UI_LOGS, scenarios, mode), UIS)
        print("\n".join(label + line for line in ui_lines))
        frame_names, frame_lines = frame_report(scenarios, mode_view(FRAME_DUMPS, scenarios, mode), FRAMES)
        print("\n".join(label + line for line in frame_lines))
        save_names, save_lines = save_report(scenarios, mode_view(SAVE_BYTES, scenarios, mode), SAVES)
        print("\n".join(label + line for line in save_lines))
        sound_bad += len(sound_names)
        ui_bad += len(ui_names)
        frame_bad += len(frame_names)
        save_bad += len(save_names)
        if scans is None:
            print(f"{label}block pointers: not checked with --roundtrip (the scan names allocations by address)")
        else:
            pointers += report_block_pointers(exe, mode_view(scans, scenarios, mode), tier, save_blockptrs / mode if save_blockptrs and both_modes else save_blockptrs, label)
        pointers += report_fuel(exe, scenarios, mode_view(FUEL, scenarios, mode), mode == "native" if both_modes else None, label)
    if not bad:
        if pointers or sound_bad or ui_bad or frame_bad or save_bad:
            sys.exit(1)
        if tier:
            print(tier_message(len(scenarios), head_commit()))
        return

    if mode_of(bad[0]) == "native":
        os.environ["BT_NATIVE_MODULES"] = "1"
    report_divergence(untag(bad[0]), expected, exe, scenarios, poison, game_dir)


def report_divergence(name: str, expected: dict[str, tuple], exe: Path, scenarios: dict[str, Path], poison: bool,
                      game_dir: str | None) -> None:
    """Run tracediff on the scenario `name` against the reference build of its stored trace, then exit 1."""
    sha = expected[name][0].get("commit", "unknown")
    print(f"\ntracediff for {name}: a = the reference at {sha}, b = {exe}", flush=True)
    if resolve_quiet(sha) is None:
        fail(f"{name}: the trace's commit {sha} is not in this repository")
    ref_dir = build_cached(sha)
    subprocess.run(tracediff_command(ref_dir / "TotalA.exe", exe, scenarios[name], poison, game_dir))
    sys.exit(1)


def head_commit() -> str:
    """The short sha of the checked-out commit, or `unknown` outside a repository."""
    sha = resolve_quiet("HEAD")
    return sha[:12] if sha else "unknown"


def tier_message(count: int, sha: str) -> str:
    """The line a passing --tier run ends with: the tier and the commit, and that the full suite is a separate run."""
    return (f"gate tier passed: {count} scenarios equal at {sha}; the full --check is the gate of the changes "
            f"that reference/README.md, \"Gate\", lists")


def report_block_pointers(exe: Path, scans: dict[str, str | None], tier: bool, save: Path | None, label: str = "") -> int:
    """Print the pointers out of the block that data/block_pointers.csv does not cover; how many there are.

    The names of repeated sites are numbered over the scenarios played, so the gate tier is not checked: block pointers are checked by the full suite only.
    With `save`, each scenario's blockptrs.txt is also written there as <scenario>.txt.
    """
    if save is not None:
        save.mkdir(parents=True, exist_ok=True)
        for name, text in scans.items():
            if text is not None:
                (save / f"{name}.txt").write_text(text)
    if tier:
        print(label + BLOCK_POINTERS_TIER_TEXT)
        return 0
    problems = blockpointers.check_scans(scans, exe.with_suffix(".map"), blockpointers.default_build_dir())
    for line in problems:
        print(f"{label}BLOCK POINTER {line}")
    if problems:
        print(f"{label}{len(problems)} problems with the pointers out of the block; add them to data/block_pointers.csv "
              f"(uv run tools/blockpointers.py --from <dir>, see reference/README.md)")
    else:
        print(f"{label}block pointers: all covered by data/block_pointers.csv ({len(scans)} scenarios)")
    return len(problems)


def tracediff_command(ref_exe: Path, exe: Path, scenario: Path, poison: bool, game_dir: str | None) -> list[str]:
    cmd = [sys.executable, str(ROOT / "tools/tracediff.py"), str(ref_exe), str(exe),
           "--scenario", str(scenario), "--reference", "a"]
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


def write_traces(game_dir: str | None, parallel: int) -> None:
    directory = reference_branch()
    branch_scenarios = directory / "reference/scenarios"
    shutil.rmtree(branch_scenarios, ignore_errors=True)
    shutil.copytree(SCENARIOS, branch_scenarios)
    cmd = ["uv", "run", "tools/reference.py", "HEAD", "--traces", "--parallel", str(parallel)]
    if game_dir:
        cmd += ["--game-dir", game_dir]
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
    copy_saves(directory)
    copy_ui(directory)
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
    ap.add_argument("--exe", metavar="PATH", help="--check: the exe to check (build/modern/TotalA.exe)")
    ap.add_argument("--save-blockptrs", metavar="DIR", type=Path,
                    help="--check: write each scenario's blockptrs.txt to DIR/<scenario>.txt (input of blockpointers.py --from)")
    ap.add_argument("--steps-per-frame", type=int, metavar="N",
                    help="--check: play every scenario with steps_per_frame N (1 to 8) and compare with the stored traces")
    ap.add_argument("--game-dir", help="--traces, --check: the game's install (default: Steam's)")
    ap.add_argument("--parallel", type=int, default=None, metavar="N",
                    help="--traces, --check: scenarios played at once (default: half the CPUs, at most 6, or 10 with --both-modes)")
    args = ap.parse_args()
    if args.parallel is None:
        args.parallel = default_parallel(args.both_modes)
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
    if args.exe and not args.check:
        ap.error("--exe needs --check")
    if args.traces:
        write_traces(args.game_dir, args.parallel)
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
                             "poison": args.poison}, ROOT)
        stored = cached_pass(key)
        if stored is not None:
            report_cached(stored)
            return
    with contextlib.nullcontext() if args.no_lock else GateLock():
        if key is not None:
            stored = cached_pass(key)
            if stored is not None:
                report_cached(stored)
                return
        tee = Tee(sys.stdout)
        sys.stdout = tee
        try:
            check_traces(exe, args.game_dir, args.parallel, args.tier, args.poison, args.steps_per_frame,
                         args.quirks, args.save_blockptrs, args.roundtrip, args.both_modes)
        finally:
            sys.stdout = tee.stream
        if key is not None:
            store_pass(key, tee.text(), head_commit())


if __name__ == "__main__":
    main()
