"""Canary checks for a clean-room session VM: every one must fail to reach its target.

    python3 canary.py --manifest FILE --proxy-sock PATH [--host-path PATH ...] [--home DIR] [--scan-root DIR ...]

Runs inside the VM under a plain python3, next to manifest.py, before the agent starts. Prints
`PASS name` or `FAIL name: why` per check and exits 1 if any failed. The checks: host paths do not
exist, no default route, no direct DNS or TCP to the internet, the proxy refuses GitHub and other
forbidden targets, no credentials in the environment or the home directory, no file of the
manifest anywhere in the scan roots (default /), and the scanner itself finds a planted file.
"""

import argparse
import contextlib
import io
import os
import socket
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import manifest as mf

FORBIDDEN_ENV = ("GH_TOKEN", "GITHUB_TOKEN", "GH_ENTERPRISE_TOKEN", "SSH_AUTH_SOCK", "GIT_ASKPASS", "SSH_ASKPASS")
HOME_ENTRIES = (".ssh", ".config/gh", ".git-credentials", ".netrc", ".gnupg", ".claude", ".claude.json", ".ta")
PROXY_REFUSED = (
    "github.com:443",
    "github.com:22",
    "api.github.com:443",
    "raw.githubusercontent.com:443",
    "codeload.github.com:443",
    "127.0.0.1:22",
    "169.254.169.254:80",
)
DIRECT_TARGETS = (("140.82.112.4", 443), ("1.1.1.1", 443), ("8.8.8.8", 53))


def check_host_paths(paths):
    found = [p for p in paths if os.path.lexists(p)]
    return (not found, f"reachable: {found}" if found else "")


def check_default_route():
    routes = Path("/proc/net/route").read_text().splitlines()[1:]
    default = [r for r in routes if r.split()[1] == "00000000"]
    return (not default, f"default route: {default[0]}" if default else "")


def check_direct_dns():
    try:
        socket.setdefaulttimeout(3)
        socket.getaddrinfo("github.com", 443)
    except OSError:
        return True, ""
    return False, "github.com resolved"


def check_direct_tcp():
    reached = []
    for target in DIRECT_TARGETS:
        try:
            socket.create_connection(target, timeout=3).close()
            reached.append(target)
        except OSError:
            pass
    return (not reached, f"connected: {reached}" if reached else "")


def connect_status(sock_path, target):
    """Send CONNECT through the proxy socket and return the status code, or 0 if it hung up."""
    s = socket.socket(socket.AF_UNIX)
    s.settimeout(10)
    try:
        s.connect(sock_path)
        s.sendall(f"CONNECT {target} HTTP/1.1\r\nHost: {target}\r\n\r\n".encode())
        head = s.recv(256).decode("latin-1")
    except OSError:
        return 0
    finally:
        s.close()
    parts = head.split(" ", 2)
    return int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else 0


def check_proxy_refuses(sock_path):
    if not os.path.exists(sock_path):
        return False, f"proxy socket {sock_path} is missing"
    bad = []
    for target in PROXY_REFUSED:
        status = connect_status(sock_path, target)
        if status != 403:
            bad.append((target, status))
    return (not bad, f"not refused with 403: {bad}" if bad else "")


def check_environment():
    found = [n for n in FORBIDDEN_ENV if n in os.environ]
    return (not found, f"set: {found}" if found else "")


def check_home(home):
    found = [e for e in HOME_ENTRIES if os.path.lexists(os.path.join(home, e))]
    return (not found, f"present in {home}: {found}" if found else "")


def check_manifest_scan(manifest, roots, max_bytes):
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        code = mf.scan(manifest, roots, max_bytes)
    return code == 0, out.getvalue().strip().replace("\n", "; ")


def check_scanner_works(max_bytes):
    with tempfile.TemporaryDirectory() as tmp:
        planted = Path(tmp) / "planted-copy.bin"
        planted.write_bytes(b"canary self-test " + os.urandom(16))
        manifest = Path(tmp) / "m.txt"
        manifest.write_text(f"{mf.sha256_file(planted)}  planted\n")
        renamed = Path(tmp) / "sub"
        renamed.mkdir()
        (renamed / "other-name.dat").write_bytes(planted.read_bytes())
        planted.unlink()
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = mf.scan(manifest, [tmp], max_bytes)
    return code == 1, "the scanner missed a planted file" if code != 1 else ""


def run(args):
    checks = [
        ("host paths absent", lambda: check_host_paths(args.host_path)),
        ("no default route", check_default_route),
        ("no direct DNS", check_direct_dns),
        ("no direct TCP", check_direct_tcp),
        ("proxy refuses github and other targets", lambda: check_proxy_refuses(args.proxy_sock)),
        ("no credentials in the environment", check_environment),
        ("no credentials in the home directory", lambda: check_home(args.home)),
        ("scanner finds a planted file", lambda: check_scanner_works(args.max_bytes)),
        ("no file of the manifest is visible", lambda: check_manifest_scan(args.manifest, args.scan_root, args.max_bytes)),
    ]
    failed = 0
    for name, fn in checks:
        ok, why = fn()
        if ok:
            print(f"PASS {name}" + (f" ({why})" if why else ""))
        else:
            failed += 1
            print(f"FAIL {name}: {why}")
    print(f"canary checks: {len(checks) - failed} passed, {failed} failed")
    return 1 if failed else 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--manifest", required=True)
    ap.add_argument("--proxy-sock", required=True)
    ap.add_argument("--host-path", action="append", default=[])
    ap.add_argument("--home", default=os.path.expanduser("~"))
    ap.add_argument("--scan-root", action="append", default=None)
    ap.add_argument("--max-bytes", type=int, default=mf.DEFAULT_MAX)
    args = ap.parse_args(argv)
    args.scan_root = args.scan_root or ["/"]
    return run(args)


if __name__ == "__main__":
    sys.exit(main())
