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
        self.assertIn('{ "Projectile", sizeof(Projectile), RS_STATE, 3,', text)
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
        rs.parse_length("a+1", "t", errors)
        self.assertEqual(len(errors), 2, errors)


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
                      "{ offsetof(MovementClass, height), (unsigned char)sizeof(((MovementClass*)nullptr)->height), 4 } } }", text)
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
        self.assertTrue(any("only a struct in can be nullable" in e for e in errors), errors)

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
        self.assertIn('{ "ApproachRadius", sizeof(ApproachRadius), RS_STATE, RT_ORDER, 0, 0, 0, "", 0, 0 },', text)
        self.assertIn("    case ROK_APPROACHRADIUS:\n        return g_approachRadiusVtable;", vtables)
        self.assertIn('    case ROK_APPROACHRADIUS:\n        return "g_approachRadiusVtable";', vtables)
        base = [l for l in text.split("\n") if l.startswith('    { "OrderFx",')][0]
        self.assertTrue(base.endswith('"", 0, 3 },'), base)
        self.assertIn("kReplayPolyKinds[] = { ROK_APPROACHRADIUS, ROK_RINGAPPROACH, ROK_PATHORDER };", text)
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
        ann["subsystems"]["map"].setdefault("slots", {})["VisitObjectsInRange"] = {"args": {"visitor": {"mode": "callback", "callback": "UnitVisitor"}}}
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

    def test_a_callback_argument_of_a_slot_no_subsystem_imports_is_an_error(self):
        def only_note(ann):
            ann["subsystems"]["wind_meteors"]["imports"].remove("map.VisitObjectsInRange")
        self.assertTrue(any("map.VisitObjectsInRange has a callback argument and is imported by no subsystem" in e for e in self.errors(self.with_import(only_note))))

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
