"""Tests for the clean-room proxy and canary manifest, on localhost and temp trees only.

    python3 tools/test_cleanroom.py
    uv run tools/test_cleanroom.py
"""

import asyncio
import contextlib
import http.server
import hashlib
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest import mock
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "cleanroom"))

import broker as bk
import canary as cn
import launch as ln
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

class LaunchRefusalTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        base = Path(self.tmp.name).resolve()
        self.home = base / "home"
        self.repo = base / "repo"
        (self.home / ".claude").mkdir(parents=True)
        (self.home / ".ta").mkdir()
        (self.repo / "build" / "scratch" / "t").mkdir(parents=True)
        (self.repo / "src").mkdir()
        (base / "other").mkdir()
        self.roots = [self.repo]

    def mount(self, path, role="tree"):
        return ln.check_mount(path, role, self.roots, self.home)

    def test_accepts_scratch_and_outside_directories(self):
        self.assertEqual(self.mount(self.repo / "build" / "scratch" / "t"), self.repo / "build" / "scratch" / "t")
        self.assertEqual(self.mount(Path(self.tmp.name) / "other"), Path(self.tmp.name).resolve() / "other")

    def test_refuses_the_repository_and_its_parts(self):
        for bad in (self.repo, self.repo / "src", self.repo / "build", self.repo / "build" / "scratch"):
            with self.assertRaises(ln.Refusal, msg=str(bad)):
                self.mount(bad)

    def test_refuses_ancestors_of_the_repository_and_home(self):
        for bad in (self.repo.parent, Path("/"), self.home):
            with self.assertRaises(ln.Refusal, msg=str(bad)):
                self.mount(bad)

    def test_refuses_credential_and_memory_directories(self):
        for bad in (self.home / ".claude", self.home / ".ta"):
            with self.assertRaises(ln.Refusal):
                self.mount(bad)

    def test_refuses_a_missing_directory(self):
        with self.assertRaises(ln.Refusal):
            self.mount(self.repo / "nope")

    def test_nested_worktree_uses_its_own_scratch(self):
        inner = self.repo / ".worktrees" / "w"
        (inner / "build" / "scratch" / "x").mkdir(parents=True)
        (inner / "src").mkdir()
        roots = [self.repo, inner]
        ln.check_mount(inner / "build" / "scratch" / "x", "tree", roots, self.home)
        with self.assertRaises(ln.Refusal):
            ln.check_mount(inner / "src", "tree", roots, self.home)

    def test_tree_needs_its_own_git_without_the_decompilation_remote(self):
        tree = Path(self.tmp.name) / "other"
        with self.assertRaises(ln.Refusal):
            ln.check_tree(tree)
        (tree / ".git").mkdir()
        (tree / ".git" / "config").write_text("[remote \"origin\"]\n\turl = https://github.com/ozgb/btm.git\n")
        ln.check_tree(tree)
        (tree / ".git" / "config").write_text("[remote \"origin\"]\n\turl = https://github.com/x/Byte-Tactics.git\n")
        with self.assertRaises(ln.Refusal):
            ln.check_tree(tree)

    def test_overlapping_mounts(self):
        a = Path(self.tmp.name)
        with self.assertRaises(ln.Refusal):
            ln.check_disjoint(("tree", a), ("data kit", a / "other"))
        with self.assertRaises(ln.Refusal):
            ln.check_disjoint(("tree", a / "home"), ("data kit", a / "other"), ("session", a / "home" / "s"))
        ln.check_disjoint(("tree", a / "home"), ("data kit", a / "other"))

    def test_session_directory_gets_the_mount_rules(self):
        scratch = self.repo / "build" / "scratch" / "t"
        self.assertEqual(self.mount(scratch, "session"), scratch)
        for bad in (self.repo / "src", self.home / ".claude", self.repo):
            with self.assertRaises(ln.Refusal, msg=str(bad)):
                self.mount(bad, "session")
        manifest = Path(self.tmp.name) / "m.txt"
        (self.repo / "src" / "a.c").write_text("secret\n")
        manifest.write_text(f"{mf.sha256_file(self.repo / 'src' / 'a.c')}  src/a.c\n")
        (scratch / "copy.txt").write_text("secret\n")
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(ln.Refusal):
                ln.check_scan_clean(manifest, scratch)

    def test_canary_directory_must_be_gone_before_the_agent_starts(self):
        calls = []

        def runner(argv, **kw):
            calls.append(argv[argv.index("--") + 1:])
            return subprocess.CompletedProcess(argv, self.still_there if argv[-4] == "test" else 0)

        self.still_there = 0
        self.assertEqual(ln.remove_canary("vm", runner), 0)
        self.assertEqual(calls, [["rm", "-rf", "/opt/canary"], ["test", "!", "-e", "/opt/canary"]])
        self.still_there = 1
        self.assertEqual(ln.remove_canary("vm", runner), 1)

    def test_allowlist_rules(self):
        ln.check_allowlist_text("api.anthropic.com\nindex.crates.io\n")
        for bad in ("", "github.com\n", "api.github.com\n", "raw.githubusercontent.com\n", "10.0.0.1:80\n"):
            with self.assertRaises(ln.Refusal, msg=bad):
                ln.check_allowlist_text(bad)
        ln.check_allowlist_text("notgithub.com\n")

    def test_api_key_variable_name(self):
        ln.check_api_key_env(None)
        ln.check_api_key_env("ANTHROPIC_API_KEY_IMPL")
        for bad in ("GH_TOKEN", "SSH_AUTH_SOCK", "OPENAI_API_KEY"):
            with self.assertRaises(ln.Refusal):
                ln.check_api_key_env(bad)

    def test_refuses_a_mount_holding_a_manifest_file(self):
        root = Path(self.tmp.name) / "m"
        (root / "src").mkdir(parents=True)
        (root / "src" / "a.cpp").write_text("secret\n")
        manifest = Path(self.tmp.name) / "manifest.txt"
        mf.build(root, manifest, [])
        leaked = Path(self.tmp.name) / "tree-with-copy"
        leaked.mkdir()
        (leaked / "renamed.txt").write_text("secret\n")
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(ln.Refusal):
                ln.check_scan_clean(manifest, leaked)
            ln.check_scan_clean(manifest, self.home)

    def test_create_argv_has_exactly_the_mounts_and_no_network(self):
        argv = ln.create_argv("n", "/i.smolmachine", "/t", "/d", "/s", "/p.sock")
        ln.check_argv(argv)
        self.assertNotIn("--net", argv)
        self.assertEqual(argv.count("-v"), 4)
        self.assertFalse([a for a in argv if "sync" in a])
        for extra in ("--net", "--ssh-agent", "--allow-system-mounts", "--allow-host-loopback"):
            with self.assertRaises(ln.Refusal):
                ln.check_argv(argv + [extra])
        with self.assertRaises(ln.Refusal):
            ln.check_argv(argv + ["-v", "/home:/home"])


class CanaryTests(unittest.TestCase):
    def test_host_paths(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertTrue(cn.check_host_paths([os.path.join(tmp, "absent")])[0])
            self.assertFalse(cn.check_host_paths([tmp])[0])

    def test_home_and_environment(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertTrue(cn.check_home(tmp)[0])
            os.mkdir(os.path.join(tmp, ".ssh"))
            self.assertFalse(cn.check_home(tmp)[0])

    def test_managed_settings_must_deny_the_web_tools(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "managed-settings.json"
            self.assertFalse(cn.check_managed_settings(path)[0])
            for text in ("not json", "{}", '{"permissions": {"deny": ["WebSearch"]}}', '{"permissions": {"allow": ["WebFetch"]}}'):
                path.write_text(text)
                self.assertFalse(cn.check_managed_settings(path)[0], text)
            path.write_text('{"permissions": {"deny": ["WebFetch", "WebSearch", "Bash(x)"]}}')
            self.assertTrue(cn.check_managed_settings(path)[0])
            shipped = Path(__file__).resolve().parent / "cleanroom" / "managed-settings.json"
            self.assertTrue(cn.check_managed_settings(shipped)[0])

    def test_scanner_selftest(self):
        self.assertTrue(cn.check_scanner_works(mf.DEFAULT_MAX)[0])

    def test_manifest_scan_check(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "f").write_text("x\n")
            manifest = tmp / "m"
            manifest.write_text(f"{mf.sha256_file(tmp / 'f')}  f\n")
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertFalse(cn.check_manifest_scan(manifest, [str(tmp)], mf.DEFAULT_MAX)[0])
            clean = tmp / "clean"
            clean.mkdir()
            (clean / "g").write_text("y\n")
            self.assertTrue(cn.check_manifest_scan(manifest, [str(clean)], mf.DEFAULT_MAX)[0])

    def test_proxy_refusal_check_against_a_unix_proxy(self):
        with tempfile.TemporaryDirectory() as tmp:
            sock = os.path.join(tmp, "p.sock")
            log = os.path.join(tmp, "log")

            async def scenario():
                server, proxy = await px.start({("api.example.com", 443)}, log, sock)
                result = await asyncio.get_running_loop().run_in_executor(None, cn.check_proxy_refuses, sock)
                server.close()
                proxy.log.close()
                return result

            ok, why = asyncio.run(scenario())
            self.assertTrue(ok, why)
            self.assertFalse(cn.check_proxy_refuses(os.path.join(tmp, "missing"))[0])
            records = [json.loads(line) for line in Path(log).read_text().splitlines()]
            self.assertEqual(len(records), len(cn.PROXY_REFUSED))
            self.assertTrue(all(r["status"] == 403 and r["client"] == "unix" for r in records))


def client_hello(name):
    """Return a minimal TLS ClientHello record naming a server."""
    host = name.encode()
    sni = b"\0\0" + (len(host) + 5).to_bytes(2, "big") + (len(host) + 3).to_bytes(2, "big") + b"\0" + len(host).to_bytes(2, "big") + host
    body = b"\3\3" + bytes(32) + b"\0" + b"\0\2\0\x2f" + b"\1\0" + len(sni).to_bytes(2, "big") + sni
    hello = b"\1" + len(body).to_bytes(3, "big") + body
    return b"\x16\3\1" + len(hello).to_bytes(2, "big") + hello


class SniTests(unittest.TestCase):
    def test_parse_sni(self):
        self.assertEqual(px.parse_sni(client_hello("API.Anthropic.com")), "api.anthropic.com")
        self.assertIsNone(px.parse_sni(b"GET / HTTP/1.1\r\n\r\n"))
        self.assertIsNone(px.parse_sni(client_hello("x")[:20]))
        self.assertIsNone(px.parse_sni(b""))

    def test_tunnel_needs_a_matching_server_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = os.path.join(tmp, "log")

            async def one(port, name, echo_port, payload):
                reader, writer = await asyncio.open_connection("127.0.0.1", port)
                writer.write(f"CONNECT {name}:{echo_port} HTTP/1.1\r\n\r\n".encode())
                await asyncio.wait_for(reader.readuntil(b"\r\n\r\n"), 5)
                writer.write(payload)
                await writer.drain()
                data = await asyncio.wait_for(reader.read(len(payload)), 5)
                writer.close()
                return data

            async def scenario():
                async def echo(reader, writer):
                    while data := await reader.read(1024):
                        writer.write(data)
                        await writer.drain()
                    writer.close()

                echo_server = await asyncio.start_server(echo, "127.0.0.1", 0)
                echo_port = echo_server.sockets[0].getsockname()[1]
                server, proxy = await px.start({("api.example.test", echo_port)}, log, "127.0.0.1", 0)
                port = server.sockets[0].getsockname()[1]
                good = client_hello("API.example.test")
                results = [
                    await one(port, "api.example.test", echo_port, good),
                    await one(port, "api.example.test", echo_port, client_hello("evil.example.test")),
                    await one(port, "api.example.test", echo_port, b"GET / HTTP/1.1\r\n\r\n"),
                ]
                server.close()
                echo_server.close()
                proxy.log.close()
                return good, results

            async def public(host, port):
                return ["127.0.0.1"]

            with mock.patch.object(px, "resolve_public", public):
                good, results = asyncio.run(scenario())
            self.assertEqual(results, [good, b"", b""])
            events = [json.loads(line) for line in Path(log).read_text().splitlines()]
            refused = [r for r in events if r.get("event") == "sni_refused"]
            self.assertEqual([r["sni"] for r in refused], ["evil.example.test", None])


class ProxyHardeningTests(unittest.TestCase):
    def test_name_resolving_to_loopback_is_refused_with_502(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = os.path.join(tmp, "log")

            async def scenario():
                async def echo(reader, writer):
                    writer.close()

                echo_server = await asyncio.start_server(echo, "127.0.0.1", 0)
                echo_port = echo_server.sockets[0].getsockname()[1]
                server, proxy = await px.start({("localhost", echo_port)}, log, "127.0.0.1", 0)
                port = server.sockets[0].getsockname()[1]
                reader, writer = await asyncio.open_connection("127.0.0.1", port)
                writer.write(f"CONNECT localhost:{echo_port} HTTP/1.1\r\n\r\n".encode())
                head = await asyncio.wait_for(reader.readuntil(b"\r\n\r\n"), 5)
                writer.close()
                server.close()
                echo_server.close()
                proxy.log.close()
                return head

            self.assertTrue(asyncio.run(scenario()).startswith(b"HTTP/1.1 502"))

    def test_reopen_writes_to_a_fresh_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp) / "log"
            proxy = px.Proxy(set(), log)
            proxy.write({"n": 1})
            os.replace(log, Path(tmp) / "old")
            proxy.reopen()
            proxy.write({"n": 2})
            proxy.log.close()
            self.assertEqual(json.loads(log.read_text())["n"], 2)
            self.assertEqual(json.loads((Path(tmp) / "old").read_text())["n"], 1)


class BrokerTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.session = bk.prepare(Path(self.tmp.name) / "s")

    def request(self, name, wasm=b"\0asm\1\0\0\0", subsystem="parser", ready=True):
        d = self.session / "outbox" / name
        d.mkdir()
        (d / "module.wasm").write_bytes(wasm)
        (d / "subsystem").write_text(subsystem + "\n")
        if ready:
            (d / "ready").write_text("")

    def test_stub_gate_answers_a_ready_request_once(self):
        self.request("r1")
        self.request("r2", ready=False)
        self.assertEqual(bk.process_mailbox(self.session, bk.stub_gate), ["r1"])
        result = json.loads((self.session / "inbox" / "r1.json").read_text())
        self.assertEqual(result["verdict"], bk.STUB_VERDICT)
        self.assertEqual(result["subsystem"], "parser")
        self.assertEqual(bk.process_mailbox(self.session, bk.stub_gate), [])
        self.assertFalse((self.session / "inbox" / "r2.json").exists())

    def test_bad_requests_get_an_error_result(self):
        self.request("bad1", wasm=b"not wasm")
        self.request("bad2", subsystem="../etc")
        self.request("bad3", subsystem="Parser")
        bk.process_mailbox(self.session, bk.stub_gate)
        for name in ("bad1", "bad2", "bad3"):
            self.assertIn("error", json.loads((self.session / "inbox" / f"{name}.json").read_text()))

    def test_symlinked_module_is_refused(self):
        self.request("link")
        module = self.session / "outbox" / "link" / "module.wasm"
        module.unlink()
        module.symlink_to("/etc/passwd")
        bk.process_mailbox(self.session, bk.stub_gate)
        self.assertIn("error", json.loads((self.session / "inbox" / "link.json").read_text()))

    def commit(self, repo_dir, text):
        env = {"GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t", "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@t"}
        env = {**os.environ, **env, "GIT_CONFIG_GLOBAL": "/dev/null", "GIT_CONFIG_NOSYSTEM": "1"}
        (repo_dir / "f").write_text(text)
        subprocess.run(["git", "-C", str(repo_dir), "add", "f"], check=True, env=env)
        subprocess.run(["git", "-C", str(repo_dir), "commit", "-qm", text], check=True, env=env)

    def make_repo(self, name="work"):
        work = Path(self.tmp.name) / name
        subprocess.run(["git", "init", "-q", "-b", "impl/w", str(work)], check=True)
        return work

    def bundle(self, work, target, *revs):
        subprocess.run(["git", "-C", str(work), "bundle", "create", "-q", str(target), *revs], check=True)

    def head(self, ref="refs/heads/impl/w"):
        bare = self.session / "bare" / bk.REPO_NAME
        out = subprocess.run(["git", "--git-dir", str(bare), "rev-parse", "--verify", "-q", ref], capture_output=True, text=True)
        return out.stdout.strip()

    @unittest.skipUnless(shutil.which("git"), "needs git")
    def test_bundle_fetches_fast_forwards_and_refuses_the_rest(self):
        work = self.make_repo()
        outbox = self.session / "outbox" / bk.BUNDLE_NAME
        self.assertEqual(bk.sync_refs(self.session), "")
        self.commit(work, "one")
        self.bundle(work, outbox, "impl/w")
        bk.sync_refs(self.session)
        first = self.head()
        self.assertNotEqual(first, "")
        self.commit(work, "two")
        self.bundle(work, outbox, "impl/w")
        bk.sync_refs(self.session)
        second = self.head()
        self.assertNotEqual(first, second)
        subprocess.run(["git", "-C", str(work), "checkout", "-q", "--orphan", "other"], check=True)
        self.commit(work, "three")
        subprocess.run(["git", "-C", str(work), "branch", "-f", "impl/w", "other"], check=True)
        self.bundle(work, outbox, "impl/w")
        out = bk.sync_refs(self.session)
        self.assertEqual(self.head(), second)
        self.assertIn("rejected", out)
        self.assertIn("sync refused", (self.session / "broker.log").read_text())

    @unittest.skipUnless(shutil.which("git"), "needs git")
    def test_unchanged_bundle_is_processed_once(self):
        work = self.make_repo()
        self.commit(work, "one")
        self.bundle(work, self.session / "outbox" / bk.BUNDLE_NAME, "impl/w")
        bk.sync_refs(self.session)
        bk.sync_refs(self.session)
        self.assertEqual((self.session / "broker.log").read_text().count("sync:"), 1)

    @unittest.skipUnless(shutil.which("git"), "needs git")
    def test_symlinked_bundle_is_refused(self):
        host = self.make_repo("host")
        self.commit(host, "SECRET-DECOMPILED")
        real = Path(self.tmp.name) / "host.bundle"
        self.bundle(host, real, "impl/w")
        (self.session / "outbox" / bk.BUNDLE_NAME).symlink_to(real)
        bk.sync_refs(self.session)
        self.assertEqual(self.head(), "")
        self.assertIn("sync refused", (self.session / "broker.log").read_text())

    @unittest.skipUnless(shutil.which("git"), "needs git")
    def test_bundle_with_prerequisites_the_host_lacks_is_refused(self):
        host = self.make_repo("host")
        self.commit(host, "SECRET-DECOMPILED")
        self.commit(host, "later")
        self.bundle(host, self.session / "outbox" / bk.BUNDLE_NAME, "impl/w~1..impl/w")
        bk.sync_refs(self.session)
        self.assertEqual(self.head(), "")
        self.assertIn("does not verify", (self.session / "broker.log").read_text())

    @unittest.skipUnless(shutil.which("git"), "needs git")
    def test_old_sync_repository_attacks_reach_nothing(self):
        host = self.make_repo("host")
        self.commit(host, "SECRET-DECOMPILED")
        sync = self.session / "sync"
        sync.mkdir()
        (sync / bk.REPO_NAME).symlink_to(host / ".git")
        bk.process_mailbox(self.session, bk.stub_gate)
        bk.sync_refs(self.session)
        self.assertEqual(self.head(), "")
        (sync / bk.REPO_NAME).unlink()
        subprocess.run(["git", "init", "-q", "--bare", str(sync / bk.REPO_NAME)], check=True)
        info = sync / bk.REPO_NAME / "objects" / "info"
        info.mkdir(exist_ok=True)
        (info / "alternates").write_text(str(host / ".git" / "objects") + "\n")
        bk.sync_refs(self.session)
        self.assertEqual(self.head(), "")
        bare = self.session / "bare" / bk.REPO_NAME
        secret = subprocess.run(["git", "--git-dir", str(bare), "cat-file", "-t", self.host_head(host)], capture_output=True)
        self.assertNotEqual(secret.returncode, 0)

    def host_head(self, host):
        return subprocess.run(["git", "-C", str(host), "rev-parse", "impl/w"], capture_output=True, text=True, check=True).stdout.strip()

    @unittest.skipUnless(shutil.which("git"), "needs git")
    def test_hook_and_config_in_a_bundle_hand_over_do_not_run(self):
        work = self.make_repo()
        self.commit(work, "one")
        marker = Path(self.tmp.name) / "ran"
        hook = work / ".git" / "hooks" / "reference-transaction"
        hook.write_text(f"#!/bin/sh\ntouch {marker}\n")
        hook.chmod(0o755)
        self.bundle(work, self.session / "outbox" / bk.BUNDLE_NAME, "impl/w")
        bk.sync_refs(self.session)
        self.assertNotEqual(self.head(), "")
        self.assertFalse(marker.exists())

    def test_symlinked_request_directory_is_ignored(self):
        self.request("real")
        outbox = self.session / "outbox"
        (outbox / "link").symlink_to(outbox / "real")
        bk.process_mailbox(self.session, bk.stub_gate)
        self.assertTrue((self.session / "inbox" / "real.json").exists())
        self.assertFalse((self.session / "inbox" / "link.json").exists())

    def test_fifo_and_oversized_modules_are_refused(self):
        self.request("fifo")
        module = self.session / "outbox" / "fifo" / "module.wasm"
        module.unlink()
        os.mkfifo(module)
        self.request("big")
        with mock.patch.object(bk, "MAX_WASM", 4):
            bk.process_mailbox(self.session, bk.stub_gate)
        for name in ("fifo", "big"):
            self.assertIn("error", json.loads((self.session / "inbox" / f"{name}.json").read_text()))

    def test_files_in_the_outbox_are_not_requests(self):
        (self.session / "outbox" / ".btm.tmp").write_text("x")
        (self.session / "outbox" / bk.BUNDLE_NAME).write_text("x")
        self.assertEqual(bk.process_mailbox(self.session, bk.stub_gate), [])
        self.assertEqual(list((self.session / "inbox").iterdir()), [])


FAKE_TOKEN = "test-token-not-real"


def make_secret(directory, name, text, mode=0o600):
    path = Path(directory) / name
    path.write_text(text)
    path.chmod(mode)
    return path


class CredentialTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name)
        self.host = self.dir / "host-credentials.json"

    def read(self, path, kind="oauth", environ=None):
        return ln.read_credential(path, kind, environ or {}, self.host)

    def test_oauth_and_api_key_names(self):
        key = make_secret(self.dir, "k", FAKE_TOKEN + "\n")
        self.assertEqual(self.read(key), ("CLAUDE_CODE_OAUTH_TOKEN", FAKE_TOKEN))
        self.assertEqual(self.read(key, "api-key"), ("ANTHROPIC_API_KEY", FAKE_TOKEN))
        with self.assertRaises(ln.Refusal):
            self.read(key, "other")

    def test_missing_empty_and_loose_files_are_refused(self):
        with self.assertRaises(ln.Refusal):
            self.read(self.dir / "absent")
        with self.assertRaises(ln.Refusal):
            self.read(make_secret(self.dir, "empty", " \n"))
        for mode in (0o640, 0o604, 0o644, 0o660):
            with self.assertRaises(ln.Refusal, msg=oct(mode)):
                self.read(make_secret(self.dir, f"m{mode:o}", FAKE_TOKEN, mode))
        with self.assertRaises(ln.Refusal):
            self.read(self.dir)

    def test_value_the_host_claude_code_uses_is_refused(self):
        key = make_secret(self.dir, "k", FAKE_TOKEN)
        for var in ("ANTHROPIC_API_KEY", "CLAUDE_CODE_OAUTH_TOKEN"):
            with self.assertRaises(ln.Refusal, msg=var):
                self.read(key, environ={var: FAKE_TOKEN})
        self.read(key, environ={"ANTHROPIC_API_KEY": "another-fake"})
        self.host.write_text(json.dumps({"claudeAiOauth": {"accessToken": FAKE_TOKEN}}))
        with self.assertRaises(ln.Refusal):
            self.read(key)
        self.host.write_text(json.dumps({"claudeAiOauth": {"accessToken": "different"}}))
        self.read(key)
        self.host.write_text("not json")
        self.read(key)

    def test_token_in_the_claude_config_dir_is_refused(self):
        config = self.dir / "cfg"
        config.mkdir()
        key = make_secret(self.dir, "k", FAKE_TOKEN)
        environ = {"CLAUDE_CONFIG_DIR": str(config)}
        self.read(key, environ=environ)
        (config / ".credentials.json").write_text(json.dumps({"claudeAiOauth": {"accessToken": FAKE_TOKEN}}))
        with self.assertRaises(ln.Refusal):
            self.read(key, environ=environ)
        self.read(key, environ={"CLAUDE_CONFIG_DIR": ""})
        self.read(key)

    def test_default_host_credentials_follow_home_at_call_time(self):
        home = self.dir / "home"
        (home / ".claude").mkdir(parents=True)
        (home / ".claude" / ".credentials.json").write_text(json.dumps({"claudeAiOauth": {"accessToken": FAKE_TOKEN}}))
        with mock.patch.dict(os.environ, {"HOME": str(home)}):
            self.assertEqual(ln.host_claude_tokens({}), {FAKE_TOKEN})

    def test_refusal_text_does_not_hold_the_value(self):
        key = make_secret(self.dir, "k", FAKE_TOKEN)
        with self.assertRaises(ln.Refusal) as caught:
            self.read(key, environ={"ANTHROPIC_API_KEY": FAKE_TOKEN})
        self.assertNotIn(FAKE_TOKEN, str(caught.exception))

    def test_session_may_be_checked_before_it_exists(self):
        base = self.dir / "repo"
        (base / "src").mkdir(parents=True)
        later = self.dir / "new-session"
        got = ln.check_mount(later, "session", [base], self.dir / "home", must_exist=False)
        self.assertEqual(got, later.resolve())
        self.assertFalse(later.exists())
        with self.assertRaises(ln.Refusal):
            ln.check_mount(base / "src" / "s", "session", [base], self.dir / "home", must_exist=False)
        self.assertFalse((base / "src" / "s").exists())

    def test_bt_cleanroom_directory_is_never_mounted(self):
        home = self.dir / "home"
        vault = home / ".config" / "bt-cleanroom"
        vault.mkdir(parents=True)
        (self.dir / "repo").mkdir()
        for bad in (vault, vault.parent, home):
            with self.assertRaises(ln.Refusal, msg=str(bad)):
                ln.check_mount(bad, "tree", [self.dir / "repo"], home)
        inside = vault / "sub"
        inside.mkdir()
        with self.assertRaises(ln.Refusal):
            ln.check_mount(inside, "tree", [self.dir / "repo"], home)
        self.assertIn(str(vault), ln.host_paths([], home))

    def test_claude_config_dir_is_never_mounted(self):
        home = self.dir / "home"
        config = self.dir / "elsewhere" / "claude-cfg"
        (config / "sub").mkdir(parents=True)
        home.mkdir()
        (self.dir / "repo").mkdir()
        self.assertIsNone(ln.claude_config_dir({}))
        self.assertIsNone(ln.claude_config_dir({"CLAUDE_CONFIG_DIR": ""}))
        found = ln.claude_config_dir({"CLAUDE_CONFIG_DIR": str(config)})
        self.assertEqual(found, config.resolve())
        ln.check_mount(config, "tree", [self.dir / "repo"], home)
        for bad in (config, config.parent, config / "sub"):
            with self.assertRaises(ln.Refusal, msg=str(bad)):
                ln.check_mount(bad, "tree", [self.dir / "repo"], home, config_dir=found)
        with self.assertRaises(ln.Refusal):
            ln.check_mount(config / "new", "session", [self.dir / "repo"], home, must_exist=False, config_dir=found)
        self.assertIn(str(found), ln.host_paths([], home, found))
        self.assertNotIn(str(found), ln.host_paths([], home))

    def test_canary_finds_the_directory_in_the_guest_home(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertTrue(cn.check_home(tmp)[0])
            os.makedirs(os.path.join(tmp, ".config", "bt-cleanroom"))
            self.assertFalse(cn.check_home(tmp)[0])

    def test_check_log_counts_refused_server_names(self):
        log = self.dir / "p.log"
        allow = self.dir / "allow.txt"
        allow.write_text("a.example\n")
        records = [{"method": "CONNECT", "target": "a.example:443", "allowed": True, "status": 200},
                   {"event": "sni_refused", "target": "a.example:443", "sni": "evil.example"}]
        log.write_text("".join(json.dumps(r) + "\n" for r in records))
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = px.main(["--check-log", str(log), "--allow", str(allow)])
        self.assertEqual(code, 1)
        self.assertIn("1 tunnels refused", out.getvalue())


class FakeGate:
    """A judge that writes result.json, logs/ and a stray file the way oraclegate does."""

    def __init__(self, directory):
        self.path = Path(directory) / "fake_gate.py"
        self.path.write_text(
            "import json, sys\nfrom pathlib import Path\n"
            "out = Path(sys.argv[sys.argv.index('--out') + 1])\n"
            "(out / 'logs').mkdir()\n(out / 'logs' / 'reference.txt').write_text('REFERENCE-VALUES')\n"
            "(out / 'runtime').mkdir()\n(out / 'extra.txt').write_text('EXTRA')\n"
            "mode = sys.argv[sys.argv.index('--subsystem') + 1]\n"
            "if mode == 'silent':\n    sys.exit(3)\n"
            "(out / 'result.json').write_text(json.dumps({'verdict': 'equal', 'subsystem': mode}))\n"
            "print('LOG-ONLY-TEXT')\n"
        )

    def command(self):
        return [sys.executable, str(self.path)]


class RealGateTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.session = bk.prepare(Path(self.tmp.name) / "s")
        self.fake = FakeGate(self.tmp.name)

    def request(self, name, subsystem):
        d = self.session / "outbox" / name
        d.mkdir()
        (d / "module.wasm").write_bytes(b"\0asm\1\0\0\0")
        (d / "subsystem").write_text(subsystem + "\n")
        (d / "ready").write_text("")

    def test_only_result_json_crosses(self):
        self.request("r1", "wind_meteors")
        gate = bk.make_gate(self.session, self.fake.command())
        self.assertEqual(bk.process_mailbox(self.session, gate), ["r1"])
        inbox = sorted(p.name for p in (self.session / "inbox").iterdir())
        self.assertEqual(inbox, ["r1.json"])
        result = json.loads((self.session / "inbox" / "r1.json").read_text())
        self.assertEqual(result["verdict"], "equal")
        self.assertEqual(result["subsystem"], "wind_meteors")
        text = (self.session / "inbox" / "r1.json").read_text()
        for secret in ("REFERENCE-VALUES", "LOG-ONLY-TEXT", "EXTRA"):
            self.assertNotIn(secret, text)
        kept = self.session / "gate" / "r1" / "out" / "logs" / "reference.txt"
        self.assertTrue(kept.exists())

    def test_game_dir_reaches_the_gate_command(self):
        rec = Path(self.tmp.name) / "argv.json"
        script = Path(self.tmp.name) / "rec.py"
        script.write_text(f"import json, sys\nopen({str(rec)!r}, 'w').write(json.dumps(sys.argv))\n")
        self.request("rg", "x")
        bk.process_mailbox(self.session, bk.make_gate(self.session, [sys.executable, str(script)], game_dir="/some/game"))
        argv = json.loads(rec.read_text())
        self.assertEqual(argv[argv.index("--game-dir") + 1], "/some/game")
        bk.process_mailbox(self.session, bk.make_gate(self.session, [sys.executable, str(script)]))
        self.request("rh", "x")
        bk.process_mailbox(self.session, bk.make_gate(self.session, [sys.executable, str(script)]))
        self.assertNotIn("--game-dir", json.loads(rec.read_text()))

    def test_launch_forwards_game_dir_to_the_broker(self):
        self.assertEqual(ln.broker_argv("/s", "/g")[-2:], ["--game-dir", "/g"])
        self.assertNotIn("--game-dir", ln.broker_argv("/s"))

    def test_missing_result_is_an_error_without_the_logs(self):
        self.request("r2", "silent")
        gate = bk.make_gate(self.session, self.fake.command())
        bk.process_mailbox(self.session, gate)
        text = (self.session / "inbox" / "r2.json").read_text()
        self.assertIn("error", json.loads(text))
        self.assertNotIn("REFERENCE-VALUES", text)

    def test_timeout_is_an_error(self):
        sleeper = Path(self.tmp.name) / "sleep.py"
        sleeper.write_text("import time\ntime.sleep(30)\n")
        self.request("r3", "x")
        gate = bk.make_gate(self.session, [sys.executable, str(sleeper)], timeout=1)
        bk.process_mailbox(self.session, gate)
        self.assertIn("timed out", json.loads((self.session / "inbox" / "r3.json").read_text())["error"])

    def test_filter_refuses_symlinks_non_objects_and_big_files(self):
        d = Path(self.tmp.name)
        target = d / "t.json"
        target.write_text("{}")
        (d / "link.json").symlink_to(target)
        (d / "list.json").write_text("[1]")
        (d / "big.json").write_text("{}")
        with mock.patch.object(bk, "MAX_RESULT", 1):
            self.assertIn("error", bk.filter_result(d / "big.json", "i", 0))
        self.assertIn("error", bk.filter_result(d / "link.json", "i", 0))
        self.assertIn("error", bk.filter_result(d / "list.json", "i", 0))
        self.assertIn("error", bk.filter_result(d / "none.json", "i", 0))
        self.assertEqual(bk.filter_result(target, "i", 0)["id"], "i")


class BranchRuleTests(unittest.TestCase):
    def test_accepted_and_refused_names(self):
        for ok in ("impl/a", "impl/wind-meteors", "impl/a/b.c_d", "impl/0x/y-1", "impl/v1.2"):
            self.assertTrue(bk.branch_allowed(f"refs/heads/{ok}"), ok)
        for bad in ("main", "impl", "impl/", "impl//a", "impl/-x", "impl/a/-x", "impl/A", "impl/a b", "impl/a..b", "impl/a.lock",
                    "impl/a.", "impl/.a", "impl/a/", "impl/../main", "other/a", "-x", "impl/a@{1}", "impl/a~1", "impl/a:b"):
            self.assertFalse(bk.branch_allowed(f"refs/heads/{bad}"), bad)
        for bad in ("refs/tags/impl/a", "refs/heads/main", "refs/remotes/impl/a", "impl/a", "refs/heads/impl/" + "a" * 200):
            self.assertFalse(bk.branch_allowed(bad), bad)


class PushHandler(http.server.BaseHTTPRequestHandler):
    requests = []
    status = 201
    body = b""

    def do_POST(self):
        body = self.rfile.read(int(self.headers["Content-Length"]))
        PushHandler.requests.append((self.path, self.headers["Authorization"], json.loads(body)))
        self.send_response(PushHandler.status)
        self.end_headers()
        self.wfile.write(PushHandler.body or json.dumps({"html_url": "https://example.invalid/pull/1"}).encode())

    def log_message(self, *args):
        pass


@unittest.skipUnless(shutil.which("git"), "needs git")
class PushTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name)
        self.session = bk.prepare(self.dir / "s")
        self.bare = self.session / "bare" / bk.REPO_NAME
        self.remote = self.dir / "remote.git"
        subprocess.run(["git", "init", "-q", "--bare", str(self.remote)], check=True)
        self.token = make_secret(self.dir, "btm-token", FAKE_TOKEN + "\n")
        self.wasm_dir = self.dir / "wasm"
        self.wasm_dir.mkdir()
        (self.session / bk.MANIFEST_NAME).write_text(hashlib.sha256(b"canary").hexdigest() + "  src/canary.cpp\n")
        PushHandler.requests = []
        PushHandler.status = 201
        self.server = http.server.HTTPServer(("127.0.0.1", 0), PushHandler)
        threading.Thread(target=self.server.serve_forever, daemon=True).start()
        self.addCleanup(self.server.server_close)
        self.addCleanup(self.server.shutdown)
        PushHandler.body = b""
        self.api = f"http://127.0.0.1:{self.server.server_port}"
        self.work = self.dir / "work"
        subprocess.run(["git", "init", "-q", "-b", "main", str(self.work)], check=True)
        env = {**os.environ, "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t", "GIT_COMMITTER_NAME": "t",
               "GIT_COMMITTER_EMAIL": "t@t", "GIT_CONFIG_GLOBAL": "/dev/null"}
        (self.work / "f").write_text("x")
        subprocess.run(["git", "-C", str(self.work), "add", "f"], check=True, env=env)
        subprocess.run(["git", "-C", str(self.work), "commit", "-qm", "one"], check=True, env=env)

    def bundle(self, *branches):
        for b in branches:
            if b != "main" and subprocess.run(["git", "-C", str(self.work), "rev-parse", "-q", "--verify", b], capture_output=True).returncode:
                subprocess.run(["git", "-C", str(self.work), "branch", b, "main"], check=True)
        subprocess.run(["git", "-C", str(self.work), "bundle", "create", "-q", str(self.session / "outbox" / bk.BUNDLE_NAME), *branches], check=True)
        (self.session / bk.STATE_NAME).unlink(missing_ok=True)

    def remote_refs(self):
        out = subprocess.run(["git", "--git-dir", str(self.remote), "for-each-ref", "--format=%(refname)"], capture_output=True, text=True).stdout
        return out.split()

    def push(self, token_file=None):
        bk.push_branches(self.session, token_file or self.token, str(self.remote), "ozgb/btm", self.api, wasm_dirs=[self.wasm_dir])

    def commit_file(self, branch, name, content):
        env = {**os.environ, "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t", "GIT_COMMITTER_NAME": "t",
               "GIT_COMMITTER_EMAIL": "t@t", "GIT_CONFIG_GLOBAL": "/dev/null"}
        subprocess.run(["git", "-C", str(self.work), "checkout", "-q", "-B", branch, "main"], check=True)
        (self.work / name).write_bytes(content)
        subprocess.run(["git", "-C", str(self.work), "add", name], check=True, env=env)
        subprocess.run(["git", "-C", str(self.work), "commit", "-qm", "two"], check=True, env=env)
        subprocess.run(["git", "-C", str(self.work), "checkout", "-q", "main"], check=True)

    def refused(self, branch, content_name="f.txt", content=b"y"):
        self.commit_file(branch, content_name, content)
        self.bundle(branch)
        bk.sync_refs(self.session)
        self.push()
        self.assertEqual(self.remote_refs(), [])
        self.assertEqual(PushHandler.requests, [])
        self.assertNotIn("refs/heads/" + branch, (self.session / bk.PUSH_STATE).read_text() if (self.session / bk.PUSH_STATE).exists() else "")
        log = (self.session / "broker.log").read_text()
        self.assertIn(f"push refused for refs/heads/{branch}", log)
        self.assertNotIn(content.decode(errors="replace") + "\n", log)
        return log

    def test_wasm_path_is_refused_whatever_the_case(self):
        log = self.refused("impl/w", "Mod.WASM", b"not even wasm")
        self.assertIn("Mod.WASM", log)

    def test_manifest_content_is_refused(self):
        secret = b"secret source text"
        (self.session / bk.MANIFEST_NAME).write_text(hashlib.sha256(secret).hexdigest() + "  src/x.cpp\n")
        log = self.refused("impl/m", "innocent.txt", secret)
        self.assertIn("innocent.txt", log)
        self.assertNotIn("src/x.cpp", log)

    def test_build_wasm_content_is_refused(self):
        (self.wasm_dir / "sub").mkdir(parents=True)
        (self.wasm_dir / "sub" / "m.wasm").write_bytes(b"\0asm\1\0\0\0module")
        self.refused("impl/b", "renamed.bin", b"\0asm\1\0\0\0module")

    def test_content_in_an_earlier_commit_is_refused(self):
        self.commit_file("impl/h", "a.wasm", b"1")
        subprocess.run(["git", "-C", str(self.work), "checkout", "-q", "impl/h"], check=True)
        env = {**os.environ, "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t", "GIT_COMMITTER_NAME": "t",
               "GIT_COMMITTER_EMAIL": "t@t", "GIT_CONFIG_GLOBAL": "/dev/null"}
        subprocess.run(["git", "-C", str(self.work), "rm", "-q", "a.wasm"], check=True, env=env)
        subprocess.run(["git", "-C", str(self.work), "commit", "-qm", "gone"], check=True, env=env)
        subprocess.run(["git", "-C", str(self.work), "checkout", "-q", "main"], check=True)
        self.bundle("impl/h")
        bk.sync_refs(self.session)
        self.push()
        self.assertEqual(self.remote_refs(), [])

    def test_missing_manifest_refuses(self):
        (self.session / bk.MANIFEST_NAME).unlink()
        self.bundle("impl/ok")
        bk.sync_refs(self.session)
        self.push()
        self.assertEqual(self.remote_refs(), [])
        self.assertIn("push refused for refs/heads/impl/ok", (self.session / "broker.log").read_text())

    def test_clean_branch_with_a_commit_still_pushes(self):
        self.commit_file("impl/clean", "notes.txt", b"fine")
        self.bundle("impl/clean")
        bk.sync_refs(self.session)
        self.push()
        self.assertEqual(self.remote_refs(), ["refs/heads/impl/clean"])

    def test_only_impl_branches_reach_the_bare_repository(self):
        self.bundle("main", "impl/ok", "impl/a/b", "other")
        subprocess.run(["git", "-C", str(self.work), "tag", "v1"], check=True)
        bk.sync_refs(self.session)
        refs = subprocess.run(["git", "--git-dir", str(self.bare), "for-each-ref", "--format=%(refname)"], capture_output=True, text=True).stdout.split()
        self.assertEqual(sorted(refs), ["refs/heads/impl/a/b", "refs/heads/impl/ok"])
        log = (self.session / "broker.log").read_text()
        self.assertIn("refs/heads/main", log)
        self.assertIn("refs/heads/other", log)

    def test_push_publishes_branches_and_opens_one_pull_request_each(self):
        self.bundle("impl/ok", "impl/two")
        bk.sync_refs(self.session)
        self.push()
        self.assertEqual(sorted(self.remote_refs()), ["refs/heads/impl/ok", "refs/heads/impl/two"])
        self.assertEqual(len(PushHandler.requests), 2)
        path, auth, body = PushHandler.requests[0]
        self.assertEqual(path, "/repos/ozgb/btm/pulls")
        self.assertEqual(auth, f"Bearer {FAKE_TOKEN}")
        self.assertEqual((body["base"], body["head"]), ("main", "impl/ok"))
        self.push()
        self.assertEqual(len(PushHandler.requests), 2)

    def test_other_422_is_retried(self):
        PushHandler.status = 422
        PushHandler.body = json.dumps({"message": "Validation Failed", "errors": [{"message": "No commits between main and impl/ok"}]}).encode()
        self.bundle("impl/ok")
        bk.sync_refs(self.session)
        self.push()
        self.assertEqual(json.loads((self.session / bk.PUSH_STATE).read_text())["pr"], {})
        self.assertIn("pull request for impl/ok failed", (self.session / "broker.log").read_text())
        PushHandler.status = 201
        PushHandler.body = b""
        self.push()
        self.assertEqual(len(PushHandler.requests), 2)
        self.assertIn("refs/heads/impl/ok", json.loads((self.session / bk.PUSH_STATE).read_text())["pr"])

    def test_existing_pull_request_is_not_an_error(self):
        PushHandler.status = 422
        PushHandler.body = json.dumps({"errors": [{"message": "A pull request already exists for ozgb:impl/ok."}]}).encode()
        self.bundle("impl/ok")
        bk.sync_refs(self.session)
        self.push()
        self.assertIn("exists", json.loads((self.session / bk.PUSH_STATE).read_text())["pr"]["refs/heads/impl/ok"])

    def test_token_is_in_no_log_config_or_environment(self):
        self.bundle("impl/ok")
        bk.sync_refs(self.session)
        before = dict(os.environ)
        self.push()
        self.assertEqual(dict(os.environ), before)
        self.assertNotIn(FAKE_TOKEN, "".join(os.environ.values()))
        leaks = [str(p) for p in self.session.rglob("*") if p.is_file() and FAKE_TOKEN.encode() in p.read_bytes()]
        self.assertEqual(leaks, [])
        self.assertTrue(self.remote_refs())

    def test_bad_token_files_push_nothing(self):
        self.bundle("impl/ok")
        bk.sync_refs(self.session)
        for name, token in (("absent", self.dir / "absent"), ("empty", make_secret(self.dir, "e", "")),
                            ("loose", make_secret(self.dir, "l", FAKE_TOKEN, 0o644))):
            self.push(token)
            self.assertEqual(self.remote_refs(), [], name)
        self.assertEqual(PushHandler.requests, [])
        self.assertIn("push skipped", (self.session / "broker.log").read_text())

    def test_failed_push_is_retried_without_a_pull_request(self):
        self.bundle("impl/ok")
        bk.sync_refs(self.session)
        bk.push_branches(self.session, self.token, str(self.dir / "no-such-remote.git"), "ozgb/btm", self.api)
        self.assertEqual(PushHandler.requests, [])
        self.assertIn("push refused", (self.session / "broker.log").read_text())
        self.push()
        self.assertEqual(len(PushHandler.requests), 1)

    def test_askpass_prints_the_variable_and_the_environment_stays_clean(self):
        out = subprocess.run([str(bk.ASKPASS), "Password for x: "], capture_output=True, text=True, env={bk.TOKEN_ENV: FAKE_TOKEN})
        self.assertEqual(out.stdout.strip(), FAKE_TOKEN)

    def test_a_bundle_that_fails_to_verify_is_logged_once(self):
        self.bundle("impl/ok")
        host = self.dir / "host"
        subprocess.run(["git", "init", "-q", "-b", "main", str(host)], check=True)
        env = {**os.environ, "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t", "GIT_COMMITTER_NAME": "t",
               "GIT_COMMITTER_EMAIL": "t@t", "GIT_CONFIG_GLOBAL": "/dev/null"}
        for n in ("a", "b"):
            (host / "f").write_text(n)
            subprocess.run(["git", "-C", str(host), "add", "f"], check=True, env=env)
            subprocess.run(["git", "-C", str(host), "commit", "-qm", n], check=True, env=env)
        subprocess.run(["git", "-C", str(host), "bundle", "create", "-q", str(self.session / "outbox" / bk.BUNDLE_NAME), "main~1..main"], check=True)
        bk.sync_refs(self.session)
        bk.sync_refs(self.session)
        self.assertEqual((self.session / "broker.log").read_text().count("does not verify"), 1)
        self.assertFalse((self.session / bk.STATE_NAME).exists())


if __name__ == "__main__":
    unittest.main()
