
(import "env" "memory" (memory 2 65536 shared))
(func $mtx_lock (param $mutexAddr i32)

	(atomic.fence)	;; Sanity check: instruction not removed until 
	;; wasm-opt pass. Although--with a basic lock--binaryen would 
	;; likely never shuffle atomic instructions over other instructions,
	;; so these simple function bodies are redundant to protect. Compiler 
	;; fences are likely redundant, I leave them here for best
	;; practice in case wasm linker LTO gets funny ideas.

	(block $done
		(loop $retry
			(i32.atomic.rmw.cmpxchg 	;; returns original value
				(local.get $mutexAddr) 
				(i32.const 1)	;; expected value
				(i32.const 0))	;; replacement value
			(br_if $done)
			(memory.atomic.wait32	;; 0 => awoken by notify, 1 => not awoken
				(local.get $mutexAddr) 
				(i32.const 0) 	;; expected value
				(i64.const -1))	;; infinite time out
			(drop)
			(br $retry)
		)
	)
	(atomic.fence)
)

(func $mtx_unlock (param $mutexAddr i32)
	(atomic.fence)
	(i32.atomic.store (local.get $mutexAddr) (i32.const 1)) ;; 1 => unlocked
	(drop 
		(memory.atomic.notify
			(local.get $mutexAddr)  ;; mutex address
			(i32.const 1)))			;; notify 1 waiter
	(atomic.fence)
)

(func $spin_lock (param $mutexAddr i32)

	(atomic.fence)	;; Sanity check: instruction not removed until 
	;; wasm-opt pass. Although--with a basic lock--binaryen would 
	;; likely never shuffle atomic instructions over other instructions,
	;; so these simple function bodies are redundant to protect. Compiler 
	;; fences are likely redundant, I leave them here for best
	;; practice in case wasm linker LTO gets funny ideas.

	(block $done
		(loop $retry
			(i32.atomic.rmw.cmpxchg 	;; returns original value
				(local.get $mutexAddr) 
				(i32.const 1)	;; expected value
				(i32.const 0))	;; replacement value
			(br_if $done)
			(br $retry)
		)
	)
	(atomic.fence)
)

(func $spin_unlock (param $mutexAddr i32)
	(atomic.fence)
	(i32.atomic.store (local.get $mutexAddr) (i32.const 1)) ;; 1 => unlocked
	(drop 
		(memory.atomic.notify
			(local.get $mutexAddr)  ;; mutex address
			(i32.const 1)))			;; notify 1 waiter
	(atomic.fence)
)