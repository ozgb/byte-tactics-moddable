"""Tests for seamlink.py, on hand-written units and temporary trees.

    uv run tools/test_seamlink.py
"""

import tempfile
import unittest
from pathlib import Path

import seamlink
from seamlink import Unit

SUBSYSTEMS = """function,file,subsystem,rule,override
Tick,src/map/wind.cpp,wind_meteors,x,
Helper,src/map/wind.cpp,wind_meteors,x,
Spend,src/game/economy.cpp,economy,x,
Mixed,src/game/mixed.cpp,economy,x,
Other,src/game/mixed.cpp,units,x,
Foreign,src/units/units.cpp,units,x,
"""
HEADER = """#ifndef W_H
#define W_H
// a comment mentioning Hidden(x);
struct WindSeam {
    void (__cdecl* Update)(void);
};
int __cdecl RandomInt(int n);
extern int g_extra;
#endif
"""


def sym(name, args="XXZ"):
    return f"?{name}@@YA{args}"


class SeamLinkTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.write("data/subsystems.csv", SUBSYSTEMS)
        self.write("interfaces/wind_meteors.h", HEADER)
        self.write("interfaces/seams.h", "struct Seams { int x; };\nextern Seams g_seams;\nvoid InstallNativeSeams();\n")
        self.write("interfaces/ref.h", "bool __cdecl SimOwns(const void* p);\n")
        self.write("lib/bitexact/bitexact.h", "double bx_sin(double x);\n")

    def write(self, path, text):
        p = self.root / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text)

    def wind(self, *targets, code=(), data=()):
        unit = Unit("src/map/wind.cpp", code={"?Tick@@YAXXZ", "?Helper@@YAXXZ", *code}, data=set(data))
        unit.refs = [("?Tick@@YAXXZ", t) for t in targets]
        return unit

    def run_with(self, *units):
        return seamlink.violations(list(units), self.root)

    def test_seam_table_is_allowed(self):
        self.assertEqual(self.run_with(self.wind("?g_seams@@3USeams@@A")), [])

    def test_state_block_file_and_declaration_are_allowed(self):
        state = Unit("src/state/block.cpp", data={"?g_block@@3HA"})
        state.code.add("?Accessor@@YAXXZ")
        self.assertEqual(self.run_with(self.wind("?g_block@@3HA", "?Accessor@@YAXXZ", "?SimOwns@@YA_NPBX@Z"), state), [])

    def test_engine_service_in_own_header_is_allowed(self):
        other = Unit("src/util/rand.cpp", code={"_RandomInt"}, data={"?g_extra@@3HA"})
        self.assertEqual(self.run_with(self.wind("_RandomInt", "?g_extra@@3HA"), other), [])

    def test_engine_service_of_another_subsystem_is_not_allowed(self):
        self.write("interfaces/economy.h", "int __cdecl Spend2(int n);\n")
        other = Unit("src/util/x.cpp", code={"_Spend2"})
        found = self.run_with(self.wind("_Spend2"), other)
        self.assertEqual([v.symbol for v in found], ["Spend2"])

    def test_declaration_inside_the_seam_struct_is_not_a_service(self):
        other = Unit("src/util/x.cpp", code={"_Update"})
        self.assertEqual([v.symbol for v in self.run_with(self.wind("_Update"), other)], ["Update"])

    def test_commented_declaration_is_not_a_service(self):
        other = Unit("src/util/x.cpp", code={"_Hidden"})
        self.assertEqual(len(self.run_with(self.wind("_Hidden"), other)), 1)

    def test_bitexact_is_allowed(self):
        self.assertEqual(self.run_with(self.wind("_bx_sin", "_bx_other")), [])

    def test_library_symbol_nobody_defines_is_allowed(self):
        self.assertEqual(self.run_with(self.wind("_memcpy", "__imp__GetTickCount@0")), [])

    def test_own_function_is_allowed(self):
        self.assertEqual(self.run_with(self.wind("?Helper@@YAXXZ")), [])

    def test_own_data_in_sole_file_is_allowed(self):
        self.assertEqual(self.run_with(self.wind("?local@@3HA", data={"?local@@3HA"})), [])

    def test_compiler_made_symbol_of_shared_file_is_allowed(self):
        mixed = Unit("src/game/mixed.cpp", data={"??_C@_03AB@abc?$AA@"})
        mixed.refs = [("?Mixed@@YAXXZ", "??_C@_03AB@abc?$AA@")]
        self.assertEqual(self.run_with(mixed), [])

    def test_call_to_other_subsystem_is_code_violation(self):
        foreign = Unit("src/units/units.cpp", code={"?Foreign@@YAXXZ"})
        found = self.run_with(self.wind("?Foreign@@YAXXZ"), foreign)
        self.assertEqual([v.line() for v in found], ["wind_meteors Tick -> Foreign (code, units)"])

    def test_call_to_other_function_in_same_file_is_violation(self):
        mixed = Unit("src/game/mixed.cpp", code={"?Mixed@@YAXXZ", "?Other@@YAXXZ"})
        mixed.refs = [("?Mixed@@YAXXZ", "?Other@@YAXXZ")]
        found = self.run_with(mixed)
        self.assertEqual([v.line() for v in found], ["economy Mixed -> Other (code, units)"])

    def test_global_outside_own_files_is_data_violation(self):
        link = Unit("link/data.cpp", data={"?g_game@@3HA"})
        found = self.run_with(self.wind("?g_game@@3HA"), link)
        self.assertEqual([v.line() for v in found], ["wind_meteors Tick -> g_game (data, link/data.cpp)"])

    def test_data_of_shared_file_is_a_violation(self):
        mixed = Unit("src/game/mixed.cpp", data={"?g_shared@@3HA"})
        found = self.run_with(self.wind("?g_shared@@3HA"), mixed)
        self.assertEqual([(v.kind, v.owner) for v in found], [("data", "src/game/mixed.cpp")])

    def test_function_of_a_non_simulation_subsystem_is_not_checked(self):
        unit = Unit("src/gui/x.cpp")
        unit.refs = [("?Draw@@YAXXZ", "?g_game@@3HA")]
        self.assertEqual(self.run_with(unit), [])

    def test_duplicate_references_report_once(self):
        link = Unit("link/data.cpp", data={"?g_game@@3HA"})
        self.assertEqual(len(self.run_with(self.wind("?g_game@@3HA", "?g_game@@3HA"), link)), 1)

    def test_check_passes_with_row_and_fails_without(self):
        link = Unit("link/data.cpp", data={"?g_game@@3HA"})
        vs = self.run_with(self.wind("?g_game@@3HA"), link)
        row = ("wind_meteors", "Tick", "g_game", "TODO(#400): move into the block")
        self.assertEqual(seamlink.check(vs, [row]), [])
        self.assertEqual(len(seamlink.check(vs, [])), 1)

    def test_stale_row_fails(self):
        row = ("wind_meteors", "Tick", "g_game", "TODO(#400): move into the block")
        problems = seamlink.check([], [row])
        self.assertEqual(len(problems), 1)
        self.assertIn("stale", problems[0])

    def test_reason_must_name_an_issue(self):
        link = Unit("link/data.cpp", data={"?g_game@@3HA"})
        vs = self.run_with(self.wind("?g_game@@3HA"), link)
        self.assertEqual(len(seamlink.check(vs, [("wind_meteors", "Tick", "g_game", "later")])), 1)

    def test_rows_round_trip_sorted(self):
        text = seamlink.render([("b", "f", "s", "TODO(#1): x"), ("a", "f, g", "s", "TODO(#2): y")])
        self.write(seamlink.EXCEPTIONS, text)
        self.assertEqual(seamlink.read_exceptions(self.root), [("a", "f, g", "s", "TODO(#2): y"), ("b", "f", "s", "TODO(#1): x")])

    def test_summary_counts_kinds(self):
        link = Unit("link/data.cpp", data={"?g_game@@3HA"})
        foreign = Unit("src/units/units.cpp", code={"?Foreign@@YAXXZ"})
        text = seamlink.summary(self.run_with(self.wind("?g_game@@3HA", "?Foreign@@YAXXZ"), link, foreign))
        self.assertIn("wind_meteors: 2 violations (1 code, 1 data)", text)


if __name__ == "__main__":
    unittest.main()
