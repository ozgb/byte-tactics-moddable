(module
  (import "env" "memory" (memory 1))
  (import "env" "__indirect_function_table" (table 1 funcref))
  (func (export "add") (result i32)))
