"""Check data/crt_semantics.csv, the audit of the C semantics of game-logic runtime calls (#1060).

    uv run tools/crtsemantics.py --check      # fail on a missing, stale or malformed row
    uv run tools/crtsemantics.py --summary    # per callee: spec and drop counts, union of spec inputs
    uv run tools/test_crtsemantics.py         # tests, on fixture files

The C runtime functions in GROUP behave differently from the obvious Rust equivalent on
some inputs (white space and signs before a number, high bytes, the zero fill of
`strncpy`, the hidden state of `strtok`, ...). data/crt_semantics.csv has one row per
call row of data/crt_calls.csv whose `class` is `game` and whose callee is in GROUP, and
says whether the clean-room spec must describe the C behaviour of that call.

Columns (COLUMNS), one row per (address, callee), sorted by address then callee:

  address  the function's address, as in data/crt_calls.csv (`0x4010b0`)
  callee   the runtime function
  reaches  where the result goes, the first that applies (REACHES):
             state  the game state block, or which game logic runs
             save   the content or name of a saved game, or of a file read back into the block
             trace  a test-mode output the gate compares: UI log, sound log, sim events, frames
             none   only debug files, logs for humans, error text, or nothing (result unused)
  inputs   the inputs this site can receive on which C and a straightforward Rust
           implementation differ: empty, or `;`-separated tags, sorted and unique (TAGS):
             whitespace       leading white space before a parsed number
             sign             a leading + or - on a parsed number
             trailing         characters after a parsed number
             empty            no digits where a number is parsed
             overflow         a parsed number outside the result type's range
             negative         a negative number formatted by _itoa or %d, %i, %x, %u
             high-byte        a byte 0x80 to 0xff the call examines
             fold-order       a case-folding comparison whose sign is used, on characters
                              between Z and a
             order            a comparison whose sign, not only zero, is used
             truncate         a source at least as long as the count
             pad              strncpy's zero fill is observable
             overlap          memmove with overlapping source and destination
             nul-search       strchr or strrchr searched for the terminator
             empty-needle     strstr with an empty needle, strcspn with an empty set
             tokenizer-state  strtok's position between calls, delimiter runs, a null first argument
             unknown-format   a printf or scanf format that is not a literal
             fmt:<conversion> one per distinct conversion of the site's formats, as written
                              (`fmt:%04x`, `fmt:%-10s`, `fmt:%[a-zA-Z0-9_.]`); only on
                              sprintf and sscanf rows
  verdict  `spec` exactly when reaches is not `none` and inputs is not empty, else
           `drop: <reason>`
"""

import argparse
import csv
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

COLUMNS = ["address", "callee", "reaches", "inputs", "verdict"]

GROUP = frozenset(
    "atoi sprintf sscanf _itoa _strcmpi _strnicmp _strlwr tolower toupper isspace isdigit isalnum "
    "strtok strncpy strncmp strstr strchr strrchr strcspn memmove".split()
)

FORMAT_CALLEES = frozenset({"sprintf", "sscanf"})

REACHES = ("state", "save", "trace", "none")

TAGS = frozenset(
    "whitespace sign trailing empty overflow negative high-byte fold-order order truncate pad overlap "
    "nul-search empty-needle tokenizer-state unknown-format".split()
)

CONVERSION = re.compile(
    r"%\*?(?:[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?[hlL]?[diouxXcsfeEgGnp%]|\d*\[\^?\]?[^\]]*\])\Z"
)


def tag_problem(tag: str, callee: str) -> str:
    """Why `tag` is not allowed on a row of `callee`, or an empty string."""
    if tag.startswith("fmt:"):
        if callee not in FORMAT_CALLEES:
            return f"fmt tag {tag} on {callee}"
        if not CONVERSION.match(tag[4:]):
            return f"malformed conversion {tag}"
        return ""
    return "" if tag in TAGS else f"unknown tag {tag!r}"


def read_rows(path: Path) -> tuple[list[str], list[dict[str, str]], bool]:
    """The header, the rows and whether the file has only `\\n` line ends."""
    raw = path.read_bytes()
    with path.open(newline="") as f:
        reader = csv.reader(f)
        header = next(reader, [])
        rows = [dict(zip(header, r)) | {"_n": str(i)} for i, r in enumerate(reader, 2) if r]
    return header, rows, b"\r" not in raw


def expected_pairs(crt_calls: Path) -> dict[tuple[str, str], str]:
    """(address, callee) to function for the game rows of data/crt_calls.csv in GROUP."""
    with crt_calls.open(newline="") as f:
        return {
            (r["address"], r["callee"]): r["function"]
            for r in csv.DictReader(f)
            if r["class"] == "game" and r["callee"] in GROUP
        }


def check(semantics: Path, crt_calls: Path) -> tuple[list[str], int, int, int]:
    """The problems of `semantics`, and its row, spec and drop counts."""
    problems: list[str] = []
    header, rows, lf = read_rows(semantics)
    if header != COLUMNS:
        problems.append(f"header: expected {','.join(COLUMNS)}, got {','.join(header)}")
        return problems, 0, 0, 0
    if not lf:
        problems.append("line ends: expected \\n only")
    want = expected_pairs(crt_calls)
    seen: set[tuple[str, str]] = set()
    spec = drop = 0
    for r in rows:
        key = (r["address"], r["callee"])
        where = f"line {r['_n']}: {key[0]} {key[1]}"
        if key in seen:
            problems.append(f"duplicate: {key[0]} {key[1]}")
        seen.add(key)
        if key not in want:
            problems.append(f"stale: {key[0]} {key[1]}")
        if r["reaches"] not in REACHES:
            problems.append(f"{where}: reaches {r['reaches']!r} is not one of {', '.join(REACHES)}")
        tags = r["inputs"].split(";") if r["inputs"] else []
        if r["inputs"] and "" in tags:
            problems.append(f"{where}: empty tag in inputs")
        for t in tags:
            msg = tag_problem(t, r["callee"]) if t else ""
            if msg:
                problems.append(f"{where}: {msg}")
        if tags != sorted(set(tags)):
            problems.append(f"{where}: inputs not sorted and unique")
        verdict = r["verdict"]
        should_spec = r["reaches"] in REACHES and r["reaches"] != "none" and bool(tags)
        if verdict == "spec":
            spec += 1
            if not should_spec:
                why = "reaches none" if r["reaches"] == "none" else "empty inputs"
                problems.append(f"{where}: spec with {why}")
        elif verdict.startswith("drop:"):
            drop += 1
            if not verdict[5:].strip():
                problems.append(f"{where}: drop with no reason")
            elif should_spec:
                problems.append(f"{where}: drop with reaches {r['reaches']} and inputs {r['inputs']}")
        else:
            problems.append(f"{where}: verdict {verdict!r} is neither spec nor drop: <reason>")
    for key in sorted(want, key=lambda k: (int(k[0], 16), k[1])):
        if key not in seen:
            problems.append(f"missing: {key[0]} {key[1]} ({want[key]})")
    order = [(int(r["address"], 16), r["callee"]) for r in rows if re.fullmatch(r"0x[0-9a-fA-F]+", r["address"])]
    if order != sorted(order):
        problems.append("rows not sorted by address then callee")
    return problems, len(rows), spec, drop


def summary(semantics: Path) -> list[str]:
    """One line per callee: spec and drop counts, then the union of the inputs of its spec rows."""
    _, rows, _ = read_rows(semantics)
    counts: dict[str, list[int]] = defaultdict(lambda: [0, 0])
    union: dict[str, set[str]] = defaultdict(set)
    for r in rows:
        if r["verdict"] == "spec":
            counts[r["callee"]][0] += 1
            union[r["callee"]].update(t for t in r["inputs"].split(";") if t)
        else:
            counts[r["callee"]][1] += 1
    return [
        f"{c}: spec {s}, drop {d}, inputs {';'.join(sorted(union[c]))}" for c, (s, d) in sorted(counts.items())
    ]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--summary", action="store_true")
    ap.add_argument("--csv", type=Path, default=ROOT / "data" / "crt_semantics.csv")
    ap.add_argument("--calls", type=Path, default=ROOT / "data" / "crt_calls.csv")
    args = ap.parse_args()
    if args.summary:
        print("\n".join(summary(args.csv)))
        return 0
    if not args.check:
        ap.error("pass --check or --summary")
    if not args.csv.exists():
        print(f"{args.csv}: not found")
        return 1
    problems, n, spec, drop = check(args.csv, args.calls)
    if problems:
        print("\n".join(problems))
        return 1
    print(f"crt_semantics: {n} rows, {spec} spec, {drop} dropped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
