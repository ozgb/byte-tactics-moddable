"""Fuel metering for a wasm module: count the module's own work in code compiled into it, so every host stops it at the same point.

    uv run tools/wasm_fuel.py module.wasm -o metered.wasm [--wasm-tools PATH]
    uv run tools/wasm_fuel.py metered.wasm --check

The rule (version 1, recorded as "fuel" in wasm/modules.json):

    The counter drops by 1 at each function entry and at the top of each loop body: once per call and once
    per iteration. When it goes below zero the module executes `unreachable`.

The counter is a mutable i64 global the pass defines in the module. It is never imported or exported, so a host
can neither depend on its index nor change it from outside. The pass adds two exported functions, `__fuel_set(i64)`
(store the counter) and `__fuel_get() -> i64` (read it). A host sets the counter to a budget before a stretch of
work and reads it after: the fuel used is the budget minus the value read, and a trap with the counter below zero is
running out of fuel. The counter starts at the largest i64 so that a start function runs unmetered.

The global, the two functions and the two exports are added after everything the module has, so no function, global,
table or data index moves, and neither does the layout of memory or the table. Types are only added at the end. The
pass prints the module with wasm-tools (the version pinned in wasm/modules.json), inserts the metering instructions
into the text, and parses it back: the printer names every function and label by its index, and the parser reads
every encoding wasm-ld, rustc and the toolchains after them write, so the pass needs no opcode table of its own.
The parse drops the name section.

Running the pass on a module that is already metered is refused, since a second counter would burn twice per step.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

RULE = ("The counter drops by 1 at each function entry and at the top of each loop body (once per call and once per iteration)."
        " When it goes below zero the module traps (unreachable).")
VERSION = 1
COUNTER = "$__fuel_counter"
SET_EXPORT = "__fuel_set"
GET_EXPORT = "__fuel_get"
EXPORTS = (SET_EXPORT, GET_EXPORT)
INITIAL = (1 << 63) - 1
METER = ["global.get " + COUNTER, "i64.const 1", "i64.sub", "global.set " + COUNTER,
         "global.get " + COUNTER, "i64.const 0", "i64.lt_s", "if", "  unreachable", "end"]

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WASM_TOOLS = os.path.join(ROOT, "build", "tools", "bin", "wasm-tools")


class FuelError(Exception):
    pass


def find_wasm_tools(path=None):
    found = path or os.environ.get("WASM_TOOLS") or (WASM_TOOLS if os.path.exists(WASM_TOOLS) else shutil.which("wasm-tools"))
    if not found:
        raise FuelError("wasm-tools not found: cargo install wasm-tools --locked --root build/tools")
    return found


def tool(wasm_tools, args, data):
    result = subprocess.run([wasm_tools, *args], input=data, capture_output=True)
    if result.returncode:
        raise FuelError(f"wasm-tools {' '.join(args)} failed: {result.stderr.decode(errors='replace').strip()}")
    return result.stdout


def is_metered(text):
    return COUNTER in text or any(f'(export "{name}"' in text for name in EXPORTS)


def open_parens(line):
    """The parentheses a line opens minus those it closes, outside quoted names and strings."""
    bare = re.sub(r'"(?:[^"\\]|\\.)*"', "", line)
    return bare.count("(") - bare.count(")")


def meter_text(text):
    """The wat text of a module with the counter, the metering code and the two functions added."""
    if is_metered(text):
        raise FuelError("the module is already metered: a second counter would burn twice per step")
    lines = text.split("\n")
    out = []
    globals_end = None
    i = 0
    while i < len(lines):
        line = lines[i]
        out.append(line)
        i += 1
        if re.match(r"^  \(global ", line):
            globals_end = len(out)
        if re.match(r"^  \(func ", line) and open_parens(line) > 0:
            while i < len(lines) and re.match(r"^    \(local ", lines[i]):
                out.append(lines[i])
                i += 1
            out.extend("    " + code for code in METER)
        elif re.match(r"^\s+loop( |$)", line):
            indent = re.match(r"^\s*", line).group(0) + "  "
            out.extend(indent + code for code in METER)
    if globals_end is None:
        defined = max((n for n, line in enumerate(out) if re.match(r"^  \((?:import|type|table|memory)", line)), default=0) + 1
        globals_end = defined
    out.insert(globals_end, f"  (global {COUNTER} (mut i64) i64.const {INITIAL})")
    close = max(n for n, line in enumerate(out) if line == ")")
    added = [
        f'  (func $__fuel_set (param i64) local.get 0 global.set {COUNTER})',
        f'  (func $__fuel_get (result i64) global.get {COUNTER})',
        f'  (export "{SET_EXPORT}" (func $__fuel_set))',
        f'  (export "{GET_EXPORT}" (func $__fuel_get))',
    ]
    out[close:close] = added
    return "\n".join(out)


def instrument(wasm, wasm_tools=None):
    """The wasm bytes of the module with fuel metering added; FuelError if the input is already metered."""
    wasm_tools = find_wasm_tools(wasm_tools)
    text = tool(wasm_tools, ["print", "-"], wasm).decode()
    return tool(wasm_tools, ["parse", "-"], meter_text(text).encode())


def check_unmetered_surface(wasm, wasm_tools=None):
    """Reasons a metered module breaks the rule that the counter is neither imported nor exported and the two functions are exported."""
    text = tool(find_wasm_tools(wasm_tools), ["print", "-"], wasm).decode()
    problems = []
    for module, name in re.findall(r'^\s*\(import "([^"]*)" "([^"]*)" \(global', text, re.M):
        problems.append(f"imports the global {module}.{name}")
    exported = re.findall(r'^\s*\(export "([^"]*)"', text, re.M)
    for name in EXPORTS:
        if name not in exported:
            problems.append(f"does not export {name}")
    counter = re.search(r"i64\.sub\s+global\.set ([^\s()]+)", text)
    if counter is None:
        problems.append("has no metering code")
    else:
        for name, label in re.findall(r'^\s*\(export "([^"]*)" \(global ([^\s()]+)\)', text, re.M):
            if label == counter.group(1):
                problems.append(f"exports the counter as {name}")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("module")
    parser.add_argument("-o", "--output")
    parser.add_argument("--wasm-tools")
    parser.add_argument("--check", action="store_true", help="check a metered module's surface instead of metering it")
    args = parser.parse_args()
    with open(args.module, "rb") as f:
        data = f.read()
    try:
        if args.check:
            problems = check_unmetered_surface(data, args.wasm_tools)
            for problem in problems:
                print("FAIL", problem)
            return 1 if problems else 0
        if not args.output:
            parser.error("-o is needed")
        result = instrument(data, args.wasm_tools)
    except FuelError as e:
        sys.exit(str(e))
    with open(args.output, "wb") as f:
        f.write(result)
    print(f"metered {args.module}: {len(data)} -> {len(result)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
