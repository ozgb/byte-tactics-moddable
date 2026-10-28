"""Tests for subsystemsdoc.py, on a fixture doc and hand-written counts.

    uv run tools/test_subsystemsdoc.py
"""

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import subsystemsdoc as sd

COUNTS = {
    "alpha": {"functions": 16, "called": 11, "imported": 15, "targets": ["platform", "weapons", "files"], "written": 24,
              "target_sizes": {"platform": 12, "weapons": 2, "files": 1},
              "callers": {"Foo": {"game": 2, "map": 1}, "Bar": {"game": 1}}, "called_by": {"game": 2, "map": 1},
              "named": 19, "owned": 18, "shared": 1, "only_read": 10, "read_named": 9,
              "wrote_items": {"g_a", "g_b"}, "read_items": {"g_c"}},
    "beta": {"functions": 5, "called": 3, "imported": 7, "targets": ["platform", "alpha"], "written": 2,
             "target_sizes": {"platform": 4, "alpha": 3},
             "callers": {"Foo": {"game": 2, "map": 1}, "Bar": {"game": 1}}, "called_by": {"game": 2, "map": 1},
             "named": 0, "owned": 0, "shared": 0, "only_read": 0, "read_named": 0, "wrote_items": set(), "read_items": set()},
}
COVER = {
    "alpha": {"functions": 16, "entered": 15, "never": 1, "dead": 0, "fpct": "93.8%", "lpct": "96.4%", "bpct": "76.3%",
              "reasons": None},
    "beta": {"functions": 5, "entered": 3, "never": 2, "dead": 1, "fpct": "60.0%", "lpct": "50.0%", "bpct": "-",
             "reasons": {"network": ["SendA", "SendB"]}},
}
DOC = """# Subsystems

The 21 functions of `data/rwsets.csv` are in 2 subsystems.

| subsystem | functions | issue |
|---|---|---|
| [alpha](#alpha) | 16 | #9 |
| [beta](#the-remaining-subsystems) | 5 | none yet |

## alpha

Issue #9. 16 functions. The first.

Coverage: 93.8% of functions (15 of 16), 96.4% of lines, 76.3% of branches; 1 live function no scenario enters, and 0 more are dead (data/coverage.csv).

**Live functions no scenario enters** (1 of 16 after `x` (#1); 15 are entered, 93.8%), by reason:

- Shutdown only (1): `Free`.

11 functions are called from other subsystems. The most called, with their callers:

- `Foo` (2 from game, 1 from map)
- `Bar` (1 from game)
- By calling subsystem (functions called): game 2, map 1

**Imports.** 15 functions of 3 subsystems, by target, each with the number of functions it calls there:

- platform (12): `A`, `B`, `C`, `D`, and 8 more
- weapons (2): `E`, `F`
- 1 smaller target: files (1)

**Sim writes.** 24 items in all, 19 after leaving out the shared value types. The most used:

`g_a` (sim); `g_b` (mixed)

**Sim reads.** 10 items only read, 9 named. The most used:

`g_c` (sim)

**Draft interface.**

- Exports: 11 functions (all).
- Needs from seams: 15 functions in platform, weapons, files.
- Owns: 18 named sim items that only this subsystem writes, for example `g_a`.
- Shares: 1 named sim item that other subsystems also write, for example `g_b`.

## The remaining subsystems

**beta** (5 functions). No phase root. 3 functions are called from outside, mostly by game (2) and map (1); the busiest are `Foo` (3), `Bar` (1). It imports 7 functions: platform 4, alpha 3. Its packet handlers write sim items (2 in all). Coverage: 60.0% of functions (3 of 5), 50.0% of lines, - of branches; 2 live functions no scenario enters, and 1 more is dead (data/coverage.csv). The list of those 2 follows this paragraph.

**Live functions no scenario enters** (2 of 5 after `y` (#2); 3 are entered, 60.0%), by reason:

- Network only (2): `SendA`, `SendB`. Needs other machines.

## Coverage

| subsystem | entered of live functions | functions | lines | branches | never entered | dead |
|---|---|---|---|---|---|---|
| **alpha** | 15 of 16 | 93.8% | 96.4% | 76.3% | 1 | 0 |
| beta | 3 of 5 | 60.0% | 50.0% | - | 2 | 1 |
"""


def run_main(doc: Path, flag: str, counts: dict = COUNTS, cover: dict = COVER) -> tuple[int, str]:
    out = io.StringIO()
    code = 0
    with mock.patch.object(sd, "load_counts", return_value=(counts, 21, cover)), mock.patch.object(
            sys, "argv", ["subsystemsdoc.py", flag, "--doc", str(doc)]), contextlib.redirect_stdout(out):
        try:
            sd.main()
        except SystemExit as exit:
            code = exit.code if isinstance(exit.code, int) else 1
    return code, out.getvalue()


ORDERS_COUNTS = {"orders" if name == "alpha" else name: value for name, value in COUNTS.items()}
ORDERS_COUNTS["beta"] = dict(COUNTS["beta"], targets=["platform", "orders"], target_sizes={"platform": 4, "orders": 3})
ORDERS_COVER = {"orders" if name == "alpha" else name: dict(value) for name, value in COVER.items()}
ORDERS_COVER["orders"].update(entered=13, never=3, fpct="81.3%", reasons={"info-panel": ["GetOrderName", "Marked"],
                                                                         "no-caller": ["ReadyOrder"]})
ORDERS_DOC = (DOC.replace("alpha", "orders")
              .replace("1 live function no scenario enters, and 0 more", "3 live functions no scenario enters, and 0 more")
              .replace("93.8% of functions (15 of 16)", "81.3% of functions (13 of 16)")
              .replace("(1 of 16 after `x` (#1); 15 are entered, 93.8%)", "(3 of 16 after `x` (#1); 13 are entered, 81.3%)")
              .replace("| **orders** | 15 of 16 | 93.8% | 96.4% | 76.3% | 1 | 0 |", "| **orders** | 13 of 16 | 81.3% | 96.4% | 76.3% | 3 | 0 |")
              .replace("- Shutdown only (1): `Free`.",
                       "- Info panel, cursor, build menu and shutdown only (2): `GetOrderName`, `Marked`. The info panel and the smart click.\n"
                       "- No caller in the game (1): `ReadyOrder`. Nothing creates the order."))


class SortedSubsystemDocTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.path = Path(self.dir.name) / "subsystems.md"

    def check(self, doc: str) -> tuple[int, str]:
        self.path.write_text(doc)
        return run_main(self.path, "--check", ORDERS_COUNTS, ORDERS_COVER)

    def test_a_matching_orders_doc_passes(self):
        self.assertEqual(self.check(ORDERS_DOC)[0], 0)

    def test_a_renamed_orders_name_fails(self):
        self.assertIn("`Marked`", ORDERS_DOC)
        code, out = self.check(ORDERS_DOC.replace("`Marked`", "`Marker`", 1))
        self.assertEqual(code, 1)
        self.assertIn("orders, reason info-panel names: doc has extra: Marker, tool has absent: Marked", out)

    def test_a_count_moved_between_orders_bullets_fails(self):
        doc = (ORDERS_DOC.replace("shutdown only (2)", "shutdown only (1)")
               .replace("No caller in the game (1)", "No caller in the game (2)"))
        code, out = self.check(doc)
        self.assertEqual(code, 1)
        self.assertIn("orders, reason info-panel count: doc has 1, tool has 2", out)
        self.assertIn("orders, reason no-caller count: doc has 2, tool has 1", out)


class SubsystemsDocTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.path = Path(self.dir.name) / "subsystems.md"

    def fails(self, old: str, new: str, message: str) -> None:
        """Changing old to new in the doc makes --check fail with the message, and --write alone does not restore it if unwritable."""
        self.assertIn(old, DOC)
        self.path.write_text(DOC.replace(old, new, 1))
        code, out = run_main(self.path, "--check")
        self.assertEqual(code, 1)
        self.assertIn(message, out)

    def test_a_matching_doc_passes(self):
        self.path.write_text(DOC)
        self.assertEqual(run_main(self.path, "--check")[0], 0)

    def test_a_wrong_count_names_its_section(self):
        self.fails("**Imports.** 15 functions", "**Imports.** 13 functions", "alpha, imported functions and subsystems: doc has 13, tool has 15")

    def test_a_wrong_paragraph_count_names_its_section(self):
        self.fails("(2 in all)", "(9 in all)", "beta, sim writes: doc has 9, tool has 2")

    def test_a_wrong_overall_total_fails(self):
        self.fails("The 21 functions", "The 22 functions", "overall, function total: doc has 22, tool has 21")

    def test_a_different_set_of_names_fails(self):
        self.fails("platform, weapons, files.", "platform, map, files.", "alpha, needs from seams")

    def test_the_same_names_in_another_order_pass(self):
        self.path.write_text(DOC.replace("platform, weapons, files.", "files, weapons, platform."))
        self.assertEqual(run_main(self.path, "--check")[0], 0)

    def test_a_subsystem_without_a_section_fails(self):
        self.fails("**beta** (5 functions)", "**gamma** (5 functions)", "beta, section")

    def test_a_wrong_import_count_of_a_target_fails(self):
        self.fails("- platform (12):", "- platform (13):", "alpha, imports from platform: doc has 13, tool has 12")

    def test_a_wrong_and_more_count_fails(self):
        self.fails("and 8 more", "and 9 more", "alpha, imports from platform, and more: doc has 9, tool has 8")

    def test_a_wrong_count_of_a_smaller_target_fails(self):
        self.fails("files (1)", "files (2)", "alpha, imports from files: doc has 2, tool has 1")

    def test_a_wrong_number_of_smaller_targets_fails(self):
        self.fails("- 1 smaller target:", "- 2 smaller target:", "alpha, smaller import targets: doc has 2, tool has 1")

    def test_import_targets_out_of_order_fail(self):
        self.fails("- platform (12): `A`, `B`, `C`, `D`, and 8 more\n- weapons (2): `E`, `F`\n",
                   "- weapons (2): `E`, `F`\n- platform (12): `A`, `B`, `C`, `D`, and 8 more\n", "alpha, import targets in order")

    def test_a_wrong_caller_count_fails(self):
        self.fails("(2 from game, 1 from map)", "(3 from game, 1 from map)", "alpha, callers of Foo: doc has 3 from game, 1 from map")

    def test_a_function_nothing_calls_fails(self):
        self.fails("`Bar` (1 from game)", "`Baz` (1 from game)", "alpha, callers of Baz")

    def test_a_wrong_count_by_calling_subsystem_fails(self):
        self.fails("game 2, map 1\n", "game 3, map 1\n", "alpha, functions called, by calling subsystem")

    def test_a_wrong_named_write_count_fails(self):
        self.fails("19 after leaving out", "20 after leaving out", "alpha, named sim writes: doc has 20, tool has 19")

    def test_a_wrong_sim_read_count_fails(self):
        self.fails("10 items only read", "11 items only read", "alpha, sim reads: doc has 11, tool has 10")
        self.fails("9 named", "8 named", "alpha, sim reads: doc has 8, tool has 9")

    def test_a_wrong_owned_or_shared_count_fails(self):
        self.fails("Owns: 18 named", "Owns: 17 named", "alpha, owned sim items: doc has 17, tool has 18")
        self.fails("Shares: 1 named", "Shares: 2 named", "alpha, shared sim items: doc has 2, tool has 1")

    def test_a_sim_item_the_subsystem_does_not_access_fails(self):
        self.fails("`g_a` (sim); `g_b` (mixed)", "`g_z` (sim); `g_b` (mixed)", "alpha, sim item names: doc has g_z")

    def test_a_wrong_busiest_count_fails(self):
        self.fails("`Foo` (3), `Bar` (1)", "`Foo` (4), `Bar` (1)", "beta, busiest Foo: doc has 4, tool has 3")

    def test_a_wrong_count_of_functions_called_by_a_subsystem_fails(self):
        self.fails("mostly by game (2) and map (1)", "mostly by game (3) and map (1)", "beta, functions called by game: doc has 3, tool has 2")

    def test_a_wrong_count_of_imports_from_a_subsystem_fails(self):
        self.fails("platform 4, alpha 3", "platform 5, alpha 3", "beta, imports from platform: doc has 5, tool has 4")

    def test_a_wrong_coverage_line_fails(self):
        self.fails("(15 of 16)", "(14 of 16)", "alpha, functions entered: doc has 14, tool has 15")
        self.fails("96.4% of lines", "90.0% of lines", "alpha, line coverage: doc has 90.0%, tool has 96.4%")
        self.fails("1 more is dead", "3 more is dead", "beta, dead functions: doc has 3, tool has 1")

    def test_a_wrong_never_entered_header_fails(self):
        self.fails("(1 of 16 after", "(2 of 16 after", "alpha, functions never entered: doc has 2, tool has 1")
        self.fails("3 are entered, 60.0%", "4 are entered, 60.0%", "beta, functions entered: doc has 4, tool has 3")

    def test_a_wrong_list_length_fails(self):
        self.fails("The list of those 2", "The list of those 3", "beta, list of functions never entered: doc has 3, tool has 2")

    def test_reason_bullets_that_do_not_add_up_fail(self):
        self.fails("Shutdown only (1)", "Shutdown only (2)", "alpha, reason bullets in all: doc has 2, tool has 1")

    def test_a_wrong_reason_count_fails(self):
        self.fails("Network only (2)", "Network only (3)", "beta, reason network count: doc has 3, tool has 2")

    def test_a_wrong_reason_name_fails(self):
        self.fails("`SendB`.", "`SendC`.", "beta, reason network names: doc has extra: SendC, tool has absent: SendB")

    def test_a_name_in_parentheses_is_not_a_listed_name(self):
        self.path.write_text(DOC.replace("`SendB`. Needs", "`SendB` (called by `SendZ`). Needs"))
        self.assertEqual(run_main(self.path, "--check")[0], 0)

    def test_a_wrong_coverage_table_row_fails(self):
        self.fails("| 15 of 16 |", "| 14 of 16 |", "alpha, functions entered: doc has 14, tool has 15")
        self.fails("| 60.0% | 50.0% |", "| 60.0% | 55.0% |", "beta, line coverage: doc has 55.0%, tool has 50.0%")
        self.fails("| 2 | 1 |\n", "| 3 | 1 |\n", "beta, functions never entered: doc has 3, tool has 2")

    def test_a_subsystem_without_a_coverage_row_fails(self):
        self.fails("| beta | 3 of 5 | 60.0% | 50.0% | - | 2 | 1 |\n", "", "beta, coverage table row")

    def test_write_fixes_the_doc_and_changes_only_numbers(self):
        wrong = DOC
        for old, new in (("16 functions. The", "17 functions. The"), ("Exports: 11", "Exports: 12"), ("5 functions)", "6 functions)"),
                         ("| 16 |", "| 15 |"), ("- platform (12)", "- platform (13)"), ("and 8 more", "and 9 more"),
                         ("files (1)", "files (2)"), ("(2 from game, 1 from map)", "(3 from game, 1 from map)"),
                         ("game 2, map 1\n", "game 3, map 1\n"), ("19 after", "20 after"), ("10 items only", "11 items only"),
                         ("Owns: 18", "Owns: 17"), ("`Foo` (3)", "`Foo` (4)"), ("by game (2)", "by game (3)"),
                         ("platform 4, alpha 3", "platform 5, alpha 4"), ("(15 of 16)", "(14 of 16)"), ("93.8% of func", "90.0% of func"),
                         ("(1 of 16 after", "(2 of 16 after"), ("those 2", "those 3"), ("Network only (2)", "Network only (3)"),
                         ("| 15 of 16 |", "| 14 of 16 |"), ("| 2 | 1 |\n", "| 3 | 1 |\n")):
            self.assertIn(old, wrong)
            wrong = wrong.replace(old, new, 1)
        self.path.write_text(wrong)
        code, _ = run_main(self.path, "--write")
        self.assertEqual(code, 0)
        self.assertEqual(self.path.read_text(), DOC)
        self.assertEqual(run_main(self.path, "--check")[0], 0)

    def test_write_does_not_hide_a_name_it_cannot_fix(self):
        self.path.write_text(DOC.replace("`SendB`.", "`SendC`."))
        self.assertEqual(run_main(self.path, "--write")[0], 1)


if __name__ == "__main__":
    unittest.main()
