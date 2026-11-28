"""Judge a submitted module by behaviour alone (docs/module-abi.md, "Judging a module").

    uv run tools/oraclegate.py MODULE.wasm --subsystem wind_meteors [--full | --recordings-only] [--manifest M] [--out DIR] [--build-dir build/modern] [--game-dir DIR]

Refuses the module, before anything plays, when `tools/wasm_modules.py check-module` finds a problem in the file: the window,
table, imports, exports, kinds or size budget of docs/module-abi.md, section 5.2. Then copies the runtime files of the modern build (and objs/objects.rsp, which the block pointer check reads beside the map) into a fresh directory with MODULE.wasm in place of the subsystem's module,
starts the game on a short scenario to see whether the loader accepts the module, then plays
`tools/reference.py --check --end-of-tick --tier` (`--full`: without `--tier`) and `tools/replay.py --check --subsystem NAME` on that
runtime. It writes DIR/result.json, built from the parsed fields of those tools' reports and nothing else: for each scenario
and recording, `equal` or the first divergence (tick, phase, field name, the module's value), the kit version and the
module's sha256. A failure of another check of the run is `other_checks: failed`, with `reason: block-pointer` when it is
the block pointer check, which judges the harness's scan and not the module's behaviour. The reference's values never reach it. The raw reports stay in DIR/logs/, which is for the host only.

The verdict of a scenario is its end-of-tick `state` (the hash of the `tick` row, the whole judged state at the end of the tick),
and so is the exit status. Each scenario also has a `digest` verdict, and the result a top-level `digest`: `equal` or `differs`
(from a tick) for the digest of all the rows of the tick. A module equal on `state` but different on `digest` kept the end-of-tick
contract with another order inside the tick; the digest is not a failure. The `view` verdicts are the same for the hash of the `view` row
(the player view of the end of the tick, docs/view.md "The recorded view"): `equal`, `differs` (from a tick) or `not_judged`, per scenario and at the top
level. A view that differs is not a failure either: the state stays the verdict.

A module of a subsystem whose manifest entry is `blocked` (its default does not build to WASM, so the game cannot load it in
a scenario), or any module with `--recordings-only`, takes the recordings route instead: after the same check-module, it
replays every recording of tests/replay/NAME/ against the module (`replay_native_test.exe --wasm`, one run per recording,
under Wine) and judges it on those alone. Its result has `scope: recordings` and `scenarios: not_judged`, and names the
replay rules it was judged under in `rules`: the imports answered as pure queries (`services.GetGafFrame`: by its arguments,
whenever the module asks, in any count or order) and the arguments no callee reads (the `y` of the position the reclaim
scan passes to `los_radar.GetMapCellAtPosition`, which the scan leaves unset). The scenario route's result has `scope: scenarios`.

A blocked entry has no budgets, so its module is judged without them: check-module does not judge its size, and the replayer
runs it with the largest fuel budget (`--no-budgets`, 2147483647 a tick). Its result has `budgets: not_judged`, and the
summary line says so; every other result has `budgets: judged`. `--manifest` judges against another game manifest than
wasm/modules.json (recordings route only): check-module, the replayer and the blocked test all read it.

A scenario or recording that failed to run has a `reason`, one word of a closed set parsed from the run's output (none of its
text): the host's `host-runtime`, `host-replayer` or `host-timeout`, the module's `module-load` or `module-trap` (each with a
`class`: the loader's refusal or the trap's kind), or `unknown`. Before the first recording, the recordings route checks the
host: the replayer and the wasmtime DLL are in the build, and `wine` starts the replayer (its usage line). When that fails it
plays nothing and writes a result with only `error` (a fixed sentence), `reason`, `format` and `subsystem`, and exits 2.

Every run first removes DIR/result.json, DIR/runtime and DIR/logs, so a run that stops with an error leaves no earlier result.

Exit status: 0 when every scenario and recording is equal, 1 when one differs or failed to run, 2 when the loader refused
the module, the host check failed or the arguments were wrong.
"""

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

import reference
import replay
import wasm_modules

ROOT = Path(__file__).resolve().parent.parent
FORMAT = 4
REPLAYER = "replay_native_test.exe"
MANIFEST = ROOT / "wasm/modules.json"
REPLAY_TIMEOUT = 3600
RULES = {"queries": ["services.GetGafFrame"], "unread_arguments": ["los_radar.GetMapCellAtPosition pos.y"]}
RUNTIME_FILES = ("TotalA.exe", "TotalA.map", "CMakeCache.txt", "wasmtime_host.dll", "modules.json")
OBJECT_LIST = "objs/objects.rsp"
PROBE = "game_seed 1\ncrt_seed 2\nview_seed 3\nmap The Pass\nstart_positions fixed\nticks 10\nplayer0.controller human\nplayer0.side Arm\nplayer1.controller ai\nplayer1.side Core\n"
MESSAGE_LIMIT = 2000
VALUE_LIMIT = 200
NAME = re.compile(r"[A-Za-z0-9_.#\[\]-]{1,200}$")
SCENARIO_LINE = re.compile(r"(ok|DIFF)\s+(\S+) \(\d+ ticks, [^)]*\)(?: (.*))?$")
TICK = re.compile(r"tick (\d+): ")
DIGEST_DIFFERS = re.compile(r"\[digest differs from tick (\d+)\]")
VIEW_DIFFERS = re.compile(r"\[view differs from tick (\d+)\]")
DIVERGENCE_FOR = re.compile(r"tracediff for (\S+): a = ")
DIVERGENCE_AT = re.compile(r"first divergence: tick (\d+), phase (\S+)$")
DIVERGENCE_FIELD = re.compile(r"first differing field: (\S+): a=(.*?) b=(.*)$")
FAILED_TO_RUN = re.compile(r"(\S+): failed to run")
EARLY = re.compile(r"^EARLY (\S+): ended at tick \d+ of \d+ \((.*?)\)(?:, host said [^:]+: (.*?))?(?: \[kept: [^\]]*\])?$")
BLOCK_POINTER_SUMMARY = re.compile(r"(?:\[\w+\] )?\d+ problems with the pointers out of the block")
REPLAY_DIVERGED = re.compile(r"^DIVERGED .*: first at call (\d+), (.+)$", re.M)
REPLAY_IDENTICAL = re.compile(r": \d+ calls, \d+ imports, identical on a second run$", re.M)
REPLAY_ARRAY_BYTE = re.compile(r"byte (\d+) of \d+: recorded \S*, replayed (\S*), \d+ bytes differ$")
REPLAY_ARRAY_COUNT = re.compile(r"recorded \d+ elements, replayed (\d+)$")
PREFLIGHT_TIMEOUT = 300
REPLAYER_USAGE = "usage: replay_native_test"
REPLAYER_BANNER = "replaying against the WASM module "
HOST_ERRORS = {
    "host-runtime": "the host could not start the replayer: wine or the wasmtime runtime did not start (a fault of the host, not of the module)",
    "host-replayer": "the host's build has no replay_native_test.exe (a fault of the host, not of the module)",
}
WINE_MISSING_REPLAYER = re.compile(r"^wine: cannot find .*" + re.escape(REPLAYER), re.M | re.I)
WINE_FAULT = re.compile(r"^(?:wine: |wineserver: |wine client error)", re.M)
RUNTIME_FAULT = re.compile(r"cannot load \S*wasmtime_host\.dll$|\S*wasmtime_host\.dll has no export \S+$")
REPLAYER_STOP = re.compile(r"^FAIL: .*?\.wasm: (.*)$", re.M)
FATAL_PENDING = re.compile(r"^\S+: wl_call returned with a fatal pending$", re.M)
TRAP = re.compile(r"^\S+ trapped(?: at tick \d+| before the match)?: (.*)$", re.S)
OUT_OF_FUEL = re.compile(r"^\S+ ran out of fuel")
IMPORT_REFUSAL = re.compile(r"(?:^|: )import \w+: ")
GLUE_ERROR = re.compile(r"^slot \S+: |: the handle table is full")
LOAD_REFUSALS = (
    ("compile", re.compile(r"\bmodule \S+: compile failed: ")),
    ("instantiate", re.compile(r"\bmodule \S+: instantiate failed: ")),
    ("export", re.compile(r"\bmodule \S+: (?:no function export |no global export |no memory export |export \S+ is not an i32 address)")),
    ("major", re.compile(r"\bmodule \S+: (?:built against interface |majors\.|the adapter for |\S+ \S+ of interface |import \S+ is adapted from major )")),
)
TRAP_CLASSES = (
    ("stack", re.compile(r"call stack exhausted")),
    ("unreachable", re.compile(r"unreachable")),
    ("memory_access", re.compile(r"out of bounds memory access")),
    ("divide", re.compile(r"divide by zero|integer overflow|invalid conversion to integer")),
    ("table", re.compile(r"uninitialized element|indirect call|table access|table index")),
    ("stack", re.compile(r"stack")),
)


class Error(Exception):
    pass


class HostFault(Error):
    """A fault of the host, not of the module: `reason` is a key of HOST_ERRORS, the message stays on the host."""

    def __init__(self, reason: str, message: str):
        super().__init__(message)
        self.reason = reason


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def latest_tag(prefix: str) -> str | None:
    """The newest git tag `<prefix>*` (by version), or None when there is none."""
    r = subprocess.run(["git", "tag", "-l", prefix + "*", "--sort=-v:refname"], cwd=ROOT, capture_output=True, text=True)
    tags = r.stdout.split()
    return tags[0] if r.returncode == 0 and tags else None


def kit_version() -> dict[str, str | None]:
    """The version pair of the kit the module is judged against: the newest interfaces-v* and reference-traces-v* tags."""
    return {"interfaces": latest_tag("interfaces-v"), "reference_traces": latest_tag("reference-traces-v")}


def manifest_entries(build: Path) -> list[dict]:
    return json.loads((build / "modules.json").read_text())["modules"]


def manifest_modules(build: Path) -> list[str]:
    return [m["name"] for m in manifest_entries(build)]


def blocked(subsystem: str, manifest: Path = MANIFEST) -> bool:
    """Whether the subsystem's entry of the game manifest is blocked: its default does not build to WASM, so no scenario can load a module of it, and it has no budgets."""
    for entry in json.loads(manifest.read_text())["modules"]:
        if entry["name"] == subsystem:
            try:
                return wasm_modules.is_blocked(entry)
            except ValueError as e:
                raise Error(f"{manifest}: {e}") from None
    raise Error(f"{manifest} has no module {subsystem}")


def clear_run(out: Path) -> None:
    """Remove what an earlier run left in `out`, so that a run that stops early leaves no result that is not its own."""
    for stale in ("runtime", "logs"):
        shutil.rmtree(out / stale, ignore_errors=True)
    (out / "result.json").unlink(missing_ok=True)


def prepare_replay_runtime(build: Path, subsystem: str, module: Path, directory: Path, manifest: Path = MANIFEST) -> Path:
    """A fresh directory with the files `replay_native_test.exe --wasm` reads beside the module: the wasmtime DLL, the manifest and `module` as the subsystem's."""
    for name, reason in (("wasmtime_host.dll", "host-runtime"), (REPLAYER, "host-replayer")):
        if not (build / name).is_file():
            raise HostFault(reason, f"the build has no {name}: build the replay_native_test and wasm_runtime targets")
    shutil.rmtree(directory, ignore_errors=True)
    directory.mkdir(parents=True)
    shutil.copy2(build / "wasmtime_host.dll", directory / "wasmtime_host.dll")
    shutil.copy2(manifest, directory / "modules.json")
    shutil.copyfile(module, directory / f"{subsystem}.wasm")
    return directory / f"{subsystem}.wasm"


def preflight(build: Path) -> None:
    """Refuse to start, with a HostFault, when the build has no replayer or wasmtime DLL, or wine cannot start the replayer (its usage line)."""
    for name, reason in ((REPLAYER, "host-replayer"), ("wasmtime_host.dll", "host-runtime")):
        if not (build / name).is_file():
            raise HostFault(reason, f"the build has no {name}: build the replay_native_test and wasm_runtime targets")
    try:
        r = subprocess.run(["wine", str(build / REPLAYER)], capture_output=True, text=True, errors="replace", stdin=subprocess.DEVNULL,
                           cwd=ROOT, timeout=PREFLIGHT_TIMEOUT)
    except (OSError, subprocess.TimeoutExpired) as e:
        raise HostFault("host-runtime", f"wine could not start {REPLAYER}: {e}") from None
    if REPLAYER_USAGE not in r.stdout:
        reason = "host-replayer" if WINE_MISSING_REPLAYER.search(r.stdout + r.stderr) else "host-runtime"
        raise HostFault(reason, f"wine could not start {REPLAYER} (exit {r.returncode}): {(r.stdout + r.stderr).strip()[-MESSAGE_LIMIT:]}")


def trap_class(text: str) -> str:
    """The class of a trap from the engine's text: a word of TRAP_CLASSES, or `trap` for any other."""
    if IMPORT_REFUSAL.match(text):
        return "import"
    head, found, cause = text.rpartition("wasm trap: ")
    if not found:
        return "trap"
    cause = cause.splitlines()[0] if cause else ""
    return next((name for name, pattern in TRAP_CLASSES if pattern.search(cause)), "trap")


def stop_reason(text: str) -> dict:
    """The reason of the stop message of a module (the replayer's or the game's): `module-trap` or `module-load` with its class, else `unknown`."""
    if OUT_OF_FUEL.match(text):
        return {"reason": "module-trap", "class": "out_of_fuel"}
    if m := TRAP.match(text):
        return {"reason": "module-trap", "class": trap_class(m[1])}
    if IMPORT_REFUSAL.search(text):
        return {"reason": "module-trap", "class": "import"}
    if GLUE_ERROR.search(text):
        return {"reason": "module-trap", "class": "glue"}
    for name, pattern in LOAD_REFUSALS:
        if pattern.search(text):
            return {"reason": "module-load", "class": name}
    return {"reason": "unknown"}


def failure_reason(output: str, timed_out: bool = False) -> dict:
    """Why a replayer run did not finish, as a word of a closed set (docs/module-abi.md, section 15), from its output; never a word of its text."""
    if timed_out:
        return {"reason": "host-timeout"}
    if WINE_MISSING_REPLAYER.search(output):
        return {"reason": "host-replayer"}
    if REPLAYER_BANNER not in output and WINE_FAULT.search(output):
        return {"reason": "host-runtime"}
    stops = list(REPLAYER_STOP.finditer(output))
    if not stops:
        return {"reason": "unknown"}
    stop = output[stops[-1].start(1):].strip()
    if RUNTIME_FAULT.match(stop):
        return {"reason": "host-runtime"}
    found = stop_reason(stop)
    if found["reason"] == "unknown" and FATAL_PENDING.search(output):
        return {"reason": "module-trap", "class": "fatal"}
    return found


def game_failure_reason(text: str) -> dict:
    """Why a game run of the scenario route did not finish: the reason of the first stop message of a module in its text, else `unknown`."""
    for line in text.splitlines():
        found = stop_reason(line.strip())
        if found["reason"] != "unknown":
            return found
    return {"reason": "unknown"}


def prepare_runtime(build: Path, subsystem: str, module: Path, directory: Path) -> Path:
    """A fresh runtime in `directory`: the build's files, the object list the block pointer check reads beside the map, the manifest and the other modules that are not blocked, as built, `module` as the subsystem's."""
    names = manifest_modules(build)
    if subsystem not in names:
        raise Error(f"the manifest has no module {subsystem} (it has {', '.join(names)})")
    objects = build / OBJECT_LIST
    if not objects.is_file():
        raise Error(f"the build has no {OBJECT_LIST}: the block pointer check needs the linked objects")
    shutil.rmtree(directory, ignore_errors=True)
    directory.mkdir(parents=True)
    (directory / OBJECT_LIST).parent.mkdir()
    shutil.copy2(objects, directory / OBJECT_LIST)
    for name in RUNTIME_FILES:
        if (build / name).is_file():
            shutil.copy2(build / name, directory / name)
    for entry in manifest_entries(build):
        if not wasm_modules.is_blocked(entry):
            shutil.copy2(build / f"{entry['name']}.wasm", directory / f"{entry['name']}.wasm")
    shutil.copyfile(module, directory / f"{subsystem}.wasm")
    return directory / "TotalA.exe"


def refusal(wine_log: str) -> str | None:
    """The loader's message in the stderr of a run that ended before the match, or None when there is none."""
    lines = wine_log.splitlines()
    for start, line in enumerate(lines):
        if " before the match: " in line and not re.match(r"[0-9a-f]{4}:", line):
            end = start
            while end < len(lines) and not re.match(r"[0-9a-f]{4}:", lines[end]):
                end += 1
            return "\n".join(lines[start:end]).strip()[:MESSAGE_LIMIT]
    return None


def check_module(module: Path, subsystem: str, manifest: Path | None = None) -> str | None:
    """The problems `wasm_modules.py check-module` finds in the file (a subprocess, as the build runs it), None when there are none."""
    cmd = [sys.executable, str(ROOT / "tools/wasm_modules.py"), "check-module", str(module), "--name", subsystem]
    if manifest is not None:
        cmd += ["--manifest", str(manifest)]
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    if r.returncode == 0:
        return None
    if r.returncode != 1:
        raise Error(f"check-module could not check the module (exit {r.returncode}): {(r.stderr + r.stdout).strip()[-MESSAGE_LIMIT:]}")
    return ((r.stdout + r.stderr).strip() or "check-module exited 1")[:MESSAGE_LIMIT]


def probe(exe: Path, game_dir: str | None, work: Path) -> str | None:
    """Start the game on a ten-tick scenario. The loader's message when it refuses the module, None when the game gets through the load."""
    scenario = work / "probe.txt"
    scenario.write_text(PROBE)
    cmd = [sys.executable, str(ROOT / "tools/playtest.py"), "--exe", str(exe), "--testmode", str(scenario)]
    if game_dir:
        cmd += ["--game-dir", game_dir]
    r = subprocess.run(cmd, capture_output=True, text=True)
    (work / "logs").mkdir(exist_ok=True)
    (work / "logs/probe.txt").write_text(r.stdout + r.stderr)
    run = re.search(r"^logs and screenshots: (.+)/$", r.stdout, re.M)
    message = None
    if run:
        logs = sorted((ROOT / run[1]).glob("*/wine.log"))
        message = refusal(logs[0].read_text(errors="replace")) if logs else None
        shutil.rmtree(ROOT / run[1], ignore_errors=True)
    if r.returncode != 0 and message is None:
        raise Error(f"the probe run failed without a loader message (log: {work / 'logs/probe.txt'})")
    return message if r.returncode != 0 else None


def clean_name(text: str) -> str | None:
    return text if NAME.match(text) else None


def clean_value(text: str | None) -> str | None:
    return None if text is None else text[:VALUE_LIMIT]


def parse_suite(output: str, exit_code: int, expected: list[str]) -> dict[str, dict]:
    """The verdict of each scenario from the lines of tools/reference.py's --check --end-of-tick report.

    `ok` and `DIFF` lines give equal or differs with the tick (of the end-of-tick state), and a ` [digest differs from tick T]` suffix
    gives the scenario's `digest` verdict and a ` [view differs from tick T]` suffix gives its `view` verdict, both the same way (`equal` for an `ok` line without it, not_judged for a `DIFF` line without it, as the watch prints it, and for a scenario that is not_judged or failed); the tracediff block gives the phase, the field and the value of
    b (the module's side) for the scenario it names; `<name>: failed to run` lines give failed. A scenario with no line is
    not_judged (the run stopped at the first divergence). Nothing is read from a stored or reference side."""
    verdicts: dict[str, dict] = {name: {"status": "not_judged"} for name in expected}
    blocks: dict[str, dict] = {}
    digests: dict[str, dict] = {}
    views: dict[str, dict] = {}
    current = None
    lines = output.splitlines()
    for at, line in enumerate(lines):
        if m := SCENARIO_LINE.match(line):
            name = m[2]
            if name in verdicts:
                late, seen = DIGEST_DIFFERS.search(line), VIEW_DIFFERS.search(line)
                if late:
                    digests[name] = {"status": "differs", "tick": int(late[1])}
                elif m[1] == "ok":
                    digests[name] = {"status": "equal"}
                if seen:
                    views[name] = {"status": "differs", "tick": int(seen[1])}
                elif m[1] == "ok":
                    views[name] = {"status": "equal"}
            if name in verdicts and m[1] == "ok":
                verdicts[name] = {"status": "equal"}
            elif name in verdicts:
                tick = TICK.search(m[3] or "")
                verdicts[name] = {"status": "differs", "tick": int(tick[1]) if tick else None, "phase": None, "field": None, "value": None}
        elif m := DIVERGENCE_FOR.match(line):
            current = m[1]
            blocks[current] = {}
        elif current and (m := DIVERGENCE_AT.match(line.strip())):
            blocks[current].update(tick=int(m[1]), phase=clean_name(m[2]))
        elif current and (m := DIVERGENCE_FIELD.match(line.strip())) and "field" not in blocks[current]:
            blocks[current].update(field=clean_name(m[1]), value=clean_value(m[3]))
        elif m := EARLY.match(line):
            if m[1] in verdicts:
                verdicts[m[1]] = {"status": "failed", **({"reason": "host-timeout"} if m[2] == "timeout" else game_failure_reason(m[3] or ""))}
        elif m := FAILED_TO_RUN.search(line):
            if m[1] in verdicts:
                block = [line[m.end():].lstrip(":, ")]
                for more in lines[at + 1:]:
                    if not more[:1].isspace():
                        break
                    block.append(more)
                verdicts[m[1]] = {"status": "failed", **game_failure_reason("\n".join(block))}
    for name, block in blocks.items():
        if name in verdicts and verdicts[name].get("status") in ("differs", "not_judged"):
            verdicts[name] = {"status": "differs", "tick": None, "phase": None, "field": None, "value": None, **block}
    if exit_code == 0:
        for name in verdicts:
            if verdicts[name]["status"] == "not_judged":
                verdicts[name] = {"status": "failed", "reason": "unknown"}
    for name, v in verdicts.items():
        v["digest"] = digests[name] if name in digests and v["status"] in ("equal", "differs") else {"status": "not_judged"}
        v["view"] = views[name] if name in views and v["status"] in ("equal", "differs") else {"status": "not_judged"}
    if exit_code != 0 and all(v["status"] == "equal" for v in verdicts.values()):
        verdicts["other_checks"] = {"status": "failed"}
        if any(BLOCK_POINTER_SUMMARY.match(line) for line in output.splitlines()):
            verdicts["other_checks"]["reason"] = "block-pointer"
    return verdicts


def parse_recordings(data: dict[str, dict]) -> dict[str, dict]:
    """The verdict of each recording from the JSON that tools/replay.py --json writes, through an allow-list."""
    out: dict[str, dict] = {}
    for name, v in data.items():
        if v.get("status") == "equal":
            out[name] = {"status": "equal"}
        else:
            out[name] = {"status": "differs", "line": v.get("line"), "tick": v.get("tick"), "phase": clean_name(v.get("slot") or ""),
                         "kind": clean_name(v.get("kind") or ""), "field": clean_name(v.get("field") or ""), "value": clean_value(v.get("value"))}
    return out


def clean_key(text: str) -> str | None:
    words = text.split(" ")
    return text if len(words) <= 3 and len(text) <= 200 and all(NAME.match(w) for w in words) else None


def replayed_side(detail: str) -> str | None:
    """The module's side of a replayer detail line (`recorded X, replayed Y` and its array forms), never the recorded side."""
    if m := REPLAY_ARRAY_BYTE.match(detail):
        return f"byte {m[1]}: {m[2]}"
    if m := REPLAY_ARRAY_COUNT.match(detail):
        return f"{m[1]} elements"
    if detail.startswith("recorded an array, replayed "):
        return detail[len("recorded an array, replayed "):]
    if detail.endswith(", replayed an array"):
        return "an array"
    head, sep, tail = detail.rpartition(", replayed ")
    return tail if sep and head.startswith("recorded ") else None


def parse_replay(output: str, exit_code: int) -> dict:
    """The verdict of one recording from replay_native_test's report: equal, differs at a call (its tick, slot, key and the module's value) or failed."""
    output = output.replace("\r\n", "\n")
    if m := REPLAY_DIVERGED.search(output):
        call, key = int(m[1]), m[2]
        found = {"status": "differs", "call": call, "tick": None, "phase": None, "field": clean_key(key), "value": None}
        lines = output.splitlines()
        head = re.compile(rf"call {call} (\S+) (\S+) DIFFERS$")
        for i, line in enumerate(lines):
            if h := head.match(line):
                found.update(tick=int(h[1]) if h[1].isdigit() else None, phase=clean_name(h[2]))
                for detail in lines[i + 1:]:
                    if not detail.startswith("    "):
                        break
                    if detail.startswith(f"    {key}: "):
                        found["value"] = clean_value(replayed_side(detail[len(key) + 6:]))
                        break
                break
        return found
    if exit_code == 0 and REPLAY_IDENTICAL.search(output):
        return {"status": "equal"}
    return {"status": "failed", **failure_reason(output, exit_code is None)}


def verdict_of(suite: dict[str, dict], recordings: dict[str, dict]) -> str:
    statuses = [v["status"] for v in [*suite.values(), *recordings.values()]]
    if "differs" in statuses:
        return "differs"
    return "equal" if statuses and set(statuses) == {"equal"} else "failed"


def digest_of(suite: dict[str, dict], column: str = "digest") -> str:
    """The digest (or `view`) verdict of the suite: differs when a judged scenario's differs, equal when all the judged ones are equal, else not_judged."""
    statuses = [v[column]["status"] for v in suite.values() if column in v]
    if "differs" in statuses:
        return "differs"
    return "equal" if "equal" in statuses else "not_judged"


def build_result(subsystem: str, module: Path, full: bool, kit: dict, suite: dict | None, recordings: dict | None,
                 message: str | None, seconds: float) -> dict:
    """The contents of result.json. Every value comes from a parsed field, none from a report's text."""
    result = {"format": FORMAT, "subsystem": subsystem, "module": {"sha256": sha256(module), "bytes": module.stat().st_size},
              "kit": kit, "scope": "scenarios", "budgets": "judged", "suite": "full" if full else "tier", "seconds": round(seconds)}
    if message is not None:
        result.update(verdict="refused", message=message)
        return result
    result.update(scenarios=suite or {}, recordings=recordings or {}, verdict=verdict_of(suite or {}, recordings or {}),
                  digest=digest_of(suite or {}), view=digest_of(suite or {}, "view"))
    return result


def build_recordings_result(subsystem: str, module: Path, kit: dict, recordings: dict | None, message: str | None, seconds: float,
                            budgets: bool = True) -> dict:
    """The contents of result.json for the recordings route. Every value comes from a parsed field, none from a report's text."""
    result = {"format": FORMAT, "subsystem": subsystem, "module": {"sha256": sha256(module), "bytes": module.stat().st_size},
              "kit": kit, "scope": "recordings", "budgets": "judged" if budgets else "not_judged", "rules": RULES, "seconds": round(seconds)}
    if message is not None:
        result.update(verdict="refused", message=message)
        return result
    result.update(scenarios="not_judged", recordings=recordings or {}, verdict=verdict_of({}, recordings or {}),
                  digest="not_judged", view="not_judged")
    return result


def recording_files(subsystem: str) -> list[Path]:
    return sorted((ROOT / "tests/replay" / subsystem).glob("*.txt"))


def judge_recordings(module: Path, subsystem: str, build: Path, out: Path, manifest: Path = MANIFEST) -> dict:
    """Judge the module on the recordings of tests/replay/SUBSYSTEM/ alone, one replayer run per recording; a blocked entry's without its budgets."""
    started = time.time()
    clear_run(out)
    budgets = not blocked(subsystem, manifest)
    problems = check_module(module, subsystem, None if manifest == MANIFEST else manifest)
    kit = kit_version()
    if problems is not None:
        return build_recordings_result(subsystem, module, kit, None, problems, time.time() - started, budgets)
    files = recording_files(subsystem)
    if not files:
        raise Error(f"tests/replay/{subsystem}/ has no recordings")
    preflight(build)
    wasm = prepare_replay_runtime(build, subsystem, module, out / "runtime", manifest)
    (out / "logs/recordings").mkdir(parents=True)
    recordings = {}
    for path in files:
        cmd = ["wine", str(build / REPLAYER), "--wasm", str(wasm), *([] if budgets else ["--no-budgets"]), str(path.relative_to(ROOT))]
        log = out / "logs/recordings" / path.name
        try:
            with log.open("w") as f:
                code = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, text=True, cwd=ROOT, timeout=REPLAY_TIMEOUT).returncode
        except subprocess.TimeoutExpired:
            code = None
        recordings[f"{subsystem}/{path.stem}"] = parse_replay(log.read_text(errors="replace"), code)
    return build_recordings_result(subsystem, module, kit, recordings, None, time.time() - started, budgets)


def stream(cmd: list[str], log: Path) -> tuple[int, str]:
    with log.open("w") as f:
        r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, text=True, cwd=ROOT)
    return r.returncode, log.read_text(errors="replace")


def judge(module: Path, subsystem: str, full: bool, build: Path, out: Path, game_dir: str | None) -> dict:
    started = time.time()
    clear_run(out)
    problems = check_module(module, subsystem)
    if problems is not None:
        return build_result(subsystem, module, full, kit_version(), None, None, problems, time.time() - started)
    exe = prepare_runtime(build, subsystem, module, out / "runtime")
    (out / "logs").mkdir(parents=True, exist_ok=True)
    kit = kit_version()
    message = probe(exe, game_dir, out)
    if message is not None:
        return build_result(subsystem, module, full, kit, None, None, message, time.time() - started)
    check = [sys.executable, str(ROOT / "tools/reference.py"), "--check", "--end-of-tick", "--no-cache", "--exe", str(exe)]
    if not full:
        check.append("--tier")
    if game_dir:
        check += ["--game-dir", game_dir]
    code, text = stream(check, out / "logs/reference.txt")
    expected = list(reference.GATE_TIER) if not full else sorted(reference.scenario_files())
    suite = parse_suite(text, code, expected)
    verdicts = out / "logs/replay.json"
    verdicts.unlink(missing_ok=True)
    play = [sys.executable, str(ROOT / "tools/replay.py"), "--check", "--exe", str(exe), "--subsystem", subsystem, "--json", str(verdicts)]
    if game_dir:
        play += ["--game-dir", game_dir]
    _, replay_text = stream(play, out / "logs/replay.txt")
    names = [f"{subsystem}/{p.stem}" for p in replay.scenario_recordings(subsystem)]
    recordings = parse_recordings(json.loads(verdicts.read_text())) if verdicts.is_file() else {}
    for name in names:
        recordings.setdefault(name, {"status": "failed", **game_failure_reason(replay_text)})
    return build_result(subsystem, module, full, kit, suite, recordings, None, time.time() - started)


def summary(result: dict) -> str:
    if result["verdict"] == "refused":
        return "refused: " + result["message"].splitlines()[0]
    scenarios = result["scenarios"] if isinstance(result["scenarios"], dict) else {}
    bad = [f"{n}: {v['status']}" + (f" ({v['reason']}" + (f", {v['class']}" if "class" in v else "") + ")" if "reason" in v else "") + (f" at call {v['call']}, {v['field']}" if "call" in v else "")
           for n, v in {**scenarios, **result["recordings"]}.items() if v["status"] != "equal"]
    scope = " on recordings; scenarios not judged" if result["scope"] == "recordings" else ""
    scope += "; budgets not judged" if result.get("budgets") == "not_judged" else ""
    return f"{result['verdict']}{scope} ({len(bad)} not equal)" + "".join("\n  " + b for b in bad)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("module", type=Path)
    ap.add_argument("--subsystem", required=True)
    route = ap.add_mutually_exclusive_group()
    route.add_argument("--full", action="store_true", help="play the full --check instead of the tier")
    route.add_argument("--recordings-only", action="store_true", help="judge on the recordings of tests/replay/SUBSYSTEM/ alone (the route of a blocked manifest entry)")
    ap.add_argument("--out", type=Path, default=ROOT / "build/oracle", help="the directory of the run: runtime/, logs/ and result.json (default build/oracle)")
    ap.add_argument("--manifest", type=Path, help="the game manifest to judge against (default wasm/modules.json; recordings route only)")
    ap.add_argument("--build-dir", type=Path, default=ROOT / "build/modern")
    ap.add_argument("--game-dir")
    args = ap.parse_args()
    if not args.module.is_file():
        ap.error(f"{args.module} is not a file")
    manifest = args.manifest.resolve() if args.manifest else MANIFEST
    (args.out / "result.json").unlink(missing_ok=True)
    try:
        if args.recordings_only or blocked(args.subsystem, manifest):
            if args.full:
                ap.error(f"{args.subsystem}'s manifest entry is blocked: no scenario can load its module, so it is judged on its recordings alone")
            result = judge_recordings(args.module.resolve(), args.subsystem, args.build_dir.resolve(), args.out.resolve(), manifest)
        else:
            if args.manifest:
                ap.error("--manifest judges on the recordings route only: add --recordings-only")
            result = judge(args.module.resolve(), args.subsystem, args.full, args.build_dir.resolve(), args.out.resolve(), args.game_dir)
    except HostFault as e:
        print(f"oraclegate: {e}", file=sys.stderr)
        args.out.mkdir(parents=True, exist_ok=True)
        error = {"format": FORMAT, "subsystem": args.subsystem, "error": HOST_ERRORS[e.reason], "reason": e.reason}
        (args.out / "result.json").write_text(json.dumps(error, indent=1) + "\n")
        return 2
    except (Error, OSError) as e:
        print(f"oraclegate: {e}", file=sys.stderr)
        return 2
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "result.json").write_text(json.dumps(result, indent=1) + "\n")
    print(summary(result))
    print(f"oraclegate: {result['verdict']}, result in {args.out / 'result.json'}")
    return {"equal": 0, "refused": 2}.get(result["verdict"], 1)


if __name__ == "__main__":
    sys.exit(main())
