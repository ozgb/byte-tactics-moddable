"""Tests of the Wine release a play-test uses: `uv run tools/test_playtest_wine.py`.

No Wine is started: `wine --version` and the prefix commands are fakes.
"""

import os
import tempfile
from pathlib import Path
from unittest import mock

import playtest


def fake_version(version: str):
    return mock.patch.object(playtest, "wine_version", return_value=version)


def fake_wine(command, **kwargs):
    Path(kwargs["env"]["WINEPREFIX"]).mkdir(parents=True, exist_ok=True)
    return mock.Mock(stdout="")


def test_the_release_names_a_directory() -> None:
    assert playtest.release_name("wine-11.0 (Staging)") == "wine-11.0-Staging"
    assert playtest.release_name("wine-10.0") == "wine-10.0"
    assert playtest.release_name("") == "unknown"


def test_each_release_has_its_own_template() -> None:
    with tempfile.TemporaryDirectory() as d, mock.patch.object(playtest, "OUT", Path(d)), \
            mock.patch.object(playtest, "ROOT", Path(d)), mock.patch.object(playtest.subprocess, "run", fake_wine):
        with fake_version("wine-11.0 (Staging)"):
            staging = playtest.template_prefix()
        with fake_version("wine-10.0"):
            stable = playtest.template_prefix()
        with fake_version("wine-11.0 (Staging)"):
            assert playtest.template_prefix() == staging
        assert staging == Path(d) / "prefix/wine-11.0-Staging"
        assert stable == Path(d) / "prefix/wine-10.0"
        assert (staging / ".playtest-wine-version").is_file(), "the second release rebuilt the first's template"
        with fake_version("wine-10.0"):
            base = playtest.template_prefix()
            assert base.with_name(f"{base.name}-directplay") == Path(d) / "prefix/wine-10.0-directplay"


def test_bt_wine_bin_comes_first_on_path() -> None:
    with mock.patch.dict(os.environ, {"PATH": "/usr/bin", "BT_WINE_BIN": "/opt/wine-stable/bin"}):
        assert playtest.wine_path() == "/opt/wine-stable/bin:/usr/bin"
        assert playtest.wine_env(Path("/p"))["PATH"].startswith("/opt/wine-stable/bin:")
    env = {k: v for k, v in os.environ.items() if k != "BT_WINE_BIN"}
    with mock.patch.dict(os.environ, env, clear=True):
        assert playtest.wine_path() == os.environ.get("PATH", os.defpath)


def test_the_version_probe_uses_bt_wine_bin() -> None:
    seen = {}

    def fake_run(command, **kwargs):
        seen.update(kwargs["env"])
        return mock.Mock(stdout="wine-10.0\n")

    with mock.patch.dict(os.environ, {"BT_WINE_BIN": "/opt/wine-stable/bin"}), mock.patch.object(playtest.subprocess, "run", fake_run):
        assert playtest.wine_version() == "wine-10.0"
    assert seen["PATH"].startswith("/opt/wine-stable/bin:")


if __name__ == "__main__":
    tests = [t for n, t in sorted(globals().items()) if n.startswith("test_") and callable(t)]
    for t in tests:
        t()
    print(f"{len(tests)} tests passed")
