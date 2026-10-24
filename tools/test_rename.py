"""Tests for rename.py, on a fixture tree in a temp directory.

    uv run tools/test_rename.py
"""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
REAL = TOOLS.parent

FILES = {
    "src/a.cpp": """// FLAGS: /Od
int __cdecl FUN_00401000(int a);
void Walk(int FUN_local, int other)
{
    try { FUN_local = FUN_00401000(other); } catch (...) { }
    const char *s = "FUN_00401000";
    char c = 'F';
}
""",
    "src/b.cpp": "int one = FUN_00402000(1);  // FUN_00402000 comment\nint FUN_00403000, FUN_00403001;\n",
    "src/units/unit.h": "int FUN_00401000(int a);\n",
    "interfaces/seams.h": "extern int FUN_00401000(int);\n",
    "include/ta_types.h": "int FUN_00404000(int);\nint FUN_00404001(int);\nstruct Unit { short field_8; };\n",
    "include/ta_protos.h": "int FUN_00404002(int);\nint FUN_00404003(int);\n",
    "lib/bitexact/bitexact.h": "int bx_FUN_00401000(int);\n",
    "lib/bitexact/trig.c": "int bx_FUN_00401000(int a) { return a; }\n",
    "link/data.cpp": "int g = FUN_00401000(0);\n",
    "tests/t.cpp": "int t = FUN_00401000(0);\n",
    "src/gap.cpp": """asm
{
        // ENTRY: 0x405000
        ret
        // ENTRY: 0x405010 _NamedEntry
        ret
}
""",
    "src/types.cpp": "struct Squad_00406000 { int x; };\n",
    "src/types2.cpp": "struct Squad { int y; };\n",
    "data/symbols.csv": "address,name\n0x401000,FUN_00401000\n0x409000,Taken\n0x40a000,?FUN_00401000@Squad_00406000@@QAEXXZ\n",
    "data/aliases.csv": "name,address,note\n",
    "data/state_classes.csv": ("name,address,kind,proposed,class,rule,note\n"
                               "field_8,0x8,game-region,view,platform,\"a, b\",hand note\n"
                               "FUN_00401000,0x401000,global,sim,sim,r,keep\n"
                               "other,0x9,global,sim,sim,r,mentions field_8 in a note\n"),
    "data/trace_coverage.csv": ("kind,name,address,class,traced,plan,pr,schema_ref,evidence\n"
                                "game-region,field_8,0x8,mixed,yes,none,,,hand evidence\n"),
    "docs/notes.md": "FUN_00401000 is the sine.\n",
}

SCOPED_FILES = {
    "src/game/game.h": """#pragma pack(push, 1)
struct Game {
    union {
        struct { char unknown_0; char version; };
        int field_0;
    };
    int field_4;
    struct Inner { int field_0; } inner;
    int Get() { return field_0 + this->field_0; }
};
#pragma pack(pop)
static_assert(offsetof(Game, field_0) == 0x0, "field_0");
static_assert(offsetof(Game, field_4) == 0x4, "field_4");
""",
    "src/game/other.h": "struct Other { int field_0; int field_4; };\n",
    "src/game/use.cpp": """extern Game* g_game;
extern Other* g_other;
int Game::Total() { return field_0 + field_4; }
int Read(Game* game, Game& ref, Other* other, Unknown* p)
{
    int a = g_game->field_0 + game->field_0 + ref.field_0;
    int b = other->field_0 + g_other->field_0 + p->field_0;
    int c = ((Game*)p)->field_0 + static_cast<Game*>(p)->field_0;
    const char* s = "g_game->field_0";  // g_game->field_0 in a comment
    return a + b + c + g_game->field_4;
}
int Shadow(Other* g_game) { return g_game->field_0; }
""",
    "data/ref_exceptions.csv": "struct,field,kind,file,issue\nGame,field_0,pointer,src/game/game.h,#1\nOther,field_0,pointer,src/game/other.h,#1\n",
    "data/state_classes.csv": ("name,address,kind,proposed,class,rule,note\n"
                               "field_0,0x0,game-region,,platform,r,Game::field_0 note\n"
                               "field_0,0x0,heap,,platform,r,other field_0\n"),
    "data/trace_coverage.csv": "kind,name,address,class\ngame-region,field_0,0x0,mixed\nheap,field_0,0x0,mixed\n",
    "data/hazards.csv": "function,note\nF,Game.field_0 and Other.field_0\n",
}


def write(root: Path, files: dict[str, str]) -> None:
    for name, text in files.items():
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode("latin-1"))


def read(root: Path, name: str) -> str:
    return (root / name).read_bytes().decode("latin-1")


def snapshot(root: Path) -> dict[str, bytes]:
    return {str(p.relative_to(root)): p.read_bytes() for p in sorted(root.rglob("*")) if p.is_file()}


class Fixture(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.root = Path(self.dir.name)
        write(self.root, FILES)

    def tearDown(self):
        self.dir.cleanup()

    def run_rename(self, *args: str, root: Path | None = None) -> subprocess.CompletedProcess:
        return subprocess.run([sys.executable, str(TOOLS / "rename.py"), "--root", str(root or self.root), *args],
                              capture_output=True, text=True)

    def ok(self, *args: str) -> str:
        done = self.run_rename(*args)
        self.assertEqual(done.returncode, 0, done.stdout + done.stderr)
        return done.stdout

    def refused(self, *args: str) -> str:
        before = snapshot(self.root)
        done = self.run_rename(*args)
        self.assertEqual(done.returncode, 2, done.stdout + done.stderr)
        self.assertEqual(snapshot(self.root), before)
        return done.stdout + done.stderr


class Dropped(Fixture):
    def test_local_in_try_function_in_od_file_is_renamed(self):
        self.ok("FUN_local", "count")
        text = read(self.root, "src/a.cpp")
        self.assertIn("int count, int other", text)
        self.assertNotIn("FUN_local", text)

    def test_generated_header_merge_is_renamed(self):
        self.ok("FUN_00404000", "FUN_00404001")
        self.assertEqual(read(self.root, "include/ta_types.h").count("FUN_00404001"), 2)
        self.ok("FUN_00404002", "Merged", "FUN_00404003", "Merged")
        self.assertEqual(read(self.root, "include/ta_protos.h").count("Merged"), 2)

    def test_gap_entry_label_is_named_in_its_annotation(self):
        self.ok("FUN_00405000", "EntryOne")
        text = read(self.root, "src/gap.cpp")
        self.assertIn("// ENTRY: 0x405000 _EntryOne\n", text)
        self.assertIn("// ENTRY: 0x405010 _NamedEntry\n", text)

    def test_named_entry_label_is_renamed(self):
        self.ok("NamedEntry", "Other")
        self.assertIn("// ENTRY: 0x405010 _Other\n", read(self.root, "src/gap.cpp"))

    def test_gap_entry_does_not_swallow_the_next_line(self):
        self.ok("ret", "stop")
        text = read(self.root, "src/gap.cpp")
        self.assertIn("// ENTRY: 0x405000\n        stop\n", text)
        self.assertIn("// ENTRY: 0x405010 _NamedEntry\n        stop\n", text)


class Kept(Fixture):
    def test_not_an_identifier(self):
        self.assertIn("is not a name", self.refused("FUN_00401000", "1x"))

    def test_keyword(self):
        self.assertIn("is not a name", self.refused("FUN_00401000", "class"))

    def test_two_names_of_one_file(self):
        self.assertIn("would both be", self.refused("FUN_00403000", "FUN_00403001"))

    def test_two_olds_to_one_new(self):
        self.assertIn("would both be", self.refused("FUN_00403000", "Same", "FUN_00403001", "Same"))

    def test_symbols_csv_address_clash(self):
        self.assertIn("two addresses", self.refused("FUN_00401000", "Taken"))

    def test_type_without_join(self):
        self.assertIn("already a type", self.refused("Squad_00406000", "Squad"))

    def test_type_with_join(self):
        self.ok("Squad_00406000", "Squad", "--join")
        self.assertIn("Squad", read(self.root, "data/symbols.csv"))

    def test_keep_going_renames_the_rest(self):
        done = self.run_rename("FUN_00401000", "Sine", "FUN_00403000", "FUN_00403001", "--keep-going")
        self.assertEqual(done.returncode, 0, done.stdout)
        self.assertIn("refused 1 of", done.stdout)
        self.assertIn("Sine", read(self.root, "src/b.cpp") + read(self.root, "src/units/unit.h"))
        self.assertIn("FUN_00403000, FUN_00403001", read(self.root, "src/b.cpp"))


class Scope(Fixture):
    def test_every_code_directory_and_bx_pair(self):
        out = self.ok("FUN_00401000", "Sine")
        for name in ("src/a.cpp", "src/units/unit.h", "interfaces/seams.h", "lib/bitexact/bitexact.h",
                     "lib/bitexact/trig.c", "link/data.cpp", "tests/t.cpp", "data/symbols.csv", "docs/notes.md"):
            self.assertIn(name + "\n", out)
            self.assertNotIn("FUN_00401000", read(self.root, name).replace('"FUN_00401000"', ""), name)
        self.assertIn("int bx_Sine(int);", read(self.root, "lib/bitexact/bitexact.h"))
        self.assertIn("Sine@Squad_00406000", read(self.root, "data/symbols.csv").replace("?Sine@", "Sine@"))

    def test_literals_untouched(self):
        self.ok("FUN_00401000", "Sine")
        text = read(self.root, "src/a.cpp")
        self.assertIn('"FUN_00401000"', text)
        self.assertIn("Sine(other)", text)

    def test_data_tables_keep_their_other_columns(self):
        self.ok("field_8", "hp")
        self.assertEqual(read(self.root, "data/state_classes.csv").splitlines(), [
            "name,address,kind,proposed,class,rule,note",
            'hp,0x8,game-region,view,platform,"a, b",hand note',
            "FUN_00401000,0x401000,global,sim,sim,r,keep",
            "other,0x9,global,sim,sim,r,mentions field_8 in a note",
        ])
        self.assertEqual(read(self.root, "data/trace_coverage.csv").splitlines()[1],
                         "game-region,hp,0x8,mixed,yes,none,,,hand evidence")
        self.assertIn("short hp;", read(self.root, "include/ta_types.h"))

    def test_dry_run_writes_nothing(self):
        before = snapshot(self.root)
        out = self.ok("FUN_00401000", "Sine", "--dry-run")
        self.assertEqual(snapshot(self.root), before)
        self.assertIn("would change", out)
        self.assertIn("src/a.cpp\n", out)
        self.assertNotIn("Regenerate", out)

    def test_real_run_lists_the_follow_up(self):
        out = self.ok("FUN_00401000", "Sine")
        self.assertIn("uv run tools/traceschema.py", out)
        self.assertIn("uv run tools/layoutasserts.py --write --build-dir build/modern", out)
        self.assertIn("uv run tools/rwsets.py", out)


class ScopedMember(Fixture):
    def setUp(self):
        super().setUp()
        write(self.root, SCOPED_FILES)

    def test_only_the_named_type_changes(self):
        out = self.ok("Game::field_0", "hp")
        header = read(self.root, "src/game/game.h")
        self.assertIn("        int hp;\n", header)
        self.assertIn("struct Inner { int field_0; }", header)
        self.assertIn("return hp + this->hp;", header)
        self.assertIn('offsetof(Game, hp) == 0x0, "hp"', header)
        self.assertIn("offsetof(Game, field_4)", header)
        self.assertEqual(read(self.root, "src/game/other.h"), "struct Other { int field_0; int field_4; };\n")
        use = read(self.root, "src/game/use.cpp")
        self.assertIn("int Game::Total() { return hp + field_4; }", use)
        self.assertIn("g_game->hp + game->hp + ref.hp;", use)
        self.assertIn("other->field_0 + g_other->field_0 + p->field_0;", use)
        self.assertIn("((Game*)p)->hp + static_cast<Game*>(p)->hp;", use)
        self.assertIn('"g_game->field_0";  // g_game->field_0 in a comment', use)
        self.assertIn("return g_game->field_0; }", use)
        self.assertIn("p->field_0: 1 site(s), first src/game/use.cpp:7", out)
        self.assertNotIn("other->field_0:", out.split("warning")[0])

    def test_data_rows_of_the_type_only(self):
        self.ok("Game::field_0", "hp")
        self.assertEqual(read(self.root, "data/ref_exceptions.csv").splitlines()[1:],
                         ["Game,hp,pointer,src/game/game.h,#1", "Other,field_0,pointer,src/game/other.h,#1"])
        self.assertEqual(read(self.root, "data/state_classes.csv").splitlines()[1:],
                         ["hp,0x0,game-region,,platform,r,Game::hp note", "field_0,0x0,heap,,platform,r,other field_0"])
        self.assertEqual(read(self.root, "data/trace_coverage.csv").splitlines()[1:],
                         ["game-region,hp,0x0,mixed", "heap,field_0,0x0,mixed"])
        self.assertIn("Game.hp and Other.field_0", read(self.root, "data/hazards.csv"))

    def test_from_a_csv(self):
        table = self.root / "pairs.csv"
        table.write_text("old,new,evidence\nGame::field_0,hp,seen\nFUN_00401000,Sine,\n")
        self.ok("--from", str(table))
        self.assertIn("g_game->hp", read(self.root, "src/game/use.cpp"))
        self.assertIn("int Sine(int a);", read(self.root, "src/units/unit.h"))

    def test_dry_run_writes_nothing(self):
        before = snapshot(self.root)
        out = self.ok("Game::field_0", "hp", "--dry-run")
        self.assertEqual(snapshot(self.root), before)
        self.assertIn("would change", out)

    def test_new_is_already_a_member(self):
        self.assertIn("already a member of Game", self.refused("Game::field_0", "field_4"))

    def test_not_a_name(self):
        self.assertIn("is not a name", self.refused("Game::field_0", "class"))
        self.assertIn("is not a name", self.refused("Game::field_0", "1x"))

    def test_member_the_type_lacks(self):
        self.assertIn("does not declare", self.refused("Game::nothing", "hp"))

    def test_type_without_a_definition(self):
        self.assertIn("no struct, class or union definition", self.refused("Missing::field_0", "hp"))


class RealTree(unittest.TestCase):
    def test_traceschema_check_passes_after_a_rename(self):
        """Rename a field the harness hashes in a copy of the tree, regenerate, and check."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for part in ("tools", "include", "src", "interfaces", "data", "lib", "link", "cmake"):
                shutil.copytree(REAL / part, root / part, ignore=shutil.ignore_patterns("__pycache__"))
            for name in ("pyproject.toml", "uv.lock"):
                if (REAL / name).exists():
                    shutil.copy(REAL / name, root / name)
            env = dict(os.environ)

            def run(*a):
                return subprocess.run([sys.executable, *a], cwd=root, env=env, capture_output=True, text=True)

            schema = (root / "src/harness/trace_schema.h").read_text(errors="replace")
            types = (root / "include/ta_types.h").read_text(errors="replace")
            import re
            field = next(f for f in re.findall(r'"(field_[0-9a-f]+)"', schema)
                         if len(re.findall(rf"\b{f}\b", types)) == 1)
            done = run("tools/rename.py", field, "renamedField")
            self.assertEqual(done.returncode, 0, done.stdout + done.stderr)
            self.assertEqual(run("tools/traceschema.py", "--check").returncode, 1)
            done = run("tools/traceschema.py")
            self.assertEqual(done.returncode, 0, done.stdout + done.stderr)
            done = run("tools/traceschema.py", "--check")
            self.assertEqual(done.returncode, 0, done.stdout + done.stderr)


if __name__ == "__main__":
    unittest.main()
