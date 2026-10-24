#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <vector>

void* __cdecl operator new(size_t n);
void __cdecl operator delete(void* p);

extern char* g_game;

struct Vec3_00473d50 {
    int x;
    int y;
    int z;

    Vec3_00473d50 operator-(const Vec3_00473d50& o) const
    {
        Vec3_00473d50 r;
        r.x = x - o.x;
        r.y = y - o.y;
        r.z = z - o.z;
        return r;
    }
    int Length() const
    {
        float fx = x;
        float fy = y;
        float fz = z;
        return (int)sqrt(fx * fx + fy * fy + fz * fz);
    }
};

union Fix_00473d50 {
    int whole;
    short half[2];
};

struct NanoParticle {
    Vec3_00473d50 pos;
    Vec3_00473d50 tgt;
    Vec3_00473d50 vel;
    int unused;
    int flags;
    int endTime;
};

class NanoParticles {
public:
    char unknown_0[4];
    int deadline;
    int time;
    std::vector<NanoParticle> records;
    Vec3_00473d50 center;
    Vec3_00473d50 radius;
    Vec3_00473d50 target;
    Vec3_00473d50 spread;

    void Emit();
};

#include "../harness/harness.h"

// FUNCTION: 0x473d50
void NanoParticles::Emit()
{
    int grow = deadline - *(int*)(g_game + 0x38a47) + 1;
    if (grow > 0)
        records.reserve(records.size() + grow * 5);

    int i = 0;
    for (int n = 5; n != 0; n--) {
        NanoParticle e;
        e.pos.x = (int)(((__int64)HarnessViewRand() * radius.x) / 0x8000) + center.x;
        e.pos.y = (int)(((__int64)HarnessViewRand() * radius.y) / 0x8000) + center.y;
        e.pos.z = (int)(((__int64)HarnessViewRand() * radius.z) / 0x8000) + center.z;
        e.tgt.x = (int)(((__int64)HarnessViewRand() * spread.x) / 0x8000) + target.x;
        e.tgt.y = (int)(((__int64)HarnessViewRand() * spread.y) / 0x8000) + target.y;
        e.tgt.z = (int)(((__int64)HarnessViewRand() * spread.z) / 0x8000) + target.z;

        e.vel = e.tgt - e.pos;
        int len = e.vel.Length();
        Fix_00473d50 step;
        step.whole = (int)(((__int64)len << 16) / 0x40000);
        short s = step.half[1];
        if (s != 0) {
            e.vel.x = e.vel.x / s;
            e.vel.y = e.vel.y / s;
            e.vel.z = e.vel.z / s;
            e.endTime = *(int*)(g_game + 0x38a47) + s;
            e.unused = 0x100;
            e.flags = i % 7 + 0xa1;
            records.push_back(e);
        }
        i++;
    }
    time = *(int*)(g_game + 0x38a47) + 1;
}
