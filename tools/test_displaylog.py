"""Tests for tools/displaylog.py: parsing, layers, the digest and the comparisons of docs/display-log.md.

    uv run tools/test_displaylog.py
"""

import random
import tempfile
import unittest
from pathlib import Path

import displaylog
from displaylog import Error, digest_text, list_difference, parse_layered

PALETTE = "0123456789abcdef"
CURSOR = "cursor - - -"


def frame(*items: str, number: int = 1, tick: int = 0, steps: int = 1, cursor: str = CURSOR) -> str:
    return "".join(line + "\n" for line in [f"frame {number} {tick} {steps} {PALETTE}", *items, cursor])


def listing(*frames: str) -> str:
    return "bt-display 1\n" + "".join(frames)


def fill(target: str, x0: int, y0: int, x1: int, y1: int, colour: int = 1) -> str:
    return f"fill {target} {x0} {y0} {x1} {y1} {colour}"


def layers_of(*items: str) -> list[int]:
    return [item.layer for item in parse_layered(listing(frame(*items)))[0].items]


def ordered_and_set(*items: str) -> tuple[str, str]:
    words = digest_text(listing(frame(*items))).splitlines()[2].split(" ")
    return words[3], words[4]


A = fill("screen", 0, 0, 10, 10, 1)
B = fill("screen", 5, 5, 15, 15, 2)
C = fill("screen", 100, 100, 110, 110, 3)
D = fill("screen", 200, 200, 210, 210, 4)


class LayerTest(unittest.TestCase):
    def test_overlapping_items_stack(self):
        self.assertEqual(layers_of(A, B), [0, 1])

    def test_items_that_do_not_overlap_share_layer_0(self):
        self.assertEqual(layers_of(A, C, D), [0, 0, 0])

    def test_touching_rectangles_do_not_overlap(self):
        self.assertEqual(layers_of(fill("screen", 0, 0, 10, 10), fill("screen", 10, 0, 20, 10)), [0, 0])

    def test_the_layer_is_one_more_than_the_highest_dependency(self):
        self.assertEqual(layers_of(A, B, C, fill("screen", 8, 8, 102, 102)), [0, 1, 0, 2])

    def test_different_targets_are_independent(self):
        self.assertEqual(layers_of(A, fill("map", 0, 0, 10, 10)), [0, 0])

    def test_an_empty_item_takes_no_layer(self):
        self.assertEqual(layers_of(A, "text screen 0 0 0 0 comix 1 2 3 4 5 hello", B), [0, 1])

    def test_a_blit_reads_its_source_target(self):
        self.assertEqual(layers_of(fill("map", 0, 0, 10, 10), "blit screen 0 0 640 480 map 0 0"), [0, 1])

    def test_a_blit_comes_before_a_later_draw_on_its_source(self):
        self.assertEqual(layers_of("blit screen 0 0 640 480 map 0 0", fill("map", 0, 0, 10, 10)), [0, 1])

    def test_two_reads_of_one_target_commute(self):
        self.assertEqual(layers_of(fill("map", 0, 0, 10, 10), "copy screen 0 0 4 4 map 0 0 3 3 0 0", "blit other 100 100 200 200 map 5 5"), [0, 1, 1])

    def test_a_sprite_of_a_target_reads_it(self):
        self.assertEqual(layers_of(fill("map", 0, 0, 10, 10), "sprite screen 50 50 60 60 target:map 50 50"), [0, 1])

    def test_a_draw_with_an_empty_area_does_not_order_a_read(self):
        self.assertEqual(layers_of("fill map 0 0 0 0 1", "blit screen 0 0 640 480 map 0 0"), [0])

    def test_a_read_with_an_empty_area_does_not_order_a_draw(self):
        self.assertEqual(layers_of("blit screen 0 0 0 0 map 0 0", fill("map", 0, 0, 10, 10)), [0])

    def test_a_picture_that_is_a_file_reads_no_target(self):
        self.assertEqual(layers_of(fill("map", 0, 0, 10, 10), "sprite screen 0 0 10 10 anims/logos.gaf:logo:0 0 0"), [0, 0])

    def test_a_text_whose_font_looks_like_a_target_reads_none(self):
        self.assertEqual(layers_of(fill("map", 0, 0, 10, 10), "text screen 0 0 10 10 target:map 1 2 3 4 5 x"), [0, 0])

    def test_big_areas_and_negative_positions(self):
        self.assertEqual(layers_of("clear screen 0 0 640 480 0", fill("screen", -20, -20, 4, 4), "model screen 0 0 640 480 armcom"), [0, 1, 2])

    def test_the_grid_agrees_with_a_plain_scan(self):
        rng = random.Random(7)
        lines = []
        for _ in range(400):
            x, y = rng.randrange(-30, 700), rng.randrange(-30, 500)
            w, h = rng.choice((1, 3, 20, 90, 700)), rng.choice((1, 3, 20, 90, 500))
            lines.append(fill(rng.choice(("screen", "map")), x, y, x + w, y + h))
        got = layers_of(*lines)
        rects = [tuple(map(int, line.split(" ")[2:6])) + (line.split(" ")[1],) for line in lines]
        want: list[int] = []
        for i, r in enumerate(rects):
            deps = [want[j] for j in range(i) if rects[j][4] == r[4] and displaylog.overlaps(rects[j][:4], r[:4])]
            want.append(max(deps) + 1 if deps else 0)
        self.assertEqual(got, want)

    def test_a_frame_of_20000_items(self):
        rng = random.Random(1)
        lines = [fill("screen", x, y, x + 8, y + 8) for x, y in ((rng.randrange(632), rng.randrange(472)) for _ in range(20000))]
        self.assertEqual(len(layers_of(*lines)), 20000)


class DigestTest(unittest.TestCase):
    def test_a_tiny_frame_has_the_hand_computed_hashes(self):
        items = [A, C]
        ordered = displaylog.hash_bytes(("".join(line + "\n" for line in [f"frame 1 0 1 {PALETTE}", *items, CURSOR])).encode())
        sets = displaylog.hash_bytes(("".join(line + "\n" for line in [f"frame 1 0 1 {PALETTE}", "0 " + A, "0 " + C, CURSOR])).encode())
        self.assertEqual(digest_text(listing(frame(*items))), f"bt-display-digest 1\nlog bt-display 1\n1 0 2 {ordered:016x} {sets:016x}\n")

    def test_the_hash_is_the_word_then_byte_fnv(self):
        h = displaylog.OFFSET_BASIS
        data = b"0123456789"
        h = ((h ^ int.from_bytes(data[:8], "little")) * displaylog.PRIME) & displaylog.MASK64
        for byte in data[8:]:
            h = ((h ^ byte) * displaylog.PRIME) & displaylog.MASK64
        self.assertEqual(displaylog.hash_bytes(data), h)
        self.assertEqual(displaylog.hash_bytes(b""), displaylog.OFFSET_BASIS)

    def test_set_sorts_by_bytes_with_the_shorter_first(self):
        self.assertEqual(sorted(["0 a", "0 ab", "0 a "]), ["0 a", "0 a ", "0 ab"])

    def test_one_line_for_each_frame(self):
        text = digest_text(listing(frame(A, number=1, tick=0), frame(B, number=2, tick=4, steps=4)))
        self.assertEqual([line.split(" ")[:3] for line in text.splitlines()[2:]], [["1", "0", "1"], ["2", "4", "1"]])

    def test_two_overlapping_items_swapped_change_set_and_ordered(self):
        self.assertNotEqual(ordered_and_set(A, B)[0], ordered_and_set(B, A)[0])
        self.assertNotEqual(ordered_and_set(A, B)[1], ordered_and_set(B, A)[1])

    def test_two_items_apart_swapped_keep_set_and_change_ordered(self):
        one, two = ordered_and_set(A, C), ordered_and_set(C, A)
        self.assertNotEqual(one[0], two[0])
        self.assertEqual(one[1], two[1])

    def test_a_blit_that_reads_a_target_swapped_with_a_draw_on_it_differs(self):
        draw, blit = fill("map", 300, 300, 310, 310), "blit screen 0 0 10 10 map 0 0"
        self.assertNotEqual(ordered_and_set(draw, blit)[1], ordered_and_set(blit, draw)[1])

    def test_a_list_with_empty_items_has_the_digest_of_the_list_without(self):
        empty_one, empty_two = "text screen 0 0 0 0 comix 1 2 3 4 5 a", "sprite_blend screen 0 0 0 0 anims/fx.gaf:smoke:0 -40 -40"
        self.assertEqual(digest_text(listing(frame(A, empty_one, B, empty_two))), digest_text(listing(frame(A, B))))
        self.assertEqual(digest_text(listing(frame(A, empty_one, B, empty_two))).splitlines()[2].split(" ")[2], "2")

    def test_a_different_count_changes_both(self):
        one, two = ordered_and_set(A), ordered_and_set(A, C)
        self.assertNotEqual(one[0], two[0])
        self.assertNotEqual(one[1], two[1])

    def test_the_cursor_is_in_both(self):
        one = digest_text(listing(frame(A, cursor="cursor a.gaf:x:0 1 2")))
        two = digest_text(listing(frame(A, cursor="cursor a.gaf:x:0 1 3")))
        self.assertNotEqual(one.splitlines()[2].split(" ")[3:], two.splitlines()[2].split(" ")[3:])
        self.assertNotEqual(one.splitlines()[2].split(" ")[4], two.splitlines()[2].split(" ")[4])


class DigestCompareTest(unittest.TestCase):
    def digests(self, *item_lists: list[str]) -> list[str]:
        return [digest_text(listing(frame(*items))) for items in item_lists]

    def test_equal_digests(self):
        one, two = self.digests([A, C], [A, C])
        self.assertIsNone(displaylog.digest_difference(one, two))

    def test_names_the_line_frame_and_tick(self):
        one = digest_text(listing(frame(A), frame(B, number=2, tick=3)))
        two = digest_text(listing(frame(A), frame(B, C, number=2, tick=3)))
        diff = displaylog.digest_difference(one, two)
        self.assertIn("line 4 (frame 2, tick 3)", diff)
        self.assertIn("stored `2 3 1 ", diff)
        self.assertEqual(displaylog.digest_frame(one, two), (2, 3))

    def test_end_of_tick_ignores_the_order_that_does_not_count(self):
        one, two = self.digests([A, C], [C, A])
        self.assertIsNotNone(displaylog.digest_difference(one, two))
        self.assertIsNone(displaylog.digest_difference(one, two, end_of_tick=True))

    def test_end_of_tick_still_sees_the_order_that_counts(self):
        one, two = self.digests([A, B], [B, A])
        self.assertIn("line 3", displaylog.digest_difference(one, two, end_of_tick=True))

    def test_a_frame_only_one_digest_has(self):
        one = digest_text(listing(frame(A), frame(B, number=2, tick=3)))
        two = digest_text(listing(frame(A)))
        self.assertIn("only the stored digest has it", displaylog.digest_difference(one, two))
        self.assertIn("only the got digest has it", displaylog.digest_difference(two, one))

    def test_the_header_lines_are_compared(self):
        one = digest_text(listing(frame(A)))
        self.assertIn("line 1", displaylog.digest_difference(one, one.replace("digest 1", "digest 2")))


class ListCompareTest(unittest.TestCase):
    def test_equal_lists(self):
        self.assertIsNone(list_difference(listing(frame(A, B)), listing(frame(A, B))))

    def test_an_empty_item_only_one_list_has_is_no_difference(self):
        self.assertIsNone(list_difference(listing(frame(A, "fill screen 0 0 0 0 1", B)), listing(frame(A, B))))

    def test_names_the_frame_and_the_item_with_both_lines(self):
        one = listing(frame(A, number=1), frame(B, C, number=2, tick=5))
        two = listing(frame(A, number=1), frame(B, D, number=2, tick=5))
        diff = list_difference(one, two)
        self.assertIn("frame 2 (tick 5)", diff)
        self.assertIn(f"line 7 of A `{C}`, line 7 of B `{D}`", diff)

    def test_an_item_only_one_list_has(self):
        diff = list_difference(listing(frame(A, B)), listing(frame(A)))
        self.assertIn("only A has the item at line 4", diff)

    def test_the_frame_line_and_the_cursor_are_compared(self):
        self.assertIn("frame line", list_difference(listing(frame(A, steps=1)), listing(frame(A, steps=2))))
        self.assertIn("cursor line", list_difference(listing(frame(A)), listing(frame(A, cursor="cursor a.gaf:x:0 1 2"))))

    def test_a_frame_only_one_list_has(self):
        self.assertEqual(list_difference(listing(frame(A), frame(A, number=2, tick=4)), listing(frame(A))), "frame 2 (tick 4): only A has it")

    def test_end_of_tick_ignores_swaps_that_do_not_count(self):
        one, two = listing(frame(A, C, D)), listing(frame(D, A, C))
        self.assertIsNotNone(list_difference(one, two))
        self.assertIsNone(list_difference(one, two, end_of_tick=True))

    def test_end_of_tick_names_the_first_sorted_item_that_differs(self):
        diff = list_difference(listing(frame(A, B)), listing(frame(B, A)), end_of_tick=True)
        self.assertIn("frame 1 (tick 0)", diff)
        self.assertIn(f"line 3 of A `0 {A}`, line 3 of B `0 {B}`", diff)

    def test_the_first_frame_that_differs_is_named(self):
        one = listing(frame(A, number=1), frame(A, number=2, tick=2), frame(A, number=3, tick=4))
        two = listing(frame(A, number=1), frame(B, number=2, tick=2), frame(D, number=3, tick=4))
        self.assertTrue(list_difference(one, two).startswith("frame 2 (tick 2)"))


class ParseTest(unittest.TestCase):
    def test_the_first_line_is_the_version(self):
        with self.assertRaises(Error):
            parse_layered("bt-display 2\n" + frame(A))

    def test_a_frame_needs_its_cursor(self):
        with self.assertRaises(Error):
            parse_layered("bt-display 1\n" + frame(A).rsplit("\n", 2)[0] + "\n")

    def test_an_item_needs_an_area(self):
        with self.assertRaises(Error):
            parse_layered(listing(frame("fill screen 0 0")))
        with self.assertRaises(Error):
            parse_layered(listing(frame("fill screen a b c d 1")))

    def test_a_line_outside_a_frame(self):
        with self.assertRaises(Error):
            parse_layered("bt-display 1\n" + A + "\n")

    def test_a_blit_needs_a_source(self):
        with self.assertRaises(Error):
            parse_layered(listing(frame("blit screen 0 0 10 10")))

    def test_an_empty_item_is_left_out(self):
        for empty in ("fill screen 0 0 0 0 1", "fill screen 5 0 5 10 1", "fill screen 0 5 10 5 1", "fill screen 10 10 5 5 1"):
            items = parse_layered(listing(frame(A, empty, B)))[0].items
            self.assertEqual([item.line for item in items], [A, B], empty)

    def test_an_item_with_an_area_is_kept(self):
        one_pixel = "pixel screen 3 4 4 5 3 4 7"
        items = parse_layered(listing(frame(one_pixel, "text screen 0 0 0 0 comix 1 2 3 4 5 a", C)))[0].items
        self.assertEqual([(item.line, item.number) for item in items], [(one_pixel, 3), (C, 5)])

    def test_the_fields_of_an_item(self):
        item = parse_layered(listing(frame("copy screen 1 2 3 4 map 0 0 5 5 8 9")))[0].items[0]
        self.assertEqual((item.kind, item.target, item.area, item.reads, item.number), ("copy", "screen", (1, 2, 3, 4), "map", 3))


class CommandLineTest(unittest.TestCase):
    def run_main(self, *argv: str) -> tuple[int, str]:
        import contextlib
        import io
        import sys
        out = io.StringIO()
        with mock_argv(("displaylog.py", *argv)), contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            try:
                code = displaylog.main()
            except SystemExit as e:
                code = int(e.code or 0)
        return code, out.getvalue()

    def test_digest_compare_and_against(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            one, two = root / "a.txt", root / "b.txt"
            one.write_text(listing(frame(A, C)))
            two.write_text(listing(frame(C, A)))
            self.assertEqual(self.run_main("digest", str(one)), (0, digest_text(one.read_text())))
            stored = root / "stored.txt"
            stored.write_text(digest_text(one.read_text()))
            self.assertEqual(self.run_main("digest", str(one), "--against", str(stored))[0], 0)
            self.assertEqual(self.run_main("digest", str(two), "--against", str(stored))[0], 1)
            self.assertEqual(self.run_main("digest", str(two), "--against", str(stored), "--end-of-tick")[0], 0)
            self.assertEqual(self.run_main("compare", str(one), str(two))[0], 1)
            self.assertEqual(self.run_main("compare", str(one), str(two), "--end-of-tick")[0], 0)
            other = root / "other.txt"
            other.write_text(digest_text(two.read_text()))
            self.assertEqual(self.run_main("against-digests", str(stored), str(other))[0], 1)
            self.assertEqual(self.run_main("against-digests", str(stored), str(other), "--end-of-tick")[0], 0)

    def test_an_unreadable_file_is_exit_2(self):
        with tempfile.TemporaryDirectory() as tmp:
            bad = Path(tmp) / "bad.txt"
            bad.write_text("nonsense\n")
            self.assertEqual(self.run_main("digest", str(bad))[0], 2)
            self.assertEqual(self.run_main("digest", str(Path(tmp) / "missing.txt"))[0], 2)


def mock_argv(argv: tuple[str, ...]):
    import contextlib
    import sys

    @contextlib.contextmanager
    def swap():
        saved = sys.argv
        sys.argv = list(argv)
        try:
            yield
        finally:
            sys.argv = saved

    return swap()


if __name__ == "__main__":
    unittest.main()
