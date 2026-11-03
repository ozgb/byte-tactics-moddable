"""Tests for replayschema.py.

    uv run tools/test_replayschema.py
"""

import json
import unittest

import replayschema as rs


def row(function, kind, target):
    return {"address": "", "function": function, "file": "", "kind": kind, "target": target}


class SlotSets(unittest.TestCase):
    def test_reads_and_writes_follow_the_row_kinds(self):
        rows = [row("EnableMeteors", "global_write", "g_meteorsEnabled"),
                row("SaveMeteors", "global_read", "g_meteorActive"),
                row("SetMeteorParams", "global_maybe_written", "g_meteorWeaponName")]
        sets = rs.slot_sets(rows)
        self.assertEqual(sets["EnableMeteors"], ([], ["SimGlobals.g_meteorsEnabled"]))
        self.assertEqual(sets["SaveMeteors"], (["SimGlobals.g_meteorActive"], []))
        self.assertEqual(sets["SetMeteorParams"], (["SimGlobals.g_meteorWeaponName"], ["SimGlobals.g_meteorWeaponName"]))

    def test_a_draw_adds_the_generator_words(self):
        sets = rs.slot_sets([row("UpdateMeteors", "call", "GameRand")])
        self.assertEqual(sets["UpdateMeteors"][0], rs.RNG_FIELDS)
        self.assertEqual(sets["UpdateMeteors"][1], rs.RNG_FIELDS)

    def test_other_functions_do_not_count(self):
        sets = rs.slot_sets([row("Elsewhere", "global_write", "g_meteorActive")])
        self.assertTrue(all(r == [] and w == [] for r, w in sets.values()))


class Unaccounted(unittest.TestCase):
    def test_an_untracked_game_member_is_named(self):
        rows = [row("UpdateMeteors", "field_read", "Game::mapWidth"), row("UpdateMeteors", "field_read", "Game::ticks")]
        self.assertEqual(rs.unaccounted(rows), ["Game::mapWidth"])

    def test_an_exempt_target_is_not(self):
        self.assertEqual(rs.unaccounted([row("UpdateMeteors", "global_read", "g_game")]), [])

    def test_rows_of_other_functions_are_ignored(self):
        self.assertEqual(rs.unaccounted([row("Elsewhere", "global_read", "g_other")]), [])


class Committed(unittest.TestCase):
    def test_the_data_is_current_and_complete(self):
        text, schema_json, errors = rs.generate(rs.read_rows())
        self.assertEqual(errors, [])
        self.assertEqual(rs.OUT.read_text(), text)
        self.assertEqual(rs.JSON_OUT.read_text(), schema_json)
        model = json.loads(schema_json)
        self.assertEqual(sorted(model["slots"]), sorted(rs.SLOT_FUNCTIONS))

    def test_every_manifest_slot_has_sets(self):
        manifest = json.loads((rs.ROOT / "wasm/modules.json").read_text())
        exports = [m for m in manifest["modules"] if m["name"] == "wind_meteors"][0]["exports"]
        self.assertEqual(sorted(exports), sorted(rs.SLOT_FUNCTIONS))


if __name__ == "__main__":
    unittest.main()
