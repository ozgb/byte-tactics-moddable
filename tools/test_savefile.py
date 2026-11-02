import struct
import unittest

import savefile


def bank(account: str, boxes: dict[str, bytes], extra: int = 0, clutter: int = 0) -> bytes:
    """An uncompressed bank of one account holding an int item and the named boxes.

    `extra` bytes of padding in an account before it move it later in the file, as a longer compressed account would, and
    `clutter` is put in the table word that the game leaves uninitialised for a named box.
    """
    pool = b""
    at: dict[str, int] = {}
    for text in ("Bank", account, "Item", *boxes):
        at[text] = len(pool)
        pool += text.encode() + b"\0"
    lead = struct.pack("<8i", 0x20 + extra, at["Bank"], 0, 0, 0, 0, 0, 0) + bytes(extra)
    start = 34 + len(lead)
    table = struct.pack("<ii", at["Item"], 7)
    place = start + 0x20 + len(table) + 16 * len(boxes)
    data = b""
    for key, content in boxes.items():
        table += struct.pack("<iiii", at[key], clutter, place + len(data), len(content))
        data += content
    body = table + data
    chunk = struct.pack("<8i", 0x20 + len(body), at[account], 1, 0, 0, len(boxes), 0, 0) + body
    header = b"HAPIBANK" + struct.pack("<iiii", 0, start + len(chunk), 34, 1) + b"\0" * 10
    return header + lead + chunk + pool


def units(key: str, content: bytes, **kw) -> bytes:
    return bank("Units", {key: content}, **kw)


class SaveFileTest(unittest.TestCase):
    def test_equal_banks(self):
        data = bank("Players", {"GameTime": bytes(28)})
        self.assertEqual(savefile.differences(data, data), [])

    def test_offsets_that_follow_the_compressed_sizes_are_not_differences(self):
        self.assertEqual(savefile.differences(bank("Players", {"A": b"xy"}), bank("Players", {"A": b"xy"}, extra=3, clutter=9)), [])

    def test_a_box_byte_that_differs_is_named_with_its_offset(self):
        found = savefile.differences(bank("Players", {"A": b"xyz"}), bank("Players", {"A": b"xyw"}))
        self.assertEqual(len(found), 1)
        self.assertIn("Players/A offset 0x2: stored 0x7a, got 0x77", found[0])

    def test_the_game_time_dword_is_masked_and_the_rest_is_not(self):
        stored = bytes(28)
        self.assertEqual(savefile.differences(bank("Players", {"GameTime": stored}), bank("Players", {"GameTime": b"\x5a\xe4\0\0" + stored[4:]})), [])
        self.assertEqual(len(savefile.differences(bank("Players", {"GameTime": stored}), bank("Players", {"GameTime": stored[:4] + b"\1" + stored[5:]}))), 1)

    def test_unit_record_masks(self):
        base = bytearray(0xB8)
        base[:4] = b"COM\0"
        other = bytearray(base)
        for offset, value in ((5, 0x77), (31, 1), (88, 0xE0), (112, 0xE0), (136, 0xE0), (182, 0x0E)):
            other[offset] = value
        self.assertEqual(savefile.differences(units("#0", bytes(base)), units("#0", bytes(other))), [])
        for offset, value in ((2, 0x77), (88, 0x01), (182, 0x01), (183, 0x01), (33, 1)):
            other = bytearray(base)
            other[offset] = value
            self.assertEqual(len(savefile.differences(units("#0", bytes(base)), units("#0", bytes(other)))), 1, offset)

    def test_motion_record_mask(self):
        motion = bytes(0x23)
        self.assertEqual(savefile.differences(units("u0001mob", motion), units("u0001mob", motion[:34] + b"\x48")), [])
        self.assertEqual(len(savefile.differences(units("u0001mob", motion), units("u0001mob", motion[:34] + b"\x01"))), 1)

    def test_point_marker_mask(self):
        marker = bytes(20)
        self.assertEqual(savefile.differences(units("u0001m0000g", marker), units("u0001m0000g", b"\x58\xfa\xe8\0" + marker[4:])), [])
        self.assertEqual(len(savefile.differences(units("u0001m0000g", marker), units("u0001m0000g", marker[:4] + b"\1" + marker[5:]))), 1)

    def test_a_file_that_is_not_a_bank_is_reported(self):
        self.assertIn("does not decode", savefile.differences(b"nope", b"nope")[0])

    def test_lzss_expands_literals_then_a_copy_of_them(self):
        self.assertEqual(savefile.lzss_expand(bytes([0x0C, 0x61, 0x62, 0x10, 0x00, 0x00, 0x00])), b"abab")


if __name__ == "__main__":
    unittest.main()
