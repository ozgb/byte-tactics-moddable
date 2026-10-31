"""Tests for refcheck.py, on canned types and layouts and on fixture sources compiled with clang-cl.

    uv run tools/test_refcheck.py
"""

import contextlib
import csv
import io
import shutil
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import refcheck as rc

FIXTURE = """typedef void *HANDLE;
typedef char *LPSTR;
typedef int (*Fn)(int);
typedef struct { int a; char *b; HANDLE h; } Anon;
template <class T> struct Ref { unsigned offset; };
template <class T> struct MsvcVector { T *first; T *last; };
template <class T> struct BlockVector { unsigned char padding; Ref<T> first; };
struct Base { virtual void f(); int x; };
struct Derived : Base {
    LPSTR s; Fn fn; int (Derived::*mp)(int);
    struct { int q; void *r; } in;
    union { int u; char *v; };
    Anon an[2];
    const char *const cc[3];
    void (*arr[2])();
    MsvcVector<int> vec;
    BlockVector<int> block;
    Ref<Base> ref;
    int handle;
    int plain;
};
struct Plain { int a; void *vtable; };
struct Holds { Base b; int y; };
struct Item { int a; char *p; };
struct Game { int x; char *view; Item items[3]; int tail; };
struct SimGlobals { int a; char *list; char *table; };
void use(Derived *d, Holds *h, Game *g, SimGlobals *s, Plain *p) { (void)d; (void)h; (void)g; (void)s;
    char *c = (char *)d->handle; (void)c; }
"""

LAYOUT = """*** Dumping AST Record Layout
         0 | struct Derived
         0 |   struct Base (primary base)
         0 |     (Base vftable pointer)
         4 |     int x
         8 |   LPSTR s
        12 |   union Derived::(anonymous at a.cpp:6:97)\x20
        12 |     int u
        12 |     char * v
        16 |   struct Derived::(unnamed at a.cpp:6:66) in
        16 |     int q
        20 |     void * r
           | [sizeof=24, align=4,
           |  nvsize=24, nvalign=4]

*** Dumping AST Record Layout
         0 | struct Holds
         0 |   struct Base b
         0 |     (Base vftable pointer)
         4 |     int x
         8 |   int y
           | [sizeof=12, align=4,
           |  nvsize=12, nvalign=4]
"""


def scan_text(root: Path, text: str) -> rc.UnitScan:
    (root / "src").mkdir(parents=True, exist_ok=True)
    path = root / "src/a.cpp"
    path.write_text(text)
    entry = {"command": "clang-cl --target=i686-pc-windows-msvc /nologo -TP -fms-compatibility -fms-extensions "
                        "-c -- %s" % path, "directory": str(root), "file": str(path)}
    with mock.patch.object(rc, "ROOT", root):
        return rc.scan_unit((entry, True))


class FieldKindTest(unittest.TestCase):
    def test_kinds(self):
        cases = [("int", None, None), ("char *", None, "pointer"), ("LPSTR", "char *", "pointer"),
                 ("char *[4]", None, "pointer"), ("const char *const[3]", None, "pointer"),
                 ("int (*)(int)", None, "function-pointer"), ("Fn", "int (*)(int)", "function-pointer"),
                 ("void (*[2])()", None, "function-pointer"),
                 ("int (Derived::*)(int) __attribute__((thiscall))", None, "function-pointer"),
                 ("std::vector<int *>", None, "container"), ("BlockVector<int>", None, None),
                 ("MsvcVector<Unit *>", None, "container"), ("SyncTaggedVector<Order>", None, "container"),
                 ("Ref<Unit>", None, "ref"), ("Ref<Unit>[4]", None, "ref"), ("Ref<Unit> *", None, "pointer"),
                 ("Unit", None, None), ("Point16[4]", None, None), ("Foo &", None, "pointer"),
                 ("int (*)[4]", None, "pointer")]
        for qual, desugared, want in cases:
            with self.subTest(qual=qual):
                self.assertEqual(rc.field_kind(qual, desugared), want)


class LayoutTest(unittest.TestCase):
    def test_flatten_owners_and_slots(self):
        by_name, _ = rc.parse_layouts(LAYOUT)
        leaves, slots = rc.flatten(by_name["Derived"], "Derived")
        found = {(l.offset, l.owner, l.path) for l in leaves}
        self.assertIn((4, "Base", "x"), found)
        self.assertIn((12, "Derived", "u"), found)
        self.assertIn((12, "Derived", "v"), found)
        self.assertIn((20, "Derived", "in.r"), found)
        self.assertEqual(slots, [0])

    def test_member_vftable_is_not_an_own_slot(self):
        by_name, _ = rc.parse_layouts(LAYOUT)
        self.assertEqual(rc.flatten(by_name["Holds"], "Holds")[1], [])

    def test_unnamed_records_are_kept_by_location(self):
        text = "*** Dumping AST Record Layout\n         0 | struct (unnamed at sub/a.cpp:4:9)\n         0 |   int a\n" \
               "           | [sizeof=4, align=4,\n           |  nvsize=4, nvalign=4]\n"
        _, by_loc = rc.parse_layouts(text, "/d")
        self.assertEqual(list(by_loc), [("/d/sub/a.cpp", 4)])
        self.assertEqual(by_loc[("/d/sub/a.cpp", 4)].size, 4)


class ClangTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if shutil.which("clang-cl") is None:
            raise unittest.SkipTest("clang-cl is not installed")
        cls.tmp = tempfile.TemporaryDirectory()
        cls.root = Path(cls.tmp.name)
        cls.scan = scan_text(cls.root, FIXTURE)
        cls.records = {r.name: r for r in cls.scan.records}

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def kinds(self, name):
        return {m.path: m.kind for m in self.records[name].members}

    def test_typedefs_are_resolved(self):
        kinds = self.kinds("Derived")
        self.assertEqual(kinds["s"], "pointer")
        self.assertEqual(kinds["fn"], "function-pointer")
        self.assertEqual(kinds["mp"], "function-pointer")
        self.assertEqual(kinds["arr"], "function-pointer")
        self.assertEqual(kinds["cc"], "pointer")
        self.assertEqual(self.kinds("Anon")["h"], "pointer")
        self.assertEqual(self.kinds("Anon")["b"], "pointer")

    def test_nested_and_anonymous_members(self):
        kinds = self.kinds("Derived")
        self.assertEqual(kinds["in.r"], "pointer")
        self.assertIsNone(kinds["in.q"])
        self.assertEqual(kinds["v"], "pointer")
        self.assertIsNone(kinds["u"])

    def test_containers_and_refs(self):
        kinds = self.kinds("Derived")
        self.assertEqual(kinds["vec"], "container")
        self.assertIsNone(kinds["block"])
        self.assertEqual(kinds["ref"], "ref")

    def test_polymorphic_and_vptr(self):
        self.assertTrue(self.records["Base"].polymorphic)
        self.assertTrue(self.records["Derived"].polymorphic)
        self.assertFalse(self.records["Plain"].polymorphic)
        self.assertEqual(self.kinds("Plain")["vtable"], "pointer")
        self.assertNotIn("vfptr", self.kinds("Base"))
        by_name = self.scan.by_name
        self.assertEqual(rc.flatten(by_name["Base"], "Base")[1], [0])
        self.assertEqual(rc.flatten(by_name["Plain"], "Plain")[1], [])
        self.assertEqual(rc.flatten(by_name["Holds"], "Holds")[1], [])

    def test_vptr_problems(self):
        base = rc.Def("Base", "src/a.cpp", self.records["Base"], self.scan.by_name["Base"], "")
        plain = rc.Def("Plain", "src/a.cpp", self.records["Plain"], self.scan.by_name["Plain"], "")
        self.assertEqual(rc.vptr_problems([base, plain]), ([], 1))
        lying = rc.Record("Base", "src/a.cpp", 1, False, [], {})
        bad = rc.Def("Base", "src/a.cpp", lying, self.scan.by_name["Base"], "")
        problems, _ = rc.vptr_problems([bad])
        self.assertIn("has a vftable slot at +0x0 but is not polymorphic", problems[0])
        flat = rc.Def("Plain", "src/a.cpp", rc.Record("Plain", "src/a.cpp", 1, True, [], {}),
                      self.scan.by_name["Plain"], "")
        self.assertIn("polymorphic but its layout has no vftable slot", rc.vptr_problems([flat])[0][0])

    def test_integer_cast_to_pointer(self):
        self.assertEqual(self.scan.casts, {("Derived", "src/a.cpp", "handle")})
        d = rc.Def("Derived", "src/a.cpp", self.records["Derived"], self.scan.by_name["Derived"], "")
        found = rc.raw_fields([d], self.scan.casts)
        self.assertIn(("Derived", "handle", "int-pointer"), found)
        self.assertNotIn(("Derived", "plain", "int-pointer"), found)
        self.assertEqual(found[("Derived", "handle", "int-pointer")].files, {"src/a.cpp"})

    def test_locator_maps_offsets_through_arrays(self):
        locator = rc.Locator(self.scan)
        self.assertEqual(locator.find("Game", 4), ("Game", "view", "pointer", "view"))
        self.assertEqual(locator.find("Game", 12), ("Item", "p", "pointer", "items"))
        self.assertEqual(locator.find("Game", 28), ("Item", "p", "pointer", "items"))
        self.assertIsNone(locator.find("Game", 0))
        self.assertIsNone(locator.find("Game", 8))
        self.assertIsNone(locator.find("Game", 32))

    def test_locator_ignores_offsets_inside_a_field(self):
        locator = rc.Locator(self.scan)
        for offset in (5, 6, 7, 13, 14, 15, 29, 30, 31):
            self.assertIsNone(locator.find("Game", offset))

    def test_straddling_token_adds_no_row(self):
        out = self.outside_notes([("0x6 0xe+0x8*k", "Game.view")])
        self.assertEqual(out, {})
        out = self.outside_notes([("0x4 0xc+0x8*k", "Game.view")])
        self.assertEqual(sorted(out), [("p", "0xc+0x8*k"), ("view", "0x4")])

    def test_outside_rows(self):
        data = self.root / "data"
        data.mkdir(exist_ok=True)
        header = "holder,offsets,target,scenarios,ticks,target_offsets,class,note\n"
        rows = ['CreateGameObject,0x4 0xc+0x8*k 0x0,T,1,1,-,outside,"Game.view"',
                'CreateGameObject,0x14,T,1,1,-,not-a-pointer,skip',
                '"LoadUnitInfo: UNITINFO",0x47a+0x249*k 0x191c,T,1,1,-,outside,"UnitType.data"',
                'SimGlobals.list,0x0,T,1,1,-,outside,"LOS table vector"',
                'SimGlobals.table,0x0,T,1,1,-,outside,"damage table vector"']
        (data / "block_pointers.csv").write_text(header + "\n".join(rows) + "\n")
        locators = {"Game": rc.Locator(self.scan), "SimGlobals": rc.Locator(self.scan)}
        classes = {"view": "view", "items": "sim"}
        out = rc.outside_rows(self.root, locators, classes)
        keyed = {(r["struct"], r["field"], r["holder"]): r for r in out}
        self.assertEqual(keyed[("Game", "view", "CreateGameObject")]["issue"], "#392")
        self.assertEqual(keyed[("Item", "p", "CreateGameObject")]["issue"], "")
        self.assertEqual(keyed[("", "", "UNITINFO")]["words"], 2)
        self.assertEqual(keyed[("", "", "UNITINFO")]["offset"], "0x47a+0x249*k")
        self.assertEqual(keyed[("SimGlobals", "list", "SimGlobals.list")]["issue"], "")
        self.assertEqual(keyed[("SimGlobals", "table", "SimGlobals.table")]["issue"], "#596")
        self.assertEqual(len(out), 5)

    def test_progression_hits_two_fields(self):
        out = self.outside_notes([("0x4+0x8*k", "Game.view")])
        self.assertEqual(sorted(out), [("p", "0xc"), ("view", "0x4+0x8*k")])

    def test_progression_does_not_double_count_a_field_with_its_own_token(self):
        out = self.outside_notes([("0x4+0x8*k 0x14", "Game.view")])
        self.assertEqual(sorted(out), [("p", "0x14"), ("view", "0x4+0x8*k")])

    def test_progression_stops_at_the_first_offset_with_no_raw_field(self):
        out = self.outside_notes([("0x4+0x4*k", "Game.view")])
        self.assertEqual(sorted(out), [("view", "0x4+0x4*k")])

    def test_straddling_progression_adds_no_row(self):
        self.assertEqual(self.outside_notes([("0x6+0x8*k", "Game.view")]), {})

    def outside_notes(self, rows, hand=()):
        data = self.root / "data"
        data.mkdir(exist_ok=True)
        header = "holder,offsets,target,scenarios,ticks,target_offsets,class,note\n"
        (data / "block_pointers.csv").write_text(header + "".join(
            'CreateGameObject,%s,T,1,1,-,outside,"%s"\n' % row for row in rows))
        old = [{"struct": "Game", "field": "view", "holder": "CreateGameObject", "offset": offset, "words": 1,
                "issue": "#392", "kind": "pointer", "note": note} for offset, note in hand]
        (data / "outside_pointers.csv").write_text(rc.render(rc.OUTSIDE_COLUMNS, old))
        out = rc.outside_rows(self.root, {"Game": rc.Locator(self.scan)}, {"view": "view", "items": "sim"})
        return {(r["field"], r["offset"]): r["note"] for r in out}

    def test_note_naming_another_offset_is_not_copied(self):
        notes = self.outside_notes([("0x4 0xc", "Game.items (0xc): the list")])
        self.assertEqual(notes[("view", "0x4")], "")
        self.assertEqual(notes[("p", "0xc")], "Game.items (0xc): the list")

    def test_note_naming_the_row_offset_is_copied(self):
        notes = self.outside_notes([("0x4 0xc", "Game.view (0x4)")])
        self.assertEqual(notes[("view", "0x4")], "Game.view (0x4)")

    def test_note_naming_no_offset_is_copied(self):
        notes = self.outside_notes([("0x4 0xc", "Game.view")])
        self.assertEqual(notes[("view", "0x4")], "Game.view")
        self.assertEqual(notes[("p", "0xc")], "Game.view")

    def test_hand_note_is_kept_when_the_block_note_is_not_copied(self):
        notes = self.outside_notes([("0x4 0xc", "Game.items (0xc): the list")], hand=[("0x4", "the view surface")])
        self.assertEqual(notes[("view", "0x4")], "the view surface")

    def test_holder_only_row_takes_a_note_naming_any_of_its_offsets(self):
        data = self.root / "data"
        data.mkdir(exist_ok=True)
        header = "holder,offsets,target,scenarios,ticks,target_offsets,class,note\n"
        (data / "block_pointers.csv").write_text(
            header + 'H,0x4+0x8*k 0x20,T,1,1,-,outside,"X (0x20)"\nG,0x4,T,1,1,-,outside,"X (0x99)"\n')
        (data / "outside_pointers.csv").write_text(rc.render(rc.OUTSIDE_COLUMNS, []))
        out = rc.outside_rows(self.root, {}, {})
        self.assertEqual({r["holder"]: r["note"] for r in out}, {"H": "X (0x20)", "G": ""})

    def hand_rows(self, rows):
        data = self.root / "data"
        data.mkdir(exist_ok=True)
        header = "holder,offsets,target,scenarios,ticks,target_offsets,class,note\n"
        (data / "block_pointers.csv").write_text(header + 'CreateGameObject,0x4 0xc,T,1,1,-,outside,"Game.view"\n')
        (data / "outside_pointers.csv").write_text(rc.render(rc.OUTSIDE_COLUMNS, rows))
        return rc.outside_rows(self.root, {"Game": rc.Locator(self.scan)}, {"view": "view", "items": "sim"})

    def hand_row(self, struct, field, kind, holder="CreateGameObject", offset="0x4"):
        return {"struct": struct, "field": field, "holder": holder, "offset": offset, "words": 1, "issue": "",
                "kind": kind, "note": "hand"}

    def test_load_kind_is_kept_over_the_scan_and_has_no_issue(self):
        out = self.hand_rows([self.hand_row("Game", "view", "load")])
        keyed = {r["field"]: r for r in out}
        self.assertEqual((keyed["view"]["kind"], keyed["view"]["issue"]), ("load", ""))
        self.assertEqual((keyed["p"]["kind"], keyed["p"]["issue"]), ("pointer", ""))

    def test_hand_only_code_row_survives_without_a_scan_row(self):
        row = self.hand_row("WeaponDef", "fire", "code", holder="hand", offset="+0x60")
        out = self.hand_rows([row])
        kept = [r for r in out if r["field"] == "fire"]
        self.assertEqual([(r["kind"], r["issue"], r["note"]) for r in kept], [("code", "", "hand")])

    def test_outside_rows_fail_when_the_game_holder_is_gone(self):
        data = self.root / "data"
        data.mkdir(exist_ok=True)
        header = "holder,offsets,target,scenarios,ticks,target_offsets,class,note\n"
        (data / "block_pointers.csv").write_text(
            header + 'CreateGame,0x4,T,1,1,-,outside,"Game.view"\nCreateGameObject,0x14,T,1,1,-,not-a-pointer,skip\n')
        with self.assertRaises(SystemExit) as cm:
            rc.outside_rows(self.root, {"Game": rc.Locator(self.scan)}, {})
        self.assertIn("no outside rows for the Game holder CreateGameObject", str(cm.exception))
        self.assertEqual([r["holder"] for r in rc.outside_rows(self.root, {}, {})], ["CreateGame"])


def make_result(found, outside=()):
    return rc.Result(set(), [], found, list(outside), [], 0, [], {}, {})


def found_field(struct, name, kind="pointer", files=("src/a.h",)):
    return {(struct, name, kind): rc.Found(struct, name, kind, set(files))}


class CheckTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / "data").mkdir()

    def tearDown(self):
        self.tmp.cleanup()

    def put(self, name, columns, rows):
        (self.root / name).write_text(rc.render(columns, rows))

    def exceptions(self, *rows):
        self.put(rc.EXCEPTIONS, rc.EXCEPTION_COLUMNS, [dict(zip(rc.EXCEPTION_COLUMNS, r)) for r in rows])

    def outside(self, rows=()):
        self.put(rc.OUTSIDE, rc.OUTSIDE_COLUMNS, rows)

    def test_listed_field_passes(self):
        self.exceptions(("Unit", "def", "pointer", "src/a.h", "#349"))
        self.outside()
        self.assertEqual(rc.check(self.root, make_result(found_field("Unit", "def"))), [])

    def test_unlisted_field_fails(self):
        self.exceptions()
        self.outside()
        problems = rc.check(self.root, make_result(found_field("Unit", "def")))
        self.assertEqual(len(problems), 1)
        self.assertIn("src/a.h: Unit.def is a raw pointer in a block-held struct and is in neither", problems[0])

    def test_stale_rows_fail(self):
        self.exceptions(("Unit", "gone", "pointer", "src/a.h", "#349"),
                        ("Unit", "def", "container", "src/a.h", "#349"))
        self.outside()
        problems = rc.check(self.root, make_result(found_field("Unit", "def")))
        self.assertTrue(any("stale row Unit.gone" in p for p in problems))
        self.assertTrue(any("is no longer a raw container" in p or "stale row Unit.def" in p for p in problems))

    def test_stale_file_fails(self):
        self.exceptions(("Unit", "def", "pointer", "src/old.h", "#349"))
        self.outside()
        problems = rc.check(self.root, make_result(found_field("Unit", "def")))
        self.assertIn("file is src/old.h, expected src/a.h", problems[0])

    def test_outside_field_needs_no_exception_and_blocks_a_row(self):
        row = {"struct": "Game", "field": "view", "holder": "H", "offset": "0x4", "words": 1, "issue": "#392",
               "kind": "pointer", "note": "n"}
        self.outside([row])
        self.exceptions()
        result = make_result(found_field("Game", "view", files=("src/game/game.h",)), [row])
        self.assertEqual(rc.check(self.root, result), [])
        self.exceptions(("Game", "view", "pointer", "src/game/game.h", "#392"))
        self.assertIn("the field is on", rc.check(self.root, result)[0])

    def test_outside_list_out_of_date_fails(self):
        self.exceptions()
        self.outside()
        row = {"struct": "", "field": "", "holder": "H", "offset": "0x4", "words": 1, "issue": "#353", "kind": "",
               "note": "n"}
        problems = rc.check(self.root, make_result({}, [row]))
        self.assertEqual(len(problems), 1)
        self.assertIn("data/outside_pointers.csv: out of date", problems[0])

    def test_empty_note_fails(self):
        self.exceptions()
        row = {"struct": "Game", "field": "sounds", "holder": "H", "offset": "0x4", "words": 1, "issue": "#392",
               "kind": "pointer", "note": ""}
        self.outside([row])
        problems = rc.check(self.root, make_result({}, [row]))
        self.assertEqual(len(problems), 1)
        self.assertIn("Game.sounds H 0x4 has an empty note", problems[0])

    def test_load_row_passes_without_an_issue(self):
        self.exceptions()
        row = {"struct": "Game", "field": "view", "holder": "H", "offset": "0x4", "words": 1, "issue": "",
               "kind": "load", "note": "n"}
        self.outside([row])
        self.assertEqual(rc.check(self.root, make_result(found_field("Game", "view"), [row])), [])

    def test_load_row_with_an_issue_fails(self):
        self.exceptions()
        row = {"struct": "Game", "field": "view", "holder": "H", "offset": "0x4", "words": 1, "issue": "#353",
               "kind": "load", "note": "n"}
        self.outside([row])
        self.assertIn("names none", rc.check(self.root, make_result({}, [row]))[0])

    def test_blank_kind_without_an_issue_fails(self):
        self.exceptions()
        row = {"struct": "Game", "field": "view", "holder": "H", "offset": "0x4", "words": 1, "issue": "",
               "kind": "pointer", "note": "n"}
        self.outside([row])
        self.assertIn("expected #<number>", rc.check(self.root, make_result({}, [row]))[0])

    def test_load_field_drops_its_exception_row(self):
        self.exceptions(("Game", "view", "pointer", "src/a.h", "#353"))
        row = {"struct": "Game", "field": "view", "holder": "H", "offset": "0x4", "words": 1, "issue": "",
               "kind": "load", "note": "n"}
        result = make_result(found_field("Game", "view"), [row])
        rc.write(self.root, result, init=False)
        self.assertEqual(rc.read_csv(self.root / rc.EXCEPTIONS), [])
        self.assertEqual(rc.check(self.root, result), [])

    def test_bad_issue_fails(self):
        self.exceptions(("Unit", "def", "pointer", "src/a.h", "later"))
        self.outside()
        self.assertIn("is not #<number>", rc.check(self.root, make_result(found_field("Unit", "def")))[0])

    def test_closed_issue_fails(self):
        self.exceptions(("Unit", "def", "pointer", "src/a.h", "#349"))
        self.outside()
        with mock.patch.object(rc, "gh_issue_state", return_value="CLOSED"):
            problems = rc.check(self.root, make_result(found_field("Unit", "def")), open_issues=True)
        self.assertIn("issue #349 is CLOSED, not open", problems[0])

    def test_write_prunes_and_never_adds(self):
        self.exceptions(("Unit", "def", "pointer", "src/old.h", "#349"), ("Unit", "gone", "pointer", "src/a.h", "#349"))
        found = {**found_field("Unit", "def"), **found_field("Unit", "fresh")}
        rc.write(self.root, make_result(found), init=False)
        rows = rc.read_csv(self.root / rc.EXCEPTIONS)
        self.assertEqual([(r["field"], r["file"], r["issue"]) for r in rows], [("def", "src/a.h", "#349")])
        rc.write(self.root, make_result(found), init=True)
        self.assertEqual([r["field"] for r in rc.read_csv(self.root / rc.EXCEPTIONS)], ["def", "fresh"])

    def test_main_check_exit_status(self):
        self.exceptions()
        self.outside()
        out = io.StringIO()
        with mock.patch.object(rc, "analyse", return_value=make_result(found_field("Unit", "def"))), \
                contextlib.redirect_stdout(out):
            self.assertEqual(rc.main(["--root", str(self.root), "--check"]), 1)


class IssueTest(unittest.TestCase):
    TABLE = {"src/units/unit.cpp": rc.Counter({"units": 3}), "src/network/packets.cpp": rc.Counter({"network": 2}),
             "src/map/a.cpp": rc.Counter({"map": 1, "los_radar": 2}), "src/gui/x.cpp": rc.Counter({"presentation": 1})}
    CLASSES = {"units": "sim", "tab": "mixed", "view": "view", "gaf": "platform"}

    def issue(self, struct, field, file):
        return rc.exception_issue(rc.Found(struct, field, "pointer", {file}), self.TABLE, self.CLASSES)

    def test_by_subsystem(self):
        self.assertEqual(self.issue("Thing", "p", "src/units/unit.h"), "#353")
        self.assertEqual(self.issue("Packet", "p", "src/network/packets.cpp"), "#353")
        self.assertEqual(self.issue("Cell", "p", "src/map/a.h"), "#353")
        self.assertEqual(self.issue("Panel", "p", "src/gui/x.h"), "#353")
        self.assertEqual(self.issue("Thing", "p", "src/elsewhere/y.h"), "#353")

    def test_named_structs_and_game_regions(self):
        self.assertEqual(self.issue("Mission", "p", "src/map/mission.h"), "#353")
        self.assertEqual(self.issue("Game", "units", "src/game/game.h"), "#353")
        self.assertEqual(self.issue("Game", "tab", "src/game/game.h"), "#353")
        self.assertEqual(self.issue("Game", "view", "src/game/game.h"), "#353")
        self.assertEqual(self.issue("Game", "unknown", "src/game/game.h"), "#353")

    def test_container_exception_takes_the_container_issue(self):
        found = rc.Found("Thing", "v", "container", {"src/units/unit.h"})
        self.assertEqual(rc.exception_issue(found, self.TABLE, self.CLASSES), "#596")

    def test_outside_issue(self):
        self.assertEqual(rc.outside_issue("", "Game", "view", self.CLASSES), "#392")
        self.assertEqual(rc.outside_issue("", "Game", "gaf", self.CLASSES), "#392")
        self.assertEqual(rc.outside_issue("", "Game", "units", self.CLASSES), "")
        self.assertEqual(rc.outside_issue("FeatureAnim.state (0x4)", "", "", self.CLASSES), "#392")
        self.assertEqual(rc.outside_issue("UnitType.data", "", "", self.CLASSES), "")


class OffsetTest(unittest.TestCase):
    def test_expand(self):
        self.assertEqual(rc.expand_offsets("0x4+0x30*k 0x10 junk"), [("0x4+0x30*k", 4), ("0x10", 16)])


if __name__ == "__main__":
    unittest.main()
