# Sim events

The sim events are how the game logic tells the presentation what happened in a tick, so that the logic calls no presentation code and keeps no result of any. The logic records an event into a queue in the state block, and the presentation reads the queue and acts on each event. This page states the queue and each kind of event: what is recorded, when, and with what values. What the presentation does with an event is given in one line for each kind, for orientation, and is not part of the contract the logic is judged by.

## Conventions

- **Phases.** A tick runs the phases `units` (1), `projectiles` (2), `explosions` (3), `players` (4), `features` (5), `wind` (6) and `meteors` (7), in this order, after a `net` phase (0) that handles network packets. A kind lists the phases in which it can be recorded. "Outside a step" means recorded between ticks: when a match is set up, when a console command or an interface action runs, and when a match is shut down.
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

### SIM_EVENT_UNIT_SPEECH

Value 1.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit that speaks. |
| 1 | integer | The kind of line: the voice slot, as the unit's sound category names it. |
| 2 | speech text number | 0 for the text of the line's kind, or a number of the speech texts table. |

Flags: bit 0, the unit speaks only while it is off screen.

Recorded when a unit announces something: it is given or finishes an order (starting construction, building complete, construction stopped or terminated, capture, reclaim, repair, resurrection, transport, landing and unloading successes and failures, patrolling, guarding, attacking, a refused order such as "I can't get there"), when it is activated or deactivated, when it gains or loses state bit 2 (kinds 14 and 15), and when an attacked unit reacts to damage from another player or from a damage type 1 (kind 2, with flag bit 0). The logic records it only while the unit exists and is not already dying. Units placed at the start of a match speak at once, outside a step.

Phases: units (orders, activation, damage reactions), players (activation changes made by the economy and the AI for the players' units), and outside a step (units placed at the start of a match, an order given from the interface).

The presentation says the line only for a unit of the local player, and drops it when the unit has died by then or, with flag bit 0, is on screen. The line enters a queue that keeps one line per kind, ordered by priority.

### SIM_EVENT_SOUND

Value 2.

| Argument | Type | Meaning |
|---|---|---|
| 0 | sound name number | A number of the sound names table. |
| 1 | integer | The play flag. 0 in every use. |

Flags: none.

Recorded when a victory condition of the mission is first satisfied, once for each condition (sound name 0). The conditions are: the enemy commander is killed, all enemy units are destroyed, all mobile enemy units are killed, a unit of the given type is built, a unit of the given type is captured, all units of a type are killed, a unit of a type is killed, a unit reaches a radius, a unit of a type passes a given x, a unit of a type passes a given z.

Phases: units (when a unit dies or is captured and the condition is tested on that unit), players (when the conditions are tested against the units each tick).

The presentation plays the sound by name.

### SIM_EVENT_NANO_PARTICLES

Value 3.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The origin, the nanolathe point of the builder. |
| 3 to 8 | box | The low and high corners of the target. |
| 9 | integer | The particle list. Always 6. |

Flags: none. Two records (ten arguments).

Recorded while a builder works on a build, repair or resurrect order, each time the order's step runs. The flow runs from the origin to the box.

Phases: units.

The presentation draws a beam of particles flowing from the origin to the box.

### SIM_EVENT_REVERSE_NANO_PARTICLES

Value 4.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 5 | box | The low and high corners of the source. |
| 6 to 8 | position | The origin, the point the particles flow to. |
| 9 | integer | The particle list. Always 6. |

Flags: none. Two records (ten arguments).

Recorded while a builder works on a reclaim or capture order, each time the order's step runs. A reclaim order of a unit or feature records two of these in one tick.

Phases: units.

The presentation draws a beam of particles flowing from the box to the origin.

### SIM_EVENT_TELEPORT_PARTICLES

Value 5.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The start, the position of the unit. |
| 3 to 5 | position | The end, the destination of the teleport. |
| 6 | integer | The lifetime in ticks. Always 30. |
| 7 | integer | The particle list. Always 5. |

Flags: none.

Recorded when a unit executes a teleport order, once, as the unit is moved.

Phases: units.

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

Recorded when a unit's script emits a thrust effect from a piece (lifetime 6 for one of the two thrust effects, 7 for the other).

Phases: units.

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

Recorded when a unit's script emits a wake effect from a piece. There are four wake effects in two sizes (period 16 and period 8), each with the piece's two positions in either order.

Phases: units.

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

Recorded when a unit's script emits an underwater bubble effect from a piece.

Phases: units.

The presentation raises bubbles from the start to the end.

### SIM_EVENT_WEAPON_SMOKE

Value 9.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | Where the weapon fired. |
| 3 | integer | The particle list. Always 9. |

Flags: none.

Recorded when a unit fires a weapon whose definition has the start smoke property, once for each shot (each of the three firing paths records it).

Phases: units.

The presentation puffs smoke at the position.

### SIM_EVENT_TIMED_SUB_PARTICLES

Value 10.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | The centre of the feature's footprint, or the position the feature was placed at. |
| 3 | integer | The particle list. Always 4. |

Flags: none.

Recorded when a feature whose definition has a geothermal vent is placed on the map. This happens for every such feature when a match is loaded (so the first drain of a match can hold hundreds), and again whenever such a feature is placed during play, as when a burnt feature replaces a feature.

Phases: outside a step (match load), features and projectiles (a feature replaced by its burnt version).

The presentation starts a timed plume of particles rising over the vent.

### SIM_EVENT_SCREEN_SHAKE

Value 11.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The x offset. |
| 1 | integer | The y offset. Equal to argument 0 in every use. |
| 2 | integer | The strength: the shake duration of the weapon definition. |

Flags: none.

Recorded when a projectile detonates, once for each detonation, with the shake magnitude of the weapon definition as both offsets, whatever the magnitude (it may be 0).

Phases: projectiles, units (a dying unit's weapon detonating).

The presentation adds the shake to the screen's shake, unless the player turned screen shake off.

### SIM_EVENT_CD_ACTIVITY

Value 12.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The amount: 1 for a hit, 5 for a kill. |
| 1 | player index | A player. |
| 2 | player index | A second player. |

Flags: none.

Recorded in two cases. When a unit takes damage from an attacker and either the attacker's player or the victim's player is the local player: amount 1, the attacker's player, the victim's player. When a unit dies and is credited as a kill by the local player: amount 5, the killer's slot twice.

Phases: units, projectiles, players (wherever a unit takes damage or dies).

The presentation counts the amount in the disc activity meter, which picks the music, when either player is the local player.

### SIM_EVENT_FORCES_DESTROYED

Value 13.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The player whose side has lost its last unit. |

Flags: none.

Recorded when a unit dies and it was the last unit of its player, in a game of the team-less multiplayer type (game type 2).

Phases: units, projectiles, players, and outside a step (match shutdown), wherever a unit dies.

The presentation prints the message that the side's forces are destroyed, in the side's name, with one of three texts chosen from the view random stream.

### SIM_EVENT_SCORE_FLASH

Value 14.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The killer's slot. Never 10 as recorded. |
| 1 | player index | The player who lost the unit. |

Flags: none.

Recorded when a unit dies and the death is credited to an attacker that is another player.

Phases: units, projectiles, players, wherever a unit dies.

The presentation flashes the kill and the loss on the score panel, when the player's options show the score panel.

### SIM_EVENT_SELECTION_ORDERS_DIRTY

Value 15.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit. |

Flags: none.

Recorded when what a unit's orders panel shows changes: when a unit's activation, or its state bit 2, changes, when a build, repair, reclaim, resurrect or capture order starts, ends or is terminated, and when an order of the unit is replaced.

Phases: units, players (state changes made by the economy and the AI).

The presentation redraws the orders panel if the unit is selected and belongs to the local player.

### SIM_EVENT_BUILD_MENU_UPDATE

Value 16.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit. |

Flags: none.

Recorded when the build counts of a factory or builder change: when a build order is queued, finishes or is cancelled, when construction of a unit ends, and when a unit of a stockpiling weapon fires.

Phases: units, net (a build finished by a network packet), outside a step (an interface action).

The presentation writes the queued build counts into the build menu if the unit is the one it shows.

### SIM_EVENT_DESELECT_IF_INELIGIBLE

Value 17.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit. |

Flags: none.

Recorded when a unit is attached to or detached from a transporter, after its state changed.

Phases: units, net.

The presentation removes the unit from the selection when it can no longer be selected (unfinished, in transit, or carried by a transport that may not carry it).

### SIM_EVENT_GUI_CHANGED_IF_SCREEN

Value 18.

| Argument | Type | Meaning |
|---|---|---|
| 0 | screen number | A number of the screens table. |

Flags: none.

Recorded when construction of a unit is finished for a player who is human or computer-controlled and the unit is the one the player has focus on (screen 0, the build menu).

Phases: units, net, outside a step.

The presentation marks the GUI as changed when that screen is on top.

### SIM_EVENT_FOCUS_COMMANDER

Value 19.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The force flag. Always 1. |

Flags: none.

Recorded when the local player's commander is created anew after a defeat countdown, in a match whose commander-death setting is 2, after the line of sight is recalculated.

Phases: players.

The presentation centres the camera on the commander of the viewed player and, with the flag set, selects it and closes the open screens.

### SIM_EVENT_POP_TO_GAME_LAYOUT

Value 20.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The force flag. Always 1. |

Flags: none.

Recorded when a commander of a human or computer-controlled player is killed in a match whose commander-death setting is on, just before that player's units are killed.

Phases: units, projectiles, players, wherever a unit is killed; and outside a step (match shutdown).

The presentation closes the screens above the game screen, unless a dialog is open and the flag is 0.

### SIM_EVENT_PLAYER_TOOK_LEAD

Value 21.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The player who has taken the lead. |
| 1 | integer | The kills that put it there: the commander kills when the commander-death setting is 2, else the kills. |

Flags: none.

Recorded when a kill moves a player to rank 0 on the score list, in a team-less or team multiplayer game (game types 2 and 3).

Phases: units, projectiles, players, wherever a unit dies.

The presentation prints the message that the player has taken the lead, in the player's name and with the kills.

### SIM_EVENT_PLAYER_LEFT

Value 22.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The player's network id. |

Flags: none.

Recorded when a unit dies and it was the last unit of its player, in a game of team multiplayer type (game type 3).

Phases: units, projectiles, players, wherever a unit dies.

The presentation prints that the player left the game, in the player's name, with one of eight texts chosen from the view random stream, unless the local player was rejected or no slot has the id.

### SIM_EVENT_BUILD_QUEUE_SOUND

Value 23.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The player of the unit. |
| 1 | integer | The change in the build count. Always 1 as recorded. |

Flags: none.

Recorded when a computer-controlled player queues a build order on an idle factory.

Phases: players (the AI of the players phase).

The presentation plays the add sound of the build menu when the player is the one shown, and the subtract sound when the change is not positive.

### SIM_EVENT_WATCH_PROMPT

Value 25.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The defeated player. |

Flags: none.

Recorded when a defeated player in a team multiplayer game (game type 3) who is not rejected may keep watching, is the local player and hosts no AI player that is still alive.

Phases: players.

The presentation asks the local player whether to keep watching, when the player is the local player.

### SIM_EVENT_WATCH_MODE_NOTICE

Value 26.

| Argument | Type | Meaning |
|---|---|---|
| 0 | player index | The player placed in watch mode. |

Flags: none.

Recorded when a defeated player in a team multiplayer game (game type 3) is the local player and hosts AI players that are still alive, and there are combat players left.

Phases: players.

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

Recorded in two cases. When a unit leaves a corpse and the wreck is placed as a feature (period 15, lifetime 900). When an explosion effect is added on land, above sea level (period 7, lifetime 15).

Phases: units, projectiles (wherever an explosion effect is added or a unit leaves a corpse).

The presentation starts a smoke plume. The smoke picks its animation frames from the view random stream.

### SIM_EVENT_WHITE_SMOKE

Value 28.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | Where the puff is. |
| 3 | integer | The particle list: 9, or 5 for a burning feature. |

Flags: none.

Recorded when a projectile with an end smoke property detonates, when a projectile ends its flight without exploding, once in each tick a projectile with a smoke trail is past its next smoke time (the trail), once in each tick a burning feature is due to smoke (list 5), and when a unit's script emits a white smoke effect from a piece.

Phases: units, projectiles, features.

The presentation puffs white smoke at the position.

### SIM_EVENT_BLACK_SMOKE

Value 29.

| Argument | Type | Meaning |
|---|---|---|
| 0 to 2 | position | Where the puff is. |
| 3 | integer | The particle list. Always 9. |

Flags: none.

Recorded when a unit's script emits a black smoke effect from a piece.

Phases: units.

The presentation puffs black smoke at the position.

### SIM_EVENT_SOUND_AT

Value 30.

| Argument | Type | Meaning |
|---|---|---|
| 0 | integer | The sound index of the game. |
| 1 to 3 | position | Where the sound is. |
| 4 | unsigned | The set of players who hear it: bit n is set for player n. |

Flags: none.

Recorded when a weapon fires (the weapon's sound), when a projectile hits (the weapon's hit sound) or lands in water (its water sound), and when a projectile of a burst weapon with a sound trigger spawns the next shot.

The set of players is decided by the logic when it records: 0 if the position is not on the map, else each player for whom the position is explored (when the map shows explored areas) or in sight (otherwise).

Phases: units (a weapon firing), projectiles (hits and burst spawns).

The presentation plays the sound at the position, when the sound settings allow it and the local player is in the set. With 3D sound off, a sound off screen plays quieter.

### SIM_EVENT_SOUND_AT_NAME

Value 31.

| Argument | Type | Meaning |
|---|---|---|
| 0 | sound name number | A number of the sound names table. |
| 1 to 3 | position | Where the sound is. |
| 4 | unsigned | The set of players who hear it, decided as for `SIM_EVENT_SOUND_AT`. |

Flags: none.

Recorded when a feature starts burning (sound name 1, at the feature's cell).

Phases: features, projectiles, and units (a feature set alight by a weapon), and outside a step (a feature set alight at match load).

The presentation plays the named sound at the position with the checks of `SIM_EVENT_SOUND_AT`.

### SIM_EVENT_REMOVE_UNIT_SPEECH

Value 32.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unit index | The unit that died. |

Flags: none.

Recorded when a unit dies, after its orders are deleted.

Phases: units, projectiles, players, wherever a unit dies; and outside a step (match shutdown).

The presentation drops the unit's pending lines from the speech queue.

### SIM_EVENT_MESSAGE

Value 33.

| Argument | Type | Meaning |
|---|---|---|
| 0 | message text number | A number of the message texts table. |
| 1 | integer | The value: for a toggle, 0 for off and not 0 for on; for a threshold, the number. |

Flags: none.

Recorded when a console command sets sharing: a toggle of sharing metal, energy, mapping, line of sight or radar (texts 0 to 4, the value is the new state), or a threshold of metal or energy (texts 5 and 6, the value is the argument of the command).

Phases: outside a step (console commands run between ticks).

The presentation puts the line in the message log: the text with ON or OFF for a toggle, or with the number.

### SIM_EVENT_RADAR_UNITS

Value 34.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unsigned | The byte offset of the snapshot in the radar arena. |

Flags: none.

Recorded for the viewed player's radar. The snapshot is 8 bytes (the unit count and the projectile count, u32 each), then 48 bytes for each unit the radar shows (colour, damage and range flags, radar, sonar and jammer distances, weapon coverage for units with interceptors), then 12 bytes for each projectile the radar shows (x, y, kind, colour). The units shown and the projectiles shown are decided by the logic when it records.

It is recorded once for each pass of the players phase over the viewed player, and when the line of sight is recalculated.

Phases: players, and outside a step (a line of sight recalculation by a console command or at match load).

The presentation draws the radar picture from the snapshot.

### SIM_EVENT_RADAR_MAPPED

Value 35.

| Argument | Type | Meaning |
|---|---|---|
| 0 | unsigned | The byte offset of the snapshot in the radar arena. |

Flags: none.

Recorded only when the map's explored or visible cells changed since the last snapshot (the change flag is then cleared). The snapshot is 12 bytes (width, height and cell count of the half-resolution map, u32 each) then one byte for each half-resolution map cell: hidden, explored, or in sight now, for the viewed player.

Phases: players (after the viewed player's sensors are updated), and outside a step (a line of sight recalculation).

The presentation marks the cells on the radar picture as explored or hidden.

## Tables

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
