"""Tests for coverage.py, on a hand-written llvm-cov export and subsystems rows.

    uv run tools/test_coverage.py
"""

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
        region(10, 1, 14, 2, 4), region(12, 5, 12, 20, 0)], [[11, 5, 11, 9, 3, 0, 0, 0, 4]]),
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
        fn = cov.parse_function(EXPORT["data"][0]["functions"][2])[1]
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
        self.assertEqual((t.lines, t.lines_hit), (7, 4))
        self.assertEqual((t.branches, t.branches_covered), (4, 1))

    def test_line_counts_use_the_innermost_region(self):
        counts = cov.region_lines([region(10, 1, 14, 2, 4), region(12, 5, 12, 20, 0)], {0})
        self.assertEqual(counts, {10: 4, 11: 4, 12: 0, 13: 4, 14: 4})

    def test_never_entered(self):
        self.assertEqual(self.totals["economy"].never_entered, [("Never", "src/game/economy.cpp")])
        self.assertEqual(self.totals["units"].never_entered, [])

    def test_overload_entered_counts_as_entered(self):
        self.assertEqual(self.totals["units"].functions_entered, 2)
        self.assertEqual(self.totals["units"].never_entered, [])

    def test_csv_round_trip(self):
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / "coverage.csv"
            cov.write_csv(self.totals, out)
            lines = out.read_text().splitlines()
            self.assertEqual(lines[0], ",".join(cov.COLUMNS))
            self.assertEqual(lines[1], "economy,*,,2,1,7,4,4,1")
            self.assertEqual(lines[2], "economy,Never,src/game/economy.cpp,1,0,0,0,0,0")
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


class PlaySuiteTest(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
