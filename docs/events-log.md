# The event log

A test-mode run of the game writes `events.txt` beside `trace.txt`, `sound.txt` and
`ui.txt`. It lists every sim event the presentation takes off the queue
(`docs/sim-events.md`), in the order it takes them, with the tick of the record and the
phase in which the logic recorded it. It records what the game logic decided to tell the
presentation, so that two builds can be compared on the events they emit without
drawing or playing anything: nothing in it depends on the host or the platform.

## Format

The first line is `bt-events 1`, the format version. Every other line is one event:

    <tick> <phase> <kind> <flags> <arg1> ... <argN>

Fields are separated by one space. There are no blank lines, and the file ends with a
newline. An event of two records (`docs/sim-events.md`, "The record") is one line.

| Field | Meaning |
| --- | --- |
| `tick` | The tick of the event's record (decimal): the value of the tick counter when the logic recorded it. |
| `phase` | The phase running when the logic recorded the event, one of the labels below. |
| `kind` | The name of the kind without the `SIM_EVENT_` prefix, in lower case (`unit_speech`, `screen_shake`, `radar_units`). |
| `flags` | The flags of the first record in hexadecimal with a `0x` prefix, with bit 15 (the continuation mark) cleared. |
| `arg1 ... argN` | The arguments of the kind in order, decimal and unsigned, as stored (a signed value appears in two's complement, as a 32-bit unsigned number). Exactly the number of arguments the kind takes (`docs/sim-events.md`); unused trailing arguments are not written. |

The two radar kinds carry no arguments: in place of the arena offset the line has one
token, the content of the snapshot as `docs/sim-events.md` defines its bytes (the
8-byte header, then the unit records and the projectile records for `radar_units`; the
12-byte header, then one byte for each cell for `radar_mapped`), in lower-case
hexadecimal.

When the queue's count of dropped events has risen since the last drain, the drain
writes a line before its events:

    <tick> <phase> dropped <count>

where `count` is the queue's total. A reference run never drops an event, so the stored
logs have no such line and any one is a difference.

## Phases

| Label | The phase |
| --- | --- |
| `net`, `units`, `projectiles`, `explosions`, `players`, `features`, `wind`, `meteors` | The phase of the step of that name (`docs/sim-events.md`, "Conventions"). |
| `sharing` | The end of a step, after the phases: the sharing of resources and the later parts of the step. |
| `load` | Outside a step, before the first step of the match: what the match load recorded. |
| `outside` | Outside a step, once the match has begun: console commands, interface actions and the shutdown of a match. |

The label of a `dropped` line is `load` or `outside` by the same rule, as it is written by the drain that sees the count rise.

## The digest

`reference/events/<scenario>.txt` stores the digest of the log, not the log (the logs of the
scenarios are over a gigabyte). `tools/eventdigest.py` makes the digest from a full log. The
first line is `bt-event-digest 2`, the digest format version. The second line is `log `
followed by the first line of the log (`log bt-events 1`). The other lines are of two kinds: the
lines of the runs, which are the digest of format 1 and are kept as they were, and one line for
each tick.

### The lines of the runs

Each covers one run of records with the same tick and the same phase, taken in the order of the log:

    <tick> <phase> <count> <hash>

| Field | Meaning |
| --- | --- |
| `tick`, `phase` | The tick and phase of the records, as they appear in the log. A tick and phase that the log has again after other records have their own line, in the order of the log. |
| `count` | The number of records in the run (decimal). |
| `hash` | The 64-bit hash of the run's bytes, as 16 lowercase hexadecimal digits. |

The hash is the one of the traces (`docs/trace-format.md`, section 3: the 64-bit FNV-1a variant
with the offset basis `0xcbf29ce484222325`, whose step on each whole 8-byte word is the word
step), applied once to the run's bytes as one run of bytes, written `HB(bytes)`. The bytes are
the text of the run's records exactly as the log has them: for each record, the whole line
from the tick to the last token, without the line feed, then one line feed (byte `0x0a`).
Nothing is added between records and the tick and phase are part of each record's bytes.

What these lines cover, therefore:

- Every field of every record: the tick, the phase, the kind, the flags and every argument.
- The content of a radar snapshot: the hexadecimal token of a `radar_units` or `radar_mapped`
  record is part of the record's line, so every byte of the snapshot is in the hash.
- A `dropped` line: it is a record of its tick and phase like any other, so it is counted in
  `count` and its text is in the hash. A reference run never drops an event, so a run that has
  one differs from the stored digest in the count and in the hash of that line's phase.
- The order of the records in a run, and the order of the runs.
- The header line, through the second line of the digest.

Two logs have the same lines of the runs only if they are equal, up to a collision of the 64-bit
hash of one run.

### The line of a tick

After the last line of the runs of a tick comes one line for that tick, which judges the tick's
events without their phases and with the order only where it counts:

    <tick> tick <count> <ordered> <set>

A tick that has no records has no line. The word `tick` stands where the phase is on the other
lines (no phase is called `tick`).

| Field | Meaning |
| --- | --- |
| `tick` | The tick, as the records carry it (decimal). |
| `count` | The number of the tick's records, `dropped` lines included (decimal). |
| `ordered` | The hash of the order in which the kinds whose order counts occur, below, as 16 lowercase hexadecimal digits. |
| `set` | The hash of the tick's records as a multiset, below, as 16 lowercase hexadecimal digits. |

The records of a tick are the records whose first word is the tick, whatever their phase: a
record of `load` or `outside` belongs to the tick it carries, like any other. All the records of
a tick are next to each other in the log (the digest tool refuses a log in which they are not,
and names the line). The bytes of a record, for this line, are its line without the phase word:
`<tick> <kind> <flags> <arg1> ... <argN>` (for a `dropped` line `<tick> dropped <count>`),
then one line feed (byte `0x0a`). Nothing else is added.

**`set`** is `HB` of the records' bytes sorted bytewise (as unsigned bytes, the shorter first
when one is the start of the other) and then concatenated. It covers every field of every
record except the phase, and the number of times each record occurs, and it does not depend on
the order.

**`ordered`** says in what order the records appear, only where `docs/sim-events.md` says the
order counts. Each kind has an `Order:` line on that page, and the lines give the pairs of
kinds whose order counts:

- A pair (A, B) of kind values with A at most B is a pair that counts when the line of A names B,
  or the line of B names A, or A is B and the line of A names A or is `same`. A line of `every`
  makes a pair of its kind with every kind of the page, itself included. A line of `none` makes
  no pair of its own. The kind values are the `Value N.` lines of the page; the kinds are named
  in the log as in the `Order:` lines (`docs/sim-events.md` gives each kind's name).
- For each pair that counts, the projection of the tick onto the pair is the bytes of the tick's
  records of kind A or kind B (a `dropped` line is of neither, so it is in no projection), in the
  order of the log, concatenated. A projection of a pair with no record of either kind is empty
  and is left out.
- `ordered` is `HB` of the sequence that has, for each non-empty projection in ascending order of
  (A, B), these 12 bytes: A as an unsigned 16-bit integer, B as an unsigned 16-bit integer, and
  `HB` of the projection as an unsigned 64-bit integer, all little-endian. A tick whose records
  are in no pair that counts has `ordered` equal to `HB` of no bytes, `cbf29ce484222325`.

The records, not the kinds, are the letters of this order: two records of one kind with
different bytes are two different letters that have the same dependencies, and the projection
onto (A, B) holds every pair of records of those kinds, so the rule covers them too. A projection
keeps the order of the records of one kind among themselves, so a kind that is in any pair that
counts is ordered against itself, and the page names it in its own `Order:` line. A kind that is in
no pair (the lines of `none`) has records that commute with every other record, and only `set`
judges them.

The rule is exactly the order the `Order:` lines allow. Two sequences of records are the same up
to swapping neighbouring records whose kinds do not order against each other if and only if they
hold the same records as a multiset and have equal projections onto every pair of kinds that
orders (the projection lemma for partially commutative words). So two logs of a tick have the
same `set` and `ordered` exactly when one of them can be made from the other by such swaps, up to
a collision of the 64-bit hash. The line has no more strictness than the classes: a phase, or the
place of a record among records of unrelated kinds, changes neither field.

What the lines of the ticks cover, therefore:

- Every field of every record except the phase: the tick, the kind, the flags, every argument and
  the whole radar snapshot, with the number of times each occurs.
- The order of the records of the kinds that depend on one another, as `docs/sim-events.md`
  states for each kind.
- A `dropped` line, in `count` and in `set`, and in no projection.

The lines of the runs stay as the gate inside the repository, where the phase and the whole order
are part of the contract. The lines of the ticks are the contract of a re-implementation that
keeps the end-of-tick events but not 3.1c's phases (`--check --end-of-tick`, below).

## What the check compares

For each scenario, the check makes the digest of the run's log and compares it with the stored
digest line by line, including the first two lines. Two digests are equal when they have the
same lines in the same order. The first line that differs is reported with its number, its tick
and phase, and both lines, which hold the counts and the hashes: a different count shows a
missing or an extra event in that phase, and a digest that is longer or shorter than the other
differs at the first line only one of them has.

With `--end-of-tick` (`tools/reference.py --check --end-of-tick`, the verdict for an engine that
keeps the end-of-tick contract with its own order inside the tick) the check compares the two
header lines and the `tick` lines only, and ignores the lines of the runs. The first `tick` line
that differs is reported the same way, with its number, its tick and both lines.

A scenario with no stored digest is not compared, and is counted in the summary. The check
runs in the tier as it does in the full suite.

## Naming the record

A digest names a tick and a phase, not a record. When a scenario's digest differs, the check
then plays the reference build of the stored traces and the build under test on that scenario
with `tools/tracediff.py A B --scenario S --events`, which compares the two full logs line by
line and prints the number of the first record that differs, its tick and phase, and both
records (or the record only one log has). `tools/tracediff.py --event-logs A B` does the same
for two logs that are already written. With `--end-of-tick` on either command the two logs are compared tick by
tick under the rule above: the report names the first tick whose `tick` line differs and, in it,
the first record (in the sorted order of the records' bytes) that the logs hold a different
number of times, or else the first pair of kinds (A, B) whose projection differs and the first
record of that projection that differs, with both records and their line numbers. A build that has the reference's logs can do the same
with its own tools: the full log of a scenario is what a test-mode run writes.
