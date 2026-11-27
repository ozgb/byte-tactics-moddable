"""Tests for routes.py.

    uv run tools/test_routes.py
"""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import routes

SOURCE = """
    {SCHED_MAP, LOS_RADAR_CELL_HEIGHTS, SCHED_LOS_RADAR, LosRadarInbox, 0},
    {SCHED_WIND_METEORS, WIND_METEORS_EFFECT_SPAWN, SCHED_ENGINE, EngineInbox, 1},
"""
VALUES = {"LOS_RADAR_CELL_HEIGHTS": 132, "WIND_METEORS_EFFECT_SPAWN": 80}


class RoutesTest(unittest.TestCase):
    def test_a_routes_source_parses_into_rows_in_order(self):
        self.assertEqual(routes.parse(SOURCE, VALUES), [
            ["map", "LOS_RADAR_CELL_HEIGHTS", "132", "los_radar", "queued"],
            ["wind_meteors", "WIND_METEORS_EFFECT_SPAWN", "80", "engine", "synchronous"]])

    def test_an_unknown_kind_fails_naming_it(self):
        with self.assertRaisesRegex(ValueError, "LOS_RADAR_CELL_HEIGHTS"):
            routes.parse(SOURCE, {"WIND_METEORS_EFFECT_SPAWN": 80})

    def test_the_csv_has_a_header_and_a_row_per_route(self):
        text = routes.render(routes.parse(SOURCE, VALUES))
        self.assertEqual(text.splitlines()[0], "publisher,kind,value,subscriber,delivery")
        self.assertEqual(len(text.splitlines()), 3)

    def test_enum_values_follow_c(self):
        values = routes.kind_values()
        self.assertEqual(values["LOS_RADAR_CELL_HEIGHTS"], 132)
        self.assertEqual(values["WIND_METEORS_EFFECT_SPAWN"], 80)


if __name__ == "__main__":
    unittest.main()
