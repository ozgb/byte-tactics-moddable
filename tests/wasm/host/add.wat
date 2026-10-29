(module
  (import "env" "memory" (memory 1))
  (import "env" "__indirect_function_table" (table 8 funcref))
  (func $add (export "add") (param i32 i32) (result i32)
    (i32.store (i32.const 16) (i32.add (local.get 0) (local.get 1)))
    (i32.add (local.get 0) (local.get 1)))
  (func $mix (export "mix") (param $n i32) (result i32)
    (local $i i32) (local $a i32)
    (local.set $a (local.get $n))
    (block $done
      (loop $next
        (br_if $done (i32.ge_u (local.get $i) (i32.const 32)))
        (local.set $a
          (i32.xor
            (i32.add (i32.mul (local.get $a) (i32.const 31)) (local.get $n))
            (i32.shr_u (local.get $a) (i32.const 3))))
        (local.set $a (i32.rotl (local.get $a) (i32.const 5)))
        (local.set $i (i32.add (local.get $i) (i32.const 1)))
        (br $next)))
    (local.get $a))
  (elem (i32.const 0) $add $mix))
