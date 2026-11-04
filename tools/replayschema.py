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
    src/harness/replay_hooks.inc    the recording copy of every seam table, with the argument and import hooks
    tests/replay_slots.h            the slot runners, import stubs and native seam tables of the replayer
    src/harness/replay_services.h   the ids and names of the services imports (GameRand, RandomInt, the TDF and command classes)
    src/harness/replay_services.inc the recorder's lines for them
    tests/replay_services.h         the replayer's answers for them

A slot's reads are field_read, global_read and global_maybe_written rows; its writes are field_write, global_write and
global_maybe_written rows, over the functions behind the slot. A row names a block field (Game member or SimGlobals
global), an object field (`Type::member` of an object kind) or something else. A `Game::` or `g_` target that is
tracked by no table and is not exempt is an error for a complete subsystem; a pending subsystem lists what is left
under `pending`, which its own issue empties. A deref_read or deref_write on a bare scalar pointer and every
field_escape row needs a hand rule in `annotations.rules`; a rule that matches no row is an error.
"""

import argparse
import csv
import json
import re
import sys
from pathlib import Path

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
SIM_GLOBALS_H = ROOT / "src/state/sim_globals.h"
FORMAT = 2

RNG_FIELDS = ["rng.main", "rng.load", "rng.seed"]
RNG_CALLS = {"GameRand", "RandomInt"}
SIZES = {"i32": 4, "u32": 4, "u16": 2, "f32": 4, "u8": 1, "i16": 2, "f64": 8}
BLOCK_KINDS = {"field_read": "r", "field_write": "w", "global_read": "r", "global_write": "w", "global_maybe_written": "rw"}
OBJECT_KINDS = {"field_read": "r", "field_write": "w", "field_maybe_written": "rw"}
FIELD_KINDS = {"i32": "I32", "u32": "U32", "u16": "U16", "i16": "I16", "f32": "F32", "point16": "POINT16", "string": "STRING",
               "weapon": "WEAPON", "ref": "REF", "flags": "FLAGS", "u8": "U8", "f64": "F64", "bytes": "BYTES", "embed": "EMBED",
               "unitid": "UNITID", "array": "ARRAY"}
INTEGER_ARGS = {"int", "unsigned", "unsigned int", "long", "unsigned long", "short", "unsigned short", "char", "unsigned char"}
FACTOR = re.compile(r"^\(?(\w+)(?:>>(\d+))?\)?$")
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


def load_annotations() -> dict:
    return json.loads(JSON_OUT.read_text())["annotations"]


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


def parse_length(expr, where: str, errors: list[str]) -> list[dict]:
    """The two factors of a length expression: an integer, or `member`, `member>>k` or an integer, once or twice, joined by `*`."""
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
            out.append({"member": m.group(1), "shift": int(m.group(2) or 0)})
        else:
            errors.append("%s: the factor `%s` of the length is no integer, member or member>>k" % (where, p))
            out.append({"const": 1})
    while len(out) < 2:
        out.append({"const": 1})
    return out


def render_factor(f: dict, owner: str) -> str:
    if "const" in f:
        return "{ %d, 0, 0 }" % f["const"]
    return "{ offsetof(%s, %s), (unsigned char)sizeof(((%s*)nullptr)->%s), %d }" % (owner, f["member"], owner, f["member"], f["shift"])


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


def check_table(kind: str, spec: dict, errors: list[str]) -> dict:
    where = "%s table" % kind
    base = spec.get("base")
    if base not in ("game", "sim") or not spec.get("member"):
        errors.append("%s: needs a `base` of game or sim and a `member`" % where)
    return {"kind": kind, "sim": base == "sim", "ref": bool(spec.get("ref", False)), "member": spec.get("member", "?"),
            "count": parse_length(spec.get("count", 0), where, errors), "alloc": int(spec.get("alloc", 0)),
            "owner": "SimGlobals" if base == "sim" else "Game"}


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


WALKABLE = ("ref", "embed", "unitid", "weapon")
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
            when = f.get("when")
            if not when or (f["object"], when["field"]) not in object_index:
                errors.append("%s: a unitid field names the field and the value that make it an id" % where)
            else:
                f["condField"] = object_index[(f["object"], when["field"])] + 1
                f["condValue"] = when["equals"]
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
    elif ctype == "char*":
        out["mode"] = "text"
    else:
        out["mode"] = "pending"
    return out


ARG_MODES = {"scalar", "stream", "text", "handle", "object", "struct", "structs", "value", "argv", "bytes-in", "bytes-out", "weapon", "vec3", "units", "pending"}
RESULT_MODES = {"scalar", "object", "text", "weapon", "value", "pending"}


def check_forms(where: str, args: list[dict], result: dict | None, ann: dict) -> list[str]:
    """Errors for the struct forms of a slot: the kind must be a value kind of the argument's type, a count must name an integer argument."""
    errors = []
    kinds = ann["objects"]
    names = [a["name"] for a in args]

    def value_kind(item: dict, label: str, pointer: bool) -> None:
        spec = kinds.get(item.get("kind"))
        if spec is None or spec["scope"] != "value":
            errors.append("%s: %s needs a `kind` that is a value kind of the schema" % (where, label))
        elif item["type"] != spec["type"] + ("*" if pointer else ""):
            errors.append("%s: %s is %s, and the kind %s is %s" % (where, label, item["type"], item["kind"], spec["type"] + ("*" if pointer else "")))

    for a in args:
        label = "the argument %s" % a["name"]
        if a["mode"] not in ARG_MODES:
            errors.append("%s: %s has the unknown mode %s" % (where, label, a["mode"]))
        elif a["mode"] == "struct":
            value_kind(a, label, True)
            if a.get("nullable") and a.get("dir", "in") != "in":
                errors.append("%s: %s is an out struct, and only a struct in can be nullable" % (where, label))
        elif a["mode"] == "structs":
            value_kind(a, label, True)
            if a.get("dir", "in") != "in":
                errors.append("%s: %s is an array of structs, and only an array in is recorded" % (where, label))
            count = a.get("count")
            if count not in names or count == a["name"]:
                errors.append("%s: %s needs a `count` that names another argument" % (where, label))
            elif args[names.index(count)]["type"] not in INTEGER_ARGS:
                errors.append("%s: the count %s of %s is no integer" % (where, count, a["name"]))
        elif a["mode"] == "value":
            value_kind(a, label, False)
    if result is not None:
        if result["mode"] not in RESULT_MODES:
            errors.append("%s: the result has the unknown mode %s" % (where, result["mode"]))
        elif result["mode"] == "value":
            value_kind(result, "the result", False)
    return errors


def build(ann: dict, rows: list[dict[str, str]], assignment: dict[str, list[str]]) -> dict:
    """The generated model: fields, objects, subsystems, and the errors."""
    game, sim = block_tables(ann)
    errors = check_sim_names(sim)
    arrays: list[dict] = []
    fields = field_table(game, sim, arrays, errors)
    field_index = {f["name"]: i for i, f in enumerate(fields)}
    objects = []
    object_fields = []
    tables: list[dict] = []
    kind_names = list(ann["objects"])
    for kind in kind_names:
        spec = ann["objects"][kind]
        objects.append({"name": kind, "type": spec["type"], "scope": spec["scope"], "table": spec.get("table"), "inside": spec.get("inside")})
        if spec.get("table"):
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
            object_fields.append(entry)
    table_targets = set()
    for t in tables:
        top = t["member"].split(".")[0]
        table_targets.add(top if t["sim"] else "Game::" + top)
    object_index = {(f["object"], f["name"]): i for i, f in enumerate(object_fields)}
    reach_table: list[int] = []
    errors += resolve_field_reaches(ann, object_fields, object_index, reach_table)
    by_function: dict[str, list[dict]] = {}
    for r in rows:
        by_function.setdefault(r["function"], []).append(r)
    used_rules = set()
    rules_of: dict[tuple[str, str], list[int]] = {}
    for i, r in enumerate(ann["rules"]):
        rules_of.setdefault((r["kind"], r["target"]), []).append(i)
    state_types = {spec["type"] for spec in ann["objects"].values() if spec["scope"] == "state"}

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
        pending = {"untracked": set(), "rules": set(), "functions": [], "args": []}
        for s in iface["slots"]:
            note = notes.get(s["name"], {})
            functions = functions_of(s, sub, assignment, note)
            if not functions:
                pending["functions"].append(s["name"])
            reads, writes = set(), set()
            obj_reads, obj_writes = set(), set()
            draws = False
            all_of = None
            for fn in functions:
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
                        if not hits and target.split("::")[0] in state_types and (kind, target) not in rules_of:
                            pending["untracked"].add(target)
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
                            for tag, into in (("reads", obj_reads), ("writes", obj_writes)):
                                for name in rule.get(tag, []):
                                    k, fname = name.split(".", 1)
                                    if (k, fname) not in object_index:
                                        errors.append("the rule for %s %s names the unknown field %s" % (kind, target, name))
                                    else:
                                        into.add(object_index[(k, fname)])
                        elif rule["rule"] == "all-of":
                            if all_of is not None and all_of != rule["object"]:
                                errors.append("%s: the slot %s has rules for two all-of kinds" % (sub, s["name"]))
                            all_of = rule["object"]
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
            slots[s["name"]] = {
                "conv": s["conv"], "ret": s["ret"], "functions": functions,
                "reads": [f for f in order if f in reads], "writes": [f for f in order if f in writes],
                "objectReads": sorted(obj_reads), "objectWrites": sorted(obj_writes), "allOf": all_of,
                "args": args, "result": result, "native": note.get("native", s["name"]),
            }
            slot_list.append((sub, s["name"]))
        done = {k: sorted(v) if isinstance(v, set) else v for k, v in pending.items()}
        subsystems[sub] = {
            "interface": "interfaces/%s.h" % sub, "struct": struct, "version": iface["version"], "status": spec["status"],
            "imports": spec.get("imports", []), "slots": slots,
        }
        if spec.get("issue"):
            subsystems[sub]["issue"] = spec["issue"]
        left = {k: v for k, v in done.items() if v}
        if complete and left:
            for k, v in left.items():
                errors += ["%s is complete but has pending %s: %s" % (sub, k, ", ".join(v[:6]))]
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
            if slot not in subsystems.get(seam, {}).get("slots", {}):
                errors.append("%s imports %s, which is no slot of the %s interface" % (sub, imp, seam))
    return {"fields": fields, "objects": objects, "object_fields": object_fields, "subsystems": subsystems, "slot_list": slot_list, "reach_table": reach_table,
            "arrays": arrays, "tables": tables}, errors


def check_sim_names(sim: dict) -> list[str]:
    text = SIM_GLOBALS_H.read_text()
    known = load_sim_map()
    return ["SimGlobals has no member %s" % n for n in sim if n + ";" not in text and n not in known.fields]


def field_table(game: dict, sim: dict, arrays: list[dict], errors: list[str]) -> list[dict]:
    schema = traceschema.Schema()
    layout = {l["name"]: l for l in schema.layout("Game")[0]}
    fields = []
    for name, kind in game.items():
        leaf = layout.get(name)
        spec = kind if isinstance(kind, dict) else None
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


GENERATED = "// Generated by tools/replayschema.py. Do not edit."


def render_schema(ann: dict, model: dict) -> str:
    fields, objects, ofields, subsystems = model["fields"], model["objects"], model["object_fields"], model["subsystems"]
    sim = ann["sim_fields"]
    L = [GENERATED, "#ifndef REPLAY_SCHEMA_H", "#define REPLAY_SCHEMA_H", "", "#include <stddef.h>", ""]
    L += ['#include "../state/game_rand.h"', "#pragma pack(push, 1)", '#include "../game/game.h"', "#pragma pack(pop)"]
    for inc in ann["includes"]:
        L.append('#include "%s"' % inc)
    L += ['#include "../state/sim_globals.h"', '#include "../state/block.h"', '#include "replay_types.h"', "",
          "#define REPLAY_FORMAT %d" % FORMAT, ""]
    arrays, tables = model["arrays"], model["tables"]
    macros = list(sim)
    for t in tables:
        if t["sim"]:
            macros.append(t["member"].split(".")[0])
    for a in arrays:
        if a["owner"] == "SimGlobals":
            macros += [a["member"]] + [f["member"] for f in a["length"] if "member" in f]
    for t in tables:
        if t["sim"]:
            macros += [f["member"] for f in t["count"] if "member" in f]
    macros = list(dict.fromkeys(macros))
    for name in macros:
        L += ['#pragma push_macro("%s")' % name, "#undef %s" % name]
    L += ["", "static const ReplayField kReplayFields[] = {"]
    for f in fields:
        base = {"game": "RB_GAME", "sim": "RB_SIM", "rng": "RB_RNG"}[f["base"]]
        offset = "offsetof(SimGlobals, %s)" % f["offset"] if f["base"] == "sim" else "%d" % f["offset"]
        extra = ", %d" % f["array"] if f.get("array") else ""
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
        L.append("    { %d, %d, offsetof(%s, %s), %s, %d }," % (int(t["sim"]), int(t["ref"]), t["owner"], t["member"], render_length(t["count"], t["owner"]), t["alloc"]))
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
        constant = all("const" in f for f in t["count"])
        if not t["ref"] and constant:
            n = t["count"][0]["const"] * t["count"][1]["const"]
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == %d * sizeof(%s), "the table of %s holds %d elements");' % (t["owner"], t["member"], n, kind_type[t["kind"]], t["kind"], n))
        elif t["ref"]:
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == sizeof(unsigned), "the table of %s is a Ref");' % (t["owner"], t["member"], t["kind"]))
    L.append("")
    for name in macros:
        L.append('#pragma pop_macro("%s")' % name)
    L += ["", "enum ReplayFieldIndex {"]
    L += ["    RF_%s," % upper(f["name"]) for f in fields]
    L += ["};", "", "enum ReplayObjectKindId {"]
    L += ["    ROK_%s," % upper(o["name"]) for o in objects]
    L += ["    ROK_COUNT", "};", "", "static const ReplayObjectKind kReplayObjectKinds[] = {"]
    scope = {"state": "RS_STATE", "index": "RS_INDEX", "value": "RS_VALUE"}
    kind_id = {o["name"]: "ROK_" + upper(o["name"]) for o in objects}
    types = {o["name"]: o["type"] for o in objects}
    table_id = {t["kind"]: str(i + 1) for i, t in enumerate(tables)}
    for o in objects:
        inside = o.get("inside")
        if inside:
            container = types[inside["kind"]]
            tail = '1, %s, offsetof(%s, %s), "%s"' % (kind_id[inside["kind"]], container, inside["member"], inside["member"])
        else:
            tail = '0, 0, 0, ""'
        L.append('    { "%s", sizeof(%s), %s, %s, %s },' % (o["name"], o["type"], scope[o["scope"]], table_id.get(o["name"], "RT_ORDER"), tail))
    L += ["};", "static const unsigned kReplayObjectKindCount = %d;" % len(objects), "",
          "static const ReplayObjectField kReplayObjectFields[] = {"]
    position = {(f["object"], f["name"]): i for i, f in enumerate(ofields)}
    range_end, range_begin = {}, {}
    for f in ofields:
        if f.get("range_end_of"):
            begin = position[(f["object"], f["range_end_of"])]
            range_end[begin] = position[(f["object"], f["name"])] + 1
            range_begin[position[(f["object"], f["name"])]] = begin + 1
    for i, f in enumerate(ofields):
        member = f.get("member", f["name"])
        t = types[f["object"]]
        size = f.get("size")
        size_text = str(size) if size else "sizeof(((%s*)nullptr)->%s)" % (t, member)
        target = kind_id[f["target"]] if f["kind"] in ("ref", "embed", "unitid") else ("ROK_WEAPONDEF" if f["kind"] == "weapon" else "0")
        extra = ", %d" % f["array"] if f.get("array") else ""
        L.append('    { "%s", offsetof(%s, %s), %s, %s, RK_%s, %s, %d, %d, %d, %d, %d, %d%s },' % (
            f["name"], t, member, size_text, kind_id[f["object"]], FIELD_KINDS[f["kind"]], target, range_end.get(i, 0), range_begin.get(i, 0),
            f.get("condField", 0), f.get("condValue", 0), f.get("reachFirst", 0), f.get("reachCount", 0), extra))
    L += ["};", "static const unsigned kReplayObjectFieldCount = %d;" % len(ofields), ""]
    L += ["static const unsigned short kReplayReachFields[] = { %s };" % ", ".join(str(i) for i in model["reach_table"] or [0]), ""]
    for f in ofields:
        t = types[f["object"]]
        member = f.get("member", f["name"])
        if f["kind"] == "embed":
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == sizeof(%s), "%s::%s has the size of its kind");' % (t, member, types[f["target"]], t, f["name"]))
        elif f["kind"] == "bytes":
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == %d, "%s::%s has the size the schema says");' % (t, member, f["size"], t, f["name"]))
        elif f["kind"] == "array":
            continue
        elif f.get("size") is None:
            want = {"i32": 4, "u32": 4, "u16": 2, "i16": 2, "f32": 4, "flags": 4, "ref": 4, "point16": 4, "weapon": 4, "u8": 1, "f64": 8, "unitid": 2}[f["kind"]]
            L.append('static_assert(sizeof(((%s*)nullptr)->%s) == %d, "%s::%s has the size the schema says");' % (t, member, want, t, f["name"]))
    L += ["", "enum ReplaySubsystemId {"]
    L += ["    SUB_%s," % upper(s) for s in subsystems]
    L += ["    SUB_COUNT", "};", "", "enum ReplaySlotId {"]
    for sub, spec in subsystems.items():
        for slot in spec["slots"]:
            L.append("    RSL_%s_%s," % (upper(sub), upper(slot)))
    L += ["    RSL_COUNT", "};", ""]
    for sub, spec in subsystems.items():
        for slot, s in spec["slots"].items():
            base = "kReplay%s%s" % (re.sub(r"[^A-Za-z0-9]", "", sub.title()), slot)
            for tag, names, kindname, idx in (("Reads", s["reads"], "unsigned char", [fields_index(fields, n) for n in s["reads"]]),
                                              ("Writes", s["writes"], "unsigned char", [fields_index(fields, n) for n in s["writes"]]),
                                              ("ObjReads", s["objectReads"], "unsigned short", s["objectReads"]),
                                              ("ObjWrites", s["objectWrites"], "unsigned short", s["objectWrites"])):
                if names:
                    L.append("static const %s %s%s[] = { %s };" % (kindname, base, tag, ", ".join(str(i) for i in idx)))
    L += ["", "static const ReplaySlotSets kReplaySlots[] = {"]
    for si, (sub, spec) in enumerate(subsystems.items()):
        for slot, s in spec["slots"].items():
            base = "kReplay%s%s" % (re.sub(r"[^A-Za-z0-9]", "", sub.title()), slot)

            def ref(tag, items):
                return ("%s%s" % (base, tag) if items else "nullptr", len(items))
            r, w = ref("Reads", s["reads"]), ref("Writes", s["writes"])
            ro, wo = ref("ObjReads", s["objectReads"]), ref("ObjWrites", s["objectWrites"])
            every = "ROK_%s + 1" % upper(s["allOf"]) if s.get("allOf") else "0"
            L.append('    { "%s", SUB_%s, %s, %d, %s, %d, %s, %d, %s, %d, %s },' % (slot, upper(sub), r[0], r[1], w[0], w[1], ro[0], ro[1], wo[0], wo[1], every))
    L += ["};", "static const unsigned kReplaySlotCount = RSL_COUNT;", "", "static const ReplaySubsystem kReplaySubsystems[] = {"]
    first = 0
    for sub, spec in subsystems.items():
        n = len(spec["slots"])
        L.append('    { "%s", %d, %d, %d, %d, %d },' % (sub, spec["version"][0], spec["version"][1], int(spec["status"] == "complete"), first, n))
        first += n
    L += ["};", "static const unsigned kReplaySubsystemCount = SUB_COUNT;", "", "#endif", ""]
    return "\n".join(L)


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
    for sub, spec in subsystems.items():
        for slot, s in spec["slots"].items():
            L += wrapper(sub, spec, slot, s, objects, sub + "." + slot in imported)
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
    L.append("void HookSeams(const bool* on)")
    L.append("{")
    for sub, spec in subsystems.items():
        L.append("    s_inner_%s = g_seams.%s;" % (sub, sub))
        L.append("    s_rec_%s = *s_inner_%s;" % (sub, sub))
    for sub, spec in subsystems.items():
        for slot in spec["slots"]:
            owners = ["on[SUB_%s]" % upper(sub)] + ["on[SUB_%s]" % upper(o) for o in imported.get(sub + "." + slot, [])]
            L.append("    if (%s)" % " || ".join(owners))
            L.append("        s_rec_%s.%s = Rec%s%s;" % (sub, slot, camel(sub), slot))
    for sub in subsystems:
        L.append("    g_seams.%s = &s_rec_%s;" % (sub, sub))
    L += ["}", ""]
    return "\n".join(L)


def wrapper(sub: str, spec: dict, slot: str, s: dict, objects: dict, is_import: bool) -> list[str]:
    params = ", ".join("%s a%d" % (raw_type(s, i), i) for i in range(len(s["args"])))
    call_args = ", ".join("a%d" % i for i in range(len(s["args"])))
    rid = "RSL_%s_%s" % (upper(sub), upper(slot))
    ret = s["ret"]
    names = [a["name"] for a in s["args"]]
    for a in s["args"]:
        if a.get("length"):
            a["lengthIndex"] = names.index(a["length"])
        if a.get("count") in names:
            a["countIndex"] = names.index(a["count"])
    L = ["%s %s Rec%s%s(%s)" % (ret, s["conv"], camel(sub), slot, params), "{"]
    L.append("    ReplayRoots roots;")
    for i, a in enumerate(s["args"]):
        if a["mode"] == "object":
            L.append("    roots.Add(ROK_%s, a%d);" % (upper(a["kind"]), i))
    L.append("    SlotScope scope(%s, SUB_%s, roots);" % (rid, upper(sub)))
    L.append("    if (scope.Open()) {")
    for i, a in enumerate(s["args"]):
        L += ["        " + x for x in arg_out(a, i, "a", objects)]
    L.append("    }")
    if is_import:
        L.append("    ReplayText line;")
        L.append("    if (scope.Import()) {")
        L.append('        line.Add("  i %s.%s");' % (sub, slot))
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
        L.append("    if (scope.Import()) {")
        if res is not None:
            L += ["        " + x for x in result_out(res, objects, "i")]
        for i, a in enumerate(s["args"]):
            if a["mode"] == "bytes-out":
                L.append('        ImportOutBytes(line, "%s", a%d, (int)result, a%d);' % (a["line"], i, a["lengthIndex"]))
            elif a["mode"] == "vec3" and a.get("dir") == "out":
                L.append('        ImportOutVec3(line, "%s", a%d);' % (a["line"], i))
            elif a["mode"] == "units":
                L.append('        ImportOutUnits(line, "%s", a%d, kReplayReachFields + %d, %d);' % (a["line"], i, a["reachFirst"], a["reachCount"]))
        L.append("        FinishImport(line);")
        L.append("    }")
    L.append("    if (scope.Open()) {")
    for i, a in enumerate(s["args"]):
        L += ["        " + x for x in arg_out(a, i, "o", objects)]
    if res is not None and res["mode"] in ("scalar", "object", "value"):
        L += ["        " + x for x in result_out(res, objects, "o")]
    L.append("    }")
    if ret != "void":
        L.append("    return result;")
    L += ["}", ""]
    return L


def raw_type(s: dict, i: int) -> str:
    return s["rawParams"][i] if "rawParams" in s else s["args"][i]["type"]


def arg_out(a: dict, i: int, phase: str, objects: dict) -> list[str]:
    """Lines of C++ recording argument i. phase: 'a' outer entry, 'i' import line, 'o' outer exit."""
    mode, var, line = a["mode"], "a%d" % i, a["line"]
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
        if a.get("dir", "in") == "in" and phase == "a":
            return ['ArgStruct(s_call, "a", "%s", ROK_%s, %s);' % (line, upper(a["kind"]), var)]
        if a.get("dir") == "out" and phase == "o":
            guard = "if (result != 0) " if a.get("when") else ""
            return ['%sArgStruct(s_tail, "o", "%s", ROK_%s, %s);' % (guard, line, upper(a["kind"]), var)]
    elif mode == "structs":
        if phase == "a":
            return ['ArgStructs(s_call, "%s", ROK_%s, %s, (int)a%d);' % (line, upper(a["kind"]), var, a.get("countIndex", 0))]
    elif mode == "value":
        if phase == "a":
            return ['ArgValueStruct(s_call, "a", "%s", ROK_%s, &%s);' % (line, upper(a["kind"]), var)]
        if phase == "i":
            return ["ImportValueStruct(line, ROK_%s, &%s);" % (upper(a["kind"]), var)]
    elif mode == "argv":
        if phase == "a":
            return ["ArgArgv(s_call, %s);" % var]
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
    elif mode == "units":
        return []
    elif mode == "bytes-in":
        if phase == "a":
            return ['ArgPending(s_call, "%s");' % line]
        if phase == "i":
            return ["ImportBytes(line, %s, %s);" % (var, "a%d" % a["lengthIndex"])]
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
            L += import_stub(seam, slot, spec["slots"][slot])
    for sub, spec in ready.items():
        L.append("%s g_native_%s = {" % (spec["struct"], sub))
        for slot, s in spec["slots"].items():
            L.append("    %s," % s["native"])
        L += ["};", ""]
    for sub, spec in ready.items():
        for slot, s in spec["slots"].items():
            L += runner(sub, slot, s)
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
            if any(a["mode"] == "struct" and a.get("dir", "in") == "out" for a in s["args"]) or (res is not None and res["mode"] in ("scalar", "object", "value")):
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


def import_stub(seam: str, slot: str, s: dict) -> list[str]:
    params = ", ".join("%s a%d" % (s["params"][i], i) for i in range(len(s["args"])))
    names = [a["name"] for a in s["args"]]
    for a in s["args"]:
        if a.get("length"):
            a["lengthIndex"] = names.index(a["length"])
    L = ["%s %s Imp_%s_%s(%s)" % (s["ret"], s["conv"], seam, slot, params), "{"]
    L += ["    ReplayText t;", '    StartImport(t, "%s.%s");' % (seam, slot)]
    for i, a in enumerate(s["args"]):
        m, v = a["mode"], "a%d" % i
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
        elif m == "units":
            pass
        elif m == "vec3":
            L.append('    Space(t);\n    t.Add("%s=");\n    AddVec3%s(t, %s);' % (a.get("label", a["line"]), "Xz" if a.get("axes") == "xz" else "", v))
        elif m == "object":
            L.append("    Space(t);\n    ReplayAddObjectName(t, g_objects, ROK_%s, %s, false);" % (upper(a["kind"]), v))
        elif m == "bytes-in":
            L.append("    Space(t);\n    AddBytes(t, %s, a%d);" % (v, a["lengthIndex"]))
        elif m == "value":
            L.append("    Space(t);\n    AddValueStruct(t, ROK_%s, &%s);" % (upper(a["kind"]), v))
        else:
            L.append("    Space(t);\n    t.Add(\"?\");")
    L.append("    const char* r = Enter(t);")
    for i, a in enumerate(s["args"]):
        if a["mode"] == "bytes-out":
            L.append('    ApplyBytes(a%d, a%d, "%s");' % (i, a["lengthIndex"], a["line"]))
        elif a["mode"] == "vec3" and a.get("dir") == "out":
            L.append('    ApplyVec3(a%d, "%s");' % (i, a["line"]))
        elif a["mode"] == "units":
            L.append('    ApplyUnits(a%d, "%s");' % (i, a["line"]))
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


def runner(sub: str, slot: str, s: dict) -> list[str]:
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
            L.append("    %s* %s = (%s*)ArgObject(\"%s\", ROK_%s);" % (a["type"][:-1], v, a["type"][:-1], line, upper(a["kind"])))
            call.append(v)
        elif m == "struct":
            ctype = a["type"][:-1]
            if a.get("dir", "in") == "in" and a.get("nullable"):
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
            call.append("(%s*)%s.data" % (a["type"][:-1], v))
        elif m == "value":
            L.append("    %s %s = %s();\n    ParseValueArg(\"%s\", ROK_%s, &%s);" % (t, v, t, line, upper(a["kind"]), v))
            call.append(v)
        elif m == "argv":
            L.append("    ArgvHolder %s;\n    ParseArgv(%s);" % (v, v))
            call.append("&%s.args" % v)
        else:
            L.append("    ArgPending();")
            call.append("0")
    res = s["result"]
    if res is not None and res["mode"] in ("scalar", "object", "value"):
        closing.append("result")
    L.append("    SetClosing(%s);" % (", ".join('"%s"' % c for c in closing) if closing else "nullptr"))
    expr = "g_seams.%s->%s(%s)" % (sub, slot, ", ".join(call))
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
    for path in sorted((ROOT / "src").rglob("*.cpp")):
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


def render_service_ids(services: list[dict]) -> str:
    L = [GENERATED, "#ifndef HARNESS_REPLAY_SERVICES_H", "#define HARNESS_REPLAY_SERVICES_H", "", "enum ReplayImportId {"]
    L += ["    %s," % s["id"] for s in services]
    L += ["    RI_COUNT", "};", "", "static const char* const kReplayImportNames[RI_COUNT] = {"]
    L += ['    "services.%s",' % s["name"] for s in services]
    L += ["};", "", "#endif", ""]
    return "\n".join(L)


def render_service_end(services: list[dict]) -> str:
    L = [GENERATED, "", "// included by replay.cpp after the helpers of the import lines", "",
         "void ReplayImportEnd(ReplayImport& c)", "{", "    ReplayText& t = s_call;", "    const ReplayArg* a = c.args;",
         '    t.Add("  i ");', "    t.Add(kReplayImportNames[c.id]);", "    switch (c.id) {"]
    for s in services:
        L.append("    case %s:" % s["id"])
        L.append("    {")
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
        res = s.get("result")
        if res == "int":
            L.append("        AddResult(t, c.result);")
        elif res == "double":
            L.append('        t.Add(" = ");')
            L.append("        AddDoubleBits(t, c.result.v.d);")
        elif res == "text":
            L.append('        t.Add(" = ");')
            L.append("        t.AddString((const char*)c.result.v.p);")
        handle_at = [j for j, a in enumerate(s["args"]) if a["mode"] == "handle"]
        for o in s.get("outs", []):
            if "member" in o:
                L.append("        {")
                L.append("            const %s* self = (const %s*)a[%d].v.p;" % (s["owner"], s["owner"], handle_at[0]))
                L.append('            t.Add("\\n  o ");')
                L.append("            AddHandle(t, %s, self);" % handle_id(s["args"][handle_at[0]]["kind"]))
                L.append('            t.Add("%s ");' % o["label"])
                L.append("            AddHandle(t, %s, self->%s);" % (handle_id(o["handle"]), o["member"]))
                L.append("        }")
            else:
                index = [j for j, a in enumerate(s["args"]) if a.get("param") == o["text"]][0]
                L.append('        t.Add("\\n  o %s ");' % o["label"])
                L.append("        t.AddString((const char*)a[%d].v.p);" % index)
        L.append("        break;")
        L.append("    }")
    L += ["    }", "    t.AddChar('\\n');", "    ReplayCalls() = 1;", "}", ""]
    return "\n".join(L)


def render_service_answers(services: list[dict]) -> str:
    L = [GENERATED, "", "// included by replay_native_test.cpp after its helpers", ""]
    for s in services:
        declaration = service_declaration(s).rstrip(";")
        owner = s.get("owner")
        if owner:
            short = declaration.split("(")[0].split()[-1]
            declaration = declaration.replace(short + "(", "%s::%s(" % (owner, short), 1)
        L += [declaration, "{"]
        L += ["    " + stmt for stmt in s.get("pre", [])]
        L += ["    ReplayText t;", "    StartImport(t, %s);" % s["id"]]
        for a in s["args"]:
            if a["mode"] == "skip":
                continue
            v = a.get("param", "this")
            L.append("    Space(t);")
            if a["mode"] == "stream":
                L.append("    ReplayValue(t, %s);" % v)
            elif a["mode"] == "int":
                L.append("    t.AddInt((int)%s);" % v)
            elif a["mode"] == "double":
                L.append("    AddDoubleBits(t, %s);" % v)
            elif a["mode"] == "text":
                L.append("    t.AddString(%s);" % v)
            elif a["mode"] == "handle":
                L.append("    AddObject(t, %s, %s);" % (handle_id(a["kind"]), v))
        res = s.get("result")
        L.append("    const char* %s = Enter(t);" % ("result" if res else "r"))
        for o in s.get("outs", []):
            if "member" in o:
                L.append('    const char* %sText = OutValue("%s");' % (o["member"], o["label"]))
                if o.get("optional"):
                    L.append("    if (%sText != nullptr)\n        %s = ParseRecord(%sText);" % (o["member"], o["member"], o["member"]))
                else:
                    L.append("    %s = %sText == nullptr ? nullptr : ParseRecord(%sText);" % (o["member"], o["member"], o["member"]))
            else:
                L.append('    const char* %sText = OutValue("%s");' % (o["text"], o["label"]))
                L.append("    if (%sText != nullptr)\n        ParseQuoted(%sText, %s, (unsigned)%s);" % (o["text"], o["text"], o["text"], o["size"]))
        L += ["    " + stmt for stmt in s.get("effect", [])]
        if res == "int":
            L.append("    return atoi(result);")
        elif res == "double":
            L.append("    return ParseF64(result);")
        elif res == "text":
            L.append("    ParseQuoted(result, %s, 256);" % s["into"])
            L.append("    return %s;" % s["into"])
        else:
            L.append("    (void)r;")
        L += ["}", ""]
    return "\n".join(L)


def generate(rows: list[dict[str, str]], ann: dict | None = None, assignment: dict | None = None):
    ann = ann or load_annotations()
    assignment = assignment or read_assignment()
    model, errors = build(ann, rows, assignment)
    attach_raw_params(model)
    services = ann["services"]
    errors += check_service_hooks(services)
    for sub, spec in ann["subsystems"].items():
        errors += ["%s lists the service %s, which has no spec" % (sub, n) for n in spec.get("services", [])
                   if not any(x["name"] == n or x["name"].startswith(n + "_") for x in services)]
    files = {
        SCHEMA_H: render_schema(ann, model),
        HOOKS_H: render_hooks(ann, model),
        SLOTS_H: render_slots(ann, model),
        SERVICES_H: render_service_ids(services),
        SERVICES_INC: render_service_end(services),
        SERVICES_SLOTS: render_service_answers(services),
        JSON_OUT: json_text(ann, model),
    }
    return files, errors, model


def attach_raw_params(model: dict) -> None:
    """The parameter types as the header spells them (the classification keeps a normalised copy)."""
    for sub, spec in model["subsystems"].items():
        iface = parse_interface(sub, spec["struct"])
        for s in iface["slots"]:
            slot = spec["slots"][s["name"]]
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
