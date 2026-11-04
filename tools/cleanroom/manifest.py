"""Canary manifest for the clean-room sandbox: hash the repository's files, then scan for them.

    uv run tools/cleanroom/manifest.py build ROOT --out FILE [--extra PATH ...]
    python3 tools/cleanroom/manifest.py scan MANIFEST ROOT... [--max-bytes N]

`build` writes `sha256  path` for every regular file under ROOT/src and ROOT/include, plus
ROOT/orig/TotalA.exe and ~/.ta/TotalA.exe when they exist, plus each --extra file. Symlinks are not
followed. The lines are sorted, and the paths are only for the host's report.

`scan` walks each ROOT without following symlinks, skips /proc, /sys and /dev, hashes each regular
file up to --max-bytes (default 64 MiB) and prints every file whose hash is in the manifest. It
exits 1 on any match, else 0. Unreadable files are counted, not fatal. It needs only the standard
library and the manifest, so it runs under a plain python3 inside the sandbox.
"""

import argparse
import hashlib
import os
import stat
import sys
from pathlib import Path

SKIP = ("/proc", "/sys", "/dev")
DEFAULT_MAX = 64 * 1024 * 1024


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def is_regular(path):
    try:
        return stat.S_ISREG(os.lstat(path).st_mode)
    except OSError:
        return False


def walk_files(root):
    """Yield every regular file path under root, following no symlinks."""
    root = str(root)
    if is_regular(root):
        yield root
        return
    for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
        if any(dirpath == s or dirpath.startswith(s + "/") for s in SKIP):
            dirnames[:] = []
            continue
        dirnames.sort()
        for name in sorted(filenames):
            path = os.path.join(dirpath, name)
            if is_regular(path):
                yield path


def build(root, out, extra):
    root = Path(root)
    files = []
    for sub in ("src", "include"):
        files.extend(walk_files(root / sub))
    for p in (root / "orig" / "TotalA.exe", Path.home() / ".ta" / "TotalA.exe", *map(Path, extra)):
        if is_regular(p):
            files.append(str(p))
    lines = set()
    for path in files:
        shown = os.path.relpath(path, root) if path.startswith(str(root) + os.sep) else path
        lines.add(f"{sha256_file(path)}  {shown}")
    Path(out).write_text("".join(line + "\n" for line in sorted(lines)))
    print(f"manifest: {len(lines)} files")
    return 0


def load_manifest(path):
    hashes = {}
    for line in Path(path).read_text().splitlines():
        digest, _, name = line.partition("  ")
        if len(digest) == 64:
            hashes.setdefault(digest, name)
    return hashes


def scan(manifest, roots, max_bytes):
    hashes = load_manifest(manifest)
    scanned = skipped = unreadable = matches = 0
    for root in roots:
        for path in walk_files(root):
            try:
                if os.lstat(path).st_size > max_bytes:
                    skipped += 1
                    continue
                digest = sha256_file(path)
            except OSError:
                unreadable += 1
                continue
            scanned += 1
            if digest in hashes:
                matches += 1
                print(f"{path}  matches {hashes[digest]}")
    if unreadable:
        print(f"canary: {unreadable} files unreadable")
    if skipped:
        print(f"canary: {skipped} files over the size cap")
    if matches:
        print(f"canary: {matches} of {scanned} files scanned are in the manifest")
        return 1
    print(f"canary: {scanned} files scanned, no file of the manifest")
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build")
    b.add_argument("root")
    b.add_argument("--out", required=True)
    b.add_argument("--extra", action="append", default=[])
    s = sub.add_parser("scan")
    s.add_argument("manifest")
    s.add_argument("roots", nargs="+")
    s.add_argument("--max-bytes", type=int, default=DEFAULT_MAX)
    args = ap.parse_args(argv)
    if args.cmd == "build":
        return build(args.root, args.out, args.extra)
    return scan(args.manifest, args.roots, args.max_bytes)


if __name__ == "__main__":
    sys.exit(main())
