"""Tests for tracerehash.py, on hand-built hash inputs and a tiny schema.

    uv run tools/test_tracerehash.py
"""

import unittest

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


if __name__ == "__main__":
    unittest.main()
