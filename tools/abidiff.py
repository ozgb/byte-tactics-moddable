"""Compare the interface headers with a baseline and fail on a change that has no version bump.

    uv run tools/abidiff.py                      # baseline: newest interfaces-v* tag, else the merge base with origin/moddable
    uv run tools/abidiff.py --base REF           # baseline: the headers at REF
    uv run tools/abidiff.py --report-standins    # list the writable int*, void* and char* in slot signatures
    uv run tools/test_abidiff.py                 # tests, on synthetic header pairs

For each interfaces/*.h that has `<NAME>_VERSION_MAJOR` and `_MINOR` macros, the tool compares the
header with the one at the baseline and prints one line per interface. It exits non-zero when one
fails. Headers without version macros (seams.h) are skipped.

What is compared, per struct or class the header defines:

  slots    the function-pointer fields of a struct and the virtual methods of a class: name, order,
           full signature and calling convention
  layout   the member offsets, member types and size, for a struct with data members or a class with
           a vtable pointer, bases or data members (a pure function table is covered by its slots)
  entries  for services.h and host.h only: every free function and non-virtual member function (static ones
           too), keyed `Class::Method` or the plain name, with its return type, parameter types and const-ness;
           order doesn't matter, as a module imports them by name and a host exports them by name. Declarations
           inside `extern "C" {` count as top-level ones (host.h holds all of its own there). The verdict line
           of these two ends with the number of entries judged
  version  the `_VERSION_MAJOR` and `_VERSION_MINOR` macros

Every change is classed, and the class decides which bump it needs:

  additive  a slot or a field appended at the end, an entry added, a new struct, or a stand-in retype (below):
            needs a minor bump, or a major bump
  breaking  a slot or an entry removed, reordered, inserted before the end or retyped: needs a major bump, and an
            adapter for each changed slot unless no module was built against the old major
  layout    a field that moves or changes size: needs a major bump, and no module built against the old major
            (no adapter can cover a data layout)

A major bump with changes passes in one of two ways, read from wasm/modules.json (#566):

  - no module in the manifest was built against the old major (its "majors" for the interface), so nothing
    needs to be adapted: `ok (major 2->3, no module built against 2)`;
  - the manifest has an adapter ("adapters": interface, from_major, name) for the old major and every slot the
    change touches, a slot reordered in a struct touching all of that struct's slots: `ok (major 1->2, adapters:
    Foo, Bar)`.

Otherwise it fails with `FAIL (adapter required: Foo, Bar)`, naming the slots with no adapter. The baseline's
major is the old major; a module built against a major between the baseline's and the current one is not seen.

A bump with nothing changed is fine. A version that goes down fails.

For services.h, the entries must also be exactly the names of `signatures.services` in wasm/modules.json, by import name
(`Class::Method` is imported as `Class_Method`), each with the kinds the manifest gives it, and every name of a module's
`imports.services` must be one of them: one line, `services (imports): ok` or a FAIL naming each entry without a
signature, each signature the header doesn't declare, each import it doesn't declare, and each entry whose kinds differ.
A service may be declared before any module imports it. The engine's other services live in src/util/engine_services.h,
which no module includes.

A retype is a parameter or return type that changed in a slot. Once an `interfaces-v*` tag exists, every
retype is breaking (even `void*` to `Foo*`). Until then (the baseline is the merge base), a retype that
keeps the 4-byte argument slot of each parameter and of the return value, and the calling convention, is
additive and reported as `retype (before interfaces-v1)`: a minor bump is enough. Pointer to pointer,
pointer to an integer of 4 bytes or less, and a small integer to another small integer keep the slot. A
retype that changes a slot is breaking: a by-value struct, a double, an added or removed parameter, a
different calling convention. Each retype is printed as its own line.

Layouts and signatures come from clang-cl (LLVM 22.1.8, as cmake/clang-cl-i686.cmake) with the flags
of the modern build, on a one-line translation unit that includes the header: `-Xclang
-fdump-record-layouts-complete` for the layouts and `-Xclang -ast-dump=json` for the slot types. No build
directory is needed.

The baseline headers are read with `git show REF:interfaces/F.h` into a temporary directory. REF is
the newest `interfaces-v*` tag; until one exists, the merge base of HEAD and origin/moddable.

A header with a row in data/frozen_interfaces.csv (#1089) must also have the row's major, whatever the baseline: one
line each, `ok (frozen at major N by <decision>)` or `FAIL (frozen at major N by <decision>, ...)`, even when the bump
would otherwise pass. A sanctioned bump moves the row's major and cites the human's decision in the same pull request.
Rows for reference/core/ are tools/coregate.py's (`--check-frozen`).
"""

import argparse
import atexit
import csv
import io
import functools
import json
import os
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MSVC_INCLUDE = ROOT / "toolchain" / "msvc5-sp3" / "INCLUDE"
BASE_BRANCH = "origin/moddable"
MODULES_JSON = ROOT / "wasm" / "modules.json"
FROZEN = ROOT / "data" / "frozen_interfaces.csv"
CLANG = ["clang-cl", "--target=i686-pc-windows-msvc", "/nologo", "-TP", "-fms-compatibility",
         "-fms-extensions", "-fms-compatibility-version=11.00", "-fsyntax-only",
         "-Xclang", "-fdump-record-layouts-complete", "-Xclang", "-ast-dump=json"]
MAJOR = re.compile(r"^\s*#\s*define\s+(\w+)_VERSION_MAJOR\s+(\d+)", re.M)
MINOR = re.compile(r"^\s*#\s*define\s+(\w+)_VERSION_MINOR\s+(\d+)", re.M)
CALLCONV = re.compile(r"\s*__attribute__\(\((\w+)\)\)\s*$")
ENTRY_INTERFACES = {"services", "host"}
IMPORTED_INTERFACES = {"services"}
LAYOUT_START = re.compile(r"^\*\*\* Dumping AST Record Layout\s*$", re.M)
LAYOUT_MEMBER = re.compile(r"^\s*(\d+(?::\d+-\d+)?)\s*\|(\s*)(.*?)\s*$")
ANONYMOUS = re.compile(r"\(anonymous((?: \w+)?) at [^)]*\)")
LAYOUT_SIZE = re.compile(r"^\s*\|\s*\[sizeof=(\d+), align=(\d+)")
STAND_IN = re.compile(r"(?:volatile )?(?:int|void|char) \*(?: (?:const|volatile))*")
POINTER = re.compile(r".*\*(?: (?:const|volatile))*$")
SMALL_INTEGER = {"int", "unsigned", "signed", "short", "char", "long", "bool", "const", "volatile"}
JSON_START = re.compile(r'^\{\n  "id"', re.M)


@dataclass(frozen=True)
class Signature:
    ret: str
    params: tuple
    conv: str

    def text(self) -> str:
        return "%s (%s) %s" % (self.ret, ", ".join(self.params), self.conv or "cdecl")


@dataclass
class Interface:
    name: str = ""
    major: int = 0
    minor: int = 0
    slots: dict = field(default_factory=dict)
    entries: dict = field(default_factory=dict)
    layouts: dict = field(default_factory=dict)


@dataclass
class Change:
    kind: str
    text: str
    slots: tuple = ()


@dataclass
class Result:
    name: str
    ok: bool
    summary: str
    details: list = field(default_factory=list)


def split_top(text: str) -> list[str]:
    """The comma-separated parts of a parameter list, ignoring commas inside brackets."""
    parts, depth, start = [], 0, 0
    for i, c in enumerate(text):
        if c in "<([":
            depth += 1
        elif c in ">)]":
            depth -= 1
        elif c == "," and depth == 0:
            parts.append(text[start:i].strip())
            start = i + 1
    if text.strip():
        parts.append(text[start:].strip())
    return parts


def parse_signature(text: str, is_pointer: bool) -> Signature:
    """A Signature from clang's type text: `R (*)(P, Q) __attribute__((cc))`, or `R (P, Q)` for a method."""
    conv = ""
    qualifiers = ""
    if not is_pointer:
        while True:
            tail = re.search(r"\s+(const|volatile|noexcept)\s*$", text)
            if tail is None:
                break
            qualifiers = " " + tail.group(1) + qualifiers
            text = text[:tail.start()]
    found = CALLCONV.search(text)
    if found:
        conv = found.group(1)
        text = text[:found.start()]
    if is_pointer:
        head = text.index("(*)(")
        ret = text[:head].strip()
        body = text[head + len("(*)("):]
        depth = 1
        for i, c in enumerate(body):
            depth += (c == "(") - (c == ")")
            if depth == 0:
                body = body[:i]
                break
        return Signature(ret, tuple(split_top(body)), conv)
    text = text.rstrip()
    depth = 0
    for i in range(len(text) - 1, -1, -1):
        depth += (text[i] == ")") - (text[i] == "(")
        if depth == 0:
            break
    ret, body = text[:i].strip(), text[i + 1:-1]
    return Signature(ret, tuple(split_top(body)), conv + qualifiers)


def is_function_pointer(qual: str) -> bool:
    return "(*)(" in qual


@functools.cache
def system_include() -> list[str]:
    """clang-cl flags for the MSVC 5 headers, through lower-case symlinks (the toolchain's file names are upper case); none without the toolchain."""
    if not MSVC_INCLUDE.is_dir():
        return []
    tmp = tempfile.TemporaryDirectory(prefix="abidiff-inc-")
    atexit.register(tmp.cleanup)
    for here, _, names in os.walk(MSVC_INCLUDE):
        target = Path(tmp.name) / Path(here).relative_to(MSVC_INCLUDE).as_posix().lower()
        target.mkdir(parents=True, exist_ok=True)
        for name in names:
            (target / name.lower()).symlink_to(Path(here) / name)
    return ["-imsvc", tmp.name]


def clang_dump(header: Path, scratch: Path) -> tuple[dict, str]:
    """clang-cl's syntax tree and record layouts of a translation unit that includes the header."""
    scratch.mkdir(parents=True, exist_ok=True)
    unit = scratch / "unit.cpp"
    unit.write_text('#include "%s"\n' % header.as_posix())
    run = subprocess.run(CLANG + system_include() + ["--", str(unit)], capture_output=True, text=True, errors="replace")
    found = JSON_START.search(run.stdout)
    if run.returncode != 0 or found is None:
        raise SystemExit("%s: clang-cl failed\n%s" % (header, run.stderr[-2000:]))
    return json.loads(run.stdout[found.start():]), run.stderr + run.stdout[:found.start()]


def parse_layouts(text: str) -> dict:
    """Record name -> (members, size, align), from the -fdump-record-layouts-complete text."""
    out = {}
    for block in LAYOUT_START.split(text)[1:]:
        block = block.split("\n\n")[0]
        members, size, name = [], None, None
        for line in block.split("\n"):
            size_line = LAYOUT_SIZE.match(line)
            if size_line:
                size = (int(size_line.group(1)), int(size_line.group(2)))
                continue
            found = LAYOUT_MEMBER.match(line)
            if not found:
                continue
            if name is None:
                name = re.sub(r"^(struct|class|union) ", "", found.group(3))
                continue
            members.append((found.group(1), len(found.group(2)), ANONYMOUS.sub(r"(anonymous\1)", found.group(3))))
        if name is not None and size is not None:
            out[name] = (tuple(members), size[0], size[1])
    return out


def load_interface(header: Path, scratch: Path) -> Interface:
    """The versions, slots and layouts of one interface header."""
    text = header.read_text()
    major, minor = MAJOR.search(text), MINOR.search(text)
    iface = Interface(name=header.stem)
    if major is None or minor is None:
        return iface
    iface.major, iface.minor = int(major.group(2)), int(minor.group(2))
    tree, layout_text = clang_dump(header, scratch)
    layouts = parse_layouts(layout_text)
    named = header.stem in ENTRY_INTERFACES
    found_entries: dict[str, list[Signature]] = {}
    here = None
    for node in top_level(tree["inner"]):
        loc = node.get("loc", {})
        file = loc.get("file") or (loc.get("spellingLoc") or {}).get("file") or (loc.get("expansionLoc") or {}).get("file")
        here = file or here
        in_header = here == header.as_posix()
        if named and in_header and node["kind"] == "FunctionDecl" and not node.get("isImplicit"):
            add_entry(found_entries, node["name"], node, "")
        if node.get("isImplicit") or node["kind"] != "CXXRecordDecl" or "name" not in node:
            continue
        if not node.get("completeDefinition") or not in_header:
            continue
        slots, data = [], False
        for member in node.get("inner", []):
            kind, qual = member["kind"], member.get("type", {}).get("qualType", "")
            if kind == "FieldDecl" and is_function_pointer(qual):
                slots.append((member["name"], parse_signature(qual, True)))
            elif kind == "FieldDecl":
                data = True
            elif kind == "CXXMethodDecl" and member.get("virtual"):
                slots.append((member["name"], parse_signature(qual, False)))
            elif kind == "CXXMethodDecl" and named and in_header and not member.get("isImplicit"):
                add_entry(found_entries, "%s::%s" % (node["name"], member["name"]), member, " static" if member.get("storageClass") == "static" else "")
        has_vtable = any(m["kind"] == "CXXMethodDecl" and m.get("virtual") for m in node.get("inner", []))
        iface.slots[node["name"]] = slots
        if data or has_vtable or node.get("bases"):
            iface.layouts[node["name"]] = layouts.get(node["name"])
    for name, sigs in found_entries.items():
        for sig in sigs:
            iface.entries[name if len(sigs) == 1 else "%s(%s)" % (name, ", ".join(sig.params))] = sig
    return iface


def top_level(nodes: list) -> list:
    """The top-level declarations in source order, with those of each `extern "C"` block in its place."""
    out = []
    for node in nodes:
        out.append(node)
        if node["kind"] == "LinkageSpecDecl":
            out += top_level(node.get("inner", []))
    return out


def add_entry(found: dict, name: str, node: dict, extra: str) -> None:
    """Record a free function or a non-virtual member under its name; an overload keeps every distinct signature."""
    sig = parse_signature(node.get("type", {}).get("qualType", ""), False)
    sig = Signature(sig.ret, sig.params, sig.conv + extra)
    if sig not in found.setdefault(name, []):
        found[name].append(sig)


def keeps_slot(old: str, new: str) -> bool:
    """Whether two types both fill exactly one 4-byte argument slot: a pointer, or an integer of 4 bytes or less."""
    def one_slot(text: str) -> bool:
        words = text.split()
        return bool(POINTER.match(text)) or (bool(words) and words.count("long") < 2 and set(words) <= SMALL_INTEGER)
    return one_slot(old) and one_slot(new)


def compare_signature(where: str, old: Signature, new: Signature, tagged: bool) -> list[Change]:
    if old == new:
        return []
    slot = (where.split(".")[-1],)
    if tagged or old.conv != new.conv or len(old.params) != len(new.params):
        return [Change("breaking", "%s: %s -> %s" % (where, old.text(), new.text()), slot)]
    out = []
    pairs = [("return", old.ret, new.ret)] + [("param %d" % (i + 1), a, b)
                                              for i, (a, b) in enumerate(zip(old.params, new.params))]
    for label, a, b in pairs:
        if a == b:
            continue
        kind = "retype" if keeps_slot(a, b) else "breaking"
        out.append(Change(kind, "%s %s: %s -> %s" % (where, label, a, b), slot))
    return out


def compare_slots(record: str, old: list, new: list, tagged: bool) -> list[Change]:
    out = []
    old_names, new_names = [n for n, _ in old], [n for n, _ in new]
    old_by, new_by = dict(old), dict(new)
    for name in old_names:
        if name not in new_by:
            out.append(Change("breaking", "slot removed: %s.%s" % (record, name), (name,)))
    common_old = [n for n in old_names if n in new_by]
    common_new = [n for n in new_names if n in old_by]
    if common_old != common_new:
        out.append(Change("breaking", "slots reordered in %s" % record, tuple(common_old)))
    tail = new_names[len(old_names):] if new_names[:len(old_names)] == old_names else None
    for name in new_names:
        if name in old_by:
            continue
        if tail is not None and name in tail:
            out.append(Change("additive", "new slot: %s.%s" % (record, name)))
        else:
            out.append(Change("breaking", "slot inserted before the end: %s.%s" % (record, name), (name,)))
    for name in common_old:
        out += compare_signature("%s.%s" % (record, name), old_by[name], new_by[name], tagged)
    return out


def compare_entries(old: dict, new: dict) -> list[Change]:
    """Named entries (free functions and non-virtual members): removed or retyped is breaking, added is additive."""
    out = []
    overloaded = {}
    for name in old:
        if name not in new and any(k.startswith(name + "(") for k in new):
            overloaded[name] = [k for k in new if k.startswith(name + "(")]
    single = {}
    for key in old:
        base = key.split("(")[0]
        if "(" in key and base in new:
            single.setdefault(base, []).append(key)
    for name, keys in overloaded.items():
        out.append(Change("breaking", "%s: overloaded (%d signatures), one import name" % (name, len(keys)), (name.replace("::", "_"),)))
    for name, keys in single.items():
        out.append(Change("breaking", "%s: no longer overloaded (%d signatures -> 1)" % (name, len(keys)), (name.replace("::", "_"),)))
    skip = set(overloaded) | {k for keys in overloaded.values() for k in keys} | set(single) | {k for keys in single.values() for k in keys}
    for name, sig in old.items():
        if name in skip:
            continue
        slot = (name.replace("::", "_"),)
        if name not in new:
            out.append(Change("breaking", "entry removed: %s" % name, slot))
        elif new[name] != sig:
            out.append(Change("breaking", "%s: %s -> %s" % (name, sig.text(), new[name].text()), slot))
    out += [Change("additive", "new entry: %s" % name) for name in new if name not in old and name not in skip]
    return out


SAME_WIDTH = re.compile(r"\blong(?! long)\b(?<!long long)")


def member_shape(member: tuple) -> tuple:
    """A layout member without its name, with long spelled int (the same width on every target): (offset, depth, type)."""
    offset, depth, text = member
    return offset, depth, SAME_WIDTH.sub("int", text.rsplit(" ", 1)[0])


def compare_layout(record: str, old: tuple, new: tuple) -> list[Change]:
    if old is None or new is None:
        return [Change("layout", "no layout from clang for %s" % record)] if old != new else []
    old_members, old_size, _ = old
    new_members, new_size, _ = new
    old_shapes, new_shapes = [member_shape(m) for m in old_members], [member_shape(m) for m in new_members]
    old_offsets: dict[tuple, set] = {}
    for offset, depth, text in old_members:
        old_offsets.setdefault((depth, text.rsplit(" ", 1)[-1]), set()).add(offset)
    moved = any(m[0] not in old_offsets.get((m[1], m[2].rsplit(" ", 1)[-1]), {m[0]}) for m in new_members)
    if new_shapes[:len(old_shapes)] != old_shapes or new_size < old_size or moved:
        return [Change("layout", "field moved or changed size in %s" % record)]
    changes = []
    if new_members[:len(old_members)] != old_members:
        changes.append(Change("breaking", "field renamed or retyped at the same offset and width in %s" % record))
    if len(new_members) > len(old_members):
        changes.append(Change("additive", "field appended to %s" % record))
    return changes


def compare_interfaces(old: Interface, new: Interface, tagged: bool) -> list[Change]:
    """Every change from old to new, each of kind additive, retype, breaking or layout."""
    changes = []
    for record in old.slots:
        if record not in new.slots:
            changes.append(Change("breaking", "struct removed: %s" % record, tuple(n for n, _ in old.slots[record])))
    for record, slots in new.slots.items():
        if record not in old.slots:
            changes.append(Change("additive", "new struct: %s" % record))
        else:
            changes += compare_slots(record, old.slots[record], slots, tagged)
    for record, layout in new.layouts.items():
        if record in old.layouts:
            changes += compare_layout(record, old.layouts[record], layout)
    return changes + compare_entries(old.entries, new.entries)


def built_against(manifest: dict | None, interface: str, major: int) -> list[str]:
    """The names of the modules in the manifest built against the given major of an interface."""
    return [m["name"] for m in (manifest or {}).get("modules", []) if m.get("majors", {}).get(interface) == major]


def adapted_slots(manifest: dict | None, interface: str, major: int) -> set[str]:
    """The slots and imports the manifest has an adapter for, from the given major of an interface."""
    return {a["name"] for a in (manifest or {}).get("adapters", []) if a.get("interface") == interface and a.get("from_major") == major}


def judge(old: Interface, new: Interface, changes: list[Change], manifest: dict | None = None) -> Result:
    """The verdict on the changes, given the version bump and the module manifest (wasm/modules.json)."""
    kinds = {c.kind for c in changes}
    details = [c.text if c.kind != "retype" else "retype (before interfaces-v1), " + c.text for c in changes]
    major_up, minor_up = new.major > old.major, new.minor > old.minor
    versions = "%d.%d -> %d.%d" % (old.major, old.minor, new.major, new.minor)

    def fail(reason: str) -> Result:
        return Result(new.name, False, "FAIL (%s)" % reason, details)

    if (new.major, new.minor) < (old.major, old.minor):
        return fail("version went down, %s" % versions)
    if "layout" in kinds and not (major_up and not built_against(manifest, new.name, old.major)):
        return fail("a field moved or changed size, which only a major bump with no module built against the old major allows")
    if not changes:
        bump = ", " + bump_text(old, new) if (old.major, old.minor) != (new.major, new.minor) else ""
        return Result(new.name, True, "ok (no change%s)" % bump)
    if major_up:
        bump = "major %d->%d" % (old.major, new.major)
        if not built_against(manifest, new.name, old.major):
            return Result(new.name, True, "ok (%s, no module built against %d)" % (bump, old.major), details)
        changed = sorted({slot for c in changes if c.kind in ("breaking", "retype") for slot in c.slots})
        have = adapted_slots(manifest, new.name, old.major)
        missing = [slot for slot in changed if slot not in have]
        if missing:
            return fail("adapter required: %s" % ", ".join(missing))
        return Result(new.name, True, "ok (%s, %s)" % (bump, "adapters: " + ", ".join(changed) if changed else "no slot changed"), details)
    if "breaking" in kinds:
        return fail("breaking change needs a major bump, version is %s" % versions)
    if not minor_up:
        return fail("additive change needs a minor bump, version is %s" % versions)
    return Result(new.name, True, "ok (additive, %s)" % bump_text(old, new), details)


def import_name(entry: str) -> str:
    """The name a module imports an entry by: `Class::Method` as `Class_Method`, an overload without its parameters."""
    return entry.split("(")[0].replace("::", "_")


def manifest_imports(manifest: dict | None, interface: str) -> set[str]:
    """The union of every module's imports of an interface in the manifest."""
    return {name for m in (manifest or {}).get("modules", []) for name in m.get("imports", {}).get(interface, {})}


HANDLE_CLASS = re.compile(r"^(?://.*only as a handle.*)\n(?:class|struct)\s+(\w+)\s*;", re.M)
FLOAT_KIND = {"float": "f32", "double": "f64"}


def handle_classes(header_text: str) -> set[str]:
    """The classes a header forward-declares for modules to hold only as a handle: the declaration whose comment line says "only as a handle"."""
    return set(HANDLE_CLASS.findall(header_text))


def kind_of(type_text: str, handles: set[str]) -> str:
    """The kind (docs/module-abi.md section 7) of a C type: float f32, double f64, void void, a pointer to a handle class
    handle, any other pointer linear, anything else (integers, bool, enums) int. A result of kind block (a pointer into
    the block, FindWeaponByName) is not told from linear by the type: the entry-point headers declare none, and the
    services entries return none."""
    words = type_text.replace("*", " * ").split()
    if "*" in words:
        base = [w for w in words if w not in ("const", "volatile", "struct", "class", "*")]
        return "handle" if base and base[-1] in handles and words.count("*") == 1 else "linear"
    bare = [w for w in words if w not in ("const", "volatile")]
    if bare == ["void"]:
        return "void"
    return FLOAT_KIND.get(" ".join(bare), "int")


def signature_kinds(sig: Signature, handles: set[str]) -> tuple[list[str], str]:
    """The manifest's form of a declared entry: the kinds of its parameters and of its result."""
    params = [] if sig.params == ("void",) else sig.params
    return [kind_of(p, handles) for p in params], kind_of(sig.ret, handles)


def check_imports(iface: Interface, manifest: dict | None, handles: set[str] | None = None) -> Result:
    """Whether an imported interface declares exactly the entries of the manifest's signatures for it, each with the
    parameter and result kinds they give it, and every name a module imports of it."""
    declared = {import_name(e): sig for e, sig in iface.entries.items()}
    recorded = (manifest or {}).get("signatures", {}).get(iface.name, {})
    imported = manifest_imports(manifest, iface.name)
    details = []
    for text, names in (("declared but without a signature in the manifest", set(declared) - set(recorded)),
                        ("a signature in the manifest but not declared", set(recorded) - set(declared)),
                        ("imported but not declared", imported - set(declared))):
        if names:
            details.append("%s: %s" % (text, ", ".join(sorted(names))))
    name = "%s (imports)" % iface.name
    if details:
        return Result(name, False, "FAIL (the entries differ from the manifest's signatures)", details)
    for entry in sorted(declared):
        params, ret = signature_kinds(declared[entry], handles or set())
        have = recorded[entry]
        if [list(have[0]), have[1]] != [params, ret]:
            details.append("%s: the header declares (%s) -> %s, the manifest has (%s) -> %s" % (
                entry, ", ".join(params), ret, ", ".join(have[0]), have[1]))
    if details:
        return Result(name, False, "FAIL (the kinds differ from the manifest's signatures)", details)
    return Result(name, True, "ok (%d entries, each with its kinds; %d imported)" % (len(declared), len(imported)))


def imports_results(directory: Path, manifest: dict | None) -> list[Result]:
    """The import check of each interface header in directory that modules import by name."""
    out = []
    with tempfile.TemporaryDirectory(prefix="abidiff-") as tmp:
        for name in sorted(IMPORTED_INTERFACES):
            header = directory / ("%s.h" % name)
            if header.exists():
                out.append(check_imports(load_interface(header, Path(tmp) / name), manifest, handle_classes(header.read_text())))
    return out


def bump_text(old: Interface, new: Interface) -> str:
    if new.major != old.major:
        return "major %d->%d" % (old.major, new.major)
    return "minor %d->%d" % (old.minor, new.minor)


def diff_headers(old_path: Path, new_path: Path, scratch: Path, tagged: bool, manifest: dict | None = None) -> Result:
    """The verdict for one interface header against its baseline."""
    name = new_path.stem if new_path.exists() else old_path.stem
    if not old_path.exists():
        return Result(name, True, "ok (new interface)")
    if not new_path.exists():
        return Result(name, False, "FAIL (interface removed)")
    old = load_interface(old_path, scratch / "old")
    new = load_interface(new_path, scratch / "new")
    if not new.major and not new.minor and not old.major and not old.minor:
        return Result(name, True, "skipped (no version macros)")
    result = judge(old, new, compare_interfaces(old, new, tagged), manifest)
    if name in ENTRY_INTERFACES:
        result.summary = "%s, %d entries)" % (result.summary[:-1], len(new.entries))
    return result


def diff_dirs(old_dir: Path, new_dir: Path, tagged: bool, manifest: dict | None = None) -> list[Result]:
    """The verdicts for every header in either directory, sorted by name."""
    names = sorted({p.name for p in old_dir.glob("*.h")} | {p.name for p in new_dir.glob("*.h")})
    with tempfile.TemporaryDirectory(prefix="abidiff-") as tmp:
        def one(name: str) -> Result:
            return diff_headers(old_dir / name, new_dir / name, Path(tmp) / name, tagged, manifest)
        with ThreadPoolExecutor() as pool:
            return list(pool.map(one, names))


def frozen_rows(text: str) -> dict[str, dict]:
    """The rows of data/frozen_interfaces.csv, keyed by header path."""
    return {row["header"]: row for row in csv.DictReader(io.StringIO(text))}


def check_frozen(rows: dict[str, dict], root: Path) -> list[Result]:
    """One verdict for each frozen interface header under root: its major must be the row's."""
    out = []
    for path, row in sorted(rows.items()):
        if not path.startswith("interfaces/"):
            continue
        frozen = "frozen at major %s by %s" % (row["major"], row["decision"])
        name = "%s (frozen)" % Path(path).stem
        header = root / path
        found = MAJOR.search(header.read_text()) if header.exists() else None
        if found is None:
            out.append(Result(name, False, "FAIL (%s, %s)" % (frozen, "header removed" if not header.exists() else "no major")))
        elif found.group(2) != row["major"]:
            out.append(Result(name, False, "FAIL (%s, header is at major %s)" % (frozen, found.group(2))))
        else:
            out.append(Result(name, True, "ok (%s)" % frozen))
    return out


def git(*args: str) -> str:
    run = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True)
    if run.returncode != 0:
        raise SystemExit("git %s failed\n%s" % (" ".join(args), run.stderr.strip()))
    return run.stdout


def baseline_ref() -> tuple[str, bool]:
    """The baseline ref and whether an interfaces-v* tag exists: the newest tag, else the merge base of HEAD and the base branch."""
    tags = git("tag", "-l", "interfaces-v*", "--sort=-v:refname").split()
    return (tags[0], True) if tags else (git("merge-base", "HEAD", BASE_BRANCH).strip(), False)


def extract_baseline(ref: str, target: Path) -> None:
    """Write interfaces/*.h as they were at ref into target."""
    for path in git("ls-tree", "--name-only", ref, "interfaces/").split():
        if path.endswith(".h"):
            (target / Path(path).name).write_text(git("show", "%s:%s" % (ref, path)))


def standins(directory: Path) -> list[str]:
    """One line for each int*, void* and char* whose target is not const in a slot signature of the headers in directory."""
    out = []
    with tempfile.TemporaryDirectory(prefix="abidiff-") as tmp:
        for header in sorted(directory.glob("*.h")):
            iface = load_interface(header, Path(tmp) / header.stem)
            for record, slots in iface.slots.items():
                for slot, sig in slots:
                    pairs = [("return", sig.ret)] + [("param %d" % (i + 1), p) for i, p in enumerate(sig.params)]
                    out += ["%s.%s.%s %s: %s" % (header.stem, record, slot, label, text)
                            for label, text in pairs if STAND_IN.fullmatch(text)]
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--base", metavar="REF", help="baseline ref (default: newest interfaces-v* tag, else the merge base with %s)" % BASE_BRANCH)
    parser.add_argument("--report-standins", action="store_true", help="list every int*, void* and char* whose target is not const in a slot signature")
    args = parser.parse_args()
    if args.report_standins:
        lines = standins(ROOT / "interfaces")
        print("\n".join(lines))
        print("%d writable int*, void* or char* in slot signatures" % len(lines))
        return
    ref, tagged = baseline_ref()
    ref = args.base or ref
    manifest = json.loads(MODULES_JSON.read_text())
    with tempfile.TemporaryDirectory(prefix="abidiff-base-") as tmp:
        extract_baseline(ref, Path(tmp))
        results = diff_dirs(Path(tmp), ROOT / "interfaces", tagged, manifest)
    results += imports_results(ROOT / "interfaces", manifest)
    results += check_frozen(frozen_rows(FROZEN.read_text()) if FROZEN.exists() else {}, ROOT)
    print("baseline: %s (%s)" % (ref, "interfaces-v* tag exists" if tagged else "before interfaces-v1"))
    for result in results:
        print("%s: %s" % (result.name, result.summary))
        for line in result.details:
            print("    " + line)
    sys.exit(0 if all(r.ok for r in results) else 1)


if __name__ == "__main__":
    main()
