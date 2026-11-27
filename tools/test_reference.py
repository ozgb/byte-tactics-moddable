"""Tests for the trace name check in tools/reference.py.

    uv run tools/test_reference.py
"""

import contextlib
import datetime
import io
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path
from unittest import mock

import displaylog
import displayreplay
import eventdigest
import interfacelog
import reference
import tracediff
import tracerehash


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


class EventsLogTest(unittest.TestCase):
    LOG = "bt-events 1\n0 load smoke 0x0 1 2 3 15 900 9\n5 units sound 0x0 0 0\n5 units sound 0x0 1 0\n5 players radar_units 0x0 0800000000000000\n"
    DIGEST = eventdigest.digest_text(LOG)

    def test_the_digest_has_a_line_for_each_tick_and_phase(self):
        lines = self.DIGEST.splitlines()
        self.assertEqual(lines[:2], ["bt-event-digest 2", "log bt-events 1"])
        runs = [line for line in lines[2:] if " tick " not in line]
        self.assertEqual([line.rsplit(" ", 1)[0] for line in runs], ["0 load 1", "5 units 2", "5 players 1"])
        self.assertTrue(all(re.fullmatch(r"[0-9a-f]{16}", line.rsplit(" ", 1)[1]) for line in runs))
        self.assertEqual([line.split(" ")[:3] for line in eventdigest.tick_lines(self.DIGEST)], [["0", "tick", "1"], ["5", "tick", "3"]])

    def test_the_hash_is_the_trace_hash_of_the_records_bytes(self):
        hasher = tracerehash.Hasher()
        hasher.run(b"0 load smoke 0x0 1 2 3 15 900 9\n")
        self.assertEqual(self.DIGEST.splitlines()[2], f"0 load 1 {hasher.h:016x}")

    def test_equal_digests(self):
        self.assertIsNone(eventdigest.digest_difference(self.DIGEST, eventdigest.digest_text(self.LOG)))

    def test_a_changed_record_changes_the_hash_of_its_tick_and_phase(self):
        got = eventdigest.digest_text(self.LOG.replace("sound 0x0 1 0", "sound 0x0 2 0"))
        diff = eventdigest.digest_difference(self.DIGEST, got)
        self.assertTrue(diff.startswith("events digest line 5 (tick 5, phase units): stored `5 units 2 "), diff)

    def test_a_missing_or_extra_record_changes_the_count(self):
        got = eventdigest.digest_text(self.LOG.replace("5 units sound 0x0 1 0\n", ""))
        self.assertIn("(tick 5, phase units): stored `5 units 2 ", eventdigest.digest_difference(self.DIGEST, got))
        self.assertIn(" got `5 units 1 ", eventdigest.digest_difference(self.DIGEST, got))

    def test_a_different_header_differs_on_the_second_line(self):
        got = eventdigest.digest_text(self.LOG.replace("bt-events 1", "bt-events 2"))
        self.assertEqual(eventdigest.digest_difference(self.DIGEST, got),
                         "events digest line 2: stored `log bt-events 1`, got `log bt-events 2`")

    def test_a_longer_digest_differs_at_the_extra_line(self):
        got = eventdigest.digest_text(self.LOG + "6 meteors message 0x0 0 1\n")
        self.assertIn("line 8 (tick 6, phase meteors): only the got digest has it (`6 meteors 1 ", eventdigest.digest_difference(self.DIGEST, got))

    def test_a_dropped_line_is_a_record_of_its_phase(self):
        got = eventdigest.digest_text(self.LOG + "5 players dropped 3\n")
        self.assertIn("(tick 5, phase players): stored `5 players 1 ", eventdigest.digest_difference(self.DIGEST, got))
        self.assertIn(" got `5 players 2 ", eventdigest.digest_difference(self.DIGEST, got))

    def test_a_radar_snapshot_is_covered_byte_for_byte(self):
        got = eventdigest.digest_text(self.LOG.replace("0800000000000000", "0800000000000001"))
        self.assertIn("(tick 5, phase players)", eventdigest.digest_difference(self.DIGEST, got))

    def test_a_log_without_a_header_or_a_phase_is_refused(self):
        with self.assertRaises(eventdigest.Error):
            eventdigest.digest_text("0 load smoke 0x0\n")
        with self.assertRaises(eventdigest.Error):
            eventdigest.digest_text("bt-events 1\nbroken\n")

    def test_record_difference_names_the_record(self):
        got = self.LOG.replace("sound 0x0 1 0", "sound 0x0 2 0")
        self.assertEqual(tracediff.event_difference(self.LOG, got),
                         "event record at line 4 (tick 5, phase units): a `5 units sound 0x0 1 0`, b `5 units sound 0x0 2 0`")
        self.assertIsNone(tracediff.event_difference(self.LOG, self.LOG))
        self.assertEqual(tracediff.event_difference(self.LOG, self.LOG + "6 meteors message 0x0 0 1\n"),
                         "event record at line 6 (tick 6, phase meteors): only b has it (`6 meteors message 0x0 0 1`)")

    def test_report_counts_unstored_digests_and_fails_only_on_stored_ones(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory / "same.txt").write_text(self.DIGEST)
            (directory / "other.txt").write_text(self.DIGEST)
            scenarios = {n: Path(n) for n in ("same", "other", "none")}
            logs = {"same": self.DIGEST, "other": eventdigest.digest_text(self.LOG + "6 meteors message 0x0 0 1\n"), "none": self.DIGEST}
            with mock.patch.object(reference, "ROOT", directory.parent):
                differing, lines = reference.events_report(scenarios, logs, directory)
        self.assertEqual(differing, ["other"])
        self.assertIn("1 of 3 equal, 1 differ, 1 with no stored digest", lines[-1])

    def test_end_of_tick_compares_the_tick_lines_only(self):
        moved = eventdigest.digest_text(self.LOG.replace("5 players radar_units", "5 units radar_units"))
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory / "a.txt").write_text(self.DIGEST)
            (directory / "b.txt").write_text(self.DIGEST)
            scenarios = {"a": Path("a"), "b": Path("b")}
            logs = {"a": moved, "b": eventdigest.digest_text(self.LOG + "6 meteors message 0x0 0 1\n")}
            with mock.patch.object(reference, "ROOT", directory.parent):
                strict, _ = reference.events_report(scenarios, logs, directory)
                loose, lines = reference.events_report(scenarios, logs, directory, end_of_tick=True)
        self.assertEqual(strict, ["a", "b"])
        self.assertEqual(loose, ["b"])
        self.assertIn("1 of 2 equal, 1 differ", lines[-1])
        self.assertIn("(tick 6)", lines[0])

    def test_the_event_records_report_passes_the_end_of_tick_switch_to_tracediff(self):
        for end_of_tick in (False, True):
            with mock.patch.object(reference, "resolve_quiet", return_value="abc"), mock.patch.object(reference, "build_cached", return_value=Path("ref")), \
                    mock.patch.object(reference.subprocess, "run") as run, contextlib.redirect_stdout(io.StringIO()):
                reference.report_event_records("a", "wasm", {"a": ({"commit": "abc"},)}, Path("x.exe"), {"a": Path("s.txt")}, False, None, end_of_tick)
            cmd = run.call_args[0][0]
            self.assertEqual(cmd[-1], "--events")
            self.assertEqual("--end-of-tick" in cmd, end_of_tick)

    def test_a_run_with_no_event_log_differs_from_a_stored_one(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory / "a.txt").write_text(self.DIGEST)
            with mock.patch.object(reference, "ROOT", directory.parent):
                differing, _ = reference.events_report({"a": Path("a")}, {"a": None}, directory)
        self.assertEqual(differing, ["a"])

    def test_copy_events_writes_digests_not_logs(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, dest = Path(tmp) / "branch", Path(tmp) / "events"
            (src / "reference/events").mkdir(parents=True)
            (src / "reference/events/a.txt").write_text(self.LOG)
            with mock.patch.object(reference, "EVENTS", dest), mock.patch.object(reference, "ROOT", Path(tmp)):
                reference.copy_events(src)
            self.assertEqual((dest / "a.txt").read_text(), self.DIGEST)


class DisplayLogTest(unittest.TestCase):
    PALETTE = "0123456789abcdef"
    FIRST = "fill screen 0 0 10 10 1"
    SECOND = "fill screen 5 5 15 15 2"
    FAR = "fill screen 100 100 110 110 3"

    def frame(self, number: int, tick: int, *items: str) -> str:
        return "".join(line + "\n" for line in [f"frame {number} {tick} 1 {self.PALETTE}", *items, "cursor - - -"])

    def listing(self, *frames: str) -> str:
        return "bt-display 1.2\n" + "".join(frames)

    def run_report(self, report, stored: dict[str, str], scenarios: dict[str, Path], logs: dict[str, str | None], *args):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp) / "reference/dir"
            directory.mkdir(parents=True)
            for name, text in stored.items():
                if report is reference.display_list_report:
                    displayreplay.stored_list(name, directory).write_bytes(displayreplay.pack_list(text))
                else:
                    (directory / f"{name}.txt").write_text(text)
            with mock.patch.object(reference, "ROOT", Path(tmp)):
                return report(scenarios, logs, directory, *args, known={"a", "b", "none"})

    def digest(self, *frames: str) -> str:
        return displaylog.digest_text(self.listing(*frames))

    def test_digests_are_compared_line_by_line_and_unstored_ones_counted(self):
        same = self.digest(self.frame(1, 0, self.FIRST))
        other = self.digest(self.frame(1, 0, self.FIRST, self.FAR))
        scenarios = {n: Path(n) for n in ("a", "b", "none")}
        differing, lines = self.run_report(reference.display_report, {"a": same, "b": same}, scenarios, {"a": same, "b": other, "none": same})
        self.assertEqual(differing, ["b"])
        self.assertIn("(frame 1, tick 0)", lines[0])
        self.assertIn("display digests: 1 of 3 equal, 1 differ, 1 with no stored digest", lines[-1])

    def test_end_of_tick_digests_ignore_the_order_that_does_not_count(self):
        stored = self.digest(self.frame(1, 0, self.FIRST, self.FAR))
        got = self.digest(self.frame(1, 0, self.FAR, self.FIRST))
        strict, _ = self.run_report(reference.display_report, {"a": stored}, {"a": Path("a")}, {"a": got})
        loose, lines = self.run_report(reference.display_report, {"a": stored}, {"a": Path("a")}, {"a": got}, True)
        self.assertEqual((strict, loose), (["a"], []))
        self.assertIn("1 of 1 equal", lines[-1])

    def test_a_run_with_no_display_log_or_a_stored_digest_with_no_scenario_fails(self):
        stored = self.digest(self.frame(1, 0, self.FIRST))
        differing, lines = self.run_report(reference.display_report, {"a": stored, "gone": stored}, {"a": Path("a")}, {"a": None})
        self.assertEqual(differing, ["a", "gone"])
        self.assertIn("DIFF gone stored display digest has no scenario of that name", lines)

    def test_lists_are_compared_for_scenarios_with_a_frames_line_only(self):
        stored = self.listing(self.frame(1, 4, self.FIRST), self.frame(2, 8, self.SECOND))
        got = self.listing(self.frame(1, 4, self.FIRST), self.frame(2, 8, self.FAR))
        with tempfile.TemporaryDirectory() as tmp:
            framed, plain = Path(tmp) / "a.txt", Path(tmp) / "b.txt"
            framed.write_text("ticks 8\nframes 4 8\n")
            plain.write_text("ticks 8\n")
            differing, lines = self.run_report(reference.display_list_report, {"a": stored, "b": stored}, {"a": framed, "b": plain}, {"a": got, "b": got})
        self.assertEqual(differing, ["a"])
        self.assertIn("0 of 1 equal, 1 differ, 0 with no stored list", lines[-1])

    def test_a_differing_list_names_the_frame_and_item(self):
        stored = self.listing(self.frame(1, 4, self.FIRST), self.frame(2, 8, self.SECOND))
        got = self.listing(self.frame(1, 4, self.FIRST), self.frame(2, 8, self.FAR))
        with tempfile.TemporaryDirectory() as tmp:
            framed = Path(tmp) / "a.txt"
            framed.write_text("ticks 8\nframes 4 8\n")
            differing, lines = self.run_report(reference.display_list_report, {"a": stored}, {"a": framed}, {"a": got})
        self.assertEqual(differing, ["a"])
        self.assertIn("frame 2 (tick 8)", lines[0])
        self.assertIn("0 of 1 equal, 1 differ, 0 with no stored list", lines[-1])

    def test_a_scenario_with_frames_and_no_stored_list_is_counted(self):
        with tempfile.TemporaryDirectory() as tmp:
            framed = Path(tmp) / "a.txt"
            framed.write_text("frames 4\n")
            differing, lines = self.run_report(reference.display_list_report, {}, {"a": framed}, {"a": None})
        self.assertEqual(differing, [])
        self.assertIn("0 of 1 equal, 0 differ, 1 with no stored list", lines[-1])

    def test_a_frame_with_no_stored_list_is_named_by_its_tick(self):
        stored = self.digest(self.frame(1, 0, self.FIRST), self.frame(2, 4, self.FIRST))
        got = self.digest(self.frame(1, 0, self.FIRST), self.frame(2, 4, self.SECOND))
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            self.assertEqual(reference.unlisted_frame("a", {"a": got}, stored, False, directory), 4)
            displayreplay.stored_list("a", directory).write_bytes(displayreplay.pack_list(self.listing(self.frame(2, 4, self.FIRST))))
            self.assertIsNone(reference.unlisted_frame("a", {"a": got}, stored, False, directory))

    def test_the_display_items_report_asks_tracediff_for_the_tick(self):
        for end_of_tick in (False, True):
            with mock.patch.object(reference, "resolve_quiet", return_value="abc"), mock.patch.object(reference, "build_cached", return_value=Path("ref")), \
                    mock.patch.object(reference.subprocess, "run") as run, contextlib.redirect_stdout(io.StringIO()):
                reference.report_display_items("a", "wasm", 12, {"a": ({"commit": "abc"},)}, Path("x.exe"), {"a": Path("s.txt")}, False, None, end_of_tick)
            cmd = run.call_args[0][0]
            self.assertEqual(cmd[-3:], ["--display", "--display-tick", "12"])
            self.assertEqual("--end-of-tick" in cmd, end_of_tick)

    def test_copy_display_replaces_both_directories_and_leaves_one_with_no_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, root = Path(tmp) / "branch", Path(tmp) / "tree"
            (src / "reference/display").mkdir(parents=True)
            (src / "reference/display/a.txt").write_text("digest\n")
            (root / "reference/displaylists").mkdir(parents=True)
            (root / "reference/displaylists/old.txt").write_text("old\n")
            with mock.patch.object(reference, "ROOT", root), mock.patch.object(reference, "DISPLAYS", root / "reference/display"), \
                    mock.patch.object(reference, "DISPLAYLISTS", root / "reference/displaylists"), contextlib.redirect_stdout(io.StringIO()):
                reference.copy_display(src)
            self.assertEqual((root / "reference/display/a.txt").read_text(), "digest\n")
            self.assertEqual((root / "reference/displaylists/old.txt").read_text(), "old\n")

    def test_copy_display_stores_a_frontend_scenarios_digest(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, root = Path(tmp) / "branch", Path(tmp) / "tree"
            (src / "reference/display").mkdir(parents=True)
            (root / "reference/scenarios").mkdir(parents=True)
            for name, scenario in (("menu", "frontend 1\nticks 10\n"), ("battle", "ticks 10\n")):
                (src / f"reference/display/{name}.txt").write_text("digest\n")
                (root / f"reference/scenarios/{name}.txt").write_text(scenario)
            with mock.patch.object(reference, "ROOT", root), mock.patch.object(reference, "DISPLAYS", root / "reference/display"), \
                    mock.patch.object(reference, "DISPLAYLISTS", root / "reference/displaylists"), \
                    mock.patch.object(reference, "SCENARIOS", root / "reference/scenarios"), contextlib.redirect_stdout(io.StringIO()) as out:
                reference.copy_display(src)
            self.assertEqual(sorted(p.name for p in (root / "reference/display").iterdir()), ["battle.txt", "menu.txt"])
            self.assertIn("2 display files copied", out.getvalue())

    def test_copy_display_stores_the_lists_as_xz(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, root = Path(tmp) / "branch", Path(tmp) / "tree"
            (src / "reference/displaylists").mkdir(parents=True)
            (src / "reference/displaylists/a.txt").write_text("bt-display 1.2\n")
            (root / "reference/displaylists").mkdir(parents=True)
            (root / "reference/displaylists/old.txt.xz").write_bytes(displayreplay.pack_list("old\n"))
            with mock.patch.object(reference, "ROOT", root), mock.patch.object(reference, "DISPLAYS", root / "reference/display"), \
                    mock.patch.object(reference, "DISPLAYLISTS", root / "reference/displaylists"), contextlib.redirect_stdout(io.StringIO()):
                reference.copy_display(src)
            self.assertEqual(sorted(p.name for p in (root / "reference/displaylists").iterdir()), ["a.txt.xz"])
            self.assertEqual(displayreplay.read_list(root / "reference/displaylists/a.txt.xz"), "bt-display 1.2\n")

    def test_the_key_covers_the_display_files_and_tool(self):
        self.assertIn("reference/display", reference.KEY_TREES)
        self.assertIn("reference/displaylists", reference.KEY_TREES)
        self.assertIn("tools/displaylog.py", reference.KEY_TOOLS)
        self.assertIn("tools/displayreplay.py", reference.KEY_TOOLS)


class UiLogTest(unittest.TestCase):
    LOG = "bt-ui 2\n1 scroll 304 32\n1 select -\n20 input mouse left down 384 240\n22 select 1\n"

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
        self.assertIn("ui line 6: only the got log has it", reference.ui_difference(self.LOG, self.LOG + "23 group 1 2,3\n"))
        self.assertIn("ui line 5: only the stored log has it", reference.ui_difference(self.LOG, "bt-ui 2\n1 scroll 304 32\n1 select -\n20 input mouse left down 384 240\n"))

    def test_a_log_that_orders_or_selects_nothing_gives_nothing(self):
        self.assertTrue(reference.ui_gives_nothing("bt-ui 2\n1 scroll 304 32\n1 select -\n20 input mouse left down 384 240\n"))
        self.assertFalse(reference.ui_gives_nothing(self.LOG))
        self.assertFalse(reference.ui_gives_nothing("bt-ui 2\n21 group 1 2,3\n"))
        self.assertFalse(reference.ui_gives_nothing("bt-ui 2\n1 select -\n", "bt-interface 1\n23 ORDERS_CANCEL unit 1 type 38\n"))
        self.assertTrue(reference.ui_gives_nothing("bt-ui 2\n1 select -\n", "bt-interface 1\n"))

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
        log = "bt-ui 2\n1 select -\n20 input mouse left down 384 240\n"
        differing, lines = self.report({"ui-click": log}, ["ui-click"], {"ui-click": log})
        self.assertEqual(differing, ["ui-click"])
        self.assertIn("the input gives no command and selects no unit", lines[0])
        self.assertEqual(self.report({"other": log}, ["other"], {"other": log})[0], [])

    def test_a_stored_log_with_no_scenario_fails(self):
        differing, lines = self.report({"a": self.LOG, "gone": self.LOG}, ["a"], {"a": self.LOG}, {"a"})
        self.assertEqual(differing, ["gone"])
        self.assertIn("DIFF gone stored UI log has no scenario", lines[0])

    def test_a_subset_does_not_flag_logs_of_scenarios_left_out(self):
        differing, _ = self.report({"a": self.LOG, "b": self.LOG}, ["a"], {"a": self.LOG}, {"a", "b"})
        self.assertEqual(differing, [])

    def test_a_ui_scenario_whose_only_effect_is_an_interface_command_passes(self):
        log = "bt-ui 2\n1 select -\n20 input mouse left down 384 240\n"
        interface = "bt-interface 1\n23 ORDERS_CANCEL unit 1 type 38\n"
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp) / "ui"
            directory.mkdir()
            (directory / "ui-click.txt").write_text(log)
            with mock.patch.object(reference, "ROOT", directory.parent):
                differing, _ = reference.ui_report({"ui-click": Path("ui-click")}, {"ui-click": log}, directory, {"ui-click"},
                                                   {"ui-click": interface})
        self.assertEqual(differing, [])


class InterfaceLogTest(unittest.TestCase):
    SCHEMA = json.loads((Path(__file__).resolve().parent.parent / "data/trace_schema.json").read_text())
    OFFSET, STRIDE = 3315576, 280
    UNIT_1 = OFFSET + STRIDE
    UNIT_252 = OFFSET + 252 * STRIDE

    def group(self, target: int = 0, unit: int = UNIT_1) -> str:
        units = struct.pack("<16I", 1, *([0] * 15))
        payload = struct.pack("<7I", 297, unit, 3, 0, 0, target, 1) + struct.pack("<3i", 0x2860000, 0x10000, 0x1360000)
        return (payload + bytes(8) + units).hex()

    def commands(self, *lines: str) -> str:
        return "\n".join(["bt-commands 3", f"units {self.OFFSET} {self.STRIDE}", *lines, "end 40"]) + "\n"

    def test_group_command_without_a_target(self):
        text = self.commands(f"23 interface - 1 ORDERS_GROUP_COMMAND {self.group()}")
        lines = interfacelog.decode(text, self.SCHEMA).splitlines()
        self.assertEqual(lines[0], "bt-interface 1")
        self.assertEqual(lines[1], "23 ORDERS_GROUP_COMMAND unit 1 mode 3 type 0 queue 0 target - hasPos 1 pos "
                         "000086020000010000003601 params 0000000000000000 units 01000000" + "00" * 60)

    def test_group_command_with_a_target_unit(self):
        text = self.commands(f"23 interface - 1 ORDERS_GROUP_COMMAND {self.group(self.UNIT_252)}")
        self.assertIn(" target 252 ", interfacelog.decode(text, self.SCHEMA))

    def test_refs_that_are_no_unit_decode_as_heap(self):
        for value in (self.OFFSET - 4, self.OFFSET + 5, self.OFFSET + 65536 * self.STRIDE):
            text = self.commands(f"23 interface - 1 ORDERS_GROUP_COMMAND {self.group(value)}")
            self.assertIn(" target heap ", interfacelog.decode(text, self.SCHEMA))

    def test_without_a_units_line_every_ref_is_heap(self):
        text = f"bt-commands 3\n23 interface - 1 ORDERS_GROUP_COMMAND {self.group()}\nend 22\n"
        self.assertIn("unit heap mode 3", interfacelog.decode(text, self.SCHEMA))

    def test_queue_build_and_cancel(self):
        name = b"ARMPW".ljust(32, b"\0")
        build = struct.pack("<3I", 289, self.UNIT_1, 2) + name
        cancel = struct.pack("<3I", 296, self.UNIT_1, 38)
        text = self.commands(f"30 interface - 1 ORDERS_QUEUE_BUILD {build.hex()}", f"31 interface - 1 ORDERS_CANCEL {cancel.hex()}")
        self.assertEqual(interfacelog.decode(text, self.SCHEMA).splitlines()[1:],
                         [f"30 ORDERS_QUEUE_BUILD unit 1 count 2 name {name.hex()}", "31 ORDERS_CANCEL unit 1 type 38"])

    def test_unknown_kind_keeps_its_payload(self):
        text = self.commands("9 interface - 1 KIND_448 c0010000aabb")
        self.assertEqual(interfacelog.decode(text, self.SCHEMA), "bt-interface 1\n9 KIND_448 payload c0010000aabb\n")

    def test_scripted_lines_are_ignored(self):
        text = self.commands("1 spawn 12 1 UNITS_CREATE 8b010000", "2 order 14 1", "3 console 15 1")
        self.assertEqual(interfacelog.decode(text, self.SCHEMA), "bt-interface 1\n")

    def test_bad_input_raises(self):
        with self.assertRaises(ValueError):
            interfacelog.decode("bt-commands 1\n", self.SCHEMA)
        with self.assertRaises(ValueError):
            interfacelog.decode("", self.SCHEMA)
        with self.assertRaises(ValueError):
            interfacelog.decode(self.commands("23 interface - 1 ORDERS_CANCEL zz"), self.SCHEMA)
        with self.assertRaises(ValueError):
            interfacelog.decode(self.commands("23 interface"), self.SCHEMA)

    def test_the_end_line_is_the_last_and_holds_a_tick(self):
        with self.assertRaises(ValueError):
            interfacelog.decode("bt-commands 3\nunits 1 2\n", self.SCHEMA)
        with self.assertRaises(ValueError):
            interfacelog.decode(self.commands("end 39"), self.SCHEMA)
        with self.assertRaises(ValueError):
            interfacelog.decode("bt-commands 3\nunits 1 2\nend x\n", self.SCHEMA)
        self.assertEqual(interfacelog.decode("bt-commands 3\nunits 1 2\nend 0\n", self.SCHEMA), "bt-interface 1\n")

    def test_has_input(self):
        self.assertTrue(interfacelog.has_input("ticks 10\ninput 20 mouse left down 1 2\n"))
        self.assertFalse(interfacelog.has_input("ticks 10\nspawn 1 ARMCOM\n# input\ninputs\n"))

    def test_a_click_moved_a_pixel_across_a_units_edge_changes_the_command(self):
        miss = interfacelog.decode(self.commands(f"23 interface - 1 ORDERS_GROUP_COMMAND {self.group()}"), self.SCHEMA)
        hit = interfacelog.decode(self.commands(f"23 interface - 1 ORDERS_GROUP_COMMAND {self.group(self.UNIT_252)}"), self.SCHEMA)
        difference = reference.interface_difference(miss, hit)
        self.assertIn("interface line 2", difference)
        self.assertIn("target - ", difference)
        self.assertIn("target 252 ", difference)

    def report(self, files: dict[str, str], scenarios: dict[str, str], logs: dict[str, str | None], known: set[str] | None = None):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp) / "commands"
            directory.mkdir()
            for name, text in files.items():
                (directory / f"{name}.txt").write_text(text)
            with mock.patch.object(reference, "ROOT", directory.parent):
                return reference.interface_report({n: Path(n) for n in scenarios}, logs, directory,
                                                  set(scenarios) if known is None else known, scenarios)

    DECODED = "bt-interface 1\n23 ORDERS_CANCEL unit 1 type 38\n"
    WITH_INPUT = "ticks 100\ninput 20 mouse left down 1 2\n"

    def test_report_equal(self):
        differing, lines = self.report({"a": self.DECODED}, {"a": self.WITH_INPUT, "b": "ticks 100\n"},
                                       {"a": self.DECODED, "b": "bt-interface 1\n"})
        self.assertEqual(differing, [])
        self.assertEqual(lines, ["interface commands: 1 of 1 equal, 0 differ"])

    def test_report_names_the_first_differing_line(self):
        differing, lines = self.report({"a": self.DECODED}, {"a": self.WITH_INPUT}, {"a": self.DECODED.replace("38", "39")})
        self.assertEqual(differing, ["a"])
        self.assertIn("DIFF a interface line 2: stored", lines[0])

    def test_report_run_without_commands_file(self):
        differing, lines = self.report({"a": self.DECODED}, {"a": self.WITH_INPUT}, {"a": None})
        self.assertEqual(differing, ["a"])
        self.assertIn("the run wrote no commands.txt", lines[0])

    def test_report_missing_stored_file_for_an_input_scenario(self):
        differing, lines = self.report({}, {"a": self.WITH_INPUT}, {"a": self.DECODED})
        self.assertEqual(differing, ["a"])
        self.assertIn("DIFF a has no stored interface commands in", lines[0])

    def test_report_scenario_without_input_sending_commands(self):
        differing, lines = self.report({}, {"b": "ticks 100\n"}, {"b": self.DECODED})
        self.assertEqual(differing, ["b"])
        self.assertEqual(lines[0], "DIFF b the interface sent commands without input lines: 23 ORDERS_CANCEL unit 1 type 38")

    def test_report_stored_file_for_a_scenario_without_input(self):
        differing, _ = self.report({"b": "bt-interface 1\n"}, {"b": "ticks 100\n"}, {"b": "bt-interface 1\n"})
        self.assertEqual(differing, ["b"])

    def test_report_stored_file_with_no_scenario(self):
        differing, lines = self.report({"a": self.DECODED, "gone": self.DECODED}, {"a": self.WITH_INPUT}, {"a": self.DECODED}, {"a"})
        self.assertEqual(differing, ["gone"])
        self.assertIn("DIFF gone stored interface commands have no scenario of that name", lines[0])


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


class ViewReportTest(unittest.TestCase):
    @staticmethod
    def view(tick: int = 30, metal: float = 1.0) -> bytes:
        import test_viewblock as tv
        return reference.viewblock.FILE_HEADER + tv.header(tick=tick) + tv.players(metal)

    def report(self, stored: dict[int, bytes], got: dict[int, bytes]):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            scenario = root / "a.txt"
            scenario.write_text("ticks 100\nframes 30 60\n")
            plain = root / "b.txt"
            plain.write_text("ticks 100\n")
            for tick, data in stored.items():
                (root / "views" / "a").mkdir(parents=True, exist_ok=True)
                (root / "views" / "a" / f"{tick}.bin").write_bytes(data)
            with mock.patch.object(reference, "ROOT", root):
                return reference.view_report({"a": scenario, "b": plain}, {"a": got}, root / "views")

    def test_report_equal(self):
        views = {30: self.view(30), 60: self.view(60)}
        differing, lines = self.report(views, views)
        self.assertEqual((differing, lines), ([], ["views: 1 of 1 scenarios equal, 0 differ, 1 with no frames line"]))

    def test_report_names_the_tick_and_field(self):
        differing, lines = self.report({30: self.view(30), 60: self.view(60)}, {30: self.view(30), 60: self.view(60, metal=2.0)})
        self.assertEqual(differing, ["a"])
        self.assertEqual(lines[0], "DIFF a view 60: players[0].metal: stored 1, got 2")

    def test_missing_stored_view_and_missing_run_view_fail(self):
        differing, lines = self.report({30: self.view(30)}, {30: self.view(30), 60: self.view(60)})
        self.assertEqual(differing, ["a"])
        self.assertIn("view 60: no stored view", lines[0])
        differing, lines = self.report({30: self.view(30), 60: self.view(60)}, {30: self.view(30)})
        self.assertIn("view 60: the run wrote no view", lines[0])

    def test_a_file_that_is_not_a_view_is_named(self):
        differing, lines = self.report({30: b"BTFRxxxxxxxx", 60: self.view(60)}, {30: self.view(30), 60: self.view(60)})
        self.assertIn("view 30: not a view file", lines[0])

    def test_the_branch_views_replace_the_stored_ones(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "branch/a").mkdir(parents=True)
            (root / "branch/a/30.bin").write_bytes(b"new")
            (root / "reference/views/old").mkdir(parents=True)
            (root / "reference/views/old/1.bin").write_bytes(b"old")
            with mock.patch.object(reference, "ROOT", root), mock.patch.object(reference, "VIEWS", root / "reference/views"), \
                    contextlib.redirect_stdout(io.StringIO()) as out:
                reference.copy_views(root / "branch")
            self.assertEqual([p.relative_to(root / "reference/views").as_posix() for p in (root / "reference/views").rglob("*.bin")], ["a/30.bin"])
            self.assertIn("1 views copied", out.getvalue())

    def test_the_views_are_in_the_gate_key(self):
        self.assertIn("reference/views", reference.KEY_TREES)


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


class SetupLogTest(unittest.TestCase):
    SETUP = "# bt-setup 1\nmap Coast To Coast\nplayer0.controller human\nplayer1.controller ai\n"

    def test_only_frontend_scenarios_are_compared_and_each_needs_a_stored_log(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            directory = root / "setup"
            directory.mkdir()
            menu, unstored, plain = (root / n for n in ("menu.txt", "unstored.txt", "plain.txt"))
            menu.write_text("frontend 1\nticks 10\n")
            unstored.write_text("frontend 1\nticks 10\n")
            plain.write_text("ticks 10\nmap The Pass\n")
            (directory / "menu.txt").write_text(self.SETUP)
            scenarios = {"menu": menu, "unstored": unstored, "plain": plain}
            with mock.patch.object(reference, "ROOT", root):
                differing, lines = reference.setup_report(scenarios, {"menu": self.SETUP, "unstored": self.SETUP, "plain": "x"}, directory)
                self.assertEqual(differing, ["unstored"])
                self.assertIn("setup logs: 1 of 2 equal", lines[-1])
                differing, _ = reference.setup_report(scenarios, {"menu": self.SETUP.replace("ai", "human"), "unstored": None}, directory)
                self.assertEqual(differing, ["menu", "unstored"])

    def test_the_readback_is_the_setup_log_with_the_seeds_for_one_tick(self):
        with tempfile.TemporaryDirectory() as tmp:
            scenario = Path(tmp) / "menu.txt"
            scenario.write_text("# menus\nfrontend 1\ngame_seed 5\ncrt_seed 6\nview_seed 7\nticks 300\ninput 0.20 mouse move 1 2\n")
            text = reference.readback_scenario(scenario, self.SETUP)
        self.assertEqual(text, self.SETUP + "game_seed 5\ncrt_seed 6\nview_seed 7\nticks 1\n")

    def test_copy_setup_replaces_the_stored_logs(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "branch/reference/setup").mkdir(parents=True)
            (root / "branch/reference/setup/a.txt").write_text(self.SETUP)
            (root / "reference/setup").mkdir(parents=True)
            (root / "reference/setup/old.txt").write_text("old")
            with mock.patch.object(reference, "ROOT", root), mock.patch.object(reference, "SETUPS", root / "reference/setup"), \
                    contextlib.redirect_stdout(io.StringIO()) as out:
                reference.copy_setup(root / "branch")
            self.assertEqual(sorted(p.name for p in (root / "reference/setup").iterdir()), ["a.txt"])
            self.assertIn("1 setup logs copied", out.getvalue())


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
                    mock.patch.object(reference, "ui_report", lambda *a, **k: ([], [])), \
                    mock.patch.object(reference, "interface_report", lambda *a, **k: ([], [])), \
                    mock.patch.object(reference, "events_report", lambda *a: ([], [])), \
                    mock.patch.object(reference, "display_report", lambda *a: ([], [])), \
                    mock.patch.object(reference, "display_list_report", lambda *a: ([], [])), \
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
                (d / "trace.txt").write_text("bt-trace 10\n" + "\n".join(rows) + "\n")
                return d

            quirk_rows = [(1, "units", "1" * 16), (1, "tick", "4" * 16), (1, "view", "7" * 16), (2, "units", "2" * 16), (2, "players", "9" * 16),
                          (2, "tick", "5" * 16), (2, "view", "7" * 16)]
            off_rows = [(1, "units", "1" * 16), (1, "tick", "4" * 16), (1, "view", "7" * 16), (2, "units", "2" * 16), (2, "players", "3" * 16),
                        (2, "tick", "5" * 16), (2, "view", "7" * 16)]

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


class EarlyExitTest(unittest.TestCase):
    HEADER = "bt-trace 10\ngame_seed 1\ncrt_seed 1\nview_seed 1\n"

    @staticmethod
    def rows(first, last):
        return "".join(f"{tick} tick {tick:016x}\n{tick} view {tick:016x}\n" for tick in range(first, last + 1))

    def stored(self, ticks):
        header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
        rows = [(tick, phase, f"{tick:016x}") for tick in range(1, ticks + 1) for phase in ("tick", "view")]
        return header, reference.tracedigest.digests(rows)

    def play(self, trace_text, ticks=5, result=None, wine_log=None, want=True):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            run = root / "play/run"
            run.mkdir(parents=True)
            (run / "trace.txt").write_text(trace_text)
            if result is not None:
                (run / "result.json").write_text(json.dumps(result))
            if wine_log is not None:
                (run / "wine.log").write_text(wine_log)
            (root / "s.txt").write_text(f"ticks {ticks}\n")
            with mock.patch.object(reference, "ROOT", root), \
                    mock.patch.object(reference.tracediff, "play", lambda *a, **k: run), \
                    mock.patch.dict(reference.SOUND_LOGS), mock.patch.dict(reference.UI_LOGS), mock.patch.dict(reference.FUEL), \
                    contextlib.redirect_stdout(io.StringIO()):
                try:
                    out = reference.play_scenario(Path("x.exe"), root / "s.txt", None, self.stored(ticks) if want else None, mode="wasm")
                except reference.EarlyExit as e:
                    kept = list((root / "build/scratch").glob("check-s-wasm-*"))
                    return e, sorted(p.name for p in kept[0].iterdir()) if kept else []
                return out, []

    def test_a_cut_trace_is_an_early_exit_with_its_exit_code_and_host_message(self):
        trace = self.HEADER + self.rows(1, 3) + "4 tick 00000"
        early, kept = self.play(trace, result={"exit_codes": [2], "reason": ""}, wine_log="noise\nmodule trapped at tick 4: out of fuel\n")
        self.assertIsInstance(early, reference.EarlyExit)
        text = str(early)
        self.assertRegex(text, r"^EARLY s \[wasm\]: ended at tick 3 of 5 \(exit code 2\), host said wine.log: module trapped at tick 4: out of fuel")
        self.assertRegex(text, r"\[kept: build/scratch/check-s-wasm-[\d-]+\]$")
        self.assertEqual(kept, ["result.json", "trace.txt", "wine.log"])

    def test_a_trace_that_stops_at_a_line_end_short_of_the_count_is_early(self):
        early, _ = self.play(self.HEADER + self.rows(1, 3), result={"exit_codes": [-9], "reason": ""})
        self.assertIn("ended at tick 3 of 5 (signal SIGKILL)", str(early))

    def test_a_timeout_and_a_missing_result_are_named(self):
        early, _ = self.play(self.HEADER + self.rows(1, 2), result={"exit_codes": [None], "reason": "the game still runs after 900s"})
        self.assertIn("(timeout)", str(early))
        early, _ = self.play(self.HEADER + self.rows(1, 2))
        self.assertIn("(exit status unknown)", str(early))

    def test_a_cut_line_in_the_next_tick_leaves_the_last_tick_whole(self):
        self.assertEqual(reference.trace_end(self.write(self.HEADER + self.rows(1, 3) + "4 un")), (3, True))
        self.assertEqual(reference.trace_end(self.write(self.HEADER + self.rows(1, 3) + "3 tick 00")), (2, True))
        self.assertEqual(reference.trace_end(self.write(self.HEADER + self.rows(1, 3))), (3, False))

    def write(self, text):
        handle = tempfile.NamedTemporaryFile("w", delete=False)
        self.addCleanup(os.unlink, handle.name)
        handle.write(text)
        handle.close()
        return Path(handle.name)

    def test_a_full_length_trace_with_a_different_digest_is_a_difference_not_an_early_exit(self):
        trace = self.HEADER + self.rows(1, 4) + "5 tick " + "f" * 16 + "\n5 view " + "5" * 16 + "\n"
        (header, digests, _, kept), _ = self.play(trace)
        self.assertEqual(len(digests), 5)
        self.assertIn("tick 5", reference.difference(self.stored(5), header, digests))
        self.assertIsNotNone(kept)

    def test_a_trace_that_ends_exactly_at_the_count_is_not_early(self):
        (header, digests, _, kept), _ = self.play(self.HEADER + self.rows(1, 5))
        self.assertEqual(len(digests), 5)
        self.assertIsNone(reference.difference(self.stored(5), header, digests))
        self.assertIsNone(kept)

    def test_a_scenario_that_ends_by_design_before_its_ticks_is_not_early(self):
        (_, digests, _, _), _ = self.play(self.HEADER + self.rows(1, 3), ticks=100, want=False)
        self.assertEqual(len(digests), 3)

    def test_the_stored_length_not_the_tick_count_is_the_end(self):
        stored = self.stored(3)
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            run = root / "play/run"
            run.mkdir(parents=True)
            (run / "trace.txt").write_text(self.HEADER + self.rows(1, 3))
            (root / "s.txt").write_text("ticks 100\n")
            with mock.patch.object(reference, "ROOT", root), \
                    mock.patch.object(reference.tracediff, "play", lambda *a, **k: run), \
                    mock.patch.dict(reference.SOUND_LOGS), mock.patch.dict(reference.UI_LOGS), mock.patch.dict(reference.FUEL), \
                    mock.patch.dict(reference.DISPLAY_LOGS), mock.patch.dict(reference.DISPLAY_LISTS):
                _, digests, _, kept = reference.play_scenario(Path("x.exe"), root / "s.txt", None, stored)
        self.assertEqual(len(digests), 3)
        self.assertIsNone(kept)

    def test_a_failed_run_with_a_short_trace_is_an_early_exit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            run = root / "build/playtest/20260101-000000-1/run"
            run.mkdir(parents=True)
            (run / "trace.txt").write_text(self.HEADER + self.rows(1, 2))
            (run / "result.json").write_text(json.dumps({"exit_codes": [-11], "reason": "crash"}))
            message = "playtest.py failed for x.exe (exit 1):\n\nlogs and screenshots: build/playtest/20260101-000000-1/\n"
            (root / "s.txt").write_text("ticks 5\n")

            def fake_play(*a, **k):
                raise reference.tracediff.Error(message)

            with mock.patch.object(reference, "ROOT", root), mock.patch.object(reference.tracediff, "play", fake_play), \
                    contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(reference.EarlyExit) as raised:
                    reference.play_scenario(Path("x.exe"), root / "s.txt", None, self.stored(5), retry_any=False)
            self.assertIn("ended at tick 2 of 5 (signal SIGSEGV)", str(raised.exception))

    def test_an_early_exit_halts_the_watch_and_skips_tracediff(self):
        watch = reference.Watch({})
        watch.halt("s [wasm]", "EARLY s [wasm]: ended at tick 3 of 5 (exit code 2)", 3, 1.0, None, early=True)
        self.assertTrue(watch.found.early)


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


class ReplayTest(unittest.TestCase):
    HEADER = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
    ROWS = [(1, "tick", "1" * 16), (1, "view", "3" * 16), (2, "tick", "2" * 16), (2, "view", "4" * 16)]

    def scenarios(self, tmp: Path) -> dict[str, Path]:
        texts = {"clicks": "ticks 9\ninput 2 char a\n", "plain": "ticks 50\n"}
        for name, text in texts.items():
            (tmp / f"{name}.txt").write_text(text)
        return {name: tmp / f"{name}.txt" for name in texts}

    def test_names(self):
        self.assertEqual(reference.replay_name("ui-keys"), "ui-keys+commands")
        self.assertEqual(reference.replay_source("ui-keys+commands"), "ui-keys")
        self.assertIsNone(reference.replay_source("ui-keys"))
        self.assertEqual(reference.untag(reference.tag("ui-keys+commands", "wasm")), "ui-keys+commands")

    def test_only_a_scenario_with_input_lines_is_replayed(self):
        with tempfile.TemporaryDirectory() as tmp:
            scenarios = self.scenarios(Path(tmp))
            self.assertEqual(reference.with_replays(scenarios), {"clicks+commands": scenarios["clicks"]})

    def test_a_scenario_with_menus_is_not_replayed(self):
        with tempfile.TemporaryDirectory() as tmp:
            scenarios = self.scenarios(Path(tmp))
            menus = Path(tmp) / "menus.txt"
            menus.write_text("frontend 1\nticks 9\ninput 2 char a\n")
            self.assertTrue(reference.interfacelog.has_input(menus.read_text()))
            self.assertEqual(reference.with_replays({**scenarios, "menus": menus}), {"clicks+commands": scenarios["clicks"]})

    def play(self, scenarios, writes="bt-commands 3\nunits 1 2\nend 0\n", fail=(), modes=(None,)):
        calls = []
        lock = threading.Lock()

        def fake_play_scenario(exe, scenario, game_dir, want=None, poison=False, keep_always=False, scans=None,
                               scan_every=1, retry_any=False, save_to=None, load_from=None, mode=None, watch=None,
                               name=None, testcommands=None):
            key = reference.tag(name or scenario.stem, mode)
            with lock:
                calls.append((key, want, watch, testcommands.read_text() if testcommands else None))
            if scenario.stem in fail and name is None:
                raise reference.tracediff.Error("boom")
            if name is None and writes is not None:
                reference.COMMAND_TEXTS[key] = writes
            return {}, [], 0.0, None

        failed: dict[str, str] = {}
        played = {**scenarios, **reference.with_replays(scenarios)}
        expected = {name: ("want", name) for name in played}
        with mock.patch.object(reference.playtest, "template_prefix"), \
                mock.patch.object(reference, "COMMAND_TEXTS", {}), \
                mock.patch.object(reference, "play_scenario", fake_play_scenario):
            reference.play_all(Path("x.exe"), played, None, 3, expected, failed=failed,
                               **({"modes": modes} if modes != (None,) else {}))
        return calls, failed

    def test_a_replay_plays_its_own_runs_commands_after_it(self):
        for modes in ((None,), reference.MODES):
            with tempfile.TemporaryDirectory() as tmp, mock.patch.object(reference, "REPLAY_LEFT_OUT", {}):
                calls, failed = self.play(self.scenarios(Path(tmp)), modes=modes)
            self.assertEqual(failed, {})
            keys = [c[0] for c in calls]
            for mode in modes:
                replay = reference.tag("clicks+commands", mode)
                self.assertLess(keys.index(reference.tag("clicks", mode)), keys.index(replay))
                key, want, _, played = calls[keys.index(replay)]
                self.assertEqual(want, ("want", "clicks+commands"))
                self.assertEqual(played, "bt-commands 3\nunits 1 2\nend 0\n")
            self.assertEqual(len(calls), 3 * len(modes))

    def test_a_replay_fails_without_running_when_its_scenario_failed_or_wrote_no_file(self):
        for kwargs in ({"fail": ("clicks",)}, {"writes": None}):
            with tempfile.TemporaryDirectory() as tmp:
                calls, failed = self.play(self.scenarios(Path(tmp)), **kwargs)
            self.assertNotIn("clicks+commands", [c[0] for c in calls])
            self.assertIn("clicks+commands: clicks failed to run or wrote no commands.txt", failed["clicks+commands"])

    def test_a_listed_replay_is_played_without_its_traces_or_the_watch(self):
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(reference, "REPLAY_LEFT_OUT", {"clicks": "why"}):
            scenarios = self.scenarios(Path(tmp))
            watch = reference.Watch({})
            calls = []

            def fake_play_scenario(exe, scenario, game_dir, want=None, *args, watch=None, name=None, **kw):
                calls.append((name or scenario.stem, want, watch))
                reference.COMMAND_TEXTS[scenario.stem] = "bt-commands 3\nend 0\n"
                return {}, [], 0.0, None

            played = {**scenarios, **reference.with_replays(scenarios)}
            with mock.patch.object(reference.playtest, "template_prefix"), \
                    mock.patch.object(reference, "COMMAND_TEXTS", {}), \
                    mock.patch.object(reference, "play_scenario", fake_play_scenario):
                reference.play_all(Path("x.exe"), played, None, 1, {n: ("want", n) for n in played}, watch=watch)
        self.assertIn(("clicks+commands", None, None), calls)
        self.assertIn(("clicks", ("want", "clicks"), watch), calls)

    def report(self, digests, left_out):
        results = {reference.tag("clicks+commands", mode): (self.HEADER, digests, 1.0, None) for mode in reference.MODES}
        expected = {"clicks+commands": (self.HEADER, reference.tracedigest.digests(self.ROWS))}
        with mock.patch.object(reference, "REPLAY_LEFT_OUT", left_out):
            return reference.replay_report({"clicks+commands": Path("clicks.txt")}, reference.MODES, results, expected)

    def test_report(self):
        same = reference.tracedigest.digests(self.ROWS)
        other = reference.tracedigest.digests(self.ROWS[:2] + [(2, "tick", "9" * 16), self.ROWS[3]])
        bad, stale, lines = self.report(same, {})
        self.assertEqual((bad, stale), ([], []))
        self.assertRegex(lines[0], r"^ok   clicks\+commands \[wasm\] \(2 ticks")
        self.assertEqual(lines[-1], "2 of 2 runs from commands.txt equal, 0 left out (REPLAY_LEFT_OUT)")
        bad, stale, lines = self.report(other, {})
        self.assertEqual(bad, ["clicks+commands [wasm]", "clicks+commands [native]"])
        self.assertRegex(lines[0], r"^DIFF clicks\+commands \[wasm\] .* tick 2: stored")
        bad, stale, lines = self.report(other, {"clicks": "a field (#1)"})
        self.assertEqual((bad, stale), ([], []))
        self.assertRegex(lines[0], r"^left clicks\+commands \[wasm\] .* tick 2: .*: a field \(#1\)$")
        self.assertEqual(lines[-1], "0 of 2 runs from commands.txt equal, 2 left out (REPLAY_LEFT_OUT)")
        bad, stale, lines = self.report(same, {"clicks": "a field (#1)"})
        self.assertEqual((bad, stale), ([], ["clicks+commands [wasm]", "clicks+commands [native]"]))
        self.assertIn("equal, but REPLAY_LEFT_OUT lists it: remove it", lines[0])

    def test_every_listed_scenario_has_input_lines(self):
        scenarios = reference.scenario_files()
        for name in reference.REPLAY_LEFT_OUT:
            self.assertIn(name, scenarios)
            self.assertTrue(interfacelog.has_input(scenarios[name].read_text()), name)


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

    def test_block_pointers_are_checked_on_the_tier(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out), mock.patch.object(reference.blockpointers, "check_scans", return_value=[]) as check:
            self.assertEqual(reference.report_block_pointers(Path("TotalA.exe"), {"a": "text"}, None), 0)
        check.assert_called_once()
        self.assertEqual(out.getvalue().strip(), "block pointers: all covered by data/block_pointers.csv (1 scenarios)")

    def test_a_scan_row_the_csv_does_not_cover_fails(self):
        out = io.StringIO()
        missing = ["a: holder not listed: 0x1234 at 0x4 -> T"]
        with contextlib.redirect_stdout(out), mock.patch.object(reference.blockpointers, "check_scans", return_value=missing):
            self.assertEqual(reference.report_block_pointers(Path("TotalA.exe"), {"a": "text"}, None), 1)
        self.assertIn("BLOCK POINTER a: holder not listed", out.getvalue())

    def test_a_scenario_with_no_scan_fails(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            self.assertEqual(reference.report_block_pointers(Path("TotalA.exe"), {"a": None}, None), 1)
        self.assertIn("a: no blockptrs.txt", out.getvalue())


class GameTraceTest(unittest.TestCase):
    START = 1_800_000_000.0

    def name(self, offset: float, pid: int = 77) -> str:
        t = datetime.datetime.fromtimestamp(self.START + offset)
        return t.strftime("%Y%m%d-%H%M%S") + f"-{pid}-smoke"

    def locate(self, *offsets: float) -> Path | None:
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            for o in offsets:
                (out / "work" / self.name(o)).mkdir(parents=True)
            with mock.patch.object(reference.playtest, "OUT", out):
                found = reference.game_trace(77, self.START)
            return found.relative_to(out / "work") if found else None

    def test_stale_directory_is_ignored(self):
        self.assertIsNone(self.locate(-60))

    def test_own_directory_is_found(self):
        self.assertEqual(self.locate(3), Path(self.name(3)) / "i1/game/trace.txt")
        self.assertEqual(self.locate(0), Path(self.name(0)) / "i1/game/trace.txt")

    def test_new_directory_wins_over_stale(self):
        self.assertEqual(self.locate(-60, 2), Path(self.name(2)) / "i1/game/trace.txt")

    def test_other_pid_is_ignored(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            (out / "work" / self.name(1, pid=78)).mkdir(parents=True)
            with mock.patch.object(reference.playtest, "OUT", out):
                self.assertIsNone(reference.game_trace(77, self.START))


class CheckStopsAtFirstDifferenceTest(unittest.TestCase):
    NAMES = ("a", "b", "c")

    def test_the_other_runs_are_stopped_and_the_report_names_the_tick(self):
        header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
        good = [row for t in "1234" for row in ((int(t), "tick", t * 16), (int(t), "view", "7" * 16))]
        bad = good[:2] + [(2, "tick", "9" * 16), (2, "view", "7" * 16)] + good[4:]
        stored = (header, reference.tracedigest.digests(good))
        procs, paths, calls, reported = {}, {}, [], []

        def rows(items):
            return "bt-trace 10\n" + "".join(f"{t} {ph} {h}\n" for t, ph, h in items)

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
                    trace.write_text(rows(bad) + "3 tick 33")
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
                    mock.patch.object(reference, "game_trace", lambda pid, started: paths.get(pid)), \
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
        text = b"bt-trace 10\nseed 1\n1 tick " + b"1" * 16 + b"\n1 view " + b"7" * 16 + b"\n2 tick " + b"2" * 16 + b"\n2 view " + b"7" * 16 + b"\n3 ti"
        self.assertEqual(len(tail.feed(text)), 1)
        self.assertEqual(tail.feed(b"ck " + b"3" * 16 + b"\n"), [reference.tracedigest.digests([(2, "tick", "2" * 16), (2, "view", "7" * 16)])[0]])


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
            with mock.patch.object(os, "cpu_count", return_value=64):
                self.assertEqual(reference.gate_slots(), reference.GATE_SLOTS)
            with mock.patch.object(os, "cpu_count", return_value=8):
                self.assertEqual(reference.gate_slots(), 4)
            with mock.patch.object(os, "cpu_count", return_value=1):
                self.assertEqual(reference.gate_slots(), 1)


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
        good = [(1, "tick", "1" * 16), (1, "view", "7" * 16), (2, "tick", "2" * 16), (2, "view", "7" * 16)]
        expected = {"a": (header, reference.tracedigest.digests(good))}
        with tempfile.TemporaryDirectory() as tmp:
            trace = Path(tmp) / "trace.txt"
            trace.write_text("bt-trace 10\n" + "".join(f"{t} tick {v * 16}\n{t} view {'7' * 16}\n" for t, v in ((1, "1"), (2, "9"), (3, "3"))))
            proc = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
            watch = reference.Watch(expected, lambda pid, started: trace)
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
            (run / "trace.txt").write_text("bt-trace 10\ngame_seed 1\ncrt_seed 1\nview_seed 1\n1 tick " + "1" * 16 + "\n1 view " + "7" * 16 + "\n")
            (run / "blockptrs.txt").write_text("scan")
            (run / "sound.txt").write_text("header\n")
            (run / reference.UI_FILE).write_text("ui\n")
            (run / reference.DISPLAY_FILE).write_text("digest\n")
            (run / reference.DISPLAYLIST_FILE).write_text("list\n")
            scans = {}
            with mock.patch.object(reference, "scenario_frames", lambda s: []), \
                    mock.patch.object(reference, "scenario_saves", lambda s: False), \
                    mock.patch.object(reference, "scenario_ticks", lambda s: 1), \
                    mock.patch.object(reference.tracediff, "play", lambda *a, **k: run), \
                    mock.patch.dict(reference.SOUND_LOGS), mock.patch.dict(reference.UI_LOGS), mock.patch.dict(reference.FUEL), \
                    mock.patch.dict(reference.DISPLAY_LOGS), mock.patch.dict(reference.DISPLAY_LISTS):
                reference.play_scenario(Path("x.exe"), Path("s.txt"), None, scans=scans, mode="native")
                self.assertEqual(scans, {"s [native]": "scan"})
                self.assertEqual(reference.SOUND_LOGS["s [native]"], "header\n")
                self.assertEqual(reference.UI_LOGS["s [native]"], "ui\n")
                self.assertEqual(reference.DISPLAY_LOGS["s [native]"], "digest\n")
                self.assertEqual(reference.DISPLAY_LISTS["s [native]"], "list\n")
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


class RecheckTest(unittest.TestCase):
    HEADER = "bt-trace 10\ngame_seed 1\ncrt_seed 1\nview_seed 1\n"

    def rows(self, tick_values):
        return "".join(f"{t} tick {v * 16}\n{t} view {'7' * 16}\n" for t, v in tick_values)

    def found(self, tmp, kept_text, watch_text):
        """Run a Watch over `watch_text` (the live trace), keeping a trace.txt of `kept_text` in place of the real copy."""
        good = reference.tracedigest.digests([(t, phase, h) for t, v in ((1, "1"), (2, "2"), (3, "3")) for phase, h in (("tick", v * 16), ("view", "7" * 16))])
        live = Path(tmp) / "work/i1/game/trace.txt"
        live.parent.mkdir(parents=True)
        live.write_text(self.HEADER + watch_text)
        kept = Path(tmp) / "kept"
        kept.mkdir()
        (kept / "trace.txt").write_text(self.HEADER + kept_text)
        proc = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
        watch = reference.Watch({"a": ({}, good)}, lambda pid, started: live)
        try:
            with mock.patch.object(reference, "keep_run", lambda *a, **k: kept):
                watch.started("a")(proc)
                watch.poll()
        finally:
            proc.kill()
            proc.wait()
        return watch.found, live

    def test_the_file_holding_the_stored_digest_says_the_watch_read_another_value(self):
        with tempfile.TemporaryDirectory() as tmp:
            found, live = self.found(tmp, self.rows([(1, "1"), (2, "2"), (3, "3")]), self.rows([(1, "1"), (2, "9"), (3, "3")]))
            self.assertEqual((found.tick, found.trace), (2, live))
            self.assertNotEqual(found.stored, found.got)
            note = reference.recheck(found)
            self.assertIn("the watch read a different value than the file now holds", note)
            self.assertIn("tick 2", note)
            self.assertIn(found.stored, note)
            self.assertIn(found.got, note)
            self.assertIn(str(live), note)

    def test_the_file_holding_the_watchs_value_keeps_the_diff(self):
        with tempfile.TemporaryDirectory() as tmp:
            text = self.rows([(1, "1"), (2, "9"), (3, "3")])
            found, _ = self.found(tmp, text, text)
            self.assertIsNone(reference.recheck(found))

    def test_a_file_that_stops_short_of_the_tick_says_so(self):
        with tempfile.TemporaryDirectory() as tmp:
            found, _ = self.found(tmp, self.rows([(1, "1")]) + "2 tick " + "2" * 5, self.rows([(1, "1"), (2, "9"), (3, "3")]))
            self.assertIn("does not reach tick 2", reference.recheck(found))

    def test_a_run_whose_directory_is_gone_keeps_the_trace_the_watch_read(self):
        with tempfile.TemporaryDirectory() as tmp:
            good = reference.tracedigest.digests([(t, phase, h) for t, v in ((1, "1"), (2, "2"), (3, "3")) for phase, h in (("tick", v * 16), ("view", "7" * 16))])
            text = self.HEADER + self.rows([(1, "1"), (2, "9"), (3, "3")])
            live = Path(tmp) / "work/i1/game/trace.txt"
            live.parent.mkdir(parents=True)
            live.write_text(text)
            keep_run = reference.keep_run

            def gone(run, *a, **k):
                reference.shutil.rmtree(run)
                return keep_run(run, *a, **k)
            proc = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
            watch = reference.Watch({"a": ({}, good)}, lambda pid, started: live)
            try:
                with mock.patch.object(reference, "ROOT", Path(tmp)), mock.patch.object(reference, "keep_run", gone):
                    watch.started("a")(proc)
                    watch.poll()
            finally:
                proc.kill()
                proc.wait()
            found = watch.found
            self.assertFalse(live.parent.exists())
            self.assertEqual((found.kept / "trace.txt").read_text(), text)
            self.assertEqual(found.tick, 2)
            self.assertIn("tick 2", found.diff)
            self.assertIsNone(reference.recheck(found))

    def test_a_kept_directory_without_a_trace_says_nothing_was_kept(self):
        with tempfile.TemporaryDirectory() as tmp:
            found, _ = self.found(tmp, "", self.rows([(1, "1"), (2, "9"), (3, "3")]))
            (found.kept / "trace.txt").unlink()
            note = reference.recheck(found)
            self.assertIn("no trace.txt was kept", note)
            self.assertNotIn("cannot read", note)

    def test_the_diff_line_names_the_trace_the_watch_read(self):
        with tempfile.TemporaryDirectory() as tmp:
            found, live = self.found(tmp, self.rows([(1, "1")]), self.rows([(1, "1"), (2, "9"), (3, "3")]))
            self.assertEqual(reference.shown(found.trace), live)


class EndOfTickTest(unittest.TestCase):
    STORED = [(1, "d1", "s1", "v1"), (2, "d2", "s2", "v2"), (3, "d3", "s3", "v3")]
    NAMES = ("eq", "diff", "bad")

    def report(self, end_of_tick, got_by_name):
        """check_traces on three scenarios whose played digests are `got_by_name`; (exit status, output, the columns play_all was given)."""
        header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
        stored = (header, self.STORED)
        columns = []
        divergences = []

        def fake_play_all(exe, scenarios, game_dir, parallel, expected, poison, failed, keep_always=False, scans=None,
                          scan_every=1, watch=None, column=reference.DIGEST):
            columns.append(column)
            return {name: (header, got_by_name[name], 1.0, None) for name in scenarios}

        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "accesslog.attr").write_text("")
            files = {}
            for n in self.NAMES:
                files[n] = tmp / f"{n}.txt"
                files[n].write_text("ticks 2\n")
            (tmp / "traces").mkdir()
            for n in self.NAMES:
                (tmp / "traces" / f"{n}.txt").write_text("")
            out = io.StringIO()
            status = None
            with mock.patch.object(reference, "scenario_files", lambda: files), \
                    mock.patch.object(reference, "TRACES", tmp / "traces"), \
                    mock.patch.object(reference.tracedigest, "parse_digest", lambda p: stored), \
                    mock.patch.object(reference, "play_all", fake_play_all), \
                    mock.patch.object(reference, "report_fuel", lambda *a, **k: 0), \
                    mock.patch.multiple(reference, **{n: (lambda *a, **k: ([], [])) for n in
                                                      ("sound_report", "ui_report", "interface_report", "events_report", "display_report", "display_list_report", "frame_report", "view_report", "save_report")}), \
                    mock.patch.object(reference, "report_divergence", lambda *a: divergences.append(a) or (_ for _ in ()).throw(SystemExit(1))), \
                    contextlib.redirect_stdout(out):
                try:
                    reference.check_traces(tmp / "x.exe", None, 1, end_of_tick=end_of_tick)
                except SystemExit as e:
                    status = e.code
        self.divergences = divergences
        return status, out.getvalue(), columns

    def test_end_of_tick_marks_a_digest_only_difference_without_failing(self):
        digest_only = [(1, "d1", "s1", "v1"), (2, "x2", "s2", "v2"), (3, "d3", "s3", "v3")]
        status, out, columns = self.report(True, {"eq": self.STORED, "diff": digest_only, "bad": self.STORED})
        self.assertEqual(columns, [reference.STATE])
        self.assertIsNone(status)
        self.assertRegex(out, r"ok   diff \(3 ticks, [^)]*\) \[digest differs from tick 2\]\n")
        self.assertRegex(out, r"ok   eq \(3 ticks, [^)]*\)\n")
        self.assertRegex(out, r"3 of 3 scenarios equal, total [^\n]*\n2 of 3 scenarios equal on every row\n")

    def test_end_of_tick_marks_a_view_only_difference_without_failing(self):
        view_only = [(1, "d1", "s1", "v1"), (2, "d2", "s2", "w2"), (3, "d3", "s3", "v3")]
        status, out, _ = self.report(True, {"eq": self.STORED, "diff": view_only, "bad": self.STORED})
        self.assertIsNone(status)
        self.assertRegex(out, r"ok   diff \(3 ticks, [^)]*\) \[view differs from tick 2\]\n")

    def test_end_of_tick_fails_a_state_difference_and_runs_tracediff_on_it(self):
        state = [(1, "d1", "s1", "v1"), (2, "x2", "y2", "v2"), (3, "d3", "s3", "v3")]
        status, out, _ = self.report(True, {"eq": self.STORED, "diff": state, "bad": self.STORED})
        self.assertEqual(status, 1)
        self.assertRegex(out, r"DIFF diff \(3 ticks, [^)]*\) tick 2: stored s2, got y2 \[digest differs from tick 2\]\n")
        self.assertIn("2 of 3 scenarios equal, total", out)
        self.assertIn("2 of 3 scenarios equal on every row", out)
        self.assertEqual([(d[0], d[-1]) for d in self.divergences], [("diff", True)])

    def test_without_the_switch_the_digest_decides_and_nothing_is_added(self):
        digest_only = [(1, "d1", "s1", "v1"), (2, "x2", "s2", "v2"), (3, "d3", "s3", "v3")]
        status, out, columns = self.report(False, {"eq": self.STORED, "diff": digest_only, "bad": self.STORED})
        self.assertEqual(columns, [reference.DIGEST])
        self.assertEqual(status, 1)
        self.assertRegex(out, r"DIFF diff \(3 ticks, [^)]*\) tick 2: stored d2, got x2\n")
        self.assertNotIn("digest differs", out)
        self.assertNotIn("on every row", out)

    def test_the_digest_column_is_compared_by_default(self):
        got = [(1, "d1", "s1", "v1"), (2, "x2", "s2", "v2"), (3, "d3", "s3", "v3")]
        self.assertEqual(reference.first_difference(self.STORED, got), "tick 2: stored d2, got x2")
        self.assertEqual(reference.first_difference(self.STORED, got, reference.DIGEST), "tick 2: stored d2, got x2")

    def test_the_state_column_ignores_the_digest(self):
        got = [(1, "d1", "s1", "v1"), (2, "x2", "s2", "v2"), (3, "d3", "s3", "v3")]
        self.assertIsNone(reference.first_difference(self.STORED, got, reference.STATE))

    def test_the_state_column_reports_the_state(self):
        got = [(1, "d1", "s1", "v1"), (2, "x2", "s2", "v2"), (3, "x3", "y3", "v3")]
        self.assertEqual(reference.first_difference(self.STORED, got, reference.STATE), "tick 3: stored s3, got y3")

    def test_a_short_trace_differs_in_either_column(self):
        for column in (reference.DIGEST, reference.STATE):
            self.assertEqual(reference.first_difference(self.STORED, self.STORED[:2], column), "tick 3: only the stored trace has it")

    def test_difference_takes_the_column_and_still_checks_the_seeds(self):
        header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
        got = [(1, "d1", "s1", "v1"), (2, "x2", "s2", "v2"), (3, "d3", "s3", "v3")]
        self.assertIsNone(reference.difference((header, self.STORED), header, got, reference.STATE))
        self.assertIsNotNone(reference.difference((header, self.STORED), header, got))
        seeds = {**header, "game_seed": "2"}
        self.assertIn("seeds differ", reference.difference((header, self.STORED), seeds, self.STORED, reference.STATE))

    def test_the_note_names_the_first_tick_whose_digest_differs(self):
        got = [(1, "d1", "s1", "v1"), (2, "x2", "s2", "v2"), (3, "x3", "s3", "v3")]
        self.assertEqual(reference.digest_note(self.STORED, got), " [digest differs from tick 2]")
        self.assertEqual(reference.digest_note(self.STORED, self.STORED), "")
        self.assertEqual(reference.digest_note(self.STORED, self.STORED[:1]), " [digest differs from tick 2]")

    def test_the_every_row_line(self):
        self.assertEqual(reference.every_row_line(2, 3), "2 of 3 scenarios equal on every row")

    def test_the_tracediff_command_gets_the_switch(self):
        cmd = reference.tracediff_command(Path("a"), Path("b"), Path("s"), False, None, True)
        self.assertIn("--end-of-tick", cmd)
        self.assertNotIn("--end-of-tick", reference.tracediff_command(Path("a"), Path("b"), Path("s"), False, None))

    def test_the_options_are_refused_with_traces_and_quirks(self):
        for argv in (["--traces", "--end-of-tick"], ["--check", "--end-of-tick", "--quirks", "recommended"], ["--end-of-tick"]):
            with mock.patch.object(sys, "argv", ["reference.py", *argv]), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as raised:
                    reference.main()
            self.assertEqual(raised.exception.code, 2, argv)

    def watch_over(self, tmp, column):
        """A Watch over a live trace whose second tick differs in its `units` row only, the `tick` rows being equal."""
        def text(units2):
            rows = [(1, "units", "1" * 16), (1, "tick", "a" * 16), (1, "view", "7" * 16), (2, "units", units2 * 16), (2, "tick", "b" * 16),
                    (2, "view", "7" * 16), (3, "units", "3" * 16), (3, "tick", "c" * 16), (3, "view", "7" * 16)]
            return "bt-trace 10\ngame_seed 1\ncrt_seed 1\nview_seed 1\n" + "".join(f"{t} {ph} {h}\n" for t, ph, h in rows)

        live = Path(tmp) / "live/trace.txt"
        live.parent.mkdir(parents=True)
        live.write_text(text("9"))
        expected_rows = [line.split() for line in text("2").splitlines()[4:]]
        good = reference.tracedigest.digests([(int(t), ph, h) for t, ph, h in expected_rows])
        kept = Path(tmp) / "kept"
        kept.mkdir()
        (kept / "trace.txt").write_text(text("9"))
        proc = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
        watch = reference.Watch({"a": ({}, good)}, lambda pid, started: live, column)
        try:
            with mock.patch.object(reference, "keep_run", lambda *a, **k: kept):
                watch.started("a")(proc)
                watch.poll()
        finally:
            proc.kill()
            proc.wait()
        return watch.found

    def test_the_watch_halts_on_a_digest_difference_by_default(self):
        with tempfile.TemporaryDirectory() as tmp:
            found = self.watch_over(tmp, reference.DIGEST)
            self.assertIsNotNone(found)
            self.assertEqual(found.tick, 2)
            self.assertRegex(found.diff, r"tick 2: stored \w{16}, got \w{16}")

    def test_the_watch_in_end_of_tick_mode_ignores_a_digest_only_difference(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertIsNone(self.watch_over(tmp, reference.STATE))

    def test_the_watch_in_end_of_tick_mode_halts_on_a_state_difference(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            live = tmp / "trace.txt"
            live.write_text("bt-trace 10\n" + "".join(f"{t} tick {v * 16}\n{t} view {'7' * 16}\n" for t, v in ((1, "a"), (2, "9"), (3, "c"))))
            good = reference.tracedigest.digests([(t, ph, h) for t, v in ((1, "a"), (2, "b"), (3, "c")) for ph, h in (("tick", v * 16), ("view", "7" * 16))])
            kept = tmp / "kept"
            kept.mkdir()
            (kept / "trace.txt").write_text(live.read_text())
            proc = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
            watch = reference.Watch({"a": ({}, good)}, lambda pid, started: live, reference.STATE)
            try:
                with mock.patch.object(reference, "keep_run", lambda *a, **k: kept):
                    watch.started("a")(proc)
                    watch.poll()
            finally:
                proc.kill()
                proc.wait()
            found = watch.found
            self.assertEqual((found.tick, found.stored, found.got), (2, "b" * 16, "9" * 16))
            self.assertIsNone(reference.recheck(found, reference.STATE))

    def test_recheck_reads_the_column_the_watch_judged(self):
        found = reference.Found("a", "d", 2, 1.0, None, tick=2, stored="b" * 16, got="9" * 16)
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / "trace.txt").write_text("bt-trace 10\n" + "".join(f"{t} tick {v * 16}\n{t} view {'7' * 16}\n" for t, v in ((1, "a"), (2, "b"), (3, "c"))))
            found.kept = Path(tmp)
            self.assertIn("a different value", reference.recheck(found, reference.STATE))
            self.assertIsNone(reference.recheck(found, reference.DIGEST))


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
    OPTIONS = {"tier": False, "mode": "wasm", "steps_per_frame": None, "quirks": None, "roundtrip": None, "poison": False, "end_of_tick": False}

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
        (self.root / "docs").mkdir()
        (self.root / "docs/sim-events.md").write_text("### SIM_EVENT_SOUND\n")
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

    def test_end_of_tick_changes_the_key(self):
        self.assertNotEqual(self.key(end_of_tick=True), self.key(end_of_tick=False))

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
        for relative in ("tools/savefile.py", "data/block_pointers.csv", "docs/sim-events.md", "tools/deeper.py"):
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
