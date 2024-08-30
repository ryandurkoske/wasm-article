#include "rng.h"

#include <stdlib.h>
#include <stdint.h>

#define XOR_Y 401002
#define XOR_Z 6002007
#define XOR_W 2006003009

rng_t* rng_alloc(uint32_t seed){
	rng_t* rng = malloc(sizeof(rng_t));
	rng_set(rng, seed);
	return rng;
}
void rng_free(rng_t* rng){
	free(rng);
}


void rng_set(rng_t* rng, uint32_t seed){
	rng->x = seed;
	rng->y = XOR_Y;
	rng->z = XOR_Z;
	rng->w = XOR_W;
}

uint32_t rng_next(rng_t* rng){
	uint32_t t = (rng->x ^ (rng->x << 11));
	rng->x = rng->y;
	rng->y = rng->z;
	rng->z = rng->w;
	rng->w = (rng->w ^ (rng->w >> 19)) ^ (t ^ (t >> 8));
	return rng->w;
}
