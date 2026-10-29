"""Tests for the import allowlist and window checks of wasm_modules.py.

    uv run tools/test_wasm_modules.py
"""

import json
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
    PINS = {"block_base": 1048576, "block_size": 33554432, "scratch": [983040, 65536], "memory_pages": 528}
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


class Budgets(unittest.TestCase):
    def test_a_module_over_its_size_budget_fails(self):
        entry = {"name": "m", "size_budget": 1000}
        self.assertEqual(wm.check_size(entry, 1000), [])
        self.assertTrue(any("over its size_budget 1000" in p for p in wm.check_size(entry, 1001)))

    def test_a_module_without_a_size_budget_fails(self):
        self.assertTrue(wm.check_size({"name": "m"}, 10))

    def test_the_fuel_entry_must_be_the_passes_rule(self):
        pins = {"fuel": {"rule": wm.wasm_fuel.RULE, "version": wm.wasm_fuel.VERSION}, "modules": [{"name": "m", "fuel_budget": 5}]}
        self.assertEqual(wm.check_fuel_manifest(pins), [])
        self.assertTrue(wm.check_fuel_manifest(dict(pins, fuel={"rule": "other", "version": 1})))
        self.assertTrue(wm.check_fuel_manifest({"modules": pins["modules"]}))

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
        self.assertEqual(majors["wind_meteors"], 1)
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
        self.assertEqual(found["weapons"], {"Spawn": [["block", "int", "int"], "int"]})
        self.assertEqual(found["services"], {"Rand": [["int"], "int"], "Log": [["linear"], "void"]})

    def test_modules_that_disagree_on_a_current_signature_fail(self):
        first = dict(ADAPTED["modules"][0], majors={"wind_meteors": 1, "weapons": 1, "services": 2}, imports={"services": {"Rand": [["int"], "int"]}})
        second = dict(first, name="n", imports={"services": {"Rand": [["linear"], "int"]}})
        with self.assertRaises(SystemExit):
            wm.current_signatures(ADAPTED, [first, second], ADAPTED["majors"])

    def test_the_manifest_writer_keeps_adapters_one_per_line(self):
        text = wm.manifest_text(dict(ADAPTED, adapters=[RAND_ADAPTER, dict(RAND_ADAPTER, name="Log")]))
        self.assertIn('"adapters": [\n    {"interface": "services", "from_major": 1, "name": "Rand", "args": [0], "result": "pass"},\n', text)
        self.assertEqual(json.loads(text)["adapters"][1]["name"], "Log")


if __name__ == "__main__":
    unittest.main()
