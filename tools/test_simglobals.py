"""Tests for simglobals.py.

    uv run tools/test_simglobals.py
"""

import unittest

import simglobals as sg


def row(leaf, type_):
    r = sg.Row(leaf, 0x500000, "global")
    r.type = type_
    r.size = 4
    r.csv = {"type": type_}
    return r


class RefFieldTest(unittest.TestCase):
    def test_a_pointer_becomes_a_ref(self):
        self.assertEqual(sg.as_ref("WeaponDef*"), "Ref<WeaponDef>")

    def test_a_pointer_array_keeps_its_dimension(self):
        self.assertEqual(sg.as_ref("PlayerAI*[10]"), "Ref<PlayerAI>[10]")

    def test_a_function_pointer_is_refused(self):
        with self.assertRaises(SystemExit):
            sg.as_ref("int (__stdcall*[45])(int)")

    def test_only_the_named_fields_are_refs(self):
        self.assertEqual(sg.resolve_type(row("g_meteorWeapon", "WeaponDef*")), "Ref<WeaponDef>")
        self.assertEqual(sg.resolve_type(row("g_playerAI", "")), "Ref<PlayerAI>[10]")
        self.assertEqual(sg.resolve_type(row("g_explodedPieces", "")), "Ref<ExplodedPiece>[100]")
        self.assertEqual(sg.resolve_type(row("g_other", "Unit*")), "Unit*")

    def test_a_ref_field_is_declared_as_one(self):
        self.assertEqual(sg.make_decl("Ref<PlayerAI>[10]", "g_playerAI"), "Ref<PlayerAI> g_playerAI[10]")


if __name__ == "__main__":
    unittest.main()
