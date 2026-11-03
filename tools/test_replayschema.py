"""Tests for replayschema.py.

    uv run tools/test_replayschema.py
"""

import copy
import json
import unittest

import replayschema as rs


def row(function, kind, target):
    return {"address": "", "function": function, "file": "", "kind": kind, "target": target}


def annotations():
    return copy.deepcopy(rs.load_annotations())


def build(rows, ann=None, assignment=None):
    return rs.build(ann or annotations(), rows, assignment or rs.read_assignment())


class Interface(unittest.TestCase):
    def test_slots_come_from_the_header(self):
        iface = rs.parse_interface("wind_meteors", "WindMeteorsSeam")
        names = [s["name"] for s in iface["slots"]]
        self.assertEqual(names[0], "UpdateWind")
        self.assertEqual(len(names), 12)
        self.assertEqual(iface["version"], [1, 2])
        update = iface["slots"][0]
        self.assertEqual((update["conv"], update["ret"], update["params"]), ("__cdecl", "void", []))

    def test_every_member_of_seams_parses(self):
        for sub, struct in rs.seam_members():
            iface = rs.parse_interface(sub, struct)
            self.assertTrue(iface["slots"], sub)
            for s in iface["slots"]:
                for p in s["params"]:
                    self.assertTrue(p["type"] and p["name"], (sub, s["name"]))

    def test_template_parameters_stay_whole(self):
        self.assertEqual(rs.split_params("int a, std::vector<Unit*, std::allocator<Unit*> >* out"),
                         ["int a", "std::vector<Unit*, std::allocator<Unit*> >* out"])


class Sets(unittest.TestCase):
    def slot(self, rows, name, sub="wind_meteors"):
        model, errors = build(rows)
        return model["subsystems"][sub]["slots"][name]

    def test_reads_and_writes_follow_the_row_kinds(self):
        rows = [row("EnableMeteors", "global_write", "g_meteorsEnabled"),
                row("SaveMeteors", "global_read", "g_meteorActive"),
                row("SetMeteorParams", "global_maybe_written", "g_meteorWeaponName")]
        self.assertEqual(self.slot(rows, "EnableMeteors")["writes"], ["SimGlobals.g_meteorsEnabled"])
        self.assertEqual(self.slot(rows, "SaveMeteors")["reads"], ["SimGlobals.g_meteorActive"])
        s = self.slot(rows, "SetMeteorParams")
        self.assertEqual((s["reads"], s["writes"]), (["SimGlobals.g_meteorWeaponName"],) * 2)

    def test_a_draw_adds_the_generator_words(self):
        s = self.slot([row("UpdateMeteors", "call", "GameRand")], "UpdateMeteors")
        self.assertEqual(s["reads"], rs.RNG_FIELDS)
        self.assertEqual(s["writes"], rs.RNG_FIELDS)

    def test_other_functions_do_not_count(self):
        s = self.slot([row("Elsewhere", "global_write", "g_meteorActive")], "EnableMeteors")
        self.assertEqual((s["reads"], s["writes"]), ([], []))

    def test_object_fields_join_the_object_sets(self):
        model, _ = build([row("UpdatePlayerEconomy", "field_read", "Player::energy"), row("UpdatePlayerEconomy", "field_write", "Unit::health")])
        s = model["subsystems"]["economy"]["slots"]["UpdatePlayerEconomy"]
        names = ["%s.%s" % (f["object"], f["name"]) for f in model["object_fields"]]
        self.assertEqual([names[i] for i in s["objectReads"]], ["Player.energy"])
        self.assertEqual([names[i] for i in s["objectWrites"]], ["Unit.health"])


class Accounting(unittest.TestCase):
    def test_an_untracked_game_member_is_an_error_for_a_complete_subsystem(self):
        _, errors = build([row("UpdateMeteors", "field_read", "Game::mapWidth")])
        self.assertTrue(any("wind_meteors is complete but has pending untracked" in e and "Game::mapWidth" in e for e in errors), errors)

    def test_an_exempt_target_is_not(self):
        _, errors = build([row("UpdateMeteors", "global_read", "g_game")])
        self.assertEqual([e for e in errors if "pending" in e], [])

    def test_a_pending_subsystem_lists_what_is_left(self):
        model, errors = build([row("UpdatePlayerEconomy", "field_read", "Game::mapWidth"), row("UpdatePlayerEconomy", "field_escape", "Unit::extraction")])
        pending = model["subsystems"]["economy"]["pending"]
        self.assertEqual((pending["untracked"], pending["ruleless"]), (1, 1))
        self.assertEqual([e for e in errors if "economy" in e], [])

    def test_a_row_with_no_rule_is_an_error_for_a_complete_subsystem(self):
        _, errors = build([row("UpdateMeteors", "field_escape", "Game::unmatched")])
        self.assertTrue(any("pending rules" in e for e in errors), errors)

    def test_a_rule_that_matches_nothing_is_an_error(self):
        _, errors = build([])
        self.assertTrue(any("matches no row" in e for e in errors), errors)

    def test_a_pointer_argument_without_an_annotation_is_pending(self):
        ann = annotations()
        del ann["subsystems"]["wind_meteors"]["slots"]["SetMeteorParams"]["args"]
        _, errors = build([], ann)
        self.assertTrue(any("SetMeteorParams(params)" in e for e in errors), errors)


class Committed(unittest.TestCase):
    def test_the_data_is_current_and_complete(self):
        files, errors, model = rs.generate(rs.read_rows())
        self.assertEqual(errors, [])
        for path, want in files.items():
            self.assertEqual(path.read_text(), want, path)

    def test_every_simulation_subsystem_is_covered(self):
        files, _, model = rs.generate(rs.read_rows())
        written = json.loads(files[rs.JSON_OUT])
        self.assertEqual(sorted(written["subsystems"]), sorted(m[0] for m in rs.seam_members()))
        self.assertEqual(len(written["subsystems"]), 14)

    def test_every_manifest_slot_has_sets(self):
        manifest = json.loads((rs.ROOT / "wasm/modules.json").read_text())
        exports = [m for m in manifest["modules"] if m["name"] == "wind_meteors"][0]["exports"]
        _, _, model = rs.generate(rs.read_rows())
        self.assertEqual(sorted(exports), sorted(model["subsystems"]["wind_meteors"]["slots"]))


if __name__ == "__main__":
    unittest.main()
