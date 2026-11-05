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


class Main(unittest.TestCase):
    def run_main(self, header, doc):
        with tempfile.TemporaryDirectory() as tmp:
            h, d = Path(tmp) / "services.h", Path(tmp) / "sim-events.md"
            h.write_text(header)
            if doc is not None:
                d.write_text(doc)
            out = io.StringIO()
            with mock.patch("sys.argv", ["simeventsdoc", "--check", "--header", str(h), "--doc", str(d)]):
                with contextlib.redirect_stdout(out):
                    try:
                        sd.main()
                        code = 0
                    except SystemExit as exc:
                        code = exc.code
            return code, out.getvalue()

    def test_passes(self):
        code, out = self.run_main(HEADER, GOOD)
        self.assertEqual(code, 0)
        self.assertIn("3 kinds", out)

    def test_fails_naming_each_problem(self):
        code, out = self.run_main(HEADER, "### SIM_EVENT_NONE\n### SIM_EVENT_GHOST\n")
        self.assertEqual(code, 1)
        self.assertIn("SIM_EVENT_SOUND has no section", out)
        self.assertIn("SIM_EVENT_SMOKE has no section", out)
        self.assertIn("SIM_EVENT_GHOST", out)

    def test_missing_doc_fails(self):
        self.assertEqual(self.run_main(HEADER, None)[0], 1)


if __name__ == "__main__":
    unittest.main()
