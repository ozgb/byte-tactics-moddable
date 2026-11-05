"""Make the digest of a sim event log (docs/events-log.md, "The digest"), standard library only.

    uv run tools/eventdigest.py events.txt                      # print the digest
    uv run tools/eventdigest.py events.txt --against stored.txt # compare it with a stored digest

The digest has one line for each run of records with the same tick and phase: the tick, the phase, the number of
records, and the 64-bit hash of the records' bytes (the hash of docs/trace-format.md, section 3). With --against,
the first line that differs is reported with both lines, and the exit status is 1 when any differs.
"""

import argparse
import sys
from pathlib import Path

from tracerehash import Hasher

DIGEST_HEADER = "bt-event-digest 1"
LOG_HEADER_PREFIX = "log "


class Error(Exception):
    pass


def digest_text(log: str) -> str:
    """The digest of a full event log: two header lines, then `<tick> <phase> <count> <hash>` for each run of records of one tick and phase."""
    lines = log.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    if not lines or not lines[0].startswith("bt-events "):
        raise Error("not a sim event log: the first line is not `bt-events <version>`")
    out = [DIGEST_HEADER, LOG_HEADER_PREFIX + lines[0]]
    key: tuple[str, str] | None = None
    count = 0
    chunk: list[str] = []

    def flush() -> None:
        if key is not None:
            hasher = Hasher()
            hasher.run("".join(chunk).encode("latin-1"))
            out.append(f"{key[0]} {key[1]} {count} {hasher.h:016x}")

    for number, line in enumerate(lines[1:], 2):
        words = line.split(" ", 2)
        if len(words) < 3 or not words[0].isdigit():
            raise Error(f"line {number} has no tick and phase: `{line}`")
        if (words[0], words[1]) != key:
            flush()
            key, count, chunk = (words[0], words[1]), 0, []
        count += 1
        chunk.append(line + "\n")
    flush()
    return "\n".join(out) + "\n"


def where(line: str) -> str:
    words = line.split(" ")
    if len(words) >= 4 and words[0].isdigit():
        return f" (tick {words[0]}, phase {words[1]})"
    return ""


def digest_difference(stored: str, got: str) -> str | None:
    """Where two digests first differ (a line number, its tick and phase, both lines with their counts and hashes), or None when they are equal."""
    want, have = stored.splitlines(), got.splitlines()
    for number, (a, b) in enumerate(zip(want, have), 1):
        if a != b:
            return f"events digest line {number}{where(a) or where(b)}: stored `{a}`, got `{b}`"
    if len(want) != len(have):
        number = min(len(want), len(have)) + 1
        longer, which = (want, "stored") if len(want) > len(have) else (have, "got")
        return f"events digest line {number}{where(longer[number - 1])}: only the {which} digest has it (`{longer[number - 1]}`)"
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log", help="a full event log (events.txt)")
    ap.add_argument("--against", metavar="DIGEST", help="compare with this stored digest instead of printing")
    args = ap.parse_args()
    try:
        text = digest_text(Path(args.log).read_text(encoding="latin-1"))
    except (Error, OSError) as e:
        print(f"eventdigest: {e}", file=sys.stderr)
        return 2
    if args.against is None:
        sys.stdout.write(text)
        return 0
    diff = digest_difference(Path(args.against).read_text(), text)
    print(diff or f"digests equal ({len(text.splitlines()) - 2} lines)")
    return 1 if diff else 0


if __name__ == "__main__":
    sys.exit(main())
