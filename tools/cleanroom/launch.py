"""Start a clean-room session VM: exact mounts, no network device, canary checks, then the agent.

    uv run tools/cleanroom/launch.py --tree DIR --datakit DIR --image FILE.smolmachine \\
        [--session DIR] [--credential-file FILE] [--credential-kind oauth|api-key] [--game-dir DIR] [--keep] -- COMMAND [ARG ...]

The VM has no network device. Its only way out is a Unix socket to proxy.py on the host. It sees the
clean-room tree (read-write at /work), the data kit (read-only at /data), the mailbox directories
/srv/outbox (read-write) and /srv/inbox (read-only) and the proxy socket /srv/proxy.sock. broker.py
runs beside it: it reads a git bundle and the gate requests from the outbox as data.

Refusals, each exit status 2 before any VM exists: a mount that is, contains or sits inside this
repository (except under build/scratch), another worktree of it, ~/.claude*, $CLAUDE_CONFIG_DIR, ~/.ta, ~/.ssh,
~/.config/gh, ~/.config/bt-cleanroom, ~/.gnupg or ~/source/repos; a tree without its own .git, or one whose git config names
byte-tactics; overlapping mounts, the session directory included; a session directory that is, contains or sits
inside this repository or ~/.claude* and the like; a file of the canary manifest in a mount or the
session directory; an allowlist that
names GitHub or similar hosts or an IP literal; a credential file that is missing, empty or readable by anyone but its owner,
or whose value equals a credential the host's Claude Code uses (ANTHROPIC_API_KEY or CLAUDE_CODE_OAUTH_TOKEN in the
launcher's environment, or the token in ~/.claude/.credentials.json or $CLAUDE_CONFIG_DIR/.credentials.json); an API key variable not named ANTHROPIC_*. The
credential reaches the agent command as CLAUDE_CODE_OAUTH_TOKEN (oauth) or ANTHROPIC_API_KEY (api-key), by --secret-env. The
canary checks then run inside the VM, and a failing check stops the session before the agent starts.
The canary directory is deleted from the VM before the agent starts, and the launch fails if it is
still there.
"""

import argparse
import contextlib
import io
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import broker
import manifest as mf
import proxy as px

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
SMOLVM = Path.home() / ".local" / "bin" / "smolvm"
DENIED_HOST_SUFFIXES = (
    "github.com",
    "githubusercontent.com",
    "githubassets.com",
    "github.io",
    "ghcr.io",
    "gitlab.com",
    "bitbucket.org",
)
HOME_FORBIDDEN = (".ta", ".ssh", ".config/gh", ".config/bt-cleanroom", ".gnupg", "source/repos")
DEFAULT_CREDENTIAL = Path.home() / ".config" / "bt-cleanroom" / "anthropic-impl-key"
CREDENTIAL_VARS = {"oauth": "CLAUDE_CODE_OAUTH_TOKEN", "api-key": "ANTHROPIC_API_KEY"}
HOST_CLAUDE_VARS = ("ANTHROPIC_API_KEY", "CLAUDE_CODE_OAUTH_TOKEN")
SECRET_HOST_VAR = "BT_CLEANROOM_CREDENTIAL"
GUEST_SCRIPTS = ("canary.py", "manifest.py")
GUEST_HOME = "/home/agent"
CANARY_DIR = "/opt/canary"
FORBIDDEN_FLAGS = ("--net", "--ssh-agent", "--allow-system-mounts", "--allow-host-loopback", "--docker-config", "--nested")
EXIT_REFUSED = 2


class Refusal(Exception):
    pass


def inside(path, parent):
    return path == parent or parent in path.parents


def repo_roots(repo=REPO):
    """Return the roots of every worktree of the repository."""
    roots = {repo}
    result = subprocess.run(["git", "-C", str(repo), "worktree", "list", "--porcelain"], capture_output=True, text=True)
    for line in result.stdout.splitlines():
        if line.startswith("worktree "):
            roots.add(Path(line[9:]).resolve())
    return sorted(roots)


def claude_config_dir(environ):
    """Return the resolved CLAUDE_CONFIG_DIR of the environment, or None when it is unset or empty."""
    value = environ.get("CLAUDE_CONFIG_DIR")
    return Path(value).expanduser().resolve() if value else None


def forbidden_paths(home, config_dir=None):
    """Return the paths that a mount may not overlap: under the home directory, and the Claude config directory when given."""
    paths = [home / p for p in HOME_FORBIDDEN]
    paths.extend(sorted(home.glob(".claude*")))
    if config_dir is not None:
        paths.append(config_dir)
    return paths


def check_mount(path, role, roots, home, must_exist=True, config_dir=None):
    """Return the resolved mount path, or raise Refusal. A path that does not exist yet passes when must_exist is False."""
    path = Path(path)
    if path.exists() or must_exist:
        if not path.is_dir():
            raise Refusal(f"{role} {path} is not a directory")
    path = path.resolve()
    for root in roots:
        if inside(root, path):
            raise Refusal(f"{role} {path} contains {root}")
    holders = [r for r in roots if inside(path, r)]
    if holders:
        innermost = max(holders, key=lambda r: len(r.parts))
        scratch = innermost / "build" / "scratch"
        if path == scratch or not inside(path, scratch):
            raise Refusal(f"{role} {path} is inside {innermost}, outside build/scratch")
    for bad in forbidden_paths(home, config_dir):
        if inside(path, bad) or inside(bad, path):
            raise Refusal(f"{role} {path} overlaps {bad}")
    return path


def check_tree(path):
    git = path / ".git"
    if not git.exists():
        raise Refusal(f"tree {path} has no .git of its own")
    config = git / "config" if git.is_dir() else None
    if config is None:
        raise Refusal(f"tree {path} has a .git file, not a directory: a worktree points outside the mount")
    text = config.read_text(errors="replace").lower() if config.exists() else ""
    for needle in ("byte-tactics", "hectorbailey", "ozgb/byte"):
        if needle in text:
            raise Refusal(f"tree {path} git config names {needle}")


def check_disjoint(*named):
    """Raise Refusal when any two of the (role, path) pairs overlap."""
    for i, (role_a, a) in enumerate(named):
        for role_b, b in named[i + 1:]:
            if inside(a, b) or inside(b, a):
                raise Refusal(f"{role_a} {a} and {role_b} {b} overlap")


def check_allowlist_text(text):
    allowed = px.parse_allowlist(text)
    if not allowed:
        raise Refusal("the allowlist is empty")
    for host, port in sorted(allowed):
        if px.is_ip_literal(host):
            raise Refusal(f"the allowlist names an IP literal: {host}")
        for suffix in DENIED_HOST_SUFFIXES:
            if host == suffix or host.endswith("." + suffix):
                raise Refusal(f"the allowlist names a forbidden host: {host}")
    return allowed


def check_api_key_env(name):
    if name is not None and not name.startswith("ANTHROPIC_"):
        raise Refusal(f"--api-key-env {name} is not an ANTHROPIC_* variable")


def default_host_credentials():
    return Path.home() / ".claude" / ".credentials.json"


def host_claude_tokens(environ, credentials_file=None):
    """Return the credentials the host's Claude Code uses: the two variables and the stored OAuth access token of ~/.claude and of $CLAUDE_CONFIG_DIR."""
    tokens = {environ[v] for v in HOST_CLAUDE_VARS if environ.get(v)}
    files = [Path(credentials_file) if credentials_file else default_host_credentials()]
    config_dir = claude_config_dir(environ)
    if config_dir is not None:
        files.append(config_dir / ".credentials.json")
    for file in files:
        try:
            stored = json.loads(file.read_text())["claudeAiOauth"]["accessToken"]
            if isinstance(stored, str) and stored:
                tokens.add(stored)
        except (OSError, ValueError, KeyError, TypeError):
            pass
    return tokens


def read_credential(path, kind, environ, credentials_file):
    """Return (guest variable name, value) for the credential file, or raise Refusal."""
    if kind not in CREDENTIAL_VARS:
        raise Refusal(f"--credential-kind {kind} is not oauth or api-key")
    path = Path(path)
    try:
        info = path.stat()
    except OSError as err:
        raise Refusal(f"credential file {path} is missing: {err.strerror}") from err
    if not path.is_file():
        raise Refusal(f"credential file {path} is not a regular file")
    if info.st_mode & 0o077:
        raise Refusal(f"credential file {path} is readable by group or others (mode {info.st_mode & 0o777:o}): chmod 600 it")
    if info.st_uid != os.getuid():
        raise Refusal(f"credential file {path} is not owned by you")
    value = path.read_text().strip()
    if not value:
        raise Refusal(f"credential file {path} is empty")
    if value in host_claude_tokens(environ, credentials_file):
        raise Refusal(f"credential file {path} holds a credential the host's Claude Code uses: make a separate one")
    return CREDENTIAL_VARS[kind], value


def check_scan_clean(manifest, *roots):
    for root in roots:
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = mf.scan(manifest, [str(root)], mf.DEFAULT_MAX)
        if code:
            raise Refusal(f"{root} holds a file of the canary manifest:\n{out.getvalue().strip()}")


def create_argv(name, image, tree, datakit, session, sock, cpus=4, mem=4096):
    """Return the `smolvm machine create` argv for a session, with exactly the allowed mounts."""
    return [
        str(SMOLVM), "machine", "create", "--name", name, "--from", str(image),
        "--cpus", str(cpus), "--mem", str(mem),
        "-v", f"{tree}:/work",
        "-v", f"{datakit}:/data:ro",
        "-v", f"{session}/outbox:/srv/outbox",
        "-v", f"{session}/inbox:/srv/inbox:ro",
        "--mount-socket", f"{sock}:/srv/proxy.sock",
    ]


def check_argv(argv):
    """Raise Refusal unless a create argv has no network or credential flag and only the expected mounts."""
    for flag in FORBIDDEN_FLAGS:
        if flag in argv:
            raise Refusal(f"forbidden flag {flag}")
    mounts = [argv[i + 1] for i, a in enumerate(argv) if a in ("-v", "--volume")]
    guests = sorted(m.split(":")[1] for m in mounts)
    if guests != ["/data", "/srv/inbox", "/srv/outbox", "/work"]:
        raise Refusal(f"unexpected mounts: {guests}")
    if argv.count("--mount-socket") != 1:
        raise Refusal("expected exactly one socket mount")


def run(cmd, **kw):
    return subprocess.run(cmd, text=True, **kw)


def exec_in(name, command, user=None, env=(), secret_env=(), detach=False):
    argv = [str(SMOLVM), "machine", "exec", "--name", name]
    if user:
        argv += ["--user", user]
    for e in env:
        argv += ["-e", e]
    for s in secret_env:
        argv += ["--secret-env", s]
    if detach:
        argv += ["--detach"]
    return argv + ["--"] + list(command)


def machine_has_network(name):
    out = subprocess.run([str(SMOLVM), "machine", "ls", "--json"], capture_output=True, text=True).stdout
    return any(m.get("name") == name and m.get("network") for m in json.loads(out))


def drop_network(name):
    """Turn off the network a packed machine may carry, and raise Refusal if it is still on."""
    run([str(SMOLVM), "machine", "update", "--name", name, "--no-net"], capture_output=True)
    if machine_has_network(name):
        raise Refusal(f"machine {name} still has a network after --no-net")


def wait_for(path, seconds=15):
    end = time.time() + seconds
    while time.time() < end:
        if os.path.exists(path):
            return True
        time.sleep(0.1)
    return False


def host_paths(roots, home, config_dir=None):
    paths = [str(p) for p in (home, home / ".claude", home / ".claude.json", home / ".ta", home / ".ssh", home / ".config" / "bt-cleanroom", home / "work")]
    paths += [str(home / "source" / "repos" / n) for n in ("byte-tactics", "byte-tactics-moddable")]
    paths += [str(r) for r in roots]
    if config_dir is not None:
        paths.append(str(config_dir))
    return sorted(set(paths))


def guest_env(proxy_url="http://127.0.0.1:3128"):
    return [
        f"HOME={GUEST_HOME}",
        f"HTTPS_PROXY={proxy_url}",
        f"https_proxy={proxy_url}",
        f"HTTP_PROXY={proxy_url}",
        "NO_PROXY=127.0.0.1,localhost",
        "CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC=1",
        "DISABLE_AUTOUPDATER=1",
        "GIT_CONFIG_NOSYSTEM=1",
        "CARGO_HOME=/home/agent/.cargo",
        "CARGO_NET_GIT_FETCH_WITH_CLI=false",
    ]


def prepare_guest(name, uid, gid):
    script = (
        f"mkdir -p {GUEST_HOME} && chown {uid}:{gid} {GUEST_HOME} && "
        f"echo 'agent:x:{uid}:{gid}::{GUEST_HOME}:/bin/sh' >> /etc/passwd && "
        f"printf '[user]\\n\\tname = Implementer\\n\\temail = implementer@cleanroom.invalid\\n"
        f"[safe]\\n\\tdirectory = *\\n[init]\\n\\tdefaultBranch = main\\n' > {GUEST_HOME}/.gitconfig && "
        f"chown {uid}:{gid} {GUEST_HOME}/.gitconfig && mkdir -p /opt/canary"
    )
    return run(exec_in(name, ["sh", "-c", script]))


def run_canary(name, manifest, sock_guest, roots, home, home_guest=GUEST_HOME, config_dir=None):
    for script in GUEST_SCRIPTS:
        r = run([str(SMOLVM), "machine", "cp", str(HERE / script), f"{name}:/opt/canary/{script}"])
        if r.returncode:
            return r.returncode
    r = run([str(SMOLVM), "machine", "cp", str(manifest), f"{name}:/opt/canary/manifest.txt"])
    if r.returncode:
        return r.returncode
    argv = ["python3", "/opt/canary/canary.py", "--manifest", "/opt/canary/manifest.txt",
            "--proxy-sock", sock_guest, "--home", home_guest]
    for p in host_paths(roots, home, config_dir):
        argv += ["--host-path", p]
    return run(exec_in(name, argv)).returncode


def remove_canary(name, runner=run):
    """Delete the canary directory from the VM and return 0 only when it is gone."""
    runner(exec_in(name, ["rm", "-rf", CANARY_DIR]))
    return runner(exec_in(name, ["test", "!", "-e", CANARY_DIR])).returncode


def broker_argv(session, game_dir=None):
    """Return the argv that starts broker.py for this session, with --game-dir when given."""
    argv = [sys.executable, str(HERE / "broker.py"), "serve", "--session", str(session)]
    if game_dir:
        argv += ["--game-dir", str(game_dir)]
    return argv


def start_session(args):
    """Validate, start everything, run the agent; return the agent's exit status."""
    home = Path.home()
    roots = repo_roots()
    config_dir = claude_config_dir(os.environ)
    check_api_key_env(args.api_key_env)
    if args.api_key_env:
        guest_var, secret_value = "ANTHROPIC_API_KEY", os.environ.get(args.api_key_env, "")
        if not secret_value:
            raise Refusal(f"--api-key-env {args.api_key_env} is not set")
    else:
        guest_var, secret_value = read_credential(args.credential_file, args.credential_kind, os.environ, args.host_credentials)
    tree = check_mount(args.tree, "tree", roots, home, config_dir=config_dir)
    datakit = check_mount(args.datakit, "data kit", roots, home, config_dir=config_dir)
    check_tree(tree)
    allowed_text = Path(args.allow).read_text()
    check_allowlist_text(allowed_text)
    image = Path(args.image).resolve()
    if not image.is_file():
        raise Refusal(f"image {image} does not exist: build it as docs/cleanroom.md says")
    if not SMOLVM.exists():
        raise Refusal(f"{SMOLVM} is not installed")
    if not os.access("/dev/kvm", os.R_OK | os.W_OK):
        raise Refusal("/dev/kvm is not usable")
    session = check_mount(args.session, "session", roots, home, must_exist=False, config_dir=config_dir)
    session.mkdir(parents=True, exist_ok=True)
    check_disjoint(("tree", tree), ("data kit", datakit), ("session", session))
    broker.prepare(session)
    manifest = session / "manifest.txt"
    mf.build(REPO, manifest, [])
    check_scan_clean(manifest, tree, datakit, session)

    name = f"cr-{os.getpid()}"
    sockdir = Path(tempfile.mkdtemp(prefix="cr-"))
    sock = sockdir / "p.sock"
    argv = create_argv(name, image, tree, datakit, session, sock)
    check_argv(argv)
    proxy_log = session / "proxy.log"
    procs = []
    created = False
    status = 1
    try:
        t0 = time.time()
        proxy = subprocess.Popen([sys.executable, str(HERE / "proxy.py"), "--allow", args.allow, "--log", str(proxy_log),
                                  "--listen", f"unix:{sock}"])
        procs.append(proxy)
        if not wait_for(sock):
            raise RuntimeError("the proxy did not start")
        procs.append(subprocess.Popen(broker_argv(session, args.game_dir)))
        if run(argv).returncode:
            raise RuntimeError("machine create failed")
        created = True
        drop_network(name)
        if run([str(SMOLVM), "machine", "start", "--name", name]).returncode:
            raise RuntimeError("machine start failed")
        t_boot = time.time()
        uid, gid = os.getuid(), os.getgid()
        if prepare_guest(name, uid, gid).returncode:
            raise RuntimeError("guest preparation failed")
        relay = ["sh", "-c", "socat TCP-LISTEN:3128,bind=127.0.0.1,fork,reuseaddr UNIX-CONNECT:/srv/proxy.sock"]
        run(exec_in(name, relay, detach=True))
        code = run_canary(name, manifest, "/srv/proxy.sock", roots, home, config_dir=config_dir)
        if remove_canary(name):
            print(f"launch: {CANARY_DIR} is still in the VM", file=sys.stderr)
            code = code or 4
        t_canary = time.time()
        os.replace(proxy_log, session / "canary-proxy.log")
        proxy.send_signal(signal.SIGHUP)
        print(f"launch: vm up in {t_boot - t0:.1f}s, canary checks in {t_canary - t_boot:.1f}s", flush=True)
        if code:
            print("launch: a canary check or the canary cleanup failed: the agent was not started", file=sys.stderr)
            return 3
        secret = [f"{guest_var}={SECRET_HOST_VAR}"]
        agent_env = {**os.environ, SECRET_HOST_VAR: secret_value}
        status = run(exec_in(name, args.command, user=f"{uid}:{gid}", env=guest_env(), secret_env=secret), env=agent_env).returncode
        print(f"launch: agent command exited {status} after {time.time() - t_canary:.1f}s", flush=True)
    finally:
        for p in reversed(procs):
            p.send_signal(signal.SIGTERM)
        for p in procs:
            with contextlib.suppress(Exception):
                p.wait(timeout=20)
        if created and not args.keep:
            run([str(SMOLVM), "machine", "stop", "--name", name], capture_output=True)
            run([str(SMOLVM), "machine", "delete", "--name", name, "--force"], capture_output=True)
        shutil.rmtree(sockdir, ignore_errors=True)
    if proxy_log.exists():
        run([sys.executable, str(HERE / "proxy.py"), "--check-log", str(proxy_log), "--allow", args.allow])
    return status


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--tree", required=True, help="clean-room working tree (a git repository), mounted read-write")
    ap.add_argument("--datakit", required=True, help="data kit directory, mounted read-only")
    ap.add_argument("--image", required=True, help=".smolmachine built from tools/cleanroom/Smolfile")
    ap.add_argument("--allow", default=str(HERE / "allowlist.txt"), help="proxy allowlist")
    ap.add_argument("--session", default=None, help="session directory (default: a new one in a temp dir)")
    ap.add_argument("--credential-file", default=str(DEFAULT_CREDENTIAL), help="file holding the implementer credential (mode 600)")
    ap.add_argument("--credential-kind", default="oauth", choices=sorted(CREDENTIAL_VARS), help="oauth: CLAUDE_CODE_OAUTH_TOKEN; api-key: ANTHROPIC_API_KEY")
    ap.add_argument("--host-credentials", default=None, help=argparse.SUPPRESS)
    ap.add_argument("--api-key-env", default=None, help=argparse.SUPPRESS)
    ap.add_argument("--game-dir", default=None, help="game directory the real gate plays on, when it is not in a Steam/GOG place")
    ap.add_argument("--keep", action="store_true", help="leave the machine in place after the command")
    ap.add_argument("command", nargs="+", help="command to run in the VM, after --")
    args = ap.parse_args(argv)
    if args.session is None:
        args.session = tempfile.mkdtemp(prefix="cr-session-")
    try:
        return start_session(args)
    except Refusal as err:
        print(f"launch: refused: {err}", file=sys.stderr)
        return EXIT_REFUSED


if __name__ == "__main__":
    sys.exit(main())
