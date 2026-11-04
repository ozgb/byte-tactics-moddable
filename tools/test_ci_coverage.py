"""Fails when a tools/test_*.py has no step in CI.

    python3 tools/test_ci_coverage.py
"""

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

NOT_IN_CI = {
    "test_harness_load.py": "needs Wine and build/modern/TotalA.exe",
    "test_blockscan.py": "plays scenarios: needs Wine, the exe and game data",
}


def unnamed(root: Path = ROOT) -> list[str]:
    """The tools/test_*.py files that neither ci.yml nor tools/ci_replays.sh names, bar NOT_IN_CI."""
    ci = (root / ".github" / "workflows" / "ci.yml").read_text() + (root / "tools" / "ci_replays.sh").read_text()
    names = sorted(p.name for p in (root / "tools").glob("test_*.py"))
    return [n for n in names if n not in NOT_IN_CI and n not in ci]


class CiCoverage(unittest.TestCase):
    def test_every_tool_test_has_a_ci_step(self):
        self.assertEqual(unnamed(), [], "add a step to .github/workflows/ci.yml, or an entry with a reason to NOT_IN_CI")

    def test_the_allow_list_names_tests_that_exist(self):
        for name in NOT_IN_CI:
            self.assertTrue((ROOT / "tools" / name).exists(), name)

    def test_the_allow_list_holds_only_tests_ci_does_not_run(self):
        ci = (ROOT / ".github" / "workflows" / "ci.yml").read_text() + (ROOT / "tools" / "ci_replays.sh").read_text()
        for name in NOT_IN_CI:
            self.assertNotIn(name, ci, f"{name} has a CI step: drop it from NOT_IN_CI")

    def test_a_test_with_no_step_is_reported(self):
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / ".github" / "workflows").mkdir(parents=True)
            (root / "tools").mkdir()
            (root / ".github" / "workflows" / "ci.yml").write_text("run: python3 tools/test_a.py\n")
            (root / "tools" / "ci_replays.sh").write_text("")
            for n in ("test_a.py", "test_b.py"):
                (root / "tools" / n).write_text("")
            self.assertEqual(unnamed(root), ["test_b.py"])


if __name__ == "__main__":
    unittest.main()
