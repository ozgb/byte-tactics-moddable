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

    def filler(self, name, target):
        unit = Unit(seamlink.NATIVE_TABLE, code={f"?{name}@@YGXXZ"})
        unit.refs = [(f"?{name}@@YGXXZ", target)]
        return unit

    def test_a_phase_function_of_the_engine_is_a_seam_for_a_phase_slot_filler_only(self):
        engine = Unit("src/game/scheduler.cpp", code={"?RunPhase@@YAXXZ"})
        other = Unit("src/game/other.cpp", code={"?Other@@YAXXZ"})
        fillers = SUBSYSTEMS + "UpdateAllUnits,src/seams/native.cpp,units,x,\nUpdatePlayers,src/seams/native.cpp,players,x,\nUnitsNative,src/seams/native.cpp,units,x,\n"
        self.write("data/subsystems.csv", fillers)
        self.assertEqual(len(self.run_with(self.filler("UpdateAllUnits", "?RunPhase@@YAXXZ"), engine)), 1)
        self.write("data/subsystems.csv", fillers + "RunPhase,src/game/scheduler.cpp,game,phase:Update,game\n")
        self.assertEqual(self.run_with(self.filler("UpdateAllUnits", "?RunPhase@@YAXXZ"), self.filler("UpdatePlayers", "?RunPhase@@YAXXZ"), engine), [])
        found = self.run_with(self.wind("?RunPhase@@YAXXZ"), self.filler("UnitsNative", "?RunPhase@@YAXXZ"), engine)
        self.assertEqual([v.line() for v in found], ["units UnitsNative -> RunPhase (code, game)", "wind_meteors Tick -> RunPhase (code, game)"])
        self.write("data/subsystems.csv", fillers + "Other,src/game/other.cpp,game,phase:Update,game\n")
        self.assertEqual(len(self.run_with(self.filler("UpdateAllUnits", "?Other@@YAXXZ"), other)), 1)

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

    def test_call_through_an_alias_is_judged_against_its_definition(self):
        foreign = Unit("src/units/units.cpp", code={"?Foreign@@YAXXZ"})
        aliases = {"??0Foreign@@QAE@XZ": "?Foreign@@YAXXZ"}
        found = seamlink.violations([self.wind("??0Foreign@@QAE@XZ"), foreign], self.root, aliases)
        self.assertEqual([v.line() for v in found], ["wind_meteors Tick -> Foreign::Foreign (code, units)"])
        self.assertEqual(self.run_with(self.wind("??0Foreign@@QAE@XZ"), foreign), [])

    def test_call_through_an_alias_within_one_subsystem_is_allowed(self):
        aliases = {"??0Helper@@QAE@XZ": "?Helper@@YAXXZ"}
        self.assertEqual(seamlink.violations([self.wind("??0Helper@@QAE@XZ")], self.root, aliases), [])

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

    def test_unlisted_function_is_judged_for_the_subsystem_of_its_file(self):
        link = Unit("link/data.cpp", data={"?g_foo@@3HA"})
        wind = self.wind(code={"??$Inline@H@@YAXXZ"})
        wind.refs = [("??$Inline@H@@YAXXZ", "?g_foo@@3HA")]
        found = self.run_with(wind, link)
        self.assertEqual([(v.subsystem, v.symbol) for v in found], [("wind_meteors", "g_foo")])

    def test_unlisted_function_in_a_shared_file_is_judged_for_each_subsystem(self):
        link = Unit("link/data.cpp", data={"?g_foo@@3HA"})
        mixed = Unit("src/game/mixed.cpp", code={"?Mixed@@YAXXZ", "?Other@@YAXXZ"})
        mixed.refs = [("??_GFoo@@UAEPAXI@Z", "?g_foo@@3HA")]
        found = self.run_with(mixed, link)
        self.assertEqual(sorted((v.subsystem, v.symbol) for v in found), [("economy", "g_foo"), ("units", "g_foo")])

    def test_unlisted_function_in_a_non_simulation_file_is_not_checked(self):
        link = Unit("link/data.cpp", data={"?g_foo@@3HA"})
        gui = Unit("src/gui/x.cpp")
        gui.refs = [("??_GFoo@@UAEPAXI@Z", "?g_foo@@3HA")]
        self.assertEqual(self.run_with(gui, link), [])

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

    def test_a_virtual_row_is_for_the_virtual_check(self):
        row = ("pathfinding", "Pathfinder::Cost", "virtual Target::Cost", "TODO(#400): seam slot")
        self.assertEqual(seamlink.check([], [row]), [])

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

    def test_throw_info_and_std_statics_are_library(self):
        names = ["_TI2?AVlogic_error", "__TI3?AVout_of_range@std@@", "_CTA3?AVout_of_range", "_CT??_R0?AVx@@@8", "?_C@?1??_Nullstr@?$basic_string@DU?$char_traits@D@std@@@std@@CGPBDXZ@4DB"]
        link = Unit("link/data.cpp", data=set(names))
        self.assertEqual(self.run_with(self.wind(*names), link), [])
        other = Unit("link/data.cpp", data={"_TXother", "?g_std@@3HA"})
        self.assertEqual(len(self.run_with(self.wind("_TXother", "?g_std@@3HA"), other)), 2)

    def test_std_members_are_library_by_scope(self):
        names = ["?insert@?$vector@HV?$allocator@H@std@@@std@@QAEXXZ", "??0?$vector@HV?$allocator@H@std@@@std@@QAE@XZ", "?npos@?$basic_string@DU?$char_traits@D@std@@@std@@2IB", "?_Xlen@std@@YAXXZ"]
        link = Unit("link/data.cpp", code=set(names))
        self.assertEqual(self.run_with(self.wind(*names), link), [])

    def test_project_function_with_std_argument_is_a_violation(self):
        names = ["?GetLosTable@LosTables@@QAEABV?$vector@HV?$allocator@H@std@@@std@@XZ", "?Find@PlayerAI@@QAEXAAV?$basic_string@DU?$char_traits@D@std@@@std@@@Z"]
        other = Unit("src/units/units.cpp", code=set(names))
        self.assertEqual(len(self.run_with(self.wind(*names), other)), 2)

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


class PrivateStateTest(unittest.TestCase):
    SUBS = """function,file,subsystem,rule,override
Search,src/ai/pathfind.cpp,pathfinding,x,
Step,src/ai/pathfind.cpp,pathfinding,x,
Order,src/orders/orders.cpp,orders,x,
Move,src/units/units.cpp,units,x,
"""

    def check(self, rows):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "data").mkdir()
            (root / seamlink.SUBSYSTEMS).write_text(self.SUBS)
            (root / seamlink.RWSETS).write_text("address,function,file,kind,target\n"
                                                + "".join(f"0x1,{f},{p},{k},{t}\n" for f, p, k, t in rows))
            return seamlink.private_state(root)

    def test_owner_may_touch_everything(self):
        rows = [("Search", "src/ai/pathfind.cpp", "field_write", "Pathfinder::grid"),
                ("Step", "src/ai/pathfind.cpp", "field_read", "Game::pathfinder"),
                ("Search", "src/ai/pathfind.cpp", "deref_write", "*PathCell")]
        self.assertEqual(self.check(rows), [])

    def test_other_subsystem_field_access_is_found(self):
        found = self.check([("Order", "src/orders/orders.cpp", "field_read", "Pathfinder::stepsPerTick")])
        self.assertEqual(found, [("orders", "Order", "field_read", "Pathfinder::stepsPerTick")])

    def test_game_pathfinder_item_is_private(self):
        found = self.check([("Move", "src/units/units.cpp", "field_write", "Game::pathfinder")])
        self.assertEqual([f[0] for f in found], ["units"])

    def test_deref_of_a_region_type_is_found(self):
        found = self.check([("Move", "src/units/units.cpp", "deref_read", "*Ref<Node>"),
                            ("Move", "src/units/units.cpp", "deref_write", "*PathCell")])
        self.assertEqual(len(found), 2)

    def test_similar_names_and_calls_are_not_found(self):
        rows = [("Order", "src/orders/orders.cpp", "field_read", "Game::pathfinderCount"),
                ("Order", "src/orders/orders.cpp", "field_read", "PathEvent::kind"),
                ("Order", "src/orders/orders.cpp", "call", "Pathfinder::Submit")]
        self.assertEqual(self.check(rows), [])

    def test_unowned_function_is_found(self):
        found = self.check([("Mystery", "src/x.cpp", "field_read", "OpenHeap::count")])
        self.assertEqual(found[0][0], "unowned")


ECONOMY_HEADER = """#include "state_writer.h"
// A ledger.
struct EconLedger {
    float make;
    float stall;
};
struct EconAccount {
    EconLedger energy;
    EconLedger metal;
};
struct EconomyWorldView {
    unsigned int (__stdcall* Tick)(void);
    int (__stdcall* UnitInfo)(EconUnit unit, EconUnitInfo* out);
    void (__stdcall* Emit)(const EconomyEvent* effect);
};
struct EconomySeam {
    unsigned int (__stdcall* RegionSize)(const EconMapInfo* map);
    void (__stdcall* Init)(const EconMapInfo* map, const EconomyWorldView* world, void* region, unsigned int size);
    void* (__stdcall* Shutdown)(void);
    int (__stdcall* Apply)(const EconomyEvent* event);
    int (__stdcall* Spend)(const EconomyEvent* spend);
    void (__stdcall* UpdatePlayerEconomy)(int player);
    void (__stdcall* UpdateMetalExtraction)(EconUnit unit);
    void (__stdcall* UpdateResourceSharing)(int player);
    int (__stdcall* RequestGranted)(EconUnit unit, unsigned int resources);
    double (__stdcall* GetEnergyUse)(EconType type);
    void (__stdcall* UnitAccount)(EconUnit unit, EconAccount* out);
    void (__stdcall* WriteState)(const StateWriter* writer);
};
"""
FILLERS = {"RegionSize": "Size", "Init": "EconomyInit", "Shutdown": "EconomyShutdown", "Apply": "EconomyApply",
           "Spend": "EconomySpend",            "UpdatePlayerEconomy": "EconomyUpdatePlayer", "UpdateMetalExtraction": "EconomyExtract",
           "UpdateResourceSharing": "EconomyShare", "RequestGranted": "Gate", "GetEnergyUse": "Use", "UnitAccount": "Account",
           "WriteState": "Dump"}
QUERY_FILLERS = ("Size", "Gate", "Use", "Account", "Dump")


def native(fillers):
    return "static EconomySeam s_economy;\nstatic PlayersSeam s_players;\ns_players.Init = PlayersInit;\n" + "".join(
        f"s_economy.{slot} = {fn};\n" for slot, fn in fillers.items())


class PilotSeamsTest(unittest.TestCase):
    SUBS = """function,file,subsystem,rule,override
Tick,src/game/scheduler.cpp,game,x,
Hurt,src/units/units.cpp,units,x,
Pay,src/orders/orders.cpp,orders,x,
Fire,src/weapons/weapons.cpp,weapons,x,
DrawPanel,src/ingame/info_panel.cpp,presentation,x,
ShowDialog,src/ingame/game_dialogs.cpp,presentation,x,
RunGameSteps,src/ingame/game_dialogs.cpp,game,x,
Gate,src/game/economy.cpp,economy,x,
Account,src/game/economy.cpp,economy,x,
Dump,src/game/economy.cpp,economy,x,
Help,src/game/economy.cpp,economy,x,
"""

    def check(self, rows, header=ECONOMY_HEADER, fillers=None):
        fillers = FILLERS if fillers is None else fillers
        rows = [(f, "src/game/economy.cpp", "function", "") for f in QUERY_FILLERS] + list(rows)
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "data").mkdir()
            (root / "src/seams").mkdir(parents=True)
            (root / "interfaces").mkdir()
            (root / seamlink.NATIVE_TABLE).write_text(native(fillers))
            (root / seamlink.ECONOMY_HEADER).write_text(header)
            (root / seamlink.SUBSYSTEMS).write_text(self.SUBS)
            (root / seamlink.RWSETS).write_text("address,function,file,kind,target\n"
                                                + "".join(f'0x1,{f},{p},{k},"{t}"\n' for f, p, k, t in rows))
            return seamlink.pilot_seams(root)

    def slot(self, name, seam="EconomySeam"):
        return f"indirect via void (*)(int) __attribute__((stdcall)) [{seam}::{name}]"

    def test_the_tree_as_given_passes(self):
        self.assertEqual(self.check([]), [])

    def test_the_engine_may_call_writing_slots(self):
        rows = [("Tick", "src/game/scheduler.cpp", "indirect_call", self.slot("Apply")),
                ("RecEconomyApply", "src/harness/replay_hooks.inc", "indirect_call", self.slot("Apply")),
                ("EconomyInbox", "src/seams/economy_world.cpp", "indirect_call", self.slot("Apply")),
                ("ScheduleEconomySpend", "src/seams/economy_world.cpp", "indirect_call", self.slot("Spend")),
                ("LoadBattleAssets", "src/game/game_state.cpp", "indirect_call", self.slot("Init")),
                ("ShutdownIngameSystems", "src/game/game_state.cpp", "indirect_call", self.slot("Shutdown")),
                ("RunGameSteps", "src/ingame/game_dialogs.cpp", "indirect_call", self.slot("UpdateResourceSharing")),
                ("RunGameSteps", "src/ingame/game_dialogs.cpp", "field_read", "EconomySeam::UpdateResourceSharing"),
                ("Help", "src/game/economy.cpp", "indirect_call", self.slot("Apply"))]
        self.assertEqual(self.check(rows), [])

    def test_another_subsystem_calling_a_writing_slot_is_found(self):
        found = self.check([("Hurt", "src/units/units.cpp", "indirect_call", self.slot("Apply")),
                            ("Pay", "src/orders/orders.cpp", "indirect_call", self.slot("UpdatePlayerEconomy")),
                            ("Fire", "src/weapons/weapons.cpp", "indirect_call", self.slot("Spend"))])
        self.assertEqual([f[:3] for f in found], [("call", "orders", "Pay"), ("call", "units", "Hurt"), ("call", "weapons", "Fire")])

    def test_a_presentation_or_unowned_caller_of_a_writing_slot_is_found(self):
        found = self.check([("DrawPanel", "src/ingame/info_panel.cpp", "indirect_call", self.slot("Apply")),
                            ("ShowDialog", "src/ingame/game_dialogs.cpp", "indirect_call", self.slot("UpdateResourceSharing")),
                            ("Mystery", "src/x.cpp", "field_read", "EconomySeam::Apply"),
                            ("LoadBattleAssets", "src/other.cpp", "indirect_call", self.slot("Init"))])
        self.assertEqual([f[:3] for f in found], [("call", "presentation", "DrawPanel"), ("call", "presentation", "ShowDialog"),
                                                  ("call", "unowned", "LoadBattleAssets"), ("call", "unowned", "Mystery")])

    def test_query_slots_may_be_called(self):
        self.assertEqual(self.check([("Hurt", "src/units/units.cpp", "indirect_call", self.slot("RequestGranted")),
                                     ("DrawPanel", "src/ingame/info_panel.cpp", "indirect_call", self.slot("UnitAccount"))]), [])

    def test_a_slot_missing_from_the_native_table_is_found(self):
        fillers = {k: v for k, v in FILLERS.items() if k not in ("RegionSize", "Apply")}
        found = self.check([], fillers=fillers)
        self.assertEqual([(f[0], f[2]) for f in found], [("slot", "Apply"), ("slot", "RegionSize")])

    def test_a_native_table_without_an_economy_seam_fills_nothing(self):
        found = self.check([], fillers={})
        self.assertEqual(len(found), len(FILLERS))
        self.assertEqual({f[0] for f in found}, {"slot"})

    def test_a_slot_in_neither_list_and_a_listed_slot_not_in_the_header_are_found(self):
        header = ECONOMY_HEADER.replace("    void (__stdcall* WriteState)", "    void (__stdcall* Debug)(int x);\n    void (__stdcall* WriteState)")
        header = header.replace("    double (__stdcall* GetEnergyUse)(EconType type);\n", "")
        found = self.check([], header=header, fillers=dict(FILLERS, Debug="EconomyDebug"))
        self.assertEqual([(f[0], f[2]) for f in found], [("slot", "Debug"), ("slot", "GetEnergyUse")])

    def test_a_write_behind_a_query_slot_is_found(self):
        found = self.check([("Gate", "src/game/economy.cpp", "field_write", "Player::energy")])
        self.assertEqual(found, [("query", "economy", "Gate", "EconomySeam::RequestGranted field write Player::energy")])

    def test_a_write_in_a_callee_is_found(self):
        found = self.check([("Gate", "src/game/economy.cpp", "call", "Help"), ("Help", "src/game/economy.cpp", "global_write", "g_x")])
        self.assertEqual(len(found), 1)
        self.assertEqual(found[0][2], "Help")

    def test_the_out_parameter_may_be_written_but_not_the_region(self):
        self.assertEqual(self.check([("Account", "src/game/economy.cpp", "deref_write", "*EconAccount"),
                                     ("Account", "src/game/economy.cpp", "field_write", "EconLedger::stall"),
                                     ("Account", "src/game/economy.cpp", "call", "UnitAccountOf"),
                                     ("UnitAccountOf", "src/game/economy.cpp", "deref_escape", "*EconAccount")]), [])
        found = self.check([("Gate", "src/game/economy.cpp", "field_write", "EconLedger::stall"),
                            ("Account", "src/game/economy.cpp", "call", "Help"),
                            ("Help", "src/game/economy.cpp", "deref_write", "*EconAccount"),
                            ("Account", "src/game/economy.cpp", "deref_maybe_written", "*EconAccount")])
        self.assertEqual([(f[2], f[3].split(" ", 1)[1]) for f in found],
                         [("Account", "deref maybe written *EconAccount"), ("Gate", "field write EconLedger::stall"),
                          ("Help", "deref write *EconAccount")])

    def test_info_structs_and_unreached_writers_are_not_found(self):
        rows = [("Gate", "src/game/economy.cpp", "field_write", "EconUnitInfo::player"),
                ("Help", "src/game/economy.cpp", "field_write", "Player::energy")]
        self.assertEqual(self.check(rows), [])

    def test_an_indirect_call_behind_a_query_other_than_a_world_read_is_found(self):
        self.assertEqual(self.check([("Gate", "src/game/economy.cpp", "indirect_call", self.slot("UnitInfo", "EconomyWorldView")),
                                     ("Dump", "src/game/economy.cpp", "indirect_call", self.slot("Int", "StateWriter"))]), [])
        found = self.check([("Gate", "src/game/economy.cpp", "call", "Help"),
                            ("Help", "src/game/economy.cpp", "indirect_call", self.slot("Emit", "EconomyWorldView")),
                            ("Account", "src/game/economy.cpp", "indirect_call", self.slot("Int", "StateWriter")),
                            ("Dump", "src/game/economy.cpp", "indirect_call", "virtual Thing::Poke"),
                            ("Use", "src/game/economy.cpp", "indirect_call", "indirect via void (*)() [callback]")])
        self.assertEqual([f[2] for f in found], ["Account", "Dump", "Help", "Use"])

    def test_a_call_without_rows_behind_a_query_is_found(self):
        self.assertEqual(self.check([("Account", "src/game/economy.cpp", "call", "operator=")]), [])
        found = self.check([("Gate", "src/game/economy.cpp", "call", "memset"), ("Use", "src/game/economy.cpp", "asm", "inline")])
        self.assertEqual([(f[2], f[3]) for f in found],
                         [("Use", "EconomySeam::GetEnergyUse has inline assembly"),
                          ("memset", "EconomySeam::RequestGranted calls memset, which data/rwsets.csv has no rows for")])


if __name__ == "__main__":
    unittest.main()
