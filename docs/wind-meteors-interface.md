# The wind_meteors interface

Wind and meteors sit behind `interfaces/wind_meteors.h` (version 3.1), the second module on the scheduler after
economy (`docs/economy-interface.md`). 3.1c's code is the default implementation: the wind pass in
`src/game/game_state.cpp` (`UpdateWindOnStream`) and the meteor schedule in `src/map/meteors.cpp`. This page is the
inventory the interface was cut from (#964):
- what others did to wind_meteors' state in 3.1c, and what each became;
- what wind_meteors did to others, and what each became;
- what it reads, and what others read of it;
- what the trace hashes, and what it only dumps;
- the rule for events published outside the tick, which every module follows.

## 1. Its place in the model

The model is economy's (`docs/economy-interface.md`, section 1): a module reads others as of their last run, changes
them only by publishing plain-data events to its outbox, and consumes its inbox before every run. wind_meteors' place
in it:
- **Runs:** `UpdateWind(stream)` in the wind phase (`SchedWindPhase`, on the main stream) and once at match load (on
  the load stream, from `LoadBattleAssets`), and `UpdateMeteors` in the meteors phase (`SchedMeteorsPhase`). The
  scheduler delivers wind_meteors' inbox before each run and at every phase boundary.
- **Queued events in:** ENABLE, DISABLE, PARAMS, START, LIMITS and RESET, published by the engine to its own outbox (`SCHED_ENGINE`).
  wind_meteors' handler (`src/seams/wind_meteors_world.cpp`) passes each to `Apply`.
- **Effect out:** SPAWN, published to wind_meteors' outbox through the service `Emit`, which the host refuses unless SPAWN is in the entry's `publishes` table at 36 bytes and its weapon slot is in the table of weapons. Weapons subscribes;
  its handler is an adapter onto `weapons.SpawnProjectile`. The route is queued.
- **Query:** `LoadMeteorDefaults`, which any caller may call (section 4).
- **Set-up, save and load:** `InitMeteors`, `EmptyShutdownPreCleanup`, `SaveMeteors` and `LoadMeteors`, called by the
  engine outside the tick.
- **No synchronous slot.** Nothing in 3.1c asks wind_meteors for an answer inside the tick, and wind_meteors asks no
  one before it publishes: 3.1c drops `SpawnProjectile`'s result. #952's rule (a query needs a delivery first only
  when an event that can still be pending writes what the query reads) needs no delivery here either (section 4).

The event kinds are 64 to 67 and the effect 80: the router matches rows on (outbox, kind), and the engine's outbox
carries other modules' events too, so kinds are unique across interfaces (economy 0 to 31, features 32 to 63).

## 2. Events into wind_meteors

| 3.1c site | 3.1c call | Event |
|---|---|---|
| the mission set-up (`Mission::LoadMission`), a map with a `MeteorWeapon` | `EnableMeteors` | ENABLE |
| the mission set-up, a map with none | `DisableMeteors` | DISABLE |
| the mission set-up, after either | `SetMeteorParams` with the map's figures or the defaults | PARAMS, with the figures by value |
| the `Meteor` console command with an argument (`CmdMeteor`) | `EnableMeteors` or `DisableMeteors` | ENABLE or DISABLE |
| the `Meteor` console command with none | `StartMeteorShower` | START |
| the map loader (`LoadTntMap`), where it set the wind limits from the mission and the TNT data | writes of `windSpeedMin` and `windSpeedMax` | LIMITS, with the two limits by value |
| `LoadBattleAssets`, before the load-stream wind pass | writes of `windSpeedDenominator` (5000) and `windCounter` (0) | RESET |

The console command's argument parsing moved out of the module into the engine (`src/game/console_commands.cpp`):
the module no longer imports the command-argument services. PARAMS carries 3.1c's `MeteorParams` as the plain struct
`MeteorShowerParams`, the weapon by name; `InitMeteors` looks it up, as 3.1c did.

Every event is published outside the tick (section 6), so each is consumed before the publishing statement returns,
at 3.1c's point.

## 3. Effects out of wind_meteors

| 3.1c call | Effect | Subscriber's handler |
|---|---|---|
| `SpawnProjectile(meteorWeapon, &pos, &vel, 1)` in `UpdateMeteors`, once for each meteor that falls | SPAWN, queued | `WeaponsOnMeteorSpawn` (`src/seams/wind_meteors_world.cpp`), onto `weapons.SpawnProjectile` |

SPAWN carries the weapon as its slot in the match's table of weapons, not as a pointer. `InitMeteors` falls back to
slot 0 when the named weapon is missing or not a meteor weapon, so the slot is always valid.

**Why queued is the same.** `SpawnProjectile` draws no random number. It takes a projectile slot (while fewer than 300
are in flight), records the launch sound, and in a network game with the flag set announces the launch. Queued, the
meteors of a tick are spawned at the phase boundary right after `UpdateMeteors`, before the meteors phase's trace row,
in the order they fell; at most one falls in a tick (a run drops one meteor at most, whatever the interval). No one reads the
projectile table, the sound log or the network queue between the two points, so the state, the sounds and the
packets come out as in 3.1c. Section 8 has the gate's verdict.

`weapons.FindWeaponByName`, wind_meteors' one query of another subsystem, stays a direct query, made by `InitMeteors`
at match start.

**Payloads.** Every event and effect starts with `WindMeteorsEventHeader`, which holds `kind` alone. Each kind has the payload of `interfaces/wind_meteors.h` (version 3.1) that starts with it, or is the header alone. `Apply(event, size)` takes the payload's size and ignores an event smaller than its kind's payload (it returns nothing); `Emit(effect, size)` publishes an effect and records it.

| Kind | Payload | Fields after the header |
|---|---|---|
| ENABLE 64, DISABLE 65, START 67, RESET 69 | the header alone | none |
| LIMITS 68 | `WindMeteorsLimits` | `min` and `max`, the wind speed limits of the map |
| PARAMS 66 | `WindMeteorsParams` | `params`: the `MeteorShowerParams` the event sets |
| SPAWN 80 | `WindMeteorsSpawn` | `weapon` (the meteor weapon's slot), `pos[3]` (the start, world units), `vel[3]` (per tick, world units), `announce` (1 when announced to the other players of a network game) |

## 4. Views and queries

**What wind_meteors reads:** the tick, the map's width and height, and the wind limits of the `Game` record
(`windSpeedMin`, `windSpeedMax`, `windSpeedDenominator`), which the set-up writes. No other subsystem writes any of
them inside the tick (`docs/dependencies.md`, "wind_meteors": 20 read-only rows, no ordered or immediate one).

**What others read of wind_meteors:**
- **The wind, a view in the state block, hashed:** the wind fields of the `Game` record (`windSpeed`,
  `windDirection`, `windX`, `windY`, `windZ`, `windStrength`, `windEnabled`, `windCounter`). Units reads the speed,
  direction and flag (wind generators), weapons the components (projectile drift), features `windZ`, ai the limits,
  and economy the strength through its world view. Only wind_meteors writes them: in the wind phase,
  and at load through LIMITS (the map loader) and RESET (`LoadBattleAssets`), both consumed before wind_meteors'
  load-stream run. `tools/privatestate.py --check` judges that from the access log, which covers the tick's windows
  and the load and save windows. `windY` is never written and stays 0.
- **The meteor schedule:** read by no one else.
- **Query:** `LoadMeteorDefaults` fills the caller's `MeteorShowerParams` from the default meteor data file. It writes
  no meteor state, and the mission set-up uses the answer in the next statement, so it is a query, not an event. It
  reads only the data file, which no event writes, so the engine delivers nothing before it (#952's rule); it is also
  called only outside the tick, when no inbox holds an event. `weapons.FindWeaponByName` (section 3) is the same case:
  it reads the weapon table, which no event writes, and runs at match start.

**As of its last run.** Every read of the wind falls in a later phase than the wind pass that wrote it: the
`carried` rows of `data/phase_dependencies.csv` (units and weapons, wind to the next tick's units and projectiles
phases). The pending inbox can hold only ENABLE, DISABLE, PARAMS and START, which write only the meteor schedule, so
no read of the wind depends on an undelivered event; LIMITS and RESET are published at load only, outside the tick.

**The immediate rows of `docs/schedule.md`.** There are none with wind_meteors as writer or reader.

**One wind slot is enough.** 3.1c's `UpdateWind` is `UpdateWindOnStream` on the main stream; the load-time pass is
the same function on the load stream. `UpdateWind(stream)` covers both: the engine names the stream.

## 5. The state and the trace

**The meteor schedule stays where it is.** The thirteen meteor globals (`g_meteor*` of `SimGlobals`) stay in the
state block and stay hashed (section 9 of the trace). They are wind_meteors' own: `tools/privatestate.py --check`
fails when any other subsystem reads or writes one, and `tools/seamlink.py --pilot-seams` fails when anything but
the engine calls a writing slot or a query slot writes state.

They are not moved into a region sized through the interface, as economy's accounts were, for two reasons:
- **The recordings.** The replay recorder can't record a module that takes a region at `Init` (#698), which is why
  economy and pathfinding have none. wind_meteors is the only WASM module with recordings; a region would cost them,
  and with them the replay and inject runs in CI and the replay half of `tools/oraclegate.py`.
- **The reference.** Taking the globals out of the hash needs a new tag and regenerated traces, for words only
  wind_meteors reads.

Moving them is left until #698 lands: `RegionSize`, `Init`, `Shutdown` and `WriteState` then join the interface, as
economy's did.

**What wind_meteors restores at load.** The meteor schedule, through `LoadMeteors` in a saved game and `InitMeteors` at match start. The wind limits and the wind's divisor and timer, through LIMITS and RESET at match load (a saved game goes through the same `LoadBattleAssets`); both are consumed at once, so each write falls where 3.1c made it.

**What the trace hashes** (`bt-trace 5`, `docs/trace-format.md`): the wind fields and the meteor schedule, as before;
the projectiles the meteors become, as before. Not the events: section 37 records every event wind_meteors consumes
and every effect it publishes, in order, in the dump only (`docs/trace-format.md`, section 8.24; the reference has no
such records).

## 6. Events published outside the tick

**The rule.** An event published outside a tick (`SchedPublish` while no tick is running: set-up, loading, a saved
game, a console command between ticks) is delivered at once: the subscriber's handler runs before `SchedPublish`
returns. Inside a tick, an event waits for the subscriber's next run, synchronous slot or phase boundary. Every module
follows the same rule; the scheduler, not the module, decides.

**Why at once.** Outside the tick nothing else runs, so delivering at once changes no order: the effect lands at
3.1c's statement. Holding the event until the next delivery would move its effect into the tick, to the first phase
boundary (after the units phase), and whatever runs before that boundary would see the old state.

**The experiment.** A build that held wind_meteors' events published outside the tick, instead of delivering them,
was compared with this one (native modules, `tools/tracediff.py`):
- **Held from the mission set-up on:** meteors-hailstorm diverges at tick 1, phase units, `meteors.weapon`
  (`weapondef:226` against `weapondef:0`). `InitMeteors` runs in `LoadBattleAssets`, before the PARAMS that names the
  weapon has been delivered, so it falls back to the first weapon. The set-up needs delivery at once.
- **Held only in a running match** (the console command): console-features is equal, and so are two scratch
  scenarios that type `Meteor` on many ticks, alone and next to the feature cheats on the same tick. A held START is
  delivered at the boundary after the units phase, and in this tree nothing in the net or units phases draws from the
  main stream (`rng.crtHoldrand`, a dump per phase): units, orders, weapons and the unit scripts draw from the
  Park-Miller generator, and only the players, features, wind and meteors phases and the network code draw the main
  stream. So the four draws keep their place in the stream either way, and the traces can't tell the rules apart here.

So the rule stands on the set-up, where holding diverges, and on keeping one rule for every module: a module whose
console or set-up event is read in the units phase would diverge under the held rule where wind_meteors does not.

## 7. What stays inside the default

The wind's period and range, the shower timing and how a meteor's start is spread around the centre are the
default's, not the interface's: another implementation receives the same events and may decide differently, at the
cost of conformity. So are the four draws of a shower's centre and the two of each meteor, and the draws of the wind
pass (one from the stream it is given and one or two from the Park-Miller generator, each time the wind changes).

## 8. What the gate decided

Every event is queued, as the model says, and the gate judged it: no route had to become synchronous.

**SPAWN is queued, and equal.** The tier (`reference.py --check --tier --both-modes`) is equal in all 38 scenarios in
both modes, sounds, UI logs, event digests, frames and the saved game included; it holds meteors-hailstorm,
meteors-earthquake, hailstorm-los-off and console-features. The projectile of a meteor lands in the same slot with the
same fields, at the phase boundary before the meteors row.

**ENABLE, DISABLE, PARAMS and START are queued, and delivered at once** by the outside-tick rule (section 6), since
the engine publishes them only outside the tick.

**Inbox depth.** wind_meteors' inbox never holds an event inside the tick, by construction: every event into it is
published outside the tick and delivered before `SchedPublish` returns. Weapons' inbox holds at most one SPAWN, from the
meteors run to the boundary after it: a run of `UpdateMeteors` drops one meteor at most, even when a dense shower's
interval rounds to 0 ticks (meteors-hailstorm drops one every tick).

**The negative tests** are local, not in CI, because CI plays no games (the replay and inject runs in CI judge the module
by its imports, which now hold each SPAWN's fields). Each changed the default implementation and was compared with
this build by `tools/tracediff.py`, in native and WASM modules:
- **A dropped spawn** (meteors-hailstorm, the meteor of tick 20 not published, its draws kept): first divergence at
  tick 20, phase meteors, `projectiles[11].type` (`weapondef:226` against none), with the projectile's 33 other fields
  and section 37's `windMeteors.count`.
- **The wind off by one** (mission-build-unit, `windSpeed` plus 1 after it is drawn): first divergence at tick 1,
  phase wind, `wind.windZ` (`0xfffff348` against `0xfffff346`), with `wind.windSpeed` (`0x671` against `0x672`) and
  `wind.windStrength`. Units read the speed and weapons the components in later phases, so the wind is hashed where
  it is written.
