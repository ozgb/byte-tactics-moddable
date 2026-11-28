"""Tests for replay.py.

    uv run tools/test_replay.py
"""

import tempfile
import unittest
from pathlib import Path
from unittest import mock

import coupling
import replay

GOOD = "bt-replay 2\nsubsystem wind_meteors 3.1\ncall 1 UpdateWind\ncall 2 UpdateMeteors\n  r Game.width 1024\n  i services.GameRand main = 5\n  w rng.main 0x00000001\n"


class Scenario(unittest.TestCase):
    def test_record_line_goes_last(self):
        self.assertEqual(replay.with_record("ticks 5\n", "wind_meteors"), "ticks 5\nrecord wind_meteors\n")
        self.assertEqual(replay.with_record("ticks 5", ["wind_meteors"]), "ticks 5\nrecord wind_meteors\n")
        self.assertEqual(replay.with_record("ticks 5\n", ["wind_meteors"], (3, 9)), "ticks 5\nrecord wind_meteors 3 9\n")

    def test_several_subsystems_get_a_line_each(self):
        self.assertEqual(replay.with_record("ticks 5\n", ["targeting", "wind_meteors"], (3, 9)),
                         "ticks 5\nrecord targeting 3 9\nrecord wind_meteors 3 9\n")

    def test_a_window_can_differ_per_subsystem(self):
        self.assertEqual(replay.with_record("ticks 5\n", ["targeting", "wind_meteors"], {"targeting": (3, 9)}),
                         "ticks 5\nrecord targeting 3 9\nrecord wind_meteors\n")

    def test_the_stored_windows_name_a_recording_that_exists(self):
        for key, window in replay.stored_windows().items():
            subsystem, scenario = key.split("/")
            self.assertTrue(replay.stored_recording(subsystem, scenario).is_file(), key)
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
        self.assertTrue(replay.check_form(GOOD, "targeting"))
        self.assertTrue(replay.check_form(GOOD.replace("call 1 UpdateWind\n", "  r Game.width 1\n")))
        self.assertTrue(replay.check_form(GOOD[:-1]))
        self.assertTrue(replay.check_form(GOOD.replace("  r Game", "  x Game")))
        loaded = GOOD.replace("call 1 UpdateWind\n", 'load cob UnitDef#2 "scripts\\\\A.COB" [4]\n  x 0:01020304\ncall 1 UpdateWind\n')
        self.assertEqual(replay.check_form(loaded), [])
        self.assertTrue(replay.check_form(loaded.replace("  x 0:", "  y 0:")))
        bound = loaded.replace("  x 0:01020304\n", "  x 0:01020304\n  bind 32 7 FrameTable#1 0 =0 0\n")
        self.assertEqual(replay.check_form(bound), [])
        cached = loaded.replace("call 1 UpdateWind\n", "cache debris 100 0\n  block 0 100 null\ncall 1 UpdateWind\n")
        self.assertEqual(replay.check_form(cached), [])
        self.assertTrue(replay.check_form(cached.replace("  block 0 100 null\n", "  x 0:01\n")))
        self.assertTrue(replay.check_form(GOOD.replace("call 1 UpdateWind\n", "  block 0 100 null\ncall 1 UpdateWind\n")))
        self.assertTrue(replay.check_form(GOOD.replace("call 1 UpdateWind\n", "  bind 32 7 null 0 null 0\ncall 1 UpdateWind\n")))
        self.assertTrue(replay.check_form(GOOD.replace("call 1 UpdateWind\n", "  x 0:01\ncall 1 UpdateWind\n")))
        self.assertTrue(replay.check_form(loaded.replace("call 1 UpdateWind\n", "call 1 UpdateWind\nload cob UnitDef#2 \"a\" [1]\n")))
        held = GOOD.replace("  i services", "  a done callback#1\n  c callback#1.Complete 5\n  i services")
        self.assertEqual(replay.check_form(held), [])
        self.assertTrue(replay.check_form(held.replace("  c callback", "  c")))
        self.assertTrue(replay.check_form(held.replace("  c callback", "  cx callback")))


class Difference(unittest.TestCase):
    def test_equal_text_has_none(self):
        self.assertIsNone(replay.first_difference(GOOD, GOOD))

    def test_the_first_line_is_named(self):
        got = replay.first_difference(GOOD, GOOD.replace("main = 5", "main = 6"))
        self.assertTrue(got.startswith("line 6:"))
        self.assertTrue(replay.first_difference(GOOD, GOOD + "call 3 UpdateWind\n").startswith("line 8:"))


def frame(index: int, width: int = 19, number: int = 1) -> str:
    return (f"  i services.GetGafFrame FrameTable#1 {index} = GafFrame#{number}\n  o GafFrame#{number}.width {width}\n"
            f"  o GafFrame#{number}.pixelsOrLayers alloc#2\n")


def freed(frames: str, pos: str = "(10,77,30)") -> str:
    return ("bt-replay 2\nsubsystem los_radar 1.0\ncall 1 Scan\n  a size 8\n" + frames
            + f"  i los_radar.GetMapCellAtPosition pos={pos} = Cell#5\n  w Projectile#0.positionX 10\n  o result 1\ncall 2 Scan\n  o result 0\n")


class Freed(unittest.TestCase):
    def test_gaf_frames_in_any_order_and_count_are_equal(self):
        stored = freed(frame(4) + frame(5))
        for played in (freed(frame(5) + frame(4)), freed(frame(4) + frame(4) + frame(5)), freed(frame(4)), freed("")):
            self.assertIsNone(replay.first_difference(stored, played))
            self.assertEqual(replay.divergence(stored, played), {"status": "equal"})

    def test_frames_already_named_are_free_but_new_frames_keep_their_order(self):
        stored = freed(frame(4, number=1) + frame(5, number=2) + frame(4, number=1) + frame(5, number=2))
        again = freed(frame(4, number=1) + frame(5, number=2) + frame(5, number=2) + frame(4, number=1))
        self.assertIsNone(replay.first_difference(stored, again))
        swapped = freed(frame(5, number=1) + frame(4, number=2))
        self.assertIn("query services.GetGafFrame at line 5", replay.first_difference(stored, swapped))

    def test_a_gaf_frame_with_unknown_arguments_differs_at_the_query(self):
        stored = freed(frame(4))
        played = freed(frame(4) + frame(9))
        self.assertIn("query services.GetGafFrame at line 8", replay.first_difference(stored, played))
        found = replay.divergence(stored, played)
        self.assertEqual((found["status"], found["line"], found["tick"], found["slot"], found["kind"], found["field"]), ("differs", 8, 1, "Scan", "i", "services.GetGafFrame"))

    def test_a_changed_gaf_frame_result_differs(self):
        stored = freed(frame(4))
        played = freed(frame(4, width=20))
        self.assertIn("differs", replay.first_difference(stored, played))
        self.assertEqual(replay.divergence(stored, played)["status"], "differs")

    def test_the_y_of_a_cell_position_is_free_but_not_x_or_z(self):
        stored = freed("")
        self.assertIsNone(replay.first_difference(stored, freed("", "(10,-5,30)")))
        self.assertTrue(replay.first_difference(stored, freed("", "(10,77,31)")).startswith("line 5:"))
        self.assertTrue(replay.first_difference(stored, freed("", "(11,77,30)")).startswith("line 5:"))

    def test_other_differences_are_reported_as_before(self):
        stored = freed(frame(4))
        played = freed(frame(4) + frame(4)).replace("  o result 0", "  o result 1")
        self.assertTrue(replay.first_difference(stored, played).startswith("line 15:"))
        self.assertEqual(replay.divergence(stored, played)["tick"], 2)
        self.assertTrue(replay.first_difference(stored, stored + "call 3 Scan\n").startswith("line 13:"))


class Subsystems(unittest.TestCase):
    def test_every_simulation_subsystem_is_known(self):
        versions = replay.schema_versions()
        self.assertEqual(set(versions), set(coupling.SIM_SUBSYSTEMS) | {"files", "network"})
        self.assertEqual(versions["wind_meteors"], "3.1")

    def test_the_default_is_the_subsystems_with_recordings(self):
        self.assertEqual(replay.default_subsystems(), ["wind_meteors", "economy", "features", "ai", "pathfinding", "weapons", "targeting", "los_radar", "orders", "units", "players", "map", "scripts", "movement", "mission"])
        self.assertEqual(replay.recorded_names(["wind_meteors"]), ["console-features", "meteors-earthquake", "meteors-hailstorm"])
        self.assertEqual(replay.plan_for(["wind_meteors"], None)["meteors-hailstorm"], ["wind_meteors"])
        self.assertEqual(replay.plan_for(["targeting"], ["x"]), {"x": ["targeting"]})


class Long(unittest.TestCase):
    """A recording in a directory below tests/replay/<subsystem>/ (long/) is checked, recorded and compared at its path."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name) / "replay"
        (self.root / "mission/long").mkdir(parents=True)
        (self.root / "mission/.scenarios").mkdir()
        for name in ("mission/console-ilose.txt", "mission/long/mission-build-unit.txt", "mission/.scenarios/x.txt"):
            (self.root / name).write_text(GOOD)
        self.patches = [mock.patch.object(replay, "RECORDINGS", self.root), mock.patch.object(replay, "PLAYTESTS", self.root / "playtests.json"),
                        mock.patch.object(replay, "WINDOWS", self.root / "windows.json")]
        for p in self.patches:
            p.start()

    def tearDown(self):
        for p in self.patches:
            p.stop()
        self.tmp.cleanup()

    def test_the_stored_recordings_include_long(self):
        self.assertEqual([replay.recording_name(p) for p in replay.scenario_recordings("mission")], ["mission/console-ilose", "mission/long/mission-build-unit"])
        self.assertEqual(replay.plan_for(["mission"], None), {"console-ilose": ["mission"], "mission-build-unit": ["mission"]})
        self.assertEqual(replay.stored_recording("mission", "mission-build-unit"), self.root / "mission/long/mission-build-unit.txt")
        self.assertEqual(replay.stored_recording("mission", "none"), self.root / "mission/none.txt")

    def test_a_play_test_recording_under_long_is_left_out(self):
        (self.root / "playtests.json").write_text('{"mission/long/mission-build-unit": "playtest.py"}')
        self.assertEqual([replay.recording_name(p) for p in replay.scenario_recordings("mission")], ["mission/console-ilose"])

    def test_two_stored_recordings_of_one_scenario_are_an_error(self):
        (self.root / "mission/console-ilose.txt").rename(self.root / "mission/mission-build-unit.txt")
        with self.assertRaisesRegex(replay.Error, "2 stored recordings of mission-build-unit"):
            replay.stored_recording("mission", "mission-build-unit")

    def test_a_long_recording_is_written_at_its_path(self):
        out = Path(self.tmp.name) / "out"
        with mock.patch.object(replay, "play_recording", lambda *a: {"mission": GOOD}), mock.patch.object(replay, "check_form", lambda *a: []), \
                mock.patch.object(replay, "scenario_path", lambda name: Path(name + ".txt")):
            replay.record(Path("x.exe"), {"mission-build-unit": ["mission"], "console-ilose": ["mission"]}, None, False, None, out, 1)
        self.assertEqual(sorted(p.relative_to(out).as_posix() for p in out.rglob("*.txt")), ["mission/console-ilose.txt", "mission/long/mission-build-unit.txt"])

    def test_compare_sees_long(self):
        other = Path(self.tmp.name) / "other"
        (other / "mission").mkdir(parents=True)
        (other / "mission/console-ilose.txt").write_text(GOOD)
        self.assertEqual(replay.compare(self.root, other), [f"only in {self.root}: mission/long/mission-build-unit.txt"])


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
