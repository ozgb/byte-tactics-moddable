# The sound log

A test-mode run of the game writes `sound.txt` beside `trace.txt`. It lists every
sound the game asks to start or stop, in the order it asked, so that two builds can be
compared on the sounds they ask for without playing them. It records what the game
decided, not what a mixer made of it: nothing in it depends on the audio device,
the host or the platform.

## Format

The first line is `bt-sound 1`, the format version. Every other line is one event:

    <tick> <kind> <name> <volume> <x> <y> <z>

Fields are separated by one space. A field that does not apply to the event is `-`.
There are no blank lines, and the file ends with a newline.

| Field | Meaning |
| --- | --- |
| `tick` | The game tick (decimal, from 0) at which the event happened. |
| `kind` | What happened, one of the kinds below. |
| `name` | The sound's name: the logical name the game's sound table gives the sound or, when it has none, the table's file name for it (at most 32 characters, without its directory), or, for a file played straight from disk, the file name without its directory (extension included). `-` when the game holds no name for the sound. |
| `volume` | The volume the game asked for, a signed decimal integer in hundredths of a decibel of attenuation (0 is full volume, -585 is the usual in-game level, -1585 is the level for a sound off the visible screen). `-` when the event carries none. |
| `x y z` | The position of the sound as the game computed it, three signed decimal integers in the game's own screen-relative units (x to the right, z up the screen, y always 0). `-` for all three when the sound has no position. |

The game computes no separate pan: a positional sound carries its position and the
audio layer derives the pan from it, so a position is the only stereo placement the
log can hold.

## Kinds

| Kind | Event |
| --- | --- |
| `play` | The game asks for a one-shot sound from its sound table. It carries `volume`, and a position when the game placed the sound in the world. |
| `loop` | The game asks for a looping sound from its sound table. Same fields as `play`. |
| `play_file` | The game asks for a sound file to be played from disk (unit speech). `name` is the file name, `volume` the requested volume, no position. |
| `wav` | The game asks for a one-shot sound on the plain system sound path. Only `name`. |
| `wav_loop` | The game asks for a looping sound on the plain system sound path. Only `name`. |
| `wav_file` | The game asks for a sound file to be played from disk on the plain system sound path. `name` is the file name. |
| `stop_all` | The game stops every sound that is playing. |
| `stop_stream` | The game stops the music stream. |

The log records what the game asked for, never what the audio device did with it. A
line is written at the point where the game requests a sound, after the checks that
read game or settings state (the sound volume and flags, whether sound is switched
off, whether the position is visible to the local player) and before anything that
depends on the device or on which buffers are free. A request the device then fails
to carry out is still logged, and the game's choice of which buffer to free is not
logged at all.

## What the check compares

For each scenario, the check compares the log of a run with the stored log of the
same scenario, line by line and byte by byte, including the header line. Two logs
are equal when they have the same lines in the same order. The first line that
differs is reported with its number and both texts; a log that is longer or shorter
than the other differs at the first line only one of them has.

A scenario with no stored log is not compared, and is counted in the summary.
