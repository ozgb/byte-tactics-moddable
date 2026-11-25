"""Make the digest of a sim event log (docs/events-log.md, "The digest"), standard library only.

    uv run tools/eventdigest.py events.txt                      # print the digest
    uv run tools/eventdigest.py events.txt --against stored.txt # compare it with a stored digest
    uv run tools/eventdigest.py events.txt --against stored.txt --end-of-tick   # compare the `tick` lines only

The digest (`bt-event-digest 2`) has one line for each run of records with the same tick and phase: the tick, the
phase, the number of records, and the 64-bit hash of the records' bytes (the hash of docs/trace-format.md, section 3).
After the last of those lines of a tick comes the tick's own line, `<tick> tick <count> <ordered> <set>`, which leaves
the phase out of every record: `set` is the hash of the tick's records sorted by their bytes, and `ordered` is the
hash of the projections of the log order onto every pair of kinds whose order counts (the `Order:` lines of
docs/sim-events.md, read by this tool). With --against, the first line that differs is reported with both lines, and
the exit status is 1 when any differs; with --end-of-tick only the header lines and the `tick` lines are compared.
"""

import argparse
import re
import struct
import sys
from functools import lru_cache
from pathlib import Path

from tracerehash import Hasher

DIGEST_HEADER = "bt-event-digest 2"
LOG_HEADER_PREFIX = "log "
TICK_PHASE = "tick"
DROPPED = "dropped"
ROOT = Path(__file__).resolve().parent.parent
PAGE = ROOT / "docs/sim-events.md"
ORDER_LINE = re.compile(r"Order: (?P<spec>(?:`[a-z0-9_]+`(?:, `[a-z0-9_]+`)*)|same|none|every): (?P<reason>.+)")


class Error(Exception):
    pass


def hash_bytes(data: bytes) -> int:
    """HB(data): the 64-bit hash of docs/trace-format.md, section 3, applied once to the bytes."""
    hasher = Hasher()
    hasher.run(data)
    return hasher.h


class Classes:
    """The kinds of docs/sim-events.md: their values and the pairs of values whose order counts."""

    def __init__(self, values: dict[str, int], specs: dict[str, tuple[str, list[str]]]) -> None:
        self.values = values
        self.specs = specs
        pairs: set[tuple[int, int]] = set()
        for name, (form, named) in specs.items():
            mine = values[name]
            if form == "same":
                pairs.add((mine, mine))
            elif form == "every":
                pairs.update((min(mine, v), max(mine, v)) for v in values.values())
            elif form == "list":
                pairs.update((min(mine, values[n]), max(mine, values[n])) for n in named)
        self.pairs = sorted(pairs)
        self.partners: dict[int, set[int]] = {}
        for a, b in self.pairs:
            self.partners.setdefault(a, set()).add(b)
            self.partners.setdefault(b, set()).add(a)


def parse_classes(page: str) -> Classes:
    """The kinds, values and `Order:` lines of the text of docs/sim-events.md; an Error for a kind without a value or an `Order:` line, or a line naming a kind the page lacks."""
    values: dict[str, int] = {}
    specs: dict[str, tuple[str, list[str]]] = {}
    kind: str | None = None
    for number, line in enumerate(page.split("\n"), 1):
        heading = re.fullmatch(r"### SIM_EVENT_([A-Z0-9_]+)", line)
        if heading:
            kind = heading.group(1).lower()
            continue
        if kind is None:
            continue
        value = re.fullmatch(r"Value (\d+)\.(?: .*)?", line)
        if value:
            values[kind] = int(value.group(1))
        elif line.startswith("Order:"):
            parsed = ORDER_LINE.fullmatch(line)
            if parsed is None:
                raise Error(f"sim-events.md line {number}: cannot read the order line of `{kind}`: `{line}`")
            if kind in specs:
                raise Error(f"sim-events.md line {number}: `{kind}` has a second order line")
            spec = parsed.group("spec")
            specs[kind] = ("list", re.findall(r"`([a-z0-9_]+)`", spec)) if spec.startswith("`") else (spec, [])
    for kind in values:
        if kind not in specs:
            raise Error(f"sim-events.md: the kind `{kind}` has no order line")
    for kind in specs:
        if kind not in values:
            raise Error(f"sim-events.md: the kind `{kind}` has an order line and no value")
    for kind, (form, named) in specs.items():
        for other in named:
            if other not in values:
                raise Error(f"sim-events.md: the order line of `{kind}` names `{other}`, which is not a kind of the page")
    return Classes(values, specs)


@lru_cache(maxsize=None)
def load_classes(page: Path = PAGE) -> Classes:
    return parse_classes(page.read_text())


def order_pairs() -> list[tuple[int, int]]:
    """The pairs (A, B) of kind values with A <= B whose order counts, ascending, read from docs/sim-events.md."""
    return load_classes().pairs


class Record:
    """One record of a log: its line number, tick, kind value (None for a `dropped` line), the log line and its bytes without the phase."""

    def __init__(self, number: int, tick: str, phase: str, kind: int | None, line: str, raw: bytes) -> None:
        self.number = number
        self.tick = tick
        self.phase = phase
        self.kind = kind
        self.line = line
        self.raw = raw


def parse_log(log: str) -> tuple[str, list[Record]]:
    """The first line and the records of a full event log; an Error for a line that has no tick, phase and known kind."""
    classes = load_classes()
    lines = log.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    if not lines or not lines[0].startswith("bt-events "):
        raise Error("not a sim event log: the first line is not `bt-events <version>`")
    records: list[Record] = []
    for number, line in enumerate(lines[1:], 2):
        words = line.split(" ", 2)
        if len(words) < 3 or not words[0].isdigit():
            raise Error(f"line {number} has no tick and phase: `{line}`")
        kind_word = words[2].split(" ", 1)[0]
        if kind_word == DROPPED:
            kind = None
        elif kind_word in classes.values:
            kind = classes.values[kind_word]
        else:
            raise Error(f"line {number} has a kind that sim-events.md lacks: `{line}`")
        raw = (words[0] + " " + words[2] + "\n").encode("latin-1")
        records.append(Record(number, words[0], words[1], kind, line, raw))
    return lines[0], records


def projection(records: list[Record], a: int, b: int) -> bytes:
    """The bytes of the records of kind `a` or `b`, in the order given."""
    return b"".join(r.raw for r in records if r.kind == a or r.kind == b)


def projection_hashes(records: list[Record]) -> list[tuple[int, int, int]]:
    """(A, B, HB of the projection) for each non-empty projection of the records onto a pair that counts, ascending in (A, B)."""
    present = {r.kind for r in records if r.kind is not None}
    classes = load_classes()
    pairs = sorted({(min(a, b), max(a, b)) for a in present for b in classes.partners.get(a, ())})
    return [(a, b, hash_bytes(projection(records, a, b))) for a, b in pairs]


def tick_line(tick: str, records: list[Record]) -> str:
    """The `<tick> tick <count> <ordered> <set>` line of one tick's records (in log order)."""
    packed = b"".join(struct.pack("<HHQ", a, b, h) for a, b, h in projection_hashes(records))
    ordered = hash_bytes(packed)
    set_hash = hash_bytes(b"".join(sorted(r.raw for r in records)))
    return f"{tick} {TICK_PHASE} {len(records)} {ordered:016x} {set_hash:016x}"


def digest_text(log: str) -> str:
    """The digest of a full event log: two header lines, `<tick> <phase> <count> <hash>` for each run of records of one tick and phase, and `<tick> tick <count> <ordered> <set>` after the last run of each tick."""
    first, records = parse_log(log)
    out = [DIGEST_HEADER, LOG_HEADER_PREFIX + first]
    finished: set[str] = set()
    run_key: tuple[str, str] | None = None
    run: list[Record] = []
    tick_records: list[Record] = []

    def flush_run() -> None:
        if run:
            out.append(f"{run[0].tick} {run[0].phase} {len(run)} {hash_bytes(''.join(r.line + chr(10) for r in run).encode('latin-1')):016x}")

    def flush_tick() -> None:
        if tick_records:
            out.append(tick_line(tick_records[0].tick, tick_records))
            finished.add(tick_records[0].tick)

    for record in records:
        if tick_records and record.tick != tick_records[0].tick:
            flush_run()
            run_key, run = None, []
            flush_tick()
            tick_records = []
        if not tick_records and record.tick in finished:
            raise Error(f"line {record.number}: the records of tick {record.tick} are not contiguous in the log: `{record.line}`")
        if (record.tick, record.phase) != run_key:
            flush_run()
            run_key, run = (record.tick, record.phase), []
        run.append(record)
        tick_records.append(record)
    flush_run()
    flush_tick()
    return "\n".join(out) + "\n"


def tick_lines(digest: str) -> list[str]:
    """The `<tick> tick ...` lines of a digest, in order."""
    return [line for line in digest.splitlines() if line.split(" ", 2)[1:2] == [TICK_PHASE]]


def where(line: str) -> str:
    words = line.split(" ")
    if len(words) >= 4 and words[0].isdigit():
        return f" (tick {words[0]})" if words[1] == TICK_PHASE else f" (tick {words[0]}, phase {words[1]})"
    return ""


def digest_difference(stored: str, got: str, end_of_tick: bool = False) -> str | None:
    """Where two digests first differ (a line number, its tick and phase, both lines with their counts and hashes), or None when they are equal.

    With `end_of_tick` only the two header lines and the `tick` lines are compared."""
    def kept(text: str) -> list[tuple[int, str]]:
        lines = text.splitlines()
        return [(n, line) for n, line in enumerate(lines, 1) if not end_of_tick or n <= 2 or line.split(" ", 2)[1:2] == [TICK_PHASE]]

    want, have = kept(stored), kept(got)
    for (number, a), (_, b) in zip(want, have):
        if a != b:
            return f"events digest line {number}{where(a) or where(b)}: stored `{a}`, got `{b}`"
    if len(want) != len(have):
        longer, which = (want, "stored") if len(want) > len(have) else (have, "got")
        number, line = longer[min(len(want), len(have))]
        return f"events digest line {number}{where(line)}: only the {which} digest has it (`{line}`)"
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log", help="a full event log (events.txt)")
    ap.add_argument("--against", metavar="DIGEST", help="compare with this stored digest instead of printing")
    ap.add_argument("--end-of-tick", action="store_true", help="with --against: compare the header and `tick` lines only")
    args = ap.parse_args()
    if args.end_of_tick and args.against is None:
        ap.error("--end-of-tick needs --against")
    try:
        text = digest_text(Path(args.log).read_text(encoding="latin-1"))
    except (Error, OSError) as e:
        print(f"eventdigest: {e}", file=sys.stderr)
        return 2
    if args.against is None:
        sys.stdout.write(text)
        return 0
    diff = digest_difference(Path(args.against).read_text(), text, args.end_of_tick)
    print(diff or f"digests equal ({len(text.splitlines()) - 2} lines)")
    return 1 if diff else 0


if __name__ == "__main__":
    sys.exit(main())
