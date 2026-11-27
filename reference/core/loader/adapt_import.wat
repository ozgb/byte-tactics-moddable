(module
  (import "env" "memory" (memory 1))
  (import "env" "__indirect_function_table" (table 1 funcref))
  (import "fixture" "mix" (func $mix (param i32 i32) (result i32)))
  (global (export "g_word") i32 (i32.const 4112))
  (func (export "m") (param i32 i32) (result i32)
    (call $mix (local.get 0) (local.get 1))))
