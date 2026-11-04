"""Judge a submitted module by behaviour alone (docs/module-abi.md, "Judging a module").

    uv run tools/oraclegate.py MODULE.wasm --subsystem wind_meteors [--full] [--out DIR] [--build-dir build/modern] [--game-dir DIR]

Refuses the module, before anything plays, when `tools/wasm_modules.py check-module` finds a problem in the file: the window,
table, imports, exports, kinds or size budget of docs/module-abi.md, section 5.2. Then copies the runtime files of the modern build (and objs/objects.rsp, which the block pointer check reads beside the map) into a fresh directory with MODULE.wasm in place of the subsystem's module,
starts the game on a short scenario to see whether the loader accepts the module, then plays
`tools/reference.py --check --tier` (`--full`: the full `--check`) and `tools/replay.py --check --subsystem NAME` on that
runtime. It writes DIR/result.json, built from the parsed fields of those tools' reports and nothing else: for each scenario
and recording, `equal` or the first divergence (tick, phase, field name, the module's value), the kit version and the
module's sha256. A failure of another check of the run is `other_checks: failed`, with `reason: block-pointer` when it is
the block pointer check, which judges the harness's scan and not the module's behaviour. The reference's values never reach it. The raw reports stay in DIR/logs/, which is for the host only.

Exit status: 0 when every scenario and recording is equal, 1 when one differs or failed to run, 2 when the loader refused
the module or the arguments were wrong.
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

ROOT = Path(__file__).resolve().parent.parent
FORMAT = 1
RUNTIME_FILES = ("TotalA.exe", "TotalA.map", "CMakeCache.txt", "wasmtime_host.dll", "modules.json")
OBJECT_LIST = "objs/objects.rsp"
PROBE = "game_seed 1\ncrt_seed 2\nview_seed 3\nmap The Pass\nstart_positions fixed\nticks 10\nplayer0.controller human\nplayer0.side Arm\nplayer1.controller ai\nplayer1.side Core\n"
MESSAGE_LIMIT = 2000
VALUE_LIMIT = 200
NAME = re.compile(r"[A-Za-z0-9_.#\[\]-]{1,200}$")
SCENARIO_LINE = re.compile(r"(ok|DIFF)\s+(\S+) \(\d+ ticks, [^)]*\)(?: (.*))?$")
TICK = re.compile(r"tick (\d+): ")
DIVERGENCE_FOR = re.compile(r"tracediff for (\S+): a = ")
DIVERGENCE_AT = re.compile(r"first divergence: tick (\d+), phase (\S+)$")
DIVERGENCE_FIELD = re.compile(r"first differing field: (\S+): a=(.*?) b=(.*)$")
FAILED_TO_RUN = re.compile(r"(\S+): failed to run")
BLOCK_POINTER_SUMMARY = re.compile(r"(?:\[\w+\] )?\d+ problems with the pointers out of the block")


class Error(Exception):
    pass


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


def manifest_modules(build: Path) -> list[str]:
    return [m["name"] for m in json.loads((build / "modules.json").read_text())["modules"]]


def prepare_runtime(build: Path, subsystem: str, module: Path, directory: Path) -> Path:
    """A fresh runtime in `directory`: the build's files, the object list the block pointer check reads beside the map, the manifest and the other modules as built, `module` as the subsystem's."""
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
    for name in names:
        shutil.copy2(build / f"{name}.wasm", directory / f"{name}.wasm")
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


def check_module(module: Path, subsystem: str) -> str | None:
    """The problems `wasm_modules.py check-module` finds in the file (a subprocess, as the build runs it), None when there are none."""
    cmd = [sys.executable, str(ROOT / "tools/wasm_modules.py"), "check-module", str(module), "--name", subsystem]
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
    """The verdict of each scenario from the lines of tools/reference.py's --check report.

    `ok` and `DIFF` lines give equal or differs with the tick; the tracediff block gives the phase, the field and the value of
    b (the module's side) for the scenario it names; `<name>: failed to run` lines give failed. A scenario with no line is
    not_judged (the run stopped at the first divergence). Nothing is read from a stored or reference side."""
    verdicts: dict[str, dict] = {name: {"status": "not_judged"} for name in expected}
    blocks: dict[str, dict] = {}
    current = None
    for line in output.splitlines():
        if m := SCENARIO_LINE.match(line):
            name = m[2]
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
        elif m := FAILED_TO_RUN.search(line):
            if m[1] in verdicts:
                verdicts[m[1]] = {"status": "failed"}
    for name, block in blocks.items():
        if name in verdicts and verdicts[name].get("status") in ("differs", "not_judged"):
            verdicts[name] = {"status": "differs", "tick": None, "phase": None, "field": None, "value": None, **block}
    if exit_code == 0:
        for name in verdicts:
            if verdicts[name]["status"] == "not_judged":
                verdicts[name] = {"status": "failed"}
    elif all(v["status"] == "equal" for v in verdicts.values()):
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


def verdict_of(suite: dict[str, dict], recordings: dict[str, dict]) -> str:
    statuses = [v["status"] for v in [*suite.values(), *recordings.values()]]
    if "differs" in statuses:
        return "differs"
    return "equal" if statuses and set(statuses) == {"equal"} else "failed"


def build_result(subsystem: str, module: Path, full: bool, kit: dict, suite: dict | None, recordings: dict | None,
                 message: str | None, seconds: float) -> dict:
    """The contents of result.json. Every value comes from a parsed field, none from a report's text."""
    result = {"format": FORMAT, "subsystem": subsystem, "module": {"sha256": sha256(module), "bytes": module.stat().st_size},
              "kit": kit, "suite": "full" if full else "tier", "seconds": round(seconds)}
    if message is not None:
        result.update(verdict="refused", message=message)
        return result
    result.update(scenarios=suite or {}, recordings=recordings or {}, verdict=verdict_of(suite or {}, recordings or {}))
    return result


def stream(cmd: list[str], log: Path) -> tuple[int, str]:
    with log.open("w") as f:
        r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, text=True, cwd=ROOT)
    return r.returncode, log.read_text(errors="replace")


def judge(module: Path, subsystem: str, full: bool, build: Path, out: Path, game_dir: str | None) -> dict:
    started = time.time()
    for stale in ("runtime", "logs"):
        shutil.rmtree(out / stale, ignore_errors=True)
    problems = check_module(module, subsystem)
    if problems is not None:
        return build_result(subsystem, module, full, kit_version(), None, None, problems, time.time() - started)
    exe = prepare_runtime(build, subsystem, module, out / "runtime")
    (out / "logs").mkdir(parents=True, exist_ok=True)
    kit = kit_version()
    message = probe(exe, game_dir, out)
    if message is not None:
        return build_result(subsystem, module, full, kit, None, None, message, time.time() - started)
    check = [sys.executable, str(ROOT / "tools/reference.py"), "--check", "--no-cache", "--exe", str(exe)]
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
    with (out / "logs/replay.txt").open("w") as f:
        subprocess.run(play, stdout=f, stderr=subprocess.STDOUT, text=True, cwd=ROOT)
    names = [f"{subsystem}/{p.stem}" for p in sorted((replay.RECORDINGS / subsystem).glob("*.txt"))]
    recordings = parse_recordings(json.loads(verdicts.read_text())) if verdicts.is_file() else {}
    for name in names:
        recordings.setdefault(name, {"status": "failed"})
    return build_result(subsystem, module, full, kit, suite, recordings, None, time.time() - started)


def summary(result: dict) -> str:
    if result["verdict"] == "refused":
        return "refused: " + result["message"].splitlines()[0]
    bad = [f"{n}: {v['status']}" + (f" ({v['reason']})" if "reason" in v else "") for n, v in {**result["scenarios"], **result["recordings"]}.items() if v["status"] != "equal"]
    return f"{result['verdict']} ({len(bad)} not equal)" + "".join("\n  " + b for b in bad)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("module", type=Path)
    ap.add_argument("--subsystem", required=True)
    ap.add_argument("--full", action="store_true", help="play the full --check instead of the tier")
    ap.add_argument("--out", type=Path, default=ROOT / "build/oracle", help="the directory of the run: runtime/, logs/ and result.json (default build/oracle)")
    ap.add_argument("--build-dir", type=Path, default=ROOT / "build/modern")
    ap.add_argument("--game-dir")
    args = ap.parse_args()
    if not args.module.is_file():
        ap.error(f"{args.module} is not a file")
    try:
        result = judge(args.module.resolve(), args.subsystem, args.full, args.build_dir.resolve(), args.out.resolve(), args.game_dir)
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
