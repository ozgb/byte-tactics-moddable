"""Check that docs/sim-events.md has a section for every kind of sim event.

    uv run tools/simeventsdoc.py --check    # fail, naming each problem, if the doc and the kinds differ
    uv run tools/test_simeventsdoc.py       # tests, on synthetic inputs

The kinds are the enumerators of `enum SimEventKind` in interfaces/services.h. The heading convention of the doc: a kind
has a section when a heading of level 2 or 3 (`## NAME` or `### NAME`) is exactly the enumerator's name, with nothing
else on the line. The check fails for:

- a kind with no such heading;
- a heading that starts with `SIM_EVENT_` and is not the name of a kind (a section that names no kind);
- a kind with two headings.

Other headings are free. The prose of the sections is not checked.
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "interfaces" / "services.h"
DOC = ROOT / "docs" / "sim-events.md"

ENUM = re.compile(r"enum\s+SimEventKind\b[^{]*\{(.*?)\}", re.S)
ENUMERATOR = re.compile(r"^\s*(SIM_EVENT_[A-Z0-9_]+)\s*(?:=[^,]*)?,", re.M)
HEADING = re.compile(r"^(#{2,3})\s+(.*?)\s*$")


def kinds(header: str) -> list[str]:
    """Return the enumerator names of SimEventKind, in order."""
    match = ENUM.search(re.sub(r"//[^\n]*", "", header))
    if match is None:
        sys.exit("simeventsdoc: no enum SimEventKind in the header")
    return ENUMERATOR.findall(match.group(1))


def headings(doc: str) -> list[str]:
    """Return the text of the level 2 and 3 headings, outside code fences."""
    out = []
    fenced = False
    for line in doc.splitlines():
        if line.startswith("```"):
            fenced = not fenced
        elif not fenced:
            match = HEADING.match(line)
            if match:
                out.append(match.group(2))
    return out


def problems(names: list[str], doc: str) -> list[str]:
    """Return one message for each way the doc and the kinds differ."""
    found = headings(doc)
    known = set(names)
    out = []
    for name in names:
        count = found.count(name)
        if count == 0:
            out.append("kind %s has no section (add `### %s`)" % (name, name))
        elif count > 1:
            out.append("kind %s has %d sections" % (name, count))
    for text in found:
        if text.startswith("SIM_EVENT_") and text not in known:
            out.append("section %r names no kind of SimEventKind" % text)
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if a kind has no section or a section names no kind")
    parser.add_argument("--header", type=Path, default=HEADER, help="the header (default interfaces/services.h)")
    parser.add_argument("--doc", type=Path, default=DOC, help="the doc (default docs/sim-events.md)")
    args = parser.parse_args()
    if not args.check:
        sys.exit("simeventsdoc: pass --check")
    names = kinds(args.header.read_text())
    found = problems(names, args.doc.read_text() if args.doc.exists() else "")
    for message in found:
        print("simeventsdoc: " + message)
    if found:
        sys.exit(1)
    print("simeventsdoc: %d kinds, each with one section in %s" % (len(names), args.doc.name))


if __name__ == "__main__":
    main()
