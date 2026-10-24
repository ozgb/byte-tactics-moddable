// FLAGS: /Gi
#include <vector>

#pragma pack(push, 1)
struct UnitSyncPacket {
    unsigned char type;
    unsigned char arg;
    unsigned int id;
    int key;
    int value;
};
#pragma pack(pop)

typedef std::vector<UnitSyncPacket> Vec_0046eba0;
typedef void (Vec_0046eba0::*InsertFn_0046eba0)(
    Vec_0046eba0::iterator, Vec_0046eba0::size_type,
    const UnitSyncPacket&);

void __stdcall Push_0046eba0(Vec_0046eba0* v, const UnitSyncPacket& x)
{
    v->push_back(x);
}

// FUNCTION: 0x46eba0 ?insert@?$vector@UUnitSyncPacket@@V?$allocator@UUnitSyncPacket@@@std@@@std@@QAEXPAUUnitSyncPacket@@IABU3@@Z
InsertFn_0046eba0 g_insert_0046eba0 = &Vec_0046eba0::insert;
