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


class EconomyOwnerTest(unittest.TestCase):
    REGION = "LoadBattleAssets: SimAllocRegion Economy region"

    def test_region_parts_are_private_to_economy(self):
        self.assertEqual(ps.private_to(self.REGION + ".accounts"), ("economy", ("writer", "reader")))
        self.assertEqual(ps.private_to(ALLOCATION + ".heap")[0], "pathfinding")

    def test_player_accounts_are_private_for_writes_only(self):
        self.assertEqual(ps.private_to("UnitResources::energyMake"), ("economy", ("writer",)))
        rows = {"edges": [("players", "economy", "UnitResources::metalMake"), ("economy", "players", "UnitResources::metalMake"),
                          ("-", "ai", "UnitResources::energyStall")]}
        found = ps.violations(rows)
        self.assertEqual(found, ["edges: writer players on UnitResources::metalMake, which is written only by economy"])

    def test_another_subsystem_touching_the_region_is_found(self):
        rows = {"edges": [("units", "economy", self.REGION + ".accounts"), ("economy", "players", self.REGION + ".accounts")],
                "reads": [("-", "weapons", self.REGION + ".accounts"), ("-", "economy", self.REGION + ".accounts")]}
        found = ps.violations(rows)
        self.assertEqual(len(found), 3)
        self.assertTrue(all("private to economy" in line for line in found))

    def test_owners_do_not_share_items(self):
        rows = {"edges": [("economy", "-", "Pathfinder::steps"), ("pathfinding", "-", self.REGION + ".accounts")]}
        self.assertEqual(len(ps.violations(rows)), 2)


class WindMeteorsOwnerTest(unittest.TestCase):
    def test_meteor_globals_are_private_for_reads_and_writes(self):
        self.assertEqual(ps.private_to("g_meteorNextStrikeTime"), ("wind_meteors", ("writer", "reader")))
        self.assertEqual(len(ps.METEOR_GLOBALS), 13)
        rows = {"edges": [("wind_meteors", "-", "g_meteorActive"), ("-", "wind_meteors", "g_meteorActive"),
                          ("-", "weapons", "g_meteorWeapon"), ("game", "-", "g_meteorsEnabled")],
                "reads": [("-", "game", "g_meteorOrigin"), ("-", "wind_meteors", "g_meteorOrigin")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: reader weapons on g_meteorWeapon, which is private to wind_meteors",
                          "edges: writer game on g_meteorsEnabled, which is private to wind_meteors",
                          "reads: reader game on g_meteorOrigin, which is private to wind_meteors"])

    def test_wind_fields_are_private_for_writes_only(self):
        self.assertEqual(ps.private_to("windSpeedMax"), ("wind_meteors", ("writer",)))
        rows = {"edges": [("wind_meteors", "units", "windSpeed"), ("-", "ai", "windStrength"), ("wind_meteors", "-", "windZ"),
                          ("units", "-", "windDirection"), ("-", "units", "windSpeed")]}
        self.assertEqual(ps.violations(rows), ["edges: writer units on windDirection, which is written only by wind_meteors"])

    def test_similar_names_are_not_private(self):
        for item in ("windgenerator", "UnitDef::windgenerator", "Game::windSpeed", "g_meteors", "g_meteorWeaponX"):
            self.assertFalse(ps.is_private(item), item)

class FeaturesOwnerTest(unittest.TestCase):
    def test_spots_cell_fields_and_the_scan_index_are_private_for_writes_only(self):
        for item in ("FeatureAnimEntry::next", "FeatureAnimEntry::prev", "Cell::feature", "Cell::spot", "Cell::offsetX", "Cell::offsetY", "scanIndex"):
            self.assertEqual(ps.private_to(item), ("features", ("writer",)), item)

    def test_the_cell_flags_and_other_features_state_are_not_private(self):
        for item in ("Cell::flags", "Cell::unit", "Feature::energy", "featureCount", "FeatureAnimEntry2::next", "scanIndexes",
                     "FeatureAnimEntry::src", "FeatureAnimEntry::state", "FeatureAnimEntry::bytes_8", "FeatureAnimEntry::bytes_10"):
            self.assertFalse(ps.is_private(item), item)

    def test_a_features_write_passes(self):
        rows = {"edges": [("features", "weapons", "FeatureAnimEntry::next"), ("features", "features", "Cell::feature"),
                          ("features", "-", "scanIndex"), ("-", "features", "Cell::spot")],
                "cross": [("features", "units", "Cell::offsetX")]}
        self.assertEqual(ps.violations(rows), [])

    def test_a_units_or_weapons_write_fails(self):
        rows = {"edges": [("units", "features", "FeatureAnimEntry::prev"), ("weapons", "map", "Cell::feature"),
                          ("units", "-", "Cell::spot"), ("weapons", "features", "scanIndex")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer units on Cell::spot, which is written only by features",
                          "edges: writer units on FeatureAnimEntry::prev, which is written only by features",
                          "edges: writer weapons on Cell::feature, which is written only by features",
                          "edges: writer weapons on scanIndex, which is written only by features"])

    def test_a_weapons_or_orders_read_passes(self):
        rows = {"edges": [("-", "weapons", "FeatureAnimEntry::src"), ("features", "orders", "Cell::feature")],
                "reads": [("-", "weapons", "FeatureAnimEntry::bytes_10"), ("-", "map", "scanIndex")], "presentation": [("-", "presentation", "Cell::spot")]}
        self.assertEqual(ps.violations(rows), [])

    def test_the_owners_are_pathfinding_economy_wind_meteors_and_features(self):
        self.assertEqual([o.name for o in ps.OWNERS], ["pathfinding", "economy", "wind_meteors", "features"])


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
