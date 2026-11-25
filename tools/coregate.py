"""Judge a clean-room engine core by its behaviour on the stored vectors of reference/core/ (#1041).

    uv run tools/coregate.py CORE --sandbox IMAGE --module MODULE.wasm [--group G] [--out DIR]   # a submitted core
    uv run tools/coregate.py --trusted [--group G] [--out DIR]                                     # the in-repo engine
    uv run tools/coregate.py record                                                                # remake reference/core/
    uv run tools/coregate.py --check-frozen [--base REF]                                           # refuse a change to a stored vector

A core answers `CORE GROUP CALLS OUT` as docs/core-gate.md states. Each vector of reference/core/<group>.json is one run:
the gate copies reference/core/ to an inputs directory (unpacking each `.xz`, adding data/trace_schema.json and the
module as `module.wasm`, and the files gate's fixtures), writes every vector's CALLS, and only then runs the core there,
once for each vector. It writes nothing into that directory once the core has run, and reads each OUT through
descriptors taken before, following no link: a core shares the directory read-write, and a link it makes must not
lead the host to a file of its own. Each line of OUT is compared with the stored result on the host. The `files` group
is the files gate's vectors (tools/filesgate.py), run the same way. The `runtime` group's vectors are the entries of
tests/runtime/vectors/, one call per record: the core gets the record's inputs, and the gate compares its outputs.

A submitted core runs only inside a machine of the clean-room image with no network (tools/cleanroom/coresandbox.py);
nothing of the stored values enters it. `--trusted` runs the in-repo engine, tools/coreadapter.py, directly, and takes
no CORE: a submitted core is never run on the host. `record` writes the vectors of tools/corecases/ from the in-repo
engine, and refuses a vector that two runs disagree on.

`--check-frozen` (#1089) compares reference/core/ with REF (default: the merge base of HEAD and origin/moddable) and
fails on a vector of <group>.json that changed or was removed, keyed by its name, and on a file of reference/core/<group>/
that changed or was removed. Added vectors, files and groups pass. A group may change only when the pull request adds or
edits its row in data/frozen_interfaces.csv (`header` reference/core/<group>.json, with a decision): a row left from an
earlier change unfreezes nothing.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import lzma
import os
import re
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import filesgate
from abidiff import frozen_rows
from filesgate import CouldNotRun, compare, kit_tag, parse_out, same

ROOT = Path(__file__).resolve().parent.parent
VECTORS = ROOT / "reference" / "core"
CASES = ROOT / "tools" / "corecases"
RUNTIME = ROOT / "tests" / "runtime" / "vectors"
ADAPTER = ROOT / "tools" / "coreadapter.py"
SCHEMA = ROOT / "data" / "trace_schema.json"
TRUSTED_MODULE = ROOT / "build" / "modern" / "wind_meteors.wasm"
FORMAT = 1
TIMEOUT = 120
OUT_LIMIT = 256 << 20
JUDGED = ("hash", "rand", "events", "commands", "loader", "fuel", "host", "console", "runtime")
RECORDED = tuple(g for g in JUDGED if g != "runtime")
FIXED = {"i32": "<i", "u32": "<I", "f64": "<Q"}
BYTES = re.compile(r"bytes([1-9][0-9]*)")
GROUPS = (*JUDGED, "files", "play")
BASE_BRANCH = "origin/moddable"
FROZEN = "data/frozen_interfaces.csv"


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


def runtime_layouts() -> dict[str, tuple[list[str], list[str]]]:
    """Each entry of tests/runtime/vectors/layouts.txt: its input types and its output types."""
    layouts = {}
    for line in (RUNTIME / "layouts.txt").read_text().splitlines():
        if line.strip():
            entry, ins, outs = line.split()
            layouts[entry] = (ins.split(","), outs.split(","))
    return layouts


def read_field(data: bytes, pos: int, kind: str):
    """One field of a runtime record at pos: (value, next pos); integers stay integers, str and bytes<N> are hex."""
    if kind in FIXED:
        return struct.unpack_from(FIXED[kind], data, pos)[0], pos + struct.calcsize(FIXED[kind])
    if kind == "str":
        n = struct.unpack_from("<I", data, pos)[0]
        start, pos = pos + 4, pos + 4 + n
    else:
        m = BYTES.fullmatch(kind)
        if m is None:
            raise ValueError("unknown field type %s" % kind)
        start, pos = pos, pos + int(m.group(1))
    if pos > len(data):
        raise ValueError("a record runs past the end of the file")
    return data[start:pos].hex(), pos


def runtime_vectors() -> dict:
    """The runtime group: one vector per entry, one call per record, its inputs as the call and its outputs as the result."""
    vectors = []
    for entry, (ins, outs) in runtime_layouts().items():
        data = (RUNTIME / (entry + ".bin")).read_bytes()
        calls, results, pos = [], [], 0
        try:
            while pos < len(data):
                inputs, outputs = [], []
                for kind in ins:
                    value, pos = read_field(data, pos, kind)
                    inputs.append(value)
                for kind in outs:
                    value, pos = read_field(data, pos, kind)
                    outputs.append(value)
                calls.append([entry, json.dumps(inputs)])
                results.append([entry, outputs])
        except (struct.error, ValueError) as e:
            raise ValueError("%s.bin: %s" % (entry, e))
        vectors.append({"name": entry, "calls": calls, "results": results})
    return {"format": FORMAT, "group": "runtime", "vectors": vectors}


def stored(groups) -> dict[str, dict]:
    try:
        return {g: runtime_vectors() if g == "runtime" else json.loads((VECTORS / (g + ".json")).read_text())
                for g in groups}
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
        cases = {g: json.loads((CASES / (g + ".json")).read_text()) for g in RECORDED}
        work = [(g, c) for g in RECORDED for c in cases[g] for _ in range(2)]
        runs = run_all(trusted_core, inputs, [(g, [g], c["calls"]) for g, c in work])
        vectors: dict[str, list] = {g: [] for g in RECORDED}
        for k in range(0, len(work), 2):
            g, case = work[k]
            pair = runs[k:k + 2]
            for run in pair:
                if "failed" in run:
                    raise SystemExit("coregate: %s/%s failed: %s" % (g, case["name"], run["failed"]))
            if not same(pair[0]["lines"], pair[1]["lines"]):
                raise SystemExit("coregate: %s/%s differs between two runs" % (g, case["name"]))
            vectors[g].append({"name": case["name"], "calls": case["calls"], "results": pair[0]["lines"]})
        for g in RECORDED:
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


def git_bytes(*args: str) -> bytes | None:
    """The output of a git command, or None when it fails."""
    run = subprocess.run(["git", *args], cwd=ROOT, capture_output=True)
    return run.stdout if run.returncode == 0 else None


def vector_group(path: str) -> str | None:
    """The group a path of reference/core/ belongs to: <group>.json, or a file under <group>/."""
    parts = path.split("/")
    if len(parts) == 3 and parts[2].endswith(".json"):
        return parts[2][:-len(".json")]
    return parts[2] if len(parts) > 3 else None


def changed_vectors(old: bytes, new: bytes | None) -> list[str]:
    """The names of the vectors of a stored group file that new changes or removes."""
    have = {v["name"]: v for v in json.loads(new)["vectors"]} if new is not None else {}
    out = []
    for v in json.loads(old)["vectors"]:
        if v["name"] not in have:
            out.append("vector %s removed" % v["name"])
        elif have[v["name"]] != v:
            out.append("vector %s changed" % v["name"])
    return out


def unfrozen_groups(old_rows: dict[str, dict], new_rows: dict[str, dict]) -> set[str]:
    """The groups whose row of data/frozen_interfaces.csv is added or edited, with a decision."""
    return {vector_group(path) for path, row in new_rows.items()
            if path.startswith("reference/core/") and row.get("decision") and old_rows.get(path) != row}


def frozen_failures(base: dict[str, bytes], head: dict[str, bytes | None], unfrozen: set[str]) -> list[str]:
    """One line for each change to an existing vector or input file of a group not unfrozen. base holds the changed
    paths that exist at the baseline, head the same paths now (None when removed)."""
    out = []
    for path in sorted(base):
        group = vector_group(path)
        if group is None or group in unfrozen:
            continue
        new = head.get(path)
        if path.count("/") == 2:
            out += ["%s: %s" % (path, line) for line in changed_vectors(base[path], new)]
        elif new != base[path]:
            out.append("%s: %s" % (path, "removed" if new is None else "changed"))
    return out


def check_frozen(ref: str | None) -> int:
    if ref is None:
        found = git_bytes("merge-base", "HEAD", BASE_BRANCH)
        if found is None:
            print("coregate: no merge base with %s" % BASE_BRANCH, file=sys.stderr)
            return 2
        ref = found.decode().strip()
    listed = git_bytes("diff", "--no-renames", "--name-only", ref, "--", "reference/core/")
    if listed is None:
        print("coregate: can't diff reference/core/ with %s" % ref, file=sys.stderr)
        return 2
    base: dict[str, bytes] = {}
    head: dict[str, bytes | None] = {}
    for path in listed.decode().splitlines():
        old = git_bytes("show", "%s:%s" % (ref, path))
        if old is None:
            continue
        base[path] = old
        head[path] = (ROOT / path).read_bytes() if (ROOT / path).exists() else None
    old_rows = frozen_rows((git_bytes("show", "%s:%s" % (ref, FROZEN)) or b"").decode())
    new_rows = frozen_rows((ROOT / FROZEN).read_text()) if (ROOT / FROZEN).exists() else {}
    unfrozen = unfrozen_groups(old_rows, new_rows)
    failures = frozen_failures(base, head, unfrozen)
    for line in failures:
        print("  " + line)
    if failures:
        print("coregate: frozen vectors changed since %s: a group changes only with its row in %s and a decision" % (ref[:12], FROZEN))
        return 1
    print("coregate: no frozen vector changed since %s%s" % (
        ref[:12], " (unfrozen by %s: %s)" % (FROZEN, ", ".join(sorted(unfrozen))) if unfrozen else ""))
    return 0


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
    parser.add_argument("--check-frozen", action="store_true")
    parser.add_argument("--base", metavar="REF")
    args = parser.parse_args()
    if args.check_frozen:
        if args.core or args.sandbox or args.module or args.trusted:
            parser.error("--check-frozen takes no CORE, --sandbox, --module or --trusted")
        sys.exit(check_frozen(args.base))
    if args.base:
        parser.error("--base goes with --check-frozen")
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
