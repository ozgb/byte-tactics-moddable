"""Test that a scenario with a load line reports the load line's error before a decided key's value error.

Plays scenarios on build/modern/TotalA.exe under Wine, so it skips when either is missing.

    uv run tools/test_harness_load.py
"""

import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = ROOT / "build/modern/TotalA.exe"
GAME_DIR = os.environ.get("BT_GAME_DIR", str(Path.home() / ".ta"))
LOAD_ERROR = "cannot be set by a scenario with a load line"


@unittest.skipUnless(EXE.is_file() and shutil.which("wine"), "needs build/modern/TotalA.exe and wine")
class DecidedKeyErrorOrderTest(unittest.TestCase):
    def play(self, lines: list[str]) -> tuple[int, str]:
        """The exe's exit status and its stderr text for a scenario of `lines`."""
        (ROOT / "build/scratch").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="load-order-", dir=ROOT / "build/scratch") as tmp:
            scenario = Path(tmp) / "order.txt"
            scenario.write_text("\n".join(lines) + "\n")
            cmd = [sys.executable, str(ROOT / "tools/playtest.py"), "--exe", str(EXE), "--testmode", str(scenario),
                   "--game-dir", GAME_DIR, "--testmode-timeout", "120"]
            out = subprocess.run(cmd, capture_output=True, text=True).stdout
            logs = re.search(r"^logs and screenshots: (.+)/$", out, re.M)
            self.assertIsNotNone(logs, out)
            session = ROOT / logs[1]
            try:
                run = next(d for d in session.iterdir() if d.is_dir())
                status = json.loads((run / "result.json").read_text())["exit_codes"][0]
                return status, (run / "wine.log").read_text()
            finally:
                shutil.rmtree(session, ignore_errors=True)

    def check(self, lines: list[str], key: str, line_no: int) -> None:
        status, log = self.play(lines)
        self.assertEqual(status, 2)
        self.assertIn(f"testmode: scenario line {line_no}: {key} {LOAD_ERROR}", log)

    def test_numeric_key_with_a_bad_value(self):
        self.check(["ticks 100", "load save-midgame", "difficulty banana"], "difficulty", 3)

    def test_numeric_key_out_of_range(self):
        self.check(["ticks 100", "load save-midgame", "game_type 9"], "game_type", 3)

    def test_player_key_with_a_bad_value(self):
        self.check(["ticks 100", "load save-midgame", "player0.metal banana"], "player0.metal", 3)

    def test_decided_key_before_the_load_line(self):
        self.check(["ticks 100", "difficulty banana", "load save-midgame"], "difficulty", 2)


if __name__ == "__main__":
    unittest.main()
