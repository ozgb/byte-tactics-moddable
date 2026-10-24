"""Tests for tracerehash.py, on hand-built hash inputs and a tiny schema.

    uv run tools/test_tracerehash.py
"""

import lzma
import tempfile
import unittest
from pathlib import Path

import tracerehash as tr

SCHEMA = tr.Schema(
    {
        "targets": [{"tag": 1, "name": "heap"}, {"tag": 2, "name": "unit"}],
        "tables": {
            "T": {
                "fields": [
                    {"name": "a", "offset": 0, "size": 4, "kind": "scalar", "target": None},
                    {"name": "", "offset": 4, "size": 2, "kind": "bytes", "target": None},
                    {"name": "p", "offset": 6, "size": 4, "kind": "ref", "target": "unit"},
                    {"name": "m", "offset": 10, "size": 2, "kind": "scalar", "target": None, "mask": 0xFF0},
                ],
                "plan": [
                    {"name": "", "offset": 0, "size": 6, "kind": "bytes", "target": None},
                    {"name": "p", "offset": 6, "size": 4, "kind": "ref", "target": "unit"},
                    {"name": "m", "offset": 10, "size": 2, "kind": "scalar", "target": None, "mask": 0xFF0},
                ],
            }
        },
    }
)


def hashed(*runs):
    h = tr.Hasher()
    for r in runs:
        h.run(r)
    return h.h


class HasherTest(unittest.TestCase):
    def test_empty_is_offset_basis(self):
        self.assertEqual(hashed(), tr.OFFSET_BASIS)

    def test_one_byte(self):
        self.assertEqual(hashed(b"\x01"), ((tr.OFFSET_BASIS ^ 1) * tr.PRIME) & tr.MASK64)

    def test_eight_bytes_is_one_word_step(self):
        w = int.from_bytes(bytes(range(1, 9)), "little")
        self.assertEqual(hashed(bytes(range(1, 9))), ((tr.OFFSET_BASIS ^ w) * tr.PRIME) & tr.MASK64)

    def test_nine_bytes_word_then_byte(self):
        h = hashed(bytes(range(1, 9)))
        self.assertEqual(hashed(bytes(range(1, 10))), ((h ^ 9) * tr.PRIME) & tr.MASK64)

    def test_runs_are_chunked_on_their_own(self):
        self.assertNotEqual(hashed(b"\x01" * 4, b"\x02" * 4), hashed(b"\x01" * 4 + b"\x02" * 4))
        self.assertEqual(hashed(b"\x01" * 8, b"\x02" * 8), hashed(b"\x01" * 8 + b"\x02" * 8))

    def test_h32_is_four_byte_steps(self):
        h = tr.Hasher()
        h.h32(0x04030201)
        self.assertEqual(h.h, hashed(b"\x01", b"\x02", b"\x03", b"\x04"))

    def test_end(self):
        h = tr.Hasher()
        h.end()
        self.assertEqual(h.h, hashed(b"\xff", b"\xff", b"\xff", b"\xff"))

    def test_pointer_is_tag_byte_then_index_word(self):
        h = tr.Hasher()
        h.pointer(2, 5)
        self.assertEqual(h.h, hashed(b"\x02", b"\x05", b"\x00", b"\x00", b"\x00"))


class DumpTest(unittest.TestCase):
    def test_parse_dump_keeps_values_with_spaces(self):
        blocks = tr.parse_dump("bt-dump 3\n7 units a.b str:x y\n7 units a.c 0x01\n")
        self.assertEqual(blocks[(7, "units")], [("a.b", "str:x y"), ("a.c", "0x01")])

    def test_parse_dump_rejects_other_formats(self):
        with self.assertRaises(tr.Error):
            tr.parse_dump("bt-dump 2\n")

    def test_parse_trace_rows(self):
        rows = tr.parse_trace_rows("bt-trace 3\n5 units 0123456789abcdef\n")
        self.assertEqual(rows, {(5, "units"): "0123456789abcdef"})


class WalkerTest(unittest.TestCase):
    def walker(self, lines):
        return tr.Walker(SCHEMA, lines)

    def test_pointer_ids(self):
        w = self.walker([])
        self.assertEqual(w.pointer_id("null"), (0, 0))
        self.assertEqual(w.pointer_id("heap"), (1, 0))
        self.assertEqual(w.pointer_id("rec:overflow"), (9, 0xFFFFFFFF))
        self.assertEqual(w.pointer_id("unit:12"), (2, 12))

    def test_table_merges_bytes_masks_and_ids(self):
        w = self.walker([("t.a", "0x04030201"), ("t.+0x4", "hex:0605"), ("t.p", "unit:3"), ("t.m", "0x0ff0")])
        w.table("T", "t")
        want = tr.Hasher()
        want.run(bytes([1, 2, 3, 4, 6, 5]))
        want.pointer(2, 3)
        want.h32(0x0FF0)
        self.assertEqual(w.hash.h, want.h)
        self.assertEqual(w.pos, 4)

    def test_wrong_path_is_an_error(self):
        w = self.walker([("t.b", "0x00")])
        with self.assertRaises(tr.Error):
            w.take("t.a")

    def test_present_follows_first_field(self):
        w = self.walker([("t.a", "0x00000000")])
        self.assertTrue(w.present("T", "t"))
        self.assertFalse(w.present("T", "u"))


class ReadTextTest(unittest.TestCase):
    def test_xz_path_is_decompressed(self):
        with tempfile.TemporaryDirectory() as d:
            plain, packed = Path(d, "a.txt"), Path(d, "a.txt.xz")
            plain.write_text("1 units 0123456789abcdef\n")
            with lzma.open(packed, "wt") as f:
                f.write("1 units 0123456789abcdef\n")
            self.assertEqual(tr.read_text(str(packed)), tr.read_text(str(plain)))


def one_field_table(name):
    field = {"name": name, "offset": 0, "size": 1, "kind": "scalar", "target": None}
    return {"fields": [field], "plan": [{"name": "", "offset": 0, "size": 1, "kind": "bytes", "target": None}]}


DEFINITIONS = tr.Schema(
    {
        "targets": [],
        "tables": {
            "UnitDefData": one_field_table("u"),
            "WeaponDefData": one_field_table("w"),
            "MissionData": one_field_table("type"),
            "MissionStartRest": one_field_table("rest"),
        },
    }
)


def expected(steps):
    h = tr.Hasher()
    for kind, value in steps:
        if kind == "8":
            h.h8(value)
        elif kind == "32":
            h.h32(value)
        else:
            h.run(value)
    return h.h


class DefinitionSectionsTest(unittest.TestCase):
    def run_section(self, method, lines):
        w = tr.Walker(DEFINITIONS, lines)
        getattr(w, method)()
        self.assertEqual(w.pos, len(lines))
        return w.hash.h

    def test_string_null_and_text(self):
        w = tr.Walker(DEFINITIONS, [("a", "null"), ("b", "str:hi there")])
        w.string("a")
        w.string("b")
        self.assertEqual(w.hash.h, expected([("8", 0), ("8", 1), ("32", 8), ("b", b"hi there")]))

    def test_string_empty(self):
        w = tr.Walker(DEFINITIONS, [("a", "str:")])
        w.string("a")
        self.assertEqual(w.hash.h, expected([("8", 1), ("32", 0)]))

    def test_unit_definitions(self):
        lines = [
            ("defs.count", "0x00000001"),
            ("defs[0].u", "0x07"),
            ("defs[0].yardmap", "0x00000002"),
            ("defs[0].yardmap", "hex:0102"),
            ("defs[0].ids", "0x00000000"),
            ("defs[0].ids", "hex:"),
            ("defs[0].categories[1]", "hex:" + "aa" * 0x40),
            ("defs[0].categories[3]", "hex:" + "bb" * 0x40),
        ]
        got = self.run_section("unit_definitions", lines)
        want = expected(
            [("8", 29), ("32", 1), ("b", b"\x07"), ("32", 2), ("b", b"\x01\x02"), ("32", 0), ("b", b"")]
            + [("8", 0), ("8", 1), ("b", b"\xaa" * 0x40), ("8", 0), ("8", 1), ("b", b"\xbb" * 0x40), ("32", 0xFFFFFFFF)]
        )
        self.assertEqual(got, want)

    def test_weapon_definitions(self):
        lines = []
        steps = [("8", 30)]
        for i in range(256):
            lines.append((f"weapons[{i}].w", "0x%02x" % (i & 0xFF)))
            steps.append(("b", bytes([i & 0xFF])))
            if i == 3:
                lines += [
                    ("weapons[3].damage.entries", "0x00000002"),
                    ("weapons[3].damage[0].name", "str:ARMCOM"),
                    ("weapons[3].damage[0].value", "0x0000000a"),
                    ("weapons[3].damage[1].name", "null"),
                    ("weapons[3].damage[1].value", "0x00000001"),
                ]
                steps += [("8", 1), ("32", 2), ("8", 1), ("32", 6), ("b", b"ARMCOM"), ("32", 10), ("8", 0), ("32", 1)]
            else:
                steps.append(("8", 0))
        steps.append(("32", 0xFFFFFFFF))
        self.assertEqual(self.run_section("weapon_definitions", lines), expected(steps))

    def test_mission_absent(self):
        self.assertEqual(self.run_section("mission", []), expected([("8", 31), ("8", 0), ("32", 0xFFFFFFFF)]))

    def test_mission_campaign(self):
        lines = [
            ("mission.type", "0x01"),
            ("mission.rest", "0x09"),
            ("mission.units.count", "0x00000001"),
            ("mission.units[0].name", "str:ARMCOM"),
            ("mission.units[0].ident", "null"),
            ("mission.units[0].initialMission", "str:x"),
            ("mission.units[0]", "hex:" + "01" * 0x18),
            ("mission.rules.count", "0x00000001"),
            ("mission.rules", "hex:" + "02" * 12),
            ("mission.features.count", "0x00000001"),
            ("mission.features[0].name", "str:tree"),
            ("mission.features[0].pos", "hex:0102030405060708"),
        ]
        want = expected(
            [("8", 31), ("8", 1), ("b", b"\x01"), ("b", b"\x09"), ("32", 1)]
            + [("8", 1), ("32", 6), ("b", b"ARMCOM"), ("8", 0), ("8", 1), ("32", 1), ("b", b"x")]
            + [("b", b"\x01" * 0x18), ("32", 1), ("b", b"\x02" * 12), ("32", 1)]
            + [("8", 1), ("32", 4), ("b", b"tree"), ("b", bytes(range(1, 9))), ("32", 0xFFFFFFFF)]
        )
        self.assertEqual(self.run_section("mission", lines), want)


if __name__ == "__main__":
    unittest.main()
