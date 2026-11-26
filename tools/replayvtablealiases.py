"""Write tests/replay_vtable_aliases.rsp: the /alternatename lines that bind the placeholder names of src/data/vtables.cpp to the methods the replay link holds.

    uv run tools/replayvtablealiases.py --build-dir build/modern          # rewrite the file
    uv run tools/replayvtablealiases.py --build-dir build/modern --check  # exit 1 when it is out of date

The game link maps those names to the methods by address (tools/aliases.py). The replay link has no address map, so a virtual call on a rebuilt
OrderFx would call 0 without them. The file also binds every other name a module object uses and does not define, such as a constructor
that orders declares with other parameter types, to the definition the game's aliases.obj maps it to, when the replay link holds that definition.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "tests/replay_vtable_aliases.rsp"
GLOBAL_NAMES = ("SerializeSave", "TryGetDesiredHeading")
PLACEHOLDER = re.compile(r"^\?(\w+)@((?:\w+@)?)@YGXXZ$")


def nm(path: Path, flag: str) -> list[str]:
    out = subprocess.run(["llvm-nm", flag, str(path)], capture_output=True, text=True, check=True).stdout
    return [line.split()[-1] for line in out.splitlines() if line.split()]


def lines(build: Path) -> list[str]:
    vtables = build / "CMakeFiles/replay_native_test_objects.dir/src/data/vtables.cpp.obj"
    modules = sorted((build / "CMakeFiles/replay_native_modules.dir").rglob("*.obj"))
    defined: dict[str, list[str]] = {}
    for obj in modules:
        for symbol in nm(obj, "--defined-only"):
            m = re.match(r"^\?(\w+)@((?:\w+@)?)@([QU]A|YG)", symbol)
            if m:
                defined.setdefault((m.group(1), m.group(2)), []).append(symbol)
    out = []
    for symbol in nm(vtables, "-u"):
        m = PLACEHOLDER.match(symbol)
        if not m or (not m.group(2) and m.group(1) not in GLOBAL_NAMES):
            continue
        found = defined.get((m.group(1), m.group(2)), [])
        if len(found) == 1:
            out.append("/alternatename:%s=%s" % (symbol, found[0]))
    tables = {m.group(1): symbol for symbol in nm(vtables, "--defined-only") for m in [re.match(r"^\?(g_\w+Vtable)@@3", symbol)] if m}
    objects = modules + sorted((build / "CMakeFiles/replay_native_test_objects.dir").rglob("*.obj"))
    seen = set()
    for obj in objects:
        for symbol in nm(obj, "-u"):
            m = re.match(r"^\?(g_\w+Vtable)@@3", symbol)
            if m and m.group(1) in tables and symbol != tables[m.group(1)] and symbol not in seen:
                seen.add(symbol)
                out.append("/alternatename:%s=%s" % (symbol, tables[m.group(1)]))
    linked = weak_externals(build / "aliases.obj")
    held = {symbol for obj in objects for symbol in nm(obj, "--defined-only")}
    bound = {line.split(":", 1)[1].split("=")[0] for line in out}
    for obj in objects:
        for symbol in nm(obj, "-u"):
            target = symbol
            for _ in range(8):
                target = linked.get(target)
                if target is None or target in held:
                    break
            if target in held and symbol not in held and symbol not in bound:
                bound.add(symbol)
                out.append("/alternatename:%s=%s" % (symbol, target))
    return sorted(out)


def weak_externals(obj: Path) -> dict[str, str]:
    """Each weak external of the game's alias object: the name a caller spells, and the name it is linked to."""
    if not obj.is_file():
        raise SystemExit("replayvtablealiases: %s is missing; build it first (ninja -C %s aliases.obj)" % (obj, obj.parent))
    text = subprocess.run(["llvm-readobj", "--symbols", str(obj)], capture_output=True, text=True, check=True).stdout
    return {m.group(1): m.group(2) for m in re.finditer(r"Name: (\S+)\n(?:.*\n){1,7}?\s*AuxWeakExternal \{\n\s*Linked: (\S+) \(", text)}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--build-dir", default="build/modern")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    text = "\n".join(lines(ROOT / args.build_dir)) + "\n"
    if args.check:
        if not OUT.is_file() or OUT.read_text() != text:
            print("replayvtablealiases: tests/replay_vtable_aliases.rsp is out of date; run `uv run tools/replayvtablealiases.py`", file=sys.stderr)
            return 1
        print("replayvtablealiases: up to date")
        return 0
    OUT.write_text(text)
    print("replayvtablealiases: wrote %s" % OUT.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
