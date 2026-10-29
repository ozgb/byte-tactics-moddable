#ifndef WASM_STDLIB_H
#define WASM_STDLIB_H

#include <stddef.h>

extern "C" void* malloc(size_t n);
extern "C" void free(void* p);
extern "C" double atof(const char* s);
extern "C" void exit(int code);
extern "C" char* _itoa(int value, char* dst, int radix);
#define __min(a, b) (((a) < (b)) ? (a) : (b))
#define __max(a, b) (((a) > (b)) ? (a) : (b))

#endif
