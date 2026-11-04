"""Tests for oraclegate.py: a divergence report goes in, no reference value comes out; a refusal comes back with the loader's message.

    uv run tools/test_oraclegate.py
"""

import json
import os
import subprocess
import tempfile
import unittest
from unittest import mock
from pathlib import Path

import oraclegate
import replay

REFERENCE_VALUE = "0x5ec2e7"
REFERENCE_HASH = "c0ffee1234567890"

REPORT = f"""ok   allied-victory (900 ticks, 1:02)
DIFF console-features (300 ticks, 0:40) tick 70: stored {REFERENCE_HASH}, got badc0de [kept: build/scratch/x]
stopped: 24 scenarios not finished

tracediff for console-features: a = the reference at 0123abc, b = build/oracle/runtime/TotalA.exe
first divergence: tick 70, phase meteors
  phase function: none (the once-per-tick tables, hashed after UpdateMeteors)
  first differing field: SimGlobals.g_meteorScheduleGap: a={REFERENCE_VALUE} b=0x3c
  all 2 differing fields:
    SimGlobals.g_meteorScheduleGap: a={REFERENCE_VALUE} b=0x3c
    SimGlobals.g_other: a=0x1 b=0x2
"""

STORED = "bt-replay 2\nsubsystem wind_meteors 1.2\ncall 1 UpdateWind\ncall 2 UpdateMeteors\n  r Game.width 1024\n  w SimGlobals.g_meteorScheduleGap 5\n"
GOT = STORED.replace("g_meteorScheduleGap 5", "g_meteorScheduleGap 155")

WINE_LOG = (
    '0120:warn:seh:dispatch_exception "wind_meteors before the match: module wind_meteors: instantiate failed: out of bounds\\n"\n'
    "wind_meteors before the match: module wind_meteors: instantiate failed: data segment out of bounds\n"
    "\n"
    "Caused by:\n"
    "    memory access at 0x2200000\n"
    "\n"
    "0120:warn:seh:dispatch_exception \"\\n\"\n"
)


class Suite(unittest.TestCase):
    def test_the_first_divergence_has_the_modules_side_only(self):
        suite = oraclegate.parse_suite(REPORT, 1, ["allied-victory", "console-features", "console-sight"])
        self.assertEqual(suite["allied-victory"], {"status": "equal"})
        self.assertEqual(suite["console-features"], {"status": "differs", "tick": 70, "phase": "meteors",
                                                     "field": "SimGlobals.g_meteorScheduleGap", "value": "0x3c"})
        self.assertEqual(suite["console-sight"], {"status": "not_judged"})

    def test_no_reference_value_reaches_the_result(self):
        suite = oraclegate.parse_suite(REPORT, 1, ["allied-victory", "console-features"])
        with tempfile.TemporaryDirectory() as tmp:
            module = Path(tmp) / "m.wasm"
            module.write_bytes(b"\0asm\1\0\0\0")
            result = oraclegate.build_result("wind_meteors", module, False, {"interfaces": "interfaces-v1", "reference_traces": "reference-traces-v20"},
                                             suite, {}, None, 12.4)
        text = json.dumps(result)
        for secret in (REFERENCE_VALUE, REFERENCE_HASH, "0123abc", "stored"):
            self.assertNotIn(secret, text)
        self.assertEqual(result["verdict"], "differs")
        self.assertEqual(result["module"]["sha256"], oraclegate.hashlib.sha256(b"\0asm\1\0\0\0").hexdigest())

    def test_a_scenario_that_failed_to_run(self):
        report = "scenarios failed to run:\nconsole-features: failed to run, playtest.py ended with:\n    the game exited with code 2\n"
        suite = oraclegate.parse_suite(report, 1, ["console-features"])
        self.assertEqual(suite["console-features"], {"status": "failed"})
        self.assertEqual(oraclegate.verdict_of(suite, {}), "failed")

    def test_other_checks_failing_is_not_equal(self):
        suite = oraclegate.parse_suite("ok   a (10 ticks, 0:01)\n", 1, ["a"])
        self.assertEqual(oraclegate.verdict_of(suite, {}), "failed")

    def test_a_block_pointer_failure_names_its_reason_and_no_code(self):
        report = ("ok   a (10 ticks, 0:01)\n1 of 1 scenarios equal, total 0:02\n"
                  "BLOCK POINTER ui-keys: ?Insert@?$BlockVector@ULosStep@@@@QAEXXZ offset 0xd -> ?_Insert@std@@ (holder not listed)\n"
                  "1 problems with the pointers out of the block; add them to data/block_pointers.csv (uv run tools/blockpointers.py --from <dir>, see reference/README.md)\n")
        suite = oraclegate.parse_suite(report, 1, ["a"])
        self.assertEqual(suite["other_checks"], {"status": "failed", "reason": "block-pointer"})
        self.assertEqual(oraclegate.verdict_of(suite, {}), "failed")
        self.assertNotIn("BlockVector", json.dumps(suite))
        self.assertNotIn("0xd", json.dumps(suite))
        labelled = oraclegate.parse_suite(report.replace("1 problems", "[native] 1 problems"), 1, ["a"])
        self.assertEqual(labelled["other_checks"]["reason"], "block-pointer")

    def test_another_failed_check_has_no_reason(self):
        suite = oraclegate.parse_suite("ok   a (10 ticks, 0:01)\nsound logs: 0 of 1 equal, 1 differ\n", 1, ["a"])
        self.assertEqual(suite["other_checks"], {"status": "failed"})

    def test_all_equal(self):
        suite = oraclegate.parse_suite("ok   a (10 ticks, 0:01)\n", 0, ["a"])
        self.assertEqual(oraclegate.verdict_of(suite, {"wind_meteors/x": {"status": "equal"}}), "equal")

    def test_a_name_with_odd_characters_is_dropped(self):
        field = "  first differing field: bad name here: a=1 b=2"
        suite = oraclegate.parse_suite("DIFF a (1 ticks, 0:01) tick 3: x\ntracediff for a: a = r\n" + field + "\n", 1, ["a"])
        self.assertIsNone(suite["a"]["field"])


class Recordings(unittest.TestCase):
    def test_a_difference_is_described_from_the_played_run(self):
        found = replay.divergence(STORED, GOT)
        self.assertEqual(found, {"status": "differs", "line": 6, "tick": 2, "slot": "UpdateMeteors", "kind": "w",
                                 "field": "SimGlobals.g_meteorScheduleGap", "value": "155"})

    def test_equal(self):
        self.assertEqual(replay.divergence(STORED, STORED), {"status": "equal"})

    def test_the_result_holds_the_allow_listed_fields(self):
        data = {"wind_meteors/a": replay.divergence(STORED, GOT), "wind_meteors/b": {"status": "equal"}}
        data["wind_meteors/a"]["stored"] = "g_meteorScheduleGap 5"
        out = oraclegate.parse_recordings(data)
        self.assertNotIn("stored", out["wind_meteors/a"])
        self.assertEqual(out["wind_meteors/b"], {"status": "equal"})
        self.assertEqual(out["wind_meteors/a"]["phase"], "UpdateMeteors")

    def test_a_shorter_run_names_no_line(self):
        found = replay.divergence(STORED, STORED[: STORED.rindex("  w ")])
        self.assertEqual(found["status"], "differs")
        self.assertIsNone(found["field"])


class Refusal(unittest.TestCase):
    def test_the_loaders_message_comes_back(self):
        message = oraclegate.refusal(WINE_LOG)
        self.assertTrue(message.startswith("wind_meteors before the match: module wind_meteors: instantiate failed"))
        self.assertNotIn("dispatch_exception", message)

    def test_no_message_in_a_clean_log(self):
        self.assertIsNone(oraclegate.refusal("0120:trace:seh:something\n"))

    def test_a_refused_result_has_no_scenarios(self):
        with tempfile.TemporaryDirectory() as tmp:
            module = Path(tmp) / "m.wasm"
            module.write_bytes(b"\0asm\1\0\0\0")
            result = oraclegate.build_result("wind_meteors", module, True, {}, None, None, oraclegate.refusal(WINE_LOG), 3)
        self.assertEqual(result["verdict"], "refused")
        self.assertNotIn("scenarios", result)
        self.assertIn("instantiate failed", result["message"])


class CheckFailure(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.module = self.dir / "m.wasm"
        self.module.write_bytes(b"\0asm\1\0\0\0")

    def tearDown(self):
        self.tmp.cleanup()

    def test_an_unknown_subsystem_is_an_error_not_a_refusal(self):
        with self.assertRaisesRegex(oraclegate.Error, "could not check the module .exit 2.*no module nope"):
            oraclegate.judge(self.module, "nope", False, self.dir / "no-build", self.dir / "out", None)
        self.assertFalse((self.dir / "out/result.json").exists())

    def test_a_missing_wasm_tools_is_an_error_not_a_refusal(self):
        saved = os.environ.get("WASM_TOOLS")
        os.environ["WASM_TOOLS"] = str(self.dir / "none")
        try:
            with self.assertRaises(oraclegate.Error):
                oraclegate.check_module(self.module, "wind_meteors")
        finally:
            if saved is None:
                del os.environ["WASM_TOOLS"]
            else:
                os.environ["WASM_TOOLS"] = saved

    def test_a_broken_module_is_still_a_refusal(self):
        problem = "wind_meteors: has no stack pointer\n"
        with mock.patch.object(oraclegate.subprocess, "run", return_value=subprocess.CompletedProcess([], 1, problem, "")):
            self.assertEqual(oraclegate.check_module(self.module, "wind_meteors"), problem.strip())

    def test_a_refusal_clears_the_earlier_runs_files(self):
        out = self.dir / "out"
        for stale in ("runtime/TotalA.exe", "logs/reference.txt"):
            (out / stale).parent.mkdir(parents=True, exist_ok=True)
            (out / stale).write_text("old")
        with mock.patch.object(oraclegate, "check_module", return_value="wind_meteors: has no stack pointer"):
            result = oraclegate.judge(self.module, "wind_meteors", False, self.dir / "no-build", out, None)
        self.assertEqual(result["verdict"], "refused")
        self.assertFalse((out / "runtime").exists() or (out / "logs").exists())


class PrepareRuntime(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.build = self.dir / "build"
        (self.build / "objs").mkdir(parents=True)
        for name in (*oraclegate.RUNTIME_FILES, "wind_meteors.wasm"):
            (self.build / name).write_text("x")
        (self.build / "modules.json").write_text(json.dumps({"modules": [{"name": "wind_meteors"}]}))
        (self.build / "objs/objects.rsp").write_text("/abs/a.obj\n")
        self.module = self.dir / "m.wasm"
        self.module.write_bytes(b"\0asm\1\0\0\0")

    def tearDown(self):
        self.tmp.cleanup()

    def test_the_object_list_is_copied_beside_the_map(self):
        exe = oraclegate.prepare_runtime(self.build, "wind_meteors", self.module, self.dir / "runtime")
        self.assertEqual((exe.parent / "objs/objects.rsp").read_text(), "/abs/a.obj\n")
        self.assertTrue((exe.parent / "TotalA.map").is_file())

    def test_a_build_without_the_object_list_is_an_error(self):
        (self.build / "objs/objects.rsp").unlink()
        with self.assertRaisesRegex(oraclegate.Error, "objects.rsp"):
            oraclegate.prepare_runtime(self.build, "wind_meteors", self.module, self.dir / "runtime")


if __name__ == "__main__":
    unittest.main()
