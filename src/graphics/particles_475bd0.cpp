// FLAGS: /Gi
#include <vector>

struct ThrustParticle {
    int data;
    int pos0X;
    int pos0Y;
    int pos0Z;
    int pos1X;
    int pos1Y;
    int pos1Z;
    int pos2X;
    int pos2Y;
    int pos2Z;
    int frameCount;
    int frame;
    int tick;
    int period;
    int endTime;
};

typedef std::vector<ThrustParticle> Vec_00475bd0;
typedef void (Vec_00475bd0::*InsertFn_00475bd0)(
    Vec_00475bd0::iterator, Vec_00475bd0::size_type, const ThrustParticle&);

void __stdcall Grow_00475bd0(Vec_00475bd0* records, int extra)
{
    records->reserve(extra + records->size());
}

// FUNCTION: 0x475bd0 ?insert@?$vector@UThrustParticle@@V?$allocator@UThrustParticle@@@std@@@std@@QAEXPAUThrustParticle@@IABU3@@Z
InsertFn_00475bd0 g_insert_00475bd0 = &Vec_00475bd0::insert;
