"""Tests for tracedigest.py: the per-tick digest and state of a trace.

    uv run tools/test_tracedigest.py
"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path

import test_matchsetup
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


class MatchSetupTest(unittest.TestCase):
    def test_the_stored_traces_it_compares_have_ticks(self):
        for name in ("console-units", "orders-move-attack"):
            self.assertTrue(test_matchsetup.stored_of(name), name)

    def test_a_run_trace_digests_to_its_ticks(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "trace.txt"
            path.write_text(f"bt-trace {TRACE_FORMAT}\ngame_seed 1\n" + "".join(f"{t} {p} {h}\n" for t, p, h in ROWS))
            self.assertEqual(test_matchsetup.digest_of(path), tg.digests(ROWS))
            self.assertTrue(test_matchsetup.digest_of(path))

    def test_a_trace_check_fails_on_an_empty_trace(self):
        test_matchsetup.check.failed = False
        with contextlib.redirect_stdout(io.StringIO()) as out:
            test_matchsetup.check([] == [], "empty", ([], []))
        self.assertIn("FAIL empty (no ticks", out.getvalue())
        self.assertTrue(test_matchsetup.check.failed)
        test_matchsetup.check.failed = False


if __name__ == "__main__":
    unittest.main()
