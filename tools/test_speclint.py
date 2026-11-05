"""Tests for speclint.py: each refusal rule on a planted example, and a clean spec and the template passing.

    uv run tools/test_speclint.py
"""

import json
import tempfile
import unittest
from pathlib import Path

import speclint

SOURCE = """\
#include "meteors.h"
// FUNCTION: 0x437de0
void UpdateMeteorShower(int tickCount)
{
    if (g_meteorNextStrike <= (unsigned int)tickCount) {
        localTargetZ = (int)((__int64)GameRand(RAND_STREAM_MAIN) * g_game->height / 0x8000);
    }
    StartScript(script, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
    FatalError("Hey, hoser!  The default meteor shower data was bogus!");
}
"""

KIT_DOC = """\
# Kit
The fields `SimGlobals.g_meteorNextStrike` and `Game.height`, the service `GameRand`, and the stream
`RAND_STREAM_MAIN` are documented here.
"""

CLEAN = """\
# example

## 4. Behaviour

When `g_meteorNextStrike` is due, one draw from `GameRand(main)` gives *d*, and the target z becomes
d times `Game.height` divided by 32768. The message is `Hey, hoser!  The default meteor shower data was bogus!`.

```text
call 1 UpdateMeteors
```
"""


class Lint(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        (root / "src" / "map").mkdir(parents=True)
        (root / "src" / "map" / "meteors.cpp").write_text(SOURCE)
        self.kit_dir = root / "kit"
        (self.kit_dir / "docs").mkdir(parents=True)
        (self.kit_dir / "docs" / "kit.md").write_text(KIT_DOC)
        (self.kit_dir / "MANIFEST.json").write_text(json.dumps({"files": []}))
        self.root = root
        self.ids, self.runs, self.literals = speclint.source_index(root, speclint.DEFAULT_RUN)
        self.kit = speclint.kit_text(self.kit_dir)
        self.kit_ids = speclint.identifiers(self.kit)

    def tearDown(self):
        self.tmp.cleanup()

    def lint(self, text):
        return [m for _, m in speclint.lint(text, self.ids, self.kit_ids, self.runs, speclint.DEFAULT_RUN,
                                      self.literals, self.kit)]

    def test_clean_spec_passes(self):
        self.assertEqual(self.lint(CLEAN), [])

    def test_template_passes(self):
        template = Path(__file__).resolve().parent.parent / "tests" / "speclint" / "TEMPLATE.md"
        self.assertEqual(self.lint(template.read_text()), [])

    def test_address(self):
        self.assertTrue(any("0x437de0" in m for m in self.lint("The slot at 0x437de0 runs first.")))

    def test_fun_and_dat_names(self):
        problems = self.lint("It calls FUN_00437de0 and reads DAT_005119b8.")
        self.assertEqual(len(problems), 2)

    def test_marker(self):
        self.assertTrue(any("FUNCTION:" in m for m in self.lint("FUNCTION: none")))

    def test_small_hex_values_pass(self):
        self.assertEqual(self.lint("The cosine index adds 0x4020, and f32:0x40000000 is 2.0."), [])

    def test_source_only_identifier(self):
        problems = self.lint("The local `localTargetZ` holds the target.")
        self.assertEqual(problems, ["identifier 'localTargetZ' is in the source but not in the kit"])

    def test_function_name_not_in_kit(self):
        self.assertTrue(any("UpdateMeteorShower" in m for m in self.lint("`UpdateMeteorShower` does it.")))

    def test_kit_identifier_passes(self):
        self.assertEqual(self.lint("`g_meteorNextStrike` and `RAND_STREAM_MAIN`."), [])

    def test_quoted_identifier_passes(self):
        self.assertEqual(self.lint('The message "Hey, hoser!  The default meteor shower data was bogus!" is shown.'), [])

    def test_quoted_kit_text_passes(self):
        self.assertEqual(self.lint('The stream is named "RAND_STREAM_MAIN" in the kit.'), [])

    def test_quoted_source_statement_is_refused(self):
        copied = '"localTargetZ = (int)((__int64)GameRand(RAND_STREAM_MAIN) * g_game->height / 0x8000);"'
        self.assertTrue(any("matches the source" in m for m in self.lint(f"The target is {copied}")))
        self.assertTrue(any("localTargetZ" in m for m in self.lint('The item "localTargetZ" is set.')))

    def test_fence_without_text(self):
        self.assertTrue(any("only `text`" in m for m in self.lint("```c\nint x;\n```\n")))

    def test_fence_without_info_string(self):
        self.assertTrue(any("no info string" in m for m in self.lint("```\nint x;\n```\n")))

    def test_text_fence_passes(self):
        self.assertEqual(self.lint("```text\ncall 0 InitMeteors\n```\n"), [])

    def test_copied_statement(self):
        copied = "if (g_meteorNextStrike <= (unsigned int)tickCount) {"
        self.assertTrue(any("matches the source" in m for m in self.lint(copied)))

    def test_copied_string_literal_passes(self):
        self.assertEqual(self.lint("`Hey, hoser!  The default meteor shower data was bogus!`"), [])

    def test_run_of_few_words_passes(self):
        self.assertEqual(self.lint("The kinds are `[block, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]`."), [])

    def test_one_problem_per_copied_run(self):
        copied = "localTargetZ = (int)((__int64)GameRand(RAND_STREAM_MAIN) * g_game->height / 0x8000);"
        runs = [m for m in self.lint(copied) if "matches the source" in m]
        self.assertEqual(len(runs), 1)

    def test_main_refuses_a_directory_that_is_not_a_kit(self):
        spec = self.root / "spec.md"
        spec.write_text(CLEAN)
        self.assertEqual(speclint.main([str(spec), "--kit", str(self.root), "--root", str(self.root)]), 2)

    def test_main_exit_codes(self):
        spec = self.root / "spec.md"
        spec.write_text(CLEAN)
        self.assertEqual(speclint.main([str(spec), "--kit", str(self.kit_dir), "--root", str(self.root)]), 0)
        spec.write_text("It reads DAT_005119b8.")
        self.assertEqual(speclint.main([str(spec), "--kit", str(self.kit_dir), "--root", str(self.root)]), 1)


if __name__ == "__main__":
    unittest.main()
