"""Tests for simglobals.py.

    uv run tools/test_simglobals.py
"""

import csv
import tempfile
import unittest
from pathlib import Path

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


STATE_HEADER = "name,address,kind,proposed,class,rule,phases_write,phases_read,presentation,load_write,writers,readers,note"
GLOBALS_HEADER = "address,name,section,size,size_from,kind,type,type_files,other_files,types,verdict,files,max_offset,ghidra,pointers,defined,init"


class TreeTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        saved = (sg.ROOT, sg.STATE_CLASSES, sg.GLOBALS, sg.SIM_HEADER)
        self.addCleanup(lambda: (setattr(sg, "ROOT", saved[0]), setattr(sg, "STATE_CLASSES", saved[1]),
                                 setattr(sg, "GLOBALS", saved[2]), setattr(sg, "SIM_HEADER", saved[3])))
        sg.ROOT = self.root
        sg.STATE_CLASSES = self.root / "state_classes.csv"
        sg.GLOBALS = self.root / "globals.csv"
        sg.SIM_HEADER = self.root / "src" / "state" / "sim_globals.h"

    def load(self, state, globals_):
        sg.STATE_CLASSES.write_text(STATE_HEADER + "\n" + "\n".join(state) + "\n")
        sg.GLOBALS.write_text(GLOBALS_HEADER + "\n" + "\n".join(globals_) + "\n")
        return sg.load_rows()

    def test_a_view_into_a_block_vector_is_left_alone(self):
        sg.TYPE_OVERRIDES["g_vec"] = "BlockVector<Thing>"
        self.addCleanup(sg.TYPE_OVERRIDES.pop, "g_vec")
        rows = self.load(
            ["g_vec,0x512340,global,,sim,,,,,,,,", "g_vecBegin,0x512344,global,,sim,,,,,,,,"],
            ["0x512340,g_vec,.bss,16,definition,data,int,1,0,1,one type,1,,,,SimGlobals,",
             "0x512344,g_vecBegin,.bss,4,type,data,char*,1,0,1,one type,1,,,,in g_vec+0x4,"])
        self.assertEqual([r.leaf for r in rows], ["g_vec"])
        self.assertNotIn("g_vecBegin", sg.header_text(rows))
        self.assertNotIn("g_vecBegin", sg.csv_text(rows))

    def test_a_view_into_a_plain_field_is_kept(self):
        rows = self.load(
            ["g_blob,0x512340,global,,sim,,,,,,,,", "g_blobTail,0x512344,global,,sim,,,,,,,,"],
            ["0x512340,g_blob,.bss,16,definition,data,int,1,0,1,one type,1,,,,SimGlobals,",
             "0x512344,g_blobTail,.bss,4,type,data,int,1,0,1,one type,1,,,,in g_blob+0x4,"])
        self.assertEqual([r.leaf for r in rows], ["g_blob", "g_blobTail"])
        self.assertIn("g_blobTail", sg.header_text(rows))

    def test_a_char_array_gets_a_string_initialiser(self):
        rows = self.load(
            ["g_name,0x503ea0,global,,sim,,,,,,,,"],
            ["0x503ea0,g_name,.data,8,definition,data,char[8],1,0,1,one type,1,,,,,6e6f6e6500000000"])
        self.assertIn('char g_name[8] = "none";', sg.header_text(rows))

    def test_a_string_initialiser_is_escaped(self):
        self.assertEqual(sg.string_literal(b'a"b\\c\x01\0z', 8), '"a\\"b\\\\c\\001"')

    def test_a_zeroed_char_array_has_no_initialiser(self):
        rows = self.load(
            ["g_buf,0x503ea0,global,,sim,,,,,,,,"],
            ["0x503ea0,g_buf,.data,8,definition,data,char[8],1,0,1,one type,1,,,,,0000000000000000"])
        self.assertIn("char g_buf[8];", sg.header_text(rows))

    def source(self, text):
        rows = self.load(
            ["g_seed,0x503ea0,global,,sim,,,,,,,,"],
            ["0x503ea0,g_seed,.data,4,definition,data,unsigned int,1,0,1,one type,1,,,,,00000000"])
        path = self.root / "src" / "harness" / "x.cpp"
        path.parent.mkdir(parents=True)
        path.write_text(text)
        sg.Tool(rows, False).rewrite_source("src/harness/x.cpp")
        return path.read_text()

    def test_a_push_macro_and_a_member_need_no_include(self):
        text = ('#include "a.h"\n\n#pragma push_macro("g_seed")\n#undef g_seed\n\n'
                "unsigned Seed()\n{\n    return g_simGlobals->g_seed;\n}\n\n"
                '#pragma pop_macro("g_seed")\n')
        self.assertEqual(self.source(text), text)

    def test_a_use_of_the_global_gets_the_include(self):
        text = '#include "a.h"\n\nunsigned Seed()\n{\n    return g_seed;\n}\n'
        self.assertIn('#include "../state/sim_globals.h"', self.source(text))


if __name__ == "__main__":
    unittest.main()
