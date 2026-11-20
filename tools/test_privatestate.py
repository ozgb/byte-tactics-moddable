"""Tests for privatestate.py, on synthetic summaries.

    uv run tools/test_privatestate.py
"""

import gzip
import json
import tempfile
import unittest
from pathlib import Path

import privatestate as ps

ALLOCATION = "LoadBattleAssets: GameAllocIgnoreTag AISearch region"


class PrivateItemTest(unittest.TestCase):
    def test_region_items_are_private(self):
        for item in ("pathfinder", "Pathfinder::steps", "PathExtra::seenWidth", "Node::data.g", "PathCell::flags", "PathPending::unit",
                     ALLOCATION + ".heap", ALLOCATION + ".events"):
            self.assertTrue(ps.is_private(item), item)

    def test_other_items_are_not(self):
        for item in ("Unit::pos", "g_game", "players", "HpaInit: GameAllocIgnoreTag AISearch region", "Pathfinders", "PathEvent::kind",
                     "rng.parkMiller"):
            self.assertFalse(ps.is_private(item), item)


class EconomyOwnerTest(unittest.TestCase):
    REGION = "LoadBattleAssets: SimAllocRegion Economy region"

    def test_region_parts_are_private_to_economy(self):
        self.assertEqual(ps.private_to(self.REGION + ".accounts"), ("economy", ("writer", "reader")))
        self.assertEqual(ps.private_to(ALLOCATION + ".heap")[0], "pathfinding")

    def test_player_accounts_are_private_for_writes_only(self):
        self.assertEqual(ps.private_to("UnitResources::energyMake"), ("economy", ("writer",)))
        rows = {"edges": [("players", "economy", "UnitResources::metalMake"), ("economy", "players", "UnitResources::metalMake"),
                          ("-", "ai", "UnitResources::energyStall")]}
        found = ps.violations(rows)
        self.assertEqual(found, ["edges: writer players on UnitResources::metalMake, which is written only by economy"])

    def test_another_subsystem_touching_the_region_is_found(self):
        rows = {"edges": [("units", "economy", self.REGION + ".accounts"), ("economy", "players", self.REGION + ".accounts")],
                "reads": [("-", "weapons", self.REGION + ".accounts"), ("-", "economy", self.REGION + ".accounts")]}
        found = ps.violations(rows)
        self.assertEqual(len(found), 3)
        self.assertTrue(all("private to economy" in line for line in found))

    def test_owners_do_not_share_items(self):
        rows = {"edges": [("economy", "-", "Pathfinder::steps"), ("pathfinding", "-", self.REGION + ".accounts")]}
        self.assertEqual(len(ps.violations(rows)), 2)


class WindMeteorsOwnerTest(unittest.TestCase):
    def test_meteor_globals_are_private_for_reads_and_writes(self):
        self.assertEqual(ps.private_to("g_meteorNextStrikeTime"), ("wind_meteors", ("writer", "reader")))
        self.assertEqual(len(ps.METEOR_GLOBALS), 13)
        rows = {"edges": [("wind_meteors", "-", "g_meteorActive"), ("-", "wind_meteors", "g_meteorActive"),
                          ("-", "weapons", "g_meteorWeapon"), ("game", "-", "g_meteorsEnabled")],
                "reads": [("-", "game", "g_meteorOrigin"), ("-", "wind_meteors", "g_meteorOrigin")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: reader weapons on g_meteorWeapon, which is private to wind_meteors",
                          "edges: writer game on g_meteorsEnabled, which is private to wind_meteors",
                          "reads: reader game on g_meteorOrigin, which is private to wind_meteors"])

    def test_wind_fields_are_private_for_writes_only(self):
        self.assertEqual(ps.private_to("windSpeedMax"), ("wind_meteors", ("writer",)))
        rows = {"edges": [("wind_meteors", "units", "windSpeed"), ("-", "ai", "windStrength"), ("wind_meteors", "-", "windZ"),
                          ("units", "-", "windDirection"), ("-", "units", "windSpeed")]}
        self.assertEqual(ps.violations(rows), ["edges: writer units on windDirection, which is written only by wind_meteors"])

    def test_similar_names_are_not_private(self):
        for item in ("windgenerator", "UnitDef::windgenerator", "Game::windSpeed", "g_meteors", "g_meteorWeaponX"):
            self.assertFalse(ps.is_private(item), item)

class FeaturesOwnerTest(unittest.TestCase):
    def test_spots_cell_fields_and_the_scan_index_are_private_for_writes_only(self):
        for item in ("FeatureAnimEntry::next", "FeatureAnimEntry::prev", "Cell::feature", "Cell::spot", "Cell::offsetX", "Cell::offsetY", "scanIndex"):
            self.assertEqual(ps.private_to(item), ("features", ("writer",)), item)

    def test_the_cell_flags_and_other_features_state_are_not_private(self):
        for item in ("Cell::flags", "Cell::unit", "Feature::energy", "featureCount", "FeatureAnimEntry2::next", "scanIndexes",
                     "FeatureAnimEntry::src", "FeatureAnimEntry::state", "FeatureAnimEntry::bytes_8", "FeatureAnimEntry::bytes_10"):
            self.assertFalse(ps.is_private(item), item)

    def test_a_features_write_passes(self):
        rows = {"edges": [("features", "weapons", "FeatureAnimEntry::next"), ("features", "features", "Cell::feature"),
                          ("features", "-", "scanIndex"), ("-", "features", "Cell::spot")],
                "cross": [("features", "units", "Cell::offsetX")]}
        self.assertEqual(ps.violations(rows), [])

    def test_a_units_or_weapons_write_fails(self):
        rows = {"edges": [("units", "features", "FeatureAnimEntry::prev"), ("weapons", "map", "Cell::feature"),
                          ("units", "-", "Cell::spot"), ("weapons", "features", "scanIndex")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer units on Cell::spot, which is written only by features",
                          "edges: writer units on FeatureAnimEntry::prev, which is written only by features",
                          "edges: writer weapons on Cell::feature, which is written only by features",
                          "edges: writer weapons on scanIndex, which is written only by features"])

    def test_a_weapons_or_orders_read_passes(self):
        rows = {"edges": [("-", "weapons", "FeatureAnimEntry::src"), ("features", "orders", "Cell::feature")],
                "reads": [("-", "weapons", "FeatureAnimEntry::bytes_10"), ("-", "map", "scanIndex")], "presentation": [("-", "presentation", "Cell::spot")]}
        self.assertEqual(ps.violations(rows), [])

    def test_the_owners_are_pathfinding_economy_wind_meteors_features_targeting_los_radar_map_players_ai_weapons_orders_and_movement(self):
        self.assertEqual([o.name for o in ps.OWNERS], ["pathfinding", "economy", "wind_meteors", "features", "targeting", "los_radar", "map", "players", "ai", "weapons", "orders", "movement"])



class TargetingOwnerTest(unittest.TestCase):
    def test_the_weapon_target_pairs_are_private_for_writes_only(self):
        for item in ("Unit::current", "Unit::field_20", "Unit::weapon2Target"):
            self.assertEqual(ps.private_to(item), ("targeting", ("writer",)), item)

    def test_other_unit_members_are_not_private(self):
        for item in ("Unit::netDirtyFlags", "Unit::pos", "Unit::current2", "Unit::weapon2Target2"):
            self.assertFalse(ps.is_private(item), item)

    def test_a_targeting_write_passes(self):
        rows = {"edges": [("targeting", "weapons", "Unit::current"), ("targeting", "-", "Unit::field_20"), ("-", "targeting", "Unit::weapon2Target")],
                "cross": [("targeting", "units", "Unit::weapon2Target")]}
        self.assertEqual(ps.violations(rows), [])

    def test_a_weapons_or_units_write_fails(self):
        rows = {"edges": [("weapons", "targeting", "Unit::current"), ("units", "-", "Unit::weapon2Target"), ("weapons", "-", "Unit::field_20")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer units on Unit::weapon2Target, which is written only by targeting",
                          "edges: writer weapons on Unit::current, which is written only by targeting",
                          "edges: writer weapons on Unit::field_20, which is written only by targeting"])

    def test_a_read_and_a_shared_flag_write_pass(self):
        rows = {"edges": [("-", "weapons", "Unit::current"), ("targeting", "units", "Unit::weapon2Target"), ("weapons", "-", "Unit::netDirtyFlags")],
                "reads": [("-", "ai", "Unit::field_20")]}
        self.assertEqual(ps.violations(rows), [])


class LosRadarOwnerTest(unittest.TestCase):
    MAPPED = "LoadTntMap: GameAllocIgnoreTag MAPPED MEMORY"
    EXPLORED = "InitPlayerSlot: operator new unsigned char"
    REGION = "LoadBattleAssets: SimAllocRegion LosRadar region"

    def test_the_visibility_layer_and_the_explored_maps_are_private_for_writes_only(self):
        for item in (self.MAPPED, self.EXPLORED, "LosUnitIndexEntry::visible"):
            self.assertEqual(ps.private_to(item), ("los_radar", ("writer",)), item)

    def test_the_eyeballs_and_the_region_are_private_for_reads_and_writes(self):
        for item in ("count_14277", "eyes", "Eye::pos", "Eye::expires", "Eye::screenPos", self.REGION + ".sight"):
            self.assertEqual(ps.private_to(item), ("los_radar", ("writer", "reader")), item)

    def test_the_cells_and_the_shared_state_are_not_private(self):
        for item in ("LoadTntMap: GameAllocIgnoreTag PLOT MEMORY", "Cell::height", "Unit::unusedLosCache", "Unit::flags",
                     "Game::mapFlags_14281", "InitPlayerSlot: operator new Group", "LoadTntMap: GameAllocIgnoreTag MAPPED MEMORY 2"):
            self.assertFalse(ps.is_private(item), item)

    def test_a_los_radar_access_passes(self):
        rows = {"edges": [("los_radar", "units", self.MAPPED), ("los_radar", "-", self.EXPLORED), ("-", "los_radar", "count_14277")],
                "cross": [("los_radar", "los_radar", "Eye::pos")], "reads": [("-", "los_radar", self.REGION + ".sight")]}
        self.assertEqual(ps.violations(rows), [])

    def test_another_subsystem_write_fails(self):
        rows = {"edges": [("players", "los_radar", self.EXPLORED), ("map", "-", self.MAPPED)]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer map on %s, which is written only by los_radar" % self.MAPPED,
                          "edges: writer players on %s, which is written only by los_radar" % self.EXPLORED])

    def test_another_subsystem_touching_the_eyeballs_or_the_region_fails(self):
        rows = {"edges": [("units", "-", "Eye::x"), ("game", "-", "eyes")], "reads": [("-", "weapons", "Eye::pos"), ("-", "ai", self.REGION + ".sight")]}
        self.assertEqual(len(ps.violations(rows)), 4)
        self.assertTrue(all("private to los_radar" in line for line in ps.violations(rows)))

    def test_a_read_of_the_visibility_layer_passes(self):
        rows = {"edges": [("-", "sound", self.EXPLORED), ("los_radar", "ai", self.MAPPED)], "presentation": [("-", "presentation", self.MAPPED)]}
        self.assertEqual(ps.violations(rows), [])


class MapOwnerTest(unittest.TestCase):
    PASS_MAPS = "BuildAllPassMaps: operator new unsigned int"

    def test_the_unit_bucket_and_the_pass_maps_are_private_for_writes_only(self):
        for item in ("Unit::spatialBucket", self.PASS_MAPS, "g_lastPlaceHeight", "g_lastPlaceMetalSum"):
            self.assertEqual(ps.private_to(item), ("map", ("writer",)), item)

    def test_the_bucket_records_are_private_for_reads_and_writes(self):
        for item in ("UnitBucket::first", "SpatialBucket::head"):
            self.assertEqual(ps.private_to(item), ("map", ("writer", "reader")), item)

    def test_the_cells_and_the_shared_unit_state_are_not_private(self):
        for item in ("LoadTntMap: GameAllocIgnoreTag PLOT MEMORY", "Cell::unit", "Cell::flags", "Unit::cargoNext", "Unit::flags", "Unit::bit2",
                     "Unit::cobStateFlags", "cells_1429f", "overflowBucket", "BuildAllPassMaps: operator new unsigned short"):
            self.assertFalse(ps.is_private(item), item)

    def test_a_map_access_passes(self):
        rows = {"edges": [("map", "units", "Unit::spatialBucket"), ("map", "-", self.PASS_MAPS), ("-", "map", "g_lastPlaceMetalSum")],
                "cross": [("map", "ai", "g_lastPlaceHeight")], "reads": [("-", "map", "UnitBucket::first")]}
        self.assertEqual(ps.violations(rows), [])

    def test_another_subsystem_write_fails(self):
        rows = {"edges": [("units", "map", "Unit::spatialBucket"), ("ai", "-", "g_lastPlaceMetalSum"), ("pathfinding", "-", self.PASS_MAPS)]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer ai on g_lastPlaceMetalSum, which is written only by map",
                          "edges: writer pathfinding on %s, which is written only by map" % self.PASS_MAPS,
                          "edges: writer units on Unit::spatialBucket, which is written only by map"])

    def test_another_subsystem_touching_the_bucket_records_fails(self):
        rows = {"edges": [("orders", "-", "UnitBucket::first")], "reads": [("-", "orders", "SpatialBucket::head")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer orders on UnitBucket::first, which is private to map",
                          "reads: reader orders on SpatialBucket::head, which is private to map"])

    def test_a_read_of_the_unit_bucket_passes(self):
        rows = {"edges": [("-", "orders", "Unit::spatialBucket"), ("map", "ai", "g_lastPlaceMetalSum")], "reads": [("-", "units", self.PASS_MAPS)],
                "presentation": [("-", "presentation", "g_lastPlaceHeight")]}
        self.assertEqual(ps.violations(rows), [])


class PlayersOwnerTest(unittest.TestCase):
    REGION = "LoadBattleAssets: SimAllocRegion Players region"

    def test_the_conditions_and_the_region_are_private_for_reads_and_writes(self):
        for item in ("MissionCondition::met", "VictoryTimerRunsOut::deadline", "DefeatCommanderKilled::player", self.REGION + ".clocks"):
            self.assertEqual(ps.private_to(item), ("players", ("writer", "reader")), item)

    def test_the_condition_list_and_the_radius_condition_are_private_for_writes_only(self):
        for item in ("MissionConditions::active", "VictoryMoveUnitToRadius::pos"):
            self.assertEqual(ps.private_to(item), ("players", ("writer",)), item)

    def test_the_rest_of_a_player_is_not_private(self):
        for item in ("Player::econ", "Player::units", "Player::controller", "Player::unusedUpdateTime"):
            self.assertFalse(ps.is_private(item), item)

    def test_a_players_access_passes(self):
        rows = {"edges": [("players", "units", "MissionConditions::active"), ("players", "-", "MissionCondition::met")],
                "cross": [("players", "ai", "VictoryMoveUnitToRadius::pos")], "reads": [("-", "map", "VictoryMoveUnitToRadius::pos")]}
        self.assertEqual(ps.violations(rows), [])

    def test_another_subsystem_write_fails(self):
        rows = {"edges": [("units", "players", "MissionConditions::active"), ("ai", "-", "VictoryTimerRunsOut::deadline")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer ai on VictoryTimerRunsOut::deadline, which is private to players",
                          "edges: writer units on MissionConditions::active, which is written only by players"])

    def test_another_subsystem_reading_a_condition_fails(self):
        rows = {"reads": [("-", "orders", "DefeatCommanderKilled::player"), ("-", "ai", self.REGION + ".clocks")]}
        self.assertEqual(len(ps.violations(rows)), 2)


class AiOwnerTest(unittest.TestCase):
    def test_the_squad_records_are_private_for_reads_and_writes(self):
        for item in ("KeyedCell::key", "SquadManager::nextAction", "SquadTimer::next", "AssaultTimer::attacking", "SpatialTimer::best", "Group::units"):
            self.assertEqual(ps.private_to(item), ("ai", ("writer", "reader")), item)

    def test_the_player_record_and_the_three_items_are_private_for_writes_only(self):
        for item in ("PlayerAI::lastTick", "Player::groups", "g_aiCommandsEnabled", "g_playerAI"):
            self.assertEqual(ps.private_to(item), ("ai", ("writer",)), item)

    def test_the_group_of_a_unit_and_the_ai_of_a_player_are_not_private(self):
        for item in ("Unit::group", "Player::ai", "Player::econ"):
            self.assertFalse(ps.is_private(item), item)

    def test_an_ai_access_passes(self):
        rows = {"edges": [("ai", "units", "Player::groups"), ("ai", "-", "g_aiCommandsEnabled")], "cross": [("ai", "ai", "SquadManager::countdown")],
                "reads": [("-", "ai", "Group::units"), ("-", "orders", "PlayerAI::lastTick")]}
        self.assertEqual(ps.violations(rows), [])

    def test_another_subsystem_write_fails(self):
        rows = {"edges": [("units", "ai", "Player::groups"), ("players", "-", "g_playerAI")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer players on g_playerAI, which is written only by ai",
                          "edges: writer units on Player::groups, which is written only by ai"])

    def test_another_subsystem_reading_a_squad_record_fails(self):
        rows = {"reads": [("-", "orders", "SquadManager::nextAction")]}
        self.assertEqual(ps.violations(rows), ["reads: reader orders on SquadManager::nextAction, which is private to ai"])


class WeaponsOwnerTest(unittest.TestCase):
    def test_the_projectiles_the_debris_and_the_slots_are_private_for_writes_only(self):
        for item in ("Projectile::position", "projectileCount", "projectiles", "g_explodedPieces", "Debris::pos", "DebrisPiece::state",
                     "Unit::weapon", "Unit::field_1e", "Unit::b3a", "Unit::b56", "Unit::f_1f", "Unit::f2c", "Unit::f_3b", "Unit::f48", "Unit::f_57", "Unit::weapon2AimCob"):
            self.assertEqual(ps.private_to(item), ("weapons", ("writer",)), item)

    def test_the_aim_pairs_stay_targetings_and_the_rest_is_not_private(self):
        self.assertEqual(ps.private_to("Unit::current")[0], "targeting")
        for item in ("Unit::health", "Debris::ref1", "Debris::ref2", "WeaponDef::damage"):
            self.assertFalse(ps.is_private(item), item)

    def test_a_weapons_write_passes_and_a_read_by_anyone_passes(self):
        rows = {"edges": [("weapons", "ai", "Projectile::attacker"), ("weapons", "-", "Unit::f_1f")],
                "reads": [("-", "los_radar", "Projectile::position"), ("-", "map", "Debris::pos")]}
        self.assertEqual(ps.violations(rows), [])

    def test_another_subsystem_write_fails(self):
        rows = {"edges": [("units", "weapons", "Unit::f2c"), ("orders", "-", "Projectile::flags")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer orders on Projectile::flags, which is written only by weapons",
                          "edges: writer units on Unit::f2c, which is written only by weapons"])


class OrdersOwnerTest(unittest.TestCase):
    def test_the_orders_and_the_first_list_are_private_for_writes_only(self):
        for item in ("Order::pos", "Order::subFlags", "Order::target", "Unit::list"):
            self.assertEqual(ps.private_to(item), ("orders", ("writer",)), item)

    def test_the_second_list_is_private_for_reads_and_writes(self):
        self.assertEqual(ps.private_to("Unit::list2"), ("orders", ("writer", "reader")))

    def test_the_net_dirty_flags_and_the_motion_are_not_orders(self):
        self.assertFalse(ps.is_private("Unit::netDirtyFlags"))
        self.assertEqual(ps.private_to("Unit::motion")[0], "movement")

    def test_an_orders_write_passes_and_a_read_by_anyone_passes(self):
        rows = {"edges": [("orders", "ai", "Unit::list"), ("orders", "-", "Order::flags")], "cross": [("orders", "orders", "Unit::list2")],
                "reads": [("-", "presentation", "Order::pos"), ("-", "movement", "Unit::list"), ("-", "orders", "Unit::list2")]}
        self.assertEqual(ps.violations(rows), [])

    def test_another_subsystem_write_or_second_list_read_fails(self):
        rows = {"edges": [("map", "orders", "Order::pos"), ("movement", "-", "Order::subFlags"), ("ai", "-", "Order::target")], "reads": [("-", "ai", "Unit::list2")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer ai on Order::target, which is written only by orders",
                          "edges: writer map on Order::pos, which is written only by orders",
                          "edges: writer movement on Order::subFlags, which is written only by orders",
                          "reads: reader ai on Unit::list2, which is private to orders"])

    def test_units_may_write_the_link(self):
        rows = {"edges": [("units", "-", "Order::unknown_12"), ("units", "orders", "Order::target"), ("units", "-", "Order::unknown_1a")]}
        self.assertEqual(ps.violations(rows), [])


class MovementOwnerTest(unittest.TestCase):
    def test_the_motion_the_goals_and_the_targets_are_private_for_writes_only(self):
        for item in ("UnitMotion::flags", "UnitMotion::speed", "Unit::motion", "PathGoal::target", "AiSearchGoal::path", "PackedGoal::pos", "LiteGoal::state",
                     "PackedPosGoal::pos", "PatrolGoal::index", "PathOrder::ref", "AirManeuverOrder::pos", "ApproachRadius::radius",
                     "RingApproach::inner", "PointMarker::size"):
            self.assertEqual(ps.private_to(item), ("movement", ("writer",)), item)

    def test_the_path_lock_stamp_is_maps(self):
        self.assertFalse(ps.is_private("UnitMotion::pathLockStamp"))
        rows = {"edges": [("map", "-", "UnitMotion::pathLockStamp"), ("units", "-", "UnitMotion::speed")]}
        self.assertEqual(ps.violations(rows), ["edges: writer units on UnitMotion::speed, which is written only by movement"])

    def test_a_movement_write_passes_and_a_read_by_anyone_passes(self):
        rows = {"edges": [("movement", "units", "UnitMotion::speed"), ("movement", "-", "Unit::motion")],
                "reads": [("-", "map", "UnitMotion::mode"), ("-", "platform", "PathOrder::ref"), ("-", "weapons", "Unit::motion")]}
        self.assertEqual(ps.violations(rows), [])

    def test_another_subsystem_write_fails(self):
        rows = {"edges": [("units", "movement", "Unit::motion"), ("orders", "-", "PathOrder::ref"), ("seams", "-", "Unit::motion")]}
        self.assertEqual(ps.violations(rows),
                         ["edges: writer orders on PathOrder::ref, which is written only by movement",
                          "edges: writer seams on Unit::motion, which is written only by movement",
                          "edges: writer units on Unit::motion, which is written only by movement"])

    def test_units_may_write_the_link_and_no_other_item(self):
        rows = {"edges": [("units", "movement", "PathOrder::ref"), ("units", "-", "PathOrder::pos_z")]}
        self.assertEqual(ps.violations(rows), ["edges: writer units on PathOrder::pos_z, which is written only by movement"])

    def test_the_owners_list_ends_with_orders_and_movement(self):
        self.assertEqual([o.name for o in ps.OWNERS][-2:], ["orders", "movement"])


class ViolationTest(unittest.TestCase):
    def test_pathfinding_and_no_subsystem_are_allowed(self):
        rows = {"edges": [("pathfinding", "-", "Pathfinder::steps"), ("-", "pathfinding", "Node::data.g"), ("pathfinding", "pathfinding", "pathfinder")],
                "reads": [("-", "pathfinding", "PathExtra::tickLoad")]}
        self.assertEqual(ps.violations(rows), [])

    def test_another_reader_or_writer_is_found(self):
        rows = {"edges": [("pathfinding", "orders", "pathfinder"), ("seams", "pathfinding", "PathExtra::seenWidth")],
                "reads": [("-", "players", ALLOCATION + ".goal")], "presentation": [("-", "presentation", "PathCell::flags")]}
        found = ps.violations(rows)
        self.assertEqual(len(found), 4)
        self.assertTrue(any(line.startswith("edges: reader orders on pathfinder") for line in found))
        self.assertTrue(any(line.startswith("edges: writer seams on PathExtra::seenWidth") for line in found))
        self.assertTrue(any(line.startswith("presentation: reader presentation") for line in found))

    def test_other_items_are_not_judged(self):
        self.assertEqual(ps.violations({"edges": [("orders", "units", "Unit::pos")]}), [])

    def test_a_row_is_reported_once(self):
        rows = {"edges": [("units", "orders", "pathfinder")] * 3}
        self.assertEqual(len(ps.violations(rows)), 2)


class ExposedTest(unittest.TestCase):
    SCHEMA = {"tables": {"Pathfinder": {"fields": [{"name": "pool"}, {"name": "cursor[0]"}, {"name": "cursor[1]"}]},
                         "PathfinderSearch": {"fields": [{"name": "start_x"}]}, "Node": {"fields": [{"name": "g"}]}},
              "globals": {"pathfinder": [{"name": "playerTickLoad"}]}}

    def test_fields_map_to_items(self):
        wanted, problems = ps.exposed(self.SCHEMA)
        self.assertEqual(problems, [])
        self.assertEqual(wanted["Pathfinder::cursor"], ["Pathfinder.cursor[0]", "Pathfinder.cursor[1]"])
        self.assertEqual(wanted["Pathfinder::start.x"], ["PathfinderSearch.start_x"])
        self.assertEqual(wanted["PathExtra::tickLoad"], ["pathfinder.playerTickLoad"])
        self.assertIn("PathCell::flags", wanted)
        self.assertIn(ps.HEAP, wanted)

    def test_a_new_field_is_a_problem(self):
        schema = json.loads(json.dumps(self.SCHEMA))
        schema["tables"]["Node"]["fields"].append({"name": "newField"})
        schema["globals"]["pathfinder"].append({"name": "newGlobal"})
        _, problems = ps.exposed(schema)
        self.assertEqual(len(problems), 2)

    def test_the_stored_schema_is_covered(self):
        _, problems = ps.exposed(json.loads(ps.SCHEMA.read_text()))
        self.assertEqual(problems, [])


class ShadowTest(unittest.TestCase):
    WANTED = {"Pathfinder::steps": ["Pathfinder.steps"], "Node::data.g": ["Node.g"], ps.HEAP: ["heap"]}

    def test_every_item_read_passes(self):
        reads = [{"item": "Pathfinder::steps", "reader": "pathfinding"}, {"item": "Node::data.g", "reader": "pathfinding"},
                 {"item": ALLOCATION + ".heap", "reader": "pathfinding"}]
        self.assertEqual(ps.shadow_problems(self.WANTED, ps.pathfinding_reads(reads), {}), [])

    def test_a_read_by_another_subsystem_does_not_count(self):
        reads = [{"item": "Pathfinder::steps", "reader": "pathfinding"}, {"item": "Node::data.g", "reader": "orders"}]
        found = ps.shadow_problems(self.WANTED, ps.pathfinding_reads(reads), {})
        self.assertEqual(len(found), 2)
        self.assertTrue(found[0].startswith("shadow field AISearch region.heap"))
        self.assertTrue(found[1].startswith("shadow field Node::data.g (Node.g)"))

    def test_an_allowed_item_passes_and_a_stale_row_fails(self):
        read = {"Pathfinder::steps", "Node::data.g"}
        self.assertEqual(ps.shadow_problems(self.WANTED, read, {ps.HEAP: "why"}), [])
        found = ps.shadow_problems(self.WANTED, read | {ps.HEAP}, {ps.HEAP: "why"})
        self.assertEqual(len(found), 1)
        self.assertIn("stale row", found[0])

    def test_an_allow_row_for_an_unknown_item_fails(self):
        found = ps.shadow_problems({}, set(), {"Nope::x": "why"})
        self.assertEqual(len(found), 1)
        self.assertIn("not an exposed item", found[0])


class StoreTest(unittest.TestCase):
    def test_reads_are_sorted_and_gzipped_without_a_time(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, out = Path(tmp) / "src", Path(tmp) / "out"
            source.mkdir()
            (source / "a.reads.csv").write_text('item,reader,count,first_tick\n"Node::data.g",units,3,5\n"A::b",pathfinding,1,2\n')
            self.assertEqual(ps.store(source, out), 1)
            data = (out / "a.reads.csv.gz").read_bytes()
            self.assertEqual(gzip.decompress(data).decode(), 'item,reader,count,first_tick\nA::b,pathfinding,1,2\nNode::data.g,units,3,5\n')
            ps.store(source, out)
            self.assertEqual((out / "a.reads.csv.gz").read_bytes(), data)


if __name__ == "__main__":
    unittest.main()
