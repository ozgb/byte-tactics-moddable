"""Tests for phaseclass.py, on synthetic phase files.

    uv run tools/test_phaseclass.py
"""

import gzip
import tempfile
import unittest
from pathlib import Path

import depclass
import phaseclass as pc

CROSS = "writer,reader,item,phase_from,phase_to,evidence,count,first_tick\n"
DRAWS = "pass,stream,subsystem,function,entities,draws_min,draws_max,ticks,first_tick,tainted,items\n"
PASS = "pass,item,reader,function,evidence,count,first_tick\n"
RAW = "read-after-write"
BEFORE = "read-before-write"


def cross(*rows: str) -> list[dict]:
    """Rows of a cross file from lines `writer,reader,item,from,to,evidence,tick`."""
    lines = []
    for r in rows:
        w, rd, item, a, b, ev, tick = r.split(",")
        lines.append('%s,%s,"%s",%s,%s,%s,3,%s\n' % (w, rd, item, a, b, ev, tick))
    return pc.read_rows(CROSS + "".join(lines))


def by_key(rows: list[dict], *columns: str) -> dict:
    """The rows keyed by the columns."""
    return {(r[columns[0]] if len(columns) == 1 else tuple(r[c] for c in columns)): r for r in rows}


def deps(*rows: str) -> list[dict]:
    """phase_dependencies rows of one scenario for lines of `cross`."""
    return pc.phase_dependencies({"a": cross(*rows)}, [])


class PhaseDependencies(unittest.TestCase):
    def test_immediate_and_carried(self):
        rows = by_key(deps("units,weapons,Unit::x,units,players,%s,5" % RAW, "ai,units,Unit::y,players,units,%s,7" % BEFORE),
                      "writer", "reader", "item")
        r = rows[("units", "weapons", "Unit::x")]
        self.assertEqual((r["phase_from"], r["phase_to"], r["class"], r["runs"], r["example"]), ("units", "players", "immediate", 1, "a@5"))
        r = rows[("ai", "units", "Unit::y")]
        self.assertEqual((r["phase_from"], r["phase_to"], r["class"]), ("players", "units", "carried"))

    def test_not_an_edge(self):
        rows = deps("units,units,Unit::x,units,players,%s,1" % RAW, "game,units,Unit::x,units,players,%s,1" % RAW,
                    "units,game,Unit::x,units,players,%s,1" % RAW, "units,weapons,unresolved:block,units,players,%s,1" % RAW)
        self.assertEqual(rows, [])

    def test_a_presentation_reader_is_left_out(self):
        rows = deps("units,presentation,Unit::x,tail,presentation,%s,1" % RAW, "units,presentation,Unit::x,units,players,%s,1" % RAW,
                    "units,weapons,Unit::x,units,players,%s,1" % RAW)
        self.assertEqual([r["reader"] for r in rows], ["weapons"])
        boundaries = by_key(pc.phase_boundaries(rows), "boundary")
        self.assertEqual((boundaries["tail|presentation"]["needed"], boundaries["tail|presentation"]["edges"]), ("no", 0))

    def test_a_presentation_writer_and_sim_reader_row_is_kept_by_phase_dependencies(self):
        rows = deps("presentation,weapons,Unit::x,units,players,%s,1" % RAW, "presentation,game,Unit::x,units,players,%s,1" % RAW)
        self.assertEqual([(r["writer"], r["reader"]) for r in rows], [("presentation", "weapons")])

    def test_runs_example_and_both_classes_of_one_key(self):
        a = cross("units,weapons,Unit::x,units,players,%s,9" % RAW)
        b = cross("units,weapons,Unit::x,units,players,%s,4" % RAW, "units,weapons,Unit::x,units,players,%s,6" % BEFORE)
        rows = pc.phase_dependencies({"b": b, "a": a}, [])
        self.assertEqual([(r["class"], r["runs"], r["example"]) for r in rows], [("carried", 1, "b@6"), ("immediate", 2, "b@4")])

    def test_dynamic_only_follows_the_coupling_rows(self):
        coupling = [{"from": "units", "to": "ai", "kind": "call", "target": "F", "count": "1"}]
        rows = pc.phase_dependencies({"a": cross("units,weapons,Unit::x,units,players,%s,1" % RAW,
                                                 "units,ai,Unit::x,units,players,%s,1" % RAW)}, coupling)
        self.assertEqual({r["reader"]: r["flags"] for r in rows}, {"weapons": depclass.DYNAMIC_ONLY, "ai": ""})


def feed(item="Unit::x", phase="players", reason="why", example="a@1", reader="weapons"):
    """A row of data/presentation_feeds.csv."""
    return {"writer": "presentation", "reader": reader, "item": item, "phase": phase, "example": example, "reason": reason}


def feed_tables(listed: list[dict], *lines: str) -> dict:
    """The tables of one scenario with the cross rows of `lines`, the listed feeds given."""
    with tempfile.TemporaryDirectory() as t:
        d = Path(t)
        (d / "a.cross.csv.gz").write_bytes(gzip.compress(pc.render_rows(pc.CROSS_COLUMNS, cross(*lines)).encode(), 9, mtime=0))
        return pc.build(d, [], listed)


class PresentationFeeds(unittest.TestCase):
    LINES = ("presentation,weapons,Unit::x,units,players,%s,5" % RAW, "presentation,weapons,Unit::x,tail,players,%s,3" % BEFORE,
             "units,ai,Unit::y,units,players,%s,7" % RAW)

    def problems(self, listed):
        tables = feed_tables(listed, *self.LINES)
        return tables, pc.feed_problems(tables["presentation_feeds"], tables["feeds_listed"])

    def test_the_feeds_are_one_row_per_phase_to_and_out_of_the_phase_files(self):
        tables, _ = self.problems([])
        self.assertEqual([(r["item"], r["phase"], r["example"], r["reason"]) for r in tables["presentation_feeds"]], [("Unit::x", "players", "a@3", "")])
        self.assertEqual([r["writer"] for r in tables["phase_dependencies"]], ["units"])
        self.assertEqual(sum(r["edges"] for r in tables["phase_boundaries"]), 3)

    def test_an_unlisted_feed_fails_with_the_row_named(self):
        _, problems = self.problems([])
        self.assertEqual(len(problems), 1)
        self.assertIn("presentation -> weapons, item Unit::x, read in players (a@3)", problems[0])
        self.assertIn("not listed", problems[0])

    def test_a_listed_feed_passes_and_keeps_its_reason(self):
        tables, problems = self.problems([feed(reason="Selection")])
        self.assertEqual(problems, [])
        self.assertEqual(tables["presentation_feeds"][0]["reason"], "Selection")

    def test_a_stale_feed_fails_and_is_dropped_from_the_regenerated_rows(self):
        tables, problems = self.problems([feed(reason="Selection"), feed(item="Unit::gone", reason="Old")])
        self.assertEqual(len(problems), 1)
        self.assertIn("stale", problems[0])
        self.assertIn("item Unit::gone", problems[0])
        self.assertEqual([r["item"] for r in tables["presentation_feeds"]], ["Unit::x"])

    def test_an_empty_reason_fails(self):
        _, problems = self.problems([feed(reason=" ")])
        self.assertEqual(len(problems), 1)
        self.assertIn("empty reason", problems[0])

    def test_check_includes_the_feed_problems(self):
        tables = feed_tables([], *self.LINES)
        with tempfile.TemporaryDirectory() as t:
            self.assertTrue(any("not listed" in p for p in pc.check(Path(t), tables, {})))


class Boundaries(unittest.TestCase):
    def rows(self, *lines: str) -> dict:
        return by_key(pc.phase_boundaries(deps(*lines)), "boundary")

    def test_every_boundary_has_a_row(self):
        self.assertEqual(list(self.rows()), ["0|1", "1|2", "2|3", "3|4", "4|5", "5|6", "6|7", "7|tail", "tail|presentation"])

    def test_the_net_boundary_is_absent(self):
        r = self.rows()["0|1"]
        self.assertEqual((r["needed"], r["mergeable"]), ("absent", "absent"))

    def test_an_immediate_row_is_needed_across_every_boundary_it_spans(self):
        rows = self.rows("units,weapons,Unit::x,projectiles,players,%s,5" % RAW)
        self.assertEqual([k for k, r in rows.items() if r["needed"] == "yes"], ["2|3", "3|4"])
        self.assertEqual((rows["2|3"]["edges"], rows["2|3"]["example"], rows["1|2"]["mergeable"]), (1, "a@5", "yes"))
        self.assertEqual([r["direct"] for r in rows.values()], [0, 0, 0, 0, 0, 0, 0, 0, 0])

    def test_a_row_between_neighbours_is_direct_for_its_one_boundary(self):
        rows = self.rows("units,weapons,Unit::x,projectiles,explosions,%s,5" % RAW, "units,ai,Unit::x,projectiles,players,%s,5" % RAW)
        self.assertEqual((rows["2|3"]["edges"], rows["2|3"]["direct"], rows["3|4"]["edges"], rows["3|4"]["direct"]), (2, 1, 1, 0))

    def test_a_carried_row_makes_the_boundaries_it_spans_unmergeable_but_not_needed(self):
        rows = self.rows("ai,units,Unit::y,players,projectiles,%s,7" % BEFORE)
        self.assertEqual([k for k, r in rows.items() if r["mergeable"] == "no"], ["2|3", "3|4"])
        self.assertEqual([r["needed"] for k, r in rows.items() if k in ("2|3", "3|4")], ["no", "no"])
        self.assertEqual(rows["2|3"]["carried"], 1)

    def test_the_presentation_boundary_counts_the_rows_that_end_in_it(self):
        rows = self.rows("units,weapons,Unit::x,units,presentation,%s,2" % RAW)
        self.assertEqual((rows["tail|presentation"]["needed"], rows["tail|presentation"]["edges"]), ("yes", 1))
        self.assertEqual([r["needed"] for k, r in rows.items() if k not in ("0|1", "tail|presentation")], ["no"] * 7)

    def test_the_example_is_the_row_with_the_most_runs(self):
        a = cross("units,weapons,Unit::x,units,players,%s,5" % RAW, "units,ai,Unit::y,units,players,%s,6" % RAW)
        b = cross("units,ai,Unit::y,units,players,%s,8" % RAW)
        rows = by_key(pc.phase_boundaries(pc.phase_dependencies({"a": a, "b": b}, [])), "boundary")
        self.assertEqual((rows["1|2"]["edges"], rows["1|2"]["example"]), (2, "a@6"))


def draw_rows(*rows: str) -> list[dict]:
    """Rows of a draws file from lines `pass,stream,subsystem,function,entities,min,max,ticks,first,tainted,items`."""
    return pc.read_rows(DRAWS + "".join(r + "\n" for r in rows))


class Draws(unittest.TestCase):
    def test_start_state(self):
        self.assertEqual(pc.start_state(2, 2, True), "fixed")
        self.assertEqual(pc.start_state(1, 3, False), "start-state")
        self.assertEqual(pc.start_state(1, 3, True), "depends-on-earlier")

    def test_a_row_per_pass_stream_and_function_over_the_scenarios(self):
        a = draw_rows('units,rng.parkMiller,weapons,"F",2,1,3,10,5,1,"x;y"', 'features,rng.parkMiller,features,"G",1,1,1,4,2,0,""')
        b = draw_rows('units,rng.parkMiller,weapons,"F",5,2,4,6,3,1,"y;z"')
        rows = by_key(pc.draws({"a": a, "b": b}), "pass", "function")
        r = rows[("units", "F")]
        self.assertEqual((r["entities"], r["draws_min"], r["draws_max"], r["ticks"], r["start_state"], r["example"]), (5, 1, 4, 16, "depends-on-earlier", "b@3"))
        self.assertEqual(r["evidence"], "y;x;z")
        g = rows[("features", "G")]
        self.assertEqual((g["start_state"], g["evidence"]), ("fixed", ""))

    def test_a_variable_untainted_count_follows_the_start_state(self):
        r = pc.draws({"a": draw_rows('wind,rng.parkMiller,wind_meteors,"W",1,1,2,3,1,0,""')})[0]
        self.assertEqual((r["start_state"], r["evidence"]), ("start-state", ""))


class PassReads(unittest.TestCase):
    def test_rows_runs_and_the_empty_passes(self):
        a = pc.read_rows(PASS + 'units,"Unit::x",orders,"F",same-record,4,7\nunits,"unresolved:block",orders,"F",other-record,1,1\n')
        b = pc.read_rows(PASS + 'units,"Unit::x",orders,"F",same-record,2,3\n')
        rows = pc.pass_reads({"a": a, "b": b})
        r = by_key(rows, "pass", "item")[("units", "Unit::x")]
        self.assertEqual((r["runs"], r["example"], r["evidence"]), (2, "b@3", "same-record"))
        self.assertNotIn("unresolved:block", [x["item"] for x in rows])
        self.assertEqual(sorted(x["pass"] for x in rows if x["evidence"] == "none"), sorted(set(pc.PASSES) - {"units"}))


class Store(unittest.TestCase):
    def test_store_reduces_sorts_and_is_reproducible(self):
        with tempfile.TemporaryDirectory() as t:
            src, dst = Path(t) / "src", Path(t) / "dst"
            src.mkdir()
            (src / "s.edges.csv").write_text("writer,reader,item,phase,evidence,count,first_tick\n")
            (src / "s.cross.csv").write_text(CROSS + 'b,c,"i",tail,presentation,read-after-write,99,4\nb,presentation,"i",tail,units,read-after-write,5,4\na,c,"i",units,tail,read-after-write,7,2\n')
            (src / "s.draws.csv").write_text(DRAWS + 'presentation,rng.parkMiller,game,"F",1,1,1,9,1,0,""\nunits,rng.parkMiller,game,"F",1,1,1,9,1,0,""\n')
            (src / "s.pass.csv").write_text(PASS + 'units,"i",ai,"F",same-record,1,1\n')
            (src / "s.ordered.csv").write_text("writer,reader,item,phase,evidence,count,first_tick\na,b,\"sim-event\",presentation,ordered,5,1\na,b,\"sim-event\",units,ordered,5,1\n")
            self.assertEqual(pc.store(src, dst), 1)
            first = {p.name: p.read_bytes() for p in dst.iterdir()}
            pc.store(src, dst)
            self.assertEqual(first, {p.name: p.read_bytes() for p in dst.iterdir()})
            cross_rows = pc.load_kind(dst, "cross")["s"]
            self.assertEqual([(r["writer"], r["count"]) for r in cross_rows], [("a", "7"), ("b", "0")])
            self.assertNotIn("presentation", [r["reader"] for r in cross_rows])
            self.assertEqual([r["pass"] for r in pc.load_kind(dst, "draws")["s"]], ["units"])
            self.assertEqual([r["phase"] for r in pc.load_kind(dst, "ordered")["s"]], ["units"])
            self.assertIn(b"s.cross.csv.gz", b" ".join(k.encode() for k in first))
            self.assertEqual(gzip.decompress(first["s.pass.csv.gz"]).decode().splitlines()[0], PASS.strip())

    def test_the_ordered_file_joins_the_edges_in_depclass(self):
        with tempfile.TemporaryDirectory() as t:
            edges, ordered = Path(t) / "edges", Path(t) / "ordered"
            edges.mkdir()
            ordered.mkdir()
            (edges / "s.edges.csv.gz").write_bytes(gzip.compress(b"writer,reader,item,phase,evidence,count,first_tick\n", 9, mtime=0))
            (ordered / "s.ordered.csv.gz").write_bytes(gzip.compress(
                b'writer,reader,item,phase,evidence,count,first_tick\nunits,weapons,"sim-event",units,ordered,2,7\n', 9, mtime=0))
            rows = depclass.classify(depclass.load_summaries(edges, ordered), [])
            self.assertEqual([(r["item"], r["class"], r["example"]) for r in rows], [("sim-event", "ordered", "s@7")])
            self.assertEqual(depclass.classify(depclass.load_summaries(edges), []), [])


class DepclassChecksThePhaseFiles(unittest.TestCase):
    def test_a_changed_phase_file_fails_depclass_check(self):
        import contextlib
        import io
        from unittest import mock

        with tempfile.TemporaryDirectory() as t:
            outputs = {n: (Path(t) / p.name, c) for n, (p, c) in pc.OUTPUTS.items()}
            tables = pc.build(pc.PHASES_DIR, depclass.read_coupling(depclass.COUPLING_CSV.read_text()))
            tables["feeds_listed"] = [{**r, "reason": "x"} for r in tables["presentation_feeds"]]
            tables["presentation_feeds"] = tables["feeds_listed"]
            for name, (path, columns) in outputs.items():
                path.write_text(pc.render_rows(columns, tables[name]))
            self.assertEqual(pc.check(pc.PHASES_DIR, tables, outputs), [])
            outputs["draws"][0].write_text(outputs["draws"][0].read_text().replace("fixed", "start-state", 1))
            self.assertTrue(any("draws.csv differs" in p for p in pc.check(pc.PHASES_DIR, tables, outputs)))
            with mock.patch.object(pc, "OUTPUTS", outputs), mock.patch.object(pc, "check_stored", lambda: pc.check(pc.PHASES_DIR, tables, outputs)):
                with mock.patch("sys.argv", ["depclass.py", "--check"]):
                    buf = io.StringIO()
                    with contextlib.redirect_stdout(buf):
                        status = depclass.main()
            self.assertEqual(status, 1)
            self.assertIn("draws.csv differs", buf.getvalue())


if __name__ == "__main__":
    unittest.main()
