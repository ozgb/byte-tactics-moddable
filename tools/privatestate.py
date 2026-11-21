"""Check, over the stored access-log summaries, that the pilots' state is private to them and that pathfinding's written state shows no shadow field.

    uv run tools/accesslog.py run --build-dir build/access <every scenario>
    uv run tools/privatestate.py --store build/access/access    # keeps the reads files, gzipped, in data/access_reads
    uv run tools/privatestate.py --check                        # both checks below, on the stored data
    uv run tools/test_privatestate.py                           # tests, on synthetic summaries

Private. The state of the default pathfinder is one allocation of the state block (the region, allocated by the engine in LoadBattleAssets and set up by PathfindingInit
and reached through `Game::pathfinder`, the item `pathfinder`). tools/accesslog.py names the parts of the allocation as
`Pathfinder::field`, `PathExtra::field`, `Node::field`, `PathCell::field` and `PathPending::field` and the others
`<allocation>.<part>`. --check fails on any row of the stored summaries (data/access/*.edges.csv.gz, the cross, pass and
presentation files of data/access_phases and data/access_presentation, and the reads files) in which a subsystem other than
pathfinding is the writer or the reader of such an item. A writer or reader `-` is no subsystem.

Reads. `<scenario>.reads.csv` (item,reader,count,first_tick, written by lib/accesslog) lists every subsystem that read an item
at all in the windows of a tick, a subsystem's reads of its own writes included, which the edges files leave out
(lib/accesslog/classifier.h). The `presentation` window is not in it.

Shadow fields. The harness dumps sections 16 and 17 of docs/trace-format.md (a diagnostic, not hashed since bt-trace 4) from the records
that the default pathfinder writes (PathfindingWriteState): the
tables Pathfinder, PathfinderSearch and Node of data/trace_schema.json, the cells and dirty words of the grid, the open heap and
the three globals. EXPOSED maps each of those fields to the log item that holds it. --check fails when
  * a field of those tables has no entry in EXPOSED (the written state grew a field this check does not know),
  * an item of EXPOSED has no read by pathfinding in any stored scenario: the writer shows state that the simulation never
    reads, a shadow field. data/shadow_field_allow.csv (item,reason) lists the ones that are 3.1c state the trace has always
    hashed, each with a reason; a row for an item that is read is stale.
Owners. OWNERS is the table of subsystems with private state: pathfinding (the region above, read and written by pathfinding alone), economy, features, targeting, los_radar, map, players, ai, weapons, orders and movement.
Economy's unit accounts live in one allocation, "Economy region" (allocated by the engine in LoadBattleAssets, set up by EconomyInit), whose
parts the log names `<allocation>.<part>` like pathfinding's: no subsystem but economy writes or reads one. The player accounts
(`players[i].econ`, type UnitResources) stay in the state block and are read by others, so a `UnitResources::*` item is private for writes
only: any writer but economy fails, which is what makes every income, spend and request reach them through the scheduler's events. Economy's
other view state, the resource fields of the Player record, is named in the log by the Game union aliases (`unknown_1b8e`, `pad_1df4`,
`unknown_1cd9`) that other fields of the record share, so it can't be told from the rest of the record and is not checked. Economy's
WriteState (section 34) is dumped and never hashed, so no shadow-field check applies to it.
wind_meteors owns no allocation, only named items of the state block. The 13 meteor globals of SimGlobals (g_meteorNextStrikeTime and the others
in METEOR_GLOBALS, named in the log by the global's name) are private for reads and writes: any subsystem but wind_meteors that reads or writes one
fails. The wind fields of the Game record (WIND_FIELDS, named in the log by the bare field name) are its view: others read them, so they are private
for writes only, and any writer but wind_meteors fails. Like every check here it sees the windows of a tick only, so it holds the wind to
wind_meteors inside the tick: at load the engine still writes windCounter and windSpeedDenominator (LoadBattleAssets) and the map loader
the limits windSpeedMin and windSpeedMax (MapSettings), which the log does not cover.

Features' state stays in the state block, hashed and read by others as a view, and is written by features alone, like the player accounts: the
links of the feature animation pool's spots (the 0x800 entries of 0x30 bytes allocated by InitFeatureAnimPool as "FEATURE ANIM DATA", which the log
names `FeatureAnimEntry::field` after the type of an entry: `next` and `prev`), the feature fields of a map cell (`Cell::feature` and `Cell::spot`, the
union with `Cell::offsetX` and `Cell::offsetY`, but not `Cell::flags`, which other subsystems share) and the scan index (`Game::scanIndex`, the item
`scanIndex`), which the owner lists name as exact items (`write_items`). The rest of a spot is not checked: it holds the spot's animation cursors
(`Spot::anim` and `Spot::shadow`), which the GAF helpers of src/graphics/gaf.cpp step and read for features and for drawing, and the log names those
helpers' subsystems (game and presentation) as the writers. The cells are in the log one allocation ("PLOT MEMORY") that it names no field
of, so the Cell items match only once it does; the others match today. Any writer but features fails, which is what makes every placement, removal,
damage and reclaim reach them through the scheduler's events.

Targeting's state stays in the Unit record, hashed and read by others, and is written by targeting alone: the weapon target pair of each of the three
weapon slots of a unit (`UnitWeaponSlot::aimTargetXOrUnitId` and `aimTargetZOrUnitSentinel`), which the log names by the first member of the union of
`Unit` that starts each slot's words: `Unit::current` (slot 0), `Unit::field_20` (slot 1) and `Unit::weapon2Target` (slot 2), each the pair's four bytes.
The other members of a slot (`Unit::field_8`, `Unit::weapon1AimCob`, `Unit::weapon2AimCob`) are not the pair and are shared with the unit's own set-up and
weapons, so they are not listed. `Unit::netDirtyFlags` is not listed either: targeting clears bits of it, but weapons and orders write it too. Any
writer but targeting fails, which is what makes every aim, clear and reset of a weapon's target reach the pair through the scheduler's events.
Like every check here it sees the windows of a tick only, and a saved game loads outside them: no stored summary covers LoadUnit, whose restore
of the pair goes through targeting's LOAD by the same rule (#1010) but is not checked.

Los_radar's state is a region and a view. The region is one allocation, "LosRadar region" (allocated by the engine in LoadBattleAssets), which holds the
unit sight cache (the 12 bytes of the Unit record that stay unused, `Unit::unusedLosCache` and `unusedLosSightFrame`): no subsystem but los_radar writes or reads a part of it. The eyeballs (the members of `Eye`, the count
`count_14277` and `eyes`, the pointer to them) are private for reads and writes as well: nothing but los_radar reads them, and `losRadarState` reports them. The view is written by los_radar alone and read by others: the visibility layer of the map, the allocation "MAPPED MEMORY" that LoadTntMap makes
(`LoadTntMap: GameAllocIgnoreTag MAPPED MEMORY`), each player's explored map, the allocation of InitPlayerSlot (`InitPlayerSlot: operator new unsigned char`),
and the unit index, each player's lists of the enemy units it sees and knows and its hasSpecial flag (the members of `LosUnitIndex` and `LosUnitIndexEntry`, read by ai through the seam).
The cells of the map (the allocation "PLOT MEMORY", which presentation, map and features write too) and the line of sight setting bits of the map flags are not listed. Any writer but los_radar fails, which is what makes every move, removal, reveal and
recalculation of line of sight reach these items through the scheduler's events.

Map's state is written by map alone. The members of the bucket types (`UnitBucket::first` and `SpatialBucket::head`) are private for reads and writes: only map reads them, and `mapState` reports them. Read elsewhere, so private for writes only: the spatial bucket index of a unit (`Unit::spatialBucket`, which orders reads: order_targets.cpp), the pass maps of the movement classes (the allocation of BuildAllPassMaps, `BuildAllPassMaps: operator new unsigned int`), the two results of
the last placement test, `g_lastPlaceHeight` and `g_lastPlaceMetalSum`, and the path lock stamp of a unit's motion (`UnitMotion::pathLockStamp`), map's bookkeeping in movement's record, which
AddUnitToMap and the pass map refreshes write and a saved game reads and restores through map's PathLockStamp and LOAD_PATH_LOCK_STAMP. The bucket array (`cells_1429f`) and the overflow bucket (`overflowBucket`) are not listed: they are allocated and freed outside the tick, and the item that names their allocation is listed once map owns it. The cells of the map (the allocation "PLOT MEMORY", which
presentation, features and los_radar write too) are not listed either, nor are `Unit::cargoNext` (units write it), `Unit::flags`, `Unit::bit2` and `Unit::cobStateFlags`. Any writer but
map fails, which is what makes every placement, move and removal reach these items through the scheduler's events.

Players' state is one region, "Players region" (allocated by the engine in LoadBattleAssets), with the two clocks and the end-game countdown that the Player and Game records keep unused, and the mission conditions: the members of `MissionCondition` and of each victory and defeat condition type, such as `VictoryTimerRunsOut` and `DefeatCommanderKilled`, are private for reads and writes, and `playersState` reports them.
Two items are read elsewhere, so private for writes only: `MissionConditions` (the list, whose registration the seams read) and `VictoryMoveUnitToRadius` (map's radius walk calls its visitor, which the log charges to map). The rest of a player's fields (units, the economy's accounts, the controller and the slot's set-up) are not listed: others write them. Any writer but players fails, which is what makes every capture, kill, transfer and creation or death of a unit reach the conditions through the scheduler's events.

Ai's state is written by ai alone: the squad manager and its timers (`SquadManager`, `SquadTimer` and the six timers that derive from it), the squad records (`Group`) and the records a computer player's AI holds (`KeyedCell`, `BuildRatingWeights`, `PercentByte`, `UnitTypeInt`, `CenterPos` and `AiVec3`) are private for reads and writes, and `aiState` reports them. `PlayerAI` itself is private for writes only: the log charges reads of its `vec_65` and `vec_8d` to no subsystem (`unknown`), and they are ai's own code (PlayerAI::ComputeBaseWeights). The pointer to the squads of a player (`Player::groups`) and the two globals `g_aiCommandsEnabled` and `g_playerAI` are private for writes only too: others read the pointers of `g_playerAI` through the indexes of los_radar and units.
`Unit::group` is not listed: orders' GetBuiltOrder gives a human player's built unit its factory's group, a UI group, and LoadUnit writes it. `Player::ai` is not listed either: players' slot set-up writes it. Any writer but ai fails, which is what makes every reaction, squad change and weight or limit command reach these items through the scheduler's events.

Weapons' state is written by weapons alone and read by others: the projectile table and its count (`Game::projectiles`, `Game::projectileCount` and the members of `Projectile`, which ai and los_radar read), the explosions and debris (`Game::count_1491b`, the `debris` array, the members of `Explosion`, `ExplodedPiece` and `Debris` but `Debris::ref1` and `Debris::ref2`, which the GAF helpers of game and presentation write, `DebrisPiece::state` and `DebrisCache::cur`, `Debris::pos` is read by map) and the exploded pieces (`g_explodedPieces`), and the weapon slots of a unit (`Unit::weapons`, src/weapons/unit_weapon_slot.h), which the log names by the members of the union of `Unit` that starts each word of a slot: the cob callback, the weapon, the reload timer, the fire angles, the flags and the three pads of each slot. Not listed: the aim target pair of each slot (targeting's, above). The stockpile of each slot (`Unit::field_1e`, `Unit::b3a` and `Unit::b56`) is listed: orders' build order adds a round through ADD_STOCKPILE. Any writer but weapons fails, which is what makes every damage, claim, release, detonation, stockpile and aim reach these items through the scheduler's events.

Orders' state is the orders of a unit, written by orders alone and read by others: the members of `Order` (the order objects; ai, features, map, movement and presentation read them) and the head of the unit's first order list (`Unit::list`, read by ai, movement and presentation) are private for writes only, and the head of its second list (`Unit::list2`, which only orders reads) for reads and writes. `Unit::netDirtyFlags` is not listed: orders, scripts, targeting and units write it. The unit record's position, heading, cell and flags stay units': orders publishes effects that units' handlers apply. An order's link (`PathOrderAttach`, the words `Order::unknown_12`, `Order::target` and `Order::unknown_1a`) is an entry of the list of references to its target unit, which units keeps (UnitRefSetUnit, UnitRefSetValue and the link's constructor and destructor), so units may write those three items too (ALSO_WRITTEN_BY). Any other writer but orders fails, which is what makes every command, stop, queue, attack and paralysis reach the orders through the scheduler's events.

Movement's state is the motion of a unit and the goals and targets that steer it, written by movement alone and read by others: the members of `UnitMotion`, of the goals (`PathGoal`, `AiSearchGoal`, `PackedGoal`, `LiteGoal`, `PackedPosGoal`, `PatrolGoal`) and of the targets that orders hold (`PathOrder`, `AirManeuverOrder`, `ApproachRadius`, `RingApproach`, `PointMarker`), which the harness reads for the trace, and the pointer to a unit's motion (`Unit::motion`) are private for writes only. `Unit::netDirtyFlags` is not listed (see orders). The unit record's position, heading, cell and flags stay units': movement publishes effects that units' handlers apply. A path target's link (`PathOrder::ref`) is units' like an order's, so units may write it too. The path lock stamp (`UnitMotion::pathLockStamp`) is not movement's but map's (above): movement neither writes it nor carries it in its events. Any other writer but movement fails, which is what makes every creation, destruction, load, mode, speed, target and path event reach the motion through the scheduler's events.

A field is judged by its item, so the ten cursors count as one item, and a field of a union as the first member of the union.
Standard library plus tools/depclass.py.
"""

import argparse
import csv
import gzip
import io
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

import depclass
from sources import ROOT

OWNER = "pathfinding"
READS_DIR = ROOT / "data/access_reads"
READS_SUFFIX = ".reads.csv"
ACCESS_DIR = ROOT / "data/access"
PHASES_DIR = ROOT / "data/access_phases"
PRESENTATION_DIR = ROOT / "data/access_presentation"
SCHEMA = ROOT / "data/trace_schema.json"
ALLOW = ROOT / "data/shadow_field_allow.csv"
ALLOW_COLUMNS = ["item", "reason"]
NONE = "-"
REGION_ROOT = "pathfinder"
REGION_TYPES = ("Pathfinder", "PathExtra", "Node", "PathCell", "PathPending")
REGION_ALLOCATION = re.compile(r"AISearch region")
REGION_FUNCTION = "LoadBattleAssets"


FEATURES_WRITE_ITEMS = ("scanIndex", "Cell::feature", "Cell::spot", "Cell::offsetX", "Cell::offsetY", "FeatureAnimEntry::next", "FeatureAnimEntry::prev")

TARGETING_WRITE_ITEMS = ("Unit::current", "Unit::field_20", "Unit::weapon2Target")

LOS_RADAR_WRITE_ITEMS = ("LoadTntMap: GameAllocIgnoreTag MAPPED MEMORY", "InitPlayerSlot: operator new unsigned char")

LOS_RADAR_ITEMS = ("count_14277", "eyes")

LOS_RADAR_TYPES = ("Eye",)

LOS_RADAR_WRITE_TYPES = ("LosUnitIndex", "LosUnitIndexEntry")

MAP_WRITE_ITEMS = ("Unit::spatialBucket", "BuildAllPassMaps: operator new unsigned int", "g_lastPlaceHeight", "g_lastPlaceMetalSum", "UnitMotion::pathLockStamp")

MAP_TYPES = ("UnitBucket", "SpatialBucket")

PLAYERS_WRITE_TYPES = ("MissionConditions", "VictoryMoveUnitToRadius")

PLAYERS_TYPES = ("MissionCondition", "VictoryKillEnemyCommander", "VictoryDestroyAllUnits", "VictoryKillAllMobileUnits",
                       "VictoryBuildUnitType", "VictoryCaptureUnitType", "VictoryKillAllOfType", "VictoryKillUnitType",
                       "VictoryUnitTypePassesX", "VictoryUnitTypePassesZ", "VictoryTimerRunsOut", "DefeatCommanderKilled", "DefeatAllUnitsKilled",
                       "DefeatAllUnitsKilledOfType", "DefeatUnitTypeKilled", "DefeatDeathTimerRunsOut", "DefeatAnyUnitPassesX", "DefeatAnyUnitPassesZ")

AI_WRITE_ITEMS = ("Player::groups", "g_aiCommandsEnabled", "g_playerAI")

AI_WRITE_TYPES = ("PlayerAI",)

AI_TYPES = ("KeyedCell", "BuildRatingWeights", "PercentByte", "UnitTypeInt", "CenterPos", "AiVec3",
                  "SquadManager", "SquadTimer", "AssaultTimer", "EscortTimer", "SquadScoutTimer", "ScoutTimer", "BuildTimer", "SpatialTimer", "Group")


WEAPONS_WRITE_TYPES = ("Projectile", "Explosion", "ExplodedPiece")

WEAPONS_WRITE_ITEMS = ("projectiles", "projectileCount", "count_1491b", "debris", "g_explodedPieces", "DebrisPiece::state", "DebrisCache::cur",
                       "Debris::field_0", "Debris::pos", "Debris::vel", "Debris::spin", "Debris::size", "Debris::angle_x", "Debris::angle_y", "Debris::angle_z",
                       "Unit::field_8", "Unit::type", "Unit::weapon", "Unit::unknown_14", "Unit::field_16", "Unit::field_18", "Unit::unknown_1a", "Unit::f_1b",
                       "Unit::unknown_1c", "Unit::field_1e", "Unit::f_1f", "Unit::weapon1AimCob", "Unit::f2c", "Unit::unknown_30",
                       "Unit::b3a", "Unit::f_3b", "Unit::weapon2AimCob", "Unit::f48", "Unit::unknown_4c", "Unit::b56", "Unit::f_57")


ORDERS_WRITE_TYPES = ("Order",)

ORDERS_ITEMS = ("Unit::list2",)

ORDERS_WRITE_ITEMS = ("Unit::list",)

MOVEMENT_WRITE_TYPES = ("UnitMotion", "PathGoal", "AiSearchGoal", "PackedGoal", "LiteGoal", "PackedPosGoal", "PatrolGoal", "PathOrder",
                        "AirManeuverOrder", "ApproachRadius", "RingApproach", "PointMarker")

MOVEMENT_WRITE_ITEMS = ("Unit::motion",)

MOVEMENT_EXCEPT = ("UnitMotion::pathLockStamp",)


ORDERS_ALSO_WRITTEN_BY = (("Order::unknown_12", "units"), ("Order::target", "units"), ("Order::unknown_1a", "units"))

MOVEMENT_ALSO_WRITTEN_BY = (("PathOrder::ref", "units"),)


@dataclass(frozen=True)
class Owner:
    """A subsystem with private state: the items of its types, its region root and its allocation (an item named `<function>: <allocator> <tag>` of a region it allocates), the types of items private for writes only, and the named items private for reads and writes (items) or for writes only (write_items)."""
    name: str
    root: str
    types: tuple
    allocation: re.Pattern
    write_types: tuple = ()
    items: tuple = ()
    write_items: tuple = ()
    except_items: tuple = ()
    also_written_by: tuple = ()


METEOR_GLOBALS = ("g_meteorNextStrikeTime", "g_meteorWeaponName", "g_meteorSpawnMagnitude", "g_meteorSpawnInterval", "g_meteorActive",
                  "g_meteorStrikeEndTime", "g_meteorOrigin", "g_meteorShowerDuration", "g_meteorWeapon", "g_meteorsEnabled",
                  "g_meteorNextHitTime", "g_meteorTarget", "g_meteorScheduleGap")
WIND_FIELDS = ("windCounter", "windSpeedDenominator", "windX", "windY", "windZ", "windDirection", "windSpeed", "windStrength",
               "windEnabled", "windSpeedMin", "windSpeedMax")
NO_ALLOCATION = re.compile(r"(?!)")
OWNERS = (Owner(OWNER, REGION_ROOT, REGION_TYPES, REGION_ALLOCATION),
          Owner("economy", "", (), re.compile(r"Economy region"), ("UnitResources",)),
          Owner("wind_meteors", "", (), NO_ALLOCATION, (), METEOR_GLOBALS, WIND_FIELDS),
          Owner("features", "", (), NO_ALLOCATION, write_items=FEATURES_WRITE_ITEMS),
          Owner("targeting", "", (), NO_ALLOCATION, write_items=TARGETING_WRITE_ITEMS),
          Owner("los_radar", "", LOS_RADAR_TYPES, re.compile(r"LosRadar region"), LOS_RADAR_WRITE_TYPES, LOS_RADAR_ITEMS, LOS_RADAR_WRITE_ITEMS),
          Owner("map", "", MAP_TYPES, NO_ALLOCATION, write_items=MAP_WRITE_ITEMS),
          Owner("players", "", PLAYERS_TYPES, re.compile(r"Players region"), PLAYERS_WRITE_TYPES),
          Owner("ai", "", AI_TYPES, NO_ALLOCATION, AI_WRITE_TYPES, write_items=AI_WRITE_ITEMS),
          Owner("weapons", "", (), NO_ALLOCATION, WEAPONS_WRITE_TYPES, write_items=WEAPONS_WRITE_ITEMS),
          Owner("orders", "", (), NO_ALLOCATION, ORDERS_WRITE_TYPES, ORDERS_ITEMS, ORDERS_WRITE_ITEMS, also_written_by=ORDERS_ALSO_WRITTEN_BY),
          Owner("movement", "", (), NO_ALLOCATION, MOVEMENT_WRITE_TYPES, write_items=MOVEMENT_WRITE_ITEMS, except_items=MOVEMENT_EXCEPT,
                also_written_by=MOVEMENT_ALSO_WRITTEN_BY))

RECORD = {"pool": "Pathfinder::pool", "items": "Pathfinder::items", "freeHead": "Pathfinder::freeHead", "used": "Pathfinder::used",
          "count": "Pathfinder::count", "topPopped": "Pathfinder::topPopped", "cells": "Pathfinder::grid.cells",
          "width": "Pathfinder::grid.width", "height": "Pathfinder::grid.height", "cellCount": "Pathfinder::grid.count",
          "dirty": "Pathfinder::grid.dirty", "stepsPerTick": "Pathfinder::stepsPerTick", "steps": "Pathfinder::steps",
          "costScale": "Pathfinder::costScale", "baseScale": "Pathfinder::baseScale", "object": "Pathfinder::object",
          "player": "Pathfinder::player", "cursor": "Pathfinder::cursor", "budget": "Pathfinder::budget"}
SEARCH = {"start_x": "Pathfinder::start.x", "start_y": "Pathfinder::start.y", "goalX": "Pathfinder::goalX",
          "goalY": "Pathfinder::goalY", "probe": "Pathfinder::probe", "range": "Pathfinder::range", "path": "Pathfinder::path",
          "target": "Pathfinder::target", "owner": "Pathfinder::owner", "turnCost": "Pathfinder::table",
          "stepCost": "Pathfinder::pairs"}
NODE = {"index": "Node::index", "pos_x": "Node::data.pos.x", "pos_y": "Node::data.pos.y", "g": "Node::data.g",
        "f": "Node::data.f", "steps": "Node::data.steps"}
GLOBALS = {"playerTickLoad": "PathExtra::tickLoad", "playerBudgetCap": "PathExtra::budgetCap",
           "budgetRefreshCounter": "PathExtra::refreshCounter"}
HEAP = "AISearch region.heap"
DIRTY = "AISearch region.dirty"
EXTRA = {"Node::data.penalty": "node penalty, hashed with a node whose g is not 0 (harness VisitPathNodes)",
         "PathCell::flags": "cell flags, hashed for the cells of the dirty words (harness VisitPathCells)",
         "PathCell::dir": "cell dir, hashed for an open or closed cell or one with flag 8",
         "PathCell::node": "cell node, hashed for an open or closed cell",
         HEAP: "the open heap's entries, hashed after pathfinder.heap.count",
         DIRTY: "the dirty words, hashed before the cells they cover"}
TABLES = (("Pathfinder", RECORD), ("PathfinderSearch", SEARCH), ("Node", NODE))


def exposed(schema: dict) -> tuple[dict, list]:
    """({item: [where it is hashed]}, [problems]) from the schema's tables and globals and the fixed entries of EXTRA."""
    out: dict[str, list[str]] = {}
    problems = []
    for table, names in TABLES:
        for f in schema["tables"][table]["fields"]:
            name = re.sub(r"\[\d+\]$", "", f["name"])
            item = names.get(name)
            if item is None:
                problems.append("the trace schema field %s.%s has no entry in privatestate.py" % (table, f["name"]))
                continue
            out.setdefault(item, []).append("%s.%s" % (table, f["name"]))
    for g in schema["globals"]["pathfinder"]:
        item = GLOBALS.get(g["name"])
        if item is None:
            problems.append("the trace schema global pathfinder.%s has no entry in privatestate.py" % g["name"])
            continue
        out.setdefault(item, []).append("pathfinder.%s" % g["name"])
    for item, where in EXTRA.items():
        out.setdefault(item, []).append(where)
    return out, problems


def private_to(item: str) -> tuple[str, tuple[str, ...]]:
    """(owner, roles) of an item of the log that is private state: the owner's name and the roles ("writer", "reader") no other subsystem may take. ("", ()) if it is not."""
    for o in OWNERS:
        if item in o.except_items:
            continue
        if o.root and item == o.root:
            return o.name, ("writer", "reader")
        if item.split("::")[0] in o.types:
            return o.name, ("writer", "reader")
        if item.startswith(REGION_FUNCTION + ":") and o.allocation.search(item) is not None:
            return o.name, ("writer", "reader")
        if item in o.items:
            return o.name, ("writer", "reader")
        if item.split("::")[0] in o.write_types or item in o.write_items:
            return o.name, ("writer",)
    return "", ()


def is_private(item: str) -> bool:
    """Whether an item of the log is the root or a part of the allocation of an owner, or a private type of it."""
    return private_to(item)[0] != ""


def read_gz(path: Path) -> list[dict]:
    return list(csv.DictReader(io.StringIO(gzip.decompress(path.read_bytes()).decode())))


def stored_rows() -> dict[str, list[tuple[str, str, str]]]:
    """{file kind: [(writer, reader, item)]} of every stored summary with an item, `-` for a column the file does not have."""
    out: dict[str, list[tuple[str, str, str]]] = {"edges": [], "cross": [], "pass": [], "presentation": [], "reads": []}
    for kind, directory in (("edges", ACCESS_DIR), ("cross", PHASES_DIR), ("pass", PHASES_DIR), ("reads", READS_DIR)):
        suffix = ".%s.csv.gz" % kind
        for path in sorted(directory.glob("*" + suffix)):
            for r in read_gz(path):
                out[kind].append((r.get("writer", NONE), r.get("reader", NONE), r["item"]))
    for path in sorted(PRESENTATION_DIR.glob("*.csv")):
        with path.open(newline="") as fh:
            out["presentation"].extend((NONE, r["reader"], r["item"]) for r in csv.DictReader(fh))
    return out


def violations(rows: dict[str, list[tuple[str, str, str]]]) -> list[str]:
    """One message for each (file kind, role, subsystem, item) of the rows in which a subsystem but the owner takes a role the item bars."""
    found = set()
    for kind, triples in rows.items():
        for writer, reader, item in triples:
            owner, roles = private_to(item)
            if not owner:
                continue
            also = {w for i, w in next(o for o in OWNERS if o.name == owner).also_written_by if i == item}
            for role, who in (("writer", writer), ("reader", reader)):
                if role in roles and who not in (NONE, owner) and not (role == "writer" and who in also):
                    found.add((kind, role, who, item, owner, "private" if len(roles) > 1 else "written only by"))
    return ["%s: %s %s on %s, which is %s %s" % (k, r, w, i, "private to" if how == "private" else "written only by", o)
            for k, r, w, i, o, how in sorted(found)]


def canonical(item: str) -> str:
    """The item of a part of the allocation without the name of the allocating function and allocator."""
    if item.startswith(REGION_FUNCTION + ":"):
        found = REGION_ALLOCATION.search(item)
        if found:
            return item[found.start():]
    return item


def pathfinding_reads(reads: list[dict]) -> set[str]:
    return {canonical(r["item"]) for r in reads if r["reader"] == OWNER}


def shadow_problems(wanted: dict, read: set[str], allow: dict[str, str]) -> list[str]:
    """The shadow fields (an exposed item that pathfinding never reads, with no reason in the allow list) and the stale allow rows."""
    out = []
    for item in sorted(wanted):
        if item not in read and item not in allow:
            out.append("shadow field %s (%s) is hashed by the trace but pathfinding reads it in no scenario" % (item, ", ".join(wanted[item])))
        if item in read and item in allow:
            out.append("%s: stale row for %s, which pathfinding reads" % (ALLOW.name, item))
    for item in sorted(set(allow) - set(wanted)):
        out.append("%s: %s is not an exposed item" % (ALLOW.name, item))
    return out


def read_allow() -> dict[str, str]:
    if not ALLOW.is_file():
        return {}
    with ALLOW.open(newline="") as fh:
        return {r["item"]: r["reason"] for r in csv.DictReader(fh)}


def store(source: Path, directory: Path) -> int:
    """Keep each reads file of source (accesslog.py's output) in directory, sorted and gzipped."""
    import accesslog

    for stats in sorted(source.glob("*.stats.txt")):
        accesslog.check_stats(stats.name[: -len(".stats.txt")], stats)
    directory.mkdir(exist_ok=True)
    count = 0
    for path in sorted(source.glob("*" + READS_SUFFIX)):
        with path.open(newline="") as fh:
            rows = sorted(csv.DictReader(fh), key=lambda r: (r["item"], r["reader"]))
        buf = io.StringIO()
        w = csv.writer(buf, lineterminator="\n")
        w.writerow(["item", "reader", "count", "first_tick"])
        w.writerows([r["item"], r["reader"], r["count"], r["first_tick"]] for r in rows)
        (directory / (path.name + ".gz")).write_bytes(gzip.compress(buf.getvalue().encode(), 9, mtime=0))
        count += 1
    return count


def check() -> list[str]:
    problems = []
    names = {p.name[: -len(READS_SUFFIX + ".gz")] for p in READS_DIR.glob("*" + READS_SUFFIX + ".gz")}
    for s in depclass.missing_scenarios({n: [] for n in names}):
        problems.append("no summary data/access_reads/%s.reads.csv.gz for reference/scenarios/%s.txt" % (s, s))
    problems += violations(stored_rows())
    schema = json.loads(SCHEMA.read_text())
    wanted, bad = exposed(schema)
    problems += bad
    read = set()
    for path in sorted(READS_DIR.glob("*" + READS_SUFFIX + ".gz")):
        read |= pathfinding_reads(read_gz(path))
    problems += shadow_problems(wanted, read, read_allow())
    return problems


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--store", type=Path, metavar="DIR", help="store the reads files of DIR in data/access_reads")
    mode.add_argument("--check", action="store_true", help="fail on another subsystem's access to the region or on a shadow field")
    args = ap.parse_args()
    if args.store:
        print("privatestate: stored %d reads files" % store(args.store, READS_DIR))
        return 0
    problems = check()
    if problems:
        print("\n".join(problems))
        return 1
    print("privatestate: the state of %s is private, or written only by its owner, in the stored summaries and every field the trace hashes is read"
          % ", ".join(o.name for o in OWNERS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
