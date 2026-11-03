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
