"""Tests for privatestate.py, on synthetic summaries.

    uv run tools/test_privatestate.py
"""

import gzip
import json
import tempfile
import unittest
from pathlib import Path

import privatestate as ps

ALLOCATION = "LoadBattleAssets: GameAllocIgnoreTag AISearch region"


class PrivateItemTest(unittest.TestCase):
    def test_region_items_are_private(self):
        for item in ("pathfinder", "Pathfinder::steps", "PathExtra::seenWidth", "Node::data.g", "PathCell::flags", "PathPending::unit",
                     ALLOCATION + ".heap", ALLOCATION + ".events"):
            self.assertTrue(ps.is_private(item), item)

    def test_other_items_are_not(self):
        for item in ("Unit::pos", "g_game", "players", "HpaInit: GameAllocIgnoreTag AISearch region", "Pathfinders", "PathEvent::kind",
                     "rng.parkMiller"):
            self.assertFalse(ps.is_private(item), item)


class ViolationTest(unittest.TestCase):
    def test_pathfinding_and_no_subsystem_are_allowed(self):
        rows = {"edges": [("pathfinding", "-", "Pathfinder::steps"), ("-", "pathfinding", "Node::data.g"), ("pathfinding", "pathfinding", "pathfinder")],
                "reads": [("-", "pathfinding", "PathExtra::tickLoad")]}
        self.assertEqual(ps.violations(rows), [])

    def test_another_reader_or_writer_is_found(self):
        rows = {"edges": [("pathfinding", "orders", "pathfinder"), ("seams", "pathfinding", "PathExtra::seenWidth")],
                "reads": [("-", "players", ALLOCATION + ".goal")], "presentation": [("-", "presentation", "PathCell::flags")]}
        found = ps.violations(rows)
        self.assertEqual(len(found), 4)
        self.assertTrue(any(line.startswith("edges: reader orders on pathfinder") for line in found))
        self.assertTrue(any(line.startswith("edges: writer seams on PathExtra::seenWidth") for line in found))
        self.assertTrue(any(line.startswith("presentation: reader presentation") for line in found))

    def test_other_items_are_not_judged(self):
        self.assertEqual(ps.violations({"edges": [("orders", "units", "Unit::pos")]}), [])

    def test_a_row_is_reported_once(self):
        rows = {"edges": [("units", "orders", "pathfinder")] * 3}
        self.assertEqual(len(ps.violations(rows)), 2)


class ExposedTest(unittest.TestCase):
    SCHEMA = {"tables": {"Pathfinder": {"fields": [{"name": "pool"}, {"name": "cursor[0]"}, {"name": "cursor[1]"}]},
                         "PathfinderSearch": {"fields": [{"name": "start_x"}]}, "Node": {"fields": [{"name": "g"}]}},
              "globals": {"pathfinder": [{"name": "playerTickLoad"}]}}

    def test_fields_map_to_items(self):
        wanted, problems = ps.exposed(self.SCHEMA)
        self.assertEqual(problems, [])
        self.assertEqual(wanted["Pathfinder::cursor"], ["Pathfinder.cursor[0]", "Pathfinder.cursor[1]"])
        self.assertEqual(wanted["Pathfinder::start.x"], ["PathfinderSearch.start_x"])
        self.assertEqual(wanted["PathExtra::tickLoad"], ["pathfinder.playerTickLoad"])
        self.assertIn("PathCell::flags", wanted)
        self.assertIn(ps.HEAP, wanted)

    def test_a_new_field_is_a_problem(self):
        schema = json.loads(json.dumps(self.SCHEMA))
        schema["tables"]["Node"]["fields"].append({"name": "newField"})
        schema["globals"]["pathfinder"].append({"name": "newGlobal"})
        _, problems = ps.exposed(schema)
        self.assertEqual(len(problems), 2)

    def test_the_stored_schema_is_covered(self):
        _, problems = ps.exposed(json.loads(ps.SCHEMA.read_text()))
        self.assertEqual(problems, [])


class ShadowTest(unittest.TestCase):
    WANTED = {"Pathfinder::steps": ["Pathfinder.steps"], "Node::data.g": ["Node.g"], ps.HEAP: ["heap"]}

    def test_every_item_read_passes(self):
        reads = [{"item": "Pathfinder::steps", "reader": "pathfinding"}, {"item": "Node::data.g", "reader": "pathfinding"},
                 {"item": ALLOCATION + ".heap", "reader": "pathfinding"}]
        self.assertEqual(ps.shadow_problems(self.WANTED, ps.pathfinding_reads(reads), {}), [])

    def test_a_read_by_another_subsystem_does_not_count(self):
        reads = [{"item": "Pathfinder::steps", "reader": "pathfinding"}, {"item": "Node::data.g", "reader": "orders"}]
        found = ps.shadow_problems(self.WANTED, ps.pathfinding_reads(reads), {})
        self.assertEqual(len(found), 2)
        self.assertTrue(found[0].startswith("shadow field AISearch region.heap"))
        self.assertTrue(found[1].startswith("shadow field Node::data.g (Node.g)"))

    def test_an_allowed_item_passes_and_a_stale_row_fails(self):
        read = {"Pathfinder::steps", "Node::data.g"}
        self.assertEqual(ps.shadow_problems(self.WANTED, read, {ps.HEAP: "why"}), [])
        found = ps.shadow_problems(self.WANTED, read | {ps.HEAP}, {ps.HEAP: "why"})
        self.assertEqual(len(found), 1)
        self.assertIn("stale row", found[0])

    def test_an_allow_row_for_an_unknown_item_fails(self):
        found = ps.shadow_problems({}, set(), {"Nope::x": "why"})
        self.assertEqual(len(found), 1)
        self.assertIn("not an exposed item", found[0])


class StoreTest(unittest.TestCase):
    def test_reads_are_sorted_and_gzipped_without_a_time(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, out = Path(tmp) / "src", Path(tmp) / "out"
            source.mkdir()
            (source / "a.reads.csv").write_text('item,reader,count,first_tick\n"Node::data.g",units,3,5\n"A::b",pathfinding,1,2\n')
            self.assertEqual(ps.store(source, out), 1)
            data = (out / "a.reads.csv.gz").read_bytes()
            self.assertEqual(gzip.decompress(data).decode(), 'item,reader,count,first_tick\nA::b,pathfinding,1,2\nNode::data.g,units,3,5\n')
            ps.store(source, out)
            self.assertEqual((out / "a.reads.csv.gz").read_bytes(), data)


if __name__ == "__main__":
    unittest.main()
