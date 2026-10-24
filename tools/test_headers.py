"""Tests for headers.py.

    uv run tools/test_headers.py
"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path

import headers

FILES = {
    "src/types.h": """#ifndef TYPES_H
#define TYPES_H
struct Point { int x; int y; };
typedef unsigned int Handle;
#endif
""",
    "src/a.cpp": """#include "types.h"

// FUNCTION: 0x401000
int __stdcall Add(Point* p, Handle h)
{
    return p->x + h;
}

// FUNCTION: 0x401010
int __cdecl Cd(int x)
{
    return x;
}

// FUNCTION: 0x401020
int Plain(int x)
{
    return x;
}

static int Hidden(int x)
{
    return x;
}

namespace {
int Anon(int x)
{
    return x;
}
}

// FUNCTION: 0x401030
int Counter::Next()
{
    return 1;
}

// FUNCTION: 0x401040
extern "C" int __cdecl CLink(int x)
{
    return x;
}

// FUNCTION: 0x401050
int __stdcall Defaulted(int a, int b = 3)
{
    return a + b;
}

// FUNCTION: 0x401060
void* __stdcall Wrong(int a)
{
    return 0;
}

int g_count = 1;
int g_table[4] = {1, 2, 3, 4};
static int g_hidden = 2;
""",
    "src/d.cpp": """// FLAGS: /Gd
// FUNCTION: 0x402000
int FromCdecl(int x)
{
    return x;
}
""",
    "src/b.cpp": """#include "types.h"

int __stdcall Add(Point*, Handle);
int __cdecl Cd(int);
int Plain(int);
int Hidden(int);
extern "C" int __cdecl CLink(int);
int __stdcall Defaulted(int, int = 3);
int __stdcall Wrong(int);
int __cdecl FromCdecl(int);
extern int g_count;
extern int g_table[];
extern int g_hidden;
int __stdcall Imported(int);

int use()
{
    return Add(0, 1) + Cd(1) + Plain(2) + CLink(3) + Defaulted(1) + Wrong(1) + FromCdecl(2) + g_count + g_table[1] + Imported(1);
}
""",
    "src/c.cpp": """int __stdcall Add(struct Point*, unsigned int);
int Plain(int);
extern int g_count;

int use2()
{
    return Add(0, 1) + Plain(2) + g_count;
}
""",
}


def make(tmp, files=FILES):
    for name, text in files.items():
        f = Path(tmp) / name
        f.parent.mkdir(parents=True, exist_ok=True)
        f.write_text(text)


def run(tmp, *args):
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        code = headers.main(["--root", str(tmp), *args])
    return code, out.getvalue()


class HeadersTest(unittest.TestCase):
    def setUp(self):
        self._dir = tempfile.TemporaryDirectory()
        self.tmp = Path(self._dir.name)
        make(self.tmp)

    def tearDown(self):
        self._dir.cleanup()

    def text(self, name):
        return (self.tmp / name).read_text()

    def test_function_used_in_two_files(self):
        run(self.tmp)
        header = self.text("src/a.h")
        self.assertIn("int __stdcall Add(Point* p, Handle h);", header)
        self.assertIn('#include "types.h"', header)
        self.assertIn("struct Point;", header)
        for name in ("src/b.cpp", "src/c.cpp"):
            text = self.text(name)
            self.assertIn('#include "a.h"', text)
            self.assertNotIn("Add(Point*", text.replace("Add(0", ""))
        self.assertEqual(self.text("src/c.cpp").count('#include "a.h"'), 1)
        self.assertNotIn("struct Point*, unsigned", self.text("src/c.cpp"))

    def test_global_and_array(self):
        run(self.tmp)
        header = self.text("src/a.h")
        self.assertIn("extern int g_count;", header)
        self.assertIn("extern int g_table[4];", header)
        self.assertNotIn("extern int g_count", self.text("src/b.cpp"))
        self.assertNotIn("extern int g_table", self.text("src/b.cpp"))

    def test_static_and_anonymous_skipped(self):
        run(self.tmp)
        header = self.text("src/a.h")
        self.assertNotIn("Hidden", header)
        self.assertNotIn("Anon", header)
        self.assertNotIn("g_hidden", header)
        self.assertIn("int Hidden(int);", self.text("src/b.cpp"))
        self.assertIn("extern int g_hidden;", self.text("src/b.cpp"))

    def test_method_skipped(self):
        run(self.tmp)
        self.assertNotIn("Next", self.text("src/a.h"))
        self.assertNotIn("Counter", self.text("src/a.h"))

    def test_convention_spelled_out(self):
        run(self.tmp)
        header = self.text("src/a.h")
        self.assertIn("int __stdcall Plain(int x);", header)
        self.assertIn("int __cdecl Cd(int x);", header)
        self.assertIn("int __cdecl FromCdecl(int x);", self.text("src/d.h"))

    def test_c_linkage(self):
        run(self.tmp)
        self.assertIn('extern "C" int __cdecl CLink(int x);', self.text("src/a.h"))

    def test_default_arguments_move_to_the_header(self):
        run(self.tmp)
        self.assertIn("int __stdcall Defaulted(int a, int b = 3);", self.text("src/a.h"))
        self.assertIn("int __stdcall Defaulted(int a, int b)\n", self.text("src/a.cpp"))
        self.assertIn('#include "a.h"', self.text("src/a.cpp"))

    def test_mismatch_reported_and_left(self):
        code, out = run(self.tmp, "--dry-run")
        self.assertEqual(code, 0)
        self.assertIn("src/b.cpp:9: Wrong", out)
        self.assertIn("return type int vs void*", out)
        run(self.tmp)
        self.assertIn("int __stdcall Wrong(int);", self.text("src/b.cpp"))
        self.assertNotIn("Wrong", self.text("src/a.h"))

    def test_no_definition_left_alone(self):
        run(self.tmp)
        self.assertIn("int __stdcall Imported(int);", self.text("src/b.cpp"))

    def test_rerun_changes_nothing(self):
        run(self.tmp)
        before = {p.name: p.read_text() for p in self.tmp.rglob("*") if p.is_file()}
        run(self.tmp)
        after = {p.name: p.read_text() for p in self.tmp.rglob("*") if p.is_file()}
        self.assertEqual(before, after)
        code, out = run(self.tmp, "--dry-run")
        self.assertIn("headers created: 0", out)
        self.assertIn("prototypes removed: 0", out)

    def test_check(self):
        code, out = run(self.tmp, "--check")
        self.assertEqual(code, 1)
        self.assertIn("would change src/b.cpp", out)
        run(self.tmp)
        code, out = run(self.tmp, "--check")
        self.assertEqual(code, 1)
        self.assertIn("Wrong disagrees with its definition", out)
        self.assertNotIn("would change", out)
        run(self.tmp, "--dry-run")
        (self.tmp / "src/b.cpp").write_text((self.tmp / "src/b.cpp").read_text().replace("int __stdcall Wrong(int);", "void* __stdcall Wrong(int);").replace("int __stdcall Imported(int);\n", "").replace("Imported(1)", "0").replace("int Hidden(int);\n", "").replace("extern int g_hidden;\n", ""))
        self.assertEqual(run(self.tmp, "--check")[0], 1)
        run(self.tmp)
        code, out = run(self.tmp, "--check")
        self.assertEqual((code, out), (0, ""))

    def test_dry_run_changes_nothing(self):
        before = {p.name: p.read_text() for p in self.tmp.rglob("*") if p.is_file()}
        run(self.tmp, "--dry-run")
        self.assertEqual({p.name: p.read_text() for p in self.tmp.rglob("*") if p.is_file()}, before)

    def test_report_file(self):
        run(self.tmp, "--report", str(self.tmp / "report.txt"))
        self.assertIn("mismatches: 1", (self.tmp / "report.txt").read_text())

    def test_existing_header_gets_the_declarations(self):
        (self.tmp / "src/a.h").write_text("#ifndef A_H\n#define A_H\n\nstruct Mine;\n\n#endif\n")
        run(self.tmp)
        header = self.text("src/a.h")
        self.assertTrue(header.startswith("#ifndef A_H\n#define A_H\n"))
        self.assertIn("struct Mine;", header)
        self.assertIn("int __stdcall Plain(int x);", header)
        self.assertTrue(header.rstrip().endswith("#endif"))
        self.assertEqual(header.count("#ifndef"), 1)

    def test_type_defined_in_two_places_is_not_included(self):
        files = dict(FILES)
        files["src/c.cpp"] = "struct Point { int x; };\n" + FILES["src/c.cpp"]
        files["src/types.h"] = FILES["src/types.h"]
        self._dir.cleanup()
        self._dir = tempfile.TemporaryDirectory()
        self.tmp = Path(self._dir.name)
        make(self.tmp, files)
        make(self.tmp, {"src/a.h": '#include "types.h"\n'})
        run(self.tmp)
        self.assertIn("Add(", self.text("src/c.cpp"))

    def test_comments_are_not_written(self):
        run(self.tmp)
        for p in self.tmp.rglob("*.h"):
            self.assertNotIn("//", p.read_text())
            self.assertNotIn("/*", p.read_text())

    def left_out(self, *rows):
        (self.tmp / "data").mkdir(exist_ok=True)
        (self.tmp / "data/headers_left_out.csv").write_text("name,kind,reason\n" + "".join(f"{r}\n" for r in rows))

    def test_listed_left_out_name_passes_the_check(self):
        run(self.tmp)
        self.left_out("Wrong,mismatch,returns a pointer")
        self.assertEqual(run(self.tmp, "--check"), (0, ""))

    def test_unlisted_declaration_fails_the_check(self):
        run(self.tmp)
        self.left_out("Other,mismatch,because")
        code, out = run(self.tmp, "--check")
        self.assertEqual(code, 1)
        self.assertIn("Wrong disagrees with its definition", out)

    def test_listed_name_that_needs_no_listing_fails_the_check(self):
        run(self.tmp)
        self.left_out("Wrong,mismatch,returns a pointer", "Plain,mismatch,stale")
        code, out = run(self.tmp, "--check")
        self.assertEqual(code, 1)
        self.assertIn("Plain (mismatch) is listed but needs no listing", out)
        self.assertNotIn("Wrong", out)

    def test_listing_by_the_wrong_kind_does_not_cover_the_name(self):
        run(self.tmp)
        self.left_out("Wrong,skip,because")
        code, out = run(self.tmp, "--check")
        self.assertEqual(code, 1)
        self.assertIn("Wrong disagrees", out)
        self.assertIn("Wrong (skip) is listed", out)

    def test_placeholder_parameter_names_are_dropped(self):
        make(self.tmp, {"src/e.cpp": "// FUNCTION: 0x403000\nint __stdcall Pp(int param_1, int count)\n{\n    return count;\n}\n",
                        "src/f.cpp": "int __stdcall Pp(int, int);\nint use3() { return Pp(1, 2); }\n"})
        run(self.tmp)
        self.assertIn("int __stdcall Pp(int, int count);", self.text("src/e.h"))

    def test_include_not_added_to_a_file_that_no_longer_uses_the_name(self):
        make(self.tmp, {"src/e.cpp": "// FUNCTION: 0x403000\nint __stdcall Unused(int x)\n{\n    return x;\n}\n",
                        "src/f.cpp": "int __stdcall Unused(int);\nint other() { return 1; }\n"})
        run(self.tmp)
        self.assertNotIn("Unused", self.text("src/f.cpp"))
        self.assertNotIn("#include", self.text("src/f.cpp"))
        self.assertIn("int __stdcall Unused(int x);", self.text("src/e.h"))

    def test_declaration_naming_a_placeholder_type_is_left_in_place(self):
        make(self.tmp, {"src/e.cpp": "struct Thing_00401000 { int x; };\n// FUNCTION: 0x403000\nint __stdcall UseThing(Thing_00401000* t)\n{\n    return t->x;\n}\n",
                        "src/f.cpp": "struct Thing_00401000;\nint __stdcall UseThing(Thing_00401000*);\nint use5(Thing_00401000* t) { return UseThing(t); }\n"})
        run(self.tmp)
        self.assertFalse((self.tmp / "src/e.h").exists())
        self.assertIn("int __stdcall UseThing(Thing_00401000*);", self.text("src/f.cpp"))
        code, out = run(self.tmp, "--check")
        self.assertIn("UseThing cannot be declared in a header: placeholder type Thing_00401000", out)


if __name__ == "__main__":
    unittest.main()
