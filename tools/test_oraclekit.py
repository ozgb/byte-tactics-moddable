"""Tests for oraclekit.py: the refusals, the manifest's hashes and the tag rule.

    uv run tools/test_oraclekit.py
"""

import hashlib
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

import oraclekit

CONTENTS = [("file", "docs/a.md", ""), ("dir", "data", "")]


def git(root: Path, *args: str) -> None:
    subprocess.run(["git", "-C", str(root), "-c", "user.name=t", "-c", "user.email=t@t", "-c", "commit.gpgsign=false",
                    "-c", "tag.gpgsign=false", *args], check=True,
                   capture_output=True)


def repo(tmp: str, extra: dict[str, str] | None = None) -> Path:
    root = Path(tmp)
    files = {"docs/a.md": "alpha\n", "data/x.json": "{}\n", "data/y/z.txt": "z\n", oraclekit.README_SOURCE: "readme\n",
             oraclekit.LAYOUT_REFS: "function\n", **(extra or {})}
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
            self.assertEqual(paths, sorted(["docs/a.md", "data/x.json", "data/y/z.txt", "data/layout_refs.csv", "layout.json", "README.md"]))
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

    def test_the_clang_type_paths_are_cleaned(self):
        self.assertEqual(oraclekit.clean_type("union W::(anonymous at /a/b/c.h:24:5) "), "union W::(anonymous) ")
        self.assertEqual(oraclekit.clean_type("struct W::(unnamed union)::(unnamed at /a/c.h:61:9)"),
                         "struct W::(unnamed union)::(unnamed)")


if __name__ == "__main__":
    unittest.main()
