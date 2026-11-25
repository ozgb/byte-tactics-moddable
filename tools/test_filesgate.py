"""Tests of tools/filesgate.py and tools/filesfixtures.py.

    uv run tools/test_filesgate.py              # the tool tests, and the in-repo adapter's when the adapter (build/modern/files_adapter.exe or $BT_FILES_ADAPTER) and wine exist

The adapter tests are the gate's CI checks: the in-repo adapter is equal on every group, and each injected fault
(BT_FILES_FAULT) makes its group differ and names the vector.
"""

from __future__ import annotations

import json
import os
import shutil
import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import filesfixtures
import filesgate

ADAPTER = filesgate.ADAPTER
REPLAYER = """
import json, os, sys
fixture, calls_path, out_path = os.path.basename(sys.argv[3]), sys.argv[4], sys.argv[5]
calls = [l.rstrip("\\n").split("\\t") for l in open(calls_path, newline="")]
for g in %(groups)r:
    for v in json.load(open(%(dir)r + "/" + g + ".json"))["vectors"]:
        if v["calls"] == calls and v["fixture"] == fixture:
            res = v["results"]
            %(fault)s
            with open(out_path, "w") as f:
                for op, value in res:
                    f.write(op + "\\t" + json.dumps(value) + "\\n")
            sys.exit(0)
sys.exit(3)
"""


def expand(src: bytes) -> bytes:
    """The method 1 expander, as the format states it."""
    win = bytearray(0x1011)
    pos, out, i = 1, bytearray(), 1
    flags, mask = src[0], 1
    while True:
        if not flags & mask:
            c = src[i]
            i += 1
            out.append(c)
            win[pos] = c
            pos = (pos + 1) & 0xFFF
        else:
            w = src[i] | src[i + 1] << 8
            i += 2
            if w >> 4 == 0:
                return bytes(out)
            for k in range((w & 15) + 2):
                c = win[((w >> 4) + k) & 0xFFF]
                out.append(c)
                win[pos] = c
                pos = (pos + 1) & 0xFFF
        mask <<= 1
        if mask & 0x100:
            mask, flags = 1, src[i]
            i += 1


def c_string(data: bytes, at: int) -> str:
    return data[at:data.index(b"\0", at)].decode("latin-1")


def read_cob(data: bytes) -> dict:
    """The COB reading of docs/files-gate.md, from the file's bytes."""
    w = struct.unpack_from("<11i", data, 0)
    word = lambda at: struct.unpack_from("<i", data, at)[0]
    return {"version": w[0],
            "scripts": [[c_string(data, word(w[7] + 4 * i)), word(w[6] + 4 * i)] for i in range(w[1])],
            "pieces": [c_string(data, word(w[8] + 4 * i)) for i in range(w[2])],
            "statics": w[4],
            "pairs": [[word(w[10] + 8 * i), word(w[10] + 8 * i + 4)] for i in range(w[5])],
            "code": [word(w[9] + 4 * i) for i in range(w[3])]}


def gaf_pixels(data: bytes, at: int, width: int, height: int) -> list:
    """A compressed frame's rows, decoded as docs/files-gate.md ("GAF") states."""
    rows = []
    for _ in range(height):
        (size,) = struct.unpack_from("<H", data, at)
        row, i = [None] * width, at + 2
        x = 0
        while size and x < width:
            b = data[i]
            i += 1
            if b & 1:
                x += b >> 1
            elif b & 2:
                for _ in range((b >> 2) + 1):
                    if x < width:
                        row[x] = data[i]
                    x += 1
                i += 1
            else:
                for k in range((b >> 2) + 1):
                    if x < width:
                        row[x] = data[i + k]
                    x += 1
                i += (b >> 2) + 1
        rows.append(row)
        at += 2 + size
    return rows


def read_gaf_frame(data: bytes, at: int, duration: int | None) -> dict:
    w, h, x, y, transparent, compressed, layers, blend, _, ptr, _ = struct.unpack_from("<HHhhBBBBIII", data, at)
    out = {"width": w, "height": h, "x": x, "y": y, "transparent": transparent, "compressed": compressed,
           "blend": blend}
    if duration is not None:
        out["duration"] = duration
    if layers:
        out["frames"] = [read_gaf_frame(data, struct.unpack_from("<I", data, ptr + 4 * k)[0], None)
                         for k in range(layers)]
    elif compressed:
        out["pixels"] = gaf_pixels(data, ptr, w, h)
    else:
        out["pixels"] = [list(data[ptr + r * w:ptr + (r + 1) * w]) for r in range(h)]
    return out


def read_gaf(data: bytes) -> list:
    """The GAF reading of docs/files-gate.md, from the file's bytes."""
    (count,) = struct.unpack_from("<h", data, 4)
    entries = []
    for i in range(count):
        (at,) = struct.unpack_from("<I", data, 12 + 4 * i)
        (frames,) = struct.unpack_from("<H", data, at)
        entries.append({"name": c_string(data, at + 8), "loop": data[at + 2], "frames": [
            read_gaf_frame(data, struct.unpack_from("<I", data, at + 40 + 8 * j)[0],
                           struct.unpack_from("<H", data, at + 44 + 8 * j)[0]) for j in range(frames)]})
    return entries


def read_3do(data: bytes, at: int = 0) -> list:
    """The 3DO reading of docs/files-gate.md, from the file's bytes: the pieces from `at` and its siblings."""
    out = []
    while True:
        _, nv, nprim, selection, x, y, z, name, _, verts, prims, sibling, child = struct.unpack_from("<13i", data, at)
        vertices = [list(struct.unpack_from("<3i", data, verts + 12 * i)) for i in range(nv)]
        primitives = []
        for i in range(nprim):
            colour, n, _, idx, tex = struct.unpack_from("<5i", data, prims + 32 * i)
            primitives.append({"colour": colour, "vertices": list(struct.unpack_from("<%dH" % n, data, idx)),
                               "texture": c_string(data, tex) if tex else None})
        if selection != -1 and primitives:
            primitives[0], primitives[selection] = primitives[selection], primitives[0]
            selection = 0

        def height(p):
            total = sum(vertices[v][1] for v in p["vertices"])
            return abs(total) // len(p["vertices"]) * (1 if total >= 0 else -1)

        swapped = True
        while swapped:
            swapped = False
            for i in range(1, len(primitives) - 1):
                if height(primitives[i]) > height(primitives[i + 1]):
                    primitives[i], primitives[i + 1] = primitives[i + 1], primitives[i]
                    swapped = True
        out.append({"name": c_string(data, name) if name else None, "offset": [x, y, z], "vertices": vertices,
                    "selection": selection, "primitives": primitives,
                    "children": read_3do(data, child) if child else []})
        if not sibling:
            return out
        at = sibling


def loose(top: Path, name: str) -> bytes:
    """The bytes of the loose file `name` names under `top`, its case folded; empty when there is none."""
    at = top
    for part in name.split("\\"):
        found = [p for p in at.iterdir() if p.name.lower() == part.lower()] if at.is_dir() else []
        if not found:
            return b""
        at = found[0]
    return at.read_bytes() if at.is_file() else b""


READERS = {"cob": read_cob, "gaf": read_gaf, "3do": read_3do}


def replayer(tmp: Path, fault: str = "pass") -> list[str]:
    script = tmp / "replayer.py"
    script.write_text(REPLAYER % {"groups": filesgate.GROUPS, "dir": str(filesgate.VECTORS), "fault": fault})
    return [sys.executable, str(script)]


class Fixtures(unittest.TestCase):
    def test_the_encoders_round_trip(self):
        for n, seed in ((1, 1), (100, 3), (65536, 4), (5000, 9)):
            data = filesfixtures.pattern(n, seed)
            self.assertEqual(zlib.decompress(filesfixtures.deflate(data)), data)
            self.assertEqual(expand(filesfixtures.lz77(data)), data)
        runs = b"a" * 4000 + b"b" * 5000
        self.assertEqual(expand(filesfixtures.lz77(runs)), runs)

    def test_a_build_matches_the_stored_digests(self):
        stored = json.loads((filesgate.VECTORS / "fixtures.json").read_text())["fixtures"]
        with tempfile.TemporaryDirectory() as tmp:
            filesfixtures.build(Path(tmp))
            self.assertEqual(filesfixtures.check(Path(tmp), stored), [])
            (Path(tmp) / "base" / "readme.txt").write_bytes(b"changed")
            self.assertEqual(filesfixtures.check(Path(tmp), stored), ["differs: base/readme.txt"])
            (Path(tmp) / "base" / "readme.txt").unlink()
            (Path(tmp) / "base" / "stray.txt").write_bytes(b"x")
            self.assertEqual(filesfixtures.check(Path(tmp), stored),
                             ["extra: base/stray.txt", "missing: base/readme.txt"])
            with self.assertRaises(SystemExit):
                filesfixtures.build(Path(tmp))

    def test_the_key_byte_255_derives_no_key(self):
        self.assertEqual(filesfixtures.derived_key(0xFF), 0)
        self.assertEqual(filesfixtures.derived_key(0), 0)
        self.assertEqual(filesfixtures.derived_key(0x31), ~((0x31 << 2) | (0x31 >> 6)) & 0xFF)

    def test_an_archive_ends_with_the_footer(self):
        data = filesfixtures.archive([filesfixtures.File("a.txt", b"x")], key=7)
        self.assertTrue(data.startswith(b"HAPI\x00\x00\x01\x00"))
        self.assertTrue(data.endswith(filesfixtures.FOOTER))


class Vectors(unittest.TestCase):
    def test_every_case_is_stored_with_its_calls(self):
        for g in filesgate.GROUPS:
            stored = json.loads((filesgate.VECTORS / (g + ".json")).read_text())
            self.assertEqual([(v["name"], v["fixture"], v["calls"]) for v in stored["vectors"]],
                             [(n, f, c) for n, f, c in filesgate.CASES[g]], g)
            for v in stored["vectors"]:
                self.assertLessEqual(len(v["results"]), len(v["calls"]), v["name"])

    def test_no_vector_stores_an_error(self):
        for g in filesgate.GROUPS:
            for v in json.loads((filesgate.VECTORS / (g + ".json")).read_text())["vectors"]:
                self.assertNotIn("error", [r[1] for r in v["results"]], v["name"])

    def test_compare_names_the_first_line(self):
        calls = [["a"], ["b"], ["c"]]
        want = [["a", 1], ["b", 2], ["c", 3]]
        self.assertEqual(filesgate.compare(calls, want, want), {"status": "equal"})
        self.assertEqual(filesgate.compare(calls, want, [["a", 1], ["b", 5], ["c", 4]]),
                         {"status": "differs", "line": 2, "call": "b", "value": ["b", 5]})
        self.assertEqual(filesgate.compare(calls, want, want[:1])["value"], "missing")

    def test_types_are_compared_strictly(self):
        self.assertFalse(filesgate.same(True, 1))
        self.assertFalse(filesgate.same(1.0, 1))
        self.assertFalse(filesgate.same([1, [True]], [1, [1]]))
        self.assertFalse(filesgate.same({"a": 1}, {"a": 1.0}))
        self.assertTrue(filesgate.same([1, {"a": [None, "x"]}], [1, {"a": [None, "x"]}]))
        calls = [["match", "a", "*"]]
        self.assertEqual(filesgate.compare(calls, [["match", 1]], [["match", True]])["status"], "differs")

    def test_fixtures_that_do_not_match_stop_the_gate_with_exit_2(self):
        with tempfile.TemporaryDirectory() as tmp:
            stored = Path(tmp) / "vectors"
            shutil.copytree(filesgate.VECTORS, stored)
            body = json.loads((stored / "fixtures.json").read_text())
            body["fixtures"]["base"]["readme.txt"]["sha256"] = "0" * 64
            (stored / "fixtures.json").write_text(json.dumps(body))
            old, filesgate.VECTORS = filesgate.VECTORS, stored
            try:
                with self.assertRaises(filesgate.CouldNotRun):
                    filesgate.judge(replayer(Path(tmp)), ("formats",))
            finally:
                filesgate.VECTORS = old

    def test_a_line_without_json_is_malformed(self):
        with self.assertRaises(ValueError):
            filesgate.parse_out("mount\n")
        self.assertEqual(filesgate.parse_out('read\t[1, "ab"]\n'), [["read", [1, "ab"]]])


class Readings(unittest.TestCase):
    def test_the_doc_readings_give_the_stored_vectors(self):
        with tempfile.TemporaryDirectory() as tmp:
            filesfixtures.build(Path(tmp))
            for group, read in READERS.items():
                for v in json.loads((filesgate.VECTORS / (group + ".json")).read_text())["vectors"]:
                    for call, (op, value) in zip(v["calls"], v["results"]):
                        if op != group:
                            continue
                        data = loose(Path(tmp) / v["fixture"], call[1])
                        with self.subTest(vector=v["name"], call=call):
                            self.assertEqual(read(data) if data else None, value)

    def test_a_compressed_row_round_trips(self):
        for row in ([None, None, 5, 5, 5, 7], [1, 2, 3, 3, 4], [None] * 200, [9] * 70, list(range(80))):
            raw = filesfixtures.gaf_row(row)
            self.assertEqual(gaf_pixels(struct.pack("<H", len(raw)) + raw, 0, len(row), 1), [row])


class Judge(unittest.TestCase):
    def test_a_loader_that_answers_the_vectors_is_equal(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = filesgate.judge(replayer(Path(tmp)))
        self.assertEqual(result["verdict"], "equal")
        self.assertEqual(set(result["groups"]), set(filesgate.GROUPS))

    def test_a_wrong_value_differs_and_names_the_vector_but_not_the_stored_value(self):
        fault = "if v['name'] == 'shadowing': res = res[:2] + [['load', [-1, None]]] + res[3:]"
        with tempfile.TemporaryDirectory() as tmp:
            result = filesgate.judge(replayer(Path(tmp), fault), ("lookup",))
        self.assertEqual(result["verdict"], "differs")
        bad = {k: v for k, v in result["vectors"].items() if v["status"] != "equal"}
        self.assertEqual(list(bad), ["lookup/shadowing"])
        self.assertEqual(bad["lookup/shadowing"]["line"], 3)
        self.assertEqual(bad["lookup/shadowing"]["value"], ["load", [-1, None]])

    def test_a_loader_that_exits_badly_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = filesgate.judge(replayer(Path(tmp), "sys.exit(4)"), ("formats",))
        self.assertEqual(result["verdict"], "failed")
        self.assertEqual(result["vectors"]["formats/tdf"], {"status": "failed", "reason": "exit status 4"})


FAULTS = {
    "budget": ("mounting", "mounting/base", "mount"),
    "winner": ("lookup", "lookup/shadowing", "load\tunits\\core.fbi"),
    "case": ("lookup", "lookup/shadowing", "load\tunits\\Shadow.TXT"),
    "swap": ("listing", "listing/root", "find\t*\t-1\t1"),
    "read": ("reading", "reading/loose", "read\t1\t100"),
    "mountall": ("malformed", "malformed/mount", "mount"),
    "tdforder": ("formats", "formats/tdf", "tdf\tgamedata\\sample.tdf"),
    "cobswap": ("cob", "cob/scripts", "cob\tscripts\\armfix.cob"),
    "gafshort": ("gaf", "gaf/anims", "gaf\tanims\\fixture.gaf"),
    "3doparent": ("3do", "3do/model", "3do\tobjects3d\\fixture.3do"),
}


@unittest.skipUnless(ADAPTER.is_file() and shutil.which("wine"), "needs the files adapter and wine")
class Adapter(unittest.TestCase):
    def setUp(self):
        os.environ.setdefault("WINEDEBUG", "-all")

    def test_the_in_repo_adapter_is_equal_on_every_group(self):
        result = filesgate.judge(filesgate.loader_command(filesgate.DEFAULT_LOADER))
        self.assertEqual({g: s["status"] for g, s in result["groups"].items()},
                         {g: "equal" for g in filesgate.GROUPS},
                         [k for k, v in result["vectors"].items() if v["status"] != "equal"])

    def test_each_injected_fault_is_caught_at_its_call(self):
        for fault, (group, vector, call) in FAULTS.items():
            with self.subTest(fault=fault):
                env = dict(os.environ, BT_FILES_FAULT=fault)
                result = filesgate.judge(filesgate.loader_command(filesgate.DEFAULT_LOADER), (group,), env=env)
                self.assertEqual(result["groups"][group]["status"], "differs")
                first = next(k for k, v in result["vectors"].items() if v["status"] != "equal")
                self.assertEqual(first, vector)
                self.assertEqual(result["vectors"][vector]["call"], call)

    def test_the_kit_tags_are_reported(self):
        result = filesgate.judge(filesgate.loader_command(filesgate.DEFAULT_LOADER), ("formats",))
        self.assertEqual(set(result["kit"]), {"interfaces", "reference_traces"})


if __name__ == "__main__":
    unittest.main()
