"""Tests for schedule.py, on a hand-written program of three subsystems.

    uv run tools/test_schedule.py
"""

import contextlib
import io
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
            (7, "indirect_call", seam("OrdersSeam", "Run"), False)],
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
        with mock.patch.object(sc, "LOOPS", {("units", 1): "x", ("ghost", 1): "y"}), mock.patch.object(sc, "TICK_STEPS", {}):
            self.assertEqual(page().problems(), ["LOOPS describes pass ghost, loop 1, which the marks do not hold"])
        with mock.patch.object(sc, "LOOPS", {}), mock.patch.object(sc, "TICK_STEPS", {}):
            self.assertEqual(page().problems(), ["pass units, loop 1 has no description in LOOPS"])

    def test_tick_steps_must_be_called(self):
        with mock.patch.object(sc, "LOOPS", LOOPS), mock.patch.object(sc, "TICK_STEPS", {"Gone": "x"}):
            self.assertEqual(page().problems(), ["TICK_STEPS describes Gone, which the tick does not call"])


class Render(unittest.TestCase):
    def render(self):
        with mock.patch.object(sc, "LOOPS", LOOPS), mock.patch.object(sc, "TICK_STEPS", {}):
            return sc.render(page([dep("economy", "orders")]))

    def test_content(self):
        text = self.render()
        self.assertIn("| 1 | units | `units.UpdateAll` (the pass below) |", text)
        self.assertIn("Phase units, through `units.UpdateAll`. It visits each unit.", text)
        self.assertIn("1. `economy.Spend`: intent\n1. `economy.Query`: view read\n1. `orders.Run`: intent\n", text)
        self.assertIn("| units | `orders.Run` | intent |", text)
        self.assertIn("| economy | orders | units, each entity | split | `economy.Spend` | `orders.Run` | by item | 1 | 0 |",
                      text)
        self.assertIn("### `orders.Run`\n\n1. a draw from the Park-Miller generator (`rng`)\n", text)
        self.assertIn("| economy | 0 / 1 | 0 / 0 | 0 / 1 | 0 / 0 | 1 | 0 | 1 |", text)

    def test_names_no_function_outside_an_interface(self):
        text = self.render()
        for name in ("RunTick", "UpdateAll()", "Player::energy", "src/"):
            self.assertNotIn(name, text)


class Main(unittest.TestCase):
    def run_main(self, args, doc):
        out, err = io.StringIO(), io.StringIO()
        with mock.patch.object(sc, "load", lambda build_dir, jobs: page([dep("economy", "orders")])), \
                mock.patch.object(sc, "LOOPS", LOOPS), mock.patch.object(sc, "TICK_STEPS", {}), \
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
