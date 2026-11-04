"""Host-side broker for a clean-room session: git sync and the gate mailbox.

    uv run tools/cleanroom/broker.py serve --session DIR [--interval SECONDS]
    uv run tools/cleanroom/broker.py once --session DIR

The session directory holds `outbox/` (the agent writes, the broker reads), `inbox/` (only the
broker writes) and `bare/btm.git` (the host's own bare repository, created on first use). Nothing
is ever read from the agent's working tree, and no repository the agent controls is ever opened.

Git sync: the agent writes `git bundle create` output to `outbox/btm.bundle` (writing a temporary
name and renaming it). The broker opens it with O_NOFOLLOW through the outbox directory descriptor,
requires a regular file, copies the bytes into a file only the broker can write, runs
`git bundle verify` on the copy against the bare repository, and fetches `refs/heads/*` from the
copy without force, with hooks, system config and global config switched off. A bundle whose
prerequisites the bare repository lacks fails to verify, and a ref that is not a fast-forward is
refused and logged. Each distinct bundle is processed once.

Gate mailbox: the agent writes `outbox/ID/module.wasm` and `outbox/ID/subsystem`, then creates
`outbox/ID/ready`. The broker opens every one of them with O_NOFOLLOW relative to a directory
descriptor and checks the opened descriptors, so a swap for a symlink after the check cannot
redirect a read. It checks the id, the subsystem name, the size and the wasm magic, runs the gate,
and writes `inbox/ID.json` by rename. The gate here is a stub that writes a fixed verdict: the
real one is a later issue.

`serve` loops until SIGTERM or SIGINT and makes a last pass before it exits. `once` makes one pass.
"""

import argparse
import errno
import hashlib
import json
import os
import re
import shutil
import signal
import stat
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

REPO_NAME = "btm.git"
WASM_MAGIC = b"\0asm"
MAX_WASM = 64 * 1024 * 1024
MAX_BUNDLE = 1024 * 1024 * 1024
BUNDLE_NAME = "btm.bundle"
STATE_NAME = "bundle.state"
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
        ["git", "-c", "core.hooksPath=/dev/null", "-c", "transfer.fsckObjects=true", *args],
        cwd=cwd,
        env=env,
        capture_output=True,
        text=True,
    )


def prepare(session):
    """Create the session's channel directories and the host's bare repository."""
    session = Path(session)
    for name in ("outbox", "inbox"):
        (session / name).mkdir(parents=True, exist_ok=True)
    repo = session / "bare" / REPO_NAME
    if not repo.exists():
        repo.parent.mkdir(parents=True, exist_ok=True)
        result = git(["init", "--bare", "--initial-branch=main", str(repo)])
        if result.returncode:
            raise RuntimeError(result.stderr.strip())
    return session


def open_outbox(session):
    """Return a directory descriptor for the outbox that refuses a symlink."""
    return os.open(Path(session) / "outbox", os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)


def open_regular(name, dir_fd):
    """Open a regular file below a directory descriptor without following a symlink; return (fd, size)."""
    try:
        fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK | os.O_CLOEXEC, dir_fd=dir_fd)
    except OSError as err:
        if err.errno in (errno.ELOOP, errno.ENXIO):
            raise ValueError(f"not a regular file: {name}") from err
        raise
    info = os.fstat(fd)
    if not stat.S_ISREG(info.st_mode):
        os.close(fd)
        raise ValueError(f"not a regular file: {name}")
    return fd, info.st_size


def read_limited(fd, limit):
    """Read a descriptor to its end and close it; raise ValueError when it holds more than limit bytes."""
    with os.fdopen(fd, "rb") as f:
        data = f.read(limit + 1)
    if len(data) > limit:
        raise ValueError("too large")
    return data


def copy_bundle(outbox_fd, dest):
    """Copy the outbox bundle into dest and return its sha256, or None when there is no bundle."""
    try:
        fd, size = open_regular(BUNDLE_NAME, outbox_fd)
    except FileNotFoundError:
        return None
    if size > MAX_BUNDLE:
        os.close(fd)
        raise ValueError("bundle too large")
    digest = hashlib.sha256()
    total = 0
    with os.fdopen(fd, "rb") as src, open(dest, "wb") as out:
        while chunk := src.read(1 << 20):
            total += len(chunk)
            if total > MAX_BUNDLE:
                raise ValueError("bundle too large")
            digest.update(chunk)
            out.write(chunk)
    return digest.hexdigest()


def sync_refs(session):
    """Fetch the branches of the outbox bundle into the host's bare repository; return the output."""
    session = Path(session)
    bare = session / "bare" / REPO_NAME
    state = session / STATE_NAME
    fd = open_outbox(session)
    scratch = session / ".bundle.tmp"
    try:
        try:
            digest = copy_bundle(fd, scratch)
        except (ValueError, OSError) as err:
            log(session, f"sync refused: {err}")
            return ""
        if digest is None or (state.exists() and state.read_text().strip() == digest):
            return ""
        state.write_text(digest + "\n")
        verify = git(["--git-dir", str(bare), "bundle", "verify", str(scratch)])
        if verify.returncode:
            out = (verify.stdout + verify.stderr).strip()
            log(session, f"sync refused: bundle {digest[:12]} does not verify: {out}")
            return out
        result = git(["--git-dir", str(bare), "fetch", "--no-tags", str(scratch), "refs/heads/*:refs/heads/*"])
        out = (result.stdout + result.stderr).strip()
        log(session, f"sync refused: {out}" if result.returncode else f"sync: {out or 'bundle ' + digest[:12]}")
        return out
    finally:
        os.close(fd)
        scratch.unlink(missing_ok=True)


def read_request(outbox_fd, name):
    """Return (wasm bytes, subsystem) for a ready request directory, None if not ready; raise ValueError."""
    try:
        dir_fd = os.open(name, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=outbox_fd)
    except OSError:
        return None
    wasm_fd = sub_fd = None
    try:
        try:
            os.close(open_regular("ready", dir_fd)[0])
        except (FileNotFoundError, ValueError):
            return None
        if not ID_RE.match(name):
            raise ValueError("bad id")
        try:
            wasm_fd, size = open_regular("module.wasm", dir_fd)
            sub_fd, _ = open_regular("subsystem", dir_fd)
        except (FileNotFoundError, ValueError) as err:
            raise ValueError(f"missing or not a regular file: {err}") from err
        if size > MAX_WASM:
            raise ValueError("module too large")
        taken, wasm_fd = wasm_fd, None
        wasm = read_limited(taken, MAX_WASM)
        taken, sub_fd = sub_fd, None
        subsystem = read_limited(taken, 4096).decode(errors="replace").strip()
    finally:
        for fd in (wasm_fd, sub_fd, dir_fd):
            if fd is not None:
                os.close(fd)
    if not SUBSYSTEM_RE.match(subsystem):
        raise ValueError("bad subsystem name")
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
    outbox_fd = open_outbox(session)
    try:
        for request_id in sorted(os.listdir(outbox_fd)):
            if ID_RE.match(request_id) and (session / "inbox" / f"{request_id}.json").exists():
                continue
            try:
                request = read_request(outbox_fd, request_id)
                if request is None:
                    continue
                wasm, subsystem = request
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
    finally:
        os.close(outbox_fd)
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
