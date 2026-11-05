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
first line is `bt-event-digest 1`, the digest format version. The second line is `log `
followed by the first line of the log (`log bt-events 1`). Every other line covers one run of
records with the same tick and the same phase, taken in the order of the log:

    <tick> <phase> <count> <hash>

| Field | Meaning |
| --- | --- |
| `tick`, `phase` | The tick and the phase of the records, as they appear in the log. A tick and phase that the log has again after other records have their own line, in the order of the log. |
| `count` | The number of records in the run (decimal). |
| `hash` | The 64-bit hash of the run's bytes, as 16 lowercase hexadecimal digits. |

The hash is the one of the traces (`docs/trace-format.md`, section 3: the 64-bit FNV-1a variant
with the offset basis `0xcbf29ce484222325`, whose step on each whole 8-byte word is the word
step), applied once to the run's bytes as one run of bytes, written `HB(bytes)`. The bytes are
the text of the run's records exactly as the log has them: for each record, the whole line
from the tick to the last token, without the line feed, then one line feed (byte `0x0a`).
Nothing is added between records and the tick and phase are part of each record's bytes.

What the digest covers, therefore:

- Every field of every record: the tick, the phase, the kind, the flags and every argument.
- The content of a radar snapshot: the hexadecimal token of a `radar_units` or `radar_mapped`
  record is part of the record's line, so every byte of the snapshot is in the hash.
- A `dropped` line: it is a record of its tick and phase like any other, so it is counted in
  `count` and its text is in the hash. A reference run never drops an event, so a run that has
  one differs from the stored digest in the count and in the hash of that line's phase.
- The order of the records in a run, and the order of the runs.
- The header line, through the second line of the digest.

Two logs have the same digest only if they are equal, up to a collision of the 64-bit hash of
one run.

## What the check compares

For each scenario, the check makes the digest of the run's log and compares it with the stored
digest line by line, including the first two lines. Two digests are equal when they have the
same lines in the same order. The first line that differs is reported with its number, its tick
and phase, and both lines, which hold the counts and the hashes: a different count shows a
missing or an extra event in that phase, and a digest that is longer or shorter than the other
differs at the first line only one of them has.

A scenario with no stored digest is not compared, and is counted in the summary. The check
runs in the tier as it does in the full suite.

## Naming the record

A digest names a tick and a phase, not a record. When a scenario's digest differs, the check
then plays the reference build of the stored traces and the build under test on that scenario
with `tools/tracediff.py A B --scenario S --events`, which compares the two full logs line by
line and prints the number of the first record that differs, its tick and phase, and both
records (or the record only one log has). `tools/tracediff.py --event-logs A B` does the same
for two logs that are already written. A build that has the reference's logs can do the same
with its own tools: the full log of a scenario is what a test-mode run writes.
