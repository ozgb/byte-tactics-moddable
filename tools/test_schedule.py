"""Tests for schedule.py, on a hand-written program of three subsystems.

    uv run tools/test_schedule.py
"""

import contextlib
import io
import os
import shutil
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import schedule as sc

TICK = """void RunTick() {
    HarnessBeforeStep();
    HarnessPhase(0);
    g_seams.units->UpdateAll();
    HarnessPhase(1);
    HarnessPhase(2);
    HarnessPhase(3);
    HarnessPhase(4);
    HarnessPhase(5);
    HarnessPhase(6);
    HarnessPhase(7);
    HarnessAfterStep();
}
"""
UNITS = """void UpdateAll() {
    AccessLogPassScope accessPass(ACCESS_PASS_UNITS);
    for (int i = 0; i < n; ++i) {
        AccessLogEntity();
        g_seams.economy->Spend(u);
        x = g_seams.economy->Query(u);
        g_seams.orders->Run(u);
    }
}
"""
SEAMS = """struct Seams {
    UnitsSeam* units;
    EconomySeam* economy;
    OrdersSeam* orders;
};
"""
NATIVE = """static UnitsSeam s_units;
static EconomySeam s_economy;
static OrdersSeam s_orders;
    s_units.UpdateAll = UpdateAll;
    s_economy.Spend = Spend;
    s_economy.Query = Query;
    s_orders.Run = Run;
"""


def seam(struct, slot):
    return "indirect via void (*)() [%s::%s]" % (struct, slot)


def fn(name, file, start, end, calls=(), items=(), branches=0, returns="void"):
    return {"function": name, "file": file, "start": start, "end": end, "calls": [list(c) for c in calls],
            "items": [list(i) for i in items], "branches": branches, "returns": returns}


def functions(query_writes=False, units_reads=False):
    return [
        fn("RunTick", "src/tick.cpp", 1, 13, [(4, "indirect_call", seam("UnitsSeam", "UpdateAll"), False)]),
        fn("UpdateAll", "src/units.cpp", 1, 9,
           [(5, "indirect_call", seam("EconomySeam", "Spend"), False),
            (6, "indirect_call", seam("EconomySeam", "Query"), True),
            (7, "indirect_call", seam("OrdersSeam", "Run"), False)]
           + ([(8, "field_read", "Player::energy", False)] if units_reads else []),
           [("field_read", "Player::energy")] if units_reads else []),
        fn("Spend", "src/economy.cpp", 1, 3, items=[("field_write", "Player::energy")]),
        fn("Query", "src/economy.cpp", 5, 7, returns="int",
           items=[("field_read", "Player::energy")] + ([("global_write", "g_count")] if query_writes else [])),
        fn("Run", "src/orders.cpp", 1, 4, [(2, "call", "RandomInt", True)], [("field_read", "Player::energy")]),
    ]


SUBSYSTEMS = [{"function": "RunTick", "file": "src/tick.cpp", "subsystem": "game"},
              {"function": "UpdateAll", "file": "src/units.cpp", "subsystem": "units"},
              {"function": "Spend", "file": "src/economy.cpp", "subsystem": "economy"},
              {"function": "Query", "file": "src/economy.cpp", "subsystem": "economy"},
              {"function": "Run", "file": "src/orders.cpp", "subsystem": "orders"}]
SOURCES = {"src/tick.cpp": TICK, "src/units.cpp": UNITS, "src/economy.cpp": "\n" * 8, "src/orders.cpp": "\n" * 4}
LOOPS = {("units", 1): "each unit."}
FIXED = {"LOOPS": LOOPS, "TICK_STEPS": {}, "SILENT_INTENTS": {}}


def fixed(**names):
    """The page's description tables, set for the hand-written program (or as given)."""
    stack = contextlib.ExitStack()
    for name, value in dict(FIXED, **names).items():
        stack.enter_context(mock.patch.object(sc, name, value))
    return stack


def program(**kw):
    return sc.Program(functions(**kw), SUBSYSTEMS, [], SEAMS, NATIVE, SOURCES)


def dep(writer, reader, item="Player::energy", phase="units"):
    return {"writer": writer, "reader": reader, "item": item, "phase": phase, "class": "immediate", "runs": "3",
            "example": "s@1", "flags": ""}


def page(deps=(), pass_reads=(), **kw):
    return sc.Page(program(**kw), list(deps), list(pass_reads))


class Marks(unittest.TestCase):
    def test_pass_and_entity_extent(self):
        marks = sc.file_marks(UNITS)
        self.assertEqual([(m.kind, m.line, m.end, m.name) for m in marks],
                         [("pass", 2, 9, "units"), ("entity", 4, 8, "")])

    def test_comments_and_strings_hide_marks(self):
        text = 'void f() {\n    // AccessLogEntity();\n    g("AccessLogEntity()");\n}\n'
        self.assertEqual(sc.file_marks(text), [])

    def test_second_entity_in_one_block_ends_the_first(self):
        text = "void f() {\n  for (;;) {\n    AccessLogEntity();\n    a();\n    AccessLogEntity();\n    b();\n  }\n}\n"
        self.assertEqual([(m.line, m.end) for m in sc.file_marks(text)], [(3, 4), (5, 7)])


class Orders(unittest.TestCase):
    def test_phases_and_pass(self):
        built = sc.build(program())
        phases = dict(built["phases"])
        self.assertEqual([n.label for n in phases["units"]], ["units.UpdateAll"])
        self.assertEqual(phases["net"], [])
        unit_pass = next(p for p in built["passes"] if p.name == "units")
        self.assertEqual((unit_pass.phase, unit_pass.sub, unit_pass.holder), ("units", "units", "units.UpdateAll"))
        entity = next(part for part in unit_pass.parts if part.kind == "entity")
        self.assertEqual([n.label for n in entity.steps], ["economy.Spend", "economy.Query", "orders.Run"])

    def test_nested_effects(self):
        steps, _ = program().entry("OrdersSeam::Run")
        self.assertEqual([(n.kind, n.label) for n in steps], [("effect", "draw rng")])


class Classes(unittest.TestCase):
    def classes(self, **kw):
        p = page(**kw)
        return {label: sorted(c) for (_, label), c in p.calls.items()}

    def test_classes(self):
        self.assertEqual(self.classes(), {"economy.Spend": ["intent"], "economy.Query": ["view read"],
                                          "orders.Run": ["intent"]})

    def test_used_and_writing_is_mixed(self):
        self.assertEqual(self.classes(query_writes=True)["economy.Query"], ["mixed"])

    def test_branches_make_a_query(self):
        p = program()
        p.fns["Query"][0].branches = 1
        node = sc.Node("entry", "economy.Query", "economy", True, "UpdateAll", "EconomySeam::Query")
        self.assertEqual(sc.Effects(p).classify(node), "query")

    def test_tick_calls_are_not_listed(self):
        self.assertNotIn(("game", "units.UpdateAll"), page().calls)


class Splits(unittest.TestCase):
    def test_split_by_item(self):
        (d, found), = page([dep("economy", "orders")]).placed
        self.assertEqual((found.kind, found.where, found.first, found.second, found.by_subsystem),
                         ("split", "units, each entity", "economy.Spend", "orders.Run", False))

    def test_unnamed_item_is_placed_by_subsystem(self):
        (d, found), = page([dep("economy", "orders", item="Game: region_1")]).placed
        self.assertEqual((found.kind, found.by_subsystem), ("split", True))

    def test_caller_reads_back(self):
        (d, found), = page([dep("economy", "units")], units_reads=True).placed
        self.assertEqual((found.kind, found.first), ("apply", "economy.Spend"))

    def test_reader_before_writer_is_not_placed(self):
        p = page([dep("orders", "economy")])
        self.assertEqual(p.placed[0][1], None)
        self.assertIn("no split point for the immediate row orders -> economy, Player::energy, phase units",
                      p.problems())

    def test_same_subsystem_and_other_classes_ignored(self):
        rows = [dep("economy", "economy"), dict(dep("economy", "orders"), **{"class": "ordered"}),
                dep("economy", "presentation")]
        self.assertEqual(page(rows).placed, [])

    def test_across_entities(self):
        p = page([dep("economy", "orders")], [{"pass": "units", "item": "Player::energy", "reader": "orders"}])
        (key, rows, across), = p.groups()["units"]
        self.assertEqual((rows, across), (1, 1))


class Problems(unittest.TestCase):
    def test_loops_must_match_the_marks(self):
        with fixed(LOOPS={("units", 1): "x", ("ghost", 1): "y"}):
            self.assertEqual(page().problems(), ["LOOPS describes pass ghost, loop 1, which the marks do not hold"])
        with fixed(LOOPS={}):
            self.assertEqual(page().problems(), ["pass units, loop 1 has no description in LOOPS"])

    def test_tick_steps_must_be_called(self):
        with fixed(TICK_STEPS={"Gone": "x"}):
            self.assertEqual(page().problems(), ["TICK_STEPS describes Gone, which the tick does not call"])

    def test_intent_with_no_write_is_reported(self):
        p = program()
        p.fns["Spend"][0].items = set()
        with fixed():
            self.assertEqual(sc.Page(p, [], []).problems(),
                             ["economy.Spend is called as an intent but no write is found in it: it may hand back a value "
                              "the caller uses; class it, or give the reason in SILENT_INTENTS"])
        with fixed(SILENT_INTENTS={"economy.Spend": "a notification."}):
            self.assertEqual(sc.Page(p, [], []).problems(), [])
            self.assertIn("| `economy.Spend` | a notification. |", sc.render(sc.Page(p, [], [])))

    def test_listed_intent_that_writes_is_reported(self):
        with fixed(SILENT_INTENTS={"economy.Spend": "x"}):
            self.assertEqual(page().problems(), ["SILENT_INTENTS lists economy.Spend, which is no intent without a write"])


class Render(unittest.TestCase):
    def render(self):
        with fixed():
            return sc.render(page([dep("economy", "orders")]))

    def test_content(self):
        text = self.render()
        self.assertIn("| 1 | units | `units.UpdateAll` (the pass below) |", text)
        self.assertIn("Phase units, through `units.UpdateAll`. It visits each unit.", text)
        self.assertIn("1. `economy.Spend`: intent\n1. `economy.Query`: view read\n1. `orders.Run`: intent\n", text)
        self.assertIn("| units | `orders.Run` | intent |", text)
        self.assertIn("| economy | orders | units, each entity | split | `economy.Spend` (step 1) | `orders.Run` (step 3) |  "
                      "| by item | 1 | 0 |", text)
        self.assertIn("### `orders.Run`\n\n1. a draw from the Park-Miller generator (`rng`)\n", text)
        self.assertIn("| economy | 0 / 1 | 0 / 0 | 0 / 1 | 0 / 0 | 1 | 0 | 1 |", text)

    def test_names_no_function_outside_an_interface(self):
        text = self.render()
        for name in ("RunTick", "UpdateAll()", "Player::energy", "src/"):
            self.assertNotIn(name, text)


NESTED = """void UpdateAll() {
    AccessLogPassScope outer(ACCESS_PASS_UNITS_PLAYERS);
    for (int p = 0; p < 2; ++p) {
        AccessLogEntity();
        g_seams.economy->Spend(u);
        {
        AccessLogPassScope inner(ACCESS_PASS_UNITS);
        for (int i = 0; i < n; ++i) {
            AccessLogEntity();
            g_seams.orders->Run(u);
        }
        }
        x = g_seams.economy->Query(u);
    }
}
"""


def labels(steps):
    return [n.label for n in steps]


class Cycles(unittest.TestCase):
    def test_a_cut_order_is_not_kept(self):
        extra = [fn("CycA", "src/units.cpp", 20, 23, [(21, "call", "CycB", False),
                                                      (22, "indirect_call", seam("OrdersSeam", "Run"), False)]),
                 fn("CycB", "src/units.cpp", 25, 28, [(26, "call", "CycA", False),
                                                      (27, "indirect_call", seam("EconomySeam", "Spend"), False)])]
        subs = SUBSYSTEMS + [{"function": f["function"], "file": f["file"], "subsystem": "units"} for f in extra]
        p = sc.Program(functions() + extra, subs, [], SEAMS, NATIVE, SOURCES)
        self.assertEqual(labels(p.expand(p.fns["CycA"][0], "units")), ["economy.Spend", "orders.Run"])
        self.assertEqual(labels(p.expand(p.fns["CycB"][0], "units")), ["economy.Spend", "orders.Run", "economy.Spend"])


class NestedPasses(unittest.TestCase):
    def program(self):
        fns = functions()
        fns[1] = fn("UpdateAll", "src/units.cpp", 1, 15,
                    [(5, "indirect_call", seam("EconomySeam", "Spend"), False),
                     (10, "indirect_call", seam("OrdersSeam", "Run"), False),
                     (13, "indirect_call", seam("EconomySeam", "Query"), True)])
        return sc.Program(fns, SUBSYSTEMS, [], SEAMS, NATIVE, dict(SOURCES, **{"src/units.cpp": NESTED}))

    def test_inner_pass_is_a_step_of_its_parent(self):
        passes = {p.name: p for p in sc.build(self.program())["passes"]}
        outer, inner = passes["units.players"], passes["units"]
        self.assertEqual((inner.parent, inner.depth, outer.parent), ("units.players", 1, ""))
        entity = next(part for part in outer.parts if part.kind == "entity")
        self.assertEqual([(n.kind, n.label) for n in entity.steps],
                         [("entry", "economy.Spend"), ("pass", "units"), ("entry", "economy.Query")])
        self.assertEqual(labels(next(part for part in inner.parts if part.kind == "entity").steps), ["orders.Run"])

    def test_loops_name_both_passes(self):
        loops = {("units.players", 1): "each player.", ("units", 1): "each unit."}
        with fixed(LOOPS=loops):
            p = sc.Page(self.program(), [], [])
            self.assertEqual(p.problems(), [])
            self.assertIn("Nested in the pass `units.players`, where its lists name it.", sc.render(p))


class Placements(unittest.TestCase):
    def placed(self, p, row):
        (_, found), = sc.Page(p, [row], []).placed
        return found

    def test_writer_step_that_reaches_the_read_is_named(self):
        p = program()
        p.fns["Spend"][0].calls = [[2, "indirect_call", seam("OrdersSeam", "Run"), False]]
        found = self.placed(p, dep("economy", "orders"))
        self.assertEqual((found.kind, found.first, found.second, found.inside),
                         ("split", "economy.Spend", "orders.Run", "economy.Spend"))
        with fixed():
            self.assertIn("| `orders.Run` (step 3) | `economy.Spend` | by item |",
                          sc.render(sc.Page(p, [dep("economy", "orders")], [])))

    def nested(self, calls):
        p = program()
        p.fns["UpdateAll"][0].calls = [[5, "indirect_call", seam("EconomySeam", "Spend"), False]]
        p.fns["Spend"][0].calls = calls
        return self.placed(p, dep("economy", "orders"))

    def test_write_before_a_nested_call_in_source_order(self):
        write = [2, "field_write", "Player::energy", False]
        run = [3, "indirect_call", seam("OrdersSeam", "Run"), False]
        found = self.nested([write, run])
        self.assertEqual((found.kind, found.where, found.second), ("before", "units, each entity, in `economy.Spend`",
                                                                   "orders.Run"))
        found = self.nested([run, write])
        self.assertEqual((found.kind, found.where, found.first, found.second),
                         ("across", "units, each entity", "economy.Spend", "economy.Spend"))

    def test_caller_write_before_a_later_call_of_a_listed_step(self):
        p = program()
        p.fns["UpdateAll"][0].calls = [[5, "indirect_call", seam("OrdersSeam", "Run"), False],
                                       [6, "field_write", "Player::energy", False],
                                       [7, "indirect_call", seam("OrdersSeam", "Run"), False]]
        found = self.placed(p, dep("units", "orders"))
        self.assertEqual((found.kind, found.first, found.second, found.second_at, found.first_after),
                         ("before", "", "orders.Run", 1, 1))
        with fixed():
            self.assertIn("| write before calling | the caller, after step 1 | `orders.Run` again, after the write |",
                          sc.render(sc.Page(p, [dep("units", "orders")], [])))

    def test_reader_before_writer_is_across_entities(self):
        p = program()
        p.fns["Run"][0].items = {("field_write", "Player::energy")}
        found = self.placed(p, dep("orders", "economy"))
        self.assertEqual((found.kind, found.first, found.second), ("across", "orders.Run", "economy.Query"))


SNIPPET = """struct A { short pitch; short bank; };
struct O { A angles; int n; };
struct Seam { void (*Fill)(int* out); void (*Peek)(const int* in); void (*Take)(O* o); int (*Count)(); };
struct Seams { Seam* s; };
Seams g_seams;
A Get(O* o) { A p; p.pitch = o->angles.pitch; p.bank = 1; o->n = 2; return p; }
struct B { int k; B() { this->k = 0; } };
void Caller(O* o) {
    int v;
    g_seams.s->Fill(&v);
    g_seams.s->Peek(&v);
    g_seams.s->Take(o);
    g_seams.s->Count();
    o->n = g_seams.s->Count();
}
"""


@unittest.skipUnless(shutil.which("clang-cl"), "clang-cl is not installed")
class SyntaxTree(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with tempfile.TemporaryDirectory() as tmp, mock.patch.dict(os.environ, {"BT_NO_ASTCACHE": "1"}):
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "src/a.cpp").write_text(SNIPPET)
            argv = ["clang-cl", "--target=i686-pc-windows-msvc", "/nologo", "-TP", "-fms-compatibility",
                    "-fms-extensions", "-fsyntax-only", "-Xclang", "-ast-dump=json", "--", str(root / "src/a.cpp")]
            cls.functions = {f["function"]: f for f in sc.analyse((root, argv, str(root)))}

    def test_used_and_out_arguments(self):
        calls = [(line, target.split("[")[-1].rstrip("]"), used) for line, kind, target, used
                 in self.functions["Caller"]["calls"] if kind == "indirect_call"]
        self.assertEqual(calls, [(10, "Seam::Fill", True), (11, "Seam::Peek", False), (12, "Seam::Take", False),
                                 (13, "Seam::Count", False), (14, "Seam::Count", True)])

    def test_write_is_ordered_after_the_value_it_stores(self):
        steps = [(kind, target) for line, kind, target, _ in self.functions["Caller"]["calls"] if line == 14]
        self.assertEqual(steps[-2:], [("indirect_call", "indirect via int (*)() [Seam::Count]"), ("field_write", "O::n")])

    def test_local_and_constructed_objects_are_not_written(self):
        self.assertEqual(sorted(self.functions["Get"]["items"]), [("field_read", "A::pitch"), ("field_write", "O::n")])
        self.assertEqual(self.functions["B::B"]["items"], [])


class Main(unittest.TestCase):
    def run_main(self, args, doc):
        out, err = io.StringIO(), io.StringIO()
        with mock.patch.object(sc, "load", lambda build_dir, jobs: page([dep("economy", "orders")])), fixed(), \
                contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = sc.main(args + ["--doc", str(doc)])
        return code, out.getvalue() + err.getvalue()

    def test_write_then_check(self):
        with tempfile.TemporaryDirectory() as tmp:
            doc = Path(tmp) / "schedule.md"
            self.assertEqual(self.run_main(["--write"], doc)[0], 0)
            self.assertEqual(self.run_main(["--check"], doc), (0, "schedule: %s is current\n" % doc))

    def test_check_fails_on_stale_doc(self):
        with tempfile.TemporaryDirectory() as tmp:
            doc = Path(tmp) / "schedule.md"
            doc.write_text("old\n")
            code, out = self.run_main(["--check"], doc)
            self.assertEqual(code, 1)
            self.assertIn("is stale", out)

    def test_check_fails_on_missing_doc(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(self.run_main(["--check"], Path(tmp) / "none.md")[0], 1)


if __name__ == "__main__":
    unittest.main()
