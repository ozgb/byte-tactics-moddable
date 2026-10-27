"""Tests for subsystemsdoc.py, on a fixture doc and hand-written counts.

    uv run tools/test_subsystemsdoc.py
"""

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import subsystemsdoc as sd

COUNTS = {
    "alpha": {"functions": 16, "called": 11, "imported": 14, "targets": ["platform", "weapons"], "written": 24},
    "beta": {"functions": 5, "called": 3, "imported": 7, "targets": ["alpha"], "written": 2},
}
DOC = """# Subsystems

The 21 functions of `data/rwsets.csv` are in 2 subsystems.

| subsystem | functions | issue |
|---|---|---|
| [alpha](#alpha) | 16 | #9 |
| [beta](#the-remaining-subsystems) | 5 | none yet |

## alpha

Issue #9. 16 functions. The first.

11 functions are called from other subsystems. The most called:

**Imports.** 14 functions of 2 subsystems, by target:

**Sim writes.** 24 items in all, 19 after leaving out the shared value types. The most used:

- Exports: 11 functions (all).
- Needs from seams: 14 functions in platform, weapons.

## The remaining subsystems

**beta** (5 functions). No phase root. 3 functions are called from outside. It imports 7 functions. Its packet handlers write sim items (2 in all).

## Coverage
"""


def run_main(doc: Path, flag: str) -> tuple[int, str]:
    out = io.StringIO()
    code = 0
    with mock.patch.object(sd, "load_counts", return_value=(COUNTS, 21)), mock.patch.object(
            sys, "argv", ["subsystemsdoc.py", flag, "--doc", str(doc)]), contextlib.redirect_stdout(out):
        try:
            sd.main()
        except SystemExit as exit:
            code = exit.code if isinstance(exit.code, int) else 1
    return code, out.getvalue()


class SubsystemsDocTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.path = Path(self.dir.name) / "subsystems.md"

    def test_a_matching_doc_passes(self):
        self.path.write_text(DOC)
        self.assertEqual(run_main(self.path, "--check")[0], 0)

    def test_a_wrong_count_names_its_section(self):
        self.path.write_text(DOC.replace("**Imports.** 14 functions", "**Imports.** 13 functions"))
        code, out = run_main(self.path, "--check")
        self.assertEqual(code, 1)
        self.assertIn("alpha, imported functions and subsystems: doc has 13, tool has 14", out)

    def test_a_wrong_paragraph_count_names_its_section(self):
        self.path.write_text(DOC.replace("(2 in all)", "(9 in all)"))
        code, out = run_main(self.path, "--check")
        self.assertEqual(code, 1)
        self.assertIn("beta, sim writes: doc has 9, tool has 2", out)

    def test_a_wrong_overall_total_fails(self):
        self.path.write_text(DOC.replace("The 21 functions", "The 22 functions"))
        code, out = run_main(self.path, "--check")
        self.assertEqual(code, 1)
        self.assertIn("overall, function total: doc has 22, tool has 21", out)

    def test_a_different_set_of_names_fails(self):
        self.path.write_text(DOC.replace("platform, weapons", "platform, map"))
        code, out = run_main(self.path, "--check")
        self.assertEqual(code, 1)
        self.assertIn("alpha, needs from seams", out)

    def test_the_same_names_in_another_order_pass(self):
        self.path.write_text(DOC.replace("platform, weapons", "weapons, platform"))
        self.assertEqual(run_main(self.path, "--check")[0], 0)

    def test_a_subsystem_without_a_section_fails(self):
        self.path.write_text(DOC.replace("**beta** (5 functions)", "**gamma** (5 functions)"))
        code, out = run_main(self.path, "--check")
        self.assertEqual(code, 1)
        self.assertIn("beta, section", out)

    def test_write_fixes_the_doc_and_changes_only_numbers(self):
        wrong = DOC.replace("16 functions. The", "17 functions. The").replace("Exports: 11", "Exports: 12").replace(
            "5 functions)", "6 functions)").replace("| 16 |", "| 15 |")
        self.path.write_text(wrong)
        code, _ = run_main(self.path, "--write")
        self.assertEqual(code, 0)
        self.assertEqual(self.path.read_text(), DOC)
        self.assertEqual(run_main(self.path, "--check")[0], 0)


if __name__ == "__main__":
    unittest.main()
