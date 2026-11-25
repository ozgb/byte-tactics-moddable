# The display log

A test-mode run of the game records, for every frame it draws, the list of what the
presentation asked to be drawn: the display list of the frame. Two builds are compared on
what they ask to draw, not on the pixels, so a presentation that draws through another
rasteriser, at another size or in a browser can still be equal. The list is the output a
re-implementation of the presentation is judged on, and its kinds of item are the display
interface of that re-implementation. The pixels of the same frames are in the frame dump
(`docs/frame-dump.md`); a rasteriser that turns a display list into those pixels is judged
on them.

Nothing in the log depends on the host, the platform, the display device or the memory
layout of a build: pictures are named by their file, entry and frame, models by their file
and pieces, and drawing surfaces by name.

## Frames

The game draws one frame after it has run the game ticks of one pass of its main loop. The
battle view, the menus and the loading screens all draw through the same items, and every
frame the run draws is in the log, whatever screen it shows. The scenarios of the suite
show the battle view after a few frames of the loading screen.

A frame is identified by its number, counted from 1 for the first frame the run draws, and
by its tick, the value of the tick counter when the frame is drawn (0 before the first
tick). The frame's **steps** are the number of game ticks run since the frame before it
(since the start of the run for the first frame): this is the frame time, an input of the
presentation. In test mode every frame of the battle view follows the ticks of one pass,
`steps_per_frame` of them (`docs/scenario-format.md`).

What the presentation draws is a function of the state at the end of the frame's last tick,
the sim events of the ticks since the last frame, the raw input and the frame time. The
particles, the smoke, the camera shake and follow, and the animation of pictures that loop
(GAF sequences) are stepped once for each tick, not for each frame: a frame after `n` steps
shows them `n` steps on. A frame drawn between two ticks, with 0 steps, which a test-mode
run of the battle view never draws, holds the same items as the frame before it except for
what depends on the input and the frame alone: the cursor, a scroll by the mouse or the
keys, and the messages that expire by frame.

## Files

A test-mode run writes `display.txt` beside `trace.txt`: the digest of every frame (below).
It also writes `displaylist.txt`, the full display list of chosen frames: those whose tick
is in the scenario's `frames` line (the frames whose pixels the frame dump writes), and with
the switch `-testdisplay <tick>` the frame of that tick too, or with `-testdisplay all`
every frame. The ticks follow the rule of the frame dump: a `frames` tick or a
`-testdisplay` tick that the run never draws as the last tick of a pass is an error, and the
run exits with status 2 naming the tick. Both files have LF line endings, no blank lines,
and end with a newline.

## The display list

`displaylist.txt` starts with the line `bt-display 1`, the format version. Each frame it
holds is a frame line, one line for each item in the order the presentation drew them, and
a cursor line:

    frame <number> <tick> <steps> <palette>
    <kind> <target> <x0> <y0> <x1> <y1> <fields>
    ...
    cursor <picture> <x> <y>

Fields are separated by one space. Numbers are signed decimal integers. `palette` is the
hash of the frame's palette, the 1024 bytes of the frame dump's palette (four bytes for each
of the 256 entries: red, green, blue, then 0), as 16 lowercase hexadecimal digits (the hash
is the one of `docs/trace-format.md`, section 3, applied once to the bytes, written
`HB(bytes)`).

The cursor line closes the frame. It names the picture the display shows as the mouse
cursor and the position of the mouse, which is where the picture's origin is put (the
picture's own offset is applied as for a `sprite`). The frame dump leaves the cursor out;
its picture is a decision of the presentation (the order the cursor would give), so it is
judged here. The game steps an animated cursor by the wall clock, not by ticks or frames, so
the line names the cursor's GAF entry, `<file>:<entry>`, without the frame. When the display
shows no cursor the line is `cursor - - -`.

### Names

A **name** is written with every byte below 0x21 or above 0x7e, and every `%` and `,`,
replaced by `%` and two uppercase hexadecimal digits (a space is `%20`). An empty name is
`-`, and a name that is the one character `-` is written `%2D`.

- A **picture** loaded from a GAF file is `<file>:<entry>:<frame>`: the file's path in the
  game's archives in lower case with `/` between directories, as the game loaded it
  (`anims/logos.gaf`), the entry's name as the file spells it, and the frame's index in the
  entry from 0. A picture with layers is named as a whole.
- A picture the game made while running, and not loaded from a file, is
  `@<name>:<width>x<height>:<hash>`: the name the game gave it when it made it, its size,
  and `HB` of its pixels, row by row without padding (`-` for a picture stored compressed).
  A picture whose pixels are a drawing surface (`target:` below) is `target:<target>`.
- A picture known by neither is `?<width>x<height>:<hash>`.
- A **target**, the surface an item draws on, is the name the game gave the surface when it
  made it, as a name. The surface of the frame, the one the frame dump writes and the frame's
  items mostly draw on, is `screen`. A surface the game made without a name is
  `?<width>x<height>`.
- A **font** is the name the game loads it by (`comix`), in lower case, or the side's font
  as its side's data names it.
- A **model** is its 3DO file's name without directory or extension, in lower case
  (`armcom`), and a piece of it is `<model>:<piece>`.
- A **type** is the name of a unit type (`armcom`) or a feature type, in lower case.
- A **text** is the bytes of the string as a name.

### The area

`x0 y0 x1 y1` is the item's **area**: the smallest rectangle of the target that holds every
pixel the item can change, with `x0` and `y0` included and `x1` and `y1` not, in the target's
pixels from its top left corner, cut to the target's clip rectangle at the time of the
item. An item that can change no pixel has the area `0 0 0 0`. The area is what orders the
items (below), and it is also the clip of the item: a rasteriser that draws the item clipped
to its area draws what the game drew.

### Kinds

The positions of an item are in the target's pixels. A colour is a palette index, 0 to 255.

| Kind | Fields | The item |
| --- | --- | --- |
| `sprite` | `<picture> <x> <y>` | The picture with its transparent index left out, its origin at `x y` (the picture's top left corner is at `x` minus its x offset, `y` minus its y offset). A layer of the picture that blends is drawn as a `sprite_blend`. |
| `sprite_opaque` | `<picture> <x> <y>` | The same with every pixel copied, the transparent index included. |
| `sprite_blend` | `<picture> <x> <y>` | The picture mixed with what is under it through the palette's blend table. |
| `sprite_gray` | `<picture> <x> <y>` | What is under the picture's opaque pixels turned gray through the palette's gray table. |
| `sprite_shadow` | `<picture> <x> <y>` | What is under the picture darkened, by the row of the palette's light table that each of the picture's pixels selects. |
| `sprite_lit` | `<picture> <x> <y> <row>` | The picture through row `row` of the palette's light table. |
| `sprite_erase` | `<picture> <x> <y> <parity>` | Colour 0 written on every other pixel of the picture's opaque pixels, the ones whose `x + y` has the parity `parity`. |
| `lens` | `<picture> <x> <y>` | What is under the picture, moved by the displacement map the picture holds (an explosion's heat haze). |
| `quad` | `<picture> <a> <b> <c> <d> <source>` | The picture mapped onto the quadrilateral of the four corners `a b c d`, each `x,y`. `source` is the quadrilateral of the picture that is mapped, four `x,y` corners joined by commas, or `-` for the whole picture. |
| `blit` | `<source> <x> <y>` | All of the target `source` copied with its top left corner at `x y`. |
| `copy` | `<source> <sx0> <sy0> <sx1> <sy1> <x> <y>` | The rectangle `sx0 sy0` to `sx1 sy1` (both included) of the target `source` copied to `x y`. |
| `clear` | `<colour>` | Every pixel of the target set to the colour. |
| `fill` | `<colour>` | Every pixel of the area set to the colour. |
| `fade` | `<level>` | Every pixel of the area through the palette's shade table (`level` below 0, darker) or its light table (`level` 0 or more). |
| `gray` | | Every pixel of the area through the palette's gray table. |
| `dither` | `<phase>` | Colour 0 on every other pixel of the area, those whose `x + y` has the parity `phase`. |
| `outline` | `<rx0> <ry0> <rx1> <ry1> <colour>` | The edges of the rectangle `rx0 ry0` to `rx1 ry1` (both included) in the colour. |
| `outline_lit` | `<rx0> <ry0> <rx1> <ry1> <row>` | The same edges through row `row` of the light table. |
| `line` | `<ax> <ay> <bx> <by> <colour>` | A line from `a` to `b`, both ends included. |
| `pixel` | `<x> <y> <colour>` | One pixel. |
| `circle` | `<x> <y> <radius> <colour>` | A circle about `x y`. |
| `circle_dashed` | `<x> <y> <radius> <colour> <dashes> <start>` | A circle of `dashes` dashes, the first at step `start`. |
| `polygon` | `<colour> <p1> ... <pn>` | A filled polygon of the corners `p1` to `pn`, each `x,y`. |
| `text` | `<font> <fore> <back> <key> <x> <y> <text>` | A string in the font with its top left corner at `x y`, glyph pixels in `fore` and their outline in `back`, and pixels of the colour `key` left out. A string the game cut to a width is the cut string. The area is the string's box; a string whose box is not inside the clip rectangle is not drawn, and has the area `0 0 0 0`. |
| `terrain` | `<map> <sx> <sy>` | The map's tiles, with the map position `sx sy` (the camera's scroll, in pixels of the map) at the top left corner of the area. `map` is the map's name, as the scenario's `map` key gives it. |
| `model` | `<type> <model> <x> <y> <wx> <wy> <wz> <ax> <ay> <az> <colour> <build> <cloak> <light> <pieces>` | A unit or a feature drawn from its 3D model, below. |
| `cargo` | the fields of `model` | A unit a transport carries, drawn with the transport, right after it. |
| `projectile` | `<piece> <ox> <oy> <oz> <ax> <ay> <az>` | A projectile drawn from a piece of a 3D model (for a whole model, its root piece). |
| `debris` | `<piece> <colour> <ox> <oy> <oz> <ax> <ay> <az>` | A piece of a unit's model thrown whole when the unit explodes, in the colour of the unit's player. |
| `shard` | `<face> <ox> <oy> <oz> <ax> <ay> <az> <v1> ... <v8>` | A shard of a unit's model thrown when the unit explodes: a box made from one face of a piece, its eight corners `v1` to `v8` each `x,y,z` (fixed point, as `wx`) about its centre. `face` is the picture its faces are drawn with, or `#<colour>` for a face of one colour. |

The fields of a `model`:

- `type`, `model`: the unit or feature type and its model.
- `x y`: where the model's origin is on the target.
- `wx wy wz`: its position in the world, the game's fixed-point values with 16 bits of
  fraction (`wy` is the height), and `ax ay az` its turn about the three axes, in the game's
  units of 65536 to the full turn.
- `colour`: the colour index of the player that owns it, which selects the player's
  colour in the model's textures.
- `build`: how much of it is still to build, the 8 hexadecimal digits of the IEEE 754
  single the game holds (`00000000` for a finished unit).
- `cloak`: 1 when it is drawn see-through (a cloaked unit), else 0.
- `light`: the direction of the light, three IEEE 754 singles as 8 hexadecimal digits each,
  joined by commas.
- `pieces`: one field for each piece, in the model's order (a piece, then its children,
  then its next sibling): `<name>,<flags>,<rx>,<ry>,<rz>,<ox>,<oy>,<oz>`: the piece's name,
  its state (1 shown, 2 drawn in the player's colour), its turn about the three axes and its
  offset from its parent (fixed point, as `wx`).

The fields of a `projectile`, a `debris` and a `shard`: `ox oy oz` is the position of the
piece relative to the top left corner of the camera's view of the world, fixed point as `wx`,
and `ax ay az` its turn. A projectile whose turn the game leaves unset (it draws it with
whatever the memory holds) has `- - -` for its turn.

The area of an item is the clip rectangle for `model`, `cargo`, `projectile`, `debris` and `shard`
(their pixels are the rasteriser's to find), the view of the map for `terrain`, and the
target's whole surface for `clear`. For the others it follows from the fields: the picture's
rectangle for the sprites and the `lens`, the bounding box of the corners for `quad` and
`polygon`, of the ends for `line`, of the circle for the circles, the rectangle for the
rectangle kinds, the copied rectangle for `blit` and `copy`, and the string's box for
`text`, all cut to the clip rectangle.

## Which orders count

The list is a list, not a call sequence: two items can be drawn in either order when neither
can change what the other draws. Two items **depend** on each other when they are on the same
target and their areas overlap, or when one copies from a target (`blit`, `copy`, and an
item whose picture is `target:<name>`, which read the whole of that target) and the other
draws on that target with an area that is not empty. Two items that depend on each other
keep their order; any two that don't may be swapped.

The **layer** of an item is 0 when it depends on no item before it in the frame, and
otherwise one more than the highest layer of the items before it that it depends on. Two
lists of a frame are the same up to swapping neighbours that do not depend on each other if
and only if they hold the same items with the same layers, counted with how often each
occurs. Sorting the items by layer is such a list, so the layers and the items say the whole
of what counts.

## The digest

`display.txt` starts with `bt-display-digest 1`, then `log bt-display 1`, then one line for
each frame the run draws:

    <number> <tick> <count> <ordered> <set>

| Field | Meaning |
| --- | --- |
| `number`, `tick` | The frame's number and tick. |
| `count` | The number of items in the frame. |
| `ordered` | `HB` of the frame's lines in the order of the display list: the frame line, the items, the cursor line, each with its line feed. |
| `set` | `HB` of the frame line, then the lines `<layer> <item>` of every item (the item's layer in decimal, a space, the item's line), sorted bytewise (the shorter first when one is the start of the other), then the cursor line, each with its line feed. |

`set` does not change when items that do not depend on each other are swapped, and `ordered`
does. `tools/displaylog.py digest` writes the digest of a display list, which for the frames
it holds is the same as the run's lines.

## What the check compares

For each scenario the check compares the run's `display.txt` with the stored digest,
`reference/display/<scenario>.txt`, line by line, the two header lines included, and
reports the first line that differs with its frame number and tick: a different `count`
shows a missing or an extra item. With `--end-of-tick` it compares the frame numbers,
ticks, counts and `set` only, which is the verdict for a presentation that keeps the orders
that count and no others. A scenario with no stored digest is counted and not compared.

For a scenario with a `frames` line, the stored `reference/displaylists/<scenario>.txt`
holds the display lists of those frames, and the check compares the run's `displaylist.txt`
with it frame by frame: it names the first frame that differs and in it the first item that
differs, by line number, with both lines (or the line only one of them has). With
`--end-of-tick` the items are compared as `<layer> <item>` lines in sorted order, so the
first item that differs is the first in that order. `tools/displaylog.py compare A B` does
the same for any two display lists, with `--end-of-tick` for the sorted comparison.

When a digest differs in a scenario with no stored list for that frame, the check plays the
reference build of the stored traces and the build under test with `-testdisplay <tick>` at
the first frame that differs, and compares their lists in the same way.

## What is not an item

Everything the game draws on a target while it draws a frame is an item, except these, which
the log leaves out:

- **The minimap's base picture.** The game writes the pixels of the minimap's map picture
  (the target `RADAR MAPPED`) one by one from the map and from what the player has seen,
  not through any drawing item. The items that draw the minimap read it whole (a `blit` from
  `RADAR MAPPED`), so a frame that differs there shows in the frame dump, not in the log.
- **The map's own minimap picture.** The picture a map file holds of itself is drawn as a
  `sprite` of a picture named by its hash (`?252x252:<hash>`), since it is in no GAF file.
- **Pictures the game makes.** The game makes some pictures by writing their pixels: a
  unit's picture and shadow from its model (the `model` item stands for them), the pictures
  of explosions and of the heat haze, the fog of war's tiles and the minimap's small
  picture. They are not items; an item that draws one names it by its name, its size and the
  hash of its pixels.
- **Movies.** A movie is decoded straight into the screen. No scenario of the suite plays one.
- **The frame rate.** The frame rate the game shows when asked is drawn by the host on the
  displayed picture, after the frame.
- **The cursor and the picture shown.** The cursor is drawn on the displayed picture after
  the frame and is the frame's cursor line; the copy of the frame to the display, and what
  the game saves and restores under the cursor, are not items.
- **Screenshots.** A screenshot draws into a picture of its own, through the same items with
  that picture as their target, and writes it to a file. No scenario takes one.
- **Draws on the game's other threads.** While a game loads, the game makes pictures on a
  thread of its own (the minimap's small picture among them), and the cursor's thread draws
  the cursor. Only what the game draws on its main thread, the one that draws the frames, is
  an item.
- **The options screen's backdrop.** When the options screen opens, the game copies the
  frame under it into a surface of its own with no drawing item.
