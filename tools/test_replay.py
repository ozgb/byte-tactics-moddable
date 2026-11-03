"""Tests for replay.py.

    uv run tools/test_replay.py
"""

import tempfile
import unittest
from pathlib import Path

import replay

GOOD = "bt-replay 1\nsubsystem wind_meteors 1.2\ncall 1 UpdateWind\ncall 2 UpdateMeteors\n  r Game.width 1024\n  i services.GameRand main = 5\n  w rng.main 0x00000001\n"


class Scenario(unittest.TestCase):
    def test_record_line_goes_last(self):
        self.assertEqual(replay.with_record("ticks 5\n"), "ticks 5\nrecord wind_meteors\n")
        self.assertEqual(replay.with_record("ticks 5"), "ticks 5\nrecord wind_meteors\n")
        self.assertEqual(replay.with_record("ticks 5\n", (3, 9)), "ticks 5\nrecord wind_meteors 3 9\n")

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
        self.assertTrue(replay.check_form("bt-replay 2\n"))
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


class Compare(unittest.TestCase):
    def test_directories(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            (Path(a) / "x.wind_meteors.txt").write_text(GOOD)
            (Path(b) / "x.wind_meteors.txt").write_text(GOOD)
            self.assertEqual(replay.compare(Path(a), Path(b)), [])
            (Path(b) / "x.wind_meteors.txt").write_text(GOOD.replace("1024", "512"))
            self.assertEqual(len(replay.compare(Path(a), Path(b))), 1)
            (Path(b) / "y.wind_meteors.txt").write_text(GOOD)
            self.assertEqual(len(replay.compare(Path(a), Path(b))), 2)


if __name__ == "__main__":
    unittest.main()
