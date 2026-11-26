"""Tests for tools/runtimevectors.py, on fixture directories.

    uv run tools/test_runtimevectors.py
"""

import json
import struct
import tempfile
import unittest
from pathlib import Path

import runtimevectors

PAIR = struct.pack("<ii", 3, 4)
WORD = struct.pack("<I", 2) + b"ab" + struct.pack("<i", 7)


def manifest(**overrides):
    entries = {
        "alpha": {"file": "alpha.bin", "inputs": [["a", "i32"]], "outputs": [["b", "i32"]], "edge": 1, "random": 1,
                  "random_total": 5, "dropped": 0},
        "beta": {"file": "beta.bin", "inputs": [["s", "str"]], "outputs": [["n", "i32"]], "edge": 1, "random": 0,
                 "random_total": 0, "dropped": 0},
    }
    entries.update(overrides)
    return {"format": 1, "entries": entries, "omitted": {"gamma": "left open"}}


def write(root: Path, data=None, files=None, readme="`alpha` `beta` `gamma`\n"):
    data = data or manifest()
    (root / "vectors.json").write_text(json.dumps(data))
    files = files if files is not None else {"alpha.bin": PAIR * 2, "beta.bin": WORD}
    for name, content in files.items():
        (root / name).write_bytes(content)
    (root / "README.md").write_text(readme)
    (root / "layouts.txt").write_text("".join(runtimevectors.layout_line(n, e) + "\n" for n, e in sorted(data["entries"].items())))


class Check(unittest.TestCase):
    def problems(self, **kwargs):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write(root, **kwargs)
            return runtimevectors.check(root)

    def test_a_good_directory_passes(self):
        self.assertEqual(self.problems(), [])

    def test_an_over_budget_directory_fails(self):
        files = {"alpha.bin": PAIR * 2, "beta.bin": WORD + b"\0" * (runtimevectors.BUDGET + 1)}
        problems = self.problems(files=files)
        self.assertTrue(any("over the budget" in p for p in problems), problems)

    def test_a_count_mismatch_fails(self):
        problems = self.problems(data=manifest(alpha={"file": "alpha.bin", "inputs": [["a", "i32"]],
                                                      "outputs": [["b", "i32"]], "edge": 2, "random": 1,
                                                      "random_total": 5, "dropped": 0}))
        self.assertTrue(any("holds 2 records" in p for p in problems), problems)

    def test_a_truncated_record_fails(self):
        problems = self.problems(files={"alpha.bin": PAIR * 2, "beta.bin": WORD[:-1]})
        self.assertTrue(any("beta" in p for p in problems), problems)

    def test_a_file_not_in_the_manifest_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write(root)
            (root / "stray.bin").write_bytes(b"x")
            self.assertEqual(runtimevectors.check(root), ["stray.bin: not in the manifest"])

    def test_a_readme_missing_an_entry_fails(self):
        problems = self.problems(readme="`alpha` `gamma`\n")
        self.assertEqual(problems, ["README.md does not name beta"])

    def test_a_stale_readme_fails_and_readme_rewrites_it(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write(root)
            self.assertEqual(runtimevectors.readme_problems(root),
                             ["README.md is not readme(entries): run uv run tools/runtimevectors.py readme"])
            self.assertEqual(runtimevectors.main(["readme", "--dir", str(root)]), 0)
            self.assertEqual(runtimevectors.readme_problems(root), [])

    def test_an_unknown_type_fails(self):
        problems = self.problems(data=manifest(alpha={"file": "alpha.bin", "inputs": [["a", "float"]],
                                                      "outputs": [], "edge": 1, "random": 1, "random_total": 5,
                                                      "dropped": 0}))
        self.assertTrue(any("malformed" in p for p in problems), problems)

    def test_a_missing_manifest_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(len(runtimevectors.check(Path(tmp))), 1)


class Sampling(unittest.TestCase):
    def test_strata_take_one_index_from_each_stratum(self):
        picked = runtimevectors.strata(1000, 10)
        self.assertEqual(len(picked), 10)
        for j, i in enumerate(picked):
            self.assertTrue(j * 100 <= i < (j + 1) * 100)
        self.assertEqual(picked, runtimevectors.strata(1000, 10))

    def test_a_sample_keeps_what_fits(self):
        data = PAIR * 10
        self.assertEqual(len(runtimevectors.sample(data, ["i32", "i32"], 10_000)), 10)
        self.assertEqual(len(runtimevectors.sample(data, ["i32", "i32"], 40)), 5)

    def test_a_walk_reads_strings(self):
        self.assertEqual(runtimevectors.count_records(WORD * 3, ["str", "i32"]), 3)


if __name__ == "__main__":
    unittest.main()
