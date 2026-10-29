"""Measure what running a subsystem as WASM costs (a local report, not a CI check).

    uv run tools/wasm_bench.py run --exe build/modern/TotalA.exe --game-dir ~/.ta [--scenario long-greenhaven-8p]
    uv run tools/wasm_bench.py ref-add

run plays one scenario twice under Wine, once with the modules in WASM and once native (-modules=native), each with
the module timing on, and prints per tick the slot calls and import calls and the time the module's slots take. Timing
is off unless the game directory holds a file moduletime.on (this tool puts one there with playtest's --put); a test
run then times, through host_time_counter, every slot call and every import call and writes moduletime.txt. A
timed WASM tick splits into:
  glue     the thunk's work outside the module (checks, copying g_game and the other pointers in, setting the fuel
           counter) plus the entry and exit of wasmtime_func_call_unchecked, which is the measured cost of calling an
           empty export (__fuel_set) once per slot call
  imports  the time inside import calls: the glue's translation and the native service (SpawnProjectile and the like)
  module   the rest of the time inside wasmtime_func_call_unchecked: the module's own code
Each timed interval carries about one timer read (timer_cost, measured at the end of the run), which is taken off.
The test-mode loop is not paced, so the scenario runs as fast as it can; the 10x game speed is 300 ticks a second, or
3.33 ms a tick, and the report compares the wall time per tick with that.

ref-add measures the cost of a Ref's base add in the module (wasm32 Ref is an offset added to g_blockBase). The module
has few sites that load g_blockBase (printed from the built module); a variant of the built module counts how often they
run on the stubbed run of `wasm_modules.py statics`; a small wasm loop run under the pinned wasmtime times a
dependent load with and without the base load and add, which gives the cost of one base add. Cost per tick is count times
cost, and is compared with the module's time per tick from run.
"""

import argparse
import os
import pathlib
import re
import statistics
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import wasm_fuel
import wasm_modules

ROOT = wasm_modules.ROOT
GUARD = "WASM_BENCH_UNDER_WASMTIME"
DEFAULT_SCENARIO = "long-greenhaven-8p"
TICKS_PER_SECOND_10X = 300


def parse_report(path):
    lines = open(path).read().splitlines()
    if not lines or lines[0] != "bt-moduletime 1":
        sys.exit(f"{path}: not a bt-moduletime 1 file")
    report = {"slots": {}, "imports": {}, "empty": (0, 0), "warm": (0, 0)}
    for line in lines[1:]:
        key, *rest = line.split("\t")
        if key in ("mode",):
            report[key] = rest[0]
        elif key in ("ticks", "wall_counter", "wall_ms", "timer_cost"):
            report[key] = int(rest[0])
        elif key == "slot":
            report["slots"][rest[0]] = tuple(int(v) for v in rest[1:])
        elif key == "import":
            report["imports"][rest[0]] = tuple(int(v) for v in rest[1:])
        elif key in ("empty", "warm"):
            report[key] = tuple(int(v) for v in rest)
    return report


def play(exe, scenario, game_dir, native):
    import tracediff
    marker = os.path.join(tempfile.mkdtemp(prefix="moduletime-", dir=os.path.join(ROOT, "build", "scratch")), "moduletime.on")
    open(marker, "w").close()
    env = dict(os.environ, BT_NATIVE_MODULES="1" if native else "0")
    run = tracediff.play(exe, scenario, game_dir, None, 3600, put={"moduletime.on": marker}, collect=("moduletime.txt",), env=env)
    return parse_report(str(run / "moduletime.txt"))


def describe(report):
    """Print one run's calls and time per tick; returns (slot time per tick in us, wall ms per tick, module us per tick or None)."""
    ticks = report["ticks"]
    per_second = report["wall_counter"] * 1000.0 / report["wall_ms"]

    def us(counter):
        return counter * 1e6 / per_second

    r = report["timer_cost"]
    slots = report["slots"]
    slot_calls = sum(v[0] for v in slots.values())
    print(f"  mode {report['mode']}: {ticks} ticks, counter {per_second:,.0f} per second, timer read {us(r) * 1000:.0f} ns,"
          f" wall {report['wall_ms'] / ticks:.3f} ms per tick")
    if report["mode"] == "native":
        slot_time = sum(v[1] for v in slots.values()) - slot_calls * r
        print(f"  slot calls per tick {slot_calls / ticks:.3f}; slot time per tick {us(slot_time) / ticks:.3f} us")
        for name, values in sorted(slots.items()):
            print(f"    {name}: {values[0] / ticks:.3f} calls per tick, {us(values[1] - values[0] * r) / values[0]:.3f} us per call")
        return us(slot_time) / ticks, report["wall_ms"] / ticks, None
    imports = report["imports"]
    import_calls = sum(v[0] for v in imports.values())
    cold_calls, cold_time = report["empty"]
    warm_calls, warm_time = report["warm"]
    cold = (cold_time - cold_calls * r) / cold_calls if cold_calls else 0
    warm = (warm_time - warm_calls * r) / warm_calls if warm_calls else 0
    imports_true = sum(v[1] for v in imports.values()) - import_calls * r
    call_true = sum(v[2] for v in slots.values()) - slot_calls * r - 2 * import_calls * r
    slot_true = sum(v[1] for v in slots.values()) - 3 * slot_calls * r - 2 * cold_calls * r
    module = call_true - imports_true - slot_calls * warm
    glue = slot_true - module - imports_true
    print(f"  slot calls per tick {slot_calls / ticks:.3f}; import calls per tick {import_calls / ticks:.3f}")
    for name, values in sorted(slots.items()):
        print(f"    {name}: {values[0] / ticks:.3f} calls per tick, {us(values[1] - values[0] * r * 3) / values[0]:.3f} us per call"
              f" (wasmtime_func_call {us(values[2] - values[0] * r) / values[0]:.3f} us)")
    for name, values in sorted(imports.items()):
        print(f"    import {name}: {values[0] / ticks:.3f} calls per tick, {us(values[1] - values[0] * r) / values[0]:.3f} us per call")
    print(f"  an empty export call: {us(warm) * 1000:.0f} ns just after a slot call, {us(cold) * 1000:.0f} ns at the first call of a tick")
    print(f"  per tick: module {us(module) / ticks:.3f} us, imports {us(imports_true) / ticks:.3f} us,"
          f" glue {us(glue) / ticks:.3f} us (of which {us(slot_calls * warm) / ticks:.3f} us entering and leaving wasmtime),"
          f" slots in all {us(slot_true) / ticks:.3f} us")
    return us(slot_true) / ticks, report["wall_ms"] / ticks, us(module) / ticks


def run_command(args):
    scenario = pathlib.Path(ROOT) / "reference" / "scenarios" / f"{args.scenario}.txt"
    exe = os.path.abspath(args.exe)
    results = {}
    for native in (False, True):
        t0 = time.time()
        report = play(exe, scenario, os.path.expanduser(args.game_dir) if args.game_dir else None, native)
        print(f"{args.scenario}, {'native' if native else 'WASM'} modules ({time.time() - t0:.0f} s of wall time):")
        results[native] = describe(report)
    wasm, native = results[False][0], results[True][0]
    print(f"slots per tick: WASM {wasm:.3f} us, native {native:.3f} us, {wasm / native:.1f}x")
    budget_ms = 1000.0 / TICKS_PER_SECOND_10X
    print(f"wall per tick: WASM {results[False][1]:.3f} ms, native {results[True][1]:.3f} ms; 10x game speed allows {budget_ms:.2f} ms")
    return 0


def base_sites(wat):
    """The address of g_blockBase and the number of places the module loads it."""
    index = re.search(r'\(export "g_blockBase" \(global (\d+)\)', wat).group(1)
    address = re.search(rf"\(global \(;{index};\) i32 i32\.const (\d+)\)", wat).group(1)
    return address, len(re.findall(rf"i32\.const 0\n\s+i32\.load offset={address}\n", wat))


def variant(wasm, wasm_tools, replacement, extra=()):
    """The module with each load of g_blockBase replaced by `replacement` lines (extra fields added before the module's end)."""
    wat = wasm_fuel.tool(wasm_tools, ["print", "-"], wasm).decode()
    address, _ = base_sites(wat)
    pattern = re.compile(rf"( *)i32\.const 0\n\s+i32\.load offset={address}\n")
    wat = pattern.sub(lambda m: "".join(f"{m.group(1)}{line}\n" for line in replacement), wat)
    close = wat.rindex("\n)")
    wat = wat[:close] + "\n" + "\n".join(extra) + wat[close:]
    return wasm_fuel.tool(wasm_tools, ["parse", "-"], wat.encode())


def time_loop(wasmtime, source, iterations):
    """The median seconds of seven runs of the module's `run` export over `iterations`."""
    engine = wasmtime.Engine()
    store = wasmtime.Store(engine)
    instance = wasmtime.Instance(store, wasmtime.Module(engine, source), [])
    run = instance.exports(store)["run"]
    memory = instance.exports(store)["m"]
    for node in range(1024):
        memory.write(store, ((node * 7 + 1) % 1024 * 16).to_bytes(4, "little"), 4096 + node * 16)
    run(store, 1000)
    samples = []
    for _ in range(7):
        t0 = time.perf_counter()
        run(store, iterations)
        samples.append(time.perf_counter() - t0)
    return statistics.median(samples)


def per_add_cost(wasmtime):
    """(ns per base add with independent loads, ns per base add on a chain of dependent Ref loads, the seconds measured)."""
    iterations, derefs = 20_000_000, 8
    flat = "".join(f"{{load}} local.get $i i32.const {i * 4} i32.add i32.const 4092 i32.and i32.add i32.load local.get $acc i32.add local.set $acc\n"
                   for i in range(derefs))
    flat_source = """(module (memory (export "m") 1)
      (func (export "run") (param $n i32) (result i32) (local $i i32) (local $acc i32)
        loop $l
          %s
          local.get $i i32.const 1 i32.add local.tee $i local.get $n i32.lt_s br_if $l
        end
        local.get $acc))"""
    chain = "".join("{load} local.get $p i32.add i32.load offset=4096 local.set $p\n" for _ in range(derefs))
    chain_source = """(module (memory (export "m") 1)
      (func (export "run") (param $n i32) (result i32) (local $i i32) (local $p i32)
        loop $l
          %s
          local.get $i i32.const 1 i32.add local.tee $i local.get $n i32.lt_s br_if $l
        end
        local.get $p))"""
    times = {}
    for kind, source, body in (("flat", flat_source, flat), ("chain", chain_source, chain)):
        for name, load in (("with", "i32.const 0 i32.load offset=8"), ("without", "i32.const 0")):
            times[kind, name] = time_loop(wasmtime, source % body.replace("{load}", load), iterations)
    cost = {kind: (times[kind, "with"] - times[kind, "without"]) / (iterations * derefs) for kind in ("flat", "chain")}
    return cost["flat"], cost["chain"], times


def ref_add_command(_args):
    import wasmtime
    game = wasm_modules.game_manifest()
    wasm_tools = wasm_modules.find_wasm_tools(game["tools"]["wasm-tools"])
    path = os.path.join(wasm_modules.OUT, game["modules"][0]["name"] + ".wasm")
    wasm = open(path, "rb").read()
    wat = wasm_fuel.tool(wasm_tools, ["print", "-"], wasm).decode()
    address, sites = base_sites(wat)
    print(f"g_blockBase at {address}: loaded at {sites} places in the module")
    counted = variant(wasm, wasm_tools,
                      ["i32.const 0", f"i32.load offset={address}", "global.get $__base_count", "i64.const 1", "i64.add", "global.set $__base_count"],
                      ['(global $__base_count (mut i64) i64.const 0)', '(export "__base_count" (global $__base_count))'])
    removed = variant(wasm, wasm_tools, ["i32.const 0"])
    dirs = tempfile.mkdtemp(prefix="ref-add-", dir=os.path.join(ROOT, "build", "scratch"))
    paths = {}
    for name, data in (("counted", counted), ("removed", removed)):
        paths[name] = os.path.join(dirs, name + ".wasm")
        with open(paths[name], "wb") as f:
            f.write(data)
    plain, count_report = {}, {}
    wasm_modules.statics_run(path, plain)
    wasm_modules.statics_run(paths["counted"], count_report)
    wasm_modules.statics_run(paths["removed"], {})
    store, exports = count_report["store"], count_report["exports"]
    total = exports["__base_count"].value(store)
    ticks = wasm_modules.STATICS_TICKS
    print(f"stubbed run: {total} base loads in {ticks} ticks, {total / ticks:.3f} per tick (SpawnProjectile calls {plain['spawns']} / {count_report['spawns']})")
    flat, chain, times = per_add_cost(wasmtime)
    print(f"one base load and add: {flat * 1e9:.3f} ns with independent loads ({times['flat', 'with']:.3f} s with, {times['flat', 'without']:.3f} s without),"
          f" {chain * 1e9:.3f} ns on a chain of dependent loads ({times['chain', 'with']:.3f} s with, {times['chain', 'without']:.3f} s without)")
    print(f"base adds on the stubbed run cost at most {total / ticks * chain * 1e9:.4f} ns per tick")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    run = sub.add_parser("run")
    run.add_argument("--exe", required=True)
    run.add_argument("--game-dir")
    run.add_argument("--scenario", default=DEFAULT_SCENARIO)
    sub.add_parser("ref-add")
    args = parser.parse_args()
    if args.command == "run":
        return run_command(args)
    try:
        import wasmtime
    except ImportError:
        if os.environ.get(GUARD):
            raise
        pin = wasm_modules.manifest()["tools"]["wasmtime"]
        cmd = ["uv", "run", "--no-project", "--with", f"wasmtime=={pin}", "python", "-I", os.path.abspath(__file__), "ref-add"]
        return subprocess.run(cmd, env=dict(os.environ, **{GUARD: "1"})).returncode
    return ref_add_command(args)


if __name__ == "__main__":
    sys.exit(main())
