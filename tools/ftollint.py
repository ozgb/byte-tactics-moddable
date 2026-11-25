"""Fail when a game-logic function of the modern exe converts a float to an integer inline instead of through bx_ftol.

    uv run tools/ftollint.py --exe build/modern/TotalA.exe          # the map is TotalA.map beside it
    uv run tools/ftollint.py --exe E --map M --objdump llvm-objdump # name them

3.1c converts every double or float to an integer by calling MSVC 5's `_ftol`, which truncates to a 64-bit integer
and keeps its low 32 bits: a value out of the int range wraps, and NaN, the infinities and magnitudes of 2^63 or
more give 0. clang converts inline (`fistp` of 32 bits, or `cvttsd2si`), which gives -2147483648 there, so game
logic calls `bx_ftol` (lib/bitexact/ftol.c) for each conversion (#1066). The trace gate sees a conversion only
when a scenario drives it out of range; this lint sees all of them. It disassembles .text with llvm-objdump, names
each instruction's function from the linker map (as tools/x87lint.py does) and fails on a `fist`, `fistp`,
`fisttp` or a truncating or rounding SSE conversion to an integer in a game-logic function: one whose
`// FUNCTION:` address is in data/game_logic.csv or is a `game` row of data/crt_calls.csv, or whose own name is
there. LIBCMT and lib/bitexact objects are left out. The limit: an instruction is put in the public symbol that
precedes it, so a static function with no map entry counts as part of the function before it, and a game function
inlined into a presentation function is not checked. Exits 1 on a failure.
"""

import argparse
import csv
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sources
from x87lint import INSN_LINE, parse_map

ROOT = Path(__file__).resolve().parent.parent
GAME_LOGIC = ROOT / "data" / "game_logic.csv"
CRT_CALLS = ROOT / "data" / "crt_calls.csv"
CONVERSION = re.compile(r"^(fistp?|fisttp)[slq]*$|^v?cvtt?(s[sd]2si|p[sd]2dq|p[sd]2pi)[lq]?$")
EXCLUDED_OBJECTS = ("LIBCMT:", "bitexact:")


def game_addresses(game_logic: Path, crt_calls: Path) -> tuple[set[int], set[str]]:
    """The 3.1c addresses and names of the game-logic functions: data/game_logic.csv and the game rows of
    data/crt_calls.csv."""
    addresses, names = set(), set()
    with open(game_logic, newline="") as f:
        for row in csv.DictReader(f):
            addresses.add(int(row["address"], 16))
            names.add(row["function"])
    with open(crt_calls, newline="") as f:
        for row in csv.DictReader(f):
            if row["class"] == "game":
                addresses.add(int(row["address"], 16))
                names.add(row["function"])
    return addresses, names


def game_names(addresses: set[int], names: set[str], annotations: list[tuple[int, str]]) -> set[str]:
    """The modern names of the game-logic functions: the names of the files' `// FUNCTION:` markers at those
    addresses, and the names the data files give."""
    return set(names) | {name for address, name in annotations if address in addresses}


def lint(disasm: str, map_text: str, game: set[str]) -> tuple[list[str], list[str]]:
    """The lines to print and the problems found."""
    code = parse_map(map_text)
    report, problems = [], []
    counts: dict[tuple[str, str, str], int] = {}
    for m in map(INSN_LINE.match, disasm.splitlines()):
        if not m or not CONVERSION.match(m.group(2)):
            continue
        f = code.at(int(m.group(1), 16))
        if f is None or f.obj.startswith(EXCLUDED_OBJECTS):
            continue
        key = (f.plain, f.obj, m.group(2))
        counts[key] = counts.get(key, 0) + 1
    for (name, obj, mnemonic), n in sorted(counts.items()):
        if name in game:
            report.append("%s x%d in %s (%s): game logic" % (mnemonic, n, name, obj))
            problems.append("%s converts inline (%s x%d), not through bx_ftol" % (name, mnemonic, n))
        else:
            report.append("%s x%d in %s (%s): not game logic" % (mnemonic, n, name, obj))
    return report, problems


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe", required=True, help="the linked exe (its map is TotalA.map beside it)")
    parser.add_argument("--map", help="the linker map (default: the exe's name with .map)")
    parser.add_argument("--objdump", default="llvm-objdump", help="the disassembler (default llvm-objdump)")
    args = parser.parse_args(argv)
    map_path = Path(args.map) if args.map else Path(args.exe).with_suffix(".map")
    disasm = subprocess.run([args.objdump, "-d", "--no-show-raw-insn", "--section=.text", args.exe],
                            capture_output=True, text=True, check=True).stdout
    addresses, names = game_addresses(GAME_LOGIC, CRT_CALLS)
    annotations = [a for src in sources.source_files() for a in sources.annotations(src)]
    report, problems = lint(disasm, map_path.read_text(errors="replace"), game_names(addresses, names, annotations))
    for line in report:
        print(line)
    for line in problems:
        print("FAIL: " + line)
    if not problems:
        print("ftollint: ok")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
