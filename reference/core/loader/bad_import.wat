(module
  (import "env" "memory" (memory 1))
  (import "env" "__indirect_function_table" (table 1 funcref))
  (import "env" "missing" (func (result i32)))
  (func (export "add") (result i32) (i32.const 1)))
