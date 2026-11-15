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

CONTENTS = [("file", "docs/a.md", ""), ("file", "data/x.json", ""), ("dir", "data/y", ""), ("file", "data/layout_refs.csv", "")]
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


COUPLING_DATA = {
    "data/dependencies.csv": "writer,reader,item,phase,class,runs,example,flags\n"
                             "ai,units,Unit::flags,players,immediate,3,a@1,\n"
                             "ai,units,g_hiddenTable,players,immediate,2,a@2,\n"
                             "ai,units,g_otherTable,players,immediate,7,a@9,\n"
                             "ai,orders,DeleteOrders,-,unobserved,0,,indirect\n"
                             "ai,units,MysteryCall,-,unobserved,0,,indirect\n"
                             "ai,orders,Unit::orders,-,unobserved,0,,\n"
                             "-,units,rng.parkMiller,players,ordered,1,a@3,\n",
    "data/phase_dependencies.csv": "writer,reader,item,phase_from,phase_to,class,runs,example,flags\n"
                                   "ai,map,Unit::flags,players,units,carried,3,a@1,\n",
    "data/phase_boundaries.csv": "boundary,needed,edges,direct,carried,mergeable,example\n1|2,yes,3,1,2,no,a@1\n",
    "data/draws.csv": "pass,stream,subsystem,function,entities,draws_min,draws_max,ticks,start_state,evidence,example\n"
                      "units,rng.parkMiller,ai,ReactToAttack,1,1,1,3,fixed,,a@1\n"
                      "units,rng.parkMiller,ai,ChooseBuildOption,2,1,9,5,depends-on-earlier,"
                      "Unit::flags;SquadManager: new BuildTimer;g_hiddenTable,a@2\n",
    "data/pass_reads.csv": "pass,item,reader,function,evidence,runs,example\n"
                           "units,Unit::flags,ai,ReactToAttack,same-record,2,a@1\n"
                           "units,Unit::flags,ai,ChooseBuildOption,same-record,5,a@2\n"
                           "units,g_hiddenTable,ai,ReactToAttack,other-record,1,a@3\n",
    "data/presentation_reads.csv": "item,reader,pattern,covered_by_event,visibility,runs\n"
                                   "Unit::flags,presentation,unknown,,all-players,0\n"
                                   "Build: operator new int,map,unknown,,own,1\n"
                                   "Make: new Order,map,unknown,,own,2\n",
    "data/presentation_feeds.csv": "writer,reader,item,phase,example,reason\n"
                                   "presentation,units,Unit::flags,units,a@1,Selection\n",
    "data/dependency_checks.csv": "writer,reader,item,phase,group,scenarios,result,scenarios_counted,defer_held,defer_third_stores\n"
                                  "ai,units,Unit::flags,players,g1,3,equal,3,0,0\n",
}
COUPLING_EXTRA = {"interfaces/orders.h": "struct OrdersSeam {\n    void (__stdcall* DeleteOrders)(int all);\n};\n",
                  "data/state_classes.csv": "name,address,kind,proposed,class\nMake: new Order,0x0,heap,,sim\n"
                                           "g_hiddenTable,0x0,global,,const\ng_otherTable,0x0,global,,const\n"}
COUPLING_LAYOUT = {"records": {"Unit": {"fields": [{"name": "flags"}, {"name": "orders"}]}}}
COUPLING_SOURCE = "int MysteryCall(int a) { return a; }\nint ReactToAttack(int a) { return a; }\nint ChooseBuildOption(int a) { return a; }\nint g_hiddenTable[4];\n"


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
             "data/layout_refs.csv": "function\n", oraclekit.MODULES: MODULES_TEXT, **COUPLING_DATA, **COUPLING_EXTRA, **(extra or {})}
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
            self.assertEqual(paths, sorted(["docs/a.md", "data/x.json", "data/y/z.txt", "data/layout_refs.csv", "layout.json", "README.md", "wasm/modules.json"] +
                                                  ["coupling/%s.csv" % n for n in oraclekit.COUPLING]))
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

    def test_a_traces_tag_on_an_earlier_commit_is_taken(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"reference/t.csv": "1\n"})
            git(root, "tag", "reference-traces-v9")
            git(root, "tag", "reference-traces-v10")
            first = oraclekit.git(root, "rev-parse", "HEAD").strip()
            (root / "docs" / "a.md").write_text("changed\n")
            git(root, "commit", "-q", "-am", "y")
            git(root, "tag", "interfaces-v1")
            manifest = run_export(root, Path(out) / "k", untagged=False)
            self.assertEqual(manifest["version"], {"interfaces": "interfaces-v1", "reference_traces": "reference-traces-v10"})
            self.assertEqual(manifest["tags"], manifest["version"])
            self.assertEqual(manifest["tag_commits"], {"interfaces": oraclekit.git(root, "rev-parse", "HEAD").strip(), "reference_traces": first})

    def test_a_reference_change_after_the_traces_tag_is_refused_by_name(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"reference/t.csv": "1\n", "reference/README.md": "r\n"})
            git(root, "tag", "reference-traces-v1")
            (root / "reference" / "t.csv").write_text("2\n")
            git(root, "commit", "-q", "-am", "y")
            git(root, "tag", "interfaces-v1")
            with self.assertRaises(oraclekit.Refusal) as e:
                run_export(root, Path(out) / "k", untagged=False)
            self.assertIn("reference/t.csv", str(e.exception))
            self.assertNotIn("README.md", str(e.exception))
            self.assertFalse((Path(out) / "k").exists())
            self.assertIsNone(run_export(root, Path(out) / "u")["version"])

    def test_a_markdown_change_under_reference_is_accepted(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"reference/t.csv": "1\n", "reference/README.md": "r\n"})
            git(root, "tag", "reference-traces-v1")
            (root / "reference" / "README.md").write_text("changed\n")
            git(root, "commit", "-q", "-am", "y")
            git(root, "tag", "interfaces-v1")
            manifest = run_export(root, Path(out) / "k", untagged=False)
            self.assertEqual(manifest["version"]["reference_traces"], "reference-traces-v1")

    def test_a_missing_interfaces_tag_on_head_is_still_refused(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            git(root, "tag", "reference-traces-v1")
            with self.assertRaises(oraclekit.Refusal) as e:
                run_export(root, Path(out) / "k", untagged=False)
            self.assertIn("interfaces", str(e.exception))

    def test_a_non_empty_target_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp)
            (Path(out) / "keep").write_text("k")
            with self.assertRaises(oraclekit.Refusal):
                run_export(root, Path(out))

    def test_a_tracked_symlink_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"src/game.cpp": "secret\n"})
            (root / "data" / "y" / "link.txt").symlink_to("../src/game.cpp")
            git(root, "add", "data/y/link.txt")
            git(root, "commit", "-q", "-m", "link")
            with self.assertRaises(oraclekit.Refusal) as e:
                run_export(root, Path(out) / "k")
            self.assertIn("data/y/link.txt", str(e.exception))
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


SIM_HEADER = """\
enum SimEventKind : unsigned short {
    SIM_EVENT_NONE = 0,
    SIM_EVENT_SOUND = 2,
    SIM_EVENT_NANO = 3,
};
"""
SIM_PAGE = """\
# Sim events

## The kinds

### SIM_EVENT_NONE

Value 0. Marks an empty record.

Phases: none.

### SIM_EVENT_SOUND

Value 2.

| Argument | Type | Meaning |
|---|---|---|
| 0 | sound name number | A number of the sound names table. |
| 1 | integer | The play flag. |

Flags: none.

Recorded when a unit with flag 0x4000 set is called with flag 8 in state 2 of the pickup order, by kill type 3.

Phases: net (0), units (1).

### SIM_EVENT_NANO

Value 3.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The origin. |
| 3 to 8 | box | The target. |
| 9 | integer | The particle list. |

Flags: none. Two records (ten arguments).

Phases: units (1).
"""
SIM_TERMS = (
    "term,definition,kit_reference\n"
    "unit flag 0x4000,The unit is dying.,layout.json Unit.killPending\n"
    "call flag 8,A referenced unit was removed.,\n"
    "state 2 of the pickup order,The transport starts a pickup attempt.,layout.json Order.state\n"
    "kill type 3,A self-destruct.,\n"
)
SIM_FILES = {"interfaces/services.h": SIM_HEADER, oraclekit.SIM_EVENTS_PAGE: SIM_PAGE, oraclekit.TERMS_SOURCE: SIM_TERMS}
SIM_CONTENTS = [("dir", "interfaces", ""), ("file", oraclekit.SIM_EVENTS_PAGE, "")]
REAL_LAYOUT: dict = {}


def real_layout() -> dict:
    """The layout.json document of a build directory that has a compile_commands.json, the same one for every test."""
    if not REAL_LAYOUT:
        for build in ("build/kitb", "build/datack", "build/modern"):
            if (ROOT / build / "compile_commands.json").exists():
                REAL_LAYOUT.update(oraclekit.generate_layout(ROOT, ROOT / build))
                break
        else:
            raise unittest.SkipTest("no build directory with compile_commands.json")
    return REAL_LAYOUT


def csv_dicts(data: bytes) -> list[dict[str, str]]:
    return oraclekit.csv_rows(data)[1]


class SimEventKitTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.page = (ROOT / oraclekit.SIM_EVENTS_PAGE).read_text(encoding="utf-8")
        cls.header = (ROOT / oraclekit.SIM_EVENTS_HEADER).read_text(encoding="utf-8")
        cls.terms = oraclekit.term_rows((ROOT / oraclekit.TERMS_SOURCE).read_bytes())

    def test_the_queue_and_its_record_are_in_the_layout(self):
        records = real_layout()["records"]
        self.assertEqual(records["SimEvent"]["size"], 32)
        self.assertEqual([f["name"] for f in records["SimEvent"]["fields"]], ["kind", "flags", "tick", "args"])
        queue = records["SimEventQueue"]
        self.assertEqual(queue["size"], 16 + 4096 * 32)
        self.assertEqual([(f["name"], f["offset"], f["size"]) for f in queue["fields"]],
                         [("head", 0, 4), ("tail", 4, 4), ("dropped", 8, 4), ("reserved", 12, 4), ("records", 16, 4096 * 32)])

    def test_the_layout_of_the_queue_names_no_source_file(self):
        text = json.dumps({k: v for k, v in real_layout()["records"].items() if k.startswith("SimEvent")})
        self.assertEqual(oraclekit.names_source(text.encode()), [])

    def test_the_kinds_table_has_a_row_for_each_kind_of_the_enum(self):
        rows = csv_dicts(oraclekit.kinds_table(self.header, self.page))
        enum = oraclekit.enum_kinds(self.header)
        self.assertEqual({r["kind"]: int(r["value"]) for r in rows}, enum)
        self.assertEqual([int(r["value"]) for r in rows], sorted(enum.values()))
        self.assertNotIn(24, enum.values())
        by_kind = {r["kind"]: r for r in rows}
        speech = by_kind["SIM_EVENT_UNIT_SPEECH"]
        self.assertEqual((speech["arguments"], speech["records"]), ("3", "1"))
        self.assertIn("speaks only while it is off screen", speech["flags"])
        self.assertIn("0 unit index:", speech["argument_list"])
        nano = by_kind["SIM_EVENT_NANO_PARTICLES"]
        self.assertEqual((nano["arguments"], nano["records"], nano["flags"]), ("10", "2", "none"))
        self.assertIn("0-2 position:", nano["argument_list"])
        self.assertEqual(by_kind["SIM_EVENT_THRUST_PARTICLES"]["arguments"], "9")
        self.assertEqual((by_kind["SIM_EVENT_NONE"]["arguments"], by_kind["SIM_EVENT_NONE"]["records"]), ("0", "0"))

    def test_the_kinds_table_is_the_same_each_time(self):
        self.assertEqual(oraclekit.kinds_table(self.header, self.page), oraclekit.kinds_table(self.header, self.page))

    def test_a_kind_the_enum_and_the_page_disagree_on_stops_the_table(self):
        with self.assertRaises(oraclekit.Refusal) as caught:
            oraclekit.kinds_table(self.header, self.page.replace("### SIM_EVENT_SOUND\n\nValue 2.", "### SIM_EVENT_SOUND\n\nValue 24.", 1))
        self.assertIn("SIM_EVENT_SOUND is 24 on the page and 2 in the enum", str(caught.exception))
        with self.assertRaises(oraclekit.Refusal) as caught:
            oraclekit.kinds_table(self.header + "\n", self.page.replace("### SIM_EVENT_BLACK_SMOKE", "### SIM_EVENT_BLACKER_SMOKE"))
        self.assertIn("SIM_EVENT_BLACK_SMOKE is in the enum and not on the page", str(caught.exception))
        self.assertIn("SIM_EVENT_BLACKER_SMOKE is on the page and not in the enum", str(caught.exception))

    def test_the_page_scan_finds_the_terms_of_the_page(self):
        keys, hexes = oraclekit.page_terms(self.page)
        for key in ("call flag 2", "call flag 8", "call flag 32", "state 2 of the pickup order", "state 2", "kill type 7",
                    "damage kind 10", "damage kind 11", "damage kind 2", "damage kind 1", "map flag 1", "map flag 2", "match flag 2",
                    "net game flag 1", "spot flag bit 0", "activation bit 0", "activation bit 2",
                    "game type 1", "game type 2", "game type 3", "player type 1", "player type 2", "player type 3"):
            self.assertIn(key, keys)
        for number in (0x2000, 0x10000000, 0x4000, 0x20000000, 0x20, 0x300, 0x60000000):
            self.assertIn(number, hexes)

    def test_every_number_of_the_page_has_a_definition(self):
        oraclekit.check_terms(self.page, self.terms)
        keys, hexes = oraclekit.page_terms(self.page)
        defined = {oraclekit.norm_term(r["term"]) for r in self.terms}
        self.assertEqual(sorted(keys - defined), [])
        self.assertTrue(all(r["definition"].strip() for r in self.terms))

    def test_a_term_of_the_page_without_a_definition_stops_the_export(self):
        for added, term in (("A unit in state 7 of the pickup order.", "state 7 of the pickup order"),
                            ("The unit has flag 0x1234.", "0x1234"),
                            ("The unit dies with kill type 9.", "kill type 9"),
                            ("Hit by damage kind 12.", "damage kind 12"),
                            ("The game type must be 4.", "game type 4"),
                            ("A player of type 4 stays.", "player type 4"),
                            ("The map flag 4 is set.", "map flag 4"),
                            ("A step called with flag 16.", "call flag 16")):
            with self.assertRaises(oraclekit.Refusal) as caught:
                oraclekit.kit_page(self.page + "\n" + added + "\n", self.terms)
            self.assertIn(term, str(caught.exception))
            self.assertIn("has no row", str(caught.exception))

    def test_the_player_and_game_types_are_defined_by_what_they_are(self):
        by_term = {r["term"]: r for r in self.terms}
        for n in (1, 2, 3):
            self.assertIn("docs/scenario-format.md", by_term["game type %d" % n]["kit_reference"])
            self.assertIn("layout.json Player.type", by_term["player type %d" % n]["kit_reference"])
        self.assertIn("isfeature", by_term["kill type 7"]["definition"])
        self.assertIn("nothing else sets it", by_term["match flag 2"]["definition"])

    def test_a_definition_the_page_does_not_use_stops_the_export(self):
        rows = self.terms + [{"term": "kill type 12", "definition": "A kill.", "kit_reference": ""}]
        with self.assertRaises(oraclekit.Refusal) as caught:
            oraclekit.kit_page(self.page, rows)
        self.assertIn("kill type 12 is defined", str(caught.exception))

    def test_a_row_without_a_definition_is_refused(self):
        with self.assertRaises(oraclekit.Refusal):
            oraclekit.term_rows(b"term,definition,kit_reference\nkill type 3,,\n")
        with self.assertRaises(oraclekit.Refusal):
            oraclekit.term_rows(b"term,meaning\nkill type 3,x\n")

    def test_the_kit_page_is_the_page_with_the_terms_after_it(self):
        kit = oraclekit.kit_page(self.page, self.terms).decode()
        head, tail = kit.split("\n## Terms\n")
        self.assertEqual(head, oraclekit.one_entry_per_line(self.page).rstrip("\n") + "\n")
        before, after = self.page.split("\n## Tables\n")
        self.assertTrue(kit.startswith(before + "\n## Tables\n"))
        self.assertEqual(after.split("\n## ", 1)[1], head.split("\n## Tables\n")[1].split("\n## ", 1)[1])
        for row in self.terms:
            self.assertIn("| %s |" % row["term"], tail)
        self.assertEqual(kit, oraclekit.kit_page(self.page, self.terms).decode())
        self.assertEqual(oraclekit.page_terms(kit), oraclekit.page_terms(self.page))
        self.assertIn("viewed player", kit)

    def test_a_long_numbered_list_is_set_one_entry_to_a_line_and_loses_nothing(self):
        page = "# T\n\n## Tables\n\n### K\n\nThe kinds: " + ", ".join('%d "name%d"' % (n, n) for n in range(10)) + ".\n\n### S\n\n0 is \"a\", 1 is \"b\".\n\n## End\n"
        text = oraclekit.one_entry_per_line(page)
        self.assertIn("The kinds:\n\n- 0 \"name0\"\n- 1 \"name1\"\n", text)
        self.assertIn('- 9 "name9".\n', text)
        self.assertIn('0 is "a", 1 is "b".', text)
        self.assertEqual(re.findall(r'\d+ "name\d+"', text), re.findall(r'\d+ "name\d+"', page))
        self.assertEqual(oraclekit.one_entry_per_line("no tables here\n"), "no tables here\n")
        real = oraclekit.one_entry_per_line(self.page)
        self.assertEqual(oraclekit.page_terms(real), oraclekit.page_terms(self.page))
        self.assertIn('- 23 "canceldestruct" ("Self destruct terminated").', real)
        self.assertEqual(oraclekit.one_entry_per_line(real), real)

    def test_the_machine_dependent_notes_are_kept(self):
        kit = oraclekit.kit_page(self.page, self.terms).decode()
        for line in self.page.splitlines():
            if "viewed player" in line or "local player" in line:
                self.assertIn(line, kit)

    def test_each_term_points_into_the_kit_or_is_defined_here(self):
        layout_names = set()
        for name, entry in real_layout()["records"].items():
            layout_names.update("%s.%s" % (name, f["name"]) for f in entry["fields"])
        for row in self.terms:
            for ref in re.findall(r"layout\.json ([A-Z]\w*\.\w+)", row["kit_reference"]):
                self.assertIn(ref, layout_names, row["term"])
            for ref in re.findall(r"\b(SIM_EVENT_\w+)\b", row["kit_reference"]):
                self.assertIn(ref, self.header, row["term"])

    def test_the_export_carries_the_page_the_table_and_the_layout(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, SIM_FILES)
            kit = Path(out) / "k"
            layout = {"records": {"SimEventQueue": {"size": 16}}}
            manifest = oraclekit.export(root, kit, True, layout, SIM_CONTENTS)
            paths = [f["path"] for f in manifest["files"]]
            self.assertIn("interfaces/sim_events.csv", paths)
            self.assertIn("docs/sim-events.md", paths)
            self.assertIn("SimEventQueue", (kit / "layout.json").read_text())
            page = (kit / "docs" / "sim-events.md").read_text()
            self.assertIn("\n## Terms\n", page)
            self.assertIn("| kill type 3 | A self-destruct. | this page |", page)
            rows = csv_dicts((kit / "interfaces" / "sim_events.csv").read_bytes())
            self.assertEqual([r["kind"] for r in rows], ["SIM_EVENT_NONE", "SIM_EVENT_SOUND", "SIM_EVENT_NANO"])
            self.assertEqual(oraclekit.verify(kit), [])

    def test_the_export_without_a_definition_writes_nothing(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {**SIM_FILES, oraclekit.TERMS_SOURCE: SIM_TERMS.replace("kill type 3,A self-destruct.,\n", "")})
            kit = Path(out) / "k"
            with self.assertRaises(oraclekit.Refusal) as caught:
                oraclekit.export(root, kit, True, {"records": {}}, SIM_CONTENTS)
            self.assertIn("kill type 3 is used by docs/sim-events.md", str(caught.exception))
            self.assertFalse(kit.exists())

    def test_the_generated_table_is_checked_for_names_like_a_document(self):
        files = {"layout.json": LEAK_LAYOUT.encode(), oraclekit.SIM_EVENTS_TABLE: b"kind\nUpdateMeteorShower\n"}
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "meteors.cpp").write_text(LEAK_SOURCE)
            hits = oraclekit.leak_check(files, Path(tmp))
        self.assertTrue(any("UpdateMeteorShower" in h for h in hits), hits)


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
        hits = self.check("Use `g_meteorNextStrike` and `meteorTimer`.\n")
        self.assertEqual(len(hits), 1)
        self.assertIn("g_meteorNextStrike", hits[0])

    def test_a_source_function_is_a_hit_even_when_kit_data_names_it(self):
        files = {**leak_files("Call `UpdateMeteorShower` each tick.\n"), "data/x.json": b'{"note": "UpdateMeteorShower"}\n'}
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "meteors.cpp").write_text(LEAK_SOURCE)
            hits = oraclekit.leak_check(files, Path(tmp))
        self.assertIn("docs/a.md:1: function 'UpdateMeteorShower' is declared by no interface header", hits)
        self.assertIn("data/x.json:1: function 'UpdateMeteorShower' is declared by no interface header", hits)
        self.assertFalse(any("identifier" in h for h in hits), hits)

    def test_a_function_an_interface_or_a_recording_declares_is_no_hit(self):
        for declaring in ("interfaces/m.h", "tests/replay/m/a.txt"):
            files = {**leak_files("Call `UpdateMeteorShower` each tick.\n"), declaring: b"void UpdateMeteorShower(int tickCount);\n"}
            with tempfile.TemporaryDirectory() as tmp:
                src = Path(tmp) / "src"
                src.mkdir()
                (src / "meteors.cpp").write_text(LEAK_SOURCE)
                self.assertEqual(oraclekit.leak_check(files, Path(tmp)), [], declaring)

    def test_a_source_file_is_a_hit_in_every_text_file(self):
        files = {**leak_files("Set at `weapons.cpp:1529`, see src/map/meteors.h.\n"), "data/x.json": b'{"includes": ["../map/cell.h"]}\n',
                 "tests/replay/m/a.txt": b"# from meteors.c\n"}
        with tempfile.TemporaryDirectory() as tmp:
            hits = oraclekit.leak_check(files, Path(tmp))
        self.assertEqual(hits, ["data/x.json:1: names the source file '../map/cell.h'",
                                "docs/a.md:1: names the source file 'weapons.cpp'",
                                "docs/a.md:1: names the source file 'src/map/meteors.h'",
                                "tests/replay/m/a.txt:1: names the source file 'meteors.c'"])

    def test_the_source_file_rule_spares_interface_headers_system_includes_and_python_attributes(self):
        text = '#include "services.h"\n#include <stdlib.h>\nsee interfaces/services.h and `import.c[0]`\n'
        self.assertEqual(oraclekit.source_file_hits("interfaces/ref.h", text, {"services.h"}), [])
        self.assertEqual(oraclekit.source_file_hits("tools/t.py", "self.h = h.h\n", set()), [])
        self.assertEqual(oraclekit.source_file_hits("tools/t.py", "# see src/a.cpp\n", set()), ["tools/t.py:1: names the source file 'src/a.cpp'"])
        self.assertEqual(oraclekit.source_file_hits("docs/a.md", "see `other.h`\n", {"services.h"}), ["docs/a.md:1: names the source file 'other.h'"])

    def test_a_quoted_string_of_the_source_is_data(self):
        source = 'void RockUnit(int a) { }\nvoid Fire(void) { StartScript("RockUnit"); }\n'
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "w.cpp").write_text(source)
            self.assertEqual(oraclekit.leak_check(leak_files('It starts the script "RockUnit".\n'), Path(tmp)), [])
            self.assertEqual(len(oraclekit.leak_check(leak_files("It calls RockUnit.\n"), Path(tmp))), 2)

    def test_the_files_under_reference_are_asked_about_functions(self):
        files = {**leak_files("ok\n"), "reference/scenarios/s.txt": b"# takes the UpdateMeteorShower path\n"}
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "meteors.cpp").write_text(LEAK_SOURCE)
            self.assertEqual(oraclekit.leak_check(files, Path(tmp)),
                             ["reference/scenarios/s.txt:1: function 'UpdateMeteorShower' is declared by no interface header"])

    def test_constructors_are_not_functions(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "b.h").write_text("struct UnitBucket { UnitBucket() { } };\nint FirstUnit(int a) { return a; }\n")
            self.assertEqual(oraclekit.source_functions(Path(tmp)), {"FirstUnit"})

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

    def test_the_export_strips_address_marker_lines_from_interfaces(self):
        header = "int a;\n    // FUNCTION: 0x4c54a0 ??0X@@QAE@XZ\nint b;\n// GLOBAL: 0x4fc490\n// kept comment\n"
        self.assertEqual(oraclekit.strip_markers("interfaces/x.h", header.encode()), b"int a;\nint b;\n// kept comment\n")
        self.assertEqual(oraclekit.strip_markers("docs/x.md", header.encode()), header.encode())
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"interfaces/x.h": header})
            kit = Path(out) / "k"
            manifest = oraclekit.export(root, kit, True, {"records": {}}, CONTENTS + [("dir", "interfaces", "")])
            stripped = (kit / "interfaces" / "x.h").read_bytes()
            self.assertNotIn(b"0x4c54a0", stripped)
            entry = next(f for f in manifest["files"] if f["path"] == "interfaces/x.h")
            self.assertEqual(entry["sha256"], hashlib.sha256(stripped).hexdigest())

    def test_stored_reference_data_defines_names(self):
        files = {**leak_files("ok\n"), "reference/sounds/a.txt": b"count1 6\n"}
        self.assertIn("count1", oraclekit.allowed_identifiers(files))

    def test_a_document_that_names_a_source_line_stops_the_export_with_its_path_and_line(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"docs/a.md": "alpha\n\nThe bit is tested at foo.cpp:12.\n"})
            kit = Path(out) / "k"
            with self.assertRaises(oraclekit.Refusal) as caught:
                run_export(root, kit)
            self.assertIn("docs/a.md:3: names the source file 'foo.cpp'", str(caught.exception))
            self.assertFalse(kit.exists())

    def test_a_leaking_document_stops_the_export_before_it_writes(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"docs/a.md": "see 0x437de0\n"})
            kit = Path(out) / "k"
            with self.assertRaises(oraclekit.Refusal) as caught:
                run_export(root, kit)
            self.assertIn("docs/a.md:1:", str(caught.exception))
            self.assertFalse(kit.exists())


SCHEMA_TEXT = json.dumps({
    "annotations": {
        "includes": ["../orders/unit_orders.h"],
        "rules": [{"kind": "field_escape", "target": "Game::weapons", "functions": ["AttackChaseOrder"]}],
        "subsystems": {"orders": {"slots": {"RunOrders": {"functions": ["AttackChaseOrder"],
                                                          "through": {"OrderTableEntry.notify": ["AttackChaseOrder"]}}}}},
        "services": [{"name": "GameRand", "header": "src/state/game_rand.h", "declaration": "int GameRand(int s);",
                      "effect": ["bx_rand(s);"]}],
        "code_tables": {"orderNotify": {"entries": ["ReadyOrder", "AttackChaseOrder"], "headers": ["src/orders/unit_orders.h"],
                                        "fill": "orders.RegisterAllOrderTypes"}},
        "callbacks": {"Scan": {"header": "src/game/victory_conditions.h", "handle": "scanner", "methods": []}},
        "exempt": {"g_hiddenTable": "a constant table"},
        "exempt_types": {"HiddenView": "a stack copy"},
        "objects": {"Spot": {"type": "Spot", "vptr": "g_hiddenVtable",
                             "fields": [{"name": "velY", "kind": "i32", "member": "vel.y", "aliases": ["HiddenView::y"]}]}},
    },
    "objects": [{"name": "Spot", "vtable": "g_hiddenVtable", "fields": [{"name": "velY", "kind": "i32", "member": "vel.y",
                                                                       "aliases": ["HiddenView::y"], "reachFirst": 0, "reachCount": 1}]}],
    "subsystems": {"orders": {"slots": {"RunOrders": {"functions": ["AttackChaseOrder"], "reads": ["Game.ticks"], "allOf": [],
                                                      "args": [{"name": "out", "mode": "units", "reachFirst": 0, "reachCount": 1}]}}}},
}, indent=1) + "\n"
CODE_TABLES_TEXT = "table,entry,kit_name\norderNotify,ReadyOrder,Ready\norderNotify,AttackChaseOrder,Attack_Chase\n"
SCHEMA_SOURCE = ("int ReadyOrder(int a) { return a; }\nint AttackChaseOrder(int a) { return a; }\n"
                 "struct HiddenView { int y; };\nint g_hiddenTable[4];\nint g_hiddenVtable[2];\nint velY;\n")
SCHEMA_CONTENTS = CONTENTS + [("file", oraclekit.SCHEMA, "")]


def schema_repo(tmp: str, doc: str = "alpha\n", tables: str = CODE_TABLES_TEXT) -> Path:
    return repo(tmp, {"docs/a.md": doc, oraclekit.SCHEMA: SCHEMA_TEXT, oraclekit.CODE_TABLES_SOURCE: tables, "src/o.cpp": SCHEMA_SOURCE})


class ReplaySchemaKitTest(unittest.TestCase):
    def test_the_kit_schema_drops_the_source_fields_and_renames_the_code_tables(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = schema_repo(tmp)
            oraclekit.export(root, Path(out) / "k", True, {"records": {}}, SCHEMA_CONTENTS)
            text = (Path(out) / "k" / oraclekit.SCHEMA).read_text()
            for name in ("AttackChaseOrder", "ReadyOrder", "src/", ".h", "bx_rand"):
                self.assertNotIn(name, text)
            kit = json.loads(text)
            self.assertEqual(kit["annotations"]["code_tables"]["orderNotify"],
                             {"entries": ["Ready", "Attack_Chase"], "fill": "orders.RegisterAllOrderTypes"})
            self.assertEqual(kit["annotations"]["services"], [{"name": "GameRand", "declaration": "int GameRand(int s);"}])
            self.assertEqual(kit["subsystems"]["orders"]["slots"]["RunOrders"],
                             {"reads": ["Game.ticks"], "args": [{"name": "out", "mode": "units"}]})
            for key in ("rules", "exempt", "exempt_types"):
                self.assertNotIn(key, kit["annotations"])
            velY = {"name": "velY", "kind": "i32", "member": "vel.y"}
            self.assertEqual(kit["annotations"]["objects"], {"Spot": {"type": "Spot", "fields": [velY]}})
            self.assertEqual(kit["objects"], [{"name": "Spot", "fields": [velY]}])
            for name in ("HiddenView", "g_hidden"):
                self.assertNotIn(name, text)

    def test_services_and_callbacks_named_by_an_undeclared_source_name_are_left_out(self):
        schema = json.dumps({"annotations": {
            "services": [{"name": "SimAlloc", "id": "RI_SIM_ALLOC"}, {"name": "SimAllocTagged", "id": "RI_SIM_ALLOC_TAGGED"}],
            "callbacks": {"UnitVisitor": {"handle": "visitor", "methods": [{"name": "Visit"}]},
                          "UnitScanVisitor": {"handle": "scanner", "methods": [{"name": "VisitUnit"}]}}}}).encode()
        kit = json.loads(oraclekit.kit_schema(schema, {}, {"SimAllocTagged", "VisitUnit", "RI_SIM_ALLOC"}))
        self.assertEqual(kit["annotations"]["services"], [{"name": "SimAlloc"}])
        self.assertEqual(list(kit["annotations"]["callbacks"]), ["UnitVisitor"])

    def test_a_schema_entry_naming_an_undeclared_source_global_or_type_is_refused_with_its_path_and_key(self):
        schema = json.dumps({"annotations": {"game_aliases": {"g_hiddenTable": "Game.table"},
                                             "objects": {"Spot": {"type": "HiddenView", "fields": [
                                                 {"name": "velY", "member": "vel.y"}, {"name": "posY", "member": "pos.y"}]}}},
                             "subsystems": {"s": {"slots": {"Run": {"objectReads": ["Spot.velY", "Spot.posY"]}}}}})
        layout = json.dumps({"records": {"Spot": {"fields": [{"name": "vel"}, {"name": "pos"}]}}})
        files = {"layout.json": layout.encode(), oraclekit.SCHEMA: schema.encode()}
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "o.cpp").write_text(SCHEMA_SOURCE + "int posY;\n")
            hits = oraclekit.leak_check(files, Path(tmp))
        self.assertEqual(hits, [
            "data/replay_schema.json: /annotations/game_aliases: the key 'g_hiddenTable' names 'g_hiddenTable', which the kit does not declare",
            "data/replay_schema.json: /annotations/objects/Spot/type: 'HiddenView' names 'HiddenView', which the kit does not declare",
        ])

    def test_the_export_refuses_a_schema_that_names_an_undeclared_source_type(self):
        schema = json.loads(SCHEMA_TEXT)
        schema["annotations"]["objects"]["Spot"]["type"] = "HiddenView"
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = repo(tmp, {"docs/a.md": "alpha\n", oraclekit.SCHEMA: json.dumps(schema), oraclekit.CODE_TABLES_SOURCE: CODE_TABLES_TEXT,
                              "src/o.cpp": SCHEMA_SOURCE})
            kit = Path(out) / "k"
            with self.assertRaises(oraclekit.Refusal) as caught:
                oraclekit.export(root, kit, True, {"records": {}}, SCHEMA_CONTENTS)
            self.assertIn("data/replay_schema.json: /annotations/objects/Spot/type: 'HiddenView' names 'HiddenView'", str(caught.exception))
            self.assertFalse(kit.exists())

    def test_a_function_only_the_source_and_the_schema_name_is_refused_in_a_document(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root = schema_repo(tmp, "alpha\n\nIts only caller, `AttackChaseOrder`, runs for an armed unit.\n")
            kit = Path(out) / "k"
            with self.assertRaises(oraclekit.Refusal) as caught:
                oraclekit.export(root, kit, True, {"records": {}}, SCHEMA_CONTENTS)
            self.assertIn("docs/a.md:3: function 'AttackChaseOrder' is declared by no interface header", str(caught.exception))
            self.assertIn("docs/a.md:3: identifier 'AttackChaseOrder' is in the source but not in the kit", str(caught.exception))
            self.assertFalse(kit.exists())

    def test_a_code_table_entry_without_a_row_or_a_row_without_an_entry_is_refused(self):
        for tables, message in ((CODE_TABLES_TEXT.replace("orderNotify,ReadyOrder,Ready\n", ""), "orderNotify[ReadyOrder] has no row"),
                                (CODE_TABLES_TEXT + "orderNotify,StopOrder,Stop\n", "names orderNotify[StopOrder]")):
            with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
                root = schema_repo(tmp, tables=tables)
                with self.assertRaises(oraclekit.Refusal) as caught:
                    oraclekit.export(root, Path(out) / "k", True, {"records": {}}, SCHEMA_CONTENTS)
                self.assertIn(message, str(caught.exception))

    def test_the_real_code_tables_have_a_kit_name_for_every_entry(self):
        names = oraclekit.code_table_names((ROOT / oraclekit.CODE_TABLES_SOURCE).read_bytes())
        kit = json.loads(oraclekit.kit_schema((ROOT / oraclekit.SCHEMA).read_bytes(), names))
        self.assertEqual(kit["annotations"]["code_tables"]["weaponFire"]["entries"], ["turret", "vLaunch", "lineOfSight or selfProp", "dropped"])
        self.assertIn("Attack_Chase", kit["annotations"]["code_tables"]["orderNotify"]["entries"])
        self.assertEqual(oraclekit.names_source(json.dumps(kit["annotations"]["code_tables"]).encode()), [])

    def test_a_coupling_item_that_names_a_source_file_is_renamed(self):
        names = oraclekit.Names({"g_orderTable", "order_list"}, {}, {"g_orderTable": "const"})
        self.assertEqual(names.item("g_orderTable@src/orders/order_list.cpp"), "heap")
        self.assertEqual(names.item("g_orderTable"), "g_orderTable")


def coupling_export(tmp: str, out: Path, counts: dict | None = None) -> tuple[Path, dict[str, str]]:
    root = repo(tmp, {"src/a.cpp": COUPLING_SOURCE})
    oraclekit.export(root, out, True, COUPLING_LAYOUT, CONTENTS + [("dir", "interfaces", "")], counts=counts)
    return root, {p.name: p.read_text() for p in (out / oraclekit.COUPLING_DIR).glob("*.csv")}


class CouplingTest(unittest.TestCase):
    def test_the_coupling_files_are_exported(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            _, files = coupling_export(tmp, Path(out) / "k")
            self.assertEqual(sorted(files), sorted(n + ".csv" for n in oraclekit.COUPLING))

    def test_the_presentation_feeds_are_exported_with_their_reason(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            _, files = coupling_export(tmp, Path(out) / "k")
            self.assertEqual(files["presentation_feeds.csv"],
                             "writer,reader,item,phase,example,reason\npresentation,units,Unit::flags,units,a@1,Selection\n")

    def test_the_function_column_of_draws_becomes_a_site_number(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            _, files = coupling_export(tmp, Path(out) / "k")
            lines = files["draws.csv"].splitlines()
            self.assertEqual(lines[0], "pass,stream,subsystem,site,entities,draws_min,draws_max,ticks,start_state,evidence,example")
            self.assertEqual([l.split(",")[3] for l in lines[1:]], ["1", "2"])
            self.assertTrue(lines[1].startswith("units,rng,ai,1,"))

    def test_draws_evidence_renames_allocations_and_drops_undefined_items(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            _, files = coupling_export(tmp, Path(out) / "k")
            self.assertTrue(files["draws.csv"].splitlines()[2].endswith("depends-on-earlier,Unit::flags;heap;const,a@2"))

    def test_pass_reads_drop_the_function_and_merge(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            _, files = coupling_export(tmp, Path(out) / "k")
            self.assertEqual(files["pass_reads.csv"], "pass,item,reader,evidence,runs,example\nunits,Unit::flags,ai,same-record,5,a@2\nunits,const,ai,other-record,1,a@3\n")

    def test_indirect_rows_are_kept_with_the_slot_or_call_as_item(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            _, files = coupling_export(tmp, Path(out) / "k")
            rows = files["dependencies.csv"].splitlines()
            self.assertNotIn(",DeleteOrders,", files["dependencies.csv"])
            self.assertIn("ai,orders,orders.DeleteOrders,-,unobserved,0,,indirect", rows)
            self.assertIn("ai,units,call,-,unobserved,0,,indirect", rows)
            self.assertIn("ai,orders,Unit::orders,-,unobserved,0,,", rows)
            self.assertNotIn("g_hiddenTable", files["dependencies.csv"])
            self.assertIn("ai,units,const,players,immediate,7,a@9,", rows)
            self.assertEqual(sum(r.startswith("ai,units,const,") for r in rows), 1)
            self.assertIn("-,units,rng,players,ordered,1,a@3,", rows)

    def test_allocation_sites_take_a_state_class_or_heap_and_the_renames_are_counted(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            counts: dict[str, int] = {}
            _, files = coupling_export(tmp, Path(out) / "k", counts)
            rows = files["presentation_reads.csv"].splitlines()
            self.assertIn("heap,map,unknown,,own,1", rows)
            self.assertIn("sim,map,unknown,,own,2", rows)
            self.assertEqual(counts, {"slot": 1, "call": 1, "state class": 1, "heap": 2, "undefined class": 4,
                                      "undefined heap": 0, "merged": 2})
            self.assertEqual(oraclekit.renames(counts), "oraclekit: renamed 1 indirect items to interface.Slot, 1 to call, "
                                                       "1 allocation sites to a state class, 2 to heap\n"
                                                       "oraclekit: renamed 4 undefined items to a state class, 0 to heap; "
                                                       "merged 2 rows that then shared a key")

    def test_no_function_name_is_exported_and_the_leak_check_passes(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root, files = coupling_export(tmp, Path(out) / "k")
            text = "".join(files.values())
            for name in ("ReactToAttack", "ChooseBuildOption", "MysteryCall", "g_hiddenTable", "parkMiller"):
                self.assertNotIn(name, text)
            kit = {p: b for p, b in ((q.relative_to(Path(out) / "k").as_posix(), q.read_bytes())
                                     for q in (Path(out) / "k").rglob("*") if q.is_file())}
            self.assertEqual(oraclekit.leak_check(kit, root), [])

    def test_the_leak_check_fails_on_the_unfiltered_files(self):
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as out:
            root, _ = coupling_export(tmp, Path(out) / "k")
            raw = {"layout.json": json.dumps(COUPLING_LAYOUT).encode(),
                   "coupling/draws.csv": COUPLING_DATA["data/draws.csv"].encode()}
            hits = oraclekit.leak_check(raw, root)
            self.assertTrue(any("ReactToAttack" in h for h in hits), hits)


if __name__ == "__main__":
    unittest.main()
