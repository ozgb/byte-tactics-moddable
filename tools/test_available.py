"""Tests for available.py, on a canned #2 body and canned issue states, with no network.

    uv run tools/test_available.py
"""

import contextlib
import io
import json
import unittest
import unittest.mock
from pathlib import Path

import available

BODY = (Path(__file__).resolve().parent / "testdata" / "available" / "issue2.md").read_text()


def claim(lead: str | None, when: str) -> dict:
    named = f", lead {lead}" if lead else ""
    return {"body": f"Lead: Claimed by Claude Code / claude-opus-5-5{named}. Branch issue-1.", "createdAt": when}


def release(lead: str | None, when: str) -> dict:
    named = f", lead {lead}." if lead else ""
    return {"body": f"Lead: Released{named}", "createdAt": when}


def issue(n, state="OPEN", labels=(), assignees=(), comments=(), title=None) -> dict:
    return {"number": n, "title": title or f"Issue {n}", "state": state,
            "labels": [{"name": lab} for lab in labels],
            "assignees": [{"login": a} for a in assignees], "comments": list(comments)}


class Fake:
    """Canned GitHub: the issues, their sub-issues and their blockers."""

    def __init__(self, issues, subs=None, blockers=None, prs=(), body=BODY):
        self.issues = {i["number"]: i for i in issues}
        self.subs = subs or {}
        self.blockers = blockers or {}
        self.prs = list(prs)
        self.plan = body
        self.reads = []

    def state_of(self, n):
        return self.issues[n]["state"].lower() if n in self.issues else "open"

    def body(self, n):
        return self.plan

    def issue(self, n):
        self.reads.append(n)
        return self.issues[n]

    def sub_issues(self, n):
        return [(s, self.state_of(s)) for s in self.subs.get(n, [])]

    def blocked_by(self, n):
        return [(b, self.state_of(b)) for b in self.blockers.get(n, [])]

    def open_issues(self):
        return [{k: i[k] for k in ("number", "title", "labels", "assignees")}
                for i in self.issues.values() if i["state"] == "OPEN"]

    def assigned_issues(self):
        return [i for i in self.issues.values() if i["state"] == "OPEN" and i["assignees"]]

    def open_prs(self):
        return self.prs


def today() -> Fake:
    """#2 as it stood on 2026-11-24, with #1036, #1037, #1039 and #1056 claimed by the lead named lead."""
    held = ("ozgb",)
    issues = [
        issue(2, labels=()), issue(180), issue(392), issue(632),
        issue(933, state="CLOSED"),
        issue(1056, assignees=held, comments=[claim("lead", "2026-11-24T22:30:00Z")]),
        issue(1036, assignees=held, comments=[claim(None, "2026-11-24T10:00:00Z")]),
        issue(1037, assignees=held, comments=[claim(None, "2026-11-24T11:00:00Z")]),
        issue(1038), issue(1039, assignees=held, comments=[claim(None, "2026-11-24T11:00:00Z")]),
        issue(1040), issue(1041), issue(1042), issue(1043), issue(1044), issue(1045), issue(897), issue(1046),
        issue(1047), issue(1048), issue(1052, labels=("needs-grooming",)), issue(1060, labels=("needs-human",)),
        issue(700, labels=("parked",)), issue(701, labels=("parked",)),
        issue(1061, assignees=held),
        issue(1062, comments=[claim("north", "2026-11-24T09:00:00Z")]),
    ]
    subs = {2: [1038, 1043, 1047, 1056, 632], 180: [1046], 1038: [1036, 1037, 1039, 1040, 1041, 1042],
            1043: [1044, 1045, 897], 1047: [1048, 1052]}
    blockers = {1041: [1036, 1037, 1039, 1040], 1042: [1041, 933], 897: [1045], 1048: [1040], 1043: [1044]}
    return Fake(issues, subs, blockers)


def run(fake, *argv) -> tuple[int, str]:
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        code = available.main(["--lead", "lead", "--lane", "", *argv], source=fake)
    return code, out.getvalue()


def entries(text: str) -> list[tuple[int, str]]:
    found = []
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("#"):
            n, rest = line[1:].split(None, 1)
            found.append((int(n), rest))
    return found


class PriorityNowTest(unittest.TestCase):
    def test_order_skips_done_and_status_lines(self):
        self.assertEqual(available.priority_now(BODY),
                         [933, 1056, 1036, 1038, 1037, 1039, 1040, 1041, 1042, 1043, 1044, 1045, 897, 1046])

    def test_trackers_from_the_claimable_paragraph(self):
        self.assertEqual(available.trackers(BODY), {2, 180, 392, 632})
        self.assertEqual(available.trackers("no such paragraph"), set(available.TRACKERS))

    def test_no_priority_now(self):
        self.assertEqual(available.priority_now("## Order\n\n1. #5\n"), [])


class ClaimTest(unittest.TestCase):
    def test_old_claims_are_the_default_lead(self):
        self.assertEqual(available.standing_claims([claim(None, "1")]), ["lead"])

    def test_release_by_the_same_lead_only(self):
        comments = [claim("north", "1"), claim("south", "2"), release("south", "3")]
        self.assertEqual(available.standing_claims(comments), ["north"])
        self.assertEqual(available.standing_claims(comments + [release(None, "4")]), ["north"])
        self.assertEqual(available.standing_claims(comments + [release("north", "4")]), [])

    def test_earliest_standing_claim_first(self):
        comments = [claim("south", "2026-11-24T10:00:02Z"), claim("north", "2026-11-24T10:00:01Z")]
        self.assertEqual(available.standing_claims(comments), ["north", "south"])

    def test_reclaim_after_release(self):
        comments = [claim("north", "1"), release("north", "2"), claim("north", "3")]
        self.assertEqual(available.standing_claims(comments), ["north"])

    def test_other_comments_ignored(self):
        comments = [{"body": "Maintainer: the lead takes this now, lead x.", "createdAt": "1"},
                    {"body": "Lead: took it, lead north.", "createdAt": "2"}]
        self.assertEqual(available.standing_claims(comments), [])


class ListingTest(unittest.TestCase):
    def test_done_when(self):
        code, out = run(today())
        self.assertEqual(code, 0)
        got = entries(out)
        order = [n for n, _ in got]
        self.assertLess(order.index(1036), order.index(1040))
        self.assertLess(order.index(1040), order.index(1044))
        states = dict(got)
        self.assertTrue(states[1038].startswith("parent of open #1036"))
        self.assertTrue(states[1043].startswith("parent of open #1044"))
        for parent in (1038, 1043, 1047, 2, 180):
            self.assertFalse(states[parent].startswith("available"), parent)
        self.assertIn("First available: #1040", out)

    def test_reasons(self):
        states = dict(entries(run(today())[1]))
        self.assertTrue(states[933].startswith("closed"))
        self.assertTrue(states[1036].startswith("claimed (lead lead)"))
        self.assertTrue(states[1041].startswith("blocked by #1036, #1037, #1039, #1040"))
        self.assertTrue(states[1042].startswith("blocked by #1041 "))
        self.assertTrue(states[1052].startswith("`needs-grooming`"))
        self.assertTrue(states[1060].startswith("`needs-human`"))
        self.assertTrue(states[392].startswith("tracker"))
        self.assertTrue(states[1061].startswith("assigned, no claim comment"))
        self.assertTrue(states[1062].startswith("claimed (lead north)"))
        self.assertTrue(states[1044].startswith("available"))
        self.assertTrue(states[1046].startswith("available"))

    def test_rest_oldest_first_after_priority_now(self):
        out = run(today())[1]
        head, rest = out.split("Every other open issue, oldest first:")
        self.assertEqual([n for n, _ in entries(head)], available.priority_now(BODY))
        rest_numbers = [n for n, _ in entries(rest)]
        self.assertEqual(rest_numbers, sorted(rest_numbers))
        self.assertIn(1047, rest_numbers)

    def test_parked_counted_not_listed(self):
        out = run(today())[1]
        self.assertNotIn(700, [n for n, _ in entries(out)])
        self.assertIn("Parked, not listed: 2", out)

    def test_lane(self):
        out = run(today(), "--lane", "1043")[1]
        self.assertIn("lane #1043", out)
        self.assertEqual([n for n, _ in entries(out)], [1044, 1045, 897])
        self.assertIn("First available: #1044", out)

    def test_lane_at_any_depth(self):
        got = [n for n, _ in entries(run(today(), "--lane", "#2")[1])]
        self.assertIn(1040, got)
        self.assertIn(1048, got)
        self.assertNotIn(1046, got)
        self.assertNotIn(2, got)

    def test_lane_from_the_environment(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out), unittest.mock.patch.dict(
                "os.environ", {"BT_LEAD_LANE": "1043", "BT_LEAD_NAME": "north"}):
            available.main([], source=today())
        self.assertIn("Lead north, lane #1043", out.getvalue())

    def test_nothing_available(self):
        fake = today()
        for n in (1040, 1044, 1045, 1046):
            fake.issues[n]["labels"] = [{"name": "needs-grooming"}]
        self.assertIn("First available: none", run(fake)[1])

    def test_each_issue_read_once(self):
        fake = today()
        run(fake)
        self.assertEqual(len(fake.reads), len(set(fake.reads)))


class OneIssueTest(unittest.TestCase):
    def test_check(self):
        self.assertEqual(run(today(), "--check", "1040")[0], 0)
        self.assertEqual(run(today(), "--check", "1038")[0], 1)
        self.assertEqual(run(today(), "--check", "1036")[0], 1)

    def test_holder_race(self):
        fake = today()
        fake.issues[1040]["comments"] = [claim("north", "2026-11-24T10:00:01Z"), claim("lead", "2026-11-24T10:00:02Z")]
        code, out = run(fake, "--holder", "1040")
        self.assertEqual(code, 1)
        self.assertIn("held by lead north", out)
        fake.issues[1040]["comments"].append(release("north", "2026-11-24T10:00:03Z"))
        self.assertEqual(run(fake, "--holder", "1040")[0], 0)

    def test_holder_none(self):
        code, out = run(today(), "--holder", "1040")
        self.assertEqual(code, 1)
        self.assertIn("no standing claim", out)


class OwnTest(unittest.TestCase):
    def test_claims_count_only_this_lead(self):
        code, out = run(today(), "--claims")
        self.assertIn("Lead lead: 4 claims", out)
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            available.main(["--lead", "north", "--claims"], source=today())
        self.assertIn("Lead north: 0 claims", out.getvalue())

    def test_prs(self):
        prs = [{"number": 1, "title": "a", "body": "Closes #5\n\nModel: x\nLead: north\n"},
               {"number": 2, "title": "b", "body": "Closes #6\n\nModel: x\nLead: lead\n"},
               {"number": 3, "title": "c", "body": "Closes #7\n\nModel: x\n\nLead: is a word here\n"}]
        self.assertEqual([p["number"] for p in available.own_prs(prs, "lead")], [2, 3])
        self.assertEqual([p["number"] for p in available.own_prs(prs, "north")], [1])
        self.assertEqual([p["number"] for p in available.own_prs(prs, "south")], [])

    def test_bad_names(self):
        with self.assertRaises(SystemExit):
            available.main(["--lead", "a b"], source=today())
        with self.assertRaises(SystemExit):
            available.main(["--lead", "maintainer"], source=today())
        with self.assertRaises(SystemExit):
            available.main(["--lane", "1043,x"], source=today())


class GhTest(unittest.TestCase):
    def gh(self, sandbox: bool):
        calls = []

        def runner(argv):
            calls.append(argv)
            if argv[0] == "gh-safe":
                return "#1036 open Replace WindMeteorsEmit\n#1037 closed Add: End-of-tick row\n"
            if argv[:2] == ["gh", "api"]:
                return "1036 open\n1037 closed\n"
            return json.dumps(issue(1040))

        tools = {"gh-safe", "bwai-outside"} if sandbox else {"gh"}
        return available.Gh("o/r", runner, lambda t: f"/bin/{t}" if t in tools else None), calls

    def test_sandbox(self):
        gh, calls = self.gh(True)
        gh.issue(1040)
        self.assertEqual(gh.sub_issues(1038), [(1036, "open"), (1037, "closed")])
        gh.blocked_by(1041)
        self.assertEqual(calls[0][:6], ["bwai-outside", "gh", "issue", "-R", "o/r", "view"])
        self.assertEqual(calls[1], ["gh-safe", "-R", "o/r", "sub-issues", "1038"])
        self.assertEqual(calls[2], ["gh-safe", "-R", "o/r", "blocked-by", "1041"])

    def test_outside_the_sandbox(self):
        gh, calls = self.gh(False)
        gh.issue(1040)
        self.assertEqual(gh.blocked_by(1041), [(1036, "open"), (1037, "closed")])
        gh.sub_issues(1038)
        self.assertEqual(calls[0][:5], ["gh", "issue", "-R", "o/r", "view"])
        self.assertEqual(calls[1][:4], ["gh", "api", "--paginate", "repos/o/r/issues/1041/dependencies/blocked_by"])
        self.assertEqual(calls[2][3], "repos/o/r/issues/1038/sub_issues")

    def test_failure_exits_2(self):
        def runner(argv):
            raise available.GhError("denied")

        gh = available.Gh("o/r", runner, lambda t: None)
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            self.assertEqual(available.main(["--check", "5"], source=gh), 2)
        self.assertIn("denied", err.getvalue())


if __name__ == "__main__":
    unittest.main()
