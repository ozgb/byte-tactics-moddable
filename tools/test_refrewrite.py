"""Tests for refrewrite.py, on the fixtures in tools/testdata/refrewrite.

Needs clang-cl 22.1.8, so it runs where the modern build does (the
modern-compile job), not in the checks job.

    uv run tools/test_refrewrite.py
"""

import contextlib
import io
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import refcheck
import refrewrite

DATA = Path(__file__).parent / "testdata/refrewrite"
FILE = "src/units/sample.h"
POINTERS = ["a", "b", "c", "d", "e", "f", "fixed", "g", "h", "inUnion", "kept", "m", "n", "spaced", "viaMacro"]


def fake_analyse(outside=("kept",), fields=POINTERS):
    found = {("Holder", name, "pointer"): refcheck.Found("Holder", name, "pointer", {FILE}) for name in fields}
    found[("Holder", "callback", "function-pointer")] = refcheck.Found("Holder", "callback", "function-pointer", {FILE})
    rows = [{"struct": "Holder", "field": name} for name in outside]
    return lambda root, build_dir, jobs: refcheck.Result(set(), [], found, rows, [], 0, [], {})


class Tree:
    def __init__(self, test: unittest.TestCase, outside=("kept",), fields=POINTERS):
        self.dir = tempfile.TemporaryDirectory()
        test.addCleanup(self.dir.cleanup)
        self.root = Path(self.dir.name)
        (self.root / "src/units").mkdir(parents=True)
        self.header = self.root / "src/units/sample.h"
        shutil.copy(DATA / "input.fixture", self.header)
        patch = mock.patch.object(refcheck, "analyse", fake_analyse(outside, fields))
        patch.start()
        test.addCleanup(patch.stop)

    def run(self, *args: str) -> tuple[int, str, str]:
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = refrewrite.main(["--plain", "--root", str(self.root), *args])
        return code, out.getvalue(), err.getvalue()


class RefRewriteTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        version = subprocess.run(["clang-cl", "--version"], capture_output=True, text=True).stdout
        if "22.1.8" not in version:
            raise unittest.SkipTest("clang-cl 22.1.8 is required")

    def test_rewrites_by_struct_and_skips_the_outside_list(self):
        tree = Tree(self)
        code, _, err = tree.run("--struct", "Holder")
        self.assertEqual(code, 0)
        self.assertEqual(tree.header.read_text(), (DATA / "expected.fixture").read_text())
        self.assertIn("Holder::fixed (src/units/sample.h): not a plain pointer declarator", err)
        self.assertIn("Holder::viaMacro (src/units/sample.h): declared through a macro", err)
        self.assertIn("Holder::e is a pointer to pointer, now Ref<Target*>", err)

    def test_dry_run_writes_nothing(self):
        tree = Tree(self)
        before = tree.header.read_text()
        code, out, _ = tree.run("--struct", "Holder", "--dry-run")
        self.assertEqual(code, 0)
        self.assertEqual(tree.header.read_text(), before)
        self.assertIn("-    Target *a;", out)
        self.assertIn("+    Ref<Target> a;", out)
        self.assertIn("+#include \"../../interfaces/ref.h\"", out)

    def test_struct_takes_a_field_that_is_not_on_the_outside_list(self):
        tree = Tree(self, outside=())
        tree.run("--struct", "Holder")
        self.assertIn("Ref<Target> kept;", tree.header.read_text())

    def test_field_selects_only_that_field(self):
        tree = Tree(self, outside=())
        tree.run("--field", "Holder.kept")
        text = tree.header.read_text()
        self.assertIn("Ref<Target> kept;", text)
        self.assertIn("Target *a;", text)

    def test_second_run_changes_nothing(self):
        tree = Tree(self)
        tree.run("--struct", "Holder")
        after = tree.header.read_text()
        code, out, _ = tree.run("--struct", "Holder", "--dry-run")
        self.assertEqual(code, 0)
        self.assertEqual(out, "")
        self.assertEqual(tree.header.read_text(), after)

    def test_existing_include_is_kept(self):
        text = '#pragma once\n#include "../../interfaces/ref.h"\nstruct S;\n'
        self.assertEqual(refrewrite.add_include(text, Path("/r/src/units/s.h"), Path("/r/interfaces/ref.h")), text)

    def test_include_after_last_include(self):
        text = '#include "a.h"\n#include "b.h"\n\nstruct S;\n'
        out = refrewrite.add_include(text, Path("/r/src/s.cpp"), Path("/r/interfaces/ref.h"))
        self.assertEqual(out, '#include "a.h"\n#include "b.h"\n#include "../interfaces/ref.h"\n\nstruct S;\n')

    def test_comments_and_alignment_survive(self):
        tree = Tree(self, fields=["a", "b", "c", "d"])
        tree.header.write_text(
            "struct T { int v; };\n"
            "struct Holder {\n"
            "    T*     a;      // first\n"
            "    int    b;      // second\n"
            "    T *c, *d;      // pair\n"
            "};\n")
        tree.run("--field", "Holder.a", "--field", "Holder.c")
        self.assertEqual(
            tree.header.read_text(),
            '#include "../../interfaces/ref.h"\n'
            "struct T { int v; };\n"
            "struct Holder {\n"
            "    Ref<T>     a;      // first\n"
            "    int    b;      // second\n"
            "    Ref<T> c;\n"
            "    T *d;      // pair\n"
            "};\n")


if __name__ == "__main__":
    unittest.main()
