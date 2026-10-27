"""Check the numbers in docs/subsystems.md against tools/coupling.py, and rewrite them.

    uv run tools/subsystemsdoc.py --check     # fail on any number of the doc that differs from the tool
    uv run tools/subsystemsdoc.py --write     # rewrite those numbers in place, and nothing else
    uv run tools/test_subsystemsdoc.py        # tests, on a fixture doc and hand-written counts

The counts come from the functions `uv run tools/coupling.py --summary` uses. Per subsystem the doc
states: the function count (the table row, `Issue #N. 16 functions.`, `**units** (193 functions).`),
the functions called from other subsystems (`11 functions are called from other subsystems`,
`Exports: 11 functions`, `68 functions are called from outside`), the imported functions and their
subsystems (`**Imports.** 14 functions of 2 subsystems`, `Needs from seams: 14 functions in platform,
weapons`, `It imports 179 functions`) and the sim items written (`**Sim writes.** 24 items in all`,
`(250 in all)`). It also states the overall function and subsystem counts. A subsystem without a
section, a section without a subsystem and a statement a section lacks are mismatches too. Prose and
numbers the tool does not count (busiest callers, coverage, items left after the filters) are
left alone.
"""

import argparse
import re
import sys
from pathlib import Path

import coupling

DOC = Path(__file__).resolve().parent.parent / "docs" / "subsystems.md"
REMAINING = "The remaining subsystems"

OVERALL = re.compile(r"The (\d+) functions of `data/rwsets\.csv` are in (\d+) subsystems")
TABLE_ROW = re.compile(r"^\| \[(\w+)\]\(#[^)]*\) \| (\d+) \|")
HEADING = re.compile(r"^## (.*)$")
PARAGRAPH = re.compile(r"^\*\*(\w+)\*\* \((\d+) functions\)\.")

SECTION_RULES = [
    ("function count", re.compile(r"^Issue #\d+\. (\d+) functions\."), ["functions"]),
    ("called functions", re.compile(r"^(\d+) functions are called from other subsystems\."), ["called"]),
    ("imported functions and subsystems", re.compile(r"^\*\*Imports\.\*\* (\d+) functions of (\d+) subsystems"),
     ["imported", "targets"]),
    ("sim writes", re.compile(r"^\*\*Sim writes\.\*\* (\d+) items in all"), ["written"]),
    ("exports", re.compile(r"^- Exports: (\d+) functions"), ["called"]),
    ("needs from seams", re.compile(r"^- Needs from seams: (\d+) functions in (.*)\.$"), ["imported", "names"]),
]
PARAGRAPH_RULES = [
    ("called functions", re.compile(r"(?<![\d,])(\d+) functions are called from outside"), ["called"], False),
    ("imported functions", re.compile(r"It imports (\d+) functions"), ["imported"], False),
    ("sim writes", re.compile(r"sim items \((\d+) in all\)"), ["written"], False),
]
Problem = tuple[str, str, str, str]


def expected(counts: dict, key: str):
    if key == "targets":
        return len(counts["targets"])
    if key == "names":
        return counts["targets"]
    return counts[key]


def apply_rule(line: str, rule: tuple, counts: dict, section: str, problems: list[Problem]) -> str:
    """The line with the numbers of one rule replaced, and a problem for each that differed."""
    what, pattern, keys = rule[:3]
    match = pattern.search(line)
    if not match:
        return line
    out, last = [], 0
    for group, key in enumerate(keys, 1):
        want = expected(counts, key)
        have = match.group(group)
        if key == "names":
            names = [n.strip() for n in have.split(",")]
            if set(names) == set(want):
                continue
            have, want = ", ".join(names), ", ".join(want)
        elif int(have) == want:
            continue
        else:
            want = str(want)
        problems.append((section, what, have, want))
        out.append(line[last:match.start(group)] + want)
        last = match.end(group)
    return "".join(out) + line[last:]


def process(text: str, counts: dict[str, dict], total: int) -> tuple[str, list[Problem]]:
    """The doc with every number the tool knows corrected, and the mismatches found."""
    problems: list[Problem] = []
    lines = text.split("\n")
    section = None
    seen: dict[str, set] = {}
    in_table, tabled = False, set()
    for i, line in enumerate(lines):
        heading = HEADING.match(line)
        if heading:
            section = heading.group(1)
            if section in counts:
                seen[section] = set()
            continue
        overall = OVERALL.search(line)
        if overall:
            for what, have, want in (("function total", overall.group(1), total), ("subsystem total", overall.group(2), len(counts))):
                if int(have) != want:
                    problems.append(("overall", what, have, str(want)))
            lines[i] = line[:overall.start(1)] + str(total) + line[overall.end(1):overall.start(2)] + str(len(counts)) + line[overall.end(2):]
            continue
        row = TABLE_ROW.match(line)
        if row:
            name = row.group(1)
            if name in counts:
                tabled.add(name)
                if int(row.group(2)) != counts[name]["functions"]:
                    problems.append((name, "table row function count", row.group(2), str(counts[name]["functions"])))
                    lines[i] = line[:row.start(2)] + str(counts[name]["functions"]) + line[row.end(2):]
            continue
        if section in counts:
            for rule in SECTION_RULES:
                if rule[1].search(line):
                    seen[section].add(rule[0] + rule[1].pattern)
                lines[i] = line = apply_rule(line, rule, counts[section], section, problems)
        elif section == REMAINING:
            paragraph = PARAGRAPH.match(line)
            if paragraph and paragraph.group(1) in counts:
                name = paragraph.group(1)
                seen[name] = {"paragraph"}
                if int(paragraph.group(2)) != counts[name]["functions"]:
                    problems.append((name, "function count", paragraph.group(2), str(counts[name]["functions"])))
                    line = line[:paragraph.start(2)] + str(counts[name]["functions"]) + line[paragraph.end(2):]
                for rule in PARAGRAPH_RULES:
                    line = apply_rule(line, rule, counts[name], name, problems)
                lines[i] = line
    for name in counts:
        if name not in seen:
            problems.append((name, "section", "none", "a section"))
        elif seen[name] != {"paragraph"} and len(seen[name]) != len(SECTION_RULES):
            for rule in SECTION_RULES:
                if rule[0] + rule[1].pattern not in seen[name]:
                    problems.append((name, rule[0], "missing", "present"))
        if name not in tabled:
            problems.append((name, "table row", "none", "a row"))
    return "\n".join(lines), problems


def load_counts() -> tuple[dict[str, dict], int]:
    subsystems, result, _, _ = coupling.generate(coupling.read_overrides())
    return coupling.subsystem_counts(result, subsystems), len(subsystems)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail on any number that differs from the tool")
    parser.add_argument("--write", action="store_true", help="rewrite the numbers that differ")
    parser.add_argument("--doc", type=Path, default=DOC, help="the doc to read (default docs/subsystems.md)")
    args = parser.parse_args()
    if args.check == args.write:
        sys.exit("subsystemsdoc: pass exactly one of --check and --write")
    counts, total = load_counts()
    text = args.doc.read_text()
    new, problems = process(text, counts, total)
    for section, what, have, want in problems:
        print("subsystemsdoc: %s, %s: doc has %s, tool has %s" % (section, what, have, want))
    unwritable = [p for p in problems if p[2] in ("none", "missing")]
    if args.write:
        if new != text:
            args.doc.write_text(new)
        print("subsystemsdoc: rewrote %d numbers in %s" % (len(problems) - len(unwritable), args.doc.name))
    if problems and (args.check or unwritable):
        if args.check:
            print("subsystemsdoc: %s is out of date; run `uv run tools/subsystemsdoc.py --write` and commit the result" % args.doc.name)
        sys.exit(1)
    print("subsystemsdoc: %s matches the tool (%d functions, %d subsystems)" % (args.doc.name, total, len(counts)))


if __name__ == "__main__":
    main()
