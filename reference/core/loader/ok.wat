(module
  (import "env" "memory" (memory 1))
  (import "env" "__indirect_function_table" (table 1 funcref))
  (global (export "g_word") i32 (i32.const 4112))
  (data (i32.const 4112) "\07\00\00\00")
  (func (export "add") (param i32 i32) (result i32) (i32.add (local.get 0) (local.get 1)))
  (func (export "store") (param i32 i32) (i32.store (local.get 0) (local.get 1)))
  (func (export "load") (param i32) (result i32) (i32.load (local.get 0)))
  (func (export "noop")))
