"""Build the WebAssembly fixtures of the core gate's loader and fuel groups, standard library and wasm-tools only.

    uv run tools/corefixtures.py build     # rebuild every reference/core/{loader,fuel}/NAME.wasm from its NAME.wat
    uv run tools/corefixtures.py check     # exit 1 when a committed .wasm differs from a rebuild

The fuel group's modules also get the fuel pass of docs/module-abi.md, section 11 (tools/wasm_fuel.py). wasm-tools is
build/tools/bin/wasm-tools, else $WASM_TOOLS or PATH.
"""

import argparse
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORE = os.path.join(ROOT, "reference", "core")
GROUPS = ("loader", "fuel")
METERED = ("fuel",)
WASM_TOOLS = os.path.join(ROOT, "build", "tools", "bin", "wasm-tools")

sys.path.insert(0, os.path.join(ROOT, "tools"))
import wasm_fuel


def find_wasm_tools():
    if os.path.exists(WASM_TOOLS):
        return WASM_TOOLS
    return wasm_fuel.find_wasm_tools()


def build_one(wasm_tools, group, wat):
    result = subprocess.run([wasm_tools, "parse", wat], capture_output=True)
    if result.returncode:
        sys.exit(f"{wat}: {result.stderr.decode(errors='replace').strip()}")
    data = result.stdout
    if group in METERED:
        data = wasm_fuel.instrument(data, wasm_tools)
    return data


def fixtures():
    for group in GROUPS:
        directory = os.path.join(CORE, group)
        for name in sorted(os.listdir(directory)):
            if name.endswith(".wat"):
                yield group, os.path.join(directory, name)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("command", choices=("build", "check"))
    args = parser.parse_args()
    wasm_tools = find_wasm_tools()
    stale = []
    for group, wat in fixtures():
        data = build_one(wasm_tools, group, wat)
        target = wat[:-4] + ".wasm"
        if args.command == "build":
            with open(target, "wb") as f:
                f.write(data)
            print(f"built {os.path.relpath(target, ROOT)}: {len(data)} bytes")
            continue
        try:
            with open(target, "rb") as f:
                same = f.read() == data
        except FileNotFoundError:
            same = False
        if not same:
            stale.append(os.path.relpath(target, ROOT))
    for path in stale:
        print(f"differs from a rebuild: {path}")
    return 1 if stale else 0


if __name__ == "__main__":
    sys.exit(main())
