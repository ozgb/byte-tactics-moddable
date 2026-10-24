#include <vector>

#pragma pack(push, 1)
struct Elem_0046faf0 {
	unsigned char type;
	unsigned char arg;
	int id;
	int key;
	int value;
};
#pragma pack(pop)

extern "C" int __cdecl GetLocalHumanDpid();
extern "C" void __stdcall SendPacketToPlayer(int a, unsigned int b, void* c, int d);

class PacketSequencer {
public:
	int lastSent;
	char unknown_4[8];
	std::vector<Elem_0046faf0> sentQueue;
	void SendSequenced(Elem_0046faf0* to, Elem_0046faf0* packet);
};

// FUNCTION: 0x46cc10
void PacketSequencer::SendSequenced(Elem_0046faf0* to, Elem_0046faf0* packet)
{
	packet->id = ++lastSent;
	std::vector<Elem_0046faf0>& _v = sentQueue;
	_v.insert(_v.end(), 1, *packet);
	SendPacketToPlayer(GetLocalHumanDpid(), (unsigned int)to, packet, 0xe);
}
