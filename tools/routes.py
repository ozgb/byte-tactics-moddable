"""data/routes.csv: the scheduler's route table as data, one row per route in the order of src/game/routes.cpp.

    uv run tools/routes.py --write   # regenerate data/routes.csv
    uv run tools/routes.py --check   # exit 1 when the file differs from the routes

Columns: publisher, kind, value, subscriber, delivery. Publisher and subscriber are the lower-case subsystem names of
docs/schedule.md, kind is the enumerator of interfaces/ and value its integer, delivery is synchronous or queued.
"""

from __future__ import annotations

import argparse
import csv
import re
import io
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import schedule
import traceschema

ROOT = schedule.ROOT
OUTPUT = ROOT / "data" / "routes.csv"
COLUMNS = ["publisher", "kind", "value", "subscriber", "delivery"]


def kind_values() -> dict[str, int]:
    """Per enumerator name of every enum of interfaces/: its value; an enumerator without `=` is the one before plus one."""
    out: dict[str, int] = {}
    for header in sorted(traceschema.INTERFACES.glob("*.h")):
        text = re.sub(r"//[^\n]*", "", header.read_text(encoding="latin-1"))
        for m in re.finditer(r"\benum \w+ \{([^{}]*)\};", text):
            value = -1
            for member in m.group(1).split(","):
                name, eq, given = member.partition("=")
                name = name.strip()
                if not name:
                    continue
                try:
                    value = int(given.strip(), 0) if eq else value + 1
                except ValueError:
                    continue
                out[name] = value
    return out


def parse(source: str, values: dict[str, int]) -> list[list[str]]:
    """The rows of a routes source; a kind that no interface enum declares is an error naming it."""
    rows = []
    for publisher, kind, subscriber, sync in schedule.ROUTE_KIND_ROW.findall(source):
        if kind not in values:
            raise ValueError("route kind %s is not an enumerator of any interface" % kind)
        rows.append([publisher.lower(), kind, str(values[kind]), subscriber.lower(),
                     "synchronous" if sync == "1" else "queued"])
    return rows


def rows() -> list[list[str]]:
    return parse((ROOT / schedule.ROUTES).read_text(), kind_values())


def render(table: list[list[str]]) -> str:
    out = io.StringIO()
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(COLUMNS)
    writer.writerows(table)
    return out.getvalue()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true")
    mode.add_argument("--check", action="store_true")
    args = ap.parse_args()
    try:
        text = render(rows())
    except ValueError as e:
        print("routes: %s" % e)
        return 1
    if args.write:
        OUTPUT.write_text(text)
        return 0
    if not OUTPUT.exists() or OUTPUT.read_text() != text:
        print("data/routes.csv differs from src/game/routes.cpp: run uv run tools/routes.py --write")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
