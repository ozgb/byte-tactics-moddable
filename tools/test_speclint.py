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

## 2. Interface

### Events

None.

## 3. State

## 4. Behaviour

When `g_meteorNextStrike` is due, one draw from `GameRand(main)` gives *d*, and the target z becomes
d times `Game.height` divided by 32768. The message is `Hey, hoser!  The default meteor shower data was bogus!`.

```text
call 1 UpdateMeteors
```
"""


SIM_EVENTS_CSV = "kind,value,arguments\nSIM_EVENT_SOUND,2,3\nSIM_EVENT_SMOKE,27,4\n"

SIM_EVENTS_PAGE = """\
# Sim events

## Terms

| Term | Meaning | In the kit |
|---|---|---|
| call flag 8 | Bit 3 of the call flags. | this page |
| unit flag 0x4000 | The unit is dying. | layout.json |
"""


def spec(events: str, extra: str = "") -> str:
    return f"# s\n\n## 2. Interface\n\n### Events\n\n{events}\n\n### Notes\n\n{extra}\n## 3. State\n"


class Lint(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        (root / "src" / "map").mkdir(parents=True)
        (root / "src" / "map" / "meteors.cpp").write_text(SOURCE)
        self.kit_dir = root / "kit"
        (self.kit_dir / "docs").mkdir(parents=True)
        (self.kit_dir / "docs" / "kit.md").write_text(KIT_DOC)
        (self.kit_dir / "interfaces").mkdir()
        (self.kit_dir / "interfaces" / "sim_events.csv").write_text(SIM_EVENTS_CSV)
        (self.kit_dir / "docs" / "sim-events.md").write_text(SIM_EVENTS_PAGE)
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

    def events(self, text, template=False):
        terms = speclint.page_terms(self.kit_dir)
        return [m for _, m in speclint.event_problems(text, speclint.event_kinds(self.kit_dir), terms, template)]

    def test_template_passes(self):
        template = Path(__file__).resolve().parent.parent / "tests" / "speclint" / "TEMPLATE.md"
        self.assertEqual(self.lint(template.read_text()), [])
        self.assertEqual(self.events(template.read_text(), True), [])
        spec_path = self.root / "TEMPLATE.md"
        spec_path.write_text(template.read_text())
        self.assertEqual(speclint.main([str(spec_path), "--kit", str(self.kit_dir), "--root", str(self.root)]), 0)

    def test_missing_events_subsection(self):
        got = self.events("# s\n\n## 2. Interface\n\nslots\n\n## 3. State\n")
        self.assertEqual(len(got), 1)
        self.assertIn("no `### Events` subsection", got[0])

    def test_events_after_section_three_does_not_count(self):
        self.assertEqual(len(self.events("## 2. Interface\n\n## 3. State\n\n### Events\n\nNone.\n")), 1)

    def test_empty_events_subsection(self):
        got = self.events(spec(""))
        self.assertEqual(len(got), 1)
        self.assertIn("names no `SIM_EVENT_` kind", got[0])

    def test_none_passes(self):
        self.assertEqual(self.events(spec("None.")), [])

    def test_known_kind_with_the_page_passes(self):
        text = spec("`SIM_EVENT_SOUND`, recorded when a unit is hit; arguments in `docs/sim-events.md`.")
        self.assertEqual(self.events(text), [])

    def test_kind_without_the_page(self):
        got = self.events(spec("`SIM_EVENT_SOUND`, recorded when a unit is hit."))
        self.assertEqual(len(got), 1)
        self.assertIn("does not cite `docs/sim-events.md`", got[0])

    def test_unknown_kind_anywhere(self):
        text = spec("None.", "The `SIM_EVENT_GHOST` kind is recorded.\n")
        self.assertEqual(self.events(text), ["SIM_EVENT_GHOST is not a kind in interfaces/sim_events.csv"])

    def test_kit_without_the_table_is_refused_once(self):
        got = speclint.event_problems(spec("`SIM_EVENT_A` and `SIM_EVENT_B`, see `docs/sim-events.md`."), None, [])
        self.assertEqual(len(got), 1)
        self.assertIn("no interfaces/sim_events.csv", got[0][1])

    def test_term_without_the_citation(self):
        got = self.events(spec("None.", "The unit is dying when it has flag 0x4000 set, and a Call Flag 8 is passed.\n"))
        self.assertEqual(len(got), 1)
        self.assertIn("call flag 8", got[0])
        got = self.events(spec("None.", "A unit with unit flag 16384 is dying.\n"))
        self.assertIn("unit flag 16384", got[0].replace("'", ""))

    def test_term_with_the_citation(self):
        extra = "A unit with unit flag 0x4000 is dying (`docs/sim-events.md`, Terms).\n"
        self.assertEqual(self.events(spec("None.", extra)), [])

    def test_terms_are_whole_words(self):
        self.assertEqual(self.events(spec("None.", "It passes call flag 80 and call flag 8x.\n")), [])

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


UNITS_H = """\
// units.h: GetGroundHeight moved to map.h.
#define UNITS_VERSION_MAJOR 3
#define UNITS_VERSION_MINOR 2
enum { UNITS_EVENT_KILLED = 1 };
typedef struct UnitEvent { int kind; } UnitEvent;
typedef struct UnitsSlots {
    void (__stdcall *SetStateBits)(void* unit, int bits);
    int (__stdcall *GetUnitCount)(void);
} UnitsSlots;
"""

MAP_H = """\
#define MAP_VERSION_MAJOR 5
#define MAP_VERSION_MINOR 0
typedef struct MapSlots { int (__stdcall *GetGroundHeight)(const void* pos); } MapSlots;
"""


class Interfaces(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.kit_dir = Path(self.tmp.name)
        (self.kit_dir / "interfaces").mkdir()
        (self.kit_dir / "interfaces" / "units.h").write_text(UNITS_H)
        (self.kit_dir / "interfaces" / "map.h").write_text(MAP_H)
        (self.kit_dir / "interfaces" / "seams.h").write_text("typedef struct Seam { int slots; } Seam;\n")
        (self.kit_dir / "interfaces" / "services.h").write_text("int __stdcall RandomInt(int range);\n")
        (self.kit_dir / "wasm").mkdir()
        signatures = {"signatures": {"services": {"GameRand": [["int"], "int"]}}}
        (self.kit_dir / "wasm" / "modules.json").write_text(json.dumps(signatures))

    def tearDown(self):
        self.tmp.cleanup()

    def problems(self, text):
        return speclint.interface_problems(text, speclint.kit_headers(self.kit_dir), speclint.kit_signatures(self.kit_dir))

    def test_a_slot_of_the_header_passes(self):
        self.assertEqual(self.problems("| `units.SetStateBits` | `[block, int] -> void` |\n| `map.GetGroundHeight` |"), [])

    def test_a_stale_import_is_refused(self):
        got = self.problems("# s\n\n| `units.GetGroundHeight` | `[linear] -> int` | the height |\n")
        self.assertEqual(got, [(3, "`units.GetGroundHeight` is not declared by the kit's interfaces/units.h")])

    def test_a_name_only_in_a_comment_is_refused(self):
        self.assertEqual(len(self.problems("`units.GetGroundHeight(pos)` gives the height.")), 1)

    def test_a_declared_type_event_kind_or_constant_passes(self):
        self.assertEqual(self.problems("`units.UnitEvent`, `units.UNITS_EVENT_KILLED` and `units.UNITS_VERSION_MAJOR`."), [])

    def test_a_prefix_passes_when_a_name_has_it(self):
        self.assertEqual(self.problems("The `units.Get*` imports."), [])
        self.assertEqual(len(self.problems("The `units.Bank*` imports.")), 1)

    def test_a_host_name_of_the_manifest_passes(self):
        self.assertEqual(self.problems("`services.GameRand` and `services.RandomInt`."), [])
        self.assertEqual(len(self.problems("`services.CommandArgs_GetIntArg`.")), 1)

    def test_a_module_with_no_header_is_left_to_the_other_rules(self):
        self.assertEqual(self.problems("`ghost.Apply`, `rng.Seed` and `scripts.GetSweetSpot`."), [])

    def test_file_names_and_fields_are_not_imports(self):
        text = "`interfaces/units.h`, `units.h`, `docs/units.Md`, `units.count` and `edges-units.Apply`."
        self.assertEqual(self.problems(text), [])

    def test_the_kit_version_passes(self):
        self.assertEqual(self.problems("### Slots (`interfaces/units.h`, version 3.2)\n(`interfaces/map.h`, version 5.0)"), [])

    def test_a_lower_minor_passes(self):
        self.assertEqual(self.problems("(`interfaces/units.h`, version 3.0)"), [])

    def test_a_stale_version_is_refused(self):
        got = self.problems("# s\n### Slots (`interfaces/units.h`,\nversion 2.2)")
        self.assertEqual(got, [(2, "the kit's interfaces/units.h is version 3.2, cited at version 2.2")])
        self.assertEqual(len(self.problems("(`interfaces/map.h`, version 5.1)")), 1)

    def test_a_header_without_version_or_missing_is_refused(self):
        self.assertIn("no version macros", self.problems("(`interfaces/seams.h`, version 1.0)")[0][1])
        self.assertIn("the kit has no interfaces/ghost.h", self.problems("(`interfaces/ghost.h`, version 1.0)")[0][1])

    def test_a_kit_without_headers_refuses_only_a_cited_version(self):
        self.assertEqual(speclint.interface_problems("`units.GetGroundHeight`", {}, {}), [])
        self.assertEqual(len(speclint.interface_problems("(`interfaces/units.h`, version 1.0)", {}, {})), 1)

    def test_the_repository_headers_moved_the_ground_height_to_map(self):
        headers = speclint.kit_headers(Path(__file__).resolve().parent.parent)
        self.assertEqual(speclint.interface_problems("`map.GetGroundHeight`", headers, {}), [])
        self.assertEqual(len(speclint.interface_problems("`units.GetGroundHeight` and `units.GetSweetSpot`", headers, {})), 2)

    def test_main_refuses_a_stale_import(self):
        (self.kit_dir / "MANIFEST.json").write_text(json.dumps({"files": []}))
        (self.kit_dir / "interfaces" / "sim_events.csv").write_text(SIM_EVENTS_CSV)
        spec_path = self.kit_dir / "spec.md"
        spec_path.write_text(spec("None.", "It imports `map.GetGroundHeight`.\n"))
        argv = [str(spec_path), "--kit", str(self.kit_dir), "--root", str(self.kit_dir)]
        self.assertEqual(speclint.main(argv), 0)
        spec_path.write_text(spec("None.", "It imports `units.GetGroundHeight`.\n"))
        self.assertEqual(speclint.main(argv), 1)


if __name__ == "__main__":
    unittest.main()
