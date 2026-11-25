(module
  (import "env" "memory" (memory 1))
  (import "env" "__indirect_function_table" (table 1 funcref))
  (func $boot unreachable)
  (start $boot)
  (func (export "add") (param i32 i32) (result i32) (i32.add (local.get 0) (local.get 1))))
