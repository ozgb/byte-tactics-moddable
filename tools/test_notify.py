"""Tests for notify.py, on temporary directories and without the network.

    uv run tools/test_notify.py
"""

import re
import stat
import tempfile
import unittest
from pathlib import Path

import notify


class TopicTest(unittest.TestCase):
    def test_generates_once_and_reuses(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            first, created = notify.topic(root)
            self.assertTrue(created)
            self.assertRegex(first, r"^bt-[a-z2-9]{10}$")
            self.assertFalse(set(first[3:]) & set("ilo01"))
            self.assertEqual(stat.S_IMODE((root / notify.FILE).stat().st_mode), 0o600)
            again, created = notify.topic(root)
            self.assertFalse(created)
            self.assertEqual(again, first)

    def test_reads_an_existing_topic(self):
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / notify.FILE).write_text("  my-topic \n")
            self.assertEqual(notify.topic(Path(d)), ("my-topic", False))

    def test_empty_file_is_replaced(self):
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / notify.FILE).write_text("\n")
            name, created = notify.topic(Path(d))
            self.assertTrue(created)
            self.assertTrue(re.match(r"^bt-", name))

    def test_topics_differ(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            self.assertNotEqual(notify.topic(Path(a))[0], notify.topic(Path(b))[0])


class RequestTest(unittest.TestCase):
    def test_headers_and_body(self):
        req = notify.request("t", "hello", title="PR #1", click="https://x/1", priority=4, tags="eyes")
        self.assertEqual(req.full_url, "https://ntfy.sh/t")
        self.assertEqual(req.get_method(), "POST")
        self.assertEqual(req.data, b"hello")
        self.assertEqual(req.get_header("Title"), "PR #1")
        self.assertEqual(req.get_header("Click"), "https://x/1")
        self.assertEqual(req.get_header("Priority"), "4")
        self.assertEqual(req.get_header("Tags"), "eyes")

    def test_no_optional_headers(self):
        req = notify.request("t", "hello")
        self.assertEqual(req.header_items(), [])


class CloneRootTest(unittest.TestCase):
    def test_worktree_shares_the_main_root(self):
        here = Path(__file__).resolve().parent
        root = notify.clone_root(here)
        self.assertTrue((root / ".git").exists())


if __name__ == "__main__":
    unittest.main()
