"""Tests for replayschema.py.

    uv run tools/test_replayschema.py
"""

import copy
import csv
import json
import re
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import replayschema as rs


def row(function, kind, target):
    return {"address": "", "function": function, "file": "", "kind": kind, "target": target}


def annotations():
    return copy.deepcopy(rs.load_annotations(fixtures=True))


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
        sub, struct, slot = [(m, st, x["name"]) for m, st in rs.seam_members() if m not in ("wind_meteors", "economy", "targeting", "map")
                             for x in rs.parse_interface(m, st)["slots"] if x["name"] in assignment.get(m, [])][0]
        model, errors = build([row(slot, "field_read", "Game::mapWidth"), row(slot, "field_escape", "Unit::unmatched")])
        pending = model["subsystems"][sub]["pending"]
        self.assertTrue(pending["untracked"] >= 1 and pending["ruleless"] >= 1)
        self.assertEqual([e for e in errors if e.startswith(sub)], [])

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


class Unrecorded(unittest.TestCase):
    def unannotated(self):
        ann = annotations()
        del ann["subsystems"]["wind_meteors"]["slots"]["SetMeteorParams"]["args"]
        return ann

    def test_a_slot_with_pending_work_passes_only_when_it_is_not_recorded(self):
        ann = self.unannotated()
        _, errors = build([], ann)
        self.assertTrue(any("SetMeteorParams(params)" in e for e in errors), errors)
        ann["subsystems"]["wind_meteors"]["slots"]["SetMeteorParams"].update({"record": False, "reason": "load class"})
        model, errors = build([], ann)
        self.assertEqual([e for e in errors if "matches no row" not in e and not e.startswith("map")], [])
        spec = model["subsystems"]["wind_meteors"]
        self.assertNotIn("SetMeteorParams", spec["slots"])
        self.assertEqual(spec["unrecorded"], {"SetMeteorParams": {"reason": "load class", "native": "SetMeteorParams"}})
        self.assertNotIn(("wind_meteors", "SetMeteorParams"), model["slot_list"])

    def test_the_native_table_keeps_the_slot_in_its_place(self):
        ann = self.unannotated()
        ann["subsystems"]["wind_meteors"]["slots"]["SetMeteorParams"].update({"record": False, "reason": "#711"})
        model, _ = build([], ann)
        rs.attach_raw_params(model)
        text = rs.render_slots(ann, model)
        table = text.split("g_native_wind_meteors = {")[1].split("};")[0]
        order = [s["name"] for s in rs.parse_interface("wind_meteors", model["subsystems"]["wind_meteors"]["struct"])["slots"]]
        self.assertEqual([x.strip(" ,") for x in table.split("\n") if x.strip()], [annotations()["subsystems"]["wind_meteors"]["slots"].get(n, {}).get("native", n) for n in order])

    def test_a_slot_that_is_not_recorded_needs_a_reason(self):
        ann = annotations()
        ann["subsystems"]["wind_meteors"]["slots"]["SetMeteorParams"]["record"] = False
        _, errors = build([], ann)
        self.assertTrue(any("SetMeteorParams is not recorded and has no `reason`" in e for e in errors), errors)

    def test_a_reason_on_a_recorded_slot_is_an_error(self):
        ann = annotations()
        ann["subsystems"]["wind_meteors"]["slots"]["SetMeteorParams"]["reason"] = "load class"
        _, errors = build([], ann)
        self.assertTrue(any("SetMeteorParams has a reason and is recorded" in e for e in errors), errors)

    def imported(self):
        ann = annotations()
        ann["subsystems"]["weapons"]["slots"]["SpawnProjectile"].update({"record": False, "reason": "#711"})
        return ann

    def test_an_unrecorded_slot_that_a_complete_subsystem_imports_keeps_its_import_hook(self):
        ann = self.imported()
        files, errors, model = rs.generate(rs.read_rows(), ann)
        self.assertEqual([e for e in errors if "matches no row" not in e and not e.startswith("map")], [])
        weapons = model["subsystems"]["weapons"]
        self.assertNotIn("SpawnProjectile", weapons["slots"])
        kept = weapons["unrecorded"]["SpawnProjectile"]
        self.assertEqual([a["mode"] for a in kept["args"]], ["weapon", "vec3", "vec3", "scalar"])
        self.assertEqual((kept["reads"], kept["writes"], kept["functions"]), ([], [], []))
        hooks = files[rs.HOOKS_H]
        self.assertIn("RecWeaponsSpawnProjectile", hooks)
        self.assertIn("SlotScope scope(RSL_WEAPONS_SPAWNPROJECTILE, SUB_WEAPONS, roots, false);", hooks)
        self.assertIn('line.Add("  i weapons.SpawnProjectile");', hooks)
        self.assertIn("s_rec_weapons.SpawnProjectile = RecWeaponsSpawnProjectile;", hooks)
        self.assertNotIn("on[SUB_WEAPONS] ||", hooks.split("s_rec_weapons.SpawnProjectile")[0].rsplit("if (", 1)[1])

    def test_an_imported_slot_that_is_not_recorded_has_no_recording_slot(self):
        files, _, model = rs.generate(rs.read_rows(), self.imported())
        spec = model["subsystems"]["weapons"]
        self.assertNotIn(("weapons", "SpawnProjectile"), model["slot_list"])
        self.assertEqual(spec["slots"].keys() & {"SpawnProjectile"}, set())
        schema = files[rs.SCHEMA_H]
        self.assertIn("RSL_WEAPONS_SPAWNPROJECTILE,", schema)
        self.assertIn('{ "SpawnProjectile", SUB_WEAPONS, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0 },', schema)

    def test_a_missing_argument_annotation_on_an_imported_unrecorded_slot_is_reported(self):
        ann = self.unannotated()
        ann["subsystems"]["wind_meteors"]["slots"]["SetMeteorParams"].update({"record": False, "reason": "#711"})
        _, errors = build([], ann)
        self.assertEqual([e for e in errors if "pending" in e], [])
        ann["subsystems"]["economy"]["imports"].append("wind_meteors.SetMeteorParams")
        _, errors = build([], ann)
        self.assertTrue(any("wind_meteors is complete but has pending args" in e and "SetMeteorParams(params)" in e for e in errors), errors)


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
        self.assertEqual(s["allOf"], ["Player"])

    def test_a_member_a_state_type_does_not_record_is_unaccounted_for(self):
        _, errors = build([row("UpdatePlayerEconomy", "field_read", "Player::noSuchMember")])
        self.assertTrue(any("economy is complete but has pending untracked" in e and "Player::noSuchMember" in e for e in errors), errors)

    def test_the_range_marks_both_fields_of_the_pair(self):
        files, _, _ = rs.generate(rs.read_rows())
        lines = files[rs.SCHEMA_H].split("\n")
        begin = [l for l in lines if l.startswith('    { "unitsBegin"')][0].rstrip(" },").split(", ")
        end = [l for l in lines if l.startswith('    { "unitsEnd"')][0].rstrip(" },").split(", ")
        self.assertTrue(int(begin[-5]) > 0 and int(begin[-4]) == 0, begin)
        self.assertTrue(int(end[-5]) == 0 and int(end[-4]) > 0, end)


class WritesThroughPointers(unittest.TestCase):
    """A slot note may add `writes` as well as `reads`, and a `fields` rule may name block fields."""

    def slot(self, ann, rows, name="UpdateWind"):
        model, errors = build(rows, ann)
        return model["subsystems"]["wind_meteors"]["slots"][name], errors

    def test_a_slot_note_adds_block_fields_to_the_write_set(self):
        ann = annotations()
        ann["subsystems"]["wind_meteors"]["slots"]["UpdateWind"]["writes"] = ["Game.visibilityMask", "Game.width"]
        s, errors = self.slot(ann, [])
        self.assertEqual([e for e in errors if "UpdateWind" in e], [])
        self.assertIn("Game.visibilityMask", s["writes"])
        self.assertIn("Game.width", s["writes"])
        self.assertNotIn("Game.visibilityMask", s["reads"])

    def test_the_write_set_keeps_the_order_of_the_field_table(self):
        ann = annotations()
        ann["subsystems"]["wind_meteors"]["slots"]["UpdateWind"]["writes"] = ["Game.visibilityMask", "Game.width"]
        s, _ = self.slot(ann, [])
        order = [f["name"] for f in build([], ann)[0]["fields"]]
        self.assertEqual(s["writes"], sorted(s["writes"], key=order.index))

    def test_a_slot_note_write_must_be_a_block_field(self):
        ann = annotations()
        ann["subsystems"]["wind_meteors"]["slots"]["UpdateWind"]["writes"] = ["Game.nowhere"]
        _, errors = self.slot(ann, [])
        self.assertTrue(any("wind_meteors.UpdateWind: the extra write Game.nowhere is no block field" in e for e in errors), errors)

    def test_a_slot_note_read_is_still_checked(self):
        ann = annotations()
        ann["subsystems"]["wind_meteors"]["slots"]["UpdateWind"]["reads"] = ["Game.nowhere"]
        _, errors = self.slot(ann, [])
        self.assertTrue(any("wind_meteors.UpdateWind: the extra read Game.nowhere is no block field" in e for e in errors), errors)

    def test_a_fields_rule_names_block_fields_in_reads_and_writes(self):
        ann = annotations()
        ann["rules"].append({"kind": "deref_write", "target": "*short", "rule": "fields", "reads": ["Game.width"], "writes": ["Game.visibilityMask", "Player.energy"],
                             "functions": ["UpdateMeteors"], "note": "test"})
        s, errors = self.slot(ann, [row("UpdateMeteors", "deref_write", "*short")], "UpdateMeteors")
        self.assertEqual([e for e in errors if "unknown field" in e or "UpdateMeteors" in e], [])
        self.assertIn("Game.visibilityMask", s["writes"])
        self.assertIn("Game.width", s["reads"])
        model, _ = build([row("UpdateMeteors", "deref_write", "*short")], ann)
        names = ["%s.%s" % (f["object"], f["name"]) for f in model["object_fields"]]
        self.assertIn("Player.energy", [names[i] for i in s["objectWrites"]])

    def test_a_fields_rule_still_rejects_an_unknown_name(self):
        ann = annotations()
        ann["rules"].append({"kind": "deref_write", "target": "*short", "rule": "fields", "writes": ["Game.nowhere"], "functions": ["UpdateMeteors"], "note": "test"})
        _, errors = self.slot(ann, [row("UpdateMeteors", "deref_write", "*short")], "UpdateMeteors")
        self.assertTrue(any("the rule for deref_write *short names the unknown field Game.nowhere" in e for e in errors), errors)

    def test_the_committed_schema_is_unchanged_by_the_forms(self):
        files, errors, _ = rs.generate(rs.read_rows())
        self.assertEqual(errors, [])
        for path, text in files.items():
            self.assertEqual(path.read_text(), text, path)


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

    def test_a_unit_id_field_may_name_the_value_that_makes_it_an_id(self):
        files, _, _ = self.generated()
        row = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "weapon0TargetUnit"')][0]
        self.assertIn("RK_UNITID", row)
        self.assertIn("-32768", row)
        ann = annotations()
        del [f for f in ann["objects"]["Unit"]["fields"] if f["name"] == "weapon1TargetUnit"][0]["when"]
        _, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])

    def test_a_category_mask_is_a_ref_to_an_object_of_the_block(self):
        files, _, _ = self.generated()
        self.assertIn("UnitDef::categories has the size the schema says", files[rs.SCHEMA_H])
        self.assertTrue(any(l.startswith('    { "weaponCategories2"') and "RK_REF, ROK_UNITTYPESET" in l for l in files[rs.SCHEMA_H].split("\n")))
        self.assertTrue(any(l.startswith('    { "bits"') and "RK_BYTES" in l for l in files[rs.SCHEMA_H].split("\n")))

    def test_two_subsystems_importing_one_slot_get_one_stub(self):
        files, _, _ = self.generated()
        self.assertEqual(files[rs.SLOTS_H].count("int __stdcall Imp_units_StartScriptWithArgs("), 1)

    def test_an_object_result_is_compared_by_name(self):
        files, _, _ = self.generated()
        self.assertIn("CompareObjectResult(ROK_UNIT, g_result_targeting_FindWeaponTarget);", files[rs.SLOTS_H])


class UnannotatedTypes(unittest.TestCase):
    ROW = row("UpdatePlayerEconomy", "field_read", "OpenHeap::count")

    def pending(self, rows, ann=None):
        _, errors = build(rows, ann)
        return [e for e in errors if "pending types" in e]

    def test_a_field_row_on_a_type_that_is_no_kind_is_an_error_naming_the_row(self):
        errors = self.pending([self.ROW])
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("economy is complete but has pending types", errors[0])
        self.assertIn("field_read OpenHeap::count", errors[0])

    def test_a_pending_subsystem_is_not_asked(self):
        assignment = rs.read_assignment()
        sub, slot = [(m, x["name"]) for m, st in rs.seam_members() if m not in ("wind_meteors", "economy", "targeting", "map")
                     for x in rs.parse_interface(m, st)["slots"] if x["name"] in assignment.get(m, [])][0]
        _, errors = build([row(slot, "field_read", "OpenHeap::count")])
        self.assertEqual([e for e in errors if "OpenHeap" in e], [])

    def test_an_exempt_type_resolves_it(self):
        ann = annotations()
        ann["exempt_types"] = {"OpenHeap": "a local heap of the search"}
        self.assertEqual(self.pending([self.ROW], ann), [])

    def test_an_exempt_type_needs_a_reason(self):
        ann = annotations()
        ann["exempt_types"] = {"OpenHeap": ""}
        _, errors = build([self.ROW], ann)
        self.assertTrue(any("exempt_types: OpenHeap needs a reason" in e for e in errors), errors)

    def test_an_exempt_target_resolves_it(self):
        ann = annotations()
        ann["exempt"]["OpenHeap::count"] = "a local"
        self.assertEqual(self.pending([self.ROW], ann), [])

    def test_an_alias_of_a_kind_field_resolves_it(self):
        ann = annotations()
        ann["objects"]["Player"]["fields"][0].setdefault("aliases", []).append("OpenHeap::count")
        self.assertEqual(self.pending([self.ROW], ann), [])

    def test_a_hand_rule_resolves_it(self):
        ann = annotations()
        ann["rules"].append({"kind": "field_read", "target": "OpenHeap::count", "rule": "fields", "reads": ["Player.active"], "note": "x"})
        self.assertEqual(self.pending([self.ROW], ann), [])

    def test_a_type_of_a_kind_is_not_asked(self):
        self.assertEqual(self.pending([row("UpdatePlayerEconomy", "field_read", "Player::active")]), [])

    def test_a_seam_table_is_not_asked(self):
        self.assertEqual(self.pending([row("UpdatePlayerEconomy", "field_read", "Seams::units"), row("UpdatePlayerEconomy", "field_read", "UnitsSeam::SetStateBits")]), [])

    def test_a_service_class_the_subsystem_lists_is_not_asked(self):
        self.assertEqual(self.pending([row("UpdateMeteors", "field_read", "TdfFile::current"), row("UpdateMeteors", "field_read", "CommandArgs::count")]), [])
        self.assertEqual(len(self.pending([row("UpdatePlayerEconomy", "field_read", "TdfFile::current")])), 1)


class PendingList(unittest.TestCase):
    def test_every_pending_item_is_printed_one_per_line(self):
        rows = [row("UpdatePlayerEconomy", "field_read", "Cold%d::x" % i) for i in range(9)]
        _, errors = build(rows)
        message = [e for e in errors if "pending types" in e][0]
        lines = message.split("\n")
        self.assertEqual(len(lines), 10, message)
        self.assertEqual(sorted(l.strip() for l in lines[1:]), sorted("field_read Cold%d::x" % i for i in range(9)))


class IndirectCalls(unittest.TestCase):
    SEAM = "indirect via void (*)(Unit *, int, int) __attribute__((stdcall)) [UnitsSeam::SetStateBits]"
    OTHER = "indirect via void (*)(Unit *, int) __attribute__((stdcall)) [TargetingSeam::ClearWeaponTarget]"
    OWN = "indirect via void (*)(RandStream) __attribute__((cdecl)) [WindMeteorsSeam::UpdateWindOnStream]"
    TABLE = "indirect via int (*)(void *, Order *, unsigned int) __attribute__((stdcall)) [OrderTableEntry::notify]"
    VIRTUAL = "virtual OrderFx::GetType"
    CONCRETE = ["ApproachRadius::GetType", "RingApproach::GetType", "PathOrder::GetType"]

    def pending(self, rows, ann=None, slot_name="UpdatePlayerEconomy"):
        model, errors = build(rows, ann)
        return [e for e in errors if ("pending indirect" in e or "through" in e) and not e.startswith("map")], model["subsystems"]["economy"]["slots"].get(slot_name)

    def economy(self, **changes):
        ann = annotations()
        ann["subsystems"]["economy"]["slots"]["UpdatePlayerEconomy"].update(changes)
        return ann

    def test_a_call_through_an_import_is_accounted_for(self):
        self.assertEqual(self.pending([row("UpdatePlayerEconomy", "indirect_call", self.SEAM)])[0], [])

    def test_a_call_through_a_seam_it_does_not_import_is_an_error(self):
        errors, _ = self.pending([row("UpdatePlayerEconomy", "indirect_call", self.OTHER)])
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("economy is complete but has pending indirect", errors[0])
        self.assertIn("ClearWeaponTarget", errors[0])

    def test_a_call_to_a_slot_of_its_own_seam_needs_the_functions_in_the_closure(self):
        model, errors = build([row("UpdateWind", "indirect_call", self.OWN)])
        self.assertEqual([e for e in errors if "pending indirect" in e], [])
        model, errors = build([row("UpdateMeteors", "indirect_call", self.OWN)])
        self.assertTrue(any("pending indirect" in e and "UpdateWindOnStream" in e for e in errors), errors)

    def test_a_code_pointer_is_an_error(self):
        errors, _ = self.pending([row("UpdatePlayerEconomy", "indirect_call", self.TABLE)])
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("OrderTableEntry::notify", errors[0])

    def test_a_vtable_dispatch_needs_the_concrete_functions_in_the_closure(self):
        rows = [row("UpdatePlayerEconomy", "indirect_call", self.VIRTUAL)] + [row(f, "function", "") for f in self.CONCRETE]
        errors, _ = self.pending(rows)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("ApproachRadius::GetType", errors[0])
        ann = self.economy()
        ann["subsystems"]["economy"]["slots"]["UpdatePlayerEconomy"]["functions"] += self.CONCRETE
        self.assertEqual(self.pending(rows, ann)[0], [])

    def test_one_concrete_function_out_of_the_closure_is_still_an_error(self):
        rows = [row("UpdatePlayerEconomy", "indirect_call", self.VIRTUAL)] + [row(f, "function", "") for f in self.CONCRETE]
        ann = self.economy()
        ann["subsystems"]["economy"]["slots"]["UpdatePlayerEconomy"]["functions"] += self.CONCRETE[:2]
        errors, _ = self.pending(rows, ann)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("PathOrder::GetType", errors[0])

    def test_a_dispatch_on_a_class_that_is_no_kind_is_an_error(self):
        errors, _ = self.pending([row("UpdatePlayerEconomy", "indirect_call", "virtual DirectPlay::Close")])
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("DirectPlay", errors[0])

    def test_through_adds_the_functions_to_the_closure(self):
        rows = [row("UpdatePlayerEconomy", "indirect_call", self.TABLE), row("StopOrder", "field_write", "Unit::health")]
        errors, slot = self.pending(rows, self.economy(through={self.TABLE: ["StopOrder"]}))
        self.assertEqual(errors, [])
        model, _ = build(rows, self.economy(through={self.TABLE: ["StopOrder"]}))
        names = ["%s.%s" % (f["object"], f["name"]) for f in model["object_fields"]]
        self.assertEqual([names[i] for i in model["subsystems"]["economy"]["slots"]["UpdatePlayerEconomy"]["objectWrites"]], ["Unit.health"])

    def test_through_may_be_named_by_the_field_in_brackets(self):
        rows = [row("UpdatePlayerEconomy", "indirect_call", self.TABLE), row("StopOrder", "function", "")]
        self.assertEqual(self.pending(rows, self.economy(through={"OrderTableEntry::notify": ["StopOrder"]}))[0], [])

    def test_the_functions_of_through_are_held_to_the_same_checks(self):
        rows = [row("UpdatePlayerEconomy", "indirect_call", self.TABLE), row("StopOrder", "function", ""), row("StopOrder", "indirect_call", self.OTHER)]
        errors, _ = self.pending(rows, self.economy(through={self.TABLE: ["StopOrder"]}))
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("ClearWeaponTarget", errors[0])

    def test_through_with_a_reason_is_a_call_that_does_nothing(self):
        rows = [row("UpdatePlayerEconomy", "indirect_call", self.TABLE)]
        self.assertEqual(self.pending(rows, self.economy(through={self.TABLE: {"reason": "the table is empty on the recorded paths"}}))[0], [])

    def test_through_with_nothing_and_no_reason_is_an_error(self):
        rows = [row("UpdatePlayerEconomy", "indirect_call", self.TABLE)]
        for entry in ([], {}):
            errors, _ = self.pending(rows, self.economy(through={self.TABLE: entry}))
            self.assertTrue(any("through" in e and "reason" in e for e in errors), errors)

    def test_through_that_names_no_function_is_an_error(self):
        rows = [row("UpdatePlayerEconomy", "indirect_call", self.TABLE)]
        errors, _ = self.pending(rows, self.economy(through={self.TABLE: ["NoSuchFunction"]}))
        self.assertTrue(any("NoSuchFunction" in e and "no function" in e for e in errors), errors)

    def test_through_that_matches_no_row_is_an_error(self):
        errors, _ = self.pending([], self.economy(through={self.TABLE: {"reason": "x"}}))
        self.assertTrue(any("matches no indirect_call row" in e for e in errors), errors)


class Tables(unittest.TestCase):
    """N1: an object of a kind with a table is Kind#index, where the annotation names the table once."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def test_units_and_players_are_instances_of_the_table_form(self):
        _, errors, model = self.generated()
        self.assertEqual(errors, [])
        by_kind = {t["kind"]: t for t in model["tables"]}
        self.assertEqual((by_kind["Player"]["member"], by_kind["Player"]["ref"], by_kind["Player"]["alloc"]), ("players", False, 0))
        self.assertEqual((by_kind["Unit"]["member"], by_kind["Unit"]["ref"], by_kind["Unit"]["alloc"]), ("units", True, 5001))
        self.assertEqual([t["kind"] for t in model["tables"][:2]], ["Player", "Unit"])

    def test_a_table_of_the_game_and_one_of_the_sim_globals_are_in_the_schema(self):
        files, _, _ = self.generated()
        text = files[rs.SCHEMA_H]
        self.assertIn("offsetof(Game, projectiles)", text)
        self.assertIn("offsetof(SimGlobals, g_movementClasses.entries)", text)
        self.assertIn('#pragma push_macro("g_movementClasses")', text)
        self.assertIn('{ "Projectile", sizeof(Projectile), RS_STATE, 5,', text)
        self.assertIn('{ "UnitDef", sizeof(UnitDef), RS_STATE, RT_ORDER,', text)

    def test_the_size_of_an_inline_table_is_checked_against_its_count(self):
        files, _, _ = self.generated()
        self.assertIn("sizeof(((Game*)nullptr)->players) == 10 * sizeof(Player)", files[rs.SCHEMA_H])
        self.assertIn("sizeof(((SimGlobals*)nullptr)->g_movementClasses.entries) == 32 * sizeof(MovementClass)", files[rs.SCHEMA_H])

    def test_the_member_of_a_table_is_not_an_untracked_target(self):
        _, errors = build([row("UpdateMeteors", "field_read", "Game::projectiles"), row("UpdateMeteors", "global_read", "g_movementClasses")])
        self.assertEqual([e for e in errors if "pending" in e], [])

    def test_a_table_needs_a_base_and_a_member(self):
        ann = annotations()
        ann["objects"]["Projectile"]["table"] = {"member": "projectiles", "count": "projectileCount"}
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("Projectile table: needs a `base`" in e for e in errors), errors)

    def test_only_a_state_kind_has_a_table(self):
        ann = annotations()
        ann["objects"]["Vec3"]["table"] = {"base": "game", "member": "projectiles", "count": 4}
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("Vec3: only a state kind can have a table" in e for e in errors), errors)

    def test_a_count_is_an_integer_or_one_or_two_factors(self):
        errors = []
        self.assertEqual(rs.parse_length(7, "t", errors), [{"const": 7}, {"const": 1}])
        self.assertEqual(rs.parse_length("width*(height>>4)", "t", errors), [{"member": "width", "shift": 0}, {"member": "height", "shift": 4}])
        self.assertEqual(errors, [])
        rs.parse_length("a*b*c", "t", errors)
        rs.parse_length("a-1", "t", errors)
        self.assertEqual(len(errors), 2, errors)

    def test_a_factor_may_add_before_it_shifts(self):
        errors = []
        self.assertEqual(rs.parse_length("(count+255)>>8", "t", errors), [{"member": "count", "shift": 8, "add": 255}, {"const": 1}])
        self.assertEqual(rs.parse_length("count+0xff>>8", "t", errors)[0], {"member": "count", "shift": 8, "add": 255})
        self.assertEqual(rs.parse_length("(width+1)*height", "t", errors), [{"member": "width", "shift": 0, "add": 1}, {"member": "height", "shift": 0}])
        self.assertEqual(rs.parse_length("n+010", "t", errors)[0], {"member": "n", "shift": 0, "add": 10})
        self.assertEqual(rs.parse_length("width*(height+15>>4)", "t", errors), [{"member": "width", "shift": 0}, {"member": "height", "shift": 4, "add": 15}])
        self.assertEqual(errors, [])
        rs.parse_length("a+b", "t", errors)
        self.assertEqual(len(errors), 1, errors)

    def test_an_addend_is_rendered_after_the_shift_and_only_when_there_is_one(self):
        self.assertEqual(rs.render_factor({"member": "count", "shift": 8, "add": 255}, "Grid"),
                         "{ offsetof(Grid, count), (unsigned char)sizeof(((Grid*)nullptr)->count), 8, 255u }")
        self.assertEqual(rs.render_factor({"member": "count", "shift": 8}, "Grid"), "{ offsetof(Grid, count), (unsigned char)sizeof(((Grid*)nullptr)->count), 8 }")

    def test_an_array_length_with_an_addend_is_in_the_generated_table(self):
        ann = annotations()
        ann["game_fields"]["visibilityMask"]["length"] = "(width+0xff)>>8"
        files, errors, _ = rs.generate(rs.read_rows(), ann)
        self.assertEqual(errors, [])
        self.assertIn("{ offsetof(Game, width), (unsigned char)sizeof(((Game*)nullptr)->width), 8, 255u }", files[rs.SCHEMA_H])


class ProductCounts(unittest.TestCase):
    """A table counted by a product of block fields needs those fields in the read set of every slot that reaches it."""

    def test_cell_is_a_table_counted_by_width_times_height(self):
        _, errors, model = rs.generate(rs.read_rows(), annotations())
        self.assertEqual(errors, [])
        cell = {t["kind"]: t for t in model["tables"]}["Cell"]
        self.assertEqual((cell["member"], cell["ref"], cell["alloc"]), ("cells", True, 0))
        self.assertEqual([f["member"] for f in cell["count"]], ["width", "height"])

    def test_a_slot_that_reaches_the_table_must_read_both_factors(self):
        ann = annotations()
        del ann["subsystems"]["economy"]["slots"]["UpdateMetalExtraction"]
        _, errors, _ = rs.generate(rs.read_rows(), ann)
        self.assertTrue(any("economy.UpdateMetalExtraction reaches Cell, whose count multiplies Game.width" in e for e in errors), errors)
        self.assertTrue(any("economy.UpdateMetalExtraction reaches Cell, whose count multiplies Game.height" in e for e in errors), errors)

    def test_one_missing_factor_is_reported_alone(self):
        ann = annotations()
        ann["subsystems"]["economy"]["slots"]["UpdateMetalExtraction"]["reads"] = ["Game.width"]
        _, errors, _ = rs.generate(rs.read_rows(), ann)
        found = [e for e in errors if "reaches Cell" in e]
        self.assertEqual(len(found), 1, found)
        self.assertIn("Game.height", found[0])

    def test_an_extra_read_must_be_a_block_field(self):
        ann = annotations()
        ann["subsystems"]["economy"]["slots"]["UpdateMetalExtraction"]["reads"] = ["Game.width", "Game.height", "Game.nothing"]
        _, errors, _ = rs.generate(rs.read_rows(), ann)
        self.assertTrue(any("the extra read Game.nothing is no block field" in e for e in errors), errors)


class Arrays(unittest.TestCase):
    """N2: an array whose length is a field expression, written whole at first use and as element runs after it."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def test_an_array_of_an_object_is_a_field_with_a_length_expression(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        explored = [a for a in model["arrays"] if a["owner"] == "Player" and a["member"] == "explored"][0]
        self.assertEqual((explored["elem"], explored["ref"], explored["length"]), (1, True, [{"member": "exploredSize", "shift": 0}, {"const": 1}]))
        row = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "explored"')][0]
        self.assertIn("RK_ARRAY", row)
        self.assertTrue(row.rstrip(" },").endswith(", %d" % (model["arrays"].index(explored) + 1)), row)

    def test_the_length_is_read_from_the_object_it_belongs_to(self):
        files, _, _ = self.generated()
        text = files[rs.SCHEMA_H]
        self.assertIn("{ 4, 1, 0, { { offsetof(MovementClass, width), (unsigned char)sizeof(((MovementClass*)nullptr)->width), 0 }, "
                      "{ offsetof(MovementClass, height), (unsigned char)sizeof(((MovementClass*)nullptr)->height), 4, 15u } } }", text)
        self.assertIn("{ 2, 1, 0, { { offsetof(Game, width), (unsigned char)sizeof(((Game*)nullptr)->width), 1 }, "
                      "{ offsetof(Game, height), (unsigned char)sizeof(((Game*)nullptr)->height), 1 } } }", text)

    def test_a_block_field_can_be_an_array(self):
        files, _, model = self.generated()
        field = [f for f in model["fields"] if f["name"] == "Game.visibilityMask"][0]
        self.assertEqual((field["kind"], field["size"]), ("array", 4))
        self.assertIn('{ "Game.visibilityMask",', files[rs.SCHEMA_H])
        self.assertIn("RF_GAME_VISIBILITYMASK,", files[rs.SCHEMA_H])

    def test_the_length_of_a_block_array_names_members_of_the_game(self):
        ann = annotations()
        ann["game_fields"]["visibilityMask"]["length"] = "noSuchMember"
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("noSuchMember, which is no member of Game" in e for e in errors), errors)

    def test_an_array_needs_an_element_and_a_length(self):
        ann = annotations()
        field = [f for f in ann["objects"]["Player"]["fields"] if f["name"] == "explored"][0]
        del field["elem"]
        del field["length"]
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("Player.explored: an array needs an `elem`" in e for e in errors), errors)
        self.assertTrue(any("Player.explored: an array needs a `length`" in e for e in errors), errors)

    def test_only_the_fields_of_a_state_kind_are_arrays(self):
        ann = annotations()
        ann["objects"]["Vec3"]["fields"].append({"name": "w", "kind": "array", "elem": "u8", "length": 4})
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("Vec3.w: only the fields of a state kind can be arrays" in e for e in errors), errors)

    def test_an_array_that_is_a_ref_is_checked_to_be_one(self):
        files, _, _ = self.generated()
        self.assertIn("sizeof(((Player*)nullptr)->explored) == sizeof(unsigned)", files[rs.SCHEMA_H])


class StructForms(unittest.TestCase):
    """N3, N4 and N10: a nullable struct in, an array of structs in, and structs by value as arguments and results."""

    def generated(self, ann):
        return rs.generate(rs.read_rows(), ann)

    def targeting(self, **pos):
        ann = annotations()
        ann["subsystems"]["targeting"]["slots"]["SetWeaponTargetPos"]["args"]["pos"] = dict({"mode": "struct", "kind": "Vec3", "dir": "in"}, **pos)
        return ann

    def test_a_nullable_struct_is_recorded_as_null_and_replayed_as_a_null_pointer(self):
        files, errors, _ = self.generated(self.targeting(nullable=True))
        self.assertEqual(errors, [])
        self.assertIn('ArgStruct(s_call, "a", "pos", ROK_VEC3, a1);', files[rs.HOOKS_H])
        runner = files[rs.SLOTS_H]
        body = runner[runner.index("void Run_targeting_SetWeaponTargetPos"):]
        body = body[:body.index("\n}\n")]
        self.assertIn('Vec3* p1 = (Vec3*)ParseStructOpt("pos", ROK_VEC3, &p1_value);', body)
        self.assertIn("g_seams.targeting->SetWeaponTargetPos(p0, p1, p2)", body)

    def test_a_struct_that_is_not_nullable_is_passed_by_address(self):
        files, _, _ = self.generated(annotations())
        self.assertIn("g_seams.targeting->SetWeaponTargetPos(p0, &p1, p2)", files[rs.SLOTS_H])

    def test_only_a_struct_in_can_be_nullable(self):
        ann = annotations()
        ann["subsystems"]["targeting"]["slots"]["GetWeaponTargetPos"]["args"]["pos"]["nullable"] = True
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("only a struct in or inout can be nullable" in e for e in errors), errors)

    def test_an_array_of_structs_is_a_pointer_and_a_count(self):
        files, errors, _ = self.generated(self.targeting(mode="structs", count="weapon"))
        self.assertEqual(errors, [])
        self.assertIn('ArgStructs(s_call, "pos", ROK_VEC3, a1, (int)a2);', files[rs.HOOKS_H])
        runner = files[rs.SLOTS_H]
        body = runner[runner.index("void Run_targeting_SetWeaponTargetPos"):]
        self.assertIn('StructsHolder p1;\n    p1.Parse("pos", ROK_VEC3, ArgScalar<int>("weapon"));', body)
        self.assertIn("g_seams.targeting->SetWeaponTargetPos(p0, (Vec3*)p1.data, p2)", body)

    def test_the_count_names_another_integer_argument(self):
        for count, want in (("nothing", "needs a `count` that names another argument"), ("pos", "needs a `count` that names another argument"), ("unit", "is no integer")):
            _, errors, _ = self.generated(self.targeting(mode="structs", count=count))
            self.assertTrue(any(want in e for e in errors), (count, errors))

    def test_only_an_array_in_is_recorded(self):
        _, errors, _ = self.generated(self.targeting(mode="structs", count="weapon", dir="out"))
        self.assertTrue(any("only an array in is recorded" in e for e in errors), errors)

    def test_the_kind_of_a_struct_form_is_a_value_kind_of_the_argument_type(self):
        _, errors, _ = self.generated(self.targeting(kind="Player"))
        self.assertTrue(any("needs a `kind` that is a value kind" in e for e in errors), errors)
        _, errors, _ = self.generated(self.targeting(kind="Point16"))
        self.assertTrue(any("the argument pos is Vec3*, and the kind Point16 is Point16*" in e for e in errors), errors)

    def test_a_struct_by_value_is_written_as_its_fields_in_one_line(self):
        ann = annotations()
        orders = ann["subsystems"]["orders"].setdefault("slots", {})
        orders.setdefault("AddOrder", {}).setdefault("args", {})["kind"] = {"mode": "value", "kind": "MissionType"}
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        self.assertIn('ArgValueStruct(s_call, "a", "kind", ROK_MISSIONTYPE, &a0);', files[rs.HOOKS_H])

    def test_a_result_by_value_is_recorded_in_the_import_line_and_the_closing_line(self):
        ann = annotations()
        ann["subsystems"]["targeting"]["imports"].append("units.GetPieceAngles")
        ann["subsystems"]["units"]["slots"]["GetPieceAngles"] = {"result": {"mode": "value", "kind": "Angles16"}}
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        hooks = files[rs.HOOKS_H]
        body = hooks[hooks.index("RecUnitsGetPieceAngles"):]
        body = body[:body.index("\n}\n")]
        self.assertIn("ImportResultValue(line, ROK_ANGLES16, &result);", body)
        self.assertIn('ArgValueStruct(s_tail, "o", "result", ROK_ANGLES16, &result);', body)
        self.assertIn("return ParseValueText<Angles16>(ROK_ANGLES16, r);", files[rs.SLOTS_H])

    def test_a_result_by_value_of_another_type_is_an_error(self):
        ann = annotations()
        ann["subsystems"]["units"]["slots"]["GetPieceAngles"] = {"result": {"mode": "value", "kind": "MissionType"}}
        _, errors, _ = self.generated(ann)
        self.assertTrue(any("the result is Angles16" in e for e in errors), errors)

    def test_an_unknown_mode_is_an_error(self):
        _, errors, _ = self.generated(self.targeting(mode="nonsense"))
        self.assertTrue(any("unknown mode nonsense" in e for e in errors), errors)


class Polymorphic(unittest.TestCase):
    """N5: a base kind lists its concrete kinds, each concrete kind names its vtable, a code pointer is an index in a table."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def errors(self, ann):
        return self.generated(ann)[1]

    def test_the_base_lists_its_kinds_and_each_kind_its_vtable(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        vtables = files[rs.VTABLES_CPP]
        self.assertIn("extern void (* const g_approachRadiusVtable[])();", vtables)
        self.assertIn('{ "ApproachRadius", sizeof(ApproachRadius), RS_STATE, RT_ORDER, 0, 0, 0, 0, 0 },', text)
        self.assertIn("    case ROK_APPROACHRADIUS:\n        return g_approachRadiusVtable;", vtables)
        self.assertIn('    case ROK_APPROACHRADIUS:\n        return "g_approachRadiusVtable";', vtables)
        base = [l for l in text.split("\n") if l.startswith('    { "OrderFx",')][0]
        self.assertTrue(base.endswith('0, 0, 0, 3, 0 },'), base)
        self.assertIn("kReplayPolyKinds[] = { ROK_APPROACHRADIUS, ROK_RINGAPPROACH, ROK_PATHORDER, ROK_FIXTURETIMER, ROK_FIXTUREASSAULT };", text)
        self.assertIn('static_assert(__is_base_of(OrderFx, PathOrder), "PathOrder is a kind of OrderFx");', text)
        self.assertIn('static_assert(__is_polymorphic(RingApproach), "RingApproach has a vtable");', text)

    def test_a_pointer_to_the_base_names_the_base_and_its_target_is_the_base(self):
        files, _, model = self.generated()
        field = [f for f in model["object_fields"] if (f["object"], f["name"]) == ("Order", "attached")][0]
        self.assertEqual((field["target"], field["poly"]), ("OrderFx", "OrderFx"))
        self.assertIn('{ "attached", offsetof(Order, attached), sizeof(((Order*)nullptr)->attached), ROK_ORDER, RK_REF, ROK_ORDERFX,', files[rs.SCHEMA_H])

    def test_an_argument_of_the_base_type_is_recorded_as_the_base(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        arg = model["subsystems"]["orders"]["slots"]["OrderFxAddFlags"]["args"][0]
        self.assertEqual((arg["mode"], arg["kind"], arg["poly"]), ("object", "OrderFx", "OrderFx"))
        self.assertIn("roots.Add(ROK_ORDERFX, a0);", files[rs.HOOKS_H])

    def test_an_argument_note_of_the_base_needs_poly(self):
        ann = annotations()
        ann["subsystems"]["orders"]["slots"] = {"OrderFxAddFlags": {"args": {"self": {"mode": "object", "kind": "OrderFx"}}}}
        self.assertTrue(any("the argument self points at the base kind OrderFx and needs" in e for e in self.errors(ann)))

    def test_poly_of_a_kind_that_is_no_base_is_an_error(self):
        ann = annotations()
        ann["subsystems"]["orders"]["slots"] = {"AppendOrder": {"args": {"node": {"mode": "object", "kind": "Order", "poly": "Order"}}}}
        self.assertTrue(any("`poly` of the argument node names the base kind" in e for e in self.errors(ann)))

    def test_a_ref_to_a_base_needs_poly(self):
        ann = annotations()
        del [f for f in ann["objects"]["Order"]["fields"] if f["name"] == "attached"][0]["poly"]
        self.assertTrue(any('Order.attached: points at the base kind OrderFx and needs "poly"' in e for e in self.errors(ann)))

    def test_poly_names_the_target(self):
        ann = annotations()
        [f for f in ann["objects"]["Order"]["fields"] if f["name"] == "next"][0]["poly"] = "OrderFx"
        self.assertTrue(any("Order.next: `poly` names the base kind" in e for e in self.errors(ann)))

    def test_a_base_has_no_fields_and_a_kind_has_a_vtable_and_one_base(self):
        ann = annotations()
        ann["objects"]["OrderFx"]["fields"].append({"name": "source", "kind": "ref", "target": "Order"})
        self.assertTrue(any("OrderFx (base): a base kind has no fields" in e for e in self.errors(ann)))
        ann = annotations()
        del ann["objects"]["PathOrder"]["vtable"]
        self.assertTrue(any("OrderFx (base): the kind PathOrder has no vtable" in e for e in self.errors(ann)))
        ann = annotations()
        ann["objects"]["Order"]["kinds"] = ["PathOrder"]
        ann["objects"]["Order"]["fields"] = []
        self.assertTrue(any("the kind PathOrder is also a kind of OrderFx" in e or "Order (base): the kind PathOrder" in e for e in self.errors(ann)))

    def test_two_kinds_cannot_share_a_vtable(self):
        ann = annotations()
        ann["objects"]["RingApproach"]["vtable"] = "g_approachRadiusVtable"
        self.assertTrue(any("RingApproach: the vtable g_approachRadiusVtable is also the vtable of ApproachRadius" in e for e in self.errors(ann)))

    def test_a_vtable_is_a_name_and_belongs_to_a_kind_of_a_base(self):
        ann = annotations()
        ann["objects"]["PathOrder"]["vtable"] = "0x4fd3b8"
        self.assertTrue(any("PathOrder: the vtable is the name of a global" in e for e in self.errors(ann)))
        ann = annotations()
        ann["objects"]["OrderFx"]["kinds"].remove("PathOrder")
        self.assertTrue(any("PathOrder: has a vtable and is a kind of no base" in e for e in self.errors(ann)))

    def test_a_code_field_is_an_index_in_its_table(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        self.assertIn("kReplayCodeOrderNotify[] = {", text)
        self.assertIn("    (const void*)StopOrder,", text)
        self.assertIn('{ "orderNotify", kReplayCodeOrderNotify, %d },' % len(model["code_tables"][0]["entries"]), text)
        self.assertIn('static_assert(sizeof(((OrderTableEntry*)nullptr)->notify) == sizeof(void*), "OrderTableEntry::notify is a code pointer");', text)
        row_ = [l for l in text.split("\n") if l.startswith('    { "notify",')][0]
        self.assertIn("RK_CODE, 0,", row_)

    def test_the_table_lists_each_function_once_in_the_order_of_registration(self):
        _, _, model = self.generated()
        entries = model["code_tables"][0]["entries"]
        self.assertEqual(len(entries), len(set(entries)))
        self.assertEqual(entries[0], "ReadyOrder")
        self.assertLess(entries.index("StandbyOrder"), entries.index("VtolStandbyOrder"))
        self.assertLess(entries.index("VtolStandbyOrder"), entries.index("StopOrder"))

    def test_a_function_that_no_header_declares_is_an_error(self):
        ann = annotations()
        ann["code_tables"]["orderNotify"]["entries"].append("NoSuchOrder")
        self.assertTrue(any("code table orderNotify: NoSuchOrder is declared in none of its headers" in e for e in self.errors(ann)))

    def test_a_header_of_the_table_is_one_of_the_includes(self):
        ann = annotations()
        ann["includes"].remove("../orders/vtol_orders.h")
        self.assertTrue(any("vtol_orders.h is not one of the schema's `includes`" in e for e in self.errors(ann)))

    def test_the_fill_is_a_slot_of_a_seam(self):
        for fill in ("RegisterAllOrderTypes", "orders.NoSuchSlot", "nowhere.RegisterAllOrderTypes"):
            ann = annotations()
            ann["code_tables"]["orderNotify"]["fill"] = fill
            self.assertTrue(any("`fill` names a slot of a seam" in e for e in self.errors(ann)), fill)

    def test_a_code_field_names_a_table(self):
        ann = annotations()
        [f for f in ann["objects"]["OrderTableEntry"]["fields"] if f["name"] == "notify"][0]["table"] = "nowhere"
        self.assertTrue(any("OrderTableEntry.notify: a code field names a table" in e for e in self.errors(ann)))


class Callbacks(unittest.TestCase):
    """N6: an import that calls back into the subsystem records each call under its i line, and the stub makes them in order."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def with_import(self, mutate=None):
        ann = annotations()
        ann["subsystems"]["map"]["slots"]["VisitObjectsInRange"]["args"] = {"pos": {"mode": "struct", "kind": "Vec3", "dir": "in"}, "visitor": {"mode": "callback", "callback": "UnitVisitor"}}
        ann["subsystems"]["wind_meteors"]["imports"].append("map.VisitObjectsInRange")
        if mutate:
            mutate(ann)
        return ann

    def errors(self, ann):
        return self.generated(ann)[1]

    def test_the_interfaces_are_in_the_schema_with_their_methods(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        self.assertIn("RCB_UNITVISITOR,", text)
        self.assertIn('static const char* const kReplayUnitScanVisitorMethods[] = { "VisitUnit" };', text)
        self.assertIn('{ "UnitScanVisitor", "scanner", 1, kReplayUnitScanVisitorMethods },', text)

    def test_the_recorder_gets_a_class_that_records_and_forwards_each_method(self):
        files, _, _ = self.generated()
        text = files[rs.CALLBACKS_H]
        self.assertIn("class ReplayThunkUnitScanVisitor : public UnitScanVisitor {", text)
        self.assertIn("inline int ReplayThunkUnitScanVisitor::VisitUnit(Unit* a0)", text)
        self.assertIn("ReplayAddObjectName(call.Text(), call.Table(), ROK_UNIT, a0, true);", text)
        self.assertIn("typedef int (__thiscall* Method)(void*, Unit*);", text)
        self.assertIn("int result = ((Method)call.Original(0))(this, a0);", text)
        self.assertIn("ReplayValue(call.Text(), result);", text)
        self.assertIn("ReplayValue(call.Text(), a0);", text)
        self.assertIn("case RCB_CALLBACK: {", text)

    def test_the_replayer_gets_a_player_that_calls_the_modules_object(self):
        files, _, _ = self.generated()
        text = files[rs.CALLBACKS_SLOTS]
        self.assertIn("void PlayCallbacks_UnitScanVisitor(void* object, unsigned handle, const char* import)", text)
        self.assertIn("int result = ((UnitScanVisitor*)object)->VisitUnit(a0);", text)
        self.assertIn("CallbackDone(import, k, args, have.Data());", text)
        self.assertIn("CallbackDone(import, k, args, nullptr);", text)

    def test_a_callback_argument_of_an_import_is_recorded_under_the_i_line(self):
        files, errors, _ = self.generated(self.with_import())
        self.assertEqual(errors, [])
        hooks = files[rs.HOOKS_H]
        body = hooks[hooks.index("RecMapVisitObjectsInRange("):]
        body = body[:body.index("\n}\n")]
        self.assertIn("ReplayCallbackScope cb2;", body)
        self.assertIn("ImportCallback(line, cb2, RCB_UNITVISITOR, a2);", body)
        self.assertLess(body.index("s_inner_map->VisitObjectsInRange"), body.index("cb2.Stop();"))
        self.assertLess(body.index("cb2.Stop();"), body.index("line.AddText(cb2.Lines());"))
        self.assertLess(body.index("line.AddText(cb2.Lines());"), body.index("FinishImport(line);"))

    def test_the_stub_plays_the_calls_after_it_matches_the_i_line(self):
        files, _, _ = self.generated(self.with_import())
        slots = files[rs.SLOTS_H]
        body = slots[slots.index("Imp_map_VisitObjectsInRange("):]
        body = body[:body.index("\n}\n")]
        self.assertIn("AddObject(t, OBJ_CALLBACK + RCB_UNITVISITOR, a2);", body)
        self.assertLess(body.index("Enter(t)"), body.index('PlayCallbacks_UnitVisitor(a2, OBJ_CALLBACK + RCB_UNITVISITOR, "map.VisitObjectsInRange");'))

    def test_a_callback_argument_of_a_slot_no_subsystem_imports_is_recorded_as_an_a_line(self):
        def only_note(ann):
            ann["subsystems"]["wind_meteors"]["imports"].remove("map.VisitObjectsInRange")
            del ann["subsystems"]["map"]["slots"]["VisitObjectsInRange"]["record"]
            del ann["subsystems"]["map"]["slots"]["VisitObjectsInRange"]["reason"]
        files, errors, _ = self.generated(self.with_import(only_note))
        self.assertFalse([e for e in errors if "callback argument" in e])
        hooks = files[rs.HOOKS_H]
        body = hooks[hooks.index("RecMapVisitObjectsInRange("):]
        body = body[:body.index("\n}\n")]
        self.assertIn("ReplayCallbackScope held2;", body)
        self.assertIn('ArgCallback(s_call, "visitor", held2, RCB_UNITVISITOR, a2);', body)
        self.assertLess(body.index("s_inner_map->VisitObjectsInRange"), body.index("held2.Stop();"))
        self.assertLess(body.index("held2.Stop();"), body.index("ArgCallbackExit(s_tail, held2);"))
        self.assertNotIn("ImportCallback", body)

    def test_the_runner_of_a_slot_binds_its_callback_argument_by_the_line(self):
        files, _, _ = self.generated()
        shapes = files[rs.SHAPES_SLOTS]
        body = shapes[shapes.index("void Run_fixture_Notify()"):]
        body = body[:body.index("\n}\n")]
        self.assertIn('Callback* p1 = (Callback*)ArgCallback("done", RCB_CALLBACK);', body)
        self.assertIn("Native_fixture_Notify(p0, p1, p2)", body)
        hooks = files[rs.FIXTURE_HOOKS_INC]
        wrapper = hooks[hooks.index("RecFixtureNotify("):]
        wrapper = wrapper[:wrapper.index("\n}\n")]
        self.assertIn('ArgCallback(s_call, "done", held1, RCB_CALLBACK, a1);', wrapper)
        self.assertIn("ArgCallbackExit(s_tail, held1);", wrapper)

    def test_the_replayer_gets_a_class_per_interface_that_records_the_calls_it_gets(self):
        files, _, _ = self.generated()
        text = files[rs.CALLBACKS_SLOTS]
        self.assertIn("class ReplayHeldUnitScanVisitor : public UnitScanVisitor {", text)
        self.assertIn('HeldCall call(RCB_UNITSCANVISITOR, (const UnitScanVisitor*)this, "VisitUnit");', text)
        self.assertIn("ReplayAddObjectName(call.Text(), g_objects, ROK_UNIT, a0, false);", text)
        self.assertIn("return ParseResult<int>(call.Finish());", text)
        self.assertIn('HeldCall call(RCB_CALLBACK, (const Callback*)this, "Complete");', text)
        self.assertIn("return (UnitScanVisitor*)new (memory) ReplayHeldUnitScanVisitor;", text)
        self.assertIn("return sizeof(ReplayHeldCallback);", text)

    def test_a_ref_or_ptr_field_that_holds_a_callback_is_a_field_kind_of_its_own(self):
        files, errors, _ = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.FIXTURE_SCHEMA_H]
        self.assertIn('ROK_FIXTURENOTIFY, RK_CBPTR, RCB_CALLBACK,', text)
        self.assertIn('ROK_FIXTURENOTIFY, RK_CBREF, RCB_CALLBACK,', text)
        self.assertIn('static_assert(__is_same(decltype(((FixtureNotify*)nullptr)->hook), Callback*), "FixtureNotify::hook holds a Callback*");', text)
        self.assertIn('static_assert(__is_same(decltype(((FixtureNotify*)nullptr)->link), Ref<Callback>), "FixtureNotify::link holds a Ref<Callback>");', text)

    def test_the_callback_key_of_a_field_is_for_a_ref_or_ptr_and_names_an_entry(self):
        def field(ann, name):
            return [f for f in ann["objects"]["FixtureNotify"]["fields"] if f["name"] == name][0]
        ann = annotations()
        field(ann, "id")["callback"] = "Callback"
        self.assertTrue(any("FixtureNotify.id: the `callback` key is for a ref or ptr field, and this one is i32" in e for e in self.errors(ann)))
        ann = annotations()
        field(ann, "hook")["callback"] = "Nowhere"
        self.assertTrue(any("FixtureNotify.hook: the `callback` Nowhere is no entry of `callbacks`" in e for e in self.errors(ann)))
        ann = annotations()
        field(ann, "hook")["target"] = "Unit"
        self.assertTrue(any("FixtureNotify.hook: a field that holds a callback is" in e and "target" in e for e in self.errors(ann)))
        ann = annotations()
        ann["objects"]["FixtureNotify"]["scope"] = "value"
        self.assertTrue(any("FixtureNotify.hook: only the fields of a state kind hold callbacks" in e for e in self.errors(ann)))

    def fixture_errors(self, mutate):
        fixtures = json.loads(rs.FIXTURES_JSON.read_text())
        mutate(fixtures["fixtures"])
        return rs.generate(rs.read_rows(), annotations(), fixtures=fixtures)[1]

    def test_the_callback_key_is_rejected_on_the_other_forms(self):
        ann = annotations()
        ann["game_fields"]["slowest"]["callback"] = "Callback"
        self.assertTrue(any("Game.slowest: a ref field of Game is" in e for e in self.errors(ann)))
        errors = self.fixture_errors(lambda f: f["Query"]["args"]["first"].update(callback="Callback"))
        self.assertTrue(any("fixtures.Query: the argument first has the mode struct and a `callback` key" in e for e in errors))
        errors = self.fixture_errors(lambda f: f["Notify"]["args"]["done"].update(callback="Nowhere"))
        self.assertTrue(any("fixtures.Notify: the argument done needs a `callback` that is an entry of `callbacks`" in e for e in errors))
        errors = self.fixture_errors(lambda f: f["FillGoals"]["args"]["goals"]["elem"].update(callback="Callback"))
        self.assertTrue(any("fixtures.FillGoals: the `callback` key is for the argument of a callback" in e for e in errors))
        errors = self.fixture_errors(lambda f: f["Query"]["result"].update(callback="Callback"))
        self.assertTrue(any("fixtures.Query: the result has a `callback` key" in e for e in errors))
        ann = annotations()
        ann["callbacks"]["Callback"]["methods"][0]["args"][0]["callback"] = "Callback"
        self.assertTrue(any("callback Callback.Complete: the argument result has a `callback` key" in e for e in self.errors(ann)))
        ann = annotations()
        ann["objects"]["FixtureNotify"]["fields"][0]["elem"] = {"mode": "object", "kind": "Unit", "callback": "Callback"}
        self.assertTrue(any("the `callback` key is for a ref or ptr field, and not for the elements" in e for e in self.errors(ann)))

    def test_a_condition_cannot_read_through_a_field_that_holds_a_callback(self):
        ann = annotations()
        ann["objects"]["FixtureNotify"]["fields"].append({"name": "late", "kind": "i32", "when": {"via": "hook", "field": "id", "equals": 1}})
        self.assertTrue(any("the `via` field hook holds a callback" in e for e in self.errors(ann)))

    def test_the_callback_names_an_entry_and_its_type(self):
        def unknown(ann):
            ann["subsystems"]["map"]["slots"]["VisitObjectsInRange"]["args"]["visitor"]["callback"] = "Nowhere"
        self.assertTrue(any("needs a `callback` that is an entry of `callbacks`" in e for e in self.errors(self.with_import(unknown))))

        def wrong(ann):
            ann["subsystems"]["map"]["slots"]["VisitObjectsInRange"]["args"]["visitor"]["callback"] = "Callback"
        self.assertTrue(any("the callback Callback is Callback*" in e for e in self.errors(self.with_import(wrong))))

    def test_the_methods_are_the_virtuals_of_the_class_in_order(self):
        ann = annotations()
        ann["callbacks"]["UnitVisitor"]["methods"][0]["name"] = "VisitAll"
        self.assertTrue(any("callback UnitVisitor: the methods are ['VisitAll'], and the class declares the virtuals ['Visit']" in e for e in self.errors(ann)))
        ann = annotations()
        ann["callbacks"]["UnitVisitor"]["header"] = "interfaces/units.h"
        self.assertTrue(any("units.h declares no such class" in e for e in self.errors(ann)))

    def test_a_handle_is_a_name_no_other_handle_has(self):
        ann = annotations()
        ann["callbacks"]["Callback"]["handle"] = "visitor"
        self.assertTrue(any("callback Callback: the handle is a name that no other handle has" in e for e in self.errors(ann)))
        ann = annotations()
        ann["callbacks"]["Callback"]["handle"] = "bank"
        self.assertTrue(any("callback Callback: the handle is a name that no other handle has" in e for e in self.errors(ann)))

    def test_an_argument_has_a_mode_a_type_and_a_kind_that_fit(self):
        ann = annotations()
        ann["callbacks"]["UnitVisitor"]["methods"][0]["args"][0]["mode"] = "bytes-in"
        self.assertTrue(any("the argument unit has the mode bytes-in" in e for e in self.errors(ann)))
        ann = annotations()
        ann["callbacks"]["UnitVisitor"]["methods"][0]["args"][0]["type"] = "Player*"
        self.assertTrue(any("the argument unit is a pointer to a state kind of the schema" in e for e in self.errors(ann)))
        ann = annotations()
        ann["callbacks"]["Callback"]["methods"][0]["args"][0]["type"] = "Widget"
        self.assertTrue(any("the argument result is Widget, which is no scalar" in e for e in self.errors(ann)))
        ann = annotations()
        ann["callbacks"]["UnitScanVisitor"]["methods"][0]["ret"] = "Widget"
        self.assertTrue(any("the result Widget is void, a scalar or a pointer to a state kind" in e for e in self.errors(ann)))


class Vectors(unittest.TestCase):
    """N8: a BlockVector member is [length/capacity] and its elements, as handles, runs of bytes or lines of fields."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def errors(self, ann):
        return self.generated(ann)[1]

    def field(self, ann, name):
        return [f for f in ann["objects"]["PlayerAI"]["fields"] if f["name"] == name][0]

    def test_each_element_mode_has_a_spec_and_a_check_of_the_c_type(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        self.assertEqual([(v["mode"], v["kind"], v["ctype"]) for v in model["vectors"]],
                         [("object", "Unit", "Ref<Unit>"), ("value", "KeyedCell", "KeyedCell"), ("scalar", "", "short")])
        self.assertIn("{ RV_OBJECT, ROK_UNIT, sizeof(Ref<Unit>) },", text)
        self.assertIn("{ RV_VALUE, ROK_KEYEDCELL, sizeof(KeyedCell) },", text)
        self.assertIn("{ RV_SCALAR, 0, sizeof(short) },", text)
        self.assertIn('static_assert(__is_same(decltype(((PlayerAI*)nullptr)->visible.units)::value_type, Ref<Unit>), "PlayerAI::visibleUnits holds Ref<Unit>");', text)
        self.assertIn('static_assert(sizeof(decltype(((PlayerAI*)nullptr)->cells)) == 16, "PlayerAI::cells is a block vector");', text)

    def test_the_field_row_names_the_vector_and_the_element_kind_to_walk(self):
        files, _, _ = self.generated()
        row = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "visibleUnits",')][0]
        self.assertIn("offsetof(PlayerAI, visible.units)", row)
        self.assertIn("RK_VECTOR, ROK_UNIT,", row)
        self.assertTrue(row.rstrip(" },").endswith(", 1"), row)

    def test_a_header_that_needs_the_packing_of_the_game_is_included_packed(self):
        files, _, _ = self.generated()
        self.assertIn('#pragma pack(push, 1)\n#include "../ai/player_ai.h"\n#pragma pack(pop)', files[rs.SCHEMA_H])

    def test_a_vector_needs_an_element_mode(self):
        ann = annotations()
        del self.field(ann, "counts")["elem"]
        self.assertTrue(any("PlayerAI.counts: a vector needs an `elem` with a mode of object, scalar, value" in e for e in self.errors(ann)))
        ann = annotations()
        self.field(ann, "counts")["elem"] = {"mode": "pointer"}
        self.assertTrue(any("PlayerAI.counts: a vector needs an `elem`" in e for e in self.errors(ann)))

    def test_an_element_names_a_type_or_a_kind_of_its_mode(self):
        ann = annotations()
        self.field(ann, "counts")["elem"] = {"mode": "scalar", "type": "Widget"}
        self.assertTrue(any("a scalar element needs a `type` of" in e for e in self.errors(ann)))
        ann = annotations()
        self.field(ann, "visibleUnits")["elem"] = {"mode": "object", "kind": "KeyedCell"}
        self.assertTrue(any("PlayerAI.visibleUnits: an object element needs a `kind` that is a state kind" in e for e in self.errors(ann)))
        ann = annotations()
        self.field(ann, "cells")["elem"] = {"mode": "value", "kind": "Unit"}
        self.assertTrue(any("PlayerAI.cells: a value element needs a `kind` that is a value kind" in e for e in self.errors(ann)))

    def test_only_the_fields_of_a_state_kind_are_vectors(self):
        ann = annotations()
        ann["objects"]["KeyedCell"]["fields"].append({"name": "v", "kind": "vector", "elem": {"mode": "scalar", "type": "int"}})
        self.assertTrue(any("KeyedCell.v: only the fields of a state kind can be vectors" in e for e in self.errors(ann)))


class Snapshots(unittest.TestCase):
    """N9: an import that returns a buffer the slot fills is recorded by its size and a handle, and the bytes at slot exit."""

    def spec(self, **changes):
        out = {"name": "SimCaptureAlloc", "id": "RI_SIM_CAPTURE_ALLOC", "declaration": "void* SimCaptureAlloc(unsigned int bytes, unsigned int* offset);",
               "args": [{"mode": "int", "param": "bytes"}, {"mode": "skip", "param": "offset"}], "result": "snapshot", "bytes": "bytes", "offset": "offset"}
        out.update(changes)
        return out

    def test_the_recorder_writes_the_size_and_the_handle(self):
        text = rs.render_service_end([self.spec()])
        self.assertIn("case RI_SIM_CAPTURE_ALLOC:", text)
        self.assertIn("t.AddInt(a[0].v.i);", text)
        self.assertIn("AddSnapshot(t, (c.result.kind == 1 ? c.result.v.p : nullptr), (unsigned)a[0].v.i);", text)

    def test_the_offset_is_not_a_line(self):
        text = rs.render_service_end([self.spec()])
        self.assertNotIn("offset", text)

    def test_the_replayer_returns_a_buffer_of_its_own_with_its_own_offset(self):
        text = rs.render_service_answers([self.spec()])
        self.assertIn("void* SimCaptureAlloc(unsigned int bytes, unsigned int* offset)", text)
        self.assertIn("t.AddInt((int)bytes);", text)
        self.assertIn("return SnapshotAnswer(result, (unsigned)bytes, offset);", text)

    def test_a_snapshot_needs_a_size_an_offset_and_a_pointer(self):
        self.assertEqual(rs.check_service_snapshots([self.spec()]), [])
        self.assertTrue(rs.check_service_snapshots([self.spec(bytes="nowhere")]))
        self.assertTrue(rs.check_service_snapshots([self.spec(offset="bytes")]))
        self.assertTrue(rs.check_service_snapshots([self.spec(declaration="int SimCaptureAlloc(unsigned int bytes, unsigned int* offset);")]))

    def test_another_result_is_untouched(self):
        self.assertEqual(rs.check_service_snapshots([self.spec(result="int")]), [])


class Services(unittest.TestCase):
    """N7: the services of the files, commands, sim plumbing and load groups, through the services annotation."""

    def spec(self, name, ann=None):
        return [s for s in (ann or annotations())["services"] if s["name"] == name][0]

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def errors(self, ann):
        return self.generated(ann)[1]

    def test_every_new_service_is_hooked_and_declared(self):
        files, errors, _ = self.generated()
        self.assertEqual(errors, [])
        names = {s["name"] for s in annotations()["services"]}
        for name in ("FilePrintf", "TdfRecord_FindSubRecord", "TdfRecord_GetFieldFixed", "Translate", "CommandArgs_InitArgs", "ComputeChecksum",
                     "SimAlloc", "SimAllocTagged", "SimFree", "RecordSimEvent", "SimCaptureAlloc", "FindSlotByDpid", "GetSlotDpid"):
            self.assertIn(name, names)
            self.assertIn("RI_", files[rs.SERVICES_H])

    def test_bytes_and_words_are_written_with_their_lengths(self):
        text = rs.render_service_end([self.spec("ComputeChecksum"), self.spec("RecordSimEvent")])
        self.assertIn("ReplayBytes(t, a[0].v.p, a[1].v.i);", text)
        self.assertIn("ReplayAddWords(t, (const unsigned*)a[2].v.p, (unsigned)a[3].v.i);", text)

    def test_a_handle_result_is_a_record_the_replayer_parses(self):
        spec = self.spec("TdfRecord_FindSubRecord")
        self.assertIn("AddHandle(t, OBJ_RECORD, (c.result.kind == 1 ? c.result.v.p : nullptr));", rs.render_service_end([spec]))
        self.assertIn("return (TdfRecord*)ParseHandle(OBJ_RECORD, result);", rs.render_service_answers([spec]))

    def test_a_number_written_through_a_pointer_is_an_out_line(self):
        spec = self.spec("TdfRecord_GetFieldFixed")
        self.assertIn('t.Add("\\n  o dst ");\n        t.AddInt(*(const int*)a[1].v.p);', rs.render_service_end([spec]))
        answers = rs.render_service_answers([spec])
        self.assertIn('OutValue("dst")', answers)
        self.assertIn("*dst = atoi(dstText);", answers)
        self.assertIn("return dst;", answers)

    def test_a_text_result_that_is_null_is_null(self):
        self.assertIn('if (strcmp(result, "null") == 0)\n        return nullptr;', rs.render_service_answers([self.spec("Translate")]))

    def test_a_formatted_text_is_an_argument_the_definition_builds(self):
        answers = rs.render_service_answers([self.spec("FilePrintf")])
        self.assertIn("va_start(list, fmt);", answers)
        self.assertIn("t.AddString(formatted);", answers)

    def test_a_service_in_a_source_the_replayer_links_is_answered_not_defined(self):
        services = annotations()["services"]
        linked = rs.linked_services(services)
        self.assertEqual({"SimAlloc", "SimAllocTagged", "SimFree"} - linked, set())
        answers = rs.render_service_answers(services, linked)
        self.assertNotIn("void* __cdecl SimAlloc(unsigned int size)\n{", answers)
        self.assertIn("bool ReplayAnswerImport(ReplayImport& c)", answers)
        self.assertIn("case RI_SIM_ALLOC: {", answers)
        self.assertIn("c.Return(AllocAnswer(result, (unsigned)c.args[0].v.i));", answers)

    def test_a_free_goes_on_into_the_real_function(self):
        answers = rs.render_service_answers(annotations()["services"], rs.linked_services(annotations()["services"]))
        body = answers[answers.index("case RI_SIM_FREE: {"):]
        body = body[:body.index("    }\n")]
        self.assertIn("AddAllocName(t, c.args[0].v.p);", body)
        self.assertIn("return false;", body)

    def test_an_alloc_is_named_by_its_tag_when_the_annotation_has_one(self):
        files, _, _ = self.generated()
        self.assertIn('{ "Player Info", ROK_PLAYERINFO, 185 },', files[rs.SCHEMA_H])
        self.assertIn("AddAlloc(t, (c.result.kind == 1 ? c.result.v.p : nullptr), (const char*)a[0].v.p);", files[rs.SERVICES_INC])

    def test_an_unknown_mode_or_result_is_an_error(self):
        ann = annotations()
        self.spec("Translate", ann)["result"] = "pointer"
        self.assertTrue(any("service Translate: the result pointer is none of" in e for e in self.errors(ann)))
        ann = annotations()
        self.spec("Translate", ann)["args"][0]["mode"] = "wide"
        self.assertTrue(any("the argument key has the unknown mode wide" in e for e in self.errors(ann)))

    def test_bytes_and_ints_name_an_int_argument(self):
        ann = annotations()
        self.spec("ComputeChecksum", ann)["args"][0]["length"] = "nowhere"
        self.assertTrue(any("service ComputeChecksum: the argument data needs a `length` that names an int argument" in e for e in self.errors(ann)))
        ann = annotations()
        self.spec("RecordSimEvent", ann)["args"][2]["count"] = "nowhere"
        self.assertTrue(any("service RecordSimEvent: the argument args needs a `count`" in e for e in self.errors(ann)))

    def test_an_alloc_needs_its_size_and_its_tags_a_kind_without_a_table(self):
        ann = annotations()
        del self.spec("SimAlloc", ann)["size"]
        self.assertTrue(any("service SimAlloc: an alloc result needs `size`" in e for e in self.errors(ann)))
        ann = annotations()
        ann["alloc_tags"]["Units"] = {"kind": "Unit", "size": 280}
        self.assertTrue(any("the alloc tag Units names a state kind with no table" in e for e in self.errors(ann)))

    def test_a_handle_needs_a_kind_of_file_rec_or_bank(self):
        ann = annotations()
        self.spec("TdfRecord_FindSubRecord", ann)["kind"] = "gaf"
        self.assertTrue(any("a handle result needs a `kind` of file, rec or bank" in e for e in self.errors(ann)))

    def test_a_hook_that_gives_other_arguments_is_an_error(self):
        errors = rs.check_service_hooks(annotations()["services"])
        self.assertEqual(errors, [])
        services = annotations()["services"]
        self.spec("Translate", {"services": services})["args"].append({"mode": "int", "param": "extra"})
        self.assertTrue(any("RI_TRANSLATE" in e for e in rs.check_service_hooks(services)))


class ReentrantServices(unittest.TestCase):
    """ExecuteCommandText: the commands it runs are c lines under its i line."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def spec(self):
        return [s for s in annotations()["services"] if s["name"] == "ExecuteCommandText"][0]

    def test_the_text_and_the_arguments_are_written_by_their_lengths_and_lists(self):
        text = rs.render_service_end([self.spec()])
        self.assertIn("ReplayAddTextN(t, (const char*)a[0].v.p, a[1].v.i);", text)
        self.assertIn("ReplayAddArgs(t, (const CommandArgs*)a[2].v.p);", text)
        self.assertIn("t.AddText(s_nested);\n        s_nested.Clear();", text)

    def test_the_stub_finds_its_line_among_the_nested_imports_and_plays_the_commands(self):
        answers = rs.render_service_answers([self.spec()])
        self.assertIn("const char* result = EnterNested(t);", answers)
        self.assertIn('PlayNestedCalls("services.ExecuteCommandText");', answers)

    def test_a_command_slot_is_nested_and_a_slot_that_is_not_a_command_is_not(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        slots = model["subsystems"]["wind_meteors"]["slots"]
        self.assertTrue(rs.nestable("CmdMeteor", slots["CmdMeteor"]))
        self.assertFalse(rs.nestable("UpdateWind", slots["UpdateWind"]))
        hooks = files[rs.HOOKS_H]
        body = hooks[hooks.index("RecWindMeteorsCmdMeteor("):]
        body = body[:body.index("\n}\n")]
        self.assertIn('line.Add("\\n    c wind_meteors.CmdMeteor");', body)
        self.assertIn("ImportArgv(line, a0);", body)
        self.assertIn("FinishNested(line);", body)
        self.assertIn("bool IsNestedSlot(unsigned slot)\n{\n    switch (slot) {\n    case RSL_WIND_METEORS_CMDMETEOR:", hooks)
        self.assertIn("if (on[SUB_WIND_METEORS] || any)\n        s_rec_wind_meteors.CmdMeteor = RecWindMeteorsCmdMeteor;", hooks)

    def test_the_replayer_makes_the_nested_calls_through_the_seam_table(self):
        files, _, _ = self.generated()
        slots = files[rs.SLOTS_H]
        self.assertIn("void PlayNestedCalls(const char* import)", slots)
        self.assertIn('if (CallbackIs(name, "wind_meteors.CmdMeteor")) {', slots)
        self.assertIn("g_seams.wind_meteors->CmdMeteor(&p0.args);", slots)

    def test_a_length_for_the_text_names_an_int_argument(self):
        ann = annotations()
        [s for s in ann["services"] if s["name"] == "ExecuteCommandText"][0]["args"][0]["length"] = "nowhere"
        self.assertTrue(any("service ExecuteCommandText: the argument text needs a `length`" in e for e in self.generated(ann)[1]))

    def test_a_re_entrant_service_has_an_int_result(self):
        ann = annotations()
        [s for s in ann["services"] if s["name"] == "ExecuteCommandText"][0]["result"] = "text"
        self.assertTrue(any("a re-entrant service has an int or no result" in e for e in self.generated(ann)[1]))


class RawPointers(unittest.TestCase):
    """A ptr field is a raw pointer to a named object, and a service writes the fields of its object argument as o lines."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def spec(self, name, ann=None):
        return [s for s in (ann or annotations())["services"] if s["name"] == name][0]

    def field(self, ann):
        return [f for f in ann["objects"]["Handle"]["fields"] if f["name"] == "src"][0]

    def test_a_ptr_field_has_its_own_kind_and_a_target(self):
        files, errors, _ = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        self.assertIn('{ "src", offsetof(Handle, src), sizeof(void*), ROK_HANDLE, RK_PTR, ROK_FRAMETABLE,', text)
        self.assertIn('static_assert(__is_pointer(decltype(((Handle*)nullptr)->src)), "Handle::src is a raw pointer");', text)

    def test_a_ptr_is_not_walked(self):
        self.assertNotIn("ptr", rs.WALKABLE)

    def test_a_ptr_to_a_value_kind_is_an_error(self):
        ann = annotations()
        self.field(ann)["target"] = "Vec3"
        self.assertTrue(any("Handle.src: a ptr field points at a state kind of the schema" in e for e in self.generated(ann)[1]))

    def test_the_cursor_services_write_the_fields_of_the_handle(self):
        text = rs.render_service_end([self.spec("InitGafSequence"), self.spec("StepGafSequence")])
        out = 'ImportObjectOuts(t, ROK_HANDLE, a[0].v.p, 4, "index", "value", "kind", "src");'
        self.assertEqual(text.count(out), 2)
        self.assertIn("ReplayAddObjectName(t, Table(), ROK_FRAMETABLE, a[1].v.p, true);", text)
        answers = rs.render_service_answers([self.spec("InitGafSequence")])
        self.assertIn('ApplyReach((void*)player, ROK_HANDLE, ".src");', answers)

    def test_the_reach_of_an_argument_names_fields_of_its_kind(self):
        ann = annotations()
        self.spec("StepGafSequence", ann)["args"][0]["reach"] = ["nowhere"]
        self.assertTrue(any("service StepGafSequence: the reach of player names nowhere, which is no field of Handle" in e for e in self.generated(ann)[1]))
        ann = annotations()
        self.spec("InitGafSequence", ann)["args"][2]["reach"] = ["x"]
        self.assertTrue(any("service InitGafSequence: the argument index has a `reach` and is not an object" in e for e in self.generated(ann)[1]))


class Conditions(unittest.TestCase):
    """A `when` on any field: a mask, a value, and an optional `via` field that leads to the object the condition reads."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def field(self, ann, kind, name):
        return [f for f in ann["objects"][kind]["fields"] if f["name"] == name][0]

    def errors(self, ann, kind, name, when):
        self.field(ann, kind, name)["when"] = when
        return self.generated(ann)[1]

    def line(self, text, name, kind):
        return [l for l in text.split("\n") if l.startswith('    { "%s", offsetof(%s,' % (name, kind))][0]

    def test_a_masked_condition_is_in_the_field_table(self):
        files, errors, _ = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        self.assertRegex(self.line(text, "count", "FixtureView"), r"RK_I32, 0, 0, 0, \{ \d+, 0, ROK_FIXTUREVIEW, 1u, 0 \}, 0, 0 \},")
        self.assertRegex(self.line(text, "scale", "FixtureView"), r"\{ \d+, 0, ROK_FIXTUREVIEW, 1u, 1 \}")

    def test_a_condition_through_a_field_names_that_field_and_the_kind_it_reaches(self):
        files, _, model = self.generated()
        text = files[rs.SCHEMA_H]
        names = ["%s.%s" % (f["object"], f["name"]) for f in model["object_fields"]]
        moved = self.line(text, "moved", "FixtureSlave")
        self.assertRegex(moved, r"\{ (\d+), (\d+), ROK_PLAYER, 4u, 4 \}")
        read, via = [int(x) for x in re.search(r"\{ (\d+), (\d+), ROK_PLAYER", moved).groups()]
        self.assertEqual((names[read - 1], names[via - 1]), ("Player.flags", "FixtureSlave.player"))
        low = self.line(text, "low", "FixtureSlave")
        read, via = [int(x) for x in re.search(r"\{ (\d+), (\d+), ROK_FIXTUREVIEW, 2u, 0 \}", low).groups()]
        self.assertEqual((names[read - 1], names[via - 1]), ("FixtureView.flags", "FixtureSlave.master"))

    def test_a_condition_without_a_mask_reads_every_bit(self):
        files, _, _ = self.generated()
        self.assertRegex(self.line(files[rs.SCHEMA_H], "weapon0TargetUnit", "Unit"), r"RK_UNITID, ROK_UNIT, 0, 0, \{ \d+, 0, ROK_UNIT, 4294967295u, -32768 \}, 0, 5")

    def test_a_field_without_a_condition_has_an_empty_one(self):
        files, _, _ = self.generated()
        self.assertIn("ROK_FIXTUREVIEW, RK_U16, 0, 0, 0, {}, 0, 0 },", self.line(files[rs.SCHEMA_H], "flags", "FixtureView"))

    def test_a_mask_and_a_value_that_cannot_meet_are_errors(self):
        for when, message in (({"field": "flags", "mask": 0, "equals": 0}, "non-zero integer that fits the 2 bytes"),
                              ({"field": "flags", "mask": 65536, "equals": 0}, "non-zero integer that fits the 2 bytes"),
                              ({"field": "flags", "mask": 1, "equals": 2}, "can never hold"),
                              ({"field": "flags", "mask": 1, "equals": -1}, "can never hold"),
                              ({"field": "flags", "equals": 70000}, "outside the range of FixtureView.flags (0 to 65535)"),
                              ({"field": "flags", "equals": "1"}, "`equals` of a condition is an integer")):
            errors = self.errors(annotations(), "FixtureView", "count", when)
            self.assertTrue(any("FixtureView.count" in e and message in e for e in errors), (when, errors))

    def test_the_field_a_condition_reads_is_an_integer_without_a_condition(self):
        ann = annotations()
        errors = self.errors(ann, "FixtureView", "count", {"field": "scale", "equals": 0})
        self.assertTrue(any("only an integer or flags field can be read" in e for e in errors), errors)
        errors = self.errors(annotations(), "FixtureView", "count", {"field": "total", "equals": 0})
        self.assertTrue(any("FixtureView.total, which has a `when` of its own" in e for e in errors), errors)
        errors = self.errors(annotations(), "FixtureView", "count", {"field": "nowhere", "equals": 0})
        self.assertTrue(any("nowhere, which is no field of FixtureView" in e for e in errors), errors)

    def test_a_via_names_a_plain_field_that_leads_to_an_object(self):
        for when, message in (({"via": "nowhere", "field": "flags", "equals": 0}, "nowhere, which is no field of FixtureSlave"),
                              ({"via": "moved", "field": "flags", "equals": 0}, "the `via` field moved has a `when` of its own"),
                              ({"via": "player", "field": "flags", "equals": 0}, "the condition's `kind` names the table kind it indexes"),
                              ({"via": "player", "kind": "UnitDef", "field": "flags1", "equals": 0}, "the condition's `kind` names the table kind it indexes"),
                              ({"via": "master", "kind": "FixtureView", "field": "flags", "equals": 0}, "leads to its target, and `kind` is for an integer"),
                              ({"kind": "Player", "field": "flags", "equals": 0}, "there is no `via`")):
            errors = self.errors(annotations(), "FixtureSlave", "speed", when)
            self.assertTrue(any("FixtureSlave.speed" in e and message in e for e in errors), (when, errors))

    def test_a_condition_is_a_dictionary_of_known_keys(self):
        for when in ({"field": "flags"}, {"equals": 1}, {"field": "flags", "equals": 0, "also": 1}, "flags == 0"):
            errors = self.errors(annotations(), "FixtureView", "count", when)
            self.assertTrue(any("FixtureView.count: a `when` is" in e for e in errors), (when, errors))

    def test_only_a_state_kind_has_a_condition(self):
        ann = annotations()
        self.field(ann, "Vec3", "y")["when"] = {"field": "x", "equals": 0}
        self.assertTrue(any("Vec3.y: only the field of a state kind has a `when`" in e for e in self.generated(ann)[1]))

    def test_a_unitid_field_needs_no_condition(self):
        ann = annotations()
        del self.field(ann, "Unit", "weapon0TargetUnit")["when"]
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        row = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "weapon0TargetUnit"')][0]
        self.assertIn("RK_UNITID, ROK_UNIT, 0, 0, {},", row)

    def test_a_unitid_field_without_a_condition_still_needs_its_reach(self):
        ann = annotations()
        del self.field(ann, "FixtureLane", "occupant")["reach"]
        self.assertTrue(any("FixtureLane.occupant needs a reach list" in e for e in self.generated(ann)[1]))

    def test_the_committed_lane_has_a_unitid_without_a_condition(self):
        files, errors, _ = self.generated()
        self.assertEqual(errors, [])
        row = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "occupant", offsetof(FixtureLane, unit)')][0]
        self.assertIn("RK_UNITID, ROK_UNIT, 0, 0, {},", row)


class InsideSites(unittest.TestCase):
    """`inside` lists every {kind, member} a kind is embedded at."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def test_each_site_is_a_row_of_the_site_table(self):
        files, errors, _ = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        self.assertIn('    { ROK_UNITRESOURCES, ROK_UNIT, offsetof(Unit, resourceSlot), "resourceSlot", 0, 1, 0 },', text)
        self.assertIn('    { ROK_HANDLE, ROK_FEATURE, offsetof(Feature, anim), "anim", 0, 1, 0 },', text)
        self.assertIn('    { ROK_HANDLE, ROK_FEATURE, offsetof(Feature, shadowAnim), "shadowAnim", 0, 1, 0 },', text)
        self.assertIn('    { ROK_HANDLE, ROK_SPOT, offsetof(Spot, anim), "anim", 0, 1, 0 },', text)
        self.assertIn('    { ROK_HANDLE, ROK_SPOT, offsetof(Spot, shadow), "shadow", 0, 1, 0 },', text)
        self.assertIn('    { ROK_HANDLE, ROK_FIXTUREHOLDER, offsetof(FixtureHolder, anim), "anim", 0, 1, 0 },', text)
        self.assertIn('    { ROK_HANDLE, ROK_FIXTUREHOLDER, offsetof(FixtureHolder, shadow), "shadow", 0, 1, 0 },', text)
        self.assertIn("kReplayInsideSiteCount = 12;", text)

    def test_a_kind_points_at_its_sites(self):
        files, _, _ = self.generated()
        text = files[rs.SCHEMA_H]
        self.assertIn('{ "UnitResources", sizeof(UnitResources), RS_STATE, RT_ORDER, 0, 1, 0, 0, 0 },', text)
        self.assertIn('{ "Handle", sizeof(Handle), RS_STATE, RT_ORDER, 1, 6, 0, 0, 0 },', text)
        self.assertIn('{ "Player", sizeof(Player), RS_STATE, 1, 0, 0, 0, 0, 0 },', text)

    def test_the_sites_of_a_kind_are_in_the_model(self):
        _, _, model = self.generated()
        handle = [o for o in model["objects"] if o["name"] == "Handle"][0]
        self.assertEqual(handle["inside"], [{"kind": "Feature", "member": "anim"}, {"kind": "Feature", "member": "shadowAnim"},
                                            {"kind": "Spot", "member": "anim"}, {"kind": "Spot", "member": "shadow"},
                                            {"kind": "FixtureHolder", "member": "anim"}, {"kind": "FixtureHolder", "member": "shadow"}])
        self.assertIsNone([o for o in model["objects"] if o["name"] == "Unit"][0]["inside"])

    def test_the_single_site_form_is_an_error(self):
        ann = annotations()
        ann["objects"]["UnitResources"]["inside"] = {"kind": "Unit", "member": "resourceSlot"}
        self.assertTrue(any("UnitResources inside: `inside` lists the sites" in e for e in self.generated(ann)[1]))

    def test_a_site_is_an_embed_field_of_a_state_kind(self):
        ann = annotations()
        ann["objects"]["Handle"]["inside"].append({"kind": "FixtureHolder", "member": "id"})
        self.assertTrue(any("Handle inside: the site FixtureHolder.id is no embed field of FixtureHolder that leads to Handle" in e for e in self.generated(ann)[1]))
        ann = annotations()
        ann["objects"]["Handle"]["inside"].append({"kind": "Vec3", "member": "x"})
        self.assertTrue(any("Handle inside: the site Vec3.x is not in a state kind of the schema" in e for e in self.generated(ann)[1]))
        ann = annotations()
        ann["objects"]["Handle"]["inside"].append({"kind": "FixtureHolder", "member": "anim", "extra": 1})
        self.assertTrue(any("Handle inside: a site is" in e for e in self.generated(ann)[1]))

    def test_a_site_belongs_to_one_kind(self):
        ann = annotations()
        ann["objects"]["FrameTable"]["inside"] = [{"kind": "FixtureHolder", "member": "anim"}]
        errors = self.generated(ann)[1]
        self.assertTrue(any("Handle inside: the site FixtureHolder.anim is already the site of a kind" in e for e in errors), errors)

    def test_a_container_is_not_itself_inside_a_kind_and_a_table_is_not_inside_one(self):
        ann = annotations()
        ann["objects"]["FixtureHolder"]["inside"] = [{"kind": "Unit", "member": "resourceSlot"}]
        errors = self.generated(ann)[1]
        self.assertTrue(any("Handle inside: the container FixtureHolder is itself inside another kind" in e for e in errors), errors)
        ann = annotations()
        ann["objects"]["Cell"]["inside"] = [{"kind": "FixtureHolder", "member": "anim"}]
        errors = self.generated(ann)[1]
        self.assertTrue(any("Cell inside: a kind with a table is named by its index" in e for e in errors), errors)


class FeatureKinds(unittest.TestCase):
    """Feature, Spot and Pool, and the views they hold."""

    def field(self, kind, name):
        return [f for f in annotations()["objects"][kind]["fields"] if f["name"] == name][0]

    def test_the_tables_are_the_arrays_of_the_game(self):
        kinds = annotations()["objects"]
        self.assertEqual(kinds["Feature"]["table"], {"base": "game", "member": "features", "ref": True, "count": "featureCount", "alloc": 2048})
        self.assertEqual(kinds["Spot"]["table"], {"base": "game", "member": "spots", "ref": True, "count": 2048, "alloc": 2048})
        self.assertEqual(kinds["Pool"]["table"], {"base": "game", "member": "pool", "count": 1})
        self.assertEqual(kinds["ModelState"]["fields"], [])
        self.assertEqual(self.field("Spot", "state")["target"], "ModelState")

    def test_the_two_sides_of_a_union_are_never_live_together(self):
        sprite = {"field": "flags16", "mask": 1, "equals": 1}
        model = {"field": "flags16", "mask": 1, "equals": 0}
        self.assertEqual(self.field("Feature", "filename")["when"], sprite)
        self.assertEqual(self.field("Feature", "object")["when"], model)
        via = {"via": "feature", "kind": "Feature", "field": "flags16", "mask": 1}
        for name in ("anim", "shadow"):
            self.assertEqual(self.field("Spot", name)["when"], dict(via, equals=1))
        for name in ("state", "posX", "posY", "posZ", "velX", "velY", "velZ"):
            self.assertEqual(self.field("Spot", name)["when"], dict(via, equals=0))

    def test_a_view_is_an_alias_of_the_field_it_is_recorded_as(self):
        ann = annotations()
        self.assertEqual(rs.object_targets("Feature::noobject", ann), [("Feature", "flags16")])
        self.assertEqual(rs.object_targets("Feature::footprintX", ann), [("Feature", "footprint")])
        self.assertEqual(rs.object_targets("Feature::animIds", ann), [("Feature", n) for n in ("seqburn", "seqburnshad", "seqdie", "seqdieshad", "seqreclamate", "seqreclamateshad")])
        self.assertEqual(rs.object_targets("Spot::pos2", ann), [("Spot", "posX"), ("Spot", "posY"), ("Spot", "posZ")])
        self.assertEqual(rs.object_targets("Spot::frame", ann), [("Handle", "index")])
        self.assertEqual(rs.object_targets("Spot::animIndex", ann), [("Handle", "src")])
        self.assertEqual(rs.object_targets("Spot::animHi", ann), [("Spot", "timer")])

    def test_every_row_of_the_features_functions_on_these_kinds_names_a_field(self):
        ann = annotations()
        kinds = {"Feature", "Spot", "Pool", "Handle"}
        functions = set(rs.read_assignment()["features"])
        left = sorted({"%s %s" % (r["function"], r["target"]) for r in rs.read_rows()
                       if r["function"] in functions and r["kind"] in rs.OBJECT_KINDS and r["target"].split("::")[0] in kinds and not rs.object_targets(r["target"], ann)})
        self.assertEqual(left, [])

    def test_the_rows_that_need_a_rule_have_one(self):
        ann = annotations()
        have = {(r["kind"], r["target"]) for r in ann["rules"]}
        for want in (("field_escape", "Feature::name"), ("field_escape", "FeatureName::name"), ("field_escape", "Game::anim"), ("field_escape", "Game::featureCount"),
                     ("field_escape", "Spot::frame"), ("deref_read", "*int"), ("deref_write", "*unsigned short")):
            self.assertIn(want, have)
        _, _, model = rs.generate(rs.read_rows(), ann)
        self.assertEqual(model["subsystems"]["features"]["pending"]["ruleless"], 0)


class AllOfKinds(unittest.TestCase):
    """A slot names every all-of kind it reaches."""

    def rules(self, extra=()):
        ann = annotations()
        ann["rules"].append({"kind": "field_escape", "target": "Game::p1cd5", "rule": "all-of", "object": "FixtureLane", "note": "test"})
        ann["rules"].append({"kind": "field_escape", "target": "Game::p1b8a", "rule": "all-of", "object": "FixtureCell", "note": "test"})
        rows = [row("UpdateResourceSharing", "field_escape", t) for t in ("Game::p1cd5", "Game::players", "Game::p1b8a")]
        model, errors = build(rows + list(extra), ann)
        return model["subsystems"]["economy"]["slots"]["UpdateResourceSharing"], errors

    def test_a_slot_lists_every_all_of_kind_in_kind_order(self):
        s, errors = self.rules()
        self.assertEqual([e for e in errors if "all-of" in e], [])
        self.assertEqual(s["allOf"], ["Player", "FixtureCell", "FixtureLane"])

    def test_a_kind_named_twice_is_listed_once(self):
        s, errors = self.rules([row("UpdateResourceSharing", "field_escape", "Game::players")])
        self.assertEqual([e for e in errors if "all-of" in e], [])
        self.assertEqual(s["allOf"], ["Player", "FixtureCell", "FixtureLane"])

    def test_the_generated_slot_has_a_list_of_kinds(self):
        ann = annotations()
        ann["rules"].append({"kind": "field_escape", "target": "Game::p1cd5", "rule": "all-of", "object": "FixtureLane", "note": "test"})
        files, errors, _ = rs.generate(rs.read_rows() + [row("UpdateResourceSharing", "field_escape", "Game::p1cd5")], ann)
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        self.assertIn("kReplayEconomyUpdateResourceSharingAllOf[] = { ROK_PLAYER, ROK_FIXTURELANE };", text)
        slot = [l for l in text.split("\n") if l.startswith('    { "UpdateResourceSharing", SUB_ECONOMY')][0]
        self.assertTrue(slot.endswith("kReplayEconomyUpdateResourceSharingAllOf, 2 },"), slot)

    def test_a_slot_with_one_kind_is_a_list_of_one(self):
        files, _, _ = rs.generate(rs.read_rows())
        text = files[rs.SCHEMA_H]
        self.assertIn("kReplayEconomyUpdateResourceSharingAllOf[] = { ROK_PLAYER };", text)
        slot = [l for l in text.split("\n") if l.startswith('    { "UpdateResourceSharing", SUB_ECONOMY')][0]
        self.assertTrue(slot.endswith("kReplayEconomyUpdateResourceSharingAllOf, 1 },"), slot)

    def test_a_slot_with_none_has_no_list(self):
        files, _, _ = rs.generate(rs.read_rows())
        slot = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "UpdatePlayerEconomy", SUB_ECONOMY')][0]
        self.assertTrue(slot.endswith("nullptr, 0 },"), slot)

    def test_a_kind_with_no_table_is_an_error(self):
        ann = annotations()
        ann["rules"].append({"kind": "field_escape", "target": "Game::p1cd5", "rule": "all-of", "object": "Vec3", "note": "test"})
        _, errors = build([row("UpdateResourceSharing", "field_escape", "Game::p1cd5")], ann)
        self.assertTrue(any("the all-of rule names Vec3, which is no table kind of the schema" in e for e in errors), errors)


class DeltaKinds(unittest.TestCase):
    """`delta` on a table kind."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def test_the_kind_row_says_it_is_a_delta(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        self.assertTrue(any(l.startswith('    { "FixtureCell", sizeof(FixtureCell), RS_STATE,') and l.endswith(", 0, 0, 0, 0, 1 },") for l in text.split("\n")))
        self.assertTrue(any(l.startswith('    { "FixtureLane", sizeof(FixtureLane), RS_STATE,') and l.endswith(", 0, 0, 0, 0, 0 },") for l in text.split("\n")))
        self.assertTrue([o for o in model["objects"] if o["name"] == "FixtureCell"][0]["delta"])
        self.assertNotIn("delta", [o for o in model["objects"] if o["name"] == "Player"][0])

    def test_a_delta_kind_is_a_state_kind_with_a_table(self):
        ann = annotations()
        ann["objects"]["Handle"]["delta"] = True
        errors = self.generated(ann)[1]
        self.assertTrue(any("Handle delta: only a kind with a table of Game or SimGlobals can be a delta" in e for e in errors), errors)
        self.assertTrue(any("Handle delta: a delta kind is a state kind with no sites, kinds or vtable" in e for e in errors), errors)
        ann = annotations()
        ann["objects"]["Vec3"]["delta"] = True
        errors = self.generated(ann)[1]
        self.assertTrue(any("Vec3 delta: only a kind with a table of Game or SimGlobals can be a delta" in e for e in errors), errors)

    def test_delta_is_true_or_absent(self):
        ann = annotations()
        ann["objects"]["FixtureCell"]["delta"] = "yes"
        self.assertTrue(any("FixtureCell delta: `delta` is true, or absent" in e for e in self.generated(ann)[1]))


class GameRefs(unittest.TestCase):
    """A `ref` field of Game."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def test_the_field_has_a_target_in_the_field_table(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        row = [l for l in text.split("\n") if l.startswith('    { "Game.slowest"')][0]
        self.assertTrue(row.endswith("RK_REF, 0, ROK_FIXTUREVIEW },"), row)
        self.assertLess(text.index("ROK_FIXTUREVIEW,\n"), text.index("kReplayFields[] = {"))
        field = [f for f in model["fields"] if f["name"] == "Game.slowest"][0]
        self.assertEqual((field["kind"], field["target"], field["size"]), ("ref", "FixtureView", 4))

    def test_a_scalar_field_has_no_target(self):
        files, _, _ = self.generated()
        row = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "Game.ticks"')][0]
        self.assertTrue(row.endswith("RK_I32 },"), row)

    def test_the_target_is_a_state_kind_named_at_first_sight(self):
        for target, message in (("Nowhere", "the target Nowhere is no state kind of the schema"),
                                ("Vec3", "the target Vec3 is no state kind of the schema"),
                                ("Unit", "the target Unit has a table, sites or kinds"),
                                ("Handle", "the target Handle has a table, sites or kinds"),
                                ("OrderFx", "the target OrderFx has a table, sites or kinds")):
            ann = annotations()
            ann["game_fields"]["slowest"]["target"] = target
            errors = self.generated(ann)[1]
            self.assertTrue(any("Game.slowest: " + message in e for e in errors), (target, errors))

    def test_a_ref_field_has_no_other_keys(self):
        ann = annotations()
        ann["game_fields"]["slowest"]["elem"] = "u8"
        self.assertTrue(any("Game.slowest: a ref field of Game is" in e for e in self.generated(ann)[1]))

    def test_a_ref_field_is_four_bytes(self):
        ann = annotations()
        ann["game_fields"]["flags_2a44"] = {"kind": "ref", "target": "FixtureView"}
        with self.assertRaises(rs.Error):
            self.generated(ann)


class ArgumentShapes(unittest.TestCase):
    """#765: an out vector of value structs, a cast on a void*, an array sized by a state path, and a nullable in/out value."""

    SPANS = "std::vector<FixtureSpan, std::allocator<FixtureSpan> >*"
    VECTOR = {"mode": "vector", "dir": "out", "elem": {"mode": "value", "kind": "FixtureSpan"}}
    INOUT = {"mode": "struct", "kind": "FixtureAmount", "dir": "inout", "nullable": True}
    SCALAR_INOUT = {"mode": "value", "dir": "inout", "nullable": True}

    def slot(self, params, notes, ret="void", result=None):
        ann = annotations()
        args = [rs.classify_arg({"type": t, "name": n}, notes.get(n), ann) for t, n in params]
        res = rs.classify_result(ret, result, ann)
        return {"args": args, "result": res, "ret": ret, "conv": "__stdcall", "params": [t for t, _ in params]}, ann

    def errors(self, params, notes, ret="void", result=None):
        s, ann = self.slot(params, notes, ret, result)
        return rs.check_forms("x.Y", s["args"], s["result"], ann)

    def generated(self, ann):
        return rs.generate(rs.read_rows(), ann)

    def test_an_out_vector_is_recorded_after_the_import_line_and_pushed_by_the_stub(self):
        s, _ = self.slot([("int", "owner"), (self.SPANS, "goals")], {"goals": self.VECTOR})
        wrapper = "\n".join(rs.wrapper("orders", {}, "Fill", s, {}, True))
        self.assertIn('ImportOutValues(line, "goals", ROK_FIXTURESPAN, a1);', wrapper)
        self.assertNotIn("ImportPending", wrapper)
        stub = "\n".join(rs.import_stub("orders", "Fill", s))
        self.assertIn('ApplyValues(a1, "goals", ROK_FIXTURESPAN);', stub)
        self.assertNotIn('"?"', stub)

    def test_an_out_vector_is_in_no_argument_line_of_the_slot(self):
        s, _ = self.slot([(self.SPANS, "goals")], {"goals": self.VECTOR})
        for phase in "aio":
            self.assertEqual(rs.arg_out(s["args"][0], 0, phase, {}), [])

    def test_an_out_vector_is_an_out_vector_of_values_of_its_own_type(self):
        self.assertEqual(self.errors([(self.SPANS, "goals")], {"goals": self.VECTOR}), [])
        cases = [(dict(self.VECTOR, dir="in"), self.SPANS, "only a vector out is recorded"),
                 ({"mode": "vector", "dir": "out"}, self.SPANS, "needs an `elem` of mode value"),
                 (dict(self.VECTOR, elem={"mode": "object", "kind": "Unit"}), self.SPANS, "needs an `elem` of mode value"),
                 (dict(self.VECTOR, elem={"mode": "value", "kind": "Unit"}), self.SPANS, "needs a `kind` that is a value kind"),
                 (dict(self.VECTOR, elem={"mode": "value", "kind": "Point16"}), self.SPANS,
                  "is %s, and a vector of the kind Point16 is std::vector<Point16, std::allocator<Point16> >*" % self.SPANS)]
        for note, ctype, want in cases:
            errors = self.errors([(ctype, "goals")], {"goals": note})
            self.assertTrue(any(want in e for e in errors), (note, errors))

    def test_a_void_pointer_is_refused_without_a_cast_and_takes_the_modes_of_its_kind_with_one(self):
        note = {"mode": "struct", "kind": "Vec3", "dir": "in", "nullable": True}
        errors = self.errors([("void*", "pos")], {"pos": note})
        self.assertTrue(any("the argument pos is void*, and the kind Vec3 is Vec3*" in e for e in errors), errors)
        self.assertEqual(self.errors([("void*", "pos")], {"pos": dict(note, cast="Vec3*")}), [])

    def test_a_cast_is_applied_in_the_recorder_the_stub_and_the_runner(self):
        s, _ = self.slot([("void*", "pos")], {"pos": {"mode": "struct", "kind": "Vec3", "dir": "in", "nullable": True, "cast": "Vec3*"}})
        self.assertEqual(rs.arg_out(s["args"][0], 0, "a", {}), ['ArgStruct(s_call, "a", "pos", ROK_VEC3, ((Vec3*)a0));'])
        runner = "\n".join(rs.runner("orders", "NewOrder", s))
        self.assertIn('Vec3* p0 = (Vec3*)ParseStructOpt("pos", ROK_VEC3, &p0_value);', runner)
        v, _ = self.slot([("void*", "pos")], {"pos": {"mode": "vec3", "label": "pos", "cast": "Vec3*"}})
        self.assertIn("AddVec3(t, ((Vec3*)a0));", "\n".join(rs.import_stub("orders", "Place", v)))
        self.assertEqual(rs.arg_out(v["args"][0], 0, "i", {}), ['ImportVec3(line, "pos", ((Vec3*)a0));'])

    def test_a_cast_is_for_a_void_pointer_and_a_pointer_mode(self):
        note = {"mode": "struct", "kind": "Vec3", "dir": "in", "cast": "Vec3*"}
        errors = self.errors([("Vec3*", "pos")], {"pos": note})
        self.assertTrue(any("is Vec3*, and only a void* argument takes a `cast`" in e for e in errors), errors)
        errors = self.errors([("void*", "pos")], {"pos": dict(note, cast="Vec3")})
        self.assertTrue(any("the `cast` of the argument pos is a pointer type other than void*" in e for e in errors), errors)
        errors = self.errors([("void*", "pos")], {"pos": {"mode": "scalar", "cast": "Vec3*"}})
        self.assertTrue(any("has a `cast` and the mode scalar" in e for e in errors), errors)

    def test_a_cast_kind_that_does_not_fit_is_an_error(self):
        errors = self.errors([("void*", "pos")], {"pos": {"mode": "struct", "kind": "Point16", "dir": "in", "cast": "Vec3*"}})
        self.assertTrue(any("the argument pos is Vec3*, and the kind Point16 is Point16*" in e for e in errors), errors)

    def ids(self, mode, length, **extra):
        return self.slot([("void*", "ids")], {"ids": dict({"mode": mode, "length": length}, **extra)}, "int", {"mode": "scalar"})

    def test_a_length_that_is_a_state_path_is_read_from_the_state_at_the_call(self):
        s, _ = self.ids("bytes-in", "Game.net.unitCount", elem="i32")
        lines, expr = rs.length_parts(s["args"][0], 0)
        self.assertEqual(lines, ["const auto* l0_0 = ReplayStep(g_game);",
                                 "const auto* l0_1 = l0_0 != nullptr ? ReplayStep(l0_0->net) : nullptr;",
                                 "int len0 = l0_1 != nullptr ? (int)l0_1->unitCount : 0;"])
        self.assertEqual(expr, "(len0 * 4)")
        wrapper = "\n".join(rs.wrapper("units", {}, "Read", s, {}, True))
        self.assertIn("int len0 = l0_1 != nullptr ? (int)l0_1->unitCount : 0;", wrapper)
        self.assertLess(wrapper.index("int len0"), wrapper.index("ReplayText line;"))
        self.assertIn("ImportBytes(line, a0, (len0 * 4));", wrapper)
        stub = "\n".join(rs.import_stub("units", "Read", s))
        self.assertIn("AddBytes(t, a0, (len0 * 4));", stub)

    def test_a_state_path_without_elem_is_a_length_in_bytes(self):
        s, _ = self.ids("bytes-out", "Game.width")
        lines, expr = rs.length_parts(s["args"][0], 0)
        self.assertEqual((len(lines), expr), (2, "len0"))
        wrapper = "\n".join(rs.wrapper("units", {}, "Fill", s, {}, True))
        self.assertIn('ImportOutBytes(line, "ids", a0, (int)result, len0);', wrapper)
        self.assertIn('ApplyBytes(a0, len0, "ids");', "\n".join(rs.import_stub("units", "Fill", s)))

    def test_a_length_that_is_an_argument_is_what_it_was(self):
        s, _ = self.slot([("void*", "src"), ("int", "len")], {"src": {"mode": "bytes-in", "length": "len"}}, "int", {"mode": "scalar"})
        self.assertEqual(self.errors([("void*", "src"), ("int", "len")], {"src": {"mode": "bytes-in", "length": "len"}}, "int", {"mode": "scalar"}), [])
        wrapper = "\n".join(rs.wrapper("files", {}, "Write", s, {}, True))
        self.assertIn("ImportBytes(line, a0, a1);", wrapper)
        self.assertNotIn("ReplayStep", wrapper)

    def test_a_length_names_an_integer_argument_or_a_path_through_game(self):
        cases = [("nothing", "needs a `length` that names another argument or a path through Game"),
                 ("ids", "needs a `length` that names another argument or a path through Game"),
                 ("name", "is no integer"),
                 ("Other.net.unitCount", "needs a `length` that names another argument or a path through Game"),
                 ("Game.nowhere.unitCount", "starts at nowhere, which is no member of Game")]
        for length, want in cases:
            errors = self.errors([("void*", "ids"), ("char*", "name")], {"ids": {"mode": "bytes-in", "length": length}}, "int", {"mode": "scalar"})
            self.assertTrue(any(want in e for e in errors), (length, errors))
        errors = self.errors([("void*", "ids")], {"ids": {"mode": "bytes-in", "length": "Game.width", "elem": "i128"}}, "int", {"mode": "scalar"})
        self.assertTrue(any("the `elem` of the argument ids is one of" in e for e in errors), errors)

    def test_an_in_out_value_goes_in_as_a_value_and_comes_out_as_an_o_line(self):
        s, _ = self.slot([("FixtureAmount*", "count")], {"count": self.INOUT})
        a = s["args"][0]
        self.assertEqual(rs.arg_out(a, 0, "a", {}), ['ArgInOut(s_call, "count", ROK_FIXTUREAMOUNT, a0);'])
        self.assertEqual(rs.arg_out(a, 0, "i", {}), ["ImportInOut(line, ROK_FIXTUREAMOUNT, a0);"])
        self.assertEqual(rs.arg_out(a, 0, "o", {}), ['ArgInOutExit(s_tail, "count", ROK_FIXTUREAMOUNT, a0);'])
        wrapper = "\n".join(rs.wrapper("units", {}, "Query", s, {}, True))
        self.assertIn('ImportOutInOut(line, "count", ROK_FIXTUREAMOUNT, a0);', wrapper)
        stub = "\n".join(rs.import_stub("units", "Query", s))
        self.assertIn("AddInOut(t, ROK_FIXTUREAMOUNT, a0);", stub)
        self.assertIn('ApplyInOut(a0, "count", ROK_FIXTUREAMOUNT);', stub)

    def test_the_replayer_passes_an_in_out_value_by_address_or_null_and_compares_it_after(self):
        s, _ = self.slot([("FixtureAmount*", "count")], {"count": self.INOUT}, "int", {"mode": "scalar"})
        runner = "\n".join(rs.runner("units", "Query", s))
        self.assertIn('FixtureAmount* p0 = (FixtureAmount*)ParseInOut("count", ROK_FIXTUREAMOUNT, &g_inout_units_Query_0);', runner)
        self.assertIn('SetClosing("count ", "result");', runner)
        self.assertIn('CompareInOut("count", ROK_FIXTUREAMOUNT, g_inout_units_Query_0_null ? nullptr : &g_inout_units_Query_0);', runner)
        self.assertIn("g_seams.units->Query(p0)", runner)

    def test_an_in_out_value_can_be_nullable_and_an_out_value_cannot(self):
        self.assertEqual(self.errors([("FixtureAmount*", "count")], {"count": self.INOUT}), [])
        self.assertEqual(self.errors([("FixtureAmount*", "count")], {"count": dict(self.INOUT, nullable=False)}), [])
        errors = self.errors([("FixtureAmount*", "count")], {"count": dict(self.INOUT, dir="out")})
        self.assertTrue(any("only a struct in or inout can be nullable" in e for e in errors), errors)
        errors = self.errors([("FixtureAmount*", "count")], {"count": dict(self.INOUT, dir="both")})
        self.assertTrue(any("has the direction both" in e for e in errors), errors)

    def test_an_in_out_value_is_of_a_value_kind_of_its_own_type(self):
        errors = self.errors([("FixtureAmount*", "count")], {"count": dict(self.INOUT, kind="Player")})
        self.assertTrue(any("needs a `kind` that is a value kind" in e for e in errors), errors)
        errors = self.errors([("int*", "count")], {"count": self.INOUT})
        self.assertTrue(any("the argument count is int*, and the kind FixtureAmount is FixtureAmount*" in e for e in errors), errors)

    def test_a_scalar_in_out_value_is_the_value_in_parentheses_and_an_o_line(self):
        s, _ = self.slot([("int*", "p2")], {"p2": self.SCALAR_INOUT})
        a = s["args"][0]
        self.assertEqual(rs.arg_out(a, 0, "a", {}), ['ArgInOutScalar(s_call, "p2", a0);'])
        self.assertEqual(rs.arg_out(a, 0, "i", {}), ["ImportInOutScalar(line, a0);"])
        self.assertEqual(rs.arg_out(a, 0, "o", {}), ['ArgInOutScalarExit(s_tail, "p2", a0);'])
        wrapper = "\n".join(rs.wrapper("units", {}, "Query", s, {}, True))
        self.assertIn('ImportOutInOutScalar(line, "p2", a0);', wrapper)
        stub = "\n".join(rs.import_stub("units", "Query", s))
        self.assertIn("AddInOutScalar(t, a0);", stub)
        self.assertIn('ApplyInOutScalar(a0, "p2");', stub)

    def test_the_replayer_passes_a_scalar_in_out_value_by_address_or_null_and_compares_it_after(self):
        s, _ = self.slot([("unsigned char*", "p4")], {"p4": self.SCALAR_INOUT}, "int", {"mode": "scalar"})
        runner = "\n".join(rs.runner("units", "Query", s))
        self.assertIn('unsigned char* p0 = ParseInOutScalar("p4", &g_inout_units_Query_0);', runner)
        self.assertIn('SetClosing("p4 ", "result");', runner)
        self.assertIn('CompareInOutScalar("p4", g_inout_units_Query_0_null ? nullptr : &g_inout_units_Query_0);', runner)
        self.assertIn("g_seams.units->Query(p0)", runner)

    def test_a_scalar_in_out_value_is_a_pointer_to_a_scalar_without_a_kind(self):
        for ctype in ("int*", "short*", "unsigned char*", "unsigned short*", "unsigned int*"):
            self.assertEqual(self.errors([(ctype, "p2")], {"p2": self.SCALAR_INOUT}), [], ctype)
        for ctype in ("long*", "char*", "float*", "int"):
            errors = self.errors([(ctype, "p2")], {"p2": self.SCALAR_INOUT})
            self.assertTrue(any("a scalar in/out value is a pointer to one of" in e for e in errors), (ctype, errors))
        errors = self.errors([("int*", "p2")], {"p2": dict(self.SCALAR_INOUT, kind="FixtureAmount")})
        self.assertTrue(any("a scalar in/out value, which takes no `kind`" in e for e in errors), errors)
        errors = self.errors([("FixtureAmount", "p2")], {"p2": {"mode": "value", "kind": "FixtureAmount", "dir": "out"}})
        self.assertTrue(any("only inout is allowed on a value" in e for e in errors), errors)

    def test_query_script_takes_its_four_values_as_scalar_in_out(self):
        files, errors, model = self.generated(annotations())
        self.assertEqual(errors, [])
        args = model["subsystems"]["units"]["slots"]["QueryScript"]["args"]
        self.assertEqual([a["mode"] for a in args[2:]], ["value"] * 4)
        self.assertIn('ArgInOutScalar(s_call, "p5", a5);', files[rs.HOOKS_H])

    def test_a_slot_with_an_in_out_value_gets_an_after_function(self):
        ann = annotations()
        ann["subsystems"]["targeting"]["slots"]["SetWeaponTargetPos"]["args"]["pos"] = {"mode": "struct", "kind": "Vec3", "dir": "inout", "nullable": True}
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        slots = files[rs.SLOTS_H]
        self.assertIn("void After_targeting_SetWeaponTargetPos()", slots)
        self.assertIn('if (strcmp(subsystem, "targeting") == 0 && strcmp(slot, "SetWeaponTargetPos") == 0)\n        After_targeting_SetWeaponTargetPos();', slots)
        self.assertIn('ArgInOut(s_call, "pos", ROK_VEC3, a1);', files[rs.HOOKS_H])

    def test_the_fixture_slots_are_stubs_of_the_native_self_test(self):
        files, errors, _ = self.generated(annotations())
        self.assertEqual(errors, [])
        text = files[rs.SHAPES_SLOTS]
        for want in ('void __stdcall Imp_fixture_FillGoals(int a0, %s a1)' % self.SPANS, 'ApplyValues(a1, "goals", ROK_FIXTURESPAN);',
                     "int __stdcall Imp_fixture_PlaceAt(void* a0)", "AddVec3(t, ((Vec3*)a0));",
                     "int __stdcall Imp_fixture_Query(FixtureAmount* a0, FixtureAmount* a1)", 'ApplyInOut(a1, "second", ROK_FIXTUREAMOUNT);',
                     "int __stdcall Imp_fixture_ReadIds(void* a0)", "AddBytes(t, a0, (len0 * 4));",
                     "int __stdcall Imp_fixture_FillIds(void* a0)", 'ApplyBytes(a0, (len0 * 4), "ids");',
                     "int __stdcall Imp_fixture_Measure(int* a0, short* a1, unsigned char* a2)", 'ApplyInOutScalar(a2, "p4");'):
            self.assertIn(want, text)

    def test_a_fixture_slot_the_runner_can_pass_gets_a_runner_that_calls_a_native_function(self):
        files, errors, _ = self.generated(annotations())
        self.assertEqual(errors, [])
        text = files[rs.SHAPES_SLOTS]
        for want in ("void Run_fixture_Query()", "void After_fixture_Query()", "g_result_fixture_Query = Native_fixture_Query(p0, p1);",
                     "void Run_fixture_Measure()", "void After_fixture_Measure()", "g_result_fixture_Measure = Native_fixture_Measure(p0, p1, p2);"):
            self.assertIn(want, text)
        for slot in ("FillGoals", "PlaceAt", "ReadIds", "FillIds"):
            self.assertNotIn("Run_fixture_%s()" % slot, text)

    def test_the_fixture_recorder_wraps_every_fixture_slot_in_the_recorders_own_scope(self):
        files, errors, _ = self.generated(annotations())
        self.assertEqual(errors, [])
        text = files[rs.FIXTURE_HOOKS_INC]
        for want in ("SUB_FIXTURE = SUB_COUNT,", "RSL_FIXTURE_MEASURE = RSL_COUNT + 5", "int (__stdcall* Measure)(int*, short*, unsigned char*);",
                     "int __stdcall RecFixtureMeasure(int* a0, short* a1, unsigned char* a2)", "SlotScope scope(RSL_FIXTURE_MEASURE, SUB_FIXTURE, roots);",
                     'ArgInOutScalar(s_call, "p3", a1);', 'ImportOutInOutScalar(line, "p4", a2);', 'ArgInOutExit(s_tail, "second", ROK_FIXTUREAMOUNT, a1);',
                     "void __stdcall RecFixtureFillGoals(int a0, std::vector<FixtureSpan, std::allocator<FixtureSpan> >* a1)",
                     "FixtureSeam s_rec_fixture = {", "    RecFixtureFillIds,"):
            self.assertIn(want, text)

    def test_a_runner_calls_the_target_it_is_given(self):
        s, _ = self.slot([("int*", "p2")], {"p2": self.SCALAR_INOUT}, "int", {"mode": "scalar"})
        runner = "\n".join(rs.runner("fixture", "Measure", s, "Native_fixture_Measure"))
        self.assertIn("g_result_fixture_Measure = Native_fixture_Measure(p0);", runner)

    def test_a_fixture_slot_is_checked_like_a_slot(self):
        fixtures = json.loads(rs.FIXTURES_JSON.read_text())
        fixtures["fixtures"]["FillGoals"]["args"]["goals"]["dir"] = "in"
        fixtures["fixtures"]["Query"]["args"]["nowhere"] = {"mode": "scalar"}
        _, errors, _ = rs.generate(rs.read_rows(), rs.load_annotations(), None, fixtures)
        self.assertTrue(any("fixtures.FillGoals: the argument goals is a vector, and only a vector out is recorded" in e for e in errors), errors)
        self.assertTrue(any("fixtures.Query: the note for nowhere names no parameter" in e for e in errors), errors)


class ElementForms(unittest.TestCase):
    """Arrays of refs, arrays of embeds, tables behind a ref field, sites in an array element, `vptr` and embedded polymorphic kinds."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def errors(self, ann):
        return self.generated(ann)[1]

    def has(self, ann, text):
        errors = self.errors(ann)
        self.assertTrue(any(text in e for e in errors), (text, errors))

    def row(self, text, start):
        return [l for l in text.split("\n") if l.startswith(start)][0]

    def test_the_committed_fixtures_have_no_errors(self):
        self.assertEqual(self.errors(None), [])

    def test_an_array_of_refs_has_a_count_and_a_stride(self):
        files, _, model = self.generated()
        text = files[rs.FIXTURE_SCHEMA_H]
        self.assertIn("    { 4, sizeof(unsigned) },", text)
        self.assertRegex(self.row(text, '    { "timers", offsetof(FixtureManager, timers)'), r"RK_REFS, ROK_FIXTURETIMERBASE, 0, 0, \{\}, 0, 0, \d+ \},")
        self.assertIn("FixtureManager::timers holds Ref<FixtureTimerBase>", text)
        self.assertEqual([e["count"] for e in model["elems"] if e["kind"] == "refs"], [4])

    def test_an_array_of_refs_is_checked(self):
        for change, message in (({"count": 0}, "FixtureManager.timers: `count` is a positive integer"),
                                ({"count": "4"}, "FixtureManager.timers: `count` is a positive integer"),
                                ({"target": "Vec3"}, "the target Vec3 is no state kind")):
            ann = annotations()
            next(f for f in ann["objects"]["FixtureManager"]["fields"] if f["name"] == "timers").update(change)
            self.has(ann, message)

    def test_an_array_of_refs_to_a_base_needs_poly(self):
        ann = annotations()
        del next(f for f in ann["objects"]["FixtureManager"]["fields"] if f["name"] == "timers")["poly"]
        self.has(ann, "FixtureManager.timers: points at the base kind FixtureTimerBase and needs \"poly\"")
        ann = annotations()
        next(f for f in ann["objects"]["FixtureManager"]["fields"] if f["name"] == "timers").update({"target": "FixtureGroup", "poly": "FixtureGroup"})
        self.has(ann, "FixtureManager.timers: `poly` names the base kind that the ref points at")

    def test_an_array_of_embeds_is_a_site_with_a_stride(self):
        files, errors, model = self.generated()
        text = files[rs.FIXTURE_SCHEMA_H]
        self.assertIn('    { ROK_FIXTURECHANNEL, ROK_FIXTURESCRIPT, offsetof(FixtureScript, channels[0]), "channels[]", sizeof(((FixtureScript*)nullptr)->channels[0]), 4, 0 },', text)
        self.assertIn("FixtureScript::channels holds 4 elements", text)
        self.assertEqual(model["sites"]["FixtureChannel"][0]["count"], 4)

    def test_a_site_in_an_array_element_names_the_member_of_each_element(self):
        files, _, _ = self.generated()
        text = files[rs.FIXTURE_SCHEMA_H]
        self.assertIn('    { ROK_FIXTURECB, ROK_FIXTURERIG, offsetof(FixtureRig, slots[0].cb), "slots[].cb", sizeof(((FixtureRig*)nullptr)->slots[0]), 3, 0 },', text)
        self.assertIn("sizeof(((FixtureRig*)nullptr)->slots[0].cb) == sizeof(FixtureCb)", text)

    def test_a_site_in_an_array_needs_an_embeds_field(self):
        ann = annotations()
        ann["objects"]["FixtureCb"]["inside"] = [{"kind": "FixtureRig", "member": "slots[].nowhere"}]
        self.has(ann, "FixtureCb inside: the site FixtureRig.slots[].nowhere is no embeds field of FixtureRig that leads to FixtureCb")
        ann = annotations()
        ann["objects"]["FixtureChannel"]["inside"] = [{"kind": "FixtureScript", "member": "channels"}]
        self.has(ann, "FixtureChannel inside: the site FixtureScript.channels is no embed field of FixtureScript that leads to FixtureChannel")

    def test_an_array_of_embeds_is_checked(self):
        for change, message in (({"count": 0}, "FixtureScript.channels: `count` is a positive integer"),
                                ({"member": "channels"}, "the member of an array of embeds has one `[]`"),
                                ({"poly": "FixtureChannel"}, "an array of embeds names one kind, and has no `poly`")):
            ann = annotations()
            next(f for f in ann["objects"]["FixtureScript"]["fields"] if f["name"] == "channels").update(change)
            self.has(ann, message)

    def test_a_table_behind_a_ref_field_is_a_site_of_its_owner(self):
        files, errors, model = self.generated()
        text = files[rs.FIXTURE_SCHEMA_H]
        self.assertIn('    { ROK_FIXTUREGROUP, ROK_FIXTUREOWNER, offsetof(FixtureOwner, groups), "groups[]", sizeof(FixtureGroup), 4, 1 },', text)
        self.assertTrue(self.row(text, '    { "FixtureGroup", sizeof(FixtureGroup), RS_STATE, RT_ORDER,'))
        self.assertEqual([t["kind"] for t in model["tables"] if t["kind"].startswith("Fixture")], ["FixtureCell", "FixtureLane", "FixtureEye", "FixtureTally"])

    def test_a_table_behind_a_ref_field_is_checked(self):
        cases = (({"base": {"object": "Nowhere", "field": "groups"}, "count": 4}, "the base Nowhere is no state kind"),
                 ({"base": {"object": "FixtureOwner", "field": "id"}, "count": 4}, "the base FixtureOwner.id is no ref field that leads to FixtureGroup"),
                 ({"base": {"object": "FixtureOwner", "field": "groups"}, "count": 0}, "the count is a positive integer"),
                 ({"base": {"object": "FixtureOwner", "field": "groups", "x": 1}, "count": 4}, "a table behind a ref field is"))
        for table, message in cases:
            ann = annotations()
            ann["objects"]["FixtureGroup"]["table"] = table
            self.has(ann, message)

    def test_a_table_behind_a_ref_field_is_no_delta_all_of_or_inside_kind(self):
        ann = annotations()
        ann["objects"]["FixtureGroup"]["delta"] = True
        self.has(ann, "FixtureGroup delta: only a kind with a table of Game or SimGlobals can be a delta")
        ann = annotations()
        ann["objects"]["FixtureGroup"]["inside"] = [{"kind": "FixtureOwner", "member": "id"}]
        self.has(ann, "a kind in a table behind a ref field is a state kind with no sites")
        ann = annotations()
        ann["rules"].append({"kind": "field_escape", "target": "Game::p1cd5", "rule": "all-of", "object": "FixtureGroup", "note": "test"})
        errors = rs.generate(rs.read_rows() + [row("UpdateResourceSharing", "field_escape", "Game::p1cd5")], ann)[1]
        self.assertTrue(any("the all-of rule names FixtureGroup, which is no table kind of the schema" in e for e in errors), errors)

    def test_a_vptr_kind_has_its_global_in_the_vtable_functions(self):
        files, _, _ = self.generated()
        text = files[rs.FIXTURE_VTABLES_CPP]
        self.assertIn("extern void (* const g_fixtureCbVtable[])();", text)
        self.assertIn("case ROK_FIXTURECB:\n        return g_fixtureCbVtable;", text)
        self.assertIn('return "g_fixtureCbVtable";', text)
        self.assertIn("FixtureCb has a vptr that the code sets by hand", files[rs.FIXTURE_SCHEMA_H])
        self.assertNotIn("Fixture", files[rs.VTABLES_CPP])

    def test_a_vptr_is_checked(self):
        for change, message in (({"vptr": "not a name"}, "FixtureCb: the vptr is the name of a global"),
                                ({"vptr": "g_fixtureAttachVtable"}, "FixtureCb: the vptr g_fixtureAttachVtable is also the vtable of FixtureAttach"),
                                ({"vtable": "g_other"}, "FixtureCb: a kind with a vptr is a state kind with no table, kinds or vtable")):
            ann = annotations()
            ann["objects"]["FixtureCb"].update(change)
            self.has(ann, message)

    def test_an_embedded_polymorphic_kind_has_a_vtable_and_a_site(self):
        files, errors, _ = self.generated()
        self.assertEqual(errors, [])
        self.assertIn("static_assert(__is_polymorphic(FixtureAttach)", files[rs.FIXTURE_SCHEMA_H])
        self.assertIn('    { ROK_FIXTUREATTACH, ROK_FIXTUREORDER, offsetof(FixtureOrder, link), "link", 0, 1, 0 },', files[rs.FIXTURE_SCHEMA_H])
        ann = annotations()
        del ann["objects"]["FixtureAttach"]["inside"]
        self.has(ann, "FixtureAttach: has a vtable and is a kind of no base, and is embedded nowhere")

    def test_a_pointer_to_an_embedded_kind_is_an_object_argument_named_by_its_site(self):
        ann = annotations()
        ann["subsystems"]["weapons"]["slots"]["WeaponAimCobComplete"] = {"args": {"callback": {"mode": "object", "kind": "FixtureCb", "line": "callback"}}}
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        hooks = files[rs.HOOKS_H]
        body = hooks[hooks.index("RecWeaponsWeaponAimCobComplete"):]
        body = body[:body.index("\n}\n")]
        self.assertIn('ArgObject(s_call, "callback", ROK_FIXTURECB, a0);', body)
        self.assertIn("roots.Add(ROK_FIXTURECB, a0);", body)

    def test_an_import_takes_the_same_object_argument(self):
        ann = annotations()
        slot = ann["subsystems"]["units"]["slots"]["StartScriptWithArgs"] = {"args": {"callback": {"mode": "object", "kind": "FixtureCb", "line": "callback"}}}
        ann["subsystems"]["weapons"]["imports"] = ann["subsystems"]["weapons"].get("imports", []) + ["units.StartScriptWithArgs"]
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        hooks = files[rs.HOOKS_H]
        body = hooks[hooks.index("RecUnitsStartScriptWithArgs"):]
        body = body[:body.index("\n}\n")]
        self.assertIn("ImportObject(line, ROK_FIXTURECB, a2);", body)
        del slot


class CapacityTables(unittest.TestCase):
    """A table with a live count and room for more: a `capacity`, and `"alloc": "module"` for memory the module frees."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def has(self, ann, text):
        errors = self.generated(ann)[1]
        self.assertTrue(any(text in e for e in errors), (text, errors))

    def table(self, ann, **change):
        ann["objects"]["FixtureEye"]["table"].update(change)
        return ann

    def test_the_committed_fixtures_have_no_errors(self):
        self.assertEqual(self.generated()[1], [])

    def test_a_global_table_has_a_capacity_and_the_module_flag(self):
        files, _, model = self.generated()
        eyes = {t["kind"]: t for t in model["tables"]}["FixtureEye"]
        self.assertEqual((eyes["module"], eyes["alloc"], eyes["capacity"]), (True, 0, [{"const": 4}, {"const": 1}]))
        self.assertEqual(eyes["count"], [{"member": "hotRadarCount", "shift": 0}, {"const": 1}])
        text = files[rs.SCHEMA_H]
        self.assertIn("{ 0, 1, offsetof(Game, hotRadar), { { offsetof(Game, hotRadarCount), (unsigned char)sizeof(((Game*)nullptr)->hotRadarCount), 0 }, { 1, 0, 0 } }, 0, "
                      "{ { 4, 0, 0 }, { 1, 0, 0 } }, 1, 1 },", text)
        self.assertIn("{ 0, 1, offsetof(Game, p1b8a), { { 9000, 0, 0 }, { 1, 0, 0 } }, 9000 },", text)

    def test_a_capacity_may_be_a_member(self):
        ann = self.table(annotations(), capacity="hotRadarCount")
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        self.assertIn("0, { { offsetof(Game, hotRadarCount), (unsigned char)sizeof(((Game*)nullptr)->hotRadarCount), 0 }, { 1, 0, 0 } }, 1, 1 },", files[rs.SCHEMA_H])

    def test_a_table_without_a_capacity_has_the_old_row(self):
        ann = self.table(annotations(), alloc=3)
        del ann["objects"]["FixtureEye"]["table"]["capacity"]
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        self.assertIn("{ 0, 1, offsetof(Game, hotRadar), { { offsetof(Game, hotRadarCount), (unsigned char)sizeof(((Game*)nullptr)->hotRadarCount), 0 }, { 1, 0, 0 } }, 3 },", files[rs.SCHEMA_H])

    def test_a_global_table_with_a_capacity_is_checked(self):
        ann = annotations()
        del ann["objects"]["FixtureEye"]["table"]["capacity"]
        self.has(ann, "FixtureEye table: \"alloc\": \"module\" needs a capacity")
        self.has(self.table(annotations(), alloc=4), "a table with a capacity has no integer alloc")
        self.has(self.table(annotations(), alloc="elsewhere"), "FixtureEye table: a table with a capacity has no integer alloc")
        self.has(self.table(annotations(), ref=False), "only a ref table can be allocated by the module")
        self.has(self.table(annotations(), capacity=0), "the capacity is positive")
        self.has(self.table(annotations(), count=9, capacity=4), "the count 9 is more than the capacity 4")
        self.has(self.table(annotations(), capacity="a*b*c"), "the length `a*b*c`")

    def test_the_count_and_the_capacity_are_recorded_block_fields(self):
        ann = annotations()
        del ann["game_fields"]["hotRadarCount"]
        self.has(ann, "FixtureEye table: the count names hotRadarCount, which is no tracked field of Game")
        self.has(self.table(annotations(), capacity="autoFollowTicks"), "FixtureEye table: the capacity names autoFollowTicks, which is no tracked field of Game")

    def test_a_slot_that_reaches_the_table_reads_the_capacity_member(self):
        ann = self.table(annotations(), capacity="hotRadarCount")
        ann["rules"].append({"kind": "field_escape", "target": "Game::hotRadar", "rule": "all-of", "object": "FixtureEye", "note": "test"})
        errors = rs.generate(rs.read_rows() + [row("UpdateResourceSharing", "field_escape", "Game::hotRadar")], ann)[1]
        self.assertTrue(any("economy.UpdateResourceSharing reaches FixtureEye, whose capacity is Game.hotRadarCount" in e for e in errors), errors)
        ann["subsystems"]["economy"]["slots"].setdefault("UpdateResourceSharing", {})["reads"] = ["Game.hotRadarCount"]
        errors = rs.generate(rs.read_rows() + [row("UpdateResourceSharing", "field_escape", "Game::hotRadar")], ann)[1]
        self.assertEqual([e for e in errors if "reaches" in e], [])

    def test_a_table_behind_a_ref_field_has_a_capacity_that_is_a_member_of_the_owner(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        site = model["sites"]["FixtureNode"][0]
        self.assertEqual((site["capped"], site["module"], site["room"]), (True, True, [{"member": "capacity", "shift": 0}, {"const": 1}]))
        self.assertIn('    { ROK_FIXTURENODE, ROK_FIXTUREHEAP, offsetof(FixtureHeap, pool), "pool[]", sizeof(FixtureNode), 0, 1, 1, '
                      '{ { offsetof(FixtureHeap, capacity), (unsigned char)sizeof(((FixtureHeap*)nullptr)->capacity), 0 }, { 1, 0, 0 } }, 1 },', files[rs.SCHEMA_H])

    def test_a_table_behind_a_ref_field_may_have_a_constant_capacity(self):
        ann = annotations()
        ann["objects"]["FixtureNode"]["table"]["capacity"] = 6
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        self.assertIn('"pool[]", sizeof(FixtureNode), 6, 1, 1, { { 6, 0, 0 }, { 1, 0, 0 } }, 1 },', files[rs.SCHEMA_H])

    def test_a_table_behind_a_ref_field_with_a_capacity_is_checked(self):
        def table(**change):
            ann = annotations()
            ann["objects"]["FixtureNode"]["table"].update(change)
            return ann
        self.has(table(count="nowhere"), "the count nowhere is no integer field of FixtureHeap")
        self.has(table(capacity="nowhere"), "the capacity nowhere is no integer field of FixtureHeap")
        self.has(table(capacity="pool"), "the capacity pool is no integer field of FixtureHeap")
        self.has(table(capacity=0), "the capacity is a positive integer or a member of FixtureHeap")
        self.has(table(count=9, capacity=4), "the count 9 is more than the capacity 4")
        self.has(table(alloc="elsewhere"), "the alloc of a table with a capacity is \"module\", or absent")
        ann = annotations()
        del ann["objects"]["FixtureNode"]["table"]["capacity"]
        self.has(ann, "only a table with a capacity has an alloc")
        ann = annotations()
        del ann["objects"]["FixtureNode"]["table"]["count"]
        self.has(ann, "FixtureNode table: the count is an integer or the name of a member of FixtureHeap")

    def test_a_table_behind_a_ref_field_without_alloc_uses_the_replayers_allocator(self):
        ann = annotations()
        del ann["objects"]["FixtureNode"]["table"]["alloc"]
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        self.assertIn('"pool[]", sizeof(FixtureNode), 0, 1, 0, { { offsetof(FixtureHeap, capacity)', files[rs.SCHEMA_H])


class SelfPointers(unittest.TestCase):
    """A field that points into its own object: a member (never a line), or an element of the array a member holds (written as the index)."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def has(self, ann, text):
        errors = self.generated(ann)[1]
        self.assertTrue(any(text in e for e in errors), (text, errors))

    def field(self, ann, kind, name):
        return next(f for f in ann["objects"][kind]["fields"] if f["name"] == name)

    def test_a_self_pointer_is_a_row_of_the_self_table(self):
        files, errors, model = self.generated()
        self.assertEqual(errors, [])
        text = files[rs.SCHEMA_H]
        self.assertIn("static const ReplaySelfSpec kReplaySelfs[] = {\n    { offsetof(FixtureEye, pos), 0 },\n    { offsetof(FixtureEye, flag), 0 },\n"
                      "    { offsetof(FixtureHeap, pool), sizeof(FixtureNode) },\n};", text)
        row = [l for l in text.split("\n") if l.startswith('    { "screen", offsetof(FixtureEye, screen)')][0]
        self.assertTrue(row.endswith("RK_SELF, 0, 0, 0, {}, 0, 0, 1 },"), row)
        self.assertEqual([(x["name"], x["member"], x["index"]) for x in model["selfs"]], [("screen", "pos", False), ("flagPtr", "flag", False), ("top", "pool", True)])

    def test_an_index_has_its_own_kind_and_a_target(self):
        files, _, _ = self.generated()
        row = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "top", offsetof(FixtureHeap, top)')][0]
        self.assertTrue(row.endswith("RK_SELFINDEX, ROK_FIXTURENODE, 0, 0, {}, 0, 0, 3 },"), row)

    def test_the_header_checks_that_a_self_pointer_is_a_ref_to_a_member_that_exists(self):
        text = self.generated()[0][rs.SCHEMA_H]
        self.assertIn('static_assert(sizeof(((FixtureEye*)nullptr)->screen) == sizeof(unsigned), "FixtureEye::screen is a Ref");', text)
        self.assertIn('static_assert(sizeof(((FixtureEye*)nullptr)->pos) >= 1,', text)

    def test_the_real_schema_has_no_self_pointer_and_an_empty_table(self):
        files, _, _ = rs.generate(rs.read_rows())
        self.assertIn("static const ReplaySelfSpec kReplaySelfs[] = {\n    { 0, 0 },\n};", files[rs.SCHEMA_H])

    def test_a_self_pointer_is_never_walked(self):
        self.assertNotIn("self", rs.WALKABLE)
        self.assertNotIn("selfindex", rs.WALKABLE)

    def test_a_self_field_is_checked(self):
        ann = annotations()
        self.field(ann, "FixtureEye", "screen")["target"] = "FixtureNode"
        self.has(ann, "FixtureEye.screen: a self pointer to a member has no `target`")
        ann = annotations()
        self.field(ann, "FixtureEye", "screen")["extra"] = 1
        self.has(ann, "FixtureEye.screen: a self field is")
        ann = annotations()
        del self.field(ann, "FixtureEye", "screen")["member"]
        self.has(ann, "FixtureEye.screen: the `member` of a self field is the name of a member of FixtureEye")
        ann = annotations()
        self.field(ann, "FixtureEye", "screen")["member"] = "pos x"
        self.has(ann, "FixtureEye.screen: the `member` of a self field is the name of a member of FixtureEye")
        ann = annotations()
        self.field(ann, "FixtureEye", "screen")["index"] = "yes"
        self.has(ann, "FixtureEye.screen: `index` is true or false")

    def test_an_index_names_the_ref_field_that_holds_the_array(self):
        ann = annotations()
        del self.field(ann, "FixtureHeap", "top")["target"]
        self.has(ann, "FixtureHeap.top: the target of a self index is the state kind of the array's elements")
        ann = annotations()
        self.field(ann, "FixtureHeap", "top")["member"] = "used"
        self.has(ann, "FixtureHeap.top: the member used is no ref field of FixtureHeap that comes earlier and leads to FixtureNode")
        ann = annotations()
        self.field(ann, "FixtureHeap", "top")["member"] = "nowhere"
        self.has(ann, "FixtureHeap.top: the member nowhere is no ref field")
        ann = annotations()
        fields = ann["objects"]["FixtureHeap"]["fields"]
        fields.append(fields.pop(next(i for i, f in enumerate(fields) if f["name"] == "pool")))
        self.has(ann, "FixtureHeap.top: the member pool is no ref field of FixtureHeap that comes earlier")
        ann = annotations()
        self.field(ann, "FixtureHeap", "pool")["target"] = "FixtureEye"
        self.has(ann, "FixtureHeap.top: the member pool is no ref field of FixtureHeap that comes earlier and leads to FixtureNode")

    def test_only_a_state_kind_has_a_self_pointer(self):
        ann = annotations()
        ann["objects"]["Vec3"]["fields"].append({"name": "x", "kind": "self", "member": "y"})
        self.has(ann, "only the fields of a state kind are self pointers")

    def test_a_self_pointer_may_have_aliases(self):
        ann = annotations()
        self.field(ann, "FixtureEye", "screen")["aliases"] = ["FixtureEye::screenAlias"]
        self.assertEqual(self.generated(ann)[1], [])

    def test_a_row_on_a_self_pointer_is_an_object_field(self):
        model, errors = build([row("UpdatePlayerEconomy", "field_read", "FixtureEye::screen")])
        names = ["%s.%s" % (f["object"], f["name"]) for f in model["object_fields"]]
        s = model["subsystems"]["economy"]["slots"]["UpdatePlayerEconomy"]
        self.assertIn("FixtureEye.screen", [names[i] for i in s["objectReads"]])


class RefCTypes(unittest.TestCase):
    """A ref whose C type differs from its target kind: `Ref<char>` to a state kind."""

    def generated(self, ann=None):
        return rs.generate(rs.read_rows(), ann or annotations())

    def has(self, ann, text):
        errors = self.generated(ann)[1]
        self.assertTrue(any(text in e for e in errors), (text, errors))

    def owner(self, ann):
        return next(f for f in ann["objects"]["FixtureEye"]["fields"] if f["name"] == "owner")

    def test_the_header_checks_the_declared_type(self):
        files, errors, _ = self.generated()
        self.assertEqual(errors, [])
        self.assertIn('static_assert(__is_same(decltype(((FixtureEye*)nullptr)->owner), Ref<char>), "FixtureEye::owner is a Ref<char>");', files[rs.SCHEMA_H])

    def test_a_ref_without_a_ctype_is_not_checked_for_its_type(self):
        ann = annotations()
        del self.owner(ann)["ctype"]
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        self.assertNotIn("FixtureEye::owner is a Ref", files[rs.SCHEMA_H])

    def test_the_target_is_the_kind_whatever_the_ctype(self):
        files, _, model = self.generated()
        row = [l for l in files[rs.SCHEMA_H].split("\n") if l.startswith('    { "owner", offsetof(FixtureEye, owner)')][0]
        self.assertIn("RK_REF, ROK_FIXTUREVIEW,", row)

    def test_an_array_of_refs_checks_its_ctype_too(self):
        ann = annotations()
        next(f for f in ann["objects"]["FixtureManager"]["fields"] if f["name"] == "timers")["ctype"] = "FixtureTimerBase"
        files, errors, _ = self.generated(ann)
        self.assertEqual(errors, [])
        self.assertIn("FixtureManager::timers holds Ref<FixtureTimerBase>", files[rs.SCHEMA_H])

    def test_the_ctype_is_checked(self):
        ann = annotations()
        self.owner(ann)["ctype"] = "Ref<char>"
        self.has(ann, "FixtureEye.owner: the `ctype` is the C type that the Ref holds")
        ann = annotations()
        self.owner(ann)["kind"] = "ptr"
        self.has(ann, "FixtureEye.owner: only a ref or an array of refs has a `ctype`")


class Committed(unittest.TestCase):
    def test_the_data_is_current_and_complete(self):
        files, errors, model = rs.generate(rs.read_rows())
        self.assertEqual(errors, [])
        for path, want in files.items():
            self.assertEqual(path.read_text(), want, path)

    def test_no_test_kind_is_in_the_committed_schema(self):
        files, _, _ = rs.generate(rs.read_rows())
        for path in (rs.JSON_OUT, rs.SCHEMA_H, rs.VTABLES_CPP):
            self.assertNotIn("Fixture", files[path], path)
        self.assertIn("FixtureView", files[rs.FIXTURE_SCHEMA_H])
        self.assertIn('Game.slowest', files[rs.FIXTURE_SCHEMA_H])
        self.assertIn("BT_REPLAY_FIXTURES", files[rs.SCHEMA_H])

    def test_the_test_schema_adds_its_kinds_after_the_real_ones(self):
        real, _ = rs.build(rs.load_annotations(), rs.read_rows(), rs.read_assignment())
        both, _ = rs.build(rs.load_annotations(fixtures=True), rs.read_rows(), rs.read_assignment())
        names = [o["name"] for o in both["objects"]]
        self.assertEqual(names[:len(real["objects"])], [o["name"] for o in real["objects"]])
        self.assertTrue(all(n.startswith("Fixture") for n in names[len(real["objects"]):]))
        self.assertEqual([f["name"] for f in both["object_fields"][:len(real["object_fields"])]], [f["name"] for f in real["object_fields"]])

    def test_the_test_kinds_are_checked_too(self):
        ann = annotations()
        ann["objects"]["FixtureCell"]["delta"] = "yes"
        files, errors, _ = rs.generate(rs.read_rows(), rs.load_annotations(), None, {"objects": {"FixtureCell": ann["objects"]["FixtureCell"]}, "includes": ["../../tests/replay_fixtures.h"]})
        self.assertTrue(any("FixtureCell delta" in e for e in errors), errors)

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


class LinkSources(unittest.TestCase):
    def test_reads_the_subsystems_from_the_json_list(self):
        import replay_subsystems

        listed = set(replay_subsystems.load())
        with rs.SUBSYSTEMS.open(newline="") as f:
            rows = list(csv.DictReader(f))
        other = next(r for r in rows if r["subsystem"] not in listed)
        self.assertNotIn(other["file"], rs.replay_link_sources())
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "replay_subsystems.json"
            path.write_text(json.dumps({other["subsystem"]: {}}))
            real = replay_subsystems.load
            with mock.patch.object(replay_subsystems, "load", lambda: real(path)):
                self.assertIn(other["file"], rs.replay_link_sources())


if __name__ == "__main__":
    unittest.main()
