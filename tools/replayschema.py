"""Generate src/harness/replay_schema.h and data/replay_schema.json, the field tables of the seam call recorder.

    uv run tools/replayschema.py          # rewrite both files; a second run leaves them unchanged
    uv run tools/replayschema.py --check  # exit 1 when either committed file is out of date, or a field is unaccounted for

For each slot of the wind_meteors seam the tables list the fields of the block the call can read
and write, from the rows of data/rwsets.csv for the functions behind the slot. A field is a member
of `Game` (offset from the layout the trace schema reads) or of `SimGlobals`, or one of the three
random generator words. A row that names a member of `Game` or a `g_` global that is in neither
table nor in EXEMPT is an error, so a change to what the module touches shows up here.

Reads are field_read, global_read and global_maybe_written rows; writes are field_write,
global_write and global_maybe_written rows. rwsets is flow-insensitive, so a read may follow a
write of the same function; the recorder writes only values that differ from the last known one.
"""

import argparse
import csv
import json
import sys
from pathlib import Path

import traceschema
from rwsets import load_sim_map

ROOT = Path(__file__).resolve().parent.parent
RWSETS = ROOT / "data/rwsets.csv"
OUT = ROOT / "src/harness/replay_schema.h"
JSON_OUT = ROOT / "data/replay_schema.json"
SIM_GLOBALS_H = ROOT / "src/state/sim_globals.h"
FORMAT = 1

SLOT_FUNCTIONS: dict[str, list[str]] = {
    "UpdateWind": ["UpdateWind", "UpdateWindOnStream"],
    "UpdateMeteors": ["UpdateMeteors", "StartMeteorShower"],
    "InitMeteors": ["InitMeteors"],
    "EmptyShutdownPreCleanup": ["EmptyShutdownPreCleanup"],
    "EnableMeteors": ["EnableMeteors"],
    "DisableMeteors": ["DisableMeteors"],
    "SetMeteorParams": ["SetMeteorParams"],
    "LoadMeteorDefaults": ["MeteorParams::LoadMeteorDefaults"],
    "SaveMeteors": ["SaveMeteors"],
    "LoadMeteors": ["LoadMeteors"],
    "CmdMeteor": ["CmdMeteor", "StartMeteorShower", "EnableMeteors", "DisableMeteors"],
    "UpdateWindOnStream": ["UpdateWindOnStream"],
}

GAME_FIELDS: dict[str, str] = {
    "ticks": "i32", "width": "i32", "height": "i32", "windCounter": "u32", "windSpeedDenominator": "i32",
    "windX": "i32", "windY": "i32", "windZ": "i32", "windDirection": "u16", "windSpeed": "i32",
    "windStrength": "f32", "windEnabled": "i32", "windSpeedMin": "i32", "windSpeedMax": "i32",
}

SIM_FIELDS: dict[str, tuple[str, int]] = {
    "g_meteorNextStrikeTime": ("i32", 4), "g_meteorWeaponName": ("string", 32), "g_meteorSpawnMagnitude": ("i32", 4),
    "g_meteorSpawnInterval": ("i32", 4), "g_meteorActive": ("i32", 4), "g_meteorStrikeEndTime": ("i32", 4),
    "g_meteorOrigin": ("point16", 4), "g_meteorShowerDuration": ("i32", 4), "g_meteorWeapon": ("weapon", 4),
    "g_meteorsEnabled": ("i32", 4), "g_meteorNextHitTime": ("i32", 4), "g_meteorTarget": ("point16", 4),
    "g_meteorScheduleGap": ("i32", 4),
}

RNG_FIELDS = ["rng.main", "rng.load", "rng.seed"]
RNG_CALLS = {"GameRand", "RandomInt"}

SIZES = {"i32": 4, "u32": 4, "u16": 2, "f32": 4}

EXEMPT: dict[str, str] = {
    "Game::weapons": "the weapon the recorder resolves through g_meteorWeapon and the import results",
    "g_game": "the pointer to the game record, not state",
    "g_seams": "the seam table, not state",
    "g_extTdf": "a constant string passed to an import",
    "DAT_005119b8": "a constant empty string passed to an import",
}

BLOCK_KINDS = {"field_read": "r", "field_write": "w", "global_read": "r", "global_write": "w", "global_maybe_written": "rw"}


def read_rows(path: Path = RWSETS) -> list[dict[str, str]]:
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def tracked(target: str) -> str | None:
    """The recorder's name for an rwsets target, or None when the target is no block field of the module."""
    if target.startswith("Game::") and target[6:] in GAME_FIELDS:
        return "Game." + target[6:]
    if target in SIM_FIELDS:
        return "SimGlobals." + target
    return None


def unaccounted(rows: list[dict[str, str]]) -> list[str]:
    """Targets of the module's rows that name a Game member or a g_ global and are in no table."""
    module = {fn for fns in SLOT_FUNCTIONS.values() for fn in fns}
    found = set()
    for row in rows:
        if row["function"] not in module or row["kind"] not in BLOCK_KINDS:
            continue
        target = row["target"]
        if tracked(target) is not None or target in EXEMPT:
            continue
        if target.startswith("Game::") or target.startswith("g_"):
            found.add(target)
    return sorted(found)


def slot_sets(rows: list[dict[str, str]]) -> dict[str, tuple[list[str], list[str]]]:
    """Per slot, the (reads, writes) field names, in table order."""
    order = ["Game." + n for n in GAME_FIELDS] + ["SimGlobals." + n for n in SIM_FIELDS] + RNG_FIELDS
    out = {}
    for slot, functions in SLOT_FUNCTIONS.items():
        reads, writes = set(), set()
        draws = False
        for row in rows:
            if row["function"] not in functions:
                continue
            if row["kind"] == "call" and row["target"] in RNG_CALLS:
                draws = True
            name = tracked(row["target"]) if row["kind"] in BLOCK_KINDS else None
            if name is None:
                continue
            access = BLOCK_KINDS[row["kind"]]
            if "r" in access:
                reads.add(name)
            if "w" in access:
                writes.add(name)
        if draws:
            reads.update(RNG_FIELDS)
            writes.update(RNG_FIELDS)
        out[slot] = ([f for f in order if f in reads], [f for f in order if f in writes])
    return out


def check_sim_names() -> list[str]:
    text = SIM_GLOBALS_H.read_text()
    sim = load_sim_map()
    errors = []
    for name in SIM_FIELDS:
        if name + ";" not in text and name not in sim.fields:
            errors.append("SimGlobals has no member %s" % name)
    return errors


def field_table() -> list[dict]:
    schema = traceschema.Schema()
    layout = {l["name"]: l for l in schema.layout("Game")[0]}
    fields = []
    for name, kind in GAME_FIELDS.items():
        leaf = layout.get(name)
        if leaf is None or leaf["size"] != SIZES[kind]:
            raise SystemExit("replayschema: Game has no field %s of %d bytes" % (name, SIZES[kind]))
        fields.append({"name": "Game." + name, "base": "game", "offset": leaf["offset"], "size": leaf["size"], "kind": kind})
    for name, (kind, size) in SIM_FIELDS.items():
        fields.append({"name": "SimGlobals." + name, "base": "sim", "offset": name, "size": size, "kind": kind})
    for i, name in enumerate(RNG_FIELDS):
        fields.append({"name": name, "base": "rng", "offset": i, "size": 4, "kind": "u32"})
    return fields


def generate(rows: list[dict[str, str]]) -> tuple[str, str, list[str]]:
    errors = ["rwsets names %s, which the recorder does not track" % t for t in unaccounted(rows)] + check_sim_names()
    fields = field_table()
    index = {f["name"]: i for i, f in enumerate(fields)}
    sets = slot_sets(rows)
    kinds = ["i32", "u32", "u16", "f32", "point16", "string", "weapon"]
    lines = [
        "// Generated by tools/replayschema.py from data/rwsets.csv and the layout of Game. Do not edit.",
        "#ifndef REPLAY_SCHEMA_H",
        "#define REPLAY_SCHEMA_H",
        "",
        "#define REPLAY_FORMAT %d" % FORMAT,
        "",
        "enum ReplayBase { RB_GAME, RB_SIM, RB_RNG };",
        "enum ReplayKind { RK_I32, RK_U32, RK_U16, RK_F32, RK_POINT16, RK_STRING, RK_WEAPON };",
        "",
        "struct ReplayField {",
        "    const char* name;",
        "    unsigned char base;",
        "    unsigned offset;",
        "    unsigned size;",
        "    unsigned char kind;",
        "};",
        "",
        "struct ReplaySlotSets {",
        "    const char* name;",
        "    const unsigned char* reads;",
        "    unsigned readCount;",
        "    const unsigned char* writes;",
        "    unsigned writeCount;",
        "};",
        "",
    ]
    for name in SIM_FIELDS:
        lines += ['#pragma push_macro("%s")' % name, "#undef %s" % name]
    lines += ["", "static const ReplayField kReplayFields[] = {"]
    for f in fields:
        base = {"game": "RB_GAME", "sim": "RB_SIM", "rng": "RB_RNG"}[f["base"]]
        offset = "offsetof(SimGlobals, %s)" % f["offset"] if f["base"] == "sim" else "%d" % f["offset"]
        lines.append('    { "%s", %s, %s, %d, RK_%s },' % (f["name"], base, offset, f["size"], f["kind"].upper()))
    lines += ["};", "static const unsigned kReplayFieldCount = %d;" % len(fields), ""]
    for name in SIM_FIELDS:
        lines.append('#pragma pop_macro("%s")' % name)
    lines.append("")
    lines.append("enum ReplayFieldIndex {")
    for f in fields:
        lines.append("    RF_%s," % f["name"].replace(".", "_").upper())
    lines += ["};", ""]
    for slot, (reads, writes) in sets.items():
        for tag, names in (("Reads", reads), ("Writes", writes)):
            if names:
                lines.append("static const unsigned char kReplay%s%s[] = { %s };" % (slot, tag, ", ".join(str(index[n]) for n in names)))
    lines += ["", "static const ReplaySlotSets kReplaySlots[] = {"]
    for slot, (reads, writes) in sets.items():
        r = "kReplay%sReads" % slot if reads else "0"
        w = "kReplay%sWrites" % slot if writes else "0"
        lines.append('    { "%s", %s, %d, %s, %d },' % (slot, r, len(reads), w, len(writes)))
    lines += ["};", "static const unsigned kReplaySlotCount = %d;" % len(sets), "", "#endif", ""]
    model = {
        "replay_format": FORMAT,
        "fields": [{"name": f["name"], "base": f["base"], "size": f["size"], "kind": f["kind"]} for f in fields],
        "slots": {slot: {"reads": r, "writes": w} for slot, (r, w) in sets.items()},
    }
    assert set(kinds) >= {f["kind"] for f in fields}
    return "\n".join(lines), json.dumps(model, indent=1) + "\n", errors


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    text, schema_json, errors = generate(read_rows())
    for e in errors:
        print("replayschema: " + e, file=sys.stderr)
    files = [(OUT, text), (JSON_OUT, schema_json)]
    if args.check:
        stale = [path for path, want in files if not path.exists() or path.read_text() != want]
        for path in stale:
            print("stale %s" % path.relative_to(ROOT), file=sys.stderr)
        sys.exit(1 if stale or errors else 0)
    if errors:
        sys.exit(1)
    for path, want in files:
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists() or path.read_text() != want:
            path.write_text(want, newline="\n")
            print("wrote %s" % path.relative_to(ROOT))
        else:
            print("unchanged %s" % path.relative_to(ROOT))


if __name__ == "__main__":
    main()
