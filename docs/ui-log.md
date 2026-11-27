# The UI log

A test-mode run of the game can be fed raw input (key presses, characters and mouse
events) by tick, and writes `ui.txt` beside `trace.txt`. The log lists the input it
delivered, the control groups the game's own interface made and recalled, and the selection and
camera after each tick: the interface's own state. What the interface sent to the simulation is
not in it: every command it sends is a record of the command log, `commands.txt`
(`docs/commands.md`, section 2), in the format of a scripted command. Two builds conform when the
same input gives the same log and the same commands, so a re-implementation of the interface is
judged on its behaviour alone: nothing in either depends on the window, the host or the
platform.

## Scripted input

A scenario file gives the input with one `input` line per event. Lines may repeat and
accumulate.

    input <tick> <event>
    input <tick>.<frame> <event>

| Event | Meaning |
| --- | --- |
| `key down <key>`, `key up <key>` | A key goes down or up. |
| `char <c>` | A typed character; `c` is one character or a code from 1 to 255 (decimal or `0x` hex). |
| `mouse move <x> <y>` | The mouse moves to a window position. |
| `mouse left down <x> <y>`, `mouse left up <x> <y>` | The left button goes down or up at a window position. |
| `mouse right down <x> <y>`, `mouse right up <x> <y>` | The right button goes down or up at a window position. |

A `<key>` is a virtual-key code (decimal or `0x` hex, 1 to 255), `shift`, `ctrl`,
`alt`, `esc`, `f1` to `f12`, a letter `a` to `z` (the key, not the character) or a
digit `0` to `9`. A window position is in the 640 by 480 window, `x` from 0 to 639 and
`y` from 0 to 479.

- **Delivery.** `<tick>` is a game tick from 1 to the scenario's last tick. The event
  is queued at the start of that tick, and the interface handles it after the tick's
  simulation has finished, before the next tick starts. The interface handles one mouse
  event and one key or character event per frame, so a scenario has at most one of each
  on a tick and runs one tick per frame.
- **While no tick runs.** The game runs no tick while it is paused (the Pause key) or while
  a dialog that stops the game is open (the options menu of F2, the exit menu), but it
  keeps drawing frames and handling input. `<tick>.<frame>` addresses those frames: the
  event is queued at the end of the `<frame>`-th frame (from 1) drawn after tick `<tick>`,
  and the interface handles it in the frame after. A line that names a frame after a tick
  that the game did not stop at (it ran tick + 1 before that frame) is an error, and so is
  a run that draws 300 frames after the last such line of its tick without running a tick:
  the game would wait for input forever. A frame line at the scenario's last tick is an
  error too, since the run ends when that tick has run, and so is a frame line that the run
  ends before (a game over or a left game first). The one-event rule holds per frame. The input that
  resumes the game is its last frame line; an event after it is a plain `input <tick + 1>`
  line. Pause pressed with `input 10` takes effect in the frame that runs tick 11, so the
  stopped frames are `11.1`, `11.2` and on; a key pressed at `11.5` is handled in frame 6,
  which runs no tick yet, and frame 7 runs tick 12.
- **Before tick 1.** A scenario with `frontend 1` starts at the main menu
  (`docs/scenario-format.md`, section 3.5), and `0.<frame>` addresses the frames the menus
  draw before tick 1, counted from the first frame of the run. The rules above hold for
  them, with the battle's start in place of the next tick: a run that draws 300 frames after
  its last `0.<frame>` line without starting tick 1 is an error, and so is a `0.<frame>`
  line that the run never delivers. A scenario without `frontend 1` cannot name tick 0. The
  log's lines for those frames carry tick 0.
- **State.** A key is held from its `key down` to its `key up`, a mouse button from its
  `down` to its `up`, and the cursor is where the last mouse event put it. While Alt is
  held, key events reach the game as system key events, as on Windows. The time of an
  event is its tick, never the clock.
- **Keys and characters.** The game reads its letter commands (`a` attack, `p` patrol,
  `m` move, `s` stop, `d` special attack) from `char` events, and Shift, Ctrl, Alt,
  Escape and the digits from `key` events. Ctrl plus a digit makes a control group,
  and Alt plus a digit recalls it. Ctrl plus `d` toggles self-destruct.
- **Interface type.** The game has two ways to give orders. With type 0 (the default) a
  left click in an order mode (entered with a command character or a panel button)
  gives the order, and a right click clears the selection; a right click on the
  minimap centres the camera. With type 1 a right click on the ground or a unit gives
  the order, and a left click on the minimap centres the camera. A scenario says which
  it uses by typing the console command `+iface 0` or `+iface 1` at tick 1.
- **Where things are on the screen.** The main view is the rectangle at (128, 32) of the
  window. For a unit at world position `x z` and height `y`, and a camera scroll of
  `sx sz` (the `scroll` line below), the screen position is
  `(x - sx + 128, z - sz - y / 2 + 32)`, and the ground under a screen position
  `(a, b)` is `(a - 128 + sx, b - 32 + sz)`. The minimap is at the top left, from (0, 0),
  128 pixels square. The panel for the selected unit is below it, from (0, 128): a
  gadget at `(gx, gy)` of size `(gw, gh)` in the panel's own layout is at
  `(gx, gy + 128)` on the screen. The build menu of a commander puts its first button
  (the first building) at gadget (0, 27, 64, 64) and the next at (64, 27, 64, 64).
- **A click or a box.** The time of a mouse event is its tick. A left press at tick `p` and its
  release at tick `r` are a click when `r - p` is under 25 (ticks of 30 a second) and the
  ground under the cursor in the frame before the release is less than 32 world units from the
  ground under the press on each axis, and a box otherwise. 3.1c judges the time by the clock
  instead, so there a hold of 2 to 24 ticks is a click or a box depending on how fast the frames
  ran. A scenario therefore releases a button on the tick after its press (a click), or moves
  the mouse at least 32 pixels from the press position while the button is down (a box), and
  the scenario loader rejects anything between.

## Format

The first line is `bt-ui 2`, the format version. Every other line is one record:

    <tick> <kind> <fields>

Fields are separated by one space. There are no blank lines, and the file ends with a
newline. Units are named by their index in the game's unit table, which is the same in
every conforming build for the same input (a unit list is the indices in ascending order
separated by commas without spaces, or `-` when empty). Positions are whole world units
(the game's fixed-point value without its fraction).

The tick of a record is the tick whose simulation had last finished when the interface
acted, so the effect of an event queued at tick `N` carries tick `N`, and its effect on
the simulation shows from tick `N + 1`. The records of one tick are in the order they
happened: the delivered input first, then what the end of the tick saw, then what the
interface did with the input.

| Kind | Fields | Meaning |
| --- | --- | --- |
| `input` | `key down <vk>`, `key up <vk>`, `char <code>`, `mouse move <x> <y>`, `mouse left down <x> <y>` and the like | An event of the scenario's `input` lines, in decimal, at the tick of its line (a frame line's at its `<tick>`). |
| `scroll` | `<x> <z>` | The camera scroll, at tick 1 and at the end of any tick where it differs from the last `scroll` line. Every scenario logs it (see below). |
| `view` | `<draws>` | The number of draws made so far from the view random stream, counted from the start of the run, at the end of any tick where it differs from the last `view` line. A draw made between two steps counts into the next tick. See below. |
| `select` | `<units>` | The local player's selected units, at tick 1 and at the end of any tick where the selection differs from the last `select` line. The selection is view state, which no trace row hashes: this record is what a presentation is judged on. |
| `group` | `<n> <units>` | Control group `n` was made: the units now in it. |
| `groupsel` | `<n> <units>` | Control group `n` was recalled: the units in it before the selection changed. |

Format 1 also had `command`, `order`, `qbuild` and `cancel` records, for what the interface
sent. Format 2 leaves them to `commands.txt`:

- **The commands.** Every event the interface publishes (the group command of an order click,
  the build site of a builder, the build menu's queue changes, the self-destruct toggle's
  cancels) is a record of `commands.txt` with the key `interface`, and what is judged is its
  decoded form (`docs/commands.md`, section 2).
- **The orders of each unit.** The orders module expands a group command into one COMMAND
  event for each unit that takes the order. Each is a record of the orders events section of a
  dump (`docs/trace-format.md`, section 8.32), and the orders it gives are part of every unit's
  state, which the trace rows hash.

Commands the scenario's own `order` and `console` lines give are records of `commands.txt`
too, under their own keys, and are not the interface's.

## What the check compares

For each scenario, the check compares the log of a run with the stored log of the same
scenario, line by line and byte by byte, including the header line. Two logs are equal
when they have the same lines in the same order. The first line that differs is reported
with its number and both texts; a log that is longer or shorter than the other differs at
the first line only one of them has.

A scenario with no stored log is not compared, and is counted in the summary, except a
scenario whose name starts with `ui-`: it must have one, and the run must show that the
input did something, with at least one `group` or `groupsel` record, a `select` record with
units, or a command of the interface in `commands.txt`. Input that sends no command and selects
nothing is a broken scenario, not a pass. A stored log with no scenario is a failure.

## The scroll snapshot

The `scroll` record is in every scenario, whether or not it has `input` lines. The camera shake and the particles draw from the
game's view random stream, which is presentation only (no trace field hashes it), and the shake offset is part of the scroll. A
build that falls one draw behind the reference at a tick boundary has a different shake offset from then on, so its `scroll` records
differ though its traces are equal.

## The view stream

The `view` record counts the draws from the view random stream, the only random stream that presentation draws
from. It is absolute: a single line reads on its own, and a build that skips or adds one draw changes the line of that tick and every
later line. The check names the first tick where the counts differ, so a lost draw is found at the tick it is lost, not at the shake
tick many ticks later. Draws are counted in the order of the game's own calls; the count says how many draws were made, and the
`scroll` records say whether they went to the same consumers.
In test mode the load completes before the first frame after it starts, so the number of frames drawn before tick 1, and with it the draws those frames make, is fixed and does not depend on how fast the machine loads.
