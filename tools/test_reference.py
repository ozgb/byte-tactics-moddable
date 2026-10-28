"""Tests for the trace name check in tools/reference.py.

    uv run tools/test_reference.py
"""

import contextlib
import io
import os
import tempfile
import threading
import unittest
from pathlib import Path
from unittest import mock

import reference


class NameMismatchTest(unittest.TestCase):
    def test_equal_sets_pass(self):
        self.assertEqual(reference.name_mismatch({"a", "b"}, {"a", "b"}), "")

    def test_names_missing_and_extra(self):
        message = reference.name_mismatch({"a", "x"}, {"a", "b", "c"})
        self.assertIn("missing: b, c", message)
        self.assertIn("extra: x", message)

    def test_nothing_extra(self):
        self.assertIn("extra: none", reference.name_mismatch({"a"}, {"a", "b"}))


class StepsPerFrameTest(unittest.TestCase):
    def test_copies_end_with_the_key(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "a.txt").write_text("ticks 5\nmap X")
            (src / "b.txt").write_text("ticks 6\nsteps_per_frame 2\n")
            copies = reference.with_steps_per_frame(
                {"a": src / "a.txt", "b": src / "b.txt"}, 5, Path(tmp) / "out")
            self.assertEqual(copies["a"].read_text(), "ticks 5\nmap X\nsteps_per_frame 5\n")
            self.assertEqual(copies["b"].read_text().splitlines()[-1], "steps_per_frame 5")
            self.assertEqual((src / "a.txt").read_text(), "ticks 5\nmap X")


class WithQuirksTest(unittest.TestCase):
    def rewrite(self, text: str) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "a.txt"
            src.write_text(text)
            copies = reference.with_quirks({"a": src}, "3.1c +x", Path(tmp) / "out")
            self.assertEqual(src.read_text(), text)
            return copies["a"].read_text().splitlines()

    def test_line_goes_before_the_first_order(self):
        lines = self.rewrite("ticks 5\norder 1 0:A:1 move 1 2\norder 2 0:A:1 move 3 4\n")
        self.assertEqual(lines, ["ticks 5", "quirks 3.1c +x", "order 1 0:A:1 move 1 2", "order 2 0:A:1 move 3 4"])

    def test_line_goes_at_the_end_without_orders(self):
        self.assertEqual(self.rewrite("ticks 5\nmap X"), ["ticks 5", "map X", "quirks 3.1c +x"])


class CheckQuirksTest(unittest.TestCase):
    NAMES = ("eq", "diff", "bad")
    STORED = [(1, "a" * 16), (2, "b" * 16)]
    DIFFERS = [(1, "a" * 16), (2, "c" * 16)]

    def run_check(self, header_extra=None, diff_digests=None, failed_names=(), off_failed=None, off_digests=None):
        """check_traces(quirks=...) with play_all mocked, for a quirks run and the all-off rerun of "diff".

        Returns (exit status or None, output, scenario texts played, the names rerun).
        """
        header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1", **(header_extra or {})}
        plain = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
        stored = (plain, self.STORED)
        played, reruns = {}, []
        self.rerun_texts = {}

        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)

            def kept(name, phase_hashes):
                d = tmp / name
                d.mkdir()
                rows = [f"{t} {ph} {h}" for t, ph, h in phase_hashes]
                (d / "trace.txt").write_text("bt-trace 3\n" + "\n".join(rows) + "\n")
                return d

            quirk_rows = [(1, "units", "1" * 16), (2, "units", "2" * 16), (2, "players", "9" * 16)]
            off_rows = [(1, "units", "1" * 16), (2, "units", "2" * 16), (2, "players", "3" * 16)]

            def fake_play_all(exe, scenarios, game_dir, parallel, expected, poison, failed, keep_always=False, scans=None, scan_every=1):
                if keep_always:
                    reruns.extend(scenarios)
                    self.rerun_texts = {name: path.read_text() for name, path in scenarios.items()}
                    if off_failed:
                        failed["diff"] = off_failed
                        return {}
                    return {"diff": (plain, off_digests or self.STORED, 1.0, kept("off", off_rows))}
                for name, path in scenarios.items():
                    played[name] = path.read_text()
                for name in failed_names:
                    failed[name] = "playtest.py ended with a crash"
                return {
                    "eq": (header, self.STORED, 1.0, None),
                    "bad": (header, self.STORED, 1.0, None),
                    "diff": (header, diff_digests or self.STORED, 1.0, kept("quirks", quirk_rows)),
                }

            files = {}
            for n in self.NAMES:
                files[n] = tmp / f"{n}.txt"
                files[n].write_text("ticks 2\norder 1 0:A:1 move 1 2\n")
            (tmp / "traces").mkdir()
            for n in self.NAMES:
                (tmp / "traces" / f"{n}.txt").write_text("")
            out = io.StringIO()
            status = None
            with mock.patch.object(reference, "scenario_files", lambda: files), \
                    mock.patch.object(reference, "TRACES", tmp / "traces"), \
                    mock.patch.object(reference.tracedigest, "parse_digest", lambda p: stored), \
                    mock.patch.object(reference, "play_all", fake_play_all), \
                    contextlib.redirect_stdout(out):
                try:
                    reference.check_traces(Path("x.exe"), None, 1, quirks="recommended")
                except SystemExit as e:
                    status = e.code
        self.played_texts = played
        return status, out.getvalue(), played, reruns

    def test_header_quirks_line_alone_is_equal(self):
        status, out, played, reruns = self.run_check({"quirks": "0000abcd recommended"})
        self.assertIn("equal   eq", out)
        self.assertIn("equal   diff", out)
        self.assertNotIn("differs", out)
        self.assertIn("3 equal, 0 differ, 0 failed", out)
        self.assertIsNone(status)
        self.assertEqual(reruns, [])
        self.assertIn("quirks recommended\norder", played["eq"])

    def test_difference_names_the_tick_and_phase_and_is_not_a_failure(self):
        status, out, played, reruns = self.run_check(diff_digests=self.DIFFERS)
        self.assertIn("differs diff", out)
        self.assertIn("tick 2, phase players", out)
        self.assertIn("FUNCTION:", out)
        self.assertIn("2 equal, 1 differ, 0 failed", out)
        self.assertIsNone(status)
        self.assertEqual(reruns, ["diff"])
        self.assertIn("quirks recommended\norder", played["diff"])

    def test_all_off_rerun_plays_scenarios_without_the_quirks_line(self):
        self.run_check(diff_digests=self.DIFFERS)
        self.assertTrue(any(line.startswith("quirks") for line in self.played_texts["diff"].splitlines()))
        self.assertEqual(list(self.rerun_texts), ["diff"])
        self.assertFalse(any(line.startswith("quirks") for line in self.rerun_texts["diff"].splitlines()))

    def test_failed_scenario_exits_1(self):
        status, out, _, _ = self.run_check(failed_names=("bad",))
        self.assertIn("FAIL bad: playtest.py ended with a crash", out)
        self.assertIn("2 equal, 0 differ, 1 failed", out)
        self.assertEqual(status, 1)

    def test_failed_all_off_rerun_is_a_failure(self):
        status, out, _, _ = self.run_check(diff_digests=self.DIFFERS, off_failed="the exe hung")
        self.assertIn("FAIL diff: the all-off rerun failed: the exe hung", out)
        self.assertIn("2 equal, 0 differ, 1 failed", out)
        self.assertEqual(status, 1)

    def test_all_off_rerun_differing_from_stored_is_a_failure(self):
        status, out, _, _ = self.run_check(diff_digests=self.DIFFERS, off_digests=self.DIFFERS)
        self.assertIn("FAIL diff: all-off run differs from the stored trace", out)
        self.assertNotIn("differs diff", out)
        self.assertEqual(status, 1)

    def test_usage_errors(self):
        for argv in (["--quirks", "recommended"],
                     ["--check", "--exe", __file__, "--quirks", "recommended", "--poison"]):
            with mock.patch("sys.argv", ["reference.py", *argv]), \
                    contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as cm:
                reference.main()
            self.assertEqual(cm.exception.code, 2)


class ScanRunTest(unittest.TestCase):
    def run_scenario(self, scans, scan_every, retry_any, errors):
        timeouts = []

        def fake_play(exe, scenario, game_dir, want, timeout, poison, every):
            timeouts.append(timeout)
            if errors:
                raise reference.tracediff.Error(errors.pop(0))
            raise StopIteration

        with mock.patch.object(reference, "scenario_ticks", lambda s: 600), \
                mock.patch.object(reference.tracediff, "play", fake_play), \
                contextlib.redirect_stdout(io.StringIO()) as out:
            try:
                reference.play_scenario(Path("x.exe"), Path("s.txt"), None, scans=scans, scan_every=scan_every,
                                        retry_any=retry_any)
            except (StopIteration, reference.tracediff.Error) as e:
                return timeouts, e, out.getvalue()
        return timeouts, None, out.getvalue()

    def test_every_tick_scan_gets_the_longer_timeout(self):
        t = reference.RUN_SECONDS_BASE
        self.assertEqual(self.run_scenario({}, 1, False, [])[0], [t + 600 / reference.RUN_TICKS_PER_SECOND_SCAN_EVERY_TICK])
        self.assertEqual(self.run_scenario({}, 8, False, [])[0], [t + 600 / reference.RUN_TICKS_PER_SECOND])
        self.assertEqual(self.run_scenario(None, 1, False, [])[0], [t + 600 / reference.RUN_TICKS_PER_SECOND])

    def test_retry_any_retries_a_game_fault_once(self):
        fault = "scenario: playtest.py: dialog-r6025 pure virtual"
        timeouts, err, out = self.run_scenario({}, 1, True, [fault, fault])
        self.assertEqual(len(timeouts), 2)
        self.assertIsInstance(err, reference.tracediff.Error)
        self.assertIn("trying again", out)

    def test_without_retry_any_a_game_fault_fails_at_once(self):
        timeouts, _, _ = self.run_scenario({}, 1, False, ["x: dialog-r6025 pure virtual"])
        self.assertEqual(len(timeouts), 1)


class FailedAttemptTest(unittest.TestCase):
    def test_a_failed_attempt_is_kept_when_the_retry_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            session = root / "build/playtest/20260101-000000-1"
            (session / "run").mkdir(parents=True)
            (session / "run/wine.log").write_text("trace:seh:...\n")
            (session / "run/crash.txt").write_text("purecall from 0x0040106c\n")
            (session / "summary.md").write_text("| FAIL |\n")
            message = ("playtest.py failed for x.exe (exit 1):\n| FAIL |\n\n"
                       "logs and screenshots: build/playtest/20260101-000000-1/\n")
            run = root / "ok/run"
            run.mkdir(parents=True)
            (run / "trace.txt").write_text("")
            attempts = []

            def fake_play(exe, scenario, game_dir, want, timeout, poison, every):
                attempts.append(1)
                if len(attempts) == 1:
                    raise reference.tracediff.Error(message)
                return run

            (root / "s.txt").write_text("ticks 5\n")
            with mock.patch.object(reference, "ROOT", root), \
                    mock.patch.object(reference.tracediff, "play", fake_play), \
                    mock.patch.object(reference.tracediff, "parse_trace", lambda p: ({"game_seed": "1", "crt_seed": "1", "view_seed": "1"}, [])), \
                    mock.patch.object(reference.tracedigest, "digests", lambda rows: []), \
                    contextlib.redirect_stdout(io.StringIO()) as out:
                reference.play_scenario(Path("x.exe"), root / "s.txt", None)
            self.assertEqual(len(attempts), 2)
            kept = list((root / "build/scratch").glob("check-s-failed-*"))
            self.assertEqual(len(kept), 1)
            self.assertEqual((kept[0] / "run/crash.txt").read_text(), "purecall from 0x0040106c\n")
            self.assertTrue((kept[0] / "run/wine.log").is_file())
            self.assertTrue((kept[0] / "summary.md").is_file())
            self.assertIn(f"[failed attempt kept: build/scratch/{kept[0].name}]", out.getvalue())

    def test_an_error_that_names_no_session_keeps_nothing(self):
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(reference, "ROOT", Path(tmp)):
            self.assertIsNone(reference.keep_failed_attempt(RuntimeError("playtest.py failed"), "s"))
            self.assertIsNone(reference.keep_failed_attempt(RuntimeError("logs and screenshots: build/playtest/x/"), "s"))


class ReferenceEnvTest(unittest.TestCase):
    def test_pins_the_python_of_this_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / ".python-version").write_text("3.14\n")
            env = reference.reference_env({"VIRTUAL_ENV": "x", "PATH": "/bin"}, Path(tmp))
        self.assertEqual(env, {"PATH": "/bin", "UV_PYTHON": "3.14"})

    def test_keeps_an_explicit_uv_python(self):
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / ".python-version").write_text("3.14\n")
            env = reference.reference_env({"UV_PYTHON": "/usr/bin/python3.14"}, Path(tmp))
        self.assertEqual(env["UV_PYTHON"], "/usr/bin/python3.14")

    def test_without_the_pin_file_the_env_is_unchanged(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = reference.reference_env({"PATH": "/bin"}, Path(tmp))
        self.assertEqual(env, {"PATH": "/bin"})

    def test_module_env_carries_the_pin(self):
        pin = (reference.ROOT / ".python-version").read_text().strip()
        self.assertEqual(reference.REF_ENV["UV_PYTHON"], os.environ.get("UV_PYTHON", pin))


class LoadOrderTest(unittest.TestCase):
    def scenarios(self, tmp: Path, loader_saver: str = "saver") -> dict[str, Path]:
        texts = {"loader": f"ticks 9\nload {loader_saver}\n", "saver": "ticks 99\nsave 5\n", "plain": "ticks 5\n",
                 "longer": "ticks 500\n"}
        for name, text in texts.items():
            (tmp / f"{name}.txt").write_text(text)
        return {name: tmp / f"{name}.txt" for name in texts}

    def play(self, scenarios, parallel, writes_save=True, fail=(), writes=("harness.sav", "harness.opt")):
        order, loaded = [], {}
        lock = threading.Lock()

        def fake_play_scenario(exe, scenario, game_dir, want=None, poison=False, keep_always=False, scans=None,
                               scan_every=1, retry_any=False, save_to=None, load_from=None):
            name = scenario.stem
            with lock:
                order.append(name)
            if name in fail:
                raise reference.tracediff.Error("boom")
            if load_from is not None:
                loaded[name] = {f.name: f.read_bytes() for f in load_from.iterdir()}
            if save_to is not None and writes_save:
                save_to.mkdir(parents=True, exist_ok=True)
                for file in writes:
                    (save_to / file).write_bytes(file.encode() + b" of " + name.encode())
            return {}, [], 0.0, None

        failed: dict[str, str] = {}
        with mock.patch.object(reference.playtest, "template_prefix"), \
                mock.patch.object(reference, "play_scenario", fake_play_scenario):
            results = reference.play_all(Path("x.exe"), scenarios, None, parallel, failed=failed)
        return order, loaded, failed, results

    def test_loader_runs_after_every_scenario_that_loads_nothing(self):
        for parallel in (1, 2, 6):
            with tempfile.TemporaryDirectory() as tmp:
                order, loaded, failed, results = self.play(self.scenarios(Path(tmp)), parallel)
            self.assertEqual(order[-1], "loader", parallel)
            self.assertEqual(sorted(order), ["loader", "longer", "plain", "saver"])
            self.assertEqual(failed, {})
            self.assertEqual(loaded, {"loader": {"harness.sav": b"harness.sav of saver", "harness.opt": b"harness.opt of saver"}})
            self.assertIn("loader", results)

    def test_only_a_saver_collects_a_save_and_only_a_loader_gets_one(self):
        with tempfile.TemporaryDirectory() as tmp:
            saves = []

            def spy(*args, save_to=None, load_from=None, **kw):
                if save_to is not None:
                    save_to.mkdir()
                    (save_to / "harness.sav").write_bytes(b"x")
                    (save_to / "harness.opt").write_bytes(b"x")
                saves.append((args[1].stem, save_to is not None, load_from is not None))
                return {}, [], 0.0, None

            scenarios = self.scenarios(Path(tmp))
            with mock.patch.object(reference.playtest, "template_prefix"), mock.patch.object(reference, "play_scenario", spy):
                reference.play_all(Path("x.exe"), scenarios, None, 1, failed={})
        self.assertEqual(sorted(saves), [("loader", False, True), ("longer", False, False), ("plain", False, False),
                                         ("saver", True, False)])

    def test_saver_that_failed_fails_the_loader_naming_both(self):
        with tempfile.TemporaryDirectory() as tmp:
            order, loaded, failed, results = self.play(self.scenarios(Path(tmp)), 2, fail=("saver",))
        self.assertNotIn("loader", order)
        self.assertIn("loader: load saver: saver failed to run", failed["loader"])
        self.assertIn("saver", failed)

    def test_saver_that_wrote_no_save_fails_the_loader_naming_both(self):
        with tempfile.TemporaryDirectory() as tmp:
            order, loaded, failed, results = self.play(self.scenarios(Path(tmp)), 1, writes_save=False)
        self.assertNotIn("loader", order)
        self.assertIn("loader: load saver: saver wrote no savegame/harness.sav", failed["loader"])
        self.assertNotIn("saver", failed)

    def test_saver_that_wrote_only_one_file_fails_the_loader_naming_the_missing_one(self):
        for written, missing in (("harness.sav", "harness.opt"), ("harness.opt", "harness.sav")):
            with tempfile.TemporaryDirectory() as tmp:
                order, loaded, failed, results = self.play(self.scenarios(Path(tmp)), 1, writes=(written,))
            self.assertNotIn("loader", order)
            self.assertIn(f"loader: load saver: saver wrote no savegame/{missing}", failed["loader"])

    def test_saver_outside_the_set_fails_the_loader_naming_both(self):
        with tempfile.TemporaryDirectory() as tmp:
            scenarios = self.scenarios(Path(tmp))
            del scenarios["saver"]
            order, loaded, failed, results = self.play(scenarios, 1)
        self.assertNotIn("loader", order)
        self.assertIn("loader: load saver: saver is not among the scenarios played", failed["loader"])

    def test_with_savers_adds_the_scenario_a_loader_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            every = self.scenarios(Path(tmp))
            chosen = reference.with_savers({"loader": every["loader"]}, every)
            self.assertEqual(sorted(chosen), ["loader", "saver"])
            self.assertEqual(sorted(reference.with_savers({"plain": every["plain"]}, every)), ["plain"])

    def test_with_savers_ignores_a_name_that_is_no_scenario(self):
        with tempfile.TemporaryDirectory() as tmp:
            every = self.scenarios(Path(tmp), "nothing")
            self.assertEqual(sorted(reference.with_savers({"loader": every["loader"]}, every)), ["loader"])

    def test_the_save_is_copied_out_before_the_run_is_removed(self):
        with tempfile.TemporaryDirectory() as tmp:
            run = Path(tmp) / "session" / "run"
            (run / "savegame").mkdir(parents=True)
            (run / "savegame/harness.sav").write_bytes(b"data")
            (run / "savegame/harness.opt").write_bytes(b"options")
            (run / "trace.txt").write_text("")
            seen = {}

            def fake_play(exe, scenario, game_dir, dump, timeout, poison, scan, put=None, collect=()):
                seen.update(put=put, collect=collect)
                return run

            target = Path(tmp) / "kept"
            with mock.patch.object(reference.tracediff, "play", fake_play), \
                    mock.patch.object(reference.tracediff, "parse_trace", lambda p: ({"game_seed": "1", "crt_seed": "1", "view_seed": "1"}, [])), \
                    mock.patch.object(reference.tracedigest, "digests", lambda rows: []):
                (Path(tmp) / "s.txt").write_text("ticks 5\n")
                reference.play_scenario(Path("x.exe"), Path(tmp) / "s.txt", None, save_to=target)
            self.assertEqual((target / "harness.sav").read_bytes(), b"data")
            self.assertEqual((target / "harness.opt").read_bytes(), b"options")
            self.assertEqual(seen["collect"], ("savegame/harness.sav", "savegame/harness.opt"))
            self.assertFalse(run.parent.exists())


class TracediffCommandTest(unittest.TestCase):
    def test_names_the_reference_side(self):
        cmd = reference.tracediff_command(Path("a/TotalA.exe"), Path("b/TotalA.exe"), Path("s.txt"), False, None)
        self.assertEqual(cmd[cmd.index("--reference") + 1], "a")
        self.assertEqual(cmd[2:4], ["a/TotalA.exe", "b/TotalA.exe"])
        self.assertNotIn("--poison-b", cmd)
        self.assertNotIn("--game-dir", cmd)

    def test_poison_and_game_dir(self):
        cmd = reference.tracediff_command(Path("a"), Path("b"), Path("s"), True, "/ta")
        self.assertIn("--poison-b", cmd)
        self.assertEqual(cmd[cmd.index("--game-dir") + 1], "/ta")


if __name__ == "__main__":
    unittest.main()
