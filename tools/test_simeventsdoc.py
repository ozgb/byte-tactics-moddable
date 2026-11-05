"""Tests for simeventsdoc.py, on synthetic inputs.

    uv run tools/test_simeventsdoc.py
"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import simeventsdoc as sd

HEADER = """\
// The kinds.
enum SimEventKind : unsigned short {
    // Marks an empty record.
    SIM_EVENT_NONE = 0,
    // A sound.
    SIM_EVENT_SOUND = 2,
    SIM_EVENT_SMOKE = 27,
};
enum Other { SIM_EVENT_NOT_A_KIND = 1, };
"""

GOOD = "# T\n\n### SIM_EVENT_NONE\n\ntext\n\n## SIM_EVENT_SOUND\n\n### SIM_EVENT_SMOKE\n"
WITH_PHASES = "".join("### %s\nPhases: none.\n\n" % n for n in ["SIM_EVENT_NONE", "SIM_EVENT_SOUND", "SIM_EVENT_SMOKE"])


class Kinds(unittest.TestCase):
    def test_enumerators_in_order(self):
        self.assertEqual(sd.kinds(HEADER), ["SIM_EVENT_NONE", "SIM_EVENT_SOUND", "SIM_EVENT_SMOKE"])

    def test_commented_name_is_not_a_kind(self):
        self.assertEqual(sd.kinds("enum SimEventKind { // SIM_EVENT_X = 1,\n SIM_EVENT_A = 1, };"), ["SIM_EVENT_A"])


class Problems(unittest.TestCase):
    names = ["SIM_EVENT_NONE", "SIM_EVENT_SOUND", "SIM_EVENT_SMOKE"]

    def test_clean(self):
        self.assertEqual(sd.problems(self.names, GOOD), [])

    def test_missing_section(self):
        got = sd.problems(self.names, GOOD.replace("### SIM_EVENT_SMOKE", "### Smoke"))
        self.assertEqual(len(got), 1)
        self.assertIn("SIM_EVENT_SMOKE has no section", got[0])

    def test_section_names_no_kind(self):
        got = sd.problems(self.names, GOOD + "\n### SIM_EVENT_GHOST\n")
        self.assertEqual(len(got), 1)
        self.assertIn("SIM_EVENT_GHOST", got[0])

    def test_duplicate_section(self):
        got = sd.problems(self.names, GOOD + "\n### SIM_EVENT_SOUND\n")
        self.assertEqual(got, ["kind SIM_EVENT_SOUND has 2 sections"])

    def test_extra_text_on_heading_does_not_count(self):
        got = sd.problems(self.names, GOOD.replace("### SIM_EVENT_SMOKE", "### SIM_EVENT_SMOKE, value 27"))
        self.assertTrue(any("SIM_EVENT_SMOKE has no section" in m for m in got))

    def test_level_one_and_four_do_not_count(self):
        doc = "# SIM_EVENT_NONE\n#### SIM_EVENT_SOUND\n### SIM_EVENT_SMOKE\n"
        self.assertEqual(len(sd.problems(self.names, doc)), 2)

    def test_fenced_heading_is_ignored(self):
        doc = GOOD.replace("### SIM_EVENT_SMOKE\n", "```\n### SIM_EVENT_SMOKE\n```\n")
        self.assertEqual(len(sd.problems(self.names, doc)), 1)


SOURCES = {
    "src/state/sim_effects.h": "inline void RecordSound(int a)\n{\n    RecordSimEvent(SIM_EVENT_SOUND, 0, 0, 0);\n}\n"
                               "inline void Helper(int kind)\n{\n    RecordSimEvent(kind, 0, 0, 0);\n}\n"
                               "inline void RecordSmoke(int a)\n{\n    Helper(SIM_EVENT_SMOKE);\n}\n",
    "src/ingame/sim_event_drain.cpp": "void DrainSimEvents()\n{\n    switch (k) { case SIM_EVENT_NONE: break; }\n}\n",
}
ROWS = [
    ["", "Root", "src/a.cpp", "function", ""], ["", "Root", "src/a.cpp", "call", "Leaf"],
    ["", "Leaf", "src/a.cpp", "function", ""], ["", "Leaf", "src/a.cpp", "call", "RecordSound"],
    ["", "Other", "src/a.cpp", "function", ""], ["", "Other", "src/a.cpp", "call", "RecordSmoke"],
    ["", "RecordSound", "src/state/sim_effects.h", "function", ""],
]
NAMES = ["SIM_EVENT_NONE", "SIM_EVENT_SOUND", "SIM_EVENT_SMOKE"]


class Recorders(unittest.TestCase):
    def test_bodies_hold_the_kind(self):
        got = sd.recorders(SOURCES, NAMES)
        self.assertEqual(got["SIM_EVENT_SOUND"], {"RecordSound"})
        self.assertEqual(got["SIM_EVENT_SMOKE"], {"RecordSmoke"})

    def test_the_drain_records_nothing(self):
        self.assertEqual(sd.recorders(SOURCES, NAMES)["SIM_EVENT_NONE"], set())

    def test_comments_do_not_count(self):
        src = {"src/x.h": "void F()\n{\n    // SIM_EVENT_SOUND\n}\n"}
        self.assertEqual(sd.recorders(src, NAMES)["SIM_EVENT_SOUND"], set())


class Phases(unittest.TestCase):
    def setUp(self):
        self.patch = mock.patch.object(sd.sc, "Walks")
        walks = self.patch.start().return_value
        walks.stops = set()
        self.addCleanup(self.patch.stop)

    def phases(self, roots):
        with mock.patch.object(sd.sc, "PHASE_LABELS", roots), mock.patch.object(sd.sc, "LOAD_ROOTS", []):
            return sd.computed_phases(NAMES, SOURCES, ROWS, [])

    def test_a_root_reaches_a_recorder_through_a_call(self):
        got = self.phases({"Root": "1"})
        self.assertEqual(got["SIM_EVENT_SOUND"], ["units (1)"])

    def test_unreached_recorder_is_outside_the_steps(self):
        got = self.phases({"Root": "1"})
        self.assertEqual(got["SIM_EVENT_SMOKE"], ["outside the steps"])

    def test_extra_edge_adds_a_phase(self):
        with mock.patch.object(sd.sc, "PHASE_LABELS", {"Root": "1", "Other": "4"}), mock.patch.object(sd.sc, "LOAD_ROOTS", []):
            got = sd.computed_phases(NAMES, SOURCES, ROWS, [])
        self.assertEqual(got["SIM_EVENT_SMOKE"], ["players (4)"])
        self.assertEqual(got["SIM_EVENT_NONE"], [])


class PhaseLines(unittest.TestCase):
    computed = {"SIM_EVENT_SOUND": ["units (1)", "match load"], "SIM_EVENT_NONE": []}

    def test_match(self):
        doc = "### SIM_EVENT_SOUND\n\nPhases: units (1), match load.\n\n### SIM_EVENT_NONE\nPhases: none.\n"
        self.assertEqual(sd.phase_problems(self.computed, doc), [])

    def test_difference_names_both(self):
        doc = "### SIM_EVENT_SOUND\nPhases: units (1).\n"
        got = sd.phase_problems(self.computed, doc)
        self.assertEqual(len(got), 1)
        self.assertIn("units (1), match load.", got[0])

    def test_missing_line(self):
        got = sd.phase_problems(self.computed, "### SIM_EVENT_SOUND\ntext\n")
        self.assertIn("no `Phases:` line", got[0])

    def test_phases_line_of_another_section_does_not_leak(self):
        doc = "### SIM_EVENT_SOUND\ntext\n\n## Tables\nPhases: units (1), match load.\n"
        self.assertIn("no `Phases:` line", sd.phase_problems(self.computed, doc)[0])


class Main(unittest.TestCase):
    phases = {"SIM_EVENT_NONE": [], "SIM_EVENT_SOUND": [], "SIM_EVENT_SMOKE": []}

    def run_main(self, header, doc):
        with tempfile.TemporaryDirectory() as tmp:
            h, d = Path(tmp) / "services.h", Path(tmp) / "sim-events.md"
            h.write_text(header)
            if doc is not None:
                d.write_text(doc)
            out = io.StringIO()
            with mock.patch("sys.argv", ["simeventsdoc", "--check", "--header", str(h), "--doc", str(d)]), \
                    mock.patch.object(sd, "computed_phases", return_value=self.phases), \
                    mock.patch.object(sd.hazards, "load_graph", return_value=([], mock.Mock(functions=set()))), \
                    mock.patch.object(sd.hazards, "read_edges", return_value=[]), \
                    mock.patch.object(sd.coupling, "virtual_inputs", return_value=([],)), \
                    mock.patch.object(sd.coupling.Classes, "read", return_value=mock.Mock(bases={}, virtuals={})):
                with contextlib.redirect_stdout(out):
                    try:
                        sd.main()
                        code = 0
                    except SystemExit as exc:
                        code = exc.code
            return code, out.getvalue()

    def test_passes(self):
        code, out = self.run_main(HEADER, WITH_PHASES)
        self.assertEqual(code, 0)
        self.assertIn("3 kinds", out)

    def test_fails_naming_each_problem(self):
        code, out = self.run_main(HEADER, "### SIM_EVENT_NONE\n### SIM_EVENT_GHOST\n")
        self.assertEqual(code, 1)
        self.assertIn("SIM_EVENT_SOUND has no section", out)
        self.assertIn("SIM_EVENT_SMOKE has no section", out)
        self.assertIn("SIM_EVENT_GHOST", out)

    def test_a_wrong_phases_line_fails(self):
        code, out = self.run_main(HEADER, WITH_PHASES.replace("Phases: none.", "Phases: units (1).", 1))
        self.assertEqual(code, 1)
        self.assertIn("call graph gives", out)

    def test_missing_doc_fails(self):
        self.assertEqual(self.run_main(HEADER, None)[0], 1)


if __name__ == "__main__":
    unittest.main()
