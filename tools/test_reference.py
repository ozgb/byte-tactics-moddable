"""Tests for the trace name check in tools/reference.py.

    uv run tools/test_reference.py
"""

import contextlib
import io
import os
import re
import subprocess
import sys
import tempfile
import threading
import time
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


class SoundLogTest(unittest.TestCase):
    LOG = "bt-sound 1\n1 play a -585 - - -\n2 stop_all - - - - -\n"

    def test_equal_logs(self):
        self.assertIsNone(reference.sound_difference(self.LOG, self.LOG))

    def test_first_differing_line_is_named(self):
        got = self.LOG.replace("-585", "-1585")
        self.assertEqual(reference.sound_difference(self.LOG, got),
                         "sound line 2: stored `1 play a -585 - - -`, got `1 play a -1585 - - -`")

    def test_a_longer_log_differs_at_the_extra_line(self):
        self.assertIn("sound line 4: only the got log has it", reference.sound_difference(self.LOG, self.LOG + "3 play b -585 - - -\n"))
        self.assertIn("sound line 3: only the stored log has it", reference.sound_difference(self.LOG, "bt-sound 1\n1 play a -585 - - -\n"))

    def test_report_counts_unstored_logs_and_fails_only_on_stored_ones(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory / "same.txt").write_text(self.LOG)
            (directory / "other.txt").write_text(self.LOG)
            scenarios = {n: Path(n) for n in ("same", "other", "none")}
            logs = {"same": self.LOG, "other": self.LOG + "x\n", "none": self.LOG}
            with mock.patch.object(reference, "ROOT", directory.parent):
                differing, lines = reference.sound_report(scenarios, logs, directory)
        self.assertEqual(differing, ["other"])
        self.assertIn("1 of 3 equal, 1 differ, 1 with no stored log", lines[-1])

    def test_a_run_with_no_sound_log_differs_from_a_stored_one(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory / "a.txt").write_text(self.LOG)
            with mock.patch.object(reference, "ROOT", directory.parent):
                differing, _ = reference.sound_report({"a": Path("a")}, {"a": None}, directory)
        self.assertEqual(differing, ["a"])


class UiLogTest(unittest.TestCase):
    LOG = "bt-ui 1\n1 scroll 304 32\n1 select -\n20 input mouse left down 384 240\n22 select 1\n"

    def report(self, files: dict[str, str], scenarios: list[str], logs: dict[str, str | None], known: set[str] | None = None):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp) / "ui"
            directory.mkdir()
            for name, text in files.items():
                (directory / f"{name}.txt").write_text(text)
            with mock.patch.object(reference, "ROOT", directory.parent):
                return reference.ui_report({n: Path(n) for n in scenarios}, logs, directory, set(scenarios) if known is None else known)

    def test_equal_logs(self):
        self.assertIsNone(reference.ui_difference(self.LOG, self.LOG))

    def test_first_differing_line_is_named(self):
        got = self.LOG.replace("select 1", "select 2")
        self.assertEqual(reference.ui_difference(self.LOG, got), "ui line 5: stored `22 select 1`, got `22 select 2`")

    def test_a_longer_log_differs_at_the_extra_line(self):
        self.assertIn("ui line 6: only the got log has it", reference.ui_difference(self.LOG, self.LOG + "23 order Stop unit 1\n"))
        self.assertIn("ui line 5: only the stored log has it", reference.ui_difference(self.LOG, "bt-ui 1\n1 scroll 304 32\n1 select -\n20 input mouse left down 384 240\n"))

    def test_a_log_that_orders_or_selects_nothing_gives_nothing(self):
        self.assertTrue(reference.ui_gives_nothing("bt-ui 1\n1 scroll 304 32\n1 select -\n20 input mouse left down 384 240\n"))
        self.assertFalse(reference.ui_gives_nothing(self.LOG))
        self.assertFalse(reference.ui_gives_nothing("bt-ui 1\n21 order Stop unit 1 target - pos - type - param 0 amount 0 remove 0\n"))
        self.assertFalse(reference.ui_gives_nothing("bt-ui 1\n21 qbuild 2 ARMPW 1\n"))
        self.assertFalse(reference.ui_gives_nothing("bt-ui 1\n21 group 1 2,3\n"))

    def test_report_counts_unstored_logs_and_fails_on_stored_ones(self):
        differing, lines = self.report({"same": self.LOG, "other": self.LOG}, ["same", "other", "none"],
                                       {"same": self.LOG, "other": self.LOG + "x\n", "none": self.LOG})
        self.assertEqual(differing, ["other"])
        self.assertIn("1 of 3 equal, 1 differ, 1 with no stored log", lines[-1])

    def test_a_run_with_no_ui_log_differs_from_a_stored_one(self):
        differing, _ = self.report({"a": self.LOG}, ["a"], {"a": None})
        self.assertEqual(differing, ["a"])

    def test_a_ui_scenario_needs_a_stored_log(self):
        differing, lines = self.report({}, ["ui-click"], {"ui-click": self.LOG})
        self.assertEqual(differing, ["ui-click"])
        self.assertIn("has no stored UI log", lines[0])

    def test_a_ui_scenario_whose_input_gives_nothing_fails_though_equal(self):
        log = "bt-ui 1\n1 select -\n20 input mouse left down 384 240\n"
        differing, lines = self.report({"ui-click": log}, ["ui-click"], {"ui-click": log})
        self.assertEqual(differing, ["ui-click"])
        self.assertIn("the input gives no order and selects no unit", lines[0])
        self.assertEqual(self.report({"other": log}, ["other"], {"other": log})[0], [])

    def test_a_stored_log_with_no_scenario_fails(self):
        differing, lines = self.report({"a": self.LOG, "gone": self.LOG}, ["a"], {"a": self.LOG}, {"a"})
        self.assertEqual(differing, ["gone"])
        self.assertIn("DIFF gone stored UI log has no scenario", lines[0])

    def test_a_subset_does_not_flag_logs_of_scenarios_left_out(self):
        differing, _ = self.report({"a": self.LOG, "b": self.LOG}, ["a"], {"a": self.LOG}, {"a", "b"})
        self.assertEqual(differing, [])


class FrameTest(unittest.TestCase):
    WIDTH, HEIGHT = 4, 3

    def frame(self, tick: int = 30, palette: int = 7, pixels: bytes | None = None) -> bytes:
        header = b"BTFR" + (1).to_bytes(2, "little") + self.WIDTH.to_bytes(2, "little") + self.HEIGHT.to_bytes(2, "little") + bytes(2) + tick.to_bytes(4, "little")
        return header + bytes([palette]) * reference.FRAME_PALETTE_SIZE + (pixels or bytes(self.WIDTH * self.HEIGHT))

    def test_equal_frames(self):
        self.assertIsNone(reference.frame_difference(self.frame(), self.frame()))

    def test_first_differing_pixel_is_named(self):
        pixels = bytearray(self.WIDTH * self.HEIGHT)
        pixels[1 * self.WIDTH + 2] = 9
        self.assertEqual(reference.frame_difference(self.frame(), self.frame(pixels=bytes(pixels))), "pixel x 2 y 1: stored 0, got 9")

    def test_palette_and_header_differences(self):
        self.assertEqual(reference.frame_difference(self.frame(), self.frame(palette=8)), "palette")
        self.assertEqual(reference.frame_difference(self.frame(), self.frame(tick=31)), "header")

    def report(self, stored: dict[int, bytes], got: dict[int, bytes]):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            scenario = root / "a.txt"
            scenario.write_text("ticks 100\nframes 30 60\n")
            plain = root / "b.txt"
            plain.write_text("ticks 100\n")
            for tick, data in stored.items():
                (root / "frames" / "a").mkdir(parents=True, exist_ok=True)
                (root / "frames" / "a" / f"{tick}.bin").write_bytes(data)
            with mock.patch.object(reference, "ROOT", root):
                return reference.frame_report({"a": scenario, "b": plain}, {"a": got}, root / "frames")

    def test_report_equal(self):
        frames = {30: self.frame(30), 60: self.frame(60)}
        differing, lines = self.report(frames, frames)
        self.assertEqual(differing, [])
        self.assertEqual(lines, ["frames: 1 of 1 scenarios equal, 0 differ, 1 with no frames line"])

    def test_report_names_the_tick_and_pixel(self):
        pixels = bytearray(self.WIDTH * self.HEIGHT)
        pixels[0] = 5
        differing, lines = self.report({30: self.frame(30), 60: self.frame(60)}, {30: self.frame(30), 60: self.frame(60, pixels=bytes(pixels))})
        self.assertEqual(differing, ["a"])
        self.assertEqual(lines[0], "DIFF a frame 60: pixel x 0 y 0: stored 0, got 5")

    def test_missing_stored_frame_fails(self):
        differing, lines = self.report({30: self.frame(30)}, {30: self.frame(30), 60: self.frame(60)})
        self.assertEqual(differing, ["a"])
        self.assertIn("frame 60: no stored frame", lines[0])

    def test_missing_run_frame_fails(self):
        frames = {30: self.frame(30), 60: self.frame(60)}
        differing, lines = self.report(frames, {30: frames[30]})
        self.assertEqual(differing, ["a"])
        self.assertIn("frame 60: the run wrote no frame", lines[0])


class SaveReportTest(unittest.TestCase):
    @staticmethod
    def bank(box: bytes) -> bytes:
        import test_savefile
        return test_savefile.bank("Players", {"A": box})

    def test_equal_and_different_saves(self):
        self.assertIsNone(reference.save_difference(self.bank(b"abc"), self.bank(b"abc")))
        self.assertIn("Players/A offset 0x2", reference.save_difference(self.bank(b"abc"), self.bank(b"abd")))

    def test_only_scenarios_with_a_save_line_are_counted(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp) / "saves"
            directory.mkdir()
            saver, plain, unstored = (Path(tmp) / n for n in ("saver.txt", "plain.txt", "unstored.txt"))
            saver.write_text("ticks 10\nsave 5\n")
            unstored.write_text("save 5\n")
            plain.write_text("ticks 10\n")
            (directory / "saver.sav").write_bytes(self.bank(b"abc"))
            scenarios = {"saver": saver, "plain": plain, "unstored": unstored}
            with mock.patch.object(reference, "ROOT", Path(tmp)):
                differing, lines = reference.save_report(scenarios, {"saver": self.bank(b"abc"), "unstored": b"x"}, directory)
                self.assertEqual(differing, [])
                self.assertIn("saves: 1 of 2 equal, 0 differ, 1 with no stored save", lines[-1])
                differing, _ = reference.save_report(scenarios, {"saver": self.bank(b"abd")}, directory)
                self.assertEqual(differing, ["saver"])
                differing, _ = reference.save_report(scenarios, {"saver": None}, directory)
                self.assertEqual(differing, ["saver"])


class CopySavesTest(unittest.TestCase):
    def test_the_branch_saves_replace_the_stored_ones(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            branch = root / "branch/reference/saves"
            branch.mkdir(parents=True)
            (branch / "a.sav").write_bytes(b"new")
            (root / "reference/saves").mkdir(parents=True)
            (root / "reference/saves/old.sav").write_bytes(b"old")
            with mock.patch.object(reference, "ROOT", root), mock.patch.object(reference, "SAVES", root / "reference/saves"), \
                    contextlib.redirect_stdout(io.StringIO()) as out:
                reference.copy_saves(root / "branch")
            self.assertEqual(sorted(p.name for p in (root / "reference/saves").iterdir()), ["a.sav"])
            self.assertEqual((root / "reference/saves/a.sav").read_bytes(), b"new")
            self.assertIn("1 saves copied", out.getvalue())

    def test_a_branch_with_no_saves_leaves_the_stored_ones(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "branch").mkdir()
            (root / "reference/saves").mkdir(parents=True)
            (root / "reference/saves/old.sav").write_bytes(b"old")
            with mock.patch.object(reference, "ROOT", root), mock.patch.object(reference, "SAVES", root / "reference/saves"), \
                    contextlib.redirect_stdout(io.StringIO()) as out:
                reference.copy_saves(root / "branch")
            self.assertTrue((root / "reference/saves/old.sav").exists())
            self.assertIn("untouched", out.getvalue())


class StepsPerFrameTest(unittest.TestCase):
    def test_copies_end_with_the_key(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "a.txt").write_text("ticks 5\nmap X")
            (src / "b.txt").write_text("ticks 6\nsteps_per_frame 2\n")
            (src / "c.txt").write_text("ticks 7\ninput 3 char 97\n")
            copies = reference.with_steps_per_frame(
                {"a": src / "a.txt", "b": src / "b.txt", "c": src / "c.txt"}, 5, Path(tmp) / "out")
            self.assertEqual(copies["c"].read_text(), "ticks 7\ninput 3 char 97\n")
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


class WithRoundtripTest(unittest.TestCase):
    def rewrite(self, text: str, spec: str) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "a.txt"
            src.write_text(text)
            copies = reference.with_roundtrip({"a": src}, spec, Path(tmp) / "out")
            self.assertEqual(src.read_text(), text)
            return copies["a"].read_text().splitlines()

    def test_listed_ticks_become_lines(self):
        self.assertEqual(self.rewrite("ticks 50\nmap X", "3,40"), ["ticks 50", "map X", "roundtrip 3", "roundtrip 40"])

    def test_auto_picks_early_middle_and_last_minus_one(self):
        self.assertEqual(reference.roundtrip_ticks("auto", 200), [20, 100, 199])
        self.assertEqual(self.rewrite("ticks 200\n", "auto")[1:], ["roundtrip 20", "roundtrip 100", "roundtrip 199"])

    def test_auto_on_a_tiny_scenario_has_no_duplicates(self):
        self.assertEqual(reference.roundtrip_ticks("auto", 1), [1])
        self.assertEqual(reference.roundtrip_ticks("auto", 3), [1, 2])

    def test_bad_specs_fail(self):
        for spec in ("x", "0", "5,0"):
            with self.assertRaises(SystemExit):
                reference.roundtrip_ticks(spec, 100)

    def test_a_scenario_without_ticks_fails(self):
        with self.assertRaises(SystemExit):
            self.rewrite("map X\n", "auto")


class CheckRoundtripTest(unittest.TestCase):
    def test_scenarios_get_the_lines_and_the_pointer_scan_is_off(self):
        seen = {}
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "traces").mkdir()
            (tmp / "traces" / "a.txt").write_text("")
            (tmp / "a.txt").write_text("ticks 100\nmap X\n")
            header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
            stored = (header, [(1, "a" * 16)])

            def fake_play_all(exe, scenarios, game_dir, parallel, expected, poison, failed, keep_always=False, scans=None, scan_every=1, watch=None):
                seen["text"] = scenarios["a"].read_text()
                seen["scans"] = scans
                return {"a": (header, [(1, "a" * 16)], 1.0, None)}

            out = io.StringIO()
            with mock.patch.object(reference, "scenario_files", lambda: {"a": tmp / "a.txt"}), \
                    mock.patch.object(reference, "TRACES", tmp / "traces"), \
                    mock.patch.object(reference.tracedigest, "parse_digest", lambda p: stored), \
                    mock.patch.object(reference, "play_all", fake_play_all), \
                    mock.patch.object(reference, "sound_report", lambda *a: ([], [])), \
                    mock.patch.object(reference, "ui_report", lambda *a: ([], [])), \
                    mock.patch.object(reference, "report_fuel", lambda *a: 0), \
                    contextlib.redirect_stdout(out):
                reference.check_traces(Path("x.exe"), None, 1, roundtrip="auto")
        self.assertEqual(seen["text"], "ticks 100\nmap X\nroundtrip 10\nroundtrip 50\nroundtrip 99\n")
        self.assertEqual(seen["scans"], {})


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

            def fake_play_all(exe, scenarios, game_dir, parallel, expected, poison, failed, keep_always=False, scans=None, scan_every=1, watch=None):
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

        def fake_play(exe, scenario, game_dir, want, timeout, poison, every, collect=()):
            timeouts.append(timeout)
            if errors:
                raise reference.tracediff.Error(errors.pop(0))
            raise StopIteration

        with mock.patch.object(reference, "scenario_ticks", lambda s: 600), \
                mock.patch.object(reference, "scenario_frames", lambda s: []), \
                mock.patch.object(reference, "scenario_saves", lambda s: False), \
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

            def fake_play(exe, scenario, game_dir, want, timeout, poison, every, collect=()):
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

    def test_loader_runs_after_its_saver_and_before_the_scenarios_not_yet_started(self):
        for parallel in (1, 2, 6):
            with tempfile.TemporaryDirectory() as tmp:
                order, loaded, failed, results = self.play(self.scenarios(Path(tmp)), parallel)
            self.assertEqual(sorted(order), ["loader", "longer", "plain", "saver"])
            self.assertLess(order.index("saver"), order.index("loader"), parallel)
            self.assertEqual(failed, {})
            self.assertEqual(loaded, {"loader": {"harness.sav": b"harness.sav of saver", "harness.opt": b"harness.opt of saver"}})
            self.assertIn("loader", results)
        with tempfile.TemporaryDirectory() as tmp:
            order, *_ = self.play(self.scenarios(Path(tmp)), 1)
        self.assertEqual(order, ["longer", "saver", "loader", "plain"])

    def test_a_loader_starts_before_an_unrelated_longer_run_ends(self):
        started, ended = {}, {}
        lock = threading.Lock()
        gate = threading.Event()

        def fake_play_scenario(exe, scenario, game_dir, want=None, poison=False, keep_always=False, scans=None,
                               scan_every=1, retry_any=False, save_to=None, load_from=None):
            name = scenario.stem
            with lock:
                started[name] = time.monotonic()
            if name == "longer":
                gate.wait(10)
            if save_to is not None:
                save_to.mkdir(parents=True, exist_ok=True)
                for file in ("harness.sav", "harness.opt"):
                    (save_to / file).write_bytes(b"x")
            if name == "loader":
                gate.set()
            with lock:
                ended[name] = time.monotonic()
            return {}, [], 0.0, None

        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(reference.playtest, "template_prefix"), \
                mock.patch.object(reference, "play_scenario", fake_play_scenario):
            reference.play_all(Path("x.exe"), self.scenarios(Path(tmp)), None, 3, failed={})
        self.assertLess(started["loader"], ended["longer"])
        self.assertGreaterEqual(started["loader"], ended["saver"])

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
            self.assertEqual(seen["collect"][-2:], ("savegame/harness.sav", "savegame/harness.opt"))
            self.assertFalse(run.parent.exists())


class GateTierTest(unittest.TestCase):
    def test_every_scenario_has_a_trace_and_is_listed_once(self):
        scenarios = reference.scenario_files()
        self.assertEqual(len(set(reference.GATE_TIER)), len(reference.GATE_TIER))
        for name in reference.GATE_TIER:
            self.assertIn(name, scenarios)
            self.assertTrue((reference.TRACES / f"{name}.txt").is_file(), name)

    def test_a_loader_comes_with_its_saver(self):
        scenarios = reference.scenario_files()
        chosen = {n: scenarios[n] for n in reference.GATE_TIER}
        self.assertEqual(set(reference.with_savers(chosen, scenarios)), set(chosen))

    def test_it_keeps_a_game_of_eight_players(self):
        scenarios = reference.scenario_files()
        most = max(len(set(re.findall(r"^player(\d+)\.side", scenarios[n].read_text(), re.M))) for n in reference.GATE_TIER)
        self.assertGreaterEqual(most, 8)

    def test_the_pass_message_names_the_tier_and_the_commit(self):
        text = reference.tier_message(26, "0123456789ab")
        self.assertIn("gate tier passed: 26 scenarios equal at 0123456789ab", text)
        self.assertNotIn("not the gate", text)

    def test_block_pointers_are_left_to_the_full_suite(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            self.assertEqual(reference.report_block_pointers(Path("TotalA.exe"), {}, True, None), 0)
        self.assertEqual(out.getvalue().strip(), "block pointers: full suite only")


class CheckStopsAtFirstDifferenceTest(unittest.TestCase):
    NAMES = ("a", "b", "c")

    def test_the_other_runs_are_stopped_and_the_report_names_the_tick(self):
        header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
        good = [(1, "units", "1" * 16), (2, "units", "2" * 16), (3, "units", "3" * 16), (4, "units", "4" * 16)]
        bad = good[:1] + [(2, "units", "9" * 16)] + good[2:]
        stored = (header, reference.tracedigest.digests(good))
        procs, paths, calls, reported = {}, {}, [], []

        def rows(items):
            return "bt-trace 3\n" + "".join(f"{t} {ph} {h}\n" for t, ph, h in items)

        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)

            def fake_play(exe, scenario, game_dir, dump, timeout, poison, scan, collect=(), started=None):
                name = scenario.stem
                calls.append(name)
                trace = tmp / "runs" / name / "trace.txt"
                trace.parent.mkdir(parents=True)
                trace.write_text(rows(good[:1]))
                proc = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
                procs[name] = proc
                paths[proc.pid] = trace
                started(proc)
                if name == "a":
                    trace.write_text(rows(bad) + "3 units 33")
                try:
                    proc.wait(30)
                finally:
                    if proc.poll() is None:
                        proc.kill()
                        proc.wait()
                raise reference.tracediff.Error(f"playtest.py failed (exit {proc.returncode})")

            files = {n: tmp / f"{n}.txt" for n in self.NAMES}
            for path in files.values():
                path.write_text("ticks 4\n")
            (tmp / "traces").mkdir()
            for n in self.NAMES:
                (tmp / "traces" / f"{n}.txt").write_text("")
            out = io.StringIO()
            status = None
            with mock.patch.object(reference, "ROOT", tmp), \
                    mock.patch.object(reference, "scenario_files", lambda: files), \
                    mock.patch.object(reference, "TRACES", tmp / "traces"), \
                    mock.patch.object(reference, "WATCH_SECONDS", 0.05), \
                    mock.patch.object(reference, "game_trace", lambda pid: paths.get(pid)), \
                    mock.patch.object(reference, "scenario_ticks", lambda s: 4), \
                    mock.patch.object(reference, "scenario_frames", lambda s: []), \
                    mock.patch.object(reference, "scenario_saves", lambda s: False), \
                    mock.patch.object(reference.playtest, "template_prefix"), \
                    mock.patch.object(reference.tracedigest, "parse_digest", lambda p: stored), \
                    mock.patch.object(reference.tracediff, "play", fake_play), \
                    mock.patch.object(reference, "report_divergence", lambda name, *a: reported.append(name) or (_ for _ in ()).throw(SystemExit(1))), \
                    contextlib.redirect_stdout(out):
                try:
                    reference.check_traces(Path("x.exe"), None, 3)
                except SystemExit as e:
                    status = e.code
        self.assertEqual(status, 1)
        self.assertEqual(sorted(calls), ["a", "b", "c"])
        self.assertEqual(reported, ["a"])
        self.assertTrue(all(p.poll() is not None for p in procs.values()))
        self.assertRegex(out.getvalue(), r"DIFF a \(2 ticks, .*\) tick 2: stored \w{16}, got \w{16}")
        self.assertIn("stopped: 3 scenarios not finished", out.getvalue())

    def test_a_partial_last_line_is_not_compared(self):
        tail = reference.tracedigest.TraceTail()
        text = b"bt-trace 3\nseed 1\n1 units " + b"1" * 16 + b"\n2 units " + b"2" * 16 + b"\n3 un"
        self.assertEqual(len(tail.feed(text)), 1)
        self.assertEqual(tail.feed(b"its " + b"3" * 16 + b"\n"), [reference.tracedigest.digests([(2, "units", "2" * 16)])[0]])


class KeyLockTest(unittest.TestCase):
    CHILD = (
        "import sys, time\n"
        "from pathlib import Path\n"
        "import reference\n"
        "with reference.KeyLock(Path(sys.argv[1]), Path(sys.argv[2])):\n"
        "    print('held', flush=True)\n"
        "    sys.stdin.readline()\n"
    )

    def release(self, proc):
        """Let the child leave its lock (it may have left already) and wait for it."""
        try:
            proc.stdin.write("\n")
            proc.stdin.flush()
        except BrokenPipeError:
            pass
        proc.wait(30)
        proc.stdin.close()
        proc.stdout.close()

    def start(self, lock, worktree, env):
        return subprocess.Popen([sys.executable, "-c", self.CHILD, str(lock), worktree], stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE, text=True, env=env)

    def test_the_second_process_waits_and_names_the_first(self):
        env = dict(os.environ, PYTHONPATH=str(Path(reference.__file__).parent))
        with tempfile.TemporaryDirectory() as tmp:
            lock = Path(tmp) / "gate.lock"
            first = self.start(lock, "/work/first", env)
            try:
                self.assertEqual(first.stdout.readline().strip(), "held")
                second = self.start(lock, "/work/second", env)
                try:
                    self.assertEqual(second.stdout.readline().strip(), f"waiting for the gate run of {first.pid} (/work/first)")
                    self.assertIsNone(second.poll())
                    self.release(first)
                    self.assertEqual(second.stdout.readline().strip(), "held")
                    self.assertIn(f"{second.pid} /work/second", lock.read_text())
                finally:
                    self.release(second)
            finally:
                if first.poll() is None:
                    self.release(first)

    def test_the_lock_is_free_after_the_holder_leaves(self):
        with tempfile.TemporaryDirectory() as tmp:
            lock = Path(tmp) / "gate.lock"
            with reference.KeyLock(lock, Path("/w")):
                pass
            with contextlib.redirect_stdout(io.StringIO()) as out, reference.KeyLock(lock, Path("/w")):
                pass
            self.assertEqual(out.getvalue(), "")

    def test_the_locks_live_under_the_runtime_dir_or_tmp(self):
        with mock.patch.dict(os.environ, {"XDG_RUNTIME_DIR": "/run/user/1"}):
            self.assertEqual(reference.lock_dir(), Path("/run/user/1"))
            self.assertEqual(reference.key_lock_path("a" * 64), Path("/run/user/1") / ("bt-gate-key-" + "a" * 16 + ".lock"))
        with mock.patch.dict(os.environ):
            os.environ.pop("XDG_RUNTIME_DIR", None)
            self.assertEqual(reference.lock_dir(), Path("/tmp"))


class RunSlotsTest(unittest.TestCase):
    CHILD = (
        "import sys\n"
        "from pathlib import Path\n"
        "import reference\n"
        "with reference.RunSlots(Path(sys.argv[1]), 2, Path(sys.argv[2])).slot():\n"
        "    print('held', flush=True)\n"
        "    sys.stdin.readline()\n"
    )

    def start(self, directory, worktree, env):
        return subprocess.Popen([sys.executable, "-c", self.CHILD, str(directory), worktree], stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE, text=True, env=env)

    def release(self, proc):
        try:
            proc.stdin.write("\n")
            proc.stdin.flush()
        except BrokenPipeError:
            pass
        proc.wait(30)
        proc.stdin.close()
        proc.stdout.close()

    def test_with_two_slots_the_third_process_waits_until_one_is_released(self):
        env = dict(os.environ, PYTHONPATH=str(Path(reference.__file__).parent))
        with tempfile.TemporaryDirectory() as tmp:
            procs = [self.start(tmp, "/work/a", env), self.start(tmp, "/work/b", env)]
            try:
                for proc in procs:
                    self.assertEqual(proc.stdout.readline().strip(), "held")
                third = self.start(tmp, "/work/c", env)
                procs.append(third)
                said = third.stdout.readline().strip()
                self.assertTrue(said.startswith("waiting for a run slot, held by the gates of "), said)
                self.assertIn(f"{procs[0].pid} (/work/a)", said)
                self.assertIn(f"{procs[1].pid} (/work/b)", said)
                self.assertIsNone(third.poll())
                self.release(procs[0])
                self.assertEqual(third.stdout.readline().strip(), "held")
            finally:
                for proc in procs:
                    if proc.poll() is None:
                        self.release(proc)

    def test_threads_of_one_process_share_the_slots_and_free_them(self):
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(reference, "SLOT_POLL_SECONDS", 0.01):
            slots = reference.RunSlots(Path(tmp), 2, Path("/w"))
            live, most, guard = [0], [0], threading.Lock()

            def run():
                with slots.slot():
                    with guard:
                        live[0] += 1
                        most[0] = max(most[0], live[0])
                    time.sleep(0.05)
                    with guard:
                        live[0] -= 1

            with contextlib.redirect_stdout(io.StringIO()) as out:
                threads = [threading.Thread(target=run) for _ in range(6)]
                for t in threads:
                    t.start()
                for t in threads:
                    t.join()
            self.assertEqual(most[0], 2)
            self.assertEqual(out.getvalue().count("waiting for a run slot"), 1)
            with slots.slot():
                pass

    def test_no_lock_takes_no_slot(self):
        with mock.patch.object(reference, "GATE_RUN_SLOTS", None):
            with reference.run_slot():
                pass

    def test_the_count_comes_from_the_environment_else_the_default(self):
        with mock.patch.dict(os.environ, {reference.SLOTS_ENV: "3"}):
            self.assertEqual(reference.gate_slots(), 3)
        with mock.patch.dict(os.environ):
            os.environ.pop(reference.SLOTS_ENV, None)
            self.assertEqual(reference.gate_slots(), reference.GATE_SLOTS)


class BothModesTest(unittest.TestCase):
    def test_tags_name_the_mode(self):
        self.assertEqual(reference.tag("a", None), "a")
        self.assertEqual(reference.tag("a", "native"), "a [native]")
        self.assertEqual(reference.untag("a [native]"), "a")
        self.assertEqual(reference.mode_of("a [native]"), "native")
        self.assertIsNone(reference.mode_of("a"))
        self.assertEqual(reference.default_modes(True), ("wasm", "native"))
        self.assertEqual(reference.default_modes(False), (None,))

    def test_both_modes_may_run_more_at_once(self):
        with mock.patch.object(os, "cpu_count", lambda: 64):
            self.assertEqual(reference.default_parallel(), reference.MAX_PARALLEL)
            self.assertEqual(reference.default_parallel(True), reference.MAX_PARALLEL_BOTH_MODES)
        with mock.patch.object(os, "cpu_count", lambda: 4):
            self.assertEqual(reference.default_parallel(True), 2)

    def test_mode_view_keeps_the_stores_apart(self):
        store = {"a [wasm]": "w", "a [native]": "n", "b [native]": "m"}
        scenarios = {"a": Path("a.txt"), "b": Path("b.txt")}
        self.assertEqual(reference.mode_view(store, scenarios, "wasm"), {"a": "w"})
        self.assertEqual(reference.mode_view(store, scenarios, "native"), {"a": "n", "b": "m"})

    def test_the_watch_judges_a_native_run_against_the_scenarios_digests_and_names_the_mode(self):
        header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
        good = [(1, "units", "1" * 16), (2, "units", "2" * 16)]
        expected = {"a": (header, reference.tracedigest.digests(good))}
        with tempfile.TemporaryDirectory() as tmp:
            trace = Path(tmp) / "trace.txt"
            trace.write_text("bt-trace 3\n1 units " + "1" * 16 + "\n2 units " + "9" * 16 + "\n3 units " + "3" * 16 + "\n")
            proc = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
            watch = reference.Watch(expected, lambda pid: trace)
            try:
                watch.started("a [native]")(proc)
                watch.poll()
                self.assertTrue(watch.halted.is_set())
                self.assertEqual(watch.found.name, "a [native]")
                self.assertIn("tick 2", watch.found.diff)
            finally:
                proc.kill()
                proc.wait()

    def test_a_run_stores_its_scan_sound_and_fuel_under_its_tag(self):
        with tempfile.TemporaryDirectory() as tmp:
            run = Path(tmp) / "run"
            run.mkdir()
            (run / "trace.txt").write_text("bt-trace 3\ngame_seed 1\ncrt_seed 1\nview_seed 1\n1 units " + "1" * 16 + "\n")
            (run / "blockptrs.txt").write_text("scan")
            (run / "sound.txt").write_text("header\n")
            (run / reference.UI_FILE).write_text("ui\n")
            scans = {}
            with mock.patch.object(reference, "scenario_frames", lambda s: []), \
                    mock.patch.object(reference, "scenario_saves", lambda s: False), \
                    mock.patch.object(reference, "scenario_ticks", lambda s: 1), \
                    mock.patch.object(reference.tracediff, "play", lambda *a, **k: run), \
                    mock.patch.dict(reference.SOUND_LOGS), mock.patch.dict(reference.UI_LOGS), mock.patch.dict(reference.FUEL):
                reference.play_scenario(Path("x.exe"), Path("s.txt"), None, scans=scans, mode="native")
                self.assertEqual(scans, {"s [native]": "scan"})
                self.assertEqual(reference.SOUND_LOGS["s [native]"], "header\n")
                self.assertEqual(reference.UI_LOGS["s [native]"], "ui\n")
                self.assertIn("s [native]", reference.FUEL)

    def play(self, modes, loads=None, differ=None):
        calls, lock = [], threading.Lock()
        loads = loads or {}

        def fake_play_scenario(exe, scenario, game_dir, want, poison, keep_always, scans, scan_every, retry_any,
                               save_to=None, load_from=None, watch=None, mode=None):
            with lock:
                calls.append((scenario.stem, mode))
            reference.SOUND_LOGS[reference.tag(scenario.stem, mode)] = f"{mode}\n"
            return {}, [], 0.0, None

        scenarios = {n: Path(f"{n}.txt") for n in ("a", "b", "c")}
        with tempfile.TemporaryDirectory() as tmp, \
                mock.patch.object(reference, "ROOT", Path(tmp)), \
                mock.patch.object(reference, "scenario_ticks", lambda s: 100), \
                mock.patch.object(reference.playtest, "template_prefix"), \
                mock.patch.object(reference.tracediff, "scenario_load", lambda s: loads.get(s.stem)), \
                mock.patch.object(reference.tracediff, "save_put", lambda *a: {}), \
                mock.patch.object(reference, "play_scenario", fake_play_scenario):
            results = reference.play_all(Path("x.exe"), scenarios, None, 2, modes=modes)
        return calls, results

    def test_every_scenario_is_played_once_in_each_mode(self):
        calls, results = self.play(("wasm", "native"))
        self.assertEqual(sorted(calls), sorted((n, m) for n in "abc" for m in ("wasm", "native")))
        self.assertEqual(sorted(results), sorted(f"{n} [{m}]" for n in "abc" for m in ("wasm", "native")))

    def test_the_default_plays_each_scenario_once_with_no_mode(self):
        calls, results = self.play((None,))
        self.assertEqual(sorted(calls), [("a", None), ("b", None), ("c", None)])
        self.assertEqual(sorted(results), ["a", "b", "c"])

    def test_a_loader_is_played_in_its_savers_mode_after_its_saver(self):
        calls, _ = self.play(("wasm", "native"), loads={"c": "a"})
        for mode in ("wasm", "native"):
            self.assertLess(calls.index(("a", mode)), calls.index(("c", mode)))
        self.assertEqual(sorted(calls), sorted((n, m) for n in "abc" for m in ("wasm", "native")))


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


class GateCacheTest(unittest.TestCase):
    OPTIONS = {"tier": False, "mode": "wasm", "steps_per_frame": None, "quirks": None, "roundtrip": None, "poison": False}

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name) / "root"
        self.build = Path(tmp.name) / "build"
        self.cache = Path(tmp.name) / "cache"
        self.build.mkdir()
        for relative in (*reference.KEY_TREES, "tools"):
            (self.root / relative).mkdir(parents=True, exist_ok=True)
        for relative in reference.KEY_TREES:
            (self.root / relative / "a.txt").write_text("a")
        for relative in reference.KEY_TOOLS:
            (self.root / relative).write_text(relative)
        (self.root / "reference/commit.txt").write_text("c1")
        (self.root / "data").mkdir()
        (self.root / "data/block_pointers.csv").write_text("site,class\n")
        (self.root / "tools/reference.py").write_text("import helper\nfrom helper import x\n")
        (self.root / "tools/helper.py").write_text("def f():\n    import deeper\n")
        (self.root / "tools/deeper.py").write_text("x = 1\n")
        (self.root / "tools/unrelated.py").write_text("y = 1\n")
        self.exe = self.build / "TotalA.exe"
        for name in ("TotalA.exe", "wasmtime_host.dll", "modules.json", "m.wasm"):
            (self.build / name).write_text(name)
        patch = mock.patch.dict(os.environ, {reference.CACHE_ENV: str(self.cache)})
        patch.start()
        self.addCleanup(patch.stop)

    def key(self, **options):
        return reference.gate_key(self.exe, {**self.OPTIONS, **options}, self.root)

    def test_the_key_is_stable(self):
        self.assertEqual(self.key(), self.key())

    def test_each_input_changes_the_key(self):
        base = self.key()
        edits = [self.build / n for n in ("TotalA.exe", "wasmtime_host.dll", "modules.json", "m.wasm")]
        edits += [self.root / r / "a.txt" for r in reference.KEY_TREES]
        edits += [self.root / r for r in reference.KEY_TOOLS]
        edits += [self.root / r for r in reference.KEY_DATA]
        edits += [self.root / "tools/helper.py", self.root / "tools/deeper.py"]
        for path in edits:
            old = path.read_text()
            path.write_text(old + "x")
            self.assertNotEqual(self.key(), base, str(path))
            path.write_text(old)
        self.assertEqual(self.key(), base)

    def test_savefile_block_pointers_and_imports_change_the_key(self):
        base = self.key()
        for relative in ("tools/savefile.py", "data/block_pointers.csv", "tools/deeper.py"):
            self.assertIn(relative, (*reference.KEY_TOOLS, *reference.KEY_DATA, "tools/deeper.py"))
            path = self.root / relative
            old = path.read_text()
            path.write_text(old + "\n# changed\n")
            self.assertNotEqual(self.key(), base, relative)
            path.write_text(old)
        self.assertEqual(self.key(), base)

    def test_a_module_nothing_imports_is_not_in_the_key(self):
        base = self.key()
        (self.root / "tools/unrelated.py").write_text("y = 2\n")
        self.assertEqual(self.key(), base)

    def test_the_map_beside_the_exe_and_its_absence_change_the_key(self):
        absent = self.key()
        (self.build / "TotalA.map").write_text("m1")
        present = self.key()
        (self.build / "TotalA.map").write_text("m2")
        self.assertEqual(len({absent, present, self.key()}), 3)

    def test_a_new_file_in_a_stored_tree_changes_the_key(self):
        base = self.key()
        (self.root / "reference/frames/s").mkdir()
        (self.root / "reference/frames/s/0.bin").write_text("f")
        self.assertNotEqual(self.key(), base)

    def test_each_option_changes_the_key(self):
        base = self.key()
        for name, value in (("tier", True), ("mode", "native"), ("mode", "both"), ("steps_per_frame", 2),
                            ("quirks", "recommended"), ("roundtrip", "auto"), ("poison", True)):
            self.assertNotEqual(self.key(**{name: value}), base, name)

    def test_commit_txt_does_not_change_the_key(self):
        base = self.key()
        (self.root / "reference/commit.txt").write_text("c2")
        (self.build / "commit.txt").write_text("c2")
        self.assertEqual(self.key(), base)

    def test_nothing_is_stored_until_a_pass_is_written(self):
        self.assertIsNone(reference.cached_pass(self.key()))
        self.assertFalse(self.cache.exists())

    def test_a_pass_is_stored_with_date_commit_and_output(self):
        now = reference.datetime(2026, 11, 3, 12, 0, 0, tzinfo=reference.timezone.utc)
        path = reference.store_pass("k", "ok a\n2 of 2 equal\n", "abc123", now)
        self.assertEqual(path, self.cache / "k.txt")
        self.assertEqual(path.read_text(), "date: 2026-11-03 12:00:00 UTC\ncommit: abc123\nok a\n2 of 2 equal\n")
        self.assertEqual(reference.cached_pass("k"), path.read_text())

    def test_the_hit_prints_the_date_commit_and_stored_lines(self):
        reference.store_pass("k", "ok a\n2 of 2 equal\n", "abc123", reference.datetime(2026, 11, 3, tzinfo=reference.timezone.utc))
        with contextlib.redirect_stdout(io.StringIO()) as out:
            reference.report_cached(reference.cached_pass("k"))
        self.assertEqual(out.getvalue(), "cached pass from 2026-11-03 00:00:00 UTC at abc123\nok a\n2 of 2 equal\n")

    def run_main(self, check, *extra):
        argv = ["reference.py", "--check", "--exe", str(self.exe), *extra]
        with mock.patch.object(sys, "argv", argv), mock.patch.object(reference, "ROOT", self.root), \
                mock.patch.object(reference, "check_traces", check), mock.patch.object(reference, "head_commit", return_value="abc"), \
                mock.patch.object(reference, "KeyLock", mock.MagicMock()), contextlib.redirect_stdout(io.StringIO()) as out:
            try:
                reference.main()
                code = 0
            except SystemExit as e:
                code = e.code
        return code, out.getvalue()

    def test_main_plays_once_then_reports_the_cached_pass(self):
        played = []

        def check(*args):
            played.append(args)
            print("ok a\n1 of 1 equal")

        code, out = self.run_main(check)
        self.assertEqual((code, len(played), out), (0, 1, "ok a\n1 of 1 equal\n"))
        code, out = self.run_main(check)
        self.assertEqual((code, len(played)), (0, 1))
        self.assertTrue(out.startswith("cached pass from "))
        self.assertTrue(out.endswith(" at abc\nok a\n1 of 1 equal\n"))

    def test_main_plays_again_with_no_cache_or_other_options(self):
        played = []
        self.run_main(lambda *a: played.append(a))
        self.run_main(lambda *a: played.append(a), "--no-cache")
        self.run_main(lambda *a: played.append(a), "--tier")
        self.assertEqual(len(played), 3)

    def test_main_stores_nothing_for_a_failing_run(self):
        def check(*args):
            print("DIFF a")
            sys.exit(1)

        code, _ = self.run_main(check)
        self.assertEqual(code, 1)
        self.assertEqual(list(self.cache.glob("*.txt")) if self.cache.exists() else [], [])

    def test_a_second_gate_of_the_same_key_waits_and_reports_the_first_pass(self):
        played = []
        started, release = threading.Event(), threading.Event()

        def first(*args):
            played.append("first")
            started.set()
            release.wait(30)
            print("ok a\n1 of 1 equal")

        def second(*args):
            played.append("second")

        with tempfile.TemporaryDirectory() as tmp, mock.patch.dict(os.environ, {"XDG_RUNTIME_DIR": tmp}), \
                mock.patch.object(sys, "argv", ["reference.py", "--check", "--exe", str(self.exe)]), \
                mock.patch.object(reference, "ROOT", self.root), mock.patch.object(reference, "head_commit", return_value="abc"), \
                contextlib.redirect_stdout(io.StringIO()) as out:
            with mock.patch.object(reference, "check_traces", first):
                one = threading.Thread(target=reference.main)
                one.start()
                self.assertTrue(started.wait(30))
                with mock.patch.object(reference, "check_traces", second):
                    two = threading.Thread(target=reference.main)
                    two.start()
                    for _ in range(300):
                        if "waiting for the gate run of" in out.getvalue():
                            break
                        time.sleep(0.01)
                    self.assertIn("waiting for the gate run of", out.getvalue())
                    self.assertTrue(two.is_alive())
                    release.set()
                    one.join(30)
                    two.join(30)
        self.assertEqual(played, ["first"])
        self.assertRegex(out.getvalue(), r"cached pass from .* at abc\n")

    def test_a_pass_that_appears_while_waiting_for_the_lock_is_reported(self):
        played = []
        key = self.key(mode="wasm")

        class Lock:
            def __init__(inner, *args):
                pass

            def __enter__(inner):
                reference.store_pass(key, "ok a\n", "other")

            def __exit__(inner, *exc):
                pass

        with mock.patch.object(reference, "KeyLock", Lock):
            argv = ["reference.py", "--check", "--exe", str(self.exe)]
            with mock.patch.object(sys, "argv", argv), mock.patch.object(reference, "ROOT", self.root), \
                    mock.patch.object(reference, "check_traces", lambda *a: played.append(a)), \
                    contextlib.redirect_stdout(io.StringIO()) as out:
                reference.main()
        self.assertEqual(played, [])
        self.assertIn("at other\nok a\n", out.getvalue())


if __name__ == "__main__":
    unittest.main()
