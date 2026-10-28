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
#endif
"""
SERVICES = """#ifndef S_H
#define S_H
int __stdcall RandomInt(int n);
extern int g_extra;
// Reader.
class Reader {
public:
    int pos;
    Reader();
    ~Reader();
    int GetInt(int key) { return key + pos; }
    int Seek(int to);
};
#endif
"""
HOST = """#ifndef H_H
#define H_H
#ifdef __cplusplus
extern "C" {
#endif
void __cdecl host_fatal(void* owner, const char* title, const char* text);
#ifdef __cplusplus
}
#endif
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
        self.write("interfaces/services.h", SERVICES)
        self.write("interfaces/host.h", HOST)
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

    def test_engine_service_is_allowed_for_every_subsystem(self):
        other = Unit("src/util/rand.cpp", code={"_RandomInt@4"}, data={"?g_extra@@3HA"})
        self.assertEqual(self.run_with(self.wind("_RandomInt@4", "?g_extra@@3HA"), other), [])
        spend = Unit("src/game/economy.cpp", code={"?Spend@@YAXXZ"})
        spend.refs = [("?Spend@@YAXXZ", "_RandomInt@4")]
        self.assertEqual(self.run_with(spend, other), [])

    def test_members_of_a_service_class_are_allowed(self):
        reader = Unit("src/util/reader.cpp", code={"??0Reader@@QAE@XZ", "??1Reader@@QAE@XZ", "?Seek@Reader@@QAEHH@Z", "?GetInt@Reader@@QAEHH@Z", "?Other@Reader@@QAEHH@Z"})
        found = self.run_with(self.wind("??0Reader@@QAE@XZ", "??1Reader@@QAE@XZ", "?Seek@Reader@@QAEHH@Z",
                                        "?GetInt@Reader@@QAEHH@Z", "?Other@Reader@@QAEHH@Z"), reader)
        self.assertEqual([v.symbol for v in found], ["Reader::Other"])

    def test_member_of_another_class_with_a_service_member_name_is_not_allowed(self):
        stranger = Unit("src/util/stranger.cpp", code={"?GetInt@Stranger@@QAEHH@Z"})
        found = self.run_with(self.wind("?GetInt@Stranger@@QAEHH@Z"), stranger)
        self.assertEqual([v.symbol for v in found], ["Stranger::GetInt"])

    def test_host_declaration_is_allowed_as_host(self):
        platform = Unit("src/platform/host.cpp", code={"_host_fatal"})
        self.assertEqual(self.run_with(self.wind("_host_fatal"), platform), [])
        rules = seamlink.build_rules(self.root, [platform])
        self.assertEqual(seamlink.classify(rules, "wind_meteors", "_host_fatal"), "host")

    def test_subsystem_header_declaration_is_not_a_service(self):
        self.write("interfaces/wind_meteors.h", HEADER + "int __cdecl Spend2(int n);\n")
        other = Unit("src/util/x.cpp", code={"_Spend2"})
        self.assertEqual([v.symbol for v in self.run_with(self.wind("_Spend2"), other)], ["Spend2"])

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
        link = Unit("link/data.cpp", data={"?g_foo@@3HA"})
        found = self.run_with(self.wind("?g_foo@@3HA"), link)
        self.assertEqual([v.line() for v in found], ["wind_meteors Tick -> g_foo (data, link/data.cpp)"])

    def test_data_of_shared_file_is_a_violation(self):
        mixed = Unit("src/game/mixed.cpp", data={"?g_shared@@3HA"})
        found = self.run_with(self.wind("?g_shared@@3HA"), mixed)
        self.assertEqual([(v.kind, v.owner) for v in found], [("data", "src/game/mixed.cpp")])

    def test_function_of_a_non_simulation_subsystem_is_not_checked(self):
        unit = Unit("src/gui/x.cpp")
        unit.refs = [("?Draw@@YAXXZ", "?g_foo@@3HA")]
        self.assertEqual(self.run_with(unit), [])

    def test_duplicate_references_report_once(self):
        link = Unit("link/data.cpp", data={"?g_foo@@3HA"})
        self.assertEqual(len(self.run_with(self.wind("?g_foo@@3HA", "?g_foo@@3HA"), link)), 1)

    def test_check_passes_with_row_and_fails_without(self):
        link = Unit("link/data.cpp", data={"?g_foo@@3HA"})
        vs = self.run_with(self.wind("?g_foo@@3HA"), link)
        row = ("wind_meteors", "Tick", "g_foo", "TODO(#400): move into the block")
        self.assertEqual(seamlink.check(vs, [row]), [])
        self.assertEqual(len(seamlink.check(vs, [])), 1)

    def test_stale_row_fails(self):
        row = ("wind_meteors", "Tick", "g_foo", "TODO(#400): move into the block")
        problems = seamlink.check([], [row])
        self.assertEqual(len(problems), 1)
        self.assertIn("stale", problems[0])

    def test_reason_must_name_an_issue(self):
        link = Unit("link/data.cpp", data={"?g_foo@@3HA"})
        vs = self.run_with(self.wind("?g_foo@@3HA"), link)
        self.assertEqual(len(seamlink.check(vs, [("wind_meteors", "Tick", "g_foo", "later")])), 1)

    def test_rows_round_trip_sorted(self):
        text = seamlink.render([("b", "f", "s", "TODO(#1): x"), ("a", "f, g", "s", "TODO(#2): y")])
        self.write(seamlink.EXCEPTIONS, text)
        self.assertEqual(seamlink.read_exceptions(self.root), [("a", "f, g", "s", "TODO(#2): y"), ("b", "f", "s", "TODO(#1): x")])

    def test_g_game_is_allowed_wherever_defined(self):
        link = Unit("link/data.cpp", data={"?g_game@@3PAVGame@@A"})
        self.assertEqual(self.run_with(self.wind("?g_game@@3PAVGame@@A"), link), [])

    def test_const_and_sim_classes_are_allowed_and_others_are_not(self):
        self.write("data/state_classes.csv", "name,class\nk_table,const\ng_input,sim\ng_view,view\ng_mix,mixed\n")
        link = Unit("link/data.cpp", data={"?k_table@@3HA", "?g_input@@3HA", "?g_view@@3HA", "?g_mix@@3HA"})
        found = self.run_with(self.wind("?k_table@@3HA", "?g_input@@3HA", "?g_view@@3HA", "?g_mix@@3HA"), link)
        self.assertEqual([v.symbol for v in found], ["g_mix", "g_view"])

    def test_const_rule_does_not_allow_code(self):
        self.write("data/state_classes.csv", "name,class\nFoo,const\n")
        other = Unit("src/units/units.cpp", code={"?Foo@@YAXXZ"})
        self.assertEqual(len(self.run_with(self.wind("?Foo@@YAXXZ"), other)), 1)

    def test_operator_new_file_is_library(self):
        lib = Unit("src/util/operator_new.cpp", code={"??2@YAPAXI@Z", "??3@YAXPAX@Z", "??_U@YAPAXI@Z"})
        self.assertEqual(self.run_with(self.wind("??2@YAPAXI@Z", "??3@YAXPAX@Z", "??_U@YAPAXI@Z"), lib), [])

    def test_ref_header_name_defined_outside_state_is_a_violation(self):
        self.write("interfaces/ref.h", "bool __cdecl SimOwns(const void* p);\nint HarnessTestMode();\n")
        harness = Unit("src/harness/harness.cpp", code={"?HarnessTestMode@@YAHXZ"})
        state = Unit("src/state/block.cpp", code={"?SimOwns@@YA_NPBX@Z"})
        found = self.run_with(self.wind("?HarnessTestMode@@YAHXZ", "?SimOwns@@YA_NPBX@Z"), harness, state)
        self.assertEqual([v.symbol for v in found], ["HarnessTestMode"])

    def test_summary_shows_allowed_counts_per_rule(self):
        text = seamlink.summary([], {("wind_meteors", "const"): 3, ("wind_meteors", "state"): 1})
        self.assertIn("wind_meteors: allowed references by rule: seam 0, state 1, service 0, host 0, bitexact 0, library 0, own 0, const 3", text)

    def test_summary_counts_kinds(self):
        link = Unit("link/data.cpp", data={"?g_foo@@3HA"})
        foreign = Unit("src/units/units.cpp", code={"?Foreign@@YAXXZ"})
        text = seamlink.summary(self.run_with(self.wind("?g_foo@@3HA", "?Foreign@@YAXXZ"), link, foreign))
        self.assertIn("wind_meteors: 2 violations (1 code, 1 data)", text)


if __name__ == "__main__":
    unittest.main()
