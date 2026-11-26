"""Tests for tracedigest.py: the per-tick digest and state of a trace.

    uv run tools/test_tracedigest.py
"""

import tempfile
import unittest
from pathlib import Path

import tracedigest as tg
from tracediff import Error
from traceschema import TRACE_FORMAT

ROWS = [(1, "units", "00000000000000aa"), (1, "tick", "00000000000000a1"), (1, "view", "00000000000000a2"),
        (2, "units", "00000000000000bb"), (2, "tick", "00000000000000b1"), (2, "view", "00000000000000b2")]


class DigestTest(unittest.TestCase):
    def test_a_tick_gives_its_digest_its_tick_row_and_its_view_row(self):
        got = tg.digests(ROWS)
        self.assertEqual([(t, s, v) for t, _, s, v in got],
                         [(1, "00000000000000a1", "00000000000000a2"), (2, "00000000000000b1", "00000000000000b2")])
        want = tg.fnv1a64(bytes.fromhex("aa00000000000000a100000000000000a200000000000000"))
        self.assertEqual(got[0][1], f"{want:016x}")

    def test_the_digest_covers_the_tick_row(self):
        other = [ROWS[0], (1, "tick", "00000000000000a3")] + ROWS[2:]
        self.assertNotEqual(tg.digests(ROWS)[0][1], tg.digests(other)[0][1])

    def test_the_digest_covers_the_view_row(self):
        other = ROWS[:2] + [(1, "view", "00000000000000a3")] + ROWS[3:]
        self.assertNotEqual(tg.digests(ROWS)[0][1], tg.digests(other)[0][1])
        self.assertNotEqual(tg.digests(ROWS)[0][3], tg.digests(other)[0][3])

    def test_a_tick_without_a_tick_row_is_named(self):
        with self.assertRaisesRegex(Error, "tick 2"):
            tg.digests(ROWS[:4])

    def test_a_tick_without_a_view_row_is_named(self):
        with self.assertRaisesRegex(Error, "tick 2 has no view row"):
            tg.digests(ROWS[:5])

    def test_tail_returns_finished_ticks_only(self):
        tail = tg.TraceTail()
        text = f"bt-trace {TRACE_FORMAT}\nseed 1\n".encode() + b"".join(f"{t} {p} {h}\n".encode() for t, p, h in ROWS[:4])
        self.assertEqual(tail.feed(text), [tg.digests(ROWS[:3])[0]])
        self.assertEqual(tail.feed(b"2 tick 00000000000000b1\n2 view 00000000000000b2\n"), [])
        self.assertEqual(tail.hasher.finish(), tg.digests(ROWS)[1])

    def test_render_and_parse_round_trip(self):
        header = {k: "1" for k in tg.HEADER_KEYS}
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "digest.txt"
            path.write_text(tg.render(header, tg.digests(ROWS)))
            parsed = tg.parse_digest(path)
        self.assertEqual(parsed[1], tg.digests(ROWS))
        self.assertEqual({k: v for k, v in parsed[0].items() if k != "trace_format"}, header)

    def test_parse_digest_refuses_three_columns(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "digest.txt"
            path.write_text(f"bt-digest {TRACE_FORMAT}\ncommit x\n1 0123456789abcdef 0123456789abcdef\n")
            with self.assertRaises(Error):
                tg.parse_digest(path)


if __name__ == "__main__":
    unittest.main()
