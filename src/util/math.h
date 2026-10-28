#ifndef MATH_H
#define MATH_H

int __stdcall ApproxDistance(int dx, int dy);
double __stdcall DotProduct(float ax, float ay, float az, float bx, float by, float bz);
float __stdcall VectorLengthFloat(float x, float y, float z);

int __stdcall ComputeChecksum(unsigned char* data, int len);
void __stdcall SeedRandom(unsigned int seed);
#include "../../interfaces/services.h"

#endif
