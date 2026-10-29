"""Check that the wind and meteors module's maths gives the native build's bits.

    uv run tools/wasm_maths.py build [--wasm-only]   # build/wasm-maths: maths.wasm and maths_native.exe
    uv run tools/wasm_maths.py check                 # run both, compare every case
    uv run tools/wasm_maths.py check --write         # also write the native hashes to tests/wasm/maths/hashes.txt
    uv run tools/wasm_maths.py check --wasm-only     # compare the wasm side with hashes.txt (no Wine)
    uv run tools/wasm_maths.py check --embedded      # run maths.wasm under the game's engine, compare with hashes.txt

check runs native (Wine), maths.wasm under the wasmtime package, and maths.wasm under the game's own engine
(wasmtime 50.0.0 on Pulley, NaN canonicalisation on, through lib/wasmloader and tests/wasm/host/host.c as
wasmtime_host.dll and host.exe under Wine), and every case must agree across all three. The embedded runs need
`uv run tools/wasm_host.py build` first. CI runs only --wasm-only, which needs no Wine.

tests/wasm/maths/maths.c walks the input range of each helper the module reaches (ScaledSin and ScaledCos,
FixedSine, the float expressions of SetMeteorParams, the wind division and clamp, double to float, atof) and
hashes every result with SHA-256. It is built twice from the same source: for the native game with clang-cl
for i686 (x87 code, -mno-sse, as the game's files are; lib/bitexact compiled with SSE2 doubles, as the game
links it) against MSVC 5's headers and LIBCMT.LIB, run under Wine with the FPU at 53-bit precision, round to
nearest (_control87 0x9001F); and for the module with clang --target=wasm32 and wasm-ld together with
lib/bitexact and its generated tables, run under the pinned wasmtime package. The module is built with
-mno-nontrapping-fptoint, so every float to int conversion is LLVM's range-checked lowering, which gives
0x80000000 for overflow and NaN as x87 _ftol does; the meteor_*_overflow, _negative and _special cases
check that on every expression of SetMeteorParams.
"""

import argparse
import importlib.util
import os
import shutil
import subprocess
import sys
from pathlib import Path

import bitexact
import bitexact_tables
from sources import ROOT

MATHS = ROOT / "tests/wasm/maths"
HASHES = MATHS / "hashes.txt"
OUT = ROOT / "build/wasm-maths"
LIB = ROOT / "lib/bitexact"
GUARD = "WASM_MATHS_UNDER_WASMTIME"
WASMTIME = "50.0.1"
CLANG = "22.1.8"
LIB_SOURCES = [LIB / "trig.c", LIB / "math.c", LIB / "strtof.c"]
WASM_FLAGS = ["--target=wasm32", "-O2", "-nostdlib", "-ffp-contract=off", "-mno-nontrapping-fptoint", "-fwrapv", "-fno-strict-aliasing", "-w",
              f"-I{LIB}"]


def tables() -> Path:
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / "bx_tables.c"
    bitexact_tables.generate(path)
    return path


def check_versions() -> None:
    for tool, pattern in (("clang", "clang version "), ("wasm-ld", "LLD ")):
        text = subprocess.run([tool, "--version"], capture_output=True, text=True, check=True).stdout
        found = text.split(pattern)[1].split()[0]
        if found != CLANG:
            sys.exit(f"{tool} is {found}, expected {CLANG}")


def build_wasm(tables_c: Path) -> None:
    objects = []
    for src in [MATHS / "maths.c", *LIB_SOURCES, tables_c]:
        obj = OUT / (src.stem + ".wasm.o")
        subprocess.run(["clang", *WASM_FLAGS, "-c", str(src), "-o", str(obj)], check=True)
        objects.append(str(obj))
    subprocess.run(["wasm-ld", "--no-entry", "--export=maths_case_count", "--export=maths_case_name",
                    "--export=maths_case_info", "--export=maths_run_case", "--export=maths_last_count",
                    "-zstack-size=65536", *objects, "-o", str(OUT / "maths.wasm")], check=True)
    print(f"built {OUT / 'maths.wasm'}")


def build_native(tables_c: Path) -> bool:
    bitexact.CLANG_BUILD = OUT / "clang"
    tools = bitexact.clang_tools()
    flags = bitexact.clang_setup(tools) if tools else None
    if tools is None or flags is None:
        return False
    flags = [f for f in flags if f != "-DBX_REPLAY"]
    objects = []
    for src in [MATHS / "maths.c", *LIB_SOURCES, tables_c]:
        obj = bitexact.CLANG_BUILD / (src.stem + ".obj")
        if src == MATHS / "maths.c":
            extra = ["-mno-sse", "-DMATHS_NATIVE", f"-I{LIB}"]
        else:
            extra = ["-msse2", "-mfpmath=sse"]
        proc = subprocess.run([tools["clang-cl"], *flags, *extra, "/c", f"/Fo{obj}", str(src)], capture_output=True,
                              text=True)
        if proc.returncode != 0:
            print(proc.stdout + proc.stderr, file=sys.stderr)
            return False
        objects.append(str(obj))
    msvc5 = bitexact.MSVC5
    exe = OUT / "maths_native.exe"
    cmd = [tools["lld-link"], "/nologo", "/machine:x86", "/subsystem:console", "/entry:mainCRTStartup", "/fixed:no",
           "/safeseh:no", "/incremental:no", "/nodefaultlib", f"/out:{exe}", *objects, str(msvc5 / "LIB/LIBCMT.LIB"),
           str(msvc5 / "LIB/OLDNAMES.LIB"), str(bitexact.CLANG_BUILD / "KERNEL32.lib")]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0 or not exe.exists():
        print(proc.stdout + proc.stderr, file=sys.stderr)
        return False
    print(f"built {exe}")
    return True


def build(native: bool = True) -> int:
    check_versions()
    tables_c = tables()
    build_wasm(tables_c)
    if native and not build_native(tables_c):
        return 2
    return 0


def parse_hashes(text: str) -> dict[str, tuple[int, str]]:
    """The lines `name count sha256` of a native run or of hashes.txt, as name -> (count, hash); other lines are ignored."""
    result = {}
    for line in text.splitlines():
        parts = line.split()
        if len(parts) == 3 and len(parts[2]) == 64 and parts[1].isdigit():
            result[parts[0]] = (int(parts[1]), parts[2])
    return result


def run_native() -> dict[str, tuple[int, str]]:
    env = dict(os.environ, WINEDEBUG="-all", WINEARCH="win64", WINEPREFIX=str(OUT / "prefix"))
    for var in ("DISPLAY", "WAYLAND_DISPLAY"):
        env.pop(var, None)
    proc = subprocess.run(["wine", bitexact.windows_path(OUT / "maths_native.exe")], env=env, capture_output=True,
                          text=True)
    sys.stderr.write(proc.stderr)
    if proc.returncode != 0:
        sys.exit(f"the native run exited with {proc.returncode}")
    first = proc.stdout.splitlines()[0]
    print(f"native: {first}")
    return parse_hashes(proc.stdout)


def run_wasm() -> dict[str, tuple[int, str]]:
    import wasmtime

    store = wasmtime.Store()
    module = wasmtime.Module.from_file(store.engine, str(OUT / "maths.wasm"))
    instance = wasmtime.Linker(store.engine).instantiate(store, module)
    exports = instance.exports(store)
    memory = exports["memory"]
    result = {}
    for index in range(exports["maths_case_count"](store)):
        pointer = exports["maths_case_name"](store, index)
        name = bytes(memory.read(store, pointer, pointer + 64)).split(b"\0")[0].decode()
        digest = exports["maths_run_case"](store, index)
        result[name] = (exports["maths_last_count"](store), bytes(memory.read(store, digest, digest + 32)).hex())
    return result


def run_embedded() -> dict[str, tuple[int, str]]:
    host = ROOT / "build/wasm_host"
    for name in ("host.exe", "wasmtime_host.dll"):
        if not (host / name).exists():
            sys.exit(f"{host / name} is missing: run `uv run tools/wasm_host.py build` first")
    env = dict(os.environ, WINEDEBUG="-all", WINEARCH="win64", WINEPREFIX=str(host / "prefix"))
    for var in ("DISPLAY", "WAYLAND_DISPLAY"):
        env.pop(var, None)
    proc = subprocess.run(["wine", str(host / "host.exe"), "--maths", str(OUT / "maths.wasm")], env=env, cwd=host,
                          capture_output=True, text=True)
    sys.stderr.write(proc.stderr)
    if proc.returncode != 0:
        sys.exit(f"the embedded run exited with {proc.returncode}")
    return parse_hashes(proc.stdout)


def compare(reference: dict[str, tuple[int, str]], others: dict[str, dict[str, tuple[int, str]]],
            label: str = "native") -> int:
    bad = 0
    for name in reference:
        n = reference[name]
        found = {key: results.get(name) for key, results in others.items()}
        same = all(value == n for value in found.values())
        status = "ok" if same else "MISMATCH"
        shown = " / ".join(f"{value[1] if value else 'missing'} {key}" for key, value in found.items())
        print(f"{name}: {n[1]} {label} / {shown} ({n[0]} cases) {status}")
        if not same:
            bad += 1
    for key, results in others.items():
        for name in results:
            if name not in reference:
                print(f"{name}: in the {key} results but not in the {label} results")
                bad += 1
    return bad


def check(args: argparse.Namespace) -> int:
    if not (OUT / "maths.wasm").exists() or (not (args.wasm_only or args.embedded) and not (OUT / "maths_native.exe").exists()):
        sys.exit("build first: uv run tools/wasm_maths.py build")
    if args.wasm_only or args.embedded:
        native = parse_hashes(HASHES.read_text())
        label = "hashes.txt"
    else:
        native = run_native()
        label = "native"
    others = {}
    if not args.embedded:
        others["wasm"] = run_wasm()
    if not args.wasm_only:
        others["embedded"] = run_embedded()
    if args.write:
        HASHES.write_text("".join(f"{name} {count} {digest}\n" for name, (count, digest) in native.items()))
        print(f"wrote {HASHES}")
    bad = compare(native, others, label)
    print("ok" if bad == 0 else f"{bad} MISMATCH")
    return 1 if bad else 0


def check_in_wasmtime(argv: list[str]) -> int:
    if importlib.util.find_spec("wasmtime") is None:
        if os.environ.get(GUARD):
            raise ImportError("wasmtime")
        cmd = ["uv", "run", "--no-project", "--with", f"wasmtime=={WASMTIME}", "python",
               os.path.abspath(__file__), *argv]
        return subprocess.run(cmd, env=dict(os.environ, **{GUARD: "1"})).returncode
    return -1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    bld = sub.add_parser("build")
    bld.add_argument("--wasm-only", action="store_true", help="skip the native build (no clang-cl or MSVC 5 needed)")
    chk = sub.add_parser("check")
    chk.add_argument("--wasm-only", action="store_true", help="compare with tests/wasm/maths/hashes.txt, no Wine")
    chk.add_argument("--embedded", action="store_true",
                     help="run maths.wasm under the game's engine (Wine) and compare with hashes.txt")
    chk.add_argument("--write", action="store_true", help="write the native hashes to tests/wasm/maths/hashes.txt")
    args = parser.parse_args()
    if args.command == "check" and args.wasm_only and args.embedded:
        parser.error("--wasm-only and --embedded exclude each other")
    if args.command == "build":
        return build(not args.wasm_only)
    code = -1 if args.embedded else check_in_wasmtime(sys.argv[1:])
    return check(args) if code == -1 else code


if __name__ == "__main__":
    sys.exit(main())
