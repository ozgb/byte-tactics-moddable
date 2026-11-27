(module
  (import "env" "memory" (memory 1))
  (import "env" "__indirect_function_table" (table 1 funcref))
  (global (export "g_word") i32 (i32.const 4112))
  (func (export "noop"))
  (func (export "__fuel_set") (param i64) unreachable)
  (func (export "__fuel_get") (result i64) (i64.const 9223372036854775807)))
