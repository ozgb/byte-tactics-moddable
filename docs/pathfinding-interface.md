# The pathfinding interface

Pathfinding sits behind a general interface, `interfaces/pathfinding.h` (version 2.0). 3.1c's search is
its default implementation (`src/ai/pathfind.cpp`). This page is the inventory that interface was cut
from (#883):
- what 3.1c's search takes, when it runs, and what it leaves;
- what its consumers read;
- what the trace hashes of it;
- which of 3.1c's behaviours stay inside the default.

Each item a consumer reads is marked **in the result** (it fits a general path record: a waypoint list
and a status) or **internal** (3.1c reads more than the result, so the default search and the default
movement have to stay together, or the interface had to widen).

## 1. Requests

3.1c has no request call. A unit wants a path when its goal object, an `AiSearchGoal`, has `flag_1`
set. The search finds those units by walking them (section 2).

| Who sets `flag_1` | Phase | Goal |
|---|---|---|
| `AiSearchGoal::SetPathOrder`, when the order gets a new target and the stored waypoints don't already lead into it | wherever an order gives the unit a new target | the order effect (`OrderFx`) of the order |
| `AiSearchGoal::TickTowardGoal`, every tick the unit has a target and is blocked (`UnitMotion::flags & 4`) or has fewer than 2 waypoints left | units | the same order effect |

`flag_1` is cleared by `SetWaypoints` (every end of a search) and by `SetPathOrder` when the target is
cleared or the old waypoints still lead into it.

A goal is one of the order effects' areas:
- `ApproachRadius`: a circle;
- `RingApproach`: a ring between two radii;
- `PointMarker`: a rectangle;
- any other order effect: no cell is a goal, and every distance is 0.

The search asks a goal three things: whether a cell is in it (`ContainsCell`), the estimated distance to it
(`ApproxDist`, the heuristic), and its goal cells (`FillGoalCells`, to mark them on the grid).

**Throttle.** A unit is taken only when 60 ticks have passed since its last claim (`repathClaimTick`, in
the goal object). `SetPathOrder` resets `repathClaimTick` to 0 when the last claim is 10 or more ticks old.

**Cancellation.** `SetPathOrder` and the goal's destructor call `AbortIfGoalMatch`, which drops the search
in progress if it is that goal's. There is no pending queue to cancel from, since a request is only a flag.

**In the interface:**
- `Submit` takes a request: a unit, a goal area (`PathGoalArea`), a `notBefore` tick, and two opaque
  handles (the goal object and the order effect).
- Orders submit wherever 3.1c sets `flag_1`, with `notBefore = repathClaimTick + 60`.
- Orders call `Cancel` wherever 3.1c called `AbortIfGoalMatch`, and when `SetPathOrder` clears `flag_1`.
- The goal area is copied from the order effect when the request is submitted. The three areas'
  `ContainsCell`, `ApproxDist` and goal-cell fill are `PathGoalContains`, `PathGoalDistance` and
  `PathGoalCells`. The order effects call the same functions, so there is one copy of the maths.
- The start (cell, heading, movement class) isn't in the request. A search starts from where the unit
  stands when the search begins, as in 3.1c.

## 2. The schedule

`RunSearches` runs once per tick, first in the players phase, before the players' own updates.

- **The budget.** 0x535 (1333) steps per tick, divided equally among the players in the match
  (`numPlayers`). Each player's share is added to that player's bank, `budget[p]`, for every playing
  player. The step loop runs while the sum of the banks is above 0.
- **One search at a time.** The pathfinder has one search slot (`object`).
- **When the slot is empty**, it serves the current player (`player`) while that player's bank is positive,
  and otherwise moves on to the next of the ten player slots with a positive bank:
  - It advances that player's cursor (`cursor[p]`) to the next unit slot, wrapping from the last slot to
    the first, and counts the visit in `g_playerTickLoad[p]`.
  - A live unit with a movement class whose goal claims a repath (section 1) starts a search, at a cost of
    100 steps.
  - A visit that starts nothing costs 1 step.
- **When the slot is busy**, it expands the open list's best node, up to 100 steps in one slice. The
  slice's steps are charged to the current player's bank, so one player's long search can drive that bank
  negative and starve the player's later turns (**budget starvation**).
- **A search runs over as many ticks as it needs.** It isn't abandoned for taking long. It ends:
  - when a goal cell is popped (found);
  - when the open list is empty (failed);
  - at once, when the start is in the goal or off the grid, or when the straight-line probe gets no closer
    to the goal than the start;
  - when it is aborted.
- **The heuristic weight.** Every 150 ticks (`g_budgetRefreshCounter`), each player's visits per unit slot
  over the window set that player's weight `g_playerBudgetCap[p]`:
  - under 1 visit per slot: 6 × `baseScale`;
  - under 2: 3 × `baseScale`;
  - otherwise: `baseScale` (0x18000, 1.5 in 16.16).

  The weight multiplies the distance estimate, so a player whose units seldom search gets a greedier
  search.
- **When the result is visible.** In 3.1c, at once: the waypoints are written into the goal object during
  the step. A spike that deferred every one of those writes to the end of the step kept every tier
  scenario equal (#883). In the interface, a step's results are events that the players phase drains with
  `Poll` right after `Step`. So they are visible in the same phase, before any other subsystem runs.

## 3. The result

| Outcome | What 3.1c leaves | Event |
|---|---|---|
| Search started, the straight-line probe reaches the goal (or the start is in it) | order flag 0x100 (`Order::subFlags`, through the order effect) | `STARTED`, `PATH_START_DIRECT` |
| Search started, the probe doesn't reach it (or the start is off the grid) | order flag 0x200 | `STARTED`, `PATH_START_INDIRECT` |
| Found | the corners of the route, from the start, as waypoints in world units (cell × 16 + footprint × 8), into the goal object; `flag_1` cleared, `active` set | `FINISHED`, `PATH_FOUND` |
| Found, more than 64 corners | only the 64 corners nearest the start (a 64-entry ring buffer) | `FINISHED`, `PATH_PARTIAL` |
| Failed, or ended at once | `SetWaypoints(0, 0)`: `active` cleared, and order flag 0x40 unless the unit is already in the goal | `FINISHED`, `PATH_FAILED`, no points |
| Claim | `repathClaimTick` = the tick | the `STARTED` event's tick |

The goal object keeps at most 20 waypoints (`SetWaypoints` cuts the rest), so a long route is always
partial in practice. The unit asks again when it runs out (section 1). The cut belongs to the consumer:
the event carries up to 64.

Partial results over several ticks don't happen in 3.1c (a search reports once, at its end). The
interface has `PATH_EVENT_PROGRESS` for implementations that do.

## 4. What the consumers read

The consumers are the order (`AiSearchGoal`, `OrderFx`, `Order`) and the unit's movement (`UnitMotion`).
Orders apply each polled event with `ApplyPathEvent` (`src/orders/order_targets.cpp`).

| What the consumer reads | Who reads it | Marked |
|---|---|---|
| The waypoints, their count and `active` in the goal object | movement (`SteerGroundUnit`, through `HasReadyWaypoints` and `FillWaypointWorldPos`) | in the result: `FINISHED` points |
| `flag_1` (a repath is wanted) | orders and the goal object | in the result: orders keep it, and it mirrors a pending request |
| `repathClaimTick` (the 60-tick throttle) | the goal object | in the result: written from the `STARTED` event's tick; the request's `notBefore` carries it back |
| Order flags 0x100 and 0x200 (`Order::subFlags`) | the order's own update | in the result: the `STARTED` event's `PathStart` |
| Order flag 0x40 (no route) | the order's own update | in the result: a `FINISHED` event with no points |
| The cut to 20 waypoints | the goal object (`SetWaypoints`) | in the result: the consumer cuts, the event carries up to 64 |
| The pass maps re-stamped for the searching unit's class | movement, for its own collision tests | **internal**: the default calls `BeginSearch` and `EndSearch`, which are 3.1c's lazy refresh (section 7) |
| `PROGRESS` events | nobody | orders ignore them; 3.1c never sends one |

Everything movement reads of a search is in the result. The one coupling left is the pass maps'
refresh, which a search triggers. It is in the world view, not in pathfinding's state, so another
implementation can call it or not. Not calling it changes when the pass maps are fresh, which movement
sees, so a replacement that skips it doesn't conform even with the same waypoints.

Inside the default, and not reached by any consumer:
- the budget (`stepsPerTick`, `baseScale`), set by the `SetSearchStepsPerTick` and `SetSearchBaseScale`
  calls of the seams table (the `search` console command);
- the debug overlay's per-cell flags (`PathfindingDebugCell`);
- the quirks of section 6.

## 5. The traced state

Sections 16 and 17 of `docs/trace-format.md`. The harness hashes them from the view `Project` fills, not
from the game record. The default's view points into its own region, so the bytes are 3.1c's.

| Item | Carries over between ticks | Kind |
|---|---|---|
| `Pathfinder` record: node pool and heap counters, grid size, `stepsPerTick`, `steps`, `costScale`, `baseScale`, `object`, `player`, `cursor[10]`, `budget[10]` | yes | schedule state |
| `PathfinderSearch` (only while `object` is set): `start`, `goalX/Y`, `probe`, `range`, `path`, `target`, `owner`, `turnCost`, `stepCost` | yes, while a search runs | search state |
| node pool, heap | yes, while a search runs; a finished search leaves them as they were | search state |
| grid cells and dirty words | yes: a search's marks stay until the next search clears them | search state |
| `g_playerBudgetCap`, `g_playerTickLoad`, `g_budgetRefreshCounter` | yes | schedule state |

Nothing in these sections is a result: the results are in section 21 (the goal objects) and section 20
(the orders' flags), which the consumers own.

## 6. Quirks that stay inside the default

- **Budget starvation:** a long search is charged to one player's bank, and that player's later turns
  wait (section 2).
- **Load-weighted heuristic:** the weight follows how often a player's units were visited, not how
  often they searched (section 2).
- **Fog counts as open:** a cell the searching player hasn't seen is treated as open at no penalty,
  whatever is there (`GetCellState`).
- **The 64-corner ring buffer:** a route with more corners keeps those nearest the start, and the far
  end is lost (`TracePath`).
- **Abort without release:** `AbortIfGoalMatch` drops the search without bringing the unit's own cells
  up to date (`EndSearch`), which a normal end does.
- **Tie-breaking:** among nodes of equal cost, the binary heap's order decides; among goal cells at equal
  distance, the first in the goal's cell order becomes `goalX/Y`.
- **Turn costs:** a turn within the first 5 steps of a straight run costs 75 more, and a cell of pass
  state 1 costs 30 more.
- **The straight-line probe:** it walks greedily, then follows walls both ways, before any A* step. Its
  outcome sets 0x100 or 0x200 even when the search goes on.
- **Unit slots, not units:** the cursor walks slots, empty ones included, and each visit costs a step.

## 7. The world the search reads

`PathWorldView`, a table the engine fills (`g_seams.pathworld`, `src/seams/pathfinding_world.cpp`).
Every slot reads only, apart from `BeginSearch` and `EndSearch`.

| Slot | What it gives | In 3.1c |
|---|---|---|
| `Tick` | the tick being played | `g_game->ticks` |
| `PlayerCount`, `IsPlaying` | the players that share the budget | `numPlayers`, and an active human, AI or network player |
| `UnitsPerPlayer`, `FirstUnit`, `LastUnit`, `NextUnit` | each player's unit slots, as handles | the players' unit arrays, walked by the cursors |
| `UnitState` | a unit's player, cell, heading, movement class and its class's footprint | the unit and its `UnitMotion` |
| `MoveClass` | the movement classes by number | `g_movementClasses` |
| `InBounds`, `Pass` | a class's pass map | `MovementClassInBounds`, `MovementClassGet` |
| `SeenSize`, `Seen` | a player's visibility grid, 2 by 2 cells a square | `visibilityMask` |
| `LineOfSight` | a straight walk over a class's pass map | not in 3.1c; for any-angle implementations |
| `BeginSearch`, `EndSearch` | bring the pass map up to date around a search for a unit | `RefreshMovedUnits`, `RefreshUnitIfStale` |

Units and classes are handles, and the view gives no pointer into the engine, so an implementation in
a WASM module can use the same table through imports.

`CellsChanged` is the other way in: the map calls it after a terrain or feature change re-stamps the
pass maps (`src/map/terrain.cpp`, `src/map/features.cpp`). It names a rectangle of cells, not a class.
The default ignores it (it reads the pass maps live); the HPA\* demo marks its clusters there stale.

## 8. The private region

The default's state is one block of the state block, allocated by `Init` (`SimAllocTagged`, tag
`AISearch region`, in `src/ai/pathfind.cpp`) and freed by `Shutdown`. `Game::pathfinder` holds its
reference. Its size follows from the map at load, and it never grows:

| Part | Size | Holds |
|---|---|---|
| `Pathfinder` record | 208 bytes | 3.1c's record, as section 16 hashes it |
| `PathExtra` | fixed | the capacities and offsets of the parts below, the current search's goal area, and the three section 17 globals |
| node pool | min(cells + 1, 0x10000) `Node`s | the open and closed nodes |
| heap items | as many `Ref<Node>`s | the open list |
| grid | one `PathCell` per cell (rounded up to 8) | the cells' marks |
| dirty words | one bit per 8 cells | which cells a search marked |
| pending requests | unit slots + 16 `PathPending`s, and two hash indexes | the requests not yet started, by requester and by unit |
| events | 512 `PathEvent`s | the last step's events, until polled |

A search's goal cells, which it needs only while it starts, are on the stack: every call out of
pathfinding (the goal maths, the world view) gets a copy or a local, never a pointer into the region.

The pool and the heap items are counted in the record but filled only as a search needs them, as 3.1c's
`GrowNodes` did, so the bytes the trace hashes are 3.1c's.

Only pathfinding reads or writes the region and `Game::pathfinder`. Two checks hold that:
- `tools/seamlink.py`, statically, over every function's field accesses (`data/rwsets.csv`);
- the access log (#835), at run time, over every scenario.

`Project` gives the harness pointers into the region (the record and the three globals), so the trace
hashes the region without the harness knowing its layout. Another implementation may give null
pointers, and sections 16 and 17 are then empty.
