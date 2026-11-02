"""Count the merges on origin/moddable that needed the gate since the last full-suite run, and say when the next one is due.

    uv run tools/fullgate.py                    # print the count
    uv run tools/fullgate.py due                # the same, and exit 1 when the count is DUE_AFTER or more
    uv run tools/fullgate.py --comments-file F  # read the tracking issue's comments from F (gh's `--json comments` output)

The last full run is the newest comment `Full gate on <sha>: ...` of the tracking issue (#632), which the lead posts after
a full `tools/reference.py --check` on the tip of moddable. The comments are read with
`bwai-outside gh issue -R ozgb/byte-tactics-moddable view 632 --json comments`; --comments-command replaces that command
(split like a shell line) and --comments-file reads a file instead, so the tests need no network.

A merge needed the gate when it changed a path of GATE_PATHS (reference/README.md, "Gate"). Each first-parent commit of the
ref after the last full run counts once: a merged pull request is one merge commit, a squash is one commit.
"""

import argparse
import json
import re
import shlex
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TRACKING_ISSUE = 632
REPO = "ozgb/byte-tactics-moddable"
DUE_AFTER = 5
GATE_PATHS = ("src/", "include/", "interfaces/", "lib/", "link/", "cmake/", "reference/", "CMakeLists.txt",
              "data/layout_refs.csv", "data/trace_schema.json", "tools/reference.py", "tools/playtest.py",
              "tools/tracediff.py", "tools/tracedigest.py", "tools/traceschema.py")
FULL_GATE = re.compile(r"^\s*Full gate on ([0-9a-f]{7,40})\b", re.MULTILINE)


def default_command(issue: int = TRACKING_ISSUE) -> list[str]:
    """The command that prints the tracking issue's comments as JSON."""
    return ["bwai-outside", "gh", "issue", "-R", REPO, "view", str(issue), "--json", "comments"]


def needs_gate(paths: list[str]) -> bool:
    """Whether any changed path is one the gate applies to."""
    return any(p == g or (g.endswith("/") and p.startswith(g)) for p in paths for g in GATE_PATHS)


def last_full_run(comments: list[dict]) -> str | None:
    """The sha of the newest `Full gate on <sha>` comment (by createdAt when given, else by position), or None."""
    found = [(c.get("createdAt", ""), i, m.group(1)) for i, c in enumerate(comments)
             for m in [FULL_GATE.search(c.get("body", ""))] if m]
    return max(found)[2] if found else None


def parse_comments(text: str) -> list[dict]:
    """The comments of gh's `issue view --json comments` output."""
    return json.loads(text).get("comments", [])


def read_comments(command: list[str]) -> list[dict]:
    """Run the command and parse its output; exits 2 when it fails."""
    proc = subprocess.run(command, capture_output=True, text=True)
    if proc.returncode != 0:
        sys.exit(f"fullgate: {' '.join(command)} failed: {proc.stderr.strip()}")
    return parse_comments(proc.stdout)


def git(*args: str) -> str:
    """The output of a git command in the repository; exits 2 when it fails."""
    proc = subprocess.run(["git", "-C", str(ROOT), *args], capture_output=True, text=True)
    if proc.returncode != 0:
        sys.exit(f"fullgate: git {' '.join(args)} failed: {proc.stderr.strip()}")
    return proc.stdout


def merges_since(ref: str, sha: str | None) -> list[tuple[str, list[str]]]:
    """(commit, changed paths) for each first-parent commit of ref after sha (all of them when sha is None), newest first."""
    span = f"{sha}..{ref}" if sha else ref
    out = []
    for commit in git("rev-list", "--first-parent", span).split():
        out.append((commit, git("diff-tree", "-m", "--first-parent", "--root", "--no-commit-id", "--name-only", "-r", commit).split()))
    return out


def count_needing_gate(merges: list[tuple[str, list[str]]]) -> int:
    """How many of the merges changed a path the gate applies to."""
    return sum(needs_gate(paths) for _, paths in merges)


def report(count: int, sha: str | None, issue: int) -> str:
    """The line fullgate prints."""
    since = f"the full gate on {sha[:12]}" if sha else f"the start (no `Full gate on <sha>` comment on #{issue} yet)"
    return f"{count} merged pull requests needed the gate since {since}; a full run is due at {DUE_AFTER}"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("action", nargs="?", choices=("due",), help="exit 1 when the count is %d or more" % DUE_AFTER)
    parser.add_argument("--ref", default="origin/moddable", help="the branch whose merges are counted")
    parser.add_argument("--issue", type=int, default=TRACKING_ISSUE, help="the tracking issue")
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--comments-command", help="the command that prints the issue's comments as JSON")
    source.add_argument("--comments-file", type=Path, help="a file with that JSON")
    args = parser.parse_args(argv)
    if args.comments_file:
        comments = parse_comments(args.comments_file.read_text())
    else:
        comments = read_comments(shlex.split(args.comments_command) if args.comments_command else default_command(args.issue))
    sha = last_full_run(comments)
    count = count_needing_gate(merges_since(args.ref, sha))
    print(report(count, sha, args.issue))
    return 1 if args.action == "due" and count >= DUE_AFTER else 0


if __name__ == "__main__":
    sys.exit(main())
