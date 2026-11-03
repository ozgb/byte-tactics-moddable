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
        assignment = rs.read_assignment()
        sub, struct, slot = [(m, st, x["name"]) for m, st in rs.seam_members() if m not in ("wind_meteors", "economy", "targeting")
                             for x in rs.parse_interface(m, st)["slots"] if x["name"] in assignment.get(m, [])][0]
        model, errors = build([row(slot, "field_read", "Game::mapWidth"), row(slot, "field_escape", "Unit::unmatched")])
        pending = model["subsystems"][sub]["pending"]
        self.assertTrue(pending["untracked"] >= 1 and pending["ruleless"] >= 1)
        self.assertEqual([e for e in errors if sub in e], [])

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


class Rules(unittest.TestCase):
    def slot(self, rows, name="UpdatePlayerEconomy", sub="economy"):
        model, errors = build(rows)
        names = ["%s.%s" % (f["object"], f["name"]) for f in model["object_fields"]]
        s = model["subsystems"][sub]["slots"][name]
        return {"reads": [names[i] for i in s["objectReads"]], "writes": [names[i] for i in s["objectWrites"]], "allOf": s["allOf"], "errors": errors}

    def test_a_write_through_a_float_pointer_lands_on_the_fields_of_its_rule(self):
        s = self.slot([row("AddIncome", "deref_write", "*float")])
        self.assertEqual(s["writes"], ["UnitResources.energyMake", "UnitResources.metalMake"])
        self.assertEqual(s["reads"], [])

    def test_a_rule_of_functions_does_not_cover_the_others(self):
        s = self.slot([row("UpdateMetalExtraction", "deref_write", "*float")], "UpdateMetalExtraction")
        self.assertEqual(s["writes"], ["Unit.extraction"])
        s = self.slot([row("UseEnergyD", "deref_write", "*float")])
        self.assertEqual(s["writes"], ["UnitResources.energyUse"])

    def test_an_alias_names_the_field_that_records_a_bit_field(self):
        s = self.slot([row("UpdatePlayerEconomy", "field_read", "Unit::cloakOn")])
        self.assertEqual(s["reads"], ["Unit.flags"])

    def test_a_plain_row_may_have_a_rule(self):
        s = self.slot([row("UpdateMetalExtraction", "field_read", "Point16::x")], "UpdateMetalExtraction")
        self.assertEqual(s["reads"], ["Unit.cell", "Unit.footprint"])

    def test_an_all_of_rule_makes_every_slot_of_the_table_a_root(self):
        s = self.slot([row("UpdateResourceSharing", "field_escape", "Game::players")], "UpdateResourceSharing")
        self.assertEqual(s["allOf"], "Player")

    def test_a_member_a_state_type_does_not_record_is_unaccounted_for(self):
        _, errors = build([row("UpdatePlayerEconomy", "field_read", "Player::noSuchMember")])
        self.assertTrue(any("economy is complete but has pending untracked" in e and "Player::noSuchMember" in e for e in errors), errors)

    def test_the_range_marks_both_fields_of_the_pair(self):
        files, _, _ = rs.generate(rs.read_rows())
        lines = files[rs.SCHEMA_H].split("\n")
        begin = [l for l in lines if l.startswith('    { "unitsBegin"')][0].rstrip(" },").split(", ")
        end = [l for l in lines if l.startswith('    { "unitsEnd"')][0].rstrip(" },").split(", ")
        self.assertTrue(int(begin[-6]) > 0 and int(begin[-5]) == 0, begin)
        self.assertTrue(int(end[-6]) == 0 and int(end[-5]) > 0, end)


class Services(unittest.TestCase):
    def test_a_declaration_the_header_lacks_is_an_error(self):
        ann = annotations()
        ann["services"][1]["declaration"] = "int __stdcall RandomInt(unsigned range);"
        with self.assertRaises(rs.Error):
            rs.generate(rs.read_rows(), ann)

    def test_a_hook_that_gives_other_arguments_is_an_error(self):
        spec = copy.deepcopy(annotations()["services"])
        spec[1]["args"].append({"mode": "int", "param": "extra"})
        self.assertTrue(any("RI_RANDOM_INT" in e for e in rs.check_service_hooks(spec)))

    def test_a_hook_with_no_spec_is_an_error(self):
        spec = [s for s in annotations()["services"] if s["id"] != "RI_GET_INT_ARG"]
        self.assertTrue(any("RI_GET_INT_ARG" in e and "no service spec" in e for e in rs.check_service_hooks(spec)))

    def test_every_service_has_a_line_in_the_recorder_and_an_answer(self):
        files, _, _ = rs.generate(rs.read_rows())
        for s in annotations()["services"]:
            self.assertIn("case %s:" % s["id"], files[rs.SERVICES_INC])
            self.assertIn("StartImport(t, %s);" % s["id"], files[rs.SERVICES_SLOTS])


class TargetingForms(unittest.TestCase):
    """The forms #669 added: unit-id fields, raw byte pointers, in-out structs, vec3 out, unit vectors and reach lists."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def test_a_pos_the_call_writes_only_on_success_is_an_out_struct_with_a_condition(self):
        _, _, model = self.generated()
        slots = model["subsystems"]["targeting"]["slots"]
        pos = [a for a in slots["GetWeaponTargetPos"]["args"] if a["mode"] == "struct"][0]
        self.assertEqual((pos["dir"], pos["when"]), ("out", "result != 0"))
        self.assertEqual([a["dir"] for a in slots["SetWeaponTargetPos"]["args"] if a["mode"] == "struct"], ["in"])

    def test_a_conditional_out_struct_is_recorded_only_when_the_result_is_nonzero(self):
        files, _, _ = self.generated()
        hooks = files[rs.HOOKS_H]
        body = hooks[hooks.index("RecTargetingGetWeaponTargetPos"):]
        body = body[:body.index("\n}\n")]
        self.assertIn('if (result != 0) ArgStruct(s_tail, "o", "pos", ROK_VEC3, a1);', body)
        self.assertNotIn('ArgStruct(s_call, "a", "pos"', body)

    def test_a_conditional_out_struct_is_compared_only_when_the_result_is_nonzero(self):
        files, _, _ = self.generated()
        runner = files[rs.SLOTS_H]
        body = runner[runner.index("void After_targeting_GetWeaponTargetPos"):]
        body = body[:body.index("\n}\n")]
        self.assertIn('if (g_result_targeting_GetWeaponTargetPos != 0) CompareStruct("pos", ROK_VEC3', body)

    def test_a_condition_on_anything_else_is_an_error(self):
        ann = annotations()
        ann["subsystems"]["targeting"]["slots"]["GetWeaponTargetPos"]["args"]["pos"]["when"] = "result == 0"
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("`when` is only `result != 0`" in e for e in errors), errors)

    def test_an_import_may_print_only_the_axes_it_reads(self):
        files, _, _ = self.generated()
        hooks = files[rs.HOOKS_H]
        body = hooks[hooks.index("RecUnitsGetGroundHeight"):]
        self.assertIn('ImportVec3Xz(line, "pos", a0);', body[:body.index("\n}\n")])
        self.assertIn("AddVec3Xz(t, a0);", files[rs.SLOTS_H])

    def test_a_vec3_out_of_an_import_is_recorded_after_the_call_and_applied_by_the_stub(self):
        files, _, _ = self.generated()
        self.assertIn('ImportOutVec3(line, "out", a1);', files[rs.HOOKS_H])
        self.assertIn('ApplyVec3(a1, "out");', files[rs.SLOTS_H])

    def test_a_unit_vector_out_carries_its_reach_into_the_table(self):
        files, _, model = self.generated()
        out = [a for a in model["subsystems"]["ai"]["slots"]["GetVisibleEnemiesInRadius"]["args"] if a["name"] == "out"][0]
        self.assertEqual((out["mode"], out["reachCount"], out["depth"]), ("units", 7, 2))
        self.assertIn("ImportOutUnits(line, \"out\", a4, kReplayReachFields + %d, 7);" % out["reachFirst"], files[rs.HOOKS_H])
        self.assertIn("ApplyUnits(a4, \"out\");", files[rs.SLOTS_H])

    def test_a_reach_deeper_than_its_depth_is_an_error(self):
        ann = annotations()
        out = ann["subsystems"]["ai"]["slots"]["GetVisibleEnemiesInRadius"]["args"]["out"]
        out["reach"] = out["reach"] + ["UnitDef.flags1", "Unit.player", "Player.unitsBegin"]
        out["depth"] = 2
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("deep and its depth says" in e for e in errors), errors)

    def test_a_reach_that_names_no_field_is_an_error(self):
        ann = annotations()
        ann["subsystems"]["ai"]["slots"]["GetVisibleEnemiesInRadius"]["args"]["out"]["reach"].append("Unit.noSuchField")
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("unknown field Unit.noSuchField" in e for e in errors), errors)

    def test_a_unit_id_field_names_the_value_that_makes_it_an_id(self):
        files, _, _ = self.generated()
        row = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "weapon0TargetUnit"')][0]
        self.assertIn("RK_UNITID", row)
        self.assertIn("-32768", row)
        ann = annotations()
        del [f for f in ann["objects"]["Unit"]["fields"] if f["name"] == "weapon1TargetUnit"][0]["when"]
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("weapon1TargetUnit" in e and "value that make it an id" in e for e in errors), errors)

    def test_a_raw_pointer_is_checked_to_be_a_pointer(self):
        files, _, _ = self.generated()
        self.assertIn("UnitDef::categories is a pointer", files[rs.SCHEMA_H])
        self.assertTrue(any(l.startswith('    { "weaponCategories2"') and "RK_RAWBYTES" in l for l in files[rs.SCHEMA_H].split("\n")))

    def test_two_subsystems_importing_one_slot_get_one_stub(self):
        files, _, _ = self.generated()
        self.assertEqual(files[rs.SLOTS_H].count("int __stdcall Imp_units_StartScriptWithArgs("), 1)

    def test_an_object_result_is_compared_by_name(self):
        files, _, _ = self.generated()
        self.assertIn("CompareObjectResult(ROK_UNIT, g_result_targeting_FindWeaponTarget);", files[rs.SLOTS_H])


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
