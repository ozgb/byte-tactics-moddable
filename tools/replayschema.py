"""Generate the tables and hooks of the seam call recorder and replayer (bt-replay 2, docs/replay-format.md).

    uv run tools/replayschema.py          # rewrite the generated files; a second run leaves them unchanged
    uv run tools/replayschema.py --check  # exit 1 when a committed file is out of date, or an access is unaccounted for

Inputs: interfaces/<subsystem>.h of every simulation subsystem (the members of `Seams`), data/subsystems.csv (which
functions are behind a slot), data/rwsets.csv (what they read and write), the layout of `Game`, and the `annotations`
key of data/replay_schema.json, which holds what no header says: the tracked block fields, the object kinds and their
fields, the hand rules, and per subsystem the slot annotations (functions, how each pointer argument is recorded).

Outputs:

    data/replay_schema.json         the annotations (kept as written) and the generated fields, objects and subsystems
    src/harness/replay_schema.h     the field, object and slot tables
    tests/replay_schema_fixtures.h  the same tables with the test-only kinds of tests/replay_fixtures.json added, which replay_schema.h
                                    includes when BT_REPLAY_FIXTURES is defined (only the tests' build defines it)
    tests/replay_vtables_fixtures.cpp the vtable of each concrete kind of that schema
    tests/replay_shapes.h             the import stubs and the runners of the fixture slots of tests/replay_fixtures.json
    tests/replay_fixture_hooks.inc    the recording wrappers of those slots, compiled by tests/replay_fixture_recorder.cpp
    src/harness/replay_hooks.inc    the recording copy of every seam table, with the argument and import hooks
    tests/replay_slots.h            the slot runners, import stubs and native seam tables of the replayer
    src/harness/replay_services.h   the ids and names of the services imports (GameRand, RandomInt, the TDF and command classes)
    src/harness/replay_services.inc the recorder's lines for them
    tests/replay_services.h         the replayer's answers for them
    src/harness/replay_vtables.cpp  the vtable of each concrete kind of a polymorphic base
    src/harness/replay_callbacks.h  the recorder's thunk classes for the callback interfaces
    tests/replay_callbacks.h        the replayer's players for the callbacks an import made, and its classes for the callbacks a module holds

A slot's reads are field_read, global_read and global_maybe_written rows; its writes are field_write, global_write and
global_maybe_written rows, over the functions behind the slot. A row names a block field (Game member or SimGlobals
global), an object field (`Type::member` of an object kind) or something else. A `Game::` or `g_` target that is
tracked by no table and is not exempt is an error for a complete subsystem; a pending subsystem lists what is left
under `pending`, which its own issue empties. A deref_read or deref_write on a bare scalar pointer and every
field_escape row needs a hand rule in `annotations.rules`; a rule that matches no row is an error.
A field row on a type that is no kind, and an indirect_call row nothing accounts for, are errors for a complete subsystem too
(docs/replay-format.md, "The schema and its annotations": `exempt_types`, `through`).
A slot annotated `"record": false` with a `"reason"` is classified and unrecorded: it gets no recording slot and none of the checks above.
"""

import argparse
import copy
import csv
import functools
import json
import os
import re
import sys
from pathlib import Path

import replay_subsystems
import traceschema
from rwsets import load_sim_map

ROOT = Path(__file__).resolve().parent.parent
RWSETS = ROOT / "data/rwsets.csv"
SUBSYSTEMS = ROOT / "data/subsystems.csv"
INTERFACES = ROOT / "interfaces"
SCHEMA_H = ROOT / "src/harness/replay_schema.h"
HOOKS_H = ROOT / "src/harness/replay_hooks.inc"
SLOTS_H = ROOT / "tests/replay_slots.h"
JSON_OUT = ROOT / "data/replay_schema.json"
FIXTURES_JSON = ROOT / "tests/replay_fixtures.json"
FIXTURE_SCHEMA_H = ROOT / "tests/replay_schema_fixtures.h"
FIXTURE_VTABLES_CPP = ROOT / "tests/replay_vtables_fixtures.cpp"
FIXTURE_HOOKS_INC = ROOT / "tests/replay_fixture_hooks.inc"
SIM_GLOBALS_H = ROOT / "src/state/sim_globals.h"
FORMAT = 2

RNG_FIELDS = ["rng.main", "rng.load", "rng.seed"]
RNG_CALLS = {"GameRand", "RandomInt"}
SIZES = {"i32": 4, "u32": 4, "u16": 2, "f32": 4, "u8": 1, "i16": 2, "f64": 8}
BLOCK_KINDS = {"field_read": "r", "field_write": "w", "global_read": "r", "global_write": "w", "global_maybe_written": "rw"}
OBJECT_KINDS = {"field_read": "r", "field_write": "w", "field_maybe_written": "rw"}
FIELD_KINDS = {"i32": "I32", "u32": "U32", "u16": "U16", "i16": "I16", "f32": "F32", "point16": "POINT16", "string": "STRING",
               "weapon": "WEAPON", "ref": "REF", "flags": "FLAGS", "u8": "U8", "f64": "F64", "bytes": "BYTES", "embed": "EMBED",
               "unitid": "UNITID", "array": "ARRAY", "code": "CODE", "vector": "VECTOR", "ptr": "PTR", "refs": "REFS", "embeds": "EMBEDS",
               "self": "SELF", "selfindex": "SELFINDEX", "cbref": "CBREF", "cbptr": "CBPTR"}
INTEGER_ARGS = {"int", "unsigned", "unsigned int", "long", "unsigned long", "short", "unsigned short", "char", "unsigned char"}
FACTOR = re.compile(r"^\(?\s*(\w+)\s*(?:\+\s*(0[xX][0-9a-fA-F]+|\d+))?\s*\)?\s*(?:>>\s*(\d+))?\s*\)?$")
SCALARS = {"int", "unsigned", "unsigned int", "long", "unsigned long", "short", "unsigned short", "char", "unsigned char",
           "bool", "float", "double", "Point16", "Vec3", "RandStream"}
BARE_POINTER = re.compile(r"^\*(float|double|short|int|long|unsigned short|unsigned int|unsigned char|char)$")
SLOT = re.compile(r"([\w:<>\*\s&,]+?)\s*\(\s*(__\w+)\s*\*\s*(\w+)\s*\)\s*\(([^;]*?)\)\s*;")
TYPE_WORDS = {"unsigned", "char", "short", "int", "long", "float", "double", "bool"}


class Error(Exception):
    pass


def read_rows(path: Path = RWSETS) -> list[dict[str, str]]:
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def read_assignment(path: Path = SUBSYSTEMS) -> dict[str, list[str]]:
    """Subsystem -> its functions, from data/subsystems.csv."""
    out: dict[str, list[str]] = {}
    with path.open(newline="") as f:
        for r in csv.DictReader(f):
            out.setdefault(r["subsystem"], []).append(r["function"])
    return out


def merge_fixtures(ann: dict, fixtures: dict) -> dict:
    """The annotations with the test-only kinds of tests/replay_fixtures.json added: its includes, game fields and kinds come last, `inside` adds sites to a kind, and its `fixtures` are the slots the replayer's self-tests call."""
    merged = copy.deepcopy(ann)
    merged["includes"] += [i for i in fixtures.get("includes", []) if i not in merged["includes"]]
    merged["game_fields"].update(fixtures.get("game_fields", {}))
    merged["objects"].update(copy.deepcopy(fixtures.get("objects", {})))
    merged["fixtures"] = copy.deepcopy(fixtures.get("fixtures", {}))
    for kind, sites in fixtures.get("inside", {}).items():
        have = merged["objects"][kind].setdefault("inside", [])
        have += [x for x in sites if x not in have]
    return merged


def load_annotations(fixtures: bool = False) -> dict:
    """The annotations of data/replay_schema.json; with `fixtures`, those of the schema that only the tests load as well."""
    ann = json.loads(JSON_OUT.read_text())["annotations"]
    return merge_fixtures(ann, json.loads(FIXTURES_JSON.read_text())) if fixtures else ann


def seam_members() -> list[tuple[str, str]]:
    """(subsystem, struct name) of every member of `Seams`, in order."""
    text = (INTERFACES / "seams.h").read_text()
    body = re.search(r"struct Seams \{(.*?)\};", text, re.S).group(1)
    return [(m.group(2), m.group(1)) for m in re.finditer(r"(\w+Seam)\* (\w+);", body)]


def normalise(ctype: str) -> str:
    t = re.sub(r"\bconst\b", "", ctype)
    t = re.sub(r"\s*\*\s*", "*", t)
    return " ".join(t.split())


def split_params(text: str) -> list[str]:
    text = text.strip()
    if text in ("", "void"):
        return []
    out, depth, cur = [], 0, ""
    for c in text:
        if c in "<(":
            depth += 1
        if c in ">)":
            depth -= 1
        if c == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += c
    out.append(cur)
    return [p.strip() for p in out]


def parse_interface(subsystem: str, struct: str) -> dict:
    """The version and slots of interfaces/<subsystem>.h."""
    path = INTERFACES / (subsystem + ".h")
    text = path.read_text()
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    prefix = subsystem.upper()
    major = int(re.search(r"#define %s_VERSION_MAJOR (\d+)" % prefix, text).group(1))
    minor = int(re.search(r"#define %s_VERSION_MINOR (\d+)" % prefix, text).group(1))
    body = re.search(r"struct %s \{(.*?)\n\};" % struct, text, re.S).group(1)
    slots = []
    for m in SLOT.finditer(body):
        params = []
        for i, p in enumerate(split_params(m.group(4))):
            n = re.match(r"^(.*?[\s\*&>])(\w+)$", p)
            if n and n.group(2) not in TYPE_WORDS:
                ctype, name = n.group(1).strip(), n.group(2)
            else:
                ctype, name = p, "a%d" % i
            params.append({"type": ctype, "name": name})
        slots.append({"name": m.group(3), "conv": m.group(2), "ret": " ".join(m.group(1).split()), "params": params})
    return {"version": [major, minor], "struct": struct, "slots": slots}


def block_tables(ann: dict):
    game = ann["game_fields"]
    sim = {k: (v if isinstance(v, dict) else tuple(v)) for k, v in ann["sim_fields"].items()}
    return game, sim


def parse_addend(text: str | None) -> int:
    """The integer of a `+k` in a length factor, decimal or 0x hexadecimal."""
    if not text:
        return 0
    return int(text, 16) if text.lower().startswith("0x") else int(text)


def parse_length(expr, where: str, errors: list[str]) -> list[dict]:
    """The two factors of a length expression: an integer, or `member`, `member>>k`, `member+a` or `member+a>>k` (the addend before the shift), once or twice, joined by `*`."""
    parts = [str(expr)] if isinstance(expr, int) else [p.strip() for p in str(expr).split("*")]
    if not 1 <= len(parts) <= 2:
        errors.append("%s: the length `%s` is not one or two factors joined by *" % (where, expr))
        parts = ["1"]
    out = []
    for p in parts:
        m = FACTOR.match(p)
        if p.isdigit():
            out.append({"const": int(p)})
        elif m:
            out.append({"member": m.group(1), "shift": int(m.group(3) or 0), **({"add": parse_addend(m.group(2))} if m.group(2) else {})})
        else:
            errors.append("%s: the factor `%s` of the length is no integer, member, member>>k, member+a or member+a>>k" % (where, p))
            out.append({"const": 1})
    while len(out) < 2:
        out.append({"const": 1})
    return out


def render_factor(f: dict, owner: str) -> str:
    if "const" in f:
        return "{ %d, 0, 0 }" % f["const"]
    add = ", %du" % f["add"] if f.get("add") else ""
    return "{ offsetof(%s, %s), (unsigned char)sizeof(((%s*)nullptr)->%s), %d%s }" % (owner, f["member"], owner, f["member"], f["shift"], add)


def render_length(length: list[dict], owner: str) -> str:
    return "{ %s, %s }" % (render_factor(length[0], owner), render_factor(length[1], owner))


def factor_asserts(length: list[dict], owner: str, where: str) -> list[str]:
    return ['static_assert(sizeof(((%s*)nullptr)->%s) == 1 || sizeof(((%s*)nullptr)->%s) == 2 || sizeof(((%s*)nullptr)->%s) == 4, "%s: %s is a byte, a short or an int");'
            % (owner, f["member"], owner, f["member"], owner, f["member"], where, f["member"]) for f in length if "member" in f]


def check_array(where: str, spec: dict, owner: str, errors: list[str]) -> dict:
    """The array entry of a field: element size, storage, capacity source and the parsed length. Errors for what is missing."""
    elem = spec.get("elem")
    if elem not in SIZES:
        errors.append("%s: an array needs an `elem` of %s" % (where, ", ".join(sorted(SIZES))))
        elem = "u8"
    if "length" not in spec:
        errors.append("%s: an array needs a `length`" % where)
    return {"elem": SIZES[elem], "ref": bool(spec.get("ref", False)), "length": parse_length(spec.get("length", 0), where, errors), "owner": owner}


VECTOR_SCALARS = {"char": 1, "unsigned char": 1, "short": 2, "unsigned short": 2, "int": 4, "unsigned": 4, "unsigned int": 4, "long": 4,
                  "unsigned long": 4, "float": 4, "double": 8}
VECTOR_MODES = {"object": "RV_OBJECT", "scalar": "RV_SCALAR", "value": "RV_VALUE"}


def check_vector(where: str, spec: dict, owner: str, ann: dict, errors: list[str]) -> dict:
    """The vector entry of a field: the element mode, the kind or C type, and the C++ type the BlockVector must hold."""
    elem = spec.get("elem")
    kinds = ann["objects"]
    out = {"mode": "object", "kind": "", "ctype": "", "owner": owner, "member": spec.get("member", spec["name"])}
    if not isinstance(elem, dict) or elem.get("mode") not in VECTOR_MODES:
        errors.append("%s: a vector needs an `elem` with a mode of %s" % (where, ", ".join(sorted(VECTOR_MODES))))
        return out
    out["mode"] = elem["mode"]
    if elem["mode"] == "scalar":
        if elem.get("type") not in VECTOR_SCALARS:
            errors.append("%s: a scalar element needs a `type` of %s" % (where, ", ".join(sorted(VECTOR_SCALARS))))
        out["ctype"] = elem.get("type", "int")
    else:
        target = kinds.get(elem.get("kind"))
        want = "state" if elem["mode"] == "object" else "value"
        if target is None or (target["scope"] != want and not (want == "state" and "kinds" in target)):
            errors.append("%s: a%s element needs a `kind` that is a %s kind of the schema" % (where, "n object" if want == "state" else " value", want))
        else:
            out["kind"] = elem["kind"]
            out["ctype"] = target["type"] if elem["mode"] == "value" else "Ref<%s>" % target["type"]
    return out


def is_based(spec: dict) -> bool:
    """A table whose elements are behind a ref field of another kind, `{"base": {"object": K, "field": F}, "count": N}`."""
    return isinstance(spec.get("table"), dict) and isinstance(spec["table"].get("base"), dict)


def is_global_table(spec: dict) -> bool:
    """A kind named by its index in a table that Game or SimGlobals holds."""
    return bool(spec.get("table")) and not is_based(spec)


def owner_integer_member(where: str, what: str, owner: dict, member: object, errors: list[str]) -> str | None:
    """The C++ member of a kind that a count or a capacity names, a recorded integer field of the kind."""
    if not isinstance(member, str):
        errors.append("%s: the %s is an integer or the name of a member of %s" % (where, what, owner["type"]))
        return None
    if not any(f.get("member", f["name"]) == member and f["kind"] in ("i32", "u32", "i16", "u16", "u8") for f in owner["fields"]):
        errors.append("%s: the %s %s is no integer field of %s" % (where, what, member, owner["type"]))
        return None
    return member


def check_based(kind: str, spec: dict, ann: dict, errors: list[str]) -> dict | None:
    """The site of a kind in a table behind a ref field: the owner's field points at an array of `count` elements, named `Owner#N.field[i]`.

    A table with a `capacity` has room for that many elements, an integer or a member of the owner, of which the owner's `count` member (or an integer)
    are live; `"alloc": "module"` makes the replayer allocate the array with the module's own operator new."""
    where = "%s table" % kind
    table = spec["table"]
    base = table["base"]
    if set(table) - {"base", "count", "capacity", "alloc"} or set(base) != {"object", "field"}:
        errors.append("%s: a table behind a ref field is {\"base\": {\"object\", \"field\"}, \"count\"} with an optional \"capacity\" and \"alloc\"" % where)
        return None
    if spec["scope"] != "state" or spec.get("inside") or spec.get("kinds") or spec.get("delta"):
        errors.append("%s: a kind in a table behind a ref field is a state kind with no sites, kinds or delta" % where)
    owner = ann["objects"].get(base["object"])
    if owner is None or owner["scope"] != "state":
        errors.append("%s: the base %s is no state kind of the schema" % (where, base["object"]))
        return None
    if owner.get("kinds"):
        errors.append("%s: the base %s is a polymorphic base, which has no fields of its own" % (where, base["object"]))
    field = next((f for f in owner["fields"] if f["name"] == base["field"]), None)
    if field is None or field["kind"] != "ref" or field.get("target") != kind:
        errors.append("%s: the base %s.%s is no ref field that leads to %s" % (where, base["object"], base["field"], kind))
        return None
    out = {"kind": kind, "container": base["object"], "member": base["field"] + "[]", "offset": field.get("member", field["name"]),
           "stride": None, "count": 0, "ref": True, "field": field["name"], "capped": False, "module": False, "room": None}
    if "capacity" not in table:
        if "alloc" in table:
            errors.append("%s: only a table with a capacity has an alloc" % where)
        count = table.get("count")
        if isinstance(count, bool) or not isinstance(count, int) or count < 1:
            errors.append("%s: the count is a positive integer" % where)
            return None
        out["count"] = count
        return out
    out["capped"] = True
    count = table.get("count")
    if not isinstance(count, int) or isinstance(count, bool):
        owner_integer_member(where, "count", owner, count, errors)
    elif count < 0:
        errors.append("%s: the count is not negative" % where)
    capacity = table["capacity"]
    if isinstance(capacity, int) and not isinstance(capacity, bool):
        if capacity < 1:
            errors.append("%s: the capacity is a positive integer or a member of %s" % (where, owner["type"]))
        elif isinstance(count, int) and count > capacity:
            errors.append("%s: the count %d is more than the capacity %d" % (where, count, capacity))
        out["count"] = max(capacity, 1)
        out["room"] = parse_length(capacity, where, errors)
    elif owner_integer_member(where, "capacity", owner, capacity, errors) is not None:
        out["room"] = parse_length(capacity, where, errors)
    if out["room"] is None:
        out["room"] = parse_length(1, where, errors)
    if table.get("alloc", "module") != "module":
        errors.append("%s: the alloc of a table with a capacity is \"module\", or absent" % where)
    out["module"] = table.get("alloc") == "module"
    return out


def check_table(kind: str, spec: dict, errors: list[str]) -> dict:
    where = "%s table" % kind
    base = spec.get("base")
    if base not in ("game", "sim") or not spec.get("member"):
        errors.append("%s: needs a `base` of game or sim and a `member`" % where)
    alloc = spec.get("alloc", 0)
    module = alloc == "module"
    capacity = None
    if "capacity" in spec:
        capacity = parse_length(spec["capacity"], where + " capacity", errors)
        if isinstance(spec["capacity"], int) and not isinstance(spec["capacity"], bool) and spec["capacity"] < 1:
            errors.append("%s: the capacity is positive" % where)
        if isinstance(spec.get("count"), int) and isinstance(spec["capacity"], int) and spec["count"] > spec["capacity"]:
            errors.append("%s: the count %d is more than the capacity %d" % (where, spec["count"], spec["capacity"]))
        if not module and alloc != 0:
            errors.append("%s: a table with a capacity has no integer alloc, the capacity is what the replayer allocates" % where)
    elif module:
        errors.append("%s: \"alloc\": \"module\" needs a capacity" % where)
    elif isinstance(alloc, bool) or not isinstance(alloc, int) or alloc < 0:
        errors.append("%s: the alloc is an integer, or \"module\" for a table with a capacity" % where)
        alloc = 0
    if module and not spec.get("ref", False):
        errors.append("%s: only a ref table can be allocated by the module" % where)
    return {"kind": kind, "sim": base == "sim", "ref": bool(spec.get("ref", False)), "member": spec.get("member", "?"),
            "count": parse_length(spec.get("count", 0), where, errors), "alloc": 0 if module or capacity else int(alloc),
            "owner": "SimGlobals" if base == "sim" else "Game", "capacity": capacity, "module": module}


VTABLE_NAME = re.compile(r"^[A-Za-z_]\w*$")


def check_polymorphic(ann: dict) -> list[str]:
    """Errors for the N5 keys: `vtable` of a concrete kind, `kinds` of a base, `poly` of a pointer field, `table` of a code field."""
    errors = []
    kinds = ann["objects"]
    tables = ann.get("code_tables", {})
    owner: dict[str, str] = {}
    for base, spec in kinds.items():
        if "kinds" not in spec:
            continue
        where = "%s (base)" % base
        if spec["scope"] != "state" or spec.get("table") or spec.get("inside") or spec.get("vtable"):
            errors.append("%s: a base kind is a state kind with no table, inside or vtable of its own" % where)
        if spec["fields"]:
            errors.append("%s: a base kind has no fields, its kinds have them" % where)
        if not spec["kinds"]:
            errors.append("%s: lists no kinds" % where)
        for name in spec["kinds"]:
            concrete = kinds.get(name)
            if concrete is None:
                errors.append("%s: the kind %s is not an object of the schema" % (where, name))
            elif "vtable" not in concrete:
                errors.append("%s: the kind %s has no vtable" % (where, name))
            elif name in owner:
                errors.append("%s: the kind %s is also a kind of %s" % (where, name, owner[name]))
            else:
                owner[name] = base
    seen: dict[str, str] = {}
    for name, spec in kinds.items():
        if "vtable" not in spec:
            continue
        if not isinstance(spec["vtable"], str) or not VTABLE_NAME.match(spec["vtable"]):
            errors.append("%s: the vtable is the name of a global" % name)
        elif spec["vtable"] in seen:
            errors.append("%s: the vtable %s is also the vtable of %s" % (name, spec["vtable"], seen[spec["vtable"]]))
        else:
            seen[spec["vtable"]] = name
        if spec["scope"] != "state" or spec.get("table"):
            errors.append("%s: a kind with a vtable is a state kind with no table" % name)
        if name not in owner and not spec.get("inside"):
            errors.append("%s: has a vtable and is a kind of no base, and is embedded nowhere" % name)
    for name, spec in kinds.items():
        for f in spec["fields"]:
            where = "%s.%s" % (name, f["name"])
            if "callback" in f:
                continue
            base = kinds.get(f.get("target")) if f["kind"] in ("ref", "embed", "unitid", "ptr", "refs", "embeds") else None
            if f["kind"] == "ptr" and (base is None or base["scope"] != "state"):
                errors.append("%s: a ptr field points at a state kind of the schema" % where)
            if "poly" in f:
                if f["kind"] not in ("ref", "refs") or f.get("target") != f["poly"] or base is None or "kinds" not in base:
                    errors.append("%s: `poly` names the base kind that the ref points at" % where)
            elif base is not None and "kinds" in base:
                errors.append("%s: points at the base kind %s and needs \"poly\": \"%s\"" % (where, f["target"], f["target"]))
            if f["kind"] == "code":
                if f.get("table") not in tables:
                    errors.append("%s: a code field names a table of `code_tables`" % where)
                if spec["scope"] != "state":
                    errors.append("%s: only the fields of a state kind are code pointers" % where)
    return errors


def embeds_member(f: dict) -> str:
    """The member path of an `embeds` field, with `[]` where the index goes: `channels[]` by default, `weapons[].aimCob` for a member of each element."""
    return f.get("member", f["name"] + "[]")


def check_elements(ann: dict) -> list[str]:
    """Errors for the array fields of a state kind: `refs` (a count of refs) and `embeds` (a count of structs of a kind, embedded in place)."""
    errors = []
    kinds = ann["objects"]
    for name, spec in kinds.items():
        for f in spec["fields"]:
            if f["kind"] not in ("refs", "embeds"):
                continue
            where = "%s.%s" % (name, f["name"])
            count = f.get("count")
            target = kinds.get(f.get("target"))
            if spec["scope"] != "state":
                errors.append("%s: only the fields of a state kind are arrays of refs or embeds" % where)
            if isinstance(count, bool) or not isinstance(count, int) or count < 1:
                errors.append("%s: `count` is a positive integer" % where)
            if target is None or target["scope"] != "state":
                errors.append("%s: the target %s is no state kind of the schema" % (where, f.get("target")))
            if f["kind"] == "refs":
                if "member" in f and not VTABLE_NAME.match(f["member"]):
                    errors.append("%s: the member of an array of refs is a name" % where)
                continue
            member = embeds_member(f)
            if member.count("[]") != 1 or member.endswith("."):
                errors.append("%s: the member of an array of embeds has one `[]`, as in `channels[]` or `weapons[].aimCob`" % where)
            if "poly" in f:
                errors.append("%s: an array of embeds names one kind, and has no `poly`" % where)
    return errors


MEMBER_PATH = re.compile(r"^[A-Za-z_]\w*(\.[A-Za-z_]\w*)*$")
SELF_KEYS = {"name", "kind", "member", "index", "target", "aliases"}


def field_kind(f: dict) -> str:
    """The key of FIELD_KINDS for an object field: a self pointer written as an index, and a ref or ptr that holds a callback, are kinds of their own."""
    if f.get("callback") is not None and f["kind"] in ("ref", "ptr"):
        return "cb" + f["kind"]
    return "selfindex" if f["kind"] == "self" and f.get("index") else f["kind"]


def check_self(kind: str, spec: dict, f: dict, kinds: dict, errors: list[str]) -> dict:
    """The entry of a `self` field: a pointer into the same object, to a member (never written as a line) or, with `index`, to an element of the array that a member holds (written as the index)."""
    where = "%s.%s" % (kind, f["name"])
    member = f.get("member")
    out = {"owner": spec["type"], "name": f["name"], "member": member if isinstance(member, str) else "?", "index": f.get("index") is True,
           "target": f.get("target")}
    if spec["scope"] != "state":
        errors.append("%s: only the fields of a state kind are self pointers" % where)
    if set(f) - SELF_KEYS:
        errors.append("%s: a self field is {\"name\", \"kind\", \"member\"} with an optional \"index\", \"target\" and \"aliases\"" % where)
    if "index" in f and not isinstance(f["index"], bool):
        errors.append("%s: `index` is true or false" % where)
    if not isinstance(member, str) or not MEMBER_PATH.match(member):
        errors.append("%s: the `member` of a self field is the name of a member of %s" % (where, spec["type"]))
        return out
    if not out["index"]:
        if "target" in f:
            errors.append("%s: a self pointer to a member has no `target`, only an index into an array does" % where)
        return out
    target = kinds.get(f.get("target"))
    if target is None or target["scope"] != "state":
        errors.append("%s: the target of a self index is the state kind of the array's elements" % where)
    earlier = spec["fields"][:spec["fields"].index(f)]
    holder = next((g for g in earlier if g.get("member", g["name"]) == member), None)
    if holder is None or holder["kind"] != "ref" or holder.get("target") != f.get("target"):
        errors.append("%s: the member %s is no ref field of %s that comes earlier and leads to %s, and the index counts elements of its array"
                      % (where, member, kind, f.get("target")))
    return out


def check_ctype(ann: dict) -> list[str]:
    """Errors for `ctype`, the C type a ref is declared with (`Ref<char>` to a `Player`), when it differs from its target kind."""
    errors = []
    for name, spec in ann["objects"].items():
        for f in spec["fields"]:
            if "ctype" not in f:
                continue
            where = "%s.%s" % (name, f["name"])
            if f["kind"] not in ("ref", "refs"):
                errors.append("%s: only a ref or an array of refs has a `ctype`" % where)
            elif not isinstance(f["ctype"], str) or not re.match(r"^[A-Za-z_][\w ]*(::[A-Za-z_]\w*)*$", f["ctype"]):
                errors.append("%s: the `ctype` is the C type that the Ref holds, `char`" % where)
    return errors


def check_inside(ann: dict) -> tuple[dict, list[str]]:
    """Errors for `inside`, and the sites of each kind.

    A site is {"kind", "member"}: the member of a container that the kind lives in, an `embed` field of the container, or an `embeds` field when the
    member has `[]` for the index of an element. A kind in a table behind a ref field of another kind (`table` with an object `base`) has that
    field's elements as its site."""
    errors = []
    kinds = ann["objects"]
    taken: set[tuple[str, str]] = set()
    sites: dict[str, list[dict]] = {}
    for name, spec in kinds.items():
        if is_based(spec):
            based = check_based(name, spec, ann, errors)
            if based is not None:
                if (based["container"], based["member"]) in taken:
                    errors.append("%s table: the site %s.%s is already the site of a kind" % (name, based["container"], based["member"]))
                taken.add((based["container"], based["member"]))
                sites[name] = [based]
            continue
        inside = spec.get("inside")
        if inside is None:
            continue
        where = "%s inside" % name
        if not isinstance(inside, list) or not inside:
            errors.append("%s: `inside` lists the sites {\"kind\", \"member\"} that the kind is embedded at" % where)
            continue
        if spec.get("table"):
            errors.append("%s: a kind with a table is named by its index, and is not inside another kind" % where)
        for site in inside:
            if not isinstance(site, dict) or set(site) != {"kind", "member"}:
                errors.append("%s: a site is {\"kind\", \"member\"}" % where)
                continue
            container = kinds.get(site["kind"])
            if container is None or container["scope"] != "state":
                errors.append("%s: the site %s.%s is not in a state kind of the schema" % (where, site["kind"], site["member"]))
                continue
            if container.get("inside") or is_based(container):
                errors.append("%s: the container %s is itself inside another kind" % (where, site["kind"]))
            key = (site["kind"], site["member"])
            if key in taken:
                errors.append("%s: the site %s.%s is already the site of a kind" % (where, key[0], key[1]))
            taken.add(key)
            member = site["member"]
            array = "[]" in member
            leads = [f for f in container["fields"] if f.get("target") == name and (
                (array and f["kind"] == "embeds" and embeds_member(f) == member) or (not array and f["kind"] == "embed" and f.get("member", f["name"]) == member))]
            if not leads:
                errors.append("%s: the site %s.%s is no %s field of %s that leads to %s" % (where, key[0], key[1], "embeds" if array else "embed", key[0], name))
                continue
            sites.setdefault(name, []).append({"kind": name, "container": site["kind"], "member": member, "offset": member.replace("[]", "[0]"),
                                               "array": member[:member.index("[]")] if array else None, "count": leads[0]["count"] if array else 1, "ref": False})
    return sites, errors


def check_vptr(ann: dict) -> list[str]:
    """Errors for `vptr`: the global that holds the vtable of a kind whose first word the code sets by hand."""
    errors = []
    taken = {spec["vtable"]: name for name, spec in ann["objects"].items() if isinstance(spec.get("vtable"), str)}
    for name, spec in ann["objects"].items():
        if "vptr" not in spec:
            continue
        if not isinstance(spec["vptr"], str) or not VTABLE_NAME.match(spec["vptr"]):
            errors.append("%s: the vptr is the name of a global" % name)
        elif spec["vptr"] in taken:
            errors.append("%s: the vptr %s is also the vtable of %s" % (name, spec["vptr"], taken[spec["vptr"]]))
        else:
            taken[spec["vptr"]] = name
        if spec["scope"] != "state" or spec.get("table") or spec.get("kinds") or "vtable" in spec:
            errors.append("%s: a kind with a vptr is a state kind with no table, kinds or vtable" % name)
    return errors


def check_delta(ann: dict) -> list[str]:
    """Errors for `delta`: true on a state kind with a table, recorded as changed elements when a slot is all-of over it."""
    errors = []
    for name, spec in ann["objects"].items():
        if "delta" not in spec:
            continue
        where = "%s delta" % name
        if spec["delta"] is not True:
            errors.append("%s: `delta` is true, or absent" % where)
        if not is_global_table(spec):
            errors.append("%s: only a kind with a table of Game or SimGlobals can be a delta" % where)
        if spec["scope"] != "state" or spec.get("inside") or spec.get("kinds") or spec.get("vtable"):
            errors.append("%s: a delta kind is a state kind with no sites, kinds or vtable" % where)
    return errors


CALLBACK_ARG_MODES = {"scalar", "text", "object", "value"}
CALLBACK_FIELD_KEYS = {"name", "kind", "callback", "member"}


def check_callback_fields(ann: dict) -> list[str]:
    """Errors for the `callback` key of a field: it names an interface of `callbacks` and belongs on a ref or ptr field of a state kind, which then has no target."""
    errors = []
    for name, spec in ann["objects"].items():
        for f in spec["fields"]:
            where = "%s.%s" % (name, f["name"])
            if "callback" in f:
                if f["kind"] not in ("ref", "ptr"):
                    errors.append("%s: the `callback` key is for a ref or ptr field, and this one is %s" % (where, f["kind"]))
                elif f["callback"] not in ann.get("callbacks", {}):
                    errors.append("%s: the `callback` %s is no entry of `callbacks`" % (where, f["callback"]))
                elif set(f) - CALLBACK_FIELD_KEYS:
                    errors.append("%s: a field that holds a callback is {\"name\", \"kind\", \"callback\"} with an optional \"member\", and has no %s"
                                  % (where, ", ".join(sorted(set(f) - CALLBACK_FIELD_KEYS))))
                if spec["scope"] != "state":
                    errors.append("%s: only the fields of a state kind hold callbacks" % where)
            for key in ("elem",):
                if isinstance(f.get(key), dict) and "callback" in f[key]:
                    errors.append("%s: the `callback` key is for a ref or ptr field, and not for the elements of a %s" % (where, f["kind"]))
    return errors


def callback_virtuals(entry: dict, name: str, errors: list[str]) -> list[str]:
    """The virtual methods of the class in the header, in declaration order."""
    path = ROOT / entry["header"]
    if not path.exists():
        errors.append("callback %s: the header %s does not exist" % (name, entry["header"]))
        return []
    text = re.sub(r"//[^\n]*", "", re.sub(r"/\*.*?\*/", "", path.read_text(), flags=re.S))
    m = re.search(r"\b(?:class|struct)\s+%s\b[^;{]*\{(.*?)\n\};" % re.escape(name), text, re.S)
    if not m:
        errors.append("callback %s: %s declares no such class" % (name, entry["header"]))
        return []
    return re.findall(r"\bvirtual\b[^;{]*?(~?\w+)\s*\(", m.group(1))


def check_callbacks(ann: dict) -> tuple[list[dict], list[str]]:
    """The callback interfaces: each method listed once in declaration order, with the modes of its arguments and its result."""
    errors = []
    out = []
    handles = {"file", "rec", "bank"}
    kinds = ann["objects"]
    for name, entry in ann.get("callbacks", {}).items():
        where = "callback %s" % name
        handle = entry.get("handle", "")
        if not VTABLE_NAME.match(handle) or handle in handles:
            errors.append("%s: the handle is a name that no other handle has" % where)
        handles.add(handle)
        virtuals = callback_virtuals(entry, name, errors)
        methods = entry.get("methods", [])
        if virtuals and [m["name"] for m in methods] != virtuals:
            errors.append("%s: the methods are %s, and the class declares the virtuals %s" % (where, [m["name"] for m in methods], virtuals))
        if any(v.startswith("~") for v in virtuals):
            errors.append("%s: a callback interface has no virtual destructor" % where)
        for m in methods:
            label = "%s.%s" % (where, m["name"])
            for a in m.get("args", []):
                if "callback" in a:
                    errors.append("%s: the argument %s has a `callback` key, which a callback's arguments do not take" % (label, a.get("name")))
                if a.get("mode") not in CALLBACK_ARG_MODES:
                    errors.append("%s: the argument %s has the mode %s, and callbacks take %s" % (label, a.get("name"), a.get("mode"), ", ".join(sorted(CALLBACK_ARG_MODES))))
                elif a["mode"] == "object" and (a.get("kind") not in kinds or a["type"] != kinds[a["kind"]]["type"] + "*" or kinds[a["kind"]]["scope"] == "value"):
                    errors.append("%s: the argument %s is a pointer to a state kind of the schema" % (label, a["name"]))
                elif a["mode"] == "value" and (a.get("kind") not in kinds or kinds[a["kind"]]["scope"] != "value" or a["type"] != kinds[a["kind"]]["type"]):
                    errors.append("%s: the argument %s is a value kind of the schema by value" % (label, a["name"]))
                elif a["mode"] == "scalar" and a["type"] not in SCALARS:
                    errors.append("%s: the argument %s is %s, which is no scalar" % (label, a["name"], a["type"]))
                elif a["mode"] == "text" and normalise(a["type"]) != "char*":
                    errors.append("%s: the argument %s of mode text is a char*" % (label, a["name"]))
            ret = m.get("ret", "void")
            if ret != "void" and ret not in SCALARS and not (ret.endswith("*") and ret[:-1] in {s["type"] for s in kinds.values() if s["scope"] == "state"}):
                errors.append("%s: the result %s is void, a scalar or a pointer to a state kind" % (label, ret))
        out.append({"name": name, "header": entry.get("header", ""), "handle": handle, "methods": methods})
    return out, errors


def check_code_tables(ann: dict, members: list[tuple[str, str]]) -> tuple[list[dict], list[str]]:
    """The code tables: the functions of each in order, every one declared in a listed header, and the slot that fills it."""
    errors = []
    out = []
    includes = ann.get("includes", [])
    for name, spec in ann.get("code_tables", {}).items():
        where = "code table %s" % name
        entries = spec.get("entries", [])
        if not entries or len(set(entries)) != len(entries):
            errors.append("%s: needs entries, each once" % where)
        text = ""
        for header in spec.get("headers", []):
            path = ROOT / header
            if not path.exists():
                errors.append("%s: the header %s does not exist" % (where, header))
                continue
            if os.path.relpath(path, ROOT / "src/harness").replace(os.sep, "/") not in includes:
                errors.append("%s: the header %s is not one of the schema's `includes`" % (where, header))
            text += " ".join(path.read_text().split()) + " "
        for fn in entries:
            if not re.search(r"\b%s\(" % re.escape(fn), text):
                errors.append("%s: %s is declared in none of its headers" % (where, fn))
        fill = spec.get("fill", "")
        seam, _, slot = fill.partition(".")
        if seam not in {m[0] for m in members} or slot not in {s["name"] for s in parse_interface(seam, dict((m[0], m[1]) for m in members)[seam])["slots"]}:
            errors.append("%s: `fill` names a slot of a seam as seam.Slot" % where)
        out.append({"name": name, "entries": entries, "fill": fill})
    return out, errors


def tracked(target: str, game: dict, sim: dict, aliases: dict | None = None) -> str | None:
    if target.startswith("Game::") and target[6:] in game:
        return "Game." + target[6:]
    if target.startswith("Game::") and target[6:] in (aliases or {}):
        return "Game." + aliases[target[6:]]
    if target in sim:
        return "SimGlobals." + target
    return None


def object_targets(target: str, ann: dict) -> list[tuple[str, str]]:
    """(kind, field) of every object field the target `Type::member` names: the member of a state or index kind of that type, or an alias.

    A value kind is not matched: the fields of a struct argument are recorded as its lines, never as `r` and `w` lines of an object."""
    if "::" not in target:
        return []
    type_name, member = target.split("::", 1)
    hits = []
    for kind, spec in ann["objects"].items():
        if spec["scope"] == "value":
            continue
        for f in spec["fields"]:
            if (spec["type"] == type_name and f["name"] == member) or target in f.get("aliases", []):
                hits.append((kind, f["name"]))
    return hits


WALKABLE = ("ref", "embed", "unitid", "weapon", "refs", "embeds")
MAX_REACH_DEPTH = 2


def reach_depth(kind: str, names: set, ann: dict, seen: tuple = ()) -> int:
    """Objects deep a reach list goes from `kind`: the kind is 1, and each listed pointer field adds the depth of its target."""
    if kind in seen:
        return 99
    deepest = 0
    for f in ann["objects"][kind]["fields"]:
        if "%s.%s" % (kind, f["name"]) in names and f["kind"] in WALKABLE and f.get("target"):
            deepest = max(deepest, reach_depth(f["target"], names, ann, seen + (kind,)))
    return 1 + deepest


def resolve_reach(where: str, spec: dict, root: str, ann: dict, object_fields: list, object_index: dict, table: list) -> list[str]:
    """Checks the `reach` list of a spec (field names `Kind.field`, and its explicit `depth`) and gives it a slice of the table."""
    errors = []
    spec["reachFirst"], spec["reachCount"] = 0, 0
    names = spec.get("reach")
    if not names:
        return ["%s needs a reach list" % where]
    indices = []
    for n in names:
        kind, _, field = n.partition(".")
        if (kind, field) not in object_index:
            errors.append("%s: the reach names the unknown field %s" % (where, n))
        else:
            indices.append(object_index[(kind, field)])
    if errors:
        return errors
    depth = reach_depth(root, set(names), ann)
    if spec.get("depth") != depth or depth > MAX_REACH_DEPTH:
        errors.append("%s: the reach is %d deep and its depth says %s (at most %d)" % (where, depth, spec.get("depth"), MAX_REACH_DEPTH))
    for i in range(len(table) - len(indices) + 1):
        if table[i:i + len(indices)] == indices:
            spec["reachFirst"], spec["reachCount"] = i, len(indices)
            return errors
    spec["reachFirst"], spec["reachCount"] = len(table), len(indices)
    table.extend(indices)
    return errors


def resolve_field_reaches(ann: dict, object_fields: list, object_index: dict, table: list) -> list[str]:
    errors = []
    for f in object_fields:
        if f["kind"] == "unitid":
            where = "%s.%s" % (f["object"], f["name"])
            errors += resolve_reach(where, f, f["target"], ann, object_fields, object_index, table)
    return errors


CONDITION_FIELDS = {"i32": (4, -2**31, 2**31 - 1), "u32": (4, -2**31, 2**32 - 1), "flags": (4, -2**31, 2**32 - 1),
                    "i16": (2, -2**15, 2**15 - 1), "u16": (2, 0, 2**16 - 1), "u8": (1, 0, 2**8 - 1)}
CONDITION_KEYS = {"field", "mask", "equals", "via", "kind"}


def condition_source(where: str, f: dict, when: dict, object_fields: list, object_index: dict, ann: dict, errors: list[str]) -> str | None:
    """The kind whose field a condition reads: the kind of f itself, or the one that its `via` field names or indexes."""
    via = when.get("via")
    if via is None:
        if "kind" in when:
            errors.append("%s: the `kind` of a condition says which table a `via` integer indexes, and there is no `via`" % where)
        return f["object"]
    if (f["object"], via) not in object_index:
        errors.append("%s: the `via` of the condition names %s, which is no field of %s" % (where, via, f["object"]))
        return None
    v = object_fields[object_index[(f["object"], via)]]
    if v.get("when"):
        errors.append("%s: the `via` field %s has a `when` of its own" % (where, via))
        return None
    f["condVia"] = object_index[(f["object"], via)] + 1
    if "callback" in v:
        errors.append("%s: the `via` field %s holds a callback, which has no fields to read" % (where, via))
        return None
    if v["kind"] in ("ref", "ptr"):
        if "kind" in when:
            errors.append("%s: the `via` field %s leads to its target, and `kind` is for an integer" % (where, via))
        return v["target"]
    if v["kind"] not in ("u8", "u16", "i16", "u32", "i32"):
        errors.append("%s: the `via` field %s is a ref, a ptr or an integer that indexes a table" % (where, via))
        return None
    table = ann["objects"].get(when.get("kind"))
    if table is None or not is_global_table(table):
        errors.append("%s: the `via` field %s is an integer, and the condition's `kind` names the table kind it indexes" % (where, via))
        return None
    return when["kind"]


def resolve_conditions(ann: dict, object_fields: list, object_index: dict) -> list[str]:
    """Errors for every `when`, and the condition fields of the generated table: the field read, its mask and value, and the `via` field."""
    errors = []
    for f in object_fields:
        when = f.get("when")
        if when is None:
            continue
        where = "%s.%s" % (f["object"], f["name"])
        if not isinstance(when, dict) or "field" not in when or "equals" not in when or set(when) - CONDITION_KEYS:
            errors.append("%s: a `when` is {\"field\", \"equals\"} with an optional \"mask\", \"via\" and \"kind\"" % where)
            continue
        if ann["objects"][f["object"]]["scope"] != "state":
            errors.append("%s: only the field of a state kind has a `when`" % where)
        source = condition_source(where, f, when, object_fields, object_index, ann, errors)
        if source is None:
            continue
        if (source, when["field"]) not in object_index:
            errors.append("%s: the condition reads %s, which is no field of %s" % (where, when["field"], source))
            continue
        read = object_fields[object_index[(source, when["field"])]]
        if read["kind"] not in CONDITION_FIELDS:
            errors.append("%s: the condition reads %s.%s, and only an integer or flags field can be read" % (where, source, read["name"]))
            continue
        if read.get("when"):
            errors.append("%s: the condition reads %s.%s, which has a `when` of its own" % (where, source, read["name"]))
            continue
        size, low, high = CONDITION_FIELDS[read["kind"]]
        mask, equals = when.get("mask"), when["equals"]
        if isinstance(equals, bool) or not isinstance(equals, int):
            errors.append("%s: the `equals` of a condition is an integer" % where)
            continue
        if mask is not None:
            if isinstance(mask, bool) or not isinstance(mask, int) or not 0 < mask < 1 << (8 * size):
                errors.append("%s: the `mask` of a condition is a non-zero integer that fits the %d bytes of %s.%s" % (where, size, source, read["name"]))
                continue
            if equals < 0 or equals & ~mask:
                errors.append("%s: the condition can never hold, because `equals` has bits outside its `mask`" % where)
                continue
            f["condMask"] = mask
        elif not low <= equals <= high:
            errors.append("%s: the `equals` of the condition is outside the range of %s.%s (%d to %d)" % (where, source, read["name"], low, high))
            continue
        f["condField"] = object_index[(source, when["field"])] + 1
        f["condValue"] = equals if equals < 2**31 else equals - 2**32
        f["condKind"] = source
    return errors


def functions_of(slot: dict, subsystem: str, assignment: dict[str, list[str]], note: dict) -> list[str]:
    if "functions" in note:
        return note["functions"]
    own = assignment.get(subsystem, [])
    for fn in own:
        if fn == slot["name"]:
            return [fn]
    for fn in own:
        if fn.endswith("::" + slot["name"]):
            return [fn]
    return []


def classify_arg(param: dict, note: dict | None, ann: dict) -> dict:
    """How one parameter is recorded: mode and its details, or mode 'pending'."""
    ctype = normalise(param["type"])
    out = {"name": param["name"], "type": ctype, "line": (note or {}).get("line", param["name"])}
    if note:
        out.update({k: v for k, v in note.items() if k != "line"})
        return out
    if ctype in SCALARS:
        out["mode"] = "stream" if ctype == "RandStream" else "scalar"
    elif ctype == "char*":
        out["mode"] = "text"
    elif ctype == "HapiBank*":
        out.update({"mode": "handle", "kind": "bank"})
    elif ctype.endswith("*") and ctype[:-1] in {s["type"] for s in ann["objects"].values() if s["scope"] == "state"}:
        kind = [k for k, s in ann["objects"].items() if s["type"] == ctype[:-1]][0]
        out.update({"mode": "object", "kind": kind})
        if "kinds" in ann["objects"][kind]:
            out["poly"] = kind
    elif ctype == "WeaponDef*":
        out["mode"] = "weapon"
    else:
        out["mode"] = "pending"
    return out


def classify_result(ret: str, note: dict | None, ann: dict) -> dict | None:
    ctype = normalise(ret)
    if ctype == "void":
        return None
    out = {"type": ctype}
    if note:
        out.update(note)
        return out
    if ctype in SCALARS:
        out["mode"] = "scalar"
    elif ctype.endswith("*") and ctype[:-1] in {s["type"] for s in ann["objects"].values() if s["scope"] == "state"}:
        kind = [k for k, s in ann["objects"].items() if s["type"] == ctype[:-1]][0]
        out.update({"mode": "object", "kind": kind})
        if "kinds" in ann["objects"][kind]:
            out["poly"] = kind
    elif ctype == "char*":
        out["mode"] = "text"
    else:
        out["mode"] = "pending"
    return out


ARG_MODES = {"scalar", "stream", "text", "handle", "object", "struct", "structs", "value", "argv", "bytes-in", "bytes-out", "weapon", "vec3", "units", "vector", "callback", "pending"}
STATE_PATH = re.compile(r"^Game(\.[A-Za-z_]\w*)+$")


@functools.lru_cache(maxsize=None)
def game_members() -> frozenset[str]:
    return frozenset(l["name"] for l in traceschema.Schema().layout("Game")[0])


def arg_type(a: dict) -> str:
    """The type the modes of an argument see: its `cast` when it has one, else its own."""
    return a.get("cast") or a["type"]


def vector_type(ctype: str) -> str:
    return "std::vector<%s, std::allocator<%s> >*" % (ctype, ctype)


def length_parts(a: dict, i: int) -> tuple[list[str], str]:
    """The statements and the C++ expression of the byte length of argument i: another argument, or a path through Game, times `elem`."""
    scale = SIZES[a["elem"]] if a.get("elem") else 1
    if not STATE_PATH.match(str(a["length"])):
        base = "a%d" % a["lengthIndex"]
        return [], base if scale == 1 else "(%s * %d)" % (base, scale)
    steps = a["length"].split(".")[1:]
    lines = ["const auto* l%d_0 = ReplayStep(g_game);" % i]
    for k, member in enumerate(steps[:-1], 1):
        lines.append("const auto* l%d_%d = l%d_%d != nullptr ? ReplayStep(l%d_%d->%s) : nullptr;" % (i, k, i, k - 1, i, k - 1, member))
    last = len(steps) - 1
    lines.append("int len%d = l%d_%d != nullptr ? (int)l%d_%d->%s : 0;" % (i, i, last, i, last, steps[-1]))
    return lines, "len%d" % i if scale == 1 else "(len%d * %d)" % (i, scale)
RESULT_MODES = {"scalar", "object", "text", "weapon", "value", "pending"}


CAST_MODES = {"object", "struct", "structs", "vec3"}
SCALAR_INOUT = {"int*", "unsigned int*", "short*", "unsigned short*", "unsigned char*"}


def is_scalar_inout(a: dict) -> bool:
    """A pointer to a scalar that is read and written back: `{"mode": "value", "dir": "inout"}` with no kind."""
    return a["mode"] == "value" and a.get("dir") == "inout"


def check_vector_arg(where: str, a: dict, label: str, kinds: dict, errors: list[str]) -> None:
    """A vector argument is an out vector of value structs: `elem` names the value kind, and the argument is the std::vector of its type."""
    elem = a.get("elem")
    if a.get("dir") != "out":
        errors.append("%s: %s is a vector, and only a vector out is recorded (`dir`: out)" % (where, label))
    if not isinstance(elem, dict) or elem.get("mode") != "value":
        errors.append("%s: %s needs an `elem` of mode value" % (where, label))
        return
    if "callback" in elem:
        errors.append("%s: the `callback` key is for the argument of a callback, and not for the elements of %s" % (where, label))
    spec = kinds.get(elem.get("kind"))
    if spec is None or spec["scope"] != "value":
        errors.append("%s: the element of %s needs a `kind` that is a value kind of the schema" % (where, label))
    elif arg_type(a) != vector_type(spec["type"]):
        errors.append("%s: %s is %s, and a vector of the kind %s is %s" % (where, label, arg_type(a), elem["kind"], vector_type(spec["type"])))


def check_length(where: str, a: dict, label: str, args: list[dict], errors: list[str]) -> None:
    """The length of a byte buffer names another integer argument, or a path through Game (`Game.net.unitCount`); `elem` scales it."""
    length = a.get("length")
    names = [x["name"] for x in args]
    if STATE_PATH.match(str(length)):
        if str(length).split(".")[1] not in game_members():
            errors.append("%s: the length %s of %s starts at %s, which is no member of Game" % (where, length, label, length.split(".")[1]))
    elif length not in names or length == a["name"]:
        errors.append("%s: %s needs a `length` that names another argument or a path through Game" % (where, label))
    elif args[names.index(length)]["type"] not in INTEGER_ARGS:
        errors.append("%s: the length %s of %s is no integer" % (where, length, a["name"]))
    if "elem" in a and a["elem"] not in SIZES:
        errors.append("%s: the `elem` of %s is one of %s" % (where, label, ", ".join(sorted(SIZES))))


def check_forms(where: str, args: list[dict], result: dict | None, ann: dict) -> list[str]:
    """Errors for the struct forms of a slot: the kind must be a value kind of the argument's type, a count must name an integer argument."""
    errors = []
    kinds = ann["objects"]
    names = [a["name"] for a in args]

    def value_kind(item: dict, label: str, pointer: bool) -> None:
        spec = kinds.get(item.get("kind"))
        if spec is None or spec["scope"] != "value":
            errors.append("%s: %s needs a `kind` that is a value kind of the schema" % (where, label))
        elif arg_type(item) != spec["type"] + ("*" if pointer else ""):
            errors.append("%s: %s is %s, and the kind %s is %s" % (where, label, arg_type(item), item["kind"], spec["type"] + ("*" if pointer else "")))

    def poly_kind(item: dict, label: str) -> None:
        spec = kinds.get(item.get("kind"))
        base = spec is not None and "kinds" in spec
        if base and item.get("poly") != item["kind"]:
            errors.append("%s: %s points at the base kind %s and needs \"poly\": \"%s\"" % (where, label, item["kind"], item["kind"]))
        if "poly" in item and (not base or item["poly"] != item["kind"]):
            errors.append("%s: `poly` of %s names the base kind that it points at" % (where, label))

    for a in args:
        label = "the argument %s" % a["name"]
        if a["mode"] == "object":
            poly_kind(a, label)
        if "cast" in a:
            if a["type"] != "void*":
                errors.append("%s: %s is %s, and only a void* argument takes a `cast`" % (where, label, a["type"]))
            elif not str(a["cast"]).endswith("*") or a["cast"] == "void*":
                errors.append("%s: the `cast` of %s is a pointer type other than void*" % (where, label))
            elif a["mode"] not in CAST_MODES:
                errors.append("%s: %s has a `cast` and the mode %s, and a cast is for %s" % (where, label, a["mode"], ", ".join(sorted(CAST_MODES))))
        if "callback" in a and a["mode"] != "callback":
            errors.append("%s: %s has the mode %s and a `callback` key, which only the mode callback takes" % (where, label, a["mode"]))
        if a["mode"] not in ARG_MODES:
            errors.append("%s: %s has the unknown mode %s" % (where, label, a["mode"]))
        elif a["mode"] == "struct":
            value_kind(a, label, True)
            if a.get("dir", "in") not in ("in", "out", "inout"):
                errors.append("%s: %s has the direction %s, and a struct is in, out or inout" % (where, label, a["dir"]))
            elif a.get("nullable") and a.get("dir", "in") == "out":
                errors.append("%s: %s is an out struct, and only a struct in or inout can be nullable" % (where, label))
        elif a["mode"] == "vector":
            check_vector_arg(where, a, label, kinds, errors)
        elif a["mode"] in ("bytes-in", "bytes-out"):
            check_length(where, a, label, args, errors)
        elif a["mode"] == "structs":
            value_kind(a, label, True)
            if a.get("dir", "in") != "in":
                errors.append("%s: %s is an array of structs, and only an array in is recorded" % (where, label))
            count = a.get("count")
            if count not in names or count == a["name"]:
                errors.append("%s: %s needs a `count` that names another argument" % (where, label))
            elif args[names.index(count)]["type"] not in INTEGER_ARGS:
                errors.append("%s: the count %s of %s is no integer" % (where, count, a["name"]))
        elif is_scalar_inout(a):
            if "kind" in a:
                errors.append("%s: %s is a scalar in/out value, which takes no `kind`" % (where, label))
            if a["type"] not in SCALAR_INOUT:
                errors.append("%s: %s is %s, and a scalar in/out value is a pointer to one of %s" % (where, label, a["type"], ", ".join(sorted(SCALAR_INOUT))))
        elif a["mode"] == "value":
            if "dir" in a:
                errors.append("%s: %s is a value with the direction %s, and only inout is allowed on a value" % (where, label, a["dir"]))
            value_kind(a, label, False)
        elif a["mode"] == "callback":
            entry = ann.get("callbacks", {}).get(a.get("callback"))
            if entry is None:
                errors.append("%s: %s needs a `callback` that is an entry of `callbacks`" % (where, label))
            elif a["type"] != a["callback"] + "*":
                errors.append("%s: %s is %s, and the callback %s is %s*" % (where, label, a["type"], a["callback"], a["callback"]))
    if result is not None:
        if "callback" in result:
            errors.append("%s: the result has a `callback` key, which only the argument of mode callback takes" % where)
        if result["mode"] == "object":
            poly_kind(result, "the result")
        if result["mode"] not in RESULT_MODES:
            errors.append("%s: the result has the unknown mode %s" % (where, result["mode"]))
        elif result["mode"] == "value":
            value_kind(result, "the result", False)
    return errors


def reached_kinds(slot: dict, object_fields: list[dict]) -> set[str]:
    """The object kinds a slot's arguments, result, all-of tables and object fields lead to."""
    kinds = {a["kind"] for a in slot["args"] if a.get("mode") in ("object", "units") and a.get("kind")}
    if slot["result"] is not None and slot["result"].get("mode") == "object" and slot["result"].get("kind"):
        kinds.add(slot["result"]["kind"])
    kinds.update(slot["allOf"])
    for i in slot["objectReads"] + slot["objectWrites"]:
        f = object_fields[i]
        kinds.add(f["object"])
        if f.get("target"):
            kinds.add(f["target"])
    return kinds


def check_product_counts(where: str, slot: dict, tables: list[dict], object_fields: list[dict]) -> list[str]:
    """Errors for a slot that reaches a table whose count is a product of block fields, or whose capacity names one, that its read set lacks."""
    errors = []
    reached = reached_kinds(slot, object_fields)
    for t in tables:
        if t["kind"] not in reached:
            continue
        prefix = "SimGlobals." if t["sim"] else "Game."
        if t.get("capacity"):
            errors += ["%s reaches %s, whose capacity is %s, and the slot's read set lacks it" % (where, t["kind"], prefix + f["member"])
                       for f in t["capacity"] if "member" in f and prefix + f["member"] not in slot["reads"]]
        if not all("member" in f for f in t["count"]):
            continue
        for f in t["count"]:
            name = prefix + f["member"]
            if name not in slot["reads"]:
                errors.append("%s reaches %s, whose count multiplies %s, and the slot's read set lacks it" % (where, t["kind"], name))
    return errors


SEAM_CALL = re.compile(r"\[(\w+)::(\w+)\]$")
VIRTUAL_CALL = re.compile(r"^virtual (\w+)::(~?\w+)$")


def through_functions(entry) -> list[str]:
    """The functions a `through` entry adds: a list, or an object with `functions` and `reason`."""
    return entry if isinstance(entry, list) else entry.get("functions", [])


def check_through(where: str, through: dict, by_function: dict) -> list[str]:
    errors = []
    for target, entry in through.items():
        names = through_functions(entry)
        if isinstance(entry, dict):
            if not entry.get("reason"):
                errors.append("%s: the `through` entry for %s is an object and needs a `reason`" % (where, target))
        elif not isinstance(entry, list) or not entry:
            errors.append("%s: the `through` entry for %s is a list of functions, or {\"reason\": ...} for a call that does nothing" % (where, target))
        if isinstance(entry, dict) and names:
            errors.append("%s: the `through` entry for %s has a reason and functions: a reason is for a call that does nothing" % (where, target))
        errors += ["%s: the `through` entry for %s names %s, which is no function" % (where, target, n) for n in names if n not in by_function]
    return errors


def through_key(target: str, through: dict) -> str | None:
    """The `through` key that names an indirect_call target: the whole target, or the name in its last brackets."""
    if target in through:
        return target
    hint = re.search(r"\[([^\[\]]+)\]$", target)
    if hint and hint.group(1) in through:
        return hint.group(1)
    return None


def unaccounted_indirect(target: str, sub: str, spec: dict, ann: dict, closure: set, by_function: dict, members: list[tuple[str, str]], own_functions) -> str | None:
    """Why an indirect_call row is not accounted for, or None.

    Accounted for: a call through a seam table to a slot the subsystem imports, or to a recorded slot of its own seam whose functions are in the
    closure; a vtable dispatch on a poly kind whose concrete functions are in the closure."""
    seam = SEAM_CALL.search(target)
    if seam and target.startswith("indirect via "):
        owner = {struct: name for name, struct in members}.get(seam.group(1))
        if owner is not None and "%s.%s" % (owner, seam.group(2)) in spec.get("imports", []):
            return None
        if owner == sub:
            inside = own_functions(seam.group(2))
            if inside and all(f in closure for f in inside):
                return None
            return "a call to the slot %s of its own seam, whose functions are not all in the closure" % seam.group(2)
        return "a seam call that %s does not import" % sub
    virtual = VIRTUAL_CALL.match(target)
    if virtual:
        cls, method = virtual.groups()
        kinds = ann["objects"]
        base = next((s for s in kinds.values() if s["type"] == cls and "kinds" in s), None)
        concrete = [kinds[k]["type"] for k in base["kinds"]] if base else [s["type"] for s in kinds.values() if s["type"] == cls and "vtable" in s]
        if not concrete:
            return "a vtable dispatch on %s, which is no kind with `poly`" % cls
        defined = [t + "::" + method for t in concrete if t + "::" + method in by_function]
        if base and len(defined) < len(concrete) and cls + "::" + method in by_function:
            defined.append(cls + "::" + method)
        missing = [f for f in defined if f not in closure]
        if missing:
            return "a vtable dispatch whose functions %s are not in the closure" % ", ".join(missing)
        return None
    return "an indirect call"


def build(ann: dict, rows: list[dict[str, str]], assignment: dict[str, list[str]]) -> dict:
    """The generated model: fields, objects, subsystems, and the errors."""
    game, sim = block_tables(ann)
    errors = check_sim_names(sim)
    arrays: list[dict] = []
    fields = field_table(game, sim, arrays, errors, ann["objects"])
    field_index = {f["name"]: i for i, f in enumerate(fields)}
    objects = []
    object_fields = []
    tables: list[dict] = []
    vectors: list[dict] = []
    elems: list[dict] = []
    selfs: list[dict] = []
    kind_names = list(ann["objects"])
    for kind in kind_names:
        spec = ann["objects"][kind]
        entry = {"name": kind, "type": spec["type"], "scope": spec["scope"], "table": spec.get("table"), "inside": spec.get("inside")}
        for key in ("vtable", "vptr", "kinds", "delta"):
            if spec.get(key):
                entry[key] = spec[key]
        objects.append(entry)
        if is_global_table(spec):
            if spec["scope"] != "state":
                errors.append("%s: only a state kind can have a table" % kind)
            tables.append(check_table(kind, spec["table"], errors))
        for f in spec["fields"]:
            entry = dict(f, object=kind)
            if f["kind"] == "array":
                where = "%s.%s" % (kind, f["name"])
                if spec["scope"] != "state":
                    errors.append("%s: only the fields of a state kind can be arrays" % where)
                array = check_array(where, f, spec["type"], errors)
                array["member"] = f.get("member", f["name"])
                arrays.append(array)
                entry["array"] = len(arrays)
            if f["kind"] in ("refs", "embeds"):
                elems.append({"kind": f["kind"], "count": f["count"] if isinstance(f.get("count"), int) and f["count"] > 0 else 1, "owner": spec["type"],
                              "member": embeds_member(f) if f["kind"] == "embeds" else f.get("member", f["name"])})
                entry["elems"] = len(elems)
            if f["kind"] == "self":
                selfs.append(check_self(kind, spec, f, ann["objects"], errors))
                entry["selfspec"] = len(selfs)
            if f["kind"] == "vector":
                where = "%s.%s" % (kind, f["name"])
                if spec["scope"] != "state":
                    errors.append("%s: only the fields of a state kind can be vectors" % where)
                vectors.append(check_vector(where, f, spec["type"], ann, errors))
                entry["vector"] = len(vectors)
                if vectors[-1]["mode"] == "object" and vectors[-1]["kind"]:
                    entry["target"] = vectors[-1]["kind"]
            object_fields.append(entry)
    for t in tables:
        prefix = "SimGlobals." if t["sim"] else "Game."
        for tag, length in (("count", t["count"]), ("capacity", t["capacity"] or [])):
            errors += ["%s table: the %s names %s, which is no tracked field of %s" % (t["kind"], tag, f["member"], prefix[:-1])
                       for f in length if "member" in f and t["capacity"] and prefix + f["member"] not in field_index]
    table_targets = set()
    for t in tables:
        top = t["member"].split(".")[0]
        table_targets.add(top if t["sim"] else "Game::" + top)
    object_index = {(f["object"], f["name"]): i for i, f in enumerate(object_fields)}
    errors += check_polymorphic(ann) + check_callback_fields(ann)
    callbacks, callback_errors = check_callbacks(ann)
    errors += callback_errors
    code_tables, code_errors = check_code_tables(ann, seam_members())
    errors += code_errors
    code_index = {t["name"]: i for i, t in enumerate(code_tables)}
    for f in object_fields:
        if f["kind"] == "code":
            f["codeTable"] = code_index.get(f.get("table"), 0)
    reach_table: list[int] = []
    errors += resolve_field_reaches(ann, object_fields, object_index, reach_table)
    errors += resolve_conditions(ann, object_fields, object_index)
    sites, inside_errors = check_inside(ann)
    errors += inside_errors + check_elements(ann) + check_vptr(ann)
    errors += check_delta(ann) + check_ctype(ann)
    by_function: dict[str, list[dict]] = {}
    for r in rows:
        by_function.setdefault(r["function"], []).append(r)
    used_rules = set()
    rules_of: dict[tuple[str, str], list[int]] = {}
    for i, r in enumerate(ann["rules"]):
        rules_of.setdefault((r["kind"], r["target"]), []).append(i)
    state_types = {spec["type"] for spec in ann["objects"].values() if spec["scope"] == "state"}
    kind_types = {spec["type"] for spec in ann["objects"].values()}
    exempt_types = ann.get("exempt_types", {})
    seam_types = {struct for _, struct in seam_members()} | {"Seams"}
    errors += ["exempt_types: %s needs a reason" % t for t, why in exempt_types.items() if not why]

    def rule_for(kind: str, target: str, function: str) -> int | None:
        scoped = [i for i in rules_of.get((kind, target), []) if function in ann["rules"][i].get("functions", [])]
        plain = [i for i in rules_of.get((kind, target), []) if "functions" not in ann["rules"][i]]
        found = scoped or plain
        return found[0] if found else None
    subsystems = {}
    slot_list = []
    members = seam_members()
    unknown = sorted(set(ann["subsystems"]) - {m[0] for m in members})
    errors += ["annotations name the unknown subsystem %s" % s for s in unknown]
    imported_any = {imp for spec in ann["subsystems"].values() for imp in spec.get("imports", [])}
    for sub, struct in members:
        spec = ann["subsystems"].get(sub)
        if spec is None:
            errors.append("annotations have no entry for subsystem %s" % sub)
            continue
        iface = parse_interface(sub, struct)
        complete = spec["status"] == "complete"
        notes = spec.get("slots", {})
        names = [s["name"] for s in iface["slots"]]
        errors += ["%s: annotated slot %s is not in interfaces/%s.h" % (sub, n, sub) for n in notes if n not in names]
        slots = {}
        pending = {"untracked": set(), "rules": set(), "functions": [], "args": [], "types": set(), "indirect": set()}
        unrecorded = {}
        for s in iface["slots"]:
            note = notes.get(s["name"], {})
            if "record" in note or "reason" in note:
                if "record" not in note:
                    errors.append("%s.%s has a reason and is recorded: `reason` goes with `\"record\": false`" % (sub, s["name"]))
                elif note["record"] is not False:
                    errors.append("%s.%s: `record` is only ever false" % (sub, s["name"]))
                elif not note.get("reason"):
                    errors.append("%s.%s is not recorded and has no `reason`" % (sub, s["name"]))
                else:
                    unrecorded[s["name"]] = {"reason": note["reason"], "native": note.get("native", s["name"])}
                    if "%s.%s" % (sub, s["name"]) not in imported_any:
                        continue
            skipped = s["name"] in unrecorded
            functions = [] if skipped else functions_of(s, sub, assignment, note)
            if not functions and not skipped:
                pending["functions"].append(s["name"])
            reads, writes = set(), set()
            obj_reads, obj_writes = set(), set()
            draws = False
            all_of: list[str] = []
            for tag, extra in (("reads", reads), ("writes", writes)):
                for name in [] if skipped else note.get(tag, []):
                    if name not in field_index:
                        errors.append("%s.%s: the extra %s %s is no block field" % (sub, s["name"], tag[:-1], name))
                    else:
                        extra.add(name)
            through = {} if skipped else note.get("through", {})
            errors += [] if skipped else check_through("%s.%s" % (sub, s["name"]), through, by_function)
            closure = list(functions)
            through_used = set()
            indirect_rows = []
            for fn in closure:
                for r in by_function.get(fn, []):
                    kind, target = r["kind"], r["target"]
                    if kind == "call" and target in RNG_CALLS:
                        draws = True
                    if kind in BLOCK_KINDS:
                        name = tracked(target, game, sim, ann.get("game_aliases"))
                        if name is not None:
                            if "r" in BLOCK_KINDS[kind]:
                                reads.add(name)
                            if "w" in BLOCK_KINDS[kind]:
                                writes.add(name)
                        elif target in ann["exempt"] or target in table_targets:
                            pass
                        elif target.startswith("Game::") or target.startswith("g_"):
                            pending["untracked"].add(target)
                    if kind in OBJECT_KINDS:
                        hits = object_targets(target, ann)
                        for hit in hits:
                            i = object_index[(hit[0], hit[1])]
                            if "r" in OBJECT_KINDS[kind]:
                                obj_reads.add(i)
                            if "w" in OBJECT_KINDS[kind]:
                                obj_writes.add(i)
                        owner_type = target.split("::")[0]
                        if not hits and owner_type in state_types and (kind, target) not in rules_of:
                            pending["untracked"].add(target)
                        elif (not hits and "::" in target and owner_type not in kind_types and owner_type not in exempt_types
                              and owner_type not in seam_types and not any(n == owner_type or n.startswith(owner_type + "_") for n in spec.get("services", []))
                              and owner_type != "Game" and (kind, target) not in rules_of and target not in ann["exempt"]):
                            pending["types"].add("%s %s" % (kind, target))
                    if kind == "indirect_call":
                        key = through_key(target, through)
                        if key is not None:
                            through_used.add(key)
                            for extra in through_functions(through[key]):
                                if extra not in closure:
                                    closure.append(extra)
                        else:
                            indirect_rows.append(target)
                    index = rule_for(kind, target, fn)
                    needs = kind == "field_escape" or (kind in ("deref_read", "deref_write") and BARE_POINTER.match(target))
                    if index is None:
                        if needs:
                            pending["rules"].add("%s %s" % (kind, target))
                    else:
                        rule = ann["rules"][index]
                        used_rules.add(index)
                        if rule["rule"] == "whole-object":
                            for i, f in enumerate(object_fields):
                                if f["object"] == rule["object"]:
                                    obj_reads.add(i)
                                    obj_writes.add(i)
                        elif rule["rule"] == "fields":
                            for tag, into, block in (("reads", obj_reads, reads), ("writes", obj_writes, writes)):
                                for name in rule.get(tag, []):
                                    k, fname = name.split(".", 1)
                                    if name in field_index:
                                        block.add(name)
                                    elif (k, fname) not in object_index:
                                        errors.append("the rule for %s %s names the unknown field %s" % (kind, target, name))
                                    else:
                                        into.add(object_index[(k, fname)])
                        elif rule["rule"] == "all-of":
                            if not is_global_table(ann["objects"].get(rule["object"], {})):
                                message = "the all-of rule names %s, which is no table kind of the schema" % rule["object"]
                                if message not in errors:
                                    errors.append(message)
                            elif rule["object"] not in all_of:
                                all_of.append(rule["object"])
            for target in indirect_rows:
                why = unaccounted_indirect(target, sub, spec, ann, set(closure), by_function, members,
                                           lambda name: [] if name not in names or name in unrecorded else functions_of({"name": name}, sub, assignment, notes.get(name, {})))
                if why is not None:
                    pending["indirect"].add("%s: %s (%s)" % (s["name"], target, why))
            errors += ["%s.%s: the `through` entry for %s matches no indirect_call row of the slot" % (sub, s["name"], k) for k in through if k not in through_used]
            if draws:
                reads.update(RNG_FIELDS)
                writes.update(RNG_FIELDS)
            order = list(field_index)
            args = [classify_arg(p, note.get("args", {}).get(p["name"]), ann) for p in s["params"]]
            for a in args:
                if a.get("when") and (a["mode"] != "struct" or a.get("dir") != "out" or a["when"] != "result != 0" or s["ret"] == "void"):
                    errors.append("%s.%s(%s): `when` is only `result != 0`, on an out struct of a slot that returns a value" % (sub, s["name"], a["name"]))
                if a["mode"] == "units":
                    errors += resolve_reach("%s.%s(%s)" % (sub, s["name"], a["name"]), a, a["kind"], ann, object_fields, object_index, reach_table)
            result = classify_result(s["ret"], note.get("result"), ann)
            errors += check_forms("%s.%s" % (sub, s["name"]), args, result, ann)
            for a in args:
                if a["mode"] == "pending":
                    pending["args"].append("%s(%s)" % (s["name"], a["name"]))
            if result is not None and result["mode"] == "pending":
                pending["args"].append("%s result" % s["name"])
            entry = {
                "conv": s["conv"], "ret": s["ret"], "functions": functions,
                "reads": [f for f in order if f in reads], "writes": [f for f in order if f in writes],
                "objectReads": sorted(obj_reads), "objectWrites": sorted(obj_writes), "allOf": [k for k in kind_names if k in all_of],
                "args": args, "result": result, "native": note.get("native", s["name"]),
            }
            if skipped:
                unrecorded[s["name"]].update(entry)
                continue
            slots[s["name"]] = entry
            slot_list.append((sub, s["name"]))
            if complete:
                errors += check_product_counts("%s.%s" % (sub, s["name"]), slots[s["name"]], tables, object_fields)
        done = {k: sorted(v) if isinstance(v, set) else v for k, v in pending.items()}
        subsystems[sub] = {
            "interface": "interfaces/%s.h" % sub, "struct": struct, "version": iface["version"], "status": spec["status"],
            "imports": spec.get("imports", []), "slots": slots,
        }
        if spec.get("issue"):
            subsystems[sub]["issue"] = spec["issue"]
        if unrecorded:
            subsystems[sub]["unrecorded"] = unrecorded
        left = {k: v for k, v in done.items() if v}
        if complete and left:
            for k, v in left.items():
                errors += ["%s is complete but has pending %s:\n    %s" % (sub, k, "\n    ".join(v))]
        if not complete:
            subsystems[sub]["pending"] = {
                "untracked": len(done["untracked"]), "ruleless": len(done["rules"]),
                "unresolvedSlots": done["functions"], "unannotatedArgs": done["args"],
            }
        for imp in spec.get("imports", []):
            seam, slot = imp.split(".")
            if seam not in {m[0] for m in members}:
                errors.append("%s imports from the unknown seam %s" % (sub, seam))
    stale = sorted("%s %s" % (r["kind"], r["target"]) for i, r in enumerate(ann["rules"]) if i not in used_rules)
    errors += ["the rule for %s matches no row of a subsystem function" % s for s in stale]
    for sub, spec in subsystems.items():
        for imp in spec["imports"]:
            seam, slot = imp.split(".")
            if slot not in subsystems.get(seam, {}).get("slots", {}) and slot not in subsystems.get(seam, {}).get("unrecorded", {}):
                errors.append("%s imports %s, which is no slot of the %s interface" % (sub, imp, seam))
    return {"fields": fields, "objects": objects, "object_fields": object_fields, "subsystems": subsystems, "slot_list": slot_list, "reach_table": reach_table,
            "arrays": arrays, "tables": tables, "vectors": vectors, "elems": elems, "selfs": selfs, "sites": sites, "code_tables": code_tables, "callbacks": callbacks,
            "alloc_tags": ann.get("alloc_tags", {}),
            "kind_types": {k: v["type"] for k, v in ann["objects"].items()}}, errors


def check_sim_names(sim: dict) -> list[str]:
    text = SIM_GLOBALS_H.read_text()
    known = load_sim_map()
    return ["SimGlobals has no member %s" % n for n in sim if n + ";" not in text and n not in known.fields]


def check_game_ref(name: str, spec: dict, leaf: dict | None, kinds: dict, errors: list[str]) -> dict:
    """The field entry of a Game member that is a ref: its target is a state kind with no table, named at first sight."""
    where = "Game.%s" % name
    target = spec.get("target")
    if set(spec) - {"kind", "target"}:
        errors.append("%s: a ref field of Game is {\"kind\": \"ref\", \"target\": KIND}" % where)
    found = kinds.get(target)
    if found is None or found["scope"] != "state":
        errors.append("%s: the target %s is no state kind of the schema" % (where, target))
    elif found.get("table") or found.get("inside") or found.get("kinds"):
        errors.append("%s: the target %s has a table, sites or kinds, and is named by them, not at first sight" % (where, target))
    if leaf is None or leaf["size"] != 4:
        raise Error("Game has no field %s of 4 bytes for a ref" % name)
    return {"name": "Game." + name, "base": "game", "offset": leaf["offset"], "size": 4, "kind": "ref", "target": target}


def field_table(game: dict, sim: dict, arrays: list[dict], errors: list[str], kinds: dict | None = None) -> list[dict]:
    schema = traceschema.Schema()
    layout = {l["name"]: l for l in schema.layout("Game")[0]}
    fields = []
    for name, kind in game.items():
        leaf = layout.get(name)
        spec = kind if isinstance(kind, dict) else None
        if spec is not None and spec.get("kind") == "ref":
            fields.append(check_game_ref(name, spec, leaf, kinds or {}, errors))
            continue
        if spec is not None:
            array = check_array("Game.%s" % name, spec, "Game", errors)
            array["member"] = name
            for f in array["length"]:
                if "member" in f and f["member"] not in layout:
                    errors.append("Game.%s: the length names %s, which is no member of Game" % (name, f["member"]))
            arrays.append(array)
            if leaf is None or (array["ref"] and leaf["size"] != 4) or (not array["ref"] and leaf["size"] % array["elem"] != 0):
                raise Error("Game has no field %s that suits the array spec" % name)
            fields.append({"name": "Game." + name, "base": "game", "offset": leaf["offset"], "size": leaf["size"], "kind": "array", "array": len(arrays)})
            continue
        if leaf is None or leaf["size"] != SIZES[kind]:
            raise Error("Game has no field %s of %d bytes" % (name, SIZES[kind]))
        fields.append({"name": "Game." + name, "base": "game", "offset": leaf["offset"], "size": leaf["size"], "kind": kind})
    for name, spec in sim.items():
        if isinstance(spec, dict):
            array = check_array("SimGlobals.%s" % name, spec, "SimGlobals", errors)
            array["member"] = name
            arrays.append(array)
            fields.append({"name": "SimGlobals." + name, "base": "sim", "offset": name, "size": 4 if array["ref"] else spec["size"], "kind": "array", "array": len(arrays)})
            continue
        kind, size = spec
        fields.append({"name": "SimGlobals." + name, "base": "sim", "offset": name, "size": size, "kind": kind})
    for i, name in enumerate(RNG_FIELDS):
        fields.append({"name": name, "base": "rng", "offset": i, "size": 4, "kind": "u32"})
    return fields


def upper(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "_", name).upper()


def camel(name: str) -> str:
    return "".join(part.capitalize() for part in name.split("_"))


def pascal(name: str) -> str:
    return name[:1].upper() + name[1:]


GENERATED = "// Generated by tools/replayschema.py. Do not edit."


def render_schema(ann: dict, model: dict) -> str:
    """src/harness/replay_schema.h: the tables of the real kinds, or those of tests/replay_schema_fixtures.h when BT_REPLAY_FIXTURES is defined."""
    text = schema_body(ann, model, "REPLAY_SCHEMA_H")
    head = "#define REPLAY_SCHEMA_H\n"
    text = text.replace(head, head + '\n#ifdef BT_REPLAY_FIXTURES\n#include "../../tests/replay_schema_fixtures.h"\n#else\n', 1)
    tail = "\n#endif\n"
    return text[:-len(tail)] + "\n#endif\n#endif\n"


def fixture_include(path: str) -> str:
    """An include of a header, spelled from src/harness, as the same file is spelled from tests/."""
    return os.path.relpath(os.path.normpath("src/harness/" + path), "tests").replace(os.sep, "/")


def render_fixture_schema(ann: dict, model: dict) -> str:
    """tests/replay_schema_fixtures.h: the tables of the real kinds and the test-only kinds, which only the tests' build includes."""
    text = schema_body(ann, model, "REPLAY_SCHEMA_FIXTURES_H")
    return re.sub(r'^#include "([^"]+)"$', lambda m: '#include "%s"' % fixture_include(m.group(1)), text, flags=re.M)


def schema_body(ann: dict, model: dict, guard: str) -> str:
    fields, objects, ofields, subsystems = model["fields"], model["objects"], model["object_fields"], model["subsystems"]
    sim = ann["sim_fields"]
    L = [GENERATED, "#ifndef " + guard, "#define " + guard, "", "#include <stddef.h>", ""]
    L += ['#include "../state/game_rand.h"', "#pragma pack(push, 1)", '#include "../game/game.h"', "#pragma pack(pop)"]
    for inc in ann["includes"]:
        if inc.startswith("pack1:"):
            L += ["#pragma pack(push, 1)", '#include "%s"' % inc[6:], "#pragma pack(pop)"]
        else:
            L.append('#include "%s"' % inc)
    L += ['#include "../state/sim_globals.h"', '#include "../state/block.h"', '#include "replay_types.h"', "",
          "#define REPLAY_FORMAT %d" % FORMAT, ""]
    arrays, tables, vectors = model["arrays"], model["tables"], model["vectors"]
    macros = list(sim)
    for t in tables:
        if t["sim"]:
            macros.append(t["member"].split(".")[0])
    for a in arrays:
        if a["owner"] == "SimGlobals":
            macros += [a["member"]] + [f["member"] for f in a["length"] if "member" in f]
    for t in tables:
        if t["sim"]:
            macros += [f["member"] for f in t["count"] + (t["capacity"] or []) if "member" in f]
    macros = list(dict.fromkeys(macros))
    for name in macros:
        L += ['#pragma push_macro("%s")' % name, "#undef %s" % name]
    for t in model["code_tables"]:
        L.append("static const void* const kReplayCode%s[] = {" % pascal(t["name"]))
        L += ["    (const void*)%s," % fn for fn in t["entries"]]
        L.append("};")
    L += ["", "enum ReplayObjectKindId {"]
    L += ["    ROK_%s," % upper(o["name"]) for o in objects]
    L += ["    ROK_COUNT", "};", "", "static const ReplayField kReplayFields[] = {"]
    for f in fields:
        base = {"game": "RB_GAME", "sim": "RB_SIM", "rng": "RB_RNG"}[f["base"]]
        offset = "offsetof(SimGlobals, %s)" % f["offset"] if f["base"] == "sim" else "%d" % f["offset"]
        extra = ", %d" % f["array"] if f.get("array") else ", 0, ROK_%s" % upper(f["target"]) if f["kind"] == "ref" else ""
        L.append('    { "%s", %s, %d, %s, RK_%s%s },' % (f["name"], offset, f["size"], base, FIELD_KINDS[f["kind"]], extra))
    L += ["};", "static const unsigned kReplayFieldCount = %d;" % len(fields), ""]
    L += ["static const ReplayArraySpec kReplayArrays[] = {"]
    for a in arrays:
        capacity = "0" if a["ref"] else "sizeof(((%s*)nullptr)->%s)" % (a["owner"], a["member"])
        L.append("    { %d, %d, %s, %s }," % (a["elem"], int(a["ref"]), capacity, render_length(a["length"], a["owner"])))
    if not arrays:
        L.append("    { 1, 0, 0, { { 1, 0, 0 }, { 1, 0, 0 } } },")
    L += ["};", "", "static const ReplayTableSpec kReplayTables[] = {"]
    for t in tables:
        capacity = ", %s, 1, %d" % (render_length(t["capacity"], t["owner"]), int(t["module"])) if t["capacity"] else ""
        L.append("    { %d, %d, offsetof(%s, %s), %s, %d%s }," % (int(t["sim"]), int(t["ref"]), t["owner"], t["member"], render_length(t["count"], t["owner"]), t["alloc"], capacity))
    if not tables:
        L.append("    { 0, 0, 0, { { 1, 0, 0 }, { 1, 0, 0 } }, 0 },")
    L += ["};", ""]
    kind_type = {o["name"]: o["type"] for o in objects}
    for a in arrays:
        L += factor_asserts(a["length"], a["owner"], "%s::%s" % (a["owner"], a["member"]))
        if a["ref"]:
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == sizeof(unsigned), "%s::%s is a Ref");' % (a["owner"], a["member"], a["owner"], a["member"]))
        else:
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) %% %d == 0, "%s::%s holds whole elements");' % (a["owner"], a["member"], a["elem"], a["owner"], a["member"]))
    for t in tables:
        L += factor_asserts(t["count"], t["owner"], t["kind"])
        room = t["capacity"] or t["count"]
        L += factor_asserts(t["capacity"] or [], t["owner"], t["kind"])
        constant = all("const" in f for f in room)
        if not t["ref"] and constant:
            n = room[0]["const"] * room[1]["const"]
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == %d * sizeof(%s), "the table of %s holds %d elements");' % (t["owner"], t["member"], n, kind_type[t["kind"]], t["kind"], n))
        elif t["ref"]:
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == sizeof(unsigned), "the table of %s is a Ref");' % (t["owner"], t["member"], t["kind"]))
    L.append("")
    for name in macros:
        L.append('#pragma pop_macro("%s")' % name)
    L += ["", "enum ReplayFieldIndex {"]
    L += ["    RF_%s," % upper(f["name"]) for f in fields]
    L += ["};", "", "static const ReplayVectorSpec kReplayVectors[] = {"]
    for v in vectors:
        L.append("    { %s, %s, sizeof(%s) }," % (VECTOR_MODES[v["mode"]], "ROK_" + upper(v["kind"]) if v["kind"] else "0", v["ctype"]))
    if not vectors:
        L.append("    { RV_OBJECT, 0, 1 },")
    L += ["};", "", "static const ReplayElemSpec kReplayElems[] = {"]
    for e in model["elems"]:
        stride = "sizeof(unsigned)" if e["kind"] == "refs" else "sizeof(((%s*)nullptr)->%s[0])" % (e["owner"], e["member"].split("[]")[0])
        L.append("    { %d, %s }," % (e["count"], stride))
    if not model["elems"]:
        L.append("    { 1, 1 },")
    L += ["};", "", "static const ReplaySelfSpec kReplaySelfs[] = {"]
    for x in model["selfs"]:
        stride = "sizeof(%s)" % kind_type[x["target"]] if x["index"] and x["target"] in kind_type else "0"
        L.append("    { offsetof(%s, %s), %s }," % (x["owner"], x["member"], stride))
    if not model["selfs"]:
        L.append("    { 0, 0 },")
    L += ["};", ""]
    scope = {"state": "RS_STATE", "index": "RS_INDEX", "value": "RS_VALUE"}
    kind_id = {o["name"]: "ROK_" + upper(o["name"]) for o in objects}
    types = {o["name"]: o["type"] for o in objects}
    table_id = {t["kind"]: str(i + 1) for i, t in enumerate(tables)}
    poly_kinds: list[str] = []
    poly_at: dict[str, tuple[int, int]] = {}
    for o in objects:
        if o.get("kinds"):
            poly_at[o["name"]] = (len(poly_kinds), len(o["kinds"]))
            poly_kinds += [kind_id[k] for k in o["kinds"]]
    sites: list[str] = []
    site_at: dict[str, tuple[int, int]] = {}
    for o in objects:
        listed = model["sites"].get(o["name"], [])
        site_at[o["name"]] = (len(sites) if listed else 0, len(listed))
        for site in listed:
            container = types[site["container"]]
            if site.get("ref"):
                stride = "sizeof(%s)" % o["type"]
            elif site.get("array"):
                stride = "sizeof(((%s*)nullptr)->%s[0])" % (container, site["array"])
            else:
                stride = "0"
            room = ", %d, %s, 1" % (int(site["module"]), render_length(site["room"], container)) if site.get("capped") else ""
            if site.get("capped"):
                L += factor_asserts(site["room"], container, "%s table" % o["name"])
            sites.append('    { %s, %s, offsetof(%s, %s), "%s", %s, %d, %d%s },' % (kind_id[o["name"]], kind_id[site["container"]], container, site["offset"], site["member"], stride, site["count"], int(bool(site.get("ref"))), room))
    L += ["static const ReplayInsideSite kReplayInsideSites[] = {"] + (sites or ['    { 0, 0, 0, "" },']) + ["};", "static const unsigned kReplayInsideSiteCount = %d;" % len(sites), "", "static const ReplayObjectKind kReplayObjectKinds[] = {"]
    for o in objects:
        first, count = poly_at.get(o["name"], (0, 0))
        inside_first, inside_count = site_at[o["name"]]
        L.append('    { "%s", sizeof(%s), %s, %s, %d, %d, %d, %d, %d },' % (o["name"], o["type"], scope[o["scope"]], table_id.get(o["name"], "RT_ORDER"), inside_first, inside_count, first, count, int(o.get("delta") is True)))
    L += ["};", "static const unsigned kReplayObjectKindCount = %d;" % len(objects), "",
          "static const unsigned char kReplayPolyKinds[] = { %s };" % (", ".join(poly_kinds) or "0"), "",
          "enum ReplayCallbackId {"]
    L += ["    RCB_%s," % upper(c["name"]) for c in model["callbacks"]]
    L += ["    RCB_COUNT", "};", ""]
    for c in model["callbacks"]:
        L.append("static const char* const kReplay%sMethods[] = { %s };" % (c["name"], ", ".join('"%s"' % m["name"] for m in c["methods"])))
    L.append("static const ReplayCallbackType kReplayCallbacks[] = {")
    for c in model["callbacks"]:
        L.append('    { "%s", "%s", %d, kReplay%sMethods },' % (c["name"], c["handle"], len(c["methods"]), c["name"]))
    if not model["callbacks"]:
        L.append('    { "", "", 0, nullptr },')
    L += ["};", "", "static const ReplayCodeTable kReplayCodeTables[] = {"]
    for t in model["code_tables"]:
        L.append("    { \"%s\", kReplayCode%s, %d }," % (t["name"], pascal(t["name"]), len(t["entries"])))
    if not model["code_tables"]:
        L.append("    { \"\", nullptr, 0 },")
    L += ["};", "static const unsigned kReplayCodeTableCount = %d;" % len(model["code_tables"]), "",
          "static const ReplayAllocTag kReplayAllocTags[] = {"]
    for tag, spec in model["alloc_tags"].items():
        L.append('    { "%s", ROK_%s, %d },' % (tag, upper(spec["kind"]), spec["size"]))
    if not model["alloc_tags"]:
        L.append('    { "", 0, 0 },')
    L += ["};", "static const unsigned kReplayAllocTagCount = %d;" % len(model["alloc_tags"]), "",
          "static const ReplayObjectField kReplayObjectFields[] = {"]
    position = {(f["object"], f["name"]): i for i, f in enumerate(ofields)}
    range_end, range_begin = {}, {}
    for f in ofields:
        if f.get("range_end_of"):
            begin = position[(f["object"], f["range_end_of"])]
            range_end[begin] = position[(f["object"], f["name"])] + 1
            range_begin[position[(f["object"], f["name"])]] = begin + 1
    for i, f in enumerate(ofields):
        member = embeds_member(f).replace("[]", "[0]") if f["kind"] == "embeds" else f["name"] if f["kind"] == "self" else f.get("member", f["name"])
        t = types[f["object"]]
        size = f.get("size")
        size_text = str(size) if size else "sizeof(void*)" if f["kind"] == "ptr" else "sizeof(((%s*)nullptr)->%s)" % (t, member)
        target = "RCB_%s" % upper(f["callback"]) if field_kind(f) in ("cbref", "cbptr") else kind_id[f["target"]] if f["kind"] in ("ref", "embed", "unitid", "ptr", "refs", "embeds") or (f["kind"] == "vector" and f.get("target")) else kind_id.get(f.get("target"), "0") if field_kind(f) == "selfindex" else ("ROK_WEAPONDEF" if f["kind"] == "weapon" else str(f.get("codeTable", 0)) if f["kind"] == "code" else "0")
        extra = ", %d" % (f.get("array") or f.get("vector") or f.get("elems") or f.get("selfspec")) if f.get("array") or f.get("vector") or f.get("elems") or f.get("selfspec") else ""
        when = "{ %d, %d, %s, %du, %d }" % (f["condField"], f.get("condVia", 0), kind_id[f["condKind"]], f.get("condMask", 0xFFFFFFFF), f["condValue"]) if f.get("condField") else "{}"
        L.append('    { "%s", offsetof(%s, %s), %s, %s, RK_%s, %s, %d, %d, %s, %d, %d%s },' % (
            f["name"], t, member, size_text, kind_id[f["object"]], FIELD_KINDS[field_kind(f)], target, range_end.get(i, 0), range_begin.get(i, 0),
            when, f.get("reachFirst", 0), f.get("reachCount", 0), extra))
    L += ["};", "static const unsigned kReplayObjectFieldCount = %d;" % len(ofields), ""]
    L += ["static const unsigned short kReplayReachFields[] = { %s };" % ", ".join(str(i) for i in model["reach_table"] or [0]), ""]
    for f in ofields:
        t = types[f["object"]]
        member = f.get("member", f["name"])
        if f["kind"] == "self":
            x = model["selfs"][f["selfspec"] - 1]
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == sizeof(unsigned), "%s::%s is a Ref");' % (t, f["name"], t, f["name"]))
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) >= 1, "%s::%s is a member of its own object");' % (t, x["member"], t, x["member"]))
            continue
        if f["kind"] == "refs":
            ctype = f.get("ctype", types[f["target"]])
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == %s * sizeof(unsigned), "%s::%s holds %s refs");' % (t, member, f["count"], t, f["name"], f["count"]))
            L.append('static_assert(__is_same(decltype(&((%s*)nullptr)->%s[0]), Ref<%s>*), "%s::%s holds Ref<%s>");' % (t, member, ctype, t, f["name"], ctype))
            continue
        if field_kind(f) in ("cbref", "cbptr"):
            held = f["callback"] + "*" if f["kind"] == "ptr" else "Ref<%s>" % f["callback"]
            L.append('static_assert(__is_same(decltype(((%s*)nullptr)->%s), %s), "%s::%s holds a %s");' % (t, member, held, t, f["name"], held))
            continue
        if f["kind"] == "ref" and f.get("ctype"):
            L.append('static_assert(__is_same(decltype(((%s*)nullptr)->%s), Ref<%s>), "%s::%s is a Ref<%s>");' % (t, member, f["ctype"], t, f["name"], f["ctype"]))
        if f["kind"] == "embeds":
            mem = embeds_member(f)
            array = mem.split("[]")[0]
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == %s * sizeof(((%s*)nullptr)->%s[0]), "%s::%s holds %s elements");' % (t, array, f["count"], t, array, t, f["name"], f["count"]))
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == sizeof(%s), "%s::%s has the size of its kind");' % (t, mem.replace("[]", "[0]"), types[f["target"]], t, f["name"]))
            continue
        if f["kind"] == "embed":
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == sizeof(%s), "%s::%s has the size of its kind");' % (t, member, types[f["target"]], t, f["name"]))
        elif f["kind"] == "bytes":
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == %d, "%s::%s has the size the schema says");' % (t, member, f["size"], t, f["name"]))
        elif f["kind"] == "array":
            continue
        elif f["kind"] == "vector":
            v = vectors[f["vector"] - 1]
            access = "decltype(((%s*)nullptr)->%s)" % (t, member)
            L.append('static_assert(sizeof(%s) == 16, "%s::%s is a block vector");' % (access, t, f["name"]))
            L.append('static_assert(__is_same(%s::value_type, %s), "%s::%s holds %s");' % (access, v["ctype"], t, f["name"], v["ctype"]))
        elif f["kind"] == "code":
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == sizeof(void*), "%s::%s is a code pointer");' % (t, member, t, f["name"]))
        elif f["kind"] == "ptr":
            L.append('static_assert(__is_pointer(decltype(((%s*)nullptr)->%s)), "%s::%s is a raw pointer");' % (t, member, t, f["name"]))
        elif f.get("size") is None:
            want = {"i32": 4, "u32": 4, "u16": 2, "i16": 2, "f32": 4, "flags": 4, "ref": 4, "point16": 4, "weapon": 4, "u8": 1, "f64": 8, "unitid": 2}[f["kind"]]
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == %d, "%s::%s has the size the schema says");' % (t, member, want, t, f["name"]))
    for o in objects:
        for k in o.get("kinds", []):
            L.append('static_assert(__is_base_of(%s, %s), "%s is a kind of %s");' % (o["type"], types[k], k, o["name"]))
        if o.get("vtable"):
            L.append('static_assert(__is_polymorphic(%s), "%s has a vtable");' % (o["type"], o["name"]))
        if o.get("vptr"):
            L.append('static_assert(!__is_polymorphic(%s) && sizeof(%s) >= sizeof(void*), "%s has a vptr that the code sets by hand");' % (o["type"], o["type"], o["name"]))
    L += ["", "enum ReplaySubsystemId {"]
    L += ["    SUB_%s," % upper(s) for s in subsystems]
    L += ["    SUB_COUNT", "};", "", "enum ReplaySlotId {"]
    for sub, slot, _ in slot_table(subsystems):
        L.append("    RSL_%s_%s," % (upper(sub), upper(slot)))
    L += ["    RSL_COUNT", "};", ""]
    for sub, spec in subsystems.items():
        for slot, s in spec["slots"].items():
            base = "kReplay%s%s" % (re.sub(r"[^A-Za-z0-9]", "", sub.title()), slot)
            if s["allOf"]:
                L.append("static const unsigned char %sAllOf[] = { %s };" % (base, ", ".join("ROK_" + upper(k) for k in s["allOf"])))
            for tag, names, kindname, idx in (("Reads", s["reads"], "unsigned char", [fields_index(fields, n) for n in s["reads"]]),
                                              ("Writes", s["writes"], "unsigned char", [fields_index(fields, n) for n in s["writes"]]),
                                              ("ObjReads", s["objectReads"], "unsigned short", s["objectReads"]),
                                              ("ObjWrites", s["objectWrites"], "unsigned short", s["objectWrites"])):
                if names:
                    L.append("static const %s %s%s[] = { %s };" % (kindname, base, tag, ", ".join(str(i) for i in idx)))
    L += ["", "static const ReplaySlotSets kReplaySlots[] = {"]
    for sub, slot, s in slot_table(subsystems):
        base = "kReplay%s%s" % (re.sub(r"[^A-Za-z0-9]", "", sub.title()), slot)

        def ref(tag, items):
            return ("%s%s" % (base, tag) if items else "nullptr", len(items))
        r, w = ref("Reads", s["reads"]), ref("Writes", s["writes"])
        ro, wo = ref("ObjReads", s["objectReads"]), ref("ObjWrites", s["objectWrites"])
        every = ref("AllOf", s["allOf"])
        L.append('    { "%s", SUB_%s, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d },' % (slot, upper(sub), r[0], r[1], w[0], w[1], ro[0], ro[1], wo[0], wo[1], every[0], every[1]))
    L += ["};", "static const unsigned kReplaySlotCount = RSL_COUNT;", "", "static const ReplaySubsystem kReplaySubsystems[] = {"]
    first = 0
    for sub, spec in subsystems.items():
        n = len(spec["slots"])
        L.append('    { "%s", %d, %d, %d, %d, %d },' % (sub, spec["version"][0], spec["version"][1], int(spec["status"] == "complete"), first, n))
        first += n
    L += ["};", "static const unsigned kReplaySubsystemCount = SUB_COUNT;", "", "#endif", ""]
    return "\n".join(L)


def imported_unrecorded(spec: dict) -> dict:
    """The unrecorded slots of a subsystem that some subsystem imports: they keep their arguments and result, for the import hook."""
    return {n: u for n, u in spec.get("unrecorded", {}).items() if "args" in u}


def slot_table(subsystems: dict):
    """Every slot with an id: the recorded slots in order, then the imported unrecorded ones, which no call line names."""
    for sub, spec in subsystems.items():
        for slot, s in spec["slots"].items():
            yield sub, slot, s
    for sub, spec in subsystems.items():
        for slot, s in imported_unrecorded(spec).items():
            yield sub, slot, s


def fields_index(fields: list[dict], name: str) -> int:
    for i, f in enumerate(fields):
        if f["name"] == name:
            return i
    raise Error("no field " + name)


def render_hooks(ann: dict, model: dict) -> str:
    subsystems = model["subsystems"]
    objects = {o["name"]: o for o in model["objects"]}
    L = [GENERATED, "", "// included by replay.cpp inside its anonymous namespace", ""]
    imported = {}
    for sub, spec in subsystems.items():
        for imp in spec["imports"]:
            imported.setdefault(imp, []).append(sub)
    for sub, spec in subsystems.items():
        L.append("const %s* s_inner_%s;" % (spec["struct"], sub))
        L.append("%s s_rec_%s;" % (spec["struct"], sub))
    L.append("")
    for sub, slot, s in slot_table(subsystems):
        L += wrapper(sub, subsystems[sub], slot, s, objects, sub + "." + slot in imported, slot in subsystems[sub].get("unrecorded", {}))
    L.append("bool IsImportOf(unsigned owner, unsigned slot)")
    L.append("{")
    L.append("    switch (owner) {")
    for sub, spec in subsystems.items():
        if spec["imports"]:
            L.append("    case SUB_%s:" % upper(sub))
            for imp in spec["imports"]:
                seam, slot = imp.split(".")
                L.append("        if (slot == RSL_%s_%s)" % (upper(seam), upper(slot)))
                L.append("            return true;")
            L.append("        return false;")
    L += ["    default:", "        return false;", "    }", "}", ""]
    L += ["bool IsNestedSlot(unsigned slot)", "{", "    switch (slot) {"]
    for sub, spec in subsystems.items():
        for slot, s in spec["slots"].items():
            if nestable(slot, s):
                L.append("    case RSL_%s_%s:" % (upper(sub), upper(slot)))
    L += ["        return true;", "    default:", "        return false;", "    }", "}", ""]
    L.append("void HookSeams(const bool* on)")
    L.append("{")
    L.append("    bool any = false;")
    L.append("    for (unsigned i = 0; i < SUB_COUNT; i++)")
    L.append("        any = any || on[i];")
    for sub, spec in subsystems.items():
        L.append("    s_inner_%s = g_seams.%s;" % (sub, sub))
        L.append("    s_rec_%s = *s_inner_%s;" % (sub, sub))
    for sub, slot, s in slot_table(subsystems):
        spec = subsystems[sub]
        unrecorded = slot in spec.get("unrecorded", {})
        owners = ([] if unrecorded else ["on[SUB_%s]" % upper(sub)]) + ["on[SUB_%s]" % upper(o) for o in imported.get(sub + "." + slot, [])]
        if not unrecorded and nestable(slot, s):
            owners.append("any")
        L.append("    if (%s)" % " || ".join(owners))
        L.append("        s_rec_%s.%s = Rec%s%s;" % (sub, slot, camel(sub), slot))
    for sub in subsystems:
        L.append("    g_seams.%s = &s_rec_%s;" % (sub, sub))
    L += ["}", ""]
    return "\n".join(L)


def wrapper(sub: str, spec: dict, slot: str, s: dict, objects: dict, is_import: bool, unrecorded: bool = False) -> list[str]:
    params = ", ".join("%s a%d" % (raw_type(s, i), i) for i in range(len(s["args"])))
    call_args = ", ".join("a%d" % i for i in range(len(s["args"])))
    rid = "RSL_%s_%s" % (upper(sub), upper(slot))
    ret = s["ret"]
    names = [a["name"] for a in s["args"]]
    for a in s["args"]:
        if a.get("length") in names:
            a["lengthIndex"] = names.index(a["length"])
        if a.get("count") in names:
            a["countIndex"] = names.index(a["count"])
    L = ["%s %s Rec%s%s(%s)" % (ret, s["conv"], camel(sub), slot, params), "{"]
    L.append("    ReplayRoots roots;")
    for i, a in enumerate(s["args"]):
        if a["mode"] == "object":
            L.append("    roots.Add(ROK_%s, a%d);" % (upper(a["kind"]), i))
    L.append("    SlotScope scope(%s, SUB_%s, roots%s);" % (rid, upper(sub), ", false" if unrecorded else ""))
    callback_args = [i for i, a in enumerate(s["args"]) if a["mode"] == "callback"]
    held_args = [] if is_import else callback_args
    for i in held_args:
        L.append("    ReplayCallbackScope held%d;" % i)
    L.append("    if (scope.Open()) {")
    for i, a in enumerate(s["args"]):
        if i in held_args:
            L.append('        ArgCallback(s_call, "%s", held%d, RCB_%s, a%d);' % (a["line"], i, upper(a["callback"]), i))
        else:
            L += ["        " + x for x in arg_out(a, i, "a", objects)]
    L.append("    }")
    nest = nestable(slot, s) and not unrecorded
    if is_import or nest:
        for i in callback_args:
            L.append("    ReplayCallbackScope cb%d;" % i)
        for i, a in enumerate(s["args"]):
            if a["mode"] in ("bytes-in", "bytes-out"):
                L += ["    " + x for x in length_parts(a, i)[0]]
        L.append("    ReplayText line;")
        if is_import and nest:
            condition = "scope.Import() || scope.Nested()"
            head = 'line.Add(scope.Nested() ? "\\n    c %s.%s" : "  i %s.%s");' % (sub, slot, sub, slot)
        elif is_import:
            condition = "scope.Import()"
            head = 'line.Add("  i %s.%s");' % (sub, slot)
        else:
            condition = "scope.Nested()"
            head = 'line.Add("\\n    c %s.%s");' % (sub, slot)
        L.append("    if (%s) {" % condition)
        L.append("        " + head)
        for i, a in enumerate(s["args"]):
            L += ["        " + x for x in arg_out(a, i, "i", objects)]
        L.append("    }")
    inner = "s_inner_%s->%s(%s)" % (sub, slot, call_args)
    res = s["result"]
    if ret == "void":
        L.append("    " + inner + ";")
    else:
        L.append("    %s result = %s;" % (ret, inner))
    if is_import:
        for i in callback_args:
            L.append("    cb%d.Stop();" % i)
        L.append("    if (scope.Import()) {")
        if res is not None:
            L += ["        " + x for x in result_out(res, objects, "i")]
        for i in callback_args:
            L.append("        line.AddText(cb%d.Lines());" % i)
        for i, a in enumerate(s["args"]):
            if a["mode"] == "bytes-out":
                L.append('        ImportOutBytes(line, "%s", a%d, (int)result, %s);' % (a["line"], i, length_parts(a, i)[1]))
            elif a["mode"] == "vec3" and a.get("dir") == "out":
                L.append('        ImportOutVec3(line, "%s", %s);' % (a["line"], arg_var(a, i)))
            elif a["mode"] == "units":
                L.append('        ImportOutUnits(line, "%s", a%d, kReplayReachFields + %d, %d);' % (a["line"], i, a["reachFirst"], a["reachCount"]))
            elif a["mode"] == "vector":
                L.append('        ImportOutValues(line, "%s", ROK_%s, a%d);' % (a["line"], upper(a["elem"]["kind"]), i))
            elif a["mode"] == "struct" and a.get("dir") == "inout":
                L.append('        ImportOutInOut(line, "%s", ROK_%s, %s);' % (a["line"], upper(a["kind"]), arg_var(a, i)))
            elif is_scalar_inout(a):
                L.append('        ImportOutInOutScalar(line, "%s", %s);' % (a["line"], arg_var(a, i)))
        L.append("        FinishImport(line);")
        L.append("    }")
    if nest:
        L.append("    if (scope.Nested()) {")
        if res is not None:
            L += ["        " + x for x in result_out(res, objects, "i")]
        L.append("        FinishNested(line);")
        L.append("    }")
    for i in held_args:
        L.append("    held%d.Stop();" % i)
    L.append("    if (scope.Open()) {")
    for i in held_args:
        L.append("        ArgCallbackExit(s_tail, held%d);" % i)
    for i, a in enumerate(s["args"]):
        L += ["        " + x for x in arg_out(a, i, "o", objects)]
    if res is not None and res["mode"] in ("scalar", "object", "value"):
        L += ["        " + x for x in result_out(res, objects, "o")]
    L.append("    }")
    if ret != "void":
        L.append("    return result;")
    L += ["}", ""]
    return L


def nestable(slot: str, s: dict) -> bool:
    """A command slot can be made inside ExecuteCommandText: its call is a c line under the import, its arguments and result plain values."""
    res = s["result"]
    return slot.startswith("Cmd") and all(a["mode"] in ("scalar", "argv", "text", "stream") for a in s["args"]) and (res is None or res["mode"] in ("scalar", "text"))


def raw_type(s: dict, i: int) -> str:
    return s["rawParams"][i] if "rawParams" in s else s["args"][i]["type"]


def arg_var(a: dict, i: int) -> str:
    """The generated parameter i as the modes of its argument see it: cast to its `cast` type when it has one."""
    return "((%s)a%d)" % (a["cast"], i) if a.get("cast") else "a%d" % i


def arg_out(a: dict, i: int, phase: str, objects: dict) -> list[str]:
    """Lines of C++ recording argument i. phase: 'a' outer entry, 'i' import line, 'o' outer exit."""
    mode, var, line = a["mode"], arg_var(a, i), a["line"]
    out = phase == "o"
    if mode == "scalar" or mode == "stream":
        if phase == "a":
            return ['ArgValue(s_call, "%s", %s);' % (line, var)]
        if phase == "i":
            return ["ImportValue(line, %s);" % var]
    elif mode == "text":
        if phase == "a":
            return ['ArgValue(s_call, "%s", (const char*)%s);' % (line, var)]
        if phase == "i":
            return ["ImportValue(line, (const char*)%s);" % var]
    elif mode == "handle":
        if phase == "a":
            return ['ArgHandle(s_call, "%s", %s, %s);' % (line, handle_id(a["kind"]), var)]
        if phase == "i":
            return ["ImportHandle(line, %s, %s);" % (handle_id(a["kind"]), var)]
    elif mode == "object":
        if phase == "a":
            return ['ArgObject(s_call, "%s", ROK_%s, %s);' % (line, upper(a["kind"]), var)]
        if phase == "i":
            return ["ImportObject(line, ROK_%s, %s);" % (upper(a["kind"]), var)]
    elif mode == "struct":
        if a.get("dir") == "inout":
            if phase == "a":
                return ['ArgInOut(s_call, "%s", ROK_%s, %s);' % (line, upper(a["kind"]), var)]
            if phase == "i":
                return ["ImportInOut(line, ROK_%s, %s);" % (upper(a["kind"]), var)]
            return ['ArgInOutExit(s_tail, "%s", ROK_%s, %s);' % (line, upper(a["kind"]), var)]
        if a.get("dir", "in") == "in" and phase == "a":
            return ['ArgStruct(s_call, "a", "%s", ROK_%s, %s);' % (line, upper(a["kind"]), var)]
        if a.get("dir") == "out" and phase == "o":
            guard = "if (result != 0) " if a.get("when") else ""
            return ['%sArgStruct(s_tail, "o", "%s", ROK_%s, %s);' % (guard, line, upper(a["kind"]), var)]
    elif mode == "structs":
        if phase == "a":
            return ['ArgStructs(s_call, "%s", ROK_%s, %s, (int)a%d);' % (line, upper(a["kind"]), var, a.get("countIndex", 0))]
    elif is_scalar_inout(a):
        if phase == "a":
            return ['ArgInOutScalar(s_call, "%s", %s);' % (line, var)]
        if phase == "i":
            return ["ImportInOutScalar(line, %s);" % var]
        return ['ArgInOutScalarExit(s_tail, "%s", %s);' % (line, var)]
    elif mode == "value":
        if phase == "a":
            return ['ArgValueStruct(s_call, "a", "%s", ROK_%s, &%s);' % (line, upper(a["kind"]), var)]
        if phase == "i":
            return ["ImportValueStruct(line, ROK_%s, &%s);" % (upper(a["kind"]), var)]
    elif mode == "argv":
        if phase == "a":
            return ["ArgArgv(s_call, %s);" % var]
        if phase == "i":
            return ["ImportArgv(line, %s);" % var]
    elif mode == "weapon":
        if phase == "a":
            return ['ArgWeapon(s_call, "%s", %s);' % (line, var)]
        if phase == "i":
            return ["ImportWeapon(line, %s);" % var]
    elif mode == "vec3":
        if a.get("dir") == "out":
            return []
        if phase == "a":
            return ['ArgVec3(s_call, "%s", %s);' % (line, var)]
        if phase == "i":
            return ['ImportVec3%s(line, "%s", %s);' % ("Xz" if a.get("axes") == "xz" else "", a.get("label", line), var)]
    elif mode == "units" or mode == "vector":
        return []
    elif mode == "callback":
        if phase == "i":
            return ["ImportCallback(line, cb%d, RCB_%s, a%d);" % (i, upper(a["callback"]), i)]
    elif mode == "bytes-in":
        if phase == "a":
            return ['ArgPending(s_call, "%s");' % line]
        if phase == "i":
            return ["ImportBytes(line, %s, %s);" % (var, length_parts(a, i)[1])]
    elif mode == "bytes-out":
        if phase == "a":
            return ['ArgPending(s_call, "%s");' % line]
        if phase == "i":
            return ["ImportPending(line);"]
    elif mode == "pending":
        if phase == "a":
            return ['ArgPending(s_call, "%s");' % line]
        if phase == "i":
            return ["ImportPending(line);"]
    del out
    return []


def handle_id(kind: str) -> str:
    return {"file": "OBJ_FILE", "rec": "OBJ_RECORD", "bank": "OBJ_BANK"}[kind]


def result_out(res: dict, objects: dict, phase: str) -> list[str]:
    mode = res["mode"]
    if phase == "o":
        if mode == "scalar":
            return ['ArgValue(s_tail, "result", result, "o");']
        if mode == "object":
            return ['ArgObject(s_tail, "result", ROK_%s, result, "o");' % upper(res["kind"])]
        if mode == "value":
            return ['ArgValueStruct(s_tail, "o", "result", ROK_%s, &result);' % upper(res["kind"])]
        return []
    lines = []
    if mode == "scalar":
        lines.append("ImportResult(line, result);")
    elif mode == "text":
        lines.append("ImportResult(line, (const char*)result);")
    elif mode == "object" and res.get("reach"):
        reach = ", ".join('"%s"' % f for f in res["reach"])
        lines.append("ImportResultObjectReach(line, ROK_%s, result, %d, %s);" % (upper(res["kind"]), len(res["reach"]), reach))
    elif mode == "object":
        lines.append("ImportResultObject(line, ROK_%s, result);" % upper(res["kind"]))
    elif mode == "value":
        lines.append("ImportResultValue(line, ROK_%s, &result);" % upper(res["kind"]))
    elif mode == "weapon":
        reach = ", ".join('"%s"' % f for f in res.get("reach", []))
        lines.append("ImportResultWeapon(line, result, %d%s);" % (len(res.get("reach", [])), (", " + reach) if reach else ""))
    else:
        lines.append("ImportResultPending(line);")
    return lines


def render_slots(ann: dict, model: dict) -> str:
    subsystems = model["subsystems"]
    L = [GENERATED, "", "// included by replay_native_test.cpp after its helpers", ""]
    ready = {sub: spec for sub, spec in subsystems.items() if spec["status"] == "complete"}
    imports = {}
    for sub, spec in ready.items():
        for imp in spec["imports"]:
            seam, slot = imp.split(".")
            if slot not in imports.setdefault(seam, []):
                imports[seam].append(slot)
    for seam, slots in imports.items():
        spec = subsystems[seam]
        for slot in slots:
            L += import_stub(seam, slot, {**spec["slots"], **imported_unrecorded(spec)}[slot])
    for sub, spec in ready.items():
        L.append("%s g_native_%s = {" % (spec["struct"], sub))
        for slot in (x["name"] for x in parse_interface(sub, spec["struct"])["slots"]):
            L.append("    %s," % (spec["slots"][slot] if slot in spec["slots"] else spec["unrecorded"][slot])["native"])
        L += ["};", ""]
    for sub, spec in ready.items():
        for slot, s in spec["slots"].items():
            L += runner(sub, slot, s)
    L += nested_player(ready)
    L.append("bool RunSlot(const char* subsystem, const char* slot)")
    L.append("{")
    for sub, spec in ready.items():
        for slot in spec["slots"]:
            L.append('    if (strcmp(subsystem, "%s") == 0 && strcmp(slot, "%s") == 0) {' % (sub, slot))
            L.append("        Run_%s_%s();" % (sub, slot))
            L.append("        return true;")
            L.append("    }")
    L += ["    return false;", "}", ""]
    L.append("void RunAfter(const char* subsystem, const char* slot)")
    L.append("{")
    for sub, spec in ready.items():
        for slot, s in spec["slots"].items():
            res = s["result"]
            if any(a["mode"] == "struct" and a.get("dir", "in") in ("out", "inout") or is_scalar_inout(a) for a in s["args"]) or (res is not None and res["mode"] in ("scalar", "object", "value")):
                L.append('    if (strcmp(subsystem, "%s") == 0 && strcmp(slot, "%s") == 0)' % (sub, slot))
                L.append("        After_%s_%s();" % (sub, slot))
    L += ["}", ""]
    for seam in imports:
        L.append("%s g_import_%s;" % (subsystems[seam]["struct"], seam))
    L += ["", "void BindSeams()", "{"]
    for sub in ready:
        L.append("    g_seams.%s = &g_native_%s;" % (sub, sub))
    for seam, slots in imports.items():
        if seam not in ready:
            L.append("    g_seams.%s = &g_import_%s;" % (seam, seam))
            for slot in slots:
                L.append("    g_import_%s.%s = Imp_%s_%s;" % (seam, slot, seam, slot))
    L += ["}", ""]
    return "\n".join(L)


def nested_player(ready: dict) -> list[str]:
    """The replayer's side of ExecuteCommandText: it makes each command the import ran through the seam of the subsystem."""
    L = ["void PlayNestedCalls(const char* import)", "{", "    NestedScope scope;", "    for (int k = 0; k < scope.count; k++) {",
         "        scope.Restore();", "        const char* args = nullptr;", "        const char* name = NestedName(scope.line[k], &args);"]
    for sub, spec in ready.items():
        for slot, s in spec["slots"].items():
            if not nestable(slot, s):
                continue
            L.append('        if (CallbackIs(name, "%s.%s")) {' % (sub, slot))
            call = []
            for i, a in enumerate(s["args"]):
                v = "p%d" % i
                if a["mode"] == "argv":
                    L.append("            ArgvHolder %s;\n            ParseArgsList(&args, %s);" % (v, v))
                    call.append("&%s.args" % v)
                elif a["mode"] == "text":
                    L.append("            char %s_text[256];\n            CallbackText(&args, %s_text, sizeof %s_text);" % (v, v, v))
                    call.append("%s_text" % v)
                elif a["mode"] == "stream":
                    L.append("            RandStream %s = (RandStream)CallbackScalar<int>(&args);" % v)
                    call.append(v)
                else:
                    L.append("            %s %s = CallbackScalar<%s>(&args);" % (a["type"], v, a["type"]))
                    call.append(v)
            expr = "g_seams.%s->%s(%s)" % (sub, slot, ", ".join(call))
            res = s["result"]
            if res is None:
                L += ["            %s;" % expr, "            scope.Restore();", "            CallbackDone(import, k, args, nullptr);"]
            else:
                L += ["            %s result = %s;" % (s["ret"], expr), "            ReplayText have;", "            ReplayValue(have, result);",
                      "            scope.Restore();", "            CallbackDone(import, k, args, have.Data());", "            have.Free();"]
            L += ["            continue;", "        }"]
    L += ["        CallbackUnknown(import, k, name);", "    }", "}", ""]
    return L


def import_stub(seam: str, slot: str, s: dict) -> list[str]:
    params = ", ".join("%s a%d" % (s["params"][i], i) for i in range(len(s["args"])))
    names = [a["name"] for a in s["args"]]
    for a in s["args"]:
        if a.get("length") in names:
            a["lengthIndex"] = names.index(a["length"])
    L = ["%s %s Imp_%s_%s(%s)" % (s["ret"], s["conv"], seam, slot, params), "{"]
    L += ["    ReplayText t;", '    StartImport(t, "%s.%s");' % (seam, slot)]
    for i, a in enumerate(s["args"]):
        if a["mode"] in ("bytes-in", "bytes-out"):
            L += ["    " + x for x in length_parts(a, i)[0]]
    for i, a in enumerate(s["args"]):
        m, v = a["mode"], arg_var(a, i)
        if m in ("scalar", "stream"):
            L.append("    Space(t);\n    ReplayValue(t, %s);" % v)
        elif m == "text":
            L.append("    Space(t);\n    t.AddString((const char*)%s);" % v)
        elif m == "handle":
            L.append("    Space(t);\n    AddObject(t, %s, %s);" % (handle_id(a["kind"]), v))
        elif m == "weapon":
            L.append("    Space(t);\n    ReplayAddWeapon(t, %s);" % v)
        elif m == "vec3" and a.get("dir") == "out":
            pass
        elif m == "units" or m == "vector":
            pass
        elif m == "struct" and a.get("dir") == "inout":
            L.append("    Space(t);\n    AddInOut(t, ROK_%s, %s);" % (upper(a["kind"]), v))
        elif is_scalar_inout(a):
            L.append("    Space(t);\n    AddInOutScalar(t, %s);" % v)
        elif m == "vec3":
            L.append('    Space(t);\n    t.Add("%s=");\n    AddVec3%s(t, %s);' % (a.get("label", a["line"]), "Xz" if a.get("axes") == "xz" else "", v))
        elif m == "callback":
            L.append("    Space(t);\n    AddObject(t, OBJ_CALLBACK + RCB_%s, %s);" % (upper(a["callback"]), v))
        elif m == "object":
            L.append("    Space(t);\n    ReplayAddObjectName(t, g_objects, ROK_%s, %s, false);" % (upper(a["kind"]), v))
        elif m == "bytes-in":
            L.append("    Space(t);\n    AddBytes(t, %s, %s);" % (v, length_parts(a, i)[1]))
        elif m == "value":
            L.append("    Space(t);\n    AddValueStruct(t, ROK_%s, &%s);" % (upper(a["kind"]), v))
        else:
            L.append("    Space(t);\n    t.Add(\"?\");")
    L.append("    const char* r = Enter(t);")
    for i, a in enumerate(s["args"]):
        if a["mode"] == "callback":
            L.append('    PlayCallbacks_%s(a%d, OBJ_CALLBACK + RCB_%s, "%s.%s");' % (a["callback"], i, upper(a["callback"]), seam, slot))
    for i, a in enumerate(s["args"]):
        if a["mode"] == "bytes-out":
            L.append('    ApplyBytes(a%d, %s, "%s");' % (i, length_parts(a, i)[1], a["line"]))
        elif a["mode"] == "vec3" and a.get("dir") == "out":
            L.append('    ApplyVec3(%s, "%s");' % (arg_var(a, i), a["line"]))
        elif a["mode"] == "units":
            L.append('    ApplyUnits(a%d, "%s");' % (i, a["line"]))
        elif a["mode"] == "vector":
            L.append('    ApplyValues(a%d, "%s", ROK_%s);' % (i, a["line"], upper(a["elem"]["kind"])))
        elif a["mode"] == "struct" and a.get("dir") == "inout":
            L.append('    ApplyInOut(%s, "%s", ROK_%s);' % (arg_var(a, i), a["line"], upper(a["kind"])))
        elif is_scalar_inout(a):
            L.append('    ApplyInOutScalar(%s, "%s");' % (arg_var(a, i), a["line"]))
    res = s["result"]
    if res is None:
        L.append("    (void)r;")
    elif res["mode"] == "scalar":
        L.append("    return ParseResult<%s>(r);" % s["ret"])
    elif res["mode"] == "value":
        L.append("    return ParseValueText<%s>(ROK_%s, r);" % (s["ret"], upper(res["kind"])))
    elif res["mode"] == "object":
        L.append("    %s o = (%s)ReplayResolveObject(r, nullptr, g_objects, AllocObject);" % (s["ret"], s["ret"]))
        for f in res.get("reach", []):
            L.append('    ApplyReach(o, ROK_%s, ".%s");' % (upper(res["kind"]), f))
        L.append("    return o;")
    elif res["mode"] == "weapon":
        L.append("    %s w = (%s)ReplayParseWeapon(r);" % (s["ret"], s["ret"]))
        for f in res.get("reach", []):
            L.append('    ApplyReach(w, ROK_WEAPONDEF, ".%s");' % f)
        L.append("    return w;")
    else:
        L.append("    (void)r;\n    return (%s)0;" % s["ret"])
    L += ["}", ""]
    return L


def runner(sub: str, slot: str, s: dict, target: str | None = None) -> list[str]:
    names = [a["name"] for a in s["args"]]
    for a in s["args"]:
        if a.get("count") in names:
            a["countIndex"] = names.index(a["count"])
    head = []
    L = ["void Run_%s_%s()" % (sub, slot), "{"]
    call = []
    after = []
    closing = []
    for i, a in enumerate(s["args"]):
        m, line, v = a["mode"], a["line"], "p%d" % i
        t = a["type"]
        if m == "scalar":
            L.append("    %s %s = ArgScalar<%s>(\"%s\");" % (t, v, t, line))
            call.append(v)
        elif m == "stream":
            L.append("    RandStream %s = ArgStream(\"%s\");" % (v, line))
            call.append(v)
        elif m == "text":
            L.append("    char %s_text[256];\n    ArgText(\"%s\", %s_text, sizeof %s_text);" % (v, line, v, v))
            call.append("%s_text" % v)
        elif m == "handle":
            L.append("    %s* %s = (%s*)ArgHandle(\"%s\");" % (handle_type(a["kind"]), v, handle_type(a["kind"]), line))
            call.append(v)
        elif m == "object":
            L.append("    %s* %s = (%s*)ArgObject(\"%s\", ROK_%s);" % (arg_type(a)[:-1], v, arg_type(a)[:-1], line, upper(a["kind"])))
            call.append(v)
        elif m == "struct":
            ctype = arg_type(a)[:-1]
            if a.get("dir") == "inout":
                g = "g_inout_%s_%s_%d" % (sub, slot, i)
                head += ["%s %s;" % (ctype, g), "bool %s_null;" % g]
                L.append("    memset(&%s, 0, sizeof %s);" % (g, g))
                L.append('    %s* %s = (%s*)ParseInOut("%s", ROK_%s, &%s);' % (ctype, v, ctype, line, upper(a["kind"]), g))
                L.append("    %s_null = %s == nullptr;" % (g, v))
                closing.append(line + " ")
                after.append('    CompareInOut("%s", ROK_%s, %s_null ? nullptr : &%s);' % (line, upper(a["kind"]), g, g))
                call.append(v)
            elif a.get("dir", "in") == "in" and a.get("nullable"):
                L.append("    %s %s_value;\n    memset(&%s_value, 0, sizeof %s_value);" % (ctype, v, v, v))
                L.append('    %s* %s = (%s*)ParseStructOpt("%s", ROK_%s, &%s_value);' % (ctype, v, ctype, line, upper(a["kind"]), v))
                call.append(v)
            elif a.get("dir", "in") == "in":
                L.append("    %s %s;\n    memset(&%s, 0, sizeof %s);" % (ctype, v, v, v))
                L.append('    ParseStruct("%s", ROK_%s, &%s);' % (line, upper(a["kind"]), v))
                call.append("&" + v)
            else:
                g = "g_out_%s_%s_%d" % (sub, slot, i)
                head.append("%s %s;" % (ctype, g))
                L.append("    memset(&%s, 0, sizeof %s);" % (g, g))
                closing.append(line + ".")
                guard = "if (%s != 0) " % ("g_result_%s_%s" % (sub, slot)) if a.get("when") else ""
                after.append('    %sCompareStruct("%s", ROK_%s, &%s);' % (guard, line, upper(a["kind"]), g))
                call.append("&" + g)
        elif m == "structs":
            counted = s["args"][a.get("countIndex", 0)]
            L.append("    StructsHolder %s;\n    %s.Parse(\"%s\", ROK_%s, ArgScalar<int>(\"%s\"));" % (v, v, line, upper(a["kind"]), counted["line"]))
            call.append("(%s*)%s.data" % (arg_type(a)[:-1], v))
        elif is_scalar_inout(a):
            g = "g_inout_%s_%s_%d" % (sub, slot, i)
            head += ["%s %s;" % (t[:-1], g), "bool %s_null;" % g]
            L.append("    %s = 0;" % g)
            L.append('    %s %s = ParseInOutScalar("%s", &%s);' % (t, v, line, g))
            L.append("    %s_null = %s == nullptr;" % (g, v))
            closing.append(line + " ")
            after.append('    CompareInOutScalar("%s", %s_null ? nullptr : &%s);' % (line, g, g))
            call.append(v)
        elif m == "value":
            L.append("    %s %s = %s();\n    ParseValueArg(\"%s\", ROK_%s, &%s);" % (t, v, t, line, upper(a["kind"]), v))
            call.append(v)
        elif m == "argv":
            L.append("    ArgvHolder %s;\n    ParseArgv(%s);" % (v, v))
            call.append("&%s.args" % v)
        elif m == "callback":
            L.append('    %s %s = (%s)ArgCallback("%s", RCB_%s);' % (t, v, t, line, upper(a["callback"])))
            call.append(v)
        else:
            L.append("    ArgPending();")
            call.append("0")
    res = s["result"]
    if res is not None and res["mode"] in ("scalar", "object", "value"):
        closing.append("result")
    L.append("    SetClosing(%s);" % (", ".join('"%s"' % c for c in closing) if closing else "nullptr"))
    expr = "%s(%s)" % (target or "g_seams.%s->%s" % (sub, slot), ", ".join(call))
    if res is None:
        L.append("    " + expr + ";")
    else:
        g = "g_result_%s_%s" % (sub, slot)
        head.append("%s %s;" % (s["ret"], g))
        L.append("    %s = %s;" % (g, expr))
        if res["mode"] == "object":
            after.append("    CompareObjectResult(ROK_%s, %s);" % (upper(res["kind"]), g))
        elif res["mode"] == "scalar":
            after.append("    CompareResult(%s);" % g)
        elif res["mode"] == "value":
            after.append("    CompareValueResult(ROK_%s, &%s);" % (upper(res["kind"]), g))
    L += ["}", ""]
    out = head + [""] if head else []
    out += L
    if after:
        out += ["void After_%s_%s()" % (sub, slot), "{"] + after + ["}", ""]
    return out


def handle_type(kind: str) -> str:
    return {"bank": "HapiBank", "file": "TdfFile", "rec": "TdfRecord"}[kind]


def json_slot(s: dict, model: dict) -> dict:
    names = ["%s.%s" % (f["object"], f["name"]) for f in model["object_fields"]]
    out = {k: v for k, v in s.items() if k not in ("rawParams", "params")}
    out["objectReads"] = [names[i] for i in s["objectReads"]]
    out["objectWrites"] = [names[i] for i in s["objectWrites"]]
    return out


def json_text(ann: dict, model: dict) -> str:
    def compact(x):
        return json.dumps(x, separators=(", ", ": "))
    out = ["{", ' "replay_format": %d,' % FORMAT, ' "annotations": ' + json.dumps(ann, indent=1).replace("\n", "\n ") + ",",
           ' "fields": [']
    out.append(",\n".join("  " + compact({k: v for k, v in f.items() if k != "offset"}) for f in model["fields"]))
    out += [" ],", ' "objects": [']
    parts = []
    for o in model["objects"]:
        fs = [f for f in model["object_fields"] if f["object"] == o["name"]]
        parts.append("  " + compact(dict(o, fields=[{k: v for k, v in f.items() if k != "object"} for f in fs])))
    out.append(",\n".join(parts))
    out += [" ],", ' "subsystems": {']
    subs = []
    for sub, spec in model["subsystems"].items():
        head = {k: v for k, v in spec.items() if k != "slots"}
        if "unrecorded" in head:
            head["unrecorded"] = {n: json_slot(u, model) if "args" in u else u for n, u in head["unrecorded"].items()}
        lines = ["  %s: {" % json.dumps(sub), '   "header": ' + compact(head) + ",", '   "slots": {']
        lines.append(",\n".join("    %s: %s" % (json.dumps(n), compact(json_slot(s, model))) for n, s in spec["slots"].items()))
        lines += ["   }", "  }"]
        subs.append("\n".join(lines))
    out.append(",\n".join(subs))
    out += [" }", "}", ""]
    return "\n".join(out)


SERVICES_H = ROOT / "src/harness/replay_services.h"
SERVICES_INC = ROOT / "src/harness/replay_services.inc"
SERVICES_SLOTS = ROOT / "tests/replay_services.h"
HOOK_KIND = {"stream": "Int", "int": "Int", "double": "Dbl"}


def service_declaration(spec: dict) -> str:
    """The declaration of the spec, checked against its header (interfaces/services.h unless `header` says another)."""
    text = " ".join((ROOT / spec.get("header", "interfaces/services.h")).read_text().split())
    declaration = " ".join(spec["declaration"].split()).rstrip(";")
    if declaration + ";" not in text and declaration + " {" not in text:
        raise Error("the service %s is not declared in %s as `%s`" % (spec["name"], spec.get("header", "interfaces/services.h"), spec["declaration"]))
    return declaration


def check_service_hooks(services: list[dict]) -> list[str]:
    """Errors for a `ReplayImport call(RI_X);` in the game sources whose arguments differ from the spec."""
    errors = []
    found = {}
    for path in sorted(list((ROOT / "src").rglob("*.cpp")) + list((ROOT / "src").rglob("*.h"))):
        if "harness" in path.parts:
            continue
        lines = path.read_text(errors="replace").split("\n")
        for i, line in enumerate(lines):
            m = re.match(r"\s*ReplayImport call\((RI_\w+)\);", line)
            if not m:
                continue
            kinds = []
            for follow in lines[i + 1:]:
                k = re.match(r"\s*call\.(Int|Ptr|Dbl)\(", follow)
                if not k:
                    break
                kinds.append(k.group(1))
            found[m.group(1)] = (path, kinds)
    for spec in services:
        want = [HOOK_KIND.get(a["mode"], "Ptr") for a in spec["args"]]
        got = found.get(spec["id"])
        if got is None:
            errors.append("no game source hooks %s with `ReplayImport call(%s);`" % (spec["name"], spec["id"]))
        elif got[1] != want:
            errors.append("%s gives %s to ReplayImport for %s, and the spec has %s" % (got[0].relative_to(ROOT), got[1], spec["id"], want))
    for rid in sorted(set(found) - {s["id"] for s in services}):
        errors.append("%s is hooked in %s and has no service spec" % (rid, found[rid][0].relative_to(ROOT)))
    return errors


def check_service_snapshots(services: list[dict]) -> list[str]:
    """Errors for a service whose result is a snapshot: `bytes` names its int argument and `offset` its out argument."""
    errors = []
    for spec in services:
        if spec.get("result") != "snapshot":
            continue
        params = {a.get("param"): a for a in spec["args"]}
        if params.get(spec.get("bytes"), {}).get("mode") != "int":
            errors.append("%s: a snapshot result needs `bytes` that names an int argument" % spec["name"])
        if spec.get("offset") not in params or params[spec["offset"]]["mode"] != "skip":
            errors.append("%s: a snapshot result needs `offset` that names a skipped argument" % spec["name"])
        if not re.match(r"^void\*", spec.get("declaration", "").replace(" ", "")):
            errors.append("%s: a snapshot result is a pointer to the buffer, and the declaration returns void*" % spec["name"])
    return errors


def render_service_ids(services: list[dict]) -> str:
    L = [GENERATED, "#ifndef HARNESS_REPLAY_SERVICES_H", "#define HARNESS_REPLAY_SERVICES_H", "", "enum ReplayImportId {"]
    L += ["    %s," % s["id"] for s in services]
    L += ["    RI_COUNT", "};", "", "static const char* const kReplayImportNames[RI_COUNT] = {"]
    L += ['    "services.%s",' % s["name"] for s in services]
    L += ["};", "", "#endif", ""]
    return "\n".join(L)


SERVICE_ARG_MODES = {"stream", "int", "double", "text", "handle", "skip", "bytes", "object", "ints", "allocated", "textn", "commandargs"}
SERVICE_RESULTS = {"int", "double", "text", "snapshot", "handle", "object", "alloc", "arg"}
HANDLE_KINDS = ("file", "rec", "bank")


def service_function(spec: dict) -> str:
    """The name of the function a spec stands for: `Owner::member` for a member, else the declared name."""
    name = " ".join(spec["declaration"].split()).rstrip(";").split("(")[0].split()[-1].lstrip("*&")
    return "%s::%s" % (spec["owner"], name) if spec.get("owner") else name


def function_files() -> dict[str, str]:
    out: dict[str, str] = {}
    with SUBSYSTEMS.open(newline="") as f:
        for r in csv.DictReader(f):
            out.setdefault(r["function"], r["file"])
    return out


def replay_link_sources() -> set[str]:
    """The source files the replayer links: those of the REPLAY_SUBSYSTEMS and the ones its target lists."""
    text = (ROOT / "CMakeLists.txt").read_text()
    subs = set(replay_subsystems.load())
    listed = re.search(r"add_library\(replay_native_test_objects OBJECT ([^)]*)\)", text).group(1)
    files = set(re.findall(r"\b(src/[\w/]+\.cpp)\b", listed))
    with SUBSYSTEMS.open(newline="") as f:
        for r in csv.DictReader(f):
            if r["subsystem"] in subs:
                files.add(r["file"])
    return files


def linked_services(services: list[dict]) -> set[str]:
    """The names of the services whose definition is in a source the replayer links."""
    sources = replay_link_sources()
    files_of = function_files()
    return {s["name"] for s in services if files_of.get(service_function(s)) in sources or s.get("linked")}


def check_service_specs(services: list[dict], ann: dict) -> list[str]:
    """Errors for the keys of the service specs: modes, the arguments they name, the kinds of objects and handles."""
    errors = []
    kinds = ann["objects"]
    if len({s["name"] for s in services}) != len(services) or len({s["id"] for s in services}) != len(services):
        errors.append("the services have a name or an id twice")
    for s in services:
        where = "service %s" % s["name"]
        params = {a.get("param"): a for a in s["args"]}
        for a in s["args"]:
            m = a["mode"]
            if m not in SERVICE_ARG_MODES:
                errors.append("%s: the argument %s has the unknown mode %s" % (where, a.get("param"), m))
            elif m == "bytes" and params.get(a.get("length"), {}).get("mode") != "int":
                errors.append("%s: the argument %s needs a `length` that names an int argument" % (where, a["param"]))
            elif m == "textn" and params.get(a.get("length"), {}).get("mode") != "int":
                errors.append("%s: the argument %s needs a `length` that names an int argument" % (where, a["param"]))
            elif m == "ints" and params.get(a.get("count"), {}).get("mode") != "int":
                errors.append("%s: the argument %s needs a `count` that names an int argument" % (where, a["param"]))
            elif m == "object" and (a.get("kind") not in kinds or kinds[a["kind"]]["scope"] == "value"):
                errors.append("%s: the argument %s needs a `kind` that is an object kind of the schema" % (where, a["param"]))
            elif m == "handle" and a.get("kind") not in HANDLE_KINDS:
                errors.append("%s: the argument %s needs a `kind` of file, rec or bank" % (where, a["param"]))
            if "reach" in a:
                if m != "object":
                    errors.append("%s: the argument %s has a `reach` and is not an object" % (where, a.get("param")))
                for f in a["reach"]:
                    if not any(x["name"] == f for x in kinds.get(a.get("kind"), {"fields": []})["fields"]):
                        errors.append("%s: the reach of %s names %s, which is no field of %s" % (where, a.get("param"), f, a.get("kind")))
        res = s.get("result")
        if res is not None and res not in SERVICE_RESULTS:
            errors.append("%s: the result %s is none of %s" % (where, res, ", ".join(sorted(SERVICE_RESULTS))))
        if res == "handle" and s.get("kind") not in HANDLE_KINDS:
            errors.append("%s: a handle result needs a `kind` of file, rec or bank" % where)
        if res == "object":
            if s.get("kind") not in kinds or kinds[s["kind"]]["scope"] == "value" or not s.get("ctype"):
                errors.append("%s: an object result needs a `kind` of the schema and the `ctype` that the function returns" % where)
            for f in s.get("reach", []):
                if not any(x["name"] == f for x in kinds.get(s.get("kind"), {"fields": []})["fields"]):
                    errors.append("%s: the reach names %s, which is no field of %s" % (where, f, s.get("kind")))
        if res == "alloc":
            if params.get(s.get("size"), {}).get("mode") != "int":
                errors.append("%s: an alloc result needs `size` that names an int argument" % where)
            if s.get("tag") is not None and params.get(s["tag"], {}).get("mode") != "text":
                errors.append("%s: an alloc result names its `tag` argument, a text one" % where)
        if res == "arg" and s.get("param") not in params:
            errors.append("%s: an arg result names an argument by `param`" % where)
        if s.get("nested") and res not in ("int", None):
            errors.append("%s: a re-entrant service has an int or no result" % where)
    for tag, spec in ann.get("alloc_tags", {}).items():
        target = kinds.get(spec.get("kind"))
        if target is None or target["scope"] != "state" or target.get("table") or "kinds" in target:
            errors.append("the alloc tag %s names a state kind with no table and no kinds" % tag)
        if not isinstance(spec.get("size"), int):
            errors.append("the alloc tag %s needs the `size` of one object" % tag)
    return errors


def service_arg_expr(s: dict, i: int, ctx: str) -> str:
    """The C++ expression of argument i: the parameter in a definition, a field of the call in an answer."""
    a = s["args"][i]
    if ctx == "definition":
        name = a.get("param", "this")
        return "(%s)%s" % (a["cast"], name) if a.get("cast") else name
    v = "c.args[%d].v" % i
    if a["mode"] in ("stream", "int"):
        return "%s.i" % v
    if a["mode"] == "double":
        return "%s.d" % v
    if a["mode"] == "text":
        return "(const char*)%s.p" % v
    return "%s.p" % v


def service_param_expr(s: dict, name: str, ctx: str) -> str:
    for i, a in enumerate(s["args"]):
        if a.get("param") == name:
            return service_arg_expr(s, i, ctx)
    raise Error("the service %s names no argument %s" % (s["name"], name))


def service_input_lines(s: dict, ctx: str) -> list[str]:
    """The lines of C++ that put the arguments into the import line `t` of the replayer."""
    L = []
    for i, a in enumerate(s["args"]):
        m = a["mode"]
        if m == "skip":
            continue
        v = service_arg_expr(s, i, ctx)
        L.append("Space(t);")
        if m == "stream":
            L.append("ReplayValue(t, %s);" % (v if ctx == "definition" else "(RandStream)" + v))
        elif m == "int":
            L.append("t.AddInt((int)%s);" % v)
        elif m == "double":
            L.append("AddDoubleBits(t, %s);" % v)
        elif m == "text":
            L.append("t.AddString(%s);" % v)
        elif m == "handle":
            L.append("AddObject(t, %s, %s);" % (handle_id(a["kind"]), v))
        elif m == "bytes":
            L.append("AddBytes(t, %s, (int)%s);" % (v, service_param_expr(s, a["length"], ctx)))
        elif m == "object":
            L.append("ReplayAddObjectName(t, g_objects, ROK_%s, %s, false);" % (upper(a["kind"]), v))
        elif m == "ints":
            L.append("ReplayAddWords(t, (const unsigned*)%s, (unsigned)%s);" % (v, service_param_expr(s, a["count"], ctx)))
        elif m == "allocated":
            L.append("AddAllocName(t, %s);" % v)
        elif m == "textn":
            L.append("ReplayAddTextN(t, %s, (int)%s);" % (v, service_param_expr(s, a["length"], ctx)))
        elif m == "commandargs":
            L.append("ReplayAddArgs(t, (const CommandArgs*)%s);" % v)
    return L


def render_service_end(services: list[dict]) -> str:
    L = [GENERATED, "", "// included by replay.cpp after the helpers of the import lines", "",
         "void ReplayImportEnd(ReplayImport& c)", "{",
         "    ReplayText& t = s_call;", "    const ReplayArg* a = c.args;",
         '    t.Add("  i ");', "    t.Add(kReplayImportNames[c.id]);", "    switch (c.id) {"]
    for s in services:
        L.append("    case %s:" % s["id"])
        L.append("    {")
        index = {x.get("param"): j for j, x in enumerate(s["args"])}
        for i, a in enumerate(s["args"]):
            m = a["mode"]
            if m == "skip":
                continue
            L.append("        AddSpace(t);")
            if m == "stream":
                L.append("        AddStream(t, a[%d].v.i);" % i)
            elif m == "int":
                L.append("        t.AddInt(a[%d].v.i);" % i)
            elif m == "double":
                L.append("        AddDoubleBits(t, a[%d].v.d);" % i)
            elif m == "text":
                L.append("        t.AddString((const char*)a[%d].v.p);" % i)
            elif m == "handle":
                L.append("        AddHandle(t, %s, a[%d].v.p);" % (handle_id(a["kind"]), i))
            elif m == "bytes":
                L.append("        ReplayBytes(t, a[%d].v.p, a[%d].v.i);" % (i, index[a["length"]]))
            elif m == "object":
                L.append("        ReplayAddObjectName(t, Table(), ROK_%s, a[%d].v.p, true);" % (upper(a["kind"]), i))
            elif m == "ints":
                L.append("        ReplayAddWords(t, (const unsigned*)a[%d].v.p, (unsigned)a[%d].v.i);" % (i, index[a["count"]]))
            elif m == "allocated":
                L.append("        AddAllocName(t, a[%d].v.p);" % i)
            elif m == "textn":
                L.append("        ReplayAddTextN(t, (const char*)a[%d].v.p, a[%d].v.i);" % (i, index[a["length"]]))
            elif m == "commandargs":
                L.append("        ReplayAddArgs(t, (const CommandArgs*)a[%d].v.p);" % i)
        res = s.get("result")
        pointer = "(c.result.kind == 1 ? c.result.v.p : nullptr)"
        outs_after = [(i, a) for i, a in enumerate(s["args"]) if a.get("reach")]
        if res == "snapshot":
            L.append("        AddSnapshot(t, %s, (unsigned)a[%d].v.i);" % (pointer, index[s["bytes"]]))
        elif res == "int":
            L.append("        AddResult(t, c.result);")
        elif res == "double":
            L.append('        t.Add(" = ");')
            L.append("        AddDoubleBits(t, c.result.v.d);")
        elif res == "text":
            L.append('        t.Add(" = ");')
            L.append("        t.AddString((const char*)c.result.v.p);")
        elif res == "handle":
            L.append('        t.Add(" = ");')
            L.append("        AddHandle(t, %s, %s);" % (handle_id(s["kind"]), pointer))
        elif res == "object":
            reach = s.get("reach", [])
            if reach:
                L.append("        ImportResultObjectReach(t, ROK_%s, %s, %d, %s);" % (upper(s["kind"]), pointer, len(reach), ", ".join('"%s"' % f for f in reach)))
            else:
                L.append("        ImportResultObject(t, ROK_%s, %s);" % (upper(s["kind"]), pointer))
        elif res == "alloc":
            tag = "(const char*)a[%d].v.p" % index[s["tag"]] if s.get("tag") else "nullptr"
            L.append("        AddAlloc(t, %s, %s);" % (pointer, tag))
        for i, a in outs_after:
            L.append("        ImportObjectOuts(t, ROK_%s, a[%d].v.p, %d, %s);" % (upper(a["kind"]), i, len(a["reach"]), ", ".join('"%s"' % f for f in a["reach"])))
        if s.get("nested"):
            L.append("        t.AddText(s_nested);")
            L.append("        s_nested.Clear();")
        handle_at = [j for j, x in enumerate(s["args"]) if x["mode"] == "handle"]
        for o in s.get("outs", []):
            if "member" in o:
                L.append("        {")
                L.append("            const %s* self = (const %s*)a[%d].v.p;" % (s["owner"], s["owner"], handle_at[0]))
                L.append('            t.Add("\\n  o ");')
                L.append("            AddHandle(t, %s, self);" % handle_id(s["args"][handle_at[0]]["kind"]))
                L.append('            t.Add("%s ");' % o["label"])
                L.append("            AddHandle(t, %s, self->%s);" % (handle_id(o["handle"]), o["member"]))
                L.append("        }")
            elif "int" in o:
                L.append('        t.Add("\\n  o %s ");' % o["label"])
                L.append("        t.AddInt(*(const int*)a[%d].v.p);" % index[o["int"]])
            else:
                L.append('        t.Add("\\n  o %s ");' % o["label"])
                L.append("        t.AddString((const char*)a[%d].v.p);" % index[o["text"]])
        L.append("        break;")
        L.append("    }")
    L += ["    }", "    t.AddChar('\\n');", "    ReplayCalls() = 1;", "}", ""]
    return "\n".join(L)


def service_return_type(s: dict) -> str:
    head = " ".join(s["declaration"].split()).rstrip(";").split("(")[0]
    words = head.split()
    words = [w for w in words if w not in ("__stdcall", "__cdecl", "__fastcall")]
    return " ".join(words[:-1]) or "void"


def service_result_lines(s: dict, ctx: str) -> list[str]:
    """The lines after Enter: the outs and effects of a definition, then the result."""
    L = []
    res = s.get("result")
    for i, a in enumerate(s["args"]):
        for f in a.get("reach", []):
            L.append('ApplyReach((void*)%s, ROK_%s, ".%s");' % (service_arg_expr(s, i, ctx), upper(a["kind"]), f))
    if ctx == "definition":
        for o in s.get("outs", []):
            if "member" in o:
                L.append('const char* %sText = OutValue("%s");' % (o["member"], o["label"]))
                if o.get("optional"):
                    L.append("if (%sText != nullptr)\n    %s = ParseRecord(%sText);" % (o["member"], o["member"], o["member"]))
                else:
                    L.append("%s = %sText == nullptr ? nullptr : ParseRecord(%sText);" % (o["member"], o["member"], o["member"]))
            elif "int" in o:
                L.append('const char* %sText = OutValue("%s");' % (o["int"], o["label"]))
                L.append("if (%sText != nullptr && %s != nullptr)\n    *%s = atoi(%sText);" % (o["int"], o["int"], o["int"], o["int"]))
            else:
                L.append('const char* %sText = OutValue("%s");' % (o["text"], o["label"]))
                L.append("if (%sText != nullptr)\n    ParseQuoted(%sText, %s, (unsigned)%s);" % (o["text"], o["text"], o["text"], o["size"]))
        L += list(s.get("effect", []))
    if s.get("nested"):
        L.append('PlayNestedCalls("%s");' % ("services." + s["name"]))
    cast = s.get("cast")

    def finish(expr: str, kind: str) -> list[str]:
        if ctx == "definition":
            return ["return %s;" % expr]
        return ["c.Return(%s);" % expr, "return true;"]

    if res == "snapshot" and ctx == "answer":
        size = "(unsigned)" + service_param_expr(s, s["bytes"], ctx)
        L.append("void* buffer = %s(%s, (unsigned*)%s);" % (service_function(s), size, service_param_expr(s, s["offset"], ctx)))
        L.append("SnapshotRegister(result, buffer, %s);" % size)
        L += finish("buffer", "ptr")
    elif res == "snapshot":
        L += finish("SnapshotAnswer(result, %s, %s)" % ("(unsigned)" + service_param_expr(s, s["bytes"], ctx), service_param_expr(s, s["offset"], ctx)), "ptr")
    elif res == "int":
        L += finish("(%s)atoi(result)" % cast if cast else "atoi(result)", "int")
    elif res == "double":
        L += finish("ParseF64(result)", "dbl")
    elif res == "text":
        size = s.get("into_size", 256)
        if ctx == "definition" and "into" in s:
            L.append("ParseQuoted(result, %s, %d);" % (s["into"], size))
            buf = s["into"]
        else:
            L.append("static char text[%d];" % size)
            L.append("ParseQuoted(result, text, %d);" % size)
            buf = "text"
        if "into" not in s:
            L.append("if (strcmp(result, \"null\") == 0)")
            L += ["    " + x for x in finish("(%s)nullptr" % cast if cast else "nullptr", "ptr")]
        L += finish("(%s)%s" % (cast, buf) if cast else buf, "ptr")
    elif res == "handle":
        L += finish("(%s)ParseHandle(%s, result)" % (service_return_type(s), handle_id(s["kind"])), "ptr")
    elif res == "object":
        L.append("%s o = (%s)ReplayResolveObject(result, nullptr, g_objects, AllocObject);" % (s["ctype"], s["ctype"]))
        for f in s.get("reach", []):
            L.append('ApplyReach(o, ROK_%s, ".%s");' % (upper(s["kind"]), f))
        L += finish("o", "ptr")
    elif res == "alloc":
        L += finish("AllocAnswer(result, (unsigned)%s)" % service_param_expr(s, s["size"], ctx), "ptr")
    elif res == "arg":
        L.append("(void)result;")
        L += finish(service_param_expr(s, s["param"], ctx), "ptr")
    elif ctx == "definition":
        L.append("(void)r;")
    else:
        L.append("(void)r;")
        L.append("return %s;" % ("false" if s.get("proceed") else "true"))
    return L


def indent(lines: list[str], n: int) -> list[str]:
    pad = " " * n
    return [pad + part for line in lines for part in line.split("\n")]


def render_service_answers(services: list[dict], linked: set[str] | None = None) -> str:
    linked = linked or set()
    L = [GENERATED, "", "// included by replay_native_test.cpp after its helpers", ""]
    for s in services:
        if s["name"] in linked:
            continue
        declaration = service_declaration(s).rstrip(";")
        owner = s.get("owner")
        if owner:
            short = declaration.split("(")[0].split()[-1]
            declaration = declaration.replace(short + "(", "%s::%s(" % (owner, short), 1)
        L += [declaration, "{"]
        L += indent(s.get("pre", []), 4)
        L += ["    ReplayText t;", "    StartImport(t, %s);" % s["id"]]
        L += indent(service_input_lines(s, "definition"), 4)
        L.append("    const char* %s = %s(t);" % ("result" if s.get("result") else "r", "EnterNested" if s.get("nested") else "Enter"))
        L += indent(service_result_lines(s, "definition"), 4)
        L += ["}", ""]
    L += ["bool ReplayAnswerImport(ReplayImport& c)", "{", "    switch (c.id) {"]
    for s in services:
        if s["name"] not in linked:
            continue
        L += ["    case %s: {" % s["id"], "        ReplayText t;", "        StartImport(t, %s);" % s["id"]]
        L += indent(service_input_lines(s, "answer"), 8)
        L.append("        const char* %s = Enter(t);" % ("result" if s.get("result") else "r"))
        L += indent(service_result_lines(s, "answer"), 8)
        L.append("    }")
    L += ["    default:", "        return false;", "    }", "}", ""]
    return "\n".join(L)



VTABLES_CPP = ROOT / "src/harness/replay_vtables.cpp"


def render_vtables(model: dict, fixtures: bool = False) -> str:
    """The vtable of each concrete kind, by kind id: the externs are here and not in a header."""
    kinds = [o for o in model["objects"] if o.get("vtable") or o.get("vptr")]
    if fixtures:
        L = [GENERATED, '#include "replay_schema_fixtures.h"', '#include "../src/harness/replay_objects.h"', ""]
    else:
        L = [GENERATED, '#include "replay_schema.h"', '#include "replay_objects.h"', ""]
    L += ["extern void (* const %s[])();" % (o.get("vtable") or o["vptr"]) for o in kinds]
    for fn, ret, expr in (("ReplayVtable", "const void*", "%s"), ("ReplayVtableName", "const char*", '"%s"')):
        L += ["", "%s %s(unsigned kind)" % (ret, fn), "{", "    switch (kind) {"]
        for o in kinds:
            L += ["    case ROK_%s:" % upper(o["name"]), "        return %s;" % (expr % (o.get("vtable") or o["vptr"]))]
        L += ["    default:", "        return %s;" % ("nullptr" if ret == "const void*" else '""'), "    }", "}"]
    return "\n".join(L) + "\n"


CALLBACKS_H = ROOT / "src/harness/replay_callbacks.h"
CALLBACKS_SLOTS = ROOT / "tests/replay_callbacks.h"
SHAPES_SLOTS = ROOT / "tests/replay_shapes.h"


def callback_include(header: str, base: str) -> str:
    return os.path.relpath(ROOT / header, ROOT / base).replace(os.sep, "/")


def callback_signature(m: dict) -> str:
    return ", ".join("%s a%d" % (a["type"], i) for i, a in enumerate(m.get("args", [])))


def render_callbacks(model: dict) -> str:
    """The recorder's side of a callback: a class per interface that records the call and forwards it to the object's own method."""
    L = [GENERATED, "#ifndef HARNESS_REPLAY_CALLBACKS_H", "#define HARNESS_REPLAY_CALLBACKS_H", "", '#include "replay_objects.h"', '#include "replay_values.h"']
    for c in model["callbacks"]:
        L.append('#include "%s"' % callback_include(c["header"], "src/harness"))
    L.append("")
    for c in model["callbacks"]:
        name = c["name"]
        L += ["class ReplayThunk%s : public %s {" % (name, name), "public:"]
        for m in c["methods"]:
            L.append("    %s %s(%s) override;" % (m.get("ret", "void"), m["name"], callback_signature(m)))
        L += ["};", ""]
        for index, m in enumerate(c["methods"]):
            ret = m.get("ret", "void")
            args = m.get("args", [])
            L += ["inline %s ReplayThunk%s::%s(%s)" % (ret, name, m["name"], callback_signature(m)), "{",
                  "    ReplayCallbackCall call(this, %d);" % index]
            for i, a in enumerate(args):
                v = "a%d" % i
                L.append("    call.Space();")
                if a["mode"] == "scalar":
                    L.append("    ReplayValue(call.Text(), %s);" % v)
                elif a["mode"] == "text":
                    L.append("    call.Text().AddString((const char*)%s);" % v)
                elif a["mode"] == "object":
                    L.append("    ReplayAddObjectName(call.Text(), call.Table(), ROK_%s, %s, true);" % (upper(a["kind"]), v))
                else:
                    L.append("    ReplayAddValue(call.Text(), ROK_%s, &%s, call.Table());" % (upper(a["kind"]), v))
            types = ", ".join(["void*"] + [a["type"] for a in args])
            L.append("    typedef %s (__thiscall* Method)(%s);" % (ret, types))
            call = "((Method)call.Original(%d))(%s)" % (index, ", ".join(["this"] + ["a%d" % i for i in range(len(args))]))
            if ret == "void":
                L.append("    %s;" % call)
            else:
                L.append("    %s result = %s;" % (ret, call))
                L.append('    call.Text().Add(" = ");')
                if ret.endswith("*"):
                    L.append("    ReplayAddObjectName(call.Text(), call.Table(), ROK_%s, result, true);" % upper([k for k, o in model["kind_types"].items() if o == ret[:-1]][0]))
                else:
                    L.append("    ReplayValue(call.Text(), result);")
                L.append("    return result;")
            L += ["}", ""]
    L += ["inline const void* ReplayThunkVtable(unsigned callback)", "{", "    switch (callback) {"]
    for c in model["callbacks"]:
        L += ["    case RCB_%s: {" % upper(c["name"]), "        ReplayThunk%s probe;" % c["name"], "        return ReplayVptrOf(&probe);", "    }"]
    L += ["    }", "    return nullptr;", "}", "", "#endif", ""]
    return "\n".join(L)


def render_callback_players(model: dict) -> str:
    """The replayer's side of a callback: it makes each recorded call on the module's own object and compares the result, and for a callback that the module holds, a class that records the calls the module makes on it."""
    L = [GENERATED, "", "// included by replay_native_test.cpp after its helpers", ""]
    for c in model["callbacks"]:
        name = c["name"]
        L += ["void PlayCallbacks_%s(void* object, unsigned handle, const char* import)" % name, "{",
              "    for (int k = 0; k < g_cbCount; k++) {", "        const char* args = nullptr;",
              '        const char* method = CallbackMethod(k, handle, object, "%s", import, &args);' % c["handle"],
              "        if (method == nullptr)", "            continue;"]
        for m in c["methods"]:
            ret = m.get("ret", "void")
            L.append('        if (CallbackIs(method, "%s")) {' % m["name"])
            for i, a in enumerate(m.get("args", [])):
                v = "a%d" % i
                if a["mode"] == "scalar":
                    L.append("            %s %s = CallbackScalar<%s>(&args);" % (a["type"], v, a["type"]))
                elif a["mode"] == "text":
                    L.append("            char %s_text[256];\n            CallbackText(&args, %s_text, sizeof %s_text);\n            char* %s = %s_text;" % (v, v, v, v, v))
                elif a["mode"] == "object":
                    L.append("            %s %s = (%s)CallbackObject(&args, ROK_%s);" % (a["type"], v, a["type"], upper(a["kind"])))
                else:
                    L.append("            %s %s = CallbackValue<%s>(&args, ROK_%s);" % (a["type"], v, a["type"], upper(a["kind"])))
            call = "((%s*)object)->%s(%s)" % (name, m["name"], ", ".join("a%d" % i for i in range(len(m.get("args", [])))))
            if ret == "void":
                L += ["            %s;" % call, "            CallbackDone(import, k, args, nullptr);"]
            else:
                L += ["            %s result = %s;" % (ret, call), "            ReplayText have;"]
                if ret.endswith("*"):
                    L.append("            ReplayAddObjectName(have, g_objects, ROK_%s, result, false);" % upper([k for k, o in model["kind_types"].items() if o == ret[:-1]][0]))
                else:
                    L.append("            ReplayValue(have, result);")
                L += ["            CallbackDone(import, k, args, have.Data());", "            have.Free();"]
            L.append("            continue;")
            L.append("        }")
        L += ["        CallbackUnknown(import, k, method);", "    }", "}", ""]
    for c in model["callbacks"]:
        name = c["name"]
        L += ["class ReplayHeld%s : public %s {" % (name, name), "public:"]
        for m in c["methods"]:
            L.append("    %s %s(%s) override;" % (m.get("ret", "void"), m["name"], callback_signature(m)))
        L += ["};", ""]
        for m in c["methods"]:
            ret = m.get("ret", "void")
            L += ["inline %s ReplayHeld%s::%s(%s)" % (ret, name, m["name"], callback_signature(m)), "{",
                  '    HeldCall call(RCB_%s, (const %s*)this, "%s");' % (upper(name), name, m["name"])]
            for i, a in enumerate(m.get("args", [])):
                v = "a%d" % i
                L.append("    call.Space();")
                if a["mode"] == "scalar":
                    L.append("    ReplayValue(call.Text(), %s);" % v)
                elif a["mode"] == "text":
                    L.append("    call.Text().AddString((const char*)%s);" % v)
                elif a["mode"] == "object":
                    L.append("    ReplayAddObjectName(call.Text(), g_objects, ROK_%s, %s, false);" % (upper(a["kind"]), v))
                else:
                    L.append("    ReplayAddValue(call.Text(), ROK_%s, &%s, g_objects);" % (upper(a["kind"]), v))
            if ret == "void":
                L.append("    call.Finish();")
            elif ret.endswith("*"):
                L.append("    return (%s)ReplayResolveObject(call.Finish(), nullptr, g_objects, AllocObject);" % ret)
            else:
                L.append("    return ParseResult<%s>(call.Finish());" % ret)
            L += ["}", ""]
    L += ["inline void* NewHeldCallback(unsigned callback, void* memory)", "{", "    switch (callback) {"]
    for c in model["callbacks"]:
        L += ["    case RCB_%s:" % upper(c["name"]), "        return (%s*)new (memory) ReplayHeld%s;" % (c["name"], c["name"])]
    L += ["    }", "    return nullptr;", "}", "", "inline unsigned HeldCallbackSize(unsigned callback)", "{", "    switch (callback) {"]
    for c in model["callbacks"]:
        L += ["    case RCB_%s:" % upper(c["name"]), "        return sizeof(ReplayHeld%s);" % c["name"]]
    L += ["    }", "    return 0;", "}", ""]
    return "\n".join(L)


def shape_slots(ann: dict) -> tuple[list[dict], list[str]]:
    """The fixture slots of `fixtures` in tests/replay_fixtures.json: each classified as a slot of a subsystem is, for the stubs the replayer's self-tests call."""
    out, errors = [], []
    for name, spec in ann.get("fixtures", {}).items():
        params = spec["params"]
        notes = spec.get("args", {})
        errors += ["fixtures.%s: the note for %s names no parameter" % (name, n) for n in notes if n not in {p["name"] for p in params}]
        args = [classify_arg(p, notes.get(p["name"]), ann) for p in params]
        result = classify_result(spec["ret"], spec.get("result"), ann)
        errors += check_forms("fixtures.%s" % name, args, result, ann)
        errors += ["fixtures.%s: the argument %s has no mode" % (name, a["name"]) for a in args if a["mode"] == "pending"]
        out.append({"name": name, "conv": spec.get("conv", "__stdcall"), "ret": spec["ret"], "params": [p["type"] for p in params], "rawParams": [p["type"] for p in params], "args": args, "result": result})
    return out, errors


RUNNER_ARG_MODES = {"scalar", "stream", "text", "handle", "object", "struct", "structs", "value", "argv", "callback"}


def runnable(s: dict) -> bool:
    """The replayer's runner can pass every argument of the slot and compare its result."""
    return all(a["mode"] in RUNNER_ARG_MODES for a in s["args"]) and (s["result"] is None or s["result"]["mode"] in ("scalar", "object", "value"))


def native_name(slot: str) -> str:
    return "Native_fixture_" + slot


def render_shapes(ann: dict) -> str:
    L = [GENERATED, "", "// included by replay_native_test.cpp after its helpers, which define Native_fixture_<Slot> for each runner", ""]
    slots, _ = shape_slots(ann)
    for s in slots:
        L += import_stub("fixture", s["name"], s)
    for s in slots:
        if runnable(s):
            L += runner("fixture", s["name"], s, native_name(s["name"]))
    return "\n".join(L)


def render_fixture_hooks(ann: dict, model: dict) -> str:
    """The recording wrappers of the fixture slots, which tests/replay_fixture_recorder.cpp compiles inside replay.cpp's anonymous namespace."""
    L = [GENERATED, "", "// included by tests/replay_fixture_recorder.cpp inside a second anonymous namespace after replay.cpp", ""]
    slots, _ = shape_slots(ann)
    objects = {o["name"]: o for o in model["objects"]}
    L.append("enum {")
    L.append("    SUB_FIXTURE = SUB_COUNT,")
    for n, s in enumerate(slots):
        L.append("    RSL_FIXTURE_%s = RSL_COUNT%s%s" % (upper(s["name"]), " + %d" % n if n else "", "," if n + 1 < len(slots) else ""))
    L += ["};", "", "struct FixtureSeam {"]
    for s in slots:
        L.append("    %s (%s* %s)(%s);" % (s["ret"], s["conv"], s["name"], ", ".join(s["params"])))
    L += ["};", "", "const FixtureSeam* s_inner_fixture;", ""]
    for s in slots:
        L += wrapper("fixture", {}, s["name"], s, objects, not any(a["mode"] == "callback" for a in s["args"]))
    L.append("FixtureSeam s_rec_fixture = {")
    L += ["    Rec%s%s," % (camel("fixture"), s["name"]) for s in slots]
    L += ["};", ""]
    return "\n".join(L)


def generate(rows: list[dict[str, str]], ann: dict | None = None, assignment: dict | None = None, fixtures: dict | None = None):
    ann = ann or load_annotations()
    assignment = assignment or read_assignment()
    model, errors = build(ann, rows, assignment)
    fixture_ann = merge_fixtures(ann, fixtures if fixtures is not None else json.loads(FIXTURES_JSON.read_text()))
    fixture_model, fixture_errors = build(fixture_ann, rows, assignment)
    errors += [e for e in fixture_errors if e not in errors]
    attach_raw_params(model)
    services = ann["services"]
    errors += shape_slots(fixture_ann)[1]
    errors += check_service_hooks(services)
    errors += check_service_snapshots(services)
    spec_errors = check_service_specs(services, ann)
    errors += spec_errors
    if spec_errors:
        services = []
    for sub, spec in ann["subsystems"].items():
        errors += ["%s lists the service %s, which has no spec" % (sub, n) for n in spec.get("services", [])
                   if not any(x["name"] == n or x["name"].startswith(n + "_") for x in services)]
    files = {
        SCHEMA_H: render_schema(ann, model),
        FIXTURE_SCHEMA_H: render_fixture_schema(fixture_ann, fixture_model),
        FIXTURE_VTABLES_CPP: render_vtables(fixture_model, True),
        HOOKS_H: render_hooks(ann, model),
        SLOTS_H: render_slots(ann, model),
        SERVICES_H: render_service_ids(services),
        SERVICES_INC: render_service_end(services),
        SERVICES_SLOTS: render_service_answers(services, linked_services(services)),
        VTABLES_CPP: render_vtables(model),
        CALLBACKS_H: render_callbacks(model),
        CALLBACKS_SLOTS: render_callback_players(model),
        SHAPES_SLOTS: render_shapes(fixture_ann),
        FIXTURE_HOOKS_INC: render_fixture_hooks(fixture_ann, fixture_model),
        JSON_OUT: json_text(ann, model),
    }
    return files, errors, model


def attach_raw_params(model: dict) -> None:
    """The parameter types as the header spells them (the classification keeps a normalised copy)."""
    for sub, spec in model["subsystems"].items():
        iface = parse_interface(sub, spec["struct"])
        for s in iface["slots"]:
            slot = {**spec["slots"], **imported_unrecorded(spec)}.get(s["name"])
            if slot is None:
                continue
            slot["rawParams"] = [p["type"] for p in s["params"]]
            slot["params"] = [p["type"] for p in s["params"]]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        files, errors, _ = generate(read_rows())
    except Error as e:
        sys.exit("replayschema: %s" % e)
    for e in errors:
        print("replayschema: " + e, file=sys.stderr)
    if args.check:
        stale = [p for p, want in files.items() if not p.exists() or p.read_text() != want]
        for p in stale:
            print("stale %s" % p.relative_to(ROOT), file=sys.stderr)
        sys.exit(1 if stale or errors else 0)
    if errors:
        sys.exit(1)
    for path, want in files.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists() or path.read_text() != want:
            path.write_text(want, newline="\n")
            print("wrote %s" % path.relative_to(ROOT))
        else:
            print("unchanged %s" % path.relative_to(ROOT))


if __name__ == "__main__":
    main()
