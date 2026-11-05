"""Tests for depclass.py, on synthetic summaries and coupling rows.

    uv run tools/test_depclass.py
"""

import gzip
import tempfile
import unittest
from pathlib import Path

import depclass as dc

HEADER = "writer,reader,item,phase,evidence,count,first_tick\n"


def summary(*rows: str) -> list[dict]:
    """The rows of an edges file from lines `writer,reader,item,phase,evidence,first_tick`."""
    lines = []
    for r in rows:
        w, rd, item, phase, ev, tick = r.split(",")
        lines.append('%s,%s,"%s",%s,%s,1,%s\n' % (w, rd, item, phase, ev, tick))
    return dc.read_summary(HEADER + "".join(lines))


def coupling(*edges: tuple[str, str, str, str]) -> list[dict]:
    """Coupling rows from (from, to, kind, target)."""
    return [{"from": a, "to": b, "kind": k, "target": t, "count": "1"} for a, b, k, t in edges]


def by_edge(rows: list[dict]) -> dict[tuple, dict]:
    """The rows keyed by (writer, reader, item, phase)."""
    return {(r["writer"], r["reader"], r["item"], r["phase"]): r for r in rows}


class Classify(unittest.TestCase):
    def test_immediate(self):
        rows = dc.classify({"a": summary("units,weapons,Unit::x,units,read-after-write,5")}, [])
        r = by_edge(rows)[("units", "weapons", "Unit::x", "units")]
        self.assertEqual((r["class"], r["runs"], r["example"]), ("immediate", 1, "a@5"))

    def test_ordered_by_evidence_and_by_item(self):
        rows = by_edge(dc.classify({"a": summary("ai,units,Unit::x,units,ordered,3",
                                                 "ai,units,rng.parkMiller,units,read-after-write,4")}, []))
        self.assertEqual(rows[("ai", "units", "Unit::x", "units")]["class"], "ordered")
        self.assertEqual(rows[("ai", "units", "rng.parkMiller", "units")]["class"], "ordered")

    def test_deferrable_needs_both_sides_in_one_run(self):
        both = summary("units,-,Unit::x,units,write-unread,7", "-,weapons,Unit::x,units,read-unwritten,9")
        r = by_edge(dc.classify({"a": both}, []))[("units", "weapons", "Unit::x", "units")]
        self.assertEqual((r["class"], r["example"]), ("deferrable", "a@9"))
        split = {"a": summary("units,-,Unit::x,units,write-unread,7"), "b": summary("-,weapons,Unit::x,units,read-unwritten,9")}
        self.assertNotIn(("units", "weapons", "Unit::x", "units"), by_edge(dc.classify(split, [])))

    def test_precedence_immediate_over_deferrable(self):
        a = summary("units,-,Unit::x,units,write-unread,7", "-,weapons,Unit::x,units,read-unwritten,9")
        b = summary("units,weapons,Unit::x,units,read-after-write,3")
        r = by_edge(dc.classify({"a": a, "b": b}, []))[("units", "weapons", "Unit::x", "units")]
        self.assertEqual((r["class"], r["runs"], r["example"]), ("immediate", 1, "b@3"))

    def test_read_only_needs_no_write_in_the_phase(self):
        a = summary("-,weapons,Unit::x,units,read-unwritten,2", "-,weapons,Unit::y,units,read-unwritten,2",
                    "game,-,Unit::y,units,write-unread,1")
        rows = by_edge(dc.classify({"a": a}, []))
        self.assertEqual(rows[("-", "weapons", "Unit::x", "units")]["class"], "read-only")
        self.assertNotIn(("-", "weapons", "Unit::y", "units"), rows)

    def test_example_is_the_smallest_tick_then_the_first_name(self):
        row = "units,weapons,Unit::x,units,read-after-write,"
        rows = dc.classify({"b": summary(row + "4"), "a": summary(row + "4"), "c": summary(row + "9")}, [])
        r = by_edge(rows)[("units", "weapons", "Unit::x", "units")]
        self.assertEqual((r["runs"], r["example"]), (3, "a@4"))

    def test_not_an_edge(self):
        a = summary("units,units,Unit::x,units,read-after-write,1", "game,units,Unit::x,units,read-after-write,1",
                    "units,weapons,unresolved:block,units,read-after-write,1")
        self.assertEqual(dc.classify({"a": a}, []), [])

    def test_dynamic_only(self):
        a = summary("units,weapons,Unit::x,units,read-after-write,1", "units,ai,Unit::x,units,read-after-write,1")
        rows = by_edge(dc.classify({"a": a}, coupling(("units", "ai", "call", "F"))))
        self.assertEqual(rows[("units", "weapons", "Unit::x", "units")]["flags"], "dynamic-only")
        self.assertEqual(rows[("units", "ai", "Unit::x", "units")]["flags"], "")

    def test_unobserved_coupling_edge(self):
        a = summary("units,weapons,Unit::x,units,read-after-write,1")
        edges = coupling(("units", "weapons", "shared_write", "Unit::x"), ("units", "weapons", "shared_write", "Unit::y"),
                         ("units", "game", "shared_write", "Unit::z"), ("units", "weapons", "indirect", "F"))
        rows = dc.classify({"a": a}, edges)
        r = by_edge(rows)[("units", "weapons", "Unit::y", "-")]
        self.assertEqual((r["class"], r["runs"], r["example"]), ("unobserved", 0, "-"))
        self.assertEqual(len(rows), 2)
        self.assertEqual(dc.uncovered(rows, edges), [])
        self.assertEqual(dc.uncovered([], edges), [("units", "weapons", "Unit::x"), ("units", "weapons", "Unit::y")])


class Store(unittest.TestCase):
    def test_store_drops_presentation_and_is_reproducible(self):
        with tempfile.TemporaryDirectory() as t:
            src, dst = Path(t) / "src", Path(t) / "dst"
            src.mkdir()
            (src / "s.edges.csv").write_text(HEADER + 'a,b,"i",units,ordered,1,2\n-,ai,"j",presentation,read-unwritten,9,1\n')
            dc.store(src, dst)
            first = (dst / "s.edges.csv.gz").read_bytes()
            dc.store(src, dst)
            self.assertEqual(first, (dst / "s.edges.csv.gz").read_bytes())
            self.assertNotIn("presentation", gzip.decompress(first).decode())
            self.assertEqual(list(dc.load_summaries(dst)), ["s"])


if __name__ == "__main__":
    unittest.main()
