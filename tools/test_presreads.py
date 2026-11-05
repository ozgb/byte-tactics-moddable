"""Tests for presreads.py, on synthetic stored data, call graphs and state classes.

    uv run tools/test_presreads.py
"""

import tempfile
import unittest
from pathlib import Path

import presreads as pr

SERVICES = """enum SimEventKind : unsigned short {
    SIM_EVENT_NONE = 0,
    SIM_EVENT_UNIT_SPEECH = 1,
    SIM_EVENT_SOUND = 2,
    SIM_EVENT_SCREEN_SHAKE = 11,
};"""
DRAIN = """
void DrainSimEvents()
{
    switch (e.kind) {
    case SIM_EVENT_UNIT_SPEECH:
        DrainUnitSpeech(e);
        break;
    case SIM_EVENT_SCREEN_SHAKE:
        if (x) Shake(e);
        break;
    default:
        break;
    }
}
"""


def classes(*rows):
    return [{"name": n, "kind": k, "class": c, "address": "0x0"} for n, k, c in rows]


def rw(function, kind, target="", file="a.cpp"):
    return {"function": function, "file": file, "kind": kind, "target": target, "address": "0x0"}


def stored_row(item, reader="presentation", function="Draw", container="units", every="0", after=""):
    return {"item": item, "reader": reader, "function": function, "container": container, "every": every, "after": after}


STATE = classes(("units", "game-region", "sim"), ("wind", "global", "sim"), ("players", "game-region", "mixed"),
                ("gui", "game-region", "view"), ("tick", "game-region", "sim"), ("Feature heap: realloc Feature", "heap", "sim"))
RWSETS = [rw("Draw", "function"), rw("OnShake", "function"), rw("Click", "function"), rw("Shake", "function"),
          rw("DrainUnitSpeech", "function"), rw("HandleLeftClick", "function"), rw("Util", "function"), rw("Idle", "function"),
          rw("Shake", "call", "Util"), rw("HandleLeftClick", "call", "Click"),
          rw("DrainUnitSpeech", "call", "Util"), rw("HandleLeftClick", "call", "Util")]
SUBSYSTEMS = [{"function": "Draw", "file": "a.cpp", "subsystem": "presentation"},
              {"function": "Click", "file": "a.cpp", "subsystem": "presentation"},
              {"function": "OnShake", "file": "a.cpp", "subsystem": "presentation"},
              {"function": "Idle", "file": "a.cpp", "subsystem": "units"}]


def build(stored, extra_rw=()):
    rows, report = pr.build(stored, STATE, RWSETS + list(extra_rw), SUBSYSTEMS, SERVICES, DRAIN)
    return {(r["item"], r["reader"]): r for r in rows}, report


class KindsTest(unittest.TestCase):
    def test_kind_names_leave_out_none(self):
        self.assertEqual(pr.kind_names(SERVICES), {1: "SIM_EVENT_UNIT_SPEECH", 2: "SIM_EVENT_SOUND", 11: "SIM_EVENT_SCREEN_SHAKE"})

    def test_a_case_runs_to_the_next_one(self):
        calls = pr.dispatch_calls(DRAIN)
        self.assertIn("DrainUnitSpeech", calls["SIM_EVENT_UNIT_SPEECH"])
        self.assertNotIn("Shake", calls["SIM_EVENT_UNIT_SPEECH"])
        self.assertIn("Shake", calls["SIM_EVENT_SCREEN_SHAKE"])


class ReduceTest(unittest.TestCase):
    ROWS = [
        {"item": "Unit::x", "reader": "presentation", "function": "Draw", "container": "units", "windows": "10", "events": "3:10;5:10;6:4"},
        {"item": "Unit::y", "reader": "presentation", "function": "Draw", "container": "units", "windows": "2", "events": "5:2"},
        {"item": "gui_x", "reader": "presentation", "function": "Draw", "container": "gui", "windows": "10", "events": ""},
        {"item": "ticks", "reader": "presentation", "function": "Draw", "container": "tick", "windows": "10", "events": ""},
    ]
    STATS = {"presentation_active_windows": 10, "presentation_kind_3": 10, "presentation_kind_5": 7}

    def test_reduction_keeps_every_after_and_drops_view_containers(self):
        rows = pr.reduce_scenario(self.ROWS, self.STATS, pr.classes_of(STATE))
        by_item = {r["item"]: r for r in rows}
        self.assertNotIn("gui_x", by_item)
        self.assertEqual(by_item["Unit::x"]["every"], "1")
        self.assertEqual(by_item["Unit::x"]["after"], "5")
        self.assertEqual(by_item["Unit::y"]["after"], pr.THIN)
        self.assertEqual(by_item["Unit::y"]["every"], "0")

    def test_a_kind_every_tick_emits_is_not_listed(self):
        rows = pr.reduce_scenario(self.ROWS[:1], self.STATS, pr.classes_of(STATE))
        self.assertNotIn("3", rows[0]["after"].split("|"))


class PatternTest(unittest.TestCase):
    def test_every_frame(self):
        rows, _ = build({"s": [stored_row("Unit::x", every="1")]})
        self.assertEqual(rows[("Unit::x", "presentation")]["pattern"], "every-frame")

    def test_after_event_from_the_runs_with_the_kind_name(self):
        rows, _ = build({"s": [stored_row("Unit::x", after="2")], "t": [stored_row("Unit::x", after="2|11")]})
        r = rows[("Unit::x", "presentation")]
        self.assertEqual((r["pattern"], r["covered_by_event"], r["runs"]), ("after-event", "SIM_EVENT_SOUND", 2))

    def test_a_scenario_that_disagrees_or_is_thin(self):
        rows, _ = build({"s": [stored_row("Unit::x", after="2")], "t": [stored_row("Unit::x", after="11")]})
        self.assertEqual(rows[("Unit::x", "presentation")]["pattern"], "unknown")
        rows, _ = build({"s": [stored_row("Unit::x", after="2")], "t": [stored_row("Unit::x", after=pr.THIN)]})
        self.assertEqual(rows[("Unit::x", "presentation")]["pattern"], "after-event")

    def test_two_functions_must_both_follow_the_kind(self):
        rows, _ = build({"s": [stored_row("Unit::x", function="Draw", after="2"), stored_row("Unit::x", function="Idle", after="11")]})
        self.assertEqual(rows[("Unit::x", "presentation")]["pattern"], "unknown")

    def test_ui_action_by_static_reach(self):
        rows, _ = build({"s": [stored_row("Unit::x", function="Click")]})
        self.assertEqual(rows[("Unit::x", "presentation")]["pattern"], "ui-action")

    def test_after_event_by_static_reach_and_the_covering_kind(self):
        rows, _ = build({"s": [stored_row("Unit::x", function="OnShake")]}, [rw("DrainUnitSpeech", "call", "OnShake")])
        r = rows[("Unit::x", "presentation")]
        self.assertEqual((r["pattern"], r["covered_by_event"]), ("after-event", "SIM_EVENT_UNIT_SPEECH"))

    def test_a_function_two_cases_reach_covers_no_kind(self):
        rows, _ = build({"s": [stored_row("Unit::x", function="Util")]})
        r = rows[("Unit::x", "presentation")]
        self.assertEqual((r["covered_by_event"], r["pattern"]), ("", "ui-action"))

    def test_covered_by_event_empty_and_unknown(self):
        rows, _ = build({"s": [stored_row("Unit::x", function="Idle")]})
        r = rows[("Unit::x", "presentation")]
        self.assertEqual((r["covered_by_event"], r["pattern"]), ("", "unknown"))

    def test_one_row_per_reader_and_runs_count_scenarios(self):
        rows, _ = build({"s": [stored_row("Unit::x"), stored_row("Unit::x", reader="units", function="Idle")], "t": [stored_row("Unit::x")]})
        self.assertEqual(rows[("Unit::x", "presentation")]["runs"], 2)
        self.assertEqual(rows[("Unit::x", "units")]["runs"], 1)


class VisibilityTest(unittest.TestCase):
    def test_each_label(self):
        self.assertEqual(pr.visibility("Unit::health", "units"), "per-player-filter")
        self.assertEqual(pr.visibility("Feature::x", "heap"), "per-player-filter")
        self.assertEqual(pr.visibility("Player::metal", "players"), "own")
        self.assertEqual(pr.visibility("localPlayer", "localPlayer"), "own")
        self.assertEqual(pr.visibility("windSpeedMin", "windSpeedMin"), "all-players")
        self.assertEqual(pr.visibility("ticks", "ticks"), "all-players")


class StaticOnlyTest(unittest.TestCase):
    def test_an_unreached_sim_item_gets_a_static_only_row(self):
        extra = [rw("Draw", "global_read", "wind"), rw("Draw", "field_read", "Game::tick"), rw("Click", "field_read", "Game::gui"),
                 rw("Idle", "global_read", "wind"), rw("Draw", "field_read", "Feature::name"), rw("Draw", "field_read", "Mystery::z")]
        rows, report = build({"s": [stored_row("Unit::x")]}, extra)
        self.assertEqual(rows[("wind", "presentation")]["runs"], 0)
        self.assertEqual(rows[("wind", "presentation")]["visibility"], "all-players")
        self.assertIn(("tick", "presentation"), rows)
        self.assertNotIn(("gui", "presentation"), rows)
        self.assertEqual(rows[("Feature::name", "presentation")]["visibility"], "per-player-filter")
        self.assertNotIn(("Mystery::z", "presentation"), rows)
        self.assertEqual((report["static_not_sim"], report["static_unclassified"]), (1, 1))

    def test_a_reached_item_is_not_static_only(self):
        rows, _ = build({"s": [stored_row("wind", container="wind")]}, [rw("Draw", "global_read", "wind")])
        self.assertEqual(rows[("wind", "presentation")]["runs"], 1)

    def test_static_pattern_by_reach(self):
        extra = [rw("Click", "global_read", "wind"), rw("OnShake", "field_read", "Game::tick"),
                 rw("DrainUnitSpeech", "call", "OnShake"), rw("Draw", "field_read", "Game::units")]
        rows, _ = build({}, extra)
        self.assertEqual(rows[("wind", "presentation")]["pattern"], "ui-action")
        self.assertEqual(rows[("tick", "presentation")]["pattern"], "after-event")
        self.assertEqual(rows[("tick", "presentation")]["covered_by_event"], "SIM_EVENT_UNIT_SPEECH")
        self.assertEqual(rows[("units", "presentation")]["pattern"], "unknown")


class FilesTest(unittest.TestCase):
    def test_render_and_store_round_trip(self):
        rows, _ = build({"s": [stored_row("Unit::x", every="1")]})
        text = pr.render(sorted(rows.values(), key=lambda r: (r["item"], r["reader"])))
        self.assertTrue(text.startswith("item,reader,pattern,covered_by_event,visibility,runs\n"))
        self.assertIn("Unit::x,presentation,every-frame,,per-player-filter,1\n", text)
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "s.presentation.csv").write_text("item,reader,function,windows,events,container\n"
                                                    '"Unit::x",presentation,"Draw",5,5:5,"units"\n')
            (tmp / "s.stats.txt").write_text("presentation_active_windows 5\npresentation_kind_5 5\n")
            out = tmp / "out"
            self.assertEqual(pr.store(tmp, out, pr.classes_of(STATE)), 1)
            self.assertEqual(pr.load_stored(out)["s"][0]["every"], "1")


if __name__ == "__main__":
    unittest.main()
