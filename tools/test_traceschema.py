"""Tests for traceschema.py, on synthetic records.

    uv run tools/test_traceschema.py
"""

import io
import json
import re
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import traceschema as ts

BODY = """    int head;
    union {
        struct {
            short lo;
            short hi;
        };
        int whole;
    };
    union {
        int fine;
        char coarse[4];
    };
    union { int a; char b[4]; };
    int tail;
"""


def leaves_of(name, body, declared):
    with mock.patch.object(ts.Schema, "__init__", lambda self: None):
        schema = ts.Schema()
    schema.cache = {}
    schema.errors = []
    return schema.parse(name, body, declared), schema.errors


class FlattenUnionsTest(unittest.TestCase):
    def test_non_unit_record_keeps_fine_fields(self):
        (leaves, size), errors = leaves_of("Widget", BODY, 0x14)
        self.assertEqual(errors, [])
        self.assertEqual(size, 0x14)
        self.assertEqual([(l["name"], l["offset"], l["size"]) for l in leaves],
                         [("head", 0, 4), ("lo", 4, 2), ("hi", 6, 2), ("fine", 8, 4),
                          ("a", 0xC, 4), ("tail", 0x10, 4)])

    def test_single_line_union_spans_its_widest_member(self):
        (leaves, size), errors = leaves_of("Widget", "    union { char a; int b; };\n    int c;\n", 8)
        self.assertEqual(errors, [])
        self.assertEqual([(l["name"], l["offset"], l["size"]) for l in leaves], [("a", 0, 4), ("c", 4, 4)])

    def test_record_without_unions_is_unchanged(self):
        (leaves, size), errors = leaves_of("Plain", "    int a;\n    int b;\n", 8)
        self.assertEqual(errors, [])
        self.assertEqual([l["name"] for l in leaves], ["a", "b"])


class InlineMethodTest(unittest.TestCase):
    BODY = """    int x;
    int y;

    Point operator+(const Point& o) const
    {
        Point r;
        if (o.x) {
            r.x = x;
        }
        return r;
    }
    int Sum() const { return x + y; }
    void Reset() {
        Point r;
        x = r.x;
    }
    int z;
"""

    def test_method_bodies_add_no_fields(self):
        (leaves, size), errors = leaves_of("Point", self.BODY, 12)
        self.assertEqual(errors, [])
        self.assertEqual(size, 12)
        self.assertEqual([(l["name"], l["offset"], l["size"]) for l in leaves],
                         [("x", 0, 4), ("y", 4, 4), ("z", 8, 4)])


class FunctionPointerTest(unittest.TestCase):
    def test_function_pointer_fields_are_pointer_sized(self):
        body = ("    int a;\n    void (__stdcall *handler)(void);\n"
                "    int (*plain)(int x, char* y);\n    char* (__cdecl *named)();\n    int b;\n")
        (leaves, size), errors = leaves_of("Widget", body, 0x14)
        self.assertEqual(errors, [])
        self.assertEqual([(l["name"], l["offset"], l["size"]) for l in leaves],
                         [("a", 0, 4), ("handler", 4, 4), ("plain", 8, 4), ("named", 0xC, 4), ("b", 0x10, 4)])


class FollowedHeadersTest(unittest.TestCase):
    def test_follows_includes_under_src_transitively_in_order(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            (root / "include").mkdir()
            (root / "src").mkdir()
            (root / "include/top.h").write_text(
                '#include <stdio.h>\n#include "../src/a.h"\n#include "../src/b.h"\n#include "../src/a.h"\n'
                '#include "missing.h"\n')
            (root / "src/a.h").write_text('#include "c.h"\n')
            (root / "src/b.h").write_text("")
            (root / "src/c.h").write_text('#include "b.h"\n')
            (root / "src/d.h").write_text('#include "b.h"\n')
            with mock.patch.object(ts, "ROOT", root):
                found = ts.followed_headers(root / "include/top.h")
                headers = ts.layout_headers([root / "src/d.h", root / "src/a.h"])
            self.assertEqual([p.name for p in found], ["a.h", "c.h", "b.h"])
            self.assertEqual([p.name for p in headers], ["d.h", "b.h", "a.h", "c.h"])

    def test_real_list_has_unit_and_game_under_src_only(self):
        self.assertEqual(len(ts.LAYOUT_HEADERS), len(set(ts.LAYOUT_HEADERS)))
        names = [p.name for p in ts.LAYOUT_HEADERS]
        self.assertIn("unit.h", names)
        self.assertIn("game.h", names)
        for path in ts.LAYOUT_HEADERS:
            self.assertIn(ts.ROOT / "src", path.parents)


class MovedGlobalsTest(unittest.TestCase):
    def generate(self, text):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sim_globals.csv"
            path.write_text(text)
            sim = ts.load_sim_map(path)
        with mock.patch.object(ts, "load_sim_map", lambda: sim):
            return ts.generate()[0]

    def test_empty_table_keeps_link_addresses(self):
        text = self.generate("field,name,container,offset\n")
        self.assertIn("{ \"active\", &g_meteorActive, 4, TF_SCALAR, TT_NONE },", text)
        self.assertIn("extern int g_meteorActive;", text)
        self.assertNotIn("sim_globals.h", text)

    def test_moved_global_is_an_offset_in_the_block(self):
        text = self.generate("field,name,container,offset\ng_meteorActive,g_meteorActive,,\n"
                             "g_economyPacketTick,g_economyPacketTick,g_packetManager,0x8\n")
        self.assertIn("{ \"active\", (const void*)(offsetof(SimGlobals, g_meteorActive)), 4, TF_SCALAR, TT_NONE, 1 },",
                      text)
        self.assertIn("(const void*)(offsetof(SimGlobals, g_packetManager) + 0x8), 4, TF_SCALAR, TT_NONE, 1 },", text)
        self.assertNotIn("extern int g_meteorActive;", text)
        self.assertNotIn("g_playerBudgetCap", text)
        self.assertIn("{ \"playerTickLoad\", nullptr, 40, TF_BYTES, TT_NONE },", text)
        self.assertIn("{ \"budgetRefreshCounter\", (const void*)2, 4, TF_SCALAR, TT_NONE },", text)
        self.assertIn('#include "../state/sim_globals.h"', text)
        self.assertIn('#pragma push_macro("g_meteorActive")', text)
        self.assertLess(text.index('#undef g_meteorActive'), text.index("kMeteorGlobals[]"))
        self.assertLess(text.index("kPathGlobalsCount"), text.index('#pragma pop_macro("g_meteorActive")'))

    def test_container_of_an_interior_global_is_not_expanded_in_offsetof(self):
        text = self.generate("field,name,container,offset\ng_economyPacketTick,g_economyPacketTick,g_packetManager,0x8\n")
        self.assertIn('#pragma push_macro("g_packetManager")', text)
        self.assertLess(text.index("#undef g_packetManager"), text.index("kPathGlobals[]"))
        self.assertLess(text.index("kPathGlobalsCount"), text.index('#pragma pop_macro("g_packetManager")'))

    def test_order_sizes_and_kinds_do_not_change(self):
        plain = self.generate("field,name,container,offset\n")
        moved = self.generate("field,name,container,offset\ng_meteorWeapon,g_meteorWeapon,,\n")
        names = lambda t: [l.split(",")[0] for l in t.splitlines() if l.startswith("    { \"")]
        self.assertEqual(names(plain), names(moved))
        self.assertIn('{ "weapon", (const void*)(offsetof(SimGlobals, g_meteorWeapon)), 4, TF_REF, TT_WEAPONDEF, 1 },',
                      moved)


class SchemaJsonTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header, cls.errors, cls.text = ts.generate()
        cls.model = json.loads(cls.text)

    def test_text_is_deterministic_and_ends_in_a_newline(self):
        self.assertEqual(ts.generate()[2], self.text)
        self.assertTrue(self.text.endswith("}\n"))
        self.assertEqual(self.model["trace_format"], ts.TRACE_FORMAT)

    def test_every_table_of_the_header_is_in_the_json_with_its_counts(self):
        found = re.findall(r"static const TraceTable k(\w+)Table = \{ k\w+, (\d+), k\w+, (\d+), 0x([0-9a-f]+) \};",
                           self.header)
        self.assertGreater(len(found), 40)
        self.assertEqual(sorted(name for name, *_ in found), list(self.model["tables"]))
        for name, nfields, nplan, size in found:
            table = self.model["tables"][name]
            self.assertEqual((len(table["fields"]), len(table["plan"]), table["size"]),
                             (int(nfields), int(nplan), int(size, 16)), name)

    def test_the_plan_covers_the_bytes_of_the_fields(self):
        for name, table in self.model["tables"].items():
            def covered(entries):
                return {o for e in entries for o in range(e["offset"], e["offset"] + e["size"])}
            self.assertEqual(covered(table["fields"]), covered(table["plan"]), name)
            for e in table["plan"]:
                if e["kind"] == "bytes":
                    self.assertEqual(e["name"], "")

    def test_fields_are_in_offset_order_and_pointers_name_a_target(self):
        targets = {t["name"] for t in self.model["targets"]}
        for name, table in self.model["tables"].items():
            offsets = [f["offset"] for f in table["fields"]]
            self.assertEqual(offsets, sorted(offsets), name)
            for f in table["fields"]:
                if f["kind"] in ("pointer", "ref"):
                    self.assertIn(f["target"], targets, name)
                else:
                    self.assertIsNone(f["target"], name)

    def test_order_flags_mask_carries_its_reason(self):
        flags = next(f for f in self.model["tables"]["Order"]["fields"] if f["offset"] == 0x42)
        self.assertEqual(flags["mask"], 0xFFDFFFFF)
        self.assertIn("0x200000", flags["mask_reason"])

    def test_every_mask_has_a_reason(self):
        masked = [f for t in self.model["tables"].values() for f in t["fields"] if "mask" in f]
        masked += [f for f in self.model["game_scalars"] if "mask" in f]
        self.assertGreaterEqual(len(masked), len(ts.MASKS) + len(ts.GAME_MASK_REASONS))
        for f in masked:
            self.assertTrue(f.get("mask_reason"), f)

    def test_split_tables_name_their_parent_and_exclusions_are_listed_once(self):
        tables = self.model["tables"]
        self.assertEqual(tables["OrderPosY"]["split_of"], "Order")
        self.assertNotIn("excluded", tables["OrderPosY"])
        self.assertEqual([e["field"] for e in tables["Order"]["excluded"]],
                         [f for t, f, _ in ts.EXCLUSIONS if t == "Order"])

    def test_globals_hold_name_size_and_kind(self):
        self.assertEqual(set(self.model["globals"]), {"meteors", "game", "pathfinder"})
        weapon = next(g for g in self.model["globals"]["meteors"] if g["name"] == "weapon")
        self.assertEqual((weapon["size"], weapon["kind"], weapon["target"]), (4, "ref", "weapondef"))
        self.assertEqual(self.model["wind"][0]["name"], "windCounter")

    def test_results_section_matches_the_harness_and_names_its_fields_in_dump_order(self):
        results = self.model["results"]
        header = (ts.ROOT / "src/harness/path_results.h").read_text()
        constants = dict(re.findall(r"(PATH_RESULT_\w+) = (\d+)", header))
        self.assertEqual((results["section"], results["points"]),
                         (int(constants["PATH_RESULT_SECTION"]), int(constants["PATH_RESULT_POINTS"])))
        self.assertEqual([f["name"] for f in results["fields"]], ["kind", "status", "unit", "tick", "count", "points"])
        targets = {t["name"] for t in self.model["targets"]}
        for f in results["fields"]:
            self.assertEqual(f["target"] in targets, f["kind"] == "ref", f["name"])

    def test_packets_section_matches_the_harness_and_names_its_fields_in_dump_order(self):
        packets = self.model["packets"]
        header = (ts.ROOT / "src/harness/share_packets.h").read_text()
        constants = dict(re.findall(r"(SHARE_PACKET_\w+) = (\d+)", header))
        self.assertEqual(packets["section"], int(constants["SHARE_PACKET_SECTION"]))
        self.assertEqual([f["name"] for f in packets["fields"]], ["type", "subtype", "from", "to", "amount"])
        self.assertEqual({(f["size"], f["kind"], f["target"]) for f in packets["fields"]}, {(4, "scalar", None)})

    def test_every_event_section_has_one_field_list_per_kind(self):
        sections = {m.key: m.section for m in ts.EVENT_MODULES}
        self.assertEqual(sections, {"economy": 33, "features": 36, "windMeteors": 37, "targeting": 38, "los_radar": 39,
                                    "map": 40, "playersEvents": 41, "aiEvents": 42, "weaponsEvents": 47,
                                    "ordersEvents": 48, "movementEvents": 49})
        targets = {t["name"] for t in self.model["targets"]}
        for module in ts.EVENT_MODULES:
            section = self.model[module.key]
            self.assertEqual(section["section"], module.section)
            self.assertNotIn("fields", section)
            self.assertNotIn("account", section)
            self.assertEqual(len(section["kinds"]), len(module.payloads), module.key)
            for value, kind in section["kinds"].items():
                names = [f["name"] for f in kind["fields"]]
                self.assertEqual(names[0], "kind", kind["name"])
                self.assertEqual(len(names), len(set(names)), kind["name"])
                self.assertEqual(names[-1] == "outcome", module.outcome, kind["name"])
                self.assertEqual(sum(f["size"] for f in kind["fields"]) - (4 if module.outcome else 0), kind["size"], kind["name"])
                for f in kind["fields"]:
                    self.assertEqual(f["target"] in targets, f["kind"] == "ref", f["name"])

    def test_targeting_kinds_carry_only_their_own_fields(self):
        kinds = self.model["targeting"]["kinds"]
        fields = lambda value: [(f["name"], f["size"], f["kind"]) for f in kinds[value]["fields"]]
        head = [("kind", 4, "scalar"), ("unit", 4, "ref"), ("weapon", 4, "scalar")]
        self.assertEqual(kinds["96"]["name"], "TARGETING_SET_UNIT")
        self.assertEqual(fields("96"), head + [("target", 4, "ref"), ("outcome", 4, "scalar")])
        self.assertEqual(fields("97"), head + [("pos", 12, "bytes"), ("outcome", 4, "scalar")])
        self.assertEqual(fields("98"), head + [("outcome", 4, "scalar")])
        self.assertEqual(fields("100"), head + [("xOrUnit", 4, "scalar"), ("zOrSentinel", 4, "scalar"), ("outcome", 4, "scalar")])

    def test_economy_load_holds_the_account_and_wind_meteors_has_no_outcome(self):
        load = self.model["economy"]["kinds"]["1"]
        self.assertEqual([(f["name"], f["size"], f["kind"]) for f in load["fields"]],
                         [("kind", 4, "scalar"), ("player", 4, "scalar"), ("unit", 4, "ref"), ("account", 48, "bytes"),
                          ("outcome", 4, "scalar")])
        params = self.model["windMeteors"]["kinds"]["66"]
        self.assertEqual([(f["name"], f["size"], f["kind"]) for f in params["fields"]],
                         [("kind", 4, "scalar"), ("params", 48, "bytes")])
        self.assertEqual([f["name"] for f in self.model["windMeteors"]["kinds"]["67"]["fields"]], ["kind"])

    def test_features_state_is_a_handle_and_players_record_is_flattened(self):
        free = self.model["features"]["kinds"]["48"]
        self.assertEqual([(f["name"], f["kind"]) for f in free["fields"]],
                         [("kind", "scalar"), ("state", "handle"), ("outcome", "scalar")])
        give = [f["name"] for f in self.model["playersEvents"]["kinds"]["192"]["fields"]]
        self.assertEqual(give, ["kind", "unit", "player", "record.present", "record.hitPoints", "record.buildPercent",
                                "record.angles", "record.stockpiles", "outcome"])

    def test_the_header_has_the_kind_tables_with_offsetof_and_payload_sizes(self):
        for module in ts.EVENT_MODULES:
            found = re.search(r'static const TraceEventModule k%sEvents = \{ "%s", %d, k%sKinds, (\d+) \};'
                              % (module.c_name, module.key, module.section, module.c_name), self.header)
            self.assertIsNotNone(found, module.key)
            self.assertEqual(int(found.group(1)), len(self.model[module.key]["kinds"]))
        self.assertIn("{ \"target\", offsetof(TargetingSetUnit, target), 4, TF_REF, TT_UNIT, 0 },", self.header)
        self.assertIn("{ 96, \"TARGETING_SET_UNIT\", sizeof(TargetingSetUnit), kEventTargetingSetUnit, 5 },", self.header)
        self.assertIn("#define TRACE_EVENT_MAX_BYTES 80", self.header)

    def test_a_header_with_an_unknown_payload_is_refused(self):
        module = ts.EventModule("x", 1, "X", "targeting.h", "TargetingEventHeader", ["TargetingEventKind"], {})
        with self.assertRaises(ValueError):
            ts.event_kinds(module)

    def test_the_unit_account_is_not_hashed(self):
        unit = self.model["tables"]["Unit"]
        self.assertEqual([e["field"] for e in unit["excluded"]], ["+0xbc..+0xeb", "unusedLosCache", "unusedLosSightFrame"])
        for f in unit["fields"] + unit["plan"]:
            self.assertFalse(0xBC <= f["offset"] <= 0xEB, f)
        self.assertIn({"name": "", "offset": 0xA6, "size": 22, "kind": "bytes", "target": None}, unit["plan"])


class EventKindsTest(unittest.TestCase):
    def test_every_unit_offset_of_a_payload_is_a_unit_ref(self):
        for module in ts.EVENT_MODULES:
            lines = (ts.INTERFACES / module.header_file).read_text(encoding="latin-1").splitlines()
            for doc, line in zip(lines, lines[1:]):
                member = re.fullmatch(r"\s+(unsigned int) (\w+);", line)
                if member and "block offset" in doc and member.group(2) not in module.handles:
                    self.assertEqual(ts.is_unit_ref(*member.groups()), "unit" in doc.lower(), (module.header_file, doc))

    def test_a_size_function_is_generated_for_each_apply_that_checks_a_size(self):
        files = dict(ts.event_size_files())
        self.assertEqual(sorted(p.name for p in files),
                         ["ai_event_size.h", "los_radar_event_size.h", "map_event_size.h", "movement_event_size.h",
                          "orders_event_size.h", "players_event_size.h", "weapons_event_size.h"])
        for path, text in files.items():
            self.assertTrue(text.startswith("// Generated by tools/traceschema.py"), path)

    def test_a_size_function_groups_the_kinds_of_one_payload_and_leaves_the_header_kinds_to_the_default(self):
        text = dict((p.name, t) for p, t in ts.event_size_files())["weapons_event_size.h"]
        self.assertIn("inline unsigned int WeaponsEventSize(int kind)", text)
        self.assertIn("    case WEAPONS_DAMAGE:\n    case WEAPONS_REMOTE_DAMAGE:\n        return sizeof(WeaponsDamage);\n", text)
        self.assertIn("    default:\n        return sizeof(WeaponsEventHeader);\n", text)
        self.assertNotIn("WEAPONS_INIT_SLOTS", text)
        self.assertEqual(text.count("return sizeof("), len(set(ts.module_of_header("weapons.h").payloads.values()) - {None}) + 1)

    def test_a_module_is_found_by_its_header(self):
        self.assertEqual(ts.module_of_header("weapons.h").c_name, "Weapons")
        with self.assertRaises(ValueError):
            ts.module_of_header("nothing.h")


class OwnerOnlyStateTest(unittest.TestCase):
    def setUp(self):
        self.model = json.loads(ts.JSON_OUT.read_text())

    def test_the_format_is_7(self):
        self.assertEqual(ts.TRACE_FORMAT, 7)

    def test_rec_hashes_only_b(self):
        rec = self.model["tables"]["Rec"]
        self.assertEqual([f["name"] for f in rec["fields"]], ["b"])
        self.assertEqual([e["field"] for e in rec["excluded"]], ["a", "flags", "first"])

    def test_the_clocks_of_a_player_are_not_hashed(self):
        player = self.model["tables"]["Player"]
        self.assertEqual([e["field"] for e in player["excluded"]], ["unusedUpdateTime", "unusedWinLoseTime"])
        for f in player["fields"] + player["plan"]:
            self.assertFalse(f["offset"] <= 0xF0 < f["offset"] + f["size"] and f["name"] != "", f)

    def test_the_end_game_countdown_is_not_a_game_scalar(self):
        self.assertNotIn("endGameCountdown", [g["name"] for g in self.model["game_scalars"]])


class CheckTest(unittest.TestCase):
    def run_main(self, header_text, json_text, *args, size_tail=""):
        with tempfile.TemporaryDirectory() as tmp:
            out, js = Path(tmp) / "trace_schema.h", Path(tmp) / "trace_schema.json"
            if header_text is not None:
                out.write_text(header_text)
            if json_text is not None:
                js.write_text(json_text)
            for path, text in ts.event_size_files():
                mirror = Path(tmp) / path.relative_to(ts.ROOT)
                mirror.parent.mkdir(parents=True, exist_ok=True)
                mirror.write_text(text + size_tail)
            with mock.patch.object(ts, "OUT", out), mock.patch.object(ts, "JSON_OUT", js), \
                    mock.patch.object(ts, "ROOT", Path(tmp)), mock.patch.object(sys, "argv", ["traceschema", *args]), \
                    mock.patch("sys.stdout", io.StringIO()), mock.patch("sys.stderr", io.StringIO()):
                try:
                    ts.main()
                    code = 0
                except SystemExit as e:
                    code = e.code
            return code, out.read_text(), js.read_text() if js.exists() else None

    def test_check_passes_on_fresh_files_and_fails_when_either_is_stale(self):
        header, _, text = ts.generate()
        self.assertEqual(self.run_main(header, text, "--check")[0], 0)
        self.assertEqual(self.run_main(header, text.replace('"size": 4', '"size": 5', 1), "--check")[0], 1)
        self.assertEqual(self.run_main(header + "x", text, "--check")[0], 1)
        self.assertEqual(self.run_main(header, None, "--check")[0], 1)
        self.assertEqual(self.run_main(header, text, "--check", size_tail="x")[0], 1)

    def test_write_repairs_a_stale_json_and_leaves_the_header(self):
        header, _, text = ts.generate()
        code, out, js = self.run_main(header, "{}", )
        self.assertEqual((code, out, js), (0, header, text))


if __name__ == "__main__":
    unittest.main()
