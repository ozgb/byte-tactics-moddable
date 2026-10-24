#include "../util/vec3.h"

struct Unit;

// FUNCTION: 0x40f790
Vec3 __stdcall AddVec3(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}
