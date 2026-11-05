"""Write the data kit Milestone 2 imports (#732, part of #388).

    uv run tools/oraclekit.py export DIR                    # the kit, MANIFEST.json and layout.json into DIR
    uv run tools/oraclekit.py export DIR --untagged         # the same without the interface and trace tags (CI, trials)
    uv run tools/oraclekit.py export DIR --build-dir D      # read the types from D/compile_commands.json (default build/modern)
    uv run tools/test_oraclekit.py                          # tests

The kit is the files of CONTENTS (the table below) in their own paths, `layout.json` (the state block
layout, generated from the modern build's types like tools/layoutasserts.py), `wasm/modules.json` (the
module manifest of the game without the fields that name the source files) and `README.md` (from
tools/oraclekit/README.md). `MANIFEST.json` lists each file's path, sha256 and size, the commit, the
`interfaces-v*` and `reference-traces-v*` tags at it and the tool's version. The kit's version is the
pair of tags; without both on the commit `export` refuses, and with --untagged it writes `"version": null`.
Any path under FORBIDDEN, and any file the table does not list, is refused with its path named, and so is a
generated file or the fuel pass that still names a source (SOURCE_NAME).

`layout.json` carries no `layout_refs`: they name the decompilation's functions, files and globals, and the globals
are not records of the state block. After the files are written, `leak_check` runs tools/speclint.py's address,
identifier and token-run rules over every text file of the kit and the export fails on a hit. The fenced-block rule
is not applied: the kit documents use `sh` and `json` fences for commands and schema examples. The identifiers a
file may use are those the kit's structured data defines (`allowed_identifiers`: the json, the interface headers and
everything under reference/ and tests/replay/), never the checked text itself. The interface headers declare the names
and are the source's own declarations, so only the address rule applies to them, and the export strips their
address-marker lines (`// FUNCTION: 0x...`, `// GLOBAL: 0x...`) before the check and the manifest digests.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

import speclint

ROOT = Path(__file__).resolve().parent.parent
TOOL_VERSION = 1
MANIFEST = "MANIFEST.json"
LAYOUT = "layout.json"
README = "README.md"
README_SOURCE = "tools/oraclekit/README.md"
MODULES = "wasm/modules.json"
FUEL_PASS = "tools/wasm_fuel.py"
MODULE_SOURCE_FIELDS = ("cxx", "c")
SOURCE_NAME = re.compile(r"(?<![\w.-])(?:src|include|orig|lib|build|cmake)/|\.(?:cpp|c|h)\b")
FORBIDDEN = ("src/", "include/", "orig/", "lib/", "build/", "cmake/")
TAG_PATTERNS = {"interfaces": re.compile(r"^interfaces-v(\d+)$"),
                "reference_traces": re.compile(r"^reference-traces-v(\d+)$")}

CONTENTS = [
    ("file", "docs/trace-format.md", "the trace spec"),
    ("file", "data/trace_schema.json", "the trace schema"),
    ("file", "tools/tracerehash.py", "a second implementation of the trace hashes, standard library only"),
    ("dir", "reference/traces", "the stored traces"),
    ("dir", "reference/scenarios", "the scenarios"),
    ("dir", "reference/sounds", "the stored sound logs"),
    ("dir", "reference/frames", "the stored frames"),
    ("dir", "reference/ui", "the stored UI logs"),
    ("dir", "reference/saves", "the saved games the scenarios load"),
    ("file", "docs/sound-log.md", "the sound log format"),
    ("file", "docs/frame-dump.md", "the frame dump format"),
    ("file", "docs/ui-log.md", "the UI log format"),
    ("file", "docs/reference-traces.md", "how the reference traces are made"),
    ("file", "docs/scenario-format.md", "the scenario format and the test-mode run"),
    ("dir", "interfaces", "the subsystem interface headers"),
    ("file", "docs/module-abi.md", "the module ABI"),
    ("file", FUEL_PASS, "the fuel pass of ABI 11, standard library and wasm-tools only"),
    ("dir", "tests/replay", "the stored replay recordings"),
    ("file", "docs/replay-format.md", "the replay format"),
    ("file", "data/replay_schema.json", "the annotations of the replay format"),
]

ROLES = {
    "game": "Game",
    "sim_globals": "SimGlobals",
    "params": "MeteorParams",
    "vec3": "Vec3",
    "weapon": "WeaponDef",
}
ROLE_HEADERS = ["src/state/sim_globals.h", "src/map/meteors.h"]
EXTRA_RECORDS = ["UnitWeaponSlot"]


class Refusal(Exception):
    """The export is refused; the message names the path or the tag."""


def forbidden(path: str) -> bool:
    """Whether the path lies under a directory that never leaves this repository."""
    return path.startswith(FORBIDDEN)


def listed(path: str, contents=None) -> bool:
    """Whether the contents table lists the file, by name or by directory."""
    for kind, entry, _ in (contents if contents is not None else CONTENTS):
        if kind == "file" and path == entry:
            return True
        if kind == "dir" and path.startswith(entry + "/"):
            return True
    return False


def admit(path: str, contents=None) -> None:
    """Raise Refusal unless the path may be exported."""
    if forbidden(path):
        raise Refusal("refused: %s is under %s" % (path, next(p for p in FORBIDDEN if path.startswith(p))))
    if not listed(path, contents):
        raise Refusal("refused: %s is not on the contents list of tools/oraclekit.py" % path)


def git(root: Path, *args: str) -> str:
    """The output of a git command in `root`."""
    return subprocess.run(["git", "-C", str(root), *args], check=True, capture_output=True, text=True).stdout


KIT_MODES = ("100644", "100755")


def tracked(root: Path, prefix: str) -> list[tuple[str, str]]:
    """The (path, mode) of every file in HEAD's tree under a path (the path itself for a file), sorted."""
    out = git(root, "ls-tree", "-r", "-z", "HEAD", "--", prefix)
    rows = []
    for record in out.split("\0")[:-1]:
        meta, path = record.split("\t", 1)
        rows.append((path, meta.split()[0]))
    return sorted(rows)


def blob(root: Path, path: str) -> bytes:
    """The bytes of a file as HEAD holds it, not as the working tree does."""
    return subprocess.run(["git", "-C", str(root), "cat-file", "blob", "HEAD:" + path], check=True,
                          capture_output=True).stdout


def select(root: Path, contents=None, lister=tracked) -> list[str]:
    """Every file of the contents table, admitted one by one; Refusal when an entry has no file or a file is no regular file."""
    out: list[str] = []
    for kind, entry, _ in (contents if contents is not None else CONTENTS):
        found = lister(root, entry)
        if not found:
            raise Refusal("refused: %s on the contents list has no tracked file" % entry)
        for path, mode in found:
            admit(path, contents)
            if mode not in KIT_MODES:
                raise Refusal("refused: %s has mode %s, only regular files (100644, 100755) are exported" % (path, mode))
            out.append(path)
    return sorted(set(out))


def version_tags(root: Path) -> dict[str, str | None]:
    """The highest interfaces-v* and reference-traces-v* tag on HEAD, each None when absent."""
    names = git(root, "tag", "--points-at", "HEAD").split()
    out: dict[str, str | None] = {}
    for key, pattern in TAG_PATTERNS.items():
        hits = [(int(m.group(1)), n) for n in names if (m := pattern.match(n))]
        out[key] = max(hits)[1] if hits else None
    return out


def check_tags(tags: dict[str, str | None], untagged: bool) -> dict | None:
    """The kit's version, the pair of tags; None with --untagged; Refusal when a tag is missing."""
    if untagged:
        return None
    missing = [k for k, v in tags.items() if v is None]
    if missing:
        raise Refusal("refused: no %s tag on the commit (--untagged writes a kit without a version)" %
                      ", ".join(TAG_PATTERNS[k].pattern.strip("^$").replace(r"(\d+)", "*") for k in missing))
    return dict(tags)


def digest(data: bytes) -> str:
    """The sha256 of the bytes in hex."""
    return hashlib.sha256(data).hexdigest()


def manifest_of(files: dict[str, bytes], commit: str, tags: dict, version: dict | None) -> dict:
    """The manifest: each file's path, sha256 and size, then the commit, the tags, the version and the tool version."""
    return {
        "tool_version": TOOL_VERSION,
        "commit": commit,
        "tags": tags,
        "version": version,
        "files": [{"path": p, "sha256": digest(files[p]), "size": len(files[p])} for p in sorted(files)],
    }


def verify(out: Path) -> list[str]:
    """The files of an export whose hash or size differs from the manifest, missing, or not in it."""
    manifest = json.loads((out / MANIFEST).read_text())
    problems: list[str] = []
    listed_paths = {f["path"] for f in manifest["files"]}
    for f in manifest["files"]:
        path = out / f["path"]
        if not path.is_file():
            problems.append("missing: " + f["path"])
            continue
        data = path.read_bytes()
        if digest(data) != f["sha256"] or len(data) != f["size"]:
            problems.append("differs: " + f["path"])
    for path in sorted(p.relative_to(out).as_posix() for p in out.rglob("*") if p.is_file()):
        if path != MANIFEST and path not in listed_paths:
            problems.append("unlisted: " + path)
    return problems


ANONYMOUS = re.compile(r"\((anonymous|unnamed)(?: \w+)? at [^)]*\)")
SIZE_PROBE = re.compile(r"char\[(\d+)\] s(\d+)$", re.M)


def clean_type(text: str) -> str:
    """A field type as clang prints it, without the build's absolute paths."""
    return ANONYMOUS.sub(r"(\1)", text)


def unnamed_bits(text: str, type_name: str, base: int = 0) -> list[dict]:
    """The bit-fields inside an unnamed record type of a layout dump, each as {name, bit, width} from the start of the owning field."""
    import stateclasses
    found = stateclasses.layout_fields(text, type_name)
    out: list[dict] = []
    for f in (found[1] if found else []):
        if f.bits:
            out.append({"name": f.name, "bit": base + f.start * 8 + f.bit_low, "width": f.bit_high - f.bit_low + 1})
        elif ANONYMOUS.search(f.type):
            out += unnamed_bits(text, f.type, base + f.start * 8)
    return out


def record_entry(text: str, name: str, size: int) -> dict:
    """{size, bases, vptr, fields} of one record of a clang layout dump; fields have no sizes yet."""
    import stateclasses
    found = stateclasses.layout_fields(text, name)
    fields: list[dict] = []
    bases: list[dict] = []
    vptr = False
    for f in (found[1] if found else []):
        if f.name == "pointer)":
            vptr = True
        elif f.name == "base)":
            bases.append({"type": clean_type(f.type.replace(" (primary", "")), "offset": f.start})
        elif re.match(r"^[A-Za-z_]\w*$", f.name):
            item = {"name": f.name, "offset": f.start, "size": None, "type": clean_type(f.type)}
            if f.bits:
                item["bit_offset"] = f.bit_low
                item["bit_width"] = f.bit_high - f.bit_low + 1
            elif ANONYMOUS.search(f.type) and (bits := unnamed_bits(text, f.type)):
                item["bits"] = bits
            fields.append(item)
    return {"size": size, "bases": bases, "vptr": vptr, "fields": fields}


def probe_text(includes: list[str], records: dict[str, dict]) -> tuple[str, list[tuple[str, dict]]]:
    """A translation unit with one char array per sized field, its length the field's sizeof, and the fields in order."""
    import layoutasserts
    order = [(r, f) for r, e in records.items() for f in e["fields"] if "bit_width" not in f]
    lines = ["#include " + i for i in layoutasserts.SYSTEM_INCLUDES] + layoutasserts.PRELUDE
    lines += ['#include "%s"' % i for i in includes]
    lines += ["#undef " + n for n in sorted({f["name"] for _, f in order})]
    lines += ["", "struct OraclekitSizes {"]
    lines += ["char s%d[sizeof(%s::%s)];" % (i, r, f["name"]) for i, (r, f) in enumerate(order)]
    lines.append("};")
    return "\n".join(lines) + "\n", order


def fill_sizes(includes: list[str], records: dict[str, dict], flags: list[str], directory: str, run) -> int:
    """Set each field's size from clang's own sizeof; fields clang refuses (private) keep None. Returns how many."""
    import layoutasserts
    text, order = probe_text(includes, records)
    for _ in range(3):
        result = run(flags, directory, text, True)
        bad = {int(m.group("line")) - 1 for m in layoutasserts.DIAGNOSTIC.finditer(result.stderr.replace("\r", ""))
               if Path(m.group("file")).name == layoutasserts.UNIT_NAME}
        if not bad:
            break
        text = "\n".join(l for i, l in enumerate(text.split("\n")) if i not in bad)
    else:
        raise SystemExit("oraclekit: the size probe does not settle")
    sizes = {int(m.group(2)): int(m.group(1)) for m in SIZE_PROBE.finditer(result.stdout)}
    count = 0
    for i, (_, f) in enumerate(order):
        if i in sizes:
            f["size"] = sizes[i]
            count += 1
    return count


def generate_layout(root: Path, build_dir: Path, run=None) -> dict:
    """The layout.json document: every record the block holds and the role-tagged records."""
    import layoutasserts
    run = run or layoutasserts.run_clang
    names, homes, cpp_only = layoutasserts.block_types()
    flags, directory = layoutasserts.clang_flags(build_dir)
    includes = layoutasserts.include_order(homes)
    extra = [Path(layoutasserts.os.path.relpath(root / h, layoutasserts.OUT_DIR)).as_posix() for h in ROLE_HEADERS]
    includes = includes + [i for i in extra if i not in includes]
    wanted = set(homes) | set(ROLES.values()) | set(EXTRA_RECORDS)
    probe = run(flags, directory, layoutasserts.emit(includes, []), True)
    if layoutasserts.DIAGNOSTIC.findall(probe.stderr) or probe.returncode:
        raise SystemExit("oraclekit: the headers do not compile together:\n" + "\n".join(probe.stderr.splitlines()[:20]))
    found = layoutasserts.parse_records(probe.stdout, wanted)
    absent = sorted(wanted - set(found))
    if absent:
        raise SystemExit("oraclekit: no layout for " + " ".join(absent))
    records = {n: record_entry(probe.stdout, n, found[n].size) for n in sorted(wanted)}
    unsized = fill_sizes(includes, records, flags, directory, run)
    total = sum(len([f for f in e["fields"] if "bit_width" not in f]) for e in records.values())
    if unsized != total:
        raise SystemExit("oraclekit: %d of %d field sizes could not be read from clang" % (total - unsized, total))
    for role, name in ROLES.items():
        records[name]["role"] = role
    return {"layout_version": 1, "records": records}


def kit_modules(data: bytes) -> bytes:
    """The module manifest of the kit: wasm/modules.json without the fields of a module entry that name its source files."""
    manifest = json.loads(data)
    for entry in manifest.get("modules", []):
        for field in MODULE_SOURCE_FIELDS:
            entry.pop(field, None)
    return (json.dumps(manifest, indent=1) + "\n").encode()


def names_source(data: bytes) -> list[str]:
    """What in a text names a source path or file: the matches of SOURCE_NAME."""
    return sorted({m.group(0) for m in SOURCE_NAME.finditer(data.decode())})


def check_names(files: dict[str, bytes]) -> None:
    """Raise Refusal when the kit's manifest or fuel pass names a source."""
    for path in (MODULES, FUEL_PASS):
        found = names_source(files[path]) if path in files else []
        if found:
            raise Refusal("refused: %s names the source: %s" % (path, ", ".join(found)))


STRUCTURED_DIRS = ("interfaces/", "tests/replay/", "reference/")
DECLARED_DIRS = ("interfaces/",)


def text_of(data: bytes) -> str | None:
    """The text of a file, None when it is binary."""
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        return None


def structured(path: str) -> bool:
    """Whether a kit file defines names: the json, the interface headers, the stored reference data and the recordings."""
    return path.endswith(".json") or path.startswith(STRUCTURED_DIRS)


def structured_text(files: dict[str, bytes]) -> str:
    """The text of the kit's files that define names, and the kit's own file names."""
    parts = [path.replace("/", " ") for path in files]
    for path in sorted(files):
        if structured(path) and (text := text_of(files[path])) is not None:
            parts.append(text)
    return "\n".join(parts)


def allowed_identifiers(files: dict[str, bytes]) -> set[str]:
    """The identifiers the kit defines: layout.json record and field names, the trace schema's field names, the manifest's names,
    the interface headers' names, the replay and scenario names, and the kit's file names; not the names in a kit document."""
    return speclint.identifiers(structured_text(files))


def leak_check(files: dict[str, bytes], root: Path, run: int = speclint.DEFAULT_RUN, index=None) -> list[str]:
    """One line per hit of speclint's address, identifier and token-run rules over every text file of the export."""
    source_ids, source_runs, literals = index if index is not None else speclint.source_index(root, run)
    kit = structured_text(files)
    allowed = speclint.identifiers(kit)
    hits: list[str] = []
    for path in sorted(files):
        text = text_of(files[path])
        if text is None:
            continue
        for number, message in speclint.lint(text, source_ids, allowed, source_runs, run, literals, kit):
            if message.startswith("fenced block"):
                continue
            if path.startswith(DECLARED_DIRS) and not message.startswith("address or marker"):
                continue
            hits.append("%s:%d: %s" % (path, number, message))
    return hits


MARKER_LINE = re.compile(rb"^[ \t]*//[ \t]*(?:FUNCTION|GLOBAL):[ \t]*0x[0-9a-fA-F]+.*(?:\r?\n|$)", re.M)


def strip_markers(path: str, data: bytes) -> bytes:
    """An interface header without its whole `// FUNCTION:` and `// GLOBAL:` address-marker lines; other files as they are."""
    return MARKER_LINE.sub(b"", data) if path.startswith(DECLARED_DIRS) else data


def check_target(out: Path) -> None:
    """Refuse a target that is a file or a directory with something in it."""
    if out.exists() and not out.is_dir():
        raise Refusal("refused: %s exists and is not a directory" % out)
    if out.is_dir() and any(out.iterdir()):
        raise Refusal("refused: %s is not empty" % out)


def export(root: Path, out: Path, untagged: bool, layout: dict, contents=None, lister=tracked, leaks=None) -> dict:
    """Write the kit to `out` and return its manifest. Nothing is written when the export is refused or leaks (`leaks` replaces leak_check in tests)."""
    paths = select(root, contents, lister)
    tags = version_tags(root)
    version = check_tags(tags, untagged)
    files = {p: strip_markers(p, blob(root, p)) for p in paths}
    files[LAYOUT] = (json.dumps(layout, indent=1, sort_keys=False) + "\n").encode()
    try:
        files[MODULES] = kit_modules(blob(root, MODULES))
    except subprocess.CalledProcessError:
        raise Refusal("refused: %s has no tracked file" % MODULES)
    check_names(files)
    files[README] = blob(root, README_SOURCE)
    hits = (leaks or leak_check)(files, root)
    if hits:
        raise Refusal("refused: the kit leaks the decompilation:\n" + "\n".join(hits))
    manifest = manifest_of(files, git(root, "rev-parse", "HEAD").strip(), tags, version)
    check_target(out)
    for path, data in files.items():
        target = out / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    (out / MANIFEST).write_text(json.dumps(manifest, indent=1) + "\n")
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    ex = sub.add_parser("export", help="write the data kit")
    ex.add_argument("dir", type=Path)
    ex.add_argument("--untagged", action="store_true", help="write the kit without the version tags (version: null)")
    ex.add_argument("--build-dir", type=Path, default=ROOT / "build" / "modern")
    args = parser.parse_args()
    try:
        check_target(args.dir)
        select(ROOT)
        check_tags(version_tags(ROOT), args.untagged)
        manifest = export(ROOT, args.dir, args.untagged, generate_layout(ROOT, args.build_dir))
    except Refusal as e:
        sys.exit("oraclekit: " + str(e))
    problems = verify(args.dir)
    if problems:
        sys.exit("oraclekit: the export does not match its manifest:\n" + "\n".join(problems))
    print("oraclekit: wrote %d files and %s to %s (version %s)" % (
        len(manifest["files"]), MANIFEST, args.dir, manifest["version"]))


if __name__ == "__main__":
    main()
