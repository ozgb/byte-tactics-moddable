"""Tests for tracediff.py's translation of the reference's leaf names, on synthetic schemas and dumps.

    uv run tools/test_tracediff.py
"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import tracediff as td
from traceschema import TRACE_FORMAT

REFERENCE_SCHEMA = """
static const TraceField kUnitFields[] = {
    { "f10", 0x0, 4, TF_POINTER, TT_HEAP, 0x0 },
    { "health", 0x4, 2, TF_SCALAR, TT_NONE, 0x0 },
    { "", 0x6, 2, TF_BYTES, TT_NONE, 0x0 },
    { "field_8", 0x8, 4, TF_SCALAR, TT_NONE, 0x0 },
};

static const TraceField kPlayerFields[] = {
    { "metal", 0x0, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "energy", 0x4, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "name", 0x8, 4, TF_BYTES, TT_NONE, 0x0 },
    { "", 0xc, 4, TF_BYTES, TT_NONE, 0x0 },
    { "tail", 0x10, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "lo", 0x14, 4, TF_BYTES, TT_NONE, 0x0 },
    { "hi", 0x18, 4, TF_BYTES, TT_NONE, 0x0 },
    { "total", 0x1c, 8, TF_BYTES, TT_NONE, 0x0 },
};
"""

MODERN_SCHEMA = """
static const TraceField kUnitFields[] = {
    { "weapon", 0x0, 4, TF_POINTER, TT_HEAP, 0x0 },
    { "health", 0x4, 2, TF_SCALAR, TT_NONE, 0x0 },
    { "", 0x6, 2, TF_BYTES, TT_NONE, 0x0 },
    { "cargo", 0x8, 4, TF_SCALAR, TT_NONE, 0x0 },
};

static const TraceField kPlayerFields[] = {
    { "energy", 0x0, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "metal", 0x4, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "name", 0x8, 8, TF_BYTES, TT_NONE, 0x0 },
    { "tail", 0x10, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "double", 0x14, 8, TF_SCALAR, TT_NONE, 0x0 },
    { "total", 0x1c, 8, TF_SCALAR, TT_NONE, 0x0 },
};
"""

REFERENCE_DUMP = """100 units units[3].f10 heap
100 units units[3].health 0x0064
100 units units[3].+0x6 hex:0102
100 units units[3].field_8 0x0000000a
100 players players[1].metal 0x00000001
100 players players[1].energy 0x00000002
100 players players[1].name hex:41424344
100 players players[1].+0xc hex:45464748
100 players players[1].tail 0x00000009
100 players players[1].lo hex:00000000
100 players players[1].hi hex:00004040
100 players players[1].total hex:0000000080662640
"""

MODERN_DUMP = """100 units units[3].weapon heap
100 units units[3].health 0x0064
100 units units[3].+0x6 hex:0102
100 units units[3].cargo 0x0000000a
100 players players[1].energy 0x00000001
100 players players[1].metal 0x00000002
100 players players[1].name hex:4142434445464748
100 players players[1].tail 0x00000009
100 players players[1].double 0x4040000000000000
100 players players[1].total 0x4026668000000000
"""


def write_dump(directory: Path, name: str, body: str) -> Path:
    path = directory / name
    path.write_text(f"bt-dump {TRACE_FORMAT}\n{body}")
    return path


class TableMapTest(unittest.TestCase):
    def setUp(self):
        self.maps = td.build_maps(REFERENCE_SCHEMA, MODERN_SCHEMA)

    def test_same_offset_and_size_is_renamed(self):
        renames = self.maps["kUnitFields"].renames
        self.assertEqual(renames["f10"], "weapon")
        self.assertEqual(renames["field_8"], "cargo")
        self.assertEqual(renames["health"], "health")
        self.assertEqual(renames["+0x6"], "+0x6")

    def test_swapped_names_are_each_mapped_to_the_other(self):
        renames = self.maps["kPlayerFields"].renames
        self.assertEqual((renames["metal"], renames["energy"]), ("energy", "metal"))

    def test_split_leaf_is_a_group_not_a_rename(self):
        table = self.maps["kPlayerFields"]
        self.assertNotIn("name", table.renames)
        self.assertNotIn("+0xc", table.renames)
        group = table.reference_groups["name"]
        self.assertIs(group, table.reference_groups["+0xc"])
        self.assertIs(group, table.modern_groups["name"])
        self.assertEqual(group.key, "name")


class DiffDumpsTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.reference = write_dump(Path(self.dir.name), "reference.txt", REFERENCE_DUMP)
        self.modern = write_dump(Path(self.dir.name), "modern.txt", MODERN_DUMP)
        self.maps = td.build_maps(REFERENCE_SCHEMA, MODERN_SCHEMA)

    def diff(self, reference):
        a, b = (self.reference, self.modern) if reference != "b" else (self.modern, self.reference)
        return td.diff_dumps(a, b, 100, "units", reference, self.maps), td.diff_dumps(a, b, 100, "players", reference, self.maps)

    def test_without_reference_nothing_is_translated(self):
        units, players = self.diff(None)
        self.assertEqual([d[0] for d in units], ["units[3].f10", "units[3].field_8", "units[3].weapon", "units[3].cargo"])
        self.assertEqual([d[0] for d in players],
                         ["players[1].metal", "players[1].energy", "players[1].name", "players[1].+0xc",
                          "players[1].lo", "players[1].hi", "players[1].total", "players[1].double"])
        self.assertIn(("players[1].metal", "0x00000001", "0x00000002"), players)

    def test_reference_a_renames_and_swaps(self):
        units, players = self.diff("a")
        self.assertEqual(units, [])
        self.assertEqual(players, [])

    def test_reference_b_translates_the_other_side(self):
        units, players = self.diff("b")
        self.assertEqual((units, players), ([], []))

    def test_swapped_names_still_report_real_differences(self):
        self.reference.write_text(self.reference.read_text().replace("players[1].metal 0x00000001", "players[1].metal 0x00000007"))
        _, players = self.diff("a")
        self.assertEqual(players, [("players[1].energy", "0x00000007", "0x00000001")])

    def test_split_leaf_compares_on_its_bytes(self):
        self.reference.write_text(self.reference.read_text().replace("hex:45464748", "hex:45464749"))
        _, players = self.diff("a")
        self.assertEqual(players, [("players[1].name", "hex:4142434445464749", "hex:4142434445464748")])

    def test_one_modern_scalar_equals_two_reference_leaves_on_bytes(self):
        _, players = self.diff("a")
        self.assertEqual(players, [])
        self.reference.write_text(self.reference.read_text().replace("hex:00004040", "hex:00004041"))
        _, players = self.diff("a")
        self.assertEqual([d[0] for d in players], ["players[1].double"])

    def test_renamed_leaf_of_a_different_kind_compares_on_bytes(self):
        _, players = self.diff("a")
        self.assertNotIn("players[1].total", [d[0] for d in players])
        self.assertTrue(td.same_bytes("hex:0000000080662640", "0x4026668000000000"))
        self.assertFalse(td.same_bytes("hex:0000000080662640", "0x4026668000000001"))

    def test_wrong_side_leaves_names_untranslated(self):
        units = td.diff_dumps(self.reference, self.modern, 100, "units", "b", self.maps)
        self.assertIn("units[3].f10", [d[0] for d in units])

    def test_unknown_paths_pass_through(self):
        fields = {"mystery[0].x": "0x1", "units[3].f10": "heap"}
        out = td.translate_phase(fields, self.maps, True)
        self.assertEqual(out, {"mystery[0].x": "0x1", "units[3].weapon": "heap"})


class EventKindsTest(unittest.TestCase):
    def test_events_of_different_kinds_are_compared_field_by_field(self):
        a = ("100 units targeting.count 0x00000002\n100 units targeting[0].kind 0x00000060\n"
             "100 units targeting[0].target unit:4\n100 units targeting[1].kind 0x00000062\n"
             "100 units targeting[1].outcome 0x00000001\n")
        b = a.replace("unit:4", "null").replace("targeting[1].kind 0x00000062", "targeting[1].kind 0x00000063")
        with tempfile.TemporaryDirectory() as d:
            one, two = write_dump(Path(d), "a.txt", a), write_dump(Path(d), "b.txt", b)
            found = td.diff_dumps(one, two, 100, "units")
        self.assertEqual(found, [("targeting[0].target", "unit:4", "null"),
                                 ("targeting[1].kind", "0x00000062", "0x00000063")])


class EndOfTickEventsTest(unittest.TestCase):
    SOUND = "5 units sound 0x0 0 0"
    SOUND_AT = "5 units sound_at 0x0 1 2 3 4 5"
    SMOKE = "5 units smoke 0x0 1 2 3 15 900 9"
    NEXT = "6 units sound 0x0 7 0"

    @staticmethod
    def log(*lines):
        return "bt-events 1\n" + "".join(line + "\n" for line in lines)

    def test_equal_ticks_with_a_commuting_swap_are_equal(self):
        a = self.log(self.SOUND, self.SMOKE, self.NEXT)
        b = self.log(self.SMOKE, self.SOUND, self.NEXT)
        self.assertIsNotNone(td.event_difference(a, b))
        self.assertIsNone(td.end_of_tick_event_difference(a, b))

    def test_a_swap_that_counts_names_the_pair_and_the_first_record_that_differs(self):
        a = self.log(self.SMOKE, self.SOUND, self.SOUND_AT, self.NEXT)
        b = self.log(self.SMOKE, self.SOUND_AT, self.SOUND, self.NEXT)
        lines = td.end_of_tick_event_difference(a, b).splitlines()
        self.assertTrue(lines[0].startswith("event records at tick 5: the tick lines differ: a `5 tick 3 "), lines[0])
        self.assertIn("the order of the kinds `sound` and `sound_at` differs", lines[1])
        self.assertEqual(lines[2], "  is number 1 of 2 and 2")
        self.assertEqual(lines[3], f"  a: line 3 `{self.SOUND}`")
        self.assertEqual(lines[4], f"  b: line 3 `{self.SOUND_AT}`")

    def test_the_first_pair_in_ascending_order_is_named(self):
        a = self.log(self.SOUND, self.SOUND_AT, "5 units sound 0x0 1 0")
        b = self.log("5 units sound 0x0 1 0", self.SOUND_AT, self.SOUND)
        report = td.end_of_tick_event_difference(a, b)
        self.assertIn("the order of the kinds `sound` and `sound` differs", report)
        self.assertIn(f"a: line 2 `{self.SOUND}`", report)
        self.assertIn("b: line 2 `5 units sound 0x0 1 0`", report)

    def test_a_record_that_only_one_log_has_is_named_as_a_set_difference(self):
        a = self.log(self.SOUND, self.SMOKE, self.NEXT)
        b = self.log(self.SOUND, self.NEXT)
        report = td.end_of_tick_event_difference(a, b)
        self.assertIn("the records differ as a set", report)
        self.assertIn(f"a: line 3 `{self.SMOKE}`", report)
        self.assertIn("b: no such record", report)

    def test_a_changed_argument_shows_both_records(self):
        changed = self.SOUND.replace("0 0", "1 0")
        report = td.end_of_tick_event_difference(self.log(self.SOUND, self.NEXT), self.log(changed, self.NEXT))
        self.assertIn("the records differ as a set", report)
        self.assertIn(f"a: line 2 `{self.SOUND}`", report)
        self.assertIn("b: no such record", report)

    def test_the_first_differing_tick_is_named_not_a_later_one(self):
        a = self.log(self.SOUND, self.NEXT)
        b = self.log(self.SOUND, self.NEXT.replace("7 0", "8 0"), "7 units sound 0x0 0 0")
        self.assertTrue(td.end_of_tick_event_difference(a, b).startswith("event records at tick 6: "))

    def test_a_tick_only_one_log_has_is_named(self):
        report = td.end_of_tick_event_difference(self.log(self.SOUND), self.log(self.SOUND, self.NEXT))
        self.assertEqual(report, "event records at tick 6: only b has records of the tick (1, the first at line 3)")

    def test_a_phase_change_is_not_a_difference(self):
        a = self.log(self.SOUND, self.NEXT)
        b = self.log(self.SOUND.replace("units", "players"), self.NEXT)
        self.assertIsNone(td.end_of_tick_event_difference(a, b))

    def test_the_command_line_takes_the_switch_for_event_logs(self):
        with tempfile.TemporaryDirectory() as d:
            one, two = Path(d) / "a.txt", Path(d) / "b.txt"
            one.write_text(self.log(self.SOUND, self.SMOKE))
            two.write_text(self.log(self.SMOKE, self.SOUND))
            for extra, code in (([], 1), (["--end-of-tick"], 0)):
                out = io.StringIO()
                with mock.patch("sys.argv", ["tracediff.py", "--event-logs", str(one), str(two), *extra]), contextlib.redirect_stdout(out):
                    self.assertEqual(td.main(), code)
            self.assertIn("equal at the end of every tick", out.getvalue())


class CommandLineTest(unittest.TestCase):
    def run_main(self, *argv):
        out, err = io.StringIO(), io.StringIO()
        with mock.patch("sys.argv", ["tracediff.py", *argv]), mock.patch.object(td, "load_maps", return_value=td.build_maps(REFERENCE_SCHEMA, MODERN_SCHEMA)), \
                contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = td.main()
        return code, out.getvalue()

    def test_dumps_with_reference_option(self):
        with tempfile.TemporaryDirectory() as d:
            ref = write_dump(Path(d), "ref.txt", REFERENCE_DUMP)
            mod = write_dump(Path(d), "mod.txt", MODERN_DUMP)
            code, out = self.run_main("--dumps", str(ref), str(mod), "--at", "100", "players", "--reference", "a")
            self.assertEqual(code, 0)
            self.assertIn("no field differs", out)
            code, out = self.run_main("--dumps", str(ref), str(mod), "--at", "100", "players")
            self.assertEqual(code, 1)
            self.assertIn("players[1].metal", out)


class ReferenceDisplayTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        (self.dir / "s.txt").write_text("ticks 9\n")
        (self.dir / "trace.txt").write_text(f"bt-trace {TRACE_FORMAT}\nscenario s\n1 units 00\n")

    def tearDown(self):
        self.tmp.cleanup()

    def command(self, **kwargs):
        done = mock.Mock(returncode=0, stdout="logs and screenshots: out/\n", stderr="")
        with mock.patch.object(td.subprocess, "run", return_value=done) as run, mock.patch.object(td, "ROOT", self.dir):
            (self.dir / "out" / "r").mkdir(parents=True, exist_ok=True)
            td.play(Path("TotalA.exe"), self.dir / "s.txt", None, None, **kwargs)
        return run.call_args.args[0]

    def test_play_starts_an_xserver_only_when_asked(self):
        self.assertNotIn("--xserver", self.command())
        self.assertIn("--xserver", self.command(xserver=True))

    def test_the_reference_side_gets_a_display_and_the_other_does_not(self):
        for side, expected in (("a", [True, False]), ("b", [False, True]), (None, [False, False])):
            calls = []

            def play_loading(exe, scenario, game_dir, dump, timeout, poison, xserver):
                calls.append(xserver)
                return self.dir

            argv = ["tracediff.py", "a.exe", "b.exe", "--scenario", str(self.dir / "s.txt")]
            with mock.patch("sys.argv", argv + (["--reference", side] if side else [])), \
                    mock.patch.object(td, "play_loading", play_loading), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(td.main(), 0)
            self.assertEqual(calls, expected, side)


class TickRowTest(unittest.TestCase):
    def traces(self, d, a, b):
        paths = []
        for name, rows in (("a.txt", a), ("b.txt", b)):
            path = Path(d) / name
            path.write_text(f"bt-trace {TRACE_FORMAT}\nscenario s\ngame_seed 1\ncrt_seed 1\nview_seed 1\n" + "".join(rows))
            paths.append(str(path))
        return paths

    def run_main(self, *argv):
        out = io.StringIO()
        with mock.patch("sys.argv", ["tracediff.py", *argv]), contextlib.redirect_stdout(out):
            return td.main(), out.getvalue()

    def test_tick_rows_parse_and_follow_world(self):
        self.assertEqual(td.parse_row("3 tick 00000000000000aa"), (3, "tick", "00000000000000aa"))
        self.assertLess(td.PHASES.index("world"), td.PHASES.index("tick"))
        self.assertEqual(td.PHASES[-1], "tick")

    def test_a_tick_row_has_its_own_dump_block_and_no_phase_function(self):
        self.assertNotIn("tick", td.DUMP_PHASE)
        self.assertEqual(td.DUMP_PHASE["world"], "meteors")
        self.assertIn("end of the tick", td.describe_phase("tick"))

    def test_a_differing_tick_row_is_a_divergence_without_the_switch(self):
        with tempfile.TemporaryDirectory() as d:
            a, b = self.traces(d, ["1 units 01\n", "1 world 02\n", "1 tick 03\n"], ["1 units 01\n", "1 world 02\n", "1 tick 04\n"])
            code, out = self.run_main("--traces", a, b)
        self.assertEqual(code, 1)
        self.assertIn("tick 1, phase tick", out)

    def test_end_of_tick_ignores_the_phase_rows(self):
        with tempfile.TemporaryDirectory() as d:
            a, b = self.traces(d, ["1 units 01\n", "1 tick 03\n", "2 units 05\n", "2 tick 06\n"],
                               ["1 units 09\n", "1 tick 03\n", "2 units 05\n", "2 tick 07\n"])
            code, out = self.run_main("--traces", a, b)
            self.assertIn("tick 1, phase units", out)
            code, out = self.run_main("--traces", a, b, "--end-of-tick")
        self.assertEqual(code, 1)
        self.assertIn("tick 2, phase tick", out)

    def test_end_of_tick_reports_identical_when_the_tick_rows_agree(self):
        with tempfile.TemporaryDirectory() as d:
            a, b = self.traces(d, ["1 units 01\n", "1 tick 03\n"], ["1 units 02\n", "1 tick 03\n"])
            code, out = self.run_main("--traces", a, b, "--end-of-tick")
        self.assertEqual(code, 0)
        self.assertIn("traces identical", out)


class PlayLoadingTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        (self.dir / "loader.txt").write_text("ticks 9\nload saver\n")
        (self.dir / "saver.txt").write_text("ticks 9\nsave 4\n")
        (self.dir / "plain.txt").write_text("ticks 9\n")
        self.runs = []

    def tearDown(self):
        self.tmp.cleanup()

    def fake_play(self, writes_save=True, fails=False, writes=("harness.sav", "harness.opt")):
        def play(exe, scenario, game_dir, dump, timeout=None, poison=False, blockscan=0, env=None, put=None, collect=(), xserver=False):
            run = self.dir / "session" / f"run-{scenario.stem}" / "run"
            run.mkdir(parents=True)
            seen = {"scenario": scenario.stem, "put": {k: v.read_bytes() for k, v in (put or {}).items()},
                    "collect": collect, "dump": dump, "xserver": xserver}
            if fails and scenario.stem == "saver":
                raise td.Error("exited with 2")
            if collect and writes_save:
                (run / "savegame").mkdir()
                for file in writes:
                    (run / "savegame" / file).write_bytes(b"the " + file.encode())
            self.runs.append(seen)
            return run
        return play

    def test_scenario_load_reads_the_line(self):
        self.assertEqual(td.scenario_load(self.dir / "loader.txt"), "saver")
        self.assertIsNone(td.scenario_load(self.dir / "plain.txt"))

    def test_a_plain_scenario_is_played_once(self):
        with mock.patch.object(td, "play", self.fake_play()):
            td.play_loading(Path("a.exe"), self.dir / "plain.txt", None, 7)
        self.assertEqual([r["scenario"] for r in self.runs], ["plain"])
        self.assertEqual(self.runs[0]["put"], {})

    def test_a_loader_plays_its_saver_first_and_gets_the_save(self):
        with mock.patch.object(td, "play", self.fake_play()), mock.patch.object(td, "ROOT", self.dir):
            td.play_loading(Path("a.exe"), self.dir / "loader.txt", None, 7)
        self.assertEqual([r["scenario"] for r in self.runs], ["saver", "loader"])
        self.assertEqual(self.runs[0]["collect"], ("savegame/harness.sav", "savegame/harness.opt"))
        self.assertIsNone(self.runs[0]["dump"])
        self.assertEqual(self.runs[1]["put"], {"savegame/harness.sav": b"the harness.sav",
                                               "savegame/harness.opt": b"the harness.opt"})
        self.assertEqual(self.runs[1]["dump"], 7)
        self.assertFalse((self.dir / "session" / "run-saver").exists())

    def test_a_saver_that_wrote_no_save_is_named(self):
        with mock.patch.object(td, "play", self.fake_play(writes_save=False)), mock.patch.object(td, "ROOT", self.dir):
            with self.assertRaises(td.Error) as raised:
                td.play_loading(Path("a.exe"), self.dir / "loader.txt", None, None)
        self.assertIn("loader: load saver: saver wrote no savegame/harness.sav", str(raised.exception))
        self.assertEqual([r["scenario"] for r in self.runs], ["saver"])

    def test_a_saver_that_wrote_only_one_file_is_named_for_the_other(self):
        for written, missing in (("harness.sav", "harness.opt"), ("harness.opt", "harness.sav")):
            self.runs.clear()
            with mock.patch.object(td, "play", self.fake_play(writes=(written,))), mock.patch.object(td, "ROOT", self.dir):
                with self.assertRaises(td.Error) as raised:
                    td.play_loading(Path("a.exe"), self.dir / "loader.txt", None, None)
            self.assertIn(f"loader: load saver: saver wrote no savegame/{missing}", str(raised.exception))
            self.assertEqual([r["scenario"] for r in self.runs], ["saver"])

    def test_a_saver_that_failed_is_named(self):
        with mock.patch.object(td, "play", self.fake_play(fails=True)), mock.patch.object(td, "ROOT", self.dir):
            with self.assertRaises(td.Error) as raised:
                td.play_loading(Path("a.exe"), self.dir / "loader.txt", None, None)
        self.assertIn("loader: load saver: saver failed to run", str(raised.exception))

    def test_a_missing_saver_scenario_is_named(self):
        (self.dir / "saver.txt").unlink()
        with mock.patch.object(td, "play", self.fake_play()), mock.patch.object(td, "ROOT", self.dir):
            with self.assertRaises(td.Error) as raised:
                td.play_loading(Path("a.exe"), self.dir / "loader.txt", None, None)
        self.assertIn("loader: load saver: there is no scenario", str(raised.exception))


class SaveHelperTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.dir = Path(tmp.name)
        self.run = self.dir / "run"
        (self.run / "savegame").mkdir(parents=True)

    def write(self, *names: str) -> None:
        for name in names:
            (self.run / "savegame" / name).write_bytes(b"the " + name.encode())

    def test_copy_save_copies_each_file_by_name(self):
        self.write("harness.sav", "harness.opt")
        td.copy_save(self.run, self.dir / "saved")
        self.assertEqual(sorted(f.name for f in (self.dir / "saved").iterdir()), ["harness.opt", "harness.sav"])
        self.assertEqual((self.dir / "saved" / "harness.sav").read_bytes(), b"the harness.sav")

    def test_copy_save_leaves_out_a_missing_file(self):
        self.write("harness.opt")
        td.copy_save(self.run, self.dir / "saved")
        self.assertEqual([f.name for f in (self.dir / "saved").iterdir()], ["harness.opt"])

    def test_save_put_maps_each_save_file_to_its_copy(self):
        self.write("harness.sav", "harness.opt")
        td.copy_save(self.run, self.dir / "saved")
        self.assertEqual(td.save_put(self.dir / "saved", "loader", "saver"),
                         {"savegame/harness.sav": self.dir / "saved" / "harness.sav",
                          "savegame/harness.opt": self.dir / "saved" / "harness.opt"})

    def test_save_put_names_the_loader_saver_and_missing_file(self):
        self.write("harness.sav")
        td.copy_save(self.run, self.dir / "saved")
        with self.assertRaises(td.Error) as raised:
            td.save_put(self.dir / "saved", "loader", "saver")
        self.assertEqual(str(raised.exception), "loader: load saver: saver wrote no savegame/harness.opt")


class DisplayTest(unittest.TestCase):
    PALETTE = "0123456789abcdef"

    @staticmethod
    def listing(*items: str, tick: int = 4) -> str:
        return "bt-display 1.2\n" + "".join(line + "\n" for line in [f"frame 1 {tick} 1 {DisplayTest.PALETTE}", *items, "cursor - - -"])

    def test_lists_are_compared_with_both_lines(self):
        one = self.listing("fill screen 0 0 10 10 1", "fill screen 20 20 30 30 2")
        two = self.listing("fill screen 0 0 10 10 1", "fill screen 20 20 30 30 3")
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            self.assertEqual(td.print_display_difference(one, two), 1)
            self.assertEqual(td.print_display_difference(one, one), 0)
        first, second = out.getvalue().splitlines()
        self.assertIn("frame 1 (tick 4): item at line 4 of A `fill screen 20 20 30 30 2`, line 4 of B `fill screen 20 20 30 30 3`", first)
        self.assertTrue(second.startswith("display lists identical"))

    def test_end_of_tick_forgives_a_swap_that_does_not_count(self):
        one = self.listing("fill screen 0 0 10 10 1", "fill screen 20 20 30 30 2")
        two = self.listing("fill screen 20 20 30 30 2", "fill screen 0 0 10 10 1")
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(td.print_display_difference(one, two), 1)
            self.assertEqual(td.print_display_difference(one, two, True), 0)

    def args(self, **kwargs):
        base = dict(exe=["a.exe", "b.exe"], scenario=["s.txt"], poison_b=False, reference=None, game_dir=None, end_of_tick=False, display_tick=None)
        return mock.Mock(**{**base, **kwargs})

    def run_display(self, digests: tuple[str, str], lists: tuple[str, str], **kwargs):
        calls = []
        with tempfile.TemporaryDirectory() as tmp:
            runs = []
            for i, (digest, text) in enumerate(zip(digests, lists)):
                run = Path(tmp) / f"run{i}"
                run.mkdir()
                (run / "display.txt").write_text(digest)
                (run / "displaylist.txt").write_text(text)
                runs.append(run)

            def fake(exe, scenario, game_dir, dump, timeout=None, poison=False, xserver=False, collect=(), testdisplay=None):
                calls.append(testdisplay)
                return runs[(len(calls) - 1) % 2]

            out = io.StringIO()
            with mock.patch.object(td, "play_loading", fake), contextlib.redirect_stdout(out):
                code = td.compare_display(self.args(**kwargs))
        return code, calls, out.getvalue()

    def test_the_first_differing_frame_is_replayed_with_its_tick(self):
        one = self.listing("fill screen 0 0 10 10 1")
        two = self.listing("fill screen 0 0 10 10 2")
        code, calls, out = self.run_display((td.displaylog.digest_text(one), td.displaylog.digest_text(two)), (one, two))
        self.assertEqual(code, 1)
        self.assertEqual(calls, [None, None, "4", "4"])
        self.assertIn("display digest line 3 (frame 1, tick 4)", out)
        self.assertIn("item at line 3 of A", out)

    def test_equal_digests_need_no_second_run(self):
        one = self.listing("fill screen 0 0 10 10 1")
        digest = td.displaylog.digest_text(one)
        code, calls, out = self.run_display((digest, digest), (one, one))
        self.assertEqual((code, calls), (0, [None, None]))
        self.assertIn("display digests identical", out)

    def test_a_given_tick_skips_the_digest_runs(self):
        one = self.listing("fill screen 0 0 10 10 1")
        code, calls, _ = self.run_display(("", ""), (one, one), display_tick=9)
        self.assertEqual((code, calls), (0, ["9", "9"]))


    def test_a_frame_of_tick_0_is_replayed_with_every_frame(self):
        one = self.listing("fill screen 0 0 10 10 1", tick=0)
        two = self.listing("fill screen 0 0 10 10 2", tick=0)
        code, calls, out = self.run_display((td.displaylog.digest_text(one), td.displaylog.digest_text(two)), (one, two))
        self.assertEqual(code, 1)
        self.assertEqual(calls, [None, None, "all", "all"])
        self.assertIn("item at line 3 of A", out)

    def test_a_given_tick_0_lists_every_frame(self):
        one = self.listing("fill screen 0 0 10 10 1", tick=0)
        code, calls, _ = self.run_display(("", ""), (one, one), display_tick=0)
        self.assertEqual((code, calls), (0, ["all", "all"]))

if __name__ == "__main__":
    unittest.main()
