"""Tests for oraclekit.py: the refusals, the manifest's hashes and the tag rule.

    uv run tools/test_oraclekit.py
"""

import hashlib
import json
import os
import posixpath
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import oraclekit
import wasm_fuel

ROOT = oraclekit.ROOT
OUTSIDE_URL = re.compile(r"^https?://github\.com/ozgb/byte-tactics-moddable/(blob|tree)/", re.I)
SCHEME = re.compile(r"^[a-z][a-z0-9+.-]*:", re.I)
FENCE = re.compile(r"^(```|~~~).*?^\1[^\n]*$", re.M | re.S)
CODE_SPAN = re.compile(r"`[^`\n]*`")
INLINE_LINK = re.compile(r"!?\[[^\]\n]*\]\(\s*(<[^>\n]*>|[^)\s]*)[^)\n]*\)")
REFERENCE_LINK = re.compile(r"^ {0,3}\[[^\]\n]+\]:\s*(<[^>\n]*>|\S+)", re.M)

CONTENTS = [("file", "docs/a.md", ""), ("dir", "data", "")]
MODULES_TEXT = json.dumps({
    "tools": {"clang": "1"},
    "modules": [{"name": "m", "exports": ["Go"], "cxx": ["wasm/m/module.cpp", "src/map/m.cpp"], "c": ["lib/bitexact/t.c"],
                 "imports": {"services": {"Rand": [["int"], "int"]}}}],
}) + "\n"
UNNAMED_DUMP = """*** Dumping AST Record Layout
         0 | struct W::(unnamed union)::(unnamed at /a/w.h:7:9)
     0:0-0 |   unsigned int first
     0:5-5 |   unsigned int meteor
     1:0-0 |   unsigned int ninth
           | [sizeof=4, align=1,
           |  nvsize=4, nvalign=1]

*** Dumping AST Record Layout
         0 | union W::(unnamed at /a/w.h:5:5)
         0 |   unsigned int value
         0 |   struct W::(unnamed union)::(unnamed at /a/w.h:7:9) b
     0:0-0 |     unsigned int first
     0:5-5 |     unsigned int meteor
     1:0-0 |     unsigned int ninth
           | [sizeof=4, align=1,
           |  nvsize=4, nvalign=1]

*** Dumping AST Record Layout
         0 | union W::(unnamed at /a/w.h:12:5)
         0 |   float pitch
         0 |   int other
           | [sizeof=4, align=1,
           |  nvsize=4, nvalign=1]

*** Dumping AST Record Layout
         0 | struct W
         0 |   char[4] name
         4 |   union W::(unnamed at /a/w.h:5:5) flags
         4 |     unsigned int value
         4 |     struct W::(unnamed union)::(unnamed at /a/w.h:7:9) b
     4:0-0 |       unsigned int first
     4:5-5 |       unsigned int meteor
     5:0-0 |       unsigned int ninth
         8 |   union W::(unnamed at /a/w.h:12:5) angle
         8 |     float pitch
         8 |     int other
           | [sizeof=12, align=1,
           |  nvsize=12, nvalign=1]
"""


def on_disk(root: Path, entry: str) -> list[tuple[str, str]]:
    """The working tree's files under a contents entry, as `oraclekit.tracked` lists the committed ones."""
    base = root / entry
    found = [base] if base.is_file() else sorted(p for p in base.rglob("*") if p.is_file()) if base.is_dir() else []
    return [(p.relative_to(root).as_posix(), "100644") for p in found]


def links(markdown: str) -> list[str]:
    """The link targets of a Markdown text, leaving out code blocks and code spans."""
    text = CODE_SPAN.sub("", FENCE.sub("", markdown))
    targets = [m.group(1) for m in INLINE_LINK.finditer(text)] + [m.group(1) for m in REFERENCE_LINK.finditer(text)]
    return [target[1:-1] if target.startswith("<") else target for target in targets]


def links_outside_the_kit(documents: dict[str, str], shipped: set[str]) -> list[str]:
    """One line per link of a document (by kit path) to a file the kit does not ship."""
    problems = []
    for path, text in sorted(documents.items()):
        for target in links(text):
            if OUTSIDE_URL.match(target):
                problems.append("%s links to %s, which is in the source repository" % (path, target))
            elif SCHEME.match(target) or target.startswith("#") or target == "":
                continue
            else:
                relative = target.split("#", 1)[0].split("?", 1)[0]
                resolved = posixpath.normpath(posixpath.join(posixpath.dirname(path), relative))
                if relative.startswith("/") or resolved not in shipped:
                    problems.append("%s links to %s, which the kit does not ship" % (path, target))
    return problems


def git(root: Path, *args: str) -> None:
    subprocess.run(["git", "-C", str(root), "-c", "user.name=t", "-c", "user.email=t@t", "-c", "commit.gpgsign=false",
                    "-c", "tag.gpgsign=false", *args], check=True,
                   capture_output=True)


def repo(tmp: str, extra: dict[str, str] | None = None) -> Path:
    root = Path(tmp)
    files = {"docs/a.md": "alpha\n", "data/x.json": "{}\n", "data/y/z.txt": "z\n", oraclekit.README_SOURCE: "readme\n",
             "data/layout_refs.csv": "function\n", oraclekit.MODULES: MODULES_TEXT, **(extra or {})}
    for path, text in files.items():
        (root / path).parent.mkdir(parents=True, exist_ok=True)
        (root / path).write_text(text)
    git(root, "init", "-q")
    git(root, "add", "-A")
    git(root, "commit", "-q", "-m", "x")
    return root


def run_export(root: Path, out: Path, untagged: bool = True) -> dict:
    return oraclekit.export(root, out, untagged, {"records": {}}, CONTENTS)


class OracleKitTest(unittest.TestCase):
    def test_a_src_path_is_refused_by_name(self):
        for path in ("src/game/game.h", "include/x.h", "orig/TotalA.exe", "lib/a", "build/b", "cmake/c"):
            with self.assertRaises(oraclekit.Refusal) as e:
                oraclekit.admit(path, [("dir", path.split("/")[0], "")])
            self.assertIn(path, str(e.exception))
            self.assertIn("is under", str(e.exception))

    def test_a_path_off_the_list_is_refused_by_name(self):
        with self.assertRaises(oraclekit.Refusal) as e:
            oraclekit.admit("docs/b.md", CONTENTS)
        self.assertIn("docs/b.md", str(e.exception))
        oraclekit.admit("docs/a.md", CONTENTS)
        oraclekit.admit("data/y/z.txt", CONTENTS)

    def test_the_real_table_lists_nothing_forbidden(self):
        for kind, entry, _ in oraclekit.CONTENTS:
            self.assertFalse(oraclekit.forbidden(entry), entry)

    def test_a_table_entry_under_src_stops_the_export(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"src/a.cpp": "x\n"})
            with self.assertRaises(oraclekit.Refusal) as e:
                oraclekit.export(root, Path(out) / "k", True, {}, CONTENTS + [("dir", "src", "")])
            self.assertIn("src/a.cpp", str(e.exception))
            self.assertFalse((Path(out) / "k").exists())

    def test_the_manifest_hashes_check(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            kit = Path(out) / "k"
            manifest = run_export(root, kit)
            paths = [f["path"] for f in manifest["files"]]
            self.assertEqual(paths, sorted(["docs/a.md", "data/x.json", "data/y/z.txt", "data/layout_refs.csv", "layout.json", "README.md", "wasm/modules.json"]))
            by_path = {f["path"]: f for f in manifest["files"]}
            self.assertEqual(by_path["docs/a.md"]["sha256"], hashlib.sha256(b"alpha\n").hexdigest())
            self.assertEqual(by_path["docs/a.md"]["size"], 6)
            self.assertEqual(manifest["commit"], subprocess.run(["git", "-C", tmp, "rev-parse", "HEAD"], capture_output=True,
                                                               text=True).stdout.strip())
            self.assertEqual(json.loads((kit / oraclekit.MANIFEST).read_text()), manifest)
            self.assertEqual(oraclekit.verify(kit), [])
            (kit / "docs" / "a.md").write_text("beta!\n")
            (kit / "extra.txt").write_text("e")
            self.assertEqual(oraclekit.verify(kit), ["differs: docs/a.md", "unlisted: extra.txt"])

    def test_untagged_refuses_without_the_flag_and_writes_null_with_it(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            with self.assertRaises(oraclekit.Refusal):
                run_export(root, Path(out) / "a", untagged=False)
            self.assertFalse((Path(out) / "a").exists())
            self.assertIsNone(run_export(root, Path(out) / "b")["version"])

    def test_both_tags_give_the_version(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            git(root, "tag", "interfaces-v1")
            with self.assertRaises(oraclekit.Refusal) as e:
                run_export(root, Path(out) / "a", untagged=False)
            self.assertIn("reference-traces", str(e.exception))
            git(root, "tag", "reference-traces-v2")
            git(root, "tag", "reference-traces-v10")
            manifest = run_export(root, Path(out) / "b", untagged=False)
            self.assertEqual(manifest["version"], {"interfaces": "interfaces-v1", "reference_traces": "reference-traces-v10"})

    def test_tags_on_another_commit_do_not_count(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            git(root, "tag", "interfaces-v1")
            git(root, "tag", "reference-traces-v1")
            (root / "docs" / "a.md").write_text("changed\n")
            git(root, "commit", "-q", "-am", "y")
            with self.assertRaises(oraclekit.Refusal):
                run_export(root, Path(out) / "a", untagged=False)

    def test_a_non_empty_target_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            (Path(out) / "keep").write_text("k")
            with self.assertRaises(oraclekit.Refusal):
                run_export(root, Path(out))

    def test_a_tracked_symlink_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"src/game.cpp": "secret\n"})
            (root / "data" / "link.txt").symlink_to("../src/game.cpp")
            git(root, "add", "data/link.txt")
            git(root, "commit", "-q", "-m", "link")
            with self.assertRaises(oraclekit.Refusal) as e:
                run_export(root, Path(out) / "k")
            self.assertIn("data/link.txt", str(e.exception))
            self.assertIn("120000", str(e.exception))
            self.assertFalse((Path(out) / "k").exists())

    def test_an_uncommitted_edit_is_not_exported(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            (root / "docs" / "a.md").write_text("edited\n")
            (root / "data" / "x.json").write_text("[]\n")
            manifest = run_export(root, Path(out) / "k")
            self.assertEqual((Path(out) / "k" / "docs" / "a.md").read_bytes(), b"alpha\n")
            self.assertEqual((Path(out) / "k" / "data" / "x.json").read_bytes(), b"{}\n")
            by_path = {f["path"]: f for f in manifest["files"]}
            self.assertEqual(by_path["docs/a.md"]["sha256"], hashlib.sha256(b"alpha\n").hexdigest())
            self.assertEqual(oraclekit.verify(Path(out) / "k"), [])

    def test_a_target_that_is_a_file_is_refused(self):
        with tempfile.TemporaryDirectory() as out:
            target = Path(out) / "f"
            target.write_text("x")
            with self.assertRaises(oraclekit.Refusal):
                oraclekit.check_target(target)

    def test_the_clang_type_paths_are_cleaned(self):
        self.assertEqual(oraclekit.clean_type("union W::(anonymous at /a/b/c.h:24:5) "), "union W::(anonymous) ")
        self.assertEqual(oraclekit.clean_type("struct W::(unnamed union)::(unnamed at /a/c.h:61:9)"),
                         "struct W::(unnamed union)::(unnamed)")


class ModuleManifestTest(unittest.TestCase):
    def test_the_kit_manifest_drops_the_source_fields_and_keeps_the_rest(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            run_export(root, Path(out) / "k")
            kit = json.loads((Path(out) / "k" / "wasm" / "modules.json").read_text())
            entry = kit["modules"][0]
            self.assertNotIn("cxx", entry)
            self.assertNotIn("c", entry)
            want = json.loads(MODULES_TEXT)
            for field in oraclekit.MODULE_SOURCE_FIELDS:
                del want["modules"][0][field]
            self.assertEqual(kit, want)
            self.assertEqual(oraclekit.verify(Path(out) / "k"), [])

    def test_a_manifest_that_still_names_a_source_is_refused(self):
        text = json.dumps({"modules": [{"name": "m", "note": "see src/map/m.cpp"}]}) + "\n"
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {oraclekit.MODULES: text})
            with self.assertRaises(oraclekit.Refusal) as e:
                run_export(root, Path(out) / "k")
            self.assertIn("wasm/modules.json", str(e.exception))
            self.assertIn("src/", str(e.exception))
            self.assertFalse((Path(out) / "k").exists())

    def test_a_repository_without_the_manifest_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            git(root, "rm", "-q", oraclekit.MODULES)
            git(root, "commit", "-q", "-m", "drop")
            with self.assertRaises(oraclekit.Refusal) as e:
                run_export(root, Path(out) / "k")
            self.assertIn("wasm/modules.json", str(e.exception))

    def test_the_real_manifest_exports_without_a_source_name(self):
        kit = oraclekit.kit_modules((ROOT / oraclekit.MODULES).read_bytes())
        self.assertEqual(oraclekit.names_source(kit), [])
        document = json.loads(kit)
        for entry in document["modules"]:
            self.assertFalse(set(oraclekit.MODULE_SOURCE_FIELDS) & set(entry), entry["name"])
        meteors = next(e for e in document["modules"] if e["name"] == "wind_meteors")
        self.assertEqual(meteors["imports"]["services"]["CommandArgs_Count"], [["scratch"], "int"])
        self.assertEqual(meteors["exports"][:2], ["UpdateWind", "UpdateMeteors"])
        self.assertEqual(document["majors"]["services"], 1)

    def test_the_name_check_finds_paths_and_files(self):
        for text in ("src/a", "x lib/b", "a.cpp", "m.h ", "orig/TotalA.exe", "build/tools"):
            self.assertTrue(oraclekit.names_source(text.encode()), text)
        for text in ("wasm-ld", "once per call", "the .wasm file", "sub/src", "0xF0000", "wasm/modules.json"):
            self.assertEqual(oraclekit.names_source(text.encode()), [], text)


FUEL_MODULE = """(module
  (import "env" "g" (func $g (param i32) (result i32)))
  (memory 1)
  (func $inner (param $n i32) (result i32)
    (local $i i32)
    loop $l
      local.get $i
      i32.const 1
      i32.add
      local.tee $i
      local.get $n
      i32.lt_s
      br_if $l
    end
    local.get $i)
  (func (export "run") (param i32) (result i32)
    local.get 0
    call $inner
    call $g))
"""


def find_wasm_tools() -> str | None:
    repo_copy = ROOT / "build" / "tools" / "bin" / "wasm-tools"
    return os.environ.get("WASM_TOOLS") or (str(repo_copy) if repo_copy.exists() else shutil.which("wasm-tools"))


class FuelPassTest(unittest.TestCase):
    def test_the_fuel_pass_is_on_the_contents_list_and_names_no_source(self):
        self.assertTrue(oraclekit.listed(oraclekit.FUEL_PASS))
        self.assertEqual(oraclekit.names_source((ROOT / oraclekit.FUEL_PASS).read_bytes()), [])

    def test_a_pass_that_names_a_source_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {oraclekit.FUEL_PASS: 'ROOT = "build/tools"\n'})
            with self.assertRaises(oraclekit.Refusal) as e:
                oraclekit.export(root, Path(out) / "k", True, {}, CONTENTS + [("file", oraclekit.FUEL_PASS, "")])
            self.assertIn(oraclekit.FUEL_PASS, str(e.exception))
            self.assertFalse((Path(out) / "k").exists())

    def test_the_exported_pass_runs_alone_and_meters_like_the_repository_pass(self):
        tools = find_wasm_tools()
        if tools is None:
            self.skipTest("wasm-tools is not installed")
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {oraclekit.FUEL_PASS: (ROOT / oraclekit.FUEL_PASS).read_text()})
            run_fuel = Path(out) / "k"
            oraclekit.export(root, run_fuel, True, {}, CONTENTS + [("file", oraclekit.FUEL_PASS, "")])
            alone = run_fuel / oraclekit.FUEL_PASS
            module = Path(out) / "m.wasm"
            module.write_bytes(subprocess.run([tools, "parse", "-"], input=FUEL_MODULE.encode(), capture_output=True,
                                              check=True).stdout)
            metered = Path(out) / "metered.wasm"
            env = {k: v for k, v in os.environ.items() if k != "PYTHONPATH"}
            result = subprocess.run([sys.executable, "-I", str(alone), str(module), "-o", str(metered), "--wasm-tools", tools],
                                    cwd=out, env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(metered.read_bytes(), wasm_fuel.instrument(module.read_bytes(), tools))
            self.assertNotEqual(metered.read_bytes(), module.read_bytes())
            checked = subprocess.run([sys.executable, "-I", str(alone), str(metered), "--check", "--wasm-tools", tools],
                                     cwd=out, env=env, capture_output=True, text=True)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)


class UnnamedBitsTest(unittest.TestCase):
    def test_the_bits_of_an_unnamed_union_are_listed_under_the_field(self):
        entry = oraclekit.record_entry(UNNAMED_DUMP, "W", 12)
        fields = {f["name"]: f for f in entry["fields"]}
        self.assertEqual(fields["flags"]["type"], "W::(unnamed)")
        self.assertEqual(fields["flags"]["bits"], [{"name": "first", "bit": 0, "width": 1}, {"name": "meteor", "bit": 5, "width": 1},
                                                   {"name": "ninth", "bit": 8, "width": 1}])
        self.assertNotIn("bits", fields["angle"])
        self.assertNotIn("bits", fields["name"])

    def test_the_real_layout_names_the_meteor_bit(self):
        for build in ("build/datack", "build/modern"):
            if (ROOT / build / "compile_commands.json").exists():
                break
        else:
            self.skipTest("no build directory with compile_commands.json")
        layout = oraclekit.generate_layout(ROOT, ROOT / build)
        flags = next(f for f in layout["records"]["WeaponDef"]["fields"] if f["name"] == "flags")
        self.assertIn({"name": "meteor", "bit": 5, "width": 1}, flags["bits"])
        self.assertEqual(len(flags["bits"]), 32)
        unnamed = [f for e in layout["records"].values() for f in e["fields"] if "(unnamed)" in f["type"] or "(anonymous)" in f["type"]]
        self.assertGreater(len(unnamed), 0)
        self.assertTrue(all(b["width"] >= 1 for f in unnamed for b in f.get("bits", [])))


class KitDocumentsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.shipped = set(oraclekit.select(ROOT, lister=on_disk)) | {oraclekit.README, oraclekit.LAYOUT, oraclekit.MANIFEST}
        cls.documents = {}
        for path in sorted(cls.shipped):
            if path.endswith(".md"):
                source = oraclekit.README_SOURCE if path == oraclekit.README else path
                cls.documents[path] = (ROOT / source).read_text(encoding="utf-8")

    def test_the_kit_ships_markdown_documents(self):
        self.assertIn("docs/scenario-format.md", self.documents)
        self.assertIn(oraclekit.README, self.documents)

    def test_no_document_of_the_kit_links_outside_it(self):
        self.assertEqual(links_outside_the_kit(self.documents, self.shipped), [])

    def test_the_link_check_finds_links_outside_the_kit(self):
        shipped = {"docs/a.md", "docs/b.md", "data/x.json"}
        documents = {"docs/a.md": "\n".join([
            "[ok](b.md) [anchor](b.md#top) [here](#top) [data](../data/x.json) ![img](../data/x.json)",
            "[web](https://example.com/x.md) [mail](mailto:a@b.c)",
            "[ref]: b.md",
            "`[code](nothing.md)`",
            "```",
            "[fenced](nothing.md)",
            "```",
            "[no](c.md) [up](../src/a.cpp#L1) [dir](../data/) [title](c.md \"t\") [abs](/docs/b.md)",
            "[blob](https://github.com/ozgb/byte-tactics-moddable/blob/moddable/docs/b.md)",
            "[tree](https://github.com/ozgb/byte-tactics-moddable/tree/moddable/data)",
            "[late]: <missing.md>",
        ])}
        found = links_outside_the_kit(documents, shipped)
        self.assertEqual(len(found), 8, found)
        for target in ("c.md", "../src/a.cpp#L1", "../data/", "/docs/b.md", "blob/moddable", "tree/moddable", "missing.md"):
            self.assertTrue(any(target in line for line in found), target)

    def test_every_key_of_the_scenarios_is_in_the_scenario_spec(self):
        spec = (ROOT / "docs" / "scenario-format.md").read_text(encoding="utf-8")
        keys: dict[str, str] = {}
        for scenario in sorted((ROOT / "reference" / "scenarios").glob("*.txt")):
            for line in scenario.read_text(encoding="utf-8").splitlines():
                words = line.split()
                if words and not words[0].startswith("#"):
                    keys.setdefault(re.sub(r"^player\d+\.", "player<N>.", words[0]), scenario.name)
        self.assertGreater(len(keys), 20)
        missing = ["%s (used by %s)" % (key, scenario) for key, scenario in sorted(keys.items()) if "`%s`" % key not in spec]
        self.assertEqual(missing, [])


LEAK_SOURCE = """\
// FUNCTION: 0x437de0
void UpdateMeteorShower(int tickCount)
{
    if (g_meteorNextStrike <= (unsigned int)tickCount) {
        localTargetZ = (int)((__int64)GameRand(RAND_STREAM_MAIN) * g_game->height / 0x8000);
        StartScript(script, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
    }
}
"""
LEAK_LAYOUT = json.dumps({"records": {"Game": {"fields": [{"name": "height"}, {"name": "meteorTimer"}]}}})


def leak_files(doc: str) -> dict[str, bytes]:
    return {"layout.json": LEAK_LAYOUT.encode(), "docs/a.md": doc.encode(), "reference/frames/f.bin": b"\xff\xfe0x437de0"}


class LeakCheckTest(unittest.TestCase):
    def check(self, doc: str) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "meteors.cpp").write_text(LEAK_SOURCE)
            return oraclekit.leak_check(leak_files(doc), Path(tmp))

    def test_a_clean_document_passes(self):
        self.assertEqual(self.check("# a\n\nThe `meteorTimer` field of `Game` counts down; `height` is the map height.\n\n```sh\nuv run tools/x.py\n```\n"), [])

    def test_an_address_is_a_hit_and_binary_files_are_skipped(self):
        hits = self.check("The strike is at 0x437de0.\n")
        self.assertEqual(len(hits), 1)
        self.assertTrue(hits[0].startswith("docs/a.md:1:") and "0x437de0" in hits[0], hits)

    def test_a_source_identifier_is_a_hit_and_a_layout_name_is_not(self):
        hits = self.check("Use `UpdateMeteorShower` and `meteorTimer`.\n")
        self.assertEqual(len(hits), 1)
        self.assertIn("UpdateMeteorShower", hits[0])

    def test_a_name_in_the_checked_document_does_not_allow_itself(self):
        files = leak_files("The `g_meteorNextStrike` global.\n")
        self.assertNotIn("g_meteorNextStrike", oraclekit.allowed_identifiers(files))

    def test_a_long_token_run_of_the_source_is_a_hit(self):
        hits = self.check("Call StartScript(script, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0) now.\n")
        self.assertTrue(any("tokens matches the source" in h for h in hits), hits)

    def test_fenced_blocks_of_any_kind_are_allowed(self):
        self.assertEqual(self.check("```json\n{\"a\": 1}\n```\n"), [])

    def test_interface_headers_are_checked_for_addresses_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "meteors.cpp").write_text(LEAK_SOURCE)
            header = "void UpdateMeteorShower(int tickCount);\nCall StartScript(script, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);\n"
            files = {**leak_files("ok\n"), "interfaces/m.h": header.encode()}
            self.assertEqual(oraclekit.leak_check(files, Path(tmp)), [])
            files["interfaces/m.h"] = (header + "// FUNCTION: 0x437de0\n").encode()
            self.assertEqual(len(oraclekit.leak_check(files, Path(tmp))), 2)

    def test_stored_reference_data_defines_names(self):
        files = {**leak_files("ok\n"), "reference/sounds/a.txt": b"count1 6\n"}
        self.assertIn("count1", oraclekit.allowed_identifiers(files))

    def test_a_leaking_document_stops_the_export_before_it_writes(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"docs/a.md": "see 0x437de0\n"})
            kit = Path(out) / "k"
            with self.assertRaises(oraclekit.Refusal) as caught:
                run_export(root, kit)
            self.assertIn("docs/a.md:1:", str(caught.exception))
            self.assertFalse(kit.exists())


if __name__ == "__main__":
    unittest.main()
