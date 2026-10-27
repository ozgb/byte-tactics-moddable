"""Fail when a hand-written note in data/ cites a source file by line number.

    python3 tools/citecheck.py               # check the repository
    python3 tools/citecheck.py --root DIR    # check another tree

A line range goes stale when code moves, and a function name does not, so a note cites
`Name (file.cpp)`, never `file.cpp:12-34`. NOTES names the note column of each data file
whose text people write. Columns a tool generates from the source (type_dupes.files,
globals.defined, hazards.file and line) are not listed and may hold `file:line` text.
Prints one line per citation and exits 1 if there are any. Standard library only.
"""

import argparse
import csv
import re
import sys
from pathlib import Path

NOTES = {
    "hazards.csv": "note",
    "hazard_edges.csv": "reason",
    "state_classes.csv": "note",
    "trace_coverage.csv": "evidence",
    "simalloc_allow.csv": "reason",
}
CITATION = re.compile(r"\.(?:cpp|h):[0-9]")


def check(root: Path) -> list[str]:
    """One message for each note that has a `file.cpp:N` citation."""
    problems = []
    for name, column in NOTES.items():
        path = root / "data" / name
        if not path.exists():
            problems.append("%s: missing" % path)
            continue
        with open(path, newline="") as f:
            rows = csv.DictReader(f)
            if column not in (rows.fieldnames or []):
                problems.append("data/%s: no column %s" % (name, column))
                continue
            for number, row in enumerate(rows, 2):
                hit = CITATION.search(row[column])
                if hit:
                    problems.append("data/%s row %d, %s: cites a line (%s); cite the function and file instead"
                                    % (name, number, column, row[column][max(0, hit.start() - 30):hit.end() + 10]))
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    problems = check(parser.parse_args().root)
    for line in problems:
        print(line)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
