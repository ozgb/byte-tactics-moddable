"""Tests for viewblock.py: the layout of interfaces/view.h, the decoding of a recorded view and its comparison.

    uv run tools/test_viewblock.py
"""

import struct
import tempfile
import unittest
from pathlib import Path

import viewblock as vb

HEAD = "<HHIBBBBHHHH" + "IIII" + "iiii" + "9I"
PLAYER = "<8B4HIHH10f" + "II" + "4d"
UNIT = "<HHBBBBI" + "8B" + "3i" + "hhhH" + "fHH" + "4I" + "4f"
BLIP = "<HBB3i"
MESSAGE = "<HH6I"


def header(tick=1, grid=(0, 0), counts=(0,) * 9):
    return struct.pack(HEAD, 1, 0, tick, 0, 0, 0, 0, 0, 0, 0, 0, 64, 64, grid[0], grid[1], 0, 0, 0, 0, *counts)


def players(metal=1000.0):
    rows = b""
    for i in range(10):
        rows += struct.pack(PLAYER, i, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, metal + i, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0)
    return rows


def unit(x):
    return struct.pack(UNIT, 7, 3, 1, 1, 0, 0, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, x, 5, 6, 0, 0, 100, 0, 0.0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0)


class LayoutTest(unittest.TestCase):
    def test_sizes_follow_the_i686_rules(self):
        lay = vb.layouts()
        self.assertEqual(lay["ViewPlayer"].size, struct.calcsize(PLAYER))
        self.assertEqual(lay["ViewPlayer"].align, 8)
        self.assertEqual(lay["ViewUnit"].size, struct.calcsize(UNIT))
        self.assertEqual(lay["ViewBlip"].size, struct.calcsize(BLIP))
        self.assertEqual(lay["ViewMessage"].size, struct.calcsize(MESSAGE))
        self.assertEqual(vb.header_size(), struct.calcsize(HEAD))

    def test_constants_come_from_the_header(self):
        self.assertEqual(vb.capacity("units"), vb.constants()["VIEW_MAX_UNITS"])
        self.assertEqual(vb.capacity("sight"), 1048576)


class DecodeTest(unittest.TestCase):
    def block(self, x=1024, sight=b""):
        grid = (len(sight), 1) if sight else (0, 0)
        return header(grid=grid, counts=(1, 0, 0, 0, 0, 0, 0, 0, 0)) + players() + unit(x) + sight

    def test_paths_and_values(self):
        got = dict(vb.decode(self.block()))
        self.assertEqual(got["tick"], 1)
        self.assertEqual(got["players[0].metal"], 1000.0)
        self.assertEqual(got["players[9].metal"], 1009.0)
        self.assertEqual(got["units[0].x"], 1024)
        self.assertEqual(got["units[0].flags"], 0x10)
        self.assertNotIn("units[1].x", got)

    def test_order_is_serialisation_order(self):
        paths = [p for p, _ in vb.decode(self.block(sight=b"\x00\x02"))]
        self.assertLess(paths.index("players[9].metal"), paths.index("units[0].id"))
        self.assertEqual(paths[-2:], ["sight[0]", "sight[1]"])

    def test_header_only_block(self):
        got = vb.decode(header())
        self.assertEqual(got[0], ("versionMajor", 1))
        self.assertTrue(all(not p.startswith("players") for p, _ in got))

    def test_truncated_sight_grid(self):
        data = self.block(sight=b"\x01\x02\x00\x01")[:-2]
        paths = [p for p, _ in vb.decode(data)]
        self.assertEqual(paths[-2:], ["sight[0]", "sight[1]"])
        self.assertEqual(vb.serialisation_size(data) - len(data), 2)

    def test_the_counts_cap_at_the_capacity(self):
        data = header(counts=(0, 0, 0, 99999, 0, 0, 0, 0, 0))
        self.assertEqual(vb.serialisation_size(data), vb.header_size() + 10 * struct.calcsize(PLAYER) + 5001 * struct.calcsize(BLIP))

    def test_the_sight_grid_caps_at_the_cell_limit(self):
        data = header(grid=(4096, 4096))
        self.assertEqual(vb.serialisation_size(data) - vb.header_size() - 10 * struct.calcsize(PLAYER), 1048576)


class DiffTest(unittest.TestCase):
    def test_equal(self):
        data = header() + players()
        self.assertIsNone(vb.first_difference(data, data))

    def test_names_the_first_field_in_serialisation_order(self):
        a = header(counts=(2, 0, 0, 0, 0, 0, 0, 0, 0)) + players() + unit(1) + unit(2)
        b = header(counts=(2, 0, 0, 0, 0, 0, 0, 0, 0)) + players(2000.0) + unit(9) + unit(9)
        self.assertEqual(vb.first_difference(a, b), ("players[0].metal", "1000", "2000"))
        c = header(counts=(2, 0, 0, 0, 0, 0, 0, 0, 0)) + players() + unit(1) + unit(9)
        self.assertEqual(vb.first_difference(a, c), ("units[1].x", "2", "9"))

    def test_a_missing_part_is_dash(self):
        a = header() + players()
        self.assertEqual(vb.first_difference(a, header()), ("players[0].active", "0", "-"))

    def test_padding_only(self):
        a = header() + players()
        b = bytearray(a)
        b[vb.header_size() + 7] = 1
        found = vb.first_difference(a, bytes(b))
        self.assertEqual(found[0], "players[0].reserved")
        pad = bytearray(a)
        pad[vb.header_size() + struct.calcsize("<8B4HIHH10f2I") + 0] = 0
        self.assertIsNone(vb.first_difference(a, bytes(pad)))

    def test_files(self):
        with tempfile.TemporaryDirectory() as d:
            paths = []
            for name, metal in (("a", 1.0), ("b", 2.0)):
                path = Path(d) / name
                path.write_bytes(vb.FILE_HEADER + header() + players(metal))
                paths.append(path)
            self.assertEqual(vb.describe_difference(*paths), "players[0].metal: a=1 b=2")
            paths[1].write_bytes(b"NOPE" + b"\0" * 8)
            with self.assertRaises(vb.Error):
                vb.describe_difference(*paths)


class HashTest(unittest.TestCase):
    def test_words_then_bytes(self):
        h = vb.FNV_OFFSET
        h = ((h ^ int.from_bytes(b"12345678", "little")) * vb.FNV_PRIME) & vb.MASK
        for x in b"abc":
            h = ((h ^ x) * vb.FNV_PRIME) & vb.MASK
        self.assertEqual(vb.view_hash(b"12345678abc"), h)

    def test_empty_is_the_offset_basis(self):
        self.assertEqual(vb.view_hash(b""), vb.FNV_OFFSET)

    def test_one_run_is_not_two(self):
        self.assertNotEqual(vb.view_hash(b"abc" + b"defgh"), vb.view_hash(b"abcdefgh"[:0]))


if __name__ == "__main__":
    unittest.main()
