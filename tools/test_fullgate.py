"""Tests for fullgate.py, on a temporary repository and fake issue comments, with no network.

    uv run tools/test_fullgate.py
"""

import contextlib
import io
import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
import unittest.mock
from pathlib import Path

import fullgate

ENV = {**os.environ, "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t", "GIT_COMMITTER_NAME": "t",
       "GIT_COMMITTER_EMAIL": "t@t"}


def git(repo: Path, *args: str) -> str:
    return subprocess.run(["git", "-C", str(repo), "-c", "commit.gpgsign=false", *args], check=True,
                          capture_output=True, text=True, env=ENV).stdout.strip()


def merge_pr(repo: Path, number: int, rel: str) -> str:
    git(repo, "checkout", "-q", "-b", f"pr{number}", "main")
    target = repo / rel
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(f"{number}\n")
    git(repo, "add", rel)
    git(repo, "commit", "-q", "-m", f"Update: {rel}")
    git(repo, "checkout", "-q", "main")
    git(repo, "merge", "-q", "--no-ff", "-m", f"Merge pull request #{number} from ozgb/issue-{number}", f"pr{number}")
    return git(repo, "rev-parse", "HEAD")


def comment(body: str, when: str) -> dict:
    return {"body": body, "createdAt": when}


class FullGateTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = Path(self.tmp.name) / "repo"
        self.repo.mkdir()
        git(self.repo, "init", "-q", "-b", "main")
        (self.repo / "README").write_text("x\n")
        git(self.repo, "add", "README")
        git(self.repo, "commit", "-q", "-m", "Add: Start")
        self.base = git(self.repo, "rev-parse", "HEAD")
        self.saved = fullgate.ROOT
        fullgate.ROOT = self.repo
        self.addCleanup(setattr, fullgate, "ROOT", self.saved)

    def finish(self):
        git(self.repo, "update-ref", "refs/remotes/origin/moddable", "main")

    def run_main(self, comments, *args):
        path = Path(self.tmp.name) / "comments.json"
        path.write_text(json.dumps({"comments": comments}))
        out, err = io.StringIO(), io.StringIO()
        old = sys.argv
        sys.argv = ["fullgate.py", "--no-fetch", "--comments-json", str(path), *args]
        try:
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                try:
                    code = fullgate.main()
                except SystemExit as exit_:
                    code = exit_.code
        finally:
            sys.argv = old
        return code, out.getvalue(), err.getvalue()

    def merges(self, count, rel="src/game/a.cpp"):
        for number in range(100, 100 + count):
            merge_pr(self.repo, number, f"{rel[:-4]}{number}{rel[-4:]}")
        self.finish()

    def full_run(self):
        return [comment(f"Full gate on {self.base[:9]}: 65 of 65 equal, block pointers: ok", "2026-11-02T10:00:00Z")]

    def test_below_cadence_is_not_due(self):
        self.merges(fullgate.CADENCE - 1)
        code, out, _ = self.run_main(self.full_run(), "due")
        self.assertEqual(code, 0)
        self.assertIn("4 of 5 merges since the full gate on", out)
        self.assertIn("#100, #101, #102, #103", out)
        self.assertIn("not due", out)

    def test_cadence_is_due(self):
        self.merges(fullgate.CADENCE)
        code, out, _ = self.run_main(self.full_run(), "due")
        self.assertEqual(code, 1)
        self.assertIn("5 of 5 merges", out)
        self.assertEqual(self.run_main(self.full_run())[0], 0)

    def test_docs_only_merge_is_not_counted(self):
        merge_pr(self.repo, 100, "docs/notes.md")
        merge_pr(self.repo, 101, "src/game/b.cpp")
        merge_pr(self.repo, 102, "tools/other.py")
        self.finish()
        code, out, _ = self.run_main(self.full_run())
        self.assertEqual(code, 0)
        self.assertIn("1 of 5 merges", out)
        self.assertIn(": #101", out)
        self.assertNotIn("#100", out)

    def test_newest_full_gate_comment_wins(self):
        comments = [
            comment("Full gate on aaaaaaa: 65 of 65 equal", "2026-11-01T10:00:00Z"),
            comment("Full gate on " + self.base + ": 65 of 65 equal", "2026-11-02T10:00:00Z"),
            comment("The full gate is overdue", "2026-11-03T10:00:00Z"),
        ]
        self.assertEqual(fullgate.last_full_run(comments), self.base)
        self.assertEqual(fullgate.last_full_run(list(reversed(comments))), self.base)

    def test_a_merge_before_the_full_run_is_not_counted(self):
        merge_pr(self.repo, 100, "src/game/c.cpp")
        before = git(self.repo, "rev-parse", "HEAD")
        merge_pr(self.repo, 101, "src/game/d.cpp")
        self.finish()
        code, out, _ = self.run_main([comment(f"Full gate on {before}: ok", "2026-11-02T10:00:00Z")])
        self.assertEqual(code, 0)
        self.assertIn("1 of 5 merges", out)
        self.assertNotIn("#100", out)

    def test_no_comment_exits_2(self):
        self.merges(1)
        code, _, err = self.run_main([comment("hello", "2026-11-02T10:00:00Z")], "due")
        self.assertEqual(code, 2)
        self.assertIn("no full run is recorded", err)

    def test_no_comment_source_exits_2(self):
        env = {**os.environ, "FULLGATE_GH": "", "PATH": "/nonexistent"}
        with unittest.mock.patch.dict(os.environ, env, clear=True):
            with self.assertRaises(SystemExit) as raised:
                with contextlib.redirect_stderr(io.StringIO()):
                    fullgate.read_comments(None)
        self.assertEqual(raised.exception.code, 2)


BOTH_MODES = """146 of 146 scenarios equal, total 5:34
[wasm] block pointers: all covered by data/block_pointers.csv (73 scenarios)
[native] block pointers: all covered by data/block_pointers.csv (73 scenarios)
"""
BOTH_MODES_BAD = """145 of 146 scenarios equal, total 5:34
[wasm] block pointers: all covered by data/block_pointers.csv (73 scenarios)
[native] 10 problems with the pointers out of the block; add them to data/block_pointers.csv
"""
COVERAGE = """profile: build/coverage-run/merged.profdata
excluded (harness, build, outside the repository): 880 functions
unassigned: 0 functions
coverage: no subsystem fell
"""
COVERAGE_FELL = """coverage fell:
ai functions: 80.0% (80/100) fell from 90.0% (90/100)
"""
ROUND_TRIP = """73 of 73 scenarios equal, total 3:02
block pointers: all covered by data/block_pointers.csv (73 scenarios)
"""


class CommentTextTest(unittest.TestCase):
    def test_all_equal(self):
        text = fullgate.comment_text("abc1234", (0, BOTH_MODES), (0, ROUND_TRIP), (0, COVERAGE))
        self.assertEqual(text, "Full gate on abc1234: 146 of 146 equal (both modes), block pointers: "
                               "all covered by data/block_pointers.csv in both modes, round trip: 73 of 73 equal, "
                               "coverage: no subsystem fell")
        self.assertEqual(fullgate.last_full_run([comment(text, "2026-11-04T10:00:00Z")]), "abc1234")

    def test_the_host_line_of_the_check_ends_the_comment(self):
        host = "host: Some CPU (GenuineIntel), Fedora Linux 44 7.2.7, wine-11.0 (Staging)"
        text = fullgate.comment_text("abc1234", (0, host + "\n" + BOTH_MODES), (0, ROUND_TRIP), (0, COVERAGE))
        self.assertEqual(text.split("\n"), [fullgate.comment_text("abc1234", (0, BOTH_MODES), (0, ROUND_TRIP), (0, COVERAGE)), host])
        self.assertEqual(fullgate.last_full_run([comment(text, "2026-11-04T10:00:00Z")]), "abc1234")

    def test_pointer_problems_name_the_mode(self):
        self.assertEqual(fullgate.pointer_result(BOTH_MODES_BAD),
                         "[wasm] all covered by data/block_pointers.csv; [native] 10 block pointer problems")

    def test_round_trip_divergence_and_exit_status(self):
        trip = "70 of 73 scenarios equal, total 3:02\n"
        text = fullgate.comment_text("abc1234", (1, BOTH_MODES_BAD), (1, trip), (0, COVERAGE))
        self.assertIn("145 of 146 equal (both modes)", text)
        self.assertTrue(text.endswith("round trip: 70 of 73 equal, coverage: no subsystem fell, both modes exit 1, round trip exit 1"))

    def test_a_run_that_did_not_finish(self):
        text = fullgate.comment_text("abc1234", (1, ""), (0, ROUND_TRIP), (0, COVERAGE))
        self.assertIn("not finished equal (both modes), block pointers: not reported", text)

    def test_round_trip_pointer_problems_are_named(self):
        trip = "73 of 73 scenarios equal, total 3:02\n2 problems with the pointers out of the block; add them\n"
        text = fullgate.comment_text("abc1234", (0, BOTH_MODES), (1, trip), (0, COVERAGE))
        self.assertTrue(text.endswith("round trip: 73 of 73 equal, coverage: no subsystem fell (block pointers: 2 block pointer problems), round trip exit 1"))

    def test_coverage_fall_names_the_status(self):
        text = fullgate.comment_text("abc1234", (0, BOTH_MODES), (0, ROUND_TRIP), (1, COVERAGE_FELL))
        self.assertTrue(text.endswith("round trip: 73 of 73 equal, coverage: failed, coverage exit 1"))

    def test_coverage_build_failure_and_no_output(self):
        self.assertEqual(fullgate.coverage_result(fullgate.BUILD_FAILED, "FAILED: link\n"), "build failed")
        self.assertEqual(fullgate.coverage_result(1, ""), "not finished")
        self.assertEqual(fullgate.coverage_result(0, COVERAGE), "no subsystem fell")
        text = fullgate.comment_text("abc1234", (0, BOTH_MODES), (0, ROUND_TRIP), (fullgate.BUILD_FAILED, "x"))
        self.assertTrue(text.endswith("coverage: build failed, coverage exit 1"))

    def test_coverage_check_builds_then_plays(self):
        calls = []

        def fake_stream(command, log, what):
            calls.append(command)
            return (0, COVERAGE) if command[:3] == ["uv", "run", "tools/coverage.py"] else (0, "")

        with unittest.mock.patch.object(fullgate, "stream", fake_stream), unittest.mock.patch.object(
                fullgate.shutil, "rmtree"):
            self.assertEqual(fullgate.coverage_check("g", None), (0, COVERAGE))
        self.assertEqual(calls[:-1], list(fullgate.COVERAGE_BUILD))
        self.assertEqual(calls[-1][-3:], ["--check", "--game-dir", "g"])
        calls.clear()

        def failing_build(command, log, what):
            calls.append(command)
            return 1, "link failed\n"

        with unittest.mock.patch.object(fullgate, "stream", failing_build):
            self.assertEqual(fullgate.coverage_check("g", None), (fullgate.BUILD_FAILED, "link failed\n"))
        self.assertEqual(len(calls), 1)

    def test_single_mode_pointers(self):
        out = "73 of 73 scenarios equal, total 1:00\nblock pointers: all covered by data/block_pointers.csv (73 scenarios)\n"
        self.assertEqual(fullgate.pointer_result(out), "all covered by data/block_pointers.csv")

    def test_run_plays_both_checks_and_prints_the_comment(self):
        played = []

        def fake_play(args, log, what):
            played.append((what, args))
            return (0, BOTH_MODES) if what == "both modes" else (0, ROUND_TRIP)

        out = io.StringIO()
        old = sys.argv
        sys.argv = ["fullgate.py", "run", "--sha", "abc1234", "--exe", "x.exe", "--game-dir", ""]
        try:
            with unittest.mock.patch.object(fullgate, "play", fake_play), unittest.mock.patch.object(
                    fullgate, "coverage_check", lambda game_dir, log: (0, COVERAGE)), unittest.mock.patch.object(
                    fullgate.shutil, "rmtree"), contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                code = fullgate.main()
        finally:
            sys.argv = old
        self.assertEqual(code, 0)
        self.assertEqual([what for what, _ in played], ["both modes", "round trip"])
        self.assertEqual(played[0][1], ["--exe", "x.exe", "--both-modes"])
        self.assertEqual(played[1][1], ["--exe", "x.exe", "--roundtrip", "auto"])
        self.assertIn("round trip: 73 of 73 equal, coverage: no subsystem fell", out.getvalue())


class GatePathsTest(unittest.TestCase):
    def test_paths_match_the_readme(self):
        text = (fullgate.ROOT / "reference/README.md").read_text()
        section = text.split("\n## Gate\n", 1)[1].split("\n## ", 1)[0]
        paragraph = next((p for p in section.split("\n\n") if p.startswith("The gate applies when")), None)
        self.assertIsNotNone(paragraph, "reference/README.md, Gate, lost its `The gate applies when` paragraph")
        named = re.findall(r"`([^`\s]+)`", paragraph)
        self.assertTrue(named)
        self.assertEqual(sorted(named), sorted(fullgate.GATE_PATHS))


if __name__ == "__main__":
    unittest.main()
