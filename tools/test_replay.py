"""Tests for replay.py.

    uv run tools/test_replay.py
"""

import tempfile
import unittest
from pathlib import Path

import replay

GOOD = "bt-replay 2\nsubsystem wind_meteors 1.2\ncall 1 UpdateWind\ncall 2 UpdateMeteors\n  r Game.width 1024\n  i services.GameRand main = 5\n  w rng.main 0x00000001\n"


class Scenario(unittest.TestCase):
    def test_record_line_goes_last(self):
        self.assertEqual(replay.with_record("ticks 5\n", "wind_meteors"), "ticks 5\nrecord wind_meteors\n")
        self.assertEqual(replay.with_record("ticks 5", ["wind_meteors"]), "ticks 5\nrecord wind_meteors\n")
        self.assertEqual(replay.with_record("ticks 5\n", ["wind_meteors"], (3, 9)), "ticks 5\nrecord wind_meteors 3 9\n")

    def test_several_subsystems_get_a_line_each(self):
        self.assertEqual(replay.with_record("ticks 5\n", ["economy", "wind_meteors"], (3, 9)),
                         "ticks 5\nrecord economy 3 9\nrecord wind_meteors 3 9\n")

    def test_a_window_can_differ_per_subsystem(self):
        self.assertEqual(replay.with_record("ticks 5\n", ["economy", "wind_meteors"], {"economy": (3, 9)}),
                         "ticks 5\nrecord economy 3 9\nrecord wind_meteors\n")

    def test_the_stored_windows_name_a_recording_that_exists(self):
        for key, window in replay.stored_windows().items():
            subsystem, scenario = key.split("/")
            self.assertTrue((replay.RECORDINGS / subsystem / (scenario + ".txt")).is_file(), key)
            self.assertLessEqual(window[0], window[1])

    def test_ticks(self):
        self.assertEqual(replay.scenario_ticks("map x\nticks 150\n"), 150)
        with self.assertRaises(replay.Error):
            replay.scenario_ticks("map x\n")

    def test_named_scenarios_resolve(self):
        self.assertEqual(replay.scenario_path("meteors-hailstorm").name, "meteors-hailstorm.txt")
        with self.assertRaises(replay.Error):
            replay.scenario_path("no-such-scenario")


class Form(unittest.TestCase):
    def test_a_good_recording_has_no_problems(self):
        self.assertEqual(replay.check_form(GOOD), [])

    def test_problems_are_named(self):
        self.assertTrue(replay.check_form("bt-replay 1\nsubsystem wind_meteors 1.2\n"))
        self.assertTrue(replay.check_form(GOOD, "economy"))
        self.assertTrue(replay.check_form(GOOD.replace("call 1 UpdateWind\n", "  r Game.width 1\n")))
        self.assertTrue(replay.check_form(GOOD[:-1]))
        self.assertTrue(replay.check_form(GOOD.replace("  r Game", "  x Game")))


class Difference(unittest.TestCase):
    def test_equal_text_has_none(self):
        self.assertIsNone(replay.first_difference(GOOD, GOOD))

    def test_the_first_line_is_named(self):
        got = replay.first_difference(GOOD, GOOD.replace("main = 5", "main = 6"))
        self.assertTrue(got.startswith("line 6:"))
        self.assertTrue(replay.first_difference(GOOD, GOOD + "call 3 UpdateWind\n").startswith("line 8:"))


class Subsystems(unittest.TestCase):
    def test_every_simulation_subsystem_is_known(self):
        versions = replay.schema_versions()
        self.assertEqual(len(versions), 14)
        self.assertEqual(versions["wind_meteors"], "1.2")

    def test_the_default_is_the_subsystems_with_recordings(self):
        self.assertEqual(replay.default_subsystems(), ["wind_meteors", "economy", "targeting"])
        self.assertEqual(replay.recorded_names(["wind_meteors"]), ["console-features", "meteors-earthquake", "meteors-hailstorm"])
        self.assertEqual(replay.plan_for(["wind_meteors"], None)["meteors-hailstorm"], ["wind_meteors"])
        self.assertEqual(replay.plan_for(["economy"], ["x"]), {"x": ["economy"]})


class Compare(unittest.TestCase):
    def test_directories(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            for d in (a, b):
                (Path(d) / "wind_meteors").mkdir()
            (Path(a) / "wind_meteors" / "x.txt").write_text(GOOD)
            (Path(b) / "wind_meteors" / "x.txt").write_text(GOOD)
            self.assertEqual(replay.compare(Path(a), Path(b)), [])
            (Path(b) / "wind_meteors" / "x.txt").write_text(GOOD.replace("1024", "512"))
            self.assertEqual(len(replay.compare(Path(a), Path(b))), 1)
            (Path(b) / "wind_meteors" / "y.txt").write_text(GOOD)
            self.assertEqual(len(replay.compare(Path(a), Path(b))), 2)


if __name__ == "__main__":
    unittest.main()
