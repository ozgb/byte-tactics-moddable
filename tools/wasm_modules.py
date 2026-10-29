"""Build and check the wasm modules: the C and Rust fixtures of tests/wasm/modules and wind_meteors.

    uv run tools/wasm_modules.py build     # build/wasm-modules/c.wasm, rust.wasm and wind_meteors.wasm
    uv run tools/wasm_modules.py check     # run both fixtures in one wasmtime memory and table
    uv run tools/wasm_modules.py imports   # check every built module's imports and exports
    uv run tools/wasm_modules.py statics   # run wind_meteors for 2000 ticks and check its statics never change

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

wind_meteors (wasm/modules.json) is the 3.1c wind and meteors source built the same way: clang
--target=wasm32 over src/map/meteors.cpp, src/game/game_state.cpp and src/game/console_commands.cpp
beside wasm/wind_meteors/module.cpp, linked with --gc-sections and no --allow-undefined, so only the
module's functions stay and every import is one the module code declares. It compiles against the
freestanding headers of wasm/include, not MSVC 5's. The build fails when the module's window (data,
bss and stack, as linked) leaves its manifest window or meets a fixture's, or when its table entries
leave the module's table range.

imports reads each built module's import and export sections with the pinned wasm-tools (found in
build/tools/bin, install it with `cargo install wasm-tools --version <pin> --locked --root
build/tools`, or on PATH). A game-logic module may import only env.memory, env.__indirect_function_table,
the imported globals (env.__sim_base, the block base the loader sets), the interface imports and engine services its manifest lists, and no WASI, clock, random, file or
network import; it must export exactly the manifest's slots and the data the loader fills.

statics runs wind_meteors under the pinned wasmtime package with the manifest's block layout: a memory of
memory_pages pages, __sim_base set to block_base, a Game and a SimGlobals zeroed in the block (with the
size, wind and meteor fields set so showers start within the run), the module's g_game, g_simGlobals and
g_extTdf written through its exports, and stubs for every import (GameRand and RandomInt from an LCG,
SpawnProjectile counting its calls, FindWeaponByName returning the first weapon of the Game, FatalError
raising). It calls InitMeteors, EnableMeteors and SetMeteorParams, then 2000 ticks of UpdateWind and
UpdateMeteors with Game::ticks incremented, and hashes the module's statics (its data and bss, from
data_base to the low end of its stack, as linked) after setup and after every tick. A module that keeps
state outside the block fails at the first tick whose hash differs. The field offsets come from
src/state/layout_asserts.cpp (Game) and wasm/wind_meteors/layout_asserts.h (SimGlobals).
"""

import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODULES = os.path.join(ROOT, "tests", "wasm", "modules")
GAME_MODULES = os.path.join(ROOT, "wasm", "modules.json")
OUT = os.path.join(ROOT, "build", "wasm-modules")
WASM_TOOLS = os.path.join(ROOT, "build", "tools", "bin", "wasm-tools")
ENV_IMPORTS = {("env", "memory"), ("env", "__indirect_function_table")}
FIXTURE_EXPORTS = ["module_data", "module_slot"]
LAYOUT_TYPES = {"Game", "WeaponDef", "Point16", "Vec3", "TdfFile", "TdfRecord", "TdfField"}
CXX_FLAGS = [
    "--target=wasm32", "-O2", "-std=gnu++14", "-w", "-fms-compatibility", "-fms-extensions", "-fms-compatibility-version=11.00",
    "-mms-bitfields", "-fshort-wchar", "-fno-strict-aliasing", "-fwrapv", "-fno-delete-null-pointer-checks", "-ffp-contract=off",
    "-mno-nontrapping-fptoint", "-fno-rtti", "-fno-exceptions", "-ffunction-sections", "-fdata-sections", "-nostdinc", "-nostdinc++",
]
C_FLAGS = ["--target=wasm32", "-O2", "-w", "-ffreestanding", "-fno-builtin", "-ffp-contract=off", "-mno-nontrapping-fptoint", "-fwrapv", "-fno-strict-aliasing",
           "-ffunction-sections", "-fdata-sections", "-nostdinc"]
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
    game = game_manifest()
    check_tools_game(game)
    for entry in game["modules"]:
        build_game_module(entry, game)


def game_manifest():
    with open(GAME_MODULES) as f:
        return json.load(f)


def find_wasm_tools(pin):
    path = os.environ.get("WASM_TOOLS") or (WASM_TOOLS if os.path.exists(WASM_TOOLS) else shutil.which("wasm-tools"))
    if not path:
        sys.exit(f"wasm-tools {pin} not found: cargo install wasm-tools --version {pin} --locked --root build/tools")
    found = tool_version([path, "--version"], r"wasm-tools (\S+)")
    if found != pin:
        sys.exit(f"wasm-tools is {found}, wasm/modules.json pins {pin}")
    return path


def leb(data, i, signed=False):
    result = shift = 0
    while True:
        byte = data[i]
        i += 1
        result |= (byte & 0x7F) << shift
        shift += 7
        if not byte & 0x80:
            break
    if signed and byte & 0x40:
        result -= 1 << shift
    return result, i


def name_at(data, i):
    size, i = leb(data, i)
    return data[i:i + size].decode(), i + size


def const_expr(data, i):
    if data[i] != 0x41:
        raise ValueError(f"offset expression starts with opcode {data[i]:#x}, not i32.const")
    value, i = leb(data, i + 1, True)
    if data[i] != 0x0B:
        raise ValueError("offset expression is not a single i32.const")
    return value, i + 1


def limits_at(data, i):
    flag = data[i]
    low, i = leb(data, i + 1)
    if flag & 1:
        _, i = leb(data, i)
    return low, i


def parse_module(data):
    """The facts of a built module the window check needs: its memory and table minimums, stack top, table entries and data segments."""
    if data[:8] != b"\0asm\1\0\0\0":
        raise ValueError("not a wasm module")
    info = {"memory_min": None, "table_min": None, "stack_top": None, "elements": [], "segments": []}
    i = 8
    while i < len(data):
        section, i = data[i], i + 1
        size, i = leb(data, i)
        body, end = i, i + size
        if section == 2:
            count, j = leb(data, body)
            for _ in range(count):
                _, j = name_at(data, j)
                _, j = name_at(data, j)
                kind, j = data[j], j + 1
                if kind == 0:
                    _, j = leb(data, j)
                elif kind == 1:
                    info["table_min"], j = limits_at(data, j + 1)
                elif kind == 2:
                    info["memory_min"], j = limits_at(data, j)
                else:
                    j += 2
        elif section == 6:
            count, j = leb(data, body)
            for index in range(count):
                j += 2
                if data[j] == 0x41 and index == 0:
                    info["stack_top"], j = const_expr(data, j)
                else:
                    while data[j] != 0x0B:
                        j += 1
                    j += 1
        elif section == 9:
            count, j = leb(data, body)
            for _ in range(count):
                flag, j = leb(data, j)
                if flag == 2:
                    _, j = leb(data, j)
                if flag not in (0, 2):
                    raise ValueError(f"element segment of kind {flag}")
                offset, j = const_expr(data, j)
                if flag == 2:
                    j += 1
                funcs, j = leb(data, j)
                for _ in range(funcs):
                    _, j = leb(data, j)
                info["elements"].append((offset, funcs))
        elif section == 11:
            count, j = leb(data, body)
            for _ in range(count):
                flag, j = leb(data, j)
                if flag == 2:
                    _, j = leb(data, j)
                if flag not in (0, 2):
                    raise ValueError(f"data segment of kind {flag}")
                offset, j = const_expr(data, j)
                length, j = leb(data, j)
                info["segments"].append((offset, length))
                j += length
        i = end
    return info


def check_window(entry, pins, info, others):
    """Reasons the module's linked window or table entries break its manifest or collide with another module's."""
    problems = []
    low, window = entry["data_base"], pins["window"]
    high = low + window
    extents = [offset + length for offset, length in info["segments"]]
    top = max(extents + [info["stack_top"]])
    first = min([offset for offset, _ in info["segments"]] + [info["stack_top"]])
    if first < low or top > high:
        problems.append(f"{entry['name']}: data and stack span {first:#x}..{top:#x}, outside its window {low:#x}..{high:#x}")
    if info["memory_min"] is not None and info["memory_min"] > pins["memory_pages"]:
        problems.append(f"{entry['name']}: needs {info['memory_min']} pages, the manifest has {pins['memory_pages']}")
    t_low, t_high = entry["table_base"], entry["table_base"] + pins["table_slots"]
    for offset, count in info["elements"]:
        if offset < t_low or offset + count > t_high:
            problems.append(f"{entry['name']}: table entries {offset}..{offset + count} leave its range {t_low}..{t_high}")
    if t_high > pins["table_size"]:
        problems.append(f"{entry['name']}: table range ends at {t_high}, past the table size {pins['table_size']}")
    if high > pins["memory_pages"] * 65536:
        problems.append(f"{entry['name']}: window ends at {high:#x}, past {pins['memory_pages']} pages")
    for other_name, other_low, other_high, other_t_low, other_t_high in others:
        if low < other_high and other_low < high:
            problems.append(f"{entry['name']}: window {low:#x}..{high:#x} overlaps {other_name}'s {other_low:#x}..{other_high:#x}")
        if t_low < other_t_high and other_t_low < t_high:
            problems.append(f"{entry['name']}: table range {t_low}..{t_high} overlaps {other_name}'s {other_t_low}..{other_t_high}")
    return problems


def check_layout(pins, ranges):
    """Reasons the module ranges (name, low, high) and the scratch range overlap each other or reach the block, or the pages do not hold the block."""
    problems = []
    base, size = pins["block_base"], pins["block_size"]
    scratch_low, scratch_size = pins["scratch"]
    named = list(ranges) + [("scratch", scratch_low, scratch_low + scratch_size)]
    for name, low, high in named:
        if high > base:
            problems.append(f"{name}: {low:#x}..{high:#x} reaches the block base {base:#x}")
    for index, (name, low, high) in enumerate(named):
        for other_name, other_low, other_high in named[index + 1:]:
            if low < other_high and other_low < high:
                problems.append(f"{name}: {low:#x}..{high:#x} overlaps {other_name}'s {other_low:#x}..{other_high:#x}")
    if pins["memory_pages"] * 65536 != base + size:
        problems.append(f"memory_pages {pins['memory_pages']} does not hold the block: {base + size:#x} bytes needed")
    return problems


def fixture_ranges():
    pins = manifest()
    return [(m["name"], m["data_base"], m["data_base"] + pins["window"], m["table_base"], m["table_base"] + pins["table_slots"])
            for m in pins["modules"]]


def generated_layout_asserts(path):
    """The Game, WeaponDef, Vec3 and TDF layout asserts of src/state/layout_asserts.cpp, which the i686 build compiles."""
    with open(os.path.join(ROOT, "src", "state", "layout_asserts.cpp")) as f:
        lines = f.read().split("\n")
    keep = []
    for line in lines:
        match = re.match(r"static_assert\((?:sizeof|offsetof)\((\w+)", line)
        if match and match.group(1) in LAYOUT_TYPES:
            keep.append(line)
    if len(keep) < 100:
        sys.exit(f"src/state/layout_asserts.cpp has only {len(keep)} asserts for {sorted(LAYOUT_TYPES)}")
    with open(path, "w") as f:
        f.write("\n".join(keep) + "\n")


def run(cmd, **kwargs):
    result = subprocess.run(cmd, capture_output=True, text=True, **kwargs)
    if result.returncode:
        sys.stderr.write(result.stdout + result.stderr)
        sys.exit(f"failed: {' '.join(cmd[:3])} ...")


def build_game_module(entry, pins):
    out = os.path.join(OUT, entry["name"])
    os.makedirs(out, exist_ok=True)
    generated_layout_asserts(os.path.join(out, "layout_generated.h"))
    tables = os.path.join(out, "bx_tables.c")
    generator = os.path.join(ROOT, "tools", "bitexact_tables.py")
    if not os.path.exists(tables) or os.path.getmtime(tables) < os.path.getmtime(generator):
        run([sys.executable, generator, tables])
    jobs = []
    includes = [f"-I{os.path.join(ROOT, d)}" for d in ("wasm/include", "include", ".")] + [f"-I{out}"]
    for index, source in enumerate(entry["cxx"]):
        obj = os.path.join(out, f"{index}_{os.path.basename(source)}.o")
        jobs.append((["clang++", *CXX_FLAGS, *includes, "-c", os.path.join(ROOT, source), "-o", obj], obj))
    c_includes = [f"-I{os.path.join(ROOT, d)}" for d in ("wasm/include", "lib/bitexact")]
    for index, source in enumerate(entry["c"] + [tables]):
        path = source if os.path.isabs(source) else os.path.join(ROOT, source)
        obj = os.path.join(out, f"c{index}_{os.path.basename(source)}.o")
        jobs.append((["clang", *C_FLAGS, *c_includes, "-c", path, "-o", obj], obj))
    with concurrent.futures.ThreadPoolExecutor() as pool:
        list(pool.map(lambda job: run(job[0]), jobs))
    target = os.path.join(OUT, entry["name"] + ".wasm")
    exports = [f"--export={name}" for name in entry["data"]]
    cmd = ["wasm-ld", *link_flags(entry, pins["stack_size"]), "--gc-sections", *exports, *[obj for _, obj in jobs], "-o", target]
    run(cmd)
    with open(target, "rb") as f:
        info = parse_module(f.read())
    others = [r for r in fixture_ranges()]
    problems = check_window(entry, pins, info, others)
    linked = (min(o for o, _ in info["segments"] + [(info["stack_top"], 0)]),
              max(o + n for o, n in info["segments"] + [(info["stack_top"], 0)]))
    ranges = [(name, low, high) for name, low, high, _, _ in others] + [(entry["name"], *linked)]
    problems += check_layout(pins, ranges)
    if problems:
        sys.exit("\n".join(problems))
    low = min(offset for offset, _ in info["segments"])
    print(f"built {target}: data {low:#x}..{max(o + n for o, n in info['segments']):#x}, stack top {info['stack_top']:#x}, "
          f"window {entry['data_base']:#x}..{entry['data_base'] + pins['window']:#x}, "
          f"table {min(o for o, _ in info['elements'])}..{max(o + n for o, n in info['elements'])}")


def check_tools_game(pins):
    for name in ("clang", "wasm-ld"):
        found = tool_version([name, "--version"], r"(?:clang version|LLD) (\S+)")
        if found != pins["tools"]["clang"]:
            sys.exit(f"{name} is {found}, wasm/modules.json pins {pins['tools']['clang']}")


def check_interface(name, imports, exports, spec):
    """Reasons a module's imports or exports are outside its allowlist; imports and exports are lists of names (module, name) and name."""
    problems = []
    allowed = set(ENV_IMPORTS)
    allowed |= {("env", n) for n in spec.get("globals", [])}
    for module, names in spec.get("imports", {}).items():
        if module.startswith("wasi") or module == "env":
            problems.append(f"{name}: the manifest lists imports from {module}")
        allowed |= {(module, n) for n in names}
    for module, field in imports:
        if module.startswith("wasi"):
            problems.append(f"{name}: imports {module}.{field}, a WASI call")
        elif (module, field) not in allowed:
            problems.append(f"{name}: imports {module}.{field}, which its manifest does not list")
    for module, field in sorted(allowed - set(imports)):
        problems.append(f"{name}: the manifest lists {module}.{field}, which the module does not import")
    if sorted(exports) != sorted(spec["exports"]):
        extra, missing = sorted(set(exports) - set(spec["exports"])), sorted(set(spec["exports"]) - set(exports))
        problems.append(f"{name}: exports differ from the manifest: extra {extra}, missing {missing}")
    return problems


def module_interface(wasm_tools, path):
    text = subprocess.run([wasm_tools, "print", path], capture_output=True, text=True, check=True).stdout
    imports = re.findall(r'^\s*\(import "([^"]*)" "([^"]*)"', text, re.M)
    exports = re.findall(r'^\s*\(export "([^"]*)"', text, re.M)
    return imports, exports


def run_imports():
    game = game_manifest()
    wasm_tools = find_wasm_tools(game["tools"]["wasm-tools"])
    specs = {m["name"]: {"imports": m["imports"], "exports": m["exports"] + m["data"], "globals": m.get("globals", [])} for m in game["modules"]}
    for entry in manifest()["modules"]:
        specs[entry["name"]] = {"imports": {}, "exports": FIXTURE_EXPORTS}
    failures = []
    for name, spec in specs.items():
        path = os.path.join(OUT, name + ".wasm")
        if not os.path.exists(path):
            sys.exit(f"{path} is missing: build first: uv run tools/wasm_modules.py build")
        imports, exports = module_interface(wasm_tools, path)
        problems = check_interface(name, imports, exports, spec)
        failures += problems
        print(f"{name}: {len(imports)} imports, {len(exports)} exports" + (": FAIL" if problems else ""))
    for failure in failures:
        print("FAIL", failure)
    if failures:
        return 1
    print("ok")
    return 0


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
        WINDOW = pins["window"]
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


STATICS_TICKS = 2000
STATICS_GAME_AT = 0x1000
STATICS_SIM_AT = 0x1000 + 0x40000


def layout_offsets(wanted):
    """The offsets {(type, field): offset} of the wanted (type, field) pairs from the layout assert files."""
    found = {}
    for path in ("src/state/layout_asserts.cpp", "wasm/wind_meteors/layout_asserts.h"):
        with open(os.path.join(ROOT, path)) as f:
            for kind, field, value in re.findall(r"static_assert\(offsetof\((\w+), (\w+)\) == (\d+)", f.read()):
                if (kind, field) in wanted:
                    found[(kind, field)] = int(value)
    missing = sorted(set(wanted) - set(found))
    if missing:
        sys.exit(f"no layout assert for {missing}")
    return found


def statics_run():
    import wasmtime

    pins = game_manifest()
    entry = pins["modules"][0]
    path = os.path.join(OUT, entry["name"] + ".wasm")
    if not os.path.exists(path):
        sys.exit(f"{path} is missing: build first: uv run tools/wasm_modules.py build")
    with open(path, "rb") as f:
        info = parse_module(f.read())
    statics = (entry["data_base"], info["stack_top"] - pins["stack_size"])
    base, scratch = pins["block_base"], pins["scratch"][0]
    game_at, sim_at = base + STATICS_GAME_AT, base + STATICS_SIM_AT
    fields = [("Game", n) for n in ("ticks", "width", "height", "weapons", "windSpeedMin", "windSpeedMax", "windSpeedDenominator")]
    fields += [("SimGlobals", "g_meteorWeaponName"), ("SimGlobals", "g_meteorsEnabled")]
    off = layout_offsets(set(fields))

    store = wasmtime.Store()
    memory = wasmtime.Memory(store, wasmtime.MemoryType(wasmtime.Limits(pins["memory_pages"], pins["memory_pages"])))
    table = wasmtime.Table(store, wasmtime.TableType(wasmtime.ValType.funcref(),
                                                      wasmtime.Limits(pins["table_size"], pins["table_size"])), None)
    state = {"lcg": 0x2545F491, "spawns": 0}

    def rand32():
        state["lcg"] = (state["lcg"] * 1103515245 + 12345) & 0x7FFFFFFF
        return state["lcg"]

    def game_rand(_stream):
        return (rand32() >> 8) & 0x7FFF

    def random_int(count):
        return rand32() % count if count > 0 else 0

    def spawn_projectile(_weapon, _pos, _vel, _flag):
        state["spawns"] += 1
        return 1

    def fatal_error(_message):
        raise RuntimeError("the module called FatalError")

    stubs = {("services", "GameRand"): game_rand, ("services", "RandomInt"): random_int,
             ("weapons", "SpawnProjectile"): spawn_projectile,
             ("weapons", "FindWeaponByName"): lambda _name: game_at + off[("Game", "weapons")],
             ("services", "FatalError"): fatal_error}

    def unexpected(name):
        def stub(*_args):
            raise RuntimeError(f"the run called {name}, which it should not reach")
        return stub

    module = wasmtime.Module.from_file(store.engine, path)
    linker = wasmtime.Linker(store.engine)
    linker.define(store, "env", "memory", memory)
    linker.define(store, "env", "__indirect_function_table", table)
    linker.define(store, "env", "__sim_base", wasmtime.Global(store, wasmtime.GlobalType(wasmtime.ValType.i32(), False),
                                                              wasmtime.Val.i32(base)))
    for imp in module.imports:
        if isinstance(imp.type, wasmtime.FuncType):
            key = (imp.module, imp.name)
            linker.define(store, imp.module, imp.name, wasmtime.Func(store, imp.type, stubs.get(key, unexpected(f"{key[0]}.{key[1]}"))))
    instance = linker.instantiate(store, module)
    exports = instance.exports(store)

    def put(address, value, size=4):
        memory.write(store, (value & ((1 << (8 * size)) - 1)).to_bytes(size, "little"), address)

    def get(address):
        return int.from_bytes(memory.read(store, address, address + 4), "little")

    put(exports["g_game"].value(store), game_at)
    put(exports["g_simGlobals"].value(store), sim_at)
    memory.write(store, b"tdf\0", exports["g_extTdf"].value(store))
    put(game_at + off[("Game", "width")], 4096)
    put(game_at + off[("Game", "height")], 4096)
    put(game_at + off[("Game", "windSpeedMin")], 0)
    put(game_at + off[("Game", "windSpeedMax")], 30)
    put(game_at + off[("Game", "windSpeedDenominator")], 30)

    params = scratch
    memory.write(store, b"Meteor\0".ljust(32, b"\0"), params)
    memory.write(store, (50).to_bytes(4, "little") + struct.pack("<fff", 0.5, 2.0, 3.0), params + 32)

    def call(name, *args):
        exports[name](store, *args)

    def digest():
        return hashlib.sha256(memory.read(store, statics[0], statics[1])).hexdigest()

    call("InitMeteors")
    call("EnableMeteors")
    call("SetMeteorParams", params)
    if get(sim_at + off[("SimGlobals", "g_meteorsEnabled")]) != 1:
        sys.exit("EnableMeteors did not set g_meteorsEnabled in the block")
    setup = digest()
    ticks = off[("Game", "ticks")]
    for tick in range(1, STATICS_TICKS + 1):
        put(game_at + ticks, tick)
        call("UpdateWind")
        call("UpdateMeteors")
        if digest() != setup:
            print(f"FAIL: the statics {statics[0]:#x}..{statics[1]:#x} of {entry['name']} changed at tick {tick}")
            return 1
    if state["spawns"] == 0:
        print("FAIL: SpawnProjectile was never called, so no meteor ran")
        return 1
    print(f"statics: {STATICS_TICKS} ticks, {state['spawns']} SpawnProjectile calls, {statics[1] - statics[0]} bytes hashed, ok")
    return 0


def run_under_wasmtime(command):
    try:
        import wasmtime
    except ImportError:
        if os.environ.get(GUARD):
            raise
        pin = manifest()["tools"]["wasmtime"]
        env = dict(os.environ, **{GUARD: "1"})
        cmd = ["uv", "run", "--no-project", "--with", f"wasmtime=={pin}", "python", "-I", os.path.abspath(__file__), command]
        return subprocess.run(cmd, env=env).returncode
    return statics_run() if command == "statics" else -1


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
    parser.add_argument("command", choices=["build", "check", "imports", "statics"])
    args = parser.parse_args()
    if args.command == "build":
        build()
        return 0
    if args.command == "imports":
        return run_imports()
    if args.command == "statics":
        return run_under_wasmtime("statics")
    return check()


if __name__ == "__main__":
    sys.exit(main())
