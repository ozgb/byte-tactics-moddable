"""Tests for the dropped initialiser check in aliases.py, on synthetic objects.

    uv run tools/test_aliases.py
"""

import contextlib
import io
import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import aliases
from aliases import Section, Symbol

CODE = 0x60500020


def object_bytes(initialiser: str | None) -> bytes:
    secs = [Section(".text", CODE, b"\xc3", 1)]
    syms = [Symbol("_f", 0, 1, 0x20, aliases.IMAGE_SYM_CLASS_EXTERNAL)]
    if initialiser is not None:
        secs.append(Section(".CRT$XCU", 0xC0300040, b"\0\0\0\0", 4, [[0, 1, 0x06]]))
        syms.append(Symbol(initialiser, 0, 1, 0x20, aliases.IMAGE_SYM_CLASS_EXTERNAL))
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "o.obj"
        aliases.write(0, secs, syms, path)
        return path.read_bytes()


def sections_kept(data: bytes) -> list[str]:
    _, secs, _ = aliases.read(data)
    return [s.name for s in secs if not s.chars & aliases.IMAGE_SCN_LNK_REMOVE]


class InitialiserSymbolsTest(unittest.TestCase):
    def test_names_the_target(self):
        self.assertEqual(aliases.initialiser_symbols(object_bytes("??__Eg_x@@YAXXZ")), ["??__Eg_x@@YAXXZ"])

    def test_none_without_section(self):
        self.assertEqual(aliases.initialiser_symbols(object_bytes(None)), [])

    def test_drop_removes_section(self):
        data = object_bytes("_init")
        self.assertEqual(sections_kept(data), [".text", ".CRT$XCU"])
        self.assertEqual(sections_kept(aliases.drop_initialisers(data)), [".text"])


class NoOriginalTest(unittest.TestCase):
    def patch(self, no_orig: bool) -> list[str]:
        missing = Path(tempfile.gettempdir()) / "no-such-dir" / "TotalA.exe"
        env = {"BT_NO_ORIG": "1"} if no_orig else {}
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(aliases, "EXE", missing), \
                mock.patch.dict("os.environ", env), contextlib.redirect_stderr(io.StringIO()):
            if not no_orig:
                os.environ.pop("BT_NO_ORIG", None)
            listed, progress = Path(tmp) / "initialisers.csv", Path(tmp) / "progress.csv"
            listed.write_text("address,name\n0x10,_$E1\n")
            progress.write_text("address,size,file,symbol,status,similarity\n0x10,1,tools/aliases.py,_$E1,matched,100\n")
            src = Path(tmp) / "o.obj"
            src.write_bytes(object_bytes("??__Eg_x@@YAXXZ"))
            with mock.patch.object(aliases, "INITIALISERS", listed), mock.patch.object(aliases, "PROGRESS", progress):
                out = aliases.patch_objects([src], {}, Path(tmp) / "out", lambda _: Path(aliases.__file__), True)
            return sections_kept(next(p for p in out if p.name.endswith("o.obj")).read_bytes())

    def test_reads_the_list_without_opening_the_exe(self):
        self.assertEqual(self.patch(True), [".text", ".CRT$XCU"])

    def test_default_needs_the_exe(self):
        with self.assertRaises(FileNotFoundError):
            self.patch(False)


class InitialisersTest(unittest.TestCase):
    def listed(self, text: str, no_orig: bool, exe: list[int]) -> list[int]:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "initialisers.csv"
            path.write_text(text)
            env = {"BT_NO_ORIG": "1"} if no_orig else {}
            with mock.patch.object(aliases, "INITIALISERS", path), mock.patch.dict("os.environ", env), \
                    mock.patch.object(aliases, "original_initialisers", return_value=exe) as read:
                if not no_orig:
                    os.environ.pop("BT_NO_ORIG", None)
                found = aliases.initialisers()
                self.assertEqual(read.called, not no_orig)
                return found

    def test_no_orig_reads_the_file_only(self):
        self.assertEqual(self.listed("address,name\n0x10,a\n0x20,\n", True, [99]), [0x10, 0x20])

    def test_matching_exe_passes(self):
        self.assertEqual(self.listed("address,name\n0x10,a\n", False, [0x10]), [0x10])

    def test_differing_exe_fails(self):
        with self.assertRaises(SystemExit):
            self.listed("address,name\n0x10,a\n", False, [0x10, 0x20])


class ReportTest(unittest.TestCase):
    def fails(self, lost, allowed) -> str:
        err = io.StringIO()
        with self.assertRaises(SystemExit) as raised, contextlib.redirect_stderr(err):
            aliases.report_dropped_initialisers(lost, allowed)
        self.assertEqual(raised.exception.code, 1)
        return err.getvalue()

    def test_unlisted_drop_is_named(self):
        err = self.fails([("src/new.cpp", "_init", [])], {})
        self.assertIn("aliases: dropped dynamic initialiser _init in src/new.cpp", err)

    def test_allowed_drop_passes(self):
        aliases.report_dropped_initialisers([("src/old.cpp", "_init", ["_a", "_b"])],
                                            {("src/old.cpp", "_init"): {"_a", "_b"}})

    def test_allowance_is_per_file(self):
        self.fails([("src/new.cpp", "_init", [])], {("src/old.cpp", "_init"): set()})

    def test_extra_ref_fails_and_is_named(self):
        err = self.fails([("src/old.cpp", "_init", ["_a", "_rand"])], {("src/old.cpp", "_init"): {"_a"}})
        self.assertIn("src/old.cpp", err)
        self.assertIn("added: _rand", err)

    def test_missing_ref_fails_and_is_named(self):
        err = self.fails([("src/old.cpp", "_init", ["_a"])], {("src/old.cpp", "_init"): {"_a", "_b"}})
        self.assertIn("removed: _b", err)

    def test_no_drops_passes(self):
        aliases.report_dropped_initialisers([], {})

    def test_the_access_log_callbacks_are_not_references(self):
        lost = [("src/old.cpp", "_init", ["_a", "___asan_load4", "___asan_memset", "___asan_report_load1"])]
        self.fails(lost, {("src/old.cpp", "_init"): {"_a", "_memset"}})
        aliases.IGNORED_REFERENCES = ("asan_",)
        try:
            aliases.report_dropped_initialisers(lost, {("src/old.cpp", "_init"): {"_a", "_memset"}})
        finally:
            aliases.IGNORED_REFERENCES = ()

    def test_allowlist_file_parses(self):
        allowed = aliases.load_initialiser_allowlist()
        self.assertIn("_atexit", allowed[("src/game/economy.cpp", "__GLOBAL__sub_I_economy.cpp")])


class InitialiserRefsTest(unittest.TestCase):
    def test_follows_the_function_and_its_helper(self):
        secs = [Section(".text$a", CODE, b"\xc3", 1, [[0, 1, 0x06], [0, 2, 0x06]]),
                Section(".text$b", CODE, b"\xc3", 1, [[0, 3, 0x06]]),
                Section(".CRT$XCU", 0xC0300040, b"\0\0\0\0", 4, [[0, 0, 0x06]])]
        ext = aliases.IMAGE_SYM_CLASS_EXTERNAL
        syms = [Symbol("__GLOBAL__sub_I_x.cpp", 0, 1, 0x20, ext),
                Symbol("__cxx_global_var_init", 0, 2, 0x20, ext),
                Symbol("_rand", 0, 0, 0x20, ext),
                Symbol("?probe@@3_NB", 0, 0, 0, ext),
                Symbol(".text$a", 0, 1, 0, aliases.IMAGE_SYM_CLASS_STATIC)]
        secs[0].relocs.append([0, 4, 0x06])
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "o.obj"
            aliases.write(0, secs, syms, path)
            refs = aliases.initialiser_refs(path.read_bytes())
        self.assertEqual(refs, {"__GLOBAL__sub_I_x.cpp": ["?probe@@3_NB", "_rand"]})


COMDAT_CODE = CODE | aliases.IMAGE_SCN_LNK_COMDAT
DATA = 0xC0400040


def coverage_object(tmp: str) -> list[Path]:
    ext = aliases.IMAGE_SYM_CLASS_EXTERNAL
    secs = [Section(".text", CODE, b"\xc3", 1, [[0, 1, 0x06]]),
            Section(".text", COMDAT_CODE, b"\xc3", 1, [[0, 3, 0x06], [0, 4, 0x06]]),
            Section(".text", COMDAT_CODE, b"\xc3", 1, [[0, 5, 0x06]]),
            Section(".lprfc$M", DATA, b"\0" * 32, 32),
            Section(".lprfd$M", DATA, b"\0" * 4, 4, [[0, 2, 0x06]])]
    syms = [Symbol("_root", 0, 1, 0x20, ext),
            Symbol("_live", 0, 2, 0x20, ext),
            Symbol("_orphan", 0, 3, 0x20, ext),
            Symbol("___profc__live", 0, 4, 0, ext),
            Symbol("___profc__inlined", 8, 4, 0, ext),
            Symbol("___profc__orphan", 16, 4, 0, ext)]
    path = Path(tmp) / "c.obj"
    aliases.write(0, secs, syms, path)
    return [path]


class LiveCountersTest(unittest.TestCase):
    def test_counters_of_kept_and_inlined_code_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            counters = aliases.live_counters(coverage_object(tmp), {}, ())
        self.assertEqual(counters, ["_inlined", "_live"])

    def test_profile_data_is_a_root_only_when_asked(self):
        with tempfile.TemporaryDirectory() as tmp:
            objs = [aliases.read(p.read_bytes()) for p in coverage_object(tmp)]
        rooted, _ = aliases.reachable_sections(objs, {}, ())
        unrooted, _ = aliases.reachable_sections(objs, {}, (), profile_roots=False)
        self.assertIn((0, 3), rooted)
        self.assertNotIn((0, 3), unrooted)
        self.assertNotIn((0, 5), unrooted)
        self.assertIn((0, 2), unrooted)

    def test_a_named_root_keeps_its_counter(self):
        with tempfile.TemporaryDirectory() as tmp:
            counters = aliases.live_counters(coverage_object(tmp), {}, ("_orphan",))
        self.assertEqual(counters, ["_inlined", "_live", "_orphan"])


if __name__ == "__main__":
    unittest.main()
