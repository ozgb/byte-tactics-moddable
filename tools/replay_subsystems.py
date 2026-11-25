"""Reads tests/replay_subsystems.json for tools/ci_replays.sh and checks it against tests/replay/.

    uv run tools/replay_subsystems.py check
    uv run tools/replay_subsystems.py list inject|self|wasm|wasm-self|wasm-inject|native
"""

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LIST = ROOT / "tests" / "replay_subsystems.json"
RECORDINGS = ROOT / "tests" / "replay"


def injects(entry):
    """The injected changes of an entry: its `inject`, one object or a list of them."""
    inject = entry["inject"]
    return inject if isinstance(inject, list) else [inject]


def load(path=LIST):
    return json.loads(Path(path).read_text())


def check(entries, recordings=RECORDINGS):
    errors = []
    directories = sorted(p.name for p in Path(recordings).iterdir() if p.is_dir())
    for name in directories:
        if name not in entries:
            errors.append(f"tests/replay/{name}/ has no entry in tests/replay_subsystems.json")
    for name, entry in entries.items():
        directory = Path(recordings) / name
        if not directory.is_dir():
            errors.append(f"tests/replay_subsystems.json entry {name} has no directory tests/replay/{name}/")
            continue
        if not list(directory.glob("*.txt")):
            errors.append(f"tests/replay/{name}/ holds no recordings")
        wanted = [inject["recording"] for inject in injects(entry)] + ([entry["self_test"]] if "self_test" in entry else [])
        for recording in wanted:
            if not (directory / recording).is_file():
                errors.append(f"tests/replay/{name}/{recording} does not exist")
    return errors


def log_name(name, entry, kind):
    stem = kind.replace("-", "_")
    default = f"replay_{name}" if kind == "native" else f"replay_{name}_{stem}"
    return entry.get("log_names", {}).get(stem, default)


def rows(entries, kind):
    out = []
    for name, entry in entries.items():
        inject = injects(entry)[0]
        wasm = entry.get("wasm")
        log = log_name(name, entry, kind)
        if kind == "native":
            out.append([log, name])
        elif kind == "self" and "self_test" in entry:
            out.append([log, name, entry["self_test"]])
        elif kind == "inject":
            for i, each in enumerate(injects(entry)):
                out.append([log if i == 0 else f"{log}_{i + 1}", name, str(each["expect"]), each["field"], each["recording"]])
        elif kind == "wasm" and wasm:
            out.append([log, name, wasm["module"]])
        elif kind == "wasm-self" and wasm and "self_test" in entry:
            out.append([log, name, wasm["module"], entry["self_test"]])
        elif kind == "wasm-inject" and wasm:
            out.append([log, name, wasm["inject_variant"], str(inject["expect"]), inject["field"], inject["recording"]])
    return out


def main(argv):
    if argv[:1] == ["check"]:
        errors = check(load())
        for e in errors:
            print(f"replay_subsystems: {e}", file=sys.stderr)
        return 1 if errors else 0
    if len(argv) == 2 and argv[0] == "list":
        for row in rows(load(), argv[1]):
            sys.stdout.buffer.write("\t".join(row).encode() + b"\n")
        return 0
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
