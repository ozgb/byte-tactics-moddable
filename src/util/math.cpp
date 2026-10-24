#include "win_types.h"
#include <stdlib.h>
#include <math.h>
#include "../../interfaces/host.h"
#include "../state/sim_globals.h"

extern const char g_errorCaption[];

// FUNCTION: 0x4b6b80
void __stdcall ShowErrorBox(const char* message, int unused)
{
    (void)unused;
    host_message(NULL, g_errorCaption, message, HOST_MESSAGE_TOPMOST);
}

// FUNCTION: 0x4b6ba0
int __stdcall ComputeChecksum(unsigned char* data, int len)
{
    unsigned char a = 0, b = 0, c = 0, d = 0;
    for (int i = 0; i < len; i++, data++) {
        a += *data;
        b ^= *data;
        c += i ^ *data;
        d ^= i + *data;
    }
    return (d << 24) | (c << 16) | (b << 8) | a;
}

// FUNCTION: 0x4b6ca0
void __stdcall SeedRandom(unsigned int seed)
{
    g_randomSeed = (seed ^ 0x66e29572) | 1;
}

struct Vec3_004b6cc0 {
    int x;
    int y;
    int z;
};

void __cdecl RotatePoint2D(short angle, int* xy);
void __cdecl RotatePoint2D(int angle, int* xy);

// FUNCTION: 0x4b6cc0
void __stdcall RotateByAngles(Vec3_004b6cc0* in, Vec3_004b6cc0* out, short* angles)
{
    int a[2];
    int b[2];
    a[0] = in->x;
    a[1] = in->y;
    RotatePoint2D(angles[0], a);
    b[0] = a[1];
    b[1] = in->z;
    RotatePoint2D(angles[2], b);
    out->y = b[0];
    a[1] = b[1];
    RotatePoint2D(angles[1], a);
    out->x = a[0];
    out->z = a[1];
}

// FUNCTION: 0x4b6d50
void __stdcall RotateAboutZ(Vec3_004b6cc0* in, Vec3_004b6cc0* out, int angle)
{
    int xy[2];
    xy[0] = in->x;
    xy[1] = in->y;
    RotatePoint2D(angle, xy);
    out->x = xy[0];
    out->y = xy[1];
    out->z = in->z;
}

// FUNCTION: 0x4b6da0
void __stdcall RotateAboutY(Vec3_004b6cc0* in, Vec3_004b6cc0* out, int angle)
{
    int xz[2];
    xz[0] = in->x;
    xz[1] = in->z;
    RotatePoint2D(angle, xz);
    out->x = xz[0];
    out->y = in->y;
    out->z = xz[1];
}

// FUNCTION: 0x4b6df0
void __stdcall RotateAboutX(Vec3_004b6cc0* in, Vec3_004b6cc0* out, int angle)
{
    int yz[2];
    yz[0] = in->y;
    yz[1] = in->z;
    RotatePoint2D(angle, yz);
    out->x = in->x;
    out->y = yz[0];
    out->z = yz[1];
}

// FUNCTION: 0x4b6e40
int __stdcall ApproxDistance(int dx, int dy)
{
    int a = abs(dx);
    int b = abs(dy);
    if (a > b)
        return (b >> 2) + a;
    return (a >> 2) + b;
}

// FUNCTION: 0x4b6e80
double __stdcall DotProduct(float ax, float ay, float az,
                               float bx, float by, float bz)
{
    double result = bx * ax;
    result = result + by * ay;
    result = result + bz * az;
    return result;
}

struct Vec3f_004b6eb0 {
    float x;
    float y;
    float z;
};

// FUNCTION: 0x4b6eb0
Vec3f_004b6eb0 __stdcall VectorFromTo(Vec3f_004b6eb0 from, Vec3f_004b6eb0 to)
{
    Vec3f_004b6eb0 r;
    r.x = to.x - from.x;
    r.y = to.y - from.y;
    r.z = to.z - from.z;
    return r;
}

// FUNCTION: 0x4b6f00
Vec3f_004b6eb0 __stdcall VectorFromToInt(Vec3_004b6cc0 from, Vec3_004b6cc0 to)
{
    Vec3f_004b6eb0 r;
    r.x = (float)(to.x - from.x);
    r.y = (float)(to.y - from.y);
    r.z = (float)(to.z - from.z);
    return r;
}

// FUNCTION: 0x4b6f70
Vec3f_004b6eb0 __stdcall CrossProduct(Vec3f_004b6eb0 a, Vec3f_004b6eb0 b)
{
#ifdef __clang__
    double yz = (double)a.y * b.z;
    double zy = (double)a.z * b.y;
    double zx = (double)a.z * b.x;
    double xz = (double)a.x * b.z;
    double xy = (double)a.x * b.y;
    double yx = (double)a.y * b.x;
#else
    float yz = a.y * b.z;
    float zy = a.z * b.y;
    float zx = a.z * b.x;
    float xz = a.x * b.z;
    float xy = a.x * b.y;
    float yx = a.y * b.x;
#endif
    Vec3f_004b6eb0 r;
    r.z = (float)(xy - yx);
    r.x = (float)(yz - zy);
    r.y = (float)(zx - xz);
    return r;
}

// FUNCTION: 0x4b6ff0
Vec3f_004b6eb0 __stdcall NormalizeVector(Vec3f_004b6eb0 v)
{
#ifdef __clang__
    struct { double x, y, z; } sq;
    sq.x = (double)v.x * v.x;
    sq.y = (double)v.y * v.y;
    sq.z = (double)v.z * v.z;
    double len = sqrt(sq.x + sq.y + sq.z);
#else
    Vec3f_004b6eb0 sq;
    sq.x = v.x * v.x;
    sq.y = v.y * v.y;
    sq.z = v.z * v.z;
    float len = sqrt(sq.x + sq.y + sq.z);
#endif
    v.x /= len;
    v.y /= len;
    v.z /= len;
    return v;
}

// FUNCTION: 0x4b7070
float __stdcall VectorLengthFloat(float x, float y, float z)
{
    return sqrtf(x * x + y * y + z * z);
}
