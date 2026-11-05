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

## The recording wrappers

The reference build writes this log from hooks in the harness (`src/harness/harness.cpp`, declared in `src/harness/harness_events.h`, on the `reference` branch). A hook is a `HarnessEvent*` call placed at the entry of the 3.1c presentation function, before any check inside it, so the log has the call the logic made and not what the presentation did with it. The moddable build records the same events through the wrappers of `src/state/sim_effects.h` (and `RecordUnitSpeech` in `src/state/sim_speech.h`, `RecordRadarUnits` and `RecordRadarMapped` in `src/map/radar_capture.cpp`), at the place in the logic where the call was. The table has one row for each of the 31 recording wrappers of `src/state/sim_effects.h` (the overloads of one wrapper share a row). Hook files are on `origin/reference`.

Common to every row: `HarnessEventWords`, `HarnessEventPtrs`, `HarnessEventUnit`, `HarnessEventSoundAt` and `HarnessEventRadar` write nothing unless the run is in test mode, the log is open and the suppression count (`HarnessEventSuppress`) is 0. The reference wraps a nested or presentation-only call in `HarnessEventSuppress(1)` and `(0)` so that it does not count: `PlaySoundAt` called from `PlaySoundAtByName`, `PlaySoundAt` in `HandleNetPackets` (`src/network/net_game_453d40.cpp`), `EmitWhiteSmoke` in `DrawExplodedPiece` (`src/weapons/explosions.cpp`), `QueueUnitSpeech` in `src/ingame/selection.cpp` and in `ReactToAttack`. The moddable build has no recording at those places. A position or a list argument is written as the unsigned 32-bit value of the stored int (a `short` list index is sign-extended first, in the hook and in the wrapper alike). A position is the 3 ints at the pointer the 3.1c call receives (the fixed-point `value`, as stored); the wrapper's `Vec3` holds the same 3 ints.

| Wrapper | Kind | 3.1c call | Reference hook | Arguments |
| --- | --- | --- | --- | --- |
| `RecordSound` | `SIM_EVENT_SOUND` | `PlaySoundByName("Victory Condition", 0)`, called from the ten victory-condition functions (`VictoryKillEnemyCommander::OnUnitDied` 0x48ea40 through `VictoryUnitTypePassesZ::VisitUnit` 0x48f4c0) | `src/game/victory.cpp`: `HarnessEventWords(HEV_SOUND, 0, 2, 0, 0)` in each of those ten functions, on the line before `PlaySoundByName` | The hook writes the constants 0 (sound name number of "Victory Condition") and 0 (the flag, the call's second argument); it does not read the call's arguments. The wrapper takes `SIM_SOUND_VICTORY_CONDITION` and the same literal 0. Ten call sites on each side. |
| `RecordNanoParticles` | `SIM_EVENT_NANO_PARTICLES` | `EmitNanoParticles` 0x4720d0 | `src/graphics/particles.cpp`, `EmitNanoParticles`: `HarnessEventPtrs(HEV_NANO_PARTICLES, p, 3, posB, 6, 1, index)`, first statement | The 3 ints at the first pointer (origin), the 6 ints at the second (low and high corner), then the list. The wrapper takes the origin, then the corners as two `Vec3`s, a `Box` or a `Vec3[2]`, and the list. Ten callers in the reference, ten call sites in moddable (`unit_orders.cpp` 7, `vtol_orders.cpp` 3). |
| `RecordReverseNanoParticles` | `SIM_EVENT_REVERSE_NANO_PARTICLES` | `EmitReverseNanoParticles` 0x472200 | `src/graphics/particles.cpp`, `EmitReverseNanoParticles`: `HarnessEventPtrs(HEV_REVERSE_NANO_PARTICLES, targetBounds, 6, origin, 3, 1, index)`, first statement | The 6 ints of the source bounds, then the 3 of the origin, then the list. Same order in the wrapper (corners or `Box`, origin, list). Seven callers in the reference, seven call sites in moddable. |
| `RecordTeleportParticles` | `SIM_EVENT_TELEPORT_PARTICLES` | `EmitTeleportParticles` 0x471fd0, called from `TeleportOrder` 0x406aa0 | `src/graphics/particles.cpp`, `EmitTeleportParticles`: `HarnessEventPtrs(HEV_TELEPORT_PARTICLES, posA, 3, posB, 3, 2, lifetime, index)`, first statement | The 3 ints at each pointer (unit position, then the computed end), then the lifetime and the list. The wrapper takes the same two positions, 30 and 5 as the call does. One caller on each side. |
| `RecordThrustParticles` | `SIM_EVENT_THRUST_PARTICLES` | `EmitThrustParticles` 0x472330, called from `UnitScript::EmitSfx` 0x480eb0 | `src/graphics/particles.cpp`, `EmitThrustParticles`: `HarnessEventPtrs(HEV_THRUST_PARTICLES, posA, 3, posB, 3, 3, period, lifetime, index)`, first statement | Two positions (3 ints each), the period, the lifetime, the list. The wrapper has the same order. Two callers (effects 0 and 1) on each side. |
| `RecordWakeParticles` | `SIM_EVENT_WAKE_PARTICLES` | `EmitWakeParticles` 0x472430, called from `UnitScript::EmitSfx` | `src/graphics/particles.cpp`, `EmitWakeParticles`: `HarnessEventPtrs(HEV_WAKE_PARTICLES, posA, 3, posB, 3, 2, period, index)`, first statement | Two positions, the period, the list. The wrapper has the same order; effects 4 and 5 pass the two positions swapped, in the call and in the wrapper alike. Four callers on each side. |
| `RecordBubbles` | `SIM_EVENT_BUBBLES` | `EmitBubbles` 0x472530, called from `UnitScript::EmitSfx` (effect 0x103) | `src/graphics/particles.cpp`, `EmitBubbles`: `HarnessEventPtrs(HEV_BUBBLES, posA, 3, posB, 3, 2, period, index)`, first statement | As for the wake. The end position is the start with y set to the sea level shifted left by 16, computed by the caller before the hook. One caller on each side. |
| `RecordWeaponSmoke` | `SIM_EVENT_WEAPON_SMOKE` | `EmitWeaponSmoke` 0x4729d0, called from `FireLineOfSightProjectile`, `FireVLaunchProjectile` and `FireBallisticProjectile` | `src/graphics/particles_472630.cpp`, `EmitWeaponSmoke`: `HarnessEventPtrs(HEV_WEAPON_SMOKE, p, 3, 0, 0, 1, index)`, first statement | The 3 ints at the pointer, then the list. Three callers on each side. |
| `RecordTimedSubParticles` | `SIM_EVENT_TIMED_SUB_PARTICLES` | `EmitTimedSubParticles` 0x472c50, called from `PlaceFeature` 0x423c50 | `src/graphics/particles_472630.cpp`, `EmitTimedSubParticles`: `HarnessEventPtrs(HEV_TIMED_SUB_PARTICLES, p, 3, 0, 0, 1, index)`, first statement | The 3 ints at the pointer, then the list. Two callers (the two branches of `PlaceFeature`) on each side. |
| `RecordScreenShake` | `SIM_EVENT_SCREEN_SHAKE` | `AccumulateScreenShake` 0x41c640, called from `DetonateProjectile` 0x499eb0 | `src/ingame/control_panel.cpp`, `AccumulateScreenShake`: `HarnessEventWords(HEV_SCREEN_SHAKE, 0, 3, dx, dy, value)`, first statement, before the `uiOptionFlags` and `flags_1434e` tests | The call's three arguments as they are. The wrapper receives `shakeMagnitude`, `shakeMagnitude`, `shakeDuration`, the call's arguments. One caller on each side. |
| `RecordCdActivity` | `SIM_EVENT_CD_ACTIVITY` | `AddCdActivitySample(amount)`, in `ApplyUnitDamage` 0x489ce0 (amount 1) and `ApplyUnitDeath` 0x4866d0 (amount 5) | `src/units/unit_targets.cpp` (`ApplyUnitDamage`) and `src/units/units_485010.cpp` (`ApplyUnitDeath`): `HarnessEventWords(HEV_CD_ACTIVITY, 0, 3, ...)` on the line before `AddCdActivitySample`, inside the local-player test | The hook is on the 3.1c call `AddCdActivitySample`, not on a presentation function with these arguments: the amount is the call's argument, the two players are values the hook takes from the logic at that line (the attacker's and the unit's player index for a hit, the last attacker's slot twice for a kill). The wrapper takes the same three values under the same local-player test. Two call sites on each side. |
| `RecordBuildQueueSound` | `SIM_EVENT_BUILD_QUEUE_SOUND` | No presentation function: the 3.1c code at `BuildTimer::OnTimer` 0x4086d0 and `BuildTimer::TryIssueIdleFactoryBuildOrder` 0x408620 where the AI queues a build order with `QueueBuildOrder(..., 1)` | `src/ai/ai_player.cpp` (`BuildTimer::OnTimer`) and `src/ai/ai_player_408620.cpp` (`TryIssueIdleFactoryBuildOrder`): `HarnessEventWords(HEV_BUILD_QUEUE_SOUND, 0, 2, player, 1)`, in the `if (id)` or `if (idx != 0)` branch, on the line before `QueueBuildOrder` | The player is `u->playerIndex` in the first and the byte at offset 0xff of the unit in the second (the offset of `Unit::playerIndex`, `src/state/layout_asserts.cpp`), the count the literal 1 (the call's last argument). The wrapper takes `unit->playerIndex` and 1. Order differs: the hook is before `QueueBuildOrder`, the wrapper after it (`src/ai/ai_player.cpp`). Whether `QueueBuildOrder` records an event of its own, which would change the order of two records, is not established here. |
| `RecordWatchPrompt` | `SIM_EVENT_WATCH_PROMPT` | `LoadGuiLayer(&g_game->gui, "YESORNO.GUI", 0x900)` in `UpdatePlayers` 0x464f80 (the "You're out! Continue Watching?" dialog) | `src/game/players_464290.cpp`, `UpdatePlayers`: `HarnessEventWords(HEV_WATCH_PROMPT, 0, 1, bl)`, in the `CountActiveAIPlayers() == 0` branch before `LoadGuiLayer` | `bl`, the player index in that loop. The wrapper takes `bl` in the same branch (`src/game/players.cpp`, `UpdatePlayersOnStream`). One call site on each side. |
| `RecordWatchModeNotice` | `SIM_EVENT_WATCH_MODE_NOTICE` | `OpenMessageBox(&g_game->gui, "You are placed in watch mode because ...")` in `UpdatePlayers` 0x464f80 | `src/game/players_464290.cpp`, `UpdatePlayers`: `HarnessEventWords(HEV_WATCH_MODE_NOTICE, 0, 1, bl)`, after the `CountCombatPlayers() <= 0` test and before `OpenMessageBox` | `bl`, as above. One call site on each side. |
| `RecordForcesDestroyed` | `SIM_EVENT_FORCES_DESTROYED` | `AnnounceForcesDestroyed` 0x47bd70, called from `ApplyUnitDeath` 0x4866d0 | `src/frontend/skirmish_menu.cpp`, `AnnounceForcesDestroyed`: `HarnessEventWords(HEV_FORCES_DESTROYED, 0, 1, player->index)`, first statement | The player's index. The wrapper takes `unit->player->index` under the same game type test (2) and `unitCount == 0`. One caller on each side. |
| `RecordPlayerLeft` | `SIM_EVENT_PLAYER_LEFT` | `AnnouncePlayerLeft` 0x450380, called from `ApplyUnitDeath` 0x4866d0 | `src/network/net_game.cpp`, `AnnouncePlayerLeft`: `HarnessEventWords(HEV_PLAYER_LEFT, 0, 1, id)`, first statement, before the `rejectReason` test | The id argument (the player's network id). The wrapper takes `unit->player->id` under the game type test (3) and `unitCount == 0`. The reference has one caller of the function (`ApplyUnitDeath`); the declarations in `src/gui/gui.cpp` and `src/network/packets.cpp` have no call. |
| `RecordPlayerTookLead` | `SIM_EVENT_PLAYER_TOOK_LEAD` | `AddMessage(text, 2, 0, 10)` with `g_takenLeadFormat`, in `ApplyUnitDeath` 0x4866d0 | `src/units/units_485010.cpp`, `ApplyUnitDeath`: `HarnessEventWords(HEV_PLAYER_TOOK_LEAD, 0, 2, rec->index, kills)`, in the `best == 0` branch after the text is formatted, before `AddMessage` | The ranked player's index, then the kills the format printed: commander kills when `battleCommanderDeath == 2`, else kills. The wrapper takes the same two values from the same expression (`commanderDeath` in moddable). |
| `RecordScoreFlash` | `SIM_EVENT_SCORE_FLASH` | `FlashScorePanelKillLoss(killer, loser)` in `ApplyUnitDeath` 0x4866d0 | `src/units/units_485010.cpp`, `ApplyUnitDeath`: `HarnessEventWords(HEV_SCORE_FLASH, 0, 2, lastAttackerSlot, player->index)`, on the line before the `g_game->b7` test | The last attacker's slot, then the dying unit's player index, the call's arguments. The wrapper records at the same place, so it records whether or not `b7` is set. |
| `RecordSelectionOrdersDirty` | `SIM_EVENT_SELECTION_ORDERS_DIRTY` | `MarkSelectionOrdersDirty` 0x41c110 | `src/ingame/control_panel.cpp`, `MarkSelectionOrdersDirty`: `HarnessEventUnit(HEV_SELECTION_ORDERS_DIRTY, unit)`, first statement | The unit index of the unit argument (the u16 at offset 0xa8), which the wrapper writes as `unit->id`. Fourteen callers in the reference (`unit_orders*.cpp`, `vtol_orders*.cpp`, `unit.cpp`), fourteen call sites in moddable. |
| `RecordBuildMenuUpdate` | `SIM_EVENT_BUILD_MENU_UPDATE` | `UpdateBuildMenuIfFocusUnit` 0x41c150, and the inlined equivalent in `FinishConstruction` 0x41b8d0 | `src/ingame/control_panel.cpp`: `HarnessEventUnit(HEV_BUILD_MENU_UPDATE, unit)` as the first statement of `UpdateBuildMenuIfFocusUnit`, and the same call in `FinishConstruction`, before the `unitIndex == unit->id` test | The unit index, as above. Six callers of the function plus the `FinishConstruction` hook in the reference, seven call sites in moddable (the `FinishConstruction` one is in `src/ingame/control_panel.cpp`). Merged: the reference has two hook places for the one wrapper. |
| `RecordDeselectIfIneligible` | `SIM_EVENT_DESELECT_IF_INELIGIBLE` | `DeselectIfIneligible` 0x48c9b0, called from `ApplyAttachUnit` 0x48ab70 | `src/ingame/selection.cpp`, `DeselectIfIneligible`: `HarnessEventUnit(HEV_DESELECT_IF_INELIGIBLE, unit)`, first statement | The unit index. One caller on each side. |
| `RecordGuiChangedIfScreen` | `SIM_EVENT_GUI_CHANGED_IF_SCREEN` | `IsScreenNamed(&g_game->gui, "BUILDER.GUI")` followed by `MarkChanged`, in `FinishConstruction` 0x41b8d0 | `src/ingame/control_panel.cpp`, `FinishConstruction`: `HarnessEventWords(HEV_GUI_CHANGED_IF_SCREEN, 0, 1, 0)`, before `IsScreenNamed` | The literal 0, the screen number of "BUILDER.GUI" (`docs/sim-events.md`, "Screens"). The wrapper takes `SIM_SCREEN_BUILDER`. The hook is on a pair of calls, not on one function; there is one on each side. |
| `RecordFocusCommander` | `SIM_EVENT_FOCUS_COMMANDER` | `FocusCommander` 0x48d630, called from `UpdatePlayers` 0x464f80 | `src/ingame/selection.cpp`, `FocusCommander`: `HarnessEventWords(HEV_FOCUS_COMMANDER, 0, 1, param_1)`, first statement | The call's argument. The wrapper takes the literal 1, which is what the one caller passes. |
| `RecordPopToGameLayout` | `SIM_EVENT_POP_TO_GAME_LAYOUT` | `PopUntilNamedLayout(1)` in `KillUnit` 0x4864b0 | `src/units/units_485010.cpp`, `KillUnit`: `HarnessEventWords(HEV_POP_TO_GAME_LAYOUT, 0, 1, 1)`, on the line before `PopUntilNamedLayout(1)`, inside the commander-death test | The literal 1, the call's argument. The hook is on `PopUntilNamedLayout`'s call in `KillUnit`, not in `PopUntilNamedLayout`, so other callers of it do not count. One site on each side. |
| `RecordSmoke` | `SIM_EVENT_SMOKE` | `EmitSmoke` 0x472630, called from `CreateUnitCorpse` 0x486360 and `AddExplosionEffect` 0x420a30 | `src/graphics/particles_472630.cpp`, `EmitSmoke`: `HarnessEventPtrs(HEV_SMOKE, pos, 3, 0, 0, 3, emitPeriod, lifetime, index)`, first statement | The position, the emit period, the lifetime, the list. Two callers on each side. |
| `RecordWhiteSmoke` | `SIM_EVENT_WHITE_SMOKE` | `EmitWhiteSmoke` 0x472810, called from `UpdateFeatures` 0x424050, `UnitScript::EmitSfx`, `DetonateProjectile`, `UpdateProjectiles` 0x49b720 (twice) | `src/graphics/particles_472630.cpp`, `EmitWhiteSmoke`: `HarnessEventPtrs(HEV_WHITE_SMOKE, pos, 3, 0, 0, 1, index)`, first statement | The position, the list. Five counted callers in the reference and five call sites in moddable. A sixth reference caller, `DrawExplodedPiece`, is wrapped in `HarnessEventSuppress` and left out of the log (a drawing path). |
| `RecordBlackSmoke` | `SIM_EVENT_BLACK_SMOKE` | `EmitBlackSmoke` 0x4728f0, called from `UnitScript::EmitSfx` (effect 0x102) | `src/graphics/particles_472630.cpp`, `EmitBlackSmoke`: `HarnessEventPtrs(HEV_BLACK_SMOKE, p, 3, 0, 0, 1, index)`, first statement | The position, the list. One caller on each side. |
| `RecordSoundAt` | `SIM_EVENT_SOUND_AT` | `PlaySoundAt` 0x47f300, called from `DetonateProjectile` (twice), `InitProjectile` 0x49c740 and `UpdateProjectiles` 0x49b720 | `src/sound/sound.cpp`, `PlaySoundAt`: `HarnessEventSoundAt(HEV_SOUND_AT, index, pos)`, first statement, before the `g_useWindowsSound` and `0xffff` tests | The sound index (the call's argument), the 3 ints at the position, and the hearing mask: a snapshot the hook computes from state at that moment (`HeardMask` in `src/harness/harness.cpp`). The wrapper stores `SoundHeardMask(at)` from `src/state/sim_sound.cpp`, computed from state when the event is recorded. The two functions have the same steps by reading (cell test, tile of x and of z minus half y, the `mapFlags & 2` choice between the explored map and the visibility mask); they were not run against each other here. Four counted callers on each side; the fifth reference caller (`HandleNetPackets`) is suppressed. |
| `RecordSoundAtName` | `SIM_EVENT_SOUND_AT_NAME` | `PlaySoundAtByName("treeburn", &pos, 0)` 0x47f610, called from `StartFeatureBurning` 0x4233a0 | `src/sound/sound.cpp`, `PlaySoundAtByName`: `HarnessEventSoundAt(HEV_SOUND_AT_NAME, 1, pos)`, first statement; the `PlaySoundAt` inside is suppressed | The hook writes the constant 1 (sound name number of "treeburn") and does not read the name, so it is right for the one caller; the wrapper takes `SIM_SOUND_TREEBURN`. The position and the hearing mask are formed as for `RecordSoundAt`. One caller on each side. |
| `RecordRemoveUnitSpeech` | `SIM_EVENT_REMOVE_UNIT_SPEECH` | `RemoveSpeechOfUnit` 0x47f8c0, called from `ApplyUnitDeath` 0x4866d0 | `src/sound/sound.cpp`, `RemoveSpeechOfUnit`: `HarnessEventUnit(HEV_REMOVE_UNIT_SPEECH, (const void*)id)`, first statement | The function's parameter is the unit pointer (it is compared with the queue entries' `unit` field), so the hook reads the unit index at offset 0xa8 as for the other unit kinds. The wrapper writes `unit->id`. One caller on each side. |
| `RecordMessage` | `SIM_EVENT_MESSAGE` | `AddMessage(buf, 2, 0, 10)` after each of the share commands: `CmdShareMetal` 0x418cd0, `CmdShareEnergy` 0x418d90, `CmdShareMapping` 0x418e50, `ToggleShareLos` 0x418f10, `CmdShareRadar` 0x418fd0, `CmdSetShareMetal` 0x419340, `CmdSetShareEnergy` 0x419400, and the four inlined toggles of `CmdShareAll` 0x419090 | `src/game/share_commands.cpp`: `HarnessEventWords(HEV_MESSAGE, 0, 2, text, value)` on the line before each `AddMessage`, inside the `netGameFlags & 1` test; eleven hooks (seven commands, four in `CmdShareAll`) | The text number is a literal (0 metal, 1 energy, 2 mapping, 3 LOS, 4 radar, 5 and 6 the thresholds). The value is the new state read after the toggle (`shareMetal != 0` or `flags_97_wide & mask`, so 0 or the mask for energy, mapping and radar) or `args->GetIntArg(1, 0)` for the thresholds. The wrapper takes the same expressions. Merged: the reference inlines the toggles into `CmdShareAll` with four hooks, while moddable's `CmdShareAll` calls the four commands, which record once each (the LOS toggle is not among the four, on both sides). |

### Kinds with no wrapper in `sim_effects.h`

- `SIM_EVENT_UNIT_SPEECH` is recorded by `RecordUnitSpeech` (`src/state/sim_speech.h`), not by a wrapper of `sim_effects.h`. The reference hook is `HarnessEventSpeech` (`src/harness/harness.cpp`), called from `QueueUnitSpeech`, `QueueUnitSpeechIfVisible` and `QueueUnitSpeechIfNotVisible` (0x47f780 to 0x47f850) and from `ReactToAttack`. Like the wrapper, it writes nothing unless the unit has flag 0x10000000 and not 0x4000; it maps the text to its number in the speech texts table, 0 for no text. Not part of the 31 rows.
- `SIM_EVENT_RADAR_UNITS` and `SIM_EVENT_RADAR_MAPPED` are recorded by `RecordRadarUnits` and `RecordRadarMapped` (`src/map/radar_capture.cpp`). The reference hooks are `HarnessRadarUnits` at the entry of `DrawRadarUnits` 0x466dc0 and `HarnessRadarMapped` in `UpdateRadarMapped` 0x466c20, after the map-changed flag is cleared. Each hook builds the snapshot in the harness's own buffer from the game state at the hook (`HarnessEventBuffer`) and writes its bytes through `HarnessEventRadar`; the radar arena of moddable does not exist in 3.1c, so the offset the moddable record carries has no counterpart. The log therefore carries the content in place of the offset (see "Format"), and no field is left out of the comparison: every byte of the snapshot is compared. The snapshot fields are all computed from the state at the moment of the call (the unit and projectile lists, the viewed player's explored map and visibility mask), not copied from an argument. The reference snapshot fills the reserved bytes with 0 (for `radar_units`, the buffer is cleared for the whole size before the units are written); that moddable's capture clears them too is not established here and is judged by the logs.

### Fields left out of the comparison

None. Every argument of every kind is written to the log and covered by the digest. Where the reference hook could not read a value from the call (the sound name numbers of `RecordSound` and `RecordSoundAtName`, the screen of `RecordGuiChangedIfScreen`, the text numbers of `RecordMessage`), it writes the constant of the one call site, as the rows say.

## The digest

`reference/events/<scenario>.txt` stores the digest of the log, not the log (the logs of the
scenarios are over a gigabyte). `tools/eventdigest.py` makes the digest from a full log. The
first line is `bt-event-digest 1`, the digest format version. The second line is `log `
followed by the first line of the log (`log bt-events 1`). Every other line covers one run of
records with the same tick and the same phase, taken in the order of the log:

    <tick> <phase> <count> <hash>

| Field | Meaning |
| --- | --- |
| `tick`, `phase` | The tick and the phase of the records, as they appear in the log. A tick and phase that the log has again after other records have their own line, in the order of the log. |
| `count` | The number of records in the run (decimal). |
| `hash` | The 64-bit hash of the run's bytes, as 16 lowercase hexadecimal digits. |

The hash is the one of the traces (`docs/trace-format.md`, section 3: the 64-bit FNV-1a variant
with the offset basis `0xcbf29ce484222325`, whose step on each whole 8-byte word is the word
step), applied once to the run's bytes as one run of bytes, written `HB(bytes)`. The bytes are
the text of the run's records exactly as the log has them: for each record, the whole line
from the tick to the last token, without the line feed, then one line feed (byte `0x0a`).
Nothing is added between records and the tick and phase are part of each record's bytes.

What the digest covers, therefore:

- Every field of every record: the tick, the phase, the kind, the flags and every argument.
- The content of a radar snapshot: the hexadecimal token of a `radar_units` or `radar_mapped`
  record is part of the record's line, so every byte of the snapshot is in the hash.
- A `dropped` line: it is a record of its tick and phase like any other, so it is counted in
  `count` and its text is in the hash. A reference run never drops an event, so a run that has
  one differs from the stored digest in the count and in the hash of that line's phase.
- The order of the records in a run, and the order of the runs.
- The header line, through the second line of the digest.

Two logs have the same digest only if they are equal, up to a collision of the 64-bit hash of
one run.

## What the check compares

For each scenario, the check makes the digest of the run's log and compares it with the stored
digest line by line, including the first two lines. Two digests are equal when they have the
same lines in the same order. The first line that differs is reported with its number, its tick
and phase, and both lines, which hold the counts and the hashes: a different count shows a
missing or an extra event in that phase, and a digest that is longer or shorter than the other
differs at the first line only one of them has.

A scenario with no stored digest is not compared, and is counted in the summary. The check
runs in the tier as it does in the full suite.

## Naming the record

A digest names a tick and a phase, not a record. When a scenario's digest differs, the check
then plays the reference build of the stored traces and the build under test on that scenario
with `tools/tracediff.py A B --scenario S --events`, which compares the two full logs line by
line and prints the number of the first record that differs, its tick and phase, and both
records (or the record only one log has). `tools/tracediff.py --event-logs A B` does the same
for two logs that are already written. A build that has the reference's logs can do the same
with its own tools: the full log of a scenario is what a test-mode run writes.
