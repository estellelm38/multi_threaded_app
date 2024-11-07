#include "fastrand.h"

static __thread uint32_t __fast_random_next_z = 2;
static __thread uint32_t __fast_random_next_w = 2;


// A simple pseudo-random 32-bit number generator implementing the multiply-with-carry method 
// invented by George Marsaglia. It is computationally fast and has good properties.
// http://en.wikipedia.org/wiki/Random_number_generation#Computational_methods
inline uint32_t fastRandom32(void) {
    __fast_random_next_z = 36969 * (__fast_random_next_z & 65535) + (__fast_random_next_z >> 16);
    __fast_random_next_w = 18000 * (__fast_random_next_w & 65535) + (__fast_random_next_w >> 16);
    return (__fast_random_next_z << 16) + __fast_random_next_w;  /* 32-bit result */
}

inline void fastRandomSetSeed(uint32_t seed) {
    __fast_random_next_z = seed;
    __fast_random_next_w = seed/2;

    if (__fast_random_next_z == 0 || __fast_random_next_z == 0x9068ffff)
        __fast_random_next_z++;
    if (__fast_random_next_w == 0 || __fast_random_next_w == 0x464fffff)
        __fast_random_next_w++;
}

