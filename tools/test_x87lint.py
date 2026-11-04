"""Tests for x87lint.py, on a hand-written disassembly excerpt and map.

    uv run tools/test_x87lint.py
"""

import unittest

import x87lint

MAP = """ Address         Publics by Value              Rva+Base               Lib:Object

 0001:00001000       ?Game@@YAXXZ               0000000000401000     0001_game.obj
 0001:00001100       ?Trig@@YAXXZ               0000000000401100     0002_trig.obj
 0001:00001200       ?HandleReportClick@@YGXPAUGui@@@Z 0000000000401200     0003_multi.cpp.obj
 0001:00001300       ?Indirect@@YAXXZ           0000000000401300     0004_misc.obj
 0001:00001400       _pow                       0000000000401400     LIBCMT:..\\build\\intel\\mt_obj\\pow.obj
 0001:00001500       __twoToTOS                 0000000000401500     LIBCMT:..\\build\\intel\\mt_obj\\common.obj
 0001:00001600       _sqrt                      0000000000401600     LIBCMT:..\\build\\intel\\mt_obj\\sqrt.obj
 0001:00001700       _time                      0000000000401700     LIBCMT:build\\intel\\mt_obj\\time.obj
 0001:00001800       _atan2                     0000000000401800     LIBCMT:..\\build\\intel\\mt_obj\\87ctriga.obj
 0003:00000100       __tzname                   0000000000501000     LIBCMT:build\\intel\\mt_obj\\timeset.obj
 0003:00000110       __OP_ATAN2jmptab           0000000000501010     LIBCMT:..\\build\\intel\\mt_obj\\87triga.obj
 0003:00000130       __pctype                   0000000000501030     LIBCMT:build\\intel\\mt_obj\\ctype.obj
"""

LIBCMT = """  401400:      	pushl	%ebp
  401403:      	fyl2x
  401405:      	calll	0x401500 <.text+0x500>
  401500:      	f2xm1
  401600:      	fsqrt
"""

TABLE = """ 501000 00000000 00000000 00000000 00000000  ................
 501010 05617461 6e320000 00000000 00154000  .atan2..........
 501020 00144000 00000000 00000000 00000000  ................
"""


def lint(game: str, subsystems: dict[str, str] | None = None):
    return x87lint.lint(game + LIBCMT, MAP, subsystems if subsystems is not None else {"HandleReportClick": "presentation"},
                        x87lint.parse_data("Contents of section .data:\n" + TABLE))


class X87LintTest(unittest.TestCase):
    def test_clean_build_passes(self):
        report, problems = lint("  401000:      	nop\n  401001:      	fsqrt\n")
        self.assertEqual(problems, [])
        self.assertIn("CPU-dependent LIBCMT functions: _twoToTOS, pow", report)

    def test_fpatan_outside_libcmt_fails(self):
        _, problems = lint("  401100:      	fpatan\n")
        self.assertEqual(len(problems), 1)
        self.assertIn("fpatan outside LIBCMT: Trig", problems[0])

    def test_game_function_calling_pow_fails(self):
        _, problems = lint("  401000:      	calll	0x401400 <.text+0x400>\n")
        self.assertEqual(len(problems), 1)
        self.assertIn("Game (0001_game.obj) calls pow", problems[0])

    def test_jump_into_the_set_fails(self):
        _, problems = lint("  401000:      	jmp	0x401400 <.text+0x400>\n")
        self.assertEqual(len(problems), 1)

    def test_call_through_the_set_fails(self):
        _, problems = lint("  401000:      	calll	0x401500 <.text+0x500>\n")
        self.assertIn("Game (0001_game.obj) calls _twoToTOS", problems[0])

    def test_allowed_caller_passes(self):
        report, problems = lint("  401200:      	calll	0x401400 <.text+0x400>\n")
        self.assertEqual(problems, [])
        self.assertIn("HandleReportClick calls pow: allowed (presentation: the multiplayer report screen), subsystem presentation", report)

    def test_allowed_caller_in_a_simulation_subsystem_fails(self):
        _, problems = lint("  401200:      	calll	0x401400 <.text+0x400>\n", {"HandleReportClick": "weapons"})
        self.assertEqual(len(problems), 1)
        self.assertIn("puts it in weapons", problems[0])

    def test_allowed_caller_missing_from_subsystems_fails(self):
        _, problems = lint("  401200:      	calll	0x401400 <.text+0x400>\n", {})
        self.assertIn("no subsystem", problems[0])

    def test_indirect_call_is_not_followed(self):
        _, problems = lint("  401300:      	calll	*0x401000\n")
        self.assertEqual(problems, [])

    def test_libcmt_function_calling_libcmt_is_not_a_caller(self):
        _, problems = lint("  401700:      	calll	0x401400 <.text+0x400>\n")
        self.assertEqual(problems, [])

    def test_table_with_a_cpu_dependent_function_makes_its_user_a_caller(self):
        _, problems = lint("  401800:      \tmovl\t$0x501010, %edx\n"
                           "  401000:      \tcalll\t0x401800 <.text+0x800>\n")
        self.assertIn("Game (0001_game.obj) calls atan2, which reaches", problems[0])

    def test_table_words_stop_at_the_next_data_symbol(self):
        _, problems = lint("  401800:      \tmovl\t$0x501000, %edx\n"
                           "  401000:      \tcalll\t0x401800 <.text+0x800>\n")
        self.assertEqual(problems, [])

    def test_function_without_a_public_symbol_takes_the_next_objects(self):
        disasm = "  401000:      	nop\n  401001:      	retl\n"
        disasm += "  401010:      	nop\n  401011:      	nop\n"
        disasm += "  401020:      	fyl2x\n"
        code = x87lint.parse_map(MAP)
        instructions, _ = x87lint.scan(disasm, code)
        self.assertEqual([(f.plain, f.obj) for f, _ in instructions], [("unnamed_401020", "0002_trig.obj")])


if __name__ == "__main__":
    unittest.main()
