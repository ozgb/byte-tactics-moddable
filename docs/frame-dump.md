# The frame dump

A test-mode run of the game can write the pictures it draws, so that two builds can be
compared on what the player would see without a display. The scenario's `frames` line
lists the game ticks to dump:

    frames 30 60 90

The ticks are decimal numbers from 1, in ascending order, at most 64, each before the
last tick of the run. A scenario has one `frames` line. A scenario without one dumps
nothing.

## When a frame is taken

The game draws one frame after it has run the game ticks of one pass of its main loop.
The frame dumped for tick T is the one drawn after the pass whose last tick was T, while
the battle view is on screen. It is taken once the game has composed the whole picture
and before it draws the mouse cursor into it. The cursor is left out because its
position comes from input, not from the game's state.

A tick that the run never draws as the last tick of a pass is an error and the run
exits with status 2, naming the tick. This happens when the scenario runs several ticks
between two frames and T falls inside a pass, or when the run ends before T.

## Files

Each frame is written to `frames/<tick>.bin` beside `trace.txt`, creating the directory.
All numbers are little-endian.

| Offset | Size | Content |
| --- | --- | --- |
| 0 | 4 | The bytes `BTFR` |
| 4 | 2 | Version, 1 |
| 6 | 2 | Width in pixels |
| 8 | 2 | Height in pixels |
| 10 | 2 | Reserved, 0 |
| 12 | 4 | The tick |
| 16 | 1024 | The palette: 256 entries of 4 bytes, red, green, blue, then 0 |
| 1040 | width x height | The pixels, one byte each, an index into the palette, top row first, no padding between rows |

## What is not in it

- The mouse cursor.
- Anything the display does after the game hands it the picture: scaling, colour
  correction or a window frame. The palette is the one the game set, before any such
  change.

The picture is not altered otherwise. Text and overlays the game draws from its own
state, such as the game clock, are part of it.

## What a comparison needs

Two runs of one build on one scenario write identical files. A comparison reads the
palette and pixels of each file and compares them byte by byte.
