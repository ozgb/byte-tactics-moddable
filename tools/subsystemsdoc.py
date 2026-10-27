"""Check the numbers in docs/subsystems.md against tools/coupling.py and data/coverage.csv, and rewrite them.

    uv run tools/subsystemsdoc.py --check     # fail on any number of the doc that differs from the tools
    uv run tools/subsystemsdoc.py --write     # rewrite those numbers in place, and nothing else
    uv run tools/test_subsystemsdoc.py        # tests, on a fixture doc and hand-written counts

The coupling counts come from `coupling.subsystem_counts`, the functions `uv run tools/coupling.py --summary` uses.
Per subsystem the doc states: the function count (the table row, `Issue #N. 16 functions.`,
`**units** (193 functions).`), the functions called from other subsystems (`11 functions are called from other
subsystems`, `Exports: 11 functions`, `68 functions are called from outside`) with their callers (`- `X` (2 from game)`,
`By calling subsystem`, `by presentation (24), game (22)`, `the busiest are `X` (28)`), the imported functions and
their subsystems (`**Imports.** 14 functions of 2 subsystems`, `Needs from seams: 14 functions in platform, weapons`,
`It imports 179 functions: platform 85`, the `- platform (12): ..., and 8 more` bullets) and the sim items (`**Sim
writes.** 24 items in all, 19 after leaving out`, `**Sim reads.** 10 items only read, 9 named`, `Owns: 18 named`,
`Shares: 1 named`, `(250 in all)`). It also states the overall function and subsystem counts. A subsystem without a
section, a section without a subsystem and a statement a section lacks are mismatches too.

The coverage counts come from data/coverage.csv, which counts functions its own way (live functions only, the
compiler's inline and generated ones included), so they are never mixed with the coupling counts. The doc states them
in `Coverage: 93.8% of functions (15 of 16), ...` lines, in the `Live functions no scenario enters` headers with the
count of each reason bullet under them (and, for a subsystem whose rows have a reason in the csv, the names of each
reason), and in the table of the Coverage section. Prose and numbers neither tool counts are left alone, and the doc
has none.
"""

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

import coupling

DOC = Path(__file__).resolve().parent.parent / "docs" / "subsystems.md"
REMAINING = "The remaining subsystems"
COVERAGE = "Coverage"

OVERALL = re.compile(r"The (\d+) functions of `data/rwsets\.csv` are in (\d+) subsystems")
TABLE_ROW = re.compile(r"^\| \[(\w+)\]\(#[^)]*\) \| (\d+) \|")
COVERAGE_ROW = re.compile(r"^\| (\*\*)?(\w+)(\*\*)? \| (\d+) of (\d+) \| ([\d.]+%|-) \| ([\d.]+%|-) \| ([\d.]+%|-) \| (\d+) \| (\d+) \|$")
HEADING = re.compile(r"^## (.*)$")
PARAGRAPH = re.compile(r"^\*\*(\w+)\*\* \((\d+) functions\)\.")
PCT = r"([\d.]+%|-)"
COVERAGE_LINE = re.compile(r"Coverage: " + PCT + r" of functions \((\d+) of (\d+)\), " + PCT + r" of lines, " + PCT
                           + r" of branches; (\d+) live functions? no scenario enters, and (\d+) more (?:are|is) dead")
NEVER_HEADER = re.compile(r"^\*\*(?:(\w+), functions no scenario enters|Live functions no scenario enters)\*\* "
                          r"\((\d+) of (\d+) after .*; (\d+) are entered, " + PCT + r"\), by reason:$")
LIST_OF = re.compile(r"The list of those (\d+) ")
REASON_BULLET = re.compile(r"^- (.+?) \((\d+)\): (.*)$")
CALLERS_BULLET = re.compile(r"^- `([^`]+)` \(((?:\d+ from \w+)(?:, \d+ from \w+)*)\)$")
BY_CALLING = re.compile(r"^- By calling subsystem \(functions called\): (.*)$")
IMPORT_BULLET = re.compile(r"^- (\w+) \((\d+)\): (.*)$")
SMALLER_BULLET = re.compile(r"^- (\d+) smaller targets?: (.*)$")
BUSIEST = re.compile(r"the busiest are (.*?)\.(?: |$)")
CALLED_BY = re.compile(r"called (?:from outside, (?:mostly )?by|most by) ")
CALLED_BY_ITEM = re.compile(r"(?:, then |, | and )?(\w+) \((\d+)(?: distinct functions)?\)")
IMPORTS = re.compile(r"It imports (\d+) functions")
IMPORT_ITEMS = [(re.compile(r"(\d+) of them from (\w+)"), (2, 1)), (re.compile(r"\b(\w+) \((\d+)\)"), (1, 2)),
                (re.compile(r"\b(\w+) (\d+)\b"), (1, 2))]
CALLERS_HEAD = "The most called, with their callers:"
ITEM = re.compile(r"`([^`]+)` \((?:sim|mixed)\b")

SECTION_RULES = [
    ("function count", re.compile(r"^Issue #\d+\. (\d+) functions\."), ["functions"]),
    ("called functions", re.compile(r"^(\d+) functions are called from other subsystems\."), ["called"]),
    ("imported functions and subsystems", re.compile(r"^\*\*Imports\.\*\* (\d+) functions of (\d+) subsystems"),
     ["imported", "targets"]),
    ("sim writes", re.compile(r"^\*\*Sim writes\.\*\* (\d+) items in all"), ["written"]),
    ("named sim writes", re.compile(r"^\*\*Sim writes\.\*\* \d+ items in all, (\d+) after leaving out"), ["named"]),
    ("sim reads", re.compile(r"^\*\*Sim reads\.\*\* (\d+) items only read, (\d+) named"), ["only_read", "read_named"]),
    ("exports", re.compile(r"^- Exports: (\d+) functions"), ["called"]),
    ("needs from seams", re.compile(r"^- Needs from seams: (\d+) functions in (.*)\.$"), ["imported", "names"]),
    ("owned sim items", re.compile(r"^- Owns: (\d+) named sim items?"), ["owned"]),
    ("shared sim items", re.compile(r"^- Shares: (\d+) named sim items?"), ["shared"]),
]
PARAGRAPH_RULES = [
    ("called functions", re.compile(r"(?<![\d,])(\d+) functions are called from outside"), ["called"], False),
    ("imported functions", re.compile(r"It imports (\d+) functions"), ["imported"], False),
    ("sim writes", re.compile(r"sim items \((\d+) in all\)"), ["written"], False),
]
REQUIRED = {rule[0] + rule[1].pattern for rule in SECTION_RULES} | {"coverage"}
REASON_CODES = {
    "Network only": "network", "Menu or front end only": "menu", "Save and load only": "save-load",
    "Console command, game logic only": "console", "Console command, display, sound, debugger or host only": "console",
    "Console command, host only": "console", "Key, window or screenshot input only": "input", "Shutdown only": "shutdown",
    "Game over only": "game-over", "Error paths only": "error-path", "Test only": "test-only",
    "Moddable-only key": "moddable-only", "Command-line switch only": "command-line", "Developer tool only": "dev-tool",
    "Game data only": "data",
}
UNWRITABLE = ("none", "missing", "unlisted")
UNWRITABLE_KINDS = ("names", "names listed", "in order", "sim item names")
Problem = tuple[str, str, str, str]


def expected(counts: dict, key: str):
    if key == "targets":
        return len(counts["targets"])
    if key == "names":
        return counts["targets"]
    return counts[key]


def splice(line: str, edits: list[tuple[int, int, str]]) -> str:
    """The line with each (start, end, text) span replaced."""
    out, last = [], 0
    for start, end, text in sorted(edits):
        out.append(line[last:start] + text)
        last = end
    return "".join(out) + line[last:]


def apply_rule(line: str, rule: tuple, counts: dict, section: str, problems: list[Problem]) -> str:
    """The line with the numbers of one rule replaced, and a problem for each that differed."""
    what, pattern, keys = rule[:3]
    match = pattern.search(line)
    if not match:
        return line
    edits = []
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
        edits.append((match.start(group), match.end(group), want))
    return splice(line, edits)


def number(match, group: int, want: int, what: str, section: str, problems: list[Problem], edits: list) -> None:
    """Record a problem and an edit when the number of the group is not want."""
    if int(match.group(group)) != want:
        problems.append((section, what, match.group(group), str(want)))
        edits.append((match.start(group), match.end(group), str(want)))


def text(match, group: int, want: str, what: str, section: str, problems: list[Problem], edits: list) -> None:
    """Record a problem and an edit when the text of the group is not want."""
    if match.group(group) != want:
        problems.append((section, what, match.group(group), want))
        edits.append((match.start(group), match.end(group), want))


def ranked(counts: dict[str, int], form: str) -> str:
    """The counts as the doc writes them by the format (`{n} from {name}`), the largest first and ties by name."""
    return ", ".join(form.format(n=n, name=name) for name, n in sorted(counts.items(), key=lambda x: (-x[1], x[0])))


def callers_bullet(line: str, counts: dict, section: str, problems: list[Problem]) -> str:
    """A `- `X` (2 from game)` bullet against the callers of X."""
    match = CALLERS_BULLET.match(line)
    if not match:
        return line
    target = match.group(1)
    want = counts["callers"].get(target)
    if want is None:
        problems.append((section, "callers of " + target, "none", "a caller"))
        return line
    have = {name: int(n) for n, name in (item.split(" from ") for item in match.group(2).split(", "))}
    if have == want:
        return line
    edits: list = []
    text(match, 2, ranked(want, "{n} from {name}"), "callers of " + target, section, problems, edits)
    return splice(line, edits)


def by_calling_bullet(line: str, counts: dict, section: str, problems: list[Problem]) -> str:
    """The `By calling subsystem` bullet against the functions each subsystem calls."""
    match = BY_CALLING.match(line)
    if not match:
        return line
    have = {name: int(n) for name, n in re.findall(r"(\w+) (\d+)", match.group(1))}
    if have == counts["called_by"]:
        return line
    edits: list = []
    text(match, 1, ranked(counts["called_by"], "{name} {n}"), "functions called, by calling subsystem", section, problems, edits)
    return splice(line, edits)


def import_bullet(line: str, counts: dict, section: str, problems: list[Problem], order: list[str]) -> str:
    """A `- platform (12): ..., and 8 more` or `- 6 smaller targets: ai (5)` bullet against the imports of each target."""
    sizes = counts["target_sizes"]
    edits: list = []
    match = IMPORT_BULLET.match(line)
    if match and match.group(1) not in sizes:
        problems.append((section, "imports from " + match.group(1), "unlisted", "a target"))
        return line
    if match:
        target, size = match.group(1), sizes[match.group(1)]
        order.append(target)
        number(match, 2, size, "imports from " + target, section, problems, edits)
        more = re.search(r"and (\d+) more$", line)
        shown = len(re.findall(r"`[^`]+`", line[match.start(3):more.start() if more else len(line)]))
        if more and int(more.group(1)) != size - shown:
            problems.append((section, "imports from %s, and more" % target, more.group(1), str(size - shown)))
            edits.append((more.start(1), more.end(1), str(size - shown)))
        elif not more and shown != size:
            problems.append((section, "imports from %s, names listed" % target, str(shown), str(size)))
        return splice(line, edits)
    match = SMALLER_BULLET.match(line)
    if match:
        items = list(re.finditer(r"(\w+) \((\d+)\)", match.group(2)))
        number(match, 1, len(items), "smaller import targets", section, problems, edits)
        for item in items:
            order.append(item.group(1))
            if item.group(1) not in sizes:
                problems.append((section, "imports from " + item.group(1), "unlisted", "a target"))
            elif int(item.group(2)) != sizes[item.group(1)]:
                problems.append((section, "imports from " + item.group(1), item.group(2), str(sizes[item.group(1)])))
                edits.append((match.start(2) + item.start(2), match.start(2) + item.end(2), str(sizes[item.group(1)])))
        return splice(line, edits)
    return line


def run_items(clause: str, base: int) -> list[tuple[int, int, str, int]]:
    """The (start, end, name, number) of each `name (N)` of a run of items after a clause."""
    items, pos = [], 0
    while True:
        match = CALLED_BY_ITEM.match(clause, pos)
        if not match:
            return items
        items.append((base + match.start(2), base + match.end(2), match.group(1), int(match.group(2))))
        pos = match.end()


def paragraph_counts(line: str, counts: dict, section: str, problems: list[Problem]) -> str:
    """The callers, the busiest functions and the imports by subsystem of a paragraph, against the tools."""
    edits: list = []
    busiest = BUSIEST.search(line)
    if busiest:
        for item in re.finditer(r"`([^`]+)` \((\d+)\)", busiest.group(1)):
            callers = counts["callers"].get(item.group(1))
            if callers is None:
                problems.append((section, "busiest " + item.group(1), "none", "a called function"))
                continue
            if int(item.group(2)) != sum(callers.values()):
                problems.append((section, "busiest " + item.group(1), item.group(2), str(sum(callers.values()))))
                edits.append((busiest.start(1) + item.start(2), busiest.start(1) + item.end(2), str(sum(callers.values()))))
    called = CALLED_BY.search(line)
    if called:
        for start, end, name, have in run_items(line[called.end():], called.end()):
            want = counts["called_by"].get(name, 0)
            if have != want:
                problems.append((section, "functions called by " + name, str(have), str(want)))
                edits.append((start, end, str(want)))
    imports = IMPORTS.search(line)
    if imports:
        rest = line[imports.end():]
        sentence = rest[:rest.find(". ")] if ". " in rest else rest
        for pattern, (name_group, count_group) in IMPORT_ITEMS:
            for item in pattern.finditer(sentence):
                name = item.group(name_group)
                if name not in coupling.SUBSYSTEMS and name not in counts["target_sizes"]:
                    continue
                want = counts["target_sizes"].get(name, 0)
                if int(item.group(count_group)) != want:
                    problems.append((section, "imports from " + name, item.group(count_group), str(want)))
                    edits.append((imports.end() + item.start(count_group), imports.end() + item.end(count_group), str(want)))
    return splice(line, sorted(set(edits)))


def check_order(block: str, order: list[str], counts: dict, section: str, problems: list[Problem]) -> None:
    """The import targets of the doc in the tool's order, the largest first."""
    if block == "imports" and order != counts["targets"]:
        problems.append((section, "import targets in order", ", ".join(order), ", ".join(counts["targets"])))


def coverage_line(line: str, cover: dict, section: str, problems: list[Problem]) -> str:
    """The `Coverage: 93.8% of functions (15 of 16), ...` statement against data/coverage.csv."""
    match = COVERAGE_LINE.search(line)
    if not match:
        return line
    edits: list = []
    for group, key, what in ((1, "fpct", "function coverage"), (4, "lpct", "line coverage"), (5, "bpct", "branch coverage")):
        text(match, group, cover[key], what, section, problems, edits)
    for group, key, what in ((2, "entered", "functions entered"), (3, "functions", "coverage functions"),
                             (6, "never", "functions never entered"), (7, "dead", "dead functions")):
        number(match, group, cover[key], what, section, problems, edits)
    return splice(line, edits)


def reason_names(sentence: str) -> Counter:
    """The names in backticks of the first sentence of a reason bullet, leaving out those in parentheses."""
    names: Counter = Counter()
    depth, quoted, start = 0, False, 0
    for i, char in enumerate(sentence):
        if char == "`":
            if quoted and not depth:
                names[sentence[start:i]] += 1
            quoted, start = not quoted, i + 1
        elif quoted:
            continue
        elif char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
        elif char == "." and not depth and sentence[i + 1:i + 2] in ("", " "):
            break
    return names


def check_reasons(lines: list[str], head: int, cover: dict, section: str, problems: list[Problem]) -> None:
    """The reason bullets under a header: their counts against the never-entered functions, and the names where the csv has reasons."""
    i = head + 1
    while i < len(lines) and not lines[i]:
        i += 1
    bullets = []
    while i < len(lines) and lines[i].startswith("- "):
        match = REASON_BULLET.match(lines[i])
        if match and not match.group(1).startswith("Entered by"):
            bullets.append((i, match))
        i += 1
    if cover["reasons"] is None:
        total = sum(int(m.group(2)) for _, m in bullets)
        if total != cover["never"]:
            problems.append((section, "reason bullets in all", str(total), str(cover["never"])))
        return
    by_code: dict[str, list] = {}
    for index, match in bullets:
        code = REASON_CODES.get(match.group(1))
        if code is None:
            problems.append((section, "reason " + match.group(1), "unlisted", "a reason of data/coverage.csv"))
        else:
            by_code.setdefault(code, []).append((index, match))
    for code, names in cover["reasons"].items():
        want = Counter(names)
        if code not in by_code:
            problems.append((section, "reason " + code, "missing", "a bullet"))
            continue
        listed = sum((reason_names(m.group(3)) for _, m in by_code[code]), Counter())
        if listed != want:
            problems.append((section, "reason %s names" % code, "extra: %s" % (", ".join(sorted((listed - want).elements())) or "none"),
                             "absent: %s" % (", ".join(sorted((want - listed).elements())) or "none")))
        count = sum(int(m.group(2)) for _, m in by_code[code])
        if count == sum(want.values()):
            continue
        if len(by_code[code]) == 1:
            index, match = by_code[code][0]
            problems.append((section, "reason %s count" % code, match.group(2), str(sum(want.values()))))
            lines[index] = lines[index][:match.start(2)] + str(sum(want.values())) + lines[index][match.end(2):]
        else:
            problems.append((section, "reason %s counts in all" % code, str(count), str(sum(want.values()))))
    for code in by_code:
        if code not in cover["reasons"]:
            problems.append((section, "reason " + code, "unlisted", "no function"))


def never_header(lines: list[str], i: int, context: str, cover: dict, section: str, problems: list[Problem]) -> None:
    """The header of a never-entered list and its bullets against data/coverage.csv."""
    match = NEVER_HEADER.match(lines[i])
    name = match.group(1) or context
    if name not in cover:
        return
    c = cover[name]
    edits: list = []
    number(match, 2, c["never"], "functions never entered", name, problems, edits)
    number(match, 3, c["functions"], "coverage functions", name, problems, edits)
    number(match, 4, c["entered"], "functions entered", name, problems, edits)
    text(match, 5, c["fpct"], "function coverage", name, problems, edits)
    lines[i] = splice(lines[i], edits)
    check_reasons(lines, i, c, name, problems)


def process(text_: str, counts: dict[str, dict], total: int, cover: dict[str, dict]) -> tuple[str, list[Problem]]:
    """The doc with every number the tools know corrected, and the mismatches found."""
    problems: list[Problem] = []
    lines = text_.split("\n")
    section = None
    context = None
    seen: dict[str, set] = {}
    in_table, tabled = False, set()
    covered: set[str] = set()
    block = None
    order: list[str] = []
    for i, line in enumerate(lines):
        heading = HEADING.match(line)
        if heading:
            if block and section in counts:
                check_order(block, order, counts[section], section, problems)
            section = context = heading.group(1)
            block = None
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
        if NEVER_HEADER.match(line):
            if section in counts or section == REMAINING:
                never_header(lines, i, context, cover, section, problems)
            continue
        if section == COVERAGE:
            row = COVERAGE_ROW.match(line)
            if row and row.group(2) in cover:
                c = cover[row.group(2)]
                covered.add(row.group(2))
                edits: list = []
                for group, key, what in ((4, "entered", "functions entered"), (5, "functions", "coverage functions"),
                                         (9, "never", "functions never entered"), (10, "dead", "dead functions")):
                    number(row, group, c[key], what, row.group(2), problems, edits)
                for group, key, what in ((6, "fpct", "function coverage"), (7, "lpct", "line coverage"),
                                         (8, "bpct", "branch coverage")):
                    text(row, group, c[key], what, row.group(2), problems, edits)
                lines[i] = splice(line, edits)
            continue
        if section in counts:
            for rule in SECTION_RULES:
                if rule[1].search(line):
                    seen[section].add(rule[0] + rule[1].pattern)
                lines[i] = line = apply_rule(line, rule, counts[section], section, problems)
            if COVERAGE_LINE.search(line) and section in cover:
                seen[section].add("coverage")
                lines[i] = line = coverage_line(line, cover[section], section, problems)
            if block == "imports" and line and not line.startswith(("- ", "**Imports.**")):
                check_order(block, order, counts[section], section, problems)
                block = None
            if line.startswith(("**Sim writes.**", "**Sim reads.**")):
                block = "writes" if line.startswith("**Sim writes.**") else "reads"
            elif block in ("writes", "reads") and line.startswith("`"):
                items = counts[section]["wrote_items" if block == "writes" else "read_items"]
                for item in ITEM.finditer(line):
                    if item.group(1) not in items:
                        problems.append((section, "sim item names", item.group(1),
                                         "an item the subsystem " + ("writes" if block == "writes" else "only reads")))
            elif line.startswith("**Imports.**"):
                block, order = "imports", []
            elif line.endswith(CALLERS_HEAD):
                block = "callers"
            elif block in ("imports", "callers") and line.startswith("- "):
                if block == "callers":
                    line = callers_bullet(line, counts[section], section, problems)
                    lines[i] = by_calling_bullet(line, counts[section], section, problems)
                else:
                    lines[i] = import_bullet(line, counts[section], section, problems, order)
            elif block and line:
                check_order(block, order, counts[section], section, problems)
                block = None
        elif section == REMAINING:
            paragraph = PARAGRAPH.match(line)
            if paragraph and paragraph.group(1) in counts:
                name = context = paragraph.group(1)
                seen[name] = {"paragraph"}
                if int(paragraph.group(2)) != counts[name]["functions"]:
                    problems.append((name, "function count", paragraph.group(2), str(counts[name]["functions"])))
                    line = line[:paragraph.start(2)] + str(counts[name]["functions"]) + line[paragraph.end(2):]
                for rule in PARAGRAPH_RULES:
                    line = apply_rule(line, rule, counts[name], name, problems)
                line = paragraph_counts(line, counts[name], name, problems)
                if name in cover:
                    line = coverage_line(line, cover[name], name, problems)
                    listed = LIST_OF.search(line)
                    if listed and int(listed.group(1)) != cover[name]["never"]:
                        problems.append((name, "list of functions never entered", listed.group(1), str(cover[name]["never"])))
                        line = line[:listed.start(1)] + str(cover[name]["never"]) + line[listed.end(1):]
                lines[i] = line
    for name in counts:
        if name not in seen:
            problems.append((name, "section", "none", "a section"))
        elif seen[name] != {"paragraph"} and seen[name] != REQUIRED:
            for rule in SECTION_RULES:
                if rule[0] + rule[1].pattern not in seen[name]:
                    problems.append((name, rule[0], "missing", "present"))
            if "coverage" not in seen[name] and name in cover:
                problems.append((name, "coverage statement", "missing", "present"))
        if name not in tabled:
            problems.append((name, "table row", "none", "a row"))
    for name in cover:
        if name not in covered:
            problems.append((name, "coverage table row", "none", "a row"))
    return "\n".join(lines), problems


def cover_counts(totals: dict) -> dict[str, dict]:
    """The coverage figures the doc states, per subsystem, from the totals of coverage.read_csv."""
    import coverage

    out = {}
    for name, t in totals.items():
        coded = bool(t.never_entered) and all(t.reasons.get(key) for key in t.never_entered)
        reasons: dict[str, list[str]] = {}
        for key in t.never_entered:
            reasons.setdefault(t.reasons.get(key, ""), []).append(key[0])
        out[name] = {"functions": t.functions, "entered": t.functions_entered, "never": len(t.never_entered), "dead": t.dead,
                     "fpct": coverage.percent(t.functions_entered, t.functions), "lpct": coverage.percent(t.lines_hit, t.lines),
                     "bpct": coverage.percent(t.branches_covered, t.branches), "reasons": reasons if coded else None}
    return out


def load_counts() -> tuple[dict[str, dict], int, dict[str, dict]]:
    import coverage

    subsystems, result, _, _ = coupling.generate(coupling.read_overrides())
    return (coupling.subsystem_counts(result, subsystems), len(subsystems),
            cover_counts(coverage.read_csv(coverage.OUT)))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail on any number that differs from the tools")
    parser.add_argument("--write", action="store_true", help="rewrite the numbers that differ")
    parser.add_argument("--doc", type=Path, default=DOC, help="the doc to read (default docs/subsystems.md)")
    args = parser.parse_args()
    if args.check == args.write:
        sys.exit("subsystemsdoc: pass exactly one of --check and --write")
    counts, total, cover = load_counts()
    text_ = args.doc.read_text()
    new, problems = process(text_, counts, total, cover)
    for section, what, have, want in problems:
        print("subsystemsdoc: %s, %s: doc has %s, tool has %s" % (section, what, have, want))
    unwritable = [p for p in problems if p[2] in UNWRITABLE or p[1].endswith(UNWRITABLE_KINDS) or p[2].startswith("extra: ")]
    if args.write:
        if new != text_:
            args.doc.write_text(new)
        print("subsystemsdoc: rewrote %d numbers in %s" % (len(problems) - len(unwritable), args.doc.name))
    if problems and (args.check or unwritable):
        if args.check:
            print("subsystemsdoc: %s is out of date; run `uv run tools/subsystemsdoc.py --write` and commit the result" % args.doc.name)
        sys.exit(1)
    print("subsystemsdoc: %s matches the tools (%d functions, %d subsystems)" % (args.doc.name, total, len(counts)))


if __name__ == "__main__":
    main()
