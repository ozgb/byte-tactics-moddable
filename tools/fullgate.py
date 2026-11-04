"""Count the merges on origin/moddable that needed the gate since the last full gate run.

    uv run tools/fullgate.py                          # print the count and whether the full run is due
    uv run tools/fullgate.py due                      # the same, exit 1 when the count is CADENCE or more
    uv run tools/fullgate.py run                      # play the full suite, the round trip and the coverage check, print the comment text
    uv run tools/fullgate.py --no-fetch               # do not fetch origin/moddable first
    uv run tools/fullgate.py --comments-json FILE     # tracking issue comments from a file (gh's --json comments)
    uv run tools/test_fullgate.py                     # tests, on a temporary repository and fake comments

The pull request gate plays the tier; the full `reference.py --check` runs on the tip of moddable once every
CADENCE merges that needed a gate (issue #631). The lead runs it when `due` exits 1, then comments on tracking
issue #632: `Full gate on <sha>: <n> of <n> equal (both modes), block pointers: <result>, round trip: <n> of <n>
equal, coverage: <result>`. The newest comment of that form names the last full run. The merges counted are the first-parent merge commits `Merge pull request #N`
on origin/moddable after that sha whose diff against their first parent touches GATE_PATHS, the paths
reference/README.md, "Gate", says need a gate (test_fullgate.py checks the two agree).

`run` plays `reference.py --check --both-modes --no-cache` and then, straight after it, `--check --no-cache
--roundtrip auto` (the same exe, a fresh Wine prefix for each), prints their output to stderr and the text of the
comment to stdout (or to --out FILE), and exits with the first nonzero status of the three. Posting stays with the lead.
The round trip run scans block pointers too; the comment names them only when they are not all covered.

After them `run` builds the coverage exe (tools/coverage_runtime.sh, then the BT_COVERAGE=ON build in build/coverage, with its
wasm_runtime target) and plays `tools/coverage.py --check` on it, so that the next time the coverage link breaks or
data/coverage.csv goes stale a periodic run says so (issue #770). The comment's coverage field is `no subsystem fell`, or
`build failed`, or `failed`, or `not finished`.

The comments are read through `bwai-outside gh issue -R ozgb/byte-tactics-moddable view 632 --json comments`,
or through the command in FULLGATE_GH (split like a shell line, the issue arguments appended) or from
--comments-json. With no way to read them, or no `Full gate on` comment, it prints why and exits 2.
"""

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPO = "ozgb/byte-tactics-moddable"
TRACKING_ISSUE = 632
CADENCE = 5
BUILD_FAILED = -1
BRANCH = "origin/moddable"
GATE_PATHS = (
    "src/",
    "include/",
    "interfaces/",
    "lib/",
    "wasm/",
    "reference/",
    "CMakeLists.txt",
    "cmake/",
    "data/layout_refs.csv",
    "data/trace_schema.json",
    "tools/traceschema.py",
    "tools/reference.py",
    "tools/playtest.py",
)
FULL_RUN = re.compile(r"\s*Full gate on ([0-9a-fA-F]{7,40})\b")
MERGE_SUBJECT = re.compile(r"Merge pull request #(\d+)\b")
EQUAL_LINE = re.compile(r"^(\d+) of (\d+) scenarios equal, total ", re.MULTILINE)
POINTERS_OK = re.compile(r"^(?:\[(\w+)\] )?block pointers: all covered by ", re.MULTILINE)
COVERAGE_OK = re.compile(r"^coverage: no subsystem fell$", re.MULTILINE)
POINTERS_BAD = re.compile(r"^(?:\[(\w+)\] )?(\d+) problems with the pointers out of the block", re.MULTILINE)
CHECK_COMMAND = ["uv", "run", "tools/reference.py", "--check", "--no-cache"]
COVERAGE_DIR = "build/coverage"
COVERAGE_BUILD = (
    ["tools/coverage_runtime.sh"],
    ["cmake", "-S", ".", "-B", COVERAGE_DIR, "-G", "Ninja", "-DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-i686.cmake", "-DBT_COVERAGE=ON"],
    ["cmake", "--build", COVERAGE_DIR],
    ["cmake", "--build", COVERAGE_DIR, "--target", "wasm_runtime"],
)


def fail(message: str, code: int = 1):
    print(f"fullgate: {message}", file=sys.stderr)
    sys.exit(code)


def git(*args: str) -> str:
    result = subprocess.run(["git", "-C", str(ROOT), *args], capture_output=True, text=True)
    if result.returncode != 0:
        fail(f"git {' '.join(args)} failed: {result.stderr.strip()}")
    return result.stdout


def needs_gate(path: str) -> bool:
    return any(path.startswith(p) for p in GATE_PATHS)


def last_full_run(comments: list[dict]) -> str | None:
    """The sha of the newest comment that starts with `Full gate on <sha>`, or None."""
    ordered = sorted(enumerate(comments), key=lambda item: (item[1].get("createdAt") or "", item[0]))
    for _, comment in reversed(ordered):
        match = FULL_RUN.match(comment.get("body") or "")
        if match:
            return match.group(1)
    return None


def read_comments(path: str | None) -> list[dict]:
    if path:
        text = Path(path).read_text()
    else:
        command = shlex.split(os.environ.get("FULLGATE_GH", "")) or (
            ["bwai-outside", "gh"] if shutil.which("bwai-outside") else [])
        if not command:
            fail("cannot read the comments of issue #632: no bwai-outside on PATH; "
                 "give --comments-json FILE or set FULLGATE_GH", 2)
        run = subprocess.run([*command, "issue", "-R", REPO, "view", str(TRACKING_ISSUE), "--json", "comments"],
                             capture_output=True, text=True)
        if run.returncode != 0:
            fail(f"reading the comments of issue #{TRACKING_ISSUE} failed: {run.stderr.strip()}", 2)
        text = run.stdout
    try:
        data = json.loads(text)
    except json.JSONDecodeError as error:
        fail(f"the comments are not JSON: {error}", 2)
    return data["comments"] if isinstance(data, dict) else data


def merges_since(sha: str) -> list[tuple[int, str, bool]]:
    """(pull request number, merge sha, needed the gate) for each first-parent merge after sha, oldest first."""
    log = git("log", "--first-parent", "--merges", "--reverse", "--format=%H %s", f"{sha}..{BRANCH}")
    merges = []
    for line in log.splitlines():
        merge, _, subject = line.partition(" ")
        match = MERGE_SUBJECT.match(subject)
        if not match:
            continue
        changed = git("diff", "--name-only", f"{merge}^1", merge).splitlines()
        merges.append((int(match.group(1)), merge, any(needs_gate(p) for p in changed)))
    return merges


def equal_count(output: str) -> str:
    """`<n> of <n>` from the `<n> of <n> scenarios equal` line of a reference.py --check run, or `not finished`."""
    match = EQUAL_LINE.search(output)
    return f"{match.group(1)} of {match.group(2)}" if match else "not finished"


def pointer_result(output: str) -> str:
    """The block pointer field of a reference.py --check run: what it printed for each mode, or `not reported`."""
    ok = [m.group(1) or "" for m in POINTERS_OK.finditer(output)]
    bad = {m.group(1) or "": int(m.group(2)) for m in POINTERS_BAD.finditer(output)}
    modes = list(dict.fromkeys(ok + list(bad)))
    if not modes:
        return "not reported"
    if not bad:
        return "all covered by data/block_pointers.csv" + (" in both modes" if modes != [""] else "")
    parts = []
    for mode in modes:
        what = f"{bad[mode]} block pointer problems" if mode in bad else "all covered by data/block_pointers.csv"
        parts.append(f"[{mode}] {what}" if mode else what)
    return "; ".join(parts)


def coverage_result(code: int, output: str) -> str:
    """The coverage field: `no subsystem fell` for a passing check, else `build failed` (-1), `failed` or `not finished`."""
    if code == 0 and COVERAGE_OK.search(output):
        return "no subsystem fell"
    if code == BUILD_FAILED:
        return "build failed"
    return "failed" if output.strip() else "not finished"


def comment_text(sha: str, check: tuple[int, str], roundtrip: tuple[int, str], coverage: tuple[int, str]) -> str:
    """The comment for issue #632 from the exit status and output of the both-modes run, the round trip run and the coverage check."""
    (code, output), (trip_code, trip_output), (cov_code, cov_output) = check, roundtrip, coverage
    text = (f"Full gate on {sha}: {equal_count(output)} equal (both modes), block pointers: {pointer_result(output)}, "
            f"round trip: {equal_count(trip_output)} equal, coverage: {coverage_result(cov_code, cov_output)}")
    trip_pointers = pointer_result(trip_output)
    if "problems" in trip_pointers:
        text += f" (block pointers: {trip_pointers})"
    for name, status in (("both modes", code), ("round trip", trip_code), ("coverage", cov_code)):
        if status != 0:
            text += f", {name} exit {1 if status == BUILD_FAILED else status}"
    host = next((line for line in output.splitlines() if line.startswith("host: ")), None)
    return f"{text}\n{host}" if host else text


def play(args: list[str], log: Path | None, what: str) -> tuple[int, str]:
    """Run reference.py --check with `args`, copying its output to stderr (and to `log`) as it comes; (exit status, output)."""
    return stream(CHECK_COMMAND + args, log, what)


def stream(command: list[str], log: Path | None, what: str) -> tuple[int, str]:
    """Run `command` in the repository, copying its output to stderr (and to `log`) as it comes; (exit status, output)."""
    print(f"fullgate: {what}: {' '.join(command)}", file=sys.stderr, flush=True)
    process = subprocess.Popen(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    lines = []
    for line in process.stdout:
        sys.stderr.write(line)
        lines.append(line)
    process.wait()
    output = "".join(lines)
    if log is not None:
        with log.open("a") as handle:
            handle.write(f"== {what}\n{output}== exit {process.returncode}\n")
    return process.returncode, output


def coverage_check(game_dir: str | None, log: Path | None) -> tuple[int, str]:
    """Build the coverage exe and play `coverage.py --check` on it; (exit status, output). A failed build is BUILD_FAILED."""
    for step in COVERAGE_BUILD:
        code, output = stream(step, log, "coverage build")
        if code != 0:
            return BUILD_FAILED, output
    shutil.rmtree(ROOT / "build/coverage-run", ignore_errors=True)
    command = ["uv", "run", "tools/coverage.py", "--exe", f"{COVERAGE_DIR}/TotalA.exe", "--check"]
    return stream(command + (["--game-dir", game_dir] if game_dir else []), log, "coverage")


def run(args: argparse.Namespace) -> int:
    """Play the both-modes check, the round trip check and the coverage check, and print the comment for issue #632."""
    sha = args.sha or git("rev-parse", "HEAD").strip()
    exe = ["--exe", args.exe] + (["--game-dir", args.game_dir] if args.game_dir else [])
    log = Path(args.log) if args.log else None
    results = []
    for what, extra in (("both modes", ["--both-modes"]), ("round trip", ["--roundtrip", "auto"])):
        shutil.rmtree(ROOT / "build/playtest/prefix", ignore_errors=True)
        started = time.time()
        results.append(play(exe + extra, log, what))
        seconds = int(time.time() - started)
        print(f"fullgate: {what} took {seconds // 60}:{seconds % 60:02d}", file=sys.stderr, flush=True)
    shutil.rmtree(ROOT / "build/playtest/prefix", ignore_errors=True)
    started = time.time()
    results.append(coverage_check(args.game_dir, log))
    seconds = int(time.time() - started)
    print(f"fullgate: coverage took {seconds // 60}:{seconds % 60:02d}", file=sys.stderr, flush=True)
    text = comment_text(sha, *results)
    if args.out:
        Path(args.out).write_text(text + "\n")
    print(text)
    return next((1 if code == BUILD_FAILED else code for code, _ in results if code != 0), 0)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("command", nargs="?", choices=["due", "run"],
                        help="due: exit 1 when the full run is due; run: play the full suite and the round trip")
    parser.add_argument("--no-fetch", action="store_true", help=f"do not fetch {BRANCH} first")
    parser.add_argument("--comments-json", metavar="FILE", help="issue comments as JSON, instead of gh")
    parser.add_argument("--exe", default="build/modern/TotalA.exe", help="run: the exe to play")
    parser.add_argument("--game-dir", default=os.path.expanduser("~/.ta"), help="run: the game directory")
    parser.add_argument("--sha", help="run: the commit the comment names, default HEAD")
    parser.add_argument("--out", metavar="FILE", help="run: also write the comment text to FILE")
    parser.add_argument("--log", metavar="FILE", help="run: append both runs' output to FILE")
    args = parser.parse_args()
    if args.command == "run":
        return run(args)

    sha = last_full_run(read_comments(args.comments_json))
    if sha is None:
        fail(f"no full run is recorded: no comment on issue #{TRACKING_ISSUE} starts with `Full gate on <sha>`", 2)
    if not args.no_fetch:
        git("fetch", "origin", "moddable")
    if subprocess.run(["git", "-C", str(ROOT), "cat-file", "-e", f"{sha}^{{commit}}"],
                      capture_output=True).returncode != 0:
        fail(f"the full run's sha {sha} is not a commit of this repository", 2)

    merges = merges_since(sha)
    needed = [number for number, _, gate in merges if gate]
    names = ", ".join(f"#{n}" for n in needed) or "none"
    print(f"fullgate: {len(needed)} of {CADENCE} merges since the full gate on {sha[:7]} needed the gate: {names}")
    due = len(needed) >= CADENCE
    print("fullgate: the full gate is due" if due else "fullgate: the full gate is not due")
    return 1 if due and args.command == "due" else 0


if __name__ == "__main__":
    sys.exit(main())
