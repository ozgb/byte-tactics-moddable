"""Tests for fullgate.py, on a throwaway repository and fake comments (no network).

    uv run tools/test_fullgate.py
"""

import contextlib
import io
import json
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import fullgate


def run_git(repo: Path, *args: str) -> str:
    env = {"GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t", "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@t",
           "GIT_CONFIG_GLOBAL": "/dev/null", "GIT_CONFIG_SYSTEM": "/dev/null", "PATH": "/usr/bin:/bin"}
    return subprocess.run(["git", "-C", str(repo), *args], check=True, capture_output=True, text=True, env=env).stdout.strip()


def make_repo(root: Path) -> Path:
    repo = root / "repo"
    repo.mkdir()
    run_git(repo, "init", "-q", "-b", "moddable")
    (repo / "README.md").write_text("start\n")
    run_git(repo, "add", ".")
    run_git(repo, "commit", "-q", "-m", "Add: Start")
    return repo


def merge(repo: Path, name: str, path: str) -> str:
    """Merge a branch that changes `path` into moddable with a merge commit; its sha."""
    run_git(repo, "checkout", "-q", "-b", name)
    target = repo / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(name + "\n")
    run_git(repo, "add", ".")
    run_git(repo, "commit", "-q", "-m", f"Add: {name}")
    run_git(repo, "checkout", "-q", "moddable")
    run_git(repo, "merge", "-q", "--no-ff", "-m", f"Merge pull request from {name}", name)
    return run_git(repo, "rev-parse", "HEAD")


def comment(body: str, at: str = "") -> dict:
    return {"body": body, "createdAt": at}


class PathsTest(unittest.TestCase):
    def test_the_paths_the_gate_applies_to(self):
        for path in ("src/game/economy.cpp", "interfaces/state.h", "reference/scenarios/a.txt", "tools/reference.py",
                     "data/trace_schema.json", "CMakeLists.txt", "lib/bitexact/x.cpp"):
            self.assertTrue(fullgate.needs_gate([path]), path)

    def test_the_paths_it_does_not(self):
        for path in ("docs/team.md", "tools/coverage.py", "data/coverage.csv", "AGENTS.md", ".github/workflows/ci.yml", "srcx/a.cpp"):
            self.assertFalse(fullgate.needs_gate([path]), path)

    def test_one_path_is_enough(self):
        self.assertTrue(fullgate.needs_gate(["docs/team.md", "src/a.cpp"]))
        self.assertFalse(fullgate.needs_gate([]))


class CommentsTest(unittest.TestCase):
    def test_the_newest_full_gate_comment_wins(self):
        comments = [comment("Full gate on aaaaaaa1: 65 of 65 equal", "2026-11-01T00:00:00Z"),
                    comment("Full gate on bbbbbbb2: 65 of 65 equal", "2026-11-03T00:00:00Z"),
                    comment("Full gate on ccccccc3: 65 of 65 equal", "2026-11-02T00:00:00Z")]
        self.assertEqual(fullgate.last_full_run(comments), "bbbbbbb2")

    def test_position_orders_comments_without_dates(self):
        comments = [comment("Full gate on aaaaaaa1: 1 of 1 equal"), comment("Full gate on bbbbbbb2: 1 of 1 equal")]
        self.assertEqual(fullgate.last_full_run(comments), "bbbbbbb2")

    def test_other_comments_are_ignored(self):
        comments = [comment("Tracking the full gate on moddable."), comment("The Full gate on abc failed", "x"),
                    comment("see Full gate on abcdef12")]
        self.assertIsNone(fullgate.last_full_run(comments))
        self.assertIsNone(fullgate.last_full_run([]))

    def test_parse_gh_json(self):
        text = json.dumps({"comments": [comment("Full gate on abcdef12: 3 of 3 equal", "t")]})
        self.assertEqual(fullgate.last_full_run(fullgate.parse_comments(text)), "abcdef12")
        self.assertEqual(fullgate.parse_comments("{}"), [])

    def test_default_command_names_the_issue_and_repo(self):
        cmd = fullgate.default_command()
        self.assertEqual(cmd[:2], ["bwai-outside", "gh"])
        self.assertIn("ozgb/byte-tactics-moddable", cmd)
        self.assertIn(str(fullgate.TRACKING_ISSUE), cmd)


class CountTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = make_repo(Path(self.tmp.name))
        patch = mock.patch.object(fullgate, "ROOT", self.repo)
        patch.start()
        self.addCleanup(patch.stop)

    def test_counts_the_merges_after_the_sha_that_need_the_gate(self):
        merge(self.repo, "docs-1", "docs/a.md")
        full = merge(self.repo, "src-1", "src/a.cpp")
        merge(self.repo, "src-2", "src/b.cpp")
        merge(self.repo, "docs-2", "docs/b.md")
        merge(self.repo, "ref-1", "reference/scenarios/x.txt")
        merges = fullgate.merges_since("moddable", full)
        self.assertEqual(len(merges), 3)
        self.assertEqual(fullgate.count_needing_gate(merges), 2)

    def test_without_a_full_run_everything_counts(self):
        merge(self.repo, "src-1", "src/a.cpp")
        merge(self.repo, "docs-1", "docs/a.md")
        self.assertEqual(fullgate.count_needing_gate(fullgate.merges_since("moddable", None)), 1)

    def test_the_branch_commits_are_not_counted_apart(self):
        full = run_git(self.repo, "rev-parse", "HEAD")
        merge(self.repo, "src-1", "src/a.cpp")
        self.assertEqual(len(fullgate.merges_since("moddable", full)), 1)

    def run_main(self, *args):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = fullgate.main(list(args))
        return code, out.getvalue()

    def comments_file(self, sha: str) -> str:
        path = Path(self.tmp.name) / "comments.json"
        path.write_text(json.dumps({"comments": [comment(f"Full gate on {sha}: 65 of 65 equal", "t")]}))
        return str(path)

    def test_due_exits_1_at_five(self):
        full = run_git(self.repo, "rev-parse", "HEAD")
        for n in range(4):
            merge(self.repo, f"src-{n}", f"src/{n}.cpp")
        file = self.comments_file(full)
        code, out = self.run_main("due", "--ref", "moddable", "--comments-file", file)
        self.assertEqual(code, 0)
        self.assertIn("4 merged pull requests needed the gate since the full gate on", out)
        merge(self.repo, "src-4", "src/4.cpp")
        code, out = self.run_main("due", "--ref", "moddable", "--comments-file", file)
        self.assertEqual(code, 1)
        self.assertIn("5 merged pull requests", out)

    def test_without_the_action_it_only_prints(self):
        full = run_git(self.repo, "rev-parse", "HEAD")
        for n in range(6):
            merge(self.repo, f"src-{n}", f"src/{n}.cpp")
        code, out = self.run_main("--ref", "moddable", "--comments-file", self.comments_file(full))
        self.assertEqual(code, 0)
        self.assertIn("6 merged", out)

    def test_the_comments_command_is_replaceable(self):
        full = run_git(self.repo, "rev-parse", "HEAD")
        body = json.dumps({"comments": [comment(f"Full gate on {full}: 1 of 1 equal")]})
        code, out = self.run_main("--ref", "moddable", "--comments-command", f"echo {body!r}")
        self.assertEqual(code, 0)
        self.assertIn("0 merged pull requests", out)

    def test_a_failing_command_exits_with_its_error(self):
        with self.assertRaises(SystemExit) as raised:
            self.run_main("--ref", "moddable", "--comments-command", "false")
        self.assertIn("failed", str(raised.exception))


if __name__ == "__main__":
    unittest.main()
