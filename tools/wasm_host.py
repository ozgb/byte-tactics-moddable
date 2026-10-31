"""Build and run the wasmtime pilot host for the modern i686 build (#562).

    uv run tools/wasm_host.py build [--out DIR] [--debug] [--target TARGET] [--xwin DIR]
    uv run tools/wasm_host.py run MODULE.wasm EXPORT [I32 ARGS...] [--loop N] [--out DIR]
    uv run tools/wasm_host.py modules [--overlap] [--out DIR]
    uv run tools/wasm_host.py wind-meteors [--out DIR]
    uv run tools/wasm_host.py headers [--update]

build --debug (DIR defaults to build/wasm_host_debug) builds the DLL with the debugdll profile of its Cargo.toml: panic=unwind,
debug info, not stripped, so a wasmtime panic prints its message and a backtrace (the C API's extern "C" functions abort
after the message). The runs of wasm_maths.py --debug and wasm_loop.py set RUST_BACKTRACE=1.

build --target i686-pc-windows-msvc (DIR defaults to build/wasm_host_msvc) builds the DLL with rustup's cargo
(. "$HOME/.cargo/env", target installed with rustup) and lld-link against the x86 CRT and SDK libraries that
`xwin --arch x86 splat` wrote to --xwin DIR (default: $BT_XWIN, else ~/.xwin-x86). host.exe is the same.

build compiles three things into DIR (default build/wasm_host):

  * wasmtime_host.dll: tests/wasm/host/dll, wasmtime-c-api-impl pinned in its
    Cargo.toml and Cargo.lock, built with cargo for i686-pc-windows-gnu (its own
    CRT, Pulley interpreter, no WASI, no cache, no threads);
  * host.exe: tests/wasm/host/host.c, lib/wasmloader and the host's library and time
    calls (src/host/win32), built with clang-cl for
    i686-pc-windows-msvc and lld-link against MSVC 5's LIBCMT, like the game, against the
    C API headers vendored in third_party/wasmtime. The host passes host_lib_open and
    host_lib_symbol to the loader, so that only C types and pointers cross the boundary;
  * add.wasm: tests/wasm/host/add.wat, through wat2wasm.

run starts host.exe under Wine (WINEPREFIX=DIR/prefix) with the module, calls the
export with the i32 arguments LOOP times and prints the result and, for LOOP > 1,
calls per second.

modules builds the fixture modules of tests/wasm/modules (tools/wasm_modules.py
build) and the host if needed, then runs the host over one memory and one table
sized and placed by tests/wasm/modules/modules.json: the loader refuses overlapping
windows, instantiates every module, and the host calls each module's slot through the
shared table and checks the results and each module's data. With --overlap it writes a
manifest to DIR that moves the second module's bases onto the first's and expects
the loader to refuse it, naming both modules.

wind-meteors builds wind_meteors.wasm (tools/wasm_modules.py build), creates the
memory of wasm/modules.json (block_base, block_size, scratch and memory_pages fall back to
the game's values when the manifest lacks them), loads the module with every import
stubbed to trap and prints its slot handles and the addresses of its data exports.

headers fails when third_party/wasmtime/include differs from the include directory of the
pinned wasmtime-c-api-impl crate (apart from wasmtime/conf.h, which this file writes);
--update copies the crate's headers over the vendored ones. build runs the same check.
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
LOADER_DIR = ROOT / "lib" / "wasmloader"
HOST_SOURCES = ROOT / "src" / "host" / "win32"
VENDORED_INCLUDE = ROOT / "third_party" / "wasmtime" / "include"
GAME_MANIFEST = ROOT / "wasm" / "modules.json"
GAME_BLOCK_DEFAULTS = {"memory_pages": 528, "block_base": 0x100000, "block_size": 32 * 1024 * 1024, "scratch": [0xF0000, 0x10000]}
DLL_CRATE = HOST_DIR / "dll"
MSVC5 = (ROOT / "toolchain" / "msvc5-sp3").resolve()
MODULES_DIR = ROOT / "tests" / "wasm" / "modules"
WASM_MODULES_OUT = ROOT / "build" / "wasm-modules"
RUST_TARGET = "i686-pc-windows-gnu"
RUST_TARGETS = ("i686-pc-windows-gnu", "i686-pc-windows-msvc")
XWIN_LIBS = ("crt/lib/x86", "sdk/lib/um/x86", "sdk/lib/ucrt/x86")
DEBUG_PROFILE = "debugdll"
DEBUG_OUT = ROOT / "build" / "wasm_host_debug"
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


def default_xwin():
    return Path(os.environ.get("BT_XWIN") or Path.home() / ".xwin-x86")


def cargo_env(out, target=RUST_TARGET, xwin=None):
    env = dict(os.environ)
    env["CARGO_TARGET_DIR"] = str(out / "target")
    if target == "i686-pc-windows-msvc":
        xwin = Path(xwin or default_xwin())
        env["CARGO_TARGET_I686_PC_WINDOWS_MSVC_LINKER"] = "lld-link"
        flags = " ".join(f"-Lnative={xwin / lib}" for lib in XWIN_LIBS)
        env["RUSTFLAGS"] = f"{env.get('RUSTFLAGS', '')} {flags}".strip()
    else:
        env["CARGO_TARGET_I686_PC_WINDOWS_GNU_LINKER"] = "i686-w64-mingw32-gcc"
    return env


def build_dll(out, debug=False, target=RUST_TARGET, xwin=None):
    env = cargo_env(out, target, xwin)
    manifest = DLL_CRATE / "Cargo.toml"
    profile = ["--profile", DEBUG_PROFILE] if debug else ["--release"]
    run(["cargo", "build", *profile, "--locked", "--target", target, "--manifest-path", manifest], env=env)
    shutil.copy2(out / "target" / target / (DEBUG_PROFILE if debug else "release") / "wasmtime_host.dll", out / "wasmtime_host.dll")


def crate_include(out, target=RUST_TARGET, xwin=None):
    env = cargo_env(out, target, xwin)
    manifest = DLL_CRATE / "Cargo.toml"
    metadata = subprocess.run(
        ["cargo", "metadata", "--format-version", "1", "--locked", "--filter-platform", target, "--manifest-path", str(manifest)],
        check=True, capture_output=True, text=True, env=env).stdout
    for package in json.loads(metadata)["packages"]:
        if package["name"] == C_API_PACKAGE:
            return Path(package["manifest_path"]).parent / "include"
    sys.exit(f"{C_API_PACKAGE} is not in the crate's dependencies")


def header_files(root):
    return sorted(p.relative_to(root).as_posix() for p in root.rglob("*.h") if p.relative_to(root).as_posix() != "wasmtime/conf.h")


def check_headers(crate, update=False):
    """Differences between the vendored C API headers and the crate's, as lines; with update, copy the crate's over."""
    problems = []
    theirs = header_files(crate)
    ours = header_files(VENDORED_INCLUDE)
    for name in sorted(set(theirs) | set(ours)):
        a, b = crate / name, VENDORED_INCLUDE / name
        if name not in ours:
            problems.append(f"{name}: missing from third_party/wasmtime/include")
        elif name not in theirs:
            problems.append(f"{name}: not in the crate")
        elif a.read_bytes() != b.read_bytes():
            problems.append(f"{name}: differs from the crate's")
    conf = VENDORED_INCLUDE / "wasmtime" / "conf.h"
    if not conf.exists() or conf.read_text() != CONF_H:
        problems.append("wasmtime/conf.h: differs from CONF_H in tools/wasm_host.py")
    if update:
        for name in theirs:
            (VENDORED_INCLUDE / name).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(crate / name, VENDORED_INCLUDE / name)
        conf.write_text(CONF_H)
        return []
    return problems


def headers(out, update, target=RUST_TARGET):
    problems = check_headers(crate_include(out, target), update)
    for problem in problems:
        print(problem, file=sys.stderr)
    if problems:
        sys.exit("the vendored wasmtime headers differ from the crate's: run `uv run tools/wasm_host.py headers --update`")
    print("third_party/wasmtime/include matches the crate" if not update else "updated third_party/wasmtime/include")


def build_host(out):
    work = out / "host"
    work.mkdir(parents=True, exist_ok=True)
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
    sources = [HOST_DIR / "host.c", LOADER_DIR / "wasmloader.c", LOADER_DIR / "json.c", LOADER_DIR / "format.c",
               HOST_SOURCES / "library.cpp", HOST_SOURCES / "time.cpp"]
    objects = []
    for source in sources:
        obj = work / (source.stem + ".obj")
        objects.append(obj)
        run(["clang-cl", "--target=i686-pc-windows-msvc", "-fms-compatibility", "-fms-extensions", "-fms-compatibility-version=11.00",
             "-mno-sse", "/GS-", "/GR-", "-w", "/O2", "-vfsoverlay", work / "overlay.json", f"-I{LOADER_DIR}", f"-I{ROOT}",
             "-imsvc", VENDORED_INCLUDE, "-imsvc", MSVC5 / "INCLUDE", "/c", source, f"/Fo{obj}"])
    run(["lld-link", "/nologo", "/machine:x86", "/subsystem:console", f"/out:{out / 'host.exe'}", f"/libpath:{libs}", "/incremental:no",
         "/safeseh:no", *objects, work / "KERNEL32.lib", MSVC5 / "LIB" / "LIBCMT.LIB", MSVC5 / "LIB" / "OLDNAMES.LIB"])


def build(out, debug=False, target=RUST_TARGET, xwin=None):
    out.mkdir(parents=True, exist_ok=True)
    build_dll(out, debug, target, xwin)
    problems = check_headers(crate_include(out, target, xwin))
    if problems:
        sys.exit("the vendored wasmtime headers differ from the crate's:\n  " + "\n  ".join(problems))
    build_host(out)
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


def run_host(out, manifest_path, mode):
    env = wine_env(out)
    command = ["wine", out / "host.exe", "--manifest", manifest_path, "--wasm-dir", WASM_MODULES_OUT, mode]
    print("+", " ".join(str(part) for part in command), flush=True)
    return subprocess.run([str(part) for part in command], env=env, cwd=out, capture_output=True, text=True)


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
    result = run_host(out, manifest_path, "--fixtures")
    sys.stdout.write(result.stdout)
    sys.stderr.write(result.stderr)
    if overlap:
        names = " and ".join(entry["name"] for entry in json.loads(manifest_path.read_text())["modules"][:2])
        if result.returncode == 1 and all(entry["name"] in result.stderr for entry in json.loads(manifest_path.read_text())["modules"][:2]):
            print(f"refused as expected: {names} overlap")
            return 0
        sys.exit(f"the host did not refuse the overlapping manifest (exit {result.returncode})")
    return result.returncode


def run_wind_meteors(out):
    manifest = json.loads(GAME_MANIFEST.read_text())
    filled = [key for key in GAME_BLOCK_DEFAULTS if key not in manifest]
    manifest = {**{key: value for key, value in GAME_BLOCK_DEFAULTS.items() if key not in manifest}, **manifest}
    out.mkdir(parents=True, exist_ok=True)
    manifest_path = out / "wind_meteors_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2))
    if filled:
        print(f"{GAME_MANIFEST.relative_to(ROOT)} lacks {', '.join(filled)}: using the game's values")
    if not (out / "host.exe").exists():
        build(out)
    run([sys.executable, ROOT / "tools" / "wasm_modules.py", "build"])
    result = run_host(out, manifest_path, "--wind-meteors")
    sys.stdout.write(result.stdout)
    sys.stderr.write(result.stderr)
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("build", "run", "modules", "wind-meteors", "headers"):
        cmd = sub.add_parser(name)
        cmd.add_argument("--out", type=Path, default=None)
    runner = sub.choices["run"]
    runner.add_argument("module")
    runner.add_argument("export")
    runner.add_argument("args", nargs="*")
    runner.add_argument("--loop", type=int, default=1)
    sub.choices["build"].add_argument("--target", choices=RUST_TARGETS, default=RUST_TARGET, help="the Rust target of wasmtime_host.dll")
    sub.choices["build"].add_argument("--xwin", type=Path, help="msvc target: the xwin splat directory (default: $BT_XWIN, else ~/.xwin-x86)")
    sub.choices["build"].add_argument("--debug", action="store_true", help="unstripped DLL with panic=unwind and debug info, into build/wasm_host_debug")
    sub.choices["modules"].add_argument("--overlap", action="store_true")
    sub.choices["headers"].add_argument("--update", action="store_true")
    options = parser.parse_args()
    debug = getattr(options, "debug", False)
    msvc = getattr(options, "target", RUST_TARGET) == "i686-pc-windows-msvc"
    out = (options.out or (DEBUG_OUT if debug else ROOT / "build" / ("wasm_host_msvc" if msvc else "wasm_host"))).resolve()
    if options.command == "build":
        build(out, debug, options.target, options.xwin)
    elif options.command == "modules":
        sys.exit(run_modules(out, options.overlap))
    elif options.command == "wind-meteors":
        sys.exit(run_wind_meteors(out))
    elif options.command == "headers":
        headers(out, options.update)
    else:
        run_module(out, options.module, options.export, options.args, options.loop)


if __name__ == "__main__":
    main()
