"""Fail when the modern exe's game logic can reach an x87 instruction whose result depends on the CPU.

    uv run tools/x87lint.py --exe build/modern/TotalA.exe          # the map is TotalA.map beside it
    uv run tools/x87lint.py --exe E --map M --objdump llvm-objdump # name them

fsin, fcos, fsincos, fptan, fpatan, f2xm1, fyl2x and fyl2xp1 are not correctly rounded and differ between
Intel and AMD (#19). lib/bitexact/ replaces the CRT calls that reach them, but the trace gate cannot see a call
that escapes that routing: the reference traces were recorded on one CPU. This lint disassembles .text with
llvm-objdump, names each instruction's function from the linker map (`Publics by Value`, the `Lib:Object`
column) and fails on:

  - one of the eight instructions outside a LIBCMT object;
  - a direct `call` or `jmp` from a function outside LIBCMT into the CPU-dependent set (the LIBCMT functions that
    hold one of the instructions, and the LIBCMT functions that call or jump to those, to a fixed point:
    `_pow` reaches `__twoToTOS`), unless the pair is in ALLOWED. The CRT reaches `fpatan` and the rest through
    dispatch tables (`_atan2` names `__OP_ATAN2jmptab`, which holds the address of `__fFATN2`), so a function
    that names the address of a data symbol whose words (up to the next data symbol) hold the address of a
    function counts as calling it (a CRT function only for a CRT function);
  - an allowed caller that data/subsystems.csv does not put in presentation, platform or network.

It prints each instruction and each caller it found, so a failure names the function. The limit: indirect calls
through a register or a function pointer that is not in a table the code names are not followed. Exits 1 on a
failure.
"""

import argparse
import bisect
import csv
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from blockpointers import SKIPPED, demangle

DEPENDENT = {"fsin", "fcos", "fsincos", "fptan", "fpatan", "f2xm1", "fyl2x", "fyl2xp1"}
BRANCHES = {"call", "calll", "jmp", "jmpl"}
ALLOWED_SUBSYSTEMS = {"presentation", "platform", "network"}
ALLOWED = {("HandleReportClick", "pow"): "presentation: the multiplayer report screen"}
SUBSYSTEMS = Path(__file__).resolve().parent.parent / "data" / "subsystems.csv"

MAP_LINE = re.compile(r"^\s*(000\d):[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8,16})\s+(\S.*?)\s*$")
INSN_LINE = re.compile(r"^\s*([0-9a-f]+):\s+(\w+)\s*(.*)$")
DATA_LINE = re.compile(r"^ ([0-9a-f]+)((?: [0-9a-f]{1,8}){1,4})")
LITERAL = re.compile(r"0x([0-9a-f]+)")
TABLE_WORDS = 64


@dataclass
class Function:
    """A code symbol of the map: its decorated name, address and object."""
    name: str
    address: int
    obj: str

    @property
    def libcmt(self) -> bool:
        """Whether the function comes from the C runtime library."""
        return self.obj.startswith("LIBCMT:")

    @property
    def plain(self) -> str:
        """The name without decoration."""
        return demangle(self.name)


class CodeMap:
    """The code functions of a linker map, found by address."""

    def __init__(self, functions: list[Function], data_addresses: list[int] | None = None):
        """`functions` are in any order; the first of several names at one address wins. `data_addresses` are the
        addresses of the data symbols."""
        self.data_addresses = sorted(set(data_addresses or []))
        seen: dict[int, Function] = {}
        for f in functions:
            seen.setdefault(f.address, f)
        self.functions = [seen[a] for a in sorted(seen)]
        self.addresses = [f.address for f in self.functions]

    def add(self, function: Function) -> None:
        """Insert a function in address order."""
        i = bisect.bisect_right(self.addresses, function.address)
        self.functions.insert(i, function)
        self.addresses.insert(i, function.address)

    def next_data_symbol(self, address: int) -> int:
        """The address of the first data symbol after the address (the end of the table it starts)."""
        i = bisect.bisect_right(self.data_addresses, address)
        return self.data_addresses[i] if i < len(self.data_addresses) else address + 4 * TABLE_WORDS

    def at(self, address: int) -> Function | None:
        """The function that contains the address."""
        i = bisect.bisect_right(self.addresses, address) - 1
        return self.functions[i] if i >= 0 else None


def parse_map(text: str) -> CodeMap:
    """The code symbols of a linker map with their objects."""
    functions, data = [], []
    for line in text.splitlines():
        m = MAP_LINE.match(line)
        if not m or m.group(2).startswith(SKIPPED):
            continue
        if m.group(1) == "0001":
            functions.append(Function(m.group(2), int(m.group(3), 16), m.group(4)))
        else:
            data.append(int(m.group(3), 16))
    return CodeMap(functions, data)


def add_unnamed_functions(lines: list[re.Match], code: CodeMap) -> None:
    """Add the functions the map has no name for. An object's first function may have no public symbol (87tran.obj
    starts 17 bytes before `_fFEXP`), so the code would be taken as the end of the function before it. Such a
    function follows padding (nop or int3) at a 16-byte boundary and starts before the next symbol; it takes that
    symbol's object."""
    padding = False
    for m in lines:
        address = int(m.group(1), 16)
        if m.group(2) in ("nop", "int3"):
            padding = True
            continue
        if padding and address % 16 == 0:
            f, i = code.at(address), bisect.bisect_right(code.addresses, address)
            if f and f.address < address and i < len(code.functions):
                code.add(Function("?unnamed_%x@@" % address, address, code.functions[i].obj))
        padding = False


class DataImage:
    """The words of the exe's data sections, by address."""

    def __init__(self):
        self.words: dict[int, int] = {}

    def add(self, address: int, hexwords: list[str]) -> None:
        """Add the words of one line of `llvm-objdump -s`; a last word shorter than 4 bytes is left out."""
        for i, w in enumerate(hexwords):
            if len(w) == 8:
                self.words[address + 4 * i] = int.from_bytes(bytes.fromhex(w), "little")


def parse_data(dump: str) -> DataImage:
    """The words of an `llvm-objdump -s` dump."""
    data = DataImage()
    for line in dump.splitlines():
        m = DATA_LINE.match(line)
        if m:
            data.add(int(m.group(1), 16), m.group(2).split())
    return data


def scan(disasm: str, code: CodeMap, data: DataImage | None = None):
    """The CPU-dependent instructions as (function, mnemonic) and the references between functions as
    (function, target function, how): a direct call or jmp, or a code pointer in a data table the function names."""
    instructions, branches = [], []
    data = data or DataImage()
    lines = [m for m in map(INSN_LINE.match, disasm.splitlines()) if m]
    add_unnamed_functions(lines, code)
    addresses = [int(m.group(1), 16) for m in lines]
    text = (min(addresses), max(addresses)) if addresses else (0, 0)
    for m in lines:
        mnemonic = m.group(2)
        if mnemonic in DEPENDENT:
            f = code.at(int(m.group(1), 16))
            if f:
                instructions.append((f, mnemonic))
        if mnemonic in BRANCHES and re.match(r"0x[0-9a-f]+\b", m.group(3)):
            source, target = code.at(int(m.group(1), 16)), code.at(int(m.group(3).split()[0], 16))
            if source and target and source is not target:
                branches.append((source, target, "calls"))
        elif data.words and mnemonic not in BRANCHES:
            for literal in LITERAL.findall(m.group(3)):
                address = int(literal, 16)
                if address not in data.words:
                    continue
                source = code.at(int(m.group(1), 16))
                end = code.next_data_symbol(address)
                for pointer in (data.words.get(address + 4 * i) for i in range(min(TABLE_WORDS, (end - address) // 4))):
                    target = code.at(pointer) if pointer and text[0] <= pointer <= text[1] else None
                    if source and target and target.address == pointer and source is not target \
                            and (target.libcmt or not source.libcmt):
                        branches.append((source, target, "points at (table 0x%x)" % address))
    return instructions, branches


def read_subsystems(path: Path) -> dict[str, str]:
    """The subsystem of each function of data/subsystems.csv."""
    with open(path, newline="") as f:
        return {row["function"]: row["subsystem"] for row in csv.DictReader(f)}


def lint(disasm: str, map_text: str, subsystems: dict[str, str], data: DataImage | None = None) -> tuple[list[str], list[str]]:
    """The lines to print and the problems found."""
    code = parse_map(map_text)
    instructions, branches = scan(disasm, code, data)
    report, problems = [], []
    reach: dict[int, Function] = {}
    for f, mnemonic in instructions:
        report.append("%s in %s (%s)" % (mnemonic, f.plain, f.obj))
        if f.libcmt:
            reach[f.address] = f
        else:
            problems.append("%s outside LIBCMT: %s (%s)" % (mnemonic, f.plain, f.obj))
    changed = True
    while changed:
        changed = False
        for source, target, _ in branches:
            if source.libcmt and target.address in reach and source.address not in reach:
                reach[source.address] = source
                changed = True
    report.append("CPU-dependent LIBCMT functions: %s" % (", ".join(sorted(f.plain for f in reach.values())) or "none"))
    for source, target, how in branches:
        if source.libcmt or target.address not in reach:
            continue
        pair = (source.plain, target.plain)
        subsystem = subsystems.get(source.plain)
        if pair not in ALLOWED:
            report.append("%s %s %s: not allowed" % (pair[0], how, pair[1]))
            problems.append("%s (%s) %s %s, which reaches a CPU-dependent x87 instruction" % (source.plain, source.obj, how, target.plain))
        elif subsystem not in ALLOWED_SUBSYSTEMS:
            report.append("%s %s %s: allowed (%s), subsystem %s" % (pair[0], how, pair[1], ALLOWED[pair], subsystem))
            problems.append("%s %s %s, allowed as %s, but data/subsystems.csv puts it in %s, not presentation, platform or network"
                            % (pair[0], how, pair[1], ALLOWED[pair], subsystem or "no subsystem"))
        else:
            report.append("%s %s %s: allowed (%s), subsystem %s" % (pair[0], how, pair[1], ALLOWED[pair], subsystem))
    return report, problems


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe", required=True, help="the linked exe (its map is TotalA.map beside it)")
    parser.add_argument("--map", help="the linker map (default: the exe's name with .map)")
    parser.add_argument("--objdump", default="llvm-objdump", help="the disassembler (default llvm-objdump)")
    parser.add_argument("--subsystems", default=str(SUBSYSTEMS), help="data/subsystems.csv")
    args = parser.parse_args(argv)
    map_path = Path(args.map) if args.map else Path(args.exe).with_suffix(".map")
    disasm = subprocess.run([args.objdump, "-d", "--no-show-raw-insn", "--section=.text", args.exe],
                            capture_output=True, text=True, check=True).stdout
    dump = subprocess.run([args.objdump, "-s", "--section=.rdata", "--section=.data", args.exe],
                          capture_output=True, text=True, check=True).stdout
    report, problems = lint(disasm, map_path.read_text(errors="replace"), read_subsystems(Path(args.subsystems)), parse_data(dump))
    for line in report:
        print(line)
    for line in problems:
        print("FAIL: " + line)
    if not problems:
        print("x87lint: ok")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
