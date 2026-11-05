# The UI log

A test-mode run of the game can be fed raw input (key presses, characters and mouse
events) by tick, and writes `ui.txt` beside `trace.txt`. The log lists the input it
delivered, every order and queued build the game's own interface turned that input
into, the control groups it made and recalled, and the selection and camera after
each tick. Two builds conform when the same input gives the same log, so a
re-implementation of the interface is judged on its behaviour alone: nothing in the log depends on
the window, the host or the platform.

## Scripted input

A scenario file gives the input with one `input` line per event. Lines may repeat and
accumulate.

    input <tick> <event>

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
- **A click or a box.** The game takes a press and release within about a second and less than
  32 world units apart for a click, and anything else for a box, judging the time by the
  clock. A scenario releases a button on the tick after its press (a click), or moves the
  mouse at least 32 pixels from the press position while the button is down (a box); the
  scenario loader rejects anything between.

## Format

The first line is `bt-ui 1`, the format version. Every other line is one record:

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
| `input` | `key down <vk>`, `key up <vk>`, `char <code>`, `mouse move <x> <y>`, `mouse left down <x> <y>` and the like | An event of the scenario's `input` lines, in decimal, at the tick of its line. |
| `scroll` | `<x> <z>` | The camera scroll, at tick 1 and at the end of any tick where it differs from the last `scroll` line. Every scenario logs it (see below). |
| `view` | `<draws>` | The number of draws made so far from the view random stream, counted from the start of the run, at the end of any tick where it differs from the last `view` line. A draw made between two steps counts into the next tick. See below. |
| `select` | `<units>` | The local player's selected units, at tick 1 and at the end of any tick where the selection differs from the last `select` line. |
| `order` | `<KIND> unit <U> target <T or -> pos <x> <y> <z> or - type <NAME or -> param <N or -> amount <A> remove <0 or 1>` | One order given to one unit. See below. |
| `qbuild` | `<U> <TYPE> <count>` | A click in a factory's build menu changed the build queue of unit `U` by `count` (1 or 5, negative for a right click) of the unit type, or missile, named `TYPE` by the button. |
| `cancel` | `<KIND> unit <U>` | The interface cancelled a queued order of kind `KIND` of unit `U` (the self-destruct toggle). |
| `group` | `<n> <units>` | Control group `n` was made: the units now in it. |
| `groupsel` | `<n> <units>` | Control group `n` was recalled: the units in it before the selection changed. |

The `order` record has one line for each unit that gets the order, after the game has
spread a position over a group of selected units. `KIND` is the game's name of the
order. `target` is the unit under the cursor, or `-`. `pos` is the world position
`x y z` (`y` is the height) or `-` for an order with none. `type` is the unit type the
order builds, for the building orders, otherwise `-`; `param` is the order's number
(the standing-order state, 0 to 2, for the standing orders) and `-` for the building
orders. `amount` is the order's count. `remove` is 1 when the player held Shift: the
order joins the queue instead of replacing it, and repeating a queued order cancels it.

Orders the scenario's own `order` lines give are not in the log, being the
scenario's and not the interface's; an order a `console` line types (the chat line)
is.

## What the check compares

For each scenario, the check compares the log of a run with the stored log of the same
scenario, line by line and byte by byte, including the header line. Two logs are equal
when they have the same lines in the same order. The first line that differs is reported
with its number and both texts; a log that is longer or shorter than the other differs at
the first line only one of them has.

A scenario with no stored log is not compared, and is counted in the summary, except a
scenario whose name starts with `ui-`: it must have one, and its log must show that the
input did something, with at least one `order`, `qbuild`, `cancel`, `group` or
`groupsel` record or a `select` record with units. Input that gives no order and selects
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
