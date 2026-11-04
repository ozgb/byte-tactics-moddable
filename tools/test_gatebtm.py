"""Tests for gatebtm.py, against a fake GitHub API on localhost and a stub oraclegate, with fake tokens in a temp dir.

    uv run tools/test_gatebtm.py
"""

import contextlib
import http.server
import io
import json
import os
import shutil
import sys
import tempfile
import threading
import unittest
import zipfile
from pathlib import Path

import gatebtm

SHA = "a" * 40
EQUAL = {"verdict": "equal", "scenarios": {"s1": {"status": "equal"}}, "recordings": {"wind_meteors/r1": {"status": "equal"}}}
DIFFERS = {"verdict": "differs", "scenarios": {"s1": {"status": "equal"}, "s2": {"status": "differs", "tick": 70, "phase": "meteors", "field": "SimGlobals.g_x", "value": "0x3c"}},
           "recordings": {"wind_meteors/r1": {"status": "failed"}}}
STUB = """import json, os, sys
out = sys.argv[sys.argv.index("--out") + 1]
os.makedirs(out, exist_ok=True)
with open(os.environ["STUB_RESULT"]) as f:
    data = f.read()
with open(out + "/result.json", "w") as f:
    f.write(data)
with open(os.environ["STUB_SEEN"], "a") as f:
    f.write(open(sys.argv[1], "rb").read().decode("latin1") + "|" + sys.argv[sys.argv.index("--subsystem") + 1] + "\\n")
"""


def make_zip(files: dict[str, bytes]) -> bytes:
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w") as z:
        for name, data in files.items():
            z.writestr(name, data)
    return buf.getvalue()


GOOD_ZIP = make_zip({"module.wasm": b"\0asm", "subsystem": b"wind_meteors\n"})


class Fake:
    """The state of the fake GitHub: what it serves and what it was asked."""

    def __init__(self):
        self.pulls = []
        self.permissions = {}
        self.runs = [{"id": 5, "status": "completed", "head_sha": SHA}]
        self.artifacts = [{"id": 9, "name": "module", "expired": False}]
        self.zip = GOOD_ZIP
        self.log = []
        self.posts = []
        self.tokens = set()


class Handler(http.server.BaseHTTPRequestHandler):
    fake: Fake

    def log_message(self, *args):
        pass

    def reply(self, code, body=b"", headers=None):
        self.send_response(code)
        for k, v in (headers or {}).items():
            self.send_header(k, v)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_json(self, data):
        self.reply(200, json.dumps(data).encode(), {"Content-Type": "application/json"})

    def do_GET(self):
        f = self.fake
        path = self.path.split("?")[0]
        f.log.append(path)
        f.tokens.add(self.headers.get("Authorization"))
        if path == "/blob":
            f.tokens.add(("blob", self.headers.get("Authorization")))
            return self.reply(200, f.zip)
        if path == "/repos/o/r/pulls":
            return self.send_json(f.pulls)
        if path.startswith("/repos/o/r/collaborators/") and path.endswith("/permission"):
            return self.send_json({"permission": f.permissions.get(path.split("/")[5], "none")})
        if path == "/repos/o/r/actions/runs":
            return self.send_json({"workflow_runs": f.runs})
        if path.startswith("/repos/o/r/actions/runs/") and path.endswith("/artifacts"):
            return self.send_json({"artifacts": f.artifacts})
        if path.startswith("/repos/o/r/actions/artifacts/") and path.endswith("/zip"):
            return self.reply(302, headers={"Location": f"http://127.0.0.1:{self.server.server_port}/blob"})
        self.reply(404)

    def do_POST(self):
        body = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        self.fake.log.append(self.path)
        self.fake.posts.append((self.path, body["body"], self.headers.get("Authorization")))
        self.reply(201, b"{}")


def pull(number, login, labels=(), sha=SHA, **extra):
    return {"number": number, "user": {"login": login, "id": 1}, "labels": [{"name": n, "id": 2} for n in labels], "head": {"sha": sha, "ref": "x"}, **extra}


class Base(unittest.TestCase):
    def setUp(self):
        self.dir = Path(tempfile.mkdtemp(prefix="test-gatebtm-"))
        self.addCleanup(shutil.rmtree, self.dir, True)
        self.fake = Fake()
        handler = type("H", (Handler,), {"fake": self.fake})
        self.server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
        threading.Thread(target=self.server.serve_forever, daemon=True).start()
        self.addCleanup(self.server.server_close)
        self.addCleanup(self.server.shutdown)
        self.api = gatebtm.Api(f"http://127.0.0.1:{self.server.server_port}", "o/r", "fake-token-123")
        self.stub = self.dir / "stub.py"
        self.stub.write_text(STUB)
        self.result = self.dir / "stub-result.json"
        self.seen = self.dir / "seen.txt"
        self.set_result(EQUAL)
        os.environ["STUB_RESULT"], os.environ["STUB_SEEN"] = str(self.result), str(self.seen)
        self.state = self.dir / "state" / "judged.json"

    def set_result(self, data):
        self.result.write_text(json.dumps(data))

    def run_pass(self, dry_run=False):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = gatebtm.run_pass(self.api, self.state, None, False, dry_run, self.stub)
        return code, out.getvalue()


class EligibilityTests(Base):
    def test_write_label_and_neither(self):
        self.fake.pulls = [pull(1, "writer"), pull(2, "stranger", labels=["gate"]), pull(3, "stranger"), pull(4, "reader")]
        self.fake.permissions = {"writer": "write", "reader": "read"}
        code, out = self.run_pass()
        self.assertEqual(code, 0)
        self.assertEqual(sorted(int(p.split("/")[5]) for p, _, _ in self.fake.posts), [1, 2])
        self.assertIn("#3 skipped: not eligible", out)
        self.assertIn("#4 skipped: not eligible", out)

    def test_ineligible_is_not_read_further(self):
        self.fake.pulls = [pull(3, "stranger")]
        self.run_pass()
        self.assertEqual([p for p in self.fake.log if "actions" in p or "comments" in p], [])
        self.assertEqual(self.fake.posts, [])

    def test_admin_and_maintain(self):
        self.fake.pulls = [pull(1, "a"), pull(2, "m")]
        self.fake.permissions = {"a": "admin", "m": "maintain"}
        self.run_pass()
        self.assertEqual(len(self.fake.posts), 2)


class FieldTests(Base):
    def test_only_four_fields_survive_parsing(self):
        page = [pull(7, "ann", labels=["gate", "x"], title="SECRET TITLE", body="SECRET BODY", comments=3, draft=True)]
        parsed = gatebtm.parse_pulls(page)
        self.assertEqual(parsed, [{"number": 7, "login": "ann", "labels": ["gate", "x"], "sha": SHA}])
        self.assertNotIn("SECRET", json.dumps(parsed))

    def test_malformed_records_are_dropped(self):
        self.assertEqual(gatebtm.parse_pulls([pull(1, "ok", sha="zz"), pull(2, "bad login!"), {"number": 3}]), [])

    def test_no_endpoint_past_the_four_is_requested(self):
        self.fake.pulls = [pull(1, "w", title="t", body="b")]
        self.fake.permissions = {"w": "write"}
        self.run_pass()
        for path in self.fake.log:
            self.assertFalse(any(x in path for x in ("/comments", "/reviews", "/files", "/issues/1")) and path not in [p for p, _, _ in self.fake.posts], path)

    def test_token_only_goes_to_the_api(self):
        self.fake.pulls = [pull(1, "w")]
        self.fake.permissions = {"w": "write"}
        self.run_pass()
        self.assertIn(("blob", None), self.fake.tokens)
        self.assertNotIn("Bearer fake-token-123", [t for t in self.fake.tokens if isinstance(t, tuple)])


class ArtifactTests(Base):
    def refused(self, files):
        self.fake.zip = make_zip(files) if isinstance(files, dict) else files
        self.fake.pulls = [pull(1, "w")]
        self.fake.permissions = {"w": "write"}
        code, out = self.run_pass()
        self.assertEqual(code, 1, out)
        self.assertEqual(self.fake.posts, [])
        self.assertFalse(self.seen.exists())
        return out

    def test_extra_file(self):
        self.assertIn("exactly", self.refused({"module.wasm": b"x", "subsystem": b"wind_meteors", "extra": b"1"}))

    def test_path_in_name(self):
        self.assertIn("exactly", self.refused({"../module.wasm": b"x", "subsystem": b"wind_meteors"}))
        self.assertIn("exactly", self.refused({"dir/module.wasm": b"x", "subsystem": b"wind_meteors"}))

    def test_unknown_subsystem(self):
        self.assertIn("unknown subsystem", self.refused({"module.wasm": b"x", "subsystem": b"../../etc"}))

    def test_oversized(self):
        self.assertIn("size limit", self.refused({"module.wasm": b"\0" * (gatebtm.MODULE_LIMIT + 1), "subsystem": b"wind_meteors"}))
        self.assertIn("size limit", self.refused({"module.wasm": b"x", "subsystem": b"w" * 100}))

    def test_not_a_zip(self):
        self.assertIn("not a zip", self.refused(b"nope"))

    def test_no_run_or_no_artifact_skips_without_failing(self):
        self.fake.pulls = [pull(1, "w")]
        self.fake.permissions = {"w": "write"}
        self.fake.runs = [{"id": 5, "status": "in_progress", "head_sha": SHA}]
        code, out = self.run_pass()
        self.assertEqual((code, self.fake.posts), (0, []))
        self.assertIn("#1 skipped: no module artifact", out)
        self.fake.runs = [{"id": 5, "status": "completed", "head_sha": SHA}]
        self.fake.artifacts = [{"id": 9, "name": "other"}]
        code, out = self.run_pass()
        self.assertEqual((code, self.fake.posts), (0, []))

    def test_run_for_another_commit_is_ignored(self):
        self.fake.pulls = [pull(1, "w")]
        self.fake.permissions = {"w": "write"}
        self.fake.runs = [{"id": 5, "status": "completed", "head_sha": "b" * 40}]
        self.run_pass()
        self.assertEqual(self.fake.posts, [])

    def test_the_judged_module_is_the_artifact(self):
        self.fake.pulls = [pull(1, "w")]
        self.fake.permissions = {"w": "write"}
        self.run_pass()
        self.assertEqual(self.seen.read_text(), "\0asm|wind_meteors\n")


class SummaryTests(unittest.TestCase):
    def test_equal(self):
        self.assertEqual(gatebtm.summary(EQUAL), "equal: every scenario and recording equal (1 scenarios, 1 recordings)")

    def test_differs(self):
        text = gatebtm.summary(DIFFERS)
        self.assertEqual(text.splitlines()[0], "differs: 2 of 3 checks not equal (2 scenarios, 1 recordings)")
        self.assertIn("first difference: s2, tick 70, phase meteors, field SimGlobals.g_x", text)
        self.assertIn("s2: differs", text)
        self.assertIn("wind_meteors/r1: failed", text)

    def test_no_value_and_no_free_text(self):
        self.assertNotIn("0x3c", gatebtm.summary(DIFFERS))
        hostile = {"verdict": "differs", "scenarios": {"bad name\nwith lines": {"status": "differs", "tick": 1, "phase": "p q", "field": None}}, "recordings": {}}
        text = gatebtm.summary(hostile)
        self.assertNotIn("bad name", text)
        self.assertEqual(len(text.splitlines()), 3)

    def test_refused(self):
        self.assertEqual(gatebtm.summary({"verdict": "refused", "message": "bad import\nmore"}), "refused: bad import")


class PostingTests(Base):
    def test_post_is_the_summary_alone_with_the_scoped_token(self):
        self.fake.pulls = [pull(1, "w")]
        self.fake.permissions = {"w": "write"}
        self.set_result(DIFFERS)
        self.run_pass()
        (path, body, auth), = self.fake.posts
        self.assertEqual(path, "/repos/o/r/issues/1/comments")
        self.assertEqual(body, gatebtm.summary(DIFFERS))
        self.assertEqual(auth, "Bearer fake-token-123")

    def test_dry_run_posts_and_records_nothing(self):
        self.fake.pulls = [pull(1, "w")]
        self.fake.permissions = {"w": "write"}
        code, out = self.run_pass(dry_run=True)
        self.assertEqual((code, self.fake.posts), (0, []))
        self.assertFalse(self.state.exists())
        self.assertIn("dry run", out)


class StateTests(Base):
    def test_judged_head_is_skipped_and_a_new_head_is_judged(self):
        self.fake.pulls = [pull(1, "w")]
        self.fake.permissions = {"w": "write"}
        self.run_pass()
        self.assertEqual(json.loads(self.state.read_text()), {"1": SHA})
        code, out = self.run_pass()
        self.assertEqual(len(self.fake.posts), 1)
        self.assertIn("#1 skipped: already judged", out)
        new = "c" * 40
        self.fake.pulls = [pull(1, "w", sha=new)]
        self.fake.runs = [{"id": 6, "status": "completed", "head_sha": new}]
        self.run_pass()
        self.assertEqual(len(self.fake.posts), 2)
        self.assertEqual(json.loads(self.state.read_text()), {"1": new})

    def test_unreadable_state_is_empty(self):
        self.state.parent.mkdir(parents=True)
        self.state.write_text("not json")
        self.assertEqual(gatebtm.load_state(self.state), {})


class LocalTests(Base):
    def test_local_prints_and_never_posts(self):
        module = self.dir / "m.wasm"
        module.write_bytes(b"\0asm")
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = gatebtm.run_local(module, "wind_meteors", None, False, self.stub)
        self.assertEqual(code, 0)
        self.assertIn("every scenario and recording equal", out.getvalue())
        self.assertEqual(self.fake.posts, [])
        self.assertEqual(self.fake.log, [])
        self.assertEqual(self.seen.read_text().split("|")[0], "\0asm")

    def test_local_not_equal_exits_1(self):
        module = self.dir / "m.wasm"
        module.write_bytes(b"\0asm")
        self.set_result(DIFFERS)
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(gatebtm.run_local(module, "wind_meteors", None, False, self.stub), 1)

    def test_local_unknown_subsystem(self):
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(gatebtm.run_local(self.dir / "m.wasm", "nope", None, False, self.stub), 2)

    def test_main_never_reads_the_token_for_local(self):
        module = self.dir / "m.wasm"
        module.write_bytes(b"\0asm")
        argv = sys.argv
        sys.argv = ["gatebtm", "--local", str(module), "--subsystem", "wind_meteors", "--token-file", str(self.dir / "absent")]
        gatebtm.ORACLEGATE, saved = self.stub, gatebtm.ORACLEGATE
        try:
            with contextlib.redirect_stdout(io.StringIO()) as out:
                code = gatebtm.main()
        finally:
            sys.argv, gatebtm.ORACLEGATE = argv, saved
        self.assertEqual(code, 0)


class TokenTests(Base):
    def test_missing_and_loose_token_files_exit_2(self):
        for make in (lambda p: None, lambda p: (p.write_text("tok"), p.chmod(0o644))):
            p = self.dir / "tok"
            p.unlink(missing_ok=True)
            make(p)
            argv, sys.argv = sys.argv, ["gatebtm", "--token-file", str(p), "--state", str(self.state)]
            try:
                with contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(gatebtm.main(), 2)
            finally:
                sys.argv = argv
        self.assertEqual(self.fake.log, [])

    def test_a_failed_request_does_not_print_the_token(self):
        self.fake.pulls = [pull(1, "w")]
        self.fake.permissions = {"w": "write"}
        self.fake.zip = b"x"
        self.fake.artifacts = [{"id": 9, "name": "module"}]
        _, out = self.run_pass()
        self.assertNotIn("fake-token-123", out)


if __name__ == "__main__":
    unittest.main()
