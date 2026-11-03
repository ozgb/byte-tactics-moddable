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
