"""Tests for oraclegate.py: a divergence report goes in, no reference value comes out; a refusal comes back with the loader's message.

    uv run tools/test_oraclegate.py
"""

import contextlib
import io
import json
import os
import re
import subprocess
import sys
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
        self.assertEqual(suite["allied-victory"], {"status": "equal", "digest": {"status": "equal"}, "view": {"status": "equal"}})
        self.assertEqual(suite["console-features"], {"status": "differs", "tick": 70, "phase": "meteors",
                                                     "field": "SimGlobals.g_meteorScheduleGap", "value": "0x3c",
                                                     "digest": {"status": "not_judged"}, "view": {"status": "not_judged"}})
        self.assertEqual(suite["console-sight"], {"status": "not_judged", "digest": {"status": "not_judged"}, "view": {"status": "not_judged"}})

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
        self.assertEqual(suite["console-features"], {"status": "failed", "reason": "unknown", "digest": {"status": "not_judged"}, "view": {"status": "not_judged"}})
        self.assertEqual(oraclegate.verdict_of(suite, {}), "failed")

    def test_a_scenario_whose_module_trapped_says_so(self):
        report = f"scenarios failed to run:\nconsole-features: failed to run, playtest.py ended with:\n    wind_meteors trapped at tick 4: wasm trap: out of bounds memory access {REFERENCE_VALUE}\n"
        suite = oraclegate.parse_suite(report, 1, ["console-features"])
        self.assertEqual((suite["console-features"]["reason"], suite["console-features"]["class"]), ("module-trap", "memory_access"))
        self.assertNotIn(REFERENCE_VALUE, json.dumps(suite))

    def test_a_scenario_that_ended_early_names_its_reason(self):
        report = ("EARLY console-features: ended at tick 3 of 300 (exit code 2), host said wine.log: wind_meteors ran out of fuel at tick 4: "
                  "it burned more than its fuel_budget of 9 in one tick [kept: build/scratch/x]\nEARLY allied-victory: ended at tick 9 of 900 (timeout)\n")
        suite = oraclegate.parse_suite(report, 1, ["console-features", "allied-victory"])
        self.assertEqual({k: suite["console-features"][k] for k in ("status", "reason", "class")}, {"status": "failed", "reason": "module-trap", "class": "out_of_fuel"})
        self.assertEqual(suite["allied-victory"]["reason"], "host-timeout")

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


END_OF_TICK_REPORT = f"""ok   a (900 ticks, 1:02)
ok   b (300 ticks, 0:40, 12 sound lines) [digest differs from tick 41]
DIFF c (300 ticks, 0:40) tick 70: stored {REFERENCE_HASH}, got badc0de [kept: build/scratch/x] [digest differs from tick 70]
3 of 3 scenarios equal, total 0:30
2 of 3 scenarios equal on every row

tracediff for c: a = the reference at 0123abc, b = build/oracle/runtime/TotalA.exe
first divergence: tick 70, phase tick
  first differing field: Players[0].metal: a={REFERENCE_VALUE} b=0x3c
"""


class DigestVerdict(unittest.TestCase):
    def module(self, tmp):
        module = Path(tmp) / "m.wasm"
        module.write_bytes(b"\0asm\1\0\0\0")
        return module

    def test_the_suffix_gives_each_scenarios_digest_verdict(self):
        suite = oraclegate.parse_suite(END_OF_TICK_REPORT, 1, ["a", "b", "c", "d"])
        self.assertEqual(suite["a"], {"status": "equal", "digest": {"status": "equal"}, "view": {"status": "equal"}})
        self.assertEqual(suite["b"], {"status": "equal", "digest": {"status": "differs", "tick": 41}, "view": {"status": "equal"}})
        self.assertEqual(suite["c"]["status"], "differs")
        self.assertEqual((suite["c"]["tick"], suite["c"]["phase"], suite["c"]["value"]), (70, "tick", "0x3c"))
        self.assertEqual(suite["c"]["digest"], {"status": "differs", "tick": 70})
        self.assertEqual(suite["d"], {"status": "not_judged", "digest": {"status": "not_judged"}, "view": {"status": "not_judged"}})

    def test_the_suffix_gives_each_scenarios_view_verdict(self):
        report = ("ok   a (10 ticks, 0:01) [digest differs from tick 3] [view differs from tick 5]\n"
                  "ok   b (10 ticks, 0:01) [view differs from tick 2]\n"
                  "ok   c (10 ticks, 0:01)\n")
        suite = oraclegate.parse_suite(report, 0, ["a", "b", "c"])
        self.assertEqual(suite["a"]["digest"], {"status": "differs", "tick": 3})
        self.assertEqual(suite["a"]["view"], {"status": "differs", "tick": 5})
        self.assertEqual(suite["b"]["digest"], {"status": "equal"})
        self.assertEqual(suite["b"]["view"], {"status": "differs", "tick": 2})
        self.assertEqual(suite["c"]["view"], {"status": "equal"})
        self.assertEqual(oraclegate.digest_of(suite, "view"), "differs")
        self.assertEqual(oraclegate.verdict_of(suite, {}), "equal")

    def test_a_failed_scenario_has_no_digest_verdict(self):
        suite = oraclegate.parse_suite("a: failed to run, playtest.py ended with:\n", 1, ["a"])
        self.assertEqual(suite["a"]["digest"], {"status": "not_judged"})

    def test_the_result_has_format_4_and_the_top_level_digest_and_view(self):
        self.assertEqual(oraclegate.FORMAT, 4)
        suite = oraclegate.parse_suite("ok   a (10 ticks, 0:01) [digest differs from tick 4]\nok   b (10 ticks, 0:01)\n", 0, ["a", "b"])
        with tempfile.TemporaryDirectory() as tmp:
            result = oraclegate.build_result("wind_meteors", self.module(tmp), False, {}, suite, {"wind_meteors/x": {"status": "equal"}}, None, 1)
        self.assertEqual(result["format"], 4)
        self.assertEqual(result["scope"], "scenarios")
        self.assertEqual(result["view"], "equal")
        self.assertEqual(result["verdict"], "equal")
        self.assertEqual(result["digest"], "differs")
        self.assertEqual(result["scenarios"]["a"]["digest"], {"status": "differs", "tick": 4})
        self.assertEqual(result["scenarios"]["b"]["digest"], {"status": "equal"})

    def test_the_top_level_digest_is_equal_or_not_judged(self):
        equal = oraclegate.parse_suite("ok   a (10 ticks, 0:01)\nok   b (10 ticks, 0:01)\n", 0, ["a", "b"])
        self.assertEqual(oraclegate.digest_of(equal), "equal")
        self.assertEqual(oraclegate.digest_of(oraclegate.parse_suite("", 1, ["a"])), "not_judged")
        self.assertEqual(oraclegate.digest_of({}), "not_judged")

    def test_the_verdict_stays_the_state_verdict(self):
        suite = oraclegate.parse_suite("ok   a (10 ticks, 0:01) [digest differs from tick 4]\n", 0, ["a"])
        self.assertEqual(oraclegate.verdict_of(suite, {}), "equal")

    def test_a_refused_module_has_no_digest(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = oraclegate.build_result("wind_meteors", self.module(tmp), False, {}, None, None, "bad", 1)
        self.assertNotIn("digest", result)
        self.assertNotIn("view", result)

    def test_no_reference_value_reaches_the_digest_fields(self):
        suite = oraclegate.parse_suite(END_OF_TICK_REPORT, 1, ["a", "b", "c"])
        text = json.dumps(suite)
        for secret in (REFERENCE_VALUE, REFERENCE_HASH, "0123abc"):
            self.assertNotIn(secret, text)


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


REPLAY_SECRET = "0x7a5e11"
REPLAY_EQUAL = "tests/replay/wind_meteors/a.txt: 120 calls, 40 imports, identical on a second run\nreplay_native_test: all passed\n"
REPLAY_DIVERGED = f"""DIVERGED tests/replay/wind_meteors/a.txt: first at call 2, SimGlobals.g_meteorScheduleGap
call 0 1 UpdateWind ok imports=0 hash=0badf00d
call 1 1 UpdateMeteors ok imports=2 hash=12345678
call 2 2 UpdateMeteors DIFFERS
    SimGlobals.g_meteorScheduleGap: recorded {REPLAY_SECRET}, replayed 155
    SimGlobals.g_other: recorded 1, replayed 2
replay_native_test: 1 failures
"""


class ReplayReport(unittest.TestCase):
    def test_equal(self):
        self.assertEqual(oraclegate.parse_replay(REPLAY_EQUAL, 0), {"status": "equal"})

    def test_the_first_difference_has_the_modules_side_only(self):
        found = oraclegate.parse_replay(REPLAY_DIVERGED, 1)
        self.assertEqual(found, {"status": "differs", "call": 2, "tick": 2, "phase": "UpdateMeteors",
                                 "field": "SimGlobals.g_meteorScheduleGap", "value": "155"})
        self.assertNotIn(REPLAY_SECRET, json.dumps(found))

    def test_wine_line_ends(self):
        self.assertEqual(oraclegate.parse_replay(REPLAY_DIVERGED.replace("\n", "\r\n"), 1), oraclegate.parse_replay(REPLAY_DIVERGED, 1))
        self.assertEqual(oraclegate.parse_replay(REPLAY_EQUAL.replace("\n", "\r\n"), 0), {"status": "equal"})

    def test_an_import_key_keeps_its_index_and_name(self):
        report = REPLAY_DIVERGED.replace("SimGlobals.g_meteorScheduleGap", "import 3 services.SimAlloc")
        found = oraclegate.parse_replay(report, 1)
        self.assertEqual((found["field"], found["value"]), ("import 3 services.SimAlloc", "155"))

    def test_the_array_forms_give_the_replayed_side(self):
        self.assertEqual(oraclegate.replayed_side(f"byte 4 of 16: recorded {REPLAY_SECRET[2:4]}, replayed 7f, 2 bytes differ"), "byte 4: 7f")
        self.assertEqual(oraclegate.replayed_side("recorded 9 elements, replayed 8"), "8 elements")
        self.assertEqual(oraclegate.replayed_side("recorded an array, replayed null"), "null")
        self.assertEqual(oraclegate.replayed_side(f"recorded {REPLAY_SECRET}, replayed an array"), "an array")
        self.assertEqual(oraclegate.replayed_side(f"recorded {REPLAY_SECRET}, replayed no call (missing)"), "no call (missing)")

    def test_a_query_with_no_answer_names_the_query(self):
        report = REPLAY_DIVERGED.replace("SimGlobals.g_meteorScheduleGap", "query services.GetGafFrame").replace(
            f"recorded {REPLAY_SECRET}, replayed 155", "recorded no answer to these arguments, replayed services.GetGafFrame FrameTable#1 9")
        found = oraclegate.parse_replay(report, 1)
        self.assertEqual((found["field"], found["value"]), ("query services.GetGafFrame", "services.GetGafFrame FrameTable#1 9"))

    def test_a_fatal_stop_is_failed_with_no_text(self):
        report = f"FAIL: tests/replay/wind_meteors/a.txt: call 3: a slot with no runner: {REPLAY_SECRET}\n"
        self.assertEqual(oraclegate.parse_replay(report, 2), {"status": "failed", "reason": "unknown"})

    def test_an_identical_line_with_a_failing_exit_is_failed(self):
        self.assertEqual(oraclegate.parse_replay("FAIL: replaying twice gives the same result\n" + REPLAY_EQUAL, 1), {"status": "failed", "reason": "unknown"})

    def test_a_key_with_odd_characters_is_dropped(self):
        found = oraclegate.parse_replay(REPLAY_DIVERGED.replace("SimGlobals.g_meteorScheduleGap", "a: b = c d e"), 1)
        self.assertIsNone(found["field"])


BANNER = "replaying against the WASM module Z:\\out\\runtime\\wind_meteors.wasm\n"
STOP = "FAIL: Z:\\out\\runtime\\wind_meteors.wasm: "
BACKTRACE = f"error while executing at wasm backtrace:\n    0:  0x1a2b - stack_{REPLAY_SECRET}\n\nCaused by:\n    wasm trap: "
REASONS = {
    "host-runtime, kernel32": ("wine: could not load kernel32.dll, status c0000135\n", 1, {"reason": "host-runtime"}),
    "host-runtime, the DLL": (BANNER + STOP + "cannot load Z:\\out\\runtime\\wasmtime_host.dll\n", 2, {"reason": "host-runtime"}),
    "host-replayer": ('wine: cannot find L"Z:\\build\\replay_native_test.exe"\n', 1, {"reason": "host-replayer"}),
    "module-load, compile": (BANNER + STOP + f"module wind_meteors: compile failed: {REPLAY_SECRET}\n", 2, {"reason": "module-load", "class": "compile"}),
    "module-load, instantiate": (BANNER + STOP + "module wind_meteors: instantiate failed: unknown import: `env::x` has not been defined\n", 2,
                                 {"reason": "module-load", "class": "instantiate"}),
    "module-load, export": (BANNER + STOP + "module wind_meteors: no function export UpdateWind\n", 2, {"reason": "module-load", "class": "export"}),
    "module-load, major": (BANNER + STOP + "module wind_meteors: built against interface wind_meteors major 2, the manifest has 1\n", 2,
                           {"reason": "module-load", "class": "major"}),
    "module-trap, unreachable": (BANNER + STOP + "wind_meteors trapped: " + BACKTRACE + "wasm `unreachable` instruction executed\n", 2,
                                 {"reason": "module-trap", "class": "unreachable"}),
    "module-trap, memory": (BANNER + STOP + "wind_meteors trapped: " + BACKTRACE + "out of bounds memory access\n", 2, {"reason": "module-trap", "class": "memory_access"}),
    "module-trap, divide": (BANNER + STOP + "wind_meteors trapped: " + BACKTRACE + "integer divide by zero\n", 2, {"reason": "module-trap", "class": "divide"}),
    "module-trap, table": (BANNER + STOP + "wind_meteors trapped: " + BACKTRACE + "uninitialized element\n", 2, {"reason": "module-trap", "class": "table"}),
    "module-trap, stack": (BANNER + STOP + "wind_meteors trapped: " + BACKTRACE + "call stack exhausted\n", 2, {"reason": "module-trap", "class": "stack"}),
    "module-trap, other": (BANNER + STOP + "wind_meteors trapped: " + BACKTRACE + "something new\n", 2, {"reason": "module-trap", "class": "trap"}),
    "module-trap, fuel": (BANNER + STOP + "wind_meteors ran out of fuel: it burned more than its fuel_budget of 9 in one tick\n", 2,
                          {"reason": "module-trap", "class": "out_of_fuel"}),
    "module-trap, import": (BANNER + STOP + "import SimFree: the pointer is outside the sim block\n", 2, {"reason": "module-trap", "class": "import"}),
    "module-trap, trapped import": (BANNER + STOP + "wind_meteors trapped: import Emit: the kind is not in the module's publishes table\n", 2,
                                    {"reason": "module-trap", "class": "import"}),
    "module-trap, glue": (BANNER + STOP + "slot UpdateWind: the argument does not fit the scratch area\n", 2, {"reason": "module-trap", "class": "glue"}),
    "module-trap, handles": (BANNER + STOP + "wind_meteors: the handle table is full\n", 2, {"reason": "module-trap", "class": "glue"}),
    "module-trap, fatal": (BANNER + "wind_meteors: wl_call returned with a fatal pending\n" + STOP + f"{REPLAY_SECRET} is not a weapon\n", 2,
                           {"reason": "module-trap", "class": "fatal"}),
    "unknown, a crash": (BANNER + "wine: Unhandled page fault on read access to 00000000\n", 3, {"reason": "unknown"}),
    "unknown, a replayer stop": (BANNER + STOP + "the manifest has no fuel_budget\n", 2, {"reason": "unknown"}),
}


class FailureReason(unittest.TestCase):
    def test_each_reason(self):
        for name, (output, code, want) in REASONS.items():
            with self.subTest(name):
                found = oraclegate.parse_replay(output, code)
                self.assertEqual(found, {"status": "failed", **want})
                self.assertEqual(oraclegate.parse_replay(output.replace("\n", "\r\n"), code), found)

    def test_a_timeout_is_the_hosts(self):
        self.assertEqual(oraclegate.parse_replay(BANNER, None), {"status": "failed", "reason": "host-timeout"})

    def test_the_reasons_are_a_closed_set_and_name_no_text(self):
        reasons = {"host-runtime", "host-replayer", "host-timeout", "module-load", "module-trap", "unknown"}
        classes = {"compile", "instantiate", "export", "major", "unreachable", "memory_access", "divide", "table", "stack", "out_of_fuel", "import",
                   "glue", "fatal", "trap"}
        for name, (output, code, _) in REASONS.items():
            found = oraclegate.parse_replay(output, code)
            self.assertIn(found["reason"], reasons)
            self.assertIn(found.get("class", "trap"), classes)
            self.assertNotIn(REPLAY_SECRET, json.dumps(found))
            self.assertLessEqual(set(found), {"status", "reason", "class"})

    def test_the_summary_names_the_reason(self):
        recordings = {"wind_meteors/a": oraclegate.parse_replay(*REASONS["module-trap, unreachable"][:2])}
        with tempfile.TemporaryDirectory() as tmp:
            module = Path(tmp) / "m.wasm"
            module.write_bytes(b"\0asm\1\0\0\0")
            result = oraclegate.build_recordings_result("wind_meteors", module, {}, recordings, None, 3)
        self.assertEqual(oraclegate.summary(result).splitlines()[1], "  wind_meteors/a: failed (module-trap, unreachable)")


class Preflight(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.build = Path(self.tmp.name)
        for name in ("wasmtime_host.dll", oraclegate.REPLAYER):
            (self.build / name).write_text("x")

    def tearDown(self):
        self.tmp.cleanup()

    def run_with(self, stdout="", stderr="", code=2, raises=None):
        def fake(cmd, **kw):
            self.assertEqual(cmd, ["wine", str(self.build / oraclegate.REPLAYER)])
            if raises:
                raise raises
            return subprocess.CompletedProcess(cmd, code, stdout, stderr)
        with mock.patch.object(oraclegate.subprocess, "run", side_effect=fake):
            oraclegate.preflight(self.build)

    def assertFault(self, reason, **kw):
        with self.assertRaises(oraclegate.HostFault) as caught:
            self.run_with(**kw)
        self.assertEqual(caught.exception.reason, reason)

    def test_the_usage_line_passes(self):
        self.run_with("usage: replay_native_test [--wasm MODULE.wasm]\r\n")

    def test_a_broken_prefix_is_the_runtimes(self):
        self.assertFault("host-runtime", stderr="wine: could not load kernel32.dll, status c0000135\n", code=1)

    def test_no_wine_is_the_runtimes(self):
        self.assertFault("host-runtime", raises=FileNotFoundError("wine"))

    def test_a_hung_wine_is_the_runtimes(self):
        self.assertFault("host-runtime", raises=subprocess.TimeoutExpired("wine", 1))

    def test_a_missing_replayer_is_the_replayers(self):
        (self.build / oraclegate.REPLAYER).unlink()
        self.assertFault("host-replayer")

    def test_a_missing_dll_is_the_runtimes(self):
        (self.build / "wasmtime_host.dll").unlink()
        self.assertFault("host-runtime")

    def test_a_host_fault_writes_an_error_result(self):
        out = self.build / "out"
        argv = ["oraclegate.py", str(self.build / "wasmtime_host.dll"), "--subsystem", "wind_meteors", "--recordings-only", "--out", str(out),
                "--build-dir", str(self.build)]
        fault = oraclegate.HostFault("host-runtime", f"wine: could not load kernel32.dll in {self.build}")
        with mock.patch.object(sys, "argv", argv), mock.patch.object(oraclegate, "judge_recordings", side_effect=fault), \
                contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(oraclegate.main(), 2)
        result = json.loads((out / "result.json").read_text())
        self.assertEqual(result, {"format": oraclegate.FORMAT, "subsystem": "wind_meteors", "reason": "host-runtime",
                                  "error": oraclegate.HOST_ERRORS["host-runtime"]})
        self.assertNotIn(str(self.build), json.dumps(result))


class RecordingsRoute(unittest.TestCase):
    def module(self, tmp):
        module = Path(tmp) / "m.wasm"
        module.write_bytes(b"\0asm\1\0\0\0")
        return module

    def test_the_result_names_its_scope_and_rules(self):
        recordings = {"los_radar/console-sight": {"status": "equal"}}
        with tempfile.TemporaryDirectory() as tmp:
            result = oraclegate.build_recordings_result("los_radar", self.module(tmp), {}, recordings, None, 3)
        self.assertEqual((result["format"], result["scope"], result["scenarios"], result["verdict"], result["budgets"]), (4, "recordings", "not_judged", "equal", "judged"))
        self.assertEqual(result["rules"], {"queries": ["services.GetGafFrame"], "unread_arguments": ["los_radar.GetMapCellAtPosition pos.y"]})
        self.assertEqual(oraclegate.summary(result), "equal on recordings; scenarios not judged (0 not equal)")

    def test_a_difference_names_its_recording_and_call(self):
        recordings = {"wind_meteors/a": oraclegate.parse_replay(REPLAY_DIVERGED, 1), "wind_meteors/b": {"status": "equal"}}
        with tempfile.TemporaryDirectory() as tmp:
            result = oraclegate.build_recordings_result("wind_meteors", self.module(tmp), {}, recordings, None, 3)
        self.assertEqual(result["verdict"], "differs")
        self.assertEqual(oraclegate.summary(result).splitlines()[1], "  wind_meteors/a: differs at call 2, SimGlobals.g_meteorScheduleGap")

    def test_a_refused_module_has_no_recordings(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = oraclegate.build_recordings_result("los_radar", self.module(tmp), {}, None, "los_radar: the manifest has no size_budget", 1)
        self.assertEqual(result["verdict"], "refused")
        self.assertNotIn("recordings", result)

    def test_the_rules_are_the_replayers(self):
        source = (oraclegate.ROOT / "tests/replay_native_test.cpp").read_text()
        for name in oraclegate.RULES["queries"]:
            self.assertRegex(source, r"kQueryImports\[\] = \{[^}]*\"" + re.escape(name) + "\"")
        for rule in oraclegate.RULES["unread_arguments"]:
            name, vector = rule.split(" ")
            self.assertIn(f'{{ "{name}", "{vector.split(".")[0]}=(", 1 }}', source)

    def test_a_blocked_entry_takes_the_recordings_route(self):
        self.assertTrue(oraclegate.blocked("los_radar"))
        self.assertFalse(oraclegate.blocked("wind_meteors"))
        with self.assertRaises(oraclegate.Error):
            oraclegate.blocked("nope")


class BlockedEntry(unittest.TestCase):
    """A fixture manifest whose wind_meteors entry is marked blocked, without its budgets, judged with the replayer mocked."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.build = self.dir / "build"
        self.build.mkdir()
        for name in ("wasmtime_host.dll", oraclegate.REPLAYER):
            (self.build / name).write_text("x")
        game = json.loads(oraclegate.MANIFEST.read_text())
        entry = next(m for m in game["modules"] if m["name"] == "wind_meteors")
        for budget in ("fuel_budget", "size_budget"):
            del entry[budget]
        entry["blocked"] = "a fixture: its default does not build"
        self.manifest = self.dir / "modules.json"
        self.manifest.write_text(json.dumps(game))
        self.module = self.dir / "m.wasm"
        self.module.write_bytes(b"\0asm\1\0\0\0")
        self.calls = []

    def tearDown(self):
        self.tmp.cleanup()

    def fake_run(self, cmd, **kw):
        self.calls.append(cmd)
        if cmd[0] == "wine" and len(cmd) == 2:
            return subprocess.CompletedProcess(cmd, 2, "usage: replay_native_test\n", "")
        if cmd[0] == "wine":
            kw["stdout"].write(f"{cmd[-1]}: 10 calls, 3 imports, identical on a second run\nreplay_native_test: all passed\n")
            return subprocess.CompletedProcess(cmd, 0)
        if "check-module" in cmd:
            return subprocess.CompletedProcess(cmd, 0, "wind_meteors: ok\n", "")
        return subprocess.CompletedProcess(cmd, 1, "", "")

    def judge(self, manifest):
        with mock.patch.object(oraclegate.subprocess, "run", side_effect=self.fake_run):
            return oraclegate.judge_recordings(self.module, "wind_meteors", self.build, self.dir / "out", manifest)

    def test_a_blocked_entry_is_judged_on_its_recordings_without_budgets(self):
        self.assertTrue(oraclegate.blocked("wind_meteors", self.manifest))
        result = self.judge(self.manifest)
        self.assertEqual((result["scope"], result["scenarios"], result["budgets"], result["verdict"]), ("recordings", "not_judged", "not_judged", "equal"))
        self.assertEqual(sorted(result["recordings"]), [f"wind_meteors/{p.stem}" for p in oraclegate.recording_files("wind_meteors")])
        self.assertEqual(oraclegate.summary(result), "equal on recordings; scenarios not judged; budgets not judged (0 not equal)")
        replays = [c for c in self.calls if c[0] == "wine" and len(c) > 2]
        self.assertTrue(replays and all("--no-budgets" in c for c in replays))
        check = next(c for c in self.calls if "check-module" in c)
        self.assertEqual(check[check.index("--manifest") + 1], str(self.manifest))
        self.assertEqual((self.dir / "out/runtime/modules.json").read_text(), self.manifest.read_text())

    def test_an_entry_with_budgets_is_judged_with_them(self):
        result = self.judge(oraclegate.MANIFEST)
        self.assertEqual(result["budgets"], "judged")
        self.assertFalse(any("--no-budgets" in c for c in self.calls))
        self.assertFalse(any("--manifest" in c for c in self.calls))
        self.assertEqual(oraclegate.summary(result), "equal on recordings; scenarios not judged (0 not equal)")

    def test_a_refused_blocked_module_still_names_its_budgets(self):
        with mock.patch.object(oraclegate, "check_module", return_value="wind_meteors: has no stack pointer"):
            result = oraclegate.judge_recordings(self.module, "wind_meteors", self.build, self.dir / "out", self.manifest)
        self.assertEqual((result["verdict"], result["budgets"]), ("refused", "not_judged"))


class StaleResult(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.out = self.dir / "out"
        self.out.mkdir()
        (self.out / "result.json").write_text(json.dumps({"verdict": "equal"}))
        self.module = self.dir / "m.wasm"
        self.module.write_bytes(b"\0asm\1\0\0\0")

    def tearDown(self):
        self.tmp.cleanup()

    def test_a_recordings_run_that_stops_leaves_no_earlier_result(self):
        with mock.patch.object(oraclegate, "check_module", return_value=None), self.assertRaises(oraclegate.Error):
            oraclegate.judge_recordings(self.module, "wind_meteors", self.dir / "no-build", self.out)
        self.assertFalse((self.out / "result.json").exists())

    def test_a_scenario_run_that_stops_leaves_no_earlier_result(self):
        with mock.patch.object(oraclegate, "check_module", return_value=None), self.assertRaises((oraclegate.Error, OSError)):
            oraclegate.judge(self.module, "wind_meteors", False, self.dir / "no-build", self.out, None)
        self.assertFalse((self.out / "result.json").exists())


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
