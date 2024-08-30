#pragma once

#define CHUNK_SIZE 16384
#define RNG_RANGE 32

#include <stdint.h>
#include "rng.h"

int32_t generate_chunk(int32_t* chunk, rng_t*); 

EXPORT("compute_sum") 
void compute_sum(int32_t* chunk, int32_t* res);

