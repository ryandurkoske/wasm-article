#pragma once

#include <stdint.h>

typedef struct rng_t {
	uint32_t x;
	uint32_t y;
	uint32_t z;
	uint32_t w;
} rng_t;

rng_t* rng_alloc(uint32_t seed);
void rng_set(rng_t*, uint32_t seed);
void rng_free(rng_t*);

uint32_t rng_next(rng_t*);