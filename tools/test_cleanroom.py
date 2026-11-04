"""Tests for the clean-room proxy and canary manifest, on localhost and temp trees only.

    python3 tools/test_cleanroom.py
    uv run tools/test_cleanroom.py
"""

import asyncio
import contextlib
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
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
            ln.check_disjoint(a, a / "other")
        ln.check_disjoint(a / "home", a / "other")

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
        self.assertEqual(argv.count("-v"), 5)
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
        self.assertEqual(bk.process_mailbox(self.session), ["r1"])
        result = json.loads((self.session / "inbox" / "r1.json").read_text())
        self.assertEqual(result["verdict"], bk.STUB_VERDICT)
        self.assertEqual(result["subsystem"], "parser")
        self.assertEqual(bk.process_mailbox(self.session), [])
        self.assertFalse((self.session / "inbox" / "r2.json").exists())

    def test_bad_requests_get_an_error_result(self):
        self.request("bad1", wasm=b"not wasm")
        self.request("bad2", subsystem="../etc")
        self.request("bad3", subsystem="Parser")
        bk.process_mailbox(self.session)
        for name in ("bad1", "bad2", "bad3"):
            self.assertIn("error", json.loads((self.session / "inbox" / f"{name}.json").read_text()))

    def test_symlinked_module_is_refused(self):
        self.request("link")
        module = self.session / "outbox" / "link" / "module.wasm"
        module.unlink()
        module.symlink_to("/etc/passwd")
        bk.process_mailbox(self.session)
        self.assertIn("error", json.loads((self.session / "inbox" / "link.json").read_text()))

    def commit(self, repo_dir, text):
        env = {"GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t", "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@t"}
        env = {**os.environ, **env, "GIT_CONFIG_GLOBAL": "/dev/null", "GIT_CONFIG_NOSYSTEM": "1"}
        (repo_dir / "f").write_text(text)
        subprocess.run(["git", "-C", str(repo_dir), "add", "f"], check=True, env=env)
        subprocess.run(["git", "-C", str(repo_dir), "commit", "-qm", text], check=True, env=env)

    @unittest.skipUnless(shutil.which("git"), "needs git")
    def test_sync_fetches_fast_forwards_and_refuses_the_rest(self):
        work = Path(self.tmp.name) / "work"
        subprocess.run(["git", "init", "-q", "-b", "main", str(work)], check=True)
        sync = self.session / "sync" / bk.REPO_NAME
        bare = self.session / "bare" / bk.REPO_NAME
        self.assertEqual(bk.sync_refs(self.session), "")
        self.commit(work, "one")
        subprocess.run(["git", "-C", str(work), "push", "-q", str(sync), "main"], check=True)
        bk.sync_refs(self.session)
        first = subprocess.run(["git", "--git-dir", str(bare), "rev-parse", "main"], capture_output=True, text=True)
        self.assertEqual(first.returncode, 0)
        self.commit(work, "two")
        subprocess.run(["git", "-C", str(work), "push", "-q", str(sync), "main"], check=True)
        bk.sync_refs(self.session)
        second = subprocess.run(["git", "--git-dir", str(bare), "rev-parse", "main"], capture_output=True, text=True)
        self.assertNotEqual(first.stdout, second.stdout)
        subprocess.run(["git", "-C", str(work), "checkout", "-q", "--orphan", "other"], check=True)
        self.commit(work, "three")
        subprocess.run(["git", "-C", str(work), "push", "-q", "-f", str(sync), "other:main"], check=True)
        bk.sync_refs(self.session)
        third = subprocess.run(["git", "--git-dir", str(bare), "rev-parse", "main"], capture_output=True, text=True)
        self.assertEqual(second.stdout, third.stdout)

    @unittest.skipUnless(shutil.which("git"), "needs git")
    def test_hook_planted_in_the_sync_repository_does_not_run(self):
        work = Path(self.tmp.name) / "work"
        subprocess.run(["git", "init", "-q", "-b", "main", str(work)], check=True)
        sync = self.session / "sync" / bk.REPO_NAME
        self.commit(work, "one")
        subprocess.run(["git", "-C", str(work), "push", "-q", str(sync), "main"], check=True)
        marker = Path(self.tmp.name) / "ran"
        for hook in ("pre-upload-pack", "post-update", "reference-transaction"):
            path = sync / "hooks" / hook
            path.parent.mkdir(exist_ok=True)
            path.write_text(f"#!/bin/sh\ntouch {marker}\n")
            path.chmod(0o755)
        (sync / "config").write_text((sync / "config").read_text() + f"[uploadpack]\n\tpackObjectsHook = {marker}\n")
        bk.sync_refs(self.session)
        self.assertFalse(marker.exists())


if __name__ == "__main__":
    unittest.main()
