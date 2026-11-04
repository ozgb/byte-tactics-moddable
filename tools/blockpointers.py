"""List every pointer from the shared state block to memory outside it, found at run time, and class each one.

    uv run tools/blockpointers.py                    # play the suite with -blockscan, write data/block_pointers.csv
    uv run tools/blockpointers.py --from DIR...      # build from blockptrs files already collected (no Wine)
    uv run tools/blockpointers.py --save-scans DIR   # also write each played scenario's scan to DIR/<scenario>.txt
    uv run tools/blockpointers.py --scenario N...    # play only these (needs --save-scans), to merge later with --from
    uv run tools/blockpointers.py --from DIR... --prune   # also drop the wildcard rows whose holder is no site of the exe
    uv run tools/blockpointers.py --out F            # write F instead (a dry run; class and note kept from the data file)
    uv run tools/blockpointers.py --check            # CI: the file itself is valid (classes, notes, order)
    uv run tools/blockpointers.py --unclassified     # list the rows with no class
    uv run tools/test_blockpointers.py               # tests, on hand-written scans, maps and rows

This is the list for #285. The decompilation's types do not mark every pointer, so the game finds
them: run with `-blockscan` (tools/playtest.py --blockscan), a test-mode run walks every live
allocation of the block and the `SimGlobals` struct at the end of each tick and writes blockptrs.txt
(reference/README.md, "Block pointers") with each 4-byte word, read at every byte offset (the packed
structs keep pointers at odd offsets), that points into a live heap allocation outside the block.
This tool plays the whole scenario suite (the scenarios of reference/scenarios/, as tools/reference.py
does, quirks off) with the scan on every tick (a scenario that fails is tried once more; the scans
that finished are saved with --save-scans even when another scenario fails, and the data file is then
left alone), or takes the files of an earlier run (`--from DIR...`,
files named `<scenario>.txt`, which `reference.py --check --save-blockptrs DIR` writes; the gate scans
every 8th tick and every tick of the last 64, so the list is best made by playing the suite), resolves
the allocation sites with the linker map and merges the results. It accepts version 2 files only.

Names. A site is a return address in the exe. The map (build/modern/TotalA.map) gives the function
that contains it, and the tag the caller passed adds `: <tag>`. A function with one allocator call site
is named by that (`AddOrder`); with several, each site takes its rank among them in address order
(`VtolFollowOrder #2`). The call sites are those in the exe's code (the exe beside the map is read, and
the map names the allocators), not those a run saw, so a name is the same in every run and changes only
when the function's allocation calls are added, removed or reordered. A site the exe has no call for is
named by its offset in the function (`@0x1a3`). The function is named by its plain name
(`Class::method`), except a member of a class template and a plain name that two functions in the map
share, which take their decorated name (`?insert@?$vector@...`), so that a row moves only when its own
function does. The objects listed in objs/objects.rsp beside the map say which functions are empty
(see parse_map); the tool stops when that file is missing. Address 0 is `unknown`. The holder `SimGlobals`
is named `SimGlobals.<field>`, the field of src/state/sim_globals.h that holds the word (from the
record layout clang prints for a probe that includes the header), and the offset is then inside that
field; with no layout the holder is `SimGlobals` and the offset is the struct's.

Rows, one per (holder, target), sorted by holder, then target (a `*` target first):

  holder, target  the sites named above
  offsets         where in the holder the pointers sit. The offsets of one pair that form an
                  arithmetic progression of 3 or more (a field in each element of an array) are
                  `start+stride*k` (k from 0, unbounded: more elements in another run still match);
                  the others are listed (`0x10 0x24`)
  scenarios       in how many scenarios the pair was seen
  ticks           the ticks it was seen in, summed over the scenarios
  target_offsets  `0x0` when the word always pointed at the start of the target, else `0xa-0x40`
  class           outside (load-time data that stays outside the block), moves-in (sim state that
                  should be allocated in the block) or not-a-pointer (an integer that looks like an
                  address; it shows in few runs or at varying target offsets, and needs a note)
  note            free text

`class` and `note` are kept by regeneration, by (holder, target), else by holder and a shared offset when
a target was renamed. Addresses outside the block differ from run to run, so the pair rows can never be
complete: they are the record, and `reference.py --check` plays the scan on and fails only on a word
whose holder has no wildcard row.

Wildcard rows. A holder has one row with `*` as its offsets and `*` as its target for each class of
its pair rows, and one for not-a-pointer (the words that only look like addresses). `*` is no other
row's offsets or target, and no other column takes it. A wildcard row has scenarios `0`, ticks `0` and
target_offsets `-`, and a note; regeneration keeps it as written and builds the pair rows as before.
Wildcard rows are keyed by (holder, class) where pair rows are keyed by (holder, target), and sort
with the rest: holder, then wildcard rows first, by class. `--check` fails on a holder with pair rows
that lacks a wildcard row for one of their classes or for not-a-pointer.
The holders that are member functions of a `BlockVector<T>` need no wildcard row (`is_data_holder`): a heap
address that shows in their buffers is a coincidence with the element bytes, which no run can rule out.
"""

import argparse
import bisect
import csv
import difflib
import io
import re
import struct
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "data/block_pointers.csv"
DEFAULT_EXE = ROOT / "build/modern/TotalA.exe"
COLUMNS = ["holder", "offsets", "target", "scenarios", "ticks", "target_offsets", "class", "note"]
KEPT = ("class", "note")
CLASSES = ("outside", "moves-in", "not-a-pointer")
SCAN_MAGIC = "bt-blockptrs 2"
SCAN_COLUMNS = ["holder", "holder_tag", "offset", "target", "target_tag", "ticks", "target_offset_min",
                "target_offset_max"]
MIN_RUN = 3
SIM_GLOBALS = "SimGlobals"
SIM_GLOBALS_HEADER = "src/state/sim_globals.h"
WILD = "*"
HEX = r"0x[0-9a-f]+"
OFFSET_TOKEN = re.compile(r"^(%s)(?:\+(%s)\*k)?$" % (HEX, HEX))
TARGET_OFFSETS = re.compile(r"^%s(?:-%s)?$" % (HEX, HEX))
csv.field_size_limit(1 << 28)
LAYOUT_FLAG = ["-Xclang", "-fdump-record-layouts-complete"]
DATA_HOLDER = re.compile(r"\?(?:[A-Za-z_]\w*@|\?[0-9A-Z])\?\$BlockVector@")


@dataclass
class ScanRow:
    """One line of a blockptrs.txt: a word of a holder allocation that points into a target allocation."""
    scenario: str
    holder: str
    holder_tag: str
    offset: int
    target: str
    target_tag: str
    ticks: int
    target_min: int
    target_max: int


@dataclass
class Scan:
    """A parsed blockptrs.txt: the commit, sampling and scanned tick count of the run and its rows."""
    scenario: str
    commit: str
    ticks: int
    every: int = 1
    rows: list[ScanRow] = field(default_factory=list)


def parse_scan(text: str, scenario: str) -> Scan:
    """The rows of a blockptrs.txt; raises ValueError on a file that does not follow the format."""
    lines = text.splitlines()
    if not lines or lines[0].strip() != SCAN_MAGIC:
        raise ValueError("%s: not a %r file" % (scenario, SCAN_MAGIC))
    header: dict[str, str] = {}
    rows: list[ScanRow] = []
    for number, line in enumerate(lines[1:], 2):
        if not line.strip():
            continue
        cells = line.split("\t")
        if cells[0] in ("commit", "every", "ticks") and len(cells) == 2:
            header[cells[0]] = cells[1]
            continue
        if cells == SCAN_COLUMNS:
            continue
        if len(cells) != len(SCAN_COLUMNS):
            raise ValueError("%s:%d: %d columns, expected %d" % (scenario, number, len(cells), len(SCAN_COLUMNS)))
        try:
            rows.append(ScanRow(scenario, cells[0], cells[1], int(cells[2], 16), cells[3], cells[4],
                                int(cells[5]), int(cells[6], 16), int(cells[7], 16)))
        except ValueError:
            raise ValueError("%s:%d: a number does not parse: %r" % (scenario, number, line)) from None
    if "ticks" not in header:
        raise ValueError("%s: no ticks line" % scenario)
    if "every" not in header:
        raise ValueError("%s: no every line" % scenario)
    try:
        ticks, every = int(header["ticks"]), int(header["every"])
    except ValueError:
        raise ValueError("%s: ticks or every is not a number" % scenario) from None
    if every < 1:
        raise ValueError("%s: every is %d, expected 1 or more" % (scenario, every))
    return Scan(scenario, header.get("commit", "-"), ticks, every, rows)


def read_scans(paths: list[Path]) -> list[Scan]:
    """The scans of the files, and of the *.txt files of the directories, each named by its file's stem."""
    files: list[Path] = []
    for path in paths:
        files += sorted(path.glob("*.txt")) if path.is_dir() else [path]
    if not files:
        raise ValueError("no blockptrs files in %s" % ", ".join(str(p) for p in paths))
    return [parse_scan(p.read_text(), p.stem) for p in files]


@dataclass
class CodeMap:
    """The functions of a linker map by address, for naming the function that holds a return address."""
    addresses: list[int]
    names: list[str]

    def function(self, address: int) -> str:
        """The function that contains the instruction before the return address, or its hex address."""
        i = bisect.bisect_right(self.addresses, address - 1) - 1
        return self.names[i] if i >= 0 else "0x%08x" % address

    def start(self, address: int) -> int | None:
        """The address of the function that contains the instruction before the return address, or None."""
        i = bisect.bisect_right(self.addresses, address - 1) - 1
        return self.addresses[i] if i >= 0 else None


MAP_LINE = re.compile(r"^\s*0001:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8,16})\s")
SKIPPED = ("$", "?catch$", "___ehhandler$")


def demangle(name: str) -> str:
    """The plain name of a decorated one: `Class::method`, `function`; other names are left alone."""
    if name.startswith("?"):
        m = re.match(r"\?([^@?]+)@(?:\?\$([^@]+)@|([^@?]+)@)?", name)
        if m:
            owner = m.group(2) or m.group(3)
            return "%s::%s" % (owner, m.group(1)) if owner else m.group(1)
        return name
    return re.sub(r"@\d+$", "", name[1:] if name.startswith("_") else name)


def is_instantiation(name: str) -> bool:
    """Whether a decorated name is a member of a class template (`?insert@?$vector@...`)."""
    return re.match(r"\?[^@?]+@\?\$", name) is not None


def objects_listed(objs_dir: Path) -> list[Path]:
    """The objects the link took, from objs/objects.rsp (one path per line, quoted or not)."""
    rsp = objs_dir / "objects.rsp"
    if not rsp.is_file():
        raise ValueError("no objs/objects.rsp beside the map (%s): the block pointer holders need the linked objects"
                         % rsp.parent.parent)
    paths = []
    for line in rsp.read_text().splitlines():
        line = line.strip().strip('"')
        if line:
            path = Path(line)
            paths.append(path if path.is_absolute() else objs_dir / path)
    return paths


def empty_functions(objs_dir: Path) -> set[str]:
    """The decorated names of functions whose code the link left out (empty sections in the objects listed in
    objs/objects.rsp). The linker puts such a name at the next live function's address."""
    import objread
    dead: set[str] = set()
    for path in objects_listed(objs_dir):
        obj = objread.parse(path)
        dead.update(name for name, sym in obj.externals.items()
                    if sym.is_func and sym.section > 0 and not obj.secs[sym.section - 1].data)
    return dead


def parse_map(text: str, dead: set[str] | None = None) -> CodeMap:
    """The code symbols of a linker map. Of several names at one address the first by (FUN_ last, name) wins,
    leaving out the names in `dead` (functions with no code) unless every name there is dead.

    A function named by its plain name keeps that name only while no other function in the map has it; a
    member of a class template, or a plain name that two functions share, is named by its decorated name,
    which belongs to that function alone.
    """
    by_address: dict[int, list[str]] = {}
    for line in text.splitlines():
        m = MAP_LINE.match(line)
        if not m or m.group(1).startswith(SKIPPED):
            continue
        by_address.setdefault(int(m.group(2), 16), []).append(m.group(1))
    addresses = sorted(by_address)
    chosen = []
    for a in addresses:
        live = [n for n in by_address[a] if not dead or n not in dead] or by_address[a]
        chosen.append(min(live, key=lambda n: (demangle(n).startswith("FUN_"), demangle(n))))
    shared: dict[str, int] = {}
    for n in chosen:
        shared[demangle(n)] = shared.get(demangle(n), 0) + 1
    names = [n if is_instantiation(n) or shared[demangle(n)] > 1 else demangle(n) for n in chosen]
    return CodeMap(addresses, names)


def load_code_map(map_path: Path) -> CodeMap:
    """The map's code symbols, named by live code from the objects listed beside the map (objs/objects.rsp)."""
    return parse_map(map_path.read_text(errors="replace"), empty_functions(map_path.parent / "objs"))


ALLOCATORS = re.compile(r"\?(SimAllocTagged|GameAlloc|GameCalloc|GameRealloc|GameStrdup|SimAlloc|SimRealloc)\w*@@")
NEW_OPERATORS = ("??2", "??_U")
CODE_SECTION = 0x20000000


def allocator_addresses(map_text: str) -> set[int]:
    """The addresses of the allocation functions (and operator new) in a linker map."""
    found = set()
    for line in map_text.splitlines():
        m = MAP_LINE.match(line)
        if m and (ALLOCATORS.match(m.group(1)) or m.group(1).startswith(NEW_OPERATORS)):
            found.add(int(m.group(2), 16))
    return found


def call_sites(code: bytes, address: int, callees: set[int]) -> set[int]:
    """The return addresses of the direct calls to `callees` in `code`, which is loaded at `address`."""
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    sites: set[int] = set()
    pos = 0
    while pos < len(code):
        for at, size, mnemonic, operand in disassembler.disasm_lite(code[pos:], address + pos):
            pos = at + size - address
            if mnemonic == "call" and operand.startswith("0x") and int(operand, 16) in callees:
                sites.add(at + size)
        pos += 1
    return sites


def exe_call_sites(exe: Path, callees: set[int]) -> list[int]:
    """The return addresses of the calls to `callees` in the code sections of a PE exe, in address order."""
    data = exe.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count, optional_size = struct.unpack_from("<H", data, pe + 6)[0], struct.unpack_from("<H", data, pe + 20)[0]
    image_base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    sites: set[int] = set()
    for i in range(count):
        _, size, rva, raw_size, raw, *_, flags = struct.unpack_from("<8sIIIIIIHHI", data, pe + 24 + optional_size + 40 * i)
        if flags & CODE_SECTION:
            sites |= call_sites(data[raw:raw + min(size, raw_size)], image_base + rva, callees)
    return sorted(sites)


def load_call_sites(map_path: Path) -> list[int]:
    """The allocator call sites of the exe beside a linker map, named by the map."""
    exe = map_path.with_suffix(".exe")
    if not exe.is_file():
        raise ValueError("no exe beside the map (%s): the allocation sites are the calls in its code" % exe)
    return exe_call_sites(exe, allocator_addresses(map_path.read_text(errors="replace")))


class Namer:
    """Names allocation sites: function (and tag), then ` #k` when the function has several allocator call sites.

    `sites` are the exe's allocator call sites (return addresses); k is a site's rank among those of its function
    in address order. A site not among them is named by `@0x<offset>`, its offset in its function.
    """

    def __init__(self, code: CodeMap, sites: list[int]):
        self.code = code
        self.ranks: dict[int, tuple[int, int]] = {}
        by_function: dict[int | None, list[int]] = {}
        for site in sorted(sites):
            by_function.setdefault(code.start(site), []).append(site)
        for group in by_function.values():
            for k, site in enumerate(group, 1):
                self.ranks[site] = (k, len(group))

    def name(self, site: str, tag: str) -> str:
        """The name of a site given as `0x%08x` (or SimGlobals, which has no tag)."""
        if site == SIM_GLOBALS:
            return SIM_GLOBALS
        address = int(site, 16)
        if address == 0:
            return "unknown"
        text = self.code.function(address)
        if tag != "-":
            text += ": " + tag
        if address in self.ranks:
            rank, of = self.ranks[address]
            return text if of == 1 else "%s #%d" % (text, rank)
        start = self.code.start(address)
        return text if start is None else text + " @0x%x" % (address - start)


def is_site_holder(namer: Namer, holder: str) -> bool:
    """Whether a holder name is one the exe can give: `unknown`, a SimGlobals holder, or a function with the
    allocator call site its ` #k` rank names (`fn`, `fn: tag` or `fn #k`, `fn: tag #k`)."""
    if holder == "unknown" or holder.split(".", 1)[0].split("+", 1)[0] == SIM_GLOBALS:
        return True
    m = re.fullmatch(r"(.*?)(?: #(\d+))?", holder)
    function = m.group(1).split(": ", 1)[0]
    rank = int(m.group(2)) if m.group(2) else None
    counts = {namer.code.function(site): of for site, (_, of) in namer.ranks.items()}
    if function not in counts:
        return False
    return rank is None and counts[function] == 1 or rank is not None and 1 <= rank <= counts[function] > 1


def prune_wild(old: list[dict], namer: Namer) -> list[dict]:
    """The old rows without the wildcard rows whose holder is no site of the exe."""
    return [r for r in old if not is_wild(r) or is_site_holder(namer, r["holder"])]


def load_namer(map_path: Path) -> Namer:
    """The namer for the exe beside a linker map: the map's functions and the exe's allocator call sites."""
    return Namer(load_code_map(map_path), load_call_sites(map_path))


@dataclass
class Member:
    """A top-level member of SimGlobals and its byte range."""
    name: str
    start: int
    end: int


def simglobals_members(layout_text: str) -> list[Member] | None:
    """The members of SimGlobals in a clang record-layout dump, or None when it has none."""
    import stateclasses
    found = stateclasses.layout_fields(layout_text, SIM_GLOBALS)
    if not found:
        return None
    return [Member(f.name, f.start, f.end) for f in found[1] if not f.bits]


def load_members(build_dir: Path) -> list[Member] | None:
    """The members of SimGlobals from clang-cl on a probe that includes its header, or None when that fails."""
    import rwsets
    try:
        _, argv, directory = rwsets.compile_commands(build_dir, [])[0]
        i = argv.index("--")
        dump = argv.index("-ast-dump=json")
        flags = argv[:dump - 1] + argv[dump + 1:i]
        with tempfile.TemporaryDirectory() as tmp:
            probe = Path(tmp) / "probe.cpp"
            probe.write_text('#include "%s"\nstatic_assert(sizeof(SimGlobals) > 0, "");\n'
                             % (ROOT / SIM_GLOBALS_HEADER))
            run = subprocess.run(flags + LAYOUT_FLAG + ["--", str(probe)], cwd=directory, capture_output=True,
                                 text=True, errors="replace")
        return simglobals_members(run.stdout)
    except (OSError, IndexError, subprocess.SubprocessError, ValueError) as e:
        sys.stderr.write("blockpointers: no SimGlobals layout (%s); its holders are named SimGlobals+0x<off>\n" % e)
        return None


def holder_name(row: ScanRow, namer: Namer, members: list[Member] | None) -> tuple[str, int]:
    """The holder's name and the offset inside it; SimGlobals words are named by field with a field-relative offset."""
    if row.holder != SIM_GLOBALS:
        return namer.name(row.holder, row.holder_tag), row.offset
    for m in members or []:
        if m.start <= row.offset < max(m.end, m.start + 1):
            return "%s.%s" % (SIM_GLOBALS, m.name), row.offset - m.start
    return SIM_GLOBALS + ("+0x%x" % row.offset if members is None else ""), (0 if members is None else row.offset)


@dataclass
class Resolved:
    """A scanned word with its holder and target named."""
    scenario: str
    holder: str
    offset: int
    target: str
    ticks: int
    target_min: int
    target_max: int


def resolve(scans: list[Scan], namer: Namer, members: list[Member] | None) -> list[Resolved]:
    """Every scanned row with names."""
    out = []
    for scan in scans:
        for r in scan.rows:
            holder, offset = holder_name(r, namer, members)
            if holder == SIM_GLOBALS + "+0x%x" % r.offset:
                offset = 0
            out.append(Resolved(scan.scenario, holder, offset, namer.name(r.target, r.target_tag), r.ticks,
                                r.target_min, r.target_max))
    return out


def fold(offsets: set[int]) -> list[tuple[int, int | None]]:
    """The offsets as (start, stride) runs: progressions of MIN_RUN or more, else single (start, None) entries.

    Greedy from the smallest remaining offset: the stride (smallest wins a tie) that reaches the most
    consecutive members of the set, taken as a run if it reaches MIN_RUN.
    """
    left = sorted(offsets)
    out: list[tuple[int, int | None]] = []
    while left:
        start = left[0]
        members = set(left)
        best_stride, best_length = 0, 1
        for other in left[1:]:
            stride = other - start
            length = 1
            while start + stride * length in members:
                length += 1
            if length > best_length:
                best_stride, best_length = stride, length
        if best_length >= MIN_RUN:
            out.append((start, best_stride))
            taken = {start + best_stride * i for i in range(best_length)}
        else:
            out.append((start, None))
            taken = {start}
        left = [o for o in left if o not in taken]
    return out


def offsets_text(offsets: set[int]) -> str:
    """The offsets column: runs as `start+stride*k`, the rest as plain offsets."""
    return " ".join("0x%x" % s if d is None else "0x%x+0x%x*k" % (s, d) for s, d in fold(offsets))


def target_offsets_text(low: int, high: int) -> str:
    return "0x%x" % low if low == high else "0x%x-0x%x" % (low, high)


def stats(members: list[Resolved]) -> dict:
    """The scenarios, ticks and target_offsets columns of the words of one row."""
    per_scenario: dict[str, int] = {}
    for r in members:
        per_scenario[r.scenario] = max(per_scenario.get(r.scenario, 0), r.ticks)
    if not members:
        return {"scenarios": "0", "ticks": "0", "target_offsets": "-"}
    return {"scenarios": str(len(per_scenario)), "ticks": str(sum(per_scenario.values())),
            "target_offsets": target_offsets_text(min(r.target_min for r in members),
                                                  max(r.target_max for r in members))}


def merge(rows: list[Resolved]) -> list[dict]:
    """One row per (holder, target) with no class or note, sorted by holder then target."""
    groups: dict[tuple[str, str], list[Resolved]] = {}
    for r in rows:
        groups.setdefault((r.holder, r.target), []).append(r)
    out = []
    for (holder, target), members in groups.items():
        out.append({"holder": holder, "offsets": offsets_text({r.offset for r in members}), "target": target,
                    **stats(members), "class": "", "note": ""})
    return sorted(out, key=row_order)


def is_wild(row: dict) -> bool:
    """Whether the row is a holder's wildcard row: `*` as its offsets and as its target."""
    return row["target"] == WILD and row["offsets"].strip() == WILD


def row_key(row: dict) -> tuple:
    """The identity of a row: (holder, target) for a pair row, (holder, `*`, class) for a wildcard row."""
    return (row["holder"], WILD, row["class"]) if is_wild(row) else (row["holder"], row["target"])


def row_order(row: dict) -> tuple:
    """The sort key of a row: holder, then wildcard rows by class, then the pair rows by target."""
    return (row["holder"], False, row["class"]) if is_wild(row) else (row["holder"], True, row["target"])


def with_wild(rows: list[dict], old: list[dict]) -> list[dict]:
    """The pair rows and the old file's wildcard rows, as written, sorted."""
    return sorted(rows + [dict(r) for r in old if is_wild(r)], key=row_order)


def render(rows: list[dict]) -> str:
    out = io.StringIO()
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(COLUMNS)
    writer.writerows([[row[c] for c in COLUMNS] for row in rows])
    return out.getvalue()


def read_rows(path: Path = OUT) -> list[dict]:
    """The rows of data/block_pointers.csv, or none when it does not exist."""
    if not path.exists():
        return []
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def offset_starts(offsets: str) -> set[str]:
    """The first offset of each term of an offsets column: `0xa+0x4*k 0x20` gives 0xa and 0x20."""
    return {term.split("+")[0] for term in offsets.split()}


def merge_kept(rows: list[dict], old: list[dict]) -> list[dict]:
    """The rows with the class and note the old file had for the same row_key. A pair row with none (its target
    was renamed) takes them from the old rows of its holder with the same offsets, else with an offset in common,
    when those agree."""
    kept = {row_key(r): r for r in old}
    by_holder: dict[str, list[tuple[set[str], dict]]] = {}
    for r in old:
        if not is_wild(r):
            by_holder.setdefault(r["holder"], []).append((offset_starts(r["offsets"]), r))
    for row in rows:
        before = kept.get(row_key(row))
        if not before and not is_wild(row):
            starts = offset_starts(row["offsets"])
            held = by_holder.get(row["holder"], [])
            same = [r for s, r in held if s == starts] or [r for s, r in held if s & starts]
            if same and len({tuple(r[c] for c in KEPT) for r in same}) == 1:
                before = same[0]
        if before:
            for column in KEPT:
                row[column] = before[column]
    return rows


def differences(have: list[dict], want: list[dict]) -> list[str]:
    """Unified diff lines of the rows with class and note left out."""
    def lines(rows):
        return [",".join(r[c] for c in COLUMNS if c not in KEPT) for r in rows]
    return list(difflib.unified_diff(lines(have), lines(want), "data/block_pointers.csv", "regenerated",
                                     lineterm="", n=0))


def check_rows(text: str) -> list[str]:
    """What is wrong with the text of data/block_pointers.csv: columns, values, classes, notes, order."""
    reader = csv.reader(io.StringIO(text))
    table = list(reader)
    if not table or table[0] != COLUMNS:
        return ["the header is not %s" % ",".join(COLUMNS)]
    problems = []
    previous = None
    pairs: dict[str, set[str]] = {}
    wilds: dict[str, set[str]] = {}
    for number, cells in enumerate(table[1:], 2):
        if len(cells) != len(COLUMNS):
            problems.append("line %d: %d columns, expected %d" % (number, len(cells), len(COLUMNS)))
            continue
        row = dict(zip(COLUMNS, cells))
        where = "line %d (%s -> %s)" % (number, row["holder"], row["target"])
        if not row["holder"] or not row["target"]:
            problems.append("line %d: holder and target must be set" % number)
        if WILD in row["holder"] or (WILD in row["target"] and row["target"] != WILD):
            problems.append("%s: `*` is only a whole target or the offsets" % where)
        wild = is_wild(row)
        offsets = row["offsets"].split()
        if (row["offsets"].strip() == WILD) != (row["target"] == WILD):
            problems.append("%s: `*` is the offsets and the target of a wildcard row together" % where)
        if offsets == [WILD]:
            pass
        elif not offsets or not all(OFFSET_TOKEN.match(o) for o in offsets):
            problems.append("%s: offsets %r are not `*` or `0x10`, `0x10+0x24*k` entries" % (where, row["offsets"]))
        if not row["scenarios"].isdigit() or not row["ticks"].isdigit():
            problems.append("%s: scenarios and ticks must be numbers" % where)
        if not TARGET_OFFSETS.match(row["target_offsets"]) and not (wild and row["target_offsets"] == "-"):
            problems.append("%s: target_offsets %r is not `0x0` or `0xa-0x40`%s"
                            % (where, row["target_offsets"], " (or `-` on a wildcard row)" if wild else ""))
        if row["class"] not in CLASSES:
            problems.append("%s: class %r is not one of %s" % (where, row["class"], ", ".join(CLASSES)))
        if row["class"] == "not-a-pointer" and not row["note"].strip():
            problems.append("%s: a not-a-pointer row needs a note" % where)
        key = row_order(row)
        if previous is not None and key <= previous:
            problems.append("%s: rows must be sorted by holder then target, without repeats" % where)
        previous = key
        if row["class"] in CLASSES:
            classes = (wilds if wild else pairs).setdefault(row["holder"], set())
            classes.add(row["class"])
    for holder, classes in sorted(pairs.items()):
        for cls in sorted(classes | {"not-a-pointer"}):
            if cls not in wilds.get(holder, set()):
                problems.append("%s: no `*` row of class %s for its pair rows" % (holder, cls))
    return problems


def is_data_holder(holder: str) -> bool:
    """Whether a holder is a member function of a `BlockVector<T>` instantiation, named by its decorated symbol."""
    return DATA_HOLDER.match(holder) is not None


def uncovered(scanned: list[Resolved], rows: list[dict]) -> list[Resolved]:
    """The scanned words whose holder has no wildcard row, one per distinct word.

    The buffer of a block vector holds the elements of the vector (the slack after them is zeroed), and its words are never
    pointers: the vector's own links are Ref words, offsets into the block. A word of such a buffer that reads as a heap
    address is the address of an allocation that landed on the element bytes, which depends on where the process heap is on
    that run, so a holder of that kind is covered without a row and no run can fail on one."""
    holders = {r["holder"] for r in rows if is_wild(r)}
    out = []
    seen = set()
    for s in scanned:
        if s.holder in holders or is_data_holder(s.holder):
            continue
        key = (s.scenario, s.holder, s.offset, s.target)
        if key not in seen:
            seen.add(key)
            out.append(s)
    return out


def describe_uncovered(missing: list[Resolved]) -> list[str]:
    """One line per word the file does not cover."""
    return ["%s: %s offset 0x%x -> %s (holder not listed)" % (s.scenario, s.holder, s.offset, s.target)
            for s in missing]


def check_scans(texts: dict[str, str | None], map_path: Path, build_dir: Path,
                rows: list[dict] | None = None) -> list[str]:
    """The problems of a gate run's blockptrs.txt files against data/block_pointers.csv, as lines."""
    problems = ["%s: no blockptrs.txt (did the exe run with -blockscan?)" % n for n, t in texts.items() if t is None]
    scans = []
    for name, text in texts.items():
        if text is None:
            continue
        try:
            scans.append(parse_scan(text, name))
        except ValueError as e:
            problems.append(str(e))
    if not scans:
        return problems
    if not map_path.is_file():
        return problems + ["%s: no linker map to name the sites with" % map_path]
    try:
        namer = load_namer(map_path)
    except ValueError as e:
        return problems + [str(e)]
    needs_layout = any(r.holder == SIM_GLOBALS for s in scans for r in s.rows)
    members = load_members(build_dir) if needs_layout else []
    listed = read_rows() if rows is None else rows
    missing = uncovered(resolve(scans, namer, members), listed)
    return problems + describe_uncovered(missing)


def default_build_dir() -> Path:
    """The compile database directory for the SimGlobals layout (build/rwsets)."""
    import rwsets
    return rwsets.DEFAULT_BUILD_DIR


def select_scenarios(all_scenarios: dict[str, Path], names: list[str] | None) -> dict[str, Path]:
    """The scenarios named (all of them when names is None); exits on a name that is not a scenario."""
    if not names:
        return all_scenarios
    unknown = [n for n in names if n not in all_scenarios]
    if unknown:
        sys.exit("blockpointers: no such scenario: %s (have %s)" % (", ".join(unknown), ", ".join(sorted(all_scenarios))))
    return {n: all_scenarios[n] for n in names}


def play_suite(exe: Path, game_dir: str | None, parallel: int, names: list[str] | None = None,
               failed: dict[str, str] | None = None) -> dict[str, str | None]:
    """Play the scenarios with the scan on every tick; each one's blockptrs.txt text (None when it wrote none).

    A scenario that fails is tried once more, and one that fails twice is recorded in `failed`
    (name -> the last lines of its error) and has no entry in the result.
    """
    import reference
    scans: dict[str, str | None] = {}
    available = reference.scenario_files()
    selected = select_scenarios(available, names)
    if names:
        selected = reference.with_savers(selected, available)
    reference.play_all(exe, selected, game_dir, parallel,
                       failed=failed if failed is not None else {}, scans=scans, retry_any=True)
    return scans


def save_scans(texts: dict[str, str | None], directory: Path) -> None:
    """Write each scenario's scan text to directory/<scenario>.txt, the files --from reads; skip the None ones."""
    directory.mkdir(parents=True, exist_ok=True)
    for name, text in texts.items():
        if text is not None:
            (directory / f"{name}.txt").write_text(text)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the data file is not valid (CI)")
    parser.add_argument("--unclassified", action="store_true", help="list the rows with no class")
    parser.add_argument("--from", dest="sources", nargs="+", type=Path, metavar="DIR",
                        help="build from these blockptrs files or directories of <scenario>.txt, playing nothing")
    parser.add_argument("--save-scans", type=Path, metavar="DIR",
                        help="write each played scenario's blockptrs.txt to DIR/<scenario>.txt, as --from reads them")
    parser.add_argument("--scenario", nargs="+", metavar="NAME",
                        help="play only these scenarios (needs --save-scans, to merge later with --from)")
    parser.add_argument("--exe", type=Path, default=DEFAULT_EXE, help="the exe to play (default build/modern/TotalA.exe)")
    parser.add_argument("--map", type=Path, help="the linker map (default: the exe's)")
    parser.add_argument("--game-dir", help="the game's install (default: Steam's)")
    parser.add_argument("--parallel", type=int, metavar="N", help="scenarios played at once")
    parser.add_argument("--build-dir", type=Path, help="compile database for the SimGlobals layout (default build/rwsets)")
    parser.add_argument("--prune", action="store_true",
                        help="drop the wildcard rows whose holder is no allocation site of the exe (pair rows come from the scans)")
    parser.add_argument("--out", type=Path, default=OUT, help="write the rows here instead of the data file")
    args = parser.parse_args()
    if args.save_scans and (args.sources or args.check or args.unclassified):
        parser.error("--save-scans saves the scans the tool plays; it cannot be used with --from, --check or --unclassified")
    if args.scenario and (args.sources or args.check or args.unclassified):
        parser.error("--scenario picks the scenarios to play; it cannot be used with --from, --check or --unclassified")
    if args.prune and (args.check or args.unclassified):
        parser.error("--prune rewrites the data file; it cannot be used with --check or --unclassified")
    if args.scenario and not args.save_scans:
        parser.error("--scenario plays part of the suite, which must not overwrite the data file: add --save-scans DIR "
                     "and merge the saved scans with --from")
    if args.check:
        problems = check_rows(OUT.read_text()) if OUT.exists() else ["%s does not exist" % OUT]
        for p in problems[:40]:
            print(p)
        if problems:
            print("blockpointers: %d problems in data/block_pointers.csv" % len(problems))
            sys.exit(1)
        print("blockpointers: ok (%d rows)" % len(read_rows()))
        return
    old = read_rows()
    if args.unclassified:
        empty = [r for r in old if not r["class"]]
        bad = [r for r in old if r["class"] and r["class"] not in CLASSES]
        for r in bad:
            print("bad class %r: %s -> %s" % (r["class"], r["holder"], r["target"]))
        for r in empty[:20]:
            print("no class: %s -> %s (%s scenarios, %s ticks)" % (r["holder"], r["target"], r["scenarios"], r["ticks"]))
        print("blockpointers: %d of %d rows have no class" % (len(empty), len(old)))
        sys.exit(1 if empty or bad else 0)
    map_path = args.map or args.exe.with_suffix(".map")
    if args.sources:
        scans = read_scans(args.sources)
    else:
        import reference
        failed: dict[str, str] = {}
        texts = play_suite(args.exe.resolve(), args.game_dir, args.parallel or reference.default_parallel(),
                           args.scenario, failed)
        if args.save_scans:
            save_scans(texts, args.save_scans)
        if failed:
            print("blockpointers: %d scenarios failed:" % len(failed))
            for name, message in sorted(failed.items()):
                print("  %s:\n    %s" % (name, message))
            sys.exit(1)
        if args.scenario:
            print("blockpointers: played %d scenarios; merge with --from %s" % (len(texts), args.save_scans))
            return
        absent = [n for n, t in texts.items() if t is None]
        if absent:
            sys.exit("blockpointers: no blockptrs.txt from: " + ", ".join(absent))
        scans = [parse_scan(t, n) for n, t in sorted(texts.items())]
    if not map_path.is_file():
        sys.exit("blockpointers: no linker map at %s (--map)" % map_path)
    sampled = sorted({s.every for s in scans} - {1})
    if sampled:
        print("warning: some scans sample the ticks (every %s); the list is made from every tick"
              % ", ".join(str(n) for n in sampled))
    commits = {s.commit for s in scans}
    if len(commits) > 1:
        print("warning: the scans come from different commits: " + ", ".join(sorted(commits)))
    members = load_members(args.build_dir or default_build_dir()) \
        if any(r.holder == SIM_GLOBALS for s in scans for r in s.rows) else []
    try:
        namer = load_namer(map_path)
    except ValueError as e:
        sys.exit("blockpointers: %s" % e)
    kept_wild = prune_wild(old, namer) if args.prune else old
    merged = merge_kept(with_wild(merge(resolve(scans, namer, members)), kept_wild), old)
    now = {row_key(r) for r in merged}
    lost = [r for r in old if (r["class"] or r["note"]) and row_key(r) not in now]
    args.out.write_text(render(merged))
    print("%d rows from %d scans" % (len(merged), len(scans)))
    if lost:
        print("warning: %d rows with a class or note no longer exist and were dropped" % len(lost))
        for r in lost[:20]:
            print("  %s -> %s: %s %s" % (r["holder"], r["target"], r["class"], r["note"]))


if __name__ == "__main__":
    main()
