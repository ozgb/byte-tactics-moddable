"""Tests for tracerehash.py, on hand-built hash inputs and a tiny schema.

    uv run tools/test_tracerehash.py
"""

import lzma
import tempfile
import unittest
from pathlib import Path

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
    [("kind", "0x000000f1"), ("unit", "unit:3"), ("type", "0x00000001"), ("queue", "0x00000000"), ("target", "null"),
     ("hasPos", "0x00000001"), ("pos", "hex:" + bytes(range(12)).hex()), ("params", "hex:" + bytes(8).hex()),
     ("outcome", "0x00000001")],
])

WEAPONS_EVENTS = event_rows("weaponsEvents", [
    [("kind", "0x00000100"), ("unit", "unit:3"), ("source", "unit:4"), ("amount", "0x00000001"),
     ("damageKind", "0x00000002"), ("direction", "0x00001234"), ("outcome", "0x00000001")],
    [("kind", "0x00000115"), ("unit", "unit:3"), ("outcome", "0x00000001")],
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


class ReadTextTest(unittest.TestCase):
    def test_xz_path_is_decompressed(self):
        with tempfile.TemporaryDirectory() as d:
            plain, packed = Path(d, "a.txt"), Path(d, "a.txt.xz")
            plain.write_text("1 units 0123456789abcdef\n")
            with lzma.open(packed, "wt") as f:
                f.write("1 units 0123456789abcdef\n")
            self.assertEqual(tr.read_text(str(packed)), tr.read_text(str(plain)))


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
