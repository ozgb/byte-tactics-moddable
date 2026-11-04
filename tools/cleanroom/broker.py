"""Host-side broker for a clean-room session: git sync and the gate mailbox.

    uv run tools/cleanroom/broker.py serve --session DIR [--interval SECONDS]
    uv run tools/cleanroom/broker.py once --session DIR

The session directory holds `outbox/` (the agent writes, the broker reads), `inbox/` (only the
broker writes), `sync/btm.git` (a bare repository the agent pushes to) and `bare/btm.git` (the
host's own bare repository, created on first use). Nothing is ever read from the agent's working
tree.

Git sync: refs under refs/heads/ of the sync repository are fetched into the host's bare
repository without force, with hooks, system config and global config switched off, so a hook or
config planted in the sync repository does not run. A ref that is not a fast-forward is refused
and logged.

Gate mailbox: the agent writes `outbox/ID/module.wasm` and `outbox/ID/subsystem`, then creates
`outbox/ID/ready`. The broker copies the module once, checks the id, the subsystem name, the size
and the wasm magic, runs the gate, and writes `inbox/ID.json` by rename. The gate here is a stub that
writes a fixed verdict: the real one is a later issue.

`serve` loops until SIGTERM or SIGINT and makes a last pass before it exits. `once` makes one pass.
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

REPO_NAME = "btm.git"
WASM_MAGIC = b"\0asm"
MAX_WASM = 64 * 1024 * 1024
ID_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]{0,63}$")
SUBSYSTEM_RE = re.compile(r"^[a-z][a-z0-9_-]{0,63}$")
STUB_VERDICT = "stub: the module was not judged"
GIT_ENV = {
    "GIT_CONFIG_NOSYSTEM": "1",
    "GIT_CONFIG_GLOBAL": "/dev/null",
    "GIT_TERMINAL_PROMPT": "0",
    "GIT_ATTR_NOSYSTEM": "1",
}


def stamp():
    return datetime.now(timezone.utc).isoformat()


def log(session, message):
    with open(Path(session) / "broker.log", "a", encoding="utf-8") as f:
        f.write(f"{stamp()} {message}\n")


def git(args, cwd=None):
    env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
    env.update(GIT_ENV)
    return subprocess.run(
        ["git", "-c", "core.hooksPath=/dev/null", "-c", "protocol.file.allow=always", *args],
        cwd=cwd,
        env=env,
        capture_output=True,
        text=True,
    )


def prepare(session):
    """Create the session's channel directories and both bare repositories."""
    session = Path(session)
    for name in ("outbox", "inbox"):
        (session / name).mkdir(parents=True, exist_ok=True)
    for name in ("sync", "bare"):
        repo = session / name / REPO_NAME
        if not repo.exists():
            repo.parent.mkdir(parents=True, exist_ok=True)
            result = git(["init", "--bare", "--initial-branch=main", str(repo)])
            if result.returncode:
                raise RuntimeError(result.stderr.strip())
    return session


def sync_refs(session):
    """Fetch the sync repository's branches into the host's bare repository; return the output."""
    session = Path(session)
    sync = session / "sync" / REPO_NAME
    bare = session / "bare" / REPO_NAME
    heads = git(["--git-dir", str(sync), "for-each-ref", "--format=%(refname)", "refs/heads/"])
    if heads.returncode or not heads.stdout.strip():
        return ""
    result = git(["--git-dir", str(bare), "fetch", "--no-tags", str(sync), "refs/heads/*:refs/heads/*"])
    out = (result.stdout + result.stderr).strip()
    if result.returncode:
        log(session, f"sync refused: {out}")
    elif out:
        log(session, f"sync: {out}")
    return out


def valid_request(directory):
    """Return (wasm bytes, subsystem) for a request directory, or raise ValueError."""
    if not ID_RE.match(directory.name):
        raise ValueError("bad id")
    wasm_path = directory / "module.wasm"
    sub_path = directory / "subsystem"
    for p in (wasm_path, sub_path):
        if p.is_symlink() or not p.is_file():
            raise ValueError(f"missing or not a regular file: {p.name}")
    if wasm_path.stat().st_size > MAX_WASM:
        raise ValueError("module too large")
    subsystem = sub_path.read_text(errors="replace").strip()
    if not SUBSYSTEM_RE.match(subsystem):
        raise ValueError("bad subsystem name")
    wasm = wasm_path.read_bytes()
    if not wasm.startswith(WASM_MAGIC):
        raise ValueError("not a wasm module")
    return wasm, subsystem


def stub_gate(wasm, subsystem, request_id):
    """Return the stub gate's fixed result for a module."""
    return {
        "gate": "stub",
        "id": request_id,
        "subsystem": subsystem,
        "sha256": hashlib.sha256(wasm).hexdigest(),
        "bytes": len(wasm),
        "verdict": STUB_VERDICT,
    }


def write_result(inbox, request_id, result):
    inbox = Path(inbox)
    tmp = inbox / f".{request_id}.tmp"
    tmp.write_text(json.dumps(result, sort_keys=True) + "\n")
    os.replace(tmp, inbox / f"{request_id}.json")


def process_mailbox(session, gate=stub_gate):
    """Answer every ready request that has no result yet; return the ids answered."""
    session = Path(session)
    answered = []
    outbox = session / "outbox"
    for directory in sorted(outbox.iterdir()):
        if directory.is_symlink() or not directory.is_dir():
            continue
        if not (directory / "ready").exists():
            continue
        request_id = directory.name
        if ID_RE.match(request_id) and (session / "inbox" / f"{request_id}.json").exists():
            continue
        try:
            wasm, subsystem = valid_request(directory)
            result = gate(wasm, subsystem, request_id)
        except (ValueError, OSError) as err:
            result = {"gate": "stub", "id": request_id, "error": str(err)}
            log(session, f"gate request {request_id!r} refused: {err}")
            if not ID_RE.match(request_id):
                continue
        else:
            log(session, f"gate request {request_id} subsystem {subsystem} answered")
        write_result(session / "inbox", request_id, result)
        answered.append(request_id)
    return answered


def one_pass(session):
    sync_refs(session)
    return process_mailbox(session)


def serve(session, interval):
    stop = []
    signal.signal(signal.SIGTERM, lambda *_: stop.append(1))
    signal.signal(signal.SIGINT, lambda *_: stop.append(1))
    log(session, "broker started")
    while not stop:
        one_pass(session)
        time.sleep(interval)
    one_pass(session)
    log(session, "broker stopped")
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("cmd", choices=("serve", "once"))
    ap.add_argument("--session", required=True)
    ap.add_argument("--interval", type=float, default=0.5)
    args = ap.parse_args(argv)
    prepare(args.session)
    if args.cmd == "once":
        print(json.dumps(one_pass(args.session)))
        return 0
    return serve(args.session, args.interval)


if __name__ == "__main__":
    sys.exit(main())
