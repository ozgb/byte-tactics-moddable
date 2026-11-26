"""Decode the interface commands a test-mode run wrote to commands.txt, the form stored in reference/commands/.

    uv run tools/interfacelog.py commands.txt

The `interface` lines of a `bt-commands 2` file hold the raw payload of each event the game's interface sent to the
simulation. The output is `bt-interface 1`, then one line per event: `<tick> <KIND>` and ` <field> <value>` for each
field of the kind in data/trace_schema.json, with unit references as the unit's index (`-` for none, `heap` for a
pointer that is no unit of the table). Scripted lines (spawn, order, console) are left out.
"""

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
COMMANDS_HEADER = "bt-commands 2"
INTERFACE_HEADER = "bt-interface 1"
MAX_UNITS = 65536


def kinds(schema: dict) -> dict[str, dict]:
    """The event kinds of the schema by name."""
    found: dict[str, dict] = {}
    for section in schema.values():
        if isinstance(section, dict) and isinstance(section.get("kinds"), dict):
            for entry in section["kinds"].values():
                found[entry["name"]] = entry
    return found


def unit_ref(value: int, units: tuple[int, int] | None) -> str:
    if value == 0:
        return "-"
    if units is not None:
        offset, stride = units
        index, rest = divmod(value - offset, stride)
        if value >= offset and rest == 0 and index < MAX_UNITS:
            return str(index)
    return "heap"


def decode_record(tick: str, name: str, payload: bytes, entry: dict | None, units: tuple[int, int] | None) -> str:
    if entry is None:
        return f"{tick} {name} payload {payload.hex()}"
    parts = [tick, name]
    offset = 0
    for field in entry["fields"]:
        size = field["size"]
        start, offset = offset, offset + size
        if field["name"] == "kind" or offset > len(payload):
            continue
        raw = payload[start:offset]
        if field["kind"] == "scalar":
            value = str(int.from_bytes(raw, "little", signed=True))
        elif field["kind"] == "ref":
            number = int.from_bytes(raw, "little")
            value = unit_ref(number, units) if field["target"] == "unit" else ("-" if number == 0 else "heap")
        else:
            value = raw.hex()
        parts += [field["name"], value]
    return " ".join(parts)


def decode(commands_text: str, schema: dict) -> str:
    """The `bt-interface 1` text of the interface records of a `bt-commands 2` file."""
    lines = commands_text.splitlines()
    if not lines or lines[0] != COMMANDS_HEADER:
        raise ValueError(f"not a {COMMANDS_HEADER} file")
    by_name = kinds(schema)
    units: tuple[int, int] | None = None
    out = [INTERFACE_HEADER]
    for number, line in enumerate(lines[1:], 2):
        words = line.split()
        try:
            if words[0] == "units":
                units = (int(words[1]), int(words[2]))
                if units[1] <= 0:
                    raise ValueError
                continue
            if words[1] != "interface":
                continue
            int(words[0])
            if words[2] != "-" or words[3] != "1" or len(words) != 6:
                raise ValueError
            name = words[4]
            payload = bytes.fromhex(words[5])
        except (IndexError, ValueError):
            raise ValueError(f"commands.txt line {number}: malformed `{line}`") from None
        out.append(decode_record(words[0], name, payload, by_name.get(name), units))
    return "\n".join(out) + "\n"


def has_input(scenario_text: str) -> bool:
    """Whether a scenario has an `input` line."""
    return any(line.split()[:1] == ["input"] for line in scenario_text.splitlines())


def main() -> None:
    parser = argparse.ArgumentParser(description="Print the decoded interface commands of a commands.txt.")
    parser.add_argument("commands", type=Path)
    args = parser.parse_args()
    schema = json.loads((ROOT / "data/trace_schema.json").read_text())
    try:
        sys.stdout.write(decode(args.commands.read_text(), schema))
    except ValueError as error:
        sys.exit(f"{args.commands}: {error}")


if __name__ == "__main__":
    main()
