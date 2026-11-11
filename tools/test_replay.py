"""Tests for replay.py.

    uv run tools/test_replay.py
"""

import struct
import tempfile
import unittest
from pathlib import Path

import replay

METAL_FACTOR = 0x3EAAAAAB
ENERGY_FACTOR = 0x3F000000

GOOD = "bt-replay 2\nsubsystem wind_meteors 1.2\ncall 1 UpdateWind\ncall 2 UpdateMeteors\n  r Game.width 1024\n  i services.GameRand main = 5\n  w rng.main 0x00000001\n"


def f32(bits: int) -> float:
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def f32_bits(value: float) -> int:
    return struct.unpack("<I", struct.pack("<f", value))[0]


def share_amount_bits(stock: int, keep: int, factor: int, capacity: int, partner: int) -> int:
    """ShareAmount (src/network/resource_share.h) as the x87 gives it at 53-bit precision: doubles, rounded to a float at the end."""
    product = (f32(stock) - f32(keep)) * f32(factor)
    limit = f32(capacity) - f32(partner)
    return f32_bits(product if product < limit else limit)


def float_bits(text: str) -> int:
    assert text.startswith("f32:0x"), text
    return int(text[6:], 16)


def share_transfers(text: str) -> list[dict]:
    """Every players.TransferMetal and players.TransferEnergy import of a recording, with the amount the model gives for the values the
    recording knows at that point (the last r or w line of each field). The metal share passes through TransferEnergy and the energy share
    through TransferMetal, as UpdateResourceSharing calls them; the factors are 1/3 and 1/2."""
    known: dict[str, str] = {}
    found = []
    tick = None
    for line in text.split("\n"):
        words = line.strip().split(" ")
        if line.startswith("call "):
            tick = int(words[1])
        elif words[0] in ("r", "w") and len(words) == 3:
            known[words[1]] = words[2]
        elif words[0] == "i" and words[1] in ("players.TransferMetal", "players.TransferEnergy"):
            source, target = int(words[2]), int(words[3])
            kind, factor = ("metal", METAL_FACTOR) if words[1] == "players.TransferEnergy" else ("energy", ENERGY_FACTOR)
            names = [f"Player#{source}.{kind}", f"Player#{source}.share{kind.capitalize()}", f"Player#{target}.{kind}Capacity", f"Player#{target}.{kind}"]
            record = {"tick": tick, "import": words[1], "recorded": float_bits(words[4]), "inputs": names}
            if all(n in known for n in names):
                stock, keep, capacity, partner = (float_bits(known[n]) for n in names)
                record["expected"] = share_amount_bits(stock, keep, factor, capacity, partner)
                record["limited"] = f32(capacity) - f32(partner) <= (f32(stock) - f32(keep)) * f32(factor)
            found.append(record)
    return found


def share_problems(text: str) -> list[str]:
    """The transfers whose recorded amount is not the model's, bit for bit."""
    return [f"tick {t['tick']} {t['import']}: recorded 0x{t['recorded']:08x}, " + (f"model 0x{t['expected']:08x}" if "expected" in t else f"inputs unknown {t['inputs']}")
            for t in share_transfers(text) if t.get("expected") != t["recorded"]]


SHARING = replay.RECORDINGS / "economy" / "mp-sharing.txt"


class Sharing(unittest.TestCase):
    def test_the_model_is_resource_share_h(self):
        self.assertEqual(f32(share_amount_bits(f32_bits(1000.0), f32_bits(100.0), METAL_FACTOR, f32_bits(1000.0), f32_bits(0.0))), 300.0)
        self.assertEqual(f32(share_amount_bits(f32_bits(1000.0), f32_bits(100.0), ENERGY_FACTOR, f32_bits(1000.0), f32_bits(0.0))), 450.0)
        self.assertEqual(f32(share_amount_bits(f32_bits(1000.0), f32_bits(100.0), ENERGY_FACTOR, f32_bits(1000.0), f32_bits(900.0))), 100.0)

    def test_the_recording_reaches_both_transfers_and_every_amount_is_the_models(self):
        text = SHARING.read_text()
        transfers = share_transfers(text)
        self.assertGreaterEqual(sum(t["import"] == "players.TransferMetal" for t in transfers), 3)
        self.assertGreaterEqual(sum(t["import"] == "players.TransferEnergy" for t in transfers), 3)
        self.assertEqual(share_problems(text), [])

    def test_the_recording_reaches_both_limits_of_the_amount(self):
        transfers = share_transfers(SHARING.read_text())
        for name in ("players.TransferMetal", "players.TransferEnergy"):
            self.assertEqual({t["limited"] for t in transfers if t["import"] == name}, {True, False}, name)

    def test_an_amount_one_ulp_off_is_a_problem(self):
        text = SHARING.read_text()
        for name in ("players.TransferMetal", "players.TransferEnergy"):
            line = next(l for l in text.split("\n") if l.startswith(f"  i {name} "))
            words = line.split(" ")
            recorded = float_bits(words[6])
            for step in (1, -1):
                words[6] = f"f32:0x{recorded + step:08x}"
                self.assertEqual(len(share_problems(text.replace(line, " ".join(words), 1))), 1, (name, step))

    def test_an_unknown_input_is_a_problem(self):
        self.assertEqual(len(share_problems("call 1 UpdateResourceSharing\n  i players.TransferMetal 0 1 f32:0x42c80000 1\n")), 1)


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


class Subsystems(unittest.TestCase):
    def test_every_simulation_subsystem_is_known(self):
        versions = replay.schema_versions()
        self.assertEqual(len(versions), 14)
        self.assertEqual(versions["wind_meteors"], "1.2")

    def test_the_default_is_the_subsystems_with_recordings(self):
        self.assertEqual(replay.default_subsystems(), ["wind_meteors", "economy", "weapons", "targeting", "map"])
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
