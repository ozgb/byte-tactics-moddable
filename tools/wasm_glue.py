"""The game manifest's module entries and the generic glue's tables, both made from the seam headers and the replay schema.

Every simulation subsystem has an entry in wasm/modules.json (docs/module-abi.md, 4.2): its window and table range, the
slots it exports (the seam header's order, less the slots that stay in the engine, which the entry's "engine" object
names with their reason), its data words, its imports and the kinds it publishes. src/wasm/glue_tables.cpp holds, for
every entry, the native thunks the seam points at in WASM mode, the timed native slots of a timing run and the import
thunks of the interfaces it imports; src/wasm/module_glue.cpp is the hand-written core they call.

  uv run tools/wasm_glue.py --write    regenerate both
  uv run tools/wasm_glue.py --check    exit 1 when either is stale (CI)
"""

import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import wasm_modules

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCHEMA = os.path.join(ROOT, "data", "replay_schema.json")
TABLES = os.path.join(ROOT, "src", "wasm", "glue_tables.cpp")
INTERFACES = os.path.join(ROOT, "interfaces")
REPLAY = os.path.join(ROOT, "tests", "replay")

MODULES = ["wind_meteors", "economy", "features", "ai", "pathfinding", "weapons", "targeting", "los_radar", "orders",
           "units", "players", "map", "scripts", "movement", "mission"]
FIRST_DATA_BASE = 0x30000
FIRST_TABLE_BASE = 48
WINDOW = 0x8000
TABLE_SLOTS = 32
WORDS = ["g_game", "g_simGlobals", "g_extTdf", "g_blockBase"]
REGION_WORD = "g_region"
WORLD_VIEWS = {"econworld": ("economy", "EconomyWorldView"), "pathworld": ("pathfinding", "PathWorldView")}

BLOCKED = {
    "economy": "a static object with a destructor (ConstructedElement) needs __cxa_atexit, and SimBlockTrack has no import",
    "features": "map/features.cpp mixes the load code (std::vector of TdfFile*, insert and std::copy) with the kept slots, "
                "and the GAF services, which the glue does not give until the frame tables are in the shared memory",
    "ai": "std::vector in ai_player.cpp and squads.cpp, file output (FilePrintf, sprintf) and the command parser",
    "pathfinding": "the direction and turn tables (g_dirDeltaX, g_dirDeltaZ, g_turnCosts) live in another file, and "
                   "PathGoalDistance, PathGoalCells and PathGoalContains are direct calls with no import",
    "weapons": "about 20 engine calls with no import (AngleOfVector, SolveLaunchAngle, InitProjectile, CalcAimAngles, "
               "AddExplosionEffect and others), the GAF services, which the glue does not give until the frame tables are in the shared memory, and the aim-script vtable data",
    "targeting": "unit_query_buffer.h is a std::vector<Ref<Unit>> on the heap",
    "los_radar": "operator new in line_of_sight.cpp, the std::allocator internals of los_tables.cpp and the GAF services, "
                 "which the glue does not give until the frame tables are in the shared memory",
    "orders": "std::list in vtol_orders.cpp, the MSVC STL internals of order_list.cpp and new of sim objects",
    "units": "std::vector, std::list and std::string in units.cpp, unit_commands.cpp, unit_types.cpp and control_panel.cpp, "
             "and UI code in its files",
    "players": "ScheduleMapCanPlaceUnitFootprint, FindHostSlot and the four g_*Key string tables have no import",
    "map": "operator new[] and delete[] of the spatial buckets",
    "scripts": "std::string in unit_script.cpp and ReleaseUnitPictures in its files",
    "movement": "the MSVC vector internals of order_targets.cpp, new and delete of path orders, and PathGoal::DrawOnSurface (UI)",
    "mission": "its slots read a heap singleton (TheMission) with a std::vector map cache",
}

ENGINE_REASONS = ("load class", "save class", "a query of a saved game", "a dump query", "a query for the trace dump",
                  "network class", "presentation", "a query no match makes", "a campaign query", "optional,",
                  "a query of the game data", "only units itself calls it")
ENGINE_SLOTS = {
    ("map", "LoadMetalPlotmap"): "save class: reads the metal plot map of a saved game into a width x height buffer of the engine",
    ("map", "SaveMetalPlotmap"): "save class: writes the metal plot map of a saved game from a width x height buffer of the engine",
    ("map", "LoadPlayerFeaturesPlotmap"): "save class: reads a player's feature plot map of a saved game into a buffer of the engine",
    ("map", "SavePlayerFeaturesPlotmap"): "save class: writes a player's feature plot map of a saved game from a buffer of the engine",
    ("map", "FreePassMaps"): "load class: frees the pass maps when the map is unloaded",
    ("map", "AllocateBuckets"): "load class: allocates the spatial buckets when the map is loaded",
    ("map", "FreeBuckets"): "load class: frees the spatial buckets when the map is unloaded",
}

BANK_SERVICES = {"BankOpenAccount": "HapiBank_OpenAccount", "BankSetInteger": "HapiBank_SetIntegerItem",
                 "BankGetInteger": "HapiBank_GetIntegerItem"}
SCHEDULED = {
    "ScheduleTargetingTargetUnit": ("targeting", "GetWeaponTargetUnit", "ScheduleTargetingTargetUnit({0}, {1})"),
    "ScheduleTargetingTargetPos": ("targeting", "GetWeaponTargetPos", "ScheduleTargetingTargetPos({0}, {1}, {2})"),
    "ScheduleTargetingFind": ("targeting", "FindWeaponTarget", "ScheduleTargetingFind({0}, {1}, {2})"),
    "ScheduleTargetingFireAtWill": ("targeting", "FindBestTargetIfFireAtWill", "ScheduleTargetingFireAtWill({0})"),
    "ScheduleMapCanBuildAt": ("map", "CanBuildAt", "ScheduleMapCanBuildAt({0}, *{1}, {2}, {3})"),
    "ScheduleMapCanPlaceUnitFootprint": ("map", "CanPlaceUnitFootprint", "ScheduleMapCanPlaceUnitFootprint({0}, {1}, *{2}, {3})"),
    "ScheduleMapPickReclaimable": ("map", "PickRandomReclaimableResourcesInRadius",
                                   "ScheduleMapPickReclaimable({0}, {1}, {2}, {3}, {4}, {5})"),
    "ScheduleEconomySpend": ("economy", "Spend", "ScheduleEconomySpend({0})"),
    "ScheduleEconomyRequestGranted": ("economy", "RequestGranted", "ScheduleEconomyRequestGranted({0}, {1})"),
    "ScheduleMetalExtraction": ("economy", "UpdateMetalExtraction", "ScheduleMetalExtraction({0})"),
    "ScheduleFeaturesPlace": ("features", "Place", "ScheduleFeaturesPlace({0})"),
}

SEAM_SLOT = re.compile(r"^\s*(?P<ret>[^()\n/]+?)\s*\((?P<conv>__\w+)\*\s*(?P<name>\w+)\)\((?P<params>[^)]*)\);", re.M)
SERVICE_DECL = re.compile(r"^[^/\n][^\n(]*?\b(\w+)\(", re.M)


def camel(name):
    head, *rest = name.split("_")
    return head + "".join(part.title() for part in rest)


def pascal(name):
    return "".join(part.title() for part in name.split("_"))


def read(path):
    with open(path) as f:
        return f.read()


def seam_slots(header, struct):
    """[(name, conv, ret, [(type, name)])] of a seam struct, in the header's order."""
    text = read(os.path.join(INTERFACES, header))
    start = re.search(rf"^struct {struct} {{", text, re.M)
    if start is None:
        sys.exit(f"interfaces/{header}: no struct {struct}")
    body = text[start.end():text.index("\n};", start.end())]
    slots = []
    for m in SEAM_SLOT.finditer(body):
        params = []
        raw = m["params"].strip()
        if raw and raw != "void":
            for i, part in enumerate(raw.split(",")):
                part = part.strip()
                pm = re.match(r"^(.*?[\s\*&>])(\w+)$", part)
                if pm is None:
                    params.append((part, f"a{i}"))
                else:
                    params.append((pm[1].strip(), pm[2]))
        slots.append((m["name"], m["conv"], m["ret"].strip(), params))
    return slots


def service_names():
    text = wasm_modules.COMMENT.sub("", read(os.path.join(INTERFACES, "services.h")))
    return [m for m in SERVICE_DECL.findall(text) if m not in ("defined",)]


class Schema:
    def __init__(self):
        self.data = json.load(open(SCHEMA))

    def header(self, subsystem):
        return self.data["subsystems"][subsystem]["header"]

    def slot(self, subsystem, name):
        """(args by name, result) of a slot: the recorded entry, else the annotation."""
        recorded = self.data["subsystems"].get(subsystem, {}).get("slots", {}).get(name)
        if recorded is not None:
            return {a["name"]: a for a in recorded.get("args", [])}, recorded.get("result")
        note = self.data["annotations"]["subsystems"].get(subsystem, {}).get("slots", {}).get(name, {})
        return dict(note.get("args", {})), note.get("result")

    def engine_reason(self, subsystem, name):
        if (subsystem, name) in ENGINE_SLOTS:
            return ENGINE_SLOTS[(subsystem, name)]
        note = self.data["annotations"]["subsystems"].get(subsystem, {}).get("slots", {}).get(name, {})
        reason = note.get("reason") if note.get("record") is False else None
        if reason is not None and reason.startswith(ENGINE_REASONS):
            return reason
        return None

    def services(self, subsystem):
        names = self.data["annotations"]["subsystems"].get(subsystem, {}).get("services") or []
        return names


def pointer(ctype):
    return ctype.rstrip().endswith("*")


def pointee(ctype):
    return ctype.rstrip()[:-1].strip()


def scalar_field(ctype):
    base = ctype.replace("const ", "").strip()
    if base == "double":
        return "f64"
    if base == "float":
        return "f32"
    return "i32"


def kind_of(ctype, spec, result=False):
    """The manifest kind of an argument or result (docs/module-abi.md, 4.2)."""
    mode = (spec or {}).get("mode")
    if ctype == "void":
        return "void"
    if not pointer(ctype):
        return {"f64": "f64", "f32": "f32"}.get(scalar_field(ctype), "int")
    if mode == "handle":
        return "handle"
    if mode in ("object", "weapon", "region", "load", "pending"):
        return "block"
    return "linear"


def import_group(subsystem):
    return subsystem


class Slot:
    def __init__(self, subsystem, name, conv, ret, params, schema):
        self.subsystem, self.name, self.conv, self.ret, self.params = subsystem, name, conv, ret, params
        args, result = schema.slot(subsystem, name)
        self.specs = []
        for i, (ctype, pname) in enumerate(params):
            spec = args.get(pname)
            if spec is None:
                spec = next((a for a in args.values() if a.get("index") == i), None)
            self.specs.append(spec or {})
        self.result = result or {}

    def signature(self, imported=False):
        specs = [dict(s, **s.get("import", {})) if imported else s for s in self.specs]
        return [[kind_of(t, s) for (t, _), s in zip(self.params, specs)], kind_of(self.ret, self.result, True)]

    def decl(self, name):
        params = ", ".join(f"{t} {n}" for t, n in self.params) or ""
        return f"{self.ret} {self.conv} {name}({params})"


def seams(schema):
    """{subsystem: [Slot]} of every module and every interface a module imports."""
    out = {}
    for name in os.listdir(INTERFACES):
        if not name.endswith(".h"):
            continue
        text = read(os.path.join(INTERFACES, name))
        for struct in re.findall(r"^struct (\w+Seam) {", text, re.M):
            subsystem = name[:-2]
            out[subsystem] = [Slot(subsystem, *s, schema) for s in seam_slots(name, struct)]
    for view, (owner, struct) in WORLD_VIEWS.items():
        out[view] = [Slot(view, *s, schema) for s in seam_slots(owner + ".h", struct)]
    return out


def recorded_services(subsystem):
    """The services names a subsystem's recordings import, in the order they first appear."""
    names = []
    directory = os.path.join(REPLAY, subsystem)
    if not os.path.isdir(directory):
        return names
    for file in sorted(os.listdir(directory)):
        if not file.endswith(".txt"):
            continue
        for m in re.finditer(r"^\s*i services\.(\w+)", read(os.path.join(directory, file)), re.M):
            if m[1] not in names:
                names.append(m[1])
    return names


def module_imports(subsystem, schema, all_seams, services):
    """{group: [name]} of a module's imports: the header's interface imports, the scheduled services as the interface
    slots they route, and the services its recordings and annotations name, in services.h order."""
    groups = {}

    def add(group, name):
        groups.setdefault(group, [])
        if name not in groups[group]:
            groups[group].append(name)

    wanted = set()
    for item in schema.header(subsystem).get("imports", []):
        group, name = item.split(".")
        if group == "files" and name in BANK_SERVICES:
            wanted.add(BANK_SERVICES[name])
            continue
        add(group, name)
    for name in schema.services(subsystem):
        if name in ("TdfFile", "TdfRecord"):
            wanted.update(s for s in services if s.startswith(name + "_"))
        else:
            wanted.add(name)
    for name in recorded_services(subsystem):
        if name in SCHEDULED:
            group, slot, _ = SCHEDULED[name]
            add(group, slot)
        elif name == "SchedPublish":
            wanted.add("Emit")
        else:
            wanted.add(name)
    for name in list(wanted):
        if name in SCHEDULED:
            group, slot, _ = SCHEDULED[name]
            add(group, slot)
    service_list = [s for s in services if s in wanted]
    for group in groups:
        names = {s.name for s in all_seams[group]}
        missing = [n for n in groups[group] if n not in names]
        if missing:
            sys.exit(f"{subsystem}: imports {group}.{missing}, which interfaces declare no slot of")
    for group in groups:
        if group in MODULES:
            groups[group] = [n for n in groups[group]
                             if not (schema.engine_reason(group, n) or "").startswith(("load class", "save class"))]
    groups = {g: names for g, names in groups.items() if names}
    order = {s.name: i for g in groups for i, s in enumerate(all_seams[g])}
    out = {group: sorted(groups[group], key=lambda n: order[n]) for group in groups}
    if service_list:
        out["services"] = service_list
    return out


def entries(schema, all_seams, game):
    old = {m["name"]: m for m in game["modules"]}
    services = service_names()
    service_sigs = game["signatures"]["services"]
    out = []
    for index, subsystem in enumerate(MODULES):
        slots = all_seams[subsystem]
        engine = {s.name: r for s in slots if (r := schema.engine_reason(subsystem, s.name)) is not None}
        exports = [s.name for s in slots if s.name not in engine]
        imports = module_imports(subsystem, schema, all_seams, services)
        region = any(spec.get("mode") == "region" for s in slots for spec in s.specs)
        entry = {"name": subsystem, "interface": subsystem}
        keep = old.get(subsystem, {})
        for key in ("fuel_budget", "size_budget"):
            if key in keep:
                entry[key] = keep[key]
        entry["data_base"] = FIRST_DATA_BASE + index * WINDOW
        entry["table_base"] = FIRST_TABLE_BASE + index * TABLE_SLOTS
        for key in ("cxx", "c"):
            if key in keep:
                entry[key] = keep[key]
        if subsystem in BLOCKED:
            entry["blocked"] = BLOCKED[subsystem]
        entry["exports"] = exports
        entry["data"] = WORDS + ([REGION_WORD] if region else [])
        if engine:
            entry["engine"] = engine
        by_name = {s.name: s for s in slots}
        entry["slots"] = {n: by_name[n].signature() for n in exports}
        entry["imports"] = {}
        for group, names in imports.items():
            if group == "services":
                entry["imports"][group] = {n: service_sigs[n] for n in names}
            else:
                seam = {s.name: s for s in all_seams[group]}
                entry["imports"][group] = {n: seam[n].signature(imported=True) for n in names}
        publishes = {str(k): v for k, v in sorted(wasm_modules.routed_kinds(subsystem).items())}
        if publishes:
            entry["publishes"] = publishes
        out.append(entry)
    return out


def updated_manifest(schema, all_seams):
    game = wasm_modules.game_manifest()
    modules = entries(schema, all_seams, game)
    out = {}
    for key, value in game.items():
        if key == "window":
            value = WINDOW
        elif key == "table_size":
            value = FIRST_TABLE_BASE + len(MODULES) * TABLE_SLOTS
        elif key == "modules":
            value = modules
        out[key] = value
    return wasm_modules.with_majors(out, wasm_modules.header_majors())


class Writer:
    def __init__(self):
        self.lines = []

    def __call__(self, line=""):
        self.lines.append(line)

    def text(self):
        return "\n".join(self.lines).rstrip("\n") + "\n"


EVENT_CAPS = None


def event_cap(events, header_type):
    import traceschema
    module = next((m for m in traceschema.EVENT_MODULES if m.key == camel(events)), None)
    if module is None:
        return f"sizeof({header_type})"
    sizes = [f"sizeof({module.header})"]
    for kind, struct in module.payloads.items():
        if struct is not None and "_EFFECT_" not in kind:
            sizes.append(f"sizeof({struct})")
    expr = sizes[0]
    for size in sizes[1:]:
        expr = f"GlueMax({expr}, {size})"
    return expr


def slot_thunk(w, subsystem, index, slot, glue):
    """The native function the seam holds in WASM mode for one module slot."""
    w(f"{slot.decl(f'Glue{pascal(subsystem)}{slot.name}')}")
    w("{")
    w(f"    GlueModule* glue = &{glue};")
    w("    unsigned mark = GlueScratchMark();")
    count = max(len(slot.params), 0 if slot.ret == "void" else 1)
    w(f"    wasmtime_val_raw_t slotArgs[{max(count, 1)}];")
    w("    slotArgs[0].i64 = 0;")
    after = []
    capped = {}
    for i, ((ctype, name), spec) in enumerate(zip(slot.params, slot.specs)):
        mode = spec.get("mode")
        if mode == "event" and "sizeIndex" in spec:
            cap = event_cap(spec.get("events", subsystem), pointee(ctype).replace("const ", ""))
            size = slot.params[spec["sizeIndex"]][1]
            w(f"    unsigned {name}Size = {size} > {cap} ? (unsigned){cap} : (unsigned){size};")
            capped[spec["sizeIndex"]] = f"{name}Size"
    for i, ((ctype, name), spec) in enumerate(zip(slot.params, slot.specs)):
        mode = spec.get("mode")
        if i in capped:
            w(f"    slotArgs[{i}].i32 = (int){capped[i]};")
            continue
        if not pointer(ctype):
            field = scalar_field(ctype)
            cast = {"f64": "(double)", "f32": "(float)", "i32": "(int)"}[field]
            w(f"    slotArgs[{i}].{field} = {cast}{name};")
            continue
        base = pointee(ctype).replace("const ", "").strip()
        if mode == "handle":
            w(f"    slotArgs[{i}].i32 = (int)GlueHandleOf((void*){name});")
        elif mode == "text":
            w(f"    slotArgs[{i}].i32 = (int)GlueText(glue, {index}, {name});")
        elif mode in ("object", "load", "pending", "weapon"):
            w(f"    slotArgs[{i}].i32 = (int)GlueObject(glue, {index}, {name});")
        elif mode == "region":
            w(f"    glue->region = (void*){name};")
            w(f"    slotArgs[{i}].i32 = (int)GlueObject(glue, {index}, {name});")
        elif mode == "world":
            w(f"    glue->world = {name};")
            w(f"    slotArgs[{i}].i32 = 0;")
        elif mode == "event":
            size = f"{name}Size" if f"{name}Size" in capped.values() else f"sizeof({base})"
            w(f"    slotArgs[{i}].i32 = (int)GlueIn(glue, {index}, {name}, {size}, 1);")
        elif mode in ("bytes-in", "bytes-out", "units-buffer", "scratch", "structs"):
            length = spec.get("length") or spec.get("capacity") or spec.get("count")
            unit = "1" if mode in ("bytes-in", "bytes-out") else f"sizeof({base})"
            size = f"(unsigned)({length}) * {unit}"
            copy_in = 1 if mode in ("bytes-in", "structs") else 0
            w(f"    unsigned {name}At = GlueIn(glue, {index}, {name}, {size}, {copy_in});")
            w(f"    slotArgs[{i}].i32 = (int){name}At;")
            if mode != "bytes-in" and not ctype.startswith("const "):
                after.append(f"    GlueOut((void*){name}, {name}At, {size});")
        elif not mode:
            w(f"    slotArgs[{i}].i32 = (int)GlueObject(glue, {index}, {name});")
        else:
            if base == "void":
                sys.exit(f"{subsystem}.{slot.name}: argument {name} is void* of mode {mode}, which the glue cannot size")
            direction = spec.get("dir", "in")
            if spec.get("byPointer") or ctype.startswith("const "):
                direction = "in"
            w(f"    unsigned {name}At = GlueIn(glue, {index}, {name}, sizeof({base}), {0 if direction == 'out' else 1});")
            w(f"    slotArgs[{i}].i32 = (int){name}At;")
            if direction != "in":
                after.append(f"    GlueOut((void*){name}, {name}At, sizeof({base}));")
    w(f"    GlueCall(glue, {index}, slotArgs, {count});")
    for line in after:
        w(line)
    w("    GlueScratchRelease(mark);")
    if slot.ret != "void":
        if pointer(slot.ret):
            in_block = 0 if slot.result.get("mode") == "text" else 1
            w(f"    return ({slot.ret})GlueResult(glue, {index}, (unsigned)slotArgs[0].i32, {in_block});")
        else:
            field = scalar_field(slot.ret)
            w(f"    return ({slot.ret})slotArgs[0].{field};")
    w("}")
    w()


def timed_native(w, subsystem, index, slot, glue, native):
    w(f"{slot.decl(f'Glue{pascal(subsystem)}Timed{slot.name}')}")
    w("{")
    w("    unsigned long long start = host_time_counter();")
    call = f"{native}->{slot.name}({', '.join(n for _, n in slot.params)})"
    if slot.ret == "void":
        w(f"    {call};")
        w(f"    GlueTimed(&{glue}, {index}, start);")
    else:
        w(f"    {slot.ret} result = {call};")
        w(f"    GlueTimed(&{glue}, {index}, start);")
        w("    return result;")
    w("}")
    w()


def import_thunk(w, group, slot, function):
    """The host function of one interface import: module addresses in, the seam's slot (or the scheduled service), the result out."""
    label = f"{group}.{slot.name}"
    w(f"const char* __cdecl {function}(wasmtime_val_raw_t* a)")
    w("{")
    names = []
    fixups = []
    specs = [dict(s, **s.get("import", {})) for s in slot.specs]
    for i, ((ctype, name), spec) in enumerate(zip(slot.params, specs)):
        if not pointer(ctype):
            field = scalar_field(ctype)
            w(f"    {ctype} {name} = ({ctype})a[{i}].{field};")
        elif spec.get("mode") == "handle":
            w(f"    void* {name}Native;")
            w(f"    const char* {name}Error = GlueLiveOf(\"{label}\", (unsigned)a[{i}].i32, &{name}Native);")
            w(f"    if ({name}Error != nullptr)")
            w(f"        return {name}Error;")
            w(f"    {ctype} {name} = ({ctype}){name}Native;")
        else:
            w(f"    GLUE_NEED(\"{label}\", {i});")
            w(f"    {ctype} {name} = ({ctype})GlueLinear((unsigned)a[{i}].i32);")
            if pointee(ctype).endswith("*"):
                fixups.append(name)
        names.append(name)
    scheduled = next((call for _, (g, s, call) in SCHEDULED.items() if g == group and s == slot.name), None)
    if scheduled is not None:
        call = scheduled.format(*names)
    elif group in WORLD_VIEWS:
        struct = WORLD_VIEWS[group][1]
        if group == "pathworld":
            target = "g_seams.pathworld"
        else:
            target = f"((const {struct}*)GlueActive()->world)"
            w("    if (GlueActive() == nullptr || GlueActive()->world == nullptr)")
            w(f"        return GlueImportError(\"{label}\", \"the world view is not set\");")
        call = f"{target}->{slot.name}({', '.join(names)})"
    else:
        call = f"g_seams.{group}->{slot.name}({', '.join(names)})"
    if slot.ret == "void":
        w(f"    {call};")
    else:
        w(f"    {slot.ret} result = {call};")
    for name in fixups:
        w(f"    if ({name} != nullptr) {{")
        w("        unsigned address;")
        w(f"        const char* error = GlueAddressOf(\"{label}\", *(void**){name}, 0, &address, \"pointer\");")
        w("        if (error != nullptr)")
        w("            return error;")
        w(f"        *(unsigned*){name} = address;")
        w("    }")
    if slot.ret != "void":
        if pointer(slot.ret):
            in_block = 0 if slot.result.get("mode") == "text" else 1
            w("    unsigned address;")
            noun = {"weapon": "weapon", "object": "object", "region": "region"}.get(slot.result.get("mode"), "pointer")
            w(f"    const char* error = GlueAddressOf(\"{label}\", (const void*)result, {in_block}, &address, \"{noun}\");")
            w("    a[0].i32 = (int)address;")
            w("    return error;")
            w("}")
            w()
            return
        field = scalar_field(slot.ret)
        cast = {"f64": "(double)", "f32": "(float)", "i32": "(int)"}[field]
        w(f"    a[0].{field} = {cast}result;")
    w("    return nullptr;")
    w("}")
    w()


def letters(signature):
    args, ret = signature
    code = {"int": "i", "linear": "i", "block": "i", "handle": "i", "scratch": "i", "f32": "f", "f64": "d", "i64": "I"}
    return "".join(code[k] for k in args) + ">" + ("" if ret == "void" else code[ret])


def tables(manifest, all_seams):
    import routes

    w = Writer()
    w("#include <wasmtime.h>")
    w("#include \"../units/unit_def.h\"")
    w("#include \"../../interfaces/host.h\"")
    w("#include \"../util/vec3.h\"")
    w("#include \"../util/angles.h\"")
    for header in ("wind_meteors", "economy", "features", "ai", "pathfinding", "weapons", "targeting", "los_radar", "orders",
                   "units", "players", "map", "scripts", "movement", "mission", "network", "ref", "rand_stream", "services", "seams"):
        w(f"#include \"../../interfaces/{header}.h\"")
    w("#include \"../util/engine_services.h\"")
    w("#include \"module_glue.h\"")
    w()
    w("namespace {")
    w()
    w("constexpr unsigned GlueMax(unsigned a, unsigned b)")
    w("{")
    w("    return a > b ? a : b;")
    w("}")
    w()
    defs = []
    for entry in manifest["modules"]:
        subsystem = entry["name"]
        p = pascal(subsystem)
        seam_type = next(s for s in re.findall(r"^struct (\w+Seam) {", read(os.path.join(INTERFACES, subsystem + ".h")), re.M))
        slots = {s.name: s for s in all_seams[subsystem]}
        exports = entry["exports"]
        glue = f"s_{camel(subsystem)}Glue"
        native = f"s_{camel(subsystem)}Native"
        w(f"const char* const k{p}Slots[] = {{")
        for name in exports:
            w(f"    \"{name}\",")
        w("};")
        w(f"wasmtime_func_t s_{camel(subsystem)}Funcs[{len(exports)}];")
        w(f"GlueTiming s_{camel(subsystem)}Timing[{len(exports)}];")
        w(f"extern GlueModule {glue};")
        w(f"const {seam_type}* {native};")
        w(f"{seam_type} s_{camel(subsystem)}Seam;")
        w(f"{seam_type} s_{camel(subsystem)}Timed;")
        w()
        for index, name in enumerate(exports):
            slot_thunk(w, subsystem, index, slots[name], glue)
        for index, name in enumerate(exports):
            timed_native(w, subsystem, index, slots[name], glue, native)
        field = subsystem
        w(f"void {p}UseSeam(GlueModule* glue)")
        w("{")
        w("    (void)glue;")
        w(f"    if (g_seams.{field} == &s_{camel(subsystem)}Seam)")
        w("        return;")
        w(f"    {native} = g_seams.{field};")
        w(f"    s_{camel(subsystem)}Seam = *{native};")
        for name in exports:
            w(f"    s_{camel(subsystem)}Seam.{name} = Glue{p}{name};")
        w(f"    g_seams.{field} = &s_{camel(subsystem)}Seam;")
        w("}")
        w()
        w(f"void {p}TimeNative()")
        w("{")
        w(f"    {native} = g_seams.{field};")
        w(f"    s_{camel(subsystem)}Timed = *{native};")
        for name in exports:
            w(f"    s_{camel(subsystem)}Timed.{name} = Glue{p}Timed{name};")
        w(f"    g_seams.{field} = &s_{camel(subsystem)}Timed;")
        w("}")
        w()
        outbox = f"SCHED_{subsystem.upper()}" if subsystem != "mission" else "-1"
        w(f"const GlueModuleDef k{p}Def = {{\"{subsystem}\", {outbox}, k{p}Slots, {len(exports)}, s_{camel(subsystem)}Funcs, "
          f"s_{camel(subsystem)}Timing, {p}UseSeam, {p}TimeNative, {1 if 'blocked' in entry else 0}, "
          f"{0 if 'Emit' in recorded_services(subsystem) else 1}}};")
        w(f"GlueModule {glue} = {{&k{p}Def}};")
        w()
        defs.append(glue)
    imports = {}
    for entry in manifest["modules"]:
        for group, names in entry["imports"].items():
            if group == "services":
                continue
            for name in names:
                imports.setdefault((group, name), names[name])
    functions = []
    for (group, name), signature in sorted(imports.items()):
        slot = next(s for s in all_seams[group] if s.name == name)
        function = f"GlueImport{pascal(group)}{name}"
        import_thunk(w, group, slot, function)
        functions.append((group, name, letters(signature), function))
    w("GlueModule* const kGlueModules[] = {")
    for glue in defs:
        w(f"    &{glue},")
    w("};")
    w()
    w("const GlueImport kGlueInterfaceImports[] = {")
    for group, name, sig, function in functions:
        w(f"    {{\"{group}\", \"{name}\", \"{sig}\", {function}}},")
    w("};")
    w()
    rows = sorted({(publisher, int(value)) for publisher, _kind, value, _sub, _how in routes.rows()},
                  key=lambda r: (MODULES.index(r[0]) if r[0] in MODULES else -1, r[1]))
    w("const GlueRoute kGlueRoutes[] = {")
    for publisher, value in rows:
        w(f"    {{SCHED_{publisher.upper()}, {value}}},")
    w("};")
    w()
    w("}")
    w()
    for function, kind, table in (("GlueModules", "GlueModule* const", "kGlueModules"),
                                  ("GlueInterfaceImports", "const GlueImport", "kGlueInterfaceImports"),
                                  ("GlueRoutes", "const GlueRoute", "kGlueRoutes")):
        w(f"{kind}* {function}(unsigned* count)")
        w("{")
        w(f"    *count = sizeof {table} / sizeof {table}[0];")
        w(f"    return {table};")
        w("}")
        w()
    return w.text()


def build():
    schema = Schema()
    all_seams = seams(schema)
    manifest = updated_manifest(schema, all_seams)
    return wasm_modules.manifest_text(manifest), tables(manifest, all_seams)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--write", action="store_true")
    group.add_argument("--check", action="store_true")
    args = parser.parse_args()
    manifest_text, tables_text = build()
    targets = [(wasm_modules.GAME_MODULES, manifest_text), (TABLES, tables_text)]
    stale = []
    for path, text in targets:
        current = read(path) if os.path.exists(path) else None
        if current == text:
            continue
        if args.write:
            with open(path, "w", newline="\n") as f:
                f.write(text)
            print(f"wrote {os.path.relpath(path, ROOT)}")
        else:
            stale.append(os.path.relpath(path, ROOT))
    if stale:
        print(f"stale: {', '.join(stale)}: run uv run tools/wasm_glue.py --write")
        return 1
    if args.check:
        print("ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
