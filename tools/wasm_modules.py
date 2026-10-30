"""Build and check the wasm modules: the C and Rust fixtures of tests/wasm/modules and wind_meteors.

    uv run tools/wasm_modules.py build     # build/wasm-modules/c.wasm, rust.wasm and wind_meteors.wasm
    uv run tools/wasm_modules.py check     # run both fixtures in one wasmtime memory and table
    uv run tools/wasm_modules.py imports   # check every built module's imports and exports
    uv run tools/wasm_modules.py statics   # run wind_meteors for 2000 ticks and check its statics never change
    uv run tools/wasm_modules.py adapters  # run the loader's adapters and refusals over tests/wasm/adapters
    uv run tools/wasm_modules.py fatal     # make an import end the game, and check the exit comes after the call
    uv run tools/wasm_modules.py glue-fatal [--exe EXE] [--game-dir DIR]   # the same through the real glue, under Wine

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
the interface imports and engine services its manifest lists, and no global, WASI, clock, random, file or
network import; it must export exactly the manifest's slots and the data the loader fills. The manifest records each import's
parameter and result kinds (int, f64, void, linear, block, scratch, handle) and each slot's parameter kinds; imports checks them
against the wasm signatures: the same count, f64 only where the wasm type is f64, and no other kind where it is.

statics runs wind_meteors under the pinned wasmtime package with the manifest's block layout: a memory of
memory_pages pages, a Game and a SimGlobals zeroed in the block (with the
size, wind and meteor fields set so showers start within the run), the module's g_game, g_simGlobals,
g_extTdf and g_blockBase (block_base) written through its exports, and stubs for every import (GameRand and RandomInt from an LCG,
SpawnProjectile counting its calls, FindWeaponByName returning the first weapon of the Game, FatalError
raising). It calls InitMeteors, EnableMeteors and SetMeteorParams, then 2000 ticks of UpdateWind and
UpdateMeteors with Game::ticks incremented, and hashes the module's statics (its data and bss, from
data_base to the low end of its stack, as linked) after setup and after every tick. A module that keeps
state outside the block fails at the first tick whose hash differs. The field offsets come from
src/state/layout_asserts.cpp (Game) and wasm/wind_meteors/layout_asserts.h (SimGlobals).

Adapters (#566). When a module was built against an older major of an interface, the loader (lib/wasmloader)
fills each slot and import whose signature changed through an adapter. The manifest holds "signatures"
({interface: {name: [[kinds], result]}}, the current kinds of every slot and import) and "adapters", a list of
{"interface", "from_major", "name", "args", "result"} (name is a slot of the module's own interface, or an import of
one of its import groups). The caller makes the call (the game for a slot, the module for an import) and the callee
receives the converted call (the module for a slot, the host for an import). "args" has one entry per parameter of
the callee, each the index of a caller parameter of the same kind, or {"const": n} for an int parameter. "result"
is "pass" (the callee's result is the caller's, of the same kind), "void" (the caller has no result) or
{"const": n} (the caller's int result). Only reorders, drops and added constants fit; a conversion that needs
code (linear and handle, int and f64) is refused by the loader. build writes "signatures" from the modules built
against the current major; check fails when an older module has a changed slot or import with no adapter.

adapters builds lib/wasmloader with clang for the build machine, together with tests/wasm/adapters/adapters.c, and runs it
against the libwasmtime that the pinned wasmtime package ships (fetched with uv). tests/wasm/adapters holds a manifest of
a test interface calc (the module's slots, major 2) and calc_host (imports, major 3), and two modules in text format
built with wasm-tools: current, built against those majors, and old, built against calc 1 and calc_host 2, which
the manifest's adapters bring to the current signatures with each form (reorder, drop, constant argument, constant
result, no result). The program loads old, then current, and checks every slot and import of both, and then checks the
loader's refusals: a changed slot or import without an adapter, and each adapter its kinds rule out.

Interface majors (#566). wasm/modules.json records the major of every interfaces/*.h that has version macros under
"majors", and for each module the interface its slots belong to ("interface") and the major of each interface it
exports slots for or imports from, the one it was built against ("majors"). The import groups of a module are
header stems (weapons, services). build writes all of it from the headers, because an in-repo module is built
against the current headers; check fails when the top-level majors differ from the headers, when a module's
majors name an interface it does not use, or are missing one it does, or are ahead of the headers.
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

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import wasm_fuel

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
    "-fno-rtti", "-fno-exceptions", "-ffunction-sections", "-fdata-sections", "-nostdinc", "-nostdinc++",
]
C_FLAGS = ["--target=wasm32", "-O2", "-w", "-ffreestanding", "-fno-builtin", "-ffp-contract=off", "-fwrapv", "-fno-strict-aliasing",
           "-ffunction-sections", "-fdata-sections", "-nostdinc"]
KINDS = {"int", "f64", "linear", "block", "scratch", "handle"}
INTERFACES = os.path.join(ROOT, "interfaces")
MAJOR_MACRO = re.compile(r"^\s*#\s*define\s+(\w+)_VERSION_MAJOR\s+(\d+)", re.M)
EXPANDED = {(), ("modules",), ("modules", "*"), ("modules", "*", "slots"), ("modules", "*", "imports"), ("modules", "*", "imports", "*"),
            ("modules", "*", "majors"), ("majors",), ("signatures",), ("signatures", "*")}
WRAP = 132
ADAPTERS = os.path.join(ROOT, "tests", "wasm", "adapters")
LOADER = os.path.join(ROOT, "lib", "wasmloader")
WASMTIME_INCLUDE = os.path.join(ROOT, "third_party", "wasmtime", "include")
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
    write_game_manifest(with_majors(game, header_majors()))


def game_manifest():
    with open(GAME_MODULES) as f:
        return json.load(f)


def header_majors():
    """{interface: major} of every interfaces/*.h that has version macros, the interface being the file stem."""
    found = {}
    for name in sorted(os.listdir(INTERFACES)):
        if not name.endswith(".h"):
            continue
        stem = name[:-2]
        with open(os.path.join(INTERFACES, name)) as f:
            for prefix, major in MAJOR_MACRO.findall(f.read()):
                if prefix != stem.upper():
                    sys.exit(f"interfaces/{name}: the version macro prefix {prefix} is not {stem.upper()}")
                found[stem] = int(major)
    return found


def module_interfaces(entry):
    """The interfaces a game module uses: the one its slots belong to, then the import groups."""
    return [entry["interface"], *entry["imports"]]


def with_majors(game, majors):
    """The manifest with its interface majors set from the headers: an in-repo module is built against them."""
    out = {}
    for key, value in game.items():
        if key in ("majors", "signatures", "adapters"):
            continue
        if key == "modules":
            value = [with_module_majors(entry, majors) for entry in value]
            out["majors"] = dict(majors)
            out["signatures"] = current_signatures(game, value, majors)
            out["adapters"] = list(game.get("adapters", []))
        out[key] = value
    return out


def module_table(entry, interface):
    """{name: signature} of the slots (the module's own interface) or the imports (a group) of a module for an interface."""
    return entry["slots"] if interface == entry["interface"] else entry["imports"].get(interface, {})


def current_signatures(game, modules, majors):
    """The signatures of the manifest: the old ones, with those of every module built against the current major over them."""
    out = {interface: dict(names) for interface, names in game.get("signatures", {}).items()}
    seen = {}
    for entry in modules:
        for interface in module_interfaces(entry):
            if entry["majors"][interface] != majors[interface]:
                continue
            for name, signature in module_table(entry, interface).items():
                if seen.setdefault((interface, name), (entry["name"], signature))[1] != signature:
                    sys.exit(f"{interface}.{name}: {seen[(interface, name)][0]} and {entry['name']} are both built against the current major "
                             "and disagree on its signature")
                out.setdefault(interface, {})[name] = signature
    return {interface: out[interface] for interface in sorted(out)}


def with_module_majors(entry, majors):
    out = {}
    for key, value in entry.items():
        if key == "majors":
            continue
        out[key] = value
        if key == "interface":
            missing = [name for name in module_interfaces(entry) if name not in majors]
            if missing:
                sys.exit(f"{entry['name']}: no interfaces/<name>.h with version macros for {missing}")
            out["majors"] = {name: majors[name] for name in module_interfaces(entry)}
    return out


def dump_manifest(value, path=(), indent=0, key=None):
    """The manifest as text: the layout of wasm/modules.json, one key per line down to the slots and imports, lists inline and wrapped."""
    generic = tuple("*" if (i == 1 and path[0] in ("modules", "signatures")) or (i == 3 and path[2:3] == ("imports",)) else part
                    for i, part in enumerate(path))
    pad = " " * (indent + 2)
    if isinstance(value, dict) and generic in EXPANDED:
        lines = [f'{pad}"{k}": {dump_manifest(v, path + (k,), indent + 2, k)}' for k, v in value.items()]
        return "{\n" + ",\n".join(lines) + "\n" + " " * indent + "}"
    if isinstance(value, dict):
        return "{" + ", ".join(f'"{k}": {dump_manifest(v, path + (k,), indent, k)}' for k, v in value.items()) + "}"
    if isinstance(value, list) and path in (("modules",), ("adapters",)) and value:
        return "[\n" + ",\n".join(pad + dump_manifest(v, path + (i,), indent + 2) for i, v in enumerate(value)) + "\n" + " " * indent + "]"
    if isinstance(value, list):
        items = [dump_manifest(v, path + (i,), indent) for i, v in enumerate(value)]
        head = " " * indent + (f'"{key}": ' if key else "")
        text = "[" + ", ".join(items) + "]"
        if len(head) + len(text) <= WRAP or not all(i.startswith('"') for i in items):
            return text
        column = len(head) + 1
        lines, current = [], ""
        for item in items:
            if current and column + len(current) + len(item) + 1 > WRAP:
                lines.append(current.rstrip())
                current = ""
            current += item + ", "
        lines.append(current[:-2])
        return "[" + ("\n" + " " * column).join(lines) + "]"
    return json.dumps(value)


def manifest_text(game):
    return dump_manifest(game) + "\n"


def write_game_manifest(game):
    """Write game to wasm/modules.json when its text differs from the file's."""
    text = manifest_text(game)
    with open(GAME_MODULES) as f:
        if f.read() == text:
            return
    with open(GAME_MODULES, "w") as f:
        f.write(text)
    print(f"wrote {os.path.relpath(GAME_MODULES, ROOT)}")


def check_majors(game, majors):
    """Reasons the manifest's majors, signatures and adapters disagree with the headers or with each other."""
    problems = []
    if game.get("majors") != majors:
        problems.append(f"majors differ from interfaces/*.h: {game.get('majors')} against {majors}: uv run tools/wasm_modules.py build")
    signatures = game.get("signatures", {})
    adapters = {(a.get("interface"), a.get("from_major"), a.get("name")) for a in game.get("adapters", [])}
    for adapter in game.get("adapters", []):
        interface, name = adapter.get("interface"), adapter.get("name")
        label = f"adapter {interface}.{name} from major {adapter.get('from_major')}"
        if interface not in majors:
            problems.append(f"{label}: no such interface")
        elif not isinstance(adapter.get("from_major"), int) or not 0 < adapter["from_major"] < majors[interface]:
            problems.append(f"{label}: from_major must be older than the current major {majors[interface]}")
        if name not in signatures.get(interface, {}):
            problems.append(f"{label}: the manifest's signatures have no {name} for {interface}")
    for entry in game["modules"]:
        name, built = entry["name"], entry.get("majors", {})
        used = module_interfaces(entry) if "interface" in entry else []
        for interface in used:
            if interface not in built:
                problems.append(f"{name}: no major for {interface}, an interface it uses")
            elif interface in majors and built[interface] > majors[interface]:
                problems.append(f"{name}: built against {interface} {built[interface]}, the headers have {majors[interface]}")
            elif interface in majors:
                problems += check_signatures(name, entry, interface, built[interface], majors[interface], signatures, adapters)
        for interface in sorted(set(built) - set(used)):
            problems.append(f"{name}: has a major for {interface}, which it neither exports slots for nor imports from")
    return problems


def check_signatures(name, entry, interface, built, current, signatures, adapters):
    """Reasons a module's slots or imports of an interface differ from the current signatures without an adapter that covers the difference."""
    problems = []
    for item, signature in module_table(entry, interface).items():
        if item not in signatures.get(interface, {}):
            problems.append(f"{name}: the manifest's signatures have no {item} for {interface}")
        elif signatures[interface][item] == signature:
            continue
        elif built == current:
            problems.append(f"{name}: {interface}.{item} is {signature} but the current signature is {signatures[interface][item]}")
        elif (interface, built, item) not in adapters:
            problems.append(f"{name}: {interface}.{item} changed between major {built} and {current}, and the manifest has no adapter for it")
    return problems


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
    for name, low, high in ranges:
        if high > scratch_low:
            problems.append(f"{name}: {low:#x}..{high:#x} reaches the scratch base {scratch_low:#x}")
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
    build_flags = pins["build_flags"]
    jobs = []
    includes = [f"-I{os.path.join(ROOT, d)}" for d in ("wasm/include", "include", ".")] + [f"-I{out}"]
    for index, source in enumerate(entry["cxx"]):
        obj = os.path.join(out, f"{index}_{os.path.basename(source)}.o")
        jobs.append((["clang++", *CXX_FLAGS, *build_flags, *includes, "-c", os.path.join(ROOT, source), "-o", obj], obj))
    c_includes = [f"-I{os.path.join(ROOT, d)}" for d in ("wasm/include", "lib/bitexact")]
    for index, source in enumerate(entry["c"] + [tables]):
        path = source if os.path.isabs(source) else os.path.join(ROOT, source)
        obj = os.path.join(out, f"c{index}_{os.path.basename(source)}.o")
        jobs.append((["clang", *C_FLAGS, *build_flags, *c_includes, "-c", path, "-o", obj], obj))
    with concurrent.futures.ThreadPoolExecutor() as pool:
        list(pool.map(lambda job: run(job[0]), jobs))
    target = os.path.join(OUT, entry["name"] + ".wasm")
    exports = [f"--export={name}" for name in entry["data"]]
    cmd = ["wasm-ld", *link_flags(entry, pins["stack_size"]), "--gc-sections", *exports, *[obj for _, obj in jobs], "-o", target]
    run(cmd)
    with open(target, "rb") as f:
        linked_wasm = f.read()
    try:
        metered = wasm_fuel.instrument(linked_wasm, find_wasm_tools(pins["tools"]["wasm-tools"]))
    except wasm_fuel.FuelError as e:
        sys.exit(f"{entry['name']}: {e}")
    with open(target, "wb") as f:
        f.write(metered)
    info = parse_module(metered)
    others = [r for r in fixture_ranges()]
    problems = check_window(entry, pins, info, others)
    linked = (min(o for o, _ in info["segments"] + [(info["stack_top"], 0)]),
              max(o + n for o, n in info["segments"] + [(info["stack_top"], 0)]))
    ranges = [(name, low, high) for name, low, high, _, _ in others] + [(entry["name"], *linked)]
    problems += check_layout(pins, ranges)
    if problems:
        sys.exit("\n".join(problems))
    low = min(offset for offset, _ in info["segments"])
    print(f"built {target}: {len(metered)} bytes (size budget {entry['size_budget']}), data {low:#x}..{max(o + n for o, n in info['segments']):#x}, stack top {info['stack_top']:#x}, "
          f"window {entry['data_base']:#x}..{entry['data_base'] + pins['window']:#x}, "
          f"table {min(o for o, _ in info['elements'])}..{max(o + n for o, n in info['elements'])}")


def check_tools_game(pins):
    for name in ("clang", "wasm-ld"):
        found = tool_version([name, "--version"], r"(?:clang version|LLD) (\S+)")
        if found != pins["tools"]["clang"]:
            sys.exit(f"{name} is {found}, wasm/modules.json pins {pins['tools']['clang']}")


def check_size(entry, size):
    """Reasons a module's .wasm of the given size is over its manifest size_budget."""
    budget = entry.get("size_budget")
    if not isinstance(budget, int) or budget <= 0:
        return [f"{entry['name']}: the manifest has no size_budget"]
    if size > budget:
        return [f"{entry['name']}: the .wasm is {size} bytes, over its size_budget {budget}"]
    return []


def check_fuel_manifest(pins):
    """Reasons the manifest's fuel rule, or a module's fuel_budget, is not the one the metering pass implements."""
    problems = []
    if pins.get("fuel") != {"rule": wasm_fuel.RULE, "version": wasm_fuel.VERSION}:
        problems.append(f"the manifest's fuel entry is not the pass's rule: {wasm_fuel.RULE!r}, version {wasm_fuel.VERSION}")
    for entry in pins["modules"]:
        budget = entry.get("fuel_budget")
        if not isinstance(budget, int) or not 0 < budget < 1 << 31:
            problems.append(f"{entry['name']}: fuel_budget must be an integer from 1 to {(1 << 31) - 1}")
    return problems


def check_kinds(name, spec, details):
    """Reasons the manifest's kinds for the imports and slots disagree with the wasm signatures in details."""
    problems = []

    def compare(label, entry, signature):
        if not (isinstance(entry, list) and len(entry) == 2 and isinstance(entry[0], list)):
            return [f"{name}: {label}: the manifest entry is not [[parameter kinds], result kind]"]
        params, result = entry
        wasm_params, wasm_result = signature
        found = []
        if len(params) != len(wasm_params):
            found.append(f"{name}: {label}: the manifest has {len(params)} parameters, the wasm signature {len(wasm_params)}")
        else:
            for index, (kind, wasm) in enumerate(zip(params, wasm_params)):
                if kind not in KINDS or (kind == "f64") != (wasm == "f64"):
                    found.append(f"{name}: {label}: parameter {index} is {kind!r} in the manifest and {wasm} in wasm")
        if result == "void":
            if wasm_result != "void":
                found.append(f"{name}: {label}: the manifest result is void, the wasm result {wasm_result}")
        elif result not in KINDS or (result == "f64") != (wasm_result == "f64") or wasm_result == "void":
            found.append(f"{name}: {label}: the result is {result!r} in the manifest and {wasm_result} in wasm")
        return found

    for module, entries in spec.get("imports", {}).items():
        for field, entry in entries.items():
            if (module, field) in details["imports"]:
                problems += compare(f"{module}.{field}", entry, details["imports"][(module, field)])
    if "slots" not in spec:
        return problems
    slots = spec["slots"]
    for slot in sorted((set(details["exports"]) - set(wasm_fuel.EXPORTS)) ^ set(slots)):
        problems.append(f"{name}: slot {slot} is in only one of the manifest's slots and the module's function exports")
    for slot, entry in slots.items():
        if slot in details["exports"]:
            problems += compare(f"slot {slot}", entry, details["exports"][slot])
    return problems


def check_interface(name, imports, exports, spec, details=None):
    """Reasons a module's imports or exports are outside its allowlist; imports and exports are lists of names (module, name) and name.

    details, from module_interface, adds the checks of imported globals and of the manifest's argument kinds."""
    problems = []
    allowed = set(ENV_IMPORTS)
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
    if details is not None:
        for module, field in details["globals"]:
            problems.append(f"{name}: imports the global {module}.{field}: a module reads its block base from g_blockBase")
        problems += check_kinds(name, spec, details)
    return problems


def signature_of(text):
    """The (parameter types, result type or "void") of a wasm-tools signature text."""
    params = [t for group in re.findall(r"\(param ([^)]*)\)", text) for t in group.split()]
    results = [t for group in re.findall(r"\(result ([^)]*)\)", text) for t in group.split()]
    return params, results[0] if results else "void"


def module_interface(wasm_tools, path):
    text = subprocess.run([wasm_tools, "print", path], capture_output=True, text=True, check=True).stdout
    imports = re.findall(r'^\s*\(import "([^"]*)" "([^"]*)"', text, re.M)
    exports = re.findall(r'^\s*\(export "([^"]*)"', text, re.M)
    types = {int(n): body for n, body in re.findall(r"^\s*\(type \(;(\d+);\) \(func([^\n]*)\)\)$", text, re.M)}
    details = {"globals": re.findall(r'^\s*\(import "([^"]*)" "([^"]*)" \(global', text, re.M), "imports": {}, "exports": {}}
    for module, field, body in re.findall(r'^\s*\(import "([^"]*)" "([^"]*)" \(func (.*)\)\)$', text, re.M):
        details["imports"][(module, field)] = signature_of(body)
    funcs = {}
    for label, typeno, rest in re.findall(r"^\s*\(func (\$\S+) \(;\d+;\) \(type (\d+)\)([^\n]*)$", text, re.M):
        funcs[label] = signature_of(types[int(typeno)])
    for field, label in re.findall(r'^\s*\(export "([^"]*)" \(func (\$\S+)\)\)', text, re.M):
        details["exports"][field] = funcs[label]
    return imports, exports, details


def run_imports():
    game = game_manifest()
    wasm_tools = find_wasm_tools(game["tools"]["wasm-tools"])
    specs = {m["name"]: {"imports": m["imports"], "exports": m["exports"] + m["data"] + list(wasm_fuel.EXPORTS), "slots": m["slots"]} for m in game["modules"]}
    for entry in manifest()["modules"]:
        specs[entry["name"]] = {"imports": {}, "exports": FIXTURE_EXPORTS}
    failures = []
    for name, spec in specs.items():
        path = os.path.join(OUT, name + ".wasm")
        if not os.path.exists(path):
            sys.exit(f"{path} is missing: build first: uv run tools/wasm_modules.py build")
        imports, exports, details = module_interface(wasm_tools, path)
        problems = check_interface(name, imports, exports, spec, details)
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

    game = game_manifest()
    failures += check_fuel_manifest(game)
    wasm_tools = find_wasm_tools(game["tools"]["wasm-tools"])
    for entry in game["modules"]:
        path = os.path.join(OUT, entry["name"] + ".wasm")
        if os.path.exists(path):
            for imp in wasmtime.Module.from_file(store.engine, path).imports:
                if isinstance(imp.type, wasmtime.GlobalType):
                    failures.append(f"{entry['name']}: imports the global {imp.module}.{imp.name}")
            with open(path, "rb") as f:
                data = f.read()
            failures += [f"{entry['name']}: {problem}" for problem in wasm_fuel.check_unmetered_surface(data, wasm_tools)]
            failures += check_size(entry, len(data))
            print(f"{entry['name']}: {len(data)} bytes, size budget {entry['size_budget']}, fuel budget {entry['fuel_budget']}")

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


def run_adapters():
    """Build the loader and the adapter program for this machine and run them against the pinned libwasmtime."""
    import glob

    import wasmtime

    libraries = glob.glob(os.path.join(os.path.dirname(wasmtime.__file__), "*", "_libwasmtime.*"))
    if len(libraries) != 1:
        sys.exit(f"expected one libwasmtime in the wasmtime package, found {libraries}")
    out = os.path.join(OUT, "adapters")
    os.makedirs(out, exist_ok=True)
    wasm_tools = find_wasm_tools(game_manifest()["tools"]["wasm-tools"])
    for name in ("old", "current"):
        run([wasm_tools, "parse", os.path.join(ADAPTERS, name + ".wat"), "-o", os.path.join(out, name + ".wasm")])
    program = os.path.join(out, "adapters")
    sources = [os.path.join(LOADER, name) for name in ("wasmloader.c", "json.c", "format.c")]
    run(["clang", "-std=gnu11", "-O1", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-D_vsnprintf=vsnprintf", f"-I{LOADER}",
         f"-isystem{WASMTIME_INCLUDE}", os.path.join(ADAPTERS, "adapters.c"), *sources, "-ldl", "-o", program])
    result = subprocess.run([program, libraries[0], os.path.join(ADAPTERS, "manifest.json"), out], text=True)
    return result.returncode


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


def statics_run(path=None, report=None):
    """The statics check on the stubbed run; with path, another build of the module (a variant), and with report (a dict), its exports and spawn count."""
    import wasmtime

    pins = game_manifest()
    entry = pins["modules"][0]
    path = path or os.path.join(OUT, entry["name"] + ".wasm")
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
    put(exports["g_blockBase"].value(store), base)
    put(game_at + off[("Game", "width")], 4096)
    put(game_at + off[("Game", "height")], 4096)
    put(game_at + off[("Game", "windSpeedMin")], 0)
    put(game_at + off[("Game", "windSpeedMax")], 30)
    put(game_at + off[("Game", "windSpeedDenominator")], 30)

    params = scratch
    memory.write(store, b"Meteor\0".ljust(32, b"\0"), params)
    memory.write(store, (50).to_bytes(4, "little") + struct.pack("<fff", 0.5, 2.0, 3.0), params + 32)

    budget = entry["fuel_budget"]

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
    most = 0
    for tick in range(1, STATICS_TICKS + 1):
        put(game_at + ticks, tick)
        exports["__fuel_set"](store, budget)
        try:
            call("UpdateWind")
            call("UpdateMeteors")
        except wasmtime.Trap as trap:
            if exports["__fuel_get"](store) < 0:
                print(f"FAIL: {entry['name']} ran out of fuel at tick {tick}: more than its fuel_budget {budget}")
            else:
                print(f"FAIL: {entry['name']} trapped at tick {tick}: {trap}")
            return 1
        most = max(most, budget - exports["__fuel_get"](store))
        if digest() != setup:
            print(f"FAIL: the statics {statics[0]:#x}..{statics[1]:#x} of {entry['name']} changed at tick {tick}")
            return 1
    if state["spawns"] == 0:
        print("FAIL: SpawnProjectile was never called, so no meteor ran")
        return 1
    if report is not None:
        report.update(exports=exports, store=store, spawns=state["spawns"], most=most)
    print(f"statics: {STATICS_TICKS} ticks, {state['spawns']} SpawnProjectile calls, {statics[1] - statics[0]} bytes hashed, ok")
    print(f"fuel: the most one tick burned on the stubbed run is {most}, under the fuel_budget {budget} of {entry['name']}")
    return 0


FATAL_ORDER = ["import FatalError", "call returned", "fatal exit 2"]


def fatal_run(path=None):
    """Run wind_meteors' LoadMeteorDefaults into bogus data: the FatalError import records the message and traps; returns the events in order."""
    import wasmtime

    pins = game_manifest()
    entry = pins["modules"][0]
    path = path or os.path.join(OUT, entry["name"] + ".wasm")
    if not os.path.exists(path):
        sys.exit(f"{path} is missing: build first: uv run tools/wasm_modules.py build")
    base, scratch = pins["block_base"], pins["scratch"][0]
    game_at, sim_at = base + STATICS_GAME_AT, base + STATICS_SIM_AT
    store = wasmtime.Store()
    memory = wasmtime.Memory(store, wasmtime.MemoryType(wasmtime.Limits(pins["memory_pages"], pins["memory_pages"])))
    table = wasmtime.Table(store, wasmtime.TableType(wasmtime.ValType.funcref(),
                                                      wasmtime.Limits(pins["table_size"], pins["table_size"])), None)
    events = []
    recorded = []

    def fatal_error(address):
        end = address
        while memory.read(store, end, end + 1)[0] != 0:
            end += 1
        recorded.append(bytes(memory.read(store, address, end)).decode("latin-1"))
        events.append("import FatalError")
        raise RuntimeError("the import ends the game")

    def build_data_path(buf, _dir, _name, _ext):
        return buf

    stubs = {("services", "FatalError"): fatal_error, ("services", "BuildDataPath"): build_data_path,
             ("services", "TdfFile_LoadFile"): lambda _file, _path: 1,
             ("services", "TdfFile_SelectRecord"): lambda _file, _name: 1,
             ("services", "TdfRecord_GetFieldString"): lambda _record, _dst, _key, _size, _def: 1,
             ("services", "TdfRecord_GetFieldInt"): lambda _record, _key, _def: 0,
             ("services", "TdfRecord_GetFieldDouble"): lambda _record, _key, _def: 0.0}

    def after(name, results):
        def stub(*_args):
            events.append(f"import {name}")
            return 0 if results else None
        return stub

    module = wasmtime.Module.from_file(store.engine, path)
    linker = wasmtime.Linker(store.engine)
    linker.define(store, "env", "memory", memory)
    linker.define(store, "env", "__indirect_function_table", table)
    for imp in module.imports:
        if isinstance(imp.type, wasmtime.FuncType):
            key = (imp.module, imp.name)
            linker.define(store, imp.module, imp.name, wasmtime.Func(store, imp.type, stubs.get(key, after(imp.name, imp.type.results))))
    exports = linker.instantiate(store, module).exports(store)

    def put(address, value):
        memory.write(store, value.to_bytes(4, "little"), address)

    put(exports["g_game"].value(store), game_at)
    put(exports["g_simGlobals"].value(store), sim_at)
    memory.write(store, b"tdf\0", exports["g_extTdf"].value(store))
    put(exports["g_blockBase"].value(store), base)
    exports["__fuel_set"](store, entry["fuel_budget"])
    try:
        exports["LoadMeteorDefaults"](store, scratch)
        events.append("call completed")
    except (wasmtime.Trap, wasmtime.WasmtimeError, RuntimeError):
        events.append("call returned")
    if recorded:
        events.append("fatal exit 2")
    return events, recorded


def run_fatal():
    events, recorded = fatal_run()
    expected = [event for event in events if event in FATAL_ORDER]
    if expected != FATAL_ORDER or events[-1] != FATAL_ORDER[-1] or events[events.index("call returned") - 1] != FATAL_ORDER[0]:
        print(f"FAIL: expected {FATAL_ORDER} with nothing between the import and the trap, got {events}")
        return 1
    if recorded != ["Hey, hoser!  The default meteor shower data was bogus!"]:
        print(f"FAIL: the message was {recorded}")
        return 1
    print(f"fatal: {' -> '.join(events)}; message {recorded[0]!r}, ok")
    return 0


GLUE_FATAL_SCENARIO = os.path.join(ROOT, "reference", "scenarios", "console-ilose.txt")
GLUE_FATAL_TDF = "[Default]\n\t{\n\tMeteorWeapon=BOGUS;\n\tMeteorRadius=0;\n\tMeteorDensity=0;\n\tMeteorDuration=0;\n\tMeteorInterval=0;\n\t}\n"
GLUE_FATAL_MESSAGE = "Hey, hoser!  The default meteor shower data was bogus!"
GLUE_FATAL_RETURNED = "wind_meteors: wl_call returned with a fatal pending"


def glue_fatal_problem(exit_codes, log):
    """The reason a glue-fatal run is wrong (None when right): the exit code, the message, and the order of the glue's line and the message."""
    if exit_codes != [2]:
        return f"the exit code was {exit_codes}, expected [2]"
    message = log.find(GLUE_FATAL_MESSAGE)
    if message < 0:
        return f"the log has no {GLUE_FATAL_MESSAGE!r}"
    returned = log.find(GLUE_FATAL_RETURNED)
    if returned < 0:
        return f"the log has no {GLUE_FATAL_RETURNED!r}: the game ended before wl_call returned"
    if returned > message:
        return "the message was logged before wl_call returned"
    if " trapped " in log:
        return "the log has a module trap text: the exit was not the glue's"
    return None


def run_glue_fatal(exe, game_dir):
    """Run the exe in test mode with a bogus gamedata/meteor.tdf placed in its game copy; the module's FatalError must end the game after wl_call returned."""
    exe = os.path.abspath(exe)
    if not os.path.exists(exe):
        print(f"FAIL: {exe} is missing: build it first")
        return 1
    for name in ("wasmtime_host.dll", "wind_meteors.wasm", "modules.json"):
        if not os.path.exists(os.path.join(os.path.dirname(exe), name)):
            print(f"FAIL: {name} is missing next to {exe}: cmake --build build/modern --target wasm_runtime")
            return 1
    scratch = os.path.join(ROOT, "build", "scratch", "glue-fatal")
    os.makedirs(scratch, exist_ok=True)
    fixture = os.path.join(scratch, "meteor.tdf")
    with open(fixture, "w") as f:
        f.write(GLUE_FATAL_TDF)
    cmd = [sys.executable, os.path.join(ROOT, "tools", "playtest.py"), "--exe", exe, "--testmode", GLUE_FATAL_SCENARIO,
           "--testmode-timeout", "120", "--put", "gamedata/meteor.tdf", fixture]
    if game_dir:
        cmd += ["--game-dir", game_dir]
    done = subprocess.run(cmd, capture_output=True, text=True)
    found = re.search(r"^logs and screenshots: (.+)/$", done.stdout, re.M)
    if not found:
        print(f"FAIL: playtest.py gave no run directory (exit {done.returncode}):\n{done.stdout}{done.stderr}")
        return 1
    session = os.path.join(ROOT, found.group(1))
    runs = [d for d in os.listdir(session) if os.path.isdir(os.path.join(session, d))]
    if len(runs) != 1:
        print(f"FAIL: expected one run in {session}, found {runs}")
        return 1
    run = os.path.join(session, runs[0])
    with open(os.path.join(run, "result.json")) as f:
        result = json.load(f)
    log_path = os.path.join(run, "wine.log")
    log = open(log_path, errors="replace").read() if os.path.exists(log_path) else ""
    problem = glue_fatal_problem(result["exit_codes"], log)
    if problem:
        print(f"FAIL: {problem} (kept: {run})")
        return 1
    shutil.rmtree(session)
    print(f"glue-fatal: exit code 2, {GLUE_FATAL_RETURNED!r} before {GLUE_FATAL_MESSAGE!r}, no trap text, ok")
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
    if command == "fatal":
        return run_fatal()
    return statics_run() if command == "statics" else run_adapters()


def check():
    problems = check_majors(game_manifest(), header_majors())
    for problem in problems:
        print("FAIL", problem)
    if problems:
        return 1
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
    parser.add_argument("command", choices=["build", "check", "imports", "statics", "adapters", "fatal", "glue-fatal"])
    parser.add_argument("--exe", default=os.path.join(ROOT, "build", "modern", "TotalA.exe"), help="glue-fatal: the exe to run")
    parser.add_argument("--game-dir", help="glue-fatal: the game's install (default: Steam's)")
    args = parser.parse_args()
    if args.command == "glue-fatal":
        return run_glue_fatal(args.exe, args.game_dir)
    if args.command == "build":
        build()
        return 0
    if args.command == "imports":
        return run_imports()
    if args.command == "statics":
        return run_under_wasmtime("statics")
    if args.command == "adapters":
        return run_under_wasmtime("adapters")
    if args.command == "fatal":
        return run_under_wasmtime("fatal")
    return check()


if __name__ == "__main__":
    sys.exit(main())
