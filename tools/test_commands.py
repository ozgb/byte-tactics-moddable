"""Tests for commands.py.

    python3 tools/test_commands.py
"""

import tempfile
import unittest
from pathlib import Path

import commands

DOC = """# Doc

| Command | Group | Class | Events |
| --- | --- | --- | --- |
| `give` | events | always | X |
| `armpw`, `corak` | converted | cheat | Y |
| `control` | direct | cheat | none |

| Order | Mode | Target | Events |
| --- | --- | --- | --- |
| `move` | 2 | yes | Z |

| Interface event | Paths | Scenarios |
| --- | --- | --- |
| `ORDERS_CANCEL` | a key | b |

## The scenarios of the core's verdict

| Scenario | Verdict | Reason |
| --- | --- | --- |
| a | judged | - |
| b | left out | uses control |
"""
README = "# R\n\n" + DOC[DOC.index("## The scenarios"):]


LOGS = {"b": "bt-interface 1\n12 ORDERS_CANCEL unit 1 type 4\n"}


def tree(tmp: str, scenarios: dict, doc: str = DOC, readme: str = README, logs: dict = LOGS) -> Path:
    root = Path(tmp)
    (root / "docs").mkdir()
    (root / "reference" / "scenarios").mkdir(parents=True)
    (root / "docs" / "commands.md").write_text(doc)
    (root / "reference" / "README.md").write_text(readme)
    for name, text in scenarios.items():
        (root / "reference" / "scenarios" / (name + ".txt")).write_text(text)
    (root / "reference" / "commands").mkdir()
    for name, text in logs.items():
        (root / "reference" / "commands" / (name + ".txt")).write_text(text)
    return root


GOOD = {"a": "ticks 5\norder 1 0:X:1 +move 1 2\nconsole 1 +give 1 2 metal\nconsole 2 +ARMPW 0\n",
        "b": "console 1 +control 1\n"}


class CommandsTest(unittest.TestCase):
    def problems(self, scenarios, **kw):
        with tempfile.TemporaryDirectory() as tmp:
            return commands.check(tree(tmp, scenarios, **kw))

    def test_a_consistent_tree_passes(self):
        self.assertEqual(self.problems(GOOD), [])

    def test_an_unlisted_console_command_fails(self):
        bad = dict(GOOD, a="console 3 +frobnicate\n")
        out = self.problems(bad)
        self.assertEqual(len(out), 1)
        self.assertIn("a.txt:1", out[0])
        self.assertIn("frobnicate", out[0])

    def test_an_unlisted_order_kind_fails(self):
        out = self.problems(dict(GOOD, a="order 1 0:X:1 +dance 1 2\n"))
        self.assertEqual(len(out), 1)
        self.assertIn("dance", out[0])

    def test_a_scenario_missing_from_the_tables_fails(self):
        out = self.problems(dict(GOOD, c="ticks 1\n"))
        self.assertEqual(len(out), 2)

    def test_a_listed_scenario_that_does_not_exist_fails(self):
        out = self.problems({"a": GOOD["a"]})
        self.assertEqual(len(out), 2)

    def test_the_two_tables_must_agree(self):
        readme = README.replace("| b | left out | uses control |", "| b | left out | other |")
        out = self.problems(GOOD, readme=readme)
        self.assertEqual(len(out), 1)
        self.assertIn("differ", out[0])

    def test_a_duplicate_row_fails(self):
        doc = DOC + "| b | left out | again |\n"
        self.assertTrue(any("twice" in p for p in self.problems(GOOD, doc=doc)))

    def test_a_judged_scenario_with_a_direct_command_fails(self):
        doc = DOC.replace("| a | judged | - |", "| a | judged | - |\n| a2 | judged | - |")
        both = (doc, "# R\n\n" + doc[doc.index("## The scenarios"):])
        out = self.problems(dict(GOOD, a2="console 1 +control 0\n"), doc=both[0], readme=both[1])
        self.assertEqual(len(out), 1)
        self.assertIn("judged", out[0])

    def test_a_judged_scenario_with_an_input_line_fails(self):
        out = self.problems(dict(GOOD, a="input 3 key down a\n"))
        self.assertEqual(len(out), 1)
        self.assertIn("input", out[0])

    def test_a_left_out_scenario_needs_a_reason(self):
        doc = DOC.replace("uses control", "-")
        out = self.problems(GOOD, doc=doc, readme="# R\n\n" + doc[doc.index("## The scenarios"):])
        self.assertTrue(any("no reason" in p for p in out))

    def test_an_interface_event_in_no_stored_log_fails(self):
        out = self.problems(GOOD, logs={})
        self.assertEqual(len(out), 1)
        self.assertIn("ORDERS_CANCEL", out[0])

    def test_an_interface_event_missing_from_a_named_log_fails(self):
        out = self.problems(GOOD, logs={"a": LOGS["b"]})
        self.assertEqual(len(out), 1)
        self.assertIn("commands/b.txt", out[0])

    def test_an_interface_event_with_no_scenario_needs_no_log(self):
        doc = DOC.replace("| `ORDERS_CANCEL` | a key | b |", "| `ORDERS_CANCEL` | a key | b |\n| `UNITS_FORGET_ATTACKER` | a key | none |")
        self.assertEqual(self.problems(GOOD, doc=doc), [])

    def test_a_logged_event_with_no_scenario_fails(self):
        doc = DOC.replace("| a key | b |", "| a key | none |")
        out = self.problems(GOOD, doc=doc)
        self.assertEqual(len(out), 1)
        self.assertIn("has no scenario", out[0])

    def test_a_logged_event_the_table_leaves_out_fails(self):
        out = self.problems(GOOD, logs={"b": LOGS["b"] + "13 PLAYERS_LEAVE_GAME unit -\n"})
        self.assertEqual(len(out), 1)
        self.assertIn("PLAYERS_LEAVE_GAME", out[0])

    def test_an_unsorted_table_fails(self):
        doc = DOC.replace("| a | judged | - |\n| b | left out | uses control |", "| b | left out | uses control |\n| a | judged | - |")
        out = self.problems(GOOD, doc=doc, readme="# R\n\n" + doc[doc.index("## The scenarios"):])
        self.assertTrue(any("sorted" in p for p in out))


if __name__ == "__main__":
    unittest.main()
