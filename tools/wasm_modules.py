"""Build and check the C and Rust wasm module fixtures of tests/wasm/modules.

    uv run tools/wasm_modules.py build     # build/wasm-modules/c.wasm and rust.wasm
    uv run tools/wasm_modules.py check     # run both in one wasmtime memory and table

Both fixtures use the fixed-base convention (#562): each imports env.memory and
env.__indirect_function_table, is linked with its own --global-base and --table-base
from tests/wasm/modules/modules.json, and needs no relocation at load. The C module is
built by clang and wasm-ld, the Rust module by cargo (no_std, wasm32-unknown-unknown),
each with the tool versions that file pins. The build fails on any other version.

check instantiates both modules into one memory and one table, each export taking the
imports and nothing else, then looks up each module's slot through the shared table and
calls it. It fails unless the slot sits at the module's table base, the module's data
lies in its own window from its data base, the call returns the manifest's value, and
the data of both modules is unchanged after both ran. check runs under the pinned
wasmtime package, which it fetches with uv. Standard library only apart from that.
"""

import argparse
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODULES = os.path.join(ROOT, "tests", "wasm", "modules")
OUT = os.path.join(ROOT, "build", "wasm-modules")
WINDOW = 65536
GUARD = "WASM_MODULES_UNDER_WASMTIME"


def manifest():
    with open(os.path.join(MODULES, "modules.json")) as f:
        return json.load(f)


def tool_version(argv, pattern):
    text = subprocess.run(argv, capture_output=True, text=True, check=True).stdout
    return re.search(pattern, text).group(1)


def check_tools(pins):
    found = {
        "clang": tool_version(["clang", "--version"], r"clang version (\S+)"),
        "rustc": tool_version(["rustc", "--version"], r"rustc (\S+)"),
        "wasm-ld": tool_version(["wasm-ld", "--version"], r"LLD (\S+)"),
    }
    for name, want in (("clang", pins["clang"]), ("rustc", pins["rustc"]), ("wasm-ld", pins["clang"])):
        if found[name] != want:
            sys.exit(f"{name} is {found[name]}, the manifest pins {want}")


def link_flags(entry, stack_size):
    return [
        "--import-memory",
        "--import-table",
        f"--global-base={entry['data_base']}",
        f"--table-base={entry['table_base']}",
        "--no-stack-first",
        "--no-entry",
        f"-zstack-size={stack_size}",
    ]


def build_c(entry, stack_size):
    obj = os.path.join(OUT, "c.o")
    src = os.path.join(MODULES, "c", "slot.c")
    subprocess.run(["clang", "--target=wasm32", "-O2", "-nostdlib", "-c", src, "-o", obj], check=True)
    cmd = ["wasm-ld", *link_flags(entry, stack_size), "--export=module_data", "--export=module_slot", obj,
           "-o", os.path.join(OUT, "c.wasm")]
    subprocess.run(cmd, check=True)


def build_rust(entry, stack_size):
    target_dir = os.path.join(OUT, "rust-target")
    env = dict(os.environ, CARGO_TARGET_DIR=target_dir)
    link_args = [arg for flag in link_flags(entry, stack_size) for arg in ("-C", f"link-arg={flag}")]
    cmd = ["cargo", "rustc", "--release", "--locked", "--offline", "--target", "wasm32-unknown-unknown",
           "--manifest-path", os.path.join(MODULES, "rust", "Cargo.toml"), "--", *link_args]
    subprocess.run(cmd, check=True, env=env)
    built = os.path.join(target_dir, "wasm32-unknown-unknown", "release", "slot.wasm")
    with open(built, "rb") as src, open(os.path.join(OUT, "rust.wasm"), "wb") as dst:
        dst.write(src.read())


def build():
    pins = manifest()
    check_tools(pins["tools"])
    os.makedirs(OUT, exist_ok=True)
    for entry in pins["modules"]:
        {"c": build_c, "rust": build_rust}[entry["name"]](entry, pins["stack_size"])
        print(f"built {os.path.join(OUT, entry['name'] + '.wasm')}")


def read_i32(memory, store, address):
    return int.from_bytes(memory.read(store, address, address + 4), "little")


def run_check():
    import wasmtime

    pins = manifest()
    store = wasmtime.Store()
    memory = wasmtime.Memory(store, wasmtime.MemoryType(wasmtime.Limits(pins["memory_pages"], pins["memory_pages"])))
    table = wasmtime.Table(store, wasmtime.TableType(wasmtime.ValType.funcref(),
                                                      wasmtime.Limits(pins["table_size"], pins["table_size"])), None)
    linker = wasmtime.Linker(store.engine)
    linker.define(store, "env", "memory", memory)
    linker.define(store, "env", "__indirect_function_table", table)

    failures = []
    instances = {}
    for entry in pins["modules"]:
        name = entry["name"]
        module = wasmtime.Module.from_file(store.engine, os.path.join(OUT, name + ".wasm"))
        imports = sorted(f"{i.module}.{i.name}" for i in module.imports)
        if imports != ["env.__indirect_function_table", "env.memory"]:
            failures.append(f"{name}: imports are {imports}")
        instances[name] = linker.instantiate(store, module)

    for entry in pins["modules"]:
        name = entry["name"]
        exports = instances[name].exports(store)
        data = exports["module_data"](store)
        slot_cell = exports["module_slot"](store)
        index = read_i32(memory, store, slot_cell)
        low = entry["data_base"]
        if not low <= data < low + WINDOW or not low <= slot_cell < low + WINDOW:
            failures.append(f"{name}: data {data:#x} or slot cell {slot_cell:#x} outside {low:#x}+{WINDOW:#x}")
        if index != entry["table_base"]:
            failures.append(f"{name}: slot at table index {index}, table base is {entry['table_base']}")
        func = table.get(store, index)
        if func is None:
            failures.append(f"{name}: table index {index} is empty")
            continue
        result = func(store, entry["slot_arg"])
        print(f"{name}: data at {data:#x}, slot at table index {index}, slot({entry['slot_arg']}) = {result}")
        if result != entry["slot_result"]:
            failures.append(f"{name}: slot returned {result}, expected {entry['slot_result']}")

    for entry in pins["modules"]:
        name = entry["name"]
        data = instances[name].exports(store)["module_data"](store)
        word = read_i32(memory, store, data)
        if word != entry["data_word"]:
            failures.append(f"{name}: data word is {word} after both ran, expected {entry['data_word']}")

    for failure in failures:
        print("FAIL", failure)
    if failures:
        return 1
    print("ok: both modules ran their slot through one table, data intact")
    return 0


def check():
    try:
        import wasmtime
    except ImportError:
        if os.environ.get(GUARD):
            raise
        pin = manifest()["tools"]["wasmtime"]
        env = dict(os.environ, **{GUARD: "1"})
        cmd = ["uv", "run", "--no-project", "--with", f"wasmtime=={pin}", "python", "-I", os.path.abspath(__file__),
               "check"]
        return subprocess.run(cmd, env=env).returncode
    if not os.path.exists(os.path.join(OUT, "c.wasm")):
        sys.exit("build first: uv run tools/wasm_modules.py build")
    return run_check()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("command", choices=["build", "check"])
    args = parser.parse_args()
    if args.command == "build":
        build()
        return 0
    return check()


if __name__ == "__main__":
    sys.exit(main())
