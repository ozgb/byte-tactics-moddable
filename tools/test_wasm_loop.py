"""Tests for tools/wasm_loop.py: uv run tools/test_wasm_loop.py"""

import unittest

import wasm_loop


class Summary(unittest.TestCase):
    def test_counts_and_tail(self):
        lines = [f"line {n}" for n in range(60)]
        text = wasm_loop.summarise([(0, 0, ["ok"]), (1, 101, lines)], "wine-11.0", "Debugger REG_SZ", 3.0)
        self.assertIn("runs: 2, failed: 1", text)
        self.assertIn("line 59", text)
        self.assertNotIn("line 19\n", text)

    def test_backtrace_is_kept(self):
        lines = ["start", "thread panicked at traphandlers.rs:627", "stack backtrace:", "  0: frame"] + ["x"] * 80
        self.assertEqual(wasm_loop.panic_lines(lines)[0], "thread panicked at traphandlers.rs:627")
        self.assertEqual(wasm_loop.panic_lines(["fine"]), [])


if __name__ == "__main__":
    unittest.main()
