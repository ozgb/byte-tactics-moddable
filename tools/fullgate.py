"""Count the merges on origin/moddable that needed the gate since the last full gate run.

    uv run tools/fullgate.py                          # print the count and whether the full run is due
    uv run tools/fullgate.py due                      # the same, exit 1 when the count is CADENCE or more
    uv run tools/fullgate.py --no-fetch               # do not fetch origin/moddable first
    uv run tools/fullgate.py --comments-json FILE     # tracking issue comments from a file (gh's --json comments)
    uv run tools/test_fullgate.py                     # tests, on a temporary repository and fake comments

The pull request gate plays the tier; the full `reference.py --check` runs on the tip of moddable once every
CADENCE merges that needed a gate (issue #631). The lead runs it when `due` exits 1, then comments on tracking
issue #632: `Full gate on <sha>: <n> of <n> equal, block pointers: <result>`. The newest comment of that
form names the last full run. The merges counted are the first-parent merge commits `Merge pull request #N`
on origin/moddable after that sha whose diff against their first parent touches GATE_PATHS, the paths
reference/README.md, "Gate", says need a gate (test_fullgate.py checks the two agree).

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
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPO = "ozgb/byte-tactics-moddable"
TRACKING_ISSUE = 632
CADENCE = 5
BRANCH = "origin/moddable"
GATE_PATHS = (
    "src/",
    "include/",
    "interfaces/",
    "lib/bitexact/",
    "reference/",
    "data/layout_refs.csv",
    "data/trace_schema.json",
    "tools/traceschema.py",
    "tools/reference.py",
    "tools/playtest.py",
)
FULL_RUN = re.compile(r"\s*Full gate on ([0-9a-fA-F]{7,40})\b")
MERGE_SUBJECT = re.compile(r"Merge pull request #(\d+)\b")


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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("command", nargs="?", choices=["due"], help="exit 1 when the full run is due")
    parser.add_argument("--no-fetch", action="store_true", help=f"do not fetch {BRANCH} first")
    parser.add_argument("--comments-json", metavar="FILE", help="issue comments as JSON, instead of gh")
    args = parser.parse_args()

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
