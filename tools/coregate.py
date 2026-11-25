"""Judge a clean-room engine core by its behaviour on the stored vectors of reference/core/ (#1041).

    uv run tools/coregate.py CORE --sandbox IMAGE --module MODULE.wasm [--group G] [--out DIR]   # a submitted core
    uv run tools/coregate.py --trusted [--group G] [--out DIR]                                     # the in-repo engine
    uv run tools/coregate.py record                                                                # remake reference/core/

A core answers `CORE GROUP CALLS OUT` as docs/core-gate.md states. Each vector of reference/core/<group>.json is one run:
the gate copies reference/core/ to an inputs directory (unpacking each `.xz`, adding data/trace_schema.json and the
module as `module.wasm`), writes the vector's calls to CALLS, runs the core there, and compares each line of OUT with the
stored result on the host. The `files` group is the files gate's (tools/filesgate.py), run on the same core.

A submitted core runs only inside a machine of the clean-room image with no network (tools/cleanroom/coresandbox.py);
nothing of the stored values enters it. `--trusted` runs the in-repo engine, tools/coreadapter.py, directly, and takes
no CORE: a submitted core is never run on the host. `record` writes the vectors of tools/corecases/ from the in-repo
engine, and refuses a vector that two runs disagree on.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import lzma
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import filesgate
from filesgate import CouldNotRun, compare, kit_tag, parse_out, same

ROOT = Path(__file__).resolve().parent.parent
VECTORS = ROOT / "reference" / "core"
CASES = ROOT / "tools" / "corecases"
ADAPTER = ROOT / "tools" / "coreadapter.py"
SCHEMA = ROOT / "data" / "trace_schema.json"
TRUSTED_MODULE = ROOT / "build" / "modern" / "wind_meteors.wasm"
FORMAT = 1
TIMEOUT = 120
JUDGED = ("hash", "rand", "events", "commands", "loader", "fuel", "host")
GROUPS = (*JUDGED, "files", "play")


def calls_text(calls: list[list[str]]) -> str:
    return "".join("\t".join(c) + "\n" for c in calls)


def make_inputs(where: Path, module: Path) -> None:
    """The inputs directory: reference/core/ with each .xz unpacked, the trace schema and the module."""
    for src in VECTORS.rglob("*"):
        if src.is_dir() or src.parent == VECTORS:
            continue
        dest = where / src.relative_to(VECTORS)
        dest.parent.mkdir(parents=True, exist_ok=True)
        if src.suffix == ".xz":
            dest.with_suffix("").write_bytes(lzma.decompress(src.read_bytes()))
        else:
            shutil.copyfile(src, dest)
    shutil.copyfile(SCHEMA, where / "trace_schema.json")
    shutil.copyfile(module, where / "module.wasm")


def run_vector(core: list[str], inputs: Path, group: str, index: int, calls: list[list[str]], env=None) -> dict:
    """Run one vector in the inputs directory: {"lines": [...]} or {"failed": reason}."""
    runs = inputs / "runs"
    cpath, opath = runs / ("%s-%d.calls" % (group, index)), runs / ("%s-%d.out" % (group, index))
    cpath.write_text(calls_text(calls), newline="")
    try:
        proc = subprocess.run(core + [group, str(cpath), str(opath)], cwd=inputs, stdin=subprocess.DEVNULL,
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=TIMEOUT, env=env)
    except subprocess.TimeoutExpired:
        return {"failed": "timed out after %d s" % TIMEOUT}
    except OSError as e:
        return {"failed": "could not start the core: %s" % e}
    if proc.returncode != 0:
        return {"failed": "exit status %d" % proc.returncode}
    if not opath.is_file():
        return {"failed": "no output file"}
    try:
        return {"lines": parse_out(opath.read_text(encoding="latin-1"))}
    except ValueError as e:
        return {"failed": "malformed output: %s" % e}


def stored(groups) -> dict[str, dict]:
    try:
        return {g: json.loads((VECTORS / (g + ".json")).read_text()) for g in groups}
    except (OSError, ValueError) as e:
        raise CouldNotRun("coregate: could not read the stored vectors: %s" % e)


def status_of(states: list[str]) -> str:
    return "equal" if all(s == "equal" for s in states) else "failed" if "failed" in states else "differs"


def judge(core, module: Path, groups=GROUPS, scratch: Path | None = None, env=None, jobs: int = 8) -> dict:
    """result.json (without `core`) on the stored vectors; `core(inputs)` is the command that runs the core there."""
    start = time.time()
    judged = [g for g in JUDGED if g in groups]
    data = stored(judged)
    vectors: dict[str, dict] = {}
    group_status: dict[str, dict] = {}
    if scratch is not None:
        scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="coregate-", dir=scratch) as tmp:
        inputs = Path(tmp) / "inputs"
        (inputs / "runs").mkdir(parents=True)
        make_inputs(inputs, module)
        jobs_list = [(g, i, v) for g in judged for i, v in enumerate(data[g]["vectors"])]
        with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
            runs = list(pool.map(lambda j: run_vector(core(inputs), inputs, j[0], j[1], j[2]["calls"], env), jobs_list))
        for (g, _, v), run in zip(jobs_list, runs):
            vectors["%s/%s" % (g, v["name"])] = {"status": "failed", "reason": run["failed"]} if "failed" in run \
                else compare(v["calls"], v["results"], run["lines"])
        for g in judged:
            states = [vectors["%s/%s" % (g, v["name"])]["status"] for v in data[g]["vectors"]]
            group_status[g] = {"status": status_of(states), "vectors": len(states)}
        if "files" in groups:
            old = tempfile.tempdir
            tempfile.tempdir = tmp
            try:
                files = filesgate.judge(core(inputs), env=env)
                group_status["files"] = {"status": files["verdict"], "vectors": len(files["vectors"]),
                                         "groups": files["groups"]}
                vectors.update({"files/" + k: v for k, v in files["vectors"].items()})
            except CouldNotRun as e:
                group_status["files"] = {"status": "failed", "vectors": 0, "reason": str(e)}
            finally:
                tempfile.tempdir = old
    if "play" in groups:
        group_status["play"] = {"status": "not_judged"}
    return {
        "format": FORMAT,
        "verdict": status_of([s["status"] for g, s in group_status.items() if g != "play"]),
        "suite": "core",
        "kit": {"interfaces": kit_tag("interfaces"), "reference_traces": kit_tag("reference-traces")},
        "seconds": round(time.time() - start, 1),
        "groups": group_status,
        "vectors": vectors,
    }


def trusted_core(inputs: Path | None = None) -> list[str]:
    return [sys.executable, str(ADAPTER)]


def refused(message: str) -> dict:
    return {"format": FORMAT, "verdict": "refused", "suite": "core",
            "kit": {"interfaces": kit_tag("interfaces"), "reference_traces": kit_tag("reference-traces")},
            "seconds": 0, "message": message}


def judge_submitted(core: Path, module: Path, image: Path, groups=GROUPS) -> dict:
    """result.json for a submitted core, run in a sandbox of the clean-room image."""
    sys.path.insert(0, str(ROOT / "tools" / "cleanroom"))
    import coresandbox

    data = core.read_bytes()
    about = {"sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data)}
    reason = coresandbox.is_core_executable(core)
    if reason is not None:
        return {**refused("the core is %s" % reason), "core": about}
    if module.read_bytes()[:4] != b"\0asm":
        return {**refused("the module is not a WebAssembly binary"), "core": about}
    scratch = ROOT / "build" / "scratch" / "coregate"
    scratch.mkdir(parents=True, exist_ok=True)
    share = Path(tempfile.mkdtemp(prefix="share-", dir=scratch))
    try:
        exe = share / "core"
        exe.write_bytes(data)
        exe.chmod(0o755)
        try:
            with coresandbox.Sandbox(image, share) as box:
                result = judge(lambda inputs: [*box.prefix(), "/bin/sh", "-c", 'cd "$0" && exec "$@"', str(inputs),
                                               str(exe)], module, groups, scratch=share)
        except coresandbox.SandboxRefused as e:
            return {**refused("the sandbox could not start: %s" % e), "core": about}
    finally:
        shutil.rmtree(share, ignore_errors=True)
    return {**result, "core": about}


def record() -> None:
    """Write reference/core/<group>.json from the in-repo engine; refuse when two runs disagree."""
    with tempfile.TemporaryDirectory(prefix="coregate-rec-") as tmp:
        inputs = Path(tmp)
        (inputs / "runs").mkdir()
        make_inputs(inputs, TRUSTED_MODULE)
        for g in JUDGED:
            cases = json.loads((CASES / (g + ".json")).read_text())
            jobs = [(i, c) for i, c in enumerate(cases)]
            with concurrent.futures.ThreadPoolExecutor(8) as pool:
                runs = list(pool.map(lambda j: [run_vector(trusted_core(), inputs, g, 2 * j[0] + k, j[1]["calls"])
                                                for k in range(2)], jobs))
            vectors = []
            for (_, case), pair in zip(jobs, runs):
                for run in pair:
                    if "failed" in run:
                        raise SystemExit("coregate: %s/%s failed: %s" % (g, case["name"], run["failed"]))
                if not same(pair[0]["lines"], pair[1]["lines"]):
                    raise SystemExit("coregate: %s/%s differs between two runs" % (g, case["name"]))
                vectors.append({"name": case["name"], "calls": case["calls"], "results": pair[0]["lines"]})
            (VECTORS / (g + ".json")).write_text(dump({"format": FORMAT, "group": g, "vectors": vectors}))
            print("coregate: wrote %s (%d vectors)" % (g, len(vectors)))


def dump(body: dict) -> str:
    """A vector file: one vector per block, one call and one result per line, so a diff names the line."""
    lines = ['{"format": %d, "group": %s, "vectors": [' % (body["format"], json.dumps(body["group"]))]
    for i, v in enumerate(body["vectors"]):
        lines.append(' {"name": %s,' % json.dumps(v["name"]))
        lines.append('  "calls": [')
        lines.append(",\n".join("   " + json.dumps(c) for c in v["calls"]))
        lines.append('  ],')
        lines.append('  "results": [')
        lines.append(",\n".join("   " + json.dumps(r) for r in v["results"]))
        lines.append("  ]}" + ("," if i + 1 < len(body["vectors"]) else ""))
    lines.append("]}")
    return "\n".join(lines) + "\n"


def report(result: dict, out: Path) -> int:
    out.mkdir(parents=True, exist_ok=True)
    (out / "result.json").write_text(json.dumps(result, indent=1) + "\n")
    if result["verdict"] == "refused":
        print("coregate: refused: %s" % result["message"])
        return 2
    for g, s in result["groups"].items():
        print("%-10s %s" % (g, s["status"]))
    for name, v in result["vectors"].items():
        if v["status"] != "equal":
            print("  %s: %s %s" % (name, v["status"], v.get("line", v.get("reason", ""))))
    print("coregate: %s, result in %s" % (result["verdict"], out / "result.json"))
    return 0 if result["verdict"] == "equal" else 1


def main() -> None:
    if sys.argv[1:2] == ["record"]:
        os.environ.setdefault("WINEDEBUG", "-all")
        record()
        return
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("core", type=Path, nargs="?")
    parser.add_argument("--module", type=Path)
    parser.add_argument("--sandbox", type=Path, metavar="IMAGE")
    parser.add_argument("--trusted", action="store_true")
    parser.add_argument("--group", action="append", choices=GROUPS)
    parser.add_argument("--out", type=Path, default=ROOT / "build" / "core-gate")
    args = parser.parse_args()
    groups = tuple(args.group or GROUPS)
    os.environ.setdefault("WINEDEBUG", "-all")
    try:
        if args.trusted:
            if args.core or args.sandbox or args.module:
                parser.error("--trusted runs the in-repo engine and takes no CORE, --sandbox or --module")
            result = judge(trusted_core, TRUSTED_MODULE, groups)
        else:
            if not (args.core and args.sandbox and args.module):
                parser.error("a core runs only in the sandbox: give CORE, --sandbox IMAGE and --module MODULE.wasm")
            result = judge_submitted(args.core.resolve(), args.module.resolve(), args.sandbox.resolve(), groups)
    except CouldNotRun as e:
        print(e, file=sys.stderr)
        sys.exit(2)
    sys.exit(report(result, args.out))


if __name__ == "__main__":
    main()
