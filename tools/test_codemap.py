"""Tests for codemap.py, on hand-written sources and on the committed data.

    uv run tools/test_codemap.py
"""

import unittest

import codemap as cm


def parse(body, known):
    raw = "void F() {" + body + "}"
    masked = cm.mask_source(raw)
    definition = cm.find_definitions(masked)[0]
    callees = {name: {"id": i, "name": name, "via": ""} for i, name in enumerate(known)}
    return cm.FlowParser(masked, raw, cm.line_starts(raw), callees.get).parse(definition.body, definition.end)


class MaskTest(unittest.TestCase):
    def test_keeps_offsets_and_blanks_literals(self):
        raw = 'int a; // call(b)\nchar* s = "call(c)"; /* call(d) */ f();\n'
        masked = cm.mask_source(raw)
        self.assertEqual(len(raw), len(masked))
        self.assertEqual(masked.count("call"), 0)
        self.assertIn("f()", masked)

    def test_drops_access_log_blocks_and_keeps_one_arm(self):
        raw = "#if BT_ACCESS_LOG\nlog();\n#endif\n#ifdef X\nkept();\n#else\ngone();\n#endif\n"
        masked = cm.mask_source(raw)
        self.assertNotIn("log", masked)
        self.assertIn("kept", masked)
        self.assertNotIn("gone", masked)


class DefinitionTest(unittest.TestCase):
    def test_finds_methods_constructors_and_ignores_calls(self):
        masked = cm.mask_source("void A::B(int x) const { C(x); }\nA::A() : v(1), w(2) { }\nint D(int);\n")
        self.assertEqual([d.name for d in cm.find_definitions(masked)], ["A::B", "A::A"])


class FlowTest(unittest.TestCase):
    def test_branches_loops_and_order(self):
        nodes = parse("if (A()) { B(1, x); } else C(); while (n--) { D(); } return E();", "ABCDE")
        self.assertEqual([n["t"] for n in nodes], ["if", "loop", "return"])
        self.assertEqual(nodes[0]["calls"][0]["name"], "A")
        self.assertEqual(nodes[0]["then"][0]["calls"][0]["values"], ["1", "x"])
        self.assertEqual(nodes[0]["else"][0]["calls"][0]["name"], "C")
        self.assertEqual(nodes[2]["calls"][0]["name"], "E")

    def test_nested_calls_run_inner_first(self):
        nodes = parse("A(B());", "AB")
        self.assertEqual([c["name"] for c in nodes[0]["calls"]], ["B", "A"])

    def test_switch_merges_fallthrough_labels(self):
        nodes = parse("switch (v) { case 1: case 2: A(); break; default: B(); }", "AB")
        self.assertEqual([c["labels"] for c in nodes[0]["cases"]], [["1", "2"], ["default"]])

    def test_goto_labels_and_unknown_names_are_not_calls(self):
        nodes = parse("top: strlen(s); goto top;", "A")
        self.assertEqual([n["t"] for n in nodes], ["label", "stmt", "goto"])

    def test_parameter_names(self):
        self.assertEqual(cm.parameter_names("WeaponDef* weapon, Vec3* pos, int"), ["weapon", "pos", ""])
        self.assertEqual(cm.parameter_names("void"), [])


class ModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model = cm.Model()

    def function(self, name):
        return self.model.by_name[name][0]

    def test_every_seam_slot_resolves(self):
        unresolved = [s.key for s in self.model.slots.values() if s.function is None]
        self.assertEqual(unresolved, [])

    def test_nearly_every_function_is_located_and_charted(self):
        stats = self.model.stats()
        self.assertGreater(stats["placed"] / stats["functions"], 0.98)
        self.assertGreater(stats["flow_parsed"] / stats["placed"], 0.99)

    def test_calls_go_through_seams_to_the_implementation(self):
        meteors = self.function("UpdateMeteors")
        spawn = self.function("SpawnProjectile")
        edge = meteors.out[spawn.id]
        self.assertIn("seam", edge.kinds)
        self.assertEqual(edge.via, "WeaponsSeam::SpawnProjectile")
        self.assertEqual([a["value"] for a in edge.sites[0]["args"]], ["g_meteorWeapon", "&pos", "&vel", "1"])
        self.assertEqual(edge.sites[0]["args"][0]["param"], "weapon")

    def test_forwarding_wrappers_are_drawn_through(self):
        activate = self.function("ActivateOrder")
        callees = {self.model.functions[i].name for i, e in activate.out.items() if "seam" in e.kinds}
        self.assertIn("Unit::SetStateBits", callees)
        self.assertNotIn("SetStateBitsNative", callees)

    def test_a_wrapper_with_its_own_work_stays_visible(self):
        busy = []
        for function in self.model.functions:
            if function.subsystem != "seams":
                continue
            targets = {self.model.choose(t, function) for kind, t in function.rows if kind == "call"} - {None}
            if len([t for t in targets if t.subsystem != "seams"]) > 1:
                busy.append(function)
        self.assertTrue(busy)
        for wrapper in busy:
            self.assertIs(self.model.through_native(wrapper), wrapper)

    def test_tick_lists_the_phases_in_order(self):
        names = [p["name"] for p in self.model.tick_phases()["phases"]]
        expected = ["UpdateAllUnits", "UpdateProjectiles", "UpdateExplosions", "UpdatePlayers", "UpdateFeatures", "UpdateWind", "UpdateMeteors"]
        self.assertEqual(names[: len(expected)], expected)

    def test_data_items_know_writers_and_declaration(self):
        item = self.model.item_payload("Game::ticks")
        self.assertIn("RunGameSteps", [f["name"] for f in item["write"]])
        self.assertEqual(item["declaration"]["file"], "src/game/game.h")
        self.assertEqual(item["declaration"]["text"], "int ticks")

    def test_source_is_limited_to_the_scanned_folders(self):
        self.assertIsNone(self.model.source_payload("../../etc/passwd"))
        self.assertIsNone(self.model.source_payload("tools/codemap.py"))
        self.assertIn("RunGameSteps", self.model.source_payload("src/ingame/game_dialogs.cpp")["text"])


if __name__ == "__main__":
    unittest.main()
