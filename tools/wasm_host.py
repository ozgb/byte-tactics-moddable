"""Build and run the wasmtime pilot host for the modern i686 build (#562).

    uv run tools/wasm_host.py build [--out DIR]
    uv run tools/wasm_host.py run MODULE.wasm EXPORT [I32 ARGS...] [--loop N] [--out DIR]
    uv run tools/wasm_host.py modules [--overlap] [--out DIR]

build compiles three things into DIR (default build/wasm_host):

  * wasmtime_host.dll: tests/wasm/host/dll, wasmtime-c-api-impl pinned in its
    Cargo.toml and Cargo.lock, built with cargo for i686-pc-windows-gnu (its own
    CRT, Pulley interpreter, no WASI, no cache, no threads);
  * host.exe: tests/wasm/host/host.c, built with clang-cl for i686-pc-windows-msvc
    and lld-link against MSVC 5's LIBCMT, like the game, loading the DLL with
    LoadLibraryA so that only C types and pointers cross the boundary;
  * add.wasm: tests/wasm/host/add.wat, through wat2wasm.

run starts host.exe under Wine (WINEPREFIX=DIR/prefix) with the module, calls the
export with the i32 arguments LOOP times and prints the result and, for LOOP > 1,
calls per second.

modules builds the fixture modules of tests/wasm/modules (tools/wasm_modules.py
build) and the host if needed, then runs the host over one memory and one table
sized and placed by tests/wasm/modules/modules.json: it refuses overlapping
windows, instantiates every module, calls each module's slot through the shared
table and checks the results and each module's data. With --overlap it writes a
manifest to DIR that moves the second module's bases onto the first's and expects
the host to refuse it, naming both modules.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HOST_DIR = ROOT / "tests" / "wasm" / "host"
DLL_CRATE = HOST_DIR / "dll"
MSVC5 = (ROOT / "toolchain" / "msvc5-sp3").resolve()
MODULES_DIR = ROOT / "tests" / "wasm" / "modules"
WASM_MODULES_OUT = ROOT / "build" / "wasm-modules"
RUST_TARGET = "i686-pc-windows-gnu"
C_API_PACKAGE = "wasmtime-c-api-impl"
CONF_H = """#ifndef WASMTIME_CONF_H
#define WASMTIME_CONF_H
#define WASMTIME_FEATURE_CRANELIFT
#define WASMTIME_FEATURE_PULLEY
#define WASMTIME_FEATURE_GC
#define WASMTIME_FEATURE_COMPILER
#endif
"""


def run(command, **kwargs):
    print("+", " ".join(str(part) for part in command), flush=True)
    result = subprocess.run([str(part) for part in command], **kwargs)
    if result.returncode:
        sys.exit(result.returncode)


def cargo_env(out):
    env = dict(os.environ)
    env["CARGO_TARGET_DIR"] = str(out / "target")
    env["CARGO_TARGET_I686_PC_WINDOWS_GNU_LINKER"] = "i686-w64-mingw32-gcc"
    return env


def build_dll(out):
    env = cargo_env(out)
    manifest = DLL_CRATE / "Cargo.toml"
    run(["cargo", "build", "--release", "--locked", "--target", RUST_TARGET, "--manifest-path", manifest], env=env)
    shutil.copy2(out / "target" / RUST_TARGET / "release" / "wasmtime_host.dll", out / "wasmtime_host.dll")
    metadata = subprocess.run(
        ["cargo", "metadata", "--format-version", "1", "--locked", "--filter-platform", RUST_TARGET, "--manifest-path", str(manifest)],
        check=True, capture_output=True, text=True, env=env).stdout
    for package in json.loads(metadata)["packages"]:
        if package["name"] == C_API_PACKAGE:
            return Path(package["manifest_path"]).parent / "include"
    sys.exit(f"{C_API_PACKAGE} is not in the crate's dependencies")


def build_host(out, c_api_include):
    work = out / "host"
    conf = work / "conf" / "wasmtime"
    conf.mkdir(parents=True, exist_ok=True)
    (conf / "conf.h").write_text(CONF_H)
    run([sys.executable, ROOT / "tools" / "modern_headers.py", MSVC5 / "INCLUDE", work])
    libs = work / "lib"
    libs.mkdir(exist_ok=True)
    for lib in (MSVC5 / "LIB").glob("*.LIB"):
        for name in (lib.stem + ".lib", lib.stem.lower() + ".lib"):
            link = libs / name
            if not link.is_symlink():
                link.symlink_to(lib)
    run([sys.executable, ROOT / "tools" / "aliases.py", "--import-def", MSVC5 / "LIB" / "KERNEL32.LIB", "--out", work / "KERNEL32.def"])
    run(["llvm-dlltool", "-m", "i386", "-k", "-d", work / "KERNEL32.def", "-l", work / "KERNEL32.lib"])
    run(["clang-cl", "--target=i686-pc-windows-msvc", "-fms-compatibility", "-fms-extensions", "-fms-compatibility-version=11.00",
         "-mno-sse", "/GS-", "-w", "/O2", "-vfsoverlay", work / "overlay.json", f"-I{work / 'conf'}",
         "-imsvc", c_api_include, "-imsvc", MSVC5 / "INCLUDE", "/c", HOST_DIR / "host.c", f"/Fo{work / 'host.obj'}"])
    run(["lld-link", "/nologo", "/machine:x86", "/subsystem:console", f"/out:{out / 'host.exe'}", f"/libpath:{libs}", "/incremental:no",
         "/safeseh:no", work / "host.obj", work / "KERNEL32.lib", MSVC5 / "LIB" / "LIBCMT.LIB", MSVC5 / "LIB" / "OLDNAMES.LIB"])


def build(out):
    out.mkdir(parents=True, exist_ok=True)
    c_api_include = build_dll(out)
    build_host(out, c_api_include)
    run(["wat2wasm", HOST_DIR / "add.wat", "-o", out / "add.wasm"])
    print(f"built {out}")


def wine_env(out):
    env = dict(os.environ)
    for name in ("DISPLAY", "WAYLAND_DISPLAY"):
        env.pop(name, None)
    env["WINEARCH"] = "win64"
    env["WINEPREFIX"] = str(out / "prefix")
    env["WINEDEBUG"] = "-all"
    return env


def run_module(out, module, export, args, loop):
    for name in ("host.exe", "wasmtime_host.dll"):
        if not (out / name).exists():
            sys.exit(f"{out / name} is missing: run `uv run tools/wasm_host.py build` first")
    env = wine_env(out)
    command = ["wine", out / "host.exe"]
    if loop != 1:
        command += ["--loop", str(loop)]
    module = Path(module).resolve()
    if not module.exists():
        sys.exit(f"{module} does not exist (build writes {out / 'add.wasm'})")
    command += [module, export] + list(args)
    run(command, env=env, cwd=out)


def module_arguments(manifest_path):
    manifest = json.loads(Path(manifest_path).read_text())
    arguments = ["--modules", manifest["memory_pages"], manifest["table_size"], manifest["window"], manifest["table_slots"]]
    for entry in manifest["modules"]:
        arguments += [entry["name"], WASM_MODULES_OUT / f"{entry['name']}.wasm", entry["data_base"], entry["table_base"],
                      entry["slot_arg"], entry["slot_result"], entry["data_word"]]
    return arguments


def run_modules(out, overlap):
    manifest_path = MODULES_DIR / "modules.json"
    if overlap:
        manifest = json.loads(manifest_path.read_text())
        first, second = manifest["modules"][0], manifest["modules"][1]
        second["data_base"] = first["data_base"]
        second["table_base"] = first["table_base"]
        manifest_path = out / "overlap.json"
        manifest_path.write_text(json.dumps(manifest, indent=2))
    if not (out / "host.exe").exists():
        build(out)
    run([sys.executable, ROOT / "tools" / "wasm_modules.py", "build"])
    env = wine_env(out)
    command = ["wine", out / "host.exe"] + module_arguments(manifest_path)
    print("+", " ".join(str(part) for part in command), flush=True)
    result = subprocess.run([str(part) for part in command], env=env, cwd=out, capture_output=True, text=True)
    sys.stdout.write(result.stdout)
    sys.stderr.write(result.stderr)
    if overlap:
        names = " and ".join(entry["name"] for entry in json.loads(manifest_path.read_text())["modules"][:2])
        if result.returncode == 1 and all(entry["name"] in result.stderr for entry in json.loads(manifest_path.read_text())["modules"][:2]):
            print(f"refused as expected: {names} overlap")
            return 0
        sys.exit(f"the host did not refuse the overlapping manifest (exit {result.returncode})")
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("build", "run", "modules"):
        cmd = sub.add_parser(name)
        cmd.add_argument("--out", type=Path, default=ROOT / "build" / "wasm_host")
    runner = sub.choices["run"]
    runner.add_argument("module")
    runner.add_argument("export")
    runner.add_argument("args", nargs="*")
    runner.add_argument("--loop", type=int, default=1)
    sub.choices["modules"].add_argument("--overlap", action="store_true")
    options = parser.parse_args()
    out = options.out.resolve()
    if options.command == "build":
        build(out)
    elif options.command == "modules":
        sys.exit(run_modules(out, options.overlap))
    else:
        run_module(out, options.module, options.export, options.args, options.loop)


if __name__ == "__main__":
    main()
