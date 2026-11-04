"""Tests for blockpointers.py, on hand-written scans, maps, layouts and rows.

    uv run tools/test_blockpointers.py
"""

import contextlib
import io
import struct
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import blockpointers as bp

SCAN = "\n".join([
    "bt-blockptrs 2",
    "commit\tabc",
    "every\t8",
    "ticks\t10",
    "holder\tholder_tag\toffset\ttarget\ttarget_tag\tticks\ttarget_offset_min\ttarget_offset_max",
    "0x00401010\t-\t0x4\t0x00401200\tNAMES\t10\t0x0\t0x0",
    "SimGlobals\t-\t0x14\t0x00401030\t-\t3\t0xa\t0x40",
    "0x00401010\t-\t0x9e\t0x00401200\tNAMES\t10\t0x0\t0x0",
    "",
])

MAP = """ Address         Publics by Value              Rva+Base               Lib:Object

 0001:00001000       ?Load@Loader@@QAEXXZ       0000000000401000     a.obj
 0001:00001000       ?Alias@@YAXXZ              0000000000401000     a.obj
 0001:00001020       _Setup@8                   0000000000401020     a.obj
 0001:00001100       ?Make@?$Vec@H@@QAEXXZ      0000000000401100     a.obj
 0001:00001150       ?catch$4@?0??Make@@4HA     0000000000401150     a.obj
 0001:00001200       ?Other@@YAXXZ              0000000000401200     a.obj
 0002:00000000       _Data                      0000000000500000     a.obj
"""

LAYOUT = """*** Dumping AST Record Layout
         0 | struct SimGlobals
         0 |   int a
         4 |   int[10] table
        44 |   struct Vec vec
        44 |     int begin
        48 |   int last
           | [sizeof=52, align=4,
           |  nvsize=52, nvalign=4]
"""


def resolved(holder, offset, target, scenario="s", ticks=1, low=0, high=0):
    return bp.Resolved(scenario, holder, offset, target, ticks, low, high)


def row(holder, target, offsets="0x0", cls="outside", note=""):
    return {"holder": holder, "offsets": offsets, "target": target, "scenarios": "1", "ticks": "1",
            "target_offsets": "0x0", "class": cls, "note": note}


class ScanTest(unittest.TestCase):
    def test_parse(self):
        scan = bp.parse_scan(SCAN, "eight")
        self.assertEqual((scan.commit, scan.ticks, scan.every, len(scan.rows)), ("abc", 10, 8, 3))
        r = scan.rows[1]
        self.assertEqual((r.holder, r.offset, r.target_min, r.target_max), ("SimGlobals", 0x14, 0xa, 0x40))
        self.assertEqual(scan.rows[0].target_tag, "NAMES")
        self.assertEqual(scan.rows[2].offset, 0x9e)

    def test_parse_rejects(self):
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-trace 3\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 2\nevery\t1\nticks\t1\na\tb\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 2\ncommit\t-\nevery\t1\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 2\ncommit\t-\nticks\t1\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 2\nevery\t0\nticks\t1\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 1\ncommit\t-\nticks\t1\n", "x")


class NamingTest(unittest.TestCase):
    def setUp(self):
        self.code = bp.parse_map(MAP)

    def test_map_names(self):
        self.assertEqual(self.code.function(0x401010), "Alias")
        self.assertEqual(self.code.function(0x401030), "Setup")
        self.assertEqual(self.code.function(0x401120), "?Make@?$Vec@H@@QAEXXZ")
        self.assertEqual(self.code.function(0x401160), "?Make@?$Vec@H@@QAEXXZ")
        self.assertEqual(self.code.function(0x401201), "Other")

    def test_dead_names_leave_the_address(self):
        dead = {"?Alias@@YAXXZ"}
        code = bp.parse_map(MAP, dead)
        self.assertEqual(code.function(0x401010), "Loader::Load")
        self.assertEqual(code.function(0x401030), "Setup")
        both = bp.parse_map(MAP, {"?Alias@@YAXXZ", "?Load@Loader@@QAEXXZ"})
        self.assertEqual(both.function(0x401010), "Alias")

    def test_instantiations_are_named_by_their_own_symbol(self):
        lines = [" 0001:00001000       ?insert@?$vector@H@std@@QAEXXZ   0000000000401000   a.obj",
                 " 0001:00001100       ?insert@?$vector@G@std@@QAEXXZ   0000000000401100   a.obj"]
        before = bp.parse_map("\n".join(lines))
        added = bp.parse_map("\n".join([" 0001:00000f00       ?insert@?$vector@F@std@@QAEXXZ   0000000000400f00   a.obj"]
                                       + lines))
        for code in (before, added):
            self.assertEqual(code.function(0x401010), "?insert@?$vector@H@std@@QAEXXZ")
            self.assertEqual(code.function(0x401110), "?insert@?$vector@G@std@@QAEXXZ")
        self.assertEqual(added.function(0x400f10), "?insert@?$vector@F@std@@QAEXXZ")
        namer = bp.Namer(added, [0x401110])
        self.assertEqual(namer.name("0x00401110", "T"), "?insert@?$vector@G@std@@QAEXXZ: T")

    def test_shared_plain_names_use_the_decorated_name(self):
        lines = [" 0001:00001000       ?Run@@YAXH@Z   0000000000401000   a.obj",
                 " 0001:00001100       ?Run@@YAXF@Z   0000000000401100   a.obj",
                 " 0001:00001200       ?Once@@YAXF@Z  0000000000401200   a.obj"]
        code = bp.parse_map("\n".join(lines))
        self.assertEqual(code.function(0x401010), "?Run@@YAXH@Z")
        self.assertEqual(code.function(0x401110), "?Run@@YAXF@Z")
        self.assertEqual(code.function(0x401210), "Once")

    def test_names_by_rank_among_the_function_sites(self):
        namer = bp.Namer(self.code, [0x401004, 0x401008, 0x40100c, 0x401030])
        self.assertEqual(namer.name("0x00401004", "-"), "Alias #1")
        self.assertEqual(namer.name("0x0040100c", "T"), "Alias: T #3")
        self.assertEqual(namer.name("0x00401030", "-"), "Setup")
        self.assertEqual(namer.name("0x00401018", "-"), "Alias @0x18")
        self.assertEqual(namer.name("0x00000000", "-"), "unknown")
        self.assertEqual(namer.name("SimGlobals", "-"), "SimGlobals")

    def test_names_do_not_depend_on_the_sites_a_scan_saw(self):
        def scan(*sites):
            lines = ["bt-blockptrs 2", "commit\tabc", "every\t1", "ticks\t1"]
            lines += ["0x%08x\t-\t0x0\t0x00401200\tNAMES\t1\t0x0\t0x0" % s for s in sites]
            return bp.parse_scan("\n".join(lines) + "\n", "s")

        namer = bp.Namer(self.code, [0x401004, 0x401008, 0x40100c])
        every = bp.resolve([scan(0x401004, 0x401008, 0x40100c)], namer, [])
        without_middle = bp.resolve([scan(0x401004, 0x40100c)], namer, [])
        self.assertEqual(without_middle[1].holder, every[2].holder)
        self.assertEqual(without_middle[0].holder, every[0].holder)
        self.assertEqual([r.holder for r in every], ["Alias #1", "Alias #2", "Alias #3"])

    def exe_code(self, pad):
        """Alias at 0x401000: `pad` nops, then three calls to 0x401200; the sites are the return addresses."""
        code, sites = b"\x90" * pad, []
        for _ in range(3):
            code += b"\xe8" + struct.pack("<i", 0x401200 - (0x401000 + len(code) + 5))
            sites.append(0x401000 + len(code))
            code += b"\x90"
        return code, sites

    def test_site_names_survive_an_edit_before_the_first_call(self):
        names = []
        for pad in (0, 1, 7):
            code, sites = self.exe_code(pad)
            found = bp.call_sites(code, 0x401000, {0x401200})
            self.assertEqual(sorted(found), sites)
            namer = bp.Namer(self.code, sorted(found))
            names.append([namer.name("0x%08x" % s, "-") for s in sites])
        self.assertEqual(names, [["Alias #1", "Alias #2", "Alias #3"]] * 3)

    def test_only_calls_to_allocators_are_sites(self):
        code = b"\xe8" + struct.pack("<i", 0x401100 - 0x401005) + b"\xe9" + struct.pack("<i", 0x401200 - 0x40100a)
        self.assertEqual(bp.call_sites(code, 0x401000, {0x401200}), set())

    def test_allocators_come_from_the_map(self):
        text = (" 0001:00000100  ?SimAllocTagged@@YAPAXPBDI@Z  0000000000401100  a.obj\n"
                " 0001:00000200  ??2@YAPAXI@Z  0000000000401200  a.obj\n"
                " 0001:00000300  ?Other@@YAXXZ  0000000000401300  a.obj\n")
        self.assertEqual(bp.allocator_addresses(text), {0x401100, 0x401200})

    def test_resolve_simglobals(self):
        members = bp.simglobals_members(LAYOUT)
        self.assertEqual([(m.name, m.start, m.end) for m in members],
                         [("a", 0, 4), ("table", 4, 44), ("vec", 44, 48), ("last", 48, 52)])
        out = bp.resolve([bp.parse_scan(SCAN, "s")], bp.Namer(self.code, []), members)
        self.assertEqual((out[1].holder, out[1].offset), ("SimGlobals.table", 0x10))
        out = bp.resolve([bp.parse_scan(SCAN, "s")], bp.Namer(self.code, []), None)
        self.assertEqual((out[1].holder, out[1].offset), ("SimGlobals+0x14", 0))


class LinkedObjectsTest(unittest.TestCase):
    class Obj:
        def __init__(self, dead):
            sym = type("Sym", (), {"is_func": True, "section": 1})
            self.externals = {name: sym for name in dead}
            self.secs = [type("Sec", (), {"data": b""})]

    def fake_parse(self, path):
        return self.Obj(["?Dead@@YAXXZ"] if Path(path).name == "stale.obj" else ["?Gone@@YAXXZ"])

    def test_only_listed_objects_count(self):
        with tempfile.TemporaryDirectory() as tmp:
            objs = Path(tmp) / "objs"
            objs.mkdir()
            (objs / "0000_a.obj").write_bytes(b"")
            (objs / "stale.obj").write_bytes(b"")
            (objs / "objects.rsp").write_text('"%s"\n0000_a.obj\n\n' % (objs / "0000_a.obj"))
            with mock.patch("objread.parse", self.fake_parse):
                self.assertEqual(bp.empty_functions(objs), {"?Gone@@YAXXZ"})

    def test_missing_list_is_an_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(ValueError) as cm:
                bp.empty_functions(Path(tmp) / "objs")
            self.assertIn("no objs/objects.rsp beside", str(cm.exception))
            self.assertIn("block pointer holders need the linked objects", str(cm.exception))

    def test_check_scans_reports_the_missing_list(self):
        with tempfile.TemporaryDirectory() as tmp:
            map_path = Path(tmp) / "TotalA.map"
            map_path.write_text(MAP)
            problems = bp.check_scans({"eight": SCAN}, map_path, bp.ROOT, [])
            self.assertEqual(len(problems), 1)
            self.assertIn("no objs/objects.rsp beside", problems[0])


class FoldTest(unittest.TestCase):
    def test_progression(self):
        self.assertEqual(bp.offsets_text({0x10, 0x24, 0x38, 0x4c}), "0x10+0x14*k")

    def test_two_are_listed(self):
        self.assertEqual(bp.offsets_text({0x24, 0x10}), "0x10 0x24")

    def test_run_and_rest(self):
        self.assertEqual(bp.offsets_text({0, 8, 16, 100}), "0x0+0x8*k 0x64")

    def test_longest_wins(self):
        self.assertEqual(bp.fold({0, 4, 8, 12, 16}), [(0, 4)])

    def test_merge_groups(self):
        rows = [resolved("H", 0, "T", "a", 5, 0, 0), resolved("H", 4, "T", "a", 7, 8, 0x10),
                resolved("H", 8, "T", "b", 2, 0, 0), resolved("A", 0, "T", "a", 1)]
        merged = bp.merge(rows)
        self.assertEqual([(r["holder"], r["target"]) for r in merged], [("A", "T"), ("H", "T")])
        h = merged[1]
        self.assertEqual((h["offsets"], h["scenarios"], h["ticks"], h["target_offsets"]),
                         ("0x0+0x4*k", "2", "9", "0x0-0x10"))
        self.assertEqual(merged[0]["target_offsets"], "0x0")


class KeptTest(unittest.TestCase):
    def test_merge_kept(self):
        old = [row("H", "T", cls="moves-in", note="n")]
        new = bp.merge_kept([row("H", "T", cls="", note=""), row("H", "U", "0x40", cls="", note="")], old)
        self.assertEqual([(r["class"], r["note"]) for r in new], [("moves-in", "n"), ("", "")])

    def test_merge_kept_by_offsets_when_target_renamed(self):
        old = [row("H", "Old", "0x4", cls="not-a-pointer", note="n"), row("H", "A", "0x8", cls="outside", note="a"),
               row("H", "B", "0x8", cls="moves-in", note="b")]
        new = bp.merge_kept([row("H", "New", "0x4+0x2*k", cls="", note=""), row("H", "C", "0x8", cls="", note=""),
                             row("G", "New", "0x4", cls="", note="")], old)
        self.assertEqual([(r["class"], r["note"]) for r in new], [("not-a-pointer", "n"), ("", ""), ("", "")])

    def test_merge_kept_prefers_identical_offsets(self):
        old = [row("H", "A", "0x4 0x8", cls="outside", note="a"), row("H", "B", "0x8", cls="moves-in", note="b")]
        new = bp.merge_kept([row("H", "New", "0x4 0x8", cls="", note="")], old)
        self.assertEqual([(r["class"], r["note"]) for r in new], [("outside", "a")])

    def test_merge_kept_disagreeing_identical_offsets_carry_nothing(self):
        old = [row("H", "A", "0x4 0x8", cls="outside", note="a"), row("H", "B", "0x4 0x8", cls="moves-in", note="b"),
               row("H", "C", "0x4", cls="outside", note="a")]
        new = bp.merge_kept([row("H", "New", "0x4 0x8", cls="", note="")], old)
        self.assertEqual([(r["class"], r["note"]) for r in new], [("", "")])

    def test_differences_ignore_kept(self):
        a = [row("H", "T", cls="outside")]
        b = [row("H", "T", cls="")]
        self.assertEqual(bp.differences(a, b), [])


class CheckTest(unittest.TestCase):
    def test_valid(self):
        text = bp.render([wild("A"), wild("A", "outside", ""), row("A", "T", "0x0+0x4*k 0x1"), wild("B"),
                          row("B", "T", cls="not-a-pointer", note="an id")])
        self.assertEqual(bp.check_rows(text), [])

    def test_header_only(self):
        self.assertEqual(bp.check_rows(bp.render([])), [])

    def test_problems(self):
        text = bp.render([row("B", "T", cls=""), row("A", "T", cls="not-a-pointer"), row("A", "T", "zz"),
                          row("C", "T", cls="bogus")])
        problems = "\n".join(bp.check_rows(text))
        self.assertIn("class '' is not one of", problems)
        self.assertIn("not-a-pointer row needs a note", problems)
        self.assertIn("offsets 'zz'", problems)
        self.assertIn("sorted", problems)
        self.assertIn("class 'bogus'", problems)

    def test_header(self):
        self.assertTrue(bp.check_rows("a,b\n"))


def wild(holder, cls="not-a-pointer", note="words that only look like addresses"):
    return {"holder": holder, "offsets": "*", "target": "*", "scenarios": "0", "ticks": "0",
            "target_offsets": "-", "class": cls, "note": note}


class MatcherTest(unittest.TestCase):
    ROWS = [wild("H"), row("H", "T", "0x4+0x10*k 0x100"), wild("S.f", "outside", "n"), wild("S.f"),
            row("S.f", "U", "0x0"), row("P", "T")]

    def test_holder_with_wild_row_covers_any_word(self):
        scanned = [resolved("H", 0x4, "T"), resolved("H", 0x8, "T"), resolved("H", 0x4, "V"),
                   resolved("S.f", 0x99, "anything")]
        self.assertEqual(bp.uncovered(scanned, self.ROWS), [])

    def test_unknown_holder_and_holder_without_wild_row(self):
        scanned = [resolved("X", 0, "T"), resolved("P", 0, "T"), resolved("X", 0, "T"), resolved("H", 1, "T")]
        missing = bp.uncovered(scanned, self.ROWS)
        self.assertEqual([(m.holder, m.offset, m.target) for m in missing], [("X", 0, "T"), ("P", 0, "T")])
        self.assertIn("holder not listed", bp.describe_uncovered(missing)[0])

    def test_block_vector_holders_are_never_pointer_holders(self):
        losstep = "?Insert@?$BlockVector@ULosStep@@@@QAEXPAULosStep@@IABU2@@Z"
        scanned = [resolved(losstep, 0xb, "?_Insert@?$_Tree@HU?$pair@$$CBHH@std@@@std@@"), resolved(losstep, 0xd, "T"),
                   resolved("??0?$BlockVector@ULosStep@@@@QAE@ABV0@@Z", 0x1d, "T"),
                   resolved("??4?$BlockVector@VLosLine@@@@QAEAAV0@ABV0@@Z", 0x4, "T")]
        self.assertEqual(bp.uncovered(scanned, self.ROWS), [])

    def test_only_block_vector_member_functions_are_data_holders(self):
        for name in ("?Insert@?$BlockVector@ULosStep@@@@QAEXPAULosStep@@IABU2@@Z", "??0?$BlockVector@VLosLine@@@@QAE@ABV0@@Z",
                     "?Reserve@?$BlockVector@UOrderTableEntry@@@@QAEXI@Z"):
            self.assertTrue(bp.is_data_holder(name), name)
        for name in ("AddOrder", "AddOrder: T #2", "?Use@@YAXAAV?$BlockVector@ULosStep@@@@@Z", "?Insert@?$vector@H@std@@QAEXXZ",
                     "SimGlobals.g_table", "unknown"):
            self.assertFalse(bp.is_data_holder(name), name)
        self.assertEqual([m.holder for m in bp.uncovered([resolved("AddOrder", 0, "T")], self.ROWS)], ["AddOrder"])

    def test_a_scan_that_flagged_a_los_step_vector_is_clean(self):
        map_text = MAP + " 0001:00001300       ?Insert@?$BlockVector@ULosStep@@@@QAEXPAULosStep@@IABU2@@Z   0000000000401300     a.obj\n"
        scan = SCAN + "0x00401304\t-\t0xd\t0x00401200\tNAMES\t10\t0x0\t0x0\n"
        namer = bp.Namer(bp.parse_map(map_text), [0x401304])
        found = bp.resolve([bp.parse_scan(scan, "ui-keys")], namer, [])
        stale = [r for r in found if r.holder.startswith("?Insert@?$BlockVector@ULosStep")]
        self.assertEqual([r.offset for r in stale], [0xd])
        self.assertEqual(bp.uncovered(stale, []), [])

    def test_check_scans_missing_file(self):
        problems = bp.check_scans({"eight": None}, bp.ROOT / "none.map", bp.ROOT, [])
        self.assertIn("eight: no blockptrs.txt", problems[0])


class WildcardTest(unittest.TestCase):
    WORDS = [resolved("G", 0, "T1", "a", 5, 1, 2), resolved("G", 4, "T2", "b", 7, 3, 9),
             resolved("H", 0x10, "T1")]

    def test_wild_rows_are_kept_and_pair_rows_built(self):
        old = [wild("G", "outside", "n"), wild("G")]
        out = bp.with_wild(bp.merge(self.WORDS), old)
        self.assertEqual([(r["holder"], r["target"], r["class"]) for r in out],
                         [("G", "*", "not-a-pointer"), ("G", "*", "outside"), ("G", "T1", ""), ("G", "T2", ""),
                          ("H", "T1", "")])
        self.assertEqual(out[0]["scenarios"], "0")
        self.assertEqual(out[2]["scenarios"], "1")

    def test_key_is_holder_and_class(self):
        old = [wild("G", "outside", "pairs"), wild("G", "not-a-pointer", "ids"), row("G", "T1", cls="moves-in", note="m")]
        rows = bp.merge_kept(bp.with_wild(bp.merge(self.WORDS), old[:2]), old)
        self.assertEqual([(r["target"], r["class"], r["note"]) for r in rows[:3]],
                         [("*", "not-a-pointer", "ids"), ("*", "outside", "pairs"), ("T1", "moves-in", "m")])
        self.assertEqual(bp.with_wild(bp.merge(self.WORDS), rows)[:2], rows[:2])

    def test_prune_keeps_wild_rows_of_exe_sites_only(self):
        namer = bp.Namer(bp.parse_map(MAP), [0x401004, 0x401008, 0x401030])
        for holder, kept in [("Alias #1", True), ("Alias: T #2", True), ("Alias #3", False), ("Alias", False),
                             ("Setup", True), ("Setup: T", True), ("Setup #1", False), ("Gone", False),
                             ("Gone #1", False), ("unknown", True), ("SimGlobals.a", True), ("SimGlobals", True),
                             ("SimGlobals+0x4", True)]:
            self.assertEqual(bp.is_site_holder(namer, holder), kept, holder)

    def test_prune_drops_unscanned_pairs_and_keeps_notes(self):
        namer = bp.Namer(bp.parse_map(MAP), [0x401030])
        old = [wild("Setup", "outside", "n"), wild("Gone", "outside", "stale"), row("Setup", "T1", cls="moves-in", note="m"),
               row("Setup", "T9", note="stale")]
        words = [resolved("Setup", 0, "T1")]
        rows = bp.merge_kept(bp.with_wild(bp.merge(words), bp.prune_wild(old, namer)), old)
        self.assertEqual([(r["holder"], r["target"], r["class"], r["note"]) for r in rows],
                         [("Setup", "*", "outside", "n"), ("Setup", "T1", "moves-in", "m")])

    def test_order_by_class(self):
        self.assertEqual(bp.check_rows(bp.render([wild("A", "moves-in", ""), wild("A"), wild("A", "outside", "")])), [])
        self.assertIn("sorted", "\n".join(bp.check_rows(bp.render([wild("A"), wild("A", "moves-in", "")]))))
        self.assertIn("without repeats", "\n".join(bp.check_rows(bp.render([wild("A"), wild("A")]))))

    def test_check_wants_a_row_per_class(self):
        rows = [wild("A"), wild("A", "outside", ""), row("A", "T", cls="outside"), row("A", "U", cls="moves-in"),
                row("B", "T", cls="not-a-pointer", note="n")]
        problems = "\n".join(bp.check_rows(bp.render(rows)))
        self.assertIn("A: no `*` row of class moves-in", problems)
        self.assertIn("B: no `*` row of class not-a-pointer", problems)
        self.assertNotIn("A: no `*` row of class outside", problems)
        self.assertNotIn("A: no `*` row of class not-a-pointer", problems)
        self.assertEqual(bp.check_rows(bp.render(
            [wild("A", "moves-in", ""), wild("A"), wild("A", "outside", ""), row("A", "T", cls="outside"),
             row("A", "U", cls="moves-in")])), [])

    def test_check_rejects_half_wildcards(self):
        rows = [wild("A"), dict(row("A", "*", "0x4"), target_offsets="-"), dict(row("A", "T", "*"), target_offsets="0x0"),
                row("C*", "T")]
        problems = "\n".join(bp.check_rows(bp.render(rows)))
        self.assertIn("together", problems)
        self.assertIn("(C* -> T): `*` is only", problems)

    def test_check_dash_only_on_wildcard(self):
        plain = dict(row("A", "T"), target_offsets="-")
        self.assertIn("target_offsets", "\n".join(bp.check_rows(bp.render([wild("A", "outside", ""), plain]))))

    def test_wild_note_needed(self):
        self.assertIn("needs a note", "\n".join(bp.check_rows(bp.render([wild("A", note="")]))))


class SaveScansTest(unittest.TestCase):
    def test_saved_files_read_back(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / "nested" / "scans"
            bp.save_scans({"b": SCAN, "a": SCAN, "none": None}, target)
            self.assertEqual(sorted(p.name for p in target.iterdir()), ["a.txt", "b.txt"])
            self.assertEqual([s.scenario for s in bp.read_scans([target])], ["a", "b"])


class PlaySuiteTest(unittest.TestCase):
    SCENARIOS = {"a": Path("a.txt"), "b": Path("b.txt"), "c": Path("c.txt")}

    def play(self, names, fail=()):
        import reference
        calls = []

        def fake_play_all(exe, scenarios, game_dir, parallel, failed=None, scans=None, retry_any=False, **kw):
            calls.append((sorted(scenarios), retry_any))
            for n in scenarios:
                if n in fail:
                    failed[n] = "boom"
                else:
                    scans[n] = SCAN

        failed = {}
        with mock.patch.object(reference, "play_all", fake_play_all), \
                mock.patch.object(reference, "scenario_files", lambda: self.SCENARIOS):
            texts = bp.play_suite(Path("x.exe"), None, 1, names, failed)
        return texts, failed, calls

    def test_finished_scans_are_kept_and_failures_listed(self):
        texts, failed, calls = self.play(None, fail=("b",))
        self.assertEqual(sorted(texts), ["a", "c"])
        self.assertEqual(failed, {"b": "boom"})
        self.assertEqual(calls, [(["a", "b", "c"], True)])

    def test_scenario_subset(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.SCENARIOS = {n: Path(tmp) / f"{n}.txt" for n in "abc"}
            for path in self.SCENARIOS.values():
                path.write_text("ticks 5\n")
            texts, _, calls = self.play(["c", "a"])
        self.assertEqual(sorted(texts), ["a", "c"])
        self.assertEqual(calls[0][0], ["a", "c"])

    def test_a_loader_is_played_with_its_saver(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.SCENARIOS = {n: Path(tmp) / f"{n}.txt" for n in "abc"}
            self.SCENARIOS["a"].write_text("ticks 5\n")
            self.SCENARIOS["b"].write_text("ticks 5\nsave 2\n")
            self.SCENARIOS["c"].write_text("ticks 5\nload b\n")
            texts, _, calls = self.play(["c"])
        self.assertEqual(calls[0][0], ["b", "c"])

    def test_unknown_scenario_exits(self):
        with self.assertRaises(SystemExit):
            self.play(["nope"])

    def test_scenario_without_save_scans_is_refused(self):
        with mock.patch("sys.argv", ["blockpointers.py", "--scenario", "a"]), \
                contextlib.redirect_stderr(io.StringIO()) as err, self.assertRaises(SystemExit) as cm:
            bp.main()
        self.assertEqual(cm.exception.code, 2)
        self.assertIn("--save-scans", err.getvalue())


if __name__ == "__main__":
    unittest.main()
