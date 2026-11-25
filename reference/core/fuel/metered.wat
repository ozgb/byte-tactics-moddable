(module
  (import "env" "memory" (memory 1))
  (import "env" "__indirect_function_table" (table 1 funcref))
  (func (export "spin") (param $n i32) (result i32)
    (local $i i32)
    (loop $again
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br_if $again (i32.lt_u (local.get $i) (local.get $n))))
    (local.get $i))
  (func (export "noop"))
  (func (export "trap") unreachable))
