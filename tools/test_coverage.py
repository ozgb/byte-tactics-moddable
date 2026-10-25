"""Tests for coverage.py, on a hand-written llvm-cov export and subsystems rows.

    uv run tools/test_coverage.py
"""

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

import coverage as cov

ROOT = cov.ROOT


def path(rel: str) -> str:
    return str(ROOT / rel)


def region(first, col, last, end_col, count, file_id=0, kind=0):
    return [first, col, last, end_col, count, file_id, 0, kind]


def entry(name, rel, count, regions, branches=()):
    return {"name": name, "filenames": [path(rel)], "count": count, "regions": regions, "branches": list(branches)}


EXPORT = {"data": [{"functions": [
    entry("?Reset@UnitResources@@QAEXXZ", "src/game/economy.cpp", 4, [
        region(10, 1, 14, 2, 4), region(12, 5, 12, 20, 0)], [[11, 5, 11, 9, 3, 0, 0, 9, 4]]),
    entry("?Never@@YAXXZ", "src/game/economy.cpp", 0, [region(20, 1, 21, 2, 0)], [[20, 3, 20, 7, 0, 0, 0, 0, 4]]),
    entry("?Moved@@YAXXZ", "src/other/elsewhere.cpp", 2, [region(1, 1, 2, 2, 2)]),
    entry("?Lost@@YAXXZ", "src/other/lost.cpp", 1, [region(1, 1, 1, 9, 1)]),
    entry("?Host@@YAXXZ", "src/harness/run.cpp", 1, [region(1, 1, 1, 9, 1)]),
    {"name": "?Runtime@@YAXXZ", "filenames": ["/usr/lib/llvm/profile.c"], "count": 1, "regions": [], "branches": []},
    entry("?Move@Unit@@QAEXXZ", "src/units/unit.cpp", 0, [region(5, 1, 6, 2, 0)]),
    entry("?Move@Unit@@QAEXH@Z", "src/units/unit.cpp", 7, [region(8, 1, 9, 2, 7)]),
]}]}

KEY = {("src/game/economy.cpp", "UnitResources::Reset"): "economy",
       ("src/game/economy.cpp", "Never"): "economy",
       ("src/units/unit.cpp", "Unit::Move"): "units",
       ("src/units/other.cpp", "Moved"): "units"}
NAME = {"UnitResources::Reset": {"economy"}, "Never": {"economy"}, "Unit::Move": {"units"}, "Moved": {"units"}}


class Coverage(unittest.TestCase):
    def setUp(self):
        self.totals, self.excluded, self.unassigned = cov.summarize(EXPORT, KEY, NAME)

    def test_assignment_by_file_and_name(self):
        self.assertEqual(self.totals["economy"].functions, 2)
        self.assertEqual(self.totals["units"].functions, 3)

    def test_name_alone_when_the_file_has_no_row(self):
        self.assertIn("units", self.totals)
        fn = cov.parse_function(EXPORT["data"][0]["functions"][2])
        self.assertEqual(cov.subsystem_of(fn, KEY, NAME), "units")

    def test_unlisted_function_takes_the_rule_of_its_file(self):
        self.assertEqual(self.unassigned, 0)
        self.assertEqual(self.totals["game"].functions, 1)
        fn = cov.Function("Unlisted", "src/ai/pathfind.cpp", True, 0, 0, 0, 0)
        self.assertEqual(cov.subsystem_of(fn, KEY, NAME), "pathfinding")

    def test_state_files_without_a_row(self):
        for file in ("interfaces/ref.h", "src/state/sim_allocator.h", "src/state/block.cpp"):
            fn = cov.Function("Ref::operator?C", file, True, 0, 0, 0, 0)
            self.assertEqual(cov.subsystem_of(fn, KEY, NAME), "state")

    def test_plain_names_of_special_members(self):
        self.assertEqual(cov.plain_name("??0Pathfinder@@QAE@XZ"), "Pathfinder::Pathfinder")
        self.assertEqual(cov.plain_name("??1TdfRecord@@QAE@XZ"), "TdfRecord::~TdfRecord")
        self.assertEqual(cov.plain_name("??C?$Ref@VMission@@@@QBEPAVMission@@XZ"), "Ref::operator?C")
        self.assertEqual(cov.plain_name("??HVec3@@QBE?AV0@ABV0@@Z"), "Vec3::operator?H")
        self.assertEqual(cov.plain_name("?Reset@UnitResources@@QAEXXZ"), "UnitResources::Reset")

    def test_constructor_matched_after_renaming(self):
        export = {"data": [{"functions": [entry("??0Pathfinder@@QAE@XZ", "src/ai/pathfind.cpp", 1, [region(1, 1, 2, 2, 1)])]}]}
        key = {("src/ai/pathfind.cpp", "Pathfinder::Pathfinder"): "units"}
        totals, _, unassigned = cov.summarize(export, key, {})
        self.assertEqual(list(totals), ["units"])
        self.assertEqual(unassigned, 0)

    def test_exclusions(self):
        self.assertEqual(self.excluded, 2)
        self.assertIsNone(cov.repo_path("/usr/lib/llvm/profile.c"))
        self.assertIsNone(cov.repo_path(path("src/harness/run.cpp")))
        self.assertEqual(cov.repo_path(path("src/game/economy.cpp")), "src/game/economy.cpp")

    def test_sums(self):
        t = self.totals["economy"]
        self.assertEqual((t.functions, t.functions_entered), (2, 1))
        self.assertEqual((t.lines, t.lines_hit), (7, 5))
        self.assertEqual((t.branches, t.branches_covered), (4, 1))

    def test_branch_of_another_file_is_left_out(self):
        function = entry("?Reset@UnitResources@@QAEXXZ", "src/game/economy.cpp", 1, [region(1, 1, 2, 2, 1)],
                         [[1, 1, 1, 2, 1, 1, 3, 0, 4], [1, 3, 1, 4, 1, 0, 0, 3, 4]])
        self.assertEqual(cov.parse_function(function).branch_sides, {(1, 3, 1, 4): (True, False)})

    def test_line_counts_take_the_larger_count(self):
        counts = cov.region_lines([region(10, 1, 14, 2, 4), region(12, 5, 12, 20, 0)], {0})
        self.assertEqual(counts, {10: 4, 11: 4, 12: 4, 13: 4, 14: 4})

    def test_rare_branch_on_one_line_of_an_executed_body(self):
        counts = cov.region_lines([region(1, 1, 5, 2, 4), region(3, 5, 3, 20, 0)], {0})
        self.assertEqual(counts[3], 4)
        counts = cov.region_lines([region(1, 1, 5, 2, 0), region(3, 5, 3, 20, 2)], {0})
        self.assertEqual(counts[3], 2)

    def test_never_entered(self):
        self.assertEqual(self.totals["economy"].never_entered, [("Never", "src/game/economy.cpp", "?Never@@YAXXZ")])
        self.assertEqual(self.totals["units"].never_entered,
                         [("Unit::Move", "src/units/unit.cpp", "?Move@Unit@@QAEXXZ")])

    def test_overloads_count_separately(self):
        self.assertEqual(self.totals["units"].functions_entered, 2)
        self.assertEqual(self.totals["units"].functions, 3)

    def test_csv_round_trip(self):
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / "coverage.csv"
            cov.write_csv(self.totals, out)
            lines = out.read_text().splitlines()
            self.assertEqual(lines[0], ",".join(cov.COLUMNS))
            self.assertEqual(lines[1], "economy,*,,2,1,7,5,4,1,")
            self.assertEqual(lines[2], "economy,Never,src/game/economy.cpp,1,0,0,0,0,0,?Never@@YAXXZ")
            back = cov.read_csv(out)
        self.assertEqual(back.keys(), self.totals.keys())
        for name, t in self.totals.items():
            self.assertEqual(back[name], t)

    def test_check_detects_a_fall(self):
        worse = cov.summarize(EXPORT, KEY, NAME)[0]
        worse["economy"].lines_hit -= 1
        worse["units"].functions_entered -= 1
        worse["economy"].branches += 2
        falls, notes = cov.compare(self.totals, worse)
        self.assertEqual(len(falls), 3)
        self.assertTrue(any(f.startswith("economy lines") for f in falls))
        self.assertTrue(any(f.startswith("units functions") for f in falls))
        self.assertTrue(any(f.startswith("economy branches") for f in falls))
        self.assertEqual(notes, [])

    def test_check_allows_a_rise_and_new_subsystems(self):
        better = cov.summarize(EXPORT, KEY, NAME)[0]
        better["economy"].lines_hit += 1
        better["extra"] = cov.Totals(functions=1)
        falls, notes = cov.compare(self.totals, better)
        self.assertEqual(falls, [])
        self.assertEqual(len(notes), 2)

    def test_check_fails_a_vanished_subsystem(self):
        falls, _ = cov.compare(self.totals, {})
        self.assertTrue(falls)

    def test_table(self):
        text = cov.table(self.totals)
        self.assertIn("economy", text)
        self.assertIn("50.0%", text)


def static_copy(tu, count, regions, branches=()):
    name = "%s:?HAPI_Length@@YGJPAX@Z" % tu
    return entry(name, "src/files/files_api.h", count, regions, branches)


class MergeTest(unittest.TestCase):
    def merged(self, functions):
        key = {("src/files/files_api.h", "HAPI_Length"): "files"}
        return cov.summarize({"data": [{"functions": functions}]}, key, {})[0]["files"]

    def test_two_copies_of_a_static_function_are_one(self):
        t = self.merged([
            static_copy("a.cpp", 0, [region(1, 1, 3, 2, 0), region(2, 1, 2, 9, 4)], [[2, 3, 2, 7, 0, 0, 0, 0, 4], [3, 3, 3, 7, 0, 0, 0, 0, 4]]),
            static_copy("b.cpp", 5, [region(1, 1, 3, 2, 5), region(2, 1, 2, 9, 0)],
                        [[2, 3, 2, 7, 5, 0, 0, 0, 4]])])
        self.assertEqual((t.functions, t.functions_entered), (1, 1))
        self.assertEqual((t.lines, t.lines_hit), (3, 3))
        self.assertEqual((t.branches, t.branches_covered), (4, 1))
        self.assertEqual(t.never_entered, [])

    def test_branch_sides_merge_by_position(self):
        t = self.merged([
            static_copy("a.cpp", 1, [region(1, 1, 1, 9, 1)], [[2, 3, 2, 7, 1, 0, 0, 0, 4]]),
            static_copy("b.cpp", 1, [region(1, 1, 1, 9, 1)], [[2, 3, 2, 7, 0, 2, 0, 0, 4]])])
        self.assertEqual((t.branches, t.branches_covered), (2, 2))

    def test_copies_never_entered_give_one_row(self):
        t = self.merged([static_copy("a.cpp", 0, [region(1, 1, 1, 9, 0)]),
                         static_copy("b.cpp", 0, [region(1, 1, 1, 9, 0)])])
        self.assertEqual((t.functions, t.functions_entered), (1, 0))
        self.assertEqual(t.never_entered, [("HAPI_Length", "src/files/files_api.h", "?HAPI_Length@@YGJPAX@Z")])

    def test_overloads_stay_separate(self):
        t = self.merged([
            entry("a.cpp:?HAPI_Length@@YGJPAX@Z", "src/files/files_api.h", 1, [region(1, 1, 1, 9, 1)]),
            entry("a.cpp:?HAPI_Length@@YGJPAUFileHandle@@@Z", "src/files/files_api.h", 0, [region(5, 1, 5, 9, 0)])])
        self.assertEqual((t.functions, t.functions_entered), (2, 1))
        self.assertEqual(len(t.never_entered), 1)

    def test_never_entered_overloads_give_two_distinguishable_rows(self):
        t = self.merged([
            entry("a.cpp:?HAPI_Length@@YGJPAX@Z", "src/files/files_api.h", 0, [region(1, 1, 1, 9, 0)]),
            entry("a.cpp:?HAPI_Length@@YGJPAUFileHandle@@@Z", "src/files/files_api.h", 0, [region(5, 1, 5, 9, 0)])])
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / "coverage.csv"
            cov.write_csv({"files": t}, out)
            rows = out.read_text().splitlines()[2:]
            self.assertEqual(len(rows), 2)
            self.assertEqual(len(set(rows)), 2)
            self.assertEqual(cov.read_csv(out)["files"], t)

    def test_functions_less_entered_is_the_never_entered_rows(self):
        totals = cov.summarize(EXPORT, KEY, NAME)[0]
        totals["files"] = self.merged([static_copy("a.cpp", 0, [region(1, 1, 1, 9, 0)]),
                                       static_copy("b.cpp", 3, [region(1, 1, 1, 9, 3)])])
        for t in totals.values():
            self.assertEqual(t.functions - t.functions_entered, len(t.never_entered))


class PlaySuiteTest(unittest.TestCase):
    def test_attempt_clears_the_scenarios_old_profiles(self):
        seen = []

        def fake_play(exe, scenario, game_dir, dump, timeout=None, poison=False, blockscan=0, env=None):
            seen.append(sorted(p.name for p in (run_dir / scenario.stem).glob("*.profraw")))
            (run_dir / scenario.stem / ("cov-%d.profraw" % len(seen))).write_bytes(b"x")
            return Path()

        def fake_play_all(exe, scenarios, game_dir, parallel, failed=None, retry_any=False):
            for name, path in scenarios.items():
                cov.reference.play_scenario(exe, path, game_dir)
                cov.tracediff.play(exe, path, game_dir, None)

        merged = []
        with tempfile.TemporaryDirectory() as tmp:
            run_dir = Path(tmp) / "run"
            saved = (cov.RUN_DIR, cov.tracediff.play, cov.reference.play_scenario, cov.reference.play_all,
                     cov.reference.scenario_files, cov.subprocess.run)
            cov.RUN_DIR = run_dir
            cov.tracediff.play = fake_play
            cov.reference.play_all = fake_play_all
            cov.reference.scenario_files = lambda: {"alpha": Path("alpha.txt")}
            cov.subprocess.run = lambda cmd, **k: merged.append(cmd)
            try:
                cov.reference.play_scenario = lambda exe, scenario, *a, **k: cov.tracediff.play(exe, scenario, None, None)
                cov.play_suite(Path("TotalA.exe"), None, 1, None)
            finally:
                (cov.RUN_DIR, cov.tracediff.play, cov.reference.play_scenario, cov.reference.play_all,
                 cov.reference.scenario_files, cov.subprocess.run) = saved
        self.assertEqual(seen, [[], []])
        self.assertEqual([Path(p).name for p in merged[0][5:]], ["cov-2.profraw"])

    def test_profile_path_reaches_tracediff_play(self):
        seen = {}

        def fake_play(exe, scenario, game_dir, dump, timeout=None, poison=False, blockscan=0, env=None):
            seen[scenario.stem] = env
            (run_dir / scenario.stem / "cov-1.profraw").write_bytes(b"x")
            return Path()

        def fake_play_all(exe, scenarios, game_dir, parallel, failed=None, retry_any=False):
            for name, path in scenarios.items():
                cov.reference.play_scenario(exe, path, game_dir)

        with tempfile.TemporaryDirectory() as tmp:
            run_dir = Path(tmp) / "run"
            saved = (cov.RUN_DIR, cov.tracediff.play, cov.reference.play_scenario, cov.reference.play_all,
                     cov.reference.scenario_files, cov.subprocess.run)
            cov.RUN_DIR = run_dir
            cov.tracediff.play = fake_play
            cov.reference.play_all = fake_play_all
            cov.reference.scenario_files = lambda: {"alpha": Path("alpha.txt")}
            cov.subprocess.run = lambda *a, **k: None
            original_play_scenario = cov.reference.play_scenario
            try:
                cov.reference.play_scenario = lambda exe, scenario, *a, **k: cov.tracediff.play(exe, scenario, None, None)
                wrapped_target = cov.reference.play_scenario
                cov.play_suite(Path("TotalA.exe"), None, 1, None)
                self.assertIs(cov.reference.play_scenario, wrapped_target)
                self.assertIs(cov.tracediff.play, fake_play)
            finally:
                (cov.RUN_DIR, cov.tracediff.play, cov.reference.play_scenario, cov.reference.play_all,
                 cov.reference.scenario_files, cov.subprocess.run) = saved
                cov.reference.play_scenario = original_play_scenario
        self.assertEqual(seen["alpha"]["LLVM_PROFILE_FILE"], cov.windows_path(run_dir / "alpha") + "\\cov-%p.profraw")


class WriteNothingTest(unittest.TestCase):
    def test_write_refuses_an_export_without_game_functions(self):
        with tempfile.TemporaryDirectory() as d:
            exe = Path(d) / "TotalA.exe"
            exe.write_bytes(b"")
            saved = (cov.export, cov.read_subsystems, cov.OUT, sys.argv)
            cov.export = lambda exe, profdata: {"data": [{"functions": []}]}
            cov.read_subsystems = lambda: ({}, {})
            cov.OUT = Path(d) / "coverage.csv"
            sys.argv = ["coverage.py", "--exe", str(exe), "--from", "x.profdata", "--write"]
            try:
                with contextlib.redirect_stdout(io.StringIO()), self.assertRaises(SystemExit) as raised:
                    cov.main()
            finally:
                cov.export, cov.read_subsystems, cov.OUT, sys.argv = saved
            self.assertIn("empty baseline", str(raised.exception))
            self.assertFalse((Path(d) / "coverage.csv").exists())


if __name__ == "__main__":
    unittest.main()
