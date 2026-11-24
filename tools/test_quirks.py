"""Tests for the doc comments of interfaces/quirks.h against quirks.def.

    uv run tools/test_quirks.py
"""

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEF_RE = re.compile(r'^QUIRK\((\w+), "([^"]+)", (\w+),', re.M)
MEMBER_RE = re.compile(r"^\s*//\s*(.*)\n\s*int (\w+);", re.M)


def check(def_text, header_text):
    """Return the list of problems between a quirks.def text and a quirks.h text."""
    flags = DEF_RE.findall(def_text)
    body = header_text[header_text.index("struct Quirks"):]
    members = MEMBER_RE.findall(body)
    problems = []
    if [m for _, m in members] != [f[0] for f in flags]:
        problems.append("members differ from quirks.def or are out of order: %s vs %s" % ([m for _, m in members], [f[0] for f in flags]))
        return problems
    for (comment, member), (_, name, scope) in zip(members, flags):
        want = "%s (%s)" % (name, scope)
        if not comment.startswith(want):
            problems.append("%s: comment should start with %r, found %r" % (member, want, comment))
    return problems


DEF = """\
QUIRK(alpha, "a.one", sim, "")
QUIRK(beta, "b.two", view, "")
"""

HEADER = """\
struct Quirks {
    // a.one (sim): first.
    int alpha;
    // b.two (view): second.
    int beta;
};
"""


class CheckTest(unittest.TestCase):
    def test_good(self):
        self.assertEqual(check(DEF, HEADER), [])

    def test_wrong_name(self):
        self.assertTrue(check(DEF, HEADER.replace("b.two", "b.three")))

    def test_wrong_scope(self):
        self.assertTrue(check(DEF, HEADER.replace("(view)", "(sim)")))

    def test_out_of_order(self):
        swapped = "struct Quirks {\n    // b.two (view): second.\n    int beta;\n    // a.one (sim): first.\n    int alpha;\n};\n"
        self.assertTrue(check(DEF, swapped))

    def test_missing_member(self):
        self.assertTrue(check(DEF, HEADER.replace("    // b.two (view): second.\n    int beta;\n", "")))

    def test_real_files(self):
        problems = check((ROOT / "src/quirks/quirks.def").read_text(), (ROOT / "interfaces/quirks.h").read_text())
        self.assertEqual(problems, [])


if __name__ == "__main__":
    unittest.main()
