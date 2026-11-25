"""Judge a data loader by its behaviour on the fixtures of tools/filesfixtures.py (#1046).

    uv run tools/filesgate.py check [--loader CMD] [--out DIR] [--group G]   # result.json for a loader (default: the in-repo adapter)
    uv run tools/filesgate.py record [--loader CMD]                          # remake reference/files/ with the in-repo adapter
    uv run tools/filesgate.py retail --game-dir ~/.ta --loader CMD [--reference CMD]   # listing and lookup on retail data

A loader is a program that answers `CMD files --game DIR CALLS OUT` as docs/files-gate.md states. Each vector of
reference/files/<group>.json is one run: the gate writes the vector's calls to CALLS, runs the loader in the vector's
fixture directory, and compares each line of OUT with the stored result. `result.json` holds a verdict for each group
and each vector in the fields of docs/module-abi.md section 15: a `value` is what the loader wrote, never the stored one.

The default loader is the modern build's adapter under Wine (`wine build/modern/files_adapter.exe`, or the exe
`BT_FILES_ADAPTER` names). `record` writes the
vectors from it, and a vector it can't pass is wrong. `retail` runs the listing and lookup calls of RETAIL_CALLS (and the
listing of every directory it finds) on a game directory with both loaders and prints where they part; nothing it makes
is stored. `BT_FILES_FAULT` makes the in-repo adapter inject one fault, for the negative tests of tools/test_filesgate.py.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import filesfixtures

ROOT = Path(__file__).resolve().parent.parent
VECTORS = ROOT / "reference" / "files"
ADAPTER = Path(os.environ.get("BT_FILES_ADAPTER", ROOT / "build" / "modern" / "files_adapter.exe"))
DEFAULT_LOADER = "wine " + shlex.quote(str(ADAPTER))
FORMAT = 1
TIMEOUT = 120
GROUPS = ("mounting", "lookup", "listing", "reading", "malformed", "formats", "cob", "gaf", "3do")

M = [["mount"]]


def load(*names: str) -> list[list[str]]:
    return [["load", n] for n in names]


def reads(name: str, size: int, *, loose: bool = False) -> list[list[str]]:
    """The read calls on one file: a handle walked across each block boundary inside it, to the end and back.

    No call reads an archived file at or past its end when the end is a block boundary: 3.1c reads outside its
    buffers there.
    """
    calls = [["open", name], ["inarchive", "1"], ["length", "1"], ["tell", "1"], ["read", "1", "0"],
             ["read", "1", "100"], ["tell", "1"]]
    if size > 65536 + 12:
        calls += [["read", "1", str(65536 - 100 - 6)], ["tell", "1"], ["read", "1", "12"], ["tell", "1"],
                  ["seek", "1", "65530"], ["read", "1", "1"], ["read", "1", "1"], ["read", "1", "0"]]
    if size > 131072 + 4:
        calls += [["seek", "1", "131070"], ["read", "1", "4"]]
    calls += [["seek", "1", "-1"], ["tell", "1"]]
    if loose or size % 65536:
        calls += [["read", "1", "10"]]
    calls += [["seek", "1", "0"],
              ["read", "1", str(size + 100)], ["tell", "1"], ["seek", "1", str(size - 5)], ["read", "1", "100"],
              ["tell", "1"], ["seek", "1", str(size // 3)], ["read", "1", str(size // 2)], ["tell", "1"]]
    if loose:
        calls += [["seek", "1", str(size + 50)], ["tell", "1"], ["read", "1", "10"]]
    calls += [["close", "1"], ["lengthbyname", name], ["load", name], ["readat", name, str(size - 10), "20"],
              ["readat", name, "0", "1"], ["readat", name, str(size // 2), "300"]]
    if size > 65536:
        calls += [["readat", name, "65500", "100"]]
    return calls


def matchcalls(pairs: list[tuple[str, str]]) -> list[list[str]]:
    return [["match", s, p] for s, p in pairs]


CASES: dict[str, list[tuple[str, str, list[list[str]]]]] = {
    "mounting": [
        ("base", "base", M),
        ("base_twice", "base", M + M),
        ("base_unmounted", "base", load(r"units\core.fbi", r"units\arm.fbi", "readme.txt") + M + load(r"units\core.fbi")),
        ("not_mounted", "base", M + load(r"rev32.txt", "x.hpi")),
        ("budget", "budget", M + load("id.txt", "h11.txt", "h10.txt")),
        ("budget_twice", "budget", M + M + load("id.txt", "h11.txt", "h12.txt")),
        ("malformed", "malformed", M),
    ],
    "lookup": [
        ("shadowing", "base", M + load(r"units\arm.fbi", r"units\core.fbi", r"units\only_ccx.fbi", "readme.txt",
                                       r"units\ufo.fbi", r"units\c.fbi", r"units\d.fbi", r"units\b.fbi",
                                       r"gamedata\sample.tdf", r"gamedata\sidedata.tdf", r"units\Shadow.TXT",
                                       r"units\fold.fbi")),
        ("case", "base", M + load(r"UNITS\CORE.FBI", r"Units\Core.Fbi", r"mixed\case.txt", r"MIXED\CASE.TXT",
                                  "README.TXT", r"units\shadow.txt", r"UNITS\ARM.FBI", r"units\D.fbi")),
        ("separators", "base", M + load("units/core.fbi", "units/arm.fbi", "a/b.txt", r"a\b.txt", r"\units\core.fbi",
                                        r"units\\core.fbi", r"units\core.fbi\\", "units")),
        ("duplicates", "base", M + load("dup.txt", r"twin\a.txt", r"twin\b.txt", r"blk\inner.txt", "blk")),
        ("missing", "base", M + load("nothing.txt", r"units\nothing.fbi", r"nowhere\x.txt", "", "notes")),
        ("by_name", "base", M + [["lengthbyname", n] for n in (r"units\arm.fbi", r"UNITS\CORE.FBI", "dup.txt",
                                                                 "nothing.txt", r"data\big.bin")]
         + [["open", r"units\arm.fbi"], ["inarchive", "1"], ["open", r"units\core.fbi"], ["inarchive", "2"],
            ["open", "nothing.txt"], ["close", "2"], ["close", "1"]]),
    ],
    "listing": [
        ("root", "base", M + [["find", "*", "-1", "1"], ["find", "*", "-1", "0"], ["find", "*", "0", "0"],
                              ["find", "*", "2", "1"], ["find", "*.*", "-1", "1"], ["find", "*.txt", "-1", "1"]]),
        ("units", "base", M + [["find", r"units\*", "-1", "1"], ["find", r"units\*.fbi", "-1", "1"],
                               ["find", r"units\?.fbi", "-1", "1"], ["find", r"UNITS\*", "-1", "1"],
                               ["find", r"units\*", "1", "1"], ["find", r"units\*.FBI", "-1", "0"]]),
        ("directories", "base", M + [["find", r"notes\*", "-1", "1"], ["find", r"data\*", "-1", "1"],
                                     ["find", r"twin\*", "-1", "1"], ["find", r"nowhere\*", "-1", "1"],
                                     ["find", r"blk\*", "-1", "1"], ["find", r"Mixed\*", "-1", "1"]]),
        ("counts", "base", M + [["count", "*", "0"], ["count", "*", "1"], ["count", r"units\*", "0"],
                                ["count", r"data\*", "0"], ["count", r"nowhere\*", "0"]]),
        ("lists", "base", M + [["list", "*", "0"], ["list", "*", "1"], ["list", r"units\*", "0"],
                               ["list", r"data\*.bin", "0"]]),
        ("trees", "base", M + [["tree", "units", "*.fbi", "-1", "1"], ["tree", "data", "*", "-1", "1"],
                               ["tree", "twin", "*", "-1", "1"], ["tree", "gamedata", "*.TDF", "-1", "1"],
                               ["tree", "units", "*", "0", "1"], ["tree", ".", "*.txt", "-1", "1"]]),
        ("unmounted", "base", [["find", "*", "-1", "1"], ["count", "*", "0"], ["tree", "units", "*", "-1", "1"]]),
        ("wildcards", "base", matchcalls([
            ("abc", "*"), ("abc", "a?c"), ("abc", "A*"), ("abc", "*c*"), ("abc", ""), ("", "*"), ("", ""),
            ("x.y.z", "*.*"), ("abc", "ab"), ("ab", "abc"), ("aXbXc", "*x*x*"), ("x", "**"), ("abc", "abc*"),
            ("ABC", "abc"), ("abc", "?"), ("abc", "???"), ("abc", "*?"), ("a", "*a*a"), ("aaaa", "*a*a"),
            ("units.fbi", "*.FBI"), ("a" * 150, "*a" * 60), ("a" * 150 + "b", "*" * 120 + "b"),
            ("ab*", "ab*"), ("ab?", "ab?"), ("dir\\file", "*\\*")])),
    ],
    "reading": [
        ("loose", "base", M + reads(r"data\loose.bin", 70000, loose=True)),
        ("stored", "base", M + reads(r"data\stored.bin", 70000)),
        ("stored_keyed", "base", M + reads(r"data\keyed_stored.bin", 5000)),
        ("lz77", "base", M + reads(r"data\big.bin", 150000)),
        ("lz77_keyed", "base", M + reads(r"data\keyed.bin", 90000)),
        ("deflate", "base", M + reads(r"data\big2.bin", 140000)),
        ("deflate_encrypted", "base", M + reads(r"data\enc.bin", 70000)),
        ("mixed", "base", M + reads(r"data\mixed.bin", 200000)),
        ("exact", "base", M + reads(r"data\exact.bin", 131072)),
        ("small", "base", M + [["open", r"data\small.lz"], ["read", "1", "1"], ["read", "1", "1000"],
                               ["tell", "1"], ["seek", "1", "50"], ["read", "1", "10"], ["close", "1"]]
         + load(r"data\small.lz")),
        ("empty", "base", M + [["open", r"data\empty.bin"], ["length", "1"], ["read", "1", "10"], ["close", "1"],
                               ["lengthbyname", r"data\empty.bin"], ["load", r"data\empty.bin"],
                               ["readat", r"data\empty.bin", "0", "4"]]),
        ("interleaved", "base", M + [["open", r"data\big.bin"], ["open", r"data\big.bin"], ["open", r"data\stored.bin"],
                                     ["read", "1", "70000"], ["read", "2", "10"], ["read", "3", "70000"],
                                     ["read", "1", "10"], ["seek", "2", "100000"], ["read", "2", "60000"],
                                     ["close", "1"], ["read", "2", "10"], ["close", "2"], ["close", "3"]]),
        ("small_files", "base", M + load(r"units\c.fbi", r"units\lz.fbi", r"units\ufo.fbi", r"units\b.fbi", "D.HPI")),
    ],
    "malformed": [
        ("mount", "malformed", M + load("good.txt", "year.txt", "digits.txt", r"good.txt")),
        ("bad_checksum", "malformed", M + [["open", "badsum.bin"], ["length", "1"], ["read", "1", "10"]]),
        ("bad_chunk_magic", "malformed", M + [["open", "badsqsh.bin"], ["read", "1", "10"]]),
        ("bad_method", "malformed", M + [["open", "badmethod.bin"], ["read", "1", "10"]]),
        ("bad_size", "malformed", M + [["open", "badsize.bin"], ["read", "1", "10"]]),
        ("truncated", "malformed", M + [["open", "trunc.bin"], ["length", "1"], ["read", "1", "10"], ["tell", "1"],
                                        ["close", "1"]] + load("trunc.bin")
         + [["readat", "trunc.bin", "0", "10"], ["lengthbyname", "trunc.bin"]]),
    ],
    "formats": [
        ("tdf", "base", M + [["tdf", r"gamedata\sample.tdf"], ["tdf", r"gamedata\sidedata.tdf"],
                             ["tdf", "missing.tdf"], ["tdf", r"data\empty.bin"]]),
        ("fbi", "base", M + [["tdf", r"units\arm.fbi"]]),
        ("tdf_broken", "base", M + [["tdf", r"gamedata\broken.tdf"], ["tdf", r"gamedata\sample.tdf"]]),
    ],
    "cob": [
        ("scripts", "base", M + [["cob", r"scripts\armfix.cob"], ["cob", r"SCRIPTS\ARMFIX.COB"]]),
        ("none", "base", M + [["cob", r"scripts\none.cob"], ["cob", r"scripts\negative.cob"]]),
        ("missing", "base", M + [["cob", r"scripts\nothing.cob"], ["cob", r"data\empty.bin"]]),
    ],
    "gaf": [
        ("anims", "base", M + [["gaf", r"anims\fixture.gaf"]]),
        ("count", "base", M + [["gaf", r"anims\count.gaf"], ["gaf", r"anims\negative.gaf"]]),
        ("missing", "base", M + [["gaf", r"anims\nothing.gaf"], ["gaf", r"data\empty.bin"]]),
    ],
    "3do": [
        ("model", "base", M + [["3do", r"objects3d\fixture.3do"]]),
        ("flat", "base", M + [["3do", r"objects3d\flat.3do"], ["3do", r"objects3d\negative.3do"]]),
        ("missing", "base", M + [["3do", r"objects3d\nothing.3do"], ["3do", r"data\empty.bin"]]),
    ],
}

RETAIL_CALLS = [["find", "*", "-1", "1"], ["find", r"units\*", "-1", "1"], ["count", r"units\*", "0"],
                ["list", "*", "1"], ["tree", "units", "*.fbi", "-1", "1"], ["tree", "gamedata", "*", "-1", "1"],
                ["find", r"anims\*.gaf", "-1", "1"], ["find", r"maps\*.ota", "-1", "1"]]


def loader_command(spec: str) -> list[str]:
    return shlex.split(spec)


def calls_text(calls: list[list[str]]) -> str:
    return "".join("\t".join(c) + "\n" for c in calls)


def parse_out(text: str) -> list:
    """The (op, value) of each line a loader wrote; a line that is not `op<TAB>json` is a ValueError."""
    out = []
    for line in text.splitlines():
        op, sep, value = line.partition("\t")
        if not sep:
            raise ValueError("line %d is not op<TAB>value: %r" % (len(out) + 1, line[:80]))
        out.append([op, json.loads(value)])
    return out


def run_vector(loader: list[str], game: Path, calls: list[list[str]], env=None) -> dict:
    """Run one vector: {"lines": [...]} or {"failed": reason}."""
    with tempfile.TemporaryDirectory(prefix="filesgate-") as tmp:
        cpath, opath = Path(tmp) / "calls.txt", Path(tmp) / "out.txt"
        cpath.write_text(calls_text(calls), newline="")
        try:
            proc = subprocess.run(loader + ["files", "--game", str(game), str(cpath), str(opath)],
                                  stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                  timeout=TIMEOUT, env=env)
        except subprocess.TimeoutExpired:
            return {"failed": "timed out after %d s" % TIMEOUT}
        except OSError as e:
            return {"failed": "could not start the loader: %s" % e}
        if proc.returncode != 0:
            return {"failed": "exit status %d" % proc.returncode}
        if not opath.is_file():
            return {"failed": "no output file"}
        try:
            return {"lines": parse_out(opath.read_text(encoding="latin-1"))}
        except ValueError as e:
            return {"failed": "malformed output: %s" % e}


def same(a, b) -> bool:
    """JSON equality that also holds the types apart: true is not 1, and 1.0 is not 1."""
    if type(a) is not type(b):
        return False
    if isinstance(a, list):
        return len(a) == len(b) and all(same(x, y) for x, y in zip(a, b))
    if isinstance(a, dict):
        return a.keys() == b.keys() and all(same(a[k], b[k]) for k in a)
    return a == b


class CouldNotRun(Exception):
    """The gate itself could not run (exit 2, no result)."""


def compare(calls: list[list[str]], want: list, got: list) -> dict:
    """The verdict of one vector: equal, or differs at the first line that differs (the loader's value)."""
    for i, expected in enumerate(want):
        if i >= len(got):
            return {"status": "differs", "line": i + 1, "call": "\t".join(calls[i]), "value": "missing"}
        if not same(got[i], expected):
            return {"status": "differs", "line": i + 1, "call": "\t".join(calls[i]), "value": got[i]}
    if len(got) > len(want):
        return {"status": "differs", "line": len(want) + 1, "call": "", "value": got[len(want)]}
    return {"status": "equal"}


def stored_groups(groups=GROUPS) -> dict[str, dict]:
    try:
        return {g: json.loads((VECTORS / (g + ".json")).read_text()) for g in groups}
    except (OSError, ValueError) as e:
        raise CouldNotRun("filesgate: could not read the stored vectors: %s" % e)


def build_fixtures(where: Path) -> list[str]:
    filesfixtures.build(where)
    stored = json.loads((VECTORS / "fixtures.json").read_text())["fixtures"]
    return filesfixtures.check(where, stored)


def judge(loader: list[str], groups=GROUPS, env=None, jobs: int = 8) -> dict:
    """result.json for a loader on the stored vectors."""
    start = time.time()
    data = stored_groups(groups)
    with tempfile.TemporaryDirectory(prefix="filesgate-fx-") as tmp:
        fx = Path(tmp)
        problems = build_fixtures(fx)
        if problems:
            raise CouldNotRun("filesgate: the rebuilt fixtures differ from reference/files/fixtures.json:\n"
                             + "\n".join(problems))
        jobs_list = [(g, v) for g in groups for v in data[g]["vectors"]]
        with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
            runs = list(pool.map(lambda gv: run_vector(loader, fx / gv[1]["fixture"], gv[1]["calls"], env), jobs_list))
    vectors: dict[str, dict] = {}
    group_status: dict[str, dict] = {}
    for (g, v), run in zip(jobs_list, runs):
        verdict = {"status": "failed", "reason": run["failed"]} if "failed" in run else \
            compare(v["calls"], v["results"], run["lines"])
        vectors["%s/%s" % (g, v["name"])] = verdict
    for g in groups:
        states = [vectors["%s/%s" % (g, v["name"])]["status"] for v in data[g]["vectors"]]
        status = "equal" if all(s == "equal" for s in states) else "failed" if "failed" in states else "differs"
        group_status[g] = {"status": status, "vectors": len(states)}
    overall = [s["status"] for s in group_status.values()]
    return {
        "format": FORMAT,
        "verdict": "equal" if all(s == "equal" for s in overall) else "failed" if "failed" in overall else "differs",
        "suite": "files",
        "kit": {"interfaces": kit_tag("interfaces"), "reference_traces": kit_tag("reference-traces")},
        "seconds": round(time.time() - start, 1),
        "groups": group_status,
        "vectors": vectors,
    }


def kit_tag(prefix: str) -> str | None:
    """The newest `<prefix>-v*` tag reachable from HEAD."""
    try:
        tags = subprocess.run(["git", "-C", str(ROOT), "tag", "--merged", "HEAD", prefix + "-v*"], check=True,
                              capture_output=True, text=True).stdout.split()
    except (OSError, subprocess.CalledProcessError):
        return None
    nums = [(int(t.split("-v")[1]), t) for t in tags if t.split("-v")[1].isdigit()]
    return max(nums)[1] if nums else None


def record(loader: list[str]) -> None:
    """Write reference/files/ from the in-repo adapter; refuse when two runs disagree."""
    VECTORS.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="filesgate-fx-") as tmp:
        fx = Path(tmp)
        digests = filesfixtures.build(fx)
        jobs = [(g, name, fixture, calls) for g in GROUPS for name, fixture, calls in CASES[g]]
        with concurrent.futures.ThreadPoolExecutor(8) as pool:
            runs = list(pool.map(lambda j: [run_vector(loader, fx / j[2], j[3]) for _ in range(2)], jobs))
        vectors: dict[str, list] = {g: [] for g in GROUPS}
        for (g, name, fixture, calls), pair in zip(jobs, runs):
            for run in pair:
                if "failed" in run:
                    raise SystemExit("filesgate: %s/%s failed: %s" % (g, name, run["failed"]))
            if not same(pair[0]["lines"], pair[1]["lines"]):
                raise SystemExit("filesgate: %s/%s differs between two runs" % (g, name))
            vectors[g].append({"name": name, "fixture": fixture, "calls": calls, "results": pair[0]["lines"]})
        for g in GROUPS:
            (VECTORS / (g + ".json")).write_text(dump({"format": FORMAT, "group": g, "vectors": vectors[g]}))
    (VECTORS / "fixtures.json").write_text(json.dumps({"format": FORMAT, "fixtures": digests}, indent=1) + "\n")
    print("filesgate: wrote %s" % ", ".join(GROUPS))


def dump(body: dict) -> str:
    """A vector file: one vector per block, one call and one result per line, so a diff names the line."""
    lines = ['{"format": %d, "group": %s, "vectors": [' % (body["format"], json.dumps(body["group"]))]
    for i, v in enumerate(body["vectors"]):
        lines.append(' {"name": %s, "fixture": %s,' % (json.dumps(v["name"]), json.dumps(v["fixture"])))
        lines.append('  "calls": [')
        lines.append(",\n".join("   " + json.dumps(c) for c in v["calls"]))
        lines.append('  ],')
        lines.append('  "results": [')
        lines.append(",\n".join("   " + json.dumps(r) for r in v["results"]))
        lines.append("  ]}" + ("," if i + 1 < len(body["vectors"]) else ""))
    lines.append("]}")
    return "\n".join(lines) + "\n"


def retail(game: Path, loader: list[str], reference: list[str]) -> int:
    """Compare two loaders on a game directory; print each call where they part. Nothing is written."""
    first = [["mount"]] + RETAIL_CALLS
    ref = run_vector(reference, game, first)
    if "failed" in ref:
        raise SystemExit("filesgate: the reference loader failed: %s" % ref["failed"])
    calls = list(first)
    names = set()
    for (op, value), call in zip(ref["lines"], first):
        if op == "tree":
            names.update(value)
    for (op, value), call in zip(ref["lines"], first):
        if op == "find" and call[1] == "*":
            for entry in value:
                if entry[1] & 0x10 and entry[0] not in (".", ".."):
                    calls.append(["find", entry[0] + "\\*", "-1", "1"])
                    calls.append(["count", entry[0] + "\\*", "0"])
    for name in sorted(names):
        calls += [["lengthbyname", name], ["load", name.upper()], ["load", name.lower()]]
    want = run_vector(reference, game, calls)
    got = run_vector(loader, game, calls)
    for label, run in (("reference", want), ("loader", got)):
        if "failed" in run:
            raise SystemExit("filesgate: the %s failed on the retail calls: %s" % (label, run["failed"]))
    parted = 0
    for i, call in enumerate(calls):
        a = want["lines"][i] if i < len(want["lines"]) else None
        b = got["lines"][i] if i < len(got["lines"]) else None
        if not same(a, b):
            parted += 1
            if parted <= 20:
                print("differs: line %d: %s" % (i + 1, "\t".join(call)))
    print("filesgate: retail: %d calls, %d differ" % (len(calls), parted))
    return 1 if parted else 0


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    ch = sub.add_parser("check", help="judge a loader on the stored vectors")
    ch.add_argument("--loader", default=DEFAULT_LOADER)
    ch.add_argument("--out", type=Path, default=ROOT / "build" / "files-gate")
    ch.add_argument("--group", action="append", choices=GROUPS)
    rec = sub.add_parser("record", help="remake reference/files/ with the in-repo adapter")
    rec.add_argument("--loader", default=DEFAULT_LOADER)
    rt = sub.add_parser("retail", help="compare a loader with the in-repo adapter on a game directory")
    rt.add_argument("--game-dir", type=Path, required=True)
    rt.add_argument("--loader", required=True)
    rt.add_argument("--reference", default=DEFAULT_LOADER)
    args = parser.parse_args()
    os.environ.setdefault("WINEDEBUG", "-all")
    if args.command == "record":
        record(loader_command(args.loader))
    elif args.command == "retail":
        sys.exit(retail(args.game_dir.expanduser().resolve(), loader_command(args.loader),
                        loader_command(args.reference)))
    else:
        try:
            result = judge(loader_command(args.loader), tuple(args.group or GROUPS))
        except CouldNotRun as e:
            print(e, file=sys.stderr)
            sys.exit(2)
        args.out.mkdir(parents=True, exist_ok=True)
        (args.out / "result.json").write_text(json.dumps(result, indent=1) + "\n")
        for g, s in result["groups"].items():
            print("%-10s %s" % (g, s["status"]))
        for name, v in result["vectors"].items():
            if v["status"] != "equal":
                print("  %s: %s %s" % (name, v["status"], v.get("line", v.get("reason", ""))))
        print("filesgate: %s, result in %s" % (result["verdict"], args.out / "result.json"))
        sys.exit(0 if result["verdict"] == "equal" else 1)


if __name__ == "__main__":
    main()
