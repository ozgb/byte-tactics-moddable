"""Tests for replay_subsystems.py.

    uv run tools/test_replay_subsystems.py
"""

import tempfile
import unittest
from pathlib import Path

import replay_subsystems as rs


def entry(recording="a.txt", **extra):
    return {"inject": {"define": "X=1", "recording": recording, "expect": 3, "field": "f"}, **extra}


def tree(root, layout):
    for directory, files in layout.items():
        (Path(root) / directory).mkdir()
        for name in files:
            (Path(root) / directory / name).write_text("")


class Check(unittest.TestCase):
    def test_the_stored_list_matches_the_recordings(self):
        self.assertEqual(rs.check(rs.load()), [])

    def test_a_directory_without_an_entry_is_named(self):
        with tempfile.TemporaryDirectory() as root:
            tree(root, {"ai": ["a.txt"], "economy": ["a.txt"]})
            errors = rs.check({"economy": entry()}, root)
        self.assertEqual(errors, ["tests/replay/ai/ has no entry in tests/replay_subsystems.json"])

    def test_an_entry_without_a_directory_is_named(self):
        with tempfile.TemporaryDirectory() as root:
            tree(root, {"economy": ["a.txt"]})
            errors = rs.check({"economy": entry(), "ai": entry()}, root)
        self.assertEqual(errors, ["tests/replay_subsystems.json entry ai has no directory tests/replay/ai/"])

    def test_a_missing_recording_is_named(self):
        with tempfile.TemporaryDirectory() as root:
            tree(root, {"economy": ["b.txt"]})
            errors = rs.check({"economy": entry("a.txt", self_test="c.txt")}, root)
        self.assertEqual(
            errors,
            ["tests/replay/economy/a.txt does not exist", "tests/replay/economy/c.txt does not exist"],
        )

    def test_a_file_beside_the_directories_is_not_a_subsystem(self):
        with tempfile.TemporaryDirectory() as root:
            tree(root, {"economy": ["a.txt"]})
            (Path(root) / "windows.json").write_text("{}")
            self.assertEqual(rs.check({"economy": entry()}, root), [])


class Rows(unittest.TestCase):
    entries = {
        "ai": entry(),
        "wind": entry(self_test="s.txt", wasm={"module": "wind", "inject_variant": "wind_inject", "inject_define": "X=1"}),
    }

    def test_log_names_default_to_the_subsystem(self):
        self.assertEqual(rs.rows(self.entries, "native"), [["replay_ai", "ai"], ["replay_wind", "wind"]])
        self.assertEqual(rs.rows(self.entries, "inject")[0], ["replay_ai_inject", "ai", "3", "f", "a.txt"])

    def test_only_a_wasm_subsystem_has_wasm_rows(self):
        self.assertEqual(rs.rows(self.entries, "wasm"), [["replay_wind_wasm", "wind", "wind"]])
        self.assertEqual(rs.rows(self.entries, "wasm-self"), [["replay_wind_wasm_self", "wind", "wind", "s.txt"]])
        self.assertEqual(
            rs.rows(self.entries, "wasm-inject"),
            [["replay_wind_wasm_inject", "wind", "wind_inject", "3", "f", "a.txt"]],
        )

    def test_only_an_entry_with_a_self_test_has_a_self_row(self):
        self.assertEqual(rs.rows(self.entries, "self"), [["replay_wind_self", "wind", "s.txt"]])

    def test_log_names_can_be_overridden(self):
        entries = {"wind": entry(log_names={"native": "replay_native", "wasm_inject": "replay_wasm_inject"})}
        self.assertEqual(rs.rows(entries, "native"), [["replay_native", "wind"]])
        self.assertEqual(rs.log_name("wind", entries["wind"], "wasm-inject"), "replay_wasm_inject")


if __name__ == "__main__":
    unittest.main()
