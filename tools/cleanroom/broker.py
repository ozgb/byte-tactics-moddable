"""Host-side broker for a clean-room session: git sync and the gate mailbox.

    uv run tools/cleanroom/broker.py serve --session DIR [--interval SECONDS] [--btm-token-file FILE] [--no-push] [--game-dir DIR]
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
and runs `tools/oraclegate.py` on a copy of the module in `SESSION/gate/ID/`, on the host. Only that run's `result.json`
crosses, as `inbox/ID.json` by rename; its `logs/` and `runtime/` stay in `SESSION/gate/`.

Push: after each sync the broker pushes every `refs/heads/impl/<name>` of the bare repository whose commit changed to
`--btm-remote` and opens a pull request into `main` of `--btm-repo` for each new branch. It reads the token from
`--btm-token-file` (default `~/.config/bt-cleanroom/btm-token`) at that moment, hands it to git through
`GIT_ASKPASS` (askpass.sh, which prints an environment variable set on that one process) and to the API in one
Authorization header. The token is never in the broker's environment, a git config file or a log line. Without a usable
token file (missing, empty, or readable by group or others) nothing is pushed.

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
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

REPO_NAME = "btm.git"
WASM_MAGIC = b"\0asm"
MAX_WASM = 64 * 1024 * 1024
MAX_BUNDLE = 1024 * 1024 * 1024
BUNDLE_NAME = "btm.bundle"
STATE_NAME = "bundle.state"
PUSH_STATE = "push.state"
ID_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]{0,63}$")
SUBSYSTEM_RE = re.compile(r"^[a-z][a-z0-9_-]{0,63}$")
STUB_VERDICT = "stub: the module was not judged"
ROOT = Path(__file__).resolve().parents[2]
ORACLEGATE = ROOT / "tools" / "oraclegate.py"
ASKPASS = Path(__file__).resolve().parent / "askpass.sh"
GATE_TIMEOUT = 4 * 3600
MAX_RESULT = 1024 * 1024
DEFAULT_TOKEN_FILE = Path.home() / ".config" / "bt-cleanroom" / "btm-token"
DEFAULT_REPO = "ozgb/btm"
DEFAULT_API = "https://api.github.com"
TOKEN_ENV = "BT_BTM_TOKEN"
BRANCH_PREFIX = "refs/heads/impl/"
SEGMENT_RE = re.compile(r"^[a-z0-9][a-z0-9._-]*$")
REPO_RE = re.compile(r"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$")
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


def git(args, cwd=None, extra_env=None):
    env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
    env.update(GIT_ENV)
    env.update(extra_env or {})
    return subprocess.run(
        ["git", "-c", "core.hooksPath=/dev/null", "-c", "transfer.fsckObjects=true", "-c", "gc.auto=0", "-c", "maintenance.auto=false", *args],
        cwd=cwd,
        env=env,
        capture_output=True,
        text=True,
    )


def branch_allowed(ref):
    """Return True for refs/heads/impl/<name>, <name> being /-separated segments of [a-z0-9][a-z0-9._-]*."""
    if not ref.startswith(BRANCH_PREFIX) or len(ref) > 200:
        return False
    segments = ref[len(BRANCH_PREFIX):].split("/")
    return all(
        SEGMENT_RE.match(seg) and ".." not in seg and not seg.endswith((".", ".lock"))
        for seg in segments
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


_unverified_logged = set()


def sync_refs(session):
    """Fetch the allowed branches of the outbox bundle into the host's bare repository; return the output."""
    session = Path(session)
    bare = session / "bare" / REPO_NAME
    state = session / STATE_NAME
    scratch = session / ".bundle.tmp"
    fd = None
    try:
        fd = open_outbox(session)
        try:
            digest = copy_bundle(fd, scratch)
        except (ValueError, OSError) as err:
            log(session, f"sync refused: {err}")
            return ""
        if digest is None or (state.exists() and state.read_text().strip() == digest):
            return ""
        verify = git(["--git-dir", str(bare), "bundle", "verify", str(scratch)])
        if verify.returncode:
            out = (verify.stdout + verify.stderr).strip()
            if (str(session), digest) not in _unverified_logged:
                _unverified_logged.add((str(session), digest))
                log(session, f"sync refused: bundle {digest[:12]} does not verify: {out}")
            return out
        state.write_text(digest + "\n")
        heads = git(["--git-dir", str(bare), "bundle", "list-heads", str(scratch)])
        refs = [line.split(" ", 1)[1] for line in heads.stdout.splitlines() if " " in line]
        allowed = [r for r in refs if branch_allowed(r)]
        for ref in refs:
            if ref not in allowed:
                log(session, f"sync refused: ref {ref!r} is not refs/heads/impl/<name>")
        if not allowed:
            return ""
        result = git(["--git-dir", str(bare), "fetch", "--no-tags", str(scratch), *[f"{r}:{r}" for r in allowed]])
        out = (result.stdout + result.stderr).strip()
        log(session, f"sync refused: {out}" if result.returncode else f"sync: {out or 'bundle ' + digest[:12]}")
        return out
    finally:
        if fd is not None:
            os.close(fd)
        scratch.unlink(missing_ok=True)


def read_token(token_file):
    """Return the token in a file only its owner may access, or None (with the reason logged by the caller)."""
    path = Path(token_file)
    try:
        info = path.stat()
        if info.st_mode & 0o077:
            return None, f"{path} is accessible to group or others"
        token = path.read_text().strip()
    except OSError as err:
        return None, f"{path} is unreadable: {err.strerror}"
    return (token, "") if token else (None, f"{path} is empty")


def bare_branches(bare):
    """Return {ref: sha} for the refs of the bare repository that the branch rule allows."""
    out = git(["--git-dir", str(bare), "for-each-ref", "--format=%(objectname) %(refname)", "refs/"]).stdout
    refs = {}
    for line in out.splitlines():
        sha, _, ref = line.partition(" ")
        if branch_allowed(ref):
            refs[ref] = sha
    return refs


def open_pull_request(api_base, repo, token, branch, base="main"):
    """Open a pull request of `branch` into `base`; return its URL, or "exists" when GitHub says one is open."""
    body = {"title": f"Implementer: {branch}", "head": branch, "base": base, "body": "Opened by the clean-room broker."}
    request = urllib.request.Request(
        f"{api_base.rstrip('/')}/repos/{repo}/pulls",
        data=json.dumps(body).encode(),
        method="POST",
        headers={
            "Authorization": f"Bearer {token}",
            "Accept": "application/vnd.github+json",
            "Content-Type": "application/json",
            "User-Agent": "bt-cleanroom-broker",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            return json.loads(response.read()).get("html_url", "")
    except urllib.error.HTTPError as err:
        err.close()
        if err.code == 422:
            return "exists"
        raise


def push_branches(session, token_file, remote, repo=DEFAULT_REPO, api_base=DEFAULT_API):
    """Push the allowed branches of the bare repository to the remote and open a pull request for each new one.

    The token is read now, handed to git through GIT_ASKPASS for that push alone and to the API in one header.
    """
    session = Path(session)
    bare = session / "bare" / REPO_NAME
    state_file = session / PUSH_STATE
    state = json.loads(state_file.read_text()) if state_file.exists() else {"pushed": {}, "pr": {}}
    pending = {r: sha for r, sha in bare_branches(bare).items() if state["pushed"].get(r) != sha}
    need_pr = [r for r in bare_branches(bare) if r not in state["pr"] and r in state["pushed"] and r not in pending]
    if not pending and not need_pr:
        return
    if not REPO_RE.match(repo):
        log(session, f"push skipped: bad repository name {repo!r}")
        return
    token, why = read_token(token_file)
    if token is None:
        if (str(session), why) not in _unverified_logged:
            _unverified_logged.add((str(session), why))
            log(session, f"push skipped: {why}")
        return
    for ref, sha in sorted(pending.items()):
        result = git(
            ["--git-dir", str(bare), "-c", "credential.helper=", "-c", "core.askPass=", "push", remote, f"{sha}:{ref}"],
            extra_env={"GIT_ASKPASS": str(ASKPASS), TOKEN_ENV: token},
        )
        if result.returncode:
            log(session, f"push refused for {ref}: {(result.stdout + result.stderr).replace(token, '***').strip()}")
            continue
        state["pushed"][ref] = sha
        need_pr.append(ref)
        log(session, f"push: {ref} {sha[:12]}")
        state_file.write_text(json.dumps(state, sort_keys=True) + "\n")
    for ref in sorted(set(need_pr)):
        if ref in state["pr"]:
            continue
        branch = ref[len("refs/heads/"):]
        try:
            url = open_pull_request(api_base, repo, token, branch)
        except (urllib.error.URLError, OSError, ValueError) as err:
            log(session, f"pull request for {branch} failed: {str(err).replace(token, '***')}")
            continue
        state["pr"][ref] = url
        log(session, f"pull request: {branch} {url}")
        state_file.write_text(json.dumps(state, sort_keys=True) + "\n")


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


def make_gate(session, command=None, timeout=GATE_TIMEOUT, game_dir=None):
    """Return a gate that runs oraclegate on a module and lets only its result.json cross.

    `command` is the argv prefix of the judge (default: this interpreter and tools/oraclegate.py). It runs as
    `command MODULE --subsystem NAME --out DIR` with DIR on the host, outside the mailbox: its logs/ and
    runtime/ never leave DIR. With `game_dir` it also gets `--game-dir GAME_DIR`.
    """
    prefix = list(command) if command else [sys.executable, str(ORACLEGATE)]
    extra = ["--game-dir", str(game_dir)] if game_dir else []

    def gate(wasm, subsystem, request_id):
        work = Path(session) / "gate" / request_id
        shutil.rmtree(work, ignore_errors=True)
        work.mkdir(parents=True)
        module = work / "module.wasm"
        module.write_bytes(wasm)
        out = work / "out"
        out.mkdir()
        try:
            run = subprocess.run(
                [*prefix, str(module), "--subsystem", subsystem, "--out", str(out), *extra],
                cwd=ROOT, capture_output=True, text=True, timeout=timeout,
            )
        except subprocess.TimeoutExpired:
            return {"gate": "oraclegate", "id": request_id, "error": f"the gate timed out after {timeout}s"}
        (work / "gate.log").write_text(run.stdout + run.stderr)
        return filter_result(out / "result.json", request_id, run.returncode)

    return gate


def filter_result(path, request_id, code):
    """Return the parsed result.json at path (and nothing else of the gate's output), or an error result."""
    try:
        info = os.lstat(path)
        if not stat.S_ISREG(info.st_mode) or info.st_size > MAX_RESULT:
            raise ValueError("not a regular file or too large")
        result = json.loads(Path(path).read_text())
        if not isinstance(result, dict):
            raise ValueError("not an object")
    except (OSError, ValueError) as err:
        return {"gate": "oraclegate", "id": request_id, "error": f"the gate wrote no usable result (exit {code}): {err}"}
    return {**result, "gate": "oraclegate", "id": request_id}


def write_result(inbox, request_id, result):
    inbox = Path(inbox)
    tmp = inbox / f".{request_id}.tmp"
    tmp.write_text(json.dumps(result, sort_keys=True) + "\n")
    os.replace(tmp, inbox / f"{request_id}.json")


def process_mailbox(session, gate=None):
    """Answer every ready request that has no result yet; return the ids answered."""
    session = Path(session)
    answered = []
    gate = gate or make_gate(session)
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
                result = {"gate": "oraclegate", "id": request_id, "error": str(err)}
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


def one_pass(session, push=None, gate=None):
    """Sync the bundle, push when `push` (a dict of push_branches options) is given, answer the mailbox."""
    sync_refs(session)
    if push:
        push_branches(session, **push)
    return process_mailbox(session, gate)


def serve(session, interval, push=None, gate=None):
    stop = []
    signal.signal(signal.SIGTERM, lambda *_: stop.append(1))
    signal.signal(signal.SIGINT, lambda *_: stop.append(1))
    log(session, "broker started")
    while not stop:
        one_pass(session, push, gate)
        time.sleep(interval)
    one_pass(session, push, gate)
    log(session, "broker stopped")
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("cmd", choices=("serve", "once"))
    ap.add_argument("--session", required=True)
    ap.add_argument("--interval", type=float, default=0.5)
    ap.add_argument("--btm-token-file", default=str(DEFAULT_TOKEN_FILE), help="file holding the btm token, read when pushing")
    ap.add_argument("--btm-remote", default=f"https://github.com/{DEFAULT_REPO}.git", help="push URL of the clean-room repository")
    ap.add_argument("--btm-repo", default=DEFAULT_REPO, help="OWNER/NAME for pull requests")
    ap.add_argument("--api-base", default=DEFAULT_API)
    ap.add_argument("--no-push", action="store_true", help="do not push branches or open pull requests")
    ap.add_argument("--gate-timeout", type=int, default=GATE_TIMEOUT)
    ap.add_argument("--game-dir", default=None, help="game directory for the real gate (default: oraclegate looks in Steam/GOG places)")
    args = ap.parse_args(argv)
    prepare(args.session)
    push = None if args.no_push else {
        "token_file": args.btm_token_file, "remote": args.btm_remote, "repo": args.btm_repo, "api_base": args.api_base,
    }
    gate = make_gate(args.session, timeout=args.gate_timeout, game_dir=args.game_dir)
    if args.cmd == "once":
        print(json.dumps(one_pass(args.session, push, gate)))
        return 0
    return serve(args.session, args.interval, push, gate)


if __name__ == "__main__":
    sys.exit(main())
