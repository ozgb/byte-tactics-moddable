"""Tests for tidy.py, on temporary git repositories.

    uv run tools/test_tidy.py
"""

import contextlib
import io
import json
import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import tidy

GIT = ["git", "-c", "user.name=t", "-c", "user.email=t@example.com", "-c", "commit.gpgsign=false"]

SOURCE = """#include "a.h"
int Add(int a, int b)
{
    int total = a + b;
    return total;
}
"""

HEADER = """#ifndef A_H
#define A_H
int Add(int a, int b);
#endif
"""

LEGACY = "int FUN_00401000(void);\nint Old(void) { return FUN_00401000(); }\n"


def finding(root, path, line, check="bugprone-assignment-in-if-condition"):
    return f"{root}/{path}:{line}:5: warning: an assignment within an 'if' condition [{check}]\n"


class Stub:
    """A runner that answers with canned clang-tidy output per file."""

    def __init__(self, root, answers):
        self.root = root
        self.answers = answers
        self.calls = []

    def __call__(self, clang_tidy, root, build_dir, path):
        base = Path(root).resolve() != self.root
        self.calls.append(("base", path) if base else path)
        status, text = self.answers.get(("base", path) if base else path, (0, ""))
        return status, text.replace("ROOT", str(root))


class Repo:
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name).resolve()
        self.git("init", "-q")
        self.write("src/a.cpp", SOURCE)
        self.write("src/a.h", HEADER)
        self.write("src/b.cpp", '#include "a.h"\n' + LEGACY)
        self.write("src/c.cpp", "int C(void) { return 0; }\n")
        self.write_db(["src/a.cpp", "src/b.cpp", "src/c.cpp"])
        self.commit()

    def git(self, *args):
        subprocess.run([*GIT, *args], cwd=self.root, check=True, capture_output=True)

    def write(self, path, text):
        p = self.root / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text)

    def write_db(self, files, command="clang++ -c"):
        db = [{"directory": str(self.root), "file": str(self.root / f), "command": f"{command} {self.root / f}"}
              for f in files]
        self.write("build/modern/compile_commands.json", json.dumps(db))

    def commit(self):
        self.git("add", "-A")
        self.git("commit", "-q", "-m", "c")

    def edit(self, path, old, new):
        p = self.root / path
        p.write_text(p.read_text().replace(old, new, 1))
        self.commit()

    def lint(self, answers=None):
        stub = Stub(self.root, answers or {})
        found = tidy.lint(str(self.root), "HEAD~1...HEAD", jobs=1, runner=stub)
        return found, stub


    def main(self, argv):
        out, err = io.StringIO(), io.StringIO()
        cwd = os.getcwd()
        os.chdir(self.root)
        try:
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                status = tidy.main(argv)
        finally:
            os.chdir(cwd)
        return status, out.getvalue(), err.getvalue()

    def placeholders(self):
        return tidy.lint_placeholders(str(self.root), "HEAD~1...HEAD", tidy.added_lines(str(self.root), "HEAD~1...HEAD"))


class TidyTest(Repo, unittest.TestCase):
    def test_finding_on_added_line_reported(self):
        self.edit("src/a.cpp", "    return total;", "    if (total = 1) {}\n    return total;")
        found, stub = self.lint({"src/a.cpp": (0, finding("ROOT", "src/a.cpp", 5))})
        self.assertEqual(found, [("src/a.cpp", 5, "an assignment within an 'if' condition [bugprone-assignment-in-if-condition]")])
        self.assertEqual(stub.calls, ["src/a.cpp"])

    def test_finding_on_unchanged_line_ignored(self):
        self.edit("src/a.cpp", "    return total;", "    return total + 0;")
        found, _ = self.lint({"src/a.cpp": (0, finding("ROOT", "src/a.cpp", 4))})
        self.assertEqual(found, [])

    def test_duplicate_findings_from_several_units_reported_once(self):
        self.edit("src/a.h", "int Add(int a, int b);", "int Add(int a, int b);\nint Sub(int a, int b);")
        text = finding("ROOT", "src/a.h", 4)
        found, stub = self.lint({"src/a.cpp": (0, text), "src/b.cpp": (0, text)})
        self.assertEqual([f[:2] for f in found], [("src/a.h", 4)])
        self.assertEqual(stub.calls, ["src/a.cpp"])

    def test_changed_header_runs_first_unit_that_includes_it(self):
        self.edit("src/a.h", "int Add(int a, int b);", "int Add(int a, int b);\nint Sub(int a, int b);")
        _, stub = self.lint()
        self.assertEqual(stub.calls, ["src/a.cpp"])

    def test_changed_header_reached_through_another_header(self):
        self.write("src/mid.h", '#include "a.h"\n')
        self.write("src/d.cpp", '#include "mid.h"\n')
        self.write("src/b.cpp", "int B(void) { return 0; }\n")
        self.write("src/a.cpp", "int A(void) { return 0; }\n")
        self.write_db(["src/a.cpp", "src/b.cpp", "src/c.cpp", "src/d.cpp"])
        self.commit()
        self.edit("src/a.h", "int Add(int a, int b);", "int Add(int a, int b);\nint Sub(int a, int b);")
        _, stub = self.lint()
        self.assertEqual(stub.calls, ["src/d.cpp"])

    def test_files_outside_src_and_interfaces_not_run(self):
        self.write("tools/x.cpp", "int X(void) { return 0; }\n")
        self.write("README.md", "hello\n")
        self.commit()
        _, stub = self.lint()
        self.assertEqual(stub.calls, [])

    def test_diagnostic_outside_src_ignored(self):
        self.edit("src/a.cpp", "    return total;", "    return total + 0;")
        found, _ = self.lint({"src/a.cpp": (0, finding("ROOT", "build/modern/include/VECTOR", 4))})
        self.assertEqual(found, [])

    def test_parse_failure_is_a_tool_error(self):
        self.edit("src/a.cpp", "    return total;", "    return total + 0;")
        error = "ROOT/src/a.cpp:1:10: error: 'a.h' file not found [clang-diagnostic-error]\n"
        with self.assertRaises(tidy.ToolError) as ctx:
            self.lint({"src/a.cpp": (1, error)})
        self.assertIn("file not found", str(ctx.exception))

    def test_failure_without_diagnostics_is_a_tool_error(self):
        self.edit("src/a.cpp", "    return total;", "    return total + 0;")
        with self.assertRaises(tidy.ToolError):
            self.lint({"src/a.cpp": (1, "LLVM ERROR: out of memory\n")})

    def test_missing_compile_commands_is_a_tool_error(self):
        os.unlink(self.root / "build/modern/compile_commands.json")
        self.edit("src/a.cpp", "    return total;", "    return total + 0;")
        with self.assertRaises(tidy.ToolError):
            self.lint()

    def test_main_exits_2_on_parse_failure(self):
        self.edit("src/a.cpp", "    return total;", "    return total + 0;")
        fake = self.root / "fake-tidy"
        fake.write_text("#!/bin/sh\necho \"$PWD/src/a.cpp:1:10: error: 'a.h' file not found [clang-diagnostic-error]\"\nexit 1\n")
        fake.chmod(0o755)
        status, out, err = self.main(["HEAD~1...HEAD", "--clang-tidy", str(fake)])
        self.assertEqual(status, 2)
        self.assertIn("file not found", err)

    def test_main_exits_1_with_findings_and_0_without(self):
        self.edit("src/a.cpp", "    return total;", "    if (total = 1) {}\n    return total;")
        fake = self.root / "fake-tidy"
        fake.write_text(f"#!/bin/sh\necho \"$PWD/src/a.cpp:5:5: warning: assignment [bugprone-x]\"\n")
        fake.chmod(0o755)
        status, out, _ = self.main(["HEAD~1...HEAD", "--clang-tidy", str(fake)])
        self.assertEqual((status, out), (1, "src/a.cpp:5: assignment [bugprone-x]\n"))
        fake.write_text("#!/bin/sh\nexit 0\n")
        status, out, _ = self.main(["HEAD~1...HEAD", "--clang-tidy", str(fake)])
        self.assertEqual((status, out), (0, ""))

    def test_all_reports_every_finding(self):
        stub = Stub(self.root, {"src/a.cpp": (0, finding("ROOT", "src/a.cpp", 1) + finding("ROOT", "src/a.cpp", 4))})
        found = tidy.lint_all(str(self.root), ["src/a.cpp"], jobs=1, runner=stub)
        self.assertEqual([f[:2] for f in found], [("src/a.cpp", 1), ("src/a.cpp", 4)])



    def test_new_placeholder_names_fail(self):
        names = ["FUN_00bad001", "DAT_00bad001", "field_1f4", "unk_8", "unknown_1c", "unkFlags", "param_3", "Thing_00bad001"]
        for name in names:
            with self.subTest(name=name):
                self.edit("src/c.cpp", "return 0;", f"return {name};")
                found = self.placeholders()
                self.assertEqual([f[:2] for f in found], [("src/c.cpp", 1)], found)
                self.assertIn(name, found[0][2])
                self.git("reset", "-q", "--hard", "HEAD~1")

    def test_name_already_in_the_tree_is_new_when_added_again(self):
        self.edit("src/c.cpp", "return 0;", "return FUN_00401000();")
        found = self.placeholders()
        self.assertEqual([f[:2] for f in found], [("src/c.cpp", 1)], found)

    def test_new_common_placeholder_member_fails_though_the_tree_has_it(self):
        self.write("src/d.h", "struct D { int field_10; };\n")
        self.commit()
        self.write("src/e.h", "struct E { int field_10; };\n")
        self.commit()
        found = self.placeholders()
        self.assertEqual([f[:2] for f in found], [("src/e.h", 1)], found)
        self.assertIn("field_10", found[0][2])

    def test_moved_placeholder_name_passes(self):
        self.write("src/b.cpp", '#include "a.h"\n')
        self.write("src/c.cpp", LEGACY)
        self.commit()
        self.assertEqual(self.placeholders(), [])

    def test_move_out_of_a_deleted_file_passes(self):
        self.write("src/c.cpp", LEGACY)
        self.git("rm", "-q", "src/b.cpp")
        self.commit()
        self.assertEqual(self.placeholders(), [])

    def test_name_added_twice_and_removed_once_fails(self):
        self.write("src/b.cpp", '#include "a.h"\n')
        self.write("src/c.cpp", LEGACY + LEGACY)
        self.commit()
        found = self.placeholders()
        self.assertEqual({f[:2] for f in found}, {("src/c.cpp", n) for n in (1, 2, 3, 4)}, found)

    def test_naming_regexes_only_exempt_placeholder_forms(self):
        text = (Path(__file__).resolve().parent.parent / ".clang-tidy").read_text()
        rows = re.findall(r"IgnoredRegexp: '(.*)'", text)
        self.assertGreater(len(rows), 10)
        for row in rows:
            regex = re.compile("^(" + row[row.index(".*_00"):-3] + ")$")
            for name in ("is_bad", "do_fade", "add_face", "set_dead"):
                self.assertIsNone(regex.match(name), (name, row))
            for name in ("Class_00463be0", "field_10", "unknown_4", "param_1", "unk_8", "vec_c"):
                self.assertIsNotNone(regex.match(name), (name, row))

    def test_name_in_comment_or_string_ignored(self):
        self.edit("src/c.cpp", "return 0;", 'return 0; // FUN_00bad001\nconst char *s = "DAT_00bad001";')
        self.assertEqual(self.placeholders(), [])

    def test_ordinary_names_pass(self):
        names = ["unit", "unkind", "fieldWidth", "field", "paramCount", "Class_x", "Thing_0123", "FUN_xyz", "uniform"]
        self.edit("src/c.cpp", "return 0;", "return 0; " + " ".join(f"int {n};" for n in names))
        self.assertEqual(self.placeholders(), [])

    def test_placeholder_failure_without_clang_tidy_findings(self):
        self.edit("src/c.cpp", "return 0;", "return FUN_00bad001();")
        found, _ = self.lint()
        self.assertEqual([f[:2] for f in found], [("src/c.cpp", 1)])

    def test_modified_line_keeping_an_old_finding_passes(self):
        self.edit("src/a.cpp", "    int total = a + b;", "    int total = a + b;\n    int t = 1;")
        self.edit("src/a.cpp", "    int t = 1;", "    int t = 2;")
        text = finding("ROOT", "src/a.cpp", 5, "modernize-use-nullptr")
        found, stub = self.lint({"src/a.cpp": (0, text), ("base", "src/a.cpp"): (0, text)})
        self.assertEqual(found, [])
        self.assertIn(("base", "src/a.cpp"), stub.calls)

    def test_modified_line_adding_a_new_check_fails(self):
        self.edit("src/a.cpp", "    return total;", "    return total;\n    int t = 1;")
        self.edit("src/a.cpp", "    int t = 1;", "    int t = 2;")
        old = finding("ROOT", "src/a.cpp", 6, "modernize-use-nullptr")
        new = old + finding("ROOT", "src/a.cpp", 6)
        found, _ = self.lint({"src/a.cpp": (0, new), ("base", "src/a.cpp"): (0, old)})
        self.assertEqual([(f[0], f[1]) for f in found], [("src/a.cpp", 6)])
        self.assertIn("bugprone-assignment-in-if-condition", found[0][2])

    def test_modified_line_with_same_check_but_other_message_fails(self):
        self.edit("src/a.cpp", "    return total;", "    return total;\n    int t = 1;")
        self.edit("src/a.cpp", "    int t = 1;", "    int t = 2;")
        head = finding("ROOT", "src/a.cpp", 6).replace("an assignment", "another assignment")
        found, _ = self.lint({"src/a.cpp": (0, head), ("base", "src/a.cpp"): (0, finding("ROOT", "src/a.cpp", 6))})
        self.assertEqual(len(found), 1)

    def test_pure_addition_with_the_same_finding_as_elsewhere_fails(self):
        self.edit("src/a.cpp", "    return total;", "    int t = 1;\n    return total;")
        text = finding("ROOT", "src/a.cpp", 4) + finding("ROOT", "src/a.cpp", 5)
        found, stub = self.lint({"src/a.cpp": (0, text), ("base", "src/a.cpp"): (0, finding("ROOT", "src/a.cpp", 4))})
        self.assertEqual([f[:2] for f in found], [("src/a.cpp", 5)])
        self.assertNotIn(("base", "src/a.cpp"), stub.calls)

    def test_base_parse_failure_counts_as_no_base_findings(self):
        self.edit("src/a.cpp", "    return total;", "    return total;\n    int t = 1;")
        self.edit("src/a.cpp", "    int t = 1;", "    int t = 2;")
        text = finding("ROOT", "src/a.cpp", 6)
        error = "ROOT/src/a.cpp:1:10: error: boom [clang-diagnostic-error]\n"
        found, _ = self.lint({"src/a.cpp": (0, text), ("base", "src/a.cpp"): (1, error)})
        self.assertEqual(len(found), 1)

    def test_untracked_include_dir_is_visible_in_the_base_worktree(self):
        self.write_db(["src/a.cpp"], f"clang++ -imsvc {self.root}/toolchain/INC -c")
        self.commit()
        self.edit("src/a.cpp", "    return total;", "    return total;\n    int t = 1;")
        self.edit("src/a.cpp", "    int t = 1;", "    int t = 2;")
        (self.root / "toolchain/INC").mkdir(parents=True)
        text = finding("ROOT", "src/a.cpp", 6)

        def runner(clang_tidy, root, build_dir, path):
            if Path(root).resolve() == self.root:
                return 0, text.replace("ROOT", str(root))
            command = json.loads((Path(build_dir) / "compile_commands.json").read_text())[0]["command"]
            include = command.split("-imsvc ")[1].split()[0]
            if not Path(include).exists():
                return 1, f"{root}/{path}:1:10: error: 'vector' file not found [clang-diagnostic-error]\n"
            return 0, text.replace("ROOT", str(root))

        found = tidy.lint(str(self.root), "HEAD~1...HEAD", jobs=1, runner=runner)
        self.assertEqual(found, [])

    def test_overlay_roots_follow_the_include_dir_into_the_base_worktree(self):
        overlay = self.root / "build/modern/overlay.json"
        command = f"clang++ -imsvc {self.root}/toolchain/INC -vfsoverlay {overlay} -c"
        self.write_db(["src/a.cpp"], command)
        self.write("build/modern/overlay.json", json.dumps({"roots": [{"name": f"{self.root}/toolchain/INC"}]}))
        self.commit()
        self.edit("src/a.cpp", "    return total;", "    return total;\n    int t = 1;")
        self.edit("src/a.cpp", "    int t = 1;", "    int t = 2;")
        (self.root / "toolchain/INC").mkdir(parents=True)
        text = finding("ROOT", "src/a.cpp", 6)

        def runner(clang_tidy, root, build_dir, path):
            if Path(root).resolve() == self.root:
                return 0, text.replace("ROOT", str(root))
            command = json.loads((Path(build_dir) / "compile_commands.json").read_text())[0]["command"]
            include = command.split("-imsvc ")[1].split()[0]
            names = [r["name"] for o in command.split("-vfsoverlay ")[1:]
                     for r in json.loads(Path(o.split()[0]).read_text())["roots"]]
            if include not in names:
                return 1, f"{root}/{path}:1:10: error: 'vector' file not found [clang-diagnostic-error]\n"
            return 0, text.replace("ROOT", str(root))

        found = tidy.lint(str(self.root), "HEAD~1...HEAD", jobs=1, runner=runner)
        self.assertEqual(found, [])

    def test_range_not_ending_at_head_is_a_tool_error(self):
        self.edit("src/c.cpp", "return 0;", "return 1;")
        self.git("branch", "later")
        self.edit("src/c.cpp", "return 1;", "return 2;")
        with self.assertRaises(tidy.ToolError) as ctx:
            tidy.lint(str(self.root), "HEAD~2...later", jobs=1, runner=Stub(self.root, {}))
        self.assertIn("must end at HEAD", str(ctx.exception))
        tidy.lint(str(self.root), "HEAD~1...", jobs=1, runner=Stub(self.root, {}))

    def test_uncommitted_change_to_a_changed_file_is_a_tool_error(self):
        self.edit("src/c.cpp", "return 0;", "return 1;")
        self.write("src/c.cpp", "int C(void) { return 2; }\n")
        with self.assertRaises(tidy.ToolError) as ctx:
            self.lint()
        self.assertIn("uncommitted", str(ctx.exception))
        self.git("checkout", "src/c.cpp")
        self.write("src/b.cpp", "int B(void) { return 2; }\n")
        self.lint()

    def test_main_exits_2_when_the_range_does_not_end_at_head(self):
        self.edit("src/c.cpp", "return 0;", "return 1;")
        status, _, err = self.main(["HEAD~1...HEAD~1", "--clang-tidy", "true"])
        self.assertEqual(status, 2)
        self.assertIn("must end at HEAD", err)

    def test_base_worktree_is_removed(self):
        self.edit("src/a.cpp", "    return total;", "    return total;\n    int t = 1;")
        self.edit("src/a.cpp", "    int t = 1;", "    int t = 2;")
        self.lint({"src/a.cpp": (0, finding("ROOT", "src/a.cpp", 6))})
        listing = subprocess.run(["git", "worktree", "list"], cwd=self.root, capture_output=True, text=True).stdout
        self.assertEqual(len(listing.splitlines()), 1, listing)

    def test_touching_a_line_with_an_old_placeholder_passes(self):
        self.edit("src/b.cpp", "return FUN_00401000();", "return FUN_00401000( );")
        found, _ = self.lint({"src/b.cpp": (0, "")})
        self.assertEqual(found, [])

    def test_range_base_with_two_dots(self):
        self.edit("src/c.cpp", "return 0;", "return FUN_00bad001();")
        found = tidy.lint_placeholders(str(self.root), "HEAD~1..HEAD", tidy.added_lines(str(self.root), "HEAD~1..HEAD"))
        self.assertEqual(len(found), 1)


@unittest.skipUnless(shutil.which("clang-tidy"), "clang-tidy not on PATH")
class RealClangTidyTest(Repo, unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.write(".clang-tidy", "Checks: 'bugprone-assignment-in-if-condition,-clang-diagnostic-*'\nHeaderFilterRegex: '(src|interfaces)/'\n")
        self.write("src/a.cpp", SOURCE.replace('#include "a.h"\n', ""))
        self.write("src/a.h", HEADER)
        self.write("src/c.cpp", "int C(int x) { return x; }\n")
        self.write_db(["src/a.cpp", "src/b.cpp", "src/c.cpp"], "clang++ -std=c++11 -I src -c")
        self.commit()

    def real(self):
        return tidy.lint(str(self.root), "HEAD~1...HEAD", jobs=1)

    def test_real_finding_on_added_line_only(self):
        self.edit("src/c.cpp", "return x;", "if (x = 2) {}\n    return x;")
        found = self.real()
        self.assertEqual([f[:2] for f in found], [("src/c.cpp", 1)], found)
        self.assertIn("bugprone-assignment-in-if-condition", found[0][2])
        self.edit("src/c.cpp", "if (x = 2) {}", "if (x = 3) {}")
        self.assertEqual(self.real(), [])
        self.edit("src/c.cpp", "return x;", "return x + 0;")
        self.assertEqual(self.real(), [])

    def test_real_syntax_error_is_a_tool_error(self):
        self.edit("src/c.cpp", "return x;", "return x +;")
        with self.assertRaises(tidy.ToolError):
            self.real()


if __name__ == "__main__":
    unittest.main()
