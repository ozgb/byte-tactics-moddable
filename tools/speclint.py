"""Refuse a behavioural spec that carries the decompilation's addresses, names or text (#798).

    uv run tools/speclint.py SPEC.md --kit DIR              # DIR: an oraclekit.py export
    uv run tools/speclint.py SPEC.md --kit DIR --run 8      # the longest token run allowed to match src/
    uv run tools/test_speclint.py                           # tests

Runs on a spec writer's machine, because it reads src/, include/ and lib/. A spec is refused for:

  * an address of the exe (0x4xxxxx or 0x5xxxxx), a FUN_ or DAT_ name, or a FUNCTION: or GLOBAL: marker;
  * an identifier (a name with an underscore, an inner capital or a digit) found in the source but nowhere in the kit;
  * a fenced code block whose info string isn't `text`;
  * a run of more than --run tokens that also appears in a source file, unless it holds fewer than three
    distinct words (a list of kinds such as `int, int, int, int` is not a copy);
  * no `### Events` heading between the `## 2.` and `## 3.` headings (a file named TEMPLATE.md is exempt from this and the
    next rule, and its placeholder `SIM_EVENT_NAME` from the rule after, because it describes the subsection);
  * an Events subsection that neither names a `SIM_EVENT_` kind nor holds `None.`, or that names a kind and does not
    contain `docs/sim-events.md`;
  * a `SIM_EVENT_` name anywhere that is not a `kind` of the kit's interfaces/sim_events.csv (read as a table; a kit
    without that file is refused once);
  * a use of a term of the Terms section of the kit's docs/sim-events.md (compared in lower case, with hex numbers as their
    values, as whole words) in a spec that nowhere contains the sentence "`docs/sim-events.md`, Terms": a spec that
    uses an internal term cites the section rather than defining it again. The terms are read from the table after the
    page's `## Terms` heading, the one part of the page that is read.

String literals and comments are taken out of the source first, and a double-quoted span out of the spec only when its
content is observable data: a string literal of the source or text found in the kit. Any other quoted span is linted
as ordinary text. Prints one line per problem and exits 1 when there is any.
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCE_DIRS = ("src", "include", "lib")
SOURCE_SUFFIXES = {".c", ".cpp", ".h", ".hpp", ".inc"}
DEFAULT_RUN = 8
MIN_RUN_WORDS = 3

ADDRESS = re.compile(r"\b0x[45][0-9a-fA-F]{5}\b|\b(?:FUN|DAT)_[0-9a-fA-F]{6,8}\b|\b(?:FUNCTION|GLOBAL):")
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
TOKEN = re.compile(r"[A-Za-z_][A-Za-z0-9_]*|0[xX][0-9a-fA-F]+|\d+|\S")
QUOTED = re.compile(r'"[^"\n]*"')
C_LITERALS = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)
FENCE = re.compile(r"^\s*(```+|~~~+)\s*(\S*)")


def is_identifier_like(word: str) -> bool:
    """True for a name that reads as code: an underscore, a capital after a lower-case letter, or a digit."""
    if len(word) < 4:
        return False
    return "_" in word.strip("_") or bool(re.search(r"[a-z][A-Z]", word)) or bool(re.search(r"[A-Za-z]\d", word))


def identifiers(text: str) -> set[str]:
    """Every identifier-like word of a text, numbers such as 0x4000 left whole."""
    return {w for w in TOKEN.findall(text) if IDENTIFIER.fullmatch(w) and is_identifier_like(w)}


def code_only(text: str) -> str:
    """C or C++ source with its comments and string and character literals removed: a literal is observable data."""
    return C_LITERALS.sub(" ", text)


def tokens(text: str) -> list[str]:
    """The tokens of a text, as a C lexer would roughly split them."""
    return TOKEN.findall(text)


def runs(toks: list[str], n: int) -> set[int]:
    """The hashes of every run of n consecutive tokens."""
    return {hash(tuple(toks[i:i + n])) for i in range(len(toks) - n + 1)}


def source_files(root: Path) -> list[Path]:
    """The C and C++ files of the source directories."""
    return sorted(p for d in SOURCE_DIRS if (root / d).is_dir()
                  for p in (root / d).rglob("*") if p.suffix in SOURCE_SUFFIXES and p.is_file())


def kit_text(kit: Path) -> str:
    """All text of a kit directory, binary files left out."""
    parts = []
    for p in sorted(kit.rglob("*")):
        if p.is_file():
            try:
                parts.append(p.read_text(encoding="utf-8"))
            except (UnicodeDecodeError, OSError):
                continue
    return "\n".join(parts)


def unquoted(line: str, literals: set[str], kit: str) -> str:
    """A line with its double-quoted spans blanked out when they are source literals or kit text, else only their quotes."""
    def blank(m: re.Match) -> str:
        inner = m.group(0)[1:-1]
        if inner in literals or (inner and inner in kit):
            return " " * len(m.group(0))
        return " " + inner + " "
    return QUOTED.sub(blank, line)


def lint(spec: str, source_ids: set[str], kit_ids: set[str], source_runs: set[int], run: int,
         literals: set[str], kit: str) -> list[tuple[int, str]]:
    """The problems of a spec's text, as (line number, message)."""
    problems: list[tuple[int, str]] = []
    lines = spec.splitlines()
    for number, line in enumerate(lines, 1):
        for m in ADDRESS.finditer(line):
            problems.append((number, f"address or marker {m.group(0)!r}"))
        for word in sorted((identifiers(unquoted(line, literals, kit)) & source_ids) - kit_ids):
            problems.append((number, f"identifier {word!r} is in the source but not in the kit"))
    in_fence = None
    for number, line in enumerate(lines, 1):
        m = FENCE.match(line)
        if not m:
            continue
        if in_fence is None:
            in_fence = m.group(1)[0]
            if m.group(2) != "text":
                problems.append((number, f"fenced block {m.group(2) or '(no info string)'!r}: only `text` is allowed"))
        elif m.group(1)[0] == in_fence:
            in_fence = None
    spec_tokens = []
    token_lines = []
    for number, line in enumerate(lines, 1):
        for t in tokens(unquoted(line, literals, kit)):
            spec_tokens.append(t)
            token_lines.append(number)
    stretches: list[list[int]] = []
    for i in range(len(spec_tokens) - run):
        if hash(tuple(spec_tokens[i:i + run + 1])) in source_runs:
            if stretches and i <= stretches[-1][1]:
                stretches[-1][1] = i + run
            else:
                stretches.append([i, i + run])
    for start, end in stretches:
        if len({t for t in spec_tokens[start:end + 1] if IDENTIFIER.fullmatch(t) or t.isdigit()}) < MIN_RUN_WORDS:
            continue
        problems.append((token_lines[start], f"a run of {end - start + 1} tokens matches the source: "
                         f"{' '.join(spec_tokens[start:end + 1])!r}"))
    return sorted(problems)


EVENT_KIND = re.compile(r"SIM_EVENT_[A-Z0-9_]+")
HEX = re.compile(r"\b0x[0-9a-fA-F]+\b")
EVENTS_TABLE = "interfaces/sim_events.csv"
EVENTS_PAGE = "docs/sim-events.md"
TERMS_CITATION = "`docs/sim-events.md`, Terms"
SECTION_TWO = re.compile(r"^## 2\.")
SECTION_THREE = re.compile(r"^## 3\.")
EVENTS_HEADING = re.compile(r"^### Events\s*$")


def norm_term(text: str) -> str:
    """A term or text as compared: lower case, hex numbers as decimal, runs of white space as one space."""
    return " ".join(HEX.sub(lambda m: str(int(m.group(0), 16)), text.lower()).split())


def event_kinds(kit: Path) -> set[str] | None:
    """The `kind` column of the kit's interfaces/sim_events.csv, None when the kit has no such file."""
    path = kit / EVENTS_TABLE
    if not path.is_file():
        return None
    with path.open(newline="", encoding="utf-8") as f:
        return {row["kind"] for row in csv.DictReader(f)}


def page_terms(kit: Path) -> list[str]:
    """The terms of the table after `## Terms` in the kit's docs/sim-events.md (the first cell of each row), normalised."""
    path = kit / EVENTS_PAGE
    if not path.is_file():
        return []
    terms = []
    in_terms = False
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("## "):
            in_terms = line.strip() == "## Terms"
        elif in_terms and line.startswith("|"):
            cell = line.strip().strip("|").split("|")[0].strip()
            if cell and cell != "Term" and not set(cell) <= {"-", ":", " "}:
                terms.append(norm_term(cell))
    return terms


def events_section(lines: list[str]) -> tuple[int, list[str]] | None:
    """The line number and text lines of the `### Events` subsection between `## 2.` and `## 3.`, None when absent."""
    inside = False
    start = None
    body: list[str] = []
    for number, line in enumerate(lines, 1):
        if SECTION_TWO.match(line):
            inside = True
        elif SECTION_THREE.match(line):
            break
        elif inside and EVENTS_HEADING.match(line):
            start = number
        elif inside and start is not None and line.startswith("#"):
            break
        elif start is not None:
            body.append(line)
    return None if start is None else (start, body)


def event_problems(spec: str, kinds: set[str] | None, terms: list[str], template: bool = False) -> list[tuple[int, str]]:
    """The problems of a spec's sim event kinds and of its use of the page's terms, as (line number, message)."""
    problems: list[tuple[int, str]] = []
    lines = spec.splitlines()
    section = events_section(lines)
    if not template:
        if section is None:
            problems.append((1, "no `### Events` subsection between `## 2.` and `## 3.`"))
        else:
            start, body = section
            text = "\n".join(body)
            named = EVENT_KIND.findall(text)
            if not named and "None." not in text:
                problems.append((start, "the Events subsection names no `SIM_EVENT_` kind and does not say `None.`"))
            if named and EVENTS_PAGE not in text:
                problems.append((start, f"the Events subsection names kinds and does not cite `{EVENTS_PAGE}`"))
    if kinds is None:
        problems.append((1, f"the kit has no {EVENTS_TABLE}: the sim event kinds cannot be checked"))
    else:
        for number, line in enumerate(lines, 1):
            for name in EVENT_KIND.findall(line):
                if name not in kinds and not (template and name == "SIM_EVENT_NAME"):
                    problems.append((number, f"{name} is not a kind in {EVENTS_TABLE}"))
    if TERMS_CITATION not in spec:
        for number, line in enumerate(lines, 1):
            text = norm_term(line)
            for term in terms:
                if re.search(r"(?<![\w-])" + re.escape(term) + r"(?!\w)", text):
                    problems.append((number, f"the term {term!r} is defined in the Terms section of {EVENTS_PAGE}: "
                                     f"cite {TERMS_CITATION} rather than defining it"))
    return problems


def source_index(root: Path, run: int) -> tuple[set[str], set[int], set[str]]:
    """The identifiers of the source files, the hashes of their runs of run + 1 tokens, and their string literals."""
    ids: set[str] = set()
    hashes: set[int] = set()
    literals: set[str] = set()
    for path in source_files(root):
        raw = path.read_text(encoding="utf-8", errors="replace")
        literals |= {m.group(0)[1:-1] for m in C_LITERALS.finditer(raw) if m.group(0)[0] == '"' and len(m.group(0)) > 2}
        text = code_only(raw)
        ids |= identifiers(text)
        hashes |= runs(tokens(text), run + 1)
    return ids, hashes, literals


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("spec", type=Path)
    ap.add_argument("--kit", type=Path, required=True, help="an exported data kit (tools/oraclekit.py export)")
    ap.add_argument("--run", type=int, default=DEFAULT_RUN, help=f"longest token run allowed to match (default {DEFAULT_RUN})")
    ap.add_argument("--root", type=Path, default=ROOT, help="the repository whose src/, include/ and lib/ are read")
    args = ap.parse_args(argv)
    if not (args.kit / "MANIFEST.json").is_file():
        print(f"speclint: {args.kit} has no MANIFEST.json: pass an oraclekit.py export", file=sys.stderr)
        return 2
    source_ids, source_runs, literals = source_index(args.root, args.run)
    kit = kit_text(args.kit)
    problems = lint(args.spec.read_text(encoding="utf-8"), source_ids, identifiers(kit), source_runs, args.run,
                    literals, kit)
    problems = sorted(problems + event_problems(args.spec.read_text(encoding="utf-8"), event_kinds(args.kit),
                                                page_terms(args.kit), args.spec.name == "TEMPLATE.md"))
    for number, message in problems:
        print(f"{args.spec}:{number}: {message}")
    if problems:
        print(f"speclint: {len(problems)} problems", file=sys.stderr)
        return 1
    print(f"speclint: {args.spec} passes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
