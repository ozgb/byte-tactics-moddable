# Sim events

The sim events are how the game logic tells the presentation what happened in a tick, so that the logic calls no presentation code and keeps no result of any. The logic records an event into a queue in the state block, and the presentation reads the queue and acts on each event. This page states the queue and each kind of event: what is recorded, when, and with what values. What the presentation does with an event is given in one line for each kind, for orientation, and is not part of the contract the logic is judged by.

## Conventions

- **Phases.** A tick runs the phases `net` (0, network packets), `units` (1), `projectiles` (2), `explosions` (3), `players` (4), `features` (5), `wind` (6) and `meteors` (7), in this order, and then the resource sharing of network play (8) at the end of the step. Two more labels are used: `match load` (the code that builds the match state) and `outside the steps` (code that records the event and that no phase and no match load reaches in the call graph: console commands, interface actions, match shutdown, and code nothing calls).
- **"Can be emitted in".** The `Phases:` line of a kind lists the phases from which a path of calls reaches a record of that kind. It means a path exists in the static call graph of the game logic, not that a scenario has shown it. The event log of the reference scenarios will show which kinds the scenarios reach. A tool checks each `Phases:` line against the call graph.
- **Units** are referred to by their index in the unit table of the state block, never by address. A unit index in an argument is only meaningful while that unit exists; the presentation checks whether it is still alive.
- **Players** are referred to by their index (0 to 9 for the ten player slots). The value 10 means no player. Where a kind says network id, the argument is the player's network identifier, not a slot.
- **Positions** are three consecutive arguments, x, y and z, each a signed 32-bit value stored in an unsigned argument, in the same fixed-point world coordinates as unit positions in the state block (16 fractional bits; y is height). Where a kind lists a position as "(3)", it takes three arguments.
- **Boxes** are two positions, the low corner and the high corner, six arguments.
- **Particle list** is the number of the list in which the presentation keeps the particles of that effect. The values the logic uses are given for each kind. A particle kind that the logic cannot change is not an argument.
- **Sounds, texts and screens** are referred to by a number in a table, given in the tables of the last sections. A sound by index (as in the kinds that take "the sound index of the game") is the index of a sound defined by the game data, as a weapon definition names it.
- **Integers** are signed 32-bit values stored in the unsigned arguments, in two's complement, unless the kind says unsigned.
- Unused arguments of a record are 0.

## The record

One record is 32 bytes.

| Field | Offset | Size | Meaning |
|---|---|---|---|
| `kind` | 0 | u16 | The kind, a value of the table in the last section. 0 never appears in a recorded event. |
| `flags` | 2 | u16 | Bit 0 and bit 15 have meanings (below). The other bits are 0. |
| `tick` | 4 | u32 | The value of the tick counter when the event was recorded. An event recorded outside a step carries the counter as the last step left it. |
| `args` | 8 | 6 x u32 | The arguments, in the order the kind gives, 0 where unused. |

Flag bit 15 (0x8000) marks a continuation record: a record that carries the arguments past the sixth of the event before it. Flag bit 0 (0x0001) is used by `SIM_EVENT_UNIT_SPEECH` only, and means the unit speaks only while it is off screen.

An event of up to six arguments is one record. An event of seven to twelve arguments is two records, one straight after the other: the first holds the arguments one to six, the second the arguments seven to twelve, and has flag bit 15 set. In a first record, flag bit 15 is clear. The two records are recorded together or not at all. No event of the kinds below has more than ten arguments. The argument numbers of a kind below count from the first record on, so argument 9 is the fourth argument of the second record.

## The queue

The queue is one block of the state block: four u32 counters followed by the ring of records.

| Field | Offset | Size | Meaning |
|---|---|---|---|
| `head` | 0 | u32 | The count of records drained. |
| `tail` | 4 | u32 | The count of records recorded. |
| `dropped` | 8 | u32 | The count of events that did not fit when playing. |
| `reserved` | 12 | u32 | Always 0. |
| `records` | 16 | 4096 x 32 bytes | The ring. Record number n is at index n modulo 4096. |

The queue is 131088 bytes. The pending events are the records numbered from `head` up to but not including `tail`, oldest first.

**Capacity.** The ring holds 4096 records. The queue is full when `tail` minus `head`, plus the records the new event needs, would exceed 4096.

**Order.** Events are recorded in the order the logic produces them, and the order is the same on every machine. Within one phase the pending events are in record order, and a phase's events come before the next phase's. The presentation takes the records oldest first. A two-record event is read as one event, at the place of its first record.

**Overflow.** When an event does not fit, no part of it is recorded. In a test-mode run, the run ends with a fatal error that says the sim event queue is full, so that a scenario can never lose an event without being seen. When playing, `dropped` counts the event and the logic goes on. The largest count the reference scenarios record in one step is 123 records, and the largest count pending at a drain is 352 (the first drain after a match load, which holds one plume for each geothermal vent of the map).

**Empty at the start and end.** The queue is empty at the start of a match, after a load, and after each drain. It is not part of a saved game and is in no trace, so it never changes a digest.

**Draining.** The presentation drains the queue (acts on every pending record, then leaves the queue empty) at these points, in every mode:

- at the start of each frame, before the speech queue is first read;
- at the start of the call that runs the steps of a frame, before the first step, for what was recorded outside a step;
- inside each step, after each of the phases 1 to 7, so that the events of a phase are drained before the next phase runs (the drain after phase 7 is before the camera follows and before the particles update);
- at the end of each step, for what the later parts of the step recorded (network resource sharing, the blink of the player's display, the expiry of line-of-sight markers);
- when a match is shut down, after every unit has been killed and before the units are freed, for what those kills recorded, which name units by index.

There is no drain after the last step. Each drain sits before the first reader of what was recorded. Effects that draw from the view random stream draw at the drain, in record order, and that stream is not hashed.

**Radar events.** The two radar kinds carry no values in the record. The logic writes a snapshot into a second block of the state block, the radar arena (2 MiB), and records the snapshot's byte offset. The arena is emptied by a drain that finds the queue empty. A full arena ends a test-mode run with a fatal error and, when playing, the snapshot and its event are dropped.

## The kinds

The kinds are 1 to 35 except 24, which is not used. Each section is headed by the name of the kind. Every section gives the arguments, the flags, when the logic records the kind, the phases, and what the presentation does with it.

### SIM_EVENT_NONE

Value 0. Marks an empty record. It is never recorded and has no arguments, no flags and no phase.

Phases: none.

### SIM_EVENT_UNIT_SPEECH

Value 1.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit that speaks. |
| 1 | integer | The kind of line: the voice slot, as the unit's sound category names it. |
| 2 | speech text number | 0 for the text of the line's kind, or a number of the speech texts table. |

Flags: bit 0, the unit speaks only while it is off screen (set for slot 2 only).

Recorded when a unit announces something. The sim records nothing for a unit that does not exist or is already dying (the unit's alive flag 0x10000000 clear, or its dying flag 0x4000 set). Units placed at the start of a match speak at once. The voice slot (argument 1) is a line kind of the table "Speech line kinds" below. Where a unit is named, it is the unit of the event. The sim records each slot from these places:

- 2, underattack: when damage of any damage kind except 10 (a heal, which returns at once) and except 11 (which skips the reaction to the attack) is applied to a unit that exists and is not dying. The test is made inside the reaction to the attack, which runs before the damage event updates the unit's last damage type and last attacker slot. So the test uses the last attacker slot and the last damage type of the previous damage event (or the unit's initial values at the first), not those of the event that caused the reaction. The line is recorded when the unit's order flags do not have bit 7 (0x80) set and either that previous attacker slot is not the unit's own player or that previous damage type is 1. The event has flag bit 0 set.
- 3, activate and 4, deactivate: when the unit's activation bits change and bit 0 (value 1) of them is gained or lost.
- 14, cloak and 15, uncloak: when the activation bits change and bit 2 (value 4) is gained or lost. The economy sets that bit every 30 ticks, when it settles the owner's accounts, from whether a cloaked unit pays its cloak cost.
- 5, ok: when an order whose status flag 0x2000 is set runs its first step. The flag is cleared by the record, so each order announces once. The text is the one the order gives (Attacking, Building, Guarding, Loading, Loading unit, Patrolling, Preparing for transport, Reclaiming, Repairing, Unloading) or the kind's own text.
- 6, arrived: when a ground move order or a kamikaze attack order is stepped with call flag 0x20 set (the unit has arrived), and when an air move order reaches its last state and has no next order (an air move with a next order records nothing).
- 7, cant: when an order fails or is refused, with the text that names the reason (the failure and refusal texts of the speech texts table).
- 8, unitcomplete: when a build order finishes (kind's own text on the factory build order), when a mobile or air build or help-build finishes ("Building complete") and when a resurrect order finishes ("Resurrection complete").
- 9, build: when construction starts ("Starting construction").
- 10, repair: when a repair finishes ("Unit repaired").
- 11, working: when a capture, reclaim or resurrect order has found its target and is about to start work.
- 12, load: for a ground transport, each time it starts a pickup attempt (state 2 of the pickup order; after three attempts the order gives up and is deleted only when another order follows it, otherwise it restarts after 30 ticks and attempts again); for an air transport, once after the cargo has been attached to its piece.
- 13, unload: for a ground transport, when the unit being unloaded is no longer carried by the transport, which is after the drop; for an air transport, after the cargo has been detached and the transport has climbed away.
- 16, capture: when a capture order has finished and the captured unit has been given to the capturing unit's player.
- 17 to 22, count5 to count0: one for each step of a self-destruct order, whose countdown starts at the unit definition's self-destruct countdown (which must be greater than 0; with 0 the order destroys the unit at once and records nothing). Each step records the slot 22 minus the current count (count 0 gives slot 22, count 5 gives slot 17) and then, for a count above 0, waits 30 ticks; at count 0 it waits a random number of ticks drawn with the sim random stream (a value from 0 to 14) and then destroys the unit with kill type 3. A countdown above 5 reads past the end of the six-entry table of slots, which is unspecified.
- 23, canceldestruct: when a self-destruct order that has not finished is stepped with call flag 2 (the order is cancelled) and the unit is not dying.

No call site records slot 0 or slot 1 (select, which the interface handles).

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation says the line only for a unit of the local player, and drops it when the unit has died by then or, with flag bit 0, is on screen. The line enters a queue that keeps one line per kind, ordered by priority.

### SIM_EVENT_SOUND

Value 2.

| Argument | Type | Meaning |
|---|---|---|
| 0 | sound name number | A number of the sound names table. |
| 1 | integer | The play flag. 0 in every use. |

Flags: none.

Recorded when a victory condition of the mission is first satisfied, once for each condition (sound name 0). The conditions are: the enemy commander is killed, all enemy units are destroyed, all mobile enemy units are killed, a unit of the given type is built, a unit of the given type is captured, all units of a type are killed, a unit of a type is killed, a unit reaches a radius, a unit of a type passes a given x, a unit of a type passes a given z.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation plays the sound by name.

### SIM_EVENT_NANO_PARTICLES

Value 3.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The origin, the nanolathe point of the builder. |
| 3 to 8 | box | The low and high corners of the target. |
| 9 | integer | The particle list. Always 6. |

Flags: none. Two records (ten arguments).

Recorded by these orders, once for each step that does the following. The origin is the nanolathe point of the builder unless the site says otherwise; the box is the target's position plus the model's extents: the low corner adds the minimum x and z (and also the minimum y at the sites marked "full box"), the high corner adds the maximum x, y and z, so at the other sites the low y is the target's own y.

- Factory build (a builder that creates a unit and builds it in place): each step that adds build progress to the unit it is building.
- Mobile build, help-build and air mobile build and air help-build (all "full box"): each step that adds build progress.
- Ground repair (unit repairs another unit within range), repair without moving, and the repair of a unit by another unit (the order whose origin is the other unit's nanolathe point and whose box is the repaired unit's): each step in which the repair progress call adds health.
- Air repair: each step that runs the repair progress call, whether or not it added progress.
- Resurrect: each step of the countdown while the countdown is not 0 (the countdown decrements on each step). The box is the footprint of the feature being resurrected: its low corner is the feature's cell, at the ground height there, and its high corner adds the footprint size in cells and the feature's height.

There is no air resurrect. The flow runs from the origin to the box.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation draws a beam of particles flowing from the origin to the box.

### SIM_EVENT_REVERSE_NANO_PARTICLES

Value 4.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 5 | box | The low and high corners of the source. |
| 6 to 8 | position | The origin, the point the particles flow to. |
| 9 | integer | The particle list. Always 6. |

Flags: none. Two records (ten arguments).

Recorded by these orders. The origin is the nanolathe point of the builder. For a unit target the box is the unit's position plus its model extents (the low corner adds minimum x and z, the high corner adds maximum x, y and z).

- Capture: each step while the order's elapsed time is below its duration and the target is not moving, one event.
- Reclaim of a unit (ground and air): each step in which the unit is within build range and can be reclaimed, one event.
- Reclaim of a feature: each step while the order's remaining time is above 15 for a ground builder, or above 30 for an air builder, two identical events. The box is the feature's footprint: its low corner is the feature's cell at the ground height there, and its high corner adds the footprint size in cells and the feature's height.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation draws a beam of particles flowing from the box to the origin.

### SIM_EVENT_TELEPORT_PARTICLES

Value 5.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The start, the position of a unit that is moved. |
| 3 to 5 | position | The end: the order's position, plus the moved unit's position, minus the teleporter's position (each unit keeps its offset from the teleporter). |
| 6 | integer | The lifetime in ticks. Always 30. |
| 7 | integer | The particle list. Always 5. |

Flags: none.

Recorded when a teleport order runs (once, in its only step), once for each slot of the unit table other than the teleporter whose position lies inside the box of the teleporter: the teleporter's position plus the minimum model extents of its definition (low corner) and plus the maximum extents (high corner), compared on x, y and z, inclusive. The scan does not test whether the slot holds a living unit, so a free slot whose stale position lies in the box records too. The event is recorded just before that slot's position is set to the end position.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation draws sparks flying from the start to the end.

### SIM_EVENT_THRUST_PARTICLES

Value 6.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The start. |
| 3 to 5 | position | The end. |
| 6 | integer | The period in ticks. Always 1. |
| 7 | integer | The lifetime in ticks, 6 or 7. |
| 8 | integer | The particle list. Always 7. |

Flags: none. Two records (nine arguments).

Recorded when a unit's script emits effect 0 (lifetime 6) or effect 1 (lifetime 7) from a piece. The start and end are the piece's two offsets from the unit's position (the second offset has its z subtracted as the first does). The script effect is dropped, with nothing recorded, unless the unit is visible to the player being viewed (the sim's player index for the viewing machine); this depends on the local machine's viewed player.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation streams a jet plume from the start to the end.

### SIM_EVENT_WAKE_PARTICLES

Value 7.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The start. |
| 3 to 5 | position | The end. |
| 6 | integer | The period in ticks, 16 or 8. |
| 7 | integer | The particle list. Always 2. |

Flags: none.

Recorded when a unit's script emits effect 2 (period 16), 3 (period 8), 4 (period 16, positions swapped) or 5 (period 8, positions swapped) from a piece. The positions are the piece's two offsets from the unit's position. The script effect is dropped, with nothing recorded, unless the unit is visible to the player being viewed (the sim's player index for the viewing machine); this depends on the local machine's viewed player.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation trails a wake from the start to the end.

### SIM_EVENT_BUBBLES

Value 8.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The start. |
| 3 to 5 | position | The end. |
| 6 | integer | The period in ticks. Always 8. |
| 7 | integer | The particle list. Always 7. |

Flags: none.

Recorded when a unit's script emits effect 0x103 from a piece. The start is the piece's position (the unit's position plus the piece's position, with z subtracted) and the end has the same x and z at the sea level height. The script effect is dropped, with nothing recorded, unless the unit is visible to the player being viewed (the sim's player index for the viewing machine); this depends on the local machine's viewed player.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation raises bubbles from the start to the end.

### SIM_EVENT_WEAPON_SMOKE

Value 9.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | Where the weapon fired. |
| 3 | integer | The particle list. Always 9. |

Flags: none.

Recorded when a unit fires a line-of-sight, vertical-launch or ballistic weapon whose definition has the start smoke property. It is recorded once the projectile slot has been allocated (the projectile table holds fewer than 300 projectiles; with no free slot the shot is not fired and nothing is recorded) and after the fire script and the recoil script have been started. The position is the firing position the fire step was given. Projectiles dropped by an air unit, projectiles created by the motion of another projectile, meteors and projectiles created from a received network fire packet do not record it.

Phases: net (0), units (1), match load.

The presentation puffs smoke at the position.

### SIM_EVENT_TIMED_SUB_PARTICLES

Value 10.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The centre of the feature's footprint, or the position the feature was placed at. |
| 3 | integer | The particle list. Always 4. |

Flags: none.

Recorded when a feature whose definition has a geothermal vent is placed on the map. This happens for every such feature when a match is loaded (so the first drain of a match can hold hundreds), and again whenever such a feature is placed later, as when a burnt feature replaces a feature that burned down.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation starts a timed plume of particles rising over the vent.

### SIM_EVENT_SCREEN_SHAKE

Value 11.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The x offset. |
| 1 | integer | The y offset. Equal to argument 0 in every use. |
| 2 | integer | The strength: the shake duration of the weapon definition. |

Flags: none.

Recorded once for each detonation of a projectile, with the shake magnitude of the weapon definition as both offsets and its shake duration as the strength, whatever the values (they may be 0). A detonation records nothing when the network game option that disables the sea level trigger is set, the impact cell is below sea level and no unit was hit.

Phases: net (0), units (1), projectiles (2), features (5), match load.

The presentation adds the shake to the screen's shake, unless the player turned screen shake off.

### SIM_EVENT_CD_ACTIVITY

Value 12.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The amount: 1 for a hit, 5 for a kill. |
| 1 | player index | A player. |
| 2 | player index | A second player. |

Flags: none.

Recorded in two cases.

- A hit: when damage is applied to a unit that exists and is not dying, with damage kind other than 10 (kinds 2 and 11 do record), from a damage record that names an attacker unit, and either the attacker's player or the victim's player is the local player. The arguments are 1, the attacker's player and the victim's player.
- A kill: when a unit with a player dies with kill type 1 or 6, or kill type 5 with a last attacker slot that is neither 10 nor the unit's own player, and the unit's last attacker slot is the local player. The arguments are 5 and the last attacker slot twice. It records for the local player's own units and for unfinished units too, and never for kill type 3.

Both tests use the local player, so the event differs between machines.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation counts the amount in the disc activity meter, which picks the music, when either player is the local player.

### SIM_EVENT_FORCES_DESTROYED

Value 13.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The player whose side has lost its last unit. |

Flags: none.

Recorded when a unit dies and it was the last unit of its player, in a skirmish (game type 2).

Phases: net (0), units (1), match load.

The presentation prints the message that the side's forces are destroyed, in the side's name, with one of three texts chosen from the view random stream.

### SIM_EVENT_SCORE_FLASH

Value 14.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The killer's slot. Never 10 as recorded. |
| 1 | player index | The player who lost the unit. |

Flags: none.

Recorded when a unit's death is credited and its last attacker slot is not 10. A death is credited by its kill type:

- Kill types 1 and 6: credited whenever the unit has a player, including when the last attacker slot is the unit's own player.
- Kill type 5: credited only when the last attacker slot is neither 10 nor the unit's own player, and then as kill types 1 and 6 are.
- Kill type 3: credited when the unit has a player and the local player does not share vision with that player. This depends on the local player, so it differs between machines, which can desynchronise the score lists of the machines.
- Other kill types are never credited.

Arguments are then the last attacker slot and the unit's player.

Phases: net (0), units (1), match load.

The presentation flashes the kill and the loss on the score panel, when the player's options show the score panel.

### SIM_EVENT_SELECTION_ORDERS_DIRTY

Value 15.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit. |

Flags: none.

Recorded in these cases:

- Any change of the unit's activation byte (all eight bits, including bits 3 and 4), after the change.
- A help-build order is cancelled (call flag 2) or starts building.
- A mobile build order is terminated (call flag 8), cancelled (call flag 2) or starts building (after "Starting construction").
- An air mobile build order or air help-build order is cancelled, is terminated (including terminated by hostile action) or starts building.
- A resurrect order creates the unit (after the target's build left is set to 0 and its health to 1).
- A get-built order is stepped while its unit's build left is 0.

Repair, reclaim and capture orders never record it.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation redraws the orders panel if the unit is selected and belongs to the local player.

### SIM_EVENT_BUILD_MENU_UPDATE

Value 16.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit. |

Flags: none.

Recorded for the builder in these cases:

- A factory build order is called with flag 2 (cancel): when it has a target, construction is finished for the target first (the last case below), then the event is recorded again before the order ends.
- A factory build order is called with flag 8 (stop), after its build count is decremented.
- A factory build order has started building a new unit, after the unit is created and attached.
- A factory build order finishes the unit (after construction is finished for it and the build count is decremented).
- A stockpile order adds a stockpiled weapon to the unit (state 2, after the stockpile count and the build count change).
- A weapon with a stockpile is fired and the unit's stockpile is decremented.
- Construction of a unit is finished for the builder, which requires the builder to exist and have a build list in its definition and the target to exist. This runs from the factory order above, from the interface and from the network packet that finishes construction.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation writes the queued build counts into the build menu if the unit is the one it shows.

### SIM_EVENT_DESELECT_IF_INELIGIBLE

Value 17.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit. |

Flags: none.

Recorded at the end of an attach order, after the unit's carrier and motion mode have been set, when the unit exists, does not have unit flag 0x20000000 (a structure), carries no cargo of its own, and the order's target is either none (the unit is detached) or an existing unit other than the unit that is itself not carried. Attaching and detaching both record it.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation removes the unit from the selection when it can no longer be selected (unfinished, in transit, or carried by a transport that may not carry it).

### SIM_EVENT_GUI_CHANGED_IF_SCREEN

Value 18.

| Argument | Type | Meaning |
|---|---|---|
| 0 | screen number | A number of the screens table. |

Flags: none.

Recorded when construction of a unit is finished (see `SIM_EVENT_BUILD_MENU_UPDATE`), the builder exists and has a build list in its definition, the target exists, the target's player is active and of type 1 or 2, and the target unit has its unit flag 0x20000000 set (the unit is a structure that stays in place). The screen is 0, the build menu.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation marks the GUI as changed when that screen is on top.

### SIM_EVENT_FOCUS_COMMANDER

Value 19.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The force flag. Always 1. |

Flags: none.

Recorded in the players phase, in the 30-tick update of the local player, when the game type is not 1 (not a campaign mission), the player is inactive or its info flag 0x40 is clear, the defeat check of the mission conditions passes, the end-game countdown (4 at the first pass, decremented on each later pass) has run below 0, and the commander-death setting is "Deathmatch" (value 2; 0 is "Game Continues", 1 is "Game Ends"). The sim then creates a new commander for the local player at a random place found with the sim random stream, adds the starting storage, recalculates the line of sight and records the event.

Phases: players (4), match load.

The presentation centres the camera on the commander of the viewed player and, with the flag set, selects it and closes the open screens.

### SIM_EVENT_POP_TO_GAME_LAYOUT

Value 20.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The force flag. Always 1. |

Flags: none.

Recorded when the commander of a human or computer-controlled player is killed in a match whose commander-death setting is "Game Ends" or "Deathmatch" (not 0), just before all that player's units are killed.

Phases: net (0), units (1), match load.

The presentation closes the screens above the game screen, unless a dialog is open and the flag is 0.

### SIM_EVENT_PLAYER_TOOK_LEAD

Value 21.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The player who has taken the lead. |
| 1 | integer | The kills that put it there: the commander kills when the commander-death setting is "Deathmatch", else the kills. |

Flags: none.

Recorded when a credited death (as for `SIM_EVENT_SCORE_FLASH`, with the last attacker slot not 10) moves the attacker to rank 0 on the score list. The attacker's player must be active, of type 1, 2 or 3, with index not 10 and rank above 0 before the move; the game type must be 2 or 3; and the attacker's count (the commander kills in Deathmatch, else the kills) must exceed the count of a player of type other than 0 whose watch flag is clear and whose rank is 0. The kills in the argument are counted after the kill that was just credited (kill types 1, 5 and 6 add one; kill type 3 adds none).

Phases: net (0), units (1), match load.

The presentation prints the message that the player has taken the lead, in the player's name and with the kills.

### SIM_EVENT_PLAYER_LEFT

Value 22.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The player's network id. |

Flags: none.

Recorded when a unit dies and it was the last unit of its player, in a multiplayer game (game type 3).

Phases: net (0), units (1), match load.

The presentation prints that the player left the game, in the player's name, with one of eight texts chosen from the view random stream, unless the local player was rejected or no slot has the id.

### SIM_EVENT_BUILD_QUEUE_SOUND

Value 23.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The player of the unit. |
| 1 | integer | The change in the build count. Always 1 as recorded. |

Flags: none.

Recorded when a computer-controlled player queues a build order on a factory that has no queue, from the AI's build timer (once every 30 ticks for each of its units) and from the code that issues a first order to an idle factory.

Phases: players (4), match load, outside the steps.

The presentation plays the add sound of the build menu when the player is the one shown, and the subtract sound when the change is not positive.

### SIM_EVENT_WATCH_PROMPT

Value 25.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The defeated player. |

Flags: none.

Recorded on the same path as `SIM_EVENT_FOCUS_COMMANDER` (the local player's 30-tick update, game type not 1, the defeat check passing and the end-game countdown run below 0) when the commander-death setting is not "Deathmatch". In a multiplayer game (game type 3) with the local player not rejected, when the host's "Watching" option is "Allowed" or an AI player is active, the local player is placed in watch mode (its watch flag is set), the line of sight is recalculated, and if no AI player is active the event is recorded. In any other case the match ends instead and nothing is recorded.

Phases: players (4), match load.

The presentation asks the local player whether to keep watching, when the player is the local player.

### SIM_EVENT_WATCH_MODE_NOTICE

Value 26.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The player placed in watch mode. |

Flags: none.

Recorded on the same path as `SIM_EVENT_WATCH_PROMPT`, when an AI player is active (instead of the prompt) and at least one combat player remains.

Phases: players (4), match load.

The presentation tells the local player that hosting AI players that are still alive keeps the match going, when the player is the local player.

### SIM_EVENT_SMOKE

Value 27.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | Where the plume rises. |
| 3 | integer | The emit period in ticks: 15 over a corpse, 7 behind an explosion. |
| 4 | integer | The lifetime in ticks: 900 over a corpse, 15 behind an explosion. |
| 5 | integer | The particle list. Always 9. |

Flags: none.

Recorded in two cases.

- A wreck: when a unit dies with wreckage level above 0 and the wreck, after following the next-wreck chain one step for each level above 1, has an id below 0xfffb, the unit's cell exists, and the kill type is not 7. Above sea level (the ground height at the unit is above the sea level) it is recorded whether or not the wreck was placed. At or below sea level it is recorded only when placing the wreck failed; a wreck that was placed under water records no smoke. The position is the unit's position; period 15, lifetime 900.
- An explosion effect: when fewer than 300 explosion effects exist, the caller's flag is 0, and the whole part of the effect's y is above the sea level. The flag is 0 for an impact on a cell that is not below sea level, for explosions that a unit script makes from a piece, and for two debris paths; it is 1 for an impact on a cell below sea level, for the splash of a projectile crossing the sea level and for the other debris paths. Period 7, lifetime 15.

Phases: net (0), units (1), projectiles (2), explosions (3), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation starts a smoke plume. The smoke picks its animation frames from the view random stream.

### SIM_EVENT_WHITE_SMOKE

Value 28.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | Where the puff is. |
| 3 | integer | The particle list: 9, or 5 for a burning feature. |

Flags: none.

Recorded in these cases:

- A projectile detonates with the end smoke property, and the detonation is not the water case (the impact cell is not below sea level, or a unit was hit). Over a cell below sea level with no unit hit no white smoke is recorded.
- A ballistic projectile with a non-zero lifetime reaches the end of its time (its time is not later than the current tick), has no burn-blow property, and is removed without exploding.
- A projectile with the smoke trail property that is alive after its step, whose time is later than the current tick and whose next smoke tick is earlier than the current tick; the next smoke tick then advances by the weapon's smoke rate. The position is the projectile's position and the list is 9.
- A burning feature with no 3D object (a sprite feature whose spot flag bit 0 is set), when the tick counter is divisible by 3. The list is 5. The position is the centre of the feature's footprint, offset by two draws of the sim random stream scaled by the size and offset of the current animation frame.
- A unit's script emits effect 0x101 from a piece, at the piece's position (the unit's position plus the piece's position, with z subtracted), subject to the visibility test of the other script effects, which depends on the viewed player of the local machine.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation puffs white smoke at the position.

### SIM_EVENT_BLACK_SMOKE

Value 29.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | Where the puff is. |
| 3 | integer | The particle list. Always 9. |

Flags: none.

Recorded when a unit's script emits effect 0x102 from a piece, at the piece's position (the unit's position plus the piece's position, with z subtracted). The script effect is dropped, with nothing recorded, unless the unit is visible to the player being viewed (the sim's player index for the viewing machine); this depends on the local machine's viewed player.

Phases: net (0), units (1), projectiles (2), players (4), features (5), meteors (7), resource sharing (8), match load.

The presentation puffs black smoke at the position.

### SIM_EVENT_SOUND_AT

Value 30.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The sound index of the game. |
| 1 to 3 | position | Where the sound is. |
| 4 | unsigned | The set of players who hear it: bit n is set for player n. |

Flags: none.

Recorded in these cases:

- When a projectile is created (all projectile creation: unit weapons, meteors, projectiles from received network fire packets), with the weapon's sound. This is recorded unconditionally, including a sound index of 0.
- When a projectile of a burst weapon with the sound trigger property spawns the next projectile of its burst, only if a projectile slot was allocated for it; the position is the burst projectile's position, after it was moved to the weapon piece when the burst rate is 5 or more or its remaining burst count is odd.
- When a projectile detonates (see `SIM_EVENT_SCREEN_SHAKE` for when a detonation records anything): the water sound when the impact cell is below sea level and no unit was hit, otherwise the hit sound.

The set of players who hear it (argument 4) is decided when it is recorded. It is 0 if the position's map cell (x and z divided by 16 world units) does not exist. Otherwise each player 0 to 9 is in the set when the position's cell in that player's explored map is inside the map: the position's whole x divided by 32, and its whole z minus half its whole y divided by 32 (the position projected by its height), indexes the map; the player is in the set when, with the map flag 2 set, that player's explored map entry is non-zero, and with it clear, the player's bit in the visibility mask of that cell is set.

Phases: net (0), units (1), projectiles (2), features (5), meteors (7), match load.

The presentation plays the sound at the position, when the sound settings allow it and the local player is in the set. With 3D sound off, a sound off screen plays quieter.

### SIM_EVENT_SOUND_AT_NAME

Value 31.

| Argument | Type | Meaning |
|---|---|---|
| 0 | sound name number | A number of the sound names table. |
| 1 to 3 | position | Where the sound is. |
| 4 | unsigned | The set of players who hear it, decided as for `SIM_EVENT_SOUND_AT`. |

Flags: none.

Recorded when a feature starts burning (sound name 1), at the position of the feature's cell: x and z are the cell coordinates shifted left by 20 and y is the cell's height shifted left by 16.

Phases: net (0), units (1), projectiles (2), features (5), match load.

The presentation plays the named sound at the position with the checks of `SIM_EVENT_SOUND_AT`.

### SIM_EVENT_REMOVE_UNIT_SPEECH

Value 32.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit that died. |

Flags: none.

Recorded when a death is applied to a unit that exists, after the dead unit's orders are deleted.

Phases: net (0), units (1), match load.

The presentation drops the unit's pending lines from the speech queue.

### SIM_EVENT_MESSAGE

Value 33.

| Argument | Type | Meaning |
|---|---|---|
| 0 | message text number | A number of the message texts table. |
| 1 | integer | The value. Texts 0 to 4: 0 for off, not 0 for on (the share-metal value is 0 or 1, the share-energy value is 0 or 4, share-mapping 0 or 32, share-radar 0 or 64, share-LOS 0 or 1). Texts 5 and 6: the number the command was given. |

Flags: none.

Recorded by a console command run by the local player, only in a network game (the net game flag 1 set): the toggles of sharing metal, energy, mapping, line of sight and radar (texts 0 to 4), with the new state, and the commands that set the thresholds for sharing metal and energy (texts 5 and 6), with the first argument of the command as given. The command that toggles all sharing records four events: metal, energy, mapping and radar (not line of sight). The radar toggle's value is the player's share-radar flag bit (64), tested after the toggle.

Phases: match load, outside the steps.

The presentation puts the line in the message log: the text with ON or OFF for a toggle, or with the number.

### SIM_EVENT_RADAR_UNITS

Value 34.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unsigned | The byte offset of the snapshot in the radar arena. |

Flags: none.

Recorded for the viewed player (the player index of the viewing machine), once in each tick for each pass of the players phase over that player (the player must be active, of type 1, 2 or 3, with index not 10), before that player's 30-tick update, and again after each line-of-sight recalculation. If the radar arena has no room the snapshot is not taken and no event is recorded.

The snapshot is 8 bytes: the unit count and the projectile count (u32 each). Then come the unit records, then the projectile records.

A unit record is 48 bytes: slot u16 (index in the unit table), id u16, whole part of x i16, 2 reserved bytes, y scaled i32 (the whole part of z minus half the whole part of y), colour u8 (the player's colour), flags u8 (1 recently damaged, 2 shows ranges: the unit's activation bit 0 is set or its definition's flag 4 of the second flags byte is clear, 4 has interceptors), radar distance i16, sonar distance i16, radar jam distance i16, sonar jam distance i16, 2 reserved bytes, then three weapon records of 8 bytes (coverage i32, interceptor u8, stocked u8, 2 reserved), which are non-zero only for a unit whose definition has the interceptor flag and for a slot that has a weapon. A unit is included when its definition index is not 0 and either radar is enabled (the map flags 1 and 2 are both clear, or the match flag 2 is set), or the unit has a flag in 0x300, or it belongs to the viewed player.

A projectile record is 12 bytes: x i32 (whole part), y i32 (whole z minus half the whole y), kind u8 (1 for an ordinary projectile, 2 for a nuclear one), colour u8 (the owner's colour for kind 2, else 0), 2 reserved bytes. An ordinary projectile (weapon flags 0x60000000 clear) is included unless its weapon has flag 0x40, and when it is on the radar (explored by the viewed player when the map flag 2 is set, else visible to the viewed player) or owned by the viewed player. Any other projectile is included when it is on the radar or its attacker belongs to the viewed player.

The units and projectiles shown depend on the viewed player, so they differ between machines.

Phases: players (4), match load.

The presentation draws the radar picture from the snapshot.

### SIM_EVENT_RADAR_MAPPED

Value 35.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unsigned | The byte offset of the snapshot in the radar arena. |

Flags: none.

Recorded only when the map-changed flag is set, which clears it. A snapshot is attempted in the 30-tick update of the viewed player in the players phase (after its sensors are updated) and after each line-of-sight recalculation. If the radar arena has no room, nothing is recorded.

The snapshot is 12 bytes: the width and the height of the full map and the cell count (u32 each; the cell count is half the width times half the height, each rounded down), then one byte for each of those cells in order: 0 (hidden) when the viewed player's bit in the cell's visibility mask is clear, otherwise 1 (explored) when the viewed player's explored entry for the cell is non-zero, otherwise 2 (in sight now). The viewed player differs between machines.

Phases: players (4), match load.

The presentation marks the cells on the radar picture as explored or hidden.

## Tables

### Speech line kinds

Argument 1 of `SIM_EVENT_UNIT_SPEECH` is one of these 24 line kinds, with the sound it plays and the text it shows, if any: 0 none, 1 "select", 2 "underattack" ("Under Attack"), 3 "activate", 4 "deactivate", 5 "ok", 6 "arrived" ("Arrived"), 7 "cant" ("Cannot Comply"), 8 "unitcomplete" ("Nanolathe Complete"), 9 "build", 10 "repair", 11 "working", 12 "load", 13 "unload", 14 "cloak" ("Cloaked"), 15 "uncloak" ("Visible"), 16 "capture", 17 "count5" ("five"), 18 "count4" ("four"), 19 "count3" ("three"), 20 "count2" ("two"), 21 "count1" ("one"), 22 "count0" ("zero"), 23 "canceldestruct" ("Self destruct terminated").

### Speech texts

Argument 2 of `SIM_EVENT_UNIT_SPEECH` is 0 for the text of the line's kind, or one of these, translated at the drain: 1 Attacking, 2 Building, 3 Building complete, 4 Capture failed, 5 Capturing, 6 Construction stopped, 7 Construction terminated, 8 Construction terminated by hostile action, 9 Guarding, 10 I can't get there, 11 I can't reach the construction site, 12 Landing, 13 Landing aborted, 14 Landing aborted: all pads are occupied, 15 Landing aborted: no pads available, 16 Landing failed, 17 Loading, 18 Loading unit, 19 Patrolling, 20 Preparing for transport, 21 Reclaiming, 22 Reclamation failed, 23 Repair aborted., 24 Repairing, 25 Repair mission failed, 26 Repairs unsuccessful., 27 Ressurection failed, 28 Resurrection complete, 29 Resurrection failed, 30 Starting construction, 31 Target area was blocked, 32 That unit cannot be captured, 33 That unit cannot be reclaimed, 34 That unit is a cloud of vapor and cannot be captured, 35 Transport mission failed, 36 Unable to create any more units, 37 Unable to unload unit, 38 Unit is too heavy to transport, 39 Unit is too large to transport, 40 Unit repaired, 41 Unloading, 42 Unloading process is proceeding non-optimally, 43 Waiting for target area to clear.

### Sound names

0 is "Victory Condition", the cue of a satisfied victory condition. 1 is "treeburn", the sound of a feature catching fire.

### Screens

0 is "BUILDER.GUI", the build menu.

### Message texts

0 Toggled ShareMetal to: %s, 1 Toggled ShareEnergy to: %s, 2 Toggled ShareMapping to: %s, 3 Toggled ShareLOS to: %s, 4 Toggled ShareRadar to: %s (each with ON or OFF), 5 OK.  Will share metal if above %d, 6 OK.  Will share energy if above %d.

## What stays a call

A call whose result the logic uses, or that the logic makes to set up a screen or load an asset, is not an event. Dialogs, the camera follow of the unit update, the picture of the radar, and the load-time asset calls are not on the queue. An error box that stops the game until it is answered is a call to the host, not an event.
