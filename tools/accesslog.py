"""Record which subsystem reads and writes which state item, tick by tick, in an instrumented test-mode build.

    cmake -S . -B build/access -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-i686.cmake -DBT_ACCESS_LOG=ON
    cmake --build build/access                       # also runs `tables` after the link
    uv run tools/accesslog.py tables --build-dir build/access
    uv run tools/accesslog.py run --build-dir build/access combat-deaths console-economy console-iwin
    uv run tools/accesslog.py run --build-dir build/access --raw combat-deaths
    uv run tools/test_accesslog.py                   # tests

This is step 1 of #830. The option BT_ACCESS_LOG builds the game's sources with ASan's instrumentation
pass in callback mode (no ASan runtime): every load and store of the game's code calls a function of
lib/accesslog/access_log.cpp, which also stands in for memcpy, memset and memmove. The callbacks attribute
each access to the function that made it by return address, and the function to a subsystem through
data/subsystems.csv. They resolve its address to an item (a row of data/state_classes.csv, refined to
`Type::field` where the layout knows the type) and classify online. Nothing here decides a class: the
output is evidence for #836 to #839.

Windows. HarnessPhase marks the end of phases 0 to 7 of a tick; the work before marker n is phase n's, filed
under its trace name (net, units, projectiles, explosions, players, features, wind, meteors). Two windows
follow: `tail` is the rest of RunGameSteps's loop body after marker 7 (DrainSimEvents, the camera, particles,
the blink, ExpireEyeballs and the resource sharing), and `presentation` is what runs after HarnessAfterStep
until the next tick's HarnessBeforeStep: drawing, input and the main loop's own work. The scripted spawns,
orders, consoles and inputs of HarnessBeforeStep and the harness's own hashing are not recorded. The work
between HarnessBeforeStep and the first marker is phase 0 (HandleNetPackets) when the run has markers 0, else
phase 1: the test-mode scenarios call RunGameSteps with showStats 0, so there is no marker 0. The spike saw
phase 1 hold the previous tick's tail for that reason: with no marker 0, the drawing and input of the main loop
between two ticks fell into the window that ends at marker 1.

Evidence, per (writer, reader, item, phase), counted in the column `count`:

  read-after-write  the item was written in this window by another subsystem, or by the same subsystem on
                    another entity (an entity is an allocation instance and element index; `-` for globals)
  read-unwritten    the item was read in this window and nobody wrote it there (writer `-`)
  write-unread      the last write to the item in the window was not followed by a read of it before the
                    window ended (reader `-`); every read counts, also the writer's own
  ordered           consecutive draws from one random stream (item `rng.parkMiller` for RandomInt, `rng.crtHoldrand` and
                    `rng.crtLoad` for GameRand's main and load streams), or consecutive
                    emitted sound records (`sound`) or UI records (`ui`), by different subsystems in one
                    window; writer is the earlier, reader the later

`build/access/access/<scenario>.edges.csv` has the columns writer,reader,item,phase,evidence,count,
first_tick and is sorted by writer, reader, item, phase and evidence, so two runs of one scenario give one
file, except for the rows of the `presentation` window: how many frames the game draws between two ticks
follows the wall clock, so their counts (and the first tick of a rare one) change from run to run. `run
--twice` plays a scenario again and checks that every other row is equal.

`<scenario>.stats.txt` counts the accesses by attribution (unknown functions, ignored helpers) and by
resolution (unresolved items, accesses to code or constants that no row names). `--raw` writes every access
of the scenario to `<scenario>.raw.txt` (ACCESSLOG_RAW=1), for debugging only.

An access is attributed to the function that contains the callback's return address; a function missing from
data/subsystems.csv is `unknown`. The functions of src/state/block.cpp (the allocator, SimBlockBase and
SimOwns, which Ref<T>::get calls) are ignored by address, so a Ref<T> dereference counts for the function
that made it: the helper's own load of the block base is skipped and the access through the pointer is
instrumented in the caller.

`tables` writes accesslog.attr and accesslog.items next to the exe after the link, from TotalA.map,
data/subsystems.csv, data/state_classes.csv, data/globals.csv and the record layouts clang prints for
src/state/layout_asserts.cpp and src/state/sim_globals.h. `run` plays each scenario in test mode with
`--native-modules` (tools/playtest.py), collects the files and checks the trace against
reference/traces/<scenario>.txt; it fails if any trace differs.

Native modules only: the WASM modules' accesses to the shared memory are not instrumented. The method also
misses what the compiler did not instrument or the callbacks cannot see:

  * the CRT's string and sort functions (strcpy, strcmp, memcmp, qsort and the like) on state: they are not
    wrapped, so a state item that only they touch has no access;
  * lib/bitexact, zlib and the system DLLs, and the hand-written assembly;
  * optimiser effects: a read served from a register or folded into an earlier one is not seen, and the
    instrumented build is compiled with the game's /O2, so the sequence of accesses is the compiled
    code's and not the source's;
  * another thread's accesses: only the thread that runs the first window is recorded.
"""

import argparse
import bisect
import csv
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path

import blockpointers
import sources
import stateclasses
import tracedigest

ROOT = sources.ROOT
STATE_CLASSES = ROOT / "data/state_classes.csv"
SUBSYSTEMS = ROOT / "data/subsystems.csv"
GLOBALS = ROOT / "data/globals.csv"
LAYOUT_UNIT = ROOT / "src/state/layout_asserts.cpp"
SIM_GLOBALS_HEADER = ROOT / "src/state/sim_globals.h"
SIM_GLOBALS_CSV = ROOT / "src/state/sim_globals.csv"
ATTR_NAME = "accesslog.attr"
ITEMS_NAME = "accesslog.items"
ATTR_MAGIC = b"ALAT"
ITEMS_MAGIC = b"ALIT"
UNKNOWN = "unknown"
IGNORED = 0xFFFF
NONE = 0xFFFFFFFF
IGNORED_OBJECTS = ("block.cpp", "access_log.cpp")
PHASE_NAMES = ["net", "units", "projectiles", "explosions", "players", "features", "wind", "meteors",
               "tail", "presentation"]
SECTION_LINE = re.compile(r"^\s*([0-9a-f]{4}):([0-9a-f]{8}) ([0-9a-f]{8})H\s+(\S+)\s+(CODE|DATA)\s*$")
SYMBOL_LINE = re.compile(r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8,16})\s+(?:\S\s+)?(\S+)\s*$")
OBJECT_STEM = re.compile(r"^\d+_(.*?)\.obj$")
PRIMITIVES = {"char": 1, "signed char": 1, "unsigned char": 1, "bool": 1, "short": 2, "unsigned short": 2,
              "int": 4, "unsigned int": 4, "long": 4, "unsigned long": 4, "float": 4, "double": 8,
              "__int64": 8, "unsigned __int64": 8, "long long": 8, "unsigned long long": 8, "void": 1}
LAYOUT_FLAGS = ["--target=i686-pc-windows-msvc", "/nologo", "-fms-compatibility", "-fms-extensions",
                "-fms-compatibility-version=11.00", "-mno-sse", "/Gz", "-fno-strict-aliasing", "-fwrapv", "-w",
                "-fsyntax-only", "-Xclang", "-fdump-record-layouts-complete"]
HEAP_ROW = re.compile(r"^(?P<function>.+?): (?P<allocator>new\[\]|new|operator new|\w+)(?: (?P<what>.*?))?(?: #\d+)?$")
TAG_TYPES = {"UNIT MEMORY": "Unit", "WEAPON ARRAY": "Projectile"}
OUTPUT_FILES = ("access.edges.csv", "access.stats.txt")
RAW_FILE = "access.raw.txt"
REF_HELPER = re.compile(r"\$(?:Pod)?Ref@")


@dataclass
class MapInfo:
    """The sections and symbols of a linker map."""
    sections: dict[int, tuple[int, str, str]] = field(default_factory=dict)
    symbols: list[tuple[int, int, str, int, str]] = field(default_factory=list)

    def code(self) -> list[tuple[int, str, str]]:
        """(address, name, object) of every symbol in a code section."""
        return [(a, n, o) for s, _, n, a, o in self.symbols if self.sections.get(s, (0, "", ""))[2] == "CODE"]

    def data(self) -> list[tuple[int, str, str]]:
        """(address, name, object) of every symbol in a data section, by address."""
        return sorted((a, n, o) for s, _, n, a, o in self.symbols if self.sections.get(s, (0, "", ""))[2] == "DATA")


def parse_map(text: str) -> MapInfo:
    """The sections and the symbols of a linker map in /map format."""
    info = MapInfo()
    for line in text.splitlines():
        m = SECTION_LINE.match(line)
        if m:
            info.sections.setdefault(int(m.group(1), 16), (int(m.group(3), 16), m.group(4), m.group(5)))
            continue
        m = SYMBOL_LINE.match(line)
        if m:
            info.symbols.append((int(m.group(1), 16), int(m.group(2), 16), m.group(3), int(m.group(4), 16), m.group(5)))
    return info


def object_source(obj: str) -> str:
    """The source file name behind a linked object's name (`0121_selection.cpp.obj` is `selection.cpp`)."""
    m = OBJECT_STEM.match(obj)
    return m.group(1) if m else obj


def function_names(info: MapInfo) -> tuple[list[int], list[str], list[str]]:
    """The functions of the map by address: their start, their name by blockpointers.parse_map and their object."""
    lines = [" 0001:%08x       %s %016x     %s" % (0, n, a, ob) for a, n, ob in info.code()]
    code_map = blockpointers.parse_map("\n".join(lines))
    objects = {}
    for a, n, ob in info.code():
        objects.setdefault(a, ob)
    return code_map.addresses, code_map.names, [objects[a] for a in code_map.addresses]


class Subsystems(dict):
    """data/subsystems.csv: function -> [(source file name, subsystem)], and in `files` the subsystems of each file."""

    def __init__(self):
        super().__init__()
        self.files: dict[str, set[str]] = {}


def read_subsystems(path: Path = SUBSYSTEMS) -> Subsystems:
    """The functions and files of data/subsystems.csv."""
    out = Subsystems()
    with path.open() as fh:
        for r in csv.DictReader(fh):
            name = Path(r["file"]).name
            out.setdefault(r["function"], []).append((name, r["subsystem"]))
            out.files.setdefault(name, set()).add(r["subsystem"])
    return out


def subsystem_of(name: str, obj: str, rows: Subsystems) -> str:
    """The subsystem of a function: the row of its name and source file, else its only subsystem, else the only
    subsystem of its source file (a template instance, a constructor or a function the csv leaves out), else unknown."""
    source = object_source(obj)
    for candidate in dict.fromkeys([name, blockpointers.demangle(name), sources.base_name(name)]):
        found = rows.get(candidate)
        if not found:
            continue
        here = {s for f, s in found if f == source}
        if len(here) == 1:
            return next(iter(here))
        every = {s for _, s in found}
        if len(every) == 1:
            return next(iter(every))
    in_file = rows.files.get(source, ())
    return next(iter(in_file)) if len(in_file) == 1 else UNKNOWN


def attribution_table(info: MapInfo, rows: Subsystems) -> tuple[list[str], list[tuple[int, int, int]], dict]:
    """(subsystem names with unknown first, [(start, end, id)] sorted, counts by kind) for the functions of a map."""
    addresses, names, objects = function_names(info)
    code_end = max((a for s, o, _, a, _ in info.symbols if info.sections.get(s, (0, "", ""))[2] == "CODE"), default=0) + 0x1000
    subs = [UNKNOWN]
    ids = {UNKNOWN: 0}
    ranges: list[tuple[int, int, int]] = []
    counts = {"functions": 0, "unknown": 0, "ignored": 0}
    for i, address in enumerate(addresses):
        end = addresses[i + 1] if i + 1 < len(addresses) else code_end
        source = object_source(objects[i])
        counts["functions"] += 1
        if source in IGNORED_OBJECTS or REF_HELPER.search(names[i]):
            counts["ignored"] += 1
            sub = IGNORED
        else:
            name = subsystem_of(names[i], objects[i], rows)
            if name == UNKNOWN:
                counts["unknown"] += 1
            if name not in ids:
                ids[name] = len(subs)
                subs.append(name)
            sub = ids[name]
        if ranges and ranges[-1][1] == address and ranges[-1][2] == sub:
            ranges[-1] = (ranges[-1][0], end, sub)
        else:
            ranges.append((address, end, sub))
    return subs, ranges, counts


def pack_attr(subs: list[str], ranges: list[tuple[int, int, int]]) -> bytes:
    """The bytes of accesslog.attr."""
    out = [ATTR_MAGIC, struct.pack("<I", len(subs))]
    for s in subs:
        b = s.encode()
        out.append(struct.pack("<B", len(b)) + b)
    out.append(struct.pack("<I", len(ranges)))
    for start, end, sub in ranges:
        out.append(struct.pack("<IIHH", start, end, sub, 0))
    return b"".join(out)


def unpack_attr(data: bytes) -> tuple[list[str], list[tuple[int, int, int]]]:
    """The subsystem names and ranges in the bytes of accesslog.attr."""
    if data[:4] != ATTR_MAGIC:
        raise ValueError("not an accesslog.attr")
    (n,) = struct.unpack_from("<I", data, 4)
    pos = 8
    subs = []
    for _ in range(n):
        length = data[pos]
        subs.append(data[pos + 1:pos + 1 + length].decode())
        pos += 1 + length
    (m,) = struct.unpack_from("<I", data, pos)
    pos += 4
    ranges = [struct.unpack_from("<IIHH", data, pos + 12 * i)[:3] for i in range(m)]
    return subs, ranges


@dataclass
class Layouts:
    """The record layouts of one or more clang dumps."""
    text: str
    cache: dict = field(default_factory=dict)

    def record(self, name: str):
        """(size, fields) of a record, or None."""
        if name not in self.cache:
            self.cache[name] = stateclasses.layout_fields(self.text, name)
        return self.cache[name]

    def size_of(self, type_name: str) -> int | None:
        """The size of a type from the dump's own spelling, or None where the layout does not say."""
        base, dims = split_array(type_name)
        count = 1
        for d in dims:
            count *= d
        unit = self.unit_size(base)
        return None if unit is None else unit * count

    def unit_size(self, base: str) -> int | None:
        """The size of one element of a type with the array dimensions taken off."""
        base = re.sub(r"\b(const|volatile)\b", "", base)
        base = re.sub(r"^(struct|class|union|enum)\s+", "", base.strip()).strip()
        if base.endswith("*") or base.startswith("Ref<") or base.startswith("PodRef<"):
            return 4
        if base in PRIMITIVES:
            return PRIMITIVES[base]
        found = self.record(base) if re.fullmatch(r"\w+", base) else None
        return found[0] if found else None


def split_array(type_name: str) -> tuple[str, list[int]]:
    """A type split into its element spelling and array dimensions: `Player[10]` is (`Player`, [10])."""
    m = re.fullmatch(r"(.*?)((?:\[\d*\])*)", type_name.strip())
    dims = [int(d) if d else 1 for d in re.findall(r"\[(\d*)\]", m.group(2))]
    return m.group(1).strip(), dims


@dataclass
class TypeTable:
    """The record types the items table refines items with: size and the fields as (start, end, item name)."""
    layouts: Layouts
    index: dict[str, int] = field(default_factory=dict)
    entries: list[tuple[int, list[tuple[int, int, str]]]] = field(default_factory=list)

    def refine(self, type_name: str | None) -> tuple[int, int]:
        """(type id, stride) for the element type of a variable's or allocation's type, or (NONE, 0)."""
        if not type_name:
            return NONE, 0
        base, dims = split_array(type_name)
        base = re.sub(r"\b(const|volatile)\b", "", base)
        base = re.sub(r"^(struct|class|union|enum)\s+", "", base.strip()).strip()
        if not re.fullmatch(r"\w+", base):
            return NONE, 0
        found = self.layouts.record(base)
        if not found or not found[0]:
            return NONE, 0
        if base not in self.index:
            size, fields = found
            out: list[tuple[int, int, str]] = []
            seen: set[int] = set()
            starts = sorted({f.start for f in fields} | {size})
            for f in fields:
                if f.bits or not re.fullmatch(r"[A-Za-z_]\w*", f.name) or f.start in seen:
                    continue
                seen.add(f.start)
                natural = self.layouts.size_of(f.type)
                following = next((s for s in starts if s > f.start), size)
                end = min(size, f.start + natural) if natural else following
                if end > f.start:
                    out.append((f.start, end, "%s::%s" % (base, f.name)))
            self.index[base] = len(self.entries)
            self.entries.append((size, sorted(out)))
        return self.index[base], found[0]


def run_layout_dump(source: Path, build_dir: Path) -> str:
    """The record layouts clang-cl prints for one translation unit, with the build's include path."""
    include = (ROOT / "toolchain/msvc5-sp3/INCLUDE").resolve()
    argv = ["clang-cl"] + LAYOUT_FLAGS + ["-vfsoverlay", str(build_dir / "overlay.json"), "-I", str(ROOT / "include"),
                                           "-imsvc", str(include), "--", str(source)]
    run = subprocess.run(argv, capture_output=True, text=True, errors="replace", cwd=ROOT)
    if "sizeof=" not in run.stdout:
        raise SystemExit("accesslog: no record layouts from %s\n%s" % (source, run.stderr[-1500:]))
    return run.stdout


def layouts_of(build_dir: Path) -> Layouts:
    """The layouts of every type in layout_asserts.cpp, plus SimGlobals from its header."""
    text = run_layout_dump(LAYOUT_UNIT, build_dir)
    with tempfile.TemporaryDirectory() as tmp:
        probe = Path(tmp) / "probe.cpp"
        probe.write_text('#include "%s"\nstatic_assert(sizeof(SimGlobals) > 0, "");\n' % SIM_GLOBALS_HEADER.as_posix())
        text += "\n" + run_layout_dump(probe, build_dir)
    return Layouts(text)


SCOPE = r"(?:\?A0x[0-9A-Fa-f]+|[^@?]+)"


def symbol_keys(name: str) -> list[tuple[str, str]]:
    """The keys a map symbol answers to when a row of state_classes.csv looks for it: its whole name, the
    variable's name, the qualified name `Owner::name`, and for a function's static `name@Function`; a
    namespace the compiler numbers (`?A0x...`) is left out."""
    keys = [("name", name)]
    if name.startswith("_") and not name.startswith("__"):
        keys.append(("name", name[1:]))
    m = re.match(r"\?(%s)@((?:%s@)*)@[0-9]" % (r"[^@?]+", SCOPE), name)
    if m:
        scopes = [s for s in m.group(2).split("@") if s and not s.startswith("?A0x")]
        keys.append(("var", m.group(1)))
        if scopes:
            keys.append(("qualified", "::".join(reversed(scopes)) + "::" + m.group(1)))
    m = re.match(r"\?([^@?]+)@\?\d+\?\?([^@?]+)@((?:%s@)*)@" % SCOPE, name)
    if m:
        scopes = [s for s in m.group(3).split("@") if s and not s.startswith("?A0x")]
        keys.append(("local", "%s@%s" % (m.group(1), "::".join(list(reversed(scopes)) + [m.group(2)]))))
        keys.append(("localvar", m.group(1)))
    return keys


@dataclass
class DataSymbols:
    """The data symbols of a map, findable by what a state_classes row calls them."""
    by_key: dict[tuple[str, str], list[tuple[int, str, str]]] = field(default_factory=dict)
    addresses: list[int] = field(default_factory=list)

    @classmethod
    def of(cls, info: MapInfo) -> "DataSymbols":
        out = cls()
        data = info.data()
        out.addresses = [a for a, _, _ in data]
        for a, n, o in data:
            for key in symbol_keys(n):
                out.by_key.setdefault(key, []).append((a, n, o))
        return out

    def find(self, kind: str, name: str) -> tuple[int, str, str] | None:
        """The map symbol for a row, or None; a name that several objects define takes the object a path names."""
        path = None
        if kind == "static":
            if "@" in name and not name.startswith("?"):
                var, _, owner = name.partition("@")
                if "/" in owner:
                    path = Path(owner).name
                    options = self.by_key.get(("var", var), [])
                else:
                    options = self.by_key.get(("local", name), [])
            elif "::" in name:
                options = self.by_key.get(("qualified", name), [])
            else:
                options = self.by_key.get(("var", name), []) + self.by_key.get(("name", name), [])
        else:
            options = self.by_key.get(("var", name), []) + self.by_key.get(("name", name), [])
        if path is not None:
            narrowed = [o for o in options if object_source(o[2]) == path]
            options = narrowed or options
        return options[0] if options else None

    def end_of(self, address: int, limit: int) -> int:
        """The end of the symbol at address: the next symbol's address, or address + limit."""
        i = bisect.bisect_right(self.addresses, address)
        return min(self.addresses[i], address + limit) if i < len(self.addresses) else address + limit


@dataclass
class Items:
    """The accesslog.items tables: strings, types, static ranges, pointer-based regions and allocation sites."""
    strings: list[str] = field(default_factory=list)
    string_ids: dict[str, int] = field(default_factory=dict)
    types: TypeTable | None = None
    statics: list[tuple[int, int, int, int, int]] = field(default_factory=list)
    bases: list[tuple[int, int, list[tuple[int, int, int, int, int]]]] = field(default_factory=list)
    sites: list[tuple[int, int, int, int]] = field(default_factory=list)

    def string(self, text: str) -> int:
        """The index of an item name, added on first use."""
        if text not in self.string_ids:
            self.string_ids[text] = len(self.strings)
            self.strings.append(text)
        return self.string_ids[text]

    def pack(self) -> bytes:
        """The bytes of accesslog.items."""
        entries = self.types.entries if self.types else []
        typed = []
        for size, fields in entries:
            typed.append(struct.pack("<II", size, len(fields)) +
                         b"".join(struct.pack("<III", start, end, self.string(name)) for start, end, name in fields))
        out = [ITEMS_MAGIC, struct.pack("<I", len(self.strings))]
        for s in self.strings:
            b = s.encode()
            out.append(struct.pack("<H", len(b)) + b)
        out.append(struct.pack("<I", len(entries)))
        out.extend(typed)
        out.append(struct.pack("<I", len(self.statics)))
        for row in sorted(self.statics):
            out.append(struct.pack("<IIIII", *row))
        out.append(struct.pack("<I", len(self.bases)))
        for pointer, size, regions in self.bases:
            out.append(struct.pack("<III", pointer, size, len(regions)))
            for row in regions:
                out.append(struct.pack("<IIIII", *row))
        out.append(struct.pack("<I", len(self.sites)))
        for row in sorted(self.sites):
            out.append(struct.pack("<IIII", *row))
        return b"".join(out)


def read_csv(path: Path) -> list[dict]:
    """The rows of a csv file."""
    with path.open(newline="") as fh:
        return list(csv.DictReader(fh))


def paint_order(regions: list[tuple[int, int, int, int, int]]) -> list[tuple[int, int, int, int, int]]:
    """Regions of one base by decreasing extent, so that painting them in order leaves the smallest on top."""
    return sorted(regions, key=lambda r: (-(r[1] - r[0]), r[0]))


def base_regions(rows: list[dict], fields: list, size: int, items: Items, kind: str,
                 inside: dict[str, tuple[str, int]] | None = None) -> list[tuple[int, int, int, int, int]]:
    """The regions of Game (rows of kind game-region) or SimGlobals (rows of kind global, by field name, or
    `inside`: a global that src/state/sim_globals.csv puts at an offset into another field)."""
    by_name = {f.name: f for f in fields}
    inside = inside or {}
    starts = sorted({f.start for f in fields} | {size})
    regions = []
    seen: set[int] = set()
    for row in rows:
        if kind == "game-region":
            first = row["name"].split("|")[0]
            f = by_name.get(first)
            if f is None or f.start in seen:
                continue
            seen.add(f.start)
        else:
            f = by_name.get(row["name"])
            if f is None and row["name"] in inside and inside[row["name"]][0] in by_name:
                container, offset = inside[row["name"]]
                outer = by_name[container]
                end = min(size, next((s for s in starts if s > outer.start), size))
                regions.append((outer.start + offset, end, items.string(row["name"]), NONE, 0))
                continue
            if f is None:
                continue
        natural = items.types.layouts.size_of(f.type)
        following = next((s for s in starts if s > f.start), size)
        end = max(following, f.start + natural) if natural and kind == "game-region" else (
            f.start + natural if natural else following)
        end = min(end, size)
        tid, stride = items.types.refine(f.type)
        regions.append((f.start, end, items.string(row["name"]), tid, stride))
    return paint_order(regions)


def source_order(rows: list[tuple[dict, re.Match]]) -> list[tuple[dict, re.Match]]:
    """The heap rows of one function in the order their allocation first appears in its source (the order of the
    compiled call sites), else in the csv's order."""
    if len(rows) < 2:
        return rows
    path = sources.find_source(int(rows[0][0]["address"], 16)) if rows[0][0]["address"] else None
    text = path.read_text(errors="replace") if path else ""
    marker = text.find("FUNCTION: %s" % rows[0][0]["address"])

    def position(item: tuple[dict, re.Match]) -> int:
        what = item[1].group("what") or ""
        for needle in ('"%s"' % what, what):
            at = text.find(needle, max(marker, 0)) if what else -1
            if at >= 0:
                return at
        return 1 << 30

    return sorted(rows, key=position)


def build_items(info: MapInfo, map_path: Path, layouts: Layouts, rows: list[dict], globals_rows: list[dict]) -> tuple[Items, dict]:
    """The items table of a build and what it could not place."""
    items = Items(types=TypeTable(layouts))
    stats = {"static": 0, "static_missing": 0, "heap_sites": 0, "heap_rows": 0, "heap_ambiguous": 0}
    symbols = DataSymbols.of(info)
    by_name = {g["name"]: g for g in globals_rows}
    dump_globals = {}
    inside = {r["name"]: (r["container"], int(r["offset"], 16)) for r in read_csv(SIM_GLOBALS_CSV) if r["container"]}
    for row in rows:
        if row["kind"] not in ("global", "static"):
            continue
        sym = symbols.find(row["kind"], row["name"])
        if sym is None:
            stats["static_missing"] += 1
            continue
        g = by_name.get(row["name"]) if row["kind"] == "global" else None
        size = int(g["size"]) if g and g["size"].isdigit() else 0
        end = sym[0] + size if size else symbols.end_of(sym[0], 1 << 16)
        tid, stride = items.types.refine(g["type"] if g else None)
        items.statics.append((sym[0], end, items.string(row["name"]), tid, stride))
        dump_globals[row["name"]] = sym[0]
        stats["static"] += 1
    for pointer_name, record, kind in (("g_game", "Game", "game-region"), ("g_simGlobals", "SimGlobals", "global")):
        sym = symbols.find("global", pointer_name)
        found = layouts.record(record)
        if sym is None or found is None:
            raise SystemExit("accesslog: no %s in the map or no layout of %s" % (pointer_name, record))
        wanted = [r for r in rows if r["kind"] == kind and (kind == "game-region" or r["name"] not in dump_globals)]
        items.bases.append((sym[0], found[0], base_regions(wanted, found[1], found[0], items, kind, inside)))
    code_map = blockpointers.load_code_map(map_path)
    namer = blockpointers.Namer(code_map, blockpointers.load_call_sites(map_path))
    heap: dict[str, list[tuple[dict, re.Match]]] = {}
    for row in rows:
        if row["kind"] == "heap":
            m = HEAP_ROW.match(row["name"])
            if m:
                heap.setdefault(m.group("function"), []).append((row, m))
    heap = {function: source_order(found) for function, found in heap.items()}
    stats["heap_rows"] = sum(len(v) for v in heap.values())
    for ra in sorted(namer.ranks):
        rank, count = namer.ranks[ra]
        function = code_map.function(ra)
        candidates = heap.get(function) or heap.get(sources.base_name(function)) or heap.get(blockpointers.demangle(function))
        if not candidates:
            continue
        if len(candidates) != count and len(candidates) > 1:
            stats["heap_ambiguous"] += 1
        row, m = candidates[min(rank, len(candidates)) - 1]
        what = TAG_TYPES.get(m.group("what") or "", m.group("what") or "")
        tid, stride = items.types.refine(what if re.fullmatch(r"\w+", what) else None)
        items.sites.append((ra, items.string(row["name"]), tid, stride))
        stats["heap_sites"] += 1
    return items, stats


def tables(build_dir: Path) -> None:
    """Write accesslog.attr and accesslog.items beside the exe of build_dir."""
    map_path = build_dir / "TotalA.map"
    if not map_path.is_file():
        raise SystemExit("accesslog: no %s (link the build first)" % map_path)
    info = parse_map(map_path.read_text(errors="replace"))
    subs, ranges, counts = attribution_table(info, read_subsystems())
    (build_dir / ATTR_NAME).write_bytes(pack_attr(subs, ranges))
    items, stats = build_items(info, map_path, layouts_of(build_dir), read_csv(STATE_CLASSES), read_csv(GLOBALS))
    (build_dir / ITEMS_NAME).write_bytes(items.pack())
    print("accesslog: %(functions)d functions, %(unknown)d unknown, %(ignored)d ignored" % counts, file=sys.stderr)
    print("accesslog: %(static)d statics placed, %(static_missing)d rows with no symbol, %(heap_sites)d allocation sites "
          "for %(heap_rows)d heap rows (%(heap_ambiguous)d ambiguous)" % stats, file=sys.stderr)


def trace_difference(trace: Path, reference: Path) -> str | None:
    """Why a run's trace differs from the stored digests of reference/traces, or None when every tick is equal."""
    _, rows = tracedigest.parse_trace(trace)
    have = tracedigest.digests(rows)
    _, want = tracedigest.parse_digest(reference)
    if have == want:
        return None
    for (tick, digest), (wanted_tick, wanted) in zip(have, want):
        if (tick, digest) != (wanted_tick, wanted):
            return "first difference at tick %d (%s, stored %s)" % (tick, digest, wanted)
    return "%d ticks, stored %d" % (len(have), len(want))


def play(exe: Path, scenario: Path, game_dir: Path, tables_dir: Path | None, raw: bool, timeout: float) -> tuple[Path, float]:
    """Play a scenario in test mode with native modules; the run's directory under build/playtest and the seconds."""
    argv = [sys.executable, str(ROOT / "tools/playtest.py"), "--exe", str(exe), "--game-dir", str(game_dir),
            "--testmode", str(scenario), "--native-modules", "--testmode-timeout", str(timeout)]
    if tables_dir is not None:
        for name in (ATTR_NAME, ITEMS_NAME):
            argv += ["--put", name, str(tables_dir / name)]
        for name in OUTPUT_FILES + (RAW_FILE,) * raw:
            argv += ["--collect", name]
    env = dict(os.environ)
    if raw:
        env["ACCESSLOG_RAW"] = "1"
    run = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True, env=env)
    session = re.search(r"logs and screenshots: (\S+)", run.stdout)
    if run.returncode != 0 or not session:
        sys.stderr.write(run.stdout[-3000:] + run.stderr[-2000:])
        raise SystemExit("accesslog: %s did not run to the end" % scenario.stem)
    directories = [d for d in sorted((ROOT / session.group(1)).iterdir()) if (d / "trace.txt").is_file()]
    if not directories:
        raise SystemExit("accesslog: %s left no trace.txt" % scenario.stem)
    return directories[0], float(json.loads((directories[0] / "result.json").read_text())["seconds"])


def steady(edges: Path) -> list[str]:
    """The lines of an edges file that do not depend on how many frames the run drew between ticks."""
    return [line for line in edges.read_text().splitlines() if ",presentation," not in line]


def summarise(name: str, edges: Path, stats: Path, seconds: float, baseline: float | None) -> list[str]:
    """The report lines for one scenario: rows, shares of unknown and unresolved, wall time."""
    with edges.open(newline="") as fh:
        rows = list(csv.DictReader(fh))
    counts = {}
    for line in stats.read_text().splitlines():
        key, _, value = line.partition(" ")
        counts[key] = int(value)
    items = {r["item"] for r in rows}
    unresolved = {i for i in items if i.startswith("unresolved:")}
    accesses = max(counts["accesses"], 1)
    lines = ["%s: %d edge rows, %d items (%d unresolved:*), %d accesses" % (name, len(rows), len(items), len(unresolved), counts["accesses"]),
             "  accesses by unknown subsystem: %.4f%% (%d), unresolved: image %.2f%% block %.2f%%, ignored helper accesses %d, "
             "unnamed constants %d, outside the state %d" % (
                 100.0 * counts["unknown_subsystem"] / accesses, counts["unknown_subsystem"],
                 100.0 * counts["unresolved_image"] / accesses, 100.0 * counts["unresolved_block"] / accesses,
                 counts["ignored_helpers"], counts["constants_unnamed"], counts["other_memory"])]
    if baseline:
        lines.append("  wall time %.1f s against %.1f s uninstrumented (x%.1f)" % (seconds, baseline, seconds / baseline))
    else:
        lines.append("  wall time %.1f s" % seconds)
    return lines


def run(build_dir: Path, names: list[str], game_dir: Path, raw: bool, twice: bool, baseline: Path | None, timeout: float) -> int:
    """Play each scenario instrumented, keep its edges, and check its trace against reference/traces."""
    out = build_dir / "access"
    out.mkdir(exist_ok=True)
    failed = 0
    for name in names:
        scenario = ROOT / "reference/scenarios" / (name + ".txt")
        stored = ROOT / "reference/traces" / (name + ".txt")
        if not scenario.is_file() or not stored.is_file():
            raise SystemExit("accesslog: no scenario or stored trace named %s" % name)
        directory, seconds = play(build_dir / "TotalA.exe", scenario, game_dir, build_dir, raw, timeout)
        why = trace_difference(directory / "trace.txt", stored)
        print("%s: %s" % (name, "trace EQUAL to reference/traces/%s.txt" % name if why is None else "trace DIFFERS, " + why))
        failed += why is not None
        for suffix, source in (("edges.csv", "access.edges.csv"), ("stats.txt", "access.stats.txt"), ("raw.txt", RAW_FILE)):
            if (directory / source).is_file():
                shutil.copyfile(directory / source, out / ("%s.%s" % (name, suffix)))
        if twice:
            again, _ = play(build_dir / "TotalA.exe", scenario, game_dir, build_dir, False, timeout)
            same = steady(again / "access.edges.csv") == steady(out / (name + ".edges.csv"))
            print("%s: two runs give %s edges file, presentation rows left out" % (name, "the same" if same else "DIFFERENT"))
            failed += not same
            if (again / "access.edges.csv").read_bytes() != (out / (name + ".edges.csv")).read_bytes():
                print("%s: the presentation rows differ between the runs (frames drawn per tick follow the wall clock)" % name)
        base = None
        if baseline is not None:
            base_dir, base = play(baseline, scenario, game_dir, None, False, timeout)
        for line in summarise(name, out / (name + ".edges.csv"), out / (name + ".stats.txt"), seconds, base):
            print(line)
    return 1 if failed else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)
    t = sub.add_parser("tables", help="write the attribution and item tables after the link")
    t.add_argument("--build-dir", type=Path, required=True)
    r = sub.add_parser("run", help="play scenarios instrumented and keep their edge summaries")
    r.add_argument("--build-dir", type=Path, required=True)
    r.add_argument("scenarios", nargs="+", metavar="SCENARIO", help="a name in reference/scenarios")
    r.add_argument("--game-dir", type=Path, default=Path("~/.ta").expanduser())
    r.add_argument("--raw", action="store_true", help="also keep the full access log (debugging only; large)")
    r.add_argument("--twice", action="store_true", help="play each scenario twice and check the two edge files are equal")
    r.add_argument("--baseline", type=Path, metavar="EXE", help="also play with this uninstrumented exe, for the wall time")
    r.add_argument("--timeout", type=float, default=1800)
    args = ap.parse_args()
    if args.command == "tables":
        tables(args.build_dir.resolve())
        return 0
    return run(args.build_dir.resolve(), args.scenarios, args.game_dir, args.raw, args.twice, args.baseline, args.timeout)


if __name__ == "__main__":
    sys.exit(main())
