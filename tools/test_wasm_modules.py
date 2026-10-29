"""Tests for the import allowlist and window checks of wasm_modules.py.

    uv run tools/test_wasm_modules.py
"""

import unittest

import wasm_modules as wm

SPEC = {
    "imports": {"weapons": ["SpawnProjectile"], "services": ["RandomInt"]},
    "exports": ["UpdateWind", "g_game"],
}
GOOD_IMPORTS = [("env", "memory"), ("env", "__indirect_function_table"), ("weapons", "SpawnProjectile"), ("services", "RandomInt")]
GOOD_EXPORTS = ["UpdateWind", "g_game"]
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
        self.assertEqual(len(problems), 1)
        self.assertIn("scratch", problems[0])

    def test_module_reaching_the_block_fails(self):
        problems = wm.check_layout(self.PINS, self.RANGES + [("c", 1040000, 1048577)])
        self.assertTrue(any("reaches the block base" in p for p in problems))

    def test_touching_ranges_pass(self):
        self.assertEqual(wm.check_layout(self.PINS, self.RANGES + [("c", 202688, 983040)]), [])

    def test_pages_must_hold_the_block(self):
        self.assertTrue(wm.check_layout(dict(self.PINS, memory_pages=527), self.RANGES))


if __name__ == "__main__":
    unittest.main()
