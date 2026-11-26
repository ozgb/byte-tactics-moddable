"""Fail when a scenario uses a command that docs/commands.md does not list, the verdict tables disagree, or a stored log misses an interface event.

    python3 tools/commands.py --check            # check the repository
    python3 tools/commands.py --check --root DIR # check another tree

Reads the console command table, the order table and the verdict table of docs/commands.md (each found by its fixed
header line), the verdict table of reference/README.md, every reference/scenarios/*.txt, the interface event table of
docs/commands.md section 8 and the stored interface logs reference/commands/*.txt: each event of that table must be in a
stored log and in the log of each scenario its row names, unless its scenarios are `none` and no log holds it, and a
stored log must hold no event the table leaves out. Prints
one line per problem and exits 1 if there are any. Standard library only.
"""

import argparse
import re
import sys
from pathlib import Path

DOC = "docs/commands.md"
README = "reference/README.md"
CONSOLE_HEADER = "| Command | Group | Class | Events |"
ORDER_HEADER = "| Order | Mode | Target | Events |"
VERDICT_HEADER = "| Scenario | Verdict | Reason |"
INTERFACE_HEADER = "| Interface event | Paths | Scenarios |"
INTERFACE_LOGS = "reference/commands"
VERDICT_HEADING = "The scenarios of the core's verdict"
NOT_EVENTS = "direct"


def table(lines: list[str], header: str, after: str = "") -> list[list[str]]:
    """The rows (cells) of the first table whose header line is `header`, after the heading line that ends with `after`."""
    start = 0
    if after:
        found = [i for i, line in enumerate(lines) if line.startswith("#") and line.endswith(after)]
        if not found:
            return []
        start = found[0]
    for i in range(start, len(lines)):
        if lines[i].strip() == header:
            rows = []
            for line in lines[i + 2:]:
                if not line.startswith("|"):
                    break
                rows.append([c.strip() for c in line.strip().strip("|").split("|")])
            return rows
    return []


def words(cell: str) -> list[str]:
    """The backticked words of a cell, lower case."""
    return [w.lower() for w in re.findall(r"`([^`]+)`", cell)]


def scenario_lines(path: Path) -> list[tuple[int, str, list[str]]]:
    """(line number, key, words after the key) of each line of a scenario that is not blank or a comment."""
    out = []
    for number, line in enumerate(path.read_text().splitlines(), 1):
        text = line.strip()
        if text and not text.startswith("#"):
            parts = text.split()
            out.append((number, parts[0], parts[1:]))
    return out


def check(root: Path) -> list[str]:
    """One message for each problem found in the tree at root."""
    problems = []
    doc_path = root / DOC
    if not doc_path.exists():
        return ["%s: missing" % DOC]
    doc = doc_path.read_text().splitlines()
    commands = {}
    for row in table(doc, CONSOLE_HEADER):
        if len(row) >= 2:
            for word in words(row[0]):
                commands[word] = row[1]
    orders = {w for row in table(doc, ORDER_HEADER) for w in words(row[0])}
    if not commands:
        problems.append("%s: no console command table (header %s)" % (DOC, CONSOLE_HEADER))
    if not orders:
        problems.append("%s: no order table (header %s)" % (DOC, ORDER_HEADER))
    doc_verdicts = verdicts(doc, DOC, problems, VERDICT_HEADING)
    readme_path = root / README
    readme = readme_path.read_text().splitlines() if readme_path.exists() else []
    readme_verdicts = verdicts(readme, README, problems, VERDICT_HEADING)

    scenarios = {}
    for path in sorted((root / "reference" / "scenarios").glob("*.txt")):
        scenarios[path.stem] = path
    for label, found in ((DOC, doc_verdicts), (README, readme_verdicts)):
        for name in scenarios:
            if name not in found:
                problems.append("%s: scenario %s has no row in the verdict table" % (label, name))
        for name in found:
            if name not in scenarios:
                problems.append("%s: the verdict table lists %s, which is not a scenario" % (label, name))
    for name in sorted(set(doc_verdicts) | set(readme_verdicts)):
        if doc_verdicts.get(name) != readme_verdicts.get(name):
            problems.append("%s and %s differ on scenario %s" % (DOC, README, name))

    for name, path in scenarios.items():
        judged = doc_verdicts.get(name, ("", ""))[0] == "judged"
        rel = "reference/scenarios/%s.txt" % name
        for number, key, rest in scenario_lines(path):
            where = "%s:%d" % (rel, number)
            if key == "console" and len(rest) >= 2:
                word = rest[1].lstrip("+").lower()
                if word not in commands:
                    problems.append("%s: console command %s is not in the console table of %s" % (where, word, DOC))
                elif commands[word] == NOT_EVENTS and judged:
                    problems.append("%s: console command %s is not an event and the scenario is judged" % (where, word))
            elif key == "order" and len(rest) >= 3:
                kind = rest[2].lstrip("+").lower()
                if kind not in orders:
                    problems.append("%s: order kind %s is not in the order table of %s" % (where, kind, DOC))
            elif key in ("input", "save", "load") and judged:
                problems.append("%s: a %s line in a judged scenario" % (where, key))
    problems.extend(interface_problems(root, doc))
    return problems


def interface_problems(root: Path, doc: list[str]) -> list[str]:
    """What is wrong between the interface event table and the stored interface logs."""
    rows = table(doc, INTERFACE_HEADER)
    if not rows:
        return ["%s: no interface event table (header %s)" % (DOC, INTERFACE_HEADER)]
    logged: dict[str, set[str]] = {}
    for path in sorted((root / INTERFACE_LOGS).glob("*.txt")):
        for line in path.read_text().splitlines()[1:]:
            parts = line.split()
            if len(parts) >= 2:
                logged.setdefault(parts[1], set()).add(path.stem)
    problems = []
    listed = set()
    for row in rows:
        kinds = re.findall(r"`([^`]+)`", row[0])
        named = [w.strip() for w in row[-1].split(",") if w.strip() not in ("", "-")]
        for kind in kinds:
            listed.add(kind)
            if named == ["none"]:
                if kind in logged:
                    problems.append("%s: interface event %s has no scenario but is in %s/%s.txt"
                                    % (DOC, kind, INTERFACE_LOGS, sorted(logged[kind])[0]))
                continue
            if kind not in logged:
                problems.append("%s: interface event %s is in no stored log of %s" % (DOC, kind, INTERFACE_LOGS))
                continue
            for name in named:
                if name not in logged[kind]:
                    problems.append("%s: interface event %s is not in %s/%s.txt" % (DOC, kind, INTERFACE_LOGS, name))
    for kind in sorted(set(logged) - listed):
        problems.append("%s/%s.txt: interface event %s is not in the interface event table of %s"
                        % (INTERFACE_LOGS, sorted(logged[kind])[0], kind, DOC))
    return problems


def verdicts(lines: list[str], label: str, problems: list[str], heading: str) -> dict[str, tuple[str, str]]:
    """The verdict table under `heading` as name -> (verdict, reason); problems gets what is wrong with it."""
    rows = table(lines, VERDICT_HEADER, heading)
    if not rows:
        problems.append("%s: no verdict table (heading %s, header %s)" % (label, heading, VERDICT_HEADER))
    found: dict[str, tuple[str, str]] = {}
    names = []
    for row in rows:
        if len(row) != 3:
            problems.append("%s: a verdict row has %d cells, not 3: %s" % (label, len(row), "|".join(row)))
            continue
        name, verdict, reason = row
        names.append(name)
        if name in found:
            problems.append("%s: scenario %s is listed twice" % (label, name))
        if verdict not in ("judged", "left out"):
            problems.append("%s: scenario %s has the verdict %r, not judged or left out" % (label, name, verdict))
        if verdict == "left out" and reason in ("", "-"):
            problems.append("%s: scenario %s is left out with no reason" % (label, name))
        found[name] = (verdict, reason)
    if names != sorted(names):
        problems.append("%s: the verdict table is not sorted by scenario" % label)
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    problems = check(parser.parse_args().root)
    for line in problems:
        print(line)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
