"""Tests of the error-dialog matching and the retry decision: `uv run tools/test_playtest_dialogs.py`.

Pure functions only; no game, display or Wine is run.
"""

import argparse
import tempfile
from pathlib import Path

import playtest
import reference
import tracediff

GAME = ("Total Annihilation", 640, 480)
BOX = ("Total Annihilation", 231, 77)


def test_classify() -> None:
    assert playtest.classify_dialog([GAME], False) is None
    assert playtest.classify_dialog([GAME, BOX], False) == ("dialog-directx", "Total Annihilation")
    assert playtest.classify_dialog([GAME, BOX], True) is None
    crt = ("Microsoft Visual C++ Runtime Library", 380, 150)
    assert playtest.classify_dialog([GAME, crt], True) == ("dialog-crt", crt[0])
    assert playtest.classify_dialog([GAME, crt], False)[0] == "dialog-crt"
    assert playtest.classify_dialog([("Cavedog", 300, 120)], True)[0] == "dialog-cavedog"
    assert playtest.classify_dialog([("tiny", 1, 1)], False) is None


def test_ticks_started() -> None:
    assert not playtest.ticks_started("bt-trace 1\ngame_seed 5\n")
    assert playtest.ticks_started("bt-trace 1\ngame_seed 5\n1 move 00ff12ab\n")


def test_retry() -> None:
    assert reference.should_retry("playtest.py failed: the game exited with code 1")
    assert reference.should_retry("the game shows an error dialog (dialog-directx, window title \"x\")")
    assert not reference.should_retry("the game shows an error dialog (dialog-crt, window title \"x\")")
    assert not reference.should_retry("the game shows an error dialog (dialog-cavedog, window title \"x\")")
    assert not reference.should_retry("the game still runs after 1470s")



def test_tracediff_poisons_only_b() -> None:
    calls = []

    def fake_play(exe, scenario, game_dir, dump, timeout=None, poison=False):
        calls.append((exe.name, dump, poison))
        return Path(exe.name)

    saved = tracediff.play, tracediff.need, tracediff.parse_trace, tracediff.diff_dumps, tracediff.print_diff
    saved_load = tracediff.scenario_load
    tracediff.play = fake_play
    tracediff.scenario_load = lambda scenario: None
    tracediff.need = lambda run, name: run / name
    tracediff.parse_trace = lambda path: ({}, [(1, "units", "aa")] if path.parent.name == "a.exe" else [(1, "units", "bb")])
    tracediff.diff_dumps = lambda *a: []
    tracediff.print_diff = lambda diffs: None
    try:
        args = argparse.Namespace(exe=["a.exe", "b.exe"], scenario=["s.txt"], game_dir=None,
                                  ignore_view_seed=False, poison_b=True, reference=None)
        assert tracediff.compare(args) == 1
    finally:
        tracediff.play, tracediff.need, tracediff.parse_trace, tracediff.diff_dumps, tracediff.print_diff = saved
        tracediff.scenario_load = saved_load
    assert calls == [("a.exe", None, False), ("b.exe", None, True), ("a.exe", 1, False), ("b.exe", 1, True)], calls


def fake_run(tmp: Path, put=None, collect=None) -> argparse.Namespace:
    exe = tmp / "TotalA.exe"
    exe.write_bytes(b"exe")
    scenario = tmp / "scenario.txt"
    scenario.write_text("ticks 1\n")
    return argparse.Namespace(work=tmp / "work", out=tmp / "out", exes=[exe], testmode=scenario, put=put or [],
                              collect=collect or [], notes=[])


def test_put_places_a_file_in_the_game_copy() -> None:
    from unittest import mock
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        src = tmp / "harness.sav"
        src.write_bytes(b"save data")
        run = fake_run(tmp, put=[("savegame/harness.sav", src)])
        inst = playtest.Instance(run, 1)

        def fake_copy_game(src_dir, dst):
            dst.mkdir(parents=True, exist_ok=True)

        with mock.patch.object(playtest, "link_prefix"), mock.patch.object(playtest, "copy_game", fake_copy_game):
            inst.prepare(tmp / "template", tmp / "game_src", [], {})
        assert (inst.game / "savegame/harness.sav").read_bytes() == b"save data"


def test_collect_keeps_a_file_of_the_game_copy() -> None:
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        run = fake_run(tmp, collect=["savegame/harness.sav", "savegame/absent.sav"])
        inst = playtest.Instance(run, 1)
        (inst.game / "savegame").mkdir(parents=True)
        (inst.game / "savegame/harness.sav").write_bytes(b"written by the game")
        run.out.mkdir(parents=True)
        inst.collect_traces()
        assert (run.out / "savegame/harness.sav").read_bytes() == b"written by the game"
        assert not (run.out / "savegame/absent.sav").exists()


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_"):
            fn()
    print("ok")
