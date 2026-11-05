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

## What the check compares

For each scenario, the check compares the log of a run with the stored log of the
same scenario, line by line and byte by byte, including the header line. Two logs
are equal when they have the same lines in the same order. The first line that
differs is reported with its number, its tick and phase, and both texts; a log that is
longer or shorter than the other differs at the first line only one of them has.

A scenario with no stored log is not compared, and is counted in the summary. The
check runs in the tier as it does in the full suite.
