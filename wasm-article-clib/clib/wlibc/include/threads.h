/**

Arguably doesnt belong in wlibc, but we rly need it for wasm.

Will require polyfills for all native implementations now

*/

#pragma once

#include <stdatomic.h>

typedef atomic_int_least32_t mtx_t; 

#define MUTEX_UNLOCKED 1

//wasm mutex implementation is a futex :)
void mtx_lock(mtx_t* mut_ptr);
void mtx_unlock(mtx_t* mut_ptr);

//for main thread
void spin_lock(mtx_t* mut_ptr);
void spin_unlock(mtx_t* mut_ptr);