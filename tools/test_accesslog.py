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
from unittest import mock
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
    if (!c.Init(64, 16, Alloc))
        return 2;
    char op;
    unsigned a, b, d, e, f;
    while (scanf(" %c", &op) == 1) {
        if (op == 'B') { scanf("%u", &a); c.BeginWindow(a); }
        else if (op == 'L') { scanf("%u", &a); c.ResolveLabel(a); }
        else if (op == 'E') { scanf("%u", &a); c.EndWindow(a); }
        else if (op == 'r') { scanf("%u %u %u %u %u", &a, &b, &d, &e, &f); fprintf(stderr, "%u\n", c.Read(a, b, d, e, f)); }
        else if (op == 'n') { scanf("%u", &a); c.Entity(a); }
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
        if (x.from != y.from) return x.from < y.from;
        return x.evidence < y.evidence;
    });
    for (auto& x : edges)
    {
        printf("%u %u %u %u %u %u %u", x.writer, x.reader, x.item, x.label, x.evidence, x.count, x.firstTick);
        if (x.evidence >= EVIDENCE_CROSS)
            printf(" %u", x.from);
        printf("\n");
    }
    return c.Overflowed() ? 3 : 0;
}
"""

NO_SUB = 0xFFFF
PENDING = 10
RAW, UNWRITTEN, UNREAD, ORDERED, CROSS, CARRIED = range(6)
EARLIER_ENTITY, SAME_RECORD = 1, 2


class ReferenceClassifier:
    """The rules of lib/accesslog/classifier.h, in Python."""

    def __init__(self):
        self.edges = {}
        self.state = {}
        self.written = []
        self.streams = {}
        self.stamp = 1
        self.label = 0
        self.epoch = 1
        self.window = 0
        self.current = 0
        self.window_labels = [[0] * 32, [0] * 32]
        self.entity = 0
        self.flags = []

    def count(self, writer, reader, item, evidence, tick, frm=0):
        key = (writer, reader, item, self.label, frm, evidence)
        if key in self.edges and self.edges[key][0] > 0:
            self.edges[key][0] += 1
        else:
            self.edges[key] = [1, tick]

    def begin(self, label):
        self.stamp += 1
        self.label = label
        self.written = []
        self.entity = 0
        if label == PENDING:
            self.epoch += 1
            self.current ^= 1
            self.window = 0
        elif self.window < 31:
            self.window += 1
        self.window_labels[self.current][self.window] = label

    def resolve(self, label):
        if self.label != PENDING:
            return
        self.label = label
        self.window_labels[self.current][self.window] = label
        for key in [k for k in self.edges if k[3] == PENDING]:
            count, first = self.edges[key]
            if count == 0:
                continue
            self.edges[key][0] = 0
            to = key[:3] + (label,) + key[4:]
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

    def item_state(self, item):
        return self.state.setdefault(item, {"stamp": 0, "writer": 0, "ent": (0, 0), "read": 0, "ord": 0,
                                            "epoch": 0, "wsub": 0, "win": 0})

    def read(self, item, a, b, sub, tick):
        s = self.item_state(item)
        flags = 0
        if s["stamp"] == self.stamp:
            if s["writer"] != sub or s["ent"] != (a, b):
                self.count(s["writer"], sub, item, RAW, tick)
            s["read"] = 1
            if s["ord"] != 0 and s["ord"] < self.entity:
                flags |= EARLIER_ENTITY
                if s["ent"] == (a, b):
                    flags |= SAME_RECORD
        else:
            self.count(NO_SUB, sub, item, UNWRITTEN, tick)
            if s["epoch"] == self.epoch and s["wsub"] != sub:
                self.count(s["wsub"], sub, item, CROSS, tick, self.window_labels[self.current][s["win"]])
            elif s["epoch"] != 0 and s["epoch"] + 1 == self.epoch and s["wsub"] != sub:
                frm = self.window_labels[self.current ^ 1][s["win"]]
                if self.label == PENDING or frm > self.label:
                    self.count(s["wsub"], sub, item, CARRIED, tick, frm)
        self.flags.append(flags)

    def write(self, item, a, b, sub, tick):
        s = self.item_state(item)
        if s["stamp"] != self.stamp:
            s["stamp"] = self.stamp
            self.written.append(item)
        s["writer"], s["ent"], s["read"] = sub, (a, b), 0
        s["ord"], s["epoch"], s["wsub"], s["win"] = self.entity, self.epoch, sub, self.window

    def set_entity(self, ordinal):
        self.entity = ordinal

    def ordered(self, stream, item, sub, tick):
        last = self.streams.get(stream)
        if last and last[0] == self.stamp and last[1] != sub:
            self.count(last[1], sub, item, ORDERED, tick)
        self.streams[stream] = (self.stamp, sub)

    def lines(self):
        rows = sorted((k, v) for k, v in self.edges.items() if v[0] > 0)
        return ["%u %u %u %u %u %u %u" % (k[0], k[1], k[2], k[3], k[5], v[0], v[1]) + (" %u" % k[4] if k[5] >= CROSS else "")
                for k, v in rows]


def feed(script, reference):
    """Run a script (a list of tuples) through the Python reference and return the C++ driver's text for it."""
    for op, *args in script:
        {"B": reference.begin, "L": reference.resolve, "E": reference.end, "r": reference.read, "w": reference.write,
         "o": reference.ordered, "n": reference.set_entity}[op](*args)
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
        self.assertEqual([int(x) for x in run.stderr.split()], reference.flags)
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

    def test_a_later_phase_reads_what_an_earlier_one_wrote(self):
        lines = self.both([("B", PENDING), ("w", 5, 0, 0, 1, 1), ("L", 1), ("E", 1), ("B", 2), ("r", 5, 0, 0, 2, 2), ("E", 2)])
        self.assertIn("1 2 5 2 %u 1 2 1" % CROSS, lines)
        self.assertIn("%u 2 5 2 %u 1 2" % (NO_SUB, UNWRITTEN), lines)

    def test_the_same_subsystem_reading_an_earlier_phase_write_is_no_cross_edge(self):
        lines = self.both([("B", PENDING), ("w", 5, 0, 0, 1, 1), ("L", 1), ("E", 1), ("B", 2), ("r", 5, 0, 0, 1, 2), ("E", 2)])
        self.assertEqual([line for line in lines if line.split()[4] == str(CROSS)], [])

    def test_a_write_of_the_pending_window_keeps_the_phase_it_resolves_to(self):
        lines = self.both([("B", PENDING), ("w", 5, 0, 0, 1, 1), ("L", 1), ("E", 1), ("B", 3), ("r", 5, 0, 0, 2, 2), ("E", 2)])
        self.assertIn("1 2 5 3 %u 1 2 1" % CROSS, lines)

    def test_the_last_write_of_the_tick_is_the_one_a_later_phase_reads(self):
        lines = self.both([("B", PENDING), ("w", 5, 0, 0, 1, 1), ("L", 1), ("E", 1), ("B", 2), ("w", 5, 0, 0, 3, 2), ("E", 2),
                           ("B", 4), ("r", 5, 0, 0, 2, 2), ("E", 2)])
        self.assertIn("3 2 5 4 %u 1 2 2" % CROSS, lines)
        self.assertNotIn("1 2 5 4 %u 1 2 1" % CROSS, lines)

    def test_a_read_of_the_last_ticks_later_phase_write_is_carried(self):
        lines = self.both([("B", PENDING), ("L", 1), ("E", 1), ("B", 4), ("w", 5, 0, 0, 1, 1), ("E", 1),
                           ("B", PENDING), ("r", 5, 0, 0, 2, 2), ("L", 1), ("E", 2)])
        self.assertIn("1 2 5 1 %u 1 2 4" % CARRIED, lines)

    def test_a_carried_read_of_an_earlier_phase_write_is_not_recorded(self):
        lines = self.both([("B", PENDING), ("L", 1), ("E", 1), ("B", 4), ("w", 5, 0, 0, 1, 1), ("E", 1),
                           ("B", PENDING), ("L", 1), ("E", 2), ("B", 5), ("r", 5, 0, 0, 2, 2), ("E", 2)])
        self.assertEqual([line for line in lines if line.split()[4] == str(CARRIED)], [])

    def test_a_write_two_ticks_ago_is_no_evidence(self):
        lines = self.both([("B", PENDING), ("L", 1), ("w", 5, 0, 0, 1, 1), ("E", 1), ("B", PENDING), ("L", 1), ("E", 2),
                           ("B", PENDING), ("r", 5, 0, 0, 2, 3), ("L", 1), ("E", 3)])
        self.assertEqual([line for line in lines if int(line.split()[4]) >= CROSS], [])

    def test_the_entity_ordinal_of_the_writer_tells_an_earlier_entity_from_the_same_one(self):
        self.both([("B", 2), ("n", 1), ("w", 5, 7, 7, 1, 1), ("r", 5, 7, 7, 1, 1), ("n", 2), ("r", 5, 7, 7, 1, 1),
                   ("r", 5, 8, 8, 1, 1), ("w", 5, 8, 8, 1, 1), ("r", 5, 8, 8, 1, 1), ("E", 1)])
        reference = ReferenceClassifier()
        feed([("B", 2), ("n", 1), ("w", 5, 7, 7, 1, 1), ("r", 5, 7, 7, 1, 1), ("n", 2), ("r", 5, 7, 7, 1, 1),
              ("r", 5, 8, 8, 1, 1), ("w", 5, 8, 8, 1, 1), ("r", 5, 8, 8, 1, 1), ("E", 1)], reference)
        self.assertEqual(reference.flags, [0, EARLIER_ENTITY | SAME_RECORD, EARLIER_ENTITY, 0])

    def test_a_write_outside_every_entity_is_no_earlier_entity(self):
        reference = ReferenceClassifier()
        feed([("B", 2), ("w", 5, 0, 0, 1, 1), ("n", 3), ("r", 5, 0, 0, 1, 1), ("E", 1)], reference)
        self.assertEqual(reference.flags, [0])
        self.both([("B", 2), ("w", 5, 0, 0, 1, 1), ("n", 3), ("r", 5, 0, 0, 1, 1), ("E", 1)])

    def test_a_new_window_starts_outside_every_entity(self):
        self.both([("B", 2), ("n", 4), ("w", 5, 0, 0, 1, 1), ("E", 1), ("B", 3), ("w", 6, 0, 0, 1, 1), ("n", 1), ("r", 6, 0, 0, 1, 1), ("E", 1)])

    def test_a_long_random_sequence_agrees_with_the_reference(self):
        rng = random.Random(835)
        script = []
        tick = 0
        for _ in range(300):
            tick += 1
            label = rng.choice([PENDING, PENDING, 1, 2, 3, 8, 9])
            script.append(("B", label))
            for _ in range(rng.randrange(1, 60)):
                kind = rng.choice("rrrwwon")
                item, sub = rng.randrange(40), rng.randrange(5)
                if kind == "o":
                    script.append(("o", rng.randrange(4), 60 + rng.randrange(4), sub, tick))
                elif kind == "n":
                    script.append(("n", rng.randrange(5)))
                else:
                    script.append((kind, item, rng.randrange(3), rng.randrange(3), sub, tick))
            if label == PENDING:
                script.append(("L", rng.choice([0, 1])))
            script.append(("E", tick))
        lines = self.both(script)
        kinds = {int(line.split()[4]) for line in lines}
        self.assertTrue({CROSS, CARRIED} <= kinds)


class OverflowTest(unittest.TestCase):
    STATS = "accesses 5\noverflow %d\nphase_overflow %d\npresentation_overflow %d\nwrites 1\n"

    def test_each_counter_above_zero_fails_the_scenario(self):
        for counter in range(3):
            values = [0, 0, 0]
            values[counter] = 2
            with tempfile.TemporaryDirectory() as t:
                path = Path(t) / "s.stats.txt"
                path.write_text(self.STATS % tuple(values))
                with self.assertRaises(SystemExit) as caught:
                    al.check_stats("scen", path)
                self.assertIn("scen", str(caught.exception))
                self.assertIn(al.OVERFLOW_COUNTERS[counter] + " is 2", str(caught.exception))

    def test_all_counters_at_zero_pass(self):
        with tempfile.TemporaryDirectory() as t:
            path = Path(t) / "s.stats.txt"
            path.write_text(self.STATS % (0, 0, 0))
            al.check_stats("scen", path)
        self.assertEqual(al.overflowed("scen", self.STATS % (0, 0, 0)), [])

    def test_the_store_paths_fail_on_an_overflow(self):
        import depclass
        import phaseclass

        with tempfile.TemporaryDirectory() as t:
            src = Path(t) / "src"
            src.mkdir()
            (src / "s.edges.csv").write_text("writer,reader,item,phase,evidence,count,first_tick\n")
            (src / "s.stats.txt").write_text(self.STATS % (0, 1, 0))
            for store in (depclass.store, phaseclass.store):
                with self.assertRaises(SystemExit) as caught:
                    store(src, Path(t) / "dst")
                self.assertIn("s: the counter phase_overflow", str(caught.exception))


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


POOL_LAYOUT = """*** Dumping AST Record Layout
         0 | struct PathExtra
         0 |   int nodeLimit
         4 |   int poolOffset
         8 |   int itemsOffset
        12 |   int cellsOffset
        16 |   int dirtyOffset
        20 |   int goalOffset
        24 |   int pendingOffset
        28 |   int requesterBuckets
        32 |   int unitBuckets
        36 |   int eventsOffset
        40 |   int[10] budgetCap
           | [sizeof=80, align=4,
           |  nvsize=80, nvalign=4]

*** Dumping AST Record Layout
         0 | struct Node
         0 |   int index
         4 |   struct NodeData data
         4 |     struct Point16 pos
         4 |       short x
         6 |       short y
         8 |     int g
           | [sizeof=12, align=4,
           |  nvsize=12, nvalign=4]

*** Dumping AST Record Layout
         0 | class Pathfinder
         0 |   class OpenHeap (base)
         0 |     struct Ref<struct Node> pool
         0 |       unsigned int offset
         4 |     int count
         8 |   struct PathGrid grid
         8 |     struct Ref<struct PathCell> cells
         8 |       unsigned int offset
        12 |     unsigned int width
        16 |   union Pathfinder::(anonymous at x.h:315:5) 
        16 |     struct TurnCostTable table
        16 |       unsigned char[8] values
        16 |     unsigned char[8] turnCost
        24 |   PathUnit[10] cursor
           | [sizeof=64, align=1,
           |  nvsize=64, nvalign=1]

*** Dumping AST Record Layout
         0 | struct PathCell
         0 |   unsigned char flags
         1 |   unsigned char dir
         2 |   unsigned short node
           | [sizeof=4, align=2,
           |  nvsize=4, nvalign=2]

*** Dumping AST Record Layout
         0 | struct PathPending
         0 |   int requester
         4 |   int unit
           | [sizeof=8, align=4,
           |  nvsize=8, nvalign=4]
"""


class RegionTest(unittest.TestCase):
    def setUp(self):
        self.layouts = al.Layouts(POOL_LAYOUT)

    def test_flat_fields_walk_bases_and_nested_records(self):
        size, fields = al.flat_fields(self.layouts, "Node")
        self.assertEqual(size, 12)
        self.assertEqual(fields, [(0, 4, "Node::index"), (4, 6, "Node::data.pos.x"), (6, 8, "Node::data.pos.y"), (8, 12, "Node::data.g")])

    def test_refs_arrays_and_unions_are_leaves(self):
        _, fields = al.flat_fields(self.layouts, "Pathfinder")
        names = {n: (s, e) for s, e, n in fields}
        self.assertEqual(names["Pathfinder::pool"], (0, 4))
        self.assertEqual(names["Pathfinder::count"], (4, 8))
        self.assertEqual(names["Pathfinder::grid.cells"], (8, 12))
        self.assertEqual(names["Pathfinder::grid.width"], (12, 16))
        self.assertEqual(names["Pathfinder::table"], (16, 24))
        self.assertNotIn("Pathfinder::turnCost", names)
        self.assertEqual(names["Pathfinder::cursor"][0], 24)

    def test_a_flattened_type_refines_by_leaf(self):
        types = al.TypeTable(self.layouts)
        tid, stride = types.refine("Node")
        self.assertEqual(stride, 12)
        self.assertIn((8, 12, "Node::data.g"), types.entries[tid][1])

    def test_region_parts_follow_the_offsets_the_region_holds(self):
        items = al.Items(types=al.TypeTable(self.layouts))
        with mock.patch.object(al, "PATHFINDER_HEADER", Path(tempfile.mkdtemp()) / "pathfinder.h"):
            al.PATHFINDER_HEADER.write_text("enum { EXTRA_OFFSET = 64 };")
            parts = al.region_parts(items, "Init: new AISearch region")
        by_name = {items.strings[p[4]]: p for p in parts}
        self.assertEqual(by_name["Init: new AISearch region.record"][:4], (al.NONE, 0, al.NONE, 64))
        self.assertEqual(by_name["Init: new AISearch region.extra"][:4], (al.NONE, 64, al.NONE, 144))
        pool = by_name["Init: new AISearch region.pool"]
        self.assertEqual(pool[:4], (64 + 4, al.NONE, 64 + 8, al.NONE))
        self.assertEqual(pool[6], 12)
        heap = by_name["Init: new AISearch region.heap"]
        self.assertEqual((heap[5], heap[6]), (al.NONE, 0))
        events = by_name["Init: new AISearch region.events"]
        self.assertEqual(events[:4], (64 + 36, al.NONE, al.NONE, al.NONE))
        self.assertEqual(len(parts), 2 + len(al.REGION_SLOTS))

    def test_items_pack_appends_the_parts_by_site(self):
        items = al.Items(types=al.TypeTable(self.layouts))
        items.sites.append((0x401234, items.string("Init: new AISearch region"), al.NONE, 0))
        items.parts[0x401234] = [(al.NONE, 0, al.NONE, 64, items.string("a"), al.NONE, 0), (4, al.NONE, 8, al.NONE, items.string("b"), al.NONE, 0)]
        data = items.pack()
        tail = data[-(4 + 8 + 2 * 28):]
        self.assertEqual(struct.unpack_from("<III", tail, 0), (1, 0x401234, 2))
        self.assertEqual(struct.unpack_from("<7I", tail, 12)[:4], (al.NONE, 0, al.NONE, 64))

    def test_the_reads_file_is_collected_and_kept(self):
        self.assertIn("access.reads.csv", al.OUTPUT_FILES)
        self.assertIn('("reads.csv", "access.reads.csv")', Path(al.__file__).read_text())


class PresentationTest(unittest.TestCase):
    def test_funcs_round_trip(self):
        data = al.pack_funcs([(0x401000, 0x401040, "Draw"), (0x401040, 0x401100, "Unit::Move")])
        self.assertEqual(data[:4], al.FUNCS_MAGIC)
        count, start, end, length = struct.unpack_from("<IIIH", data, 4)
        self.assertEqual((count, start, end, length), (2, 0x401000, 0x401040, 4))
        self.assertEqual(data[18:22], b"Draw")

    def test_function_table_covers_the_code_symbols_by_address(self):
        table = al.function_table(al.parse_map(MAP))
        self.assertEqual([a for a, _, _ in table], sorted(a for a, _, _ in table))
        for (_, end, _), (start, _, _) in zip(table, table[1:]):
            self.assertEqual(end, start)

    def test_the_presentation_file_is_put_collected_and_kept(self):
        self.assertIn("access.presentation.csv", al.OUTPUT_FILES)
        self.assertIn("accesslog.funcs", al.FUNCS_NAME)
        source = Path(al.__file__).read_text()
        self.assertIn('("presentation.csv", "access.presentation.csv")', source)
        self.assertIn("FUNCS_NAME", source.split("def play(")[1].split("def steady(")[0])
        runtime = (ROOT / "lib/accesslog/access_log.cpp").read_text()
        self.assertIn('WriteWhole("access.presentation.csv", out)', runtime)
        self.assertIn("out.Add(\"item,reader,function,windows,events,container\\n\")", runtime)


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


class LoadedSaveTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.scenarios = self.root / "scenarios"
        self.scenarios.mkdir()
        (self.scenarios / "saver.txt").write_text("scenario saver\n")
        (self.scenarios / "loader.txt").write_text("scenario loader\nload saver\n")
        (self.scenarios / "plain.txt").write_text("scenario plain\n")
        self.calls = []

    def fake_play(self, exe, scenario, game_dir, tables_dir, raw, timeout, put=None, collect=()):
        self.calls.append((scenario.stem, dict(put or {}), tuple(collect)))
        directory = self.root / ("run%d" % len(self.calls))
        for name in collect:
            (directory / name).parent.mkdir(parents=True, exist_ok=True)
            (directory / name).write_text(scenario.stem + name)
        return directory, 1.0

    def test_a_load_line_plays_the_saving_scenario_first_and_puts_its_save_into_the_loading_run(self):
        build = self.root / "build"
        build.mkdir()
        with mock.patch.object(al, "play", self.fake_play):
            put = al.loaded_save(build, self.scenarios / "loader.txt", self.root, 5.0)
        self.assertEqual([c[0] for c in self.calls], ["saver"])
        self.assertEqual(self.calls[0][2], tuple(al.tracediff.SAVE_FILES))
        self.assertEqual(sorted(put), sorted(al.tracediff.SAVE_FILES))
        for dest, source in put.items():
            self.assertEqual(source.read_text(), "saver" + dest)
            self.assertEqual(source.parent, build / "access" / "save" / "loader")
        self.assertEqual(al.loaded_save(build, self.scenarios / "plain.txt", self.root, 5.0), {})
        self.assertEqual(len(self.calls), 1)

    def test_run_plays_the_saving_scenario_before_the_loading_one(self):
        build = self.root / "build"
        build.mkdir()
        scenario = self.scenarios / "loader.txt"
        with mock.patch.object(al, "play", self.fake_play), mock.patch.object(al, "ROOT", self.root), \
                mock.patch.object(al, "trace_difference", return_value=None), \
                mock.patch.object(al, "summarise", return_value=[]):
            (self.root / "reference/scenarios").mkdir(parents=True)
            (self.root / "reference/traces").mkdir(parents=True)
            (self.root / "reference/scenarios/loader.txt").write_text(scenario.read_text())
            (self.root / "reference/scenarios/saver.txt").write_text("scenario saver\n")
            (self.root / "reference/traces/loader.txt").write_text("")
            self.assertEqual(al.run(build, ["loader"], self.root, False, False, None, 5.0), 0)
        self.assertEqual([c[0] for c in self.calls], ["saver", "loader"])
        self.assertEqual(sorted(self.calls[1][1]), sorted(al.tracediff.SAVE_FILES))


if __name__ == "__main__":
    unittest.main()
