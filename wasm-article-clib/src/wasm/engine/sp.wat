
(import "env" "__stack_pointer" (global $__stack_pointer (mut i32)))

(func $sp_set (export "sp_set") (param $sp_addr i32)
	local.get 0
	global.set $__stack_pointer
)