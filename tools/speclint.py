"""Refuse a behavioural spec that carries the decompilation's addresses, names or text (#798).

    uv run tools/speclint.py SPEC.md --kit DIR              # DIR: an oraclekit.py export
    uv run tools/speclint.py SPEC.md --kit DIR --run 8      # the longest token run allowed to match src/
    uv run tools/test_speclint.py                           # tests

Runs on a spec writer's machine, because it reads src/, include/ and lib/. A spec is refused for:

  * an address of the exe (0x4xxxxx or 0x5xxxxx), a FUN_ or DAT_ name, or a FUNCTION: or GLOBAL: marker;
  * an identifier (a name with an underscore, an inner capital or a digit) found in the source but nowhere in the kit;
  * a fenced code block whose info string isn't `text`;
  * a run of more than --run tokens that also appears in a source file, unless it holds fewer than three
    distinct words (a list of kinds such as `int, int, int, int` is not a copy).

String literals and comments are taken out of the source first, and a double-quoted span out of the spec only when its
content is observable data: a string literal of the source or text found in the kit. Any other quoted span is linted
as ordinary text. Prints one line per problem and exits 1 when there is any.
"""

from __future__ import annotations

import argparse
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
        for word in sorted(identifiers(unquoted(line, literals, kit)) & source_ids - kit_ids):
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
    for number, message in problems:
        print(f"{args.spec}:{number}: {message}")
    if problems:
        print(f"speclint: {len(problems)} problems", file=sys.stderr)
        return 1
    print(f"speclint: {args.spec} passes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
