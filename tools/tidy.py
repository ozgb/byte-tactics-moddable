"""Run clang-tidy on the lines a change adds, and refuse new placeholder names.

    uv run tools/tidy.py [RANGE]                 # default origin/moddable...HEAD
    uv run tools/tidy.py --all FILE...           # every finding in the listed files
    uv run tools/tidy.py --clang-tidy PATH ...   # default: clang-tidy on PATH

Reads the lines RANGE adds (git diff -U0, as tools/commentlint.py does) and
runs clang-tidy, with the checks in .clang-tidy and the flags in
BUILD_DIR/compile_commands.json (configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON),
once on each changed C++ file under src/ and once on one translation unit for
each changed header under src/ or interfaces/ (the first, in path order, that
reaches the header through #include lines). Only findings on an added line are
reported. When the line replaces others (a hunk that also removes lines), a
finding is old, and not reported, if the base had the same check and message on
one of the removed lines; the base is checked in a temporary detached worktree,
only when needed, and a base that does not parse counts as having no findings.
Pure additions are always reported.

Also fails an added line that introduces a placeholder name from the
decompiler: FUN_<addr>, DAT_<addr>, field_<offset>, unk*, unknown_<offset>,
param_N or <Name>_<addr> (an 8 digit hex address starting 00). A name that
already occurs anywhere in the tree at the base of RANGE is not new.

Prints file:line: message [check] for each finding and exits 1 if there are
any. A translation unit that clang-tidy cannot parse is a tool error, not a
finding: it prints the compiler error and exits 2. Standard library only.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from commentlint import C_SUFFIXES, DEFAULT_RANGE, added_lines, git, new_text
from determinismlint import mask_c

DEFAULT_BUILD_DIR = "build/modern"
TIDY_DIRS = ("src/", "interfaces/")
HEADER_SUFFIXES = {".h", ".hpp", ".inc"}
SOURCE_SUFFIXES = {".c", ".cpp"}
INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.M)
DIAGNOSTIC = re.compile(r"^(.+?):(\d+):(\d+): (warning|error): (.*?)(?: \[([\w.,-]+)\])?$")

PLACEHOLDER_KINDS = (
    ("FUN_", re.compile(r"\bFUN_[0-9a-fA-F]{6,8}\b")),
    ("DAT_", re.compile(r"\bDAT_[0-9a-fA-F]{6,8}\b")),
    ("field_", re.compile(r"\bfield_[0-9a-fA-F]+\b")),
    ("unk", re.compile(r"\bunk(?:nown)?(?:_[0-9a-fA-F]+\w*|[0-9]\w*|[A-Z_]\w*)?\b")),
    ("param_", re.compile(r"\bparam_[0-9]+\b")),
    ("<Name>_<addr>", re.compile(r"\b[A-Za-z]\w*_00[0-9a-fA-F]{6}\b")),
)


class ToolError(Exception):
    """clang-tidy could not process a file."""


def base_of(root, rev_range):
    """The commit the lines of rev_range are added to."""
    m = re.match(r"^(.*?)(\.\.\.?)(.*)$", rev_range)
    if not m:
        return rev_range
    left, dots, right = m.groups()
    left = left or "HEAD"
    if dots == "...":
        return git(root, "merge-base", left, right or "HEAD").strip()
    return left


def placeholder_names(text, lines):
    """{line: [(kind, name)]} for the placeholder names on the given lines of a C or C++ file."""
    masked = mask_c(text).splitlines()
    found = {}
    for ln in sorted(lines):
        if not 1 <= ln <= len(masked):
            continue
        seen = set()
        for kind, pattern in PLACEHOLDER_KINDS:
            for m in pattern.finditer(masked[ln - 1]):
                if m.group(0) not in seen:
                    seen.add(m.group(0))
                    found.setdefault(ln, []).append((kind, m.group(0)))
    return found


def known_at(root, base, names):
    """The subset of names that occur as a whole word anywhere in the tree at base."""
    if not names:
        return set()
    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as fh:
        fh.write("\n".join(sorted(names)) + "\n")
    try:
        r = subprocess.run(["git", "-c", "core.quotepath=false", "grep", "-ohwF", "-f", fh.name, base, "--"],
                           cwd=root, capture_output=True, text=True)
    finally:
        os.unlink(fh.name)
    if r.returncode not in (0, 1):
        sys.exit(f"git grep: {r.stderr.strip()}")
    return set(r.stdout.split())


def lint_placeholders(root, rev_range, lines_by_path):
    """[(path, line, message)] for each added line that introduces a placeholder name."""
    found = {}
    for path, added in lines_by_path.items():
        if Path(path).suffix not in C_SUFFIXES or not added:
            continue
        hits = placeholder_names(new_text(root, rev_range, path), added)
        if hits:
            found[path] = hits
    names = {n for hits in found.values() for row in hits.values() for _, n in row}
    known = known_at(root, base_of(root, rev_range), names)
    out = []
    for path, hits in sorted(found.items()):
        for ln, row in sorted(hits.items()):
            for kind, name in row:
                if name not in known:
                    out.append((path, ln, f"new placeholder name {name} ({kind}): give it a real name"))
    return out


def parse_diagnostics(output, root):
    """[(path, line, kind, message, check)] for the top-level diagnostics in clang-tidy's output."""
    root = Path(root).resolve()
    out = []
    for row in output.splitlines():
        m = DIAGNOSTIC.match(row)
        if not m:
            continue
        path, line, _, kind, message, check = m.groups()
        p = Path(path)
        try:
            rel = (p if p.is_absolute() else root / p).resolve().relative_to(root).as_posix()
        except ValueError:
            continue
        if rel.startswith(TIDY_DIRS):
            out.append((rel, int(line), kind, message, check or ""))
    return out


def include_graph(root):
    """{basename: set of tracked C and C++ files that include a file with that basename}."""
    graph = {}
    for path in git(root, "ls-files").splitlines():
        if Path(path).suffix not in C_SUFFIXES or not path.startswith(TIDY_DIRS + ("link/", "include/")):
            continue
        text = (Path(root) / path).read_text(errors="replace")
        for target in INCLUDE.findall(text):
            graph.setdefault(Path(target).name, set()).add(path)
    return graph


def unit_for_header(header, graph, units):
    """The first translation unit of units (sorted paths) that reaches header through includes."""
    reached, todo = set(), [Path(header).name]
    while todo:
        name = todo.pop()
        for includer in graph.get(name, ()):
            if includer not in reached:
                reached.add(includer)
                todo.append(Path(includer).name)
    for unit in units:
        if unit in reached:
            return unit
    return None


def compile_units(root, build_dir):
    """The relative paths of the files in BUILD_DIR/compile_commands.json."""
    db = Path(root) / build_dir / "compile_commands.json"
    if not db.exists():
        raise ToolError(f"{db} not found: configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
    root = Path(root).resolve()
    units = set()
    for entry in json.loads(db.read_text()):
        try:
            units.add(Path(entry["file"]).resolve().relative_to(root).as_posix())
        except ValueError:
            continue
    return units


def run_tidy(clang_tidy, root, build_dir, path):
    """(exit status, output) of clang-tidy on one file."""
    cmd = [clang_tidy, "-p", str(Path(build_dir)), "--quiet"]
    overlay = Path(root) / build_dir / "overlay.json"
    if overlay.exists():
        cmd.append(f"--vfsoverlay={overlay}")
    cmd.append(path)
    try:
        r = subprocess.run(cmd, cwd=root, capture_output=True, text=True)
    except OSError as e:
        raise ToolError(f"cannot run {clang_tidy}: {e}")
    return r.returncode, r.stdout + r.stderr


def tidy_files(root, build_dir, paths, clang_tidy, jobs, runner=run_tidy):
    """{path: [diagnostics]} for each file, raising ToolError on a compiler error."""
    def one(path):
        status, output = runner(clang_tidy, root, build_dir, path)
        diags = parse_diagnostics(output, root)
        errors = [d for d in diags if d[2] == "error"]
        if errors or (status and not diags):
            detail = "\n".join(f"{p}:{ln}: {msg}" for p, ln, _, msg, _ in errors) or output.strip()
            raise ToolError(f"clang-tidy could not process {path}:\n{detail}")
        return path, diags

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        return dict(pool.map(one, paths))


def targets(root, build_dir, changed):
    """Sorted translation units to run for the changed paths, and the changed paths with no unit."""
    units = compile_units(root, build_dir)
    sources = sorted(u for u in units if Path(u).suffix in SOURCE_SUFFIXES and u.startswith(TIDY_DIRS))
    chosen, missed, graph = set(), [], None
    for path in sorted(changed):
        suffix = Path(path).suffix
        if not path.startswith(TIDY_DIRS):
            continue
        if suffix in SOURCE_SUFFIXES:
            (chosen.add(path) if path in units else missed.append(path))
        elif suffix in HEADER_SUFFIXES:
            graph = graph or include_graph(root)
            unit = unit_for_header(path, graph, sources)
            (chosen.add(unit) if unit else missed.append(path))
    return sorted(chosen), missed


def hunks_of(root, rev_range):
    """{path: [(old path, removed lines, added lines)]} from git diff -U0, one entry per hunk."""
    diff = git(root, "diff", "-U0", "--no-color", "--no-ext-diff", "--diff-filter=d", rev_range, "--")
    result, old_path, path = {}, None, None
    for row in diff.splitlines():
        if row.startswith("--- "):
            old_path = row[6:].split("\t")[0] if row.startswith("--- a/") else None
        elif row.startswith("+++ "):
            path = row[6:].split("\t")[0] if row.startswith("+++ b/") else None
        elif row.startswith("@@") and path:
            m = re.match(r"@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@", row)
            old, old_n, new, new_n = int(m.group(1)), int(m.group(2) or 1), int(m.group(3)), int(m.group(4) or 1)
            result.setdefault(path, []).append(
                (old_path or path, set(range(old, old + old_n)), set(range(new, new + new_n))))
    return result


def modified_hunk(hunks, path, line):
    """The (old path, removed lines) of the hunk that adds the line and removes others, or None."""
    for old_path, removed, added in hunks.get(path, ()):
        if removed and line in added:
            return old_path, removed
    return None


def base_findings(root, build_dir, base, units, clang_tidy, runner):
    """[diagnostics] that clang-tidy reports on the units at the base commit, or [] with a note on stderr."""
    tmp = Path(tempfile.mkdtemp(prefix="tidy-base-"))
    work = tmp / "base"
    found = []
    try:
        subprocess.run(["git", "worktree", "add", "-q", "--detach", str(work), base], cwd=root,
                       capture_output=True, text=True, check=True)
        head = Path(root).resolve()
        db_dir = tmp / "db"
        db_dir.mkdir()
        db = Path(root) / build_dir
        entries = json.loads((db / "compile_commands.json").read_text())
        for entry in entries:
            for key in ("command", "file", "output"):
                if key in entry:
                    entry[key] = entry[key].replace(str(head), str(work))
        (db_dir / "compile_commands.json").write_text(json.dumps(entries))
        if (db / "overlay.json").exists():
            shutil.copy(db / "overlay.json", db_dir / "overlay.json")
        for unit in units:
            try:
                found += tidy_files(str(work), str(db_dir), [unit], clang_tidy, 1, runner)[unit]
            except ToolError as e:
                print(f"note: no base findings for {unit}: {e}", file=sys.stderr)
    except (subprocess.CalledProcessError, OSError, ValueError, KeyError) as e:
        print(f"note: no base findings: {e}", file=sys.stderr)
    finally:
        subprocess.run(["git", "worktree", "remove", "--force", str(work)], cwd=root, capture_output=True)
        shutil.rmtree(tmp, ignore_errors=True)
    return found


def lint(root, rev_range, build_dir=DEFAULT_BUILD_DIR, clang_tidy="clang-tidy", jobs=None, runner=run_tidy):
    """[(path, line, message)] for the added lines: new clang-tidy findings and new placeholder names.

    A finding on a line that replaces others is old when the base had the same check and message
    on one of the lines the hunk removes.
    """
    lines_by_path = added_lines(root, rev_range)
    findings = list(lint_placeholders(root, rev_range, lines_by_path))
    changed = [p for p, added in lines_by_path.items() if added]
    units, missed = targets(root, build_dir, changed)
    for path in missed:
        print(f"note: no translation unit reaches {path}, not checked by clang-tidy", file=sys.stderr)
    results = tidy_files(root, build_dir, units, clang_tidy, jobs or os.cpu_count() or 1, runner)
    hunks = hunks_of(root, rev_range)
    seen, candidates = set(), []
    for unit, diags in results.items():
        for path, line, _, message, check in diags:
            if line not in lines_by_path.get(path, ()) or (path, line, message, check) in seen:
                continue
            seen.add((path, line, message, check))
            candidates.append((unit, path, line, message, check))
    modified_units = sorted({u for u, p, ln, _, _ in candidates if modified_hunk(hunks, p, ln)})
    base = []
    if modified_units:
        base = base_findings(root, build_dir, base_of(root, rev_range), modified_units, clang_tidy, runner)
    old = {(p, check, message, ln) for p, ln, _, message, check in base}
    for unit, path, line, message, check in candidates:
        hunk = modified_hunk(hunks, path, line)
        if hunk and any((hunk[0], check, message, ln) in old for ln in hunk[1]):
            continue
        findings.append((path, line, f"{message} [{check}]"))
    return sorted(set(findings))


def lint_all(root, files, build_dir=DEFAULT_BUILD_DIR, clang_tidy="clang-tidy", jobs=None, runner=run_tidy):
    """[(path, line, message)] for every finding in the listed files."""
    results = tidy_files(root, build_dir, sorted(files), clang_tidy, jobs or os.cpu_count() or 1, runner)
    return sorted({(path, line, f"{message} [{check}]")
                   for diags in results.values() for path, line, _, message, check in diags})


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("range", nargs="?", default=DEFAULT_RANGE)
    ap.add_argument("--all", nargs="+", metavar="FILE", help="report every finding in these files")
    ap.add_argument("--clang-tidy", default="clang-tidy", help="the clang-tidy binary")
    ap.add_argument("--build-dir", default=DEFAULT_BUILD_DIR, help="holds compile_commands.json")
    ap.add_argument("-j", "--jobs", type=int, default=None)
    args = ap.parse_args(argv)
    root = git(".", "rev-parse", "--show-toplevel").strip()
    if shutil.which(args.clang_tidy) is None and not Path(args.clang_tidy).exists():
        print(f"tidy.py: {args.clang_tidy} not found", file=sys.stderr)
        return 2
    try:
        if args.all:
            findings = lint_all(root, args.all, args.build_dir, args.clang_tidy, args.jobs)
        else:
            findings = lint(root, args.range, args.build_dir, args.clang_tidy, args.jobs)
    except ToolError as e:
        print(f"tidy.py: {e}", file=sys.stderr)
        return 2
    for path, line, message in findings:
        print(f"{path}:{line}: {message}")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
