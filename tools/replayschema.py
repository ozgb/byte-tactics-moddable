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
SIZES = {"i32": 4, "u32": 4, "u16": 2, "f32": 4}
BLOCK_KINDS = {"field_read": "r", "field_write": "w", "global_read": "r", "global_write": "w", "global_maybe_written": "rw"}
OBJECT_KINDS = {"field_read": "r", "field_write": "w", "field_maybe_written": "rw"}
FIELD_KINDS = {"i32": "I32", "u32": "U32", "u16": "U16", "i16": "I16", "f32": "F32", "point16": "POINT16", "string": "STRING",
               "weapon": "WEAPON", "ref": "REF", "flags": "FLAGS"}
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
    sim = {k: tuple(v) for k, v in ann["sim_fields"].items()}
    return game, sim


def tracked(target: str, game: dict, sim: dict) -> str | None:
    if target.startswith("Game::") and target[6:] in game:
        return "Game." + target[6:]
    if target in sim:
        return "SimGlobals." + target
    return None


def object_target(target: str, ann: dict) -> tuple[str, str] | None:
    """(kind, field) when the target is `Type::member` of an object kind, a field of its table."""
    if "::" not in target:
        return None
    type_name, member = target.split("::", 1)
    for kind, spec in ann["objects"].items():
        if spec["type"] == type_name and any(f["name"] == member for f in spec["fields"]):
            return kind, member
    return None


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


def build(ann: dict, rows: list[dict[str, str]], assignment: dict[str, list[str]]) -> dict:
    """The generated model: fields, objects, subsystems, and the errors."""
    game, sim = block_tables(ann)
    errors = check_sim_names(sim)
    fields = field_table(game, sim)
    field_index = {f["name"]: i for i, f in enumerate(fields)}
    objects = []
    object_fields = []
    kind_names = list(ann["objects"])
    for kind in kind_names:
        spec = ann["objects"][kind]
        objects.append({"name": kind, "type": spec["type"], "scope": spec["scope"]})
        for f in spec["fields"]:
            object_fields.append(dict(f, object=kind))
    object_index = {(f["object"], f["name"]): i for i, f in enumerate(object_fields)}
    by_function: dict[str, list[dict]] = {}
    for r in rows:
        by_function.setdefault(r["function"], []).append(r)
    used_rules = set()
    rule_of = {(r["kind"], r["target"]): r for r in ann["rules"]}
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
            for fn in functions:
                for r in by_function.get(fn, []):
                    kind, target = r["kind"], r["target"]
                    if kind == "call" and target in RNG_CALLS:
                        draws = True
                    if kind in BLOCK_KINDS:
                        name = tracked(target, game, sim)
                        if name is not None:
                            if "r" in BLOCK_KINDS[kind]:
                                reads.add(name)
                            if "w" in BLOCK_KINDS[kind]:
                                writes.add(name)
                        elif target in ann["exempt"]:
                            pass
                        elif target.startswith("Game::") or target.startswith("g_"):
                            pending["untracked"].add(target)
                    if kind in OBJECT_KINDS:
                        hit = object_target(target, ann)
                        if hit is not None:
                            i = object_index[(hit[0], hit[1])]
                            if "r" in OBJECT_KINDS[kind]:
                                obj_reads.add(i)
                            if "w" in OBJECT_KINDS[kind]:
                                obj_writes.add(i)
                    if kind == "field_escape" or (kind in ("deref_read", "deref_write") and BARE_POINTER.match(target)):
                        rule = rule_of.get((kind, target))
                        if rule is None:
                            pending["rules"].add("%s %s" % (kind, target))
                        else:
                            used_rules.add((rule["kind"], rule["target"]))
                            if rule["rule"] == "whole-object":
                                for i, f in enumerate(object_fields):
                                    if f["object"] == rule["object"]:
                                        obj_reads.add(i)
                                        obj_writes.add(i)
            if draws:
                reads.update(RNG_FIELDS)
                writes.update(RNG_FIELDS)
            order = list(field_index)
            args = [classify_arg(p, note.get("args", {}).get(p["name"]), ann) for p in s["params"]]
            result = classify_result(s["ret"], note.get("result"), ann)
            for a in args:
                if a["mode"] == "pending":
                    pending["args"].append("%s(%s)" % (s["name"], a["name"]))
            if result is not None and result["mode"] == "pending":
                pending["args"].append("%s result" % s["name"])
            slots[s["name"]] = {
                "conv": s["conv"], "ret": s["ret"], "functions": functions,
                "reads": [f for f in order if f in reads], "writes": [f for f in order if f in writes],
                "objectReads": sorted(obj_reads), "objectWrites": sorted(obj_writes),
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
    stale = sorted("%s %s" % k for k in rule_of if k not in used_rules)
    errors += ["the rule for %s matches no row of a subsystem function" % s for s in stale]
    for sub, spec in subsystems.items():
        for imp in spec["imports"]:
            seam, slot = imp.split(".")
            if slot not in subsystems.get(seam, {}).get("slots", {}):
                errors.append("%s imports %s, which is no slot of the %s interface" % (sub, imp, seam))
    return {"fields": fields, "objects": objects, "object_fields": object_fields, "subsystems": subsystems, "slot_list": slot_list}, errors


def check_sim_names(sim: dict) -> list[str]:
    text = SIM_GLOBALS_H.read_text()
    known = load_sim_map()
    return ["SimGlobals has no member %s" % n for n in sim if n + ";" not in text and n not in known.fields]


def field_table(game: dict, sim: dict) -> list[dict]:
    schema = traceschema.Schema()
    layout = {l["name"]: l for l in schema.layout("Game")[0]}
    fields = []
    for name, kind in game.items():
        leaf = layout.get(name)
        if leaf is None or leaf["size"] != SIZES[kind]:
            raise Error("Game has no field %s of %d bytes" % (name, SIZES[kind]))
        fields.append({"name": "Game." + name, "base": "game", "offset": leaf["offset"], "size": leaf["size"], "kind": kind})
    for name, (kind, size) in sim.items():
        fields.append({"name": "SimGlobals." + name, "base": "sim", "offset": name, "size": size, "kind": kind})
    for i, name in enumerate(RNG_FIELDS):
        fields.append({"name": name, "base": "rng", "offset": i, "size": 4, "kind": "u32"})
    return fields


def upper(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "_", name).upper()


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
    for name in sim:
        L += ['#pragma push_macro("%s")' % name, "#undef %s" % name]
    L += ["", "static const ReplayField kReplayFields[] = {"]
    for f in fields:
        base = {"game": "RB_GAME", "sim": "RB_SIM", "rng": "RB_RNG"}[f["base"]]
        offset = "offsetof(SimGlobals, %s)" % f["offset"] if f["base"] == "sim" else "%d" % f["offset"]
        L.append('    { "%s", %s, %d, %s, RK_%s },' % (f["name"], offset, f["size"], base, FIELD_KINDS[f["kind"]]))
    L += ["};", "static const unsigned kReplayFieldCount = %d;" % len(fields), ""]
    for name in sim:
        L.append('#pragma pop_macro("%s")' % name)
    L += ["", "enum ReplayFieldIndex {"]
    L += ["    RF_%s," % upper(f["name"]) for f in fields]
    L += ["};", "", "enum ReplayObjectKindId {"]
    L += ["    ROK_%s," % upper(o["name"]) for o in objects]
    L += ["    ROK_COUNT", "};", "", "static const ReplayObjectKind kReplayObjectKinds[] = {"]
    scope = {"state": "RS_STATE", "index": "RS_INDEX", "value": "RS_VALUE"}
    for o in objects:
        L.append('    { "%s", sizeof(%s), %s },' % (o["name"], o["type"], scope[o["scope"]]))
    L += ["};", "static const unsigned kReplayObjectKindCount = %d;" % len(objects), "",
          "static const ReplayObjectField kReplayObjectFields[] = {"]
    kind_id = {o["name"]: "ROK_" + upper(o["name"]) for o in objects}
    types = {o["name"]: o["type"] for o in objects}
    for f in ofields:
        member = f.get("member", f["name"])
        t = types[f["object"]]
        size = f.get("size")
        size_text = str(size) if size else "sizeof(((%s*)nullptr)->%s)" % (t, member)
        target = kind_id[f["target"]] if f["kind"] == "ref" else "0"
        L.append('    { "%s", offsetof(%s, %s), %s, %s, RK_%s, %s },' % (f["name"], t, member, size_text, kind_id[f["object"]], FIELD_KINDS[f["kind"]], target))
    L += ["};", "static const unsigned kReplayObjectFieldCount = %d;" % len(ofields), ""]
    for f in ofields:
        t = types[f["object"]]
        member = f.get("member", f["name"])
        if f.get("size") is None:
            want = {"i32": 4, "u32": 4, "u16": 2, "i16": 2, "f32": 4, "flags": 4, "ref": 4, "point16": 4, "weapon": 4}[f["kind"]]
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
            L.append('    { "%s", SUB_%s, %s, %d, %s, %d, %s, %d, %s, %d },' % (slot, upper(sub), r[0], r[1], w[0], w[1], ro[0], ro[1], wo[0], wo[1]))
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
            L.append("        s_rec_%s.%s = Rec_%s_%s;" % (sub, slot, sub, slot))
    for sub in subsystems:
        L.append("    g_seams.%s = &s_rec_%s;" % (sub, sub))
    L += ["}", ""]
    return "\n".join(L)


def wrapper(sub: str, spec: dict, slot: str, s: dict, objects: dict, is_import: bool) -> list[str]:
    params = ", ".join("%s a%d" % (raw_type(s, i), i) for i in range(len(s["args"])))
    call_args = ", ".join("a%d" % i for i in range(len(s["args"])))
    rid = "RSL_%s_%s" % (upper(sub), upper(slot))
    ret = s["ret"]
    L = ["%s %s Rec_%s_%s(%s)" % (ret, s["conv"], sub, slot, params), "{"]
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
        L.append("        FinishImport(line);")
        L.append("    }")
    L.append("    if (scope.Open()) {")
    for i, a in enumerate(s["args"]):
        L += ["        " + x for x in arg_out(a, i, "o", objects)]
    if res is not None and res["mode"] in ("scalar", "object"):
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
            return ['ArgStruct(s_tail, "o", "%s", ROK_%s, %s);' % (line, upper(a["kind"]), var)]
    elif mode == "argv":
        if phase == "a":
            return ["ArgArgv(s_call, %s);" % var]
    elif mode == "weapon":
        if phase == "a":
            return ['ArgWeapon(s_call, "%s", %s);' % (line, var)]
        if phase == "i":
            return ["ImportWeapon(line, %s);" % var]
    elif mode == "vec3":
        if phase == "a":
            return ['ArgVec3(s_call, "%s", %s);' % (line, var)]
        if phase == "i":
            return ['ImportVec3(line, "%s", %s);' % (a.get("label", line), var)]
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
        return []
    lines = []
    if mode == "scalar":
        lines.append("ImportResult(line, result);")
    elif mode == "text":
        lines.append("ImportResult(line, (const char*)result);")
    elif mode == "object":
        lines.append("ImportResultObject(line, ROK_%s, result);" % upper(res["kind"]))
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
            imports.setdefault(seam, []).append(slot)
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
            if any(a["mode"] == "struct" and a.get("dir", "in") == "out" for a in s["args"]) or (res is not None and res["mode"] in ("scalar", "object")):
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
        elif m == "vec3":
            L.append('    Space(t);\n    t.Add("%s=");\n    AddVec3(t, %s);' % (a.get("label", a["line"]), v))
        else:
            L.append("    Space(t);\n    t.Add(\"?\");")
    L.append("    const char* r = Enter(t);")
    res = s["result"]
    if res is None:
        L.append("    (void)r;")
    elif res["mode"] == "scalar":
        L.append("    return ParseResult<%s>(r);" % s["ret"])
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
            if a.get("dir", "in") == "in":
                L.append("    %s %s;\n    memset(&%s, 0, sizeof %s);" % (ctype, v, v, v))
                L.append('    ParseStruct("%s", ROK_%s, &%s);' % (line, upper(a["kind"]), v))
                call.append("&" + v)
            else:
                g = "g_out_%s_%s_%d" % (sub, slot, i)
                head.append("%s %s;" % (ctype, g))
                L.append("    memset(&%s, 0, sizeof %s);" % (g, g))
                closing.append(line + ".")
                after.append('    CompareStruct("%s", ROK_%s, &%s);' % (line, upper(a["kind"]), g))
                call.append("&" + g)
        elif m == "argv":
            L.append("    ArgvHolder %s;\n    ParseArgv(%s);" % (v, v))
            call.append("&%s.args" % v)
        else:
            L.append("    ArgPending();")
            call.append("0")
    res = s["result"]
    if res is not None and res["mode"] in ("scalar", "object"):
        closing.append("result")
    L.append("    SetClosing(%s);" % (", ".join('"%s"' % c for c in closing) if closing else "nullptr"))
    expr = "g_seams.%s->%s(%s)" % (sub, slot, ", ".join(call))
    if res is None:
        L.append("    " + expr + ";")
    else:
        g = "g_result_%s_%s" % (sub, slot)
        head.append("%s %s;" % (s["ret"], g))
        L.append("    %s = %s;" % (g, expr))
        if res["mode"] in ("scalar", "object"):
            after.append("    CompareResult(%s);" % g)
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


def generate(rows: list[dict[str, str]], ann: dict | None = None, assignment: dict | None = None):
    ann = ann or load_annotations()
    assignment = assignment or read_assignment()
    model, errors = build(ann, rows, assignment)
    attach_raw_params(model)
    files = {
        SCHEMA_H: render_schema(ann, model),
        HOOKS_H: render_hooks(ann, model),
        SLOTS_H: render_slots(ann, model),
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
