"""Tests for tracerehash.py, on hand-built hash inputs and a tiny schema.

    uv run tools/test_tracerehash.py
"""

import json
import lzma
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import tracerehash as tr

SCHEMA = tr.Schema(
    {
        "targets": [{"tag": 1, "name": "heap"}, {"tag": 2, "name": "unit"}],
        "tables": {
            "T": {
                "fields": [
                    {"name": "a", "offset": 0, "size": 4, "kind": "scalar", "target": None},
                    {"name": "", "offset": 4, "size": 2, "kind": "bytes", "target": None},
                    {"name": "p", "offset": 6, "size": 4, "kind": "ref", "target": "unit"},
                    {"name": "m", "offset": 10, "size": 2, "kind": "scalar", "target": None, "mask": 0xFF0},
                ],
                "plan": [
                    {"name": "", "offset": 0, "size": 6, "kind": "bytes", "target": None},
                    {"name": "p", "offset": 6, "size": 4, "kind": "ref", "target": "unit"},
                    {"name": "m", "offset": 10, "size": 2, "kind": "scalar", "target": None, "mask": 0xFF0},
                ],
            }
        },
    }
)


def hashed(*runs):
    h = tr.Hasher()
    for r in runs:
        h.run(r)
    return h.h


class HasherTest(unittest.TestCase):
    def test_empty_is_offset_basis(self):
        self.assertEqual(hashed(), tr.OFFSET_BASIS)

    def test_one_byte(self):
        self.assertEqual(hashed(b"\x01"), ((tr.OFFSET_BASIS ^ 1) * tr.PRIME) & tr.MASK64)

    def test_eight_bytes_is_one_word_step(self):
        w = int.from_bytes(bytes(range(1, 9)), "little")
        self.assertEqual(hashed(bytes(range(1, 9))), ((tr.OFFSET_BASIS ^ w) * tr.PRIME) & tr.MASK64)

    def test_nine_bytes_word_then_byte(self):
        h = hashed(bytes(range(1, 9)))
        self.assertEqual(hashed(bytes(range(1, 10))), ((h ^ 9) * tr.PRIME) & tr.MASK64)

    def test_runs_are_chunked_on_their_own(self):
        self.assertNotEqual(hashed(b"\x01" * 4, b"\x02" * 4), hashed(b"\x01" * 4 + b"\x02" * 4))
        self.assertEqual(hashed(b"\x01" * 8, b"\x02" * 8), hashed(b"\x01" * 8 + b"\x02" * 8))

    def test_h32_is_four_byte_steps(self):
        h = tr.Hasher()
        h.h32(0x04030201)
        self.assertEqual(h.h, hashed(b"\x01", b"\x02", b"\x03", b"\x04"))

    def test_end(self):
        h = tr.Hasher()
        h.end()
        self.assertEqual(h.h, hashed(b"\xff", b"\xff", b"\xff", b"\xff"))

    def test_pointer_is_tag_byte_then_index_word(self):
        h = tr.Hasher()
        h.pointer(2, 5)
        self.assertEqual(h.h, hashed(b"\x02", b"\x05", b"\x00", b"\x00", b"\x00"))


class DumpTest(unittest.TestCase):
    def test_parse_dump_keeps_values_with_spaces(self):
        blocks = tr.parse_dump("bt-dump 4\n7 units a.b str:x y\n7 units a.c 0x01\n", 4)
        self.assertEqual(blocks[(7, "units")], [("a.b", "str:x y"), ("a.c", "0x01")])

    def test_parse_dump_rejects_other_formats(self):
        with self.assertRaises(tr.Error):
            tr.parse_dump("bt-dump 3\n", 4)

    def test_parse_trace_rows(self):
        rows = tr.parse_trace_rows("bt-trace 4\n5 units 0123456789abcdef\n")
        self.assertEqual(rows, {(5, "units"): "0123456789abcdef"})


class WalkerTest(unittest.TestCase):
    def walker(self, lines):
        return tr.Walker(SCHEMA, lines)

    def test_pointer_ids(self):
        w = self.walker([])
        self.assertEqual(w.pointer_id("null"), (0, 0))
        self.assertEqual(w.pointer_id("heap"), (1, 0))
        self.assertEqual(w.pointer_id("rec:overflow"), (9, 0xFFFFFFFF))
        self.assertEqual(w.pointer_id("unit:12"), (2, 12))

    def test_table_merges_bytes_masks_and_ids(self):
        w = self.walker([("t.a", "0x04030201"), ("t.+0x4", "hex:0605"), ("t.p", "unit:3"), ("t.m", "0x0ff0")])
        w.table("T", "t")
        want = tr.Hasher()
        want.run(bytes([1, 2, 3, 4, 6, 5]))
        want.pointer(2, 3)
        want.h32(0x0FF0)
        self.assertEqual(w.hash.h, want.h)
        self.assertEqual(w.pos, 4)

    def test_wrong_path_is_an_error(self):
        w = self.walker([("t.b", "0x00")])
        with self.assertRaises(tr.Error):
            w.take("t.a")

    def test_present_follows_first_field(self):
        w = self.walker([("t.a", "0x00000000")])
        self.assertTrue(w.present("T", "t"))
        self.assertFalse(w.present("T", "u"))


RESULTS = tr.Schema(
    {
        "targets": [{"tag": 1, "name": "heap"}, {"tag": 2, "name": "unit"}],
        "tables": {
            "Pathfinder": {
                "fields": [{"name": "object", "offset": 0, "size": 4, "kind": "pointer", "target": "unit"}],
                "plan": [{"name": "object", "offset": 0, "size": 4, "kind": "pointer", "target": "unit"}],
            }
        },
        "globals": {"pathfinder": [{"name": "counter", "size": 4, "kind": "scalar", "target": None}]},
    }
)

PATH_RESULTS = [
    ("results.count", "0x00000001"),
    ("results[0].kind", "0x00000002"),
    ("results[0].status", "0x00000000"),
    ("results[0].unit", "unit:7"),
    ("results[0].tick", "0x00000018"),
    ("results[0].count", "0x00000002"),
    ("results[0].points", "hex:1800280038004800"),
]

def event_rows(prefix, events):
    rows = [(prefix + ".count", "0x%08x" % len(events))]
    for k, fields in enumerate(events):
        rows += [("%s[%d].%s" % (prefix, k, name), value) for name, value in fields]
    return rows


ECONOMY_EVENTS = event_rows("economy", [
    [("kind", "0x00000001"), ("player", "0x00000000"), ("unit", "unit:7"), ("account", "hex:" + bytes(range(48)).hex()),
     ("outcome", "0x00000001")],
    [("kind", "0x00000002"), ("player", "0x00000001"), ("unit", "null"), ("resources", "0x00000003"),
     ("energy", "0x3f800000"), ("metal", "0x40000000"), ("outcome", "0x00000001")],
    [("kind", "0x00000000"), ("player", "0x00000001"), ("unit", "null"), ("outcome", "0x00000001")],
])

FEATURES_EVENTS = event_rows("features", [
    [("kind", "0x00000020"), ("x", "0x00000004"), ("z", "0x00000005"), ("feature", "0x00000006"),
     ("flags", "0x00000003"), ("owner", "0x0000000a"), ("pos", "hex:" + bytes(range(12)).hex()),
     ("rot", "hex:" + bytes(range(12)).hex()), ("outcome", "0x00000001")],
    [("kind", "0x00000031"), ("state", "heap"), ("feature", "0x00000006"), ("outcome", "0x00000001")],
])

SHARE_PACKETS = [
    ("packets.count", "0x00000001"),
    ("packets[0].type", "0x00000016"),
    ("packets[0].subtype", "0x00000002"),
    ("packets[0].from", "0x00000000"),
    ("packets[0].to", "0x00000001"),
    ("packets[0].amount", "0x00000064"),
]


WIND_METEORS_EVENTS = event_rows("windMeteors", [
    [("kind", "0x00000042"), ("params", "hex:" + bytes(range(48)).hex())],
    [("kind", "0x00000050"), ("weapon", "0x00000001"), ("pos", "hex:" + bytes(12).hex()),
     ("vel", "hex:" + bytes(12).hex()), ("announce", "0x00000000")],
    [("kind", "0x00000043")],
])

TARGETING_EVENTS = event_rows("targeting", [
    [("kind", "0x00000061"), ("unit", "unit:3"), ("weapon", "0x00000001"), ("pos", "hex:" + bytes(range(12)).hex()),
     ("outcome", "0x00000001")],
    [("kind", "0x00000060"), ("unit", "unit:4"), ("weapon", "0x00000000"), ("target", "unit:5"),
     ("outcome", "0x00000001")],
    [("kind", "0x00000070"), ("unit", "unit:4"), ("weapon", "0x00000000"), ("outcome", "0x00000001")],
])

LOS_RADAR_EVENTS = event_rows("los_radar", [
    [("kind", "0x00000080"), ("unit", "unit:3"), ("outcome", "0x00000001")],
    [("kind", "0x00000083"), ("pos", "hex:" + bytes(range(12)).hex()), ("sightDistance", "0x00000140"),
     ("eyeHeight", "0x00000010"), ("lifetime", "0x0000003c"), ("outcome", "0x00000000")],
    [("kind", "0x0000008a"), ("unit", "null"), ("cell", "0x00000001"), ("frameIdx", "0x00000002"),
     ("outcome", "0x00000001")],
])

MAP_EVENTS = event_rows("map", [
    [("kind", "0x000000a0"), ("unit", "unit:3"), ("outcome", "0x00000001")],
    [("kind", "0x000000b0"), ("unit", "null"), ("rect", "hex:" + bytes(range(16)).hex()),
     ("classes", "0x00000005"), ("cause", "0x00000001"), ("outcome", "0x00000001")],
])


PLAYERS_EVENTS = event_rows("playersEvents", [
    [("kind", "0x000000c0"), ("unit", "unit:3"), ("player", "0x00000001"), ("record.present", "0x00000001"),
     ("record.hitPoints", "0x00000002"), ("record.buildPercent", "0x00000003"),
     ("record.angles", "hex:" + bytes(range(12)).hex()), ("record.stockpiles", "hex:" + bytes(12).hex()),
     ("outcome", "0x00000001")],
    [("kind", "0x000000c3"), ("unit", "null"), ("outcome", "0x00000001")],
])

AI_EVENTS = event_rows("aiEvents", [
    [("kind", "0x000000e0"), ("unit", "unit:3"), ("attacker", "unit:4"), ("damage", "0x00000009"),
     ("outcome", "0x00000001")],
    [("kind", "0x000000f0"), ("unit", "unit:3"), ("type", "0x00000001"), ("queue", "0x00000000"), ("target", "null"),
     ("hasPos", "0x00000001"), ("pos", "hex:" + bytes(range(12)).hex()), ("params", "hex:" + bytes(8).hex()),
     ("outcome", "0x00000001")],
])

WEAPONS_EVENTS = event_rows("weaponsEvents", [
    [("kind", "0x00000100"), ("unit", "unit:3"), ("source", "unit:4"), ("amount", "0x00000001"),
     ("damageKind", "0x00000002"), ("direction", "0x00001234"), ("outcome", "0x00000001")],
    [("kind", "0x00000112"), ("unit", "unit:3"), ("outcome", "0x00000001")],
])


class DumpedOnlyTest(unittest.TestCase):
    def test_map_events_after_the_los_radar_events_are_skipped(self):
        lines = PATH_RESULTS + ECONOMY_EVENTS + FEATURES_EVENTS + TARGETING_EVENTS + LOS_RADAR_EVENTS + MAP_EVENTS + SHARE_PACKETS
        w = tr.Walker(RESULTS, lines)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(lines))

    def test_players_events_after_the_map_events_are_skipped(self):
        lines = PATH_RESULTS + ECONOMY_EVENTS + FEATURES_EVENTS + TARGETING_EVENTS + LOS_RADAR_EVENTS + MAP_EVENTS + PLAYERS_EVENTS + SHARE_PACKETS
        w = tr.Walker(RESULTS, lines)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(lines))

    def test_an_empty_players_events_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("playersEvents.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_ai_events_after_the_players_events_are_skipped(self):
        lines = PATH_RESULTS + ECONOMY_EVENTS + FEATURES_EVENTS + TARGETING_EVENTS + LOS_RADAR_EVENTS + MAP_EVENTS + PLAYERS_EVENTS + AI_EVENTS + SHARE_PACKETS
        w = tr.Walker(RESULTS, lines)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(lines))

    def test_an_empty_ai_events_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("aiEvents.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_weapons_events_after_the_ai_events_are_skipped(self):
        lines = (PATH_RESULTS + ECONOMY_EVENTS + FEATURES_EVENTS + TARGETING_EVENTS + LOS_RADAR_EVENTS + MAP_EVENTS
                 + PLAYERS_EVENTS + AI_EVENTS + WEAPONS_EVENTS + SHARE_PACKETS)
        w = tr.Walker(RESULTS, lines)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(lines))

    def test_an_empty_weapons_events_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("weaponsEvents.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_an_empty_orders_and_movement_events_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("ordersEvents.count", "0x00000000"), ("movementEvents.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 2)

    def test_an_empty_units_and_scripts_events_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("unitsEvents.count", "0x00000000"), ("scriptsEvents.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 2)

    def test_an_empty_map_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("map.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_los_radar_events_after_the_targeting_events_are_skipped(self):
        lines = PATH_RESULTS + ECONOMY_EVENTS + FEATURES_EVENTS + TARGETING_EVENTS + LOS_RADAR_EVENTS + SHARE_PACKETS
        w = tr.Walker(RESULTS, lines)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(lines))

    def test_an_empty_los_radar_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("los_radar.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_targeting_events_after_the_features_events_are_skipped(self):
        lines = PATH_RESULTS + ECONOMY_EVENTS + FEATURES_EVENTS + TARGETING_EVENTS + SHARE_PACKETS
        w = tr.Walker(RESULTS, lines)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(lines))

    def test_an_empty_targeting_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("targeting.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_wind_meteors_events_after_the_share_packets_are_skipped(self):
        lines = PATH_RESULTS + ECONOMY_EVENTS + SHARE_PACKETS + WIND_METEORS_EVENTS
        w = tr.Walker(RESULTS, lines)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(lines))

    def test_an_empty_wind_meteors_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("windMeteors.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_path_results_and_economy_events_are_skipped(self):
        w = tr.Walker(RESULTS, PATH_RESULTS + ECONOMY_EVENTS)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(PATH_RESULTS) + len(ECONOMY_EVENTS))

    def test_a_dump_without_economy_events_skips_the_path_results(self):
        w = tr.Walker(RESULTS, PATH_RESULTS)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(PATH_RESULTS))

    def test_share_packets_after_the_economy_events_are_skipped(self):
        lines = PATH_RESULTS + ECONOMY_EVENTS + SHARE_PACKETS
        w = tr.Walker(RESULTS, lines)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(lines))

    def test_features_events_between_the_economy_events_and_the_share_packets_are_skipped(self):
        lines = PATH_RESULTS + ECONOMY_EVENTS + FEATURES_EVENTS + SHARE_PACKETS
        w = tr.Walker(RESULTS, lines)
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, len(lines))

    def test_an_empty_features_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("features.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_an_empty_packet_record_is_skipped(self):
        w = tr.Walker(RESULTS, [("packets.count", "0x00000000")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_skipping_stops_at_a_hashed_line(self):
        w = tr.Walker(RESULTS, [("results.count", "0x00000000"), ("game.x", "0x01")])
        w.skip_dumped(tr.DUMPED_ONLY)
        self.assertEqual(w.pos, 1)

    def test_economy_report_after_the_pathfinder_is_skipped(self):
        w = tr.Walker(RESULTS, [("pathfinder.counter", "0x00000005"), ("economyState.count", "0x00000001"),
                                ("moveClasses[0].x", "0x00000001")])
        w.pathfinder()
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, 2)

    def test_ai_report_after_the_economy_report_is_skipped(self):
        w = tr.Walker(RESULTS, [("pathfinder.counter", "0x00000005"), ("economyState.count", "0x00000001"),
                                ("aiState.commandsEnabled", "0x00000001"), ("aiState.count", "0x00000001"),
                                ("aiState.manager[0].player", "0x00000002"), ("aiState.manager[0].countdown", "0x00000003"),
                                ("aiState.manager[0].nextAction", "0x00000004"),
                                ("aiState.manager[0].timers", "hex:" + bytes(40).hex()),
                                ("moveClasses[0].x", "0x00000001")])
        w.pathfinder()
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, 8)

    def test_map_and_players_reports_after_the_ai_report_are_skipped(self):
        w = tr.Walker(RESULTS, [("pathfinder.counter", "0x00000005"), ("aiState.players[0].ai.owner", "0x00"),
                                ("mapState.spatial.count", "0x00000000"), ("playersState.endGameCountdown", "0x0000"),
                                ("playersState.conditions.present", "0x00"), ("moveClasses[0].x", "0x00000001")])
        w.pathfinder()
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, 5)

    def test_los_radar_index_hashes_section_24_and_skips_the_sight_report(self):
        w = tr.Walker(RESULTS, [("losRadarState.unitSlots", "0x00000000"), ("losRadarState.eyes.count", "0x00000000"),
                                ("losRadarIndex[3].player", "0x00000003"), ("losRadarIndex[3].hasSpecial", "0x00000001"),
                                ("losRadarIndex[3].visible.size", "0x00000001"), ("losRadarIndex[3].visible[0]", "unit:7"),
                                ("losRadarIndex[3].known.size", "0x00000000"),
                                ("unitsFactories[3].player", "0x00000003"), ("unitsFactories[3].factories.size", "0x00000000"),
                                ("players[0].econ.x", "0x01")])
        w.los_radar_index()
        want = tr.Hasher()
        want.h8(24)
        want.h32(3)
        want.h32(1)
        want.h32(1)
        want.pointer(2, 7)
        want.h32(0)
        want.end()
        self.assertEqual(w.hash.h, want.h)
        self.assertEqual(w.pos, 7)
        w.factory_index()
        want.h8(25)
        want.h32(3)
        want.h32(0)
        want.end()
        self.assertEqual(w.hash.h, want.h)
        self.assertEqual(w.pos, 9)

    def test_no_live_player_hashes_nothing(self):
        w = tr.Walker(RESULTS, [("players[0].econ.x", "0x01")])
        w.los_radar_index()
        w.factory_index()
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, 0)

    def test_weapons_report_after_the_ai_report_is_skipped(self):
        w = tr.Walker(RESULTS, [("pathfinder.counter", "0x00000005"), ("aiState.commandsEnabled", "0x00000001"),
                                ("weaponsState.projectiles", "0x00000003"), ("weaponsState.explosions", "0x00000000"),
                                ("weaponsState.debris", "0x00000001"), ("weaponsState.pieces", "0x00000002"),
                                ("moveClasses[0].x", "0x00000001")])
        w.pathfinder()
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, 6)

    def test_pathfinder_report_is_read_but_not_hashed(self):
        w = tr.Walker(RESULTS, [("pathfinder.counter", "0x00000005")])
        w.pathfinder()
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, 1)


def field(name, offset, size, kind="scalar", target=None):
    return {"name": name, "offset": offset, "size": size, "kind": kind, "target": target}


HEAP_SCHEMA = tr.Schema(
    {
        "targets": [{"tag": 1, "name": "heap"}, {"tag": 2, "name": "unit"}],
        "tables": {
            "PathOrderRef": {
                "fields": [field("refOwner", 0, 4, "ref", "unit"), field("refNext", 4, 4, "ref", "unit")],
                "plan": [field("refOwner", 0, 4, "ref", "unit"), field("refNext", 4, 4, "ref", "unit")],
            },
            "Order": {"fields": [field("unit", 0, 4, "ref", "unit")], "plan": [field("unit", 0, 4, "ref", "unit")]},
            "UnitMotion": {"fields": [field("speed", 0, 4)], "plan": [field("", 0, 4, "bytes")]},
            "PieceTransform": {
                "fields": [field("angle", 0, 4)],
                "plan": [field("", 0, 4, "bytes")],
            },
        },
    }
)


class SharedStateTest(unittest.TestCase):
    def test_live_units_come_from_the_units_report(self):
        w = tr.Walker(HEAP_SCHEMA, [("ordersState.units[2].list[0].kind", "0x01"), ("unitsState.units[0].head", "null"),
                                    ("unitsState.units[0].hoverBobPhase", "0x0000"), ("unitsState.units[5].head", "heap")])
        self.assertEqual(w.records(r"unitsState\.units\[(\d+)\]\.head"), [0, 5])

    def test_the_other_module_reports_are_skipped_after_the_spatial_section(self):
        lines = [("pathfinder.counter", "0x00000005"), ("weaponsState.units[0].weapons[0].aimCob.vtable", "null"),
                 ("ordersState.units[0].list[0].kind", "0x01"), ("movementState.units[0].goal.class", "0x00000000"),
                 ("scriptsState.units[0].scale", "0x00000001"), ("moveClasses[0].x", "0x00000001")]
        w = tr.Walker(RESULTS, lines)
        w.pathfinder()
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, 5)

    def test_the_units_report_after_the_factory_index_is_skipped(self):
        w = tr.Walker(RESULTS, [("unitsState.units[0].head", "null"), ("players[0].econ.x", "0x01")])
        w.factory_index()
        self.assertEqual(w.hash.h, tr.OFFSET_BASIS)
        self.assertEqual(w.pos, 1)

    def test_orders_hash_the_index_of_each_live_unit_only(self):
        w = tr.Walker(HEAP_SCHEMA, [])
        w.live = [1, 4]
        w.orders()
        want = tr.Hasher()
        want.h8(20)
        for i in (1, 4):
            want.h32(i)
            want.h8(1)
            want.h8(0)
            want.h8(2)
            want.h8(0)
        want.end()
        self.assertEqual(w.hash.h, want.h)

    def test_motions_hash_no_goal(self):
        w = tr.Walker(HEAP_SCHEMA, [("units[3].motion.goal.class", "0x00000001")])
        w.live = [3]
        w.motions()
        want = tr.Hasher()
        want.h8(21)
        want.h32(3)
        want.h8(0)
        want.end()
        self.assertEqual(w.hash.h, want.h)
        self.assertEqual(w.pos, 0)

    def test_order_effects_hash_only_the_link_of_a_path_order(self):
        lines = [("orderFx[0].class", "0x00000001"), ("orderFx[0].source", "heap"),
                 ("orderFx[1].class", "0x00000004"), ("orderFx[1].source", "heap"), ("orderFx[1].refOwner", "unit:3"),
                 ("orderFx[1].refNext", "null"), ("orderFx[1].pos.x", "0x00000001"),
                 ("orderFx[2].class", "0x00000005"), ("orderFx[2].saveOnly", "0x00")]
        w = tr.Walker(HEAP_SCHEMA, lines)
        w.order_effects()
        want = tr.Hasher()
        want.h8(22)
        want.pointer(2, 3)
        want.pointer(0, 0)
        want.end()
        self.assertEqual(w.hash.h, want.h)
        self.assertEqual(w.pos, len(lines))

    def test_an_unknown_order_effect_still_hashes_its_class(self):
        lines = [("orderFx[0].class", "0x00000000"), ("orderFx[0].vtableDelta", "0x00000010")]
        w = tr.Walker(HEAP_SCHEMA, lines)
        w.order_effects()
        want = tr.Hasher()
        want.h8(22)
        want.h32(0)
        want.h32(0)
        want.h32(0x10)
        want.end()
        self.assertEqual(w.hash.h, want.h)

    def test_an_order_effect_class_above_5_fails(self):
        w = tr.Walker(HEAP_SCHEMA, [("orderFx[0].class", "0x00000006")])
        with self.assertRaisesRegex(tr.Error, "unknown order effect class 6"):
            w.order_effects()

    def test_scripts_hash_only_the_presence_and_the_transforms(self):
        lines = [("units[1].script.transforms[0].angle", "0x00000007"),
                 ("units[1].script.transforms[1].angle", "0x00000008")]
        w = tr.Walker(HEAP_SCHEMA, lines)
        w.live = [0, 1]
        w.scripted = {1}
        w.scripts()
        want = tr.Hasher()
        want.h8(23)
        want.h32(0)
        want.h8(0)
        want.h32(1)
        want.h8(1)
        want.run(bytes([7, 0, 0, 0]))
        want.run(bytes([8, 0, 0, 0]))
        want.end()
        self.assertEqual(w.hash.h, want.h)

    def test_the_phase_row_has_no_explosion_sections(self):
        lines = [("gameTick", "0x00000001"), ("rng.parkMiller", "0x00000002"), ("rng.crtHoldrand", "0x00000003")]
        w = tr.Walker(RESULTS, lines)
        for name in ("units", "projectiles", "players", "features", "wind", "meteors"):
            setattr(w, name, lambda: None)
        w.phase_row(1)
        want = tr.Hasher()
        for v in (1, 2, 3):
            want.h32(v)
        self.assertEqual(w.hash.h, want.h)


class TickRowTest(unittest.TestCase):
    PHASE = [("gameTick", "0x00000001"), ("rng.parkMiller", "0x00000002"), ("rng.crtHoldrand", "0x00000003"),
             ("meteors.x", "0x00000004")]

    @staticmethod
    def block(world):
        return TickRowTest.PHASE + [("game.x", world)]

    def rows(self, blocks):
        def meteors(self):
            self.hash.h32(int(self.take("meteors.x"), 16))

        def game(self):
            self.hash.h32(int(self.take("game.x"), 16))

        quiet = {name: lambda self: None for name in (
            "units", "projectiles", "players", "features", "wind", "cells", "visibility", "explored", "spatial",
            "pathfinder", "move_classes", "orders", "motions", "order_effects", "scripts", "los_radar_index",
            "factory_index", "unit_resources")}
        with mock.patch.multiple(tr.Walker, meteors=meteors, game=game, **quiet):
            return tr.rehash(tr.Schema({"targets": [], "tables": {}}), blocks)

    def test_the_tick_row_is_one_hash_over_the_phase_part_and_the_world_part(self):
        meteors = self.block("0x00000005")
        got = {phase: value for _, phase, value in self.rows({(2, "meteors"): meteors, (2, "tick"): meteors})}
        one = tr.Hasher()
        for v in (1, 2, 3, 4, 5):
            one.h32(v)
        world = tr.Hasher()
        world.h32(5)
        self.assertEqual(got["tick"], one.h)
        self.assertEqual(got["world"], world.h)
        self.assertNotEqual(got["tick"], got["world"])

    def test_a_changed_world_value_changes_only_the_tick_row(self):
        meteors = self.block("0x00000005")
        a = self.rows({(2, "meteors"): meteors, (2, "tick"): meteors})
        b = self.rows({(2, "meteors"): meteors, (2, "tick"): self.block("0x00000006")})
        self.assertEqual(a[:2], b[:2])
        self.assertEqual([r[1] for r in b], ["meteors", "world", "tick"])
        self.assertNotEqual(a[2], b[2])


class TailWriteTest(unittest.TestCase):
    """A write in the tail of the step (after `meteors`) is seen by the `tick` row of its own tick and by no other row."""

    FIXTURE = Path(__file__).resolve().parent / "testdata/tracerehash/orders-build-patrol-24"

    @classmethod
    def setUpClass(cls):
        schema_path = Path(__file__).resolve().parent.parent / "data/trace_schema.json"
        cls.schema = tr.Schema(json.loads(schema_path.read_text()))
        cls.blocks = tr.parse_dump(tr.read_text(f"{cls.FIXTURE}.dump.txt.xz"), cls.schema.data["trace_format"])
        cls.trace = tr.parse_trace_rows(tr.read_text(f"{cls.FIXTURE}.trace.txt"))

    def rows_with_tick_block(self, change):
        blocks = dict(self.blocks)
        blocks[(24, "tick")] = [change(path, value) for path, value in self.blocks[(24, "tick")]]
        return {phase: f"{value:016x}" for _, phase, value in tr.rehash(self.schema, blocks)}

    def assert_only_the_tick_row_changes(self, got):
        for phase in ("units", "meteors", "world"):
            self.assertEqual(got[phase], self.trace[(24, phase)])
        self.assertNotEqual(got["tick"], self.trace[(24, "tick")])

    def test_the_rows_keep_the_row_order_whatever_the_block_order(self):
        reversed_blocks = dict(reversed(list(self.blocks.items())))
        self.assertEqual([phase for _, phase, _ in tr.rehash(self.schema, reversed_blocks)],
                         ["units", "projectiles", "explosions", "players", "features", "wind", "meteors", "world", "tick"])

    def test_the_unchanged_dump_gives_the_stored_tick_row(self):
        self.assertEqual(self.rows_with_tick_block(lambda path, value: (path, value))["tick"], self.trace[(24, "tick")])

    def test_an_expired_eyeball_changes_the_tick_row(self):
        cleared = next(path for path, value in self.blocks[(24, "tick")]
                       if path.startswith("visibility[") and value.startswith("hex:") and value.strip("hex:0"))
        got = self.rows_with_tick_block(lambda path, value: (path, "hex:" + "00" * ((len(value) - 4) // 2)) if path == cleared else (path, value))
        self.assert_only_the_tick_row_changes(got)

    def test_a_shared_resource_changes_the_tick_row(self):
        got = self.rows_with_tick_block(lambda path, value: (path, "0x00000000") if path == "players[0].metal" else (path, value))
        self.assert_only_the_tick_row_changes(got)


class SelectionTest(unittest.TestCase):
    """The selection (bit 0x10 of a unit's flags), its visit mark (bits 6 and 7) and the debug bits (bits 0 and 1 of endGameFlags) are view state: setting them between two ticks changes no row (#1050, #1132)."""

    FIXTURE = Path(__file__).resolve().parent / "testdata/tracerehash/orders-build-patrol-24"

    @classmethod
    def setUpClass(cls):
        schema_path = Path(__file__).resolve().parent.parent / "data/trace_schema.json"
        cls.schema = tr.Schema(json.loads(schema_path.read_text()))
        cls.blocks = tr.parse_dump(tr.read_text(f"{cls.FIXTURE}.dump.txt.xz"), cls.schema.data["trace_format"])
        cls.trace = tr.parse_trace_rows(tr.read_text(f"{cls.FIXTURE}.trace.txt"))

    def rows_with_flags(self, bit, field="units[1].flags"):
        def change(path, value):
            return (path, "0x%0*x" % (len(value) - 2, int(value, 16) ^ bit)) if path == field else (path, value)
        blocks = {key: [change(path, value) for path, value in lines] for key, lines in self.blocks.items()}
        return {phase: f"{value:016x}" for _, phase, value in tr.rehash(self.schema, blocks)}

    def test_the_selection_bit_changes_no_row(self):
        self.assertEqual(int(dict(self.blocks[(24, "tick")])["units[1].flags"], 16) & 0x10, 0)
        got = self.rows_with_flags(0x10)
        for phase, value in got.items():
            self.assertEqual(value, self.trace[(24, phase)], phase)

    def test_the_visit_mark_changes_no_row(self):
        self.assertEqual(int(dict(self.blocks[(24, "tick")])["units[1].flags"], 16) & 0xC0, 0)
        for bit in (0x40, 0x80, 0xC0):
            got = self.rows_with_flags(bit)
            for phase, value in got.items():
                self.assertEqual(value, self.trace[(24, phase)], (bit, phase))

    def test_the_debug_bits_change_no_row(self):
        self.assertEqual(int(dict(self.blocks[(24, "tick")])["game.endGameFlags"], 16) & 3, 0)
        for bit in (1, 2):
            got = self.rows_with_flags(bit, "game.endGameFlags")
            for phase, value in got.items():
                self.assertEqual(value, self.trace[(24, phase)], (bit, phase))

    def test_the_game_over_bit_changes_the_rows(self):
        got = self.rows_with_flags(4, "game.endGameFlags")
        self.assertNotEqual(got["tick"], self.trace[(24, "tick")])

    def test_the_selectable_bit_changes_the_rows(self):
        got = self.rows_with_flags(0x20)
        for phase in ("units", "tick"):
            self.assertNotEqual(got[phase], self.trace[(24, phase)], phase)


class ReadTextTest(unittest.TestCase):
    def test_xz_path_is_decompressed(self):
        with tempfile.TemporaryDirectory() as d:
            plain, packed = Path(d, "a.txt"), Path(d, "a.txt.xz")
            plain.write_text("1 units 0123456789abcdef\n")
            with lzma.open(packed, "wt") as f:
                f.write("1 units 0123456789abcdef\n")
            self.assertEqual(tr.read_text(str(packed)), tr.read_text(str(plain)))


class ViewRowTest(unittest.TestCase):
    def test_the_view_row_is_the_hash_of_the_view_file_of_the_tick(self):
        import test_viewblock as tv
        import viewblock
        data = tv.header(tick=7) + tv.players()
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / "7.bin").write_bytes(viewblock.FILE_HEADER + data)
            self.assertEqual(tr.view_rows(Path(d), [6, 7]), [(7, "view", viewblock.view_hash(data))])
            (Path(d) / "6.bin").write_bytes(b"BTFRxxxxxxxx")
            with self.assertRaisesRegex(tr.Error, "6.bin: not a view file"):
                tr.view_rows(Path(d), [6])


def one_field_table(name):
    field = {"name": name, "offset": 0, "size": 1, "kind": "scalar", "target": None}
    return {"fields": [field], "plan": [{"name": "", "offset": 0, "size": 1, "kind": "bytes", "target": None}]}


DEFINITIONS = tr.Schema(
    {
        "targets": [],
        "tables": {
            "UnitDefData": one_field_table("u"),
            "WeaponDefData": one_field_table("w"),
            "MissionData": one_field_table("type"),
            "MissionStartRest": one_field_table("rest"),
        },
    }
)


def expected(steps):
    h = tr.Hasher()
    for kind, value in steps:
        if kind == "8":
            h.h8(value)
        elif kind == "32":
            h.h32(value)
        else:
            h.run(value)
    return h.h


class DefinitionSectionsTest(unittest.TestCase):
    def run_section(self, method, lines):
        w = tr.Walker(DEFINITIONS, lines)
        getattr(w, method)()
        self.assertEqual(w.pos, len(lines))
        return w.hash.h

    def test_string_null_and_text(self):
        w = tr.Walker(DEFINITIONS, [("a", "null"), ("b", "str:hi there")])
        w.string("a")
        w.string("b")
        self.assertEqual(w.hash.h, expected([("8", 0), ("8", 1), ("32", 8), ("b", b"hi there")]))

    def test_string_empty(self):
        w = tr.Walker(DEFINITIONS, [("a", "str:")])
        w.string("a")
        self.assertEqual(w.hash.h, expected([("8", 1), ("32", 0)]))

    def test_unit_definitions(self):
        lines = [
            ("defs.count", "0x00000001"),
            ("defs[0].u", "0x07"),
            ("defs[0].yardmap", "0x00000002"),
            ("defs[0].yardmap", "hex:0102"),
            ("defs[0].ids", "0x00000000"),
            ("defs[0].ids", "hex:"),
            ("defs[0].categories[1]", "hex:" + "aa" * 0x40),
            ("defs[0].categories[3]", "hex:" + "bb" * 0x40),
        ]
        got = self.run_section("unit_definitions", lines)
        want = expected(
            [("8", 29), ("32", 1), ("b", b"\x07"), ("32", 2), ("b", b"\x01\x02"), ("32", 0), ("b", b"")]
            + [("8", 0), ("8", 1), ("b", b"\xaa" * 0x40), ("8", 0), ("8", 1), ("b", b"\xbb" * 0x40), ("32", 0xFFFFFFFF)]
        )
        self.assertEqual(got, want)

    def test_weapon_definitions(self):
        lines = []
        steps = [("8", 30)]
        for i in range(256):
            lines.append((f"weapons[{i}].w", "0x%02x" % (i & 0xFF)))
            steps.append(("b", bytes([i & 0xFF])))
            if i == 3:
                lines += [
                    ("weapons[3].damage.entries", "0x00000002"),
                    ("weapons[3].damage[0].name", "str:ARMCOM"),
                    ("weapons[3].damage[0].value", "0x0000000a"),
                    ("weapons[3].damage[1].name", "null"),
                    ("weapons[3].damage[1].value", "0x00000001"),
                ]
                steps += [("8", 1), ("32", 2), ("8", 1), ("32", 6), ("b", b"ARMCOM"), ("32", 10), ("8", 0), ("32", 1)]
            else:
                steps.append(("8", 0))
        steps.append(("32", 0xFFFFFFFF))
        self.assertEqual(self.run_section("weapon_definitions", lines), expected(steps))

    def test_mission_absent(self):
        self.assertEqual(self.run_section("mission", []), expected([("8", 31), ("8", 0), ("32", 0xFFFFFFFF)]))

    def test_mission_campaign(self):
        lines = [
            ("mission.type", "0x01"),
            ("mission.rest", "0x09"),
            ("mission.units.count", "0x00000001"),
            ("mission.units[0].name", "str:ARMCOM"),
            ("mission.units[0].ident", "null"),
            ("mission.units[0].initialMission", "str:x"),
            ("mission.units[0]", "hex:" + "01" * 0x18),
            ("mission.rules.count", "0x00000001"),
            ("mission.rules", "hex:" + "02" * 12),
            ("mission.features.count", "0x00000001"),
            ("mission.features[0].name", "str:tree"),
            ("mission.features[0].pos", "hex:0102030405060708"),
        ]
        want = expected(
            [("8", 31), ("8", 1), ("b", b"\x01"), ("b", b"\x09"), ("32", 1)]
            + [("8", 1), ("32", 6), ("b", b"ARMCOM"), ("8", 0), ("8", 1), ("32", 1), ("b", b"x")]
            + [("b", b"\x01" * 0x18), ("32", 1), ("b", b"\x02" * 12), ("32", 1)]
            + [("8", 1), ("32", 4), ("b", b"tree"), ("b", bytes(range(1, 9))), ("32", 0xFFFFFFFF)]
        )
        self.assertEqual(self.run_section("mission", lines), want)


if __name__ == "__main__":
    unittest.main()
