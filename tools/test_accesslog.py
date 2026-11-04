"""Tests for accesslog.py and the classifier of lib/accesslog, on hand-written maps, layouts, rows and accesses.

    uv run tools/test_accesslog.py

The classifier (lib/accesslog/classifier.h) is plain C++: the tests compile it with the host's C++ compiler into
a small driver and run a synthetic sequence of accesses through it and through a Python reference of the same
rules, once per evidence kind and once on a long pseudo-random sequence.
"""

import csv
import random
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

import accesslog as al

ROOT = al.ROOT

MAP = """ TotalA

 Preferred load address is 0000000000400000

 Start         Length     Name                   Class
 0001:00000000 00000300H .text                   CODE
 0002:00000000 00000100H .rdata                  DATA
 0003:00000000 00000200H .data                   DATA

  Address         Publics by Value              Rva+Base               Lib:Object

 0001:00000000       ?Update@UnitTable@@QAEXXZ  0000000000401000     0001_units.cpp.obj
 0001:00000100       ?Resize@?$Ref@UGroup@@@@QAEXXZ 0000000000401100     0001_units.cpp.obj
 0001:00000140       ?SimBlockBase@@YAPAXXZ     0000000000401140     0002_block.cpp.obj
 0001:00000180       _Helper@4                  0000000000401180     0003_blit.cpp.obj
 0001:000001c0       ?Overloaded@@YAXXZ         00000000004011c0     0004_other.cpp.obj
 0001:00000200       ?Guess@@YAXXZ              0000000000401200     0005_unlisted.cpp.obj
 0002:00000010       ?s_table@@3QAHA            0000000000402010     0001_units.cpp.obj
 0003:00000000       ?g_game@@3PAUGame@@A       0000000000403000     0001_units.cpp.obj
 0003:00000010       ?g_back@?A0x31CA2109@@3PAEA 0000000000403010     0001_units.cpp.obj
 0003:00000020       ?len@?1??SelectSchema@Mission@@QAEHXZ@4HA 0000000000403020 0001_units.cpp.obj
 0003:00000030       _g_count                   0000000000403030     0001_units.cpp.obj
 0003:00000040       ?Owner@Outer@@2HA          0000000000403040     0001_units.cpp.obj
"""

SUBSYSTEMS = """function,file,subsystem,rule,override
UnitTable::Update,src/units/units.cpp,units,file:src/units/units,
Overloaded,src/units/a.cpp,units,file:src/units/a,
Overloaded,src/orders/other.cpp,orders,file:src/orders/other,
Helper,src/ui/blit.cpp,presentation,file:src/ui/blit,
Listed,src/ui/unlisted.cpp,presentation,file:src/ui/unlisted,
"""

LAYOUT = """*** Dumping AST Record Layout
         0 | struct Player
         0 |   int active
         4 |   int metal
         8 |   char[8] name
        16 |   struct Vec3 pos
        16 |     int x
        20 |     int y
        24 |   int tail
           | [sizeof=28, align=4,
           |  nvsize=28, nvalign=4]
*** Dumping AST Record Layout
         0 | struct Game
         0 |   int ticks
         4 |   Player[3] players
        88 |   int after
           | [sizeof=92, align=4,
           |  nvsize=92, nvalign=4]
*** Dumping AST Record Layout
         0 | struct Vec3
         0 |   int x
         4 |   int y
           | [sizeof=8, align=4,
           |  nvsize=8, nvalign=4]
"""

DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include "classifier.h"
static void* Alloc(unsigned n) { return calloc(n, 1); }
int main()
{
    Classifier c;
    if (!c.Init(64, 12, Alloc))
        return 2;
    char op;
    unsigned a, b, d, e, f;
    while (scanf(" %c", &op) == 1) {
        if (op == 'B') { scanf("%u", &a); c.BeginWindow(a); }
        else if (op == 'L') { scanf("%u", &a); c.ResolveLabel(a); }
        else if (op == 'E') { scanf("%u", &a); c.EndWindow(a); }
        else if (op == 'r') { scanf("%u %u %u %u %u", &a, &b, &d, &e, &f); c.Read(a, b, d, e, f); }
        else if (op == 'w') { scanf("%u %u %u %u %u", &a, &b, &d, &e, &f); c.Write(a, b, d, e, f); }
        else if (op == 'o') { scanf("%u %u %u %u", &a, &b, &d, &e); c.Ordered(a, b, d, e); }
    }
    std::vector<ClassifierEdge> edges;
    for (unsigned i = 0; i < c.Slots(); i++) {
        ClassifierEdge x;
        if (c.Edge(i, &x))
            edges.push_back(x);
    }
    std::sort(edges.begin(), edges.end(), [](const ClassifierEdge& x, const ClassifierEdge& y) {
        if (x.writer != y.writer) return x.writer < y.writer;
        if (x.reader != y.reader) return x.reader < y.reader;
        if (x.item != y.item) return x.item < y.item;
        if (x.label != y.label) return x.label < y.label;
        return x.evidence < y.evidence;
    });
    for (auto& x : edges)
        printf("%u %u %u %u %u %u %u\n", x.writer, x.reader, x.item, x.label, x.evidence, x.count, x.firstTick);
    return c.Overflowed() ? 3 : 0;
}
"""

NO_SUB = 0xFFFF
PENDING = 10
RAW, UNWRITTEN, UNREAD, ORDERED = range(4)


class ReferenceClassifier:
    """The rules of lib/accesslog/classifier.h, in Python."""

    def __init__(self):
        self.edges = {}
        self.state = {}
        self.written = []
        self.streams = {}
        self.stamp = 1
        self.label = 0

    def count(self, writer, reader, item, evidence, tick):
        key = (writer, reader, item, self.label, evidence)
        if key in self.edges and self.edges[key][0] > 0:
            self.edges[key][0] += 1
        else:
            self.edges[key] = [1, tick]

    def begin(self, label):
        self.stamp += 1
        self.label = label
        self.written = []

    def resolve(self, label):
        if self.label != PENDING:
            return
        self.label = label
        for key in [k for k in self.edges if k[3] == PENDING]:
            count, first = self.edges[key]
            if count == 0:
                continue
            self.edges[key][0] = 0
            to = key[:3] + (label, key[4])
            if to in self.edges and self.edges[to][0] > 0:
                self.edges[to][0] += count
                self.edges[to][1] = min(self.edges[to][1], first)
            else:
                self.edges[to] = [count, first]

    def end(self, tick):
        for item in self.written:
            s = self.state[item]
            if not s["read"]:
                self.count(s["writer"], NO_SUB, item, UNREAD, tick)
        self.written = []

    def read(self, item, a, b, sub, tick):
        s = self.state.setdefault(item, {"stamp": 0, "writer": 0, "ent": (0, 0), "read": 0})
        if s["stamp"] == self.stamp:
            if s["writer"] != sub or s["ent"] != (a, b):
                self.count(s["writer"], sub, item, RAW, tick)
            s["read"] = 1
        else:
            self.count(NO_SUB, sub, item, UNWRITTEN, tick)

    def write(self, item, a, b, sub, tick):
        s = self.state.setdefault(item, {"stamp": 0, "writer": 0, "ent": (0, 0), "read": 0})
        if s["stamp"] != self.stamp:
            s["stamp"] = self.stamp
            self.written.append(item)
        s["writer"], s["ent"], s["read"] = sub, (a, b), 0

    def ordered(self, stream, item, sub, tick):
        last = self.streams.get(stream)
        if last and last[0] == self.stamp and last[1] != sub:
            self.count(last[1], sub, item, ORDERED, tick)
        self.streams[stream] = (self.stamp, sub)

    def lines(self):
        rows = sorted((k, v) for k, v in self.edges.items() if v[0] > 0)
        return ["%u %u %u %u %u %u %u" % (k[0], k[1], k[2], k[3], k[4], v[0], v[1]) for k, v in rows]


def feed(script, reference):
    """Run a script (a list of tuples) through the Python reference and return the C++ driver's text for it."""
    for op, *args in script:
        {"B": reference.begin, "L": reference.resolve, "E": reference.end, "r": reference.read, "w": reference.write,
         "o": reference.ordered}[op](*args)
    return "\n".join("%s %s" % (op, " ".join(str(a) for a in args)) for op, *args in script) + "\n"


class ClassifierTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.exe = Path(cls.tmp.name) / "driver"
        source = Path(cls.tmp.name) / "driver.cpp"
        source.write_text(DRIVER)
        compiler = shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")
        if compiler is None:
            raise unittest.SkipTest("no host C++ compiler")
        subprocess.run([compiler, "-std=c++11", "-O1", "-I", str(ROOT / "lib/accesslog"), str(source), "-o", str(cls.exe)],
                       check=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def both(self, script):
        reference = ReferenceClassifier()
        text = feed(script, reference)
        run = subprocess.run([str(self.exe)], input=text, capture_output=True, text=True, check=True)
        self.assertEqual(run.stdout.splitlines(), reference.lines())
        return reference.lines()

    def test_read_after_write_by_another_subsystem(self):
        lines = self.both([("B", 1), ("w", 5, 0, 0, 1, 7), ("r", 5, 0, 0, 2, 7), ("E", 7)])
        self.assertIn("1 2 5 1 0 1 7", lines)

    def test_read_after_write_by_the_same_subsystem_on_another_entity(self):
        lines = self.both([("B", 1), ("w", 5, 9, 1, 1, 3), ("r", 5, 9, 2, 1, 3), ("r", 5, 9, 1, 1, 3), ("E", 3)])
        self.assertEqual(lines, ["1 1 5 1 0 1 3"])

    def test_the_writers_own_read_of_its_entity_is_no_edge_but_clears_write_unread(self):
        self.assertEqual(self.both([("B", 2), ("w", 4, 1, 1, 6, 9), ("r", 4, 1, 1, 6, 9), ("E", 9)]), [])

    def test_read_unwritten(self):
        lines = self.both([("B", 3), ("r", 8, 0, 0, 4, 12), ("r", 8, 0, 0, 4, 13), ("E", 13)])
        self.assertEqual(lines, ["%u 4 8 3 1 2 12" % NO_SUB])

    def test_an_earlier_window_is_not_this_one(self):
        lines = self.both([("B", 1), ("w", 2, 0, 0, 1, 1), ("E", 1), ("B", 2), ("r", 2, 0, 0, 2, 2), ("E", 2)])
        self.assertIn("%u 2 2 2 1 1 2" % NO_SUB, lines)

    def test_write_unread_counts_the_last_write_when_no_read_followed(self):
        lines = self.both([("B", 4), ("w", 3, 0, 0, 2, 5), ("r", 3, 0, 0, 3, 5), ("w", 3, 0, 0, 2, 5), ("E", 5)])
        self.assertIn("2 %u 3 4 2 1 5" % NO_SUB, lines)
        self.assertIn("2 3 3 4 0 1 5", lines)

    def test_a_write_read_after_it_is_not_unread(self):
        lines = self.both([("B", 4), ("w", 3, 0, 0, 2, 5), ("w", 3, 0, 0, 2, 5), ("r", 3, 0, 0, 3, 5), ("E", 5)])
        self.assertNotIn("2 %u 3 4 2 1 5" % NO_SUB, lines)

    def test_ordered_streams_count_changes_of_subsystem(self):
        lines = self.both([("B", 1), ("o", 0, 60, 1, 4), ("o", 0, 60, 1, 4), ("o", 0, 60, 2, 4), ("o", 0, 60, 1, 5),
                           ("o", 2, 62, 3, 5), ("o", 2, 62, 3, 5), ("E", 5)])
        self.assertEqual(lines, ["1 2 60 1 3 1 4", "2 1 60 1 3 1 5"])

    def test_the_pending_window_takes_the_label_of_the_marker(self):
        lines = self.both([("B", PENDING), ("w", 1, 0, 0, 1, 8), ("r", 1, 0, 0, 2, 8), ("L", 1), ("E", 8),
                           ("B", PENDING), ("w", 1, 0, 0, 1, 9), ("r", 1, 0, 0, 2, 9), ("L", 0), ("E", 9)])
        self.assertIn("1 2 1 1 0 1 8", lines)
        self.assertIn("1 2 1 0 0 1 9", lines)
        self.assertEqual([line for line in lines if line.split()[3] == str(PENDING)], [])

    def test_a_long_random_sequence_agrees_with_the_reference(self):
        rng = random.Random(835)
        script = []
        tick = 0
        for _ in range(300):
            tick += 1
            label = rng.choice([PENDING, 1, 2, 3, 8, 9])
            script.append(("B", label))
            for _ in range(rng.randrange(1, 60)):
                kind = rng.choice("rrrwwo")
                item, sub = rng.randrange(40), rng.randrange(5)
                if kind == "o":
                    script.append(("o", rng.randrange(4), 60 + rng.randrange(4), sub, tick))
                else:
                    script.append((kind, item, rng.randrange(3), rng.randrange(3), sub, tick))
            if label == PENDING:
                script.append(("L", rng.choice([0, 1])))
            script.append(("E", tick))
        self.both(script)


class MapTest(unittest.TestCase):
    def setUp(self):
        self.info = al.parse_map(MAP)
        self.tmp = tempfile.TemporaryDirectory()
        path = Path(self.tmp.name) / "subsystems.csv"
        path.write_text(SUBSYSTEMS)
        self.rows = al.read_subsystems(path)

    def tearDown(self):
        self.tmp.cleanup()

    def test_sections_and_symbols(self):
        self.assertEqual(self.info.sections[1], (0x300, ".text", "CODE"))
        self.assertEqual(len(self.info.code()), 6)
        self.assertEqual([n for _, n, _ in self.info.data()][:2], ["?s_table@@3QAHA", "?g_game@@3PAUGame@@A"])

    def test_object_source_drops_the_link_position(self):
        self.assertEqual(al.object_source("0121_selection.cpp.obj"), "selection.cpp")
        self.assertEqual(al.object_source("KERNEL32:KERNEL32.dll"), "KERNEL32:KERNEL32.dll")

    def test_subsystem_by_name_and_file(self):
        self.assertEqual(al.subsystem_of("?Overloaded@@YAXXZ", "0004_other.cpp.obj", self.rows), "orders")
        self.assertEqual(al.subsystem_of("?Overloaded@@YAXXZ", "0009_a.cpp.obj", self.rows), "units")
        self.assertEqual(al.subsystem_of("?Update@UnitTable@@QAEXXZ", "0001_units.cpp.obj", self.rows), "units")

    def test_subsystem_of_an_unlisted_function_is_its_files_only_subsystem_else_unknown(self):
        self.assertEqual(al.subsystem_of("?Guess@@YAXXZ", "0005_unlisted.cpp.obj", self.rows), "presentation")
        self.assertEqual(al.subsystem_of("?Guess@@YAXXZ", "0006_nowhere.cpp.obj", self.rows), al.UNKNOWN)

    def test_attribution_table_ignores_the_block_and_the_ref_helpers(self):
        subs, ranges, counts = al.attribution_table(self.info, self.rows)
        def sub_at(address):
            return next(sub for start, end, sub in ranges if start <= address < end)

        self.assertEqual(subs[sub_at(0x401000)], "units")
        self.assertEqual(sub_at(0x401100), al.IGNORED)
        self.assertEqual(sub_at(0x401140), al.IGNORED)
        self.assertEqual(subs[sub_at(0x401180)], "presentation")
        self.assertEqual(counts["ignored"], 2)
        self.assertEqual(ranges[0][0], 0x401000)
        self.assertTrue(all(a[1] <= b[0] for a, b in zip(ranges, ranges[1:])))

    def test_attr_round_trip(self):
        subs, ranges, _ = al.attribution_table(self.info, self.rows)
        self.assertEqual(al.unpack_attr(al.pack_attr(subs, ranges)), (subs, ranges))

    def test_data_symbols_answer_to_what_a_row_calls_them(self):
        symbols = al.DataSymbols.of(self.info)
        self.assertEqual(symbols.find("global", "g_game")[0], 0x403000)
        self.assertEqual(symbols.find("global", "g_count")[0], 0x403030)
        self.assertEqual(symbols.find("static", "g_back")[0], 0x403010)
        self.assertEqual(symbols.find("static", "len@Mission::SelectSchema")[0], 0x403020)
        self.assertEqual(symbols.find("static", "Outer::Owner")[0], 0x403040)
        self.assertEqual(symbols.find("static", "g_back@src/units/units.cpp")[0], 0x403010)
        self.assertIsNone(symbols.find("global", "g_missing"))

    def test_a_symbols_end_is_the_next_symbol_or_the_limit(self):
        symbols = al.DataSymbols.of(self.info)
        self.assertEqual(symbols.end_of(0x403010, 0x1000), 0x403020)
        self.assertEqual(symbols.end_of(0x403040, 0x10), 0x403050)


class LayoutTest(unittest.TestCase):
    def setUp(self):
        self.layouts = al.Layouts(LAYOUT)
        self.types = al.TypeTable(self.layouts)

    def test_sizes(self):
        self.assertEqual(self.layouts.size_of("Player[3]"), 84)
        self.assertEqual(self.layouts.size_of("struct Vec3"), 8)
        self.assertEqual(self.layouts.size_of("Ref<class Unit>"), 4)
        self.assertEqual(self.layouts.size_of("char[8]"), 8)
        self.assertIsNone(self.layouts.size_of("Mystery"))

    def test_an_array_of_records_refines_by_element(self):
        tid, stride = self.types.refine("Player[10]")
        self.assertEqual(stride, 28)
        size, fields = self.types.entries[tid]
        self.assertEqual(size, 28)
        self.assertEqual(fields[0], (0, 4, "Player::active"))
        self.assertIn((16, 24, "Player::pos"), fields)
        self.assertEqual(fields[-1], (24, 28, "Player::tail"))

    def test_a_type_without_a_layout_is_not_refined(self):
        self.assertEqual(self.types.refine("int"), (al.NONE, 0))
        self.assertEqual(self.types.refine("Mystery"), (al.NONE, 0))
        self.assertEqual(self.types.refine(None), (al.NONE, 0))
        self.assertEqual(self.types.refine("Ref<struct Unit>"), (al.NONE, 0))

    def test_base_regions_of_game_rows(self):
        items = al.Items(types=self.types)
        size, fields = self.layouts.record("Game")
        rows = [{"name": "ticks", "kind": "game-region"}, {"name": "players", "kind": "game-region"},
                {"name": "after", "kind": "game-region"}]
        regions = al.base_regions(rows, fields, size, items, "game-region")
        by_name = {items.strings[r[2]]: r for r in regions}
        self.assertEqual(by_name["players"][:2], (4, 88))
        self.assertEqual(by_name["players"][4], 28)
        self.assertEqual(by_name["after"][:2], (88, 92))
        self.assertEqual([r[1] - r[0] for r in regions], sorted((r[1] - r[0] for r in regions), reverse=True))

    def test_items_pack_has_the_counts_it_says(self):
        items = al.Items(types=self.types)
        tid, stride = self.types.refine("Player[2]")
        items.statics.append((0x403000, 0x403040, items.string("g_players"), tid, stride))
        items.sites.append((0x401234, items.string("Make: new Player"), tid, stride))
        data = items.pack()
        self.assertEqual(data[:4], al.ITEMS_MAGIC)
        (strings,) = struct.unpack_from("<I", data, 4)
        pos = 8
        names = []
        for _ in range(strings):
            (n,) = struct.unpack_from("<H", data, pos)
            names.append(data[pos + 2:pos + 2 + n].decode())
            pos += 2 + n
        self.assertIn("Player::active", names)
        self.assertIn("g_players", names)
        (types,) = struct.unpack_from("<I", data, pos)
        self.assertEqual(types, 1)


class RunTest(unittest.TestCase):
    def test_equal_and_different_traces(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            header = "bt-digest 3\ntrace_format bt-trace 3\ncommit x\nscenario s\ngame_seed 1\ncrt_seed 1\nview_seed 1\nfpu_control_word 0x1\n"
            trace = "\n".join(["bt-trace 3", "commit x", "scenario s", "game_seed 1", "crt_seed 1", "view_seed 1",
                               "fpu_control_word 0x1", "1 units 00000000000000aa", "2 units 00000000000000bb", ""])
            (tmp / "trace.txt").write_text(trace)
            import tracedigest
            _, rows = tracedigest.parse_trace(tmp / "trace.txt")
            digests = tracedigest.digests(rows)
            (tmp / "ref.txt").write_text(header + "".join("%d %s\n" % d for d in digests))
            self.assertIsNone(al.trace_difference(tmp / "trace.txt", tmp / "ref.txt"))
            (tmp / "other.txt").write_text(header + "1 %s\n2 %s\n" % (digests[0][1], "0" * 16))
            self.assertIn("tick 2", al.trace_difference(tmp / "trace.txt", tmp / "other.txt"))

    def test_steady_leaves_out_the_presentation_rows(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "e.csv"
            path.write_text('w,r,i,phase,evidence,count,first_tick\nunits,ai,"x",units,read-after-write,1,1\n'
                            'units,ai,"x",presentation,read-after-write,9,1\n')
            self.assertEqual(len(al.steady(path)), 2)

    def test_summarise_counts_rows_and_shares(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "e.csv").write_text('writer,reader,item,phase,evidence,count,first_tick\n-,ai,"unresolved:block",units,read-unwritten,3,1\n'
                                       'units,ai,"Unit::health",units,read-after-write,5,2\n')
            (tmp / "s.txt").write_text("accesses 1000\nunknown_subsystem 10\nignored_helpers 4\nunresolved_image 20\n"
                                       "unresolved_block 30\nconstants_unnamed 2\nother_memory 7\n")
            lines = al.summarise("s", tmp / "e.csv", tmp / "s.txt", 3.0, 1.5)
            self.assertIn("2 edge rows, 2 items (1 unresolved:*)", lines[0])
            self.assertIn("1.0000%", lines[1])
            self.assertIn("x2.0", lines[2])


if __name__ == "__main__":
    unittest.main()
