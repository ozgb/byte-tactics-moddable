"""Tests for tools/eventdigest.py and the order classes of docs/sim-events.md.

    uv run tools/test_eventdigest.py
"""

import re
import struct
import unittest

import eventdigest
from eventdigest import Error, digest_text, parse_classes, tick_lines

PAGE = eventdigest.PAGE.read_text()
HEADER = "bt-events 1\n"


def hb(data: bytes) -> int:
    return eventdigest.hash_bytes(data)


def tick_line_of(log: str, tick: str) -> str:
    return next(line for line in tick_lines(digest_text(log)) if line.startswith(tick + " tick "))


def log_of(*lines: str) -> str:
    return HEADER + "".join(line + "\n" for line in lines)


SOUND = "5 units sound 0x0 0 0"
SOUND_AT = "5 units sound_at 0x0 1 2 3 4 5"
SMOKE = "5 units smoke 0x0 1 2 3 15 900 9"
CD = "5 units cd_activity 0x0 1 0 1"
FLASH = "5 players score_flash 0x0 0 1"
RADAR = "5 players radar_units 0x0 0800000000000000"


class PageTest(unittest.TestCase):
    def test_every_kind_has_a_value_and_an_order_line(self):
        headings = re.findall(r"^### SIM_EVENT_([A-Z0-9_]+)$", PAGE, re.M)
        classes = parse_classes(PAGE)
        self.assertEqual(sorted(classes.values), sorted(h.lower() for h in headings))
        self.assertEqual(sorted(classes.specs), sorted(classes.values))
        self.assertEqual(len(headings), 35)

    def test_every_name_on_an_order_line_is_a_kind_of_the_page(self):
        classes = parse_classes(PAGE)
        for kind, (form, named) in classes.specs.items():
            for other in named:
                self.assertIn(other, classes.values, f"the order line of {kind} names {other}")

    def test_the_lines_agree_with_each_other(self):
        classes = parse_classes(PAGE)
        for kind, (form, named) in classes.specs.items():
            for other in named:
                other_form, other_named = classes.specs[other]
                self.assertTrue(other_form == "every" or kind in other_named, f"{kind} names {other}, which does not name it back")

    def test_a_kind_in_a_pair_names_itself(self):
        classes = parse_classes(PAGE)
        for kind, (form, named) in classes.specs.items():
            if form == "list":
                self.assertIn(kind, named, f"{kind} is ordered against others and not against itself")

    def test_the_pairs_are_symmetric_and_ordered(self):
        classes = parse_classes(PAGE)
        for a, b in classes.pairs:
            self.assertLessEqual(a, b)
            self.assertIn(b, classes.partners[a])
            self.assertIn(a, classes.partners[b])
        self.assertEqual(classes.pairs, sorted(set(classes.pairs)))
        self.assertEqual(eventdigest.order_pairs(), classes.pairs)

    def test_known_pairs(self):
        v = parse_classes(PAGE).values
        pairs = set(eventdigest.order_pairs())
        self.assertIn((v["sound"], v["sound_at"]), pairs)
        self.assertIn((v["sound"], v["sound"]), pairs)
        self.assertIn((v["screen_shake"], v["screen_shake"]), pairs)
        self.assertIn((v["radar_units"], v["radar_mapped"]), pairs)
        self.assertIn((v["thrust_particles"], v["bubbles"]), pairs)
        self.assertNotIn((v["smoke"], v["radar_units"]), pairs)
        self.assertNotIn((v["sound"], v["smoke"]), pairs)
        self.assertEqual([p for p in pairs if v["cd_activity"] in p or v["score_flash"] in p], [])
        self.assertIn((v["unit_speech"], v["remove_unit_speech"]), pairs)

    def test_a_kind_without_an_order_line_is_refused(self):
        lines = PAGE.split("\n")
        at = next(i for i, line in enumerate(lines) if line.startswith("Order: `thrust_particles`"))
        with self.assertRaisesRegex(Error, "`thrust_particles` has no order line"):
            parse_classes("\n".join(lines[:at] + lines[at + 1:]))

    def test_a_name_that_is_not_a_kind_is_refused(self):
        with self.assertRaisesRegex(Error, "names `no_such_kind`"):
            parse_classes(PAGE.replace("Order: `thrust_particles`, `bubbles`:", "Order: `thrust_particles`, `no_such_kind`:"))

    def test_a_malformed_order_line_is_refused(self):
        with self.assertRaisesRegex(Error, "cannot read the order line"):
            parse_classes(PAGE.replace("Order: same: the shake", "Order: sometimes: the shake", 1))

    def test_every_pairs_with_all_kinds(self):
        page = PAGE.replace("Order: none: it only adds an amount", "Order: every: it only adds an amount", 1)
        classes = parse_classes(page)
        mine = classes.values["cd_activity"]
        self.assertEqual(classes.partners[mine], set(classes.values.values()))

    def test_reasons_name_no_source_identifiers(self):
        for line in re.findall(r"^Order: .*$", PAGE, re.M):
            reason = line.split(": ", 2)[-1]
            self.assertIsNone(re.search(r"[a-z]+_[a-z_]+|\.cpp|\.h\b|src/", re.sub(r"`[a-z_0-9]+`", "", reason)), line)


class FormatTest(unittest.TestCase):
    LOG = log_of("0 load smoke 0x0 1 2 3 15 900 9", SOUND, "5 units sound 0x0 1 0", RADAR, "6 meteors message 0x0 0 1")

    def test_the_header_is_format_2(self):
        self.assertEqual(digest_text(self.LOG).splitlines()[:2], ["bt-event-digest 2", "log bt-events 1"])

    def test_the_lines_of_format_1_are_unchanged(self):
        lines = digest_text(self.LOG).splitlines()[2:]
        runs = [line for line in lines if " tick " not in line]
        expected = []
        for tick, phase, body in (("0", "load", ["0 load smoke 0x0 1 2 3 15 900 9"]), ("5", "units", [SOUND, "5 units sound 0x0 1 0"]),
                                  ("5", "players", [RADAR]), ("6", "meteors", ["6 meteors message 0x0 0 1"])):
            expected.append(f"{tick} {phase} {len(body)} {hb(''.join(b + chr(10) for b in body).encode()):016x}")
        self.assertEqual(runs, expected)

    def test_a_tick_line_follows_the_last_line_of_its_tick(self):
        lines = digest_text(self.LOG).splitlines()[2:]
        self.assertEqual([line.split(" ")[:2] for line in lines],
                         [["0", "load"], ["0", "tick"], ["5", "units"], ["5", "players"], ["5", "tick"], ["6", "meteors"], ["6", "tick"]])
        self.assertTrue(all(re.fullmatch(r"\d+ tick \d+ [0-9a-f]{16} [0-9a-f]{16}", line) for line in tick_lines(digest_text(self.LOG))))

    def test_set_is_the_hash_of_the_records_sorted_without_their_phase(self):
        raws = sorted([b"5 sound 0x0 0 0\n", b"5 sound 0x0 1 0\n", b"5 radar_units 0x0 0800000000000000\n"])
        line = tick_line_of(self.LOG, "5")
        self.assertEqual(line.split(" ")[2], "3")
        self.assertEqual(line.split(" ")[4], f"{hb(b''.join(raws)):016x}")

    def test_ordered_is_the_hash_of_the_projections_of_the_pairs_that_count(self):
        v = parse_classes(PAGE).values
        log = log_of(SOUND, SOUND_AT, "5 players sound 0x0 1 0", FLASH)
        by_kind = [(v["sound"], b"5 sound 0x0 0 0\n"), (v["sound_at"], b"5 sound_at 0x0 1 2 3 4 5\n"), (v["sound"], b"5 sound 0x0 1 0\n"),
                   (v["score_flash"], b"5 score_flash 0x0 0 1\n")]
        body = b""
        for x, y in eventdigest.order_pairs():
            chosen = b"".join(raw for kind, raw in by_kind if kind in (x, y))
            if chosen:
                body += struct.pack("<HHQ", x, y, hb(chosen))
        self.assertIn(struct.pack("<HHQ", v["sound"], v["sound_at"], hb(by_kind[0][1] + by_kind[1][1] + by_kind[2][1])), [body[i:i + 12] for i in range(0, len(body), 12)])
        self.assertEqual(tick_line_of(log, "5").split(" ")[3], f"{hb(body):016x}")

    def test_a_tick_of_kinds_that_never_order_has_the_hash_of_nothing(self):
        line = tick_line_of(log_of(CD, FLASH), "5")
        self.assertEqual(line.split(" ")[3], f"{hb(b''):016x}")

    def test_a_dropped_line_counts_in_count_and_set_and_in_no_projection(self):
        base = tick_line_of(log_of(SOUND, SOUND_AT), "5").split(" ")
        got = tick_line_of(log_of(SOUND, "5 players dropped 3", SOUND_AT), "5").split(" ")
        self.assertEqual(got[2], "3")
        self.assertEqual(got[3], base[3])
        self.assertNotEqual(got[4], base[4])

    def test_records_of_a_tick_must_be_contiguous(self):
        with self.assertRaisesRegex(Error, "line 4: the records of tick 5 are not contiguous"):
            digest_text(log_of(SOUND, "6 units sound 0x0 0 0", SOUND))

    def test_load_and_outside_records_belong_to_the_tick_they_carry(self):
        log = log_of("5 load sound 0x0 0 0", "5 outside sound 0x0 1 0", "5 units sound 0x0 2 0")
        self.assertEqual(tick_line_of(log, "5").split(" ")[2], "3")

    def test_a_log_without_a_header_or_a_phase_or_a_known_kind_is_refused(self):
        with self.assertRaises(Error):
            digest_text("0 load smoke 0x0\n")
        with self.assertRaises(Error):
            digest_text(HEADER + "broken\n")
        with self.assertRaisesRegex(Error, "kind that sim-events.md lacks"):
            digest_text(HEADER + "5 units kind99 0x0\n")

    def test_the_end_of_tick_difference_looks_at_tick_lines_only(self):
        one = digest_text(log_of(SOUND, SMOKE, CD, FLASH))
        two = digest_text(log_of(SMOKE, SOUND, FLASH, CD).replace("5 units", "5 players"))
        self.assertIsNotNone(eventdigest.digest_difference(one, two))
        self.assertIsNone(eventdigest.digest_difference(one, two, end_of_tick=True))
        three = digest_text(log_of(SOUND, SMOKE, CD))
        self.assertRegex(eventdigest.digest_difference(one, three, end_of_tick=True), r"^events digest line \d+ \(tick 5\): stored `5 tick 4 ")
        longer = digest_text(log_of(SOUND, SMOKE, CD, FLASH, "6 units sound 0x0 0 0"))
        self.assertIn("only the got digest has it", eventdigest.digest_difference(one, longer, end_of_tick=True))

    def test_a_longer_digest_differs_at_the_extra_tick_line(self):
        one = digest_text(log_of(SOUND))
        two = digest_text(log_of(SOUND, "6 units sound 0x0 0 0"))
        self.assertIn("only the got digest has it (`6 tick 1 ", eventdigest.digest_difference(one, two, end_of_tick=True))


class NegativeTest(unittest.TestCase):
    def test_a_swap_of_kinds_whose_order_does_not_count_keeps_the_tick_line(self):
        base = log_of(SOUND, SMOKE, CD, FLASH, SOUND_AT)
        self.assertEqual(tick_line_of(log_of(SOUND, CD, SMOKE, SOUND_AT, FLASH), "5"), tick_line_of(base, "5"))
        self.assertEqual(tick_line_of(log_of(SMOKE, SOUND, CD, FLASH, SOUND_AT), "5"), tick_line_of(base, "5"))
        self.assertEqual(tick_line_of(log_of(SOUND, SMOKE, FLASH, CD, SOUND_AT), "5"), tick_line_of(base, "5"))

    def test_a_swap_of_kinds_whose_order_counts_changes_ordered_and_keeps_set(self):
        one = tick_line_of(log_of(SOUND, SOUND_AT, SMOKE), "5").split(" ")
        two = tick_line_of(log_of(SOUND_AT, SOUND, SMOKE), "5").split(" ")
        self.assertEqual(one[:3], two[:3])
        self.assertNotEqual(one[3], two[3])
        self.assertEqual(one[4], two[4])

    def test_a_swap_of_two_records_of_one_kind_that_orders_against_itself_changes_ordered(self):
        a, b = "5 units sound 0x0 0 0", "5 units sound 0x0 1 0"
        one, two = tick_line_of(log_of(a, b), "5").split(" "), tick_line_of(log_of(b, a), "5").split(" ")
        self.assertNotEqual(one[3], two[3])
        self.assertEqual(one[4], two[4])

    def test_a_swap_of_two_records_of_one_kind_that_commutes_keeps_the_tick_line(self):
        a, b = "5 units cd_activity 0x0 1 0 1", "5 units cd_activity 0x0 5 1 1"
        self.assertEqual(tick_line_of(log_of(a, b), "5"), tick_line_of(log_of(b, a), "5"))

    def test_a_record_moved_to_another_phase_of_its_tick_keeps_the_tick_line(self):
        base = log_of(SOUND, SMOKE, SOUND_AT)
        moved = log_of(SOUND, SMOKE.replace("5 units", "5 projectiles"), SOUND_AT.replace("5 units", "5 sharing"))
        self.assertEqual(tick_line_of(moved, "5"), tick_line_of(base, "5"))
        self.assertNotEqual(digest_text(moved).splitlines()[2:4], digest_text(base).splitlines()[2:4])

    def test_a_record_moved_to_another_tick_changes_both_tick_lines(self):
        base = log_of(SOUND, SOUND_AT, "6 units sound 0x0 7 0")
        moved = log_of(SOUND, "6 units sound_at 0x0 1 2 3 4 5", "6 units sound 0x0 7 0")
        self.assertNotEqual(tick_line_of(moved, "5"), tick_line_of(base, "5"))
        self.assertNotEqual(tick_line_of(moved, "6"), tick_line_of(base, "6"))

    def test_a_radar_snapshot_and_a_smoke_plume_swapped_keep_the_tick_line(self):
        self.assertEqual(tick_line_of(log_of(RADAR, SMOKE), "5"), tick_line_of(log_of(SMOKE, RADAR), "5"))
        self.assertEqual(tick_line_of(log_of(SOUND, RADAR, SMOKE, SOUND_AT), "5"), tick_line_of(log_of(SOUND, SMOKE, RADAR, SOUND_AT), "5"))

    def test_a_changed_record_changes_set(self):
        self.assertNotEqual(tick_line_of(log_of(SOUND), "5"), tick_line_of(log_of(SOUND.replace("0 0", "1 0")), "5"))

    def test_a_missing_record_changes_the_count(self):
        self.assertNotEqual(tick_line_of(log_of(SOUND, CD), "5"), tick_line_of(log_of(SOUND), "5"))


if __name__ == "__main__":
    unittest.main()
