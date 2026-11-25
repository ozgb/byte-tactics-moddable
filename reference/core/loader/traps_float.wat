(module
  (import "env" "memory" (memory 1))
  (import "env" "__indirect_function_table" (table 1 funcref))
  (func (export "trap_nan") (result i32) (i32.trunc_f64_s (f64.const nan)))
  (func (export "trap_big") (result i32) (i32.trunc_f64_s (f64.const 1e10)))
  (func (export "trap_neg") (param i32) (result i32) (i32.trunc_f64_u (f64.convert_i32_s (local.get 0))))
  (func (export "ok") (result i32) (i32.const 5)))
