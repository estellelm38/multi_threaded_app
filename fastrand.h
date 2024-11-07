#ifndef _FASTRAND_H_
#define _FASTRAND_H_

#include <math.h>
#include <stdint.h>
#include <limits.h>



// This random generators are implemented 
// by following POSIX.1-2001 directives.
// ---------------------------------------

uint32_t fastRandom32(void);
void fastRandomSetSeed(uint32_t seed);


    
#endif
