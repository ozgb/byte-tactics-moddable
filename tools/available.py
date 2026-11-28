"""List the issues a lead may claim, in the order it takes them (AGENTS.md step 2).

    uv run tools/available.py                  # Priority now in #2's order, then every other open issue, oldest first
    uv run tools/available.py --lane 1038,1043 # only sub-issues of these parents, at any depth (default $BT_LEAD_LANE)
    uv run tools/available.py --check N        # one issue's state; exit 0 when it is available
    uv run tools/available.py --holder N       # the lead whose claim on N stands first; exit 0 when it is this lead
    uv run tools/available.py --claims         # this lead's claims, the ones it counts toward its 3
    uv run tools/available.py --prs            # this lead's open pull requests
    uv run tools/available.py --lead NAME      # this lead's name (default $BT_LEAD_NAME, else "lead")
    uv run tools/test_available.py             # tests, on a canned #2 body and canned issue states

An issue is available (#2, "What is claimable") when it is open, has no assignee, no standing claim and no open
blockers, is not labelled parked, needs-grooming or needs-human, is not a parent with open sub-issues and is not a
tracker (#2, #180, #392, #632, or the list in that paragraph). Every issue Priority now names is printed in its
order with its state, the reasons it is not available, and its title; then every other open issue that is not
parked, oldest first. Parked issues are counted, not listed. A lead takes the first one marked available.

Priority now is the part of #2's body from its `**Priority now**` line to the next bold heading (other than
`**What is claimable**`), an `Open to claim` or `Every other open issue` line, or a `## ` heading. Each other line
names the issue of its first `#N`; a line whose item starts struck through (`~~#N~~`) is done and skipped.

A claim is a comment whose first line starts `Lead: Claimed`, a release one that starts `Lead: Released`. Each
names its lead as `, lead <name>.`; one without a name is the default lead's, `lead`. A claim stands until a
release by the same lead, and the earliest standing claim holds the issue.

A lead's pull requests are its open ones whose body has a line `Lead: <name>` with its name, or no such line when
its name is `lead`.

In the bwai sandbox (`gh-safe` on PATH) issues are read with `bwai-outside gh issue view` and links with
`gh-safe blocked-by` and `gh-safe sub-issues`; elsewhere with `gh issue view` and `gh api`.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field

REPO = "ozgb/byte-tactics-moddable"
PLAN = 2
TRACKERS = (2, 180, 392, 632)
UNAVAILABLE_LABELS = ("parked", "needs-grooming", "needs-human")
DEFAULT_LEAD = "lead"
NAME = re.compile(r"^[A-Za-z0-9_-]+$")
LEAD_IN = re.compile(r", lead ([A-Za-z0-9_-]+)")
PR_LEAD = re.compile(r"^Lead: ([A-Za-z0-9_-]+)\s*$", re.M)
ISSUE_REF = re.compile(r"#(\d+)\b")
ITEM = re.compile(r"^\s*(?:\d+[a-z]?\.\s+)?(.*)$")
VIEW_FIELDS = "number,title,state,labels,assignees,comments"


class GhError(RuntimeError):
    pass


def run(argv: list[str]) -> str:
    try:
        done = subprocess.run(argv, capture_output=True, text=True)
    except FileNotFoundError as e:
        raise GhError(f"{argv[0]}: not found") from e
    if done.returncode != 0:
        raise GhError(f"{' '.join(argv)}: exit {done.returncode}: {done.stderr.strip()}")
    return done.stdout


class Gh:
    """Reads issues, links and pull requests from GitHub."""

    def __init__(self, repo: str = REPO, runner=run, which=shutil.which):
        self.repo = repo
        self.runner = runner
        self.sandbox = which("gh-safe") is not None
        self.gh = ["bwai-outside", "gh"] if self.sandbox and which("bwai-outside") else ["gh"]

    def issue(self, n: int) -> dict:
        return json.loads(self.runner([*self.gh, "issue", "-R", self.repo, "view", str(n), "--json", VIEW_FIELDS]))

    def body(self, n: int) -> str:
        return json.loads(self.runner([*self.gh, "issue", "-R", self.repo, "view", str(n), "--json", "body"]))["body"]

    def open_issues(self) -> list[dict]:
        return json.loads(self.runner([*self.gh, "issue", "-R", self.repo, "list", "--state", "open", "--limit",
                                       "1000", "--json", "number,title,labels,assignees"]))

    def open_prs(self) -> list[dict]:
        return json.loads(self.runner([*self.gh, "pr", "-R", self.repo, "list", "--author", "@me", "--state",
                                       "open", "--json", "number,title,body,headRefName"]))

    def assigned_issues(self) -> list[dict]:
        return json.loads(self.runner([*self.gh, "issue", "-R", self.repo, "list", "--state", "open", "--assignee",
                                       "@me", "--limit", "1000", "--json", VIEW_FIELDS]))

    def _links(self, n: int, safe_verb: str, endpoint: str) -> list[tuple[int, str]]:
        if self.sandbox:
            out = self.runner(["gh-safe", "-R", self.repo, safe_verb, str(n)])
            links = []
            for line in out.splitlines():
                m = re.match(r"^#(\d+)\s+(\S+)", line.strip())
                if m:
                    links.append((int(m.group(1)), m.group(2).lower()))
            return links
        out = self.runner(["gh", "api", "--paginate", f"repos/{self.repo}/issues/{n}/{endpoint}",
                           "--jq", '.[] | "\\(.number) \\(.state)"'])
        return [(int(a), b.lower()) for a, b in (line.split() for line in out.splitlines() if line.strip())]

    def sub_issues(self, n: int) -> list[tuple[int, str]]:
        return self._links(n, "sub-issues", "sub_issues")

    def blocked_by(self, n: int) -> list[tuple[int, str]]:
        return self._links(n, "blocked-by", "dependencies/blocked_by")


def priority_now(body: str) -> list[int]:
    """The issues #2's Priority now names, in its order, without the done items."""
    found: list[int] = []
    inside = False
    for raw in body.splitlines():
        line = raw.strip()
        if not inside:
            inside = line.startswith("**Priority now**")
            continue
        if line.startswith("## ") or line.startswith("Open to claim") or line.startswith("Every other open issue"):
            break
        if line.startswith("**"):
            if line.startswith("**What is claimable**"):
                continue
            break
        rest = ITEM.match(line).group(1)
        if not rest or rest.startswith("~~"):
            continue
        m = ISSUE_REF.search(rest)
        if m and int(m.group(1)) not in found:
            found.append(int(m.group(1)))
    return found


def trackers(body: str) -> set[int]:
    m = re.search(r"tracker \(([^)]*)\)", body)
    refs = {int(x) for x in ISSUE_REF.findall(m.group(1))} if m else set()
    return refs or set(TRACKERS)


def lead_of(first_line: str) -> str:
    m = LEAD_IN.search(first_line)
    return m.group(1) if m else DEFAULT_LEAD


def standing_claims(comments: list[dict]) -> list[str]:
    """The leads whose claims stand, earliest claim first."""
    held: dict[str, str] = {}
    for c in sorted(comments, key=lambda c: c.get("createdAt", "")):
        first = (c.get("body") or "").lstrip().split("\n", 1)[0]
        if first.startswith("Lead: Claimed"):
            held.setdefault(lead_of(first), c.get("createdAt", ""))
        elif first.startswith("Lead: Released"):
            held.pop(lead_of(first), None)
    return sorted(held, key=lambda name: held[name])


@dataclass
class State:
    number: int
    title: str = ""
    reasons: list[str] = field(default_factory=list)
    holders: list[str] = field(default_factory=list)

    @property
    def available(self) -> bool:
        return not self.reasons

    def line(self) -> str:
        what = "available" if self.available else "; ".join(self.reasons)
        return f"#{self.number:<5} {what}  {self.title}".rstrip()


class Queue:
    """Works out each issue's state, reading every issue and link at most once."""

    def __init__(self, source, body: str):
        self.source = source
        self.body = body
        self.trackers = trackers(body)
        self._issues: dict[int, dict] = {}
        self._subs: dict[int, list[tuple[int, str]]] = {}
        self._blockers: dict[int, list[tuple[int, str]]] = {}

    def prefetch(self, numbers) -> None:
        todo = [n for n in numbers if n not in self._issues]
        with ThreadPoolExecutor(8) as pool:
            for n, data in zip(todo, pool.map(self.source.issue, todo)):
                self._issues[n] = data
        live = [n for n in numbers if self._issues[n].get("state", "").upper() == "OPEN"]
        for cache, read in ((self._subs, self.source.sub_issues), (self._blockers, self.source.blocked_by)):
            todo = [n for n in live if n not in cache]
            with ThreadPoolExecutor(8) as pool:
                cache.update(zip(todo, pool.map(read, todo)))

    def issue(self, n: int) -> dict:
        if n not in self._issues:
            self._issues[n] = self.source.issue(n)
        return self._issues[n]

    def subs(self, n: int) -> list[tuple[int, str]]:
        if n not in self._subs:
            self._subs[n] = self.source.sub_issues(n)
        return self._subs[n]

    def blockers(self, n: int) -> list[tuple[int, str]]:
        if n not in self._blockers:
            self._blockers[n] = self.source.blocked_by(n)
        return self._blockers[n]

    def state(self, n: int) -> State:
        data = self.issue(n)
        st = State(n, data.get("title", ""))
        if data.get("state", "").upper() != "OPEN":
            st.reasons.append("closed")
            return st
        labels = [lab["name"] for lab in data.get("labels", [])]
        st.reasons += [f"`{lab}`" for lab in UNAVAILABLE_LABELS if lab in labels]
        if n in self.trackers:
            st.reasons.append("tracker")
        open_subs = [s for s, state in self.subs(n) if state == "open"]
        if open_subs:
            st.reasons.append("parent of open " + ", ".join(f"#{s}" for s in open_subs))
        st.holders = standing_claims(data.get("comments", []))
        if st.holders:
            st.reasons.append("claimed (" + ", ".join(f"lead {h}" for h in st.holders) + ")")
        elif data.get("assignees"):
            st.reasons.append("assigned, no claim comment")
        open_blockers = [b for b, state in self.blockers(n) if state == "open"]
        if open_blockers:
            st.reasons.append("blocked by " + ", ".join(f"#{b}" for b in open_blockers))
        return st

    def descendants(self, parents) -> set[int]:
        seen: set[int] = set()
        todo = list(parents)
        while todo:
            n = todo.pop()
            for s, _ in self.subs(n):
                if s not in seen:
                    seen.add(s)
                    todo.append(s)
        return seen


def parse_lane(text: str) -> list[int]:
    lane = []
    for part in text.replace(" ", "").split(","):
        if part:
            if not re.fullmatch(r"#?\d+", part):
                raise SystemExit(f"available.py: bad lane entry {part!r}: give issue numbers, comma-separated")
            lane.append(int(part.lstrip("#")))
    return lane


def listing(queue: Queue, open_issues: list[dict], lane: list[int]) -> list[str]:
    order = priority_now(queue.body)
    in_lane = queue.descendants(lane) if lane else None
    if in_lane is not None:
        order = [n for n in order if n in in_lane]
    rest = sorted(i["number"] for i in open_issues
                  if i["number"] not in order and (in_lane is None or i["number"] in in_lane))
    parked = [n for n in rest
              if "parked" in [lab["name"] for lab in next(i for i in open_issues if i["number"] == n)["labels"]]]
    rest = [n for n in rest if n not in parked]
    queue.prefetch(order + rest)
    out = ["Priority now, in its order:"]
    out += ["  " + queue.state(n).line() for n in order] or ["  (none)"]
    out.append("Every other open issue, oldest first:")
    out += ["  " + queue.state(n).line() for n in rest] or ["  (none)"]
    out.append(f"Parked, not listed: {len(parked)}")
    first = next((n for n in order + rest if queue.state(n).available), None)
    out.append(f"First available: #{first}" if first else "First available: none")
    return out


def own_prs(prs: list[dict], lead: str) -> list[dict]:
    mine = []
    for pr in prs:
        names = PR_LEAD.findall(pr.get("body") or "")
        if lead in names or (not names and lead == DEFAULT_LEAD):
            mine.append(pr)
    return mine


def own_claims(issues: list[dict], lead: str) -> list[dict]:
    return [i for i in issues if lead in standing_claims(i.get("comments", []))]


def main(argv=None, source=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n", 1)[0])
    ap.add_argument("--lead", default=os.environ.get("BT_LEAD_NAME") or DEFAULT_LEAD)
    ap.add_argument("--lane", default=os.environ.get("BT_LEAD_LANE", ""))
    ap.add_argument("--repo", default=REPO)
    what = ap.add_mutually_exclusive_group()
    what.add_argument("--check", type=int, metavar="N")
    what.add_argument("--holder", type=int, metavar="N")
    what.add_argument("--claims", action="store_true")
    what.add_argument("--prs", action="store_true")
    args = ap.parse_args(argv)
    if not NAME.match(args.lead) or args.lead == "maintainer":
        raise SystemExit(f"available.py: bad lead name {args.lead!r}: letters, digits, - and _ only, not maintainer")
    lane = parse_lane(args.lane)
    source = source or Gh(args.repo)

    try:
        if args.prs:
            mine = own_prs(source.open_prs(), args.lead)
            for pr in mine:
                print(f"#{pr['number']:<5} {pr['title']}")
            print(f"Lead {args.lead}: {len(mine)} open pull requests")
            return 0
        if args.claims:
            mine = own_claims(source.assigned_issues(), args.lead)
            for i in mine:
                print(f"#{i['number']:<5} {i['title']}")
            print(f"Lead {args.lead}: {len(mine)} claims")
            return 0
        queue = Queue(source, source.body(PLAN))
        if args.check is not None:
            st = queue.state(args.check)
            print(st.line())
            return 0 if st.available else 1
        if args.holder is not None:
            holders = standing_claims(queue.issue(args.holder).get("comments", []))
            print(f"#{args.holder} held by lead {holders[0]}" if holders else f"#{args.holder} has no standing claim")
            return 0 if holders and holders[0] == args.lead else 1
        print(f"Lead {args.lead}, lane " + (", ".join(f"#{n}" for n in lane) if lane else "all of #2"))
        for line in listing(queue, source.open_issues(), lane):
            print(line)
        return 0
    except GhError as e:
        print(f"available.py: {e}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
