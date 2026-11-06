"""Tests for projectioncheck.py, on hand-written units and temporary trees.

    uv run tools/test_projectioncheck.py
"""

import tempfile
import unittest
from pathlib import Path

import projectioncheck
from seamlink import Unit

PROJECT = "?PathfindingProject@@YGXPAUPathTraceView@@@Z"
GET = "?GetPathfinder@@YGPAVPathfinder@@XZ"
GAME = "?g_game@@3PAUGame@@A"
BASE = "?SimBlockBase@@YAPAXXZ"
FILE = "src/ai/pathfind.cpp"
RWSETS = "address,function,file,kind,target\n"


class ProjectionCheckTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def rows(self, *rows):
        (self.root / "data").mkdir(exist_ok=True)
        (self.root / "data/rwsets.csv").write_text(RWSETS + "".join(f",{f},{FILE},{k},{t}\n" for f, k, t in rows))

    def unit(self, project_refs, extra=()):
        unit = Unit(FILE, code={PROJECT, GET, *[c for c, _ in extra]})
        unit.refs = [(PROJECT, s) for s in project_refs] + [(c, s) for c, s in extra]
        return unit

    def check(self, *units):
        return projectioncheck.problems(list(units), self.root)

    def test_reading_the_root_is_allowed(self):
        self.rows(("PathfindingProject", "field_write", "PathTraceView::record"))
        self.assertEqual(self.check(self.unit([GAME, BASE])), [])

    def test_a_walked_callee_is_judged(self):
        self.rows()
        self.assertEqual(self.check(self.unit([GET], [(GET, GAME), (GET, BASE)])), [])
        found = self.check(self.unit([GET], [(GET, "?s_cache@@3HA")]))
        self.assertEqual(len(found), 1)
        self.assertIn("GetPathfinder references s_cache", found[0])

    def test_other_data_is_impure(self):
        self.rows()
        found = self.check(self.unit([GAME, "?g_scratchSteps@@3HA"]))
        self.assertEqual(len(found), 1)
        self.assertIn("neither the region root", found[0])

    def test_a_library_call_is_impure(self):
        self.rows()
        self.assertEqual(len(self.check(self.unit([GAME, "_memcpy"]))), 1)

    def test_symbols_naming_the_tick_the_scenario_or_the_harness_are_scenario_logic(self):
        self.rows()
        for name in ("?g_game@@3PAUGame@@A".replace("g_game", "g_tickCount"), "?HarnessTestMode@@YGHXZ", "?ReplayGateRef@@YGAAUReplayGate@@XZ",
                     "?CurrentScenario@@YAHXZ"):
            found = self.check(self.unit([name]))
            self.assertEqual(len(found), 1, name)
            self.assertIn("names the tick, the scenario or the harness", found[0])

    def test_a_write_to_state_is_found(self):
        self.rows(("PathfindingProject", "field_write", "PathExtra::refreshCounter"), ("PathfindingProject", "global_write", "g_x"),
                  ("PathfindingProject", "field_write", "PathTraceView::tickLoad"), ("PathfindingProject", "field_read", "Game::pathfinder"))
        found = self.check(self.unit([GAME]))
        self.assertEqual(len(found), 2)
        self.assertTrue(all("writes only the view" in line for line in found))

    def test_writes_of_functions_not_walked_are_ignored(self):
        self.rows(("RunSearches", "field_write", "Pathfinder::steps"))
        self.assertEqual(self.check(self.unit([GAME])), [])

    def test_a_missing_projection_is_a_problem(self):
        self.rows()
        found = self.check(Unit(FILE, code={"?Other@@YAXXZ"}))
        self.assertEqual(len(found), 1)
        self.assertIn("no function of that name", found[0])

    def test_a_function_walked_twice_is_reported_once(self):
        self.rows()
        found = self.check(self.unit([GET, GET], [(GET, "?s_a@@3HA"), (GET, "?s_a@@3HA")]))
        self.assertEqual(len(found), 1)


if __name__ == "__main__":
    unittest.main()
