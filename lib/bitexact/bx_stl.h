#ifndef BX_STL_H
#define BX_STL_H

#include "bitexact.h"

#ifdef __cplusplus
extern "C" {
#endif

/* std::random_shuffle(first, last) as MSVC 5's STL compiles it for an int array: draws last - first - 1 times from s, none for an empty or one-element range. */
void bx_random_shuffle_int(bx_rand_state *s, int *first, int *last);

#ifdef __cplusplus
}
#endif

#endif
