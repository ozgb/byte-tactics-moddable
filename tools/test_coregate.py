"""Tests of tools/coregate.py and tools/coreadapter.py.

    uv run tools/test_coregate.py   # the tool tests, and the in-repo engine's when its adapters and wine exist

The engine tests are the gate's CI checks: the in-repo engine is equal on every group, and each injected fault
(BT_CORE_FAULT) makes its group differ and names the vector.
"""

from __future__ import annotations

import json
import os
import shutil
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import coregate

BUILD = coregate.ROOT / "build" / "modern"
ENGINE = all((BUILD / n).exists() for n in ("core_state_adapter.exe", "core_wasm_adapter.exe", "core_console_adapter.exe",
                                            "core_runtime_adapter.exe", "replay_native_test.exe", "files_adapter.exe",
                                            "wind_meteors.wasm"))
FAKE = """
import json, os, sys
sys.path.insert(0, %(tools)r)
import coregate
mode = os.environ.get("FAKE_MODE", "")
group, calls_path, out_path = sys.argv[1:4]
if mode == "crash":
    os.abort()
if mode == "exit":
    sys.exit(3)
if mode == "hang":
    import time
    time.sleep(60)
if mode == "nofile":
    sys.exit(0)
first = os.path.basename(calls_path) == "0.calls"
runs = os.path.dirname(calls_path)
if mode == "outlink":
    os.symlink(os.environ["FAKE_TARGET"], out_path)
    sys.exit(0)
if mode == "plant" and first:
    for name in os.listdir(runs):
        if name.endswith(".calls") and name != "0.calls":
            os.remove(os.path.join(runs, name))
            os.symlink(os.environ["FAKE_TARGET"], os.path.join(runs, name))
calls = [l.rstrip("\\n").split("\\t") for l in open(calls_path, newline="")]
for v in coregate.stored([group])[group]["vectors"]:
    if v["calls"] == calls:
        res = v["results"]
        if mode == "runslink" and first:
            os.rename(runs, runs + ".old")
            os.symlink(os.environ["FAKE_TARGET"], runs)
        with open(out_path, "w") as f:
            if mode == "malformed":
                f.write("no tab here\\n")
            if mode == "short":
                res = res[:-1]
            if mode == "wrong":
                res = [[res[0][0], "something else"]] + res[1:]
            if mode == "wrongfirst" and first:
                res = [[res[0][0], "something else"]] + res[1:]
            for op, value in res:
                f.write(op + "\\t" + json.dumps(value) + "\\n")
        sys.exit(0)
sys.exit(4)
"""


class Fake(unittest.TestCase):
    """The gate's verdicts on a fake core that replays the stored values, or fails in one way."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.tmp = Path(tempfile.mkdtemp(prefix="test-coregate-"))
        cls.script = cls.tmp / "fake.py"
        cls.script.write_text(FAKE % {"tools": str(coregate.ROOT / "tools")})
        cls.module = cls.tmp / "m.wasm"
        cls.module.write_bytes(b"\0asm\1\0\0\0")

    @classmethod
    def tearDownClass(cls) -> None:
        shutil.rmtree(cls.tmp)

    def run_fake(self, mode: str, group: str = "commands", target: Path | None = None, jobs: int = 8) -> dict:
        env = {**os.environ, "FAKE_MODE": mode, "FAKE_TARGET": str(target)}
        return coregate.judge(lambda inputs: [sys.executable, str(self.script)], self.module, (group, "play"), env=env,
                              jobs=jobs)

    def host_file(self) -> Path:
        """A file outside the inputs directory, written as an OUT line a gate that followed links would report."""
        where = Path(tempfile.mkdtemp(dir=self.tmp))
        marker = where / "marker"
        marker.write_text('deliver\t"HOST-SECRET-MARKER"\n')
        return marker

    def test_out_link_is_not_followed(self) -> None:
        marker = self.host_file()
        result = self.run_fake("outlink", target=marker)
        self.assertEqual({v["reason"] for v in result["vectors"].values()}, {"OUT is not a regular file"})
        self.assertNotIn("HOST-SECRET-MARKER", json.dumps(result))

    def test_planted_calls_link_is_not_written(self) -> None:
        marker = self.host_file()
        before = marker.read_bytes()
        result = self.run_fake("plant", target=marker, jobs=1)
        self.assertEqual(marker.read_bytes(), before)
        self.assertEqual(sorted(os.listdir(marker.parent)), ["marker"])
        self.assertNotIn("HOST-SECRET-MARKER", json.dumps(result))
        states = [v["status"] for v in result["vectors"].values()]
        self.assertEqual(states[0], "equal")
        self.assertTrue(all(s == "failed" for s in states[1:]))

    def test_runs_link_is_not_followed(self) -> None:
        marker = self.host_file()
        before = marker.read_bytes()
        result = self.run_fake("runslink", target=marker.parent, jobs=1)
        self.assertEqual(marker.read_bytes(), before)
        self.assertEqual(sorted(os.listdir(marker.parent)), ["0.out", "marker"])
        first = next(iter(result["vectors"].values()))
        self.assertEqual(first, {"status": "failed", "reason": "OUT is not a regular file"})
        self.assertTrue(all(v["status"] == "failed" for v in result["vectors"].values()))
        self.assertNotIn("HOST-SECRET-MARKER", json.dumps(result))

    def test_equal(self) -> None:
        for group in coregate.JUDGED:
            result = self.run_fake("", group)
            self.assertEqual(result["verdict"], "equal", group)
            self.assertEqual(result["groups"]["play"], {"status": "not_judged"})

    def test_differs_names_the_line(self) -> None:
        result = self.run_fake("wrong")
        self.assertEqual(result["verdict"], "differs")
        first = next(v for v in result["vectors"].values())
        self.assertEqual((first["line"], first["value"]), (1, ["deliver", "something else"]))

    def test_short_output_is_missing(self) -> None:
        result = self.run_fake("short", "rand")
        bad = [v for v in result["vectors"].values() if v["status"] != "equal"]
        self.assertTrue(bad)
        self.assertTrue(all(v["value"] == "missing" for v in bad))

    def test_failures(self) -> None:
        expect = {"crash": "exit status -6", "exit": "exit status 3", "nofile": "no output file",
                  "malformed": "malformed output"}
        for mode, reason in expect.items():
            result = self.run_fake(mode)
            self.assertEqual(result["verdict"], "failed", mode)
            self.assertTrue(all(v["reason"].startswith(reason) for v in result["vectors"].values()), mode)

    def test_hang(self) -> None:
        with mock.patch.object(coregate, "TIMEOUT", 1):
            result = self.run_fake("hang")
        self.assertEqual(next(iter(result["vectors"].values()))["reason"], "timed out after 1 s")

    def test_runtime_names_the_entry_and_record(self) -> None:
        result = self.run_fake("wrongfirst", "runtime")
        bad = {k: v for k, v in result["vectors"].items() if v["status"] != "equal"}
        first = coregate.runtime_vectors()["vectors"][0]
        self.assertEqual(list(bad), ["runtime/" + first["name"]])
        self.assertEqual(bad["runtime/" + first["name"]]["line"], 1)
        self.assertEqual(len(result["vectors"]), len(coregate.runtime_layouts()))

    def test_runtime_records(self) -> None:
        data = bytes.fromhex("feffffff") + bytes.fromhex("0300000061620a") + bytes.fromhex("000000000000f03f")
        value, pos = coregate.read_field(data, 0, "i32")
        self.assertEqual((value, pos), (-2, 4))
        value, pos = coregate.read_field(data, pos, "str")
        self.assertEqual((value, pos), ("61620a", 11))
        value, pos = coregate.read_field(data, pos, "f64")
        self.assertEqual(value, 0x3FF0000000000000)
        self.assertEqual(coregate.read_field(data, 0, "bytes2"), ("feff", 2))
        with self.assertRaises(ValueError):
            coregate.read_field(data, 4, "bytes16")

    def test_stored_value_not_in_result(self) -> None:
        result = self.run_fake("wrong", "hash")
        self.assertNotIn("b9da8a7f2d638968", json.dumps(result))

    def test_files_gate_that_cannot_run_is_failed(self) -> None:
        with mock.patch.object(coregate.filesgate, "stored_groups", side_effect=coregate.CouldNotRun("no fixtures")):
            result = coregate.judge(lambda inputs: [sys.executable, str(self.script)], self.module, ("files",))
        self.assertEqual(result["groups"]["files"], {"status": "failed", "vectors": 0, "reason": "no fixtures"})
        self.assertEqual(result["verdict"], "failed")

    def test_refuses_a_file_that_is_not_elf(self) -> None:
        core = self.tmp / "core"
        core.write_bytes(b"#!/bin/sh\n")
        result = coregate.judge_submitted(core, self.module, self.tmp / "image")
        self.assertEqual(result["verdict"], "refused")
        self.assertIn("not an ELF file", result["message"])
        self.assertEqual(result["core"]["bytes"], 10)
        self.assertNotIn("groups", result)

    def test_sandboxed_command(self) -> None:
        """A submitted core runs through the sandbox's prefix, in the inputs directory."""
        seen = []

        class Box:
            def __init__(self, image, share):
                seen.append(Path(share))

            def __enter__(self):
                return self

            def __exit__(self, *exc):
                return False

            def prefix(self):
                return []

        sys.path.insert(0, str(coregate.ROOT / "tools" / "cleanroom"))
        import coresandbox

        core = self.tmp / "sh"
        shutil.copyfile("/bin/sh", core)
        with mock.patch.object(coresandbox, "Sandbox", Box):
            result = coregate.judge_submitted(core, self.module, self.tmp / "image", ("commands",))
        self.assertTrue(seen[0].is_relative_to(coregate.ROOT / "build" / "scratch"))
        self.assertFalse(seen[0].exists())
        self.assertEqual(result["verdict"], "failed")
        self.assertIn("sha256", result["core"])

    def test_direct_form_refused(self) -> None:
        for argv in (["core", "--module", "m.wasm"], ["core", "--trusted"], []):
            with mock.patch.object(sys, "argv", ["coregate.py", *argv]), self.assertRaises(SystemExit) as e, \
                    mock.patch("sys.stderr"):
                coregate.main()
            self.assertEqual(e.exception.code, 2, argv)


@unittest.skipUnless(ENGINE and shutil.which("wine"), "needs the modern build's adapters and wine")
class Engine(unittest.TestCase):
    """The in-repo engine is equal, and each fault makes its group differ."""

    def judge(self, groups, fault: str = "") -> dict:
        env = {**os.environ, "BT_CORE_FAULT": fault, "WINEDEBUG": "-all"}
        return coregate.judge(coregate.trusted_core, coregate.TRUSTED_MODULE, groups, env=env)

    def test_equal(self) -> None:
        result = self.judge(coregate.GROUPS)
        self.assertEqual(result["verdict"], "equal", json.dumps(result["groups"]))

    def test_faults(self) -> None:
        faults = {"hash": "hash_step", "rand": "rand_skip", "events": "events_continuation",
                  "commands": "commands_swap", "loader": "loader_class", "fuel": "fuel_budget", "host": "host_write",
                  "console": "console_byte"}
        for group, fault in faults.items():
            result = self.judge((group,), fault)
            self.assertEqual(result["groups"][group]["status"], "differs", fault)
            self.assertTrue(any(v["status"] == "differs" and v["line"] >= 1 for v in result["vectors"].values()))

    def test_console_byte_names_the_vector(self) -> None:
        result = self.judge(("console",), "console_byte")
        bad = {k for k, v in result["vectors"].items() if v["status"] != "equal"}
        self.assertIn("console/give-numbers", bad)

    def test_runtime_faults(self) -> None:
        """A wrong first result of each entry makes that entry differ at its first record, and leaves another equal."""
        layouts = coregate.runtime_layouts()
        names = list(layouts)
        for i, entry in enumerate(names):
            other = names[(i + 1) % len(names)]
            with self.subTest(entry=entry), mock.patch.object(
                    coregate, "runtime_layouts", return_value={entry: layouts[entry], other: layouts[other]}):
                result = self.judge(("runtime",), "runtime:" + entry)
                self.assertEqual(result["groups"]["runtime"]["status"], "differs")
                self.assertEqual(result["vectors"]["runtime/" + entry]["line"], 1)
                self.assertEqual(result["vectors"]["runtime/" + other]["status"], "equal")


if __name__ == "__main__":
    unittest.main()
