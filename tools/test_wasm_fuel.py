"""Tests for the fuel metering pass of wasm_fuel.py.

    uv run tools/test_wasm_fuel.py

Runs under the pinned wasmtime package, which it fetches with uv, and the pinned wasm-tools of build/tools/bin.
"""

import json
import os
import re
import subprocess
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import wasm_fuel as wf

GUARD = "TEST_WASM_FUEL_UNDER_WASMTIME"

SOURCE = """
(module
  (memory (export "mem") 1)
  (table 3 funcref)
  (global $seed (mut i32) (i32.const 7))
  (global (export "answer") i32 (i32.const 42))
  (func $leaf (export "leaf") (param i32) (result i32)
    local.get 0
    i32.const 1
    i32.add)
  (func $count (export "count") (param $n i32) (result i32)
    (local $i i32)
    (local $acc i32)
    loop $again
      local.get $acc
      local.get $i
      call $leaf
      i32.add
      local.set $acc
      local.get $i
      i32.const 1
      i32.add
      local.tee $i
      local.get $n
      i32.lt_s
      br_if $again
    end
    local.get $acc)
  (func $nested (export "nested") (param $n i32) (param $m i32) (result i32)
    (local $i i32)
    (local $j i32)
    (local $sum i32)
    loop $outer
      i32.const 0
      local.set $j
      loop $inner
        local.get $sum
        i32.const 1
        i32.add
        local.set $sum
        local.get $j
        i32.const 1
        i32.add
        local.tee $j
        local.get $m
        i32.lt_s
        br_if $inner
      end
      local.get $i
      i32.const 1
      i32.add
      local.tee $i
      local.get $n
      i32.lt_s
      br_if $outer
    end
    local.get $sum)
  (func $fact (export "fact") (param $k i32) (result i32)
    local.get $k
    i32.eqz
    if (result i32)
      i32.const 1
    else
      local.get $k
      local.get $k
      i32.const 1
      i32.sub
      call $fact
      i32.mul
    end)
  (func $started
    (local $i i32)
    loop $spin
      local.get $i
      i32.const 1
      i32.add
      local.tee $i
      i32.const 100
      i32.lt_s
      br_if $spin
    end
    i32.const 5
    global.set $seed)
  (start $started)
  (data (i32.const 16) "abc")
  (elem (i32.const 0) func $leaf $count $fact))
"""

EXPORTS_COUNTER = """
(module
  (global $c (export "c") (mut i64) (i64.const 0))
  (func $f global.get $c i64.const 1 i64.sub global.set $c)
  (func (export "__fuel_set") (param i64))
  (func (export "__fuel_get") (result i64) i64.const 0))
"""

IMPORTS_COUNTER = """
(module
  (import "env" "c" (global $c (mut i64)))
  (func $f global.get $c i64.const 1 i64.sub global.set $c)
  (func (export "__fuel_set") (param i64))
  (func (export "__fuel_get") (result i64) i64.const 0))
"""

UNMETERED_WITH_HAND_WRITTEN_EXPORTS = """
(module
  (global $c (mut i64) (i64.const 9223372036854775807))
  (func $f (export "run") (result i32) i32.const 1)
  (func (export "__fuel_set") (param i64) local.get 0 global.set $c)
  (func (export "__fuel_get") (result i64) global.get $c))
"""

METER_BLOCK = (r"[ ]+" + r"\n[ ]+".join(re.escape(line).replace(re.escape(wf.COUNTER), r"\S+").replace("if", r"if(?: ;; label = @\d+)?")
                                      for line in wf.METER) + r"\n")

LOOP_WITH_PARAM = """
(module
  (func (export "run") (result i32)
    (local $x i32)
    i32.const 3
    loop $l (param i32) (result i32)
      i32.const 1
      i32.sub
      local.tee $x
      local.get $x
      br_if $l
    end))
"""


def wasm_tools():
    return wf.find_wasm_tools()


def assemble(text):
    return wf.tool(wasm_tools(), ["parse", "-"], text.encode())


def disassemble(wasm):
    return wf.tool(wasm_tools(), ["print", "-"], wasm).decode()


def function_signatures(text):
    """[(index, signature text without the type number)] of the defined and imported functions, by index."""
    found = {}
    for index, rest in re.findall(r"\(func (?:\$\S+ )?\(;(\d+);\) \(type \d+\)([^\n]*)", text):
        found[int(index)] = rest.strip()
    for index, rest in re.findall(r'\(import "[^"]*" "[^"]*" \(func (?:\$\S+ )?\(;(\d+);\) \(type \d+\)([^\n]*)', text):
        found[int(index)] = rest.strip().rstrip(")")
    return sorted(found.items())


class Meter(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import wasmtime
        cls.wasmtime = wasmtime
        cls.original = assemble(SOURCE)
        cls.metered = wf.instrument(cls.original, wasm_tools())

    def instance(self, wasm):
        wt = self.wasmtime
        store = wt.Store()
        module = wt.Module(store.engine, wasm)
        instance = wt.Instance(store, module, [])
        return store, instance.exports(store)

    def burn(self, name, *args, budget=1000):
        store, exports = self.instance(self.metered)
        exports["__fuel_set"](store, budget)
        result = exports[name](store, *args)
        return result, budget - exports["__fuel_get"](store)

    def test_the_counter_starts_at_the_largest_i64(self):
        store, exports = self.instance(self.metered)
        self.assertEqual(exports["__fuel_get"](store), (1 << 63) - 1 - 100 - 1)

    def test_the_start_function_runs_unmetered_to_the_end(self):
        store, exports = self.instance(self.metered)
        self.assertGreater(exports["__fuel_get"](store), 0)

    def test_results_are_unchanged(self):
        store, plain = self.instance(self.original)
        for name, args in (("leaf", (4,)), ("count", (6,)), ("nested", (3, 4)), ("fact", (7,))):
            result, _ = self.burn(name, *args)
            self.assertEqual(result, plain[name](store, *args), name)

    def test_a_call_burns_one(self):
        self.assertEqual(self.burn("leaf", 4)[1], 1)

    def test_a_loop_burns_one_per_iteration_and_one_per_call(self):
        for n in (1, 2, 9):
            self.assertEqual(self.burn("count", n)[1], 1 + n + n, n)

    def test_nested_loops(self):
        for n, m in ((1, 1), (2, 3), (4, 5)):
            self.assertEqual(self.burn("nested", n, m)[1], 1 + n + n * m, (n, m))

    def test_recursion_burns_one_per_call(self):
        for k in (0, 1, 5):
            self.assertEqual(self.burn("fact", k)[1], k + 1, k)

    def test_it_traps_below_zero_and_not_at_zero(self):
        store, exports = self.instance(self.metered)
        exports["__fuel_set"](store, 11)
        exports["count"](store, 5)
        self.assertEqual(exports["__fuel_get"](store), 0)
        exports["__fuel_set"](store, 10)
        with self.assertRaises(self.wasmtime.Trap):
            exports["count"](store, 5)
        self.assertEqual(exports["__fuel_get"](store), -1)

    def test_a_deep_recursion_runs_out(self):
        store, exports = self.instance(self.metered)
        exports["__fuel_set"](store, 50)
        with self.assertRaises(self.wasmtime.Trap):
            exports["fact"](store, 1000)
        self.assertLess(exports["__fuel_get"](store), 0)

    def test_set_and_get_round_trip(self):
        store, exports = self.instance(self.metered)
        for value in (0, 1, 123456789012, -5):
            exports["__fuel_set"](store, value)
            self.assertEqual(exports["__fuel_get"](store), value)

    def test_a_loop_with_a_parameter_validates_and_burns_per_iteration(self):
        wasm = wf.instrument(assemble(LOOP_WITH_PARAM), wasm_tools())
        store = self.wasmtime.Store()
        exports = self.wasmtime.Instance(store, self.wasmtime.Module(store.engine, wasm), []).exports(store)
        exports["__fuel_set"](store, 100)
        self.assertEqual(exports["run"](store), 0)
        self.assertEqual(100 - exports["__fuel_get"](store), 1 + 3)

    def test_the_inserted_code(self):
        text = disassemble(self.metered)
        functions = len(re.findall(r"^  \(func ", disassemble(self.original), re.M))
        loops = len(re.findall(r"^\s+loop\b", text, re.M))
        self.assertEqual(loops, 4)
        self.assertEqual(text.count("i64.sub"), functions + loops)
        counter = re.findall(r"^  \(global (?:\$\S+ )?\(;(\d+);\) \(mut i64\)", text, re.M)[0]
        label = rf"(?:{re.escape(wf.COUNTER)}|{counter})"
        meter = r"\s+".join(re.escape(c).replace(re.escape(wf.COUNTER), label).replace("if", r"if(?: ;; label = @\d+)?") for c in wf.METER)
        self.assertEqual(len(re.findall(meter, text)), functions + loops)
        self.assertIn("(mut i64)", text)

    def test_no_index_moves(self):
        before, after = disassemble(self.original), disassemble(self.metered)
        defined = function_signatures(before)
        self.assertEqual(function_signatures(after)[:len(defined)], defined)
        self.assertEqual([name for name, _ in function_signatures(after)[len(defined):]], list(range(len(defined), len(defined) + 2)))
        for kind in ("elem", "data", "table", "memory", "start"):
            wanted = re.findall(rf"^  \({kind}[ (][^\n]*", before, re.M)
            got = re.findall(rf"^  \({kind}[ (][^\n]*", after, re.M)
            self.assertEqual([re.sub(r"\$\S+ ", "", w) for w in wanted], [re.sub(r"\$\S+ ", "", g) for g in got], kind)
        exports_before = re.findall(r'\(export "([^"]*)" \((func|global) (\S+)\)', before)
        exports_after = re.findall(r'\(export "([^"]*)" \((func|global) (\S+)\)', after)
        self.assertEqual(exports_after[:len(exports_before)], exports_before)
        self.assertEqual([name for name, _, _ in exports_after[len(exports_before):]], list(wf.EXPORTS))
        self.assertEqual(len(re.findall(r"^  \(global ", after, re.M)), len(re.findall(r"^  \(global ", before, re.M)) + 1)

    def test_the_new_global_is_last(self):
        text = disassemble(self.metered)
        globals_ = re.findall(r"^  \(global ([^\n]*)", text, re.M)
        self.assertIn("(mut i64)", globals_[-1])
        self.assertIn("(;2;)", re.findall(r"^  \(global (\S+ )?(\(;\d+;\))", text, re.M)[-1][1])

    def test_a_second_run_is_refused(self):
        with self.assertRaises(wf.FuelError):
            wf.instrument(self.metered, wasm_tools())

    def test_the_counter_is_neither_imported_nor_exported(self):
        self.assertEqual(wf.check_unmetered_surface(self.metered, wasm_tools()), [])
        problems = wf.check_unmetered_surface(assemble(EXPORTS_COUNTER), wasm_tools())
        self.assertTrue(any("exports the counter" in p for p in problems), problems)
        problems = wf.check_unmetered_surface(assemble(IMPORTS_COUNTER), wasm_tools())
        self.assertTrue(any("imports the global" in p for p in problems), problems)
        self.assertTrue(wf.check_unmetered_surface(self.original, wasm_tools()))

    def edited(self, edit):
        return assemble(edit(disassemble(self.metered)))

    def problems(self, wasm):
        return wf.verify(wasm, wasm_tools())

    def test_verify_passes_what_the_pass_metered(self):
        self.assertEqual(self.problems(self.metered), [])
        self.assertEqual(self.problems(wf.instrument(assemble(LOOP_WITH_PARAM), wasm_tools())), [])

    def test_verify_fails_an_unmetered_module(self):
        self.assertTrue(self.problems(self.original))

    def test_verify_fails_hand_written_exports_on_an_unmetered_module(self):
        wasm = assemble(UNMETERED_WITH_HAND_WRITTEN_EXPORTS)
        self.assertEqual(wf.check_unmetered_surface(wasm, wasm_tools()), ["has no metering code"])
        self.assertTrue(any("metering code" in p for p in self.problems(wasm)), self.problems(wasm))

    def test_verify_fails_a_function_without_the_decrement(self):
        wasm = self.edited(lambda text: re.sub(METER_BLOCK, "", text, count=1))
        self.assertTrue(any("function" in p and "metering code" in p for p in self.problems(wasm)), self.problems(wasm))

    def test_verify_fails_a_loop_without_the_decrement(self):
        wasm = self.edited(lambda text: re.sub(r"(\n[ ]+loop[^\n]*\n)" + METER_BLOCK, r"\1", text, count=1))
        self.assertTrue(any("loop" in p and "metering code" in p for p in self.problems(wasm)), self.problems(wasm))

    def test_verify_fails_a_changed_decrement(self):
        wasm = self.edited(lambda text: text.replace("i64.const 1\n", "i64.const 0\n", 1))
        self.assertTrue(self.problems(wasm))

    def test_verify_fails_a_second_write_to_the_counter(self):
        def edit(text):
            write = "    i64.const 99\n    global.set " + re.search(r"global\.set (\S+)", text).group(1) + "\n"
            return re.sub(r"(\(func [^\n]*\(param i32\) \(result i32\)\n(?:.*\n){10})", lambda m: m.group(1) + write, text, count=1)
        wasm = self.edited(edit)
        self.assertTrue(any("writes the counter" in p for p in self.problems(wasm)), self.problems(wasm))

    def test_verify_fails_an_exported_counter(self):
        def edit(text):
            counter = re.search(r"global\.set (\S+)", text).group(1)
            return text.replace('  (export "__fuel_set"', f'  (export "c" (global {counter}))\n  (export "__fuel_set"', 1)
        self.assertTrue(any("exports the counter" in p for p in self.problems(self.edited(edit))))

    def test_verify_fails_an_imported_counter(self):
        wasm = assemble(IMPORTS_COUNTER)
        self.assertTrue(self.problems(wasm))

    def test_verify_fails_a_set_that_does_more(self):
        def edit(text):
            return re.sub(r"(local\.get 0\n\s+global\.set \S+)", r"\1\n    local.get 0\n    global.set 0", text, count=1)
        self.assertTrue(any("__fuel_set" in p for p in self.problems(self.edited(edit))))

    def test_verify_main_exits_nonzero_on_a_refusal(self):
        import tempfile
        with tempfile.TemporaryDirectory() as d:
            for name, wasm, code in (("good.wasm", self.metered, 0), ("bad.wasm", self.original, 1)):
                path = os.path.join(d, name)
                with open(path, "wb") as f:
                    f.write(wasm)
                run = subprocess.run([sys.executable, os.path.join(HERE, "wasm_fuel.py"), path, "--verify"], capture_output=True, text=True)
                self.assertEqual(run.returncode, code, run.stdout)

    def test_the_manifest_records_the_rule(self):
        with open(os.path.join(wf.ROOT, "wasm", "modules.json")) as f:
            fuel = json.load(f)["fuel"]
        self.assertEqual(fuel, {"rule": wf.RULE, "version": wf.VERSION})


def main():
    try:
        import wasmtime
    except ImportError:
        if os.environ.get(GUARD):
            raise
        with open(os.path.join(os.path.dirname(HERE), "tests", "wasm", "modules", "modules.json")) as f:
            pin = json.load(f)["tools"]["wasmtime"]
        cmd = ["uv", "run", "--no-project", "--with", f"wasmtime=={pin}", "python", "-I", os.path.abspath(__file__), *sys.argv[1:]]
        return subprocess.run(cmd, env=dict(os.environ, **{GUARD: "1"}), cwd=HERE).returncode
    unittest.main(argv=sys.argv[:1] + sys.argv[1:])
    return 0


if __name__ == "__main__":
    sys.exit(main())
