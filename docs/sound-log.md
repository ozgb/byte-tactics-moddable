# The sound log

A test-mode run of the game writes `sound.txt` beside `trace.txt`. It lists every
sound the game starts or stops, in the order it did so, so that two builds can be
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
| `name` | The sound's name: the name the game's sound table gives the sample, or, for a file played straight from disk, the file name without its directory (extension included). `-` when the game holds no name for the sound. |
| `volume` | The volume the game asked for, a signed decimal integer in hundredths of a decibel of attenuation (0 is full volume, -585 is the usual in-game level, -1585 is the level for a sound off the visible screen). `-` when the event carries none. |
| `x y z` | The position of the sound as the game computed it, three signed decimal integers in the game's own screen-relative units (x to the right, z up the screen, y always 0). `-` for all three when the sound has no position. |

The game computes no separate pan: a positional sound carries its position and the
audio layer derives the pan from it, so a position is the only stereo placement the
log can hold.

## Kinds

| Kind | Event |
| --- | --- |
| `play` | A one-shot sound starts. It carries `volume`, and a position when the game placed the sound in the world. |
| `loop` | A looping sound starts. Same fields as `play`. |
| `play_memory` | A sound built from bytes in memory starts. Logged after the `play` line it causes. No name, volume or position. |
| `play_file` | A sound built from a file stream starts. Logged after the `play` line it causes. No name, volume or position. |
| `wav` | A one-shot sound starts on the plain system sound path. Only `name`. |
| `wav_loop` | A looping sound starts on the plain system sound path. Only `name`. |
| `wav_file` | A sound file is loaded from disk and starts on the plain system sound path. `name` is the file name. |
| `stop_all` | The game stops every sound that is playing. |
| `stop_oldest` | The game stops the oldest sound to make room for a new one. |
| `stop_stream` | The game stops the music stream that was playing. |

A sound the game asks for but does not start (for example because sound is turned
off, or the position is out of sight) leaves no line.

## What the check compares

For each scenario, the check compares the log of a run with the stored log of the
same scenario, line by line and byte by byte, including the header line. Two logs
are equal when they have the same lines in the same order. The first line that
differs is reported with its number and both texts; a log that is longer or shorter
than the other differs at the first line only one of them has.

A scenario with no stored log is not compared, and is counted in the summary.
