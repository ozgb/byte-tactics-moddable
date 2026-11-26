"""Tests for tools/displayreplay.py on frames and lists made here: the frame comparison, the verdict and the kind coverage.

    uv run tools/test_displayreplay.py
"""

import lzma
import tempfile
import unittest
from pathlib import Path

import displaylog
import displayreplay
from displayreplay import frame_difference, judge

WIDTH, HEIGHT = 8, 4


def picture(pixels: bytes, width: int = WIDTH, height: int = HEIGHT, tick: int = 16, palette: bytes = bytes(1024)) -> bytes:
    """A frame dump (docs/frame-dump.md) of these pixels."""
    header = b"BTFR" + (1).to_bytes(2, "little") + width.to_bytes(2, "little") + height.to_bytes(2, "little")
    header += (0).to_bytes(2, "little") + tick.to_bytes(4, "little")
    return header + palette + pixels


BLANK = picture(bytes(WIDTH * HEIGHT))


def with_pixels(*changes: tuple[int, int, int]) -> bytes:
    pixels = bytearray(WIDTH * HEIGHT)
    for x, y, colour in changes:
        pixels[y * WIDTH + x] = colour
    return picture(bytes(pixels))


class FrameDifferenceTest(unittest.TestCase):
    def test_equal_frames(self):
        self.assertIsNone(frame_difference(BLANK, BLANK))

    def test_one_pixel(self):
        self.assertEqual(frame_difference(BLANK, with_pixels((3, 2, 7))),
                         "1 pixels differ in x 3..3 y 2..2, the first at x 3 y 2: stored 0, got 7")

    def test_the_box_and_the_first_in_row_order(self):
        diff = frame_difference(with_pixels((6, 1, 5), (1, 3, 9)), BLANK)
        self.assertEqual(diff, "2 pixels differ in x 1..6 y 1..3, the first at x 6 y 1: stored 5, got 0")

    def test_the_header_and_the_palette(self):
        self.assertEqual(frame_difference(BLANK, picture(bytes(WIDTH * HEIGHT), tick=17)), "header")
        self.assertEqual(frame_difference(BLANK, picture(bytes(WIDTH * HEIGHT), palette=b"\x01" + bytes(1023))), "palette")

    def test_not_a_frame(self):
        with self.assertRaises(displayreplay.Error):
            frame_difference(b"XXXX" + BLANK[4:], BLANK[:-1] + b"\x01")


class JudgeTest(unittest.TestCase):
    def test_every_frame_equal(self):
        self.assertIsNone(judge("s", [16, 40], {16: BLANK, 40: BLANK}, {16: BLANK, 40: BLANK}, ["2 frames replayed"]))

    def test_the_first_frame_that_differs(self):
        diff = judge("s", [16, 40], {16: BLANK, 40: BLANK}, {16: BLANK, 40: with_pixels((0, 0, 1))}, [])
        self.assertTrue(diff.startswith("frame 40: 1 pixels differ"))

    def test_a_frame_the_replay_did_not_write(self):
        self.assertEqual(judge("s", [16], {16: BLANK}, {}, ["a picture with no picture line: @x:1x1:0"]),
                         "frame 16: the replay wrote no frame (a picture with no picture line: @x:1x1:0)")

    def test_a_missing_stored_frame(self):
        self.assertIn("no stored frame", judge("s", [16], {16: None}, {16: BLANK}, []))

    def test_an_item_not_drawn_fails_equal_frames(self):
        self.assertEqual(judge("s", [16], {16: BLANK}, {16: BLANK}, ["1 frames replayed", "3 shard items not drawn"]),
                         "3 shard items not drawn")


class NotesTest(unittest.TestCase):
    def test_the_replay_lines_of_a_log(self):
        log = "0120:fixme:x\ntestreplay: 2 frames replayed\nother\ntestreplay: 4 debris items not drawn\n"
        notes = displayreplay.replay_notes(log)
        self.assertEqual(notes, ["2 frames replayed", "4 debris items not drawn"])
        self.assertEqual(displayreplay.undrawn(notes), ["4 debris items not drawn"])


def stored(directory: Path, name: str, *items: str) -> None:
    text = "bt-display 1.1\nframe 1 0 0 0123456789abcdef\n" + "".join(i + "\n" for i in items) + "cursor - - -\n"
    (directory / f"{name}{displayreplay.LIST_SUFFIX}").write_bytes(displayreplay.pack_list(text))


class CoverageTest(unittest.TestCase):
    def test_kinds_by_stored_list(self):
        with tempfile.TemporaryDirectory() as d:
            stored(Path(d), "a", "fill screen 0 0 1 1 3", "line screen 0 0 2 2 0 0 1 1 4")
            stored(Path(d), "b", "fill screen 0 0 1 1 3")
            found = displayreplay.coverage(Path(d))
        self.assertEqual(found["fill"], ["a", "b"])
        self.assertEqual(found["line"], ["a"])
        self.assertEqual(found["lens"], [])
        self.assertEqual(set(found), set(displaylog.KINDS))

    def test_a_kind_in_no_list_fails_unless_left_out(self):
        found = {"fill": ["a"], "lens": []}
        missing, lines = displayreplay.coverage_report(found)
        self.assertEqual(missing, ["lens"])
        self.assertIn("lens: in no stored list", lines)
        old = dict(displayreplay.LEFT_OUT)
        displayreplay.LEFT_OUT["lens"] = "why"
        try:
            missing, lines = displayreplay.coverage_report(found)
        finally:
            displayreplay.LEFT_OUT.clear()
            displayreplay.LEFT_OUT.update(old)
        self.assertEqual(missing, [])
        self.assertIn("lens: left out (why)", lines)

    def test_a_stored_list_is_the_same_bytes_for_the_same_text(self):
        text = "bt-display 1.1\n" + "x" * 1000
        self.assertEqual(displayreplay.pack_list(text), displayreplay.pack_list(text))
        self.assertEqual(lzma.decompress(displayreplay.pack_list(text)).decode("latin-1"), text)


if __name__ == "__main__":
    unittest.main()
