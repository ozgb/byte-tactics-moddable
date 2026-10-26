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
            self.assertEqual(lines[1], "economy,*,,2,1,7,5,4,1,,0,")
            self.assertEqual(lines[2], "economy,Never,src/game/economy.cpp,1,0,0,0,0,0,?Never@@YAXXZ,0,")
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


class ReasonTest(unittest.TestCase):
    NEVER = ("Never", "src/game/economy.cpp", "?Never@@YAXXZ")

    def totals(self):
        return cov.summarize(EXPORT, KEY, NAME)[0]

    def written(self, totals):
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / "coverage.csv"
            cov.write_csv(totals, out)
            return out.read_text().splitlines(), cov.read_csv(out)

    def test_reason_is_carried_over_a_write(self):
        _, old = self.written({"economy": self.totals()["economy"]})
        old["economy"].reasons[self.NEVER] = "menu"
        with tempfile.TemporaryDirectory() as d:
            before = Path(d) / "old.csv"
            cov.write_csv(old, before)
            new = self.totals()
            cov.carry_reasons(new, cov.read_csv(before))
        lines, back = self.written(new)
        self.assertEqual(lines[2], "economy,Never,src/game/economy.cpp,1,0,0,0,0,0,?Never@@YAXXZ,0,menu")
        self.assertEqual(back["economy"].reasons, {self.NEVER: "menu"})

    def test_new_row_has_an_empty_reason(self):
        new = self.totals()
        cov.carry_reasons(new, {"economy": cov.Totals()})
        lines, back = self.written(new)
        self.assertEqual(lines[2], "economy,Never,src/game/economy.cpp,1,0,0,0,0,0,?Never@@YAXXZ,0,")
        self.assertEqual(back["economy"].reasons, {})

    def test_totals_and_dead_rows_have_no_reason(self):
        lines, _ = self.written(self.totals())
        self.assertTrue(lines[1].endswith(",0,"))

    def test_sorted_subsystem_row_without_a_reason_fails(self):
        t = cov.Totals(functions=1, never_entered=[("Send", "src/network/net.cpp", "?Send@@YAXXZ")])
        text = cov.unreasoned({"network": t})
        self.assertEqual(text, "coverage: 1 never-entered functions of files, game and network have no reason "
                               "in data/coverage.csv:\nnetwork Send (src/network/net.cpp)")
        t.reasons[t.never_entered[0]] = "network"
        self.assertEqual(cov.unreasoned({"network": t}), "")

    def test_unsorted_subsystem_without_a_reason_passes(self):
        self.assertEqual(cov.unreasoned(self.totals()), "")

    def test_reading_a_file_without_the_reason_column(self):
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / "coverage.csv"
            out.write_text("subsystem,function,file,functions,functions_entered,lines,lines_hit,branches,"
                           "branches_covered,decorated,dead\ngame,*,,1,0,0,0,0,0,,0\n"
                           "game,F,src/game/a.cpp,1,0,0,0,0,0,?F@@YAXXZ,0\n")
            t = cov.read_csv(out)["game"]
        self.assertEqual(t.never_entered, [("F", "src/game/a.cpp", "?F@@YAXXZ")])
        self.assertEqual(t.reasons, {})


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

    def test_anonymous_namespace_hash_is_left_out_of_the_csv(self):
        t = self.merged([
            entry("a.cpp:?Reset@?A0xFF389A01@@YAXXZ", "src/files/files_api.h", 0, [region(1, 1, 1, 9, 0)])])
        self.assertEqual(t.never_entered[0][2], "?Reset@?A0xFF389A01@@YAXXZ")
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / "coverage.csv"
            cov.write_csv({"files": t}, out)
            text = out.read_text()
        self.assertIn("?Reset@?A0x@@YAXXZ", text)
        self.assertNotIn("FF389A01", text)

    def test_short_anonymous_namespace_hash_is_left_out_of_the_csv(self):
        t = self.merged([
            entry("a.cpp:?Reset@?A0xF0F62@@YAXXZ", "src/files/files_api.h", 0, [region(1, 1, 1, 9, 0)])])
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / "coverage.csv"
            cov.write_csv({"files": t}, out)
            text = out.read_text()
        self.assertIn("?Reset@?A0x@@YAXXZ", text)
        self.assertNotIn("F0F62", text)

    def test_functions_less_entered_is_the_never_entered_rows(self):
        totals = cov.summarize(EXPORT, KEY, NAME)[0]
        totals["files"] = self.merged([static_copy("a.cpp", 0, [region(1, 1, 1, 9, 0)]),
                                       static_copy("b.cpp", 3, [region(1, 1, 1, 9, 3)])])
        for t in totals.values():
            self.assertEqual(t.functions - t.functions_entered, len(t.never_entered))


def inlined_export():
    return {"data": [{"functions": [
        entry("?Caller@@YAXXZ", "src/game/dead.cpp", 0, [region(1, 1, 3, 2, 0)], [[2, 3, 2, 7, 0, 0, 0, 0, 4]]),
        entry("?Inlined@@YAXXZ", "src/game/dead.cpp", 0, [region(5, 1, 6, 2, 0)], [[5, 3, 5, 7, 0, 0, 0, 0, 4]]),
        entry("?Orphan@@YAXXZ", "src/game/dead.cpp", 0, [region(8, 1, 9, 2, 0), region(9, 1, 9, 5, 0)],
              [[8, 3, 8, 7, 0, 0, 0, 0, 4]]),
        entry("dead.cpp:?Static@@YAXXZ", "src/game/dead.cpp", 0, [region(11, 1, 12, 2, 0)]),
        entry("?Ran@@YAXXZ", "src/game/dead.cpp", 3, [region(14, 1, 15, 2, 3)]),
    ]}]}


class DeadTest(unittest.TestCase):
    KEY = {("src/game/dead.cpp", name): "game" for name in ("Caller", "Inlined", "Orphan", "Static", "Ran")}
    LIVE = {"?Caller@@YAXXZ", "?Inlined@@YAXXZ", "dead.cpp_?Static@@YAXXZ"}

    def totals(self, live):
        return cov.summarize(inlined_export(), self.KEY, {}, live)[0]["game"]

    def test_counter_names(self):
        self.assertEqual(cov.counter_name("?Inlined@@YAXXZ"), "?Inlined@@YAXXZ")
        self.assertEqual(cov.counter_name("dead.cpp:?Static@@YAXXZ"), "dead.cpp_?Static@@YAXXZ")
        self.assertEqual(cov.counter_name("host_exit"), "host_exit")

    def test_function_inlined_into_a_live_one_is_live(self):
        t = self.totals({"?Caller@@YAXXZ", "?Inlined@@YAXXZ"})
        names = [n for n, _, _ in t.never_entered]
        self.assertIn("Inlined", names)
        self.assertNotIn("Inlined", [n for n, _, _ in t.dead_functions])

    def test_function_neither_linked_nor_inlined_is_dead(self):
        t = self.totals({"?Caller@@YAXXZ", "?Inlined@@YAXXZ"})
        self.assertEqual([n for n, _, _ in t.dead_functions], ["Orphan", "Static"])
        self.assertEqual(t.dead, 2)

    def test_static_copy_is_live_by_its_prefixed_counter(self):
        t = self.totals(self.LIVE)
        self.assertEqual([n for n, _, _ in t.dead_functions], ["Orphan"])
        self.assertIn("Static", [n for n, _, _ in t.never_entered])

    def test_entered_function_is_never_dead(self):
        t = self.totals(set())
        self.assertNotIn("Ran", [n for n, _, _ in t.dead_functions])
        self.assertEqual((t.functions, t.functions_entered), (1, 1))

    def test_dead_functions_are_left_out_of_the_totals(self):
        t = self.totals({"?Caller@@YAXXZ", "?Inlined@@YAXXZ"})
        self.assertEqual((t.functions, t.functions_entered), (3, 1))
        self.assertEqual((t.lines, t.lines_hit), (7, 2))
        self.assertEqual((t.branches, t.branches_covered), (4, 0))

    def test_without_live_counters_nothing_is_dead(self):
        t = cov.summarize(inlined_export(), self.KEY, {})[0]["game"]
        self.assertEqual((t.dead, t.functions), (0, 5))

    def test_copies_are_dead_only_when_all_are(self):
        export = {"data": [{"functions": [
            entry("a.cpp:?F@@YAXXZ", "src/game/dead.cpp", 0, [region(1, 1, 1, 9, 0)]),
            entry("b.cpp:?F@@YAXXZ", "src/game/dead.cpp", 0, [region(1, 1, 1, 9, 0)])]}]}
        key = {("src/game/dead.cpp", "F"): "game"}
        self.assertEqual(cov.summarize(export, key, {}, {"b.cpp_?F@@YAXXZ"})[0]["game"].dead, 0)
        self.assertEqual(cov.summarize(export, key, {}, set())[0]["game"].dead, 1)

    def test_csv_round_trip_with_dead_rows(self):
        totals = {"game": self.totals({"?Caller@@YAXXZ", "?Inlined@@YAXXZ"})}
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / "coverage.csv"
            cov.write_csv(totals, out)
            lines = out.read_text().splitlines()
            self.assertEqual(lines[1], "game,*,,3,1,7,2,4,0,,2,")
            self.assertEqual(lines[-2], "game,Orphan,src/game/dead.cpp,0,0,0,0,0,0,?Orphan@@YAXXZ,1,")
            self.assertEqual(lines[-1], "game,Static,src/game/dead.cpp,0,0,0,0,0,0,?Static@@YAXXZ,1,")
            self.assertEqual(cov.read_csv(out), totals)

    def test_reading_a_file_without_the_dead_column(self):
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / "coverage.csv"
            out.write_text("subsystem,function,file,functions,functions_entered,lines,lines_hit,branches,"
                           "branches_covered,decorated\ngame,*,,1,0,0,0,0,0,\n")
            self.assertEqual(cov.read_csv(out)["game"].dead, 0)

    def test_table_has_a_dead_column(self):
        text = cov.table({"game": self.totals({"?Caller@@YAXXZ"})})
        self.assertEqual(text.splitlines()[0].split()[-1], "dead")

    def test_live_counters_file(self):
        with tempfile.TemporaryDirectory() as d:
            out = Path(d) / cov.LIVE_COUNTERS
            out.write_text("?A@@YAXXZ\nfile.cpp_?B@@YAXXZ\n")
            self.assertEqual(cov.read_live_counters(out), {"?A@@YAXXZ", "file.cpp_?B@@YAXXZ"})


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


class PlaySuiteSaverTest(unittest.TestCase):
    def test_a_named_loader_is_played_with_its_saver(self):
        played = {}

        def fake_play_all(exe, scenarios, game_dir, parallel, failed=None, retry_any=False):
            played.update(scenarios)

        with tempfile.TemporaryDirectory() as tmp:
            every = {n: Path(tmp) / f"{n}.txt" for n in ("plain", "saver", "loader")}
            every["plain"].write_text("ticks 5\n")
            every["saver"].write_text("ticks 5\nsave 2\n")
            every["loader"].write_text("ticks 5\nload saver\n")
            saved = (cov.RUN_DIR, cov.reference.play_all, cov.reference.scenario_files, cov.subprocess.run)
            cov.RUN_DIR = Path(tmp) / "run"
            cov.reference.play_all = fake_play_all
            cov.reference.scenario_files = lambda: every
            cov.subprocess.run = lambda *a, **k: None
            try:
                with self.assertRaises(SystemExit):
                    cov.play_suite(Path("TotalA.exe"), None, 1, ["loader"])
            finally:
                cov.RUN_DIR, cov.reference.play_all, cov.reference.scenario_files, cov.subprocess.run = saved
        self.assertEqual(sorted(played), ["loader", "saver"])


class WriteNothingTest(unittest.TestCase):
    def test_write_refuses_an_export_without_game_functions(self):
        with tempfile.TemporaryDirectory() as d:
            exe = Path(d) / "TotalA.exe"
            exe.write_bytes(b"")
            (Path(d) / cov.LIVE_COUNTERS).write_text("")
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
