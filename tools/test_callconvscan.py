"""Tests for callconvscan.py, on hand-written prologues and declarations.

    uv run tools/test_callconvscan.py
"""

import tempfile
import unittest
from pathlib import Path

import capstone

import callconvscan as cs


def decode(code: bytes):
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    return list(md.disasm(code, 0x401000))


class ReadsEcxFirst(unittest.TestCase):
    def test_this_use(self):
        self.assertTrue(cs.reads_ecx_first(decode(bytes.fromhex("8b01c3"))))

    def test_ecx_set_first(self):
        self.assertFalse(cs.reads_ecx_first(decode(bytes.fromhex("b90500000089c8"))))

    def test_or_minus_one_is_a_write(self):
        self.assertFalse(cs.reads_ecx_first(decode(bytes.fromhex("83c9ff89c8"))))

    def test_push_ecx_reserves_a_slot(self):
        self.assertFalse(cs.reads_ecx_first(decode(bytes.fromhex("5133c9"))))

    def test_edx(self):
        self.assertTrue(cs.reads_ecx_first(decode(bytes.fromhex("89d0c3")), cs.EDX))


class Patterns(unittest.TestCase):
    def test_member_decoration(self):
        self.assertTrue(cs.MEMBER_DECORATION.match("?Close@MoviePlayer@@QAEXXZ"))
        self.assertTrue(cs.MEMBER_DECORATION.match("?F@A@B@@UAEHXZ"))
        self.assertFalse(cs.MEMBER_DECORATION.match("?DrawString@@YGXPAUSurface@@PAEHHH@Z"))
        self.assertFalse(cs.MEMBER_DECORATION.match("?Make@Foo@@SGXXZ"))

    def test_declaration(self):
        self.assertEqual(cs.DECL.match("int __stdcall Foo(void* p);").group(2), "Foo")
        self.assertEqual(cs.DECL.match("    void __fastcall Bar(int);").group(2), "Bar")
        self.assertIsNone(cs.DECL.match("typedef int (__stdcall *Fn)(int);"))
        self.assertIsNone(cs.DECL.match("int (__stdcall *g_fn)(int);"))


class Markers(unittest.TestCase):
    def declared(self, text: str):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "a.cpp").write_text(text)
            (Path(tmp) / "include").mkdir()
            return list(cs.declarations(Path(tmp)))

    def test_marker_binds_to_the_definition_that_follows(self):
        hits = self.declared("// FUNCTION: 0x40c510\nint __stdcall Foo(int a) { return a; }\n")
        self.assertEqual([h[3:] for h in hits], [("Foo", 0x40C510)])

    def test_marker_over_comment_lines_still_binds(self):
        hits = self.declared("// FUNCTION: 0x40c510\n// TODO(#1): note\nint __stdcall Foo(int a) { return a; }\n")
        self.assertEqual([h[3:] for h in hits], [("Foo", 0x40C510)])

    def test_marker_does_not_reach_an_unrelated_function_after_a_blank_line(self):
        hits = self.declared("// FUNCTION: 0x40c510\n\ninline void __stdcall Copy(int* d) { *d = 0; }\n")
        self.assertEqual([h[3:] for h in hits], [("Copy", None)])

    def test_marker_does_not_reach_past_code(self):
        hits = self.declared("// FUNCTION: 0x40c510\nint x;\nint __stdcall Foo(int a) { return a; }\n")
        self.assertEqual([h[3:] for h in hits], [("Foo", None)])


if __name__ == "__main__":
    unittest.main()
