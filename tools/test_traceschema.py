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
        self.assertIn("extern int g_cdPathMismatch;", text)
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


class CheckTest(unittest.TestCase):
    def run_main(self, header_text, json_text, *args):
        with tempfile.TemporaryDirectory() as tmp:
            out, js = Path(tmp) / "trace_schema.h", Path(tmp) / "trace_schema.json"
            if header_text is not None:
                out.write_text(header_text)
            if json_text is not None:
                js.write_text(json_text)
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

    def test_write_repairs_a_stale_json_and_leaves_the_header(self):
        header, _, text = ts.generate()
        code, out, js = self.run_main(header, "{}", )
        self.assertEqual((code, out, js), (0, header, text))


if __name__ == "__main__":
    unittest.main()
