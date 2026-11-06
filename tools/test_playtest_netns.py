"""Tests of the network namespace of a multiplayer run: `uv run tools/test_playtest_netns.py`.

No holder, namespace or Wine is started; the holder's pid is a fake one.
"""

import tempfile
from pathlib import Path

import playtest

FAKE_PID = 4242


def make_run(tmp: Path, scenario: playtest.Scenario) -> playtest.Run:
    return playtest.Run(1, scenario.name, scenario, [tmp / "TotalA.exe"], "modern", tmp, {}, "-all")


def with_holder(run: playtest.Run) -> None:
    if run.needs_netns():
        run.netns = playtest.NetNamespace()
        run.netns.pid = FAKE_PID


def test_two_instance_run_is_wrapped() -> None:
    with tempfile.TemporaryDirectory() as d:
        run = make_run(Path(d), playtest.Scenario("two", [], {}, instances=2))
        with_holder(run)
        assert run.needs_netns()
        for inst in run.instances:
            assert inst.wrap(["wine", "x.exe"]) == [
                "nsenter", "-t", str(FAKE_PID), "-U", "-n", "--preserve-credentials", "--", "wine", "x.exe"]
            assert inst.wrap(["wineserver", "-k"])[-2:] == ["wineserver", "-k"]


def test_directplay_run_is_wrapped() -> None:
    with tempfile.TemporaryDirectory() as d:
        run = make_run(Path(d), playtest.Scenario("dp", [], {}, directplay=True))
        with_holder(run)
        assert run.instances[0].wrap(["wine"])[0] == "nsenter"


def test_single_instance_run_is_not_wrapped() -> None:
    with tempfile.TemporaryDirectory() as d:
        run = make_run(Path(d), playtest.Scenario("one", [], {}))
        with_holder(run)
        assert not run.needs_netns()
        assert run.netns is None
        assert run.instances[0].wrap(["wine", "x.exe"]) == ["wine", "x.exe"]


def test_record_values() -> None:
    known = {"economy", "map"}
    assert playtest.check_record("economy", known) is None
    assert playtest.check_record("economy:30:900", known) is None
    assert playtest.check_record("sound", known) is not None
    assert playtest.check_record("economy:30", known) is not None
    assert playtest.check_record("economy:0:9", known) is not None
    assert playtest.check_record("economy:9:3", known) is not None
    assert playtest.check_record("economy:a:3", known) is not None


def test_record_names_the_file_of_the_run() -> None:
    with tempfile.TemporaryDirectory() as d:
        sc = playtest.Scenario("two", [], {}, instances=2)
        assert make_run(Path(d), sc).recordings() == []
        run = playtest.Run(1, "two", sc, [Path(d) / "TotalA.exe"], "modern", Path(d), {}, "-all", record="economy:30:900")
        assert run.recordings() == ["replay-economy.txt"]


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_"):
            fn()
    print("ok")
