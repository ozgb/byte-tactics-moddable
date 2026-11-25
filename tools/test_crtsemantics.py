"""Tests for tools/crtsemantics.py, on fixture files.

    python3 tools/test_crtsemantics.py
"""

import tempfile
import unittest
from pathlib import Path

import crtsemantics

CALLS = (
    "address,function,callee,kind,file,class,notes\n"
    "0x401000,F,atoi,call,a.cpp,game,sites=1\n"
    "0x401000,F,sprintf,call,a.cpp,game,sites=1\n"
    "0x401000,F,strlen,call,a.cpp,game,sites=1\n"
    "0x402000,G,atoi,call,a.cpp,presentation,sites=1\n"
    "0x403000,H,strtok,call,a.cpp,game,sites=1\n"
)

CLEAN = (
    "address,callee,reaches,inputs,verdict\n"
    "0x401000,atoi,state,sign;whitespace,spec\n"
    + "0x401000,sprintf,trace,fmt:%04x;fmt:%s,spec\n"
    "0x403000,strtok,none,tokenizer-state,drop: debug only\n"
)


class Fixture(unittest.TestCase):
    def run_check(self, text, calls=CALLS):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "sem.csv").write_text(text, newline="")
            (d / "calls.csv").write_text(calls)
            return crtsemantics.check(d / "sem.csv", d / "calls.csv")

    def problems(self, text):
        return self.run_check(text)[0]

    def swap(self, old, new):
        self.assertIn(old, CLEAN)
        return CLEAN.replace(old, new)

    def has(self, text, needle):
        found = self.problems(text)
        self.assertTrue(any(needle in p for p in found), found)


class Check(Fixture):
    def test_clean_file_passes(self):
        problems, n, spec, drop = self.run_check(CLEAN)
        self.assertEqual((problems, n, spec, drop), ([], 3, 2, 1))

    def test_missing_row(self):
        text = CLEAN.replace("0x403000,strtok,none,tokenizer-state,drop: debug only\n", "")
        self.assertEqual(self.problems(text), ["missing: 0x403000 strtok (H)"])

    def test_stale_row(self):
        self.has(CLEAN + "0x404000,atoi,state,sign,spec\n", "stale: 0x404000 atoi")

    def test_stale_presentation_row(self):
        self.has(CLEAN.replace("0x403000", "0x402000", 1), "stale: 0x402000 strtok")

    def test_duplicate(self):
        row = "0x403000,strtok,none,tokenizer-state,drop: debug only\n"
        self.has(CLEAN + row, "duplicate: 0x403000 strtok")

    def test_header(self):
        self.has(CLEAN.replace("verdict", "result", 1), "header")

    def test_bad_reaches(self):
        self.has(self.swap(",state,", ",memory,"), "reaches 'memory'")

    def test_unknown_tag(self):
        self.has(self.swap("sign;whitespace", "sign;spaces"), "unknown tag 'spaces'")

    def test_unsorted_tags(self):
        self.has(self.swap("sign;whitespace", "whitespace;sign"), "not sorted")

    def test_duplicate_tags(self):
        self.has(self.swap("sign;whitespace", "sign;sign"), "not sorted")

    def test_fmt_on_non_printf_row(self):
        self.has(self.swap("tokenizer-state", "fmt:%d"), "fmt tag fmt:%d on strtok")

    def test_malformed_conversion(self):
        self.has(self.swap("fmt:%04x", "fmt:%04q"), "malformed conversion fmt:%04q")

    def test_spec_with_reaches_none(self):
        self.has(self.swap("none,tokenizer-state,drop: debug only", "none,tokenizer-state,spec"), "spec with reaches none")

    def test_spec_with_empty_inputs(self):
        self.has(self.swap("state,sign;whitespace,spec", "state,,spec"), "spec with empty inputs")

    def test_drop_without_reason(self):
        self.has(self.swap("drop: debug only", "drop:"), "drop with no reason")

    def test_drop_that_should_be_spec(self):
        self.has(self.swap("state,sign;whitespace,spec", "state,sign,drop: unused"), "drop with reaches state")

    def test_unsorted_rows(self):
        lines = CLEAN.splitlines(keepends=True)
        self.has("".join([lines[0], lines[3], lines[1], lines[2]]), "not sorted by address")

    def test_crlf(self):
        self.has(CLEAN.replace("\n", "\r\n"), "line ends")


class Conversions(unittest.TestCase):
    def test_accepted(self):
        for c in ("%d", "%04x", "%-10s", "%.1f", "%3d", "%%", "%[a-zA-Z0-9_.]", "%*d", "%*[^\n]", "%ld", "%*s", "%[^]]"):
            self.assertEqual(crtsemantics.tag_problem("fmt:" + c, "sscanf"), "", c)

    def test_rejected(self):
        for c in ("d", "%", "%q", "%04", "%[abc", "%ll d"):
            self.assertNotEqual(crtsemantics.tag_problem("fmt:" + c, "sscanf"), "", c)


class Summary(Fixture):
    def test_summary(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "sem.csv"
            p.write_text(
                CLEAN
                + "0x405000,atoi,save,empty,spec\n"
                + "0x406000,atoi,none,empty,drop: unused\n"
            )
            self.assertEqual(
                crtsemantics.summary(p),
                [
                    "atoi: spec 2, drop 1, inputs empty;sign;whitespace",
                    "sprintf: spec 1, drop 0, inputs fmt:%04x;fmt:%s",
                    "strtok: spec 0, drop 1, inputs ",
                ],
            )


if __name__ == "__main__":
    unittest.main()
