"""Judge a clean-room engine core by its behaviour on the stored vectors of reference/core/ (#1041).

    uv run tools/coregate.py CORE --sandbox IMAGE --module MODULE.wasm [--group G] [--out DIR]   # a submitted core
    uv run tools/coregate.py --trusted [--group G] [--out DIR]                                     # the in-repo engine
    uv run tools/coregate.py record                                                                # remake reference/core/

A core answers `CORE GROUP CALLS OUT` as docs/core-gate.md states. Each vector of reference/core/<group>.json is one run:
the gate copies reference/core/ to an inputs directory (unpacking each `.xz`, adding data/trace_schema.json and the
module as `module.wasm`, and the files gate's fixtures), writes every vector's CALLS, and only then runs the core there,
once for each vector. It writes nothing into that directory once the core has run, and reads each OUT through
descriptors taken before, following no link: a core shares the directory read-write, and a link it makes must not
lead the host to a file of its own. Each line of OUT is compared with the stored result on the host. The `files` group
is the files gate's vectors (tools/filesgate.py), run the same way.

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
import stat
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
OUT_LIMIT = 256 << 20
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


def write_calls(inputs: Path, jobs: list[tuple[str, list[str], list[list[str]]]]) -> None:
    """Every vector's calls file, written before the core first runs: the gate writes nothing into the share after."""
    for n, (_, _, calls) in enumerate(jobs):
        (inputs / "runs" / ("%d.calls" % n)).write_text(calls_text(calls), newline="")


def read_out(inputs_fd: int, name: str) -> str:
    """OUT through descriptors taken before the core ran, following no link the core made: a regular file, capped."""
    runs = os.open("runs", os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=inputs_fd)
    try:
        fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK, dir_fd=runs)
    finally:
        os.close(runs)
    try:
        st = os.fstat(fd)
        if not stat.S_ISREG(st.st_mode):
            raise NotRegular()
        if st.st_size > OUT_LIMIT:
            raise ValueError("OUT is larger than %d bytes" % OUT_LIMIT)
        with os.fdopen(fd, "rb", closefd=False) as f:
            data = f.read(OUT_LIMIT + 1)
        if len(data) > OUT_LIMIT:
            raise ValueError("OUT is larger than %d bytes" % OUT_LIMIT)
        return data.decode("latin-1")
    finally:
        os.close(fd)


class NotRegular(Exception):
    pass


def run_one(argv: list[str], inputs: Path, inputs_fd: int, n: int, env=None) -> dict:
    """Run one vector in the inputs directory: {"lines": [...]} or {"failed": reason}."""
    runs = inputs / "runs"
    try:
        proc = subprocess.run(argv + [str(runs / ("%d.calls" % n)), str(runs / ("%d.out" % n))], cwd=inputs,
                              stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                              timeout=TIMEOUT, env=env)
    except subprocess.TimeoutExpired:
        return {"failed": "timed out after %d s" % TIMEOUT}
    except OSError as e:
        return {"failed": "could not start the core: %s" % e}
    if proc.returncode != 0:
        return {"failed": "exit status %d" % proc.returncode}
    try:
        text = read_out(inputs_fd, "%d.out" % n)
    except FileNotFoundError:
        return {"failed": "no output file"}
    except (NotRegular, OSError):
        return {"failed": "OUT is not a regular file"}
    except ValueError as e:
        return {"failed": str(e)}
    try:
        return {"lines": parse_out(text)}
    except ValueError as e:
        return {"failed": "malformed output: %s" % e}


def run_all(core, inputs: Path, jobs: list[tuple[str, list[str], list[list[str]]]], env=None, workers: int = 8) -> list[dict]:
    """Run each (key, arguments before CALLS and OUT, calls) job; every host input is written before the first run."""
    write_calls(inputs, jobs)
    inputs_fd = os.open(inputs, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        argv = core(inputs)
        with concurrent.futures.ThreadPoolExecutor(workers) as pool:
            return list(pool.map(lambda nj: run_one(argv + nj[1][1], inputs, inputs_fd, nj[0], env), enumerate(jobs)))
    finally:
        os.close(inputs_fd)


def stored(groups) -> dict[str, dict]:
    try:
        return {g: json.loads((VECTORS / (g + ".json")).read_text()) for g in groups}
    except (OSError, ValueError) as e:
        raise CouldNotRun("coregate: could not read the stored vectors: %s" % e)


def status_of(states: list[str]) -> str:
    return "equal" if all(s == "equal" for s in states) else "failed" if "failed" in states else "differs"


def verdict(calls: list[list[str]], want: list, run: dict) -> dict:
    return {"status": "failed", "reason": run["failed"]} if "failed" in run else compare(calls, want, run["lines"])


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
        work = [("%s/%s" % (g, v["name"]), [g], v) for g in judged for v in data[g]["vectors"]]
        files_data, files_problem = {}, None
        if "files" in groups:
            fixtures = inputs / "fixtures"
            try:
                files_data = filesgate.stored_groups()
                problems = filesgate.build_fixtures(fixtures)
                if problems:
                    raise CouldNotRun("filesgate: the rebuilt fixtures differ from reference/files/fixtures.json:\n"
                                      + "\n".join(problems))
            except CouldNotRun as e:
                files_data, files_problem = {}, str(e)
            work += [("files/%s/%s" % (fg, v["name"]), ["files", "--game", str(fixtures / v["fixture"])], v)
                     for fg in filesgate.GROUPS if fg in files_data for v in files_data[fg]["vectors"]]
        runs = run_all(core, inputs, [(key, args, v["calls"]) for key, args, v in work], env, jobs)
        for (key, _, v), run in zip(work, runs):
            vectors[key] = verdict(v["calls"], v["results"], run)
        for g in judged:
            states = [vectors["%s/%s" % (g, v["name"])]["status"] for v in data[g]["vectors"]]
            group_status[g] = {"status": status_of(states), "vectors": len(states)}
        if files_problem is not None:
            group_status["files"] = {"status": "failed", "vectors": 0, "reason": files_problem}
        elif "files" in groups:
            inner = {}
            for fg in filesgate.GROUPS:
                states = [vectors["files/%s/%s" % (fg, v["name"])]["status"] for v in files_data[fg]["vectors"]]
                inner[fg] = {"status": status_of(states), "vectors": len(states)}
            group_status["files"] = {"status": status_of([s["status"] for s in inner.values()]),
                                     "vectors": sum(s["vectors"] for s in inner.values()), "groups": inner}
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
        cases = {g: json.loads((CASES / (g + ".json")).read_text()) for g in JUDGED}
        work = [(g, c) for g in JUDGED for c in cases[g] for _ in range(2)]
        runs = run_all(trusted_core, inputs, [(g, [g], c["calls"]) for g, c in work])
        vectors: dict[str, list] = {g: [] for g in JUDGED}
        for k in range(0, len(work), 2):
            g, case = work[k]
            pair = runs[k:k + 2]
            for run in pair:
                if "failed" in run:
                    raise SystemExit("coregate: %s/%s failed: %s" % (g, case["name"], run["failed"]))
            if not same(pair[0]["lines"], pair[1]["lines"]):
                raise SystemExit("coregate: %s/%s differs between two runs" % (g, case["name"]))
            vectors[g].append({"name": case["name"], "calls": case["calls"], "results": pair[0]["lines"]})
        for g in JUDGED:
            (VECTORS / (g + ".json")).write_text(dump({"format": FORMAT, "group": g, "vectors": vectors[g]}))
            print("coregate: wrote %s (%d vectors)" % (g, len(vectors[g])))


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
