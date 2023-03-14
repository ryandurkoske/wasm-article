#include <stdint.h>
#include "rng.h"

#include "running_total.h"

int32_t generate_chunk(int32_t* chunk, rng_t* rng){
	int32_t sum = 0;
	for(int i = 0; i < CHUNK_SIZE; i++){
		int32_t next = rng_next(rng) % (2*RNG_RANGE+1) - RNG_RANGE;
		chunk[i] = next;
		sum += next;
	}
	return sum;
}

void compute_sum(int32_t* chunk, int32_t* res){
	int32_t sum = 0;
	for(int i = 0; i < CHUNK_SIZE; i++){
		sum += chunk[i];
	}
	(*res) += sum;
}
