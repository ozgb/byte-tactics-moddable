"""Tests for astcache.py: the key, the cache files and the off switch.

    uv run tools/test_astcache.py
"""

import os
import shutil
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import astcache


class AstCacheTest(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp, True)
        (self.tmp / "inc").mkdir()
        self.header = self.tmp / "inc" / "a.h"
        self.header.write_text("struct A { int x; };\n")
        self.source = self.tmp / "unit.cpp"
        self.source.write_text('#include "a.h"\nA g_a;\n')
        self.argv = ["clang-cl", "--target=i686-pc-windows-msvc", "/nologo", "-TP", "-I" + str(self.tmp / "inc"),
                     "-fsyntax-only", "--", str(self.source)]
        self.env = mock.patch.dict(os.environ, {astcache.ENV_DIR: str(self.tmp / "cache")})
        self.env.start()
        self.addCleanup(self.env.stop)
        os.environ.pop(astcache.ENV_OFF, None)
        astcache._digests.clear()

    def key(self) -> bytes:
        astcache._digests.clear()
        return astcache.key_of(astcache.canonical(self.argv), str(self.tmp))

    def test_canonical_is_one_command_line(self):
        plain = astcache.canonical(self.argv)
        layouts = self.argv[:-2] + astcache.AST_FLAG + astcache.LAYOUT_FLAG + astcache.WARNING_FLAGS[:1] + self.argv[-2:]
        self.assertEqual(astcache.canonical(layouts), plain)
        self.assertEqual(astcache.canonical(plain), plain)
        self.assertEqual(plain[-2:], self.argv[-2:])
        for flag in (astcache.LAYOUT_FLAG[1], astcache.AST_FLAG[1]) + tuple(astcache.WARNING_FLAGS):
            self.assertEqual(plain.count(flag), 1)

    def test_key_follows_an_included_header(self):
        before = self.key()
        self.assertEqual(self.key(), before)
        self.header.write_text("struct A { int x; int y; };\n")
        self.assertNotEqual(self.key(), before)

    def test_key_follows_a_header_added_to_an_include_directory(self):
        before = self.key()
        (self.tmp / "inc" / "b.h").write_text("\n")
        self.assertNotEqual(self.key(), before)

    def test_key_follows_the_source_and_the_command_line(self):
        before = self.key()
        self.source.write_text('#include "a.h"\nA g_b;\n')
        self.assertNotEqual(self.key(), before)
        other = self.argv[:-2] + ["-DX=1"] + self.argv[-2:]
        self.assertNotEqual(astcache.key_of(astcache.canonical(other), str(self.tmp)), self.key())

    def test_fill_hit_and_invalidation(self):
        first, filled = astcache.fetch(self.argv, str(self.tmp))
        self.assertIsNotNone(first)
        self.assertTrue(filled)
        with mock.patch.object(astcache.subprocess, "run", side_effect=AssertionError("clang ran")):
            astcache._digests.clear()
            second, filled = astcache.fetch(self.argv, str(self.tmp))
        self.assertFalse(filled)
        self.assertEqual(second, first)
        self.header.write_text("struct A { int x; int y; };\n")
        astcache._digests.clear()
        self.assertTrue(astcache.fetch(self.argv, str(self.tmp))[1])

    def test_a_damaged_file_is_a_miss(self):
        astcache.fetch(self.argv, str(self.tmp))
        (path,) = (self.tmp / "cache").glob("*.pickle")
        path.write_bytes(path.read_bytes()[:40])
        astcache._digests.clear()
        self.assertTrue(astcache.fetch(self.argv, str(self.tmp))[1])

    def test_off_switch(self):
        with mock.patch.dict(os.environ, {astcache.ENV_OFF: "1"}):
            with mock.patch.object(astcache.subprocess, "run", side_effect=AssertionError("clang ran")):
                self.assertIsNone(astcache.unit(self.argv, str(self.tmp)))
            self.assertFalse(astcache.enabled())
        self.assertFalse((self.tmp / "cache").exists())

    def test_not_configured_is_off(self):
        del os.environ[astcache.ENV_DIR]
        self.assertIsNone(astcache.cache_dir())
        self.assertIsNone(astcache.unit(self.argv, str(self.tmp)))

    def test_a_clang_error_is_left_to_the_caller(self):
        self.source.write_text("int f( { \n")
        self.assertIsNone(astcache.unit(self.argv, str(self.tmp)))
        self.assertFalse(list((self.tmp / "cache").glob("*.pickle")) if (self.tmp / "cache").exists() else [])


if __name__ == "__main__":
    unittest.main()
