"""Check that a state-writing function is pure, stateless and free of scenario logic, from the relocations of the build.

    uv run tools/writercheck.py --objects @build/modern/objects.rsp           # print every violation
    uv run tools/writercheck.py --objects @build/modern/objects.rsp --check
    uv run tools/test_writercheck.py                                           # tests, on hand-written units

The state writer of the default pathfinder (PathfindingWriteState, interfaces/pathfinding.h) hands the host the state that
decides what the module does next, through the functions of a StateWriter, and changes nothing and keeps nothing. The code of one function is a section of an object whose relocations name the symbols it needs
(tools/seamlink.py reads them). From the writer function the check follows every call to a function that some
object of the build defines, and judges each symbol that function references:

  region root   g_game, the pointer through which the region is reached, and SimBlockBase and SimOwns, which
                Ref<T>::get calls to turn an offset into an address: the writer may read them
  literal       a string constant (the name of a field), which is read only
  own           a function of the build that the writer calls, which is walked in turn
  anything else is a violation: a global or a static (state the call could keep or change), a library function
                (memcpy and the like write through their arguments), an import, or a function that no object defines.

A violation whose symbol names the tick, the scenario, the harness, the replay gate or the test mode is reported as
scenario logic, any other as impure. It also reads the rows of data/rwsets.csv for the walked functions: no write is allowed, since the
only way out is the writer. Standard library plus tools/seamlink.py.
"""

import argparse
import csv
import os
import re
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from seamlink import unit_of, response, Unit
from sources import ROOT, base_name

WRITERS = (("src/ai/pathfind.cpp", "PathfindingWriteState"),)
ROOT_SYMBOLS = ("?g_game@@3PAUGame@@A", "?SimBlockBase@@YAPAXXZ", "?SimOwns@@YA_NPBX@Z")
LITERAL = "??_C@"
SCENARIO = re.compile(r"tick|scenario|harness|replay|testmode|test_mode", re.I)
RWSETS = "data/rwsets.csv"
WRITES = ("field_write", "deref_write", "global_write", "field_maybe_written", "deref_maybe_written", "global_maybe_written")


def walk(units: list, file: str, name: str) -> tuple:
    """(violations, walked): the (function, symbol, kind) triples of the writer `name` of `file` and the base names of the
    functions walked. A kind is `scenario` or `impure`."""
    defined = {}
    for unit in units:
        for symbol in unit.code:
            defined.setdefault(symbol, unit)
    refs = {}
    for unit in units:
        for function, symbol in unit.refs:
            refs.setdefault(function, set()).add(symbol)
    starts = [s for u in units if u.file == file for s in u.code if base_name(s) == name]
    if not starts:
        return [(name, "-", "missing")], []
    todo, seen, found = list(starts), set(starts), []
    while todo:
        function = todo.pop()
        for symbol in sorted(refs.get(function, ())):
            if symbol in ROOT_SYMBOLS or symbol.startswith(LITERAL):
                continue
            if symbol in defined:
                if symbol not in seen:
                    seen.add(symbol)
                    todo.append(symbol)
                continue
            kind = "scenario" if SCENARIO.search(base_name(symbol)) else "impure"
            found.append((base_name(function), base_name(symbol), kind))
    return sorted(set(found)), sorted({base_name(s) for s in seen})


def writes(root: Path, file: str, names: list) -> list:
    """(function, kind, target) of every write of the walked functions in data/rwsets.csv to something but the view."""
    found = set()
    with (root / RWSETS).open(newline="") as f:
        for row in csv.DictReader(f):
            if row["function"] not in names or row["kind"] not in WRITES:
                continue
            found.add((row["function"], row["kind"], row["target"]))
    return sorted(found)


def problems(units: list, root: Path, writers: tuple = WRITERS) -> list:
    out = []
    for file, name in writers:
        found, names = walk(units, file, name)
        for function, symbol, kind in found:
            if kind == "missing":
                out.append(f"{name}: no function of that name in {file}")
            elif kind == "scenario":
                out.append(f"{name}: {function} references {symbol}, which names the tick, the scenario or the harness")
            else:
                out.append(f"{name}: {function} references {symbol}, which is neither the region root nor a function walked")
        for function, kind, target in writes(root, file, names):
            out.append(f"{name}: {function} has {kind} {target}, but the writer changes nothing")
    return out


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--objects", nargs="+", required=True, help="object files, or @file listing them")
    parser.add_argument("--root", default=str(ROOT))
    parser.add_argument("--check", action="store_true", help="the same, quiet when there is none")
    args = parser.parse_args(argv)
    units = [unit_of(p) for a in args.objects for p in response(a)]
    bad = problems(units, Path(args.root))
    for line in bad:
        print(line)
    if not bad and not args.check:
        print("writercheck: %s are pure, stateless and free of scenario logic" % ", ".join(n for _, n in WRITERS))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
