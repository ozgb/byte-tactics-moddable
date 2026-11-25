"""Tests of tools/filesgate.py and tools/filesfixtures.py.

    uv run tools/test_filesgate.py              # the tool tests, and the in-repo adapter's when the adapter (build/modern/files_adapter.exe or $BT_FILES_ADAPTER) and wine exist

The adapter tests are the gate's CI checks: the in-repo adapter is equal on every group, and each injected fault
(BT_FILES_FAULT) makes its group differ and names the vector.
"""

from __future__ import annotations

import json
import os
import shutil
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

    def test_a_line_without_json_is_malformed(self):
        with self.assertRaises(ValueError):
            filesgate.parse_out("mount\n")
        self.assertEqual(filesgate.parse_out('read\t[1, "ab"]\n'), [["read", [1, "ab"]]])


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
    "mounting": ("winner", "mounting/base"),
    "lookup": ("case", "lookup/case"),
    "listing": ("swap", "listing/root"),
    "reading": ("read", "reading/loose"),
    "malformed": ("mountall", "malformed/mount"),
    "formats": ("tdforder", "formats/tdf"),
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

    def test_each_injected_fault_is_caught(self):
        for group, (fault, vector) in FAULTS.items():
            with self.subTest(fault=fault):
                env = dict(os.environ, BT_FILES_FAULT=fault)
                result = filesgate.judge(filesgate.loader_command(filesgate.DEFAULT_LOADER), (group,), env=env)
                self.assertEqual(result["groups"][group]["status"], "differs")
                self.assertEqual(result["vectors"][vector]["status"], "differs")


if __name__ == "__main__":
    unittest.main()
