"""Tests for the import allowlist and window checks of wasm_modules.py.

    uv run tools/test_wasm_modules.py
"""

import json
import re
import os
import subprocess
import sys
import tempfile
import unittest

import wasm_modules as wm

SPEC = {
    "imports": {"weapons": ["SpawnProjectile"], "services": ["RandomInt"]},
    "exports": ["UpdateWind", "g_game"],
}
GOOD_IMPORTS = [("env", "memory"), ("env", "__indirect_function_table"), ("weapons", "SpawnProjectile"), ("services", "RandomInt")]
GOOD_EXPORTS = ["UpdateWind", "g_game"]
SPEC_KINDS = {
    "imports": {"weapons": {"SpawnProjectile": [["block", "linear"], "int"]}, "services": {"Scale": [["handle", "f64"], "f64"]}},
    "exports": ["UpdateWind", "g_game"],
    "slots": {"UpdateWind": [["int"], "void"]},
}
KIND_IMPORTS = [("env", "memory"), ("env", "__indirect_function_table"), ("weapons", "SpawnProjectile"), ("services", "Scale")]
DETAILS = {
    "globals": [],
    "imports": {("weapons", "SpawnProjectile"): (["i32", "i32"], "i32"), ("services", "Scale"): (["i32", "f64"], "f64")},
    "exports": {"UpdateWind": (["i32"], "void")},
}
PINS = {"window": 65536, "memory_pages": 4, "table_slots": 32, "table_size": 80}
ENTRY = {"name": "m", "data_base": 196608, "table_base": 48}


def uleb(value):
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        out.append(byte | (0x80 if value else 0))
        if not value:
            return bytes(out)


def sleb(value):
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        done = (value == 0 and not byte & 0x40) or (value == -1 and byte & 0x40)
        out.append(byte if done else byte | 0x80)
        if done:
            return bytes(out)


def section(number, body):
    return bytes([number]) + uleb(len(body)) + body


def build_module(stack_top, segments, elements):
    imports = uleb(2) + b"\x03env\x06memory\x02\x00\x04" + b"\x03env\x19__indirect_function_table\x01\x70\x00" + uleb(48 + elements)
    glob = uleb(1) + b"\x7f\x01\x41" + sleb(stack_top) + b"\x0b"
    elem = uleb(1) + b"\x00\x41" + sleb(48) + b"\x0b" + uleb(elements) + b"\x00" * elements
    data = uleb(len(segments))
    for offset, length in segments:
        data += b"\x00\x41" + sleb(offset) + b"\x0b" + uleb(length) + b"\x00" * length
    return b"\0asm\1\0\0\0" + section(2, imports) + section(6, glob) + section(9, elem) + section(11, data)


class InterfaceTests(unittest.TestCase):
    def test_listed_imports_and_exports_pass(self):
        self.assertEqual(wm.check_interface("m", GOOD_IMPORTS, GOOD_EXPORTS, SPEC), [])

    def test_wasi_clock_fails(self):
        imports = GOOD_IMPORTS + [("wasi_snapshot_preview1", "clock_time_get")]
        problems = wm.check_interface("m", imports, GOOD_EXPORTS, SPEC)
        self.assertEqual(len(problems), 1)
        self.assertIn("wasi_snapshot_preview1.clock_time_get", problems[0])

    def test_wasi_fails_even_when_the_manifest_lists_it(self):
        spec = dict(SPEC, imports={"wasi_snapshot_preview1": ["clock_time_get"]})
        imports = [("env", "memory"), ("env", "__indirect_function_table"), ("wasi_snapshot_preview1", "clock_time_get")]
        self.assertTrue(wm.check_interface("m", imports, GOOD_EXPORTS, spec))

    def test_any_imported_global_fails(self):
        imports = KIND_IMPORTS + [("env", "__sim_base")]
        details = dict(DETAILS, globals=[("env", "__sim_base")])
        problems = wm.check_interface("m", imports, GOOD_EXPORTS, SPEC_KINDS, details)
        self.assertTrue(any("imports the global env.__sim_base" in p for p in problems))
        self.assertEqual(wm.check_interface("m", KIND_IMPORTS, GOOD_EXPORTS, SPEC_KINDS, DETAILS), [])

    def test_unlisted_import_fails(self):
        problems = wm.check_interface("m", GOOD_IMPORTS + [("services", "ClockNow")], GOOD_EXPORTS, SPEC)
        self.assertEqual(len(problems), 1)
        self.assertIn("services.ClockNow", problems[0])

    def test_env_import_beyond_memory_and_table_fails(self):
        self.assertTrue(wm.check_interface("m", GOOD_IMPORTS + [("env", "emscripten_get_now")], GOOD_EXPORTS, SPEC))

    def test_missing_listed_import_fails(self):
        self.assertTrue(wm.check_interface("m", GOOD_IMPORTS[:3], GOOD_EXPORTS, SPEC))

    def test_extra_and_missing_exports_fail(self):
        self.assertTrue(wm.check_interface("m", GOOD_IMPORTS, GOOD_EXPORTS + ["memory"], SPEC))
        self.assertTrue(wm.check_interface("m", GOOD_IMPORTS, ["UpdateWind"], SPEC))


class KindTests(unittest.TestCase):
    def problems(self, spec=SPEC_KINDS, details=DETAILS):
        return wm.check_interface("m", KIND_IMPORTS, GOOD_EXPORTS, spec, details)

    def test_matching_kinds_pass(self):
        self.assertEqual(self.problems(), [])

    def test_wrong_parameter_count_fails(self):
        spec = dict(SPEC_KINDS, imports={"weapons": {"SpawnProjectile": [["block"], "int"]}, "services": SPEC_KINDS["imports"]["services"]})
        self.assertTrue(any("1 parameters, the wasm signature 2" in p for p in self.problems(spec)))

    def test_f64_must_match_the_wasm_type(self):
        spec = dict(SPEC_KINDS, imports={"weapons": SPEC_KINDS["imports"]["weapons"], "services": {"Scale": [["handle", "int"], "f64"]}})
        self.assertTrue(any("parameter 1 is 'int' in the manifest and f64" in p for p in self.problems(spec)))
        spec = dict(SPEC_KINDS, imports={"weapons": {"SpawnProjectile": [["f64", "linear"], "int"]}, "services": SPEC_KINDS["imports"]["services"]})
        self.assertTrue(self.problems(spec))

    def test_f32_must_match_the_wasm_type(self):
        def spec_of(params, result):
            return dict(SPEC_KINDS, imports={"weapons": SPEC_KINDS["imports"]["weapons"], "services": {"Scale": [params, result]}})

        def problems_of(params, result, wasm_params, wasm_result):
            imports = dict(DETAILS["imports"])
            imports[("services", "Scale")] = (wasm_params, wasm_result)
            return wm.check_kinds("m", spec_of(params, result), dict(DETAILS, imports=imports))

        self.assertEqual(problems_of(["handle", "f32"], "f32", ["i32", "f32"], "f32"), [])
        self.assertEqual(problems_of(["handle", "f64"], "f64", ["i32", "f64"], "f64"), [])
        self.assertTrue(problems_of(["handle", "f32"], "f32", ["i32", "f64"], "f64"))
        self.assertTrue(problems_of(["handle", "f64"], "f64", ["i32", "f32"], "f32"))
        self.assertTrue(problems_of(["handle", "f32"], "f32", ["i32", "i32"], "i32"))
        self.assertTrue(problems_of(["handle", "int"], "int", ["i32", "f32"], "f32"))
        self.assertTrue(any("parameter 1 is 'int' in the manifest and f32" in p for p in problems_of(["handle", "int"], "f32", ["i32", "f32"], "f32")))
        self.assertTrue(any("the result is 'int' in the manifest and f32" in p for p in problems_of(["handle", "f32"], "int", ["i32", "f32"], "f32")))
        self.assertTrue(any("the result is 'f32' in the manifest and f64" in p for p in problems_of(["handle", "f64"], "f32", ["i32", "f64"], "f64")))

    def test_result_kind_must_match(self):
        spec = dict(SPEC_KINDS, imports={"weapons": {"SpawnProjectile": [["block", "linear"], "void"]}, "services": SPEC_KINDS["imports"]["services"]})
        self.assertTrue(any("result is void" in p for p in self.problems(spec)))
        spec = dict(SPEC_KINDS, imports={"weapons": SPEC_KINDS["imports"]["weapons"], "services": {"Scale": [["handle", "f64"], "int"]}})
        self.assertTrue(self.problems(spec))

    def test_unknown_kind_fails(self):
        spec = dict(SPEC_KINDS, imports={"weapons": {"SpawnProjectile": [["pointer", "linear"], "int"]}, "services": SPEC_KINDS["imports"]["services"]})
        self.assertTrue(self.problems(spec))

    def test_slot_kinds_are_checked(self):
        spec = dict(SPEC_KINDS, slots={"UpdateWind": [[], "void"]})
        self.assertTrue(any("slot UpdateWind" in p for p in self.problems(spec)))
        spec = dict(SPEC_KINDS, slots={})
        self.assertTrue(any("only one of" in p for p in self.problems(spec)))

    def test_the_game_manifest_kinds_are_well_formed(self):
        game = wm.game_manifest()
        for entry in game["modules"]:
            self.assertEqual(sorted(entry["slots"]), sorted(entry["exports"]))
            for entries in entry["imports"].values():
                for params, result in entries.values():
                    self.assertTrue(set(params) <= wm.KINDS)
                    self.assertTrue(result == "void" or result in wm.KINDS)


class BuildFlagTests(unittest.TestCase):
    def test_the_manifest_carries_the_float_conversion_flag(self):
        self.assertIn("-mno-nontrapping-fptoint", wm.game_manifest()["build_flags"])
        self.assertNotIn("-mno-nontrapping-fptoint", wm.CXX_FLAGS + wm.C_FLAGS)


class WindowTests(unittest.TestCase):
    def test_parse_reads_stack_data_and_table(self):
        info = wm.parse_module(build_module(200000, [(196608, 100), (197000, 8)], 17))
        self.assertEqual(info["stack_top"], 200000)
        self.assertEqual(info["segments"], [(196608, 100), (197000, 8)])
        self.assertEqual(info["elements"], [(48, 17)])
        self.assertEqual((info["memory_min"], info["table_min"]), (4, 65))

    def check(self, stack_top, segments, elements, others=()):
        info = wm.parse_module(build_module(stack_top, segments, elements))
        return wm.check_window(ENTRY, PINS, info, list(others))

    def test_module_inside_its_window_passes(self):
        self.assertEqual(self.check(202688, [(196608, 1960)], 17), [])

    def test_stack_past_the_window_fails(self):
        self.assertTrue(self.check(262145, [(196608, 1960)], 17))

    def test_data_below_the_window_fails(self):
        self.assertTrue(self.check(202688, [(196000, 1960)], 17))

    def test_table_entries_past_the_range_fail(self):
        self.assertTrue(self.check(202688, [(196608, 1960)], 40))

    def test_overlap_with_another_module_fails(self):
        others = [("c", 131072, 196609, 32, 48)]
        problems = self.check(202688, [(196608, 1960)], 17, others)
        self.assertEqual(len(problems), 1)
        self.assertIn("overlaps c", problems[0])

    def test_table_overlap_with_another_module_fails(self):
        others = [("c", 131072, 196608, 32, 49)]
        self.assertTrue(self.check(202688, [(196608, 1960)], 17, others))

    def test_the_fixtures_and_the_game_module_are_disjoint(self):
        game = wm.game_manifest()
        for entry in game["modules"]:
            self.assertEqual(wm.check_window(entry, game, {"memory_min": 4, "stack_top": entry["data_base"] + 8,
                                                           "segments": [], "elements": []}, wm.fixture_ranges()), [])


class LayoutTests(unittest.TestCase):
    PINS = {"block_base": 1048576, "block_size": 33554432, "block_max": 268435456, "scratch": [983040, 65536], "memory_pages": 4112}
    RANGES = [("a", 65536, 131072), ("b", 196608, 202688)]

    def test_the_game_manifest_layout_passes(self):
        game = wm.game_manifest()
        ranges = [(n, lo, hi) for n, lo, hi, _, _ in wm.fixture_ranges()]
        self.assertEqual(wm.check_layout(game, ranges + [("wind_meteors", game["modules"][0]["data_base"], game["modules"][0]["data_base"] + game["window"])]), [])

    def test_disjoint_ranges_pass(self):
        self.assertEqual(wm.check_layout(self.PINS, self.RANGES), [])

    def test_module_overlapping_another_fails(self):
        problems = wm.check_layout(self.PINS, self.RANGES + [("c", 200000, 210000)])
        self.assertEqual(len(problems), 1)
        self.assertIn("overlaps c", problems[0])

    def test_module_overlapping_the_scratch_fails(self):
        problems = wm.check_layout(self.PINS, self.RANGES + [("c", 983040, 990000)])
        self.assertTrue(any("overlaps scratch" in p for p in problems))

    def test_module_reaching_the_scratch_base_fails(self):
        problems = wm.check_layout(self.PINS, self.RANGES + [("c", 900000, 983041)])
        self.assertTrue(any("c: " in p and "reaches the scratch base 0xf0000" in p for p in problems))

    def test_module_above_the_scratch_fails(self):
        problems = wm.check_layout(self.PINS, self.RANGES + [("c", 1000000, 1040000)])
        self.assertTrue(any("reaches the scratch base" in p for p in problems))

    def test_module_reaching_the_block_fails(self):
        problems = wm.check_layout(self.PINS, self.RANGES + [("c", 1040000, 1048577)])
        self.assertTrue(any("reaches the block base" in p for p in problems))

    def test_touching_ranges_pass(self):
        self.assertEqual(wm.check_layout(self.PINS, self.RANGES + [("c", 202688, 983040)]), [])

    def test_pages_must_hold_the_block(self):
        self.assertTrue(wm.check_layout(dict(self.PINS, memory_pages=527), self.RANGES))

    LOAD = dict(PINS, load_data=[269484032, 67108864], memory_pages=5136)

    def test_load_data_ending_the_memory_passes(self):
        self.assertEqual(wm.check_layout(self.LOAD, self.RANGES), [])

    def test_pages_must_end_at_the_load_data(self):
        self.assertTrue(any("does not end at the load data" in p for p in wm.check_layout(dict(self.LOAD, memory_pages=4112), self.RANGES)))

    def test_load_data_must_start_at_the_block_ceiling(self):
        problems = wm.check_layout(dict(self.LOAD, load_data=[269484033, 67108863]), self.RANGES)
        self.assertTrue(any("not at the block's ceiling" in p for p in problems))


class Budgets(unittest.TestCase):
    def test_a_module_over_its_size_budget_fails(self):
        entry = {"name": "m", "size_budget": 1000}
        self.assertEqual(wm.check_size(entry, 1000), [])
        self.assertTrue(any("over its size_budget 1000" in p for p in wm.check_size(entry, 1001)))

    def test_a_module_without_a_size_budget_fails(self):
        self.assertTrue(wm.check_size({"name": "m"}, 10))

    def test_a_blocked_entry_has_no_size_to_judge(self):
        self.assertEqual(wm.check_size({"name": "m", "blocked": "its default does not build"}, 10**9), [])

    def test_the_fuel_entry_must_be_the_passes_rule(self):
        pins = {"fuel": {"rule": wm.wasm_fuel.RULE, "version": wm.wasm_fuel.VERSION}, "modules": [{"name": "m", "fuel_budget": 5}]}
        self.assertEqual(wm.check_fuel_manifest(pins), [])
        self.assertTrue(wm.check_fuel_manifest(dict(pins, fuel={"rule": "other", "version": 1})))
        self.assertTrue(wm.check_fuel_manifest({"modules": pins["modules"]}))

    def test_the_real_publishes_table_passes(self):
        game = wm.game_manifest()
        self.assertEqual(wm.check_publishes(game), [])
        self.assertEqual(next(m for m in game["modules"] if m["name"] == "wind_meteors")["publishes"], {"80": 36})

    def publishes_problems(self, publishes, interface=None):
        game = wm.game_manifest()
        entry = dict(next(m for m in game["modules"] if m["name"] == "wind_meteors"), publishes=publishes)
        if interface:
            entry["interface"] = interface
        return wm.check_publishes(dict(game, modules=[entry]))

    def test_publishes_refuses_a_kind_no_route_takes_from_the_outbox(self):
        self.assertTrue(any("no route takes from the wind_meteors outbox" in p for p in self.publishes_problems({"64": 4})))
        self.assertTrue(any("no route takes from the wind_meteors outbox" in p for p in self.publishes_problems({"132": 36})))
        self.assertTrue(any("no route takes" in p for p in self.publishes_problems({"999": 36})))

    def test_publishes_accepts_a_routed_inbox_kind(self):
        size = wm.routed_kinds("map")[132]
        self.assertEqual(self.publishes_problems({"132": size}, interface="map"), [])
        self.assertTrue(any("size 4" in p for p in self.publishes_problems({"132": 4}, interface="map")))

    def test_routed_kinds_are_the_routes_of_an_outbox(self):
        self.assertEqual(wm.routed_kinds("wind_meteors"), {80: 36})
        self.assertEqual(wm.routed_kinds("no_such_outbox"), {})

    def test_publishes_refuses_a_wrong_size(self):
        self.assertTrue(any("size 32" in p for p in self.publishes_problems({"80": 32})))

    def test_effect_kinds_take_implicit_values_and_skip_a_trailing_comma(self):
        text = "enum XEffectKind {\n    X_A = 80,\n    X_B,\n    X_C = 0x60, // the next\n    X_D,\n};\nenum YEventKind { Y_A = 1 };"
        self.assertEqual(wm.effect_kinds(text), {80, 81, 96, 97})
        self.assertEqual(wm.effect_kinds("enum ZEffectKind { Z_A, Z_B };"), {0, 1})

    def test_effect_kinds_refuse_a_value_they_cannot_read(self):
        with self.assertRaises(ValueError):
            wm.effect_kinds("enum XEffectKind { X_A = OTHER + 1 };")

    def test_publishes_reports_an_unreadable_route_table_instead_of_crashing(self):
        import routes
        original = routes.rows

        def broken():
            raise ValueError("route kind X is not an enumerator of any interface")
        routes.rows = broken
        try:
            problems = self.publishes_problems({"80": 36})
        finally:
            routes.rows = original
        self.assertTrue(any("cannot read the routes" in p for p in problems))

    def test_publishes_refuses_a_malformed_table(self):
        self.assertTrue(self.publishes_problems([80]))
        self.assertTrue(self.publishes_problems({"eighty": 36}))
        self.assertTrue(self.publishes_problems({"80": "36"}))
        self.assertTrue(self.publishes_problems({"80": True}))

    def test_a_fuel_budget_is_a_positive_int_the_loader_can_read(self):
        base = {"fuel": {"rule": wm.wasm_fuel.RULE, "version": wm.wasm_fuel.VERSION}}
        for bad in (None, 0, -1, 1 << 31, "5"):
            self.assertTrue(wm.check_fuel_manifest(dict(base, modules=[{"name": "m", "fuel_budget": bad}])), bad)

    def test_the_fuel_functions_are_not_slots(self):
        details = dict(DETAILS, exports={"UpdateWind": (["i32"], "void"), "__fuel_set": (["i64"], "void"), "__fuel_get": ([], "i64")})
        self.assertEqual(wm.check_interface("m", KIND_IMPORTS, GOOD_EXPORTS + list(wm.wasm_fuel.EXPORTS),
                                            dict(SPEC_KINDS, exports=SPEC_KINDS["exports"] + list(wm.wasm_fuel.EXPORTS)), details), [])


MANIFEST = {
    "majors": {"weapons": 1, "services": 2, "wind_meteors": 1},
    "modules": [{"name": "m", "interface": "wind_meteors", "majors": {"wind_meteors": 1, "weapons": 1, "services": 2},
                 "slots": {}, "imports": {"weapons": {}, "services": {}}}],
}


class MajorsTests(unittest.TestCase):
    def test_matching_majors_pass(self):
        self.assertEqual(wm.check_majors(MANIFEST, MANIFEST["majors"]), [])

    def test_top_level_majors_must_match_the_headers(self):
        problems = wm.check_majors(MANIFEST, dict(MANIFEST["majors"], weapons=2))
        self.assertEqual(len(problems), 1)
        self.assertIn("differ from interfaces", problems[0])

    def test_module_missing_a_major_fails(self):
        entry = dict(MANIFEST["modules"][0], majors={"wind_meteors": 1, "weapons": 1})
        problems = wm.check_majors(dict(MANIFEST, modules=[entry]), MANIFEST["majors"])
        self.assertEqual(problems, ["m: no major for services, an interface it uses"])

    def test_module_major_ahead_of_the_headers_fails(self):
        entry = dict(MANIFEST["modules"][0], majors={"wind_meteors": 1, "weapons": 3, "services": 2})
        problems = wm.check_majors(dict(MANIFEST, modules=[entry]), MANIFEST["majors"])
        self.assertEqual(len(problems), 1)
        self.assertIn("built against weapons 3, the headers have 1", problems[0])

    def test_module_major_for_an_unused_interface_fails(self):
        entry = dict(MANIFEST["modules"][0], majors={"wind_meteors": 1, "weapons": 1, "services": 2, "map": 3})
        problems = wm.check_majors(dict(MANIFEST, modules=[entry]), dict(MANIFEST["majors"], map=3))
        self.assertTrue(any("map, which it neither exports slots for nor imports from" in p for p in problems))

    def test_older_module_major_passes_the_majors_check(self):
        entry = dict(MANIFEST["modules"][0], majors={"wind_meteors": 1, "weapons": 1, "services": 1})
        self.assertEqual(wm.check_majors(dict(MANIFEST, modules=[entry]), MANIFEST["majors"]), [])

    def test_with_majors_sets_every_module_to_the_headers(self):
        stale = dict(MANIFEST, majors={}, modules=[dict(MANIFEST["modules"][0], majors={"wind_meteors": 0, "weapons": 0, "services": 0})])
        fixed = wm.with_majors(stale, MANIFEST["majors"])
        self.assertEqual(fixed["majors"], MANIFEST["majors"])
        self.assertEqual(fixed["modules"][0]["majors"], {"wind_meteors": 1, "weapons": 1, "services": 2})

    def test_header_majors_read_every_interface(self):
        majors = wm.header_majors()
        self.assertEqual(majors["wind_meteors"], 3)
        self.assertIn("weapons", majors)
        self.assertIn("services", majors)

    def test_the_committed_manifest_is_what_build_writes(self):
        game = wm.game_manifest()
        with open(wm.GAME_MODULES) as f:
            self.assertEqual(f.read(), wm.manifest_text(wm.with_majors(game, wm.header_majors())))

    def test_manifest_text_round_trips(self):
        game = wm.game_manifest()
        self.assertEqual(json.loads(wm.manifest_text(game)), game)


SIGNATURES = {"weapons": {"Spawn": [["block", "int"], "int"]}, "services": {"Rand": [["int"], "int"], "Log": [["linear"], "void"]}}
ADAPTED = {
    "majors": {"weapons": 1, "services": 2, "wind_meteors": 1},
    "signatures": SIGNATURES,
    "adapters": [],
    "modules": [{"name": "m", "interface": "wind_meteors", "majors": {"wind_meteors": 1, "weapons": 1, "services": 1}, "slots": {},
                 "imports": {"weapons": {"Spawn": [["block", "int"], "int"]},
                             "services": {"Rand": [["int", "int"], "int"], "Log": [["linear"], "void"]}}}],
}
RAND_ADAPTER = {"interface": "services", "from_major": 1, "name": "Rand", "args": [0], "result": "pass"}


class AdapterManifestTests(unittest.TestCase):
    def test_a_changed_import_without_an_adapter_fails(self):
        problems = wm.check_majors(ADAPTED, ADAPTED["majors"])
        self.assertEqual(problems, ["m: services.Rand changed between major 1 and 2, and the manifest has no adapter for it"])

    def test_an_adapter_covers_the_change(self):
        self.assertEqual(wm.check_majors(dict(ADAPTED, adapters=[RAND_ADAPTER]), ADAPTED["majors"]), [])

    def test_an_adapter_from_another_major_does_not_cover_it(self):
        adapter = dict(RAND_ADAPTER, from_major=0)
        problems = wm.check_majors(dict(ADAPTED, adapters=[adapter]), ADAPTED["majors"])
        self.assertTrue(any("no adapter for it" in p for p in problems))
        self.assertTrue(any("from_major must be older" in p for p in problems))

    def test_an_adapter_for_an_unknown_name_fails(self):
        problems = wm.check_majors(dict(ADAPTED, adapters=[RAND_ADAPTER, dict(RAND_ADAPTER, name="Nope")]), ADAPTED["majors"])
        self.assertEqual(len(problems), 1)
        self.assertIn("no Nope for services", problems[0])

    def test_a_current_module_must_match_the_signatures(self):
        entry = dict(ADAPTED["modules"][0], majors={"wind_meteors": 1, "weapons": 1, "services": 2})
        problems = wm.check_majors(dict(ADAPTED, modules=[entry]), ADAPTED["majors"])
        self.assertEqual(len(problems), 1)
        self.assertIn("services.Rand is [['int', 'int'], 'int'] but the current signature is [['int'], 'int']", problems[0])

    def test_a_name_without_a_signature_fails(self):
        problems = wm.check_majors(dict(ADAPTED, signatures={"weapons": SIGNATURES["weapons"]}, adapters=[RAND_ADAPTER]), ADAPTED["majors"])
        self.assertTrue(any("the manifest's signatures have no Rand for services" in p for p in problems))

    def test_signatures_come_from_the_current_modules_over_the_old_ones(self):
        entry = dict(ADAPTED["modules"][0], majors={"wind_meteors": 1, "weapons": 1, "services": 2},
                     imports={"weapons": {"Spawn": [["block", "int", "int"], "int"]}, "services": {"Rand": [["int"], "int"]}})
        game = dict(ADAPTED, modules=[entry])
        found = wm.current_signatures(game, [entry], ADAPTED["majors"])
        self.assertEqual(found, {"services": {"Rand": [["int"], "int"], "Log": [["linear"], "void"]}, "weapons": {"Spawn": [["block", "int", "int"], "int"]}})

    def test_a_name_no_module_uses_is_dropped(self):
        game = dict(ADAPTED, signatures=dict(SIGNATURES, units={"GetSweetSpot": [["block", "linear"], "void"]}))
        found = wm.current_signatures(game, game["modules"], ADAPTED["majors"], lambda interface, name: True)
        self.assertNotIn("units", found)
        self.assertEqual(found, SIGNATURES)

    def test_a_service_no_module_imports_keeps_its_entry(self):
        services = dict(SIGNATURES["services"], SimFree=[["linear"], "void"])
        game = dict(ADAPTED, signatures=dict(SIGNATURES, services=services))
        found = wm.current_signatures(game, game["modules"], ADAPTED["majors"], lambda interface, name: True)
        self.assertEqual(found["services"], services)

    def test_a_name_only_an_older_module_uses_keeps_the_old_entry(self):
        found = wm.current_signatures(ADAPTED, ADAPTED["modules"], dict(ADAPTED["majors"], weapons=2), lambda interface, name: True)
        self.assertEqual(found["weapons"], {"Spawn": [["block", "int"], "int"]})
        self.assertEqual(found["services"], SIGNATURES["services"])

    def test_a_name_an_older_module_uses_that_the_header_no_longer_has_fails(self):
        majors = dict(ADAPTED["majors"], weapons=2)
        with self.assertRaises(SystemExit) as e:
            wm.current_signatures(ADAPTED, ADAPTED["modules"], majors, lambda interface, name: (interface, name) != ("weapons", "Spawn"))
        self.assertIn("m: uses weapons.Spawn, which interfaces/weapons.h no longer declares", str(e.exception))

    def test_a_current_module_keeps_a_name_the_header_check_would_refuse(self):
        entry = dict(ADAPTED["modules"][0], majors={"wind_meteors": 1, "weapons": 1, "services": 2})
        older = dict(entry, name="n", majors={"wind_meteors": 1, "weapons": 1, "services": 1})
        found = wm.current_signatures(ADAPTED, [entry, older], ADAPTED["majors"], lambda interface, name: False)
        self.assertEqual(found["services"]["Rand"], [["int", "int"], "int"])

    def test_the_headers_declare_their_slots_and_not_removed_ones(self):
        self.assertTrue(wm.header_declares("map", "GetGroundHeight"))
        self.assertFalse(wm.header_declares("units", "GetGroundHeight"))
        self.assertFalse(wm.header_declares("units", "GetSweetSpot"))
        self.assertFalse(wm.header_declares("nosuchinterface", "Apply"))
        self.assertTrue(wm.header_declares("services", "HapiBank_OpenAccount"))

    def test_every_committed_signature_is_used_by_a_module_or_a_service_and_declared(self):
        game = wm.game_manifest()
        used = {(interface, name) for entry in game["modules"] for interface in wm.module_interfaces(entry)
                for name in wm.module_table(entry, interface)}
        named = {(interface, name) for interface, names in game["signatures"].items() for name in names}
        self.assertEqual({item for item in named if item[0] not in wm.HOST_NAMED}, {item for item in used if item[0] not in wm.HOST_NAMED})
        self.assertLessEqual(used, named)
        self.assertEqual([item for item in sorted(named) if not wm.header_declares(*item)], [])

    def test_modules_that_disagree_on_a_current_signature_fail(self):
        first = dict(ADAPTED["modules"][0], majors={"wind_meteors": 1, "weapons": 1, "services": 2}, imports={"services": {"Rand": [["int"], "int"]}})
        second = dict(first, name="n", imports={"services": {"Rand": [["linear"], "int"]}})
        with self.assertRaises(SystemExit):
            wm.current_signatures(ADAPTED, [first, second], ADAPTED["majors"])

    def test_the_manifest_writer_keeps_adapters_one_per_line(self):
        text = wm.manifest_text(dict(ADAPTED, adapters=[RAND_ADAPTER, dict(RAND_ADAPTER, name="Log")]))
        self.assertIn('"adapters": [\n    {"interface": "services", "from_major": 1, "name": "Rand", "args": [0], "result": "pass"},\n', text)
        self.assertEqual(json.loads(text)["adapters"][1]["name"], "Log")


class RuntimeCopyTests(unittest.TestCase):
    GAME = {"modules": [{"name": "alpha"}, {"name": "beta", "blocked": "not yet"}]}

    def test_copies_the_manifest_and_only_the_unblocked_modules(self):
        with tempfile.TemporaryDirectory() as built, tempfile.TemporaryDirectory() as dest:
            for name in ("alpha.wasm", "beta.wasm", "c.wasm"):
                with open(os.path.join(built, name), "wb") as f:
                    f.write(b"x")
            self.assertEqual(wm.copy_runtime(self.GAME, built, os.path.join(dest, "out")), [])
            self.assertEqual(sorted(os.listdir(os.path.join(dest, "out"))), ["alpha.wasm", "modules.json"])

    def test_a_missing_built_file_is_reported_and_nothing_copied(self):
        with tempfile.TemporaryDirectory() as built, tempfile.TemporaryDirectory() as dest:
            self.assertEqual(wm.copy_runtime(self.GAME, built, dest), ["alpha.wasm"])
            self.assertEqual(os.listdir(dest), [])


BUILT = os.path.join(wm.OUT, "wind_meteors.wasm")
OBJECTS = os.path.join(wm.OUT, "wind_meteors")


def relink(entry, game, target):
    """The stock module's objects linked at the entry's bases and metered, as the build does, without the build's checks."""
    objects = sorted(os.path.join(OBJECTS, name) for name in os.listdir(OBJECTS) if name.endswith(".o"))
    exports = [f"--export={data}" for data in entry["data"]]
    wm.run(["wasm-ld", *wm.link_flags(entry, game["stack_size"]), "--gc-sections", *exports, *objects, "-o", target])
    with open(target, "rb") as f:
        linked = f.read()
    with open(target, "wb") as f:
        f.write(wm.wasm_fuel.instrument(linked, wm.find_wasm_tools(game["tools"]["wasm-tools"])))


@unittest.skipUnless(os.path.exists(BUILT) and os.path.isdir(OBJECTS), "needs uv run tools/wasm_modules.py build first")
class ModuleFileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.game = wm.game_manifest()
        cls.entry = next(m for m in cls.game["modules"] if m["name"] == "wind_meteors")
        cls.wasm_tools = wm.find_wasm_tools(cls.game["tools"]["wasm-tools"])
        cls.dir = tempfile.TemporaryDirectory()
        cls.moved = os.path.join(cls.dir.name, "moved.wasm")
        relink(dict(cls.entry, data_base=0x40000), cls.game, cls.moved)
        cls.extra = os.path.join(cls.dir.name, "extra.wasm")
        with open(BUILT, "rb") as f:
            text = subprocess.run([cls.wasm_tools, "print", "-"], input=f.read(), capture_output=True, check=True).stdout.decode()
        first = text.index("(import ")
        text = text[:first] + '(import "services" "ClockNow" (func (result i32)))\n  ' + text[first:]
        with open(cls.extra, "wb") as f:
            f.write(subprocess.run([cls.wasm_tools, "parse", "-"], input=text.encode(), capture_output=True, check=True).stdout)

    @classmethod
    def tearDownClass(cls):
        cls.dir.cleanup()

    def problems(self, path):
        return wm.check_module_file(self.game, self.entry, path, self.wasm_tools)

    def test_the_stock_module_passes(self):
        self.assertEqual(self.problems(BUILT), [])

    def test_a_module_relinked_inside_the_memory_but_outside_its_window_fails(self):
        problems = self.problems(self.moved)
        self.assertTrue(any("outside its window 0x30000..0x38000" in p for p in problems), problems)

    def test_an_import_off_the_allowlist_fails(self):
        problems = self.problems(self.extra)
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("imports services.ClockNow, which its manifest does not list", problems[0])

    def test_a_file_that_is_not_a_module_fails(self):
        path = os.path.join(self.dir.name, "junk.wasm")
        with open(path, "wb") as f:
            f.write(b"not a module")
        self.assertIn("cannot read the module", self.problems(path)[0])

    def test_the_command_exits_by_the_verdict(self):
        def run(path):
            cmd = [sys.executable, os.path.join(wm.ROOT, "tools", "wasm_modules.py"), "check-module", path, "--name", "wind_meteors"]
            return subprocess.run(cmd, capture_output=True, text=True)
        stock, moved = run(BUILT), run(self.moved)
        self.assertEqual((stock.returncode, stock.stdout), (0, "wind_meteors: ok\n"))
        self.assertEqual(moved.returncode, 1)
        self.assertIn("outside its window", moved.stdout)

    def test_the_judge_refuses_a_bad_module_before_playing(self):
        import oraclegate

        out = os.path.join(self.dir.name, "oracle")
        for path, text in ((self.moved, "outside its window"), (self.extra, "services.ClockNow")):
            result = oraclegate.judge(oraclegate.Path(path), "wind_meteors", False, oraclegate.Path(self.dir.name) / "no-build",
                                      oraclegate.Path(out), None)
            self.assertEqual(result["verdict"], "refused")
            self.assertIn(text, result["message"])
            self.assertNotIn("scenarios", result)
            self.assertFalse(os.path.exists(os.path.join(out, "runtime")))

    def edited(self, name, change):
        text = subprocess.run([self.wasm_tools, "print", BUILT], capture_output=True, text=True, check=True).stdout
        changed = change(text)
        self.assertNotEqual(changed, text)
        path = os.path.join(self.dir.name, name)
        with open(path, "wb") as f:
            f.write(subprocess.run([self.wasm_tools, "parse", "-"], input=changed.encode(), capture_output=True, check=True).stdout)
        return path

    def test_an_export_of_an_imported_function_is_named(self):
        path = self.edited("imported-export.wasm", lambda t: t.replace('(export "UpdateWind" (func $export_UpdateWind))', '(export "UpdateWind" (func 0))'))
        problems = self.problems(path)
        self.assertTrue(any("exports UpdateWind as an imported function" in p for p in problems), problems)

    def test_a_module_without_names_passes(self):
        path = os.path.join(self.dir.name, "stripped.wasm")
        subprocess.run([self.wasm_tools, "strip", "-a", BUILT, "-o", path], check=True)
        self.assertNotIn(b"export_UpdateWind", open(path, "rb").read())
        self.assertEqual(self.problems(path), [])

    def test_a_module_without_an_element_segment_passes(self):
        with open(BUILT, "rb") as f:
            data = f.read()
        self.assertTrue(wm.parse_module(data)["elements"])
        text = subprocess.run([self.wasm_tools, "print", "-"], input=data, capture_output=True, check=True).stdout.decode()
        lines = [line for line in text.split("\n") if not line.lstrip().startswith("(elem ")]
        self.assertLess(len(lines), len(text.split("\n")))
        path = os.path.join(self.dir.name, "no-elem.wasm")
        with open(path, "wb") as f:
            f.write(subprocess.run([self.wasm_tools, "parse", "-"], input="\n".join(lines).encode(), capture_output=True, check=True).stdout)
        self.assertEqual(wm.parse_module(open(path, "rb").read())["elements"], [])
        self.assertEqual(self.problems(path), [])

    def test_exports_without_metering_are_refused(self):
        path = self.edited("unmetered.wasm", lambda t: t.replace("i64.sub", "i64.add"))
        problems = self.problems(path)
        self.assertTrue(any("metering surface: has no metering code" in p for p in problems), problems)


class ModuleWithoutPlaces(unittest.TestCase):
    GAME = wm.game_manifest()
    ENTRY = GAME["modules"][0]

    def info(self, **changes):
        return dict({"memory_min": 4, "table_min": 65, "stack_top": 0x30100, "elements": [], "segments": [(0x30000, 16)]}, **changes)

    def run_with(self, info):
        saved = wm.parse_module
        wm.parse_module = lambda data: info
        try:
            with tempfile.NamedTemporaryFile(suffix=".wasm") as f:
                return wm.check_module_file(self.GAME, self.ENTRY, f.name, "wasm-tools-is-not-reached")
        finally:
            wm.parse_module = saved

    def test_no_stack_pointer_has_its_own_message(self):
        problems = self.run_with(self.info(stack_top=None))
        self.assertEqual(len(problems), 1)
        self.assertIn("has no stack pointer", problems[0])

    def test_no_data_segment_has_its_own_message(self):
        problems = self.run_with(self.info(segments=[]))
        self.assertEqual(len(problems), 1)
        self.assertIn("has no data segment", problems[0])

    def test_a_section_past_the_end_is_named(self):
        with self.assertRaisesRegex(ValueError, "section 11 runs 6 bytes past the end"):
            wm.parse_module(b"\0asm\1\0\0\0" + bytes([11, 10, 0, 0, 0, 0]))


class CheckModuleExit(unittest.TestCase):
    def run_command(self, *args, env=None):
        cmd = [sys.executable, os.path.join(wm.ROOT, "tools", "wasm_modules.py"), "check-module", *args]
        return subprocess.run(cmd, capture_output=True, text=True, env=env)

    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.module = os.path.join(self.dir.name, "m.wasm")
        with open(self.module, "wb") as f:
            f.write(b"\0asm\1\0\0\0")

    def tearDown(self):
        self.dir.cleanup()

    def test_an_unknown_subsystem_could_not_be_checked(self):
        r = self.run_command(self.module, "--name", "nope")
        self.assertEqual(r.returncode, 2)
        self.assertIn("the manifest has no module nope", r.stderr)

    def test_a_missing_wasm_tools_could_not_be_checked(self):
        r = self.run_command(self.module, "--name", "wind_meteors", env=dict(os.environ, WASM_TOOLS=os.path.join(self.dir.name, "none")))
        self.assertEqual(r.returncode, 2)
        self.assertEqual(r.stdout, "")

    def test_a_missing_file_could_not_be_checked(self):
        self.assertEqual(self.run_command(os.path.join(self.dir.name, "none.wasm"), "--name", "wind_meteors").returncode, 2)

    def test_an_internal_exception_could_not_be_checked(self):
        def broken(*_args):
            raise RuntimeError("boom")
        saved = wm.check_module_file
        wm.check_module_file = broken
        try:
            code = wm.run_check_module(self.module, "wind_meteors")
        finally:
            wm.check_module_file = saved
        self.assertEqual(code, 2)


class GlueManifestTests(unittest.TestCase):
    """The manifest entry of every subsystem and the generic glue's tables (tools/wasm_glue.py)."""

    @classmethod
    def setUpClass(cls):
        import wasm_glue
        cls.glue = wasm_glue
        cls.game = wm.game_manifest()
        cls.schema = wasm_glue.Schema()
        cls.seams = wasm_glue.seams(cls.schema)
        cls.tables = open(wasm_glue.TABLES).read()

    def test_every_subsystem_has_an_entry(self):
        self.assertEqual([m["name"] for m in self.game["modules"]], self.glue.MODULES)

    def test_the_windows_and_table_ranges_do_not_meet(self):
        ranges = [(m["name"], m["data_base"], m["data_base"] + self.game["window"], m["table_base"], m["table_base"] + self.game["table_slots"])
                  for m in self.game["modules"]] + wm.fixture_ranges()
        self.assertEqual(wm.check_layout(self.game, [r[:3] for r in ranges]), [])
        tables = sorted(r[3:] for r in ranges)
        self.assertTrue(all(a[1] <= b[0] for a, b in zip(tables, tables[1:])), tables)
        self.assertLessEqual(tables[-1][1], self.game["table_size"])

    def test_exports_and_engine_slots_are_the_header_in_order(self):
        for entry in self.game["modules"]:
            header = [s.name for s in self.seams[entry["name"]]]
            self.assertEqual(entry["exports"], [n for n in header if n not in entry.get("engine", {})], entry["name"])
            self.assertTrue(set(entry.get("engine", {})) <= set(header), entry["name"])
            self.assertEqual(list(entry["slots"]), entry["exports"], entry["name"])

    def test_a_blocked_entry_names_its_reason_and_a_built_one_its_budgets(self):
        for entry in self.game["modules"]:
            if "blocked" in entry:
                self.assertTrue(entry["blocked"].strip(), entry["name"])
            else:
                self.assertIn("fuel_budget", entry)
                self.assertIn("size_budget", entry)
        self.assertEqual([m["name"] for m in wm.built_modules(self.game)], ["wind_meteors"])

    def test_the_region_modules_have_the_region_word(self):
        with_region = sorted(m["name"] for m in self.game["modules"] if "g_region" in m["data"])
        self.assertEqual(with_region, ["economy", "los_radar", "pathfinding", "players"])

    def test_the_host_routes_take_a_routed_kind_and_not_an_unrouted_one(self):
        self.assertIn("{SCHED_WIND_METEORS, 80},", self.tables)
        self.assertNotIn("{SCHED_WIND_METEORS, 132},", self.tables)
        self.assertIn("{SCHED_MAP, 132},", self.tables)

    def test_only_the_modules_whose_recordings_emit_publish_through_emit_from(self):
        emit_from = [m for m in self.glue.MODULES if f'{{"{m}", ' in self.tables
                     and re.search(r'k\w+Def = \{"%s",[^;]*, 0\};' % m, self.tables)]
        self.assertEqual(emit_from, ["wind_meteors", "targeting"])

    def test_the_scheduled_imports_call_the_wrappers(self):
        self.assertIn("ScheduleEconomyRequestGranted(unit, resources)", self.tables)
        self.assertIn("ScheduleMapCanBuildAt(unit, *cell, type, player)", self.tables)

    def test_the_tables_and_manifest_are_current(self):
        manifest_text, tables_text = self.glue.build()
        self.assertEqual(manifest_text, open(wm.GAME_MODULES).read())
        self.assertEqual(tables_text, self.tables)


if __name__ == "__main__":
    unittest.main()
