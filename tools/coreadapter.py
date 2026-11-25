"""The in-repo engine's answer to the core gate's command line (docs/core-gate.md, #1041).

    uv run tools/coreadapter.py GROUP CALLS OUT
    uv run tools/coreadapter.py files --game DIR CALLS OUT

It is the in-repo engine as one program: `hash` is tools/tracerehash.py, `commands` delivers the commands of a script in
the order the harness delivers them, `rand` and `events` are build/modern/core_state_adapter.exe, `loader` and `fuel`
build/modern/core_wasm_adapter.exe, `host` the WASM replay of build/modern/replay_native_test.exe and `files`
build/modern/files_adapter.exe, each under Wine. BT_CORE_BUILD names another build directory, and BT_FILES_ADAPTER
another files adapter.

BT_CORE_FAULT injects one fault for the negative tests of tools/test_coregate.py: `hash_step` (the word step of the hash
multiplies by another prime), `commands_swap` (the first two deliveries of a tick change places), and the faults of the
adapters (`rand_skip`, `events_continuation`, `loader_class`, `fuel_budget`, `host_write`), which the exes read.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tracerehash

ROOT = Path(__file__).resolve().parent.parent
KEYS = ("spawn", "order", "console")


def build_dir() -> Path:
    return Path(os.environ.get("BT_CORE_BUILD", ROOT / "build" / "modern")).resolve()


def fault() -> str:
    return os.environ.get("BT_CORE_FAULT", "")


def read_calls(path: str) -> list[list[str]]:
    return [line.rstrip("\n").split("\t") for line in open(path, encoding="latin-1", newline="")]


def rows(dump: str, schema: str) -> list:
    if fault() == "hash_step":
        tracerehash.PRIME = 0x100000001B5
    sch = tracerehash.Schema(json.loads(Path(schema).read_text()))
    found = tracerehash.rehash(sch, tracerehash.parse_dump(tracerehash.read_text(dump), sch.data["trace_format"]))
    return [[tick, phase, "%016x" % value] for tick, phase, value in found]


def deliver(script: str) -> list[str]:
    """The commands.txt lines of a script, in the order of docs/commands.md, section 1."""
    lines = Path(script).read_text(encoding="latin-1").splitlines()
    if not lines or lines[0] != "bt-script 1":
        raise ValueError("%s is not a bt-script 1 file" % script)
    commands = []
    for position, line in enumerate(lines[1:]):
        number, key, tick, rest = line.split(" ", 3)
        commands.append(((int(tick), KEYS.index(key), position), "%s %s %s %s" % (tick, key, number, rest)))
    commands.sort()
    out = [text for _, text in commands]
    if fault() == "commands_swap":
        for i in range(len(commands) - 1):
            if commands[i][0][0] == commands[i + 1][0][0]:
                out[i], out[i + 1] = out[i + 1], out[i]
                break
    return out


def answer_python(group: str, calls_path: str, out_path: str) -> int:
    handlers = {("hash", "rows"): rows, ("commands", "deliver"): deliver}
    with open(out_path, "w", encoding="latin-1", newline="\n") as out:
        for call in read_calls(calls_path):
            handler = handlers.get((group, call[0]))
            if handler is None:
                print("coreadapter: no call %s in group %s" % (call[0], group), file=sys.stderr)
                return 2
            out.write("%s\t%s\n" % (call[0], json.dumps(handler(*call[1:]))))
    return 0


def beside_modules(calls_path: str, build: Path) -> None:
    """The replayer loads the runtime from beside the module, so put the build's runtime there."""
    for call in read_calls(calls_path):
        where = Path(call[2]).resolve().parent
        for name in ("wasmtime_host.dll", "modules.json"):
            if not (where / name).exists():
                shutil.copyfile(build / name, where / name)


def wine(exe: Path, args: list[str]) -> int:
    return subprocess.run(["wine", str(exe), *args], stdin=subprocess.DEVNULL).returncode


def main(argv: list[str]) -> int:
    os.environ.setdefault("WINEDEBUG", "-all")
    if len(argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    group = argv[0]
    build = build_dir()
    if group in ("hash", "commands"):
        return answer_python(group, argv[1], argv[2])
    if group in ("rand", "events"):
        return wine(build / "core_state_adapter.exe", argv)
    if group in ("loader", "fuel"):
        return wine(build / "core_wasm_adapter.exe", argv)
    if group == "host":
        beside_modules(argv[1], build)
        return wine(build / "replay_native_test.exe", ["--core", *argv])
    if group == "files":
        adapter = Path(os.environ.get("BT_FILES_ADAPTER", build / "files_adapter.exe"))
        return wine(adapter, argv)
    print("coreadapter: unknown group %s" % group, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
