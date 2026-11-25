"""Tests for ftollint.py, on a hand-written disassembly excerpt and map.

    uv run tools/test_ftollint.py
"""

import unittest

import ftollint

MAP = """ Address         Publics by Value              Rva+Base               Lib:Object

 0001:00001000       ?EconomyUpdatePlayer@@YGXPAUPlayer@@@Z 0000000000401000     0001_economy.cpp.obj
 0001:00001100       ?DrawMenu@@YGXXZ           0000000000401100     0002_gui.cpp.obj
 0001:00001200       _bx_ftol                   0000000000401200     bitexact:ftol.c.obj
 0001:00001300       __ftol2                    0000000000401300     LIBCMT:..\\build\\intel\\mt_obj\\ftol.obj
"""

ANNOTATIONS = [(0x401360, "EconomyUpdatePlayer"), (0x4a0000, "DrawMenu")]


def lint(game: str):
    names = ftollint.game_names({0x401360}, {"UpdatePlayerEconomy"}, ANNOTATIONS)
    return ftollint.lint(game, MAP, names)


class FtolLintTest(unittest.TestCase):
    def test_call_to_bx_ftol_passes(self):
        report, problems = lint("  401000:      	calll	0x401200 <.text+0x200>\n"
                                "  401200:      	fistpll	(%esp)\n"
                                "  401300:      	fistpll	(%esp)\n")
        self.assertEqual(problems, [])
        self.assertEqual(report, [])

    def test_inline_fistp_in_game_logic_fails(self):
        _, problems = lint("  401000:      	fistpl	-4(%ebp)\n  401004:      	fistpl	-8(%ebp)\n")
        self.assertEqual(problems, ["EconomyUpdatePlayer converts inline (fistpl x2), not through bx_ftol"])

    def test_sse_conversion_in_game_logic_fails(self):
        _, problems = lint("  401000:      	cvttsd2si	%xmm0, %eax\n")
        self.assertEqual(len(problems), 1)
        self.assertIn("cvttsd2si", problems[0])

    def test_presentation_conversion_passes(self):
        report, problems = lint("  401100:      	fistpl	-4(%ebp)\n")
        self.assertEqual(problems, [])
        self.assertEqual(report, ["fistpl x1 in DrawMenu (0002_gui.cpp.obj): not game logic"])

    def test_other_instructions_pass(self):
        _, problems = lint("  401000:      	fildl	-4(%ebp)\n  401004:      	fstpl	(%esp)\n  401008:      	cvtsi2sd	%eax, %xmm0\n")
        self.assertEqual(problems, [])

    def test_name_from_the_data_files(self):
        names = ftollint.game_names(set(), {"DrawMenu"}, ANNOTATIONS)
        _, problems = ftollint.lint("  401100:      	fistpl	-4(%ebp)\n", MAP, names)
        self.assertEqual(len(problems), 1)


if __name__ == "__main__":
    unittest.main()
