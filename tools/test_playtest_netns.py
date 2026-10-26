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


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_"):
            fn()
    print("ok")
