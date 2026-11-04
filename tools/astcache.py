#!/usr/bin/env python3
"""Share one clang syntax-tree dump per source file between the data checks.

rwsets.py, hazards.py, stateclasses.py and refcheck.py each ran clang-cl with
`-Xclang -ast-dump=json` over every file of compile_commands.json, cut the text
into top-level declarations and decoded the ones in the repository. This module
does that once per file and keeps the result, so the four checks read it back
and only walk it.

What is kept, per translation unit, is a `Unit`: the text clang printed before
the tree (the record layouts), its stderr (the uninitialised-use warnings), and
every top-level declaration that lives under src/, include/, link/ or
interfaces/, decoded and annotated with its file and line (`_file`, `_line`,
`_end`). The system headers, which are nearly all of the dump, are dropped.
Each tool applies its own file filter to those declarations.

One command line serves all four tools: the one of rwsets.py with the layout
flag, the tree flag and the warning flags added (`canonical`). The extra flags
change only the layout text before the tree and clang's stderr, and each tool
ignores what it does not read.

The cache is a directory, `<build-dir>/astcache`, with one file per command
line and working directory. A file holds its key and then the pickled `Unit`.
The key is the hash of the command line, the working directory, clang itself,
this file, the source file, and the name and contents of every file under every
include directory of the command line, the forced includes, the VFS overlay,
src/, link/, include/ and interfaces/. That is coarse on purpose: a change to
any of those files fills the unit again, and a stale tree is never read. A file
that does not match its key, or does not unpickle, is a miss and is replaced.

BT_NO_ASTCACHE=1 (or `--no-astcache` on each tool) turns the cache off, and
each tool then runs clang itself as it always did. BT_ASTCACHE_DIR overrides the
directory. When clang fails on a file, `unit` returns None and the caller runs
its own path, which reports the error as before.

`uv run tools/astcache.py fill --build-dir DIR` fills the cache of every file
the four tools read, in parallel, so that the tools can then run side by side.
"""

import argparse
import hashlib
import json
import os
import pickle
import re
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
from typing import NamedTuple

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sources import ROOT

ENV_DIR = "BT_ASTCACHE_DIR"
ENV_OFF = "BT_NO_ASTCACHE"
CACHE_NAME = "astcache"
LAYOUT_FLAG = ["-Xclang", "-fdump-record-layouts-complete"]
AST_FLAG = ["-Xclang", "-ast-dump=json"]
WARNING_FLAGS = ["-Wuninitialized", "-Wsometimes-uninitialized", "-Wconditional-uninitialized"]
SHARED_DIRS = ("src", "link", "include", "interfaces")
PROJECT_PREFIXES = tuple(d + os.sep for d in SHARED_DIRS)
INCLUDE_ENV = ("INCLUDE", "CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH", "OBJC_INCLUDE_PATH")
DIR_OPTIONS = ("-I", "/I", "-isystem", "-imsvc", "-iquote", "-idirafter")
FILE_OPTIONS = ("-include", "-vfsoverlay", "-imacros")
JSON_START = re.compile(r'^\{\n  "id"', re.M)
CHUNK = re.compile(r"^    \{\n.*?^    \},?\n", re.S | re.M)
INCLUDED_FROM = re.compile(r'"includedFrom": \{\s*"file": "[^"]*"\s*\}')
MAX_TOKENS = 2000


class Unit(NamedTuple):
    """The shared dump of one file: layout text, stderr, and (file, has cast, node) per project declaration."""
    layouts: str
    stderr: str
    chunks: list


class Position:
    """Follows clang's delta-encoded source locations through a dump in order."""

    def __init__(self, file: str, line: int):
        self.file = file
        self.line = line

    def feed(self, loc: dict | None) -> tuple[str, int] | None:
        """Apply one location (its macro expansion if it has one) and return (file, line)."""
        if not loc:
            return None
        if "spellingLoc" in loc:
            self.feed(loc["spellingLoc"])
            loc = loc["expansionLoc"]
        if "file" in loc:
            self.file = loc["file"]
        if "line" in loc:
            self.line = loc["line"]
        return self.file, self.line

    def annotate(self, node: dict) -> None:
        """Set _file, _line and _end on a node and every node under it."""
        here = self.feed(node.get("loc"))
        begin = end = None
        rng = node.get("range")
        if rng:
            begin = self.feed(rng.get("begin"))
            end = self.feed(rng.get("end"))
        first = here or begin or (self.file, self.line)
        node["_file"], node["_line"] = first
        node["_end"] = (end or first)[1]
        for child in node.get("inner", ()):
            self.annotate(child)


def last_position(chunk: str, file: str, line: int) -> tuple[str, int]:
    """The (file, line) state after a chunk of dump text."""
    text = INCLUDED_FROM.sub("", chunk)
    i = text.rfind('"file": "')
    if i >= 0:
        j = text.index('"', i + 9)
        file = text[i + 9:j]
    i = text.rfind('"line": ')
    if i >= 0:
        m = re.match(r"\d+", text[i + 8:])
        line = int(m.group())
    return file, line


def first_file(chunk: str) -> str | None:
    """The file the first location of a chunk names, or None when it repeats the previous one."""
    text = INCLUDED_FROM.sub("", chunk[:2000])
    i = text.find('"file": "')
    c = text.find('"col"')
    if i < 0 or (0 <= c < i):
        return None
    return text[i + 9:text.index('"', i + 9)]


def is_project(file: str) -> bool:
    """Whether a file of the dump is under src/, include/, link/ or interfaces/."""
    return os.path.relpath(os.path.realpath(file), ROOT).startswith(PROJECT_PREFIXES)


def canonical(argv: list[str]) -> list[str]:
    """The command line every tool shares: argv with the layout, tree and warning flags in one fixed order."""
    i = argv.index("--")
    head = argv[:i]
    out = []
    n = 0
    while n < len(head):
        if head[n:n + 2] in (LAYOUT_FLAG, AST_FLAG):
            n += 2
        elif head[n] in WARNING_FLAGS:
            n += 1
        else:
            out.append(head[n])
            n += 1
    return out + LAYOUT_FLAG + AST_FLAG + WARNING_FLAGS + argv[i:]


def build_unit(stdout: str, stderr: str) -> Unit | None:
    """The Unit of a clang run, or None when stdout holds no syntax tree."""
    found = JSON_START.search(stdout)
    if found is None:
        return None
    text = stdout[found.start():]
    chunks = []
    position = ("", 0)
    for m in CHUNK.finditer(text, text.index('"inner": [')):
        chunk = m.group()
        file = first_file(chunk) or position[0]
        if not file or not is_project(file):
            position = last_position(chunk, *position)
            continue
        node = json.loads(chunk.rstrip().rstrip(","))
        tracker = Position(*position)
        tracker.annotate(node)
        position = (tracker.file, tracker.line)
        chunks.append((file, "IntegralToPointer" in chunk, node))
    return Unit(stdout[:found.start()], stderr, chunks)


def enabled() -> bool:
    return not os.environ.get(ENV_OFF)


def disable() -> None:
    """Turn the cache off for this process and the workers it starts."""
    os.environ[ENV_OFF] = "1"


def configure(build_dir: Path) -> None:
    """Use `<build_dir>/astcache` unless BT_ASTCACHE_DIR names another directory or the cache is off."""
    if enabled():
        os.environ.setdefault(ENV_DIR, str(Path(build_dir) / CACHE_NAME))


def cache_dir() -> Path | None:
    """The cache directory, or None when the cache is off or not configured."""
    if not enabled() or not os.environ.get(ENV_DIR):
        return None
    return Path(os.environ[ENV_DIR])


_digests: dict[str, bytes] = {}


def digest_path(path: str) -> bytes:
    """The hash of a file, or of the names and contents of every file under a directory."""
    path = os.path.realpath(path)
    if path not in _digests:
        h = hashlib.blake2b(digest_size=16)
        if os.path.isdir(path):
            for base, dirs, files in os.walk(path):
                dirs.sort()
                for name in sorted(files):
                    full = os.path.join(base, name)
                    h.update(os.path.relpath(full, path).encode(errors="replace") + b"\0")
                    try:
                        h.update(Path(full).read_bytes())
                    except OSError:
                        h.update(b"<unreadable>")
                    h.update(b"\0")
        elif os.path.isfile(path):
            h.update(Path(path).read_bytes())
        else:
            h.update(b"<missing>")
        _digests[path] = h.digest()
    return _digests[path]


def dependency_paths(argv: list[str]) -> list[str]:
    """Every directory and file whose contents the command line can pull into the dump."""
    i = argv.index("--")
    paths = [str(ROOT / d) for d in SHARED_DIRS] + argv[i + 1:]
    for name in INCLUDE_ENV:
        paths += [p for p in re.split(r"[;" + re.escape(os.pathsep) + "]", os.environ.get(name, "")) if p]
    n = 0
    while n < i:
        arg = argv[n]
        for option in DIR_OPTIONS:
            if arg == option and n + 1 < i:
                paths.append(argv[n + 1])
            elif arg.startswith(option) and arg != option:
                paths.append(arg[len(option):])
        if arg in FILE_OPTIONS and n + 1 < i:
            paths.append(argv[n + 1])
        elif arg.startswith("/FI") and len(arg) > 3:
            paths.append(arg[3:])
        n += 1
    return sorted(set(paths))


_clang_ids: dict[str, bytes] = {}


def clang_id(exe: str) -> bytes:
    """What identifies the clang binary: its path, size, modification time and version text."""
    if exe not in _clang_ids:
        h = hashlib.blake2b(digest_size=16)
        found = exe if os.path.dirname(exe) else next(
            (os.path.join(d, exe) for d in os.environ.get("PATH", "").split(os.pathsep)
             if os.path.exists(os.path.join(d, exe))), exe)
        real = os.path.realpath(found)
        h.update(real.encode())
        try:
            info = os.stat(real)
            h.update(b"%d:%d" % (info.st_size, info.st_mtime_ns))
            h.update(subprocess.run([real, "--version"], capture_output=True, errors="replace").stdout.encode())
        except OSError:
            h.update(b"<missing>")
        _clang_ids[exe] = h.digest()
    return _clang_ids[exe]


def key_of(argv: list[str], directory: str) -> bytes:
    """The key of a unit: its inputs, hashed (see the module doc)."""
    h = hashlib.blake2b(digest_size=16)
    h.update(clang_id(argv[0]))
    h.update(digest_path(__file__))
    h.update(json.dumps([argv, directory, [os.environ.get(n, "") for n in INCLUDE_ENV]]).encode())
    for path in dependency_paths(argv):
        h.update(digest_path(path))
    return h.hexdigest().encode()


def slot_of(argv: list[str], directory: str) -> str:
    return hashlib.blake2b(json.dumps([argv, directory]).encode(), digest_size=12).hexdigest() + ".pickle"


def load(path: Path, key: bytes) -> Unit | None:
    """The Unit stored in a cache file when its key is `key`, else None."""
    try:
        head, _, body = path.read_bytes().partition(b"\n")
        if head != key:
            return None
        found = pickle.loads(body)
    except (OSError, EOFError, ValueError, AttributeError, ImportError, IndexError, pickle.UnpicklingError):
        return None
    return found if isinstance(found, Unit) else None


def store(path: Path, key: bytes, unit: Unit) -> None:
    """Write a cache file whole, so that a reader never sees half of it."""
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name("%s.%d.tmp" % (path.name, os.getpid()))
    temporary.write_bytes(key + b"\n" + pickle.dumps(unit, protocol=pickle.HIGHEST_PROTOCOL))
    os.replace(temporary, path)


def fetch(argv: list[str], directory: str) -> tuple[Unit | None, bool]:
    """(unit, filled): the shared dump of a file, and whether this call ran clang to make it.

    The unit is None when the cache is off or clang failed, and the caller then runs clang itself."""
    folder = cache_dir()
    if folder is None:
        return None, False
    argv = canonical(argv)
    key = key_of(argv, directory)
    path = folder / slot_of(argv, directory)
    found = load(path, key)
    if found is not None:
        return found, False
    run = subprocess.run(argv, cwd=directory, capture_output=True, text=True, errors="replace")
    if run.returncode != 0 and "error:" in run.stderr:
        return None, False
    unit = build_unit(run.stdout, run.stderr)
    if unit is None:
        return None, False
    store(path, key, unit)
    return unit, True


def unit(argv: list[str], directory: str) -> Unit | None:
    """The shared dump of a file, read from the cache or made and stored; None when the caller must run clang."""
    return fetch(argv, directory)[0]


def fill_one(job: tuple[list[str], str]) -> int:
    """Make one unit current; 1 when clang had to run, 0 when it was cached, -1 when clang failed."""
    found, filled = fetch(*job)
    return -1 if found is None else int(filled)


def jobs_of(build_dir: Path) -> list[tuple[list[str], str]]:
    """The (command line, directory) of every file the four tools read, without duplicates."""
    import refcheck
    import rwsets
    import typedupes
    sys.setrecursionlimit(100000)
    jobs = [(canonical(argv), directory) for _, argv, directory in rwsets.compile_commands(build_dir, [])]
    jobs += [(canonical(refcheck.ast_argv(entry)), entry["directory"])
             for entry in typedupes.translation_units(build_dir).values()]
    return sorted({(tuple(argv), directory) for argv, directory in jobs})


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("command", choices=["fill"])
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build/rwsets")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--no-astcache", action="store_true", help="leave the cache off")
    args = parser.parse_args()
    if args.no_astcache:
        disable()
    configure(args.build_dir)
    if cache_dir() is None:
        print("astcache: off")
        return
    started = time.time()
    jobs = [(list(argv), directory) for argv, directory in jobs_of(args.build_dir)]
    with ProcessPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(fill_one, jobs, chunksize=2))
    failed = results.count(-1)
    print("astcache: %d files, %d filled, %d already there, %d failed (%.0fs), %s"
          % (len(jobs), results.count(1), results.count(0), failed, time.time() - started, cache_dir()))
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    import astcache
    astcache.main()
