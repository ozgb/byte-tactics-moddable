// FLAGS: /Gi
#include <vector>

struct TeleportParticle {
    void* data;
    int f04, f08, f0c, f10, f14, f18, f1c, f20, f24, f28, f2c, f30;
};

typedef std::vector<TeleportParticle> Vec_00473590;
typedef void (Vec_00473590::*InsertFn_00473590)(
    Vec_00473590::iterator, Vec_00473590::size_type, const TeleportParticle&);

void __stdcall Assign_00473590(Vec_00473590* to, const Vec_00473590* from)
{
    *to = *from;
}

// FUNCTION: 0x4758c0 ?insert@?$vector@UTeleportParticle@@V?$allocator@UTeleportParticle@@@std@@@std@@QAEXPAUTeleportParticle@@IABU3@@Z
InsertFn_00473590 g_insert_00473590 = &Vec_00473590::insert;
