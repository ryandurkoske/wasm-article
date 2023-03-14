#pragma once

#define CHUNK_SIZE 8
#define RNG_RANGE 10

#include <stdint.h>
#include "rng.h"
#include "impexp.h"

int32_t generate_chunk(int32_t* chunk, rng_t*); 

EXPORT void compute_sum(int32_t* chunk, int32_t* res);
