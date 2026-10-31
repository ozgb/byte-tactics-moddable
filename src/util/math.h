#ifndef MATH_H
#define MATH_H
struct Vec3;
struct Vec3f;

int __cdecl ScaledSin(int angle, int scale);
int __cdecl ScaledCos(int angle, int scale);
int __stdcall ApproxDistance(int dx, int dy);
double __stdcall DotProduct(float ax, float ay, float az, float bx, float by, float bz);
float __stdcall VectorLengthFloat(float x, float y, float z);

void __stdcall SeedRandom(unsigned int seed);
#include "../../interfaces/services.h"

void __stdcall RotateAboutZ(Vec3* in, Vec3* out, int angle);
void __stdcall RotateAboutY(Vec3* in, Vec3* out, int angle);
void __stdcall RotateAboutX(Vec3* in, Vec3* out, int angle);
Vec3f __stdcall VectorFromToInt(Vec3 from, Vec3 to);

#endif
