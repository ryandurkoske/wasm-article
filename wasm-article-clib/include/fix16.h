
#include "stdint.h"

#define FIX16_ONE 65536
#define FIX16_HALF 32768
#define FIX16_ROOT2 92681

typedef int32_t fix16_t;

inline fix16_t mul(fix16_t a, fix16_t b){
	return (a*b)>>16;
}

inline fix16_t div(fix16_t n, fix16_t d){
	return (n/d)<<16 | (n<<16)/d;
}

inline fix16_t normalize(fix16_t x, fix16_t y){
	fix16_t guess = 
}