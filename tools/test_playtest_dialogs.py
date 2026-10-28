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


WINE_LOG = (
    '0120:trace:dialog:MSGBOX_DlgProc Message number 0x0007 is being ignored.\n'
    '0120:trace:msgbox:MSGBOX_OnInit L"Runtime Error!\\n\\nProgram: Z:\\\\game\\\\TotalA.exe\\n\\nR6025\\r\\n'
    '- pure virtual function call\\r\\n"\n'
    '0120:trace:dialog:MSGBOX_DlgProc Message number 0x0008 is being ignored.\n'
)


def test_dialog_text() -> None:
    assert playtest.dialog_text(WINE_LOG) == "Runtime Error! Program: Z:\\game\\TotalA.exe R6025 - pure virtual function call"
    assert playtest.dialog_text("fixme:ntdll:something\n") == ""
    assert playtest.dialog_text('trace:msgbox:MSGBOX_OnInit L"a \\"q\\" \\00e9"\n') == 'a "q" \u00e9'
    two = WINE_LOG + 'trace:msgbox:MSGBOX_OnInit L"second"\n'
    assert playtest.dialog_text(two) == "second"


def test_error_dialog_reads_the_text_from_wine_log() -> None:
    from unittest import mock
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        run = fake_run(tmp)
        inst = playtest.Instance(run, 1)
        run.out.mkdir(parents=True)
        (run.out / f"wine{inst.suffix}.log").write_text(WINE_LOG)
        crt = ("Microsoft Visual C++ Runtime Library", 380, 150)
        with mock.patch.object(inst, "windows", return_value=[GAME, crt]):
            kind, title, text = inst.error_dialog()
        assert (kind, title) == ("dialog-crt", crt[0])
        assert "R6025 - pure virtual function call" in text
        with mock.patch.object(inst, "windows", return_value=[GAME]):
            assert inst.error_dialog() is None
        (run.out / f"wine{inst.suffix}.log").unlink()
        with mock.patch.object(inst, "windows", return_value=[GAME, crt]):
            assert inst.error_dialog() == ("dialog-crt", crt[0], "")


def test_ticks_started() -> None:
    assert not playtest.ticks_started("bt-trace 1\ngame_seed 5\n")
    assert playtest.ticks_started("bt-trace 1\ngame_seed 5\n1 move 00ff12ab\n")


def test_retry() -> None:
    assert reference.should_retry("playtest.py failed: the game exited with code 1")
    assert reference.should_retry("the game shows an error dialog (dialog-directx, window title \"x\")")
    assert not reference.should_retry("the game shows an error dialog (dialog-crt, window title \"x\")")
    assert not reference.should_retry("the game shows an error dialog (dialog-cavedog, window title \"x\")")
    assert not reference.should_retry("the game still runs after 1470s")
    crt = 'the game shows an error dialog (dialog-crt, window title "x", text "R6025 - pure virtual function call")'
    assert not reference.should_retry(crt)
    assert not reference.should_retry(crt.replace("R6025", "dialog-directx"))
    directx = 'the game shows an error dialog (dialog-directx, window title "x", text "DirectDraw failed, still runs after")'
    assert reference.should_retry(directx)



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


def test_prepare_removes_a_stale_put_and_collect_path() -> None:
    from unittest import mock
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        src = tmp / "put.opt"
        src.write_bytes(b"put")
        run = fake_run(tmp, put=[("savegame/harness.opt", src)], collect=["savegame/harness.sav"])
        inst = playtest.Instance(run, 1)

        def stale_copy_game(src_dir, dst):
            (dst / "savegame").mkdir(parents=True, exist_ok=True)
            (dst / "savegame/harness.sav").write_bytes(b"stale save")
            (dst / "savegame/harness.opt").write_bytes(b"stale options")

        with mock.patch.object(playtest, "link_prefix"), mock.patch.object(playtest, "copy_game", stale_copy_game):
            inst.prepare(tmp / "template", tmp / "game_src", [], {})
        assert not (inst.game / "savegame/harness.sav").exists()
        assert (inst.game / "savegame/harness.opt").read_bytes() == b"put"


def test_put_does_not_write_through_a_symlink() -> None:
    from unittest import mock
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        install = tmp / "install.opt"
        install.write_bytes(b"install file")
        src = tmp / "put.opt"
        src.write_bytes(b"put")
        run = fake_run(tmp, put=[("savegame/harness.opt", src)])
        inst = playtest.Instance(run, 1)

        def linking_copy_game(src_dir, dst):
            (dst / "savegame").mkdir(parents=True, exist_ok=True)
            (dst / "savegame/harness.opt").symlink_to(install)

        with mock.patch.object(playtest, "link_prefix"), mock.patch.object(playtest, "copy_game", linking_copy_game):
            inst.prepare(tmp / "template", tmp / "game_src", [], {})
        assert install.read_bytes() == b"install file"
        assert not (inst.game / "savegame/harness.opt").is_symlink()
        assert (inst.game / "savegame/harness.opt").read_bytes() == b"put"


def test_collect_notes_the_crash_file() -> None:
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        run = fake_run(tmp)
        inst = playtest.Instance(run, 1)
        (inst.game).mkdir(parents=True)
        (inst.game / "crash.txt").write_text("purecall from 0x0040106c\n")
        run.out.mkdir(parents=True)
        inst.collect_traces()
        assert (run.out / "crash.txt").read_text() == "purecall from 0x0040106c\n"
        assert any("purecall from 0x0040106c" in n for n in run.notes), run.notes


def test_debug_defaults() -> None:
    assert "+seh" in playtest.DEBUG_TESTMODE.split(",") and "err+seh" not in playtest.DEBUG_TESTMODE
    assert "err+seh" in playtest.DEBUG_SCREEN and "+msgbox" in playtest.DEBUG_SCREEN


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
