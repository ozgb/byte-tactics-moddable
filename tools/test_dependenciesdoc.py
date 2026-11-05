"""Tests for dependenciesdoc.py, on synthetic inputs.

    uv run tools/test_dependenciesdoc.py
"""

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import dependenciesdoc as dd

SIMS = ["a", "b", "c", "d", "e"]


def dep(writer, reader, cls, phase="units", item="X::y", runs="3"):
    return {"writer": writer, "reader": reader, "item": item, "phase": phase, "class": cls, "runs": runs,
            "example": "s@1", "flags": ""}


def check(writer, reader, held="0", group="g1"):
    return {"writer": writer, "reader": reader, "item": "X::y", "phase": "units", "group": group, "scenarios": "83",
            "result": "equal", "scenarios_counted": "82", "defer_held": held, "defer_third_stores": "0"}


def data(deps, checks):
    return {"deps": deps, "phasedeps": [], "bounds": [], "draws": [], "passreads": [], "checks": checks, "pres": [],
            "sims": SIMS, "scenarios": 4}


class Groups(unittest.TestCase):
    def test_per_phase_and_union(self):
        deps = [dep("a", "b", "immediate", "units"), dep("c", "b", "ordered", "units"), dep("d", "e", "immediate", "players"),
                dep("b", "d", "immediate", "features"), dep("a", "e", "deferrable", "units")]
        got = dd.tight_groups(deps, SIMS)
        self.assertEqual(got["units"], [["a", "b", "c"]])
        self.assertEqual(got["players"], [["d", "e"]])
        self.assertEqual(got["all"], [["a", "b", "c", "d", "e"]])

    def test_other_classes_do_not_join(self):
        deps = [dep("a", "b", "deferrable"), dep("a", "c", "read-only"), dep("a", "d", "unobserved")]
        self.assertEqual(dd.tight_groups(deps, SIMS), {})

    def test_non_sim_rows_ignored(self):
        self.assertEqual(dd.tight_groups([dep("-", "a", "immediate"), dep("game", "a", "immediate")], SIMS), {})


class Cut(unittest.TestCase):
    def test_rule(self):
        deps = [dep("a", "b", "immediate", runs="2"), dep("c", "a", "ordered"), dep("a", "d", "deferrable"),
                dep("e", "a", "read-only"), dep("a", "b", "immediate", phase="players", runs="9")]
        can, blockers = dd.cut_from("a", deps, SIMS)
        self.assertEqual(can, ["d", "e"])
        self.assertEqual(sorted(blockers), ["b", "c"])
        self.assertEqual(blockers["b"]["runs"], "9")
        self.assertEqual(blockers["c"]["class"], "ordered")

    def test_reader_side_counts(self):
        can, blockers = dd.cut_from("b", [dep("a", "b", "immediate")], SIMS)
        self.assertNotIn("a", can)
        self.assertIn("c", can)


class Held(unittest.TestCase):
    def test_zero_everywhere(self):
        text = dd.held_claim([check("a", "b"), check("a", "c", group="g2")])
        self.assertIn("is 0 for every one of the 2 edges in 2 groups", text)

    def test_nonzero_named(self):
        text = dd.held_claim([check("a", "b"), check("a", "c", held="7", group="g2")])
        self.assertIn("not 0 everywhere", text)
        self.assertIn("g2: 7", text)
        self.assertIn("1 of 2 edges", text)


class Render(unittest.TestCase):
    def doc(self):
        return dd.render(data([dep("a", "b", "immediate"), dep("a", "c", "deferrable")], [check("a", "c")]))

    def test_content(self):
        text = self.doc()
        self.assertIn("Edges in data/dependencies.csv: 0 ordered, 1 immediate, 1 deferrable, 0 read-only, 0 unobserved.", text)
        self.assertIn("- 2 members: a, b", text)
        self.assertIn("4 scenarios", text)
        self.assertIn("## Limits and known gaps", text)
        self.assertEqual(text, self.doc())

    def run_main(self, args, doc):
        out = io.StringIO()
        with mock.patch.object(dd, "load", lambda: data([dep("a", "b", "immediate")], [])), \
                mock.patch.object(sys, "argv", ["dependenciesdoc"] + args + ["--doc", str(doc)]), \
                contextlib.redirect_stdout(out):
            try:
                dd.main()
                return 0, out.getvalue()
            except SystemExit as exit_:
                return exit_.code, out.getvalue()

    def test_check_fails_on_stale_doc(self):
        with tempfile.TemporaryDirectory() as tmp:
            doc = Path(tmp) / "dependencies.md"
            self.assertEqual(self.run_main(["--write"], doc)[0], 0)
            self.assertEqual(self.run_main(["--check"], doc)[0], 0)
            doc.write_text(doc.read_text().replace("1 immediate", "2 immediate"))
            code, out = self.run_main(["--check"], doc)
            self.assertEqual(code, 1)
            self.assertIn("out of date", out)

    def test_check_fails_on_missing_doc(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(self.run_main(["--check"], Path(tmp) / "none.md")[0], 1)


if __name__ == "__main__":
    unittest.main()
