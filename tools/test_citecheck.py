"""Tests for citecheck.py.

    python3 tools/test_citecheck.py
"""

import csv
import tempfile
import unittest
from pathlib import Path

import citecheck


def tree(tmp: str, note: str) -> Path:
    root = Path(tmp)
    (root / "data").mkdir()
    for name, column in citecheck.NOTES.items():
        with open(root / "data" / name, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["key", column])
            w.writerow(["a", note])
    for name in citecheck.DOCS:
        (root / name).parent.mkdir(exist_ok=True)
        (root / name).write_text("# Notes\n\nDrawProjectiles (weapons.cpp) is fine.\n")
    return root


def doc_tree(tmp: str, text: str) -> Path:
    root = tree(tmp, "Foo (foo.cpp)")
    (root / citecheck.DOCS[0]).write_text(text)
    return root


class CiteCheckTest(unittest.TestCase):
    def test_a_line_range_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            problems = citecheck.check(tree(tmp, "CreateUnit relies on it (foo.cpp:12-34)"))
        self.assertEqual(len(problems), len(citecheck.NOTES))

    def test_a_single_line_in_a_header_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertTrue(citecheck.check(tree(tmp, "see src/a/foo.h:7")))

    def test_a_function_and_file_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(citecheck.check(tree(tmp, "CreateUnit relies on it (Foo (foo.cpp))")), [])

    def test_a_generated_column_may_hold_a_line(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = tree(tmp, "Foo (foo.cpp)")
            with open(root / "data" / "type_dupes.csv", "w", newline="") as f:
                csv.writer(f).writerows([["name", "files"], ["T", "src/units/unit_types.h:4465|src/a.cpp:18"]])
            self.assertEqual(citecheck.check(root), [])

    def test_a_line_citation_in_the_state_document_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            problems = citecheck.check(doc_tree(tmp, "ok\nCmdEdge (src/game/console_commands.cpp:731-732) writes\n"))
        self.assertEqual(len(problems), 1)
        self.assertIn("docs/state.md line 2", problems[0])

    def test_a_function_and_file_in_the_state_document_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(citecheck.check(doc_tree(tmp, "`CmdEdge` (console_commands.cpp) writes\n")), [])

    def test_a_missing_state_document_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = tree(tmp, "Foo (foo.cpp)")
            (root / citecheck.DOCS[0]).unlink()
            self.assertEqual(len(citecheck.check(root)), 1)

    def test_the_repository_passes(self):
        self.assertEqual(citecheck.check(Path(__file__).resolve().parent.parent), [])


if __name__ == "__main__":
    unittest.main()
