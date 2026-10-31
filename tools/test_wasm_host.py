"""Tests for tools/wasm_host.py: uv run tools/test_wasm_host.py"""

import os
import unittest
from pathlib import Path
from unittest import mock

import wasm_host


class CargoEnv(unittest.TestCase):
    def test_gnu_uses_mingw(self):
        env = wasm_host.cargo_env(Path("/o"))
        self.assertEqual(env["CARGO_TARGET_I686_PC_WINDOWS_GNU_LINKER"], "i686-w64-mingw32-gcc")
        self.assertNotIn("CARGO_TARGET_I686_PC_WINDOWS_MSVC_LINKER", env)

    def test_msvc_uses_lld_link_and_xwin_paths(self):
        env = wasm_host.cargo_env(Path("/o"), "i686-pc-windows-msvc", Path("/x"))
        self.assertEqual(env["CARGO_TARGET_I686_PC_WINDOWS_MSVC_LINKER"], "lld-link")
        for lib in ("/x/crt/lib/x86", "/x/sdk/lib/um/x86", "/x/sdk/lib/ucrt/x86"):
            self.assertIn(f"-Lnative={lib}", env["RUSTFLAGS"])

    def test_xwin_default_follows_environment(self):
        with mock.patch.dict(os.environ, {"BT_XWIN": "/elsewhere"}):
            self.assertEqual(wasm_host.default_xwin(), Path("/elsewhere"))
        with mock.patch.dict(os.environ, {}, clear=True):
            self.assertEqual(wasm_host.default_xwin(), Path.home() / ".xwin-x86")


if __name__ == "__main__":
    unittest.main()
