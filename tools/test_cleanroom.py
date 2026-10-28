"""Tests for the clean-room proxy and canary manifest, on localhost and temp trees only.

    python3 tools/test_cleanroom.py
    uv run tools/test_cleanroom.py
"""

import asyncio
import contextlib
import io
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "cleanroom"))

import manifest as mf
import proxy as px


class AllowlistTests(unittest.TestCase):
    def test_comments_blanks_and_default_port(self):
        got = px.parse_allowlist("# c\n\n  api.example.com  \nother.example:8443\n")
        self.assertEqual(got, {("api.example.com", 443), ("other.example", 8443)})

    def test_case_is_folded(self):
        self.assertEqual(px.parse_allowlist("API.Example.COM\n"), {("api.example.com", 443)})
        self.assertEqual(px.split_target("Api.Example.Com:443"), ("api.example.com", 443))

    def test_exact_match_only(self):
        allowed = px.parse_allowlist("api.anthropic.com\n")
        for target in (
            "evil.api.anthropic.com.attacker.net:443",
            "evil.api.anthropic.com:443",
            "anthropic.com:443",
            "api.anthropic.com:80",
        ):
            self.assertNotIn(px.split_target(target), allowed)
        self.assertIn(px.split_target("API.anthropic.com:443"), allowed)

    def test_ip_literals_refused_unless_listed(self):
        allowed = px.parse_allowlist("api.example.com\n10.0.0.5:8080\n")
        self.assertNotIn(px.split_target("10.0.0.5:443"), allowed)
        self.assertNotIn(px.split_target("127.0.0.1:443"), allowed)
        self.assertIn(px.split_target("10.0.0.5:8080"), allowed)

    def test_malformed_targets(self):
        for bad in ("", "host:", "host:abc", "host:0", "host:70000", ":80", "a:b:c", "[::1"):
            self.assertIsNone(px.split_target(bad), bad)
        self.assertEqual(px.split_target("[::1]:80"), ("::1", 80))
        with self.assertRaises(ValueError):
            px.parse_allowlist("host:abc\n")


class ProxyTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.log = self.dir / "proxy.log"
        self.addCleanup(self.tmp.cleanup)

    async def exchange(self, port, payload, expect_echo=None):
        reader, writer = await asyncio.open_connection("127.0.0.1", port)
        writer.write(payload)
        await writer.drain()
        head = await asyncio.wait_for(reader.readuntil(b"\r\n\r\n"), 5)
        echoed = None
        if expect_echo is not None:
            writer.write(expect_echo)
            await writer.drain()
            echoed = await asyncio.wait_for(reader.readexactly(len(expect_echo)), 5)
        writer.close()
        await writer.wait_closed()
        return head.split(b"\r\n", 1)[0].decode(), echoed

    async def scenario(self):
        async def echo(reader, writer):
            while data := await reader.read(1024):
                writer.write(data)
                await writer.drain()
            writer.close()

        echo_server = await asyncio.start_server(echo, "127.0.0.1", 0)
        echo_port = echo_server.sockets[0].getsockname()[1]
        allowed = {("127.0.0.1", echo_port)}
        server, proxy = await px.start(allowed, self.log, "127.0.0.1", 0)
        port = server.sockets[0].getsockname()[1]
        results = {}
        results["ok"] = await self.exchange(
            port, f"CONNECT 127.0.0.1:{echo_port} HTTP/1.1\r\nHost: x\r\n\r\n".encode(), b"hello"
        )
        results["port"] = await self.exchange(port, f"CONNECT 127.0.0.1:{echo_port + 1} HTTP/1.1\r\n\r\n".encode())
        results["get"] = await self.exchange(port, b"GET http://127.0.0.1/ HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n")
        results["garbage"] = await self.exchange(port, b"garbage\r\n\r\n")
        results["bad_target"] = await self.exchange(port, b"CONNECT nonsense: HTTP/1.1\r\n\r\n")
        await asyncio.sleep(0.2)
        server.close()
        echo_server.close()
        proxy.log.close()
        return echo_port, results

    def test_end_to_end(self):
        echo_port, results = asyncio.run(self.scenario())
        self.assertEqual(results["ok"], ("HTTP/1.1 200 Connection established", b"hello"))
        self.assertTrue(results["port"][0].startswith("HTTP/1.1 403"))
        self.assertTrue(results["get"][0].startswith("HTTP/1.1 403"))
        self.assertTrue(results["garbage"][0].startswith("HTTP/1.1 400"))
        self.assertTrue(results["bad_target"][0].startswith("HTTP/1.1 400"))
        records = [json.loads(line) for line in self.log.read_text().splitlines()]
        requests = [r for r in records if r.get("event") != "close"]
        closes = [r for r in records if r.get("event") == "close"]
        self.assertEqual(len(requests), 5)
        by_target = {r["target"]: r for r in requests}
        ok = by_target[f"127.0.0.1:{echo_port}"]
        self.assertTrue(ok["allowed"])
        self.assertEqual(ok["status"], 200)
        self.assertEqual(ok["method"], "CONNECT")
        self.assertFalse(by_target[f"127.0.0.1:{echo_port + 1}"]["allowed"])
        self.assertEqual(by_target["http://127.0.0.1/"]["status"], 403)
        self.assertEqual(len([r for r in requests if r["status"] == 400]), 2)
        self.assertEqual(len(closes), 1)
        self.assertEqual(closes[0]["bytes_up"], 5)
        self.assertEqual(closes[0]["bytes_down"], 5)

    def check(self, log_lines, allow_text):
        log = self.dir / "c.log"
        allow = self.dir / "allow.txt"
        log.write_text("".join(json.dumps(r) + "\n" for r in log_lines))
        allow.write_text(allow_text)
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = px.main(["--check-log", str(log), "--allow", str(allow)])
        return code, out.getvalue()

    def test_check_log(self):
        good = {"method": "CONNECT", "target": "a.example:443", "allowed": True, "status": 200}
        close = {"event": "close", "target": "a.example:443"}
        code, out = self.check([good, close], "a.example\n")
        self.assertEqual(code, 0)
        self.assertIn("proxy log: 1 requests, all on the allowlist", out)
        refused = {"method": "CONNECT", "target": "b.example:443", "allowed": False, "status": 403}
        code, out = self.check([good, refused], "a.example\n")
        self.assertEqual(code, 1)
        self.assertIn("b.example:443", out)
        plain = {"method": "GET", "target": "http://a.example/", "allowed": False, "status": 403}
        code, _ = self.check([plain], "a.example\n")
        self.assertEqual(code, 1)


class ManifestTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / "repo"
        (self.root / "src").mkdir(parents=True)
        (self.root / "include").mkdir()
        (self.root / "src" / "a.cpp").write_text("int a;\n")
        (self.root / "include" / "b.h").write_text("int b;\n")
        (self.root / "other.txt").write_text("not listed\n")
        self.manifest = Path(self.tmp.name) / "manifest.txt"

    def run_tool(self, *argv):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = mf.main(list(argv))
        return code, out.getvalue()

    def build(self):
        code, _ = self.run_tool("build", str(self.root), "--out", str(self.manifest))
        self.assertEqual(code, 0)

    def test_build_lists_src_and_include_sorted(self):
        self.build()
        lines = [x for x in self.manifest.read_text().splitlines() if x.endswith(("src/a.cpp", "include/b.h"))]
        self.assertEqual(len(lines), 2)
        self.assertEqual(lines, sorted(lines))
        self.assertTrue(any(line.endswith("  src/a.cpp") for line in lines))
        self.assertFalse(any("other.txt" in line for line in lines))

    def test_extra_files(self):
        extra = Path(self.tmp.name) / "x.bin"
        extra.write_bytes(b"extra")
        self.build()
        before = len(self.manifest.read_text().splitlines())
        self.run_tool("build", str(self.root), "--out", str(self.manifest), "--extra", str(extra))
        self.assertEqual(len(self.manifest.read_text().splitlines()), before + 1)

    def test_scan_finds_copy_and_rename(self):
        self.build()
        scanned = Path(self.tmp.name) / "scanned"
        scanned.mkdir()
        (scanned / "copy.cpp").write_text("int a;\n")
        (scanned / "renamed.dat").write_text("int a;\n")
        (scanned / "different.cpp").write_text("int c;\n")
        code, out = self.run_tool("scan", str(self.manifest), str(scanned))
        self.assertEqual(code, 1)
        self.assertIn("copy.cpp", out)
        self.assertIn("renamed.dat", out)
        self.assertNotIn("different.cpp", out)

    def test_scan_clean(self):
        self.build()
        scanned = Path(self.tmp.name) / "scanned"
        scanned.mkdir()
        (scanned / "different.cpp").write_text("int c;\n")
        code, out = self.run_tool("scan", str(self.manifest), str(scanned))
        self.assertEqual(code, 0)
        self.assertIn("canary: 1 files scanned, no file of the manifest", out)

    def test_size_cap(self):
        self.build()
        scanned = Path(self.tmp.name) / "scanned"
        scanned.mkdir()
        (scanned / "copy.cpp").write_text("int a;\n")
        code, out = self.run_tool("scan", str(self.manifest), str(scanned), "--max-bytes", "3")
        self.assertEqual(code, 0)
        self.assertIn("over the size cap", out)

    def test_symlinks_not_followed(self):
        self.build()
        outside = Path(self.tmp.name) / "outside"
        outside.mkdir()
        (outside / "copy.cpp").write_text("int a;\n")
        scanned = Path(self.tmp.name) / "scanned"
        scanned.mkdir()
        os.symlink(outside, scanned / "dirlink")
        os.symlink(outside / "copy.cpp", scanned / "filelink.cpp")
        code, out = self.run_tool("scan", str(self.manifest), str(scanned))
        self.assertEqual(code, 0)
        self.assertIn("0 files scanned", out)

    def test_unreadable_is_counted(self):
        self.build()
        scanned = Path(self.tmp.name) / "scanned"
        scanned.mkdir()
        secret = scanned / "secret.cpp"
        secret.write_text("int c;\n")
        secret.chmod(0)
        self.addCleanup(secret.chmod, 0o600)
        code, out = self.run_tool("scan", str(self.manifest), str(scanned))
        self.assertEqual(code, 0)
        if os.geteuid() != 0:
            self.assertIn("1 files unreadable", out)


if __name__ == "__main__":
    unittest.main()
