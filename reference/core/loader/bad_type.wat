(module
  (import "env" "memory" (func))
  (import "env" "__indirect_function_table" (table 1 funcref))
  (func (export "add") (result i32) (i32.const 1)))
