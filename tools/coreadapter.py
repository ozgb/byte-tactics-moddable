"""The in-repo engine's answer to the core gate's command line (docs/core-gate.md, #1041).

    uv run tools/coreadapter.py GROUP CALLS OUT
    uv run tools/coreadapter.py files --game DIR CALLS OUT

It is the in-repo engine as one program: `hash` is tools/tracerehash.py, `commands` delivers the commands of a script in
the order the harness delivers them, `rand` and `events` are build/modern/core_state_adapter.exe, `loader` and `fuel`
build/modern/core_wasm_adapter.exe, `host` the WASM replay of build/modern/replay_native_test.exe, `files`
build/modern/files_adapter.exe, `console` the game's console code in build/modern/core_console_adapter.exe and `runtime`
lib/bitexact and LIBCMT in build/modern/core_runtime_adapter.exe, each under Wine. The console adapter writes each
event's kind as a number, and this program names it from data/trace_schema.json; the runtime adapter reads and writes
binary records, and this program turns the calls into them and the results back. BT_CORE_BUILD names another build
directory, and BT_FILES_ADAPTER another files adapter.

BT_CORE_FAULT injects one fault for the negative tests of tools/test_coregate.py: `hash_step` (the word step of the hash
multiplies by another prime), `commands_swap` (the first two deliveries of a tick change places), and the faults of the
adapters (`rand_skip`, `events_continuation`, `loader_class`, `fuel_budget`, `host_write`), which the exes read,
`console_byte` (a player argument of 256 or more is read as a player that does not exist, as by a core that does not cut
it to a byte) and `runtime:ENTRY` (the first result of that runtime entry is wrong).
"""

from __future__ import annotations

import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import coregate
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
    try:
        found = tracerehash.rehash(sch, tracerehash.parse_blocks(tracerehash.read_text(dump), sch.data["trace_format"]))
    except tracerehash.Error:
        raise EndsRun({"error": "dump"})
    return [[tick, phase, "%016x" % value] for tick, phase, value in found]


class EndsRun(Exception):
    """An error that ends the run (docs/core-gate.md): its value is the call's last line."""

    def __init__(self, value) -> None:
        super().__init__(value)
        self.value = value


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
            try:
                value = handler(*call[1:])
            except EndsRun as e:
                out.write("%s\t%s\n" % (call[0], json.dumps(e.value)))
                return 0
            out.write("%s\t%s\n" % (call[0], json.dumps(value)))
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


def event_names() -> dict[int, str]:
    schema = json.loads(coregate.SCHEMA.read_text())
    names = {}
    for section in schema.values():
        if isinstance(section, dict) and isinstance(section.get("kinds"), dict):
            for kind, entry in section["kinds"].items():
                names[int(kind)] = entry["name"]
    return names


def player_not_cut(text: bytes) -> bytes:
    words = text.split()
    if len(words) > 1 and words[0].lower() != b"now":
        digits = words[1].lstrip(b"+-")
        n = len(digits) - len(digits.lstrip(b"0123456789"))
        value = int(digits[:n] or b"0") * (-1 if words[1].startswith(b"-") else 1)
        if not 0 <= value < 256:
            return text.replace(words[1], b"10", 1)
    return text


def answer_console(build: Path, calls_path: str, out_path: str) -> int:
    calls = read_calls(calls_path)
    if fault() == "console_byte":
        calls = [c[:1] + [player_not_cut(bytes.fromhex(c[1])).hex()] if c[0] == "line" else c for c in calls]
    with tempfile.TemporaryDirectory(prefix="coreadapter-") as tmp:
        cpath, opath = Path(tmp) / "calls", Path(tmp) / "out"
        cpath.write_text("".join("\t".join(c) + "\n" for c in calls), newline="")
        status = wine(build / "core_console_adapter.exe", ["console", str(cpath), str(opath)])
        if status != 0:
            return status
        lines = opath.read_text(encoding="latin-1").splitlines()
    names = event_names()
    with open(out_path, "w", encoding="latin-1", newline="\n") as out:
        for line in lines:
            name, _, text = line.partition("\t")
            value = json.loads(text)
            if name == "line":
                value["events"] = [[names.get(kind, "KIND_%d" % kind), data] for kind, data in value["events"]]
            out.write("%s\t%s\n" % (name, json.dumps(value)))
    return 0


def encode_field(kind: str, value) -> bytes:
    if kind in coregate.FIXED:
        return struct.pack(coregate.FIXED[kind], value)
    data = bytes.fromhex(value)
    return struct.pack("<I", len(data)) + data if kind == "str" else data


def wrong(outputs: list) -> list:
    first = outputs[0]
    if isinstance(first, int):
        return [first ^ 1] + outputs[1:]
    return [("%02x" % (int(first[:2] or "00", 16) ^ 1)) + first[2:]] + outputs[1:]


def answer_runtime(build: Path, calls_path: str, out_path: str) -> int:
    """Each run of calls of one entry goes to the runtime adapter as one file of records."""
    layouts = coregate.runtime_layouts()
    calls = read_calls(calls_path)
    runs: list[list[list[str]]] = []
    for call in calls:
        if call[0] not in layouts:
            print("coreadapter: no runtime entry %s" % call[0], file=sys.stderr)
            return 2
        if runs and runs[-1][0][0] == call[0]:
            runs[-1].append(call)
        else:
            runs.append([call])
    with open(out_path, "w", encoding="latin-1", newline="\n") as out, \
            tempfile.TemporaryDirectory(prefix="coreadapter-") as tmp:
        for run in runs:
            entry = run[0][0]
            ins, outs = layouts[entry]
            ipath, opath = Path(tmp) / "in.bin", Path(tmp) / "out.bin"
            ipath.write_bytes(b"".join(encode_field(k, v) for c in run for k, v in zip(ins, json.loads(c[1]))))
            status = wine(build / "core_runtime_adapter.exe", [entry, str(ipath), str(opath)])
            if status != 0:
                return status
            data, pos = opath.read_bytes(), 0
            for n in range(len(run)):
                outputs = []
                for kind in outs:
                    value, pos = coregate.read_field(data, pos, kind)
                    outputs.append(value)
                if n == 0 and fault() == "runtime:" + entry:
                    outputs = wrong(outputs)
                out.write("%s\t%s\n" % (entry, json.dumps(outputs)))
    return 0


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
    if group == "console":
        return answer_console(build, argv[1], argv[2])
    if group == "runtime":
        return answer_runtime(build, argv[1], argv[2])
    if group == "files":
        adapter = Path(os.environ.get("BT_FILES_ADAPTER", build / "files_adapter.exe"))
        return wine(adapter, argv)
    print("coreadapter: unknown group %s" % group, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
